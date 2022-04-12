#pragma once

#include <cstdint>

#include <vector>

namespace ReliableUDP::Utils
{
	struct Buffer
	{
	public:
		constexpr Buffer(std::uint8_t* data, std::size_t size) noexcept : m_Data(data), m_Size(size), m_Offset(0U) {}

		constexpr void flip() noexcept { m_Offset = 0U; }

		constexpr void copy(const void* from, std::size_t count) noexcept
		{
			if (m_Offset == m_Size)
				return;

			count = std::min(m_Size - m_Offset, count);
			if (count > 0)
			{
				std::memcpy(m_Data + m_Offset, from, count);
				m_Offset += count;
			}
		}
		constexpr void pushU8(std::uint8_t value) noexcept
		{
			if (m_Offset == m_Size)
				return;

			m_Data[m_Offset] = value;
			++m_Offset;
		}
		constexpr void pushU16(std::uint16_t value) noexcept
		{
			std::size_t remaining = m_Size - m_Offset;
			if (!remaining)
				return;

			if (remaining > 1)
			{
				*reinterpret_cast<std::uint16_t*>(m_Data + m_Offset) = value;
				m_Offset += 2;
			}
			else
			{
				m_Data[m_Offset] = static_cast<std::uint8_t>((value >> 8) & 0xFF);
				++m_Offset;
			}
		}
		constexpr void pushU32(std::uint32_t value) noexcept
		{
			std::size_t remaining = m_Size - m_Offset;
			if (!remaining)
				return;

			if (remaining > 3)
			{
				*reinterpret_cast<std::uint32_t*>(m_Data + m_Offset) = value;
				m_Offset += 4;
			}
			else if (remaining > 2)
			{
				*reinterpret_cast<std::uint16_t*>(m_Data + m_Offset) = static_cast<std::uint16_t>((value >> 16) & 0xFFFF);
				m_Offset += 2;
				m_Data[m_Offset] = static_cast<std::uint8_t>((value >> 8) & 0xFF);
				++m_Offset;
			}
			else if (remaining > 1)
			{
				*reinterpret_cast<std::uint16_t*>(m_Data + m_Offset) = static_cast<std::uint16_t>((value >> 16) & 0xFFFF);
				m_Offset += 2;
			}
			else
			{
				m_Data[m_Offset] = static_cast<std::uint8_t>((value >> 24) & 0xFF);
				++m_Offset;
			}
		}
		constexpr void pushU64(std::uint64_t value) noexcept
		{
			std::size_t remaining = m_Size - m_Offset;
			if (!remaining)
				return;

			if (remaining > 7)
			{
				*reinterpret_cast<std::uint64_t*>(m_Data + m_Offset) = value;
				m_Offset += 8;
			}
			else if (remaining > 6)
			{
				*reinterpret_cast<std::uint32_t*>(m_Data + m_Offset) = static_cast<std::uint32_t>((value >> 32) & 0xFFFFFFFF);
				m_Offset += 4;
				*reinterpret_cast<std::uint16_t*>(m_Data + m_Offset) = static_cast<std::uint16_t>((value >> 16) & 0xFFFF);
				m_Offset += 2;
				m_Data[m_Offset] = static_cast<std::uint8_t>((value >> 8) & 0xFF);
				++m_Offset;
			}
			else if (remaining > 5)
			{
				*reinterpret_cast<std::uint32_t*>(m_Data + m_Offset) = static_cast<std::uint32_t>((value >> 32) & 0xFFFFFFFF);
				m_Offset += 4;
				*reinterpret_cast<std::uint16_t*>(m_Data + m_Offset) = static_cast<std::uint16_t>((value >> 16) & 0xFFFF);
				m_Offset += 2;
			}
			else if (remaining > 4)
			{
				*reinterpret_cast<std::uint32_t*>(m_Data + m_Offset) = static_cast<std::uint32_t>((value >> 32) & 0xFFFFFFFF);
				m_Offset += 4;
				m_Data[m_Offset] = static_cast<std::uint8_t>((value >> 24) & 0xFF);
				++m_Offset;
			}
			else if (remaining > 3)
			{
				*reinterpret_cast<std::uint32_t*>(m_Data + m_Offset) = static_cast<std::uint32_t>((value >> 32) & 0xFFFFFFFF);
				m_Offset += 4;
			}
			else if (remaining > 2)
			{
				*reinterpret_cast<std::uint16_t*>(m_Data + m_Offset) = static_cast<std::uint16_t>((value >> 48) & 0xFFFF);
				m_Offset += 2;
				m_Data[m_Offset] = static_cast<std::uint8_t>((value >> 40) & 0xFF);
				++m_Offset;
			}
			else if (remaining > 1)
			{
				*reinterpret_cast<std::uint16_t*>(m_Data + m_Offset) = static_cast<std::uint16_t>((value >> 48) & 0xFFFF);
				m_Offset += 2;
			}
			else
			{
				m_Data[m_Offset] = static_cast<std::uint8_t>((value >> 56) & 0xFF);
				++m_Offset;
			}
		}
		constexpr void pushI8(std::int8_t value) noexcept { pushU8(static_cast<std::uint8_t>(value)); }
		constexpr void pushI16(std::int16_t value) noexcept { pushU16(static_cast<std::uint16_t>(value)); }
		constexpr void pushI32(std::int32_t value) noexcept { pushU32(static_cast<std::uint32_t>(value)); }
		constexpr void pushI64(std::int64_t value) noexcept { pushU64(static_cast<std::uint64_t>(value)); }
		constexpr void pushF32(float value) noexcept { pushU32(std::bit_cast<std::uint32_t>(value)); }
		constexpr void pushF64(double value) noexcept { pushU32(std::bit_cast<std::uint64_t>(value)); }

		constexpr void paste(void* to, std::size_t count) noexcept
		{
			if (m_Offset == m_Size)
				return;

			count = std::min(m_Size - m_Offset, count);
			if (count > 0)
			{
				std::memcpy(to, m_Data + m_Offset, count);
				m_Offset += count;
			}
		}
		constexpr std::uint8_t popU8() noexcept
		{
			if (m_Offset == m_Size)
				return 0U;

			std::uint8_t v = m_Data[m_Offset];
			++m_Offset;
			return v;
		}
		constexpr std::uint16_t popU16() noexcept
		{
			std::size_t remaining = m_Size - m_Offset;
			if (!remaining)
				return 0U;

			if (remaining > 1)
			{
				std::uint16_t v = *reinterpret_cast<std::uint16_t*>(m_Data + m_Offset);
				m_Offset += 2;
				return v;
			}
			else
			{
				std::uint16_t v = static_cast<std::uint16_t>(m_Data[m_Offset]) << 8;
				++m_Offset;
				return v;
			}
		}
		constexpr std::uint32_t popU32() noexcept
		{
			std::size_t remaining = m_Size - m_Offset;
			if (!remaining)
				return 0U;

			if (remaining > 3)
			{
				std::uint32_t v = *reinterpret_cast<std::uint32_t*>(m_Data + m_Offset);
				m_Offset += 4;
				return v;
			}
			else if (remaining > 2)
			{
				std::uint32_t v = static_cast<std::uint32_t>(*reinterpret_cast<std::uint16_t*>(m_Data + m_Offset)) << 16;
				m_Offset += 2;
				v |= static_cast<std::uint32_t>(m_Data[m_Offset]) << 8;
				++m_Offset;
				return v;
			}
			else if (remaining > 1)
			{
				std::uint32_t v = static_cast<std::uint32_t>(*reinterpret_cast<std::uint16_t*>(m_Data + m_Offset)) << 16;
				m_Offset += 2;
				return v;
			}
			else
			{
				std::uint32_t v = static_cast<std::uint32_t>(m_Data[m_Offset]) << 24;
				++m_Offset;
				return v;
			}
		}
		constexpr std::uint64_t popU64() noexcept
		{
			std::size_t remaining = m_Size - m_Offset;
			if (!remaining)
				return 0U;

			if (remaining > 7)
			{
				std::uint64_t v = *reinterpret_cast<std::uint64_t*>(m_Data + m_Offset);
				m_Offset += 8;
				return v;
			}
			else if (remaining > 6)
			{
				std::uint64_t v = static_cast<std::uint64_t>(*reinterpret_cast<std::uint32_t*>(m_Data + m_Offset)) << 32;
				m_Offset += 4;
				v |= static_cast<std::uint64_t>(*reinterpret_cast<std::uint16_t*>(m_Data + m_Offset)) << 16;
				m_Offset += 2;
				v |= static_cast<std::uint64_t>(m_Data[m_Offset]) << 8;
				++m_Offset;
				return v;
			}
			else if (remaining > 5)
			{
				std::uint64_t v = static_cast<std::uint64_t>(*reinterpret_cast<std::uint32_t*>(m_Data + m_Offset)) << 32;
				m_Offset += 4;
				v |= static_cast<std::uint64_t>(*reinterpret_cast<std::uint16_t*>(m_Data + m_Offset)) << 16;
				m_Offset += 2;
				return v;
			}
			else if (remaining > 4)
			{
				std::uint64_t v = static_cast<std::uint64_t>(*reinterpret_cast<std::uint32_t*>(m_Data + m_Offset)) << 32;
				m_Offset += 4;
				v |= static_cast<std::uint64_t>(m_Data[m_Offset]) << 24;
				++m_Offset;
				return v;
			}
			else if (remaining > 3)
			{
				std::uint64_t v = static_cast<std::uint64_t>(*reinterpret_cast<std::uint32_t*>(m_Data + m_Offset)) << 32;
				m_Offset += 4;
				return v;
			}
			else if (remaining > 2)
			{
				std::uint64_t v = static_cast<std::uint64_t>(*reinterpret_cast<std::uint16_t*>(m_Data + m_Offset)) << 48;
				m_Offset += 2;
				v |= static_cast<std::uint64_t>(m_Data[m_Offset]) << 40;
				++m_Offset;
				return v;
			}
			else if (remaining > 1)
			{
				std::uint64_t v = static_cast<std::uint64_t>(*reinterpret_cast<std::uint16_t*>(m_Data + m_Offset)) << 48;
				m_Offset += 2;
				return v;
			}
			else
			{
				std::uint64_t v = static_cast<std::uint64_t>(*reinterpret_cast<std::uint8_t*>(m_Data + m_Offset)) << 56;
				++m_Offset;
				return v;
			}
		}
		constexpr std::int8_t  popI8() noexcept { return static_cast<std::int8_t>(popU8()); }
		constexpr std::int16_t popI16() noexcept { return static_cast<std::int16_t>(popU16()); }
		constexpr std::int32_t popI32() noexcept { return static_cast<std::int32_t>(popU32()); }
		constexpr std::int64_t popI64() noexcept { return static_cast<std::int64_t>(popU64()); }
		constexpr float        popF32() noexcept { return std::bit_cast<float>(popU32()); }
		constexpr double       popF64() noexcept { return std::bit_cast<double>(popU64()); }

		constexpr auto data() const noexcept { return m_Data; }
		constexpr auto size() const noexcept { return m_Size; }
		constexpr auto offset() const noexcept { return m_Offset; }

	public:
		std::uint8_t* m_Data;
		std::size_t   m_Size;
		std::size_t   m_Offset;
	};
} // namespace ReliableUDP::Utils