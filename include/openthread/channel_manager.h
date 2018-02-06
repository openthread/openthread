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
 * @brief
 *   This file includes the OpenThread API for Channel Manager module.
 */

#ifndef OPENTHREAD_CHANNEL_MANAGER_H_
#define OPENTHREAD_CHANNEL_MANAGER_H_

#include "openthread/types.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @addtogroup api-channel-manager
 *
 * @brief
 *   This module includes functions for Channel Manager.
 *
 *   The functions in this module are available when Channel Manager feature (`OPENTHREAD_ENABLE_CHANNEL_MANAGER`)
 *   is enabled. Channel Manager is available only on an FTD build.
 *
 * @{
 *
 */

/**
 * This function requests a Thread network channel change.
 *
 * The network switches to the given channel after a specified delay (@sa otChannelManagerSetDelay). The channel change
 * is performed by updating the Pending Operational Dataset.
 *
 * A subsequent call to this function will cancel an ongoing previously requested channel change.
 *
 * @param[in]  aInstance          A pointer to an OpenThread instance.
 * @param[in]  aChannel           The new channel for the Thread network.

 * @retval OT_ERROR_NONE          Channel change request successfully processed.
 * @retval OT_ERROR_INVALID_ARGS  The channel is not a supported channel (@sa otChannelManagerGetSupportedChannels).
 *
 */
otError otChannelManagerRequestChannelChange(otInstance *aInstance, uint8_t aChannel);

/**
 * This function gets the channel from the last successful call to `otChannelManagerRequestChannelChange()`
 *
 * @returns The last requested channel or zero if there has been no channel change request yet.
 *
 */
uint8_t otChannelManagerGetRequestedChannel(otInstance *aInstance);

/**
 * This function gets the delay (in seconds) used by Channel Manager for a channel change.
 *
 * @param[in]  aInstance          A pointer to an OpenThread instance.
 *
 * @returns The delay (in seconds) for channel change.
 *
 */
uint16_t otChannelManagerGetDelay(otInstance *aInstance);

/**
 * This function sets the delay (in seconds) used for a channel change.
 *
 * The delay should preferably be longer than maximum data poll interval used by all sleepy-end-devices within the
 * Thread network.
 *
 * @param[in]  aInstance          A pointer to an OpenThread instance.
 * @param[in]  aDelay             Delay in seconds.
 *
 * @retval OT_ERROR_NONE          Delay was updated successfully.
 * @retval OT_ERROR_INVALID_ARGS  The given delay @p aDelay is too short.
 *
 */
otError otChannelManagerSetDelay(otInstance *aInstance, uint16_t aDelay);

/**
 * This function gets the supported channel mask.
 *
 * @param[in]  aInstance       A pointer to an OpenThread instance.
 *
 * @returns  The supported channels as bit-mask.
 *
 */
uint32_t otChannelManagerGetSupportedChannels(otInstance *aInstance);

/**
 * This function sets the supported channel mask.
 *
 * @param[in]  aInstance     A pointer to an OpenThread instance.
 * @param[in]  aChannelMask  A channel mask.
 *
 */
void otChannelManagerSetSupportedChannels(otInstance *aInstance, uint32_t aChannelMask);

/**
 * @}
 *
 */

#ifdef __cplusplus
} // extern "C"
#endif

#endif // OPENTHREAD_CHANNEL_MANAGER_H_
