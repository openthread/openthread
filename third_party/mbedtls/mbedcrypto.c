/*
 *    Copyright 2016 Nest Labs Inc. All Rights Reserved.
 *
 *    Licensed under the Apache License, Version 2.0 (the "License");
 *    you may not use this file except in compliance with the License.
 *    You may obtain a copy of the License at
 *
 *    http://www.apache.org/licenses/LICENSE-2.0
 *
 *    Unless required by applicable law or agreed to in writing, software
 *    distributed under the License is distributed on an "AS IS" BASIS,
 *    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *    See the License for the specific language governing permissions and
 *    limitations under the License.
 */

#include <assert.h>
#ifndef OPEN_THREAD_DRIVER
#include <stdbool.h>
#endif
#include <string.h>

#include <openthread-types.h>

#include <mbedtls/memory_buffer_alloc.h>
#include <mbedtls/aes.h>
#include <mbedtls/md.h>

#include <crypto/aes_ecb.h>
#include <crypto/hmac_sha256.h>

void mbedInit(otCryptoContext *aCryptoContext)
{
    if (aCryptoContext->mIsInitialized)
    {
        return;
    }

    mbedtls_memory_buffer_alloc_init(aCryptoContext->mMemoryBuf, sizeof(aCryptoContext->mMemoryBuf));
    aCryptoContext->mIsInitialized = true;
}

void otCryptoHmacSha256Start(otCryptoContext *aCryptoContext, const void *aKey, uint16_t aKeyLength)
{
    const mbedtls_md_info_t *mdInfo = NULL;

    mbedInit(aCryptoContext);

    mbedtls_md_init(&aCryptoContext->mSha256Context);
    mdInfo = mbedtls_md_info_from_type(MBEDTLS_MD_SHA256);
    mbedtls_md_setup(&aCryptoContext->mSha256Context, mdInfo, 1);
    mbedtls_md_hmac_starts(&aCryptoContext->mSha256Context, aKey, aKeyLength);
}

void otCryptoHmacSha256Update(otCryptoContext *aCryptoContext, const void *aBuf, uint16_t aBufLength)
{
    mbedtls_md_hmac_update(&aCryptoContext->mSha256Context, aBuf, aBufLength);
}

void otCryptoHmacSha256Finish(otCryptoContext *aCryptoContext, uint8_t aHash[otCryptoSha256Size])
{
    mbedtls_md_hmac_finish(&aCryptoContext->mSha256Context, aHash);
    mbedtls_md_free(&aCryptoContext->mSha256Context);
}

void otCryptoAesEcbSetKey(otCryptoContext *aCryptoContext, const void *aKey, uint16_t aKeyLength)
{
    mbedtls_aes_init(&aCryptoContext->mAesContext);
    mbedtls_aes_setkey_enc(&aCryptoContext->mAesContext, aKey, aKeyLength);
}

void otCryptoAesEcbEncrypt(otCryptoContext *aCryptoContext, const uint8_t aInput[otAesBlockSize], uint8_t aOutput[otAesBlockSize])
{
    mbedtls_aes_crypt_ecb(&aCryptoContext->mAesContext, MBEDTLS_AES_ENCRYPT, aInput, aOutput);
}
