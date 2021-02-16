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

#include "common/code_utils.hpp"
#include "common/debug.hpp"
#include "common/instance.hpp"
#include "common/locator-getters.hpp"
#include "common/logging.hpp"
#include "net/udp6.hpp"
#include "thread/thread_netif.hpp"

#if OPENTHREAD_CONFIG_DNS_CLIENT_ENABLE

/**
 * @file
 *   This file implements the DNS client.
 */

namespace ot {
namespace Dns {

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

otError Client::Response::GetName(char *aNameBuffer, uint16_t aNameBufferSize) const
{
    uint16_t offset = kNameOffsetInQuery;

    return Name::ReadName(*mQuery, offset, aNameBuffer, aNameBufferSize);
}

otError Client::Response::FindHostAddress(Section       aSection,
                                          const Name &  aHostName,
                                          uint16_t      aIndex,
                                          Ip6::Address &aAddress,
                                          uint32_t &    aTtl) const
{
    otError     error;
    uint16_t    offset;
    uint16_t    numRecords;
    Name        name = aHostName;
    CnameRecord cnameRecord;
    AaaaRecord  aaaaRecord;

    VerifyOrExit(mMessage != nullptr, error = OT_ERROR_NOT_FOUND);

    // If the response includes a CNAME record mapping the query host
    // name to a canonical name, we then search for AAAA records
    // matching the canonical name.

    SelectSection(aSection, offset, numRecords);
    error = ResourceRecord::FindRecord(*mMessage, offset, numRecords, /* aIndex */ 0, aHostName, cnameRecord);

    if (error == OT_ERROR_NONE)
    {
        name.SetFromMessage(*mMessage, offset);
        SuccessOrExit(error = Name::ParseName(*mMessage, offset));
    }
    else
    {
        VerifyOrExit(error == OT_ERROR_NOT_FOUND);
    }

    SelectSection(aSection, offset, numRecords);
    SuccessOrExit(error = ResourceRecord::FindRecord(*mMessage, offset, numRecords, aIndex, name, aaaaRecord));
    aAddress = aaaaRecord.GetAddress();
    aTtl     = aaaaRecord.GetTtl();

exit:
    return error;
}

#if OPENTHREAD_CONFIG_DNS_CLIENT_SERVICE_DISCOVERY_ENABLE

otError Client::Response::FindServiceInfo(Section aSection, const Name &aName, ServiceInfo &aServiceInfo) const
{
    // This method searches for SRV and TXT records in the given
    // section matching the record name against `aName`, and updates
    // the `aServiceInfo` accordingly. It also searches for AAAA
    // record for host name associated with the service (from SRV
    // record). The search for AAAA record is always performed in
    // Additional Data section (independent of the value given in
    // `aSection`).

    otError   error;
    uint16_t  offset;
    uint16_t  numRecords;
    Name      hostName;
    SrvRecord srvRecord;
    TxtRecord txtRecord;

    VerifyOrExit(mMessage != nullptr, error = OT_ERROR_NOT_FOUND);

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

    error = FindHostAddress(kAdditionalDataSection, hostName, /* aIndex */ 0,
                            static_cast<Ip6::Address &>(aServiceInfo.mHostAddress), aServiceInfo.mHostAddressTtl);

    if (error == OT_ERROR_NOT_FOUND)
    {
        static_cast<Ip6::Address &>(aServiceInfo.mHostAddress).Clear();
        aServiceInfo.mHostAddressTtl = 0;
    }
    else
    {
        SuccessOrExit(error);
    }

    // A null `mTxtData` indicates that caller does not want to retrieve TXT data.
    VerifyOrExit(aServiceInfo.mTxtData != nullptr);

    // Search for a matching TXT record. If not found, indicate this by
    // setting `aServiceInfo.mTxtDataSize` to zero.

    SelectSection(aSection, offset, numRecords);
    error = ResourceRecord::FindRecord(*mMessage, offset, numRecords, /* aIndex */ 0, aName, txtRecord);

    switch (error)
    {
    case OT_ERROR_NONE:
        SuccessOrExit(error =
                          txtRecord.ReadTxtData(*mMessage, offset, aServiceInfo.mTxtData, aServiceInfo.mTxtDataSize));
        aServiceInfo.mTxtDataTtl = txtRecord.GetTtl();
        break;

    case OT_ERROR_NOT_FOUND:
        aServiceInfo.mTxtDataSize = 0;
        aServiceInfo.mTxtDataTtl  = 0;
        break;

    default:
        ExitNow();
    }

exit:
    return error;
}

#endif // OPENTHREAD_CONFIG_DNS_CLIENT_SERVICE_DISCOVERY_ENABLE

//---------------------------------------------------------------------------------------------------------------------
// Client::AddressResponse

otError Client::AddressResponse::GetAddress(uint16_t aIndex, Ip6::Address &aAddress, uint32_t &aTtl) const
{
    return FindHostAddress(kAnswerSection, Name(*mQuery, kNameOffsetInQuery), aIndex, aAddress, aTtl);
}

#if OPENTHREAD_CONFIG_DNS_CLIENT_SERVICE_DISCOVERY_ENABLE

//---------------------------------------------------------------------------------------------------------------------
// Client::BrowseResponse

otError Client::BrowseResponse::GetServiceInstance(uint16_t aIndex, char *aLabelBuffer, uint8_t aLabelBufferSize) const
{
    otError   error;
    uint16_t  offset;
    uint16_t  numRecords;
    Name      serviceName(*mQuery, kNameOffsetInQuery);
    PtrRecord ptrRecord;

    VerifyOrExit(mMessage != nullptr, error = OT_ERROR_NOT_FOUND);

    SelectSection(kAnswerSection, offset, numRecords);
    SuccessOrExit(error = ResourceRecord::FindRecord(*mMessage, offset, numRecords, aIndex, serviceName, ptrRecord));
    error = ptrRecord.ReadPtrName(*mMessage, offset, aLabelBuffer, aLabelBufferSize, nullptr, 0);

exit:
    return error;
}

otError Client::BrowseResponse::GetServiceInfo(const char *aInstanceLabel, ServiceInfo &aServiceInfo) const
{
    otError error;
    Name    instanceName;

    // Find a matching PTR record for the service instance label.
    // Then search and read SRV, TXT and AAAA records in Additional Data section
    // matching the same name to populate `aServiceInfo`.

    SuccessOrExit(error = FindPtrRecord(aInstanceLabel, instanceName));
    error = FindServiceInfo(kAdditionalDataSection, instanceName, aServiceInfo);

exit:
    return error;
}

otError Client::BrowseResponse::GetHostAddress(const char *  aHostName,
                                               uint16_t      aIndex,
                                               Ip6::Address &aAddress,
                                               uint32_t &    aTtl) const
{
    return FindHostAddress(kAdditionalDataSection, Name(aHostName), aIndex, aAddress, aTtl);
}

otError Client::BrowseResponse::FindPtrRecord(const char *aInstanceLabel, Name &aInstanceName) const
{
    // This method searches within the Answer Section for a PTR record
    // matching a given instance label @aInstanceLabel. If found, the
    // `aName` is updated to return the name in the message.

    otError   error;
    uint16_t  offset;
    Name      serviceName(*mQuery, kNameOffsetInQuery);
    uint16_t  numRecords;
    uint16_t  labelOffset;
    PtrRecord ptrRecord;

    VerifyOrExit(mMessage != nullptr, error = OT_ERROR_NOT_FOUND);

    SelectSection(kAnswerSection, offset, numRecords);

    for (; numRecords > 0; numRecords--)
    {
        SuccessOrExit(error = Name::CompareName(*mMessage, offset, serviceName));

        error = ResourceRecord::ReadRecord(*mMessage, offset, ptrRecord);

        if (error == OT_ERROR_NOT_FOUND)
        {
            // `ReadRecord()` updates `offset` to skip over a
            // non-matching record.
            continue;
        }

        SuccessOrExit(error);

        // It is a PTR record. Check the first label to match the
        // instance label and the rest of the name to match the service
        // name from `mQuery`.

        labelOffset = offset;
        error       = Name::CompareLabel(*mMessage, labelOffset, aInstanceLabel);

        if (error == OT_ERROR_NONE)
        {
            error = Name::CompareName(*mMessage, labelOffset, serviceName);

            if (error == OT_ERROR_NONE)
            {
                aInstanceName.SetFromMessage(*mMessage, offset);
                ExitNow();
            }
        }

        VerifyOrExit(error == OT_ERROR_NOT_FOUND);

        // Update offset to skip over the PTR record.
        offset += static_cast<uint16_t>(ptrRecord.GetSize()) - sizeof(ptrRecord);
    }

    error = OT_ERROR_NOT_FOUND;

exit:
    return error;
}

//---------------------------------------------------------------------------------------------------------------------
// Client::ServiceResponse

otError Client::ServiceResponse::GetServiceName(char *   aLabelBuffer,
                                                uint8_t  aLabelBufferSize,
                                                char *   aNameBuffer,
                                                uint16_t aNameBufferSize) const
{
    otError  error;
    uint16_t offset = kNameOffsetInQuery;

    SuccessOrExit(error = Name::ReadLabel(*mQuery, offset, aLabelBuffer, aLabelBufferSize));

    VerifyOrExit(aNameBuffer != nullptr);
    SuccessOrExit(error = Name::ReadName(*mQuery, offset, aNameBuffer, aNameBufferSize));

exit:
    return error;
}

otError Client::ServiceResponse::GetServiceInfo(ServiceInfo &aServiceInfo) const
{
    // Search and read SRV, TXT records in Answer Section
    // matching name from query.

    return FindServiceInfo(kAnswerSection, Name(*mQuery, kNameOffsetInQuery), aServiceInfo);
}

otError Client::ServiceResponse::GetHostAddress(const char *  aHostName,
                                                uint16_t      aIndex,
                                                Ip6::Address &aAddress,
                                                uint32_t &    aTtl) const
{
    return FindHostAddress(kAdditionalDataSection, Name(aHostName), aIndex, aAddress, aTtl);
}

#endif // OPENTHREAD_CONFIG_DNS_CLIENT_SERVICE_DISCOVERY_ENABLE

//---------------------------------------------------------------------------------------------------------------------
// Client

const uint16_t Client::kAddressQueryRecordTypes[] = {ResourceRecord::kTypeAaaa};
#if OPENTHREAD_CONFIG_DNS_CLIENT_SERVICE_DISCOVERY_ENABLE
const uint16_t Client::kBrowseQueryRecordTypes[]  = {ResourceRecord::kTypePtr};
const uint16_t Client::kServiceQueryRecordTypes[] = {ResourceRecord::kTypeSrv, ResourceRecord::kTypeTxt};
#endif

const uint8_t Client::kQuestionCount[] = {
    /* (0) kAddressQuery -> */ OT_ARRAY_LENGTH(kAddressQueryRecordTypes), // AAAA records
#if OPENTHREAD_CONFIG_DNS_CLIENT_SERVICE_DISCOVERY_ENABLE
    /* (1) kBrowseQuery  -> */ OT_ARRAY_LENGTH(kBrowseQueryRecordTypes),  // PTR records
    /* (2) kServiceQuery -> */ OT_ARRAY_LENGTH(kServiceQueryRecordTypes), // SRV and TXT records
#endif
};

const uint16_t *Client::kQuestionRecordTypes[] = {
    /* (0) kAddressQuery -> */ kAddressQueryRecordTypes,
#if OPENTHREAD_CONFIG_DNS_CLIENT_SERVICE_DISCOVERY_ENABLE
    /* (1) kBrowseQuery  -> */ kBrowseQueryRecordTypes,
    /* (2) kServiceQuery -> */ kServiceQueryRecordTypes,
#endif
};

Client::Client(Instance &aInstance)
    : InstanceLocator(aInstance)
    , mSocket(aInstance)
    , mTimer(aInstance, Client::HandleTimer)
{
    static_assert(kAddressQuery == 0, "kAddressQuery value is not correct");
#if OPENTHREAD_CONFIG_DNS_CLIENT_SERVICE_DISCOVERY_ENABLE
    static_assert(kBrowseQuery == 1, "kBrowseQuery value is not correct");
    static_assert(kServiceQuery == 2, "kServiceQuery value is not correct");
#endif
}

otError Client::Start(void)
{
    otError error;

    SuccessOrExit(error = mSocket.Open(&Client::HandleUdpReceive, this));
    SuccessOrExit(error = mSocket.Bind());

exit:
    return error;
}

void Client::Stop(void)
{
    Query *query;

    while ((query = mQueries.GetHead()) != nullptr)
    {
        FinalizeQuery(*query, OT_ERROR_ABORT);
    }

    IgnoreError(mSocket.Close());
}

otError Client::ResolveAddress(const Ip6::SockAddr &aServerSockAddr,
                               const char *         aHostName,
                               bool                 aNoRecursion,
                               AddressCallback      aCallback,
                               void *               aContext)
{
    QueryInfo info;

    info.Clear();
    info.mQueryType                 = kAddressQuery;
    info.mNoRecursion               = aNoRecursion;
    info.mCallback.mAddressCallback = aCallback;

    return StartQuery(info, aServerSockAddr, nullptr, aHostName, aContext);
}

#if OPENTHREAD_CONFIG_DNS_CLIENT_SERVICE_DISCOVERY_ENABLE

otError Client::Browse(const Ip6::SockAddr &aServerSockAddr,
                       const char *         aServiceName,
                       BrowseCallback       aCallback,
                       void *               aContext)
{
    QueryInfo info;

    info.Clear();
    info.mQueryType                = kBrowseQuery;
    info.mCallback.mBrowseCallback = aCallback;

    return StartQuery(info, aServerSockAddr, nullptr, aServiceName, aContext);
}

otError Client::ResolveService(const Ip6::SockAddr &aServerSockAddr,
                               const char *         aInstanceLabel,
                               const char *         aServiceName,
                               ServiceCallback      aCallback,
                               void *               aContext)
{
    QueryInfo info;

    info.Clear();
    info.mQueryType                 = kServiceQuery;
    info.mCallback.mServiceCallback = aCallback;

    return StartQuery(info, aServerSockAddr, aInstanceLabel, aServiceName, aContext);
}

#endif // OPENTHREAD_CONFIG_DNS_CLIENT_SERVICE_DISCOVERY_ENABLE

otError Client::StartQuery(QueryInfo &          aInfo,
                           const Ip6::SockAddr &aServerSockAddr,
                           const char *         aLabel,
                           const char *         aName,
                           void *               aContext)
{
    // This method assumes that `mQueryType` and `mCallback` to be
    // already set by caller on `aInfo`. The `aLabel` can be `nullptr`
    // and then `aName` provides the full name, otherwise the name is
    // appended as `{aLabel}.{aName}`.

    otError error;
    Query * query;

    VerifyOrExit(mSocket.IsBound(), error = OT_ERROR_INVALID_STATE);

    aInfo.mServerSockAddr  = aServerSockAddr;
    aInfo.mCallbackContext = aContext;

    SuccessOrExit(error = AllocateQuery(aInfo, aLabel, aName, query));
    mQueries.Enqueue(*query);

    SendQuery(*query);

exit:
    return error;
}

otError Client::AllocateQuery(const QueryInfo &aInfo, const char *aLabel, const char *aName, Query *&aQuery)
{
    otError error = OT_ERROR_NONE;

    aQuery = Get<MessagePool>().New(Message::kTypeOther, /* aReserveHeader */ 0);
    VerifyOrExit(aQuery != nullptr, error = OT_ERROR_NO_BUFS);

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

void Client::FreeQuery(Query &aQuery)
{
    mQueries.Dequeue(aQuery);
    aQuery.Free();
}

void Client::SendQuery(Query &aQuery)
{
    QueryInfo info;

    info.ReadFrom(aQuery);

    SendQuery(aQuery, info, /* aUpdateTimer */ true);
}

void Client::SendQuery(Query &aQuery, QueryInfo &aInfo, bool aUpdateTimer)
{
    // This method prepares and sends a query message represented by
    // `aQuery` and `aInfo`. This method updates `aInfo` (e.g., sets
    // the new `mRetransmissionTime`) and updates it in `aQuery` as
    // well. `aUpdateTimer` indicates whether the timer should be
    // updated when query is sent or not (used in the case where timer
    // is handled by caller).

    otError          error   = OT_ERROR_NONE;
    Message *        message = nullptr;
    Header           header;
    Ip6::MessageInfo messageInfo;

    aInfo.mRetransmissionTime = TimerMilli::GetNow() + kResponseTimeout;

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

    if (!aInfo.mNoRecursion)
    {
        header.SetRecursionDesiredFlag();
    }

    header.SetQuestionCount(kQuestionCount[aInfo.mQueryType]);

    message = mSocket.NewMessage(0);
    VerifyOrExit(message != nullptr, error = OT_ERROR_NO_BUFS);

    SuccessOrExit(error = message->Append(header));

    // Prepare the question section.

    for (uint8_t num = 0; num < kQuestionCount[aInfo.mQueryType]; num++)
    {
        SuccessOrExit(error = AppendNameFromQuery(aQuery, *message));
        SuccessOrExit(error = message->Append(Question(kQuestionRecordTypes[aInfo.mQueryType][num])));
    }

    messageInfo.SetPeerAddr(aInfo.mServerSockAddr.GetAddress());
    messageInfo.SetPeerPort(aInfo.mServerSockAddr.GetPort());

    SuccessOrExit(error = mSocket.SendTo(*message, messageInfo));

exit:
    FreeMessageOnError(message, error);

    UpdateQuery(aQuery, aInfo);

    if (aUpdateTimer)
    {
        mTimer.FireAtIfEarlier(aInfo.mRetransmissionTime);
    }
}

otError Client::AppendNameFromQuery(const Query &aQuery, Message &aMessage)
{
    otError  error = OT_ERROR_NONE;
    uint16_t offset;
    uint16_t length;

    // The name is encoded and included after the `Info` in `aQuery`. We
    // first calculate the encoded length of the name, then grow the
    // message, and finally copy the encoded name bytes from `aQuery`
    // into `aMessage`.

    length = aQuery.GetLength() - kNameOffsetInQuery;

    offset = aMessage.GetLength();
    SuccessOrExit(error = aMessage.SetLength(offset + length));

    aQuery.CopyTo(/* aSourceOffset */ kNameOffsetInQuery, /* aDestOffset */ offset, length, aMessage);

exit:
    return error;
}

void Client::FinalizeQuery(Query &aQuery, otError aError)
{
    Response  response;
    QueryInfo info;

    response.mQuery = &aQuery;
    info.ReadFrom(aQuery);

    FinalizeQuery(response, info.mQueryType, aError);
}

void Client::FinalizeQuery(Response &aResponse, QueryType aType, otError aError)
{
    Callback callback;
    void *   context;

    GetCallback(*aResponse.mQuery, callback, context);

    switch (aType)
    {
    case kAddressQuery:
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

    case kServiceQuery:
        if (callback.mServiceCallback != nullptr)
        {
            callback.mServiceCallback(aError, &aResponse, context);
        }
        break;
#endif
    }

    FreeQuery(*aResponse.mQuery);
}

void Client::GetCallback(const Query &aQuery, Callback &aCallback, void *&aContext)
{
    QueryInfo info;

    info.ReadFrom(aQuery);

    aCallback = info.mCallback;
    aContext  = info.mCallbackContext;
}

Client::Query *Client::FindQueryById(uint16_t aMessageId)
{
    Query *   query;
    QueryInfo info;

    for (query = mQueries.GetHead(); query != nullptr; query = query->GetNext())
    {
        info.ReadFrom(*query);

        if (info.mMessageId == aMessageId)
        {
            break;
        }
    }

    return query;
}

void Client::HandleUdpReceive(void *aContext, otMessage *aMessage, const otMessageInfo *aMsgInfo)
{
    OT_UNUSED_VARIABLE(aMsgInfo);

    static_cast<Client *>(aContext)->ProcessResponse(*static_cast<Message *>(aMessage));
}

void Client::ProcessResponse(const Message &aMessage)
{
    Response  response;
    QueryType type;
    otError   responseError;

    response.mMessage = &aMessage;

    SuccessOrExit(ParseResponse(response, type, responseError));
    FinalizeQuery(response, type, responseError);

exit:
    return;
}

otError Client::ParseResponse(Response &aResponse, QueryType &aType, otError &aResponseError)
{
    otError        error   = OT_ERROR_NONE;
    const Message &message = *aResponse.mMessage;
    uint16_t       offset  = message.GetOffset();
    Header         header;
    QueryInfo      info;

    SuccessOrExit(error = message.Read(offset, header));
    offset += sizeof(Header);

    VerifyOrExit((header.GetType() == Header::kTypeResponse) && (header.GetQueryType() == Header::kQueryTypeStandard) &&
                     !header.IsTruncationFlagSet(),
                 error = OT_ERROR_DROP);

    aResponse.mQuery = FindQueryById(header.GetMessageId());
    VerifyOrExit(aResponse.mQuery != nullptr, error = OT_ERROR_NOT_FOUND);

    info.ReadFrom(*aResponse.mQuery);
    aType = info.mQueryType;

    // Check the Question Section

    VerifyOrExit(header.GetQuestionCount() == kQuestionCount[aType], error = OT_ERROR_PARSE);

    for (uint8_t num = 0; num < kQuestionCount[aType]; num++)
    {
        // The name is encoded after `Info` struct in `query`.
        SuccessOrExit(error = Name::CompareName(message, offset, *aResponse.mQuery, kNameOffsetInQuery));
        offset += sizeof(Question);
    }

    // Check the answer, authority and additional record sections

    aResponse.mAnswerOffset = offset;
    SuccessOrExit(error = ResourceRecord::ParseRecords(message, offset, header.GetAnswerCount()));
    SuccessOrExit(error = ResourceRecord::ParseRecords(message, offset, header.GetAuthorityRecordCount()));
    aResponse.mAdditionalOffset = offset;
    SuccessOrExit(error = ResourceRecord::ParseRecords(message, offset, header.GetAdditionalRecordCount()));

    aResponse.mAnswerRecordCount     = header.GetAnswerCount();
    aResponse.mAdditionalRecordCount = header.GetAdditionalRecordCount();

    // Check the response code from server

    aResponseError = Header::ResponseCodeToError(header.GetResponseCode());

exit:
    if (error != OT_ERROR_NONE)
    {
        otLogInfoDns("Failed to parse response %s", otThreadErrorToString(error));
    }

    return error;
}

void Client::HandleTimer(Timer &aTimer)
{
    aTimer.Get<Client>().HandleTimer();
}

void Client::HandleTimer(void)
{
    TimeMilli now      = TimerMilli::GetNow();
    TimeMilli nextTime = now.GetDistantFuture();
    Query *   nextQuery;
    QueryInfo info;

    for (Query *query = mQueries.GetHead(); query != nullptr; query = nextQuery)
    {
        nextQuery = query->GetNext();

        info.ReadFrom(*query);

        if (now >= info.mRetransmissionTime)
        {
            if (info.mRetransmissionCount >= kMaxRetransmit)
            {
                FinalizeQuery(*query, OT_ERROR_RESPONSE_TIMEOUT);
                continue;
            }

            info.mRetransmissionCount++;
            SendQuery(*query, info, /* aUpdateTimer */ false);
        }

        if (nextTime > info.mRetransmissionTime)
        {
            nextTime = info.mRetransmissionTime;
        }
    }

    if (nextTime < now.GetDistantFuture())
    {
        mTimer.FireAt(nextTime);
    }
}

} // namespace Dns
} // namespace ot

#endif // OPENTHREAD_CONFIG_DNS_CLIENT_ENABLE
