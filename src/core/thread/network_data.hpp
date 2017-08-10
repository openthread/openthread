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

#include <openthread/types.h>

#include "coap/coap.hpp"
#include "common/locator.hpp"
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

/**
 * This class implements Network Data processing.
 *
 */
class NetworkData: public ThreadNetifLocator
{
public:
    enum
    {
        kMaxSize = 255,  ///< Maximum size of Thread Network Data in bytes.
    };

    /**
     * This constructor initializes the object.
     *
     * @param[in]  aThreadNetif  A reference to the Thread network interface.
     * @param[in]  aLocal        TRUE if this represents local network data, FALSE otherwise.
     *
     */
    NetworkData(ThreadNetif &aThreadNetif, bool aLocal);

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
     */
    void GetNetworkData(bool aStable, uint8_t *aData, uint8_t &aDataLength);

    /**
     * This method provides the next On Mesh prefix in the Thread Network Data.
     *
     * @param[inout]  aIterator  A pointer to the Network Data iterator context.
     * @param[out]    aConfig    A pointer to where the On Mesh Prefix information will be placed.
     *
     * @retval OT_ERROR_NONE       Successfully found the next On Mesh prefix.
     * @retval OT_ERROR_NOT_FOUND  No subsequent On Mesh prefix exists in the Thread Network Data.
     *
     */
    otError GetNextOnMeshPrefix(otNetworkDataIterator *aIterator, otBorderRouterConfig *aConfig);

    /**
     * This method provides the next On Mesh prefix in the Thread Network Data for a given RLOC16.
     *
     * @param[inout]  aIterator  A pointer to the Network Data iterator context.
     * @param[in]     aRloc16    The RLOC16 value.
     * @param[out]    aConfig    A pointer to where the On Mesh Prefix information will be placed.
     *
     * @retval OT_ERROR_NONE       Successfully found the next On Mesh prefix.
     * @retval OT_ERROR_NOT_FOUND  No subsequent On Mesh prefix exists in the Thread Network Data.
     *
     */
    otError GetNextOnMeshPrefix(otNetworkDataIterator *aIterator, uint16_t aRloc16, otBorderRouterConfig *aConfig);

    /**
     * This method provides the next external route in the Thread Network Data.
     *
     * @param[inout]  aIterator  A pointer to the Network Data iterator context.
     * @param[out]    aConfig    A pointer to where the external route information will be placed.
     *
     * @retval OT_ERROR_NONE       Successfully found the next external route.
     * @retval OT_ERROR_NOT_FOUND  No subsequent external route exists in the Thread Network Data.
     *
     */
    otError GetNextExternalRoute(otNetworkDataIterator *aIterator, otExternalRouteConfig *aConfig);

    /**
     * This method provides the next external route in the Thread Network Data for a given RLOC16.
     *
     * @param[inout]  aIterator  A pointer to the Network Data iterator context.
     * @param[in]     aRloc16    The RLOC16 value.
     * @param[out]    aConfig    A pointer to where the external route information will be placed.
     *
     * @retval OT_ERROR_NONE       Successfully found the next external route.
     * @retval OT_ERROR_NOT_FOUND  No subsequent external route exists in the Thread Network Data.
     *
     */
    otError GetNextExternalRoute(otNetworkDataIterator *aIterator, uint16_t aRloc16,
                                 otExternalRouteConfig *aConfig);

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
     * @param[in]  aPrefixLength  The prefix length pointed to by @p aPrefix.
     * @param[in]  aTlvs          A pointer to a specified tlvs buffer.
     * @param[in]  aTlvsLength    The specified tlvs buffer length pointed to by @p aTlvs.
     *
     * @returns A pointer to the Prefix TLV is one is found or NULL if no matching Prefix TLV exists.
     *
     */
    PrefixTlv *FindPrefix(const uint8_t *aPrefix, uint8_t aPrefixLength, uint8_t *aTlvs, uint8_t aTlvsLength);

    /**
     * This method inserts bytes into the Network Data.
     *
     * @param[in]  aStart   A pointer to the beginning of the insertion.
     * @param[in]  aLength  The number of bytes to insert.
     *
     * @retval OT_ERROR_NONE          Successfully inserted bytes.
     * @retval OT_ERROR_NO_BUFS       Insufficient buffer space to insert bytes.
     *
     */
    otError Insert(uint8_t *aStart, uint8_t aLength);

    /**
     * This method removes bytes from the Network Data.
     *
     * @param[in]  aStart   A pointer to the beginning of the removal.
     * @param[in]  aLength  The number of bytes to remove.
     *
     * @retval OT_ERROR_NONE    Successfully removed bytes.
     *
     */
    otError Remove(uint8_t *aStart, uint8_t aLength);

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

    uint8_t mTlvs[kMaxSize];  ///< The Network Data buffer.
    uint8_t mLength;          ///< The number of valid bytes in @var mTlvs.

private:
    enum
    {
        kDataResubmitDelay = 300000,  ///< DATA_RESUBMIT_DELAY (miliseconds)
    };

    class NetworkDataIterator
    {
    public:
        NetworkDataIterator(otNetworkDataIterator *aIterator):
            mIteratorBuffer(reinterpret_cast<uint8_t *>(aIterator)) { }

        uint8_t GetTlvsIndex(void) const { return mIteratorBuffer[0]; }
        uint8_t GetEntryIndex(void) const { return mIteratorBuffer[1]; }
        void SetTlvsIndex(uint8_t aIndex) { mIteratorBuffer[0] = aIndex; }
        void SetEntryIndex(uint8_t aIndex) { mIteratorBuffer[1] = aIndex; }

    private:
        uint8_t *mIteratorBuffer;
    };

    const bool      mLocal;
    bool            mLastAttemptWait;
    uint32_t        mLastAttempt;
};

}  // namespace NetworkData

/**
 * @}
 */

}  // namespace ot

#endif  // NETWORK_DATA_HPP_
