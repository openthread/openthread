/*
 *  Copyright (c) 2016, Nest Labs, Inc.
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
#include <crypto/hmac_sha256.h>
#include <thread/key_manager.hpp>
#include <thread/mle_router.hpp>
#include <thread/thread_netif.hpp>
#include <openthreadinstance.h>

namespace Thread {

static const uint8_t kThreadString[] =
{
    'T', 'h', 'r', 'e', 'a', 'd',
};

KeyManager::KeyManager(ThreadNetif &aThreadNetif):
    mNetif(aThreadNetif)
{
    mMasterKeyLength = 0;
    mKeySequence = 0;
    mMacFrameCounter = 0;
    mMleFrameCounter = 0;
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
    memcpy(mMasterKey, aKey, aKeyLength);
    mMasterKeyLength = aKeyLength;
    mKeySequence = 0;
    ComputeKey(mKeySequence, mKey);

    mNetif.SetStateChangedFlags(OT_NET_KEY_SEQUENCE);

exit:
    return error;
}

ThreadError KeyManager::ComputeKey(uint32_t aKeySequence, uint8_t *aKey)
{
    uint8_t keySequenceBytes[4];
    otCryptoContext *aCryptoContext = &mNetif.GetInstance()->mCryptoContext;

    otCryptoHmacSha256Start(aCryptoContext, mMasterKey, mMasterKeyLength);

    keySequenceBytes[0] = (aKeySequence >> 24) & 0xff;
    keySequenceBytes[1] = (aKeySequence >> 16) & 0xff;
    keySequenceBytes[2] = (aKeySequence >> 8) & 0xff;
    keySequenceBytes[3] = aKeySequence & 0xff;
    otCryptoHmacSha256Update(aCryptoContext, keySequenceBytes, sizeof(keySequenceBytes));
    otCryptoHmacSha256Update(aCryptoContext, kThreadString, sizeof(kThreadString));

    otCryptoHmacSha256Finish(aCryptoContext, aKey);

    return kThreadError_None;
}

uint32_t KeyManager::GetCurrentKeySequence() const
{
    return mKeySequence;
}

void KeyManager::SetCurrentKeySequence(uint32_t aKeySequence)
{
    if (aKeySequence != mKeySequence)
    {
        mKeySequence = aKeySequence;
        ComputeKey(mKeySequence, mKey);

        mMacFrameCounter = 0;
        mMleFrameCounter = 0;

        mNetif.SetStateChangedFlags(OT_NET_KEY_SEQUENCE);
    }
}

const uint8_t *KeyManager::GetCurrentMacKey() const
{
    return mKey + 16;
}

const uint8_t *KeyManager::GetCurrentMleKey() const
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

uint32_t KeyManager::GetMacFrameCounter() const
{
    return mMacFrameCounter;
}

void KeyManager::IncrementMacFrameCounter()
{
    mMacFrameCounter++;
}

uint32_t KeyManager::GetMleFrameCounter() const
{
    return mMleFrameCounter;
}

void KeyManager::IncrementMleFrameCounter()
{
    mMleFrameCounter++;
}

}  // namespace Thread
