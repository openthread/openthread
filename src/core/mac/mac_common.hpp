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
 *   This file includes definitions for the IEEE 802.15.4 MAC.
 */

#ifndef MAC_COMMON_HPP_
#define MAC_COMMON_HPP_

#include "openthread-core-config.h"

#include <openthread/platform/radio-phy.h>
#include <openthread/platform/time.h>

#include "common/locator.hpp"
#include "common/tasklet.hpp"
#include "common/timer.hpp"
#include "mac/channel_mask.hpp"
#include "mac/mac_filter.hpp"
#include "mac/mac_frame.hpp"
#include "thread/key_manager.hpp"
#include "thread/link_quality.hpp"
#include "thread/network_diagnostic_tlvs.hpp"
#include "thread/topology.hpp"

namespace ot {

/**
 * @addtogroup core-mac
 *
 * @brief
 *   This module includes definitions for the IEEE 802.15.4 MAC
 *
 * @{
 *
 */

namespace Mac {

/**
 * Protocol parameters and constants.
 *
 */
enum
{
    kMinBE             = 3,  ///< macMinBE (IEEE 802.15.4-2006).
    kMaxBE             = 5,  ///< macMaxBE (IEEE 802.15.4-2006).
    kUnitBackoffPeriod = 20, ///< Number of symbols (IEEE 802.15.4-2006).

    kMinBackoff = 1, ///< Minimum backoff (milliseconds).

    kAckTimeout      = 16,  ///< Timeout for waiting on an ACK (milliseconds).
    kDataPollTimeout = 100, ///< Timeout for receiving Data Frame (milliseconds).
    kSleepDelay      = 300, ///< Max sleep delay when frame is pending (milliseconds).
    kNonceSize       = 13,  ///< Size of IEEE 802.15.4 Nonce (bytes).

    kScanChannelsAll     = OT_CHANNEL_ALL, ///< All channels.
    kScanDurationDefault = 300,            ///< Default interval between channels (milliseconds).

    kMaxCsmaBackoffsDirect =
        OPENTHREAD_CONFIG_MAC_MAX_CSMA_BACKOFFS_DIRECT, ///< macMaxCsmaBackoffs for direct transmissions
    kMaxCsmaBackoffsIndirect =
        OPENTHREAD_CONFIG_MAC_MAX_CSMA_BACKOFFS_INDIRECT, ///< macMaxCsmaBackoffs for indirect transmissions

    kMaxFrameRetriesDirect =
        OPENTHREAD_CONFIG_MAC_MAX_FRAME_RETRIES_DIRECT, ///< macMaxFrameRetries for direct transmissions
    kMaxFrameRetriesIndirect =
        OPENTHREAD_CONFIG_MAC_MAX_FRAME_RETRIES_INDIRECT, ///< macMaxFrameRetries for indirect transmissions

    kTxNumBcast = OPENTHREAD_CONFIG_TX_NUM_BCAST ///< Number of times each broadcast frame is transmitted
};

/**
 * @}
 *
 */

} // namespace Mac
} // namespace ot

#endif // MAC_COMMON_HPP_
