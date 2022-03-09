#include <ModRGB/Networking/Socket.h>

#include <iostream>
#include <sstream>
#include <string>
#include <thread>

void client()
{
	using namespace ModRGB::Networking;

	IPv4Socket client(ESocketType::TCP);
	if (!client.connect({ 127, 0, 0, 1 }, 27015))
	{
		std::cout << "Client: Failed to connect!\n";
		return;
	}

	std::string msg  = "Hello server!";
	auto        sent = client.write(msg.c_str(), msg.size());

	client.closeW();

	std::ostringstream str;
	str << "Client: Sent " << sent << " Bytes!\n";
	std::cout << str.str();

	char* buf = new char[1025];
	str       = {};
	str << "Client: Server sent \"";
	//while (client.opened())
	//{
		std::size_t received;
		while ((received = client.read(buf, 1024)) != 0)
		{
			buf[received] = '\0';
			str << buf;
		}
	//}
	str << "\"\n";
	std::cout << str.str();

	delete[] buf;
	client.close();
}

int main(int argc, char** argv)
{
	using namespace ModRGB::Networking;

	SetErrorReportingCallback(
	    [](std::uint32_t errorCode)
	    {
			std::ostringstream str;
	str << "Socket Error: " << errorCode << '\n';
	std::cout << str.str(); });

	IPv4Socket server(ESocketType::TCP);
	server.setLocalAddress({ 127, 0, 0, 1 }, 27015);
	server.open();
	if (!server.opened())
	{
		std::cout << "Server: Failed to open\n";
		return 1;
	}
	server.listen(0);

	std::thread c { &client };

	auto clientSocket = server.accept();
	if (clientSocket.opened())
	{
		std::cout << "Server: Client Socket accepted!\n";
		char* buf = new char[1025];

		std::ostringstream str;
		str << "Server: Client sent \"";
		//while (clientSocket.opened())
		//{
			std::size_t received;
			while ((received = clientSocket.read(buf, 1024)) != 0)
			{
				buf[received] = '\0';
				str << buf;
			}
		//}
		str << "\"\n";
		std::cout << str.str();

		std::string msg  = "Hello client!";
		auto        sent = clientSocket.write(msg.c_str(), msg.size());

		clientSocket.closeW();

		str = {};
		str << "Server: Sent " << sent << " Bytes!\n";
		std::cout << str.str();

		delete[] buf;
		clientSocket.close();
	}
	else
	{
		std::cout << "Server: Client Socket accept failed!\n";
	}

	c.join();
	server.close();
	return 0;
}