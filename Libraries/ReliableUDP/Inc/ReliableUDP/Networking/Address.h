#pragma once

#include <cstdint>

#include <string>
#include <string_view>

namespace ReliableUDP::Networking
{
	enum class EAddressType
	{
		Both,
		IPv4,
		IPv6
	};

	struct IPv4Address
	{
	public:
		IPv4Address() : m_Value(0U) {}
		IPv4Address(std::uint32_t address) : m_Value(address) {}
		IPv4Address(std::uint8_t b0, std::uint8_t b1, std::uint8_t b2, std::uint8_t b3) : m_Bytes { b0, b1, b2, b3 } {}

	public:
		union
		{
			std::uint32_t m_Value;
			std::uint8_t  m_Bytes[4];
		};
	};

	struct IPv6Address
	{
	public:
		IPv6Address() : m_Segments { 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U } {}
		IPv6Address(std::uint32_t ipv4, std::uint16_t s3, std::uint16_t s4, std::uint16_t s5, std::uint16_t s6, std::uint16_t s7)
		    : IPv6Address(ipv4 & 0xFFFF, (ipv4 >> 16) & 0xFFFF, 0xFFFF, s3, s4, s5, s6, s7) {}
		IPv6Address(std::uint16_t s0, std::uint16_t s1, std::uint16_t s2, std::uint16_t s3, std::uint16_t s4, std::uint16_t s5, std::uint16_t s6, std::uint16_t s7) : m_Segments { s0, s1, s2, s3, s4, s5, s6, s7 } {}
		IPv6Address(std::uint8_t b0, std::uint8_t b1, std::uint8_t b2, std::uint8_t b3, std::uint8_t b4, std::uint8_t b5, std::uint8_t b6, std::uint8_t b7, std::uint8_t b8, std::uint8_t b9, std::uint8_t b10, std::uint8_t b11, std::uint8_t b12, std::uint8_t b13, std::uint8_t b14, std::uint8_t b15) : m_Bytes { b0, b1, b2, b3, b4, b5, b6, b7, b8, b9, b10, b11, b12, b13, b14, b15 } {}

	public:
		union
		{
			std::uint16_t m_Segments[8];
			std::uint8_t  m_Bytes[16];
		};
	};

	struct Address
	{
	public:
		Address() : m_IPv6() {}
		Address(IPv4Address address) : m_IPv6(address.m_Value, 0U, 0U, 0U, 0U, 0U) {}
		Address(IPv6Address address) : m_IPv6(address) {}

		bool         isIPv4() const { return getType() == EAddressType::IPv4; }
		EAddressType getType() const;

		std::string toString() const;

	public:
		union
		{
			IPv4Address m_IPv4;
			IPv6Address m_IPv6;
		};
	};
} // namespace ReliableUDP::Networking