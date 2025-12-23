/*
 *  Copyright (c) 2025, The OpenThread Authors.
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
 *  This file defines the OpenThread IEEE 802.15.4 Link Layer API.
 */

#ifndef OPENTHREAD_PROVISIONAL_LINK_H_
#define OPENTHREAD_PROVISIONAL_LINK_H_

#include <openthread/platform/radio.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Gets the Enhanced CSL channel.
 *
 * @param[in]  aInstance      A pointer to an OpenThread instance.
 *
 * @returns The Enhanced CSL channel.
 */
uint8_t otLinkGetEnhancedCslChannel(otInstance *aInstance);

/**
 * Sets the Enhanced CSL channel.
 *
 * @param[in]  aInstance      A pointer to an OpenThread instance.
 * @param[in]  aChannel       The Enhanced CSL sample channel. Channel value should be `0` (Set Enhanced CSL Channel
 *                            unspecified) or within the range [1, 10] (if 915-MHz supported) and [11, 26] (if 2.4 GHz
 *                            supported).
 *
 * @retval OT_ERROR_NONE           Successfully set the Enhanced CSL parameters.
 * @retval OT_ERROR_INVALID_ARGS   Invalid @p aChannel.
 */
otError otLinkSetEnhancedCslChannel(otInstance *aInstance, uint8_t aChannel);

/**
 * Represents Enhanced CSL period unit in microseconds.
 *
 * The Enhanced CSL period (in micro seconds) MUST be a multiple of this value.
 */
#define OT_LINK_ENHANCED_CSL_PERIOD_UNIT_IN_USEC (1250)

/**
 * Gets the Enhanced CSL period in microseconds
 *
 * @param[in]  aInstance   A pointer to an OpenThread instance.
 *
 * @returns The Enhanced CSL period in microseconds.
 */
uint32_t otLinkGetEnhancedCslPeriod(otInstance *aInstance);

/**
 * Sets the  Enhanced CSL period in microseconds. Disable CSL by setting this parameter to `0`.
 *
 * If the Enhanced CSL period is not a multiple of `OT_LINK_CSL_PERIOD_TEN_SYMBOLS_UNIT_IN_USEC`,
 * the Enhanced CSL period will be aligned.
 *
 * @param[in]  aInstance  A pointer to an OpenThread instance.
 * @param[in]  aPeriod    The Enhanced CSL period in microseconds.
 *
 * @retval OT_ERROR_NONE           Successfully set the  Enhanced CSL period.
 * @retval OT_ERROR_INVALID_ARGS   Invalid Enhanced CSL period
 */
otError otLinkSetEnhancedCslPeriod(otInstance *aInstance, uint32_t aPeriod);

/**
 * @}
 */

#ifdef __cplusplus
} // extern "C"
#endif

#endif // OPENTHREAD_PROVISIONAL_LINK_H_
