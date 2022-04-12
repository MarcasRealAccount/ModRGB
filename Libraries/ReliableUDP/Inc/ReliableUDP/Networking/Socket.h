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
		Socket() : m_Type(ESocketType::TCP), m_WriteTimeout(2000), m_ReadTimeout(2000), m_Socket(~0ULL), m_ErrorCallback(nullptr), m_UserData(nullptr) {}
		Socket(ESocketType type) : m_Type(type), m_WriteTimeout(2000), m_ReadTimeout(2000), m_Socket(~0ULL), m_ErrorCallback(nullptr), m_UserData(nullptr) {}
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
		void setErrorCallback(ErrorReportCallback callback, void* userData);

		auto getType() const { return m_Type; }
		auto getLocalEndpoint() const { return m_LocalEndpoint; }
		auto getRemoteEndpoint() const { return m_RemoteEndpoint; }
		auto getWriteTimeout() const { return m_WriteTimeout; }
		auto getReadTimeout() const { return m_ReadTimeout; }
		auto getSocket() const { return m_Socket; }
		bool isOpen() const { return m_Socket != ~0ULL; }
		auto getErrorCallback() const { return m_ErrorCallback; }
		auto getUserData() const { return m_UserData; }

	private:
		void reportError(std::uint32_t errorCode);

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