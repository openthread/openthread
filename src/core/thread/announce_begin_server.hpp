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

#ifndef ANNOUNCE_BEGIN_SERVER_HPP_
#define ANNOUNCE_BEGIN_SERVER_HPP_

#include "openthread-core-config.h"

#include <openthread/types.h>

#include "coap/coap.hpp"
#include "common/locator.hpp"
#include "common/timer.hpp"
#include "net/ip6_address.hpp"

namespace ot {

/**
 * This class implements handling Announce Begin Requests.
 *
 */
class AnnounceBeginServer: public InstanceLocator
{
public:
    /**
     * This constructor initializes the object.
     *
     */
    explicit AnnounceBeginServer(Instance &aInstance);

    /**
     * This method begins the MLE Announce transmission process using Count=3 and Period=1s.
     *
     * @param[in]  aChannelMask   The channels to use for transmission.
     *
     * @retval OT_ERROR_NONE  Successfully started the transmission process.
     *
     */
    otError SendAnnounce(uint32_t aChannelMask);

    /**
     * This method begins the MLE Announce transmission process.
     *
     * @param[in]  aChannelMask   The channels to use for transmission.
     * @param[in]  aCount         The number of transmissions per channel.
     * @param[in]  aPeriod        The time between transmissions (milliseconds).
     *
     * @retval OT_ERROR_NONE  Successfully started the transmission process.
     *
     */
    otError SendAnnounce(uint32_t aChannelMask, uint8_t aCount, uint16_t aPeriod);

private:
    enum
    {
        kDefaultCount  = 3,
        kDefaultPeriod = 1000,
    };

    static void HandleRequest(void *aContext, otCoapHeader *aHeader, otMessage *aMessage,
                              const otMessageInfo *aMessageInfo);
    void HandleRequest(Coap::Header &aHeader, Message &aMessage, const Ip6::MessageInfo &aMessageInfo);

    static void HandleTimer(Timer &aTimer);
    void HandleTimer(void);

    uint32_t mChannelMask;
    uint16_t mPeriod;
    uint8_t mCount;
    uint8_t mChannel;

    TimerMilli mTimer;

    Coap::Resource mAnnounceBegin;
};

/**
 * @}
 */

}  // namespace ot

#endif  // ANNOUNCE_BEGIN_SERVER_HPP_
