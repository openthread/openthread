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
 *   This file includes definitions for managing Thread Network Data.
 */

#ifndef NETWORK_DATA_HPP_
#define NETWORK_DATA_HPP_

#include "openthread-core-config.h"

#include <openthread/border_router.h>
#include <openthread/netdata.h>
#include <openthread/server.h>

#include "coap/coap.hpp"
#include "common/clearable.hpp"
#include "common/const_cast.hpp"
#include "common/equatable.hpp"
#include "common/locator.hpp"
#include "common/timer.hpp"
#include "net/udp6.hpp"
#include "thread/lowpan.hpp"
#include "thread/mle_router.hpp"
#include "thread/network_data_tlvs.hpp"
#include "thread/network_data_types.hpp"

namespace ot {

/**
 * @addtogroup core-netdata
 * @brief
 *   This module includes definitions for generating, processing, and managing Thread Network Data.
 *
 * @{
 *
 * @defgroup core-netdata-core Core
 * @defgroup core-netdata-leader Leader
 * @defgroup core-netdata-local Local
 * @defgroup core-netdata-tlvs TLVs
 *
 * @}
 */

/**
 * @namespace ot::NetworkData
 *
 * @brief
 *   This namespace includes definitions for managing Thread Network Data.
 */
namespace NetworkData {

namespace Service {
class Manager;
}

/**
 * @addtogroup core-netdata-core
 *
 * @brief
 *   This module includes definitions for managing Thread Network Data.
 *
 * @{
 */

class Leader;
class Publisher;
class MutableNetworkData;

/**
 * Represents a Iterator used to iterate through Network Data info (e.g., see `GetNextOnMeshPrefix()`)
 */
typedef otNetworkDataIterator Iterator;

constexpr Iterator kIteratorInit = OT_NETWORK_DATA_ITERATOR_INIT; ///< Initializer for `Iterator` type.

/**
 * Represents an immutable Network Data.
 */
class NetworkData : public InstanceLocator
{
    friend class Leader;
    friend class Publisher;
    friend class MutableNetworkData;
    friend class Service::Manager;

public:
    static constexpr uint8_t kMaxSize = 254; ///< Maximum size of Thread Network Data in bytes.

    /**
     * Initializes the `NetworkData` from a given pointer to a buffer and length.
     *
     * @param[in] aInstance     A reference to the OpenThread instance.
     * @param[in] aTlvs         A pointer to the buffer containing the TLVs.
     * @param[in] aLength       The length (number of bytes) of @p aTlvs buffer.
     */
    explicit NetworkData(Instance &aInstance, const uint8_t *aTlvs = nullptr, uint8_t aLength = 0)
        : InstanceLocator(aInstance)
        , mTlvs(aTlvs)
        , mLength(aLength)
    {
    }

    /**
     * Initializes the `NetworkData` from a range of TLVs (given as pair of start and end pointers).
     *
     * @param[in] aInstance     A reference to the OpenThread instance.
     * @param[in] aStartTlv     A pointer to the start of the TLVs buffer.
     * @param[in] aEndTlv       A pointer to the end of the TLVs buffer.
     */
    NetworkData(Instance &aInstance, const NetworkDataTlv *aStartTlv, const NetworkDataTlv *aEndTlv)
        : InstanceLocator(aInstance)
        , mTlvs(reinterpret_cast<const uint8_t *>(aStartTlv))
        , mLength(static_cast<uint8_t>(reinterpret_cast<const uint8_t *>(aEndTlv) -
                                       reinterpret_cast<const uint8_t *>(aStartTlv)))
    {
    }

    /**
     * Returns the length of `NetworkData` (number of bytes).
     *
     * @returns The length of `NetworkData` (number of bytes).
     */
    uint8_t GetLength(void) const { return mLength; }

    /**
     * Returns a pointer to the start of the TLVs in `NetworkData`.
     *
     * @returns A pointer to the start of the TLVs.
     */
    const uint8_t *GetBytes(void) const { return mTlvs; }

    /**
     * Provides full or stable copy of the Thread Network Data.
     *
     * @param[in]     aType        The Network Data type to copy, the full set or stable subset.
     * @param[out]    aData        A pointer to the data buffer to copy the Network Data into.
     * @param[in,out] aDataLength  On entry, size of the data buffer pointed to by @p aData.
     *                             On exit, number of copied bytes.
     *
     * @retval kErrorNone       Successfully copied Thread Network Data.
     * @retval kErrorNoBufs     Not enough space in @p aData to fully copy Thread Network Data.
     */
    Error CopyNetworkData(Type aType, uint8_t *aData, uint8_t &aDataLength) const;

    /**
     * Provides full or stable copy of the Thread Network Data.
     *
     * @param[in]    aType        The Network Data type to copy, the full set or stable subset.
     * @param[out]   aNetworkData A reference to a `MutableNetworkData` to copy the Network Data into.
     *
     * @retval kErrorNone       Successfully copied Thread Network Data.
     * @retval kErrorNoBufs     Not enough space in @p aNetworkData to fully copy Thread Network Data.
     */
    Error CopyNetworkData(Type aType, MutableNetworkData &aNetworkData) const;

    /**
     * Provides the next On Mesh prefix in the Thread Network Data.
     *
     * @param[in,out]  aIterator  A reference to the Network Data iterator.
     * @param[out]     aConfig    A reference to a config variable where the On Mesh Prefix information will be placed.
     *
     * @retval kErrorNone       Successfully found the next On Mesh prefix.
     * @retval kErrorNotFound   No subsequent On Mesh prefix exists in the Thread Network Data.
     */
    Error GetNextOnMeshPrefix(Iterator &aIterator, OnMeshPrefixConfig &aConfig) const;

    /**
     * Provides the next On Mesh prefix in the Thread Network Data for a given RLOC16.
     *
     * @param[in,out]  aIterator  A reference to the Network Data iterator.
     * @param[in]      aRloc16    The RLOC16 value.
     * @param[out]     aConfig    A reference to a config variable where the On Mesh Prefix information will be placed.
     *
     * @retval kErrorNone       Successfully found the next On Mesh prefix.
     * @retval kErrorNotFound   No subsequent On Mesh prefix exists in the Thread Network Data.
     */
    Error GetNextOnMeshPrefix(Iterator &aIterator, uint16_t aRloc16, OnMeshPrefixConfig &aConfig) const;

    /**
     * Provides the next external route in the Thread Network Data.
     *
     * @param[in,out]  aIterator  A reference to the Network Data iterator.
     * @param[out]     aConfig    A reference to a config variable where the external route information will be placed.
     *
     * @retval kErrorNone       Successfully found the next external route.
     * @retval kErrorNotFound   No subsequent external route exists in the Thread Network Data.
     */
    Error GetNextExternalRoute(Iterator &aIterator, ExternalRouteConfig &aConfig) const;

    /**
     * Provides the next external route in the Thread Network Data for a given RLOC16.
     *
     * @param[in,out]  aIterator  A reference to the Network Data iterator.
     * @param[in]      aRloc16    The RLOC16 value.
     * @param[out]     aConfig    A reference to a config variable where the external route information will be placed.
     *
     * @retval kErrorNone       Successfully found the next external route.
     * @retval kErrorNotFound   No subsequent external route exists in the Thread Network Data.
     */
    Error GetNextExternalRoute(Iterator &aIterator, uint16_t aRloc16, ExternalRouteConfig &aConfig) const;

    /**
     * Provides the next service in the Thread Network Data.
     *
     * @param[in,out]  aIterator  A reference to the Network Data iterator.
     * @param[out]     aConfig    A reference to a config variable where the service information will be placed.
     *
     * @retval kErrorNone       Successfully found the next service.
     * @retval kErrorNotFound   No subsequent service exists in the Thread Network Data.
     */
    Error GetNextService(Iterator &aIterator, ServiceConfig &aConfig) const;

    /**
     * Provides the next service in the Thread Network Data for a given RLOC16.
     *
     * @param[in,out]  aIterator  A reference to the Network Data iterator.
     * @param[in]      aRloc16    The RLOC16 value.
     * @param[out]     aConfig    A reference to a config variable where the service information will be placed.
     *
     * @retval kErrorNone       Successfully found the next service.
     * @retval kErrorNotFound   No subsequent service exists in the Thread Network Data.
     */
    Error GetNextService(Iterator &aIterator, uint16_t aRloc16, ServiceConfig &aConfig) const;

    /**
     * Gets the next 6LoWPAN Context ID info in the Thread Network Data.
     *
     * @param[in,out]  aIterator     A reference to the Network Data iterator.
     * @param[out]     aContextInfo  A reference to where the retrieved 6LoWPAN Context ID information will be placed.
     *
     * @retval kErrorNone      Successfully found the next 6LoWPAN Context ID info.
     * @retval kErrorNotFound  No subsequent 6LoWPAN Context info exists in the partition's Network Data.
     */
    Error GetNextLowpanContextInfo(Iterator &aIterator, LowpanContextInfo &aContextInfo) const;

    /**
     * Indicates whether or not the Thread Network Data contains a given on mesh prefix entry.
     *
     * @param[in]  aPrefix   The on mesh prefix config to check.
     *
     * @retval TRUE  if Network Data contains an on mesh prefix matching @p aPrefix.
     * @retval FALSE if Network Data does not contain an on mesh prefix matching @p aPrefix.
     */
    bool ContainsOnMeshPrefix(const OnMeshPrefixConfig &aPrefix) const;

    /**
     * Indicates whether or not the Thread Network Data contains a given external route entry.
     *
     * @param[in]  aRoute   The external route config to check.
     *
     * @retval TRUE  if Network Data contains an external route matching @p aRoute.
     * @retval FALSE if Network Data does not contain an external route matching @p aRoute.
     */
    bool ContainsExternalRoute(const ExternalRouteConfig &aRoute) const;

    /**
     * Indicates whether or not the Thread Network Data contains a given service entry.
     *
     * @param[in]  aService   The service config to check.
     *
     * @retval TRUE  if Network Data contains a service matching @p aService.
     * @retval FALSE if Network Data does not contain a service matching @p aService.
     */
    bool ContainsService(const ServiceConfig &aService) const;

    /**
     * Indicates whether or not the Thread Network Data contains all the on mesh prefixes, external
     * routes, and service entries as in another given Network Data associated with a given RLOC16.
     *
     * @param[in] aCompare         The Network Data to compare with.
     * @param[in] aRloc16          The RLOC16 to consider.
     *
     * @retval TRUE  if Network Data contains all the same entries as in @p aCompare for @p aRloc16.
     * @retval FALSE if Network Data does not contains all the same entries as in @p aCompare for @p aRloc16.
     */
    bool ContainsEntriesFrom(const NetworkData &aCompare, uint16_t aRloc16) const;

    /**
     * Finds and returns Domain ID associated with a given prefix in the Thread Network data.
     *
     * @param[in]  aPrefix     The prefix to search for.
     * @param[out] aDomainId   A reference to return the Domain ID.
     *
     * @retval kErrorNone      Successfully found @p aPrefix in the Network Data and updated @p aDomainId.
     * @retval kErrorNotFound  Could not find @p aPrefix in the Network Data.
     */
    Error FindDomainIdFor(const Ip6::Prefix &aPrefix, uint8_t &aDomainId) const;

    /**
     * Finds border routers and servers in the Network Data matching specified filters, returning their RLOC16s.
     *
     * @p aBrFilter can be used to filter the type of BRs. It can be set to `kAnyBrOrServer` to include all BRs and
     * servers. `kBrProvidingExternalIpConn` restricts it to BRs providing external IP connectivity where at least one
     * the below conditions hold:
     *
     * - It has added at least one external route entry.
     * - It has added at least one prefix entry with default-route and on-mesh flags set.
     * - It has added at least one domain prefix (domain and on-mesh flags set).
     *
     * Should be used when the RLOC16s are present in the Network Data (when the Network Data contains the
     * full set and not the stable subset).
     *
     * @param[in]  aBrFilter    Indicates BR filter.
     * @param[in]  aRoleFilter  Indicates role filter (any role, router role only, or child only).
     * @param[out] aRlocs       Array to output the list of RLOC16s.
     */
    void FindRlocs(BorderRouterFilter aBrFilter, RoleFilter aRoleFilter, Rlocs &aRlocs) const;

    /**
     * Counts the number of border routers providing external IP connectivity.
     *
     * A border router is considered to provide external IP connectivity if at least one of the below conditions hold:
     *
     * - It has added at least one external route entry.
     * - It has added at least one prefix entry with default-route and on-mesh flags set.
     * - It has added at least one domain prefix (domain and on-mesh flags set).
     *
     * Should be used when the RLOC16s are present in the Network Data (when the Network Data contains the
     * full set and not the stable subset).
     *
     * @param[in] aRoleFilter   Indicates which RLOCs to include (any role, router only, or child only).
     *
     * @returns The number of border routers in Thread Network Data matching @p aRoleFilter.
     */
    uint8_t CountBorderRouters(RoleFilter aRoleFilter) const;

    /**
     * Indicates whether the network data contains a border providing external IP connectivity with a given
     * RLOC16.
     *
     * A border router is considered to provide external IP connectivity if at least one of the below conditions hold
     *
     * - It has added at least one external route entry.
     * - It has added at least one prefix entry with default-route and on-mesh flags set.
     * - It has added at least one domain prefix (domain and on-mesh flags set).
     *
     * Should be used when the RLOC16s are present in the Network Data (when the Network Data contains the
     * full set and not the stable subset).
     *
     * @param[in] aRloc16   The RLOC16 to check.
     *
     * @returns TRUE  If the network data contains a border router with @p aRloc16 providing IP connectivity.
     * @returns FALSE If the network data does not contain a border router with @p aRloc16 providing IP connectivity.
     */
    bool ContainsBorderRouterWithRloc(uint16_t aRloc16) const;

protected:
    /**
     * Defines Service Data match mode.
     */
    enum ServiceMatchMode : uint8_t
    {
        kServicePrefixMatch, ///< Match the Service Data by prefix.
        kServiceExactMatch,  ///< Match the full Service Data exactly.
    };

    /**
     * Returns a pointer to the start of Network Data TLV sequence.
     *
     * @returns A pointer to the start of Network Data TLV sequence.
     */
    const NetworkDataTlv *GetTlvsStart(void) const { return reinterpret_cast<const NetworkDataTlv *>(mTlvs); }

    /**
     * Returns a pointer to the end of Network Data TLV sequence.
     *
     * @returns A pointer to the end of Network Data TLV sequence.
     */
    const NetworkDataTlv *GetTlvsEnd(void) const { return reinterpret_cast<const NetworkDataTlv *>(mTlvs + mLength); }

    /**
     * Returns a pointer to a Prefix TLV.
     *
     * @param[in]  aPrefix        A pointer to an IPv6 prefix.
     * @param[in]  aPrefixLength  The prefix length pointed to by @p aPrefix (in bits).
     *
     * @returns A pointer to the Prefix TLV if one is found or `nullptr` if no matching Prefix TLV exists.
     */
    const PrefixTlv *FindPrefix(const uint8_t *aPrefix, uint8_t aPrefixLength) const;

    /**
     * Returns a pointer to a Prefix TLV.
     *
     * @param[in]  aPrefix        An IPv6 prefix.
     *
     * @returns A pointer to the Prefix TLV if one is found or `nullptr` if no matching Prefix TLV exists.
     */
    const PrefixTlv *FindPrefix(const Ip6::Prefix &aPrefix) const
    {
        return FindPrefix(aPrefix.GetBytes(), aPrefix.GetLength());
    }

    /**
     * Returns a pointer to a matching Service TLV.
     *
     * @param[in]  aEnterpriseNumber  Enterprise Number.
     * @param[in]  aServiceData       A Service Data.
     * @param[in]  aServiceMatchMode  The Service Data match mode.
     *
     * @returns A pointer to the Service TLV if one is found or `nullptr` if no matching Service TLV exists.
     */
    const ServiceTlv *FindService(uint32_t           aEnterpriseNumber,
                                  const ServiceData &aServiceData,
                                  ServiceMatchMode   aServiceMatchMode) const;

    /**
     * Returns the next pointer to a matching Service TLV.
     *
     * Can be used to iterate over all Service TLVs that start with a given Service Data.
     *
     * @param[in]  aPrevServiceTlv    Set to `nullptr` to start from the beginning of the TLVs (finding the first
     *                                matching Service TLV), or a pointer to the previous Service TLV returned from
     *                                this method to iterate to the next matching Service TLV.
     * @param[in]  aEnterpriseNumber  Enterprise Number.
     * @param[in]  aServiceData       A Service Data to match with Service TLVs.
     * @param[in]  aServiceMatchMode  The Service Data match mode.
     *
     * @returns A pointer to the next matching Service TLV if one is found or `nullptr` if it cannot be found.
     */
    const ServiceTlv *FindNextService(const ServiceTlv  *aPrevServiceTlv,
                                      uint32_t           aEnterpriseNumber,
                                      const ServiceData &aServiceData,
                                      ServiceMatchMode   aServiceMatchMode) const;

    /**
     * Returns the next pointer to a matching Thread Service TLV (with Thread Enterprise number).
     *
     * Can be used to iterate over all Thread Service TLVs that start with a given Service Data.
     *
     * @param[in]  aPrevServiceTlv    Set to `nullptr` to start from the beginning of the TLVs (finding the first
     *                                matching Service TLV), or a pointer to the previous Service TLV returned from
     *                                this method to iterate to the next matching Service TLV.
     * @param[in]  aServiceData       A Service Data to match with Service TLVs.
     * @param[in]  aServiceMatchMode  The Service Data match mode.
     *
     * @returns A pointer to the next matching Thread Service TLV if one is found or `nullptr` if it cannot be found.
     */
    const ServiceTlv *FindNextThreadService(const ServiceTlv  *aPrevServiceTlv,
                                            const ServiceData &aServiceData,
                                            ServiceMatchMode   aServiceMatchMode) const;

private:
    class NetworkDataIterator
    {
    public:
        explicit NetworkDataIterator(Iterator &aIterator)
            : mIteratorBuffer(reinterpret_cast<uint8_t *>(&aIterator))
        {
        }

        const NetworkDataTlv *GetTlv(const uint8_t *aTlvs) const
        {
            return reinterpret_cast<const NetworkDataTlv *>(aTlvs + GetTlvOffset());
        }

        void AdvanceTlv(const uint8_t *aTlvs)
        {
            SaveTlvOffset(GetTlv(aTlvs)->GetNext(), aTlvs);
            SetSubTlvOffset(0);
            SetEntryIndex(0);
        }

        const NetworkDataTlv *GetSubTlv(const NetworkDataTlv *aSubTlvs) const
        {
            return reinterpret_cast<const NetworkDataTlv *>(reinterpret_cast<const uint8_t *>(aSubTlvs) +
                                                            GetSubTlvOffset());
        }

        void AdvanceSubTlv(const NetworkDataTlv *aSubTlvs)
        {
            SaveSubTlvOffset(GetSubTlv(aSubTlvs)->GetNext(), aSubTlvs);
            SetEntryIndex(0);
        }

        uint8_t GetAndAdvanceIndex(void) { return mIteratorBuffer[kEntryPosition]++; }

        bool IsNewEntry(void) const { return GetEntryIndex() == 0; }
        void MarkEntryAsNotNew(void) { SetEntryIndex(1); }

    private:
        static constexpr uint8_t kTlvPosition    = 0;
        static constexpr uint8_t kSubTlvPosition = 1;
        static constexpr uint8_t kEntryPosition  = 2;

        uint8_t GetTlvOffset(void) const { return mIteratorBuffer[kTlvPosition]; }
        uint8_t GetSubTlvOffset(void) const { return mIteratorBuffer[kSubTlvPosition]; }
        void    SetSubTlvOffset(uint8_t aOffset) { mIteratorBuffer[kSubTlvPosition] = aOffset; }
        void    SetTlvOffset(uint8_t aOffset) { mIteratorBuffer[kTlvPosition] = aOffset; }
        uint8_t GetEntryIndex(void) const { return mIteratorBuffer[kEntryPosition]; }
        void    SetEntryIndex(uint8_t aIndex) { mIteratorBuffer[kEntryPosition] = aIndex; }

        void SaveTlvOffset(const NetworkDataTlv *aTlv, const uint8_t *aTlvs)
        {
            SetTlvOffset(static_cast<uint8_t>(reinterpret_cast<const uint8_t *>(aTlv) - aTlvs));
        }

        void SaveSubTlvOffset(const NetworkDataTlv *aSubTlv, const NetworkDataTlv *aSubTlvs)
        {
            SetSubTlvOffset(static_cast<uint8_t>(reinterpret_cast<const uint8_t *>(aSubTlv) -
                                                 reinterpret_cast<const uint8_t *>(aSubTlvs)));
        }

        uint8_t *mIteratorBuffer;
    };

    struct Config
    {
        OnMeshPrefixConfig  *mOnMeshPrefix;
        ExternalRouteConfig *mExternalRoute;
        ServiceConfig       *mService;
        LowpanContextInfo   *mLowpanContext;
    };

    Error Iterate(Iterator &aIterator, uint16_t aRloc16, Config &aConfig) const;

    static bool MatchService(const ServiceTlv  &aServiceTlv,
                             uint32_t           aEnterpriseNumber,
                             const ServiceData &aServiceData,
                             ServiceMatchMode   aServiceMatchMode);

    static void AddRloc16ToRlocs(uint16_t aRloc16, Rlocs &aRlocs, RoleFilter aRoleFilter);

    const uint8_t *mTlvs;
    uint8_t        mLength;
};

/**
 * Represents mutable Network Data.
 */
class MutableNetworkData : public NetworkData
{
    friend class NetworkData;
    friend class Service::Manager;
    friend class Publisher;

public:
    /**
     * Initializes the `MutableNetworkData`
     *
     * @param[in] aInstance     A reference to the OpenThread instance.
     * @param[in] aTlvs         A pointer to the buffer to store the TLVs.
     * @param[in] aLength       The current length of the Network Data.
     * @param[in] aSize         Size of the buffer @p aTlvs (maximum length).
     */
    MutableNetworkData(Instance &aInstance, uint8_t *aTlvs, uint8_t aLength, uint8_t aSize)
        : NetworkData(aInstance, aTlvs, aLength)
        , mSize(aSize)
    {
    }

    using NetworkData::GetBytes;
    using NetworkData::GetLength;

    /**
     * Returns the size of the buffer to store the mutable Network Data.
     *
     * @returns The size of the buffer.
     */
    uint8_t GetSize(void) const { return mSize; }

    /**
     * Returns a pointer to start of the TLVs in `NetworkData`.
     *
     * @returns A pointer to start of the TLVs.
     */
    uint8_t *GetBytes(void) { return AsNonConst(AsConst(this)->GetBytes()); }

    /**
     * Clears the network data.
     */
    void Clear(void) { mLength = 0; }

protected:
    /**
     * Sets the Network Data length.
     *
     * @param[in] aLength   The length.
     */
    void SetLength(uint8_t aLength) { mLength = aLength; }

    using NetworkData::GetTlvsStart;

    /**
     * Returns a pointer to the start of Network Data TLV sequence.
     *
     * @returns A pointer to the start of Network Data TLV sequence.
     */
    NetworkDataTlv *GetTlvsStart(void) { return AsNonConst(AsConst(this)->GetTlvsStart()); }

    using NetworkData::GetTlvsEnd;

    /**
     * Returns a pointer to the end of Network Data TLV sequence.
     *
     * @returns A pointer to the end of Network Data TLV sequence.
     */
    NetworkDataTlv *GetTlvsEnd(void) { return AsNonConst(AsConst(this)->GetTlvsEnd()); }

    using NetworkData::FindPrefix;

    /**
     * Returns a pointer to a Prefix TLV.
     *
     * @param[in]  aPrefix        A pointer to an IPv6 prefix.
     * @param[in]  aPrefixLength  The prefix length pointed to by @p aPrefix (in bits).
     *
     * @returns A pointer to the Prefix TLV if one is found or `nullptr` if no matching Prefix TLV exists.
     */
    PrefixTlv *FindPrefix(const uint8_t *aPrefix, uint8_t aPrefixLength)
    {
        return AsNonConst(AsConst(this)->FindPrefix(aPrefix, aPrefixLength));
    }

    /**
     * Returns a pointer to a Prefix TLV.
     *
     * @param[in]  aPrefix        An IPv6 prefix.
     *
     * @returns A pointer to the Prefix TLV if one is found or `nullptr` if no matching Prefix TLV exists.
     */
    PrefixTlv *FindPrefix(const Ip6::Prefix &aPrefix) { return FindPrefix(aPrefix.GetBytes(), aPrefix.GetLength()); }

    using NetworkData::FindService;

    /**
     * Returns a pointer to a matching Service TLV.
     *
     * @param[in]  aEnterpriseNumber  Enterprise Number.
     * @param[in]  aServiceData       A Service Data.
     * @param[in]  aServiceMatchMode  The Service Data match mode.
     *
     * @returns A pointer to the Service TLV if one is found or `nullptr` if no matching Service TLV exists.
     */
    ServiceTlv *FindService(uint32_t           aEnterpriseNumber,
                            const ServiceData &aServiceData,
                            ServiceMatchMode   aServiceMatchMode)
    {
        return AsNonConst(AsConst(this)->FindService(aEnterpriseNumber, aServiceData, aServiceMatchMode));
    }

    /**
     * Indicates whether there is space in Network Data to insert/append new info and grow it by a given
     * number of bytes.
     *
     * @param[in]  aSize  The number of bytes to grow the Network Data.
     *
     * @retval TRUE   There is space to grow Network Data by @p aSize bytes.
     * @retval FALSE  There is no space left to grow Network Data by @p aSize bytes.
     */
    bool CanInsert(uint16_t aSize) const { return (mLength + aSize <= mSize); }

    /**
     * Grows the Network Data to append a TLV with a requested size.
     *
     * On success, the returned TLV is not initialized (i.e., the TLV Length field is not set) but the requested
     * size for it (@p aTlvSize number of bytes) is reserved in the Network Data.
     *
     * @param[in]  aTlvSize  The size of TLV (total number of bytes including Type, Length, and Value fields)
     *
     * @returns A pointer to the TLV if there is space to grow Network Data, or `nullptr` if no space to grow the
     *          Network Data with requested @p aTlvSize number of bytes.
     */
    NetworkDataTlv *AppendTlv(uint16_t aTlvSize);

    /**
     * Inserts bytes into the Network Data.
     *
     * @param[in]  aStart   A pointer to the beginning of the insertion.
     * @param[in]  aLength  The number of bytes to insert.
     */
    void Insert(void *aStart, uint8_t aLength);

    /**
     * Removes bytes from the Network Data.
     *
     * @param[in]  aRemoveStart   A pointer to the beginning of the removal.
     * @param[in]  aRemoveLength  The number of bytes to remove.
     */
    void Remove(void *aRemoveStart, uint8_t aRemoveLength);

    /**
     * Removes a TLV from the Network Data.
     *
     * @param[in]  aTlv   The TLV to remove.
     */
    void RemoveTlv(NetworkDataTlv *aTlv);

    /**
     * Strips non-stable data from the Thread Network Data.
     */
    void RemoveTemporaryData(void);

private:
    bool RemoveTemporaryDataIn(PrefixTlv &aPrefix);
    bool RemoveTemporaryDataIn(ServiceTlv &aService);

    uint8_t mSize;
};

} // namespace NetworkData

/**
 * @}
 */

} // namespace ot

#endif // NETWORK_DATA_HPP_
