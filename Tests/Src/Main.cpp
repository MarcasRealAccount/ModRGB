#include <ReliableUDP/PacketHandler.h>
#include <ReliableUDP/Utils/Core.h>

#include <csignal>

#include <iostream>
#include <thread>

#if BUILD_IS_SYSTEM_WINDOWS
#else
#include <unistd.h>
#endif

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

void clientPacketHandler([[maybe_unused]] ReliableUDP::PacketHandler* handler, [[maybe_unused]] ReliableUDP::Networking::Endpoint endpoint, std::uint8_t* packet, std::uint32_t size)
{
	std::string_view str { reinterpret_cast<char*>(packet), size };
	std::cout << str << '\n';
}

void socketErrorHandler([[maybe_unused]] ReliableUDP::Networking::Socket* socket, [[maybe_unused]] void* userData, ReliableUDP::Networking::ESocketError error)
{
	std::cout << ReliableUDP::Networking::GetSocketErrorString(error) << "\n";
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

	socket.setNonBlocking();

	if (!socket.bind({ "localhost", "45252", ReliableUDP::Networking::EAddressType::IPv4 }))
	{
		std::cout << "Failed to open socket!\n";
		return;
	}

	std::cout << "Server opened!\n";
	serverUp = true;
	while (socket.isBound() && running)
	{
		server.updatePackets();
	}
}

static bool IsCINReady()
{
#if BUILD_IS_SYSTEM_WINDOWS
	return true;
#else
	fd_set readfds {};
	FD_SET(fileno(stdin), &readfds);

	struct timeval timeout;
	timeout.tv_sec  = 0;
	timeout.tv_usec = 0;

	return select(1, &readfds, NULL, NULL, &timeout) > 0;
#endif
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

	socket.setNonBlocking();

	if (!socket.connect({ "localhost", "45252", ReliableUDP::Networking::EAddressType::IPv4 }))
	{
		std::cout << "Failed to connect socket!\n";
		running = false;
		serverThread.join();
		return 1;
	}

	std::cout << "Client connected!\n";

	while (socket.isBound() && running)
	{
		if (IsCINReady())
		{
			std::string str;
			std::getline(std::cin, str);
			std::uint16_t id;
			std::uint8_t* packet = client.allocateWritePacket(str.size(), id);
			client.setPacketEndpoint(id, socket.getRemoteEndpoint());
			std::memcpy(packet, str.data(), str.size());
			client.markWritePacketReady(id);
		}

		client.updatePackets();
	}

	serverThread.join();
	return 0;
}