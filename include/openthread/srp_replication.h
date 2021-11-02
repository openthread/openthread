/*
 *  Copyright (c) 2021, The OpenThread Authors.
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
 *  This file defines the OpenThread SRP Replication Protocol (SRPL) APIs.
 */

#ifndef OPENTHREAD_SRP_REPLICATION_H_
#define OPENTHREAD_SRP_REPLICATION_H_

#include <openthread/ip6.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @addtogroup api-srp
 *
 * @brief
 *   This module includes functions that control SRP Replication (SRPL).
 *
 * @{
 *
 */

/**
 * This enumeration specifies the states of an SRPL session with a partner.
 *
 */
typedef enum
{
    OT_SRP_REPLICATION_SESSION_DISCONNECTED,      ///< Disconnected.
    OT_SRP_REPLICATION_SESSION_CONNECTING,        ///< Establishing connection.
    OT_SRP_REPLICATION_SESSION_ESTABLISHING,      ///< Establishing SRPL session.
    OT_SRP_REPLICATION_SESSION_INITIAL_SYNC,      ///< Initial SRPL synchronization.
    OT_SRP_REPLICATION_SESSION_ROUTINE_OPERATION, ///< Routine operation (initial sync is finished).
    OT_SRP_REPLICATION_SESSION_ERRORED,           ///< Session errored earlier.
} otSrpReplicationSessionState;

/**
 * This structure represents an SRPL partner info.
 *
 */
typedef struct otSrpReplicationPartner
{
    otSockAddr                   mSockAddr;     ///< Socket address of partner.
    bool                         mHasId;        ///< Whether or not the partner ID is known.
    uint32_t                     mId;           ///< Partner's ID (valid only if `mHasId == true`).
    otSrpReplicationSessionState mSessionState; ///< SRPL session state.
} otSrpReplicationPartner;

/**
 * This structure represents an iterator to iterate over SRPL partner list.
 *
 */
typedef struct otSrpReplicationPartnerIterator
{
    const void *mData; ///< Opaque data (for use by OpenThread core only).
} otSrpReplicationPartnerIterator;

/**
 * This function enables/disables the SRP Replication (SRPL).
 *
 * SRP replication when enabled will manage the SRP server and decide when to enable it. So the SRP server MUST not
 * be enabled directly when SRP replication is being used. SRPL also sets the SRP server address mode to anycast
 * model (see `otSrpServerGetAddressMode()` and `OT_SRP_SERVER_ADDRESS_MODE_ANYCAST`).
 *
 * @param[in] aInstance            The OpenThread instance.
 * @param[in] aEnable              A boolean to enable/disable SRPL.
 *
 * @retval OT_ERROR_NONE           Successfully enabled/disabled SRPL.
 * @retval OT_ERROR_INVALID_STATE  Failed to enable SRP Replication since SRP server is already enabled.
 *
 */
otError otSrpReplicationSetEnabled(otInstance *aInstance, bool aEnable);

/**
 * This function indicates whether or not SRP Replication (SRPL) is enabled.
 *
 * @param[in] aInstance  The OpenThread instance.
 *
 * @retval TRUE   If SRPL is enabled.
 * @retval FALSE  If SRPL is disabled.
 *
 */
bool otSrpReplicationIsEnabled(otInstance *aInstance);

/**
 * This function sets the domain name and the join behavior (accept any domain, or require an exact match).
 *
 * This function can be called only when SRPL is disabled, otherwise `OT_ERROR_INVALID_STATE` is returned.
 *
 * If @p aName is not `NULL`, then SRPL will only accept and join peers with the same domain name and includes
 * @p aName as the domain when advertising "_srpl-tls._tcp" service using DNS-SD.
 *
 * If @p aName is `NULL` then SRPL will accept any joinable domain, i.e., it will adopt the domain name of the first
 * joinable SRPL peer it discovers while performing DNS-SD browse for "_srpl-tls._tcp" service. If SRPL does not
 * discover any peer to adopt its domain name (e.g., it is first/only SRPL entity) it starts advertising using the
 * default domain name from `otSrpReplicationGetDefaultDomain()` function.
 *
 * @param[in] aInstance  The OpenThread instance.
 * @param[in] aName      The domain name. Can be `NULL` to adopt any domain from joinable SRPL peers.

 * @retval OT_ERROR_NONE           Successfully set the domain name.
 * @retval OT_ERROR_INVALID_STATE  SRPL is enabled and therefore domain name cannot be set.
 * @retval OT_ERROR_NO_BUFS        Failed to allocate buffer to save the domain name.
 *
 */
otError otSrpReplicationSetDomain(otInstance *aInstance, const char *aName);

/**
 * This function gets the current domain name.
 *
 * @param[in] aInstance  The OpenThread instance.
 *
 * @returns The domain name string, or `NULL` if no domain.
 *
 */
const char *otSrpReplicationGetDomain(otInstance *aInstance);

/**
 * This function sets the default domain name.
 *
 * This function can be called only when SRPL is disabled, otherwise `OT_ERROR_INVALID_STATE` is returned.
 *
 * The default domain name is only used when `otSrpReplicationGetDomain() == NULL` and if SRPL does not discover any
 * suitable peer to adopt their domain name (during initial domain discovery phase).
 *
 * @param[in] aInstance  The OpenThread instance.
 * @param[in] aName      The default domain name. MUST NOT be `NULL`.
 *
 * @retval OT_ERROR_NONE            Successfully set the default domain name.
 * @retval OT_ERROR_INVALID_STATE   SRPL is enabled and therefore default domain cannot be set.
 * @retval OT_ERROR_NO_BUFS         Failed to allocate buffer to save the domain name.
 *
 */
otError otSrpReplicationSetDefaultDomain(otInstance *aInstance, const char *aName);

/**
 * This function gets the default domain name.
 *
 * @param[in] aInstance  The OpenThread instance.
 *
 * @returns The domain name string.
 *
 */
const char *otSrpReplicationGetDefaultDomain(otInstance *aInstance);

/**
 * This function gets the peer ID assigned to the SRPL itself (if any).
 *
 * @param[in]  aInstance  The OpenThread instance.
 * @param[out] aId        A pointer to a `uint32_t` to return the ID.
 *
 * @retval OT_ERROR_NONE        Successfully set @p aId.
 * @retval OT_ERROR_NOT_FOUND   SRPL does not yet have any assigned ID.
 *
 */
otError otSrpReplicationGetId(otInstance *aInstance, uint32_t *aId);

/**
 * This function initializes a partner iterator.
 *
 * @param[in] aIterator   The iterator to initialize.
 *
 */
void otSrpReplicationInitPartnerIterator(otSrpReplicationPartnerIterator *aIterator);

/**
 * This function iterates over the SRPL partners using an iterator and retrieves the info for the next partner in the
 * list.
 *
 * @param[in]  aInstance   The OpenThread instance.
 * @param[in]  aIterator   A pointer to the iterator. It MUST be already initialized.
 * @param[out] aPartnter   A pointer to an `otSrpReplicationPartner` struct to return the partner info.
 *
 * @retval OT_ERROR_NONE       Retrieved the next partner info from the list. @p aPartner is updated.
 * @retval OT_ERROR_NOT_FOUND  No more partner in the list.
 *
 */
otError otSrpReplicationGetNextPartner(otInstance *                     aInstance,
                                       otSrpReplicationPartnerIterator *aIterator,
                                       otSrpReplicationPartner *        aPartner);

/**
 * @}
 *
 */

#ifdef __cplusplus
} // extern "C"
#endif

#endif // OPENTHREAD_SRP_REPLICATION_H_
