/*
 *  Copyright (c) 2026, The OpenThread Authors.
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
 *   This file implements Beacon frame MAC payload related functions.
 */

#include "mac_beacon.hpp"

#include "common/clearable.hpp"
#include "common/code_utils.hpp"

namespace ot {
namespace Mac {

#if OPENTHREAD_CONFIG_MAC_OUTGOING_BEACON_PAYLOAD_ENABLE || OPENTHREAD_CONFIG_MAC_BEACON_PAYLOAD_PARSING_ENABLE

void Beacon::Init(const MeshCoP::NetworkIdentity &aNetworkIdentity, bool aJoiningPermitted)
{
    ClearAllBytes(*this);

    mSuperframeSpec = LittleEndian::HostSwap16(kSuperFrameSpec);
    mProtocolId     = kProtocolId;

    if (aJoiningPermitted)
    {
        mFlags = kJoiningFlag | (kJoinableProtocolVersion << kVersionOffset);
    }
    else
    {
        mFlags = (kDefaultProtocolVersion << kVersionOffset);
    }

    aNetworkIdentity.GetNetworkName().GetAsData().CopyTo(mNetworkName, sizeof(mNetworkName));
    mExtendedPanId = aNetworkIdentity.GetExtPanId();
}

bool Beacon::IsValid(void) const
{
    bool isValid = false;

    VerifyOrExit(mSuperframeSpec == LittleEndian::HostSwap16(kSuperFrameSpec));
    VerifyOrExit(mGtsSpec == 0);
    VerifyOrExit(mPendingAddressSpec == 0);
    VerifyOrExit(mProtocolId == kProtocolId);
    isValid = true;

exit:
    return isValid;
}

#endif // OPENTHREAD_CONFIG_MAC_OUTGOING_BEACON_PAYLOAD_ENABLE || OPENTHREAD_CONFIG_MAC_BEACON_PAYLOAD_PARSING_ENABLE

} // namespace Mac
} // namespace ot
