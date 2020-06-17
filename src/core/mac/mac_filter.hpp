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

#include "openthread-core-config.h"

#include <stdint.h>

#include "mac/mac_frame.hpp"

#if OPENTHREAD_CONFIG_MAC_FILTER_ENABLE

namespace ot {
namespace Mac {

/**
 * @addtogroup core-mac
 *
 * @{
 *
 */

/**
 * This class implements Mac Filter on IEEE 802.15.4 frames.
 *
 */
class Filter
{
public:
    /**
     * This structure represents a Mac Filter entry (used during iteration).
     *
     */
    typedef otMacFilterEntry Entry;

    /**
     * This type represents an iterator used to iterate through filter entries.
     *
     * See `GetNextAddress()` and `GetNextRssIn()`.
     *
     */
    typedef otMacFilterIterator Iterator;

    enum
    {
        kMaxEntries       = OPENTHREAD_CONFIG_MAC_FILTER_SIZE, ///< The maximum number of filter entries.
        kFixedRssDisabled = OT_MAC_FILTER_FIXED_RSS_DISABLED,  ///< Value to indicate no fixed RSS is set.
    };

    /**
     * This constructor initializes the filter.
     *
     */
    Filter(void);

    /**
     * This function gets the address mode of the filter.
     *
     * @returns  the address mode.
     *
     */
    otMacFilterAddressMode GetAddressMode(void) const { return mAddressMode; }

    /**
     * This function sets the address mode of the filter.
     *
     * @param[in]  aMode  The address mode to set.
     *
     * @retval OT_ERROR_NONE           Successfully set the AddressFilter mode.
     * @retval OT_ERROR_INVALID_ARGS   @p aMode is not valid address mode.
     *
     */
    otError SetAddressMode(otMacFilterAddressMode aMode);

    /**
     * This method adds an Extended Address to filter.
     *
     * @param[in]  aExtAddress  A reference to the Extended Address.
     *
     * @retval OT_ERROR_NONE           Successfully added @p aExtAddress to the filter.
     * @retval OT_ERROR_ALREADY        If @p aExtAddress was already in the Filter.
     * @retval OT_ERROR_NO_BUFS        No available entry exists.
     *
     */
    otError AddAddress(const ExtAddress &aExtAddress);

    /**
     * This method removes an Extended Address from the filter.
     *
     * @param[in]  aExtAddress  A reference to the Extended Address.
     *
     * @retval OT_ERROR_NONE           Successfully removed @p aExtAddress from the filter.
     * @retval OT_ERROR_NOT_FOUND      @p aExtAddress is not in the filter.
     *
     */
    otError RemoveAddress(const ExtAddress &aExtAddress);

    /**
     * This method clears all Extended Addresses from the filter.
     *
     */
    void ClearAddresses(void);

    /**
     * This method iterates through filter entries.
     *
     * @param[inout]  aIterator  A reference to the MAC filter iterator context.
     *                           To get the first in-use address filter, set it to OT_MAC_FILTER_ITERATOR_INIT.
     * @param[out]     aEntry    A reference to where the information is placed.
     *
     * @retval OT_ERROR_NONE       Successfully retrieved the next address filter entry.
     * @retval OT_ERROR_NOT_FOUND  No subsequent entry exists.
     *
     */
    otError GetNextAddress(Iterator &aIterator, Entry &aEntry) const;

    /**
     * This method sets the received signal strength for the messages from the Extended Address.
     * The default received signal strength for all received messages would be set if no Extended Address is specified.
     *
     * @param[in]  aExtAddress  A pointer to the Extended Address, or nullptr to set the default received signal
     * strength.
     * @param[in]  aRss         The received signal strength to set.
     *
     * @retval OT_ERROR_NONE     Successfully set @p aRss for @p aExtAddress, or
     *                           set the default @p aRss for all received messages if @p aExtAddress is nullptr.
     * @retval OT_ERROR_NO_BUFS  No available entry exists.
     *
     */
    otError AddRssIn(const ExtAddress *aExtAddress, int8_t aRss);

    /**
     * This method removes the received signal strength setting for the received messages from the Extended Address,
     * or removes the default received signal strength setting if no Extended Address is specified.
     *
     * @param[in]  aExtAddress  A pointer to the Extended Address.
     *
     * @retval OT_ERROR_NONE       Successfully removed the received signal strength setting for the received
     *                             messages from @p aExtAddress or removed the default received signal strength
     *                             setting if @p aExtAddress is nullptr.
     * @retval OT_ERROR_NOT_FOUND  @p aExtAddress is not in the RssIn filter if it is not nullptr.
     *
     */
    otError RemoveRssIn(const ExtAddress *aExtAddress);

    /**
     * This method clears all the received signal strength settings.
     *
     */
    void ClearRssIn(void);

    /**
     * This method iterates through RssIn filter entry.
     *
     * @param[inout]  aIterator  A reference to the MAC filter iterator context. To get the first in-use RssIn
     *                           filter entry, it should be set to OT_MAC_FILTER_ITERATOR_INIT.
     * @param[out]    aEntry     A reference to where the information is placed. The last entry would have the
     *                           Extended Address as all 0xff to indicate the default received signal strength
     *                           if it was set.
     *
     * @retval OT_ERROR_NONE       Successfully retrieved the next RssIn filter entry.
     * @retval OT_ERROR_NOT_FOUND  No subsequent entry exists.
     *
     */
    otError GetNextRssIn(Iterator &aIterator, Entry &aEntry);

    /**
     * This method applies the filter rules on a given Extended Address.
     *
     * @param[in]  aExtAddress  A reference to the Extended Address.
     * @param[out] aRss         A reference to where the received signal strength to be placed.
     *
     * @retval OT_ERROR_NONE                Successfully applied the filter rules on @p aExtAddress.
     * @retval OT_ERROR_ADDRESS_FILTERED    Address filter (whitelist or blacklist) is enabled and @p aExtAddress is
     *                                      filtered.
     *
     */
    otError Apply(const ExtAddress &aExtAddress, int8_t &aRss);

private:
    struct FilterEntry
    {
        bool       mFiltered;   // Indicates whether or not this entry is filtered (whitelist/blacklist modes).
        int8_t     mRssIn;      // The RssIn value for this entry or `kFixedRssDisabled`.
        ExtAddress mExtAddress; // IEEE 802.15.4 Extended Address.

        bool IsInUse(void) const { return mFiltered || (mRssIn != kFixedRssDisabled); }
    };

    FilterEntry *FindAvailableEntry(void);
    FilterEntry *FindEntry(const ExtAddress &aExtAddress);

    otMacFilterAddressMode mAddressMode;
    int8_t                 mDefaultRssIn;
    FilterEntry            mFilterEntries[kMaxEntries];
};

/**
 * @}
 *
 */

} // namespace Mac
} // namespace ot

#endif // OPENTHREAD_CONFIG_MAC_FILTER_ENABLE

#endif // MAC_FILTER_HPP_
