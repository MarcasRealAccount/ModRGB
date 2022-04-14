#include <ReliableUDP/PacketHandler.h>

#include <csignal>

#include <iostream>
#include <thread>

void serverPacketHandler(ReliableUDP::PacketHandler* handler, ReliableUDP::Networking::Endpoint endpoint, std::uint8_t* packet, std::uint32_t size)
{
	std::uint16_t id       = 0U;
	std::uint8_t* response = handler->allocateWritePacket(size, id);
	if (!response)
		return;
	std::memcpy(response, packet, size);
	handler->setPacketEndpoint(id, endpoint);
	handler->markWritePacketReady(id);
}

void clientPacketHandler(ReliableUDP::PacketHandler* handler, ReliableUDP::Networking::Endpoint endpoint, std::uint8_t* packet, std::uint32_t size)
{
	std::string_view str { reinterpret_cast<char*>(packet), size };
	std::cout << str << '\n';
}

void socketErrorHandler(ReliableUDP::Networking::Socket* socket, std::uint32_t errorCode)
{
	std::cout << "Socket Error " << errorCode << "\n";
}

ReliableUDP::PacketHandler server { 45056, 45056, 64, 64, 8, &serverPacketHandler, nullptr };
bool                       running  = true;
bool                       serverUp = false;

extern "C" void signalHandler(int signal)
{
	if (signal == SIGINT)
		running = false;
}

void serverFunc()
{
	auto& socket = server.getSocket();

	socket.setErrorCallback(&socketErrorHandler, nullptr);

	socket.setLocalEndpoint({ "localhost", "45252", ReliableUDP::Networking::EAddressType::IPv4 });

	if (!socket.open())
	{
		std::cout << "Failed to open socket!\n";
		return;
	}

	std::cout << "Server opened!\n";
	serverUp = true;
	while (running)
	{
		server.updatePackets();
	}
}

int main([[maybe_unused]] int argc, [[maybe_unused]] char** argv)
{
	std::signal(SIGINT, &signalHandler);

	std::thread serverThread { &serverFunc };

	while (!serverUp)
		;

	ReliableUDP::PacketHandler client { 45056, 45056, 64, 64, 8, &clientPacketHandler, nullptr };

	auto& socket = client.getSocket();

	socket.setErrorCallback(&socketErrorHandler, nullptr);

	if (!socket.connect({ "localhost", "45252", ReliableUDP::Networking::EAddressType::IPv4 }))
	{
		std::cout << "Failed to connect socket!\n";
		running = false;
		serverThread.join();
		return 1;
	}

	std::cout << "Client connected!\n";

	while (running)
	{
		std::string str;
		std::getline(std::cin, str);
		std::uint16_t id;
		std::uint8_t* packet = client.allocateWritePacket(str.size(), id);
		client.setPacketEndpoint(id, socket.getRemoteEndpoint());
		std::memcpy(packet, str.data(), str.size());
		client.markWritePacketReady(id);

		client.updatePackets();
	}

	serverThread.join();
	return 0;
}