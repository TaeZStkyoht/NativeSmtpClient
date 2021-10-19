#include "Mail.h"

#include <fstream>

using namespace std;

using namespace NativeSmtpClient;

Mail::Mail(MailAddress sender, const MailAddress& recipient) : _sender(move(sender)) {
	FillMailAddress(_recipients, recipient);
}

void Mail::Recipients(const vector<MailAddress>& recipients) {
	FillMailAddresses(_recipients, recipients);
}

void Mail::Ccs(const vector<MailAddress>& ccs) {
	FillMailAddresses(_ccs, ccs);
}

void Mail::Bccs(const vector<MailAddress>& bccs) {
	FillMailAddresses(_bccs, bccs);
}

void Mail::AddRecipient(const MailAddress& recipient) {
	FillMailAddress(_recipients, recipient);

}
void Mail::AddCc(const MailAddress& cc) {
	FillMailAddress(_ccs, cc);
}

void Mail::AddBcc(const MailAddress& bcc) {
	FillMailAddress(_bccs, bcc);
}

void Mail::Body(string body, bool isHtml) noexcept {
	_body = std::move(body);
	_isHtml = isHtml;
}

void Mail::BodyFromFile(const string& filePath, bool isHtml) {
	if (ifstream stream(filePath); stream.is_open())
		_body = { istreambuf_iterator<char>(stream), {} };
	_isHtml = isHtml;
}

bool Mail::Empty(string& errorMessage) const noexcept {
	errorMessage.clear();

	if (_sender.Empty()) {
		try { errorMessage = "Sender is empty!"; }
		catch (...) {};
		return true;
	}

	if (_recipients.empty()) {
		try { errorMessage = "Recipients are empty!"; }
		catch (...) {}
		return true;
	}

	return false;
}

void Mail::FillMailAddress(vector<MailAddress>& container, const MailAddress& mailAddress) {
	if (!mailAddress.Empty())
		container.push_back(mailAddress);
}

void Mail::FillMailAddresses(vector<MailAddress>& container, const vector<MailAddress>& mailAddresses) {
	container.reserve(container.size() + mailAddresses.size());
	for (const MailAddress& mailAddress : mailAddresses)
		FillMailAddress(container, mailAddress);
	container.shrink_to_fit();
}
