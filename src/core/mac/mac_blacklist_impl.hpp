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

#ifndef MAC_BLACKLIST_HPP_
#define MAC_BLACKLIST_HPP_

#include <stdint.h>

#include <openthread-types.h>
#include <mac/mac_frame.hpp>

namespace Thread {
namespace Mac {

/**
 * @addtogroup core-mac
 *
 * @{
 *
 */

/**
 * This class implements blacklist filtering on IEEE 802.15.4 frames.
 *
 */
class Blacklist
{
public:
    typedef otMacBlacklistEntry Entry;

    enum
    {
        kMaxEntries = OPENTHREAD_CONFIG_MAC_BLACKLIST_SIZE,
    };

    /**
     * This constructor initializes the blacklist filter.
     *
     */
    Blacklist(void);

    /**
     * This method enables the blacklist filter.
     *
     */
    void Enable(void);

    /**
     * This method disables the blacklist filter.
     *
     */
    void Disable(void);

    /**
     * This method indicates whether or not the blacklist filter is enabled.
     *
     * @retval TRUE   If the blacklist filter is enabled.
     * @retval FALSE  If the blacklist filter is disabled.
     *
     */
    bool IsEnabled(void) const;

    /**
     * This method returns the maximum number of blacklist entries.
     *
     * @returns The maximum number of blacklist entries.
     *
     */
    int GetMaxEntries(void) const;

    /**
     * This method gets a blacklist entry.
     *
     * @param[in]   aIndex  An index into the MAC blacklist table.
     * @param[out]  aEntry  A reference to where the information is placed.
     *
     * @retval kThreadError_None         Successfully retrieved the MAC blacklist entry.
     * @retval kThreadError_InvalidArgs  @p aIndex is out of bounds or @p aEntry is NULL.
     *
     */
    ThreadError GetEntry(uint8_t aIndex, Entry &aEntry) const;

    /**
     * This method adds an Extended Address to the blacklist filter.
     *
     * @param[in]  aAddress  A reference to the Extended Address.
     *
     * @returns A pointer to the blacklist entry or NULL if there are no available entries.
     *
     */
    Entry *Add(const ExtAddress &aAddress);

    /**
     * This method removes an Extended Address to the blacklist filter.
     *
     * @param[in]  aAddress  A reference to the Extended Address.
     *
     */
    void Remove(const ExtAddress &aAddress);

    /**
     * This method removes all entries from the blacklist filter.
     *
     */
    void Clear(void);

    /**
     * This method finds a blacklist entry.
     *
     * @param[in]  aAddress  A reference to the Extended Address.
     *
     * @returns A pointer to the blacklist entry or NULL if the entry could not be found.
     *
     */
    Entry *Find(const ExtAddress &aAddress);

private:
    Entry mBlacklist[kMaxEntries];

    bool mEnabled;
};

/**
 * @}
 *
 */

}  // namespace Mac
}  // namespace Thread

#endif  // MAC_BLACKLIST_HPP_
