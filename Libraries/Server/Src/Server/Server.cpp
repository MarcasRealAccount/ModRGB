#include "ModRGB/Server/Server.h"

#include <utility>

namespace ModRGB
{
	ServerClient::ServerClient(std::uint64_t uuid, Networking::IPv4Address address, std::uint16_t port)
	    : m_UUID(uuid), m_Address(address), m_Port(port), m_PreviousPing(ServerClock::now()) {}

	ServerPacket::ServerPacket(std::unique_ptr<Protocol::PacketHeader>&& packet, Networking::IPv4Address address, std::uint16_t port)
	    : m_Packet(std::move(packet)), m_ClientBased(false), m_Address(address), m_Port(port), m_Time(ServerClock::now()) {}

	ServerPacket::ServerPacket(std::unique_ptr<Protocol::PacketHeader>&& packet, std::uint64_t uuid)
	    : m_Packet(std::move(packet)), m_ClientBased(true), m_UUID(uuid), m_Time(ServerClock::now()) {}

	ServerPacket::ServerPacket(ServerPacket&& move) noexcept
	    : m_Packet(std::move(move.m_Packet)), m_ClientBased(move.m_ClientBased), m_Time(move.m_Time)
	{
		if (m_ClientBased)
		{
			m_UUID = move.m_UUID;
		}
		else
		{
			m_Address = move.m_Address;
			m_Port    = move.m_Port;
		}
	}

	Server::Server(Networking::IPv4Address address, std::uint16_t port)
	    : m_Socket(Networking::ESocketType::UDP), m_PacketRNG(std::random_device()()), m_ClientRNG(std::random_device()()), m_PreviousTimeout(ServerClock::now()), m_PreviousPacketSendout(ServerClock::now())
	{
		m_Socket.setLocalAddress(address, port);
		m_Socket.open();
	}

	Server::Server(Server&& move) noexcept
	    : m_Socket(std::move(move.m_Socket)), m_Clients(std::move(move.m_Clients)), m_UsedClientUUIDs(std::move(move.m_UsedClientUUIDs)), m_Packets(std::move(move.m_Packets)), m_PreviousPacketIDs(std::move(move.m_PreviousPacketIDs)), m_PacketRNG(std::move(move.m_PacketRNG)), m_ClientRNG(std::move(move.m_ClientRNG)), m_PreviousTimeout(std::move(move.m_PreviousTimeout)), m_PreviousPacketSendout(std::move(move.m_PreviousPacketSendout)), m_ClientTimeout(move.m_ClientTimeout), m_PacketTimeout(move.m_PacketTimeout) {}

	Server::~Server()
	{
	}

	void Server::update()
	{
		constexpr std::size_t s_BufSize = 65536;

		auto currentTime = ServerClock::now();
		if (std::chrono::duration_cast<std::chrono::milliseconds>(currentTime - m_PreviousTimeout).count() >= 128)
		{
			std::erase_if(m_Clients, [currentTime, this](ServerClient& client) -> bool
			              { return std::chrono::duration_cast<std::chrono::duration<double>>(currentTime - client.m_PreviousPing).count() >= m_ClientTimeout; });
			m_PreviousTimeout = currentTime;
		}

		std::uint8_t* buf = new std::uint8_t[s_BufSize];

		Networking::IPv4Address address { 0, 0, 0, 0 };
		std::uint16_t           port;

		std::size_t readCount = 0;
		while ((readCount = m_Socket.readFrom(buf, s_BufSize - 1, address, port)) > 0)
		{
			if (readCount < sizeof(Protocol::PacketHeader))
				continue;

			Protocol::PacketHeader& header = *reinterpret_cast<Protocol::PacketHeader*>(buf);
			if (header.m_Magic != Protocol::s_MagicNumber)
				continue;

			if (std::find(m_HandledPacketIDs.begin(), m_HandledPacketIDs.end(), header.m_PacketId) != m_HandledPacketIDs.end())
				continue;

			if (header.m_UUID == 0ULL && header.m_Type != Protocol::EPacketType::Disconnect)
			{
				header.m_UUID = newUUID();
				m_Packets.push_back({ std::make_unique<Protocol::PacketHeader>(header.m_PacketId, Protocol::EPacketType::NewID, header.m_UUID), address, port });
			}

			handlePacket(header, address, port);

			if (m_HandledPacketIDs.size() == 128)
				m_HandledPacketIDs.erase(m_HandledPacketIDs.begin());
			m_HandledPacketIDs.push_back(header.m_PacketId);
		}

		if (std::chrono::duration_cast<std::chrono::milliseconds>(currentTime - m_PreviousPacketSendout).count() >= 256)
		{
			std::erase_if(m_Packets, [currentTime, this](ServerPacket& packet)
			              { return (packet.m_ClientBased && getClient(packet.m_UUID) == nullptr) || std::chrono::duration_cast<std::chrono::duration<double>>(currentTime - packet.m_Time).count() >= m_PacketTimeout; });

			for (auto& packet : m_Packets)
				sendPacket(packet);

			m_PreviousPacketSendout = currentTime;
		}
	}

	void Server::handlePacket(Protocol::PacketHeader& packet, Networking::IPv4Address address, std::uint16_t port)
	{
		switch (packet.m_Type)
		{
		case Protocol::EPacketType::Ack:
			removePacket(packet.m_PacketId);
			break;
		case Protocol::EPacketType::Ping:
			clientPing(packet.m_UUID, address, port);
			break;
		case Protocol::EPacketType::Disconnect:
			clientDisconnect(packet.m_UUID);
			break;
		default:
			break;
		}

		if (packet.m_Type != Protocol::EPacketType::Disconnect)
			m_Packets.push_back({ std::make_unique<Protocol::PacketHeader>(packet.m_PacketId, Protocol::EPacketType::Ack, packet.m_UUID), packet.m_UUID });
	}

	void Server::clientPing(std::uint64_t uuid, Networking::IPv4Address address, std::uint16_t port)
	{
		auto itr = std::find_if(m_Clients.begin(), m_Clients.end(), [uuid](ServerClient& client)
		                        { return client.m_UUID == uuid; });
		if (itr != m_Clients.end())
		{
			itr->m_Address      = address;
			itr->m_Port         = port;
			itr->m_PreviousPing = ServerClock::now();
		}
		else
		{
			m_Clients.push_back(ServerClient { uuid, address, port });
		}
	}

	void Server::clientDisconnect(std::uint64_t uuid)
	{
		auto itr = std::find_if(m_Clients.begin(), m_Clients.end(), [uuid](ServerClient& client)
		                        { return client.m_UUID == uuid; });
		if (itr != m_Clients.end())
			m_Clients.erase(itr);
	}

	void Server::sendData(const void* buf, std::size_t size, ServerPacket& packet)
	{
		if (packet.m_ClientBased)
		{
			auto client = getClient(packet.m_UUID);
			m_Socket.writeTo(buf, size, client->m_Address, client->m_Port);
		}
		else
		{
			m_Socket.writeTo(buf, size, packet.m_Address, packet.m_Port);
		}
	}

	void Server::sendPacket(ServerPacket& packet)
	{
		switch (packet.m_Packet->m_Type)
		{
		case Protocol::EPacketType::Ack: [[fallthrough]];
		case Protocol::EPacketType::Ping: [[fallthrough]];
		case Protocol::EPacketType::Disconnect: [[fallthrough]];
		case Protocol::EPacketType::NewID:
			sendData(packet.m_Packet.get(), sizeof(Protocol::PacketHeader), packet);
			break;
		}
	}

	ServerClient* Server::getClient(std::uint64_t uuid)
	{
		auto itr = std::find_if(m_Clients.begin(), m_Clients.end(), [uuid](ServerClient& client)
		                        { return client.m_UUID == uuid; });
		return itr != m_Clients.end() ? &*itr : nullptr;
	}

	void Server::removePacket(std::uint32_t packetId)
	{
		auto itr = std::find_if(m_Packets.begin(), m_Packets.end(), [packetId](ServerPacket& packet)
		                        { return packet.m_Packet->m_PacketId == packetId; });
		if (itr != m_Packets.end())
			m_Packets.erase(itr);
	}

	std::uint32_t Server::newPacketId()
	{
		std::uint32_t id = m_PacketRNG();
		while (std::find(m_PreviousPacketIDs.begin(), m_PreviousPacketIDs.end(), id) != m_PreviousPacketIDs.end())
			id = m_PacketRNG();
		if (m_PreviousPacketIDs.size() == 128)
			m_PreviousPacketIDs.erase(m_PreviousPacketIDs.begin());
		m_PreviousPacketIDs.push_back(id);
		return id;
	}

	std::uint64_t Server::newUUID()
	{
		std::uint64_t id = m_ClientRNG();
		while (std::find(m_UsedClientUUIDs.begin(), m_UsedClientUUIDs.end(), id) != m_UsedClientUUIDs.end())
			id = m_ClientRNG();
		m_UsedClientUUIDs.push_back(id);
		return id;
	}
} // namespace ModRGB