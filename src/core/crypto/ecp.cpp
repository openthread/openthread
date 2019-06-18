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

namespace ot {
namespace Crypto {

#if OPENTHREAD_ENABLE_EST_CLIENT

otError Ecp::KeyPairGeneration(const uint8_t *aPersonalSeed,
                               uint32_t       aPersonalSeedLength,
                               uint8_t *      aPrivateKey,
                               uint32_t *     aPrivateKeyLength,
                               uint8_t *      aPublicKey,
                               uint32_t *     aPublicKeyLength)
{
    OT_UNUSED_VARIABLE(aPersonalSeed);
    OT_UNUSED_VARIABLE(aPersonalSeedLength);
    OT_UNUSED_VARIABLE(aPrivateKey);
    OT_UNUSED_VARIABLE(aPrivateKeyLength);
    OT_UNUSED_VARIABLE(aPublicKey);
    OT_UNUSED_VARIABLE(aPublicKeyLength);

    otError error = OT_ERROR_NOT_IMPLEMENTED;

    VerifyOrExit(error);

exit:

    return error;
}

#endif // OPENTHREAD_ENABLE_EST_CLIENT

} // namespace Crypto
} // namespace ot
