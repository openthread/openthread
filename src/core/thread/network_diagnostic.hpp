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

#ifndef OT_CORE_THREAD_NETWORK_DIAGNOSTIC_HPP_
#define OT_CORE_THREAD_NETWORK_DIAGNOSTIC_HPP_

#include "openthread-core-config.h"

#include <openthread/netdiag.h>

#include "common/bit_set.hpp"
#include "common/callback.hpp"
#include "common/locator.hpp"
#include "common/non_copyable.hpp"
#include "common/numeric_limits.hpp"
#include "net/udp6.hpp"
#include "thread/network_diagnostic_tlvs.hpp"
#include "thread/tmf.hpp"
#include "thread/uri_paths.hpp"

namespace ot {

namespace Utils {
class MeshDiag;
}

namespace MeshCoP {
class TcatAgent;
}

namespace NetworkDiagnostic {

/**
 * @addtogroup core-netdiag
 *
 * @brief
 *   This module includes definitions for sending and handling Network Diagnostic Commands.
 *
 * @{
 */

class Client;

/**
 * Implements the Network Diagnostic server responding to requests.
 */
class Server : public InstanceLocator, private NonCopyable
{
    friend class Tmf::Agent;
    friend class MeshCoP::TcatAgent;
    friend class Client;

public:
    /**
     * Callback function pointer to notify a reset request for `kNonPreferredChannels` TLV value.
     */
    typedef otThreadNonPreferredChannelsResetCallback NonPreferredChannelsResetCallback;

    /**
     * Initializes the Server.
     *
     * @param[in] aInstance   The OpenThread instance.
     */
    explicit Server(Instance &aInstance);

    /**
     * Sets the non-preferred channels value for `kNonPreferredChannels` TLV.
     *
     * @param[in] aChannelMask   A channel mask for non-preferred channels.
     */
    void SetNonPreferredChannels(uint32_t aChannelMask) { mNonPreferredChannels = aChannelMask; }

    /**
     * Gets the non-preferred channel mask value for `kNonPreferredChannels` TLV.
     *
     * @returns The non-preferred channels as a channel mask.
     */
    uint32_t GetNonPreferredChannels(void) const { return mNonPreferredChannels; }

    /**
     * Sets the callback to notify when a Diagnostic Reset request is received for `kNonPreferredChannels` TLV value.
     *
     * @param[in] aCallback   The callback function pointer.
     * @param[in] aContext    An arbitrary context used with @p aCallback.
     */
    void SetNonPreferredChannelsResetCallback(NonPreferredChannelsResetCallback aCallback, void *aContext)
    {
        mNonPreferredChannelsResetCallback.Set(aCallback, aContext);
    }

private:
    static constexpr uint16_t kMaxChildEntries              = 398;
    static constexpr uint16_t kAnswerMessageLengthThreshold = 800;

    typedef BitSet<NumericLimits<uint8_t>::kMax + 1> TlvTypeBitSet; // A bitset to store TLV types.

#if OPENTHREAD_FTD
    struct AnswerInfo
    {
        AnswerInfo(void)
            : mAnswerIndex(0)
            , mQueryId(0)
            , mHasQueryId(false)
            , mFirstAnswer(nullptr)
        {
        }

        uint16_t          mAnswerIndex;
        uint16_t          mQueryId;
        bool              mHasQueryId;
        Message::Priority mPriority;
        Coap::Message    *mFirstAnswer;
    };
#endif

    Error AppendDiagTlv(uint8_t aTlvType, Message &aMessage);
    Error AppendIp6AddressList(Message &aMessage);
    Error AppendMacCounters(Message &aMessage);
    Error AppendRequestedTlvs(const Message &aRequest, Message &aResponse);
    void  PrepareMessageInfoForDest(const Ip6::Address &aDestination, Tmf::MessageInfo &aMessageInfo) const;

#if OPENTHREAD_CONFIG_BLE_TCAT_ENABLE
    Error AppendRequestedTlvsForTcat(const Message &aRequest, Message &aResponse, OffsetRange &aOffsetRange);
#endif

#if OPENTHREAD_MTD
    void SendAnswer(const Ip6::Address &aDestination, const Message &aRequest);
#elif OPENTHREAD_FTD
    Error AllocateAnswer(Coap::Message *&aAnswer, AnswerInfo &aInfo);
    Error CheckAnswerLength(Coap::Message *&aAnswer, AnswerInfo &aInfo);
    bool  IsLastAnswer(const Coap::Message &aAnswer) const;
    void  FreeAllRelatedAnswers(Coap::Message &aFirstAnswer);
    void  PrepareAndSendAnswers(const Ip6::Address &aDestination, const Message &aRequest);
    void  SendNextAnswer(Coap::Message &aAnswer, const Ip6::Address &aDestination);
    Error AppendChildTable(Message &aMessage);
    Error AppendChildTableAsChildTlvs(Coap::Message *&aAnswer, AnswerInfo &aInfo);
    Error AppendRouterNeighborTlvs(Coap::Message *&aAnswer, AnswerInfo &aInfo);
    Error AppendChildTableIp6AddressList(Coap::Message *&aAnswer, AnswerInfo &aInfo);
    Error AppendChildIp6AddressListTlv(Message &aAnswer, const Child &aChild);
    Error AppendEnhancedRoute(Message &aMessage);

#if OPENTHREAD_CONFIG_BLE_TCAT_ENABLE
    Error AppendChildTableAsChildTlvs(Message &aMessage);
    Error AppendRouterNeighborTlvs(Message &aMessage);
    Error AppendChildTableIp6AddressList(Message &aMessage);
#endif

    static void HandleAnswerResponse(void                *aContext,
                                     otMessage           *aMessage,
                                     const otMessageInfo *aMessageInfo,
                                     otError              aResult);
    void        HandleAnswerResponse(Coap::Message          &aNextAnswer,
                                     Coap::Message          *aResponse,
                                     const Ip6::MessageInfo *aMessageInfo,
                                     Error                   aResult);
#endif
#if OPENTHREAD_CONFIG_BORDER_ROUTING_ENABLE
    Error AppendBorderRouterIfAddrs(Message &aMessage);
    Error AppendBrPrefixTlv(uint8_t aTlvType, Message &aMessage);
#endif

    template <Uri kUri> void HandleTmf(Coap::Msg &aMsg);

#if OPENTHREAD_FTD
    Coap::MessageQueue mAnswerQueue;
#endif
    uint32_t                                    mNonPreferredChannels;
    Callback<NonPreferredChannelsResetCallback> mNonPreferredChannelsResetCallback;
};

DeclareTmfHandler(Server, kUriDiagnosticGetRequest);
DeclareTmfHandler(Server, kUriDiagnosticGetQuery);
DeclareTmfHandler(Server, kUriDiagnosticGetAnswer);

#if OPENTHREAD_CONFIG_TMF_NETDIAG_CLIENT_ENABLE

/**
 * Implements the Network Diagnostic client sending requests and queries.
 */
class Client : public InstanceLocator, private NonCopyable
{
    friend class Tmf::Agent;
    friend class Utils::MeshDiag;

public:
    typedef otNetworkDiagIterator          Iterator;    ///< Iterator to go through TLVs in `GetNextDiagTlv()`.
    typedef otNetworkDiagTlv               DiagTlv;     ///< Parse info from a Network Diagnostic TLV.
    typedef otNetworkDiagChildEntry        ChildInfo;   ///< Parsed info for child table entry.
    typedef otReceiveDiagnosticGetCallback GetCallback; ///< Diagnostic Get callback function pointer type.

    static constexpr Iterator kIteratorInit = OT_NETWORK_DIAGNOSTIC_ITERATOR_INIT; ///< Initializer for Iterator.

    /**
     * Initializes the Client.
     *
     * @param[in] aInstance   The OpenThread instance.
     */
    explicit Client(Instance &aInstance);

    /**
     * Sends Diagnostic Get request. If the @p aDestination is of multicast type, the DIAG_GET.qry
     * message is sent or the DIAG_GET.req otherwise.
     *
     * @param[in]  aDestination      The destination address.
     * @param[in]  aTlvTypes         An array of Network Diagnostic TLV types.
     * @param[in]  aCount            Number of types in @p aTlvTypes.
     * @param[in]  aCallback         Callback when Network Diagnostic Get response is received (can be NULL).
     * @param[in]  Context           Application-specific context used with @p aCallback.
     */
    Error SendDiagnosticGet(const Ip6::Address &aDestination,
                            const uint8_t       aTlvTypes[],
                            uint8_t             aCount,
                            GetCallback         aCallback,
                            void               *Context);

    /**
     * Sends Diagnostic Reset request.
     *
     * @param[in] aDestination  The destination address.
     * @param[in] aTlvTypes     An array of Network Diagnostic TLV types.
     * @param[in] aCount        Number of types in aTlvTypes
     */
    Error SendDiagnosticReset(const Ip6::Address &aDestination, const uint8_t aTlvTypes[], uint8_t aCount);

    /**
     * Gets the next Network Diagnostic TLV in a given message.
     *
     * @param[in]      aMessage    Message to read TLVs from.
     * @param[in,out]  aIterator   The Network Diagnostic iterator. To get the first TLV set it to `kIteratorInit`.
     * @param[out]     aDiagTlv    A reference to a `DiagTlv` to output the next TLV data.
     *
     * @retval kErrorNone       Successfully found the next Network Diagnostic TLV.
     * @retval kErrorNotFound   No subsequent Network Diagnostic TLV exists in the message.
     * @retval kErrorParse      Parsing the next Network Diagnostic failed.
     */
    static Error GetNextDiagTlv(const Coap::Message &aMessage, Iterator &aIterator, DiagTlv &aDiagTlv);

    /**
     * This method returns the query ID used for the last Network Diagnostic Query command.
     *
     * @returns The query ID used for last query.
     */
    uint16_t GetLastQueryId(void) const { return mQueryId; }

private:
    typedef otNetworkDiagIp6AddrList Ip6AddrList;
    typedef otNetworkDiagMacCounters MacCounters;

    Error SendCommand(Uri                   aUri,
                      Message::Priority     aPriority,
                      const Ip6::Address   &aDestination,
                      const uint8_t         aTlvTypes[],
                      uint8_t               aCount,
                      Coap::ResponseHandler aHandler = nullptr,
                      void                 *aContext = nullptr);

    DeclareTmfResponseHandlerFullParamIn(Client, HandleGetResponse);

    template <Uri kUri> void HandleTmf(Coap::Msg &aMsg);

    static void ParseIp6AddrList(Ip6AddrList &aIp6Addrs, const Message &aMessage, OffsetRange aOffsetRange);
    static void ParseMacCounters(const MacCountersTlv &aMacCountersTlv, MacCounters &aMacCounters);

#if OT_SHOULD_LOG_AT(OT_LOG_LEVEL_INFO)
    static const char *UriToString(Uri aUri);
#endif

    uint16_t              mQueryId;
    Callback<GetCallback> mGetCallback;
};

DeclareTmfHandler(Client, kUriDiagnosticReset);

#endif // OPENTHREAD_CONFIG_TMF_NETDIAG_CLIENT_ENABLE

/**
 * @}
 */
} // namespace NetworkDiagnostic

} // namespace ot

#endif // OT_CORE_THREAD_NETWORK_DIAGNOSTIC_HPP_
