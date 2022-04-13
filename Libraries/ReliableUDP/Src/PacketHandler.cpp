#include "ReliableUDP/PacketHandler.h"

#include <cstdlib>

namespace ReliableUDP
{
	PacketHandler::PacketHandler(std::uint32_t readBufferSize, std::uint32_t writeBufferSize, std::uint32_t maxReadPackets, std::uint32_t maxWritePackets, HandleCallback handleCallback, void* userData)
	    : m_Socket(Networking::ESocketType::UDP),
	      m_ReadBufferSize(readBufferSize + 4096U),
	      m_WriteBufferSize(writeBufferSize + 4096U),
	      m_UsedReadBufferSize(4096U),
	      m_UsedWriteBufferSize(4096U),
	      m_ReadBuffer(new std::uint8_t[m_ReadBufferSize]),
	      m_WriteBuffer(new std::uint8_t[m_WriteBufferSize]),
	      m_MaxReadPackets(maxReadPackets),
	      m_MaxWritePackets(maxWritePackets),
	      m_ReadPacketInfos(new ReadPacketInfo[m_MaxReadPackets]),
	      m_WritePacketInfos(new WritePacketInfo[m_MaxWritePackets]),
	      m_HandleCallback(handleCallback),
	      m_UserData(userData)
	{
	}

	PacketHandler::~PacketHandler()
	{
		if (m_ReadBuffer)
			delete[] m_ReadBuffer;
		if (m_WriteBuffer)
			delete[] m_WriteBuffer;
		if (m_ReadPacketInfos)
			delete[] m_ReadPacketInfos;
		if (m_WritePacketInfos)
			delete[] m_WritePacketInfos;
		m_ReadBufferSize   = 0U;
		m_WriteBufferSize  = 0U;
		m_MaxReadPackets   = 0U;
		m_MaxWritePackets  = 0U;
		m_ReadBuffer       = nullptr;
		m_WriteBuffer      = nullptr;
		m_ReadPacketInfos  = nullptr;
		m_WritePacketInfos = nullptr;
	}

	void PacketHandler::updatePackets()
	{
		Networking::Endpoint endpoint {};
		std::size_t          size { 0U };
		while (size = m_Socket.readFrom(m_ReadBuffer, 4096U, endpoint))
		{
			if (size < 8)
				continue;

			auto header { reinterpret_cast<PacketHeader*>(m_ReadBuffer) };
			if (!IsMagicNumberValid(header->m_MagicNumber))
				continue;

			if (hasHandledPacketID(header->m_ID))
				continue;

			auto type { GetPacketHeaderType(header->m_MagicNumber) };
			switch (type)
			{
			case EPacketHeaderType::Normal:
			{
				std::size_t i { 0U };
				for (; i < m_MaxReadPackets; ++i)
					if (m_ReadPacketInfos[i].m_ID == header->m_ID)
						break;

				if (i == m_MaxReadPackets)
				{
					if (header->m_Size > availableReadPacketSize())
					{
						rejectPacket(endpoint, header->m_ID, header->m_Rev);
						continue;
					}

					if (!allocateReadPacket(header->m_Size, header->m_ID, header->m_Rev, endpoint))
					{
						rejectPacket(endpoint, header->m_ID, header->m_Rev);
						continue;
					}
				}
				else
				{
					if (hasHandledSection(header->m_ID, header->m_Index, header->m_Rev))
						continue;
				}

				if (!receivedSection(header->m_ID, header->m_Index, header->m_Rev))
				{
					rejectPacket(endpoint, header->m_ID, header->m_Rev);
					continue;
				}
				acknowledgePacket(endpoint, header->m_ID, header->m_Index, header->m_Rev);
				if (readPacketDone(header->m_ID))
				{
					m_HandledPacketIDs.insert(header->m_ID);
					if (m_HandleCallback)
					{
						std::uint32_t packetSize { 0U };
						std::uint8_t* ptr { getReadPacket(header->m_ID, packetSize) };
						m_HandleCallback(reinterpret_cast<PacketHeader*>(ptr), packetSize);
					}
					freeReadPacket(header->m_ID);
				}
				break;
			}
			}
		}
	}

	std::uint32_t PacketHandler::availableReadPackets() const
	{
		std::uint32_t usedPacketInfos { 0U };
		for (std::uint32_t i { 0 }; i < m_MaxReadPackets; ++i)
		{
			ReadPacketInfo& info = m_ReadPacketInfos[i];
			if (info.m_ID)
				++usedPacketInfos;
		}
		return m_MaxReadPackets - usedPacketInfos;
	}

	std::uint32_t PacketHandler::availableWritePackets() const
	{
		std::uint32_t usedPacketInfos { 0U };
		for (std::uint32_t i { 0 }; i < m_MaxWritePackets; ++i)
		{
			WritePacketInfo& info = m_WritePacketInfos[i];
			if (info.m_ID)
				++usedPacketInfos;
		}
		return m_MaxWritePackets - usedPacketInfos;
	}

	std::uint32_t PacketHandler::availableReadPacketSize() const
	{
		return m_ReadBufferSize - m_UsedReadBufferSize;
	}

	std::uint32_t PacketHandler::availableWritePacketSize() const
	{
		return m_WriteBufferSize - m_UsedWriteBufferSize;
	}

	std::uint8_t* PacketHandler::getReadPacket(std::uint16_t id, std::uint32_t& size)
	{
		if (!id)
			return nullptr;

		for (std::uint32_t i { 0 }; i < m_MaxReadPackets; ++i)
		{
			ReadPacketInfo& info = m_ReadPacketInfos[i];
			if (info.m_ID != id || !info.m_Size)
				continue;

			size = info.m_Size;
			return m_ReadBuffer + info.m_Start;
		}
		return nullptr;
	}

	std::uint8_t* PacketHandler::getWritePacket(std::uint16_t id, std::uint32_t& size)
	{
		if (!id)
			return nullptr;

		for (std::uint32_t i { 0 }; i < m_MaxWritePackets; ++i)
		{
			WritePacketInfo& info = m_WritePacketInfos[i];
			if (info.m_ID != id || !info.m_Size)
				continue;

			size = info.m_Size;
			return m_WriteBuffer + info.m_Start;
		}
		return nullptr;
	}

	void PacketHandler::markWritePacketReady(std::uint16_t id)
	{
		if (!id)
			return;

		for (std::uint32_t i { 0 }; i < m_MaxWritePackets; ++i)
		{
			WritePacketInfo& info = m_WritePacketInfos[i];
			if (info.m_ID == id)
			{
				info.m_Ready = true;
				return;
			}
		}
	}

	void PacketHandler::setPacketEndpoint(std::uint16_t id, Networking::Endpoint endpoint)
	{
		if (!id)
			return;

		for (std::uint32_t i { 0 }; i < m_MaxWritePackets; ++i)
		{
			WritePacketInfo& info = m_WritePacketInfos[i];
			if (info.m_ID == id)
			{
				info.m_Endpoint = endpoint;
				return;
			}
		}
	}

	void PacketHandler::freeReadPacket(std::uint16_t id)
	{
		if (!id)
			return;

		std::uint32_t i { 0U };
		for (; i < m_MaxReadPackets; ++i)
			if (m_ReadPacketInfos[i].m_ID == id)
				break;

		if (i == m_MaxReadPackets)
			return;

		ReadPacketInfo& info = m_ReadPacketInfos[i];

		std::uint32_t start = info.m_Start;
		std::uint32_t size  = info.m_Size;
		std::memmove(m_ReadBuffer + start, m_ReadBuffer + start + size, m_ReadBufferSize - start - size);
		for (std::uint32_t j = 0; j < m_MaxReadPackets; ++j)
		{
			ReadPacketInfo& info = m_ReadPacketInfos[j];
			if (info.m_Start > start)
				info.m_Start -= size;
		}

		if (info.m_Size > 32 * (4096 - sizeof(PacketHeader)))
			delete[] info.m_BitsDynamic;
		info.m_ID       = 0U;
		info.m_Rev      = 0U;
		info.m_Start    = ~0U;
		info.m_Size     = 0U;
		info.m_Endpoint = {};
		info.m_Bits     = 0U;
		info.m_Time     = {};
	}

	void PacketHandler::freeWritePacket(std::uint16_t id)
	{
		if (!id)
			return;

		std::uint32_t i { 0U };
		for (; i < m_MaxWritePackets; ++i)
			if (m_WritePacketInfos[i].m_ID == id)
				break;

		if (i == m_MaxWritePackets)
			return;

		WritePacketInfo& info = m_WritePacketInfos[i];

		std::uint32_t start = info.m_Start;
		std::uint32_t size  = info.m_Size;
		std::memmove(m_WriteBuffer + start, m_WriteBuffer + start + size, m_WriteBufferSize - start - size);
		for (std::uint32_t j = 0; j < m_MaxWritePackets; ++j)
		{
			WritePacketInfo& info = m_WritePacketInfos[j];
			if (info.m_Start > start)
				info.m_Start -= size;
		}

		if (info.m_Size > 32 * (4096 - sizeof(PacketHeader)))
			delete[] info.m_BitsDynamic;
		info.m_Type     = EPacketHeaderType::Normal;
		info.m_ID       = 0U;
		info.m_Index    = 0U;
		info.m_Rev      = 0U;
		info.m_Start    = ~0U;
		info.m_Size     = 0U;
		info.m_Endpoint = {};
		info.m_Ready    = false;
		info.m_Bits     = 0U;
		info.m_Time     = {};
	}

	std::uint8_t* PacketHandler::allocateReadPacket(std::uint32_t size, std::uint16_t id, std::uint16_t rev, Networking::Endpoint endpoint)
	{
		if (!availableReadPackets())
		{
			id = 0U;
			return nullptr;
		}

		std::uint32_t availableSize = availableReadPacketSize();
		if (size > availableSize)
		{
			id = 0U;
			return nullptr;
		}

		std::uint32_t start = m_ReadBufferSize - availableSize;
		std::uint8_t* ptr   = m_ReadBuffer + start;
		m_UsedReadBufferSize += size;

		std::uint32_t i = 0;
		for (; i < m_MaxReadPackets; ++i)
			if (!m_ReadPacketInfos[i].m_ID)
				break;

		ReadPacketInfo& info { m_ReadPacketInfos[i] };
		info.m_ID       = id;
		info.m_Rev      = rev;
		info.m_Start    = start;
		info.m_Size     = size;
		info.m_Endpoint = endpoint;
		if (size > 32 * (4096 - sizeof(PacketHeader)))
		{
			std::uint32_t totalSections = getRequiredSections(size);
			std::uint32_t numBytes      = (totalSections + 7) / 8;
			info.m_BitsDynamic          = new std::uint8_t[numBytes] { 0U };
			for (std::uint32_t i = 0; i < (numBytes * 8) - totalSections; ++i)
				info.m_BitsDynamic[numBytes - 1] |= 1 << i;
		}
		else
		{
			info.m_Bits                 = 0U;
			std::uint32_t totalSections = getRequiredSections(size);
			for (std::uint32_t i = totalSections; i < 32; ++i)
				info.m_Bits |= 1 << i;
		}
		info.m_Time = {};
		return ptr;
	}

	std::uint8_t* PacketHandler::allocateWritePacket(std::uint32_t size, std::uint16_t& id)
	{
		if (!availableWritePackets())
		{
			id = 0U;
			return nullptr;
		}

		std::uint32_t availableSize = availableWritePacketSize();
		if (size > availableSize)
		{
			id = 0U;
			return nullptr;
		}

		id                  = newPacketID();
		std::uint32_t start = m_WriteBufferSize - availableSize;
		std::uint8_t* ptr   = m_WriteBuffer + start;
		m_UsedWriteBufferSize += size;

		std::uint32_t i = 0;
		for (; i < m_MaxWritePackets; ++i)
			if (!m_WritePacketInfos[i].m_ID)
				break;

		WritePacketInfo& info { m_WritePacketInfos[i] };
		info.m_Type     = EPacketHeaderType::Normal;
		info.m_ID       = id;
		info.m_Rev      = 0U;
		info.m_Start    = start;
		info.m_Size     = size;
		info.m_Endpoint = {};
		info.m_Ready    = false;
		info.m_Time     = {};
		return ptr;
	}

	void PacketHandler::acknowledgePacket(Networking::Endpoint endpoint, std::uint16_t id, std::uint16_t index, std::uint16_t rev)
	{
		if (availableWritePackets())
		{
			std::uint32_t i = 0;
			for (; i < m_MaxWritePackets; ++i)
				if (!m_WritePacketInfos[i].m_ID)
					break;

			WritePacketInfo& info { m_WritePacketInfos[i] };
			info.m_Type     = EPacketHeaderType::Acknowledge;
			info.m_ID       = id;
			info.m_Index    = index;
			info.m_Rev      = rev;
			info.m_Endpoint = endpoint;
			info.m_Ready    = true;
			info.m_Bits     = 0U;
			info.m_Time     = {};
		}
		else
		{
			auto header     = reinterpret_cast<AcknowledgePacketHeader*>(m_WriteBuffer);
			*header         = {};
			header->m_ID    = id;
			header->m_Index = index;
			header->m_Rev   = rev;
			m_Socket.writeTo(m_WriteBuffer, sizeof(AcknowledgePacketHeader), endpoint);
		}
	}

	void PacketHandler::rejectPacket(Networking::Endpoint endpoint, std::uint16_t id, std::uint16_t rev)
	{
		if (availableWritePackets())
		{
			std::uint32_t i = 0;
			for (; i < m_MaxWritePackets; ++i)
				if (!m_WritePacketInfos[i].m_ID)
					break;

			WritePacketInfo& info { m_WritePacketInfos[i] };
			info.m_Type     = EPacketHeaderType::Reject;
			info.m_ID       = id;
			info.m_Index    = 0U;
			info.m_Rev      = rev;
			info.m_Endpoint = endpoint;
			info.m_Ready    = true;
			info.m_Bits     = 0U;
			info.m_Time     = {};
		}
		else
		{
			auto header     = reinterpret_cast<RejectPacketHeader*>(m_WriteBuffer);
			*header         = {};
			header->m_ID    = id;
			header->m_Index = 0U;
			header->m_Rev   = rev;
			m_Socket.writeTo(m_WriteBuffer, sizeof(RejectPacketHeader), endpoint);
		}
	}

	void PacketHandler::sendMaxSizePacket(Networking::Endpoint endpoint, std::uint16_t id)
	{
		if (availableWritePackets())
		{
			std::uint32_t i = 0;
			for (; i < m_MaxWritePackets; ++i)
				if (!m_WritePacketInfos[i].m_ID)
					break;

			WritePacketInfo& info { m_WritePacketInfos[i] };
			info.m_Type     = EPacketHeaderType::MaxSize;
			info.m_ID       = id;
			info.m_Index    = 0U;
			info.m_Rev      = 0U;
			info.m_Endpoint = endpoint;
			info.m_Ready    = true;
			info.m_Bits     = 0U;
			info.m_Time     = {};
		}
		else
		{
			auto header    = reinterpret_cast<MaxSizePacketHeader*>(m_WriteBuffer);
			*header        = {};
			header->m_ID   = id;
			header->m_Size = m_ReadBufferSize - 4096U;
			m_Socket.writeTo(m_WriteBuffer, sizeof(MaxSizePacketHeader), endpoint);
		}
	}

	bool PacketHandler::hasHandledSection(std::uint16_t id, std::uint16_t index, std::uint16_t rev)
	{
		if (!id)
			return true;

		std::uint32_t i { 0U };
		for (; i < m_MaxReadPackets; ++i)
			if (m_ReadPacketInfos[i].m_ID == id)
				break;

		if (i == m_MaxReadPackets)
			return true;

		ReadPacketInfo& info = m_ReadPacketInfos[i];
		if (rev < info.m_Rev)
			return true;

		if (info.m_Size > 32 * (4096 - sizeof(PacketHeader)))
		{
			if (index >= getRequiredSections(info.m_Size))
				return true;

			std::uint32_t bit  = index % 8;
			std::uint32_t byte = index / 8;

			return (info.m_BitsDynamic[byte] >> bit) & 1;
		}
		else
		{
			return (info.m_Bits >> index) & 1;
		}
	}

	bool PacketHandler::receivedSection(std::uint16_t id, std::uint16_t index, std::uint16_t rev)
	{
		if (!id)
			return false;

		std::uint32_t i { 0U };
		for (; i < m_MaxReadPackets; ++i)
			if (m_ReadPacketInfos[i].m_ID == id)
				break;

		if (i == m_MaxReadPackets)
			return false;

		ReadPacketInfo& info = m_ReadPacketInfos[i];
		if (rev < info.m_Rev)
			return true;

		if (rev > info.m_Rev)
		{
			std::uint32_t start = info.m_Start;
			std::uint32_t size  = info.m_Size;
			std::memmove(m_ReadBuffer + start, m_ReadBuffer + start + size, m_ReadBufferSize - start - size);
			for (std::uint32_t j = 0; j < m_MaxReadPackets; ++j)
			{
				ReadPacketInfo& info = m_ReadPacketInfos[j];
				if (info.m_Start > start)
					info.m_Start -= size;
			}

			std::uint32_t availableSize = availableReadPacketSize();
			if (size > availableSize)
			{
				if (info.m_Size > 32 * (4096 - sizeof(PacketHeader)))
					delete[] info.m_BitsDynamic;
				info.m_ID       = 0U;
				info.m_Rev      = 0U;
				info.m_Start    = ~0U;
				info.m_Size     = 0U;
				info.m_Endpoint = {};
				info.m_Bits     = 0U;
				info.m_Time     = {};
				return false;
			}

			info.m_Start = m_ReadBufferSize - availableSize;
			m_UsedReadBufferSize += size;
			info.m_Rev = rev;

			if (info.m_Size > 32 * (4096 - sizeof(PacketHeader)))
			{
				for (std::uint32_t j = 0; j < getRequiredSections(info.m_Size); ++j)
					info.m_BitsDynamic[j] = 0U;
			}
			else
			{
				info.m_Bits = 0U;
			}
		}

		if (info.m_Size > 32 * (4096 - sizeof(PacketHeader)))
		{
			if (index >= getRequiredSections(info.m_Size))
				return true;

			std::uint32_t bit  = index % 8;
			std::uint32_t byte = index / 8;

			info.m_BitsDynamic[byte] |= 1 << bit;
		}
		else
		{
			info.m_Bits |= 1 << index;
		}

		std::memcpy(m_ReadBuffer + info.m_Start, m_ReadBuffer + sizeof(PacketHeader), std::min(4096 - sizeof(PacketHeader), info.m_Size - index * (4096 - sizeof(PacketHeader))));

		return true;
	}

	bool PacketHandler::readPacketDone(std::uint16_t id)
	{
		if (!id)
			return false;

		std::uint32_t i { 0U };
		for (; i < m_MaxReadPackets; ++i)
			if (m_ReadPacketInfos[i].m_ID == id)
				break;

		if (i == m_MaxReadPackets)
			return false;

		ReadPacketInfo& info = m_ReadPacketInfos[i];
		if (info.m_Size > 32 * (4096 - sizeof(PacketHeader)))
		{
			for (std::uint32_t byte = 0; byte < (getRequiredSections(info.m_Size) + 7) / 8; ++byte)
				if (info.m_BitsDynamic[byte] != ~0U)
					return false;
			return true;
		}
		else
		{
			return info.m_Bits == ~0U;
		}
	}

	std::uint16_t PacketHandler::newPacketID()
	{
		std::uint16_t id = 0U;
		while (hasUsedPacketID(id = static_cast<std::uint16_t>(rand()) | 1))
			;
		return id;
	}

	bool PacketHandler::hasUsedPacketID(std::uint16_t id) const
	{
		if (!id)
			return true;

		for (std::uint32_t i { 0 }; i < m_MaxWritePackets; ++i)
			if (m_WritePacketInfos[i].m_ID == id)
				return true;

		for (std::uint32_t i { 0 }; i < m_UsedPacketIDs.size(); ++i)
			if (m_UsedPacketIDs[i] == id)
				return true;

		return false;
	}

	bool PacketHandler::hasHandledPacketID(std::uint16_t id) const
	{
		for (std::uint32_t i { 0 }; i < m_HandledPacketIDs.size(); ++i)
			if (m_HandledPacketIDs[i] == id)
				return true;

		return false;
	}

	std::uint32_t PacketHandler::getRequiredSections(std::uint32_t size) const
	{
		static constexpr std::uint32_t SectionSize = 4096U - sizeof(PacketHeader);
		return (size + SectionSize - 1U) / SectionSize;
	}
} // namespace ReliableUDP