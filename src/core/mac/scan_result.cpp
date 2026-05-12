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
 *   This file includes implementation of methods in `ScanResult`.
 */

#include "scan_result.hpp"

namespace ot {

Error ScanResult::PopulateFromBeacon(const Mac::RxFrame *aBeaconFrame)
{
    Error        error = kErrorNone;
    Mac::Address address;

    Clear();

    VerifyOrExit(aBeaconFrame != nullptr, error = kErrorInvalidArgs);

    VerifyOrExit(aBeaconFrame->GetType() == Mac::Frame::kTypeBeacon, error = kErrorParse);

    SuccessOrExit(error = aBeaconFrame->GetSrcAddr(address));
    VerifyOrExit(address.IsExtended(), error = kErrorParse);
    mExtAddress = address.GetExtended();

    if (aBeaconFrame->GetSrcPanId(mPanId) != kErrorNone)
    {
        IgnoreError(aBeaconFrame->GetDstPanId(mPanId));
    }

    mChannel = aBeaconFrame->GetChannel();
    mRssi    = aBeaconFrame->GetRssi();
    mLqi     = aBeaconFrame->GetLqi();

#if OPENTHREAD_CONFIG_MAC_BEACON_PAYLOAD_PARSING_ENABLE
    {
        const Mac::Beacon        *beacon;
        const Mac::BeaconPayload *beaconPayload;

        VerifyOrExit(aBeaconFrame->GetPayloadLength() >= sizeof(Mac::Beacon) + sizeof(Mac::BeaconPayload));

        beacon = reinterpret_cast<const Mac::Beacon *>(aBeaconFrame->GetPayload());
        VerifyOrExit(beacon->IsValid());

        beaconPayload = reinterpret_cast<const Mac::BeaconPayload *>(beacon->GetPayload());
        VerifyOrExit(beaconPayload->IsValid());

        mVersion       = beaconPayload->GetProtocolVersion();
        mIsJoinable    = beaconPayload->IsJoiningPermitted();
        mIsNative      = beaconPayload->IsNative();
        mExtendedPanId = beaconPayload->GetExtendedPanId();

        IgnoreError(AsCoreType(&mNetworkName).Set(beaconPayload->GetNetworkName()));
        VerifyOrExit(IsValidUtf8String(mNetworkName.m8), error = kErrorParse);
    }
#endif

exit:
    return error;
}

} // namespace ot
