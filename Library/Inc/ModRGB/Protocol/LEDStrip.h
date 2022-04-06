#pragma once

#include "Packet.h"

#include <cstdint>

#include <vector>

namespace ModRGB::Protocol
{
	struct Vec3f
	{
	public:
		float x, y, z;
	};

	enum class EStripCapability : std::uint32_t
	{
		Addressable = 1
	};

	struct LEDStrip
	{
	public:
		std::uint64_t                 m_Length;
		std::vector<Vec3f>            m_Positions;
		std::vector<EStripCapability> m_Capabilities;
	};

	enum class ELedMode
	{
		Static,
		Individual,
		Rainbow
	};

	struct Area
	{
	public:
		std::uint16_t m_StripID;
		std::uint32_t m_ID;
		std::uint64_t m_Start;
		std::uint64_t m_End;
	};

	struct LEDArea
	{
	public:
		UUID              m_UUID;
		std::vector<Area> m_Areas;
		ELedMode          m_Mode;
	};
} // namespace ModRGB::Protocol