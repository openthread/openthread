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
#include "common/clearable.hpp"
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
 * This class implements Network Data processing.
 *
 */
class NetworkData : public InstanceLocator
{
    friend class Service::Manager;

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
    void Clear(void) { mLength = 0; }

    /**
     * This method provides a full or stable copy of the Thread Network Data.
     *
     * @param[in]     aStable      TRUE when copying the stable version, FALSE when copying the full version.
     * @param[out]    aData        A pointer to the data buffer.
     * @param[inout]  aDataLength  On entry, size of the data buffer pointed to by @p aData.
     *                             On exit, number of copied bytes.
     *
     * @retval kErrorNone       Successfully copied full Thread Network Data.
     * @retval kErrorNoBufs     Not enough space to fully copy Thread Network Data.
     *
     */
    Error GetNetworkData(bool aStable, uint8_t *aData, uint8_t &aDataLength) const;

    /**
     * This method provides the next On Mesh prefix in the Thread Network Data.
     *
     * @param[inout]  aIterator  A reference to the Network Data iterator.
     * @param[out]    aConfig    A reference to a config variable where the On Mesh Prefix information will be placed.
     *
     * @retval kErrorNone       Successfully found the next On Mesh prefix.
     * @retval kErrorNotFound   No subsequent On Mesh prefix exists in the Thread Network Data.
     *
     */
    Error GetNextOnMeshPrefix(Iterator &aIterator, OnMeshPrefixConfig &aConfig) const;

    /**
     * This method provides the next On Mesh prefix in the Thread Network Data for a given RLOC16.
     *
     * @param[inout]  aIterator  A reference to the Network Data iterator.
     * @param[in]     aRloc16    The RLOC16 value.
     * @param[out]    aConfig    A reference to a config variable where the On Mesh Prefix information will be placed.
     *
     * @retval kErrorNone       Successfully found the next On Mesh prefix.
     * @retval kErrorNotFound   No subsequent On Mesh prefix exists in the Thread Network Data.
     *
     */
    Error GetNextOnMeshPrefix(Iterator &aIterator, uint16_t aRloc16, OnMeshPrefixConfig &aConfig) const;

    /**
     * This method provides the next external route in the Thread Network Data.
     *
     * @param[inout]  aIterator  A reference to the Network Data iterator.
     * @param[out]    aConfig    A reference to a config variable where the external route information will be placed.
     *
     * @retval kErrorNone       Successfully found the next external route.
     * @retval kErrorNotFound   No subsequent external route exists in the Thread Network Data.
     *
     */
    Error GetNextExternalRoute(Iterator &aIterator, ExternalRouteConfig &aConfig) const;

    /**
     * This method provides the next external route in the Thread Network Data for a given RLOC16.
     *
     * @param[inout]  aIterator  A reference to the Network Data iterator.
     * @param[in]     aRloc16    The RLOC16 value.
     * @param[out]    aConfig    A reference to a config variable where the external route information will be placed.
     *
     * @retval kErrorNone       Successfully found the next external route.
     * @retval kErrorNotFound   No subsequent external route exists in the Thread Network Data.
     *
     */
    Error GetNextExternalRoute(Iterator &aIterator, uint16_t aRloc16, ExternalRouteConfig &aConfig) const;

    /**
     * This method provides the next service in the Thread Network Data.
     *
     * @param[inout]  aIterator  A reference to the Network Data iterator.
     * @param[out]    aConfig    A reference to a config variable where the service information will be placed.
     *
     * @retval kErrorNone       Successfully found the next service.
     * @retval kErrorNotFound   No subsequent service exists in the Thread Network Data.
     *
     */
    Error GetNextService(Iterator &aIterator, ServiceConfig &aConfig) const;

    /**
     * This method provides the next service in the Thread Network Data for a given RLOC16.
     *
     * @param[inout]  aIterator  A reference to the Network Data iterator.
     * @param[in]     aRloc16    The RLOC16 value.
     * @param[out]    aConfig    A reference to a config variable where the service information will be placed.
     *
     * @retval kErrorNone       Successfully found the next service.
     * @retval kErrorNotFound   No subsequent service exists in the Thread Network Data.
     *
     */
    Error GetNextService(Iterator &aIterator, uint16_t aRloc16, ServiceConfig &aConfig) const;

    /**
     * This method provides the next Service ID in the Thread Network Data for a given RLOC16.
     *
     * @param[inout]  aIterator  A reference to the Network Data iterator.
     * @param[in]     aRloc16    The RLOC16 value.
     * @param[out]    aServiceId A reference to variable where the Service ID will be placed.
     *
     * @retval kErrorNone       Successfully found the next service.
     * @retval kErrorNotFound   No subsequent service exists in the Thread Network Data.
     *
     */
    Error GetNextServiceId(Iterator &aIterator, uint16_t aRloc16, uint8_t &aServiceId) const;

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
    bool ContainsOnMeshPrefixes(const NetworkData &aCompare, uint16_t aRloc16) const;

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
    bool ContainsExternalRoutes(const NetworkData &aCompare, uint16_t aRloc16) const;

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
    bool ContainsServices(const NetworkData &aCompare, uint16_t aRloc16) const;

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
    bool ContainsService(uint8_t aServiceId, uint16_t aRloc16) const;

    /**
     * This method provides the next server RLOC16 in the Thread Network Data.
     *
     * @param[inout]  aIterator  A reference to the Network Data iterator.
     * @param[out]    aRloc16    The RLOC16 value.
     *
     * @retval kErrorNone       Successfully found the next server.
     * @retval kErrorNotFound   No subsequent server exists in the Thread Network Data.
     *
     */
    Error GetNextServer(Iterator &aIterator, uint16_t &aRloc16) const;

protected:
    /**
     * This enumeration defines Service Data match mode.
     *
     */
    enum ServiceMatchMode : uint8_t
    {
        kServicePrefixMatch, ///< Match the Service Data by prefix.
        kServiceExactMatch,  ///< Match the full Service Data exactly.
    };

    /**
     * This method returns a pointer to the start of Network Data TLV sequence.
     *
     * @returns A pointer to the start of Network Data TLV sequence.
     *
     */
    NetworkDataTlv *GetTlvsStart(void) { return reinterpret_cast<NetworkDataTlv *>(mTlvs); }

    /**
     * This method returns a pointer to the start of Network Data TLV sequence.
     *
     * @returns A pointer to the start of Network Data TLV sequence.
     *
     */
    const NetworkDataTlv *GetTlvsStart(void) const { return reinterpret_cast<const NetworkDataTlv *>(mTlvs); }

    /**
     * This method returns a pointer to the end of Network Data TLV sequence.
     *
     * @returns A pointer to the end of Network Data TLV sequence.
     *
     */
    NetworkDataTlv *GetTlvsEnd(void) { return reinterpret_cast<NetworkDataTlv *>(mTlvs + mLength); }

    /**
     * This method returns a pointer to the end of Network Data TLV sequence.
     *
     * @returns A pointer to the end of Network Data TLV sequence.
     *
     */
    const NetworkDataTlv *GetTlvsEnd(void) const { return reinterpret_cast<const NetworkDataTlv *>(mTlvs + mLength); }

    /**
     * This method returns a pointer to a Prefix TLV.
     *
     * @param[in]  aPrefix        A pointer to an IPv6 prefix.
     * @param[in]  aPrefixLength  The prefix length pointed to by @p aPrefix (in bits).
     *
     * @returns A pointer to the Prefix TLV if one is found or nullptr if no matching Prefix TLV exists.
     *
     */
    PrefixTlv *FindPrefix(const uint8_t *aPrefix, uint8_t aPrefixLength)
    {
        return const_cast<PrefixTlv *>(const_cast<const NetworkData *>(this)->FindPrefix(aPrefix, aPrefixLength));
    }

    /**
     * This method returns a pointer to a Prefix TLV.
     *
     * @param[in]  aPrefix        A pointer to an IPv6 prefix.
     * @param[in]  aPrefixLength  The prefix length pointed to by @p aPrefix (in bits).
     *
     * @returns A pointer to the Prefix TLV if one is found or nullptr if no matching Prefix TLV exists.
     *
     */
    const PrefixTlv *FindPrefix(const uint8_t *aPrefix, uint8_t aPrefixLength) const;

    /**
     * This method returns a pointer to a Prefix TLV.
     *
     * @param[in]  aPrefix        An IPv6 prefix.
     *
     * @returns A pointer to the Prefix TLV if one is found or nullptr if no matching Prefix TLV exists.
     *
     */
    PrefixTlv *FindPrefix(const Ip6::Prefix &aPrefix) { return FindPrefix(aPrefix.GetBytes(), aPrefix.GetLength()); }

    /**
     * This method returns a pointer to a Prefix TLV.
     *
     * @param[in]  aPrefix        An IPv6 prefix.
     *
     * @returns A pointer to the Prefix TLV if one is found or nullptr if no matching Prefix TLV exists.
     *
     */
    const PrefixTlv *FindPrefix(const Ip6::Prefix &aPrefix) const
    {
        return FindPrefix(aPrefix.GetBytes(), aPrefix.GetLength());
    }

    /**
     * This method returns a pointer to a Prefix TLV in a specified tlvs buffer.
     *
     * @param[in]  aPrefix        A pointer to an IPv6 prefix.
     * @param[in]  aPrefixLength  The prefix length pointed to by @p aPrefix (in bits).
     * @param[in]  aTlvs          A pointer to a specified tlvs buffer.
     * @param[in]  aTlvsLength    The specified tlvs buffer length pointed to by @p aTlvs.
     *
     * @returns A pointer to the Prefix TLV if one is found or nullptr if no matching Prefix TLV exists.
     *
     */
    static PrefixTlv *FindPrefix(const uint8_t *aPrefix, uint8_t aPrefixLength, uint8_t *aTlvs, uint8_t aTlvsLength)
    {
        return const_cast<PrefixTlv *>(
            FindPrefix(aPrefix, aPrefixLength, const_cast<const uint8_t *>(aTlvs), aTlvsLength));
    }

    /**
     * This method returns a pointer to a Prefix TLV in a specified tlvs buffer.
     *
     * @param[in]  aPrefix        A pointer to an IPv6 prefix.
     * @param[in]  aPrefixLength  The prefix length pointed to by @p aPrefix (in bits).
     * @param[in]  aTlvs          A pointer to a specified tlvs buffer.
     * @param[in]  aTlvsLength    The specified tlvs buffer length pointed to by @p aTlvs.
     *
     * @returns A pointer to the Prefix TLV if one is found or nullptr if no matching Prefix TLV exists.
     *
     */
    static const PrefixTlv *FindPrefix(const uint8_t *aPrefix,
                                       uint8_t        aPrefixLength,
                                       const uint8_t *aTlvs,
                                       uint8_t        aTlvsLength);

    /**
     * This method returns a pointer to a matching Service TLV.
     *
     * @param[in]  aEnterpriseNumber  Enterprise Number.
     * @param[in]  aServiceData       A pointer to a Service Data.
     * @param[in]  aServiceDataLength The Service Data length pointed to by @p aServiceData.
     * @param[in]  aServiceMatchMode  The Service Data match mode.
     *
     * @returns A pointer to the Service TLV if one is found or nullptr if no matching Service TLV exists.
     *
     */
    ServiceTlv *FindService(uint32_t         aEnterpriseNumber,
                            const uint8_t *  aServiceData,
                            uint8_t          aServiceDataLength,
                            ServiceMatchMode aServiceMatchMode)
    {
        return const_cast<ServiceTlv *>(const_cast<const NetworkData *>(this)->FindService(
            aEnterpriseNumber, aServiceData, aServiceDataLength, aServiceMatchMode));
    }

    /**
     * This method returns a pointer to a matching Service TLV.
     *
     * @param[in]  aEnterpriseNumber  Enterprise Number.
     * @param[in]  aServiceData       A pointer to a Service Data.
     * @param[in]  aServiceDataLength The Service Data length pointed to by @p aServiceData.
     * @param[in]  aServiceMatchMode  The Service Data match mode.
     *
     * @returns A pointer to the Service TLV if one is found or nullptr if no matching Service TLV exists.
     *
     */
    const ServiceTlv *FindService(uint32_t         aEnterpriseNumber,
                                  const uint8_t *  aServiceData,
                                  uint8_t          aServiceDataLength,
                                  ServiceMatchMode aServiceMatchMode) const;

    /**
     * This method returns a pointer to a Service TLV in a specified tlvs buffer.
     *
     * @param[in]  aEnterpriseNumber  Enterprise Number.
     * @param[in]  aServiceData       A pointer to a Service Data.
     * @param[in]  aServiceDataLength The Service Data length pointed to by @p aServiceData.
     * @param[in]  aServiceMatchMode  The Service Data match mode.
     * @param[in]  aTlvs              A pointer to a specified tlvs buffer.
     * @param[in]  aTlvsLength        The specified tlvs buffer length pointed to by @p aTlvs.
     *
     * @returns A pointer to the Service TLV if one is found or nullptr if no matching Service TLV exists.
     *
     */
    static ServiceTlv *FindService(uint32_t         aEnterpriseNumber,
                                   const uint8_t *  aServiceData,
                                   uint8_t          aServiceDataLength,
                                   ServiceMatchMode aServiceMatchMode,
                                   uint8_t *        aTlvs,
                                   uint8_t          aTlvsLength)
    {
        return const_cast<ServiceTlv *>(FindService(aEnterpriseNumber, aServiceData, aServiceDataLength,
                                                    aServiceMatchMode, const_cast<const uint8_t *>(aTlvs),
                                                    aTlvsLength));
    }

    /**
     * This method returns a pointer to a Service TLV in a specified tlvs buffer.
     *
     * @param[in]  aEnterpriseNumber  Enterprise Number.
     * @param[in]  aServiceData       A pointer to a Service Data.
     * @param[in]  aServiceDataLength The Service Data length pointed to by @p aServiceData.
     * @param[in]  aServiceMatchMode  The Service Data match mode.
     * @param[in]  aTlvs              A pointer to a specified tlvs buffer.
     * @param[in]  aTlvsLength        The specified tlvs buffer length pointed to by @p aTlvs.
     *
     * @returns A pointer to the Service TLV if one is found or nullptr if no matching Service TLV exists.
     *
     */
    static const ServiceTlv *FindService(uint32_t         aEnterpriseNumber,
                                         const uint8_t *  aServiceData,
                                         uint8_t          aServiceDataLength,
                                         ServiceMatchMode aServiceMatchMode,
                                         const uint8_t *  aTlvs,
                                         uint8_t          aTlvsLength);

    /**
     * This method returns the next pointer to a matching Service TLV.
     *
     * This method can be used to iterate over all Service TLVs that start with a given Service Data.
     *
     * @param[in]  aPrevServiceTlv    Set to nullptr to start from the beginning of the TLVs (finding the first matching
     *                                Service TLV), or a pointer to the previous Service TLV returned from this method
     *                                to iterate to the next matching Service TLV.
     * @param[in]  aEnterpriseNumber  Enterprise Number.
     * @param[in]  aServiceData       A pointer to a Service Data to match with Service TLVs.
     * @param[in]  aServiceDataLength The Service Data length pointed to by @p aServiceData.
     * @param[in]  aServiceMatchMode  The Service Data match mode.
     *
     * @returns A pointer to the next matching Service TLV if one is found or nullptr if it cannot be found.
     *
     */
    const ServiceTlv *FindNextService(const ServiceTlv *aPrevServiceTlv,
                                      uint32_t          aEnterpriseNumber,
                                      const uint8_t *   aServiceData,
                                      uint8_t           aServiceDataLength,
                                      ServiceMatchMode  aServiceMatchMode) const;

    /**
     * This method indicates whether there is space in Network Data to insert/append new info and grow it by a given
     * number of bytes.
     *
     * @param[in]  aSize  The number of bytes to grow the Network Data.
     *
     * @retval TRUE   There is space to grow Network Data by @p aSize bytes.
     * @retval FALSE  There is no space left to grow Network Data by @p aSize bytes.
     *
     */
    bool CanInsert(uint16_t aSize) const { return (mLength + aSize <= kMaxSize); }

    /**
     * This method grows the Network Data to append a TLV with a requested size.
     *
     * On success, the returned TLV is not initialized (i.e., the TLV Length field is not set) but the requested
     * size for it (@p aTlvSize number of bytes) is reserved in the Network Data.
     *
     * @param[in]  aTlvSize  The size of TLV (total number of bytes including Type, Length, and Value fields)
     *
     * @returns A pointer to the TLV if there is space to grow Network Data, or nullptr if no space to grow the Network
     *          Data with requested @p aTlvSize number of bytes.
     *
     */
    NetworkDataTlv *AppendTlv(uint16_t aTlvSize);

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
    static void RemoveTemporaryData(uint8_t *aData, uint8_t &aDataLength);

    /**
     * This method sends a Server Data Notification message to the Leader.
     *
     * @param[in]  aRloc16   The old RLOC16 value that was previously registered.
     * @param[in]  aHandler  A function pointer that is called when the transaction ends.
     * @param[in]  aContext  A pointer to arbitrary context information.
     *
     * @retval kErrorNone     Successfully enqueued the notification message.
     * @retval kErrorNoBufs   Insufficient message buffers to generate the notification message.
     *
     */
    Error SendServerDataNotification(uint16_t aRloc16, Coap::ResponseHandler aHandler, void *aContext);

    uint8_t mTlvs[kMaxSize]; ///< The Network Data buffer.
    uint8_t mLength;         ///< The number of valid bytes in @var mTlvs.

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

        void AdvaceSubTlv(const NetworkDataTlv *aSubTlvs)
        {
            SaveSubTlvOffset(GetSubTlv(aSubTlvs)->GetNext(), aSubTlvs);
            SetEntryIndex(0);
        }

        uint8_t GetAndAdvanceIndex(void) { return mIteratorBuffer[kEntryPosition]++; }

        bool IsNewEntry(void) const { return GetEntryIndex() == 0; }
        void MarkEntryAsNotNew(void) { SetEntryIndex(1); }

    private:
        enum
        {
            kTlvPosition    = 0,
            kSubTlvPosition = 1,
            kEntryPosition  = 2,
        };

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
        OnMeshPrefixConfig * mOnMeshPrefix;
        ExternalRouteConfig *mExternalRoute;
        ServiceConfig *      mService;
    };

    Error Iterate(Iterator &aIterator, uint16_t aRloc16, Config &aConfig) const;

    static void RemoveTemporaryData(uint8_t *aData, uint8_t &aDataLength, PrefixTlv &aPrefix);
    static void RemoveTemporaryData(uint8_t *aData, uint8_t &aDataLength, ServiceTlv &aService);

    static void Remove(uint8_t *aData, uint8_t &aDataLength, uint8_t *aRemoveStart, uint8_t aRemoveLength);
    static void RemoveTlv(uint8_t *aData, uint8_t &aDataLength, NetworkDataTlv *aTlv);

    static bool MatchService(const ServiceTlv &aServiceTlv,
                             uint32_t          aEnterpriseNumber,
                             const uint8_t *   aServiceData,
                             uint8_t           aServiceDataLength,
                             ServiceMatchMode  aServiceMatchMode);

    const Type mType;
};

} // namespace NetworkData

/**
 * @}
 */

} // namespace ot

#endif // NETWORK_DATA_HPP_
