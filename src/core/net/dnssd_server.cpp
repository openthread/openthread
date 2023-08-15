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

const char Server::kDefaultDomainName[] = "default.service.arpa.";
const char Server::kSubLabel[]          = "_sub";

#if OPENTHREAD_CONFIG_DNS_UPSTREAM_QUERY_ENABLE
const char *Server::kBlockedDomains[] = {"ipv4only.arpa."};
#endif

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

    LogInfo("Started");

exit:
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
            query.Finalize(kErrorFailed);
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
    LogInfo("Stopped");

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

    LogInfo("Received query from %s", aMessageInfo.GetPeerAddr().ToString().AsCString());

    ProcessQuery(request);

exit:
    return;
}

void Server::ProcessQuery(Request &aRequest)
{
    Error        error = kErrorNone;
    Response     response;
    bool         shouldSendResponse = true;
    ResponseCode rcode              = Header::kResponseSuccess;

#if OPENTHREAD_CONFIG_DNS_UPSTREAM_QUERY_ENABLE
    if (mEnableUpstreamQuery && ShouldForwardToUpstream(aRequest))
    {
        error = ResolveByUpstream(aRequest);

        if (error == kErrorNone)
        {
            shouldSendResponse = false;
            ExitNow();
        }

        LogWarn("Error forwarding to upstream: %s", ErrorToString(error));

        error = kErrorNone;
        rcode = Header::kResponseServerFailure;

        // Continue to allocate and prepare the response message
        // to send the `kResponseServerFailure` response code.
    }
#endif

    response.mMessage = Get<Server>().mSocket.NewMessage();
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
    SuccessOrExit(rcode);
#endif

    SuccessOrExit(rcode = aRequest.ParseQuestions(mTestMode));
    SuccessOrExit(rcode = response.AddQuestionsFrom(aRequest));

#if OT_SHOULD_LOG_AT(OT_LOG_LEVEL_INFO)
    response.Log();
#endif

#if OPENTHREAD_CONFIG_SRP_SERVER_ENABLE
    switch (response.ResolveBySrp())
    {
    case kErrorNone:
        mCounters.mResolvedBySrp++;
        ExitNow();

    case kErrorNotFound:
        rcode = Header::kResponseNameError;
        break;

    default:
        rcode = Header::kResponseServerFailure;
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
            response.SetResponseCode(rcode);
        }

        response.Send(*aRequest.mMessageInfo);
    }

    FreeMessageOnError(response.mMessage, error);
}

void Server::Response::Send(const Ip6::MessageInfo &aMessageInfo)
{
    Error        error;
    ResponseCode rcode = mHeader.GetResponseCode();

    if (rcode == Header::kResponseServerFailure)
    {
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
        LogWarn("Failed to send reply: %s", ErrorToString(error));
    }
    else
    {
        LogInfo("Send response, rcode:%u", rcode);
    }

    Get<Server>().UpdateResponseCounters(rcode);
}

Server::ResponseCode Server::Request::ParseQuestions(uint8_t aTestMode)
{
    // Parse header and questions from a `Request` query message and
    // determine the `QueryType`.

    ResponseCode rcode         = Header::kResponseFormatError;
    uint16_t     offset        = sizeof(Header);
    uint16_t     questionCount = mHeader.GetQuestionCount();
    Question     question;

    VerifyOrExit(mHeader.GetQueryType() == Header::kQueryTypeStandard, rcode = Header::kResponseNotImplemented);
    VerifyOrExit(!mHeader.IsTruncationFlagSet());

    VerifyOrExit(questionCount > 0);

    SuccessOrExit(Name::ParseName(*mMessage, offset));
    SuccessOrExit(mMessage->Read(offset, question));
    offset += sizeof(question);

    switch (question.GetType())
    {
    case ResourceRecord::kTypePtr:
        mType = kPtrQuery;
        break;
    case ResourceRecord::kTypeSrv:
        mType = kSrvQuery;
        break;
    case ResourceRecord::kTypeTxt:
        mType = kTxtQuery;
        break;
    case ResourceRecord::kTypeAaaa:
        mType = kAaaaQuery;
        break;
    default:
        ExitNow(rcode = Header::kResponseNotImplemented);
    }

    if (questionCount > 1)
    {
        VerifyOrExit(!(aTestMode & kTestModeSingleQuestionOnly));

        VerifyOrExit(questionCount == 2);

        SuccessOrExit(Name::CompareName(*mMessage, offset, *mMessage, sizeof(Header)));
        SuccessOrExit(mMessage->Read(offset, question));

        switch (question.GetType())
        {
        case ResourceRecord::kTypeSrv:
            VerifyOrExit(mType == kTxtQuery);
            break;

        case ResourceRecord::kTypeTxt:
            VerifyOrExit(mType == kSrvQuery);
            break;

        default:
            ExitNow();
        }

        mType = kSrvTxtQuery;
    }

    rcode = Header::kResponseSuccess;

exit:
    return rcode;
}

Server::ResponseCode Server::Response::AddQuestionsFrom(const Request &aRequest)
{
    ResponseCode rcode = Header::kResponseServerFailure;
    uint16_t     offset;

    mType = aRequest.mType;

    // Read the name from `aRequest.mMessage` and append it as is to
    // the response message. This ensures all name formats, including
    // service instance names with dot characters in the instance
    // label, are appended correctly.

    SuccessOrExit(Name(*aRequest.mMessage, sizeof(Header)).AppendTo(*mMessage));

    // Check the name to include the correct domain name and determine
    // the domain name offset (for DNS name compression).

    VerifyOrExit(ParseQueryName() == kErrorNone, rcode = Header::kResponseNameError);

    mHeader.SetQuestionCount(aRequest.mHeader.GetQuestionCount());

    offset = sizeof(Header);

    for (uint16_t questionCount = 0; questionCount < mHeader.GetQuestionCount(); questionCount++)
    {
        Question question;

        // The names and questions in `aRequest` are validated already
        // from `ParseQuestions()`, so we can `IgnoreError()`  here.

        IgnoreError(Name::ParseName(*aRequest.mMessage, offset));
        IgnoreError(aRequest.mMessage->Read(offset, question));
        offset += sizeof(question);

        if (questionCount != 0)
        {
            SuccessOrExit(AppendQueryName());
        }

        SuccessOrExit(mMessage->Append(question));
    }

    rcode = Header::kResponseSuccess;

exit:
    return rcode;
}

Error Server::Response::ParseQueryName(void)
{
    // Parses and validates the query name and updates
    // the name compression offsets.

    Error    error = kErrorNone;
    DnsName  name;
    uint16_t offset;

    offset = sizeof(Header);
    SuccessOrExit(error = Name::ReadName(*mMessage, offset, name, sizeof(name)));

    switch (mType)
    {
    case kPtrQuery:
        // `mServiceOffset` may be updated as we read labels and if we
        // determine that the query name is a sub-type service.
        mServiceOffset = sizeof(Header);
        break;

    case kSrvQuery:
    case kTxtQuery:
    case kSrvTxtQuery:
        mInstanceOffset = sizeof(Header);
        break;

    case kAaaaQuery:
        mHostOffset = sizeof(Header);
        break;
    }

    // Read the query name labels one by one to check if the name is
    // service sub-type and also check that it is sub-domain of the
    // default domain name and determine its offset

    offset = sizeof(Header);

    while (true)
    {
        DnsLabel label;
        uint8_t  labelLength = sizeof(label);
        uint16_t comapreOffset;

        SuccessOrExit(error = Name::ReadLabel(*mMessage, offset, label, labelLength));

        if ((mType == kPtrQuery) && StringMatch(label, kSubLabel, kStringCaseInsensitiveMatch))
        {
            mServiceOffset = offset;
        }

        comapreOffset = offset;

        if (Name::CompareName(*mMessage, comapreOffset, kDefaultDomainName) == kErrorNone)
        {
            mDomainOffset = offset;
            ExitNow();
        }
    }

    error = kErrorParse;

exit:
    return error;
}

void Server::Response::ReadQueryName(DnsName &aName) const
{
    // Query name is always present immediately after `Header` in the
    // question section

    uint16_t offset = sizeof(Header);

    IgnoreError(Name::ReadName(*mMessage, offset, aName, sizeof(aName)));
}

bool Server::Response::QueryNameMatches(const char *aName) const
{
    uint16_t offset = sizeof(Header);

    return (Name::CompareName(*mMessage, offset, aName) == kErrorNone);
}

Error Server::Response::AppendQueryName(void) const { return Name::AppendPointerLabel(sizeof(Header), *mMessage); }

Error Server::Response::AppendPtrRecord(const char *aInstanceLabel, uint32_t aTtl)
{
    Error     error;
    uint16_t  recordOffset;
    PtrRecord ptrRecord;

    ptrRecord.Init();
    ptrRecord.SetTtl(aTtl);

    SuccessOrExit(error = AppendQueryName());

    recordOffset = mMessage->GetLength();
    SuccessOrExit(error = mMessage->Append(ptrRecord));

    mInstanceOffset = mMessage->GetLength();
    SuccessOrExit(error = Name::AppendLabel(aInstanceLabel, *mMessage));
    SuccessOrExit(error = Name::AppendPointerLabel(mServiceOffset, *mMessage));

    UpdateRecordLength(ptrRecord, recordOffset);

    IncResourceRecordCount();

exit:
    return error;
}

#if OPENTHREAD_CONFIG_SRP_SERVER_ENABLE
Error Server::Response::AppendSrvRecord(const Srp::Server::Service &aService)
{
    uint32_t ttl = TimeMilli::MsecToSec(aService.GetExpireTime() - TimerMilli::GetNow());

    return AppendSrvRecord(aService.GetHost().GetFullName(), ttl, aService.GetPriority(), aService.GetWeight(),
                           aService.GetPort());
}
#endif

Error Server::Response::AppendSrvRecord(const ServiceInstanceInfo &aInstanceInfo)
{
    return AppendSrvRecord(aInstanceInfo.mHostName, aInstanceInfo.mTtl, aInstanceInfo.mPriority, aInstanceInfo.mWeight,
                           aInstanceInfo.mPort);
}

Error Server::Response::AppendSrvRecord(const char *aHostName,
                                        uint32_t    aTtl,
                                        uint16_t    aPriority,
                                        uint16_t    aWeight,
                                        uint16_t    aPort)
{
    Error     error = kErrorNone;
    SrvRecord srvRecord;
    uint16_t  recordOffset;
    DnsName   hostLabels;

    SuccessOrExit(error = Name::ExtractLabels(aHostName, kDefaultDomainName, hostLabels, sizeof(hostLabels)));

    srvRecord.Init();
    srvRecord.SetTtl(aTtl);
    srvRecord.SetPriority(aPriority);
    srvRecord.SetWeight(aWeight);
    srvRecord.SetPort(aPort);

    SuccessOrExit(error = Name::AppendPointerLabel(mInstanceOffset, *mMessage));

    recordOffset = mMessage->GetLength();
    SuccessOrExit(error = mMessage->Append(srvRecord));

    mHostOffset = mMessage->GetLength();
    SuccessOrExit(error = Name::AppendMultipleLabels(hostLabels, *mMessage));
    SuccessOrExit(error = Name::AppendPointerLabel(mDomainOffset, *mMessage));

    UpdateRecordLength(srvRecord, recordOffset);

    IncResourceRecordCount();

exit:
    return error;
}

#if OPENTHREAD_CONFIG_SRP_SERVER_ENABLE
Error Server::Response::AppendHostAddresses(const Srp::Server::Host &aHost)
{
    const Ip6::Address *addrs;
    uint8_t             addrsLength;
    uint32_t            ttl;

    addrs = aHost.GetAddresses(addrsLength);
    ttl   = TimeMilli::MsecToSec(aHost.GetExpireTime() - TimerMilli::GetNow());

    return AppendHostAddresses(addrs, addrsLength, ttl);
}
#endif

Error Server::Response::AppendHostAddresses(const HostInfo &aHostInfo)
{
    return AppendHostAddresses(AsCoreTypePtr(aHostInfo.mAddresses), aHostInfo.mAddressNum, aHostInfo.mTtl);
}

Error Server::Response::AppendHostAddresses(const ServiceInstanceInfo &aInstanceInfo)
{
    return AppendHostAddresses(AsCoreTypePtr(aInstanceInfo.mAddresses), aInstanceInfo.mAddressNum, aInstanceInfo.mTtl);
}

Error Server::Response::AppendHostAddresses(const Ip6::Address *aAddrs, uint16_t aAddrsLength, uint32_t aTtl)
{
    Error error = kErrorNone;

    for (uint16_t index = 0; index < aAddrsLength; index++)
    {
        AaaaRecord aaaaRecord;

        aaaaRecord.Init();
        aaaaRecord.SetTtl(aTtl);
        aaaaRecord.SetAddress(aAddrs[index]);

        SuccessOrExit(error = Name::AppendPointerLabel(mHostOffset, *mMessage));
        SuccessOrExit(error = mMessage->Append(aaaaRecord));

        IncResourceRecordCount();
    }

exit:
    return error;
}

#if OPENTHREAD_CONFIG_SRP_SERVER_ENABLE
Error Server::Response::AppendTxtRecord(const Srp::Server::Service &aService)
{
    return AppendTxtRecord(aService.GetTxtData(), aService.GetTxtDataLength(),
                           TimeMilli::MsecToSec(aService.GetExpireTime() - TimerMilli::GetNow()));
}
#endif

Error Server::Response::AppendTxtRecord(const ServiceInstanceInfo &aInstanceInfo)
{
    return AppendTxtRecord(aInstanceInfo.mTxtData, aInstanceInfo.mTxtLength, aInstanceInfo.mTtl);
}

Error Server::Response::AppendTxtRecord(const void *aTxtData, uint16_t aTxtLength, uint32_t aTtl)
{
    Error     error = kErrorNone;
    TxtRecord txtRecord;
    uint8_t   emptyTxt = 0;

    if (aTxtLength == 0)
    {
        aTxtData   = &emptyTxt;
        aTxtLength = sizeof(emptyTxt);
    }

    txtRecord.Init();
    txtRecord.SetTtl(aTtl);
    txtRecord.SetLength(aTxtLength);

    SuccessOrExit(error = Name::AppendPointerLabel(mInstanceOffset, *mMessage));
    SuccessOrExit(error = mMessage->Append(txtRecord));
    SuccessOrExit(error = mMessage->AppendBytes(aTxtData, aTxtLength));

    IncResourceRecordCount();

exit:
    return error;
}

void Server::Response::UpdateRecordLength(ResourceRecord &aRecord, uint16_t aOffset) const
{
    // Calculates RR DATA length and updates and re-writes it in the
    // response message. This should be called immediately
    // after all the fields in the record are written in the message.
    // `aOffset` gives the offset in the message to the start of the
    // record.

    aRecord.SetLength(mMessage->GetLength() - aOffset - sizeof(Dns::ResourceRecord));
    mMessage->Write(aOffset, aRecord);
}

void Server::Response::IncResourceRecordCount(void)
{
    switch (mSection)
    {
    case kAnswerSection:
        mHeader.SetAnswerCount(mHeader.GetAnswerCount() + 1);
        break;
    case kAdditionalDataSection:
        mHeader.SetAdditionalRecordCount(mHeader.GetAdditionalRecordCount() + 1);
        break;
    }
}

uint8_t Server::GetNameLength(const char *aName)
{
    return static_cast<uint8_t>(StringLength(aName, Name::kMaxNameLength));
}

#if OT_SHOULD_LOG_AT(OT_LOG_LEVEL_INFO)
void Server::Response::Log(void) const
{
    DnsName name;

    ReadQueryName(name);
    LogInfo("%s query for '%s'", QueryTypeToString(mType), name);
}

const char *Server::Response::QueryTypeToString(QueryType aType)
{
    static const char *const kTypeNames[] = {
        "PTR",       // (0) kPtrQuery
        "SRV",       // (1) kSrvQuery
        "TXT",       // (2) kTxtQuery
        "SRV & TXT", // (3) kSrvTxtQuery
        "AAAA",      // (4) kAaaaQuery
    };

    static_assert(0 == kPtrQuery, "kPtrQuery value is incorrect");
    static_assert(1 == kSrvQuery, "kSrvQuery value is incorrect");
    static_assert(2 == kTxtQuery, "kTxtQuery value is incorrect");
    static_assert(3 == kSrvTxtQuery, "kSrvTxtQuery value is incorrect");
    static_assert(4 == kAaaaQuery, "kAaaaQuery value is incorrect");

    return kTypeNames[aType];
}
#endif

#if OPENTHREAD_CONFIG_SRP_SERVER_ENABLE

Error Server::Response::ResolveBySrp(void)
{
    static const Section kSections[] = {kAnswerSection, kAdditionalDataSection};

    Error                       error          = kErrorNotFound;
    const Srp::Server::Service *matchedService = nullptr;
    bool                        found          = false;
    Section                     srvSection;
    Section                     txtSection;

    mSection = kAnswerSection;

    for (const Srp::Server::Host &host : Get<Srp::Server>().GetHosts())
    {
        if (host.IsDeleted())
        {
            continue;
        }

        if (mType == kAaaaQuery)
        {
            if (QueryNameMatches(host.GetFullName()))
            {
                error = AppendHostAddresses(host);
                ExitNow();
            }

            continue;
        }

        // `mType` is PTR or SRV/TXT query

        for (const Srp::Server::Service &service : host.GetServices())
        {
            if (service.IsDeleted())
            {
                continue;
            }

            if (mType == kPtrQuery)
            {
                if (QueryNameMatchesService(service))
                {
                    uint32_t ttl = TimeMilli::MsecToSec(service.GetExpireTime() - TimerMilli::GetNow());

                    SuccessOrExit(error = AppendPtrRecord(service.GetInstanceLabel(), ttl));
                    matchedService = &service;
                }
            }
            else if (QueryNameMatches(service.GetInstanceName()))
            {
                matchedService = &service;
                found          = true;
                break;
            }
        }

        if (found)
        {
            break;
        }
    }

    VerifyOrExit(matchedService != nullptr);

    if (mType == kPtrQuery)
    {
        // Skip adding additional records, when answering a
        // PTR query with more than one answer. This is the
        // recommended behavior to keep the size of the
        // response small.

        VerifyOrExit(mHeader.GetAnswerCount() == 1);
    }

    srvSection = ((mType == kSrvQuery) || (mType == kSrvTxtQuery)) ? kAnswerSection : kAdditionalDataSection;
    txtSection = ((mType == kTxtQuery) || (mType == kSrvTxtQuery)) ? kAnswerSection : kAdditionalDataSection;

    for (Section section : kSections)
    {
        mSection = section;

        if (mSection == kAdditionalDataSection)
        {
            VerifyOrExit(!(Get<Server>().mTestMode & kTestModeEmptyAdditionalSection));
        }

        if (srvSection == mSection)
        {
            SuccessOrExit(error = AppendSrvRecord(*matchedService));
        }

        if (txtSection == mSection)
        {
            SuccessOrExit(error = AppendTxtRecord(*matchedService));
        }
    }

    SuccessOrExit(error = AppendHostAddresses(matchedService->GetHost()));

exit:
    return error;
}

bool Server::Response::QueryNameMatchesService(const Srp::Server::Service &aService) const
{
    // Check if the query name matches the base service name or any
    // sub-type service names associated with `aService`.

    bool matches = QueryNameMatches(aService.GetServiceName());

    VerifyOrExit(!matches);

    for (uint16_t index = 0; index < aService.GetNumberOfSubTypes(); index++)
    {
        matches = QueryNameMatches(aService.GetSubTypeServiceNameAt(index));
        VerifyOrExit(!matches);
    }

exit:
    return matches;
}

#endif // OPENTHREAD_CONFIG_SRP_SERVER_ENABLE

Error Server::ResolveByQueryCallbacks(Response &aResponse, const Ip6::MessageInfo &aMessageInfo)
{
    Error             error = kErrorNone;
    QueryTransaction *query = nullptr;
    DnsName           name;

    VerifyOrExit(mQuerySubscribe.IsSet(), error = kErrorFailed);

    query = NewQuery(aResponse, aMessageInfo);
    VerifyOrExit(query != nullptr, error = kErrorNoBufs);

    query->ReadQueryName(name);
    mQuerySubscribe.Invoke(name);

exit:
    return error;
}

#if OPENTHREAD_CONFIG_DNS_UPSTREAM_QUERY_ENABLE
bool Server::ShouldForwardToUpstream(const Request &aRequest)
{
    bool     shouldForward = false;
    uint16_t readOffset;
    DnsName  name;

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
    bool canAnswer = false;

    switch (mType)
    {
    case kPtrQuery:
        canAnswer = QueryNameMatches(aServiceFullName);
        break;

    case kSrvQuery:
    case kTxtQuery:
    case kSrvTxtQuery:
        canAnswer = QueryNameMatches(aInstanceInfo.mFullName);
        break;

    case kAaaaQuery:
        break;
    }

    return canAnswer;
}

bool Server::QueryTransaction::CanAnswer(const char *aHostFullName) const
{
    return (mType == kAaaaQuery) && QueryNameMatches(aHostFullName);
}

Error Server::QueryTransaction::ExtractServiceInstanceLabel(const char *aInstanceName, DnsLabel &aLabel)
{
    uint16_t offset;
    DnsName  serviceName;

    offset = mServiceOffset;
    IgnoreError(Name::ReadName(*mMessage, offset, serviceName, sizeof(serviceName)));

    return Name::ExtractLabels(aInstanceName, serviceName, aLabel, sizeof(aLabel));
}

void Server::QueryTransaction::Answer(const ServiceInstanceInfo &aInstanceInfo)
{
    static const Section kSections[] = {kAnswerSection, kAdditionalDataSection};

    Error   error      = kErrorNone;
    Section srvSection = ((mType == kSrvQuery) || (mType == kSrvTxtQuery)) ? kAnswerSection : kAdditionalDataSection;
    Section txtSection = ((mType == kTxtQuery) || (mType == kSrvTxtQuery)) ? kAnswerSection : kAdditionalDataSection;

    if (mType == kPtrQuery)
    {
        DnsLabel instanceLabel;

        SuccessOrExit(error = ExtractServiceInstanceLabel(aInstanceInfo.mFullName, instanceLabel));
        mSection = kAnswerSection;
        SuccessOrExit(error = AppendPtrRecord(instanceLabel, aInstanceInfo.mTtl));
    }

    for (Section section : kSections)
    {
        mSection = section;

        if (mSection == kAdditionalDataSection)
        {
            VerifyOrExit(!(Get<Server>().mTestMode & kTestModeEmptyAdditionalSection));
        }

        if (srvSection == mSection)
        {
            SuccessOrExit(error = AppendSrvRecord(aInstanceInfo));
        }

        if (txtSection == mSection)
        {
            SuccessOrExit(error = AppendTxtRecord(aInstanceInfo));
        }
    }

    error = AppendHostAddresses(aInstanceInfo);

exit:
    Finalize(error);
}

void Server::QueryTransaction::Answer(const HostInfo &aHostInfo)
{
    Error error;

    mSection = kAnswerSection;
    error    = AppendHostAddresses(aHostInfo);

    Finalize(error);
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
            query.Answer(aInstanceInfo);
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
            query.Answer(aHostInfo);
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

void Server::Response::GetQueryTypeAndName(DnsQueryType &aType, DnsName &aName) const
{
    ReadQueryName(aName);

    aType = kDnsQueryBrowse;

    switch (mType)
    {
    case kPtrQuery:
        break;

    case kSrvQuery:
    case kTxtQuery:
    case kSrvTxtQuery:
        aType = kDnsQueryResolve;
        break;

    case kAaaaQuery:
        aType = kDnsQueryResolveHost;
        break;
    }
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
            query.Finalize(kErrorNone);
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

void Server::QueryTransaction::Finalize(Error aError)
{
    DnsName name;

    ReadQueryName(name);
    Get<Server>().mQueryUnsubscribe.InvokeIfSet(name);

    mHeader.SetResponseCode((aError == kErrorNone) ? Header::kResponseSuccess : Header::kResponseServerFailure);
    Send(mMessageInfo);

    // Set the `mMessage` to null to indicate that
    // `QueryTransaction` is unused.
    mMessage = nullptr;
}

void Server::UpdateResponseCounters(ResponseCode aResponseCode)
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
