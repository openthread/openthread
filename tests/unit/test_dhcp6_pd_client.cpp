/*
 *  Copyright (c) 2025, The OpenThread Authors.
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

#include <openthread/config.h>

#include "test_platform.h"
#include "test_util.hpp"

#include <openthread/dataset_ftd.h>
#include <openthread/thread.h>
#include <openthread/platform/border_routing.h>
#include <openthread/platform/infra_if.h>

#include "border_router/dhcp6_pd_client.hpp"
#include "common/arg_macros.hpp"
#include "common/array.hpp"
#include "common/num_utils.hpp"
#include "instance/instance.hpp"

namespace ot {

#if OT_CONFIG_DHCP6_PD_CLIENT_ENABLE

// Logs a message and adds current time (sNow) as "<hours>:<min>:<secs>.<msec>"
#define Log(...)                                                                                         \
    printf("%02u:%02u:%02u.%03u " OT_FIRST_ARG(__VA_ARGS__) "\n", (sNow / 3600000), (sNow / 60000) % 60, \
           (sNow / 1000) % 60, sNow % 1000 OT_REST_ARGS(__VA_ARGS__))

static constexpr uint32_t kInfraIfIndex = 1;

static Instance *sInstance;

static uint32_t sNow = 0;
static uint32_t sAlarmTime;
static bool     sAlarmOn = false;

//----------------------------------------------------------------------------------------------------------------------
// Function prototypes

void ProcessTasklets(void);
void AdvanceTime(uint32_t aDuration);
void AdvanceNowTo(uint32_t aNewNow);

extern "C" {

#if OPENTHREAD_CONFIG_LOG_OUTPUT == OPENTHREAD_CONFIG_LOG_OUTPUT_PLATFORM_DEFINED
void otPlatLog(otLogLevel aLogLevel, otLogRegion aLogRegion, const char *aFormat, ...)
{
    OT_UNUSED_VARIABLE(aLogLevel);
    OT_UNUSED_VARIABLE(aLogRegion);

    va_list args;

    printf("   ");
    va_start(args, aFormat);
    vprintf(aFormat, args);
    va_end(args);
    printf("\n");
}
#endif

//----------------------------------------------------------------------------------------------------------------------
// otPlatAlarams

void otPlatAlarmMilliStop(otInstance *) { sAlarmOn = false; }

void otPlatAlarmMilliStartAt(otInstance *, uint32_t aT0, uint32_t aDt)
{
    sAlarmOn   = true;
    sAlarmTime = aT0 + aDt;
}

uint32_t otPlatAlarmMilliGetNow(void) { return sNow; }

//----------------------------------------------------------------------------------------------------------------------
// Heap

Array<void *, 500> sHeapAllocatedPtrs;

#if OPENTHREAD_CONFIG_HEAP_EXTERNAL_ENABLE

void *otPlatCAlloc(size_t aNum, size_t aSize)
{
    void *ptr = calloc(aNum, aSize);

    SuccessOrQuit(sHeapAllocatedPtrs.PushBack(ptr));

    return ptr;
}

void otPlatFree(void *aPtr)
{
    if (aPtr != nullptr)
    {
        void **entry = sHeapAllocatedPtrs.Find(aPtr);

        VerifyOrQuit(entry != nullptr, "A heap allocated item is freed twice");
        sHeapAllocatedPtrs.Remove(*entry);
    }

    free(aPtr);
}

#endif

} // extern "C"

//---------------------------------------------------------------------------------------------------------------------
// Dhcp6Msg

struct Dhcp6Msg : public Clearable<Dhcp6Msg>
{
    static constexpr uint16_t kMaxIaPds       = 2;
    static constexpr uint16_t kMaxIaPrefixes  = 3;
    static constexpr uint16_t kMaxReqOptions  = 3;
    static constexpr uint16_t kInfoStringSize = 100;

    typedef String<kInfoStringSize> InfoString;

    struct IaPrefix : public Clearable<IaPrefix>
    {
        Ip6::Prefix mPrefix;
        uint32_t    mPreferredLifetime;
        uint32_t    mValidLifetime;
    };

    using IaPrefixArray = Array<IaPrefix, kMaxIaPrefixes>;

    struct IaPd : Clearable<IaPd>
    {
        bool Matches(uint32_t aIaid) const { return mIaid == aIaid; }

        uint32_t      mIaid;
        uint32_t      mT1;
        uint32_t      mT2;
        bool          mHasStatus;
        uint16_t      mStatusCode;
        IaPrefixArray mIaPrefixes;
    };

    using IaPdArray = Array<IaPd, kMaxIaPds>;

    struct Duid
    {
        InfoString ToString(void) const
        {
            InfoString string;

            string.AppendHexBytes(mShared.mBytes, mLength);
            return string;
        }

        union
        {
            uint8_t          mBytes[Dhcp6::Duid::kMaxSize];
            Dhcp6::Eui64Duid mEui64;
        } mShared;
        uint16_t mLength;
    };

    using ReqOptionArray = Array<uint16_t, kMaxReqOptions>;

    void LogMsg(const char *aAction) const;
    void ParseFrom(const Message &aMessage);
    void PrepareMessage(Message &aMessage);

    uint8_t              mMsgType;
    Dhcp6::TransactionId mTransactionId;
    bool                 mHasStatus : 1;
    bool                 mHasElapsedTime : 1;
    bool                 mHasClientId : 1;
    bool                 mHasServerId : 1;
    bool                 mHasOptionRequest : 1;
    bool                 mHasPreference : 1;
    bool                 mHasServerUnicast : 1;
    bool                 mHasSolMaxRt : 1;
    uint16_t             mStatusCode;
    uint16_t             mElapsedTime;
    Duid                 mClientDuid;
    Duid                 mServerDuid;
    ReqOptionArray       mRequestedOptions;
    uint8_t              mPreference;
    Ip6::Address         mServerAddress;
    uint16_t             mSolMaxRt;
    IaPdArray            mIaPds;
};

static const char *Dhcp6MsgTypeToString(uint8_t aMsgType)
{
    const char *str = "Unknown";

    switch (aMsgType)
    {
    case Dhcp6::kMsgTypeSolicit:
        str = "Solicit";
        break;
    case Dhcp6::kMsgTypeAdvertise:
        str = "Advertise";
        break;
    case Dhcp6::kMsgTypeRequest:
        str = "Request";
        break;
    case Dhcp6::kMsgTypeRenew:
        str = "Renew";
        break;
    case Dhcp6::kMsgTypeRebind:
        str = "Rebind";
        break;
    case Dhcp6::kMsgTypeReply:
        str = "Reply";
        break;
    case Dhcp6::kMsgTypeRelease:
        str = "Release";
        break;
    case Dhcp6::kMsgTypeReconfigure:
        str = "Reconfigure";
        break;
    }

    return str;
}

void Dhcp6Msg::LogMsg(const char *aAction) const
{
    const uint8_t *txnId = reinterpret_cast<const uint8_t *>(&mTransactionId);

    Log("%s %s message", aAction, Dhcp6MsgTypeToString(mMsgType));

    Log("  %-13s : %02x%02x%02x", "TransactionId", txnId[0], txnId[1], txnId[2]);

    if (mHasStatus)
    {
        Log("  %-13s : %u", "StatusCode", mStatusCode);
    }

    if (mHasElapsedTime)
    {
        Log("  %-13s : %u", "ElapsedTime", mElapsedTime);
    }

    if (mHasClientId)
    {
        Log("  %-13s : %s", "ClientId", mClientDuid.ToString().AsCString());
    }

    if (mHasServerId)
    {
        Log("  %-13s : %s", "ServerId", mServerDuid.ToString().AsCString());
    }

    if (mHasOptionRequest)
    {
        InfoString string;

        string.Append("[ ");
        for (uint16_t optionCode : mRequestedOptions)
        {
            string.Append("%u ", optionCode);
        }
        string.Append("]");
        Log("  %-13s : %s", "ReqOptions", string.AsCString());
    }

    if (mHasPreference)
    {
        Log("  %-13s : %u", "Preference", mPreference);
    }

    if (mHasServerUnicast)
    {
        Log("  %-13s : %s", "ServerAddr", mServerAddress.ToString().AsCString());
    }

    if (mHasSolMaxRt)
    {
        Log("  %-13s : %lu", "SolMaxRt", ToUlong(mSolMaxRt));
    }

    for (const IaPd &iaPd : mIaPds)
    {
        Log("  %-13s : Iaid:%lu, T1:%lu, T2:%lu", "IaPd", ToUlong(iaPd.mIaid), ToUlong(iaPd.mT1), ToUlong(iaPd.mT2));

        if (iaPd.mHasStatus)
        {
            Log("    %-11s : %u", "StatusCode", iaPd.mStatusCode);
        }

        for (const IaPrefix &iaPrefix : iaPd.mIaPrefixes)
        {
            Log("    %-11s : %s, preferred:%lu, valid:%lu", "Prefix", iaPrefix.mPrefix.ToString().AsCString(),
                ToUlong(iaPrefix.mPreferredLifetime), ToUlong(iaPrefix.mValidLifetime));
        }
    }
}

void Dhcp6Msg::ParseFrom(const Message &aMessage)
{
    OffsetRange   offsetRange;
    OffsetRange   optionOffsetRange;
    OffsetRange   subOptionOffsetRange;
    Dhcp6::Header header;
    IaPd         *iaPd;
    IaPrefix     *iaPrefix;
    union
    {
        Dhcp6::Option              option;
        Dhcp6::StatusCodeOption    statusOption;
        Dhcp6::ElapsedTimeOption   elapsedTimeOption;
        Dhcp6::PreferenceOption    preferenceOption;
        Dhcp6::ServerUnicastOption serverUnicastOption;
        Dhcp6::SolMaxRtOption      solMaxRtOption;
        Dhcp6::IaPdOption          iaPdOption;
        Dhcp6::IaPrefixOption      iaPrefixOption;
    };

    Clear();
    offsetRange.InitFromMessageFullLength(aMessage);

    SuccessOrQuit(aMessage.Read(offsetRange, header));
    mMsgType       = header.GetMsgType();
    mTransactionId = header.GetTransactionId();
    offsetRange.AdvanceOffset(sizeof(header));

    while (!offsetRange.IsEmpty())
    {
        SuccessOrQuit(aMessage.Read(offsetRange, option));
        VerifyOrQuit(offsetRange.Contains(option.GetSize()));

        optionOffsetRange = offsetRange;
        optionOffsetRange.ShrinkLength(static_cast<uint16_t>(option.GetSize()));
        offsetRange.AdvanceOffset(option.GetSize());

        switch (option.GetCode())
        {
        case Dhcp6::Option::kStatusCode:
            VerifyOrQuit(!mHasStatus);
            mHasStatus = true;
            SuccessOrQuit(aMessage.Read(optionOffsetRange, statusOption));
            mStatusCode = statusOption.GetStatusCode();
            break;

        case Dhcp6::Option::kElapsedTime:
            VerifyOrQuit(!mHasElapsedTime);
            mHasElapsedTime = true;
            SuccessOrQuit(aMessage.Read(optionOffsetRange, elapsedTimeOption));
            mElapsedTime = elapsedTimeOption.GetElapsedTime();
            break;

        case Dhcp6::Option::kClientId:
            VerifyOrQuit(!mHasClientId);
            mHasClientId = true;
            optionOffsetRange.AdvanceOffset(sizeof(Dhcp6::Option));
            VerifyOrQuit(optionOffsetRange.GetLength() >= Dhcp6::Duid::kMinSize);
            VerifyOrQuit(optionOffsetRange.GetLength() <= Dhcp6::Duid::kMaxSize);
            mClientDuid.mLength = optionOffsetRange.GetLength();
            aMessage.ReadBytes(optionOffsetRange, mClientDuid.mShared.mBytes);
            break;

        case Dhcp6::Option::kServerId:
            VerifyOrQuit(!mHasServerId);
            mHasServerId = true;
            optionOffsetRange.AdvanceOffset(sizeof(Dhcp6::Option));
            VerifyOrQuit(optionOffsetRange.GetLength() >= Dhcp6::Duid::kMinSize);
            VerifyOrQuit(optionOffsetRange.GetLength() <= Dhcp6::Duid::kMaxSize);
            mServerDuid.mLength = optionOffsetRange.GetLength();
            aMessage.ReadBytes(optionOffsetRange, mServerDuid.mShared.mBytes);
            break;

        case Dhcp6::Option::kOptionRequest:
            VerifyOrQuit(!mHasOptionRequest);
            mHasOptionRequest = true;
            optionOffsetRange.AdvanceOffset(sizeof(Dhcp6::Option));
            VerifyOrQuit(optionOffsetRange.GetLength() > 0);

            while (!optionOffsetRange.IsEmpty())
            {
                uint16_t reqOption;

                SuccessOrQuit(aMessage.Read(optionOffsetRange, reqOption));
                reqOption = BigEndian::HostSwap16(reqOption);
                SuccessOrQuit(mRequestedOptions.PushBack(reqOption));
                optionOffsetRange.AdvanceOffset(sizeof(uint16_t));
            }

            break;

        case Dhcp6::Option::kPreference:
            VerifyOrQuit(!mHasPreference);
            mHasPreference = true;
            SuccessOrQuit(aMessage.Read(optionOffsetRange, preferenceOption));
            mPreference = preferenceOption.GetPreference();
            break;

        case Dhcp6::Option::kServerUnicast:
            VerifyOrQuit(!mHasServerUnicast);
            mHasServerUnicast = true;
            SuccessOrQuit(aMessage.Read(optionOffsetRange, serverUnicastOption));
            mServerAddress = serverUnicastOption.GetServerAddress();
            break;

        case Dhcp6::Option::kSolMaxRt:
            VerifyOrQuit(!mHasSolMaxRt);
            mHasSolMaxRt = true;
            SuccessOrQuit(aMessage.Read(optionOffsetRange, solMaxRtOption));
            mSolMaxRt = solMaxRtOption.GetSolMaxRt();
            break;

        case Dhcp6::Option::kIaPd:
            SuccessOrQuit(aMessage.Read(optionOffsetRange, iaPdOption));
            optionOffsetRange.AdvanceOffset(sizeof(iaPdOption));
            VerifyOrQuit(!mIaPds.ContainsMatching(iaPdOption.GetIaid()));

            iaPd = mIaPds.PushBack();
            VerifyOrQuit(iaPd != nullptr);
            iaPd->Clear();
            iaPd->mIaid = iaPdOption.GetIaid();
            iaPd->mT1   = iaPdOption.GetT1();
            iaPd->mT2   = iaPdOption.GetT2();

            while (!optionOffsetRange.IsEmpty())
            {
                SuccessOrQuit(aMessage.Read(optionOffsetRange, option));
                VerifyOrQuit(optionOffsetRange.Contains(option.GetSize()));

                subOptionOffsetRange = optionOffsetRange;
                subOptionOffsetRange.ShrinkLength(static_cast<uint16_t>(option.GetSize()));

                optionOffsetRange.AdvanceOffset(option.GetSize());

                switch (option.GetCode())
                {
                case Dhcp6::Option::kStatusCode:
                    VerifyOrQuit(!iaPd->mHasStatus);
                    iaPd->mHasStatus = true;
                    SuccessOrQuit(aMessage.Read(subOptionOffsetRange, statusOption));
                    iaPd->mStatusCode = statusOption.GetStatusCode();
                    break;

                case Dhcp6::Option::kIaPrefix:
                    SuccessOrQuit(aMessage.Read(subOptionOffsetRange, iaPrefixOption));
                    iaPrefix = iaPd->mIaPrefixes.PushBack();
                    VerifyOrQuit(iaPrefix != nullptr);
                    iaPrefix->Clear();
                    iaPrefixOption.GetPrefix(iaPrefix->mPrefix);
                    iaPrefix->mPreferredLifetime = iaPrefixOption.GetPreferredLifetime();
                    iaPrefix->mValidLifetime     = iaPrefixOption.GetValidLifetime();
                    break;

                default:
                    // Unexpected sub-option
                    VerifyOrQuit(false);
                    break;
                }
            }
            break;

        default:
            // Unexpected option
            VerifyOrQuit(false);
            break;
        }
    }
}

void Dhcp6Msg::PrepareMessage(Message &aMessage)
{
    Dhcp6::Header header;
    union
    {
        Dhcp6::Option              option;
        Dhcp6::StatusCodeOption    statusOption;
        Dhcp6::ElapsedTimeOption   elapsedTimeOption;
        Dhcp6::PreferenceOption    preferenceOption;
        Dhcp6::ServerUnicastOption serverUnicastOption;
        Dhcp6::SolMaxRtOption      solMaxRtOption;
        Dhcp6::IaPdOption          iaPdOption;
        Dhcp6::IaPrefixOption      iaPrefixOption;
    };

    header.SetMsgType(static_cast<Dhcp6::MsgType>(mMsgType));
    header.SetTransactionId(mTransactionId);
    SuccessOrQuit(aMessage.Append(header));

    if (mHasStatus)
    {
        statusOption.Init();
        statusOption.SetStatusCode(static_cast<Dhcp6::StatusCodeOption::Status>(mStatusCode));
        SuccessOrQuit(aMessage.Append(statusOption));
    }

    if (mHasElapsedTime)
    {
        elapsedTimeOption.Init();
        elapsedTimeOption.SetElapsedTime(mElapsedTime);
        SuccessOrQuit(aMessage.Append(elapsedTimeOption));
    }

    if (mHasClientId)
    {
        SuccessOrQuit(Dhcp6::Option::AppendOption(aMessage, Dhcp6::Option::kClientId, mClientDuid.mShared.mBytes,
                                                  mClientDuid.mLength));
    }

    if (mHasServerId)
    {
        SuccessOrQuit(Dhcp6::Option::AppendOption(aMessage, Dhcp6::Option::kServerId, mServerDuid.mShared.mBytes,
                                                  mServerDuid.mLength));
    }

    if (mHasOptionRequest)
    {
        option.SetCode(Dhcp6::Option::kOptionRequest);
        option.SetLength(mRequestedOptions.GetLength() * sizeof(uint16_t));
        SuccessOrQuit(aMessage.Append(option));

        for (uint16_t reqOption : mRequestedOptions)
        {
            uint16_t swapedOption = BigEndian::HostSwap16(reqOption);

            SuccessOrQuit(aMessage.Append(swapedOption));
        }
    }

    if (mHasPreference)
    {
        preferenceOption.Init();
        preferenceOption.SetPreference(mPreference);
        SuccessOrQuit(aMessage.Append(preferenceOption));
    }

    if (mHasServerUnicast)
    {
        serverUnicastOption.Init();
        serverUnicastOption.SetServerAddress(mServerAddress);
        SuccessOrQuit(aMessage.Append(serverUnicastOption));
    }

    if (mHasSolMaxRt)
    {
        solMaxRtOption.Init();
        solMaxRtOption.SetSolMaxRt(mSolMaxRt);
        SuccessOrQuit(aMessage.Append(solMaxRtOption));
    }

    for (const IaPd &iaPd : mIaPds)
    {
        uint16_t length;

        length = sizeof(Dhcp6::IaPdOption) - sizeof(Dhcp6::Option);

        if (mHasStatus)
        {
            length += sizeof(Dhcp6::StatusCodeOption);
        }

        length += sizeof(Dhcp6::IaPrefixOption) * iaPd.mIaPrefixes.GetLength();

        iaPdOption.Init();
        iaPdOption.SetLength(length);
        iaPdOption.SetIaid(iaPd.mIaid);
        iaPdOption.SetT1(iaPd.mT1);
        iaPdOption.SetT2(iaPd.mT2);
        SuccessOrQuit(aMessage.Append(iaPdOption));

        if (mHasStatus)
        {
            statusOption.Init();
            statusOption.SetStatusCode(static_cast<Dhcp6::StatusCodeOption::Status>(iaPd.mStatusCode));
            SuccessOrQuit(aMessage.Append(statusOption));
        }

        for (const IaPrefix &iaPrefix : iaPd.mIaPrefixes)
        {
            iaPrefixOption.Init();
            iaPrefixOption.SetPreferredLifetime(iaPrefix.mPreferredLifetime);
            iaPrefixOption.SetValidLifetime(iaPrefix.mValidLifetime);
            iaPrefixOption.SetPrefix(iaPrefix.mPrefix);
            SuccessOrQuit(aMessage.Append(iaPrefixOption));
        }
    }
}

//----------------------------------------------------------------------------------------------------------------------
// Dhcp6RxMsg

static const uint32_t kExpectedIaid         = 0;
static const uint8_t  kExpectedPrefixLength = 64;

static const uint16_t kMaxDhcp6RxMsgs = 32;

class Dhcp6RxMsg : public Dhcp6Msg
{
public:
    void ValidateAsSolicit(void) const;

    void ValidateAsRequest(const Ip6::Prefix     &aPrefix,
                           const Mac::ExtAddress &aServerMacAddr,
                           const Ip6::Address    *aServerIp6Addr = nullptr) const;

    void ValidateAsRenew(const Ip6::Prefix     &aPrefix,
                         const Mac::ExtAddress &aServerMacAddr,
                         const Ip6::Address    *aServerIp6Addr = nullptr) const;

    void ValidateAsRebind(const Ip6::Prefix &aPrefix) const;

    void ValidateAsRelease(const Ip6::Prefix     &aPrefix,
                           const Mac::ExtAddress &aServerMacAddr,
                           const Ip6::Address    *aServerIp6Addr = nullptr) const;

    uint32_t     mRxTime;
    Ip6::Address mDstAddr;

private:
    void Validate(Dhcp6::MsgType         aMsgType,
                  const Ip6::Prefix     &aPrefix,
                  const Mac::ExtAddress *aServerMacAddr = nullptr,
                  const Ip6::Address    *aServerIp6Addr = nullptr) const;
};

Ip6::Prefix PrefixFromString(const char *aString, uint8_t aPrefixLength)
{
    Ip6::Prefix prefix;

    SuccessOrQuit(AsCoreType(&prefix.mPrefix).FromString(aString));
    prefix.mLength = aPrefixLength;

    return prefix;
}

Ip6::Address AddressFromString(const char *aString)
{
    Ip6::Address address;

    SuccessOrQuit(address.FromString(aString));

    return address;
}

void Dhcp6RxMsg::ValidateAsSolicit(void) const
{
    Ip6::Prefix prefix;

    VerifyOrQuit(mMsgType == Dhcp6::kMsgTypeSolicit);
    VerifyOrQuit(mDstAddr == AddressFromString("ff02::1:2"));

    prefix.Clear();
    prefix.SetLength(kExpectedPrefixLength);

    Validate(Dhcp6::kMsgTypeSolicit, prefix);
}

void Dhcp6RxMsg::ValidateAsRequest(const Ip6::Prefix     &aPrefix,
                                   const Mac::ExtAddress &aServerMacAddr,
                                   const Ip6::Address    *aServerIp6Addr) const
{
    Validate(Dhcp6::kMsgTypeRequest, aPrefix, &aServerMacAddr, aServerIp6Addr);
}

void Dhcp6RxMsg::ValidateAsRenew(const Ip6::Prefix     &aPrefix,
                                 const Mac::ExtAddress &aServerMacAddr,
                                 const Ip6::Address    *aServerIp6Addr) const
{
    Validate(Dhcp6::kMsgTypeRenew, aPrefix, &aServerMacAddr, aServerIp6Addr);
}

void Dhcp6RxMsg::ValidateAsRebind(const Ip6::Prefix &aPrefix) const { Validate(Dhcp6::kMsgTypeRebind, aPrefix); }

void Dhcp6RxMsg::ValidateAsRelease(const Ip6::Prefix     &aPrefix,
                                   const Mac::ExtAddress &aServerMacAddr,
                                   const Ip6::Address    *aServerIp6Addr) const
{
    Validate(Dhcp6::kMsgTypeRelease, aPrefix, &aServerMacAddr, aServerIp6Addr);
}

void Dhcp6RxMsg::Validate(Dhcp6::MsgType         aMsgType,
                          const Ip6::Prefix     &aPrefix,
                          const Mac::ExtAddress *aServerMacAddr,
                          const Ip6::Address    *aServerIp6Addr) const
{
    VerifyOrQuit(mMsgType == aMsgType);

    if (aServerIp6Addr != nullptr)
    {
        VerifyOrQuit(mDstAddr == *aServerIp6Addr);
    }
    else
    {
        VerifyOrQuit(mDstAddr == AddressFromString("ff02::1:2"));
    }

    VerifyOrQuit(!mHasStatus);
    VerifyOrQuit(!mHasPreference);
    VerifyOrQuit(!mHasServerUnicast);
    VerifyOrQuit(!mHasSolMaxRt);

    VerifyOrQuit(mHasElapsedTime);

    VerifyOrQuit(mHasOptionRequest);
    VerifyOrQuit(mRequestedOptions.GetLength() == 1);
    VerifyOrQuit(mRequestedOptions[0] == Dhcp6::Option::kSolMaxRt);

    VerifyOrQuit(mHasClientId);
    VerifyOrQuit(mClientDuid.mShared.mEui64.IsValid());
    VerifyOrQuit(mClientDuid.mShared.mEui64.GetLinkLayerAddress() == sInstance->Get<Mac::Mac>().GetExtAddress());

    if (aServerMacAddr != nullptr)
    {
        VerifyOrQuit(mHasServerId);
        VerifyOrQuit(mServerDuid.mShared.mEui64.IsValid());
        VerifyOrQuit(mServerDuid.mShared.mEui64.GetLinkLayerAddress() == *aServerMacAddr);
    }
    else
    {
        VerifyOrQuit(!mHasServerId);
    }

    VerifyOrQuit(mIaPds.GetLength() == 1);
    VerifyOrQuit(mIaPds[0].mIaid == kExpectedIaid);
    VerifyOrQuit(mIaPds[0].mT1 == 0);
    VerifyOrQuit(mIaPds[0].mT2 == 0);
    VerifyOrQuit(!mIaPds[0].mHasStatus);
    VerifyOrQuit(mIaPds[0].mIaPrefixes.GetLength() == 1);
    VerifyOrQuit(mIaPds[0].mIaPrefixes[0].mPrefix == aPrefix);
    VerifyOrQuit(mIaPds[0].mIaPrefixes[0].mPreferredLifetime == 0);
    VerifyOrQuit(mIaPds[0].mIaPrefixes[0].mValidLifetime == 0);
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

struct Dhcp6TxMsg : public Dhcp6Msg
{
    struct PrefixInfo
    {
        uint16_t    mIaid;
        uint32_t    mT1;
        uint32_t    mT2;
        uint32_t    mPreferredLifetime;
        uint32_t    mValidLifetime;
        Ip6::Prefix mPrefix;
    };

    void PrepareAdvertise(const Dhcp6RxMsg      &aClientMsg,
                          const Mac::ExtAddress &aServerMacAddr,
                          const Ip6::Address    *aServerIp6Addr = nullptr);
    void PrepareReply(const Dhcp6RxMsg      &aClientMsg,
                      const Mac::ExtAddress &aServerMacAddr,
                      const Ip6::Address    *aServerIp6Addr = nullptr);
    void AddIaPrefix(const PrefixInfo &aInfo);
    void Send(void);
};

void Dhcp6TxMsg::PrepareAdvertise(const Dhcp6RxMsg      &aClientMsg,
                                  const Mac::ExtAddress &aServerMacAddr,
                                  const Ip6::Address    *aServerIp6Addr)
{
    Clear();
    mMsgType       = Dhcp6::kMsgTypeAdvertise;
    mTransactionId = aClientMsg.mTransactionId;
    mHasClientId   = true;
    mHasServerId   = true;
    mClientDuid    = aClientMsg.mClientDuid;
    mServerDuid.mShared.mEui64.Init(aServerMacAddr);
    mServerDuid.mLength = sizeof(Dhcp6::Eui64Duid);

    if (aServerIp6Addr != nullptr)
    {
        mHasServerUnicast = true;
        mServerAddress    = *aServerIp6Addr;
    }
}

void Dhcp6TxMsg::PrepareReply(const Dhcp6RxMsg      &aClientMsg,
                              const Mac::ExtAddress &aServerMacAddr,
                              const Ip6::Address    *aServerIp6Addr)
{
    Clear();
    mMsgType       = Dhcp6::kMsgTypeReply;
    mTransactionId = aClientMsg.mTransactionId;
    mHasClientId   = true;
    mHasServerId   = true;
    mClientDuid    = aClientMsg.mClientDuid;
    mServerDuid.mShared.mEui64.Init(aServerMacAddr);
    mServerDuid.mLength = sizeof(Dhcp6::Eui64Duid);

    if (aServerIp6Addr != nullptr)
    {
        mHasServerUnicast = true;
        mServerAddress    = *aServerIp6Addr;
    }
}

void Dhcp6TxMsg::AddIaPrefix(const PrefixInfo &aInfo)
{
    IaPd     *iaPd = nullptr;
    IaPrefix *iaPrefix;

    for (IaPd &ia : mIaPds)
    {
        if (ia.mIaid == aInfo.mIaid)
        {
            iaPd = &ia;
            break;
        }
    }

    if (iaPd == nullptr)
    {
        iaPd = mIaPds.PushBack();
        VerifyOrQuit(iaPd != nullptr);
    }

    iaPd->mIaid = aInfo.mIaid;
    iaPd->mT1   = aInfo.mT1;
    iaPd->mT2   = aInfo.mT2;

    iaPrefix = iaPd->mIaPrefixes.PushBack();
    VerifyOrQuit(iaPrefix != nullptr);
    iaPrefix->mPrefix            = aInfo.mPrefix;
    iaPrefix->mPreferredLifetime = aInfo.mPreferredLifetime;
    iaPrefix->mValidLifetime     = aInfo.mValidLifetime;
}

void Dhcp6TxMsg::Send(void)
{
    Message *message;

    message = sInstance->Get<MessagePool>().Allocate(Message::kTypeOther);
    VerifyOrQuit(message != nullptr);
    PrepareMessage(*message);

    LogMsg("Sending");

    otPlatInfraIfDhcp6PdClientHandleReceived(sInstance, message, kInfraIfIndex);
}

//----------------------------------------------------------------------------------------------------------------------
// otPlatInfraIf

typedef BorderRouter::Dhcp6PdClient::DelegatedPrefix DelegatedPrefix;

static bool                               sDhcp6ListeningEnabled = false;
static Array<Dhcp6RxMsg, kMaxDhcp6RxMsgs> sDhcp6RxMsgs;

extern "C" {

void otPlatInfraIfDhcp6PdClientSetListeningEnabled(otInstance *aInstance, bool aEnable, uint32_t aInfraIfIndex)
{
    Log("otPlatInfraIfDhcp6PdClientSetListeningEnabled(aEnable:%u)", aEnable);

    VerifyOrQuit(aInstance == sInstance);
    VerifyOrQuit(aInfraIfIndex == kInfraIfIndex);
    sDhcp6ListeningEnabled = aEnable;
}

void otPlatInfraIfDhcp6PdClientSend(otInstance   *aInstance,
                                    otMessage    *aMessage,
                                    otIp6Address *aDestAddress,
                                    uint32_t      aInfraIfIndex)
{
    Message    *message = AsCoreTypePtr(aMessage);
    Dhcp6RxMsg *rxMsg;

    VerifyOrQuit(aInstance == sInstance);
    VerifyOrQuit(aInfraIfIndex == kInfraIfIndex);
    VerifyOrQuit(message != nullptr);
    VerifyOrQuit(aDestAddress != nullptr);

    Log("otPlatInfraIfDhcp6PdClientSend(%s)", AsCoreType(aDestAddress).ToString().AsCString());

    rxMsg = sDhcp6RxMsgs.PushBack();
    VerifyOrQuit(rxMsg != nullptr);

    rxMsg->ParseFrom(*message);
    rxMsg->LogMsg("Received");

    rxMsg->mRxTime  = sNow;
    rxMsg->mDstAddr = AsCoreType(aDestAddress);

    message->Free();
}

} // extern "C"

//---------------------------------------------------------------------------------------------------------------------

void ProcessTasklets(void)
{
    do
    {
        otTaskletsProcess(sInstance);
    } while (otTaskletsArePending(sInstance));
}

void AdvanceTime(uint32_t aDuration)
{
    uint32_t time = sNow + aDuration;

    Log("AdvanceTime for %u.%03u", aDuration / 1000, aDuration % 1000);

    while (TimeMilli(sAlarmTime) <= TimeMilli(time))
    {
        ProcessTasklets();
        sNow = sAlarmTime;
        otPlatAlarmMilliFired(sInstance);
    }

    ProcessTasklets();
    sNow = time;
}

void AdvanceNowTo(uint32_t aNewNow)
{
    VerifyOrQuit(aNewNow >= sNow);
    AdvanceTime(aNewNow - sNow);
}

void InitTest(void)
{
    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // Initialize OT instance.

    sNow      = 0;
    sAlarmOn  = false;
    sInstance = static_cast<Instance *>(testInitInstance());

    sDhcp6ListeningEnabled = false;
    sDhcp6RxMsgs.Clear();

    SuccessOrQuit(otBorderRoutingInit(sInstance, kInfraIfIndex, /* aInfraIfIsRunning */ true));
    AdvanceTime(100);
}

void FinalizeTest(void) { testFreeInstance(sInstance); }

//---------------------------------------------------------------------------------------------------------------------

void TestDhcp6PdPrefixDelegation(bool aShortPrefix, bool aAddServerUnicastOption)
{
    uint16_t               heapAllocations;
    Dhcp6TxMsg             txMsg;
    Dhcp6TxMsg::PrefixInfo prefixInfo;
    Ip6::Prefix            prefix;
    Ip6::Prefix            adjustedPrefix;
    Mac::ExtAddress        serverMacAddr;
    const DelegatedPrefix *delegatedPrefix;
    Ip6::Address           serverAddr;
    const Ip6::Address    *serverIp6Addr;

    Log("--------------------------------------------------------------------------------------------");
    Log("TestDhcp6PdPrefixDelegation(aShortPrefix:%u, aAddServerUnicastOption:%u)", aShortPrefix,
        aAddServerUnicastOption);

    InitTest();

    heapAllocations = sHeapAllocatedPtrs.GetLength();

    serverMacAddr.GenerateRandom();

    if (aShortPrefix)
    {
        prefix         = PrefixFromString("2001:1111::", 48);
        adjustedPrefix = PrefixFromString("2001:1111::", 64);
    }
    else
    {
        prefix         = PrefixFromString("2001:2222::", 64);
        adjustedPrefix = prefix;
    }

    // If `aAddServerUnicastOption` is enabled, we add `ServerUnicastOption`
    // to Advertise and Reply messages. This prompts the client to use a
    // specific server unicast address instead of the all-servers multicast
    // address. The client's use of this address is then validated when it
    // sends Request or Renew messages throughout all the test steps.

    serverAddr    = AddressFromString("fe80::1");
    serverIp6Addr = (aAddServerUnicastOption ? &serverAddr : 0);

    Log("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ");
    Log("Start the client and wait for the first two Solicit messages");

    VerifyOrQuit(!sDhcp6ListeningEnabled);
    sInstance->Get<BorderRouter::Dhcp6PdClient>().Start();
    VerifyOrQuit(sDhcp6ListeningEnabled);

    AdvanceTime(2200);

    VerifyOrQuit(sDhcp6RxMsgs.GetLength() == 2);
    sDhcp6RxMsgs[0].ValidateAsSolicit();
    sDhcp6RxMsgs[1].ValidateAsSolicit();
    VerifyOrQuit(sDhcp6RxMsgs[0].mTransactionId == sDhcp6RxMsgs[1].mTransactionId);

    Log("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ");
    Log("Send Advertisement");

    txMsg.PrepareAdvertise(sDhcp6RxMsgs[0], serverMacAddr, serverIp6Addr);

    prefixInfo.mIaid              = sDhcp6RxMsgs[0].mIaPds[0].mIaid;
    prefixInfo.mT1                = 2000;
    prefixInfo.mT2                = 3200;
    prefixInfo.mPreferredLifetime = 3600;
    prefixInfo.mValidLifetime     = 4000;
    prefixInfo.mPrefix            = prefix;
    txMsg.AddIaPrefix(prefixInfo);

    sDhcp6RxMsgs.Clear();

    txMsg.Send();

    AdvanceTime(1);

    VerifyOrQuit(sInstance->Get<BorderRouter::Dhcp6PdClient>().GetDelegatedPrefix() == nullptr);

    Log("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ");
    Log("Validate Request message is received");

    VerifyOrQuit(sDhcp6RxMsgs.GetLength() == 1);
    sDhcp6RxMsgs[0].ValidateAsRequest(prefix, serverMacAddr, serverIp6Addr);

    for (uint16_t iter = 0; iter < 3; iter++)
    {
        Log("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ");
        Log("Send Reply message to %s", iter == 0 ? "Request" : "Renew");

        sDhcp6RxMsgs.Clear();

        txMsg.PrepareReply(sDhcp6RxMsgs[0], serverMacAddr, serverIp6Addr);
        txMsg.AddIaPrefix(prefixInfo);

        txMsg.Send();

        Log("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ");
        Log("Validate the delegated prefix on the client");

        delegatedPrefix = sInstance->Get<BorderRouter::Dhcp6PdClient>().GetDelegatedPrefix();
        VerifyOrQuit(delegatedPrefix != nullptr);

        VerifyOrQuit(delegatedPrefix->mPrefix == prefix);
        VerifyOrQuit(delegatedPrefix->mAdjustedPrefix == adjustedPrefix);
        VerifyOrQuit(delegatedPrefix->mT1 == prefixInfo.mT1);
        VerifyOrQuit(delegatedPrefix->mT2 == prefixInfo.mT2);
        VerifyOrQuit(delegatedPrefix->mPreferredLifetime == prefixInfo.mPreferredLifetime);

        Log("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ");
        Log("Validate that no messages is received until renew time T1");

        AdvanceTime(prefixInfo.mT1 * 1000 - 1);
        VerifyOrQuit(sDhcp6RxMsgs.IsEmpty());

        delegatedPrefix = sInstance->Get<BorderRouter::Dhcp6PdClient>().GetDelegatedPrefix();
        VerifyOrQuit(delegatedPrefix != nullptr);
        VerifyOrQuit(delegatedPrefix->mPrefix == prefix);

        Log("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ");
        Log("Validate that at T1 time Renew message is received");

        AdvanceTime(5);
        VerifyOrQuit(sDhcp6RxMsgs.GetLength() == 1);

        sDhcp6RxMsgs[0].ValidateAsRenew(prefix, serverMacAddr, serverIp6Addr);
        VerifyOrQuit(sDhcp6RxMsgs[0].mElapsedTime == 0);
    }

    Log("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ");
    Log("Do not send a Reply to the Renew. Wait till T2 time and check the Renew message retries");

    AdvanceTime((prefixInfo.mT2 - prefixInfo.mT1) * 1000 - 6);
    VerifyOrQuit(sDhcp6RxMsgs.GetLength() > 1);

    for (uint16_t index = 2; index < sDhcp6RxMsgs.GetLength(); index++)
    {
        sDhcp6RxMsgs[index].ValidateAsRenew(prefix, serverMacAddr, serverIp6Addr);
        VerifyOrQuit(sDhcp6RxMsgs[index].mTransactionId == sDhcp6RxMsgs[0].mTransactionId);
        VerifyOrQuit(sDhcp6RxMsgs[index].mElapsedTime > 0);
    }

    Log("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ");
    Log("Validate that the delegated prefix on the client remains unchanged");

    delegatedPrefix = sInstance->Get<BorderRouter::Dhcp6PdClient>().GetDelegatedPrefix();
    VerifyOrQuit(delegatedPrefix != nullptr);

    VerifyOrQuit(delegatedPrefix->mPrefix == prefix);
    VerifyOrQuit(delegatedPrefix->mAdjustedPrefix == adjustedPrefix);
    VerifyOrQuit(delegatedPrefix->mT1 == prefixInfo.mT1);
    VerifyOrQuit(delegatedPrefix->mT2 == prefixInfo.mT2);
    VerifyOrQuit(delegatedPrefix->mPreferredLifetime == prefixInfo.mPreferredLifetime);

    Log("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ");
    Log("Validate that Rebind message is received at T2 time");

    sDhcp6RxMsgs.Clear();
    AdvanceTime(10);

    VerifyOrQuit(sDhcp6RxMsgs.GetLength() == 1);
    sDhcp6RxMsgs[0].ValidateAsRebind(prefix);

    Log("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ");
    Log("Send Reply to Rebind");

    txMsg.PrepareReply(sDhcp6RxMsgs[0], serverMacAddr, serverIp6Addr);
    txMsg.AddIaPrefix(prefixInfo);

    sDhcp6RxMsgs.Clear();

    txMsg.Send();

    Log("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ");
    Log("Validate that the delegated prefix on the client is renewed");

    delegatedPrefix = sInstance->Get<BorderRouter::Dhcp6PdClient>().GetDelegatedPrefix();
    VerifyOrQuit(delegatedPrefix != nullptr);

    VerifyOrQuit(delegatedPrefix->mPrefix == prefix);
    VerifyOrQuit(delegatedPrefix->mAdjustedPrefix == adjustedPrefix);
    VerifyOrQuit(delegatedPrefix->mT1 == prefixInfo.mT1);
    VerifyOrQuit(delegatedPrefix->mT2 == prefixInfo.mT2);
    VerifyOrQuit(delegatedPrefix->mPreferredLifetime == prefixInfo.mPreferredLifetime);

    Log("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ");
    Log("Validate that no message is received until renew time T1");

    AdvanceTime(prefixInfo.mT1 * 1000 - 1);
    VerifyOrQuit(sDhcp6RxMsgs.IsEmpty());

    delegatedPrefix = sInstance->Get<BorderRouter::Dhcp6PdClient>().GetDelegatedPrefix();
    VerifyOrQuit(delegatedPrefix != nullptr);
    VerifyOrQuit(delegatedPrefix->mPrefix == prefix);

    Log("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ");
    Log("Validate Renew message is received");

    AdvanceTime(5);
    VerifyOrQuit(sDhcp6RxMsgs.GetLength() == 1);

    sDhcp6RxMsgs[0].ValidateAsRenew(prefix, serverMacAddr, serverIp6Addr);
    VerifyOrQuit(sDhcp6RxMsgs[0].mElapsedTime == 0);

    Log("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ");
    Log("Wait till T2 time and check that Renew message is retried");

    AdvanceTime((prefixInfo.mT2 - prefixInfo.mT1) * 1000 - 6);
    VerifyOrQuit(sDhcp6RxMsgs.GetLength() > 1);

    for (uint16_t index = 2; index < sDhcp6RxMsgs.GetLength(); index++)
    {
        sDhcp6RxMsgs[index].ValidateAsRenew(prefix, serverMacAddr, serverIp6Addr);
        VerifyOrQuit(sDhcp6RxMsgs[index].mTransactionId == sDhcp6RxMsgs[0].mTransactionId);
        VerifyOrQuit(sDhcp6RxMsgs[index].mElapsedTime > 0);
    }

    Log("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ");
    Log("Validate that Rebind message is received after T2 time");

    sDhcp6RxMsgs.Clear();
    AdvanceTime(5);

    VerifyOrQuit(sDhcp6RxMsgs.GetLength() == 1);
    sDhcp6RxMsgs[0].ValidateAsRebind(prefix);
    VerifyOrQuit(sDhcp6RxMsgs[0].mElapsedTime == 0);

    Log("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ");
    Log("Wait till preferred time and check that Rebind message is now retried");

    AdvanceTime((prefixInfo.mPreferredLifetime - prefixInfo.mT2) * 1000 - 6);
    VerifyOrQuit(sDhcp6RxMsgs.GetLength() > 1);

    for (uint16_t index = 2; index < sDhcp6RxMsgs.GetLength(); index++)
    {
        sDhcp6RxMsgs[index].ValidateAsRebind(prefix);
        VerifyOrQuit(sDhcp6RxMsgs[index].mTransactionId == sDhcp6RxMsgs[0].mTransactionId);
        VerifyOrQuit(sDhcp6RxMsgs[index].mElapsedTime > 0);
    }

    Log("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ");
    Log("Validate that the delegated prefix is still present on the client right before its expire time");

    delegatedPrefix = sInstance->Get<BorderRouter::Dhcp6PdClient>().GetDelegatedPrefix();
    VerifyOrQuit(delegatedPrefix != nullptr);

    VerifyOrQuit(delegatedPrefix->mPrefix == prefix);
    VerifyOrQuit(delegatedPrefix->mAdjustedPrefix == adjustedPrefix);
    VerifyOrQuit(delegatedPrefix->mT1 == prefixInfo.mT1);
    VerifyOrQuit(delegatedPrefix->mT2 == prefixInfo.mT2);
    VerifyOrQuit(delegatedPrefix->mPreferredLifetime == prefixInfo.mPreferredLifetime);

    Log("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ");
    Log("Validate that the delegated prefix is removed after its expire time");

    sDhcp6RxMsgs.Clear();
    AdvanceTime(5);

    delegatedPrefix = sInstance->Get<BorderRouter::Dhcp6PdClient>().GetDelegatedPrefix();
    VerifyOrQuit(delegatedPrefix == nullptr);

    Log("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ");
    Log("Validate that Solicit messages are sent again");

    AdvanceTime(10 * 1000);
    VerifyOrQuit(sDhcp6RxMsgs.GetLength() > 1);

    sDhcp6RxMsgs[0].ValidateAsSolicit();
    VerifyOrQuit(sDhcp6RxMsgs[0].mElapsedTime == 0);

    for (uint16_t index = 2; index < sDhcp6RxMsgs.GetLength(); index++)
    {
        sDhcp6RxMsgs[index].ValidateAsSolicit();
        VerifyOrQuit(sDhcp6RxMsgs[index].mTransactionId == sDhcp6RxMsgs[0].mTransactionId);
        VerifyOrQuit(sDhcp6RxMsgs[index].mElapsedTime > 0);
    }

    Log("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ");
    Log("Send Advertisement, check Request message and respond with Reply");

    txMsg.PrepareAdvertise(sDhcp6RxMsgs[0], serverMacAddr, serverIp6Addr);
    txMsg.AddIaPrefix(prefixInfo);
    sDhcp6RxMsgs.Clear();
    txMsg.Send();

    AdvanceTime(1);

    VerifyOrQuit(sDhcp6RxMsgs.GetLength() == 1);
    sDhcp6RxMsgs[0].ValidateAsRequest(prefix, serverMacAddr, serverIp6Addr);

    txMsg.PrepareReply(sDhcp6RxMsgs[0], serverMacAddr, serverIp6Addr);
    txMsg.AddIaPrefix(prefixInfo);
    sDhcp6RxMsgs.Clear();
    txMsg.Send();

    Log("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ");
    Log("Validate the delegated prefix on the client");

    delegatedPrefix = sInstance->Get<BorderRouter::Dhcp6PdClient>().GetDelegatedPrefix();
    VerifyOrQuit(delegatedPrefix != nullptr);

    VerifyOrQuit(delegatedPrefix->mPrefix == prefix);
    VerifyOrQuit(delegatedPrefix->mAdjustedPrefix == adjustedPrefix);
    VerifyOrQuit(delegatedPrefix->mT1 == prefixInfo.mT1);
    VerifyOrQuit(delegatedPrefix->mT2 == prefixInfo.mT2);
    VerifyOrQuit(delegatedPrefix->mPreferredLifetime == prefixInfo.mPreferredLifetime);

    AdvanceTime(5 * 1000);
    VerifyOrQuit(sDhcp6RxMsgs.IsEmpty());

    Log("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ");
    Log("Stop the client, and validate that Release message is received");

    sInstance->Get<BorderRouter::Dhcp6PdClient>().Stop();

    VerifyOrQuit(sDhcp6RxMsgs.GetLength() == 1);
    sDhcp6RxMsgs[0].ValidateAsRelease(prefix, serverMacAddr, serverIp6Addr);

    Log("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ");
    Log("Send Reply to Release message and check that no more messages is received");

    txMsg.PrepareReply(sDhcp6RxMsgs[0], serverMacAddr, serverIp6Addr);
    txMsg.AddIaPrefix(prefixInfo);
    sDhcp6RxMsgs.Clear();
    txMsg.Send();

    AdvanceTime(20 * 1000);

    VerifyOrQuit(sDhcp6RxMsgs.IsEmpty());

    Log("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ");

    VerifyOrQuit(heapAllocations == sHeapAllocatedPtrs.GetLength());

    Log("End of TestDhcp6PdPrefixDelegation(aShortPrefix:%u, aAddServerUnicastOption:%u)", aShortPrefix,
        aAddServerUnicastOption);

    FinalizeTest();
}

//---------------------------------------------------------------------------------------------------------------------

void TestDhcp6PdSolicitRetries(void)
{
    static uint32_t kMaxTimeout = 3600 * 1000;

    uint16_t heapAllocations;
    uint32_t firstRxTime;
    uint32_t timeout;

    Log("--------------------------------------------------------------------------------------------");
    Log("TestDhcp6PdSolicitRetries");

    InitTest();

    heapAllocations = sHeapAllocatedPtrs.GetLength();

    Log("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ");
    Log("Start the client and check initial delay for first Solicit");

    VerifyOrQuit(!sDhcp6ListeningEnabled);

    sInstance->Get<BorderRouter::Dhcp6PdClient>().Start();

    VerifyOrQuit(sDhcp6ListeningEnabled);

    // Initial random delay of [0, 1000] msec to send first solicit
    AdvanceTime(1000);

    VerifyOrQuit(sDhcp6RxMsgs.GetLength() == 1);
    sDhcp6RxMsgs[0].ValidateAsSolicit();
    VerifyOrQuit(sDhcp6RxMsgs[0].mElapsedTime == 0);
    firstRxTime = sDhcp6RxMsgs[0].mRxTime;

    VerifyOrQuit(sInstance->Get<BorderRouter::Dhcp6PdClient>().GetDelegatedPrefix() == nullptr);

    Log("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ");
    Log("Wait for more than 30 minutes and collect all Solicit messages");

    AdvanceTime(20000 * 1000);

    VerifyOrQuit(sDhcp6RxMsgs.GetLength() >= 14);

    Log("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ");
    Log("Validate the retx timing of Solicit messages");

    sDhcp6RxMsgs[1].ValidateAsSolicit();
    VerifyOrQuit(sDhcp6RxMsgs[1].mTransactionId == sDhcp6RxMsgs[0].mTransactionId);
    VerifyOrQuit(sDhcp6RxMsgs[1].mElapsedTime == (sDhcp6RxMsgs[1].mRxTime - firstRxTime) / 10);
    timeout = sDhcp6RxMsgs[1].mRxTime - firstRxTime;

    // The initial time should be randomly picked from [1, 1.1] sec.

    VerifyOrQuit(timeout >= 1000);
    VerifyOrQuit(timeout <= 1100);

    for (uint16_t index = 2; index < sDhcp6RxMsgs.GetLength(); index++)
    {
        uint32_t newTimeout;
        uint32_t minTimeout;
        uint32_t maxTimeout;
        uint32_t elapsedTime;

        sDhcp6RxMsgs[index].ValidateAsSolicit();
        VerifyOrQuit(sDhcp6RxMsgs[index].mTransactionId == sDhcp6RxMsgs[0].mTransactionId);

        newTimeout = sDhcp6RxMsgs[index].mRxTime - sDhcp6RxMsgs[index - 1].mRxTime;

        minTimeout = 2 * timeout - timeout / 10;
        maxTimeout = 2 * timeout + timeout / 10;

        if (maxTimeout > kMaxTimeout)
        {
            minTimeout = Min(minTimeout, kMaxTimeout - kMaxTimeout / 10);
            maxTimeout = kMaxTimeout + kMaxTimeout / 10;
        }

        Log("Solicit %2u -> timeout:%lu, min:%lu, max:%lu", index, ToUlong(newTimeout), ToUlong(minTimeout),
            ToUlong(maxTimeout));

        elapsedTime = (sDhcp6RxMsgs[index].mRxTime - firstRxTime) / 10;
        VerifyOrQuit(sDhcp6RxMsgs[index].mElapsedTime == ClampToUint16(elapsedTime));

        VerifyOrQuit(newTimeout >= minTimeout);
        VerifyOrQuit(newTimeout <= maxTimeout);

        timeout = newTimeout;
    }

    VerifyOrQuit(sInstance->Get<BorderRouter::Dhcp6PdClient>().GetDelegatedPrefix() == nullptr);

    Log("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ");
    Log("Stop the client and make sure there are no more transmissions");

    sDhcp6RxMsgs.Clear();
    sInstance->Get<BorderRouter::Dhcp6PdClient>().Stop();

    VerifyOrQuit(!sDhcp6ListeningEnabled);

    AdvanceTime(200 * 1000);
    VerifyOrQuit(sDhcp6RxMsgs.IsEmpty());

    Log("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ");
    Log("Start again and check the Solicit message tx");

    sInstance->Get<BorderRouter::Dhcp6PdClient>().Start();

    VerifyOrQuit(sDhcp6ListeningEnabled);

    // Initial random delay of [0, 1000] msec to send first solicit
    AdvanceTime(1000);

    VerifyOrQuit(sDhcp6RxMsgs.GetLength() == 1);
    sDhcp6RxMsgs[0].ValidateAsSolicit();
    VerifyOrQuit(sDhcp6RxMsgs[0].mElapsedTime == 0);

    Log("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ");
    Log("Stop the client");

    sInstance->Get<BorderRouter::Dhcp6PdClient>().Stop();
    AdvanceTime(15 * 1000);

    VerifyOrQuit(heapAllocations == sHeapAllocatedPtrs.GetLength());

    Log("End of TestDhcp6PdSolicitRetries");

    FinalizeTest();
}

//---------------------------------------------------------------------------------------------------------------------

void TestDhcp6PdRequestRetries(void)
{
    static constexpr uint32_t kInitialRequestTimeout = 1000;
    static constexpr uint32_t kMaxRequestTimeout     = 30 * 1000;
    static constexpr uint16_t kMaxRequestRetxCount   = 10;

    uint16_t               heapAllocations;
    Dhcp6TxMsg             txMsg;
    Dhcp6TxMsg::PrefixInfo prefixInfo;
    Ip6::Prefix            prefix;
    Mac::ExtAddress        serverMacAddr;
    uint32_t               firstRxTime;
    uint32_t               timeout;
    uint32_t               newTimeout;
    uint32_t               minTimeout;
    uint32_t               maxTimeout;
    uint32_t               elapsedTime;
    uint16_t               index;

    Log("--------------------------------------------------------------------------------------------");
    Log("TestDhcp6PdRequestRetries");

    InitTest();

    heapAllocations = sHeapAllocatedPtrs.GetLength();

    serverMacAddr.GenerateRandom();

    prefix = PrefixFromString("2001:aa::", 64);

    Log("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ");
    Log("Start the client and wait for the first two Solicit messages");

    VerifyOrQuit(!sDhcp6ListeningEnabled);
    sInstance->Get<BorderRouter::Dhcp6PdClient>().Start();
    VerifyOrQuit(sDhcp6ListeningEnabled);

    AdvanceTime(2200);

    VerifyOrQuit(sDhcp6RxMsgs.GetLength() == 2);
    sDhcp6RxMsgs[0].ValidateAsSolicit();
    sDhcp6RxMsgs[1].ValidateAsSolicit();
    VerifyOrQuit(sDhcp6RxMsgs[0].mTransactionId == sDhcp6RxMsgs[1].mTransactionId);

    Log("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ");
    Log("Send Advertisement");

    txMsg.PrepareAdvertise(sDhcp6RxMsgs[0], serverMacAddr);

    prefixInfo.mIaid              = sDhcp6RxMsgs[0].mIaPds[0].mIaid;
    prefixInfo.mT1                = 2000;
    prefixInfo.mT2                = 3200;
    prefixInfo.mPreferredLifetime = 3600;
    prefixInfo.mValidLifetime     = 4000;
    prefixInfo.mPrefix            = prefix;
    txMsg.AddIaPrefix(prefixInfo);

    sDhcp6RxMsgs.Clear();
    txMsg.Send();

    Log("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ");
    Log("Validate Request message is received");

    VerifyOrQuit(sDhcp6RxMsgs.GetLength() == 1);
    sDhcp6RxMsgs[0].ValidateAsRequest(prefix, serverMacAddr);
    VerifyOrQuit(sDhcp6RxMsgs[0].mElapsedTime == 0);
    firstRxTime = sDhcp6RxMsgs[0].mRxTime;

    Log("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ");
    Log("Wait for 5 minutes and collect all messages");

    AdvanceTime(300 * 1000);

    VerifyOrQuit(sDhcp6RxMsgs.GetLength() > kMaxRequestRetxCount);

    Log("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ");
    Log("Validate the retx timing of Request messages");

    sDhcp6RxMsgs[1].ValidateAsRequest(prefix, serverMacAddr);
    VerifyOrQuit(sDhcp6RxMsgs[1].mTransactionId == sDhcp6RxMsgs[0].mTransactionId);
    VerifyOrQuit(sDhcp6RxMsgs[1].mElapsedTime == (sDhcp6RxMsgs[1].mRxTime - firstRxTime) / 10);
    timeout = sDhcp6RxMsgs[1].mRxTime - firstRxTime;

    // Validate the initial timeout

    minTimeout = kInitialRequestTimeout - kInitialRequestTimeout / 10;
    maxTimeout = kInitialRequestTimeout + kInitialRequestTimeout / 10;

    Log("Request %2u -> timeout:%lu, min:%lu, max:%lu", 1, ToUlong(timeout), ToUlong(minTimeout), ToUlong(maxTimeout));

    VerifyOrQuit(timeout >= minTimeout);
    VerifyOrQuit(timeout <= maxTimeout);

    for (index = 2; index <= kMaxRequestRetxCount; index++)
    {
        sDhcp6RxMsgs[index].ValidateAsRequest(prefix, serverMacAddr);
        VerifyOrQuit(sDhcp6RxMsgs[index].mTransactionId == sDhcp6RxMsgs[0].mTransactionId);

        newTimeout = sDhcp6RxMsgs[index].mRxTime - sDhcp6RxMsgs[index - 1].mRxTime;

        minTimeout = 2 * timeout - timeout / 10;
        maxTimeout = 2 * timeout + timeout / 10;

        if (maxTimeout > kMaxRequestTimeout)
        {
            minTimeout = Min(minTimeout, kMaxRequestTimeout - kMaxRequestTimeout / 10);
            maxTimeout = kMaxRequestTimeout + kMaxRequestTimeout / 10;
        }

        Log("Request %2u -> timeout:%lu, min:%lu, max:%lu", index, ToUlong(newTimeout), ToUlong(minTimeout),
            ToUlong(maxTimeout));

        elapsedTime = (sDhcp6RxMsgs[index].mRxTime - firstRxTime) / 10;
        VerifyOrQuit(sDhcp6RxMsgs[index].mElapsedTime == ClampToUint16(elapsedTime));

        VerifyOrQuit(newTimeout >= minTimeout);
        VerifyOrQuit(newTimeout <= maxTimeout);

        timeout = newTimeout;
    }

    Log("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ");
    Log("Validate that after the Request Retries are finished, client restarts sending Solicit");

    VerifyOrQuit(sDhcp6RxMsgs.GetLength() > kMaxRequestRetxCount + 1);

    index = kMaxRequestRetxCount + 1;

    sDhcp6RxMsgs[index].ValidateAsSolicit();
    VerifyOrQuit(sDhcp6RxMsgs[index].mTransactionId != sDhcp6RxMsgs[0].mTransactionId);
    VerifyOrQuit(sDhcp6RxMsgs[index].mElapsedTime == 0);

    // Check the timeout of the last Request message
    timeout = sDhcp6RxMsgs[index].mRxTime - sDhcp6RxMsgs[index - 1].mRxTime;
    VerifyOrQuit(timeout >= minTimeout);
    VerifyOrQuit(timeout <= maxTimeout);

    Log("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ");
    Log("Stop the client");

    sInstance->Get<BorderRouter::Dhcp6PdClient>().Stop();
    AdvanceTime(15 * 1000);

    VerifyOrQuit(heapAllocations == sHeapAllocatedPtrs.GetLength());

    Log("End of TestDhcp6PdRequestRetries");

    FinalizeTest();
}

//---------------------------------------------------------------------------------------------------------------------

void TestDhcp6PdSelectBetweenMultipleServers(void)
{
    uint16_t               heapAllocations;
    Dhcp6TxMsg             txMsg;
    Dhcp6TxMsg::PrefixInfo prefixInfo;
    Ip6::Prefix            prefix1;
    Ip6::Prefix            prefix2;
    Ip6::Prefix            prefix3;
    Mac::ExtAddress        serverMacAddr1;
    Mac::ExtAddress        serverMacAddr2;
    Mac::ExtAddress        serverMacAddr3;

    Log("--------------------------------------------------------------------------------------------");
    Log("TestDhcp6PdSelectBetweenMultipleServers()");

    InitTest();

    heapAllocations = sHeapAllocatedPtrs.GetLength();

    serverMacAddr1.GenerateRandom();
    serverMacAddr2.GenerateRandom();
    serverMacAddr3.GenerateRandom();

    prefix1 = PrefixFromString("2001:ff::", 64);
    prefix2 = PrefixFromString("2001:0::", 48);
    prefix3 = PrefixFromString("2001:dad0::", 40);

    Log("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ");
    Log("Start the client and wait for the first Solicit message");

    VerifyOrQuit(!sDhcp6ListeningEnabled);
    sInstance->Get<BorderRouter::Dhcp6PdClient>().Start();
    VerifyOrQuit(sDhcp6ListeningEnabled);

    AdvanceTime(1000);

    VerifyOrQuit(sDhcp6RxMsgs.GetLength() == 1);
    sDhcp6RxMsgs[0].ValidateAsSolicit();

    Log("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ");
    Log("Send multiple Advertisements from different servers providing different prefixes");

    txMsg.PrepareAdvertise(sDhcp6RxMsgs[0], serverMacAddr1);
    prefixInfo.mIaid              = sDhcp6RxMsgs[0].mIaPds[0].mIaid;
    prefixInfo.mT1                = 2000;
    prefixInfo.mT2                = 3200;
    prefixInfo.mPreferredLifetime = 3600;
    prefixInfo.mValidLifetime     = 4000;
    prefixInfo.mPrefix            = prefix1;
    txMsg.AddIaPrefix(prefixInfo);
    txMsg.Send();

    txMsg.PrepareAdvertise(sDhcp6RxMsgs[0], serverMacAddr2);
    prefixInfo.mIaid              = sDhcp6RxMsgs[0].mIaPds[0].mIaid;
    prefixInfo.mT1                = 2000;
    prefixInfo.mT2                = 3200;
    prefixInfo.mPreferredLifetime = 3600;
    prefixInfo.mValidLifetime     = 4000;
    prefixInfo.mPrefix            = prefix2;
    txMsg.AddIaPrefix(prefixInfo);
    txMsg.Send();

    txMsg.PrepareAdvertise(sDhcp6RxMsgs[0], serverMacAddr3);
    prefixInfo.mIaid              = sDhcp6RxMsgs[0].mIaPds[0].mIaid;
    prefixInfo.mT1                = 2000;
    prefixInfo.mT2                = 3200;
    prefixInfo.mPreferredLifetime = 3600;
    prefixInfo.mValidLifetime     = 4000;
    prefixInfo.mPrefix            = prefix3;
    txMsg.AddIaPrefix(prefixInfo);
    txMsg.Send();

    Log("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ");
    Log("Validate that the client does wait for the full timeout (on first Solicit) ");

    VerifyOrQuit(sDhcp6RxMsgs.GetLength() == 1);

    // First timeout is at least 1000 msec
    AdvanceNowTo(sDhcp6RxMsgs[0].mRxTime + 1000 - 1);

    VerifyOrQuit(sDhcp6RxMsgs.GetLength() == 1);

    Log("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ");
    Log("Validate Request message is received and its for the preferred prefix from server2");

    AdvanceTime(200);

    VerifyOrQuit(sDhcp6RxMsgs.GetLength() == 2);
    sDhcp6RxMsgs[1].ValidateAsRequest(prefix2, serverMacAddr2);

    Log("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ");
    Log("Stop the client");

    sInstance->Get<BorderRouter::Dhcp6PdClient>().Stop();

    AdvanceTime(5 * 1000);

    VerifyOrQuit(heapAllocations == sHeapAllocatedPtrs.GetLength());

    Log("End of TestDhcp6PdSelectBetweenMultipleServers()");

    FinalizeTest();
}

void TestDhcp6PdServerWithMaxPreferrence(void)
{
    uint16_t               heapAllocations;
    Dhcp6TxMsg             txMsg;
    Dhcp6TxMsg::PrefixInfo prefixInfo;
    Ip6::Prefix            prefix;
    Mac::ExtAddress        serverMacAddr;

    Log("--------------------------------------------------------------------------------------------");
    Log("TestDhcp6PdServerWithMaxPreferrence()");

    InitTest();

    heapAllocations = sHeapAllocatedPtrs.GetLength();

    serverMacAddr.GenerateRandom();

    prefix = PrefixFromString("2001:7::", 64);

    Log("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ");
    Log("Start the client and wait for the first Solicit message");

    VerifyOrQuit(!sDhcp6ListeningEnabled);
    sInstance->Get<BorderRouter::Dhcp6PdClient>().Start();
    VerifyOrQuit(sDhcp6ListeningEnabled);

    AdvanceTime(1000);

    VerifyOrQuit(sDhcp6RxMsgs.GetLength() == 1);
    sDhcp6RxMsgs[0].ValidateAsSolicit();

    // On the first Solicit, the client must wait for the full timeout
    // unless preference is in Advertisement is set to max (255).

    Log("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ");
    Log("Send Advertisement with Preference Option string preference to max to the first Solicit");

    txMsg.PrepareAdvertise(sDhcp6RxMsgs[0], serverMacAddr);
    txMsg.mHasPreference          = true;
    txMsg.mPreference             = 255;
    prefixInfo.mIaid              = sDhcp6RxMsgs[0].mIaPds[0].mIaid;
    prefixInfo.mT1                = 2000;
    prefixInfo.mT2                = 3200;
    prefixInfo.mPreferredLifetime = 3600;
    prefixInfo.mValidLifetime     = 4000;
    prefixInfo.mPrefix            = prefix;
    txMsg.AddIaPrefix(prefixInfo);
    txMsg.Send();

    Log("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ");
    Log("Validate that the client does not wait any longer and sends Request immediately");

    VerifyOrQuit(sDhcp6RxMsgs.GetLength() == 2);
    sDhcp6RxMsgs[1].ValidateAsRequest(prefix, serverMacAddr);

    Log("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ");
    Log("Stop the client");

    sInstance->Get<BorderRouter::Dhcp6PdClient>().Stop();

    AdvanceTime(5 * 1000);

    VerifyOrQuit(heapAllocations == sHeapAllocatedPtrs.GetLength());

    Log("End of TestDhcp6PdServerWithMaxPreferrence()");

    FinalizeTest();
}

//---------------------------------------------------------------------------------------------------------------------

void TestDhcp6PdServerOfferingMultiplePrefixes(void)
{
    uint16_t               heapAllocations;
    Dhcp6TxMsg             txMsg;
    Dhcp6TxMsg::PrefixInfo prefixInfo;
    Ip6::Prefix            prefix1;
    Ip6::Prefix            prefix2;
    Ip6::Prefix            prefix3;
    Mac::ExtAddress        serverMacAddr;

    Log("--------------------------------------------------------------------------------------------");
    Log("TestDhcp6PdServerOfferingMultiplePrefixes()");

    InitTest();

    heapAllocations = sHeapAllocatedPtrs.GetLength();

    serverMacAddr.GenerateRandom();

    prefix1 = PrefixFromString("2001:5:baba:beef::", 64);
    prefix2 = PrefixFromString("2001:4::", 48);
    prefix3 = PrefixFromString("2001:ef::", 40);

    Log("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ");
    Log("Start the client and wait for the first Solicit message");

    VerifyOrQuit(!sDhcp6ListeningEnabled);
    sInstance->Get<BorderRouter::Dhcp6PdClient>().Start();
    VerifyOrQuit(sDhcp6ListeningEnabled);

    AdvanceTime(1000);

    VerifyOrQuit(sDhcp6RxMsgs.GetLength() == 1);
    sDhcp6RxMsgs[0].ValidateAsSolicit();

    Log("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ");
    Log("Send Advertisements from server offering three prefixes");

    txMsg.PrepareAdvertise(sDhcp6RxMsgs[0], serverMacAddr);
    prefixInfo.mIaid              = sDhcp6RxMsgs[0].mIaPds[0].mIaid;
    prefixInfo.mT1                = 2000;
    prefixInfo.mT2                = 3200;
    prefixInfo.mPreferredLifetime = 3600;
    prefixInfo.mValidLifetime     = 4000;
    prefixInfo.mPrefix            = prefix1;
    txMsg.AddIaPrefix(prefixInfo);
    prefixInfo.mPrefix = prefix2;
    txMsg.AddIaPrefix(prefixInfo);
    prefixInfo.mPrefix = prefix3;
    txMsg.AddIaPrefix(prefixInfo);
    txMsg.Send();

    Log("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ");
    Log("Validate that the client does wait for the full timeout (on first Solicit) ");

    VerifyOrQuit(sDhcp6RxMsgs.GetLength() == 1);
    AdvanceNowTo(sDhcp6RxMsgs[0].mRxTime + 1000 - 1);
    VerifyOrQuit(sDhcp6RxMsgs.GetLength() == 1);

    Log("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ");
    Log("Validate Request message is received and its for the preferred prefix");

    AdvanceTime(200);

    VerifyOrQuit(sDhcp6RxMsgs.GetLength() == 2);
    sDhcp6RxMsgs[1].ValidateAsRequest(prefix2, serverMacAddr);

    Log("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ");
    Log("Stop the client");

    sInstance->Get<BorderRouter::Dhcp6PdClient>().Stop();

    AdvanceTime(5 * 1000);

    VerifyOrQuit(heapAllocations == sHeapAllocatedPtrs.GetLength());

    Log("End of TestDhcp6PdServerOfferingMultiplePrefixes()");

    FinalizeTest();
}

void TestDhcp6PdInvalidOrUnusablePrefix(void)
{
    uint16_t               heapAllocations;
    Dhcp6TxMsg             txMsg;
    Dhcp6TxMsg::PrefixInfo prefixInfo;
    Ip6::Prefix            prefix;
    Mac::ExtAddress        serverMacAddr;
    Dhcp6RxMsg             modifiedSolicitRxMsg;
    Mac::ExtAddress        modifiedClientMacAddr;

    Log("--------------------------------------------------------------------------------------------");
    Log("TestDhcp6PdInvalidOrUnusablePrefix");

    InitTest();

    heapAllocations = sHeapAllocatedPtrs.GetLength();

    serverMacAddr.GenerateRandom();

    prefix = PrefixFromString("2001:9::", 64);

    Log("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ");
    Log("Start the client and wait for the first two Solicit messages");

    VerifyOrQuit(!sDhcp6ListeningEnabled);
    sInstance->Get<BorderRouter::Dhcp6PdClient>().Start();
    VerifyOrQuit(sDhcp6ListeningEnabled);

    AdvanceTime(2200);

    VerifyOrQuit(sDhcp6RxMsgs.GetLength() == 2);
    sDhcp6RxMsgs[0].ValidateAsSolicit();
    sDhcp6RxMsgs[1].ValidateAsSolicit();
    VerifyOrQuit(sDhcp6RxMsgs[0].mTransactionId == sDhcp6RxMsgs[1].mTransactionId);

    Log("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ");
    Log("Send Advertisement with wrong TransactionId and ensure it is not accepted");

    modifiedSolicitRxMsg = sDhcp6RxMsgs[0];

    do
    {
        modifiedSolicitRxMsg.mTransactionId.GenerateRandom();
    } while (modifiedSolicitRxMsg.mTransactionId == sDhcp6RxMsgs[0].mTransactionId);

    txMsg.PrepareAdvertise(modifiedSolicitRxMsg, serverMacAddr);
    prefixInfo.mIaid              = modifiedSolicitRxMsg.mIaPds[0].mIaid;
    prefixInfo.mT1                = 2000;
    prefixInfo.mT2                = 3200;
    prefixInfo.mPreferredLifetime = 3600;
    prefixInfo.mValidLifetime     = 4000;
    prefixInfo.mPrefix            = prefix;
    txMsg.AddIaPrefix(prefixInfo);

    txMsg.Send();
    AdvanceTime(1);
    VerifyOrQuit(sDhcp6RxMsgs.GetLength() == 2);

    Log("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ");
    Log("Send Advertisement with wrong Client ID and ensure it is not accepted");

    modifiedSolicitRxMsg = sDhcp6RxMsgs[0];

    do
    {
        modifiedClientMacAddr.GenerateRandom();
    } while (modifiedClientMacAddr == sDhcp6RxMsgs[0].mClientDuid.mShared.mEui64.GetLinkLayerAddress());

    modifiedSolicitRxMsg.mClientDuid.mShared.mEui64.Init(modifiedClientMacAddr);

    txMsg.PrepareAdvertise(modifiedSolicitRxMsg, serverMacAddr);
    prefixInfo.mIaid              = modifiedSolicitRxMsg.mIaPds[0].mIaid;
    prefixInfo.mT1                = 2000;
    prefixInfo.mT2                = 3200;
    prefixInfo.mPreferredLifetime = 3600;
    prefixInfo.mValidLifetime     = 4000;
    prefixInfo.mPrefix            = prefix;
    txMsg.AddIaPrefix(prefixInfo);

    txMsg.Send();
    AdvanceTime(1);
    VerifyOrQuit(sDhcp6RxMsgs.GetLength() == 2);

    Log("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ");
    Log("Send Advertisement with no Client ID and ensure it is not accepted");

    txMsg.PrepareAdvertise(sDhcp6RxMsgs[0], serverMacAddr);
    txMsg.mHasClientId            = false;
    prefixInfo.mIaid              = sDhcp6RxMsgs[0].mIaPds[0].mIaid;
    prefixInfo.mT1                = 2000;
    prefixInfo.mT2                = 3200;
    prefixInfo.mPreferredLifetime = 3600;
    prefixInfo.mValidLifetime     = 4000;
    prefixInfo.mPrefix            = prefix;
    txMsg.AddIaPrefix(prefixInfo);

    txMsg.Send();
    AdvanceTime(1);
    VerifyOrQuit(sDhcp6RxMsgs.GetLength() == 2);

    Log("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ");
    Log("Send Advertisement with no Server ID and ensure it is not accepted");

    txMsg.PrepareAdvertise(sDhcp6RxMsgs[0], serverMacAddr);
    txMsg.mHasServerId            = false;
    prefixInfo.mIaid              = sDhcp6RxMsgs[0].mIaPds[0].mIaid;
    prefixInfo.mT1                = 2000;
    prefixInfo.mT2                = 3200;
    prefixInfo.mPreferredLifetime = 3600;
    prefixInfo.mValidLifetime     = 4000;
    prefixInfo.mPrefix            = prefix;
    txMsg.AddIaPrefix(prefixInfo);

    txMsg.Send();
    AdvanceTime(1);
    VerifyOrQuit(sDhcp6RxMsgs.GetLength() == 2);

    Log("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ");
    Log("Send Advertisement with a wrong `Iaid` and ensure it is not accepted");

    txMsg.PrepareAdvertise(sDhcp6RxMsgs[0], serverMacAddr);
    prefixInfo.mIaid              = sDhcp6RxMsgs[0].mIaPds[0].mIaid + 1;
    prefixInfo.mT1                = 2000;
    prefixInfo.mT2                = 3200;
    prefixInfo.mPreferredLifetime = 3600;
    prefixInfo.mValidLifetime     = 4000;
    prefixInfo.mPrefix            = prefix;
    txMsg.AddIaPrefix(prefixInfo);

    txMsg.Send();
    AdvanceTime(1);
    VerifyOrQuit(sDhcp6RxMsgs.GetLength() == 2);

    Log("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ");
    Log("Send Advertisement with a long prefix and ensure it is not accepted");

    txMsg.PrepareAdvertise(sDhcp6RxMsgs[0], serverMacAddr);
    prefixInfo.mIaid              = sDhcp6RxMsgs[0].mIaPds[0].mIaid;
    prefixInfo.mT1                = 2000;
    prefixInfo.mT2                = 3200;
    prefixInfo.mPreferredLifetime = 3600;
    prefixInfo.mValidLifetime     = 4000;
    prefixInfo.mPrefix            = PrefixFromString("2001:dead:beef:cafe::", 65);
    txMsg.AddIaPrefix(prefixInfo);

    txMsg.Send();
    AdvanceTime(1);
    VerifyOrQuit(sDhcp6RxMsgs.GetLength() == 2);

    Log("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ");
    Log("Send Advertisement T1 longer than T2 and ensure it is not accepted");

    txMsg.PrepareAdvertise(sDhcp6RxMsgs[0], serverMacAddr);
    prefixInfo.mIaid              = sDhcp6RxMsgs[0].mIaPds[0].mIaid;
    prefixInfo.mT1                = 3000;
    prefixInfo.mT2                = 2000;
    prefixInfo.mPreferredLifetime = 3600;
    prefixInfo.mValidLifetime     = 4000;
    prefixInfo.mPrefix            = prefix;
    txMsg.AddIaPrefix(prefixInfo);

    txMsg.Send();
    AdvanceTime(1);
    VerifyOrQuit(sDhcp6RxMsgs.GetLength() == 2);

    Log("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ");
    Log("Send Advertisement preferred lifetime longer than valid lifetime and ensure it is not accepted");

    txMsg.PrepareAdvertise(sDhcp6RxMsgs[0], serverMacAddr);
    prefixInfo.mIaid              = sDhcp6RxMsgs[0].mIaPds[0].mIaid;
    prefixInfo.mT1                = 2000;
    prefixInfo.mT2                = 3200;
    prefixInfo.mPreferredLifetime = 4000;
    prefixInfo.mValidLifetime     = 3600;
    prefixInfo.mPrefix            = prefix;
    txMsg.AddIaPrefix(prefixInfo);

    txMsg.Send();
    AdvanceTime(1);
    VerifyOrQuit(sDhcp6RxMsgs.GetLength() == 2);

    Log("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ");
    Log("Send Advertisement with preferred lifetime below the min required threshold of 30 min");

    txMsg.PrepareAdvertise(sDhcp6RxMsgs[0], serverMacAddr);
    prefixInfo.mIaid              = sDhcp6RxMsgs[0].mIaPds[0].mIaid;
    prefixInfo.mT1                = 900;
    prefixInfo.mT2                = 1000;
    prefixInfo.mPreferredLifetime = 1799;
    prefixInfo.mValidLifetime     = 4000;
    prefixInfo.mPrefix            = prefix;
    txMsg.AddIaPrefix(prefixInfo);

    txMsg.Send();
    AdvanceTime(1);
    VerifyOrQuit(sDhcp6RxMsgs.GetLength() == 2);

    Log("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ");
    Log("Finally send Advertisement with everything valid and ensure it is accepted");

    txMsg.PrepareAdvertise(sDhcp6RxMsgs[0], serverMacAddr);
    prefixInfo.mIaid              = sDhcp6RxMsgs[0].mIaPds[0].mIaid;
    prefixInfo.mT1                = 0;
    prefixInfo.mT2                = 0;
    prefixInfo.mPreferredLifetime = 1800;
    prefixInfo.mValidLifetime     = 4000;
    prefixInfo.mPrefix            = prefix;
    txMsg.AddIaPrefix(prefixInfo);

    txMsg.Send();
    AdvanceTime(1);
    VerifyOrQuit(sDhcp6RxMsgs.GetLength() == 3);

    sDhcp6RxMsgs[2].ValidateAsRequest(prefix, serverMacAddr);

    Log("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ");
    Log("Stop the client");

    sInstance->Get<BorderRouter::Dhcp6PdClient>().Stop();
    AdvanceTime(15 * 1000);

    VerifyOrQuit(heapAllocations == sHeapAllocatedPtrs.GetLength());

    Log("End of TestDhcp6PdInvalidOrUnusablePrefix");

    FinalizeTest();
}

void TestDhcp6PdLifetimeT1AndT2Adjustments(void)
{
    struct TestCase
    {
        uint32_t mT1;
        uint32_t mT2;
        uint32_t mPreferredLifetime;
        uint32_t mExpectedT1;
        uint32_t mExpectedT2;
        uint32_t mExpectedPreferredLifetime;
    };

    // Validate client will adjust the T1 and T2 and lifetime to reasonable range.

    static const TestCase kTestCases[] = {
        // T1 and T2 are zero. Client must pick 0.5 and 0.8 times lifetime for T1 and T2.
        {0, 0, 1800, 900, 1440, 1800},

        // Only T1 is zero.
        {0, 1300, 1800, 900, 1300, 1800},

        // Only T2 is zero.
        {800, 0, 1800, 800, 1440, 1800},

        // T1 is zero, but default T1 (half of preferred lifetime) will be larger than given T2.
        {0, 800, 1800, 900, 900, 1800},

        // T1 and T2 are given but way too short. Client enforces min of 300s (5 min).
        {1, 5, 1800, 300, 300, 1800},

        // T1 and T2 zero with preferred lifetime of 7200 (2 hours).
        {0, 0, 7200, 3600, 5760, 7200},

        // T1 and T2 longer than lifetime.
        {2000, 2500, 1800, 900, 1440, 1800},

        // Given T1 and T2 (shorter than 0.5 and 0.8) with preferred lifetime of 7200 (2 hours).
        {1000, 1200, 7200, 1000, 1200, 7200},

        // Given T1 and T2 are too close to the preferred lifetime of 7200 (2 hours).
        {7100, 7150, 7200, 6300, 6840, 7200},

        // Very long preferred lifetime. Client limit it to 4 hours (14400)
        {0, 0, 14500, 7200, 11520, 14400},

        // Very long preferred lifetime. Client limit it to 4 hours (14400)
        {2000, 2500, 14500, 2000, 2500, 14400},

        // Infinite lifetime and T1 and T2. Client limit to 4 hours.
        {0xffffffff, 0xffffffff, 0xffffffff, 13500, 14040, 14400}};

    uint16_t               heapAllocations;
    Dhcp6TxMsg             txMsg;
    Dhcp6TxMsg::PrefixInfo prefixInfo;
    Ip6::Prefix            prefix;
    Ip6::Prefix            adjustedPrefix;
    Mac::ExtAddress        serverMacAddr;
    const DelegatedPrefix *delegatedPrefix;

    Log("--------------------------------------------------------------------------------------------");
    Log("TestDhcp6PdLifetimeT1AndT2Adjustments()");

    InitTest();

    heapAllocations = sHeapAllocatedPtrs.GetLength();

    serverMacAddr.GenerateRandom();

    prefix         = PrefixFromString("2001:13::", 48);
    adjustedPrefix = PrefixFromString("2001:13::", 64);

    for (const TestCase &testCase : kTestCases)
    {
        Log("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ");
        Log("Test case: {T1:%lu T2:%lu prf:%lu } --> Expected {T1:%lu T2:%lu prf:%lu}", ToUlong(testCase.mT1),
            ToUlong(testCase.mT2), ToUlong(testCase.mPreferredLifetime), ToUlong(testCase.mExpectedT1),
            ToUlong(testCase.mExpectedT2), ToUlong(testCase.mExpectedPreferredLifetime));

        sDhcp6RxMsgs.Clear();
        sInstance->Get<BorderRouter::Dhcp6PdClient>().Start();

        AdvanceTime(2200);

        VerifyOrQuit(sDhcp6RxMsgs.GetLength() == 2);
        sDhcp6RxMsgs[0].ValidateAsSolicit();
        sDhcp6RxMsgs[1].ValidateAsSolicit();
        VerifyOrQuit(sDhcp6RxMsgs[0].mTransactionId == sDhcp6RxMsgs[1].mTransactionId);

        txMsg.PrepareAdvertise(sDhcp6RxMsgs[0], serverMacAddr);

        prefixInfo.mIaid              = sDhcp6RxMsgs[0].mIaPds[0].mIaid;
        prefixInfo.mT1                = testCase.mT1;
        prefixInfo.mT2                = testCase.mT2;
        prefixInfo.mPreferredLifetime = testCase.mPreferredLifetime;
        prefixInfo.mValidLifetime     = testCase.mPreferredLifetime;
        prefixInfo.mPrefix            = prefix;
        txMsg.AddIaPrefix(prefixInfo);

        sDhcp6RxMsgs.Clear();
        txMsg.Send();

        AdvanceTime(1);

        VerifyOrQuit(sDhcp6RxMsgs.GetLength() == 1);
        sDhcp6RxMsgs[0].ValidateAsRequest(prefix, serverMacAddr);

        sDhcp6RxMsgs.Clear();
        txMsg.PrepareReply(sDhcp6RxMsgs[0], serverMacAddr);
        txMsg.AddIaPrefix(prefixInfo);
        txMsg.Send();

        delegatedPrefix = sInstance->Get<BorderRouter::Dhcp6PdClient>().GetDelegatedPrefix();
        VerifyOrQuit(delegatedPrefix != nullptr);

        Log("Delegated Prefix -> {T1:%lu T2:%lu prf:%lu}", ToUlong(delegatedPrefix->mT1), ToUlong(delegatedPrefix->mT2),
            ToUlong(delegatedPrefix->mPreferredLifetime));

        VerifyOrQuit(delegatedPrefix->mPrefix == prefix);
        VerifyOrQuit(delegatedPrefix->mAdjustedPrefix == adjustedPrefix);
        VerifyOrQuit(delegatedPrefix->mT1 == testCase.mExpectedT1);
        VerifyOrQuit(delegatedPrefix->mT2 == testCase.mExpectedT2);
        VerifyOrQuit(delegatedPrefix->mPreferredLifetime == testCase.mExpectedPreferredLifetime);

        sInstance->Get<BorderRouter::Dhcp6PdClient>().Stop();
        AdvanceTime(100);
    }

    VerifyOrQuit(heapAllocations == sHeapAllocatedPtrs.GetLength());

    Log("End of TestDhcp6PdLifetimeT1AndT2Adjustments");

    FinalizeTest();
}

void TestDhcp6PdServerVoidingLeaseDuringRenew(void)
{
    uint16_t               heapAllocations;
    Dhcp6TxMsg             txMsg;
    Dhcp6TxMsg::PrefixInfo prefixInfo;
    Ip6::Prefix            prefix;
    Ip6::Prefix            adjustedPrefix;
    Mac::ExtAddress        serverMacAddr;
    const DelegatedPrefix *delegatedPrefix;

    Log("--------------------------------------------------------------------------------------------");
    Log("TestDhcp6PdServerVoidingLeaseDuringRenew()");

    InitTest();

    heapAllocations = sHeapAllocatedPtrs.GetLength();

    serverMacAddr.GenerateRandom();

    prefix         = PrefixFromString("2001:cafe:5555::", 48);
    adjustedPrefix = PrefixFromString("2001:cafe:5555::", 64);

    Log("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ");
    Log("Start client, interact with it to successfully delegate a prefix");

    sInstance->Get<BorderRouter::Dhcp6PdClient>().Start();

    AdvanceTime(2200);

    // Listen for Solicit and send Advertise
    VerifyOrQuit(sDhcp6RxMsgs.GetLength() == 2);
    sDhcp6RxMsgs[0].ValidateAsSolicit();
    sDhcp6RxMsgs[1].ValidateAsSolicit();
    VerifyOrQuit(sDhcp6RxMsgs[0].mTransactionId == sDhcp6RxMsgs[1].mTransactionId);

    txMsg.PrepareAdvertise(sDhcp6RxMsgs[0], serverMacAddr);
    prefixInfo.mIaid              = sDhcp6RxMsgs[0].mIaPds[0].mIaid;
    prefixInfo.mT1                = 0;
    prefixInfo.mT2                = 0;
    prefixInfo.mPreferredLifetime = 1800;
    prefixInfo.mValidLifetime     = 1800;
    prefixInfo.mPrefix            = prefix;
    txMsg.AddIaPrefix(prefixInfo);
    sDhcp6RxMsgs.Clear();
    txMsg.Send();

    AdvanceTime(1);

    // Listen for Request and send Reply
    VerifyOrQuit(sDhcp6RxMsgs.GetLength() == 1);
    sDhcp6RxMsgs[0].ValidateAsRequest(prefix, serverMacAddr);

    sDhcp6RxMsgs.Clear();
    txMsg.PrepareReply(sDhcp6RxMsgs[0], serverMacAddr);
    txMsg.AddIaPrefix(prefixInfo);
    txMsg.Send();

    delegatedPrefix = sInstance->Get<BorderRouter::Dhcp6PdClient>().GetDelegatedPrefix();
    VerifyOrQuit(delegatedPrefix != nullptr);

    VerifyOrQuit(delegatedPrefix->mPrefix == prefix);
    VerifyOrQuit(delegatedPrefix->mAdjustedPrefix == adjustedPrefix);
    VerifyOrQuit(delegatedPrefix->mT1 == 900);
    VerifyOrQuit(delegatedPrefix->mT2 == 1440);
    VerifyOrQuit(delegatedPrefix->mPreferredLifetime == 1800);

    Log("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ");
    Log("Wait till T1 time for client to renew");

    AdvanceTime(900 * 1000 + 10);

    VerifyOrQuit(sDhcp6RxMsgs.GetLength() == 1);
    sDhcp6RxMsgs[0].ValidateAsRenew(prefix, serverMacAddr);

    Log("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ");
    Log("Send a Reply invalidating the previously delegated prefix");

    txMsg.PrepareReply(sDhcp6RxMsgs[0], serverMacAddr);
    prefixInfo.mIaid              = sDhcp6RxMsgs[0].mIaPds[0].mIaid;
    prefixInfo.mT1                = 0;
    prefixInfo.mT2                = 0;
    prefixInfo.mPreferredLifetime = 0;
    prefixInfo.mValidLifetime     = 0;
    prefixInfo.mPrefix            = prefix;
    txMsg.AddIaPrefix(prefixInfo);
    sDhcp6RxMsgs.Clear();
    txMsg.Send();

    AdvanceTime(1);

    delegatedPrefix = sInstance->Get<BorderRouter::Dhcp6PdClient>().GetDelegatedPrefix();
    VerifyOrQuit(delegatedPrefix == nullptr);

    Log("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ");

    sInstance->Get<BorderRouter::Dhcp6PdClient>().Stop();
    AdvanceTime(5 * 1000);

    VerifyOrQuit(heapAllocations == sHeapAllocatedPtrs.GetLength());

    Log("End of TestDhcp6PdServerVoidingLeaseDuringRenew");

    FinalizeTest();
}

void TestDhcp6PdServerNotExtendingLeaseDuringRenew(void)
{
    uint16_t               heapAllocations;
    Dhcp6TxMsg             txMsg;
    Dhcp6TxMsg::PrefixInfo prefixInfo;
    Ip6::Prefix            prefix;
    Ip6::Prefix            adjustedPrefix;
    Mac::ExtAddress        serverMacAddr;
    const DelegatedPrefix *delegatedPrefix;

    Log("--------------------------------------------------------------------------------------------");
    Log("TestDhcp6PdServerNotExtendingLeaseDuringRenew()");

    InitTest();

    heapAllocations = sHeapAllocatedPtrs.GetLength();

    serverMacAddr.GenerateRandom();

    prefix         = PrefixFromString("2001:4567::", 48);
    adjustedPrefix = PrefixFromString("2001:4567::", 64);

    Log("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ");
    Log("Start client, interact with it to successfully delegate a prefix");

    sInstance->Get<BorderRouter::Dhcp6PdClient>().Start();

    AdvanceTime(2200);

    // Listen for Solicit and send Advertise
    VerifyOrQuit(sDhcp6RxMsgs.GetLength() == 2);
    sDhcp6RxMsgs[0].ValidateAsSolicit();
    sDhcp6RxMsgs[1].ValidateAsSolicit();
    VerifyOrQuit(sDhcp6RxMsgs[0].mTransactionId == sDhcp6RxMsgs[1].mTransactionId);

    txMsg.PrepareAdvertise(sDhcp6RxMsgs[0], serverMacAddr);
    prefixInfo.mIaid              = sDhcp6RxMsgs[0].mIaPds[0].mIaid;
    prefixInfo.mT1                = 0;
    prefixInfo.mT2                = 0;
    prefixInfo.mPreferredLifetime = 1800;
    prefixInfo.mValidLifetime     = 1800;
    prefixInfo.mPrefix            = prefix;
    txMsg.AddIaPrefix(prefixInfo);
    sDhcp6RxMsgs.Clear();
    txMsg.Send();

    AdvanceTime(1);

    // Listen for Request and send Reply
    VerifyOrQuit(sDhcp6RxMsgs.GetLength() == 1);
    sDhcp6RxMsgs[0].ValidateAsRequest(prefix, serverMacAddr);

    sDhcp6RxMsgs.Clear();
    txMsg.PrepareReply(sDhcp6RxMsgs[0], serverMacAddr);
    txMsg.AddIaPrefix(prefixInfo);
    txMsg.Send();

    delegatedPrefix = sInstance->Get<BorderRouter::Dhcp6PdClient>().GetDelegatedPrefix();
    VerifyOrQuit(delegatedPrefix != nullptr);

    VerifyOrQuit(delegatedPrefix->mPrefix == prefix);
    VerifyOrQuit(delegatedPrefix->mAdjustedPrefix == adjustedPrefix);
    VerifyOrQuit(delegatedPrefix->mT1 == 900);
    VerifyOrQuit(delegatedPrefix->mT2 == 1440);
    VerifyOrQuit(delegatedPrefix->mPreferredLifetime == 1800);

    Log("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ");
    Log("Wait till T1 time for client to renew");

    AdvanceTime(900 * 1000 + 10);

    VerifyOrQuit(sDhcp6RxMsgs.GetLength() == 1);
    sDhcp6RxMsgs[0].ValidateAsRenew(prefix, serverMacAddr);

    Log("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ");
    Log("Send a Reply including the previously delegated prefix but with short lifetime");

    txMsg.PrepareReply(sDhcp6RxMsgs[0], serverMacAddr);
    prefixInfo.mIaid              = sDhcp6RxMsgs[0].mIaPds[0].mIaid;
    prefixInfo.mT1                = 0;
    prefixInfo.mT2                = 0;
    prefixInfo.mPreferredLifetime = 100;
    prefixInfo.mValidLifetime     = 100;
    prefixInfo.mPrefix            = prefix;
    txMsg.AddIaPrefix(prefixInfo);
    sDhcp6RxMsgs.Clear();
    txMsg.Send();

    AdvanceTime(1);

    delegatedPrefix = sInstance->Get<BorderRouter::Dhcp6PdClient>().GetDelegatedPrefix();
    VerifyOrQuit(delegatedPrefix != nullptr);

    VerifyOrQuit(delegatedPrefix->mPrefix == prefix);
    VerifyOrQuit(delegatedPrefix->mAdjustedPrefix == adjustedPrefix);
    VerifyOrQuit(delegatedPrefix->mT1 == 100);
    VerifyOrQuit(delegatedPrefix->mT2 == 100);
    VerifyOrQuit(delegatedPrefix->mPreferredLifetime == 100);

    Log("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ");
    Log("Wait for the short lifetime to expire and validate that the delegated prefix is removed");

    AdvanceTime(100 * 1000 + 1);

    delegatedPrefix = sInstance->Get<BorderRouter::Dhcp6PdClient>().GetDelegatedPrefix();
    VerifyOrQuit(delegatedPrefix == nullptr);

    AdvanceTime(5 * 1000);

    VerifyOrQuit(!sDhcp6RxMsgs.IsEmpty());
    sDhcp6RxMsgs[0].ValidateAsSolicit();
    VerifyOrQuit(sDhcp6RxMsgs[0].mElapsedTime == 0);

    Log("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ");

    sInstance->Get<BorderRouter::Dhcp6PdClient>().Stop();
    AdvanceTime(5 * 1000);

    VerifyOrQuit(heapAllocations == sHeapAllocatedPtrs.GetLength());

    Log("End of TestDhcp6PdServerNotExtendingLeaseDuringRenew");

    FinalizeTest();
}

void TestDhcp6PdServerReplacingPrefix(void)
{
    uint16_t               heapAllocations;
    Dhcp6TxMsg             txMsg;
    Dhcp6TxMsg::PrefixInfo prefixInfo;
    Ip6::Prefix            prefix1;
    Ip6::Prefix            prefix2;
    Mac::ExtAddress        serverMacAddr;
    const DelegatedPrefix *delegatedPrefix;

    Log("--------------------------------------------------------------------------------------------");
    Log("TestDhcp6PdServerReplacingPrefix()");

    InitTest();

    heapAllocations = sHeapAllocatedPtrs.GetLength();

    serverMacAddr.GenerateRandom();

    prefix1 = PrefixFromString("2001:b2d4:1111::", 64);
    prefix1 = PrefixFromString("2001:b2d4:2222::", 64);

    Log("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ");
    Log("Start client, interact with it to successfully delegate a prefix");

    sInstance->Get<BorderRouter::Dhcp6PdClient>().Start();

    AdvanceTime(2200);

    // Listen for Solicit and send Advertise
    VerifyOrQuit(sDhcp6RxMsgs.GetLength() == 2);
    sDhcp6RxMsgs[0].ValidateAsSolicit();
    sDhcp6RxMsgs[1].ValidateAsSolicit();
    VerifyOrQuit(sDhcp6RxMsgs[0].mTransactionId == sDhcp6RxMsgs[1].mTransactionId);

    txMsg.PrepareAdvertise(sDhcp6RxMsgs[0], serverMacAddr);
    prefixInfo.mIaid              = sDhcp6RxMsgs[0].mIaPds[0].mIaid;
    prefixInfo.mT1                = 0;
    prefixInfo.mT2                = 0;
    prefixInfo.mPreferredLifetime = 1800;
    prefixInfo.mValidLifetime     = 1800;
    prefixInfo.mPrefix            = prefix1;
    txMsg.AddIaPrefix(prefixInfo);
    sDhcp6RxMsgs.Clear();
    txMsg.Send();

    AdvanceTime(1);

    // Listen for Request and send Reply
    VerifyOrQuit(sDhcp6RxMsgs.GetLength() == 1);
    sDhcp6RxMsgs[0].ValidateAsRequest(prefix1, serverMacAddr);

    sDhcp6RxMsgs.Clear();
    txMsg.PrepareReply(sDhcp6RxMsgs[0], serverMacAddr);
    txMsg.AddIaPrefix(prefixInfo);
    txMsg.Send();

    delegatedPrefix = sInstance->Get<BorderRouter::Dhcp6PdClient>().GetDelegatedPrefix();
    VerifyOrQuit(delegatedPrefix != nullptr);

    VerifyOrQuit(delegatedPrefix->mPrefix == prefix1);
    VerifyOrQuit(delegatedPrefix->mAdjustedPrefix == prefix1);
    VerifyOrQuit(delegatedPrefix->mT1 == 900);
    VerifyOrQuit(delegatedPrefix->mT2 == 1440);
    VerifyOrQuit(delegatedPrefix->mPreferredLifetime == 1800);

    Log("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ");
    Log("Wait till T1 time for client to renew");

    AdvanceTime(900 * 1000 + 10);

    VerifyOrQuit(sDhcp6RxMsgs.GetLength() == 1);
    sDhcp6RxMsgs[0].ValidateAsRenew(prefix1, serverMacAddr);

    Log("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ");
    Log("Send a Reply including a new prefix with old one with short lifetime");

    txMsg.PrepareReply(sDhcp6RxMsgs[0], serverMacAddr);

    prefixInfo.mIaid              = sDhcp6RxMsgs[0].mIaPds[0].mIaid;
    prefixInfo.mT1                = 0;
    prefixInfo.mT2                = 0;
    prefixInfo.mPreferredLifetime = 120;
    prefixInfo.mValidLifetime     = 120;
    prefixInfo.mPrefix            = prefix1;
    txMsg.AddIaPrefix(prefixInfo);

    prefixInfo.mPreferredLifetime = 1800;
    prefixInfo.mValidLifetime     = 1800;
    prefixInfo.mPrefix            = prefix2;
    txMsg.AddIaPrefix(prefixInfo);

    sDhcp6RxMsgs.Clear();
    txMsg.Send();

    AdvanceTime(1);

    delegatedPrefix = sInstance->Get<BorderRouter::Dhcp6PdClient>().GetDelegatedPrefix();
    VerifyOrQuit(delegatedPrefix != nullptr);

    VerifyOrQuit(delegatedPrefix->mPrefix == prefix2);
    VerifyOrQuit(delegatedPrefix->mAdjustedPrefix == prefix2);
    VerifyOrQuit(delegatedPrefix->mT1 == 900);
    VerifyOrQuit(delegatedPrefix->mT2 == 1440);
    VerifyOrQuit(delegatedPrefix->mPreferredLifetime == 1800);

    Log("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ");

    sInstance->Get<BorderRouter::Dhcp6PdClient>().Stop();
    AdvanceTime(5 * 1000);

    VerifyOrQuit(heapAllocations == sHeapAllocatedPtrs.GetLength());

    Log("End of TestDhcp6PdServerReplacingPrefix");

    FinalizeTest();
}

//---------------------------------------------------------------------------------------------------------------------

void TestDhcp6PdServerStatusCodeUseMulticast(void)
{
    uint16_t               heapAllocations;
    Dhcp6TxMsg             txMsg;
    Dhcp6TxMsg::PrefixInfo prefixInfo;
    Ip6::Prefix            prefix;
    Ip6::Prefix            adjustedPrefix;
    Mac::ExtAddress        serverMacAddr;
    const DelegatedPrefix *delegatedPrefix;
    Ip6::Address           serverIp6Addr;

    Log("--------------------------------------------------------------------------------------------");
    Log("TestDhcp6PdServerStatusCodeUseMulticast()");

    InitTest();

    heapAllocations = sHeapAllocatedPtrs.GetLength();

    serverMacAddr.GenerateRandom();

    prefix         = PrefixFromString("2001:f57c::", 48);
    adjustedPrefix = PrefixFromString("2001:f57c::", 64);

    serverIp6Addr = AddressFromString("fe80::2");

    Log("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ");
    Log("Start the client and wait for the first two Solicit messages");

    VerifyOrQuit(!sDhcp6ListeningEnabled);
    sInstance->Get<BorderRouter::Dhcp6PdClient>().Start();
    VerifyOrQuit(sDhcp6ListeningEnabled);

    AdvanceTime(2200);

    VerifyOrQuit(sDhcp6RxMsgs.GetLength() == 2);
    sDhcp6RxMsgs[0].ValidateAsSolicit();
    sDhcp6RxMsgs[1].ValidateAsSolicit();
    VerifyOrQuit(sDhcp6RxMsgs[0].mTransactionId == sDhcp6RxMsgs[1].mTransactionId);

    Log("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ");
    Log("Send Advertisement");

    txMsg.PrepareAdvertise(sDhcp6RxMsgs[0], serverMacAddr, &serverIp6Addr);

    prefixInfo.mIaid              = sDhcp6RxMsgs[0].mIaPds[0].mIaid;
    prefixInfo.mT1                = 2000;
    prefixInfo.mT2                = 3200;
    prefixInfo.mPreferredLifetime = 3600;
    prefixInfo.mValidLifetime     = 4000;
    prefixInfo.mPrefix            = prefix;
    txMsg.AddIaPrefix(prefixInfo);

    sDhcp6RxMsgs.Clear();

    txMsg.Send();

    AdvanceTime(1);

    VerifyOrQuit(sInstance->Get<BorderRouter::Dhcp6PdClient>().GetDelegatedPrefix() == nullptr);

    Log("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ");
    Log("Validate Request message is received using unicast address of server");

    VerifyOrQuit(sDhcp6RxMsgs.GetLength() == 1);
    sDhcp6RxMsgs[0].ValidateAsRequest(prefix, serverMacAddr, &serverIp6Addr);

    Log("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ");
    Log("Send Reply message with status code UseMulticast");

    sDhcp6RxMsgs.Clear();

    txMsg.PrepareReply(sDhcp6RxMsgs[0], serverMacAddr);
    txMsg.mHasStatus  = true;
    txMsg.mStatusCode = Dhcp6::StatusCodeOption::kUseMulticast;

    sDhcp6RxMsgs.Clear();
    txMsg.Send();

    Log("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ");
    Log("Validate Request message is sent again now as a multicast");

    VerifyOrQuit(sDhcp6RxMsgs.GetLength() == 1);
    sDhcp6RxMsgs[0].ValidateAsRequest(prefix, serverMacAddr, nullptr);

    Log("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ");

    VerifyOrQuit(heapAllocations == sHeapAllocatedPtrs.GetLength());

    Log("End of TestDhcp6PdServerStatusCodeUseMulticast");

    FinalizeTest();
}

//---------------------------------------------------------------------------------------------------------------------

void TestDhcp6PdServerReplyWithNoBindingToRelease(void)
{
    uint16_t               heapAllocations;
    Dhcp6TxMsg             txMsg;
    Dhcp6TxMsg::PrefixInfo prefixInfo;
    Ip6::Prefix            prefix;
    Mac::ExtAddress        serverMacAddr;
    const DelegatedPrefix *delegatedPrefix;

    Log("--------------------------------------------------------------------------------------------");
    Log("TestDhcp6PdServerReplyWithNoBindingToRelease()");

    InitTest();

    heapAllocations = sHeapAllocatedPtrs.GetLength();

    serverMacAddr.GenerateRandom();

    prefix = PrefixFromString("2001:8765::", 64);

    Log("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ");
    Log("Start the client and wait for the first two Solicit messages");

    VerifyOrQuit(!sDhcp6ListeningEnabled);
    sInstance->Get<BorderRouter::Dhcp6PdClient>().Start();
    VerifyOrQuit(sDhcp6ListeningEnabled);

    AdvanceTime(2200);

    VerifyOrQuit(sDhcp6RxMsgs.GetLength() == 2);
    sDhcp6RxMsgs[0].ValidateAsSolicit();
    sDhcp6RxMsgs[1].ValidateAsSolicit();
    VerifyOrQuit(sDhcp6RxMsgs[0].mTransactionId == sDhcp6RxMsgs[1].mTransactionId);

    Log("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ");
    Log("Send Advertisements from server offering three prefixes");

    txMsg.PrepareAdvertise(sDhcp6RxMsgs[0], serverMacAddr);
    prefixInfo.mIaid              = sDhcp6RxMsgs[0].mIaPds[0].mIaid;
    prefixInfo.mT1                = 0;
    prefixInfo.mT2                = 0;
    prefixInfo.mPreferredLifetime = 5000;
    prefixInfo.mValidLifetime     = 5000;
    prefixInfo.mPrefix            = prefix;
    txMsg.AddIaPrefix(prefixInfo);

    sDhcp6RxMsgs.Clear();
    txMsg.Send();

    Log("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ");
    Log("Validate Request message is received");

    AdvanceTime(1);

    VerifyOrQuit(sDhcp6RxMsgs.GetLength() == 1);
    sDhcp6RxMsgs[0].ValidateAsRequest(prefix, serverMacAddr);

    Log("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ");
    Log("Send Reply message");

    txMsg.PrepareReply(sDhcp6RxMsgs[0], serverMacAddr);

    prefixInfo.mIaid = sDhcp6RxMsgs[0].mIaPds[0].mIaid;
    txMsg.AddIaPrefix(prefixInfo);

    sDhcp6RxMsgs.Clear();
    txMsg.Send();

    delegatedPrefix = sInstance->Get<BorderRouter::Dhcp6PdClient>().GetDelegatedPrefix();
    VerifyOrQuit(delegatedPrefix != nullptr);

    VerifyOrQuit(delegatedPrefix->mPrefix == prefix);
    VerifyOrQuit(delegatedPrefix->mAdjustedPrefix == prefix);
    VerifyOrQuit(delegatedPrefix->mPreferredLifetime == prefixInfo.mPreferredLifetime);

    sDhcp6RxMsgs.Clear();

    Log("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ");
    Log("Stop the client to release the prefix");

    sInstance->Get<BorderRouter::Dhcp6PdClient>().Stop();

    VerifyOrQuit(sDhcp6RxMsgs.GetLength() == 1);
    sDhcp6RxMsgs[0].ValidateAsRelease(prefix, serverMacAddr);
    VerifyOrQuit(sDhcp6RxMsgs[0].mElapsedTime == 0);

    AdvanceTime(1200);
    VerifyOrQuit(sDhcp6RxMsgs.GetLength() == 2);
    sDhcp6RxMsgs[1].ValidateAsRelease(prefix, serverMacAddr);
    VerifyOrQuit(sDhcp6RxMsgs[1].mTransactionId == sDhcp6RxMsgs[0].mTransactionId);
    VerifyOrQuit(sDhcp6RxMsgs[1].mElapsedTime > 0);

    Log("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ");
    Log("Send a Reply to the Release with status code NoBinding, validate that it is accepted");

    txMsg.PrepareReply(sDhcp6RxMsgs[0], serverMacAddr);
    prefixInfo.mIaid = sDhcp6RxMsgs[0].mIaPds[0].mIaid;
    txMsg.AddIaPrefix(prefixInfo);

    txMsg.mIaPds[0].mHasStatus  = true;
    txMsg.mIaPds[0].mStatusCode = Dhcp6::StatusCodeOption::kNoBinding;

    sDhcp6RxMsgs.Clear();
    txMsg.Send();

    Log("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ");
    Log("Validate that the client accepts the Reply and there are no more retries of Release");

    AdvanceTime(30 * 1000);
    VerifyOrQuit(sDhcp6RxMsgs.IsEmpty());

    Log("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ");

    VerifyOrQuit(heapAllocations == sHeapAllocatedPtrs.GetLength());

    Log("End of TestDhcp6PdServerReplyWithNoBindingToRelease()");

    FinalizeTest();
}

#endif // OT_CONFIG_DHCP6_PD_CLIENT_ENABLE

} // namespace ot

int main(void)
{
#if OT_CONFIG_DHCP6_PD_CLIENT_ENABLE
    ot::TestDhcp6PdPrefixDelegation(/* aShortPrefix */ false, /* aAddServerUnicastOption */ false);
    ot::TestDhcp6PdPrefixDelegation(/* aShortPrefix */ false, /* aAddServerUnicastOption */ true);
    ot::TestDhcp6PdPrefixDelegation(/* aShortPrefix */ true, /* aAddServerUnicastOption */ false);
    ot::TestDhcp6PdPrefixDelegation(/* aShortPrefix */ true, /* aAddServerUnicastOption */ true);
    ot::TestDhcp6PdSolicitRetries();
    ot::TestDhcp6PdRequestRetries();
    ot::TestDhcp6PdSelectBetweenMultipleServers();
    ot::TestDhcp6PdServerWithMaxPreferrence();
    ot::TestDhcp6PdServerOfferingMultiplePrefixes();
    ot::TestDhcp6PdInvalidOrUnusablePrefix();
    ot::TestDhcp6PdLifetimeT1AndT2Adjustments();
    ot::TestDhcp6PdServerVoidingLeaseDuringRenew();
    ot::TestDhcp6PdServerNotExtendingLeaseDuringRenew();
    ot::TestDhcp6PdServerReplacingPrefix();
    ot::TestDhcp6PdServerStatusCodeUseMulticast();
    ot::TestDhcp6PdServerReplyWithNoBindingToRelease();

    printf("All tests passed\n");
#else
    printf("DHCP6_PD_CLIENT_ENABLE feature is not enabled\n");
#endif

    return 0;
}
