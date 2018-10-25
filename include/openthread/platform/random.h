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
 * @brief
 *   This file includes the platform abstraction for random number generation.
 */

#ifndef OPENTHREAD_PLATFORM_RANDOM_H_
#define OPENTHREAD_PLATFORM_RANDOM_H_

#include <stdint.h>

#include <openthread/error.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @addtogroup plat-random
 *
 * @brief
 *   This module includes the platform abstraction for random number generation.
 *
 * @{
 *
 */

/**
 * Get a 32-bit random value.
 *
 * This function may be implemented using a pseudo-random number generator.
 *
 * @returns A 32-bit random value.
 *
 */
uint32_t otPlatRandomGet(void);

/**
 * Get true random value sequence.
 *
 * This function MUST be implemented using a true random number generator (TRNG).
 *
 * @param[out]  aOutput              A pointer to where the true random values are placed.  Must not be NULL.
 * @param[in]   aOutputLength        Size of @p aBuffer.
 *
 * @retval OT_ERROR_NONE          Successfully filled @p aBuffer with true random values.
 * @retval OT_ERROR_FAILED         Failed to fill @p aBuffer with true random values.
 * @retval OT_ERROR_INVALID_ARGS  @p aBuffer was set to NULL.
 *
 */
otError otPlatRandomGetTrue(uint8_t *aOutput, uint16_t aOutputLength);

/**
 * @}
 *
 */

#ifdef __cplusplus
} // end of extern "C"
#endif

#endif // OPENTHREAD_PLATFORM_RANDOM_H_
