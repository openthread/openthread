/*
 *  Copyright (c) 2024, The OpenThread Authors.
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
 *   This file includes definitions for endianness utility functions.
 */

#ifndef LIB_UTILS_ENDIAN_HPP_
#define LIB_UTILS_ENDIAN_HPP_

#include <stdint.h>

namespace ot {
namespace Lib {
namespace Utils {

namespace LittleEndian {
/**
 * Reads a `uint16_t` value from a given buffer assuming little-endian encoding.
 *
 * @param[in] aBuffer   Pointer to buffer to read from.
 *
 * @returns The `uint16_t` value read from buffer.
 */
inline uint16_t ReadUint16(const uint8_t *aBuffer) { return static_cast<uint16_t>(aBuffer[0] | (aBuffer[1] << 8)); }

/**
 * Writes a `uint16_t` value to a given buffer using little-endian encoding.
 *
 * @param[in]  aValue    The value to write to buffer.
 * @param[out] aBuffer   Pointer to buffer where the value will be written.
 */
inline void WriteUint16(uint16_t aValue, uint8_t *aBuffer)
{
    aBuffer[0] = (aValue >> 0) & 0xff;
    aBuffer[1] = (aValue >> 8) & 0xff;
}

} // namespace LittleEndian
} // namespace Utils
} // namespace Lib
} // namespace ot

#endif // LIB_UTILS_ENDIAN_HPP_
