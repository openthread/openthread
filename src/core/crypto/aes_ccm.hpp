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

#ifndef AES_CCM_HPP_
#define AES_CCM_HPP_

#include "openthread-core-config.h"

#include "utils/wrap_stdint.h"

#include <openthread/error.h>

#include "crypto/aes_ecb.hpp"

namespace ot {
namespace Crypto {

/**
 * @addtogroup core-security
 *
 * @{
 *
 */

/**
 * This class implements AES CCM computation.
 *
 */
class AesCcm
{
public:
    /**
     * This method sets the key.
     *
     * @param[in]  aKey        A pointer to the key.
     * @param[in]  aKeyLength  Length of the key in bytes.
     *
     */
    otError SetKey(const uint8_t *aKey, uint16_t aKeyLength);

    /**
     * This method initializes the AES CCM computation.
     *
     * @param[in]  aHeaderLength     Length of header in bytes.
     * @param[in]  aPlainTextLength  Length of plaintext in bytes.
     * @param[in]  aTagLength        Length of tag in bytes.
     * @param[in]  aNonce            A pointer to the nonce.
     * @param[in]  aNonceLength      Length of nonce in bytes.
     *
     * @retval OT_ERROR_NONE          Initialization was successful.
     * @retval OT_ERROR_INVALID_ARGS  Initialization failed.
     *
     */
    otError Init(uint32_t    aHeaderLength,
                 uint32_t    aPlainTextLength,
                 uint8_t     aTagLength,
                 const void *aNonce,
                 uint8_t     aNonceLength);

    /**
     * This method processes the header.
     *
     * @param[in]  aHeader        A pointer to the header.
     * @param[in]  aHeaderLength  Length of header in bytes.
     *
     */
    void Header(const void *aHeader, uint32_t aHeaderLength);

    /**
     * This method processes the payload.
     *
     * @param[inout]  aPlainText   A pointer to the plaintext.
     * @param[inout]  aCipherText  A pointer to the ciphertext.
     * @param[in]     aLength      Payload length in bytes.
     * @param[in]     aEncrypt     TRUE on encrypt and FALSE on decrypt.
     *
     */
    void Payload(void *aPlainText, void *aCipherText, uint32_t aLength, bool aEncrypt);

    /**
     * This method generates the tag.
     *
     * @param[out]  aTag        A pointer to the tag.
     * @param[out]  aTagLength  Length of the tag in bytes.
     *
     */
    void Finalize(void *aTag, uint8_t *aTagLength);

private:
    enum
    {
        kTagLengthMin = 4,
    };

    AesEcb   mEcb;
    uint8_t  mBlock[AesEcb::kBlockSize];
    uint8_t  mCtr[AesEcb::kBlockSize];
    uint8_t  mCtrPad[AesEcb::kBlockSize];
    uint8_t  mNonceLength;
    uint32_t mHeaderLength;
    uint32_t mHeaderCur;
    uint32_t mPlainTextLength;
    uint32_t mPlainTextCur;
    uint16_t mBlockLength;
    uint16_t mCtrLength;
    uint8_t  mTagLength;
};

/**
 * @}
 *
 */

} // namespace Crypto
} // namespace ot

#endif // AES_CCM_HPP_
