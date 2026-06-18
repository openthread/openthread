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
 *   This file implements the Thread Direct handler.
 */

#include "direct_handler.hpp"

#if OPENTHREAD_CONFIG_THREAD_DIRECT_WAKE_INITIATOR_ENABLE || OPENTHREAD_CONFIG_THREAD_DIRECT_WAKE_LISTENER_ENABLE

#include <string.h>

#include "common/code_utils.hpp"
#include "common/log.hpp"
#include "config/thread_direct.h"

namespace ot {

RegisterLogModule("DirectHandler");

DirectHandler::DirectHandler(Instance &aInstance)
    : InstanceLocator(aInstance)
    , mSlwTimeout(kDefaultSlwTimeout)
{
    memset(&mLocalSca, 0, sizeof(mLocalSca));
    mLocalSca.mRamDuration = Mac::ScaParams::kRamDurationNoConstraints;
}

Error DirectHandler::SetSlwSchedule(uint16_t aPeriodSlots)
{
    Error error = kErrorNone;

    if (aPeriodSlots != 0)
    {
        VerifyOrExit(aPeriodSlots >= OPENTHREAD_CONFIG_THREAD_DIRECT_SLW_MIN_DURATION_SLOTS, error = kErrorInvalidArgs);
    }

    mLocalSca.mSlwPeriodSlots = aPeriodSlots;
    mLocalSca.mSlwPhaseSlots  = 0; // Phase is stack-computed at frame-build time.

exit:
    return error;
}

void DirectHandler::GetSlwSchedule(uint16_t &aPeriodSlots) const { aPeriodSlots = mLocalSca.mSlwPeriodSlots; }

Error DirectHandler::SetRamMask(const Mac::ScaParams &aParams)
{
    Error error = kErrorNone;

    VerifyOrExit(aParams.mRamDuration <= Mac::ScaParams::kRamDurationMax, error = kErrorInvalidArgs);
    VerifyOrExit(aParams.mRamOffsetUs >= Mac::ScaParams::kRamOffsetUsMin &&
                     aParams.mRamOffsetUs <= Mac::ScaParams::kRamOffsetUsMax,
                 error = kErrorInvalidArgs);

    mLocalSca.mRamDuration = aParams.mRamDuration;
    mLocalSca.mRamOffsetUs = aParams.mRamOffsetUs;
    memcpy(mLocalSca.mRamBits, aParams.mRamBits, sizeof(mLocalSca.mRamBits));

exit:
    return error;
}

void DirectHandler::GetRamMask(Mac::ScaParams &aParams) const
{
    aParams.mRamDuration = mLocalSca.mRamDuration;
    aParams.mRamOffsetUs = mLocalSca.mRamOffsetUs;
    memcpy(aParams.mRamBits, mLocalSca.mRamBits, sizeof(aParams.mRamBits));
}

Error DirectHandler::SetSlwTimeout(uint32_t aTimeout)
{
    Error error = kErrorNone;

    VerifyOrExit(aTimeout <= kMaxSlwTimeout, error = kErrorInvalidArgs);
    mSlwTimeout = (aTimeout == 0) ? kDefaultSlwTimeout : aTimeout;

exit:
    return error;
}

} // namespace ot

#endif // OPENTHREAD_CONFIG_THREAD_DIRECT_WAKE_INITIATOR_ENABLE || OPENTHREAD_CONFIG_THREAD_DIRECT_WAKE_LISTENER_ENABLE
