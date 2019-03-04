/*
 *  Copyright (c) 2019, The OpenThread Authors.
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
 *   This file includes definitions for the IEEE 802.15.4 PHY.
 */

#ifndef PHY_HPP_
#define PHY_HPP_

#include "openthread-core-config.h"

#include <openthread/platform/radio.h>

#include "utils/static_assert.hpp"

namespace ot {

/**
 * @addtogroup core-phy
 *
 * @brief
 *   This module includes definitions for the IEEE 802.15.4 PHY
 *
 * @{
 *
 */

namespace Phy {

/**
 * This enumeration defines the IEEE 802.15.4 channel related parameters.
 *
 */
enum
{
#if (OPENTHREAD_CONFIG_RADIO_2P4GHZ_OQPSK_SUPPORT && OPENTHREAD_CONFIG_RADIO_915MHZ_OQPSK_SUPPORT)
    kNumChannelPages       = 2,
    kSupportedChannels     = OT_RADIO_915MHZ_OQPSK_CHANNEL_MASK | OT_RADIO_2P4GHZ_OQPSK_CHANNEL_MASK,
    kChannelMin            = OT_RADIO_915MHZ_OQPSK_CHANNEL_MIN,
    kChannelMax            = OT_RADIO_2P4GHZ_OQPSK_CHANNEL_MAX,
    kSupportedChannelPages = OT_RADIO_CHANNEL_PAGE_0_MASK | OT_RADIO_CHANNEL_PAGE_2_MASK,
#elif OPENTHREAD_CONFIG_RADIO_915MHZ_OQPSK_SUPPORT
    kNumChannelPages       = 1,
    kSupportedChannels     = OT_RADIO_915MHZ_OQPSK_CHANNEL_MASK,
    kChannelMin            = OT_RADIO_915MHZ_OQPSK_CHANNEL_MIN,
    kChannelMax            = OT_RADIO_915MHZ_OQPSK_CHANNEL_MAX,
    kSupportedChannelPages = OT_RADIO_CHANNEL_PAGE_2_MASK,
#elif OPENTHREAD_CONFIG_RADIO_2P4GHZ_OQPSK_SUPPORT
    kNumChannelPages       = 1,
    kSupportedChannels     = OT_RADIO_2P4GHZ_OQPSK_CHANNEL_MASK,
    kChannelMin            = OT_RADIO_2P4GHZ_OQPSK_CHANNEL_MIN,
    kChannelMax            = OT_RADIO_2P4GHZ_OQPSK_CHANNEL_MAX,
    kSupportedChannelPages = OT_RADIO_CHANNEL_PAGE_0_MASK,
#endif
};

OT_STATIC_ASSERT((OPENTHREAD_CONFIG_RADIO_2P4GHZ_OQPSK_SUPPORT || OPENTHREAD_CONFIG_RADIO_915MHZ_OQPSK_SUPPORT),
                 "OPENTHREAD_CONFIG_RADIO_2P4GHZ_OQPSK_SUPPORT or OPENTHREAD_CONFIG_RADIO_915MHZ_OQPSK_SUPPORT "
                 "must be set to 1 to specify the radio mode");
} // namespace Phy
} // namespace ot

#endif // PHY_HPP_
