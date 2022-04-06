#pragma once

#include "LEDStrip.h"
#include "Packet.h"

namespace ModRGB::Protocol
{
	namespace ClientServerPacketTypes
	{
		static constexpr std::uint32_t s_Connect      = 0x0100;
		static constexpr std::uint32_t s_Ping         = 0x0101;
		static constexpr std::uint32_t s_Disconnect   = 0x0102;
		static constexpr std::uint32_t s_UpdateStrips = 0x0103;
	} // namespace ClientServerPacketTypes

	namespace ServerClientPacketTypes
	{
		static constexpr std::uint32_t s_NewID       = 0x0100;
		static constexpr std::uint32_t s_AddArea     = 0x0101;
		static constexpr std::uint32_t s_UpdateAreas = 0x0102;
		static constexpr std::uint32_t s_RemoveArea  = 0x0103;
	} // namespace ServerClientPacketTypes

	namespace ControllerServerPacketTypes
	{
		static constexpr std::uint32_t s_GetClients = 0x01000100;
		static constexpr std::uint32_t s_GetClient  = 0x01000101;

		static constexpr std::uint32_t s_GetStrips = 0x01000110;
		static constexpr std::uint32_t s_GetStrip  = 0x01000111;

		static constexpr std::uint32_t s_GetAreas = 0x01000120;
		static constexpr std::uint32_t s_GetArea  = 0x01000121;

		static constexpr std::uint32_t s_ResetClient   = 0x01001000;
		static constexpr std::uint32_t s_SetClientName = 0x01001001;

		static constexpr std::uint32_t s_SetStripName = 0x01001010;

		static constexpr std::uint32_t s_AddArea     = 0x01001022;
		static constexpr std::uint32_t s_UpdateAreas = 0x01001023;
		static constexpr std::uint32_t s_RemoveArea  = 0x01001024;
	} // namespace ControllerServerPacketTypes

	namespace ServerControllerPacketTypes
	{
		static constexpr std::uint32_t s_GetClientsResponse = 0x0100;
		static constexpr std::uint32_t s_GetStripsResponse  = 0x0101;
		static constexpr std::uint32_t s_GetAreasResponse   = 0x0102;
	} // namespace ServerControllerPacketTypes

	namespace Packets
	{
		struct StripPacket : public PacketHeader
		{
		public:
			static StripPacket ReadFrom(Utils::Buffer& buffer);

		public:
			StripPacket(std::uint32_t packetId, UUID uuid, const std::vector<LEDStrip>& strips) : PacketHeader(packetId, ClientServerPacketTypes::s_Connect, uuid), m_Strips(strips) {}
			StripPacket(std::uint32_t packetId, UUID uuid, std::vector<LEDStrip>&& strips) : PacketHeader(packetId, ClientServerPacketTypes::s_Connect, uuid), m_Strips(std::move(strips)) {}
			StripPacket(UUID uuid, const std::vector<LEDStrip>& strips) : PacketHeader(ClientServerPacketTypes::s_Connect, uuid), m_Strips(strips) {}
			StripPacket(UUID uuid, std::vector<LEDStrip>&& strips) : PacketHeader(ClientServerPacketTypes::s_Connect, uuid), m_Strips(std::move(strips)) {}

			StripPacket(std::uint32_t packetId, std::uint32_t type, UUID uuid, const std::vector<LEDStrip>& strips) : PacketHeader(packetId, type, uuid), m_Strips(strips) {}
			StripPacket(std::uint32_t packetId, std::uint32_t type, UUID uuid, std::vector<LEDStrip>&& strips) : PacketHeader(packetId, type, uuid), m_Strips(std::move(strips)) {}
			StripPacket(std::uint32_t type, UUID uuid, const std::vector<LEDStrip>& strips) : PacketHeader(type, uuid), m_Strips(strips) {}
			StripPacket(std::uint32_t type, UUID uuid, std::vector<LEDStrip>&& strips) : PacketHeader(type, uuid), m_Strips(std::move(strips)) {}

			virtual void writeTo(Utils::Buffer& buffer) override;

		private:
			StripPacket(const PacketHeader& header, std::vector<LEDStrip>&& strips) : PacketHeader(header), m_Strips(std::move(strips)) {}

		public:
			std::vector<LEDStrip> m_Strips;
		};

		struct Client
		{
		public:
			Protocol::UUID m_UUID;
			std::string    m_Name;
			std::uint16_t  m_StripCount;
			std::uint32_t  m_AreaCount;
		};

		struct ClientsPacket : public PacketHeader
		{
		public:
			static ClientsPacket ReadFrom(Utils::Buffer& buffer);

		public:
			ClientsPacket(std::uint32_t packetId, UUID uuid, const std::vector<Client>& clients) : PacketHeader(packetId, ServerControllerPacketTypes::s_GetAreasResponse, uuid), m_Clients(clients) {}
			ClientsPacket(std::uint32_t packetId, UUID uuid, std::vector<Client>&& clients) : PacketHeader(packetId, ServerControllerPacketTypes::s_GetAreasResponse, uuid), m_Clients(std::move(clients)) {}
			ClientsPacket(UUID uuid, const std::vector<Client>& clients) : PacketHeader(ServerControllerPacketTypes::s_GetAreasResponse, uuid), m_Clients(clients) {}
			ClientsPacket(UUID uuid, std::vector<Client>&& clients) : PacketHeader(ServerControllerPacketTypes::s_GetAreasResponse, uuid), m_Clients(std::move(clients)) {}

			virtual void writeTo(Utils::Buffer& buffer) override;

		private:
			ClientsPacket(const PacketHeader& header, std::vector<Client>&& clients) : PacketHeader(header), m_Clients(std::move(clients)) {}

		public:
			std::vector<Client> m_Clients;
		};
	} // namespace Packets
} // namespace ModRGB::Protocol