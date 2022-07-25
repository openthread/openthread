/*
 *  Copyright (c) 2021-22, The OpenThread Authors.
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
 *  This file defines the OpenThread Border Routing Manager API.
 */

#ifndef OPENTHREAD_BORDER_ROUTING_H_
#define OPENTHREAD_BORDER_ROUTING_H_

#include <openthread/error.h>
#include <openthread/ip6.h>
#include <openthread/netdata.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @addtogroup api-border-routing
 *
 * @brief
 *  This module includes definitions related to Border Routing Manager.
 *
 *
 * @{
 *
 * All the functions in this module require `OPENTHREAD_CONFIG_BORDER_ROUTING_ENABLE` to be enabled.
 *
 * Border Routing Manager handles bi-directional routing between Thread network and adjacent infrastructure link (AIL).
 *
 * It emits ICMRv6 ND Router Advertisement (RA) messages on AIL to advertise on-link and route prefixes. It also
 * processes received RA messages from infrastructure and mirrors the discovered prefixes on the Thread Network Data to
 * ensure devices on Thread mesh can reach AIL through the Border Router.
 *
 * Routing Manager manages the Off-Mesh Routable (OMR) prefix on the Thread Network data which configures Thread
 * devices with a suitable Off-Mesh Routable IPv6 address. It announces the reachability of this prefix on AIL by
 * including it in the emitted RA messages as an IPv6 Route Information Option (RIO).
 *
 * Routing Manager also monitors and adds on-link prefix on the infrastructure network. If a router on AIL is already
 * providing RA messages containing an IPv6 Prefix Information Option (PIO) that enables IPv6 devices on the link to
 * self-configure their own routable unicast IPv6 address, this address can be used by Thread devices to reach AIL. If
 * Border Router finds no such RA message on AIL, it generates a ULA on-link prefix which it then advertises on AIL in
 * the emitted RA messages.
 *
 */

/**
 * This structure represents an iterator to iterate through the Border Router's discovered prefix table.
 *
 * The fields in this type are opaque (intended for use by OpenThread core only) and therefore should not be
 * accessed or used by caller.
 *
 * Before using an iterator, it MUST be initialized using `otBorderRoutingPrefixTableInitIterator()`.
 *
 */
typedef struct otBorderRoutingPrefixTableIterator
{
    const void *mPtr1;
    const void *mPtr2;
    uint32_t    mData32;
} otBorderRoutingPrefixTableIterator;

/**
 * This structure represents an entry from the discovered prefix table.
 *
 * The entries in the discovered table track the Prefix/Route Info Options in the received Router Advertisement messages
 * from other routers on infrastructure link.
 *
 */
typedef struct otBorderRoutingPrefixTableEntry
{
    otIp6Address      mRouterAddress;       ///< IPv6 address of the router.
    otIp6Prefix       mPrefix;              ///< The discovered IPv6 prefix.
    bool              mIsOnLink;            ///< Indicates whether the prefix is on-link or route prefix.
    uint32_t          mMsecSinceLastUpdate; ///< Milliseconds since last update of this prefix.
    uint32_t          mValidLifetime;       ///< Valid lifetime of the prefix (in seconds).
    otRoutePreference mRoutePreference;     ///< Route preference when `mIsOnlink` is false.
    uint32_t          mPreferredLifetime;   ///< Preferred lifetime of the on-link prefix when `mIsOnLink` is true.
} otBorderRoutingPrefixTableEntry;

/**
 * This method initializes the Border Routing Manager on given infrastructure interface.
 *
 * @note  This method MUST be called before any other otBorderRouting* APIs.
 *
 * @param[in]  aInstance          A pointer to an OpenThread instance.
 * @param[in]  aInfraIfIndex      The infrastructure interface index.
 * @param[in]  aInfraIfIsRunning  A boolean that indicates whether the infrastructure
 *                                interface is running.
 *
 * @retval  OT_ERROR_NONE           Successfully started the Border Routing Manager on given infrastructure.
 * @retval  OT_ERROR_INVALID_STATE  The Border Routing Manager has already been initialized.
 * @retval  OT_ERROR_INVALID_ARGS   The index of the infrastructure interface is not valid.
 * @retval  OT_ERROR_FAILED         Internal failure. Usually due to failure in generating random prefixes.
 *
 * @sa otPlatInfraIfStateChanged.
 *
 */
otError otBorderRoutingInit(otInstance *aInstance, uint32_t aInfraIfIndex, bool aInfraIfIsRunning);

/**
 * Enables or disables the Border Routing Manager.
 *
 * @note  The Border Routing Manager is disabled by default.
 *
 * @param[in]  aInstance  A pointer to an OpenThread instance.
 * @param[in]  aEnabled   A boolean to enable/disable the routing manager.
 *
 * @retval  OT_ERROR_INVALID_STATE  The Border Routing Manager is not initialized yet.
 * @retval  OT_ERROR_NONE           Successfully enabled/disabled the Border Routing Manager.
 *
 */
otError otBorderRoutingSetEnabled(otInstance *aInstance, bool aEnabled);

/**
 * This function gets the preference used when advertising Route Info Options (e.g., for discovered OMR prefixes) in
 * Router Advertisement messages sent over the infrastructure link.
 *
 * @param[in] aInstance A pointer to an OpenThread instance.
 *
 * @returns The OMR prefix advertisement preference.
 *
 */
otRoutePreference otBorderRoutingGetRouteInfoOptionPreference(otInstance *aInstance);

/**
 * This function sets the preference to use when advertising Route Info Options in Router Advertisement messages sent
 * over the infrastructure link, for example for discovered OMR prefixes.
 *
 * By default BR will use `medium` preference level, but this function allows the default value to be changed. As an
 * example, it can be set to `low` preference in the case where device is a temporary BR (a mobile BR or a
 * battery-powered BR) to indicate that other BRs (if any) should be preferred over this BR on the infrastructure link.
 *
 * @param[in] aInstance     A pointer to an OpenThread instance.
 * @param[in] aPreference   The route preference to use.
 *
 */
void otBorderRoutingSetRouteInfoOptionPreference(otInstance *aInstance, otRoutePreference aPreference);

/**
 * Gets the local Off-Mesh-Routable (OMR) Prefix, for example `fdfc:1ff5:1512:5622::/64`.
 *
 * An OMR Prefix is a randomly generated 64-bit prefix that's published in the
 * Thread network if there isn't already an OMR prefix. This prefix can be reached
 * from the local Wi-Fi or Ethernet network.
 *
 * @param[in]   aInstance  A pointer to an OpenThread instance.
 * @param[out]  aPrefix    A pointer to where the prefix will be output to.
 *
 * @retval  OT_ERROR_INVALID_STATE  The Border Routing Manager is not initialized yet.
 * @retval  OT_ERROR_NONE           Successfully retrieved the OMR prefix.
 *
 */
otError otBorderRoutingGetOmrPrefix(otInstance *aInstance, otIp6Prefix *aPrefix);

/**
 * Gets the currently favored Off-Mesh-Routable (OMR) Prefix.
 *
 * The favored OMR prefix can be discovered from Network Data or can be this device's local OMR prefix.
 *
 * @param[in]   aInstance    A pointer to an OpenThread instance.
 * @param[out]  aPrefix      A pointer to output the favored OMR prefix.
 * @param[out]  aPreference  A pointer to output the preference associated the favored prefix.
 *
 * @retval  OT_ERROR_INVALID_STATE  The Border Routing Manager is not initialized yet.
 * @retval  OT_ERROR_NONE           Successfully retrieved the favored OMR prefix.
 *
 */
otError otBorderRoutingGetFavoredOmrPrefix(otInstance *aInstance, otIp6Prefix *aPrefix, otRoutePreference *aPreference);

/**
 * Gets the On-Link Prefix for the adjacent infrastructure link, for example `fd41:2650:a6f5:0::/64`.
 *
 * An On-Link Prefix is a 64-bit prefix that's advertised on the infrastructure link if there isn't already a usable
 * on-link prefix being advertised on the link.
 *
 * @param[in]   aInstance  A pointer to an OpenThread instance.
 * @param[out]  aPrefix    A pointer to where the prefix will be output to.
 *
 * @retval  OT_ERROR_INVALID_STATE  The Border Routing Manager is not initialized yet.
 * @retval  OT_ERROR_NONE           Successfully retrieved the on-link prefix.
 *
 */
otError otBorderRoutingGetOnLinkPrefix(otInstance *aInstance, otIp6Prefix *aPrefix);

/**
 * Gets the local NAT64 Prefix of the Border Router.
 *
 * NAT64 Prefix might not be advertised in the Thread network.
 *
 * `OPENTHREAD_CONFIG_BORDER_ROUTING_NAT64_ENABLE` must be enabled.
 *
 * @param[in]   aInstance   A pointer to an OpenThread instance.
 * @param[out]  aPrefix     A pointer to where the prefix will be output to.
 *
 * @retval  OT_ERROR_INVALID_STATE  The Border Routing Manager is not initialized yet.
 * @retval  OT_ERROR_NONE           Successfully retrieved the NAT64 prefix.
 *
 */
otError otBorderRoutingGetNat64Prefix(otInstance *aInstance, otIp6Prefix *aPrefix);

/**
 * This function initializes an `otBorderRoutingPrefixTableIterator`.
 *
 * An iterator MUST be initialized before it is used.
 *
 * An iterator can be initialized again to restart from the beginning of the table.
 *
 * When iterating over entries in the table, to ensure the update times `mMsecSinceLastUpdate` of entries are
 * consistent, they are given relative to the time the iterator was initialized.
 *
 * @param[in]  aInstance  The OpenThread instance.
 * @param[out] aIterator  A pointer to the iterator to initialize.
 *
 */
void otBorderRoutingPrefixTableInitIterator(otInstance *aInstance, otBorderRoutingPrefixTableIterator *aIterator);

/**
 * This function iterates over the entries in the Border Router's discovered prefix table.
 *
 * @param[in]     aInstance    The OpenThread instance.
 * @param[in,out] aIterator    A pointer to the iterator.
 * @param[out]    aEntry       A pointer to the entry to populate.
 *
 * @retval OT_ERROR_NONE        Iterated to the next entry, @p aEntry and @p aIterator are updated.
 * @retval OT_ERROR_NOT_FOUND   No more entries in the table.
 *
 */
otError otBorderRoutingGetNextPrefixTableEntry(otInstance *                        aInstance,
                                               otBorderRoutingPrefixTableIterator *aIterator,
                                               otBorderRoutingPrefixTableEntry *   aEntry);

/**
 * @}
 *
 */

#ifdef __cplusplus
} // extern "C"
#endif

#endif // OPENTHREAD_BORDER_ROUTING_H_
