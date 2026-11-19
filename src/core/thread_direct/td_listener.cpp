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
#include "thread_direct/td_commands.hpp"

namespace ot {
namespace ThreadDirect {

RegisterLogModule("TdListener");

//---------------------------------------------------------------------------------------------------------------------
// Listener::Config

bool Listener::Config::IsValid(Radio::Time64 aNow) const
{
    static_assert(kMaxFutureStartOffset + kMaxInterval <= TimerMicro::kMaxDelay,
                  "kMaxFutureStartOffset + kMaxInterval exceeds TimerMicro::kMaxDelay");

    static_assert(2 * static_cast<uint64_t>(kMaxInterval) <= TimerMicro::kMaxDelay,
                  "2 * kMaxInterval exceeds TimerMicro::kMaxDelay");

    bool     valid = false;
    uint32_t interval;

    VerifyOrExit(Radio::IsChannelValid(GetChannel()));

    interval = GetInterval();
    VerifyOrExit((interval >= GetDuration()) && (interval <= kMaxInterval));

    if (mStartTime > aNow)
    {
        VerifyOrExit(mStartTime - aNow <= kMaxFutureStartOffset);
    }

    valid = true;

exit:
    return valid;
}

Radio::Time64 Listener::Config::DetermineStartTime(Radio::Time64 aNow) const
{
    Radio::Time64 start;

    if (mStartTime == 0)
    {
        start = aNow;
    }
    else if (mStartTime >= aNow)
    {
        start = mStartTime;
    }
    else
    {
        uint64_t interval  = GetInterval();
        uint64_t remainder = (aNow - mStartTime) % interval;

        start = (remainder == 0) ? aNow : (aNow + (interval - remainder));
    }

    return start;
}

//---------------------------------------------------------------------------------------------------------------------
// Listener

Listener::Listener(Instance &aInstance)
    : InstanceLocator(aInstance)
    , mState(kStopped)
{
    mTimedRx.Clear();
    mRxWake.Clear();
}

Error Listener::Start(const Config &aConfig)
{
    Error         error = kErrorNone;
    Radio::Time64 now;

    VerifyOrExit(mState == kStopped, error = kErrorBusy);

    now = Get<Radio::Radio>().GetNow();

    VerifyOrExit(aConfig.IsValid(now), error = kErrorInvalidArgs);

    mTimedRx.Init(aConfig.DetermineStartTime(now), aConfig.GetDuration(), aConfig.GetChannel());

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
    mRxWake.Clear();
    Get<Mac::SubMac>().mListenScheduler.UpdateOnTdListenerChange();

exit:
    return;
}

Error Listener::GetWakeInfo(WakeInfo &aWakeInfo) const
{
    Error error = kErrorNone;

    VerifyOrExit(IsRunning(), error = kErrorInvalidState);
    VerifyOrExit(mRxWake.mIsValid, error = kErrorNotFound);

    aWakeInfo.mRxTimestamp = mRxWake.mRxTimestamp;
    aWakeInfo.mExtAddress  = mRxWake.mExtAddress;
    aWakeInfo.mKey         = mRxWake.mKey;
    aWakeInfo.mLinkChannel = mRxWake.mLinkChannel;
    aWakeInfo.mPanId       = mRxWake.mPanId;
    aWakeInfo.mIsUnicast   = mRxWake.mIsUnicast;

exit:
    return error;
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
    bool              processed  = false;
    Key              *matchedKey = nullptr;
    KeyId             keyId;
    Radio::SavedPsdu  savedPsdu;
    WakeCommand::Info cmdInfo;

    VerifyOrExit(IsRunning());

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // Validate Wake Command MAC and Security Headers

    VerifyOrExit(aFrameInfo.mParsedFully);

    VerifyOrExit(aFrameInfo.GetChannel() == mTimedRx.GetChannel());

    VerifyOrExit(aFrameInfo.mType == Mac::Frame::kTypeMacCmd);
    VerifyOrExit(aFrameInfo.mVersion == Mac::Frame::kVersion2015);
    VerifyOrExit(!aFrameInfo.mIsAckRequest);
    VerifyOrExit(aFrameInfo.mIsSecurityEnabled);

    VerifyOrExit(!aFrameInfo.mPanIds.IsSourcePresent());

    if (aFrameInfo.mPanIds.IsDestinationPresent())
    {
        VerifyOrExit(Mac::MatchPanIds(aFrameInfo.mPanIds.GetDestination(), Get<Mac::SubMac>().GetPanId()));
    }

    VerifyOrExit(aFrameInfo.mAddrs.mSource.IsExtended());
    VerifyOrExit(aFrameInfo.mAddrs.mSource.GetExtended() != Get<Mac::SubMac>().GetExtAddress());

    if (!aFrameInfo.mAddrs.mDestination.IsNone())
    {
        VerifyOrExit(aFrameInfo.mAddrs.mDestination.IsBroadcast() ||
                     Get<Mac::SubMac>().HasAddress(aFrameInfo.mAddrs.mDestination));
    }

    VerifyOrExit(aFrameInfo.mSecurityLevel == Mac::Frame::kSecurityEncMic32);
    SuccessOrExit(keyId.ReadFrom(aFrameInfo));
    VerifyOrExit(aFrameInfo.mFrameCounter != NumericLimits<uint32_t>::kMax);

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

    // Mark the frame as processed once it authenticates with a registered
    // Thread Direct key (which also decrypts the PSDU in-place), so the
    // caller does not pass it to normal MAC frame processing even if
    // payload parsing or validation below rejects it.

    processed = true;

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // (For future) Peer Table replay protection:
    //
    // Once the Thread Direct Peer Table is added, look up the peer
    // (using source extended address and `matchedKey`), verify
    // `aFrameInfo.mFrameCounter` freshness to reject replayed frames, and
    // update the peer table entry with the new frame counter.

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // Parse the Wake Command payload

    SuccessOrExit(cmdInfo.ParseFrom(aFrameInfo.mPayload));

    VerifyOrExit(cmdInfo.IsValid());

    // If Action Select is present, a future-defined post-wake action is
    // being requested rather than default TD Link establishment. Since no
    // Action Select values are defined in the spec yet, we simply ignore
    // the Wake frame and continue listening.

    VerifyOrExit(!cmdInfo.mHasActionSelect);

    // The Wake Command is fully validated.

    mRxWake.mIsValid        = true;
    mRxWake.mIsUnicast      = !aFrameInfo.mAddrs.mDestination.IsNone() && !aFrameInfo.mAddrs.mDestination.IsBroadcast();
    mRxWake.mKey            = matchedKey;
    mRxWake.mRxTimestamp    = aFrameInfo.GetTimestamp();
    mRxWake.mExtAddress     = aFrameInfo.mAddrs.mSource.GetExtended();
    mRxWake.mFrameCounter   = aFrameInfo.mFrameCounter;
    mRxWake.mPanId          = aFrameInfo.mPanIds.GetDestinationOr(Mac::kPanIdBroadcast);
    mRxWake.mRendezvousTime = cmdInfo.mHasRendezvousTime ? cmdInfo.mRendezvousTime : 0;
    mRxWake.mRetryInterval  = cmdInfo.mHasConnParams ? cmdInfo.mRetryInterval : WakeCommand::kDefaultRetryInterval;
    mRxWake.mRetryCount     = cmdInfo.mHasConnParams ? cmdInfo.mRetryCount : WakeCommand::kDefaultRetryCount;
    mRxWake.mLinkChannel    = cmdInfo.mHasChannel ? cmdInfo.mChannel : mTimedRx.GetChannel();

    mWakeCallback.InvokeIfSet();

exit:
    return processed;
}

void Listener::HandleKeyUnregistered(Key &aKey)
{
    if (mRxWake.mKey == &aKey)
    {
        mRxWake.Clear();
    }
}

} // namespace ThreadDirect
} // namespace ot

#endif // OPENTHREAD_CONFIG_TD_WAKE_LISTENER_ENABLE
