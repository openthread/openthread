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
 *   This file implements ECDSA signing.
 */

#include "ecdsa.hpp"

#include <string.h>

#include <mbedtls/ctr_drbg.h>
#include <mbedtls/ecdsa.h>
#include <mbedtls/pk.h>

#include "common/code_utils.hpp"
#include "common/debug.hpp"
#include "common/random.hpp"
#include "crypto/mbedtls.hpp"

namespace ot {
namespace Crypto {
namespace Ecdsa {

#if OPENTHREAD_CONFIG_ECDSA_ENABLE

otError P256::KeyPair::Generate(void)
{
    mbedtls_pk_context pk;
    int                ret;

    mbedtls_pk_init(&pk);

    ret = mbedtls_pk_setup(&pk, mbedtls_pk_info_from_type(MBEDTLS_PK_ECKEY));
    VerifyOrExit(ret == 0);

    ret = mbedtls_ecp_gen_key(MBEDTLS_ECP_DP_SECP256R1, mbedtls_pk_ec(pk), mbedtls_ctr_drbg_random,
                              Random::Crypto::MbedTlsContextGet());
    VerifyOrExit(ret == 0);

    ret = mbedtls_pk_write_key_der(&pk, mDerBytes, sizeof(mDerBytes));
    VerifyOrExit(ret > 0);

    mDerLength = static_cast<uint8_t>(ret);

    memmove(mDerBytes, mDerBytes + sizeof(mDerBytes) - mDerLength, mDerLength);

exit:
    mbedtls_pk_free(&pk);

    return (ret >= 0) ? OT_ERROR_NONE : MbedTls::MapError(ret);
}

otError P256::KeyPair::Parse(void *aContext) const
{
    otError             error = OT_ERROR_NONE;
    mbedtls_pk_context *pk    = reinterpret_cast<mbedtls_pk_context *>(aContext);

    mbedtls_pk_init(pk);

    VerifyOrExit(mbedtls_pk_setup(pk, mbedtls_pk_info_from_type(MBEDTLS_PK_ECKEY)) == 0, error = OT_ERROR_FAILED);
    VerifyOrExit(mbedtls_pk_parse_key(pk, mDerBytes, mDerLength, nullptr, 0) == 0, error = OT_ERROR_PARSE);

exit:
    return error;
}

otError P256::KeyPair::GetPublicKey(PublicKey &aPublicKey) const
{
    otError              error;
    mbedtls_pk_context   pk;
    mbedtls_ecp_keypair *keyPair;
    int                  ret;

    SuccessOrExit(error = Parse(&pk));

    keyPair = mbedtls_pk_ec(pk);

    ret = mbedtls_mpi_write_binary(&keyPair->Q.X, aPublicKey.mData, kMpiSize);
    VerifyOrExit(ret == 0, error = MbedTls::MapError(ret));

    ret = mbedtls_mpi_write_binary(&keyPair->Q.Y, aPublicKey.mData + kMpiSize, kMpiSize);
    VerifyOrExit(ret == 0, error = MbedTls::MapError(ret));

exit:
    mbedtls_pk_free(&pk);
    return error;
}

otError P256::KeyPair::Sign(const Sha256::Hash &aHash, Signature &aSignature) const
{
    otError               error;
    mbedtls_pk_context    pk;
    mbedtls_ecp_keypair * keypair;
    mbedtls_ecdsa_context ecdsa;
    mbedtls_mpi           r;
    mbedtls_mpi           s;
    int                   ret;

    mbedtls_ecdsa_init(&ecdsa);
    mbedtls_mpi_init(&r);
    mbedtls_mpi_init(&s);

    SuccessOrExit(error = Parse(&pk));

    keypair = mbedtls_pk_ec(pk);

    ret = mbedtls_ecdsa_from_keypair(&ecdsa, keypair);
    VerifyOrExit(ret == 0, error = MbedTls::MapError(ret));

    ret =
        mbedtls_ecdsa_sign_det(&ecdsa.grp, &r, &s, &ecdsa.d, aHash.GetBytes(), Sha256::Hash::kSize, MBEDTLS_MD_SHA256);
    VerifyOrExit(ret == 0, error = MbedTls::MapError(ret));

    OT_ASSERT(mbedtls_mpi_size(&r) <= kMpiSize);

    ret = mbedtls_mpi_write_binary(&r, aSignature.mShared.mMpis.mR, kMpiSize);
    VerifyOrExit(ret == 0, error = MbedTls::MapError(ret));

    ret = mbedtls_mpi_write_binary(&s, aSignature.mShared.mMpis.mS, kMpiSize);
    VerifyOrExit(ret == 0, error = MbedTls::MapError(ret));

exit:
    mbedtls_pk_free(&pk);
    mbedtls_mpi_free(&s);
    mbedtls_mpi_free(&r);
    mbedtls_ecdsa_free(&ecdsa);

    return error;
}

otError P256::PublicKey::Verify(const Sha256::Hash &aHash, const Signature &aSignature) const
{
    otError               error = OT_ERROR_NONE;
    mbedtls_ecdsa_context ecdsa;
    mbedtls_mpi           r;
    mbedtls_mpi           s;
    int                   ret;

    mbedtls_ecdsa_init(&ecdsa);
    mbedtls_mpi_init(&r);
    mbedtls_mpi_init(&s);

    ret = mbedtls_ecp_group_load(&ecdsa.grp, MBEDTLS_ECP_DP_SECP256R1);
    VerifyOrExit(ret == 0, error = MbedTls::MapError(ret));

    ret = mbedtls_mpi_read_binary(&ecdsa.Q.X, GetBytes(), kMpiSize);
    VerifyOrExit(ret == 0, error = MbedTls::MapError(ret));
    ret = mbedtls_mpi_read_binary(&ecdsa.Q.Y, GetBytes() + kMpiSize, kMpiSize);
    VerifyOrExit(ret == 0, error = MbedTls::MapError(ret));
    ret = mbedtls_mpi_lset(&ecdsa.Q.Z, 1);
    VerifyOrExit(ret == 0, error = MbedTls::MapError(ret));

    ret = mbedtls_mpi_read_binary(&r, aSignature.mShared.mMpis.mR, kMpiSize);
    VerifyOrExit(ret == 0, error = MbedTls::MapError(ret));

    ret = mbedtls_mpi_read_binary(&s, aSignature.mShared.mMpis.mS, kMpiSize);
    VerifyOrExit(ret == 0, error = MbedTls::MapError(ret));

    ret = mbedtls_ecdsa_verify(&ecdsa.grp, aHash.GetBytes(), Sha256::Hash::kSize, &ecdsa.Q, &r, &s);
    VerifyOrExit(ret == 0, error = OT_ERROR_SECURITY);

exit:
    mbedtls_mpi_free(&s);
    mbedtls_mpi_free(&r);
    mbedtls_ecdsa_free(&ecdsa);

    return error;
}

otError Sign(uint8_t *      aOutput,
             uint16_t &     aOutputLength,
             const uint8_t *aInputHash,
             uint16_t       aInputHashLength,
             const uint8_t *aPrivateKey,
             uint16_t       aPrivateKeyLength)
{
    otError               error = OT_ERROR_NONE;
    mbedtls_ecdsa_context ctx;
    mbedtls_pk_context    pkCtx;
    mbedtls_ecp_keypair * keypair;
    mbedtls_mpi           rMpi;
    mbedtls_mpi           sMpi;

    mbedtls_pk_init(&pkCtx);
    mbedtls_ecdsa_init(&ctx);
    mbedtls_mpi_init(&rMpi);
    mbedtls_mpi_init(&sMpi);

    // Parse a private key in PEM format.
    VerifyOrExit(mbedtls_pk_parse_key(&pkCtx, aPrivateKey, aPrivateKeyLength, nullptr, 0) == 0,
                 error = OT_ERROR_INVALID_ARGS);
    VerifyOrExit(mbedtls_pk_get_type(&pkCtx) == MBEDTLS_PK_ECKEY, error = OT_ERROR_INVALID_ARGS);

    keypair = mbedtls_pk_ec(pkCtx);
    OT_ASSERT(keypair != nullptr);

    VerifyOrExit(mbedtls_ecdsa_from_keypair(&ctx, keypair) == 0, error = OT_ERROR_FAILED);

    // Sign using ECDSA.
    VerifyOrExit(mbedtls_ecdsa_sign(&ctx.grp, &rMpi, &sMpi, &ctx.d, aInputHash, aInputHashLength,
                                    mbedtls_ctr_drbg_random, Random::Crypto::MbedTlsContextGet()) == 0,
                 error = OT_ERROR_FAILED);
    VerifyOrExit(mbedtls_mpi_size(&rMpi) + mbedtls_mpi_size(&sMpi) <= aOutputLength, error = OT_ERROR_NO_BUFS);

    // Concatenate the two octet sequences in the order R and then S.
    VerifyOrExit(mbedtls_mpi_write_binary(&rMpi, aOutput, mbedtls_mpi_size(&rMpi)) == 0, error = OT_ERROR_FAILED);
    aOutputLength = static_cast<uint16_t>(mbedtls_mpi_size(&rMpi));

    VerifyOrExit(mbedtls_mpi_write_binary(&sMpi, aOutput + aOutputLength, mbedtls_mpi_size(&sMpi)) == 0,
                 error = OT_ERROR_FAILED);
    aOutputLength += mbedtls_mpi_size(&sMpi);

exit:
    mbedtls_mpi_free(&rMpi);
    mbedtls_mpi_free(&sMpi);
    mbedtls_ecdsa_free(&ctx);
    mbedtls_pk_free(&pkCtx);

    return error;
}

#endif // OPENTHREAD_CONFIG_ECDSA_ENABLE

} // namespace Ecdsa
} // namespace Crypto
} // namespace ot
