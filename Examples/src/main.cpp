#include <SmtpClient.h>

#include <iostream>
#include <future>
#include <chrono>

using namespace std;
using namespace chrono;

using namespace NativeSmtpClient;

static const string host = "smtp.do-not-use.com"; // i.e. smtp.gmail.com
static constexpr unsigned int port = 123; // i.e. 587 for smtp.gmail.com
static const string user = "user@do-not-use.com";
static const string password = "doNotUsePass";

static const string mailFromName = "From Name";
static const string mailFromMail = "from@do-not-use.com";

static const string mailToName = "To Name";
static const string mailToMail = "to@do-not-use.com";

static const string ccName = "Cc Name";
static const string ccMail = "cc@do-not-use.com";

static const string bccName = "Bcc Name";
static const string bccMail = "bcc@do-not-use.com";

static const string subject = "Hi!";
static const string mailBody = "Hey, good?";
static const string mailBodyFile = "resource/Base.htm";

static bool SimpleMail() {
	SmtpClient smtpClient(host, port, user, password);
	Mail mail({ mailFromName, mailFromMail }, { mailToName, mailToMail });
	mail.Subject(subject);
	mail.Body(mailBody);

	if (!smtpClient.Send(mail)) {
		cerr << smtpClient.ErrorMessage() << endl;
		return false;
	}

	return true;
}

static bool SimpleMailWithCcBcc() {
	SmtpClient smtpClient(host, port, user, password);
	Mail mail({ mailFromName, mailFromMail }, { mailToName, mailToMail });
	mail.AddCc({ ccName, ccMail });
	mail.AddBcc({ bccName, bccMail });
	mail.Subject(subject);
	mail.Body(mailBody);

	if (!smtpClient.Send(mail)) {
		cerr << smtpClient.ErrorMessage() << endl;
		return false;
	}

	return true;
}

static bool MultipleMail() {
	SmtpClient smtpClient(host, port, user, password);

	{
		Mail mail1({ mailFromName, mailFromMail }, { mailToName, mailToMail });
		mail1.Subject(subject);
		mail1.Body(mailBody);

		if (!smtpClient.SendWithoutQuit(mail1)) {
			cerr << smtpClient.ErrorMessage() << endl;
			return false;
		}
	}

	Mail mail2({ mailFromName, mailFromMail }, { mailToName, mailToMail });
	mail2.Subject(subject);
	mail2.Body(mailBody);

	if (!smtpClient.Send(mail2)) {
		cerr << smtpClient.ErrorMessage() << endl;
		return false;
	}

	return true;
}

static bool MultithreadSingleSmtpClient() {
	SmtpClient smtpClient(host, port, user, password);

	future mailFuture = async(launch::async,
		[&]() {
			Mail mail1({ mailFromName, mailFromMail }, { mailToName, mailToMail });
			mail1.Subject(subject);
			mail1.BodyFromFile(mailBodyFile, true);

			if (!smtpClient.SendWithoutQuit(mail1)) {
				cerr << smtpClient.ErrorMessage() << endl;
				return false;
			}

			return true;
		}
	);

	Mail mail2({ mailFromName, mailFromMail }, { mailToName, mailToMail });
	mail2.Subject(subject);
	mail2.Body(mailBody);

	bool result = true;
	if (!smtpClient.SendWithoutQuit(mail2)) {
		cerr << smtpClient.ErrorMessage() << endl;
		result = false;
	}

	return result && mailFuture.get();
}

static bool MultithreadMultipleSmtpClient() {
	future mailFuture = async(launch::async,
		[]() {
			SmtpClient smtpClient1(host, port, user, password);
			Mail mail1({ mailFromName, mailFromMail }, { mailToName, mailToMail });
			mail1.Subject(subject);
			mail1.BodyFromFile(mailBodyFile, true);

			if (!smtpClient1.SendWithoutQuit(mail1)) {
				cerr << smtpClient1.ErrorMessage() << endl;
				return false;
			}

			return true;
		}
	);

	SmtpClient smtpClient2(host, port, user, password);
	Mail mail2({ mailFromName, mailFromMail }, { mailToName, mailToMail });
	mail2.Subject(subject);
	mail2.Body(mailBody);

	bool result = true;
	if (!smtpClient2.SendWithoutQuit(mail2)) {
		cerr << smtpClient2.ErrorMessage() << endl;
		result = false;
	}

	return result && mailFuture.get();
}

int main() {
	vector<bool> results;
	results.push_back(SimpleMail());
	results.push_back(SimpleMailWithCcBcc());
	results.push_back(MultipleMail());
	results.push_back(MultithreadSingleSmtpClient());
	results.push_back(MultithreadMultipleSmtpClient());
	return all_of(results.cbegin(), results.cend(), [](bool val) { return val; }) ? EXIT_SUCCESS : EXIT_FAILURE;
}
