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

#include <openthread/config.h>

#include "common/debug.hpp"
#include "crypto/aes_ccm.hpp"

#include "test_platform.h"
#include "test_util.hpp"

namespace ot {

/**
 * Verifies test vectors from IEEE 802.15.4-2006 Annex C Section C.2.1
 */
void TestMacBeaconFrame(void)
{
    uint8_t key[] = {
        0xc0, 0xc1, 0xc2, 0xc3, 0xc4, 0xc5, 0xc6, 0xc7, 0xc8, 0xc9, 0xca, 0xcb, 0xcc, 0xcd, 0xce, 0xcf,
    };

    uint8_t test[] = {0x08, 0xD0, 0x84, 0x21, 0x43, 0x01, 0x00, 0x00, 0x00, 0x00, 0x48, 0xDE,
                      0xAC, 0x02, 0x05, 0x00, 0x00, 0x00, 0x55, 0xCF, 0x00, 0x00, 0x51, 0x52,
                      0x53, 0x54, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

    uint8_t encrypted[] = {0x08, 0xD0, 0x84, 0x21, 0x43, 0x01, 0x00, 0x00, 0x00, 0x00, 0x48, 0xDE,
                           0xAC, 0x02, 0x05, 0x00, 0x00, 0x00, 0x55, 0xCF, 0x00, 0x00, 0x51, 0x52,
                           0x53, 0x54, 0x22, 0x3B, 0xC1, 0xEC, 0x84, 0x1A, 0xB5, 0x53};

    uint8_t decrypted[] = {0x08, 0xD0, 0x84, 0x21, 0x43, 0x01, 0x00, 0x00, 0x00, 0x00, 0x48, 0xDE,
                           0xAC, 0x02, 0x05, 0x00, 0x00, 0x00, 0x55, 0xCF, 0x00, 0x00, 0x51, 0x52,
                           0x53, 0x54, 0x22, 0x3B, 0xC1, 0xEC, 0x84, 0x1A, 0xB5, 0x53};

    otInstance    *instance      = testInitInstance();
    uint32_t       headerLength  = sizeof(test) - 8;
    uint32_t       payloadLength = 0;
    uint8_t        tagLength     = 8;
    Crypto::AesCcm aesCcm;

    uint8_t nonce[] = {
        0xAC, 0xDE, 0x48, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x05, 0x02,
    };

    printf("TestMacBeaconFrame\n");

    VerifyOrQuit(instance != nullptr);

    aesCcm.SetKey(key, sizeof(key));
    aesCcm.SetNonce(nonce, sizeof(nonce));
    aesCcm.SetAuthData(test, headerLength);
    aesCcm.SetTagLength(tagLength);
    SuccessOrQuit(aesCcm.Process(Crypto::AesCcm::kEncrypt, &test[headerLength], payloadLength));
    DumpBuffer("encrypted", test, sizeof(test));
    VerifyOrQuit(memcmp(test, encrypted, sizeof(encrypted)) == 0);

    aesCcm.SetKey(key, sizeof(key));
    aesCcm.SetNonce(nonce, sizeof(nonce));
    aesCcm.SetAuthData(test, headerLength);
    aesCcm.SetTagLength(tagLength);
    SuccessOrQuit(aesCcm.Process(Crypto::AesCcm::kDecrypt, &test[headerLength], payloadLength));
    DumpBuffer("decrypted", test, sizeof(test));
    VerifyOrQuit(memcmp(test, decrypted, sizeof(decrypted)) == 0);

    testFreeInstance(instance);

    printf("\nTestMacBeaconFrame PASSED\n\n");
}

/**
 * Verifies test vectors from IEEE 802.15.4-2006 Annex C Section C.2.3
 */
void TestMacCommandFrame(void)
{
    uint8_t key[] = {
        0xc0, 0xc1, 0xc2, 0xc3, 0xc4, 0xc5, 0xc6, 0xc7, 0xc8, 0xc9, 0xca, 0xcb, 0xcc, 0xcd, 0xce, 0xcf,
    };

    uint8_t test[] = {
        0x2B, 0xDC, 0x84, 0x21, 0x43, 0x02, 0x00, 0x00, 0x00, 0x00, 0x48, 0xDE, 0xAC,
        0xFF, 0xFF, 0x01, 0x00, 0x00, 0x00, 0x00, 0x48, 0xDE, 0xAC, 0x06, 0x05, 0x00,
        0x00, 0x00, 0x01, 0xCE, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    };

    static constexpr uint32_t kHeaderLength  = 29;
    static constexpr uint32_t kPayloadLength = 1;
    static constexpr uint8_t  kTagLength     = 8;

    uint8_t encrypted[] = {
        0x2B, 0xDC, 0x84, 0x21, 0x43, 0x02, 0x00, 0x00, 0x00, 0x00, 0x48, 0xDE, 0xAC,
        0xFF, 0xFF, 0x01, 0x00, 0x00, 0x00, 0x00, 0x48, 0xDE, 0xAC, 0x06, 0x05, 0x00,
        0x00, 0x00, 0x01, 0xD8, 0x4F, 0xDE, 0x52, 0x90, 0x61, 0xF9, 0xC6, 0xF1,
    };

    uint8_t decrypted[] = {
        0x2B, 0xDC, 0x84, 0x21, 0x43, 0x02, 0x00, 0x00, 0x00, 0x00, 0x48, 0xDE, 0xAC,
        0xFF, 0xFF, 0x01, 0x00, 0x00, 0x00, 0x00, 0x48, 0xDE, 0xAC, 0x06, 0x05, 0x00,
        0x00, 0x00, 0x01, 0xCE, 0x4F, 0xDE, 0x52, 0x90, 0x61, 0xF9, 0xC6, 0xF1,
    };

    uint8_t nonce[] = {
        0xAC, 0xDE, 0x48, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x05, 0x06,
    };

    Instance      *instance = testInitInstance();
    Message       *message;
    Crypto::AesCcm aesCcm;
    uint8_t        plaintext[kPayloadLength] = {0xCE};
    uint8_t        ciphertext[kPayloadLength];
    uint8_t        tag[kTagLength];
    Crypto::Key    cryptoKey;

    printf("TestMacBeaconCommand\n");

    VerifyOrQuit(instance != nullptr);

    // Encrypt
    aesCcm.SetKey(key, sizeof(key));
    aesCcm.SetNonce(nonce, sizeof(nonce));
    aesCcm.SetAuthData(test, kHeaderLength);
    aesCcm.SetTagLength(kTagLength);
    SuccessOrQuit(aesCcm.Process(Crypto::AesCcm::kEncrypt, test + kHeaderLength, kPayloadLength));
    DumpBuffer("encrypted", test, sizeof(test));
    VerifyOrQuit(memcmp(test, encrypted, sizeof(encrypted)) == 0);

    // Decrypt
    aesCcm.SetKey(key, sizeof(key));
    aesCcm.SetNonce(nonce, sizeof(nonce));
    aesCcm.SetAuthData(test, kHeaderLength);
    aesCcm.SetTagLength(kTagLength);
    SuccessOrQuit(aesCcm.Process(Crypto::AesCcm::kDecrypt, test + kHeaderLength, kPayloadLength));
    DumpBuffer("decrypted", test, sizeof(test));
    VerifyOrQuit(memcmp(test, decrypted, sizeof(decrypted)) == 0);

    // Verify encryption/decryption in place within a message.

    message = instance->Get<MessagePool>().Allocate(Message::kTypeIp6);
    VerifyOrQuit(message != nullptr);

    SuccessOrQuit(message->AppendBytes(test, kHeaderLength + kPayloadLength));

    aesCcm.SetKey(key, sizeof(key));
    aesCcm.SetNonce(nonce, sizeof(nonce));
    aesCcm.SetAuthData(test, kHeaderLength);
    aesCcm.SetTagLength(kTagLength);
    SuccessOrQuit(aesCcm.Process(Crypto::AesCcm::kEncrypt, *message, kHeaderLength));
    VerifyOrQuit(message->GetLength() == sizeof(encrypted));
    VerifyOrQuit(message->Compare(0, encrypted));

    aesCcm.SetKey(key, sizeof(key));
    aesCcm.SetNonce(nonce, sizeof(nonce));
    aesCcm.SetAuthData(test, kHeaderLength);
    aesCcm.SetTagLength(kTagLength);
    SuccessOrQuit(aesCcm.Process(Crypto::AesCcm::kDecrypt, *message, kHeaderLength));
    VerifyOrQuit(message->GetLength() == sizeof(decrypted) - kTagLength);
    VerifyOrQuit(message->CompareBytes(0, decrypted, sizeof(decrypted) - kTagLength));

    message->Free();

    // Verify static `Perform()` with separate buffers
    cryptoKey.Set(key, sizeof(key));

    // Encrypt
    Crypto::AesCcm::Perform(Crypto::AesCcm::kEncrypt, cryptoKey, kTagLength, nonce, sizeof(nonce), test, kHeaderLength,
                            plaintext, ciphertext, kPayloadLength, tag);

    VerifyOrQuit(memcmp(ciphertext, encrypted + kHeaderLength, kPayloadLength) == 0);
    VerifyOrQuit(memcmp(tag, encrypted + kHeaderLength + kPayloadLength, kTagLength) == 0);

    // Decrypt
    memset(plaintext, 0, sizeof(plaintext));
    Crypto::AesCcm::Perform(Crypto::AesCcm::kDecrypt, cryptoKey, kTagLength, nonce, sizeof(nonce), test, kHeaderLength,
                            plaintext, ciphertext, kPayloadLength, tag);

    VerifyOrQuit(memcmp(plaintext, decrypted + kHeaderLength, kPayloadLength) == 0);
    VerifyOrQuit(memcmp(tag, decrypted + kHeaderLength + kPayloadLength, kTagLength) == 0);

    testFreeInstance(instance);

    printf("\nTestMacBeaconCommand PASSED\n\n");
}

void TestAesCcmMessageProcessing(void)
{
    static constexpr uint16_t kTagLength      = 4;
    static constexpr uint32_t kAuthDataLength = 19;
    static constexpr uint16_t kMaxBufferSize  = 1000;
    static constexpr uint16_t kNumIters       = 16;

    static const uint8_t kKey[] = {
        0xa0, 0xa1, 0xa2, 0xa3, 0xa4, 0xa5, 0xa6, 0xa7, 0xa8, 0xa9, 0xaa, 0xab, 0xac, 0xad, 0xae, 0xaf,
    };

    static const uint8_t kNonce[] = {
        0xac, 0xde, 0x48, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x05, 0x06,
    };

    static uint16_t kMessageLengths[] = {30, 400, 800};

    Instance      *instance = testInitInstance();
    uint16_t       payloadOffset;
    Message       *message;
    Message       *messageClone;
    uint8_t        buffer[kMaxBufferSize];
    Crypto::AesCcm aesCcm;

    printf("\nTestAesCcmMessageProcessing");

    VerifyOrQuit(instance != nullptr);

    message = instance->Get<MessagePool>().Allocate(Message::kTypeIp6);
    VerifyOrQuit(message != nullptr);

    aesCcm.SetKey(kKey, sizeof(kKey));
    aesCcm.SetNonce(kNonce, sizeof(kNonce));
    aesCcm.SetTagLength(kTagLength);

    for (uint16_t msgLength : kMessageLengths)
    {
        printf("\n\n- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -");
        printf("\nmsgLength %u\n", msgLength);

        SuccessOrQuit(message->SetLength(0));

        for (uint16_t i = 0; i < msgLength; i++)
        {
            SuccessOrQuit(message->Append<uint8_t>(static_cast<uint8_t>(i & 0xff) ^ static_cast<uint8_t>(i >> 8)));
        }

        messageClone = message->Clone<kNoReservedHeader>();
        VerifyOrQuit(messageClone != nullptr);
        VerifyOrQuit(messageClone->GetLength() == msgLength);

        SuccessOrQuit(message->Read(0, buffer, msgLength));
        payloadOffset = kAuthDataLength;

        printf("\nEncrypt `message`");
        aesCcm.SetAuthData(buffer, kAuthDataLength);
        SuccessOrQuit(aesCcm.Process(Crypto::AesCcm::kEncrypt, *message, payloadOffset));
        VerifyOrQuit(message->GetLength() == msgLength + kTagLength);

        printf("\nEncrypt same content in `buffer`");
        SuccessOrQuit(aesCcm.Process(Crypto::AesCcm::kEncrypt, buffer + payloadOffset, msgLength - payloadOffset));

        printf("\nValidate the two ways result in the same encrypted content and tag\n");
        VerifyOrQuit(message->CompareBytes(0, buffer, msgLength + kTagLength));

        // Corrupt the message by modifying a byte at some offset
        // (random offset or start/end of the tag)and
        // validate the decryption fails

        for (uint16_t iter = 0; iter < kNumIters; iter++)
        {
            Message *corruptedMsg = message->Clone<kNoReservedHeader>();
            uint16_t offset;
            uint8_t  byte;
            uint8_t  randomByte;

            VerifyOrQuit(corruptedMsg != nullptr);
            VerifyOrQuit(corruptedMsg->GetLength() == message->GetLength());

            switch (iter)
            {
            case 0:
                offset = msgLength; // Start of the tag
                break;
            case 1:
                offset = msgLength + kTagLength - 1; // End of the tag
                break;
            case 2:
                offset = msgLength + kTagLength / 2; // Middle of the tag
                break;
            default:
                offset = Random::NonCrypto::GenerateFromMinUpToExcluding(payloadOffset, msgLength);
                break;
            }

            SuccessOrQuit(corruptedMsg->Read<uint8_t>(offset, byte));

            do
            {
                randomByte = Random::NonCrypto::Generate<uint8_t>();
            } while (randomByte == byte);

            corruptedMsg->Write<uint8_t>(offset, randomByte);

            printf("\nCorrupt the message content - modify byte at offset %3u from 0x%02x to 0x%02x", offset, byte,
                   randomByte);

            VerifyOrQuit(aesCcm.Process(Crypto::AesCcm::kDecrypt, *corruptedMsg, payloadOffset) == kErrorSecurity);

            corruptedMsg->Free();
        }

        printf("\n\nDecrypt `message`");
        SuccessOrQuit(aesCcm.Process(Crypto::AesCcm::kDecrypt, *message, payloadOffset));
        VerifyOrQuit(message->GetLength() == msgLength);

        printf("\nDecrypt same content in `buffer`");
        SuccessOrQuit(aesCcm.Process(Crypto::AesCcm::kDecrypt, buffer + payloadOffset, msgLength - payloadOffset));

        printf("\nValidate the two ways result in the same decrypted content");
        VerifyOrQuit(message->CompareBytes(0, buffer, msgLength));

        printf("\nCheck that decrypted message is the same as original (cloned) message");
        VerifyOrQuit(message->CompareBytes(0, *messageClone, 0, msgLength));

        messageClone->Free();
    }

    message->Free();
    testFreeInstance(instance);

    printf("\nTestAesCcmMessageProcessing PASSED\n\n");
}

#if OPENTHREAD_CONFIG_CRYPTO_PLATFORM_CCM_ONE_SHOT_ENABLE

/**
 * Verifies `otPlatCryptoAesCcmProcessOneShot` directly and via `AesCcm::Process`,
 * using IEEE 802.15.4-2006 Annex C Section C.2.3
 * (MAC command frame: 29-byte header, 1-byte payload, 8-byte MIC).
 */
void TestPlatformCcmSinglePart(void)
{
    static const uint8_t kKey[] = {
        0xc0, 0xc1, 0xc2, 0xc3, 0xc4, 0xc5, 0xc6, 0xc7, 0xc8, 0xc9, 0xca, 0xcb, 0xcc, 0xcd, 0xce, 0xcf,
    };

    static const uint8_t kNonce[] = {
        0xAC, 0xDE, 0x48, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x05, 0x06,
    };

    static constexpr uint32_t kHeaderLength  = 29;
    static constexpr uint32_t kPayloadLength = 1;
    static constexpr uint8_t  kTagLength     = 8;
    static constexpr uint32_t kFrameLength   = kHeaderLength + kPayloadLength + kTagLength;

    static const uint8_t kPlainFrame[kHeaderLength + kPayloadLength] = {
        0x2B, 0xDC, 0x84, 0x21, 0x43, 0x02, 0x00, 0x00, 0x00, 0x00, 0x48, 0xDE, 0xAC, 0xFF, 0xFF,
        0x01, 0x00, 0x00, 0x00, 0x00, 0x48, 0xDE, 0xAC, 0x06, 0x05, 0x00, 0x00, 0x00, 0x01, 0xCE,
    };

    static const uint8_t kEncryptedFrame[kFrameLength] = {
        0x2B, 0xDC, 0x84, 0x21, 0x43, 0x02, 0x00, 0x00, 0x00, 0x00, 0x48, 0xDE, 0xAC,
        0xFF, 0xFF, 0x01, 0x00, 0x00, 0x00, 0x00, 0x48, 0xDE, 0xAC, 0x06, 0x05, 0x00,
        0x00, 0x00, 0x01, 0xD8, 0x4F, 0xDE, 0x52, 0x90, 0x61, 0xF9, 0xC6, 0xF1,
    };

    otInstance              *instance = testInitInstance();
    uint8_t                  frame[kFrameLength];
    otPlatCryptoAesCcmConfig config;

    printf("TestPlatformCcmSinglePart\n");

    VerifyOrQuit(instance != nullptr);

    config.mKey.mKey        = kKey;
    config.mKey.mKeyLength  = sizeof(kKey);
    config.mKey.mKeyRef     = 0;
    config.mNonce           = kNonce;
    config.mNonceLength     = sizeof(kNonce);
    config.mTagLength       = kTagLength;
    config.mHeaderLength    = kHeaderLength;
    config.mPlainTextLength = kPayloadLength;

    // Direct encrypt/decrypt round-trip through the platform hook.
    memcpy(frame, kPlainFrame, sizeof(kPlainFrame));
    memset(frame + kHeaderLength + kPayloadLength, 0, kTagLength);
    SuccessOrQuit(otPlatCryptoAesCcmProcessOneShot(true, &config, frame, frame + kHeaderLength));
    DumpBuffer("encrypted", frame, sizeof(frame));
    VerifyOrQuit(memcmp(frame, kEncryptedFrame, kFrameLength) == 0);

    SuccessOrQuit(otPlatCryptoAesCcmProcessOneShot(false, &config, frame, frame + kHeaderLength));
    DumpBuffer("decrypted", frame, kHeaderLength + kPayloadLength);
    VerifyOrQuit(memcmp(frame, kPlainFrame, kHeaderLength + kPayloadLength) == 0);

    // Tag corruption must be rejected.
    memcpy(frame, kPlainFrame, sizeof(kPlainFrame));
    memset(frame + kHeaderLength + kPayloadLength, 0, kTagLength);
    SuccessOrQuit(otPlatCryptoAesCcmProcessOneShot(true, &config, frame, frame + kHeaderLength));
    frame[kHeaderLength + kPayloadLength] ^= 0xFF;
    VerifyOrQuit(otPlatCryptoAesCcmProcessOneShot(false, &config, frame, frame + kHeaderLength) == kErrorSecurity);

    memcpy(frame, kPlainFrame, sizeof(kPlainFrame));
    memset(frame + kHeaderLength + kPayloadLength, 0, kTagLength);
    SuccessOrQuit(otPlatCryptoAesCcmProcessOneShot(true, &config, frame, frame + kHeaderLength));
    frame[kFrameLength - 1] ^= 0x01;
    VerifyOrQuit(otPlatCryptoAesCcmProcessOneShot(false, &config, frame, frame + kHeaderLength) == kErrorSecurity);

    // AesCcm::Process must produce the same result as the direct platform call.
    {
        uint8_t        directResult[kFrameLength];
        uint8_t        aesCcmResult[kFrameLength];
        Crypto::AesCcm aesCcm;

        memcpy(directResult, kPlainFrame, sizeof(kPlainFrame));
        memset(directResult + kHeaderLength + kPayloadLength, 0, kTagLength);
        SuccessOrQuit(otPlatCryptoAesCcmProcessOneShot(true, &config, directResult, directResult + kHeaderLength));

        memcpy(aesCcmResult, kPlainFrame, sizeof(kPlainFrame));
        memset(aesCcmResult + kHeaderLength + kPayloadLength, 0, kTagLength);
        aesCcm.SetKey(kKey, sizeof(kKey));
        aesCcm.SetNonce(kNonce, sizeof(kNonce));
        aesCcm.SetAuthData(aesCcmResult, kHeaderLength);
        aesCcm.SetTagLength(kTagLength);
        SuccessOrQuit(aesCcm.Process(Crypto::AesCcm::kEncrypt, aesCcmResult + kHeaderLength, kPayloadLength));
        VerifyOrQuit(memcmp(directResult, aesCcmResult, kFrameLength) == 0);
        VerifyOrQuit(memcmp(aesCcmResult, kEncryptedFrame, kFrameLength) == 0);

        aesCcm.SetKey(kKey, sizeof(kKey));
        aesCcm.SetNonce(kNonce, sizeof(kNonce));
        aesCcm.SetAuthData(aesCcmResult, kHeaderLength);
        aesCcm.SetTagLength(kTagLength);
        SuccessOrQuit(aesCcm.Process(Crypto::AesCcm::kDecrypt, aesCcmResult + kHeaderLength, kPayloadLength));
        VerifyOrQuit(memcmp(aesCcmResult, kPlainFrame, kHeaderLength + kPayloadLength) == 0);
    }

    testFreeInstance(instance);

    printf("\nTestPlatformCcmSinglePart PASSED\n\n");
}

#endif // OPENTHREAD_CONFIG_CRYPTO_PLATFORM_CCM_ONE_SHOT_ENABLE

} // namespace ot

int main(void)
{
    ot::TestMacBeaconFrame();
    ot::TestMacCommandFrame();
    ot::TestAesCcmMessageProcessing();
#if OPENTHREAD_CONFIG_CRYPTO_PLATFORM_CCM_ONE_SHOT_ENABLE
    ot::TestPlatformCcmSinglePart();
#endif
    printf("All tests passed\n");
    return 0;
}
