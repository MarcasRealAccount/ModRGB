#include "ReliableUDP/Networking/Socket.h"
#include "ReliableUDP/Utils/Core.h"

#if BUILD_IS_SYSTEM_WINDOWS
#include <WS2tcpip.h>
#include <WinSock2.h>
#undef GetAddrInfo
#undef FreeAddrInfo
#undef GetNameInfo
#else
#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#endif

namespace ReliableUDP::Networking
{
#if BUILD_IS_SYSTEM_WINDOWS
	static struct WSAState
	{
	public:
		WSAState()
		{
			if (WSAStartup(MAKEWORD(2, 2), &m_WSAData))
				return;

			m_Initialized = true;
		}

		~WSAState()
		{
			WSACleanup();
			m_Initialized = false;
		}

		auto& getWSAData() const { return m_WSAData; }
		auto  isInitialized() const { return m_Initialized; }

	private:
		WSADATA m_WSAData;
		bool    m_Initialized = false;
	} g_WSAState;
#endif

	static std::uint32_t LastError()
	{
#if BUILD_IS_SYSTEM_WINDOWS
		return WSAGetLastError();
#else
		return errno;
#endif
	}

	enum class EShutdownMethod
	{
		Receive,
		Send,
		Both
	};

	static int GetNativeShutdownMethod(EShutdownMethod method)
	{
		switch (method)
		{
#if BUILD_IS_SYSTEM_WINDOWS
		case EShutdownMethod::Receive: return SD_RECEIVE;
		case EShutdownMethod::Send: return SD_SEND;
		case EShutdownMethod::Both: return SD_BOTH;
		default: return SD_BOTH;
#else
		case EShutdownMethod::Receive: return SHUT_RD;
		case EShutdownMethod::Send: return SHUT_WR;
		case EShutdownMethod::Both: return SHUT_RDWR;
		default: return SHUT_RDWR;
#endif
		}
	}

	static std::uintptr_t CreateSocket(int af, int type, int protocol)
	{
		return static_cast<std::uintptr_t>(::socket(af, type, protocol));
	}

	static std::uintptr_t CreateSocketEx(int af, int type, int protocol, [[maybe_unused]] void* lpProtocolInfo, [[maybe_unused]] std::uint32_t g, [[maybe_unused]] std::uint32_t dwFlags)
	{
#if BUILD_IS_SYSTEM_WINDOWS
		return static_cast<std::uintptr_t>(::WSASocketW(af, type, protocol, reinterpret_cast<LPWSAPROTOCOL_INFOW>(lpProtocolInfo), static_cast<GROUP>(g), static_cast<DWORD>(dwFlags)));
#else
		return static_cast<std::uintptr_t>(::socket(af, type, protocol));
#endif
	}

	static int CloseSocket(std::uintptr_t socket)
	{
#if BUILD_IS_SYSTEM_WINDOWS
		return ::closesocket(static_cast<SOCKET>(socket));
#else
		return ::close(static_cast<int>(socket));
#endif
	}

	static int Shutdown(std::uintptr_t socket, EShutdownMethod how)
	{
#if BUILD_IS_SYSTEM_WINDOWS
		return ::shutdown(static_cast<SOCKET>(socket), GetNativeShutdownMethod(how));
#else
		return ::shutdown(static_cast<int>(socket), GetNativeShutdownMethod(how));
#endif
	}

	static int Bind(std::uintptr_t socket, const sockaddr_storage* addr, std::size_t addrSize)
	{
#if BUILD_IS_SYSTEM_WINDOWS
		return ::bind(static_cast<SOCKET>(socket), reinterpret_cast<const sockaddr*>(addr), static_cast<int>(addrSize));
#else
		return ::bind(static_cast<int>(socket), reinterpret_cast<const sockaddr*>(addr), static_cast<socklen_t>(addrSize));
#endif
	}

	static int Connect(std::uintptr_t socket, const sockaddr_storage* addr, std::size_t addrSize)
	{
#if BUILD_IS_SYSTEM_WINDOWS
		return ::connect(static_cast<SOCKET>(socket), reinterpret_cast<const sockaddr*>(addr), static_cast<int>(addrSize));
#else
		return ::connect(static_cast<int>(socket), reinterpret_cast<const sockaddr*>(addr), static_cast<int>(addrSize));
#endif
	}

	static int Listen(std::uintptr_t socket, std::uint32_t backlog)
	{
#if BUILD_IS_SYSTEM_WINDOWS
		return ::listen(static_cast<SOCKET>(socket), static_cast<int>(backlog));
#else
		return ::listen(static_cast<int>(socket), static_cast<int>(backlog));
#endif
	}

	static std::uintptr_t Accept(std::uintptr_t socket, sockaddr_storage* addr, std::size_t* addrSize)
	{
#if BUILD_IS_SYSTEM_WINDOWS
		int  addrSizeS = static_cast<int>(*addrSize);
		auto r         = ::accept(static_cast<SOCKET>(socket), reinterpret_cast<sockaddr*>(addr), &addrSizeS);
		*addrSize      = static_cast<std::size_t>(addrSizeS);
		return static_cast<std::uintptr_t>(r);
#else
		socklen_t addrSizeS = static_cast<socklen_t>(*addrSize);
		auto      r         = ::accept(static_cast<int>(socket), reinterpret_cast<sockaddr*>(addr), &addrSizeS);
		*addrSize           = static_cast<std::size_t>(addrSizeS);
		return static_cast<std::uintptr_t>(r);
#endif
	}

	static int GetSockName(std::uintptr_t socket, sockaddr_storage* addr, std::size_t* addrSize)
	{
#if BUILD_IS_SYSTEM_WINDOWS
		int  addrSizeS = static_cast<int>(*addrSize);
		auto r         = ::getsockname(static_cast<SOCKET>(socket), reinterpret_cast<sockaddr*>(addr), &addrSizeS);
		*addrSize      = static_cast<std::size_t>(addrSizeS);
		return r;
#else
		socklen_t addrSizeS = static_cast<socklen_t>(*addrSize);
		auto      r         = ::getsockname(static_cast<int>(socket), reinterpret_cast<sockaddr*>(addr), &addrSizeS);
		*addrSize           = static_cast<std::size_t>(addrSizeS);
		return r;
#endif
	}

	static int SetSockOpt(std::uintptr_t socket, int level, int optname, const void* optval, std::size_t optlen)
	{
#if BUILD_IS_SYSTEM_WINDOWS
		return ::setsockopt(static_cast<SOCKET>(socket), level, optname, reinterpret_cast<const char*>(optval), static_cast<int>(optlen));
#else
		return ::setsockopt(static_cast<int>(socket), level, optname, optval, static_cast<socklen_t>(optlen));
#endif
	}

	static int SetNonBlocking(std::uintptr_t socket, bool nonBlocking)
	{
#if BUILD_IS_SYSTEM_WINDOWS
		u_long mode = nonBlocking;
		return ::ioctlsocket(static_cast<SOCKET>(socket), FIONBIO, &mode);
#else
		int mode = (::fcntl(static_cast<int>(socket), F_GETFL, 0) & ~O_NONBLOCK) | (nonBlocking * O_NONBLOCK);
		return ::fcntl(static_cast<int>(socket), F_SETFL, mode);
#endif
	}

	static std::make_signed_t<std::size_t> Receive(std::uintptr_t socket, void* buf, std::size_t len, int flags)
	{
#if BUILD_IS_SYSTEM_WINDOWS
		return ::recv(static_cast<SOCKET>(socket), reinterpret_cast<char*>(buf), static_cast<int>(std::min<std::size_t>(len, std::numeric_limits<int>::max())), flags);
#else
		return ::recv(static_cast<int>(socket), buf, len, flags);
#endif
	}

	static std::make_signed_t<std::size_t> ReceiveFrom(std::uintptr_t socket, void* buf, std::size_t len, int flags, sockaddr_storage* addr, std::size_t* addrSize)
	{
#if BUILD_IS_SYSTEM_WINDOWS
		int  addrSizeS = static_cast<int>(*addrSize);
		auto r         = ::recvfrom(static_cast<SOCKET>(socket), reinterpret_cast<char*>(buf), static_cast<int>(std::min<std::size_t>(len, std::numeric_limits<int>::max())), flags, reinterpret_cast<sockaddr*>(addr), &addrSizeS);
		*addrSize      = static_cast<std::size_t>(addrSizeS);
		return r;
#else
		socklen_t addrSizeS = static_cast<socklen_t>(*addrSize);
		auto      r         = ::recvfrom(static_cast<int>(socket), buf, len, flags, reinterpret_cast<sockaddr*>(addr), &addrSizeS);
		*addrSize           = static_cast<std::size_t>(addrSizeS);
		return r;
#endif
	}

	static std::make_signed_t<std::size_t> Send(std::uintptr_t socket, const void* buf, std::size_t len, int flags)
	{
#if BUILD_IS_SYSTEM_WINDOWS
		return ::send(static_cast<SOCKET>(socket), reinterpret_cast<const char*>(buf), static_cast<int>(std::min<std::size_t>(len, std::numeric_limits<int>::max())), flags);
#else
		return ::send(static_cast<int>(socket), buf, len, flags);
#endif
	}

	static std::make_signed_t<std::size_t> SendTo(std::uintptr_t socket, const void* buf, std::size_t len, int flags, sockaddr_storage* addr, std::size_t addrSize)
	{
#if BUILD_IS_SYSTEM_WINDOWS
		return ::sendto(static_cast<SOCKET>(socket), reinterpret_cast<const char*>(buf), static_cast<int>(std::min<std::size_t>(len, std::numeric_limits<int>::max())), flags, reinterpret_cast<sockaddr*>(addr), static_cast<int>(addrSize));
#else
		return ::sendto(static_cast<int>(socket), buf, len, flags, reinterpret_cast<sockaddr*>(addr), static_cast<socklen_t>(addrSize));
#endif
	}

	static int GetAddrInfo(const char* node, const char* service, const addrinfo* hints, addrinfo** results)
	{
		return ::getaddrinfo(node, service, hints, results);
	}

	static void FreeAddrInfo(addrinfo* addrInfo)
	{
		::freeaddrinfo(addrInfo);
	}

	static int GetNameInfo(sockaddr_storage* addr, std::size_t addrSize, char* host, std::size_t nodeSize, char* service, std::size_t serviceSize, int flags)
	{
#if BUILD_IS_SYSTEM_WINDOWS
		return ::getnameinfo(reinterpret_cast<sockaddr*>(addr), static_cast<socklen_t>(addrSize), host, static_cast<DWORD>(nodeSize), service, static_cast<DWORD>(serviceSize), flags);
#else
		return ::getnameinfo(reinterpret_cast<sockaddr*>(addr), static_cast<socklen_t>(addrSize), host, static_cast<socklen_t>(nodeSize), service, static_cast<socklen_t>(serviceSize), flags);
#endif
	}

	static bool IsErrorCodeCloseBased(std::uint32_t errorCode)
	{
		switch (errorCode)
		{
#if BUILD_IS_SYSTEM_WINDOWS
		case WSANOTINITIALISED: [[fallthrough]];
		case WSAENETDOWN: [[fallthrough]];
		case WSAENOTCONN: [[fallthrough]];
		case WSAENETRESET: [[fallthrough]];
		case WSAENOTSOCK: [[fallthrough]];
		case WSAESHUTDOWN: [[fallthrough]];
		case WSAECONNABORTED: [[fallthrough]];
		case WSAETIMEDOUT: [[fallthrough]];
		case WSAECONNRESET:
#else
		case ENOTCONN: [[fallthrough]];
		case ENOTSOCK: [[fallthrough]];
		case ETIMEDOUT: [[fallthrough]];
		case ECONNRESET:
#endif
			return true;
		default: return false;
		}
	}

	static bool IsErrorCodeAnError(std::uint32_t errorCode)
	{
		switch (errorCode)
		{
#if BUILD_IS_SYSTEM_WINDOWS
		case WSAEWOULDBLOCK:
#else
		case EAGAIN:
#endif
			return false;
		default: return true;
		}
	}

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

	static void ToSockAddr(const Endpoint& endpoint, sockaddr_storage* addr, std::size_t* addrSize)
	{
		if (endpoint.isIPv4())
		{
			*addrSize             = sizeof(sockaddr_in);
			sockaddr_in* ipv4     = reinterpret_cast<sockaddr_in*>(addr);
			ipv4->sin_family      = AF_INET;
			ipv4->sin_port        = htons(endpoint.m_Port);
			ipv4->sin_addr.s_addr = htonl(endpoint.m_Address.m_IPv4.m_Value);
		}
		else
		{
			*addrSize          = sizeof(sockaddr_in6);
			sockaddr_in6* ipv6 = reinterpret_cast<sockaddr_in6*>(addr);
			ipv6->sin6_family  = AF_INET6;
			ipv6->sin6_port    = htons(endpoint.m_Port);
			for (std::size_t i = 0; i < 16; ++i)
				ipv6->sin6_addr.s6_addr[i] = endpoint.m_Address.m_IPv6.m_Bytes[15 - i];
		}

#if !BUILD_IS_SYSTEM_WINDOWS
		addr->ss_len = static_cast<std::uint8_t>(*addrSize);
#endif
	}

	static void ToEndpoint(Endpoint& endpoint, const sockaddr_storage* addr)
	{
		switch (addr->ss_family)
		{
		case AF_INET:
		{
			const sockaddr_in* ipv4 = reinterpret_cast<const sockaddr_in*>(addr);
			endpoint.m_Address      = IPv4Address { ntohl(ipv4->sin_addr.s_addr) };
			endpoint.m_Port         = ntohs(ipv4->sin_port);
			break;
		}
		case AF_INET6:
		{
			const sockaddr_in6* ipv6 = reinterpret_cast<const sockaddr_in6*>(addr);
			endpoint.m_Port          = ntohs(ipv6->sin6_port);
			for (std::size_t i = 0; i < 16; ++i)
				endpoint.m_Address.m_IPv6.m_Bytes[15 - i] = ipv6->sin6_addr.s6_addr[i];
			break;
		}
		}
	}

	Socket::Socket(Socket&& move) noexcept
	    : m_Type(move.m_Type), m_LocalEndpoint(move.m_LocalEndpoint), m_RemoteEndpoint(move.m_RemoteEndpoint), m_WriteTimeout(move.m_WriteTimeout), m_ReadTimeout(move.m_ReadTimeout), m_NonBlocking(move.m_NonBlocking), m_Socket(move.m_Socket), m_ErrorCallback(move.m_ErrorCallback), m_UserData(move.m_UserData)
	{
		move.m_Socket = ~0ULL;
	}

	Socket::~Socket()
	{
		if (isOpen())
			close();
	}

	std::size_t Socket::read(void* buf, std::size_t len)
	{
		if (!isOpen())
			return 0U;

		std::uint8_t* data   = reinterpret_cast<std::uint8_t*>(buf);
		std::size_t   offset = 0;

		while (len != 0)
		{
			auto r = Receive(m_Socket, data, len, 0);
			if (r == 0)
			{
				close();
			}
			else if (r < 0)
			{
				auto errorCode = LastError();
				if (IsErrorCodeCloseBased(errorCode))
					close();
				else if (IsErrorCodeAnError(errorCode))
					reportError(errorCode);
			}

			if (r <= 0)
				break;

			offset += r;
			len -= r;
			data += r;
		}

		return offset;
	}

	std::size_t Socket::readFrom(void* buf, std::size_t len, Endpoint& endpoint)
	{
		if (!isOpen())
			return 0U;

		sockaddr_storage addr {};
		std::size_t      addrSize = sizeof(addr);

		auto r = ReceiveFrom(m_Socket, buf, len, 0, &addr, &addrSize);
		if (r == 0)
		{
			close();
			return 0;
		}
		else if (r < 0)
		{
			auto errorCode = LastError();
			if (IsErrorCodeCloseBased(errorCode))
				close();
			else if (IsErrorCodeAnError(errorCode))
				reportError(errorCode);
			return 0;
		}
		else
		{
			ToEndpoint(endpoint, &addr);
			return r;
		}
	}

	std::size_t Socket::write(const void* buf, std::size_t len)
	{
		if (!isOpen())
			return 0U;

		const uint8_t* data   = reinterpret_cast<const std::uint8_t*>(buf);
		std::size_t    offset = 0;

		while (len != 0)
		{
			auto r = Send(m_Socket, data, len, 0);
			if (r == 0)
			{
				close();
			}
			else if (r < 0)
			{
				auto errorCode = LastError();
				if (IsErrorCodeCloseBased(errorCode))
					close();
				else if (IsErrorCodeAnError(errorCode))
					reportError(errorCode);
			}

			if (r <= 0)
				break;

			offset += r;
			len -= r;
			data += r;
		}

		return offset;
	}

	std::size_t Socket::writeTo(const void* buf, std::size_t len, Endpoint endpoint)
	{
		if (!isOpen())
			return 0U;

		sockaddr_storage addr {};
		std::size_t      addrSize = sizeof(addr);
		ToSockAddr(endpoint, &addr, &addrSize);

		const std::uint8_t* data   = reinterpret_cast<const std::uint8_t*>(buf);
		std::size_t         offset = 0;

		while (len != 0)
		{
			auto r = SendTo(m_Socket, data, len, 0, &addr, addrSize);
			if (r == 0)
			{
				close();
			}
			else if (r < 0)
			{
				auto errorCode = LastError();
				if (IsErrorCodeCloseBased(errorCode))
					close();
				else if (IsErrorCodeAnError(errorCode))
					reportError(errorCode);
			}

			if (r <= 0)
				break;

			offset += r;
			len -= r;
			data += r;
		}

		return offset;
	}

	bool Socket::open()
	{
		if (isOpen())
			return false;

#if BUILD_IS_SYSTEM_WINDOWS
		if (!g_WSAState.isInitialized())
			return false;
#endif

		bool isIPv4 = m_LocalEndpoint.isIPv4();

		m_Socket = CreateSocket(isIPv4 ? AF_INET : AF_INET6, GetNativeSocketType(m_Type), GetNativeSocketProtocol(m_Type));
		if (!isOpen())
		{
			reportError(LastError());
			return false;
		}

		sockaddr_storage addr {};
		std::size_t      addrSize = sizeof(addr);
		ToSockAddr(m_LocalEndpoint, &addr, &addrSize);

		if (Bind(m_Socket, &addr, addrSize) < 0)
		{
			reportError(LastError());
			close();
			return false;
		}

		if (SetSockOpt(m_Socket, SOL_SOCKET, SO_SNDTIMEO, &m_WriteTimeout, sizeof(m_WriteTimeout)) < 0)
			reportError(LastError());
		if (SetSockOpt(m_Socket, SOL_SOCKET, SO_RCVTIMEO, &m_ReadTimeout, sizeof(m_ReadTimeout)) < 0)
			reportError(LastError());

		if (SetNonBlocking(m_Socket, m_NonBlocking) < 0)
			reportError(LastError());

		return true;
	}

	void Socket::close()
	{
		if (!isOpen())
			return;

		if (CloseSocket(m_Socket) < 0)
			reportError(LastError());
		m_Socket         = ~0ULL;
		m_RemoteEndpoint = {};
	}

	void Socket::closeW()
	{
		if (!isOpen())
			return;

		if (Shutdown(m_Socket, EShutdownMethod::Send) < 0)
			reportError(LastError());
	}

	void Socket::closeR()
	{
		if (!isOpen())
			return;

		if (Shutdown(m_Socket, EShutdownMethod::Receive) < 0)
			reportError(LastError());
	}

	void Socket::closeRW()
	{
		if (!isOpen())
			return;

		if (Shutdown(m_Socket, EShutdownMethod::Both) < 0)
			reportError(LastError());
	}

	bool Socket::connect(Endpoint endpoint)
	{
		if (isOpen())
			return false;

#if BUILD_IS_SYSTEM_WINDOWS
		if (!g_WSAState.isInitialized())
			return false;
#endif

		bool isIPv4 = endpoint.isIPv4();

		m_Socket = CreateSocketEx(isIPv4 ? AF_INET : AF_INET6, GetNativeSocketType(m_Type), GetNativeSocketProtocol(m_Type), nullptr, 0, 0x1);
		if (!isOpen())
		{
			reportError(LastError());
			return false;
		}

		sockaddr_storage addr {};
		std::size_t      addrSize = sizeof(addr);
		ToSockAddr(endpoint, &addr, &addrSize);

		if (Connect(m_Socket, &addr, addrSize) < 0)
		{
			reportError(LastError());
			close();
			return false;
		}
		m_RemoteEndpoint = endpoint;

		if (SetSockOpt(m_Socket, SOL_SOCKET, SO_SNDTIMEO, &m_WriteTimeout, sizeof(m_WriteTimeout)) < 0)
			reportError(LastError());
		if (SetSockOpt(m_Socket, SOL_SOCKET, SO_RCVTIMEO, &m_ReadTimeout, sizeof(m_ReadTimeout)) < 0)
			reportError(LastError());

		if (SetNonBlocking(m_Socket, m_NonBlocking) < 0)
			reportError(LastError());

		if (GetSockName(m_Socket, &addr, &addrSize) < 0)
		{
			reportError(LastError());
		}
		else
		{
			ToEndpoint(m_LocalEndpoint, &addr);
		}

		return true;
	}

	bool Socket::listen(std::uint32_t backlog)
	{
		if (!isOpen())
			return false;

		if (Listen(m_Socket, backlog) < 0)
		{
			reportError(LastError());
			return false;
		}
		return true;
	}

	Socket Socket::accept()
	{
		Socket socket { m_Type };

		if (!isOpen())
			return socket;

		sockaddr_storage addr {};
		std::size_t      addrSize = sizeof(addr);

		socket.setWriteTimeout(m_WriteTimeout);
		socket.setReadTimeout(m_ReadTimeout);
		socket.setNonBlocking(m_NonBlocking);

		socket.m_Socket = Accept(m_Socket, &addr, &addrSize);
		if (!socket.isOpen())
		{
			reportError(LastError());
			return socket;
		}

		if (SetSockOpt(socket.m_Socket, SOL_SOCKET, SO_SNDTIMEO, &m_WriteTimeout, sizeof(m_WriteTimeout)) < 0)
			reportError(LastError());
		if (SetSockOpt(socket.m_Socket, SOL_SOCKET, SO_RCVTIMEO, &m_ReadTimeout, sizeof(m_ReadTimeout)) < 0)
			reportError(LastError());

		if (SetNonBlocking(socket.m_Socket, socket.m_NonBlocking))
			reportError(LastError());

		ToEndpoint(socket.m_RemoteEndpoint, &addr);

		if (GetSockName(socket.m_Socket, &addr, &addrSize) < 0)
		{
			reportError(LastError());
		}
		else
		{
			ToEndpoint(socket.m_LocalEndpoint, &addr);
		}

		return socket;
	}

	void Socket::setType(ESocketType type)
	{
		if (!isOpen())
			m_Type = type;
	}

	void Socket::setLocalEndpoint(Endpoint endpoint)
	{
		if (!isOpen())
			m_LocalEndpoint = endpoint;
	}

	void Socket::setWriteTimeout(std::uint32_t timeout)
	{
		if (isOpen())
		{
			if (SetSockOpt(m_Socket, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout)) < 0)
				reportError(LastError());
			else
				m_WriteTimeout = timeout;
		}
		else
		{
			m_WriteTimeout = timeout;
		}
	}

	void Socket::setReadTimeout(std::uint32_t timeout)
	{
		if (isOpen())
		{
			if (SetSockOpt(m_Socket, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)) < 0)
				reportError(LastError());
			else
				m_ReadTimeout = timeout;
		}
		else
		{
			m_ReadTimeout = timeout;
		}
	}

	void Socket::setNonBlocking(bool nonBlocking)
	{
		if (isOpen())
		{
			if (SetNonBlocking(m_Socket, nonBlocking) < 0)
				reportError(LastError());
			else
				m_NonBlocking = nonBlocking;
		}
		else
		{
			m_NonBlocking = nonBlocking;
		}
	}

	void Socket::setErrorCallback(ErrorReportCallback callback, void* userData)
	{
		m_ErrorCallback = callback;
		m_UserData      = userData;
	}

	void Socket::reportError(std::uint32_t errorCode)
	{
		if (m_ErrorCallback)
			m_ErrorCallback(this, errorCode);
	}

	//------------
	// Endpoint.h
	//------------

	Endpoint Endpoint::ResolveFromHost(std::string_view node, std::string_view service, EAddressType type)
	{
		std::string nodeStr { node };
		std::string serviceStr { service };

		addrinfo hints {};
		switch (type)
		{
		case EAddressType::Both:
			hints.ai_family = AF_UNSPEC;
			break;
		case EAddressType::IPv4:
			hints.ai_family = AF_INET;
			break;
		case EAddressType::IPv6:
			hints.ai_family = AF_INET6;
			break;
		default:
			hints.ai_family = AF_UNSPEC;
			break;
		}
		hints.ai_socktype  = 0;
		hints.ai_flags     = AI_PASSIVE;
		hints.ai_protocol  = 0;
		hints.ai_canonname = nullptr;
		hints.ai_addr      = nullptr;
		hints.ai_next      = nullptr;
		addrinfo* results;

		if (GetAddrInfo(nodeStr.data(), serviceStr.data(), &hints, &results) < 0)
			return {};

		if (!results)
			return {};

		Endpoint endpoint;
		ToEndpoint(endpoint, reinterpret_cast<const sockaddr_storage*>(results->ai_addr));
		FreeAddrInfo(results);
		return endpoint;
	}

	std::string Endpoint::ToHost(const Endpoint& endpoint)
	{
		std::string      hostName    = std::string(NI_MAXHOST, '\0');
		std::string      serviceName = std::string(NI_MAXSERV, '\0');
		sockaddr_storage addr {};
		std::size_t      addrSize = sizeof(addr);
		ToSockAddr(endpoint, &addr, &addrSize);
		if (GetNameInfo(&addr, addrSize, hostName.data(), hostName.size(), serviceName.data(), serviceName.size(), 0) < 0)
			return "UnknownHost";
		return serviceName + "://" + hostName;
	}
} // namespace ReliableUDP::Networking