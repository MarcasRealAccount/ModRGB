#include "ReliableUDP/PacketHeader.h"

namespace ReliableUDP
{
	EPacketHeaderType GetPacketHeaderType(std::uint32_t magicNumber)
	{
		return static_cast<EPacketHeaderType>((magicNumber >> 30) & 0b11);
	}
} // namespace ReliableUDP