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
 *   This file includes definitions for IEEE 802.15.4 frame filtering based on MAC address.
 */

#ifndef MAC_FILTER_HPP_
#define MAC_FILTER_HPP_

#include <openthread/config.h>

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
        kInvalidLinkQualityIn = 0xff,
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
     * @retval OT_ERROR_NONE           Successfully set the AddressFilter State.
     * @retval OT_ERROR_INVALID_STATE  The AddressFilter is in use.
     *
     */
    otError AddressFilterSetState(uint8_t aState);

    /**
     * This method adds an Extended Address to the address filter.
     *
     * @param[in]  aAddress  A pointer to the Extended Address.
     *
     * @retval OT_ERROR_NONE           Successfully added the Extended Address to the filter.
     * @retval OT_ERROR_ALREADY        The Extended Address was already in the filter.
     * @retval OT_ERROR_NO_BUFS        No available filter enty.
     *
     */
    otError AddressFilterAddEntry(const ExtAddress *aAddress);

    /**
     * This method removes an Extended Address from the filter.
     *
     * @param[in]  aAddress  A pointer to the Extended Address.
     *
     * @retval OT_ERROR_NONE           Successfully removed the MAC Filter entry.
     * @retval OT_ERROR_NOT_FOUND      The Extended Address is not filtered.
     *
     */
    otError AddressFilterRemoveEntry(const ExtAddress *aAddress);

    /**
     * This method removes all address filter entries from the filter.
     *
     * @retval OT_ERROR_INVALID_STATE  The AddressFilter is not in use.
     *
     */
    otError AddressFilterClearEntries(void);

    /**
     * This method resets address filter which would be disabled and cleared.
     *
     */
    void AddressFilterReset(void);

    /**
     * This method sets the default fixed LinkQualityIn value for all the received messages.
     *
     * @param[in] aLinkQualityIn  The LinkQualityIn value.
     *
     * @retval OT_ERROR_NONE          Successfully set the default fixed LinkQualityIn.
     * @retval OT_ERROR_INVALID_ARGS  The @p aLinkQualityIn is not valid.
     *
     */
    otError LinkQualityInFilterSet(uint8_t aLinkQualityIn);

    /**
     * This method gets the default fixed LinkQualityIn value for all the received messages if any
     *
     * @param[in] aLinkQualityIn  A pointer to where the information is placed.
     *
     * @retval OT_ERROR_NONE          Successfully set the default fixed LinkQualityIn.
     * @retval OT_ERROR_NOT_FOUND     No default fixed LinkQualityIn is set.
     *
     */
    otError LinkQualityInFilterGet(uint8_t *aLinkQualityIn);

    /**
     * This method unsets the default fixed LinkQualityIn value for all the received messages if any.
     *
     *
     */
    void LinkQualityInFilterUnset(void);

    /**
     * This method sets the LinkQualityIn for the Extended Address with a fixed value.
     *
     * @param[in]  aAddress        A pointer to the Extended Address.
     * @param[in]  aLinkQualityIn  A fixed LinkQualityIn value to set.
     *
     * @retval OT_ERROR_NONE          The default fixed LinkQualityIn is set and written to @p aLinkQualityIn.
     * @retval OT_ERROR_INVALID_ARGS  The @p aLinkQualityIn is not valid.
     * @retval OT_ERROR_NO_BUFS       No available filter enty.
     *
     */
    otError LinkQualityInFilterAddEntry(const ExtAddress *aAddress, uint8_t aLinkQualityIn);

    /**
     * This method removes the fixed LinkQualityIn setting from the Extended Address.
     *
     * @param[in]  aAddress        A pointer to the Extended Address.
     *
     * @retval OT_ERROR_NONE       Successfully removed the fixed LinkQualityIn for @p aAddress
     * @retval OT_ERROR_NOT_FOUND  No fixed LinkQualityIn for @p aAddress.
     */
    otError LinkQualityInFilterRemoveEntry(const ExtAddress *aAddress);

    /**
     * This method removes LinkQualityIn filter entry if any.
     *
     */
    void LinkQualityInFilterClearEntries(void);

    /**
     * This method resets LinkQualityIn filter.
     *
     */
    void LinkQualityInFilterReset(void);

    /**
     * This method gets a in-use Filter entry.
     *
     * @param[in]   aIterator  A pointer to the MAC filter iterator context. To get the first in-use filter entry,
     *                         it should be set to OT_MAC_FILTER_ITEERATOR_INIT.
     * @param[out]  aEntry     A pointer to where the information is placed.
     *
     * @retval OT_ERROR_NONE       Successfully retrieved the in-use Filter entry.
     * @retval OT_ERROR_NOT_FOUND  No subsequent entry exists.
     *
     */
    otError GetNextEntry(otMacFilterIterator *aIterator, Entry *aEntry) const;

    /**
     * This method converts the specified valid link quality to typical rssi.
     *
     * @note Only for test purpose
     *
     * @param[in]   aNoiseFloor   The noise floor value.
     * @param[in]   aLinkQuality  The link quality to be converted.
     *
     * @returns  The typical rssi corresponding to the @p aLinkQualitySuccessfully.
     *
     */
    int8_t ConvertLinkQualityToRss(int8_t aNoiseFloor, uint8_t aLinkQuality);

    /**
     * This method finds in-use Filter entry which has the Extended Address.
     *
     * @param[in]  aAddress        A pointer to the Extended Address.
     *
     * @retval OT_ERROR_NONE       Successfully finds the in-use Filter entry which has the Extended Address.
     * @retval OT_ERROR_NOT_FOUND  No such in-use Filter Entry.
     *
     */
    Entry *FindEntry(const ExtAddress *aAddress);

private:
    Entry *FindAvailEntry(void);

    Entry mFilterEntries[kMaxEntries];
    uint8_t mAddressFilterState;
    uint8_t mLinkQualityIn;
};

/**
 * @}
 *
 */

}  // namespace Mac
}  // namespace ot

#endif  // MAC_FILTER_HPP_
