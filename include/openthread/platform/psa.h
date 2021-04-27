/*
 *  Copyright (c) 2021, The OpenThread Authors.
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
 * @brief
 *   This file includes the platform abstraction for PSA Crypto operations.
 */

#ifndef OPENTHREAD_PLATFORM_PSA_H_
#define OPENTHREAD_PLATFORM_PSA_H_

#if OPENTHREAD_CONFIG_PSA_CRYPTO_ENABLE

#include <stdint.h>

#include <openthread/error.h>
#include "psa/crypto.h"
#include <psa/crypto_types.h>
#include "psa/crypto_values.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @addtogroup plat-psa
 *
 * @brief
 *   This module includes the platform abstraction for PSA Crypto.
 *
 * @{
 *
 */

/**
 * Initialise the PSA module.
 *
 * @retval OT_ERROR_NONE          Successfully encrypted  @p aInput.
 * @retval OT_ERROR_FAILED        Failed to encrypt @p aInput.
 *
 */
otError otPlatPsaInit(void);

/**
 * Encrypt the given data using AES-ECB cipher.
 *
 * @param[in]   aKeyId           Reference to the key to be used for crypto operations.
 * @param[out]  aInput            A pointer to the input buffer.  Must not be NULL.
 * @param[in]   aOutput           A pointer to the output buffer.  Must not be NULL.
 *
 * @retval OT_ERROR_NONE          Successfully encrypted  @p aInput.
 * @retval OT_ERROR_FAILED        Failed to encrypt @p aInput.
 * @retval OT_ERROR_INVALID_ARGS  @p aInput or @p aOutput was set to NULL.
 *
 */
otError otPlatPsaEcbEncrypt(psa_key_id_t aKeyId, const uint8_t *aInput, uint8_t *aOutput);

/**
 * Generate a key for the parameters specified and store it in PSA ITS.
 *
 * @param[out]  aKeyId            Reference to the key to be used for crypto operations.
 * @param[in]   aKeyType          Key Type encoding for the key.
 * @param[in]   aKeyAlgorithm     Key algorithm encoding for the key.
 * @param[in]   aKeyUsage         Key Usage encoding for the key.
 * @param[in]   aKeyPersistence   Key Persistence for this key
 * @param[in]   aKeyLen           Length of the key to be generated.
 *
 * @retval OT_ERROR_NONE          Successfully encrypted  @p aInput.
 * @retval OT_ERROR_FAILED        Failed to encrypt @p aInput.
 * @retval OT_ERROR_INVALID_ARGS  @p aInput or @p aOutput was set to NULL.
 *
 */
otError otPlatPsaGenerateKey(psa_key_id_t           *aKeyId,
                             psa_key_type_t         aKeyType,
                             psa_algorithm_t        aKeyAlgorithm,
                             psa_key_usage_t        aKeyUsage,
                             psa_key_persistence_t  aKeyPersistence,
                             size_t                 aKeyLen);

/**
 * Import a key into PSA ITS.
 *
 * @param[out]  aKeyId            Reference to the key to be used for crypto operations.
 * @param[in]   aKeyType          Key Type encoding for the key.
 * @param[in]   aKeyAlgorithm     Key algorithm encoding for the key.
 * @param[in]   aKeyUsage         Key Usage encoding for the key.
 * @param[in]   aKeyPersistence   Key Persistence for this key
 * @param[in]   aKey              Actual key to be imported.
 * @param[in]   aKeyLen           Length of the key to be imported.
 *
 * @retval OT_ERROR_NONE          Successfully encrypted  @p aInput.
 * @retval OT_ERROR_FAILED        Failed to encrypt @p aInput.
 * @retval OT_ERROR_INVALID_ARGS  @p aInput or @p aOutput was set to NULL.
 *
 */
otError otPlatPsaImportKey(psa_key_id_t             *aKeyId,
                           psa_key_type_t           aKeyType,
                           psa_algorithm_t          aKeyAlgorithm,
                           psa_key_usage_t          aKeyUsage,
                           psa_key_persistence_t    aKeyPersistence,
                           const uint8_t            *aKey,
                           size_t                   aKeyLen);

/**
 * Export a key stored in PSA ITS.
 *
 * @param[in]   aKeyId            Reference to the key to be used for crypto operations.
 * @param[out]  aBuffer           Pointer to the buffer where key needs to be exported.
 * @param[in]   aBufferLen        Length of the buffer passed to store the exported key.
 * @param[out]  aKeyLen           Length of the exported key.
 *
 * @retval OT_ERROR_NONE          Successfully exported  @p aKeyId.
 * @retval OT_ERROR_FAILED        Failed to export @p aInput.
 * @retval OT_ERROR_INVALID_ARGS  @p aBuffer was NULL
 *
 */
otError otPlatPsaExportKey(psa_key_id_t aKeyId,
                           uint8_t      *aBuffer,
                           uint8_t      aBufferLen,
                           size_t       *aKeyLen);

/**
 * Destroy a key stored in PSA ITS.
 *
 * @param[in]   aKeyId            Reference to the key to be used for crypto operations.
 *
 * @retval OT_ERROR_NONE          Successfully encrypted  @p aInput.
 * @retval OT_ERROR_FAILED        Failed to encrypt @p aInput.
 *
 */
otError otPlatPsaDestroyKey(psa_key_id_t aKeyId);

/**
 * Export public key of the keypair stored in ITS.
 *
 * @param[in]   aKeyId            Reference to the key to be exported.
 * @param[out]  aOutput           Pointer to the buffer to store exported public key.
 * @param[in]   aOutputSize       Size of the output buffer passed.
 * @param[out]  aOutputLen        Length of the exported key.
 *
 * @retval OT_ERROR_NONE          Successfully exported  @p aKeyId.
 * @retval OT_ERROR_FAILED        Failed to export @p aKeyId.
 * @retval OT_ERROR_INVALID_ARGS  @p aOutput or @p aOutputLen was NULL
 *
 */
otError otPlatPsaExportPublicKey(psa_key_id_t   aKeyId,
                                 uint8_t        *aOutput,
                                 size_t         aOutputSize,
                                 size_t         *aOutputLen);

/**
 * Sign a message hash using a key stored.
 *
 * @param[in]   aKeyId            Reference to the key to be used for Signing.
 * @param[in]   aKeyAlgorithm     Key algorithm compatible with the key stored.
 * @param[in]   aHash             Hash of the message to be signed.
 * @param[in]   aHashSize         Size of the hash.
 * @param[out]  aSignature        Buffer to store the calculated signature.
 * @param[in]   aSignatureSize    Size of the buffer passed.
 * @param[in]   aSignatureLen     Length of the signature calculated.
 *
 * @retval OT_ERROR_NONE          Successfully signed  @p aHash.
 * @retval OT_ERROR_FAILED        Failed to sign @p aHash.
 *
 */
otError otPlatPsaSignHash(psa_key_id_t    aKeyId,
                          psa_algorithm_t aKeyAlgorithm,
                          uint8_t         *aHash,
                          size_t          aHashSize,
                          uint8_t         *aSignature,
                          size_t          aSignatureSize,
                          size_t          *aSignatureLen);

/**
 * Verify a signature using a key stored.
 *
 * @param[in]   aKeyId            Reference to the key to be used for Verifying the signature.
 * @param[in]   aKeyAlgorithm     Key algorithm compatible with the key stored.
 * @param[in]   aHash             Hash of the message to be verified.
 * @param[in]   aHashSize         Size of the hash.
 * @param[out]  aSignature        Signature that needs verification.
 * @param[in]   aSignatureSize    Size of the signature passed.
 *
 * @retval OT_ERROR_NONE          Successfully signed  @p aHash.
 * @retval OT_ERROR_FAILED        Failed to sign @p aHash.
 *
 */
otError otPlatPsaVerifyHash(psa_key_id_t    aKeyId,
                            psa_algorithm_t aKeyAlgorithm,
                            uint8_t         *aHash,
                            size_t          aHashSize,
                            uint8_t         *aSignature,
                            size_t          aSignatureSize);


/**
 * Get Attributes for a key stored in PSA ITS.
 *
 * @param[in]  aKeyId            Reference to the key to be used for Verifying the signature.
 * @param[out] aKeyAttributes    Key attributes for the key.
 *
 * @retval OT_ERROR_NONE          Successfully signed  @p aHash.
 * @retval OT_ERROR_FAILED        Failed to sign @p aHash.
 *
 */
otError otPlatPsaGetKeyAttributes(psa_key_id_t          aKeyId,
                                  psa_key_attributes_t  *aKeyAttributes);


/**
 * @}
 *
 */

#ifdef __cplusplus
} // end of extern "C"
#endif

#endif // OPENTHREAD_CONFIG_PSA_CRYPTO_ENABLE
#endif // OPENTHREAD_PLATFORM_PSA_H_
