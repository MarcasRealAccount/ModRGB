#pragma once

#include "ModRGB/Networking/Socket.h"
#include "ModRGB/Utils/Buffer.h"

#include <cstdint>

#include <chrono>
#include <map>
#include <random>
#include <vector>

namespace ModRGB::Protocol
{
	using Clock = std::chrono::system_clock;

	using UUID       = std::uint64_t;
	using UUIDRandom = std::mt19937_64;

	namespace SharedPacketTypes
	{
		static constexpr std::uint32_t s_Ack             = 0x0001;
		static constexpr std::uint32_t s_Continuation    = 0x0002;
		static constexpr std::uint32_t s_AckContinuation = 0x0003;
	} // namespace SharedPacketTypes

	static constexpr std::uint64_t s_MagicNumber = 0x45'EA'2F'F6'3F'CB'BF'29ULL;

	struct PacketHeader
	{
	public:
		static PacketHeader ReadFrom(Utils::Buffer& buffer);

	public:
		PacketHeader() : m_PacketId(0U), m_Type(0U), m_UUID(0U), m_Size(sizeof(PacketHeader)) {}
		PacketHeader(std::uint32_t packetId, std::uint32_t type, UUID uuid) : m_PacketId(packetId), m_Type(type), m_UUID(uuid), m_Size(sizeof(PacketHeader)) {}
		PacketHeader(std::uint32_t type, UUID uuid) : m_PacketId(0U), m_Type(type), m_UUID(uuid), m_Size(sizeof(PacketHeader)) {}

		virtual void writeTo(Utils::Buffer& buffer);

	public:
		std::uint64_t m_Magic = s_MagicNumber;
		std::uint32_t m_PacketId;
		std::uint32_t m_Type;
		UUID          m_UUID;
		std::uint64_t m_Size;
	};

	template <class T>
	concept PacketData = std::is_base_of_v<PacketHeader, T>;

	struct ContinuationPacket : public PacketHeader
	{
	public:
		ContinuationPacket(std::uint32_t packetId, UUID uuid, std::uint64_t size, std::uint32_t index) : PacketHeader(packetId, SharedPacketTypes::s_Continuation, uuid), m_Index(index), m_Data { 0U } { m_Size = size; }
		ContinuationPacket(UUID uuid, std::uint64_t size, std::uint32_t index) : PacketHeader(SharedPacketTypes::s_Continuation, uuid), m_Index(index), m_Data { 0U } { m_Size = size; }

		virtual void writeTo(Utils::Buffer& buffer) override;

	public:
		std::uint32_t m_Index;
		std::uint8_t  m_Data[65503 - sizeof(PacketHeader)];
	};

	struct AckContinuationPacket : public PacketHeader
	{
	public:
		AckContinuationPacket(std::uint32_t packetId, UUID uuid, std::uint32_t index) : PacketHeader(packetId, SharedPacketTypes::s_Continuation, uuid), m_Index(index) {}
		AckContinuationPacket(UUID uuid, std::uint32_t index) : PacketHeader(SharedPacketTypes::s_Continuation, uuid), m_Index(index) {}

		virtual void writeTo(Utils::Buffer& buffer) override;

	public:
		std::uint32_t m_Index;
	};

	struct Packet
	{
	public:
		Packet(std::unique_ptr<PacketHeader>&& data, Networking::IPv4Endpoint endpoint);
		Packet(std::unique_ptr<PacketHeader>&& data, Networking::IPv4Address address, std::uint16_t port) : Packet(std::move(data), { address, port }) {}
		Packet(std::unique_ptr<PacketHeader>&& data, UUID uuid);
		Packet(Packet&& move) noexcept;

	public:
		std::unique_ptr<PacketHeader> m_Data;

		bool m_ClientBased;
		union
		{
			Networking::IPv4Endpoint m_Endpoint;
			UUID                     m_UUID;
		};

		Clock::time_point m_Time;
	};

	struct ContinuedPacket
	{
	public:
		ContinuedPacket(std::unique_ptr<PacketHeader>&& data, Networking::IPv4Endpoint endpoint);
		ContinuedPacket(std::unique_ptr<PacketHeader>&& data, Networking::IPv4Address address, std::uint16_t port) : ContinuedPacket(std::move(data), { address, port }) {}
		ContinuedPacket(std::unique_ptr<PacketHeader>&& data, UUID uuid);
		ContinuedPacket(ContinuedPacket&& move) noexcept;

	public:
		std::vector<std::unique_ptr<PacketHeader>> m_Packets;

		bool m_ClientBased;
		union
		{
			Networking::IPv4Endpoint m_Endpoint;
			UUID                     m_UUID;
		};

		Clock::time_point m_Time;
	};

	struct ContinuedPacketArrive
	{
	public:
		ContinuedPacketArrive(std::size_t size, Networking::IPv4Endpoint endpoint) : m_Data(size, 0U), m_ArrivedPackets(size, false), m_PacketsArrived(0U), m_PacketsToArrive(1 + (size - 65507) / sizeof(ContinuationPacket::m_Data)), m_Endpoint(endpoint) {}
		ContinuedPacketArrive(ContinuedPacketArrive&& move);

		void writeContinuation(std::uint32_t index, std::uint8_t* buffer, std::size_t size);

	public:
		std::vector<std::uint8_t> m_Data;
		std::vector<bool>         m_ArrivedPackets;
		std::uint32_t             m_PacketsArrived;
		std::uint32_t             m_PacketsToArrive;
		Networking::IPv4Endpoint  m_Endpoint;
	};

	class PacketHandler
	{
	public:
		PacketHandler();
		PacketHandler(PacketHandler&& move);
		~PacketHandler();

		void updatePackets();

		void sendPacket(Networking::IPv4Endpoint endpoint, std::unique_ptr<PacketHeader>&& packet);
		void sendPacket(Networking::IPv4Address address, std::uint16_t port, std::unique_ptr<PacketHeader>&& packet) { sendPacket({ address, port }, std::move(packet)); }
		void sendPacket(UUID uuid, std::unique_ptr<PacketHeader>&& packet);
		template <PacketData PacketData>
		void sendPacket(Networking::IPv4Endpoint endpoint, PacketData&& packet)
		{
			sendPacket(endpoint, std::make_unique<PacketData>(std::move(packet)));
		}
		template <PacketData PacketData>
		void sendPacket(Networking::IPv4Address address, std::uint16_t port, PacketData&& packet)
		{
			sendPacket<PacketData>({ address, port }, std::move(packet));
		}
		template <PacketData PacketData>
		void sendPacket(UUID uuid, PacketData&& packet)
		{
			sendPacket(uuid, std::make_unique<PacketData>(std::move(packet)));
		}

		void setPacketTimeout(double packetTimeout);
		auto getPacketTimeout() const { return m_PacketTimeout; }

		virtual bool handlePacket(Utils::Buffer& buffer, Networking::IPv4Endpoint endpoint) = 0;

		virtual Networking::IPv4Socket&  getSocket()                        = 0;
		virtual bool                     clientExists(UUID uuid) const      = 0;
		virtual Networking::IPv4Endpoint getClientEndpoint(UUID uuid) const = 0;

	private:
		void handle(Utils::Buffer& buffer, Networking::IPv4Endpoint endpoint);
		void send(Packet& packet);
		void send(ContinuedPacket& packet);

		std::uint32_t newPacketId();
		bool          isPacketIdUsed(std::uint32_t id) const;
		bool          isPacketHandled(std::uint32_t id) const;

	private:
		std::uint8_t* m_Buffer;

		std::vector<Packet>        m_Packets;
		std::vector<std::uint32_t> m_UsedPacketIDs;
		std::vector<std::uint32_t> m_HandledPacketIDs;
		std::size_t                m_UsedPacketIDsIndex    = 0;
		std::size_t                m_HandledPacketIDsIndex = 0;

		std::map<std::uint32_t, ContinuedPacket>       m_ContinuedPackets;
		std::map<std::uint32_t, ContinuedPacketArrive> m_ContinuedPacketArrives;

		std::mt19937 m_PacketRNG;

		Clock::time_point m_PreviousPacketSendout;

		double m_PacketTimeout = 2.0;
	};
} // namespace ModRGB::Protocol