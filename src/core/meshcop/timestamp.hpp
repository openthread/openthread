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

/**
 * Implements Timestamp generation and parsing.
 *
 */
OT_TOOL_PACKED_BEGIN
class Timestamp : public Clearable<Timestamp>
{
public:
    /**
     * Converts the timestamp to `otTimestamp`.
     *
     */
    void ConvertTo(otTimestamp &aTimestamp) const;

    /**
     * Sets the timestamp from `otTimestamp`.
     *
     */
    void SetFromTimestamp(const otTimestamp &aTimestamp);

    /**
     * Returns the Seconds value.
     *
     * @returns The Seconds value.
     *
     */
    uint64_t GetSeconds(void) const
    {
        return (static_cast<uint64_t>(BigEndian::HostSwap16(mSeconds16)) << 32) + BigEndian::HostSwap32(mSeconds32);
    }

    /**
     * Sets the Seconds value.
     *
     * @param[in]  aSeconds  The Seconds value.
     *
     */
    void SetSeconds(uint64_t aSeconds)
    {
        mSeconds16 = BigEndian::HostSwap16(static_cast<uint16_t>(aSeconds >> 32));
        mSeconds32 = BigEndian::HostSwap32(static_cast<uint32_t>(aSeconds & 0xffffffff));
    }

    /**
     * Returns the Ticks value.
     *
     * @returns The Ticks value.
     *
     */
    uint16_t GetTicks(void) const { return BigEndian::HostSwap16(mTicks) >> kTicksOffset; }

    /**
     * Sets the Ticks value.
     *
     * @param[in]  aTicks  The Ticks value.
     *
     */
    void SetTicks(uint16_t aTicks)
    {
        mTicks = BigEndian::HostSwap16((BigEndian::HostSwap16(mTicks) & ~kTicksMask) |
                                       ((aTicks << kTicksOffset) & kTicksMask));
    }

    /**
     * Returns the Authoritative value.
     *
     * @returns The Authoritative value.
     *
     */
    bool GetAuthoritative(void) const { return (BigEndian::HostSwap16(mTicks) & kAuthoritativeMask) != 0; }

    /**
     * Sets the Authoritative value.
     *
     * @param[in]  aAuthoritative  The Authoritative value.
     *
     */
    void SetAuthoritative(bool aAuthoritative)
    {
        mTicks = BigEndian::HostSwap16((BigEndian::HostSwap16(mTicks) & kTicksMask) |
                                       ((aAuthoritative << kAuthoritativeOffset) & kAuthoritativeMask));
    }

    /**
     * Increments the timestamp by a random number of ticks [0, 32767].
     *
     */
    void AdvanceRandomTicks(void);

    /**
     * Indicates whether the timestamp indicates an MLE Orphan Announce message.
     *
     * @retval TRUE   The timestamp indicates an Orphan Announce message.
     * @retval FALSE  If the timestamp does not indicate an Orphan Announce message.
     *
     */
    bool IsOrphanTimestamp(void) const { return GetSeconds() == 0 && GetTicks() == 0 && GetAuthoritative(); }

    /**
     * Compares two timestamps.
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
     * Compares two timestamps.
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
    static constexpr uint8_t  kTicksOffset         = 1;
    static constexpr uint16_t kTicksMask           = 0x7fff << kTicksOffset;
    static constexpr uint16_t kMaxRandomTicks      = 0x7fff;
    static constexpr uint8_t  kAuthoritativeOffset = 0;
    static constexpr uint16_t kAuthoritativeMask   = 1 << kAuthoritativeOffset;

    uint16_t mSeconds16; // bits 32-47
    uint32_t mSeconds32; // bits 0-31
    uint16_t mTicks;
} OT_TOOL_PACKED_END;

} // namespace MeshCoP
} // namespace ot

#endif // MESHCOP_TIMESTAMP_HPP_
