/*
 *  Copyright (c) 2026, The OpenThread Authors.
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
 *   This file implements helper methods for a radio frame.
 */

#include "radio_frame.hpp"

#include "radio/trel_link.hpp"

namespace ot {
namespace Radio {

#if OPENTHREAD_CONFIG_MULTI_RADIO
uint16_t Frame::GetMtu(void) const
{
    uint16_t mtu = 0;

    switch (GetRadioType())
    {
#if OPENTHREAD_CONFIG_RADIO_LINK_IEEE_802_15_4_ENABLE
    case kTypeIeee802154:
        mtu = k154MtuSize;
        break;
#endif

#if OPENTHREAD_CONFIG_RADIO_LINK_TREL_ENABLE
    case kTypeTrel:
        mtu = Trel::Link::kMtuSize;
        break;
#endif
    }

    return mtu;
}

uint8_t Frame::GetFcsSize(void) const
{
    uint8_t fcsSize = 0;

    switch (GetRadioType())
    {
#if OPENTHREAD_CONFIG_RADIO_LINK_IEEE_802_15_4_ENABLE
    case kTypeIeee802154:
        fcsSize = k154FcsSize;
        break;
#endif

#if OPENTHREAD_CONFIG_RADIO_LINK_TREL_ENABLE
    case kTypeTrel:
        fcsSize = Trel::Link::kFcsSize;
        break;
#endif
    }

    return fcsSize;
}

#elif OPENTHREAD_CONFIG_RADIO_LINK_TREL_ENABLE
uint16_t Frame::GetMtu(void) const { return Trel::Link::kMtuSize; }

uint8_t Frame::GetFcsSize(void) const { return Trel::Link::kFcsSize; }
#endif

} // namespace Radio
} // namespace ot
