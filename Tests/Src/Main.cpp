#include <ModRGB/Server/Server.h>

#include <iostream>
#include <sstream>
#include <string>
#include <thread>

int main(int argc, char** argv)
{
	ModRGB::Server server({ 127, 0, 0, 1 }, 27015);
	while (true)
	{
		server.update();
	}
	return 0;
}