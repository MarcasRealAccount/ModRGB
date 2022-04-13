#pragma once

#include "Utils/Version.h"

namespace ReliableUDP
{
	static constexpr std::uint16_t s_MagicNumber = 0x2FDE;

	enum class EPacketHeaderType : std::uint8_t
	{
		Normal      = 0b00,
		Acknowledge = 0b01,
		Reject      = 0b10,
		MaxSize     = 0b11
	};

	struct PacketHeader
	{
	public:
		std::uint16_t m_MagicNumber { s_MagicNumber };
		std::uint16_t m_ID;
		std::uint32_t m_Index : 20;
		std::uint32_t m_Rev : 12;
		std::uint32_t m_Size;
	};

	struct AcknowledgePacketHeader
	{
	public:
		std::uint16_t m_MagicNumber { s_MagicNumber | 0b01 << 14 };
		std::uint16_t m_ID;
		std::uint32_t m_Index : 20;
		std::uint32_t m_Rev : 12;
	};

	struct RejectPacketHeader
	{
	public:
		std::uint16_t m_MagicNumber { s_MagicNumber | 0b10 << 14 };
		std::uint16_t m_ID;
		std::uint32_t m_Index : 20 { 0U };
		std::uint32_t m_Rev : 12;
	};

	struct MaxSizePacketHeader
	{
	public:
		std::uint16_t m_MagicNumber { s_MagicNumber | 0b11 << 14 };
		std::uint16_t m_ID;
		std::uint32_t m_Size;
	};

	bool              IsMagicNumberValid(std::uint16_t magicNumber);
	EPacketHeaderType GetPacketHeaderType(std::uint16_t magicNumber);
} // namespace ReliableUDP