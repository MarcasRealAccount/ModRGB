#pragma once

#include <cstddef>
#include <cstdint>

#include <type_traits>

namespace ModRGB::Utils
{
	class Buffer
	{
	public:
		Buffer(std::size_t dataOffset, std::uint8_t* buffer, std::size_t size) : m_DataOffset(dataOffset), m_Buffer(buffer), m_Offset(0U), m_Size(size) {}

		void copy(const void* from, std::size_t count);
		void push(std::uint8_t value);
		void push(std::uint16_t value);
		void push(std::uint32_t value);
		void push(std::uint64_t value);
		void push(std::int8_t value) { push(static_cast<std::uint8_t>(value)); }
		void push(std::int16_t value) { push(static_cast<std::uint16_t>(value)); }
		void push(std::int32_t value) { push(static_cast<std::uint32_t>(value)); }
		void push(std::int64_t value) { push(static_cast<std::uint64_t>(value)); }

		void          paste(void* to, std::size_t count);
		std::uint8_t  popU8();
		std::uint16_t popU16();
		std::uint32_t popU32();
		std::uint64_t popU64();
		std::int8_t   popI8() { return static_cast<std::int8_t>(popU8()); }
		std::int16_t  popI16() { return static_cast<std::int16_t>(popU16()); }
		std::int32_t  popI32() { return static_cast<std::int32_t>(popU32()); }
		std::int64_t  popI64() { return static_cast<std::int64_t>(popU64()); }

		auto getDataOffset() const { return m_DataOffset; }
		auto getBuffer() const { return m_Buffer; }
		auto getOffset() const { return m_Offset; }
		auto getSize() const { return m_Size; }

	private:
		std::size_t   m_DataOffset;
		std::uint8_t* m_Buffer;
		std::size_t   m_Offset;
		std::size_t   m_Size;
	};
} // namespace ModRGB::Utils