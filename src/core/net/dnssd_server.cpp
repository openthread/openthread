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

#include <openthread/platform/dns.h>

#include "common/array.hpp"
#include "common/as_core_type.hpp"
#include "common/code_utils.hpp"
#include "common/debug.hpp"
#include "common/instance.hpp"
#include "common/locator_getters.hpp"
#include "common/log.hpp"
#include "common/string.hpp"
#include "net/srp_server.hpp"
#include "net/udp6.hpp"

namespace ot {
namespace Dns {
namespace ServiceDiscovery {

RegisterLogModule("DnssdServer");

const char  Server::kDnssdProtocolUdp[]  = "_udp";
const char  Server::kDnssdProtocolTcp[]  = "_tcp";
const char  Server::kDnssdSubTypeLabel[] = "._sub.";
const char  Server::kDefaultDomainName[] = "default.service.arpa.";
const char *Server::kBlockedDomains[]    = {"ipv4only.arpa."};

Server::Server(Instance &aInstance)
    : InstanceLocator(aInstance)
    , mSocket(aInstance)
#if OPENTHREAD_CONFIG_DNS_UPSTREAM_QUERY_ENABLE
    , mEnableUpstreamQuery(false)
#endif
    , mTimer(aInstance)
    , mTestMode(kTestModeDisabled)
{
    mCounters.Clear();
}

Error Server::Start(void)
{
    Error error = kErrorNone;

    VerifyOrExit(!IsRunning());

    SuccessOrExit(error = mSocket.Open(&Server::HandleUdpReceive, this));
    SuccessOrExit(error = mSocket.Bind(kPort, kBindUnspecifiedNetif ? Ip6::kNetifUnspecified : Ip6::kNetifThread));

#if OPENTHREAD_CONFIG_SRP_SERVER_ENABLE
    Get<Srp::Server>().HandleDnssdServerStateChange();
#endif

exit:
    LogInfo("started: %s", ErrorToString(error));

    if (error != kErrorNone)
    {
        IgnoreError(mSocket.Close());
    }

    return error;
}

void Server::Stop(void)
{
    // Abort all query transactions
    for (QueryTransaction &query : mQueryTransactions)
    {
        if (query.IsValid())
        {
            query.Finalize(Header::kResponseServerFailure);
        }
    }

#if OPENTHREAD_CONFIG_DNS_UPSTREAM_QUERY_ENABLE
    for (UpstreamQueryTransaction &txn : mUpstreamQueryTransactions)
    {
        if (txn.IsValid())
        {
            ResetUpstreamQueryTransaction(txn, kErrorFailed);
        }
    }
#endif

    mTimer.Stop();

    IgnoreError(mSocket.Close());
    LogInfo("stopped");

#if OPENTHREAD_CONFIG_SRP_SERVER_ENABLE
    Get<Srp::Server>().HandleDnssdServerStateChange();
#endif
}

void Server::HandleUdpReceive(void *aContext, otMessage *aMessage, const otMessageInfo *aMessageInfo)
{
    static_cast<Server *>(aContext)->HandleUdpReceive(AsCoreType(aMessage), AsCoreType(aMessageInfo));
}

void Server::HandleUdpReceive(Message &aMessage, const Ip6::MessageInfo &aMessageInfo)
{
    Request request;

#if OPENTHREAD_CONFIG_SRP_SERVER_ENABLE
    // We first let the `Srp::Server` process the received message.
    // It returns `kErrorNone` to indicate that it successfully
    // processed the message.

    VerifyOrExit(Get<Srp::Server>().HandleDnssdServerUdpReceive(aMessage, aMessageInfo) != kErrorNone);
#endif

    request.mMessage     = &aMessage;
    request.mMessageInfo = &aMessageInfo;
    SuccessOrExit(aMessage.Read(aMessage.GetOffset(), request.mHeader));

    VerifyOrExit(request.mHeader.GetType() == Header::kTypeQuery);

    ProcessQuery(request);

exit:
    return;
}

void Server::ProcessQuery(const Request &aRequest)
{
    Error            error = kErrorNone;
    Response         response;
    bool             shouldSendResponse = true;
    Header::Response rcode              = Header::kResponseSuccess;

#if OPENTHREAD_CONFIG_DNS_UPSTREAM_QUERY_ENABLE
    if (mEnableUpstreamQuery && ShouldForwardToUpstream(aRequest))
    {
        error = ResolveByUpstream(aRequest);

        if (error == kErrorNone)
        {
            shouldSendResponse = false;
            ExitNow();
        }

        LogWarn("Failed to forward DNS query to upstream: %s", ErrorToString(error));

        error = kErrorNone;
        rcode = Header::kResponseServerFailure;

        // Continue to allocate and prepare the response message
        // to send the `kResponseServerFailure` response code.
    }
#endif

    response.mMessage = mSocket.NewMessage();
    VerifyOrExit(response.mMessage != nullptr, error = kErrorNoBufs);

    // Prepare DNS response header
    response.mHeader.SetType(Header::kTypeResponse);
    response.mHeader.SetMessageId(aRequest.mHeader.GetMessageId());
    response.mHeader.SetQueryType(aRequest.mHeader.GetQueryType());

    if (aRequest.mHeader.IsRecursionDesiredFlagSet())
    {
        response.mHeader.SetRecursionDesiredFlag();
    }

    // Append the empty header to reserve room for it in the message.
    // Header will be updated in the message before sending it.
    SuccessOrExit(error = response.mMessage->Append(response.mHeader));

#if OPENTHREAD_CONFIG_DNS_UPSTREAM_QUERY_ENABLE
    // Forwarding the query to the upstream may have already set the
    // response error code.
    VerifyOrExit(rcode == Header::kResponseSuccess);
#endif

    // Validate the query
    VerifyOrExit(aRequest.mHeader.GetQueryType() == Header::kQueryTypeStandard,
                 rcode = Header::kResponseNotImplemented);
    VerifyOrExit(!aRequest.mHeader.IsTruncationFlagSet(), rcode = Header::kResponseFormatError);
    VerifyOrExit(aRequest.mHeader.GetQuestionCount() > 0, rcode = Header::kResponseFormatError);

    if (mTestMode & kTestModeSingleQuestionOnly)
    {
        VerifyOrExit(aRequest.mHeader.GetQuestionCount() == 1, rcode = Header::kResponseFormatError);
    }

    SuccessOrExit(response.AddQuestionsFrom(aRequest));

#if OPENTHREAD_CONFIG_SRP_SERVER_ENABLE
    response.ResolveBySrp();

    if (response.mHeader.GetAnswerCount() != 0)
    {
        mCounters.mResolvedBySrp++;
        ExitNow();
    }
#endif

    if (ResolveByQueryCallbacks(response, *aRequest.mMessageInfo) == kErrorNone)
    {
        // `ResolveByQueryCallbacks()` will take ownership of the
        // allocated `response.mMessage` on success. Therefore,
        // there is no need to free it at `exit`.

        shouldSendResponse = false;
    }

exit:
    if ((error == kErrorNone) && shouldSendResponse)
    {
        if (rcode != Header::kResponseSuccess)
        {
            response.mHeader.SetResponseCode(rcode);
        }

        response.Send(*aRequest.mMessageInfo);
    }

    FreeMessageOnError(response.mMessage, error);
}

void Server::Response::Send(const Ip6::MessageInfo &aMessageInfo)
{
    Error            error;
    Header::Response rcode = mHeader.GetResponseCode();

    if (rcode == Header::kResponseServerFailure)
    {
        LogWarn("failed to handle DNS query due to server failure");
        mHeader.SetQuestionCount(0);
        mHeader.SetAnswerCount(0);
        mHeader.SetAdditionalRecordCount(0);
        IgnoreError(mMessage->SetLength(sizeof(Header)));
    }

    mMessage->Write(0, mHeader);

    error = Get<Server>().mSocket.SendTo(*mMessage, aMessageInfo);

    if (error != kErrorNone)
    {
        mMessage->Free();
        LogWarn("failed to send DNS-SD reply: %s", ErrorToString(error));
    }
    else
    {
        LogInfo("send DNS-SD reply: %s, RCODE=%d", ErrorToString(error), rcode);
    }

    Get<Server>().UpdateResponseCounters(rcode);
}

Error Server::Response::AddQuestionsFrom(const Request &aRequest)
{
    uint16_t         readOffset;
    Header::Response rcode = Header::kResponseSuccess;

    readOffset = sizeof(Header);

    for (uint16_t i = 0; i < aRequest.mHeader.GetQuestionCount(); i++)
    {
        char                     name[Name::kMaxNameSize];
        NameComponentsOffsetInfo nameInfo;
        Question                 question;

        VerifyOrExit(Name::ReadName(*aRequest.mMessage, readOffset, name, sizeof(name)) == kErrorNone,
                     rcode = Header::kResponseFormatError);
        VerifyOrExit(aRequest.mMessage->Read(readOffset, question) == kErrorNone, rcode = Header::kResponseFormatError);
        readOffset += sizeof(question);

        switch (question.GetType())
        {
        case ResourceRecord::kTypePtr:
        case ResourceRecord::kTypeSrv:
        case ResourceRecord::kTypeTxt:
        case ResourceRecord::kTypeAaaa:
            break;

        default:
            rcode = Header::kResponseNotImplemented;
            ExitNow();
        }

        VerifyOrExit(FindNameComponents(name, kDefaultDomainName, nameInfo) == kErrorNone,
                     rcode = Header::kResponseNameError);

        switch (question.GetType())
        {
        case ResourceRecord::kTypePtr:
            VerifyOrExit(nameInfo.IsServiceName(), rcode = Header::kResponseNameError);
            break;
        case ResourceRecord::kTypeSrv:
            VerifyOrExit(nameInfo.IsServiceInstanceName(), rcode = Header::kResponseNameError);
            break;
        case ResourceRecord::kTypeTxt:
            VerifyOrExit(nameInfo.IsServiceInstanceName(), rcode = Header::kResponseNameError);
            break;
        case ResourceRecord::kTypeAaaa:
            VerifyOrExit(nameInfo.IsHostName(), rcode = Header::kResponseNameError);
            break;
        default:
            break;
        }

        VerifyOrExit(AppendQuestion(name, question) == kErrorNone, rcode = Header::kResponseServerFailure);
    }

    mHeader.SetQuestionCount(aRequest.mHeader.GetQuestionCount());

exit:
    mHeader.SetResponseCode(rcode);
    return (rcode == Header::kResponseSuccess) ? kErrorNone : kErrorFailed;
}

Error Server::Response::AppendQuestion(const char *aName, const Question &aQuestion)
{
    Error error = kErrorNone;

    switch (aQuestion.GetType())
    {
    case ResourceRecord::kTypePtr:
        SuccessOrExit(error = AppendServiceName(aName));
        break;
    case ResourceRecord::kTypeSrv:
    case ResourceRecord::kTypeTxt:
        SuccessOrExit(error = AppendInstanceName(aName));
        break;
    case ResourceRecord::kTypeAaaa:
        SuccessOrExit(error = AppendHostName(aName));
        break;
    default:
        OT_ASSERT(false);
    }

    error = mMessage->Append(aQuestion);

exit:
    return error;
}

Error Server::Response::AppendPtrRecord(const char *aServiceName, const char *aInstanceName, uint32_t aTtl)
{
    Error     error;
    PtrRecord ptrRecord;
    uint16_t  recordOffset;

    ptrRecord.Init();
    ptrRecord.SetTtl(aTtl);

    SuccessOrExit(error = AppendServiceName(aServiceName));

    recordOffset = mMessage->GetLength();
    SuccessOrExit(error = mMessage->SetLength(recordOffset + sizeof(ptrRecord)));

    SuccessOrExit(error = AppendInstanceName(aInstanceName));

    ptrRecord.SetLength(mMessage->GetLength() - (recordOffset + sizeof(ResourceRecord)));
    mMessage->Write(recordOffset, ptrRecord);

    IncResourceRecordCount();

exit:
    return error;
}

Error Server::Response::AppendSrvRecord(const char *aInstanceName,
                                        const char *aHostName,
                                        uint32_t    aTtl,
                                        uint16_t    aPriority,
                                        uint16_t    aWeight,
                                        uint16_t    aPort)
{
    SrvRecord srvRecord;
    Error     error = kErrorNone;
    uint16_t  recordOffset;

    srvRecord.Init();
    srvRecord.SetTtl(aTtl);
    srvRecord.SetPriority(aPriority);
    srvRecord.SetWeight(aWeight);
    srvRecord.SetPort(aPort);

    SuccessOrExit(error = AppendInstanceName(aInstanceName));

    recordOffset = mMessage->GetLength();
    SuccessOrExit(error = mMessage->SetLength(recordOffset + sizeof(srvRecord)));

    SuccessOrExit(error = AppendHostName(aHostName));

    srvRecord.SetLength(mMessage->GetLength() - (recordOffset + sizeof(ResourceRecord)));
    mMessage->Write(recordOffset, srvRecord);

    IncResourceRecordCount();

exit:
    return error;
}

Error Server::Response::AppendAaaaRecord(const char *aHostName, const Ip6::Address &aAddress, uint32_t aTtl)
{
    AaaaRecord aaaaRecord;
    Error      error;

    aaaaRecord.Init();
    aaaaRecord.SetTtl(aTtl);
    aaaaRecord.SetAddress(aAddress);

    SuccessOrExit(error = AppendHostName(aHostName));
    SuccessOrExit(error = mMessage->Append(aaaaRecord));

    IncResourceRecordCount();

exit:
    return error;
}

Error Server::Response::AppendServiceName(const char *aName)
{
    Error       error;
    uint16_t    serviceCompressOffset = mCompressInfo.GetServiceNameOffset(*mMessage, aName);
    const char *serviceName;

    // Check whether `aName` is a sub-type service name.
    serviceName = StringFind(aName, kDnssdSubTypeLabel, kStringCaseInsensitiveMatch);

    if (serviceName != nullptr)
    {
        uint8_t subTypeLabelLength = static_cast<uint8_t>(serviceName - aName) + sizeof(kDnssdSubTypeLabel) - 1;

        SuccessOrExit(error = Name::AppendMultipleLabels(aName, subTypeLabelLength, *mMessage));

        // Skip over the "._sub." label to get to the root service name.
        serviceName += sizeof(kDnssdSubTypeLabel) - 1;
    }
    else
    {
        serviceName = aName;
    }

    if (serviceCompressOffset != NameCompressInfo::kUnknownOffset)
    {
        error = Name::AppendPointerLabel(serviceCompressOffset, *mMessage);
    }
    else
    {
        uint16_t domainStart = StringLength(serviceName, Name::kMaxNameSize - 1) - (sizeof(kDefaultDomainName) - 1);
        uint16_t domainCompressOffset = mCompressInfo.GetDomainNameOffset();

        serviceCompressOffset = mMessage->GetLength();
        mCompressInfo.SetServiceNameOffset(serviceCompressOffset);

        if (domainCompressOffset == NameCompressInfo::kUnknownOffset)
        {
            mCompressInfo.SetDomainNameOffset(serviceCompressOffset + domainStart);
            error = Name::AppendName(serviceName, *mMessage);
        }
        else
        {
            SuccessOrExit(error =
                              Name::AppendMultipleLabels(serviceName, static_cast<uint8_t>(domainStart), *mMessage));
            error = Name::AppendPointerLabel(domainCompressOffset, *mMessage);
        }
    }

exit:
    return error;
}

Error Server::Response::AppendInstanceName(const char *aName)
{
    Error    error;
    uint16_t instanceCompressOffset = mCompressInfo.GetInstanceNameOffset(*mMessage, aName);

    if (instanceCompressOffset != NameCompressInfo::kUnknownOffset)
    {
        error = Name::AppendPointerLabel(instanceCompressOffset, *mMessage);
    }
    else
    {
        NameComponentsOffsetInfo nameComponentsInfo;

        IgnoreError(FindNameComponents(aName, kDefaultDomainName, nameComponentsInfo));
        OT_ASSERT(nameComponentsInfo.IsServiceInstanceName());

        mCompressInfo.SetInstanceNameOffset(mMessage->GetLength());

        // Append the instance name as one label
        SuccessOrExit(error = Name::AppendLabel(aName, nameComponentsInfo.mServiceOffset - 1, *mMessage));

        {
            const char *serviceName           = aName + nameComponentsInfo.mServiceOffset;
            uint16_t    serviceCompressOffset = mCompressInfo.GetServiceNameOffset(*mMessage, serviceName);

            if (serviceCompressOffset != NameCompressInfo::kUnknownOffset)
            {
                error = Name::AppendPointerLabel(serviceCompressOffset, *mMessage);
            }
            else
            {
                mCompressInfo.SetServiceNameOffset(mMessage->GetLength());
                error = Name::AppendName(serviceName, *mMessage);
            }
        }
    }

exit:
    return error;
}

Error Server::Response::AppendTxtRecord(const char *aInstanceName,
                                        const void *aTxtData,
                                        uint16_t    aTxtLength,
                                        uint32_t    aTtl)
{
    Error         error = kErrorNone;
    TxtRecord     txtRecord;
    const uint8_t kEmptyTxt = 0;

    SuccessOrExit(error = AppendInstanceName(aInstanceName));

    txtRecord.Init();
    txtRecord.SetTtl(aTtl);
    txtRecord.SetLength(aTxtLength > 0 ? aTxtLength : sizeof(kEmptyTxt));

    SuccessOrExit(error = mMessage->Append(txtRecord));

    if (aTxtLength > 0)
    {
        SuccessOrExit(error = mMessage->AppendBytes(aTxtData, aTxtLength));
    }
    else
    {
        SuccessOrExit(error = mMessage->Append(kEmptyTxt));
    }

    IncResourceRecordCount();

exit:
    return error;
}

Error Server::Response::AppendHostName(const char *aName)
{
    Error    error;
    uint16_t hostCompressOffset = mCompressInfo.GetHostNameOffset(*mMessage, aName);

    if (hostCompressOffset != NameCompressInfo::kUnknownOffset)
    {
        error = Name::AppendPointerLabel(hostCompressOffset, *mMessage);
    }
    else
    {
        uint16_t domainStart          = StringLength(aName, Name::kMaxNameLength) - (sizeof(kDefaultDomainName) - 1);
        uint16_t domainCompressOffset = mCompressInfo.GetDomainNameOffset();

        hostCompressOffset = mMessage->GetLength();
        mCompressInfo.SetHostNameOffset(hostCompressOffset);

        if (domainCompressOffset == NameCompressInfo::kUnknownOffset)
        {
            mCompressInfo.SetDomainNameOffset(hostCompressOffset + domainStart);
            error = Name::AppendName(aName, *mMessage);
        }
        else
        {
            SuccessOrExit(error = Name::AppendMultipleLabels(aName, static_cast<uint8_t>(domainStart), *mMessage));
            error = Name::AppendPointerLabel(domainCompressOffset, *mMessage);
        }
    }

exit:
    return error;
}

void Server::Response::IncResourceRecordCount(void)
{
    if (mAdditional)
    {
        mHeader.SetAdditionalRecordCount(mHeader.GetAdditionalRecordCount() + 1);
    }
    else
    {
        mHeader.SetAnswerCount(mHeader.GetAnswerCount() + 1);
    }
}

Error Server::FindNameComponents(const char *aName, const char *aDomain, NameComponentsOffsetInfo &aInfo)
{
    uint8_t nameLen   = static_cast<uint8_t>(StringLength(aName, Name::kMaxNameLength));
    uint8_t domainLen = static_cast<uint8_t>(StringLength(aDomain, Name::kMaxNameLength));
    Error   error     = kErrorNone;
    uint8_t labelBegin, labelEnd;

    VerifyOrExit(Name::IsSubDomainOf(aName, aDomain), error = kErrorInvalidArgs);

    labelBegin          = nameLen - domainLen;
    aInfo.mDomainOffset = labelBegin;

    while (true)
    {
        error = FindPreviousLabel(aName, labelBegin, labelEnd);

        VerifyOrExit(error == kErrorNone, error = (error == kErrorNotFound ? kErrorNone : error));

        if (labelEnd == labelBegin + kProtocolLabelLength &&
            (StringStartsWith(&aName[labelBegin], kDnssdProtocolUdp, kStringCaseInsensitiveMatch) ||
             StringStartsWith(&aName[labelBegin], kDnssdProtocolTcp, kStringCaseInsensitiveMatch)))
        {
            // <Protocol> label found
            aInfo.mProtocolOffset = labelBegin;
            break;
        }
    }

    // Get service label <Service>
    error = FindPreviousLabel(aName, labelBegin, labelEnd);
    VerifyOrExit(error == kErrorNone, error = (error == kErrorNotFound ? kErrorNone : error));

    aInfo.mServiceOffset = labelBegin;

    // Check for service subtype
    error = FindPreviousLabel(aName, labelBegin, labelEnd);
    VerifyOrExit(error == kErrorNone, error = (error == kErrorNotFound ? kErrorNone : error));

    // Note that `kDnssdSubTypeLabel` is "._sub.". Here we get the
    // label only so we want to compare it with "_sub".
    if ((labelEnd == labelBegin + kSubTypeLabelLength) &&
        StringStartsWith(&aName[labelBegin], kDnssdSubTypeLabel + 1, kStringCaseInsensitiveMatch))
    {
        SuccessOrExit(error = FindPreviousLabel(aName, labelBegin, labelEnd));
        VerifyOrExit(labelBegin == 0, error = kErrorInvalidArgs);
        aInfo.mSubTypeOffset = labelBegin;
        ExitNow();
    }

    // Treat everything before <Service> as <Instance> label
    aInfo.mInstanceOffset = 0;

exit:
    return error;
}

Error Server::FindPreviousLabel(const char *aName, uint8_t &aStart, uint8_t &aStop)
{
    // This method finds the previous label before the current label (whose start index is @p aStart), and updates @p
    // aStart to the start index of the label and @p aStop to the index of the dot just after the label.
    // @note The input value of @p aStop does not matter because it is only used to output.

    Error   error = kErrorNone;
    uint8_t start = aStart;
    uint8_t end;

    VerifyOrExit(start > 0, error = kErrorNotFound);
    VerifyOrExit(aName[--start] == Name::kLabelSeparatorChar, error = kErrorInvalidArgs);

    end = start;
    while (start > 0 && aName[start - 1] != Name::kLabelSeparatorChar)
    {
        start--;
    }

    VerifyOrExit(start < end, error = kErrorInvalidArgs);

    aStart = start;
    aStop  = end;

exit:
    return error;
}

#if OPENTHREAD_CONFIG_SRP_SERVER_ENABLE
void Server::Response::ResolveBySrp(void)
{
    uint16_t readOffset = sizeof(Header);
    char     name[Name::kMaxNameSize];
    Question question;

    mAdditional = false;

    for (uint16_t i = 0; i < mHeader.GetQuestionCount(); i++)
    {
        // The names and questions in the request message are validated
        // from `AddQuestionsFrom()`, so we `IgnoreError()`  here.

        IgnoreError(Name::ReadName(*mMessage, readOffset, name, sizeof(name)));
        IgnoreError(mMessage->Read(readOffset, question));
        readOffset += sizeof(question);

        ResolveQuestionBySrp(name, question);

        LogInfo("ANSWER: TRANSACTION=0x%04x, QUESTION=[%s %d %d], RCODE=%d", mHeader.GetMessageId(), name,
                question.GetClass(), question.GetType(), mHeader.GetResponseCode());

        VerifyOrExit(mHeader.GetResponseCode() == Header::kResponseSuccess);
    }

    // Answer the questions with additional RRs if required
    if (mHeader.GetAnswerCount() > 0)
    {
        mAdditional = true;

        VerifyOrExit(!(Get<Server>().mTestMode & kTestModeEmptyAdditionalSection));

        readOffset = sizeof(Header);
        for (uint16_t i = 0; i < mHeader.GetQuestionCount(); i++)
        {
            IgnoreError(Name::ReadName(*mMessage, readOffset, name, sizeof(name)));
            IgnoreError(mMessage->Read(readOffset, question));
            readOffset += sizeof(question);

            if ((question.GetType() == ResourceRecord::kTypePtr) && (mHeader.GetAnswerCount() > 1))
            {
                // Skip adding additional records, when answering a
                // PTR query with more than one answer. This is the
                // recommended behavior to keep the size of the
                // response small.
                continue;
            }

            ResolveQuestionBySrp(name, question);

            LogInfo("ADDITIONAL: TRANSACTION=0x%04x, QUESTION=[%s %d %d], RCODE=%d", mHeader.GetMessageId(), name,
                    question.GetClass(), question.GetType(), mHeader.GetResponseCode());

            VerifyOrExit(mHeader.GetResponseCode() == Header::kResponseSuccess);
        }
    }

exit:
    return;
}

void Server::Response::ResolveQuestionBySrp(const char *aName, const Question &aQuestion)
{
    Error            error = kErrorNone;
    TimeMilli        now   = TimerMilli::GetNow();
    uint16_t         qtype = aQuestion.GetType();
    Header::Response rcode = Header::kResponseNameError;

    for (const Srp::Server::Host &host : Get<Srp::Server>().GetHosts())
    {
        bool        needAdditionalAaaaRecord = false;
        const char *hostName                 = host.GetFullName();

        if (host.IsDeleted())
        {
            continue;
        }

        // Handle PTR/SRV/TXT query
        if (qtype == ResourceRecord::kTypePtr || qtype == ResourceRecord::kTypeSrv || qtype == ResourceRecord::kTypeTxt)
        {
            for (const Srp::Server::Service &service : host.GetServices())
            {
                uint32_t    instanceTtl;
                const char *instanceName;
                bool        serviceNameMatched;
                bool        instanceNameMatched;
                bool        ptrQueryMatched;
                bool        srvQueryMatched;
                bool        txtQueryMatched;

                if (service.IsDeleted())
                {
                    continue;
                }

                instanceTtl         = TimeMilli::MsecToSec(service.GetExpireTime() - TimerMilli::GetNow());
                instanceName        = service.GetInstanceName();
                serviceNameMatched  = service.MatchesServiceName(aName) || service.HasSubTypeServiceName(aName);
                instanceNameMatched = service.MatchesInstanceName(aName);
                ptrQueryMatched     = qtype == ResourceRecord::kTypePtr && serviceNameMatched;
                srvQueryMatched     = qtype == ResourceRecord::kTypeSrv && instanceNameMatched;
                txtQueryMatched     = qtype == ResourceRecord::kTypeTxt && instanceNameMatched;

                if (ptrQueryMatched || srvQueryMatched)
                {
                    needAdditionalAaaaRecord = true;
                }

                if (!mAdditional && ptrQueryMatched)
                {
                    SuccessOrExit(error = AppendPtrRecord(aName, instanceName, instanceTtl));
                    rcode = Header::kResponseSuccess;
                }

                if ((!mAdditional && srvQueryMatched) ||
                    (mAdditional && ptrQueryMatched && !HasQuestion(instanceName, ResourceRecord::kTypeSrv)))
                {
                    SuccessOrExit(error = AppendSrvRecord(instanceName, hostName, instanceTtl, service.GetPriority(),
                                                          service.GetWeight(), service.GetPort()));
                    rcode = Header::kResponseSuccess;
                }

                if ((!mAdditional && txtQueryMatched) ||
                    (mAdditional && ptrQueryMatched && !HasQuestion(instanceName, ResourceRecord::kTypeTxt)))
                {
                    SuccessOrExit(error = AppendTxtRecord(instanceName, service.GetTxtData(),
                                                          service.GetTxtDataLength(), instanceTtl));
                    rcode = Header::kResponseSuccess;
                }
            }
        }

        // Handle AAAA query
        if ((!mAdditional && qtype == ResourceRecord::kTypeAaaa && host.Matches(aName)) ||
            (mAdditional && needAdditionalAaaaRecord && !HasQuestion(hostName, ResourceRecord::kTypeAaaa)))
        {
            uint8_t             addrNum;
            const Ip6::Address *addrs   = host.GetAddresses(addrNum);
            uint32_t            hostTtl = TimeMilli::MsecToSec(host.GetExpireTime() - now);

            for (uint8_t i = 0; i < addrNum; i++)
            {
                SuccessOrExit(error = AppendAaaaRecord(hostName, addrs[i], hostTtl));
            }

            rcode = Header::kResponseSuccess;
        }
    }

exit:

    // If there is an `error` (for example, appending to the message
    // fails), we always set the response code in the header to
    // `kResponseServerFailure`. Otherwise, we only set the response
    // code if the entry is for the answer section, not the
    // additional data section.

    if (error != kErrorNone)
    {
        mHeader.SetResponseCode(Header::kResponseServerFailure);
    }
    else if (!mAdditional)
    {
        mHeader.SetResponseCode(rcode);
    }
}

#endif // OPENTHREAD_CONFIG_SRP_SERVER_ENABLE

Error Server::ResolveByQueryCallbacks(Response &aResponse, const Ip6::MessageInfo &aMessageInfo)
{
    Error             error = kErrorNone;
    QueryTransaction *query = nullptr;
    DnsQueryType      queryType;
    char              name[Name::kMaxNameSize];

    VerifyOrExit(mQuerySubscribe.IsSet(), error = kErrorFailed);

    aResponse.GetQueryTypeAndName(queryType, name);
    VerifyOrExit(queryType != kDnsQueryNone, error = kErrorNotImplemented);

    query = NewQuery(aResponse, aMessageInfo);
    VerifyOrExit(query != nullptr, error = kErrorNoBufs);

    mQuerySubscribe.Invoke(name);

exit:
    return error;
}

#if OPENTHREAD_CONFIG_DNS_UPSTREAM_QUERY_ENABLE
bool Server::ShouldForwardToUpstream(const Request &aRequest)
{
    bool     shouldForward = false;
    uint16_t readOffset;
    char     name[Name::kMaxNameSize];

    VerifyOrExit(aRequest.mHeader.IsRecursionDesiredFlagSet());
    readOffset = sizeof(Header);

    for (uint16_t i = 0; i < aRequest.mHeader.GetQuestionCount(); i++)
    {
        SuccessOrExit(Name::ReadName(*aRequest.mMessage, readOffset, name, sizeof(name)));
        readOffset += sizeof(Question);

        VerifyOrExit(!Name::IsSubDomainOf(name, kDefaultDomainName));

        for (const char *blockedDomain : kBlockedDomains)
        {
            VerifyOrExit(!Name::IsSameDomain(name, blockedDomain));
        }
    }

    shouldForward = true;

exit:
    return shouldForward;
}

void Server::OnUpstreamQueryDone(UpstreamQueryTransaction &aQueryTransaction, Message *aResponseMessage)
{
    Error error = kErrorNone;

    VerifyOrExit(aQueryTransaction.IsValid(), error = kErrorInvalidArgs);

    if (aResponseMessage != nullptr)
    {
        error = mSocket.SendTo(*aResponseMessage, aQueryTransaction.GetMessageInfo());
    }

    ResetUpstreamQueryTransaction(aQueryTransaction, error);

exit:
    FreeMessageOnError(aResponseMessage, error);
}

Server::UpstreamQueryTransaction *Server::AllocateUpstreamQueryTransaction(const Ip6::MessageInfo &aMessageInfo)
{
    UpstreamQueryTransaction *newTxn = nullptr;

    for (UpstreamQueryTransaction &txn : mUpstreamQueryTransactions)
    {
        if (!txn.IsValid())
        {
            newTxn = &txn;
            break;
        }
    }

    VerifyOrExit(newTxn != nullptr);

    newTxn->Init(aMessageInfo);
    LogInfo("Upstream query transaction %d initialized.", static_cast<int>(newTxn - mUpstreamQueryTransactions));
    mTimer.FireAtIfEarlier(newTxn->GetExpireTime());

exit:
    return newTxn;
}

Error Server::ResolveByUpstream(const Request &aRequest)
{
    Error                     error = kErrorNone;
    UpstreamQueryTransaction *txn;

    txn = AllocateUpstreamQueryTransaction(*aRequest.mMessageInfo);
    VerifyOrExit(txn != nullptr, error = kErrorNoBufs);

    otPlatDnsStartUpstreamQuery(&GetInstance(), txn, aRequest.mMessage);

exit:
    return error;
}
#endif // OPENTHREAD_CONFIG_DNS_UPSTREAM_QUERY_ENABLE

Server::QueryTransaction *Server::NewQuery(Response &aResponse, const Ip6::MessageInfo &aMessageInfo)
{
    QueryTransaction *newQuery = nullptr;

    for (QueryTransaction &query : mQueryTransactions)
    {
        if (!query.IsValid())
        {
            newQuery = &query;
            break;
        }
    }

    VerifyOrExit(newQuery != nullptr);

    *static_cast<Response *>(newQuery) = aResponse;
    newQuery->mMessageInfo             = aMessageInfo;
    newQuery->mExpireTime              = TimerMilli::GetNow() + kQueryTimeout;

    mTimer.FireAtIfEarlier(newQuery->mExpireTime);

exit:
    return newQuery;
}

bool Server::QueryTransaction::CanAnswer(const char *aServiceFullName, const ServiceInstanceInfo &aInstanceInfo) const
{
    char         name[Name::kMaxNameSize];
    DnsQueryType sdType;
    bool         canAnswer = false;

    GetQueryTypeAndName(sdType, name);

    switch (sdType)
    {
    case kDnsQueryBrowse:
        canAnswer = StringMatch(name, aServiceFullName, kStringCaseInsensitiveMatch);
        break;
    case kDnsQueryResolve:
        canAnswer = StringMatch(name, aInstanceInfo.mFullName, kStringCaseInsensitiveMatch);
        break;
    default:
        break;
    }

    return canAnswer;
}

bool Server::QueryTransaction::CanAnswer(const char *aHostFullName) const
{
    char         name[Name::kMaxNameSize];
    DnsQueryType sdType;

    GetQueryTypeAndName(sdType, name);

    return (sdType == kDnsQueryResolveHost) && StringMatch(name, aHostFullName, kStringCaseInsensitiveMatch);
}

void Server::QueryTransaction::Answer(const char *aServiceFullName, const ServiceInstanceInfo &aInstanceInfo)
{
    Error error = kErrorNone;

    mAdditional = false;

    if (HasQuestion(aServiceFullName, ResourceRecord::kTypePtr))
    {
        SuccessOrExit(error = AppendPtrRecord(aServiceFullName, aInstanceInfo.mFullName, aInstanceInfo.mTtl));
    }

    for (uint8_t additional = 0; additional <= 1; additional++)
    {
        if (additional == 1)
        {
            mAdditional = true;
            VerifyOrExit(!(Get<Server>().mTestMode & kTestModeEmptyAdditionalSection));
        }

        if (HasQuestion(aInstanceInfo.mFullName, ResourceRecord::kTypeSrv) == !additional)
        {
            SuccessOrExit(error = AppendSrvRecord(aInstanceInfo.mFullName, aInstanceInfo.mHostName, aInstanceInfo.mTtl,
                                                  aInstanceInfo.mPriority, aInstanceInfo.mWeight, aInstanceInfo.mPort));
        }

        if (HasQuestion(aInstanceInfo.mFullName, ResourceRecord::kTypeTxt) == !additional)
        {
            SuccessOrExit(error = AppendTxtRecord(aInstanceInfo.mFullName, aInstanceInfo.mTxtData,
                                                  aInstanceInfo.mTxtLength, aInstanceInfo.mTtl));
        }

        if (HasQuestion(aInstanceInfo.mHostName, ResourceRecord::kTypeAaaa) == !additional)
        {
            for (uint8_t i = 0; i < aInstanceInfo.mAddressNum; i++)
            {
                const Ip6::Address &address = AsCoreType(&aInstanceInfo.mAddresses[i]);

                OT_ASSERT(!address.IsUnspecified() && !address.IsLinkLocal() && !address.IsMulticast() &&
                          !address.IsLoopback());

                SuccessOrExit(error = AppendAaaaRecord(aInstanceInfo.mHostName, address, aInstanceInfo.mTtl));
            }
        }
    }

exit:
    Finalize(error == kErrorNone ? Header::kResponseSuccess : Header::kResponseServerFailure);
}

void Server::QueryTransaction::Answer(const char *aHostFullName, const HostInfo &aHostInfo)
{
    Error error = kErrorNone;

    mAdditional = false;

    if (HasQuestion(aHostFullName, ResourceRecord::kTypeAaaa))
    {
        for (uint8_t i = 0; i < aHostInfo.mAddressNum; i++)
        {
            const Ip6::Address &address = AsCoreType(&aHostInfo.mAddresses[i]);

            OT_ASSERT(!address.IsUnspecified() && !address.IsMulticast() && !address.IsLinkLocal() &&
                      !address.IsLoopback());

            SuccessOrExit(error = AppendAaaaRecord(aHostFullName, address, aHostInfo.mTtl));
        }
    }

exit:
    Finalize(error == kErrorNone ? Header::kResponseSuccess : Header::kResponseServerFailure);
}

void Server::SetQueryCallbacks(SubscribeCallback aSubscribe, UnsubscribeCallback aUnsubscribe, void *aContext)
{
    OT_ASSERT((aSubscribe == nullptr) == (aUnsubscribe == nullptr));

    mQuerySubscribe.Set(aSubscribe, aContext);
    mQueryUnsubscribe.Set(aUnsubscribe, aContext);
}

void Server::HandleDiscoveredServiceInstance(const char *aServiceFullName, const ServiceInstanceInfo &aInstanceInfo)
{
    OT_ASSERT(StringEndsWith(aServiceFullName, Name::kLabelSeparatorChar));
    OT_ASSERT(StringEndsWith(aInstanceInfo.mFullName, Name::kLabelSeparatorChar));
    OT_ASSERT(StringEndsWith(aInstanceInfo.mHostName, Name::kLabelSeparatorChar));

    for (QueryTransaction &query : mQueryTransactions)
    {
        if (query.IsValid() && query.CanAnswer(aServiceFullName, aInstanceInfo))
        {
            query.Answer(aServiceFullName, aInstanceInfo);
        }
    }
}

void Server::HandleDiscoveredHost(const char *aHostFullName, const HostInfo &aHostInfo)
{
    OT_ASSERT(StringEndsWith(aHostFullName, Name::kLabelSeparatorChar));

    for (QueryTransaction &query : mQueryTransactions)
    {
        if (query.IsValid() && query.CanAnswer(aHostFullName))
        {
            query.Answer(aHostFullName, aHostInfo);
        }
    }
}

const otDnssdQuery *Server::GetNextQuery(const otDnssdQuery *aQuery) const
{
    const QueryTransaction *cur   = &mQueryTransactions[0];
    const QueryTransaction *found = nullptr;
    const QueryTransaction *query = static_cast<const QueryTransaction *>(aQuery);

    if (aQuery != nullptr)
    {
        cur = query + 1;
    }

    for (; cur < GetArrayEnd(mQueryTransactions); cur++)
    {
        if (cur->IsValid())
        {
            found = cur;
            break;
        }
    }

    return static_cast<const otDnssdQuery *>(found);
}

Server::DnsQueryType Server::GetQueryTypeAndName(const otDnssdQuery *aQuery, char (&aName)[Name::kMaxNameSize])
{
    const QueryTransaction *query = static_cast<const QueryTransaction *>(aQuery);
    DnsQueryType            type;

    OT_ASSERT(query->IsValid());

    query->GetQueryTypeAndName(type, aName);

    return type;
}

void Server::Response::GetQueryTypeAndName(DnsQueryType &aType, char (&aName)[Name::kMaxNameSize]) const
{
    aType = kDnsQueryNone;

    for (uint16_t i = 0, readOffset = sizeof(Header); i < mHeader.GetQuestionCount(); i++)
    {
        Question question;

        IgnoreError(Name::ReadName(*mMessage, readOffset, aName, sizeof(aName)));
        IgnoreError(mMessage->Read(readOffset, question));
        readOffset += sizeof(question);

        switch (question.GetType())
        {
        case ResourceRecord::kTypePtr:
            ExitNow(aType = kDnsQueryBrowse);
        case ResourceRecord::kTypeSrv:
        case ResourceRecord::kTypeTxt:
            ExitNow(aType = kDnsQueryResolve);
        }
    }

    for (uint16_t i = 0, readOffset = sizeof(Header); i < mHeader.GetQuestionCount(); i++)
    {
        Question question;

        IgnoreError(Name::ReadName(*mMessage, readOffset, aName, sizeof(aName)));
        IgnoreError(mMessage->Read(readOffset, question));
        readOffset += sizeof(question);

        switch (question.GetType())
        {
        case ResourceRecord::kTypeAaaa:
        case ResourceRecord::kTypeA:
            ExitNow(aType = kDnsQueryResolveHost);
        }
    }

exit:
    return;
}

bool Server::Response::HasQuestion(const char *aName, uint16_t aQuestionType) const
{
    bool found = false;

    for (uint16_t i = 0, readOffset = sizeof(Header); i < mHeader.GetQuestionCount(); i++)
    {
        Question question;
        Error    error;

        error = Name::CompareName(*mMessage, readOffset, aName);
        IgnoreError(mMessage->Read(readOffset, question));
        readOffset += sizeof(question);

        if ((error == kErrorNone) && (aQuestionType == question.GetType()))
        {
            ExitNow(found = true);
        }
    }

exit:
    return found;
}

void Server::HandleTimer(void)
{
    TimeMilli now        = TimerMilli::GetNow();
    TimeMilli nextExpire = now.GetDistantFuture();

    for (QueryTransaction &query : mQueryTransactions)
    {
        if (!query.IsValid())
        {
            continue;
        }

        if (query.mExpireTime <= now)
        {
            query.Finalize(Header::kResponseSuccess);
        }
        else
        {
            nextExpire = Min(nextExpire, query.mExpireTime);
        }
    }

#if OPENTHREAD_CONFIG_DNS_UPSTREAM_QUERY_ENABLE
    for (UpstreamQueryTransaction &query : mUpstreamQueryTransactions)
    {
        if (!query.IsValid())
        {
            continue;
        }

        if (query.GetExpireTime() <= now)
        {
            otPlatDnsCancelUpstreamQuery(&GetInstance(), &query);
        }
        else
        {
            nextExpire = Min(nextExpire, query.GetExpireTime());
        }
    }
#endif

    if (nextExpire != now.GetDistantFuture())
    {
        mTimer.FireAtIfEarlier(nextExpire);
    }
}

void Server::QueryTransaction::Finalize(Header::Response aResponseCode)
{
    char         name[Name::kMaxNameSize];
    DnsQueryType sdType;

    GetQueryTypeAndName(sdType, name);

    OT_ASSERT(sdType != kDnsQueryNone);
    OT_UNUSED_VARIABLE(sdType);

    Get<Server>().mQueryUnsubscribe.InvokeIfSet(name);

    mHeader.SetResponseCode(aResponseCode);
    Send(mMessageInfo);

    // Set the `mMessage` to null to indicate that
    // `QueryTransaction` is unused.
    mMessage = nullptr;
}

void Server::UpdateResponseCounters(Header::Response aResponseCode)
{
    switch (aResponseCode)
    {
    case UpdateHeader::kResponseSuccess:
        ++mCounters.mSuccessResponse;
        break;
    case UpdateHeader::kResponseServerFailure:
        ++mCounters.mServerFailureResponse;
        break;
    case UpdateHeader::kResponseFormatError:
        ++mCounters.mFormatErrorResponse;
        break;
    case UpdateHeader::kResponseNameError:
        ++mCounters.mNameErrorResponse;
        break;
    case UpdateHeader::kResponseNotImplemented:
        ++mCounters.mNotImplementedResponse;
        break;
    default:
        ++mCounters.mOtherResponse;
        break;
    }
}

#if OPENTHREAD_CONFIG_DNS_UPSTREAM_QUERY_ENABLE
void Server::UpstreamQueryTransaction::Init(const Ip6::MessageInfo &aMessageInfo)
{
    mMessageInfo = aMessageInfo;
    mValid       = true;
    mExpireTime  = TimerMilli::GetNow() + kQueryTimeout;
}

void Server::ResetUpstreamQueryTransaction(UpstreamQueryTransaction &aTxn, Error aError)
{
    int index = static_cast<int>(&aTxn - mUpstreamQueryTransactions);

    // Avoid the warnings when info / warn logging is disabled.
    OT_UNUSED_VARIABLE(index);
    if (aError == kErrorNone)
    {
        LogInfo("Upstream query transaction %d completed.", index);
    }
    else
    {
        LogWarn("Upstream query transaction %d closed: %s.", index, ErrorToString(aError));
    }
    aTxn.Reset();
}
#endif

} // namespace ServiceDiscovery
} // namespace Dns
} // namespace ot

#endif // OPENTHREAD_CONFIG_DNS_SERVER_ENABLE
