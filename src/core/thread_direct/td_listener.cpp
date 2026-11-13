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
 *   This file includes implementation of Thread Direct Listener.
 */

#include "td_listener.hpp"

#if OPENTHREAD_CONFIG_TD_WAKE_LISTENER_ENABLE

#include "instance/instance.hpp"

namespace ot {
namespace ThreadDirect {

RegisterLogModule("TdListener");

//---------------------------------------------------------------------------------------------------------------------
// Listener::Config

bool Listener::Config::IsValid(void) const
{
    return Radio::IsChannelValid(GetChannel()) && (GetInterval() >= GetDuration()) &&
           (GetInterval() <= TimerMicro::kMaxDelay);
}

//---------------------------------------------------------------------------------------------------------------------
// Listener

Listener::Listener(Instance &aInstance)
    : InstanceLocator(aInstance)
    , mState(kStopped)
{
    mTimedRx.Clear();
}

Error Listener::Start(const Config &aConfig)
{
    Error error = kErrorNone;

    VerifyOrExit(mState == kStopped, error = kErrorBusy);
    VerifyOrExit(aConfig.IsValid(), error = kErrorInvalidArgs);

    mTimedRx.Init(Get<Radio::Radio>().GetNow(), aConfig.GetDuration(), aConfig.GetChannel());

    mState    = kRunning;
    mInterval = aConfig.GetInterval();

    mWakeCallback.Set(aConfig.mWakeCallback, aConfig.mWakeCallbackContext);

    Get<Mac::SubMac>().mListenScheduler.UpdateOnTdListenerChange();

exit:
    return error;
}

void Listener::Stop(void)
{
    VerifyOrExit(mState != kStopped);

    mState = kStopped;
    mTimedRx.Clear();
    Get<Mac::SubMac>().mListenScheduler.UpdateOnTdListenerChange();

exit:
    return;
}

Error Listener::GetWakeInfo(WakeInfo &aWakeInfo) const
{
    OT_UNUSED_VARIABLE(aWakeInfo);

    return IsRunning() ? kErrorNotFound : kErrorInvalidState;
}

const Listener::TimedRx &Listener::DetermineCurWindow(const Radio::SyncedTime &aNow)
{
    VerifyOrExit(mState == kRunning);

    while (mTimedRx.HasEnded(aNow))
    {
        mTimedRx.AdvanceStartTime(mInterval);
    }

exit:
    return mTimedRx;
}

bool Listener::ProcessRxFrame(Mac::RxFrame::ParseInfo &aFrameInfo)
{
    bool             processed  = false;
    Key             *matchedKey = nullptr;
    KeyId            keyId;
    Radio::SavedPsdu savedPsdu;

    VerifyOrExit(IsRunning());

    VerifyOrExit(aFrameInfo.mParsedFully);

    VerifyOrExit(aFrameInfo.GetChannel() == mTimedRx.GetChannel());

    VerifyOrExit(aFrameInfo.mType == Mac::Frame::kTypeMacCmd);

    if (aFrameInfo.mPanIds.IsDestinationPresent())
    {
        Mac::PanId panId = aFrameInfo.mPanIds.GetDestination();

        // TODO: Add panid checks
        VerifyOrExit(panId == Mac::kPanIdBroadcast); // || panId == mPanId || mPanIds == Mac::kPanIdBroadcast);
    }

    VerifyOrExit(aFrameInfo.mAddrs.mSource.IsExtended());

    if (!aFrameInfo.mAddrs.mDestination.IsNone())
    {
        VerifyOrExit(aFrameInfo.mAddrs.mDestination.IsBroadcast() ||
                     Get<Mac::SubMac>().HasAddress(aFrameInfo.mAddrs.mDestination));
    }

    VerifyOrExit(aFrameInfo.mIsSecurityEnabled);
    VerifyOrExit(aFrameInfo.mSecurityLevel == Mac::Frame::kSecurityEncMic32);
    SuccessOrExit(keyId.ReadFrom(aFrameInfo));

    // Multiple registered keys may share the same `KeyId`. Since
    // `ProcessReceiveAesCcm()` decrypts the frame payload in-place before
    // verifying the MIC tag, we save the PSDU before the first decryption
    // attempt and restore it if decryption fails.

    for (Key &key : Get<KeyManager>().GetKeyList())
    {
        if (!key.HasId(keyId))
        {
            continue;
        }

        if (!savedPsdu.IsSaved())
        {
            SuccessOrExit(aFrameInfo.SavePsdu(savedPsdu));
        }

        if (aFrameInfo.ProcessReceiveAesCcm(aFrameInfo.mAddrs.mSource.GetExtended(), key.GetKeyMaterial()) ==
            kErrorNone)
        {
            matchedKey = &key;
            break;
        }

        aFrameInfo.RestorePsdu(savedPsdu);
    }

    VerifyOrExit(matchedKey != nullptr);

    processed = true;

    // (For future) - Process decrypted Wake frame and `matchedKey`.

exit:
    return processed;
}

} // namespace ThreadDirect
} // namespace ot

#endif // OPENTHREAD_CONFIG_TD_WAKE_LISTENER_ENABLE
