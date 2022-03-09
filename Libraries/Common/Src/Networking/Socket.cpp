#include "ModRGB/Networking/Socket.h"

#if BUILD_IS_SYSTEM_WINDOWS
#include <WinSock2.h>
#else
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
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

	static ErrorReportCallback s_ErrorReportCallback = nullptr;

	static void ReportError(std::uint32_t errorCode)
	{
		if (s_ErrorReportCallback)
			s_ErrorReportCallback(errorCode);
	}

	void SetErrorReportingCallback(ErrorReportCallback callback)
	{
		s_ErrorReportCallback = callback;
	}

	IPv4Socket::IPv4Socket(ESocketType type)
	    : m_Type(type), m_LocalAddress(0U), m_RemoteAddress(0U), m_LocalPort(0U), m_RemotePort(0U) {}

	IPv4Socket::IPv4Socket(IPv4Socket&& move) noexcept
	    : m_Type(move.m_Type), m_LocalAddress(move.m_LocalAddress), m_RemoteAddress(move.m_RemoteAddress), m_LocalPort(move.m_LocalPort), m_RemotePort(move.m_RemotePort), m_WriteTimeout(move.m_WriteTimeout), m_ReadTimeout(move.m_ReadTimeout), m_Opened(move.m_Opened), m_Errored(move.m_Errored), m_Socket(move.m_Socket)
	{
		move.m_Socket = ~0ULL;
		move.m_Opened = false;
	}

	IPv4Socket::~IPv4Socket()
	{
		if (m_Opened)
			close();
	}

	void IPv4Socket::setLocalAddress(IPv4Address address, std::uint16_t port)
	{
		if (!m_Opened)
		{
			m_LocalAddress = address;
			m_LocalPort    = port;
		}
	}

	void IPv4Socket::setWriteTimeout(std::uint32_t timeout)
	{
		if (m_Opened)
		{
#if BUILD_IS_SYSTEM_WINDOWS
			if (::setsockopt(static_cast<SOCKET>(m_Socket), SOL_SOCKET, SO_SNDTIMEO, reinterpret_cast<char*>(&timeout), sizeof(timeout)) < 0)
				ReportError(WSAGetLastError());
			else
				m_WriteTimeout = timeout;
#else
			if (::setsockopt(static_cast<int>(m_Socket), SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout)) < 0)
				ReportError(errno);
			else
				m_WriteTimeout = timeout;
#endif
		}
		else
		{
			m_WriteTimeout = timeout;
		}
	}

	void IPv4Socket::setReadTimeout(std::uint32_t timeout)
	{
		if (m_Opened)
		{
#if BUILD_IS_SYSTEM_WINDOWS
			if (::setsockopt(static_cast<SOCKET>(m_Socket), SOL_SOCKET, SO_RCVTIMEO, reinterpret_cast<char*>(&timeout), sizeof(timeout)) < 0)
				ReportError(WSAGetLastError());
			else
				m_WriteTimeout = timeout;
#else
			if (::setsockopt(static_cast<int>(m_Socket), SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)) < 0)
				ReportError(errno);
			else
				m_ReadTimeout = timeout;
#endif
		}
		else
		{
			m_ReadTimeout = timeout;
		}
	}

	std::size_t IPv4Socket::read(void* buf, std::size_t len)
	{
		if (!m_Opened)
			return 0U;

		char* data = reinterpret_cast<char*>(buf);

		std::size_t offset = 0;

		while (len != 0)
		{
#if BUILD_IS_SYSTEM_WINDOWS
			int l = static_cast<int>(std::min(len, static_cast<std::size_t>(std::numeric_limits<int>::max())));

			int r = recv(static_cast<SOCKET>(m_Socket), data, l, 0);
			if (r < 0)
			{
				std::uint32_t errorCode = WSAGetLastError();

				switch (errorCode)
				{
				case WSAENETDOWN: [[fallthrough]];
				case WSAENOTCONN: [[fallthrough]];
				case WSAENETRESET: [[fallthrough]];
				case WSAENOTSOCK: [[fallthrough]];
				case WSAESHUTDOWN: [[fallthrough]];
				case WSAECONNABORTED: [[fallthrough]];
				case WSAECONNRESET:
					close();
					break;
				default:
					ReportError(errorCode);
				}
			}
#else
			ssize_t r = recv(static_cast<int>(m_Socket), data, len, 0);
			if (r < 0)
			{
				std::uint32_t errorCode = errno;
				switch (errorCode)
				{
				case ENETDOWN: [[fallthrough]];
				case ENOTCONN: [[fallthrough]];
				case ENETRESET: [[fallthrough]];
				case ENOTSOCK: [[fallthrough]];
				case ESHUTDOWN: [[fallthrough]];
				case ECONNABORTED: [[fallthrough]];
				case ECONNRESET: [[fallthrough]];
				case EBADF:
					close();
					break;
				default:
					ReportError(errorCode);
				}
			}
#endif

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
		if (!m_Opened)
			return 0U;

		sockaddr_in addr {};

#if BUILD_IS_SYSTEM_WINDOWS
		int sockSize = static_cast<int>(sizeof(addr));
		int l        = static_cast<int>(std::min(len, static_cast<std::size_t>(std::numeric_limits<int>::max())));
		int r        = recvfrom(static_cast<SOCKET>(m_Socket), reinterpret_cast<char*>(buf), l, 0, reinterpret_cast<sockaddr*>(&addr), &sockSize);
		if (r < 0)
		{
			std::uint32_t errorCode = WSAGetLastError();

			switch (errorCode)
			{
			case WSAENETDOWN: [[fallthrough]];
			case WSAENOTCONN: [[fallthrough]];
			case WSAENETRESET: [[fallthrough]];
			case WSAENOTSOCK: [[fallthrough]];
			case WSAESHUTDOWN: [[fallthrough]];
			case WSAECONNABORTED: [[fallthrough]];
			case WSAECONNRESET:
				close();
				break;
			default:
				ReportError(errorCode);
			}
		}
#else
		addr.sin_len = sizeof(addr);
		socklen_t sockSize = static_cast<socklen_t>(sizeof(addr));
		ssize_t r = recvfrom(static_cast<int>(m_Socket), buf, len, 0, reinterpret_cast<sockaddr*>(&addr), &sockSize);
		if (r < 0)
		{
			std::uint32_t errorCode = errno;
			switch (errorCode)
			{
			case ENETDOWN: [[fallthrough]];
			case ENOTCONN: [[fallthrough]];
			case ENETRESET: [[fallthrough]];
			case ENOTSOCK: [[fallthrough]];
			case ESHUTDOWN: [[fallthrough]];
			case ECONNABORTED: [[fallthrough]];
			case ECONNRESET: [[fallthrough]];
			case EBADF:
				close();
				break;
			default:
				ReportError(errorCode);
			}
		}
#endif

		address = addr.sin_addr.s_addr;
		port    = ntohs(addr.sin_port);

		return static_cast<std::size_t>(r);
	}

	std::size_t IPv4Socket::write(const void* buf, std::size_t len)
	{
		if (!m_Opened)
			return 0U;

		const char* data = reinterpret_cast<const char*>(buf);

		std::size_t offset = 0;

		while (len != 0)
		{
#if BUILD_IS_SYSTEM_WINDOWS
			int l = static_cast<int>(std::min(len, static_cast<std::size_t>(std::numeric_limits<int>::max())));

			int r = send(static_cast<SOCKET>(m_Socket), data, l, 0);
			if (r < 0)
			{
				std::uint32_t errorCode = WSAGetLastError();

				switch (errorCode)
				{
				case WSAENETDOWN: [[fallthrough]];
				case WSAENOTCONN: [[fallthrough]];
				case WSAENETRESET: [[fallthrough]];
				case WSAENOTSOCK: [[fallthrough]];
				case WSAESHUTDOWN: [[fallthrough]];
				case WSAECONNABORTED: [[fallthrough]];
				case WSAECONNRESET:
					close();
					break;
				default:
					ReportError(errorCode);
				}
			}
#else
			ssize_t r = send(static_cast<int>(m_Socket), data, len, 0);
			if (r < 0)
			{
				std::uint32_t errorCode = errno;
				switch (errorCode)
				{
				case ENETDOWN: [[fallthrough]];
				case ENOTCONN: [[fallthrough]];
				case ENETRESET: [[fallthrough]];
				case ENOTSOCK: [[fallthrough]];
				case ESHUTDOWN: [[fallthrough]];
				case ECONNABORTED: [[fallthrough]];
				case ECONNRESET: [[fallthrough]];
				case EBADF:
					close();
					break;
				default:
					ReportError(errorCode);
				}
			}
#endif
			if (r <= 0)
				break;

			offset += r;
			len -= r;
			data += r;
		}

		return offset;
	}

	std::size_t IPv4Socket::writeTo(const void* buf, std::size_t len, IPv4Address address, std::uint16_t port)
	{
		if (!m_Opened)
			return 0U;

		sockaddr_in addr {
			.sin_family = AF_INET,
			.sin_port   = htons(port)
		};
		addr.sin_addr.s_addr = address.m_Num;

		const char* data = reinterpret_cast<const char*>(buf);

		std::size_t offset = 0;

		while (len != 0)
		{
#if BUILD_IS_SYSTEM_WINDOWS
			int l = static_cast<int>(std::min(len, static_cast<std::size_t>(std::numeric_limits<int>::max())));

			int r = sendto(static_cast<SOCKET>(m_Socket), data, l, 0, reinterpret_cast<sockaddr*>(&addr), sizeof(addr));
			if (r < 0)
			{
				std::uint32_t errorCode = WSAGetLastError();

				switch (errorCode)
				{
				case WSAENETDOWN: [[fallthrough]];
				case WSAENOTCONN: [[fallthrough]];
				case WSAENETRESET: [[fallthrough]];
				case WSAENOTSOCK: [[fallthrough]];
				case WSAESHUTDOWN: [[fallthrough]];
				case WSAECONNABORTED: [[fallthrough]];
				case WSAECONNRESET:
					close();
					break;
				default:
					ReportError(errorCode);
				}
			}
#else
			ssize_t r = sendto(static_cast<int>(m_Socket), data, len, 0, reinterpret_cast<sockaddr*>(&addr), sizeof(addr));
			if (r < 0)
			{
				std::uint32_t errorCode = errno;
				switch (errorCode)
				{
				case ENETDOWN: [[fallthrough]];
				case ENOTCONN: [[fallthrough]];
				case ENETRESET: [[fallthrough]];
				case ENOTSOCK: [[fallthrough]];
				case ESHUTDOWN: [[fallthrough]];
				case ECONNABORTED: [[fallthrough]];
				case ECONNRESET: [[fallthrough]];
				case EBADF:
					close();
					break;
				default:
					ReportError(errorCode);
				}
			}
#endif
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
		if (m_Opened)
			return;

#if BUILD_IS_SYSTEM_WINDOWS
		if (!wsaState.IsInitialized())
			return;

		m_Socket = static_cast<std::uintptr_t>(::socket(AF_INET, GetNativeSocketType(m_Type), GetNativeSocketProtocol(m_Type)));
		if (m_Socket == ~0ULL)
		{
			ReportError(WSAGetLastError());
			return;
		}
		m_Opened = true;

		sockaddr_in sa {
			.sin_family = AF_INET,
			.sin_port   = htons(m_LocalPort)
		};
		sa.sin_addr.s_addr = m_LocalAddress.m_Num;

		if (::bind(static_cast<SOCKET>(m_Socket), reinterpret_cast<sockaddr*>(&sa), sizeof(sa)) < 0)
		{
			ReportError(WSAGetLastError());
			close();
			return;
		}

		if (::setsockopt(static_cast<SOCKET>(m_Socket), SOL_SOCKET, SO_SNDTIMEO, reinterpret_cast<char*>(&m_WriteTimeout), sizeof(m_WriteTimeout)) < 0)
			ReportError(WSAGetLastError());
		if (::setsockopt(static_cast<SOCKET>(m_Socket), SOL_SOCKET, SO_RCVTIMEO, reinterpret_cast<char*>(&m_ReadTimeout), sizeof(m_ReadTimeout)) < 0)
			ReportError(WSAGetLastError());
#else
		m_Socket = static_cast<std::uintptr_t>(::socket(AF_INET, GetNativeSocketType(m_Type), GetNativeSocketProtocol(m_Type)));
		if (m_Socket == ~0ULL)
		{
			ReportError(errno);
			return;
		}
		m_Opened = true;

		sockaddr_in sa {
			.sin_len = sizeof(sa),
			.sin_family = AF_INET,
			.sin_port = htons(m_LocalPort)
		};
		sa.sin_addr.s_addr = m_LocalAddress.m_Num;

		if (::bind(static_cast<int>(m_Socket), reinterpret_cast<sockaddr*>(&sa), sizeof(sa)) < 0)
		{
			ReportError(errno);
			close();
			return;
		}

		if (::setsockopt(static_cast<int>(m_Socket), SOL_SOCKET, SO_SNDTIMEO, &m_WriteTimeout, sizeof(m_WriteTimeout)) < 0)
			ReportError(errno);
		if (::setsockopt(static_cast<int>(m_Socket), SOL_SOCKET, SO_RCVTIMEO, &m_ReadTimeout, sizeof(m_ReadTimeout)) < 0)
			ReportError(errno);
#endif
	}

	void IPv4Socket::close()
	{
		if (!m_Opened)
			return;

#if BUILD_IS_SYSTEM_WINDOWS
		if (::closesocket(static_cast<SOCKET>(m_Socket)) < 0)
			ReportError(WSAGetLastError());
#else
		if (::close(static_cast<int>(m_Socket)) < 0)
			ReportError(errno);
#endif
		m_RemoteAddress = { 0U };
		m_RemotePort    = 0U;
		m_Opened        = false;
	}

	void IPv4Socket::closeW()
	{
#if BUILD_IS_SYSTEM_WINDOWS
		if (::shutdown(static_cast<SOCKET>(m_Socket), SD_SEND) < 0)
			ReportError(WSAGetLastError());
#else
		if (::shutdown(static_cast<int>(m_Socket), SHUT_WR) < 0)
			ReportError(errno);
#endif
	}

	void IPv4Socket::closeR()
	{
#if BUILD_IS_SYSTEM_WINDOWS
		if (::shutdown(static_cast<SOCKET>(m_Socket), SD_RECEIVE) < 0)
			ReportError(WSAGetLastError());
#else
		if (::shutdown(static_cast<int>(m_Socket), SHUT_RD) < 0)
			ReportError(errno);
#endif
	}

	void IPv4Socket::closeRW()
	{
#if BUILD_IS_SYSTEM_WINDOWS
		if (::shutdown(static_cast<SOCKET>(m_Socket), SD_BOTH) < 0)
			ReportError(WSAGetLastError());
#else
		if (::shutdown(static_cast<int>(m_Socket), SHUT_RDWR) < 0)
			ReportError(errno);
#endif
	}

	bool IPv4Socket::connectResolve(std::string_view host, std::uint16_t port)
	{
		// TODO(MarcasRealAccount): Implement host resolving functionality!!!
		return connect({ 0U }, port);
	}

	bool IPv4Socket::connect(IPv4Address address, std::uint16_t port)
	{
		if (m_Opened)
			return false;

#if BUILD_IS_SYSTEM_WINDOWS
		if (!wsaState.IsInitialized())
			return false;

		m_Socket = static_cast<std::uintptr_t>(::WSASocketW(AF_INET, GetNativeSocketType(m_Type), GetNativeSocketProtocol(m_Type), nullptr, 0, WSA_FLAG_OVERLAPPED));
		if (m_Socket == ~0ULL)
		{
			ReportError(WSAGetLastError());
			return false;
		}
		m_Opened = true;

		sockaddr_in addr = {
			.sin_family = AF_INET,
			.sin_port   = htons(port)
		};
		addr.sin_addr.s_addr = address.m_Num;
		int addrlen          = static_cast<int>(sizeof(addr));

		if (::connect(static_cast<SOCKET>(m_Socket), reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) < 0)
		{
			ReportError(WSAGetLastError());
			close();
			return false;
		}
		m_RemoteAddress = address;
		m_RemotePort    = port;

		if (::setsockopt(static_cast<SOCKET>(m_Socket), SOL_SOCKET, SO_SNDTIMEO, reinterpret_cast<char*>(&m_WriteTimeout), sizeof(m_WriteTimeout)) < 0)
			ReportError(WSAGetLastError());
		if (::setsockopt(static_cast<SOCKET>(m_Socket), SOL_SOCKET, SO_RCVTIMEO, reinterpret_cast<char*>(&m_ReadTimeout), sizeof(m_ReadTimeout)) < 0)
			ReportError(WSAGetLastError());

		if (::getsockname(static_cast<SOCKET>(m_Socket), reinterpret_cast<sockaddr*>(&addr), &addrlen) < 0)
		{
			ReportError(WSAGetLastError());
		}
		else
		{
			m_LocalAddress = addr.sin_addr.s_addr;
			m_LocalPort    = ntohs(addr.sin_port);
		}
		return true;
#else
		m_Socket = static_cast<std::uintptr_t>(::socket(AF_INET, GetNativeSocketType(m_Type), GetNativeSocketProtocol(m_Type)));
		if (m_Socket == ~0ULL)
		{
			ReportError(errno);
			return false;
		}
		m_Opened = true;

		sockaddr_in addr = {
			.sin_len = sizeof(addr),
			.sin_family = AF_INET,
			.sin_port = htons(port)
		};
		addr.sin_addr.s_addr = address.m_Num;
		socklen_t addrlen = static_cast<socklen_t>(sizeof(addr));

		if (::connect(static_cast<int>(m_Socket), reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) < 0)
		{
			ReportError(errno);
			close();
			return false;
		}
		m_RemoteAddress = address;
		m_RemotePort = port;

		if (::setsockopt(static_cast<int>(m_Socket), SOL_SOCKET, SO_SNDTIMEO, &m_WriteTimeout, sizeof(m_WriteTimeout)) < 0)
			ReportError(errno);
		if (::setsockopt(static_cast<int>(m_Socket), SOL_SOCKET, SO_RCVTIMEO, &m_ReadTimeout, sizeof(m_ReadTimeout)) < 0)
			ReportError(errno);

		if (::getsockname(static_cast<int>(m_Socket), reinterpret_cast<sockaddr*>(&addr), &addrlen) < 0)
		{
			ReportError(errno);
		}
		else
		{
			m_LocalAddress = addr.sin_addr.s_addr;
			m_LocalPort = ntohs(addr.sin_port);
		}
		return true;
#endif
	} // namespace ModRGB::Networking

	bool IPv4Socket::listen(std::uint32_t backlog)
	{
		if (!m_Opened)
			return false;

#if BUILD_IS_SYSTEM_WINDOWS
		if (::listen(static_cast<SOCKET>(m_Socket), static_cast<int>(backlog)) < 0)
		{
			ReportError(WSAGetLastError());
			return false;
		}
		return true;
#else
		if (::listen(static_cast<int>(m_Socket), static_cast<int>(backlog)) < 0)
		{
			ReportError(errno);
			return false;
		}
		return true;
#endif
	}

	IPv4Socket IPv4Socket::accept()
	{
		if (!m_Opened)
			return { m_Type };

		sockaddr_in addr = {};

		IPv4Socket socket = { m_Type };
#if BUILD_IS_SYSTEM_WINDOWS
		int addrlen     = static_cast<int>(sizeof(addr));
		socket.m_Socket = static_cast<std::uintptr_t>(::accept(static_cast<SOCKET>(m_Socket), reinterpret_cast<sockaddr*>(&addr), &addrlen));
		if (socket.m_Socket == ~0ULL)
		{
			ReportError(WSAGetLastError());
			return socket;
		}
		socket.m_Opened = true;

		if (::setsockopt(static_cast<SOCKET>(m_Socket), SOL_SOCKET, SO_SNDTIMEO, reinterpret_cast<char*>(&m_WriteTimeout), sizeof(m_WriteTimeout)) < 0)
			ReportError(WSAGetLastError());
		if (::setsockopt(static_cast<SOCKET>(m_Socket), SOL_SOCKET, SO_RCVTIMEO, reinterpret_cast<char*>(&m_ReadTimeout), sizeof(m_ReadTimeout)) < 0)
			ReportError(WSAGetLastError());
#else
		addr.sin_len = sizeof(addr);
		socklen_t addrlen = static_cast<socklen_t>(sizeof(addr));
		socket.m_Socket = static_cast<std::uintptr_t>(::accept(static_cast<int>(m_Socket), reinterpret_cast<sockaddr*>(&addr), &addrlen));
		if (socket.m_Socket == ~0ULL)
		{
			ReportError(errno);
			return socket;
		}
		socket.m_Opened = true;

		if (::setsockopt(static_cast<int>(socket.m_Socket), SOL_SOCKET, SO_SNDTIMEO, &m_WriteTimeout, sizeof(m_WriteTimeout)) < 0)
			ReportError(errno);
		if (::setsockopt(static_cast<int>(socket.m_Socket), SOL_SOCKET, SO_RCVTIMEO, &m_ReadTimeout, sizeof(m_ReadTimeout)) < 0)
			ReportError(errno);
#endif
		socket.m_RemoteAddress = addr.sin_addr.s_addr;
		socket.m_RemotePort    = ntohs(addr.sin_port);

#if BUILD_IS_SYSTEM_WINDOWS
		if (::getsockname(static_cast<SOCKET>(socket.m_Socket), reinterpret_cast<sockaddr*>(&addr), &addrlen) < 0)
		{
			ReportError(WSAGetLastError());
		}
		else
		{
			socket.m_LocalAddress = addr.sin_addr.s_addr;
			socket.m_LocalPort    = ntohs(addr.sin_port);
		}
#else
		if (::getsockname(static_cast<int>(socket.m_Socket), reinterpret_cast<sockaddr*>(&addr), &addrlen) < 0)
		{
			ReportError(errno);
		}
		else
		{
			socket.m_LocalAddress = addr.sin_addr.s_addr;
			socket.m_LocalPort = ntohs(addr.sin_port);
		}
#endif

		return socket;
	}
} // namespace ModRGB::Networking