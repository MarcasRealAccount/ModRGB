#pragma once

#include "Utils/Serializer.h"
#include "Utils/Version.h"

#include <random>

namespace ReliableUDP
{
	static constexpr std::uint32_t  s_MagicNumber { 0x72ADAD21 };
	static constexpr Utils::Version s_Version { 1U, 0U, 0U };

	static constexpr std::uint32_t s_Acknowledge = 1U;

	struct PacketHeader
	{
	public:
		constexpr PacketHeader() : m_MagicNumber(s_MagicNumber), m_Version(s_Version), m_ID(0U), m_Index(0U), m_Type(0U), m_Size(0U) {}
		constexpr PacketHeader(std::uint32_t id, std::uint32_t index, std::uint8_t rev, std::uint32_t type, std::uint32_t size) : m_MagicNumber(s_MagicNumber), m_ID(id), m_Index((index & 0xFFFFF) | (rev & 0xFFF) << 20), m_Type(type), m_Size(size) {}

		constexpr bool isValid() const { return m_MagicNumber == s_MagicNumber && m_Version.isCompatible(s_Version); }

		constexpr bool          isAck() const { return m_Type == s_Acknowledge; }
		constexpr std::uint32_t getIndex() const { return m_Index & 0xFFFFF; }
		constexpr std::uint16_t getRev() const { return static_cast<std::uint16_t>((m_Index >> 20) & 0xFFF); }

		virtual void        update() {}
		virtual bool        serialize(Utils::Buffer& buffer) const { return Utils::Serializer<PacketHeader> {}.serialize(buffer, *this); }
		virtual bool        deserialize(Utils::Buffer& buffer) { return Utils::Serializer<PacketHeader> {}.deserialize(buffer, *this); }
		virtual std::size_t size() const { return Utils::Serializer<PacketHeader> {}.size(*this); }

		void incRev() { m_Index = getIndex() | (getRev() + 1) << 20; }

	public:
		std::uint32_t  m_MagicNumber { s_MagicNumber };
		Utils::Version m_Version { s_Version };
		std::uint32_t  m_Type;
		std::uint32_t  m_ID;
		std::uint32_t  m_Index;
		std::uint32_t  m_Size;

	public:
		using SerializerInfo = Utils::SerializerInfo<
		    Utils::Bases<>,
		    Utils::Variables<Utils::Variable<&PacketHeader::m_MagicNumber>,
		                     Utils::Variable<&PacketHeader::m_Version>,
		                     Utils::Variable<&PacketHeader::m_Type>,
		                     Utils::Variable<&PacketHeader::m_ID>,
		                     Utils::Variable<&PacketHeader::m_Index>,
		                     Utils::Variable<&PacketHeader::m_Size>>,
		    true>;
	};

	using PacketTypeConstructor = std::unique_ptr<PacketHeader> (*)();
	void                  RegisterPacketType(std::uint32_t type, PacketTypeConstructor constructor);
	PacketTypeConstructor GetPacketTypeConstructor(std::uint32_t type);
} // namespace ReliableUDP