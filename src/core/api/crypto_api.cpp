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

#include "crypto/aes_ccm.hpp"
#include "crypto/ecdsa.hpp"
#include "crypto/hmac_sha256.hpp"
#include "instance/instance.hpp"

using namespace ot;
using namespace ot::Crypto;

void otCryptoHmacSha256(const otCryptoKey *aKey, const uint8_t *aBuf, uint16_t aBufLength, otCryptoSha256Hash *aHash)
{
    HmacSha256 hmac;

    AssertPointerIsNotNull(aBuf);

    hmac.Start(AsCoreType(aKey));
    hmac.Update(aBuf, aBufLength);
    hmac.Finish(AsCoreType(aHash));
}

void otCryptoAesCcm(const otCryptoKey *aKey,
                    uint8_t            aTagLength,
                    const void        *aNonce,
                    uint8_t            aNonceLength,
                    const void        *aHeader,
                    uint32_t           aHeaderLength,
                    void              *aPlainText,
                    void              *aCipherText,
                    uint32_t           aLength,
                    bool               aEncrypt,
                    void              *aTag)
{
    AesCcm aesCcm;

    AssertPointerIsNotNull(aNonce);
    AssertPointerIsNotNull(aPlainText);
    AssertPointerIsNotNull(aCipherText);
    AssertPointerIsNotNull(aTag);

    aesCcm.SetKey(AsCoreType(aKey));
    aesCcm.Init(aHeaderLength, aLength, aTagLength, aNonce, aNonceLength);

    if (aHeaderLength != 0)
    {
        OT_ASSERT(aHeader != nullptr);
        aesCcm.Header(aHeader, aHeaderLength);
    }

    aesCcm.Payload(aPlainText, aCipherText, aLength, aEncrypt ? AesCcm::kEncrypt : AesCcm::kDecrypt);
    aesCcm.Finalize(aTag);
}
