#include "ReliableUDP/Networking/Endpoint.h"

#include <sstream>

namespace ReliableUDP::Networking
{
	Endpoint::Endpoint(std::string_view node, std::string_view service, EAddressType type)
	{
		*this = ResolveFromHost(node, service, type);
	}

	std::string Endpoint::toString() const
	{
		if (isIPv4())
		{
			std::ostringstream stream;
			stream << m_Address.toString() << ':' << m_Port;
			return stream.str();
		}
		else
		{
			std::ostringstream stream;
			stream << '[' << m_Address.toString() << "]:" << m_Port;
			return stream.str();
		}
	}

	std::string Endpoint::toHost() const
	{
		return ToHost(*this);
	}

	bool Endpoint::operator==(Endpoint other) const
	{
		return m_Address == other.m_Address && m_Port == other.m_Port;
	}
} // namespace ReliableUDP::Networking