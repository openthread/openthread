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
 * @brief
 *  This file defines the OpenThread IEEE 802.15.4 Physical Layer API.
 */

#ifndef OPENTHREAD_PHY_H_
#define OPENTHREAD_PHY_H_

#include <openthread/instance.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @addtogroup api-phy
 *
 * @brief
 *   This module includes functions that get physical layer configuration.
 *
 * @{
 *
 */

/**
 * This function gets the minimum channel number of IEEE 802.15.4 physical layer.
 *
 * @param[in]  aInstance          A pointer to an OpenThread instance.
 *
 * @returns The minimum channel number.
 *
 */
OTAPI uint8_t OTCALL otPhyGetChannelMin(otInstance *aInstance);

/**
 * This function gets the maximum channel number of IEEE 802.15.4 physical layer.
 *
 * @param[in]  aInstance          A pointer to an OpenThread instance.
 *
 * @returns The maximum channel number.
 *
 */
OTAPI uint8_t OTCALL otPhyGetChannelMax(otInstance *aInstance);

/**
 * This function gets the supported channel mask (as a `uint32_t` bit-vector mask with
 * bit 0 (lsb) -> channel 0, and so on).
 *
 * @param[in]  aInstance          A pointer to an OpenThread instance.
 *
 * @returns The supported channel mask.
 *
 */
OTAPI uint32_t OTCALL otPhyGetSupportedChannelMask(otInstance *aInstance);

/**
 * @}
 *
 */

#ifdef __cplusplus
} // extern "C"
#endif

#endif // OPENTHREAD_PHY_H_
