/*
 *  Copyright (c) 2018, The OpenThread Authors.
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
 *   This file includes definitions for performing ECDSA signing.
 */

#ifndef ECDSA_HPP_
#define ECDSA_HPP_

#include "openthread-core-config.h"

#if OPENTHREAD_CONFIG_ECDSA_ENABLE

#include <stdint.h>
#include <stdlib.h>

#include <openthread/crypto.h>
#include <openthread/platform/crypto.h>

#include "common/error.hpp"
#include "crypto/sha256.hpp"
#include "crypto/storage.hpp"

namespace ot {
namespace Crypto {
namespace Ecdsa {

/**
 * @addtogroup core-security
 *
 * @{
 *
 */

/**
 * Implements ECDSA key-generation, signing, and verification for NIST P-256 curve using SHA-256 hash.
 *
 * NIST P-256 curve is also known as 256-bit Random ECP Group (RFC 5114 - 2.6), or secp256r1 (RFC 4492 - Appendix A).
 *
 */
class P256
{
public:
    static constexpr uint16_t kFieldBitLength = 256; ///< Prime field bit length used by the P-256 curve.

    /**
     * Max bytes in binary representation of an MPI (multi-precision int).
     *
     */
    static constexpr uint8_t kMpiSize = kFieldBitLength / 8;

    class PublicKey;
    class KeyPair;
#if OPENTHREAD_CONFIG_PLATFORM_KEY_REFERENCES_ENABLE
    class KeyPairAsRef;
#endif

    /**
     * Represents an ECDSA signature.
     *
     * The signature is encoded as the concatenated binary representation of two MPIs `r` and `s` which are calculated
     * during signing (RFC 6605 - section 4).
     *
     */
    OT_TOOL_PACKED_BEGIN
    class Signature : public otPlatCryptoEcdsaSignature
    {
        friend class KeyPair;
        friend class PublicKey;
#if OPENTHREAD_CONFIG_PLATFORM_KEY_REFERENCES_ENABLE
        friend class KeyPairAsRef;
#endif

    public:
        static constexpr uint8_t kSize = OT_CRYPTO_ECDSA_SIGNATURE_SIZE; ///< Signature size in bytes.

        /**
         * Returns the signature as a byte array.
         *
         * @returns A pointer to the byte array containing the signature.
         *
         */
        const uint8_t *GetBytes(void) const { return m8; }
    } OT_TOOL_PACKED_END;

    /**
     * Represents a key pair (public and private keys).
     *
     * The key pair is stored using Distinguished Encoding Rules (DER) format (per RFC 5915).
     *
     */
    class KeyPair : public otPlatCryptoEcdsaKeyPair
    {
    public:
        /**
         * Max buffer size (in bytes) for representing the key-pair in DER format.
         *
         */
        static constexpr uint8_t kMaxDerSize = OT_CRYPTO_ECDSA_MAX_DER_SIZE;

        /**
         * Initializes a `KeyPair` as empty (no key).
         *
         */
        KeyPair(void) { mDerLength = 0; }

        /**
         * Generates and populates the `KeyPair` with a new public/private keys.
         *
         * @retval kErrorNone         A new key pair was generated successfully.
         * @retval kErrorNoBufs       Failed to allocate buffer for key generation.
         * @retval kErrorNotCapable   Feature not supported.
         * @retval kErrorFailed       Failed to generate key.
         *
         */
        Error Generate(void) { return otPlatCryptoEcdsaGenerateKey(this); }

        /**
         * Gets the associated public key from the `KeyPair`.
         *
         * @param[out] aPublicKey     A reference to a `PublicKey` to output the value.
         *
         * @retval kErrorNone      Public key was retrieved successfully, and @p aPublicKey is updated.
         * @retval kErrorParse     The key-pair DER format could not be parsed (invalid format).
         *
         */
        Error GetPublicKey(PublicKey &aPublicKey) const { return otPlatCryptoEcdsaGetPublicKey(this, &aPublicKey); }

        /**
         * Gets the pointer to start of the buffer containing the key-pair info in DER format.
         *
         * The length (number of bytes) of DER format is given by `GetDerLength()`.
         *
         * @returns The pointer to the start of buffer containing the key-pair in DER format.
         *
         */
        const uint8_t *GetDerBytes(void) const { return mDerBytes; }

        /**
         * Gets the length of the byte sequence representation of the key-pair in DER format.
         *
         * @returns The length of byte sequence representation of the key-pair in DER format.
         *
         */
        uint8_t GetDerLength(void) const { return mDerLength; }

        /**
         * Gets the pointer to start of the key-pair buffer in DER format.
         *
         * Gives non-const pointer to the buffer and is intended for populating the buffer and setting
         * the key-pair (e.g., reading the key-pair from non-volatile settings). The buffer contains `kMaxDerSize`
         * bytes. After populating the buffer, `SetDerLength()` can be used to set the the number of bytes written.
         *
         * @returns The pointer to the start of key-pair buffer in DER format.
         *
         */
        uint8_t *GetDerBytes(void) { return mDerBytes; }

        /**
         * Sets the length of the byte sequence representation of the key-pair in DER format.
         *
         * @param[in] aDerLength   The length (number of bytes).
         *
         */
        void SetDerLength(uint8_t aDerLength) { mDerLength = aDerLength; }

        /**
         * Calculates the ECDSA signature for a hashed message using the private key from `KeyPair`.
         *
         * Uses the deterministic digital signature generation procedure from RFC 6979.
         *
         * @param[in]  aHash               The SHA-256 hash value of the message to use for signature calculation.
         * @param[out] aSignature          A reference to a `Signature` to output the calculated signature value.
         *
         * @retval kErrorNone           The signature was calculated successfully and @p aSignature was updated.
         * @retval kErrorParse          The key-pair DER format could not be parsed (invalid format).
         * @retval kErrorInvalidArgs    The @p aHash is invalid.
         * @retval kErrorNoBufs         Failed to allocate buffer for signature calculation.
         *
         */
        Error Sign(const Sha256::Hash &aHash, Signature &aSignature) const
        {
            return otPlatCryptoEcdsaSign(this, &aHash, &aSignature);
        }
    };

#if OPENTHREAD_CONFIG_PLATFORM_KEY_REFERENCES_ENABLE
    /**
     * Represents a key pair (public and private keys) as a PSA KeyRef.
     *
     */
    class KeyPairAsRef
    {
    public:
        /**
         * Initializes a `KeyPairAsRef`.
         *
         * @param[in] aKeyRef         PSA key reference to use while using the keypair.
         */
        explicit KeyPairAsRef(otCryptoKeyRef aKeyRef = 0) { mKeyRef = aKeyRef; }

        /**
         * Generates a new keypair and imports it into PSA ITS.
         *
         * @retval kErrorNone         A new key pair was generated successfully.
         * @retval kErrorNoBufs       Failed to allocate buffer for key generation.
         * @retval kErrorNotCapable   Feature not supported.
         * @retval kErrorFailed       Failed to generate key.
         *
         */
        Error Generate(void) const { return otPlatCryptoEcdsaGenerateAndImportKey(mKeyRef); }

        /**
         * Imports a new keypair into PSA ITS.
         *
         * @param[in] aKeyPair        KeyPair to be imported in DER format.
         *
         * @retval kErrorNone         A key pair was imported successfully.
         * @retval kErrorNotCapable   Feature not supported.
         * @retval kErrorFailed       Failed to import the key.
         *
         */
        Error ImportKeyPair(const KeyPair &aKeyPair)
        {
            return Crypto::Storage::ImportKey(mKeyRef, Storage::kKeyTypeEcdsa, Storage::kKeyAlgorithmEcdsa,
                                              (Storage::kUsageSignHash | Storage::kUsageVerifyHash),
                                              Storage::kTypePersistent, aKeyPair.GetDerBytes(),
                                              aKeyPair.GetDerLength());
        }

        /**
         * Gets the associated public key from the keypair referenced by mKeyRef.
         *
         * @param[out] aPublicKey     A reference to a `PublicKey` to output the value.
         *
         * @retval kErrorNone      Public key was retrieved successfully, and @p aPublicKey is updated.
         * @retval kErrorFailed    There was a error exporting the public key from PSA.
         *
         */
        Error GetPublicKey(PublicKey &aPublicKey) const
        {
            return otPlatCryptoEcdsaExportPublicKey(mKeyRef, &aPublicKey);
        }

        /**
         * Calculates the ECDSA signature for a hashed message using the private key from keypair
         * referenced by mKeyRef.
         *
         * Uses the deterministic digital signature generation procedure from RFC 6979.
         *
         * @param[in]  aHash               The SHA-256 hash value of the message to use for signature calculation.
         * @param[out] aSignature          A reference to a `Signature` to output the calculated signature value.
         *
         * @retval kErrorNone           The signature was calculated successfully and @p aSignature was updated.
         * @retval kErrorParse          The key-pair DER format could not be parsed (invalid format).
         * @retval kErrorInvalidArgs    The @p aHash is invalid.
         * @retval kErrorNoBufs         Failed to allocate buffer for signature calculation.
         *
         */
        Error Sign(const Sha256::Hash &aHash, Signature &aSignature) const
        {
            return otPlatCryptoEcdsaSignUsingKeyRef(mKeyRef, &aHash, &aSignature);
        }

        /**
         * Gets the Key reference for the keypair stored in the PSA.
         *
         * @returns The PSA key ref.
         *
         */
        otCryptoKeyRef GetKeyRef(void) const { return mKeyRef; }

        /**
         * Sets the Key reference.
         *
         * @param[in] aKeyRef         PSA key reference to use while using the keypair.
         *
         */
        void SetKeyRef(otCryptoKeyRef aKeyRef) { mKeyRef = aKeyRef; }

    private:
        otCryptoKeyRef mKeyRef;
    };
#endif

    /**
     * Represents a public key.
     *
     * The public key is stored as a byte sequence representation of an uncompressed curve point (RFC 6605 - sec 4).
     *
     */
    OT_TOOL_PACKED_BEGIN
    class PublicKey : public otPlatCryptoEcdsaPublicKey, public Equatable<PublicKey>
    {
        friend class KeyPair;
#if OPENTHREAD_CONFIG_PLATFORM_KEY_REFERENCES_ENABLE
        friend class KeyPairAsRef;
#endif

    public:
        static constexpr uint8_t kSize = OT_CRYPTO_ECDSA_PUBLIC_KEY_SIZE; ///< Size of the public key in bytes.

        /**
         * Gets the pointer to the buffer containing the public key (as an uncompressed curve point).
         *
         * @return The pointer to the buffer containing the public key (with `kSize` bytes).
         *
         */
        const uint8_t *GetBytes(void) const { return m8; }

        /**
         * Uses the `PublicKey` to verify the ECDSA signature of a hashed message.
         *
         * @param[in] aHash                The SHA-256 hash value of a message to use for signature verification.
         * @param[in] aSignature           The signature value to verify.
         *
         * @retval kErrorNone          The signature was verified successfully.
         * @retval kErrorSecurity      The signature is invalid.
         * @retval kErrorInvalidArgs   The key or has is invalid.
         * @retval kErrorNoBufs        Failed to allocate buffer for signature verification
         *
         */
        Error Verify(const Sha256::Hash &aHash, const Signature &aSignature) const
        {
            return otPlatCryptoEcdsaVerify(this, &aHash, &aSignature);
        }

    } OT_TOOL_PACKED_END;
};

/**
 * @}
 *
 */

} // namespace Ecdsa
} // namespace Crypto

DefineCoreType(otPlatCryptoEcdsaSignature, Crypto::Ecdsa::P256::Signature);
DefineCoreType(otPlatCryptoEcdsaKeyPair, Crypto::Ecdsa::P256::KeyPair);
DefineCoreType(otPlatCryptoEcdsaPublicKey, Crypto::Ecdsa::P256::PublicKey);

} // namespace ot

#endif // OPENTHREAD_CONFIG_ECDSA_ENABLE

#endif // ECDSA_HPP_
