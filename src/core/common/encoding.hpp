/*
 *  Copyright (c) 2016, Nest Labs, Inc.
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

#include <limits.h>
#ifndef OPEN_THREAD_DRIVER
#include <stdint.h>
#else
typedef unsigned short     uint16_t;
typedef unsigned int       uint32_t;
typedef unsigned long long uint64_t;
#endif

namespace Thread {
namespace Encoding {

inline uint16_t Swap16(uint16_t v)
{
    return
        ((v & static_cast<uint16_t>(0x00ffU)) << 8) |
        ((v & static_cast<uint16_t>(0xff00U)) >> 8);
}

inline uint32_t Swap32(uint32_t v)
{
    return
        ((v & static_cast<uint32_t>(0x000000ffUL)) << 24) |
        ((v & static_cast<uint32_t>(0x0000ff00UL)) <<  8) |
        ((v & static_cast<uint32_t>(0x00ff0000UL)) >>  8) |
        ((v & static_cast<uint32_t>(0xff000000UL)) >> 24);
}

inline uint64_t Swap64(uint64_t v)
{
    return
        ((v & static_cast<uint64_t>(0x00000000000000ffULL)) << 56) |
        ((v & static_cast<uint64_t>(0x000000000000ff00ULL)) << 40) |
        ((v & static_cast<uint64_t>(0x0000000000ff0000ULL)) << 24) |
        ((v & static_cast<uint64_t>(0x00000000ff000000ULL)) <<  8) |
        ((v & static_cast<uint64_t>(0x000000ff00000000ULL)) >>  8) |
        ((v & static_cast<uint64_t>(0x0000ff0000000000ULL)) >> 24) |
        ((v & static_cast<uint64_t>(0x00ff000000000000ULL)) >> 40) |
        ((v & static_cast<uint64_t>(0xff00000000000000ULL)) >> 56);
}

#define BitVectorBytes(x)           \
    (((x) + (CHAR_BIT-1)) / CHAR_BIT)

namespace BigEndian {

#if BYTE_ORDER_BIG_ENDIAN

inline uint16_t HostSwap16(uint16_t v) { return v; }
inline uint32_t HostSwap32(uint32_t v) { return v; }
inline uint64_t HostSwap64(uint64_t v) { return v; }

#else /* BYTE_ORDER_LITTLE_ENDIAN */

inline uint16_t HostSwap16(uint16_t v) { return Swap16(v); }
inline uint32_t HostSwap32(uint32_t v) { return Swap32(v); }
inline uint64_t HostSwap64(uint64_t v) { return Swap64(v); }

#endif  // LITTLE_ENDIAN

}  // namespace BigEndian

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

}  // namespace LittleEndian
}  // namespace Encoding
}  // namespace Thread

#endif  // ENCODING_HPP_
