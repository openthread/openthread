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
 *   This file includes functions for the Thread Border Agent Admitter.
 */

#ifndef OPENTHREAD_BORDER_AGENT_ADMITTER_H_
#define OPENTHREAD_BORDER_AGENT_ADMITTER_H_

#include <stdbool.h>
#include <stdint.h>

#include <openthread/border_agent.h>
#include <openthread/error.h>
#include <openthread/instance.h>
#include <openthread/ip6.h>
#include <openthread/steering_data.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @addtogroup api-border-agent
 *
 * @brief
 *   This module includes functions for the Thread Border Agent Admitter role.
 *
 *   All APIs in this module require both `OPENTHREAD_CONFIG_BORDER_AGENT_ENABLE` and
 *   `OPENTHREAD_CONFIG_BORDER_AGENT_ADMITTER_ENABLE` features to be enabled.
 *
 * @{
 */

/**
 * Represents an iterator for Border Admitter enroller.
 *
 * The caller MUST NOT access or update the fields in this struct. It is intended for OpenThread internal use only.
 */
typedef struct otBorderAdmitterIterator
{
    void    *mPtr1;
    void    *mPtr2;
    uint64_t mData1;
    uint32_t mData2;
} otBorderAdmitterIterator;

/**
 * Represents information about an enroller.
 *
 * To ensure consistent `mRegisterDuration` calculations, the iterator's initialization time is stored within the
 * iterator, and each enroller `mRegisterDuration` is calculated relative to this time.
 */
typedef struct otBorderAdmitterEnrollerInfo
{
    otBorderAgentSessionInfo mSessionInfo;      ///< The session information.
    const char              *mId;               ///< The enroller ID string.
    otSteeringData           mSteeringData;     ///< The steering data.
    uint8_t                  mMode;             ///< The enroller's mode.
    uint64_t                 mRegisterDuration; ///< Milliseconds since the enroller registered.
} otBorderAdmitterEnrollerInfo;

/**
 * Represents information about a joiner accepted by an enroller.
 *
 * To ensure consistent duration calculations, the iterator's initialization time is stored within the iterator, and
 * the `mMsecSinceAccept` is calculated relative to this time.
 */
typedef struct otBorderAdmitterJoinerInfo
{
    otIp6InterfaceIdentifier mIid;                ///< Joiner IID.
    uint64_t                 mMsecSinceAccept;    ///< Milliseconds since the joiner was accepted by the enroller.
    uint32_t                 mMsecTillExpiration; ///< Milliseconds till the joiner will be expired and removed.
} otBorderAdmitterJoinerInfo;

/**
 * Enables or disables the Border Agent Admitter.
 *
 * The default enable/disable state of Border Admitter (after OpenThread stack initialization) is determined by the
 * OpenThread config `OPENTHREAD_CONFIG_BORDER_AGENT_ADMITTER_ENABLED_BY_DEFAULT`.
 *
 * @param[in] aInstance  The OpenThread instance.
 * @param[in] aEnabled   A boolean to indicate whether to enable (TRUE) or disable (FALSE) the Border Agent Admitter.
 */
void otBorderAdmitterSetEnabled(otInstance *aInstance, bool aEnable);

/**
 * Indicates whether the Border Agent Admitter is enabled.
 *
 * @param[in] aInstance  The OpenThread instance.
 *
 * @retval TRUE   The Border Agent Admitter is enabled.
 * @retval FALSE  The Border Agent Admitter is disabled.
 */
bool otBorderAdmitterIsEnabled(otInstance *aInstance);

/**
 * Indicates whether the device is currently the Prime Admitter.
 *
 * The Prime Admitter is the device that wins the election among all Admitters within the Thread mesh network. The
 * election algorithm ensures convergence on a single Prime Admitter within the mesh.
 *
 * @param[in] aInstance  The OpenThread instance.
 *
 * @retval TRUE   This device is the Prime Admitter.
 * @retval FALSE  This device is not the Prime Admitter.
 */
bool otBorderAdmitterIsPrimeAdmitter(otInstance *aInstance);

/**
 * Indicates whether the Prime Admitter is currently the active commissioner.
 *
 * After becoming the Prime Admitter and having at least one enroller register, the Admitter petitions the Leader to
 * be granted the commissioner role.
 *
 * @param[in] aInstance  The OpenThread instance.
 *
 * @retval TRUE   This device is the active commissioner.
 * @retval FALSE  This device is not the active commissioner.
 */
bool otBorderAdmitterIsActiveCommissioner(otInstance *aInstance);

/**
 * Indicates whether the Prime Admitter's petition to become the native mesh commissioner was rejected.
 *
 * A rejection typically occurs if there is already another active commissioner in the Thread network.
 *
 * The Admitter will automatically retry petitioning. It monitors the Thread Network Data to see when the other
 * commissioner is removed and retry its own petition.
 *
 * @param[in] aInstance  The OpenThread instance.
 *
 * @retval TRUE   The petition was rejected.
 * @retval FALSE  The petition was not rejected.
 */
bool otBorderAdmitterIsPetitionRejected(otInstance *aInstance);

/**
 * Gets the Joiner UDP port.
 *
 * A zero value indicates the Joiner UDP port is not specified/fixed by the Admitter (Joiner Routers can pick).
 *
 * @param[in] aInstance  The OpenThread instance.
 *
 * @returns The joiner UDP port number.
 */
uint16_t otBorderAdmitterGetJoinerUdpPort(otInstance *aInstance);

/**
 * Sets the joiner UDP port.
 *
 * A zero value indicates the Joiner UDP port is not specified/fixed by the Admitter (Joiner Routers can pick).
 *
 * @param[in] aInstance  The OpenThread instance.
 * @param[in] aUdpPort   The joiner UDP port number.
 */
void otBorderAdmitterSetJoinerUdpPort(otInstance *aInstance, uint16_t aUdpPort);

/**
 * Initializes an `otBorderAdmitterIterator`.
 *
 * An iterator MUST be initialized before it is used.
 *
 * An iterator can be initialized again to restart from the beginning of the list.
 *
 * When iterating over enrollers, the initialization time is recorded and used to calculate a consistent
 * `mRegisterDuration` for each enroller.
 *
 * @param[in] aInstance  A pointer to an OpenThread instance.
 * @param[in] aIterator  A pointer to the iterator to initialize.
 *
 */
void otBorderAdmitterInitIterator(otInstance *aInstance, otBorderAdmitterIterator *aIterator);

/**
 * Retrieves the information about the next Enroller registered with the Admitter.
 *
 * @param[in]  aIterator        The iterator to use.
 * @param[out] aEnrollerInfo    A pointer to an `otBorderAdmitterEnrollerInfo` to populate.
 *
 * @retval OT_ERROR_NONE        Successfully retrieved the next enroller info.
 * @retval OT_ERROR_NOT_FOUND   No more enrollers are available. The end of the list has been reached.
 */
otError otBorderAdmitterGetNextEnrollerInfo(otBorderAdmitterIterator     *aIterator,
                                            otBorderAdmitterEnrollerInfo *aEnrollerInfo);

/**
 * Retrieves the information about the next accepted joiner by the latest retrieved enroller during iteration.
 *
 * Iterates over all joiners which are accepted by the latest enroller, i.e., the last enroller which was retrieved
 * using the @p aIterator along with `otBorderAdmitterGetNextEnrollerInfo()`.
 *
 * @param[in]  aIterator    The iterator to use.
 * @param[out] aJoinerInfo  A pointer to an `otBorderAdmitterJoinerInfo` to populate.
 *
 * @retval OT_ERROR_NONE        Successfully retrieved the next joiner info.
 * @retval OT_ERROR_NOT_FOUND   No more joiners are available. The end of the list has been reached.
 */
otError otBorderAdmitterGetNextJoinerInfo(otBorderAdmitterIterator *aIterator, otBorderAdmitterJoinerInfo *aJoinerInfo);

/**
 * @}
 */

#ifdef __cplusplus
} // end of extern "C"
#endif

#endif // OPENTHREAD_BORDER_AGENT_ADMITTER_H_
