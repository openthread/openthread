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
#include <openthread/server.h>

#include "coap/coap.hpp"
#include "common/locator.hpp"
#include "common/timer.hpp"
#include "net/udp6.hpp"
#include "thread/lowpan.hpp"
#include "thread/mle_router.hpp"
#include "thread/network_data_tlvs.hpp"

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
 *
 */

/**
 * @namespace ot::NetworkData
 *
 * @brief
 *   This namespace includes definitions for managing Thread Network Data.
 *
 */
namespace NetworkData {

/**
 * @addtogroup core-netdata-core
 *
 * @brief
 *   This module includes definitions for managing Thread Network Data.
 *
 * @{
 *
 */

enum
{
    kIteratorInit = OT_NETWORK_DATA_ITERATOR_INIT, ///< Initializer for `Iterator` type.
};

/**
 * This type represents a Iterator used to iterate through Network Data info (e.g., see `GetNextOnMeshPrefix()`)
 *
 */
typedef otNetworkDataIterator Iterator;

/**
 * This type represents an On Mesh Prefix (Border Router) configuration.
 *
 */
typedef otBorderRouterConfig OnMeshPrefixConfig;

/**
 * This type represents an External Route configuration.
 *
 */
typedef otExternalRouteConfig ExternalRouteConfig;

/**
 * This type represents a Service configuration.
 *
 */
typedef otServiceConfig ServiceConfig;

/**
 * This class implements Network Data processing.
 *
 */
class NetworkData : public InstanceLocator
{
public:
    enum
    {
        kMaxSize = 254, ///< Maximum size of Thread Network Data in bytes.
    };

    /**
     * This enumeration specifies the type of Network Data (local or leader).
     *
     */
    enum Type
    {
        kTypeLocal,  ///< Local Network Data.
        kTypeLeader, ///< Leader Network Data.
    };

    /**
     * This constructor initializes the object.
     *
     * @param[in]  aInstance     A reference to the OpenThread instance.
     * @param[in]  aType         Network data type
     *
     */
    NetworkData(Instance &aInstance, Type aType);

    /**
     * This method clears the network data.
     *
     */
    void Clear(void);

    /**
     * This method provides a full or stable copy of the Thread Network Data.
     *
     * @param[in]     aStable      TRUE when copying the stable version, FALSE when copying the full version.
     * @param[out]    aData        A pointer to the data buffer.
     * @param[inout]  aDataLength  On entry, size of the data buffer pointed to by @p aData.
     *                             On exit, number of copied bytes.
     *
     * @retval OT_ERROR_NONE       Successfully copied full Thread Network Data.
     * @retval OT_ERROR_NO_BUFS    Not enough space to fully copy Thread Network Data.
     *
     */
    otError GetNetworkData(bool aStable, uint8_t *aData, uint8_t &aDataLength);

    /**
     * This method provides the next On Mesh prefix in the Thread Network Data.
     *
     * @param[inout]  aIterator  A reference to the Network Data iterator.
     * @param[out]    aConfig    A reference to a config variable where the On Mesh Prefix information will be placed.
     *
     * @retval OT_ERROR_NONE       Successfully found the next On Mesh prefix.
     * @retval OT_ERROR_NOT_FOUND  No subsequent On Mesh prefix exists in the Thread Network Data.
     *
     */
    otError GetNextOnMeshPrefix(Iterator &aIterator, OnMeshPrefixConfig &aConfig);

    /**
     * This method provides the next On Mesh prefix in the Thread Network Data for a given RLOC16.
     *
     * @param[inout]  aIterator  A reference to the Network Data iterator.
     * @param[in]     aRloc16    The RLOC16 value.
     * @param[out]    aConfig    A reference to a config variable where the On Mesh Prefix information will be placed.
     *
     * @retval OT_ERROR_NONE       Successfully found the next On Mesh prefix.
     * @retval OT_ERROR_NOT_FOUND  No subsequent On Mesh prefix exists in the Thread Network Data.
     *
     */
    otError GetNextOnMeshPrefix(Iterator &aIterator, uint16_t aRloc16, OnMeshPrefixConfig &aConfig);

    /**
     * This method provides the next external route in the Thread Network Data.
     *
     * @param[inout]  aIterator  A reference to the Network Data iterator.
     * @param[out]    aConfig    A reference to a config variable where the external route information will be placed.
     *
     * @retval OT_ERROR_NONE       Successfully found the next external route.
     * @retval OT_ERROR_NOT_FOUND  No subsequent external route exists in the Thread Network Data.
     *
     */
    otError GetNextExternalRoute(Iterator &aIterator, ExternalRouteConfig &aConfig);

    /**
     * This method provides the next external route in the Thread Network Data for a given RLOC16.
     *
     * @param[inout]  aIterator  A reference to the Network Data iterator.
     * @param[in]     aRloc16    The RLOC16 value.
     * @param[out]    aConfig    A reference to a config variable where the external route information will be placed.
     *
     * @retval OT_ERROR_NONE       Successfully found the next external route.
     * @retval OT_ERROR_NOT_FOUND  No subsequent external route exists in the Thread Network Data.
     *
     */
    otError GetNextExternalRoute(Iterator &aIterator, uint16_t aRloc16, ExternalRouteConfig &aConfig);

    /**
     * This method provides the next service in the Thread Network Data.
     *
     * @param[inout]  aIterator  A reference to the Network Data iterator.
     * @param[out]    aConfig    A reference to a config variable where the service information will be placed.
     *
     * @retval OT_ERROR_NONE       Successfully found the next service.
     * @retval OT_ERROR_NOT_FOUND  No subsequent service exists in the Thread Network Data.
     *
     */
    otError GetNextService(Iterator &aIterator, ServiceConfig &aConfig);

    /**
     * This method provides the next service in the Thread Network Data for a given RLOC16.
     *
     * @param[inout]  aIterator  A reference to the Network Data iterator.
     * @param[in]     aRloc16    The RLOC16 value.
     * @param[out]    aConfig    A reference to a config variable where the service information will be placed.
     *
     * @retval OT_ERROR_NONE       Successfully found the next service.
     * @retval OT_ERROR_NOT_FOUND  No subsequent service exists in the Thread Network Data.
     *
     */
    otError GetNextService(Iterator &aIterator, uint16_t aRloc16, ServiceConfig &aConfig);

    /**
     * This method provides the next Service ID in the Thread Network Data for a given RLOC16.
     *
     * @param[inout]  aIterator  A reference to the Network Data iterator.
     * @param[in]     aRloc16    The RLOC16 value.
     * @param[out]    aServiceId A reference to variable where the Service ID will be placed.
     *
     * @retval OT_ERROR_NONE       Successfully found the next service.
     * @retval OT_ERROR_NOT_FOUND  No subsequent service exists in the Thread Network Data.
     *
     */
    otError GetNextServiceId(Iterator &aIterator, uint16_t aRloc16, uint8_t &aServiceId);

    /**
     * This method indicates whether or not the Thread Network Data contains all of the on mesh prefix information
     * in @p aCompare associated with @p aRloc16.
     *
     * @param[in]  aCompare  The Network Data to use for the query.
     * @param[in]  aRloc16   The RLOC16 to consider.
     *
     * @returns TRUE if this object contains all on mesh prefix information in @p aCompare associated with @p aRloc16,
     *          FALSE otherwise.
     *
     */
    bool ContainsOnMeshPrefixes(NetworkData &aCompare, uint16_t aRloc16);

    /**
     * This method indicates whether or not the Thread Network Data contains all of the external route information
     * in @p aCompare associated with @p aRloc16.
     *
     * @param[in]  aCompare  The Network Data to use for the query.
     * @param[in]  aRloc16   The RLOC16 to consider.
     *
     * @returns TRUE if this object contains all external route information in @p aCompare associated with @p aRloc16,
     *          FALSE otherwise.
     *
     */
    bool ContainsExternalRoutes(NetworkData &aCompare, uint16_t aRloc16);

    /**
     * This method indicates whether or not the Thread Network Data contains all of the service information
     * in @p aCompare associated with @p aRloc16.
     *
     * @param[in]  aCompare  The Network Data to use for the query.
     * @param[in]  aRloc16   The RLOC16 to consider.
     *
     * @returns TRUE if this object contains all service information in @p aCompare associated with @p aRloc16,
     *          FALSE otherwise.
     *
     */
    bool ContainsServices(NetworkData &aCompare, uint16_t aRloc16);

    /**
     * This method indicates whether or not the Thread Network Data contains the service with given Service ID
     * associated with @p aRloc16.
     *
     * @param[in]  aServiceId The Service ID to search for.
     * @param[in]  aRloc16    The RLOC16 to consider.
     *
     * @returns TRUE if this object contains the service with given ID associated with @p aRloc16,
     *          FALSE otherwise.
     *
     */
    bool ContainsService(uint8_t aServiceId, uint16_t aRloc16);

    /**
     * This method provides the next server RLOC16 in the Thread Network Data.
     *
     * @param[inout]  aIterator  A reference to the Network Data iterator.
     * @param[out]    aRloc16    The RLOC16 value.
     *
     * @retval OT_ERROR_NONE       Successfully found the next server.
     * @retval OT_ERROR_NOT_FOUND  No subsequent server exists in the Thread Network Data.
     *
     */
    otError GetNextServer(Iterator &aIterator, uint16_t &aRloc16);

    /**
     * This method cancels the data resubmit delay timer.
     *
     */
    void ClearResubmitDelayTimer(void);

protected:
    /**
     * This method returns a pointer to the Border Router TLV within a given Prefix TLV.
     *
     * @param[in]  aPrefix  A reference to the Prefix TLV.
     *
     * @returns A pointer to the Border Router TLV if one is found or NULL if no Border Router TLV exists.
     *
     */
    BorderRouterTlv *FindBorderRouter(PrefixTlv &aPrefix);

    /**
     * This method returns a pointer to the stable or non-stable Border Router TLV within a given Prefix TLV.
     *
     * @param[in]  aPrefix  A reference to the Prefix TLV.
     * @param[in]  aStable  TRUE if requesting a stable Border Router TLV, FALSE otherwise.
     *
     * @returns A pointer to the Border Router TLV if one is found or NULL if no Border Router TLV exists.
     *
     */
    BorderRouterTlv *FindBorderRouter(PrefixTlv &aPrefix, bool aStable);

    /**
     * This method returns a pointer to the Has Route TLV within a given Prefix TLV.
     *
     * @param[in]  aPrefix  A reference to the Prefix TLV.
     *
     * @returns A pointer to the Has Route TLV if one is found or NULL if no Has Route TLV exists.
     *
     */
    HasRouteTlv *FindHasRoute(PrefixTlv &aPrefix);

    /**
     * This method returns a pointer to the stable or non-stable Has Route TLV within a given Prefix TLV.
     *
     * @param[in]  aPrefix  A reference to the Prefix TLV.
     * @param[in]  aStable  TRUE if requesting a stable Has Route TLV, FALSE otherwise.
     *
     * @returns A pointer to the Has Route TLV if one is found or NULL if no Has Route TLV exists.
     *
     */
    HasRouteTlv *FindHasRoute(PrefixTlv &aPrefix, bool aStable);

    /**
     * This method returns a pointer to the Context TLV within a given Prefix TLV.
     *
     * @param[in]  aPrefix  A reference to the Prefix TLV.
     *
     * @returns A pointer to the Context TLV is one is found or NULL if no Context TLV exists.
     *
     */
    ContextTlv *FindContext(PrefixTlv &aPrefix);

    /**
     * This method returns a pointer to a Prefix TLV.
     *
     * @param[in]  aPrefix        A pointer to an IPv6 prefix.
     * @param[in]  aPrefixLength  The prefix length pointed to by @p aPrefix.
     *
     * @returns A pointer to the Prefix TLV is one is found or NULL if no matching Prefix TLV exists.
     *
     */
    PrefixTlv *FindPrefix(const uint8_t *aPrefix, uint8_t aPrefixLength);

    /**
     * This method returns a pointer to a Prefix TLV in a specified tlvs buffer.
     *
     * @param[in]  aPrefix        A pointer to an IPv6 prefix.
     * @param[in]  aPrefixLength  The prefix length pointed to by @p aPrefix (in bits).
     * @param[in]  aTlvs          A pointer to a specified tlvs buffer.
     * @param[in]  aTlvsLength    The specified tlvs buffer length pointed to by @p aTlvs.
     *
     * @returns A pointer to the Prefix TLV is one is found or NULL if no matching Prefix TLV exists.
     *
     */
    PrefixTlv *FindPrefix(const uint8_t *aPrefix, uint8_t aPrefixLength, uint8_t *aTlvs, uint8_t aTlvsLength);

    /**
     * This method returns a pointer to a matching Service TLV.
     *
     * @param[in]  aEnterpriseNumber  Enterprise Number.
     * @param[in]  aServiceData       A pointer to a Service Data.
     * @param[in]  aServiceDataLength The Service Data length pointed to by @p aServiceData.
     *
     * @returns A pointer to the Service TLV is one is found or NULL if no matching Service TLV exists.
     *
     */
    ServiceTlv *FindService(uint32_t aEnterpriseNumber, const uint8_t *aServiceData, uint8_t aServiceDataLength);

    /**
     * This method returns a pointer to a Service TLV in a specified tlvs buffer.
     *
     * @param[in]  aEnterpriseNumber  Enterprise Number.
     * @param[in]  aServiceData       A pointer to an Service Data.
     * @param[in]  aServiceDataLength The Service Data length pointed to by @p aServiceData.
     * @param[in]  aTlvs              A pointer to a specified tlvs buffer.
     * @param[in]  aTlvsLength        The specified tlvs buffer length pointed to by @p aTlvs.
     *
     * @returns A pointer to the Service TLV is one is found or NULL if no matching Service TLV exists.
     *
     */
    ServiceTlv *FindService(uint32_t       aEnterpriseNumber,
                            const uint8_t *aServiceData,
                            uint8_t        aServiceDataLength,
                            uint8_t *      aTlvs,
                            uint8_t        aTlvsLength);

    /**
     * This method grows the Network Data to append a TLV with a requested size.
     *
     * On success, the returned TLV is not initialized (i.e., the TLV Length field is not set) but the requested
     * size for it (@p aTlvSize number of bytes) is reserved in the Network Data.
     *
     * @param[in]  aTlvSize  The size of TLV (total number of bytes including Type, Length, and Value fields)
     *
     * @returns A pointer to the TLV if there is space to grow Network Data, or NULL if no space to grow the Network
     *          Data with requested @p aTlvSize number of bytes.
     *
     */
    NetworkDataTlv *AppendTlv(uint8_t aTlvSize);

    /**
     * This method inserts bytes into the Network Data.
     *
     * @param[in]  aStart   A pointer to the beginning of the insertion.
     * @param[in]  aLength  The number of bytes to insert.
     *
     */
    void Insert(void *aStart, uint8_t aLength);

    /**
     * This method removes bytes from the Network Data.
     *
     * @param[in]  aRemoveStart   A pointer to the beginning of the removal.
     * @param[in]  aRemoveLength  The number of bytes to remove.
     *
     */
    void Remove(void *aRemoveStart, uint8_t aRemoveLength);

    /**
     * This method removes a TLV from the Network Data.
     *
     * @param[in]  aTlv   The TLV to remove.
     *
     */
    void RemoveTlv(NetworkDataTlv *aTlv);

    /**
     * This method strips non-stable data from the Thread Network Data.
     *
     * @param[inout]  aData        A pointer to the Network Data to modify.
     * @param[inout]  aDataLength  On entry, the size of the Network Data in bytes.  On exit, the size of the
     *                             resulting Network Data in bytes.
     *
     */
    void RemoveTemporaryData(uint8_t *aData, uint8_t &aDataLength);

    /**
     * This method strips non-stable Sub-TLVs from a Prefix TLV.
     *
     * @param[inout]  aData        A pointer to the Network Data to modify.
     * @param[inout]  aDataLength  On entry, the size of the Network Data in bytes.  On exit, the size of the
     *                             resulting Network Data in bytes.
     * @param[inout]  aPrefix      A reference to the Prefix TLV to modify.
     *
     */
    void RemoveTemporaryData(uint8_t *aData, uint8_t &aDataLength, PrefixTlv &aPrefix);

    /**
     * This method strips non-stable Sub-TLVs from a Service TLV.
     *
     * @param[inout]  aData        A pointer to the Network Data to modify.
     * @param[inout]  aDataLength  On entry, the size of the Network Data in bytes.  On exit, the size of the
     *                             resulting Network Data in bytes.
     * @param[inout]  aService     A reference to the Service TLV to modify.
     *
     */
    void RemoveTemporaryData(uint8_t *aData, uint8_t &aDataLength, ServiceTlv &aService);

    /**
     * This method computes the number of IPv6 Prefix bits that match.
     *
     * @param[in]  a        A pointer to the first IPv6 Prefix.
     * @param[in]  b        A pointer to the second IPv6 prefix.
     * @param[in]  aLength  The maximum length in bits to compare.
     *
     * @returns The number of matching bits.
     *
     */
    int8_t PrefixMatch(const uint8_t *a, const uint8_t *b, uint8_t aLength);

    /**
     * This method sends a Server Data Notification message to the Leader.
     *
     * @param[in]  aRloc16  The old RLOC16 value that was previously registered.
     *
     * @retval OT_ERROR_NONE     Successfully enqueued the notification message.
     * @retval OT_ERROR_NO_BUFS  Insufficient message buffers to generate the notification message.
     *
     */
    otError SendServerDataNotification(uint16_t aRloc16);

    /**
     * This static method searches in a given sequence of TLVs to find the first TLV with a given TLV Type.
     *
     * @param[in]  aStart  A pointer to the start of the sequence of TLVs to search within.
     * @param[in]  aEnd    A pointer to the end of the sequence of TLVs.
     * @param[in]  aType   The TLV type to find.
     *
     * @returns A pointer to the TLV if found, or NULL if not found.
     *
     */
    static NetworkDataTlv *FindTlv(NetworkDataTlv *aStart, NetworkDataTlv *aEnd, NetworkDataTlv::Type aType);

    /**
     * This static method searches in a given sequence of TLVs to find the first TLV with a given TLV Type and stable
     * flag.
     *
     * @param[in]  aStart  A pointer to the start of the sequence of TLVs to search within.
     * @param [in] aEnd    A pointer to the end of the sequence of TLVs.
     * @param[in]  aType   The TLV type to find.
     * @param[in]  aStable TRUE if to find a stable TLV, FALSE otherwise.
     *
     * @returns A pointer to the TLV if found, or NULL if not found.
     *
     */
    static NetworkDataTlv *FindTlv(NetworkDataTlv *     aStart,
                                   NetworkDataTlv *     aEnd,
                                   NetworkDataTlv::Type aType,
                                   bool                 aStable);

    /**
     * This method returns a pointer to the end of Network Data TLV sequence.
     *
     * @returns A pointer to the end of Network Data TLV sequence.
     *
     */
    NetworkDataTlv *GetTlvsEnd(void) { return reinterpret_cast<NetworkDataTlv *>(mTlvs + mLength); }

    uint8_t mTlvs[kMaxSize]; ///< The Network Data buffer.
    uint8_t mLength;         ///< The number of valid bytes in @var mTlvs.

private:
    enum
    {
        kDataResubmitDelay  = 300000, ///< DATA_RESUBMIT_DELAY (milliseconds) if the device itself is the server.
        kProxyResubmitDelay = 5000,   ///< Resubmit delay (milliseconds) if deregister as the child server proxy.

    };

    class NetworkDataIterator
    {
    public:
        enum Type
        {
            kTypeOnMeshPrefix  = 0,
            kTypeExternalRoute = 1,
            kTypeService       = 2,
        };

        explicit NetworkDataIterator(Iterator &aIterator)
            : mIteratorBuffer(reinterpret_cast<uint8_t *>(&aIterator))
        {
        }

        uint8_t GetTlvOffset(void) const { return mIteratorBuffer[kTlvPosition]; }
        uint8_t GetSubTlvOffset(void) const { return mIteratorBuffer[kSubTlvPosition]; }
        uint8_t GetEntryIndex(void) const { return mIteratorBuffer[kEntryPosition]; }
        Type    GetType(void) const { return static_cast<Type>(mIteratorBuffer[kTypePosition]); }
        void    SetTlvOffset(uint8_t aOffset) { mIteratorBuffer[kTlvPosition] = aOffset; }
        void    SetSubTlvOffset(uint8_t aOffset) { mIteratorBuffer[kSubTlvPosition] = aOffset; }
        void    SetEntryIndex(uint8_t aIndex) { mIteratorBuffer[kEntryPosition] = aIndex; }
        void    SetType(Type aType) { mIteratorBuffer[kTypePosition] = static_cast<uint8_t>(aType); }

        bool IsNewEntry(void) const { return GetEntryIndex() == 0; }
        void MarkEntryAsNotnew(void) { SetEntryIndex(1); }

        NetworkDataTlv *GetTlv(uint8_t *aTlvs) const
        {
            return reinterpret_cast<NetworkDataTlv *>(aTlvs + GetTlvOffset());
        }

        NetworkDataTlv *GetSubTlv(NetworkDataTlv *aSubTlvs)
        {
            return reinterpret_cast<NetworkDataTlv *>(reinterpret_cast<uint8_t *>(aSubTlvs) + GetSubTlvOffset());
        }

        void SaveTlvOffset(const NetworkDataTlv *aTlv, const uint8_t *aTlvs)
        {
            SetTlvOffset(static_cast<uint8_t>(reinterpret_cast<const uint8_t *>(aTlv) - aTlvs));
        }

        void SaveSubTlvOffset(const NetworkDataTlv *aSubTlv, const NetworkDataTlv *aSubTlvs)
        {
            SetSubTlvOffset(static_cast<uint8_t>(reinterpret_cast<const uint8_t *>(aSubTlv) -
                                                 reinterpret_cast<const uint8_t *>(aSubTlvs)));
        }

    private:
        enum
        {
            kTlvPosition    = 0,
            kSubTlvPosition = 1,
            kEntryPosition  = 2,
            kTypePosition   = 3,
        };

        uint8_t *mIteratorBuffer;
    };

    static void Remove(uint8_t *aData, uint8_t &aDataLength, uint8_t *aRemoveStart, uint8_t aRemoveLength);
    static void RemoveTlv(uint8_t *aData, uint8_t &aDataLength, NetworkDataTlv *aTlv);

    NetworkDataTlv *FindTlv(NetworkDataIterator &aIterator, NetworkDataTlv::Type aTlvType);
    void            IterateToNextTlv(NetworkDataIterator &aIterator);
    NetworkDataTlv *FindSubTlv(NetworkDataIterator &aIterator,
                               NetworkDataTlv::Type aSubTlvType,
                               NetworkDataTlv *     aSubTlvs,
                               NetworkDataTlv *     aSubTlvsEnd);
    void            IterateToNextSubTlv(NetworkDataIterator &aIterator, NetworkDataTlv *aSubTlvs);

    const Type mType;
    TimeMilli  mLastAttempt;
};

} // namespace NetworkData

/**
 * @}
 */

} // namespace ot

#endif // NETWORK_DATA_HPP_
