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
 *   This file implements AES-ECB.
 */

#include "aes_ecb.hpp"
#include <openthread/platform/psa.h>

namespace ot {
namespace Crypto {

AesEcb::AesEcb(void)
{
#if !OPENTHREAD_CONFIG_PSA_CRYPTO_ENABLE
    mbedtls_aes_init(&mContext);
#endif
}

#if OPENTHREAD_CONFIG_PSA_CRYPTO_ENABLE
void AesEcb::SetKey(const uint32_t aKeyRef)
{
    mKeyRef = aKeyRef;
}
#else
void AesEcb::SetKey(const uint8_t *aKey, uint16_t aKeyLength)
{
    mbedtls_aes_setkey_enc(&mContext, aKey, aKeyLength);
}
#endif

void AesEcb::Encrypt(const uint8_t aInput[kBlockSize], uint8_t aOutput[kBlockSize])
{
#if OPENTHREAD_CONFIG_PSA_CRYPTO_ENABLE
    otPlatPsaEcbEncrypt(mKeyRef, aInput, aOutput);
#else
    mbedtls_aes_crypt_ecb(&mContext, MBEDTLS_AES_ENCRYPT, aInput, aOutput);
#endif
}

AesEcb::~AesEcb(void)
{
#if !OPENTHREAD_CONFIG_PSA_CRYPTO_ENABLE
    mbedtls_aes_free(&mContext);
#endif
}

} // namespace Crypto
} // namespace ot
