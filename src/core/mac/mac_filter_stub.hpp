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

class Filter
{
public:
    typedef otMacFilterEntry Entry;
    Filter(void) { }
    int GetMaxEntries(void) const { return 0; }
    otError AddEntry(const ExtAddress *, int8_t) { return OT_ERROR_NOT_IMPLEMENTED; }
    uint8_t AddressFilterGetState(void) const { return 0; }
    otError AddressFilterSetState(uint8_t) { return OT_ERROR_NOT_IMPLEMNTED; }
    otError AddressFilterAddEntry(const ExtAddress *) { return OT_ERROR_NOT_IMPLEMNTED; }
    otError AddressFilterRemoveEntry(const ExtAddress *) { return OT_ERROR_NOT_IMPLEMNTED; }
    void AddressFilterClearEntries(void) { }
    otError GetNextAddressFilterEntry(otMacFilterIterator *, Entry *) const { return OT_ERROR_NOT_IMPLEMNTED; }
    Entry *AddressFilterFindEntry(const ExtAddress *) { return NULL; }
    void RssiInFilterSet(int8_t) { }
    int8_t RssiInFilterGet(void) { return OT_RSSI_OVERRIDE_DISABLED; }
    otError RssiInFilterAddEntry(const ExtAddress *, int8_t) { return OT_ERROR_NOT_IMPLEMNTED; }
    otError RssiInFilterRemoveEntry(const ExtAddress *) { return OT_ERROR_NOT_IMPLEMNTED; }
    void RssiInFilterClearEntries(void) { }
    otError GetNextRssiInFilterEntry(otMacFilterIterator *, Entry *) const { return OT_ERROR_NOT_IMPLEMNTED; }
    Entry *RssiInFilterFindEntry(const ExtAddress *) { return NULL; }
};

/**
 * @}
 *
 */

}  // namespace Mac
}  // namespace ot

#endif  // MAC_FILTER_HPP_
