/*
 *  Copyright (c) 2017, The OpenThread Authors.
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
 *   This file implements the wrappers around the platform radio calls.
 */

#define WPP_NAME "radio.tmh"

#include "radio.hpp"
#include "common/timer.hpp"
#include "common/code_utils.hpp"
#include "common/logging.hpp"
#include "openthread-instance.h"

#include <openthread/platform/radio.h>

namespace ot {
namespace Mac {

Radio::Radio()
{
    mRxTotal    = 0;
    mTxTotal    = 0;
    mLastChange = 0;
    mState      = kRadioStateUnknown;
}

otError Radio::Sleep(otInstance *aInstance)
{
    otError error = OT_ERROR_NONE;
    uint32_t now;

    error = otPlatRadioSleep(aInstance);
    VerifyOrExit(error == OT_ERROR_NONE);

    if (mState != kRadioStateSleep)
    {
        now = Timer::GetNow();

        if (mState == kRadioStateRx)
        {
            mRxTotal += now - mLastChange;
        }

        mLastChange = now;
        mState      = kRadioStateSleep;
    }

exit:
    return error;
}

otError Radio::Receive(otInstance *aInstance, uint8_t aChannel)
{
    otError error = OT_ERROR_NONE;
    uint32_t now;

    error = otPlatRadioReceive(aInstance, aChannel);
    VerifyOrExit(error == OT_ERROR_NONE);

    if (mState != kRadioStateRx)
    {
        now = Timer::GetNow();

        if (mState == kRadioStateTx)
        {
            mTxTotal += now - mLastChange;
        }

        mLastChange = now;
        mState      = kRadioStateRx;
    }

exit:
    return error;
}

otError Radio::Transmit(otInstance *aInstance, otRadioFrame *aFrame)
{
    otError error = OT_ERROR_NONE;
    uint32_t now;

    error = otPlatRadioTransmit(aInstance, aFrame);
    VerifyOrExit(error == OT_ERROR_NONE);

    if (mState != kRadioStateTx)
    {
        now = Timer::GetNow();

        if (mState == kRadioStateRx)
        {
            mRxTotal += now - mLastChange;
        }

        mLastChange = now;
        mState      = kRadioStateTx;
    }

exit:
    return error;
}

otError Radio::TransmitDone(void)
{
    otError error = OT_ERROR_NONE;
    uint32_t now;

    if (mState != kRadioStateRx)
    {
        now = Timer::GetNow();

        if (mState == kRadioStateTx)
        {
            mTxTotal += now - mLastChange;
        }

        mLastChange = now;
        mState      = kRadioStateRx;
    }

    return error;
}

#if OPENTHREAD_CONFIG_LEGACY_TRANSMIT_DONE
extern "C" void otPlatRadioTransmitDone(otInstance *aInstance, otRadioFrame *aFrame, bool aRxPending,
                                        otError aError)
{
    otLogFuncEntryMsg("%!otError!, aRxPending=%u", aError, aRxPending ? 1 : 0);

#if OPENTHREAD_ENABLE_RAW_LINK_API

    if (aInstance->mLinkRaw.IsEnabled())
    {
        aInstance->mLinkRaw.InvokeTransmitDone(aFrame, aRxPending, aError);
    }
    else
#endif // OPENTHREAD_ENABLE_RAW_LINK_API
    {
        aInstance->mThreadNetif.GetMac().TransmitDoneTask(aFrame, aRxPending, aError);
    }

    otLogFuncExit();
}

#else // #if OPENTHREAD_CONFIG_LEGACY_TRANSMIT_DONE
extern "C" void otPlatRadioTxDone(otInstance *aInstance, otRadioFrame *aFrame, otRadioFrame *aAckFrame,
                                  otError aError)
{
    otLogFuncEntryMsg("%!otError!", aError);

#if OPENTHREAD_ENABLE_RAW_LINK_API

    if (aInstance->mLinkRaw.IsEnabled())
    {
        aInstance->mLinkRaw.InvokeTransmitDone(aFrame, (static_cast<Frame *>(aAckFrame))->GetFramePending(), aError);
    }
    else
#endif // OPENTHREAD_ENABLE_RAW_LINK_API
    {
        aInstance->mThreadNetif.GetMac().TransmitDoneTask(aFrame, aAckFrame, aError);
    }

    otLogFuncExit();
}
#endif // OPENTHREAD_CONFIG_LEGACY_TRANSMIT_DONE

}  // namespace Mac
}  // namespace ot
