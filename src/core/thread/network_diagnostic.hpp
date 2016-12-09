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
 *   This file includes definitions for handle network diagnostic.
 */

#ifndef NETWORK_DIAGNOSTIC_HPP_
#define NETWORK_DIAGNOSTIC_HPP_

#include <openthread-core-config.h>
#include <openthread-types.h>
#include <coap/coap_client.hpp>
#include <coap/coap_server.hpp>
#include <net/udp6.hpp>

namespace Thread {

class ThreadNetif;
using namespace Coap;

namespace NetworkDiagnostic {

class IPv6AddressListTlv;
class ChildTableTlv;

/**
 * @addtogroup core-netdiag
 *
 * @brief
 *   This module includes definitions for sending and handling Network Diagnostic Commands.
 *
 * @{
 */

/**
 * This class implements the Network Diagnostic processing.
 *
 */
class NetworkDiagnostic
{
public:
    /**
     * This constructor initializes the object.
     *
     */
    explicit NetworkDiagnostic(ThreadNetif &aThreadNetif);

    /**
     * This method sends Diagnostic Get request.
     *
     * @param[in] aDestination  A reference to the destination address.
     * @param[in] aTlvTypes     An array of Network Diagnostic TLV types.
     * @param[in] aCount        Number of types in aTlvTypes
     *
     */
    ThreadError SendDiagnosticGet(const Ip6::Address &aDestination, const uint8_t aTlvTypes[], uint8_t aCount);

    /**
     * This method sends Diagnostic Reset request.
     *
     * @param[in] aDestination  A reference to the destination address.
     * @param[in] aTlvTypes     An array of Network Diagnostic TLV types.
     * @param[in] aCount        Number of types in aTlvTypes
     *
     */
    ThreadError SendDiagnosticReset(const Ip6::Address &aDestination, const uint8_t aTlvTypes[], uint8_t aCount);

    /**
     * This method fills IPv6AddressListTlv.
     *
     * @param[out] aTlv         A reference to the tlv.
     *
     */
    ThreadError AppendIPv6AddressList(Message &aMessage);

    /**
     * This method fills ChildTableTlv.
     *
     * @param[out] aTlv         A reference to the tlv.
     *
     */
    ThreadError AppendChildTable(Message &aMessage);

private:
    static void HandleDiagnosticGetResponse(void *aContext, otCoapHeader *aHeader, otMessage aMessage,
                                            const otMessageInfo *aMessageInfo, ThreadError aResult);
    void HandleDiagnosticGetResponse(Coap::Header *aHeader, Message *aMessage,
                                     const Ip6::MessageInfo *aMessageInfo, ThreadError aResult);

    static void HandleDiagnosticGet(void *aContext, otCoapHeader *aHeader, otMessage aMessage,
                                    const otMessageInfo *aMessageInfo);
    void HandleDiagnosticGet(Thread::Coap::Header &aHeader, Thread::Message &aMessage,
                             const Thread::Ip6::MessageInfo &aMessageInfo);

    static void HandleDiagnosticReset(void *aContext, otCoapHeader *aHeader, otMessage aMessage,
                                      const otMessageInfo *aMessageInfo);
    void HandleDiagnosticReset(Coap::Header &aHeader, Message &aMessage,
                               const Ip6::MessageInfo &aMessageInfo);

    Coap::Resource mDiagnosticGet;
    Coap::Resource mDiagnosticReset;
    Coap::Server &mCoapServer;
    Coap::Client &mCoapClient;

    Mle::MleRouter &mMle;
    Mac::Mac &mMac;
    ThreadNetif &mNetif;
};

/**
 * @}
 */
} // namespace NetworkDiagnostic

}  // namespace Thread

#endif  // NETWORK_DIAGNOSTIC_HPP_
