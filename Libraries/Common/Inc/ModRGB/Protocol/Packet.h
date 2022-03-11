#pragma once

#include <cstdint>

namespace ModRGB::Protocol
{
	enum class EPacketType : std::uint32_t
	{
		Ack = 0,
		Ping,
		Disconnect,
		NewID
	};

	static constexpr std::uint64_t s_MagicNumber = 0x45'EA'2F'F6'3F'CB'BF'29ULL;

	struct PacketHeader
	{
	public:
		PacketHeader(std::uint32_t packetId, EPacketType type, std::uint64_t uuid) : m_PacketId(packetId), m_Type(type), m_UUID(uuid) {}

	public:
		std::uint64_t m_Magic = s_MagicNumber;
		std::uint32_t m_PacketId;
		EPacketType   m_Type;
		std::uint64_t m_UUID;
	};
} // namespace ModRGB::Protocol