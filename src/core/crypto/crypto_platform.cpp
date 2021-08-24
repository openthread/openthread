/*
 *    Copyright (c) 2021, The OpenThread Authors.
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
 *   This file implements the Crypto platform callbacks into OpenThread and default/weak Crypto platform APIs.
 */

#include <mbedtls/aes.h>
#include <mbedtls/md.h>

#include <openthread/instance.h>
#include <openthread/platform/time.h>

#include <openthread/platform/crypto.h>
#include "common/code_utils.hpp"
#include "common/instance.hpp"
#include "common/message.hpp"
#include "crypto/hmac_sha256.hpp"

using namespace ot;
using namespace Crypto;

//---------------------------------------------------------------------------------------------------------------------
// Default/weak implementation of crypto platform APIs

OT_TOOL_WEAK otError otPlatCryptoInit(void)
{
    return kErrorNone;
}

OT_TOOL_WEAK otError otPlatCryptoImportKey(otCryptoKeyRef *     aKeyRef,
                                           otCryptoKeyType      aKeyType,
                                           otCryptoKeyAlgorithm aKeyAlgorithm,
                                           int                  aKeyUsage,
                                           otCryptoKeyStorage   aKeyPersistence,
                                           const uint8_t *      aKey,
                                           size_t               aKeyLen)
{
    OT_UNUSED_VARIABLE(aKeyRef);
    OT_UNUSED_VARIABLE(aKeyType);
    OT_UNUSED_VARIABLE(aKeyAlgorithm);
    OT_UNUSED_VARIABLE(aKeyUsage);
    OT_UNUSED_VARIABLE(aKeyPersistence);
    OT_UNUSED_VARIABLE(aKey);
    OT_UNUSED_VARIABLE(aKeyLen);

    return kErrorNotImplemented;
}

OT_TOOL_WEAK otError otPlatCryptoExportKey(otCryptoKeyRef aKeyRef, uint8_t *aBuffer, size_t aBufferLen, size_t *aKeyLen)
{
    OT_UNUSED_VARIABLE(aKeyRef);
    OT_UNUSED_VARIABLE(aBuffer);
    OT_UNUSED_VARIABLE(aBufferLen);
    OT_UNUSED_VARIABLE(aKeyLen);

    return kErrorNotImplemented;
}

OT_TOOL_WEAK otError otPlatCryptoDestroyKey(otCryptoKeyRef aKeyRef)
{
    OT_UNUSED_VARIABLE(aKeyRef);

    return kErrorNotImplemented;
}

OT_TOOL_WEAK otError otPlatCryptoGetKeyAttributes(otCryptoKeyRef aKeyRef, otCryptoKeyAttributes *aKeyAttributes)
{
    OT_UNUSED_VARIABLE(aKeyRef);
    OT_UNUSED_VARIABLE(aKeyAttributes);

    return kErrorNotImplemented;
}

// AES  Implementation
OT_TOOL_WEAK otError otPlatCryptoAesInit(void *aContext, size_t aContextSize)
{
    Error                error   = kErrorNone;
    mbedtls_aes_context *context = static_cast<mbedtls_aes_context *>(aContext);

    VerifyOrExit(aContextSize >= sizeof(mbedtls_aes_context), error = kErrorFailed);
    mbedtls_aes_init(context);

exit:
    return error;
}

OT_TOOL_WEAK otError otPlatCryptoAesSetKey(void *aContext, size_t aContextSize, const otCryptoKey *aKey)
{
    Error                error   = kErrorNone;
    mbedtls_aes_context *context = static_cast<mbedtls_aes_context *>(aContext);

    VerifyOrExit(aContextSize >= sizeof(mbedtls_aes_context), error = kErrorFailed);
    VerifyOrExit((mbedtls_aes_setkey_enc(context, aKey->mKey, (aKey->mKeyLength * CHAR_BIT)) == 0),
                 error = kErrorFailed);

exit:
    return error;
}

OT_TOOL_WEAK otError otPlatCryptoAesEncrypt(void *         aContext,
                                            size_t         aContextSize,
                                            const uint8_t *aInput,
                                            uint8_t *      aOutput)
{
    Error                error   = kErrorNone;
    mbedtls_aes_context *context = static_cast<mbedtls_aes_context *>(aContext);

    VerifyOrExit(aContextSize >= sizeof(mbedtls_aes_context), error = kErrorFailed);
    VerifyOrExit((mbedtls_aes_crypt_ecb(context, MBEDTLS_AES_ENCRYPT, aInput, aOutput) == 0), error = kErrorFailed);

exit:
    return error;
}

OT_TOOL_WEAK otError otPlatCryptoAesFree(void *aContext, size_t aContextSize)
{
    Error                error   = kErrorNone;
    mbedtls_aes_context *context = static_cast<mbedtls_aes_context *>(aContext);

    VerifyOrExit(aContextSize >= sizeof(mbedtls_aes_context), error = kErrorFailed);
    mbedtls_aes_free(context);

exit:
    return error;
}

// HMAC implementations
OT_TOOL_WEAK otError otPlatCryptoHmacSha256Init(void *aContext, size_t aContextSize)
{
    Error error = kErrorNone;

#if !OPENTHREAD_RADIO
    const mbedtls_md_info_t *mdInfo  = nullptr;
    mbedtls_md_context_t *   context = static_cast<mbedtls_md_context_t *>(aContext);

    VerifyOrExit(aContextSize >= sizeof(mbedtls_md_context_t), error = kErrorFailed);
    mbedtls_md_init(context);
    mdInfo = mbedtls_md_info_from_type(MBEDTLS_MD_SHA256);
    VerifyOrExit((mbedtls_md_setup(context, mdInfo, 1) == 0), error = kErrorFailed);

#else

    OT_UNUSED_VARIABLE(aContext);
    OT_UNUSED_VARIABLE(aContextSize);

    ExitNow(error = kErrorNotImplemented);

#endif

exit:
    return error;
}

OT_TOOL_WEAK otError otPlatCryptoHmacSha256Deinit(void *aContext, size_t aContextSize)
{
    Error error = kErrorNone;

#if !OPENTHREAD_RADIO
    mbedtls_md_context_t *context = static_cast<mbedtls_md_context_t *>(aContext);

    VerifyOrExit(aContextSize >= sizeof(mbedtls_md_context_t), error = kErrorFailed);
    mbedtls_md_free(context);
#else

    OT_UNUSED_VARIABLE(aContext);
    OT_UNUSED_VARIABLE(aContextSize);

    ExitNow(error = kErrorNotImplemented);

#endif

exit:
    return error;
}

OT_TOOL_WEAK otError otPlatCryptoHmacSha256Start(void *aContext, size_t aContextSize, const otCryptoKey *aKey)
{
    Error error = kErrorNone;

#if !OPENTHREAD_RADIO
    mbedtls_md_context_t *context = static_cast<mbedtls_md_context_t *>(aContext);

    VerifyOrExit(aContextSize >= sizeof(mbedtls_md_context_t), error = kErrorFailed);
    VerifyOrExit((mbedtls_md_hmac_starts(context, aKey->mKey, aKey->mKeyLength) == 0), error = kErrorFailed);
#else

    OT_UNUSED_VARIABLE(aContext);
    OT_UNUSED_VARIABLE(aContextSize);
    OT_UNUSED_VARIABLE(aKey);

    ExitNow(error = kErrorNotImplemented);

#endif

exit:
    return error;
}

OT_TOOL_WEAK otError otPlatCryptoHmacSha256Update(void *      aContext,
                                                  size_t      aContextSize,
                                                  const void *aBuf,
                                                  uint16_t    aBufLength)
{
    Error error = kErrorNone;

#if !OPENTHREAD_RADIO
    mbedtls_md_context_t *context = static_cast<mbedtls_md_context_t *>(aContext);

    VerifyOrExit(aContextSize >= sizeof(mbedtls_md_context_t), error = kErrorFailed);
    VerifyOrExit((mbedtls_md_hmac_update(context, reinterpret_cast<const uint8_t *>(aBuf), aBufLength) == 0),
                 error = kErrorFailed);
#else

    OT_UNUSED_VARIABLE(aContext);
    OT_UNUSED_VARIABLE(aContextSize);
    OT_UNUSED_VARIABLE(aBuf);
    OT_UNUSED_VARIABLE(aBufLength);

    ExitNow(error = kErrorNotImplemented);
#endif

exit:
    return error;
}

OT_TOOL_WEAK otError otPlatCryptoHmacSha256Finish(void *aContext, size_t aContextSize, uint8_t *aBuf, size_t aBufLength)
{
    Error error = kErrorNone;

#if !OPENTHREAD_RADIO
    mbedtls_md_context_t *context = static_cast<mbedtls_md_context_t *>(aContext);

    VerifyOrExit(aContextSize >= sizeof(mbedtls_md_context_t), error = kErrorFailed);
    VerifyOrExit((mbedtls_md_hmac_finish(context, aBuf) == 0), error = kErrorFailed);

    OT_UNUSED_VARIABLE(aBufLength);

#else

    OT_UNUSED_VARIABLE(aContext);
    OT_UNUSED_VARIABLE(aContextSize);
    OT_UNUSED_VARIABLE(aBuf);
    OT_UNUSED_VARIABLE(aBufLength);

    ExitNow(error = kErrorNotImplemented);
#endif

exit:
    return error;
}

// HKDF platform implementations
// As the HKDF does not actually use mbedTLS APIs but uses HMAC module, this feature is not implemented.
OT_TOOL_WEAK otError otPlatCryptoHkdfExpand(void *         aContext,
                                            size_t         aContextSize,
                                            const uint8_t *aInfo,
                                            uint16_t       aInfoLength,
                                            uint8_t *      aOutputKey,
                                            uint16_t       aOutputKeyLength)
{
    Error error = kErrorNone;

#if !OPENTHREAD_RADIO
    HmacSha256        hmac;
    HmacSha256::Hash  hash;
    uint8_t           iter = 0;
    uint16_t          copyLength;
    HmacSha256::Hash *prk = static_cast<HmacSha256::Hash *>(aContext);

    VerifyOrExit(aContextSize >= sizeof(HmacSha256::Hash), error = kErrorFailed);

    // The aOutputKey is calculated as follows [RFC5889]:
    //
    //   N = ceil( aOutputKeyLength / HashSize)
    //   T = T(1) | T(2) | T(3) | ... | T(N)
    //   aOutputKey is first aOutputKeyLength of T
    //
    // Where:
    //   T(0) = empty string (zero length)
    //   T(1) = HMAC-Hash(PRK, T(0) | info | 0x01)
    //   T(2) = HMAC-Hash(PRK, T(1) | info | 0x02)
    //   T(3) = HMAC-Hash(PRK, T(2) | info | 0x03)
    //   ...

    while (aOutputKeyLength > 0)
    {
        Key cryptoKey;

        cryptoKey.Set(prk->GetBytes(), sizeof(HmacSha256::Hash));
        hmac.Start(cryptoKey);

        if (iter != 0)
        {
            hmac.Update(hash);
        }

        hmac.Update(aInfo, aInfoLength);

        iter++;
        hmac.Update(iter);
        hmac.Finish(hash);

        copyLength = (aOutputKeyLength > sizeof(hash)) ? sizeof(hash) : aOutputKeyLength;

        memcpy(aOutputKey, hash.GetBytes(), copyLength);
        aOutputKey += copyLength;
        aOutputKeyLength -= copyLength;
    }

    error = kErrorNone;
#else
    OT_UNUSED_VARIABLE(aContext);
    OT_UNUSED_VARIABLE(aContextSize);
    OT_UNUSED_VARIABLE(aInfo);
    OT_UNUSED_VARIABLE(aInfoLength);
    OT_UNUSED_VARIABLE(aOutputKey);
    OT_UNUSED_VARIABLE(aOutputKeyLength);

    ExitNow(error = kErrorNotImplemented);
#endif

exit:
    return error;
}

OT_TOOL_WEAK otError otPlatCryptoHkdfExtract(void *             aContext,
                                             size_t             aContextSize,
                                             const uint8_t *    aSalt,
                                             uint16_t           aSaltLength,
                                             const otCryptoKey *aKey)
{
    Error error = kErrorFailed;

#if !OPENTHREAD_RADIO
    HmacSha256        hmac;
    Key               cryptoKey;
    HmacSha256::Hash *prk = static_cast<HmacSha256::Hash *>(aContext);

    VerifyOrExit(aContextSize >= sizeof(HmacSha256::Hash), error = kErrorFailed);

    cryptoKey.Set(aSalt, aSaltLength);

    // PRK is calculated as HMAC-Hash(aSalt, aInputKey)
    hmac.Start(cryptoKey);

    hmac.Update(aKey->mKey, aKey->mKeyLength);
    hmac.Finish(*prk);

    error = kErrorNone;

#else

    OT_UNUSED_VARIABLE(aContext);
    OT_UNUSED_VARIABLE(aContextSize);
    OT_UNUSED_VARIABLE(aSalt);
    OT_UNUSED_VARIABLE(aSaltLength);
    OT_UNUSED_VARIABLE(aKey);

    ExitNow(error = kErrorNotImplemented);
#endif

exit:
    return error;
}

// SHA256 platform implementations
OT_TOOL_WEAK otError otPlatCryptoSha256Init(void *aContext, size_t aContextSize)
{
    Error error = kErrorNone;

#if !OPENTHREAD_RADIO
    mbedtls_sha256_context *context = static_cast<mbedtls_sha256_context *>(aContext);

    VerifyOrExit(aContextSize >= sizeof(mbedtls_sha256_context), error = kErrorFailed);
    mbedtls_sha256_init(context);
#else

    OT_UNUSED_VARIABLE(aContext);
    OT_UNUSED_VARIABLE(aContextSize);

    ExitNow(error = kErrorNotImplemented);
#endif

exit:
    return error;
}

OT_TOOL_WEAK otError otPlatCryptoSha256Deinit(void *aContext, size_t aContextSize)
{
    Error error = kErrorNone;

#if !OPENTHREAD_RADIO
    mbedtls_sha256_context *context = static_cast<mbedtls_sha256_context *>(aContext);

    VerifyOrExit(aContextSize >= sizeof(mbedtls_sha256_context), error = kErrorFailed);
    mbedtls_sha256_free(context);
#else

    OT_UNUSED_VARIABLE(aContext);
    OT_UNUSED_VARIABLE(aContextSize);

    ExitNow(error = kErrorNotImplemented);
#endif

exit:
    return error;
}

OT_TOOL_WEAK otError otPlatCryptoSha256Start(void *aContext, size_t aContextSize)
{
    Error error = kErrorNone;

#if !OPENTHREAD_RADIO
    mbedtls_sha256_context *context = static_cast<mbedtls_sha256_context *>(aContext);

    VerifyOrExit(aContextSize >= sizeof(mbedtls_sha256_context), error = kErrorFailed);
    VerifyOrExit((mbedtls_sha256_starts_ret(context, 0) == 0), error = kErrorFailed);
#else

    OT_UNUSED_VARIABLE(aContext);
    OT_UNUSED_VARIABLE(aContextSize);

    ExitNow(error = kErrorNotImplemented);
#endif

exit:
    return error;
}

OT_TOOL_WEAK otError otPlatCryptoSha256Update(void *      aContext,
                                              size_t      aContextSize,
                                              const void *aBuf,
                                              uint16_t    aBufLength)
{
    Error error = kErrorNone;

#if !OPENTHREAD_RADIO
    mbedtls_sha256_context *context = static_cast<mbedtls_sha256_context *>(aContext);

    VerifyOrExit(aContextSize >= sizeof(mbedtls_sha256_context), error = kErrorFailed);
    VerifyOrExit((mbedtls_sha256_update_ret(context, reinterpret_cast<const uint8_t *>(aBuf), aBufLength) == 0),
                 error = kErrorFailed);
#else

    OT_UNUSED_VARIABLE(aContext);
    OT_UNUSED_VARIABLE(aContextSize);
    OT_UNUSED_VARIABLE(aBuf);
    OT_UNUSED_VARIABLE(aBufLength);

    ExitNow(error = kErrorNotImplemented);
#endif

exit:
    return error;
}

OT_TOOL_WEAK otError otPlatCryptoSha256Finish(void *aContext, size_t aContextSize, uint8_t *aHash, uint16_t aHashSize)
{
    Error error = kErrorNone;

    OT_UNUSED_VARIABLE(aHashSize);

#if !OPENTHREAD_RADIO
    mbedtls_sha256_context *context = static_cast<mbedtls_sha256_context *>(aContext);

    VerifyOrExit(aContextSize >= sizeof(mbedtls_sha256_context), error = kErrorFailed);
    VerifyOrExit((mbedtls_sha256_finish_ret(context, aHash) == 0), error = kErrorFailed);
#else

    OT_UNUSED_VARIABLE(aContext);
    OT_UNUSED_VARIABLE(aHash);
    OT_UNUSED_VARIABLE(aContextSize);

    ExitNow(error = kErrorNotImplemented);
#endif

exit:
    return error;
}
