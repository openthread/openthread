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
                      0x53, 0x54, 0x22, 0x3B, 0xC1, 0xEC, 0x84, 0x1A, 0xB5, 0x53};

    uint8_t encrypted[] = {0x08, 0xD0, 0x84, 0x21, 0x43, 0x01, 0x00, 0x00, 0x00, 0x00, 0x48, 0xDE,
                           0xAC, 0x02, 0x05, 0x00, 0x00, 0x00, 0x55, 0xCF, 0x00, 0x00, 0x51, 0x52,
                           0x53, 0x54, 0x22, 0x3B, 0xC1, 0xEC, 0x84, 0x1A, 0xB5, 0x53};

    uint8_t decrypted[] = {0x08, 0xD0, 0x84, 0x21, 0x43, 0x01, 0x00, 0x00, 0x00, 0x00, 0x48, 0xDE,
                           0xAC, 0x02, 0x05, 0x00, 0x00, 0x00, 0x55, 0xCF, 0x00, 0x00, 0x51, 0x52,
                           0x53, 0x54, 0x22, 0x3B, 0xC1, 0xEC, 0x84, 0x1A, 0xB5, 0x53};

    otInstance    *instance = testInitInstance();
    Crypto::AesCcm aesCcm;
    uint32_t       headerLength  = sizeof(test) - 8;
    uint32_t       payloadLength = 0;
    uint8_t        tagLength     = 8;

    uint8_t nonce[] = {
        0xAC, 0xDE, 0x48, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x05, 0x02,
    };

    VerifyOrQuit(instance != nullptr);

    aesCcm.SetKey(key, sizeof(key));
    aesCcm.Init(headerLength, payloadLength, tagLength, nonce, sizeof(nonce));
    aesCcm.Header(test, headerLength);
    VerifyOrQuit(aesCcm.GetTagLength() == tagLength);
    aesCcm.Finalize(test + headerLength);

    VerifyOrQuit(memcmp(test, encrypted, sizeof(encrypted)) == 0);

    aesCcm.Init(headerLength, payloadLength, tagLength, nonce, sizeof(nonce));
    aesCcm.Header(test, headerLength);
    VerifyOrQuit(aesCcm.GetTagLength() == tagLength);
    aesCcm.Finalize(test + headerLength);

    VerifyOrQuit(memcmp(test, decrypted, sizeof(decrypted)) == 0);

    testFreeInstance(instance);
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

    uint8_t tag[kTagLength];

    Instance      *instance = testInitInstance();
    Message       *message;
    Crypto::AesCcm aesCcm;

    VerifyOrQuit(instance != nullptr);

    aesCcm.SetKey(key, sizeof(key));
    aesCcm.Init(kHeaderLength, kPayloadLength, kTagLength, nonce, sizeof(nonce));
    aesCcm.Header(test, kHeaderLength);
    aesCcm.Payload(test + kHeaderLength, test + kHeaderLength, kPayloadLength, Crypto::AesCcm::kEncrypt);
    VerifyOrQuit(aesCcm.GetTagLength() == kTagLength);
    aesCcm.Finalize(test + kHeaderLength + kPayloadLength);
    VerifyOrQuit(memcmp(test, encrypted, sizeof(encrypted)) == 0);

    aesCcm.Init(kHeaderLength, kPayloadLength, kTagLength, nonce, sizeof(nonce));
    aesCcm.Header(test, kHeaderLength);
    aesCcm.Payload(test + kHeaderLength, test + kHeaderLength, kPayloadLength, Crypto::AesCcm::kDecrypt);
    VerifyOrQuit(aesCcm.GetTagLength() == kTagLength);
    aesCcm.Finalize(test + kHeaderLength + kPayloadLength);

    VerifyOrQuit(memcmp(test, decrypted, sizeof(decrypted)) == 0);

    // Verify encryption/decryption in place within a message.

    message = instance->Get<MessagePool>().Allocate(Message::kTypeIp6);
    VerifyOrQuit(message != nullptr);

    SuccessOrQuit(message->AppendBytes(test, kHeaderLength + kPayloadLength));

    aesCcm.Init(kHeaderLength, kPayloadLength, kTagLength, nonce, sizeof(nonce));
    aesCcm.Header(test, kHeaderLength);

    aesCcm.Payload(*message, kHeaderLength, kPayloadLength, Crypto::AesCcm::kEncrypt);
    VerifyOrQuit(aesCcm.GetTagLength() == kTagLength);
    aesCcm.Finalize(tag);
    SuccessOrQuit(message->Append(tag));
    VerifyOrQuit(message->GetLength() == sizeof(encrypted));
    VerifyOrQuit(message->Compare(0, encrypted));

    aesCcm.Init(kHeaderLength, kPayloadLength, kTagLength, nonce, sizeof(nonce));
    aesCcm.Header(test, kHeaderLength);
    aesCcm.Payload(*message, kHeaderLength, kPayloadLength, Crypto::AesCcm::kDecrypt);

    VerifyOrQuit(message->GetLength() == sizeof(encrypted));
    VerifyOrQuit(message->Compare(0, decrypted));

    message->Free();
    testFreeInstance(instance);
}

/**
 * Verifies in-place encryption/decryption.
 */
void TestInPlaceAesCcmProcessing(void)
{
    static constexpr uint16_t kTagLength    = 4;
    static constexpr uint32_t kHeaderLength = 19;

    static const uint8_t kKey[] = {
        0xa0, 0xa1, 0xa2, 0xa3, 0xa4, 0xa5, 0xa6, 0xa7, 0xa8, 0xa9, 0xaa, 0xab, 0xac, 0xad, 0xae, 0xaf,
    };

    static const uint8_t kNonce[] = {
        0xac, 0xde, 0x48, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x05, 0x06,
    };

    static uint16_t kMessageLengths[] = {30, 400, 800};

    uint8_t tag[kTagLength];
    uint8_t header[kHeaderLength];

    Crypto::AesCcm aesCcm;
    Instance      *instance = testInitInstance();
    Message       *message;
    Message       *messageClone;

    VerifyOrQuit(instance != nullptr);

    message = instance->Get<MessagePool>().Allocate(Message::kTypeIp6);
    VerifyOrQuit(message != nullptr);

    aesCcm.SetKey(kKey, sizeof(kKey));

    for (uint16_t msgLength : kMessageLengths)
    {
        printf("msgLength %d\n", msgLength);

        SuccessOrQuit(message->SetLength(0));

        for (uint16_t i = msgLength; i != 0; i--)
        {
            SuccessOrQuit(message->Append<uint8_t>(i & 0xff));
        }

        messageClone = message->Clone();
        VerifyOrQuit(messageClone != nullptr);
        VerifyOrQuit(messageClone->GetLength() == msgLength);

        SuccessOrQuit(message->Read(0, header));

        // Encrypt in place
        aesCcm.Init(kHeaderLength, msgLength - kHeaderLength, kTagLength, kNonce, sizeof(kNonce));
        aesCcm.Header(header);
        aesCcm.Payload(*message, kHeaderLength, msgLength - kHeaderLength, Crypto::AesCcm::kEncrypt);

        // Append the tag
        aesCcm.Finalize(tag);
        SuccessOrQuit(message->Append(tag));

        VerifyOrQuit(message->GetLength() == msgLength + kTagLength);

        // Decrypt in place
        aesCcm.Init(kHeaderLength, msgLength - kHeaderLength, kTagLength, kNonce, sizeof(kNonce));
        aesCcm.Header(header);
        aesCcm.Payload(*message, kHeaderLength, msgLength - kHeaderLength, Crypto::AesCcm::kDecrypt);

        // Check the tag against what is the message
        aesCcm.Finalize(tag);
        VerifyOrQuit(message->Compare(msgLength, tag));

        // Check that decrypted message is the same as original (cloned) message
        VerifyOrQuit(message->CompareBytes(0, *messageClone, 0, msgLength));

        messageClone->Free();
    }

    message->Free();
    testFreeInstance(instance);
}

} // namespace ot

int main(void)
{
    ot::TestMacBeaconFrame();
    ot::TestMacCommandFrame();
    ot::TestInPlaceAesCcmProcessing();
    printf("All tests passed\n");
    return 0;
}
