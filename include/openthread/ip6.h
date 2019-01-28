/*
 *  Copyright (c) 2016, The OpenThread Authors.
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
 *  This file defines the OpenThread IPv6 API.
 */

#ifndef OPENTHREAD_IP6_H_
#define OPENTHREAD_IP6_H_

#include <openthread/message.h>
#include <openthread/platform/radio.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @addtogroup api-ip6
 *
 * @brief
 *   This module includes functions that control IPv6 communication.
 *
 * @{
 *
 */

#define OT_IP6_PREFIX_SIZE 8   ///< Size of an IPv6 prefix (bytes)
#define OT_IP6_IID_SIZE 8      ///< Size of an IPv6 Interface Identifier (bytes)
#define OT_IP6_ADDRESS_SIZE 16 ///< Size of an IPv6 address (bytes)

/**
 * @struct otIp6Address
 *
 * This structure represents an IPv6 address.
 *
 */
OT_TOOL_PACKED_BEGIN
struct otIp6Address
{
    union OT_TOOL_PACKED_FIELD
    {
        uint8_t  m8[OT_IP6_ADDRESS_SIZE];                     ///< 8-bit fields
        uint16_t m16[OT_IP6_ADDRESS_SIZE / sizeof(uint16_t)]; ///< 16-bit fields
        uint32_t m32[OT_IP6_ADDRESS_SIZE / sizeof(uint32_t)]; ///< 32-bit fields
    } mFields;                                                ///< IPv6 accessor fields
} OT_TOOL_PACKED_END;

/**
 * This structure represents an IPv6 address.
 *
 */
typedef struct otIp6Address otIp6Address;

/**
 * This structure represents an IPv6 prefix.
 *
 */
OT_TOOL_PACKED_BEGIN
struct otIp6Prefix
{
    otIp6Address mPrefix; ///< The IPv6 prefix.
    uint8_t      mLength; ///< The IPv6 prefix length.
} OT_TOOL_PACKED_END;

/**
 * This structure represents an IPv6 prefix.
 *
 */
typedef struct otIp6Prefix otIp6Prefix;

/**
 * This structure represents an IPv6 network interface unicast address.
 *
 */
typedef struct otNetifAddress
{
    otIp6Address           mAddress;                ///< The IPv6 unicast address.
    uint8_t                mPrefixLength;           ///< The Prefix length.
    bool                   mPreferred : 1;          ///< TRUE if the address is preferred, FALSE otherwise.
    bool                   mValid : 1;              ///< TRUE if the address is valid, FALSE otherwise.
    bool                   mScopeOverrideValid : 1; ///< TRUE if the mScopeOverride value is valid, FALSE otherwise.
    unsigned int           mScopeOverride : 4;      ///< The IPv6 scope of this address.
    bool                   mRloc : 1;               ///< TRUE if the address is an RLOC, FALSE otherwise.
    struct otNetifAddress *mNext;                   ///< A pointer to the next network interface address.
} otNetifAddress;

/**
 * This structure represents an IPv6 network interface multicast address.
 *
 */
typedef struct otNetifMulticastAddress
{
    otIp6Address                          mAddress; ///< The IPv6 multicast address.
    const struct otNetifMulticastAddress *mNext;    ///< A pointer to the next network interface multicast address.
} otNetifMulticastAddress;

/**
 * This enumeration represents the list of allowable values for an InterfaceId.
 *
 */
typedef enum otNetifInterfaceId
{
    OT_NETIF_INTERFACE_ID_HOST   = -1, ///< The interface ID telling packets received by host side interfaces.
    OT_NETIF_INTERFACE_ID_THREAD = 1,  ///< The Thread Network interface ID.
} otNetifInterfaceId;

/**
 * This structure represents data used by Semantically Opaque IID Generator.
 *
 */
typedef struct
{
    uint8_t *mInterfaceId;       ///< String of bytes representing interface ID. Like "eth0" or "wlan0".
    uint8_t  mInterfaceIdLength; ///< Length of interface ID string.

    uint8_t *mNetworkId;       ///< Network ID (or name). Can be null if mNetworkIdLength is 0.
    uint8_t  mNetworkIdLength; ///< Length of Network ID string.

    uint8_t mDadCounter; ///< Duplicate address detection counter.

    uint8_t *mSecretKey;       ///< Secret key used to create IID. Cannot be null.
    uint16_t mSecretKeyLength; ///< Secret key length in bytes. Should be at least 16 bytes == 128 bits.
} otSemanticallyOpaqueIidGeneratorData;

/**
 * This structure represents an IPv6 socket address.
 *
 */
typedef struct otSockAddr
{
    otIp6Address mAddress; ///< An IPv6 address.
    uint16_t     mPort;    ///< A transport-layer port.
    int8_t       mScopeId; ///< An IPv6 scope identifier.
} otSockAddr;

/**
 * This structure represents the local and peer IPv6 socket addresses.
 *
 */
typedef struct otMessageInfo
{
    otIp6Address mSockAddr;    ///< The local IPv6 address.
    otIp6Address mPeerAddr;    ///< The peer IPv6 address.
    uint16_t     mSockPort;    ///< The local transport-layer port.
    uint16_t     mPeerPort;    ///< The peer transport-layer port.
    int8_t       mInterfaceId; ///< The IPv6 interface identifier.
    uint8_t      mHopLimit;    ///< The IPv6 hop limit.

    /**
     * A pointer to link-specific information. In case @p mInterfaceId is set to OT_NETIF_INTERFACE_ID_THREAD,
     * @p mLinkInfo points to @sa otThreadLinkInfo. This field is only valid for messages received from the
     * Thread radio and is ignored on transmission.
     */
    const void *mLinkInfo;
} otMessageInfo;

/**
 * This function brings up/down the IPv6 interface.
 *
 * Call this function to enable/disable IPv6 communication.
 *
 * @param[in] aInstance A pointer to an OpenThread instance.
 * @param[in] aEnabled  TRUE to enable IPv6, FALSE otherwise.
 *
 * @retval OT_ERROR_NONE            Successfully brought the IPv6 interface up/down.
 * @retval OT_ERROR_INVALID_STATE   IPv6 interface is not available since device is operating in raw-link mode
 *                                  (applicable only when `OPENTHREAD_ENABLE_RAW_LINK_API` feature is enabled).
 *
 */
OTAPI otError OTCALL otIp6SetEnabled(otInstance *aInstance, bool aEnabled);

/**
 * This function indicates whether or not the IPv6 interface is up.
 *
 * @param[in] aInstance A pointer to an OpenThread instance.
 *
 * @retval TRUE   The IPv6 interface is enabled.
 * @retval FALSE  The IPv6 interface is disabled.
 *
 */
OTAPI bool OTCALL otIp6IsEnabled(otInstance *aInstance);

/**
 * Add a Network Interface Address to the Thread interface.
 *
 * The passed-in instance @p aAddress is copied by the Thread interface. The Thread interface only
 * supports a fixed number of externally added unicast addresses. See OPENTHREAD_CONFIG_MAX_EXT_IP_ADDRS.
 *
 * @param[in]  aInstance A pointer to an OpenThread instance.
 * @param[in]  aAddress  A pointer to a Network Interface Address.
 *
 * @retval OT_ERROR_NONE          Successfully added (or updated) the Network Interface Address.
 * @retval OT_ERROR_INVALID_ARGS  The IP Address indicated by @p aAddress is an internal address.
 * @retval OT_ERROR_NO_BUFS       The Network Interface is already storing the maximum allowed external addresses.
 */
OTAPI otError OTCALL otIp6AddUnicastAddress(otInstance *aInstance, const otNetifAddress *aAddress);

/**
 * Remove a Network Interface Address from the Thread interface.
 *
 * @param[in]  aInstance A pointer to an OpenThread instance.
 * @param[in]  aAddress  A pointer to an IP Address.
 *
 * @retval OT_ERROR_NONE          Successfully removed the Network Interface Address.
 * @retval OT_ERROR_INVALID_ARGS  The IP Address indicated by @p aAddress is an internal address.
 * @retval OT_ERROR_NOT_FOUND     The IP Address indicated by @p aAddress was not found.
 */
OTAPI otError OTCALL otIp6RemoveUnicastAddress(otInstance *aInstance, const otIp6Address *aAddress);

/**
 * Get the list of IPv6 addresses assigned to the Thread interface.
 *
 * @param[in]  aInstance A pointer to an OpenThread instance.
 *
 * @returns A pointer to the first Network Interface Address.
 */
OTAPI const otNetifAddress *OTCALL otIp6GetUnicastAddresses(otInstance *aInstance);

/**
 * Subscribe the Thread interface to a Network Interface Multicast Address.
 *
 * The passed in instance @p aAddress will be copied by the Thread interface. The Thread interface only
 * supports a fixed number of externally added multicast addresses. See OPENTHREAD_CONFIG_MAX_EXT_MULTICAST_IP_ADDRS.
 *
 * @param[in]  aInstance A pointer to an OpenThread instance.
 * @param[in]  aAddress  A pointer to an IP Address.
 *
 * @retval OT_ERROR_NONE           Successfully subscribed to the Network Interface Multicast Address.
 * @retval OT_ERROR_ALREADY        The multicast address is already subscribed.
 * @retval OT_ERROR_INVALID_ARGS   The IP Address indicated by @p aAddress is invalid address.
 * @retval OT_ERROR_INVALID_STATE  The Network Interface is not up.
 * @retval OT_ERROR_NO_BUFS        The Network Interface is already storing the maximum allowed external multicast
 *                                 addresses.
 *
 */
otError otIp6SubscribeMulticastAddress(otInstance *aInstance, const otIp6Address *aAddress);

/**
 * Unsubscribe the Thread interface to a Network Interface Multicast Address.
 *
 * @param[in]  aInstance A pointer to an OpenThread instance.
 * @param[in]  aAddress  A pointer to an IP Address.
 *
 * @retval OT_ERROR_NONE          Successfully unsubscribed to the Network Interface Multicast Address.
 * @retval OT_ERROR_INVALID_ARGS  The IP Address indicated by @p aAddress is an internal address.
 * @retval OT_ERROR_NOT_FOUND     The IP Address indicated by @p aAddress was not found.
 */
otError otIp6UnsubscribeMulticastAddress(otInstance *aInstance, const otIp6Address *aAddress);

/**
 * Get the list of IPv6 multicast addresses subscribed to the Thread interface.
 *
 * @param[in]  aInstance A pointer to an OpenThread instance.
 *
 * @returns A pointer to the first Network Interface Multicast Address.
 *
 */
const otNetifMulticastAddress *otIp6GetMulticastAddresses(otInstance *aInstance);

/**
 * Check if multicast promiscuous mode is enabled on the Thread interface.
 *
 * @param[in]  aInstance A pointer to an OpenThread instance.
 *
 * @sa otIp6SetMulticastPromiscuousEnabled
 *
 */
bool otIp6IsMulticastPromiscuousEnabled(otInstance *aInstance);

/**
 * Enable multicast promiscuous mode on the Thread interface.
 *
 * @param[in]  aInstance  A pointer to an OpenThread instance.
 * @param[in]  aEnabled   TRUE to enable Multicast Promiscuous mode, FALSE otherwise.
 *
 * @sa otIp6IsMulticastPromiscuousEnabled
 *
 */
void otIp6SetMulticastPromiscuousEnabled(otInstance *aInstance, bool aEnabled);

/**
 * This function pointer is called to create IPv6 IID during SLAAC procedure.
 *
 * @param[in]     aInstance  A pointer to an OpenThread instance.
 * @param[inout]  aAddress   A pointer to structure containing IPv6 address for which IID is being created.
 * @param[inout]  aContext   A pointer to creator-specific context.
 *
 * @retval OT_ERROR_NONE                          Created valid IID for given IPv6 address.
 * @retval OT_ERROR_IP6_ADDRESS_CREATION_FAILURE  Creation of valid IID for given IPv6 address failed.
 *
 */
typedef otError (*otIp6SlaacIidCreate)(otInstance *aInstance, otNetifAddress *aAddress, void *aContext);

/**
 * Update all automatically created IPv6 addresses for prefixes from current Network Data with SLAAC procedure.
 *
 * @param[in]     aInstance      A pointer to an OpenThread instance.
 * @param[inout]  aAddresses     A pointer to an array of automatically created IPv6 addresses.
 * @param[in]     aNumAddresses  The number of slots in aAddresses array.
 * @param[in]     aIidCreate     A pointer to a function that is called to create IPv6 IIDs.
 * @param[in]     aContext       A pointer to data passed to aIidCreate function.
 *
 */
void otIp6SlaacUpdate(otInstance *        aInstance,
                      otNetifAddress *    aAddresses,
                      uint32_t            aNumAddresses,
                      otIp6SlaacIidCreate aIidCreate,
                      void *              aContext);

/**
 * Create random IID for given IPv6 address.
 *
 * @param[in]     aInstance   A pointer to an OpenThread instance.
 * @param[inout]  aAddresses  A pointer to structure containing IPv6 address for which IID is being created.
 * @param[in]     aContext    A pointer to unused data.
 *
 * @retval OT_ERROR_NONE  Created valid IID for given IPv6 address.
 *
 */
otError otIp6CreateRandomIid(otInstance *aInstance, otNetifAddress *aAddresses, void *aContext);

/**
 * Create IID for given IPv6 address using extended MAC address.
 *
 * @param[in]     aInstance   A pointer to an OpenThread instance.
 * @param[inout]  aAddresses  A pointer to structure containing IPv6 address for which IID is being created.
 * @param[in]     aContext    A pointer to unused data.
 *
 * @retval OT_ERROR_NONE  Created valid IID for given IPv6 address.
 *
 */
otError otIp6CreateMacIid(otInstance *aInstance, otNetifAddress *aAddresses, void *aContext);

/**
 * Create semantically opaque IID for given IPv6 address.
 *
 * @param[in]     aInstance   A pointer to an OpenThread instance.
 * @param[inout]  aAddresses  A pointer to structure containing IPv6 address for which IID is being created.
 * @param[inout]  aContext    A pointer to a otSemanticallyOpaqueIidGeneratorData structure.
 *
 * @retval OT_ERROR_NONE                          Created valid IID for given IPv6 address.
 * @retval OT_ERROR_IP6_ADDRESS_CREATION_FAILURE  Could not create valid IID for given IPv6 address.
 *
 */
otError otIp6CreateSemanticallyOpaqueIid(otInstance *aInstance, otNetifAddress *aAddresses, void *aContext);

/**
 * Allocate a new message buffer for sending an IPv6 message.
 *
 * @note If @p aSettings is 'NULL', the link layer security is enabled and the message priority is set to
 * OT_MESSAGE_PRIORITY_NORMAL by default.
 *
 * @param[in]  aInstance  A pointer to an OpenThread instance.
 * @param[in]  aSettings  A pointer to the message settings or NULL to set default settings.
 *
 * @returns A pointer to the message buffer or NULL if no message buffers are available or parameters are invalid.
 *
 * @sa otMessageFree
 *
 */
otMessage *otIp6NewMessage(otInstance *aInstance, const otMessageSettings *aSettings);

/**
 * Allocate a new message buffer and write the IPv6 datagram to the message buffer for sending an IPv6 message.
 *
 * @note If @p aSettings is NULL, the link layer security is enabled and the message priority is obtained from IPv6
 *       message itself.
 *       If @p aSettings is not NULL, the @p aSetting->mPriority is ignored and obtained from IPv6 message itself.
 *
 * @param[in]  aInstance    A pointer to an OpenThread instance.
 * @param[in]  aData        A pointer to the IPv6 datagram buffer.
 * @param[in]  aDataLength  The size of the IPv6 datagram buffer pointed by @p aData.
 * @param[in]  aSettings    A pointer to the message settings or NULL to set default settings.
 *
 * @returns A pointer to the message or NULL if malformed IPv6 header or insufficient message buffers are available.
 *
 * @sa otMessageFree
 *
 */
otMessage *otIp6NewMessageFromBuffer(otInstance *             aInstance,
                                     const uint8_t *          aData,
                                     uint16_t                 aDataLength,
                                     const otMessageSettings *aSettings);

/**
 * This function pointer is called when an IPv6 datagram is received.
 *
 * @param[in]  aMessage  A pointer to the message buffer containing the received IPv6 datagram.
 * @param[in]  aContext  A pointer to application-specific context.
 *
 */
typedef void (*otIp6ReceiveCallback)(otMessage *aMessage, void *aContext);

/**
 * This function registers a callback to provide received IPv6 datagrams.
 *
 * By default, this callback does not pass Thread control traffic.  See otIp6SetReceiveFilterEnabled() to
 * change the Thread control traffic filter setting.
 *
 * @param[in]  aInstance         A pointer to an OpenThread instance.
 * @param[in]  aCallback         A pointer to a function that is called when an IPv6 datagram is received or
 *                               NULL to disable the callback.
 * @param[in]  aCallbackContext  A pointer to application-specific context.
 *
 * @sa otIp6IsReceiveFilterEnabled
 * @sa otIp6SetReceiveFilterEnabled
 *
 */
void otIp6SetReceiveCallback(otInstance *aInstance, otIp6ReceiveCallback aCallback, void *aCallbackContext);

/**
 * This function pointer is called when an internal IPv6 address is added or removed.
 *
 * @param[in]   aAddress            A pointer to the IPv6 address.
 * @param[in]   aPrefixLength       The prefix length if @p aAddress is unicast address, and 128 for multicast address.
 * @param[in]   aIsAdded            TRUE if the @p aAddress was added, FALSE if @p aAddress was removed.
 * @param[in]   aContext            A pointer to application-specific context.
 *
 */
typedef void (*otIp6AddressCallback)(const otIp6Address *aAddress,
                                     uint8_t             aPrefixLength,
                                     bool                aIsAdded,
                                     void *              aContext);

/**
 * This function registers a callback to notify internal IPv6 address changes.
 *
 * @param[in]   aInstance           A pointer to an OpenThread instance.
 * @param[in]   aCallback           A pointer to a function that is called when an internal IPv6 address is added or
 *                                  removed. NULL to disable the callback.
 * @param[in]   aCallbackContext    A pointer to application-specific context.
 *
 */
void otIp6SetAddressCallback(otInstance *aInstance, otIp6AddressCallback aCallback, void *aCallbackContext);

/**
 * This function indicates whether or not Thread control traffic is filtered out when delivering IPv6 datagrams
 * via the callback specified in otIp6SetReceiveCallback().
 *
 * @param[in]  aInstance A pointer to an OpenThread instance.
 *
 * @returns  TRUE if Thread control traffic is filtered out, FALSE otherwise.
 *
 * @sa otIp6SetReceiveCallback
 * @sa otIp6SetReceiveFilterEnabled
 *
 */
bool otIp6IsReceiveFilterEnabled(otInstance *aInstance);

/**
 * This function sets whether or not Thread control traffic is filtered out when delivering IPv6 datagrams
 * via the callback specified in otIp6SetReceiveCallback().
 *
 * @param[in]  aInstance A pointer to an OpenThread instance.
 * @param[in]  aEnabled  TRUE if Thread control traffic is filtered out, FALSE otherwise.
 *
 * @sa otIp6SetReceiveCallback
 * @sa otIsReceiveIp6FilterEnabled
 *
 */
void otIp6SetReceiveFilterEnabled(otInstance *aInstance, bool aEnabled);

/**
 * This function sends an IPv6 datagram via the Thread interface.
 *
 * @param[in]  aInstance A pointer to an OpenThread instance.
 * @param[in]  aMessage  A pointer to the message buffer containing the IPv6 datagram.
 *
 */
otError otIp6Send(otInstance *aInstance, otMessage *aMessage);

/**
 * This function adds a port to the allowed unsecured port list.
 *
 * @param[in]  aInstance A pointer to an OpenThread instance.
 * @param[in]  aPort     The port value.
 *
 * @retval OT_ERROR_NONE     The port was successfully added to the allowed unsecure port list.
 * @retval OT_ERROR_NO_BUFS  The unsecure port list is full.
 *
 */
otError otIp6AddUnsecurePort(otInstance *aInstance, uint16_t aPort);

/**
 * This function removes a port from the allowed unsecure port list.
 *
 * @note This function removes @p aPort by overwriting @p aPort with the element after @p aPort in the internal port
 *       list. Be careful when calling otIp6GetUnsecurePorts() followed by otIp6RemoveUnsecurePort() to remove unsecure
 *       ports.
 *
 * @param[in]  aInstance A pointer to an OpenThread instance.
 * @param[in]  aPort     The port value.
 *
 * @retval OT_ERROR_NONE       The port was successfully removed from the allowed unsecure port list.
 * @retval OT_ERROR_NOT_FOUND  The port was not found in the unsecure port list.
 *
 */
otError otIp6RemoveUnsecurePort(otInstance *aInstance, uint16_t aPort);

/**
 * This function removes all ports from the allowed unsecure port list.
 *
 * @param[in]  aInstance A pointer to an OpenThread instance.
 *
 */
void otIp6RemoveAllUnsecurePorts(otInstance *aInstance);

/**
 * This function returns a pointer to the unsecure port list.
 *
 * @note Port value 0 is used to indicate an invalid entry.
 *
 * @param[in]   aInstance    A pointer to an OpenThread instance.
 * @param[out]  aNumEntries  The number of entries in the list.
 *
 * @returns A pointer to the unsecure port list.
 *
 */
const uint16_t *otIp6GetUnsecurePorts(otInstance *aInstance, uint8_t *aNumEntries);

/**
 * Test if two IPv6 addresses are the same.
 *
 * @param[in]  aFirst   A pointer to the first IPv6 address to compare.
 * @param[in]  aSecond  A pointer to the second IPv6 address to compare.
 *
 * @retval TRUE   The two IPv6 addresses are the same.
 * @retval FALSE  The two IPv6 addresses are not the same.
 *
 */
OTAPI bool OTCALL otIp6IsAddressEqual(const otIp6Address *aFirst, const otIp6Address *aSecond);

/**
 * Convert a human-readable IPv6 address string into a binary representation.
 *
 * @param[in]   aString   A pointer to a NULL-terminated string.
 * @param[out]  aAddress  A pointer to an IPv6 address.
 *
 * @retval OT_ERROR_NONE          Successfully parsed the string.
 * @retval OT_ERROR_INVALID_ARGS  Failed to parse the string.
 *
 */
OTAPI otError OTCALL otIp6AddressFromString(const char *aString, otIp6Address *aAddress);

/**
 * This function returns the prefix match length (bits) for two IPv6 addresses.
 *
 * @param[in]  aFirst   A pointer to the first IPv6 address.
 * @param[in]  aSecond  A pointer to the second IPv6 address.
 *
 * @returns  The prefix match length in bits.
 *
 */
OTAPI uint8_t OTCALL otIp6PrefixMatch(const otIp6Address *aFirst, const otIp6Address *aSecond);

/**
 * This function indicates whether or not a given IPv6 address is the Unspecified Address.
 *
 * @param[in]  aAddress   A pointer to an IPv6 address.
 *
 * @retval TRUE   If the IPv6 address is the Unspecified Address.
 * @retval FALSE  If the IPv6 address is not the Unspecified Address.
 *
 */
bool otIp6IsAddressUnspecified(const otIp6Address *aAddress);

/**
 * This function perform OpenThread source address selection.
 *
 * @param[in]     aInstance     A pointer to an OpenThread instance.
 * @param[inout]  aMessageInfo  A pointer to the message information.
 *
 * @retval  OT_ERROR_NONE       Found a source address and is filled into mSockAddr of @p aMessageInfo.
 * @retval  OT_ERROR_NOT_FOUND  No source address was found and @p aMessageInfo is unchanged.
 *
 */
otError otIp6SelectSourceAddress(otInstance *aInstance, otMessageInfo *aMessageInfo);

/**
 * @}
 *
 */

#ifdef __cplusplus
} // extern "C"
#endif

#endif // OPENTHREAD_IP6_H_
