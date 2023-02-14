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

#include "openthread-core-config.h"

#if OPENTHREAD_FTD || OPENTHREAD_CONFIG_TMF_NETWORK_DIAG_MTD_ENABLE

#include <openthread/netdiag.h>

#include "common/callback.hpp"
#include "common/locator.hpp"
#include "common/non_copyable.hpp"
#include "net/udp6.hpp"
#include "thread/network_diagnostic_tlvs.hpp"
#include "thread/tmf.hpp"
#include "thread/uri_paths.hpp"

namespace ot {

namespace NetworkDiagnostic {

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
class NetworkDiagnostic : public InstanceLocator, private NonCopyable
{
    friend class Tmf::Agent;

public:
    typedef otNetworkDiagIterator          Iterator;    ///< Iterator to go through TLVs in `GetNextDiagTlv()`.
    typedef otNetworkDiagTlv               TlvInfo;     ///< Parse info from a Network Diagnostic TLV.
    typedef otNetworkDiagChildEntry        ChildInfo;   ///< Parsed info for child table entry.
    typedef otReceiveDiagnosticGetCallback GetCallback; ///< Diagnostic Get callback function pointer type.

    static constexpr Iterator kIteratorInit = OT_NETWORK_DIAGNOSTIC_ITERATOR_INIT; ///< Initializer for Iterator.

    /**
     * This constructor initializes the object.
     *
     */
    explicit NetworkDiagnostic(Instance &aInstance);

    /**
     * This method sends Diagnostic Get request. If the @p aDestination is of multicast type, the DIAG_GET.qry
     * message is sent or the DIAG_GET.req otherwise.
     *
     * @param[in]  aDestination      The destination address.
     * @param[in]  aTlvTypes         An array of Network Diagnostic TLV types.
     * @param[in]  aCount            Number of types in @p aTlvTypes.
     * @param[in]  aCallback         Callback when Network Diagnostic Get response is received (can be NULL).
     * @param[in]  Context           Application-specific context used with @p aCallback.
     *
     */
    Error SendDiagnosticGet(const Ip6::Address &aDestination,
                            const uint8_t       aTlvTypes[],
                            uint8_t             aCount,
                            GetCallback         aCallback,
                            void               *Context);

    /**
     * This method sends Diagnostic Reset request.
     *
     * @param[in] aDestination  The destination address.
     * @param[in] aTlvTypes     An array of Network Diagnostic TLV types.
     * @param[in] aCount        Number of types in aTlvTypes
     *
     */
    Error SendDiagnosticReset(const Ip6::Address &aDestination, const uint8_t aTlvTypes[], uint8_t aCount);

    /**
     * This static method gets the next Network Diagnostic TLV in a given message.
     *
     * @param[in]      aMessage    Message to read TLVs from.
     * @param[in,out]  aIterator   The Network Diagnostic iterator. To get the first TLV set it to `kIteratorInit`.
     * @param[out]     aTlvInfo    A reference to a `TlvInfo` to output the next TLV data.
     *
     * @retval kErrorNone       Successfully found the next Network Diagnostic TLV.
     * @retval kErrorNotFound   No subsequent Network Diagnostic TLV exists in the message.
     * @retval kErrorParse      Parsing the next Network Diagnostic failed.
     *
     */
    static Error GetNextDiagTlv(const Coap::Message &aMessage, Iterator &aIterator, TlvInfo &aTlvInfo);

private:
    static constexpr uint16_t kMaxChildEntries = 398;

    enum Action : uint8_t
    {
        kMessageSend,
        kMessageReceive,
    };

    Error SendCommand(Uri                   aUri,
                      const Ip6::Address   &aDestination,
                      const uint8_t         aTlvTypes[],
                      uint8_t               aCount,
                      Coap::ResponseHandler aHandler = nullptr,
                      void                 *aContext = nullptr);

    Error AppendIp6AddressList(Message &aMessage);
    Error AppendChildTable(Message &aMessage);
    void  FillMacCountersTlv(MacCountersTlv &aTlv);
    Error AppendRequestedTlvs(const Message &aRequest, Message &aResponse, Tlv &aTlv);
    void  PrepareMessageInfoForDest(const Ip6::Address &aDestination, Tmf::MessageInfo &aMessageInfo) const;

    static void HandleGetResponse(void                *aContext,
                                  otMessage           *aMessage,
                                  const otMessageInfo *aMessageInfo,
                                  Error                aResult);
    void        HandleGetResponse(Coap::Message *aMessage, const Ip6::MessageInfo *aMessageInfo, Error aResult);

    template <Uri kUri> void HandleTmf(Coap::Message &aMessage, const Ip6::MessageInfo &aMessageInfo);

#if OT_SHOULD_LOG_AT(OT_LOG_LEVEL_INFO)
    static const char *UriToString(Uri aUri);
    void               Log(Action aAction, Uri aUri, const Ip6::Address &aIp6Address) const;
#else
    void Log(Action, Uri, const Ip6::Address &) const {}
#endif

    Callback<GetCallback> mGetCallback;
};

DeclareTmfHandler(NetworkDiagnostic, kUriDiagnosticGetRequest);
DeclareTmfHandler(NetworkDiagnostic, kUriDiagnosticGetQuery);
DeclareTmfHandler(NetworkDiagnostic, kUriDiagnosticGetAnswer);
DeclareTmfHandler(NetworkDiagnostic, kUriDiagnosticReset);

/**
 * @}
 */
} // namespace NetworkDiagnostic

} // namespace ot

#endif // OPENTHREAD_FTD || OPENTHREAD_CONFIG_TMF_NETWORK_DIAG_MTD_ENABLE

#endif // NETWORK_DIAGNOSTIC_HPP_
