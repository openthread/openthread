/*
 *  Copyright (c) 2021, The OpenThread Authors.
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
 *   This file implements the DNS-SD server.
 */

#include "dnssd_server.hpp"

#if OPENTHREAD_CONFIG_DNSSD_SERVER_ENABLE

#include "common/code_utils.hpp"
#include "common/debug.hpp"
#include "common/instance.hpp"
#include "common/locator-getters.hpp"
#include "common/logging.hpp"
#include "net/srp_server.hpp"
#include "net/udp6.hpp"

using ot::Encoding::BigEndian::HostSwap16;

namespace ot {
namespace Dns {
namespace ServiceDiscovery {

const char Server::kDnssdProtocolUdp[4] = {'_', 'u', 'd', 'p'};
const char Server::kDnssdProtocolTcp[4] = {'_', 't', 'c', 'p'};
const char Server::kDefaultDomainName[] = "default.service.arpa.";

Server::Server(Instance &aInstance)
    : InstanceLocator(aInstance)
    , mSocket(aInstance)
{
}

otError Server::Start(void)
{
    otError error = OT_ERROR_NONE;

    VerifyOrExit(!IsRunning());

    SuccessOrExit(error = mSocket.Open(&Server::HandleUdpReceive, this));
    SuccessOrExit(error = mSocket.Bind(kPort));

exit:
    otLogInfoDns("[server] started: %s", otThreadErrorToString(error));
    return error;
}

void Server::Stop(void)
{
    IgnoreError(mSocket.Close());
    otLogInfoDns("[server] stopped");
}

void Server::HandleUdpReceive(void *aContext, otMessage *aMessage, const otMessageInfo *aMessageInfo)
{
    static_cast<Server *>(aContext)->HandleUdpReceive(*static_cast<Message *>(aMessage),
                                                      *static_cast<const Ip6::MessageInfo *>(aMessageInfo));
}

void Server::HandleUdpReceive(Message &aMessage, const Ip6::MessageInfo &aMessageInfo)
{
    otError  error = OT_ERROR_NONE;
    Header   requestHeader;
    Message *responseMessage = nullptr;

    SuccessOrExit(error = aMessage.Read(aMessage.GetOffset(), requestHeader));
    VerifyOrExit(requestHeader.GetType() == Header::kTypeQuery, error = OT_ERROR_DROP);

    responseMessage = mSocket.NewMessage(0);
    VerifyOrExit(responseMessage != nullptr, error = OT_ERROR_NO_BUFS);

    // Allocate space for DNS header
    SuccessOrExit(error = responseMessage->SetLength(sizeof(Header)));

    // ProcessQuery is assumed to always prepare the response DNS message and header properly even in the case of system
    // failures (e.g. no more buffers).
    ProcessQuery(aMessage, *responseMessage, requestHeader);

    error = mSocket.SendTo(*responseMessage, aMessageInfo);

exit:
    FreeMessageOnError(responseMessage, error);
}

void Server::ProcessQuery(Message &aMessage, Message &aResponse, const Header &aRequestHeader)
{
    Header           responseHeader;
    uint16_t         readOffset, nameSerializeOffset;
    Question         question;
    uint16_t         qtype;
    char             name[Dns::Name::kMaxNameSize];
    otError          error = OT_ERROR_NONE;
    NameCompressInfo compressInfo;

    // Setup initial DNS response header
    responseHeader.Clear();
    responseHeader.SetResponseCode(Header::kResponseSuccess);
    responseHeader.SetType(Header::kTypeResponse);
    responseHeader.SetMessageId(aRequestHeader.GetMessageId());

    // Validate the query
    VerifyOrExit(aRequestHeader.GetQueryType() == Header::kQueryTypeStandard,
                 responseHeader.SetResponseCode(Header::kResponseNotImplemented));
    VerifyOrExit(!aRequestHeader.IsTruncationFlagSet(), responseHeader.SetResponseCode(Header::kResponseFormatError));
    VerifyOrExit(aRequestHeader.GetQuestionCount() == 1,
                 responseHeader.SetResponseCode(Header::kResponseNotImplemented));

    // Read the query name and question
    readOffset = sizeof(Header);
    VerifyOrExit(OT_ERROR_NONE == Dns::Name::ReadName(aMessage, readOffset, name, sizeof(name)),
                 responseHeader.SetResponseCode(Header::kResponseFormatError));
    VerifyOrExit(OT_ERROR_NONE == aMessage.Read(readOffset, question),
                 responseHeader.SetResponseCode(Header::kResponseFormatError));

    // Add the question to the response, and save the serialize offset of name
    nameSerializeOffset = aResponse.GetLength();
    SuccessOrExit(error = AddQuestionToResponse(name, question, aResponse, responseHeader));

    // Further validate the query
    qtype = question.GetType();
    VerifyOrExit(qtype == ResourceRecord::kTypePtr || qtype == ResourceRecord::kTypeSrv ||
                     qtype == ResourceRecord::kTypeTxt || qtype == ResourceRecord::kTypeAaaa,
                 responseHeader.SetResponseCode(Header::kResponseNotImplemented));

    VerifyOrExit(question.GetClass() == ResourceRecord::kClassInternet ||
                     question.GetClass() == ResourceRecord::kClassAny,
                 responseHeader.SetResponseCode(Header::kResponseNotImplemented));

    // Prepare the information for name compression
    VerifyOrExit(OT_ERROR_NONE == PrepareCompressInfo(qtype, name, nameSerializeOffset, compressInfo),
                 responseHeader.SetResponseCode(Header::kResponseNameError));

    // Resolve the question
    SuccessOrExit(error = ResolveQuestion(name, question, responseHeader, aResponse, compressInfo));

    otLogInfoDns("[server] TRANSACTION=0x%04x, QUESTION=[%s %d %d], RCODE=%d, ANSWER=%d, ADDITIONAL=%d",
                 aRequestHeader.GetMessageId(), name, question.GetClass(), question.GetType(),
                 responseHeader.GetResponseCode(), responseHeader.GetQuestionCount(),
                 responseHeader.GetAdditionalRecordCount());
exit:
    if (error != OT_ERROR_NONE)
    {
        otLogWarnDns("[server] failed to handle DNS query: %s", otThreadErrorToString(error));

        responseHeader.SetQuestionCount(0);
        responseHeader.SetAnswerCount(0);
        responseHeader.SetAdditionalRecordCount(0);
        responseHeader.SetResponseCode(Header::kResponseServerFailure);
        IgnoreError(aResponse.SetLength(sizeof(Header)));
    }

    aResponse.Write(0, responseHeader);
}

otError Server::AddQuestionToResponse(const char *    aName,
                                      const Question &aQuestion,
                                      Message &       aResponse,
                                      Header &        aResponseHeader)
{
    otError error = OT_ERROR_NONE;

    SuccessOrExit(error = Dns::Name::AppendName(aName, aResponse));
    SuccessOrExit(error = aResponse.Append(aQuestion));

    aResponseHeader.SetQuestionCount(1);

exit:
    return error;
}

otError Server::PrepareCompressInfo(uint16_t                  aQueryType,
                                    const char *              aName,
                                    uint16_t                  aNameSerializeOffset,
                                    Server::NameCompressInfo &aCompressInfo)
{
    const char *             domain = kDefaultDomainName;
    NameComponentsOffsetInfo nameComponentsInfo;
    otError                  error = OT_ERROR_NONE;

#if OPENTHREAD_CONFIG_SRP_SERVER_ENABLE
    domain = Get<Srp::Server>().GetDomain();
#endif

    SuccessOrExit(error = FindNameComponents(aName, domain, nameComponentsInfo));

    switch (aQueryType)
    {
    case ResourceRecord::kTypePtr:
        VerifyOrExit(nameComponentsInfo.IsServiceName(), error = OT_ERROR_INVALID_ARGS);
        aCompressInfo.SetServiceNameOffset(aNameSerializeOffset, aName);
        break;

    case ResourceRecord::kTypeSrv:
    case ResourceRecord::kTypeTxt:
        VerifyOrExit(nameComponentsInfo.IsServiceInstanceName(), error = OT_ERROR_INVALID_ARGS);
        aCompressInfo.SetInstanceNameOffset(aNameSerializeOffset, aName);
        aCompressInfo.SetServiceNameOffset(aNameSerializeOffset + nameComponentsInfo.mServiceOffset,
                                           aName + nameComponentsInfo.mServiceOffset);
        break;

    case ResourceRecord::kTypeAaaa:
        VerifyOrExit(nameComponentsInfo.IsHostName(), error = OT_ERROR_INVALID_ARGS);
        aCompressInfo.SetHostNameOffset(aNameSerializeOffset, aName);
        break;

    default:
        OT_ASSERT(false);
    }

    OT_ASSERT(nameComponentsInfo.mDomainOffset != NameComponentsOffsetInfo::kNotPresent);
    aCompressInfo.SetDomainNameOffset(aNameSerializeOffset + nameComponentsInfo.mDomainOffset,
                                      aName + nameComponentsInfo.mDomainOffset);

exit:
    return error;
}

otError Server::ResolveQuestion(const char *      aName,
                                const Question &  aQuestion,
                                Header &          aResponseHeader,
                                Message &         aResponseMessage,
                                NameCompressInfo &aCompressInfo)
{
    OT_UNUSED_VARIABLE(aName);
    OT_UNUSED_VARIABLE(aQuestion);
    OT_UNUSED_VARIABLE(aResponseHeader);
    OT_UNUSED_VARIABLE(aResponseMessage);
    OT_UNUSED_VARIABLE(aCompressInfo);

    otError error = OT_ERROR_NONE;

#if OPENTHREAD_CONFIG_SRP_SERVER_ENABLE
    SuccessOrExit(error = ResolveQuestionBySrp(aName, aQuestion, aResponseHeader, aResponseMessage,
                                               /* aAdditional */ false, aCompressInfo));

    if (aResponseHeader.GetAnswerCount() > 0)
    {
        SuccessOrExit(error = ResolveQuestionBySrp(aName, aQuestion, aResponseHeader, aResponseMessage,
                                                   /* aAdditional */ true, aCompressInfo));
    }
    else
    {
        aResponseHeader.SetResponseCode(Header::kResponseNameError);
    }

exit:
#endif
    return error;
}

otError Server::AppendPtrRecord(Message &         aMessage,
                                const char *      aServiceName,
                                const char *      aInstanceName,
                                uint32_t          aTtl,
                                NameCompressInfo &aCompressInfo)
{
    otError   error;
    PtrRecord ptrRecord;
    uint16_t  recordOffset;

    ptrRecord.Init();
    ptrRecord.SetTtl(aTtl);

    SuccessOrExit(error = AppendServiceName(aMessage, aServiceName, aCompressInfo));

    recordOffset = aMessage.GetLength();
    SuccessOrExit(error = aMessage.SetLength(recordOffset + sizeof(ptrRecord)));

    SuccessOrExit(error = AppendInstanceName(aMessage, aInstanceName, aCompressInfo));

    ptrRecord.SetLength(aMessage.GetLength() - (recordOffset + sizeof(ResourceRecord)));
    aMessage.Write(recordOffset, ptrRecord);

exit:
    return error;
}

otError Server::AppendSrvRecord(Message &         aMessage,
                                const char *      aInstanceName,
                                const char *      aHostName,
                                uint32_t          aTtl,
                                uint16_t          aPriority,
                                uint16_t          aWeight,
                                uint16_t          aPort,
                                NameCompressInfo &aCompressInfo)
{
    SrvRecord srvRecord;
    otError   error = OT_ERROR_NONE;
    uint16_t  recordOffset;

    srvRecord.Init();
    srvRecord.SetTtl(aTtl);
    srvRecord.SetPriority(aPriority);
    srvRecord.SetWeight(aWeight);
    srvRecord.SetPort(aPort);

    SuccessOrExit(error = AppendInstanceName(aMessage, aInstanceName, aCompressInfo));

    recordOffset = aMessage.GetLength();
    SuccessOrExit(error = aMessage.SetLength(recordOffset + sizeof(srvRecord)));

    SuccessOrExit(error = AppendHostName(aMessage, aHostName, aCompressInfo));

    srvRecord.SetLength(aMessage.GetLength() - (recordOffset + sizeof(ResourceRecord)));
    aMessage.Write(recordOffset, srvRecord);

exit:
    return error;
}

otError Server::AppendAaaaRecord(Message &           aMessage,
                                 const char *        aHostName,
                                 const Ip6::Address &aAddress,
                                 uint32_t            aTtl,
                                 NameCompressInfo &  aCompressInfo)
{
    AaaaRecord aaaaRecord;
    otError    error;

    aaaaRecord.Init();
    aaaaRecord.SetTtl(aTtl);
    aaaaRecord.SetAddress(aAddress);

    SuccessOrExit(error = AppendHostName(aMessage, aHostName, aCompressInfo));
    error = aMessage.Append(aaaaRecord);

exit:
    return error;
}

otError Server::AppendServiceName(Message &aMessage, const char *aName, NameCompressInfo &aCompressInfo)
{
    OT_UNUSED_VARIABLE(aName);
    OT_ASSERT(strcmp(aCompressInfo.GetServiceName(), aName) == 0);

    return Dns::Name::AppendPointerLabel(aCompressInfo.GetServiceNameOffset(), aMessage);
}

otError Server::AppendInstanceName(Message &aMessage, const char *aName, NameCompressInfo &aCompressInfo)
{
    otError error;

    uint16_t nameOffset = aCompressInfo.GetInstanceNameOffset(aName);

    if (nameOffset != NameCompressInfo::kUnknownOffset)
    {
        error = Dns::Name::AppendPointerLabel(nameOffset, aMessage);
    }
    else
    {
        uint8_t serviceStart = static_cast<uint8_t>(StringLength(aName, Name::kMaxNameLength) -
                                                    StringLength(aCompressInfo.GetServiceName(), Name::kMaxNameLength));

        aCompressInfo.SetInstanceNameOffset(aMessage.GetLength(), aName);

        SuccessOrExit(error = Dns::Name::AppendLabel(aName, serviceStart - 1, aMessage));
        error = Dns::Name::AppendPointerLabel(aCompressInfo.GetServiceNameOffset(), aMessage);
    }

exit:
    return error;
}

otError Server::AppendHostName(Message &aMessage, const char *aName, NameCompressInfo &aCompressInfo)
{
    otError  error;
    uint16_t hostCompressOffset = aCompressInfo.GetHostNameOffset(aName);

    if (hostCompressOffset != NameCompressInfo::kUnknownOffset)
    {
        error = Dns::Name::AppendPointerLabel(hostCompressOffset, aMessage);
    }
    else
    {
        uint8_t domainStart = static_cast<uint8_t>(StringLength(aName, Name::kMaxNameLength) -
                                                   StringLength(aCompressInfo.GetDomainName(), Name::kMaxNameLength));

        aCompressInfo.SetHostNameOffset(aMessage.GetLength(), aName);

        SuccessOrExit(error = Dns::Name::AppendMultipleLabels(aName, domainStart, aMessage));
        error = Dns::Name::AppendPointerLabel(aCompressInfo.GetDomainNameOffset(), aMessage);
    }

exit:
    return error;
}

void Server::IncResourceRecordCount(Header &aHeader, bool aAdditional)
{
    if (aAdditional)
    {
        aHeader.SetAdditionalRecordCount(aHeader.GetAdditionalRecordCount() + 1);
    }
    else
    {
        aHeader.SetAnswerCount(aHeader.GetAnswerCount() + 1);
    }
}

otError Server::FindNameComponents(const char *aName, const char *aDomain, NameComponentsOffsetInfo &aInfo)
{
    uint8_t nameLen   = static_cast<uint8_t>(StringLength(aName, Name::kMaxNameLength));
    uint8_t domainLen = static_cast<uint8_t>(StringLength(aDomain, Name::kMaxNameLength));
    otError error     = OT_ERROR_NONE;
    uint8_t labelBegin, labelEnd;

    VerifyOrExit(Dns::Name::IsSubDomainOf(aName, aDomain), error = OT_ERROR_INVALID_ARGS);

    labelBegin          = nameLen - domainLen;
    aInfo.mDomainOffset = labelBegin;

    while (true)
    {
        error = FindPreviousLabel(aName, labelBegin, labelEnd);

        VerifyOrExit(error == OT_ERROR_NONE, error = (error == OT_ERROR_NOT_FOUND ? OT_ERROR_NONE : error));

        if (labelEnd == labelBegin + kProtocolLabelLength &&
            (memcmp(&aName[labelBegin], kDnssdProtocolUdp, kProtocolLabelLength) == 0 ||
             memcmp(&aName[labelBegin], kDnssdProtocolTcp, kProtocolLabelLength) == 0))
        {
            // <Protocol> label found
            aInfo.mProtocolOffset = labelBegin;
            break;
        }
    }

    // Get service label <Service>
    error = FindPreviousLabel(aName, labelBegin, labelEnd);
    VerifyOrExit(error == OT_ERROR_NONE, error = (error == OT_ERROR_NOT_FOUND ? OT_ERROR_NONE : error));

    aInfo.mServiceOffset = labelBegin;

    // Treat everything before <Service> as <Instance> label
    error = FindPreviousLabel(aName, labelBegin, labelEnd);
    VerifyOrExit(error == OT_ERROR_NONE, error = (error == OT_ERROR_NOT_FOUND ? OT_ERROR_NONE : error));

    aInfo.mInstanceOffset = 0;

exit:
    return error;
}

otError Server::FindPreviousLabel(const char *aName, uint8_t &aStart, uint8_t &aStop)
{
    // This method finds the previous label before the current label (whose start index is @p aStart), and updates @p
    // aStart to the start index of the label and @p aStop to the index of the dot just after the label.
    // @note The input value of @p aStop does not matter because it is only used to output.

    otError error = OT_ERROR_NONE;
    uint8_t start = aStart;
    uint8_t end;

    VerifyOrExit(start > 0, error = OT_ERROR_NOT_FOUND);
    VerifyOrExit(aName[--start] == Name::kLabelSeperatorChar, error = OT_ERROR_INVALID_ARGS);

    end = start;
    while (start > 0 && aName[start - 1] != Name::kLabelSeperatorChar)
    {
        start--;
    }

    VerifyOrExit(start < end, error = OT_ERROR_INVALID_ARGS);

    aStart = start;
    aStop  = end;

exit:
    return error;
}

#if OPENTHREAD_CONFIG_SRP_SERVER_ENABLE
otError Server::ResolveQuestionBySrp(const char *      aName,
                                     const Question &  aQuestion,
                                     Header &          aResponseHeader,
                                     Message &         aResponseMessage,
                                     bool              aAdditional,
                                     NameCompressInfo &aCompressInfo)
{
    otError                  error = OT_ERROR_NONE;
    const Srp::Server::Host *host  = nullptr;
    TimeMilli                now   = TimerMilli::GetNow();
    uint16_t                 qtype = aQuestion.GetType();

    while ((host = GetNextSrpHost(host)) != nullptr)
    {
        bool        needAdditionalAaaaRecord = false;
        const char *hostName                 = host->GetFullName();

        // Handle PTR/SRV/TXT query
        if (qtype == ResourceRecord::kTypePtr || qtype == ResourceRecord::kTypeSrv || qtype == ResourceRecord::kTypeTxt)
        {
            const Srp::Server::Service *service = nullptr;

            while ((service = GetNextSrpService(*host, service)) != nullptr)
            {
                uint32_t    instanceTtl         = TimeMilli::MsecToSec(service->GetExpireTime() - TimerMilli::GetNow());
                const char *instanceName        = service->GetFullName();
                bool        serviceNameMatched  = service->MatchesServiceName(aName);
                bool        instanceNameMatched = service->Matches(aName);
                bool        ptrQueryMatched     = qtype == ResourceRecord::kTypePtr && serviceNameMatched;
                bool        srvQueryMatched     = qtype == ResourceRecord::kTypeSrv && instanceNameMatched;
                bool        txtQueryMatched     = qtype == ResourceRecord::kTypeTxt && instanceNameMatched;

                if (ptrQueryMatched || srvQueryMatched)
                {
                    needAdditionalAaaaRecord = true;
                }

                if (!aAdditional && ptrQueryMatched)
                {
                    SuccessOrExit(
                        error = AppendPtrRecord(aResponseMessage, aName, instanceName, instanceTtl, aCompressInfo));
                    IncResourceRecordCount(aResponseHeader, aAdditional);
                }

                if ((!aAdditional && srvQueryMatched) || (aAdditional && ptrQueryMatched))
                {
                    SuccessOrExit(error = AppendSrvRecord(aResponseMessage, instanceName, hostName, instanceTtl,
                                                          service->GetPriority(), service->GetWeight(),
                                                          service->GetPort(), aCompressInfo));
                    IncResourceRecordCount(aResponseHeader, aAdditional);
                }

                if ((!aAdditional && txtQueryMatched) || (aAdditional && ptrQueryMatched))
                {
                    SuccessOrExit(
                        error = AppendTxtRecord(aResponseMessage, instanceName, *service, instanceTtl, aCompressInfo));
                    IncResourceRecordCount(aResponseHeader, aAdditional);
                }
            }
        }

        // Handle AAAA query
        if ((!aAdditional && qtype == ResourceRecord::kTypeAaaa && host->Matches(aName)) ||
            (aAdditional && needAdditionalAaaaRecord))
        {
            uint8_t             addrNum;
            const Ip6::Address *addrs   = host->GetAddresses(addrNum);
            uint32_t            hostTtl = TimeMilli::MsecToSec(host->GetExpireTime() - now);

            for (uint8_t i = 0; i < addrNum; i++)
            {
                SuccessOrExit(error = AppendAaaaRecord(aResponseMessage, hostName, addrs[i], hostTtl, aCompressInfo));
                IncResourceRecordCount(aResponseHeader, aAdditional);
            }
        }
    }

exit:
    return error;
}

const Srp::Server::Host *Server::GetNextSrpHost(const Srp::Server::Host *aHost)
{
    const Srp::Server::Host *host = Get<Srp::Server>().GetNextHost(aHost);

    while (host != nullptr && host->IsDeleted())
    {
        host = Get<Srp::Server>().GetNextHost(host);
    }

    return host;
}

const Srp::Server::Service *Server::GetNextSrpService(const Srp::Server::Host &   aHost,
                                                      const Srp::Server::Service *aService)
{
    const Srp::Server::Service *service = aHost.GetNextService(aService);

    while (service != nullptr && service->IsDeleted())
    {
        service = aHost.GetNextService(service);
    }

    return service;
}

otError Server::AppendTxtRecord(Message &                   aMessage,
                                const char *                aInstanceName,
                                const Srp::Server::Service &aService,
                                uint32_t                    aTtl,
                                NameCompressInfo &          aCompressInfo)
{
    TxtRecord                   txtRecord;
    otError                     error;
    Dns::TxtRecord::TxtIterator txtIterator = OT_DNS_TXT_ITERATOR_INIT;
    Dns::TxtEntry               txtEntry;
    uint16_t                    recordOffset;
    bool                        hasAnyTxtEntry = false;

    SuccessOrExit(error = AppendInstanceName(aMessage, aInstanceName, aCompressInfo));

    recordOffset = aMessage.GetLength();
    SuccessOrExit(error = aMessage.SetLength(recordOffset + sizeof(txtRecord)));

    while (aService.GetNextTxtEntry(txtIterator, txtEntry) == OT_ERROR_NONE)
    {
        SuccessOrExit(error = txtEntry.AppendTo(aMessage));
        hasAnyTxtEntry = true;
    }

    if (!hasAnyTxtEntry)
    {
        SuccessOrExit(error = aMessage.Append<uint8_t>(0));
    }

    txtRecord.Init();
    txtRecord.SetTtl(aTtl);
    txtRecord.SetLength(aMessage.GetLength() - (recordOffset + sizeof(ResourceRecord)));

    aMessage.Write(recordOffset, txtRecord);

exit:
    return error;
}
#endif // OPENTHREAD_CONFIG_SRP_SERVER_ENABLE

} // namespace ServiceDiscovery
} // namespace Dns
} // namespace ot

#endif // OPENTHREAD_CONFIG_DNS_SERVER_ENABLE
