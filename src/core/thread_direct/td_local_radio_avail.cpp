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
 *   This file includes implementation of Thread Direct Local Radio Availability.
 */

#include "td_local_radio_avail.hpp"

#if OT_CONFIG_THREAD_DIRECT_ENABLE

#include "instance/instance.hpp"

namespace ot {
namespace ThreadDirect {

bool LocalRadioAvailability::IsPreferred(uint8_t aBitIndex) const
{
    return mAvailability.IsAvailable(aBitIndex) && RadioAvailability::GetBit(aBitIndex, mPreferredBitmap);
}

bool LocalRadioAvailability::IsPreferredAt(Radio::Time64 aTime) const
{
    bool                        isPreferred = true;
    RadioAvailability::SlotInfo slotInfo;

    VerifyOrExit(mAvailability.IsSpecified());

    mAvailability.DetermineSlotInfo(aTime, slotInfo);
    isPreferred = IsPreferred(slotInfo.mIndex);

exit:
    return isPreferred;
}

void LocalRadioAvailability::Update(void)
{
    Radio::Availability radioAvail;

    if ((Get<Radio::Radio>().GetAvailability(radioAvail) != kErrorNone) || (radioAvail.mNumSlots == 0))
    {
        mAvailability.Clear();
        ClearAllBytes(mPreferredBitmap);
        ExitNow();
    }

    VerifyOrExit(IsSlotDurationValid(radioAvail.mSlotDuration));

    SuccessOrExit(mAvailability.Init(radioAvail.mStartTime, MapEnum(radioAvail.mSlotDuration), radioAvail.mNumSlots,
                                     radioAvail.mAvailableSlots));
    memcpy(mPreferredBitmap, radioAvail.mPreferredSlots, RadioAvailability::kMaxBitmapSize);

exit:
    return;
}

} // namespace ThreadDirect
} // namespace ot

#endif // OT_CONFIG_THREAD_DIRECT_ENABLE
