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

#ifndef OT_CORE_MAC_MAC_FILTER_HPP_
#define OT_CORE_MAC_MAC_FILTER_HPP_

#include "openthread-core-config.h"

#if OPENTHREAD_CONFIG_MAC_FILTER_ENABLE

#include <stdint.h>

#include "common/array.hpp"
#include "common/as_core_type.hpp"
#include "common/non_copyable.hpp"
#include "mac/mac_frame.hpp"

namespace ot {

class Neighbor;

namespace Mac {

/**
 * @addtogroup core-mac
 *
 * @{
 */

/**
 * Implements MAC Filter on IEEE 802.15.4 frames.
 */
class Filter : private NonCopyable
{
public:
    /**
     * Represents MAC Filter entry information.
     */
    typedef otMacFilterEntry EntryInfo;

    /**
     * Represents an iterator used to iterate through filter entries.
     *
     * See `GetNextAddress()` and `GetNextRssIn()`.
     */
    typedef otMacFilterIterator Iterator;

    /**
     * Represents the MAC Filter mode.
     */
    enum Mode : uint8_t
    {
        kModeRssInOnly = OT_MAC_FILTER_ADDRESS_MODE_DISABLED,  ///< No address filtering. RSS-In update only.
        kModeAllowlist = OT_MAC_FILTER_ADDRESS_MODE_ALLOWLIST, ///< Enable allowlist address filter mode.
        kModeDenylist  = OT_MAC_FILTER_ADDRESS_MODE_DENYLIST,  ///< Enable denylist address filter mode.
    };

    static constexpr int8_t   kFixedRssDisabled = OT_MAC_FILTER_FIXED_RSS_DISABLED; ///< Value when no fixed RSS is set.
    static constexpr Iterator kIteratorInit     = OT_MAC_FILTER_ITERATOR_INIT;      ///< Initializer for `Iterator`.

    /**
     * Initializes the filter.
     */
    Filter(void);

    /**
     * Gets the MAC Filter mode.
     *
     * @returns  The Filter mode.
     */
    Mode GetMode(void) const { return mMode; }

    /**
     * Sets the address mode of the filter.
     *
     * @param[in]  aMode  The new Filter mode.
     */
    void SetMode(Mode aMode) { mMode = aMode; }

    /**
     * Adds an Extended Address to filter.
     *
     * @param[in]  aExtAddress  A reference to the Extended Address.
     *
     * @retval kErrorNone          Successfully added @p aExtAddress to the filter.
     * @retval kErrorNoBufs        No available entry exists.
     */
    Error AddAddress(const ExtAddress &aExtAddress)
    {
        return AddOrUpdateEntry(kAddrFilter, aExtAddress, kFixedRssDisabled);
    }

    /**
     * Removes an Extended Address from the filter.
     *
     * No action is performed if there is no existing entry in the filter list matching the given Extended Address.
     *
     * @param[in]  aExtAddress  A reference to the Extended Address to remove.
     */
    void RemoveAddress(const ExtAddress &aExtAddress) { RemoveEntry(kAddrFilter, aExtAddress); }

    /**
     * Clears all Extended Addresses from the filter.
     */
    void ClearAddresses(void) { ClearAll(kAddrFilter); }

    /**
     * Iterates through filter entries.
     *
     * @param[in,out]  aIterator  A reference to the MAC filter iterator context.
     *                            To get the first in-use address filter, set it to `kIteratorInit`.
     * @param[out]     aInfo      A reference to where the entry information is placed.
     *
     * @retval kErrorNone      Successfully retrieved the next address filter entry.
     * @retval kErrorNotFound  No subsequent entry exists.
     */
    Error GetNextAddress(Iterator &aIterator, EntryInfo &aInfo) const { return GetNext(kAddrFilter, aIterator, aInfo); }

    /**
     * Adds a fixed received signal strength entry for the messages from a given Extended Address.
     *
     * @param[in]  aExtAddress  An Extended Address.
     * @param[in]  aRss         The received signal strength to set.
     *
     * @retval kErrorNone         Successfully set @p aRss for @p aExtAddress.
     * @retval kErrorInvalidArgs  @p aRss is not valid (equal to `kFixedRssDisabled`).
     * @retval kErrorNoBufs       No available entry exists.
     */
    Error AddRssIn(const ExtAddress &aExtAddress, int8_t aRss);

    /**
     * Removes a fixed received signal strength entry for a given Extended Address.
     *
     * No action is performed if there is no existing entry in the filter list matching the given Extended Address.
     *
     * @param[in]  aExtAddress   An Extended Address.
     */
    void RemoveRssIn(const ExtAddress &aExtAddress) { RemoveEntry(kRssFilter, aExtAddress); }

    /**
     * Sets the default received signal strength.
     *
     * The default RSS value is used for all received frames from addresses for which there is no explicit RSS-IN entry
     * in the Filter list (added using `AddRssIn()`).
     *
     * @param[in]  aRss  The default received signal strength to set.
     */
    void SetDefaultRssIn(int8_t aRss) { mDefaultRssIn = aRss; }

    /**
     * Clears the default received signal strength.
     */
    void ClearDefaultRssIn(void) { mDefaultRssIn = kFixedRssDisabled; }

    /**
     * Clears all the received signal strength settings (including the default RSS-In).
     */
    void ClearAllRssIn(void) { ClearAll(kRssFilter); }

    /**
     * Iterates through RssIn filter entry.
     *
     * @param[in,out]  aIterator  A reference to the MAC filter iterator context. To get the first in-use RssIn
     *                            filter entry, it should be set to `kIteratorInit`.
     * @param[out]     aInfo      A reference to where the entry information is placed. The last entry would have the
     *                            Extended Address as all 0xff to indicate the default received signal strength
     *                            if it was set.
     *
     * @retval kErrorNone      Successfully retrieved the next RssIn filter entry.
     * @retval kErrorNotFound  No subsequent entry exists.
     */
    Error GetNextRssIn(Iterator &aIterator, EntryInfo &aInfo) const { return GetNext(kRssFilter, aIterator, aInfo); }

    /**
     * Applies the filter rules on a given Extended Address.
     *
     * @param[in]  aExtAddress  A reference to the Extended Address.
     * @param[out] aRss         A reference to where the received signal strength to be placed.
     *
     * @retval kErrorNone             Successfully applied the filter rules on @p aExtAddress.
     * @retval kErrorAddressFiltered  Address filter (allowlist or denylist) is enabled and @p aExtAddress is filtered.
     */
    Error Apply(const ExtAddress &aExtAddress, int8_t &aRss) const;

    /**
     * Applies the filter rules to a received frame from a given Extended Address.
     *
     * Can potentially update the signal strength value on the received frame @p aRxFrame. If @p aNeighbor
     * is not `nullptr` and filter applies a fixed RSS to the @p aRxFrame, this method will also clear the current RSS
     * average on @p aNeighbor to ensure that the new fixed RSS takes effect quickly.
     *
     * @param[out] aRxFrame     The received frame.
     * @param[in]  aExtAddress  The extended address from which @p aRxFrame was received.
     * @param[in]  aNeighbor    A pointer to the neighbor (can be `nullptr` if not known).
     *
     * @retval kErrorNone             Successfully applied the filter, @p aRxFrame RSS may be updated.
     * @retval kErrorAddressFiltered  Address filter (allowlist or denylist) is enabled and @p aExtAddress is filtered.
     */
    Error ApplyToRxFrame(RxFrame &aRxFrame, const ExtAddress &aExtAddress, Neighbor *aNeighbor = nullptr) const;

private:
    static constexpr uint8_t kMaxEntries = OPENTHREAD_CONFIG_MAC_FILTER_SIZE;

    enum Type : bool
    {
        kRssFilter,
        kAddrFilter,
    };

    class Entry
    {
    public:
        enum NotInUseMatcher : uint8_t
        {
            kNotInUse,
        };

        void              Init(const ExtAddress &aExtAddress);
        const ExtAddress &GetExtAddress(void) const { return mExtAddress; }
        bool              IsInAddrFilter(void) const { return mInAddrFilter; }
        void              SetInAddrFilter(bool aInAddrFilter) { mInAddrFilter = aInAddrFilter; }
        bool              IsInRssFilter(void) const { return (mRssIn != kFixedRssDisabled); }
        int8_t            GetRssIn(void) const { return mRssIn; }
        void              SetRssIn(int8_t aRss) { mRssIn = aRss; }
        void              ClearRssIn(void) { mRssIn = kFixedRssDisabled; }
        bool              Matches(const ExtAddress &aExtAddress) const { return mExtAddress == aExtAddress; }
        bool              Matches(NotInUseMatcher) const { return !IsInAddrFilter() && !IsInRssFilter(); }

    private:
        ExtAddress mExtAddress;
        bool       mInAddrFilter;
        int8_t     mRssIn;
    };

    Error AddOrUpdateEntry(Type aType, const ExtAddress &aExtAddress, int8_t aRss);
    void  RemoveEntry(Type aType, const ExtAddress &aExtAddress);
    Error GetNext(Type aType, Iterator &aIterator, EntryInfo &aInfo) const;
    void  ClearAll(Type aType);

    Array<Entry, kMaxEntries> mEntries;
    Mode                      mMode;
    int8_t                    mDefaultRssIn;
};

/**
 * @}
 */

} // namespace Mac

DefineMapEnum(otMacFilterAddressMode, Mac::Filter::Mode);

} // namespace ot

#endif // OPENTHREAD_CONFIG_MAC_FILTER_ENABLE

#endif // OT_CORE_MAC_MAC_FILTER_HPP_
