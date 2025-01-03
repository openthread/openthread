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
 */
OT_TOOL_PACKED_BEGIN
class Timestamp : public Clearable<Timestamp>
{
public:
    /**
     * Represents timestamp components.
     */
    typedef otTimestamp Info;

    /**
     * Copies the `Timestamp` information to  `Timestamp::Info` data structure.
     *
     * @param[out] aInfo   A reference to a `Timestamp::Info` to populate.
     */
    void ConvertTo(Info &aInfo) const;

    /**
     * Sets the `Timestamp` from a given component-wise `Info` structure.
     *
     * @param[in] aInfo    A `Timestamp::Info` structure.
     */
    void SetFrom(const Info &aInfo);

    /**
     * Sets the `Timestamp` to invalid value.
     */
    void SetToInvalid(void);

    /**
     * Indicates whether or not the `Timestamp` is valid.
     *
     * @retval TRUE   The timestamp is valid.
     * @retval FALSE  The timestamp is not valid.
     */
    bool IsValid(void) const;

    /**
     * Sets the `Timestamp` to value used in MLE Orphan Announce messages.
     *
     * Second and ticks fields are set to zero with Authoritative flag set.
     */
    void SetToOrphanAnnounce(void);

    /**
     * Indicates whether the timestamp indicates an MLE Orphan Announce message.
     *
     * @retval TRUE   The timestamp indicates an Orphan Announce message.
     * @retval FALSE  The timestamp does not indicate an Orphan Announce message.
     */
    bool IsOrphanAnnounce(void) const;

    /**
     * Returns the Seconds value.
     *
     * @returns The Seconds value.
     */
    uint64_t GetSeconds(void) const;

    /**
     * Sets the Seconds value.
     *
     * @param[in]  aSeconds  The Seconds value.
     */
    void SetSeconds(uint64_t aSeconds);

    /**
     * Returns the Ticks value.
     *
     * @returns The Ticks value.
     */
    uint16_t GetTicks(void) const { return GetTicksAndAuthFlag() >> kTicksOffset; }

    /**
     * Sets the Ticks value.
     *
     * @param[in]  aTicks  The Ticks value.
     */
    void SetTicks(uint16_t aTicks);

    /**
     * Returns the Authoritative value.
     *
     * @returns The Authoritative value.
     */
    bool GetAuthoritative(void) const { return (GetTicksAndAuthFlag() & kAuthoritativeFlag) != 0; }

    /**
     * Sets the Authoritative value.
     *
     * @param[in]  aAuthoritative  The Authoritative value.
     */
    void SetAuthoritative(bool aAuthoritative);

    /**
     * Increments the timestamp by a random number of ticks [0, 32767].
     */
    void AdvanceRandomTicks(void);

    /**
     * Compares two timestamps.
     *
     * Either one or both @p aFirst or @p aSecond can be invalid. A valid timestamp is considered greater than an
     * invalid one. If both are invalid, they are considered as equal.
     *
     * @param[in]  aFirst   A reference to the first timestamp to compare.
     * @param[in]  aSecond  A reference to the second timestamp to compare.
     *
     * @retval -1  if @p aFirst is less than @p aSecond (`aFirst < aSecond`).
     * @retval  0  if @p aFirst is equal to @p aSecond (`aFirst == aSecond`).
     * @retval  1  if @p aFirst is greater than @p aSecond (`aFirst > aSecond`).
     */
    static int Compare(const Timestamp &aFirst, const Timestamp &aSecond);

    // Comparison operator overloads for two `Timestamp` instances.
    bool operator==(const Timestamp &aOther) const { return Compare(*this, aOther) == 0; }
    bool operator!=(const Timestamp &aOther) const { return Compare(*this, aOther) != 0; }
    bool operator>(const Timestamp &aOther) const { return Compare(*this, aOther) > 0; }
    bool operator<(const Timestamp &aOther) const { return Compare(*this, aOther) < 0; }
    bool operator>=(const Timestamp &aOther) const { return Compare(*this, aOther) >= 0; }
    bool operator<=(const Timestamp &aOther) const { return Compare(*this, aOther) <= 0; }

private:
    uint16_t GetTicksAndAuthFlag(void) const { return BigEndian::HostSwap16(mTicksAndAuthFlag); }
    void     SetTicksAndAuthFlag(uint16_t aValue) { mTicksAndAuthFlag = BigEndian::HostSwap16(aValue); }

    static constexpr uint8_t  kTicksOffset         = 1;
    static constexpr uint16_t kTicksMask           = 0x7fff << kTicksOffset;
    static constexpr uint8_t  kAuthoritativeOffset = 0;
    static constexpr uint16_t kAuthoritativeFlag   = 1 << kAuthoritativeOffset;
    static constexpr uint16_t kMaxTicks            = 0x7fff;

    uint16_t mSeconds16; // bits 32-47
    uint32_t mSeconds32; // bits 0-31
    uint16_t mTicksAndAuthFlag;
} OT_TOOL_PACKED_END;

} // namespace MeshCoP
} // namespace ot

#endif // MESHCOP_TIMESTAMP_HPP_
