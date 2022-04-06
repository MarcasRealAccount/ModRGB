#pragma once

#include <ModRGB/Networking/Socket.h>
#include <ModRGB/Protocol/Packet.h>

#include <chrono>
#include <memory>
#include <random>
#include <vector>

namespace ModRGB
{
	using ClientClock = std::chrono::system_clock;

	struct ClientPacket
	{
	public:
		std::unique_ptr<Protocol::PacketHeader> m_Packet;
		ClientClock::time_point                 m_Time;
	};

	class Client
	{
	public:
		Client()                       = default;
		Client(Client&& move) noexcept = default;
		Client& operator=(Client&& move) noexcept = default;
		~Client()                                 = default;

		void update();

		void handlePacket(Protocol::PacketHeader& packet);

	private:
		Networking::IPv4Socket m_Socket;

		std::vector<ClientPacket>  m_Packets;
		std::vector<std::uint32_t> m_PreviousPacketIDs;
		std::vector<std::uint32_t> m_HandledPacketIDs;

		std::mt19937 m_PacketRNG;

		ClientClock::time_point m_PreviousTimeout;
		ClientClock::time_point m_PreviousPacketSendout;

		double m_ServerTimeout = 2.0;
		double m_PacketTimeout = 2.0;
	};
} // namespace ModRGB