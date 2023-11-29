/*
 *  Copyright (c) 2016, The OpenThread Authors.
 *  All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions are met:
 *  1. Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *  2. Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *  3. Neither the name of the copyright holder nor the
 *     names of its contributors may be used to endorse or promote products
 *     derived from this software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 *  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 *  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 *  ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 *  LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 *  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 *  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 *  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 *  CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 *  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 *  POSSIBILITY OF SUCH DAMAGE.
 */

/**
 * @file
 *   This file includes definitions for byte-ordering encoding.
 */

#ifndef ENCODING_HPP_
#define ENCODING_HPP_

#include "openthread-core-config.h"

#ifndef BYTE_ORDER_BIG_ENDIAN
#if defined(WORDS_BIGENDIAN) || \
    defined(__BYTE_ORDER__) && defined(__ORDER_BIG_ENDIAN__) && __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
#define BYTE_ORDER_BIG_ENDIAN 1
#else
#define BYTE_ORDER_BIG_ENDIAN 0
#endif
#endif

#include <stdint.h>

namespace ot {

inline uint16_t Swap16(uint16_t v) { return (((v & 0x00ffU) << 8) & 0xff00) | (((v & 0xff00U) >> 8) & 0x00ff); }

inline uint32_t Swap32(uint32_t v)
{
    return ((v & static_cast<uint32_t>(0x000000ffUL)) << 24) | ((v & static_cast<uint32_t>(0x0000ff00UL)) << 8) |
           ((v & static_cast<uint32_t>(0x00ff0000UL)) >> 8) | ((v & static_cast<uint32_t>(0xff000000UL)) >> 24);
}

inline uint64_t Swap64(uint64_t v)
{
    return ((v & static_cast<uint64_t>(0x00000000000000ffULL)) << 56) |
           ((v & static_cast<uint64_t>(0x000000000000ff00ULL)) << 40) |
           ((v & static_cast<uint64_t>(0x0000000000ff0000ULL)) << 24) |
           ((v & static_cast<uint64_t>(0x00000000ff000000ULL)) << 8) |
           ((v & static_cast<uint64_t>(0x000000ff00000000ULL)) >> 8) |
           ((v & static_cast<uint64_t>(0x0000ff0000000000ULL)) >> 24) |
           ((v & static_cast<uint64_t>(0x00ff000000000000ULL)) >> 40) |
           ((v & static_cast<uint64_t>(0xff00000000000000ULL)) >> 56);
}

inline uint32_t Reverse32(uint32_t v)
{
    v = ((v & 0x55555555) << 1) | ((v & 0xaaaaaaaa) >> 1);
    v = ((v & 0x33333333) << 2) | ((v & 0xcccccccc) >> 2);
    v = ((v & 0x0f0f0f0f) << 4) | ((v & 0xf0f0f0f0) >> 4);
    v = ((v & 0x00ff00ff) << 8) | ((v & 0xff00ff00) >> 8);
    v = ((v & 0x0000ffff) << 16) | ((v & 0xffff0000) >> 16);

    return v;
}

namespace BigEndian {

#if BYTE_ORDER_BIG_ENDIAN

inline uint16_t HostSwap16(uint16_t v) { return v; }
inline uint32_t HostSwap32(uint32_t v) { return v; }
inline uint64_t HostSwap64(uint64_t v) { return v; }

#else /* BYTE_ORDER_LITTLE_ENDIAN */

inline uint16_t HostSwap16(uint16_t v) { return Swap16(v); }
inline uint32_t HostSwap32(uint32_t v) { return Swap32(v); }
inline uint64_t HostSwap64(uint64_t v) { return Swap64(v); }

#endif // LITTLE_ENDIAN

/**
 * This template function performs host swap on a given unsigned integer value assuming big-endian encoding.
 *
 * @tparam  UintType   The unsigned int type.
 *
 * @param   aValue     The value to host swap.
 *
 * @returns The host swapped value.
 *
 */
template <typename UintType> UintType HostSwap(UintType aValue);

template <> inline uint8_t  HostSwap(uint8_t aValue) { return aValue; }
template <> inline uint16_t HostSwap(uint16_t aValue) { return HostSwap16(aValue); }
template <> inline uint32_t HostSwap(uint32_t aValue) { return HostSwap32(aValue); }
template <> inline uint64_t HostSwap(uint64_t aValue) { return HostSwap64(aValue); }

/**
 * Reads a `uint16_t` value from a given buffer assuming big-endian encoding.
 *
 * @param[in] aBuffer   Pointer to buffer to read from.
 *
 * @returns The `uint16_t` value read from buffer.
 *
 */
inline uint16_t ReadUint16(const uint8_t *aBuffer) { return static_cast<uint16_t>((aBuffer[0] << 8) | aBuffer[1]); }

/**
 * Reads a `uint32_t` value from a given buffer assuming big-endian encoding.
 *
 * @param[in] aBuffer   Pointer to buffer to read from.
 *
 * @returns The `uint32_t` value read from buffer.
 *
 */
inline uint32_t ReadUint32(const uint8_t *aBuffer)
{
    return ((static_cast<uint32_t>(aBuffer[0]) << 24) | (static_cast<uint32_t>(aBuffer[1]) << 16) |
            (static_cast<uint32_t>(aBuffer[2]) << 8) | (static_cast<uint32_t>(aBuffer[3]) << 0));
}

/**
 * Reads a 24-bit integer value from a given buffer assuming big-endian encoding.
 *
 * @param[in] aBuffer   Pointer to buffer to read from.
 *
 * @returns The value read from buffer.
 *
 */
inline uint32_t ReadUint24(const uint8_t *aBuffer)
{
    return ((static_cast<uint32_t>(aBuffer[0]) << 16) | (static_cast<uint32_t>(aBuffer[1]) << 8) |
            (static_cast<uint32_t>(aBuffer[2]) << 0));
}

/**
 * Reads a `uint64_t` value from a given buffer assuming big-endian encoding.
 *
 * @param[in] aBuffer   Pointer to buffer to read from.
 *
 * @returns The `uint64_t` value read from buffer.
 *
 */
inline uint64_t ReadUint64(const uint8_t *aBuffer)
{
    return ((static_cast<uint64_t>(aBuffer[0]) << 56) | (static_cast<uint64_t>(aBuffer[1]) << 48) |
            (static_cast<uint64_t>(aBuffer[2]) << 40) | (static_cast<uint64_t>(aBuffer[3]) << 32) |
            (static_cast<uint64_t>(aBuffer[4]) << 24) | (static_cast<uint64_t>(aBuffer[5]) << 16) |
            (static_cast<uint64_t>(aBuffer[6]) << 8) | (static_cast<uint64_t>(aBuffer[7]) << 0));
}

/**
 * Reads a `UintType` integer value from a given buffer assuming big-endian encoding.
 *
 * @tparam  UintType   The unsigned int type.
 *
 * @param[in] aBuffer   Pointer to the buffer to read from.
 *
 * @returns The `UintType` value read from the buffer.
 *
 */
template <typename UintType> UintType Read(const uint8_t *aBuffer);

template <> inline uint8_t  Read(const uint8_t *aBuffer) { return *aBuffer; }
template <> inline uint16_t Read(const uint8_t *aBuffer) { return ReadUint16(aBuffer); }
template <> inline uint32_t Read(const uint8_t *aBuffer) { return ReadUint32(aBuffer); }
template <> inline uint64_t Read(const uint8_t *aBuffer) { return ReadUint64(aBuffer); }

/**
 * Writes a `uint16_t` value to a given buffer using big-endian encoding.
 *
 * @param[in]  aValue    The value to write to buffer.
 * @param[out] aBuffer   Pointer to buffer where the value will be written.
 *
 */
inline void WriteUint16(uint16_t aValue, uint8_t *aBuffer)
{
    aBuffer[0] = (aValue >> 8) & 0xff;
    aBuffer[1] = (aValue >> 0) & 0xff;
}

/**
 * Writes a 24-bit integer value to a given buffer using big-endian encoding.
 *
 * @param[in]  aValue    The value to write to buffer.
 * @param[out] aBuffer   Pointer to buffer where the value will be written.
 *
 */
inline void WriteUint24(uint32_t aValue, uint8_t *aBuffer)
{
    aBuffer[0] = (aValue >> 16) & 0xff;
    aBuffer[1] = (aValue >> 8) & 0xff;
    aBuffer[2] = (aValue >> 0) & 0xff;
}

/**
 * Writes a `uint32_t` value to a given buffer using big-endian encoding.
 *
 * @param[in]  aValue    The value to write to buffer.
 * @param[out] aBuffer   Pointer to buffer where the value will be written.
 *
 */
inline void WriteUint32(uint32_t aValue, uint8_t *aBuffer)
{
    aBuffer[0] = (aValue >> 24) & 0xff;
    aBuffer[1] = (aValue >> 16) & 0xff;
    aBuffer[2] = (aValue >> 8) & 0xff;
    aBuffer[3] = (aValue >> 0) & 0xff;
}

/**
 * Writes a `uint64_t` value to a given buffer using big-endian encoding.
 *
 * @param[in]  aValue    The value to write to buffer.
 * @param[out] aBuffer   Pointer to buffer where the value will be written.
 *
 */
inline void WriteUint64(uint64_t aValue, uint8_t *aBuffer)
{
    aBuffer[0] = (aValue >> 56) & 0xff;
    aBuffer[1] = (aValue >> 48) & 0xff;
    aBuffer[2] = (aValue >> 40) & 0xff;
    aBuffer[3] = (aValue >> 32) & 0xff;
    aBuffer[4] = (aValue >> 24) & 0xff;
    aBuffer[5] = (aValue >> 16) & 0xff;
    aBuffer[6] = (aValue >> 8) & 0xff;
    aBuffer[7] = (aValue >> 0) & 0xff;
}

/**
 * Writes a `UintType` integer value to a given buffer assuming big-endian encoding.
 *
 * @tparam  UintType   The unsigned int type.
 *
 * @param[in] aValue    The value to write to buffer.
 * @param[in] aBuffer   Pointer to the buffer to write to.
 *
 */
template <typename UintType> void Write(UintType aValue, uint8_t *aBuffer);

template <> inline void Write(uint8_t aValue, uint8_t *aBuffer) { *aBuffer = aValue; }
template <> inline void Write(uint16_t aValue, uint8_t *aBuffer) { WriteUint16(aValue, aBuffer); }
template <> inline void Write(uint32_t aValue, uint8_t *aBuffer) { WriteUint32(aValue, aBuffer); }
template <> inline void Write(uint64_t aValue, uint8_t *aBuffer) { WriteUint64(aValue, aBuffer); }

} // namespace BigEndian

namespace LittleEndian {

#if BYTE_ORDER_BIG_ENDIAN

inline uint16_t HostSwap16(uint16_t v) { return Swap16(v); }
inline uint32_t HostSwap32(uint32_t v) { return Swap32(v); }
inline uint64_t HostSwap64(uint64_t v) { return Swap64(v); }

#else /* BYTE_ORDER_LITTLE_ENDIAN */

inline uint16_t HostSwap16(uint16_t v) { return v; }
inline uint32_t HostSwap32(uint32_t v) { return v; }
inline uint64_t HostSwap64(uint64_t v) { return v; }

#endif

/**
 * This template function performs host swap on a given unsigned integer value assuming little-endian encoding.
 *
 * @tparam  UintType   The unsigned int type.
 *
 * @param   aValue     The value to host swap.
 *
 * @returns The host swapped value.
 *
 */
template <typename UintType> UintType HostSwap(UintType aValue);

template <> inline uint8_t  HostSwap(uint8_t aValue) { return aValue; }
template <> inline uint16_t HostSwap(uint16_t aValue) { return HostSwap16(aValue); }
template <> inline uint32_t HostSwap(uint32_t aValue) { return HostSwap32(aValue); }
template <> inline uint64_t HostSwap(uint64_t aValue) { return HostSwap64(aValue); }

/**
 * Reads a `uint16_t` value from a given buffer assuming little-endian encoding.
 *
 * @param[in] aBuffer   Pointer to buffer to read from.
 *
 * @returns The `uint16_t` value read from buffer.
 *
 */
inline uint16_t ReadUint16(const uint8_t *aBuffer) { return static_cast<uint16_t>(aBuffer[0] | (aBuffer[1] << 8)); }

/**
 * Reads a 24-bit integer value from a given buffer assuming little-endian encoding.
 *
 * @param[in] aBuffer   Pointer to buffer to read from.
 *
 * @returns The value read from buffer.
 *
 */
inline uint32_t ReadUint24(const uint8_t *aBuffer)
{
    return ((static_cast<uint32_t>(aBuffer[0]) << 0) | (static_cast<uint32_t>(aBuffer[1]) << 8) |
            (static_cast<uint32_t>(aBuffer[2]) << 16));
}

/**
 * Reads a `uint32_t` value from a given buffer assuming little-endian encoding.
 *
 * @param[in] aBuffer   Pointer to buffer to read from.
 *
 * @returns The `uint32_t` value read from buffer.
 *
 */
inline uint32_t ReadUint32(const uint8_t *aBuffer)
{
    return ((static_cast<uint32_t>(aBuffer[0]) << 0) | (static_cast<uint32_t>(aBuffer[1]) << 8) |
            (static_cast<uint32_t>(aBuffer[2]) << 16) | (static_cast<uint32_t>(aBuffer[3]) << 24));
}

/**
 * Reads a `uint64_t` value from a given buffer assuming little-endian encoding.
 *
 * @param[in] aBuffer   Pointer to buffer to read from.
 *
 * @returns The `uint64_t` value read from buffer.
 *
 */
inline uint64_t ReadUint64(const uint8_t *aBuffer)
{
    return ((static_cast<uint64_t>(aBuffer[0]) << 0) | (static_cast<uint64_t>(aBuffer[1]) << 8) |
            (static_cast<uint64_t>(aBuffer[2]) << 16) | (static_cast<uint64_t>(aBuffer[3]) << 24) |
            (static_cast<uint64_t>(aBuffer[4]) << 32) | (static_cast<uint64_t>(aBuffer[5]) << 40) |
            (static_cast<uint64_t>(aBuffer[6]) << 48) | (static_cast<uint64_t>(aBuffer[7]) << 56));
}

/**
 * Reads a `UintType` integer value from a given buffer assuming little-endian encoding.
 *
 * @tparam  UintType   The unsigned int type.
 *
 * @param[in] aBuffer   Pointer to the buffer to read from.
 *
 * @returns The `UintType` value read from the buffer.
 *
 */
template <typename UintType> UintType Read(const uint8_t *aBuffer);

template <> inline uint8_t  Read(const uint8_t *aBuffer) { return *aBuffer; }
template <> inline uint16_t Read(const uint8_t *aBuffer) { return ReadUint16(aBuffer); }
template <> inline uint32_t Read(const uint8_t *aBuffer) { return ReadUint32(aBuffer); }
template <> inline uint64_t Read(const uint8_t *aBuffer) { return ReadUint64(aBuffer); }

/**
 * Writes a `uint16_t` value to a given buffer using little-endian encoding.
 *
 * @param[in]  aValue    The value to write to buffer.
 * @param[out] aBuffer   Pointer to buffer where the value will be written.
 *
 */
inline void WriteUint16(uint16_t aValue, uint8_t *aBuffer)
{
    aBuffer[0] = (aValue >> 0) & 0xff;
    aBuffer[1] = (aValue >> 8) & 0xff;
}

/**
 * Writes a 24-bit integer value to a given buffer using little-endian encoding.
 *
 * @param[in]  aValue   The value to write to buffer.
 * @param[out] aBuffer  Pointer to buffer where the value will be written.
 *
 */
inline void WriteUint24(uint32_t aValue, uint8_t *aBuffer)
{
    aBuffer[0] = (aValue >> 0) & 0xff;
    aBuffer[1] = (aValue >> 8) & 0xff;
    aBuffer[2] = (aValue >> 16) & 0xff;
}

/**
 * Writes a `uint32_t` value to a given buffer using little-endian encoding.
 *
 * @param[in]  aValue   The value to write to buffer.
 * @param[out] aBuffer  Pointer to buffer where the value will be written.
 *
 */
inline void WriteUint32(uint32_t aValue, uint8_t *aBuffer)
{
    aBuffer[0] = (aValue >> 0) & 0xff;
    aBuffer[1] = (aValue >> 8) & 0xff;
    aBuffer[2] = (aValue >> 16) & 0xff;
    aBuffer[3] = (aValue >> 24) & 0xff;
}

/**
 * Writes a `uint64_t` value to a given buffer using little-endian encoding.
 *
 * @param[in]  aValue   The value to write to buffer.
 * @param[out] aBuffer  Pointer to buffer where the value will be written.
 *
 */
inline void WriteUint64(uint64_t aValue, uint8_t *aBuffer)
{
    aBuffer[0] = (aValue >> 0) & 0xff;
    aBuffer[1] = (aValue >> 8) & 0xff;
    aBuffer[2] = (aValue >> 16) & 0xff;
    aBuffer[3] = (aValue >> 24) & 0xff;
    aBuffer[4] = (aValue >> 32) & 0xff;
    aBuffer[5] = (aValue >> 40) & 0xff;
    aBuffer[6] = (aValue >> 48) & 0xff;
    aBuffer[7] = (aValue >> 56) & 0xff;
}

/**
 * Writes a `UintType` integer value to a given buffer assuming little-endian encoding.
 *
 * @tparam  UintType   The unsigned int type.
 *
 * @param[in] aValue    The value to write to buffer.
 * @param[in] aBuffer   Pointer to the buffer to write to.
 *
 */
template <typename UintType> void Write(UintType aValue, uint8_t *aBuffer);

template <> inline void Write(uint8_t aValue, uint8_t *aBuffer) { *aBuffer = aValue; }
template <> inline void Write(uint16_t aValue, uint8_t *aBuffer) { WriteUint16(aValue, aBuffer); }
template <> inline void Write(uint32_t aValue, uint8_t *aBuffer) { WriteUint32(aValue, aBuffer); }
template <> inline void Write(uint64_t aValue, uint8_t *aBuffer) { WriteUint64(aValue, aBuffer); }

} // namespace LittleEndian

} // namespace ot

#endif // ENCODING_HPP_
