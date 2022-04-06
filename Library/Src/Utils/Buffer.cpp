#include "ModRGB/Utils/Buffer.h"

#include <cstring>

#include <algorithm>

namespace ModRGB::Utils
{
	void Buffer::copy(const void* from, std::size_t count)
	{
		if (m_Offset == m_Size)
		{
			return;
		}
		else if (m_DataOffset > count)
		{
			m_DataOffset -= count;
			return;
		}
		else
		{
			count -= m_DataOffset;
			m_DataOffset = 0;
			count        = std::min(count, m_Size - m_Offset);
			std::memcpy(m_Buffer + m_Offset, from, count);
			m_Offset += count;
		}
	}

	void Buffer::push(std::uint8_t value)
	{
		if (m_Offset == m_Size)
		{
			return;
		}
		else if (m_DataOffset > 0)
		{
			--m_DataOffset;
			return;
		}
		else
		{
			m_Buffer[m_Offset] = value;
			++m_Offset;
		}
	}

	void Buffer::push(std::uint16_t value)
	{
		push(static_cast<std::uint8_t>(value >> 8));
		push(static_cast<std::uint8_t>(value));
	}

	void Buffer::push(std::uint32_t value)
	{
		push(static_cast<std::uint16_t>(value >> 16));
		push(static_cast<std::uint16_t>(value));
	}

	void Buffer::push(std::uint64_t value)
	{
		push(static_cast<std::uint32_t>(value >> 32));
		push(static_cast<std::uint32_t>(value));
	}

	void Buffer::paste(void* to, std::size_t count)
	{
		if (m_Offset == m_Size)
		{
			return;
		}
		else if (m_DataOffset > count)
		{
			m_DataOffset -= count;
			return;
		}
		else
		{
			count -= m_DataOffset;
			m_DataOffset = 0;
			count        = std::min(count, m_Size - m_Offset);
			std::memcpy(to, m_Buffer, count);
			m_Offset += count;
		}
	}

	std::uint8_t Buffer::popU8()
	{
		if (m_Offset == m_Size)
		{
			return 0U;
		}
		else if (m_DataOffset > 0)
		{
			--m_DataOffset;
			return 0U;
		}
		else
		{
			auto value = m_Buffer[m_Offset];
			++m_Offset;
			return value;
		}
	}

	std::uint16_t Buffer::popU16()
	{
		return popU8() << 8 | popU8();
	}

	std::uint32_t Buffer::popU32()
	{
		return popU16() << 16 | popU16();
	}

	std::uint64_t Buffer::popU64()
	{
		return popU32() << 32 | popU32();
	}
} // namespace ModRGB::Utils