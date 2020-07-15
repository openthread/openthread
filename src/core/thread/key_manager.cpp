/*
 *  Copyright (c) 2016, The OpenThread Authors.
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
 *   This file implements Thread security material generation.
 */

#include "key_manager.hpp"

#include "common/code_utils.hpp"
#include "common/encoding.hpp"
#include "common/instance.hpp"
#include "common/locator-getters.hpp"
#include "common/timer.hpp"
#include "thread/mle_router.hpp"
#include "thread/thread_netif.hpp"

namespace ot {

const uint8_t KeyManager::kThreadString[] = {
    'T', 'h', 'r', 'e', 'a', 'd',
};

const otMasterKey KeyManager::kDefaultMasterKey = {{
    0x00,
    0x11,
    0x22,
    0x33,
    0x44,
    0x55,
    0x66,
    0x77,
    0x88,
    0x99,
    0xaa,
    0xbb,
    0xcc,
    0xdd,
    0xee,
    0xff,
}};

KeyManager::KeyManager(Instance &aInstance)
    : InstanceLocator(aInstance)
    , mKeySequence(0)
    , mMleFrameCounter(0)
    , mStoredMacFrameCounter(0)
    , mStoredMleFrameCounter(0)
    , mHoursSinceKeyRotation(0)
    , mKeyRotationTime(kDefaultKeyRotationTime)
    , mKeySwitchGuardTime(kDefaultKeySwitchGuardTime)
    , mKeySwitchGuardEnabled(false)
    , mKeyRotationTimer(aInstance, KeyManager::HandleKeyRotationTimer, this)
    , mKekFrameCounter(0)
    , mSecurityPolicyFlags(0xff)
    , mIsPskcSet(false)
{
    mMasterKey = static_cast<const MasterKey &>(kDefaultMasterKey);
    mPskc.Clear();
}

void KeyManager::Start(void)
{
    mKeySwitchGuardEnabled = false;
    StartKeyRotationTimer();
}

void KeyManager::Stop(void)
{
    mKeyRotationTimer.Stop();
}

#if OPENTHREAD_MTD || OPENTHREAD_FTD
void KeyManager::SetPskc(const Pskc &aPskc)
{
    IgnoreError(Get<Notifier>().Update(mPskc, aPskc, kEventPskcChanged));
    mIsPskcSet = true;
}
#endif // OPENTHREAD_MTD || OPENTHREAD_FTD

otError KeyManager::SetMasterKey(const MasterKey &aKey)
{
    otError error = OT_ERROR_NONE;
    Router *parent;

    SuccessOrExit(Get<Notifier>().Update(mMasterKey, aKey, kEventMasterKeyChanged));
    Get<Notifier>().Signal(kEventThreadKeySeqCounterChanged);
    mKeySequence = 0;
    UpdateKeyMaterial();

    // reset parent frame counters
    parent = &Get<Mle::MleRouter>().GetParent();
    parent->SetKeySequence(0);
    parent->SetLinkFrameCounter(0);
    parent->SetMleFrameCounter(0);

#if OPENTHREAD_FTD
    // reset router frame counters
    for (Router &router : Get<RouterTable>().Iterate())
    {
        router.SetKeySequence(0);
        router.SetLinkFrameCounter(0);
        router.SetMleFrameCounter(0);
    }

    // reset child frame counters
    for (Child &child : Get<ChildTable>().Iterate(Child::kInStateAnyExceptInvalid))
    {
        child.SetKeySequence(0);
        child.SetLinkFrameCounter(0);
        child.SetMleFrameCounter(0);
    }
#endif

exit:
    return error;
}

void KeyManager::ComputeKeys(uint32_t aKeySequence, HashKeys &aHashKeys)
{
    Crypto::HmacSha256 hmac;
    uint8_t            keySequenceBytes[sizeof(uint32_t)];

    hmac.Start(mMasterKey.m8, sizeof(mMasterKey.m8));

    Encoding::BigEndian::WriteUint32(aKeySequence, keySequenceBytes);
    hmac.Update(keySequenceBytes, sizeof(keySequenceBytes));
    hmac.Update(kThreadString, sizeof(kThreadString));

    hmac.Finish(aHashKeys.mHash);
}

void KeyManager::UpdateKeyMaterial(void)
{
    HashKeys prev;
    HashKeys cur;
    HashKeys next;

    ComputeKeys(mKeySequence - 1, prev);
    ComputeKeys(mKeySequence, cur);
    ComputeKeys(mKeySequence + 1, next);

    mMleKey = cur.mKeys.mMleKey;

    Get<Mac::SubMac>().SetMacKey(Mac::Frame::kKeyIdMode1, (mKeySequence & 0x7f) + 1, prev.mKeys.mMacKey,
                                 cur.mKeys.mMacKey, next.mKeys.mMacKey);
}

void KeyManager::SetCurrentKeySequence(uint32_t aKeySequence)
{
    VerifyOrExit(aKeySequence != mKeySequence, Get<Notifier>().SignalIfFirst(kEventThreadKeySeqCounterChanged));

    if ((aKeySequence == (mKeySequence + 1)) && mKeyRotationTimer.IsRunning())
    {
        if (mKeySwitchGuardEnabled)
        {
            // Check if the guard timer has expired if key rotation is requested.
            VerifyOrExit(mHoursSinceKeyRotation >= mKeySwitchGuardTime, OT_NOOP);
            StartKeyRotationTimer();
        }

        mKeySwitchGuardEnabled = true;
    }

    mKeySequence = aKeySequence;
    UpdateKeyMaterial();

    SetMacFrameCounter(0);
    mMleFrameCounter = 0;

    Get<Notifier>().Signal(kEventThreadKeySeqCounterChanged);

exit:
    return;
}

const Mle::Key &KeyManager::GetTemporaryMleKey(uint32_t aKeySequence)
{
    HashKeys hashKeys;

    ComputeKeys(aKeySequence, hashKeys);
    mTemporaryMleKey = hashKeys.mKeys.mMleKey;

    return mTemporaryMleKey;
}

uint32_t KeyManager::GetMacFrameCounter(void) const
{
    return Get<Mac::SubMac>().GetFrameCounter();
}

void KeyManager::SetMacFrameCounter(uint32_t aMacFrameCounter)
{
    Get<Mac::SubMac>().SetFrameCounter(aMacFrameCounter);
}

void KeyManager::MacFrameCounterUpdated(uint32_t aMacFrameCounter)
{
    if (aMacFrameCounter >= mStoredMacFrameCounter)
    {
        IgnoreError(Get<Mle::MleRouter>().Store());
    }
}

void KeyManager::IncrementMleFrameCounter(void)
{
    mMleFrameCounter++;

    if (mMleFrameCounter >= mStoredMleFrameCounter)
    {
        IgnoreError(Get<Mle::MleRouter>().Store());
    }
}

void KeyManager::SetKek(const Kek &aKek)
{
    mKek             = aKek;
    mKekFrameCounter = 0;
}

void KeyManager::SetKek(const uint8_t *aKek)
{
    memcpy(mKek.m8, aKek, sizeof(mKek));
    mKekFrameCounter = 0;
}

otError KeyManager::SetKeyRotation(uint32_t aKeyRotation)
{
    otError result = OT_ERROR_NONE;

    VerifyOrExit(aKeyRotation >= static_cast<uint32_t>(kMinKeyRotationTime), result = OT_ERROR_INVALID_ARGS);

    mKeyRotationTime = aKeyRotation;

exit:
    return result;
}

void KeyManager::SetSecurityPolicyFlags(uint8_t aSecurityPolicyFlags)
{
    IgnoreError(Get<Notifier>().Update(mSecurityPolicyFlags, aSecurityPolicyFlags, kEventSecurityPolicyChanged));
}

void KeyManager::StartKeyRotationTimer(void)
{
    mHoursSinceKeyRotation = 0;
    mKeyRotationTimer.Start(kOneHourIntervalInMsec);
}

void KeyManager::HandleKeyRotationTimer(Timer &aTimer)
{
    aTimer.GetOwner<KeyManager>().HandleKeyRotationTimer();
}

void KeyManager::HandleKeyRotationTimer(void)
{
    mHoursSinceKeyRotation++;

    // Order of operations below is important. We should restart the timer (from
    // last fire time for one hour interval) before potentially calling
    // `SetCurrentKeySequence()`. `SetCurrentKeySequence()` uses the fact that
    // timer is running to decide to check for the guard time and to reset the
    // rotation timer (and the `mHoursSinceKeyRotation`) if it updates the key
    // sequence.

    mKeyRotationTimer.StartAt(mKeyRotationTimer.GetFireTime(), kOneHourIntervalInMsec);

    if (mHoursSinceKeyRotation >= mKeyRotationTime)
    {
        SetCurrentKeySequence(mKeySequence + 1);
    }
}

} // namespace ot
