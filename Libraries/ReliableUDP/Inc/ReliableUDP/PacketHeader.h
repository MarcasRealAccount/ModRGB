#pragma once

#include "Utils/Serializer.h"
#include "Utils/Version.h"

#include <random>

namespace ReliableUDP
{
	using PacketIDRNG = std::mt19937;

	static constexpr std::uint32_t  s_MagicNumber { 0x72ADAD21 };
	static constexpr Utils::Version s_Version { 1U, 0U, 0U };

	struct PacketHeader
	{
	public:
		constexpr PacketHeader() : m_MagicNumber(s_MagicNumber), m_Version(s_Version), m_ID(0U), m_Index(0U), m_Size(0U) {}
		constexpr PacketHeader(std::uint32_t id, std::uint32_t index, std::uint8_t rev, std::uint32_t size, bool ack = false) : m_MagicNumber(s_MagicNumber | ack << 31), m_ID(id), m_Index((index & 0xFFFFF) | rev << 20), m_Size(size) {}

		constexpr bool isValid() const { return (m_MagicNumber & 0x7FFFFFFF) == s_MagicNumber && m_Version.isCompatible(s_Version); }

		constexpr bool          isAck() const { return (m_MagicNumber >> 31) & 1; }
		constexpr std::uint32_t getIndex() const { return m_Index & 0xFFFFF; }
		constexpr std::uint8_t  getRev() const { return static_cast<std::uint8_t>((m_Index >> 20) & 0xFFF); }

	public:
		std::uint32_t  m_MagicNumber { s_MagicNumber };
		Utils::Version m_Version { s_Version };
		std::uint32_t  m_ID;
		std::uint32_t  m_Index;
		std::uint32_t  m_Size;

	public:
		using SerializerInfo = Utils::SerializerInfo<
		    Utils::Bases<>,
		    Utils::Variables<Utils::Variable<&Packet::m_MagicNumber>,
		                     Utils::Variable<&Packet::m_Version>,
		                     Utils::Variable<&Packet::m_ID>,
		                     Utils::Variable<&Packet::m_Index>,
		                     Utils::Variable<&Packet::m_Size>>,
		    true>;
	};
} // namespace ReliableUDP