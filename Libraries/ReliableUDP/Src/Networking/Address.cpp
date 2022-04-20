#include "ReliableUDP/Networking/Address.h"

#include <sstream>

namespace ReliableUDP::Networking
{
	EAddressType Address::getType() const
	{
		for (std::size_t i = 3; i < 8; ++i)
			if (m_IPv6.m_Segments[i])
				return EAddressType::IPv6;
		return m_IPv6.m_Segments[2] == 0xFFFF ? EAddressType::IPv4 : EAddressType::IPv6;
	}

	std::string Address::toString() const
	{
		if (isIPv4())
		{
			std::ostringstream stream;
			for (std::size_t i = 4; i > 0; --i)
			{
				if (i < 4)
					stream << '.';
				stream << static_cast<std::uint32_t>(m_IPv4.m_Bytes[i - 1]);
			}
			return stream.str();
		}
		else
		{
			std::ostringstream stream;
			stream << std::hex;

			for (std::size_t i = 8; i > 0; --i)
			{
				if (i < 8)
					stream << ':';
				stream << static_cast<std::uint32_t>(m_IPv6.m_Segments[i - 1]);
			}

			std::string str = stream.str();

			std::size_t largestConsecStart = ~0ULL;
			std::size_t largestConsecEnd   = ~0ULL;
			std::size_t totalColons        = 0;

			std::size_t start  = 0;
			std::size_t colons = 1;

			for (std::size_t i = 0; i < str.size(); ++i)
			{
				auto c = str[i];

				switch (c)
				{
				case ':':
					if (!colons)
						start = i;
					++colons;
					break;
				case '0':
					break;
				default:
					if (colons > 1 && totalColons < colons)
					{
						largestConsecStart = start;
						largestConsecEnd   = i;
						totalColons        = colons;
					}
					colons = 0;
					break;
				}
			}

			if (totalColons)
				str.replace(largestConsecStart, largestConsecEnd - largestConsecStart, "::");

			return str;
		}
	}

	bool Address::operator==(Address other) const
	{
		return memcmp(m_IPv6.m_Bytes, other.m_IPv6.m_Bytes, 16) == 0;
	}
} // namespace ReliableUDP::Networking