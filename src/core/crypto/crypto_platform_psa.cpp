/*
 *    Copyright (c) 2025, The OpenThread Authors.
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
 *   This file implements the default/weak Crypto platform APIs using ARM PSA API.
 */

#include "openthread-core-config.h"

#include <string.h>

#include <mbedtls/asn1.h>
#include <psa/crypto.h>

#include <openthread/instance.h>
#include <openthread/platform/crypto.h>
#include <openthread/platform/entropy.h>
#include <openthread/platform/memory.h>

#include "common/clearable.hpp"
#include "common/code_utils.hpp"
#include "common/debug.hpp"
#include "common/error.hpp"
#include "common/new.hpp"
#include "config/crypto.h"
#include "crypto/ecdsa.hpp"
#include "crypto/hmac_sha256.hpp"
#include "crypto/storage.hpp"
#include "instance/instance.hpp"

using namespace ot;
using namespace Crypto;

#if OPENTHREAD_CONFIG_CRYPTO_LIB == OPENTHREAD_CONFIG_CRYPTO_LIB_PSA

//---------------------------------------------------------------------------------------------------------------------
// Default/weak implementation of crypto platform APIs

static Error PsaToOtError(psa_status_t aStatus)
{
    Error error;

    switch (aStatus)
    {
    case PSA_SUCCESS:
        error = kErrorNone;
        break;
    case PSA_ERROR_INVALID_ARGUMENT:
        error = kErrorInvalidArgs;
        break;
    case PSA_ERROR_BUFFER_TOO_SMALL:
        error = kErrorNoBufs;
        break;
    default:
        error = kErrorFailed;
        break;
    }

    return error;
}

static psa_key_type_t ToPsaKeyType(otCryptoKeyType aType)
{
    psa_key_type_t type;

    switch (aType)
    {
    case OT_CRYPTO_KEY_TYPE_RAW:
        type = PSA_KEY_TYPE_RAW_DATA;
        break;
    case OT_CRYPTO_KEY_TYPE_AES:
        type = PSA_KEY_TYPE_AES;
        break;
    case OT_CRYPTO_KEY_TYPE_HMAC:
        type = PSA_KEY_TYPE_HMAC;
        break;
    case OT_CRYPTO_KEY_TYPE_ECDSA:
        type = PSA_KEY_TYPE_ECC_KEY_PAIR(PSA_ECC_FAMILY_SECP_R1);
        break;
    case OT_CRYPTO_KEY_TYPE_DERIVE:
        type = PSA_KEY_TYPE_DERIVE;
        break;
    default:
        type = PSA_KEY_TYPE_NONE;
        break;
    }

    return type;
}

static psa_algorithm_t ToPsaAlgorithm(otCryptoKeyAlgorithm aAlgorithm)
{
    psa_algorithm_t algorithm;

    switch (aAlgorithm)
    {
    case OT_CRYPTO_KEY_ALG_AES_ECB:
        algorithm = PSA_ALG_ECB_NO_PADDING;
        break;
    case OT_CRYPTO_KEY_ALG_HMAC_SHA_256:
        algorithm = PSA_ALG_HMAC(PSA_ALG_SHA_256);
        break;
    case OT_CRYPTO_KEY_ALG_ECDSA:
        algorithm = PSA_ALG_DETERMINISTIC_ECDSA(PSA_ALG_SHA_256);
        break;
    case OT_CRYPTO_KEY_ALG_HKDF_SHA256:
        algorithm = PSA_ALG_HKDF(PSA_ALG_SHA_256);
        break;
    default:
        algorithm = PSA_ALG_NONE;
        break;
    }

    return algorithm;
}

static psa_key_usage_t ToPsaKeyUsage(int aUsage)
{
    psa_key_usage_t usage = 0;

    if (aUsage & OT_CRYPTO_KEY_USAGE_EXPORT)
    {
        usage |= PSA_KEY_USAGE_EXPORT;
    }

    if (aUsage & OT_CRYPTO_KEY_USAGE_ENCRYPT)
    {
        usage |= PSA_KEY_USAGE_ENCRYPT;
    }

    if (aUsage & OT_CRYPTO_KEY_USAGE_DECRYPT)
    {
        usage |= PSA_KEY_USAGE_DECRYPT;
    }

    if (aUsage & OT_CRYPTO_KEY_USAGE_SIGN_HASH)
    {
        usage |= PSA_KEY_USAGE_SIGN_HASH;
    }

    if (aUsage & OT_CRYPTO_KEY_USAGE_VERIFY_HASH)
    {
        usage |= PSA_KEY_USAGE_VERIFY_HASH;
    }

    if (aUsage & OT_CRYPTO_KEY_USAGE_DERIVE)
    {
        usage |= PSA_KEY_USAGE_DERIVE;
    }

    return usage;
}

static Error ValidateKeyUsage(int aUsage)
{
    Error error = kErrorNone;

    // Check if only supported flags have been passed
    static constexpr int supportedFlags = OT_CRYPTO_KEY_USAGE_EXPORT | OT_CRYPTO_KEY_USAGE_ENCRYPT |
                                          OT_CRYPTO_KEY_USAGE_DECRYPT | OT_CRYPTO_KEY_USAGE_SIGN_HASH |
                                          OT_CRYPTO_KEY_USAGE_VERIFY_HASH | OT_CRYPTO_KEY_USAGE_DERIVE;

    VerifyOrExit((aUsage & ~supportedFlags) == 0, error = kErrorInvalidArgs);

exit:
    return error;
}

static Error ValidateContext(otCryptoContext *aContext, size_t aMinSize)
{
    Error error = kErrorNone;

    // Verify that the passed context is initialized and points to a big enough buffer
    VerifyOrExit(aContext != nullptr && aContext->mContext != nullptr && aContext->mContextSize >= aMinSize,
                 error = kErrorInvalidArgs);

exit:
    return error;
}

static Error ExtractPrivateKeyInfo(const uint8_t *aAsn1KeyPair,
                                   size_t         aAsn1KeyPairLen,
                                   size_t        *aKeyOffset,
                                   size_t        *aKeyLen)
{
    int                  ret;
    Error                error = kErrorNone;
    unsigned char       *p     = const_cast<unsigned char *>(aAsn1KeyPair);
    const unsigned char *end   = p + aAsn1KeyPairLen;
    size_t               len;

    // Parse the ASN.1 SEQUENCE headers
    ret = mbedtls_asn1_get_tag(&p, end, &len, MBEDTLS_ASN1_CONSTRUCTED | MBEDTLS_ASN1_SEQUENCE);
    VerifyOrExit(ret == 0, error = kErrorInvalidArgs);

    // Parse the version (INTEGER)
    ret = mbedtls_asn1_get_tag(&p, end, &len, MBEDTLS_ASN1_INTEGER);
    VerifyOrExit(ret == 0, error = kErrorInvalidArgs);

    // Skip the version.
    p += len;

    // Parse the private key (OCTET STRING)
    ret = mbedtls_asn1_get_tag(&p, end, &len, MBEDTLS_ASN1_OCTET_STRING);
    VerifyOrExit(ret == 0, error = kErrorInvalidArgs);

    // Check if the private key includes a padding byte (0x00)
    if (*p == 0x00)
    {
        p++;
        len--; // Skip the padding byte and reduce length by 1
    }

    *aKeyOffset = (p - aAsn1KeyPair);
    *aKeyLen    = len;

exit:
    return error;
}

OT_TOOL_WEAK void otPlatCryptoInit(void) { psa_crypto_init(); }

#if OPENTHREAD_CONFIG_HEAP_EXTERNAL_ENABLE
OT_TOOL_WEAK void *otPlatCryptoCAlloc(size_t aNum, size_t aSize) { return otPlatCAlloc(aNum, aSize); }
OT_TOOL_WEAK void  otPlatCryptoFree(void *aPtr) { otPlatFree(aPtr); }
#endif

OT_TOOL_WEAK otError otPlatCryptoImportKey(otCryptoKeyRef      *aKeyRef,
                                           otCryptoKeyType      aKeyType,
                                           otCryptoKeyAlgorithm aKeyAlgorithm,
                                           int                  aKeyUsage,
                                           otCryptoKeyStorage   aKeyPersistence,
                                           const uint8_t       *aKey,
                                           size_t               aKeyLen)
{
    Error                error      = kErrorNone;
    psa_key_attributes_t attributes = PSA_KEY_ATTRIBUTES_INIT;

    SuccessOrExit(error = ValidateKeyUsage(aKeyUsage));
    VerifyOrExit(aKeyRef != nullptr && aKey != nullptr, error = kErrorInvalidArgs);

    // PSA Crypto API expects the private key to be provided, not the full ASN1 buffer.
    if (aKeyType == OT_CRYPTO_KEY_TYPE_ECDSA)
    {
        size_t pkOffset;
        size_t pkLength;

        SuccessOrExit(error = ExtractPrivateKeyInfo(aKey, aKeyLen, &pkOffset, &pkLength));

        // Overwrite the content of the key.
        aKey += pkOffset;
        aKeyLen = pkLength;

        psa_set_key_bits(&attributes, 256);
    }

    psa_set_key_type(&attributes, ToPsaKeyType(aKeyType));
    psa_set_key_algorithm(&attributes, ToPsaAlgorithm(aKeyAlgorithm));
    psa_set_key_usage_flags(&attributes, ToPsaKeyUsage(aKeyUsage));

    switch (aKeyPersistence)
    {
    case OT_CRYPTO_KEY_STORAGE_PERSISTENT:
        psa_set_key_lifetime(&attributes, PSA_KEY_LIFETIME_PERSISTENT);
        psa_set_key_id(&attributes, *aKeyRef);
        break;
    case OT_CRYPTO_KEY_STORAGE_VOLATILE:
        psa_set_key_lifetime(&attributes, PSA_KEY_LIFETIME_VOLATILE);
        break;
    default:
        OT_ASSERT(false);
    }

    error = PsaToOtError(psa_import_key(&attributes, aKey, aKeyLen, aKeyRef));

exit:
    psa_reset_key_attributes(&attributes);

    return error;
}

OT_TOOL_WEAK otError otPlatCryptoExportKey(otCryptoKeyRef aKeyRef, uint8_t *aBuffer, size_t aBufferLen, size_t *aKeyLen)
{
    Error error = kErrorNone;

    VerifyOrExit(aBuffer != nullptr && aKeyLen != nullptr, error = kErrorInvalidArgs);

    error = PsaToOtError(psa_export_key(aKeyRef, aBuffer, aBufferLen, aKeyLen));

exit:
    return error;
}

OT_TOOL_WEAK otError otPlatCryptoDestroyKey(otCryptoKeyRef aKeyRef) { return PsaToOtError(psa_destroy_key(aKeyRef)); }

OT_TOOL_WEAK bool otPlatCryptoHasKey(otCryptoKeyRef aKeyRef)
{
    psa_key_attributes_t attributes = PSA_KEY_ATTRIBUTES_INIT;
    psa_status_t         status;

    status = psa_get_key_attributes(aKeyRef, &attributes);
    psa_reset_key_attributes(&attributes);

    return status == PSA_SUCCESS;
}

OT_TOOL_WEAK otError otPlatCryptoAesInit(otCryptoContext *aContext)
{
    Error         error = kErrorNone;
    psa_key_id_t *keyRef;

    SuccessOrExit(error = ValidateContext(aContext, sizeof(psa_key_id_t)));

    keyRef  = static_cast<psa_key_id_t *>(aContext->mContext);
    *keyRef = PSA_KEY_ID_NULL;

exit:
    return error;
}

OT_TOOL_WEAK otError otPlatCryptoAesSetKey(otCryptoContext *aContext, const otCryptoKey *aKey)
{
    Error         error = kErrorNone;
    psa_key_id_t *keyRef;

    SuccessOrExit(error = ValidateContext(aContext, sizeof(psa_key_id_t)));
    VerifyOrExit(aKey != nullptr, error = kErrorInvalidArgs);

    keyRef  = static_cast<psa_key_id_t *>(aContext->mContext);
    *keyRef = aKey->mKeyRef;

exit:
    return error;
}

OT_TOOL_WEAK otError otPlatCryptoAesEncrypt(otCryptoContext *aContext, const uint8_t *aInput, uint8_t *aOutput)
{
    Error         error     = kErrorNone;
    const size_t  blockSize = PSA_BLOCK_CIPHER_BLOCK_LENGTH(PSA_KEY_TYPE_AES);
    psa_status_t  status    = PSA_SUCCESS;
    psa_key_id_t *keyRef;
    size_t        cipherLen;

    SuccessOrExit(error = ValidateContext(aContext, sizeof(psa_key_id_t)));
    VerifyOrExit(aInput != nullptr && aOutput != nullptr, error = kErrorInvalidArgs);

    keyRef = static_cast<psa_key_id_t *>(aContext->mContext);
    status = psa_cipher_encrypt(*keyRef, PSA_ALG_ECB_NO_PADDING, aInput, blockSize, aOutput, blockSize, &cipherLen);

    error = PsaToOtError(status);

exit:
    return error;
}

OT_TOOL_WEAK otError otPlatCryptoAesFree(otCryptoContext *aContext)
{
    OT_UNUSED_VARIABLE(aContext);

    return kErrorNone;
}

#if OPENTHREAD_FTD || OPENTHREAD_MTD

OT_TOOL_WEAK otError otPlatCryptoHmacSha256Init(otCryptoContext *aContext)
{
    Error                error = kErrorNone;
    psa_mac_operation_t *operation;

    SuccessOrExit(error = ValidateContext(aContext, sizeof(psa_mac_operation_t)));

    operation = static_cast<psa_mac_operation_t *>(aContext->mContext);

    // Initialize the structure using memset as documented alternative for psa_mac_operation_init().
    ClearAllBytes(*operation);

exit:
    return error;
}

OT_TOOL_WEAK otError otPlatCryptoHmacSha256Deinit(otCryptoContext *aContext)
{
    Error                error = kErrorNone;
    psa_mac_operation_t *operation;

    SuccessOrExit(error = ValidateContext(aContext, sizeof(psa_mac_operation_t)));

    operation = static_cast<psa_mac_operation_t *>(aContext->mContext);

    error = PsaToOtError(psa_mac_abort(operation));

exit:
    return error;
}

OT_TOOL_WEAK otError otPlatCryptoHmacSha256Start(otCryptoContext *aContext, const otCryptoKey *aKey)
{
    Error                error = kErrorNone;
    psa_mac_operation_t *operation;

    SuccessOrExit(error = ValidateContext(aContext, sizeof(psa_mac_operation_t)));
    VerifyOrExit(aKey != nullptr, error = kErrorInvalidArgs);

    operation = static_cast<psa_mac_operation_t *>(aContext->mContext);

    error = PsaToOtError(psa_mac_sign_setup(operation, aKey->mKeyRef, PSA_ALG_HMAC(PSA_ALG_SHA_256)));

exit:
    return error;
}

OT_TOOL_WEAK otError otPlatCryptoHmacSha256Update(otCryptoContext *aContext, const void *aBuf, uint16_t aBufLength)
{
    Error                error = kErrorNone;
    psa_mac_operation_t *operation;

    SuccessOrExit(error = ValidateContext(aContext, sizeof(psa_mac_operation_t)));
    VerifyOrExit(aBuf != nullptr, error = kErrorInvalidArgs);

    operation = static_cast<psa_mac_operation_t *>(aContext->mContext);

    error = PsaToOtError(psa_mac_update(operation, static_cast<const uint8_t *>(aBuf), aBufLength));

exit:
    return error;
}

OT_TOOL_WEAK otError otPlatCryptoHmacSha256Finish(otCryptoContext *aContext, uint8_t *aBuf, size_t aBufLength)
{
    Error                error = kErrorNone;
    psa_mac_operation_t *operation;
    size_t               macLength;

    SuccessOrExit(error = ValidateContext(aContext, sizeof(psa_mac_operation_t)));
    VerifyOrExit(aBuf != nullptr, error = kErrorInvalidArgs);

    operation = static_cast<psa_mac_operation_t *>(aContext->mContext);

    error = PsaToOtError(psa_mac_sign_finish(operation, aBuf, aBufLength, &macLength));

exit:
    return error;
}

OT_TOOL_WEAK otError otPlatCryptoHkdfInit(otCryptoContext *aContext)
{
    Error                           error = kErrorNone;
    psa_key_derivation_operation_t *operation;

    SuccessOrExit(error = ValidateContext(aContext, sizeof(psa_key_derivation_operation_t)));

    operation = static_cast<psa_key_derivation_operation_t *>(aContext->mContext);

    *operation = PSA_KEY_DERIVATION_OPERATION_INIT;

    error = PsaToOtError(psa_key_derivation_setup(operation, PSA_ALG_HKDF(PSA_ALG_SHA_256)));

exit:
    return error;
}

OT_TOOL_WEAK otError otPlatCryptoHkdfExtract(otCryptoContext   *aContext,
                                             const uint8_t     *aSalt,
                                             uint16_t           aSaltLength,
                                             const otCryptoKey *aInputKey)
{
    Error                           error       = kErrorNone;
    psa_status_t                    status      = PSA_SUCCESS;
    psa_key_derivation_operation_t *operation   = nullptr;
    otCryptoKeyRef                  keyRef      = PSA_KEY_ID_NULL;
    psa_key_attributes_t            attributes  = PSA_KEY_ATTRIBUTES_INIT;
    psa_algorithm_t                 keyAlg      = PSA_ALG_NONE;
    size_t                          keyLength   = 0;
    constexpr size_t                kBufferSize = 80;
    uint8_t                         keyBuffer[kBufferSize];

    SuccessOrExit(error = ValidateContext(aContext, sizeof(psa_key_derivation_operation_t)));
    VerifyOrExit(aInputKey != nullptr, error = kErrorInvalidArgs);

    operation = static_cast<psa_key_derivation_operation_t *>(aContext->mContext);

    status = psa_key_derivation_input_bytes(operation, PSA_KEY_DERIVATION_INPUT_SALT, aSalt, aSaltLength);
    SuccessOrExit(error = PsaToOtError(status));

    status = psa_get_key_attributes(aInputKey->mKeyRef, &attributes);
    SuccessOrExit(error = PsaToOtError(status));

    keyAlg = psa_get_key_algorithm(&attributes);

    // The PSA API enforces a policy that restricts each key to a single algorithm. If the key is already HKDF-SHA256,
    // we can use it directly. Otherwise, export and re-import it as a volatile HKDF key. Note that this requires the
    // input key to have the PSA_KEY_USAGE_EXPORT flag.
    if (keyAlg != ToPsaAlgorithm(OT_CRYPTO_KEY_ALG_HKDF_SHA256))
    {
        SuccessOrExit(error = otPlatCryptoExportKey(aInputKey->mKeyRef, keyBuffer, sizeof(keyBuffer), &keyLength));
        SuccessOrExit(error = otPlatCryptoImportKey(&keyRef, OT_CRYPTO_KEY_TYPE_DERIVE, OT_CRYPTO_KEY_ALG_HKDF_SHA256,
                                                    OT_CRYPTO_KEY_USAGE_DERIVE, OT_CRYPTO_KEY_STORAGE_VOLATILE,
                                                    keyBuffer, keyLength));

        status = psa_key_derivation_input_key(operation, PSA_KEY_DERIVATION_INPUT_SECRET, keyRef);
        SuccessOrExit(error = PsaToOtError(status));
    }
    else
    {
        status = psa_key_derivation_input_key(operation, PSA_KEY_DERIVATION_INPUT_SECRET, aInputKey->mKeyRef);
        SuccessOrExit(error = PsaToOtError(status));
    }

exit:
    psa_reset_key_attributes(&attributes);
    psa_destroy_key(keyRef);

    return error;
}

OT_TOOL_WEAK otError otPlatCryptoHkdfExpand(otCryptoContext *aContext,
                                            const uint8_t   *aInfo,
                                            uint16_t         aInfoLength,
                                            uint8_t         *aOutputKey,
                                            uint16_t         aOutputKeyLength)
{
    Error                           error  = kErrorNone;
    psa_status_t                    status = PSA_SUCCESS;
    psa_key_derivation_operation_t *operation;

    SuccessOrExit(error = ValidateContext(aContext, sizeof(psa_key_derivation_operation_t)));
    VerifyOrExit(aOutputKey != nullptr, error = kErrorInvalidArgs);
    VerifyOrExit(aOutputKeyLength != 0, error = kErrorInvalidArgs);

    operation = static_cast<psa_key_derivation_operation_t *>(aContext->mContext);

    status = psa_key_derivation_input_bytes(operation, PSA_KEY_DERIVATION_INPUT_INFO, aInfo, aInfoLength);
    SuccessOrExit(error = PsaToOtError(status));

    status = psa_key_derivation_output_bytes(operation, aOutputKey, aOutputKeyLength);
    SuccessOrExit(error = PsaToOtError(status));

exit:
    return error;
}

OT_TOOL_WEAK otError otPlatCryptoHkdfDeinit(otCryptoContext *aContext)
{
    Error                           error = kErrorNone;
    psa_key_derivation_operation_t *operation;

    SuccessOrExit(error = ValidateContext(aContext, sizeof(psa_key_derivation_operation_t)));

    operation = static_cast<psa_key_derivation_operation_t *>(aContext->mContext);

    error = PsaToOtError(psa_key_derivation_abort(operation));

exit:
    return error;
}

OT_TOOL_WEAK otError otPlatCryptoSha256Init(otCryptoContext *aContext)
{
    Error                 error = kErrorNone;
    psa_hash_operation_t *operation;

    SuccessOrExit(error = ValidateContext(aContext, sizeof(psa_hash_operation_t)));

    operation = static_cast<psa_hash_operation_t *>(aContext->mContext);

    // Initialize the structure using memset as documented alternative for psa_hash_operation_init().
    ClearAllBytes(*operation);

exit:
    return error;
}

OT_TOOL_WEAK otError otPlatCryptoSha256Deinit(otCryptoContext *aContext)
{
    Error                 error = kErrorNone;
    psa_hash_operation_t *operation;

    SuccessOrExit(error = ValidateContext(aContext, sizeof(psa_hash_operation_t)));

    operation = static_cast<psa_hash_operation_t *>(aContext->mContext);

    error = PsaToOtError(psa_hash_abort(operation));

exit:
    return error;
}

OT_TOOL_WEAK otError otPlatCryptoSha256Start(otCryptoContext *aContext)
{
    Error                 error = kErrorNone;
    psa_hash_operation_t *operation;

    SuccessOrExit(error = ValidateContext(aContext, sizeof(psa_hash_operation_t)));

    operation = static_cast<psa_hash_operation_t *>(aContext->mContext);

    error = PsaToOtError(psa_hash_setup(operation, PSA_ALG_SHA_256));

exit:
    return error;
}

OT_TOOL_WEAK otError otPlatCryptoSha256Update(otCryptoContext *aContext, const void *aBuf, uint16_t aBufLength)
{
    Error                 error = kErrorNone;
    psa_hash_operation_t *operation;

    SuccessOrExit(error = ValidateContext(aContext, sizeof(psa_hash_operation_t)));
    VerifyOrExit(aBuf != nullptr, error = kErrorInvalidArgs);

    operation = static_cast<psa_hash_operation_t *>(aContext->mContext);

    error = PsaToOtError(psa_hash_update(operation, static_cast<const uint8_t *>(aBuf), aBufLength));

exit:
    return error;
}

OT_TOOL_WEAK otError otPlatCryptoSha256Finish(otCryptoContext *aContext, uint8_t *aHash, uint16_t aHashSize)
{
    Error                 error = kErrorNone;
    psa_hash_operation_t *operation;
    size_t                hashSize;

    SuccessOrExit(error = ValidateContext(aContext, sizeof(psa_hash_operation_t)));
    VerifyOrExit(aHash != nullptr, error = kErrorInvalidArgs);

    operation = static_cast<psa_hash_operation_t *>(aContext->mContext);

    error = PsaToOtError(psa_hash_finish(operation, aHash, aHashSize, &hashSize));

exit:
    return error;
}

OT_TOOL_WEAK void otPlatCryptoRandomInit(void) { psa_crypto_init(); }

OT_TOOL_WEAK void otPlatCryptoRandomDeinit(void)
{
    // Intentionally empty
}

OT_TOOL_WEAK otError otPlatCryptoRandomGet(uint8_t *aBuffer, uint16_t aSize)
{
    return PsaToOtError(psa_generate_random(aBuffer, aSize));
}

#if OPENTHREAD_CONFIG_ECDSA_ENABLE

OT_TOOL_WEAK otError otPlatCryptoEcdsaGenerateAndImportKey(otCryptoKeyRef aKeyRef)
{
    psa_key_attributes_t attributes = PSA_KEY_ATTRIBUTES_INIT;
    psa_status_t         status;
    psa_key_id_t         keyId = static_cast<psa_key_id_t>(aKeyRef);

    psa_set_key_usage_flags(&attributes, PSA_KEY_USAGE_VERIFY_HASH | PSA_KEY_USAGE_SIGN_HASH);
    psa_set_key_algorithm(&attributes, PSA_ALG_DETERMINISTIC_ECDSA(PSA_ALG_SHA_256));
    psa_set_key_type(&attributes, PSA_KEY_TYPE_ECC_KEY_PAIR(PSA_ECC_FAMILY_SECP_R1));
    psa_set_key_lifetime(&attributes, PSA_KEY_LIFETIME_PERSISTENT);
    psa_set_key_id(&attributes, keyId);
    psa_set_key_bits(&attributes, 256);

    status = psa_generate_key(&attributes, &keyId);
    VerifyOrExit(status == PSA_SUCCESS);

exit:
    psa_reset_key_attributes(&attributes);

    return PsaToOtError(status);
}

OT_TOOL_WEAK otError otPlatCryptoEcdsaExportPublicKey(otCryptoKeyRef aKeyRef, otPlatCryptoEcdsaPublicKey *aPublicKey)
{
    psa_status_t status;
    size_t       exportedLen;
    uint8_t      buffer[1 + OT_CRYPTO_ECDSA_PUBLIC_KEY_SIZE];

    status = psa_export_public_key(aKeyRef, buffer, sizeof(buffer), &exportedLen);
    VerifyOrExit(status == PSA_SUCCESS);

    OT_ASSERT(exportedLen == sizeof(buffer));
    memcpy(aPublicKey->m8, buffer + 1, OT_CRYPTO_ECDSA_PUBLIC_KEY_SIZE);

exit:
    return PsaToOtError(status);
}

OT_TOOL_WEAK otError otPlatCryptoEcdsaSignUsingKeyRef(otCryptoKeyRef                aKeyRef,
                                                      const otPlatCryptoSha256Hash *aHash,
                                                      otPlatCryptoEcdsaSignature   *aSignature)
{
    psa_status_t status;
    size_t       signatureLen;

    status = psa_sign_hash(aKeyRef, PSA_ALG_DETERMINISTIC_ECDSA(PSA_ALG_SHA_256), aHash->m8, OT_CRYPTO_SHA256_HASH_SIZE,
                           aSignature->m8, OT_CRYPTO_ECDSA_SIGNATURE_SIZE, &signatureLen);
    VerifyOrExit(status == PSA_SUCCESS);

    OT_ASSERT(signatureLen == OT_CRYPTO_ECDSA_SIGNATURE_SIZE);

exit:
    return PsaToOtError(status);
}

OT_TOOL_WEAK otError otPlatCryptoEcdsaVerify(const otPlatCryptoEcdsaPublicKey *aPublicKey,
                                             const otPlatCryptoSha256Hash     *aHash,
                                             const otPlatCryptoEcdsaSignature *aSignature)
{
    psa_key_attributes_t attributes = PSA_KEY_ATTRIBUTES_INIT;
    psa_key_id_t         keyId      = PSA_KEY_ID_NULL;
    psa_status_t         status;
    uint8_t              buffer[1 + OT_CRYPTO_ECDSA_PUBLIC_KEY_SIZE];

    psa_set_key_usage_flags(&attributes, PSA_KEY_USAGE_VERIFY_HASH);
    psa_set_key_algorithm(&attributes, PSA_ALG_DETERMINISTIC_ECDSA(PSA_ALG_SHA_256));
    psa_set_key_type(&attributes, PSA_KEY_TYPE_ECC_PUBLIC_KEY(PSA_ECC_FAMILY_SECP_R1));
    psa_set_key_bits(&attributes, 256);

    // `psa_import_key` expects a key format as specified by SEC1 &sect;2.3.3 for the
    // uncompressed representation of the ECPoint.
    buffer[0] = 0x04;
    memcpy(buffer + 1, aPublicKey->m8, OT_CRYPTO_ECDSA_PUBLIC_KEY_SIZE);

    status = psa_import_key(&attributes, buffer, sizeof(buffer), &keyId);
    VerifyOrExit(status == PSA_SUCCESS);

    status = psa_verify_hash(keyId, PSA_ALG_DETERMINISTIC_ECDSA(PSA_ALG_SHA_256), aHash->m8, OT_CRYPTO_SHA256_HASH_SIZE,
                             aSignature->m8, OT_CRYPTO_ECDSA_SIGNATURE_SIZE);
    VerifyOrExit(status == PSA_SUCCESS);

exit:
    psa_reset_key_attributes(&attributes);
    psa_destroy_key(keyId);

    return PsaToOtError(status);
}

OT_TOOL_WEAK otError otPlatCryptoEcdsaVerifyUsingKeyRef(otCryptoKeyRef                    aKeyRef,
                                                        const otPlatCryptoSha256Hash     *aHash,
                                                        const otPlatCryptoEcdsaSignature *aSignature)
{
    psa_status_t status;

    status = psa_verify_hash(aKeyRef, PSA_ALG_DETERMINISTIC_ECDSA(PSA_ALG_SHA_256), aHash->m8,
                             OT_CRYPTO_SHA256_HASH_SIZE, aSignature->m8, OT_CRYPTO_ECDSA_SIGNATURE_SIZE);
    VerifyOrExit(status == PSA_SUCCESS);

exit:
    return PsaToOtError(status);
}

#endif // #if OPENTHREAD_CONFIG_ECDSA_ENABLE

#endif // #if OPENTHREAD_FTD || OPENTHREAD_MTD

#if OPENTHREAD_FTD

OT_TOOL_WEAK otError otPlatCryptoPbkdf2GenerateKey(const uint8_t *aPassword,
                                                   uint16_t       aPasswordLen,
                                                   const uint8_t *aSalt,
                                                   uint16_t       aSaltLen,
                                                   uint32_t       aIterationCounter,
                                                   uint16_t       aKeyLen,
                                                   uint8_t       *aKey)
{
    psa_status_t                   status     = PSA_SUCCESS;
    psa_key_id_t                   key_id     = PSA_KEY_ID_NULL;
    psa_key_attributes_t           attributes = PSA_KEY_ATTRIBUTES_INIT;
    psa_algorithm_t                algorithm  = PSA_ALG_PBKDF2_AES_CMAC_PRF_128;
    psa_key_derivation_operation_t operation  = PSA_KEY_DERIVATION_OPERATION_INIT;

    psa_set_key_usage_flags(&attributes, PSA_KEY_USAGE_DERIVE);
    psa_set_key_lifetime(&attributes, PSA_KEY_LIFETIME_VOLATILE);
    psa_set_key_algorithm(&attributes, algorithm);
    psa_set_key_type(&attributes, PSA_KEY_TYPE_PASSWORD);
    psa_set_key_bits(&attributes, PSA_BYTES_TO_BITS(aPasswordLen));

    status = psa_import_key(&attributes, aPassword, aPasswordLen, &key_id);
    VerifyOrExit(status == PSA_SUCCESS);

    status = psa_key_derivation_setup(&operation, algorithm);
    VerifyOrExit(status == PSA_SUCCESS);

    status = psa_key_derivation_input_integer(&operation, PSA_KEY_DERIVATION_INPUT_COST, aIterationCounter);
    VerifyOrExit(status == PSA_SUCCESS);

    status = psa_key_derivation_input_bytes(&operation, PSA_KEY_DERIVATION_INPUT_SALT, aSalt, aSaltLen);
    VerifyOrExit(status == PSA_SUCCESS);

    status = psa_key_derivation_input_key(&operation, PSA_KEY_DERIVATION_INPUT_PASSWORD, key_id);
    VerifyOrExit(status == PSA_SUCCESS);

    status = psa_key_derivation_output_bytes(&operation, aKey, aKeyLen);
    VerifyOrExit(status == PSA_SUCCESS);

exit:
    psa_reset_key_attributes(&attributes);
    psa_key_derivation_abort(&operation);
    psa_destroy_key(key_id);

    return PsaToOtError(status);
}

#endif // #if OPENTHREAD_FTD

#endif // #if OPENTHREAD_CONFIG_CRYPTO_LIB == OPENTHREAD_CONFIG_CRYPTO_LIB_PSA
