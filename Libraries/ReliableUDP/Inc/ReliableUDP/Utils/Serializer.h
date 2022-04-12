#pragma once

#include "Buffer.h"

namespace ReliableUDP::Utils
{
	template <class T>
	struct Serializer;

	namespace Details
	{
		template <class T>
		struct ConstType
		{
			using Type = T;
		};

		template <bool B, class A, class B>
		struct Select : public ConstType<B>
		{
		};

		template <class A, class B>
		struct Select<true, A, B> : public ConstType<A>
		{
		};

		template <bool B, class A, class B>
		using SelectType = typename Select<B, A, B>::Type;

		template <class... Ts>
		struct Union;

		template <class T, class... Ts>
		struct Union<T, Ts...>
		{
		public:
			using Type = T;
			using Next = Union<Ts...>;

			static constexpr std::size_t Count = 1 + sizeof...(Ts);

			template <std::size_t N>
			using Nth = SelectType<N == 0, Type, typename Next::Nth<N - 1>>;
		};

		template <>
		struct Union<>
		{
			using Type = void;
			using Next = Union<>;

			static constexpr std::size_t Count = 0;

			template <std::size_t N>
			using Nth = void;
		};

		template <class T>
		struct VariableInfo;

		template <class T, class C>
		struct VariableInfo<T C::*>
		{
		public:
			using Type       = T;
			using Class      = C;
			using Serializer = Serializer<Type>;
		};

		template <class T>
		concept UInt8Serializable = !std::is_pointer_v<T> && sizeof(T) == 1 && std::is_convertible_v<T, std::uint8_t> && std::is_convertible_v<std::uint8_t, T>;
		template <class T>
		concept UInt16Serializable = !std::is_pointer_v<T> && sizeof(T) == 2 && std::is_convertible_v<T, std::uint16_t> && std::is_convertible_v<std::uint16_t, T>;
		template <class T>
		concept UInt32Serializable = !std::is_pointer_v<T> && sizeof(T) == 4 && std::is_convertible_v<T, std::uint32_t> && std::is_convertible_v<std::uint32_t, T>;
		template <class T>
		concept UInt64Serializable = !std::is_pointer_v<T> && sizeof(T) == 8 && std::is_convertible_v<T, std::uint64_t> && std::is_convertible_v<std::uint64_t, T>;

		template <class T>
		concept Integral = UInt8Serializable<T> || UInt16Serializable<T> || UInt32Serializable<T> || UInt64Serializable<T>;
	} // namespace Details

	template <class... BaseTypes>
	struct Bases : public Details::Union<BaseTypes...>
	{
	};

	template <auto T>
	struct Variable
	{
	public:
		using Info    = Details::VariableInfo<decltype(T)>;
		using Type    = typename Info::Type;
		using Class   = typename Info::Class;
		using PtrType = Type Class::*;

		using Serializer = Info::Serializer;

		static constexpr PtrType Ptr = T;
	};

	template <class... Vars>
	struct Variables : public Details::Union<Vars...>
	{
	};

	template <class Bases, class Vars, bool DirectCopy>
	struct SerializerInfo
	{
	public:
		using BaseTypes = Bases;
		using Variables = Vars;

		static constexpr bool CanDirectCopy = DirectCopy;
	};

	template <class T>
	constexpr bool Serialize(Buffer buffer, const T& value) noexcept
	{
		return Serializer<T> {}.serialize(buffer, value);
	}

	template <class T>
	constexpr bool Deserialize(Buffer buffer, T& value) noexcept
	{
		return Serializer<T> {}.deserialize(buffer, value);
	}

	template <class T>
	constexpr std::size_t Size(const T& value) noexcept
	{
		return Serializer<T> {}.size(value);
	}

	template <class T>
	concept Serializable = requires
	{
		typename T::SerializerInfo;
	};

	template <Serializable T>
	struct Serializer<T>
	{
	public:
		using SerializerInfo = T::SerializerInfo;
		using BaseTypes      = typename SerializerInfo::BaseTypes;
		using Vars           = typename SerializerInfo::Variables;

	public:
		template <template <class...> class BT, class... Ts>
		constexpr bool serializeBases(Buffer& buffer, const T& value, BT<Ts...>) noexcept
		{
			return (Serializer<Ts> {}.serialize(buffer, static_cast<const Ts&>(value)) && ...);
		}

		template <template <class...> class BT, class... Ts>
		constexpr bool deserializeBases(Buffer& buffer, T& value, BT<Ts...>) noexcept
		{
			return (Serializer<Ts> {}.deserialize(buffer, static_cast<Ts&>(value)) && ...);
		}

		template <template <class...> class BT, class... Ts>
		constexpr std::size_t size(const T& value, BT<Ts...>) noexcept
		{
			std::size_t size = 0;
			(size += Serializer<Ts> {}.size(static_cast<const Ts&>(value)), ...);
			return size;
		}

		template <template <class...> class VT, class... Vs>
		constexpr bool serializeVars(Buffer& buffer, const T& value, VT<Vs...>) noexcept
		{
			return (Vs::Serializer {}.serialize(buffer, value.*Vs::Ptr) && ...);
		}

		template <template <class...> class VT, class... Vs>
		constexpr bool deserializeVars(Buffer& buffer, T& value, VT<Vs...>) noexcept
		{
			return (Vs::Serializer {}.deserialize(buffer, value.*Vs::Ptr) && ...);
		}

		template <template <class...> class VT, class... Vs>
		constexpr std::size_t sizeVars(const T& value, VT<Vs...>) noexcept
		{
			std::size_t size = 0;
			(size += Vs::Serializer {}.size(value.*Vs::Ptr), ...);
			return size;
		}

		constexpr bool serialize(Buffer& buffer, const T& value) noexcept
		{
			if constexpr (SerializerInfo::CanDirectCopy)
			{
				buffer.copy(&value, sizeof(T));
			}
			else
			{
				if constexpr (BaseTypes::Count > 0)
					if (!serializeBases(buffer, value, BaseTypes {}))
						return false;
				if constexpr (Vars::Count > 0)
					if (!serializeVars(buffer, value, Vars {}))
						return false;
			}
			return true;
		}

		constexpr bool deserialize(Buffer& buffer, T& value) noexcept
		{
			if constexpr (SerializerInfo::CanDirectCopy)
			{
				buffer.paste(&value, sizeof(T));
			}
			else
			{
				if constexpr (BaseTypes::Count > 0)
					if (!deserializeBases(buffer, value, BaseTypes {}))
						return false;
				if constexpr (Vars::Count > 0)
					if (!deserializeVars(buffer, value, Vars {}))
						return false;
			}
			return true;
		}

		constexpr std::size_t size(const T& value) noexcept
		{
			if constexpr (SerializerInfo::CanDirectCopy)
			{
				return sizeof(T);
			}
			else
			{
				std::size_t size = 0;
				if constexpr (BaseTypes::Count > 0)
					size += sizeBases(value, BaseTypes {});
				if constexpr (Vars::Count > 0)
					size += sizeVars(value, Vars {});
				return size;
			}
		}
	};

	template <Details::UInt8Serializable T>
	struct Serializer<T>
	{
	public:
		constexpr bool serialize(Buffer& buffer, T value) noexcept
		{
			buffer.pushU8(static_cast<std::uint8_t>(value));
			return true;
		}

		constexpr bool deserialize(Buffer& buffer, T& value) noexcept
		{
			value = static_cast<T>(buffer.popU8());
			return true;
		}

		constexpr std::size_t size(T value) noexcept
		{
			return 1U;
		}
	};

	template <Details::UInt16Serializable T>
	struct Serializer<T>
	{
	public:
		constexpr bool serialize(Buffer& buffer, T value) noexcept
		{
			buffer.pushU16(static_cast<std::uint16_t>(value));
			return true;
		}

		constexpr bool deserialize(Buffer& buffer, T& value) noexcept
		{
			value = static_cast<T>(buffer.popU16());
			return true;
		}

		constexpr std::size_t size(T value) noexcept
		{
			return 2U;
		}
	};

	template <Details::UInt32Serializable T>
	struct Serializer<T>
	{
	public:
		constexpr bool serialize(Buffer& buffer, T value) noexcept
		{
			buffer.pushU32(static_cast<std::uint32_t>(value));
			return true;
		}

		constexpr bool deserialize(Buffer& buffer, T& value) noexcept
		{
			value = static_cast<T>(buffer.popU32());
			return true;
		}

		constexpr std::size_t size(T value) noexcept
		{
			return 4U;
		}
	};

	template <Details::UInt64Serializable T>
	struct Serializer<T>
	{
	public:
		constexpr bool serialize(Buffer& buffer, T value) noexcept
		{
			buffer.pushU64(static_cast<std::uint64_t>(value));
			return true;
		}

		constexpr bool deserialize(Buffer& buffer, T& value) noexcept
		{
			value = static_cast<T>(buffer.popU64());
			return true;
		}

		constexpr std::size_t size(T value) noexcept
		{
			return 8U;
		}
	};

	template <class Value, class Traits, class Alloc>
	struct Serializer<std::basic_string<Value, Traits, Alloc>>
	{
	public:
		constexpr bool serialize(Buffer& buffer, const std::basic_string<Value, Traits, Alloc>& value) noexcept
		{
			buffer.pushU64(value.size());
			buffer.copy(value.data(), value.size() * sizeof(Value));
			return true;
		}

		constexpr bool deserialize(Buffer& buffer, std::basic_string<Value, Traits, Alloc>& value) noexcept
		{
			value.resize(buffer.popU64());
			buffer.paste(value.data(), value.size() * sizeof(Value));
			return true;
		}

		constexpr std::size_t size(const std::basic_string<Value, Traits, Alloc>& value) noexcept
		{
			return 8 + value.size() * sizeof(Value);
		}
	};

	template <class Value, class Alloc>
	struct Serializer<std::vector<Value, Alloc>>
	{
	public:
		constexpr bool serialize(Buffer& buffer, const std::vector<Value, Alloc>& values) noexcept
		{
			buffer.pushU64(values.size());
			for (auto& value : values)
				Serializer<Value> {}.serialize(buffer, value);
			return true;
		}

		constexpr bool deserialize(Buffer& buffer, std::vector<Value, Alloc>& values) noexcept
		{
			values.resize(buffer.popU64());
			for (auto& value : values)
				Serializer<Value> {}.deserialize(buffer, value);
			return true;
		}

		constexpr std::size_t size(const std::vector<Value, Alloc>& values) noexcept
		{
			std::size_t size = 8;
			for (auto& value : values)
				size += Serializer<Value> {}.size(value);
			return size;
		}
	};

	template <Details::Integral Value, class Alloc>
	struct Serializer<std::vector<Value, Alloc>>
	{
		constexpr bool serialize(Buffer& buffer, const std::vector<Value, Alloc>& value) noexcept
		{
			buffer.pushU64(value.size());
			buffer.copy(value.data(), value.size() * sizeof(Value));
			return true;
		}

		constexpr bool deserialize(Buffer& buffer, std::vector<Value, Alloc>& value) noexcept
		{
			value.resize(buffer.popU64());
			buffer.paste(value.data(), value.size() * sizeof(Value));
			return true;
		}

		constexpr std::size_t size(const std::vector<Value, Alloc>& value) noexcept
		{
			return 8 + value.size() * sizeof(Value);
		}
	};
} // namespace ReliableUDP::Utils