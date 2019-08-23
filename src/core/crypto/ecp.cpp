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

#if OPENTHREAD_CONFIG_EST_CLIENT_ENABLE

otError Ecp::KeyPairGeneration(uint8_t * aPrivateKey,
                               uint32_t *aPrivateKeyLength,
                               uint8_t * aPublicKey,
                               uint32_t *aPublicKeyLength)
{
    otError            error = OT_ERROR_NONE;
    mbedtls_pk_context keypair;

    mbedtls_pk_init(&keypair);

    VerifyOrExit(mbedtls_pk_setup(&keypair, mbedtls_pk_info_from_type(MBEDTLS_PK_ECKEY)) == 0, error = OT_ERROR_FAILED);

    VerifyOrExit(mbedtls_ecp_group_load(&mbedtls_pk_ec(keypair)->grp, MBEDTLS_ECP_DP_SECP256R1) == 0,
                 error = OT_ERROR_FAILED);

    VerifyOrExit(mbedtls_ecp_gen_keypair(&mbedtls_pk_ec(keypair)->grp, &mbedtls_pk_ec(keypair)->d,
                                         &mbedtls_pk_ec(keypair)->Q, mbedtls_ctr_drbg_random,
                                         Random::Crypto::MbedTlsContextGet()) == 0,
                 error = OT_ERROR_FAILED);

    VerifyOrExit(mbedtls_pk_write_pubkey_pem(&keypair, (unsigned char *)aPublicKey, *aPublicKeyLength) == 0,
                 error = OT_ERROR_INVALID_ARGS);

    VerifyOrExit(mbedtls_pk_write_key_pem(&keypair, (unsigned char *)aPrivateKey, *aPrivateKeyLength) == 0,
                 error = OT_ERROR_INVALID_ARGS);

    *aPublicKeyLength  = strlen((char *)aPublicKey) + 1;
    *aPrivateKeyLength = strlen((char *)aPrivateKey) + 1;

exit:
    mbedtls_pk_free(&keypair);

    return error;
}

#endif // OPENTHREAD_CONFIG_EST_CLIENT_ENABLE

} // namespace Crypto
} // namespace ot
