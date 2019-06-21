/*
 *  Copyright (c) 2019, The OpenThread Authors.
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
 *   This file implements elliptic curve key generation.
 */

#include "ecp.hpp"

#include <mbedtls/ctr_drbg.h>
#include <mbedtls/ecp.h>
#include <mbedtls/pk.h>

#include "common/code_utils.hpp"
#include "common/debug.hpp"
#include "common/random.hpp"

#include "openthread/entropy.h"
#include "openthread/random_crypto.h"

namespace ot {
namespace Crypto {

#if OPENTHREAD_ENABLE_EST_CLIENT

#define ECP_RANDOM_THRESHOLD    32

static int EntropyPollHandle(void *aData, unsigned char *aOutput, size_t aInLen, size_t *aOutLen)
{
    int error = 0;
    OT_UNUSED_VARIABLE(aData);

    VerifyOrExit(otRandomCryptoFillBuffer((uint8_t*)aOutput, (uint16_t)aInLen) == OT_ERROR_NONE,
                 error = MBEDTLS_ERR_ENTROPY_SOURCE_FAILED);

    if(aOutLen != NULL)
    {
        *aOutLen = aInLen;
    }

exit:

    return error;
}

otError Ecp::KeyPairGeneration(const uint8_t *aPersonalSeed,
                               uint32_t       aPersonalSeedLength,
                               uint8_t *      aPrivateKey,
                               uint32_t *     aPrivateKeyLength,
                               uint8_t *      aPublicKey,
                               uint32_t *     aPublicKeyLength)
{
    otError error = OT_ERROR_NONE;
    mbedtls_entropy_context *entropyCtx;
    mbedtls_ctr_drbg_context *ctrDrbgCtx;
    mbedtls_pk_context keypair;

    entropyCtx = otEntropyMbedTlsContextGet();
    ctrDrbgCtx = otRandomCryptoMbedTlsContextGet();
    mbedtls_pk_init(&keypair);

    // Setup entropy
    VerifyOrExit(mbedtls_entropy_add_source(entropyCtx, &EntropyPollHandle,
                                            NULL, ECP_RANDOM_THRESHOLD,
                                            MBEDTLS_ENTROPY_SOURCE_STRONG) == 0,
                 error = OT_ERROR_FAILED);

    // Setup CTR_DRBG context
    mbedtls_ctr_drbg_set_prediction_resistance(ctrDrbgCtx, MBEDTLS_CTR_DRBG_PR_ON);

    VerifyOrExit(mbedtls_ctr_drbg_seed(ctrDrbgCtx, mbedtls_entropy_func, entropyCtx,
                                       aPersonalSeed, aPersonalSeedLength) == 0,
                 error = OT_ERROR_FAILED);

    // Generate keypair
    VerifyOrExit(mbedtls_pk_setup(&keypair, mbedtls_pk_info_from_type(MBEDTLS_PK_ECKEY)),
                 error = OT_ERROR_FAILED);

    VerifyOrExit(mbedtls_ecp_group_load(&mbedtls_pk_ec(keypair)->grp, MBEDTLS_ECP_DP_SECP256R1) == 0,
                 error = OT_ERROR_FAILED);

    VerifyOrExit(mbedtls_ecp_gen_keypair(&mbedtls_pk_ec(keypair)->grp, &mbedtls_pk_ec(keypair)->d,
                                         &mbedtls_pk_ec(keypair)->Q, mbedtls_ctr_drbg_random,
                                         ctrDrbgCtx) == 0,
                 error = OT_ERROR_FAILED);

    VerifyOrExit(mbedtls_pk_write_pubkey_pem(&keypair, (unsigned char*)aPublicKey,
                                             *aPublicKeyLength) == 0,
                 error = OT_ERROR_INVALID_ARGS);

    VerifyOrExit(mbedtls_pk_write_key_pem(&keypair, (unsigned char*)aPrivateKey,
                                          *aPrivateKeyLength) == 0,
                 error = OT_ERROR_INVALID_ARGS);

exit:
    mbedtls_pk_free(&keypair);

    return error;
}

#endif // OPENTHREAD_ENABLE_EST_CLIENT

} // namespace Crypto
} // namespace ot
