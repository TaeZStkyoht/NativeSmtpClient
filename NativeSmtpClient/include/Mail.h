#pragma once

#include "MailAddress.h"

#include <vector>

namespace NativeSmtpClient {
	class Mail {
	public:
		Mail(MailAddress sender, const MailAddress& recipient);

		void Recipients(const std::vector<MailAddress>& recipients);
		void Ccs(const std::vector<MailAddress>& ccs);
		void Bccs(const std::vector<MailAddress>& bccs);

		void AddRecipient(const MailAddress& recipient);
		void AddCc(const MailAddress& cc);
		void AddBcc(const MailAddress& bcc);

		void Subject(std::string subject) noexcept { _subject = std::move(subject); }
		void Body(std::string body, bool isHtml = false) noexcept;
		void BodyFromFile(const std::string& filePath, bool isHtml = false);

		constexpr const MailAddress& Sender() const noexcept { return _sender; }
		constexpr const std::vector<MailAddress>& Recipients() const noexcept { return _recipients; }
		constexpr const std::vector<MailAddress>& Ccs() const noexcept { return _ccs; }
		constexpr const std::vector<MailAddress>& Bccs() const noexcept { return _bccs; }

		constexpr const std::string& Subject() const noexcept { return _subject; }
		constexpr const std::string& Body() const noexcept { return _body; }
		constexpr const bool IsHtml() const noexcept { return _isHtml; }

		bool Empty(std::string& errorMessage) const noexcept;

	private:
		static void FillMailAddress(std::vector<MailAddress>& container, const MailAddress& mailAddress);
		static void FillMailAddresses(std::vector<MailAddress>& container, const std::vector<MailAddress>& mailAddresses);

		const MailAddress _sender;
		std::vector<MailAddress> _recipients;
		std::vector<MailAddress> _ccs;
		std::vector<MailAddress> _bccs;

		std::string _subject;
		std::string _body;
		bool _isHtml;
	};
}
