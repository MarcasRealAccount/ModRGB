#pragma once

#include "ReliableUDP/Utils/Core.h"

#include <cstdint>

#include <sstream>
#include <string>
#include <string_view>

#if BUILD_IS_SYSTEM_WINDOWS
#include <WS2tcpip.h>
#include <WinSock2.h>
#else
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#endif