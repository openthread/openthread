/*
 *  Copyright (c) 2022, The OpenThread Authors.
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
 *   This file includes definitions for `FrameData`.
 */

#ifndef OT_CORE_COMMON_FRAME_DATA_HPP_
#define OT_CORE_COMMON_FRAME_DATA_HPP_

#include "openthread-core-config.h"

#include "common/data.hpp"
#include "common/encoding.hpp"
#include "common/type_traits.hpp"

namespace ot {

/**
 * Represents a frame `Data` which is simply a wrapper over a pointer to a buffer with a given frame length.
 *
 * It provide helper method to parse the content. As data is parsed and read, the `FrameData` is updated to skip over
 * the read content.
 */
class FrameData : public Data<kWithUint16Length>
{
public:
    /**
     * Indicates whether or not there are enough bytes remaining in the `FrameData` to read a given number
     * of bytes.
     *
     * @param[in] aLength   The read length to check.
     *
     * @retval TRUE   There are enough remaining bytes to read @p aLength bytes.
     * @retval FALSE  There are not enough remaining bytes to read @p aLength bytes.
     */
    bool CanRead(uint16_t aLength) const { return GetLength() >= aLength; }

    /**
     * Reads an `uint8_t` value from the `FrameData`.
     *
     * If read successfully, the `FrameData` is updated to skip over the read content.
     *
     * @param[out] aUint8   A reference to an `uint8_t` to return the read value.
     *
     * @retval kErrorNone   Successfully read `uint8_t` value and skipped over it.
     * @retval kErrorParse  Not enough bytes remaining to read.
     */
    Error ReadUint8(uint8_t &aUint8);

    /**
     * Reads an integer value with a specified encoding from the `FrameData`.
     *
     * The value is read from the frame data, converted from the specified @p kEncoding byte order to
     * host byte order, and returned in @p aUint.
     *
     * If read successfully, the `FrameData` is updated to skip over the read content.
     *
     * @tparam  kEncoding  The encoding of the integer in the frame data (big or little endian).
     * @tparam  UintType   The unsigned integer type.
     *
     * @param[out] aUint     A reference to the integer to return the read value.
     *
     * @retval kErrorNone    Successfully read the value and skipped over it.
     * @retval kErrorParse   Not enough bytes remaining to read.
     */
    template <Encoding kEncoding, typename UintType> Error ReadUint(UintType &aUint);

    /**
     * Reads a given number of bytes from the `FrameData`.
     *
     * If read successfully, the `FrameData` is updated to skip over the read content.
     *
     * @param[out] aBuffer   The buffer to copy the read bytes into.
     * @param[in]  aLength   Number of bytes to read.
     *
     * @retval kErrorNone   Successfully read @p aLength bytes into @p aBuffer and skipped over them.
     * @retval kErrorParse  Not enough bytes remaining to read @p aLength.
     */
    Error ReadBytes(void *aBuffer, uint16_t aLength);

    /**
     * Reads an object from the `FrameData`.
     *
     * @tparam     ObjectType   The object type to read from the message.
     *
     * @param[out] aObject      A reference to the object to read into.
     *
     * @retval kErrorNone     Successfully read @p aObject and skipped over the read content.
     * @retval kErrorParse    Not enough bytes remaining to read the entire object.
     */
    template <typename ObjectType> Error Read(ObjectType &aObject)
    {
        static_assert(!TypeTraits::IsPointer<ObjectType>::kValue, "ObjectType must not be a pointer");

        return ReadBytes(&aObject, sizeof(ObjectType));
    }

    /**
     * Skips over a given number of bytes from `FrameData`.
     *
     * The caller MUST make sure that the @p aLength is smaller than current data length. Otherwise the behavior of
     * this method is undefined.
     *
     * @param[in] aLength   The length (number of bytes) to skip over.
     */
    void SkipOver(uint16_t aLength);
};

} // namespace ot

#endif // OT_CORE_COMMON_FRAME_DATA_HPP_
