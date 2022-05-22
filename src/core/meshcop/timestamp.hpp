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
 *   This file includes definitions for manipulating MeshCoP timestamps.
 *
 */

#ifndef MESHCOP_TIMESTAMP_HPP_
#define MESHCOP_TIMESTAMP_HPP_

#include "openthread-core-config.h"

#include <string.h>

#include <openthread/dataset.h>
#include <openthread/platform/toolchain.h>

#include "common/clearable.hpp"
#include "common/encoding.hpp"
#include "common/random.hpp"

namespace ot {
namespace MeshCoP {

using ot::Encoding::BigEndian::HostSwap64;

/**
 * This class implements Timestamp generation and parsing.
 *
 */
OT_TOOL_PACKED_BEGIN
class Timestamp : public Clearable<Timestamp>
{
public:
    Timestamp(void) { Clear(); } // Suppress warning of accessing uninitialzed mValue

    /**
     * This method returns the timestamp as `otTimestamp`.
     *
     */
    otTimestamp GetTimestamp(void) const
    {
        otTimestamp timestamp;

        timestamp.mSeconds       = GetSeconds();
        timestamp.mTicks         = GetTicks();
        timestamp.mAuthoritative = GetAuthoritative();
        return timestamp;
    }

    /**
     * This method sets the timestamp from `otTimestamp`.
     *
     */
    void SetFromTimestamp(otTimestamp aTimestamp)
    {
        SetSeconds(aTimestamp.mSeconds);
        SetTicks(aTimestamp.mTicks);
        SetAuthoritative(aTimestamp.mAuthoritative);
    }

    /**
     * This method returns the Seconds value.
     *
     * @returns The Seconds value.
     *
     */
    uint64_t GetSeconds(void) const { return (HostSwap64(mValue) & kSecondsMask) >> kSecondsOffset; }

    /**
     * This method sets the Seconds value.
     *
     * @param[in]  aSeconds  The Seconds value.
     *
     */
    void SetSeconds(uint64_t aSeconds)
    {
        mValue = HostSwap64((HostSwap64(mValue) & ~kSecondsMask) | ((aSeconds << kSecondsOffset) & kSecondsMask));
    }

    /**
     * This method returns the Ticks value.
     *
     * @returns The Ticks value.
     *
     */
    uint16_t GetTicks(void) const { return (HostSwap64(mValue) & kTicksMask) >> kTicksOffset; }

    /**
     * This method sets the Ticks value.
     *
     * @param[in]  aTicks  The Ticks value.
     *
     */
    void SetTicks(uint16_t aTicks)
    {
        mValue = HostSwap64((HostSwap64(mValue) & ~kTicksMask) |
                            ((static_cast<uint64_t>(aTicks) << kTicksOffset) & kTicksMask));
    }

    /**
     * This method returns the Authoritative value.
     *
     * @returns The Authoritative value.
     *
     */
    bool GetAuthoritative(void) const { return (HostSwap64(mValue) & kAuthoritativeMask) != 0; }

    /**
     * This method sets the Authoritative value.
     *
     * @param[in]  aAuthoritative  The Authoritative value.
     *
     */
    void SetAuthoritative(bool aAuthoritative)
    {
        mValue = HostSwap64((HostSwap64(mValue) & ~kAuthoritativeMask) |
                            ((static_cast<uint64_t>(aAuthoritative) << kAuthoritativeOffset) & kAuthoritativeMask));
    }

    /**
     * This method increments the timestamp by a random number of ticks [0, 32767].
     *
     */
    void AdvanceRandomTicks(void);

    /**
     * This static method compares two timestamps.
     *
     * Either one or both @p aFirst or @p aSecond can be `nullptr`. A non-null timestamp is considered greater than
     * a null one. If both are null, they are considered as equal.
     *
     * @param[in]  aFirst   A pointer to the first timestamp to compare (can be nullptr).
     * @param[in]  aSecond  A pointer to the second timestamp to compare (can be nullptr).
     *
     * @retval -1  if @p aFirst is less than @p aSecond (`aFirst < aSecond`).
     * @retval  0  if @p aFirst is equal to @p aSecond (`aFirst == aSecond`).
     * @retval  1  if @p aFirst is greater than @p aSecond (`aFirst > aSecond`).
     *
     */
    static int Compare(const Timestamp *aFirst, const Timestamp *aSecond);

    /**
     * This static method compares two timestamps.
     *
     * @param[in]  aFirst   A reference to the first timestamp to compare.
     * @param[in]  aSecond  A reference to the second timestamp to compare.
     *
     * @retval -1  if @p aFirst is less than @p aSecond (`aFirst < aSecond`).
     * @retval  0  if @p aFirst is equal to @p aSecond (`aFirst == aSecond`).
     * @retval  1  if @p aFirst is greater than @p aSecond (`aFirst > aSecond`).
     *
     */
    static int Compare(const Timestamp &aFirst, const Timestamp &aSecond);

private:
    static constexpr uint8_t  kSecondsOffset       = 16;
    static constexpr uint64_t kSecondsMask         = 0xffffffffffffULL << kSecondsOffset;
    static constexpr uint8_t  kTicksOffset         = 1;
    static constexpr uint64_t kTicksMask           = 0x7fffULL << kTicksOffset;
    static constexpr uint16_t kMaxRandomTicks      = 0x7fff;
    static constexpr uint8_t  kAuthoritativeOffset = 0;
    static constexpr uint64_t kAuthoritativeMask   = 1ULL << kAuthoritativeOffset;

    uint64_t mValue;
} OT_TOOL_PACKED_END;

} // namespace MeshCoP
} // namespace ot

#endif // MESHCOP_TIMESTAMP_HPP_
