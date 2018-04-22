// Copyright (c) 2018 Steinwurf ApS
// All Rights Reserved
//
// Distributed under the "BSD License". See the accompanying LICENSE.rst file.

#pragma once

#include <cassert>
#include <cstring>
#include <cstdint>
#include <limits>

#include "helpers.hpp"

namespace endian
{
namespace detail
{

// Where the actual conversion takes place
template<class ValueType, uint8_t Bytes>
struct little_impl
{
    static void put(ValueType& value, uint8_t* buffer)
    {
        *buffer = value & 0xFF;
        value = (value >> 8);

        little_impl<ValueType, Bytes - 1>::put(value, buffer + 1);
    }

    static void get(ValueType& value, const uint8_t* buffer)
    {
        little_impl<ValueType, Bytes - 1>::get(value, buffer + 1);

        value = (value << 8);
        value |= ((ValueType) *buffer);
    }
};

template<class ValueType>
struct little_impl<ValueType, 1>
{
    static void put(ValueType& value, uint8_t* buffer)
    {
        *buffer = value & 0xFF;
    }

    static void get(ValueType& value, const uint8_t* buffer)
    {
        value |= ((ValueType) *buffer);
    }
};

// Helper to delegate to the appropiate specialization depednign on the type
// @TODO remove these wrappers when we have CXX17 support and "if constexpr"
template<class ValueType, uint8_t Bytes,
         bool IsUnsigened = std::is_unsigned<ValueType>::value,
         bool IsFloat = std::is_floating_point<ValueType>::value>
struct little
{
    static void put(ValueType& value, uint8_t* buffer)
    {
        little<ValueType, Bytes, IsUnsigened, IsFloat>::put(value, buffer);
    }

    static void get(ValueType& value, const uint8_t* buffer)
    {
        little<ValueType, Bytes, IsUnsigened, IsFloat>::get(value, buffer);
    }
};

// Unsigned specialization
template<class ValueType, uint8_t Bytes>
struct little<ValueType, Bytes, true, false>
{
    static_assert(
        Bytes > sizeof(ValueType) / 2, "ValueType fits in type of"
        "half the size compared to the provide one, use a smaller type");

    static void put(ValueType& value, uint8_t* buffer)
    {
        assert((check<ValueType, Bytes>::value(value)) &&
               "Value too big to fit in the provided bytes");

        little_impl<ValueType, Bytes>::put(value, buffer);
    }

    static void get(ValueType& value, const uint8_t* buffer)
    {
        little_impl<ValueType, Bytes>::get(value, buffer);
    }
};

// Signed specialization
template<class ValueType, uint8_t Bytes>
struct little<ValueType, Bytes, false, false>
{
    static_assert(
        Bytes == sizeof(ValueType),
        "The number of bytes must match the size of the signed type");

    static void put(ValueType& value, uint8_t* buffer)
    {
        little_impl<ValueType, Bytes>::put(value, buffer);
    }

    static void get(ValueType& value, const uint8_t* buffer)
    {
        little_impl<ValueType, Bytes>::get(value, buffer);
    }
};

// Floating point type specialization
template<class ValueType, uint8_t Bytes>
struct little<ValueType, Bytes, false, true>
{
    static_assert(
        std::numeric_limits<ValueType>::is_iec559,
        "Platform must be iec559 compliant when floating point types are used");

    static_assert(
        Bytes == sizeof(ValueType),
        "The number of bytes must match the size of the floating type");

    static void put(ValueType& value, uint8_t* buffer)
    {
        typename floating_point<ValueType>::UnsignedType temp = 0;
        memcpy(&temp, &value, sizeof(ValueType));
        little_impl<decltype(temp), sizeof(ValueType)>::put(temp, buffer);
    }

    static void get(ValueType& value, const uint8_t* buffer)
    {
        typename floating_point<ValueType>::UnsignedType temp = 0;
        little_impl<decltype(temp), sizeof(ValueType)>::get(temp, buffer);
        memcpy(&value, &temp, sizeof(ValueType));
    }
};

}

// Inserts and extracts integers in little-endian format.
struct little_endian
{
    /// Gets a ValueType-sized integer value from a data buffer.
    /// @param value variable where to get the value
    /// @param buffer pointer to the data buffer
    template<class ValueType>
    static void get(ValueType& value, const uint8_t* buffer)
    {
        assert(buffer != nullptr);

        value = 0;
        detail::little<ValueType, sizeof(ValueType)>::get(value, buffer);
    }

    /// Gets a ValueType-sized integer value from a data buffer.
    /// @return value variable where to get the value
    /// @param buffer pointer to the data buffer
    template<class ValueType>
    static ValueType get(const uint8_t* buffer)
    {
        assert(buffer != nullptr);

        ValueType value = 0;
        get(value, buffer);
        return value;
    }

    /// Gets a Bytes-sized integer value from a data buffer.
    /// @param value variable where to get the value
    /// @param buffer pointer to the data buffer
    template<uint8_t Bytes, class ValueType>
    static void get_bytes(ValueType& value, const uint8_t* buffer)
    {
        static_assert(Bytes == sizeof(ValueType) ||
                      std::is_unsigned<ValueType>::value, "Must be unsigned");
        static_assert(sizeof(ValueType) >= Bytes, "ValueType too small");
        assert(buffer != nullptr);

        value = 0;
        detail::little<ValueType, Bytes>::get(value, buffer);
    }

    /// Gets a Bytes-sized integer value from a data buffer.
    /// @return value variable where to get the value
    /// @param buffer pointer to the data buffer
    template<uint8_t Bytes, class ValueType>
    static ValueType get_bytes(const uint8_t* buffer)
    {
        static_assert(Bytes == sizeof(ValueType) ||
                      std::is_unsigned<ValueType>::value, "Must be unsigned");
        static_assert(sizeof(ValueType) >= Bytes, "ValueType too small");
        assert(buffer != nullptr);

        ValueType value = 0;
        get_bytes<Bytes>(value, buffer);
        return value;
    }

    /// Inserts a ValueType-sized integer value into the data buffer.
    /// @param value to put in the data buffer
    /// @param buffer pointer to the data buffer
    template<class ValueType>
    static void put(ValueType value, uint8_t* buffer)
    {
        assert(buffer != nullptr);

        detail::little<ValueType, sizeof(ValueType)>::put(value, buffer);
    }

    /// Inserts a Bytes-sized integer value into the data buffer.
    /// @param value to put in the data buffer
    /// @param buffer pointer to the data buffer
    template<uint8_t Bytes, class ValueType>
    static void put_bytes(ValueType value, uint8_t* buffer)
    {
        static_assert(Bytes == sizeof(ValueType) ||
                      std::is_unsigned<ValueType>::value, "Must be unsigned");
        static_assert(sizeof(ValueType) >= Bytes, "ValueType too small");
        assert(buffer != nullptr);

        detail::little<ValueType, Bytes>::put(value, buffer);
    }
};
}
