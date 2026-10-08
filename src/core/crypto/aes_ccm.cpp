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
 *   This file implements AES-CCM.
 */

#include "aes_ccm.hpp"

#include "common/code_utils.hpp"
#include "common/debug.hpp"
#include "common/encoding.hpp"
#include "common/num_utils.hpp"

namespace ot {
namespace Crypto {

//---------------------------------------------------------------------------------------------------------------------
// AesCcm

AesCcm::AesCcm(void)
{
    mConfig.Clear();
    mAuthData = nullptr;
}

void AesCcm::SetNonce(const void *aNonce, uint8_t aLength)
{
    mConfig.mNonce       = reinterpret_cast<const uint8_t *>(aNonce);
    mConfig.mNonceLength = aLength;
}

void AesCcm::SetTagLength(uint8_t aTagLength)
{
    OT_ASSERT(((aTagLength & 0x1) == 0) && IsValueInRange(aTagLength, kMinTagLength, kMaxTagLength));

    mConfig.mTagLength = aTagLength;
}

void AesCcm::SetAuthData(const void *aAuthData, uint32_t aLength)
{
    mAuthData             = reinterpret_cast<const uint8_t *>(aAuthData);
    mConfig.mHeaderLength = aLength;
}

Error AesCcm::Process(Operation aOperation, uint8_t *aData, uint32_t aLength)
{
    Engine engine;

    mConfig.mPlainTextLength = aLength;

    return engine.ProcessOneShot(aOperation, mConfig, mAuthData, aData);
}

#if OPENTHREAD_FTD || OPENTHREAD_MTD

Error AesCcm::Process(Operation aOperation, Message &aMessage, uint16_t aOffset)
{
    Error                 error = kErrorNone;
    Engine                engine;
    Message::MutableChunk chunk;
    uint16_t              remainingLength;
    uint8_t               tag[kMaxTagLength];

    VerifyOrExit(aOffset <= aMessage.GetLength(), error = kErrorInvalidArgs);

    switch (aOperation)
    {
    case kEncrypt:
        SuccessOrExit(error = aMessage.IncreaseLength(mConfig.mTagLength));
        break;
    case kDecrypt:
        VerifyOrExit(aMessage.GetLength() - aOffset >= mConfig.mTagLength, error = kErrorSecurity);
        break;
    }

    mConfig.mPlainTextLength = aMessage.GetLength() - aOffset - mConfig.mTagLength;

    // First, check if the entire payload and tag are present in
    // a single chunk (i.e., in one contiguous buffer).

    remainingLength = aMessage.GetLength() - aOffset;

    aMessage.GetFirstChunk(aOffset, remainingLength, chunk);

    if (chunk.GetLength() == mConfig.mPlainTextLength + mConfig.mTagLength)
    {
        error = engine.ProcessOneShot(aOperation, mConfig, mAuthData, chunk.GetBytes());
        ExitNow();
    }

    // The payload content spans multiple chunks. We need to process
    // it iteratively using multi-part `engine` methods.

    remainingLength = aMessage.GetLength() - aOffset - mConfig.mTagLength;

    engine.Start(aOperation, mConfig);
    engine.AddHeader(mAuthData, mConfig.mHeaderLength);

    aMessage.GetFirstChunk(aOffset, remainingLength, chunk);

    while (chunk.GetLength() > 0)
    {
        engine.AddPayload(chunk.GetBytes(), chunk.GetBytes(), chunk.GetLength());
        aMessage.GetNextChunk(remainingLength, chunk);
    }

    switch (aOperation)
    {
    case kEncrypt:
        engine.Finalize(tag);
        aMessage.WriteBytes(aMessage.GetLength() - mConfig.mTagLength, tag, mConfig.mTagLength);
        break;

    case kDecrypt:
        aMessage.ReadBytes(aMessage.GetLength() - mConfig.mTagLength, tag, mConfig.mTagLength);
        SuccessOrExit(error = engine.Verify(tag));
        break;
    }

exit:
    if ((aOperation == kDecrypt) && (error == kErrorNone))
    {
        aMessage.RemoveFooter(mConfig.mTagLength);
    }

    return error;
}
#endif //  OPENTHREAD_FTD || OPENTHREAD_MTD

void AesCcm::Perform(Operation   aOperation,
                     const Key  &aKey,
                     uint8_t     aTagLength,
                     const void *aNonce,
                     uint8_t     aNonceLength,
                     const void *aAuthData,
                     uint32_t    aAuthDataLength,
                     void       *aPlainText,
                     void       *aCipherText,
                     uint32_t    aLength,
                     void       *aTag)
{
    Config         config;
    Engine         engine;
    const uint8_t *input;
    uint8_t       *output;

    config.mKey             = aKey;
    config.mTagLength       = aTagLength;
    config.mNonce           = reinterpret_cast<const uint8_t *>(aNonce);
    config.mNonceLength     = aNonceLength;
    config.mHeaderLength    = aAuthDataLength;
    config.mPlainTextLength = aLength;

    if (aOperation == kEncrypt)
    {
        input  = reinterpret_cast<const uint8_t *>(aPlainText);
        output = reinterpret_cast<uint8_t *>(aCipherText);
    }
    else
    {
        input  = reinterpret_cast<const uint8_t *>(aCipherText);
        output = reinterpret_cast<uint8_t *>(aPlainText);
    }

    engine.Start(aOperation, config);
    engine.AddHeader(reinterpret_cast<const uint8_t *>(aAuthData), aAuthDataLength);
    engine.AddPayload(input, output, aLength);

    // We use `Finalize` and not `Verify` since we leave the comparison to the caller.
    engine.Finalize(reinterpret_cast<uint8_t *>(aTag));
}

//---------------------------------------------------------------------------------------------------------------------
// AesCcm::Config

bool AesCcm::Config::IsValid(void) const
{
    bool isValid = false;

    VerifyOrExit(IsValueInRange(mNonceLength, kMinNonceLength, kMaxNonceLength));
    VerifyOrExit(mNonce != nullptr);

    VerifyOrExit((mTagLength % 2) == 0);
    VerifyOrExit(IsValueInRange(mTagLength, kMinTagLength, kMaxTagLength));

    isValid = true;

exit:
    return isValid;
}

//---------------------------------------------------------------------------------------------------------------------
// AesCcm::Engine

Error AesCcm::Engine::ProcessOneShot(Operation      aOperation,
                                     const Config  &aConfig,
                                     const uint8_t *aHeader,
                                     uint8_t       *aData)
{
    Error error = kErrorNone;

#if OPENTHREAD_CONFIG_CRYPTO_PLATFORM_CCM_ONE_SHOT_ENABLE
    error = otPlatCryptoAesCcmProcessOneShot(aOperation == kEncrypt, &aConfig, aHeader, aData);
#else
    uint8_t *tag = aData + aConfig.mPlainTextLength;

    Start(aOperation, aConfig);
    AddHeader(aHeader, aConfig.mHeaderLength);
    AddPayload(aData, aData, aConfig.mPlainTextLength);

    switch (aOperation)
    {
    case kEncrypt:
        Finalize(tag);
        break;

    case kDecrypt:
        error = Verify(tag);
        break;
    }
#endif

    return error;
}

void AesCcm::Engine::Start(Operation aOperation, const Config &aConfig)
{
    uint8_t  blockLength = 0;
    uint32_t len;
    uint8_t  L;
    uint8_t  i;

    OT_ASSERT(aConfig.IsValid());

    mEcb.SetKey(aConfig.GetKey());

    mOperation       = aOperation;
    mNonceLength     = aConfig.mNonceLength;
    mTagLength       = aConfig.mTagLength;
    mHeaderLength    = aConfig.mHeaderLength;
    mPlainTextLength = aConfig.mPlainTextLength;

    L = 0;

    for (len = mPlainTextLength; len; len >>= 8)
    {
        L++;
    }

    if (L <= 1)
    {
        L = 2;
    }

    if (mNonceLength > 13)
    {
        mNonceLength = 13;
    }

    // increase L to match nonce len
    if (L < (15 - mNonceLength))
    {
        L = 15 - mNonceLength;
    }

    // decrease nonceLength to match L
    if (mNonceLength > (15 - L))
    {
        mNonceLength = 15 - L;
    }

    // setup initial block

    // write flags
    mBlock[0] = (static_cast<uint8_t>((mHeaderLength != 0) << 6) | static_cast<uint8_t>(((mTagLength - 2) >> 1) << 3) |
                 static_cast<uint8_t>(L - 1));

    // write nonce
    memcpy(&mBlock[1], aConfig.mNonce, mNonceLength);

    // write len
    len = mPlainTextLength;

    for (i = sizeof(mBlock) - 1; i > mNonceLength; i--)
    {
        mBlock[i] = len & 0xff;
        len >>= 8;
    }

    // encrypt initial block
    mEcb.Encrypt(mBlock, mBlock);

    // process header
    if (mHeaderLength > 0)
    {
        // process length
        if (mHeaderLength < (65536U - 256U))
        {
            mBlock[blockLength++] ^= mHeaderLength >> 8;
            mBlock[blockLength++] ^= mHeaderLength >> 0;
        }
        else
        {
            mBlock[blockLength++] ^= 0xff;
            mBlock[blockLength++] ^= 0xfe;
            mBlock[blockLength++] ^= mHeaderLength >> 24;
            mBlock[blockLength++] ^= mHeaderLength >> 16;
            mBlock[blockLength++] ^= mHeaderLength >> 8;
            mBlock[blockLength++] ^= mHeaderLength >> 0;
        }
    }

    // init counter
    mCtr[0] = L - 1;
    memcpy(&mCtr[1], aConfig.mNonce, mNonceLength);
    memset(&mCtr[mNonceLength + 1], 0, sizeof(mCtr) - mNonceLength - 1);

    mHeaderCur    = 0;
    mPlainTextCur = 0;
    mBlockLength  = blockLength;
    mCtrLength    = sizeof(mCtrPad);
}

void AesCcm::Engine::AddHeader(const uint8_t *aHeader, uint32_t aHeaderLength)
{
    OT_ASSERT((aHeaderLength == 0) || aHeader != nullptr);
    OT_ASSERT(mHeaderCur + aHeaderLength <= mHeaderLength);

    // process header
    for (unsigned i = 0; i < aHeaderLength; i++)
    {
        if (mBlockLength == sizeof(mBlock))
        {
            mEcb.Encrypt(mBlock, mBlock);
            mBlockLength = 0;
        }

        mBlock[mBlockLength++] ^= aHeader[i];
    }

    mHeaderCur += aHeaderLength;

    if (mHeaderCur == mHeaderLength)
    {
        // process remainder
        if (mBlockLength != 0)
        {
            mEcb.Encrypt(mBlock, mBlock);
        }

        mBlockLength = 0;
    }
}

void AesCcm::Engine::AddPayload(const uint8_t *aInput, uint8_t *aOutput, uint32_t aLength)
{
    uint8_t byte;

    OT_ASSERT((aLength == 0) || (aInput != nullptr && aOutput != nullptr));
    OT_ASSERT(mPlainTextCur + aLength <= mPlainTextLength);

    for (unsigned i = 0; i < aLength; i++)
    {
        if (mCtrLength == 16)
        {
            for (int j = sizeof(mCtr) - 1; j > mNonceLength; j--)
            {
                if (++mCtr[j])
                {
                    break;
                }
            }

            mEcb.Encrypt(mCtr, mCtrPad);
            mCtrLength = 0;
        }

        if (mOperation == kEncrypt)
        {
            byte       = aInput[i];
            aOutput[i] = byte ^ mCtrPad[mCtrLength++];
        }
        else
        {
            byte       = aInput[i] ^ mCtrPad[mCtrLength++];
            aOutput[i] = byte;
        }

        if (mBlockLength == sizeof(mBlock))
        {
            mEcb.Encrypt(mBlock, mBlock);
            mBlockLength = 0;
        }

        mBlock[mBlockLength++] ^= byte;
    }

    mPlainTextCur += aLength;

    if (mPlainTextCur >= mPlainTextLength)
    {
        if (mBlockLength != 0)
        {
            mEcb.Encrypt(mBlock, mBlock);
        }

        // reset counter
        memset(&mCtr[mNonceLength + 1], 0, sizeof(mCtr) - mNonceLength - 1);
    }
}

void AesCcm::Engine::Finalize(uint8_t *aTag)
{
    OT_ASSERT(mPlainTextCur == mPlainTextLength);

    mEcb.Encrypt(mCtr, mCtrPad);

    for (int i = 0; i < mTagLength; i++)
    {
        aTag[i] = mBlock[i] ^ mCtrPad[i];
    }
}

Error AesCcm::Engine::Verify(const uint8_t *aTag)
{
    uint8_t diff = 0;

    OT_ASSERT(mPlainTextCur == mPlainTextLength);

    mEcb.Encrypt(mCtr, mCtrPad);

    for (int i = 0; i < mTagLength; i++)
    {
        diff |= aTag[i] ^ (mBlock[i] ^ mCtrPad[i]);
    }

    return (diff == 0) ? kErrorNone : kErrorSecurity;
}

//---------------------------------------------------------------------------------------------------------------------
// AesCcm::Nonce

void AesCcm::Nonce::InitFrom(const Mac::ExtAddress &aExtAddress, uint32_t aFrameCounter, uint8_t aSecurityLevel)
{
    mExtAddress    = aExtAddress;
    mFrameCounter  = BigEndian::HostSwap32(aFrameCounter);
    mSecurityLevel = aSecurityLevel;
}

} // namespace Crypto
} // namespace ot
