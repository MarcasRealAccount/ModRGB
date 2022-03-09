#pragma once

#include "ModRGB/Utils/Core.h"

#include <cstdint>

#include <string_view>

namespace ModRGB::Networking
{
	using ErrorReportCallback = void (*)(std::uint32_t errorCode);
	void SetErrorReportingCallback(ErrorReportCallback callback);

	enum class ESocketType
	{
		TCP,
		UDP
	};

	struct IPv4Address
	{
	public:
		IPv4Address(std::uint32_t address) : m_Num(address) {}
		IPv4Address(std::uint8_t b0, std::uint8_t b1, std::uint8_t b2, std::uint8_t b3) : m_Bytes { b0, b1, b2, b3 } {}

		union
		{
			std::uint32_t m_Num;
			std::uint8_t  m_Bytes[4];
		};
	};

	struct IPv4Socket
	{
	public:
		IPv4Socket(ESocketType type);
		IPv4Socket(IPv4Socket&& move) noexcept;
		~IPv4Socket();

		auto getType() const { return m_Type; }

		void setLocalAddress(IPv4Address address, std::uint16_t port);
		auto getLocalAddress() const { return m_LocalAddress; }
		auto getLocalPort() const { return m_LocalPort; }
		auto getRemoteAddress() const { return m_RemoteAddress; }
		auto getRemotePort() const { return m_RemotePort; }

		void setWriteTimeout(std::uint32_t timeout);
		void setReadTimeout(std::uint32_t timeout);
		auto getWriteTimeout() const { return m_WriteTimeout; }
		auto getReadTimeout() const { return m_ReadTimeout; }

		std::size_t read(void* buf, std::size_t len);
		std::size_t readFrom(void* buf, std::size_t len, IPv4Address& address, std::uint16_t& port);
		std::size_t write(const void* buf, std::size_t len);
		std::size_t writeTo(const void* buf, std::size_t len, IPv4Address address, std::uint16_t port);

		void open();
		void close();

		void closeW();
		void closeR();
		void closeRW();

		bool connectResolve(std::string_view host, std::uint16_t port);
		bool connect(IPv4Address address, std::uint16_t port);

		bool       listen(std::uint32_t backlog);
		IPv4Socket accept();

		auto opened() const { return m_Opened; }
		auto errored() const { return m_Errored; }

	private:
		ESocketType   m_Type;
		IPv4Address   m_LocalAddress;
		IPv4Address   m_RemoteAddress;
		std::uint16_t m_LocalPort;
		std::uint16_t m_RemotePort;

		std::uint32_t m_WriteTimeout = 2000;
		std::uint32_t m_ReadTimeout  = 2000;

		bool m_Opened  = false;
		bool m_Errored = false;

		std::uintptr_t m_Socket = ~0ULL;
	};
} // namespace ModRGB::Networking