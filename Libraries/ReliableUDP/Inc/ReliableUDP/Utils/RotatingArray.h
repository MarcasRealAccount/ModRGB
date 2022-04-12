#pragma once

#include <cstddef>
#include <cstdint>

#include <type_traits>

namespace ReliableUDP::Utils
{
	template <class T, std::size_t N>
	struct RotatingArray
	{
	public:
		using value_type             = T;
		using size_type              = std::size_t;
		using difference_type        = std::ptrdiff_t;
		using reference              = value_type&;
		using const_reference        = const value_type&;
		using pointer                = value_type*;
		using const_pointer          = const value_type*;
		using iterator               = pointer;
		using const_iterator         = const_pointer;
		using reverse_iterator       = std::reverse_iterator<iterator>;
		using const_reverse_iterator = std::reverse_iterator<const_iterator>;

	public:
		constexpr void insert(const T& value) requires std::is_copy_constructible_v<T> || std::is_copy_assignable_v<T>
		{
			if constexpr (std::is_copy_constructible_v<T>)
				new (m_Elements + m_Index) T { value };
			else
				m_Elements[m_Index] = value;
			m_Index = (m_Index + 1) % N;
		}
		constexpr void insert(T&& value) requires std::is_move_constructible_v<T> || std::is_move_assignable_v<T>
		{
			if constexpr (std::is_move_constructible_v<T>)
				new (m_Elements + m_Index) T { std::move(value) };
			else
				m_Elements[m_Index] = std::move(value);
			m_Index = (m_Index + 1) % N;
		}

		constexpr reference at(size_type pos)
		{
			if (pos >= N)
				throw std::out_of_range("Accessing out of bounds");
			return m_Elements[pos];
		}
		constexpr const_reference at(size_type pos) const
		{
			if (pos >= N)
				throw std::out_of_range("Accessing out of bounds");
			return m_Elements[pos];
		}
		constexpr reference       operator[](size_type pos) { return m_Elements[pos]; }
		constexpr const_reference operator[](size_type pos) const { return m_Elements[pos]; }
		constexpr reference       front() { return m_Elements[0]; }
		constexpr const_reference front() const { return m_Elements[0]; }
		constexpr reference       back() { return m_Elements[N - 1]; }
		constexpr const_reference back() const { return m_Elements[N - 1]; }
		constexpr pointer         data() noexcept { return m_Elements; }
		constexpr const_pointer   data() const noexcept { return m_Elements; }

		constexpr iterator               begin() noexcept { return m_Elements; }
		constexpr const_iterator         begin() const noexcept { return m_Elements; }
		constexpr const_iterator         cbegin() const noexcept { return m_Elements; }
		constexpr iterator               end() noexcept { return m_Elements + N; }
		constexpr const_iterator         end() const noexcept { return m_Elements + N; }
		constexpr const_iterator         cend() const noexcept { return m_Elements + N; }
		constexpr reverse_iterator       rbegin() noexcept { return reverse_iterator { m_Elements + N - 1 }; }
		constexpr const_reverse_iterator rbegin() const noexcept { return const_reverse_iterator { m_Elements + N - 1 }; }
		constexpr const_reverse_iterator crbegin() const noexcept { return const_reverse_iterator { m_Elements + N - 1 }; }
		constexpr reverse_iterator       rend() noexcept { return reverse_iterator { m_Elements - 1 }; }
		constexpr const_reverse_iterator rend() const noexcept { return const_reverse_iterator { m_Elements - 1 }; }
		constexpr const_reverse_iterator crend() const noexcept { return const_reverse_iterator { m_Elements - 1 }; }

		[[nodiscard]] constexpr bool empty() const noexcept { return N == 0; }
		constexpr size_type          size() const noexcept { return N; }
		constexpr size_type          max_size() const noexcept { return N; }

		constexpr void fill(const T& value) requires std::is_copy_assignable_v<T> || std::is_copy_constructible_v<T>
		{
			for (size_t i = 0; i < N; ++i)
			{
				if constexpr (std::is_copy_constructible_v<T>)
					new (m_Elements + i) T { value };
				else
					m_Elements[i] = value;
			}
		}
		constexpr void swap(RotatingArray& other) noexcept(std::is_nothrow_swappable_v<T>)
		{
			for (size_t i = 0; i < N; ++i)
			{
				if constexpr (std::is_move_constructible_v<T>)
				{
					T tmp { std::move(other.m_Elements[i]) };
					new (other.m_Elements + i) T { std::move(m_Elements[i]) };
					new (m_Elements + i) T { std::move(tmp) };
				}
				else if constexpr (std::is_move_assignable_v<T>)
				{
					T tmp               = std::move(other.m_Elements[i]);
					other.m_Elements[i] = std::move(m_Elements[i]);
					m_Elements[i]       = std::move(tmp);
				}
				else if constexpr (std::is_copy_constructible_v<T>)
				{
					T tmp { other.m_Elements[i] };
					new (other.m_Elements + i) T { m_Elements[i] };
					new (m_Elements + i) T { tmp };
				}
				else
				{
					T tmp               = other.m_Elements[i];
					other.m_Elements[i] = m_Elements[i];
					m_Elements[i]       = tmp;
				}
			}
		}

	public:
		T m_Elements[N];

	private:
		std::size_t m_Index = 0;
	};
} // namespace ReliableUDP::Utils