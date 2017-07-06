/*
 *  Copyright (c) 2017, The OpenThread Authors.
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
 *   This file includes definitions for IEEE 802.15.4 frame filtering based on MAC address.
 */

#ifndef MAC_FILTER_HPP_
#define MAC_FILTER_HPP_

#include "utils/wrap_stdint.h"

#include <openthread/types.h>

#include "mac/mac_frame.hpp"

namespace ot {
namespace Mac {

/**
 * @addtogroup core-mac
 *
 * @{
 *
 */

/**
 * This class implements Filter on IEEE 802.15.4 frames.
 *
 */
class Filter
{
public:
    typedef otMacFilterEntry Entry;

    enum
    {
        kMaxEntries = OPENTHREAD_CONFIG_MAC_FILTER_SIZE,
    };

    /**
     * This constructor initializes the filter.
     *
     */
    Filter(void);

    /**
     * This method returns the maximum number of Filter entries.
     *
     * @returns The maximum number of Filterlist entries.
     *
     */
    int GetMaxEntries(void) const { return kMaxEntries; }

    /**
     * This method adds an Extended Address to the address filter with specified RssiIn setting.
     * @note  The AddressFilter should be not in blacklist mode.
     *
     * @param[in]  aAddress  A pointer to the Extended Address.
     * @param[in]  aRssi     The RssiIn value (in dBm) for the Extended Address.
     *
     * @retval OT_ERROR_NONE           Successfully added @p aAddress to the address filter
     *                                 with specified RssiIn @p aRssi value.
     * @retval OT_ERROR_INVALID_STATE  The Addressfilter is in blacklist mode.
     * @retval OT_ERROR_NO_BUFS        No available address filter entry.
     *
     */
    otError AddEntry(const ExtAddress *aAddress, int8_t aRssi);

    /**
     * This function gets the AddressFilter state.
     *
     * @returns  the AddressFilter State.
     *
     */
    uint8_t AddressFilterGetState(void) const { return mAddressFilterState; }

    /**
     * This function sets the AddressFilter state.
     *
     * @param[in]  aState        The AddressFilter state to set.
     *
     * @retval OT_ERROR_NONE           Successfully set the address filter state.
     * @retval OT_ERROR_INVALID_STATE  The AddressFilter is working in the mode different from
     *                                 the mode trying to enable.
     *
     */
    otError AddressFilterSetState(uint8_t aState);

    /**
     * This method adds an Extended Address to the address filter.
     *
     * @param[in]  aAddress  A pointer to the Extended Address.
     *
     * @retval OT_ERROR_NONE           Successfully added @p aAddress to the address filter.
     * @retval OT_ERROR_ALREADY        @p aAddress was already in the address filter.
     * @retval OT_ERROR_NO_BUFS        No available address filter enty.
     *
     */
    otError AddressFilterAddEntry(const ExtAddress *aAddress);

    /**
     * This method removes an Extended Address from the address filter.
     *
     * @param[in]  aAddress  A pointer to the Extended Address.
     *
     * @retval OT_ERROR_NONE           Successfully removed the address filter entry.
     * @retval OT_ERROR_NOT_FOUND      @p aAddress is not in the address filter.
     *
     */
    otError AddressFilterRemoveEntry(const ExtAddress *aAddress);

    /**
     * This method clears AddressFilter entries.
     *
     */
    void AddressFilterClearEntries(void);

    /**
     * This method gets an in-use AddressFilter entry.
     *
     * @param[in]   aIterator  A pointer to the MAC filter iterator context. To get the first in-use AddressFilter entry,
     *                         it should be set to OT_MAC_FILTER_ITEERATOR_INIT.
     * @param[out]  aEntry     A pointer to where the information is placed.
     *
     * @retval OT_ERROR_NONE       Successfully retrieved the in-use AddressFilter entry.
     * @retval OT_ERROR_NOT_FOUND  No subsequent entry exists.
     *
     */
    otError GetNextAddressFilterEntry(otMacFilterIterator *aIterator, Entry *aEntry) const;

    /**
     * This method finds in-use AddressFilter entry of the Extended Address.
     *
     * @param[in]  aAddress        A pointer to the Extended Address.
     *
     * @retval OT_ERROR_NONE       Successfully finds the in-use entry with @p aAddress.
     * @retval OT_ERROR_NOT_FOUND  No such entry.
     *
     */
    Entry *AddressFilterFindEntry(const ExtAddress *aAddress);

    /**
     * This method sets the default RssiIn for all the received messages.
     *
     * @param[in] aRssi The RssiIn alue (in dBm).
     *
     */
    void RssiInFilterSet(int8_t aRssiIn);

    /**
     * This method gets the default RssiIn for all the received messages.
     *
     * @returns  RssiIn value (in dBm) or RSSI_OVERRIDE_DISABLED if not set.
     *
     */
    int8_t RssiInFilterGet(void);

    /**
     * This method sets the RssiIn for the received messsages from the Extended Address.
     *
     * @param[in]  aAddress  A pointer to the Extended Address.
     * @param[in]  aRssi     The RssiIn value (in dBm) to set.
     *
     * @retval OT_ERROR_NONE    Successfully set the RssiIn as @p aRssi for the received messages from @p aAddress.
     * @retval OT_ERROR_NO_BUFS No available RssiIn filter enty.
     *
     */
    otError RssiInFilterAddEntry(const ExtAddress *aAddress, int8_t aRssi);

    /**
     * This method removes the RssiIn setting for the received messages from the Extended Address.
     *
     * @param[in]  aAddress        A pointer to the Extended Address.
     *
     * @retval OT_ERROR_NONE       Successfully removed the RssiIn setting for the received messages from @p aAddress
     * @retval OT_ERROR_NOT_FOUND  No RssiIn entry for @p aAddress.
     */
    otError RssiInFilterRemoveEntry(const ExtAddress *aAddress);

    /**
     * This method clears RssiIn entries.
     *
     */
    void RssiInFilterClearEntries(void);

    /**
     * This method gets a in-use RssiInFilter entry.
     *
     * @param[in]   aIterator  A pointer to the MAC filter iterator context. To get the first in-use AddressFilter entry,
     *                         it should be set to OT_MAC_FILTER_ITEERATOR_INIT.
     * @param[out]  aEntry     A pointer to where the information is placed.
     *
     * @retval OT_ERROR_NONE       Successfully retrieved the in-use RssiInFilter entry.
     * @retval OT_ERROR_NOT_FOUND  No subsequent entry exists.
     *
     */
    otError GetNextRssiInFilterEntry(otMacFilterIterator *aIterator, Entry *aEntry) const;

    /**
     * This method finds in-use RssiInFilter entry of the Extended Address.
     *
     * @param[in]  aAddress        A pointer to the Extended Address.
     *
     * @retval OT_ERROR_NONE       Successfully finds the in-use entry with @p aAddress.
     * @retval OT_ERROR_NOT_FOUND  No such entry.
     *
     */
    Entry *RssiInFilterFindEntry(const ExtAddress *aAddress);

    /**
     * This method applies the filter rules on the Extended Address.
     *
     * @param[in]  aAddress        A pointer to the Extended Address.
     * @param[out] aRssi           A reference to where the Rssi value to be placed.
     *
     * @retval OT_ERROR_NONE                Successfully applied the fiter rules on @p aAddress.
     * @retval OT_ERROR_WHITELIST_FILTERED  Whitelist is enabled and @p aAddress is not in the whitelist
     * @retval OT_ERROR_BLACKLIST_FILTERED  Blacklist is enabled and @p aAddress is in the blacklist
     *
     */
    otError Apply(const ExtAddress *aAddress, int8_t &aRssi);

private:
    Entry *FindAvailEntry(void);

    Entry mFilterEntries[kMaxEntries];
    uint8_t mAddressFilterState;
    int8_t mRssiIn;
};

/**
 * @}
 *
 */

}  // namespace Mac
}  // namespace ot

#endif  // MAC_FILTER_HPP_
