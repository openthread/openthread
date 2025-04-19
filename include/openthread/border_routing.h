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
 */

/**
 * Represents an iterator to iterate through the Border Router's discovered prefix table.
 *
 * The fields in this type are opaque (intended for use by OpenThread core only) and therefore should not be
 * accessed or used by caller.
 *
 * Before using an iterator, it MUST be initialized using `otBorderRoutingPrefixTableInitIterator()`.
 */
typedef struct otBorderRoutingPrefixTableIterator
{
    const void *mPtr1;
    const void *mPtr2;
    uint32_t    mData0;
    uint32_t    mData1;
    uint8_t     mData2;
    uint8_t     mData3;
} otBorderRoutingPrefixTableIterator;

/**
 * Represents a discovered router on the infrastructure link.
 *
 * The `mIsPeerBr` field requires `OPENTHREAD_CONFIG_BORDER_ROUTING_TRACK_PEER_BR_INFO_ENABLE`. Routing Manager
 * determines whether the router is a peer BR (connected to the same Thread mesh network) by comparing its advertised
 * PIO/RIO prefixes with the entries in the Thread Network Data. While this method is generally effective, it may not
 * be 100% accurate in all scenarios, so the `mIsPeerBr` flag should be used with caution.
 */
typedef struct otBorderRoutingRouterEntry
{
    otIp6Address mAddress;                      ///< IPv6 address of the router.
    uint32_t     mMsecSinceLastUpdate;          ///< Milliseconds since last update (any message rx) from this router.
    uint32_t     mAge;                          ///< The router's age in seconds (duration since its first discovery).
    bool         mManagedAddressConfigFlag : 1; ///< The router's Managed Address Config flag (`M` flag).
    bool         mOtherConfigFlag : 1;          ///< The router's Other Config flag (`O` flag).
    bool         mSnacRouterFlag : 1;           ///< The router's SNAC Router flag (`S` flag).
    bool         mIsLocalDevice : 1;            ///< This router is the local device (this BR).
    bool         mIsReachable : 1;              ///< This router is reachable.
    bool         mIsPeerBr : 1;                 ///< This router is (likely) a peer BR.
} otBorderRoutingRouterEntry;

/**
 * Represents an entry from the discovered prefix table.
 *
 * The entries in the discovered table track the Prefix/Route Info Options in the received Router Advertisement messages
 * from other routers on the infrastructure link.
 */
typedef struct otBorderRoutingPrefixTableEntry
{
    otBorderRoutingRouterEntry mRouter;              ///< Information about the router advertising this prefix.
    otIp6Prefix                mPrefix;              ///< The discovered IPv6 prefix.
    bool                       mIsOnLink;            ///< Indicates whether the prefix is on-link or route prefix.
    uint32_t                   mMsecSinceLastUpdate; ///< Milliseconds since last update of this prefix.
    uint32_t                   mValidLifetime;       ///< Valid lifetime of the prefix (in seconds).
    otRoutePreference          mRoutePreference;     ///< Route preference when `mIsOnlink` is false.
    uint32_t                   mPreferredLifetime;   ///< Preferred lifetime of the on-link prefix when `mIsOnLink`.
} otBorderRoutingPrefixTableEntry;

/**
 * Represents a discovered Recursive DNS Server (RDNSS) address entry.
 *
 * Address entries are discovered by processing the RDNSS options within received Router Advertisement messages from
 * routers on infrastructure link.
 */
typedef struct otBorderRoutingRdnssAddrEntry
{
    otBorderRoutingRouterEntry mRouter;              ///< Information about the router advertising this address.
    otIp6Address               mAddress;             ///< The DNS Server IPv6 address.
    uint32_t                   mMsecSinceLastUpdate; ///< Milliseconds since last update of this address.
    uint32_t                   mLifetime;            ///< Lifetime of the address (in seconds).
} otBorderRoutingRdnssAddrEntry;

/**
 * Represents information about a peer Border Router found in the Network Data.
 */
typedef struct otBorderRoutingPeerBorderRouterEntry
{
    uint16_t mRloc16; ///< The RLOC16 of BR.
    uint32_t mAge;    ///< Seconds since the BR appeared in the Network Data.
} otBorderRoutingPeerBorderRouterEntry;

/**
 * Represents a group of data of platform-generated RA messages processed.
 */
typedef struct otPdProcessedRaInfo
{
    uint32_t mNumPlatformRaReceived;   ///< The number of platform generated RA handled by ProcessPlatformGeneratedRa.
    uint32_t mNumPlatformPioProcessed; ///< The number of PIO processed for adding OMR prefixes.
    uint32_t mLastPlatformRaMsec;      ///< The timestamp of last processed RA message.
} otPdProcessedRaInfo;

/**
 * Represents the configuration options related to the OMR prefix.
 *
 * This is used in `otBorderRoutingSetOmrConfig()` to offer manual administration options to explicitly configure
 * the OMR prefix or to disable it.
 */
typedef enum
{
    OT_BORDER_ROUTING_OMR_CONFIG_AUTO,     ///< BR auto-generates the local OMR prefix.
    OT_BORDER_ROUTING_OMR_CONFIG_CUSTOM,   ///< BR uses a given custom OMR prefix.
    OT_BORDER_ROUTING_OMR_CONFIG_DISABLED, ///< BR does not add local/PD OMR prefix in Network Data.
} otBorderRoutingOmrConfig;

/**
 * Represents the state of Border Routing Manager.
 */
typedef enum
{
    OT_BORDER_ROUTING_STATE_UNINITIALIZED, ///< Routing Manager is uninitialized.
    OT_BORDER_ROUTING_STATE_DISABLED,      ///< Routing Manager is initialized but disabled.
    OT_BORDER_ROUTING_STATE_STOPPED,       ///< Routing Manager in initialized and enabled but currently stopped.
    OT_BORDER_ROUTING_STATE_RUNNING,       ///< Routing Manager is initialized, enabled, and running.
} otBorderRoutingState;

/**
 * This enumeration represents the state of DHCPv6 Prefix Delegation State.
 */
typedef enum
{
    OT_BORDER_ROUTING_DHCP6_PD_STATE_DISABLED, ///< DHCPv6 PD is disabled on the border router.
    OT_BORDER_ROUTING_DHCP6_PD_STATE_STOPPED,  ///< DHCPv6 PD in enabled but won't try to request and publish a prefix.
    OT_BORDER_ROUTING_DHCP6_PD_STATE_RUNNING,  ///< DHCPv6 PD is enabled and will try to request and publish a prefix.
    OT_BORDER_ROUTING_DHCP6_PD_STATE_IDLE,     ///< DHCPv6 PD is idle; Higher-prf prefix published by other BRs.
} otBorderRoutingDhcp6PdState;

/**
 * Initializes the Border Routing Manager on given infrastructure interface.
 *
 * @note  This method MUST be called before any other otBorderRouting* APIs.
 * @note  This method can be re-called to change the infrastructure interface, but the Border Routing Manager should be
 *        disabled first, and re-enabled after.
 *
 * @param[in]  aInstance          A pointer to an OpenThread instance.
 * @param[in]  aInfraIfIndex      The infrastructure interface index.
 * @param[in]  aInfraIfIsRunning  A boolean that indicates whether the infrastructure
 *                                interface is running.
 *
 * @retval  OT_ERROR_NONE           Successfully started the Border Routing Manager on given infrastructure.
 * @retval  OT_ERROR_INVALID_STATE  The Border Routing Manager is in a state other than disabled or uninitialized.
 * @retval  OT_ERROR_INVALID_ARGS   The index of the infrastructure interface is not valid.
 * @retval  OT_ERROR_FAILED         Internal failure. Usually due to failure in generating random prefixes.
 *
 * @sa otPlatInfraIfStateChanged.
 * @sa otBorderRoutingSetEnabled.
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
 */
otError otBorderRoutingSetEnabled(otInstance *aInstance, bool aEnabled);

/**
 * Gets the current state of Border Routing Manager.
 *
 * @param[in]  aInstance  A pointer to an OpenThread instance.
 *
 * @returns The current state of Border Routing Manager.
 */
otBorderRoutingState otBorderRoutingGetState(otInstance *aInstance);

/**
 * Configures the OMR prefix handling in the Border Routing Manager.
 *
 * This function offers manual administration options to explicitly configure the OMR prefix or to disable it.
 *
 * By default, `OT_BORDER_ROUTING_OMR_CONFIG_AUTO` is used. In this mode, the Border Routing Manager automatically
 * selects and manages the OMR prefix. This can involve auto-generating a local prefix or utilizing a prefix obtained
 * through DHCPv6 PD (Prefix Delegation), if the feature is enabled.
 *
 * The `OT_BORDER_ROUTING_OMR_CONFIG_CUSTOM` option enables the use of a user-specified OMR prefix. When this option
 * is selected, the @p aOmrPrefix and @p aPreference parameters are used to define the custom OMR prefix and its
 * associated preference. These parameters are ignored for other configuration modes, and @p aOmrPrefix can be `NULL`.
 *
 * The `OT_BORDER_ROUTING_OMR_CONFIG_DISABLED` option disables the Border Routing Manager's management of the OMR
 * prefix. The Routing Manager module itself will not add any local or DHCPv6 PD OMR prefixes to the Network Data.
 *
 * @param[in] aInstance      A pointer to the OpenThread instance.
 * @param[in] aConfig        The desired OMR configuration.
 * @param[in] aOmrPrefix     A pointer to the custom OMR prefix. Required only when @p aConfig is
 *                           `OT_BORDER_ROUTING_OMR_CONFIG_CUSTOM`. Otherwise, it can be `NULL`.
 * @param[in] aPreference    The preference associated with the custom OMR prefix.
 *
 * @retval OT_ERROR_NONE           The OMR configuration was successfully set to @p aConfig.
 * @retval OT_ERROR_INVALID_ARGS   The provided custom OMR prefix (@p aOmrPrefix) is invalid.
 */
otError otBorderRoutingSetOmrConfig(otInstance              *aInstance,
                                    otBorderRoutingOmrConfig aConfig,
                                    const otIp6Prefix       *aOmrPrefix,
                                    otRoutePreference        aPreference);

/**
 * Gets the current OMR prefix configuration mode.
 *
 * This function retrieves the current OMR configuration and, if a custom OMR prefix is configured, the custom prefix
 * and its associated preference.
 *
 * If the caller does not require the custom OMR prefix and preference, the @p aOmrPrefix and @p aPreference parameters
 * can be set to `NULL`.
 *
 * @param[in]  aInstance      A pointer to the OpenThread instance.
 * @param[out] aOmrPrefix     A pointer to an `otIp6Prefix` to return the custom OMR prefix, if the configuration is
 *                            `OT_BORDER_ROUTING_OMR_CONFIG_CUSTOM`.
 * @param[out] aPreference    A pointer to return the preference associated with the custom OMR prefix.
 *
 * @return The current OMR prefix configuration mode.
 */
otBorderRoutingOmrConfig otBorderRoutingGetOmrConfig(otInstance        *aInstance,
                                                     otIp6Prefix       *aOmrPrefix,
                                                     otRoutePreference *aPreference);

/**
 * Gets the current preference used when advertising Route Info Options (RIO) in Router Advertisement
 * messages sent over the infrastructure link.
 *
 * The RIO preference is determined as follows:
 *
 * - If explicitly set by user by calling `otBorderRoutingSetRouteInfoOptionPreference()`, the given preference is
 *   used.
 * - Otherwise, it is determined based on device's current role: Medium preference when in router/leader role and
 *   low preference when in child role.
 *
 * @returns The current Route Info Option preference.
 */
otRoutePreference otBorderRoutingGetRouteInfoOptionPreference(otInstance *aInstance);

/**
 * Explicitly sets the preference to use when advertising Route Info Options (RIO) in Router
 * Advertisement messages sent over the infrastructure link.
 *
 * After a call to this function, BR will use the given preference for all its advertised RIOs. The preference can be
 * cleared by calling `otBorderRoutingClearRouteInfoOptionPreference()`.
 *
 * @param[in] aInstance     A pointer to an OpenThread instance.
 * @param[in] aPreference   The route preference to use.
 */
void otBorderRoutingSetRouteInfoOptionPreference(otInstance *aInstance, otRoutePreference aPreference);

/**
 * Clears a previously set preference value for advertised Route Info Options.
 *
 * After a call to this function, BR will use device's role to determine the RIO preference: Medium preference when
 * in router/leader role and low preference when in child role.
 *
 * @param[in] aInstance     A pointer to an OpenThread instance.
 */
void otBorderRoutingClearRouteInfoOptionPreference(otInstance *aInstance);

/**
 * Sets additional options to append at the end of emitted Router Advertisement (RA) messages.
 *
 * The content of @p aOptions is copied internally, so it can be a temporary buffer (e.g., a stack allocated array).
 *
 * Subsequent calls to this function overwrite the previously set value.
 *
 * @param[in] aOptions   A pointer to the encoded options. Can be `NULL` to clear.
 * @param[in] aLength    Number of bytes in @p aOptions.
 *
 * @retval OT_ERROR_NONE     Successfully set the extra option bytes.
 * @retval OT_ERROR_NO_BUFS  Could not allocate buffer to save the buffer.
 */
otError otBorderRoutingSetExtraRouterAdvertOptions(otInstance *aInstance, const uint8_t *aOptions, uint16_t aLength);

/**
 * Gets the current preference used for published routes in Network Data.
 *
 * The preference is determined as follows:
 *
 * - If explicitly set by user by calling `otBorderRoutingSetRoutePreference()`, the given preference is used.
 * - Otherwise, it is determined automatically by `RoutingManager` based on the device's role and link quality.
 *
 * @param[in] aInstance     A pointer to an OpenThread instance.
 *
 * @returns The current published route preference.
 */
otRoutePreference otBorderRoutingGetRoutePreference(otInstance *aInstance);

/**
 * Explicitly sets the preference of published routes in Network Data.
 *
 * After a call to this function, BR will use the given preference. The preference can be cleared by calling
 * `otBorderRoutingClearRoutePreference()`.
 *
 * @param[in] aInstance     A pointer to an OpenThread instance.
 * @param[in] aPreference   The route preference to use.
 */
void otBorderRoutingSetRoutePreference(otInstance *aInstance, otRoutePreference aPreference);

/**
 * Clears a previously set preference value for published routes in Network Data.
 *
 * After a call to this function, BR will determine the preference automatically based on the device's role and
 * link quality (to the parent when acting as end-device).
 *
 * @param[in] aInstance     A pointer to an OpenThread instance.
 */
void otBorderRoutingClearRoutePreference(otInstance *aInstance);

/**
 * Gets the local Off-Mesh-Routable (OMR) Prefix, for example `fdfc:1ff5:1512:5622::/64`.
 *
 * An OMR Prefix is a randomly generated 64-bit prefix that's published in the
 * Thread network if there isn't already an OMR prefix. This prefix can be reached
 * from the local Wi-Fi or Ethernet network.
 *
 * Note: When DHCPv6 PD is enabled, the border router may publish the prefix from
 * DHCPv6 PD.
 *
 * @param[in]   aInstance  A pointer to an OpenThread instance.
 * @param[out]  aPrefix    A pointer to where the prefix will be output to.
 *
 * @retval  OT_ERROR_INVALID_STATE  The Border Routing Manager is not initialized yet.
 * @retval  OT_ERROR_NONE           Successfully retrieved the OMR prefix.
 *
 * @sa otBorderRoutingGetPdOmrPrefix
 */
otError otBorderRoutingGetOmrPrefix(otInstance *aInstance, otIp6Prefix *aPrefix);

/**
 * Gets the DHCPv6 Prefix Delegation (PD) provided off-mesh-routable (OMR) prefix.
 *
 * Only mPrefix, mValidLifetime and mPreferredLifetime fields are used in the returned prefix info.
 *
 * `OPENTHREAD_CONFIG_BORDER_ROUTING_DHCP6_PD_ENABLE` must be enabled.
 *
 * @param[in]   aInstance    A pointer to an OpenThread instance.
 * @param[out]  aPrefixInfo  A pointer to where the prefix info will be output to.
 *
 * @retval  OT_ERROR_NONE           Successfully retrieved the OMR prefix.
 * @retval  OT_ERROR_INVALID_STATE  The Border Routing Manager is not initialized yet.
 * @retval  OT_ERROR_NOT_FOUND      There are no valid PD prefix on this BR.
 *
 * @sa otBorderRoutingGetOmrPrefix
 * @sa otPlatBorderRoutingProcessIcmp6Ra
 */
otError otBorderRoutingGetPdOmrPrefix(otInstance *aInstance, otBorderRoutingPrefixTableEntry *aPrefixInfo);

/**
 * Gets the data of platform generated RA message processed..
 *
 * `OPENTHREAD_CONFIG_BORDER_ROUTING_DHCP6_PD_ENABLE` must be enabled.
 *
 * @param[in]   aInstance    A pointer to an OpenThread instance.
 * @param[out]  aPrefixInfo  A pointer to where the prefix info will be output to.
 *
 * @retval  OT_ERROR_NONE           Successfully retrieved the Info.
 * @retval  OT_ERROR_INVALID_STATE  The Border Routing Manager is not initialized yet.
 * @retval  OT_ERROR_NOT_FOUND      There are no valid Info on this BR.
 */
otError otBorderRoutingGetPdProcessedRaInfo(otInstance *aInstance, otPdProcessedRaInfo *aPdProcessedRaInfo);

/**
 * Gets the currently favored Off-Mesh-Routable (OMR) Prefix.
 *
 * The favored OMR prefix can be discovered from Network Data or can be this device's local OMR prefix.
 *
 * @param[in]   aInstance    A pointer to an OpenThread instance.
 * @param[out]  aPrefix      A pointer to output the favored OMR prefix.
 * @param[out]  aPreference  A pointer to output the preference associated the favored prefix.
 *
 * @retval  OT_ERROR_INVALID_STATE  The Border Routing Manager is not running yet.
 * @retval  OT_ERROR_NONE           Successfully retrieved the favored OMR prefix.
 */
otError otBorderRoutingGetFavoredOmrPrefix(otInstance *aInstance, otIp6Prefix *aPrefix, otRoutePreference *aPreference);

/**
 * Gets the local On-Link Prefix for the adjacent infrastructure link.
 *
 * The local On-Link Prefix is a 64-bit prefix that's advertised on the infrastructure link if there isn't already a
 * usable on-link prefix being advertised on the link.
 *
 * @param[in]   aInstance  A pointer to an OpenThread instance.
 * @param[out]  aPrefix    A pointer to where the prefix will be output to.
 *
 * @retval  OT_ERROR_INVALID_STATE  The Border Routing Manager is not initialized yet.
 * @retval  OT_ERROR_NONE           Successfully retrieved the local on-link prefix.
 */
otError otBorderRoutingGetOnLinkPrefix(otInstance *aInstance, otIp6Prefix *aPrefix);

/**
 * Gets the currently favored On-Link Prefix.
 *
 * The favored prefix is either a discovered on-link prefix on the infrastructure link or the local on-link prefix.
 *
 * @param[in]   aInstance  A pointer to an OpenThread instance.
 * @param[out]  aPrefix    A pointer to where the prefix will be output to.
 *
 * @retval  OT_ERROR_INVALID_STATE  The Border Routing Manager is not initialized yet.
 * @retval  OT_ERROR_NONE           Successfully retrieved the favored on-link prefix.
 */
otError otBorderRoutingGetFavoredOnLinkPrefix(otInstance *aInstance, otIp6Prefix *aPrefix);

/**
 * Gets the local NAT64 Prefix of the Border Router.
 *
 * NAT64 Prefix might not be advertised in the Thread network.
 *
 * `OPENTHREAD_CONFIG_NAT64_BORDER_ROUTING_ENABLE` must be enabled.
 *
 * @param[in]   aInstance   A pointer to an OpenThread instance.
 * @param[out]  aPrefix     A pointer to where the prefix will be output to.
 *
 * @retval  OT_ERROR_INVALID_STATE  The Border Routing Manager is not initialized yet.
 * @retval  OT_ERROR_NONE           Successfully retrieved the NAT64 prefix.
 */
otError otBorderRoutingGetNat64Prefix(otInstance *aInstance, otIp6Prefix *aPrefix);

/**
 * Gets the currently favored NAT64 prefix.
 *
 * The favored NAT64 prefix can be discovered from infrastructure link or can be this device's local NAT64 prefix.
 *
 * @param[in]   aInstance    A pointer to an OpenThread instance.
 * @param[out]  aPrefix      A pointer to output the favored NAT64 prefix.
 * @param[out]  aPreference  A pointer to output the preference associated the favored prefix.
 *
 * @retval  OT_ERROR_INVALID_STATE  The Border Routing Manager is not initialized yet.
 * @retval  OT_ERROR_NONE           Successfully retrieved the favored NAT64 prefix.
 */
otError otBorderRoutingGetFavoredNat64Prefix(otInstance        *aInstance,
                                             otIp6Prefix       *aPrefix,
                                             otRoutePreference *aPreference);

/**
 * Initializes an `otBorderRoutingPrefixTableIterator`.
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
 */
void otBorderRoutingPrefixTableInitIterator(otInstance *aInstance, otBorderRoutingPrefixTableIterator *aIterator);

/**
 * Iterates over the entries in the Border Router's discovered prefix table.
 *
 * Prefix entries associated with the same discovered router on an infrastructure link are guaranteed to be grouped
 * together (retrieved back-to-back).
 *
 * @param[in]     aInstance    The OpenThread instance.
 * @param[in,out] aIterator    A pointer to the iterator.
 * @param[out]    aEntry       A pointer to the entry to populate.
 *
 * @retval OT_ERROR_NONE        Iterated to the next entry, @p aEntry and @p aIterator are updated.
 * @retval OT_ERROR_NOT_FOUND   No more entries in the table.
 */
otError otBorderRoutingGetNextPrefixTableEntry(otInstance                         *aInstance,
                                               otBorderRoutingPrefixTableIterator *aIterator,
                                               otBorderRoutingPrefixTableEntry    *aEntry);
/**
 * Iterates over the discovered router entries on the infrastructure link.
 *
 * @param[in]     aInstance    The OpenThread instance.
 * @param[in,out] aIterator    A pointer to the iterator.
 * @param[out]    aEntry       A pointer to the entry to populate.
 *
 * @retval OT_ERROR_NONE        Iterated to the next router, @p aEntry and @p aIterator are updated.
 * @retval OT_ERROR_NOT_FOUND   No more router entries.
 */
otError otBorderRoutingGetNextRouterEntry(otInstance                         *aInstance,
                                          otBorderRoutingPrefixTableIterator *aIterator,
                                          otBorderRoutingRouterEntry         *aEntry);

/**
 * Iterates over the peer BRs found in the Network Data.
 *
 * Requires `OPENTHREAD_CONFIG_BORDER_ROUTING_TRACK_PEER_BR_INFO_ENABLE`.
 *
 * Peer BRs are other devices within the Thread mesh that provide external IP connectivity. A device is considered
 * to provide external IP connectivity if at least one of the following conditions is met regarding its Network Data
 * entries:
 *
 * - It has added at least one external route entry.
 * - It has added at least one prefix entry with both the default-route and on-mesh flags set.
 * - It has added at least one domain prefix (with both the domain and on-mesh flags set).
 *
 * The list of peer BRs specifically excludes the current device, even if it is itself acting as a BR.
 *
 * @param[in]     aInstance    The OpenThread instance.
 * @param[in,out] aIterator    A pointer to the iterator.
 * @param[out]    aEntry       A pointer to the entry to populate.
 *
 * @retval OT_ERROR_NONE        Iterated to the next entry, @p aEntry and @p aIterator are updated.
 * @retval OT_ERROR_NOT_FOUND   No more entries.
 */
otError otBorderRoutingGetNextPeerBrEntry(otInstance                           *aInstance,
                                          otBorderRoutingPrefixTableIterator   *aIterator,
                                          otBorderRoutingPeerBorderRouterEntry *aEntry);

/**
 * Returns the number of peer BRs found in the Network Data.
 *
 * Requires `OPENTHREAD_CONFIG_BORDER_ROUTING_TRACK_PEER_BR_INFO_ENABLE`.
 *
 * Peer BRs are other devices within the Thread mesh that provide external IP connectivity. A device is considered
 * to provide external IP connectivity if at least one of the following conditions is met regarding its Network Data
 * entries:
 *
 * - It has added at least one external route entry.
 * - It has added at least one prefix entry with both the default-route and on-mesh flags set.
 * - It has added at least one domain prefix (with both the domain and on-mesh flags set).
 *
 * The list of peer BRs specifically excludes the current device, even if it is itself acting as a BR.
 *
 * @param[in]  aInstance    The OpenThread instance.
 * @param[out] aMinAge      Pointer to an `uint32_t` to return the minimum age among all peer BRs.
 *                          Can be NULL if the caller does not need this information.
 *                          Age is represented as seconds since appearance of the BR entry in the Network Data.
 *
 * @returns The number of peer BRs.
 */
uint16_t otBorderRoutingCountPeerBrs(otInstance *aInstance, uint32_t *aMinAge);

/**
 * A callback function pointer called when the multi-AIL detection state changes.
 *
 * This callback function is invoked by the OpenThread stack whenever the Routing Manager determines a change in
 * whether Border Routers on the Thread mesh might be connected to different Adjacent Infrastructure Links (AILs).
 *
 * See `otBorderRoutingIsMultiAilDetected()` for more details.
 *
 * @param[in] aDetected   `TRUE` if multiple AILs are now detected, `FALSE` otherwise.
 * @param[in] aContext    A pointer to arbitrary context information provided when the callback was registered
 *                        using `otBorderRoutingSetMultiAilCallback()`.
 */
typedef void (*otBorderRoutingMultiAilCallback)(bool aDetected, void *aContext);

/**
 * Gets the current detected state regarding multiple Adjacent Infrastructure Links (AILs).
 *
 * Requires `OPENTHREAD_CONFIG_BORDER_ROUTING_MULTI_AIL_DETECTION_ENABLE`.
 *
 * It returns whether the Routing Manager currently believes that Border Routers (BRs) on the Thread mesh may be
 * connected to different AILs.
 *
 * The detection mechanism operates as follows: The Routing Manager monitors the number of peer BRs listed in the
 * Thread Network Data (see `otBorderRoutingCountPeerBrs()`) and compares this count with the number of peer BRs
 * discovered by processing received Router Advertisement (RA) messages on its connected AIL. If the count derived from
 * Network Data consistently exceeds the count derived from RAs for a detection duration of 10 minutes, it concludes
 * that BRs are likely connected to different AILs. To clear state a shorter window of 1 minute is used.
 *
 * The detection window of 10 minutes helps to avoid false positives due to transient changes. The Routing Manager uses
 * 200 seconds for reachability checks of peer BRs (sending Neighbor Solicitation). Stale Network Data entries are
 * also expected to age out within a few minutes. So a 10-minute detection time accommodates both cases.
 *
 * While generally effective, this detection mechanism may get less reliable in scenarios with a large number of
 * BRs, particularly exceeding ten. This is related to the "Network Data Publisher" mechanism, where BRs might refrain
 * from publishing their external route information in the Network Data to conserve its limited size, potentially
 * skewing the Network Data BR count.
 *
 * @param[in] aInstance  A pointer to the OpenThread instance.
 *
 * @retval TRUE   Has detected that BRs are likely connected to multiple AILs.
 * @retval FALSE  Has not detected (or no longer detects) that BRs are connected to multiple AILs.
 */
bool otBorderRoutingIsMultiAilDetected(otInstance *aInstance);

/**
 * Sets a callback function to be notified of changes in the multi-AIL detection state.
 *
 * Requires `OPENTHREAD_CONFIG_BORDER_ROUTING_MULTI_AIL_DETECTION_ENABLE`.
 *
 * Subsequent calls to this function will overwrite the previous callback setting. Using `NULL` for @p aCallback will
 * disable the callback.
 *
 * @param[in] aInstance  A pointer to the OpenThread instance.
 * @param[in] aCallback  A pointer to the function (`otBorderRoutingMultiAilCallback`) to be called
 *                       upon state changes, or `NULL` to unregister a previously set callback.
 * @param[in] aContext   A pointer to application-specific context that will be passed back
 *                       in the `aCallback` function. This can be `NULL` if no context is needed.
 */
void otBorderRoutingSetMultiAilCallback(otInstance                     *aInstance,
                                        otBorderRoutingMultiAilCallback aCallback,
                                        void                           *aContext);

/**
 * Iterates over the Recursive DNS Server (RDNSS) address entries.
 *
 * Address entries are discovered by processing the RDNSS options within received Router Advertisement messages from
 * routers on infrastructure link.
 *
 * Address entries associated with the same discovered router on an infrastructure link are guaranteed to be grouped
 * together (retrieved back-to-back).
 *
 * @param[in]     aInstance    The OpenThread instance.
 * @param[in,out] aIterator    A pointer to the iterator.
 * @param[out]    aEntry       A pointer to the entry to populate.
 *
 * @retval OT_ERROR_NONE          Iterated to the next address entry, @p aEntry and @p aIterator are updated.
 * @retval OT_ERROR_NOT_FOUND     No more entries in the table.
 * @retval OT_ERROR_INVALID_ARSG  The iterator is invalid (used to iterate over other entry types, e.g. prefix).
 */
otError otBorderRoutingGetNextRdnssAddrEntry(otInstance                         *aInstance,
                                             otBorderRoutingPrefixTableIterator *aIterator,
                                             otBorderRoutingRdnssAddrEntry      *aEntry);

/**
 * Callback function pointer to notify of changes to discovered Recursive DNS Server (RDNSS) address entries.
 *
 * Address entries are discovered by processing the RDNSS options within received Router Advertisement messages from
 * routers on infrastructure link.
 *
 * The `otBorderRoutingGetNextRdnssAddrEntry()` function can be used to iterate over the discovered RDNSS address
 * entries.
 *
 * This callback is invoked when any of the following changes occur to the address entries associated with a discovered
 * router:
 * - A new RDNSS address is advertised by the router.
 * - A previously discovered address is removed due to the router advertising it with a zero lifetime.
 * - A previously discovered address has aged out (its lifetime expired without being re-advertised).
 * - We determine that the router that advertised the address is now unreachable, and therefore all its associated
 *   entries are removed.
 *
 * @param[in] aContext  A pointer to arbitrary context information.
 */
typedef void (*otBorderRoutingRdnssAddrCallback)(void *aContext);

/**
 * Sets the callback to be notified of changes to discovered Recursive DNS Server (RDNSS) address entries.
 *
 * A subsequent call to this function, replaces a previously set callback.
 *
 * @param[in] aInstance   The OpenThread instance.
 * @param[in] aCallback   The callback function pointer. Can be `NULL` if no callback is required.
 * @param[in] aConext     An arbitrary context information (used when invoking the callback).
 *
 */
void otBorderRoutingSetRdnssAddrCallback(otInstance                      *aInstance,
                                         otBorderRoutingRdnssAddrCallback aCallback,
                                         void                            *aContext);

/**
 * Enables / Disables DHCPv6 Prefix Delegation.
 *
 * `OPENTHREAD_CONFIG_BORDER_ROUTING_DHCP6_PD_ENABLE` must be enabled.
 *
 * @param[in] aInstance A pointer to an OpenThread instance.
 * @param[in] aEnabled  Whether to accept platform generated RA messages.
 */
void otBorderRoutingDhcp6PdSetEnabled(otInstance *aInstance, bool aEnabled);

/**
 * Gets the current state of DHCPv6 Prefix Delegation.
 *
 * Requires `OPENTHREAD_CONFIG_BORDER_ROUTING_DHCP6_PD_ENABLE` to be enabled.
 *
 * @param[in]  aInstance  A pointer to an OpenThread instance.
 *
 * @returns The current state of DHCPv6 Prefix Delegation.
 */
otBorderRoutingDhcp6PdState otBorderRoutingDhcp6PdGetState(otInstance *aInstance);

/**
 * When the state of a DHCPv6 Prefix Delegation (PD) on the Thread interface changes, this callback notifies processes
 * in the OS of this changed state.
 *
 * @param[in] aState    The state of DHCPv6 Prefix Delegation State.
 * @param[in] aContext  A pointer to arbitrary context information.
 */
typedef void (*otBorderRoutingRequestDhcp6PdCallback)(otBorderRoutingDhcp6PdState aState, void *aContext);

/**
 * Sets the callback whenever the DHCPv6 PD state changes on the Thread interface.
 *
 * Subsequent calls to this function replace the previously set callback.
 *
 * @param[in] aInstance  A pointer to an OpenThread instance.
 * @param[in] aCallback  A pointer to a function that is called whenever the DHCPv6 PD state changes.
 * @param[in] aContext   A pointer to arbitrary context information.
 */
void otBorderRoutingDhcp6PdSetRequestCallback(otInstance                           *aInstance,
                                              otBorderRoutingRequestDhcp6PdCallback aCallback,
                                              void                                 *aContext);

/**
 * Sets the local on-link prefix.
 *
 * Requires `OPENTHREAD_CONFIG_BORDER_ROUTING_TESTING_API_ENABLE`.
 *
 * This is intended for testing only and using it will make the BR non-compliant with the Thread Specification.
 *
 * @param[in]  aPrefix      The on-link prefix to use.
 */
void otBorderRoutingSetOnLinkPrefix(otInstance *aInstance, const otIp6Prefix *aPrefix);

/**
 * @}
 */

#ifdef __cplusplus
} // extern "C"
#endif

#endif // OPENTHREAD_BORDER_ROUTING_H_
