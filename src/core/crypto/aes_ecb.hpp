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
 *   This file includes definitions for performing AES-ECB computations.
 */

#ifndef OT_CORE_CRYPTO_AES_ECB_HPP_
#define OT_CORE_CRYPTO_AES_ECB_HPP_

#include "openthread-core-config.h"

#include <openthread/platform/crypto.h>

#include "common/code_utils.hpp"
#include "common/error.hpp"
#include "crypto/context_size.hpp"
#include "crypto/storage.hpp"

namespace ot {
namespace Crypto {

/**
 * @addtogroup core-security
 *
 * @{
 */

/**
 * Implements AES ECB computation.
 */
class AesEcb
{
public:
    static constexpr uint8_t kBlockSize = 16; ///< AES-128 block size (bytes).

    /**
     * Constructor to initialize the AES operation.
     */
    AesEcb(void);

    /**
     * Destructor to free the AES context.
     */
    ~AesEcb(void);

    /**
     * Sets the key.
     *
     * @param[in]  aKey     Crypto Key used for ECB operation
     */
    void SetKey(const Key &aKey);

    /**
     * Encrypts data.
     *
     * @param[in]   aInput   A pointer to the input buffer.
     * @param[out]  aOutput  A pointer to the output buffer.
     */
    void Encrypt(const uint8_t aInput[kBlockSize], uint8_t aOutput[kBlockSize]);

#if OPENTHREAD_CONFIG_CRYPTO_PLATFORM_CCM_ENABLE
    /**
     * Decrypts and verifies the given AES-CCM* payload.
     *
     * Calls `otPlatCryptoAesDecryptAndVerify()` using the AES context set by `SetKey()`.
     *
     * @param[in]      aNonce          Nonce (13 bytes, IEEE 802.15.4 CCM* format).
     * @param[in]      aHeader         Additional authenticated data.
     * @param[in]      aHeaderLength   Length of @p aHeader in bytes.
     * @param[in,out]  aPayload        Ciphertext on input; replaced with plaintext in place on success.
     * @param[in]      aPayloadLength  Length of @p aPayload in bytes.
     * @param[in]      aTag            MIC buffer of length @p aTagLength bytes.
     * @param[in]      aTagLength      MIC length in bytes (4, 8, or 16).
     *
     * @retval kErrorNone      Successfully decrypted and verified @p aPayload.
     * @retval kErrorSecurity  MIC verification failed.
     * @retval kErrorFailed    Platform operation failed.
     */
    Error DecryptAndVerify(const uint8_t *aNonce,
                           const void    *aHeader,
                           uint16_t       aHeaderLength,
                           void          *aPayload,
                           uint16_t       aPayloadLength,
                           const void    *aTag,
                           uint8_t        aTagLength);

    /**
     * Encrypts and tags the given AES-CCM* payload.
     *
     * Calls `otPlatCryptoAesEncryptAndTag()` using the AES context set by `SetKey()`.
     *
     * @param[in]      aNonce          Nonce (13 bytes, IEEE 802.15.4 CCM* format).
     * @param[in]      aHeader         Additional authenticated data.
     * @param[in]      aHeaderLength   Length of @p aHeader in bytes.
     * @param[in,out]  aPayload        Plaintext on input; replaced with ciphertext in place on success.
     * @param[in]      aPayloadLength  Length of @p aPayload in bytes.
     * @param[out]     aTag            Buffer to receive the MIC; must be at least @p aTagLength bytes.
     * @param[in]      aTagLength      MIC length in bytes (4, 8, or 16).
     *
     * @retval kErrorNone    Successfully encrypted @p aPayload and generated MIC in @p aTag.
     * @retval kErrorFailed  Platform operation failed.
     */
    Error EncryptAndTag(const uint8_t *aNonce,
                        const void    *aHeader,
                        uint16_t       aHeaderLength,
                        void          *aPayload,
                        uint16_t       aPayloadLength,
                        void          *aTag,
                        uint8_t        aTagLength);
#endif

private:
    ContextWith<kAesContextSize> mContext;
};

/**
 * @}
 */

} // namespace Crypto
} // namespace ot

#endif // OT_CORE_CRYPTO_AES_ECB_HPP_
