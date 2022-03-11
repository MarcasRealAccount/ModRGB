#pragma once

#include <ModRGB/Networking/Socket.h>
#include <ModRGB/Protocol/Packet.h>

#include <chrono>
#include <memory>
#include <random>
#include <vector>

namespace ModRGB
{
	using ServerClock = std::chrono::system_clock;

	struct ServerClient
	{
	public:
		ServerClient(std::uint64_t uuid, Networking::IPv4Address address, std::uint16_t port);

	public:
		std::uint64_t m_UUID;

		Networking::IPv4Address m_Address;
		std::uint16_t           m_Port;

		ServerClock::time_point m_PreviousPing;
	};

	struct ServerPacket
	{
	public:
		ServerPacket(std::unique_ptr<Protocol::PacketHeader>&& packet, Networking::IPv4Address address, std::uint16_t port);
		ServerPacket(std::unique_ptr<Protocol::PacketHeader>&& packet, std::uint64_t uuid);
		ServerPacket(ServerPacket&& move) noexcept;

	public:
		std::unique_ptr<Protocol::PacketHeader> m_Packet;

		bool m_ClientBased;
		union
		{
			struct
			{
				Networking::IPv4Address m_Address;
				std::uint16_t           m_Port;
			};
			std::uint64_t m_UUID;
		};

		ServerClock::time_point m_Time;
	};

	class Server
	{
	public:
		Server(Networking::IPv4Address address, std::uint16_t port);
		Server(Server&& move) noexcept;
		~Server();

		void update();

		void handlePacket(Protocol::PacketHeader& packet, Networking::IPv4Address address, std::uint16_t port);

		void clientPing(std::uint64_t uuid, Networking::IPv4Address address, std::uint16_t port);
		void clientDisconnect(std::uint64_t uuid);

		void sendData(const void* buf, std::size_t size, ServerPacket& packet);
		void sendPacket(ServerPacket& packet);

		ServerClient* getClient(std::uint64_t uuid);

		void removePacket(std::uint32_t packetId);

		std::uint32_t newPacketId();
		std::uint64_t newUUID();

	private:
		Networking::IPv4Socket     m_Socket;
		std::vector<ServerClient>  m_Clients;
		std::vector<std::uint64_t> m_UsedClientUUIDs;

		std::vector<ServerPacket>  m_Packets;
		std::vector<std::uint32_t> m_PreviousPacketIDs;
		std::vector<std::uint32_t> m_HandledPacketIDs;

		std::mt19937    m_PacketRNG;
		std::mt19937_64 m_ClientRNG;

		ServerClock::time_point m_PreviousTimeout;
		ServerClock::time_point m_PreviousPacketSendout;

		double m_ClientTimeout = 2.0;
		double m_PacketTimeout = 2.0;
	};
} // namespace ModRGB