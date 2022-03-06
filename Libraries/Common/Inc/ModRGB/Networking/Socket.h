#pragma once

#include "ModRGB/Utils/Core.h"

#include <cstdint>

#include <string_view>

namespace ModRGB::Networking
{
	enum class ESocketType
	{
		TCP,
		UDP
	};

	struct IPv4Address
	{
	public:
		IPv4Address(std::uint32_t address) : m_Num(address) {}

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
		~IPv4Socket();

		bool setAddress(std::string_view host, std::uint16_t port);
		void setAddress(IPv4Address address);
		void setPort(std::uint16_t port);

		std::size_t read(void* buf, std::size_t len);
		std::size_t readFrom(void* buf, std::size_t len, IPv4Address& address, std::uint16_t& port);
		std::size_t write(void* buf, std::size_t len);
		std::size_t writeTo(void* buf, std::size_t len, IPv4Address address, std::uint16_t port);

		void open();
		void close();

		void closeW();
		void closeR();
		void closeRW();

		bool       listen(std::uint32_t backlog);
		IPv4Socket accept();

		auto opened() const { return m_Opened; }

	private:
		ESocketType   m_Type;
		IPv4Address   m_Address;
		std::uint16_t m_Port;

		bool m_Opened = false;

		std::uintptr_t m_Socket = ~0ULL;
	};
} // namespace ModRGB::Networking