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

#include <openthread/config.h>

#include "test_platform.h"
#include "test_util.hpp"

#include "common/arg_macros.hpp"
#include "common/array.hpp"
#include "common/as_core_type.hpp"
#include "common/num_utils.hpp"
#include "common/owning_list.hpp"
#include "common/string.hpp"
#include "common/time.hpp"
#include "instance/instance.hpp"
#include "net/dns_dso.hpp"
#include "net/mdns.hpp"

#if OPENTHREAD_CONFIG_MULTICAST_DNS_ENABLE

namespace ot {
namespace Dns {
namespace Multicast {

#define ENABLE_TEST_LOG 1 // Enable to get logs from unit test.

// Logs a message and adds current time (sNow) as "<hours>:<min>:<secs>.<msec>"
#if ENABLE_TEST_LOG
#define Log(...)                                                                                         \
    printf("%02u:%02u:%02u.%03u " OT_FIRST_ARG(__VA_ARGS__) "\n", (sNow / 3600000), (sNow / 60000) % 60, \
           (sNow / 1000) % 60, sNow % 1000 OT_REST_ARGS(__VA_ARGS__))
#else
#define Log(...)
#endif

//---------------------------------------------------------------------------------------------------------------------
// Constants

static constexpr uint16_t kClassQueryUnicastFlag  = (1U << 15);
static constexpr uint16_t kClassCacheFlushFlag    = (1U << 15);
static constexpr uint16_t kClassMask              = 0x7fff;
static constexpr uint16_t kStringSize             = 300;
static constexpr uint16_t kMaxDataSize            = 400;
static constexpr uint16_t kNumAnnounces           = 3;
static constexpr uint16_t kNumInitalQueries       = 3;
static constexpr uint16_t kNumRefreshQueries      = 4;
static constexpr bool     kCacheFlush             = true;
static constexpr uint16_t kMdnsPort               = 5353;
static constexpr uint16_t kEphemeralPort          = 49152;
static constexpr uint16_t kLegacyUnicastMessageId = 1;
static constexpr uint16_t kMaxLegacyUnicastTtl    = 10;
static constexpr uint32_t kInfraIfIndex           = 1;

static const char kDeviceIp6Address[] = "fd01::1";

class DnsMessage;

//---------------------------------------------------------------------------------------------------------------------
// Variables

static Instance *sInstance;

static uint32_t sNow = 0;
static uint32_t sAlarmTime;
static bool     sAlarmOn = false;

OwningList<DnsMessage> sDnsMessages;
uint32_t               sInfraIfIndex;

//---------------------------------------------------------------------------------------------------------------------
// Prototypes

static const char *RecordTypeToString(uint16_t aType);

//---------------------------------------------------------------------------------------------------------------------
// Types

template <typename Type> class Allocatable
{
public:
    static Type *Allocate(void)
    {
        void *buf = calloc(1, sizeof(Type));

        VerifyOrQuit(buf != nullptr);
        return new (buf) Type();
    }

    void Free(void)
    {
        static_cast<Type *>(this)->~Type();
        free(this);
    }
};

struct DnsName
{
    Name::Buffer mName;

    void ParseFrom(const Message &aMessage, uint16_t &aOffset)
    {
        SuccessOrQuit(Name::ReadName(aMessage, aOffset, mName));
    }

    void CopyFrom(const char *aName)
    {
        if (aName == nullptr)
        {
            mName[0] = '\0';
        }
        else
        {
            uint16_t len = StringLength(aName, sizeof(mName));

            VerifyOrQuit(len < sizeof(mName));
            memcpy(mName, aName, len + 1);
        }
    }

    const char *AsCString(void) const { return mName; }
    bool        Matches(const char *aName) const { return StringMatch(mName, aName, kStringCaseInsensitiveMatch); }
};

typedef String<Name::kMaxNameSize> DnsNameString;

struct AddrAndTtl
{
    bool operator==(const AddrAndTtl &aOther) const { return (mTtl == aOther.mTtl) && (mAddress == aOther.mAddress); }

    Ip6::Address mAddress;
    uint32_t     mTtl;
};

struct DnsQuestion : public Allocatable<DnsQuestion>, public LinkedListEntry<DnsQuestion>
{
    DnsQuestion *mNext;
    DnsName      mName;
    uint16_t     mType;
    uint16_t     mClass;
    bool         mUnicastResponse;

    void ParseFrom(const Message &aMessage, uint16_t &aOffset)
    {
        Question question;

        mName.ParseFrom(aMessage, aOffset);
        SuccessOrQuit(aMessage.Read(aOffset, question));
        aOffset += sizeof(Question);

        mNext            = nullptr;
        mType            = question.GetType();
        mClass           = question.GetClass() & kClassMask;
        mUnicastResponse = question.GetClass() & kClassQueryUnicastFlag;

        Log("      %s %s %s class:%u", mName.AsCString(), RecordTypeToString(mType), mUnicastResponse ? "QU" : "QM",
            mClass);
    }

    bool Matches(const char *aName) const { return mName.Matches(aName); }
};

struct DnsQuestions : public OwningList<DnsQuestion>
{
    bool Contains(uint16_t aRrType, const DnsNameString &aFullName, bool aUnicastResponse = false) const
    {
        bool               contains = false;
        const DnsQuestion *question = FindMatching(aFullName.AsCString());

        VerifyOrExit(question != nullptr);
        VerifyOrExit(question->mType == aRrType);
        VerifyOrExit(question->mClass == ResourceRecord::kClassInternet);
        VerifyOrExit(question->mUnicastResponse == aUnicastResponse);
        contains = true;

    exit:
        return contains;
    }

    bool Contains(const DnsNameString &aFullName, bool aUnicastResponse) const
    {
        return Contains(ResourceRecord::kTypeAny, aFullName, aUnicastResponse);
    }
};

enum TtlCheckMode : uint8_t
{
    kZeroTtl,
    kNonZeroTtl,
    kLegacyUnicastTtl,
};

enum Section : uint8_t
{
    kInAnswerSection,
    kInAdditionalSection,
};

struct Data : public ot::Data<kWithUint16Length>
{
    Data(const void *aBuffer, uint16_t aLength) { Init(aBuffer, aLength); }

    bool Matches(const Array<uint8_t, kMaxDataSize> &aDataArray) const
    {
        return (aDataArray.GetLength() == GetLength()) && MatchesBytesIn(aDataArray.GetArrayBuffer());
    }
};

struct DnsRecord : public Allocatable<DnsRecord>, public LinkedListEntry<DnsRecord>
{
    struct SrvData
    {
        uint16_t mPriority;
        uint16_t mWeight;
        uint16_t mPort;
        DnsName  mHostName;
    };

    union RecordData
    {
        RecordData(void) { memset(this, 0, sizeof(*this)); }

        Ip6::Address                 mIp6Address; // For AAAAA (or A)
        SrvData                      mSrv;        // For SRV
        Array<uint8_t, kMaxDataSize> mData;       // For TXT or KEY
        DnsName                      mPtrName;    // For PTR
        NsecRecord::TypeBitMap       mNsecBitmap; // For NSEC
    };

    DnsRecord *mNext;
    DnsName    mName;
    uint16_t   mType;
    uint16_t   mClass;
    uint32_t   mTtl;
    bool       mCacheFlush;
    RecordData mData;

    bool Matches(const char *aName) const { return mName.Matches(aName); }

    void ParseFrom(const Message &aMessage, uint16_t &aOffset)
    {
        String<kStringSize> logStr;
        ResourceRecord      record;
        uint16_t            offset;

        mName.ParseFrom(aMessage, aOffset);
        SuccessOrQuit(aMessage.Read(aOffset, record));
        aOffset += sizeof(ResourceRecord);

        mNext       = nullptr;
        mType       = record.GetType();
        mClass      = record.GetClass() & kClassMask;
        mCacheFlush = record.GetClass() & kClassCacheFlushFlag;
        mTtl        = record.GetTtl();

        logStr.Append("%s %s%s cls:%u ttl:%u", mName.AsCString(), RecordTypeToString(mType),
                      mCacheFlush ? " cache-flush" : "", mClass, mTtl);

        offset = aOffset;

        switch (mType)
        {
        case ResourceRecord::kTypeAaaa:
            VerifyOrQuit(record.GetLength() == sizeof(Ip6::Address));
            SuccessOrQuit(aMessage.Read(offset, mData.mIp6Address));
            logStr.Append(" %s", mData.mIp6Address.ToString().AsCString());
            break;

        case ResourceRecord::kTypeKey:
        case ResourceRecord::kTypeTxt:
            VerifyOrQuit(record.GetLength() > 0);
            VerifyOrQuit(record.GetLength() < kMaxDataSize);
            mData.mData.SetLength(record.GetLength());
            SuccessOrQuit(aMessage.Read(offset, mData.mData.GetArrayBuffer(), record.GetLength()));
            logStr.Append(" data-len:%u", record.GetLength());
            break;

        case ResourceRecord::kTypePtr:
            mData.mPtrName.ParseFrom(aMessage, offset);
            VerifyOrQuit(offset - aOffset == record.GetLength());
            logStr.Append(" %s", mData.mPtrName.AsCString());
            break;

        case ResourceRecord::kTypeSrv:
        {
            SrvRecord srv;

            offset -= sizeof(ResourceRecord);
            SuccessOrQuit(aMessage.Read(offset, srv));
            offset += sizeof(srv);
            mData.mSrv.mHostName.ParseFrom(aMessage, offset);
            VerifyOrQuit(offset - aOffset == record.GetLength());
            mData.mSrv.mPriority = srv.GetPriority();
            mData.mSrv.mWeight   = srv.GetWeight();
            mData.mSrv.mPort     = srv.GetPort();
            logStr.Append(" port:%u w:%u prio:%u host:%s", mData.mSrv.mPort, mData.mSrv.mWeight, mData.mSrv.mPriority,
                          mData.mSrv.mHostName.AsCString());
            break;
        }

        case ResourceRecord::kTypeNsec:
        {
            NsecRecord::TypeBitMap &bitmap = mData.mNsecBitmap;

            SuccessOrQuit(Name::CompareName(aMessage, offset, mName.AsCString()));
            SuccessOrQuit(aMessage.Read(offset, &bitmap, NsecRecord::TypeBitMap::kMinSize));
            VerifyOrQuit(bitmap.GetBlockNumber() == 0);
            VerifyOrQuit(bitmap.GetBitmapLength() <= NsecRecord::TypeBitMap::kMaxLength);
            SuccessOrQuit(aMessage.Read(offset, &bitmap, bitmap.GetSize()));

            offset += bitmap.GetSize();
            VerifyOrQuit(offset - aOffset == record.GetLength());

            logStr.Append(" [ ");

            for (uint16_t type = 0; type < bitmap.GetBitmapLength() * kBitsPerByte; type++)
            {
                if (bitmap.ContainsType(type))
                {
                    logStr.Append("%s ", RecordTypeToString(type));
                }
            }

            logStr.Append("]");
            break;
        }

        default:
            break;
        }

        Log("      %s", logStr.AsCString());

        aOffset += record.GetLength();
    }

    bool MatchesTtl(TtlCheckMode aTtlCheckMode, uint32_t aTtl) const
    {
        bool matches = false;

        switch (aTtlCheckMode)
        {
        case kZeroTtl:
            VerifyOrExit(mTtl == 0);
            break;
        case kNonZeroTtl:
            if (aTtl > 0)
            {
                VerifyOrQuit(mTtl == aTtl);
            }

            VerifyOrExit(mTtl > 0);
            break;
        case kLegacyUnicastTtl:
            VerifyOrQuit(mTtl <= kMaxLegacyUnicastTtl);
            break;
        }

        matches = true;

    exit:
        return matches;
    }
};

struct DnsRecords : public OwningList<DnsRecord>
{
    bool ContainsAaaa(const DnsNameString &aFullName,
                      const Ip6::Address  &aAddress,
                      bool                 aCacheFlush,
                      TtlCheckMode         aTtlCheckMode,
                      uint32_t             aTtl = 0) const
    {
        bool contains = false;

        for (const DnsRecord &record : *this)
        {
            if (record.Matches(aFullName.AsCString()) && (record.mType == ResourceRecord::kTypeAaaa) &&
                (record.mData.mIp6Address == aAddress))
            {
                VerifyOrExit(record.mClass == ResourceRecord::kClassInternet);
                VerifyOrExit(record.mCacheFlush == aCacheFlush);
                VerifyOrExit(record.MatchesTtl(aTtlCheckMode, aTtl));
                contains = true;
                ExitNow();
            }
        }

    exit:
        return contains;
    }

    bool ContainsKey(const DnsNameString &aFullName,
                     const Data          &aKeyData,
                     bool                 aCacheFlush,
                     TtlCheckMode         aTtlCheckMode,
                     uint32_t             aTtl = 0) const
    {
        bool contains = false;

        for (const DnsRecord &record : *this)
        {
            if (record.Matches(aFullName.AsCString()) && (record.mType == ResourceRecord::kTypeKey) &&
                aKeyData.Matches(record.mData.mData))
            {
                VerifyOrExit(record.mClass == ResourceRecord::kClassInternet);
                VerifyOrExit(record.mCacheFlush == aCacheFlush);
                VerifyOrExit(record.MatchesTtl(aTtlCheckMode, aTtl));
                contains = true;
                ExitNow();
            }
        }

    exit:
        return contains;
    }

    bool ContainsSrv(const DnsNameString &aFullName,
                     const Core::Service &aService,
                     bool                 aCacheFlush,
                     TtlCheckMode         aTtlCheckMode,
                     uint32_t             aTtl = 0) const
    {
        bool          contains = false;
        DnsNameString hostName;

        hostName.Append("%s.local.", aService.mHostName);

        for (const DnsRecord &record : *this)
        {
            if (record.Matches(aFullName.AsCString()) && (record.mType == ResourceRecord::kTypeSrv))
            {
                VerifyOrExit(record.mClass == ResourceRecord::kClassInternet);
                VerifyOrExit(record.mCacheFlush == aCacheFlush);
                VerifyOrExit(record.MatchesTtl(aTtlCheckMode, aTtl));
                VerifyOrExit(record.mData.mSrv.mPort == aService.mPort);
                VerifyOrExit(record.mData.mSrv.mPriority == aService.mPriority);
                VerifyOrExit(record.mData.mSrv.mWeight == aService.mWeight);
                VerifyOrExit(record.mData.mSrv.mHostName.Matches(hostName.AsCString()));
                contains = true;
                ExitNow();
            }
        }

    exit:
        return contains;
    }

    bool ContainsTxt(const DnsNameString &aFullName,
                     const Core::Service &aService,
                     bool                 aCacheFlush,
                     TtlCheckMode         aTtlCheckMode,
                     uint32_t             aTtl = 0) const
    {
        static const uint8_t kEmptyTxtData[1] = {0};

        bool contains = false;
        Data txtData(aService.mTxtData, aService.mTxtDataLength);

        if ((aService.mTxtData == nullptr) || (aService.mTxtDataLength == 0))
        {
            txtData.Init(kEmptyTxtData, sizeof(kEmptyTxtData));
        }

        for (const DnsRecord &record : *this)
        {
            if (record.Matches(aFullName.AsCString()) && (record.mType == ResourceRecord::kTypeTxt) &&
                txtData.Matches(record.mData.mData))
            {
                VerifyOrExit(record.mClass == ResourceRecord::kClassInternet);
                VerifyOrExit(record.mCacheFlush == aCacheFlush);
                VerifyOrExit(record.MatchesTtl(aTtlCheckMode, aTtl));
                contains = true;
                ExitNow();
            }
        }

    exit:
        return contains;
    }

    bool ContainsPtr(const DnsNameString &aFullName,
                     const DnsNameString &aPtrName,
                     TtlCheckMode         aTtlCheckMode,
                     uint32_t             aTtl = 0) const
    {
        bool contains = false;

        for (const DnsRecord &record : *this)
        {
            if (record.Matches(aFullName.AsCString()) && (record.mType == ResourceRecord::kTypePtr) &&
                (record.mData.mPtrName.Matches(aPtrName.AsCString())))
            {
                VerifyOrExit(record.mClass == ResourceRecord::kClassInternet);
                VerifyOrExit(!record.mCacheFlush); // PTR should never use cache-flush
                VerifyOrExit(record.MatchesTtl(aTtlCheckMode, aTtl));
                contains = true;
                ExitNow();
            }
        }

    exit:
        return contains;
    }

    bool ContainsServicesPtr(const DnsNameString &aServiceType) const
    {
        DnsNameString allServices;

        allServices.Append("_services._dns-sd._udp.local.");

        return ContainsPtr(allServices, aServiceType, kNonZeroTtl, 0);
    }

    bool ContainsNsec(const DnsNameString &aFullName, uint16_t aRecordType) const
    {
        bool contains = false;

        for (const DnsRecord &record : *this)
        {
            if (record.Matches(aFullName.AsCString()) && (record.mType == ResourceRecord::kTypeNsec))
            {
                VerifyOrQuit(!contains); // Ensure only one NSEC record
                VerifyOrExit(record.mData.mNsecBitmap.ContainsType(aRecordType));
                contains = true;
            }
        }

    exit:
        return contains;
    }
};

// Bit-flags used in `Validate()` with a `Service`
// to specify which records should be checked in the announce
// message.

typedef uint8_t AnnounceCheckFlags;

static constexpr uint8_t kCheckSrv         = (1 << 0);
static constexpr uint8_t kCheckTxt         = (1 << 1);
static constexpr uint8_t kCheckPtr         = (1 << 2);
static constexpr uint8_t kCheckServicesPtr = (1 << 3);

enum GoodBye : bool // Used to indicate "goodbye" records (with zero TTL)
{
    kNotGoodBye = false,
    kGoodBye    = true,
};

enum DnsMessageType : uint8_t
{
    kMulticastQuery,
    kMulticastResponse,
    kUnicastResponse,
    kLegacyUnicastResponse,
};

struct DnsMessage : public Allocatable<DnsMessage>, public LinkedListEntry<DnsMessage>
{
    DnsMessage       *mNext;
    uint32_t          mTimestamp;
    DnsMessageType    mType;
    Core::AddressInfo mUnicastDest;
    Header            mHeader;
    DnsQuestions      mQuestions;
    DnsRecords        mAnswerRecords;
    DnsRecords        mAuthRecords;
    DnsRecords        mAdditionalRecords;

    DnsMessage(void)
        : mNext(nullptr)
        , mTimestamp(sNow)
    {
    }

    const DnsRecords &RecordsFor(Section aSection) const
    {
        const DnsRecords *records = nullptr;

        switch (aSection)
        {
        case kInAnswerSection:
            records = &mAnswerRecords;
            break;
        case kInAdditionalSection:
            records = &mAdditionalRecords;
            break;
        }

        VerifyOrQuit(records != nullptr);

        return *records;
    }

    void ParseRecords(const Message         &aMessage,
                      uint16_t              &aOffset,
                      uint16_t               aNumRecords,
                      OwningList<DnsRecord> &aRecords,
                      const char            *aSectionName)
    {
        if (aNumRecords > 0)
        {
            Log("   %s", aSectionName);
        }

        for (; aNumRecords > 0; aNumRecords--)
        {
            DnsRecord *record = DnsRecord::Allocate();

            record->ParseFrom(aMessage, aOffset);
            aRecords.PushAfterTail(*record);
        }
    }

    void ParseFrom(const Message &aMessage)
    {
        uint16_t offset = 0;

        SuccessOrQuit(aMessage.Read(offset, mHeader));
        offset += sizeof(Header);

        Log("   %s id:%u qt:%u t:%u rcode:%u [q:%u ans:%u auth:%u addn:%u]",
            mHeader.GetType() == Header::kTypeQuery ? "Query" : "Response", mHeader.GetMessageId(),
            mHeader.GetQueryType(), mHeader.IsTruncationFlagSet(), mHeader.GetResponseCode(),
            mHeader.GetQuestionCount(), mHeader.GetAnswerCount(), mHeader.GetAuthorityRecordCount(),
            mHeader.GetAdditionalRecordCount());

        if (mHeader.GetQuestionCount() > 0)
        {
            Log("   Question");
        }

        for (uint16_t num = mHeader.GetQuestionCount(); num > 0; num--)
        {
            DnsQuestion *question = DnsQuestion::Allocate();

            question->ParseFrom(aMessage, offset);
            mQuestions.PushAfterTail(*question);
        }

        ParseRecords(aMessage, offset, mHeader.GetAnswerCount(), mAnswerRecords, "Answer");
        ParseRecords(aMessage, offset, mHeader.GetAuthorityRecordCount(), mAuthRecords, "Authority");
        ParseRecords(aMessage, offset, mHeader.GetAdditionalRecordCount(), mAdditionalRecords, "Additional");
    }

    void ValidateHeader(DnsMessageType aType,
                        uint16_t       aQuestionCount,
                        uint16_t       aAnswerCount,
                        uint16_t       aAuthCount,
                        uint16_t       aAdditionalCount) const
    {
        VerifyOrQuit(mType == aType);
        VerifyOrQuit(mHeader.GetQuestionCount() == aQuestionCount);
        VerifyOrQuit(mHeader.GetAnswerCount() == aAnswerCount);
        VerifyOrQuit(mHeader.GetAuthorityRecordCount() == aAuthCount);
        VerifyOrQuit(mHeader.GetAdditionalRecordCount() == aAdditionalCount);

        if (aType == kUnicastResponse)
        {
            Ip6::Address ip6Address;

            SuccessOrQuit(ip6Address.FromString(kDeviceIp6Address));

            VerifyOrQuit(mUnicastDest.mPort == kMdnsPort);
            VerifyOrQuit(mUnicastDest.GetAddress() == ip6Address);
        }

        if (aType == kLegacyUnicastResponse)
        {
            VerifyOrQuit(mHeader.GetMessageId() == kLegacyUnicastMessageId);
            VerifyOrQuit(mUnicastDest.mPort == kEphemeralPort);
        }
    }

    static void DetermineFullNameForKey(const Core::Key &aKey, DnsNameString &aFullName)
    {
        if (aKey.mServiceType != nullptr)
        {
            aFullName.Append("%s.%s.local.", aKey.mName, aKey.mServiceType);
        }
        else
        {
            aFullName.Append("%s.local.", aKey.mName);
        }
    }

    static TtlCheckMode DetermineTtlCheckMode(DnsMessageType aMessageType, bool aIsGoodBye)
    {
        TtlCheckMode ttlCheck;

        if (aMessageType == kLegacyUnicastResponse)
        {
            ttlCheck = kLegacyUnicastTtl;
        }
        else
        {
            ttlCheck = aIsGoodBye ? kZeroTtl : kNonZeroTtl;
        }

        return ttlCheck;
    }

    void ValidateAsProbeFor(const Core::Host &aHost, bool aUnicastResponse) const
    {
        DnsNameString fullName;

        VerifyOrQuit(mHeader.GetType() == Header::kTypeQuery);
        VerifyOrQuit(!mHeader.IsTruncationFlagSet());

        fullName.Append("%s.local.", aHost.mHostName);
        VerifyOrQuit(mQuestions.Contains(fullName, aUnicastResponse));

        for (uint16_t index = 0; index < aHost.mAddressesLength; index++)
        {
            VerifyOrQuit(mAuthRecords.ContainsAaaa(fullName, AsCoreType(&aHost.mAddresses[index]), !kCacheFlush,
                                                   kNonZeroTtl, aHost.mTtl));
        }
    }

    void ValidateAsProbeFor(const Core::Service &aService, bool aUnicastResponse) const
    {
        DnsNameString serviceName;

        VerifyOrQuit(mHeader.GetType() == Header::kTypeQuery);
        VerifyOrQuit(!mHeader.IsTruncationFlagSet());

        serviceName.Append("%s.%s.local.", aService.mServiceInstance, aService.mServiceType);

        VerifyOrQuit(mQuestions.Contains(serviceName, aUnicastResponse));

        VerifyOrQuit(mAuthRecords.ContainsSrv(serviceName, aService, !kCacheFlush, kNonZeroTtl, aService.mTtl));
        VerifyOrQuit(mAuthRecords.ContainsTxt(serviceName, aService, !kCacheFlush, kNonZeroTtl, aService.mTtl));
    }

    void ValidateAsProbeFor(const Core::Key &aKey, bool aUnicastResponse) const
    {
        DnsNameString fullName;

        VerifyOrQuit(mHeader.GetType() == Header::kTypeQuery);
        VerifyOrQuit(!mHeader.IsTruncationFlagSet());

        DetermineFullNameForKey(aKey, fullName);

        VerifyOrQuit(mQuestions.Contains(fullName, aUnicastResponse));
        VerifyOrQuit(mAuthRecords.ContainsKey(fullName, Data(aKey.mKeyData, aKey.mKeyDataLength), !kCacheFlush,
                                              kNonZeroTtl, aKey.mTtl));
    }

    void Validate(const Core::Host &aHost, Section aSection, GoodBye aIsGoodBye = kNotGoodBye) const
    {
        DnsNameString fullName;
        TtlCheckMode  ttlCheck;

        bool cacheFlushSet = (mType == kLegacyUnicastResponse) ? !kCacheFlush : kCacheFlush;

        ttlCheck = DetermineTtlCheckMode(mType, aIsGoodBye);

        VerifyOrQuit(mHeader.GetType() == Header::kTypeResponse);

        fullName.Append("%s.local.", aHost.mHostName);

        for (uint16_t index = 0; index < aHost.mAddressesLength; index++)
        {
            VerifyOrQuit(RecordsFor(aSection).ContainsAaaa(fullName, AsCoreType(&aHost.mAddresses[index]),
                                                           cacheFlushSet, ttlCheck, aHost.mTtl));
        }

        if (!aIsGoodBye && (aSection == kInAnswerSection))
        {
            VerifyOrQuit(mAdditionalRecords.ContainsNsec(fullName, ResourceRecord::kTypeAaaa));
        }
    }

    void Validate(const Core::Service &aService,
                  Section              aSection,
                  AnnounceCheckFlags   aCheckFlags,
                  GoodBye              aIsGoodBye = kNotGoodBye) const
    {
        DnsNameString serviceName;
        DnsNameString serviceType;
        TtlCheckMode  ttlCheck;
        bool          checkNsec     = false;
        bool          cacheFlushSet = (mType == kLegacyUnicastResponse) ? !kCacheFlush : kCacheFlush;

        ttlCheck = DetermineTtlCheckMode(mType, aIsGoodBye);

        VerifyOrQuit(mHeader.GetType() == Header::kTypeResponse);

        serviceName.Append("%s.%s.local.", aService.mServiceInstance, aService.mServiceType);
        serviceType.Append("%s.local.", aService.mServiceType);

        if (aCheckFlags & kCheckSrv)
        {
            VerifyOrQuit(
                RecordsFor(aSection).ContainsSrv(serviceName, aService, cacheFlushSet, ttlCheck, aService.mTtl));

            checkNsec = true;
        }

        if (aCheckFlags & kCheckTxt)
        {
            VerifyOrQuit(
                RecordsFor(aSection).ContainsTxt(serviceName, aService, cacheFlushSet, ttlCheck, aService.mTtl));

            checkNsec = true;
        }

        if (aCheckFlags & kCheckPtr)
        {
            VerifyOrQuit(RecordsFor(aSection).ContainsPtr(serviceType, serviceName, ttlCheck, aService.mTtl));
        }

        if (aCheckFlags & kCheckServicesPtr)
        {
            VerifyOrQuit(RecordsFor(aSection).ContainsServicesPtr(serviceType));
        }

        if (!aIsGoodBye && checkNsec && (aSection == kInAnswerSection))
        {
            VerifyOrQuit(mAdditionalRecords.ContainsNsec(serviceName, ResourceRecord::kTypeSrv));
            VerifyOrQuit(mAdditionalRecords.ContainsNsec(serviceName, ResourceRecord::kTypeTxt));
        }
    }

    void Validate(const Core::Key &aKey, Section aSection, GoodBye aIsGoodBye = kNotGoodBye) const
    {
        DnsNameString fullName;
        TtlCheckMode  ttlCheck;
        bool          cacheFlushSet = (mType == kLegacyUnicastResponse) ? !kCacheFlush : kCacheFlush;

        VerifyOrQuit(mHeader.GetType() == Header::kTypeResponse);

        DetermineFullNameForKey(aKey, fullName);

        ttlCheck = DetermineTtlCheckMode(mType, aIsGoodBye);

        VerifyOrQuit(RecordsFor(aSection).ContainsKey(fullName, Data(aKey.mKeyData, aKey.mKeyDataLength), cacheFlushSet,
                                                      ttlCheck, aKey.mTtl));

        if (!aIsGoodBye && (aSection == kInAnswerSection))
        {
            VerifyOrQuit(mAdditionalRecords.ContainsNsec(fullName, ResourceRecord::kTypeKey));
        }
    }

    void ValidateSubType(const char *aSubLabel, const Core::Service &aService, GoodBye aIsGoodBye = kNotGoodBye) const
    {
        DnsNameString serviceName;
        DnsNameString subServiceType;

        VerifyOrQuit(mHeader.GetType() == Header::kTypeResponse);

        serviceName.Append("%s.%s.local.", aService.mServiceInstance, aService.mServiceType);
        subServiceType.Append("%s._sub.%s.local.", aSubLabel, aService.mServiceType);

        VerifyOrQuit(mAnswerRecords.ContainsPtr(subServiceType, serviceName, aIsGoodBye ? kZeroTtl : kNonZeroTtl,
                                                aService.mTtl));
    }

    void ValidateAsQueryFor(const Core::Browser &aBrowser) const
    {
        DnsNameString fullServiceType;

        VerifyOrQuit(mHeader.GetType() == Header::kTypeQuery);
        VerifyOrQuit(!mHeader.IsTruncationFlagSet());

        if (aBrowser.mSubTypeLabel == nullptr)
        {
            fullServiceType.Append("%s.local.", aBrowser.mServiceType);
        }
        else
        {
            fullServiceType.Append("%s._sub.%s.local", aBrowser.mSubTypeLabel, aBrowser.mServiceType);
        }

        VerifyOrQuit(mQuestions.Contains(ResourceRecord::kTypePtr, fullServiceType));
    }

    void ValidateAsQueryFor(const Core::SrvResolver &aResolver) const
    {
        DnsNameString fullName;

        VerifyOrQuit(mHeader.GetType() == Header::kTypeQuery);
        VerifyOrQuit(!mHeader.IsTruncationFlagSet());

        fullName.Append("%s.%s.local.", aResolver.mServiceInstance, aResolver.mServiceType);

        VerifyOrQuit(mQuestions.Contains(ResourceRecord::kTypeSrv, fullName));
    }

    void ValidateAsQueryFor(const Core::TxtResolver &aResolver) const
    {
        DnsNameString fullName;

        VerifyOrQuit(mHeader.GetType() == Header::kTypeQuery);
        VerifyOrQuit(!mHeader.IsTruncationFlagSet());

        fullName.Append("%s.%s.local.", aResolver.mServiceInstance, aResolver.mServiceType);

        VerifyOrQuit(mQuestions.Contains(ResourceRecord::kTypeTxt, fullName));
    }

    void ValidateAsQueryFor(const Core::AddressResolver &aResolver) const
    {
        DnsNameString fullName;

        VerifyOrQuit(mHeader.GetType() == Header::kTypeQuery);
        VerifyOrQuit(!mHeader.IsTruncationFlagSet());

        fullName.Append("%s.local.", aResolver.mHostName);

        VerifyOrQuit(mQuestions.Contains(ResourceRecord::kTypeAaaa, fullName));
    }
};

struct RegCallback
{
    void Reset(void) { mWasCalled = false; }

    bool  mWasCalled;
    Error mError;
};

static constexpr uint16_t kMaxCallbacks = 8;

static RegCallback sRegCallbacks[kMaxCallbacks];

static void HandleCallback(otInstance *aInstance, otMdnsRequestId aRequestId, otError aError)
{
    Log("Register callback - ResuestId:%u Error:%s", aRequestId, otThreadErrorToString(aError));

    VerifyOrQuit(aInstance == sInstance);
    VerifyOrQuit(aRequestId < kMaxCallbacks);

    VerifyOrQuit(!sRegCallbacks[aRequestId].mWasCalled);

    sRegCallbacks[aRequestId].mWasCalled = true;
    sRegCallbacks[aRequestId].mError     = aError;
}

static void HandleSuccessCallback(otInstance *aInstance, otMdnsRequestId aRequestId, otError aError)
{
    HandleCallback(aInstance, aRequestId, aError);
    SuccessOrQuit(aError);
}

struct ConflictCallback
{
    void Reset(void) { mWasCalled = false; }

    void Handle(const char *aName, const char *aServiceType)
    {
        VerifyOrQuit(!mWasCalled);

        mWasCalled = true;
        mName.Clear();
        mName.Append("%s", aName);

        mHasServiceType = (aServiceType != nullptr);
        VerifyOrExit(mHasServiceType);
        mServiceType.Clear();
        mServiceType.Append("%s", aServiceType);

    exit:
        return;
    }

    bool          mWasCalled;
    bool          mHasServiceType;
    DnsNameString mName;
    DnsNameString mServiceType;
};

static ConflictCallback sConflictCallback;

static void HandleConflict(otInstance *aInstance, const char *aName, const char *aServiceType)
{
    Log("Conflict callback - %s %s", aName, (aServiceType == nullptr) ? "" : aServiceType);

    VerifyOrQuit(aInstance == sInstance);
    sConflictCallback.Handle(aName, aServiceType);
}

//---------------------------------------------------------------------------------------------------------------------
// Helper functions and methods

static const char *RecordTypeToString(uint16_t aType)
{
    const char *str = "Other";

    switch (aType)
    {
    case ResourceRecord::kTypeZero:
        str = "ZERO";
        break;
    case ResourceRecord::kTypeA:
        str = "A";
        break;
    case ResourceRecord::kTypeSoa:
        str = "SOA";
        break;
    case ResourceRecord::kTypeCname:
        str = "CNAME";
        break;
    case ResourceRecord::kTypePtr:
        str = "PTR";
        break;
    case ResourceRecord::kTypeTxt:
        str = "TXT";
        break;
    case ResourceRecord::kTypeSig:
        str = "SIG";
        break;
    case ResourceRecord::kTypeKey:
        str = "KEY";
        break;
    case ResourceRecord::kTypeAaaa:
        str = "AAAA";
        break;
    case ResourceRecord::kTypeSrv:
        str = "SRV";
        break;
    case ResourceRecord::kTypeOpt:
        str = "OPT";
        break;
    case ResourceRecord::kTypeNsec:
        str = "NSEC";
        break;
    case ResourceRecord::kTypeAny:
        str = "ANY";
        break;
    }

    return str;
}

static void ParseMessage(const Message &aMessage, const Core::AddressInfo *aUnicastDest)
{
    DnsMessage *msg = DnsMessage::Allocate();

    msg->ParseFrom(aMessage);

    switch (msg->mHeader.GetType())
    {
    case Header::kTypeQuery:
        msg->mType = kMulticastQuery;
        VerifyOrQuit(aUnicastDest == nullptr);
        break;

    case Header::kTypeResponse:
        if (aUnicastDest == nullptr)
        {
            msg->mType = kMulticastResponse;
        }
        else
        {
            msg->mType        = (aUnicastDest->mPort == kEphemeralPort) ? kLegacyUnicastResponse : kUnicastResponse;
            msg->mUnicastDest = *aUnicastDest;
        }
    }

    sDnsMessages.PushAfterTail(*msg);
}

static void SendQuery(const char *aName,
                      uint16_t    aRecordType,
                      uint16_t    aRecordClass        = ResourceRecord::kClassInternet,
                      bool        aTruncated          = false,
                      bool        aLegacyUnicastQuery = false)
{
    Message          *message;
    Header            header;
    Core::AddressInfo senderAddrInfo;

    message = sInstance->Get<MessagePool>().Allocate(Message::kTypeOther);
    VerifyOrQuit(message != nullptr);

    header.Clear();
    header.SetType(Header::kTypeQuery);
    header.SetQuestionCount(1);

    if (aLegacyUnicastQuery)
    {
        header.SetMessageId(kLegacyUnicastMessageId);
    }

    if (aTruncated)
    {
        header.SetTruncationFlag();
    }

    SuccessOrQuit(message->Append(header));
    SuccessOrQuit(Name::AppendName(aName, *message));
    SuccessOrQuit(message->Append(Question(aRecordType, aRecordClass)));

    SuccessOrQuit(AsCoreType(&senderAddrInfo.mAddress).FromString(kDeviceIp6Address));
    senderAddrInfo.mPort         = aLegacyUnicastQuery ? kEphemeralPort : kMdnsPort;
    senderAddrInfo.mInfraIfIndex = 0;

    Log("Sending query for %s %s", aName, RecordTypeToString(aRecordType));

    otPlatMdnsHandleReceive(sInstance, message, /* aIsUnicast */ false, &senderAddrInfo);
}

static void SendQueryForTwo(const char *aName1,
                            uint16_t    aRecordType1,
                            const char *aName2,
                            uint16_t    aRecordType2,
                            bool        aIsLegacyUnicast = false)
{
    // Send query with two questions.

    Message          *message;
    Header            header;
    Core::AddressInfo senderAddrInfo;

    message = sInstance->Get<MessagePool>().Allocate(Message::kTypeOther);
    VerifyOrQuit(message != nullptr);

    header.Clear();
    header.SetType(Header::kTypeQuery);
    header.SetQuestionCount(2);

    SuccessOrQuit(message->Append(header));
    SuccessOrQuit(Name::AppendName(aName1, *message));
    SuccessOrQuit(message->Append(Question(aRecordType1, ResourceRecord::kClassInternet)));
    SuccessOrQuit(Name::AppendName(aName2, *message));
    SuccessOrQuit(message->Append(Question(aRecordType2, ResourceRecord::kClassInternet)));

    SuccessOrQuit(AsCoreType(&senderAddrInfo.mAddress).FromString(kDeviceIp6Address));
    senderAddrInfo.mPort         = aIsLegacyUnicast ? kEphemeralPort : kMdnsPort;
    senderAddrInfo.mInfraIfIndex = 0;

    Log("Sending query for %s %s and %s %s", aName1, RecordTypeToString(aRecordType1), aName2,
        RecordTypeToString(aRecordType2));

    otPlatMdnsHandleReceive(sInstance, message, /* aIsUnicast */ false, &senderAddrInfo);
}

static void SendPtrResponse(const char *aName, const char *aPtrName, uint32_t aTtl, Section aSection)
{
    Message          *message;
    Header            header;
    PtrRecord         ptr;
    Core::AddressInfo senderAddrInfo;

    message = sInstance->Get<MessagePool>().Allocate(Message::kTypeOther);
    VerifyOrQuit(message != nullptr);

    header.Clear();
    header.SetType(Header::kTypeResponse);

    switch (aSection)
    {
    case kInAnswerSection:
        header.SetAnswerCount(1);
        break;
    case kInAdditionalSection:
        header.SetAdditionalRecordCount(1);
        break;
    }

    SuccessOrQuit(message->Append(header));
    SuccessOrQuit(Name::AppendName(aName, *message));

    ptr.Init();
    ptr.SetTtl(aTtl);
    ptr.SetLength(StringLength(aPtrName, Name::kMaxNameSize) + 1);
    SuccessOrQuit(message->Append(ptr));
    SuccessOrQuit(Name::AppendName(aPtrName, *message));

    SuccessOrQuit(AsCoreType(&senderAddrInfo.mAddress).FromString(kDeviceIp6Address));
    senderAddrInfo.mPort         = kMdnsPort;
    senderAddrInfo.mInfraIfIndex = 0;

    Log("Sending PTR response for %s with %s, ttl:%lu", aName, aPtrName, ToUlong(aTtl));

    otPlatMdnsHandleReceive(sInstance, message, /* aIsUnicast */ false, &senderAddrInfo);
}

static void SendSrvResponse(const char *aServiceName,
                            const char *aHostName,
                            uint16_t    aPort,
                            uint16_t    aPriority,
                            uint16_t    aWeight,
                            uint32_t    aTtl,
                            Section     aSection)
{
    Message          *message;
    Header            header;
    SrvRecord         srv;
    Core::AddressInfo senderAddrInfo;

    message = sInstance->Get<MessagePool>().Allocate(Message::kTypeOther);
    VerifyOrQuit(message != nullptr);

    header.Clear();
    header.SetType(Header::kTypeResponse);

    switch (aSection)
    {
    case kInAnswerSection:
        header.SetAnswerCount(1);
        break;
    case kInAdditionalSection:
        header.SetAdditionalRecordCount(1);
        break;
    }

    SuccessOrQuit(message->Append(header));
    SuccessOrQuit(Name::AppendName(aServiceName, *message));

    srv.Init();
    srv.SetTtl(aTtl);
    srv.SetPort(aPort);
    srv.SetPriority(aPriority);
    srv.SetWeight(aWeight);
    srv.SetLength(sizeof(srv) - sizeof(ResourceRecord) + StringLength(aHostName, Name::kMaxNameSize) + 1);
    SuccessOrQuit(message->Append(srv));
    SuccessOrQuit(Name::AppendName(aHostName, *message));

    SuccessOrQuit(AsCoreType(&senderAddrInfo.mAddress).FromString(kDeviceIp6Address));
    senderAddrInfo.mPort         = kMdnsPort;
    senderAddrInfo.mInfraIfIndex = 0;

    Log("Sending SRV response for %s, host:%s, port:%u, ttl:%lu", aServiceName, aHostName, aPort, ToUlong(aTtl));

    otPlatMdnsHandleReceive(sInstance, message, /* aIsUnicast */ false, &senderAddrInfo);
}

static void SendTxtResponse(const char    *aServiceName,
                            const uint8_t *aTxtData,
                            uint16_t       aTxtDataLength,
                            uint32_t       aTtl,
                            Section        aSection)
{
    Message          *message;
    Header            header;
    TxtRecord         txt;
    Core::AddressInfo senderAddrInfo;

    message = sInstance->Get<MessagePool>().Allocate(Message::kTypeOther);
    VerifyOrQuit(message != nullptr);

    header.Clear();
    header.SetType(Header::kTypeResponse);

    switch (aSection)
    {
    case kInAnswerSection:
        header.SetAnswerCount(1);
        break;
    case kInAdditionalSection:
        header.SetAdditionalRecordCount(1);
        break;
    }

    SuccessOrQuit(message->Append(header));
    SuccessOrQuit(Name::AppendName(aServiceName, *message));

    txt.Init();
    txt.SetTtl(aTtl);
    txt.SetLength(aTxtDataLength);
    SuccessOrQuit(message->Append(txt));
    SuccessOrQuit(message->AppendBytes(aTxtData, aTxtDataLength));

    SuccessOrQuit(AsCoreType(&senderAddrInfo.mAddress).FromString(kDeviceIp6Address));
    senderAddrInfo.mPort         = kMdnsPort;
    senderAddrInfo.mInfraIfIndex = 0;

    Log("Sending TXT response for %s, len:%u, ttl:%lu", aServiceName, aTxtDataLength, ToUlong(aTtl));

    otPlatMdnsHandleReceive(sInstance, message, /* aIsUnicast */ false, &senderAddrInfo);
}

static void SendHostAddrResponse(const char *aHostName,
                                 AddrAndTtl *aAddrAndTtls,
                                 uint32_t    aNumAddrs,
                                 bool        aCacheFlush,
                                 Section     aSection)
{
    Message          *message;
    Header            header;
    AaaaRecord        record;
    Core::AddressInfo senderAddrInfo;

    message = sInstance->Get<MessagePool>().Allocate(Message::kTypeOther);
    VerifyOrQuit(message != nullptr);

    header.Clear();
    header.SetType(Header::kTypeResponse);

    switch (aSection)
    {
    case kInAnswerSection:
        header.SetAnswerCount(aNumAddrs);
        break;
    case kInAdditionalSection:
        header.SetAdditionalRecordCount(aNumAddrs);
        break;
    }

    SuccessOrQuit(message->Append(header));

    record.Init();

    if (aCacheFlush)
    {
        record.SetClass(record.GetClass() | kClassCacheFlushFlag);
    }

    Log("Sending AAAA response for %s numAddrs:%u, cach-flush:%u", aHostName, aNumAddrs, aCacheFlush);

    for (uint32_t index = 0; index < aNumAddrs; index++)
    {
        record.SetTtl(aAddrAndTtls[index].mTtl);
        record.SetAddress(aAddrAndTtls[index].mAddress);

        SuccessOrQuit(Name::AppendName(aHostName, *message));
        SuccessOrQuit(message->Append(record));

        Log(" - %s, ttl:%lu", aAddrAndTtls[index].mAddress.ToString().AsCString(), ToUlong(aAddrAndTtls[index].mTtl));
    }

    SuccessOrQuit(AsCoreType(&senderAddrInfo.mAddress).FromString(kDeviceIp6Address));
    senderAddrInfo.mPort         = kMdnsPort;
    senderAddrInfo.mInfraIfIndex = 0;

    otPlatMdnsHandleReceive(sInstance, message, /* aIsUnicast */ false, &senderAddrInfo);
}

static void SendResponseWithEmptyKey(const char *aName, Section aSection)
{
    Message          *message;
    Header            header;
    ResourceRecord    record;
    Core::AddressInfo senderAddrInfo;

    message = sInstance->Get<MessagePool>().Allocate(Message::kTypeOther);
    VerifyOrQuit(message != nullptr);

    header.Clear();
    header.SetType(Header::kTypeResponse);

    switch (aSection)
    {
    case kInAnswerSection:
        header.SetAnswerCount(1);
        break;
    case kInAdditionalSection:
        header.SetAdditionalRecordCount(1);
        break;
    }

    SuccessOrQuit(message->Append(header));
    SuccessOrQuit(Name::AppendName(aName, *message));

    record.Init(ResourceRecord::kTypeKey);
    record.SetTtl(4500);
    record.SetLength(0);
    SuccessOrQuit(message->Append(record));

    SuccessOrQuit(AsCoreType(&senderAddrInfo.mAddress).FromString(kDeviceIp6Address));
    senderAddrInfo.mPort         = kMdnsPort;
    senderAddrInfo.mInfraIfIndex = 0;

    Log("Sending response with empty key for %s", aName);

    otPlatMdnsHandleReceive(sInstance, message, /* aIsUnicast */ false, &senderAddrInfo);
}

struct KnownAnswer
{
    const char *mPtrAnswer;
    uint32_t    mTtl;
};

static void SendPtrQueryWithKnownAnswers(const char *aName, const KnownAnswer *aKnownAnswers, uint16_t aNumAnswers)
{
    Message          *message;
    Header            header;
    Core::AddressInfo senderAddrInfo;
    uint16_t          nameOffset;

    message = sInstance->Get<MessagePool>().Allocate(Message::kTypeOther);
    VerifyOrQuit(message != nullptr);

    header.Clear();
    header.SetType(Header::kTypeQuery);
    header.SetQuestionCount(1);
    header.SetAnswerCount(aNumAnswers);

    SuccessOrQuit(message->Append(header));
    nameOffset = message->GetLength();
    SuccessOrQuit(Name::AppendName(aName, *message));
    SuccessOrQuit(message->Append(Question(ResourceRecord::kTypePtr, ResourceRecord::kClassInternet)));

    for (uint16_t index = 0; index < aNumAnswers; index++)
    {
        PtrRecord ptr;

        ptr.Init();
        ptr.SetTtl(aKnownAnswers[index].mTtl);
        ptr.SetLength(StringLength(aKnownAnswers[index].mPtrAnswer, Name::kMaxNameSize) + 1);

        SuccessOrQuit(Name::AppendPointerLabel(nameOffset, *message));
        SuccessOrQuit(message->Append(ptr));
        SuccessOrQuit(Name::AppendName(aKnownAnswers[index].mPtrAnswer, *message));
    }

    SuccessOrQuit(AsCoreType(&senderAddrInfo.mAddress).FromString(kDeviceIp6Address));
    senderAddrInfo.mPort         = kMdnsPort;
    senderAddrInfo.mInfraIfIndex = 0;

    Log("Sending query for %s PTR with %u known-answers", aName, aNumAnswers);

    otPlatMdnsHandleReceive(sInstance, message, /* aIsUnicast */ false, &senderAddrInfo);
}

static void SendEmtryPtrQueryWithKnownAnswers(const char *aName, const KnownAnswer *aKnownAnswers, uint16_t aNumAnswers)
{
    Message          *message;
    Header            header;
    Core::AddressInfo senderAddrInfo;
    uint16_t          nameOffset = 0;

    message = sInstance->Get<MessagePool>().Allocate(Message::kTypeOther);
    VerifyOrQuit(message != nullptr);

    header.Clear();
    header.SetType(Header::kTypeQuery);
    header.SetAnswerCount(aNumAnswers);

    SuccessOrQuit(message->Append(header));

    for (uint16_t index = 0; index < aNumAnswers; index++)
    {
        PtrRecord ptr;

        ptr.Init();
        ptr.SetTtl(aKnownAnswers[index].mTtl);
        ptr.SetLength(StringLength(aKnownAnswers[index].mPtrAnswer, Name::kMaxNameSize) + 1);

        if (nameOffset == 0)
        {
            nameOffset = message->GetLength();
            SuccessOrQuit(Name::AppendName(aName, *message));
        }
        else
        {
            SuccessOrQuit(Name::AppendPointerLabel(nameOffset, *message));
        }

        SuccessOrQuit(message->Append(ptr));
        SuccessOrQuit(Name::AppendName(aKnownAnswers[index].mPtrAnswer, *message));
    }

    SuccessOrQuit(AsCoreType(&senderAddrInfo.mAddress).FromString(kDeviceIp6Address));
    senderAddrInfo.mPort         = kMdnsPort;
    senderAddrInfo.mInfraIfIndex = 0;

    Log("Sending empty query with %u known-answers for %s", aNumAnswers, aName);

    otPlatMdnsHandleReceive(sInstance, message, /* aIsUnicast */ false, &senderAddrInfo);
}

//----------------------------------------------------------------------------------------------------------------------
// `otPlatLog`

extern "C" {

#if OPENTHREAD_CONFIG_LOG_OUTPUT == OPENTHREAD_CONFIG_LOG_OUTPUT_PLATFORM_DEFINED
void otPlatLog(otLogLevel aLogLevel, otLogRegion aLogRegion, const char *aFormat, ...)
{
    OT_UNUSED_VARIABLE(aLogLevel);
    OT_UNUSED_VARIABLE(aLogRegion);
    OT_UNUSED_VARIABLE(aFormat);

#if ENABLE_TEST_LOG
    va_list args;

    printf("   ");
    va_start(args, aFormat);
    vprintf(aFormat, args);
    va_end(args);

    printf("\n");
#endif
}

#endif

//----------------------------------------------------------------------------------------------------------------------
// `otPlatAlarm`

void otPlatAlarmMilliStop(otInstance *) { sAlarmOn = false; }

void otPlatAlarmMilliStartAt(otInstance *, uint32_t aT0, uint32_t aDt)
{
    sAlarmOn   = true;
    sAlarmTime = aT0 + aDt;
}

uint32_t otPlatAlarmMilliGetNow(void) { return sNow; }

//----------------------------------------------------------------------------------------------------------------------
// Heap allocation

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

//----------------------------------------------------------------------------------------------------------------------
// `otPlatMdns`

otError otPlatMdnsSetListeningEnabled(otInstance *aInstance, bool aEnable, uint32_t aInfraIfIndex)
{
    VerifyOrQuit(aInstance == sInstance);
    sInfraIfIndex = aInfraIfIndex;

    Log("otPlatMdnsSetListeningEnabled(%s)", aEnable ? "true" : "false");

    return kErrorNone;
}

void otPlatMdnsSendMulticast(otInstance *aInstance, otMessage *aMessage, uint32_t aInfraIfIndex)
{
    Message          &message = AsCoreType(aMessage);
    Core::AddressInfo senderAddrInfo;

    VerifyOrQuit(aInfraIfIndex == sInfraIfIndex);

    Log("otPlatMdnsSendMulticast(msg-len:%u)", message.GetLength());
    ParseMessage(message, nullptr);

    // Pass the multicast message back.

    SuccessOrQuit(AsCoreType(&senderAddrInfo.mAddress).FromString(kDeviceIp6Address));
    senderAddrInfo.mPort         = kMdnsPort;
    senderAddrInfo.mInfraIfIndex = 0;

    otPlatMdnsHandleReceive(sInstance, aMessage, /* aIsUnicast */ false, &senderAddrInfo);
}

void otPlatMdnsSendUnicast(otInstance *aInstance, otMessage *aMessage, const otPlatMdnsAddressInfo *aAddress)
{
    Message                 &message = AsCoreType(aMessage);
    const Core::AddressInfo &address = AsCoreType(aAddress);
    Ip6::Address             deviceAddress;

    Log("otPlatMdnsSendUnicast() - [%s]:%u", address.GetAddress().ToString().AsCString(), address.mPort);
    ParseMessage(message, AsCoreTypePtr(aAddress));

    SuccessOrQuit(deviceAddress.FromString(kDeviceIp6Address));

    if ((address.GetAddress() == deviceAddress) && (address.mPort == kMdnsPort))
    {
        Core::AddressInfo senderAddrInfo;

        SuccessOrQuit(AsCoreType(&senderAddrInfo.mAddress).FromString(kDeviceIp6Address));
        senderAddrInfo.mPort         = kMdnsPort;
        senderAddrInfo.mInfraIfIndex = 0;

        Log("otPlatMdnsSendUnicast() - unicast msg matches this device address, passing it back");
        otPlatMdnsHandleReceive(sInstance, &message, /* aIsUnicast */ true, &senderAddrInfo);
    }
    else
    {
        message.Free();
    }
}

} // extern "C"

//---------------------------------------------------------------------------------------------------------------------

void ProcessTasklets(void)
{
    while (otTaskletsArePending(sInstance))
    {
        otTaskletsProcess(sInstance);
    }
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

Core *InitTest(void)
{
    sNow     = 0;
    sAlarmOn = false;

    sDnsMessages.Clear();

    for (RegCallback &regCallbck : sRegCallbacks)
    {
        regCallbck.Reset();
    }

    sConflictCallback.Reset();

    sInstance = testInitInstance();

    VerifyOrQuit(sInstance != nullptr);

    return &sInstance->Get<Core>();
}

//----------------------------------------------------------------------------------------------------------------------

static const uint8_t kKey1[]         = {0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77};
static const uint8_t kKey2[]         = {0x12, 0x34, 0x56};
static const uint8_t kTxtData1[]     = {3, 'a', '=', '1', 0};
static const uint8_t kTxtData2[]     = {1, 'b', 0};
static const uint8_t kEmptyTxtData[] = {0};

//---------------------------------------------------------------------------------------------------------------------

void TestHostReg(void)
{
    Core             *mdns = InitTest();
    Core::Host        host;
    Ip6::Address      hostAddresses[3];
    const DnsMessage *dnsMsg;
    uint16_t          heapAllocations;
    DnsNameString     hostFullName;

    Log("-------------------------------------------------------------------------------------------");
    Log("TestHostReg");

    AdvanceTime(1);

    heapAllocations = sHeapAllocatedPtrs.GetLength();
    SuccessOrQuit(mdns->SetEnabled(true, kInfraIfIndex));

    SuccessOrQuit(hostAddresses[0].FromString("fd00::aaaa"));
    SuccessOrQuit(hostAddresses[1].FromString("fd00::bbbb"));
    SuccessOrQuit(hostAddresses[2].FromString("fd00::cccc"));

    host.mHostName        = "myhost";
    host.mAddresses       = hostAddresses;
    host.mAddressesLength = 3;
    host.mTtl             = 1500;

    hostFullName.Append("%s.local.", host.mHostName);

    Log("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -");
    Log("Register a `HostEntry`, check probes and announcements");

    sDnsMessages.Clear();

    sRegCallbacks[0].Reset();
    SuccessOrQuit(mdns->RegisterHost(host, 0, HandleSuccessCallback));

    for (uint8_t probeCount = 0; probeCount < 3; probeCount++)
    {
        sDnsMessages.Clear();

        VerifyOrQuit(!sRegCallbacks[0].mWasCalled);
        AdvanceTime(250);

        VerifyOrQuit(!sDnsMessages.IsEmpty());
        dnsMsg = sDnsMessages.GetHead();
        dnsMsg->ValidateHeader(kMulticastQuery, /* Q */ 1, /* Ans */ 0, /* Auth */ 3, /* Addnl */ 0);
        dnsMsg->ValidateAsProbeFor(host, /* aUnicastRequest */ (probeCount == 0));
        VerifyOrQuit(dnsMsg->GetNext() == nullptr);
    }

    for (uint8_t anncCount = 0; anncCount < kNumAnnounces; anncCount++)
    {
        sDnsMessages.Clear();

        AdvanceTime((anncCount == 0) ? 250 : (1U << (anncCount - 1)) * 1000);
        VerifyOrQuit(sRegCallbacks[0].mWasCalled);

        VerifyOrQuit(!sDnsMessages.IsEmpty());
        dnsMsg = sDnsMessages.GetHead();
        dnsMsg->ValidateHeader(kMulticastResponse, /* Q */ 0, /* Ans */ 3, /* Auth */ 0, /* Addnl */ 1);
        dnsMsg->Validate(host, kInAnswerSection);
        VerifyOrQuit(dnsMsg->GetNext() == nullptr);
    }

    Log("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -");
    Log("Send a query for AAAA record and validate the response");

    AdvanceTime(2000);

    sDnsMessages.Clear();
    SendQuery(hostFullName.AsCString(), ResourceRecord::kTypeAaaa);

    AdvanceTime(1000);

    dnsMsg = sDnsMessages.GetHead();
    VerifyOrQuit(dnsMsg != nullptr);
    dnsMsg->ValidateHeader(kMulticastResponse, /* Q */ 0, /* Ans */ 3, /* Auth */ 0, /* Addnl */ 1);
    dnsMsg->Validate(host, kInAnswerSection);

    Log("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -");
    Log("Send a query for ANY record and validate the response");

    AdvanceTime(2000);

    sDnsMessages.Clear();
    SendQuery(hostFullName.AsCString(), ResourceRecord::kTypeAny);

    AdvanceTime(1000);

    dnsMsg = sDnsMessages.GetHead();
    VerifyOrQuit(dnsMsg != nullptr);
    dnsMsg->ValidateHeader(kMulticastResponse, /* Q */ 0, /* Ans */ 3, /* Auth */ 0, /* Addnl */ 1);
    dnsMsg->Validate(host, kInAnswerSection);

    Log("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -");
    Log("Send a query for non-existing record and validate the response with NSEC");

    AdvanceTime(2000);

    sDnsMessages.Clear();
    SendQuery(hostFullName.AsCString(), ResourceRecord::kTypeA);

    AdvanceTime(1000);

    dnsMsg = sDnsMessages.GetHead();
    VerifyOrQuit(dnsMsg != nullptr);
    dnsMsg->ValidateHeader(kMulticastResponse, /* Q */ 0, /* Ans */ 0, /* Auth */ 0, /* Addnl */ 1);
    VerifyOrQuit(dnsMsg->mAdditionalRecords.ContainsNsec(hostFullName, ResourceRecord::kTypeAaaa));

    Log("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -");
    Log("Update number of host addresses and validate new announcements");

    host.mAddressesLength = 2;

    sRegCallbacks[1].Reset();
    sDnsMessages.Clear();
    SuccessOrQuit(mdns->RegisterHost(host, 1, HandleSuccessCallback));

    for (uint8_t anncCount = 0; anncCount < kNumAnnounces; anncCount++)
    {
        AdvanceTime((anncCount == 0) ? 0 : (1U << (anncCount - 1)) * 1000);
        VerifyOrQuit(sRegCallbacks[1].mWasCalled);

        VerifyOrQuit(!sDnsMessages.IsEmpty());
        dnsMsg = sDnsMessages.GetHead();
        dnsMsg->ValidateHeader(kMulticastResponse, /* Q */ 0, /* Ans */ 2, /* Auth */ 0, /* Addnl */ 1);
        dnsMsg->Validate(host, kInAnswerSection);
        VerifyOrQuit(dnsMsg->GetNext() == nullptr);
        sDnsMessages.Clear();
    }

    Log("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -");
    Log("Change the addresses and validate the first announce");

    host.mAddressesLength = 3;

    sRegCallbacks[0].Reset();
    sDnsMessages.Clear();
    SuccessOrQuit(mdns->RegisterHost(host, 0, HandleSuccessCallback));

    AdvanceTime(300);
    VerifyOrQuit(sRegCallbacks[0].mWasCalled);

    VerifyOrQuit(!sDnsMessages.IsEmpty());
    dnsMsg = sDnsMessages.GetHead();
    dnsMsg->ValidateHeader(kMulticastResponse, /* Q */ 0, /* Ans */ 3, /* Auth */ 0, /* Addnl */ 1);
    dnsMsg->Validate(host, kInAnswerSection);
    VerifyOrQuit(dnsMsg->GetNext() == nullptr);

    Log("Change the address list again before second announce");

    host.mAddressesLength = 1;

    sRegCallbacks[1].Reset();
    sDnsMessages.Clear();
    SuccessOrQuit(mdns->RegisterHost(host, 1, HandleSuccessCallback));

    for (uint8_t anncCount = 0; anncCount < kNumAnnounces; anncCount++)
    {
        AdvanceTime((anncCount == 0) ? 0 : (1U << (anncCount - 1)) * 1000);
        VerifyOrQuit(sRegCallbacks[1].mWasCalled);

        VerifyOrQuit(!sDnsMessages.IsEmpty());
        dnsMsg = sDnsMessages.GetHead();
        dnsMsg->ValidateHeader(kMulticastResponse, /* Q */ 0, /* Ans */ 1, /* Auth */ 0, /* Addnl */ 1);
        dnsMsg->Validate(host, kInAnswerSection);
        VerifyOrQuit(dnsMsg->GetNext() == nullptr);
        sDnsMessages.Clear();
    }

    Log("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -");
    Log("Change `HostEntry` TTL and validate announcements");

    host.mTtl = 120;

    sRegCallbacks[1].Reset();
    sDnsMessages.Clear();
    SuccessOrQuit(mdns->RegisterHost(host, 1, HandleSuccessCallback));

    for (uint8_t anncCount = 0; anncCount < kNumAnnounces; anncCount++)
    {
        AdvanceTime((anncCount == 0) ? 0 : (1U << (anncCount - 1)) * 1000);
        VerifyOrQuit(sRegCallbacks[1].mWasCalled);

        VerifyOrQuit(!sDnsMessages.IsEmpty());
        dnsMsg = sDnsMessages.GetHead();
        dnsMsg->ValidateHeader(kMulticastResponse, /* Q */ 0, /* Ans */ 1, /* Auth */ 0, /* Addnl */ 1);
        dnsMsg->Validate(host, kInAnswerSection);
        VerifyOrQuit(dnsMsg->GetNext() == nullptr);
        sDnsMessages.Clear();
    }

    Log("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -");
    Log("Send a query for AAAA record and validate the response");

    AdvanceTime(2000);

    sDnsMessages.Clear();
    SendQuery(hostFullName.AsCString(), ResourceRecord::kTypeAaaa);

    AdvanceTime(1000);

    dnsMsg = sDnsMessages.GetHead();
    VerifyOrQuit(dnsMsg != nullptr);
    dnsMsg->ValidateHeader(kMulticastResponse, /* Q */ 0, /* Ans */ 1, /* Auth */ 0, /* Addnl */ 1);
    dnsMsg->Validate(host, kInAnswerSection);

    Log("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -");
    Log("Unregister the host and validate the goodbye announces");

    sDnsMessages.Clear();
    SuccessOrQuit(mdns->UnregisterHost(host));

    for (uint8_t anncCount = 0; anncCount < kNumAnnounces; anncCount++)
    {
        AdvanceTime((anncCount == 0) ? 0 : (1U << (anncCount - 1)) * 1000);

        VerifyOrQuit(!sDnsMessages.IsEmpty());
        dnsMsg = sDnsMessages.GetHead();
        dnsMsg->ValidateHeader(kMulticastResponse, /* Q */ 0, /* Ans */ 1, /* Auth */ 0, /* Addnl */ 0);
        dnsMsg->Validate(host, kInAnswerSection, kGoodBye);
        VerifyOrQuit(dnsMsg->GetNext() == nullptr);
        sDnsMessages.Clear();
    }

    AdvanceTime(15000);
    VerifyOrQuit(sDnsMessages.IsEmpty());

    Log("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -");
    Log("Register a host with no address (first time)");

    host.mHostName        = "newhost";
    host.mAddresses       = nullptr;
    host.mAddressesLength = 0;
    host.mTtl             = 1500;

    sRegCallbacks[2].Reset();
    SuccessOrQuit(mdns->RegisterHost(host, 2, HandleSuccessCallback));

    AdvanceTime(1);
    VerifyOrQuit(sRegCallbacks[2].mWasCalled);

    Log("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -");
    Log("Register the same host now with an address");

    host.mAddresses       = &hostAddresses[0];
    host.mAddressesLength = 1;

    sRegCallbacks[3].Reset();
    SuccessOrQuit(mdns->RegisterHost(host, 3, HandleSuccessCallback));

    AdvanceTime(15000);
    VerifyOrQuit(sRegCallbacks[3].mWasCalled);

    Log("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -");
    Log("Register the same host again now with no address");

    host.mAddressesLength = 0;

    sRegCallbacks[4].Reset();
    sDnsMessages.Clear();
    SuccessOrQuit(mdns->RegisterHost(host, 4, HandleSuccessCallback));

    AdvanceTime(1);
    VerifyOrQuit(sRegCallbacks[4].mWasCalled);

    for (uint8_t anncCount = 0; anncCount < kNumAnnounces; anncCount++)
    {
        AdvanceTime((anncCount == 0) ? 0 : (1U << (anncCount - 1)) * 1000);

        VerifyOrQuit(!sDnsMessages.IsEmpty());
        dnsMsg = sDnsMessages.GetHead();
        dnsMsg->ValidateHeader(kMulticastResponse, /* Q */ 0, /* Ans */ 1, /* Auth */ 0, /* Addnl */ 0);
        dnsMsg->Validate(host, kInAnswerSection, kGoodBye);
        VerifyOrQuit(dnsMsg->GetNext() == nullptr);
        sDnsMessages.Clear();
    }

    Log("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -");
    Log("Register the same host again now adding an address");

    host.mAddresses       = &hostAddresses[1];
    host.mAddressesLength = 1;

    sRegCallbacks[5].Reset();
    SuccessOrQuit(mdns->RegisterHost(host, 5, HandleSuccessCallback));

    AdvanceTime(15000);
    VerifyOrQuit(sRegCallbacks[5].mWasCalled);

    Log("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -");

    SuccessOrQuit(mdns->SetEnabled(false, kInfraIfIndex));
    VerifyOrQuit(sHeapAllocatedPtrs.GetLength() <= heapAllocations);

    Log("End of test");

    testFreeInstance(sInstance);
}

//---------------------------------------------------------------------------------------------------------------------

void TestKeyReg(void)
{
    Core             *mdns = InitTest();
    Core::Key         key;
    const DnsMessage *dnsMsg;
    uint16_t          heapAllocations;

    Log("-------------------------------------------------------------------------------------------");
    Log("TestKeyReg");

    AdvanceTime(1);

    heapAllocations = sHeapAllocatedPtrs.GetLength();
    SuccessOrQuit(mdns->SetEnabled(true, kInfraIfIndex));

    // Run all tests twice. first with key for a host name, followed
    // by key for service instance name.

    for (uint8_t iter = 0; iter < 2; iter++)
    {
        DnsNameString fullName;

        if (iter == 0)
        {
            Log("= = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = =");
            Log("Registering key for 'myhost' host name");
            key.mName        = "myhost";
            key.mServiceType = nullptr;

            fullName.Append("%s.local.", key.mName);
        }
        else
        {
            Log("= = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = =");
            Log("Registering key for 'mysrv._srv._udo' service name");

            key.mName        = "mysrv";
            key.mServiceType = "_srv._udp";

            fullName.Append("%s.%s.local.", key.mName, key.mServiceType);
        }

        key.mKeyData       = kKey1;
        key.mKeyDataLength = sizeof(kKey1);
        key.mTtl           = 8000;

        Log("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -");
        Log("Register a key record and check probes and announcements");

        sDnsMessages.Clear();

        sRegCallbacks[0].Reset();
        SuccessOrQuit(mdns->RegisterKey(key, 0, HandleSuccessCallback));

        for (uint8_t probeCount = 0; probeCount < 3; probeCount++)
        {
            sDnsMessages.Clear();

            VerifyOrQuit(!sRegCallbacks[0].mWasCalled);
            AdvanceTime(250);

            VerifyOrQuit(!sDnsMessages.IsEmpty());
            dnsMsg = sDnsMessages.GetHead();
            dnsMsg->ValidateHeader(kMulticastQuery, /* Q */ 1, /* Ans */ 0, /* Auth */ 1, /* Addnl */ 0);
            dnsMsg->ValidateAsProbeFor(key, /* aUnicastRequest */ (probeCount == 0));
            VerifyOrQuit(dnsMsg->GetNext() == nullptr);
        }

        for (uint8_t anncCount = 0; anncCount < kNumAnnounces; anncCount++)
        {
            sDnsMessages.Clear();

            AdvanceTime((anncCount == 0) ? 250 : (1U << (anncCount - 1)) * 1000);
            VerifyOrQuit(sRegCallbacks[0].mWasCalled);

            VerifyOrQuit(!sDnsMessages.IsEmpty());
            dnsMsg = sDnsMessages.GetHead();
            dnsMsg->ValidateHeader(kMulticastResponse, /* Q */ 0, /* Ans */ 1, /* Auth */ 0, /* Addnl */ 1);
            dnsMsg->Validate(key, kInAnswerSection);
            VerifyOrQuit(dnsMsg->GetNext() == nullptr);
        }

        Log("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -");
        Log("Send a query for KEY record and validate the response");

        AdvanceTime(2000);

        sDnsMessages.Clear();
        SendQuery(fullName.AsCString(), ResourceRecord::kTypeKey);

        AdvanceTime(1000);

        dnsMsg = sDnsMessages.GetHead();
        VerifyOrQuit(dnsMsg != nullptr);
        dnsMsg->ValidateHeader(kMulticastResponse, /* Q */ 0, /* Ans */ 1, /* Auth */ 0, /* Addnl */ 1);
        dnsMsg->Validate(key, kInAnswerSection);

        Log("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -");
        Log("Send a query for ANY record and validate the response");

        AdvanceTime(2000);

        sDnsMessages.Clear();
        SendQuery(fullName.AsCString(), ResourceRecord::kTypeAny);

        AdvanceTime(1000);

        dnsMsg = sDnsMessages.GetHead();
        VerifyOrQuit(dnsMsg != nullptr);
        dnsMsg->ValidateHeader(kMulticastResponse, /* Q */ 0, /* Ans */ 1, /* Auth */ 0, /* Addnl */ 1);
        dnsMsg->Validate(key, kInAnswerSection);

        Log("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -");
        Log("Send a query for non-existing record and validate the response with NSEC");

        AdvanceTime(2000);

        sDnsMessages.Clear();
        SendQuery(fullName.AsCString(), ResourceRecord::kTypeA);

        AdvanceTime(1000);

        dnsMsg = sDnsMessages.GetHead();
        VerifyOrQuit(dnsMsg != nullptr);
        dnsMsg->ValidateHeader(kMulticastResponse, /* Q */ 0, /* Ans */ 0, /* Auth */ 0, /* Addnl */ 1);
        VerifyOrQuit(dnsMsg->mAdditionalRecords.ContainsNsec(fullName, ResourceRecord::kTypeKey));

        Log("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -");
        Log("Change the TTL");

        key.mTtl = 0; // Use default

        sRegCallbacks[1].Reset();
        sDnsMessages.Clear();
        SuccessOrQuit(mdns->RegisterKey(key, 1, HandleSuccessCallback));

        for (uint8_t anncCount = 0; anncCount < kNumAnnounces; anncCount++)
        {
            AdvanceTime((anncCount == 0) ? 0 : (1U << (anncCount - 1)) * 1000);
            VerifyOrQuit(sRegCallbacks[1].mWasCalled);

            VerifyOrQuit(!sDnsMessages.IsEmpty());
            dnsMsg = sDnsMessages.GetHead();
            dnsMsg->ValidateHeader(kMulticastResponse, /* Q */ 0, /* Ans */ 1, /* Auth */ 0, /* Addnl */ 1);
            dnsMsg->Validate(key, kInAnswerSection);
            VerifyOrQuit(dnsMsg->GetNext() == nullptr);

            sDnsMessages.Clear();
        }

        Log("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -");
        Log("Change the key");

        key.mKeyData       = kKey2;
        key.mKeyDataLength = sizeof(kKey2);

        sRegCallbacks[1].Reset();
        sDnsMessages.Clear();
        SuccessOrQuit(mdns->RegisterKey(key, 1, HandleSuccessCallback));

        for (uint8_t anncCount = 0; anncCount < kNumAnnounces; anncCount++)
        {
            AdvanceTime((anncCount == 0) ? 0 : (1U << (anncCount - 1)) * 1000);
            VerifyOrQuit(sRegCallbacks[1].mWasCalled);

            VerifyOrQuit(!sDnsMessages.IsEmpty());
            dnsMsg = sDnsMessages.GetHead();
            dnsMsg->ValidateHeader(kMulticastResponse, /* Q */ 0, /* Ans */ 1, /* Auth */ 0, /* Addnl */ 1);
            dnsMsg->Validate(key, kInAnswerSection);
            VerifyOrQuit(dnsMsg->GetNext() == nullptr);

            sDnsMessages.Clear();
        }

        Log("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -");
        Log("Unregister the key and validate the goodbye announces");

        sDnsMessages.Clear();
        SuccessOrQuit(mdns->UnregisterKey(key));

        for (uint8_t anncCount = 0; anncCount < kNumAnnounces; anncCount++)
        {
            AdvanceTime((anncCount == 0) ? 0 : (1U << (anncCount - 1)) * 1000);

            VerifyOrQuit(!sDnsMessages.IsEmpty());
            dnsMsg = sDnsMessages.GetHead();
            dnsMsg->ValidateHeader(kMulticastResponse, /* Q */ 0, /* Ans */ 1, /* Auth */ 0, /* Addnl */ 0);
            dnsMsg->Validate(key, kInAnswerSection, kGoodBye);
            VerifyOrQuit(dnsMsg->GetNext() == nullptr);

            sDnsMessages.Clear();
        }
    }

    AdvanceTime(15000);
    VerifyOrQuit(sDnsMessages.IsEmpty());

    SuccessOrQuit(mdns->SetEnabled(false, kInfraIfIndex));
    VerifyOrQuit(sHeapAllocatedPtrs.GetLength() <= heapAllocations);

    Log("End of test");

    testFreeInstance(sInstance);
}

//---------------------------------------------------------------------------------------------------------------------

void TestServiceReg(void)
{
    Core             *mdns = InitTest();
    Core::Service     service;
    const DnsMessage *dnsMsg;
    uint16_t          heapAllocations;
    DnsNameString     fullServiceName;
    DnsNameString     fullServiceType;

    Log("-------------------------------------------------------------------------------------------");
    Log("TestServiceReg");

    AdvanceTime(1);

    heapAllocations = sHeapAllocatedPtrs.GetLength();
    SuccessOrQuit(mdns->SetEnabled(true, kInfraIfIndex));

    service.mHostName            = "myhost";
    service.mServiceInstance     = "myservice";
    service.mServiceType         = "_srv._udp";
    service.mSubTypeLabels       = nullptr;
    service.mSubTypeLabelsLength = 0;
    service.mTxtData             = kTxtData1;
    service.mTxtDataLength       = sizeof(kTxtData1);
    service.mPort                = 1234;
    service.mPriority            = 1;
    service.mWeight              = 2;
    service.mTtl                 = 1000;

    fullServiceName.Append("%s.%s.local.", service.mServiceInstance, service.mServiceType);
    fullServiceType.Append("%s.local.", service.mServiceType);

    Log("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -");
    Log("Register a `ServiceEntry`, check probes and announcements");

    sDnsMessages.Clear();

    sRegCallbacks[0].Reset();
    SuccessOrQuit(mdns->RegisterService(service, 0, HandleSuccessCallback));

    for (uint8_t probeCount = 0; probeCount < 3; probeCount++)
    {
        sDnsMessages.Clear();

        VerifyOrQuit(!sRegCallbacks[0].mWasCalled);
        AdvanceTime(250);

        VerifyOrQuit(!sDnsMessages.IsEmpty());
        dnsMsg = sDnsMessages.GetHead();
        dnsMsg->ValidateHeader(kMulticastQuery, /* Q */ 1, /* Ans */ 0, /* Auth */ 2, /* Addnl */ 0);
        dnsMsg->ValidateAsProbeFor(service, /* aUnicastRequest */ (probeCount == 0));
        VerifyOrQuit(dnsMsg->GetNext() == nullptr);
    }

    for (uint8_t anncCount = 0; anncCount < kNumAnnounces; anncCount++)
    {
        sDnsMessages.Clear();

        AdvanceTime((anncCount == 0) ? 250 : (1U << (anncCount - 1)) * 1000);
        VerifyOrQuit(sRegCallbacks[0].mWasCalled);

        VerifyOrQuit(!sDnsMessages.IsEmpty());
        dnsMsg = sDnsMessages.GetHead();
        dnsMsg->ValidateHeader(kMulticastResponse, /* Q */ 0, /* Ans */ 4, /* Auth */ 0, /* Addnl */ 1);
        dnsMsg->Validate(service, kInAnswerSection, kCheckSrv | kCheckTxt | kCheckPtr | kCheckServicesPtr);
        VerifyOrQuit(dnsMsg->GetNext() == nullptr);
    }

    Log("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -");
    Log("Send a query for SRV record and validate the response");

    AdvanceTime(2000);

    sDnsMessages.Clear();
    SendQuery(fullServiceName.AsCString(), ResourceRecord::kTypeSrv);

    AdvanceTime(1000);

    dnsMsg = sDnsMessages.GetHead();
    VerifyOrQuit(dnsMsg != nullptr);
    dnsMsg->ValidateHeader(kMulticastResponse, /* Q */ 0, /* Ans */ 1, /* Auth */ 0, /* Addnl */ 1);
    dnsMsg->Validate(service, kInAnswerSection, kCheckSrv);

    Log("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -");
    Log("Send a query for TXT record and validate the response");

    AdvanceTime(2000);

    sDnsMessages.Clear();
    SendQuery(fullServiceName.AsCString(), ResourceRecord::kTypeTxt);

    AdvanceTime(1000);

    dnsMsg = sDnsMessages.GetHead();
    VerifyOrQuit(dnsMsg != nullptr);
    dnsMsg->ValidateHeader(kMulticastResponse, /* Q */ 0, /* Ans */ 1, /* Auth */ 0, /* Addnl */ 1);
    dnsMsg->Validate(service, kInAnswerSection, kCheckTxt);

    Log("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -");
    Log("Send a query for ANY record and validate the response");

    AdvanceTime(2000);

    sDnsMessages.Clear();
    SendQuery(fullServiceName.AsCString(), ResourceRecord::kTypeAny);

    AdvanceTime(1000);

    dnsMsg = sDnsMessages.GetHead();
    VerifyOrQuit(dnsMsg != nullptr);
    dnsMsg->ValidateHeader(kMulticastResponse, /* Q */ 0, /* Ans */ 2, /* Auth */ 0, /* Addnl */ 1);
    dnsMsg->Validate(service, kInAnswerSection, kCheckSrv | kCheckTxt);

    Log("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -");
    Log("Send a query for PTR record for service type and validate the response");

    AdvanceTime(2000);

    sDnsMessages.Clear();
    SendQuery(fullServiceType.AsCString(), ResourceRecord::kTypePtr);

    AdvanceTime(1000);

    dnsMsg = sDnsMessages.GetHead();
    VerifyOrQuit(dnsMsg != nullptr);
    dnsMsg->ValidateHeader(kMulticastResponse, /* Q */ 0, /* Ans */ 1, /* Auth */ 0, /* Addnl */ 2);
    dnsMsg->Validate(service, kInAnswerSection, kCheckPtr);
    dnsMsg->Validate(service, kInAdditionalSection, kCheckSrv | kCheckTxt);

    Log("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -");
    Log("Send a query for PTR record for `services._dns-sd` and validate the response");

    AdvanceTime(2000);

    sDnsMessages.Clear();
    SendQuery("_services._dns-sd._udp.local.", ResourceRecord::kTypePtr);

    AdvanceTime(1000);

    dnsMsg = sDnsMessages.GetHead();
    VerifyOrQuit(dnsMsg != nullptr);
    dnsMsg->ValidateHeader(kMulticastResponse, /* Q */ 0, /* Ans */ 1, /* Auth */ 0, /* Addnl */ 0);
    dnsMsg->Validate(service, kInAnswerSection, kCheckServicesPtr);

    Log("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -");
    Log("Update service port number and validate new announcements of SRV record");

    service.mPort = 4567;

    sRegCallbacks[1].Reset();
    sDnsMessages.Clear();
    SuccessOrQuit(mdns->RegisterService(service, 1, HandleSuccessCallback));

    for (uint8_t anncCount = 0; anncCount < kNumAnnounces; anncCount++)
    {
        AdvanceTime((anncCount == 0) ? 0 : (1U << (anncCount - 1)) * 1000);
        VerifyOrQuit(sRegCallbacks[1].mWasCalled);

        VerifyOrQuit(!sDnsMessages.IsEmpty());
        dnsMsg = sDnsMessages.GetHead();
        dnsMsg->ValidateHeader(kMulticastResponse, /* Q */ 0, /* Ans */ 1, /* Auth */ 0, /* Addnl */ 1);
        dnsMsg->Validate(service, kInAnswerSection, kCheckSrv);
        VerifyOrQuit(dnsMsg->GetNext() == nullptr);
        sDnsMessages.Clear();
    }

    Log("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -");
    Log("Update TXT data and validate new announcements of TXT record");

    service.mTxtData       = nullptr;
    service.mTxtDataLength = 0;

    sRegCallbacks[1].Reset();
    sDnsMessages.Clear();
    SuccessOrQuit(mdns->RegisterService(service, 1, HandleSuccessCallback));

    for (uint8_t anncCount = 0; anncCount < kNumAnnounces; anncCount++)
    {
        AdvanceTime((anncCount == 0) ? 0 : (1U << (anncCount - 1)) * 1000);
        VerifyOrQuit(sRegCallbacks[1].mWasCalled);

        VerifyOrQuit(!sDnsMessages.IsEmpty());
        dnsMsg = sDnsMessages.GetHead();
        dnsMsg->ValidateHeader(kMulticastResponse, /* Q */ 0, /* Ans */ 1, /* Auth */ 0, /* Addnl */ 1);
        dnsMsg->Validate(service, kInAnswerSection, kCheckTxt);
        VerifyOrQuit(dnsMsg->GetNext() == nullptr);
        sDnsMessages.Clear();
    }

    Log("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -");
    Log("Update both service and TXT data and validate new announcements of both records");

    service.mTxtData       = kTxtData2;
    service.mTxtDataLength = sizeof(kTxtData2);
    service.mWeight        = 0;

    sRegCallbacks[1].Reset();
    sDnsMessages.Clear();
    SuccessOrQuit(mdns->RegisterService(service, 1, HandleSuccessCallback));

    for (uint8_t anncCount = 0; anncCount < kNumAnnounces; anncCount++)
    {
        AdvanceTime((anncCount == 0) ? 0 : (1U << (anncCount - 1)) * 1000);
        VerifyOrQuit(sRegCallbacks[1].mWasCalled);

        VerifyOrQuit(!sDnsMessages.IsEmpty());
        dnsMsg = sDnsMessages.GetHead();
        dnsMsg->ValidateHeader(kMulticastResponse, /* Q */ 0, /* Ans */ 2, /* Auth */ 0, /* Addnl */ 1);
        dnsMsg->Validate(service, kInAnswerSection, kCheckSrv | kCheckTxt);
        VerifyOrQuit(dnsMsg->GetNext() == nullptr);
        sDnsMessages.Clear();
    }

    Log("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -");
    Log("Update service host name and validate new announcements of SRV record");

    service.mHostName = "newhost";

    sRegCallbacks[1].Reset();
    sDnsMessages.Clear();
    SuccessOrQuit(mdns->RegisterService(service, 1, HandleSuccessCallback));

    for (uint8_t anncCount = 0; anncCount < kNumAnnounces; anncCount++)
    {
        AdvanceTime((anncCount == 0) ? 0 : (1U << (anncCount - 1)) * 1000);
        VerifyOrQuit(sRegCallbacks[1].mWasCalled);

        VerifyOrQuit(!sDnsMessages.IsEmpty());
        dnsMsg = sDnsMessages.GetHead();
        dnsMsg->ValidateHeader(kMulticastResponse, /* Q */ 0, /* Ans */ 1, /* Auth */ 0, /* Addnl */ 1);
        dnsMsg->Validate(service, kInAnswerSection, kCheckSrv);
        VerifyOrQuit(dnsMsg->GetNext() == nullptr);
        sDnsMessages.Clear();
    }

    Log("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -");
    Log("Update TTL and validate new announcements of SRV, TXT and PTR records");

    service.mTtl = 0;

    sRegCallbacks[1].Reset();
    sDnsMessages.Clear();
    SuccessOrQuit(mdns->RegisterService(service, 1, HandleSuccessCallback));

    for (uint8_t anncCount = 0; anncCount < kNumAnnounces; anncCount++)
    {
        AdvanceTime((anncCount == 0) ? 0 : (1U << (anncCount - 1)) * 1000);
        VerifyOrQuit(sRegCallbacks[1].mWasCalled);

        VerifyOrQuit(!sDnsMessages.IsEmpty());
        dnsMsg = sDnsMessages.GetHead();
        dnsMsg->ValidateHeader(kMulticastResponse, /* Q */ 0, /* Ans */ 3, /* Auth */ 0, /* Addnl */ 1);
        dnsMsg->Validate(service, kInAnswerSection, kCheckSrv | kCheckTxt | kCheckPtr);
        VerifyOrQuit(dnsMsg->GetNext() == nullptr);
        sDnsMessages.Clear();
    }

    Log("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -");
    Log("Unregister the service and validate the goodbye announces");

    sDnsMessages.Clear();
    SuccessOrQuit(mdns->UnregisterService(service));

    for (uint8_t anncCount = 0; anncCount < kNumAnnounces; anncCount++)
    {
        AdvanceTime((anncCount == 0) ? 0 : (1U << (anncCount - 1)) * 1000);

        VerifyOrQuit(!sDnsMessages.IsEmpty());
        dnsMsg = sDnsMessages.GetHead();
        dnsMsg->ValidateHeader(kMulticastResponse, /* Q */ 0, /* Ans */ 3, /* Auth */ 0, /* Addnl */ 0);
        dnsMsg->Validate(service, kInAnswerSection, kCheckSrv | kCheckTxt | kCheckPtr, kGoodBye);
        VerifyOrQuit(dnsMsg->GetNext() == nullptr);
        sDnsMessages.Clear();
    }

    AdvanceTime(15000);
    VerifyOrQuit(sDnsMessages.IsEmpty());

    SuccessOrQuit(mdns->SetEnabled(false, kInfraIfIndex));
    VerifyOrQuit(sHeapAllocatedPtrs.GetLength() <= heapAllocations);

    Log("End of test");

    testFreeInstance(sInstance);
}

//---------------------------------------------------------------------------------------------------------------------

void TestUnregisterBeforeProbeFinished(void)
{
    const uint8_t kKey1[] = {0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77};

    Core             *mdns = InitTest();
    Core::Host        host;
    Core::Service     service;
    Core::Key         key;
    Ip6::Address      hostAddresses[3];
    const DnsMessage *dnsMsg;
    uint16_t          heapAllocations;

    Log("-------------------------------------------------------------------------------------------");
    Log("TestUnregisterBeforeProbeFinished");

    AdvanceTime(1);

    heapAllocations = sHeapAllocatedPtrs.GetLength();
    SuccessOrQuit(mdns->SetEnabled(true, kInfraIfIndex));

    SuccessOrQuit(hostAddresses[0].FromString("fd00::aaaa"));
    SuccessOrQuit(hostAddresses[1].FromString("fd00::bbbb"));
    SuccessOrQuit(hostAddresses[2].FromString("fd00::cccc"));

    host.mHostName        = "myhost";
    host.mAddresses       = hostAddresses;
    host.mAddressesLength = 3;
    host.mTtl             = 1500;

    service.mHostName            = "myhost";
    service.mServiceInstance     = "myservice";
    service.mServiceType         = "_srv._udp";
    service.mSubTypeLabels       = nullptr;
    service.mSubTypeLabelsLength = 0;
    service.mTxtData             = kTxtData1;
    service.mTxtDataLength       = sizeof(kTxtData1);
    service.mPort                = 1234;
    service.mPriority            = 1;
    service.mWeight              = 2;
    service.mTtl                 = 1000;

    key.mName          = "mysrv";
    key.mServiceType   = "_srv._udp";
    key.mKeyData       = kKey1;
    key.mKeyDataLength = sizeof(kKey1);
    key.mTtl           = 8000;

    // Repeat the same test 3 times for host and service and key registration.

    for (uint8_t iter = 0; iter < 3; iter++)
    {
        Log("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -");
        Log("Register an entry, check for the first two probes");

        sDnsMessages.Clear();

        sRegCallbacks[0].Reset();

        switch (iter)
        {
        case 0:
            SuccessOrQuit(mdns->RegisterHost(host, 0, HandleSuccessCallback));
            break;
        case 1:
            SuccessOrQuit(mdns->RegisterService(service, 0, HandleSuccessCallback));
            break;
        case 2:
            SuccessOrQuit(mdns->RegisterKey(key, 0, HandleSuccessCallback));
            break;
        }

        for (uint8_t probeCount = 0; probeCount < 2; probeCount++)
        {
            sDnsMessages.Clear();

            VerifyOrQuit(!sRegCallbacks[0].mWasCalled);
            AdvanceTime(250);

            VerifyOrQuit(!sDnsMessages.IsEmpty());
            dnsMsg = sDnsMessages.GetHead();

            switch (iter)
            {
            case 0:
                dnsMsg->ValidateHeader(kMulticastQuery, /* Q */ 1, /* Ans */ 0, /* Auth */ 3, /* Addnl */ 0);
                dnsMsg->ValidateAsProbeFor(host, /* aUnicastRequest */ (probeCount == 0));
                break;
            case 1:
                dnsMsg->ValidateHeader(kMulticastQuery, /* Q */ 1, /* Ans */ 0, /* Auth */ 2, /* Addnl */ 0);
                dnsMsg->ValidateAsProbeFor(service, /* aUnicastRequest */ (probeCount == 0));
                break;
            case 2:
                dnsMsg->ValidateHeader(kMulticastQuery, /* Q */ 1, /* Ans */ 0, /* Auth */ 1, /* Addnl */ 0);
                dnsMsg->ValidateAsProbeFor(key, /* aUnicastRequest */ (probeCount == 0));
                break;
            }

            VerifyOrQuit(dnsMsg->GetNext() == nullptr);
        }

        sDnsMessages.Clear();
        VerifyOrQuit(!sRegCallbacks[0].mWasCalled);

        Log("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -");
        Log("Unregister the entry before the last probe and make sure probing stops");

        switch (iter)
        {
        case 0:
            SuccessOrQuit(mdns->UnregisterHost(host));
            break;
        case 1:
            SuccessOrQuit(mdns->UnregisterService(service));
            break;
        case 2:
            SuccessOrQuit(mdns->UnregisterKey(key));
            break;
        }

        AdvanceTime(20 * 1000);
        VerifyOrQuit(!sRegCallbacks[0].mWasCalled);

        VerifyOrQuit(sDnsMessages.IsEmpty());
    }

    SuccessOrQuit(mdns->SetEnabled(false, kInfraIfIndex));
    VerifyOrQuit(sHeapAllocatedPtrs.GetLength() <= heapAllocations);

    Log("End of test");

    testFreeInstance(sInstance);
}

//---------------------------------------------------------------------------------------------------------------------

void TestServiceSubTypeReg(void)
{
    static const char *const kSubTypes1[] = {"_s1", "_r2", "_vXy", "_last"};
    static const char *const kSubTypes2[] = {"_vxy", "_r1", "_r2", "_zzz"};

    Core             *mdns = InitTest();
    Core::Service     service;
    const DnsMessage *dnsMsg;
    uint16_t          heapAllocations;
    DnsNameString     fullServiceName;
    DnsNameString     fullServiceType;
    DnsNameString     fullSubServiceType;

    Log("-------------------------------------------------------------------------------------------");
    Log("TestServiceSubTypeReg");

    AdvanceTime(1);

    heapAllocations = sHeapAllocatedPtrs.GetLength();
    SuccessOrQuit(mdns->SetEnabled(true, kInfraIfIndex));

    service.mHostName            = "tarnished";
    service.mServiceInstance     = "elden";
    service.mServiceType         = "_ring._udp";
    service.mSubTypeLabels       = kSubTypes1;
    service.mSubTypeLabelsLength = 3;
    service.mTxtData             = kTxtData1;
    service.mTxtDataLength       = sizeof(kTxtData1);
    service.mPort                = 1234;
    service.mPriority            = 1;
    service.mWeight              = 2;
    service.mTtl                 = 6000;

    fullServiceName.Append("%s.%s.local.", service.mServiceInstance, service.mServiceType);
    fullServiceType.Append("%s.local.", service.mServiceType);

    Log("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -");
    Log("Register a `ServiceEntry` with sub-types, check probes and announcements");

    sDnsMessages.Clear();

    sRegCallbacks[0].Reset();
    SuccessOrQuit(mdns->RegisterService(service, 0, HandleSuccessCallback));

    for (uint8_t probeCount = 0; probeCount < 3; probeCount++)
    {
        sDnsMessages.Clear();

        VerifyOrQuit(!sRegCallbacks[0].mWasCalled);
        AdvanceTime(250);

        VerifyOrQuit(!sDnsMessages.IsEmpty());
        dnsMsg = sDnsMessages.GetHead();
        dnsMsg->ValidateHeader(kMulticastQuery, /* Q */ 1, /* Ans */ 0, /* Auth */ 2, /* Addnl */ 0);
        dnsMsg->ValidateAsProbeFor(service, /* aUnicastRequest */ (probeCount == 0));
        VerifyOrQuit(dnsMsg->GetNext() == nullptr);
    }

    for (uint8_t anncCount = 0; anncCount < kNumAnnounces; anncCount++)
    {
        sDnsMessages.Clear();

        AdvanceTime((anncCount == 0) ? 250 : (1U << (anncCount - 1)) * 1000);
        VerifyOrQuit(sRegCallbacks[0].mWasCalled);

        VerifyOrQuit(!sDnsMessages.IsEmpty());
        dnsMsg = sDnsMessages.GetHead();
        dnsMsg->ValidateHeader(kMulticastResponse, /* Q */ 0, /* Ans */ 7, /* Auth */ 0, /* Addnl */ 1);
        dnsMsg->Validate(service, kInAnswerSection, kCheckSrv | kCheckTxt | kCheckPtr | kCheckServicesPtr);

        for (uint16_t index = 0; index < service.mSubTypeLabelsLength; index++)
        {
            dnsMsg->ValidateSubType(service.mSubTypeLabels[index], service);
        }

        VerifyOrQuit(dnsMsg->GetNext() == nullptr);
    }

    Log("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -");
    Log("Send a query for SRV record and validate the response");

    AdvanceTime(2000);

    sDnsMessages.Clear();
    SendQuery(fullServiceName.AsCString(), ResourceRecord::kTypeSrv);

    AdvanceTime(1000);

    dnsMsg = sDnsMessages.GetHead();
    VerifyOrQuit(dnsMsg != nullptr);
    dnsMsg->ValidateHeader(kMulticastResponse, /* Q */ 0, /* Ans */ 1, /* Auth */ 0, /* Addnl */ 1);
    dnsMsg->Validate(service, kInAnswerSection, kCheckSrv);

    Log("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -");
    Log("Send a query for TXT record and validate the response");

    AdvanceTime(2000);

    sDnsMessages.Clear();
    SendQuery(fullServiceName.AsCString(), ResourceRecord::kTypeTxt);

    AdvanceTime(1000);

    dnsMsg = sDnsMessages.GetHead();
    VerifyOrQuit(dnsMsg != nullptr);
    dnsMsg->ValidateHeader(kMulticastResponse, /* Q */ 0, /* Ans */ 1, /* Auth */ 0, /* Addnl */ 1);
    dnsMsg->Validate(service, kInAnswerSection, kCheckTxt);

    Log("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -");
    Log("Send a query for ANY record and validate the response");

    AdvanceTime(2000);

    sDnsMessages.Clear();
    SendQuery(fullServiceName.AsCString(), ResourceRecord::kTypeAny);

    AdvanceTime(1000);

    dnsMsg = sDnsMessages.GetHead();
    VerifyOrQuit(dnsMsg != nullptr);
    dnsMsg->ValidateHeader(kMulticastResponse, /* Q */ 0, /* Ans */ 2, /* Auth */ 0, /* Addnl */ 1);
    dnsMsg->Validate(service, kInAnswerSection, kCheckSrv | kCheckTxt);

    Log("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -");
    Log("Send a query for PTR record for service type and validate the response");

    AdvanceTime(2000);

    sDnsMessages.Clear();
    SendQuery(fullServiceType.AsCString(), ResourceRecord::kTypePtr);

    AdvanceTime(1000);

    dnsMsg = sDnsMessages.GetHead();
    VerifyOrQuit(dnsMsg != nullptr);
    dnsMsg->ValidateHeader(kMulticastResponse, /* Q */ 0, /* Ans */ 1, /* Auth */ 0, /* Addnl */ 2);
    dnsMsg->Validate(service, kInAnswerSection, kCheckPtr);
    dnsMsg->Validate(service, kInAdditionalSection, kCheckSrv | kCheckTxt);

    Log("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -");
    Log("Send a query for PTR record for `services._dns-sd` and validate the response");

    AdvanceTime(2000);

    sDnsMessages.Clear();
    SendQuery("_services._dns-sd._udp.local.", ResourceRecord::kTypePtr);

    AdvanceTime(1000);

    dnsMsg = sDnsMessages.GetHead();
    VerifyOrQuit(dnsMsg != nullptr);
    dnsMsg->ValidateHeader(kMulticastResponse, /* Q */ 0, /* Ans */ 1, /* Auth */ 0, /* Addnl */ 0);
    dnsMsg->Validate(service, kInAnswerSection, kCheckServicesPtr);

    for (uint16_t index = 0; index < service.mSubTypeLabelsLength; index++)
    {
        Log("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -");
        Log("Send a PTR query for sub-type `%s` and validate the response", service.mSubTypeLabels[index]);

        fullSubServiceType.Clear();
        fullSubServiceType.Append("%s._sub.%s", service.mSubTypeLabels[index], fullServiceType.AsCString());

        AdvanceTime(2000);

        sDnsMessages.Clear();
        SendQuery(fullSubServiceType.AsCString(), ResourceRecord::kTypePtr);

        AdvanceTime(1000);

        dnsMsg = sDnsMessages.GetHead();
        VerifyOrQuit(dnsMsg != nullptr);
        dnsMsg->ValidateHeader(kMulticastResponse, /* Q */ 0, /* Ans */ 1, /* Auth */ 0, /* Addnl */ 2);
        dnsMsg->ValidateSubType(service.mSubTypeLabels[index], service);
        dnsMsg->Validate(service, kInAdditionalSection, kCheckSrv | kCheckTxt);
    }

    Log("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -");
    Log("Send a PTR query for non-existing sub-type and validate there is no response");

    AdvanceTime(2000);

    fullSubServiceType.Clear();
    fullSubServiceType.Append("_none._sub.%s", fullServiceType.AsCString());

    sDnsMessages.Clear();
    SendQuery(fullSubServiceType.AsCString(), ResourceRecord::kTypePtr);

    AdvanceTime(2000);
    VerifyOrQuit(sDnsMessages.IsEmpty());

    Log("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -");
    Log("Register a new sub-type and validate announcements of PTR record for it");

    service.mSubTypeLabelsLength = 4;

    sRegCallbacks[1].Reset();
    sDnsMessages.Clear();
    SuccessOrQuit(mdns->RegisterService(service, 1, HandleSuccessCallback));

    for (uint8_t anncCount = 0; anncCount < kNumAnnounces; anncCount++)
    {
        AdvanceTime((anncCount == 0) ? 0 : (1U << (anncCount - 1)) * 1000);
        VerifyOrQuit(sRegCallbacks[1].mWasCalled);

        VerifyOrQuit(!sDnsMessages.IsEmpty());
        dnsMsg = sDnsMessages.GetHead();
        dnsMsg->ValidateHeader(kMulticastResponse, /* Q */ 0, /* Ans */ 1, /* Auth */ 0, /* Addnl */ 2);
        dnsMsg->ValidateSubType(service.mSubTypeLabels[3], service);
        dnsMsg->Validate(service, kInAdditionalSection, kCheckSrv | kCheckTxt);
        VerifyOrQuit(dnsMsg->GetNext() == nullptr);
        sDnsMessages.Clear();
    }

    Log("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -");
    Log("Remove a previous sub-type and validate announcements of its removal");

    service.mSubTypeLabels++;
    service.mSubTypeLabelsLength = 3;

    sRegCallbacks[1].Reset();
    sDnsMessages.Clear();
    SuccessOrQuit(mdns->RegisterService(service, 1, HandleSuccessCallback));

    for (uint8_t anncCount = 0; anncCount < kNumAnnounces; anncCount++)
    {
        AdvanceTime((anncCount == 0) ? 0 : (1U << (anncCount - 1)) * 1000);
        VerifyOrQuit(sRegCallbacks[1].mWasCalled);

        VerifyOrQuit(!sDnsMessages.IsEmpty());
        dnsMsg = sDnsMessages.GetHead();
        dnsMsg->ValidateHeader(kMulticastResponse, /* Q */ 0, /* Ans */ 1, /* Auth */ 0, /* Addnl */ 0);
        dnsMsg->ValidateSubType(kSubTypes1[0], service, kGoodBye);
        VerifyOrQuit(dnsMsg->GetNext() == nullptr);
        sDnsMessages.Clear();
    }

    Log("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -");
    Log("Update TTL and validate announcement of all records");

    service.mTtl = 0;

    sRegCallbacks[1].Reset();
    sDnsMessages.Clear();
    SuccessOrQuit(mdns->RegisterService(service, 1, HandleSuccessCallback));

    for (uint8_t anncCount = 0; anncCount < kNumAnnounces; anncCount++)
    {
        AdvanceTime((anncCount == 0) ? 0 : (1U << (anncCount - 1)) * 1000);
        VerifyOrQuit(sRegCallbacks[1].mWasCalled);

        VerifyOrQuit(!sDnsMessages.IsEmpty());
        dnsMsg = sDnsMessages.GetHead();
        dnsMsg->ValidateHeader(kMulticastResponse, /* Q */ 0, /* Ans */ 6, /* Auth */ 0, /* Addnl */ 1);
        dnsMsg->Validate(service, kInAnswerSection, kCheckSrv | kCheckTxt | kCheckPtr);

        for (uint16_t index = 0; index < service.mSubTypeLabelsLength; index++)
        {
            dnsMsg->ValidateSubType(service.mSubTypeLabels[index], service);
        }

        VerifyOrQuit(dnsMsg->GetNext() == nullptr);
        sDnsMessages.Clear();
    }

    Log("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -");
    Log("Add and remove sub-types at the same time and check proper announcements");

    // Registered sub-types: _r2, _vXy, _last
    // New sub-types list  : _vxy, _r1, _r2, _zzz
    //
    // Should announce removal of `_last` and addition of
    // `_r1` and `_zzz`. The `_vxy` should match with `_vXy`.

    service.mSubTypeLabels       = kSubTypes2;
    service.mSubTypeLabelsLength = 4;

    sRegCallbacks[1].Reset();
    sDnsMessages.Clear();
    SuccessOrQuit(mdns->RegisterService(service, 1, HandleSuccessCallback));

    for (uint8_t anncCount = 0; anncCount < kNumAnnounces; anncCount++)
    {
        AdvanceTime((anncCount == 0) ? 0 : (1U << (anncCount - 1)) * 1000);
        VerifyOrQuit(sRegCallbacks[1].mWasCalled);

        VerifyOrQuit(!sDnsMessages.IsEmpty());
        dnsMsg = sDnsMessages.GetHead();
        dnsMsg->ValidateHeader(kMulticastResponse, /* Q */ 0, /* Ans */ 3, /* Auth */ 0, /* Addnl */ 2);

        dnsMsg->ValidateSubType(kSubTypes1[3], service, kGoodBye);
        dnsMsg->ValidateSubType(kSubTypes2[1], service);
        dnsMsg->ValidateSubType(kSubTypes2[3], service);
        dnsMsg->Validate(service, kInAdditionalSection, kCheckSrv | kCheckTxt);

        VerifyOrQuit(dnsMsg->GetNext() == nullptr);
        sDnsMessages.Clear();
    }

    Log("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -");
    Log("Unregister the service and validate the goodbye announces for service and its sub-types");

    sDnsMessages.Clear();
    SuccessOrQuit(mdns->UnregisterService(service));

    for (uint8_t anncCount = 0; anncCount < kNumAnnounces; anncCount++)
    {
        AdvanceTime((anncCount == 0) ? 0 : (1U << (anncCount - 1)) * 1000);

        VerifyOrQuit(!sDnsMessages.IsEmpty());
        dnsMsg = sDnsMessages.GetHead();
        dnsMsg->ValidateHeader(kMulticastResponse, /* Q */ 0, /* Ans */ 7, /* Auth */ 0, /* Addnl */ 0);
        dnsMsg->Validate(service, kInAnswerSection, kCheckSrv | kCheckTxt | kCheckPtr, kGoodBye);

        for (uint16_t index = 0; index < service.mSubTypeLabelsLength; index++)
        {
            dnsMsg->ValidateSubType(service.mSubTypeLabels[index], service, kGoodBye);
        }

        VerifyOrQuit(dnsMsg->GetNext() == nullptr);
        sDnsMessages.Clear();
    }

    AdvanceTime(15000);
    VerifyOrQuit(sDnsMessages.IsEmpty());

    SuccessOrQuit(mdns->SetEnabled(false, kInfraIfIndex));
    VerifyOrQuit(sHeapAllocatedPtrs.GetLength() <= heapAllocations);

    Log("End of test");

    testFreeInstance(sInstance);
}

void TestHostOrServiceAndKeyReg(void)
{
    Core             *mdns = InitTest();
    Core::Host        host;
    Core::Service     service;
    Core::Key         key;
    Ip6::Address      hostAddresses[2];
    const DnsMessage *dnsMsg;
    uint16_t          heapAllocations;

    Log("-------------------------------------------------------------------------------------------");
    Log("TestHostOrServiceAndKeyReg");

    AdvanceTime(1);

    heapAllocations = sHeapAllocatedPtrs.GetLength();
    SuccessOrQuit(mdns->SetEnabled(true, kInfraIfIndex));

    SuccessOrQuit(hostAddresses[0].FromString("fd00::1"));
    SuccessOrQuit(hostAddresses[1].FromString("fd00::2"));

    host.mHostName        = "myhost";
    host.mAddresses       = hostAddresses;
    host.mAddressesLength = 2;
    host.mTtl             = 5000;

    key.mKeyData       = kKey1;
    key.mKeyDataLength = sizeof(kKey1);
    key.mTtl           = 80000;

    service.mHostName            = "myhost";
    service.mServiceInstance     = "myservice";
    service.mServiceType         = "_srv._udp";
    service.mSubTypeLabels       = nullptr;
    service.mSubTypeLabelsLength = 0;
    service.mTxtData             = kTxtData1;
    service.mTxtDataLength       = sizeof(kTxtData1);
    service.mPort                = 1234;
    service.mPriority            = 1;
    service.mWeight              = 2;
    service.mTtl                 = 1000;

    // Run all test step twice, first time registering host and key,
    // second time registering service and key.

    for (uint8_t iter = 0; iter < 2; iter++)
    {
        if (iter == 0)
        {
            key.mName        = host.mHostName;
            key.mServiceType = nullptr;
        }
        else
        {
            key.mName        = service.mServiceInstance;
            key.mServiceType = service.mServiceType;
        }

        Log("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -");
        Log("Register a %s entry, check the first probe is sent", iter == 0 ? "host" : "service");

        sDnsMessages.Clear();

        sRegCallbacks[0].Reset();

        if (iter == 0)
        {
            SuccessOrQuit(mdns->RegisterHost(host, 0, HandleSuccessCallback));
        }
        else
        {
            SuccessOrQuit(mdns->RegisterService(service, 0, HandleSuccessCallback));
        }

        sDnsMessages.Clear();

        VerifyOrQuit(!sRegCallbacks[0].mWasCalled);
        AdvanceTime(250);

        VerifyOrQuit(!sDnsMessages.IsEmpty());
        dnsMsg = sDnsMessages.GetHead();

        dnsMsg->ValidateHeader(kMulticastQuery, /* Q */ 1, /* Ans */ 0, /* Auth */ 2, /* Addnl */ 0);

        if (iter == 0)
        {
            dnsMsg->ValidateAsProbeFor(host, /* aUnicastRequest */ true);
        }
        else
        {
            dnsMsg->ValidateAsProbeFor(service, /* aUnicastRequest */ true);
        }

        VerifyOrQuit(dnsMsg->GetNext() == nullptr);

        Log("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -");
        Log("Register a `KeyEntry` for same name, check that probes continue");

        sRegCallbacks[1].Reset();
        SuccessOrQuit(mdns->RegisterKey(key, 1, HandleSuccessCallback));

        for (uint8_t probeCount = 1; probeCount < 3; probeCount++)
        {
            sDnsMessages.Clear();

            VerifyOrQuit(!sRegCallbacks[0].mWasCalled);
            VerifyOrQuit(!sRegCallbacks[1].mWasCalled);

            AdvanceTime(250);

            VerifyOrQuit(!sDnsMessages.IsEmpty());
            dnsMsg = sDnsMessages.GetHead();
            dnsMsg->ValidateHeader(kMulticastQuery, /* Q */ 1, /* Ans */ 0, /* Auth */ 3, /* Addnl */ 0);

            if (iter == 0)
            {
                dnsMsg->ValidateAsProbeFor(host, /* aUnicastRequest */ false);
            }
            else
            {
                dnsMsg->ValidateAsProbeFor(service, /* aUnicastRequest */ false);
            }

            dnsMsg->ValidateAsProbeFor(key, /* aUnicastRequest */ (probeCount == 0));
            VerifyOrQuit(dnsMsg->GetNext() == nullptr);
        }

        Log("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -");
        Log("Validate Announces for both entry and key");

        for (uint8_t anncCount = 0; anncCount < kNumAnnounces; anncCount++)
        {
            sDnsMessages.Clear();

            AdvanceTime((anncCount == 0) ? 250 : (1U << (anncCount - 1)) * 1000);
            VerifyOrQuit(sRegCallbacks[0].mWasCalled);
            VerifyOrQuit(sRegCallbacks[1].mWasCalled);

            VerifyOrQuit(!sDnsMessages.IsEmpty());
            dnsMsg = sDnsMessages.GetHead();

            if (iter == 0)
            {
                dnsMsg->ValidateHeader(kMulticastResponse, /* Q */ 0, /* Ans */ 3, /* Auth */ 0, /* Addnl */ 1);
                dnsMsg->Validate(host, kInAnswerSection);
            }
            else
            {
                dnsMsg->ValidateHeader(kMulticastResponse, /* Q */ 0, /* Ans */ 5, /* Auth */ 0, /* Addnl */ 1);
                dnsMsg->Validate(service, kInAnswerSection, kCheckSrv | kCheckTxt | kCheckPtr | kCheckServicesPtr);
            }

            dnsMsg->Validate(key, kInAnswerSection);
            VerifyOrQuit(dnsMsg->GetNext() == nullptr);
        }

        Log("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -");
        Log("Unregister the entry and validate its goodbye announces");

        sDnsMessages.Clear();

        if (iter == 0)
        {
            SuccessOrQuit(mdns->UnregisterHost(host));
        }
        else
        {
            SuccessOrQuit(mdns->UnregisterService(service));
        }

        for (uint8_t anncCount = 0; anncCount < kNumAnnounces; anncCount++)
        {
            AdvanceTime((anncCount == 0) ? 0 : (1U << (anncCount - 1)) * 1000);

            VerifyOrQuit(!sDnsMessages.IsEmpty());
            dnsMsg = sDnsMessages.GetHead();

            if (iter == 0)
            {
                dnsMsg->ValidateHeader(kMulticastResponse, /* Q */ 0, /* Ans */ 2, /* Auth */ 0, /* Addnl */ 1);
                dnsMsg->Validate(host, kInAnswerSection, kGoodBye);
            }
            else
            {
                dnsMsg->ValidateHeader(kMulticastResponse, /* Q */ 0, /* Ans */ 3, /* Auth */ 0, /* Addnl */ 1);
                dnsMsg->Validate(service, kInAnswerSection, kCheckSrv | kCheckTxt | kCheckPtr, kGoodBye);
            }

            VerifyOrQuit(dnsMsg->GetNext() == nullptr);
            sDnsMessages.Clear();
        }

        AdvanceTime(15000);
        VerifyOrQuit(sDnsMessages.IsEmpty());

        Log("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -");
        Log("Register the entry again, validate its announcements");

        sDnsMessages.Clear();

        sRegCallbacks[2].Reset();

        if (iter == 0)
        {
            SuccessOrQuit(mdns->RegisterHost(host, 2, HandleSuccessCallback));
        }
        else
        {
            SuccessOrQuit(mdns->RegisterService(service, 2, HandleSuccessCallback));
        }

        for (uint8_t anncCount = 0; anncCount < kNumAnnounces; anncCount++)
        {
            sDnsMessages.Clear();

            AdvanceTime((anncCount == 0) ? 250 : (1U << (anncCount - 1)) * 1000);
            VerifyOrQuit(sRegCallbacks[2].mWasCalled);

            VerifyOrQuit(!sDnsMessages.IsEmpty());
            dnsMsg = sDnsMessages.GetHead();

            if (iter == 0)
            {
                dnsMsg->ValidateHeader(kMulticastResponse, /* Q */ 0, /* Ans */ 2, /* Auth */ 0, /* Addnl */ 1);
                dnsMsg->Validate(host, kInAnswerSection);
            }
            else
            {
                dnsMsg->ValidateHeader(kMulticastResponse, /* Q */ 0, /* Ans */ 4, /* Auth */ 0, /* Addnl */ 1);
                dnsMsg->Validate(service, kInAnswerSection, kCheckSrv | kCheckTxt | kCheckPtr | kCheckServicesPtr);
            }

            VerifyOrQuit(dnsMsg->GetNext() == nullptr);
        }

        Log("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -");
        Log("Unregister the key and validate its goodbye announcements");

        sDnsMessages.Clear();
        SuccessOrQuit(mdns->UnregisterKey(key));

        for (uint8_t anncCount = 0; anncCount < kNumAnnounces; anncCount++)
        {
            AdvanceTime((anncCount == 0) ? 0 : (1U << (anncCount - 1)) * 1000);

            VerifyOrQuit(!sDnsMessages.IsEmpty());
            dnsMsg = sDnsMessages.GetHead();
            dnsMsg->ValidateHeader(kMulticastResponse, /* Q */ 0, /* Ans */ 1, /* Auth */ 0, /* Addnl */ 1);
            dnsMsg->Validate(key, kInAnswerSection, kGoodBye);
            VerifyOrQuit(dnsMsg->GetNext() == nullptr);
            sDnsMessages.Clear();
        }

        Log("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -");
        Log("Register the key again, validate its announcements");

        sDnsMessages.Clear();

        sRegCallbacks[3].Reset();
        SuccessOrQuit(mdns->RegisterKey(key, 3, HandleSuccessCallback));

        for (uint8_t anncCount = 0; anncCount < kNumAnnounces; anncCount++)
        {
            sDnsMessages.Clear();

            AdvanceTime((anncCount == 0) ? 250 : (1U << (anncCount - 1)) * 1000);
            VerifyOrQuit(sRegCallbacks[3].mWasCalled);

            VerifyOrQuit(!sDnsMessages.IsEmpty());
            dnsMsg = sDnsMessages.GetHead();
            dnsMsg->ValidateHeader(kMulticastResponse, /* Q */ 0, /* Ans */ 1, /* Auth */ 0, /* Addnl */ 1);
            dnsMsg->Validate(key, kInAnswerSection);
            VerifyOrQuit(dnsMsg->GetNext() == nullptr);
        }

        sDnsMessages.Clear();
        AdvanceTime(15000);
        VerifyOrQuit(sDnsMessages.IsEmpty());

        Log("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -");
        Log("Unregister key first, validate two of its goodbye announcements");

        sDnsMessages.Clear();

        SuccessOrQuit(mdns->UnregisterKey(key));

        for (uint8_t anncCount = 0; anncCount < 2; anncCount++)
        {
            sDnsMessages.Clear();

            AdvanceTime((anncCount == 0) ? 1 : (1U << (anncCount - 1)) * 1000);

            VerifyOrQuit(!sDnsMessages.IsEmpty());
            dnsMsg = sDnsMessages.GetHead();
            dnsMsg->ValidateHeader(kMulticastResponse, /* Q */ 0, /* Ans */ 1, /* Auth */ 0, /* Addnl */ 1);
            dnsMsg->Validate(key, kInAnswerSection, kGoodBye);
            VerifyOrQuit(dnsMsg->GetNext() == nullptr);
        }

        Log("Unregister entry as well");

        if (iter == 0)
        {
            SuccessOrQuit(mdns->UnregisterHost(host));
        }
        else
        {
            SuccessOrQuit(mdns->UnregisterService(service));
        }

        AdvanceTime(15000);

        for (uint16_t anncCount = 0; anncCount < 4; anncCount++)
        {
            dnsMsg = dnsMsg->GetNext();
            VerifyOrQuit(dnsMsg != nullptr);

            if (anncCount == 2)
            {
                dnsMsg->ValidateHeader(kMulticastResponse, /* Q */ 0, /* Ans */ 1, /* Auth */ 0, /* Addnl */ 0);
                dnsMsg->Validate(key, kInAnswerSection, kGoodBye);
            }
            else if (iter == 0)
            {
                dnsMsg->ValidateHeader(kMulticastResponse, /* Q */ 0, /* Ans */ 2, /* Auth */ 0, /* Addnl */ 0);
                dnsMsg->Validate(host, kInAnswerSection, kGoodBye);
            }
            else
            {
                dnsMsg->ValidateHeader(kMulticastResponse, /* Q */ 0, /* Ans */ 3, /* Auth */ 0, /* Addnl */ 0);
                dnsMsg->Validate(service, kInAnswerSection, kCheckSrv | kCheckTxt | kCheckPtr, kGoodBye);
            }
        }

        VerifyOrQuit(dnsMsg->GetNext() == nullptr);

        sDnsMessages.Clear();
        AdvanceTime(15000);
        VerifyOrQuit(sDnsMessages.IsEmpty());
    }

    SuccessOrQuit(mdns->SetEnabled(false, kInfraIfIndex));
    VerifyOrQuit(sHeapAllocatedPtrs.GetLength() <= heapAllocations);

    Log("End of test");

    testFreeInstance(sInstance);
}

//---------------------------------------------------------------------------------------------------------------------

void TestQuery(void)
{
    static const char *const kSubTypes[] = {"_s", "_r"};

    Core             *mdns = InitTest();
    Core::Host        host1;
    Core::Host        host2;
    Core::Service     service1;
    Core::Service     service2;
    Core::Service     service3;
    Core::Key         key1;
    Core::Key         key2;
    Ip6::Address      host1Addresses[3];
    Ip6::Address      host2Addresses[2];
    const DnsMessage *dnsMsg;
    uint16_t          heapAllocations;
    DnsNameString     host1FullName;
    DnsNameString     host2FullName;
    DnsNameString     service1FullName;
    DnsNameString     service2FullName;
    DnsNameString     service3FullName;
    KnownAnswer       knownAnswers[2];

    Log("-------------------------------------------------------------------------------------------");
    Log("TestQuery");

    AdvanceTime(1);

    heapAllocations = sHeapAllocatedPtrs.GetLength();
    SuccessOrQuit(mdns->SetEnabled(true, kInfraIfIndex));

    SuccessOrQuit(host1Addresses[0].FromString("fd00::1:aaaa"));
    SuccessOrQuit(host1Addresses[1].FromString("fd00::1:bbbb"));
    SuccessOrQuit(host1Addresses[2].FromString("fd00::1:cccc"));
    host1.mHostName        = "host1";
    host1.mAddresses       = host1Addresses;
    host1.mAddressesLength = 3;
    host1.mTtl             = 1500;
    host1FullName.Append("%s.local.", host1.mHostName);

    SuccessOrQuit(host2Addresses[0].FromString("fd00::2:eeee"));
    SuccessOrQuit(host2Addresses[1].FromString("fd00::2:ffff"));
    host2.mHostName        = "host2";
    host2.mAddresses       = host2Addresses;
    host2.mAddressesLength = 2;
    host2.mTtl             = 1500;
    host2FullName.Append("%s.local.", host2.mHostName);

    service1.mHostName            = host1.mHostName;
    service1.mServiceInstance     = "srv1";
    service1.mServiceType         = "_srv._udp";
    service1.mSubTypeLabels       = kSubTypes;
    service1.mSubTypeLabelsLength = 2;
    service1.mTxtData             = kTxtData1;
    service1.mTxtDataLength       = sizeof(kTxtData1);
    service1.mPort                = 1111;
    service1.mPriority            = 0;
    service1.mWeight              = 0;
    service1.mTtl                 = 1500;
    service1FullName.Append("%s.%s.local.", service1.mServiceInstance, service1.mServiceType);

    service2.mHostName            = host1.mHostName;
    service2.mServiceInstance     = "srv2";
    service2.mServiceType         = "_tst._tcp";
    service2.mSubTypeLabels       = nullptr;
    service2.mSubTypeLabelsLength = 0;
    service2.mTxtData             = nullptr;
    service2.mTxtDataLength       = 0;
    service2.mPort                = 2222;
    service2.mPriority            = 2;
    service2.mWeight              = 2;
    service2.mTtl                 = 1500;
    service2FullName.Append("%s.%s.local.", service2.mServiceInstance, service2.mServiceType);

    service3.mHostName            = host2.mHostName;
    service3.mServiceInstance     = "srv3";
    service3.mServiceType         = "_srv._udp";
    service3.mSubTypeLabels       = kSubTypes;
    service3.mSubTypeLabelsLength = 1;
    service3.mTxtData             = kTxtData2;
    service3.mTxtDataLength       = sizeof(kTxtData2);
    service3.mPort                = 3333;
    service3.mPriority            = 3;
    service3.mWeight              = 3;
    service3.mTtl                 = 1500;
    service3FullName.Append("%s.%s.local.", service3.mServiceInstance, service3.mServiceType);

    key1.mName          = host2.mHostName;
    key1.mServiceType   = nullptr;
    key1.mKeyData       = kKey1;
    key1.mKeyDataLength = sizeof(kKey1);
    key1.mTtl           = 8000;

    key2.mName          = service3.mServiceInstance;
    key2.mServiceType   = service3.mServiceType;
    key2.mKeyData       = kKey1;
    key2.mKeyDataLength = sizeof(kKey1);
    key2.mTtl           = 8000;

    Log("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -");
    Log("Register 2 hosts and 3 services and 2 keys");

    sDnsMessages.Clear();

    for (RegCallback &regCallbck : sRegCallbacks)
    {
        regCallbck.Reset();
    }

    SuccessOrQuit(mdns->RegisterHost(host1, 0, HandleSuccessCallback));
    SuccessOrQuit(mdns->RegisterHost(host2, 1, HandleSuccessCallback));
    SuccessOrQuit(mdns->RegisterService(service1, 2, HandleSuccessCallback));
    SuccessOrQuit(mdns->RegisterService(service2, 3, HandleSuccessCallback));
    SuccessOrQuit(mdns->RegisterService(service3, 4, HandleSuccessCallback));
    SuccessOrQuit(mdns->RegisterKey(key1, 5, HandleSuccessCallback));
    SuccessOrQuit(mdns->RegisterKey(key2, 6, HandleSuccessCallback));

    Log("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -");
    Log("Validate probes for all entries");

    for (uint8_t probeCount = 0; probeCount < 3; probeCount++)
    {
        sDnsMessages.Clear();
        AdvanceTime(250);

        VerifyOrQuit(!sDnsMessages.IsEmpty());
        dnsMsg = sDnsMessages.GetHead();

        for (uint16_t index = 0; index < 7; index++)
        {
            VerifyOrQuit(!sRegCallbacks[index].mWasCalled);
        }

        dnsMsg->ValidateHeader(kMulticastQuery, /* Q */ 5, /* Ans */ 0, /* Auth */ 13, /* Addnl */ 0);

        dnsMsg->ValidateAsProbeFor(host1, /* aUnicastRequest */ (probeCount == 0));
        dnsMsg->ValidateAsProbeFor(host2, /* aUnicastRequest */ (probeCount == 0));
        dnsMsg->ValidateAsProbeFor(service1, /* aUnicastRequest */ (probeCount == 0));
        dnsMsg->ValidateAsProbeFor(service2, /* aUnicastRequest */ (probeCount == 0));
        dnsMsg->ValidateAsProbeFor(service3, /* aUnicastRequest */ (probeCount == 0));
        dnsMsg->ValidateAsProbeFor(key1, /* aUnicastRequest */ (probeCount == 0));
        dnsMsg->ValidateAsProbeFor(key2, /* aUnicastRequest */ (probeCount == 0));

        VerifyOrQuit(dnsMsg->GetNext() == nullptr);
    }

    Log("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -");
    Log("Validate announcements for all entries");

    for (uint8_t anncCount = 0; anncCount < kNumAnnounces; anncCount++)
    {
        sDnsMessages.Clear();

        AdvanceTime((anncCount == 0) ? 250 : (1U << (anncCount - 1)) * 1000);

        for (uint16_t index = 0; index < 7; index++)
        {
            VerifyOrQuit(sRegCallbacks[index].mWasCalled);
        }

        VerifyOrQuit(!sDnsMessages.IsEmpty());
        dnsMsg = sDnsMessages.GetHead();

        dnsMsg->ValidateHeader(kMulticastResponse, /* Q */ 0, /* Ans */ 21, /* Auth */ 0, /* Addnl */ 5);

        dnsMsg->Validate(host1, kInAnswerSection);
        dnsMsg->Validate(host2, kInAnswerSection);
        dnsMsg->Validate(service1, kInAnswerSection, kCheckSrv | kCheckTxt | kCheckPtr | kCheckServicesPtr);
        dnsMsg->Validate(service2, kInAnswerSection, kCheckSrv | kCheckTxt | kCheckPtr | kCheckServicesPtr);
        dnsMsg->Validate(service2, kInAnswerSection, kCheckSrv | kCheckTxt | kCheckPtr | kCheckServicesPtr);
        dnsMsg->Validate(key1, kInAnswerSection);
        dnsMsg->Validate(key2, kInAnswerSection);

        for (uint16_t index = 0; index < service1.mSubTypeLabelsLength; index++)
        {
            dnsMsg->ValidateSubType(service1.mSubTypeLabels[index], service1);
        }

        for (uint16_t index = 0; index < service3.mSubTypeLabelsLength; index++)
        {
            dnsMsg->ValidateSubType(service3.mSubTypeLabels[index], service3);
        }

        VerifyOrQuit(dnsMsg->GetNext() == nullptr);
    }

    sDnsMessages.Clear();
    AdvanceTime(15000);
    VerifyOrQuit(sDnsMessages.IsEmpty());

    Log("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -");
    Log("Send a PTR query (browse) for `_srv._udp` and validate two answers and additional data");

    AdvanceTime(2000);
    sDnsMessages.Clear();

    SendQuery("_srv._udp.local.", ResourceRecord::kTypePtr);

    AdvanceTime(200);

    dnsMsg = sDnsMessages.GetHead();
    VerifyOrQuit(dnsMsg != nullptr);
    VerifyOrQuit(dnsMsg->GetNext() == nullptr);

    dnsMsg->ValidateHeader(kMulticastResponse, /* Q */ 0, /* Ans */ 2, /* Auth */ 0, /* Addnl */ 9);

    dnsMsg->Validate(service1, kInAnswerSection, kCheckPtr);
    dnsMsg->Validate(service3, kInAnswerSection, kCheckPtr);
    dnsMsg->Validate(service1, kInAdditionalSection, kCheckSrv | kCheckTxt);
    dnsMsg->Validate(service3, kInAdditionalSection, kCheckSrv | kCheckTxt);
    dnsMsg->Validate(host1, kInAdditionalSection);
    dnsMsg->Validate(host2, kInAdditionalSection);

    Log("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -");
    Log("Resend the same query but request a unicast response, validate the response");

    sDnsMessages.Clear();
    SendQuery("_srv._udp.local.", ResourceRecord::kTypePtr, ResourceRecord::kClassInternet | kClassQueryUnicastFlag);

    AdvanceTime(200);

    dnsMsg = sDnsMessages.GetHead();
    VerifyOrQuit(dnsMsg != nullptr);
    VerifyOrQuit(dnsMsg->GetNext() == nullptr);

    dnsMsg->ValidateHeader(kUnicastResponse, /* Q */ 0, /* Ans */ 2, /* Auth */ 0, /* Addnl */ 9);

    dnsMsg->Validate(service1, kInAnswerSection, kCheckPtr);
    dnsMsg->Validate(service3, kInAnswerSection, kCheckPtr);
    dnsMsg->Validate(service1, kInAdditionalSection, kCheckSrv | kCheckTxt);
    dnsMsg->Validate(service3, kInAdditionalSection, kCheckSrv | kCheckTxt);
    dnsMsg->Validate(host1, kInAdditionalSection);
    dnsMsg->Validate(host2, kInAdditionalSection);

    Log("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -");
    Log("Resend the same multicast query and validate that response is not emitted (rate limit)");

    sDnsMessages.Clear();
    SendQuery("_srv._udp.local.", ResourceRecord::kTypePtr);

    AdvanceTime(1000);
    VerifyOrQuit(sDnsMessages.IsEmpty());

    Log("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -");
    Log("Wait for > 1 second and resend the query and validate that now a response is emitted");

    SendQuery("_srv._udp.local.", ResourceRecord::kTypePtr);

    AdvanceTime(200);

    dnsMsg = sDnsMessages.GetHead();
    VerifyOrQuit(dnsMsg != nullptr);
    VerifyOrQuit(dnsMsg->GetNext() == nullptr);

    dnsMsg->ValidateHeader(kMulticastResponse, /* Q */ 0, /* Ans */ 2, /* Auth */ 0, /* Addnl */ 9);

    dnsMsg->Validate(service1, kInAnswerSection, kCheckPtr);
    dnsMsg->Validate(service3, kInAnswerSection, kCheckPtr);
    dnsMsg->Validate(service1, kInAdditionalSection, kCheckSrv | kCheckTxt);
    dnsMsg->Validate(service3, kInAdditionalSection, kCheckSrv | kCheckTxt);
    dnsMsg->Validate(host1, kInAdditionalSection);
    dnsMsg->Validate(host2, kInAdditionalSection);

    Log("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -");
    Log("Browse for sub-type `_s._sub._srv._udp` and validate two answers");

    sDnsMessages.Clear();
    SendQuery("_s._sub._srv._udp.local.", ResourceRecord::kTypePtr);

    AdvanceTime(200);

    dnsMsg = sDnsMessages.GetHead();
    VerifyOrQuit(dnsMsg != nullptr);
    VerifyOrQuit(dnsMsg->GetNext() == nullptr);

    dnsMsg->ValidateHeader(kMulticastResponse, /* Q */ 0, /* Ans */ 2, /* Auth */ 0, /* Addnl */ 9);

    dnsMsg->ValidateSubType("_s", service1);
    dnsMsg->ValidateSubType("_s", service3);
    dnsMsg->Validate(service1, kInAdditionalSection, kCheckSrv | kCheckTxt);
    dnsMsg->Validate(service3, kInAdditionalSection, kCheckSrv | kCheckTxt);
    dnsMsg->Validate(host1, kInAdditionalSection);
    dnsMsg->Validate(host2, kInAdditionalSection);

    // Send same query again and make sure it is ignored (rate limit).

    sDnsMessages.Clear();
    SendQuery("_s._sub._srv._udp.local.", ResourceRecord::kTypePtr);

    AdvanceTime(1000);
    VerifyOrQuit(sDnsMessages.IsEmpty());

    Log("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -");
    Log("Validate that query with `ANY class` instead of `IN class` is responded");

    AdvanceTime(2000);

    sDnsMessages.Clear();
    SendQuery("_r._sub._srv._udp.local.", ResourceRecord::kTypePtr, ResourceRecord::kClassAny);

    AdvanceTime(200);

    dnsMsg = sDnsMessages.GetHead();
    VerifyOrQuit(dnsMsg != nullptr);
    VerifyOrQuit(dnsMsg->GetNext() == nullptr);

    dnsMsg->ValidateHeader(kMulticastResponse, /* Q */ 0, /* Ans */ 1, /* Auth */ 0, /* Addnl */ 5);
    dnsMsg->ValidateSubType("_r", service1);
    dnsMsg->Validate(service1, kInAdditionalSection, kCheckSrv | kCheckTxt);
    dnsMsg->Validate(host1, kInAdditionalSection);

    Log("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -");
    Log("Validate that query with other `class` is ignored");

    AdvanceTime(2000);

    sDnsMessages.Clear();
    SendQuery("_r._sub._srv._udp.local.", ResourceRecord::kTypePtr, ResourceRecord::kClassNone);

    AdvanceTime(2000);
    VerifyOrQuit(sDnsMessages.IsEmpty());

    Log("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -");
    Log("Validate that query for non-registered name is ignored");

    sDnsMessages.Clear();
    SendQuery("_u._sub._srv._udp.local.", ResourceRecord::kTypeAny);
    SendQuery("host3.local.", ResourceRecord::kTypeAny);

    AdvanceTime(2000);
    VerifyOrQuit(sDnsMessages.IsEmpty());

    Log("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -");
    Log("Query for SRV for `srv1._srv._udp` and validate answer and additional data");

    sDnsMessages.Clear();

    SendQuery("srv1._srv._udp.local.", ResourceRecord::kTypeSrv);

    AdvanceTime(200);

    dnsMsg = sDnsMessages.GetHead();
    VerifyOrQuit(dnsMsg != nullptr);
    VerifyOrQuit(dnsMsg->GetNext() == nullptr);

    dnsMsg->ValidateHeader(kMulticastResponse, /* Q */ 0, /* Ans */ 1, /* Auth */ 0, /* Addnl */ 4);

    dnsMsg->Validate(service1, kInAnswerSection, kCheckSrv);
    dnsMsg->Validate(host1, kInAdditionalSection);

    //--- --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- ---
    // Query with multiple questions

    Log("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -");
    Log("Send a query with two questions (SRV for service1 and AAAA for host1). Validate response");

    AdvanceTime(2000);

    sDnsMessages.Clear();
    SendQueryForTwo("srv1._srv._udp.local.", ResourceRecord::kTypeSrv, "host1.local.", ResourceRecord::kTypeAaaa);

    AdvanceTime(200);

    dnsMsg = sDnsMessages.GetHead();
    VerifyOrQuit(dnsMsg != nullptr);
    VerifyOrQuit(dnsMsg->GetNext() == nullptr);

    // Since AAAA record are already present in Answer they should not be appended
    // in Additional anymore (for the SRV query).

    dnsMsg->ValidateHeader(kMulticastResponse, /* Q */ 0, /* Ans */ 4, /* Auth */ 0, /* Addnl */ 2);

    dnsMsg->Validate(service1, kInAnswerSection, kCheckSrv);
    dnsMsg->Validate(host1, kInAnswerSection);

    //--- --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- ---
    // Known-answer suppression

    Log("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -");
    Log("Send a PTR query for `_srv._udp` and include `srv1` as known-answer and validate response");

    knownAnswers[0].mPtrAnswer = "srv1._srv._udp.local.";
    knownAnswers[0].mTtl       = 1500;

    AdvanceTime(1000);

    sDnsMessages.Clear();
    SendPtrQueryWithKnownAnswers("_srv._udp.local.", knownAnswers, 1);

    AdvanceTime(200);

    dnsMsg = sDnsMessages.GetHead();
    VerifyOrQuit(dnsMsg != nullptr);
    VerifyOrQuit(dnsMsg->GetNext() == nullptr);

    // Response should include `service3` only

    dnsMsg->ValidateHeader(kMulticastResponse, /* Q */ 0, /* Ans */ 1, /* Auth */ 0, /* Addnl */ 4);
    dnsMsg->Validate(service3, kInAnswerSection, kCheckPtr);
    dnsMsg->Validate(service3, kInAdditionalSection, kCheckSrv | kCheckTxt);
    dnsMsg->Validate(host2, kInAdditionalSection);

    Log("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -");
    Log("Send a PTR query again with both services as known-answer, validate no response is emitted");

    knownAnswers[1].mPtrAnswer = "srv3._srv._udp.local.";
    knownAnswers[1].mTtl       = 1500;

    AdvanceTime(1000);

    sDnsMessages.Clear();
    SendPtrQueryWithKnownAnswers("_srv._udp.local.", knownAnswers, 2);

    AdvanceTime(2000);
    VerifyOrQuit(sDnsMessages.IsEmpty());

    Log("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -");
    Log("Send a PTR query for `_srv._udp` and include `srv1` as known-answer and validate response");

    knownAnswers[0].mPtrAnswer = "srv1._srv._udp.local.";
    knownAnswers[0].mTtl       = 1500;

    AdvanceTime(1000);

    sDnsMessages.Clear();
    SendPtrQueryWithKnownAnswers("_srv._udp.local.", knownAnswers, 1);

    AdvanceTime(200);

    dnsMsg = sDnsMessages.GetHead();
    VerifyOrQuit(dnsMsg != nullptr);
    VerifyOrQuit(dnsMsg->GetNext() == nullptr);

    // Response should include `service3` only

    dnsMsg->ValidateHeader(kMulticastResponse, /* Q */ 0, /* Ans */ 1, /* Auth */ 0, /* Addnl */ 4);
    dnsMsg->Validate(service3, kInAnswerSection, kCheckPtr);
    dnsMsg->Validate(service3, kInAdditionalSection, kCheckSrv | kCheckTxt);
    dnsMsg->Validate(host2, kInAdditionalSection);

    Log("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -");
    Log("Change the TTL for known-answer to less than half of record TTL and validate response");

    knownAnswers[1].mTtl = 1500 / 2 - 1;

    AdvanceTime(1000);

    sDnsMessages.Clear();
    SendPtrQueryWithKnownAnswers("_srv._udp.local.", knownAnswers, 2);

    AdvanceTime(200);

    dnsMsg = sDnsMessages.GetHead();
    VerifyOrQuit(dnsMsg != nullptr);
    VerifyOrQuit(dnsMsg->GetNext() == nullptr);

    // Response should include `service3` only since anwer TTL
    // is less than half of registered TTL

    dnsMsg->ValidateHeader(kMulticastResponse, /* Q */ 0, /* Ans */ 1, /* Auth */ 0, /* Addnl */ 4);
    dnsMsg->Validate(service3, kInAnswerSection, kCheckPtr);
    dnsMsg->Validate(service3, kInAdditionalSection, kCheckSrv | kCheckTxt);
    dnsMsg->Validate(host2, kInAdditionalSection);

    //--- --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- ---
    // Query during Goodbye announcements

    Log("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -");
    Log("Unregister `service1` and wait for its two announcements and validate them");

    sDnsMessages.Clear();
    SuccessOrQuit(mdns->UnregisterService(service1));

    for (uint8_t anncCount = 0; anncCount < kNumAnnounces - 1; anncCount++)
    {
        sDnsMessages.Clear();

        AdvanceTime((anncCount == 0) ? 250 : (1U << (anncCount - 1)) * 1000);

        dnsMsg = sDnsMessages.GetHead();
        VerifyOrQuit(dnsMsg != nullptr);
        VerifyOrQuit(dnsMsg->GetNext() == nullptr);

        dnsMsg->ValidateHeader(kMulticastResponse, /* Q */ 0, /* Ans */ 5, /* Auth */ 0, /* Addnl */ 0);
        dnsMsg->Validate(service1, kInAnswerSection, kCheckSrv | kCheckTxt | kCheckPtr, kGoodBye);

        for (uint16_t index = 0; index < service1.mSubTypeLabelsLength; index++)
        {
            dnsMsg->ValidateSubType(service1.mSubTypeLabels[index], service1, kGoodBye);
        }
    }

    Log("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -");
    Log("Send a query for removed `service1` before its final announcement, validate no response");

    sDnsMessages.Clear();

    AdvanceTime(1100);
    SendQuery("srv1._srv._udp.local.", ResourceRecord::kTypeSrv);

    AdvanceTime(200);

    VerifyOrQuit(sDnsMessages.IsEmpty());

    // Wait for final announcement and validate it

    AdvanceTime(2000);

    dnsMsg = sDnsMessages.GetHead();
    VerifyOrQuit(dnsMsg != nullptr);
    VerifyOrQuit(dnsMsg->GetNext() == nullptr);

    dnsMsg->ValidateHeader(kMulticastResponse, /* Q */ 0, /* Ans */ 5, /* Auth */ 0, /* Addnl */ 0);
    dnsMsg->Validate(service1, kInAnswerSection, kCheckSrv | kCheckTxt | kCheckPtr, kGoodBye);

    for (uint16_t index = 0; index < service1.mSubTypeLabelsLength; index++)
    {
        dnsMsg->ValidateSubType(service1.mSubTypeLabels[index], service1, kGoodBye);
    }

    SuccessOrQuit(mdns->SetEnabled(false, kInfraIfIndex));
    VerifyOrQuit(sHeapAllocatedPtrs.GetLength() <= heapAllocations);

    Log("End of test");

    testFreeInstance(sInstance);
}

//----------------------------------------------------------------------------------------------------------------------

void TestMultiPacket(void)
{
    static const char *const kSubTypes[] = {"_s1", "_r2", "vxy"};

    Core             *mdns = InitTest();
    Core::Service     service;
    const DnsMessage *dnsMsg;
    uint16_t          heapAllocations;
    DnsNameString     fullServiceName;
    DnsNameString     fullServiceType;
    KnownAnswer       knownAnswers[2];

    Log("-------------------------------------------------------------------------------------------");
    Log("TestMultiPacket");

    AdvanceTime(1);

    heapAllocations = sHeapAllocatedPtrs.GetLength();
    SuccessOrQuit(mdns->SetEnabled(true, kInfraIfIndex));

    service.mHostName            = "myhost";
    service.mServiceInstance     = "mysrv";
    service.mServiceType         = "_tst._udp";
    service.mSubTypeLabels       = kSubTypes;
    service.mSubTypeLabelsLength = 3;
    service.mTxtData             = kTxtData1;
    service.mTxtDataLength       = sizeof(kTxtData1);
    service.mPort                = 2222;
    service.mPriority            = 3;
    service.mWeight              = 4;
    service.mTtl                 = 2000;

    fullServiceName.Append("%s.%s.local.", service.mServiceInstance, service.mServiceType);
    fullServiceType.Append("%s.local.", service.mServiceType);

    Log("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -");
    Log("Register a `ServiceEntry` with sub-types, check probes and announcements");

    sDnsMessages.Clear();

    sRegCallbacks[0].Reset();
    SuccessOrQuit(mdns->RegisterService(service, 0, HandleSuccessCallback));

    for (uint8_t probeCount = 0; probeCount < 3; probeCount++)
    {
        sDnsMessages.Clear();

        VerifyOrQuit(!sRegCallbacks[0].mWasCalled);
        AdvanceTime(250);

        VerifyOrQuit(!sDnsMessages.IsEmpty());
        dnsMsg = sDnsMessages.GetHead();
        dnsMsg->ValidateHeader(kMulticastQuery, /* Q */ 1, /* Ans */ 0, /* Auth */ 2, /* Addnl */ 0);
        dnsMsg->ValidateAsProbeFor(service, /* aUnicastRequest */ (probeCount == 0));
        VerifyOrQuit(dnsMsg->GetNext() == nullptr);
    }

    for (uint8_t anncCount = 0; anncCount < kNumAnnounces; anncCount++)
    {
        sDnsMessages.Clear();

        AdvanceTime((anncCount == 0) ? 250 : (1U << (anncCount - 1)) * 1000);
        VerifyOrQuit(sRegCallbacks[0].mWasCalled);

        VerifyOrQuit(!sDnsMessages.IsEmpty());
        dnsMsg = sDnsMessages.GetHead();
        dnsMsg->ValidateHeader(kMulticastResponse, /* Q */ 0, /* Ans */ 7, /* Auth */ 0, /* Addnl */ 1);
        dnsMsg->Validate(service, kInAnswerSection, kCheckSrv | kCheckTxt | kCheckPtr | kCheckServicesPtr);

        for (uint16_t index = 0; index < service.mSubTypeLabelsLength; index++)
        {
            dnsMsg->ValidateSubType(service.mSubTypeLabels[index], service);
        }

        VerifyOrQuit(dnsMsg->GetNext() == nullptr);
    }

    Log("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -");
    Log("Send a query for PTR record for service type and validate the response");

    AdvanceTime(2000);

    sDnsMessages.Clear();
    SendQuery(fullServiceType.AsCString(), ResourceRecord::kTypePtr);

    AdvanceTime(1000);

    dnsMsg = sDnsMessages.GetHead();
    VerifyOrQuit(dnsMsg != nullptr);
    dnsMsg->ValidateHeader(kMulticastResponse, /* Q */ 0, /* Ans */ 1, /* Auth */ 0, /* Addnl */ 2);
    dnsMsg->Validate(service, kInAnswerSection, kCheckPtr);
    dnsMsg->Validate(service, kInAdditionalSection, kCheckSrv | kCheckTxt);

    Log("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -");
    Log("Send a PTR query again but mark it as truncated");

    AdvanceTime(2000);

    sDnsMessages.Clear();
    SendQuery(fullServiceType.AsCString(), ResourceRecord::kTypePtr, ResourceRecord::kClassInternet,
              /* aTruncated */ true);

    Log("Since message is marked as `truncated`, mDNS should wait at least 400 msec");

    AdvanceTime(400);
    VerifyOrQuit(sDnsMessages.IsEmpty());

    AdvanceTime(2000);
    dnsMsg = sDnsMessages.GetHead();
    VerifyOrQuit(dnsMsg != nullptr);
    dnsMsg->ValidateHeader(kMulticastResponse, /* Q */ 0, /* Ans */ 1, /* Auth */ 0, /* Addnl */ 2);
    dnsMsg->Validate(service, kInAnswerSection, kCheckPtr);
    dnsMsg->Validate(service, kInAdditionalSection, kCheckSrv | kCheckTxt);

    Log("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -");
    Log("Send a PTR query again as truncated followed-up by a non-matching answer");

    AdvanceTime(2000);

    sDnsMessages.Clear();
    SendQuery(fullServiceType.AsCString(), ResourceRecord::kTypePtr, ResourceRecord::kClassInternet,
              /* aTruncated */ true);
    AdvanceTime(10);

    knownAnswers[0].mPtrAnswer = "other._tst._udp.local.";
    knownAnswers[0].mTtl       = 1500;

    SendEmtryPtrQueryWithKnownAnswers(fullServiceType.AsCString(), knownAnswers, 1);

    AdvanceTime(1000);
    dnsMsg = sDnsMessages.GetHead();
    VerifyOrQuit(dnsMsg != nullptr);
    dnsMsg->ValidateHeader(kMulticastResponse, /* Q */ 0, /* Ans */ 1, /* Auth */ 0, /* Addnl */ 2);
    dnsMsg->Validate(service, kInAnswerSection, kCheckPtr);
    dnsMsg->Validate(service, kInAdditionalSection, kCheckSrv | kCheckTxt);

    Log("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -");
    Log("Send a PTR query again as truncated now followed-up by matching known-answer");

    AdvanceTime(2000);

    sDnsMessages.Clear();
    SendQuery(fullServiceType.AsCString(), ResourceRecord::kTypePtr, ResourceRecord::kClassInternet,
              /* aTruncated */ true);
    AdvanceTime(10);

    knownAnswers[1].mPtrAnswer = "mysrv._tst._udp.local.";
    knownAnswers[1].mTtl       = 1500;

    SendEmtryPtrQueryWithKnownAnswers(fullServiceType.AsCString(), knownAnswers, 2);

    Log("We expect no response since the followed-up message contains a matching known-answer");
    AdvanceTime(5000);
    VerifyOrQuit(sDnsMessages.IsEmpty());

    Log("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -");
    Log("Send a truncated query for PTR record for `services._dns-sd`");

    AdvanceTime(2000);

    sDnsMessages.Clear();
    SendQuery("_services._dns-sd._udp.local.", ResourceRecord::kTypePtr, ResourceRecord::kClassInternet,
              /* aTruncated */ true);

    Log("Response should be sent after longer wait time");
    AdvanceTime(1000);

    dnsMsg = sDnsMessages.GetHead();
    VerifyOrQuit(dnsMsg != nullptr);
    dnsMsg->ValidateHeader(kMulticastResponse, /* Q */ 0, /* Ans */ 1, /* Auth */ 0, /* Addnl */ 0);
    dnsMsg->Validate(service, kInAnswerSection, kCheckServicesPtr);

    Log("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -");
    Log("Send a truncated query for PTR record for `services._dns-sd` folloed by known-aswer");

    AdvanceTime(2000);

    sDnsMessages.Clear();
    SendQuery("_services._dns-sd._udp.local.", ResourceRecord::kTypePtr, ResourceRecord::kClassInternet,
              /* aTruncated */ true);

    AdvanceTime(20);
    knownAnswers[0].mPtrAnswer = "_other._udp.local.";
    knownAnswers[0].mTtl       = 4500;

    SendEmtryPtrQueryWithKnownAnswers("_services._dns-sd._udp.local.", knownAnswers, 1);

    Log("Response should be sent again due to answer not matching");
    AdvanceTime(1000);

    dnsMsg = sDnsMessages.GetHead();
    VerifyOrQuit(dnsMsg != nullptr);
    dnsMsg->ValidateHeader(kMulticastResponse, /* Q */ 0, /* Ans */ 1, /* Auth */ 0, /* Addnl */ 0);
    dnsMsg->Validate(service, kInAnswerSection, kCheckServicesPtr);

    Log("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -");
    Log("Send the same truncated query again but follow-up with a matching known-answer message");

    AdvanceTime(2000);

    sDnsMessages.Clear();
    SendQuery("_services._dns-sd._udp.local.", ResourceRecord::kTypePtr, ResourceRecord::kClassInternet,
              /* aTruncated */ true);

    AdvanceTime(20);
    knownAnswers[1].mPtrAnswer = "_tst._udp.local.";
    knownAnswers[1].mTtl       = 4500;

    SendEmtryPtrQueryWithKnownAnswers("_services._dns-sd._udp.local.", knownAnswers, 2);

    Log("We expect no response since the followed-up message contains a matching known-answer");
    AdvanceTime(5000);
    VerifyOrQuit(sDnsMessages.IsEmpty());

    SuccessOrQuit(mdns->SetEnabled(false, kInfraIfIndex));
    VerifyOrQuit(sHeapAllocatedPtrs.GetLength() <= heapAllocations);

    Log("End of test");

    testFreeInstance(sInstance);
}

//---------------------------------------------------------------------------------------------------------------------

void TestQuestionUnicastDisallowed(void)
{
    Core             *mdns = InitTest();
    Core::Host        host;
    Ip6::Address      hostAddresses[1];
    const DnsMessage *dnsMsg;
    uint16_t          heapAllocations;
    DnsNameString     hostFullName;

    Log("-------------------------------------------------------------------------------------------");
    Log("TestQuestionUnicastDisallowed");

    AdvanceTime(1);

    heapAllocations = sHeapAllocatedPtrs.GetLength();
    SuccessOrQuit(mdns->SetEnabled(true, kInfraIfIndex));

    SuccessOrQuit(hostAddresses[0].FromString("fd00::1234"));

    host.mHostName        = "myhost";
    host.mAddresses       = hostAddresses;
    host.mAddressesLength = 1;
    host.mTtl             = 1500;

    mdns->SetQuestionUnicastAllowed(false);
    VerifyOrQuit(!mdns->IsQuestionUnicastAllowed());

    Log("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -");
    Log("Register a `HostEntry`, check probes and announcements");

    sDnsMessages.Clear();

    sRegCallbacks[0].Reset();
    SuccessOrQuit(mdns->RegisterHost(host, 0, HandleSuccessCallback));

    for (uint8_t probeCount = 0; probeCount < 3; probeCount++)
    {
        sDnsMessages.Clear();

        VerifyOrQuit(!sRegCallbacks[0].mWasCalled);
        AdvanceTime(250);

        VerifyOrQuit(!sDnsMessages.IsEmpty());
        dnsMsg = sDnsMessages.GetHead();
        dnsMsg->ValidateHeader(kMulticastQuery, /* Q */ 1, /* Ans */ 0, /* Auth */ 1, /* Addnl */ 0);
        dnsMsg->ValidateAsProbeFor(host, /* aUnicastRequest */ false);
        VerifyOrQuit(dnsMsg->GetNext() == nullptr);
    }

    for (uint8_t anncCount = 0; anncCount < kNumAnnounces; anncCount++)
    {
        sDnsMessages.Clear();

        AdvanceTime((anncCount == 0) ? 250 : (1U << (anncCount - 1)) * 1000);
        VerifyOrQuit(sRegCallbacks[0].mWasCalled);

        VerifyOrQuit(!sDnsMessages.IsEmpty());
        dnsMsg = sDnsMessages.GetHead();
        dnsMsg->ValidateHeader(kMulticastResponse, /* Q */ 0, /* Ans */ 1, /* Auth */ 0, /* Addnl */ 1);
        dnsMsg->Validate(host, kInAnswerSection);
        VerifyOrQuit(dnsMsg->GetNext() == nullptr);
    }

    sDnsMessages.Clear();
    AdvanceTime(15000);
    VerifyOrQuit(sDnsMessages.IsEmpty());

    SuccessOrQuit(mdns->SetEnabled(false, kInfraIfIndex));
    VerifyOrQuit(sHeapAllocatedPtrs.GetLength() <= heapAllocations);

    Log("End of test");

    testFreeInstance(sInstance);
}

//---------------------------------------------------------------------------------------------------------------------

void TestTxMessageSizeLimit(void)
{
    Core             *mdns = InitTest();
    Core::Host        host;
    Core::Service     service;
    Core::Key         hostKey;
    Core::Key         serviceKey;
    Ip6::Address      hostAddresses[3];
    uint8_t           keyData[300];
    const DnsMessage *dnsMsg;
    uint16_t          heapAllocations;
    DnsNameString     hostFullName;
    DnsNameString     serviceFullName;

    memset(keyData, 1, sizeof(keyData));

    Log("-------------------------------------------------------------------------------------------");
    Log("TestTxMessageSizeLimit");

    AdvanceTime(1);

    heapAllocations = sHeapAllocatedPtrs.GetLength();
    SuccessOrQuit(mdns->SetEnabled(true, kInfraIfIndex));

    SuccessOrQuit(hostAddresses[0].FromString("fd00::1:aaaa"));
    SuccessOrQuit(hostAddresses[1].FromString("fd00::1:bbbb"));
    SuccessOrQuit(hostAddresses[2].FromString("fd00::1:cccc"));
    host.mHostName        = "myhost";
    host.mAddresses       = hostAddresses;
    host.mAddressesLength = 3;
    host.mTtl             = 1500;
    hostFullName.Append("%s.local.", host.mHostName);

    service.mHostName            = host.mHostName;
    service.mServiceInstance     = "mysrv";
    service.mServiceType         = "_srv._udp";
    service.mSubTypeLabels       = nullptr;
    service.mSubTypeLabelsLength = 0;
    service.mTxtData             = kTxtData1;
    service.mTxtDataLength       = sizeof(kTxtData1);
    service.mPort                = 1111;
    service.mPriority            = 0;
    service.mWeight              = 0;
    service.mTtl                 = 1500;
    serviceFullName.Append("%s.%s.local.", service.mServiceInstance, service.mServiceType);

    hostKey.mName          = host.mHostName;
    hostKey.mServiceType   = nullptr;
    hostKey.mKeyData       = keyData;
    hostKey.mKeyDataLength = 300;
    hostKey.mTtl           = 8000;

    serviceKey.mName          = service.mServiceInstance;
    serviceKey.mServiceType   = service.mServiceType;
    serviceKey.mKeyData       = keyData;
    serviceKey.mKeyDataLength = 300;
    serviceKey.mTtl           = 8000;

    Log("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -");
    Log("Set `MaxMessageSize` to 340 and use large key record data to trigger size limit behavior");

    mdns->SetMaxMessageSize(340);

    Log("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -");
    Log("Register host and service and keys for each");

    sDnsMessages.Clear();

    for (RegCallback &regCallbck : sRegCallbacks)
    {
        regCallbck.Reset();
    }

    SuccessOrQuit(mdns->RegisterHost(host, 0, HandleSuccessCallback));
    SuccessOrQuit(mdns->RegisterService(service, 1, HandleSuccessCallback));
    SuccessOrQuit(mdns->RegisterKey(hostKey, 2, HandleSuccessCallback));
    SuccessOrQuit(mdns->RegisterKey(serviceKey, 3, HandleSuccessCallback));

    Log("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -");
    Log("Validate probes for all entries");
    Log("Probes for host and service should be broken into separate message due to size limit");

    for (uint8_t probeCount = 0; probeCount < 3; probeCount++)
    {
        sDnsMessages.Clear();
        AdvanceTime(250);

        VerifyOrQuit(!sDnsMessages.IsEmpty());
        dnsMsg = sDnsMessages.GetHead();

        for (uint16_t index = 0; index < 4; index++)
        {
            VerifyOrQuit(!sRegCallbacks[index].mWasCalled);
        }

        dnsMsg->ValidateHeader(kMulticastQuery, /* Q */ 1, /* Ans */ 0, /* Auth */ 4, /* Addnl */ 0);
        dnsMsg->ValidateAsProbeFor(host, /* aUnicastRequest */ (probeCount == 0));
        dnsMsg->ValidateAsProbeFor(hostKey, /* aUnicastRequest */ (probeCount == 0));

        dnsMsg = dnsMsg->GetNext();
        VerifyOrQuit(dnsMsg != nullptr);

        dnsMsg->ValidateHeader(kMulticastQuery, /* Q */ 1, /* Ans */ 0, /* Auth */ 3, /* Addnl */ 0);
        dnsMsg->ValidateAsProbeFor(service, /* aUnicastRequest */ (probeCount == 0));
        dnsMsg->ValidateAsProbeFor(serviceKey, /* aUnicastRequest */ (probeCount == 0));

        VerifyOrQuit(dnsMsg->GetNext() == nullptr);
    }

    Log("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -");
    Log("Validate announcements for all entries");
    Log("Announces should also be broken into separate message due to size limit");

    for (uint8_t anncCount = 0; anncCount < kNumAnnounces; anncCount++)
    {
        sDnsMessages.Clear();

        AdvanceTime((anncCount == 0) ? 250 : (1U << (anncCount - 1)) * 1000);

        for (uint16_t index = 0; index < 4; index++)
        {
            VerifyOrQuit(sRegCallbacks[index].mWasCalled);
        }

        VerifyOrQuit(!sDnsMessages.IsEmpty());
        dnsMsg = sDnsMessages.GetHead();

        dnsMsg->ValidateHeader(kMulticastResponse, /* Q */ 0, /* Ans */ 4, /* Auth */ 0, /* Addnl */ 1);
        dnsMsg->Validate(host, kInAnswerSection);
        dnsMsg->Validate(hostKey, kInAnswerSection);

        dnsMsg = dnsMsg->GetNext();
        VerifyOrQuit(dnsMsg != nullptr);

        dnsMsg->ValidateHeader(kMulticastResponse, /* Q */ 0, /* Ans */ 4, /* Auth */ 0, /* Addnl */ 4);
        dnsMsg->Validate(service, kInAnswerSection, kCheckSrv | kCheckTxt | kCheckPtr);
        dnsMsg->Validate(serviceKey, kInAnswerSection);

        dnsMsg = dnsMsg->GetNext();
        VerifyOrQuit(dnsMsg != nullptr);

        dnsMsg->ValidateHeader(kMulticastResponse, /* Q */ 0, /* Ans */ 1, /* Auth */ 0, /* Addnl */ 0);
        dnsMsg->Validate(service, kInAnswerSection, kCheckServicesPtr);

        VerifyOrQuit(dnsMsg->GetNext() == nullptr);
    }

    SuccessOrQuit(mdns->SetEnabled(false, kInfraIfIndex));
    VerifyOrQuit(sHeapAllocatedPtrs.GetLength() <= heapAllocations);

    Log("End of test");

    testFreeInstance(sInstance);
}

//---------------------------------------------------------------------------------------------------------------------

void TestHostConflict(void)
{
    Core             *mdns = InitTest();
    Core::Host        host;
    Ip6::Address      hostAddresses[2];
    const DnsMessage *dnsMsg;
    uint16_t          heapAllocations;
    DnsNameString     hostFullName;

    Log("-------------------------------------------------------------------------------------------");
    Log("TestHostConflict");

    AdvanceTime(1);

    heapAllocations = sHeapAllocatedPtrs.GetLength();
    SuccessOrQuit(mdns->SetEnabled(true, kInfraIfIndex));

    SuccessOrQuit(hostAddresses[0].FromString("fd00::1"));
    SuccessOrQuit(hostAddresses[1].FromString("fd00::2"));

    host.mHostName        = "myhost";
    host.mAddresses       = hostAddresses;
    host.mAddressesLength = 2;
    host.mTtl             = 1500;

    hostFullName.Append("%s.local.", host.mHostName);

    // Run the test twice, first run send response with record in Answer section,
    // section run in Additional Data section.

    sConflictCallback.Reset();
    mdns->SetConflictCallback(HandleConflict);

    for (uint8_t iter = 0; iter < 2; iter++)
    {
        Log("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -");
        Log("Register a `HostEntry`, wait for first probe");

        sDnsMessages.Clear();

        sRegCallbacks[0].Reset();
        SuccessOrQuit(mdns->RegisterHost(host, 0, HandleCallback));

        VerifyOrQuit(!sRegCallbacks[0].mWasCalled);
        AdvanceTime(250);

        VerifyOrQuit(!sDnsMessages.IsEmpty());
        dnsMsg = sDnsMessages.GetHead();
        dnsMsg->ValidateHeader(kMulticastQuery, /* Q */ 1, /* Ans */ 0, /* Auth */ 2, /* Addnl */ 0);
        dnsMsg->ValidateAsProbeFor(host, /* aUnicastRequest */ true);
        VerifyOrQuit(dnsMsg->GetNext() == nullptr);

        Log("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -");
        Log("Send a response claiming the name with record in %s section", (iter == 0) ? "answer" : "additional");

        SendResponseWithEmptyKey(hostFullName.AsCString(), (iter == 0) ? kInAnswerSection : kInAdditionalSection);
        AdvanceTime(1);

        VerifyOrQuit(sRegCallbacks[0].mWasCalled);
        VerifyOrQuit(sRegCallbacks[0].mError == kErrorDuplicated);

        VerifyOrQuit(!sConflictCallback.mWasCalled);

        sDnsMessages.Clear();

        SuccessOrQuit(mdns->UnregisterHost(host));

        AdvanceTime(15000);
        VerifyOrQuit(sDnsMessages.IsEmpty());
    }

    Log("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -");
    Log("Register a `HostEntry` and respond to probe to trigger conflict");

    sRegCallbacks[0].Reset();
    SuccessOrQuit(mdns->RegisterHost(host, 0, HandleCallback));

    VerifyOrQuit(!sRegCallbacks[0].mWasCalled);

    SendResponseWithEmptyKey(hostFullName.AsCString(), kInAnswerSection);
    AdvanceTime(1);

    VerifyOrQuit(sRegCallbacks[0].mWasCalled);
    VerifyOrQuit(sRegCallbacks[0].mError == kErrorDuplicated);
    VerifyOrQuit(!sConflictCallback.mWasCalled);

    Log("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -");
    Log("Register the conflicted `HostEntry` again, and make sure no probes are sent");

    sRegCallbacks[1].Reset();
    sConflictCallback.Reset();
    sDnsMessages.Clear();

    SuccessOrQuit(mdns->RegisterHost(host, 1, HandleCallback));
    AdvanceTime(5000);

    VerifyOrQuit(sRegCallbacks[1].mWasCalled);
    VerifyOrQuit(sRegCallbacks[1].mError == kErrorDuplicated);
    VerifyOrQuit(!sConflictCallback.mWasCalled);

    Log("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -");
    Log("Unregister the conflicted host and register it again immediately, make sure we see probes");

    SuccessOrQuit(mdns->UnregisterHost(host));

    sConflictCallback.Reset();
    sRegCallbacks[0].Reset();
    SuccessOrQuit(mdns->RegisterHost(host, 0, HandleSuccessCallback));

    for (uint8_t probeCount = 0; probeCount < 3; probeCount++)
    {
        sDnsMessages.Clear();

        VerifyOrQuit(!sRegCallbacks[0].mWasCalled);
        AdvanceTime(250);

        VerifyOrQuit(!sDnsMessages.IsEmpty());
        dnsMsg = sDnsMessages.GetHead();
        dnsMsg->ValidateHeader(kMulticastQuery, /* Q */ 1, /* Ans */ 0, /* Auth */ 2, /* Addnl */ 0);
        dnsMsg->ValidateAsProbeFor(host, /* aUnicastRequest */ (probeCount == 0));
        VerifyOrQuit(dnsMsg->GetNext() == nullptr);
    }

    for (uint8_t anncCount = 0; anncCount < kNumAnnounces; anncCount++)
    {
        sDnsMessages.Clear();

        AdvanceTime((anncCount == 0) ? 250 : (1U << (anncCount - 1)) * 1000);
        VerifyOrQuit(sRegCallbacks[0].mWasCalled);

        VerifyOrQuit(!sDnsMessages.IsEmpty());
        dnsMsg = sDnsMessages.GetHead();
        dnsMsg->ValidateHeader(kMulticastResponse, /* Q */ 0, /* Ans */ 2, /* Auth */ 0, /* Addnl */ 1);
        dnsMsg->Validate(host, kInAnswerSection);
        VerifyOrQuit(dnsMsg->GetNext() == nullptr);
    }

    VerifyOrQuit(!sConflictCallback.mWasCalled);

    Log("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -");
    Log("Send a response for host name and validate that conflict is detected and callback is called");

    SendResponseWithEmptyKey(hostFullName.AsCString(), kInAnswerSection);
    AdvanceTime(1);

    VerifyOrQuit(sConflictCallback.mWasCalled);
    VerifyOrQuit(StringMatch(sConflictCallback.mName.AsCString(), host.mHostName, kStringCaseInsensitiveMatch));
    VerifyOrQuit(!sConflictCallback.mHasServiceType);

    SuccessOrQuit(mdns->SetEnabled(false, kInfraIfIndex));
    VerifyOrQuit(sHeapAllocatedPtrs.GetLength() <= heapAllocations);

    Log("End of test");

    testFreeInstance(sInstance);
}

//---------------------------------------------------------------------------------------------------------------------

void TestServiceConflict(void)
{
    Core             *mdns = InitTest();
    Core::Service     service;
    const DnsMessage *dnsMsg;
    uint16_t          heapAllocations;
    DnsNameString     fullServiceName;

    Log("-------------------------------------------------------------------------------------------");
    Log("TestServiceConflict");

    service.mHostName            = "myhost";
    service.mServiceInstance     = "myservice";
    service.mServiceType         = "_srv._udp";
    service.mSubTypeLabels       = nullptr;
    service.mSubTypeLabelsLength = 0;
    service.mTxtData             = kTxtData1;
    service.mTxtDataLength       = sizeof(kTxtData1);
    service.mPort                = 1234;
    service.mPriority            = 1;
    service.mWeight              = 2;
    service.mTtl                 = 1000;

    fullServiceName.Append("%s.%s.local.", service.mServiceInstance, service.mServiceType);

    AdvanceTime(1);

    heapAllocations = sHeapAllocatedPtrs.GetLength();
    SuccessOrQuit(mdns->SetEnabled(true, kInfraIfIndex));

    // Run the test twice, first run send response with record in Answer section,
    // section run in Additional Data section.

    sConflictCallback.Reset();
    mdns->SetConflictCallback(HandleConflict);

    for (uint8_t iter = 0; iter < 2; iter++)
    {
        Log("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -");
        Log("Register a `ServiceEntry`, wait for first probe");

        sDnsMessages.Clear();

        sRegCallbacks[0].Reset();
        SuccessOrQuit(mdns->RegisterService(service, 0, HandleCallback));

        VerifyOrQuit(!sRegCallbacks[0].mWasCalled);
        AdvanceTime(250);

        VerifyOrQuit(!sDnsMessages.IsEmpty());
        dnsMsg = sDnsMessages.GetHead();
        dnsMsg->ValidateHeader(kMulticastQuery, /* Q */ 1, /* Ans */ 0, /* Auth */ 2, /* Addnl */ 0);
        dnsMsg->ValidateAsProbeFor(service, /* aUnicastRequest */ true);
        VerifyOrQuit(dnsMsg->GetNext() == nullptr);

        Log("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -");
        Log("Send a response claiming the name with record in %s section", (iter == 0) ? "answer" : "additional");

        SendResponseWithEmptyKey(fullServiceName.AsCString(), (iter == 0) ? kInAnswerSection : kInAdditionalSection);
        AdvanceTime(1);

        VerifyOrQuit(sRegCallbacks[0].mWasCalled);
        VerifyOrQuit(sRegCallbacks[0].mError == kErrorDuplicated);

        VerifyOrQuit(!sConflictCallback.mWasCalled);

        sDnsMessages.Clear();

        SuccessOrQuit(mdns->UnregisterService(service));

        AdvanceTime(15000);
        VerifyOrQuit(sDnsMessages.IsEmpty());
    }

    Log("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -");
    Log("Register a `ServiceEntry` and respond to probe to trigger conflict");

    sRegCallbacks[0].Reset();
    SuccessOrQuit(mdns->RegisterService(service, 0, HandleCallback));

    VerifyOrQuit(!sRegCallbacks[0].mWasCalled);

    SendResponseWithEmptyKey(fullServiceName.AsCString(), kInAnswerSection);
    AdvanceTime(1);

    VerifyOrQuit(sRegCallbacks[0].mWasCalled);
    VerifyOrQuit(sRegCallbacks[0].mError == kErrorDuplicated);
    VerifyOrQuit(!sConflictCallback.mWasCalled);

    Log("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -");
    Log("Register the conflicted `ServiceEntry` again, and make sure no probes are sent");

    sRegCallbacks[1].Reset();
    sConflictCallback.Reset();
    sDnsMessages.Clear();

    SuccessOrQuit(mdns->RegisterService(service, 1, HandleCallback));
    AdvanceTime(5000);

    VerifyOrQuit(sRegCallbacks[1].mWasCalled);
    VerifyOrQuit(sRegCallbacks[1].mError == kErrorDuplicated);
    VerifyOrQuit(!sConflictCallback.mWasCalled);

    Log("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -");
    Log("Unregister the conflicted host and register it again immediately, make sure we see probes");

    SuccessOrQuit(mdns->UnregisterService(service));

    sConflictCallback.Reset();
    sRegCallbacks[0].Reset();
    SuccessOrQuit(mdns->RegisterService(service, 0, HandleSuccessCallback));

    for (uint8_t probeCount = 0; probeCount < 3; probeCount++)
    {
        sDnsMessages.Clear();

        VerifyOrQuit(!sRegCallbacks[0].mWasCalled);
        AdvanceTime(250);

        VerifyOrQuit(!sDnsMessages.IsEmpty());
        dnsMsg = sDnsMessages.GetHead();
        dnsMsg->ValidateHeader(kMulticastQuery, /* Q */ 1, /* Ans */ 0, /* Auth */ 2, /* Addnl */ 0);
        dnsMsg->ValidateAsProbeFor(service, /* aUnicastRequest */ (probeCount == 0));
        VerifyOrQuit(dnsMsg->GetNext() == nullptr);
    }

    for (uint8_t anncCount = 0; anncCount < kNumAnnounces; anncCount++)
    {
        sDnsMessages.Clear();

        AdvanceTime((anncCount == 0) ? 250 : (1U << (anncCount - 1)) * 1000);
        VerifyOrQuit(sRegCallbacks[0].mWasCalled);

        VerifyOrQuit(!sDnsMessages.IsEmpty());
        dnsMsg = sDnsMessages.GetHead();
        dnsMsg->ValidateHeader(kMulticastResponse, /* Q */ 0, /* Ans */ 4, /* Auth */ 0, /* Addnl */ 1);
        dnsMsg->Validate(service, kInAnswerSection, kCheckSrv | kCheckTxt | kCheckPtr | kCheckServicesPtr);
        VerifyOrQuit(dnsMsg->GetNext() == nullptr);
    }

    VerifyOrQuit(!sConflictCallback.mWasCalled);

    Log("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -");
    Log("Send a response for service name and validate that conflict is detected and callback is called");

    SendResponseWithEmptyKey(fullServiceName.AsCString(), kInAnswerSection);
    AdvanceTime(1);

    VerifyOrQuit(sConflictCallback.mWasCalled);
    VerifyOrQuit(
        StringMatch(sConflictCallback.mName.AsCString(), service.mServiceInstance, kStringCaseInsensitiveMatch));
    VerifyOrQuit(sConflictCallback.mHasServiceType);
    VerifyOrQuit(
        StringMatch(sConflictCallback.mServiceType.AsCString(), service.mServiceType, kStringCaseInsensitiveMatch));

    sDnsMessages.Clear();
    AdvanceTime(20000);
    VerifyOrQuit(sDnsMessages.IsEmpty());

    SuccessOrQuit(mdns->SetEnabled(false, kInfraIfIndex));
    VerifyOrQuit(sHeapAllocatedPtrs.GetLength() <= heapAllocations);

    Log("End of test");

    testFreeInstance(sInstance);
}

//=====================================================================================================================
// Browser/Resolver tests

struct BrowseCallback : public Allocatable<BrowseCallback>, public LinkedListEntry<BrowseCallback>
{
    BrowseCallback *mNext;
    DnsName         mServiceType;
    DnsName         mSubTypeLabel;
    DnsName         mServiceInstance;
    uint32_t        mTtl;
    bool            mIsSubType;
};

struct SrvCallback : public Allocatable<SrvCallback>, public LinkedListEntry<SrvCallback>
{
    SrvCallback *mNext;
    DnsName      mServiceInstance;
    DnsName      mServiceType;
    DnsName      mHostName;
    uint16_t     mPort;
    uint16_t     mPriority;
    uint16_t     mWeight;
    uint32_t     mTtl;
};

struct TxtCallback : public Allocatable<TxtCallback>, public LinkedListEntry<TxtCallback>
{
    static constexpr uint16_t kMaxTxtDataLength = 100;

    template <uint16_t kSize> bool Matches(const uint8_t (&aData)[kSize]) const
    {
        return (mTxtDataLength == kSize) && (memcmp(mTxtData, aData, kSize) == 0);
    }

    TxtCallback *mNext;
    DnsName      mServiceInstance;
    DnsName      mServiceType;
    uint8_t      mTxtData[kMaxTxtDataLength];
    uint16_t     mTxtDataLength;
    uint32_t     mTtl;
};

struct AddrCallback : public Allocatable<AddrCallback>, public LinkedListEntry<AddrCallback>
{
    static constexpr uint16_t kMaxNumAddrs = 16;

    bool Contains(const AddrAndTtl &aAddrAndTtl) const
    {
        bool contains = false;

        for (uint16_t index = 0; index < mNumAddrs; index++)
        {
            if (mAddrAndTtls[index] == aAddrAndTtl)
            {
                contains = true;
                break;
            }
        }

        return contains;
    }

    bool Matches(const AddrAndTtl *aAddrAndTtls, uint16_t aNumAddrs) const
    {
        bool matches = true;

        VerifyOrExit(aNumAddrs == mNumAddrs, matches = false);

        for (uint16_t index = 0; index < mNumAddrs; index++)
        {
            if (!Contains(aAddrAndTtls[index]))
            {
                ExitNow(matches = false);
            }
        }

    exit:
        return matches;
    }

    AddrCallback *mNext;
    DnsName       mHostName;
    AddrAndTtl    mAddrAndTtls[kMaxNumAddrs];
    uint16_t      mNumAddrs;
};

OwningList<BrowseCallback> sBrowseCallbacks;
OwningList<SrvCallback>    sSrvCallbacks;
OwningList<TxtCallback>    sTxtCallbacks;
OwningList<AddrCallback>   sAddrCallbacks;

void HandleBrowseResult(otInstance *aInstance, const otMdnsBrowseResult *aResult)
{
    BrowseCallback *entry;

    VerifyOrQuit(aInstance == sInstance);
    VerifyOrQuit(aResult != nullptr);
    VerifyOrQuit(aResult->mServiceType != nullptr);
    VerifyOrQuit(aResult->mServiceInstance != nullptr);
    VerifyOrQuit(aResult->mInfraIfIndex == kInfraIfIndex);

    Log("Browse callback: %s (subtype:%s) -> %s ttl:%lu", aResult->mServiceType,
        aResult->mSubTypeLabel == nullptr ? "(null)" : aResult->mSubTypeLabel, aResult->mServiceInstance,
        ToUlong(aResult->mTtl));

    entry = BrowseCallback::Allocate();
    VerifyOrQuit(entry != nullptr);

    entry->mServiceType.CopyFrom(aResult->mServiceType);
    entry->mSubTypeLabel.CopyFrom(aResult->mSubTypeLabel);
    entry->mServiceInstance.CopyFrom(aResult->mServiceInstance);
    entry->mTtl       = aResult->mTtl;
    entry->mIsSubType = (aResult->mSubTypeLabel != nullptr);

    sBrowseCallbacks.PushAfterTail(*entry);
}

void HandleBrowseResultAlternate(otInstance *aInstance, const otMdnsBrowseResult *aResult)
{
    Log("Alternate browse callback is called");
    HandleBrowseResult(aInstance, aResult);
}

void HandleSrvResult(otInstance *aInstance, const otMdnsSrvResult *aResult)
{
    SrvCallback *entry;

    VerifyOrQuit(aInstance == sInstance);
    VerifyOrQuit(aResult != nullptr);
    VerifyOrQuit(aResult->mServiceInstance != nullptr);
    VerifyOrQuit(aResult->mServiceType != nullptr);
    VerifyOrQuit(aResult->mInfraIfIndex == kInfraIfIndex);

    if (aResult->mTtl != 0)
    {
        VerifyOrQuit(aResult->mHostName != nullptr);

        Log("SRV callback: %s %s, host:%s port:%u, prio:%u, weight:%u, ttl:%lu", aResult->mServiceInstance,
            aResult->mServiceType, aResult->mHostName, aResult->mPort, aResult->mPriority, aResult->mWeight,
            ToUlong(aResult->mTtl));
    }
    else
    {
        Log("SRV callback: %s %s, ttl:%lu", aResult->mServiceInstance, aResult->mServiceType, ToUlong(aResult->mTtl));
    }

    entry = SrvCallback::Allocate();
    VerifyOrQuit(entry != nullptr);

    entry->mServiceInstance.CopyFrom(aResult->mServiceInstance);
    entry->mServiceType.CopyFrom(aResult->mServiceType);
    entry->mHostName.CopyFrom(aResult->mHostName);
    entry->mPort     = aResult->mPort;
    entry->mPriority = aResult->mPriority;
    entry->mWeight   = aResult->mWeight;
    entry->mTtl      = aResult->mTtl;

    sSrvCallbacks.PushAfterTail(*entry);
}

void HandleSrvResultAlternate(otInstance *aInstance, const otMdnsSrvResult *aResult)
{
    Log("Alternate SRV callback is called");
    HandleSrvResult(aInstance, aResult);
}

void HandleTxtResult(otInstance *aInstance, const otMdnsTxtResult *aResult)
{
    TxtCallback *entry;

    VerifyOrQuit(aInstance == sInstance);
    VerifyOrQuit(aResult != nullptr);
    VerifyOrQuit(aResult->mServiceInstance != nullptr);
    VerifyOrQuit(aResult->mServiceType != nullptr);
    VerifyOrQuit(aResult->mInfraIfIndex == kInfraIfIndex);

    VerifyOrQuit(aResult->mTxtDataLength <= TxtCallback::kMaxTxtDataLength);

    if (aResult->mTtl != 0)
    {
        VerifyOrQuit(aResult->mTxtData != nullptr);

        Log("TXT callback: %s %s, len:%u, ttl:%lu", aResult->mServiceInstance, aResult->mServiceType,
            aResult->mTxtDataLength, ToUlong(aResult->mTtl));
    }
    else
    {
        Log("TXT callback: %s %s, ttl:%lu", aResult->mServiceInstance, aResult->mServiceType, ToUlong(aResult->mTtl));
    }

    entry = TxtCallback::Allocate();
    VerifyOrQuit(entry != nullptr);

    entry->mServiceInstance.CopyFrom(aResult->mServiceInstance);
    entry->mServiceType.CopyFrom(aResult->mServiceType);
    entry->mTxtDataLength = aResult->mTxtDataLength;
    memcpy(entry->mTxtData, aResult->mTxtData, aResult->mTxtDataLength);
    entry->mTtl = aResult->mTtl;

    sTxtCallbacks.PushAfterTail(*entry);
}

void HandleTxtResultAlternate(otInstance *aInstance, const otMdnsTxtResult *aResult)
{
    Log("Alternate TXT callback is called");
    HandleTxtResult(aInstance, aResult);
}

void HandleAddrResult(otInstance *aInstance, const otMdnsAddressResult *aResult)
{
    AddrCallback *entry;

    VerifyOrQuit(aInstance == sInstance);
    VerifyOrQuit(aResult != nullptr);
    VerifyOrQuit(aResult->mHostName != nullptr);
    VerifyOrQuit(aResult->mInfraIfIndex == kInfraIfIndex);

    VerifyOrQuit(aResult->mAddressesLength <= AddrCallback::kMaxNumAddrs);

    entry = AddrCallback::Allocate();
    VerifyOrQuit(entry != nullptr);

    entry->mHostName.CopyFrom(aResult->mHostName);
    entry->mNumAddrs = aResult->mAddressesLength;

    Log("Addr callback: %s, num:%u", aResult->mHostName, aResult->mAddressesLength);

    for (uint16_t index = 0; index < aResult->mAddressesLength; index++)
    {
        entry->mAddrAndTtls[index].mAddress = AsCoreType(&aResult->mAddresses[index].mAddress);
        entry->mAddrAndTtls[index].mTtl     = aResult->mAddresses[index].mTtl;

        Log(" - %s, ttl:%lu", entry->mAddrAndTtls[index].mAddress.ToString().AsCString(),
            ToUlong(entry->mAddrAndTtls[index].mTtl));
    }

    sAddrCallbacks.PushAfterTail(*entry);
}

void HandleAddrResultAlternate(otInstance *aInstance, const otMdnsAddressResult *aResult)
{
    Log("Alternate addr callback is called");
    HandleAddrResult(aInstance, aResult);
}

//---------------------------------------------------------------------------------------------------------------------

void TestBrowser(void)
{
    Core                 *mdns = InitTest();
    Core::Browser         browser;
    Core::Browser         browser2;
    const DnsMessage     *dnsMsg;
    const BrowseCallback *browseCallback;
    uint16_t              heapAllocations;

    Log("-------------------------------------------------------------------------------------------");
    Log("TestBrowser");

    AdvanceTime(1);

    heapAllocations = sHeapAllocatedPtrs.GetLength();
    SuccessOrQuit(mdns->SetEnabled(true, kInfraIfIndex));

    Log("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -");
    Log("Start a browser. Validate initial queries.");

    ClearAllBytes(browser);

    browser.mServiceType  = "_srv._udp";
    browser.mSubTypeLabel = nullptr;
    browser.mInfraIfIndex = kInfraIfIndex;
    browser.mCallback     = HandleBrowseResult;

    sDnsMessages.Clear();
    SuccessOrQuit(mdns->StartBrowser(browser));

    for (uint8_t queryCount = 0; queryCount < kNumInitalQueries; queryCount++)
    {
        sDnsMessages.Clear();

        AdvanceTime((queryCount == 0) ? 125 : (1U << (queryCount - 1)) * 1000);

        VerifyOrQuit(!sDnsMessages.IsEmpty());
        dnsMsg = sDnsMessages.GetHead();
        dnsMsg->ValidateHeader(kMulticastQuery, /* Q */ 1, /* Ans */ 0, /* Auth */ 0, /* Addnl */ 0);
        dnsMsg->ValidateAsQueryFor(browser);
        VerifyOrQuit(dnsMsg->GetNext() == nullptr);
    }

    sDnsMessages.Clear();

    AdvanceTime(20000);
    VerifyOrQuit(sDnsMessages.IsEmpty());

    Log("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -");
    Log("Send a response. Validate callback result.");

    sBrowseCallbacks.Clear();

    SendPtrResponse("_srv._udp.local.", "mysrv._srv._udp.local.", 120, kInAnswerSection);

    AdvanceTime(1);

    VerifyOrQuit(!sBrowseCallbacks.IsEmpty());
    browseCallback = sBrowseCallbacks.GetHead();
    VerifyOrQuit(browseCallback->mServiceType.Matches("_srv._udp"));
    VerifyOrQuit(!browseCallback->mIsSubType);
    VerifyOrQuit(browseCallback->mServiceInstance.Matches("mysrv"));
    VerifyOrQuit(browseCallback->mTtl == 120);
    VerifyOrQuit(browseCallback->GetNext() == nullptr);

    VerifyOrQuit(sDnsMessages.IsEmpty());

    Log("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -");
    Log("Send another response. Validate callback result.");

    AdvanceTime(10000);

    sBrowseCallbacks.Clear();

    SendPtrResponse("_srv._udp.local.", "awesome._srv._udp.local.", 500, kInAnswerSection);

    AdvanceTime(1);

    VerifyOrQuit(!sBrowseCallbacks.IsEmpty());
    browseCallback = sBrowseCallbacks.GetHead();
    VerifyOrQuit(browseCallback->mServiceType.Matches("_srv._udp"));
    VerifyOrQuit(!browseCallback->mIsSubType);
    VerifyOrQuit(browseCallback->mServiceInstance.Matches("awesome"));
    VerifyOrQuit(browseCallback->mTtl == 500);
    VerifyOrQuit(browseCallback->GetNext() == nullptr);

    VerifyOrQuit(sDnsMessages.IsEmpty());

    Log("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -");
    Log("Start another browser for the same service and different callback. Validate results.");

    AdvanceTime(5000);

    browser2.mServiceType  = "_srv._udp";
    browser2.mSubTypeLabel = nullptr;
    browser2.mInfraIfIndex = kInfraIfIndex;
    browser2.mCallback     = HandleBrowseResultAlternate;

    sBrowseCallbacks.Clear();

    SuccessOrQuit(mdns->StartBrowser(browser2));

    browseCallback = sBrowseCallbacks.GetHead();

    for (uint8_t iter = 0; iter < 2; iter++)
    {
        VerifyOrQuit(browseCallback != nullptr);

        VerifyOrQuit(browseCallback->mServiceType.Matches("_srv._udp"));
        VerifyOrQuit(!browseCallback->mIsSubType);

        if (browseCallback->mServiceInstance.Matches("awesome"))
        {
            VerifyOrQuit(browseCallback->mTtl == 500);
        }
        else if (browseCallback->mServiceInstance.Matches("mysrv"))
        {
            VerifyOrQuit(browseCallback->mTtl == 120);
        }
        else
        {
            VerifyOrQuit(false);
        }

        browseCallback = browseCallback->GetNext();
    }

    VerifyOrQuit(browseCallback == nullptr);

    AdvanceTime(5000);

    VerifyOrQuit(sDnsMessages.IsEmpty());

    Log("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -");
    Log("Start same browser again and check the returned error.");

    sBrowseCallbacks.Clear();

    VerifyOrQuit(mdns->StartBrowser(browser2) == kErrorAlready);

    AdvanceTime(5000);

    VerifyOrQuit(sBrowseCallbacks.IsEmpty());

    Log("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -");
    Log("Send a goodbye response. Validate result callback for both browsers.");

    SendPtrResponse("_srv._udp.local.", "awesome._srv._udp.local.", 0, kInAnswerSection);

    AdvanceTime(1);

    browseCallback = sBrowseCallbacks.GetHead();

    for (uint8_t iter = 0; iter < 2; iter++)
    {
        VerifyOrQuit(browseCallback != nullptr);

        VerifyOrQuit(browseCallback->mServiceType.Matches("_srv._udp"));
        VerifyOrQuit(!browseCallback->mIsSubType);
        VerifyOrQuit(browseCallback->mServiceInstance.Matches("awesome"));
        VerifyOrQuit(browseCallback->mTtl == 0);

        browseCallback = browseCallback->GetNext();
    }

    VerifyOrQuit(browseCallback == nullptr);

    VerifyOrQuit(sDnsMessages.IsEmpty());

    Log("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -");
    Log("Send a response with no changes, validate that no callback is invoked.");

    sBrowseCallbacks.Clear();

    SendPtrResponse("_srv._udp.local.", "mysrv._srv._udp.local.", 120, kInAnswerSection);

    AdvanceTime(1);

    VerifyOrQuit(sBrowseCallbacks.IsEmpty());
    VerifyOrQuit(sDnsMessages.IsEmpty());

    Log("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -");
    Log("Stop the second browser.");

    sBrowseCallbacks.Clear();

    SuccessOrQuit(mdns->StopBrowser(browser2));

    AdvanceTime(5000);

    VerifyOrQuit(sBrowseCallbacks.IsEmpty());
    VerifyOrQuit(sDnsMessages.IsEmpty());

    Log("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -");
    Log("Check query is sent at 80 percentage of TTL and then respond to it.");

    // First query should be sent at 80-82% of TTL of 120 second (96.0-98.4 sec).
    // We wait for 100 second. Note that 5 seconds already passed in the
    // previous step.

    AdvanceTime(91 * 1000 - 1);

    VerifyOrQuit(sDnsMessages.IsEmpty());

    AdvanceTime(4 * 1000 + 1);

    VerifyOrQuit(!sDnsMessages.IsEmpty());
    dnsMsg = sDnsMessages.GetHead();
    dnsMsg->ValidateHeader(kMulticastQuery, /* Q */ 1, /* Ans */ 0, /* Auth */ 0, /* Addnl */ 0);
    dnsMsg->ValidateAsQueryFor(browser);
    VerifyOrQuit(dnsMsg->GetNext() == nullptr);

    sDnsMessages.Clear();
    VerifyOrQuit(sBrowseCallbacks.IsEmpty());

    AdvanceTime(10);

    SendPtrResponse("_srv._udp.local.", "mysrv._srv._udp.local.", 120, kInAnswerSection);

    Log("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -");
    Log("Check queries are sent at 80, 85, 90, 95 percentages of TTL.");

    for (uint8_t queryCount = 0; queryCount < kNumRefreshQueries; queryCount++)
    {
        if (queryCount == 0)
        {
            // First query is expected in 80-82% of TTL, so
            // 80% of 120 = 96.0, 82% of 120 = 98.4

            AdvanceTime(96 * 1000 - 1);
        }
        else
        {
            // Next query should happen within 3%-5% of TTL
            // from previous query. We wait 3% of TTL here.
            AdvanceTime(3600 - 1);
        }

        VerifyOrQuit(sDnsMessages.IsEmpty());

        // Wait for 2% of TTL of 120 which is 2.4 sec.

        AdvanceTime(2400 + 1);

        VerifyOrQuit(!sDnsMessages.IsEmpty());
        dnsMsg = sDnsMessages.GetHead();
        dnsMsg->ValidateHeader(kMulticastQuery, /* Q */ 1, /* Ans */ 0, /* Auth */ 0, /* Addnl */ 0);
        dnsMsg->ValidateAsQueryFor(browser);
        VerifyOrQuit(dnsMsg->GetNext() == nullptr);

        sDnsMessages.Clear();
        VerifyOrQuit(sBrowseCallbacks.IsEmpty());
    }

    Log("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -");
    Log("Check TTL timeout and callback result.");

    AdvanceTime(6 * 1000);

    VerifyOrQuit(!sBrowseCallbacks.IsEmpty());

    browseCallback = sBrowseCallbacks.GetHead();
    VerifyOrQuit(browseCallback->mServiceType.Matches("_srv._udp"));
    VerifyOrQuit(!browseCallback->mIsSubType);
    VerifyOrQuit(browseCallback->mServiceInstance.Matches("mysrv"));
    VerifyOrQuit(browseCallback->mTtl == 0);
    VerifyOrQuit(browseCallback->GetNext() == nullptr);

    Log("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -");

    sBrowseCallbacks.Clear();
    sDnsMessages.Clear();

    AdvanceTime(200 * 1000);

    VerifyOrQuit(sBrowseCallbacks.IsEmpty());
    VerifyOrQuit(sDnsMessages.IsEmpty());

    Log("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -");
    Log("Send a new response and make sure result callback is invoked");

    SendPtrResponse("_srv._udp.local.", "great._srv._udp.local.", 200, kInAdditionalSection);

    AdvanceTime(1);

    browseCallback = sBrowseCallbacks.GetHead();

    VerifyOrQuit(browseCallback->mServiceType.Matches("_srv._udp"));
    VerifyOrQuit(!browseCallback->mIsSubType);
    VerifyOrQuit(browseCallback->mServiceInstance.Matches("great"));
    VerifyOrQuit(browseCallback->mTtl == 200);
    VerifyOrQuit(browseCallback->GetNext() == nullptr);

    sBrowseCallbacks.Clear();

    AdvanceTime(150 * 1000);

    VerifyOrQuit(sDnsMessages.IsEmpty());
    VerifyOrQuit(sBrowseCallbacks.IsEmpty());

    Log("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -");
    Log("Stop the browser. There is no active browser for this service. Ensure no queries are sent");

    sBrowseCallbacks.Clear();

    SuccessOrQuit(mdns->StopBrowser(browser));

    AdvanceTime(100 * 1000);

    VerifyOrQuit(sBrowseCallbacks.IsEmpty());
    VerifyOrQuit(sDnsMessages.IsEmpty());

    Log("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -");
    Log("Start browser again. Validate that initial queries are sent again");

    SuccessOrQuit(mdns->StartBrowser(browser));

    AdvanceTime(125);

    VerifyOrQuit(!sDnsMessages.IsEmpty());
    dnsMsg = sDnsMessages.GetHead();
    dnsMsg->ValidateHeader(kMulticastQuery, /* Q */ 1, /* Ans */ 0, /* Auth */ 0, /* Addnl */ 0);
    dnsMsg->ValidateAsQueryFor(browser);
    VerifyOrQuit(dnsMsg->GetNext() == nullptr);

    Log("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -");
    Log("Send a response after the first initial query");

    sDnsMessages.Clear();

    SendPtrResponse("_srv._udp.local.", "mysrv._srv._udp.local.", 120, kInAnswerSection);

    AdvanceTime(1);

    browseCallback = sBrowseCallbacks.GetHead();

    VerifyOrQuit(browseCallback->mServiceType.Matches("_srv._udp"));
    VerifyOrQuit(!browseCallback->mIsSubType);
    VerifyOrQuit(browseCallback->mServiceInstance.Matches("mysrv"));
    VerifyOrQuit(browseCallback->mTtl == 120);
    VerifyOrQuit(browseCallback->GetNext() == nullptr);

    sBrowseCallbacks.Clear();

    Log("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -");
    Log("Validate initial esquires are still sent and include known-answer");

    for (uint8_t queryCount = 1; queryCount < kNumInitalQueries; queryCount++)
    {
        sDnsMessages.Clear();

        AdvanceTime((1U << (queryCount - 1)) * 1000);

        VerifyOrQuit(!sDnsMessages.IsEmpty());
        dnsMsg = sDnsMessages.GetHead();
        dnsMsg->ValidateHeader(kMulticastQuery, /* Q */ 1, /* Ans */ 1, /* Auth */ 0, /* Addnl */ 0);
        dnsMsg->ValidateAsQueryFor(browser);
        VerifyOrQuit(dnsMsg->GetNext() == nullptr);
    }

    Log("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -");

    sDnsMessages.Clear();
    AdvanceTime(50 * 1000);
    VerifyOrQuit(sDnsMessages.IsEmpty());

    SuccessOrQuit(mdns->SetEnabled(false, kInfraIfIndex));
    VerifyOrQuit(sHeapAllocatedPtrs.GetLength() <= heapAllocations);

    Log("End of test");

    testFreeInstance(sInstance);
}

void TestSrvResolver(void)
{
    Core              *mdns = InitTest();
    Core::SrvResolver  resolver;
    Core::SrvResolver  resolver2;
    const DnsMessage  *dnsMsg;
    const SrvCallback *srvCallback;
    uint16_t           heapAllocations;

    Log("-------------------------------------------------------------------------------------------");
    Log("TestSrvResolver");

    AdvanceTime(1);

    heapAllocations = sHeapAllocatedPtrs.GetLength();
    SuccessOrQuit(mdns->SetEnabled(true, kInfraIfIndex));

    Log("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -");
    Log("Start a SRV resolver. Validate initial queries.");

    ClearAllBytes(resolver);

    resolver.mServiceInstance = "mysrv";
    resolver.mServiceType     = "_srv._udp";
    resolver.mInfraIfIndex    = kInfraIfIndex;
    resolver.mCallback        = HandleSrvResult;

    sDnsMessages.Clear();
    SuccessOrQuit(mdns->StartSrvResolver(resolver));

    for (uint8_t queryCount = 0; queryCount < kNumInitalQueries; queryCount++)
    {
        sDnsMessages.Clear();

        AdvanceTime((queryCount == 0) ? 125 : (1U << (queryCount - 1)) * 1000);

        VerifyOrQuit(!sDnsMessages.IsEmpty());
        dnsMsg = sDnsMessages.GetHead();
        dnsMsg->ValidateHeader(kMulticastQuery, /* Q */ 1, /* Ans */ 0, /* Auth */ 0, /* Addnl */ 0);
        dnsMsg->ValidateAsQueryFor(resolver);
        VerifyOrQuit(dnsMsg->GetNext() == nullptr);
    }

    sDnsMessages.Clear();

    AdvanceTime(20 * 1000);
    VerifyOrQuit(sDnsMessages.IsEmpty());

    Log("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -");
    Log("Send a response. Validate callback result.");

    sSrvCallbacks.Clear();

    SendSrvResponse("mysrv._srv._udp.local.", "myhost.local.", 1234, 0, 1, 120, kInAnswerSection);

    AdvanceTime(1);

    VerifyOrQuit(!sSrvCallbacks.IsEmpty());
    srvCallback = sSrvCallbacks.GetHead();
    VerifyOrQuit(srvCallback->mServiceInstance.Matches("mysrv"));
    VerifyOrQuit(srvCallback->mServiceType.Matches("_srv._udp"));
    VerifyOrQuit(srvCallback->mHostName.Matches("myhost"));
    VerifyOrQuit(srvCallback->mPort == 1234);
    VerifyOrQuit(srvCallback->mPriority == 0);
    VerifyOrQuit(srvCallback->mWeight == 1);
    VerifyOrQuit(srvCallback->mTtl == 120);
    VerifyOrQuit(srvCallback->GetNext() == nullptr);

    VerifyOrQuit(sDnsMessages.IsEmpty());

    Log("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -");
    Log("Send an updated response changing host name. Validate callback result.");

    AdvanceTime(1000);

    sSrvCallbacks.Clear();

    SendSrvResponse("mysrv._srv._udp.local.", "myhost2.local.", 1234, 0, 1, 120, kInAnswerSection);

    AdvanceTime(1);

    VerifyOrQuit(!sSrvCallbacks.IsEmpty());
    srvCallback = sSrvCallbacks.GetHead();
    VerifyOrQuit(srvCallback->mServiceInstance.Matches("mysrv"));
    VerifyOrQuit(srvCallback->mServiceType.Matches("_srv._udp"));
    VerifyOrQuit(srvCallback->mHostName.Matches("myhost2"));
    VerifyOrQuit(srvCallback->mPort == 1234);
    VerifyOrQuit(srvCallback->mPriority == 0);
    VerifyOrQuit(srvCallback->mWeight == 1);
    VerifyOrQuit(srvCallback->mTtl == 120);
    VerifyOrQuit(srvCallback->GetNext() == nullptr);

    VerifyOrQuit(sDnsMessages.IsEmpty());

    Log("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -");
    Log("Send an updated response changing port. Validate callback result.");

    AdvanceTime(1000);

    sSrvCallbacks.Clear();

    SendSrvResponse("mysrv._srv._udp.local.", "myhost2.local.", 4567, 0, 1, 120, kInAnswerSection);

    AdvanceTime(1);

    VerifyOrQuit(!sSrvCallbacks.IsEmpty());
    srvCallback = sSrvCallbacks.GetHead();
    VerifyOrQuit(srvCallback->mServiceInstance.Matches("mysrv"));
    VerifyOrQuit(srvCallback->mServiceType.Matches("_srv._udp"));
    VerifyOrQuit(srvCallback->mHostName.Matches("myhost2"));
    VerifyOrQuit(srvCallback->mPort == 4567);
    VerifyOrQuit(srvCallback->mPriority == 0);
    VerifyOrQuit(srvCallback->mWeight == 1);
    VerifyOrQuit(srvCallback->mTtl == 120);
    VerifyOrQuit(srvCallback->GetNext() == nullptr);

    VerifyOrQuit(sDnsMessages.IsEmpty());

    Log("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -");
    Log("Send an updated response changing TTL. Validate callback result.");

    AdvanceTime(1000);

    sSrvCallbacks.Clear();

    SendSrvResponse("mysrv._srv._udp.local.", "myhost2.local.", 4567, 0, 1, 0, kInAnswerSection);

    AdvanceTime(1);

    VerifyOrQuit(!sSrvCallbacks.IsEmpty());
    srvCallback = sSrvCallbacks.GetHead();
    VerifyOrQuit(srvCallback->mServiceInstance.Matches("mysrv"));
    VerifyOrQuit(srvCallback->mServiceType.Matches("_srv._udp"));
    VerifyOrQuit(srvCallback->mHostName.Matches(""));
    VerifyOrQuit(srvCallback->mPort == 4567);
    VerifyOrQuit(srvCallback->mPriority == 0);
    VerifyOrQuit(srvCallback->mWeight == 1);
    VerifyOrQuit(srvCallback->mTtl == 0);
    VerifyOrQuit(srvCallback->GetNext() == nullptr);

    VerifyOrQuit(sDnsMessages.IsEmpty());

    Log("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -");
    Log("Send an updated response changing a bunch of things. Validate callback result.");

    AdvanceTime(1000);

    sSrvCallbacks.Clear();

    SendSrvResponse("mysrv._srv._udp.local.", "myhost.local.", 1234, 2, 3, 120, kInAnswerSection);

    AdvanceTime(1);

    VerifyOrQuit(!sSrvCallbacks.IsEmpty());
    srvCallback = sSrvCallbacks.GetHead();
    VerifyOrQuit(srvCallback->mServiceInstance.Matches("mysrv"));
    VerifyOrQuit(srvCallback->mServiceType.Matches("_srv._udp"));
    VerifyOrQuit(srvCallback->mHostName.Matches("myhost"));
    VerifyOrQuit(srvCallback->mPort == 1234);
    VerifyOrQuit(srvCallback->mPriority == 2);
    VerifyOrQuit(srvCallback->mWeight == 3);
    VerifyOrQuit(srvCallback->mTtl == 120);
    VerifyOrQuit(srvCallback->GetNext() == nullptr);

    VerifyOrQuit(sDnsMessages.IsEmpty());

    Log("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -");
    Log("Send a response with no changes. Validate callback is not invoked.");

    AdvanceTime(1000);

    sSrvCallbacks.Clear();

    SendSrvResponse("mysrv._srv._udp.local.", "myhost.local.", 1234, 2, 3, 120, kInAnswerSection);

    AdvanceTime(1);

    VerifyOrQuit(sSrvCallbacks.IsEmpty());
    VerifyOrQuit(sDnsMessages.IsEmpty());

    Log("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -");
    Log("Start another resolver for the same service and different callback. Validate results.");

    ClearAllBytes(resolver2);

    resolver2.mServiceInstance = "mysrv";
    resolver2.mServiceType     = "_srv._udp";
    resolver2.mInfraIfIndex    = kInfraIfIndex;
    resolver2.mCallback        = HandleSrvResultAlternate;

    sSrvCallbacks.Clear();

    SuccessOrQuit(mdns->StartSrvResolver(resolver2));

    AdvanceTime(1);

    VerifyOrQuit(!sSrvCallbacks.IsEmpty());
    srvCallback = sSrvCallbacks.GetHead();
    VerifyOrQuit(srvCallback->mServiceInstance.Matches("mysrv"));
    VerifyOrQuit(srvCallback->mServiceType.Matches("_srv._udp"));
    VerifyOrQuit(srvCallback->mHostName.Matches("myhost"));
    VerifyOrQuit(srvCallback->mPort == 1234);
    VerifyOrQuit(srvCallback->mPriority == 2);
    VerifyOrQuit(srvCallback->mWeight == 3);
    VerifyOrQuit(srvCallback->mTtl == 120);
    VerifyOrQuit(srvCallback->GetNext() == nullptr);

    VerifyOrQuit(sDnsMessages.IsEmpty());

    Log("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -");
    Log("Start same resolver again and check the returned error.");

    sSrvCallbacks.Clear();

    VerifyOrQuit(mdns->StartSrvResolver(resolver2) == kErrorAlready);

    AdvanceTime(5000);

    VerifyOrQuit(sSrvCallbacks.IsEmpty());

    Log("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -");
    Log("Check query is sent at 80 percentage of TTL and then respond to it.");

    SendSrvResponse("mysrv._srv._udp.local.", "myhost.local.", 1234, 2, 3, 120, kInAnswerSection);

    // First query should be sent at 80-82% of TTL of 120 second (96.0-98.4 sec).
    // We wait for 100 second. Note that 5 seconds already passed in the
    // previous step.

    AdvanceTime(96 * 1000 - 1);

    VerifyOrQuit(sDnsMessages.IsEmpty());

    AdvanceTime(4 * 1000 + 1);

    VerifyOrQuit(!sDnsMessages.IsEmpty());
    dnsMsg = sDnsMessages.GetHead();
    dnsMsg->ValidateHeader(kMulticastQuery, /* Q */ 1, /* Ans */ 0, /* Auth */ 0, /* Addnl */ 0);
    dnsMsg->ValidateAsQueryFor(resolver);
    VerifyOrQuit(dnsMsg->GetNext() == nullptr);

    sDnsMessages.Clear();
    VerifyOrQuit(sSrvCallbacks.IsEmpty());

    AdvanceTime(10);

    SendSrvResponse("mysrv._srv._udp.local.", "myhost.local.", 1234, 2, 3, 120, kInAnswerSection);

    Log("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -");
    Log("Check queries are sent at 80, 85, 90, 95 percentages of TTL.");

    for (uint8_t queryCount = 0; queryCount < kNumRefreshQueries; queryCount++)
    {
        if (queryCount == 0)
        {
            // First query is expected in 80-82% of TTL, so
            // 80% of 120 = 96.0, 82% of 120 = 98.4

            AdvanceTime(96 * 1000 - 1);
        }
        else
        {
            // Next query should happen within 3%-5% of TTL
            // from previous query. We wait 3% of TTL here.
            AdvanceTime(3600 - 1);
        }

        VerifyOrQuit(sDnsMessages.IsEmpty());

        // Wait for 2% of TTL of 120 which is 2.4 sec.

        AdvanceTime(2400 + 1);

        VerifyOrQuit(!sDnsMessages.IsEmpty());
        dnsMsg = sDnsMessages.GetHead();
        dnsMsg->ValidateHeader(kMulticastQuery, /* Q */ 1, /* Ans */ 0, /* Auth */ 0, /* Addnl */ 0);
        dnsMsg->ValidateAsQueryFor(resolver);
        VerifyOrQuit(dnsMsg->GetNext() == nullptr);

        sDnsMessages.Clear();
        VerifyOrQuit(sSrvCallbacks.IsEmpty());
    }

    Log("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -");
    Log("Check TTL timeout and callback result.");

    AdvanceTime(6 * 1000);

    srvCallback = sSrvCallbacks.GetHead();

    for (uint8_t iter = 0; iter < 2; iter++)
    {
        VerifyOrQuit(srvCallback != nullptr);
        VerifyOrQuit(srvCallback->mServiceInstance.Matches("mysrv"));
        VerifyOrQuit(srvCallback->mServiceType.Matches("_srv._udp"));
        VerifyOrQuit(srvCallback->mTtl == 0);
        srvCallback = srvCallback->GetNext();
    }

    VerifyOrQuit(srvCallback == nullptr);

    VerifyOrQuit(sDnsMessages.IsEmpty());

    Log("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -");

    sSrvCallbacks.Clear();
    sDnsMessages.Clear();

    AdvanceTime(200 * 1000);

    VerifyOrQuit(sSrvCallbacks.IsEmpty());
    VerifyOrQuit(sDnsMessages.IsEmpty());

    Log("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -");
    Log("Stop the second resolver");

    sSrvCallbacks.Clear();

    SuccessOrQuit(mdns->StopSrvResolver(resolver2));

    AdvanceTime(100 * 1000);

    VerifyOrQuit(sSrvCallbacks.IsEmpty());
    VerifyOrQuit(sDnsMessages.IsEmpty());

    Log("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -");
    Log("Send a new response and make sure result callback is invoked");

    SendSrvResponse("mysrv._srv._udp.local.", "myhost.local.", 1234, 2, 3, 120, kInAnswerSection);

    AdvanceTime(1);

    VerifyOrQuit(!sSrvCallbacks.IsEmpty());
    srvCallback = sSrvCallbacks.GetHead();
    VerifyOrQuit(srvCallback->mServiceInstance.Matches("mysrv"));
    VerifyOrQuit(srvCallback->mServiceType.Matches("_srv._udp"));
    VerifyOrQuit(srvCallback->mHostName.Matches("myhost"));
    VerifyOrQuit(srvCallback->mPort == 1234);
    VerifyOrQuit(srvCallback->mPriority == 2);
    VerifyOrQuit(srvCallback->mWeight == 3);
    VerifyOrQuit(srvCallback->mTtl == 120);
    VerifyOrQuit(srvCallback->GetNext() == nullptr);

    VerifyOrQuit(sDnsMessages.IsEmpty());

    Log("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -");
    Log("Stop the resolver. There is no active resolver. Ensure no queries are sent");

    sSrvCallbacks.Clear();

    SuccessOrQuit(mdns->StopSrvResolver(resolver));

    AdvanceTime(20 * 1000);

    VerifyOrQuit(sSrvCallbacks.IsEmpty());
    VerifyOrQuit(sDnsMessages.IsEmpty());

    Log("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -");
    Log("Restart the resolver with more than half of TTL remaining.");
    Log("Ensure cached entry is reported in the result callback and no queries are sent.");

    SuccessOrQuit(mdns->StartSrvResolver(resolver));

    AdvanceTime(1);

    VerifyOrQuit(!sSrvCallbacks.IsEmpty());
    srvCallback = sSrvCallbacks.GetHead();
    VerifyOrQuit(srvCallback->mServiceInstance.Matches("mysrv"));
    VerifyOrQuit(srvCallback->mServiceType.Matches("_srv._udp"));
    VerifyOrQuit(srvCallback->mHostName.Matches("myhost"));
    VerifyOrQuit(srvCallback->mPort == 1234);
    VerifyOrQuit(srvCallback->mPriority == 2);
    VerifyOrQuit(srvCallback->mWeight == 3);
    VerifyOrQuit(srvCallback->mTtl == 120);
    VerifyOrQuit(srvCallback->GetNext() == nullptr);

    VerifyOrQuit(sDnsMessages.IsEmpty());

    AdvanceTime(20 * 1000);

    VerifyOrQuit(sDnsMessages.IsEmpty());

    Log("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -");
    Log("Stop and start the resolver again after less than half TTL remaining.");
    Log("Ensure cached entry is still reported in the result callback but queries should be sent");

    sSrvCallbacks.Clear();

    SuccessOrQuit(mdns->StopSrvResolver(resolver));

    AdvanceTime(25 * 1000);

    SuccessOrQuit(mdns->StartSrvResolver(resolver));

    AdvanceTime(1);

    VerifyOrQuit(!sSrvCallbacks.IsEmpty());
    srvCallback = sSrvCallbacks.GetHead();
    VerifyOrQuit(srvCallback->mServiceInstance.Matches("mysrv"));
    VerifyOrQuit(srvCallback->mServiceType.Matches("_srv._udp"));
    VerifyOrQuit(srvCallback->mHostName.Matches("myhost"));
    VerifyOrQuit(srvCallback->mPort == 1234);
    VerifyOrQuit(srvCallback->mPriority == 2);
    VerifyOrQuit(srvCallback->mWeight == 3);
    VerifyOrQuit(srvCallback->mTtl == 120);
    VerifyOrQuit(srvCallback->GetNext() == nullptr);

    sSrvCallbacks.Clear();

    AdvanceTime(15 * 1000);

    dnsMsg = sDnsMessages.GetHead();

    for (uint8_t queryCount = 0; queryCount < kNumInitalQueries; queryCount++)
    {
        VerifyOrQuit(dnsMsg != nullptr);
        dnsMsg->ValidateHeader(kMulticastQuery, /* Q */ 1, /* Ans */ 0, /* Auth */ 0, /* Addnl */ 0);
        dnsMsg->ValidateAsQueryFor(resolver);
        dnsMsg = dnsMsg->GetNext();
    }

    VerifyOrQuit(dnsMsg == nullptr);

    Log("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -");

    SuccessOrQuit(mdns->SetEnabled(false, kInfraIfIndex));
    VerifyOrQuit(sHeapAllocatedPtrs.GetLength() <= heapAllocations);

    Log("End of test");

    testFreeInstance(sInstance);
}

void TestTxtResolver(void)
{
    Core              *mdns = InitTest();
    Core::TxtResolver  resolver;
    Core::TxtResolver  resolver2;
    const DnsMessage  *dnsMsg;
    const TxtCallback *txtCallback;
    uint16_t           heapAllocations;

    Log("-------------------------------------------------------------------------------------------");
    Log("TestTxtResolver");

    AdvanceTime(1);

    heapAllocations = sHeapAllocatedPtrs.GetLength();
    SuccessOrQuit(mdns->SetEnabled(true, kInfraIfIndex));

    Log("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -");
    Log("Start a TXT resolver. Validate initial queries.");

    ClearAllBytes(resolver);

    resolver.mServiceInstance = "mysrv";
    resolver.mServiceType     = "_srv._udp";
    resolver.mInfraIfIndex    = kInfraIfIndex;
    resolver.mCallback        = HandleTxtResult;

    sDnsMessages.Clear();
    SuccessOrQuit(mdns->StartTxtResolver(resolver));

    for (uint8_t queryCount = 0; queryCount < kNumInitalQueries; queryCount++)
    {
        sDnsMessages.Clear();

        AdvanceTime((queryCount == 0) ? 125 : (1U << (queryCount - 1)) * 1000);

        VerifyOrQuit(!sDnsMessages.IsEmpty());
        dnsMsg = sDnsMessages.GetHead();
        dnsMsg->ValidateHeader(kMulticastQuery, /* Q */ 1, /* Ans */ 0, /* Auth */ 0, /* Addnl */ 0);
        dnsMsg->ValidateAsQueryFor(resolver);
        VerifyOrQuit(dnsMsg->GetNext() == nullptr);
    }

    sDnsMessages.Clear();

    AdvanceTime(20 * 1000);
    VerifyOrQuit(sDnsMessages.IsEmpty());

    Log("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -");
    Log("Send a response. Validate callback result.");

    sTxtCallbacks.Clear();

    SendTxtResponse("mysrv._srv._udp.local.", kTxtData1, sizeof(kTxtData1), 120, kInAnswerSection);

    AdvanceTime(1);

    VerifyOrQuit(!sTxtCallbacks.IsEmpty());
    txtCallback = sTxtCallbacks.GetHead();
    VerifyOrQuit(txtCallback->mServiceInstance.Matches("mysrv"));
    VerifyOrQuit(txtCallback->mServiceType.Matches("_srv._udp"));
    VerifyOrQuit(txtCallback->Matches(kTxtData1));
    VerifyOrQuit(txtCallback->mTtl == 120);
    VerifyOrQuit(txtCallback->GetNext() == nullptr);

    VerifyOrQuit(sDnsMessages.IsEmpty());

    Log("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -");
    Log("Send an updated response changing TXT data. Validate callback result.");

    AdvanceTime(1000);

    sTxtCallbacks.Clear();

    SendTxtResponse("mysrv._srv._udp.local.", kTxtData2, sizeof(kTxtData2), 120, kInAnswerSection);

    AdvanceTime(1);

    VerifyOrQuit(!sTxtCallbacks.IsEmpty());
    txtCallback = sTxtCallbacks.GetHead();
    VerifyOrQuit(txtCallback->mServiceInstance.Matches("mysrv"));
    VerifyOrQuit(txtCallback->mServiceType.Matches("_srv._udp"));
    VerifyOrQuit(txtCallback->Matches(kTxtData2));
    VerifyOrQuit(txtCallback->mTtl == 120);
    VerifyOrQuit(txtCallback->GetNext() == nullptr);

    VerifyOrQuit(sDnsMessages.IsEmpty());

    Log("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -");
    Log("Send an updated response changing TXT data to empty. Validate callback result.");

    AdvanceTime(1000);

    sTxtCallbacks.Clear();

    SendTxtResponse("mysrv._srv._udp.local.", kEmptyTxtData, sizeof(kEmptyTxtData), 120, kInAnswerSection);

    AdvanceTime(1);

    VerifyOrQuit(!sTxtCallbacks.IsEmpty());
    txtCallback = sTxtCallbacks.GetHead();
    VerifyOrQuit(txtCallback->mServiceInstance.Matches("mysrv"));
    VerifyOrQuit(txtCallback->mServiceType.Matches("_srv._udp"));
    VerifyOrQuit(txtCallback->Matches(kEmptyTxtData));
    VerifyOrQuit(txtCallback->mTtl == 120);
    VerifyOrQuit(txtCallback->GetNext() == nullptr);

    VerifyOrQuit(sDnsMessages.IsEmpty());

    Log("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -");
    Log("Send an updated response changing TTL. Validate callback result.");

    AdvanceTime(1000);

    sTxtCallbacks.Clear();

    SendTxtResponse("mysrv._srv._udp.local.", kEmptyTxtData, sizeof(kEmptyTxtData), 500, kInAnswerSection);

    AdvanceTime(1);

    VerifyOrQuit(!sTxtCallbacks.IsEmpty());
    txtCallback = sTxtCallbacks.GetHead();
    VerifyOrQuit(txtCallback->mServiceInstance.Matches("mysrv"));
    VerifyOrQuit(txtCallback->mServiceType.Matches("_srv._udp"));
    VerifyOrQuit(txtCallback->Matches(kEmptyTxtData));
    VerifyOrQuit(txtCallback->mTtl == 500);
    VerifyOrQuit(txtCallback->GetNext() == nullptr);

    VerifyOrQuit(sDnsMessages.IsEmpty());

    Log("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -");
    Log("Send an updated response with zero TTL. Validate callback result.");

    AdvanceTime(1000);

    sTxtCallbacks.Clear();

    SendTxtResponse("mysrv._srv._udp.local.", kEmptyTxtData, sizeof(kEmptyTxtData), 0, kInAnswerSection);

    AdvanceTime(1);

    VerifyOrQuit(!sTxtCallbacks.IsEmpty());
    txtCallback = sTxtCallbacks.GetHead();
    VerifyOrQuit(txtCallback->mServiceInstance.Matches("mysrv"));
    VerifyOrQuit(txtCallback->mServiceType.Matches("_srv._udp"));
    VerifyOrQuit(txtCallback->mTtl == 0);
    VerifyOrQuit(txtCallback->GetNext() == nullptr);

    VerifyOrQuit(sDnsMessages.IsEmpty());

    Log("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -");
    Log("Send an updated response. Validate callback result.");

    sTxtCallbacks.Clear();
    AdvanceTime(100 * 1000);

    SendTxtResponse("mysrv._srv._udp.local.", kTxtData1, sizeof(kTxtData1), 120, kInAnswerSection);

    AdvanceTime(1);

    VerifyOrQuit(!sTxtCallbacks.IsEmpty());
    txtCallback = sTxtCallbacks.GetHead();
    VerifyOrQuit(txtCallback->mServiceInstance.Matches("mysrv"));
    VerifyOrQuit(txtCallback->mServiceType.Matches("_srv._udp"));
    VerifyOrQuit(txtCallback->Matches(kTxtData1));
    VerifyOrQuit(txtCallback->mTtl == 120);
    VerifyOrQuit(txtCallback->GetNext() == nullptr);

    VerifyOrQuit(sDnsMessages.IsEmpty());

    Log("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -");
    Log("Send a response with no changes. Validate callback is not invoked.");

    AdvanceTime(1000);

    sTxtCallbacks.Clear();

    SendTxtResponse("mysrv._srv._udp.local.", kTxtData1, sizeof(kTxtData1), 120, kInAnswerSection);

    AdvanceTime(100);

    VerifyOrQuit(sTxtCallbacks.IsEmpty());
    VerifyOrQuit(sDnsMessages.IsEmpty());

    Log("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -");
    Log("Start another resolver for the same service and different callback. Validate results.");

    resolver2.mServiceInstance = "mysrv";
    resolver2.mServiceType     = "_srv._udp";
    resolver2.mInfraIfIndex    = kInfraIfIndex;
    resolver2.mCallback        = HandleTxtResultAlternate;

    sTxtCallbacks.Clear();

    SuccessOrQuit(mdns->StartTxtResolver(resolver2));

    AdvanceTime(1);

    VerifyOrQuit(!sTxtCallbacks.IsEmpty());
    txtCallback = sTxtCallbacks.GetHead();
    VerifyOrQuit(txtCallback->mServiceInstance.Matches("mysrv"));
    VerifyOrQuit(txtCallback->mServiceType.Matches("_srv._udp"));
    VerifyOrQuit(txtCallback->Matches(kTxtData1));
    VerifyOrQuit(txtCallback->mTtl == 120);
    VerifyOrQuit(txtCallback->GetNext() == nullptr);

    Log("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -");
    Log("Start same resolver again and check the returned error.");

    sTxtCallbacks.Clear();

    VerifyOrQuit(mdns->StartTxtResolver(resolver2) == kErrorAlready);

    AdvanceTime(5000);

    VerifyOrQuit(sTxtCallbacks.IsEmpty());

    Log("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -");
    Log("Check query is sent at 80 percentage of TTL and then respond to it.");

    SendTxtResponse("mysrv._srv._udp.local.", kTxtData1, sizeof(kTxtData1), 120, kInAnswerSection);

    // First query should be sent at 80-82% of TTL of 120 second (96.0-98.4 sec).
    // We wait for 100 second. Note that 5 seconds already passed in the
    // previous step.

    AdvanceTime(96 * 1000 - 1);

    VerifyOrQuit(sDnsMessages.IsEmpty());

    AdvanceTime(4 * 1000 + 1);

    VerifyOrQuit(!sDnsMessages.IsEmpty());
    dnsMsg = sDnsMessages.GetHead();
    dnsMsg->ValidateHeader(kMulticastQuery, /* Q */ 1, /* Ans */ 0, /* Auth */ 0, /* Addnl */ 0);
    dnsMsg->ValidateAsQueryFor(resolver);
    VerifyOrQuit(dnsMsg->GetNext() == nullptr);

    sDnsMessages.Clear();
    VerifyOrQuit(sTxtCallbacks.IsEmpty());

    AdvanceTime(10);

    SendTxtResponse("mysrv._srv._udp.local.", kTxtData1, sizeof(kTxtData1), 120, kInAnswerSection);

    Log("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -");
    Log("Check queries are sent at 80, 85, 90, 95 percentages of TTL.");

    for (uint8_t queryCount = 0; queryCount < kNumRefreshQueries; queryCount++)
    {
        if (queryCount == 0)
        {
            // First query is expected in 80-82% of TTL, so
            // 80% of 120 = 96.0, 82% of 120 = 98.4

            AdvanceTime(96 * 1000 - 1);
        }
        else
        {
            // Next query should happen within 3%-5% of TTL
            // from previous query. We wait 3% of TTL here.
            AdvanceTime(3600 - 1);
        }

        VerifyOrQuit(sDnsMessages.IsEmpty());

        // Wait for 2% of TTL of 120 which is 2.4 sec.

        AdvanceTime(2400 + 1);

        VerifyOrQuit(!sDnsMessages.IsEmpty());
        dnsMsg = sDnsMessages.GetHead();
        dnsMsg->ValidateHeader(kMulticastQuery, /* Q */ 1, /* Ans */ 0, /* Auth */ 0, /* Addnl */ 0);
        dnsMsg->ValidateAsQueryFor(resolver);
        VerifyOrQuit(dnsMsg->GetNext() == nullptr);

        sDnsMessages.Clear();
        VerifyOrQuit(sTxtCallbacks.IsEmpty());
    }

    Log("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -");
    Log("Check TTL timeout and callback result.");

    AdvanceTime(6 * 1000);

    txtCallback = sTxtCallbacks.GetHead();

    for (uint8_t iter = 0; iter < 2; iter++)
    {
        VerifyOrQuit(txtCallback != nullptr);
        VerifyOrQuit(txtCallback->mServiceInstance.Matches("mysrv"));
        VerifyOrQuit(txtCallback->mServiceType.Matches("_srv._udp"));
        VerifyOrQuit(txtCallback->mTtl == 0);
        txtCallback = txtCallback->GetNext();
    }

    VerifyOrQuit(txtCallback == nullptr);

    VerifyOrQuit(sDnsMessages.IsEmpty());

    Log("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -");

    sTxtCallbacks.Clear();
    sDnsMessages.Clear();

    AdvanceTime(200 * 1000);

    VerifyOrQuit(sTxtCallbacks.IsEmpty());
    VerifyOrQuit(sDnsMessages.IsEmpty());

    Log("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -");
    Log("Stop the second resolver");

    sTxtCallbacks.Clear();

    SuccessOrQuit(mdns->StopTxtResolver(resolver2));

    AdvanceTime(100 * 1000);

    VerifyOrQuit(sTxtCallbacks.IsEmpty());
    VerifyOrQuit(sDnsMessages.IsEmpty());

    Log("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -");
    Log("Send a new response and make sure result callback is invoked");

    SendTxtResponse("mysrv._srv._udp.local.", kTxtData1, sizeof(kTxtData1), 120, kInAnswerSection);

    AdvanceTime(1);

    VerifyOrQuit(!sTxtCallbacks.IsEmpty());
    txtCallback = sTxtCallbacks.GetHead();
    VerifyOrQuit(txtCallback->mServiceInstance.Matches("mysrv"));
    VerifyOrQuit(txtCallback->mServiceType.Matches("_srv._udp"));
    VerifyOrQuit(txtCallback->Matches(kTxtData1));
    VerifyOrQuit(txtCallback->mTtl == 120);
    VerifyOrQuit(txtCallback->GetNext() == nullptr);

    VerifyOrQuit(sDnsMessages.IsEmpty());

    Log("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -");
    Log("Stop the resolver. There is no active resolver. Ensure no queries are sent");

    sTxtCallbacks.Clear();

    SuccessOrQuit(mdns->StopTxtResolver(resolver));

    AdvanceTime(20 * 1000);

    VerifyOrQuit(sTxtCallbacks.IsEmpty());
    VerifyOrQuit(sDnsMessages.IsEmpty());

    Log("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -");
    Log("Restart the resolver with more than half of TTL remaining.");
    Log("Ensure cached entry is reported in the result callback and no queries are sent.");

    SuccessOrQuit(mdns->StartTxtResolver(resolver));

    AdvanceTime(1);

    VerifyOrQuit(!sTxtCallbacks.IsEmpty());
    txtCallback = sTxtCallbacks.GetHead();
    VerifyOrQuit(txtCallback->mServiceInstance.Matches("mysrv"));
    VerifyOrQuit(txtCallback->mServiceType.Matches("_srv._udp"));
    VerifyOrQuit(txtCallback->Matches(kTxtData1));
    VerifyOrQuit(txtCallback->mTtl == 120);
    VerifyOrQuit(txtCallback->GetNext() == nullptr);

    VerifyOrQuit(sDnsMessages.IsEmpty());

    AdvanceTime(20 * 1000);

    VerifyOrQuit(sDnsMessages.IsEmpty());

    Log("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -");
    Log("Stop and start the resolver again after less than half TTL remaining.");
    Log("Ensure cached entry is still reported in the result callback but queries should be sent");

    sTxtCallbacks.Clear();

    SuccessOrQuit(mdns->StopTxtResolver(resolver));

    AdvanceTime(25 * 1000);

    SuccessOrQuit(mdns->StartTxtResolver(resolver));

    AdvanceTime(1);

    VerifyOrQuit(!sTxtCallbacks.IsEmpty());
    txtCallback = sTxtCallbacks.GetHead();
    VerifyOrQuit(txtCallback->mServiceInstance.Matches("mysrv"));
    VerifyOrQuit(txtCallback->mServiceType.Matches("_srv._udp"));
    VerifyOrQuit(txtCallback->Matches(kTxtData1));
    VerifyOrQuit(txtCallback->mTtl == 120);
    VerifyOrQuit(txtCallback->GetNext() == nullptr);

    sTxtCallbacks.Clear();

    AdvanceTime(15 * 1000);

    dnsMsg = sDnsMessages.GetHead();

    for (uint8_t queryCount = 0; queryCount < kNumInitalQueries; queryCount++)
    {
        VerifyOrQuit(dnsMsg != nullptr);
        dnsMsg->ValidateHeader(kMulticastQuery, /* Q */ 1, /* Ans */ 0, /* Auth */ 0, /* Addnl */ 0);
        dnsMsg->ValidateAsQueryFor(resolver);
        dnsMsg = dnsMsg->GetNext();
    }

    VerifyOrQuit(dnsMsg == nullptr);

    Log("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -");

    SuccessOrQuit(mdns->SetEnabled(false, kInfraIfIndex));
    VerifyOrQuit(sHeapAllocatedPtrs.GetLength() <= heapAllocations);

    Log("End of test");

    testFreeInstance(sInstance);
}

void TestIp6AddrResolver(void)
{
    Core                 *mdns = InitTest();
    Core::AddressResolver resolver;
    Core::AddressResolver resolver2;
    AddrAndTtl            addrs[5];
    const DnsMessage     *dnsMsg;
    const AddrCallback   *addrCallback;
    uint16_t              heapAllocations;

    Log("-------------------------------------------------------------------------------------------");
    Log("TestIp6AddrResolver");

    AdvanceTime(1);

    heapAllocations = sHeapAllocatedPtrs.GetLength();
    SuccessOrQuit(mdns->SetEnabled(true, kInfraIfIndex));

    Log("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -");
    Log("Start an IPv6 address resolver. Validate initial queries.");

    ClearAllBytes(resolver);

    resolver.mHostName     = "myhost";
    resolver.mInfraIfIndex = kInfraIfIndex;
    resolver.mCallback     = HandleAddrResult;

    sDnsMessages.Clear();
    SuccessOrQuit(mdns->StartIp6AddressResolver(resolver));

    for (uint8_t queryCount = 0; queryCount < kNumInitalQueries; queryCount++)
    {
        sDnsMessages.Clear();

        AdvanceTime((queryCount == 0) ? 125 : (1U << (queryCount - 1)) * 1000);

        VerifyOrQuit(!sDnsMessages.IsEmpty());
        dnsMsg = sDnsMessages.GetHead();
        dnsMsg->ValidateHeader(kMulticastQuery, /* Q */ 1, /* Ans */ 0, /* Auth */ 0, /* Addnl */ 0);
        dnsMsg->ValidateAsQueryFor(resolver);
        VerifyOrQuit(dnsMsg->GetNext() == nullptr);
    }

    sDnsMessages.Clear();

    AdvanceTime(20 * 1000);
    VerifyOrQuit(sDnsMessages.IsEmpty());

    Log("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -");
    Log("Send a response. Validate callback result.");

    sAddrCallbacks.Clear();

    SuccessOrQuit(addrs[0].mAddress.FromString("fd00::1"));
    addrs[0].mTtl = 120;

    SendHostAddrResponse("myhost.local.", addrs, 1, /* aCachFlush */ true, kInAnswerSection);

    AdvanceTime(1);

    VerifyOrQuit(!sAddrCallbacks.IsEmpty());
    addrCallback = sAddrCallbacks.GetHead();
    VerifyOrQuit(addrCallback->mHostName.Matches("myhost"));
    VerifyOrQuit(addrCallback->Matches(addrs, 1));
    VerifyOrQuit(addrCallback->GetNext() == nullptr);

    VerifyOrQuit(sDnsMessages.IsEmpty());

    Log("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -");
    Log("Send an updated response adding a new address. Validate callback result.");

    SuccessOrQuit(addrs[1].mAddress.FromString("fd00::2"));
    addrs[1].mTtl = 120;

    AdvanceTime(1000);

    sAddrCallbacks.Clear();

    SendHostAddrResponse("myhost.local.", addrs, 2, /* aCachFlush */ true, kInAnswerSection);

    AdvanceTime(1);

    VerifyOrQuit(!sAddrCallbacks.IsEmpty());
    addrCallback = sAddrCallbacks.GetHead();
    VerifyOrQuit(addrCallback->mHostName.Matches("myhost"));
    VerifyOrQuit(addrCallback->Matches(addrs, 2));
    VerifyOrQuit(addrCallback->GetNext() == nullptr);

    VerifyOrQuit(sDnsMessages.IsEmpty());

    Log("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -");
    Log("Send an updated response adding and removing addresses. Validate callback result.");

    SuccessOrQuit(addrs[0].mAddress.FromString("fd00::2"));
    SuccessOrQuit(addrs[1].mAddress.FromString("fd00::aa"));
    SuccessOrQuit(addrs[2].mAddress.FromString("fe80::bb"));
    addrs[0].mTtl = 120;
    addrs[1].mTtl = 120;
    addrs[2].mTtl = 120;

    AdvanceTime(1000);

    sAddrCallbacks.Clear();

    SendHostAddrResponse("myhost.local.", addrs, 3, /* aCachFlush */ true, kInAnswerSection);

    AdvanceTime(1);

    VerifyOrQuit(!sAddrCallbacks.IsEmpty());
    addrCallback = sAddrCallbacks.GetHead();
    VerifyOrQuit(addrCallback->mHostName.Matches("myhost"));
    VerifyOrQuit(addrCallback->Matches(addrs, 3));
    VerifyOrQuit(addrCallback->GetNext() == nullptr);

    VerifyOrQuit(sDnsMessages.IsEmpty());

    Log("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -");
    Log("Send a response without cache flush adding an address. Validate callback result.");

    SuccessOrQuit(addrs[3].mAddress.FromString("fd00::3"));
    addrs[3].mTtl = 500;

    AdvanceTime(1000);

    sAddrCallbacks.Clear();

    SendHostAddrResponse("myhost.local.", &addrs[3], 1, /* aCachFlush */ false, kInAnswerSection);

    AdvanceTime(1);

    VerifyOrQuit(!sAddrCallbacks.IsEmpty());
    addrCallback = sAddrCallbacks.GetHead();
    VerifyOrQuit(addrCallback->mHostName.Matches("myhost"));
    VerifyOrQuit(addrCallback->Matches(addrs, 4));
    VerifyOrQuit(addrCallback->GetNext() == nullptr);

    VerifyOrQuit(sDnsMessages.IsEmpty());

    Log("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -");
    Log("Send a response without cache flush with existing addresses. Validate that callback is not called.");

    AdvanceTime(1000);

    sAddrCallbacks.Clear();

    SendHostAddrResponse("myhost.local.", &addrs[2], 2, /* aCachFlush */ false, kInAnswerSection);

    AdvanceTime(1);

    VerifyOrQuit(sAddrCallbacks.IsEmpty());
    VerifyOrQuit(sDnsMessages.IsEmpty());

    Log("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -");
    Log("Send a response without no changes to the list. Validate that callback is not called");

    AdvanceTime(1000);

    sAddrCallbacks.Clear();

    SendHostAddrResponse("myhost.local.", addrs, 4, /* aCachFlush */ true, kInAdditionalSection);

    AdvanceTime(1);

    VerifyOrQuit(sAddrCallbacks.IsEmpty());
    VerifyOrQuit(sDnsMessages.IsEmpty());

    Log("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -");
    Log("Send a response without cache flush updating TTL of existing address. Validate callback result.");

    addrs[3].mTtl = 200;

    AdvanceTime(1000);

    sAddrCallbacks.Clear();

    SendHostAddrResponse("myhost.local.", &addrs[3], 1, /* aCachFlush */ false, kInAnswerSection);

    AdvanceTime(1);

    VerifyOrQuit(!sAddrCallbacks.IsEmpty());
    addrCallback = sAddrCallbacks.GetHead();
    VerifyOrQuit(addrCallback->mHostName.Matches("myhost"));
    VerifyOrQuit(addrCallback->Matches(addrs, 4));
    VerifyOrQuit(addrCallback->GetNext() == nullptr);

    VerifyOrQuit(sDnsMessages.IsEmpty());

    Log("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -");
    Log("Send a response without cache flush removing an address (zero TTL). Validate callback result.");

    addrs[3].mTtl = 0;

    AdvanceTime(1000);

    sAddrCallbacks.Clear();

    SendHostAddrResponse("myhost.local.", &addrs[3], 1, /* aCachFlush */ false, kInAnswerSection);

    AdvanceTime(1);

    VerifyOrQuit(!sAddrCallbacks.IsEmpty());
    addrCallback = sAddrCallbacks.GetHead();
    VerifyOrQuit(addrCallback->mHostName.Matches("myhost"));
    VerifyOrQuit(addrCallback->Matches(addrs, 3));
    VerifyOrQuit(addrCallback->GetNext() == nullptr);

    VerifyOrQuit(sDnsMessages.IsEmpty());

    Log("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -");
    Log("Send a response with cache flush removing all addresses. Validate callback result.");

    addrs[0].mTtl = 0;

    AdvanceTime(1000);

    sAddrCallbacks.Clear();

    SendHostAddrResponse("myhost.local.", addrs, 1, /* aCachFlush */ true, kInAnswerSection);

    AdvanceTime(1);

    VerifyOrQuit(!sAddrCallbacks.IsEmpty());
    addrCallback = sAddrCallbacks.GetHead();
    VerifyOrQuit(addrCallback->mHostName.Matches("myhost"));
    VerifyOrQuit(addrCallback->Matches(addrs, 0));
    VerifyOrQuit(addrCallback->GetNext() == nullptr);

    VerifyOrQuit(sDnsMessages.IsEmpty());

    Log("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -");
    Log("Send a response with addresses with different TTL. Validate callback result");

    SuccessOrQuit(addrs[0].mAddress.FromString("fd00::00"));
    SuccessOrQuit(addrs[1].mAddress.FromString("fd00::11"));
    SuccessOrQuit(addrs[2].mAddress.FromString("fe80::22"));
    SuccessOrQuit(addrs[3].mAddress.FromString("fe80::33"));
    addrs[0].mTtl = 120;
    addrs[1].mTtl = 800;
    addrs[2].mTtl = 2000;
    addrs[3].mTtl = 8000;

    AdvanceTime(5 * 1000);

    sAddrCallbacks.Clear();

    SendHostAddrResponse("myhost.local.", addrs, 4, /* aCachFlush */ true, kInAnswerSection);

    AdvanceTime(1);

    VerifyOrQuit(!sAddrCallbacks.IsEmpty());
    addrCallback = sAddrCallbacks.GetHead();
    VerifyOrQuit(addrCallback->mHostName.Matches("myhost"));
    VerifyOrQuit(addrCallback->Matches(addrs, 4));
    VerifyOrQuit(addrCallback->GetNext() == nullptr);

    VerifyOrQuit(sDnsMessages.IsEmpty());

    Log("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -");
    Log("Start another resolver for the same host and different callback. Validate results.");

    resolver2.mHostName     = "myhost";
    resolver2.mInfraIfIndex = kInfraIfIndex;
    resolver2.mCallback     = HandleAddrResultAlternate;

    sAddrCallbacks.Clear();

    SuccessOrQuit(mdns->StartIp6AddressResolver(resolver2));

    AdvanceTime(1);

    VerifyOrQuit(!sAddrCallbacks.IsEmpty());
    addrCallback = sAddrCallbacks.GetHead();
    VerifyOrQuit(addrCallback->mHostName.Matches("myhost"));
    VerifyOrQuit(addrCallback->Matches(addrs, 4));
    VerifyOrQuit(addrCallback->GetNext() == nullptr);

    VerifyOrQuit(sDnsMessages.IsEmpty());

    Log("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -");
    Log("Start same resolver again and check the returned error.");

    sAddrCallbacks.Clear();

    VerifyOrQuit(mdns->StartIp6AddressResolver(resolver2) == kErrorAlready);

    AdvanceTime(5000);

    VerifyOrQuit(sAddrCallbacks.IsEmpty());
    sDnsMessages.Clear();

    Log("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -");
    Log("Check query is sent at 80 percentage of TTL and then respond to it.");

    SendHostAddrResponse("myhost.local.", addrs, 4, /* aCachFlush */ true, kInAnswerSection);

    // First query should be sent at 80-82% of TTL of 120 second (96.0-98.4 sec).
    // We wait for 100 second. Note that 5 seconds already passed in the
    // previous step.

    AdvanceTime(96 * 1000 - 1);

    VerifyOrQuit(sDnsMessages.IsEmpty());

    AdvanceTime(4 * 1000 + 1);

    VerifyOrQuit(!sDnsMessages.IsEmpty());
    dnsMsg = sDnsMessages.GetHead();
    dnsMsg->ValidateHeader(kMulticastQuery, /* Q */ 1, /* Ans */ 0, /* Auth */ 0, /* Addnl */ 0);
    dnsMsg->ValidateAsQueryFor(resolver);
    VerifyOrQuit(dnsMsg->GetNext() == nullptr);

    sDnsMessages.Clear();
    VerifyOrQuit(sAddrCallbacks.IsEmpty());

    AdvanceTime(10);

    SendHostAddrResponse("myhost.local.", addrs, 4, /* aCachFlush */ true, kInAnswerSection);

    Log("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -");
    Log("Check queries are sent at 80, 85, 90, 95 percentages of TTL.");

    for (uint8_t queryCount = 0; queryCount < kNumRefreshQueries; queryCount++)
    {
        if (queryCount == 0)
        {
            // First query is expected in 80-82% of TTL, so
            // 80% of 120 = 96.0, 82% of 120 = 98.4

            AdvanceTime(96 * 1000 - 1);
        }
        else
        {
            // Next query should happen within 3%-5% of TTL
            // from previous query. We wait 3% of TTL here.
            AdvanceTime(3600 - 1);
        }

        VerifyOrQuit(sDnsMessages.IsEmpty());

        // Wait for 2% of TTL of 120 which is 2.4 sec.

        AdvanceTime(2400 + 1);

        VerifyOrQuit(!sDnsMessages.IsEmpty());
        dnsMsg = sDnsMessages.GetHead();
        dnsMsg->ValidateHeader(kMulticastQuery, /* Q */ 1, /* Ans */ 0, /* Auth */ 0, /* Addnl */ 0);
        dnsMsg->ValidateAsQueryFor(resolver);
        VerifyOrQuit(dnsMsg->GetNext() == nullptr);

        sDnsMessages.Clear();
        VerifyOrQuit(sAddrCallbacks.IsEmpty());
    }

    Log("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -");
    Log("Check TTL timeout of first address (TTL 120) and callback result.");

    AdvanceTime(6 * 1000);

    addrCallback = sAddrCallbacks.GetHead();

    for (uint8_t iter = 0; iter < 2; iter++)
    {
        VerifyOrQuit(addrCallback != nullptr);
        VerifyOrQuit(addrCallback->mHostName.Matches("myhost"));
        VerifyOrQuit(addrCallback->Matches(&addrs[1], 3));
        addrCallback = addrCallback->GetNext();
    }

    VerifyOrQuit(addrCallback == nullptr);

    VerifyOrQuit(sDnsMessages.IsEmpty());

    Log("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -");
    Log("Check TTL timeout of next address (TTL 800) and callback result.");

    sAddrCallbacks.Clear();

    AdvanceTime((800 - 120) * 1000);

    addrCallback = sAddrCallbacks.GetHead();

    for (uint8_t iter = 0; iter < 2; iter++)
    {
        VerifyOrQuit(addrCallback != nullptr);
        VerifyOrQuit(addrCallback->mHostName.Matches("myhost"));
        VerifyOrQuit(addrCallback->Matches(&addrs[2], 2));
        addrCallback = addrCallback->GetNext();
    }

    VerifyOrQuit(addrCallback == nullptr);

    Log("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -");

    sAddrCallbacks.Clear();
    sDnsMessages.Clear();

    AdvanceTime(200 * 1000);

    VerifyOrQuit(sAddrCallbacks.IsEmpty());
    VerifyOrQuit(sDnsMessages.IsEmpty());

    Log("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -");
    Log("Stop the second resolver");

    sAddrCallbacks.Clear();

    SuccessOrQuit(mdns->StopIp6AddressResolver(resolver2));

    AdvanceTime(100 * 1000);

    VerifyOrQuit(sAddrCallbacks.IsEmpty());
    VerifyOrQuit(sDnsMessages.IsEmpty());

    Log("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -");
    Log("Send a new response and make sure result callback is invoked");

    sAddrCallbacks.Clear();

    SendHostAddrResponse("myhost.local.", addrs, 1, /* aCachFlush */ true, kInAnswerSection);

    AdvanceTime(1);

    VerifyOrQuit(!sAddrCallbacks.IsEmpty());
    addrCallback = sAddrCallbacks.GetHead();
    VerifyOrQuit(addrCallback->mHostName.Matches("myhost"));
    VerifyOrQuit(addrCallback->Matches(addrs, 1));
    VerifyOrQuit(addrCallback->GetNext() == nullptr);

    VerifyOrQuit(sDnsMessages.IsEmpty());

    Log("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -");
    Log("Stop the resolver. There is no active resolver. Ensure no queries are sent");

    sAddrCallbacks.Clear();

    SuccessOrQuit(mdns->StopIp6AddressResolver(resolver));

    AdvanceTime(20 * 1000);

    VerifyOrQuit(sAddrCallbacks.IsEmpty());
    VerifyOrQuit(sDnsMessages.IsEmpty());

    Log("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -");
    Log("Restart the resolver with more than half of TTL remaining.");
    Log("Ensure cached entry is reported in the result callback and no queries are sent.");

    SuccessOrQuit(mdns->StartIp6AddressResolver(resolver));

    AdvanceTime(1);

    VerifyOrQuit(!sAddrCallbacks.IsEmpty());
    addrCallback = sAddrCallbacks.GetHead();
    VerifyOrQuit(addrCallback->mHostName.Matches("myhost"));
    VerifyOrQuit(addrCallback->Matches(addrs, 1));
    VerifyOrQuit(addrCallback->GetNext() == nullptr);

    VerifyOrQuit(sDnsMessages.IsEmpty());

    AdvanceTime(20 * 1000);

    VerifyOrQuit(sDnsMessages.IsEmpty());

    Log("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -");
    Log("Stop and start the resolver again after less than half TTL remaining.");
    Log("Ensure cached entry is still reported in the result callback but queries should be sent");

    sAddrCallbacks.Clear();

    SuccessOrQuit(mdns->StopIp6AddressResolver(resolver));

    AdvanceTime(25 * 1000);

    SuccessOrQuit(mdns->StartIp6AddressResolver(resolver));

    AdvanceTime(1);

    VerifyOrQuit(!sAddrCallbacks.IsEmpty());
    addrCallback = sAddrCallbacks.GetHead();
    VerifyOrQuit(addrCallback->mHostName.Matches("myhost"));
    VerifyOrQuit(addrCallback->Matches(addrs, 1));
    VerifyOrQuit(addrCallback->GetNext() == nullptr);

    sAddrCallbacks.Clear();

    AdvanceTime(15 * 1000);

    dnsMsg = sDnsMessages.GetHead();

    for (uint8_t queryCount = 0; queryCount < kNumInitalQueries; queryCount++)
    {
        VerifyOrQuit(dnsMsg != nullptr);
        dnsMsg->ValidateHeader(kMulticastQuery, /* Q */ 1, /* Ans */ 0, /* Auth */ 0, /* Addnl */ 0);
        dnsMsg->ValidateAsQueryFor(resolver);
        dnsMsg = dnsMsg->GetNext();
    }

    VerifyOrQuit(dnsMsg == nullptr);

    Log("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -");

    SuccessOrQuit(mdns->SetEnabled(false, kInfraIfIndex));
    VerifyOrQuit(sHeapAllocatedPtrs.GetLength() <= heapAllocations);

    Log("End of test");

    testFreeInstance(sInstance);
}

void TestPassiveCache(void)
{
    static const char *const kSubTypes[] = {"_sub1", "_xyzw"};

    Core                 *mdns = InitTest();
    Core::Browser         browser;
    Core::SrvResolver     srvResolver;
    Core::TxtResolver     txtResolver;
    Core::AddressResolver addrResolver;
    Core::Host            host1;
    Core::Host            host2;
    Core::Service         service1;
    Core::Service         service2;
    Core::Service         service3;
    Ip6::Address          host1Addresses[3];
    Ip6::Address          host2Addresses[2];
    AddrAndTtl            host1AddrTtls[3];
    AddrAndTtl            host2AddrTtls[2];
    const DnsMessage     *dnsMsg;
    BrowseCallback       *browseCallback;
    SrvCallback          *srvCallback;
    TxtCallback          *txtCallback;
    AddrCallback         *addrCallback;
    uint16_t              heapAllocations;

    Log("-------------------------------------------------------------------------------------------");
    Log("TestPassiveCache");

    AdvanceTime(1);

    heapAllocations = sHeapAllocatedPtrs.GetLength();
    SuccessOrQuit(mdns->SetEnabled(true, kInfraIfIndex));

    SuccessOrQuit(host1Addresses[0].FromString("fd00::1:aaaa"));
    SuccessOrQuit(host1Addresses[1].FromString("fd00::1:bbbb"));
    SuccessOrQuit(host1Addresses[2].FromString("fd00::1:cccc"));
    host1.mHostName        = "host1";
    host1.mAddresses       = host1Addresses;
    host1.mAddressesLength = 3;
    host1.mTtl             = 1500;

    host1AddrTtls[0].mAddress = host1Addresses[0];
    host1AddrTtls[1].mAddress = host1Addresses[1];
    host1AddrTtls[2].mAddress = host1Addresses[2];
    host1AddrTtls[0].mTtl     = host1.mTtl;
    host1AddrTtls[1].mTtl     = host1.mTtl;
    host1AddrTtls[2].mTtl     = host1.mTtl;

    SuccessOrQuit(host2Addresses[0].FromString("fd00::2:eeee"));
    SuccessOrQuit(host2Addresses[1].FromString("fd00::2:ffff"));
    host2.mHostName        = "host2";
    host2.mAddresses       = host2Addresses;
    host2.mAddressesLength = 2;
    host2.mTtl             = 1500;

    host2AddrTtls[0].mAddress = host2Addresses[0];
    host2AddrTtls[1].mAddress = host2Addresses[1];
    host2AddrTtls[0].mTtl     = host2.mTtl;
    host2AddrTtls[1].mTtl     = host2.mTtl;

    service1.mHostName            = host1.mHostName;
    service1.mServiceInstance     = "srv1";
    service1.mServiceType         = "_srv._udp";
    service1.mSubTypeLabels       = kSubTypes;
    service1.mSubTypeLabelsLength = 2;
    service1.mTxtData             = kTxtData1;
    service1.mTxtDataLength       = sizeof(kTxtData1);
    service1.mPort                = 1111;
    service1.mPriority            = 0;
    service1.mWeight              = 0;
    service1.mTtl                 = 1500;

    service2.mHostName            = host1.mHostName;
    service2.mServiceInstance     = "srv2";
    service2.mServiceType         = "_tst._tcp";
    service2.mSubTypeLabels       = nullptr;
    service2.mSubTypeLabelsLength = 0;
    service2.mTxtData             = nullptr;
    service2.mTxtDataLength       = 0;
    service2.mPort                = 2222;
    service2.mPriority            = 2;
    service2.mWeight              = 2;
    service2.mTtl                 = 1500;

    service3.mHostName            = host2.mHostName;
    service3.mServiceInstance     = "srv3";
    service3.mServiceType         = "_srv._udp";
    service3.mSubTypeLabels       = kSubTypes;
    service3.mSubTypeLabelsLength = 1;
    service3.mTxtData             = kTxtData2;
    service3.mTxtDataLength       = sizeof(kTxtData2);
    service3.mPort                = 3333;
    service3.mPriority            = 3;
    service3.mWeight              = 3;
    service3.mTtl                 = 1500;

    Log("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -");
    Log("Register 2 hosts and 3 services");

    SuccessOrQuit(mdns->RegisterHost(host1, 0, HandleSuccessCallback));
    SuccessOrQuit(mdns->RegisterHost(host2, 1, HandleSuccessCallback));
    SuccessOrQuit(mdns->RegisterService(service1, 2, HandleSuccessCallback));
    SuccessOrQuit(mdns->RegisterService(service2, 3, HandleSuccessCallback));
    SuccessOrQuit(mdns->RegisterService(service3, 4, HandleSuccessCallback));

    AdvanceTime(10 * 1000);

    Log("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -");
    Log("Start a browser for `_srv._udp`, validate callback result");

    browser.mServiceType  = "_srv._udp";
    browser.mSubTypeLabel = nullptr;
    browser.mInfraIfIndex = kInfraIfIndex;
    browser.mCallback     = HandleBrowseResult;

    sBrowseCallbacks.Clear();

    SuccessOrQuit(mdns->StartBrowser(browser));

    AdvanceTime(350);

    browseCallback = sBrowseCallbacks.GetHead();

    for (uint8_t iter = 0; iter < 2; iter++)
    {
        VerifyOrQuit(browseCallback != nullptr);

        VerifyOrQuit(browseCallback->mServiceType.Matches("_srv._udp"));
        VerifyOrQuit(!browseCallback->mIsSubType);
        VerifyOrQuit(browseCallback->mServiceInstance.Matches("srv1") ||
                     browseCallback->mServiceInstance.Matches("srv3"));
        VerifyOrQuit(browseCallback->mTtl == 1500);

        browseCallback = browseCallback->GetNext();
    }

    VerifyOrQuit(browseCallback == nullptr);

    Log("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -");
    Log("Start SRV and TXT resolvers for the srv1 and for its host name.");
    Log("Ensure all results are immediately provided from cache.");

    srvResolver.mServiceInstance = "srv1";
    srvResolver.mServiceType     = "_srv._udp";
    srvResolver.mInfraIfIndex    = kInfraIfIndex;
    srvResolver.mCallback        = HandleSrvResult;

    txtResolver.mServiceInstance = "srv1";
    txtResolver.mServiceType     = "_srv._udp";
    txtResolver.mInfraIfIndex    = kInfraIfIndex;
    txtResolver.mCallback        = HandleTxtResult;

    addrResolver.mHostName     = "host1";
    addrResolver.mInfraIfIndex = kInfraIfIndex;
    addrResolver.mCallback     = HandleAddrResult;

    sSrvCallbacks.Clear();
    sTxtCallbacks.Clear();
    sAddrCallbacks.Clear();
    sDnsMessages.Clear();

    SuccessOrQuit(mdns->StartSrvResolver(srvResolver));
    SuccessOrQuit(mdns->StartTxtResolver(txtResolver));
    SuccessOrQuit(mdns->StartIp6AddressResolver(addrResolver));

    AdvanceTime(1);

    VerifyOrQuit(!sSrvCallbacks.IsEmpty());
    srvCallback = sSrvCallbacks.GetHead();
    VerifyOrQuit(srvCallback->mServiceInstance.Matches("srv1"));
    VerifyOrQuit(srvCallback->mServiceType.Matches("_srv._udp"));
    VerifyOrQuit(srvCallback->mHostName.Matches("host1"));
    VerifyOrQuit(srvCallback->mPort == 1111);
    VerifyOrQuit(srvCallback->mPriority == 0);
    VerifyOrQuit(srvCallback->mWeight == 0);
    VerifyOrQuit(srvCallback->mTtl == 1500);
    VerifyOrQuit(srvCallback->GetNext() == nullptr);

    VerifyOrQuit(!sTxtCallbacks.IsEmpty());
    txtCallback = sTxtCallbacks.GetHead();
    VerifyOrQuit(txtCallback->mServiceInstance.Matches("srv1"));
    VerifyOrQuit(txtCallback->mServiceType.Matches("_srv._udp"));
    VerifyOrQuit(txtCallback->Matches(kTxtData1));
    VerifyOrQuit(txtCallback->mTtl == 1500);
    VerifyOrQuit(txtCallback->GetNext() == nullptr);

    VerifyOrQuit(!sAddrCallbacks.IsEmpty());
    addrCallback = sAddrCallbacks.GetHead();
    VerifyOrQuit(addrCallback->mHostName.Matches("host1"));
    VerifyOrQuit(addrCallback->Matches(host1AddrTtls, 3));
    VerifyOrQuit(addrCallback->GetNext() == nullptr);

    AdvanceTime(400);

    VerifyOrQuit(sDnsMessages.IsEmpty());

    Log("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -");
    Log("Start a browser for sub-type service, validate callback result");

    browser.mServiceType  = "_srv._udp";
    browser.mSubTypeLabel = "_xyzw";
    browser.mInfraIfIndex = kInfraIfIndex;
    browser.mCallback     = HandleBrowseResult;

    sBrowseCallbacks.Clear();

    SuccessOrQuit(mdns->StartBrowser(browser));

    AdvanceTime(350);

    browseCallback = sBrowseCallbacks.GetHead();
    VerifyOrQuit(browseCallback != nullptr);

    VerifyOrQuit(browseCallback->mServiceType.Matches("_srv._udp"));
    VerifyOrQuit(browseCallback->mIsSubType);
    VerifyOrQuit(browseCallback->mSubTypeLabel.Matches("_xyzw"));
    VerifyOrQuit(browseCallback->mServiceInstance.Matches("srv1"));
    VerifyOrQuit(browseCallback->mTtl == 1500);
    VerifyOrQuit(browseCallback->GetNext() == nullptr);

    AdvanceTime(5 * 1000);

    Log("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -");
    Log("Start SRV and TXT resolvers for `srv2._tst._tcp` service and validate callback result");

    srvResolver.mServiceInstance = "srv2";
    srvResolver.mServiceType     = "_tst._tcp";
    srvResolver.mInfraIfIndex    = kInfraIfIndex;
    srvResolver.mCallback        = HandleSrvResult;

    txtResolver.mServiceInstance = "srv2";
    txtResolver.mServiceType     = "_tst._tcp";
    txtResolver.mInfraIfIndex    = kInfraIfIndex;
    txtResolver.mCallback        = HandleTxtResult;

    sSrvCallbacks.Clear();
    sTxtCallbacks.Clear();

    SuccessOrQuit(mdns->StartSrvResolver(srvResolver));
    SuccessOrQuit(mdns->StartTxtResolver(txtResolver));

    AdvanceTime(350);

    VerifyOrQuit(!sSrvCallbacks.IsEmpty());
    srvCallback = sSrvCallbacks.GetHead();
    VerifyOrQuit(srvCallback->mServiceInstance.Matches("srv2"));
    VerifyOrQuit(srvCallback->mServiceType.Matches("_tst._tcp"));
    VerifyOrQuit(srvCallback->mHostName.Matches("host1"));
    VerifyOrQuit(srvCallback->mPort == 2222);
    VerifyOrQuit(srvCallback->mPriority == 2);
    VerifyOrQuit(srvCallback->mWeight == 2);
    VerifyOrQuit(srvCallback->mTtl == 1500);
    VerifyOrQuit(srvCallback->GetNext() == nullptr);

    VerifyOrQuit(!sTxtCallbacks.IsEmpty());
    txtCallback = sTxtCallbacks.GetHead();
    VerifyOrQuit(txtCallback->mServiceInstance.Matches("srv2"));
    VerifyOrQuit(txtCallback->mServiceType.Matches("_tst._tcp"));
    VerifyOrQuit(txtCallback->Matches(kEmptyTxtData));
    VerifyOrQuit(txtCallback->mTtl == 1500);
    VerifyOrQuit(txtCallback->GetNext() == nullptr);

    Log("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -");
    Log("Unregister `srv2._tst._tcp` and validate callback results");

    sSrvCallbacks.Clear();
    sTxtCallbacks.Clear();

    SuccessOrQuit(mdns->UnregisterService(service2));

    AdvanceTime(350);

    VerifyOrQuit(!sSrvCallbacks.IsEmpty());
    srvCallback = sSrvCallbacks.GetHead();
    VerifyOrQuit(srvCallback->mServiceInstance.Matches("srv2"));
    VerifyOrQuit(srvCallback->mServiceType.Matches("_tst._tcp"));
    VerifyOrQuit(srvCallback->mTtl == 0);
    VerifyOrQuit(srvCallback->GetNext() == nullptr);

    VerifyOrQuit(!sTxtCallbacks.IsEmpty());
    txtCallback = sTxtCallbacks.GetHead();
    VerifyOrQuit(txtCallback->mServiceInstance.Matches("srv2"));
    VerifyOrQuit(txtCallback->mServiceType.Matches("_tst._tcp"));
    VerifyOrQuit(txtCallback->mTtl == 0);
    VerifyOrQuit(txtCallback->GetNext() == nullptr);

    Log("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -");
    Log("Start an SRV resolver for `srv3._srv._udp` service and validate callback result");

    srvResolver.mServiceInstance = "srv3";
    srvResolver.mServiceType     = "_srv._udp";
    srvResolver.mInfraIfIndex    = kInfraIfIndex;
    srvResolver.mCallback        = HandleSrvResult;

    sSrvCallbacks.Clear();

    SuccessOrQuit(mdns->StartSrvResolver(srvResolver));

    AdvanceTime(350);

    VerifyOrQuit(!sSrvCallbacks.IsEmpty());
    srvCallback = sSrvCallbacks.GetHead();
    VerifyOrQuit(srvCallback->mServiceInstance.Matches("srv3"));
    VerifyOrQuit(srvCallback->mServiceType.Matches("_srv._udp"));
    VerifyOrQuit(srvCallback->mHostName.Matches("host2"));
    VerifyOrQuit(srvCallback->mPort == 3333);
    VerifyOrQuit(srvCallback->mPriority == 3);
    VerifyOrQuit(srvCallback->mWeight == 3);
    VerifyOrQuit(srvCallback->mTtl == 1500);
    VerifyOrQuit(srvCallback->GetNext() == nullptr);

    Log("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -");
    Log("Start an address resolver for host2 and validate result is immediately reported from cache");

    addrResolver.mHostName     = "host2";
    addrResolver.mInfraIfIndex = kInfraIfIndex;
    addrResolver.mCallback     = HandleAddrResult;

    sAddrCallbacks.Clear();
    SuccessOrQuit(mdns->StartIp6AddressResolver(addrResolver));

    AdvanceTime(1);

    VerifyOrQuit(!sAddrCallbacks.IsEmpty());
    addrCallback = sAddrCallbacks.GetHead();
    VerifyOrQuit(addrCallback->mHostName.Matches("host2"));
    VerifyOrQuit(addrCallback->Matches(host2AddrTtls, 2));
    VerifyOrQuit(addrCallback->GetNext() == nullptr);

    Log("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -");

    SuccessOrQuit(mdns->SetEnabled(false, kInfraIfIndex));
    VerifyOrQuit(sHeapAllocatedPtrs.GetLength() <= heapAllocations);

    Log("End of test");

    testFreeInstance(sInstance);
}

void TestLegacyUnicastResponse(void)
{
    Core             *mdns = InitTest();
    Core::Host        host;
    Core::Service     service;
    const DnsMessage *dnsMsg;
    uint16_t          heapAllocations;
    DnsNameString     fullServiceName;
    DnsNameString     fullServiceType;
    DnsNameString     hostFullName;
    Ip6::Address      hostAddresses[2];

    Log("-------------------------------------------------------------------------------------------");
    Log("TestLegacyUnicastResponse");

    AdvanceTime(1);

    heapAllocations = sHeapAllocatedPtrs.GetLength();
    SuccessOrQuit(mdns->SetEnabled(true, kInfraIfIndex));

    SuccessOrQuit(hostAddresses[0].FromString("fd00::1:aaaa"));
    SuccessOrQuit(hostAddresses[1].FromString("fd00::1:bbbb"));
    host.mHostName        = "host";
    host.mAddresses       = hostAddresses;
    host.mAddressesLength = 2;
    host.mTtl             = 1500;
    hostFullName.Append("%s.local.", host.mHostName);

    service.mHostName            = host.mHostName;
    service.mServiceInstance     = "myservice";
    service.mServiceType         = "_srv._udp";
    service.mSubTypeLabels       = nullptr;
    service.mSubTypeLabelsLength = 0;
    service.mTxtData             = kTxtData1;
    service.mTxtDataLength       = sizeof(kTxtData1);
    service.mPort                = 1234;
    service.mPriority            = 1;
    service.mWeight              = 2;
    service.mTtl                 = 1000;

    fullServiceName.Append("%s.%s.local.", service.mServiceInstance, service.mServiceType);
    fullServiceType.Append("%s.local.", service.mServiceType);

    Log("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -");

    sDnsMessages.Clear();

    for (RegCallback &regCallbck : sRegCallbacks)
    {
        regCallbck.Reset();
    }

    SuccessOrQuit(mdns->RegisterHost(host, 0, HandleSuccessCallback));
    SuccessOrQuit(mdns->RegisterService(service, 1, HandleSuccessCallback));

    AdvanceTime(10 * 1000);

    Log("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -");
    Log("Send a query with two questions (SRV for service1 and AAAA for host). Validate that no response is sent");

    AdvanceTime(2000);

    sDnsMessages.Clear();
    SendQueryForTwo(fullServiceName.AsCString(), ResourceRecord::kTypeSrv, hostFullName.AsCString(),
                    ResourceRecord::kTypeAaaa, /* aIsLegacyUnicast */ true);

    AdvanceTime(200);

    dnsMsg = sDnsMessages.GetHead();
    VerifyOrQuit(dnsMsg == nullptr);

    Log("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -");
    Log("Send a query for SRV record and validate the response");

    AdvanceTime(2000);

    sDnsMessages.Clear();
    SendQuery(fullServiceName.AsCString(), ResourceRecord::kTypeSrv, ResourceRecord::kClassInternet,
              /* aTruncated */ false,
              /* aLegacyUnicastQuery */ true);

    AdvanceTime(1000);

    dnsMsg = sDnsMessages.GetHead();
    VerifyOrQuit(dnsMsg != nullptr);
    dnsMsg->ValidateHeader(kLegacyUnicastResponse, /* Q */ 1, /* Ans */ 1, /* Auth */ 0, /* Addnl */ 3);
    dnsMsg->Validate(service, kInAnswerSection, kCheckSrv);
    dnsMsg->Validate(host, kInAdditionalSection);

    Log("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -");
    Log("Send a query for TXT record and validate the response");

    AdvanceTime(2000);

    sDnsMessages.Clear();
    SendQuery(fullServiceName.AsCString(), ResourceRecord::kTypeTxt, ResourceRecord::kClassInternet,
              /* aTruncated */ false,
              /* aLegacyUnicastQuery */ true);

    AdvanceTime(1000);

    dnsMsg = sDnsMessages.GetHead();
    VerifyOrQuit(dnsMsg != nullptr);
    dnsMsg->ValidateHeader(kLegacyUnicastResponse, /* Q */ 1, /* Ans */ 1, /* Auth */ 0, /* Addnl */ 1);
    dnsMsg->Validate(service, kInAnswerSection, kCheckTxt);

    Log("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -");
    Log("Send a query for ANY record and validate the response");

    AdvanceTime(2000);

    sDnsMessages.Clear();
    SendQuery(fullServiceName.AsCString(), ResourceRecord::kTypeAny, ResourceRecord::kClassInternet,
              /* aTruncated */ false,
              /* aLegacyUnicastQuery */ true);

    AdvanceTime(1000);

    dnsMsg = sDnsMessages.GetHead();
    VerifyOrQuit(dnsMsg != nullptr);
    dnsMsg->ValidateHeader(kLegacyUnicastResponse, /* Q */ 1, /* Ans */ 2, /* Auth */ 0, /* Addnl */ 3);
    dnsMsg->Validate(service, kInAnswerSection, kCheckSrv | kCheckTxt);
    dnsMsg->Validate(host, kInAdditionalSection);

    Log("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -");
    Log("Send a query for PTR record for service type and validate the response");

    AdvanceTime(2000);

    sDnsMessages.Clear();
    SendQuery(fullServiceType.AsCString(), ResourceRecord::kTypePtr, ResourceRecord::kClassInternet,
              /* aTruncated */ false,
              /* aLegacyUnicastQuery */ true);

    AdvanceTime(1000);

    dnsMsg = sDnsMessages.GetHead();
    VerifyOrQuit(dnsMsg != nullptr);
    dnsMsg->ValidateHeader(kLegacyUnicastResponse, /* Q */ 1, /* Ans */ 1, /* Auth */ 0, /* Addnl */ 4);
    dnsMsg->Validate(service, kInAnswerSection, kCheckPtr);
    dnsMsg->Validate(service, kInAdditionalSection, kCheckSrv | kCheckTxt);
    dnsMsg->Validate(host, kInAdditionalSection);

    Log("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -");
    Log("Send a query for non-existing record and validate the response with NSEC");

    AdvanceTime(2000);

    sDnsMessages.Clear();
    SendQuery(hostFullName.AsCString(), ResourceRecord::kTypeA, ResourceRecord::kClassInternet, /* aTruncated */ false,
              /* aLegacyUnicastQuery */ true);

    AdvanceTime(1000);

    dnsMsg = sDnsMessages.GetHead();
    VerifyOrQuit(dnsMsg != nullptr);
    dnsMsg->ValidateHeader(kLegacyUnicastResponse, /* Q */ 1, /* Ans */ 0, /* Auth */ 0, /* Addnl */ 1);
    VerifyOrQuit(dnsMsg->mAdditionalRecords.ContainsNsec(hostFullName, ResourceRecord::kTypeAaaa));

    Log("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -");

    sDnsMessages.Clear();

    SuccessOrQuit(mdns->UnregisterHost(host));

    AdvanceTime(15000);

    SuccessOrQuit(mdns->SetEnabled(false, kInfraIfIndex));
    VerifyOrQuit(sHeapAllocatedPtrs.GetLength() <= heapAllocations);

    Log("End of test");

    testFreeInstance(sInstance);
}

} // namespace Multicast
} // namespace Dns
} // namespace ot

#endif // OPENTHREAD_CONFIG_MULTICAST_DNS_ENABLE

int main(void)
{
#if OPENTHREAD_CONFIG_MULTICAST_DNS_ENABLE
    ot::Dns::Multicast::TestHostReg();
    ot::Dns::Multicast::TestKeyReg();
    ot::Dns::Multicast::TestServiceReg();
    ot::Dns::Multicast::TestUnregisterBeforeProbeFinished();
    ot::Dns::Multicast::TestServiceSubTypeReg();
    ot::Dns::Multicast::TestHostOrServiceAndKeyReg();
    ot::Dns::Multicast::TestQuery();
    ot::Dns::Multicast::TestMultiPacket();
    ot::Dns::Multicast::TestQuestionUnicastDisallowed();
    ot::Dns::Multicast::TestTxMessageSizeLimit();
    ot::Dns::Multicast::TestHostConflict();
    ot::Dns::Multicast::TestServiceConflict();

    ot::Dns::Multicast::TestBrowser();
    ot::Dns::Multicast::TestSrvResolver();
    ot::Dns::Multicast::TestTxtResolver();
    ot::Dns::Multicast::TestIp6AddrResolver();
    ot::Dns::Multicast::TestPassiveCache();
    ot::Dns::Multicast::TestLegacyUnicastResponse();

    printf("All tests passed\n");
#else
    printf("mDNS feature is not enabled\n");
#endif

    return 0;
}
