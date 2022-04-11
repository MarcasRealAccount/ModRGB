#include <ReliableUDP/Networking/Socket.h>

#include <csignal>

#include <iostream>
#include <thread>

ReliableUDP::Networking::Socket socket { ReliableUDP::Networking::ESocketType::TCP };

extern "C" void signalHandler(int signal)
{
	if (signal == SIGINT)
	{
		socket.close();
		std::cout << "Hosting closed\n";
		std::exit(0);
	}
}

int main([[maybe_unused]] int argc, [[maybe_unused]] char** argv)
{
	std::signal(SIGINT, &signalHandler);

	using namespace ReliableUDP::Networking;
	socket.setErrorCallback(
	    []([[maybe_unused]] Socket* sock, std::uint32_t errorCode)
	    {
		    std::cout << "Error " << errorCode << "\n";
	    });

	socket.setLocalEndpoint({ "localhost", "http", EAddressType::IPv4 });

	if (!socket.open())
	{
		std::cout << "Failed to open socket!\n";
		return 1;
	}

	if (!socket.listen(10))
	{
		std::cout << "Failed to listen on connections!\n";
		return 2;
	}

	std::cout << "Hosting on " << socket.getLocalEndpoint().toString() << "\n";

	std::string website = "HTTP/1.0 200 OK\nContent-Type: text/html\n\n<html><head><title>Hehe</title></head><body><p3>DUDE THIS GO BRR</p3></body></html>";

	while (true)
	{
		auto client = socket.accept();
		if (client.isOpen())
		{
			std::cout << "Client connected from " << client.getRemoteEndpoint().toString() << "\n";
			client.write(website.data(), website.size());
			using namespace std::chrono_literals;
			std::this_thread::sleep_for(10ms);
		}
	}
}