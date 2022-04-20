#pragma once

#include <cstdint>

namespace ReliableUDP::Utils
{
	struct Version
	{
	public:
		constexpr Version() noexcept : m_Version(0U) {}
		constexpr Version(std::uint32_t version) noexcept : m_Version(version) {}
		constexpr Version(std::uint32_t major, std::uint32_t minor, std::uint32_t patch) noexcept : m_Version((major & 0x3FF) << 22 | (minor & 0x3FF) << 12 | (patch & 0xFFF)) {}

		constexpr operator std::uint32_t() const noexcept { return m_Version; }

		constexpr std::uint16_t major() const noexcept { return (m_Version >> 22) & 0x3FF; }
		constexpr std::uint16_t minor() const noexcept { return (m_Version >> 12) & 0x3FF; }
		constexpr std::uint16_t patch() const noexcept { return m_Version & 0xFFF; }
		constexpr void          major(std::uint16_t major) noexcept { m_Version = (m_Version & 0x3FFFFF) | (major & 0x3FF) << 22; }
		constexpr void          minor(std::uint16_t minor) noexcept { m_Version = (m_Version & 0xFFC00FFF) | (minor & 0x3FF) << 12; }
		constexpr void          patch(std::uint16_t patch) noexcept { m_Version = (m_Version & 0xFFFFF000) | (patch & 0xFFF); }

		constexpr bool isCompatibleWith(Version other) const noexcept { return !(other.major() != major() || other.minor() > minor()); }

	public:
		std::uint32_t m_Version;
	};
} // namespace ReliableUDP::Utils