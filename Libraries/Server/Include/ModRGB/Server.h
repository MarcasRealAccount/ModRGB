#pragma once

#include "Client.h"

#include <unordered_map>

namespace ModRGB
{
	class Server
	{
	public:
		Server()  = default;
		~Server() = default;

	private:
		std::unordered_map<std::uint64_t, Client> m_Clients;
	};
} // namespace ModRGB