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

#include <common/code_utils.hpp>
#include <common/timer.hpp>
#include <crypto/hmac_sha256.hpp>
#include <thread/key_manager.hpp>
#include <thread/mle_router.hpp>
#include <thread/thread_netif.hpp>

namespace Thread {

static const uint8_t kThreadString[] =
{
    'T', 'h', 'r', 'e', 'a', 'd',
};

KeyManager::KeyManager(ThreadNetif &aThreadNetif):
    mNetif(aThreadNetif),
    mKeyRotationTimer(aThreadNetif.GetIp6().mTimerScheduler, &KeyManager::HandleKeyRotationTimer, this)
{
    mMasterKeyLength = 0;
    mKeySequence = 0;
    mMacFrameCounter = 0;
    mMleFrameCounter = 0;
    mStoredMleFrameCounter = 0;
    mStoredMacFrameCounter = 0;

    mKeyRotationTime = kDefaultKeyRotationTime;
    mKeySwitchGuardTime = kDefaultKeySwitchGuardTime;
    mKeySwitchGuardEnabled = false;

    mSecurityPolicyFlags = 0xff;
}

void KeyManager::Start(void)
{
    mKeySwitchGuardEnabled = false;
    mKeyRotationTimer.Start(Timer::HoursToMsec(mKeyRotationTime));
}

void KeyManager::Stop(void)
{
    mKeyRotationTimer.Stop();
}

const uint8_t *KeyManager::GetMasterKey(uint8_t *aKeyLength) const
{
    if (aKeyLength)
    {
        *aKeyLength = mMasterKeyLength;
    }

    return mMasterKey;
}

ThreadError KeyManager::SetMasterKey(const void *aKey, uint8_t aKeyLength)
{
    ThreadError error = kThreadError_None;

    VerifyOrExit(aKeyLength <= sizeof(mMasterKey), error = kThreadError_InvalidArgs);
    VerifyOrExit((mMasterKeyLength != aKeyLength) || (memcmp(mMasterKey, aKey, aKeyLength) != 0), ;);

    memcpy(mMasterKey, aKey, aKeyLength);
    mMasterKeyLength = aKeyLength;
    mKeySequence = 0;
    ComputeKey(mKeySequence, mKey);

    mNetif.SetStateChangedFlags(OT_NET_KEY_SEQUENCE_COUNTER);

exit:
    return error;
}

ThreadError KeyManager::ComputeKey(uint32_t aKeySequence, uint8_t *aKey)
{
    Crypto::HmacSha256 hmac;
    uint8_t keySequenceBytes[4];

    hmac.Start(mMasterKey, mMasterKeyLength);

    keySequenceBytes[0] = (aKeySequence >> 24) & 0xff;
    keySequenceBytes[1] = (aKeySequence >> 16) & 0xff;
    keySequenceBytes[2] = (aKeySequence >> 8) & 0xff;
    keySequenceBytes[3] = aKeySequence & 0xff;
    hmac.Update(keySequenceBytes, sizeof(keySequenceBytes));
    hmac.Update(kThreadString, sizeof(kThreadString));

    hmac.Finish(aKey);

    return kThreadError_None;
}

uint32_t KeyManager::GetCurrentKeySequence(void) const
{
    return mKeySequence;
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
        uint32_t now = Timer::GetNow();
        uint32_t guardStartTimestamp = mKeyRotationTimer.Gett0();
        uint32_t guardEndTimestamp = guardStartTimestamp + Timer::HoursToMsec(mKeySwitchGuardTime);

        // Check for timer overflow
        if (guardEndTimestamp < mKeyRotationTimer.Gett0())
        {
            if ((now > guardStartTimestamp) || (now < guardEndTimestamp))
            {
                ExitNow();
            }
        }
        else
        {
            if ((now > guardStartTimestamp) && (now < guardEndTimestamp))
            {
                ExitNow();
            }
        }
    }

    mKeySequence = aKeySequence;
    ComputeKey(mKeySequence, mKey);

    mMacFrameCounter = 0;
    mMleFrameCounter = 0;

    if (mKeyRotationTimer.IsRunning())
    {
        mKeySwitchGuardEnabled = true;
        mKeyRotationTimer.Start(Timer::HoursToMsec(mKeyRotationTime));
    }

    mNetif.SetStateChangedFlags(OT_NET_KEY_SEQUENCE_COUNTER);

exit:
    return;
}

const uint8_t *KeyManager::GetCurrentMacKey(void) const
{
    return mKey + 16;
}

const uint8_t *KeyManager::GetCurrentMleKey(void) const
{
    return mKey;
}

const uint8_t *KeyManager::GetTemporaryMacKey(uint32_t aKeySequence)
{
    ComputeKey(aKeySequence, mTemporaryKey);
    return mTemporaryKey + 16;
}

const uint8_t *KeyManager::GetTemporaryMleKey(uint32_t aKeySequence)
{
    ComputeKey(aKeySequence, mTemporaryKey);
    return mTemporaryKey;
}

uint32_t KeyManager::GetMacFrameCounter(void) const
{
    return mMacFrameCounter;
}

void KeyManager::SetMacFrameCounter(uint32_t aMacFrameCounter)
{
    mMacFrameCounter = aMacFrameCounter;
}

void KeyManager::SetStoredMacFrameCounter(uint32_t aStoredMacFrameCounter)
{
    mStoredMacFrameCounter = aStoredMacFrameCounter;
}

void KeyManager::IncrementMacFrameCounter(void)
{
    mMacFrameCounter++;

    if (mMacFrameCounter >= mStoredMacFrameCounter)
    {
        mNetif.GetMle().Store();
    }
}

uint32_t KeyManager::GetMleFrameCounter(void) const
{
    return mMleFrameCounter;
}

void KeyManager::SetMleFrameCounter(uint32_t aMleFrameCounter)
{
    mMleFrameCounter = aMleFrameCounter;
}

void KeyManager::SetStoredMleFrameCounter(uint32_t aStoredMleFrameCounter)
{
    mStoredMleFrameCounter = aStoredMleFrameCounter;
}

void KeyManager::IncrementMleFrameCounter(void)
{
    mMleFrameCounter++;

    if (mMleFrameCounter >= mStoredMleFrameCounter)
    {
        mNetif.GetMle().Store();
    }
}

const uint8_t *KeyManager::GetKek(void) const
{
    return mKek;
}

void KeyManager::SetKek(const uint8_t *aKek)
{
    memcpy(mKek, aKek, sizeof(mKek));
    mKekFrameCounter = 0;
}

uint32_t KeyManager::GetKekFrameCounter(void) const
{
    return mKekFrameCounter;
}

void KeyManager::IncrementKekFrameCounter(void)
{
    mKekFrameCounter++;
}

ThreadError KeyManager::SetKeyRotation(uint32_t aKeyRotation)
{
    ThreadError result = kThreadError_None;

    VerifyOrExit(aKeyRotation >= static_cast<uint32_t>(kMinKeyRotationTime), result = kThreadError_InvalidArgs);
    VerifyOrExit(aKeyRotation <= static_cast<uint32_t>(kMaxKeyRotationTime), result = kThreadError_InvalidArgs);

    mKeyRotationTime = aKeyRotation;

exit:
    return result;
}

void KeyManager::HandleKeyRotationTimer(void *aContext)
{
    static_cast<KeyManager *>(aContext)->HandleKeyRotationTimer();
}

void KeyManager::HandleKeyRotationTimer(void)
{
    SetCurrentKeySequence(mKeySequence + 1);
}

}  // namespace Thread
