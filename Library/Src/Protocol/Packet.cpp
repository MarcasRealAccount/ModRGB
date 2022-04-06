#include "ModRGB/Protocol/Packet.h"

namespace ModRGB::Protocol
{
	PacketHeader PacketHeader::ReadFrom(Utils::Buffer& buffer)
	{
		PacketHeader header;
		buffer.paste(&header, sizeof(PacketHeader));
		return header;
	}

	void PacketHeader::writeTo(Utils::Buffer& buffer)
	{
		buffer.copy(this, sizeof(PacketHeader));
	}

	void ContinuationPacket::writeTo(Utils::Buffer& buffer)
	{
		PacketHeader::writeTo(buffer);
		buffer.copy(reinterpret_cast<std::uint8_t*>(this) + sizeof(PacketHeader), sizeof(ContinuationPacket) - sizeof(PacketHeader));
	}

	void AckContinuationPacket::writeTo(Utils::Buffer& buffer)
	{
		PacketHeader::writeTo(buffer);
		buffer.copy(reinterpret_cast<std::uint8_t*>(this) + sizeof(PacketHeader), sizeof(AckContinuationPacket) - sizeof(PacketHeader));
	}

	Packet::Packet(std::unique_ptr<PacketHeader>&& data, Networking::IPv4Endpoint endpoint)
	    : m_Data(std::move(data)), m_ClientBased(false), m_Endpoint(endpoint), m_Time(Clock::now()) {}

	Packet::Packet(std::unique_ptr<PacketHeader>&& data, UUID uuid)
	    : m_Data(std::move(data)), m_ClientBased(true), m_UUID(uuid), m_Time(Clock::now()) {}

	Packet::Packet(Packet&& move) noexcept
	    : m_Data(std::move(move.m_Data)), m_ClientBased(move.m_ClientBased), m_Time(move.m_Time)
	{
		move.m_ClientBased = false;
		if (m_ClientBased)
		{
			m_UUID      = move.m_UUID;
			move.m_UUID = 0U;
		}
		else
		{
			m_Endpoint      = move.m_Endpoint;
			move.m_Endpoint = {};
		}
	}

	ContinuedPacket::ContinuedPacket(std::unique_ptr<PacketHeader>&& data, Networking::IPv4Endpoint endpoint)
	    : m_Packets(1 + (data->m_Size - 65507) / sizeof(ContinuationPacket::m_Data)), m_ClientBased(false), m_Endpoint(endpoint), m_Time(Clock::now())
	{
		for (std::uint32_t i = 1; i < m_Packets.size(); ++i)
		{
			auto packet = std::make_unique<ContinuationPacket>(data->m_PacketId, data->m_UUID, data->m_Size, i);

			Utils::Buffer buffer { 65507 + sizeof(ContinuationPacket::m_Data) * (i - 1), packet->m_Data, sizeof(ContinuationPacket::m_Data) };
			data->writeTo(buffer);

			m_Packets[i] = std::move(packet);
		}
		m_Packets[0] = std::move(data);
	}

	ContinuedPacket::ContinuedPacket(std::unique_ptr<PacketHeader>&& data, UUID uuid)
	    : m_Packets(1 + (data->m_Size - 65507) / sizeof(ContinuationPacket::m_Data)), m_ClientBased(true), m_UUID(uuid), m_Time(Clock::now())
	{
		for (std::uint32_t i = 1; i < m_Packets.size(); ++i)
		{
			auto packet = std::make_unique<ContinuationPacket>(data->m_PacketId, data->m_UUID, data->m_Size, i);

			Utils::Buffer buffer { 65507 + sizeof(ContinuationPacket::m_Data) * (i - 1), packet->m_Data, sizeof(ContinuationPacket::m_Data) };
			data->writeTo(buffer);

			m_Packets[i] = std::move(packet);
		}
		m_Packets[0] = std::move(data);
	}

	ContinuedPacket::ContinuedPacket(ContinuedPacket&& move) noexcept
	    : m_Packets(std::move(move.m_Packets)), m_ClientBased(move.m_ClientBased), m_Time(move.m_Time)
	{
		move.m_ClientBased = false;
		if (m_ClientBased)
		{
			m_UUID      = move.m_UUID;
			move.m_UUID = 0U;
		}
		else
		{
			m_Endpoint      = move.m_Endpoint;
			move.m_Endpoint = {};
		}
	}

	ContinuedPacketArrive::ContinuedPacketArrive(ContinuedPacketArrive&& move)
	    : m_Data(std::move(move.m_Data)), m_ArrivedPackets(std::move(move.m_ArrivedPackets)), m_PacketsArrived(move.m_PacketsArrived), m_PacketsToArrive(move.m_PacketsToArrive), m_Endpoint(move.m_Endpoint) {}

	void ContinuedPacketArrive::writeContinuation(std::uint32_t index, std::uint8_t* buffer, std::size_t size)
	{
		if (index == 0)
			std::memcpy(m_Data.data(), buffer, size);
		else
			std::memcpy(m_Data.data() + 65507 + sizeof(ContinuationPacket::m_Data) * (index - 1), buffer, size);

		m_ArrivedPackets[index] = true;
		++m_PacketsArrived;
	}

	PacketHandler::PacketHandler()
	    : m_Buffer(new std::uint8_t[65507]), m_UsedPacketIDs(32, 0U), m_HandledPacketIDs(32, 0U), m_PacketRNG(std::random_device {}()) {}

	PacketHandler::PacketHandler(PacketHandler&& move)
	    : m_Buffer(move.m_Buffer), m_Packets(std::move(move.m_Packets)), m_UsedPacketIDs(std::move(move.m_UsedPacketIDs)), m_HandledPacketIDs(std::move(move.m_HandledPacketIDs)), m_UsedPacketIDsIndex(move.m_UsedPacketIDsIndex), m_HandledPacketIDsIndex(move.m_HandledPacketIDsIndex), m_PacketRNG(std::move(move.m_PacketRNG)), m_PreviousPacketSendout(move.m_PreviousPacketSendout), m_PacketTimeout(move.m_PacketTimeout)
	{
		move.m_Buffer = nullptr;
	}

	PacketHandler::~PacketHandler()
	{
		delete[] m_Buffer;
	}

	void PacketHandler::updatePackets()
	{
		auto& socket = getSocket();

		Networking::IPv4Endpoint endpoint;

		std::size_t readCount = 0;
		while ((readCount = socket.readFrom(m_Buffer, 65507, endpoint)))
		{
			if (readCount < sizeof(PacketHeader))
				continue;

			PacketHeader& header = *reinterpret_cast<PacketHeader*>(m_Buffer);
			if (header.m_Magic != s_MagicNumber)
				continue;

			if (isPacketHandled(header.m_PacketId))
				continue;

			if (header.m_Size > 65507)
			{
				auto itr = m_ContinuedPacketArrives.find(header.m_PacketId);
				if (itr == m_ContinuedPacketArrives.end())
					itr = m_ContinuedPacketArrives.insert({ header.m_PacketId, ContinuedPacketArrive(header.m_Size, endpoint) }).first;

				ContinuedPacketArrive& continued = itr->second;

				if (header.m_Type == SharedPacketTypes::s_Continuation)
				{
					ContinuationPacket& continuationPacket = *reinterpret_cast<ContinuationPacket*>(m_Buffer);

					if (continued.m_ArrivedPackets[continuationPacket.m_Index])
						continue;

					continued.writeContinuation(continuationPacket.m_Index, continuationPacket.m_Data, sizeof(ContinuationPacket::m_Data));
					sendPacket(header.m_UUID, std::make_unique<AckContinuationPacket>(header.m_PacketId, header.m_UUID, continuationPacket.m_Index));
				}
				else
				{
					if (continued.m_ArrivedPackets[0])
						continue;

					continued.writeContinuation(0, m_Buffer, 65507);
					sendPacket(header.m_UUID, std::make_unique<AckContinuationPacket>(header.m_PacketId, header.m_UUID, 0));
				}

				if (continued.m_PacketsArrived == continued.m_PacketsToArrive)
				{
					Utils::Buffer buffer { 0, continued.m_Data.data(), continued.m_Data.size() };
					handle(buffer, continued.m_Endpoint);
					m_ContinuedPacketArrives.erase(itr);

					m_HandledPacketIDs[m_HandledPacketIDsIndex] = header.m_PacketId;

					m_HandledPacketIDsIndex = (m_HandledPacketIDsIndex + 1) % m_HandledPacketIDs.size();
				}
			}
			else
			{
				Utils::Buffer buffer { 0, m_Buffer, header.m_Size };
				handle(buffer, endpoint);

				m_HandledPacketIDs[m_HandledPacketIDsIndex] = header.m_PacketId;

				m_HandledPacketIDsIndex = (m_HandledPacketIDsIndex + 1) % m_HandledPacketIDs.size();
			}
		}

		auto   timeNow   = Clock::now();
		double deltaTime = std::chrono::duration_cast<std::chrono::duration<double, std::milli>>(timeNow - m_PreviousPacketSendout).count();
		if (deltaTime >= 250.0)
		{
			std::remove_if(m_Packets.begin(), m_Packets.end(),
			               [timeNow, this](Packet& packet) -> bool
			               {
				               return std::chrono::duration_cast<std::chrono::duration<double, std::milli>>(timeNow - packet.m_Time).count() >= m_PacketTimeout || (packet.m_ClientBased && !clientExists(packet.m_UUID));
			               });

			std::remove_if(m_ContinuedPackets.begin(), m_ContinuedPackets.end(),
			               [timeNow, this](std::pair<std::uint32_t, ContinuedPacket>& packet) -> bool
			               {
				               return std::chrono::duration_cast<std::chrono::duration<double, std::milli>>(timeNow - packet.second.m_Time).count() >= m_PacketTimeout || (packet.second.m_ClientBased && !clientExists(packet.second.m_UUID));
			               });

			for (auto& packet : m_Packets)
				send(packet);

			for (auto& packet : m_ContinuedPackets)
				send(packet.second);

			m_PreviousPacketSendout = timeNow;
		}
	}

	void PacketHandler::sendPacket(Networking::IPv4Endpoint endpoint, std::unique_ptr<PacketHeader>&& packet)
	{
		if (packet->m_PacketId == 0U)
			packet->m_PacketId = newPacketId();

		if (packet->m_Size > 65507)
			m_ContinuedPackets.insert({ packet->m_PacketId, ContinuedPacket { std::move(packet), endpoint } });
		else
			m_Packets.emplace_back(std::move(packet), endpoint);
	}

	void PacketHandler::sendPacket(UUID uuid, std::unique_ptr<PacketHeader>&& packet)
	{
		if (packet->m_PacketId == 0U)
			packet->m_PacketId = newPacketId();

		if (packet->m_Size > 65507)
			m_ContinuedPackets.insert({ packet->m_PacketId, ContinuedPacket { std::move(packet), uuid } });
		else
			m_Packets.emplace_back(std::move(packet), uuid);
	}

	void PacketHandler::setPacketTimeout(double packetTimeout)
	{
		m_PacketTimeout = packetTimeout;
	}

	void PacketHandler::handle(Utils::Buffer& buffer, Networking::IPv4Endpoint endpoint)
	{
		PacketHeader& header = *reinterpret_cast<PacketHeader*>(buffer.getBuffer());

		switch (header.m_Type)
		{
		case SharedPacketTypes::s_Ack:
		{
			auto itr = std::find(m_Packets.begin(), m_Packets.end(),
			                     [header](Packet& packet) -> bool
			                     {
				                     return packet.m_Data->m_PacketId == header.m_PacketId;
			                     });
			if (itr == m_Packets.end())
				m_ContinuedPackets.erase(header.m_PacketId);
			else
				m_Packets.erase(itr);
			break;
		}
		case SharedPacketTypes::s_AckContinuation:
		{
			auto itr = m_ContinuedPackets.find(header.m_PacketId);
			if (itr != m_ContinuedPackets.end())
			{
				AckContinuationPacket& continuation         = *reinterpret_cast<AckContinuationPacket*>(buffer.getBuffer());
				itr->second.m_Packets[continuation.m_Index] = nullptr;
			}
			break;
		}
		default:
			if (handlePacket(buffer, endpoint))
				sendPacket(header.m_UUID, std::make_unique<PacketHeader>(header.m_PacketId, SharedPacketTypes::s_Ack, header.m_UUID));
			break;
		}
	}

	void PacketHandler::send(Packet& packet)
	{
		auto& socket = getSocket();

		Utils::Buffer buffer { 0, m_Buffer, 65507 };
		packet.m_Data->writeTo(buffer);
		socket.writeTo(m_Buffer, buffer.getOffset(), packet.m_ClientBased ? getClientEndpoint(packet.m_UUID) : packet.m_Endpoint);
	}

	void PacketHandler::send(ContinuedPacket& packet)
	{
		auto& socket = getSocket();

		auto endpoint = packet.m_ClientBased ? getClientEndpoint(packet.m_UUID) : packet.m_Endpoint;
		for (auto& pck : packet.m_Packets)
		{
			if (pck)
			{
				Utils::Buffer buffer { 0, m_Buffer, 65507 };
				pck->writeTo(buffer);
				socket.writeTo(m_Buffer, buffer.getOffset(), endpoint);
			}
		}
	}

	std::uint32_t PacketHandler::newPacketId()
	{
		std::uint32_t id = m_PacketRNG();
		while (isPacketIdUsed(id))
			id = m_PacketRNG();
		m_UsedPacketIDs[m_UsedPacketIDsIndex] = id;

		m_UsedPacketIDsIndex = (m_UsedPacketIDsIndex + 1) % m_UsedPacketIDs.size();
		return id;
	}

	bool PacketHandler::isPacketIdUsed(std::uint32_t id) const
	{
		return std::find(m_UsedPacketIDs.begin(), m_UsedPacketIDs.end(), id) != m_UsedPacketIDs.end();
	}

	bool PacketHandler::isPacketHandled(std::uint32_t id) const
	{
		return std::find(m_HandledPacketIDs.begin(), m_HandledPacketIDs.end(), id) != m_HandledPacketIDs.end();
	}
} // namespace ModRGB::Protocol