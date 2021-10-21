#pragma once

#include "Mail.h"

#include <memory>

namespace NativeSmtpClient {
	class SmtpClient {
	public:
		SmtpClient(std::string host, int port, std::string user, std::string password) noexcept;

		bool SendWithoutQuit(const Mail& mail) noexcept;
		bool Send(const Mail& mail) noexcept;
		void Quit() noexcept;
		const std::string& ErrorMessage() const noexcept;

	private:
		class Impl;
		std::shared_ptr<Impl> _impl;
	};
}
