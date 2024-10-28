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

#include "instance/instance.hpp"

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
    , mSocket(aInstance, *this)
#if OPENTHREAD_CONFIG_DNSSD_DISCOVERY_PROXY_ENABLE
    , mDiscoveryProxy(aInstance)
#endif
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

    SuccessOrExit(error = mSocket.Open());
    SuccessOrExit(error = mSocket.Bind(kPort, kBindUnspecifiedNetif ? Ip6::kNetifUnspecified : Ip6::kNetifThread));

#if OPENTHREAD_CONFIG_SRP_SERVER_ENABLE
    Get<Srp::Server>().HandleDnssdServerStateChange();
#endif

    LogInfo("Started");

#if OPENTHREAD_CONFIG_DNSSD_DISCOVERY_PROXY_ENABLE
    mDiscoveryProxy.UpdateState();
#endif

exit:
    if (error != kErrorNone)
    {
        IgnoreError(mSocket.Close());
    }

    return error;
}

void Server::Stop(void)
{
    for (ProxyQuery &query : mProxyQueries)
    {
        Finalize(query, Header::kResponseServerFailure);
    }

#if OPENTHREAD_CONFIG_DNSSD_DISCOVERY_PROXY_ENABLE
    mDiscoveryProxy.Stop();
#endif

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
    ResponseCode rcode         = Header::kResponseSuccess;
    bool         shouldRespond = true;
    Response     response(GetInstance());

#if OPENTHREAD_CONFIG_DNS_UPSTREAM_QUERY_ENABLE
    if (mEnableUpstreamQuery && ShouldForwardToUpstream(aRequest))
    {
        Error error = ResolveByUpstream(aRequest);

        if (error == kErrorNone)
        {
            ExitNow();
        }

        LogWarnOnError(error, "forwarding to upstream");

        rcode = Header::kResponseServerFailure;

        // Continue to allocate and prepare the response message
        // to send the `kResponseServerFailure` response code.
    }
#endif

    SuccessOrExit(response.AllocateAndInitFrom(aRequest));

#if OPENTHREAD_CONFIG_DNS_UPSTREAM_QUERY_ENABLE
    // Forwarding the query to the upstream may have already set the
    // response error code.
    SuccessOrExit(rcode);
#endif

    SuccessOrExit(rcode = aRequest.ParseQuestions(mTestMode, shouldRespond));
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

    ResolveByProxy(response, *aRequest.mMessageInfo);

exit:
    if (rcode != Header::kResponseSuccess)
    {
        response.SetResponseCode(rcode);
    }

    if (shouldRespond)
    {
        response.Send(*aRequest.mMessageInfo);
    }
}

Server::Response::Response(Instance &aInstance)
    : InstanceLocator(aInstance)
{
    // `mHeader` constructors already clears it

    mOffsets.Clear();
}

Error Server::Response::AllocateAndInitFrom(const Request &aRequest)
{
    Error error = kErrorNone;

    mMessage.Reset(Get<Server>().mSocket.NewMessage());
    VerifyOrExit(!mMessage.IsNull(), error = kErrorNoBufs);

    mHeader.SetType(Header::kTypeResponse);
    mHeader.SetMessageId(aRequest.mHeader.GetMessageId());
    mHeader.SetQueryType(aRequest.mHeader.GetQueryType());

    if (aRequest.mHeader.IsRecursionDesiredFlagSet())
    {
        mHeader.SetRecursionDesiredFlag();
    }

    // Append the empty header to reserve room for it in the message.
    // Header will be updated in the message before sending it.
    error = mMessage->Append(mHeader);

exit:
    if (error != kErrorNone)
    {
        mMessage.Free();
    }

    return error;
}

void Server::Response::Send(const Ip6::MessageInfo &aMessageInfo)
{
    ResponseCode rcode = mHeader.GetResponseCode();

    VerifyOrExit(!mMessage.IsNull());

    if (rcode == Header::kResponseServerFailure)
    {
        mHeader.SetQuestionCount(0);
        mHeader.SetAnswerCount(0);
        mHeader.SetAdditionalRecordCount(0);
        IgnoreError(mMessage->SetLength(sizeof(Header)));
    }

    mMessage->Write(0, mHeader);

    SuccessOrExit(Get<Server>().mSocket.SendTo(*mMessage, aMessageInfo));

    // When `SendTo()` returns success it takes over ownership of
    // the given message, so we release ownership of `mMessage`.

    mMessage.Release();

    LogInfo("Send response, rcode:%u", rcode);

    Get<Server>().UpdateResponseCounters(rcode);

exit:
    return;
}

Server::ResponseCode Server::Request::ParseQuestions(uint8_t aTestMode, bool &aShouldRespond)
{
    // Parse header and questions from a `Request` query message and
    // determine the `QueryType`.

    ResponseCode rcode         = Header::kResponseFormatError;
    uint16_t     offset        = sizeof(Header);
    uint16_t     questionCount = mHeader.GetQuestionCount();
    Question     question;

    aShouldRespond = true;

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
    case ResourceRecord::kTypeA:
        mType = kAQuery;
        break;
    default:
        ExitNow(rcode = Header::kResponseNotImplemented);
    }

    if (questionCount > 1)
    {
        VerifyOrExit(!(aTestMode & kTestModeRejectMultiQuestionQuery));
        VerifyOrExit(!(aTestMode & kTestModeIgnoreMultiQuestionQuery), aShouldRespond = false);

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

    Error        error = kErrorNone;
    Name::Buffer name;
    uint16_t     offset;

    offset = sizeof(Header);
    SuccessOrExit(error = Name::ReadName(*mMessage, offset, name));

    switch (mType)
    {
    case kPtrQuery:
        // `mOffsets.mServiceName` may be updated as we read labels and if we
        // determine that the query name is a sub-type service.
        mOffsets.mServiceName = sizeof(Header);
        break;

    case kSrvQuery:
    case kTxtQuery:
    case kSrvTxtQuery:
        mOffsets.mInstanceName = sizeof(Header);
        break;

    case kAaaaQuery:
    case kAQuery:
        mOffsets.mHostName = sizeof(Header);
        break;
    }

    // Read the query name labels one by one to check if the name is
    // service sub-type and also check that it is sub-domain of the
    // default domain name and determine its offset

    offset = sizeof(Header);

    while (true)
    {
        Name::LabelBuffer label;
        uint8_t           labelLength = sizeof(label);
        uint16_t          comapreOffset;

        SuccessOrExit(error = Name::ReadLabel(*mMessage, offset, label, labelLength));

        if ((mType == kPtrQuery) && StringMatch(label, kSubLabel, kStringCaseInsensitiveMatch))
        {
            mOffsets.mServiceName = offset;
        }

        comapreOffset = offset;

        if (Name::CompareName(*mMessage, comapreOffset, kDefaultDomainName) == kErrorNone)
        {
            mOffsets.mDomainName = offset;
            ExitNow();
        }
    }

    error = kErrorParse;

exit:
    return error;
}

void Server::Response::ReadQueryName(Name::Buffer &aName) const { Server::ReadQueryName(*mMessage, aName); }

bool Server::Response::QueryNameMatches(const char *aName) const { return Server::QueryNameMatches(*mMessage, aName); }

Error Server::Response::AppendQueryName(void) { return Name::AppendPointerLabel(sizeof(Header), *mMessage); }

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

    mOffsets.mInstanceName = mMessage->GetLength();
    SuccessOrExit(error = Name::AppendLabel(aInstanceLabel, *mMessage));
    SuccessOrExit(error = Name::AppendPointerLabel(mOffsets.mServiceName, *mMessage));

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
    Error        error = kErrorNone;
    SrvRecord    srvRecord;
    uint16_t     recordOffset;
    Name::Buffer hostLabels;

    SuccessOrExit(error = Name::ExtractLabels(aHostName, kDefaultDomainName, hostLabels));

    srvRecord.Init();
    srvRecord.SetTtl(aTtl);
    srvRecord.SetPriority(aPriority);
    srvRecord.SetWeight(aWeight);
    srvRecord.SetPort(aPort);

    SuccessOrExit(error = Name::AppendPointerLabel(mOffsets.mInstanceName, *mMessage));

    recordOffset = mMessage->GetLength();
    SuccessOrExit(error = mMessage->Append(srvRecord));

    mOffsets.mHostName = mMessage->GetLength();
    SuccessOrExit(error = Name::AppendMultipleLabels(hostLabels, *mMessage));
    SuccessOrExit(error = Name::AppendPointerLabel(mOffsets.mDomainName, *mMessage));

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

    return AppendHostAddresses(kIp6AddrType, addrs, addrsLength, ttl);
}
#endif

Error Server::Response::AppendHostAddresses(AddrType aAddrType, const HostInfo &aHostInfo)
{
    return AppendHostAddresses(aAddrType, AsCoreTypePtr(aHostInfo.mAddresses), aHostInfo.mAddressNum, aHostInfo.mTtl);
}

Error Server::Response::AppendHostAddresses(const ServiceInstanceInfo &aInstanceInfo)
{
    return AppendHostAddresses(kIp6AddrType, AsCoreTypePtr(aInstanceInfo.mAddresses), aInstanceInfo.mAddressNum,
                               aInstanceInfo.mTtl);
}

Error Server::Response::AppendHostAddresses(AddrType            aAddrType,
                                            const Ip6::Address *aAddrs,
                                            uint16_t            aAddrsLength,
                                            uint32_t            aTtl)
{
    Error error = kErrorNone;

    for (uint16_t index = 0; index < aAddrsLength; index++)
    {
        const Ip6::Address &address = aAddrs[index];

        switch (aAddrType)
        {
        case kIp6AddrType:
            SuccessOrExit(error = AppendAaaaRecord(address, aTtl));
            break;

        case kIp4AddrType:
            SuccessOrExit(error = AppendARecord(address, aTtl));
            break;
        }
    }

exit:
    return error;
}

Error Server::Response::AppendAaaaRecord(const Ip6::Address &aAddress, uint32_t aTtl)
{
    Error      error = kErrorNone;
    AaaaRecord aaaaRecord;

    VerifyOrExit(!aAddress.IsIp4Mapped());

    aaaaRecord.Init();
    aaaaRecord.SetTtl(aTtl);
    aaaaRecord.SetAddress(aAddress);

    SuccessOrExit(error = Name::AppendPointerLabel(mOffsets.mHostName, *mMessage));
    SuccessOrExit(error = mMessage->Append(aaaaRecord));
    IncResourceRecordCount();

exit:
    return error;
}

Error Server::Response::AppendARecord(const Ip6::Address &aAddress, uint32_t aTtl)
{
    Error        error = kErrorNone;
    ARecord      aRecord;
    Ip4::Address ip4Address;

    SuccessOrExit(ip4Address.ExtractFromIp4MappedIp6Address(aAddress));

    aRecord.Init();
    aRecord.SetTtl(aTtl);
    aRecord.SetAddress(ip4Address);

    SuccessOrExit(error = Name::AppendPointerLabel(mOffsets.mHostName, *mMessage));
    SuccessOrExit(error = mMessage->Append(aRecord));
    IncResourceRecordCount();

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

    SuccessOrExit(error = Name::AppendPointerLabel(mOffsets.mInstanceName, *mMessage));
    SuccessOrExit(error = mMessage->Append(txtRecord));
    SuccessOrExit(error = mMessage->AppendBytes(aTxtData, aTxtLength));

    IncResourceRecordCount();

exit:
    return error;
}

void Server::Response::UpdateRecordLength(ResourceRecord &aRecord, uint16_t aOffset)
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

#if OT_SHOULD_LOG_AT(OT_LOG_LEVEL_INFO)
void Server::Response::Log(void) const
{
    Name::Buffer name;

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
        "A",         // (5) kAQuery
    };

    struct EumCheck
    {
        InitEnumValidatorCounter();
        ValidateNextEnum(kPtrQuery);
        ValidateNextEnum(kSrvQuery);
        ValidateNextEnum(kTxtQuery);
        ValidateNextEnum(kSrvTxtQuery);
        ValidateNextEnum(kAaaaQuery);
        ValidateNextEnum(kAQuery);
    };

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

        if ((mType == kAaaaQuery) || (mType == kAQuery))
        {
            if (QueryNameMatches(host.GetFullName()))
            {
                mSection = (mType == kAaaaQuery) ? kAnswerSection : kAdditionalDataSection;
                error    = AppendHostAddresses(host);
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

#if OPENTHREAD_CONFIG_DNS_UPSTREAM_QUERY_ENABLE
bool Server::ShouldForwardToUpstream(const Request &aRequest)
{
    bool         shouldForward = false;
    uint16_t     readOffset;
    Name::Buffer name;

    VerifyOrExit(aRequest.mHeader.IsRecursionDesiredFlagSet());
    readOffset = sizeof(Header);

    for (uint16_t i = 0; i < aRequest.mHeader.GetQuestionCount(); i++)
    {
        SuccessOrExit(Name::ReadName(*aRequest.mMessage, readOffset, name));
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
    else
    {
        error = kErrorResponseTimeout;
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

    VerifyOrExit(newTxn != nullptr, mCounters.mUpstreamDnsCounters.mFailures++);

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
    mCounters.mUpstreamDnsCounters.mQueries++;

exit:
    return error;
}
#endif // OPENTHREAD_CONFIG_DNS_UPSTREAM_QUERY_ENABLE

void Server::ResolveByProxy(Response &aResponse, const Ip6::MessageInfo &aMessageInfo)
{
    ProxyQuery    *query;
    ProxyQueryInfo info;

#if OPENTHREAD_CONFIG_DNSSD_DISCOVERY_PROXY_ENABLE
    VerifyOrExit(mQuerySubscribe.IsSet() || mDiscoveryProxy.IsRunning());
#else
    VerifyOrExit(mQuerySubscribe.IsSet());
#endif

    // We try to convert `aResponse.mMessage` to a `ProxyQuery` by
    // appending `ProxyQueryInfo` to it.

    info.mType        = aResponse.mType;
    info.mMessageInfo = aMessageInfo;
    info.mExpireTime  = TimerMilli::GetNow() + kQueryTimeout;
    info.mOffsets     = aResponse.mOffsets;

#if OPENTHREAD_CONFIG_DNSSD_DISCOVERY_PROXY_ENABLE
    info.mAction = kNoAction;
#endif

    if (aResponse.mMessage->Append(info) != kErrorNone)
    {
        aResponse.SetResponseCode(Header::kResponseServerFailure);
        ExitNow();
    }

    // Take over the ownership of `aResponse.mMessage` and add it as a
    // `ProxyQuery` in `mProxyQueries` list.

    query = aResponse.mMessage.Release();

    query->Write(0, aResponse.mHeader);
    mProxyQueries.Enqueue(*query);

    mTimer.FireAtIfEarlier(info.mExpireTime);

#if OPENTHREAD_CONFIG_DNSSD_DISCOVERY_PROXY_ENABLE
    if (mQuerySubscribe.IsSet())
#endif
    {
        Name::Buffer name;

        ReadQueryName(*query, name);
        mQuerySubscribe.Invoke(name);
    }
#if OPENTHREAD_CONFIG_DNSSD_DISCOVERY_PROXY_ENABLE
    else
    {
        mDiscoveryProxy.Resolve(*query, info);
    }
#endif

exit:
    return;
}

void Server::ReadQueryName(const Message &aQuery, Name::Buffer &aName)
{
    uint16_t offset = sizeof(Header);

    IgnoreError(Name::ReadName(aQuery, offset, aName));
}

bool Server::QueryNameMatches(const Message &aQuery, const char *aName)
{
    uint16_t offset = sizeof(Header);

    return (Name::CompareName(aQuery, offset, aName) == kErrorNone);
}

void Server::ReadQueryInstanceName(const ProxyQuery &aQuery, const ProxyQueryInfo &aInfo, Name::Buffer &aName)
{
    uint16_t offset = aInfo.mOffsets.mInstanceName;

    IgnoreError(Name::ReadName(aQuery, offset, aName, sizeof(aName)));
}

void Server::ReadQueryInstanceName(const ProxyQuery     &aQuery,
                                   const ProxyQueryInfo &aInfo,
                                   Name::LabelBuffer    &aInstanceLabel,
                                   Name::Buffer         &aServiceType)
{
    // Reads the service instance label and service type with domain
    // name stripped.

    uint16_t offset      = aInfo.mOffsets.mInstanceName;
    uint8_t  labelLength = sizeof(aInstanceLabel);

    IgnoreError(Dns::Name::ReadLabel(aQuery, offset, aInstanceLabel, labelLength));
    IgnoreError(Dns::Name::ReadName(aQuery, offset, aServiceType));
    IgnoreError(StripDomainName(aServiceType));
}

bool Server::QueryInstanceNameMatches(const ProxyQuery &aQuery, const ProxyQueryInfo &aInfo, const char *aName)
{
    uint16_t offset = aInfo.mOffsets.mInstanceName;

    return (Name::CompareName(aQuery, offset, aName) == kErrorNone);
}

void Server::ReadQueryHostName(const ProxyQuery &aQuery, const ProxyQueryInfo &aInfo, Name::Buffer &aName)
{
    uint16_t offset = aInfo.mOffsets.mHostName;

    IgnoreError(Name::ReadName(aQuery, offset, aName, sizeof(aName)));
}

bool Server::QueryHostNameMatches(const ProxyQuery &aQuery, const ProxyQueryInfo &aInfo, const char *aName)
{
    uint16_t offset = aInfo.mOffsets.mHostName;

    return (Name::CompareName(aQuery, offset, aName) == kErrorNone);
}

Error Server::StripDomainName(Name::Buffer &aName)
{
    // In-place removes the domain name from `aName`.

    return Name::StripName(aName, kDefaultDomainName);
}

Error Server::StripDomainName(const char *aFullName, Name::Buffer &aLabels)
{
    // Remove the domain name from `aFullName` and copies
    // the result into `aLabels`.

    return Name::ExtractLabels(aFullName, kDefaultDomainName, aLabels, sizeof(aLabels));
}

void Server::ConstructFullName(const char *aLabels, Name::Buffer &aFullName)
{
    // Construct a full name by appending the default domain name
    // to `aLabels`.

    StringWriter fullName(aFullName, sizeof(aFullName));

    fullName.Append("%s.%s", aLabels, kDefaultDomainName);
}

void Server::ConstructFullInstanceName(const char *aInstanceLabel, const char *aServiceType, Name::Buffer &aFullName)
{
    StringWriter fullName(aFullName, sizeof(aFullName));

    fullName.Append("%s.%s.%s", aInstanceLabel, aServiceType, kDefaultDomainName);
}

void Server::ConstructFullServiceSubTypeName(const char   *aServiceType,
                                             const char   *aSubTypeLabel,
                                             Name::Buffer &aFullName)
{
    StringWriter fullName(aFullName, sizeof(aFullName));

    fullName.Append("%s._sub.%s.%s", aSubTypeLabel, aServiceType, kDefaultDomainName);
}

Error Server::Response::ExtractServiceInstanceLabel(const char *aInstanceName, Name::LabelBuffer &aLabel)
{
    uint16_t     offset;
    Name::Buffer serviceName;

    offset = mOffsets.mServiceName;
    IgnoreError(Name::ReadName(*mMessage, offset, serviceName));

    return Name::ExtractLabels(aInstanceName, serviceName, aLabel);
}

void Server::RemoveQueryAndPrepareResponse(ProxyQuery &aQuery, ProxyQueryInfo &aInfo, Response &aResponse)
{
#if OPENTHREAD_CONFIG_DNSSD_DISCOVERY_PROXY_ENABLE
    mDiscoveryProxy.CancelAction(aQuery, aInfo);
#endif

    mProxyQueries.Dequeue(aQuery);
    aInfo.RemoveFrom(aQuery);

    if (mQueryUnsubscribe.IsSet())
    {
        Name::Buffer name;

        ReadQueryName(aQuery, name);
        mQueryUnsubscribe.Invoke(name);
    }

    aResponse.InitFrom(aQuery, aInfo);
}

void Server::Response::InitFrom(ProxyQuery &aQuery, const ProxyQueryInfo &aInfo)
{
    mMessage.Reset(&aQuery);
    IgnoreError(mMessage->Read(0, mHeader));
    mType    = aInfo.mType;
    mOffsets = aInfo.mOffsets;
}

void Server::Response::Answer(const ServiceInstanceInfo &aInstanceInfo, const Ip6::MessageInfo &aMessageInfo)
{
    static const Section kSections[] = {kAnswerSection, kAdditionalDataSection};

    Error   error      = kErrorNone;
    Section srvSection = ((mType == kSrvQuery) || (mType == kSrvTxtQuery)) ? kAnswerSection : kAdditionalDataSection;
    Section txtSection = ((mType == kTxtQuery) || (mType == kSrvTxtQuery)) ? kAnswerSection : kAdditionalDataSection;

    if (mType == kPtrQuery)
    {
        Name::LabelBuffer instanceLabel;

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
    if (error != kErrorNone)
    {
        SetResponseCode(Header::kResponseServerFailure);
    }

    Send(aMessageInfo);
}

void Server::Response::Answer(const HostInfo &aHostInfo, const Ip6::MessageInfo &aMessageInfo)
{
    // Caller already ensures that `mType` is either `kAaaaQuery` or
    // `kAQuery`.

    AddrType addrType = (mType == kAaaaQuery) ? kIp6AddrType : kIp4AddrType;

    mSection = kAnswerSection;

    if (AppendHostAddresses(addrType, aHostInfo) != kErrorNone)
    {
        SetResponseCode(Header::kResponseServerFailure);
    }

    Send(aMessageInfo);
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

    // It is safe to remove entries from `mProxyQueries` as we iterate
    // over it since it is a `MessageQueue`.

    for (ProxyQuery &query : mProxyQueries)
    {
        bool           canAnswer = false;
        ProxyQueryInfo info;

        info.ReadFrom(query);

        switch (info.mType)
        {
        case kPtrQuery:
            canAnswer = QueryNameMatches(query, aServiceFullName);
            break;

        case kSrvQuery:
        case kTxtQuery:
        case kSrvTxtQuery:
            canAnswer = QueryNameMatches(query, aInstanceInfo.mFullName);
            break;

        case kAaaaQuery:
        case kAQuery:
            break;
        }

        if (canAnswer)
        {
            Response response(GetInstance());

            RemoveQueryAndPrepareResponse(query, info, response);
            response.Answer(aInstanceInfo, info.mMessageInfo);
        }
    }
}

void Server::HandleDiscoveredHost(const char *aHostFullName, const HostInfo &aHostInfo)
{
    OT_ASSERT(StringEndsWith(aHostFullName, Name::kLabelSeparatorChar));

    for (ProxyQuery &query : mProxyQueries)
    {
        ProxyQueryInfo info;

        info.ReadFrom(query);

        switch (info.mType)
        {
        case kAaaaQuery:
        case kAQuery:
            if (QueryNameMatches(query, aHostFullName))
            {
                Response response(GetInstance());

                RemoveQueryAndPrepareResponse(query, info, response);
                response.Answer(aHostInfo, info.mMessageInfo);
            }

            break;

        default:
            break;
        }
    }
}

const otDnssdQuery *Server::GetNextQuery(const otDnssdQuery *aQuery) const
{
    const ProxyQuery *query = static_cast<const ProxyQuery *>(aQuery);

    return (query == nullptr) ? mProxyQueries.GetHead() : query->GetNext();
}

Server::DnsQueryType Server::GetQueryTypeAndName(const otDnssdQuery *aQuery, Dns::Name::Buffer &aName)
{
    const ProxyQuery *query = static_cast<const ProxyQuery *>(aQuery);
    ProxyQueryInfo    info;
    DnsQueryType      type;

    ReadQueryName(*query, aName);
    info.ReadFrom(*query);

    type = kDnsQueryBrowse;

    switch (info.mType)
    {
    case kPtrQuery:
        break;

    case kSrvQuery:
    case kTxtQuery:
    case kSrvTxtQuery:
        type = kDnsQueryResolve;
        break;

    case kAaaaQuery:
    case kAQuery:
        type = kDnsQueryResolveHost;
        break;
    }

    return type;
}

void Server::HandleTimer(void)
{
    NextFireTime nextExpire;

    for (ProxyQuery &query : mProxyQueries)
    {
        ProxyQueryInfo info;

        info.ReadFrom(query);

        if (info.mExpireTime <= nextExpire.GetNow())
        {
            Finalize(query, Header::kResponseSuccess);
        }
        else
        {
            nextExpire.UpdateIfEarlier(info.mExpireTime);
        }
    }

#if OPENTHREAD_CONFIG_DNS_UPSTREAM_QUERY_ENABLE
    for (UpstreamQueryTransaction &query : mUpstreamQueryTransactions)
    {
        if (!query.IsValid())
        {
            continue;
        }

        if (query.GetExpireTime() <= nextExpire.GetNow())
        {
            otPlatDnsCancelUpstreamQuery(&GetInstance(), &query);
        }
        else
        {
            nextExpire.UpdateIfEarlier(query.GetExpireTime());
        }
    }
#endif

    mTimer.FireAtIfEarlier(nextExpire);
}

void Server::Finalize(ProxyQuery &aQuery, ResponseCode aResponseCode)
{
    Response       response(GetInstance());
    ProxyQueryInfo info;

    info.ReadFrom(aQuery);
    RemoveQueryAndPrepareResponse(aQuery, info, response);

    response.SetResponseCode(aResponseCode);
    response.Send(info.mMessageInfo);
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
        mCounters.mUpstreamDnsCounters.mResponses++;
        LogInfo("Upstream query transaction %d completed.", index);
    }
    else
    {
        mCounters.mUpstreamDnsCounters.mFailures++;
        LogWarn("Upstream query transaction %d closed: %s.", index, ErrorToString(aError));
    }
    aTxn.Reset();
}
#endif

#if OPENTHREAD_CONFIG_DNSSD_DISCOVERY_PROXY_ENABLE

Server::DiscoveryProxy::DiscoveryProxy(Instance &aInstance)
    : InstanceLocator(aInstance)
    , mIsRunning(false)
{
}

void Server::DiscoveryProxy::UpdateState(void)
{
    if (Get<Server>().IsRunning() && Get<Dnssd>().IsReady() && Get<BorderRouter::InfraIf>().IsRunning())
    {
        Start();
    }
    else
    {
        Stop();
    }
}

void Server::DiscoveryProxy::Start(void)
{
    VerifyOrExit(!mIsRunning);
    mIsRunning = true;
    LogInfo("Started discovery proxy");

exit:
    return;
}

void Server::DiscoveryProxy::Stop(void)
{
    VerifyOrExit(mIsRunning);

    for (ProxyQuery &query : Get<Server>().mProxyQueries)
    {
        Get<Server>().Finalize(query, Header::kResponseSuccess);
    }

    mIsRunning = false;
    LogInfo("Stopped discovery proxy");

exit:
    return;
}

void Server::DiscoveryProxy::Resolve(ProxyQuery &aQuery, ProxyQueryInfo &aInfo)
{
    ProxyAction action = kNoAction;

    switch (aInfo.mType)
    {
    case kPtrQuery:
        action = kBrowsing;
        break;

    case kSrvQuery:
    case kSrvTxtQuery:
        action = kResolvingSrv;
        break;

    case kTxtQuery:
        action = kResolvingTxt;
        break;

    case kAaaaQuery:
        action = kResolvingIp6Address;
        break;
    case kAQuery:
        action = kResolvingIp4Address;
        break;
    }

    Perform(action, aQuery, aInfo);
}

void Server::DiscoveryProxy::Perform(ProxyAction aAction, ProxyQuery &aQuery, ProxyQueryInfo &aInfo)
{
    bool         shouldStart;
    Name::Buffer name;

    VerifyOrExit(aAction != kNoAction);

    // The order of the steps below is crucial. First, we read the
    // name associated with the action. Then we check if another
    // query has an active browser/resolver for the same name. This
    // helps us determine if a new browser/resolver is needed. Then,
    // we update the `ProxyQueryInfo` within `aQuery` to reflect the
    // `aAction` being performed. Finally, if necessary, we start the
    // proper browser/resolver on DNS-SD/mDNS. Placing this last
    // ensures correct processing even if a DNS-SD/mDNS callback is
    // invoked immediately.

    ReadNameFor(aAction, aQuery, aInfo, name);

    shouldStart = !HasActive(aAction, name);

    aInfo.mAction = aAction;
    aInfo.UpdateIn(aQuery);

    VerifyOrExit(shouldStart);
    UpdateProxy(kStart, aAction, aQuery, aInfo, name);

exit:
    return;
}

void Server::DiscoveryProxy::ReadNameFor(ProxyAction     aAction,
                                         ProxyQuery     &aQuery,
                                         ProxyQueryInfo &aInfo,
                                         Name::Buffer   &aName) const
{
    // Read the name corresponding to `aAction` from `aQuery`.

    switch (aAction)
    {
    case kNoAction:
        break;
    case kBrowsing:
        ReadQueryName(aQuery, aName);
        break;
    case kResolvingSrv:
    case kResolvingTxt:
        ReadQueryInstanceName(aQuery, aInfo, aName);
        break;
    case kResolvingIp6Address:
    case kResolvingIp4Address:
        ReadQueryHostName(aQuery, aInfo, aName);
        break;
    }
}

void Server::DiscoveryProxy::CancelAction(ProxyQuery &aQuery, ProxyQueryInfo &aInfo)
{
    // Cancel the current action for a given `aQuery`, then
    // determine if we need to stop any browser/resolver
    // on infrastructure.

    ProxyAction  action = aInfo.mAction;
    Name::Buffer name;

    VerifyOrExit(mIsRunning);
    VerifyOrExit(action != kNoAction);

    // We first update the `aInfo` on `aQuery` before calling
    // `HasActive()`. This ensures that the current query is not
    // taken into account when we try to determine if any query
    // is waiting for same `aAction` browser/resolver.

    ReadNameFor(action, aQuery, aInfo, name);

    aInfo.mAction = kNoAction;
    aInfo.UpdateIn(aQuery);

    VerifyOrExit(!HasActive(action, name));
    UpdateProxy(kStop, action, aQuery, aInfo, name);

exit:
    return;
}

void Server::DiscoveryProxy::UpdateProxy(Command               aCommand,
                                         ProxyAction           aAction,
                                         const ProxyQuery     &aQuery,
                                         const ProxyQueryInfo &aInfo,
                                         Name::Buffer         &aName)
{
    // Start or stop browser/resolver corresponding to `aAction`.
    // `aName` may be changed.

    switch (aAction)
    {
    case kNoAction:
        break;
    case kBrowsing:
        StartOrStopBrowser(aCommand, aName);
        break;
    case kResolvingSrv:
        StartOrStopSrvResolver(aCommand, aQuery, aInfo);
        break;
    case kResolvingTxt:
        StartOrStopTxtResolver(aCommand, aQuery, aInfo);
        break;
    case kResolvingIp6Address:
        StartOrStopIp6Resolver(aCommand, aName);
        break;
    case kResolvingIp4Address:
        StartOrStopIp4Resolver(aCommand, aName);
        break;
    }
}

void Server::DiscoveryProxy::StartOrStopBrowser(Command aCommand, Name::Buffer &aServiceName)
{
    // Start or stop a service browser for a given service type
    // or sub-type.

    static const char kFullSubLabel[] = "._sub.";

    Dnssd::Browser browser;
    char          *ptr;

    browser.Clear();

    IgnoreError(StripDomainName(aServiceName));

    // Check if the service name is a sub-type with name
    // format: "<sub-label>._sub.<service-labels>.

    ptr = AsNonConst(StringFind(aServiceName, kFullSubLabel, kStringCaseInsensitiveMatch));

    if (ptr != nullptr)
    {
        *ptr = kNullChar;
        ptr += sizeof(kFullSubLabel) - 1;

        browser.mServiceType  = ptr;
        browser.mSubTypeLabel = aServiceName;
    }
    else
    {
        browser.mServiceType  = aServiceName;
        browser.mSubTypeLabel = nullptr;
    }

    browser.mInfraIfIndex = Get<BorderRouter::InfraIf>().GetIfIndex();
    browser.mCallback     = HandleBrowseResult;

    switch (aCommand)
    {
    case kStart:
        Get<Dnssd>().StartBrowser(browser);
        break;

    case kStop:
        Get<Dnssd>().StopBrowser(browser);
        break;
    }
}

void Server::DiscoveryProxy::StartOrStopSrvResolver(Command               aCommand,
                                                    const ProxyQuery     &aQuery,
                                                    const ProxyQueryInfo &aInfo)
{
    // Start or stop an SRV record resolver for a given query.

    Dnssd::SrvResolver resolver;
    Name::LabelBuffer  instanceLabel;
    Name::Buffer       serviceType;

    ReadQueryInstanceName(aQuery, aInfo, instanceLabel, serviceType);

    resolver.Clear();

    resolver.mServiceInstance = instanceLabel;
    resolver.mServiceType     = serviceType;
    resolver.mInfraIfIndex    = Get<BorderRouter::InfraIf>().GetIfIndex();
    resolver.mCallback        = HandleSrvResult;

    switch (aCommand)
    {
    case kStart:
        Get<Dnssd>().StartSrvResolver(resolver);
        break;

    case kStop:
        Get<Dnssd>().StopSrvResolver(resolver);
        break;
    }
}

void Server::DiscoveryProxy::StartOrStopTxtResolver(Command               aCommand,
                                                    const ProxyQuery     &aQuery,
                                                    const ProxyQueryInfo &aInfo)
{
    // Start or stop a TXT record resolver for a given query.

    Dnssd::TxtResolver resolver;
    Name::LabelBuffer  instanceLabel;
    Name::Buffer       serviceType;

    ReadQueryInstanceName(aQuery, aInfo, instanceLabel, serviceType);

    resolver.Clear();

    resolver.mServiceInstance = instanceLabel;
    resolver.mServiceType     = serviceType;
    resolver.mInfraIfIndex    = Get<BorderRouter::InfraIf>().GetIfIndex();
    resolver.mCallback        = HandleTxtResult;

    switch (aCommand)
    {
    case kStart:
        Get<Dnssd>().StartTxtResolver(resolver);
        break;

    case kStop:
        Get<Dnssd>().StopTxtResolver(resolver);
        break;
    }
}

void Server::DiscoveryProxy::StartOrStopIp6Resolver(Command aCommand, Name::Buffer &aHostName)
{
    // Start or stop an IPv6 address resolver for a given host name.

    Dnssd::AddressResolver resolver;

    IgnoreError(StripDomainName(aHostName));

    resolver.mHostName     = aHostName;
    resolver.mInfraIfIndex = Get<BorderRouter::InfraIf>().GetIfIndex();
    resolver.mCallback     = HandleIp6AddressResult;

    switch (aCommand)
    {
    case kStart:
        Get<Dnssd>().StartIp6AddressResolver(resolver);
        break;

    case kStop:
        Get<Dnssd>().StopIp6AddressResolver(resolver);
        break;
    }
}

void Server::DiscoveryProxy::StartOrStopIp4Resolver(Command aCommand, Name::Buffer &aHostName)
{
    // Start or stop an IPv4 address resolver for a given host name.

    Dnssd::AddressResolver resolver;

    IgnoreError(StripDomainName(aHostName));

    resolver.mHostName     = aHostName;
    resolver.mInfraIfIndex = Get<BorderRouter::InfraIf>().GetIfIndex();
    resolver.mCallback     = HandleIp4AddressResult;

    switch (aCommand)
    {
    case kStart:
        Get<Dnssd>().StartIp4AddressResolver(resolver);
        break;

    case kStop:
        Get<Dnssd>().StopIp4AddressResolver(resolver);
        break;
    }
}

bool Server::DiscoveryProxy::QueryMatches(const ProxyQuery     &aQuery,
                                          const ProxyQueryInfo &aInfo,
                                          ProxyAction           aAction,
                                          const Name::Buffer   &aName) const
{
    // Check whether `aQuery` is performing `aAction` and
    // its name matches `aName`.

    bool matches = false;

    VerifyOrExit(aInfo.mAction == aAction);

    switch (aAction)
    {
    case kBrowsing:
        VerifyOrExit(QueryNameMatches(aQuery, aName));
        break;
    case kResolvingSrv:
    case kResolvingTxt:
        VerifyOrExit(QueryInstanceNameMatches(aQuery, aInfo, aName));
        break;
    case kResolvingIp6Address:
    case kResolvingIp4Address:
        VerifyOrExit(QueryHostNameMatches(aQuery, aInfo, aName));
        break;
    case kNoAction:
        ExitNow();
    }

    matches = true;

exit:
    return matches;
}

bool Server::DiscoveryProxy::HasActive(ProxyAction aAction, const Name::Buffer &aName) const
{
    // Determine whether or not we have an active browser/resolver
    // corresponding to `aAction` for `aName`.

    bool has = false;

    for (const ProxyQuery &query : Get<Server>().mProxyQueries)
    {
        ProxyQueryInfo info;

        info.ReadFrom(query);

        if (QueryMatches(query, info, aAction, aName))
        {
            has = true;
            break;
        }
    }

    return has;
}

void Server::DiscoveryProxy::HandleBrowseResult(otInstance *aInstance, const otPlatDnssdBrowseResult *aResult)
{
    AsCoreType(aInstance).Get<Server>().mDiscoveryProxy.HandleBrowseResult(*aResult);
}

void Server::DiscoveryProxy::HandleBrowseResult(const Dnssd::BrowseResult &aResult)
{
    Name::Buffer serviceName;

    VerifyOrExit(mIsRunning);
    VerifyOrExit(aResult.mTtl != 0);
    VerifyOrExit(aResult.mInfraIfIndex == Get<BorderRouter::InfraIf>().GetIfIndex());

    if (aResult.mSubTypeLabel != nullptr)
    {
        ConstructFullServiceSubTypeName(aResult.mServiceType, aResult.mSubTypeLabel, serviceName);
    }
    else
    {
        ConstructFullName(aResult.mServiceType, serviceName);
    }

    HandleResult(kBrowsing, serviceName, &Response::AppendPtrRecord, ProxyResult(aResult));

exit:
    return;
}

void Server::DiscoveryProxy::HandleSrvResult(otInstance *aInstance, const otPlatDnssdSrvResult *aResult)
{
    AsCoreType(aInstance).Get<Server>().mDiscoveryProxy.HandleSrvResult(*aResult);
}

void Server::DiscoveryProxy::HandleSrvResult(const Dnssd::SrvResult &aResult)
{
    Name::Buffer instanceName;

    VerifyOrExit(mIsRunning);
    VerifyOrExit(aResult.mTtl != 0);
    VerifyOrExit(aResult.mInfraIfIndex == Get<BorderRouter::InfraIf>().GetIfIndex());

    ConstructFullInstanceName(aResult.mServiceInstance, aResult.mServiceType, instanceName);
    HandleResult(kResolvingSrv, instanceName, &Response::AppendSrvRecord, ProxyResult(aResult));

exit:
    return;
}

void Server::DiscoveryProxy::HandleTxtResult(otInstance *aInstance, const otPlatDnssdTxtResult *aResult)
{
    AsCoreType(aInstance).Get<Server>().mDiscoveryProxy.HandleTxtResult(*aResult);
}

void Server::DiscoveryProxy::HandleTxtResult(const Dnssd::TxtResult &aResult)
{
    Name::Buffer instanceName;

    VerifyOrExit(mIsRunning);
    VerifyOrExit(aResult.mTtl != 0);
    VerifyOrExit(aResult.mInfraIfIndex == Get<BorderRouter::InfraIf>().GetIfIndex());

    ConstructFullInstanceName(aResult.mServiceInstance, aResult.mServiceType, instanceName);
    HandleResult(kResolvingTxt, instanceName, &Response::AppendTxtRecord, ProxyResult(aResult));

exit:
    return;
}

void Server::DiscoveryProxy::HandleIp6AddressResult(otInstance *aInstance, const otPlatDnssdAddressResult *aResult)
{
    AsCoreType(aInstance).Get<Server>().mDiscoveryProxy.HandleIp6AddressResult(*aResult);
}

void Server::DiscoveryProxy::HandleIp6AddressResult(const Dnssd::AddressResult &aResult)
{
    bool         hasValidAddress = false;
    Name::Buffer fullHostName;

    VerifyOrExit(mIsRunning);
    VerifyOrExit(aResult.mInfraIfIndex == Get<BorderRouter::InfraIf>().GetIfIndex());

    for (uint16_t index = 0; index < aResult.mAddressesLength; index++)
    {
        const Dnssd::AddressAndTtl &entry   = aResult.mAddresses[index];
        const Ip6::Address         &address = AsCoreType(&entry.mAddress);

        if (entry.mTtl == 0)
        {
            continue;
        }

        if (IsProxyAddressValid(address))
        {
            hasValidAddress = true;
            break;
        }
    }

    VerifyOrExit(hasValidAddress);

    ConstructFullName(aResult.mHostName, fullHostName);
    HandleResult(kResolvingIp6Address, fullHostName, &Response::AppendHostIp6Addresses, ProxyResult(aResult));

exit:
    return;
}

void Server::DiscoveryProxy::HandleIp4AddressResult(otInstance *aInstance, const otPlatDnssdAddressResult *aResult)
{
    AsCoreType(aInstance).Get<Server>().mDiscoveryProxy.HandleIp4AddressResult(*aResult);
}

void Server::DiscoveryProxy::HandleIp4AddressResult(const Dnssd::AddressResult &aResult)
{
    bool         hasValidAddress = false;
    Name::Buffer fullHostName;

    VerifyOrExit(mIsRunning);
    VerifyOrExit(aResult.mInfraIfIndex == Get<BorderRouter::InfraIf>().GetIfIndex());

    for (uint16_t index = 0; index < aResult.mAddressesLength; index++)
    {
        const Dnssd::AddressAndTtl &entry   = aResult.mAddresses[index];
        const Ip6::Address         &address = AsCoreType(&entry.mAddress);

        if (entry.mTtl == 0)
        {
            continue;
        }

        if (address.IsIp4Mapped())
        {
            hasValidAddress = true;
            break;
        }
    }

    VerifyOrExit(hasValidAddress);

    ConstructFullName(aResult.mHostName, fullHostName);
    HandleResult(kResolvingIp4Address, fullHostName, &Response::AppendHostIp4Addresses, ProxyResult(aResult));

exit:
    return;
}

void Server::DiscoveryProxy::HandleResult(ProxyAction         aAction,
                                          const Name::Buffer &aName,
                                          ResponseAppender    aAppender,
                                          const ProxyResult  &aResult)
{
    // Common method that handles result from DNS-SD/mDNS. It
    // iterates over all `ProxyQuery` entries and checks if any entry
    // is waiting for the result of `aAction` for `aName`. Matching
    // queries are updated using the `aAppender` method pointer,
    // which appends the corresponding record(s) to the response. We
    // then determine the next action to be performed for the
    // `ProxyQuery` or if it can be finalized.

    ProxyQueryList nextActionQueries;
    ProxyQueryInfo info;
    ProxyAction    nextAction;

    for (ProxyQuery &query : Get<Server>().mProxyQueries)
    {
        Response response(GetInstance());
        bool     shouldFinalize;

        info.ReadFrom(query);

        if (!QueryMatches(query, info, aAction, aName))
        {
            continue;
        }

        CancelAction(query, info);

        nextAction = kNoAction;

        switch (aAction)
        {
        case kBrowsing:
            nextAction = kResolvingSrv;
            break;
        case kResolvingSrv:
            nextAction = (info.mType == kSrvQuery) ? kResolvingIp6Address : kResolvingTxt;
            break;
        case kResolvingTxt:
            nextAction = (info.mType == kTxtQuery) ? kNoAction : kResolvingIp6Address;
            break;
        case kNoAction:
        case kResolvingIp6Address:
        case kResolvingIp4Address:
            break;
        }

        shouldFinalize = (nextAction == kNoAction);

        if ((Get<Server>().mTestMode & kTestModeEmptyAdditionalSection) &&
            IsActionForAdditionalSection(nextAction, info.mType))
        {
            shouldFinalize = true;
        }

        Get<Server>().mProxyQueries.Dequeue(query);
        info.RemoveFrom(query);
        response.InitFrom(query, info);

        if ((response.*aAppender)(aResult) != kErrorNone)
        {
            response.SetResponseCode(Header::kResponseServerFailure);
            shouldFinalize = true;
        }

        if (shouldFinalize)
        {
            response.Send(info.mMessageInfo);
            continue;
        }

        // The `query` is not yet finished and we need to perform
        // the `nextAction` for it.

        // Reinitialize `response` as a `ProxyQuey` by updating
        // and appending `info` to it after the newly appended
        // records from `aResult` and saving the `mHeader`.

        info.mOffsets = response.mOffsets;
        info.mAction  = nextAction;
        response.mMessage->Write(0, response.mHeader);

        if (response.mMessage->Append(info) != kErrorNone)
        {
            response.SetResponseCode(Header::kResponseServerFailure);
            response.Send(info.mMessageInfo);
            continue;
        }

        // Take back ownership of `response.mMessage` as we still
        // treat it as a `ProxyQuery`.

        response.mMessage.Release();

        // We place the `query` in a separate list and add it back to
        // the main `mProxyQueries` list after we are done with the
        // current iteration. This ensures that other entries in the
        // `mProxyQueries` list are not updated or removed due to the
        // DNS-SD platform callback being invoked immediately when we
        // potentially start a browser or resolver to perform the
        // `nextAction` for `query`.

        nextActionQueries.Enqueue(query);
    }

    for (ProxyQuery &query : nextActionQueries)
    {
        nextActionQueries.Dequeue(query);

        info.ReadFrom(query);

        nextAction = info.mAction;

        info.mAction = kNoAction;
        info.UpdateIn(query);

        Get<Server>().mProxyQueries.Enqueue(query);
        Perform(nextAction, query, info);
    }
}

bool Server::DiscoveryProxy::IsActionForAdditionalSection(ProxyAction aAction, QueryType aQueryType)
{
    bool isForAddnlSection = false;

    switch (aAction)
    {
    case kResolvingSrv:
        VerifyOrExit((aQueryType == kSrvQuery) || (aQueryType == kSrvTxtQuery));
        break;
    case kResolvingTxt:
        VerifyOrExit((aQueryType == kTxtQuery) || (aQueryType == kSrvTxtQuery));
        break;

    case kResolvingIp6Address:
        VerifyOrExit(aQueryType == kAaaaQuery);
        break;

    case kResolvingIp4Address:
        VerifyOrExit(aQueryType == kAQuery);
        break;

    case kNoAction:
    case kBrowsing:
        ExitNow();
    }

    isForAddnlSection = true;

exit:
    return isForAddnlSection;
}

Error Server::Response::AppendPtrRecord(const ProxyResult &aResult)
{
    const Dnssd::BrowseResult *browseResult = aResult.mBrowseResult;

    mSection = kAnswerSection;

    return AppendPtrRecord(browseResult->mServiceInstance, browseResult->mTtl);
}

Error Server::Response::AppendSrvRecord(const ProxyResult &aResult)
{
    const Dnssd::SrvResult *srvResult = aResult.mSrvResult;
    Name::Buffer            fullHostName;

    mSection = ((mType == kSrvQuery) || (mType == kSrvTxtQuery)) ? kAnswerSection : kAdditionalDataSection;

    ConstructFullName(srvResult->mHostName, fullHostName);

    return AppendSrvRecord(fullHostName, srvResult->mTtl, srvResult->mPriority, srvResult->mWeight, srvResult->mPort);
}

Error Server::Response::AppendTxtRecord(const ProxyResult &aResult)
{
    const Dnssd::TxtResult *txtResult = aResult.mTxtResult;

    mSection = ((mType == kTxtQuery) || (mType == kSrvTxtQuery)) ? kAnswerSection : kAdditionalDataSection;

    return AppendTxtRecord(txtResult->mTxtData, txtResult->mTxtDataLength, txtResult->mTtl);
}

Error Server::Response::AppendHostIp6Addresses(const ProxyResult &aResult)
{
    Error                       error      = kErrorNone;
    const Dnssd::AddressResult *addrResult = aResult.mAddressResult;

    mSection = (mType == kAaaaQuery) ? kAnswerSection : kAdditionalDataSection;

    for (uint16_t index = 0; index < addrResult->mAddressesLength; index++)
    {
        const Dnssd::AddressAndTtl &entry   = addrResult->mAddresses[index];
        const Ip6::Address         &address = AsCoreType(&entry.mAddress);

        if (entry.mTtl == 0)
        {
            continue;
        }

        if (!IsProxyAddressValid(address))
        {
            continue;
        }

        SuccessOrExit(error = AppendAaaaRecord(address, entry.mTtl));
    }

exit:
    return error;
}

Error Server::Response::AppendHostIp4Addresses(const ProxyResult &aResult)
{
    Error                       error      = kErrorNone;
    const Dnssd::AddressResult *addrResult = aResult.mAddressResult;

    mSection = (mType == kAQuery) ? kAnswerSection : kAdditionalDataSection;

    for (uint16_t index = 0; index < addrResult->mAddressesLength; index++)
    {
        const Dnssd::AddressAndTtl &entry   = addrResult->mAddresses[index];
        const Ip6::Address         &address = AsCoreType(&entry.mAddress);

        if (entry.mTtl == 0)
        {
            continue;
        }

        SuccessOrExit(error = AppendARecord(address, entry.mTtl));
    }

exit:
    return error;
}

bool Server::IsProxyAddressValid(const Ip6::Address &aAddress)
{
    return !aAddress.IsLinkLocalUnicast() && !aAddress.IsMulticast() && !aAddress.IsUnspecified() &&
           !aAddress.IsLoopback();
}

#endif // OPENTHREAD_CONFIG_DNSSD_DISCOVERY_PROXY_ENABLE

} // namespace ServiceDiscovery
} // namespace Dns
} // namespace ot

#if OPENTHREAD_CONFIG_DNS_UPSTREAM_QUERY_ENABLE && OPENTHREAD_CONFIG_DNS_UPSTREAM_QUERY_MOCK_PLAT_APIS_ENABLE
void otPlatDnsStartUpstreamQuery(otInstance *aInstance, otPlatDnsUpstreamQuery *aTxn, const otMessage *aQuery)
{
    OT_UNUSED_VARIABLE(aInstance);
    OT_UNUSED_VARIABLE(aTxn);
    OT_UNUSED_VARIABLE(aQuery);
}

void otPlatDnsCancelUpstreamQuery(otInstance *aInstance, otPlatDnsUpstreamQuery *aTxn)
{
    otPlatDnsUpstreamQueryDone(aInstance, aTxn, nullptr);
}
#endif

#endif // OPENTHREAD_CONFIG_DNS_SERVER_ENABLE
