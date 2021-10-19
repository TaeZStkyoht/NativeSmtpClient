#pragma once

#include <string>

namespace NativeSmtpClient {
	class MailAddress {
	public:
		MailAddress(std::string name, std::string mail) noexcept : _name(std::move(name)), _mail(std::move(mail)) {}

		bool Empty() const noexcept { return _mail.empty(); }

		operator std::string() const;
		std::string MailWithChevrons() const;

	private:
		const std::string _name;
		const std::string _mail;
	};
}
