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
 *   This file implements HMAC SHA-256.
 */

#include "hmac_sha256.hpp"

#include "common/message.hpp"

namespace ot {
namespace Crypto {

#if OPENTHREAD_CONFIG_PSA_CRYPTO_ENABLE
HmacSha256::HmacSha256(void)
{
    mOperation = PSA_MAC_OPERATION_INIT;
}

HmacSha256::~HmacSha256(void)
{
    psa_status_t error = psa_mac_abort(&mOperation);
    (void)error;
}

void HmacSha256::Start(uint32_t aKeyRef)
{
    psa_status_t error = psa_mac_sign_setup( &mOperation,
                                             aKeyRef,
                                             PSA_ALG_HMAC(PSA_ALG_SHA_256));

    (void)error;
}

void HmacSha256::Update(const void *aBuf, uint16_t aBufLength)
{
    psa_status_t error = psa_mac_update(  &mOperation,
                                          (const uint8_t *)aBuf,
                                          (size_t)aBufLength);
    (void)error;
}

void HmacSha256::Finish(Hash &aHash)
{
    size_t aMacLength = 0;

    psa_status_t error = psa_mac_sign_finish(&mOperation,
                                             aHash.m8,
                                             aHash.kSize,
                                             &aMacLength);
    (void)error;
}
#else
HmacSha256::HmacSha256(void)
{
    const mbedtls_md_info_t *mdInfo = nullptr;
    mbedtls_md_init(&mContext);
    mdInfo = mbedtls_md_info_from_type(MBEDTLS_MD_SHA256);
    mbedtls_md_setup(&mContext, mdInfo, 1);
}

HmacSha256::~HmacSha256(void)
{
    mbedtls_md_free(&mContext);
}

void HmacSha256::Start(const uint8_t *aKey, uint16_t aKeyLength)
{
    mbedtls_md_hmac_starts(&mContext, aKey, aKeyLength);
}

void HmacSha256::Update(const void *aBuf, uint16_t aBufLength)
{
    mbedtls_md_hmac_update(&mContext, reinterpret_cast<const uint8_t *>(aBuf), aBufLength);
}

void HmacSha256::Finish(Hash &aHash)
{
    mbedtls_md_hmac_finish(&mContext, aHash.m8);
}
#endif

void HmacSha256::Update(const Message &aMessage, uint16_t aOffset, uint16_t aLength)
{
    Message::Chunk chunk;

    aMessage.GetFirstChunk(aOffset, aLength, chunk);

    while (chunk.GetLength() > 0)
    {
        Update(chunk.GetData(), chunk.GetLength());
        aMessage.GetNextChunk(aLength, chunk);
    }
}
} // namespace Crypto
} // namespace ot
