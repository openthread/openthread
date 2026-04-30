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
 *   This file includes functions for the Thread Border Agent Ephemeral Key. */

#ifndef OPENTHREAD_BORDER_AGENT_EPHEMERAL_KEY_H_
#define OPENTHREAD_BORDER_AGENT_EPHEMERAL_KEY_H_

#include <stdbool.h>
#include <stdint.h>

#include <openthread/error.h>
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
 * The string length of Thread Administration One-Time Passcode (TAP).
 */
#define OT_BORDER_AGENT_EPHEMERAL_KEY_TAP_STRING_LENGTH 9

/**
 * Represents Border Agent's Ephemeral Key Manager state.
 */
typedef enum otBorderAgentEphemeralKeyState
{
    OT_BORDER_AGENT_STATE_DISABLED  = 0, ///< Ephemeral Key Manager is disabled.
    OT_BORDER_AGENT_STATE_STOPPED   = 1, ///< Enabled, but no ephemeral key is in use (not set or started).
    OT_BORDER_AGENT_STATE_STARTED   = 2, ///< Ephemeral key is set. Listening to accept secure connections.
    OT_BORDER_AGENT_STATE_CONNECTED = 3, ///< Session is established with an external commissioner candidate.
    OT_BORDER_AGENT_STATE_ACCEPTED  = 4, ///< Session is established and candidate is accepted as full commissioner.
} otBorderAgentEphemeralKeyState;

/**
 * Represents a Thread Administration One-Time Passcode (TAP).
 */
typedef struct otBorderAgentEphemeralKeyTap
{
    char mTap[OT_BORDER_AGENT_EPHEMERAL_KEY_TAP_STRING_LENGTH + 1]; ///< TAP string buffer (including `\0` character).
} otBorderAgentEphemeralKeyTap;

/**
 * Gets the state of Border Agent's Ephemeral Key Manager.
 *
 * Requires `OPENTHREAD_CONFIG_BORDER_AGENT_EPHEMERAL_KEY_ENABLE`.
 *
 * @param[in]  aInstance  A pointer to an OpenThread instance.
 *
 * @returns The current state of Ephemeral Key Manager.
 */
otBorderAgentEphemeralKeyState otBorderAgentEphemeralKeyGetState(otInstance *aInstance);

/**
 * Enables/disables the Border Agent's Ephemeral Key Manager.
 *
 * Requires `OPENTHREAD_CONFIG_BORDER_AGENT_EPHEMERAL_KEY_ENABLE`.
 *
 * If this function is called to disable, while an an ephemeral key is in use, the ephemeral key use will be stopped
 * (as if `otBorderAgentEphemeralKeyStop()` is called).
 *
 * @param[in] aInstance    The OpenThread instance.
 * @param[in] aEnabled     Whether to enable or disable the Ephemeral Key Manager.
 */
void otBorderAgentEphemeralKeySetEnabled(otInstance *aInstance, bool aEnabled);

/**
 * Starts using an ephemeral key for a given timeout duration.
 *
 * Requires `OPENTHREAD_CONFIG_BORDER_AGENT_EPHEMERAL_KEY_ENABLE`.
 *
 * An ephemeral key can only be set when `otBorderAgentEphemeralKeyGetState()` is `OT_BORDER_AGENT_STATE_STOPPED`,
 * i.e., enabled but not yet started. Otherwise, `OT_ERROR_INVALID_STATE` is returned. This means that setting the
 * ephemeral key again while a previously set key is still in use will fail. Callers can stop the previous key by
 * calling `otBorderAgentEphemeralKeyStop()` before starting with a new key.
 *
 * The Ephemeral Key Manager and the Border Agent service (which uses PSKc) can be enabled and used in parallel, as
 * they use independent and separate DTLS transport and sessions.
 *
 * The given @p aKeyString is used directly as the ephemeral PSK (excluding the trailing null `\0` character).
 * Its length must be between `OT_BORDER_AGENT_MIN_EPHEMERAL_KEY_LENGTH` and `OT_BORDER_AGENT_MAX_EPHEMERAL_KEY_LENGTH`,
 * inclusive. Otherwise `OT_ERROR_INVALID_ARGS` is returned.
 *
 * When successfully set, the ephemeral key can be used only once by an external commissioner candidate to establish a
 * secure session. After the commissioner candidate disconnects, the use of the ephemeral key is stopped. If the
 * timeout expires, the use of the ephemeral key is stopped, and any connected session using the key is immediately
 * disconnected.
 *
 * The Ephemeral Key Manager limits the number of failed DTLS connections to 10 attempts. After the 10th failed
 * attempt, the use of the ephemeral key is automatically stopped (even if the timeout has not yet expired).
 *
 * @param[in] aInstance    The OpenThread instance.
 * @param[in] aKeyString   The ephemeral key.
 * @param[in] aTimeout     The timeout duration, in milliseconds, to use the ephemeral key.
 *                         If zero, the default `OT_BORDER_AGENT_DEFAULT_EPHEMERAL_KEY_TIMEOUT` value is used. If the
 *                         timeout value is larger than `OT_BORDER_AGENT_MAX_EPHEMERAL_KEY_TIMEOUT`, the maximum value
 *                         is used instead.
 * @param[in] aUdpPort     The UDP port to use with the ephemeral key. If the UDP port is zero, an ephemeral port will
 *                         be used. `otBorderAgentEphemeralKeyGetUdpPort()` returns the current UDP port being used.
 *
 * @retval OT_ERROR_NONE            Successfully started using the ephemeral key.
 * @retval OT_ERROR_INVALID_STATE   A previously set ephemeral key is still in use or the feature is disabled.
 * @retval OT_ERROR_INVALID_ARGS    The given @p aKeyString is not valid.
 * @retval OT_ERROR_FAILED          Failed to start (e.g., it could not bind to the given UDP port).
 */
otError otBorderAgentEphemeralKeyStart(otInstance *aInstance,
                                       const char *aKeyString,
                                       uint32_t    aTimeout,
                                       uint16_t    aUdpPort);

/**
 * Stops the ephemeral key use and disconnects any session using it.
 *
 * Requires `OPENTHREAD_CONFIG_BORDER_AGENT_EPHEMERAL_KEY_ENABLE`.
 *
 * If there is no ephemeral key in use, calling this function has no effect.
 *
 * @param[in] aInstance    The OpenThread instance.
 */
void otBorderAgentEphemeralKeyStop(otInstance *aInstance);

/**
 * Gets the UDP port used by Border Agent's Ephemeral Key Manager.
 *
 * Requires `OPENTHREAD_CONFIG_BORDER_AGENT_EPHEMERAL_KEY_ENABLE`.
 *
 * The port is applicable if an ephemeral key is in use, i.e., the state is not `OT_BORDER_AGENT_STATE_DISABLED` or
 * `OT_BORDER_AGENT_STATE_STOPPED`.
 *
 * @param[in]  aInstance  A pointer to an OpenThread instance.
 *
 * @returns The UDP port being used by Border Agent's Ephemeral Key Manager (when active).
 */
uint16_t otBorderAgentEphemeralKeyGetUdpPort(otInstance *aInstance);

/**
 * Callback function pointer to signal state changes to the Border Agent's Ephemeral Key Manager.
 *
 * This callback is invoked whenever the `otBorderAgentEphemeralKeyGetState()` gets changed.
 *
 * Any OpenThread API, including `otBorderAgent` APIs, can be safely called from this callback.
 *
 * @param[in] aContext   A pointer to an arbitrary context (provided when callback is set).
 */
typedef void (*otBorderAgentEphemeralKeyCallback)(void *aContext);

/**
 * Sets the callback function to notify state changes of Border Agent's Ephemeral Key Manager.
 *
 * Requires `OPENTHREAD_CONFIG_BORDER_AGENT_EPHEMERAL_KEY_ENABLE`.
 *
 * A subsequent call to this function will replace any previously set callback.
 *
 * @param[in] aInstance    The OpenThread instance.
 * @param[in] aCallback    The callback function pointer.
 * @param[in] aContext     The arbitrary context to use with callback.
 */
void otBorderAgentEphemeralKeySetCallback(otInstance                       *aInstance,
                                          otBorderAgentEphemeralKeyCallback aCallback,
                                          void                             *aContext);

/**
 * Converts a given `otBorderAgentEphemeralKeyState` to a human-readable string.
 *
 * @param[in] aState   The state to convert.
 *
 * @returns Human-readable string corresponding to @p aState.
 */
const char *otBorderAgentEphemeralKeyStateToString(otBorderAgentEphemeralKeyState aState);

/**
 * Generates a cryptographically secure random Thread Administration One-Time Passcode (TAP) string.
 *
 * Requires `OPENTHREAD_CONFIG_BORDER_AGENT_EPHEMERAL_KEY_ENABLE` and `OPENTHREAD_CONFIG_VERHOEFF_CHECKSUM_ENABLE`.
 *
 * The TAP is a string of 9 characters, generated as a sequence of eight cryptographically secure random
 * numeric digits [`0`-`9`] followed by a single check digit determined using the Verhoeff algorithm.
 *
 * @param[out] aTap         A pointer to an `otBorderAgentEphemeralKeyTap` to output the generated TAP.
 *
 * @retval OT_ERROR_NONE    Successfully generated a random TAP. @p aTap is updated.
 * @retval OT_ERROR_FAILED  Failed to generate a random TAP.
 */
otError otBorderAgentEphemeralKeyGenerateTap(otBorderAgentEphemeralKeyTap *aTap);

/**
 * Validates a given Thread Administration One-Time Passcode (TAP) string.
 *
 * Requires `OPENTHREAD_CONFIG_BORDER_AGENT_EPHEMERAL_KEY_ENABLE` and `OPENTHREAD_CONFIG_VERHOEFF_CHECKSUM_ENABLE`.
 *
 * Validates that the TAP string has the proper length, contains digit characters [`0`-`9`], and validates the
 * Verhoeff checksum.
 *
 * @param[in] aTap     The `otBorderAgentEphemeralKeyTap` to validate.
 *
 * @retval OT_ERROR_NONE            Successfully validated the @p aTap.
 * @retval OT_ERROR_INVALID_ARGS    The @p aTap string has an invalid length or contains non-digit characters.
 * @retval OT_ERROR_FAILED          Checksum validation failed.
 */
otError otBorderAgentEphemeralKeyValidateTap(const otBorderAgentEphemeralKeyTap *aTap);

/**
 * @}
 */

#ifdef __cplusplus
} // end of extern "C"
#endif

#endif // OPENTHREAD_BORDER_AGENT_EPHEMERAL_KEY_H_
