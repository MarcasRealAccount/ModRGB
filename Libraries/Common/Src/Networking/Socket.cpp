#include "ModRGB/Networking/Socket.h"

#if BUILD_IS_SYSTEM_WINDOWS
#include <WinSock2.h>
#else
#include <sys/socket.h>
#endif

namespace ModRGB::Networking
{
#if BUILD_IS_SYSTEM_WINDOWS
	static struct WSAState
	{
	public:
		WSAState()
		{
			int result = WSAStartup(MAKEWORD(2, 2), &m_WSAData);
			if (result)
				return;

			m_Initialized = true;
		}

		~WSAState()
		{
			WSACleanup();
			m_Initialized = false;
		}

		auto IsInitialized() const { return m_Initialized; }

	private:
		WSADATA m_WSAData;
		bool    m_Initialized = false;
	} wsaState;
#endif

	static int GetNativeSocketType(ESocketType type)
	{
		switch (type)
		{
		case ESocketType::TCP: return SOCK_STREAM;
		case ESocketType::UDP: return SOCK_DGRAM;
		default: return SOCK_STREAM;
		}
	}

	static int GetNativeSocketProtocol(ESocketType type)
	{
		switch (type)
		{
		case ESocketType::TCP: return IPPROTO_TCP;
		case ESocketType::UDP: return IPPROTO_UDP;
		default: return IPPROTO_TCP;
		}
	}

	IPv4Socket::IPv4Socket(ESocketType type)
	    : m_Type(type), m_Address(0U), m_Port(0) {}

	IPv4Socket::~IPv4Socket()
	{
		if (m_Opened)
			close();
	}

	bool IPv4Socket::setAddress(std::string_view host, std::uint16_t port)
	{
	}

	void IPv4Socket::setAddress(IPv4Address address)
	{
		m_Address = address;
	}

	void IPv4Socket::setPort(std::uint16_t port)
	{
		m_Port = port;
	}

	std::size_t IPv4Socket::read(void* buf, std::size_t len)
	{
		char* data = reinterpret_cast<char*>(buf);

		std::size_t offset = 0;

		while (len != 0)
		{
			int l = static_cast<int>(std::min(len, static_cast<std::size_t>(std::numeric_limits<int>::max())));

			int r = recv(reinterpret_cast<SOCKET>(m_Socket), data, l, 0);

			if (r <= 0)
				break;

			offset += r;
			len -= r;
			data += r;
		}

		return offset;
	}

	std::size_t IPv4Socket::readFrom(void* buf, std::size_t len, IPv4Address& address, std::uint16_t& port)
	{
		sockaddr_in addr {};
		int         sockSize = static_cast<int>(sizeof(addr));

		int l = static_cast<int>(std::min(len, static_cast<std::size_t>(std::numeric_limits<int>::max())));
		int r = recvfrom(reinterpret_cast<SOCKET>(m_Socket), reinterpret_cast<char*>(buf), l, 0, reinterpret_cast<sockaddr*>(&addr), &sockSize);

		if (r <= 0)
			return 0;

		address = ntohl(addr.sin_addr.s_addr);
		port    = ntohs(addr.sin_port);

		return static_cast<std::size_t>(r);
	}

	std::size_t IPv4Socket::write(void* buf, std::size_t len)
	{
		char* data = reinterpret_cast<char*>(buf);

		std::size_t offset = 0;

		while (len != 0)
		{
			int l = static_cast<int>(std::min(len, static_cast<std::size_t>(std::numeric_limits<int>::max())));

			int r = send(reinterpret_cast<SOCKET>(m_Socket), data, l, 0);

			if (r <= 0)
				break;

			offset += r;
			len -= r;
			data += r;
		}

		return offset;
	}

	std::size_t IPv4Socket::writeTo(void* buf, std::size_t len, IPv4Address address, std::uint16_t port)
	{
		sockaddr_in addr {
			.sin_family = AF_INET,
			.sin_port   = htons(port),
			.sin_addr {
			    .S_un {
			        .S_addr = htonl(address.m_Num) } }
		};

		char* data = reinterpret_cast<char*>(buf);

		std::size_t offset = 0;

		while (len != 0)
		{
			int l = static_cast<int>(std::min(len, static_cast<std::size_t>(std::numeric_limits<int>::max())));

			int r = sendto(reinterpret_cast<SOCKET>(m_Socket), data, l, 0, reinterpret_cast<sockaddr*>(&addr), sizeof(addr));

			if (r <= 0)
				break;

			offset += r;
			len -= r;
			data += r;
		}

		return offset;
	}

	void IPv4Socket::open()
	{
#if BUILD_IS_SYSTEM_WINDOWS
		if (!wsaState.IsInitialized())
			return;

		m_Socket = static_cast<std::uintptr_t>(WSASocketW(AF_INET, GetNativeSocketType(m_Type), GetNativeSocketProtocol(m_Type), nullptr, 0, WSA_FLAG_OVERLAPPED));
		m_Opened = m_Socket != ~0ULL;

		sockaddr_in sa {
			.sin_family = AF_INET,
			.sin_port   = htons(m_Port),
			.sin_addr {
			    .S_un {
			        .S_addr = htonl(m_Address.m_Num) } }
		};

		if (bind(static_cast<SOCKET>(m_Socket), reinterpret_cast<sockaddr*>(&sa), sizeof(sa)) == -1)
		{
			close();
			return;
		}
#else
		m_Socket = static_cast<std::uintptr_t>(socket(AF_INET, GetNativeSocketType(m_Type), GetNativeSocketProtocol(m_Type)));
		m_Opened = m_Socket != ~0ULL;

		sockaddr_in sa {
			.sin_family = AF_INET,
			.sin_port   = htons(m_Port),
			.sin_addr {
			    .S_un {
			        .S_addr = htonl(m_Address.m_Num) } }
		};

		if (bind(static_cast<int>(m_Socket), reinterpret_cast<sockaddr*>(&sa), sizeof(sa)) == -1)
		{
			close();
			return;
		}
#endif
	}

	void IPv4Socket::close()
	{
		if (!m_Opened)
			return;

#if BUILD_IS_SYSTEM_WINDOWS
		::closesocket(reinterpret_cast<SOCKET>(m_Socket));
#else
		::close(static_cast<int>(m_Socket));
#endif
		m_Opened = false;
	}

	void IPv4Socket::closeW()
	{
		::shutdown(reinterpret_cast<SOCKET>(m_Socket), SD_SEND);
	}

	void IPv4Socket::closeR()
	{
		::shutdown(reinterpret_cast<SOCKET>(m_Socket), SD_RECEIVE);
	}

	void IPv4Socket::closeRW()
	{
		::shutdown(reinterpret_cast<SOCKET>(m_Socket), SD_BOTH);
	}

	bool IPv4Socket::listen(std::uint32_t backlog)
	{
		if (!m_Opened)
			return false;

#if BUILD_IS_SYSTEM_WINDOWS
		return ::listen(static_cast<SOCKET>(m_Socket), static_cast<int>(backlog)) != -1;
#else
		return ::listen(static_cast<int>(m_Socket), static_cast<int>(backlog)) != -1;
#endif
	}

	IPv4Socket IPv4Socket::accept()
	{
		sockaddr_in addr    = {};
		int         addrlen = sizeof(addr);

		IPv4Socket socket = { m_Type };
#if BUILD_IS_SYSTEM_WINDOWS
		socket.m_Socket = ::accept(static_cast<SOCKET>(m_Socket), reinterpret_cast<sockaddr*>(&addr), &addrlen);
#else
		socket.m_Socket = ::accept(static_cast<int>(m_Socket), reinterpret_cast<sockaddr*>(&addr), &addrlen);
#endif
		socket.m_Address = ntohl(addr.sin_addr.s_addr);
		socket.m_Port    = ntohs(addr.sin_port);
		socket.m_Opened  = true;
		return socket;
	}
} // namespace ModRGB::Networking