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

#include <mbedtls/ecdsa.h>
#include <mbedtls/pk.h>

#include "common/code_utils.hpp"
#include "common/debug.hpp"
#include "common/random.hpp"

namespace ot {
namespace Crypto {

#if OPENTHREAD_ENABLE_ECDSA

otError Ecdsa::Sign(uint8_t *      aOutput,
                    uint16_t *     aOutputLength,
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
    VerifyOrExit(mbedtls_pk_parse_key(&pkCtx, aPrivateKey, aPrivateKeyLength, NULL, 0) == 0,
                 error = OT_ERROR_INVALID_ARGS);
    VerifyOrExit(mbedtls_pk_get_type(&pkCtx) == MBEDTLS_PK_ECKEY, error = OT_ERROR_INVALID_ARGS);

    keypair = mbedtls_pk_ec(pkCtx);
    assert(keypair != NULL);

    VerifyOrExit(mbedtls_ecdsa_from_keypair(&ctx, keypair) == 0, error = OT_ERROR_FAILED);

    // Sign using ECDSA.
    VerifyOrExit(mbedtls_ecdsa_sign(&ctx.grp, &rMpi, &sMpi, &ctx.d, aInputHash, aInputHashLength, FillRandom, NULL) ==
                     0,
                 error = OT_ERROR_FAILED);
    VerifyOrExit(mbedtls_mpi_size(&rMpi) + mbedtls_mpi_size(&sMpi) <= *aOutputLength, error = OT_ERROR_NO_BUFS);

    // Concatenate the two octet sequences in the order R and then S.
    VerifyOrExit(mbedtls_mpi_write_binary(&rMpi, aOutput, mbedtls_mpi_size(&rMpi)) == 0, error = OT_ERROR_FAILED);
    *aOutputLength = static_cast<uint16_t>(mbedtls_mpi_size(&rMpi));

    VerifyOrExit(mbedtls_mpi_write_binary(&sMpi, aOutput + *aOutputLength, mbedtls_mpi_size(&sMpi)) == 0,
                 error = OT_ERROR_FAILED);
    *aOutputLength += mbedtls_mpi_size(&sMpi);

exit:
    mbedtls_mpi_free(&rMpi);
    mbedtls_mpi_free(&sMpi);
    mbedtls_ecdsa_free(&ctx);
    mbedtls_pk_free(&pkCtx);

    return error;
}

int Ecdsa::FillRandom(void *, unsigned char *aBuffer, size_t aSize)
{
    Random::FillBuffer(aBuffer, static_cast<uint16_t>(aSize));

    return 0;
}

#endif // OPENTHREAD_ENABLE_ECDSA

} // namespace Crypto
} // namespace ot
