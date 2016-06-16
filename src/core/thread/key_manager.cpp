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

namespace Thread {

static const uint8_t kThreadString[] =
{
    'T', 'h', 'r', 'e', 'a', 'd',
};

KeyManager::KeyManager(ThreadNetif &aThreadNetif):
    mNetif(aThreadNetif)
{
    mPreviousKeyValid = false;
    mMasterKeyLength = 0;
    mMacFrameCounter = 0;
    mMleFrameCounter = 0;
    mCurrentKeySequence = 0;
    mPreviousKeySequence = 0;
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
    mCurrentKeySequence = 0;
    ComputeKey(mCurrentKeySequence, mCurrentKey);

    mNetif.SetStateChangedFlags(OT_NET_KEY_SEQUENCE);

exit:
    return error;
}

ThreadError KeyManager::ComputeKey(uint32_t aKeySequence, uint8_t *aKey)
{
    uint8_t keySequenceBytes[4];

    otCryptoHmacSha256Start(mMasterKey, mMasterKeyLength);

    keySequenceBytes[0] = aKeySequence >> 24;
    keySequenceBytes[1] = aKeySequence >> 16;
    keySequenceBytes[2] = aKeySequence >> 8;
    keySequenceBytes[3] = aKeySequence >> 0;
    otCryptoHmacSha256Update(keySequenceBytes, sizeof(keySequenceBytes));
    otCryptoHmacSha256Update(kThreadString, sizeof(kThreadString));

    otCryptoHmacSha256Finish(aKey);

    return kThreadError_None;
}

uint32_t KeyManager::GetCurrentKeySequence() const
{
    return mCurrentKeySequence;
}

void KeyManager::UpdateNeighbors()
{
    uint8_t numNeighbors;
    Router *routers;
    Child *children;

    routers = mNetif.GetMle().GetParent();
    routers->mPreviousKey = true;

    routers = mNetif.GetMle().GetRouters(&numNeighbors);

    for (int i = 0; i < numNeighbors; i++)
    {
        routers[i].mPreviousKey = true;
    }

    children = mNetif.GetMle().GetChildren(&numNeighbors);

    for (int i = 0; i < numNeighbors; i++)
    {
        children[i].mPreviousKey = true;
    }
}

void KeyManager::SetCurrentKeySequence(uint32_t aKeySequence)
{
    mPreviousKeyValid = true;
    mPreviousKeySequence = mCurrentKeySequence;
    memcpy(mPreviousKey, mCurrentKey, sizeof(mPreviousKey));

    mCurrentKeySequence = aKeySequence;
    ComputeKey(mCurrentKeySequence, mCurrentKey);

    mMacFrameCounter = 0;
    mMleFrameCounter = 0;

    UpdateNeighbors();

    mNetif.SetStateChangedFlags(OT_NET_KEY_SEQUENCE);
}

const uint8_t *KeyManager::GetCurrentMacKey() const
{
    return mCurrentKey + 16;
}

const uint8_t *KeyManager::GetCurrentMleKey() const
{
    return mCurrentKey;
}

bool KeyManager::IsPreviousKeyValid() const
{
    return mPreviousKeyValid;
}

uint32_t KeyManager::GetPreviousKeySequence() const
{
    return mPreviousKeySequence;
}

const uint8_t *KeyManager::GetPreviousMacKey() const
{
    return mPreviousKey + 16;
}

const uint8_t *KeyManager::GetPreviousMleKey() const
{
    return mPreviousKey;
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
