/*
 *    Copyright (c) 2019, The OpenThread Authors.
 *    All rights reserved.
 *
 *    Redistribution and use in source and binary forms, with or without
 *    modification, are permitted provided that the following conditions are met:
 *    1. Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *    2. Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *    3. Neither the name of the copyright holder nor the
 *       names of its contributors may be used to endorse or promote products
 *       derived from this software without specific prior written permission.
 *
 *    THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 *    ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 *    WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 *    DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY
 *    DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 *    (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 *    LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 *    ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 *    (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 *    SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/**
 * @file
 *   This file implements the radio platform callbacks into OpenThread and default/weak radio platform APIs.
 */

#include <openthread/instance.h>
#include <openthread/platform/time.h>

#include <openthread/platform/crypto.h>
#include "common/code_utils.hpp"
#include "common/instance.hpp"
#include "radio/radio.hpp"

using namespace ot;

//---------------------------------------------------------------------------------------------------------------------
// Default/weak implementation of crypto platform APIs

OT_TOOL_WEAK otError otPlatCryptoImportKey( psa_key_id_t             *aKeyId,
                                            psa_key_type_t           aKeyType,
                                            psa_algorithm_t          aKeyAlgorithm,
                                            psa_key_usage_t          aKeyUsage,
                                            psa_key_persistence_t    aKeyPersistence,
                                            const uint8_t            *aKey,
                                            size_t                   aKeyLen)
{
    OT_UNUSED_VARIABLE(aKeyId);
    OT_UNUSED_VARIABLE(aKeyType);
    OT_UNUSED_VARIABLE(aKeyAlgorithm);
    OT_UNUSED_VARIABLE(aKeyUsage);
    OT_UNUSED_VARIABLE(aKeyPersistence);
    OT_UNUSED_VARIABLE(aKey);
    OT_UNUSED_VARIABLE(aKeyLen);

    return kErrorNotImplemented;
}

OT_TOOL_WEAK otError otPlatCryptoExportKey( psa_key_id_t aKeyId,
                                            uint8_t      *aBuffer,
                                            uint8_t      aBufferLen,
                                            size_t       *aKeyLen)
{
    OT_UNUSED_VARIABLE(aKeyId);
    OT_UNUSED_VARIABLE(aBuffer);
    OT_UNUSED_VARIABLE(aBufferLen);
    OT_UNUSED_VARIABLE(aKeyLen);

    return kErrorNotImplemented;
}

OT_TOOL_WEAK otError otPlatCryptoDestroyKey(psa_key_id_t aKeyId)
{
    OT_UNUSED_VARIABLE(aKeyId);
    
    return kErrorNotImplemented;
}

OT_TOOL_WEAK otError otPlatCryptoGenerateKey(   psa_key_id_t           *aKeyId,
                                                psa_key_type_t         aKeyType,
                                                psa_algorithm_t        aKeyAlgorithm,
                                                psa_key_usage_t        aKeyUsage,
                                                psa_key_persistence_t  aKeyPersistence,
                                                size_t                 aKeyLen)
{
    OT_UNUSED_VARIABLE(aKeyId);
    OT_UNUSED_VARIABLE(aKeyType);
    OT_UNUSED_VARIABLE(aKeyAlgorithm);
    OT_UNUSED_VARIABLE(aKeyUsage);
    OT_UNUSED_VARIABLE(aKeyPersistence);
    OT_UNUSED_VARIABLE(aKeyLen);

    return kErrorNotImplemented;
}

OT_TOOL_WEAK otError otPlatCryptoExportPublicKey(   psa_key_id_t   aKeyId,
                                                    uint8_t        *aOutput,
                                                    size_t         aOutputSize,
                                                    size_t         *aOutputLen)
{
    OT_UNUSED_VARIABLE(aKeyId);
    OT_UNUSED_VARIABLE(aOutput);
    OT_UNUSED_VARIABLE(aOutputSize);
    OT_UNUSED_VARIABLE(aOutputLen);

    return kErrorNotImplemented;
}

OT_TOOL_WEAK otError otPlatCryptoInit(void)
{
    return kErrorNone;
}

OT_TOOL_WEAK otError otPlatCryptoGetKeyAttributes(  psa_key_id_t          aKeyId,
                                                    psa_key_attributes_t  *aKeyAttributes)
{
    OT_UNUSED_VARIABLE(aKeyId);
    OT_UNUSED_VARIABLE(aKeyAttributes);

    return kErrorNotImplemented;
}

OT_TOOL_WEAK otCryptoType otPlatCryptoGetType(void)
{
    return OT_CRYPTO_TYPE_MBEDTLS;
}

// HMAC implementations

OT_TOOL_WEAK otError otPlatCryptoHmacSha256Init(void *aContext)
{
    const mbedtls_md_info_t *mdInfo = nullptr;
    mbedtls_md_context_t *mContext = (mbedtls_md_context_t *)aContext;

    mbedtls_md_init(mContext);
    mdInfo = mbedtls_md_info_from_type(MBEDTLS_MD_SHA256);
    mbedtls_md_setup(mContext, mdInfo, 1);

    return kErrorNone;    
}

OT_TOOL_WEAK otError otPlatCryptoHmacSha256UnInit(void *aContext)
{
    mbedtls_md_context_t *mContext = (mbedtls_md_context_t *)aContext;
    mbedtls_md_free(mContext);

    return kErrorNone;    
}

OT_TOOL_WEAK otError otPlatCryptoHmacSha256Start(void *aContext, otCryptoKey *aKey)
{
    mbedtls_md_context_t *mContext = (mbedtls_md_context_t *)aContext;
    mbedtls_md_hmac_starts(mContext, aKey->mKey, aKey->mKeyLength);

    return kErrorNone;
}

OT_TOOL_WEAK otError otPlatCryptoHmacSha256Update(void *aContext, const void *aBuf, uint16_t aBufLength)
{
    mbedtls_md_context_t *mContext = (mbedtls_md_context_t *)aContext;
    mbedtls_md_hmac_update(mContext, reinterpret_cast<const uint8_t *>(aBuf), aBufLength);

    return kErrorNone;
}

OT_TOOL_WEAK otError otPlatCryptoHmacSha256Finish(void *aContext, uint8_t *aBuf, size_t aBufLength)
{
    mbedtls_md_context_t *mContext = (mbedtls_md_context_t *)aContext;
    mbedtls_md_hmac_finish(mContext, aBuf);

    OT_UNUSED_VARIABLE(aBufLength);

    return kErrorNone;
}

// AES  Implementation
OT_TOOL_WEAK otError otPlatCryptoAesInit(void *aContext)
{
    mbedtls_aes_context *mContext = (mbedtls_aes_context *)aContext;
    mbedtls_aes_init(mContext);

    return kErrorNone;
}

OT_TOOL_WEAK otError otPlatCryptoAesSetKey(void *aContext, otCryptoKey *aKey)
{
    mbedtls_aes_context *mContext = (mbedtls_aes_context *)aContext;
    mbedtls_aes_setkey_enc(mContext, aKey->mKey, aKey->mKeyLength);

    return kErrorNone;
}

OT_TOOL_WEAK otError otPlatCryptoAesEncrypt(void *aContext, const uint8_t *aInput, uint8_t *aOutput)
{
    mbedtls_aes_context *mContext = (mbedtls_aes_context *)aContext;
    mbedtls_aes_crypt_ecb(mContext, MBEDTLS_AES_ENCRYPT, aInput, aOutput);

    return kErrorNone;
}

OT_TOOL_WEAK otError otPlatCryptoAesFree(void *aContext)
{
    mbedtls_aes_context *mContext = (mbedtls_aes_context *)aContext;
    mbedtls_aes_free(mContext);

    return kErrorNone;
}

// HKDF platform implementations
// As the HKDF does not actually use mbedTLS APIs but uses HMAC module, this feature is not implemented.
OT_TOOL_WEAK otError otPlatCryptoHkdfExpand( psa_key_derivation_operation_t *mOperationCtx,
                                             const uint8_t *aInfo, 
                                             uint16_t aInfoLength, 
                                             uint8_t *aOutputKey, 
                                             uint16_t aOutputKeyLength)
{
    OT_UNUSED_VARIABLE(mOperationCtx);
    OT_UNUSED_VARIABLE(aInfo);
    OT_UNUSED_VARIABLE(aInfoLength);
    OT_UNUSED_VARIABLE(aOutputKey);
    OT_UNUSED_VARIABLE(aOutputKeyLength);

    return kErrorNotImplemented;
}

OT_TOOL_WEAK otError otPlatCryptoHkdfExtract(   psa_key_derivation_operation_t *mOperationCtx, 
                                                const uint8_t *aSalt, 
                                                uint16_t aSaltLength, 
                                                otMacKeyRef aKey)
{
    OT_UNUSED_VARIABLE(mOperationCtx);
    OT_UNUSED_VARIABLE(aSalt);
    OT_UNUSED_VARIABLE(aSaltLength);
    OT_UNUSED_VARIABLE(aKey);

    return kErrorNotImplemented;
}

// SHA256 platform implementations
OT_TOOL_WEAK otError otPlatCryptoSha256Init(void *aOperationCtx)
{
    mbedtls_sha256_context *mContext = (mbedtls_sha256_context *)aOperationCtx;
    mbedtls_sha256_init(mContext);

    return kErrorNone;
}

OT_TOOL_WEAK otError otPlatCryptoSha256Uninit(void *aOperationCtx)
{
    mbedtls_sha256_context *mContext = (mbedtls_sha256_context *)aOperationCtx;
    mbedtls_sha256_free(mContext);

    return kErrorNone;
}

OT_TOOL_WEAK otError otPlatCryptoSha256Start(void *aOperationCtx)
{
    mbedtls_sha256_context *mContext = (mbedtls_sha256_context *)aOperationCtx;
    mbedtls_sha256_starts_ret(mContext, 0);

    return kErrorNone;
}

OT_TOOL_WEAK otError otPlatCryptoSha256Update(void *aOperationCtx, const void *aBuf, uint16_t aBufLength)
{
    mbedtls_sha256_context *mContext = (mbedtls_sha256_context *)aOperationCtx;
    mbedtls_sha256_update_ret(mContext, reinterpret_cast<const uint8_t *>(aBuf), aBufLength);

    return kErrorNone;
}

OT_TOOL_WEAK otError otPlatCryptoSha256Finish(void *aOperationCtx, uint8_t *aHash, uint16_t aHashSize)
{
    mbedtls_sha256_context *mContext = (mbedtls_sha256_context *)aOperationCtx;
    mbedtls_sha256_finish_ret(mContext, aHash);

    OT_UNUSED_VARIABLE(aHashSize);

    return kErrorNone;
}
