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
 *   This file includes definitions for responding to Announce Requests.
 */

#ifndef ANNOUNCE_BEGIN_CLIENT_HPP_
#define ANNOUNCE_BEGIN_CLIENT_HPP_

#include "openthread-core-config.h"
#include "common/locator.hpp"
#include "coap/coap.hpp"
#include "net/ip6_address.hpp"
#include "net/udp6.hpp"

namespace ot {

/**
 * This class implements handling Announce Begin Requests.
 *
 */
class AnnounceBeginClient: public ThreadNetifLocator
{
public:
    /**
     * This constructor initializes the object.
     *
     */
    AnnounceBeginClient(ThreadNetif &aThreadNetif);

    /**
     * This method sends a Announce Begin message.
     *
     * @param[in]  aChannelMask   The channel mask value.
     * @param[in]  aCount         The number of energy measurements per channel.
     * @param[in]  aPeriod        The time between energy measurements (milliseconds).
     *
     * @retval OT_ERROR_NONE     Successfully enqueued the Announce Begin message.
     * @retval OT_ERROR_NO_BUFS  Insufficient buffers to generate a Announce Begin message.
     *
     */
    otError SendRequest(uint32_t aChannelMask, uint8_t aCount, uint16_t mPeriod, const Ip6::Address &aAddress);
};

/**
 * @}
 */

}  // namespace ot

#endif  // ANNOUNCE_BEGIN_CLIENT_HPP_
