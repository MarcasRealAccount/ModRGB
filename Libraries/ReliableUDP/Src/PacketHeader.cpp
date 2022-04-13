#include "ReliableUDP/PacketHeader.h"

#include <map>

namespace ReliableUDP
{
	static std::map<std::uint32_t, PacketTypeConstructor> s_PacketTypeConstructors;

	static struct ReliableUDPTypeAutoRegister
	{
		ReliableUDPTypeAutoRegister()
		{
			auto packetHeaderConstructor = []()
			{
				return std::make_unique<PacketHeader>();
			};
			RegisterPacketType(0U, packetHeaderConstructor);
			RegisterPacketType(s_Acknowledge, packetHeaderConstructor);
		}
	} s_TypeAutoRegister;

	void RegisterPacketType(std::uint32_t type, PacketTypeConstructor constructor)
	{
		s_PacketTypeConstructors.insert({ type, constructor });
	}

	PacketTypeConstructor GetPacketTypeConstructor(std::uint32_t type)
	{
		auto itr = s_PacketTypeConstructors.find(type);
		return itr != s_PacketTypeConstructors.end() ? itr->second : nullptr;
	}
} // namespace ReliableUDP