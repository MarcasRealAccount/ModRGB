#include <ModRGB/Networking/Socket.h>

#include <thread>

void client()
{
	using namespace ModRGB::Networking;

	IPv4Socket client(ESocketType::TCP);
	client.connect()
}

int main(int argc, char** argv)
{
	using namespace ModRGB::Networking;

	IPv4Socket server(ESocketType::TCP);
	server.setAddress({});
	server.open();
	if (!server.opened())
		return 1;
	server.listen(0);
	
	std::thread c { &client, server.accept() };



	return 0;
}