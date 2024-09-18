/*
 *  Copyright (c) 2024, The OpenThread Authors.
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

#include "mdns.hpp"

#if OPENTHREAD_CONFIG_MULTICAST_DNS_ENABLE

#include "instance/instance.hpp"

/**
 * @file
 *   This file implements the Multicast DNS (mDNS) per RFC 6762.
 */

namespace ot {
namespace Dns {
namespace Multicast {

RegisterLogModule("MulticastDns");

//---------------------------------------------------------------------------------------------------------------------
// otPlatMdns callbacks

extern "C" void otPlatMdnsHandleReceive(otInstance                  *aInstance,
                                        otMessage                   *aMessage,
                                        bool                         aIsUnicast,
                                        const otPlatMdnsAddressInfo *aAddress)
{
    AsCoreType(aInstance).Get<Core>().HandleMessage(AsCoreType(aMessage), aIsUnicast, AsCoreType(aAddress));
}

//----------------------------------------------------------------------------------------------------------------------
// Core

const char Core::kLocalDomain[]         = "local.";
const char Core::kUdpServiceLabel[]     = "_udp";
const char Core::kTcpServiceLabel[]     = "_tcp";
const char Core::kSubServiceLabel[]     = "_sub";
const char Core::kServicesDnssdLabels[] = "_services._dns-sd._udp";

Core::Core(Instance &aInstance)
    : InstanceLocator(aInstance)
    , mIsEnabled(false)
    , mIsQuestionUnicastAllowed(kDefaultQuAllowed)
    , mMaxMessageSize(kMaxMessageSize)
    , mInfraIfIndex(0)
    , mMultiPacketRxMessages(aInstance)
    , mNextProbeTxTime(TimerMilli::GetNow() - 1)
    , mEntryTimer(aInstance)
    , mEntryTask(aInstance)
    , mTxMessageHistory(aInstance)
    , mConflictCallback(nullptr)
    , mNextQueryTxTime(TimerMilli::GetNow() - 1)
    , mCacheTimer(aInstance)
    , mCacheTask(aInstance)
{
}

Error Core::SetEnabled(bool aEnable, uint32_t aInfraIfIndex)
{
    Error error = kErrorNone;

    VerifyOrExit(aEnable != mIsEnabled, error = kErrorAlready);
    SuccessOrExit(error = otPlatMdnsSetListeningEnabled(&GetInstance(), aEnable, aInfraIfIndex));

    mIsEnabled    = aEnable;
    mInfraIfIndex = aInfraIfIndex;

    if (mIsEnabled)
    {
        LogInfo("Enabling on infra-if-index %lu", ToUlong(mInfraIfIndex));
    }
    else
    {
        LogInfo("Disabling");
    }

    if (!mIsEnabled)
    {
        mHostEntries.Clear();
        mServiceEntries.Clear();
        mServiceTypes.Clear();
        mMultiPacketRxMessages.Clear();
        mTxMessageHistory.Clear();
        mEntryTimer.Stop();

        mBrowseCacheList.Clear();
        mSrvCacheList.Clear();
        mTxtCacheList.Clear();
        mIp6AddrCacheList.Clear();
        mIp4AddrCacheList.Clear();
        mCacheTimer.Stop();
    }

    Get<Dnssd>().HandleMdnsCoreStateChange();

exit:
    return error;
}

#if OPENTHREAD_CONFIG_MULTICAST_DNS_AUTO_ENABLE_ON_INFRA_IF
void Core::HandleInfraIfStateChanged(void)
{
    IgnoreError(SetEnabled(Get<BorderRouter::InfraIf>().IsRunning(), Get<BorderRouter::InfraIf>().GetIfIndex()));
}
#endif

template <typename EntryType, typename ItemInfo>
Error Core::Register(const ItemInfo &aItemInfo, RequestId aRequestId, RegisterCallback aCallback)
{
    Error      error = kErrorNone;
    EntryType *entry;

    VerifyOrExit(mIsEnabled, error = kErrorInvalidState);

    entry = GetEntryList<EntryType>().FindMatching(aItemInfo);

    if (entry == nullptr)
    {
        entry = EntryType::AllocateAndInit(GetInstance(), aItemInfo);
        OT_ASSERT(entry != nullptr);
        GetEntryList<EntryType>().Push(*entry);
    }

    entry->Register(aItemInfo, Callback(aRequestId, aCallback));

exit:
    return error;
}

template <typename EntryType, typename ItemInfo> Error Core::Unregister(const ItemInfo &aItemInfo)
{
    Error      error = kErrorNone;
    EntryType *entry;

    VerifyOrExit(mIsEnabled, error = kErrorInvalidState);

    entry = GetEntryList<EntryType>().FindMatching(aItemInfo);

    if (entry != nullptr)
    {
        entry->Unregister(aItemInfo);
    }

exit:
    return error;
}

Error Core::RegisterHost(const Host &aHost, RequestId aRequestId, RegisterCallback aCallback)
{
    return Register<HostEntry>(aHost, aRequestId, aCallback);
}

Error Core::UnregisterHost(const Host &aHost) { return Unregister<HostEntry>(aHost); }

Error Core::RegisterService(const Service &aService, RequestId aRequestId, RegisterCallback aCallback)
{
    return Register<ServiceEntry>(aService, aRequestId, aCallback);
}

Error Core::UnregisterService(const Service &aService) { return Unregister<ServiceEntry>(aService); }

Error Core::RegisterKey(const Key &aKey, RequestId aRequestId, RegisterCallback aCallback)
{
    return IsKeyForService(aKey) ? Register<ServiceEntry>(aKey, aRequestId, aCallback)
                                 : Register<HostEntry>(aKey, aRequestId, aCallback);
}

Error Core::UnregisterKey(const Key &aKey)
{
    return IsKeyForService(aKey) ? Unregister<ServiceEntry>(aKey) : Unregister<HostEntry>(aKey);
}

#if OPENTHREAD_CONFIG_MULTICAST_DNS_ENTRY_ITERATION_API_ENABLE

Core::Iterator *Core::AllocateIterator(void) { return EntryIterator::Allocate(GetInstance()); }

void Core::FreeIterator(Iterator &aIterator) { static_cast<EntryIterator &>(aIterator).Free(); }

Error Core::GetNextHost(Iterator &aIterator, Host &aHost, EntryState &aState) const
{
    return static_cast<EntryIterator &>(aIterator).GetNextHost(aHost, aState);
}

Error Core::GetNextService(Iterator &aIterator, Service &aService, EntryState &aState) const
{
    return static_cast<EntryIterator &>(aIterator).GetNextService(aService, aState);
}

Error Core::GetNextKey(Iterator &aIterator, Key &aKey, EntryState &aState) const
{
    return static_cast<EntryIterator &>(aIterator).GetNextKey(aKey, aState);
}

Error Core::GetNextBrowser(Iterator &aIterator, Browser &aBrowser, CacheInfo &aInfo) const
{
    return static_cast<EntryIterator &>(aIterator).GetNextBrowser(aBrowser, aInfo);
}

Error Core::GetNextSrvResolver(Iterator &aIterator, SrvResolver &aResolver, CacheInfo &aInfo) const
{
    return static_cast<EntryIterator &>(aIterator).GetNextSrvResolver(aResolver, aInfo);
}

Error Core::GetNextTxtResolver(Iterator &aIterator, TxtResolver &aResolver, CacheInfo &aInfo) const
{
    return static_cast<EntryIterator &>(aIterator).GetNextTxtResolver(aResolver, aInfo);
}

Error Core::GetNextIp6AddressResolver(Iterator &aIterator, AddressResolver &aResolver, CacheInfo &aInfo) const
{
    return static_cast<EntryIterator &>(aIterator).GetNextIp6AddressResolver(aResolver, aInfo);
}

Error Core::GetNextIp4AddressResolver(Iterator &aIterator, AddressResolver &aResolver, CacheInfo &aInfo) const
{
    return static_cast<EntryIterator &>(aIterator).GetNextIp4AddressResolver(aResolver, aInfo);
}

#endif // OPENTHREAD_CONFIG_MULTICAST_DNS_ENTRY_ITERATION_API_ENABLE

void Core::InvokeConflictCallback(const char *aName, const char *aServiceType)
{
    if (mConflictCallback != nullptr)
    {
        mConflictCallback(&GetInstance(), aName, aServiceType);
    }
}
void Core::HandleMessage(Message &aMessage, bool aIsUnicast, const AddressInfo &aSenderAddress)
{
    OwnedPtr<Message>   messagePtr(&aMessage);
    OwnedPtr<RxMessage> rxMessagePtr;

    VerifyOrExit(mIsEnabled);

    rxMessagePtr.Reset(RxMessage::AllocateAndInit(GetInstance(), messagePtr, aIsUnicast, aSenderAddress));
    VerifyOrExit(!rxMessagePtr.IsNull());

    if (rxMessagePtr->IsQuery())
    {
        // Check if this is a continuation of a multi-packet query.
        // Initial query message sets the "Truncated" flag.
        // Subsequent messages from the same sender contain no
        // question and only known-answer records.

        if ((rxMessagePtr->GetRecordCounts().GetFor(kQuestionSection) == 0) &&
            (rxMessagePtr->GetRecordCounts().GetFor(kAnswerSection) > 0))
        {
            mMultiPacketRxMessages.AddToExisting(rxMessagePtr);
            ExitNow();
        }

        switch (rxMessagePtr->ProcessQuery(/* aShouldProcessTruncated */ false))
        {
        case RxMessage::kProcessed:
            break;

        case RxMessage::kSaveAsMultiPacket:
            // This is a truncated multi-packet query and we can
            // answer some questions in this query. We save it in
            // `mMultiPacketRxMessages` list and defer its response
            // for a random time waiting to receive next messages
            // containing additional known-answer records.

            mMultiPacketRxMessages.AddNew(rxMessagePtr);
            break;
        }
    }
    else
    {
        rxMessagePtr->ProcessResponse();
    }

exit:
    return;
}

void Core::HandleEntryTimer(void)
{
    EntryTimerContext context(GetInstance());

    // We process host entries before service entries. This order
    // ensures we can determine whether host addresses have already
    // been appended to the Answer section (when processing service entries),
    // preventing duplicates.

    for (HostEntry &entry : mHostEntries)
    {
        entry.HandleTimer(context);
    }

    for (ServiceEntry &entry : mServiceEntries)
    {
        entry.HandleTimer(context);
    }

    for (ServiceType &serviceType : mServiceTypes)
    {
        serviceType.HandleTimer(context);
    }

    context.GetProbeMessage().Send();
    context.GetResponseMessage().Send();

    RemoveEmptyEntries();

    mEntryTimer.FireAtIfEarlier(context.GetNextFireTime());
}

void Core::RemoveEmptyEntries(void)
{
    mHostEntries.RemoveAndFreeAllMatching(Entry::kRemoving);
    mServiceEntries.RemoveAndFreeAllMatching(Entry::kRemoving);
}

void Core::HandleEntryTask(void)
{
    // `mEntryTask` serves two purposes:
    //
    // Invoking callbacks: This ensures `Register()` calls will always
    // return before invoking the callback, even when entry is
    // already in `kRegistered` state and registration is immediately
    // successful.
    //
    // Removing empty entries after `Unregister()` calls: This
    // prevents modification of `mHostEntries` and `mServiceEntries`
    // during callback execution while we are iterating over these
    // lists. Allows us to safely call `Register()` or `Unregister()`
    // from callbacks without iterator invalidation.

    for (HostEntry &entry : mHostEntries)
    {
        entry.InvokeCallbacks();
    }

    for (ServiceEntry &entry : mServiceEntries)
    {
        entry.InvokeCallbacks();
    }

    RemoveEmptyEntries();
}

uint32_t Core::DetermineTtl(uint32_t aTtl, uint32_t aDefaultTtl)
{
    return (aTtl == kUnspecifiedTtl) ? aDefaultTtl : aTtl;
}

bool Core::NameMatch(const Heap::String &aHeapString, const char *aName)
{
    // Compares a DNS name given as a `Heap::String` with a
    // `aName` C string.

    return !aHeapString.IsNull() && StringMatch(aHeapString.AsCString(), aName, kStringCaseInsensitiveMatch);
}

bool Core::NameMatch(const Heap::String &aFirst, const Heap::String &aSecond)
{
    // Compares two DNS names given as `Heap::String`.

    return !aSecond.IsNull() && NameMatch(aFirst, aSecond.AsCString());
}

void Core::UpdateCacheFlushFlagIn(ResourceRecord &aResourceRecord, Section aSection, bool aIsLegacyUnicast)
{
    // Do not set the cache-flush flag if the record is
    // appended in Authority Section in a probe message,
    // or is intended for a Legacy Unicast response.

    if (aSection != kAuthoritySection && !aIsLegacyUnicast)
    {
        aResourceRecord.SetClass(aResourceRecord.GetClass() | kClassCacheFlushFlag);
    }
}

void Core::UpdateRecordLengthInMessage(ResourceRecord &aRecord, Message &aMessage, uint16_t aOffset)
{
    // Determines the records DATA length and updates it in a message.
    // Should be called immediately after all the fields in the
    // record are appended to the message. `aOffset` gives the offset
    // in the message to the start of the record.

    aRecord.SetLength(aMessage.GetLength() - aOffset - sizeof(ResourceRecord));
    aMessage.Write(aOffset, aRecord);
}

void Core::UpdateCompressOffset(uint16_t &aOffset, uint16_t aNewOffset)
{
    if ((aOffset == kUnspecifiedOffset) && (aNewOffset != kUnspecifiedOffset))
    {
        aOffset = aNewOffset;
    }
}

bool Core::QuestionMatches(uint16_t aQuestionRrType, uint16_t aRrType)
{
    return (aQuestionRrType == aRrType) || (aQuestionRrType == ResourceRecord::kTypeAny);
}

bool Core::RrClassIsInternetOrAny(uint16_t aRrClass)
{
    aRrClass &= kClassMask;

    return (aRrClass == ResourceRecord::kClassInternet) || (aRrClass == ResourceRecord::kClassAny);
}

//----------------------------------------------------------------------------------------------------------------------
// Core::Callback

Core::Callback::Callback(RequestId aRequestId, RegisterCallback aCallback)
    : mRequestId(aRequestId)
    , mCallback(aCallback)
{
}

void Core::Callback::InvokeAndClear(Instance &aInstance, Error aError)
{
    if (mCallback != nullptr)
    {
        RegisterCallback callback  = mCallback;
        RequestId        requestId = mRequestId;

        Clear();

        callback(&aInstance, requestId, aError);
    }
}

//----------------------------------------------------------------------------------------------------------------------
// Core::RecordCounts

void Core::RecordCounts::ReadFrom(const Header &aHeader)
{
    mCounts[kQuestionSection]       = aHeader.GetQuestionCount();
    mCounts[kAnswerSection]         = aHeader.GetAnswerCount();
    mCounts[kAuthoritySection]      = aHeader.GetAuthorityRecordCount();
    mCounts[kAdditionalDataSection] = aHeader.GetAdditionalRecordCount();
}

void Core::RecordCounts::WriteTo(Header &aHeader) const
{
    aHeader.SetQuestionCount(mCounts[kQuestionSection]);
    aHeader.SetAnswerCount(mCounts[kAnswerSection]);
    aHeader.SetAuthorityRecordCount(mCounts[kAuthoritySection]);
    aHeader.SetAdditionalRecordCount(mCounts[kAdditionalDataSection]);
}

bool Core::RecordCounts::IsEmpty(void) const
{
    // Indicates whether or not all counts are zero.

    bool isEmpty = true;

    for (uint16_t count : mCounts)
    {
        if (count != 0)
        {
            isEmpty = false;
            break;
        }
    }

    return isEmpty;
}

//----------------------------------------------------------------------------------------------------------------------
// Core::AddressArray

bool Core::AddressArray::Matches(const Ip6::Address *aAddresses, uint16_t aNumAddresses) const
{
    bool matches = false;

    VerifyOrExit(aNumAddresses == GetLength());

    for (uint16_t i = 0; i < aNumAddresses; i++)
    {
        VerifyOrExit(Contains(aAddresses[i]));
    }

    matches = true;

exit:
    return matches;
}

void Core::AddressArray::SetFrom(const Ip6::Address *aAddresses, uint16_t aNumAddresses)
{
    Free();
    SuccessOrAssert(ReserveCapacity(aNumAddresses));

    for (uint16_t i = 0; i < aNumAddresses; i++)
    {
        IgnoreError(PushBack(aAddresses[i]));
    }
}

//----------------------------------------------------------------------------------------------------------------------
// Core::RecordInfo

template <typename UintType> void Core::RecordInfo::UpdateProperty(UintType &aProperty, UintType aValue)
{
    // Updates a property variable associated with this record. The
    // `aProperty` is updated if the record is empty (has no value
    // yet) or if its current value differs from the new `aValue`. If
    // the property is changed, we prepare the record to be announced.

    // This template version works with `UintType` properties. There
    // are similar overloads for `Heap::Data` and `Heap::String` and
    // `AddressArray` property types below.

    static_assert(TypeTraits::IsSame<UintType, uint8_t>::kValue || TypeTraits::IsSame<UintType, uint16_t>::kValue ||
                      TypeTraits::IsSame<UintType, uint32_t>::kValue || TypeTraits::IsSame<UintType, uint64_t>::kValue,
                  "UintType must be `uint8_t`, `uint16_t`, `uint32_t`, or `uint64_t`");

    if (!mIsPresent || (aProperty != aValue))
    {
        mIsPresent = true;
        aProperty  = aValue;
        StartAnnouncing();
    }
}

void Core::RecordInfo::UpdateProperty(Heap::String &aStringProperty, const char *aString)
{
    if (!mIsPresent || !NameMatch(aStringProperty, aString))
    {
        mIsPresent = true;
        SuccessOrAssert(aStringProperty.Set(aString));
        StartAnnouncing();
    }
}

void Core::RecordInfo::UpdateProperty(Heap::Data &aDataProperty, const uint8_t *aData, uint16_t aLength)
{
    if (!mIsPresent || !aDataProperty.Matches(aData, aLength))
    {
        mIsPresent = true;
        SuccessOrAssert(aDataProperty.SetFrom(aData, aLength));
        StartAnnouncing();
    }
}

void Core::RecordInfo::UpdateProperty(AddressArray &aAddrProperty, const Ip6::Address *aAddrs, uint16_t aNumAddrs)
{
    if (!mIsPresent || !aAddrProperty.Matches(aAddrs, aNumAddrs))
    {
        mIsPresent = true;
        aAddrProperty.SetFrom(aAddrs, aNumAddrs);
        StartAnnouncing();
    }
}

uint32_t Core::RecordInfo::GetTtl(bool aIsLegacyUnicast) const
{
    return aIsLegacyUnicast ? Min(kMaxLegacyUnicastTtl, mTtl) : mTtl;
}

void Core::RecordInfo::UpdateTtl(uint32_t aTtl) { return UpdateProperty(mTtl, aTtl); }

void Core::RecordInfo::StartAnnouncing(void)
{
    if (mIsPresent)
    {
        mAnnounceCounter = 0;
        mAnnounceTime    = TimerMilli::GetNow();
    }
}

bool Core::RecordInfo::CanAnswer(void) const { return (mIsPresent && (mTtl > 0)); }

void Core::RecordInfo::ScheduleAnswer(const AnswerInfo &aInfo)
{
    VerifyOrExit(CanAnswer());

    if (aInfo.mUnicastResponse || aInfo.mLegacyUnicastResponse)
    {
        mUnicastAnswerPending = true;
        ExitNow();
    }

    if (!aInfo.mIsProbe)
    {
        // Rate-limiting multicasts to prevent excessive packet flooding
        // (RFC 6762 section 6): We enforce a minimum interval of one
        // second (`kMinIntervalBetweenMulticast`) between multicast
        // transmissions of the same record. Skip the new request if the
        // answer time is too close to the last multicast time. A querier
        // that did not receive and cache the previous transmission will
        // retry its request.

        VerifyOrExit(GetDurationSinceLastMulticast(aInfo.mAnswerTime) >= kMinIntervalBetweenMulticast);
    }

    if (mMulticastAnswerPending)
    {
        VerifyOrExit(aInfo.mAnswerTime < mAnswerTime);
    }

    mMulticastAnswerPending = true;
    mAnswerTime             = aInfo.mAnswerTime;

exit:
    return;
}

bool Core::RecordInfo::ShouldAppendTo(TxMessage &aResponse, TimeMilli aNow) const
{
    bool shouldAppend = false;

    VerifyOrExit(mIsPresent);

    switch (aResponse.GetType())
    {
    case TxMessage::kMulticastResponse:

        if ((mAnnounceCounter < kNumberOfAnnounces) && (mAnnounceTime <= aNow))
        {
            shouldAppend = true;
            ExitNow();
        }

        shouldAppend = mMulticastAnswerPending && (mAnswerTime <= aNow);
        break;

    case TxMessage::kUnicastResponse:
    case TxMessage::kLegacyUnicastResponse:
        shouldAppend = mUnicastAnswerPending;
        break;

    default:
        break;
    }

exit:
    return shouldAppend;
}

void Core::RecordInfo::UpdateStateAfterAnswer(const TxMessage &aResponse)
{
    // Updates the state after a unicast or multicast response is
    // prepared containing the record in the Answer section.

    VerifyOrExit(mIsPresent);

    switch (aResponse.GetType())
    {
    case TxMessage::kMulticastResponse:
        VerifyOrExit(mAppendState == kAppendedInMulticastMsg);
        VerifyOrExit(mAppendSection == kAnswerSection);

        mMulticastAnswerPending = false;

        if (mAnnounceCounter < kNumberOfAnnounces)
        {
            mAnnounceCounter++;

            if (mAnnounceCounter < kNumberOfAnnounces)
            {
                uint32_t delay = (1U << (mAnnounceCounter - 1)) * kAnnounceInterval;

                mAnnounceTime = TimerMilli::GetNow() + delay;
            }
            else if (mTtl == 0)
            {
                // We are done announcing the removed record with zero TTL.
                mIsPresent = false;
            }
        }

        break;

    case TxMessage::kUnicastResponse:
    case TxMessage::kLegacyUnicastResponse:
        VerifyOrExit(IsAppended());
        VerifyOrExit(mAppendSection == kAnswerSection);
        mUnicastAnswerPending = false;
        break;

    default:
        break;
    }

exit:
    return;
}

void Core::RecordInfo::UpdateFireTimeOn(FireTime &aFireTime)
{
    VerifyOrExit(mIsPresent);

    if (mAnnounceCounter < kNumberOfAnnounces)
    {
        aFireTime.SetFireTime(mAnnounceTime);
    }

    if (mMulticastAnswerPending)
    {
        aFireTime.SetFireTime(mAnswerTime);
    }

    if (mIsLastMulticastValid)
    {
        // `mLastMulticastTime` tracks the timestamp of the last
        // multicast of this record. To handle potential 32-bit
        // `TimeMilli` rollover, an aging mechanism is implemented.
        // If the record isn't multicast again within a given age
        // interval `kLastMulticastTimeAge`, `mIsLastMulticastValid`
        // is cleared, indicating outdated multicast information.

        TimeMilli lastMulticastAgeTime = mLastMulticastTime + kLastMulticastTimeAge;

        if (lastMulticastAgeTime <= TimerMilli::GetNow())
        {
            mIsLastMulticastValid = false;
        }
        else
        {
            aFireTime.SetFireTime(lastMulticastAgeTime);
        }
    }

exit:
    return;
}

void Core::RecordInfo::MarkAsAppended(TxMessage &aTxMessage, Section aSection)
{
    mAppendSection = aSection;

    switch (aTxMessage.GetType())
    {
    case TxMessage::kMulticastResponse:
    case TxMessage::kMulticastProbe:

        mAppendState = kAppendedInMulticastMsg;

        if ((aSection == kAnswerSection) || (aSection == kAdditionalDataSection))
        {
            mLastMulticastTime    = TimerMilli::GetNow();
            mIsLastMulticastValid = true;
        }

        break;

    case TxMessage::kUnicastResponse:
    case TxMessage::kLegacyUnicastResponse:
        mAppendState = kAppendedInUnicastMsg;
        break;

    case TxMessage::kMulticastQuery:
        break;
    }
}

void Core::RecordInfo::MarkToAppendInAdditionalData(void)
{
    if (mAppendState == kNotAppended)
    {
        mAppendState = kToAppendInAdditionalData;
    }
}

bool Core::RecordInfo::IsAppended(void) const
{
    bool isAppended = false;

    switch (mAppendState)
    {
    case kNotAppended:
    case kToAppendInAdditionalData:
        break;
    case kAppendedInMulticastMsg:
    case kAppendedInUnicastMsg:
        isAppended = true;
        break;
    }

    return isAppended;
}

bool Core::RecordInfo::CanAppend(void) const { return mIsPresent && !IsAppended(); }

Error Core::RecordInfo::GetLastMulticastTime(TimeMilli &aLastMulticastTime) const
{
    Error error = kErrorNotFound;

    VerifyOrExit(mIsPresent && mIsLastMulticastValid);
    aLastMulticastTime = mLastMulticastTime;

exit:
    return error;
}

uint32_t Core::RecordInfo::GetDurationSinceLastMulticast(TimeMilli aTime) const
{
    uint32_t duration = NumericLimits<uint32_t>::kMax;

    VerifyOrExit(mIsPresent && mIsLastMulticastValid);
    VerifyOrExit(aTime > mLastMulticastTime, duration = 0);
    duration = aTime - mLastMulticastTime;

exit:
    return duration;
}

//----------------------------------------------------------------------------------------------------------------------
// Core::FireTime

void Core::FireTime::SetFireTime(TimeMilli aFireTime)
{
    if (mHasFireTime)
    {
        VerifyOrExit(aFireTime < mFireTime);
    }

    mFireTime    = aFireTime;
    mHasFireTime = true;

exit:
    return;
}

void Core::FireTime::ScheduleFireTimeOn(TimerMilli &aTimer)
{
    if (mHasFireTime)
    {
        aTimer.FireAtIfEarlier(mFireTime);
    }
}

void Core::FireTime::UpdateNextFireTimeOn(NextFireTime &aNextFireTime) const
{
    if (mHasFireTime)
    {
        aNextFireTime.UpdateIfEarlier(mFireTime);
    }
}

//----------------------------------------------------------------------------------------------------------------------
// Core::Entry

Core::Entry::Entry(void)
    : mState(kProbing)
    , mProbeCount(0)
    , mMulticastNsecPending(false)
    , mUnicastNsecPending(false)
    , mAppendedNsec(false)
    , mBypassCallbackStateCheck(false)
{
}

void Core::Entry::Init(Instance &aInstance)
{
    // Initializes a newly allocated entry (host or service)
    // and starts it in `kProbing` state.

    InstanceLocatorInit::Init(aInstance);
    StartProbing();
}

void Core::Entry::SetState(State aState)
{
    mState = aState;
    ScheduleCallbackTask();
}

void Core::Entry::Register(const Key &aKey, const Callback &aCallback)
{
    if (GetState() == kRemoving)
    {
        StartProbing();
    }

    mKeyRecord.UpdateTtl(DetermineTtl(aKey.mTtl, kDefaultKeyTtl));
    mKeyRecord.UpdateProperty(mKeyData, aKey.mKeyData, aKey.mKeyDataLength);

    mKeyCallback = aCallback;
    ScheduleCallbackTask();
}

void Core::Entry::Unregister(const Key &aKey)
{
    OT_UNUSED_VARIABLE(aKey);

    VerifyOrExit(mKeyRecord.IsPresent());

    mKeyCallback.Clear();

    switch (GetState())
    {
    case kRegistered:
        mKeyRecord.UpdateTtl(0);
        break;

    case kProbing:
    case kConflict:
        ClearKey();
        break;

    case kRemoving:
        break;
    }

exit:
    return;
}

void Core::Entry::ClearKey(void)
{
    mKeyRecord.Clear();
    mKeyData.Free();
}

void Core::Entry::SetCallback(const Callback &aCallback)
{
    mCallback = aCallback;
    ScheduleCallbackTask();
}

void Core::Entry::MarkToInvokeCallbackUnconditionally(void)
{
    mBypassCallbackStateCheck = true;
    Get<Core>().mEntryTask.Post();
}

void Core::Entry::ScheduleCallbackTask(void)
{
    switch (GetState())
    {
    case kRegistered:
    case kConflict:
        VerifyOrExit(!mCallback.IsEmpty() || !mKeyCallback.IsEmpty());
        Get<Core>().mEntryTask.Post();
        break;

    case kProbing:
    case kRemoving:
        break;
    }

exit:
    return;
}

void Core::Entry::InvokeCallbacks(void)
{
    Error error = kErrorNone;

    // `mBypassCallbackStateCheck` is used when host is registered
    // with no address, which is treated as unregistering the host.
    // This ensures host registration callback is invoked properly.

    if (mBypassCallbackStateCheck)
    {
        mBypassCallbackStateCheck = false;
        mCallback.InvokeAndClear(GetInstance(), error);
    }

    switch (GetState())
    {
    case kConflict:
        error = kErrorDuplicated;
        OT_FALL_THROUGH;

    case kRegistered:
        mKeyCallback.InvokeAndClear(GetInstance(), error);
        mCallback.InvokeAndClear(GetInstance(), error);
        break;

    case kProbing:
    case kRemoving:
        break;
    }
}

void Core::Entry::StartProbing(void)
{
    SetState(kProbing);
    mProbeCount = 0;
    SetFireTime(Get<Core>().RandomizeFirstProbeTxTime());
    ScheduleTimer();
}

void Core::Entry::SetStateToConflict(void)
{
    switch (GetState())
    {
    case kProbing:
    case kRegistered:
        SetState(kConflict);
        break;
    case kConflict:
    case kRemoving:
        break;
    }
}

void Core::Entry::SetStateToRemoving(void)
{
    VerifyOrExit(GetState() != kRemoving);
    SetState(kRemoving);

exit:
    return;
}

void Core::Entry::ClearAppendState(void)
{
    mKeyRecord.MarkAsNotAppended();
    mAppendedNsec = false;
}

void Core::Entry::UpdateRecordsState(const TxMessage &aResponse)
{
    mKeyRecord.UpdateStateAfterAnswer(aResponse);

    if (mAppendedNsec)
    {
        switch (aResponse.GetType())
        {
        case TxMessage::kMulticastResponse:
            mMulticastNsecPending = false;
            break;
        case TxMessage::kUnicastResponse:
            mUnicastNsecPending = false;
            break;
        default:
            break;
        }
    }
}

void Core::Entry::ScheduleNsecAnswer(const AnswerInfo &aInfo)
{
    // Schedules NSEC record to be included in a response message.
    // Used to answer to query for a record that is not present.

    VerifyOrExit(GetState() == kRegistered);

    if (aInfo.mUnicastResponse)
    {
        mUnicastNsecPending = true;
    }
    else
    {
        if (mMulticastNsecPending)
        {
            VerifyOrExit(aInfo.mAnswerTime < mNsecAnswerTime);
        }

        mMulticastNsecPending = true;
        mNsecAnswerTime       = aInfo.mAnswerTime;
    }

exit:
    return;
}

bool Core::Entry::ShouldAnswerNsec(TimeMilli aNow) const { return mMulticastNsecPending && (mNsecAnswerTime <= aNow); }

void Core::Entry::AnswerNonProbe(const AnswerInfo &aInfo, RecordAndType *aRecords, uint16_t aRecordsLength)
{
    // Schedule answers for all matching records in `aRecords` array
    // to a given non-probe question.

    bool allEmptyOrZeroTtl = true;
    bool answerNsec        = true;

    for (uint16_t index = 0; index < aRecordsLength; index++)
    {
        RecordInfo &record = aRecords[index].mRecord;

        if (!record.CanAnswer())
        {
            // Cannot answer if record is not present or has zero TTL.
            continue;
        }

        allEmptyOrZeroTtl = false;

        if (QuestionMatches(aInfo.mQuestionRrType, aRecords[index].mType))
        {
            answerNsec = false;
            record.ScheduleAnswer(aInfo);
        }
    }

    // If all records are removed or have zero TTL (we are still
    // sending "Goodbye" announces), we should not provide any answer
    // even NSEC.

    if (!allEmptyOrZeroTtl && answerNsec)
    {
        ScheduleNsecAnswer(aInfo);
    }
}

void Core::Entry::AnswerProbe(const AnswerInfo &aInfo, RecordAndType *aRecords, uint16_t aRecordsLength)
{
    bool       allEmptyOrZeroTtl = true;
    bool       shouldDelay       = false;
    TimeMilli  now               = TimerMilli::GetNow();
    AnswerInfo info              = aInfo;

    info.mAnswerTime = now;

    OT_ASSERT(info.mIsProbe);

    for (uint16_t index = 0; index < aRecordsLength; index++)
    {
        RecordInfo &record = aRecords[index].mRecord;
        TimeMilli   lastMulticastTime;

        if (!record.CanAnswer())
        {
            continue;
        }

        allEmptyOrZeroTtl = false;

        if (!info.mUnicastResponse)
        {
            // Rate limiting multicast probe responses
            //
            // We delay the response if all records were multicast
            // recently within an interval `kMinIntervalProbeResponse`
            // (250 msec).

            if (record.GetDurationSinceLastMulticast(now) >= kMinIntervalProbeResponse)
            {
                shouldDelay = false;
            }
            else if (record.GetLastMulticastTime(lastMulticastTime) == kErrorNone)
            {
                info.mAnswerTime = Max(info.mAnswerTime, lastMulticastTime + kMinIntervalProbeResponse);
            }
        }
    }

    if (allEmptyOrZeroTtl)
    {
        // All records are removed or being removed.

        // Enhancement for future: If someone is probing for
        // our name, we can stop announcement of removed records
        // to let the new probe requester take over the name.

        ExitNow();
    }

    if (!shouldDelay)
    {
        info.mAnswerTime = now;
    }

    for (uint16_t index = 0; index < aRecordsLength; index++)
    {
        aRecords[index].mRecord.ScheduleAnswer(info);
    }

exit:
    return;
}

void Core::Entry::DetermineNextFireTime(void)
{
    mKeyRecord.UpdateFireTimeOn(*this);

    if (mMulticastNsecPending)
    {
        SetFireTime(mNsecAnswerTime);
    }
}

void Core::Entry::ScheduleTimer(void) { ScheduleFireTimeOn(Get<Core>().mEntryTimer); }

template <typename EntryType> void Core::Entry::HandleTimer(EntryTimerContext &aContext)
{
    EntryType *thisAsEntryType = static_cast<EntryType *>(this);

    thisAsEntryType->ClearAppendState();

    VerifyOrExit(HasFireTime());
    VerifyOrExit(GetFireTime() <= aContext.GetNow());
    ClearFireTime();

    switch (GetState())
    {
    case kProbing:
        if (mProbeCount < kNumberOfProbes)
        {
            mProbeCount++;
            SetFireTime(aContext.GetNow() + kProbeWaitTime);
            thisAsEntryType->PrepareProbe(aContext.GetProbeMessage());
            break;
        }

        SetState(kRegistered);
        thisAsEntryType->StartAnnouncing();

        OT_FALL_THROUGH;

    case kRegistered:
        thisAsEntryType->PrepareResponse(aContext.GetResponseMessage(), aContext.GetNow());
        break;

    case kConflict:
    case kRemoving:
        ExitNow();
    }
    thisAsEntryType->DetermineNextFireTime();

exit:
    UpdateNextFireTimeOn(aContext.GetNextFireTime());
}

void Core::Entry::AppendQuestionTo(TxMessage &aTxMessage) const
{
    Message &message = aTxMessage.SelectMessageFor(kQuestionSection);
    uint16_t rrClass = ResourceRecord::kClassInternet;
    Question question;

    if ((mProbeCount == 1) && Get<Core>().IsQuestionUnicastAllowed())
    {
        rrClass |= kClassQuestionUnicastFlag;
    }

    question.SetType(ResourceRecord::kTypeAny);
    question.SetClass(rrClass);
    SuccessOrAssert(message.Append(question));

    aTxMessage.IncrementRecordCount(kQuestionSection);
}

void Core::Entry::AppendKeyRecordTo(TxMessage &aTxMessage, Section aSection, NameAppender aNameAppender)
{
    Message       *message;
    ResourceRecord record;
    bool           isLegacyUnicast = (aTxMessage.GetType() == TxMessage::kLegacyUnicastResponse);

    VerifyOrExit(mKeyRecord.CanAppend());
    mKeyRecord.MarkAsAppended(aTxMessage, aSection);

    message = &aTxMessage.SelectMessageFor(aSection);

    // Use the `aNameAppender` function to allow sub-class
    // to append the proper name.

    aNameAppender(*this, aTxMessage, aSection);

    record.Init(ResourceRecord::kTypeKey);
    record.SetLength(mKeyData.GetLength());
    record.SetTtl(mKeyRecord.GetTtl(isLegacyUnicast));
    UpdateCacheFlushFlagIn(record, aSection, isLegacyUnicast);

    SuccessOrAssert(message->Append(record));
    SuccessOrAssert(message->AppendBytes(mKeyData.GetBytes(), mKeyData.GetLength()));

    aTxMessage.IncrementRecordCount(aSection);

exit:
    return;
}

void Core::Entry::AppendNsecRecordTo(TxMessage       &aTxMessage,
                                     Section          aSection,
                                     const TypeArray &aTypes,
                                     NameAppender     aNameAppender)
{
    Message               &message = aTxMessage.SelectMessageFor(aSection);
    NsecRecord             nsec;
    NsecRecord::TypeBitMap bitmap;
    uint16_t               offset;
    bool                   isLegacyUnicast = (aTxMessage.GetType() == TxMessage::kLegacyUnicastResponse);

    nsec.Init();
    nsec.SetTtl(isLegacyUnicast ? kLegacyUnicastNsecTtl : kNsecTtl);
    UpdateCacheFlushFlagIn(nsec, aSection, isLegacyUnicast);

    bitmap.Clear();

    for (uint16_t type : aTypes)
    {
        bitmap.AddType(type);
    }

    aNameAppender(*this, aTxMessage, aSection);

    offset = message.GetLength();
    SuccessOrAssert(message.Append(nsec));

    // Next Domain Name (should be same as record name).
    aNameAppender(*this, aTxMessage, aSection);

    SuccessOrAssert(message.AppendBytes(&bitmap, bitmap.GetSize()));

    UpdateRecordLengthInMessage(nsec, message, offset);
    aTxMessage.IncrementRecordCount(aSection);

    mAppendedNsec = true;
}

Error Core::Entry::CopyKeyInfoTo(Key &aKey, EntryState &aState) const
{
    Error error = kErrorNone;

    VerifyOrExit(mKeyRecord.IsPresent(), error = kErrorNotFound);

    aKey.mKeyData       = mKeyData.GetBytes();
    aKey.mKeyDataLength = mKeyData.GetLength();
    aKey.mClass         = ResourceRecord::kClassInternet;
    aKey.mTtl           = mKeyRecord.GetTtl();
    aKey.mInfraIfIndex  = Get<Core>().mInfraIfIndex;
    aState              = static_cast<EntryState>(GetState());

exit:
    return error;
}

//----------------------------------------------------------------------------------------------------------------------
// Core::HostEntry

Core::HostEntry::HostEntry(void)
    : mNext(nullptr)
    , mNameOffset(kUnspecifiedOffset)
{
}

Error Core::HostEntry::Init(Instance &aInstance, const char *aName)
{
    Entry::Init(aInstance);

    return mName.Set(aName);
}

bool Core::HostEntry::Matches(const Name &aName) const
{
    return aName.Matches(/* aFirstLabel */ nullptr, mName.AsCString(), kLocalDomain);
}

bool Core::HostEntry::Matches(const Host &aHost) const { return NameMatch(mName, aHost.mHostName); }

bool Core::HostEntry::Matches(const Key &aKey) const { return !IsKeyForService(aKey) && NameMatch(mName, aKey.mName); }

bool Core::HostEntry::Matches(const Heap::String &aName) const { return NameMatch(mName, aName); }

bool Core::HostEntry::IsEmpty(void) const { return !mAddrRecord.IsPresent() && !mKeyRecord.IsPresent(); }

void Core::HostEntry::Register(const Host &aHost, const Callback &aCallback)
{
    if (GetState() == kRemoving)
    {
        StartProbing();
    }

    SetCallback(aCallback);

    if (aHost.mAddressesLength == 0)
    {
        // If host is registered with no addresses, treat it
        // as host being unregistered and announce removal of
        // the old addresses.

        Unregister(aHost);

        // Set the callback again as `Unregister()` may clear it.
        // Also mark to invoke the callback unconditionally (bypassing
        // entry state check). The callback will be invoked
        // after returning from this method from the posted tasklet.

        SetCallback(aCallback);
        MarkToInvokeCallbackUnconditionally();
        ExitNow();
    }

    mAddrRecord.UpdateTtl(DetermineTtl(aHost.mTtl, kDefaultTtl));
    mAddrRecord.UpdateProperty(mAddresses, AsCoreTypePtr(aHost.mAddresses), aHost.mAddressesLength);

    DetermineNextFireTime();
    ScheduleTimer();

exit:
    return;
}

void Core::HostEntry::Register(const Key &aKey, const Callback &aCallback)
{
    Entry::Register(aKey, aCallback);

    DetermineNextFireTime();
    ScheduleTimer();
}

void Core::HostEntry::Unregister(const Host &aHost)
{
    OT_UNUSED_VARIABLE(aHost);

    VerifyOrExit(mAddrRecord.IsPresent());

    ClearCallback();

    switch (GetState())
    {
    case kRegistered:
        mAddrRecord.UpdateTtl(0);
        DetermineNextFireTime();
        ScheduleTimer();
        break;

    case kProbing:
    case kConflict:
        ClearHost();
        ScheduleToRemoveIfEmpty();
        break;

    case kRemoving:
        break;
    }

exit:
    return;
}

void Core::HostEntry::Unregister(const Key &aKey)
{
    Entry::Unregister(aKey);

    DetermineNextFireTime();
    ScheduleTimer();

    ScheduleToRemoveIfEmpty();
}

void Core::HostEntry::ClearHost(void)
{
    mAddrRecord.Clear();
    mAddresses.Free();
}

void Core::HostEntry::ScheduleToRemoveIfEmpty(void)
{
    if (IsEmpty())
    {
        SetStateToRemoving();
        Get<Core>().mEntryTask.Post();
    }
}

void Core::HostEntry::HandleConflict(void)
{
    State oldState = GetState();

    SetStateToConflict();
    VerifyOrExit(oldState == kRegistered);
    Get<Core>().InvokeConflictCallback(mName.AsCString(), nullptr);

exit:
    return;
}

void Core::HostEntry::AnswerQuestion(const AnswerInfo &aInfo)
{
    RecordAndType records[] = {
        {mAddrRecord, ResourceRecord::kTypeAaaa},
        {mKeyRecord, ResourceRecord::kTypeKey},
    };

    VerifyOrExit(GetState() == kRegistered);

    if (aInfo.mIsProbe)
    {
        AnswerProbe(aInfo, records, GetArrayLength(records));
    }
    else
    {
        AnswerNonProbe(aInfo, records, GetArrayLength(records));
    }

    DetermineNextFireTime();
    ScheduleTimer();

exit:
    return;
}

void Core::HostEntry::HandleTimer(EntryTimerContext &aContext) { Entry::HandleTimer<HostEntry>(aContext); }

void Core::HostEntry::ClearAppendState(void)
{
    // Clears `HostEntry` records and all tracked saved name
    // compression offsets.

    Entry::ClearAppendState();

    mAddrRecord.MarkAsNotAppended();

    mNameOffset = kUnspecifiedOffset;
}

void Core::HostEntry::PrepareProbe(TxMessage &aProbe)
{
    bool prepareAgain = false;

    do
    {
        aProbe.SaveCurrentState();

        AppendNameTo(aProbe, kQuestionSection);
        AppendQuestionTo(aProbe);

        AppendAddressRecordsTo(aProbe, kAuthoritySection);
        AppendKeyRecordTo(aProbe, kAuthoritySection);

        aProbe.CheckSizeLimitToPrepareAgain(prepareAgain);

    } while (prepareAgain);
}

void Core::HostEntry::StartAnnouncing(void)
{
    mAddrRecord.StartAnnouncing();
    mKeyRecord.StartAnnouncing();
}

void Core::HostEntry::PrepareResponse(TxMessage &aResponse, TimeMilli aNow)
{
    bool prepareAgain = false;

    do
    {
        aResponse.SaveCurrentState();
        PrepareResponseRecords(aResponse, aNow);
        aResponse.CheckSizeLimitToPrepareAgain(prepareAgain);

    } while (prepareAgain);

    UpdateRecordsState(aResponse);
}

void Core::HostEntry::PrepareResponseRecords(TxMessage &aResponse, TimeMilli aNow)
{
    bool appendNsec = false;

    if (mAddrRecord.ShouldAppendTo(aResponse, aNow))
    {
        AppendAddressRecordsTo(aResponse, kAnswerSection);
        appendNsec = true;
    }

    if (mKeyRecord.ShouldAppendTo(aResponse, aNow))
    {
        AppendKeyRecordTo(aResponse, kAnswerSection);
        appendNsec = true;
    }

    if (appendNsec || ShouldAnswerNsec(aNow))
    {
        AppendNsecRecordTo(aResponse, kAdditionalDataSection);
    }
}

void Core::HostEntry::UpdateRecordsState(const TxMessage &aResponse)
{
    // Updates state after a response is prepared.

    Entry::UpdateRecordsState(aResponse);
    mAddrRecord.UpdateStateAfterAnswer(aResponse);

    if (IsEmpty())
    {
        SetStateToRemoving();
    }
}

void Core::HostEntry::DetermineNextFireTime(void)
{
    VerifyOrExit(GetState() == kRegistered);

    Entry::DetermineNextFireTime();
    mAddrRecord.UpdateFireTimeOn(*this);

exit:
    return;
}

void Core::HostEntry::AppendAddressRecordsTo(TxMessage &aTxMessage, Section aSection)
{
    Message *message;
    bool     isLegacyUnicast = (aTxMessage.GetType() == TxMessage::kLegacyUnicastResponse);

    VerifyOrExit(mAddrRecord.CanAppend());
    mAddrRecord.MarkAsAppended(aTxMessage, aSection);

    message = &aTxMessage.SelectMessageFor(aSection);

    for (const Ip6::Address &address : mAddresses)
    {
        AaaaRecord aaaaRecord;

        aaaaRecord.Init();
        aaaaRecord.SetAddress(address);
        aaaaRecord.SetTtl(mAddrRecord.GetTtl(isLegacyUnicast));
        UpdateCacheFlushFlagIn(aaaaRecord, aSection, isLegacyUnicast);

        AppendNameTo(aTxMessage, aSection);
        SuccessOrAssert(message->Append(aaaaRecord));

        aTxMessage.IncrementRecordCount(aSection);
    }

exit:
    return;
}

void Core::HostEntry::AppendKeyRecordTo(TxMessage &aTxMessage, Section aSection)
{
    Entry::AppendKeyRecordTo(aTxMessage, aSection, &AppendEntryName);
}

void Core::HostEntry::AppendNsecRecordTo(TxMessage &aTxMessage, Section aSection)
{
    TypeArray types;

    if (mAddrRecord.IsPresent() && (mAddrRecord.GetTtl() > 0))
    {
        types.Add(ResourceRecord::kTypeAaaa);
    }

    if (mKeyRecord.IsPresent() && (mKeyRecord.GetTtl() > 0))
    {
        types.Add(ResourceRecord::kTypeKey);
    }

    if (!types.IsEmpty())
    {
        Entry::AppendNsecRecordTo(aTxMessage, aSection, types, &AppendEntryName);
    }
}

void Core::HostEntry::AppendEntryName(Entry &aEntry, TxMessage &aTxMessage, Section aSection)
{
    static_cast<HostEntry &>(aEntry).AppendNameTo(aTxMessage, aSection);
}

void Core::HostEntry::AppendNameTo(TxMessage &aTxMessage, Section aSection)
{
    AppendOutcome outcome;

    outcome = aTxMessage.AppendMultipleLabels(aSection, mName.AsCString(), mNameOffset);
    VerifyOrExit(outcome != kAppendedFullNameAsCompressed);

    aTxMessage.AppendDomainName(aSection);

exit:
    return;
}

#if OPENTHREAD_CONFIG_MULTICAST_DNS_ENTRY_ITERATION_API_ENABLE

Error Core::HostEntry::CopyInfoTo(Host &aHost, EntryState &aState) const
{
    Error error = kErrorNone;

    VerifyOrExit(mAddrRecord.IsPresent(), error = kErrorNotFound);

    aHost.mHostName        = mName.AsCString();
    aHost.mAddresses       = mAddresses.AsCArray();
    aHost.mAddressesLength = mAddresses.GetLength();
    aHost.mTtl             = mAddrRecord.GetTtl();
    aHost.mInfraIfIndex    = Get<Core>().mInfraIfIndex;
    aState                 = static_cast<EntryState>(GetState());

exit:
    return error;
}

Error Core::HostEntry::CopyInfoTo(Key &aKey, EntryState &aState) const
{
    Error error;

    SuccessOrExit(error = CopyKeyInfoTo(aKey, aState));

    aKey.mName        = mName.AsCString();
    aKey.mServiceType = nullptr;

exit:
    return error;
}

#endif // OPENTHREAD_CONFIG_MULTICAST_DNS_ENTRY_ITERATION_API_ENABLE

//----------------------------------------------------------------------------------------------------------------------
// Core::ServiceEntry

const uint8_t Core::ServiceEntry::kEmptyTxtData[] = {0};

Core::ServiceEntry::ServiceEntry(void)
    : mNext(nullptr)
    , mPriority(0)
    , mWeight(0)
    , mPort(0)
    , mServiceNameOffset(kUnspecifiedOffset)
    , mServiceTypeOffset(kUnspecifiedOffset)
    , mSubServiceTypeOffset(kUnspecifiedOffset)
    , mHostNameOffset(kUnspecifiedOffset)
    , mIsAddedInServiceTypes(false)
{
}

Error Core::ServiceEntry::Init(Instance &aInstance, const char *aServiceInstance, const char *aServiceType)
{
    Error error;

    Entry::Init(aInstance);

    SuccessOrExit(error = mServiceInstance.Set(aServiceInstance));
    SuccessOrExit(error = mServiceType.Set(aServiceType));

exit:
    return error;
}

Error Core::ServiceEntry::Init(Instance &aInstance, const Service &aService)
{
    return Init(aInstance, aService.mServiceInstance, aService.mServiceType);
}

Error Core::ServiceEntry::Init(Instance &aInstance, const Key &aKey)
{
    return Init(aInstance, aKey.mName, aKey.mServiceType);
}

bool Core::ServiceEntry::Matches(const Name &aFullName) const
{
    return aFullName.Matches(mServiceInstance.AsCString(), mServiceType.AsCString(), kLocalDomain);
}

bool Core::ServiceEntry::MatchesServiceType(const Name &aServiceType) const
{
    // When matching service type, PTR record should be
    // present with non-zero TTL (checked by `CanAnswer()`).

    return mPtrRecord.CanAnswer() && aServiceType.Matches(nullptr, mServiceType.AsCString(), kLocalDomain);
}

bool Core::ServiceEntry::Matches(const Service &aService) const
{
    return NameMatch(mServiceInstance, aService.mServiceInstance) && NameMatch(mServiceType, aService.mServiceType);
}

bool Core::ServiceEntry::Matches(const Key &aKey) const
{
    return IsKeyForService(aKey) && NameMatch(mServiceInstance, aKey.mName) &&
           NameMatch(mServiceType, aKey.mServiceType);
}

bool Core::ServiceEntry::IsEmpty(void) const { return !mPtrRecord.IsPresent() && !mKeyRecord.IsPresent(); }

bool Core::ServiceEntry::CanAnswerSubType(const char *aSubLabel) const
{
    bool           canAnswer = false;
    const SubType *subType;

    VerifyOrExit(mPtrRecord.CanAnswer());

    subType = mSubTypes.FindMatching(aSubLabel);
    VerifyOrExit(subType != nullptr);

    canAnswer = subType->mPtrRecord.CanAnswer();

exit:
    return canAnswer;
}

void Core::ServiceEntry::Register(const Service &aService, const Callback &aCallback)
{
    uint32_t ttl = DetermineTtl(aService.mTtl, kDefaultTtl);

    if (GetState() == kRemoving)
    {
        StartProbing();
    }

    SetCallback(aCallback);

    // Register sub-types PTRs.

    // First we check for any removed sub-types. We keep removed
    // sub-types marked with zero TTL so to announce their removal
    // before fully removing them from the list.

    for (SubType &subType : mSubTypes)
    {
        uint32_t subTypeTtl = subType.IsContainedIn(aService) ? ttl : 0;

        subType.mPtrRecord.UpdateTtl(subTypeTtl);
    }

    // Next we add any new sub-types in `aService`.

    for (uint16_t i = 0; i < aService.mSubTypeLabelsLength; i++)
    {
        const char *label = aService.mSubTypeLabels[i];

        if (!mSubTypes.ContainsMatching(label))
        {
            SubType *newSubType = SubType::AllocateAndInit(label);

            OT_ASSERT(newSubType != nullptr);
            mSubTypes.Push(*newSubType);

            newSubType->mPtrRecord.UpdateTtl(ttl);
        }
    }

    // Register base PTR service.

    mPtrRecord.UpdateTtl(ttl);

    // Register SRV record info.

    mSrvRecord.UpdateTtl(ttl);
    mSrvRecord.UpdateProperty(mHostName, aService.mHostName);
    mSrvRecord.UpdateProperty(mPriority, aService.mPriority);
    mSrvRecord.UpdateProperty(mWeight, aService.mWeight);
    mSrvRecord.UpdateProperty(mPort, aService.mPort);

    // Register TXT record info.

    mTxtRecord.UpdateTtl(ttl);

    if ((aService.mTxtData == nullptr) || (aService.mTxtDataLength == 0))
    {
        mTxtRecord.UpdateProperty(mTxtData, kEmptyTxtData, sizeof(kEmptyTxtData));
    }
    else
    {
        mTxtRecord.UpdateProperty(mTxtData, aService.mTxtData, aService.mTxtDataLength);
    }

    UpdateServiceTypes();

    DetermineNextFireTime();
    ScheduleTimer();
}

void Core::ServiceEntry::Register(const Key &aKey, const Callback &aCallback)
{
    Entry::Register(aKey, aCallback);

    DetermineNextFireTime();
    ScheduleTimer();
}

void Core::ServiceEntry::Unregister(const Service &aService)
{
    OT_UNUSED_VARIABLE(aService);

    VerifyOrExit(mPtrRecord.IsPresent());

    ClearCallback();

    switch (GetState())
    {
    case kRegistered:
        for (SubType &subType : mSubTypes)
        {
            subType.mPtrRecord.UpdateTtl(0);
        }

        mPtrRecord.UpdateTtl(0);
        mSrvRecord.UpdateTtl(0);
        mTxtRecord.UpdateTtl(0);
        DetermineNextFireTime();
        ScheduleTimer();
        break;

    case kProbing:
    case kConflict:
        ClearService();
        ScheduleToRemoveIfEmpty();
        break;

    case kRemoving:
        break;
    }

    UpdateServiceTypes();

exit:
    return;
}

void Core::ServiceEntry::Unregister(const Key &aKey)
{
    Entry::Unregister(aKey);

    DetermineNextFireTime();
    ScheduleTimer();

    ScheduleToRemoveIfEmpty();
}

void Core::ServiceEntry::ClearService(void)
{
    mPtrRecord.Clear();
    mSrvRecord.Clear();
    mTxtRecord.Clear();
    mSubTypes.Free();
    mHostName.Free();
    mTxtData.Free();
}

void Core::ServiceEntry::ScheduleToRemoveIfEmpty(void)
{
    mSubTypes.RemoveAndFreeAllMatching(EmptyChecker());

    if (IsEmpty())
    {
        SetStateToRemoving();
        Get<Core>().mEntryTask.Post();
    }
}

void Core::ServiceEntry::HandleConflict(void)
{
    State oldState = GetState();

    SetStateToConflict();
    UpdateServiceTypes();

    VerifyOrExit(oldState == kRegistered);
    Get<Core>().InvokeConflictCallback(mServiceInstance.AsCString(), mServiceType.AsCString());

exit:
    return;
}

void Core::ServiceEntry::AnswerServiceNameQuestion(const AnswerInfo &aInfo)
{
    RecordAndType records[] = {
        {mSrvRecord, ResourceRecord::kTypeSrv},
        {mTxtRecord, ResourceRecord::kTypeTxt},
        {mKeyRecord, ResourceRecord::kTypeKey},
    };

    VerifyOrExit(GetState() == kRegistered);

    if (aInfo.mIsProbe)
    {
        AnswerProbe(aInfo, records, GetArrayLength(records));
    }
    else
    {
        AnswerNonProbe(aInfo, records, GetArrayLength(records));
    }

    DetermineNextFireTime();
    ScheduleTimer();

exit:
    return;
}

void Core::ServiceEntry::AnswerServiceTypeQuestion(const AnswerInfo &aInfo, const char *aSubLabel)
{
    VerifyOrExit(GetState() == kRegistered);

    if (aSubLabel == nullptr)
    {
        mPtrRecord.ScheduleAnswer(aInfo);
    }
    else
    {
        SubType *subType = mSubTypes.FindMatching(aSubLabel);

        VerifyOrExit(subType != nullptr);
        subType->mPtrRecord.ScheduleAnswer(aInfo);
    }

    DetermineNextFireTime();
    ScheduleTimer();

exit:
    return;
}

bool Core::ServiceEntry::ShouldSuppressKnownAnswer(uint32_t aTtl, const char *aSubLabel) const
{
    // Check `aTtl` of a matching record in known-answer section of
    // a query with the corresponding PTR record's TTL and suppress
    // answer if it is at least at least half the correct value.

    bool     shouldSuppress = false;
    uint32_t ttl;

    if (aSubLabel == nullptr)
    {
        ttl = mPtrRecord.GetTtl();
    }
    else
    {
        const SubType *subType = mSubTypes.FindMatching(aSubLabel);

        VerifyOrExit(subType != nullptr);
        ttl = subType->mPtrRecord.GetTtl();
    }

    shouldSuppress = (aTtl > ttl / 2);

exit:
    return shouldSuppress;
}

void Core::ServiceEntry::HandleTimer(EntryTimerContext &aContext) { Entry::HandleTimer<ServiceEntry>(aContext); }

void Core::ServiceEntry::ClearAppendState(void)
{
    // Clear the append state for all `ServiceEntry` records,
    // along with all tracked name compression offsets.

    Entry::ClearAppendState();

    mPtrRecord.MarkAsNotAppended();
    mSrvRecord.MarkAsNotAppended();
    mTxtRecord.MarkAsNotAppended();

    mServiceNameOffset    = kUnspecifiedOffset;
    mServiceTypeOffset    = kUnspecifiedOffset;
    mSubServiceTypeOffset = kUnspecifiedOffset;
    mHostNameOffset       = kUnspecifiedOffset;

    for (SubType &subType : mSubTypes)
    {
        subType.mPtrRecord.MarkAsNotAppended();
        subType.mSubServiceNameOffset = kUnspecifiedOffset;
    }
}

void Core::ServiceEntry::PrepareProbe(TxMessage &aProbe)
{
    bool prepareAgain = false;

    do
    {
        HostEntry *hostEntry = nullptr;

        aProbe.SaveCurrentState();

        DiscoverOffsetsAndHost(hostEntry);

        AppendServiceNameTo(aProbe, kQuestionSection);
        AppendQuestionTo(aProbe);

        // Append records (if present) in authority section

        AppendSrvRecordTo(aProbe, kAuthoritySection);
        AppendTxtRecordTo(aProbe, kAuthoritySection);
        AppendKeyRecordTo(aProbe, kAuthoritySection);

        aProbe.CheckSizeLimitToPrepareAgain(prepareAgain);

    } while (prepareAgain);
}

void Core::ServiceEntry::StartAnnouncing(void)
{
    for (SubType &subType : mSubTypes)
    {
        subType.mPtrRecord.StartAnnouncing();
    }

    mPtrRecord.StartAnnouncing();
    mSrvRecord.StartAnnouncing();
    mTxtRecord.StartAnnouncing();
    mKeyRecord.StartAnnouncing();

    UpdateServiceTypes();
}

void Core::ServiceEntry::PrepareResponse(TxMessage &aResponse, TimeMilli aNow)
{
    bool prepareAgain = false;

    do
    {
        aResponse.SaveCurrentState();
        PrepareResponseRecords(aResponse, aNow);
        aResponse.CheckSizeLimitToPrepareAgain(prepareAgain);

    } while (prepareAgain);

    UpdateRecordsState(aResponse);
}

void Core::ServiceEntry::PrepareResponseRecords(TxMessage &aResponse, TimeMilli aNow)
{
    bool       appendNsec                    = false;
    bool       appendAdditionalRecordsForPtr = false;
    HostEntry *hostEntry                     = nullptr;

    DiscoverOffsetsAndHost(hostEntry);

    // We determine records to include in Additional Data section
    // per RFC 6763 section 12:
    //
    // - For PTR (base or sub-type), we include SRV, TXT, and host
    //   addresses.
    // - For SRV, we include host addresses only (TXT record not
    //   recommended).
    //
    // Records already appended in Answer section are excluded from
    // Additional Data. Host Entries are processed before Service
    // Entries which ensures address inclusion accuracy.
    // `MarkToAppendInAdditionalData()` marks a record for potential
    // Additional Data inclusion, but this is skipped if the record
    // is already appended in the Answer section.

    if (mPtrRecord.ShouldAppendTo(aResponse, aNow))
    {
        AppendPtrRecordTo(aResponse, kAnswerSection);

        if (mPtrRecord.GetTtl() > 0)
        {
            appendAdditionalRecordsForPtr = true;
        }
    }

    for (SubType &subType : mSubTypes)
    {
        if (subType.mPtrRecord.ShouldAppendTo(aResponse, aNow))
        {
            AppendPtrRecordTo(aResponse, kAnswerSection, &subType);

            if (subType.mPtrRecord.GetTtl() > 0)
            {
                appendAdditionalRecordsForPtr = true;
            }
        }
    }

    if (appendAdditionalRecordsForPtr)
    {
        mSrvRecord.MarkToAppendInAdditionalData();
        mTxtRecord.MarkToAppendInAdditionalData();

        if (hostEntry != nullptr)
        {
            hostEntry->mAddrRecord.MarkToAppendInAdditionalData();
        }
    }

    if (mSrvRecord.ShouldAppendTo(aResponse, aNow))
    {
        AppendSrvRecordTo(aResponse, kAnswerSection);
        appendNsec = true;

        if ((mSrvRecord.GetTtl() > 0) && (hostEntry != nullptr))
        {
            hostEntry->mAddrRecord.MarkToAppendInAdditionalData();
        }
    }

    if (mTxtRecord.ShouldAppendTo(aResponse, aNow))
    {
        AppendTxtRecordTo(aResponse, kAnswerSection);
        appendNsec = true;
    }

    if (mKeyRecord.ShouldAppendTo(aResponse, aNow))
    {
        AppendKeyRecordTo(aResponse, kAnswerSection);
        appendNsec = true;
    }

    // Append records in Additional Data section

    if (mSrvRecord.ShouldAppendInAdditionalDataSection())
    {
        AppendSrvRecordTo(aResponse, kAdditionalDataSection);
    }

    if (mTxtRecord.ShouldAppendInAdditionalDataSection())
    {
        AppendTxtRecordTo(aResponse, kAdditionalDataSection);
    }

    if ((hostEntry != nullptr) && (hostEntry->mAddrRecord.ShouldAppendInAdditionalDataSection()))
    {
        hostEntry->AppendAddressRecordsTo(aResponse, kAdditionalDataSection);
    }

    if (appendNsec || ShouldAnswerNsec(aNow))
    {
        AppendNsecRecordTo(aResponse, kAdditionalDataSection);
    }
}

void Core::ServiceEntry::UpdateRecordsState(const TxMessage &aResponse)
{
    Entry::UpdateRecordsState(aResponse);

    mPtrRecord.UpdateStateAfterAnswer(aResponse);
    mSrvRecord.UpdateStateAfterAnswer(aResponse);
    mTxtRecord.UpdateStateAfterAnswer(aResponse);

    for (SubType &subType : mSubTypes)
    {
        subType.mPtrRecord.UpdateStateAfterAnswer(aResponse);
    }

    mSubTypes.RemoveAndFreeAllMatching(EmptyChecker());

    if (IsEmpty())
    {
        SetStateToRemoving();
    }
}

void Core::ServiceEntry::DetermineNextFireTime(void)
{
    VerifyOrExit(GetState() == kRegistered);

    Entry::DetermineNextFireTime();

    mPtrRecord.UpdateFireTimeOn(*this);
    mSrvRecord.UpdateFireTimeOn(*this);
    mTxtRecord.UpdateFireTimeOn(*this);

    for (SubType &subType : mSubTypes)
    {
        subType.mPtrRecord.UpdateFireTimeOn(*this);
    }

exit:
    return;
}

void Core::ServiceEntry::DiscoverOffsetsAndHost(HostEntry *&aHostEntry)
{
    // Discovers the `HostEntry` associated with this `ServiceEntry`
    // and name compression offsets from the previously appended
    // entries.

    aHostEntry = Get<Core>().mHostEntries.FindMatching(mHostName);

    if ((aHostEntry != nullptr) && (aHostEntry->GetState() != GetState()))
    {
        aHostEntry = nullptr;
    }

    if (aHostEntry != nullptr)
    {
        UpdateCompressOffset(mHostNameOffset, aHostEntry->mNameOffset);
    }

    for (ServiceEntry &other : Get<Core>().mServiceEntries)
    {
        // We only need to search up to `this` entry in the list,
        // since entries after `this` are not yet processed and not
        // yet appended in the response or the probe message.

        if (&other == this)
        {
            break;
        }

        if (other.GetState() != GetState())
        {
            // Validate that both entries are in the same state,
            // ensuring their records are appended in the same
            // message, i.e., a probe or a response message.

            continue;
        }

        if (NameMatch(mHostName, other.mHostName))
        {
            UpdateCompressOffset(mHostNameOffset, other.mHostNameOffset);
        }

        if (NameMatch(mServiceType, other.mServiceType))
        {
            UpdateCompressOffset(mServiceTypeOffset, other.mServiceTypeOffset);

            if (GetState() == kProbing)
            {
                // No need to search for sub-type service offsets when
                // we are still probing.

                continue;
            }

            UpdateCompressOffset(mSubServiceTypeOffset, other.mSubServiceTypeOffset);

            for (SubType &subType : mSubTypes)
            {
                const SubType *otherSubType = other.mSubTypes.FindMatching(subType.mLabel.AsCString());

                if (otherSubType != nullptr)
                {
                    UpdateCompressOffset(subType.mSubServiceNameOffset, otherSubType->mSubServiceNameOffset);
                }
            }
        }
    }
}

void Core::ServiceEntry::UpdateServiceTypes(void)
{
    // This method updates the `mServiceTypes` list adding or
    // removing this `ServiceEntry` info.
    //
    // It is called whenever the `ServiceEntry` state gets changed
    // or a PTR record is added or removed. The service is valid
    // when entry is registered and we have a PTR with non-zero
    // TTL.

    bool         shouldAdd = (GetState() == kRegistered) && mPtrRecord.CanAnswer();
    ServiceType *serviceType;

    VerifyOrExit(shouldAdd != mIsAddedInServiceTypes);

    mIsAddedInServiceTypes = shouldAdd;

    serviceType = Get<Core>().mServiceTypes.FindMatching(mServiceType);

    if (shouldAdd && (serviceType == nullptr))
    {
        serviceType = ServiceType::AllocateAndInit(GetInstance(), mServiceType.AsCString());
        OT_ASSERT(serviceType != nullptr);
        Get<Core>().mServiceTypes.Push(*serviceType);
    }

    VerifyOrExit(serviceType != nullptr);

    if (shouldAdd)
    {
        serviceType->IncrementNumEntries();
    }
    else
    {
        serviceType->DecrementNumEntries();

        if (serviceType->GetNumEntries() == 0)
        {
            // If there are no more `ServiceEntry` with
            // this service type, we remove the it from
            // the `mServiceTypes` list. It is safe to
            // remove here as this method will never be
            // called while we are iterating over the
            // `mServiceTypes` list.

            Get<Core>().mServiceTypes.RemoveMatching(*serviceType);
        }
    }

exit:
    return;
}

void Core::ServiceEntry::AppendSrvRecordTo(TxMessage &aTxMessage, Section aSection)
{
    Message  *message;
    SrvRecord srv;
    uint16_t  offset;
    bool      isLegacyUnicast = (aTxMessage.GetType() == TxMessage::kLegacyUnicastResponse);

    VerifyOrExit(mSrvRecord.CanAppend());
    mSrvRecord.MarkAsAppended(aTxMessage, aSection);

    message = &aTxMessage.SelectMessageFor(aSection);

    srv.Init();
    srv.SetPriority(mPriority);
    srv.SetWeight(mWeight);
    srv.SetPort(mPort);
    srv.SetTtl(mSrvRecord.GetTtl(isLegacyUnicast));
    UpdateCacheFlushFlagIn(srv, aSection, isLegacyUnicast);

    // RFC6762, Section 18.14 Name Compression:
    // In legacy unicast responses generated to answer legacy queries, name
    // compression MUST NOT be performed on SRV records.
    AppendServiceNameTo(aTxMessage, aSection, /* aPerformNameCompression */ !isLegacyUnicast);

    offset = message->GetLength();
    SuccessOrAssert(message->Append(srv));
    AppendHostNameTo(aTxMessage, aSection);
    UpdateRecordLengthInMessage(srv, *message, offset);

    aTxMessage.IncrementRecordCount(aSection);

exit:
    return;
}

void Core::ServiceEntry::AppendTxtRecordTo(TxMessage &aTxMessage, Section aSection)
{
    Message  *message;
    TxtRecord txt;
    bool      isLegacyUnicast = (aTxMessage.GetType() == TxMessage::kLegacyUnicastResponse);

    VerifyOrExit(mTxtRecord.CanAppend());
    mTxtRecord.MarkAsAppended(aTxMessage, aSection);

    message = &aTxMessage.SelectMessageFor(aSection);

    txt.Init();
    txt.SetLength(mTxtData.GetLength());
    txt.SetTtl(mTxtRecord.GetTtl(isLegacyUnicast));
    UpdateCacheFlushFlagIn(txt, aSection, isLegacyUnicast);

    AppendServiceNameTo(aTxMessage, aSection);
    SuccessOrAssert(message->Append(txt));
    SuccessOrAssert(message->AppendBytes(mTxtData.GetBytes(), mTxtData.GetLength()));

    aTxMessage.IncrementRecordCount(aSection);

exit:
    return;
}

void Core::ServiceEntry::AppendPtrRecordTo(TxMessage &aTxMessage, Section aSection, SubType *aSubType)
{
    // Appends PTR record for base service (when `aSubType == nullptr`) or
    // for the given `aSubType`.

    Message    *message;
    RecordInfo &ptrRecord = (aSubType == nullptr) ? mPtrRecord : aSubType->mPtrRecord;
    PtrRecord   ptr;
    uint16_t    offset;
    bool        isLegacyUnicast = (aTxMessage.GetType() == TxMessage::kLegacyUnicastResponse);

    VerifyOrExit(ptrRecord.CanAppend());
    ptrRecord.MarkAsAppended(aTxMessage, aSection);

    message = &aTxMessage.SelectMessageFor(aSection);

    ptr.Init();
    ptr.SetTtl(ptrRecord.GetTtl(isLegacyUnicast));

    if (aSubType == nullptr)
    {
        AppendServiceTypeTo(aTxMessage, aSection);
    }
    else
    {
        AppendSubServiceNameTo(aTxMessage, aSection, *aSubType);
    }

    offset = message->GetLength();
    SuccessOrAssert(message->Append(ptr));
    AppendServiceNameTo(aTxMessage, aSection);
    UpdateRecordLengthInMessage(ptr, *message, offset);

    aTxMessage.IncrementRecordCount(aSection);

exit:
    return;
}

void Core::ServiceEntry::AppendKeyRecordTo(TxMessage &aTxMessage, Section aSection)
{
    Entry::AppendKeyRecordTo(aTxMessage, aSection, &AppendEntryName);
}

void Core::ServiceEntry::AppendNsecRecordTo(TxMessage &aTxMessage, Section aSection)
{
    TypeArray types;

    if (mSrvRecord.IsPresent() && (mSrvRecord.GetTtl() > 0))
    {
        types.Add(ResourceRecord::kTypeSrv);
    }

    if (mTxtRecord.IsPresent() && (mTxtRecord.GetTtl() > 0))
    {
        types.Add(ResourceRecord::kTypeTxt);
    }

    if (mKeyRecord.IsPresent() && (mKeyRecord.GetTtl() > 0))
    {
        types.Add(ResourceRecord::kTypeKey);
    }

    if (!types.IsEmpty())
    {
        Entry::AppendNsecRecordTo(aTxMessage, aSection, types, &AppendEntryName);
    }
}

void Core::ServiceEntry::AppendEntryName(Entry &aEntry, TxMessage &aTxMessage, Section aSection)
{
    static_cast<ServiceEntry &>(aEntry).AppendServiceNameTo(aTxMessage, aSection);
}

void Core::ServiceEntry::AppendServiceNameTo(TxMessage &aTxMessage, Section aSection, bool aPerformNameCompression)
{
    AppendOutcome outcome;

    if (!aPerformNameCompression)
    {
        uint16_t compressOffset = kUnspecifiedOffset;

        outcome = aTxMessage.AppendLabel(aSection, mServiceInstance.AsCString(), compressOffset);
        VerifyOrExit(outcome == kAppendedLabels);
    }
    else
    {
        outcome = aTxMessage.AppendLabel(aSection, mServiceInstance.AsCString(), mServiceNameOffset);
        VerifyOrExit(outcome != kAppendedFullNameAsCompressed);
    }

    AppendServiceTypeTo(aTxMessage, aSection);

exit:
    return;
}

void Core::ServiceEntry::AppendServiceTypeTo(TxMessage &aTxMessage, Section aSection)
{
    aTxMessage.AppendServiceType(aSection, mServiceType.AsCString(), mServiceTypeOffset);
}

void Core::ServiceEntry::AppendSubServiceTypeTo(TxMessage &aTxMessage, Section aSection)
{
    AppendOutcome outcome;

    outcome = aTxMessage.AppendLabel(aSection, kSubServiceLabel, mSubServiceTypeOffset);
    VerifyOrExit(outcome != kAppendedFullNameAsCompressed);

    AppendServiceTypeTo(aTxMessage, aSection);

exit:
    return;
}

void Core::ServiceEntry::AppendSubServiceNameTo(TxMessage &aTxMessage, Section aSection, SubType &aSubType)
{
    AppendOutcome outcome;

    outcome = aTxMessage.AppendLabel(aSection, aSubType.mLabel.AsCString(), aSubType.mSubServiceNameOffset);
    VerifyOrExit(outcome != kAppendedFullNameAsCompressed);

    AppendSubServiceTypeTo(aTxMessage, aSection);

exit:
    return;
}

void Core::ServiceEntry::AppendHostNameTo(TxMessage &aTxMessage, Section aSection)
{
    AppendOutcome outcome;

    outcome = aTxMessage.AppendMultipleLabels(aSection, mHostName.AsCString(), mHostNameOffset);
    VerifyOrExit(outcome != kAppendedFullNameAsCompressed);

    aTxMessage.AppendDomainName(aSection);

exit:
    return;
}

#if OPENTHREAD_CONFIG_MULTICAST_DNS_ENTRY_ITERATION_API_ENABLE

Error Core::ServiceEntry::CopyInfoTo(Service &aService, EntryState &aState, EntryIterator &aIterator) const
{
    Error error = kErrorNone;

    VerifyOrExit(mPtrRecord.IsPresent(), error = kErrorNotFound);

    aIterator.mSubTypeArray.Free();

    for (const SubType &subType : mSubTypes)
    {
        SuccessOrAssert(aIterator.mSubTypeArray.PushBack(subType.mLabel.AsCString()));
    }

    aService.mHostName            = mHostName.AsCString();
    aService.mServiceInstance     = mServiceInstance.AsCString();
    aService.mServiceType         = mServiceType.AsCString();
    aService.mSubTypeLabels       = aIterator.mSubTypeArray.AsCArray();
    aService.mSubTypeLabelsLength = aIterator.mSubTypeArray.GetLength();
    aService.mTxtData             = mTxtData.GetBytes();
    aService.mTxtDataLength       = mTxtData.GetLength();
    aService.mPort                = mPort;
    aService.mPriority            = mPriority;
    aService.mWeight              = mWeight;
    aService.mTtl                 = mPtrRecord.GetTtl();
    aService.mInfraIfIndex        = Get<Core>().mInfraIfIndex;
    aState                        = static_cast<EntryState>(GetState());

exit:
    return error;
}

Error Core::ServiceEntry::CopyInfoTo(Key &aKey, EntryState &aState) const
{
    Error error;

    SuccessOrExit(error = CopyKeyInfoTo(aKey, aState));

    aKey.mName        = mServiceInstance.AsCString();
    aKey.mServiceType = mServiceType.AsCString();

exit:
    return error;
}

#endif // OPENTHREAD_CONFIG_MULTICAST_DNS_ENTRY_ITERATION_API_ENABLE

//----------------------------------------------------------------------------------------------------------------------
// Core::ServiceEntry::SubType

Error Core::ServiceEntry::SubType::Init(const char *aLabel)
{
    mSubServiceNameOffset = kUnspecifiedOffset;

    return mLabel.Set(aLabel);
}

bool Core::ServiceEntry::SubType::Matches(const EmptyChecker &aChecker) const
{
    OT_UNUSED_VARIABLE(aChecker);

    return !mPtrRecord.IsPresent();
}

bool Core::ServiceEntry::SubType::IsContainedIn(const Service &aService) const
{
    bool contains = false;

    for (uint16_t i = 0; i < aService.mSubTypeLabelsLength; i++)
    {
        if (NameMatch(mLabel, aService.mSubTypeLabels[i]))
        {
            contains = true;
            break;
        }
    }

    return contains;
}

//----------------------------------------------------------------------------------------------------------------------
// Core::ServiceType

Error Core::ServiceType::Init(Instance &aInstance, const char *aServiceType)
{
    Error error;

    InstanceLocatorInit::Init(aInstance);

    mNext       = nullptr;
    mNumEntries = 0;
    SuccessOrExit(error = mServiceType.Set(aServiceType));

    mServicesPtr.UpdateTtl(kServicesPtrTtl);
    mServicesPtr.StartAnnouncing();

    mServicesPtr.UpdateFireTimeOn(*this);
    ScheduleFireTimeOn(Get<Core>().mEntryTimer);

exit:
    return error;
}

bool Core::ServiceType::Matches(const Name &aServiceTypeName) const
{
    return aServiceTypeName.Matches(/* aFirstLabel */ nullptr, mServiceType.AsCString(), kLocalDomain);
}

bool Core::ServiceType::Matches(const Heap::String &aServiceType) const
{
    return NameMatch(aServiceType, mServiceType);
}

void Core::ServiceType::ClearAppendState(void) { mServicesPtr.MarkAsNotAppended(); }

void Core::ServiceType::AnswerQuestion(const AnswerInfo &aInfo)
{
    VerifyOrExit(mServicesPtr.CanAnswer());
    mServicesPtr.ScheduleAnswer(aInfo);
    mServicesPtr.UpdateFireTimeOn(*this);
    ScheduleFireTimeOn(Get<Core>().mEntryTimer);

exit:
    return;
}

bool Core::ServiceType::ShouldSuppressKnownAnswer(uint32_t aTtl) const
{
    // Check `aTtl` of a matching record in known-answer section of
    // a query with the corresponding PTR record's TTL and suppress
    // answer if it is at least at least half the correct value.

    return (aTtl > mServicesPtr.GetTtl() / 2);
}

void Core::ServiceType::HandleTimer(EntryTimerContext &aContext)
{
    ClearAppendState();

    VerifyOrExit(HasFireTime());
    VerifyOrExit(GetFireTime() <= aContext.GetNow());
    ClearFireTime();

    PrepareResponse(aContext.GetResponseMessage(), aContext.GetNow());

    mServicesPtr.UpdateFireTimeOn(*this);

exit:
    UpdateNextFireTimeOn(aContext.GetNextFireTime());
}

void Core::ServiceType::PrepareResponse(TxMessage &aResponse, TimeMilli aNow)
{
    bool prepareAgain = false;

    do
    {
        aResponse.SaveCurrentState();
        PrepareResponseRecords(aResponse, aNow);
        aResponse.CheckSizeLimitToPrepareAgain(prepareAgain);

    } while (prepareAgain);

    mServicesPtr.UpdateStateAfterAnswer(aResponse);
}

void Core::ServiceType::PrepareResponseRecords(TxMessage &aResponse, TimeMilli aNow)
{
    uint16_t serviceTypeOffset = kUnspecifiedOffset;

    VerifyOrExit(mServicesPtr.ShouldAppendTo(aResponse, aNow));

    // Discover compress offset for `mServiceType` if previously
    // appended from any `ServiceEntry`.

    for (const ServiceEntry &serviceEntry : Get<Core>().mServiceEntries)
    {
        if (serviceEntry.GetState() != Entry::kRegistered)
        {
            continue;
        }

        if (NameMatch(mServiceType, serviceEntry.mServiceType))
        {
            UpdateCompressOffset(serviceTypeOffset, serviceEntry.mServiceTypeOffset);

            if (serviceTypeOffset != kUnspecifiedOffset)
            {
                break;
            }
        }
    }

    AppendPtrRecordTo(aResponse, serviceTypeOffset);

exit:
    return;
}

void Core::ServiceType::AppendPtrRecordTo(TxMessage &aResponse, uint16_t aServiceTypeOffset)
{
    Message  *message;
    PtrRecord ptr;
    uint16_t  offset;

    VerifyOrExit(mServicesPtr.CanAppend());
    mServicesPtr.MarkAsAppended(aResponse, kAnswerSection);

    message = &aResponse.SelectMessageFor(kAnswerSection);

    ptr.Init();
    if (aResponse.GetType() == TxMessage::kLegacyUnicastResponse)
    {
        ptr.SetTtl(Min(Core::RecordInfo::kMaxLegacyUnicastTtl, mServicesPtr.GetTtl()));
    }
    else
    {
        ptr.SetTtl(mServicesPtr.GetTtl());
    }

    aResponse.AppendServicesDnssdName(kAnswerSection);
    offset = message->GetLength();
    SuccessOrAssert(message->Append(ptr));
    aResponse.AppendServiceType(kAnswerSection, mServiceType.AsCString(), aServiceTypeOffset);
    UpdateRecordLengthInMessage(ptr, *message, offset);

    aResponse.IncrementRecordCount(kAnswerSection);

exit:
    return;
}

//----------------------------------------------------------------------------------------------------------------------
// Core::TxMessage

Core::TxMessage::TxMessage(Instance &aInstance, Type aType, uint16_t aQueryId)
    : InstanceLocator(aInstance)
{
    Init(aType, aQueryId);
}

Core::TxMessage::TxMessage(Instance &aInstance, Type aType, const AddressInfo &aUnicastDest, uint16_t aQueryId)
    : TxMessage(aInstance, aType, aQueryId)
{
    mUnicastDest = aUnicastDest;
}

void Core::TxMessage::Init(Type aType, uint16_t aMessageId)
{
    Header header;

    mRecordCounts.Clear();
    mSavedRecordCounts.Clear();
    mSavedMsgLength      = 0;
    mSavedExtraMsgLength = 0;
    mDomainOffset        = kUnspecifiedOffset;
    mUdpOffset           = kUnspecifiedOffset;
    mTcpOffset           = kUnspecifiedOffset;
    mServicesDnssdOffset = kUnspecifiedOffset;
    mType                = aType;

    // Allocate messages. The main `mMsgPtr` is always allocated.
    // The Authority and Addition section messages are allocated
    // the first time they are used.

    mMsgPtr.Reset(Get<MessagePool>().Allocate(Message::kTypeOther));
    OT_ASSERT(!mMsgPtr.IsNull());

    mExtraMsgPtr.Reset();

    header.Clear();

    switch (aType)
    {
    case kMulticastProbe:
    case kMulticastQuery:
        header.SetType(Header::kTypeQuery);
        break;
    case kMulticastResponse:
    case kUnicastResponse:
    case kLegacyUnicastResponse:
        header.SetType(Header::kTypeResponse);
        header.SetMessageId(aMessageId);
        break;
    }

    SuccessOrAssert(mMsgPtr->Append(header));
}

Message &Core::TxMessage::SelectMessageFor(Section aSection)
{
    // Selects the `Message` to use for a given `aSection` based
    // the message type.

    Message *message      = nullptr;
    Section  mainSection  = kAnswerSection;
    Section  extraSection = kAdditionalDataSection;

    switch (mType)
    {
    case kMulticastProbe:
        mainSection  = kQuestionSection;
        extraSection = kAuthoritySection;
        break;

    case kMulticastQuery:
        mainSection  = kQuestionSection;
        extraSection = kAnswerSection;
        break;
    case kLegacyUnicastResponse:
    case kUnicastResponse:
    case kMulticastResponse:
        break;
    }

    if (aSection == mainSection)
    {
        message = mMsgPtr.Get();
    }
    else if (aSection == extraSection)
    {
        if (mExtraMsgPtr.IsNull())
        {
            mExtraMsgPtr.Reset(Get<MessagePool>().Allocate(Message::kTypeOther));
            OT_ASSERT(!mExtraMsgPtr.IsNull());
        }

        message = mExtraMsgPtr.Get();
    }

    OT_ASSERT(message != nullptr);

    return *message;
}

Core::AppendOutcome Core::TxMessage::AppendLabel(Section aSection, const char *aLabel, uint16_t &aCompressOffset)
{
    return AppendLabels(aSection, aLabel, kIsSingleLabel, aCompressOffset);
}

Core::AppendOutcome Core::TxMessage::AppendMultipleLabels(Section     aSection,
                                                          const char *aLabels,
                                                          uint16_t   &aCompressOffset)
{
    return AppendLabels(aSection, aLabels, !kIsSingleLabel, aCompressOffset);
}

Core::AppendOutcome Core::TxMessage::AppendLabels(Section     aSection,
                                                  const char *aLabels,
                                                  bool        aIsSingleLabel,
                                                  uint16_t   &aCompressOffset)
{
    // Appends DNS name label(s) to the message in the specified section,
    // using compression if possible.
    //
    // - If a valid `aCompressOffset` is given (indicating name was appended before)
    //   a compressed pointer label is used, and `kAppendedFullNameAsCompressed`
    //   is returned.
    // - Otherwise, `aLabels` is appended, `aCompressOffset` is also updated for
    //   future compression, and `kAppendedLabels` is returned.
    //
    // `aIsSingleLabel` indicates that `aLabels` string should be appended
    // as a single label. This is useful for service instance label which
    // can itself contain the dot `.` character.

    AppendOutcome outcome = kAppendedLabels;
    Message      &message = SelectMessageFor(aSection);

    if (aCompressOffset != kUnspecifiedOffset)
    {
        SuccessOrAssert(Name::AppendPointerLabel(aCompressOffset, message));
        outcome = kAppendedFullNameAsCompressed;
        ExitNow();
    }

    SaveOffset(aCompressOffset, message, aSection);

    if (aIsSingleLabel)
    {
        SuccessOrAssert(Name::AppendLabel(aLabels, message));
    }
    else
    {
        SuccessOrAssert(Name::AppendMultipleLabels(aLabels, message));
    }

exit:
    return outcome;
}

void Core::TxMessage::AppendServiceType(Section aSection, const char *aServiceType, uint16_t &aCompressOffset)
{
    // Appends DNS service type name to the message in the specified
    // section, using compression if possible.

    const char   *serviceLabels = aServiceType;
    bool          isUdp         = false;
    bool          isTcp         = false;
    Name::Buffer  labelsBuffer;
    AppendOutcome outcome;

    if (Name::ExtractLabels(serviceLabels, kUdpServiceLabel, labelsBuffer) == kErrorNone)
    {
        isUdp         = true;
        serviceLabels = labelsBuffer;
    }
    else if (Name::ExtractLabels(serviceLabels, kTcpServiceLabel, labelsBuffer) == kErrorNone)
    {
        isTcp         = true;
        serviceLabels = labelsBuffer;
    }

    outcome = AppendMultipleLabels(aSection, serviceLabels, aCompressOffset);
    VerifyOrExit(outcome != kAppendedFullNameAsCompressed);

    if (isUdp)
    {
        outcome = AppendLabel(aSection, kUdpServiceLabel, mUdpOffset);
    }
    else if (isTcp)
    {
        outcome = AppendLabel(aSection, kTcpServiceLabel, mTcpOffset);
    }

    VerifyOrExit(outcome != kAppendedFullNameAsCompressed);

    AppendDomainName(aSection);

exit:
    return;
}

void Core::TxMessage::AppendDomainName(Section aSection)
{
    Message &message = SelectMessageFor(aSection);

    if (mDomainOffset != kUnspecifiedOffset)
    {
        SuccessOrAssert(Name::AppendPointerLabel(mDomainOffset, message));
        ExitNow();
    }

    SaveOffset(mDomainOffset, message, aSection);
    SuccessOrAssert(Name::AppendName(kLocalDomain, message));

exit:
    return;
}

void Core::TxMessage::AppendServicesDnssdName(Section aSection)
{
    Message &message = SelectMessageFor(aSection);

    if (mServicesDnssdOffset != kUnspecifiedOffset)
    {
        SuccessOrAssert(Name::AppendPointerLabel(mServicesDnssdOffset, message));
        ExitNow();
    }

    SaveOffset(mServicesDnssdOffset, message, aSection);
    SuccessOrAssert(Name::AppendMultipleLabels(kServicesDnssdLabels, message));
    AppendDomainName(aSection);

exit:
    return;
}

void Core::TxMessage::AddQuestionFrom(const Message &aMessage)
{
    uint16_t offset = sizeof(Header);

    IgnoreError(Name::ParseName(aMessage, offset));
    offset += sizeof(ot::Dns::Question);
    SuccessOrAssert(mMsgPtr->AppendBytesFromMessage(aMessage, sizeof(Header), offset - sizeof(Header)));
    IncrementRecordCount(kQuestionSection);
}

void Core::TxMessage::SaveOffset(uint16_t &aCompressOffset, const Message &aMessage, Section aSection)
{
    // Saves the current message offset in `aCompressOffset` for name
    // compression, but only when appending to the question or answer
    // sections.
    //
    // This is necessary because other sections use separate message,
    // and their offsets can shift when records are added to the main
    // message.
    //
    // While current record types guarantee name inclusion in
    // question/answer sections before their use in other sections,
    // this check allows future extensions.

    switch (aSection)
    {
    case kQuestionSection:
    case kAnswerSection:
        aCompressOffset = aMessage.GetLength();
        break;

    case kAuthoritySection:
    case kAdditionalDataSection:
        break;
    }
}

bool Core::TxMessage::IsOverSizeLimit(void) const
{
    uint32_t size = mMsgPtr->GetLength();

    if (!mExtraMsgPtr.IsNull())
    {
        size += mExtraMsgPtr->GetLength();
    }

    return (size > Get<Core>().mMaxMessageSize);
}

void Core::TxMessage::SaveCurrentState(void)
{
    mSavedRecordCounts   = mRecordCounts;
    mSavedMsgLength      = mMsgPtr->GetLength();
    mSavedExtraMsgLength = mExtraMsgPtr.IsNull() ? 0 : mExtraMsgPtr->GetLength();
}

void Core::TxMessage::RestoreToSavedState(void)
{
    mRecordCounts = mSavedRecordCounts;

    IgnoreError(mMsgPtr->SetLength(mSavedMsgLength));

    if (!mExtraMsgPtr.IsNull())
    {
        IgnoreError(mExtraMsgPtr->SetLength(mSavedExtraMsgLength));
    }
}

void Core::TxMessage::CheckSizeLimitToPrepareAgain(bool &aPrepareAgain)
{
    // Manages message size limits by re-preparing messages when
    // necessary:
    // - Checks if `TxMessage` exceeds the size limit.
    // - If so, restores the `TxMessage` to its previously saved
    //   state, sends it, and re-initializes it which will also
    //   clear the "AppendState" of the related host and service
    //   entries to ensure correct re-processing.
    // - Sets `aPrepareAgain` to `true` to signal that records should
    //   be prepared and added to the new message.
    //
    // We allow the `aPrepareAgain` to happen once. The very unlikely
    // case where the `Entry` itself has so many records that its
    // contents exceed the message size limit, is not handled, i.e.
    // we always include all records of a single `Entry` within the same
    // message. In future, the code can be updated to allow truncated
    // messages.

    if (aPrepareAgain)
    {
        aPrepareAgain = false;
        ExitNow();
    }

    VerifyOrExit(IsOverSizeLimit());

    aPrepareAgain = true;

    RestoreToSavedState();
    Send();
    Reinit();

exit:
    return;
}

void Core::TxMessage::Send(void)
{
    static constexpr uint16_t kHeaderOffset = 0;

    Header header;

    VerifyOrExit(!mRecordCounts.IsEmpty());

    SuccessOrAssert(mMsgPtr->Read(kHeaderOffset, header));
    mRecordCounts.WriteTo(header);
    mMsgPtr->Write(kHeaderOffset, header);

    if (!mExtraMsgPtr.IsNull())
    {
        SuccessOrAssert(mMsgPtr->AppendBytesFromMessage(*mExtraMsgPtr, 0, mExtraMsgPtr->GetLength()));
    }

    Get<Core>().mTxMessageHistory.Add(*mMsgPtr);

    // We pass ownership of message to the platform layer.

    switch (mType)
    {
    case kMulticastProbe:
    case kMulticastQuery:
    case kMulticastResponse:
        otPlatMdnsSendMulticast(&GetInstance(), mMsgPtr.Release(), Get<Core>().mInfraIfIndex);
        break;

    case kUnicastResponse:
    case kLegacyUnicastResponse:
        otPlatMdnsSendUnicast(&GetInstance(), mMsgPtr.Release(), &mUnicastDest);
        break;
    }

exit:
    return;
}

void Core::TxMessage::Reinit(void)
{
    Init(GetType());

    // After re-initializing `TxMessage`, we clear the "AppendState"
    // on all related host and service entries, and service types
    // or all cache entries (depending on the `GetType()`).

    switch (GetType())
    {
    case kMulticastProbe:
    case kMulticastResponse:
    case kUnicastResponse:
        for (HostEntry &entry : Get<Core>().mHostEntries)
        {
            if (ShouldClearAppendStateOnReinit(entry))
            {
                entry.ClearAppendState();
            }
        }

        for (ServiceEntry &entry : Get<Core>().mServiceEntries)
        {
            if (ShouldClearAppendStateOnReinit(entry))
            {
                entry.ClearAppendState();
            }
        }

        for (ServiceType &serviceType : Get<Core>().mServiceTypes)
        {
            if ((GetType() == kMulticastResponse) || (GetType() == kUnicastResponse))
            {
                serviceType.ClearAppendState();
            }
        }

        break;

    case kMulticastQuery:

        for (BrowseCache &browseCache : Get<Core>().mBrowseCacheList)
        {
            browseCache.ClearCompressOffsets();
        }

        for (SrvCache &srvCache : Get<Core>().mSrvCacheList)
        {
            srvCache.ClearCompressOffsets();
        }

        for (TxtCache &txtCache : Get<Core>().mTxtCacheList)
        {
            txtCache.ClearCompressOffsets();
        }

        // `Ip6AddrCache` entries do not track any append state or
        // compress offset since the host name should not be used
        // in any other query question.

        break;
    case kLegacyUnicastResponse:
        break;
    }
}

bool Core::TxMessage::ShouldClearAppendStateOnReinit(const Entry &aEntry) const
{
    // Determines whether we should clear "append state" on `aEntry`
    // when re-initializing the `TxMessage`. If message is a probe, we
    // check that entry is in `kProbing` state, if message is a
    // unicast/multicast response, we check for `kRegistered` state.

    bool shouldClear = false;

    switch (aEntry.GetState())
    {
    case Entry::kProbing:
        shouldClear = (GetType() == kMulticastProbe);
        break;

    case Entry::kRegistered:
        shouldClear = (GetType() == kMulticastResponse) || (GetType() == kUnicastResponse);
        break;

    case Entry::kConflict:
    case Entry::kRemoving:
        shouldClear = true;
        break;
    }

    return shouldClear;
}

//----------------------------------------------------------------------------------------------------------------------
// Core::EntryTimerContext

Core::EntryTimerContext::EntryTimerContext(Instance &aInstance)
    : InstanceLocator(aInstance)
    , mProbeMessage(aInstance, TxMessage::kMulticastProbe)
    , mResponseMessage(aInstance, TxMessage::kMulticastResponse)
{
}

//----------------------------------------------------------------------------------------------------------------------
// Core::RxMessage

Error Core::RxMessage::Init(Instance          &aInstance,
                            OwnedPtr<Message> &aMessagePtr,
                            bool               aIsUnicast,
                            const AddressInfo &aSenderAddress)
{
    static const Section kSections[] = {kAnswerSection, kAuthoritySection, kAdditionalDataSection};

    Error    error = kErrorNone;
    Header   header;
    uint16_t offset;
    uint16_t numRecords;

    InstanceLocatorInit::Init(aInstance);

    mNext = nullptr;

    VerifyOrExit(!aMessagePtr.IsNull(), error = kErrorInvalidArgs);

    offset = aMessagePtr->GetOffset();

    SuccessOrExit(error = aMessagePtr->Read(offset, header));
    offset += sizeof(Header);

    // RFC 6762 Section 18: Query type (OPCODE) must be zero
    // (standard query). All other flags must be ignored. Messages
    // with non-zero RCODE MUST be silently ignored.

    VerifyOrExit(header.GetQueryType() == Header::kQueryTypeStandard, error = kErrorParse);
    VerifyOrExit(header.GetResponseCode() == Header::kResponseSuccess, error = kErrorParse);

    mIsQuery       = (header.GetType() == Header::kTypeQuery);
    mIsUnicast     = aIsUnicast;
    mTruncated     = header.IsTruncationFlagSet();
    mSenderAddress = aSenderAddress;

    if (aSenderAddress.mPort != kUdpPort)
    {
        // Simple DNS resolver does not allow more than one question in a query message
        if (mIsQuery && header.GetQuestionCount() == 1)
        {
            // Section 6.7 Legacy Unicast
            mIsLegacyUnicast = true;
            mQueryId         = header.GetMessageId();
        }
        else
        {
            // The source port in a response MUST be mDNS port.
            // Otherwise response message MUST be silently ignored.

            ExitNow(error = kErrorParse);
        }
    }

    if (mIsUnicast && mIsQuery)
    {
        // Direct Unicast Queries to Port 5353 (RFC 6762 - section 5.5).
        // Responders SHOULD check that the source address in the query
        // packet matches the local subnet for that link and silently ignore
        // the packet if not.

        LogInfo("We do not yet support unicast query to mDNS port");
        ExitNow(error = kErrorNotCapable);
    }

    mRecordCounts.ReadFrom(header);

    // Parse questions

    mStartOffset[kQuestionSection] = offset;

    SuccessOrAssert(mQuestions.ReserveCapacity(mRecordCounts.GetFor(kQuestionSection)));

    for (numRecords = mRecordCounts.GetFor(kQuestionSection); numRecords > 0; numRecords--)
    {
        Question         *question = mQuestions.PushBack();
        ot::Dns::Question record;
        uint16_t          rrClass;

        OT_ASSERT(question != nullptr);

        question->mNameOffset = offset;

        SuccessOrExit(error = Name::ParseName(*aMessagePtr, offset));
        SuccessOrExit(error = aMessagePtr->Read(offset, record));
        offset += sizeof(record);

        question->mRrType = record.GetType();

        rrClass                      = record.GetClass();
        question->mUnicastResponse   = rrClass & kClassQuestionUnicastFlag;
        question->mIsRrClassInternet = RrClassIsInternetOrAny(rrClass);
    }

    // Parse and validate records in Answer, Authority and Additional
    // Data sections.

    for (Section section : kSections)
    {
        mStartOffset[section] = offset;
        SuccessOrExit(error = ResourceRecord::ParseRecords(*aMessagePtr, offset, mRecordCounts.GetFor(section)));
    }

    // Determine which questions are probes by searching in the
    // Authority section for records matching the question name.

    for (Question &question : mQuestions)
    {
        Name name(*aMessagePtr, question.mNameOffset);

        offset     = mStartOffset[kAuthoritySection];
        numRecords = mRecordCounts.GetFor(kAuthoritySection);

        if (ResourceRecord::FindRecord(*aMessagePtr, offset, numRecords, name) == kErrorNone)
        {
            question.mIsProbe = true;
        }
    }

    mIsSelfOriginating = Get<Core>().mTxMessageHistory.Contains(*aMessagePtr);

    mMessagePtr = aMessagePtr.PassOwnership();

exit:
    if (error != kErrorNone)
    {
        LogInfo("Failed to parse message from %s, error:%s", aSenderAddress.GetAddress().ToString().AsCString(),
                ErrorToString(error));
    }

    return error;
}

void Core::RxMessage::ClearProcessState(void)
{
    for (Question &question : mQuestions)
    {
        question.ClearProcessState();
    }
}

Core::RxMessage::ProcessOutcome Core::RxMessage::ProcessQuery(bool aShouldProcessTruncated)
{
    ProcessOutcome outcome             = kProcessed;
    bool           shouldDelay         = false;
    bool           canAnswer           = false;
    bool           needUnicastResponse = false;
    TimeMilli      answerTime;

    for (Question &question : mQuestions)
    {
        question.ClearProcessState();

        ProcessQuestion(question);

        // Check if we can answer every question in the query and all
        // answers are for unique records (where we own the name). This
        // determines whether we need to add any random delay before
        // responding.

        if (!question.mCanAnswer || !question.mIsUnique)
        {
            shouldDelay = true;
        }

        if (question.mCanAnswer)
        {
            canAnswer = true;

            if (question.mUnicastResponse || mIsLegacyUnicast)
            {
                needUnicastResponse = true;
            }
        }
    }

    if (mIsLegacyUnicast)
    {
        shouldDelay = false;
    }

    VerifyOrExit(canAnswer);

    if (mTruncated && !aShouldProcessTruncated)
    {
        outcome = kSaveAsMultiPacket;
        ExitNow();
    }

    answerTime = TimerMilli::GetNow();

    if (shouldDelay)
    {
        answerTime += Random::NonCrypto::GetUint32InRange(kMinResponseDelay, kMaxResponseDelay);
    }

    for (const Question &question : mQuestions)
    {
        AnswerQuestion(question, answerTime);
    }

    if (needUnicastResponse)
    {
        SendUnicastResponse(mSenderAddress);
    }

exit:
    return outcome;
}

void Core::RxMessage::ProcessQuestion(Question &aQuestion)
{
    Name name(*mMessagePtr, aQuestion.mNameOffset);

    VerifyOrExit(aQuestion.mIsRrClassInternet);

    // Check if question name matches "_services._dns-sd._udp" (all services)

    if (name.Matches(/* aFirstLabel */ nullptr, kServicesDnssdLabels, kLocalDomain))
    {
        VerifyOrExit(QuestionMatches(aQuestion.mRrType, ResourceRecord::kTypePtr));
        VerifyOrExit(!Get<Core>().mServiceTypes.IsEmpty());

        aQuestion.mCanAnswer             = true;
        aQuestion.mIsForAllServicesDnssd = true;

        ExitNow();
    }

    // Check if question name matches a `HostEntry` or a `ServiceEntry`

    aQuestion.mEntry = Get<Core>().mHostEntries.FindMatching(name);

    if (aQuestion.mEntry == nullptr)
    {
        aQuestion.mEntry        = Get<Core>().mServiceEntries.FindMatching(name);
        aQuestion.mIsForService = (aQuestion.mEntry != nullptr);
    }

    if (aQuestion.mEntry != nullptr)
    {
        switch (aQuestion.mEntry->GetState())
        {
        case Entry::kProbing:
            if (aQuestion.mIsProbe)
            {
                // Handling probe conflicts deviates from RFC 6762.
                // We allow the conflict to happen and report it
                // let the caller handle it. In future, TSR can
                // help select the winner.
            }
            break;

        case Entry::kRegistered:
            aQuestion.mCanAnswer = true;
            aQuestion.mIsUnique  = true;
            break;

        case Entry::kConflict:
        case Entry::kRemoving:
            break;
        }
    }
    else
    {
        // Check if question matches a service type or sub-type. We
        // can answer PTR or ANY questions. There may be multiple
        // service entries matching the question. We find and save
        // the first match. `AnswerServiceTypeQuestion()` will start
        // from the saved entry and finds all the other matches.

        bool              isSubType;
        Name::LabelBuffer subLabel;
        Name              baseType;

        VerifyOrExit(QuestionMatches(aQuestion.mRrType, ResourceRecord::kTypePtr));

        isSubType = ParseQuestionNameAsSubType(aQuestion, subLabel, baseType);

        if (!isSubType)
        {
            baseType = name;
        }

        for (ServiceEntry &serviceEntry : Get<Core>().mServiceEntries)
        {
            if ((serviceEntry.GetState() != Entry::kRegistered) || !serviceEntry.MatchesServiceType(baseType))
            {
                continue;
            }

            if (isSubType && !serviceEntry.CanAnswerSubType(subLabel))
            {
                continue;
            }

            aQuestion.mCanAnswer     = true;
            aQuestion.mEntry         = &serviceEntry;
            aQuestion.mIsForService  = true;
            aQuestion.mIsServiceType = true;
            ExitNow();
        }
    }

exit:
    return;
}

void Core::RxMessage::AnswerQuestion(const Question &aQuestion, TimeMilli aAnswerTime)
{
    HostEntry    *hostEntry;
    ServiceEntry *serviceEntry;
    AnswerInfo    answerInfo;

    VerifyOrExit(aQuestion.mCanAnswer);

    answerInfo.mQuestionRrType        = aQuestion.mRrType;
    answerInfo.mAnswerTime            = aAnswerTime;
    answerInfo.mIsProbe               = aQuestion.mIsProbe;
    answerInfo.mUnicastResponse       = aQuestion.mUnicastResponse;
    answerInfo.mLegacyUnicastResponse = mIsLegacyUnicast;

    if (aQuestion.mIsForAllServicesDnssd)
    {
        AnswerAllServicesQuestion(aQuestion, answerInfo);
        ExitNow();
    }

    hostEntry    = aQuestion.mIsForService ? nullptr : static_cast<HostEntry *>(aQuestion.mEntry);
    serviceEntry = aQuestion.mIsForService ? static_cast<ServiceEntry *>(aQuestion.mEntry) : nullptr;

    if (hostEntry != nullptr)
    {
        hostEntry->AnswerQuestion(answerInfo);
        ExitNow();
    }

    // Question is for `ServiceEntry`

    VerifyOrExit(serviceEntry != nullptr);

    if (!aQuestion.mIsServiceType)
    {
        serviceEntry->AnswerServiceNameQuestion(answerInfo);
    }
    else
    {
        AnswerServiceTypeQuestion(aQuestion, answerInfo, *serviceEntry);
    }

exit:
    return;
}

void Core::RxMessage::AnswerServiceTypeQuestion(const Question   &aQuestion,
                                                const AnswerInfo &aInfo,
                                                ServiceEntry     &aFirstEntry)
{
    Name              serviceType(*mMessagePtr, aQuestion.mNameOffset);
    Name              baseType;
    Name::LabelBuffer labelBuffer;
    const char       *subLabel;

    if (ParseQuestionNameAsSubType(aQuestion, labelBuffer, baseType))
    {
        subLabel = labelBuffer;
    }
    else
    {
        baseType = serviceType;
        subLabel = nullptr;
    }

    for (ServiceEntry *serviceEntry = &aFirstEntry; serviceEntry != nullptr; serviceEntry = serviceEntry->GetNext())
    {
        bool shouldSuppress = false;

        if ((serviceEntry->GetState() != Entry::kRegistered) || !serviceEntry->MatchesServiceType(baseType))
        {
            continue;
        }

        if ((subLabel != nullptr) && !serviceEntry->CanAnswerSubType(subLabel))
        {
            continue;
        }

        // Check for known-answer in this `RxMessage` and all its
        // related messages in case it is multi-packet query.

        for (const RxMessage *rxMessage = this; rxMessage != nullptr; rxMessage = rxMessage->GetNext())
        {
            if (rxMessage->ShouldSuppressKnownAnswer(serviceType, subLabel, *serviceEntry))
            {
                shouldSuppress = true;
                break;
            }
        }

        if (!shouldSuppress)
        {
            serviceEntry->AnswerServiceTypeQuestion(aInfo, subLabel);
        }
    }
}

bool Core::RxMessage::ShouldSuppressKnownAnswer(const Name         &aServiceType,
                                                const char         *aSubLabel,
                                                const ServiceEntry &aServiceEntry) const
{
    bool     shouldSuppress = false;
    uint16_t offset         = mStartOffset[kAnswerSection];
    uint16_t numRecords     = mRecordCounts.GetFor(kAnswerSection);

    while (ResourceRecord::FindRecord(*mMessagePtr, offset, numRecords, aServiceType) == kErrorNone)
    {
        Error     error;
        PtrRecord ptr;

        error = ResourceRecord::ReadRecord(*mMessagePtr, offset, ptr);

        if (error == kErrorNotFound)
        {
            // `ReadRecord()` will update the `offset` to skip over
            // the entire record if it does not match the expected
            // record type (PTR in this case).
            continue;
        }

        SuccessOrExit(error);

        // `offset` is now pointing to PTR name

        if (aServiceEntry.Matches(Name(*mMessagePtr, offset)))
        {
            shouldSuppress = aServiceEntry.ShouldSuppressKnownAnswer(ptr.GetTtl(), aSubLabel);
            ExitNow();
        }

        // Parse the name and skip over it and update `offset`
        // to the start of the next record.

        SuccessOrExit(Name::ParseName(*mMessagePtr, offset));
    }

exit:
    return shouldSuppress;
}

bool Core::RxMessage::ParseQuestionNameAsSubType(const Question    &aQuestion,
                                                 Name::LabelBuffer &aSubLabel,
                                                 Name              &aServiceType) const
{
    bool     isSubType = false;
    uint16_t offset    = aQuestion.mNameOffset;
    uint8_t  length    = sizeof(aSubLabel);

    SuccessOrExit(Name::ReadLabel(*mMessagePtr, offset, aSubLabel, length));
    SuccessOrExit(Name::CompareLabel(*mMessagePtr, offset, kSubServiceLabel));
    aServiceType.SetFromMessage(*mMessagePtr, offset);
    isSubType = true;

exit:
    return isSubType;
}

void Core::RxMessage::AnswerAllServicesQuestion(const Question &aQuestion, const AnswerInfo &aInfo)
{
    for (ServiceType &serviceType : Get<Core>().mServiceTypes)
    {
        bool shouldSuppress = false;

        // Check for known-answer in this `RxMessage` and all its
        // related messages in case it is multi-packet query.

        for (const RxMessage *rxMessage = this; rxMessage != nullptr; rxMessage = rxMessage->GetNext())
        {
            if (rxMessage->ShouldSuppressKnownAnswer(aQuestion, serviceType))
            {
                shouldSuppress = true;
                break;
            }
        }

        if (!shouldSuppress)
        {
            serviceType.AnswerQuestion(aInfo);
        }
    }
}

bool Core::RxMessage::ShouldSuppressKnownAnswer(const Question &aQuestion, const ServiceType &aServiceType) const
{
    // Check answer section to determine whether to suppress answering
    // to "_services._dns-sd._udp" query with `aServiceType`

    bool     shouldSuppress = false;
    uint16_t offset         = mStartOffset[kAnswerSection];
    uint16_t numRecords     = mRecordCounts.GetFor(kAnswerSection);
    Name     name(*mMessagePtr, aQuestion.mNameOffset);

    while (ResourceRecord::FindRecord(*mMessagePtr, offset, numRecords, name) == kErrorNone)
    {
        Error     error;
        PtrRecord ptr;

        error = ResourceRecord::ReadRecord(*mMessagePtr, offset, ptr);

        if (error == kErrorNotFound)
        {
            // `ReadRecord()` will update the `offset` to skip over
            // the entire record if it does not match the expected
            // record type (PTR in this case).
            continue;
        }

        SuccessOrExit(error);

        // `offset` is now pointing to PTR name

        if (aServiceType.Matches(Name(*mMessagePtr, offset)))
        {
            shouldSuppress = aServiceType.ShouldSuppressKnownAnswer(ptr.GetTtl());
            ExitNow();
        }

        // Parse the name and skip over it and update `offset`
        // to the start of the next record.

        SuccessOrExit(Name::ParseName(*mMessagePtr, offset));
    }

exit:
    return shouldSuppress;
}

void Core::RxMessage::SendUnicastResponse(const AddressInfo &aUnicastDest)
{
    TxMessage response(GetInstance(),
                       mIsLegacyUnicast ? TxMessage::kLegacyUnicastResponse : TxMessage::kUnicastResponse, aUnicastDest,
                       mIsLegacyUnicast ? mQueryId : 0);

    if (mIsLegacyUnicast)
    {
        // RFC6762, section 6.7:
        // Legacy Unicast Response must repeat the question
        response.AddQuestionFrom(*mMessagePtr);
    }

    TimeMilli now = TimerMilli::GetNow();

    for (HostEntry &entry : Get<Core>().mHostEntries)
    {
        entry.ClearAppendState();
        entry.PrepareResponse(response, now);
    }

    for (ServiceEntry &entry : Get<Core>().mServiceEntries)
    {
        entry.ClearAppendState();
        entry.PrepareResponse(response, now);
    }

    for (ServiceType &serviceType : Get<Core>().mServiceTypes)
    {
        serviceType.ClearAppendState();
        serviceType.PrepareResponse(response, now);
    }

    response.Send();
}

void Core::RxMessage::ProcessResponse(void)
{
    if (!IsSelfOriginating())
    {
        IterateOnAllRecordsInResponse(&RxMessage::ProcessRecordForConflict);
    }

    // We process record types in a specific order to ensure correct
    // passive cache creation: First PTR records are processed, which
    // may create passive SRV/TXT cache entries for discovered
    // services. Next SRV records are processed which may create TXT
    // cache entries for service names and IPv6 address cache entries
    // for associated host name.

    if (!Get<Core>().mBrowseCacheList.IsEmpty())
    {
        IterateOnAllRecordsInResponse(&RxMessage::ProcessPtrRecord);
    }

    if (!Get<Core>().mSrvCacheList.IsEmpty())
    {
        IterateOnAllRecordsInResponse(&RxMessage::ProcessSrvRecord);
    }

    if (!Get<Core>().mTxtCacheList.IsEmpty())
    {
        IterateOnAllRecordsInResponse(&RxMessage::ProcessTxtRecord);
    }

    if (!Get<Core>().mIp6AddrCacheList.IsEmpty())
    {
        IterateOnAllRecordsInResponse(&RxMessage::ProcessAaaaRecord);

        for (Ip6AddrCache &addrCache : Get<Core>().mIp6AddrCacheList)
        {
            addrCache.CommitNewResponseEntries();
        }
    }

    if (!Get<Core>().mIp4AddrCacheList.IsEmpty())
    {
        IterateOnAllRecordsInResponse(&RxMessage::ProcessARecord);

        for (Ip4AddrCache &addrCache : Get<Core>().mIp4AddrCacheList)
        {
            addrCache.CommitNewResponseEntries();
        }
    }
}

void Core::RxMessage::IterateOnAllRecordsInResponse(RecordProcessor aRecordProcessor)
{
    // Iterates over all records in the response, calling
    // `aRecordProcessor` for each.

    static const Section kSections[] = {kAnswerSection, kAdditionalDataSection};

    for (Section section : kSections)
    {
        uint16_t offset = mStartOffset[section];

        for (uint16_t numRecords = mRecordCounts.GetFor(section); numRecords > 0; numRecords--)
        {
            Name           name(*mMessagePtr, offset);
            ResourceRecord record;

            IgnoreError(Name::ParseName(*mMessagePtr, offset));
            IgnoreError(mMessagePtr->Read(offset, record));

            if (!RrClassIsInternetOrAny(record.GetClass()))
            {
                continue;
            }

            (this->*aRecordProcessor)(name, record, offset);

            offset += static_cast<uint16_t>(record.GetSize());
        }
    }
}

void Core::RxMessage::ProcessRecordForConflict(const Name &aName, const ResourceRecord &aRecord, uint16_t aRecordOffset)
{
    HostEntry    *hostEntry;
    ServiceEntry *serviceEntry;

    VerifyOrExit(aRecord.GetTtl() > 0);

    hostEntry = Get<Core>().mHostEntries.FindMatching(aName);

    if (hostEntry != nullptr)
    {
        hostEntry->HandleConflict();
    }

    serviceEntry = Get<Core>().mServiceEntries.FindMatching(aName);

    if (serviceEntry != nullptr)
    {
        serviceEntry->HandleConflict();
    }

exit:
    OT_UNUSED_VARIABLE(aRecordOffset);
}

void Core::RxMessage::ProcessPtrRecord(const Name &aName, const ResourceRecord &aRecord, uint16_t aRecordOffset)
{
    BrowseCache *browseCache;

    VerifyOrExit(aRecord.GetType() == ResourceRecord::kTypePtr);

    browseCache = Get<Core>().mBrowseCacheList.FindMatching(aName);
    VerifyOrExit(browseCache != nullptr);

    browseCache->ProcessResponseRecord(*mMessagePtr, aRecordOffset);

exit:
    return;
}

void Core::RxMessage::ProcessSrvRecord(const Name &aName, const ResourceRecord &aRecord, uint16_t aRecordOffset)
{
    SrvCache *srvCache;

    VerifyOrExit(aRecord.GetType() == ResourceRecord::kTypeSrv);

    srvCache = Get<Core>().mSrvCacheList.FindMatching(aName);
    VerifyOrExit(srvCache != nullptr);

    srvCache->ProcessResponseRecord(*mMessagePtr, aRecordOffset);

exit:
    return;
}

void Core::RxMessage::ProcessTxtRecord(const Name &aName, const ResourceRecord &aRecord, uint16_t aRecordOffset)
{
    TxtCache *txtCache;

    VerifyOrExit(aRecord.GetType() == ResourceRecord::kTypeTxt);

    txtCache = Get<Core>().mTxtCacheList.FindMatching(aName);
    VerifyOrExit(txtCache != nullptr);

    txtCache->ProcessResponseRecord(*mMessagePtr, aRecordOffset);

exit:
    return;
}

void Core::RxMessage::ProcessAaaaRecord(const Name &aName, const ResourceRecord &aRecord, uint16_t aRecordOffset)
{
    Ip6AddrCache *ip6AddrCache;

    VerifyOrExit(aRecord.GetType() == ResourceRecord::kTypeAaaa);

    ip6AddrCache = Get<Core>().mIp6AddrCacheList.FindMatching(aName);
    VerifyOrExit(ip6AddrCache != nullptr);

    ip6AddrCache->ProcessResponseRecord(*mMessagePtr, aRecordOffset);

exit:
    return;
}

void Core::RxMessage::ProcessARecord(const Name &aName, const ResourceRecord &aRecord, uint16_t aRecordOffset)
{
    Ip4AddrCache *ip4AddrCache;

    VerifyOrExit(aRecord.GetType() == ResourceRecord::kTypeA);

    ip4AddrCache = Get<Core>().mIp4AddrCacheList.FindMatching(aName);
    VerifyOrExit(ip4AddrCache != nullptr);

    ip4AddrCache->ProcessResponseRecord(*mMessagePtr, aRecordOffset);

exit:
    return;
}

//---------------------------------------------------------------------------------------------------------------------
// Core::RxMessage::Question

void Core::RxMessage::Question::ClearProcessState(void)
{
    mCanAnswer             = false;
    mIsUnique              = false;
    mIsForService          = false;
    mIsServiceType         = false;
    mIsForAllServicesDnssd = false;
    mEntry                 = nullptr;
}

//---------------------------------------------------------------------------------------------------------------------
// Core::MultiPacketRxMessages

Core::MultiPacketRxMessages::MultiPacketRxMessages(Instance &aInstance)
    : InstanceLocator(aInstance)
    , mTimer(aInstance)
{
}

void Core::MultiPacketRxMessages::AddToExisting(OwnedPtr<RxMessage> &aRxMessagePtr)
{
    RxMsgEntry *msgEntry = mRxMsgEntries.FindMatching(aRxMessagePtr->GetSenderAddress());

    VerifyOrExit(msgEntry != nullptr);
    msgEntry->Add(aRxMessagePtr);

exit:
    return;
}

void Core::MultiPacketRxMessages::AddNew(OwnedPtr<RxMessage> &aRxMessagePtr)
{
    RxMsgEntry *newEntry = RxMsgEntry::Allocate(GetInstance());

    OT_ASSERT(newEntry != nullptr);
    newEntry->Add(aRxMessagePtr);

    // First remove an existing entries matching same sender
    // before adding the new entry to the list.

    mRxMsgEntries.RemoveMatching(aRxMessagePtr->GetSenderAddress());
    mRxMsgEntries.Push(*newEntry);
}

void Core::MultiPacketRxMessages::HandleTimer(void)
{
    NextFireTime           nextTime;
    OwningList<RxMsgEntry> expiredEntries;

    mRxMsgEntries.RemoveAllMatching(ExpireChecker(nextTime.GetNow()), expiredEntries);

    for (RxMsgEntry &expiredEntry : expiredEntries)
    {
        expiredEntry.mRxMessages.GetHead()->ProcessQuery(/* aShouldProcessTruncated */ true);
    }

    for (const RxMsgEntry &msgEntry : mRxMsgEntries)
    {
        nextTime.UpdateIfEarlier(msgEntry.mProcessTime);
    }

    mTimer.FireAtIfEarlier(nextTime);
}

void Core::MultiPacketRxMessages::Clear(void)
{
    mTimer.Stop();
    mRxMsgEntries.Clear();
}

//---------------------------------------------------------------------------------------------------------------------
// Core::MultiPacketRxMessage::RxMsgEntry

Core::MultiPacketRxMessages::RxMsgEntry::RxMsgEntry(Instance &aInstance)
    : InstanceLocator(aInstance)
    , mNext(nullptr)
{
}

bool Core::MultiPacketRxMessages::RxMsgEntry::Matches(const AddressInfo &aAddress) const
{
    bool matches = false;

    VerifyOrExit(!mRxMessages.IsEmpty());
    matches = (mRxMessages.GetHead()->GetSenderAddress() == aAddress);

exit:
    return matches;
}

bool Core::MultiPacketRxMessages::RxMsgEntry::Matches(const ExpireChecker &aExpireChecker) const
{
    return (mProcessTime <= aExpireChecker.mNow);
}

void Core::MultiPacketRxMessages::RxMsgEntry::Add(OwnedPtr<RxMessage> &aRxMessagePtr)
{
    uint16_t numMsgs = 0;

    for (const RxMessage &rxMsg : mRxMessages)
    {
        // If a subsequent received `RxMessage` is also marked as
        // truncated, we again delay the process time. To avoid
        // continuous delay and piling up of messages in the list,
        // we limit the number of messages.

        numMsgs++;
        VerifyOrExit(numMsgs < kMaxNumMessages);

        OT_UNUSED_VARIABLE(rxMsg);
    }

    mProcessTime = TimerMilli::GetNow();

    if (aRxMessagePtr->IsTruncated())
    {
        mProcessTime += Random::NonCrypto::GetUint32InRange(kMinProcessDelay, kMaxProcessDelay);
    }

    // We push the new `RxMessage` at tail of the list to keep the
    // first query containing questions at the head of the list.

    mRxMessages.PushAfterTail(*aRxMessagePtr.Release());

    Get<Core>().mMultiPacketRxMessages.mTimer.FireAtIfEarlier(mProcessTime);

exit:
    return;
}

//---------------------------------------------------------------------------------------------------------------------
// Core::TxMessageHistory

Core::TxMessageHistory::TxMessageHistory(Instance &aInstance)
    : InstanceLocator(aInstance)
    , mTimer(aInstance)
{
}

void Core::TxMessageHistory::Clear(void)
{
    mHashEntries.Clear();
    mTimer.Stop();
}

void Core::TxMessageHistory::Add(const Message &aMessage)
{
    Hash       hash;
    HashEntry *entry;

    CalculateHash(aMessage, hash);

    entry = mHashEntries.FindMatching(hash);

    if (entry == nullptr)
    {
        entry = HashEntry::Allocate();
        OT_ASSERT(entry != nullptr);
        entry->mHash = hash;
        mHashEntries.Push(*entry);
    }

    entry->mExpireTime = TimerMilli::GetNow() + kExpireInterval;
    mTimer.FireAtIfEarlier(entry->mExpireTime);
}

bool Core::TxMessageHistory::Contains(const Message &aMessage) const
{
    Hash hash;

    CalculateHash(aMessage, hash);
    return mHashEntries.ContainsMatching(hash);
}

void Core::TxMessageHistory::CalculateHash(const Message &aMessage, Hash &aHash)
{
    Crypto::Sha256 sha256;

    sha256.Start();
    sha256.Update(aMessage, /* aOffset */ 0, aMessage.GetLength());
    sha256.Finish(aHash);
}

void Core::TxMessageHistory::HandleTimer(void)
{
    NextFireTime nextTime;

    mHashEntries.RemoveAndFreeAllMatching(ExpireChecker(nextTime.GetNow()));

    for (const HashEntry &entry : mHashEntries)
    {
        nextTime.UpdateIfEarlier(entry.mExpireTime);
    }

    mTimer.FireAtIfEarlier(nextTime);
}

template <typename CacheType, typename BrowserResolverType>
Error Core::Start(const BrowserResolverType &aBrowserOrResolver)
{
    Error      error = kErrorNone;
    CacheType *cacheEntry;

    VerifyOrExit(mIsEnabled, error = kErrorInvalidState);
    VerifyOrExit(aBrowserOrResolver.mCallback != nullptr, error = kErrorInvalidArgs);

    cacheEntry = GetCacheList<CacheType>().FindMatching(aBrowserOrResolver);

    if (cacheEntry == nullptr)
    {
        cacheEntry = CacheType::AllocateAndInit(GetInstance(), aBrowserOrResolver);
        OT_ASSERT(cacheEntry != nullptr);

        GetCacheList<CacheType>().Push(*cacheEntry);
    }

    error = cacheEntry->Add(aBrowserOrResolver);

exit:
    return error;
}

template <typename CacheType, typename BrowserResolverType>
Error Core::Stop(const BrowserResolverType &aBrowserOrResolver)
{
    Error      error = kErrorNone;
    CacheType *cacheEntry;

    VerifyOrExit(mIsEnabled, error = kErrorInvalidState);
    VerifyOrExit(aBrowserOrResolver.mCallback != nullptr, error = kErrorInvalidArgs);

    cacheEntry = GetCacheList<CacheType>().FindMatching(aBrowserOrResolver);
    VerifyOrExit(cacheEntry != nullptr);

    cacheEntry->Remove(aBrowserOrResolver);

exit:
    return error;
}

Error Core::StartBrowser(const Browser &aBrowser) { return Start<BrowseCache, Browser>(aBrowser); }

Error Core::StopBrowser(const Browser &aBrowser) { return Stop<BrowseCache, Browser>(aBrowser); }

Error Core::StartSrvResolver(const SrvResolver &aResolver) { return Start<SrvCache, SrvResolver>(aResolver); }

Error Core::StopSrvResolver(const SrvResolver &aResolver) { return Stop<SrvCache, SrvResolver>(aResolver); }

Error Core::StartTxtResolver(const TxtResolver &aResolver) { return Start<TxtCache, TxtResolver>(aResolver); }

Error Core::StopTxtResolver(const TxtResolver &aResolver) { return Stop<TxtCache, TxtResolver>(aResolver); }

Error Core::StartIp6AddressResolver(const AddressResolver &aResolver)
{
    return Start<Ip6AddrCache, AddressResolver>(aResolver);
}

Error Core::StopIp6AddressResolver(const AddressResolver &aResolver)
{
    return Stop<Ip6AddrCache, AddressResolver>(aResolver);
}

Error Core::StartIp4AddressResolver(const AddressResolver &aResolver)
{
    return Start<Ip4AddrCache, AddressResolver>(aResolver);
}

Error Core::StopIp4AddressResolver(const AddressResolver &aResolver)
{
    return Stop<Ip4AddrCache, AddressResolver>(aResolver);
}

void Core::AddPassiveSrvTxtCache(const char *aServiceInstance, const char *aServiceType)
{
    ServiceName serviceName(aServiceInstance, aServiceType);

    if (!mSrvCacheList.ContainsMatching(serviceName))
    {
        SrvCache *srvCache = SrvCache::AllocateAndInit(GetInstance(), serviceName);

        OT_ASSERT(srvCache != nullptr);
        mSrvCacheList.Push(*srvCache);
    }

    if (!mTxtCacheList.ContainsMatching(serviceName))
    {
        TxtCache *txtCache = TxtCache::AllocateAndInit(GetInstance(), serviceName);

        OT_ASSERT(txtCache != nullptr);
        mTxtCacheList.Push(*txtCache);
    }
}

void Core::AddPassiveIp6AddrCache(const char *aHostName)
{
    if (!mIp6AddrCacheList.ContainsMatching(aHostName))
    {
        Ip6AddrCache *ip6AddrCache = Ip6AddrCache::AllocateAndInit(GetInstance(), aHostName);

        OT_ASSERT(ip6AddrCache != nullptr);
        mIp6AddrCacheList.Push(*ip6AddrCache);
    }
}

void Core::HandleCacheTimer(void)
{
    CacheTimerContext context(GetInstance());
    ExpireChecker     expireChecker(context.GetNow());

    // First remove all expired entries.

    mBrowseCacheList.RemoveAndFreeAllMatching(expireChecker);
    mSrvCacheList.RemoveAndFreeAllMatching(expireChecker);
    mTxtCacheList.RemoveAndFreeAllMatching(expireChecker);
    mIp6AddrCacheList.RemoveAndFreeAllMatching(expireChecker);
    mIp4AddrCacheList.RemoveAndFreeAllMatching(expireChecker);

    // Process cache types in a specific order to optimize name
    // compression when constructing query messages.

    for (SrvCache &srvCache : mSrvCacheList)
    {
        srvCache.HandleTimer(context);
    }

    for (TxtCache &txtCache : mTxtCacheList)
    {
        txtCache.HandleTimer(context);
    }

    for (BrowseCache &browseCache : mBrowseCacheList)
    {
        browseCache.HandleTimer(context);
    }

    for (Ip6AddrCache &addrCache : mIp6AddrCacheList)
    {
        addrCache.HandleTimer(context);
    }

    for (Ip4AddrCache &addrCache : mIp4AddrCacheList)
    {
        addrCache.HandleTimer(context);
    }

    context.GetQueryMessage().Send();

    mCacheTimer.FireAtIfEarlier(context.GetNextFireTime());
}

void Core::HandleCacheTask(void)
{
    // `CacheTask` is used to remove empty/null callbacks
    // from cache entries. and also removing "passive"
    // cache entries that timed out.

    for (BrowseCache &browseCache : mBrowseCacheList)
    {
        browseCache.ClearEmptyCallbacks();
    }

    for (SrvCache &srvCache : mSrvCacheList)
    {
        srvCache.ClearEmptyCallbacks();
    }

    for (TxtCache &txtCache : mTxtCacheList)
    {
        txtCache.ClearEmptyCallbacks();
    }

    for (Ip6AddrCache &addrCache : mIp6AddrCacheList)
    {
        addrCache.ClearEmptyCallbacks();
    }

    for (Ip4AddrCache &addrCache : mIp4AddrCacheList)
    {
        addrCache.ClearEmptyCallbacks();
    }
}

TimeMilli Core::RandomizeFirstProbeTxTime(void)
{
    // Randomizes the transmission time of the first probe, adding a
    // delay between 20-250 msec. Subsequent probes within a short
    // window reuse the same delay for efficient aggregation.

    TimeMilli now = TimerMilli::GetNow();

    // The comparison using `(mNextProbeTxTime - now)` will work
    // correctly even in the unlikely case that `now` has wrapped
    // (49 days has passed) since `mNextProbeTxTime` was last set.

    if ((mNextProbeTxTime - now) >= kMaxProbeDelay)
    {
        mNextProbeTxTime = now + Random::NonCrypto::GetUint32InRange(kMinProbeDelay, kMaxProbeDelay);
    }

    return mNextProbeTxTime;
}

TimeMilli Core::RandomizeInitialQueryTxTime(void)
{
    TimeMilli now = TimerMilli::GetNow();

    if ((mNextQueryTxTime - now) >= kMaxInitialQueryDelay)
    {
        mNextQueryTxTime = now + Random::NonCrypto::GetUint32InRange(kMinInitialQueryDelay, kMaxInitialQueryDelay);
    }

    return mNextQueryTxTime;
}

//---------------------------------------------------------------------------------------------------------------------
// Core::ResultCallback

void Core::ResultCallback::Invoke(Instance &aInstance, const BrowseResult &aResult) const
{
    if (mSharedCallback.mBrowse != nullptr)
    {
        mSharedCallback.mBrowse(&aInstance, &aResult);
    }
}

void Core::ResultCallback::Invoke(Instance &aInstance, const SrvResult &aResult) const
{
    if (mSharedCallback.mSrv != nullptr)
    {
        mSharedCallback.mSrv(&aInstance, &aResult);
    }
}

void Core::ResultCallback::Invoke(Instance &aInstance, const TxtResult &aResult) const
{
    if (mSharedCallback.mTxt != nullptr)
    {
        mSharedCallback.mTxt(&aInstance, &aResult);
    }
}

void Core::ResultCallback::Invoke(Instance &aInstance, const AddressResult &aResult) const
{
    if (mSharedCallback.mAddress != nullptr)
    {
        mSharedCallback.mAddress(&aInstance, &aResult);
    }
}

//---------------------------------------------------------------------------------------------------------------------
// Core::CacheTimerContext

Core::CacheTimerContext::CacheTimerContext(Instance &aInstance)
    : InstanceLocator(aInstance)
    , mQueryMessage(aInstance, TxMessage::kMulticastQuery)
{
}

//---------------------------------------------------------------------------------------------------------------------
// Core::CacheRecordInfo

Core::CacheRecordInfo::CacheRecordInfo(void)
    : mTtl(0)
    , mQueryCount(0)
{
}

bool Core::CacheRecordInfo::RefreshTtl(uint32_t aTtl)
{
    // Called when cached record is refreshed.
    // Returns a boolean to indicate if TTL value
    // was changed or not.

    bool changed = (aTtl != mTtl);

    mLastRxTime = TimerMilli::GetNow();
    mTtl        = aTtl;
    mQueryCount = 0;

    return changed;
}

bool Core::CacheRecordInfo::ShouldExpire(TimeMilli aNow) const { return IsPresent() && (GetExpireTime() <= aNow); }

void Core::CacheRecordInfo::UpdateStateAfterQuery(TimeMilli aNow)
{
    VerifyOrExit(IsPresent());

    // If the less than half TTL remains, then this record would not
    // be included as "Known-Answer" in the send query, so we can
    // count it towards queries to refresh this record.

    VerifyOrExit(LessThanHalfTtlRemains(aNow));

    if (mQueryCount < kNumberOfQueries)
    {
        mQueryCount++;
    }

exit:
    return;
}

void Core::CacheRecordInfo::UpdateQueryAndFireTimeOn(CacheEntry &aCacheEntry)
{
    TimeMilli now;
    TimeMilli expireTime;

    VerifyOrExit(IsPresent());

    now        = TimerMilli::GetNow();
    expireTime = GetExpireTime();

    aCacheEntry.SetFireTime(expireTime);

    // Determine next query time

    for (uint8_t attemptIndex = mQueryCount; attemptIndex < kNumberOfQueries; attemptIndex++)
    {
        TimeMilli queryTime = GetQueryTime(attemptIndex);

        if (queryTime > now)
        {
            queryTime += Random::NonCrypto::GetUint32InRange(0, GetClampedTtl() * kQueryTtlVariation);
            aCacheEntry.ScheduleQuery(queryTime);
            break;
        }
    }

exit:
    return;
}

bool Core::CacheRecordInfo::LessThanHalfTtlRemains(TimeMilli aNow) const
{
    return IsPresent() && ((aNow - mLastRxTime) > TimeMilli::SecToMsec(GetClampedTtl()) / 2);
}

uint32_t Core::CacheRecordInfo::GetRemainingTtl(TimeMilli aNow) const
{
    uint32_t  remainingTtl = 0;
    TimeMilli expireTime;

    VerifyOrExit(IsPresent());

    expireTime = GetExpireTime();
    VerifyOrExit(aNow < expireTime);

    remainingTtl = TimeMilli::MsecToSec(expireTime - aNow);

exit:
    return remainingTtl;
}

uint32_t Core::CacheRecordInfo::GetClampedTtl(void) const
{
    // We clamp TTL to `kMaxTtl` (one day) to prevent `TimeMilli`
    // calculation overflow.

    return Min(mTtl, kMaxTtl);
}

TimeMilli Core::CacheRecordInfo::GetExpireTime(void) const
{
    return mLastRxTime + TimeMilli::SecToMsec(GetClampedTtl());
}

TimeMilli Core::CacheRecordInfo::GetQueryTime(uint8_t aAttemptIndex) const
{
    // Queries are sent at 80%, 85%, 90% and 95% of TTL plus a random
    // variation of 2% (added when sceduling)

    static const uint32_t kTtlFactors[kNumberOfQueries] = {
        80 * 1000 / 100,
        85 * 1000 / 100,
        90 * 1000 / 100,
        95 * 1000 / 100,
    };

    OT_ASSERT(aAttemptIndex < kNumberOfQueries);

    return mLastRxTime + kTtlFactors[aAttemptIndex] * GetClampedTtl();
}

//---------------------------------------------------------------------------------------------------------------------
// Core::CacheEntry

void Core::CacheEntry::Init(Instance &aInstance, Type aType)
{
    InstanceLocatorInit::Init(aInstance);

    mType               = aType;
    mInitalQueries      = 0;
    mQueryPending       = false;
    mLastQueryTimeValid = false;
    mIsActive           = false;
    mDeleteTime         = TimerMilli::GetNow() + kNonActiveDeleteTimeout;
}

void Core::CacheEntry::SetIsActive(bool aIsActive)
{
    // Sets the active/passive state of a cache entry. An entry is
    // considered "active" when associated with at least one
    // resolver/browser. "Passive" entries (without a resolver/browser)
    // continue to process mDNS responses for updates but will not send
    // queries. Passive entries are deleted after `kNonActiveDeleteTimeout`
    // if no resolver/browser is added.

    mIsActive = aIsActive;

    if (!mIsActive)
    {
        mQueryPending = false;
        mDeleteTime   = TimerMilli::GetNow() + kNonActiveDeleteTimeout;
        SetFireTime(mDeleteTime);
    }
}

bool Core::CacheEntry::ShouldDelete(TimeMilli aNow) const { return !mIsActive && (mDeleteTime <= aNow); }

void Core::CacheEntry::StartInitialQueries(void)
{
    mInitalQueries      = 0;
    mLastQueryTimeValid = false;
    mLastQueryTime      = Get<Core>().RandomizeInitialQueryTxTime();

    ScheduleQuery(mLastQueryTime);
}

bool Core::CacheEntry::ShouldQuery(TimeMilli aNow) { return mQueryPending && (mNextQueryTime <= aNow); }

void Core::CacheEntry::ScheduleQuery(TimeMilli aQueryTime)
{
    VerifyOrExit(mIsActive);

    if (mQueryPending)
    {
        VerifyOrExit(aQueryTime < mNextQueryTime);
    }

    if (mLastQueryTimeValid)
    {
        aQueryTime = Max(aQueryTime, mLastQueryTime + kMinIntervalBetweenQueries);
    }

    mQueryPending  = true;
    mNextQueryTime = aQueryTime;
    SetFireTime(mNextQueryTime);

exit:
    return;
}

Error Core::CacheEntry::Add(const ResultCallback &aCallback)
{
    Error           error = kErrorNone;
    bool            isFirst;
    ResultCallback *callback;

    callback = FindCallbackMatching(aCallback);
    VerifyOrExit(callback == nullptr, error = kErrorAlready);

    isFirst = mCallbacks.IsEmpty();

    callback = ResultCallback::Allocate(aCallback);
    OT_ASSERT(callback != nullptr);

    mCallbacks.Push(*callback);

    // If this is the first active resolver/browser for this cache
    // entry, we mark it as active which allows queries to be sent We
    // decide whether or not to send initial queries (e.g., for
    // SRV/TXT cache entries we send initial queries if there is no
    // record, or less than half TTL remains).

    if (isFirst)
    {
        bool shouldStart = false;

        SetIsActive(true);

        switch (mType)
        {
        case kBrowseCache:
            shouldStart = true;
            break;
        case kSrvCache:
        case kTxtCache:
            shouldStart = As<ServiceCache>().ShouldStartInitialQueries();
            break;
        case kIp6AddrCache:
        case kIp4AddrCache:
            shouldStart = As<AddrCache>().ShouldStartInitialQueries();
            break;
        }

        if (shouldStart)
        {
            StartInitialQueries();
        }

        DetermineNextFireTime();
        ScheduleTimer();
    }

    // Report any discovered/cached result to the newly added
    // callback.

    switch (mType)
    {
    case kBrowseCache:
        As<BrowseCache>().ReportResultsTo(*callback);
        break;
    case kSrvCache:
        As<SrvCache>().ReportResultTo(*callback);
        break;
    case kTxtCache:
        As<TxtCache>().ReportResultTo(*callback);
        break;
    case kIp6AddrCache:
    case kIp4AddrCache:
        As<AddrCache>().ReportResultsTo(*callback);
        break;
    }

exit:
    return error;
}

void Core::CacheEntry::Remove(const ResultCallback &aCallback)
{
    ResultCallback *callback = FindCallbackMatching(aCallback);

    VerifyOrExit(callback != nullptr);

    // We clear the callback setting it to `nullptr` without removing
    // it from the list here, since the `Remove()` method may be
    // called from a callback while we are iterating over the list.
    // Removal from the list is deferred to `mCacheTask` which will
    // later call `ClearEmptyCallbacks()`.

    callback->ClearCallback();
    Get<Core>().mCacheTask.Post();

exit:
    return;
}

void Core::CacheEntry::ClearEmptyCallbacks(void)
{
    mCallbacks.RemoveAndFreeAllMatching(EmptyChecker());

    if (mCallbacks.IsEmpty())
    {
        SetIsActive(false);
        DetermineNextFireTime();
        ScheduleTimer();
    }
}

void Core::CacheEntry::HandleTimer(CacheTimerContext &aContext)
{
    switch (mType)
    {
    case kBrowseCache:
        As<BrowseCache>().ClearCompressOffsets();
        break;

    case kSrvCache:
    case kTxtCache:
        As<ServiceCache>().ClearCompressOffsets();
        break;

    case kIp6AddrCache:
    case kIp4AddrCache:
        // `AddrCache` entries do not track any append state or
        // compress offset since the host name would not be used
        // in any other query question.
        break;
    }

    VerifyOrExit(HasFireTime());
    VerifyOrExit(GetFireTime() <= aContext.GetNow());
    ClearFireTime();

    if (ShouldDelete(aContext.GetNow()))
    {
        ExitNow();
    }

    if (ShouldQuery(aContext.GetNow()))
    {
        mQueryPending = false;
        PrepareQuery(aContext);
    }

    switch (mType)
    {
    case kBrowseCache:
        As<BrowseCache>().ProcessExpiredRecords(aContext.GetNow());
        break;
    case kSrvCache:
        As<SrvCache>().ProcessExpiredRecords(aContext.GetNow());
        break;
    case kTxtCache:
        As<TxtCache>().ProcessExpiredRecords(aContext.GetNow());
        break;
    case kIp6AddrCache:
    case kIp4AddrCache:
        As<AddrCache>().ProcessExpiredRecords(aContext.GetNow());
        break;
    }

    DetermineNextFireTime();

exit:
    UpdateNextFireTimeOn(aContext.GetNextFireTime());
}

Core::ResultCallback *Core::CacheEntry::FindCallbackMatching(const ResultCallback &aCallback)
{
    ResultCallback *callback = nullptr;

    switch (mType)
    {
    case kBrowseCache:
        callback = mCallbacks.FindMatching(aCallback.mSharedCallback.mBrowse);
        break;
    case kSrvCache:
        callback = mCallbacks.FindMatching(aCallback.mSharedCallback.mSrv);
        break;
    case kTxtCache:
        callback = mCallbacks.FindMatching(aCallback.mSharedCallback.mTxt);
        break;
    case kIp6AddrCache:
    case kIp4AddrCache:
        callback = mCallbacks.FindMatching(aCallback.mSharedCallback.mAddress);
        break;
    }

    return callback;
}

void Core::CacheEntry::DetermineNextFireTime(void)
{
    mQueryPending = false;

    if (mInitalQueries < kNumberOfInitalQueries)
    {
        uint32_t interval = (mInitalQueries == 0) ? 0 : (1U << (mInitalQueries - 1)) * kInitialQueryInterval;

        ScheduleQuery(mLastQueryTime + interval);
    }

    if (!mIsActive)
    {
        SetFireTime(mDeleteTime);
    }

    // Let each cache entry type schedule query and
    // fire times based on the state of its discovered
    // records.

    switch (mType)
    {
    case kBrowseCache:
        As<BrowseCache>().DetermineRecordFireTime();
        break;
    case kSrvCache:
    case kTxtCache:
        As<ServiceCache>().DetermineRecordFireTime();
        break;
    case kIp6AddrCache:
    case kIp4AddrCache:
        As<AddrCache>().DetermineRecordFireTime();
        break;
    }
}

void Core::CacheEntry::ScheduleTimer(void) { ScheduleFireTimeOn(Get<Core>().mCacheTimer); }

void Core::CacheEntry::PrepareQuery(CacheTimerContext &aContext)
{
    bool prepareAgain = false;

    do
    {
        TxMessage &query = aContext.GetQueryMessage();

        query.SaveCurrentState();

        switch (mType)
        {
        case kBrowseCache:
            As<BrowseCache>().PreparePtrQuestion(query, aContext.GetNow());
            break;
        case kSrvCache:
            As<SrvCache>().PrepareSrvQuestion(query);
            break;
        case kTxtCache:
            As<TxtCache>().PrepareTxtQuestion(query);
            break;
        case kIp6AddrCache:
            As<Ip6AddrCache>().PrepareAaaaQuestion(query);
            break;
        case kIp4AddrCache:
            As<Ip4AddrCache>().PrepareAQuestion(query);
            break;
        }

        query.CheckSizeLimitToPrepareAgain(prepareAgain);

    } while (prepareAgain);

    mLastQueryTimeValid = true;
    mLastQueryTime      = aContext.GetNow();

    if (mInitalQueries < kNumberOfInitalQueries)
    {
        mInitalQueries++;
    }

    // Let the cache entry super-classes update their state
    // after query was sent.

    switch (mType)
    {
    case kBrowseCache:
        As<BrowseCache>().UpdateRecordStateAfterQuery(aContext.GetNow());
        break;
    case kSrvCache:
    case kTxtCache:
        As<ServiceCache>().UpdateRecordStateAfterQuery(aContext.GetNow());
        break;
    case kIp6AddrCache:
    case kIp4AddrCache:
        As<AddrCache>().UpdateRecordStateAfterQuery(aContext.GetNow());
        break;
    }
}

template <typename ResultType> void Core::CacheEntry::InvokeCallbacks(const ResultType &aResult)
{
    for (const ResultCallback &callback : mCallbacks)
    {
        callback.Invoke(GetInstance(), aResult);
    }
}

//---------------------------------------------------------------------------------------------------------------------
// Core::BrowseCache

Error Core::BrowseCache::Init(Instance &aInstance, const char *aServiceType, const char *aSubTypeLabel)
{
    Error error = kErrorNone;

    CacheEntry::Init(aInstance, kBrowseCache);
    mNext = nullptr;

    ClearCompressOffsets();
    SuccessOrExit(error = mServiceType.Set(aServiceType));
    SuccessOrExit(error = mSubTypeLabel.Set(aSubTypeLabel));

exit:
    return error;
}

Error Core::BrowseCache::Init(Instance &aInstance, const Browser &aBrowser)
{
    return Init(aInstance, aBrowser.mServiceType, aBrowser.mSubTypeLabel);
}

void Core::BrowseCache::ClearCompressOffsets(void)
{
    mServiceTypeOffset    = kUnspecifiedOffset;
    mSubServiceTypeOffset = kUnspecifiedOffset;
    mSubServiceNameOffset = kUnspecifiedOffset;
}

bool Core::BrowseCache::Matches(const Name &aFullName) const
{
    bool matches   = false;
    bool isSubType = !mSubTypeLabel.IsNull();
    Name name      = aFullName;

    OT_ASSERT(name.IsFromMessage());

    if (isSubType)
    {
        uint16_t       offset;
        const Message &message = name.GetAsMessage(offset);

        SuccessOrExit(Name::CompareLabel(message, offset, mSubTypeLabel.AsCString()));
        name.SetFromMessage(message, offset);
    }

    matches = name.Matches(isSubType ? kSubServiceLabel : nullptr, mServiceType.AsCString(), kLocalDomain);

exit:
    return matches;
}

bool Core::BrowseCache::Matches(const char *aServiceType, const char *aSubTypeLabel) const
{
    bool matches = false;

    if (aSubTypeLabel == nullptr)
    {
        VerifyOrExit(mSubTypeLabel.IsNull());
    }
    else
    {
        VerifyOrExit(NameMatch(mSubTypeLabel, aSubTypeLabel));
    }

    matches = NameMatch(mServiceType, aServiceType);

exit:
    return matches;
}

bool Core::BrowseCache::Matches(const Browser &aBrowser) const
{
    return Matches(aBrowser.mServiceType, aBrowser.mSubTypeLabel);
}

bool Core::BrowseCache::Matches(const ExpireChecker &aExpireChecker) const { return ShouldDelete(aExpireChecker.mNow); }

Error Core::BrowseCache::Add(const Browser &aBrowser) { return CacheEntry::Add(ResultCallback(aBrowser.mCallback)); }

void Core::BrowseCache::Remove(const Browser &aBrowser) { CacheEntry::Remove(ResultCallback(aBrowser.mCallback)); }

void Core::BrowseCache::ProcessResponseRecord(const Message &aMessage, uint16_t aRecordOffset)
{
    // Name and record type in `aMessage` are already matched.

    uint16_t     offset = aRecordOffset;
    PtrRecord    ptr;
    Name::Buffer fullServiceType;
    Name::Buffer serviceInstance;
    BrowseResult result;
    PtrEntry    *ptrEntry;
    bool         changed = false;

    // Read the PTR record. `ReadPtrName()` validates that
    // PTR record is well-formed.

    SuccessOrExit(aMessage.Read(offset, ptr));
    offset += sizeof(ptr);
    SuccessOrExit(ptr.ReadPtrName(aMessage, offset, serviceInstance, fullServiceType));

    VerifyOrExit(Name(fullServiceType).Matches(nullptr, mServiceType.AsCString(), kLocalDomain));

    ptrEntry = mPtrEntries.FindMatching(serviceInstance);

    if (ptr.GetTtl() == 0)
    {
        VerifyOrExit((ptrEntry != nullptr) && ptrEntry->mRecord.IsPresent());

        ptrEntry->mRecord.RefreshTtl(0);
        changed = true;
    }
    else
    {
        if (ptrEntry == nullptr)
        {
            ptrEntry = PtrEntry::AllocateAndInit(serviceInstance);
            VerifyOrExit(ptrEntry != nullptr);
            mPtrEntries.Push(*ptrEntry);
        }

        if (ptrEntry->mRecord.RefreshTtl(ptr.GetTtl()))
        {
            changed = true;
        }
    }

    VerifyOrExit(changed);

    if (ptrEntry->mRecord.IsPresent() && IsActive())
    {
        Get<Core>().AddPassiveSrvTxtCache(ptrEntry->mServiceInstance.AsCString(), mServiceType.AsCString());
    }

    ptrEntry->ConvertTo(result, *this);
    InvokeCallbacks(result);

exit:
    DetermineNextFireTime();
    ScheduleTimer();
}

void Core::BrowseCache::PreparePtrQuestion(TxMessage &aQuery, TimeMilli aNow)
{
    Question question;

    DiscoverCompressOffsets();

    question.SetType(ResourceRecord::kTypePtr);
    question.SetClass(ResourceRecord::kClassInternet);

    AppendServiceTypeOrSubTypeTo(aQuery, kQuestionSection);
    SuccessOrAssert(aQuery.SelectMessageFor(kQuestionSection).Append(question));

    aQuery.IncrementRecordCount(kQuestionSection);

    for (const PtrEntry &ptrEntry : mPtrEntries)
    {
        if (!ptrEntry.mRecord.IsPresent() || ptrEntry.mRecord.LessThanHalfTtlRemains(aNow))
        {
            continue;
        }

        AppendKnownAnswer(aQuery, ptrEntry, aNow);
    }
}

void Core::BrowseCache::DiscoverCompressOffsets(void)
{
    for (const BrowseCache &browseCache : Get<Core>().mBrowseCacheList)
    {
        if (&browseCache == this)
        {
            break;
        }

        if (NameMatch(browseCache.mServiceType, mServiceType))
        {
            UpdateCompressOffset(mServiceTypeOffset, browseCache.mServiceTypeOffset);
            UpdateCompressOffset(mSubServiceTypeOffset, browseCache.mSubServiceTypeOffset);
            VerifyOrExit(mSubServiceTypeOffset == kUnspecifiedOffset);
        }
    }

    VerifyOrExit(mServiceTypeOffset == kUnspecifiedOffset);

    for (const SrvCache &srvCache : Get<Core>().mSrvCacheList)
    {
        if (NameMatch(srvCache.mServiceType, mServiceType))
        {
            UpdateCompressOffset(mServiceTypeOffset, srvCache.mServiceTypeOffset);
            VerifyOrExit(mServiceTypeOffset == kUnspecifiedOffset);
        }
    }

    for (const TxtCache &txtCache : Get<Core>().mTxtCacheList)
    {
        if (NameMatch(txtCache.mServiceType, mServiceType))
        {
            UpdateCompressOffset(mServiceTypeOffset, txtCache.mServiceTypeOffset);
            VerifyOrExit(mServiceTypeOffset == kUnspecifiedOffset);
        }
    }

exit:
    return;
}

void Core::BrowseCache::AppendServiceTypeOrSubTypeTo(TxMessage &aTxMessage, Section aSection)
{
    if (!mSubTypeLabel.IsNull())
    {
        AppendOutcome outcome;

        outcome = aTxMessage.AppendLabel(aSection, mSubTypeLabel.AsCString(), mSubServiceNameOffset);
        VerifyOrExit(outcome != kAppendedFullNameAsCompressed);

        outcome = aTxMessage.AppendLabel(aSection, kSubServiceLabel, mSubServiceTypeOffset);
        VerifyOrExit(outcome != kAppendedFullNameAsCompressed);
    }

    aTxMessage.AppendServiceType(aSection, mServiceType.AsCString(), mServiceTypeOffset);

exit:
    return;
}

void Core::BrowseCache::AppendKnownAnswer(TxMessage &aTxMessage, const PtrEntry &aPtrEntry, TimeMilli aNow)
{
    Message  &message = aTxMessage.SelectMessageFor(kAnswerSection);
    PtrRecord ptr;
    uint16_t  offset;

    ptr.Init();
    ptr.SetTtl(aPtrEntry.mRecord.GetRemainingTtl(aNow));

    AppendServiceTypeOrSubTypeTo(aTxMessage, kAnswerSection);

    offset = message.GetLength();
    SuccessOrAssert(message.Append(ptr));

    SuccessOrAssert(Name::AppendLabel(aPtrEntry.mServiceInstance.AsCString(), message));
    aTxMessage.AppendServiceType(kAnswerSection, mServiceType.AsCString(), mServiceTypeOffset);

    UpdateRecordLengthInMessage(ptr, message, offset);

    aTxMessage.IncrementRecordCount(kAnswerSection);
}

void Core::BrowseCache::UpdateRecordStateAfterQuery(TimeMilli aNow)
{
    for (PtrEntry &ptrEntry : mPtrEntries)
    {
        ptrEntry.mRecord.UpdateStateAfterQuery(aNow);
    }
}

void Core::BrowseCache::DetermineRecordFireTime(void)
{
    for (PtrEntry &ptrEntry : mPtrEntries)
    {
        ptrEntry.mRecord.UpdateQueryAndFireTimeOn(*this);
    }
}

void Core::BrowseCache::ProcessExpiredRecords(TimeMilli aNow)
{
    OwningList<PtrEntry> expiredEntries;

    mPtrEntries.RemoveAllMatching(ExpireChecker(aNow), expiredEntries);

    for (PtrEntry &exiredEntry : expiredEntries)
    {
        BrowseResult result;

        exiredEntry.mRecord.RefreshTtl(0);

        exiredEntry.ConvertTo(result, *this);
        InvokeCallbacks(result);
    }
}

void Core::BrowseCache::ReportResultsTo(ResultCallback &aCallback) const
{
    for (const PtrEntry &ptrEntry : mPtrEntries)
    {
        if (ptrEntry.mRecord.IsPresent())
        {
            BrowseResult result;

            ptrEntry.ConvertTo(result, *this);
            aCallback.Invoke(GetInstance(), result);
        }
    }
}

#if OPENTHREAD_CONFIG_MULTICAST_DNS_ENTRY_ITERATION_API_ENABLE

void Core::BrowseCache::CopyInfoTo(Browser &aBrowser, CacheInfo &aInfo) const
{
    aBrowser.mServiceType   = mServiceType.AsCString();
    aBrowser.mSubTypeLabel  = mSubTypeLabel.AsCString();
    aBrowser.mInfraIfIndex  = Get<Core>().mInfraIfIndex;
    aBrowser.mCallback      = nullptr;
    aInfo.mIsActive         = IsActive();
    aInfo.mHasCachedResults = !mPtrEntries.IsEmpty();
}

#endif

//---------------------------------------------------------------------------------------------------------------------
// Core::BrowseCache::PtrEntry

Error Core::BrowseCache::PtrEntry::Init(const char *aServiceInstance)
{
    mNext = nullptr;

    return mServiceInstance.Set(aServiceInstance);
}

bool Core::BrowseCache::PtrEntry::Matches(const ExpireChecker &aExpireChecker) const
{
    return mRecord.ShouldExpire(aExpireChecker.mNow);
}

void Core::BrowseCache::PtrEntry::ConvertTo(BrowseResult &aResult, const BrowseCache &aBrowseCache) const
{
    ClearAllBytes(aResult);

    aResult.mServiceType     = aBrowseCache.mServiceType.AsCString();
    aResult.mSubTypeLabel    = aBrowseCache.mSubTypeLabel.AsCString();
    aResult.mServiceInstance = mServiceInstance.AsCString();
    aResult.mTtl             = mRecord.GetTtl();
    aResult.mInfraIfIndex    = aBrowseCache.Get<Core>().mInfraIfIndex;
}

//---------------------------------------------------------------------------------------------------------------------
// Core::ServiceCache

Error Core::ServiceCache::Init(Instance &aInstance, Type aType, const char *aServiceInstance, const char *aServiceType)
{
    Error error = kErrorNone;

    CacheEntry::Init(aInstance, aType);
    ClearCompressOffsets();
    SuccessOrExit(error = mServiceInstance.Set(aServiceInstance));
    SuccessOrExit(error = mServiceType.Set(aServiceType));

exit:
    return error;
}

void Core::ServiceCache::ClearCompressOffsets(void)
{
    mServiceNameOffset = kUnspecifiedOffset;
    mServiceTypeOffset = kUnspecifiedOffset;
}

bool Core::ServiceCache::Matches(const Name &aFullName) const
{
    return aFullName.Matches(mServiceInstance.AsCString(), mServiceType.AsCString(), kLocalDomain);
}

bool Core::ServiceCache::Matches(const char *aServiceInstance, const char *aServiceType) const
{
    return NameMatch(mServiceInstance, aServiceInstance) && NameMatch(mServiceType, aServiceType);
}

void Core::ServiceCache::PrepareQueryQuestion(TxMessage &aQuery, uint16_t aRrType)
{
    Message &message = aQuery.SelectMessageFor(kQuestionSection);
    Question question;

    question.SetType(aRrType);
    question.SetClass(ResourceRecord::kClassInternet);

    AppendServiceNameTo(aQuery, kQuestionSection);
    SuccessOrAssert(message.Append(question));

    aQuery.IncrementRecordCount(kQuestionSection);
}

void Core::ServiceCache::AppendServiceNameTo(TxMessage &aTxMessage, Section aSection)
{
    AppendOutcome outcome;

    outcome = aTxMessage.AppendLabel(aSection, mServiceInstance.AsCString(), mServiceNameOffset);
    VerifyOrExit(outcome != kAppendedFullNameAsCompressed);

    aTxMessage.AppendServiceType(aSection, mServiceType.AsCString(), mServiceTypeOffset);

exit:
    return;
}

void Core::ServiceCache::UpdateRecordStateAfterQuery(TimeMilli aNow) { mRecord.UpdateStateAfterQuery(aNow); }

void Core::ServiceCache::DetermineRecordFireTime(void) { mRecord.UpdateQueryAndFireTimeOn(*this); }

bool Core::ServiceCache::ShouldStartInitialQueries(void) const
{
    // This is called when the first active resolver is added
    // for this cache entry to determine whether we should
    // send initial queries. It is possible that we were passively
    // monitoring and have some cached record for this entry.
    // We send initial queries if there is no record or less than
    // half of the original TTL remains.

    return !mRecord.IsPresent() || mRecord.LessThanHalfTtlRemains(TimerMilli::GetNow());
}

//---------------------------------------------------------------------------------------------------------------------
// Core::SrvCache

Error Core::SrvCache::Init(Instance &aInstance, const char *aServiceInstance, const char *aServiceType)
{
    mNext     = nullptr;
    mPort     = 0;
    mPriority = 0;
    mWeight   = 0;

    return ServiceCache::Init(aInstance, kSrvCache, aServiceInstance, aServiceType);
}

Error Core::SrvCache::Init(Instance &aInstance, const ServiceName &aServiceName)
{
    return Init(aInstance, aServiceName.mServiceInstance, aServiceName.mServiceType);
}

Error Core::SrvCache::Init(Instance &aInstance, const SrvResolver &aResolver)
{
    return Init(aInstance, aResolver.mServiceInstance, aResolver.mServiceType);
}

bool Core::SrvCache::Matches(const Name &aFullName) const { return ServiceCache::Matches(aFullName); }

bool Core::SrvCache::Matches(const ServiceName &aServiceName) const
{
    return ServiceCache::Matches(aServiceName.mServiceInstance, aServiceName.mServiceType);
}

bool Core::SrvCache::Matches(const SrvResolver &aResolver) const
{
    return ServiceCache::Matches(aResolver.mServiceInstance, aResolver.mServiceType);
}

bool Core::SrvCache::Matches(const ExpireChecker &aExpireChecker) const { return ShouldDelete(aExpireChecker.mNow); }

Error Core::SrvCache::Add(const SrvResolver &aResolver) { return CacheEntry::Add(ResultCallback(aResolver.mCallback)); }

void Core::SrvCache::Remove(const SrvResolver &aResolver) { CacheEntry::Remove(ResultCallback(aResolver.mCallback)); }

void Core::SrvCache::ProcessResponseRecord(const Message &aMessage, uint16_t aRecordOffset)
{
    // Name and record type in `aMessage` are already matched.

    uint16_t     offset = aRecordOffset;
    SrvRecord    srv;
    Name::Buffer hostFullName;
    Name::Buffer hostName;
    SrvResult    result;
    bool         changed = false;

    // Read the SRV record. `ReadTargetHostName()` validates that
    // SRV record is well-formed.

    SuccessOrExit(aMessage.Read(offset, srv));
    offset += sizeof(srv);
    SuccessOrExit(srv.ReadTargetHostName(aMessage, offset, hostFullName));

    SuccessOrExit(Name::ExtractLabels(hostFullName, kLocalDomain, hostName));

    if (srv.GetTtl() == 0)
    {
        VerifyOrExit(mRecord.IsPresent());

        mHostName.Free();
        mRecord.RefreshTtl(0);
        changed = true;
    }
    else
    {
        if (!mRecord.IsPresent() || !NameMatch(mHostName, hostName))
        {
            SuccessOrAssert(mHostName.Set(hostName));
            changed = true;
        }

        if (!mRecord.IsPresent() || (mPort != srv.GetPort()))
        {
            mPort   = srv.GetPort();
            changed = true;
        }

        if (!mRecord.IsPresent() || (mPriority != srv.GetPriority()))
        {
            mPriority = srv.GetPriority();
            changed   = true;
        }

        if (!mRecord.IsPresent() || (mWeight != srv.GetWeight()))
        {
            mWeight = srv.GetWeight();
            changed = true;
        }

        if (mRecord.RefreshTtl(srv.GetTtl()))
        {
            changed = true;
        }
    }

    VerifyOrExit(changed);

    if (mRecord.IsPresent())
    {
        StopInitialQueries();

        // If not present already, we add a passive `TxtCache` for the
        // same service name, and an `Ip6AddrCache` for the host name.

        Get<Core>().AddPassiveSrvTxtCache(mServiceInstance.AsCString(), mServiceType.AsCString());
        Get<Core>().AddPassiveIp6AddrCache(mHostName.AsCString());
    }

    ConvertTo(result);
    InvokeCallbacks(result);

exit:
    DetermineNextFireTime();
    ScheduleTimer();
}

void Core::SrvCache::PrepareSrvQuestion(TxMessage &aQuery)
{
    DiscoverCompressOffsets();
    PrepareQueryQuestion(aQuery, ResourceRecord::kTypeSrv);
}

void Core::SrvCache::DiscoverCompressOffsets(void)
{
    for (const SrvCache &srvCache : Get<Core>().mSrvCacheList)
    {
        if (&srvCache == this)
        {
            break;
        }

        if (NameMatch(srvCache.mServiceType, mServiceType))
        {
            UpdateCompressOffset(mServiceTypeOffset, srvCache.mServiceTypeOffset);
        }

        if (mServiceTypeOffset != kUnspecifiedOffset)
        {
            break;
        }
    }
}

void Core::SrvCache::ProcessExpiredRecords(TimeMilli aNow)
{
    if (mRecord.ShouldExpire(aNow))
    {
        SrvResult result;

        mRecord.RefreshTtl(0);

        ConvertTo(result);
        InvokeCallbacks(result);
    }
}

void Core::SrvCache::ReportResultTo(ResultCallback &aCallback) const
{
    if (mRecord.IsPresent())
    {
        SrvResult result;

        ConvertTo(result);
        aCallback.Invoke(GetInstance(), result);
    }
}

void Core::SrvCache::ConvertTo(SrvResult &aResult) const
{
    ClearAllBytes(aResult);

    aResult.mServiceInstance = mServiceInstance.AsCString();
    aResult.mServiceType     = mServiceType.AsCString();
    aResult.mHostName        = mHostName.AsCString();
    aResult.mPort            = mPort;
    aResult.mPriority        = mPriority;
    aResult.mWeight          = mWeight;
    aResult.mTtl             = mRecord.GetTtl();
    aResult.mInfraIfIndex    = Get<Core>().mInfraIfIndex;
}

#if OPENTHREAD_CONFIG_MULTICAST_DNS_ENTRY_ITERATION_API_ENABLE

void Core::SrvCache::CopyInfoTo(SrvResolver &aResolver, CacheInfo &aInfo) const
{
    aResolver.mServiceInstance = mServiceInstance.AsCString();
    aResolver.mServiceType     = mServiceType.AsCString();
    aResolver.mInfraIfIndex    = Get<Core>().mInfraIfIndex;
    aResolver.mCallback        = nullptr;
    aInfo.mIsActive            = IsActive();
    aInfo.mHasCachedResults    = mRecord.IsPresent();
}

#endif

//---------------------------------------------------------------------------------------------------------------------
// Core::TxtCache

Error Core::TxtCache::Init(Instance &aInstance, const char *aServiceInstance, const char *aServiceType)
{
    mNext = nullptr;

    return ServiceCache::Init(aInstance, kTxtCache, aServiceInstance, aServiceType);
}

Error Core::TxtCache::Init(Instance &aInstance, const ServiceName &aServiceName)
{
    return Init(aInstance, aServiceName.mServiceInstance, aServiceName.mServiceType);
}

Error Core::TxtCache::Init(Instance &aInstance, const TxtResolver &aResolver)
{
    return Init(aInstance, aResolver.mServiceInstance, aResolver.mServiceType);
}

bool Core::TxtCache::Matches(const Name &aFullName) const { return ServiceCache::Matches(aFullName); }

bool Core::TxtCache::Matches(const ServiceName &aServiceName) const
{
    return ServiceCache::Matches(aServiceName.mServiceInstance, aServiceName.mServiceType);
}

bool Core::TxtCache::Matches(const TxtResolver &aResolver) const
{
    return ServiceCache::Matches(aResolver.mServiceInstance, aResolver.mServiceType);
}

bool Core::TxtCache::Matches(const ExpireChecker &aExpireChecker) const { return ShouldDelete(aExpireChecker.mNow); }

Error Core::TxtCache::Add(const TxtResolver &aResolver) { return CacheEntry::Add(ResultCallback(aResolver.mCallback)); }

void Core::TxtCache::Remove(const TxtResolver &aResolver) { CacheEntry::Remove(ResultCallback(aResolver.mCallback)); }

void Core::TxtCache::ProcessResponseRecord(const Message &aMessage, uint16_t aRecordOffset)
{
    // Name and record type are already matched.

    uint16_t  offset = aRecordOffset;
    TxtRecord txt;
    TxtResult result;
    bool      changed = false;

    SuccessOrExit(aMessage.Read(offset, txt));
    offset += sizeof(txt);

    if (txt.GetTtl() == 0)
    {
        VerifyOrExit(mRecord.IsPresent());

        mTxtData.Free();
        mRecord.RefreshTtl(0);
        changed = true;
    }
    else
    {
        VerifyOrExit(txt.GetLength() > 0);
        VerifyOrExit(aMessage.GetLength() >= offset + txt.GetLength());

        if (!mRecord.IsPresent() || (mTxtData.GetLength() != txt.GetLength()) ||
            !aMessage.CompareBytes(offset, mTxtData.GetBytes(), mTxtData.GetLength()))
        {
            SuccessOrAssert(mTxtData.SetFrom(aMessage, offset, txt.GetLength()));
            changed = true;
        }

        if (mRecord.RefreshTtl(txt.GetTtl()))
        {
            changed = true;
        }
    }

    VerifyOrExit(changed);

    if (mRecord.IsPresent())
    {
        StopInitialQueries();
    }

    ConvertTo(result);
    InvokeCallbacks(result);

exit:
    DetermineNextFireTime();
    ScheduleTimer();
}

void Core::TxtCache::PrepareTxtQuestion(TxMessage &aQuery)
{
    DiscoverCompressOffsets();
    PrepareQueryQuestion(aQuery, ResourceRecord::kTypeTxt);
}

void Core::TxtCache::DiscoverCompressOffsets(void)
{
    for (const SrvCache &srvCache : Get<Core>().mSrvCacheList)
    {
        if (!NameMatch(srvCache.mServiceType, mServiceType))
        {
            continue;
        }

        UpdateCompressOffset(mServiceTypeOffset, srvCache.mServiceTypeOffset);

        if (NameMatch(srvCache.mServiceInstance, mServiceInstance))
        {
            UpdateCompressOffset(mServiceNameOffset, srvCache.mServiceNameOffset);
        }

        VerifyOrExit(mServiceNameOffset == kUnspecifiedOffset);
    }

    for (const TxtCache &txtCache : Get<Core>().mTxtCacheList)
    {
        if (&txtCache == this)
        {
            break;
        }

        if (NameMatch(txtCache.mServiceType, mServiceType))
        {
            UpdateCompressOffset(mServiceTypeOffset, txtCache.mServiceTypeOffset);
        }

        VerifyOrExit(mServiceTypeOffset == kUnspecifiedOffset);
    }

exit:
    return;
}

void Core::TxtCache::ProcessExpiredRecords(TimeMilli aNow)
{
    if (mRecord.ShouldExpire(aNow))
    {
        TxtResult result;

        mRecord.RefreshTtl(0);

        ConvertTo(result);
        InvokeCallbacks(result);
    }
}

void Core::TxtCache::ReportResultTo(ResultCallback &aCallback) const
{
    if (mRecord.IsPresent())
    {
        TxtResult result;

        ConvertTo(result);
        aCallback.Invoke(GetInstance(), result);
    }
}

void Core::TxtCache::ConvertTo(TxtResult &aResult) const
{
    ClearAllBytes(aResult);

    aResult.mServiceInstance = mServiceInstance.AsCString();
    aResult.mServiceType     = mServiceType.AsCString();
    aResult.mTxtData         = mTxtData.GetBytes();
    aResult.mTxtDataLength   = mTxtData.GetLength();
    aResult.mTtl             = mRecord.GetTtl();
    aResult.mInfraIfIndex    = Get<Core>().mInfraIfIndex;
}

#if OPENTHREAD_CONFIG_MULTICAST_DNS_ENTRY_ITERATION_API_ENABLE

void Core::TxtCache::CopyInfoTo(TxtResolver &aResolver, CacheInfo &aInfo) const
{
    aResolver.mServiceInstance = mServiceInstance.AsCString();
    aResolver.mServiceType     = mServiceType.AsCString();
    aResolver.mInfraIfIndex    = Get<Core>().mInfraIfIndex;
    aResolver.mCallback        = nullptr;
    aInfo.mIsActive            = IsActive();
    aInfo.mHasCachedResults    = mRecord.IsPresent();
}

#endif

//---------------------------------------------------------------------------------------------------------------------
// Core::AddrCache

Error Core::AddrCache::Init(Instance &aInstance, Type aType, const char *aHostName)
{
    CacheEntry::Init(aInstance, aType);

    mNext        = nullptr;
    mShouldFlush = false;

    return mName.Set(aHostName);
}

Error Core::AddrCache::Init(Instance &aInstance, Type aType, const AddressResolver &aResolver)
{
    return Init(aInstance, aType, aResolver.mHostName);
}

bool Core::AddrCache::Matches(const Name &aFullName) const
{
    return aFullName.Matches(nullptr, mName.AsCString(), kLocalDomain);
}

bool Core::AddrCache::Matches(const char *aName) const { return NameMatch(mName, aName); }

bool Core::AddrCache::Matches(const AddressResolver &aResolver) const { return Matches(aResolver.mHostName); }

bool Core::AddrCache::Matches(const ExpireChecker &aExpireChecker) const { return ShouldDelete(aExpireChecker.mNow); }

Error Core::AddrCache::Add(const AddressResolver &aResolver)
{
    return CacheEntry::Add(ResultCallback(aResolver.mCallback));
}

void Core::AddrCache::Remove(const AddressResolver &aResolver)
{
    CacheEntry::Remove(ResultCallback(aResolver.mCallback));
}

void Core::AddrCache::PrepareQueryQuestion(TxMessage &aQuery, uint16_t aRrType)
{
    Question question;

    question.SetType(aRrType);
    question.SetClass(ResourceRecord::kClassInternet);

    AppendNameTo(aQuery, kQuestionSection);
    SuccessOrAssert(aQuery.SelectMessageFor(kQuestionSection).Append(question));

    aQuery.IncrementRecordCount(kQuestionSection);
}

void Core::AddrCache::AppendNameTo(TxMessage &aTxMessage, Section aSection)
{
    uint16_t compressOffset = kUnspecifiedOffset;

    AppendOutcome outcome;

    outcome = aTxMessage.AppendMultipleLabels(aSection, mName.AsCString(), compressOffset);
    VerifyOrExit(outcome != kAppendedFullNameAsCompressed);

    aTxMessage.AppendDomainName(aSection);

exit:
    return;
}

void Core::AddrCache::UpdateRecordStateAfterQuery(TimeMilli aNow)
{
    for (AddrEntry &entry : mCommittedEntries)
    {
        entry.mRecord.UpdateStateAfterQuery(aNow);
    }
}

void Core::AddrCache::DetermineRecordFireTime(void)
{
    for (AddrEntry &entry : mCommittedEntries)
    {
        entry.mRecord.UpdateQueryAndFireTimeOn(*this);
    }
}

void Core::AddrCache::ProcessExpiredRecords(TimeMilli aNow)
{
    Heap::Array<AddressAndTtl> addrArray;
    AddressResult              result;
    bool                       didRemoveAny;

    didRemoveAny = mCommittedEntries.RemoveAndFreeAllMatching(ExpireChecker(aNow));

    VerifyOrExit(didRemoveAny);

    ConstructResult(result, addrArray);
    InvokeCallbacks(result);

exit:
    return;
}

void Core::AddrCache::ReportResultsTo(ResultCallback &aCallback) const
{
    Heap::Array<AddressAndTtl> addrArray;
    AddressResult              result;

    ConstructResult(result, addrArray);

    if (result.mAddressesLength > 0)
    {
        aCallback.Invoke(GetInstance(), result);
    }
}

void Core::AddrCache::ConstructResult(AddressResult &aResult, Heap::Array<AddressAndTtl> &aAddrArray) const
{
    // Prepares an `AddressResult` populating it with discovered
    // addresses from the `AddrCache` entry. Uses a caller-provided
    // `Heap::Array` reference (`aAddrArray`) to ensure that the
    // allocated array for `aResult.mAddresses` remains valid until
    // after the `aResult` is used (passed as input to
    // `ResultCallback`).

    uint16_t addrCount = 0;

    ClearAllBytes(aResult);
    aAddrArray.Free();

    for (const AddrEntry &entry : mCommittedEntries)
    {
        if (entry.mRecord.IsPresent())
        {
            addrCount++;
        }
    }

    if (addrCount > 0)
    {
        SuccessOrAssert(aAddrArray.ReserveCapacity(addrCount));

        for (const AddrEntry &entry : mCommittedEntries)
        {
            AddressAndTtl *addr;

            if (!entry.mRecord.IsPresent())
            {
                continue;
            }

            addr = aAddrArray.PushBack();
            OT_ASSERT(addr != nullptr);

            addr->mAddress = entry.mAddress;
            addr->mTtl     = entry.mRecord.GetTtl();
        }
    }

    aResult.mHostName        = mName.AsCString();
    aResult.mAddresses       = aAddrArray.AsCArray();
    aResult.mAddressesLength = aAddrArray.GetLength();
    aResult.mInfraIfIndex    = Get<Core>().mInfraIfIndex;
}

bool Core::AddrCache::ShouldStartInitialQueries(void) const
{
    // This is called when the first active resolver is added
    // for this cache entry to determine whether we should
    // send initial queries. It is possible that we were passively
    // monitoring and has some cached records for this entry.
    // We send initial queries if there is no record or less than
    // half of original TTL remains on any record.

    bool      shouldStart = false;
    TimeMilli now         = TimerMilli::GetNow();

    if (mCommittedEntries.IsEmpty())
    {
        shouldStart = true;
        ExitNow();
    }

    for (const AddrEntry &entry : mCommittedEntries)
    {
        if (entry.mRecord.LessThanHalfTtlRemains(now))
        {
            shouldStart = true;
            ExitNow();
        }
    }

exit:
    return shouldStart;
}

void Core::AddrCache::AddNewResponseAddress(const Ip6::Address &aAddress, uint32_t aTtl, bool aCacheFlush)
{
    // Adds a new address record to `mNewEntries` list. This called as
    // the records in a received response are processed one by one.
    // Once all records are processed `CommitNewResponseEntries()` is
    // called to update the list of addresses.

    AddrEntry *entry;

    if (aCacheFlush)
    {
        mShouldFlush = true;
    }

    // Check for duplicate addresses in the same response.

    entry = mNewEntries.FindMatching(aAddress);

    if (entry == nullptr)
    {
        entry = AddrEntry::Allocate(aAddress);
        OT_ASSERT(entry != nullptr);
        mNewEntries.Push(*entry);
    }

    entry->mRecord.RefreshTtl(aTtl);
}

void Core::AddrCache::CommitNewResponseEntries(void)
{
    bool changed = false;

    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // Determine whether there is any changes to the list of addresses
    // between the `mNewEntries` and `mCommittedEntries` lists.
    //
    // First, we verify if all new entries are present in the
    // `mCommittedEntries` list with the same TTL value. Next, if we
    // need to flush the old cache list, we check if any existing
    // `mCommittedEntries` is absent in `mNewEntries` list.

    for (const AddrEntry &newEntry : mNewEntries)
    {
        AddrEntry *exitingEntry = mCommittedEntries.FindMatching(newEntry.mAddress);

        if (newEntry.GetTtl() == 0)
        {
            // New entry has zero TTL, removing the address. If we
            // have a matching `exitingEntry` we set its TTL to zero
            // so to remove it in the next step when updating the
            // `mCommittedEntries` list.

            if (exitingEntry != nullptr)
            {
                exitingEntry->mRecord.RefreshTtl(0);
                changed = true;
            }
        }
        else if ((exitingEntry == nullptr) || (exitingEntry->GetTtl() != newEntry.GetTtl()))
        {
            changed = true;
        }
    }

    if (mShouldFlush && !changed)
    {
        for (const AddrEntry &exitingEntry : mCommittedEntries)
        {
            if ((exitingEntry.GetTtl() > 0) && !mNewEntries.ContainsMatching(exitingEntry.mAddress))
            {
                changed = true;
                break;
            }
        }
    }

    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // Update the `mCommittedEntries` list.

    // First remove entries, if we need to flush clear everything,
    // otherwise remove the ones with zero TTL marked in previous
    // step. Then, add or update new entries to `mCommittedEntries`

    if (mShouldFlush)
    {
        mCommittedEntries.Clear();
        mShouldFlush = false;
    }
    else
    {
        mCommittedEntries.RemoveAndFreeAllMatching(EmptyChecker());
    }

    while (!mNewEntries.IsEmpty())
    {
        OwnedPtr<AddrEntry> newEntry = mNewEntries.Pop();
        AddrEntry          *entry;

        if (newEntry->GetTtl() == 0)
        {
            continue;
        }

        entry = mCommittedEntries.FindMatching(newEntry->mAddress);

        if (entry != nullptr)
        {
            entry->mRecord.RefreshTtl(newEntry->GetTtl());
        }
        else
        {
            mCommittedEntries.Push(*newEntry.Release());
        }
    }

    StopInitialQueries();

    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // Invoke callbacks if there is any change.

    if (changed)
    {
        Heap::Array<AddressAndTtl> addrArray;
        AddressResult              result;

        ConstructResult(result, addrArray);
        InvokeCallbacks(result);
    }

    DetermineNextFireTime();
    ScheduleTimer();
}

#if OPENTHREAD_CONFIG_MULTICAST_DNS_ENTRY_ITERATION_API_ENABLE

void Core::AddrCache::CopyInfoTo(AddressResolver &aResolver, CacheInfo &aInfo) const
{
    aResolver.mHostName     = mName.AsCString();
    aResolver.mInfraIfIndex = Get<Core>().mInfraIfIndex;
    aResolver.mCallback     = nullptr;
    aInfo.mIsActive         = IsActive();
    aInfo.mHasCachedResults = !mCommittedEntries.IsEmpty();
}

#endif

//---------------------------------------------------------------------------------------------------------------------
// Core::AddrCache::AddrEntry

Core::AddrCache::AddrEntry::AddrEntry(const Ip6::Address &aAddress)
    : mNext(nullptr)
    , mAddress(aAddress)
{
}

bool Core::AddrCache::AddrEntry::Matches(const ExpireChecker &aExpireChecker) const
{
    return mRecord.ShouldExpire(aExpireChecker.mNow);
}

bool Core::AddrCache::AddrEntry::Matches(EmptyChecker aChecker) const
{
    OT_UNUSED_VARIABLE(aChecker);

    return !mRecord.IsPresent();
}

//---------------------------------------------------------------------------------------------------------------------
// Core::Ip6AddrCache

Error Core::Ip6AddrCache::Init(Instance &aInstance, const char *aHostName)
{
    return AddrCache::Init(aInstance, kIp6AddrCache, aHostName);
}

Error Core::Ip6AddrCache::Init(Instance &aInstance, const AddressResolver &aResolver)
{
    return AddrCache::Init(aInstance, kIp6AddrCache, aResolver);
}

void Core::Ip6AddrCache::ProcessResponseRecord(const Message &aMessage, uint16_t aRecordOffset)
{
    // Name and record type in `aMessage` are already matched.

    AaaaRecord aaaaRecord;

    SuccessOrExit(aMessage.Read(aRecordOffset, aaaaRecord));
    VerifyOrExit(aaaaRecord.GetLength() >= sizeof(Ip6::Address));

    AddNewResponseAddress(aaaaRecord.GetAddress(), aaaaRecord.GetTtl(), aaaaRecord.GetClass() & kClassCacheFlushFlag);

exit:
    return;
}

void Core::Ip6AddrCache::PrepareAaaaQuestion(TxMessage &aQuery)
{
    PrepareQueryQuestion(aQuery, ResourceRecord::kTypeAaaa);
}

//---------------------------------------------------------------------------------------------------------------------
// Core::Ip4AddrCache

Error Core::Ip4AddrCache::Init(Instance &aInstance, const char *aHostName)
{
    return AddrCache::Init(aInstance, kIp4AddrCache, aHostName);
}

Error Core::Ip4AddrCache::Init(Instance &aInstance, const AddressResolver &aResolver)
{
    return AddrCache::Init(aInstance, kIp4AddrCache, aResolver);
}

void Core::Ip4AddrCache::ProcessResponseRecord(const Message &aMessage, uint16_t aRecordOffset)
{
    // Name and record type in `aMessage` are already matched.

    ARecord      aRecord;
    Ip6::Address address;

    SuccessOrExit(aMessage.Read(aRecordOffset, aRecord));
    VerifyOrExit(aRecord.GetLength() >= sizeof(Ip4::Address));

    address.SetToIp4Mapped(aRecord.GetAddress());

    AddNewResponseAddress(address, aRecord.GetTtl(), aRecord.GetClass() & kClassCacheFlushFlag);

exit:
    return;
}

void Core::Ip4AddrCache::PrepareAQuestion(TxMessage &aQuery) { PrepareQueryQuestion(aQuery, ResourceRecord::kTypeA); }

//---------------------------------------------------------------------------------------------------------------------
// Core::Iterator

#if OPENTHREAD_CONFIG_MULTICAST_DNS_ENTRY_ITERATION_API_ENABLE

Core::EntryIterator::EntryIterator(Instance &aInstance)
    : InstanceLocator(aInstance)
    , mType(kUnspecified)
{
}

Error Core::EntryIterator::GetNextHost(Host &aHost, EntryState &aState)
{
    Error error = kErrorNotFound;

    if (mType == kUnspecified)
    {
        mHostEntry = Get<Core>().mHostEntries.GetHead();
        mType      = kHost;
    }
    else
    {
        VerifyOrExit(mType == kHost, error = kErrorInvalidArgs);
    }

    while (error == kErrorNotFound)
    {
        VerifyOrExit(mHostEntry != nullptr);
        error      = mHostEntry->CopyInfoTo(aHost, aState);
        mHostEntry = mHostEntry->GetNext();
    }

exit:
    return error;
}

Error Core::EntryIterator::GetNextService(Service &aService, EntryState &aState)
{
    Error error = kErrorNotFound;

    if (mType == kUnspecified)
    {
        mServiceEntry = Get<Core>().mServiceEntries.GetHead();
        mType         = kService;
    }
    else
    {
        VerifyOrExit(mType == kService, error = kErrorInvalidArgs);
    }

    while (error == kErrorNotFound)
    {
        VerifyOrExit(mServiceEntry != nullptr);
        error         = mServiceEntry->CopyInfoTo(aService, aState, *this);
        mServiceEntry = mServiceEntry->GetNext();
    }

exit:
    return error;
}

Error Core::EntryIterator::GetNextKey(Key &aKey, EntryState &aState)
{
    Error error = kErrorNotFound;

    if (mType == kUnspecified)
    {
        mHostEntry = Get<Core>().mHostEntries.GetHead();
        mType      = kHostKey;
    }
    else
    {
        VerifyOrExit((mType == kServiceKey) || (mType == kHostKey), error = kErrorInvalidArgs);
    }

    while ((error == kErrorNotFound) && (mType == kHostKey))
    {
        if (mHostEntry == nullptr)
        {
            mServiceEntry = Get<Core>().mServiceEntries.GetHead();
            mType         = kServiceKey;
            break;
        }

        error      = mHostEntry->CopyInfoTo(aKey, aState);
        mHostEntry = mHostEntry->GetNext();
    }

    while ((error == kErrorNotFound) && (mType == kServiceKey))
    {
        VerifyOrExit(mServiceEntry != nullptr);
        error         = mServiceEntry->CopyInfoTo(aKey, aState);
        mServiceEntry = mServiceEntry->GetNext();
    }

exit:
    return error;
}

Error Core::EntryIterator::GetNextBrowser(Browser &aBrowser, CacheInfo &aInfo)
{
    Error error = kErrorNone;

    if (mType == kUnspecified)
    {
        mBrowseCache = Get<Core>().mBrowseCacheList.GetHead();
        mType        = kBrowser;
    }
    else
    {
        VerifyOrExit(mType == kBrowser, error = kErrorInvalidArgs);
    }

    VerifyOrExit(mBrowseCache != nullptr, error = kErrorNotFound);

    mBrowseCache->CopyInfoTo(aBrowser, aInfo);
    mBrowseCache = mBrowseCache->GetNext();

exit:
    return error;
}

Error Core::EntryIterator::GetNextSrvResolver(SrvResolver &aResolver, CacheInfo &aInfo)
{
    Error error = kErrorNone;

    if (mType == kUnspecified)
    {
        mSrvCache = Get<Core>().mSrvCacheList.GetHead();
        mType     = kSrvResolver;
    }
    else
    {
        VerifyOrExit(mType == kSrvResolver, error = kErrorInvalidArgs);
    }

    VerifyOrExit(mSrvCache != nullptr, error = kErrorNotFound);

    mSrvCache->CopyInfoTo(aResolver, aInfo);
    mSrvCache = mSrvCache->GetNext();

exit:
    return error;
}

Error Core::EntryIterator::GetNextTxtResolver(TxtResolver &aResolver, CacheInfo &aInfo)
{
    Error error = kErrorNone;

    if (mType == kUnspecified)
    {
        mTxtCache = Get<Core>().mTxtCacheList.GetHead();
        mType     = kTxtResolver;
    }
    else
    {
        VerifyOrExit(mType == kTxtResolver, error = kErrorInvalidArgs);
    }

    VerifyOrExit(mTxtCache != nullptr, error = kErrorNotFound);

    mTxtCache->CopyInfoTo(aResolver, aInfo);
    mTxtCache = mTxtCache->GetNext();

exit:
    return error;
}

Error Core::EntryIterator::GetNextIp6AddressResolver(AddressResolver &aResolver, CacheInfo &aInfo)
{
    Error error = kErrorNone;

    if (mType == kUnspecified)
    {
        mIp6AddrCache = Get<Core>().mIp6AddrCacheList.GetHead();
        mType         = kIp6AddrResolver;
    }
    else
    {
        VerifyOrExit(mType == kIp6AddrResolver, error = kErrorInvalidArgs);
    }

    VerifyOrExit(mIp6AddrCache != nullptr, error = kErrorNotFound);

    mIp6AddrCache->CopyInfoTo(aResolver, aInfo);
    mIp6AddrCache = mIp6AddrCache->GetNext();

exit:
    return error;
}

Error Core::EntryIterator::GetNextIp4AddressResolver(AddressResolver &aResolver, CacheInfo &aInfo)
{
    Error error = kErrorNone;

    if (mType == kUnspecified)
    {
        mIp4AddrCache = Get<Core>().mIp4AddrCacheList.GetHead();
        mType         = kIp4AddrResolver;
    }
    else
    {
        VerifyOrExit(mType == kIp4AddrResolver, error = kErrorInvalidArgs);
    }

    VerifyOrExit(mIp4AddrCache != nullptr, error = kErrorNotFound);

    mIp4AddrCache->CopyInfoTo(aResolver, aInfo);
    mIp4AddrCache = mIp4AddrCache->GetNext();

exit:
    return error;
}

#endif // OPENTHREAD_CONFIG_MULTICAST_DNS_ENTRY_ITERATION_API_ENABLE

} // namespace Multicast
} // namespace Dns
} // namespace ot

//---------------------------------------------------------------------------------------------------------------------

#if OPENTHREAD_CONFIG_MULTICAST_DNS_MOCK_PLAT_APIS_ENABLE

OT_TOOL_WEAK otError otPlatMdnsSetListeningEnabled(otInstance *aInstance, bool aEnable, uint32_t aInfraIfIndex)
{
    OT_UNUSED_VARIABLE(aInstance);
    OT_UNUSED_VARIABLE(aEnable);
    OT_UNUSED_VARIABLE(aInfraIfIndex);

    return OT_ERROR_FAILED;
}

OT_TOOL_WEAK void otPlatMdnsSendMulticast(otInstance *aInstance, otMessage *aMessage, uint32_t aInfraIfIndex)
{
    OT_UNUSED_VARIABLE(aInstance);
    OT_UNUSED_VARIABLE(aMessage);
    OT_UNUSED_VARIABLE(aInfraIfIndex);
}

OT_TOOL_WEAK void otPlatMdnsSendUnicast(otInstance                  *aInstance,
                                        otMessage                   *aMessage,
                                        const otPlatMdnsAddressInfo *aAddress)
{
    OT_UNUSED_VARIABLE(aInstance);
    OT_UNUSED_VARIABLE(aMessage);
    OT_UNUSED_VARIABLE(aAddress);
}

#endif // OPENTHREAD_CONFIG_MULTICAST_DNS_MOCK_PLAT_APIS_ENABLE

#endif // OPENTHREAD_CONFIG_MULTICAST_DNS_ENABLE
