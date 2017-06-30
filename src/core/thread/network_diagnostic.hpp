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

#include <openthread/types.h>

#include "openthread-core-config.h"
#include "coap/coap.hpp"
#include "common/locator.hpp"
#include "net/udp6.hpp"

namespace ot {

namespace NetworkDiagnostic {

class Ip6AddressListTlv;
class ChildTableTlv;
class NetworkDiagnosticTlv;

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
class NetworkDiagnostic: public ThreadNetifLocator
{
public:
    /**
     * This constructor initializes the object.
     *
     */
    explicit NetworkDiagnostic(ThreadNetif &aThreadNetif);

    /**
     * This method registers a callback to provide received raw DIAG_GET.rsp or an DIAG_GET.ans payload.
     *
     * @param[in]  aCallback         A pointer to a function that is called when an DIAG_GET.rsp or an DIAG_GET.ans
     *                               is received or NULL to disable the callback.
     * @param[in]  aCallbackContext  A pointer to application-specific context.
     *
     */
    void SetReceiveDiagnosticGetCallback(otReceiveDiagnosticGetCallback aCallback, void *aCallbackContext);

    /**
     * This method sends Diagnostic Get request. If the @p aDestination is of multicast type, the DIAG_GET.qry
     * message is sent or the DIAG_GET.req otherwise.
     *
     * @param[in] aDestination  A reference to the destination address.
     * @param[in] aTlvTypes     An array of Network Diagnostic TLV types.
     * @param[in] aCount        Number of types in aTlvTypes
     *
     */
    otError SendDiagnosticGet(const Ip6::Address &aDestination, const uint8_t aTlvTypes[], uint8_t aCount);

    /**
     * This method sends Diagnostic Reset request.
     *
     * @param[in] aDestination  A reference to the destination address.
     * @param[in] aTlvTypes     An array of Network Diagnostic TLV types.
     * @param[in] aCount        Number of types in aTlvTypes
     *
     */
    otError SendDiagnosticReset(const Ip6::Address &aDestination, const uint8_t aTlvTypes[], uint8_t aCount);

private:

    otError AppendIp6AddressList(Message &aMessage);
    otError AppendChildTable(Message &aMessage);
    otError FillRequestedTlvs(Message &aRequest, Message &aResponse, NetworkDiagnosticTlv &aNetworkDiagnosticTlv);

    static void HandleDiagnosticGetRequest(void *aContext, otCoapHeader *aHeader, otMessage *aMessage,
                                           const otMessageInfo *aMessageInfo);
    void HandleDiagnosticGetRequest(Coap::Header &aHeader, Message &aMessage, const Ip6::MessageInfo &aMessageInfo);

    static void HandleDiagnosticGetQuery(void *aContext, otCoapHeader *aHeader, otMessage *aMessage,
                                         const otMessageInfo *aMessageInfo);
    void HandleDiagnosticGetQuery(Coap::Header &aHeader, Message &aMessage, const Ip6::MessageInfo &aMessageInfo);

    static void HandleDiagnosticGetResponse(void *aContext, otCoapHeader *aHeader, otMessage *aMessage,
                                            const otMessageInfo *aMessageInfo, otError aResult);
    void HandleDiagnosticGetResponse(Coap::Header &aHeader, Message &aMessage, const Ip6::MessageInfo &aMessageInfo,
                                     otError aResult);

    static void HandleDiagnosticGetAnswer(void *aContext, otCoapHeader *aHeader, otMessage *aMessage,
                                          const otMessageInfo *aMessageInfo);
    void HandleDiagnosticGetAnswer(Coap::Header &aHeader, Message &aMessage, const Ip6::MessageInfo &aMessageInfo);

    static void HandleDiagnosticReset(void *aContext, otCoapHeader *aHeader, otMessage *aMessage,
                                      const otMessageInfo *aMessageInfo);
    void HandleDiagnosticReset(Coap::Header &aHeader, Message &aMessage, const Ip6::MessageInfo &aMessageInfo);

    Coap::Resource mDiagnosticGetRequest;
    Coap::Resource mDiagnosticGetQuery;
    Coap::Resource mDiagnosticGetAnswer;
    Coap::Resource mDiagnosticReset;

    otReceiveDiagnosticGetCallback mReceiveDiagnosticGetCallback;
    void *mReceiveDiagnosticGetCallbackContext;
};

/**
 * @}
 */
} // namespace NetworkDiagnostic

}  // namespace ot

#endif  // NETWORK_DIAGNOSTIC_HPP_
