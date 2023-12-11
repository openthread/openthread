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

static constexpr uint16_t kClassQueryUnicastFlag = (1U << 15);
static constexpr uint16_t kClassCacheFlushFlag   = (1U << 15);
static constexpr uint16_t kClassMask             = 0x7fff;
static constexpr uint16_t kStringSize            = 300;
static constexpr uint16_t kMaxDataSize           = 400;
static constexpr uint16_t kNumAnnounces          = 3;
static constexpr bool     kCacheFlush            = true;
static constexpr uint16_t kMdnsPort              = 5353;
static constexpr uint32_t kInfraIfIndex          = 1;

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

    const char *AsCString(void) const { return mName; }
    bool        Matches(const char *aName) const { return StringMatch(mName, aName, kStringCaseInsensitiveMatch); }
};

typedef String<Name::kMaxNameSize> DnsNameString;

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
    bool Contains(const DnsNameString &aFullName, bool aUnicastResponse) const
    {
        bool               contains = false;
        const DnsQuestion *question = FindMatching(aFullName.AsCString());

        VerifyOrExit(question != nullptr);
        VerifyOrExit(question->mType == ResourceRecord::kTypeAny);
        VerifyOrExit(question->mClass == ResourceRecord::kClassInternet);
        VerifyOrExit(question->mUnicastResponse == aUnicastResponse);
        contains = true;

    exit:
        return contains;
    }
};

enum TtlCheckMode : uint8_t
{
    kZeroTtl,
    kNonZeroTtl,
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
    }

    static void DetemineFullNameForKey(const Core::Key &aKey, DnsNameString &aFullName)
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

        DetemineFullNameForKey(aKey, fullName);

        VerifyOrQuit(mQuestions.Contains(fullName, aUnicastResponse));
        VerifyOrQuit(mAuthRecords.ContainsKey(fullName, Data(aKey.mKeyData, aKey.mKeyDataLength), !kCacheFlush,
                                              kNonZeroTtl, aKey.mTtl));
    }

    void Validate(const Core::Host &aHost, Section aSection, GoodBye aIsGoodBye = kNotGoodBye) const
    {
        DnsNameString fullName;

        VerifyOrQuit(mHeader.GetType() == Header::kTypeResponse);

        fullName.Append("%s.local.", aHost.mHostName);

        for (uint16_t index = 0; index < aHost.mAddressesLength; index++)
        {
            VerifyOrQuit(RecordsFor(aSection).ContainsAaaa(fullName, AsCoreType(&aHost.mAddresses[index]), kCacheFlush,
                                                           aIsGoodBye ? kZeroTtl : kNonZeroTtl, aHost.mTtl));
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
        bool          checkNsec = false;

        VerifyOrQuit(mHeader.GetType() == Header::kTypeResponse);

        serviceName.Append("%s.%s.local.", aService.mServiceInstance, aService.mServiceType);
        serviceType.Append("%s.local.", aService.mServiceType);

        if (aCheckFlags & kCheckSrv)
        {
            VerifyOrQuit(RecordsFor(aSection).ContainsSrv(serviceName, aService, kCacheFlush,
                                                          aIsGoodBye ? kZeroTtl : kNonZeroTtl, aService.mTtl));
            checkNsec = true;
        }

        if (aCheckFlags & kCheckTxt)
        {
            VerifyOrQuit(RecordsFor(aSection).ContainsTxt(serviceName, aService, kCacheFlush,
                                                          aIsGoodBye ? kZeroTtl : kNonZeroTtl, aService.mTtl));
            checkNsec = true;
        }

        if (aCheckFlags & kCheckPtr)
        {
            VerifyOrQuit(RecordsFor(aSection).ContainsPtr(serviceType, serviceName, aIsGoodBye ? kZeroTtl : kNonZeroTtl,
                                                          aService.mTtl));
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

        VerifyOrQuit(mHeader.GetType() == Header::kTypeResponse);

        DetemineFullNameForKey(aKey, fullName);
        VerifyOrQuit(RecordsFor(aSection).ContainsKey(fullName, Data(aKey.mKeyData, aKey.mKeyDataLength), kCacheFlush,
                                                      aIsGoodBye ? kZeroTtl : kNonZeroTtl, aKey.mTtl));

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
            msg->mType        = kUnicastResponse;
            msg->mUnicastDest = *aUnicastDest;
        }
    }

    sDnsMessages.PushAfterTail(*msg);
}

static void SendQuery(const char *aName,
                      uint16_t    aRecordType,
                      uint16_t    aRecordClass = ResourceRecord::kClassInternet,
                      bool        aTruncated   = false)
{
    Message          *message;
    Header            header;
    Core::AddressInfo senderAddrInfo;

    message = sInstance->Get<MessagePool>().Allocate(Message::kTypeOther);
    VerifyOrQuit(message != nullptr);

    header.Clear();
    header.SetType(Header::kTypeQuery);
    header.SetQuestionCount(1);

    if (aTruncated)
    {
        header.SetTruncationFlag();
    }

    SuccessOrQuit(message->Append(header));
    SuccessOrQuit(Name::AppendName(aName, *message));
    SuccessOrQuit(message->Append(Question(aRecordType, aRecordClass)));

    SuccessOrQuit(AsCoreType(&senderAddrInfo.mAddress).FromString(kDeviceIp6Address));
    senderAddrInfo.mPort         = kMdnsPort;
    senderAddrInfo.mInfraIfIndex = 0;

    Log("Sending query for %s %s", aName, RecordTypeToString(aRecordType));

    otPlatMdnsHandleReceive(sInstance, message, /* aIsUnicast */ false, &senderAddrInfo);
}

static void SendQueryForTwo(const char *aName1, uint16_t aRecordType1, const char *aName2, uint16_t aRecordType2)
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
    senderAddrInfo.mPort         = kMdnsPort;
    senderAddrInfo.mInfraIfIndex = 0;

    Log("Sending query for %s %s and %s %s", aName1, RecordTypeToString(aRecordType1), aName2,
        RecordTypeToString(aRecordType2));

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

static const uint8_t kKey1[]     = {0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77};
static const uint8_t kKey2[]     = {0x12, 0x34, 0x56};
static const uint8_t kTxtData1[] = {3, 'a', '=', '1', 0};
static const uint8_t kTxtData2[] = {1, 'b', 0};

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

        for (uint8_t index = 0; index < service.mSubTypeLabelsLength; index++)
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

    for (uint8_t index = 0; index < service.mSubTypeLabelsLength; index++)
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
        dnsMsg->ValidateHeader(kMulticastResponse, /* Q */ 0, /* Ans */ 1, /* Auth */ 0, /* Addnl */ 0);
        dnsMsg->ValidateSubType(service.mSubTypeLabels[index], service);
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
        dnsMsg->ValidateHeader(kMulticastResponse, /* Q */ 0, /* Ans */ 1, /* Auth */ 0, /* Addnl */ 0);
        dnsMsg->ValidateSubType(service.mSubTypeLabels[3], service);
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

        for (uint8_t index = 0; index < service.mSubTypeLabelsLength; index++)
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
        dnsMsg->ValidateHeader(kMulticastResponse, /* Q */ 0, /* Ans */ 3, /* Auth */ 0, /* Addnl */ 0);

        dnsMsg->ValidateSubType(kSubTypes1[3], service, kGoodBye);
        dnsMsg->ValidateSubType(kSubTypes2[1], service);
        dnsMsg->ValidateSubType(kSubTypes2[3], service);

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

        for (uint8_t index = 0; index < service.mSubTypeLabelsLength; index++)
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

        for (uint8_t index = 0; index < service1.mSubTypeLabelsLength; index++)
        {
            dnsMsg->ValidateSubType(service1.mSubTypeLabels[index], service1);
        }

        for (uint8_t index = 0; index < service3.mSubTypeLabelsLength; index++)
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

    dnsMsg->ValidateHeader(kMulticastResponse, /* Q */ 0, /* Ans */ 2, /* Auth */ 0, /* Addnl */ 0);

    dnsMsg->ValidateSubType("_s", service1);
    dnsMsg->ValidateSubType("_s", service3);

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

    dnsMsg->ValidateHeader(kMulticastResponse, /* Q */ 0, /* Ans */ 1, /* Auth */ 0, /* Addnl */ 0);
    dnsMsg->ValidateSubType("_r", service1);

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

        for (uint8_t index = 0; index < service1.mSubTypeLabelsLength; index++)
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

    for (uint8_t index = 0; index < service1.mSubTypeLabelsLength; index++)
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

        for (uint8_t index = 0; index < service.mSubTypeLabelsLength; index++)
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

    printf("All tests passed\n");
#else
    printf("mDNS feature is not enabled\n");
#endif

    return 0;
}
