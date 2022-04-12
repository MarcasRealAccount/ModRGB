#pragma once

#include <cstdint>

namespace ReliableUDP::Utils
{
	struct Version
	{
	public:
		constexpr Version() noexcept : m_Version(0U) {}
		constexpr Version(std::uint32_t version) noexcept : m_Version(version) {}
		constexpr Version(std::uint32_t major, std::uint32_t minor, std::uint32_t patch) noexcept : m_SubVersions(major, minor, patch) {}

		constexpr operator std::uint32_t() const noexcept { return m_Version; }

		constexpr bool isCompatible(Version other) const noexcept { return !(other.m_SubVersions.major != m_SubVersions.major || other.m_SubVersions.minor > m_SubVersions.minor); }

	public:
		union
		{
			struct SubVersions
			{
			public:
				constexpr SubVersions(std::uint32_t major, std::uint32_t minor, std::uint32_t patch) : patch(patch), minor(minor), major(major) {}

			public:
				std::uint32_t patch : 14;
				std::uint32_t minor : 10;
				std::uint32_t major : 10;
			} m_SubVersions;
			std::uint32_t m_Version;
		};
	};
} // namespace ReliableUDP::Utils