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
#include <openthread/ip6.h>

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
 * Represents a Border Agent Identifier.
 */
typedef struct otBorderAgentId
{
    uint8_t mId[OT_BORDER_AGENT_ID_LENGTH]; ///< Border Agent ID bytes.
} otBorderAgentId;

/**
 * Defines Border Agent counters.
 *
 * The `mEpskc` related counters require `OPENTHREAD_CONFIG_BORDER_AGENT_EPHEMERAL_KEY_ENABLE`.
 */
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
    uint32_t mPskcSecureSessionSuccesses;    ///< The number of established secure sessions with PSKc
    uint32_t mPskcSecureSessionFailures;     ///< The number of failed secure sessions with PSKc
    uint32_t mPskcCommissionerPetitions;     ///< The number of successful commissioner petitions with PSKc
    uint32_t mMgmtActiveGets;                ///< The number of MGMT_ACTIVE_GET.req sent over secure sessions
    uint32_t mMgmtPendingGets;               ///< The number of MGMT_PENDING_GET.req sent over secure sessions
} otBorderAgentCounters;

/**
 * Represents information about a Border Agent session.
 *
 * This structure is populated by `otBorderAgentGetNextSessionInfo()` during iteration over the list of sessions using
 * an `otBorderAgentSessionIterator`.
 *
 * To ensure consistent `mLifetime` calculations, the iterator's initialization time is stored within the iterator,
 * and each session's `mLifetime` is calculated relative to this time.
 */
typedef struct otBorderAgentSessionInfo
{
    otSockAddr mPeerSockAddr;   ///< Socket address (IPv6 address and port number) of session peer.
    bool       mIsConnected;    ///< Indicates whether the session is connected.
    bool       mIsCommissioner; ///< Indicates whether the session is accepted as full commissioner.
    uint64_t   mLifetime;       ///< Milliseconds since the session was first established.
} otBorderAgentSessionInfo;

/**
 * Represents an iterator for Border Agent sessions.
 *
 * The caller MUST NOT access or update the fields in this struct. It is intended for OpenThread internal use only.
 */
typedef struct otBorderAgentSessionIterator
{
    void    *mPtr;
    uint64_t mData;
} otBorderAgentSessionIterator;

/**
 * Maximum length of the OT core generated MeshCoP Service TXT data.
 *
 * Each field has the format:
 * | Length (1 byte) | "Keyname=" | Data |
 *
 * Fields:
 * Border Agent Id (id)                 - 4 + 16 = 20 bytes
 * Network Name (nn)                    - 4 + 16 = 20 bytes
 * ExtendedPanId (xp)                   - 4 + 8 = 12 bytes
 * THread Version (tv)                  - 4 + 5 = 9 bytes
 * Extended Address (xa)                - 4 + 8 = 12 bytes
 * State Bitmap (sb)                    - 4 + 4 = 8 bytes
 * Partition Id (pt)                    - 4 + 4 = 8 bytes
 * Active Timestamp (at)                - 4 + 8 = 12 bytes
 * Backbone Router Sequence Number (sq) - 4 + 1 = 5 bytes
 * Backbone Router Udp Port (bb)        - 4 + 2 = 6 bytes
 * Backbone Router Domain Name (dn)     - 4 + 16 = 20 bytes
 * On-Mesh Routable Prefix (omr)        - 5 + 9 = 14 bytes
 *
 * Maximum possible data length: 146 bytes
 */
#define OT_BORDER_AGENT_MESHCOP_SERVICE_TXT_DATA_MAX_LENGTH 256

/**
 * Represents the Border Agent MeshCoP Service TXT data.
 */
typedef struct otBorderAgentMeshCoPServiceTxtData
{
    uint8_t  mData[OT_BORDER_AGENT_MESHCOP_SERVICE_TXT_DATA_MAX_LENGTH];
    uint16_t mLength;
} otBorderAgentMeshCoPServiceTxtData;

/**
 * Enables or disables the Border Agent service on the device.
 *
 * By default, the Border Agent service is enabled when the `OPENTHREAD_CONFIG_BORDER_AGENT_ENABLE` feature is used.
 * This function allows higher-layer code to explicitly control its state. This can be useful in scenarios such as:
 *
 * - The higher-layer code wishes to delay the start of the Border Agent service (and its mDNS advertisement of the
 *  `_meshcop._udp` service on the infrastructure link). This allows time to prepare or determine vendor-specific TXT
 *   data entries for inclusion.
 * - Unit tests or test scripts might disable the Border Agent service to prevent it from interfering with specific
 *   test steps. For example, tests validating mDNS or DNS-SD functionality may disable the Border Agent to prevent its
 *   registration of the MeshCoP service.
 *
 * @param[in] aInstance  The OpenThread instance.
 * @param[in] aEnabled   A boolean to indicate whether to to enable (TRUE), or disable (FALSE).
 */
void otBorderAgentSetEnabled(otInstance *aInstance, bool aEnabled);

/**
 * Indicates whether or not the Border Agent service is enabled.
 *
 * @param[in] aInstance  The OpenThread instance.
 *
 * @retval TRUE   The Border Agent service is enabled.
 * @retval FALSE  The Border Agent service is disabled.
 */
bool otBorderAgentIsEnabled(otInstance *aInstance);

/**
 * Indicates whether or not the Border Agent service is enabled and also active.
 *
 * While the Border Agent is active, external commissioner candidates can try to connect to and establish secure DTLS
 * sessions with the Border Agent using PSKc. A connected commissioner can then petition to become a full commissioner.
 *
 * If `OPENTHREAD_CONFIG_BORDER_AGENT_EPHEMERAL_KEY_ENABLE` is enabled, independent and separate DTLS transport and
 * sessions are used for the ephemeral key. Therefore, the ephemeral key and Border Agent service can be enabled and
 * used in parallel.
 *
 * @param[in]  aInstance  A pointer to an OpenThread instance.
 *
 * @retval TRUE   The Border Agent is active.
 * @retval FALSE  The Border Agent is not active.
 */
bool otBorderAgentIsActive(otInstance *aInstance);

/**
 * Gets the UDP port of the Thread Border Agent service.
 *
 * @param[in]  aInstance  A pointer to an OpenThread instance.
 *
 * @returns UDP port of the Border Agent.
 */
uint16_t otBorderAgentGetUdpPort(otInstance *aInstance);

/**
 * This callback informs the application of the changes in the state of the MeshCoP service.
 *
 * In specific, the 'state' includes the MeshCoP TXT data originated from the Thread network and whether the
 * Border Agent is Active (which can be obtained by `otBorderAgentIsActive`).
 *
 * @param[in] aContext  A pointer to application-specific context.
 */
typedef void (*otBorderAgentMeshCoPServiceChangedCallback)(void *aContext);

/**
 * Sets the callback function used by the Border Agent to notify of any changes to the state of the MeshCoP service.
 *
 * The callback is invoked when the 'Is Active' state of the Border Agent or the MeshCoP service TXT data values
 * change. For example, it is invoked when the network name or the extended PAN ID changes and passes the updated
 * encoded TXT data to the application layer.
 *
 * This callback is invoked once right after this API is called to provide initial states of the MeshCoP service.
 *
 * @param[in] aInstance  A pointer to an OpenThread instance.
 * @param[in] aCallback  The callback to be invoked when there are any changes of the MeshCoP service.
 * @param[in] aContext   A pointer to application-specific context.
 */
void otBorderAgentSetMeshCoPServiceChangedCallback(otInstance                                *aInstance,
                                                   otBorderAgentMeshCoPServiceChangedCallback aCallback,
                                                   void                                      *aContext);

/**
 * Gets the MeshCoP service TXT data.
 *
 * @param[in]  aInstance  A pointer to an OpenThread instance.
 * @param[out] aTxtData   A pointer to a MeshCoP Service TXT data struct to get the data.
 *
 * @retval OT_ERROR_NONE      If successfully retrieved the Border Agent MeshCoP Service TXT data.
 * @retval OT_ERROR_NO_BUFS   If the buffer in @p aTxtData doesn't have enough size.
 */
otError otBorderAgentGetMeshCoPServiceTxtData(otInstance *aInstance, otBorderAgentMeshCoPServiceTxtData *aTxtData);

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
 * Initializes a session iterator.
 *
 * An iterator MUST be initialized before being used in `otBorderAgentGetNextSessionInfo()`. A previously initialized
 * iterator can be re-initialized to start from the beginning of the session list.
 *
 * @param[in] aInstance   A pointer to an OpenThread instance.
 * @param[in] aIterator   The iterator to initialize.
 */
void otBorderAgentInitSessionIterator(otInstance *aInstance, otBorderAgentSessionIterator *aIterator);

/**
 * Retrieves the next Border Agent session information.
 *
 * @param[in]  aIterator      The iterator to use.
 * @param[out] aSessionInfo   A pointer to an `otBorderAgentSessionInfo` to populate.
 *
 * @retval OT_ERROR_NONE        Successfully retrieved the next session info.
 * @retval OT_ERROR_NOT_FOUND   No more sessions are available. The end of the list has been reached.
 */
otError otBorderAgentGetNextSessionInfo(otBorderAgentSessionIterator *aIterator,
                                        otBorderAgentSessionInfo     *aSessionInfo);

/**
 * Gets the counters of the Thread Border Agent.
 *
 * @param[in]  aInstance  A pointer to an OpenThread instance.
 *
 * @returns A pointer to the Border Agent counters.
 */
const otBorderAgentCounters *otBorderAgentGetCounters(otInstance *aInstance);

/*--------------------------------------------------------------------------------------------------------------------
 * Border Agent Ephemeral Key feature */

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
 * @}
 */

#ifdef __cplusplus
} // end of extern "C"
#endif

#endif // OPENTHREAD_BORDER_AGENT_H_
