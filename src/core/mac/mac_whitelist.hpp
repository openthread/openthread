/*
 *  Copyright (c) 2016, Nest Labs, Inc.
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

#ifndef MAC_WHITELIST_HPP_
#define MAC_WHITELIST_HPP_

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
 * This class implements whitelist filtering on IEEE 802.15.4 frames.
 *
 */
class Whitelist
{
public:
    enum
    {
        kMaxEntries = 32,
    };

    /**
     * This structure represents a whitelist entry.
     *
     */
    struct Entry
    {
        ExtAddress mExtAddress;        ///< The IEEE 802.15.4 Extended Address.
        int8_t     mRssi;              ///< The constant RSSI value.
        bool       mValid : 1;         ///< TRUE if the entry is valid, FALSE otherwise.
        bool       mConstantRssi : 1;  ///< TRUE if the constant RSSI value is used, FALSE otherwise.
    };

    /**
     * This constructor initializes the whitelist filter.
     *
     */
    Whitelist(void);

    /**
     * This method enables the whitelist filter.
     *
     */
    void Enable(void);

    /**
     * This method disables the whitelist filter.
     *
     */
    void Disable(void);

    /**
     * This method indicates whether or not the whitelist filter is enabled.
     *
     * @retval TRUE   If the whitelist filter is enabled.
     * @retval FALSE  If the whitelist filter is disabled.
     *
     */
    bool IsEnabled(void) const;

    /**
     * This method returns the maximum number of whitelist entries.
     *
     * @returns The maximum number of whitelist entries.
     *
     */
    int GetMaxEntries(void) const;

    /**
     * This method returns the whitelist entries.
     *
     * @returns The whitelist entries.
     *
     */
    const Entry *GetEntries(void) const;

    /**
     * This method adds an Extended Address to the whitelist filter.
     *
     * @param[in]  aAddress  A reference to the Extended Address.
     *
     * @returns A pointer to the whitelist entry or NULL if there are no available entries.
     *
     */
    Entry *Add(const ExtAddress &aAddress);

    /**
     * This method removes an Extended Address to the whitelist filter.
     *
     * @param[in]  aAddress  A reference to the Extended Address.
     *
     */
    void Remove(const ExtAddress &aAddress);

    /**
     * This method removes all entries from the whitelist filter.
     *
     */
    void Clear(void);

    /**
     * This method finds a whitelist entry.
     *
     * @param[in]  aAddress  A reference to the Extended Address.
     *
     * @returns A pointer to the whitelist entry or NULL if the entry could not be found.
     *
     */
    Entry *Find(const ExtAddress &aAddress);

    /**
     * This method clears the constant RSSI value and uses the measured value provided by the radio instead.
     *
     * @param[in]  aEntry  A reference to the whitelist entry.
     *
     */
    void ClearConstantRssi(Entry &aEntry);

    /**
     * This method indicates whether or not the constant RSSI is set.
     *
     * @param[in]   aEntry  A reference to the whitelist entry.
     * @param[out]  aRssi   A reference to the RSSI variable.
     *
     * @retval kThreadError_None        A constant RSSI is set and written to @p aRssi.
     * @retval kThreadError_InvalidArg  A constant RSSI was not set.
     *
     */
    ThreadError GetConstantRssi(Entry &aEntry, int8_t &aRssi) const;

    /**
     * This method sets a constant RSSI value for all received messages matching @p aEntry.
     *
     * @param[in]  aEntry  A reference to the whitelist entry.
     * @param[in]  aRssi   An RSSI value in dBm.
     *
     */
    void SetConstantRssi(Entry &aEntry, int8_t aRssi);

private:
    Entry mWhitelist[kMaxEntries];

    bool mEnabled;
};

/**
 * @}
 *
 */

}  // namespace Mac
}  // namespace Thread

#endif  // MAC_WHITELIST_HPP_
