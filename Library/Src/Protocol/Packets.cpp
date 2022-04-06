#include "ModRGB/Protocol/Packets.h"

namespace ModRGB::Protocol::Packets
{
	StripPacket StripPacket::ReadFrom(Utils::Buffer& buffer)
	{
		PacketHeader header = PacketHeader::ReadFrom(buffer);

		std::vector<LEDStrip> strips { buffer.popU64() };
		for (auto& strip : strips)
		{
			strip.m_Length = buffer.popU64();
			strip.m_Positions.resize(strip.m_Length);
			buffer.paste(strip.m_Positions.data(), strip.m_Positions.size() * sizeof(Vec3f));
			strip.m_Capabilities = std::vector<EStripCapability> { buffer.popU64() };
			buffer.paste(strip.m_Capabilities.data(), strip.m_Capabilities.size() * sizeof(EStripCapability));
		}
		return StripPacket { header, std::move(strips) };
	}

	void StripPacket::writeTo(Utils::Buffer& buffer)
	{
		PacketHeader::writeTo(buffer);
		buffer.push(static_cast<std::uint64_t>(m_Strips.size()));

		for (auto& strip : m_Strips)
		{
			buffer.push(strip.m_Length);
			buffer.copy(strip.m_Positions.data(), strip.m_Positions.size() * sizeof(Vec3f));
			buffer.push(static_cast<std::uint64_t>(strip.m_Capabilities.size()));
			buffer.copy(strip.m_Capabilities.data(), strip.m_Capabilities.size() * sizeof(EStripCapability));
		}
	}

	ClientsPacket ClientsPacket::ReadFrom(Utils::Buffer& buffer)
	{
		PacketHeader header = PacketHeader::ReadFrom(buffer);

		std::vector<Client> clients { buffer.popU64() };
		for (auto& client : clients)
		{
			client.m_UUID = buffer.popU64();
			client.m_Name = std::string(buffer.popU64(), '\0');
			buffer.paste(client.m_Name.data(), client.m_Name.size());
			client.m_StripCount = buffer.popU16();
			client.m_AreaCount  = buffer.popU32();
		}
		return ClientsPacket { header, std::move(clients) };
	}

	void ClientsPacket::writeTo(Utils::Buffer& buffer)
	{
		PacketHeader::writeTo(buffer);

		buffer.push(static_cast<std::uint64_t>(m_Clients.size()));
		for (auto& client : m_Clients)
		{
			buffer.push(client.m_UUID);
			buffer.push(static_cast<std::uint64_t>(client.m_Name.size()));
			buffer.copy(client.m_Name.data(), client.m_Name.size());
			buffer.push(client.m_StripCount);
			buffer.push(client.m_AreaCount);
		}
	}
} // namespace ModRGB::Protocol::Packets