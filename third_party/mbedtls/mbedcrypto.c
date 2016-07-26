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
#include <stdbool.h>
#include <string.h>

#include <openthread-types.h>

#include <mbedtls/memory_buffer_alloc.h>
#include <mbedtls/aes.h>
#include <mbedtls/md.h>

#include <crypto/aes_ecb.h>
#include <crypto/hmac_sha256.h>

/**
 * @def MBED_MEMORY_BUF_SIZE
 *
 * The size of the memory buffer used by mbedtls.
 *
 */
#define MBED_MEMORY_BUF_SIZE  512

void mbedInit(void);

static bool sIsInitialized = false;
static unsigned char sMemoryBuf[MBED_MEMORY_BUF_SIZE];

static mbedtls_aes_context sAesContext;
static mbedtls_md_context_t sSha256Context;

void mbedInit(void)
{
    if (sIsInitialized)
    {
        return;
    }

    mbedtls_memory_buffer_alloc_init(sMemoryBuf, sizeof(sMemoryBuf));
    sIsInitialized = true;
}

void otCryptoHmacSha256Start(const void *aKey, uint16_t aKeyLength)
{
    const mbedtls_md_info_t *mdInfo = NULL;

    mbedInit();

    mbedtls_md_init(&sSha256Context);
    mdInfo = mbedtls_md_info_from_type(MBEDTLS_MD_SHA256);
    mbedtls_md_setup(&sSha256Context, mdInfo, 1);
    mbedtls_md_hmac_starts(&sSha256Context, aKey, aKeyLength);
}

void otCryptoHmacSha256Update(const void *aBuf, uint16_t aBufLength)
{
    mbedtls_md_hmac_update(&sSha256Context, aBuf, aBufLength);
}

void otCryptoHmacSha256Finish(uint8_t aHash[otCryptoSha256Size])
{
    mbedtls_md_hmac_finish(&sSha256Context, aHash);
    mbedtls_md_free(&sSha256Context);
}

void otCryptoAesEcbSetKey(const void *aKey, uint16_t aKeyLength)
{
    mbedtls_aes_init(&sAesContext);
    mbedtls_aes_setkey_enc(&sAesContext, aKey, aKeyLength);
}

void otCryptoAesEcbEncrypt(const uint8_t aInput[otAesBlockSize], uint8_t aOutput[otAesBlockSize])
{
    mbedtls_aes_crypt_ecb(&sAesContext, MBEDTLS_AES_ENCRYPT, aInput, aOutput);
}
