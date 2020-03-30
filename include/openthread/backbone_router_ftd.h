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
 *  This file defines the OpenThread Backbone Router API (for Thread 1.2 FTD with
 *  `OPENTHREAD_CONFIG_BACKBONE_ROUTER_ENABLE`).
 *
 */

#ifndef OPENTHREAD_BACKBONE_ROUTER_FTD_H_
#define OPENTHREAD_BACKBONE_ROUTER_FTD_H_

#include <openthread/backbone_router.h>
#include <openthread/ip6.h>
#include <openthread/netdata.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @addtogroup api-backbone-router
 *
 * @{
 *
 */

/**
 * Represents the Backbone Router Status.
 *
 */
typedef enum
{
    OT_BACKBONE_ROUTER_STATE_DISABLED  = 0, ///< Backbone function is disabled.
    OT_BACKBONE_ROUTER_STATE_SECONDARY = 1, ///< Secondary Backbone Router.
    OT_BACKBONE_ROUTER_STATE_PRIMARY   = 2, ///< The Primary Backbone Router.
} otBackboneRouterState;

/**
 * This function enables or disables Backbone functionality.
 *
 * @param[in] aInstance A pointer to an OpenThread instance.
 * @param[in] aEnable   TRUE to enable Backbone functionality, FALSE otherwise.
 *
 * @sa otBackboneRouterGetState
 * @sa otBackboneRouterGetConfig
 * @sa otBackboneRouterSetConfig
 * @sa otBackboneRouterRegister
 *
 */
void otBackboneRouterSetEnabled(otInstance *aInstance, bool aEnable);

/**
 * This function gets the Backbone Router state.
 *
 * @param[in] aInstance       A pointer to an OpenThread instance.
 *
 * @retval OT_BACKBONE_ROUTER_STATE_DISABLED   Backbone functionality is disabled.
 * @retval OT_BACKBONE_ROUTER_STATE_SECONDARY  Secondary Backbone Router.
 * @retval OT_BACKBONE_ROUTER_STATE_PRIMARY    The Primary Backbone Router.
 *
 * @sa otBackboneRouterSetEnabled
 * @sa otBackboneRouterGetConfig
 * @sa otBackboneRouterSetConfig
 * @sa otBackboneRouterRegister
 *
 */
otBackboneRouterState otBackboneRouterGetState(otInstance *aInstance);

/**
 * This function gets the local Backbone Router configuration.
 *
 * @param[in]   aInstance            A pointer to an OpenThread instance.
 * @param[out]  aConfig              A pointer where to put local Backbone Router configuration.
 *
 *
 * @sa otBackboneRouterSetEnabled
 * @sa otBackboneRouterGetState
 * @sa otBackboneRouterSetConfig
 * @sa otBackboneRouterRegister
 *
 */
void otBackboneRouterGetConfig(otInstance *aInstance, otBackboneRouterConfig *aConfig);

/**
 * This function sets the local Backbone Router configuration.
 *
 * @param[in]  aInstance             A pointer to an OpenThread instance.
 * @param[in]  aConfig               A pointer to the Backbone Router configuration to take effect.
 *
 * @sa otBackboneRouterSetEnabled
 * @sa otBackboneRouterGetState
 * @sa otBackboneRouterGetConfig
 * @sa otBackboneRouterRegister
 *
 */
void otBackboneRouterSetConfig(otInstance *aInstance, const otBackboneRouterConfig *aConfig);

/**
 * This function explicitly registers local Backbone Router configuration.
 *
 * @param[in]  aInstance             A pointer to an OpenThread instance.
 *
 * @retval OT_ERROR_NO_BUFS           Insufficient space to add the Backbone Router service.
 * @retval OT_ERROR_NONE              Successfully queued a Server Data Request message for delivery.
 *
 * @sa otBackboneRouterSetEnabled
 * @sa otBackboneRouterGetState
 * @sa otBackboneRouterGetConfig
 * @sa otBackboneRouterSetConfig
 *
 */
otError otBackboneRouterRegister(otInstance *aInstance);

/**
 * This method returns the Backbone Router registration jitter value.
 *
 * @returns The Backbone Router registration jitter value.
 *
 * @sa otBackboneRouterSetRegistrationJitter
 *
 */
uint8_t otBackboneRouterGetRegistrationJitter(otInstance *aInstance);

/**
 * This method sets the Backbone Router registration jitter value.
 *
 * @param[in]  aJitter the Backbone Router registration jitter value to set.
 *
 * @sa otBackboneRouterGetRegistrationJitter
 *
 */
void otBackboneRouterSetRegistrationJitter(otInstance *aInstance, uint8_t aJitter);

/**
 * This method gets the local Domain Prefix configuration.
 *
 * @param[in]  aInstance A pointer to an OpenThread instance.
 * @param[out] aConfig   A pointer to the Domain Prefix configuration.
 *
 * @retval OT_ERROR_NONE       Successfully got the Domain Prefix configuration.
 * @retval OT_ERROR_NOT_FOUND  No Domain Prefix was configured.
 *
 */
otError otBackboneRouterGetDomainPrefix(otInstance *aInstance, otBorderRouterConfig *aConfig);

/**
 * @}
 *
 */

#ifdef __cplusplus
} // extern "C"
#endif

#endif // OPENTHREAD_BACKBONE_ROUTER_FTD_H_
