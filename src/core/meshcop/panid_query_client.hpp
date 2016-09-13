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
 *   This file includes definitions for responding to PANID Query Requests.
 */

#ifndef PANID_QUERY_CLIENT_HPP_
#define PANID_QUERY_CLIENT_HPP_

#include <openthread-core-config.h>
#include <openthread-types.h>
#include <commissioning/commissioner.h>
#include <coap/coap_server.hpp>
#include <common/timer.hpp>
#include <net/ip6_address.hpp>
#include <net/udp6.hpp>

namespace Thread {

class ThreadNetif;

/**
 * This class implements handling PANID Query Requests.
 *
 */
class PanIdQueryClient
{
public:
    /**
     * This constructor initializes the object.
     *
     */
    PanIdQueryClient(ThreadNetif &aThreadNetif);

    ThreadError SendQuery(uint16_t aPanId, uint32_t aChannelMask, const Ip6::Address &aAddress,
                          otCommissionerPanIdConflictCallback aCallback, void *aContext);

private:
    static void HandleConflict(void *aContext, Coap::Header &aHeader, Message &aMessage,
                               const Ip6::MessageInfo &aMessageInfo);
    void HandleConflict(Coap::Header &aHeader, Message &aMessage, const Ip6::MessageInfo &aMessageInfo);

    static void HandleTimer(void *aContext);
    void HandleTimer(void);

    static void HandleUdpReceive(void *aContext, otMessage aMessage, const otMessageInfo *aMessageInfo);

    ThreadError SendConflictResponse(const Coap::Header &aRequestHeader, const Ip6::MessageInfo &aRequestMessageInfo);

    otCommissionerPanIdConflictCallback mCallback;
    void *mContext;

    Coap::Resource mPanIdQuery;
    Ip6::UdpSocket mSocket;
    Timer mTimer;

    Coap::Server &mCoapServer;
    ThreadNetif &mNetif;
};

/**
 * @}
 */

}  // namespace Thread

#endif  // PANID_QUERY_CLIENT_HPP_
