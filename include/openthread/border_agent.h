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
 */

/**
 * The length of Border Agent/Router ID in bytes.
 */
#define OT_BORDER_AGENT_ID_LENGTH (16)

/**
 * Minimum length of the ephemeral key string.
 */
#define OT_BORDER_AGENT_MIN_EPHEMERAL_KEY_LENGTH (6)

/**
 * Maximum length of the ephemeral key string.
 */
#define OT_BORDER_AGENT_MAX_EPHEMERAL_KEY_LENGTH (32)

/**
 * Default ephemeral key timeout interval in milliseconds.
 */
#define OT_BORDER_AGENT_DEFAULT_EPHEMERAL_KEY_TIMEOUT (2 * 60 * 1000u)

/**
 * Maximum ephemeral key timeout interval in milliseconds.
 */
#define OT_BORDER_AGENT_MAX_EPHEMERAL_KEY_TIMEOUT (10 * 60 * 1000u)

/**
 * Represents a Border Agent Identifier.
 */
typedef struct otBorderAgentId
{
    uint8_t mId[OT_BORDER_AGENT_ID_LENGTH]; ///< Border Agent ID bytes.
} otBorderAgentId;

/**
 * Defines the Border Agent state.
 */
typedef enum otBorderAgentState
{
    OT_BORDER_AGENT_STATE_STOPPED = 0, ///< Border agent role is disabled.
    OT_BORDER_AGENT_STATE_STARTED = 1, ///< Border agent is started.
    OT_BORDER_AGENT_STATE_ACTIVE  = 2, ///< Border agent is connected with external commissioner.
} otBorderAgentState;

typedef struct otBorderAgentCounters
{
    uint32_t mEpskcActivations;              ///< The number of ePSKc activations
    uint32_t mEpskcDeactivationClears;       ///< The number of ePSKc deactivations via API
    uint32_t mEpskcDeactivationTimeouts;     ///< The number of ePSKc deactivations due to timeout
    uint32_t mEpskcDeactivationMaxAttempts;  ///< The number of ePSKc deactivations due to reached max attempts
    uint32_t mEpskcDeactivationDisconnects;  ///< The number of ePSKc deactivations due to commissioner disconnected
    uint32_t mEpskcInvalidBaStateErrors;     ///< The number of invalid border agent state errors at ePSKc activation
    uint32_t mEpskcInvalidArgsErrors;        ///< The number of invalid args errors at ePSKc activation
    uint32_t mEpskcStartSecureSessionErrors; ///< The number of start secure session errors at ePSKc activation
    uint32_t mEpskcSecureSessionSuccesses;   ///< The number of established secure sessions with ePSKc
    uint32_t mEpskcSecureSessionFailures;    ///< The number of failed secure sessions with ePSKc
    uint32_t mEpskcCommissionerPetitions;    ///< The number of successful commissioner petitions with ePSKc

    uint32_t mPskcSecureSessionSuccesses; ///< The number of established secure sessions with PSKc
    uint32_t mPskcSecureSessionFailures;  ///< The number of failed secure sessions with PSKc
    uint32_t mPskcCommissionerPetitions;  ///< The number of successful commissioner petitions with PSKc

    uint32_t mMgmtActiveGets;  ///< The number of MGMT_ACTIVE_GET.req sent over secure sessions
    uint32_t mMgmtPendingGets; ///< The number of MGMT_PENDING_GET.req sent over secure sessions
} otBorderAgentCounters;

/**
 * Gets the counters of the Thread Border Agent.
 *
 * @param[in]  aInstance  A pointer to an OpenThread instance.
 *
 * @returns A pointer to the Border Agent counters.
 */
const otBorderAgentCounters *otBorderAgentGetCounters(otInstance *aInstance);

/**
 * Gets the #otBorderAgentState of the Thread Border Agent role.
 *
 * @param[in]  aInstance  A pointer to an OpenThread instance.
 *
 * @returns The current #otBorderAgentState of the Border Agent.
 */
otBorderAgentState otBorderAgentGetState(otInstance *aInstance);

/**
 * Gets the UDP port of the Thread Border Agent service.
 *
 * @param[in]  aInstance  A pointer to an OpenThread instance.
 *
 * @returns UDP port of the Border Agent.
 */
uint16_t otBorderAgentGetUdpPort(otInstance *aInstance);

/**
 * Gets the randomly generated Border Agent ID.
 *
 * Requires `OPENTHREAD_CONFIG_BORDER_AGENT_ID_ENABLE`.
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
 */
otError otBorderAgentGetId(otInstance *aInstance, otBorderAgentId *aId);

/**
 * Sets the Border Agent ID.
 *
 * Requires `OPENTHREAD_CONFIG_BORDER_AGENT_ID_ENABLE`.
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
 */
otError otBorderAgentSetId(otInstance *aInstance, const otBorderAgentId *aId);

/**
 * Sets the ephemeral key for a given timeout duration.
 *
 * Requires `OPENTHREAD_CONFIG_BORDER_AGENT_EPHEMERAL_KEY_ENABLE`.
 *
 * The ephemeral key can be set when the Border Agent is already running and is not currently connected to any external
 * commissioner (i.e., it is in `OT_BORDER_AGENT_STATE_STARTED` state). Otherwise `OT_ERROR_INVALID_STATE` is returned.
 * To terminate active commissioner sessions, use the `otBorderAgentDisconnect()` API.
 *
 * The given @p aKeyString is directly used as the ephemeral PSK (excluding the trailing null `\0` character ).
 * The @p aKeyString length must be between `OT_BORDER_AGENT_MIN_EPHEMERAL_KEY_LENGTH` and
 * `OT_BORDER_AGENT_MAX_EPHEMERAL_KEY_LENGTH`, inclusive.
 *
 * Setting the ephemeral key again before a previously set key has timed out will replace the previously set key and
 * reset the timeout.
 *
 * During the timeout interval, the ephemeral key can be used only once by an external commissioner to establish a
 * connection. After the commissioner disconnects, the ephemeral key is cleared, and the Border Agent reverts to
 * using PSKc. If the timeout expires while a commissioner is still connected, the session will be terminated, and the
 * Border Agent will cease using the ephemeral key and revert to PSKc.
 *
 * @param[in] aInstance    The OpenThread instance.
 * @param[in] aKeyString   The ephemeral key string (used as PSK excluding the trailing null `\0` character).
 * @param[in] aTimeout     The timeout duration in milliseconds to use the ephemeral key.
 *                         If zero, the default `OT_BORDER_AGENT_DEFAULT_EPHEMERAL_KEY_TIMEOUT` value will be used.
 *                         If the given timeout value is larger than `OT_BORDER_AGENT_MAX_EPHEMERAL_KEY_TIMEOUT`, the
 *                         max value `OT_BORDER_AGENT_MAX_EPHEMERAL_KEY_TIMEOUT` will be used instead.
 * @param[in] aUdpPort     The UDP port to use with ephemeral key. If zero, an ephemeral port will be used.
 *                         `otBorderAgentGetUdpPort()` will return the current UDP port being used.
 *
 * @retval OT_ERROR_NONE           Successfully set the ephemeral key.
 * @retval OT_ERROR_INVALID_STATE  Border Agent is not running or it is connected to an external commissioner.
 * @retval OT_ERROR_INVALID_ARGS   The given @p aKeyString is not valid (too short or too long).
 * @retval OT_ERROR_FAILED         Failed to set the key (e.g., could not bind to UDP port).

 */
otError otBorderAgentSetEphemeralKey(otInstance *aInstance,
                                     const char *aKeyString,
                                     uint32_t    aTimeout,
                                     uint16_t    aUdpPort);

/**
 * Cancels the ephemeral key that is in use.
 *
 * Requires `OPENTHREAD_CONFIG_BORDER_AGENT_EPHEMERAL_KEY_ENABLE`.
 *
 * Can be used to cancel a previously set ephemeral key before it times out. If the Border Agent is not running or
 * there is no ephemeral key in use, calling this function has no effect.
 *
 * If a commissioner is connected using the ephemeral key and is currently active, calling this function does not
 * change its state. In this case the `otBorderAgentIsEphemeralKeyActive()` will continue to return `TRUE` until the
 * commissioner disconnects, or the ephemeral key timeout expires. To terminate active commissioner sessions, use the
 * `otBorderAgentDisconnect()` API.
 *
 * @param[in] aInstance    The OpenThread instance.
 */
void otBorderAgentClearEphemeralKey(otInstance *aInstance);

/**
 * Indicates whether or not an ephemeral key is currently active.
 *
 * Requires `OPENTHREAD_CONFIG_BORDER_AGENT_EPHEMERAL_KEY_ENABLE`.
 *
 * @param[in] aInstance    The OpenThread instance.
 *
 * @retval TRUE    An ephemeral key is active.
 * @retval FALSE   No ephemeral key is active.
 */
bool otBorderAgentIsEphemeralKeyActive(otInstance *aInstance);

/**
 * Callback function pointer to signal changes related to the Border Agent's ephemeral key.
 *
 * This callback is invoked whenever:
 *
 * - The Border Agent starts using an ephemeral key.
 * - Any parameter related to the ephemeral key, such as the port number, changes.
 * - The Border Agent stops using the ephemeral key due to:
 *   - A direct call to `otBorderAgentClearEphemeralKey()`.
 *   - The ephemeral key timing out.
 *   - An external commissioner successfully using the key to connect and then disconnecting.
 *   - Reaching the maximum number of allowed failed connection attempts.
 *
 * Any OpenThread API, including `otBorderAgent` APIs, can be safely called from this callback.
 *
 * @param[in] aContext   A pointer to an arbitrary context (provided when callback is set).
 */
typedef void (*otBorderAgentEphemeralKeyCallback)(void *aContext);

/**
 * Sets the callback function used by the Border Agent to notify any changes related to use of ephemeral key.
 *
 * Requires `OPENTHREAD_CONFIG_BORDER_AGENT_EPHEMERAL_KEY_ENABLE`.
 *
 * A subsequent call to this function will replace any previously set callback.
 *
 * @param[in] aInstance    The OpenThread instance.
 * @param[in] aCallback    The callback function pointer.
 * @param[in] aContext     The arbitrary context to use with callback.
 */
void otBorderAgentSetEphemeralKeyCallback(otInstance                       *aInstance,
                                          otBorderAgentEphemeralKeyCallback aCallback,
                                          void                             *aContext);

/**
 * Disconnects the Border Agent from any active secure sessions.
 *
 * If Border Agent is connected to a commissioner candidate with ephemeral key, calling this API
 * will cause the ephemeral key to be cleared after the session is disconnected.
 *
 * The Border Agent state may not change immediately upon calling this method. The state will be
 * updated when the connection update is notified with a delay.
 *
 * @param[in] aInstance    The OpenThread instance.
 */
void otBorderAgentDisconnect(otInstance *aInstance);

/**
 * @}
 */

#ifdef __cplusplus
} // end of extern "C"
#endif

#endif // OPENTHREAD_BORDER_AGENT_H_
