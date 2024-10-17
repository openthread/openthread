/*
 *  Copyright (c) 2017-2021, The OpenThread Authors.
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

#include "dns_client.hpp"

#if OPENTHREAD_CONFIG_DNS_CLIENT_ENABLE

#include "instance/instance.hpp"
#include "utils/static_counter.hpp"

/**
 * @file
 *   This file implements the DNS client.
 */

namespace ot {
namespace Dns {

RegisterLogModule("DnsClient");

//---------------------------------------------------------------------------------------------------------------------
// Client::QueryConfig

const char Client::QueryConfig::kDefaultServerAddressString[] = OPENTHREAD_CONFIG_DNS_CLIENT_DEFAULT_SERVER_IP6_ADDRESS;

Client::QueryConfig::QueryConfig(InitMode aMode)
{
    OT_UNUSED_VARIABLE(aMode);

    IgnoreError(GetServerSockAddr().GetAddress().FromString(kDefaultServerAddressString));
    GetServerSockAddr().SetPort(kDefaultServerPort);
    SetResponseTimeout(kDefaultResponseTimeout);
    SetMaxTxAttempts(kDefaultMaxTxAttempts);
    SetRecursionFlag(kDefaultRecursionDesired ? kFlagRecursionDesired : kFlagNoRecursion);
    SetServiceMode(kDefaultServiceMode);
#if OPENTHREAD_CONFIG_DNS_CLIENT_NAT64_ENABLE
    SetNat64Mode(kDefaultNat64Allowed ? kNat64Allow : kNat64Disallow);
#endif
    SetTransportProto(kDnsTransportUdp);
}

void Client::QueryConfig::SetFrom(const QueryConfig *aConfig, const QueryConfig &aDefaultConfig)
{
    // This method sets the config from `aConfig` replacing any
    // unspecified fields (value zero) with the fields from
    // `aDefaultConfig`. If `aConfig` is `nullptr` then
    // `aDefaultConfig` is used.

    if (aConfig == nullptr)
    {
        *this = aDefaultConfig;
        ExitNow();
    }

    *this = *aConfig;

    if (GetServerSockAddr().GetAddress().IsUnspecified())
    {
        GetServerSockAddr().GetAddress() = aDefaultConfig.GetServerSockAddr().GetAddress();
    }

    if (GetServerSockAddr().GetPort() == 0)
    {
        GetServerSockAddr().SetPort(aDefaultConfig.GetServerSockAddr().GetPort());
    }

    if (GetResponseTimeout() == 0)
    {
        SetResponseTimeout(aDefaultConfig.GetResponseTimeout());
    }

    if (GetMaxTxAttempts() == 0)
    {
        SetMaxTxAttempts(aDefaultConfig.GetMaxTxAttempts());
    }

    if (GetRecursionFlag() == kFlagUnspecified)
    {
        SetRecursionFlag(aDefaultConfig.GetRecursionFlag());
    }

#if OPENTHREAD_CONFIG_DNS_CLIENT_NAT64_ENABLE
    if (GetNat64Mode() == kNat64Unspecified)
    {
        SetNat64Mode(aDefaultConfig.GetNat64Mode());
    }
#endif

    if (GetServiceMode() == kServiceModeUnspecified)
    {
        SetServiceMode(aDefaultConfig.GetServiceMode());
    }

    if (GetTransportProto() == kDnsTransportUnspecified)
    {
        SetTransportProto(aDefaultConfig.GetTransportProto());
    }

exit:
    return;
}

//---------------------------------------------------------------------------------------------------------------------
// Client::Response

void Client::Response::SelectSection(Section aSection, uint16_t &aOffset, uint16_t &aNumRecord) const
{
    switch (aSection)
    {
    case kAnswerSection:
        aOffset    = mAnswerOffset;
        aNumRecord = mAnswerRecordCount;
        break;
    case kAdditionalDataSection:
    default:
        aOffset    = mAdditionalOffset;
        aNumRecord = mAdditionalRecordCount;
        break;
    }
}

Error Client::Response::GetName(char *aNameBuffer, uint16_t aNameBufferSize) const
{
    uint16_t offset = kNameOffsetInQuery;

    return Name::ReadName(*mQuery, offset, aNameBuffer, aNameBufferSize);
}

Error Client::Response::CheckForHostNameAlias(Section aSection, Name &aHostName) const
{
    // If the response includes a CNAME record mapping the query host
    // name to a canonical name, we update `aHostName` to the new alias
    // name. Otherwise `aHostName` remains as before. This method handles
    // when there are multiple CNAME records mapping the host name multiple
    // times. We limit number of changes to `kMaxCnameAliasNameChanges`
    // to detect and handle if the response contains CNAME record loops.

    Error       error;
    uint16_t    offset;
    uint16_t    numRecords;
    CnameRecord cnameRecord;

    VerifyOrExit(mMessage != nullptr, error = kErrorNotFound);

    for (uint16_t counter = 0; counter < kMaxCnameAliasNameChanges; counter++)
    {
        SelectSection(aSection, offset, numRecords);
        error = ResourceRecord::FindRecord(*mMessage, offset, numRecords, /* aIndex */ 0, aHostName, cnameRecord);

        if (error == kErrorNotFound)
        {
            error = kErrorNone;
            ExitNow();
        }

        SuccessOrExit(error);

        // A CNAME record was found. `offset` now points to after the
        // last read byte within the `mMessage` into the `cnameRecord`
        // (which is the start of the new canonical name).

        aHostName.SetFromMessage(*mMessage, offset);
        SuccessOrExit(error = Name::ParseName(*mMessage, offset));

        // Loop back to check if there may be a CNAME record for the
        // new `aHostName`.
    }

    error = kErrorParse;

exit:
    return error;
}

Error Client::Response::FindHostAddress(Section       aSection,
                                        const Name   &aHostName,
                                        uint16_t      aIndex,
                                        Ip6::Address &aAddress,
                                        uint32_t     &aTtl) const
{
    Error      error;
    uint16_t   offset;
    uint16_t   numRecords;
    Name       name = aHostName;
    AaaaRecord aaaaRecord;

    SuccessOrExit(error = CheckForHostNameAlias(aSection, name));

    SelectSection(aSection, offset, numRecords);
    SuccessOrExit(error = ResourceRecord::FindRecord(*mMessage, offset, numRecords, aIndex, name, aaaaRecord));
    aAddress = aaaaRecord.GetAddress();
    aTtl     = aaaaRecord.GetTtl();

exit:
    return error;
}

#if OPENTHREAD_CONFIG_DNS_CLIENT_NAT64_ENABLE

Error Client::Response::FindARecord(Section aSection, const Name &aHostName, uint16_t aIndex, ARecord &aARecord) const
{
    Error    error;
    uint16_t offset;
    uint16_t numRecords;
    Name     name = aHostName;

    SuccessOrExit(error = CheckForHostNameAlias(aSection, name));

    SelectSection(aSection, offset, numRecords);
    error = ResourceRecord::FindRecord(*mMessage, offset, numRecords, aIndex, name, aARecord);

exit:
    return error;
}

#endif // OPENTHREAD_CONFIG_DNS_CLIENT_NAT64_ENABLE

#if OPENTHREAD_CONFIG_DNS_CLIENT_SERVICE_DISCOVERY_ENABLE

void Client::Response::InitServiceInfo(ServiceInfo &aServiceInfo) const
{
    // This method initializes `aServiceInfo` setting all
    // TTLs to zero and host name to empty string.

    aServiceInfo.mTtl              = 0;
    aServiceInfo.mHostAddressTtl   = 0;
    aServiceInfo.mTxtDataTtl       = 0;
    aServiceInfo.mTxtDataTruncated = false;

    AsCoreType(&aServiceInfo.mHostAddress).Clear();

    if ((aServiceInfo.mHostNameBuffer != nullptr) && (aServiceInfo.mHostNameBufferSize > 0))
    {
        aServiceInfo.mHostNameBuffer[0] = '\0';
    }
}

Error Client::Response::ReadServiceInfo(Section aSection, const Name &aName, ServiceInfo &aServiceInfo) const
{
    // This method searches for SRV record in the given `aSection`
    // matching the record name against `aName`, and updates the
    // `aServiceInfo` accordingly. It also searches for AAAA record
    // for host name associated with the service (from SRV record).
    // The search for AAAA record is always performed in Additional
    // Data section (independent of the value given in `aSection`).

    Error     error = kErrorNone;
    uint16_t  offset;
    uint16_t  numRecords;
    Name      hostName;
    SrvRecord srvRecord;

    // A non-zero `mTtl` indicates that SRV record is already found
    // and parsed from a previous response.
    VerifyOrExit(aServiceInfo.mTtl == 0);

    VerifyOrExit(mMessage != nullptr, error = kErrorNotFound);

    // Search for a matching SRV record
    SelectSection(aSection, offset, numRecords);
    SuccessOrExit(error = ResourceRecord::FindRecord(*mMessage, offset, numRecords, /* aIndex */ 0, aName, srvRecord));

    aServiceInfo.mTtl      = srvRecord.GetTtl();
    aServiceInfo.mPort     = srvRecord.GetPort();
    aServiceInfo.mPriority = srvRecord.GetPriority();
    aServiceInfo.mWeight   = srvRecord.GetWeight();

    hostName.SetFromMessage(*mMessage, offset);

    if (aServiceInfo.mHostNameBuffer != nullptr)
    {
        SuccessOrExit(error = srvRecord.ReadTargetHostName(*mMessage, offset, aServiceInfo.mHostNameBuffer,
                                                           aServiceInfo.mHostNameBufferSize));
    }
    else
    {
        SuccessOrExit(error = Name::ParseName(*mMessage, offset));
    }

    // Search in additional section for AAAA record for the host name.

    VerifyOrExit(AsCoreType(&aServiceInfo.mHostAddress).IsUnspecified());

    error = FindHostAddress(kAdditionalDataSection, hostName, /* aIndex */ 0, AsCoreType(&aServiceInfo.mHostAddress),
                            aServiceInfo.mHostAddressTtl);

    if (error == kErrorNotFound)
    {
        error = kErrorNone;
    }

exit:
    return error;
}

Error Client::Response::ReadTxtRecord(Section aSection, const Name &aName, ServiceInfo &aServiceInfo) const
{
    // This method searches a TXT record in the given `aSection`
    // matching the record name against `aName` and updates the TXT
    // related properties in `aServicesInfo`.
    //
    // If no match is found `mTxtDataTtl` (which is initialized to zero)
    // remains unchanged to indicate this. In this case this method still
    // returns `kErrorNone`.

    Error     error = kErrorNone;
    uint16_t  offset;
    uint16_t  numRecords;
    TxtRecord txtRecord;

    // A non-zero `mTxtDataTtl` indicates that TXT record is already
    // found and parsed from a previous response.
    VerifyOrExit(aServiceInfo.mTxtDataTtl == 0);

    // A null `mTxtData` indicates that caller does not want to retrieve
    // TXT data.
    VerifyOrExit(aServiceInfo.mTxtData != nullptr);

    VerifyOrExit(mMessage != nullptr, error = kErrorNotFound);

    SelectSection(aSection, offset, numRecords);

    aServiceInfo.mTxtDataTruncated = false;

    SuccessOrExit(error = ResourceRecord::FindRecord(*mMessage, offset, numRecords, /* aIndex */ 0, aName, txtRecord));

    error = txtRecord.ReadTxtData(*mMessage, offset, aServiceInfo.mTxtData, aServiceInfo.mTxtDataSize);

    if (error == kErrorNoBufs)
    {
        error = kErrorNone;

        // Mark `mTxtDataTruncated` to indicate that we could not read
        // the full TXT record into the given `mTxtData` buffer.
        aServiceInfo.mTxtDataTruncated = true;
    }

    SuccessOrExit(error);
    aServiceInfo.mTxtDataTtl = txtRecord.GetTtl();

exit:
    if (error == kErrorNotFound)
    {
        error = kErrorNone;
    }

    return error;
}

#endif // OPENTHREAD_CONFIG_DNS_CLIENT_SERVICE_DISCOVERY_ENABLE

void Client::Response::PopulateFrom(const Message &aMessage)
{
    // Populate `Response` with info from `aMessage`.

    uint16_t offset = aMessage.GetOffset();
    Header   header;

    mMessage = &aMessage;

    IgnoreError(aMessage.Read(offset, header));
    offset += sizeof(Header);

    for (uint16_t num = 0; num < header.GetQuestionCount(); num++)
    {
        IgnoreError(Name::ParseName(aMessage, offset));
        offset += sizeof(Question);
    }

    mAnswerOffset = offset;
    IgnoreError(ResourceRecord::ParseRecords(aMessage, offset, header.GetAnswerCount()));
    IgnoreError(ResourceRecord::ParseRecords(aMessage, offset, header.GetAuthorityRecordCount()));
    mAdditionalOffset = offset;
    IgnoreError(ResourceRecord::ParseRecords(aMessage, offset, header.GetAdditionalRecordCount()));

    mAnswerRecordCount     = header.GetAnswerCount();
    mAdditionalRecordCount = header.GetAdditionalRecordCount();
}

//---------------------------------------------------------------------------------------------------------------------
// Client::AddressResponse

Error Client::AddressResponse::GetAddress(uint16_t aIndex, Ip6::Address &aAddress, uint32_t &aTtl) const
{
    Error error = kErrorNone;
    Name  name(*mQuery, kNameOffsetInQuery);

#if OPENTHREAD_CONFIG_DNS_CLIENT_NAT64_ENABLE

    // If the response is for an IPv4 address query or if it is an
    // IPv6 address query response with no IPv6 address but with
    // an IPv4 in its additional section, we read the IPv4 address
    // and translate it to an IPv6 address.

    QueryInfo info;

    info.ReadFrom(*mQuery);

    if ((info.mQueryType == kIp4AddressQuery) || mIp6QueryResponseRequiresNat64)
    {
        Section                          section;
        ARecord                          aRecord;
        NetworkData::ExternalRouteConfig nat64Prefix;

        VerifyOrExit(mInstance->Get<NetworkData::Leader>().GetPreferredNat64Prefix(nat64Prefix) == kErrorNone,
                     error = kErrorInvalidState);

        section = (info.mQueryType == kIp4AddressQuery) ? kAnswerSection : kAdditionalDataSection;
        SuccessOrExit(error = FindARecord(section, name, aIndex, aRecord));

        aAddress.SynthesizeFromIp4Address(nat64Prefix.GetPrefix(), aRecord.GetAddress());
        aTtl = aRecord.GetTtl();

        ExitNow();
    }

#endif // OPENTHREAD_CONFIG_DNS_CLIENT_NAT64_ENABLE

    ExitNow(error = FindHostAddress(kAnswerSection, name, aIndex, aAddress, aTtl));

exit:
    return error;
}

#if OPENTHREAD_CONFIG_DNS_CLIENT_SERVICE_DISCOVERY_ENABLE

//---------------------------------------------------------------------------------------------------------------------
// Client::BrowseResponse

Error Client::BrowseResponse::GetServiceInstance(uint16_t aIndex, char *aLabelBuffer, uint8_t aLabelBufferSize) const
{
    Error     error;
    uint16_t  offset;
    uint16_t  numRecords;
    Name      serviceName(*mQuery, kNameOffsetInQuery);
    PtrRecord ptrRecord;

    VerifyOrExit(mMessage != nullptr, error = kErrorNotFound);

    SelectSection(kAnswerSection, offset, numRecords);
    SuccessOrExit(error = ResourceRecord::FindRecord(*mMessage, offset, numRecords, aIndex, serviceName, ptrRecord));
    error = ptrRecord.ReadPtrName(*mMessage, offset, aLabelBuffer, aLabelBufferSize, nullptr, 0);

exit:
    return error;
}

Error Client::BrowseResponse::GetServiceInfo(const char *aInstanceLabel, ServiceInfo &aServiceInfo) const
{
    Error error;
    Name  instanceName;

    // Find a matching PTR record for the service instance label. Then
    // search and read SRV, TXT and AAAA records in Additional Data
    // section matching the same name to populate `aServiceInfo`.

    SuccessOrExit(error = FindPtrRecord(aInstanceLabel, instanceName));

    InitServiceInfo(aServiceInfo);
    SuccessOrExit(error = ReadServiceInfo(kAdditionalDataSection, instanceName, aServiceInfo));
    SuccessOrExit(error = ReadTxtRecord(kAdditionalDataSection, instanceName, aServiceInfo));

    if (aServiceInfo.mTxtDataTtl == 0)
    {
        aServiceInfo.mTxtDataSize = 0;
    }

exit:
    return error;
}

Error Client::BrowseResponse::GetHostAddress(const char   *aHostName,
                                             uint16_t      aIndex,
                                             Ip6::Address &aAddress,
                                             uint32_t     &aTtl) const
{
    return FindHostAddress(kAdditionalDataSection, Name(aHostName), aIndex, aAddress, aTtl);
}

Error Client::BrowseResponse::FindPtrRecord(const char *aInstanceLabel, Name &aInstanceName) const
{
    // This method searches within the Answer Section for a PTR record
    // matching a given instance label @aInstanceLabel. If found, the
    // `aName` is updated to return the name in the message.

    Error     error;
    uint16_t  offset;
    Name      serviceName(*mQuery, kNameOffsetInQuery);
    uint16_t  numRecords;
    uint16_t  labelOffset;
    PtrRecord ptrRecord;

    VerifyOrExit(mMessage != nullptr, error = kErrorNotFound);

    SelectSection(kAnswerSection, offset, numRecords);

    for (; numRecords > 0; numRecords--)
    {
        SuccessOrExit(error = Name::CompareName(*mMessage, offset, serviceName));

        error = ResourceRecord::ReadRecord(*mMessage, offset, ptrRecord);

        if (error == kErrorNotFound)
        {
            // `ReadRecord()` updates `offset` to skip over a
            // non-matching record.
            continue;
        }

        SuccessOrExit(error);

        // It is a PTR record. Check the first label to match the
        // instance label.

        labelOffset = offset;
        error       = Name::CompareLabel(*mMessage, labelOffset, aInstanceLabel);

        if (error == kErrorNone)
        {
            aInstanceName.SetFromMessage(*mMessage, offset);
            ExitNow();
        }

        VerifyOrExit(error == kErrorNotFound);

        // Update offset to skip over the PTR record.
        offset += static_cast<uint16_t>(ptrRecord.GetSize()) - sizeof(ptrRecord);
    }

    error = kErrorNotFound;

exit:
    return error;
}

//---------------------------------------------------------------------------------------------------------------------
// Client::ServiceResponse

Error Client::ServiceResponse::GetServiceName(char    *aLabelBuffer,
                                              uint8_t  aLabelBufferSize,
                                              char    *aNameBuffer,
                                              uint16_t aNameBufferSize) const
{
    Error    error;
    uint16_t offset = kNameOffsetInQuery;

    SuccessOrExit(error = Name::ReadLabel(*mQuery, offset, aLabelBuffer, aLabelBufferSize));

    VerifyOrExit(aNameBuffer != nullptr);
    SuccessOrExit(error = Name::ReadName(*mQuery, offset, aNameBuffer, aNameBufferSize));

exit:
    return error;
}

Error Client::ServiceResponse::GetServiceInfo(ServiceInfo &aServiceInfo) const
{
    // Search and read SRV, TXT records matching name from query.

    Error error = kErrorNotFound;

    InitServiceInfo(aServiceInfo);

    for (const Response *response = this; response != nullptr; response = response->mNext)
    {
        Name      name(*response->mQuery, kNameOffsetInQuery);
        QueryInfo info;
        Section   srvSection;
        Section   txtSection;

        info.ReadFrom(*response->mQuery);

        switch (info.mQueryType)
        {
        case kIp6AddressQuery:
#if OPENTHREAD_CONFIG_DNS_CLIENT_NAT64_ENABLE
        case kIp4AddressQuery:
#endif
            IgnoreError(response->FindHostAddress(kAnswerSection, name, /* aIndex */ 0,
                                                  AsCoreType(&aServiceInfo.mHostAddress),
                                                  aServiceInfo.mHostAddressTtl));

            continue; // to `for()` loop

        case kServiceQuerySrvTxt:
        case kServiceQuerySrv:
        case kServiceQueryTxt:
            break;

        default:
            continue;
        }

        // Determine from which section we should try to read the SRV and
        // TXT records based on the query type.
        //
        // In `kServiceQuerySrv` or `kServiceQueryTxt` we expect to see
        // only one record (SRV or TXT) in the answer section, but we
        // still try to read the other records from additional data
        // section in case server provided them.

        srvSection = (info.mQueryType != kServiceQueryTxt) ? kAnswerSection : kAdditionalDataSection;
        txtSection = (info.mQueryType != kServiceQuerySrv) ? kAnswerSection : kAdditionalDataSection;

        error = response->ReadServiceInfo(srvSection, name, aServiceInfo);

        if ((srvSection == kAdditionalDataSection) && (error == kErrorNotFound))
        {
            error = kErrorNone;
        }

        SuccessOrExit(error);

        SuccessOrExit(error = response->ReadTxtRecord(txtSection, name, aServiceInfo));
    }

    if (aServiceInfo.mTxtDataTtl == 0)
    {
        aServiceInfo.mTxtDataSize = 0;
    }

exit:
    return error;
}

Error Client::ServiceResponse::GetHostAddress(const char   *aHostName,
                                              uint16_t      aIndex,
                                              Ip6::Address &aAddress,
                                              uint32_t     &aTtl) const
{
    Error error = kErrorNotFound;

    for (const Response *response = this; response != nullptr; response = response->mNext)
    {
        Section   section = kAdditionalDataSection;
        QueryInfo info;

        info.ReadFrom(*response->mQuery);

        switch (info.mQueryType)
        {
        case kIp6AddressQuery:
#if OPENTHREAD_CONFIG_DNS_CLIENT_NAT64_ENABLE
        case kIp4AddressQuery:
#endif
            section = kAnswerSection;
            break;

        default:
            break;
        }

        error = response->FindHostAddress(section, Name(aHostName), aIndex, aAddress, aTtl);

        if (error == kErrorNone)
        {
            break;
        }
    }

    return error;
}

#endif // OPENTHREAD_CONFIG_DNS_CLIENT_SERVICE_DISCOVERY_ENABLE

//---------------------------------------------------------------------------------------------------------------------
// Client

const uint16_t Client::kIp6AddressQueryRecordTypes[] = {ResourceRecord::kTypeAaaa};
#if OPENTHREAD_CONFIG_DNS_CLIENT_NAT64_ENABLE
const uint16_t Client::kIp4AddressQueryRecordTypes[] = {ResourceRecord::kTypeA};
#endif
#if OPENTHREAD_CONFIG_DNS_CLIENT_SERVICE_DISCOVERY_ENABLE
const uint16_t Client::kBrowseQueryRecordTypes[]  = {ResourceRecord::kTypePtr};
const uint16_t Client::kServiceQueryRecordTypes[] = {ResourceRecord::kTypeSrv, ResourceRecord::kTypeTxt};
#endif

const uint8_t Client::kQuestionCount[] = {
    /* kIp6AddressQuery -> */ GetArrayLength(kIp6AddressQueryRecordTypes), // AAAA record
#if OPENTHREAD_CONFIG_DNS_CLIENT_NAT64_ENABLE
    /* kIp4AddressQuery -> */ GetArrayLength(kIp4AddressQueryRecordTypes), // A record
#endif
#if OPENTHREAD_CONFIG_DNS_CLIENT_SERVICE_DISCOVERY_ENABLE
    /* kBrowseQuery        -> */ GetArrayLength(kBrowseQueryRecordTypes),  // PTR record
    /* kServiceQuerySrvTxt -> */ GetArrayLength(kServiceQueryRecordTypes), // SRV and TXT records
    /* kServiceQuerySrv    -> */ 1,                                        // SRV record only
    /* kServiceQueryTxt    -> */ 1,                                        // TXT record only
#endif
};

const uint16_t *const Client::kQuestionRecordTypes[] = {
    /* kIp6AddressQuery -> */ kIp6AddressQueryRecordTypes,
#if OPENTHREAD_CONFIG_DNS_CLIENT_NAT64_ENABLE
    /* kIp4AddressQuery -> */ kIp4AddressQueryRecordTypes,
#endif
#if OPENTHREAD_CONFIG_DNS_CLIENT_SERVICE_DISCOVERY_ENABLE
    /* kBrowseQuery  -> */ kBrowseQueryRecordTypes,
    /* kServiceQuerySrvTxt -> */ kServiceQueryRecordTypes,
    /* kServiceQuerySrv    -> */ &kServiceQueryRecordTypes[0],
    /* kServiceQueryTxt    -> */ &kServiceQueryRecordTypes[1],

#endif
};

Client::Client(Instance &aInstance)
    : InstanceLocator(aInstance)
    , mSocket(aInstance, *this)
#if OPENTHREAD_CONFIG_DNS_CLIENT_OVER_TCP_ENABLE
    , mTcpState(kTcpUninitialized)
#endif
    , mTimer(aInstance)
    , mDefaultConfig(QueryConfig::kInitFromDefaults)
#if OPENTHREAD_CONFIG_DNS_CLIENT_DEFAULT_SERVER_ADDRESS_AUTO_SET_ENABLE
    , mUserDidSetDefaultAddress(false)
#endif
{
    struct QueryTypeChecker
    {
        InitEnumValidatorCounter();

        ValidateNextEnum(kIp6AddressQuery);
#if OPENTHREAD_CONFIG_DNS_CLIENT_NAT64_ENABLE
        ValidateNextEnum(kIp4AddressQuery);
#endif
#if OPENTHREAD_CONFIG_DNS_CLIENT_SERVICE_DISCOVERY_ENABLE
        ValidateNextEnum(kBrowseQuery);
        ValidateNextEnum(kServiceQuerySrvTxt);
        ValidateNextEnum(kServiceQuerySrv);
        ValidateNextEnum(kServiceQueryTxt);
#endif
    };

#if OPENTHREAD_CONFIG_DNS_CLIENT_OVER_TCP_ENABLE
    ClearAllBytes(mSendLink);
#endif
}

Error Client::Start(void)
{
    Error error;

    SuccessOrExit(error = mSocket.Open());
    SuccessOrExit(error = mSocket.Bind(0, Ip6::kNetifUnspecified));

exit:
    return error;
}

void Client::Stop(void)
{
    Query *query;

    while ((query = mMainQueries.GetHead()) != nullptr)
    {
        FinalizeQuery(*query, kErrorAbort);
    }

    IgnoreError(mSocket.Close());
#if OPENTHREAD_CONFIG_DNS_CLIENT_OVER_TCP_ENABLE
    if (mTcpState != kTcpUninitialized)
    {
        IgnoreError(mEndpoint.Deinitialize());
    }
#endif

    mLimitedQueryServers.Clear();
}

#if OPENTHREAD_CONFIG_DNS_CLIENT_OVER_TCP_ENABLE
Error Client::InitTcpSocket(void)
{
    Error                       error;
    otTcpEndpointInitializeArgs endpointArgs;

    ClearAllBytes(endpointArgs);
    endpointArgs.mSendDoneCallback         = HandleTcpSendDoneCallback;
    endpointArgs.mEstablishedCallback      = HandleTcpEstablishedCallback;
    endpointArgs.mReceiveAvailableCallback = HandleTcpReceiveAvailableCallback;
    endpointArgs.mDisconnectedCallback     = HandleTcpDisconnectedCallback;
    endpointArgs.mContext                  = this;
    endpointArgs.mReceiveBuffer            = mReceiveBufferBytes;
    endpointArgs.mReceiveBufferSize        = sizeof(mReceiveBufferBytes);

    mSendLink.mNext   = nullptr;
    mSendLink.mData   = mSendBufferBytes;
    mSendLink.mLength = 0;

    SuccessOrExit(error = mEndpoint.Initialize(Get<Instance>(), endpointArgs));
exit:
    return error;
}
#endif

void Client::SetDefaultConfig(const QueryConfig &aQueryConfig)
{
    QueryConfig startingDefault(QueryConfig::kInitFromDefaults);

    mDefaultConfig.SetFrom(&aQueryConfig, startingDefault);

#if OPENTHREAD_CONFIG_DNS_CLIENT_DEFAULT_SERVER_ADDRESS_AUTO_SET_ENABLE
    mUserDidSetDefaultAddress = !aQueryConfig.GetServerSockAddr().GetAddress().IsUnspecified();
    UpdateDefaultConfigAddress();
#endif
}

void Client::ResetDefaultConfig(void)
{
    mDefaultConfig = QueryConfig(QueryConfig::kInitFromDefaults);

#if OPENTHREAD_CONFIG_DNS_CLIENT_DEFAULT_SERVER_ADDRESS_AUTO_SET_ENABLE
    mUserDidSetDefaultAddress = false;
    UpdateDefaultConfigAddress();
#endif
}

#if OPENTHREAD_CONFIG_DNS_CLIENT_DEFAULT_SERVER_ADDRESS_AUTO_SET_ENABLE
void Client::UpdateDefaultConfigAddress(void)
{
    const Ip6::Address &srpServerAddr = Get<Srp::Client>().GetServerAddress().GetAddress();

    if (!mUserDidSetDefaultAddress && Get<Srp::Client>().IsServerSelectedByAutoStart() &&
        !srpServerAddr.IsUnspecified())
    {
        mDefaultConfig.GetServerSockAddr().SetAddress(srpServerAddr);
    }
}
#endif

Error Client::ResolveAddress(const char        *aHostName,
                             AddressCallback    aCallback,
                             void              *aContext,
                             const QueryConfig *aConfig)
{
    QueryInfo info;

    info.Clear();
    info.mQueryType = kIp6AddressQuery;
    info.mConfig.SetFrom(aConfig, mDefaultConfig);
    info.mCallback.mAddressCallback = aCallback;
    info.mCallbackContext           = aContext;

    return StartQuery(info, nullptr, aHostName);
}

#if OPENTHREAD_CONFIG_DNS_CLIENT_NAT64_ENABLE
Error Client::ResolveIp4Address(const char        *aHostName,
                                AddressCallback    aCallback,
                                void              *aContext,
                                const QueryConfig *aConfig)
{
    QueryInfo info;

    info.Clear();
    info.mQueryType = kIp4AddressQuery;
    info.mConfig.SetFrom(aConfig, mDefaultConfig);
    info.mCallback.mAddressCallback = aCallback;
    info.mCallbackContext           = aContext;

    return StartQuery(info, nullptr, aHostName);
}
#endif

#if OPENTHREAD_CONFIG_DNS_CLIENT_SERVICE_DISCOVERY_ENABLE

Error Client::Browse(const char *aServiceName, BrowseCallback aCallback, void *aContext, const QueryConfig *aConfig)
{
    QueryInfo info;

    info.Clear();
    info.mQueryType = kBrowseQuery;
    info.mConfig.SetFrom(aConfig, mDefaultConfig);
    info.mCallback.mBrowseCallback = aCallback;
    info.mCallbackContext          = aContext;

    return StartQuery(info, nullptr, aServiceName);
}

Error Client::ResolveService(const char        *aInstanceLabel,
                             const char        *aServiceName,
                             ServiceCallback    aCallback,
                             void              *aContext,
                             const QueryConfig *aConfig)
{
    return Resolve(aInstanceLabel, aServiceName, aCallback, aContext, aConfig, false);
}

Error Client::ResolveServiceAndHostAddress(const char        *aInstanceLabel,
                                           const char        *aServiceName,
                                           ServiceCallback    aCallback,
                                           void              *aContext,
                                           const QueryConfig *aConfig)
{
    return Resolve(aInstanceLabel, aServiceName, aCallback, aContext, aConfig, true);
}

Error Client::Resolve(const char        *aInstanceLabel,
                      const char        *aServiceName,
                      ServiceCallback    aCallback,
                      void              *aContext,
                      const QueryConfig *aConfig,
                      bool               aShouldResolveHostAddr)
{
    QueryInfo info;
    Error     error;
    QueryType secondQueryType = kNoQuery;

    VerifyOrExit(aInstanceLabel != nullptr, error = kErrorInvalidArgs);

    info.Clear();

    info.mConfig.SetFrom(aConfig, mDefaultConfig);
    info.mShouldResolveHostAddr = aShouldResolveHostAddr;

    CheckAndUpdateServiceMode(info.mConfig, aConfig);

    switch (info.mConfig.GetServiceMode())
    {
    case QueryConfig::kServiceModeSrvTxtSeparate:
        secondQueryType = kServiceQueryTxt;

        OT_FALL_THROUGH;

    case QueryConfig::kServiceModeSrv:
        info.mQueryType = kServiceQuerySrv;
        break;

    case QueryConfig::kServiceModeTxt:
        info.mQueryType = kServiceQueryTxt;
        VerifyOrExit(!info.mShouldResolveHostAddr, error = kErrorInvalidArgs);
        break;

    case QueryConfig::kServiceModeSrvTxt:
    case QueryConfig::kServiceModeSrvTxtOptimize:
    default:
        info.mQueryType = kServiceQuerySrvTxt;
        break;
    }

    info.mCallback.mServiceCallback = aCallback;
    info.mCallbackContext           = aContext;

    error = StartQuery(info, aInstanceLabel, aServiceName, secondQueryType);

exit:
    return error;
}

#endif // OPENTHREAD_CONFIG_DNS_CLIENT_SERVICE_DISCOVERY_ENABLE

Error Client::StartQuery(QueryInfo &aInfo, const char *aLabel, const char *aName, QueryType aSecondType)
{
    // The `aLabel` can be `nullptr` and then `aName` provides the
    // full name, otherwise the name is appended as `{aLabel}.
    // {aName}`.

    Error  error;
    Query *query;

    VerifyOrExit(mSocket.IsBound(), error = kErrorInvalidState);

#if OPENTHREAD_CONFIG_DNS_CLIENT_NAT64_ENABLE
    if (aInfo.mQueryType == kIp4AddressQuery)
    {
        NetworkData::ExternalRouteConfig nat64Prefix;

        VerifyOrExit(aInfo.mConfig.GetNat64Mode() == QueryConfig::kNat64Allow, error = kErrorInvalidArgs);
        VerifyOrExit(Get<NetworkData::Leader>().GetPreferredNat64Prefix(nat64Prefix) == kErrorNone,
                     error = kErrorInvalidState);
    }
#endif

    SuccessOrExit(error = AllocateQuery(aInfo, aLabel, aName, query));

    mMainQueries.Enqueue(*query);

    error = SendQuery(*query, aInfo, /* aUpdateTimer */ true);
    VerifyOrExit(error == kErrorNone, FreeQuery(*query));

    if (aSecondType != kNoQuery)
    {
        Query *secondQuery;

        aInfo.mQueryType         = aSecondType;
        aInfo.mMessageId         = 0;
        aInfo.mTransmissionCount = 0;
        aInfo.mMainQuery         = query;

        // We intentionally do not use `error` here so in the unlikely
        // case where we cannot allocate the second query we can proceed
        // with the first one.
        SuccessOrExit(AllocateQuery(aInfo, aLabel, aName, secondQuery));

        IgnoreError(SendQuery(*secondQuery, aInfo, /* aUpdateTimer */ true));

        // Update first query to link to second one by updating
        // its `mNextQuery`.
        aInfo.ReadFrom(*query);
        aInfo.mNextQuery = secondQuery;
        UpdateQuery(*query, aInfo);
    }

exit:
    return error;
}

Error Client::AllocateQuery(const QueryInfo &aInfo, const char *aLabel, const char *aName, Query *&aQuery)
{
    Error error = kErrorNone;

    aQuery = nullptr;

    VerifyOrExit(aInfo.mConfig.GetResponseTimeout() <= TimerMilli::kMaxDelay, error = kErrorInvalidArgs);

    aQuery = Get<MessagePool>().Allocate(Message::kTypeOther);
    VerifyOrExit(aQuery != nullptr, error = kErrorNoBufs);

    SuccessOrExit(error = aQuery->Append(aInfo));

    if (aLabel != nullptr)
    {
        SuccessOrExit(error = Name::AppendLabel(aLabel, *aQuery));
    }

    SuccessOrExit(error = Name::AppendName(aName, *aQuery));

exit:
    FreeAndNullMessageOnError(aQuery, error);
    return error;
}

Client::Query &Client::FindMainQuery(Query &aQuery)
{
    QueryInfo info;

    info.ReadFrom(aQuery);

    return (info.mMainQuery == nullptr) ? aQuery : *info.mMainQuery;
}

void Client::FreeQuery(Query &aQuery)
{
    Query    &mainQuery = FindMainQuery(aQuery);
    QueryInfo info;

    mMainQueries.Dequeue(mainQuery);

    for (Query *query = &mainQuery; query != nullptr; query = info.mNextQuery)
    {
        info.ReadFrom(*query);
        FreeMessage(info.mSavedResponse);
        query->Free();
    }
}

Error Client::SendQuery(Query &aQuery, QueryInfo &aInfo, bool aUpdateTimer)
{
    // This method prepares and sends a query message represented by
    // `aQuery` and `aInfo`. This method updates `aInfo` (e.g., sets
    // the new `mRetransmissionTime`) and updates it in `aQuery` as
    // well. `aUpdateTimer` indicates whether the timer should be
    // updated when query is sent or not (used in the case where timer
    // is handled by caller).

    Error            error   = kErrorNone;
    Message         *message = nullptr;
    Header           header;
    Ip6::MessageInfo messageInfo;
    uint16_t         length = 0;

    aInfo.mTransmissionCount++;
    aInfo.mRetransmissionTime = TimerMilli::GetNow() + aInfo.mConfig.GetResponseTimeout();

    if (aInfo.mMessageId == 0)
    {
        do
        {
            SuccessOrExit(error = header.SetRandomMessageId());
        } while ((header.GetMessageId() == 0) || (FindQueryById(header.GetMessageId()) != nullptr));

        aInfo.mMessageId = header.GetMessageId();
    }
    else
    {
        header.SetMessageId(aInfo.mMessageId);
    }

    header.SetType(Header::kTypeQuery);
    header.SetQueryType(Header::kQueryTypeStandard);

    if (aInfo.mConfig.GetRecursionFlag() == QueryConfig::kFlagRecursionDesired)
    {
        header.SetRecursionDesiredFlag();
    }

    header.SetQuestionCount(kQuestionCount[aInfo.mQueryType]);

    message = mSocket.NewMessage();
    VerifyOrExit(message != nullptr, error = kErrorNoBufs);

    SuccessOrExit(error = message->Append(header));

    // Prepare the question section.

    for (uint8_t num = 0; num < kQuestionCount[aInfo.mQueryType]; num++)
    {
        SuccessOrExit(error = AppendNameFromQuery(aQuery, *message));
        SuccessOrExit(error = message->Append(Question(kQuestionRecordTypes[aInfo.mQueryType][num])));
    }

    length = message->GetLength() - message->GetOffset();

    if (aInfo.mConfig.GetTransportProto() == QueryConfig::kDnsTransportTcp)
#if OPENTHREAD_CONFIG_DNS_CLIENT_OVER_TCP_ENABLE
    {
        // Check if query will fit into tcp buffer if not return error.
        VerifyOrExit(length + sizeof(uint16_t) + mSendLink.mLength <=
                         OPENTHREAD_CONFIG_DNS_CLIENT_OVER_TCP_QUERY_MAX_SIZE,
                     error = kErrorNoBufs);

        // In case of initialized connection check if connected peer and new query have the same address.
        if (mTcpState != kTcpUninitialized)
        {
            VerifyOrExit(mEndpoint.GetPeerAddress() == AsCoreType(&aInfo.mConfig.mServerSockAddr),
                         error = kErrorFailed);
        }

        switch (mTcpState)
        {
        case kTcpUninitialized:
            SuccessOrExit(error = InitTcpSocket());
            SuccessOrExit(
                error = mEndpoint.Connect(AsCoreType(&aInfo.mConfig.mServerSockAddr), OT_TCP_CONNECT_NO_FAST_OPEN));
            mTcpState = kTcpConnecting;
            PrepareTcpMessage(*message);
            break;
        case kTcpConnectedIdle:
            PrepareTcpMessage(*message);
            SuccessOrExit(error = mEndpoint.SendByReference(mSendLink, /* aFlags */ 0));
            mTcpState = kTcpConnectedSending;
            break;
        case kTcpConnecting:
            PrepareTcpMessage(*message);
            break;
        case kTcpConnectedSending:
            BigEndian::WriteUint16(length, mSendBufferBytes + mSendLink.mLength);
            SuccessOrAssert(error = message->Read(message->GetOffset(),
                                                  (mSendBufferBytes + sizeof(uint16_t) + mSendLink.mLength), length));
            IgnoreError(mEndpoint.SendByExtension(length + sizeof(uint16_t), /* aFlags */ 0));
            break;
        }
        message->Free();
        message = nullptr;
    }
#else
    {
        error = kErrorInvalidArgs;
        LogWarn("DNS query over TCP not supported.");
        ExitNow();
    }
#endif
    else
    {
        VerifyOrExit(length <= kUdpQueryMaxSize, error = kErrorInvalidArgs);
        messageInfo.SetPeerAddr(aInfo.mConfig.GetServerSockAddr().GetAddress());
        messageInfo.SetPeerPort(aInfo.mConfig.GetServerSockAddr().GetPort());
        SuccessOrExit(error = mSocket.SendTo(*message, messageInfo));
    }

exit:

    FreeMessageOnError(message, error);
    if (aUpdateTimer)
    {
        mTimer.FireAtIfEarlier(aInfo.mRetransmissionTime);
    }

    UpdateQuery(aQuery, aInfo);

    return error;
}

Error Client::AppendNameFromQuery(const Query &aQuery, Message &aMessage)
{
    // The name is encoded and included after the `Info` in `aQuery`
    // starting at `kNameOffsetInQuery`.

    return aMessage.AppendBytesFromMessage(aQuery, kNameOffsetInQuery, aQuery.GetLength() - kNameOffsetInQuery);
}

void Client::FinalizeQuery(Query &aQuery, Error aError)
{
    Response response;
    Query   &mainQuery = FindMainQuery(aQuery);

    response.mInstance = &Get<Instance>();
    response.mQuery    = &mainQuery;

    FinalizeQuery(response, aError);
}

void Client::FinalizeQuery(Response &aResponse, Error aError)
{
    QueryType type;
    Callback  callback;
    void     *context;

    GetQueryTypeAndCallback(*aResponse.mQuery, type, callback, context);

    switch (type)
    {
    case kIp6AddressQuery:
#if OPENTHREAD_CONFIG_DNS_CLIENT_NAT64_ENABLE
    case kIp4AddressQuery:
#endif
        if (callback.mAddressCallback != nullptr)
        {
            callback.mAddressCallback(aError, &aResponse, context);
        }
        break;

#if OPENTHREAD_CONFIG_DNS_CLIENT_SERVICE_DISCOVERY_ENABLE
    case kBrowseQuery:
        if (callback.mBrowseCallback != nullptr)
        {
            callback.mBrowseCallback(aError, &aResponse, context);
        }
        break;

    case kServiceQuerySrvTxt:
    case kServiceQuerySrv:
    case kServiceQueryTxt:
        if (callback.mServiceCallback != nullptr)
        {
            callback.mServiceCallback(aError, &aResponse, context);
        }
        break;
#endif
    case kNoQuery:
        break;
    }

    FreeQuery(*aResponse.mQuery);
}

void Client::GetQueryTypeAndCallback(const Query &aQuery, QueryType &aType, Callback &aCallback, void *&aContext)
{
    QueryInfo info;

    info.ReadFrom(aQuery);

    aType     = info.mQueryType;
    aCallback = info.mCallback;
    aContext  = info.mCallbackContext;
}

Client::Query *Client::FindQueryById(uint16_t aMessageId)
{
    Query    *matchedQuery = nullptr;
    QueryInfo info;

    for (Query &mainQuery : mMainQueries)
    {
        for (Query *query = &mainQuery; query != nullptr; query = info.mNextQuery)
        {
            info.ReadFrom(*query);

            if (info.mMessageId == aMessageId)
            {
                matchedQuery = query;
                ExitNow();
            }
        }
    }

exit:
    return matchedQuery;
}

void Client::HandleUdpReceive(Message &aMessage, const Ip6::MessageInfo &aMsgInfo)
{
    OT_UNUSED_VARIABLE(aMsgInfo);
    ProcessResponse(aMessage);
}

void Client::ProcessResponse(const Message &aResponseMessage)
{
    Error  responseError;
    Query *query;

    SuccessOrExit(ParseResponse(aResponseMessage, query, responseError));

#if OPENTHREAD_CONFIG_DNS_CLIENT_NAT64_ENABLE
    if (ReplaceWithIp4Query(*query, aResponseMessage) == kErrorNone)
    {
        ExitNow();
    }
#endif

    if (responseError != kErrorNone)
    {
        // Received an error from server, check if we can replace
        // the query.

#if OPENTHREAD_CONFIG_DNS_CLIENT_SERVICE_DISCOVERY_ENABLE
        if (ReplaceWithSeparateSrvTxtQueries(*query) == kErrorNone)
        {
            ExitNow();
        }
#endif

        FinalizeQuery(*query, responseError);
        ExitNow();
    }

    // Received successful response from server.

#if OPENTHREAD_CONFIG_DNS_CLIENT_SERVICE_DISCOVERY_ENABLE
    ResolveHostAddressIfNeeded(*query, aResponseMessage);
#endif

    if (!CanFinalizeQuery(*query))
    {
        SaveQueryResponse(*query, aResponseMessage);
        ExitNow();
    }

    PrepareResponseAndFinalize(FindMainQuery(*query), aResponseMessage, nullptr);

exit:
    return;
}

Error Client::ParseResponse(const Message &aResponseMessage, Query *&aQuery, Error &aResponseError)
{
    Error     error  = kErrorNone;
    uint16_t  offset = aResponseMessage.GetOffset();
    Header    header;
    QueryInfo info;
    Name      queryName;

    SuccessOrExit(error = aResponseMessage.Read(offset, header));
    offset += sizeof(Header);

    VerifyOrExit((header.GetType() == Header::kTypeResponse) && (header.GetQueryType() == Header::kQueryTypeStandard) &&
                     !header.IsTruncationFlagSet(),
                 error = kErrorDrop);

    aQuery = FindQueryById(header.GetMessageId());
    VerifyOrExit(aQuery != nullptr, error = kErrorNotFound);

    info.ReadFrom(*aQuery);

    queryName.SetFromMessage(*aQuery, kNameOffsetInQuery);

    // Check the Question Section

    if (header.GetQuestionCount() == kQuestionCount[info.mQueryType])
    {
        for (uint8_t num = 0; num < kQuestionCount[info.mQueryType]; num++)
        {
            SuccessOrExit(error = Name::CompareName(aResponseMessage, offset, queryName));
            offset += sizeof(Question);
        }
    }
    else
    {
        VerifyOrExit((header.GetResponseCode() != Header::kResponseSuccess) && (header.GetQuestionCount() == 0),
                     error = kErrorParse);
    }

    // Check the answer, authority and additional record sections

    SuccessOrExit(error = ResourceRecord::ParseRecords(aResponseMessage, offset, header.GetAnswerCount()));
    SuccessOrExit(error = ResourceRecord::ParseRecords(aResponseMessage, offset, header.GetAuthorityRecordCount()));
    SuccessOrExit(error = ResourceRecord::ParseRecords(aResponseMessage, offset, header.GetAdditionalRecordCount()));

    // Read the response code

    aResponseError = Header::ResponseCodeToError(header.GetResponseCode());

    if ((aResponseError == kErrorNone) && (info.mQueryType == kServiceQuerySrvTxt))
    {
        RecordServerAsCapableOfMultiQuestions(info.mConfig.GetServerSockAddr().GetAddress());
    }

exit:
    return error;
}

bool Client::CanFinalizeQuery(Query &aQuery)
{
    // Determines whether we can finalize a main query by checking if
    // we have received and saved responses for all other related
    // queries associated with `aQuery`. Note that this method is
    // called when we receive a response for `aQuery`, so no need to
    // check for a saved response for `aQuery` itself.

    bool      canFinalize = true;
    QueryInfo info;

    for (Query *query = &FindMainQuery(aQuery); query != nullptr; query = info.mNextQuery)
    {
        info.ReadFrom(*query);

        if (query == &aQuery)
        {
            continue;
        }

        if (info.mSavedResponse == nullptr)
        {
            canFinalize = false;
            ExitNow();
        }
    }

exit:
    return canFinalize;
}

void Client::SaveQueryResponse(Query &aQuery, const Message &aResponseMessage)
{
    QueryInfo info;

    info.ReadFrom(aQuery);
    VerifyOrExit(info.mSavedResponse == nullptr);

    // If `Clone()` fails we let retry or timeout handle the error.
    info.mSavedResponse = aResponseMessage.Clone();

    UpdateQuery(aQuery, info);

exit:
    return;
}

Client::Query *Client::PopulateResponse(Response &aResponse, Query &aQuery, const Message &aResponseMessage)
{
    // Populate `aResponse` for `aQuery`. If there is a saved response
    // message for `aQuery` we use it, otherwise, we use
    // `aResponseMessage`.

    QueryInfo info;

    info.ReadFrom(aQuery);

    aResponse.mInstance = &Get<Instance>();
    aResponse.mQuery    = &aQuery;
    aResponse.PopulateFrom((info.mSavedResponse == nullptr) ? aResponseMessage : *info.mSavedResponse);

    return info.mNextQuery;
}

void Client::PrepareResponseAndFinalize(Query &aQuery, const Message &aResponseMessage, Response *aPrevResponse)
{
    // This method prepares a list of chained `Response` instances
    // corresponding to all related (chained) queries. It uses
    // recursion to go through the queries and construct the
    // `Response` chain.

    Response response;
    Query   *nextQuery;

    nextQuery      = PopulateResponse(response, aQuery, aResponseMessage);
    response.mNext = aPrevResponse;

    if (nextQuery != nullptr)
    {
        PrepareResponseAndFinalize(*nextQuery, aResponseMessage, &response);
    }
    else
    {
        FinalizeQuery(response, kErrorNone);
    }
}

void Client::HandleTimer(void)
{
    NextFireTime nextTime;
    QueryInfo    info;
#if OPENTHREAD_CONFIG_DNS_CLIENT_OVER_TCP_ENABLE
    bool hasTcpQuery = false;
#endif

    for (Query &mainQuery : mMainQueries)
    {
        for (Query *query = &mainQuery; query != nullptr; query = info.mNextQuery)
        {
            info.ReadFrom(*query);

            if (info.mSavedResponse != nullptr)
            {
                continue;
            }

            if (nextTime.GetNow() >= info.mRetransmissionTime)
            {
                if (info.mTransmissionCount >= info.mConfig.GetMaxTxAttempts())
                {
                    FinalizeQuery(*query, kErrorResponseTimeout);
                    break;
                }

#if OPENTHREAD_CONFIG_DNS_CLIENT_SERVICE_DISCOVERY_ENABLE
                if (ReplaceWithSeparateSrvTxtQueries(*query) == kErrorNone)
                {
                    LogInfo("Switching to separate SRV/TXT on response timeout");
                    info.ReadFrom(*query);
                }
                else
#endif
                {
                    IgnoreError(SendQuery(*query, info, /* aUpdateTimer */ false));
                }
            }

            nextTime.UpdateIfEarlier(info.mRetransmissionTime);

#if OPENTHREAD_CONFIG_DNS_CLIENT_OVER_TCP_ENABLE
            if (info.mConfig.GetTransportProto() == QueryConfig::kDnsTransportTcp)
            {
                hasTcpQuery = true;
            }
#endif
        }
    }

    mTimer.FireAtIfEarlier(nextTime);

#if OPENTHREAD_CONFIG_DNS_CLIENT_OVER_TCP_ENABLE
    if (!hasTcpQuery && mTcpState != kTcpUninitialized)
    {
        IgnoreError(mEndpoint.SendEndOfStream());
    }
#endif
}

#if OPENTHREAD_CONFIG_DNS_CLIENT_NAT64_ENABLE

Error Client::ReplaceWithIp4Query(Query &aQuery, const Message &aResponseMessage)
{
    Error     error = kErrorFailed;
    QueryInfo info;
    Header    header;

    info.ReadFrom(aQuery);

    VerifyOrExit(info.mQueryType == kIp6AddressQuery);
    VerifyOrExit(info.mConfig.GetNat64Mode() == QueryConfig::kNat64Allow);

    // Check the response to the IPv6 query from the server. If the
    // response code is success but the answer section is empty
    // (indicating the name exists but has no IPv6 address), or the
    // response code indicates an error other than `NameError`, we
    // replace the query with an IPv4 address resolution query for
    // the same name. If the server responded with `NameError`
    // (RCode=3), it indicates that the name doesn't exist, so there
    // is no need to try an IPv4 query.

    SuccessOrExit(aResponseMessage.Read(aResponseMessage.GetOffset(), header));

    switch (header.GetResponseCode())
    {
    case Header::kResponseSuccess:
        VerifyOrExit(header.GetAnswerCount() == 0);
        OT_FALL_THROUGH;

    default:
        break;

    case Header::kResponseNameError:
        ExitNow();
    }

    // We send a new query for IPv4 address resolution
    // for the same host name. We reuse the existing `aQuery`
    // instance and keep all the info but clear `mTransmissionCount`
    // and `mMessageId` (so that a new random message ID is
    // selected). The new `info` will be saved in the query in
    // `SendQuery()`. Note that the current query is still in the
    // `mMainQueries` list when `SendQuery()` selects a new random
    // message ID, so the existing message ID for this query will
    // not be reused.

    info.mQueryType         = kIp4AddressQuery;
    info.mMessageId         = 0;
    info.mTransmissionCount = 0;

    IgnoreError(SendQuery(aQuery, info, /* aUpdateTimer */ true));
    error = kErrorNone;

exit:
    return error;
}

#endif // OPENTHREAD_CONFIG_DNS_CLIENT_NAT64_ENABLE

#if OPENTHREAD_CONFIG_DNS_CLIENT_SERVICE_DISCOVERY_ENABLE

void Client::CheckAndUpdateServiceMode(QueryConfig &aConfig, const QueryConfig *aRequestConfig) const
{
    // If the user explicitly requested "optimize" mode, we honor that
    // request. Otherwise, if "optimize" is chosen from the default
    // config, we check if the DNS server is known to have trouble
    // with multiple-question queries. If so, we switch to "separate"
    // mode.

    if ((aRequestConfig != nullptr) && (aRequestConfig->GetServiceMode() == QueryConfig::kServiceModeSrvTxtOptimize))
    {
        ExitNow();
    }

    VerifyOrExit(aConfig.GetServiceMode() == QueryConfig::kServiceModeSrvTxtOptimize);

    if (mLimitedQueryServers.Contains(aConfig.GetServerSockAddr().GetAddress()))
    {
        aConfig.SetServiceMode(QueryConfig::kServiceModeSrvTxtSeparate);
    }

exit:
    return;
}

void Client::RecordServerAsLimitedToSingleQuestion(const Ip6::Address &aServerAddress)
{
    VerifyOrExit(!aServerAddress.IsUnspecified());

    VerifyOrExit(!mLimitedQueryServers.Contains(aServerAddress));

    if (mLimitedQueryServers.IsFull())
    {
        uint8_t randomIndex = Random::NonCrypto::GetUint8InRange(0, mLimitedQueryServers.GetMaxSize());

        mLimitedQueryServers.Remove(mLimitedQueryServers[randomIndex]);
    }

    IgnoreError(mLimitedQueryServers.PushBack(aServerAddress));

exit:
    return;
}

void Client::RecordServerAsCapableOfMultiQuestions(const Ip6::Address &aServerAddress)
{
    Ip6::Address *entry = mLimitedQueryServers.Find(aServerAddress);

    VerifyOrExit(entry != nullptr);
    mLimitedQueryServers.Remove(*entry);

exit:
    return;
}

Error Client::ReplaceWithSeparateSrvTxtQueries(Query &aQuery)
{
    Error     error = kErrorFailed;
    QueryInfo info;
    Query    *secondQuery;

    info.ReadFrom(aQuery);

    VerifyOrExit(info.mQueryType == kServiceQuerySrvTxt);
    VerifyOrExit(info.mConfig.GetServiceMode() == QueryConfig::kServiceModeSrvTxtOptimize);

    RecordServerAsLimitedToSingleQuestion(info.mConfig.GetServerSockAddr().GetAddress());

    secondQuery = aQuery.Clone();
    VerifyOrExit(secondQuery != nullptr);

    info.mQueryType         = kServiceQueryTxt;
    info.mMessageId         = 0;
    info.mTransmissionCount = 0;
    info.mMainQuery         = &aQuery;
    IgnoreError(SendQuery(*secondQuery, info, /* aUpdateTimer */ true));

    info.mQueryType         = kServiceQuerySrv;
    info.mMessageId         = 0;
    info.mTransmissionCount = 0;
    info.mNextQuery         = secondQuery;
    IgnoreError(SendQuery(aQuery, info, /* aUpdateTimer */ true));
    error = kErrorNone;

exit:
    return error;
}

void Client::ResolveHostAddressIfNeeded(Query &aQuery, const Message &aResponseMessage)
{
    QueryInfo   info;
    Response    response;
    ServiceInfo serviceInfo;
    char        hostName[Name::kMaxNameSize];

    info.ReadFrom(aQuery);

    VerifyOrExit(info.mQueryType == kServiceQuerySrvTxt || info.mQueryType == kServiceQuerySrv);
    VerifyOrExit(info.mShouldResolveHostAddr);

    PopulateResponse(response, aQuery, aResponseMessage);

    ClearAllBytes(serviceInfo);
    serviceInfo.mHostNameBuffer     = hostName;
    serviceInfo.mHostNameBufferSize = sizeof(hostName);
    SuccessOrExit(response.ReadServiceInfo(Response::kAnswerSection, Name(aQuery, kNameOffsetInQuery), serviceInfo));

    // Check whether AAAA record for host address is provided in the SRV query response

    if (AsCoreType(&serviceInfo.mHostAddress).IsUnspecified())
    {
        Query *newQuery;

        info.mQueryType         = kIp6AddressQuery;
        info.mMessageId         = 0;
        info.mTransmissionCount = 0;
        info.mMainQuery         = &FindMainQuery(aQuery);

        SuccessOrExit(AllocateQuery(info, nullptr, hostName, newQuery));
        IgnoreError(SendQuery(*newQuery, info, /* aUpdateTimer */ true));

        // Update `aQuery` to be linked with new query (inserting
        // the `newQuery` into the linked-list after `aQuery`).

        info.ReadFrom(aQuery);
        info.mNextQuery = newQuery;
        UpdateQuery(aQuery, info);
    }

exit:
    return;
}

#endif // OPENTHREAD_CONFIG_DNS_CLIENT_SERVICE_DISCOVERY_ENABLE

#if OPENTHREAD_CONFIG_DNS_CLIENT_OVER_TCP_ENABLE
void Client::PrepareTcpMessage(Message &aMessage)
{
    uint16_t length = aMessage.GetLength() - aMessage.GetOffset();

    // Prepending the DNS query with length of the packet according to RFC1035.
    BigEndian::WriteUint16(length, mSendBufferBytes + mSendLink.mLength);
    SuccessOrAssert(
        aMessage.Read(aMessage.GetOffset(), (mSendBufferBytes + sizeof(uint16_t) + mSendLink.mLength), length));
    mSendLink.mLength += length + sizeof(uint16_t);
}

void Client::HandleTcpSendDone(otTcpEndpoint *aEndpoint, otLinkedBuffer *aData)
{
    OT_UNUSED_VARIABLE(aEndpoint);
    OT_UNUSED_VARIABLE(aData);
    OT_ASSERT(mTcpState == kTcpConnectedSending);

    mSendLink.mLength = 0;
    mTcpState         = kTcpConnectedIdle;
}

void Client::HandleTcpSendDoneCallback(otTcpEndpoint *aEndpoint, otLinkedBuffer *aData)
{
    static_cast<Client *>(otTcpEndpointGetContext(aEndpoint))->HandleTcpSendDone(aEndpoint, aData);
}

void Client::HandleTcpEstablished(otTcpEndpoint *aEndpoint)
{
    OT_UNUSED_VARIABLE(aEndpoint);
    IgnoreError(mEndpoint.SendByReference(mSendLink, /* aFlags */ 0));
    mTcpState = kTcpConnectedSending;
}

void Client::HandleTcpEstablishedCallback(otTcpEndpoint *aEndpoint)
{
    static_cast<Client *>(otTcpEndpointGetContext(aEndpoint))->HandleTcpEstablished(aEndpoint);
}

Error Client::ReadFromLinkBuffer(const otLinkedBuffer *&aLinkedBuffer,
                                 size_t                &aOffset,
                                 Message               &aMessage,
                                 uint16_t               aLength)
{
    // Read `aLength` bytes from `aLinkedBuffer` starting at `aOffset`
    // and copy the content into `aMessage`. As we read we can move
    // to the next `aLinkedBuffer` and update `aOffset`.
    // Returns:
    // - `kErrorNone` if `aLength` bytes are successfully read and
    //    `aOffset` and `aLinkedBuffer` are updated.
    // - `kErrorNotFound` is not enough bytes available to read
    //    from `aLinkedBuffer`.
    // - `kErrorNotBufs` if cannot grow `aMessage` to append bytes.

    Error error = kErrorNone;

    while (aLength > 0)
    {
        uint16_t bytesToRead = aLength;

        VerifyOrExit(aLinkedBuffer != nullptr, error = kErrorNotFound);

        if (bytesToRead > aLinkedBuffer->mLength - aOffset)
        {
            bytesToRead = static_cast<uint16_t>(aLinkedBuffer->mLength - aOffset);
        }

        SuccessOrExit(error = aMessage.AppendBytes(&aLinkedBuffer->mData[aOffset], bytesToRead));

        aLength -= bytesToRead;
        aOffset += bytesToRead;

        if (aOffset == aLinkedBuffer->mLength)
        {
            aLinkedBuffer = aLinkedBuffer->mNext;
            aOffset       = 0;
        }
    }

exit:
    return error;
}

void Client::HandleTcpReceiveAvailable(otTcpEndpoint *aEndpoint,
                                       size_t         aBytesAvailable,
                                       bool           aEndOfStream,
                                       size_t         aBytesRemaining)
{
    OT_UNUSED_VARIABLE(aEndpoint);
    OT_UNUSED_VARIABLE(aBytesRemaining);

    Message              *message   = nullptr;
    size_t                totalRead = 0;
    size_t                offset    = 0;
    const otLinkedBuffer *data;

    if (aEndOfStream)
    {
        // Cleanup is done in disconnected callback.
        IgnoreError(mEndpoint.SendEndOfStream());
    }

    SuccessOrExit(mEndpoint.ReceiveByReference(data));
    VerifyOrExit(data != nullptr);

    message = mSocket.NewMessage();
    VerifyOrExit(message != nullptr);

    while (aBytesAvailable > totalRead)
    {
        uint16_t length;

        // Read the `length` field.
        SuccessOrExit(ReadFromLinkBuffer(data, offset, *message, sizeof(uint16_t)));

        IgnoreError(message->Read(/* aOffset */ 0, length));
        length = BigEndian::HostSwap16(length);

        // Try to read `length` bytes.
        IgnoreError(message->SetLength(0));
        SuccessOrExit(ReadFromLinkBuffer(data, offset, *message, length));

        totalRead += length + sizeof(uint16_t);

        // Now process the read message as query response.
        ProcessResponse(*message);

        IgnoreError(message->SetLength(0));

        // Loop again to see if we can read another response.
    }

exit:
    // Inform `mEndPoint` about the total read and processed bytes
    IgnoreError(mEndpoint.CommitReceive(totalRead, /* aFlags */ 0));
    FreeMessage(message);
}

void Client::HandleTcpReceiveAvailableCallback(otTcpEndpoint *aEndpoint,
                                               size_t         aBytesAvailable,
                                               bool           aEndOfStream,
                                               size_t         aBytesRemaining)
{
    static_cast<Client *>(otTcpEndpointGetContext(aEndpoint))
        ->HandleTcpReceiveAvailable(aEndpoint, aBytesAvailable, aEndOfStream, aBytesRemaining);
}

void Client::HandleTcpDisconnected(otTcpEndpoint *aEndpoint, otTcpDisconnectedReason aReason)
{
    OT_UNUSED_VARIABLE(aEndpoint);
    OT_UNUSED_VARIABLE(aReason);
    QueryInfo info;

    IgnoreError(mEndpoint.Deinitialize());
    mTcpState = kTcpUninitialized;

    // Abort queries in case of connection failures
    for (Query &mainQuery : mMainQueries)
    {
        info.ReadFrom(mainQuery);

        if (info.mConfig.GetTransportProto() == QueryConfig::kDnsTransportTcp)
        {
            FinalizeQuery(mainQuery, kErrorAbort);
        }
    }
}

void Client::HandleTcpDisconnectedCallback(otTcpEndpoint *aEndpoint, otTcpDisconnectedReason aReason)
{
    static_cast<Client *>(otTcpEndpointGetContext(aEndpoint))->HandleTcpDisconnected(aEndpoint, aReason);
}

#endif // OPENTHREAD_CONFIG_DNS_CLIENT_OVER_TCP_ENABLE

} // namespace Dns
} // namespace ot

#endif // OPENTHREAD_CONFIG_DNS_CLIENT_ENABLE
