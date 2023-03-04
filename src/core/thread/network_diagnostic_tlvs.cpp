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
 *   This file implements methods for generating and processing Network Diagnostics TLVs.
 */

#include "network_diagnostic_tlvs.hpp"

#include "common/time.hpp"

namespace ot {
namespace NetworkDiagnostic {

using ot::Encoding::BigEndian::HostSwap16;
using ot::Encoding::BigEndian::HostSwap32;

#if OPENTHREAD_FTD

void ChildTlv::InitFrom(const Child &aChild)
{
    Clear();

    SetType(kChild);
    SetLength(sizeof(*this) - sizeof(Tlv));

    mFlags |= aChild.IsRxOnWhenIdle() ? kFlagsRxOnWhenIdle : 0;
    mFlags |= aChild.IsFullThreadDevice() ? kFlagsFtd : 0;
    mFlags |= (aChild.GetNetworkDataType() == NetworkData::kFullSet) ? kFlagsFullNetdta : 0;
    mFlags |= kFlagsTrackErrRate;

    mRloc16              = HostSwap16(aChild.GetRloc16());
    mExtAddress          = aChild.GetExtAddress();
    mVersion             = HostSwap16(aChild.GetVersion());
    mTimeout             = HostSwap32(aChild.GetTimeout());
    mAge                 = HostSwap32(Time::MsecToSec(TimerMilli::GetNow() - aChild.GetLastHeard()));
    mSupervisionInterval = HostSwap16(aChild.GetSupervisionInterval());
    mAverageRssi         = aChild.GetLinkInfo().GetAverageRss();
    mLastRssi            = aChild.GetLinkInfo().GetLastRss();
    mFrameErrorRate      = HostSwap16(aChild.GetLinkInfo().GetFrameErrorRate());
    mMessageErrorRate    = HostSwap16(aChild.GetLinkInfo().GetMessageErrorRate());
    mQueuedMessageCount  = HostSwap16(aChild.GetIndirectMessageCount());

#if OPENTHREAD_CONFIG_MAC_CSL_TRANSMITTER_ENABLE
    mFlags |= aChild.IsCslSynchronized() ? kFlagsCslSync : 0;
    mCslPeriod  = HostSwap16(aChild.GetCslPeriod());
    mCslTimeout = HostSwap32(aChild.GetCslTimeout());
#endif
}

#endif // OPENTHREAD_FTD

} // namespace NetworkDiagnostic
} // namespace ot
