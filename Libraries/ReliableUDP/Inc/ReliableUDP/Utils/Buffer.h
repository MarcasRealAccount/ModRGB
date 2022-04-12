#pragma once

#include <cstdint>

#include <vector>

namespace ReliableUDP::Utils
{
	struct Buffer
	{
	public:
		using FlushCallback = bool (*)(Buffer* buffer);

	public:
		constexpr Buffer(std::uint8_t* data, std::size_t size) noexcept : m_Data(data), m_Size(size), m_Offset(0U), m_FlushCallback(nullptr), m_UserData(nullptr) {}
		constexpr Buffer(std::uint8_t* data, std::size_t size, FlushCallback flushCallback, void* userData) noexcept : m_Data(data), m_Size(size), m_Offset(0U), m_FlushCallback(flushCallback), m_UserData(userData) {}

		constexpr void flip() noexcept { m_Offset = 0U; }
		constexpr bool flush() noexcept
		{
			if (m_FlushCallback)
			{
				if (!m_FlushCallback(this))
					return false;
			}
			else
			{
				return false;
			}
			m_Offset = 0U;
			return true;
		}

		constexpr void copy(const void* from, std::size_t count) noexcept
		{
			std::size_t remaining = m_Size - m_Offset;
			if (!remaining)
				return;

			if (remaining > count)
			{
				std::memcpy(m_Data + m_Offset, from, count);
				m_Offset += count;
				return;
			}

			while (count > 0)
			{
				std::size_t copyCount = std::min(remaining, count);
				std::memcpy(m_Data + m_Offset, from, copyCount);
				m_Offset += copyCount;
				count -= copyCount;
				remaining = m_Size - m_Offset;
				if (!remaining)
					if (!flush())
						break;
			}
		}
		constexpr void pushU8(std::uint8_t value) noexcept
		{
			if (m_Offset == m_Size)
				return;

			m_Data[m_Offset] = value;
			++m_Offset;
			if (m_Offset == m_Size)
				flush();
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
				if (m_Offset == m_Size)
					flush();
			}
			else
			{
				pushU8(static_cast<std::uint8_t>((value >> 8) & 0xFF));
				pushU8(static_cast<std::uint8_t>(value & 0xFF));
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
				if (m_Offset == m_Size)
					flush();
			}
			else
			{
				pushU16(static_cast<std::uint16_t>((value >> 16) & 0xFFFF));
				pushU16(static_cast<std::uint16_t>(value & 0xFFFF));
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
				if (m_Offset == m_Size)
					flush();
			}
			else
			{
				pushU32(static_cast<std::uint16_t>((value >> 32) & 0xFFFFFFFF));
				pushU32(static_cast<std::uint16_t>(value & 0xFFFFFFFF));
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
			std::size_t remaining = m_Size - m_Offset;
			if (!remaining)
				return;

			if (remaining > count)
			{
				std::memcpy(to, m_Data + m_Offset, count);
				m_Offset += count;
				return;
			}

			while (count > 0)
			{
				std::size_t copyCount = std::min(remaining, count);
				std::memcpy(to, m_Data + m_Offset, copyCount);
				m_Offset += copyCount;
				count -= copyCount;
				remaining = m_Size - m_Offset;
				if (!remaining)
					if (!flush())
						break;
			}
		}
		constexpr std::uint8_t popU8() noexcept
		{
			if (m_Offset == m_Size)
				return 0U;

			std::uint8_t v = m_Data[m_Offset];
			++m_Offset;
			if (m_Offset == m_Size)
				flush();
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
				if (m_Offset == m_Size)
					flush();
				return v;
			}
			else
			{
				std::uint16_t v = static_cast<std::uint16_t>(popU8()) << 8;
				v |= static_cast<std::uint16_t>(popU8());
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
				if (m_Offset == m_Size)
					flush();
				return v;
			}
			else
			{
				std::uint32_t v = static_cast<std::uint16_t>(popU16()) << 16;
				v |= static_cast<std::uint16_t>(popU16());
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
				if (m_Offset == m_Size)
					flush();
				return v;
			}
			else
			{
				std::uint64_t v = static_cast<std::uint64_t>(popU32()) << 32;
				v |= static_cast<std::uint64_t>(popU32());
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
		constexpr auto getFlushCallback() const noexcept { return m_FlushCallback; }
		constexpr auto getUserData() const noexcept { return m_UserData; }

	public:
		std::uint8_t* m_Data;
		std::size_t   m_Size;
		std::size_t   m_Offset;

		FlushCallback m_FlushCallback;
		void*         m_UserData;
	};
} // namespace ReliableUDP::Utils