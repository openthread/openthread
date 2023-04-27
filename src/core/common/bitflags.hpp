/*
 *  Copyright (c) 2023, The OpenThread Authors.
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
 *   This file includes definitions for a bit-vector.
 */

#ifndef BITFLAGS_HPP_
#define BITFLAGS_HPP_

#include "openthread-core-config.h"

#include <type_traits>

#include "common/clearable.hpp"
#include "common/code_utils.hpp"
#include "common/debug.hpp"
#include "common/encoding.hpp"
#include "common/equatable.hpp"

namespace ot {

/**
 * @addtogroup core-bitflags
 *
 * @brief
 *   This module includes definitions for bitflags.
 *
 * @{
 *
 */

/**
 * This class represents a bitflags.
 *
 * Note: The value of the enum is the index of the bit set, numbered from 0.
 *
 * @tparam EnumType  Specifies the underlying enum.
 *
 */
template <typename EnumType, typename std::enable_if<std::is_enum<EnumType>::value, int>::type = 0>
class BitFlags : public Equatable<BitFlags<EnumType>>, public Clearable<BitFlags<EnumType>>
{
public:
    using BaseType = typename std::underlying_type<EnumType>::type;

    BitFlags()
        : mBits(0)
    {
    }

    template <typename... T0> explicit BitFlags(T0... aVals) { mBits = BuildBits(GetBit(aVals)...); }

    /**
     * This method indicates whether all of the given bits are set in the bit flags.
     *
     * @param[in] aVals  The bits.
     *
     * @retval TRUE   If the given bits are all set.
     * @retval FALSE  If the given bits are not all set.
     *
     */
    template <typename... T0> bool HasAll(T0... aVals)
    {
        BaseType expectedBits = BuildBits(GetBit(aVals)...);

        return (mBits & expectedBits) == expectedBits;
    }

    /**
     * This method sets the given bits.
     *
     * @param[in] aVals  The bits.
     *
     */
    template <typename... T0> void Set(T0... aVals) { mBits |= BuildBits(GetBit(aVals)...); }

    /**
     * This method unsets the given bits.
     *
     * @param[in] aVals  The bits.
     *
     */
    template <typename... T0> void Unset(T0... aVals) { mBits &= ~BuildBits(GetBit(aVals)...); }

    /**
     * This method indicates whether any of the given bit is set in the bit flags.
     *
     * @param[in] aVals  The bits.
     *
     * @retval TRUE   If any of the given bits is all set.
     * @retval FALSE  If none of given bits are all set.
     *
     */
    template <typename... T0> bool HasAny(T0... aVals) { return (mBits & BuildBits(GetBit(aVals)...)) != 0; }

    /**
     * This method indicates whether all of and only the given flags are set.
     *
     * @param[in] aVals  The bits.
     *
     * @retval TRUE   If the given bits are all set.
     * @retval FALSE  If the given bits are not all set.
     *
     */
    template <typename... T0> bool HasExactly(T0... aVals) { return mBits == BuildBits(GetBit(aVals)...); }

    /**
     * This method indicates whether none of the given flags are set.
     *
     * @param[in] aVals  The bits.
     *
     * @retval TRUE   If the given bits are all set.
     * @retval FALSE  If the given bits are not all set.
     *
     */
    template <typename... T0> bool HasNone(T0... aVals) { return (mBits & BuildBits(GetBit(aVals)...)) == 0; }

    /**
     * This method returns the raw value of the bit flags.
     *
     * Note: This is for testing purpose only.
     *
     * @returns The raw value of the bit flag.
     *
     */
    BaseType GetRaw(void) { return mBits; }

    /**
     * This method sets the raw value of the bit flags.
     *
     * Note: This is for testing purpose only.
     *
     * @param[in] The raw value of the bit flag.
     *
     */
    void SetRaw(BaseType aVal) { mBits = aVal; }

private:
    // A series of helper functions got actual calculation.

    static BaseType GetBit(EnumType aVal) { return static_cast<BaseType>(1) << static_cast<BaseType>(aVal); };

    template <typename T1, typename... T0> static T1 BuildBits(T1 aVal, T0... aRemaining)
    {
        return aVal | BuildBits(aRemaining...);
    }

    template <typename T1> static T1 BuildBits(T1 aVal) { return aVal; }

    BaseType mBits;
};

/**
 * @}
 *
 */

} // namespace ot

#endif // BITFLAGS_HPP_
