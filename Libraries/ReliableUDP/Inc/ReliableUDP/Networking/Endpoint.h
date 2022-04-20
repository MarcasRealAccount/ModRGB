#pragma once

#include "Address.h"

#include <cstdint>

#include <string>
#include <string_view>

namespace ReliableUDP::Networking
{
	struct Endpoint
	{
	public:
		static Endpoint    ResolveFromHost(std::string_view node, std::string_view service, EAddressType type = EAddressType::Both);
		static std::string ToHost(const Endpoint& endpoint);

	public:
		Endpoint() : m_Port(0U) {}
		Endpoint(Address address, std::uint16_t port) : m_Address(address), m_Port(port) {}
		Endpoint(std::string_view node, std::string_view service, EAddressType type = EAddressType::Both);

		bool         isIPv4() const { return m_Address.isIPv4(); }
		bool         isValid() const { return m_Address.isValid(); }
		EAddressType getType() const { return m_Address.getType(); }

		std::string toString() const;
		std::string toHost() const;

		bool operator==(Endpoint other) const;

	public:
		Address       m_Address;
		std::uint16_t m_Port;
	};
} // namespace ReliableUDP::Networking