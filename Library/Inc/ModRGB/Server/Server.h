#pragma once

#include <ModRGB/Networking/Socket.h>
#include <ModRGB/Protocol/LEDStrip.h>
#include <ModRGB/Protocol/Packet.h>

#include <map>
#include <string>

namespace ModRGB
{
	struct ClientInfo
	{
	public:
		ClientInfo(Protocol::UUID uuid, Networking::IPv4Endpoint endpoint, const std::vector<Protocol::LEDStrip>& strips, const std::vector<Protocol::LEDArea>& areas);
		ClientInfo(Protocol::UUID uuid, Networking::IPv4Endpoint endpoint, std::vector<Protocol::LEDStrip>&& strips, std::vector<Protocol::LEDArea>&& areas);

		Protocol::UUID                  m_UUID;
		std::string                     m_Name;
		std::vector<Protocol::LEDStrip> m_Strips;
		std::vector<Protocol::LEDArea>  m_Areas;
		Networking::IPv4Endpoint        m_Endpoint;
		Protocol::Clock::time_point     m_PreviousPing;
	};

	class Server : public Protocol::PacketHandler
	{
	public:
		Server(Networking::IPv4Endpoint endpoint);
		Server(Networking::IPv4Address address, std::uint16_t port) : Server(Networking::IPv4Endpoint { address, port }) {}
		Server(Server&& move);

		void updateClients();

		ClientInfo*       getClient(Protocol::UUID uuid);
		const ClientInfo* getClient(Protocol::UUID uuid) const;

		virtual bool handlePacket(Utils::Buffer& buffer, Networking::IPv4Endpoint endpoint) override;

		virtual Networking::IPv4Socket&  getSocket() override;
		virtual bool                     clientExists(Protocol::UUID uuid) const override;
		virtual Networking::IPv4Endpoint getClientEndpoint(Protocol::UUID uuid) const override;

	private:
		Protocol::UUID newClientUUID();
		bool           isClientUUIDUsed(Protocol::UUID uuid) const;

	private:
		Networking::IPv4Socket               m_Socket;
		std::map<Protocol::UUID, ClientInfo> m_Clients;
		std::vector<Protocol::UUID>          m_UsedClientUUIDs;

		Protocol::UUIDRandom m_ClientRNG;

		Protocol::Clock::time_point m_PreviousTimeout;

		double m_ClientTimeout = 2.0;
	};
} // namespace ModRGB