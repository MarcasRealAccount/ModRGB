#include "ReliableUDP/PacketHeader.h"

namespace ReliableUDP
{
	bool IsMagicNumberValid(std::uint16_t magicNumber)
	{
		return (magicNumber & 0x3FFF) == s_MagicNumber;
	}

	EPacketHeaderType GetPacketHeaderType(std::uint16_t magicNumber)
	{
		return static_cast<EPacketHeaderType>((magicNumber >> 14) & 0b11);
	}
} // namespace ReliableUDP