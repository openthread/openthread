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
 *  This file includes definitions for OpenThread entropy generation.
 */

#ifndef ENTROPY_HPP_
#define ENTROPY_HPP_

#include "openthread-core-config.h"

#include "utils/wrap_stdint.h"

#include <openthread/error.h>

#ifndef OPENTHREAD_RADIO

#include <mbedtls/entropy.h>

#endif // OPENTHREAD_RADIO

namespace ot {
namespace Entropy {

/**
 * This function initializes entropy source.
 *
 */
void Init(void);

/**
 * This function deinitializes entropy source.
 *
 */
void Deinit(void);

#ifndef OPENTHREAD_RADIO

/**
 * This function returns initialized mbedtls_entropy_context.
 *
 * @returns  A pointer to initialized mbedtls_entropy_context.
 */
mbedtls_entropy_context *MbedTlsContextGet(void);

#endif // OPENTHREAD_RADIO

/**
 * This function generates and returns a 32 bit entropy.
 *
 * @param aVal[out]  A generated 32 bit entropy value.
 *
 * @retval OT_ERROR_NONE    Successfully generated 32 bit entropy.
 *
 */
otError GetUint32(uint32_t &aVal);

} // namespace Entropy
} // namespace ot

#endif // ENTROPY_HPP_
