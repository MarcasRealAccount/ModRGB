#pragma once

#include "Networking/Socket.h"
#include "PacketHeader.h"
#include "Utils/RotatingArray.h"

#include <chrono>
#include <random>

namespace ReliableUDP
{
	using Clock = std::chrono::system_clock;

	struct ReadPacketInfo
	{
	public:
		std::uint16_t        m_ID { 0U };
		std::uint16_t        m_Rev { 0U };
		std::uint32_t        m_Start { ~0U };
		std::uint32_t        m_Size { 0U };
		Networking::Endpoint m_Endpoint;
		union
		{
			std::uint32_t m_Bits { 0U };
			std::uint8_t* m_BitsDynamic;
		};
		Clock::time_point m_Time {};
	};

	struct WritePacketInfo
	{
	public:
		EPacketHeaderType    m_Type { EPacketHeaderType::Normal };
		std::uint16_t        m_ID { 0U };
		std::uint32_t        m_Index : 20 { 0U };
		std::uint32_t        m_Rev : 12 { 0U };
		std::uint32_t        m_Start { ~0U };
		std::uint32_t        m_Size { 0U };
		Networking::Endpoint m_Endpoint;
		bool                 m_Ready { false };
		union
		{
			std::uint32_t m_Bits { 0U };
			std::uint8_t* m_BitsDynamic;
		};
		Clock::time_point m_Time {};
	};

	struct PacketHandler
	{
	public:
		using HandleCallback = void (*)(ReliableUDP::PacketHandler* handler, ReliableUDP::Networking::Endpoint endpoint, std::uint8_t* packet, std::uint32_t size);

	public:
		// ESP32 example: 45056, 45056, 64, 64, 8
		PacketHandler(std::uint32_t readBufferSize, std::uint32_t writeBufferSize, std::uint32_t maxReadPackets, std::uint32_t maxWritePackets, std::uint32_t sendCount, HandleCallback handleCallback, void* userData);
		~PacketHandler();

		void updatePackets();

		std::uint32_t availableReadPackets() const;
		std::uint32_t availableWritePackets() const;
		std::uint32_t availableReadPacketSize() const;
		std::uint32_t availableWritePacketSize() const;

		std::uint8_t* getReadPacket(std::uint16_t id, std::uint32_t& size);
		std::uint8_t* getWritePacket(std::uint16_t id, std::uint32_t& size);
		void          markWritePacketReady(std::uint16_t id);
		void          setPacketEndpoint(std::uint16_t id, Networking::Endpoint endpoint);
		void          freeReadPacket(std::uint16_t id);
		void          freeWritePacket(std::uint16_t id);

		[[nodiscard]] std::uint8_t* allocateReadPacket(std::uint32_t size, std::uint16_t id, std::uint16_t rev, Networking::Endpoint endpoint);
		[[nodiscard]] std::uint8_t* allocateWritePacket(std::uint32_t size, std::uint16_t& id);

		void acknowledgePacket(Networking::Endpoint endpoint, std::uint16_t id, std::uint16_t index, std::uint16_t rev);
		void rejectPacket(Networking::Endpoint endpoint, std::uint16_t id, std::uint16_t rev);
		void sendMaxSizePacket(Networking::Endpoint endpoint, std::uint16_t id);

		bool hasHandledSection(std::uint16_t id, std::uint32_t index, std::uint16_t rev);
		bool receivedSection(std::uint16_t id, std::uint32_t index, std::uint16_t rev);
		bool readPacketDone(std::uint16_t id);

		std::uint16_t newPacketID();
		bool          hasUsedPacketID(std::uint16_t id) const;
		bool          hasHandledPacketID(std::uint16_t id) const;

		std::uint32_t getRequiredSections(std::uint32_t size) const;

		auto& getSocket() { return m_Socket; }
		auto& getSocket() const { return m_Socket; }
		auto  getReadBufferSize() const { return m_ReadBufferSize; }
		auto  getWriteBufferSize() const { return m_WriteBufferSize; }
		auto  getReadBuffer() const { return m_ReadBuffer; }
		auto  getWriteBuffer() const { return m_WriteBuffer; }
		auto  getMaxReadPackets() const { return m_MaxReadPackets; }
		auto  getReadPacketInfos() const { return m_ReadPacketInfos; }
		auto  getMaxWritePackets() const { return m_MaxWritePackets; }
		auto  getWritePacketInfos() const { return m_WritePacketInfos; }
		auto& getUsedPacketIDs() const { return m_UsedPacketIDs; }
		auto& getHandledPacketIDs() const { return m_HandledPacketIDs; }
		auto  getHandleCallback() const { return m_HandleCallback; }
		auto  getUserData() const { return m_UserData; }

	private:
		Networking::Socket m_Socket;

		std::uint32_t m_ReadBufferSize;
		std::uint32_t m_WriteBufferSize;
		std::uint32_t m_UsedReadBufferSize;
		std::uint32_t m_UsedWriteBufferSize;
		std::uint8_t* m_ReadBuffer;
		std::uint8_t* m_WriteBuffer;

		std::uint32_t    m_MaxReadPackets;
		std::uint32_t    m_MaxWritePackets;
		ReadPacketInfo*  m_ReadPacketInfos;
		WritePacketInfo* m_WritePacketInfos;
		std::uint32_t    m_SendPacket { 0U };
		std::uint32_t    m_SendIndex { 0U };
		std::uint32_t    m_ToSend;
		std::uint32_t    m_SendCount;

		Utils::RotatingArray<std::uint16_t, 16U> m_UsedPacketIDs;
		Utils::RotatingArray<std::uint16_t, 16U> m_HandledPacketIDs;

		HandleCallback m_HandleCallback;
		void*          m_UserData;

		Clock::time_point m_LastSendout;
		float             m_ReadTimeout { 2.0f };
		float             m_WriteTimeout { 2.0f };
		float             m_SendoutTimer { 0.1f };
	};
} // namespace ReliableUDP