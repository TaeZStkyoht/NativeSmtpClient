#include "MailAddress.h"

using namespace std;

using namespace NativeSmtpClient;

MailAddress::operator string() const {
	string value(_name);
	value += " <";
	value += _mail;
	value += '>';
	return value;
}

string MailAddress::MailWithChevrons() const {
	string value(1, '<');
	value += _mail;
	value += '>';
	return value;
}
