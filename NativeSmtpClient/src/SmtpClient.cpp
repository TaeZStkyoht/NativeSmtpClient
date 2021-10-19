#include "SmtpClient.h"

#include <asio.hpp>
#include <asio/ssl.hpp>
#include <charconv>

using namespace std;
using namespace asio;
using namespace error;
using namespace ssl;
using namespace ip;

using endpoint = tcp::endpoint;
using resolver = tcp::resolver;
using handshake_type = stream_base::handshake_type;

using namespace NativeSmtpClient;

class SmtpClient::Impl final {
public:
	Impl(string&& host, int port, string&& user, string&& password) noexcept :
		_host(host),
		_port(port),
		_user(user),
		_password(password) {
	}

	~Impl() noexcept {
		Quit();
	}

	bool SendWithoutQuit(const Mail& mail) noexcept {
		return SendWithoutQuit(mail, CreateSendMutexLockGuard());
	}

	bool Send(const Mail& mail) noexcept {
		const auto lockGuard = CreateSendMutexLockGuard();
		const bool result = SendWithoutQuit(mail, lockGuard);
		Quit(lockGuard);
		return result;
	}

	void Quit() noexcept {
		Quit(CreateSendMutexLockGuard());
	}

	constexpr const string& ErrorMessage() const noexcept { return _errorMessage; }
	constexpr string& ErrorMessage() noexcept { return _errorMessage; }

private:
	class MailException final : public exception {
	public:
		MailException(string_view stage, const error_code& ec) : _message(PrepareMessage(stage, ec.message())) {}
		MailException(string_view stage, string_view msg) : _message(PrepareMessage(stage, msg)) {}

		const char* what() const noexcept final { return _message.c_str(); }

	private:
		static string PrepareMessage(string_view stage, string_view msg) {
			string message(stage);
			message += " error: ";
			message += msg;
			return message;
		}

		const string _message;
	};

	enum class StatusCodeHundreds : unsigned short {
		Ok = 2,
		Redirection,
		Error,
		ServerError
	};

	shared_ptr<lock_guard<mutex>> CreateSendMutexLockGuard() noexcept try {
		return make_shared<lock_guard<mutex>>(_sendMutex);
	}
	catch (...) {
		ErrorMessage("Unkown error! This error shouldn't happen! Please, contact with library owner.");
		return nullptr;
	}

	bool SendWithoutQuit(const Mail& mail, shared_ptr<lock_guard<mutex>> lockGuard) noexcept {
		if (!lockGuard)
			return false;

		bool result;

		try {
			if (!_connected) {
				Connect();
				Ehlo();
				StartTls();
				Ehlo();
				AuthLogin();
				_connected = true;
			}

			SenderReciever(mail);
			Data(mail);

			result = true;
		}
		catch (const exception& ex) {
			ErrorMessage(ex);
			result = false;
		}
		catch (...) {
			ErrorMessage("Unknown error!");
			result = false;
		}

		return result;
	}

	void Quit(shared_ptr<lock_guard<mutex>> lockGuard) noexcept {
		if (lockGuard && _connected) {
			try {
				_stage = "Quiting";
				Write("QUIT");
			}
			catch (...) {}

			_ioContext.reset();
			_socket.reset();
			_context.reset();
			_secureSocket.reset();
			_tlsActive = false;
			_connected = false;
		}
	}

	void ErrorMessage(const char* errorMessage) noexcept {
		try { _errorMessage = errorMessage; }
		catch (...) { _errorMessage.clear(); }
	}

	void ErrorMessage(const exception& ex) noexcept {
		try { _errorMessage = ex.what(); }
		catch (...) { _errorMessage.clear(); }
	}

	void CheckCode(string_view str, StatusCodeHundreds expectedStatusCodeHundreds) const {
		switch (unsigned short statusCodeHundreds; from_chars(str.data(), str.data() + 3, statusCodeHundreds).ec) {
		default:
			if (statusCodeHundreds / 100 == static_cast<unsigned short>(expectedStatusCodeHundreds))
				break;
			[[fallthrough]];
		case errc::invalid_argument:
		case errc::result_out_of_range:
			throw MailException(_stage, str);
		}
	}

	void CheckError() const {
		if (_ec)
			throw MailException(_stage, _ec);
	}

	string Read() {
		constexpr char _bufferEnd = '\n';
		asio::streambuf sbuf;
		if (_tlsActive)
			read_until(*_secureSocket, sbuf, _bufferEnd, _ec);
		else
			read_until(*_socket, sbuf, _bufferEnd, _ec);

		CheckError();
		return { istreambuf_iterator<char>(&sbuf), istreambuf_iterator<char>() };
	}

	string Write(const string& message, bool withRead = true) {
		const string msgWithCmd = message + _cmdEnd;
		if (_tlsActive)
			write(*_secureSocket, buffer(msgWithCmd), _ec);
		else
			write(*_socket, buffer(msgWithCmd), _ec);

		CheckError();

		if (withRead)
			return Read();

		return {};
	}

	void WriteAndCheckCode(const string& message, StatusCodeHundreds expectedStatusCodeHundreds) {
		CheckCode(Write(message), expectedStatusCodeHundreds);
	}

	void Connect() {
		_ioContext = make_unique<io_context>();

		_stage = "Endpoint resolve";
		const auto resolverResult = resolver(*_ioContext).resolve(resolver::query(_host, to_string(_port)), _ec);
		CheckError();

		_stage = "Connection";
		_socket = make_unique<tcp::socket>(*_ioContext);
		_socket->connect(resolverResult->endpoint(), _ec);
		CheckError();

		CheckCode(Read(), StatusCodeHundreds::Ok);
	}

	void Ehlo() {
		_stage = "EHLO";
		const string hostName = host_name();
		try {
			WriteAndCheckCode("EHLO " + hostName, StatusCodeHundreds::Ok);
		}
		catch (const MailException&) {
			WriteAndCheckCode("HELO " + hostName, StatusCodeHundreds::Ok);
		}
	}

	void StartTls() {
		_stage = "Starting Tls";
		WriteAndCheckCode("STARTTLS", StatusCodeHundreds::Ok);

		_context = make_unique<context>(context::tls_client);
		_secureSocket = make_unique<stream<tcp::socket&>>(*_socket, *_context);

		_stage = "Setting SNI hostname";
		if (!SSL_set_tlsext_host_name(_secureSocket->native_handle(), _host.c_str())) {
			_ec = error_code(static_cast<int>(ERR_get_error()), get_ssl_category());
			CheckError();
		}

		_stage = "Setting verify mode";
		_secureSocket->set_verify_mode(verify_none, _ec);
		CheckError();

		_stage = "Handshake";
		_secureSocket->handshake(handshake_type::client, _ec);
		CheckError();

		_tlsActive = true;
	}

	void AuthLogin() {
		_stage = "Auth login";
		WriteAndCheckCode("AUTH LOGIN", StatusCodeHundreds::Redirection);
		WriteAndCheckCode(base64Encode(_user), StatusCodeHundreds::Redirection);
		WriteAndCheckCode(base64Encode(_password), StatusCodeHundreds::Ok);
	}

	void SenderReciever(const Mail& mail) {
		_stage = "Setting sender & reciever";
		string command = "MAIL FROM: ";
		command += mail.Sender().MailWithChevrons();
		WriteAndCheckCode(command, StatusCodeHundreds::Ok);

		for (const MailAddress& recipient : mail.Recipients()) {
			command = "RCPT TO: ";
			command += recipient.MailWithChevrons();
			WriteAndCheckCode(command, StatusCodeHundreds::Ok);
		}

		for (const MailAddress& cc : mail.Ccs()) {
			command = "RCPT TO: ";
			command += cc.MailWithChevrons();
			WriteAndCheckCode(command, StatusCodeHundreds::Ok);
		}

		for (const MailAddress& bcc : mail.Bccs()) {
			command = "RCPT TO: ";
			command += bcc.MailWithChevrons();
			WriteAndCheckCode(command, StatusCodeHundreds::Ok);
		}
	}

	void Data(const Mail& mail) {
		_stage = "Setting data";
		WriteAndCheckCode("DATA", StatusCodeHundreds::Redirection);

		string command = "From: ";
		command += mail.Sender();
		command += _cmdEnd;

		AppendRecievers(command, "To", mail.Recipients());
		AppendRecievers(command, "Cc", mail.Ccs());
		AppendRecievers(command, "Bcc", mail.Bccs());

		if (mail.IsHtml()) {
			command += "Content-Type: text/html; charset=utf-8";
			command += _cmdEnd;
		}

		command += "Subject: ";
		command += mail.Subject();
		command += _cmdEnd;
		command += _cmdEnd;

		command += mail.Body();
		command += _cmdEnd;
		command += _cmdEnd;

		command += '.';

		WriteAndCheckCode(command, StatusCodeHundreds::Ok);
	}

	static string base64Encode(string_view str) {
		string ret;

		constexpr string_view base64_chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
		constexpr size_t tempStrMaxSize = 3;
		basic_string<unsigned char> tempStr;
		for (char c : str) {
			tempStr.push_back(c);
			if (tempStr.size() == tempStrMaxSize) {
				ret.push_back(base64_chars[(tempStr[0] & 0xfc) >> 2]);
				ret.push_back(base64_chars[((tempStr[0] & 0x03) << 4) + ((tempStr[1] & 0xf0) >> 4)]);
				ret.push_back(base64_chars[((tempStr[1] & 0x0f) << 2) + ((tempStr[2] & 0xc0) >> 6)]);
				ret.push_back(base64_chars[tempStr[2] & 0x3f]);
				tempStr.clear();
			}
		}

		const size_t tempStrSize = tempStr.size();
		if (tempStrSize) {
			for (size_t i = tempStrSize; i < tempStrMaxSize; ++i)
				tempStr.push_back('\0');

			{
				const size_t maxJ = tempStrSize + 1;
				for (size_t i = 0; i < maxJ; ++i) {
					switch (i) {
					case 0:
						ret.push_back(base64_chars[(tempStr[0] & 0xfc) >> 2]);
						break;
					case 1:
						ret.push_back(base64_chars[((tempStr[0] & 0x03) << 4) + ((tempStr[1] & 0xf0) >> 4)]);
						break;
					case 2:
						ret.push_back(base64_chars[((tempStr[1] & 0x0f) << 2) + ((tempStr[2] & 0xc0) >> 6)]);
						break;
					case 3:
						ret.push_back(base64_chars[tempStr[2] & 0x3f]);
						break;
					default:
						break;
					}
				}
			}

			for (size_t i = tempStrSize; i < tempStrMaxSize; ++i)
				ret.push_back('=');
		}

		return ret;
	}

	static void AppendRecievers(string& command, string_view recieverTitle, const vector<MailAddress>& addresses) {
		if (!addresses.empty()) {
			constexpr const char* mailSplitter = ",\r\n ";
			command += recieverTitle;
			command += ": ";
			for (const MailAddress& reciever : addresses) {
				command += reciever;
				command += mailSplitter;
			}
			command.erase(command.find_last_of(','), 1);
			command.pop_back();
		}
	}

	const string _host;
	const int _port;
	const string _user;
	const string _password;

	error_code _ec;

	unique_ptr<io_context> _ioContext;
	unique_ptr<tcp::socket> _socket;
	unique_ptr<context> _context;
	unique_ptr<stream<tcp::socket&>> _secureSocket;

	string _stage;
	string _errorMessage;

	mutex _sendMutex;

	atomic<bool> _connected = false;
	bool _tlsActive = false;

	const char* const _cmdEnd = "\r\n";
};

SmtpClient::SmtpClient(string host, int port, string user, string password) noexcept : _impl(make_unique<Impl>(move(host), port, move(user), move(password))) {}

bool SmtpClient::SendWithoutQuit(const Mail& mail) noexcept {
	return !mail.Empty(_impl->ErrorMessage()) && _impl->SendWithoutQuit(mail);
}

bool SmtpClient::Send(const Mail& mail) noexcept {
	return !mail.Empty(_impl->ErrorMessage()) && _impl->Send(mail);
}

void SmtpClient::Quit() noexcept {
	_impl->Quit();
}

const string& SmtpClient::ErrorMessage() const noexcept {
	return _impl->ErrorMessage();
}
