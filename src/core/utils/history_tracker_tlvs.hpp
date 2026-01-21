/*
 *  Copyright (c) 2025, The OpenThread Authors.
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
 *   This file includes definitions to support History Tracker TLVs.
 */

#ifndef OT_CORE_UTILS_HISTORY_TRACKER_TLVS_HPP_
#define OT_CORE_UTILS_HISTORY_TRACKER_TLVS_HPP_

#include "openthread-core-config.h"

#if OPENTHREAD_CONFIG_HISTORY_TRACKER_ENABLE

#include <openthread/history_tracker.h>

#include "common/encoding.hpp"
#include "common/tlvs.hpp"
#include "utils/history_tracker.hpp"

namespace ot {
namespace HistoryTracker {

/**
 * Implements History Tracker TLV generation and parsing.
 */
OT_TOOL_PACKED_BEGIN
class Tlv : public ot::Tlv
{
public:
    /**
     * History Tracker TLV Types.
     */
    enum Type : uint8_t
    {
        kQueryId     = 0,
        kAnswer      = 1,
        kRequest     = 2,
        kNetworkInfo = 3,
    };
} OT_TOOL_PACKED_END;

/**
 * Defines Query ID TLV constants and types.
 */
typedef UintTlvInfo<Tlv::kQueryId, uint16_t> QueryIdTlv;

/**
 * Implements Answer TLV generation and parsing.
 */
OT_TOOL_PACKED_BEGIN
class AnswerTlv : public Tlv, public TlvInfo<Tlv::kAnswer>
{
public:
    /**
     * Initializes the TLV.
     *
     * @param[in] aIndex   The index value.
     * @param[in] aIsLast  The "IsLast" flag value.
     */
    void Init(uint16_t aIndex, bool aIsLast);

    /**
     * Indicates whether or not the "IsLast" flag is set
     *
     * @retval TRUE   "IsLast" flag is set (this is the last answer for this query).
     * @retval FALSE  "IsLast" flag is not set (more answer messages are expected for this query).
     */
    bool IsLast(void) const { return GetFlagsIndex() & kIsLastFlag; }

    /**
     * Gets the index.
     *
     * @returns The index.
     */
    uint16_t GetIndex(void) const { return GetFlagsIndex() & kIndexMask; }

private:
    static constexpr uint16_t kIsLastFlag = 1 << 15;
    static constexpr uint16_t kIndexMask  = 0x7fff;

    uint16_t GetFlagsIndex(void) const { return BigEndian::HostSwap16(mFlagsIndex); }
    void     SetFlagsIndex(uint16_t aFlagsIndex) { mFlagsIndex = BigEndian::HostSwap16(aFlagsIndex); }

    uint16_t mFlagsIndex;
} OT_TOOL_PACKED_END;

/**
 * Implements Request TLV generation and parsing.
 */
OT_TOOL_PACKED_BEGIN
class RequestTlv : public Tlv, public TlvInfo<Tlv::kRequest>
{
public:
    /**
     * Initializes the TLV.
     *
     * @param[in] aTlvType      The TLV type to request.
     * @param[in] aNumEntries   Maximum number of entries to include in the reply (zero indicates all).
     * @param[in] aMaxEntryAge  Maximum entry age to include in the reply (zero indicates no limit).
     */
    void Init(uint8_t aTlvType, uint16_t aNumEntries, uint32_t aMaxEntryAge);

    /**
     * Indicates whether or not the TLV appears to be well-formed.
     *
     * @retval TRUE   If the TLV appears to be well-formed.
     * @retval FALSE  If the TLV does not appear to be well-formed.
     */
    bool IsValid(void) const { return GetLength() >= sizeof(*this) - sizeof(Tlv); }

    /**
     * Gets the requested TLV type.
     *
     * @returns The requested TLV type.
     */
    uint8_t GetTlvType(void) const { return mTlvType; }

    /**
     * Gets the maximum number of entries to include in the reply.
     *
     * @returns The maximum number of entries (zero indicates all).
     */
    uint16_t GetNumEntries(void) const { return BigEndian::HostSwap16(mNumEntries); }

    /**
     * Gets the maximum entry age to include in the reply.
     *
     * @returns The maximum entry age in milliseconds (zero indicates no age limit).
     */
    uint32_t GetMaxEntryAge(void) const { return BigEndian::HostSwap32(mMaxEntryAge); }

private:
    uint8_t  mTlvType;
    uint16_t mNumEntries;
    uint32_t mMaxEntryAge;
} OT_TOOL_PACKED_END;

/**
 * Implements Network Info TLV generation and parsing.
 */
OT_TOOL_PACKED_BEGIN
class NetworkInfoTlv : public Tlv, public TlvInfo<Tlv::kNetworkInfo>
{
public:
    /**
     * Initializes the TLV from a `NetworkInfo` object and entry age.
     *
     * @param[in] aNetworkInfo  The NetworkInfo object to initialize from.
     * @param[in] aEntryAge     The age of the entry in milliseconds.
     */
    void InitFrom(const NetworkInfo &aNetworkInfo, uint32_t aEntryAge);

    /**
     * Indicates whether or not the TLV appears to be well-formed.
     *
     * @retval TRUE   If the TLV appears to be well-formed.
     * @retval FALSE  If the TLV does not appear to be well-formed.
     */
    bool IsValid(void) const { return GetLength() >= sizeof(*this) - sizeof(Tlv); }

    /**
     * Copies the TLV data to a `NetworkInfo` object.
     *
     * @param[out] aNetworkInfo    The `NetworkInfo` object to copy data to.
     */
    void CopyTo(NetworkInfo &aNetworkInfo) const;

    /**
     * Gets the entry age.
     *
     * @returns The entry age in milliseconds.
     */
    uint32_t GetEntryAge(void) const { return BigEndian::HostSwap32(mEntryAge); }

private:
    uint32_t mEntryAge;
    uint8_t  mRole;
    uint8_t  mMode;
    uint16_t mRloc16;
    uint32_t mPartitionId;
} OT_TOOL_PACKED_END;

} // namespace HistoryTracker
} // namespace ot

#endif // OPENTHREAD_CONFIG_HISTORY_TRACKER_ENABLE

#endif // OT_CORE_UTILS_HISTORY_TRACKER_TLVS_HPP_
