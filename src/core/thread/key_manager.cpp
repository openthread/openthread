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

#include "openthread-instance.h"
#include "common/code_utils.hpp"
#include "common/timer.hpp"
#include "crypto/hmac_sha256.hpp"
#include "thread/mle_router.hpp"
#include "thread/thread_netif.hpp"

namespace ot {

static const uint8_t kThreadString[] =
{
    'T', 'h', 'r', 'e', 'a', 'd',
};

KeyManager::KeyManager(otInstance &aInstance):
    InstanceLocator(aInstance),
    mKeySequence(0),
    mMacFrameCounter(0),
    mMleFrameCounter(0),
    mStoredMacFrameCounter(0),
    mStoredMleFrameCounter(0),
    mHoursSinceKeyRotation(0),
    mKeyRotationTime(kDefaultKeyRotationTime),
    mKeySwitchGuardTime(kDefaultKeySwitchGuardTime),
    mKeySwitchGuardEnabled(false),
    mKeyRotationTimer(aInstance, &KeyManager::HandleKeyRotationTimer, this),
    mKekFrameCounter(0),
    mSecurityPolicyFlags(0xff)
{
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

#if OPENTHREAD_FTD
const uint8_t *KeyManager::GetPSKc(void) const
{
    return mPSKc;
}

void KeyManager::SetPSKc(const uint8_t *aPSKc)
{
    memcpy(mPSKc, aPSKc, sizeof(mPSKc));
}
#endif

const otMasterKey &KeyManager::GetMasterKey(void) const
{
    return mMasterKey;
}

otError KeyManager::SetMasterKey(const otMasterKey &aKey)
{
    otError error = OT_ERROR_NONE;
    Router *routers;
    Child *children;
    uint8_t num;

    VerifyOrExit(memcmp(&mMasterKey, &aKey, sizeof(mMasterKey)) != 0);

    mMasterKey = aKey;
    mKeySequence = 0;
    ComputeKey(mKeySequence, mKey);

    // reset parent frame counters
    routers = GetNetif().GetMle().GetParent();
    routers->SetKeySequence(0);
    routers->SetLinkFrameCounter(0);
    routers->SetMleFrameCounter(0);

    // reset router frame counters
    routers = GetNetif().GetMle().GetRouters(&num);

    for (uint8_t i = 0; i < num; i++)
    {
        routers[i].SetKeySequence(0);
        routers[i].SetLinkFrameCounter(0);
        routers[i].SetMleFrameCounter(0);
    }

    // reset child frame counters
    children = GetNetif().GetMle().GetChildren(&num);

    for (uint8_t i = 0; i < num; i++)
    {
        children[i].SetKeySequence(0);
        children[i].SetLinkFrameCounter(0);
        children[i].SetMleFrameCounter(0);
    }

    GetNetif().SetStateChangedFlags(OT_CHANGED_THREAD_KEY_SEQUENCE_COUNTER);

exit:
    return error;
}

otError KeyManager::ComputeKey(uint32_t aKeySequence, uint8_t *aKey)
{
    Crypto::HmacSha256 hmac;
    uint8_t keySequenceBytes[4];

    hmac.Start(mMasterKey.m8, sizeof(mMasterKey.m8));

    keySequenceBytes[0] = (aKeySequence >> 24) & 0xff;
    keySequenceBytes[1] = (aKeySequence >> 16) & 0xff;
    keySequenceBytes[2] = (aKeySequence >> 8) & 0xff;
    keySequenceBytes[3] = aKeySequence & 0xff;
    hmac.Update(keySequenceBytes, sizeof(keySequenceBytes));
    hmac.Update(kThreadString, sizeof(kThreadString));

    hmac.Finish(aKey);

    return OT_ERROR_NONE;
}

void KeyManager::SetCurrentKeySequence(uint32_t aKeySequence)
{
    if (aKeySequence == mKeySequence)
    {
        ExitNow();
    }

    // Check if the guard timer has expired if key rotation is requested.
    if ((aKeySequence == (mKeySequence + 1)) &&
        (mKeySwitchGuardTime != 0) &&
        mKeyRotationTimer.IsRunning() &&
        mKeySwitchGuardEnabled)
    {
        VerifyOrExit(mHoursSinceKeyRotation >= mKeySwitchGuardTime);
    }

    mKeySequence = aKeySequence;
    ComputeKey(mKeySequence, mKey);

    mMacFrameCounter = 0;
    mMleFrameCounter = 0;

    if (mKeyRotationTimer.IsRunning())
    {
        mKeySwitchGuardEnabled = true;
        StartKeyRotationTimer();
    }

    GetNetif().SetStateChangedFlags(OT_CHANGED_THREAD_KEY_SEQUENCE_COUNTER);

exit:
    return;
}

const uint8_t *KeyManager::GetTemporaryMacKey(uint32_t aKeySequence)
{
    ComputeKey(aKeySequence, mTemporaryKey);
    return mTemporaryKey + kMacKeyOffset;
}

const uint8_t *KeyManager::GetTemporaryMleKey(uint32_t aKeySequence)
{
    ComputeKey(aKeySequence, mTemporaryKey);
    return mTemporaryKey;
}

void KeyManager::IncrementMacFrameCounter(void)
{
    mMacFrameCounter++;

    if (mMacFrameCounter >= mStoredMacFrameCounter)
    {
        GetNetif().GetMle().Store();
    }
}

void KeyManager::IncrementMleFrameCounter(void)
{
    mMleFrameCounter++;

    if (mMleFrameCounter >= mStoredMleFrameCounter)
    {
        GetNetif().GetMle().Store();
    }
}

void KeyManager::SetKek(const uint8_t *aKek)
{
    memcpy(mKek, aKek, sizeof(mKek));
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

void KeyManager::StartKeyRotationTimer(void)
{
    mHoursSinceKeyRotation = 0;
    mKeyRotationTimer.Start(kOneHourIntervalInMsec);
}

void KeyManager::HandleKeyRotationTimer(Timer &aTimer)
{
    GetOwner(aTimer).HandleKeyRotationTimer();
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

KeyManager &KeyManager::GetOwner(const Context &aContext)
{
#if OPENTHREAD_ENABLE_MULTIPLE_INSTANCES
    KeyManager &keyManager = *static_cast<KeyManager *>(aContext.GetContext());
#else
    KeyManager &keyManager = otGetThreadNetif().GetKeyManager();
    OT_UNUSED_VARIABLE(aContext);
#endif
    return keyManager;
}

}  // namespace ot
