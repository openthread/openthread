/*
 *  Copyright (c) 2022, The OpenThread Authors.
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
 *   This file implements ECDSA signing using TinyCrypt library.
 */

#include "ecdsa.hpp"

#if OPENTHREAD_CONFIG_ECDSA_ENABLE

#ifdef MBEDTLS_USE_TINYCRYPT

#include <string.h>

#include <mbedtls/pk.h>
#include <mbedtls/version.h>

#include <tinycrypt/ecc.h>
#include <tinycrypt/ecc_dh.h>
#include <tinycrypt/ecc_dsa.h>

#include "common/code_utils.hpp"
#include "common/debug.hpp"
#include "common/random.hpp"
#include "crypto/mbedtls.hpp"

namespace ot {
namespace Crypto {
namespace Ecdsa {

Error P256::KeyPair::Generate(void)
{
    mbedtls_pk_context    pk;
    mbedtls_uecc_keypair *keypair;
    int                   ret;

    mbedtls_pk_init(&pk);

    ret = mbedtls_pk_setup(&pk, mbedtls_pk_info_from_type(MBEDTLS_PK_ECKEY));
    VerifyOrExit(ret == 0);

    keypair = mbedtls_pk_uecc(pk);

    ret = uECC_make_key(keypair->public_key, keypair->private_key);
    VerifyOrExit(ret == UECC_SUCCESS);

    ret = mbedtls_pk_write_key_der(&pk, mDerBytes, sizeof(mDerBytes));
    VerifyOrExit(ret > 0);

    mDerLength = static_cast<uint8_t>(ret);

    memmove(mDerBytes, mDerBytes + sizeof(mDerBytes) - mDerLength, mDerLength);

exit:
    mbedtls_pk_free(&pk);

    return (ret >= 0) ? kErrorNone : MbedTls::MapError(ret);
}

Error P256::KeyPair::Parse(void *aContext) const
{
    Error               error = kErrorNone;
    mbedtls_pk_context *pk    = reinterpret_cast<mbedtls_pk_context *>(aContext);

    mbedtls_pk_init(pk);

    VerifyOrExit(mbedtls_pk_setup(pk, mbedtls_pk_info_from_type(MBEDTLS_PK_ECKEY)) == 0, error = kErrorFailed);
#if (MBEDTLS_VERSION_NUMBER >= 0x03000000)
    VerifyOrExit(mbedtls_pk_parse_key(pk, mDerBytes, mDerLength, nullptr, 0, MbedTls::CryptoSecurePrng, nullptr) == 0,
                 error = kErrorParse);
#else
    VerifyOrExit(mbedtls_pk_parse_key(pk, mDerBytes, mDerLength, nullptr, 0) == 0, error = kErrorParse);
#endif

exit:
    return error;
}

Error P256::KeyPair::GetPublicKey(PublicKey &aPublicKey) const
{
    Error                 error;
    mbedtls_pk_context    pk;
    mbedtls_uecc_keypair *keyPair;
    int                   ret;

    SuccessOrExit(error = Parse(&pk));

    keyPair = mbedtls_pk_uecc(pk);

    memcpy(aPublicKey.mData, keyPair->public_key, kMpiSize);
    memcpy(aPublicKey.mData + kMpiSize, keyPair->public_key + kMpiSize, kMpiSize);

exit:
    mbedtls_pk_free(&pk);

    return error;
}

Error P256::KeyPair::Sign(const Sha256::Hash &aHash, Signature &aSignature) const
{
    Error                 error;
    mbedtls_pk_context    pk;
    mbedtls_uecc_keypair *keypair;
    int                   ret;
    uint8_t               sig[2 * kMpiSize];

    SuccessOrExit(error = Parse(&pk));

    keypair = mbedtls_pk_uecc(pk);

    ret = uECC_sign(keypair->private_key, aHash.GetBytes(), Sha256::Hash::kSize, sig);
    VerifyOrExit(ret == UECC_SUCCESS, error = MbedTls::MapError(ret));

    memcpy(aSignature.mShared.mMpis.mR, sig, kMpiSize);
    memcpy(aSignature.mShared.mMpis.mS, sig + kMpiSize, kMpiSize);

exit:
    mbedtls_pk_free(&pk);

    return error;
}

Error P256::PublicKey::Verify(const Sha256::Hash &aHash, const Signature &aSignature) const
{
    Error   error = kErrorNone;
    int     ret;
    uint8_t public_key[2 * kMpiSize];
    uint8_t sig[2 * kMpiSize];

    memcpy(public_key, GetBytes(), 2 * kMpiSize);

    memcpy(sig, aSignature.mShared.mMpis.mR, kMpiSize);
    memcpy(sig + kMpiSize, aSignature.mShared.mMpis.mS, kMpiSize);

    ret = uECC_verify(public_key, aHash.GetBytes(), Sha256::Hash::kSize, sig);
    VerifyOrExit(ret == UECC_SUCCESS, error = kErrorSecurity);

exit:
    return error;
}

Error Sign(uint8_t *      aOutput,
           uint16_t &     aOutputLength,
           const uint8_t *aInputHash,
           uint16_t       aInputHashLength,
           const uint8_t *aPrivateKey,
           uint16_t       aPrivateKeyLength)
{
    Error                 error = kErrorNone;
    mbedtls_pk_context    pkCtx;
    mbedtls_uecc_keypair *keypair;
    uint8_t               sig[2 * NUM_ECC_BYTES];

    mbedtls_pk_init(&pkCtx);

    // Parse a private key in PEM format.
    VerifyOrExit(mbedtls_pk_parse_key(&pkCtx, aPrivateKey, aPrivateKeyLength, nullptr, 0) == 0,
                 error = kErrorInvalidArgs);
    VerifyOrExit(mbedtls_pk_get_type(&pkCtx) == MBEDTLS_PK_ECKEY, error = kErrorInvalidArgs);

    keypair = mbedtls_pk_uecc(pkCtx);
    OT_ASSERT(keypair != nullptr);

    // Sign using ECDSA.
    VerifyOrExit(uECC_sign(keypair->private_key, aInputHash, aInputHashLength, sig) == UECC_SUCCESS,
                 error = kErrorFailed);
    VerifyOrExit(2 * NUM_ECC_BYTES <= aOutputLength, error = kErrorNoBufs);

    // Concatenate the two octet sequences in the order R and then S.
    memcpy(aOutput, sig, 2 * NUM_ECC_BYTES);
    aOutputLength = 2 * NUM_ECC_BYTES;

exit:
    mbedtls_pk_free(&pkCtx);

    return error;
}

} // namespace Ecdsa
} // namespace Crypto
} // namespace ot

#endif // MBEDTLS_USE_TINYCRYPT
#endif // OPENTHREAD_CONFIG_ECDSA_ENABLE
