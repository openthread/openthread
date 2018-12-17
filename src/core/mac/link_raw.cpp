/*
 *  Copyright (c) 2016-2018, The OpenThread Authors.
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
 *   This file implements the OpenThread Link Raw API.
 */

#include "openthread-core-config.h"

#include <string.h>
#include <openthread/diag.h>
#include <openthread/platform/diag.h>

#include "common/debug.hpp"
#include "common/instance.hpp"
#include "common/logging.hpp"
#include "common/random.hpp"
#include "mac/mac.hpp"
#include "mac/mac_frame.hpp"

#if OPENTHREAD_RADIO || OPENTHREAD_ENABLE_RAW_LINK_API

namespace ot {
namespace Mac {

LinkRaw::LinkRaw(Instance &aInstance)
    : InstanceLocator(aInstance)
    , mEnabled(false)
    , mReceiveChannel(OPENTHREAD_CONFIG_DEFAULT_CHANNEL)
    , mPanId(kPanIdBroadcast)
    , mReceiveDoneCallback(NULL)
    , mTransmitDoneCallback(NULL)
    , mEnergyScanDoneCallback(NULL)
#if OPENTHREAD_RADIO
    , mSubMac(aInstance, *this)
#elif OPENTHREAD_ENABLE_RAW_LINK_API
    , mSubMac(aInstance.Get<SubMac>())
#endif
{
}

otError LinkRaw::SetEnabled(bool aEnabled)
{
    otError error = OT_ERROR_NONE;

    otLogDebgMac("LinkRaw::Enabled(%s)", aEnabled ? "true" : "false");

#if OPENTHREAD_MTD || OPENTHREAD_FTD
    VerifyOrExit(!GetInstance().GetThreadNetif().IsUp(), error = OT_ERROR_INVALID_STATE);
#endif

    if (aEnabled)
    {
        SuccessOrExit(error = mSubMac.Enable());
    }
    else
    {
        mSubMac.Disable();
    }

    mEnabled = aEnabled;

exit:
    return error;
}

otError LinkRaw::SetPanId(uint16_t aPanId)
{
    otError error = OT_ERROR_NONE;

    VerifyOrExit(IsEnabled(), error = OT_ERROR_INVALID_STATE);
    mSubMac.SetPanId(aPanId);
    mPanId = aPanId;

exit:
    return error;
}

otError LinkRaw::SetChannel(uint8_t aChannel)
{
    otError error = OT_ERROR_NONE;

    VerifyOrExit(IsEnabled(), error = OT_ERROR_INVALID_STATE);
    mReceiveChannel = aChannel;

exit:
    return error;
}

otError LinkRaw::SetExtAddress(const ExtAddress &aExtAddress)
{
    otError error = OT_ERROR_NONE;

    VerifyOrExit(IsEnabled(), error = OT_ERROR_INVALID_STATE);
    mSubMac.SetExtAddress(aExtAddress);

exit:
    return error;
}

otError LinkRaw::SetShortAddress(ShortAddress aShortAddress)
{
    otError error = OT_ERROR_NONE;

    VerifyOrExit(IsEnabled(), error = OT_ERROR_INVALID_STATE);
    mSubMac.SetShortAddress(aShortAddress);

exit:
    return error;
}

otError LinkRaw::Receive(otLinkRawReceiveDone aCallback)
{
    otError error = OT_ERROR_NONE;

    VerifyOrExit(IsEnabled(), error = OT_ERROR_INVALID_STATE);

    SuccessOrExit(error = mSubMac.Receive(mReceiveChannel));
    mReceiveDoneCallback = aCallback;

exit:
    return error;
}

void LinkRaw::InvokeReceiveDone(Frame *aFrame, otError aError)
{
    otLogDebgMac("LinkRaw::ReceiveDone(%d bytes), error:%s", (aFrame != NULL) ? aFrame->mLength : 0,
                 otThreadErrorToString(aError));

    if (mReceiveDoneCallback && (aError == OT_ERROR_NONE))
    {
        mReceiveDoneCallback(&GetInstance(), aFrame, aError);
    }
}

otError LinkRaw::Transmit(otLinkRawTransmitDone aCallback)
{
    otError error = OT_ERROR_NONE;

    VerifyOrExit(IsEnabled(), error = OT_ERROR_INVALID_STATE);

    SuccessOrExit(error = mSubMac.Send());
    mTransmitDoneCallback = aCallback;

exit:
    return error;
}

void LinkRaw::InvokeTransmitDone(Frame &aFrame, Frame *aAckFrame, otError aError)
{
    otLogDebgMac("LinkRaw::TransmitDone(%d bytes), error:%s", aFrame.mLength, otThreadErrorToString(aError));

    if (mTransmitDoneCallback)
    {
        mTransmitDoneCallback(&GetInstance(), &aFrame, aAckFrame, aError);
        mTransmitDoneCallback = NULL;
    }
}

otError LinkRaw::EnergyScan(uint8_t aScanChannel, uint16_t aScanDuration, otLinkRawEnergyScanDone aCallback)
{
    otError error = OT_ERROR_NONE;

    VerifyOrExit(IsEnabled(), error = OT_ERROR_INVALID_STATE);

    SuccessOrExit(error = mSubMac.EnergyScan(aScanChannel, aScanDuration));
    mEnergyScanDoneCallback = aCallback;

exit:
    return error;
}

void LinkRaw::InvokeEnergyScanDone(int8_t aEnergyScanMaxRssi)
{
    if (IsEnabled() && mEnergyScanDoneCallback != NULL)
    {
        mEnergyScanDoneCallback(&GetInstance(), aEnergyScanMaxRssi);
        mEnergyScanDoneCallback = NULL;
    }
}

} // namespace Mac
} // namespace ot

#endif // OPENTHREAD_RADIO || OPENTHREAD_ENABLE_RAW_LINK_API
