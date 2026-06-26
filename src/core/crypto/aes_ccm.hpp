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
 *   This file includes definitions for performing AES-CCM computations.
 */

#ifndef OT_CORE_CRYPTO_AES_CCM_HPP_
#define OT_CORE_CRYPTO_AES_CCM_HPP_

#include "openthread-core-config.h"

#include <stdint.h>

#include <openthread/platform/crypto.h>
#include "common/error.hpp"
#include "common/message.hpp"
#include "common/type_traits.hpp"
#include "crypto/aes_ecb.hpp"
#include "crypto/storage.hpp"
#include "mac/mac_types.hpp"

namespace ot {
namespace Crypto {

/**
 * @addtogroup core-security
 *
 * @{
 */

/**
 * Implements AES CCM computation.
 */
class AesCcm
{
public:
    static constexpr uint8_t kMinTagLength = 4;                  ///< Minimum tag length (in bytes).
    static constexpr uint8_t kMaxTagLength = AesEcb::kBlockSize; ///< Maximum tag length (in bytes).

    /**
     * Initializes the AES-CCM object.
     */
    AesCcm(void);

    /**
     * Represents the operation to perform (encryption or decryption)
     */
    enum Operation : uint8_t
    {
        kEncrypt, ///< Encrypt.
        kDecrypt, ///< Decrypt.
    };

    /**
     * Represents an IEEE 802.15.4 nonce byte sequence.
     */
    OT_TOOL_PACKED_BEGIN
    class Nonce : public Clearable<Nonce>
    {
    public:
        /**
         * Initializes the nonce from a given extended address, frame counter, and security level.
         *
         * @param[in]  aExtAddress     An extended address.
         * @param[in]  aFrameCounter   A frame counter.
         * @param[in]  aSecurityLevel  A security level.
         */
        void InitFrom(const Mac::ExtAddress &aExtAddress, uint32_t aFrameCounter, uint8_t aSecurityLevel);

    private:
        Mac::ExtAddress mExtAddress;
        uint32_t        mFrameCounter;
        uint8_t         mSecurityLevel;
    } OT_TOOL_PACKED_END;

    static_assert(sizeof(Nonce) == 13, "Nonce format is not valid");

    /**
     * Sets the key.
     *
     * The passed-in @p aKey and its underlying buffer must remain valid during the lifecycle of the `AesCcm` object.
     *
     * @param[in]  aKey    Crypto Key used in AES operation.
     */
    void SetKey(const Key &aKey) { mConfig.mKey = aKey; }

    /**
     * Sets the key.
     *
     * The passed-in @p aKey buffer must remain valid during the lifecycle of the `AesCcm` object.
     *
     * @param[in]  aKey        A pointer to the key.
     * @param[in]  aKeyLength  Length of the key in bytes.
     */
    void SetKey(const uint8_t *aKey, uint16_t aKeyLength) { mConfig.GetKey().Set(aKey, aKeyLength); }

    /**
     * Sets the key.
     *
     * The passed-in @p aMacKey and its underlying buffer must remain valid during the lifecycle of the `AesCcm` object.
     *
     * @param[in]  aMacKey        Key Material for AES operation.
     */
    void SetKey(const Mac::KeyMaterial &aMacKey) { aMacKey.ConvertToCryptoKey(mConfig.GetKey()); }

    /**
     * Sets the Nonce.
     *
     * The passed-in @p aNonce must remain valid during the lifecycle of the `AesCcm` object.
     *
     * @param[in]  aNonce  A reference to the Nonce object.
     */
    void SetNonce(const Nonce &aNonce) { SetNonce(&aNonce, sizeof(Nonce)); }

    /**
     * Sets the Nonce.
     *
     * The passed-in @p aNonce buffer must remain valid during the lifecycle of the `AesCcm` object.
     *
     * @param[in]  aNonce   A pointer to the buffer containing the nonce.
     * @param[in]  aLength  The length of the nonce in bytes.
     */
    void SetNonce(const void *aNonce, uint8_t aLength);

    /**
     * Sets the Additional Authenticated Data.
     *
     * The passed-in @p aAuthData buffer must remain valid during the lifecycle of the `AesCcm` object.
     *
     * @param[in]  aAuthData  A pointer to the buffer containing the data.
     * @param[in]  aLength    The length of data in bytes.
     */
    void SetAuthData(const void *aAuthData, uint32_t aLength);

    /**
     * Sets the AES-CCM tag (MIC) length.
     *
     * @param[in]  aTagLength  The tag length in bytes (must be even and in [kMinTagLength, kMaxTagLength]).
     */
    void SetTagLength(uint8_t aTagLength);

    /**
     * Performs in-place AES-CCM computation (encryption or decryption) on a contiguous buffer.
     *
     * Before calling this method, the `AesCcm` object must be fully configured by calling `SetKey()`, `SetNonce()`,
     * `SetAuthData()`, and `SetTagLength()`. Otherwise, the behavior of this method is undefined.
     *
     * The buffer @p aData must have sufficient space. For encryption, it must be large enough to hold the plaintext
     * plus the tag. The tag will be appended immediately after the payload bytes. For decryption, the tag must be
     * present immediately after the ciphertext bytes.
     *
     * @param[in]      aOperation   The operation (kEncrypt or kDecrypt).
     * @param[in,out]  aData        A pointer to the data buffer.
     * @param[in]      aLength      The length of the payload in bytes (excluding the tag).
     *
     * @retval kErrorNone      Operation succeeded (for decryption, this means the tag matched).
     * @retval kErrorSecurity  Decryption failed because the tag did not match.
     */
    Error Process(Operation aOperation, uint8_t *aData, uint32_t aLength);

#if OPENTHREAD_FTD || OPENTHREAD_MTD
    /**
     * Performs in-place AES-CCM computation (encryption or decryption) on a `Message`
     *
     * Before calling this method, the `AesCcm` object must be fully configured by calling `SetKey()`, `SetNonce()`,
     * `SetAuthData()`, and `SetTagLength()`. Otherwise, the behavior of this method is undefined.
     *
     * The operation starts at @p aOffset in the message.
     *
     * For encryption, the payload is from @p aOffset to the end of the message. The calculated tag is appended to the
     * end of the message.
     *
     * For decryption, the tag is assumed to be the last bytes of the message. The payload is from @p aOffset up to the
     * tag (tag length bytes before the end). If decryption succeeds (tag matches), the tag is removed from the
     * message.
     *
     * @param[in]      aOperation   The operation (kEncrypt or kDecrypt).
     * @param[in,out]  aMessage     The Message.
     * @param[in]      aOffset      The offset in the message where the payload starts.
     *
     * @retval kErrorNone         Operation succeeded.
     * @retval kErrorSecurity     Decryption failed because the tag did not match.
     * @retval kErrorNoBufs       Failed to grow the message to append the tag (during encryption).
     * @retval kErrorInvalidArgs  The @p aOffset is invalid (larger than message length).
     */
    Error Process(Operation aOperation, Message &aMessage, uint16_t aOffset);
#endif

    /**
     * Performs a complete AES-CCM computation (encryption or decryption).
     *
     * @deprecated This method is provided to support the public `otCrypto` API and should not be used
     *             within the OpenThread core. Core modules should instantiate an `AesCcm` object and use
     *             the `Process()` methods instead.
     *
     * @param[in]      aOperation       The operation (kEncrypt or kDecrypt).
     * @param[in]      aKey             The crypto key to use.
     * @param[in]      aTagLength       The tag length in bytes.
     * @param[in]      aNonce           A pointer to the nonce.
     * @param[in]      aNonceLength     The length of the nonce in bytes.
     * @param[in]      aAuthData        A pointer to the additional authentication data.
     * @param[in]      aAuthDataLength  The length of the authentication data in bytes.
     * @param[in,out]  aPlainText       A pointer to the plaintext buffer.
     * @param[in,out]  aCipherText      A pointer to the ciphertext buffer.
     * @param[in]      aLength          The length of the payload (plaintext/ciphertext) in bytes.
     * @param[out]     aTag             A pointer to a buffer to output the calculated tag.
     */
    static void Perform(Operation   aOperation,
                        const Key  &aKey,
                        uint8_t     aTagLength,
                        const void *aNonce,
                        uint8_t     aNonceLength,
                        const void *aAuthData,
                        uint32_t    aAuthDataLength,
                        void       *aPlainText,
                        void       *aCipherText,
                        uint32_t    aLength,
                        void       *aTag);

private:
    struct Config : public otPlatCryptoAesCcmConfig, public Clearable<Config>
    {
        bool       IsValid(void) const;
        Key       &GetKey(void) { return AsCoreType(&mKey); }
        const Key &GetKey(void) const { return AsCoreType(&mKey); }
    };

    class Engine
    {
    public:
        Error ProcessOneShot(Operation aOperation, const Config &aConfig, const uint8_t *aHeader, uint8_t *aData);

        // Multi-part
        void Start(const Config &aConfig);
        void AddHeader(const void *aHeader, uint32_t aHeaderLength);
        void AddPayload(void *aPlainText, void *aCipherText, uint32_t aLength, Operation aOperation);
        void Finalize(void *aTag);

    private:
        AesEcb   mEcb;
        uint8_t  mBlock[AesEcb::kBlockSize];
        uint8_t  mCtr[AesEcb::kBlockSize];
        uint8_t  mCtrPad[AesEcb::kBlockSize];
        uint32_t mHeaderLength;
        uint32_t mHeaderCur;
        uint32_t mPlainTextLength;
        uint32_t mPlainTextCur;
        uint16_t mBlockLength;
        uint16_t mCtrLength;
        uint8_t  mNonceLength;
        uint8_t  mTagLength;
    };

    Config         mConfig;
    const uint8_t *mAuthData;
};

/**
 * @}
 */

} // namespace Crypto
} // namespace ot

#endif // OT_CORE_CRYPTO_AES_CCM_HPP_
