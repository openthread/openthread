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
 *   This file includes functions for the Thread Border Agent role.
 */

#ifndef OPENTHREAD_BORDER_AGENT_H_
#define OPENTHREAD_BORDER_AGENT_H_

#include <openthread/instance.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @addtogroup api-border-agent
 *
 * @brief
 *   This module includes functions for the Thread Border Agent role.
 *
 * @{
 *
 */

/**
 * The length of Border Agent/Router ID in bytes.
 *
 */
#define OT_BORDER_AGENT_ID_LENGTH (16)

/**
 * @struct otBorderAgentId
 *
 * Represents a Border Agent ID.
 *
 */
OT_TOOL_PACKED_BEGIN
struct otBorderAgentId
{
    uint8_t mId[OT_BORDER_AGENT_ID_LENGTH];
} OT_TOOL_PACKED_END;

/**
 * Represents a Border Agent ID.
 *
 */
typedef struct otBorderAgentId otBorderAgentId;

/**
 * Defines the Border Agent state.
 *
 */
typedef enum otBorderAgentState
{
    OT_BORDER_AGENT_STATE_STOPPED = 0, ///< Border agent role is disabled.
    OT_BORDER_AGENT_STATE_STARTED = 1, ///< Border agent is started.
    OT_BORDER_AGENT_STATE_ACTIVE  = 2, ///< Border agent is connected with external commissioner.
} otBorderAgentState;

/**
 * Gets the #otBorderAgentState of the Thread Border Agent role.
 *
 * @param[in]  aInstance  A pointer to an OpenThread instance.
 *
 * @returns The current #otBorderAgentState of the Border Agent.
 *
 */
otBorderAgentState otBorderAgentGetState(otInstance *aInstance);

/**
 * Gets the UDP port of the Thread Border Agent service.
 *
 * @param[in]  aInstance  A pointer to an OpenThread instance.
 *
 * @returns UDP port of the Border Agent.
 *
 */
uint16_t otBorderAgentGetUdpPort(otInstance *aInstance);

/**
 * Gets the randomly generated Border Agent ID.
 *
 * The ID is saved in persistent storage and survives reboots. The typical use case of the ID is to
 * be published in the MeshCoP mDNS service as the `id` TXT value for the client to identify this
 * Border Router/Agent device.
 *
 * @param[in]    aInstance  A pointer to an OpenThread instance.
 * @param[out]   aId        A pointer to buffer to receive the ID.
 *
 * @retval OT_ERROR_NONE  If successfully retrieved the Border Agent ID.
 * @retval ...            If failed to retrieve the Border Agent ID.
 *
 * @sa otBorderAgentSetId
 *
 */
otError otBorderAgentGetId(otInstance *aInstance, otBorderAgentId *aId);

/**
 * Sets the Border Agent ID.
 *
 * The Border Agent ID will be saved in persistent storage and survive reboots. It's required to
 * set the ID only once after factory reset. If the ID has never been set by calling this function,
 * a random ID will be generated and returned when `otBorderAgentGetId` is called.
 *
 * @param[in]    aInstance  A pointer to an OpenThread instance.
 * @param[out]   aId        A pointer to the Border Agent ID.
 *
 * @retval OT_ERROR_NONE  If successfully set the Border Agent ID.
 * @retval ...            If failed to set the Border Agent ID.
 *
 * @sa otBorderAgentGetId
 *
 */
otError otBorderAgentSetId(otInstance *aInstance, const otBorderAgentId *aId);

/**
 * @}
 *
 */

#ifdef __cplusplus
} // end of extern "C"
#endif

#endif // OPENTHREAD_BORDER_AGENT_H_
