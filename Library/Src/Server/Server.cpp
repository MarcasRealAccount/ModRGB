#include "ModRGB/Server/Server.h"
#include "ModRGB/Protocol/Packets.h"

#include <utility>

namespace ModRGB
{
	ClientInfo::ClientInfo(Protocol::UUID uuid, Networking::IPv4Endpoint endpoint, const std::vector<Protocol::LEDStrip>& strips, const std::vector<Protocol::LEDArea>& areas)
	    : m_UUID(uuid), m_Endpoint(endpoint), m_Strips(strips), m_Areas(areas), m_PreviousPing(Protocol::Clock::now()) {}

	ClientInfo::ClientInfo(Protocol::UUID uuid, Networking::IPv4Endpoint endpoint, std::vector<Protocol::LEDStrip>&& strips, std::vector<Protocol::LEDArea>&& areas)
	    : m_UUID(uuid), m_Endpoint(endpoint), m_Strips(std::move(strips)), m_Areas(std::move(areas)), m_PreviousPing(Protocol::Clock::now()) {}

	Server::Server(Networking::IPv4Endpoint endpoint)
	    : m_Socket(Networking::ESocketType::UDP), m_ClientRNG(std::random_device {}()), m_PreviousTimeout(Protocol::Clock::now())
	{
		m_Socket.setLocalEndpoint(endpoint);
		m_Socket.open();
	}

	Server::Server(Server&& move)
	    : m_Socket(std::move(move.m_Socket)), m_Clients(std::move(move.m_Clients)), m_UsedClientUUIDs(std::move(move.m_UsedClientUUIDs)), m_ClientRNG(std::move(move.m_ClientRNG)), m_PreviousTimeout(std::move(move.m_PreviousTimeout)), m_ClientTimeout(move.m_ClientTimeout) {}

	void Server::updateClients()
	{
		auto   timeNow   = Protocol::Clock::now();
		double deltaTime = std::chrono::duration_cast<std::chrono::duration<double, std::milli>>(timeNow - m_PreviousTimeout).count();
		if (deltaTime >= 125.0)
		{
			std::erase_if(m_Clients,
			              [timeNow, this](ClientInfo& client) -> bool
			              {
				              return std::chrono::duration_cast<std::chrono::duration<double>>(timeNow - client.m_PreviousPing).count() >= m_ClientTimeout;
			              });

			m_PreviousTimeout = timeNow;
		}

		PacketHandler::updatePackets();
	}

	ClientInfo* Server::getClient(Protocol::UUID uuid)
	{
		auto itr = m_Clients.find(uuid);
		return itr != m_Clients.end() ? &itr->second : nullptr;
	}

	const ClientInfo* Server::getClient(Protocol::UUID uuid) const
	{
		auto itr = m_Clients.find(uuid);
		return itr != m_Clients.end() ? &itr->second : nullptr;
	}

	bool Server::handlePacket(Utils::Buffer& buffer, Networking::IPv4Endpoint endpoint)
	{
		Protocol::PacketHeader& header = *reinterpret_cast<Protocol::PacketHeader*>(buffer.getBuffer());
		switch (header.m_Type)
		{
			// Client -> Server
		case Protocol::ClientServerPacketTypes::s_Connect:
		{
			if (header.m_UUID == 0)
			{
				auto newUUID  = newClientUUID();
				header.m_UUID = newUUID;
				sendPacket(newUUID, std::make_unique<Protocol::PacketHeader>(Protocol::ServerClientPacketTypes::s_NewID, newUUID));
			}

			auto stripPacket = Protocol::Packets::StripPacket::ReadFrom(buffer);
			m_Clients.insert({ stripPacket.m_UUID, ClientInfo { stripPacket.m_UUID, endpoint, std::move(stripPacket.m_Strips), {} } });
			return true;
		}
		case Protocol::ClientServerPacketTypes::s_Ping:
		{
			auto client = getClient(header.m_UUID);
			if (client)
				client->m_PreviousPing = Protocol::Clock::now();
			return true;
		}
		case Protocol::ClientServerPacketTypes::s_Disconnect:
		{
			std::remove(m_Clients.begin(), m_Clients.end(),
			            [header](ClientInfo& client) -> bool
			            {
				            return client.m_UUID == header.m_UUID;
			            });
			return false;
		}
		case Protocol::ClientServerPacketTypes::s_UpdateStrips:
		{
			auto client = getClient(header.m_UUID);
			if (client)
			{
				auto stripPacket = Protocol::Packets::StripPacket::ReadFrom(buffer);
				client->m_Strips = std::move(stripPacket.m_Strips);
			}
			return true;
		}
			// Controller -> Server
		case Protocol::ControllerServerPacketTypes::s_GetClients:
		{
			std::vector<Protocol::Packets::Client> clients;
			clients.reserve(m_Clients.size());
			for (auto& client : m_Clients)
				clients.emplace_back(client.second.m_UUID, client.second.m_Name, static_cast<std::uint16_t>(client.second.m_Strips.size()), static_cast<std::uint32_t>(client.second.m_Areas.size()));

			sendPacket(endpoint, std::make_unique<Protocol::Packets::ClientsPacket>(0U, std::move(clients)));
			return true;
		}
		case Protocol::ControllerServerPacketTypes::s_GetClient:
		{
			std::vector<Protocol::Packets::Client> clients;

			auto itr = m_Clients.find(header.m_UUID);
			if (itr != m_Clients.end())
			{
				auto& client = itr->second;
				clients.emplace_back(client.m_UUID, client.m_Name, static_cast<std::uint16_t>(client.m_Strips.size()), static_cast<std::uint32_t>(client.m_Areas.size()));
			}
			sendPacket(endpoint, std::make_unique<Protocol::Packets::ClientsPacket>(0U, std::move(clients)));
			return true;
		}
		case Protocol::ControllerServerPacketTypes::s_GetStrips:
		{
			std::vector<Protocol::LEDStrip> strips;

			auto itr = m_Clients.find(header.m_UUID);
			if (itr != m_Clients.end())
			{
				auto& client = itr->second;
				strips       = client.m_Strips;
				strips.resize(client.m_Strips.size());
			}
			sendPacket(endpoint, std::make_unique<Protocol::Packets::StripPacket>(Protocol::ServerControllerPacketTypes::s_GetStripsResponse, 0U, std::move(strips)));
			return true;
		}
		case Protocol::ControllerServerPacketTypes::s_GetStrip:
		{
			std::vector<Protocol::LEDStrip> strips;

			auto itr = m_Clients.find(header.m_UUID);
			if (itr != m_Clients.end())
			{
				auto& client = itr->second;
				strips       = client.m_Strips;
				strips.resize(client.m_Strips.size());
			}
			sendPacket(endpoint, std::make_unique<Protocol::Packets::StripPacket>(Protocol::ServerControllerPacketTypes::s_GetStripsResponse, 0U, std::move(strips)));
			return true;
		}
		}
	}

	Networking::IPv4Socket& Server::getSocket()
	{
		return m_Socket;
	}

	bool Server::clientExists(Protocol::UUID uuid) const
	{
		return isClientUUIDUsed(uuid);
	}

	Networking::IPv4Endpoint Server::getClientEndpoint(Protocol::UUID uuid) const
	{
		auto client = getClient(uuid);
		return client ? client->m_Endpoint : Networking::IPv4Endpoint {};
	}

	Protocol::UUID Server::newClientUUID()
	{
		auto uuid = m_ClientRNG();
		while (isClientUUIDUsed(uuid))
			uuid = m_ClientRNG();

		m_UsedClientUUIDs.push_back(uuid);
		return uuid;
	}

	bool Server::isClientUUIDUsed(Protocol::UUID uuid) const
	{
		return std::find(m_UsedClientUUIDs.begin(), m_UsedClientUUIDs.end(), uuid) != m_UsedClientUUIDs.end();
	}
} // namespace ModRGB