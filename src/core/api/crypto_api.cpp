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
 *   This file implements the OpenThread Crypto API.
 */

#include "openthread-core-config.h"
#include <openthread/crypto.h>
#include <openthread/error.h>

#include "common/code_utils.hpp"
#include "common/debug.hpp"
#include "crypto/aes_ccm.hpp"
#include "crypto/ecdsa.hpp"
#include "crypto/hmac_sha256.hpp"

using namespace ot::Crypto;

void otCryptoHmacSha256(const uint8_t *aKey,
                        uint16_t       aKeyLength,
                        const uint8_t *aBuf,
                        uint16_t       aBufLength,
                        uint8_t *      aHash)
{
    HmacSha256 hmac;

    assert((aKey != NULL) && (aBuf != NULL) && (aHash != NULL));

    hmac.Start(aKey, aKeyLength);
    hmac.Update(aBuf, aBufLength);
    hmac.Finish(aHash);
}

void otCryptoAesCcm(const uint8_t *aKey,
                    uint16_t       aKeyLength,
                    uint8_t        aTagLength,
                    const void *   aNonce,
                    uint8_t        aNonceLength,
                    const void *   aHeader,
                    uint32_t       aHeaderLength,
                    void *         aPlainText,
                    void *         aCipherText,
                    uint32_t       aLength,
                    bool           aEncrypt,
                    void *         aTag)
{
    AesCcm  aesCcm;
    uint8_t tagLength;

    assert((aKey != NULL) && (aNonce != NULL) && (aPlainText != NULL) && (aCipherText != NULL) && (aTag != NULL));

    SuccessOrExit(aesCcm.SetKey(aKey, aKeyLength));
    SuccessOrExit(aesCcm.Init(aHeaderLength, aLength, aTagLength, aNonce, aNonceLength));

    if (aHeaderLength != 0)
    {
        assert(aHeader != NULL);
        aesCcm.Header(aHeader, aHeaderLength);
    }

    aesCcm.Payload(aPlainText, aCipherText, aLength, aEncrypt);
    aesCcm.Finalize(aTag, &tagLength);

    assert(aTagLength == tagLength);

exit:
    return;
}

#if OPENTHREAD_ENABLE_ECDSA

otError otCryptoEcdsaSign(uint8_t *      aOutput,
                          uint16_t *     aOutputLength,
                          const uint8_t *aInputHash,
                          uint16_t       aInputHashLength,
                          const uint8_t *aPrivateKey,
                          uint16_t       aPrivateKeyLength)
{
    return Ecdsa::Sign(aOutput, aOutputLength, aInputHash, aInputHashLength, aPrivateKey, aPrivateKeyLength);
}

#endif // OPENTHREAD_ENABLE_ECDSA
