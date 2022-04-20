#pragma once

#include "Endpoint.h"

#include <string_view>

namespace ReliableUDP::Networking
{
	enum class ESocketError : std::uint32_t
	{
		Unknown = 0,
		KernelError,
		NoAccess,
		AFNotSupported,
		LowMemory,
		InsufficientPermissions,
		ProtocolNotSupported,
		TypeNotSupported,
		Interrupted,
		InvalidArgument,
		AddressNotAvailable,
		ConnectionRefused,
		NetworkUnreachable,
		HostUnreachable,
		ListenUnsupported,
		AlreadyConnected,
		NetworkDown,
		HostDown
	};

	std::string_view GetSocketErrorString(ESocketError error);

	enum class ESocketType
	{
		TCP,
		UDP
	};

	class Socket
	{
	public:
		using ErrorReportCallback = void (*)(Socket* socket, void* userData, ESocketError error);

	public:
		Socket() : m_Type(ESocketType::TCP), m_WriteTimeout(2000), m_ReadTimeout(2000), m_Socket(~0ULL), m_ErrorCallback(nullptr), m_UserData(nullptr) {}
		Socket(ESocketType type, std::uint32_t writeTimeout = 2000, std::uint32_t readTimeout = 2000) : m_Type(type), m_WriteTimeout(writeTimeout), m_ReadTimeout(readTimeout), m_Socket(~0ULL), m_ErrorCallback(nullptr), m_UserData(nullptr) {}
		Socket(Socket&& move) noexcept;
		~Socket();

		std::size_t read(void* buf, std::size_t len);
		std::size_t readFrom(void* buf, std::size_t len, Endpoint& endpoint);
		std::size_t write(const void* buf, std::size_t len);
		std::size_t writeTo(const void* buf, std::size_t len, Endpoint endpoint);

		bool bind(Endpoint endpoint);
		bool connect(Endpoint endpoint);
		void close();

		void closeW();
		void closeR();
		void closeRW();

		bool   listen(std::uint32_t backlog);
		Socket accept();

		void setType(ESocketType type);
		// timeout == 0: Non blocking
		void setWriteTimeout(std::uint32_t timeout);
		// timeout == 0: Non blocking
		void setReadTimeout(std::uint32_t timeout);
		void setNonBlocking();
		void setErrorCallback(ErrorReportCallback callback, void* userData);

		auto getType() const { return m_Type; }
		auto getLocalEndpoint() const { return m_LocalEndpoint; }
		auto getRemoteEndpoint() const { return m_RemoteEndpoint; }
		auto getWriteTimeout() const { return m_WriteTimeout; }
		auto getReadTimeout() const { return m_ReadTimeout; }
		auto isNonBlocking() const { return m_ReadTimeout == 0 || m_WriteTimeout == 0; }
		auto getSocket() const { return m_Socket; }
		bool isBound() const { return m_Socket != ~0ULL; }
		bool isConnected() const { return m_RemoteEndpoint.isValid(); }
		auto getErrorCallback() const { return m_ErrorCallback; }
		auto getUserData() const { return m_UserData; }

	private:
		void reportError(std::uint32_t errorCode);
		void reportError(ESocketError error);

	private:
		ESocketType m_Type;
		Endpoint    m_LocalEndpoint;
		Endpoint    m_RemoteEndpoint;

		std::uint32_t m_WriteTimeout;
		std::uint32_t m_ReadTimeout;

		std::uintptr_t m_Socket;

		ErrorReportCallback m_ErrorCallback;
		void*               m_UserData;
	};
} // namespace ReliableUDP::Networking