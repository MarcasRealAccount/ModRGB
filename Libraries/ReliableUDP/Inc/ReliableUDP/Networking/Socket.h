#pragma once

#include "Endpoint.h"

namespace ReliableUDP::Networking
{
	enum class ESocketType
	{
		TCP,
		UDP
	};

	class Socket
	{
	public:
		using ErrorReportCallback = void (*)(Socket* socket, std::uint32_t errorCode);

	public:
		Socket() : m_Type(ESocketType::TCP) {}
		Socket(ESocketType type) : m_Type(type) {}
		Socket(Socket&& move) noexcept;
		~Socket();

		std::size_t read(void* buf, std::size_t len);
		std::size_t readFrom(void* buf, std::size_t len, Endpoint& endpoint);
		std::size_t write(const void* buf, std::size_t len);
		std::size_t writeTo(const void* buf, std::size_t len, Endpoint endpoint);

		bool open();
		void close();

		void closeW();
		void closeR();
		void closeRW();

		bool connect(Endpoint endpoint);

		bool   listen(std::uint32_t backlog);
		Socket accept();

		void setType(ESocketType type);
		void setLocalEndpoint(Endpoint endpoint);
		void setWriteTimeout(std::uint32_t timeout);
		void setReadTimeout(std::uint32_t timeout);
		void setErrorCallback(ErrorReportCallback callback);

		auto getType() const { return m_Type; }
		auto getLocalEndpoint() const { return m_LocalEndpoint; }
		auto getRemoteEndpoint() const { return m_RemoteEndpoint; }
		auto getWriteTimeout() const { return m_WriteTimeout; }
		auto getReadTimeout() const { return m_ReadTimeout; }
		auto getSocket() const { return m_Socket; }
		bool isOpen() const { return m_Socket != ~0ULL; }
		auto getErrorCallback() const { return m_ErrorCallback; }

	private:
		void reportError(std::uint32_t errorCode);

	private:
		ESocketType m_Type;
		Endpoint    m_LocalEndpoint;
		Endpoint    m_RemoteEndpoint;

		std::uint32_t m_WriteTimeout = 2000;
		std::uint32_t m_ReadTimeout  = 2000;

		std::uintptr_t m_Socket = ~0ULL;

		ErrorReportCallback m_ErrorCallback;
	};
} // namespace ReliableUDP::Networking