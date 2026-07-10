/*
 *  Copyright (c) 2016-2026, The OpenThread Authors.
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
 *   This file includes definitions for generating and processing of Beacon frame MAC payload.
 */

#ifndef OT_CORE_MAC_MAC_BEACON_HPP_
#define OT_CORE_MAC_MAC_BEACON_HPP_

#include "openthread-core-config.h"

#include "common/encoding.hpp"
#include "meshcop/extended_panid.hpp"
#include "meshcop/network_identity.hpp"
#include "meshcop/network_name.hpp"

namespace ot {
namespace Mac {

/**
 * @addtogroup core-mac
 *
 * @{
 */

#if OPENTHREAD_CONFIG_MAC_OUTGOING_BEACON_PAYLOAD_ENABLE || OPENTHREAD_CONFIG_MAC_BEACON_PAYLOAD_PARSING_ENABLE

/**
 * Implements IEEE 802.15.4 Beacon frame MAC payload generation and parsing.
 *
 * Thread 1.2.1 and later specifications deprecated Thread Beacon payload in favor of MLE Discover/Active Scan.
 * Support for generating and parsing Beacon payloads is retained under OpenThread configuration options
 * `OPENTHREAD_CONFIG_MAC_OUTGOING_BEACON_PAYLOAD_ENABLE` and `OPENTHREAD_CONFIG_MAC_BEACON_PAYLOAD_PARSING_ENABLE`
 * for backward compatibility.
 */
OT_TOOL_PACKED_BEGIN
class Beacon
{
public:
    /**
     * Initializes the Beacon MAC payload.
     *
     * @param[in] aNetworkIdentity   The network identity (network name and extended PAN ID) to include in the beacon.
     * @param[in] aJoiningPermitted  TRUE to set the joining permitted flag, FALSE to clear.
     */
    void Init(const MeshCoP::NetworkIdentity &aNetworkIdentity, bool aJoiningPermitted);

    /**
     * Indicates whether or not the beacon appears to be a valid Thread Beacon.
     *
     * @retval TRUE   If the beacon appears to be a valid Thread Beacon.
     * @retval FALSE  If the beacon does not appear to be a valid Thread Beacon.
     */
    bool IsValid(void) const;

    /**
     * Returns the Protocol Version value.
     *
     * @returns The Protocol Version value.
     */
    uint8_t GetProtocolVersion(void) const { return mFlags >> kVersionOffset; }

    /**
     * Indicates whether or not the Native Commissioner flag is set.
     *
     * @retval TRUE   If the Native Commissioner flag is set.
     * @retval FALSE  If the Native Commissioner flag is not set.
     */
    bool IsNative(void) const { return (mFlags & kNativeFlag) != 0; }

    /**
     * Indicates whether or not the Joining Permitted flag is set.
     *
     * @retval TRUE   If the Joining Permitted flag is set.
     * @retval FALSE  If the Joining Permitted flag is not set.
     */
    bool IsJoiningPermitted(void) const { return (mFlags & kJoiningFlag) != 0; }

    /**
     * Gets the Network Name field.
     *
     * @returns The Network Name field as `NameData`.
     */
    MeshCoP::NameData GetNetworkName(void) const { return MeshCoP::NameData(mNetworkName, sizeof(mNetworkName)); }

    /**
     * Returns the Extended PAN ID field.
     *
     * @returns The Extended PAN ID field.
     */
    const MeshCoP::ExtendedPanId &GetExtendedPanId(void) const { return mExtendedPanId; }

private:
    static constexpr uint16_t kSuperFrameSpec = 0x0fff;
    static constexpr uint8_t  kProtocolId     = 3;
    static constexpr uint8_t  kVersionOffset  = 4;
    static constexpr uint8_t  kVersionMask    = 0xf << kVersionOffset;
    static constexpr uint8_t  kNativeFlag     = 1 << 3;
    static constexpr uint8_t  kJoiningFlag    = 1 << 0;

    static constexpr uint8_t kDefaultProtocolVersion  = 2;
    static constexpr uint8_t kJoinableProtocolVersion = OPENTHREAD_CONFIG_MAC_JOIN_BEACON_VERSION;

    uint16_t               mSuperframeSpec;
    uint8_t                mGtsSpec;
    uint8_t                mPendingAddressSpec;
    uint8_t                mProtocolId;
    uint8_t                mFlags;
    char                   mNetworkName[MeshCoP::NetworkName::kMaxSize];
    MeshCoP::ExtendedPanId mExtendedPanId;
} OT_TOOL_PACKED_END;

#endif // OPENTHREAD_CONFIG_MAC_OUTGOING_BEACON_PAYLOAD_ENABLE || OPENTHREAD_CONFIG_MAC_BEACON_PAYLOAD_PARSING_ENABLE

/**
 * @}
 */

} // namespace Mac
} // namespace ot

#endif // OT_CORE_MAC_MAC_BEACON_HPP_
