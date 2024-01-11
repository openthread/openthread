/*
 *  Copyright (c) 2022, The OpenThread Authors.
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

#include "border_router/routing_manager.hpp"
#include "common/arg_macros.hpp"
#include "common/array.hpp"
#include "common/time.hpp"
#include "instance/instance.hpp"
#include "net/icmp6.hpp"
#include "net/nd6.hpp"

namespace ot {

#if OPENTHREAD_CONFIG_BORDER_ROUTING_ENABLE

// Logs a message and adds current time (sNow) as "<hours>:<min>:<secs>.<msec>"
#define Log(...)                                                                                         \
    printf("%02u:%02u:%02u.%03u " OT_FIRST_ARG(__VA_ARGS__) "\n", (sNow / 3600000), (sNow / 60000) % 60, \
           (sNow / 1000) % 60, sNow % 1000 OT_REST_ARGS(__VA_ARGS__))

static constexpr uint32_t kInfraIfIndex     = 1;
static const char         kInfraIfAddress[] = "fe80::1";

static constexpr uint32_t kValidLitime       = 2000;
static constexpr uint32_t kPreferredLifetime = 1800;

static constexpr uint32_t kRioValidLifetime       = 1800;
static constexpr uint32_t kRioDeprecatingLifetime = 300;

static constexpr uint16_t kMaxRaSize              = 800;
static constexpr uint16_t kMaxDeprecatingPrefixes = 16;

static constexpr otOperationalDataset kDataset = {
    .mActiveTimestamp =
        {
            .mSeconds       = 1,
            .mTicks         = 0,
            .mAuthoritative = false,
        },
    .mNetworkKey =
        {
            .m8 = {0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0x99, 0xaa, 0xbb, 0xcc, 0xdd, 0xee, 0xff},
        },
    .mNetworkName = {"OpenThread"},
    .mExtendedPanId =
        {
            .m8 = {0xde, 0xad, 0x00, 0xbe, 0xef, 0x00, 0xca, 0xfe},
        },
    .mMeshLocalPrefix =
        {
            .m8 = {0xfd, 0x00, 0xaa, 0xbb, 0xcc, 0xdd, 0xee, 0xff},
        },
    .mPanId   = 0x1234,
    .mChannel = 11,
    .mPskc =
        {
            .m8 = {0xc2, 0x3a, 0x76, 0xe9, 0x8f, 0x1a, 0x64, 0x83, 0x63, 0x9b, 0x1a, 0xc1, 0x27, 0x1e, 0x2e, 0x27},
        },
    .mSecurityPolicy =
        {
            .mRotationTime                 = 672,
            .mObtainNetworkKeyEnabled      = true,
            .mNativeCommissioningEnabled   = true,
            .mRoutersEnabled               = true,
            .mExternalCommissioningEnabled = true,
        },
    .mChannelMask = 0x07fff800,
    .mComponents =
        {
            .mIsActiveTimestampPresent = true,
            .mIsNetworkKeyPresent      = true,
            .mIsNetworkNamePresent     = true,
            .mIsExtendedPanIdPresent   = true,
            .mIsMeshLocalPrefixPresent = true,
            .mIsPanIdPresent           = true,
            .mIsChannelPresent         = true,
            .mIsPskcPresent            = true,
            .mIsSecurityPolicyPresent  = true,
            .mIsChannelMaskPresent     = true,
        },
};

static Instance *sInstance;

static uint32_t sNow = 0;
static uint32_t sAlarmTime;
static bool     sAlarmOn = false;

static otRadioFrame sRadioTxFrame;
static uint8_t      sRadioTxFramePsdu[OT_RADIO_FRAME_MAX_SIZE];
static bool         sRadioTxOngoing = false;

using Icmp6Packet = Ip6::Nd::RouterAdvertMessage::Icmp6Packet;

enum ExpectedPio
{
    kNoPio,                     // Expect to see no PIO in RA.
    kPioAdvertisingLocalOnLink, // Expect to see local on-link prefix advertised (non-zero preferred lifetime).
    kPioDeprecatingLocalOnLink, // Expect to see local on-link prefix deprecated (zero preferred lifetime).
};

struct DeprecatingPrefix
{
    DeprecatingPrefix(void) = default;

    DeprecatingPrefix(const Ip6::Prefix &aPrefix, uint32_t aLifetime)
        : mPrefix(aPrefix)
        , mLifetime(aLifetime)
    {
    }

    bool Matches(const Ip6::Prefix &aPrefix) const { return mPrefix == aPrefix; }

    Ip6::Prefix mPrefix;   // Old on-link prefix being deprecated.
    uint32_t    mLifetime; // Valid lifetime of prefix from PIO.
};

static Ip6::Address sInfraIfAddress;

bool        sRsEmitted;      // Indicates if an RS message was emitted by BR.
bool        sRaValidated;    // Indicates if an RA was emitted by BR and successfully validated.
bool        sNsEmitted;      // Indicates if an NS message was emitted by BR.
bool        sRespondToNs;    // Indicates whether or not to respond to NS.
ExpectedPio sExpectedPio;    // Expected PIO in the emitted RA by BR (MUST be seen in RA to set `sRaValidated`).
uint32_t    sOnLinkLifetime; // Valid lifetime for local on-link prefix from the last processed RA.

enum ExpectedRaHeaderFlags
{
    kRaHeaderFlagsSkipChecking, // Skip checking the RA header flags.
    kRaHeaderFlagsNone,         // Expect no flag (neither M or O).
    kRaHeaderFlagsOnlyM,        // Expect M flag only.
    kRaHeaderFlagsOnlyO,        // Expect O flag only.
    kRaHeaderFlagsBothMAndO,    // Expect both M and O flags.
};

// The expected RA header flags when validating emitted RA message.
ExpectedRaHeaderFlags sExpectedRaHeaderFlags;

// Array containing deprecating prefixes from PIOs in the last processed RA.
Array<DeprecatingPrefix, kMaxDeprecatingPrefixes> sDeprecatingPrefixes;

static constexpr uint16_t kMaxRioPrefixes = 10;

struct RioPrefix
{
    RioPrefix(void) = default;

    explicit RioPrefix(const Ip6::Prefix &aPrefix)
        : mSawInRa(false)
        , mPrefix(aPrefix)
        , mLifetime(0)
    {
    }

    bool        mSawInRa;  // Indicate whether or not this prefix was seen in the emitted RA (as RIO).
    Ip6::Prefix mPrefix;   // The RIO prefix.
    uint32_t    mLifetime; // The RIO prefix lifetime - only valid when `mSawInRa`
};

class ExpectedRios : public Array<RioPrefix, kMaxRioPrefixes>
{
public:
    void Add(const Ip6::Prefix &aPrefix) { SuccessOrQuit(PushBack(RioPrefix(aPrefix))); }

    bool SawAll(void) const
    {
        bool sawAll = true;

        for (const RioPrefix &rioPrefix : *this)
        {
            if (!rioPrefix.mSawInRa)
            {
                sawAll = false;
                break;
            }
        }

        return sawAll;
    }
};

ExpectedRios sExpectedRios; // Expected RIO prefixes in emitted RAs.

//----------------------------------------------------------------------------------------------------------------------
// Function prototypes

void        ProcessRadioTxAndTasklets(void);
void        AdvanceTime(uint32_t aDuration);
void        LogRouterAdvert(const Icmp6Packet &aPacket);
void        ValidateRouterAdvert(const Icmp6Packet &aPacket);
const char *PreferenceToString(int8_t aPreference);
void        SendRouterAdvert(const Ip6::Address &aAddress, const Icmp6Packet &aPacket);
void        SendNeighborAdvert(const Ip6::Address &aAddress, const Ip6::Nd::NeighborAdvertMessage &aNaMessage);
void        DiscoverNat64Prefix(const Ip6::Prefix &aPrefix);

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
// `otPlatRadio

otRadioCaps otPlatRadioGetCaps(otInstance *) { return OT_RADIO_CAPS_ACK_TIMEOUT | OT_RADIO_CAPS_CSMA_BACKOFF; }

otError otPlatRadioTransmit(otInstance *, otRadioFrame *)
{
    sRadioTxOngoing = true;

    return OT_ERROR_NONE;
}

otRadioFrame *otPlatRadioGetTransmitBuffer(otInstance *) { return &sRadioTxFrame; }

//----------------------------------------------------------------------------------------------------------------------
// `otPlatAlaram

void otPlatAlarmMilliStop(otInstance *) { sAlarmOn = false; }

void otPlatAlarmMilliStartAt(otInstance *, uint32_t aT0, uint32_t aDt)
{
    sAlarmOn   = true;
    sAlarmTime = aT0 + aDt;
}

uint32_t otPlatAlarmMilliGetNow(void) { return sNow; }

//---------------------------------------------------------------------------------------------------------------------
// otPlatInfraIf

bool otPlatInfraIfHasAddress(uint32_t aInfraIfIndex, const otIp6Address *aAddress)
{
    VerifyOrQuit(aInfraIfIndex == kInfraIfIndex);

    return AsCoreType(aAddress) == sInfraIfAddress;
}

otError otPlatInfraIfSendIcmp6Nd(uint32_t            aInfraIfIndex,
                                 const otIp6Address *aDestAddress,
                                 const uint8_t      *aBuffer,
                                 uint16_t            aBufferLength)
{
    Icmp6Packet packet;

    Log("otPlatInfraIfSendIcmp6Nd(aDestAddr: %s, aBufferLength:%u)", AsCoreType(aDestAddress).ToString().AsCString(),
        aBufferLength);

    VerifyOrQuit(aInfraIfIndex == kInfraIfIndex);

    packet.Init(aBuffer, aBufferLength);

    VerifyOrQuit(aBufferLength >= sizeof(Ip6::Icmp::Header));

    switch (reinterpret_cast<const Ip6::Icmp::Header *>(aBuffer)->GetType())
    {
    case Ip6::Icmp::Header::kTypeRouterSolicit:
        Log("  Router Solicit message");
        sRsEmitted = true;
        break;

    case Ip6::Icmp::Header::kTypeRouterAdvert:
        Log("  Router Advertisement message");
        LogRouterAdvert(packet);
        ValidateRouterAdvert(packet);
        break;

    case Ip6::Icmp::Header::kTypeNeighborSolicit:
    {
        const Ip6::Nd::NeighborSolicitMessage *nsMsg =
            reinterpret_cast<const Ip6::Nd::NeighborSolicitMessage *>(packet.GetBytes());

        Log("  Neighbor Solicit message");

        VerifyOrQuit(packet.GetLength() >= sizeof(Ip6::Nd::NeighborSolicitMessage));
        VerifyOrQuit(nsMsg->IsValid());
        sNsEmitted = true;

        if (sRespondToNs)
        {
            Ip6::Nd::NeighborAdvertMessage naMsg;

            naMsg.SetTargetAddress(nsMsg->GetTargetAddress());
            naMsg.SetRouterFlag();
            naMsg.SetSolicitedFlag();
            SendNeighborAdvert(AsCoreType(aDestAddress), naMsg);
        }

        break;
    }

    default:
        VerifyOrQuit(false, "Bad ICMP6 type");
    }

    return OT_ERROR_NONE;
}

//----------------------------------------------------------------------------------------------------------------------

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

void ProcessRadioTxAndTasklets(void)
{
    do
    {
        if (sRadioTxOngoing)
        {
            sRadioTxOngoing = false;
            otPlatRadioTxStarted(sInstance, &sRadioTxFrame);
            otPlatRadioTxDone(sInstance, &sRadioTxFrame, nullptr, OT_ERROR_NONE);
        }

        otTaskletsProcess(sInstance);
    } while (otTaskletsArePending(sInstance));
}

void AdvanceTime(uint32_t aDuration)
{
    uint32_t time = sNow + aDuration;

    Log("AdvanceTime for %u.%03u", aDuration / 1000, aDuration % 1000);

    while (TimeMilli(sAlarmTime) <= TimeMilli(time))
    {
        ProcessRadioTxAndTasklets();
        sNow = sAlarmTime;
        otPlatAlarmMilliFired(sInstance);
    }

    ProcessRadioTxAndTasklets();
    sNow = time;
}

void ValidateRouterAdvert(const Icmp6Packet &aPacket)
{
    constexpr uint8_t kMaxPrefixes = 16;

    Ip6::Nd::RouterAdvertMessage     raMsg(aPacket);
    bool                             sawExpectedPio = false;
    Array<Ip6::Prefix, kMaxPrefixes> pioPrefixes;
    Array<Ip6::Prefix, kMaxPrefixes> rioPrefixes;
#if OPENTHREAD_CONFIG_BORDER_ROUTING_STUB_ROUTER_FLAG_IN_EMITTED_RA_ENABLE
    bool sawStubRouterFlag = false;
#endif

    VerifyOrQuit(raMsg.IsValid());

    VerifyOrQuit(raMsg.GetHeader().GetRouterLifetime() == 0);

    switch (sExpectedRaHeaderFlags)
    {
    case kRaHeaderFlagsSkipChecking:
        break;
    case kRaHeaderFlagsNone:
        VerifyOrQuit(!raMsg.GetHeader().IsManagedAddressConfigFlagSet());
        VerifyOrQuit(!raMsg.GetHeader().IsOtherConfigFlagSet());
        break;
    case kRaHeaderFlagsOnlyM:
        VerifyOrQuit(raMsg.GetHeader().IsManagedAddressConfigFlagSet());
        VerifyOrQuit(!raMsg.GetHeader().IsOtherConfigFlagSet());
        break;
    case kRaHeaderFlagsOnlyO:
        VerifyOrQuit(!raMsg.GetHeader().IsManagedAddressConfigFlagSet());
        VerifyOrQuit(raMsg.GetHeader().IsOtherConfigFlagSet());
        break;
    case kRaHeaderFlagsBothMAndO:
        VerifyOrQuit(raMsg.GetHeader().IsManagedAddressConfigFlagSet());
        VerifyOrQuit(raMsg.GetHeader().IsOtherConfigFlagSet());
        break;
    }

    sDeprecatingPrefixes.Clear();

    for (const Ip6::Nd::Option &option : raMsg)
    {
        switch (option.GetType())
        {
        case Ip6::Nd::Option::kTypePrefixInfo:
        {
            const Ip6::Nd::PrefixInfoOption &pio = static_cast<const Ip6::Nd::PrefixInfoOption &>(option);
            Ip6::Prefix                      prefix;
            Ip6::Prefix                      localOnLink;

            VerifyOrQuit(pio.IsValid());
            pio.GetPrefix(prefix);

            VerifyOrQuit(!pioPrefixes.Contains(prefix), "Duplicate PIO prefix in RA");
            SuccessOrQuit(pioPrefixes.PushBack(prefix));

            SuccessOrQuit(otBorderRoutingGetOnLinkPrefix(sInstance, &localOnLink));

            if (prefix == localOnLink)
            {
                switch (sExpectedPio)
                {
                case kNoPio:
                    break;

                case kPioAdvertisingLocalOnLink:
                    if (pio.GetPreferredLifetime() > 0)
                    {
                        sOnLinkLifetime = pio.GetValidLifetime();
                        sawExpectedPio  = true;
                    }
                    break;

                case kPioDeprecatingLocalOnLink:
                    if (pio.GetPreferredLifetime() == 0)
                    {
                        sOnLinkLifetime = pio.GetValidLifetime();
                        sawExpectedPio  = true;
                    }
                    break;
                }
            }
            else
            {
                VerifyOrQuit(pio.GetPreferredLifetime() == 0, "Old on link prefix is not deprecated");
                SuccessOrQuit(sDeprecatingPrefixes.PushBack(DeprecatingPrefix(prefix, pio.GetValidLifetime())));
            }
            break;
        }

        case Ip6::Nd::Option::kTypeRouteInfo:
        {
            const Ip6::Nd::RouteInfoOption &rio = static_cast<const Ip6::Nd::RouteInfoOption &>(option);
            Ip6::Prefix                     prefix;

            VerifyOrQuit(rio.IsValid());
            rio.GetPrefix(prefix);

            VerifyOrQuit(!rioPrefixes.Contains(prefix), "Duplicate RIO prefix in RA");
            SuccessOrQuit(rioPrefixes.PushBack(prefix));

            for (RioPrefix &rioPrefix : sExpectedRios)
            {
                if (prefix == rioPrefix.mPrefix)
                {
                    rioPrefix.mSawInRa  = true;
                    rioPrefix.mLifetime = rio.GetRouteLifetime();
                }
            }

            break;
        }

#if OPENTHREAD_CONFIG_BORDER_ROUTING_STUB_ROUTER_FLAG_IN_EMITTED_RA_ENABLE
        case Ip6::Nd::Option::kTypeRaFlagsExtension:
        {
            const Ip6::Nd::RaFlagsExtOption &flagsOption = static_cast<const Ip6::Nd::RaFlagsExtOption &>(option);

            VerifyOrQuit(flagsOption.IsValid());
            VerifyOrQuit(flagsOption.IsStubRouterFlagSet());
            sawStubRouterFlag = true;
            break;
        }
#endif

        default:
            VerifyOrQuit(false, "Unexpected option type in RA msg");
        }
    }

#if OPENTHREAD_CONFIG_BORDER_ROUTING_STUB_ROUTER_FLAG_IN_EMITTED_RA_ENABLE
    VerifyOrQuit(sawStubRouterFlag);
#endif

    if (!sRaValidated)
    {
        switch (sExpectedPio)
        {
        case kNoPio:
            break;
        case kPioAdvertisingLocalOnLink:
        case kPioDeprecatingLocalOnLink:
            // First emitted RAs may not yet have the expected PIO
            // so we exit and not set `sRaValidated` to allow it
            // to be checked for next received RA.
            VerifyOrExit(sawExpectedPio);
            break;
        }

        sRaValidated = true;
    }

exit:
    return;
}

//----------------------------------------------------------------------------------------------------------------------

void LogRouterAdvert(const Icmp6Packet &aPacket)
{
    Ip6::Nd::RouterAdvertMessage raMsg(aPacket);

    VerifyOrQuit(raMsg.IsValid());

    Log("     RA header - M:%u, O:%u", raMsg.GetHeader().IsManagedAddressConfigFlagSet(),
        raMsg.GetHeader().IsOtherConfigFlagSet());
    Log("     RA header - lifetime %u, prf:%s", raMsg.GetHeader().GetRouterLifetime(),
        PreferenceToString(raMsg.GetHeader().GetDefaultRouterPreference()));

    for (const Ip6::Nd::Option &option : raMsg)
    {
        switch (option.GetType())
        {
        case Ip6::Nd::Option::kTypePrefixInfo:
        {
            const Ip6::Nd::PrefixInfoOption &pio = static_cast<const Ip6::Nd::PrefixInfoOption &>(option);
            Ip6::Prefix                      prefix;

            VerifyOrQuit(pio.IsValid());
            pio.GetPrefix(prefix);
            Log("     PIO - %s, flags:%s%s, valid:%u, preferred:%u", prefix.ToString().AsCString(),
                pio.IsOnLinkFlagSet() ? "L" : "", pio.IsAutoAddrConfigFlagSet() ? "A" : "", pio.GetValidLifetime(),
                pio.GetPreferredLifetime());
            break;
        }

        case Ip6::Nd::Option::kTypeRouteInfo:
        {
            const Ip6::Nd::RouteInfoOption &rio = static_cast<const Ip6::Nd::RouteInfoOption &>(option);
            Ip6::Prefix                     prefix;

            VerifyOrQuit(rio.IsValid());
            rio.GetPrefix(prefix);
            Log("     RIO - %s, prf:%s, lifetime:%u", prefix.ToString().AsCString(),
                PreferenceToString(rio.GetPreference()), rio.GetRouteLifetime());
            break;
        }

        case Ip6::Nd::Option::kTypeRaFlagsExtension:
        {
            const Ip6::Nd::RaFlagsExtOption &flagsOption = static_cast<const Ip6::Nd::RaFlagsExtOption &>(option);

            VerifyOrQuit(flagsOption.IsValid());
            Log("     FlagsExt - StubRouter:%u", flagsOption.IsStubRouterFlagSet());
            break;
        }

        default:
            VerifyOrQuit(false, "Bad option type in RA msg");
        }
    }
}

void LogRouterAdvert(const uint8_t *aBuffer, size_t aLength)
{
    Icmp6Packet packet;
    packet.Init(aBuffer, aLength);
    LogRouterAdvert(packet);
}

const char *PreferenceToString(int8_t aPreference)
{
    const char *str = "";

    switch (aPreference)
    {
    case NetworkData::kRoutePreferenceLow:
        str = "low";
        break;

    case NetworkData::kRoutePreferenceMedium:
        str = "med";
        break;

    case NetworkData::kRoutePreferenceHigh:
        str = "high";
        break;

    default:
        break;
    }

    return str;
}

void SendRouterAdvert(const Ip6::Address &aAddress, const uint8_t *aBuffer, uint16_t aLength)
{
    otPlatInfraIfRecvIcmp6Nd(sInstance, kInfraIfIndex, &aAddress, aBuffer, aLength);
}

void SendRouterAdvert(const Ip6::Address &aAddress, const Icmp6Packet &aPacket)
{
    SendRouterAdvert(aAddress, aPacket.GetBytes(), aPacket.GetLength());
}

void SendNeighborAdvert(const Ip6::Address &aAddress, const Ip6::Nd::NeighborAdvertMessage &aNaMessage)
{
    Log("Sending NA from %s", aAddress.ToString().AsCString());
    otPlatInfraIfRecvIcmp6Nd(sInstance, kInfraIfIndex, &aAddress, reinterpret_cast<const uint8_t *>(&aNaMessage),
                             sizeof(aNaMessage));
}

void DiscoverNat64Prefix(const Ip6::Prefix &aPrefix)
{
    Log("Discovered NAT64 prefix %s", aPrefix.ToString().AsCString());

    otPlatInfraIfDiscoverNat64PrefixDone(sInstance, kInfraIfIndex, &aPrefix);
}

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

void VerifyOmrPrefixInNetData(const Ip6::Prefix &aOmrPrefix, bool aDefaultRoute)
{
    otNetworkDataIterator           iterator = OT_NETWORK_DATA_ITERATOR_INIT;
    NetworkData::OnMeshPrefixConfig prefixConfig;

    Log("VerifyOmrPrefixInNetData(%s, def-route:%s)", aOmrPrefix.ToString().AsCString(), aDefaultRoute ? "yes" : "no");

    SuccessOrQuit(otNetDataGetNextOnMeshPrefix(sInstance, &iterator, &prefixConfig));
    VerifyOrQuit(prefixConfig.GetPrefix() == aOmrPrefix);
    VerifyOrQuit(prefixConfig.mStable == true);
    VerifyOrQuit(prefixConfig.mSlaac == true);
    VerifyOrQuit(prefixConfig.mPreferred == true);
    VerifyOrQuit(prefixConfig.mOnMesh == true);
    VerifyOrQuit(prefixConfig.mDefaultRoute == aDefaultRoute);

    VerifyOrQuit(otNetDataGetNextOnMeshPrefix(sInstance, &iterator, &prefixConfig) == kErrorNotFound);
}

void VerifyNoOmrPrefixInNetData(void)
{
    otNetworkDataIterator           iterator = OT_NETWORK_DATA_ITERATOR_INIT;
    NetworkData::OnMeshPrefixConfig prefixConfig;

    Log("VerifyNoOmrPrefixInNetData()");
    VerifyOrQuit(otNetDataGetNextOnMeshPrefix(sInstance, &iterator, &prefixConfig) != kErrorNone);
}

using NetworkData::RoutePreference;

enum ExternalRouteMode : uint8_t
{
    kNoRoute,
    kDefaultRoute,
    kUlaRoute,
};

enum AdvPioMode : uint8_t
{
    kSkipAdvPioCheck,
    kWithAdvPioFlagSet,
    kWithAdvPioCleared,
};

void VerifyExternalRouteInNetData(ExternalRouteMode aExternalRouteMode, AdvPioMode aAdvPioMode = kSkipAdvPioCheck)
{
    Error                 error;
    otNetworkDataIterator iterator = OT_NETWORK_DATA_ITERATOR_INIT;
    otExternalRouteConfig routeConfig;

    error = otNetDataGetNextRoute(sInstance, &iterator, &routeConfig);

    switch (aExternalRouteMode)
    {
    case kNoRoute:
        Log("VerifyExternalRouteInNetData(kNoRoute)");
        VerifyOrQuit(error != kErrorNone);
        break;

    case kDefaultRoute:
        Log("VerifyExternalRouteInNetData(kDefaultRoute)");
        VerifyOrQuit(error == kErrorNone);
        VerifyOrQuit(routeConfig.mPrefix.mLength == 0);
        VerifyOrQuit((aAdvPioMode == kSkipAdvPioCheck) || (routeConfig.mAdvPio == (aAdvPioMode == kWithAdvPioFlagSet)));
        VerifyOrQuit(otNetDataGetNextRoute(sInstance, &iterator, &routeConfig) != kErrorNone);
        break;

    case kUlaRoute:
        Log("VerifyExternalRouteInNetData(kUlaRoute)");
        VerifyOrQuit(error == kErrorNone);
        VerifyOrQuit(routeConfig.mPrefix.mLength == 7);
        VerifyOrQuit(routeConfig.mPrefix.mPrefix.mFields.m8[0] == 0xfc);
        VerifyOrQuit((aAdvPioMode == kSkipAdvPioCheck) || (routeConfig.mAdvPio == (aAdvPioMode == kWithAdvPioFlagSet)));
        VerifyOrQuit(otNetDataGetNextRoute(sInstance, &iterator, &routeConfig) != kErrorNone);
        break;
    }
}

void VerifyNat64PrefixInNetData(const Ip6::Prefix &aNat64Prefix)
{
    otNetworkDataIterator            iterator = OT_NETWORK_DATA_ITERATOR_INIT;
    NetworkData::ExternalRouteConfig routeConfig;
    bool                             didFind = false;

    Log("VerifyNat64PrefixInNetData()");

    while (otNetDataGetNextRoute(sInstance, &iterator, &routeConfig) == kErrorNone)
    {
        if (!routeConfig.mNat64 || !routeConfig.GetPrefix().IsValidNat64())
        {
            continue;
        }

        Log("   nat64 prefix:%s, prf:%s", routeConfig.GetPrefix().ToString().AsCString(),
            PreferenceToString(routeConfig.mPreference));

        VerifyOrQuit(routeConfig.GetPrefix() == aNat64Prefix);
        didFind = true;
    }

    VerifyOrQuit(didFind);
}

struct Pio
{
    Pio(const Ip6::Prefix &aPrefix, uint32_t aValidLifetime, uint32_t aPreferredLifetime)
        : mPrefix(aPrefix)
        , mValidLifetime(aValidLifetime)
        , mPreferredLifetime(aPreferredLifetime)
    {
    }

    const Ip6::Prefix &mPrefix;
    uint32_t           mValidLifetime;
    uint32_t           mPreferredLifetime;
};

struct Rio
{
    Rio(const Ip6::Prefix &aPrefix, uint32_t aValidLifetime, RoutePreference aPreference)
        : mPrefix(aPrefix)
        , mValidLifetime(aValidLifetime)
        , mPreference(aPreference)
    {
    }

    const Ip6::Prefix &mPrefix;
    uint32_t           mValidLifetime;
    RoutePreference    mPreference;
};

struct DefaultRoute
{
    DefaultRoute(uint32_t aLifetime, RoutePreference aPreference)
        : mLifetime(aLifetime)
        , mPreference(aPreference)
    {
    }

    uint32_t        mLifetime;
    RoutePreference mPreference;
};

struct RaFlags : public Clearable<RaFlags>
{
    RaFlags(void)
        : mManagedAddressConfigFlag(false)
        , mOtherConfigFlag(false)
        , mStubRouterFlag(false)
    {
    }

    bool mManagedAddressConfigFlag;
    bool mOtherConfigFlag;
    bool mStubRouterFlag;
};

template <size_t N>
uint16_t BuildRouterAdvert(uint8_t (&aBuffer)[N],
                           const Pio          *aPios,
                           uint16_t            aNumPios,
                           const Rio          *aRios,
                           uint16_t            aNumRios,
                           const DefaultRoute &aDefaultRoute,
                           const RaFlags      &aRaFlags)
{
    Ip6::Nd::RouterAdvertMessage::Header header;
    uint16_t                             length;

    header.SetRouterLifetime(aDefaultRoute.mLifetime);
    header.SetDefaultRouterPreference(aDefaultRoute.mPreference);

    if (aRaFlags.mManagedAddressConfigFlag)
    {
        header.SetManagedAddressConfigFlag();
    }

    if (aRaFlags.mOtherConfigFlag)
    {
        header.SetOtherConfigFlag();
    }

    {
        Ip6::Nd::RouterAdvertMessage raMsg(header, aBuffer);

        if (aRaFlags.mStubRouterFlag)
        {
            SuccessOrQuit(raMsg.AppendFlagsExtensionOption(/* aStubRouterFlag */ true));
        }

        for (; aNumPios > 0; aPios++, aNumPios--)
        {
            SuccessOrQuit(
                raMsg.AppendPrefixInfoOption(aPios->mPrefix, aPios->mValidLifetime, aPios->mPreferredLifetime));
        }

        for (; aNumRios > 0; aRios++, aNumRios--)
        {
            SuccessOrQuit(raMsg.AppendRouteInfoOption(aRios->mPrefix, aRios->mValidLifetime, aRios->mPreference));
        }

        length = raMsg.GetAsPacket().GetLength();
    }

    return length;
}

void SendRouterAdvert(const Ip6::Address &aRouterAddress,
                      const Pio          *aPios,
                      uint16_t            aNumPios,
                      const Rio          *aRios,
                      uint16_t            aNumRios,
                      const DefaultRoute &aDefaultRoute,
                      const RaFlags      &aRaFlags)
{
    uint8_t  buffer[kMaxRaSize];
    uint16_t length = BuildRouterAdvert(buffer, aPios, aNumPios, aRios, aNumRios, aDefaultRoute, aRaFlags);

    SendRouterAdvert(aRouterAddress, buffer, length);
    Log("Sending RA from router %s", aRouterAddress.ToString().AsCString());
    LogRouterAdvert(buffer, length);
}

template <uint16_t kNumPios, uint16_t kNumRios>
void SendRouterAdvert(const Ip6::Address &aRouterAddress,
                      const Pio (&aPios)[kNumPios],
                      const Rio (&aRios)[kNumRios],
                      const DefaultRoute &aDefaultRoute = DefaultRoute(0, NetworkData::kRoutePreferenceMedium),
                      const RaFlags      &aRaFlags      = RaFlags())
{
    SendRouterAdvert(aRouterAddress, aPios, kNumPios, aRios, kNumRios, aDefaultRoute, aRaFlags);
}

template <uint16_t kNumPios>
void SendRouterAdvert(const Ip6::Address &aRouterAddress,
                      const Pio (&aPios)[kNumPios],
                      const DefaultRoute &aDefaultRoute = DefaultRoute(0, NetworkData::kRoutePreferenceMedium),
                      const RaFlags      &aRaFlags      = RaFlags())
{
    SendRouterAdvert(aRouterAddress, aPios, kNumPios, nullptr, 0, aDefaultRoute, aRaFlags);
}

template <uint16_t kNumRios>
void SendRouterAdvert(const Ip6::Address &aRouterAddress,
                      const Rio (&aRios)[kNumRios],
                      const DefaultRoute &aDefaultRoute = DefaultRoute(0, NetworkData::kRoutePreferenceMedium),
                      const RaFlags      &aRaFlags      = RaFlags())
{
    SendRouterAdvert(aRouterAddress, nullptr, 0, aRios, kNumRios, aDefaultRoute, aRaFlags);
}

void SendRouterAdvert(const Ip6::Address &aRouterAddress,
                      const DefaultRoute &aDefaultRoute,
                      const RaFlags      &aRaFlags = RaFlags())
{
    SendRouterAdvert(aRouterAddress, nullptr, 0, nullptr, 0, aDefaultRoute, aRaFlags);
}

void SendRouterAdvert(const Ip6::Address &aRouterAddress, const RaFlags &aRaFlags)
{
    SendRouterAdvert(aRouterAddress, nullptr, 0, nullptr, 0, DefaultRoute(0, NetworkData::kRoutePreferenceMedium),
                     aRaFlags);
}

template <uint16_t kNumPios> void SendRouterAdvertToBorderRoutingProcessIcmp6Ra(const Pio (&aPios)[kNumPios])
{
    uint8_t  buffer[kMaxRaSize];
    uint16_t length = BuildRouterAdvert(buffer, aPios, kNumPios, nullptr, 0,
                                        DefaultRoute(0, NetworkData::kRoutePreferenceMedium), RaFlags());

    otPlatBorderRoutingProcessIcmp6Ra(sInstance, buffer, length);
    Log("Passing RA to otPlatBorderRoutingProcessIcmp6Ra");
    LogRouterAdvert(buffer, length);
}

struct OnLinkPrefix : public Pio
{
    OnLinkPrefix(const Ip6::Prefix  &aPrefix,
                 uint32_t            aValidLifetime,
                 uint32_t            aPreferredLifetime,
                 const Ip6::Address &aRouterAddress)
        : Pio(aPrefix, aValidLifetime, aPreferredLifetime)
        , mRouterAddress(aRouterAddress)
    {
    }

    const Ip6::Address &mRouterAddress;
};

struct RoutePrefix : public Rio
{
    RoutePrefix(const Ip6::Prefix  &aPrefix,
                uint32_t            aValidLifetime,
                RoutePreference     aPreference,
                const Ip6::Address &aRouterAddress)
        : Rio(aPrefix, aValidLifetime, aPreference)
        , mRouterAddress(aRouterAddress)
    {
    }

    const Ip6::Address &mRouterAddress;
};

template <uint16_t kNumOnLinkPrefixes, uint16_t kNumRoutePrefixes>
void VerifyPrefixTable(const OnLinkPrefix (&aOnLinkPrefixes)[kNumOnLinkPrefixes],
                       const RoutePrefix (&aRoutePrefixes)[kNumRoutePrefixes])
{
    VerifyPrefixTable(aOnLinkPrefixes, kNumOnLinkPrefixes, aRoutePrefixes, kNumRoutePrefixes);
}

template <uint16_t kNumOnLinkPrefixes> void VerifyPrefixTable(const OnLinkPrefix (&aOnLinkPrefixes)[kNumOnLinkPrefixes])
{
    VerifyPrefixTable(aOnLinkPrefixes, kNumOnLinkPrefixes, nullptr, 0);
}

template <uint16_t kNumRoutePrefixes> void VerifyPrefixTable(const RoutePrefix (&aRoutePrefixes)[kNumRoutePrefixes])
{
    VerifyPrefixTable(nullptr, 0, aRoutePrefixes, kNumRoutePrefixes);
}

void VerifyPrefixTable(const OnLinkPrefix *aOnLinkPrefixes,
                       uint16_t            aNumOnLinkPrefixes,
                       const RoutePrefix  *aRoutePrefixes,
                       uint16_t            aNumRoutePrefixes)
{
    BorderRouter::RoutingManager::PrefixTableIterator iter;
    BorderRouter::RoutingManager::PrefixTableEntry    entry;
    uint16_t                                          onLinkPrefixCount = 0;
    uint16_t                                          routePrefixCount  = 0;

    Log("VerifyPrefixTable()");

    sInstance->Get<BorderRouter::RoutingManager>().InitPrefixTableIterator(iter);

    while (sInstance->Get<BorderRouter::RoutingManager>().GetNextPrefixTableEntry(iter, entry) == kErrorNone)
    {
        bool didFind = false;

        if (entry.mIsOnLink)
        {
            Log("   on-link prefix:%s, valid:%u, preferred:%u, router:%s, age:%u",
                AsCoreType(&entry.mPrefix).ToString().AsCString(), entry.mValidLifetime, entry.mPreferredLifetime,
                AsCoreType(&entry.mRouter.mAddress).ToString().AsCString(), entry.mMsecSinceLastUpdate / 1000);

            onLinkPrefixCount++;

            for (uint16_t index = 0; index < aNumOnLinkPrefixes; index++)
            {
                const OnLinkPrefix &onLinkPrefix = aOnLinkPrefixes[index];

                if ((onLinkPrefix.mPrefix == AsCoreType(&entry.mPrefix)) &&
                    (AsCoreType(&entry.mRouter.mAddress) == onLinkPrefix.mRouterAddress))
                {
                    VerifyOrQuit(entry.mValidLifetime == onLinkPrefix.mValidLifetime);
                    VerifyOrQuit(entry.mPreferredLifetime == onLinkPrefix.mPreferredLifetime);
                    didFind = true;
                    break;
                }
            }
        }
        else
        {
            Log("   route prefix:%s, valid:%u, prf:%s, router:%s, age:%u",
                AsCoreType(&entry.mPrefix).ToString().AsCString(), entry.mValidLifetime,
                PreferenceToString(entry.mRoutePreference), AsCoreType(&entry.mRouter.mAddress).ToString().AsCString(),
                entry.mMsecSinceLastUpdate / 1000);

            routePrefixCount++;

            for (uint16_t index = 0; index < aNumRoutePrefixes; index++)
            {
                const RoutePrefix &routePrefix = aRoutePrefixes[index];

                if ((routePrefix.mPrefix == AsCoreType(&entry.mPrefix)) &&
                    (AsCoreType(&entry.mRouter.mAddress) == routePrefix.mRouterAddress))
                {
                    VerifyOrQuit(entry.mValidLifetime == routePrefix.mValidLifetime);
                    VerifyOrQuit(static_cast<int8_t>(entry.mRoutePreference) == routePrefix.mPreference);
                    didFind = true;
                    break;
                }
            }
        }

        VerifyOrQuit(didFind);
    }

    VerifyOrQuit(onLinkPrefixCount == aNumOnLinkPrefixes);
    VerifyOrQuit(routePrefixCount == aNumRoutePrefixes);
}

void VerifyPrefixTableIsEmpty(void) { VerifyPrefixTable(nullptr, 0, nullptr, 0); }

struct InfraRouter
{
    InfraRouter(const Ip6::Address &aAddress,
                bool                aManagedAddressConfigFlag,
                bool                aOtherConfigFlag,
                bool                aStubRouterFlag)
        : mAddress(aAddress)
    {
        mFlags.Clear();
        mFlags.mManagedAddressConfigFlag = aManagedAddressConfigFlag;
        mFlags.mOtherConfigFlag          = aOtherConfigFlag;
        mFlags.mStubRouterFlag           = aStubRouterFlag;
    }

    Ip6::Address mAddress;
    RaFlags      mFlags;
};

template <uint16_t kNumRouters> void VerifyDiscoveredRouters(const InfraRouter (&aRouters)[kNumRouters])
{
    VerifyDiscoveredRouters(aRouters, kNumRouters);
}

void VerifyDiscoveredRouters(const InfraRouter *aRouters, uint16_t aNumRouters)
{
    BorderRouter::RoutingManager::PrefixTableIterator iter;
    BorderRouter::RoutingManager::RouterEntry         entry;
    uint16_t                                          count = 0;

    Log("VerifyDiscoveredRouters()");

    sInstance->Get<BorderRouter::RoutingManager>().InitPrefixTableIterator(iter);

    while (sInstance->Get<BorderRouter::RoutingManager>().GetNextRouterEntry(iter, entry) == kErrorNone)
    {
        bool didFind = false;

        Log("   address:%s, M:%u, O:%u, StubRouter:%u", AsCoreType(&entry.mAddress).ToString().AsCString(),
            entry.mManagedAddressConfigFlag, entry.mOtherConfigFlag, entry.mStubRouterFlag);

        for (uint16_t index = 0; index < aNumRouters; index++)
        {
            if (AsCoreType(&entry.mAddress) == aRouters[index].mAddress)
            {
                VerifyOrQuit(entry.mManagedAddressConfigFlag == aRouters[index].mFlags.mManagedAddressConfigFlag);
                VerifyOrQuit(entry.mOtherConfigFlag == aRouters[index].mFlags.mOtherConfigFlag);
                VerifyOrQuit(entry.mStubRouterFlag == aRouters[index].mFlags.mStubRouterFlag);
                didFind = true;
            }
        }

        VerifyOrQuit(didFind);
        count++;
    }

    VerifyOrQuit(count == aNumRouters);
}

void VerifyDiscoveredRoutersIsEmpty(void) { VerifyDiscoveredRouters(nullptr, 0); }

void InitTest(bool aEnablBorderRouting = false, bool aAfterReset = false)
{
    uint32_t delay = 10000;

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // Initialize OT instance.

    sNow      = 0;
    sAlarmOn  = false;
    sInstance = static_cast<Instance *>(testInitInstance());

    if (aAfterReset)
    {
        delay += 26000; // leader reset sync delay
    }

    memset(&sRadioTxFrame, 0, sizeof(sRadioTxFrame));
    sRadioTxFrame.mPsdu = sRadioTxFramePsdu;
    sRadioTxOngoing     = false;

    SuccessOrQuit(sInfraIfAddress.FromString(kInfraIfAddress));

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // Initialize and start Border Router and Thread operation.

    SuccessOrQuit(otBorderRoutingInit(sInstance, kInfraIfIndex, /* aInfraIfIsRunning */ true));

    otOperationalDatasetTlvs datasetTlvs;

    SuccessOrQuit(otDatasetConvertToTlvs(&kDataset, &datasetTlvs));
    SuccessOrQuit(otDatasetSetActiveTlvs(sInstance, &datasetTlvs));

    SuccessOrQuit(otIp6SetEnabled(sInstance, true));
    SuccessOrQuit(otThreadSetEnabled(sInstance, true));
    SuccessOrQuit(otBorderRoutingSetEnabled(sInstance, aEnablBorderRouting));

    // Reset all test flags
    sRsEmitted   = false;
    sRaValidated = false;
    sExpectedPio = kNoPio;
    sExpectedRios.Clear();
    sRespondToNs           = true;
    sExpectedRaHeaderFlags = kRaHeaderFlagsNone;

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // Ensure device starts as leader.

    AdvanceTime(delay);

    VerifyOrQuit(otThreadGetDeviceRole(sInstance) == OT_DEVICE_ROLE_LEADER);
}

void FinalizeTest(void)
{
    SuccessOrQuit(otIp6SetEnabled(sInstance, false));
    SuccessOrQuit(otThreadSetEnabled(sInstance, false));
    SuccessOrQuit(otInstanceErasePersistentInfo(sInstance));
    testFreeInstance(sInstance);
}

//---------------------------------------------------------------------------------------------------------------------

void TestSamePrefixesFromMultipleRouters(void)
{
    Ip6::Prefix  localOnLink;
    Ip6::Prefix  localOmr;
    Ip6::Prefix  onLinkPrefix   = PrefixFromString("2000:abba:baba::", 64);
    Ip6::Prefix  routePrefix    = PrefixFromString("2000:1234:5678::", 64);
    Ip6::Address routerAddressA = AddressFromString("fd00::aaaa");
    Ip6::Address routerAddressB = AddressFromString("fd00::bbbb");
    uint16_t     heapAllocations;

    Log("--------------------------------------------------------------------------------------------");
    Log("TestSamePrefixesFromMultipleRouters");

    InitTest();

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // Start Routing Manager. Check emitted RS and RA messages.

    sRsEmitted   = false;
    sRaValidated = false;
    sExpectedPio = kPioAdvertisingLocalOnLink;
    sExpectedRios.Clear();

    heapAllocations = sHeapAllocatedPtrs.GetLength();
    SuccessOrQuit(sInstance->Get<BorderRouter::RoutingManager>().SetEnabled(true));

    SuccessOrQuit(sInstance->Get<BorderRouter::RoutingManager>().GetOnLinkPrefix(localOnLink));
    SuccessOrQuit(sInstance->Get<BorderRouter::RoutingManager>().GetOmrPrefix(localOmr));

    Log("Local on-link prefix is %s", localOnLink.ToString().AsCString());
    Log("Local OMR prefix is %s", localOmr.ToString().AsCString());

    sExpectedRios.Add(localOmr);

    AdvanceTime(30000);

    VerifyOrQuit(sRsEmitted);
    VerifyOrQuit(sRaValidated);
    VerifyOrQuit(sExpectedRios.SawAll());
    Log("Received RA was validated");

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // Check Network Data to include the local OMR and on-link prefix.

    VerifyOmrPrefixInNetData(localOmr, /* aDefaultRoute */ false);
    VerifyExternalRouteInNetData(kUlaRoute, kWithAdvPioFlagSet);

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // Send an RA from router A with a new on-link (PIO) and route prefix (RIO).

    SendRouterAdvert(routerAddressA, {Pio(onLinkPrefix, kValidLitime, kPreferredLifetime)},
                     {Rio(routePrefix, kValidLitime, NetworkData::kRoutePreferenceMedium)});

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // Check that the local on-link prefix is now deprecating in the new RA.

    sRaValidated = false;
    sExpectedPio = kPioDeprecatingLocalOnLink;

    AdvanceTime(10000);
    VerifyOrQuit(sRaValidated);

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // Check the discovered prefix table and ensure info from router A
    // is present in the table.

    VerifyPrefixTable({OnLinkPrefix(onLinkPrefix, kValidLitime, kPreferredLifetime, routerAddressA)},
                      {RoutePrefix(routePrefix, kValidLitime, NetworkData::kRoutePreferenceMedium, routerAddressA)});

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // Check Network Data to include new prefixes from router A.

    VerifyOmrPrefixInNetData(localOmr, /* aDefaultRoute */ true);
    VerifyExternalRouteInNetData(kDefaultRoute, kWithAdvPioFlagSet);

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // Send the same RA again from router A with the on-link (PIO) and route prefix (RIO).

    SendRouterAdvert(routerAddressA, {Pio(onLinkPrefix, kValidLitime, kPreferredLifetime)},
                     {Rio(routePrefix, kValidLitime, NetworkData::kRoutePreferenceMedium)});

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // Check the discovered prefix table and ensure info from router A
    // remains unchanged.

    VerifyPrefixTable({OnLinkPrefix(onLinkPrefix, kValidLitime, kPreferredLifetime, routerAddressA)},
                      {RoutePrefix(routePrefix, kValidLitime, NetworkData::kRoutePreferenceMedium, routerAddressA)});

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // Send an RA from router B with same route prefix (RIO) but with
    // high route preference.

    SendRouterAdvert(routerAddressB, {Rio(routePrefix, kValidLitime, NetworkData::kRoutePreferenceHigh)});

    AdvanceTime(10000);

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // Check the discovered prefix table and ensure info from router B
    // is also included in the table.

    VerifyPrefixTable({OnLinkPrefix(onLinkPrefix, kValidLitime, kPreferredLifetime, routerAddressA)},
                      {RoutePrefix(routePrefix, kValidLitime, NetworkData::kRoutePreferenceMedium, routerAddressA),
                       RoutePrefix(routePrefix, kValidLitime, NetworkData::kRoutePreferenceHigh, routerAddressB)});

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // Check Network Data.

    VerifyOmrPrefixInNetData(localOmr, /* aDefaultRoute */ true);
    VerifyExternalRouteInNetData(kDefaultRoute, kWithAdvPioFlagSet);

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // Send an RA from router B removing the route prefix.

    SendRouterAdvert(routerAddressB, {Rio(routePrefix, 0, NetworkData::kRoutePreferenceHigh)});

    AdvanceTime(10000);

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // Check the discovered prefix table and ensure info from router B
    // is now removed from the table.

    VerifyPrefixTable({OnLinkPrefix(onLinkPrefix, kValidLitime, kPreferredLifetime, routerAddressA)},
                      {RoutePrefix(routePrefix, kValidLitime, NetworkData::kRoutePreferenceMedium, routerAddressA)});

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // Check Network Data.

    VerifyExternalRouteInNetData(kDefaultRoute, kWithAdvPioFlagSet);

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

    SuccessOrQuit(sInstance->Get<BorderRouter::RoutingManager>().SetEnabled(false));
    VerifyOrQuit(heapAllocations == sHeapAllocatedPtrs.GetLength());

    Log("End of TestSamePrefixesFromMultipleRouters");

    FinalizeTest();
}

void TestOmrSelection(void)
{
    Ip6::Prefix                     localOnLink;
    Ip6::Prefix                     localOmr;
    Ip6::Prefix                     omrPrefix = PrefixFromString("2000:0000:1111:4444::", 64);
    NetworkData::OnMeshPrefixConfig prefixConfig;
    uint16_t                        heapAllocations;

    Log("--------------------------------------------------------------------------------------------");
    Log("TestOmrSelection");

    InitTest();

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // Start Routing Manager. Check emitted RS and RA messages.

    sRsEmitted   = false;
    sRaValidated = false;
    sExpectedPio = kPioAdvertisingLocalOnLink;
    sExpectedRios.Clear();

    heapAllocations = sHeapAllocatedPtrs.GetLength();
    SuccessOrQuit(sInstance->Get<BorderRouter::RoutingManager>().SetEnabled(true));

    SuccessOrQuit(sInstance->Get<BorderRouter::RoutingManager>().GetOnLinkPrefix(localOnLink));
    SuccessOrQuit(sInstance->Get<BorderRouter::RoutingManager>().GetOmrPrefix(localOmr));

    Log("Local on-link prefix is %s", localOnLink.ToString().AsCString());
    Log("Local OMR prefix is %s", localOmr.ToString().AsCString());

    sExpectedRios.Add(localOmr);

    AdvanceTime(30000);

    VerifyOrQuit(sRsEmitted);
    VerifyOrQuit(sRaValidated);
    VerifyOrQuit(sExpectedRios.SawAll());
    VerifyOrQuit(sExpectedRios[0].mLifetime == kRioValidLifetime);

    Log("Received RA was validated");

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // Check Network Data to include the local OMR and on-link prefix.

    VerifyOmrPrefixInNetData(localOmr, /* aDefaultRoute */ false);
    VerifyExternalRouteInNetData(kUlaRoute, kWithAdvPioFlagSet);

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // Add a new OMR prefix directly into net data. The new prefix should
    // be favored over the local OMR prefix.

    prefixConfig.Clear();
    prefixConfig.mPrefix       = omrPrefix;
    prefixConfig.mStable       = true;
    prefixConfig.mSlaac        = true;
    prefixConfig.mPreferred    = true;
    prefixConfig.mOnMesh       = true;
    prefixConfig.mDefaultRoute = false;
    prefixConfig.mPreference   = NetworkData::kRoutePreferenceMedium;

    SuccessOrQuit(otBorderRouterAddOnMeshPrefix(sInstance, &prefixConfig));
    SuccessOrQuit(otBorderRouterRegister(sInstance));

    AdvanceTime(100);

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // Make sure BR emits RA with the new OMR prefix now, and deprecates the old OMR prefix.

    sRaValidated = false;
    sExpectedPio = kPioAdvertisingLocalOnLink;
    sExpectedRios.Clear();
    sExpectedRios.Add(omrPrefix);
    sExpectedRios.Add(localOmr);

    AdvanceTime(20000);

    VerifyOrQuit(sRaValidated);
    VerifyOrQuit(sExpectedRios.SawAll());
    VerifyOrQuit(sExpectedRios[0].mLifetime == kRioValidLifetime);
    VerifyOrQuit(sExpectedRios[1].mLifetime <= kRioDeprecatingLifetime);

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // Check Network Data. We should now see that the local OMR prefix
    // is removed.

    VerifyOmrPrefixInNetData(omrPrefix, /* aDefaultRoute */ false);
    VerifyExternalRouteInNetData(kUlaRoute, kWithAdvPioFlagSet);

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // Remove the OMR prefix previously added in net data.

    SuccessOrQuit(otBorderRouterRemoveOnMeshPrefix(sInstance, &omrPrefix));
    SuccessOrQuit(otBorderRouterRegister(sInstance));

    AdvanceTime(100);

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // Make sure BR emits RA with local OMR prefix again and start
    // deprecating the previously added OMR prefix.

    sRaValidated = false;
    sExpectedRios.Clear();
    sExpectedRios.Add(omrPrefix);
    sExpectedRios.Add(localOmr);

    AdvanceTime(20000);

    VerifyOrQuit(sRaValidated);
    VerifyOrQuit(sExpectedRios.SawAll());
    VerifyOrQuit(sExpectedRios[0].mLifetime <= kRioDeprecatingLifetime);
    VerifyOrQuit(sExpectedRios[1].mLifetime == kRioValidLifetime);

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // Check Network Data. We should see that the local OMR prefix is
    // added again.

    VerifyOmrPrefixInNetData(localOmr, /* aDefaultRoute */ false);
    VerifyExternalRouteInNetData(kUlaRoute, kWithAdvPioFlagSet);

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // Wait enough for old deprecating OMR prefix deprecating to expire.

    sRaValidated = false;
    sExpectedRios.Clear();
    sExpectedRios.Add(omrPrefix);
    sExpectedRios.Add(localOmr);

    AdvanceTime(310000);

    VerifyOrQuit(sRaValidated);
    VerifyOrQuit(sExpectedRios.SawAll());
    VerifyOrQuit(sExpectedRios[0].mLifetime == 0);
    VerifyOrQuit(sExpectedRios[1].mLifetime == kRioValidLifetime);

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

    SuccessOrQuit(sInstance->Get<BorderRouter::RoutingManager>().SetEnabled(false));
    VerifyOrQuit(heapAllocations == sHeapAllocatedPtrs.GetLength());

    Log("End of TestOmrSelection");
    FinalizeTest();
}

void TestDefaultRoute(void)
{
    Ip6::Prefix                     localOnLink;
    Ip6::Prefix                     localOmr;
    Ip6::Prefix                     omrPrefix      = PrefixFromString("2000:0000:1111:4444::", 64);
    Ip6::Prefix                     defaultRoute   = PrefixFromString("::", 0);
    Ip6::Address                    routerAddressA = AddressFromString("fd00::aaaa");
    NetworkData::OnMeshPrefixConfig prefixConfig;
    uint16_t                        heapAllocations;

    Log("--------------------------------------------------------------------------------------------");
    Log("TestDefaultRoute");

    InitTest();

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // Start Routing Manager. Check emitted RS and RA messages.

    sRsEmitted   = false;
    sRaValidated = false;
    sExpectedPio = kPioAdvertisingLocalOnLink;
    sExpectedRios.Clear();

    heapAllocations = sHeapAllocatedPtrs.GetLength();
    SuccessOrQuit(sInstance->Get<BorderRouter::RoutingManager>().SetEnabled(true));

    SuccessOrQuit(sInstance->Get<BorderRouter::RoutingManager>().GetOnLinkPrefix(localOnLink));
    SuccessOrQuit(sInstance->Get<BorderRouter::RoutingManager>().GetOmrPrefix(localOmr));

    Log("Local on-link prefix is %s", localOnLink.ToString().AsCString());
    Log("Local OMR prefix is %s", localOmr.ToString().AsCString());

    sExpectedRios.Add(localOmr);

    AdvanceTime(30000);

    VerifyOrQuit(sRsEmitted);
    VerifyOrQuit(sRaValidated);
    VerifyOrQuit(sExpectedRios.SawAll());
    Log("Received RA was validated");

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // Check Network Data to include the local OMR and ULA prefix.

    VerifyOmrPrefixInNetData(localOmr, /* aDefaultRoute */ false);
    VerifyExternalRouteInNetData(kUlaRoute, kWithAdvPioFlagSet);

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // Send RA from router A advertising a default route.

    SendRouterAdvert(routerAddressA, DefaultRoute(kValidLitime, NetworkData::kRoutePreferenceLow));

    AdvanceTime(10000);

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // Check the discovered prefix table and ensure default route
    // from router A is in the table.

    VerifyPrefixTable({RoutePrefix(defaultRoute, kValidLitime, NetworkData::kRoutePreferenceLow, routerAddressA)});

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // Check Network Data. We should not see default route in
    // Network Data yet since there is no infrastructure-derived
    // OMR prefix (with preference medium or higher).

    VerifyOmrPrefixInNetData(localOmr, /* aDefaultRoute */ false);
    VerifyExternalRouteInNetData(kUlaRoute, kWithAdvPioFlagSet);

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // Add an OMR prefix directly into Network Data with
    // preference medium (infrastructure-derived).

    prefixConfig.Clear();
    prefixConfig.mPrefix       = omrPrefix;
    prefixConfig.mStable       = true;
    prefixConfig.mSlaac        = true;
    prefixConfig.mPreferred    = true;
    prefixConfig.mOnMesh       = true;
    prefixConfig.mDefaultRoute = true;
    prefixConfig.mPreference   = NetworkData::kRoutePreferenceMedium;

    SuccessOrQuit(otBorderRouterAddOnMeshPrefix(sInstance, &prefixConfig));
    SuccessOrQuit(otBorderRouterRegister(sInstance));

    AdvanceTime(10000);

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // Check Network Data. Now that we have an infrastructure-derived
    // OMR prefix, the default route should be published.

    VerifyOmrPrefixInNetData(omrPrefix, /* aDefaultRoute */ true);
    VerifyExternalRouteInNetData(kDefaultRoute, kWithAdvPioFlagSet);

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // Remove the OMR prefix from Network Data.

    SuccessOrQuit(otBorderRouterRemoveOnMeshPrefix(sInstance, &omrPrefix));
    SuccessOrQuit(otBorderRouterRegister(sInstance));

    AdvanceTime(10000);

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // Check Network Data. We should again go back to ULA prefix. The
    // default route advertised by router A should be still present in
    // the discovered prefix table.

    VerifyOmrPrefixInNetData(localOmr, /* aDefaultRoute */ false);
    VerifyExternalRouteInNetData(kUlaRoute, kWithAdvPioFlagSet);

    VerifyPrefixTable({RoutePrefix(defaultRoute, kValidLitime, NetworkData::kRoutePreferenceLow, routerAddressA)});

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // Add the OMR prefix again.

    SuccessOrQuit(otBorderRouterAddOnMeshPrefix(sInstance, &prefixConfig));
    SuccessOrQuit(otBorderRouterRegister(sInstance));

    AdvanceTime(10000);

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // Check Network Data. Again the default route should be published.

    VerifyOmrPrefixInNetData(omrPrefix, /* aDefaultRoute */ true);
    VerifyExternalRouteInNetData(kDefaultRoute, kWithAdvPioFlagSet);

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // Send RA from router A removing the default route.

    SendRouterAdvert(routerAddressA, DefaultRoute(0, NetworkData::kRoutePreferenceLow));

    AdvanceTime(10000);

    VerifyPrefixTableIsEmpty();

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // Check Network Data. Now that router A no longer advertised
    // a default-route, we should go back to publishing ULA route.

    VerifyOmrPrefixInNetData(omrPrefix, /* aDefaultRoute */ true);
    VerifyExternalRouteInNetData(kUlaRoute, kWithAdvPioFlagSet);

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // Send RA from router A again advertising a default route.

    SendRouterAdvert(routerAddressA, DefaultRoute(kValidLitime, NetworkData::kRoutePreferenceLow));

    AdvanceTime(10000);

    VerifyPrefixTable({RoutePrefix(defaultRoute, kValidLitime, NetworkData::kRoutePreferenceLow, routerAddressA)});

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // Check Network Data. We should see default route published.

    VerifyOmrPrefixInNetData(omrPrefix, /* aDefaultRoute */ true);
    VerifyExternalRouteInNetData(kDefaultRoute, kWithAdvPioFlagSet);

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

    SuccessOrQuit(sInstance->Get<BorderRouter::RoutingManager>().SetEnabled(false));
    VerifyOrQuit(heapAllocations == sHeapAllocatedPtrs.GetLength());

    Log("End of TestDefaultRoute");

    FinalizeTest();
}

void TestAdvNonUlaRoute(void)
{
    Ip6::Prefix                     localOnLink;
    Ip6::Prefix                     localOmr;
    Ip6::Prefix                     omrPrefix      = PrefixFromString("2000:0000:1111:4444::", 64);
    Ip6::Prefix                     routePrefix    = PrefixFromString("2000:1234:5678::", 64);
    Ip6::Address                    routerAddressA = AddressFromString("fd00::aaaa");
    NetworkData::OnMeshPrefixConfig prefixConfig;
    uint16_t                        heapAllocations;

    Log("--------------------------------------------------------------------------------------------");
    Log("TestAdvNonUlaRoute");

    InitTest();

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // Start Routing Manager. Check emitted RS and RA messages.

    sRsEmitted   = false;
    sRaValidated = false;
    sExpectedPio = kPioAdvertisingLocalOnLink;
    sExpectedRios.Clear();

    heapAllocations = sHeapAllocatedPtrs.GetLength();
    SuccessOrQuit(sInstance->Get<BorderRouter::RoutingManager>().SetEnabled(true));

    SuccessOrQuit(sInstance->Get<BorderRouter::RoutingManager>().GetOnLinkPrefix(localOnLink));
    SuccessOrQuit(sInstance->Get<BorderRouter::RoutingManager>().GetOmrPrefix(localOmr));

    Log("Local on-link prefix is %s", localOnLink.ToString().AsCString());
    Log("Local OMR prefix is %s", localOmr.ToString().AsCString());

    sExpectedRios.Add(localOmr);

    AdvanceTime(30000);

    VerifyOrQuit(sRsEmitted);
    VerifyOrQuit(sRaValidated);
    VerifyOrQuit(sExpectedRios.SawAll());
    Log("Received RA was validated");

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // Check Network Data to include the local OMR and ULA prefix.

    VerifyOmrPrefixInNetData(localOmr, /* aDefaultRoute */ false);
    VerifyExternalRouteInNetData(kUlaRoute, kWithAdvPioFlagSet);

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // Send RA from router A advertising a non-ULA.

    SendRouterAdvert(routerAddressA, {Rio(routePrefix, kValidLitime, NetworkData::kRoutePreferenceMedium)});

    AdvanceTime(10000);

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // Check the discovered prefix table and ensure the non-ULA
    // from router A is in the table.

    VerifyPrefixTable({RoutePrefix(routePrefix, kValidLitime, NetworkData::kRoutePreferenceMedium, routerAddressA)});

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // Check Network Data. We should not see default route in
    // Network Data yet since there is no infrastructure-derived
    // OMR prefix (with preference medium or higher).

    VerifyOmrPrefixInNetData(localOmr, /* aDefaultRoute */ false);
    VerifyExternalRouteInNetData(kUlaRoute, kWithAdvPioFlagSet);

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // Add an OMR prefix directly into Network Data with
    // preference medium (infrastructure-derived).

    prefixConfig.Clear();
    prefixConfig.mPrefix       = omrPrefix;
    prefixConfig.mStable       = true;
    prefixConfig.mSlaac        = true;
    prefixConfig.mPreferred    = true;
    prefixConfig.mOnMesh       = true;
    prefixConfig.mDefaultRoute = true;
    prefixConfig.mPreference   = NetworkData::kRoutePreferenceMedium;

    SuccessOrQuit(otBorderRouterAddOnMeshPrefix(sInstance, &prefixConfig));
    SuccessOrQuit(otBorderRouterRegister(sInstance));

    AdvanceTime(10000);

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // Check Network Data. Now that we have an infrastructure-derived
    // OMR prefix, the default route should be published.

    VerifyOmrPrefixInNetData(omrPrefix, /* aDefaultRoute */ true);
    VerifyExternalRouteInNetData(kDefaultRoute, kWithAdvPioFlagSet);

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // Remove the OMR prefix from Network Data.

    SuccessOrQuit(otBorderRouterRemoveOnMeshPrefix(sInstance, &omrPrefix));
    SuccessOrQuit(otBorderRouterRegister(sInstance));

    AdvanceTime(10000);

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // Check Network Data. We should again go back to ULA prefix. The
    // non-ULA route advertised by router A should be still present in
    // the discovered prefix table.

    VerifyOmrPrefixInNetData(localOmr, /* aDefaultRoute */ false);
    VerifyExternalRouteInNetData(kUlaRoute, kWithAdvPioFlagSet);

    VerifyPrefixTable({RoutePrefix(routePrefix, kValidLitime, NetworkData::kRoutePreferenceMedium, routerAddressA)});

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // Add the OMR prefix again.

    SuccessOrQuit(otBorderRouterAddOnMeshPrefix(sInstance, &prefixConfig));
    SuccessOrQuit(otBorderRouterRegister(sInstance));

    AdvanceTime(10000);

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // Check Network Data. Again the default route should be published.

    VerifyOmrPrefixInNetData(omrPrefix, /* aDefaultRoute */ true);
    VerifyExternalRouteInNetData(kDefaultRoute, kWithAdvPioFlagSet);

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // Send RA from router A removing the route.

    SendRouterAdvert(routerAddressA, {Rio(routePrefix, 0, NetworkData::kRoutePreferenceMedium)});

    AdvanceTime(10000);

    VerifyPrefixTableIsEmpty();

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // Check Network Data. Now that router A no longer advertised
    // the route, we should go back to publishing the ULA route.

    VerifyOmrPrefixInNetData(omrPrefix, /* aDefaultRoute */ true);
    VerifyExternalRouteInNetData(kUlaRoute, kWithAdvPioFlagSet);

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // Send RA from router A again advertising the route again.

    SendRouterAdvert(routerAddressA, {Rio(routePrefix, kValidLitime, NetworkData::kRoutePreferenceMedium)});

    AdvanceTime(10000);

    VerifyPrefixTable({RoutePrefix(routePrefix, kValidLitime, NetworkData::kRoutePreferenceMedium, routerAddressA)});

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // Check Network Data. We should see default route published.

    VerifyOmrPrefixInNetData(omrPrefix, /* aDefaultRoute */ true);
    VerifyExternalRouteInNetData(kDefaultRoute, kWithAdvPioFlagSet);

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

    SuccessOrQuit(sInstance->Get<BorderRouter::RoutingManager>().SetEnabled(false));
    VerifyOrQuit(heapAllocations == sHeapAllocatedPtrs.GetLength());

    Log("End of TestAdvNonUlaRoute");

    FinalizeTest();
}

void TestLocalOnLinkPrefixDeprecation(void)
{
    static constexpr uint32_t kMaxRaTxInterval = 601; // In seconds

    Ip6::Prefix  localOnLink;
    Ip6::Prefix  localOmr;
    Ip6::Prefix  onLinkPrefix   = PrefixFromString("fd00:abba:baba::", 64);
    Ip6::Address routerAddressA = AddressFromString("fd00::aaaa");
    uint32_t     localOnLinkLifetime;
    uint16_t     heapAllocations;

    Log("--------------------------------------------------------------------------------------------");
    Log("TestLocalOnLinkPrefixDeprecation");

    InitTest();

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // Start Routing Manager. Check emitted RS and RA messages.

    heapAllocations = sHeapAllocatedPtrs.GetLength();
    SuccessOrQuit(sInstance->Get<BorderRouter::RoutingManager>().SetEnabled(true));

    SuccessOrQuit(sInstance->Get<BorderRouter::RoutingManager>().GetOnLinkPrefix(localOnLink));
    SuccessOrQuit(sInstance->Get<BorderRouter::RoutingManager>().GetOmrPrefix(localOmr));

    Log("Local on-link prefix is %s", localOnLink.ToString().AsCString());
    Log("Local OMR prefix is %s", localOmr.ToString().AsCString());

    sRsEmitted   = false;
    sRaValidated = false;
    sExpectedPio = kPioAdvertisingLocalOnLink;
    sExpectedRios.Clear();
    sExpectedRios.Add(localOmr);

    AdvanceTime(30000);

    VerifyOrQuit(sRsEmitted);
    VerifyOrQuit(sRaValidated);
    VerifyOrQuit(sExpectedRios.SawAll());
    Log("Local on-link prefix is being advertised, lifetime: %d", sOnLinkLifetime);
    localOnLinkLifetime = sOnLinkLifetime;

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // Check Network Data to include the local OMR and on-link prefix.

    VerifyOmrPrefixInNetData(localOmr, /* aDefaultRoute */ false);
    VerifyExternalRouteInNetData(kUlaRoute, kWithAdvPioFlagSet);

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // Send an RA from router A with a new on-link (PIO) which is preferred over
    // the local on-link prefix.

    SendRouterAdvert(routerAddressA, {Pio(onLinkPrefix, kValidLitime, kPreferredLifetime)});

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // Check that the local on-link prefix is now deprecating in the new RA.

    sRaValidated = false;
    sExpectedPio = kPioDeprecatingLocalOnLink;
    sExpectedRios.Clear();
    sExpectedRios.Add(localOmr);

    AdvanceTime(10000);
    VerifyOrQuit(sRaValidated);
    VerifyOrQuit(sExpectedRios.SawAll());
    Log("On-link prefix is deprecating, remaining lifetime:%d", sOnLinkLifetime);
    VerifyOrQuit(sOnLinkLifetime < localOnLinkLifetime);
    localOnLinkLifetime = sOnLinkLifetime;

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // Check Network Data. We must see the new on-link prefix from router A
    // along with the deprecating local on-link prefix.

    VerifyOmrPrefixInNetData(localOmr, /* aDefaultRoute */ false);
    VerifyExternalRouteInNetData(kUlaRoute, kWithAdvPioFlagSet);

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // Wait for local on-link prefix to expire

    while (localOnLinkLifetime > kMaxRaTxInterval)
    {
        // Send same RA from router A to keep the on-link prefix alive.

        SendRouterAdvert(routerAddressA, {Pio(onLinkPrefix, kValidLitime, kPreferredLifetime)});

        // Ensure Network Data entries remain as before. Mainly we still
        // see the deprecating local on-link prefix.

        VerifyOmrPrefixInNetData(localOmr, /* aDefaultRoute */ false);
        VerifyExternalRouteInNetData(kUlaRoute, kSkipAdvPioCheck);

        // Keep checking the emitted RAs and make sure on-link prefix
        // is included with smaller lifetime every time.

        sRaValidated = false;
        sExpectedPio = kPioDeprecatingLocalOnLink;
        sExpectedRios.Clear();
        sExpectedRios.Add(localOmr);

        AdvanceTime(kMaxRaTxInterval * 1000);

        VerifyOrQuit(sRaValidated);
        VerifyOrQuit(sExpectedRios.SawAll());
        Log("On-link prefix is deprecating, remaining lifetime:%d", sOnLinkLifetime);
        VerifyOrQuit(sOnLinkLifetime < localOnLinkLifetime);
        localOnLinkLifetime = sOnLinkLifetime;
    }

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // The local on-link prefix must be expired and should no
    // longer be seen in the emitted RA message.

    sRaValidated = false;
    sExpectedPio = kNoPio;
    sExpectedRios.Clear();
    sExpectedRios.Add(localOmr);

    AdvanceTime(kMaxRaTxInterval * 1000);

    VerifyOrQuit(sRaValidated);
    VerifyOrQuit(sExpectedRios.SawAll());
    Log("On-link prefix is now expired");

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // Check Network Data.

    VerifyOmrPrefixInNetData(localOmr, /* aDefaultRoute */ false);
    VerifyExternalRouteInNetData(kUlaRoute, kWithAdvPioCleared);

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

    SuccessOrQuit(sInstance->Get<BorderRouter::RoutingManager>().SetEnabled(false));
    VerifyOrQuit(heapAllocations == sHeapAllocatedPtrs.GetLength());

    Log("End of TestLocalOnLinkPrefixDeprecation");

    FinalizeTest();
}

#if OPENTHREAD_CONFIG_BACKBONE_ROUTER_ENABLE
void TestDomainPrefixAsOmr(void)
{
    Ip6::Prefix                     localOnLink;
    Ip6::Prefix                     localOmr;
    Ip6::Prefix                     domainPrefix = PrefixFromString("2000:0000:1111:4444::", 64);
    NetworkData::OnMeshPrefixConfig prefixConfig;
    uint16_t                        heapAllocations;

    Log("--------------------------------------------------------------------------------------------");
    Log("TestDomainPrefixAsOmr");

    InitTest();

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // Start Routing Manager. Check emitted RS and RA messages.

    sRsEmitted   = false;
    sRaValidated = false;
    sExpectedPio = kPioAdvertisingLocalOnLink;
    sExpectedRios.Clear();

    heapAllocations = sHeapAllocatedPtrs.GetLength();
    SuccessOrQuit(sInstance->Get<BorderRouter::RoutingManager>().SetEnabled(true));

    SuccessOrQuit(sInstance->Get<BorderRouter::RoutingManager>().GetOnLinkPrefix(localOnLink));
    SuccessOrQuit(sInstance->Get<BorderRouter::RoutingManager>().GetOmrPrefix(localOmr));

    Log("Local on-link prefix is %s", localOnLink.ToString().AsCString());
    Log("Local OMR prefix is %s", localOmr.ToString().AsCString());

    sExpectedRios.Add(localOmr);

    AdvanceTime(30000);

    VerifyOrQuit(sRsEmitted);
    VerifyOrQuit(sRaValidated);
    VerifyOrQuit(sExpectedRios.SawAll());
    Log("Received RA was validated");

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // Check Network Data to include the local OMR and on-link prefix.

    VerifyOmrPrefixInNetData(localOmr, /* aDefaultRoute */ false);
    VerifyExternalRouteInNetData(kUlaRoute, kWithAdvPioFlagSet);

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // Add a domain prefix directly into net data. The new prefix should
    // be favored over the local OMR prefix.

    otBackboneRouterSetEnabled(sInstance, true);

    prefixConfig.Clear();
    prefixConfig.mPrefix       = domainPrefix;
    prefixConfig.mStable       = true;
    prefixConfig.mSlaac        = true;
    prefixConfig.mPreferred    = true;
    prefixConfig.mOnMesh       = true;
    prefixConfig.mDefaultRoute = false;
    prefixConfig.mDp           = true;
    prefixConfig.mPreference   = NetworkData::kRoutePreferenceMedium;

    SuccessOrQuit(otBorderRouterAddOnMeshPrefix(sInstance, &prefixConfig));
    SuccessOrQuit(otBorderRouterRegister(sInstance));

    AdvanceTime(100);

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // Make sure BR emits RA without domain prefix or previous local OMR.

    sRaValidated = false;
    sExpectedPio = kPioAdvertisingLocalOnLink;
    sExpectedRios.Clear();
    sExpectedRios.Add(domainPrefix);
    sExpectedRios.Add(localOmr);

    AdvanceTime(20000);

    VerifyOrQuit(sRaValidated);

    // We should see RIO removing the local OMR prefix with lifetime zero
    // and should not see the domain prefix as RIO.

    VerifyOrQuit(sExpectedRios[0].mPrefix == domainPrefix);
    VerifyOrQuit(!sExpectedRios[0].mSawInRa);

    VerifyOrQuit(sExpectedRios[1].mPrefix == localOmr);
    VerifyOrQuit(sExpectedRios[1].mSawInRa);
    VerifyOrQuit(sExpectedRios[1].mLifetime <= kRioDeprecatingLifetime);

    // Wait long enough for deprecating RIO prefix to expire
    AdvanceTime(3200000);

    sRaValidated = false;
    sExpectedPio = kPioAdvertisingLocalOnLink;
    sExpectedRios.Clear();
    sExpectedRios.Add(domainPrefix);
    sExpectedRios.Add(localOmr);

    // Wait for next RA (650 seconds).

    AdvanceTime(650000);

    VerifyOrQuit(sRaValidated);

    // We should not see either domain prefix or local OMR
    // as RIO.

    VerifyOrQuit(!sExpectedRios[0].mSawInRa);
    VerifyOrQuit(!sExpectedRios[1].mSawInRa);

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // Check Network Data. We should now see that the local OMR prefix
    // is removed.

    VerifyOmrPrefixInNetData(domainPrefix, /* aDefaultRoute */ false);
    VerifyExternalRouteInNetData(kUlaRoute, kWithAdvPioFlagSet);

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // Remove the domain prefix from net data.

    SuccessOrQuit(otBorderRouterRemoveOnMeshPrefix(sInstance, &domainPrefix));
    SuccessOrQuit(otBorderRouterRegister(sInstance));

    AdvanceTime(100);

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // Make sure BR emits RA with local OMR prefix again.

    sRaValidated = false;
    sExpectedRios.Clear();
    sExpectedRios.Add(localOmr);

    AdvanceTime(20000);

    VerifyOrQuit(sRaValidated);
    VerifyOrQuit(sExpectedRios.SawAll());

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // Check Network Data. We should see that the local OMR prefix is
    // added again.

    VerifyOmrPrefixInNetData(localOmr, /* aDefaultRoute */ false);
    VerifyExternalRouteInNetData(kUlaRoute, kWithAdvPioFlagSet);

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

    SuccessOrQuit(sInstance->Get<BorderRouter::RoutingManager>().SetEnabled(false));
    VerifyOrQuit(heapAllocations == sHeapAllocatedPtrs.GetLength());

    Log("End of TestDomainPrefixAsOmr");
    FinalizeTest();
}
#endif // OPENTHREAD_CONFIG_BACKBONE_ROUTER_ENABLE

void TestExtPanIdChange(void)
{
    static constexpr uint32_t kMaxRaTxInterval = 601; // In seconds

    static const otExtendedPanId kExtPanId1 = {{0x01, 0x02, 0x03, 0x04, 0x05, 0x6, 0x7, 0x08}};
    static const otExtendedPanId kExtPanId2 = {{0xaa, 0xbb, 0xcc, 0xdd, 0xee, 0xff, 0x99, 0x88}};
    static const otExtendedPanId kExtPanId3 = {{0x12, 0x34, 0x56, 0x78, 0x9a, 0xab, 0xcd, 0xef}};
    static const otExtendedPanId kExtPanId4 = {{0x44, 0x00, 0x44, 0x00, 0x44, 0x00, 0x44, 0x00}};
    static const otExtendedPanId kExtPanId5 = {{0x77, 0x88, 0x00, 0x00, 0x55, 0x55, 0x55, 0x55}};

    Ip6::Prefix          localOnLink;
    Ip6::Prefix          oldLocalOnLink;
    Ip6::Prefix          localOmr;
    Ip6::Prefix          onLinkPrefix   = PrefixFromString("2000:abba:baba::", 64);
    Ip6::Address         routerAddressA = AddressFromString("fd00::aaaa");
    uint32_t             oldPrefixLifetime;
    Ip6::Prefix          oldPrefixes[4];
    otOperationalDataset dataset;
    uint16_t             heapAllocations;

    Log("--------------------------------------------------------------------------------------------");
    Log("TestExtPanIdChange");

    InitTest();

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // Start Routing Manager. Check emitted RS and RA messages.

    heapAllocations = sHeapAllocatedPtrs.GetLength();
    SuccessOrQuit(sInstance->Get<BorderRouter::RoutingManager>().SetEnabled(true));

    SuccessOrQuit(sInstance->Get<BorderRouter::RoutingManager>().GetOnLinkPrefix(localOnLink));
    SuccessOrQuit(sInstance->Get<BorderRouter::RoutingManager>().GetOmrPrefix(localOmr));

    Log("Local on-link prefix is %s", localOnLink.ToString().AsCString());
    Log("Local OMR prefix is %s", localOmr.ToString().AsCString());

    sRsEmitted   = false;
    sRaValidated = false;
    sExpectedPio = kPioAdvertisingLocalOnLink;
    sExpectedRios.Clear();
    sExpectedRios.Add(localOmr);

    AdvanceTime(30000);

    VerifyOrQuit(sRsEmitted);
    VerifyOrQuit(sRaValidated);
    VerifyOrQuit(sExpectedRios.SawAll());
    Log("Local on-link prefix is being advertised, lifetime: %d", sOnLinkLifetime);

    //= = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = =
    // Check behavior when ext PAN ID changes while the local on-link is
    // being advertised.

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // Change the extended PAN ID.

    Log("Changing ext PAN ID");

    oldLocalOnLink    = localOnLink;
    oldPrefixLifetime = sOnLinkLifetime;

    sRaValidated = false;
    sExpectedPio = kPioAdvertisingLocalOnLink;

    SuccessOrQuit(otDatasetGetActive(sInstance, &dataset));

    VerifyOrQuit(dataset.mComponents.mIsExtendedPanIdPresent);

    dataset.mExtendedPanId = kExtPanId1;
    SuccessOrQuit(otDatasetSetActive(sInstance, &dataset));

    AdvanceTime(500);
    SuccessOrQuit(sInstance->Get<BorderRouter::RoutingManager>().GetOnLinkPrefix(localOnLink));
    Log("Local on-link prefix changed to %s from %s", localOnLink.ToString().AsCString(),
        oldLocalOnLink.ToString().AsCString());

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // Validate the received RA message and that it contains the
    // old on-link prefix being deprecated.

    AdvanceTime(30000);

    VerifyOrQuit(sRaValidated);
    VerifyOrQuit(sDeprecatingPrefixes.GetLength() == 1);
    VerifyOrQuit(sDeprecatingPrefixes[0].mPrefix == oldLocalOnLink);
    oldPrefixLifetime = sDeprecatingPrefixes[0].mLifetime;

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // Validate Network Data.

    VerifyOmrPrefixInNetData(localOmr, /* aDefaultRoute */ false);
    VerifyExternalRouteInNetData(kUlaRoute, kWithAdvPioFlagSet);

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // Stop BR and validate that a final RA is emitted deprecating
    // both current local on-link prefix and old prefix.

    sRaValidated = false;
    sExpectedPio = kPioDeprecatingLocalOnLink;

    SuccessOrQuit(sInstance->Get<BorderRouter::RoutingManager>().SetEnabled(false));
    AdvanceTime(100);

    VerifyOrQuit(sRaValidated);
    VerifyOrQuit(sDeprecatingPrefixes.GetLength() == 1);
    VerifyOrQuit(sDeprecatingPrefixes[0].mPrefix == oldLocalOnLink);
    oldPrefixLifetime = sDeprecatingPrefixes[0].mLifetime;

    sRaValidated = false;
    AdvanceTime(350000);
    VerifyOrQuit(!sRaValidated);

    VerifyNoOmrPrefixInNetData();
    VerifyExternalRouteInNetData(kNoRoute);

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // Start BR again and validate old prefix will continue to
    // be deprecated.

    sRaValidated = false;
    sExpectedPio = kPioAdvertisingLocalOnLink;

    SuccessOrQuit(sInstance->Get<BorderRouter::RoutingManager>().SetEnabled(true));

    AdvanceTime(300000);
    VerifyOrQuit(sRaValidated);
    VerifyOrQuit(sDeprecatingPrefixes.GetLength() == 1);
    VerifyOrQuit(sDeprecatingPrefixes[0].mPrefix == oldLocalOnLink);
    VerifyOrQuit(oldPrefixLifetime > sDeprecatingPrefixes[0].mLifetime);
    oldPrefixLifetime = sDeprecatingPrefixes[0].mLifetime;

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // Wait for old local on-link prefix to expire.

    while (oldPrefixLifetime > 2 * kMaxRaTxInterval)
    {
        // Ensure Network Data entries remain as before.

        VerifyExternalRouteInNetData(kUlaRoute, kWithAdvPioFlagSet);

        // Keep checking the emitted RAs and make sure the prefix
        // is included with smaller lifetime every time.

        sRaValidated = false;

        AdvanceTime(kMaxRaTxInterval * 1000);

        VerifyOrQuit(sRaValidated);
        VerifyOrQuit(sDeprecatingPrefixes.GetLength() == 1);
        Log("Old on-link prefix is deprecating, remaining lifetime:%d", sDeprecatingPrefixes[0].mLifetime);
        VerifyOrQuit(sDeprecatingPrefixes[0].mLifetime < oldPrefixLifetime);
        oldPrefixLifetime = sDeprecatingPrefixes[0].mLifetime;
    }

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // The local on-link prefix must be expired now and should no
    // longer be seen in the emitted RA message.

    sRaValidated = false;

    AdvanceTime(3 * kMaxRaTxInterval * 1000);

    VerifyOrQuit(sRaValidated);
    VerifyOrQuit(sDeprecatingPrefixes.IsEmpty());
    Log("Old on-link prefix is now expired");

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // Validate the Network Data.

    VerifyOmrPrefixInNetData(localOmr, /* aDefaultRoute */ false);
    VerifyExternalRouteInNetData(kUlaRoute, kWithAdvPioFlagSet);

    //= = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = =
    // Check behavior when ext PAN ID changes while the local on-link is being
    // deprecated.

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // Send an RA from router A with a new on-link (PIO) which is preferred over
    // the local on-link prefix.

    SendRouterAdvert(routerAddressA, {Pio(onLinkPrefix, kValidLitime, kPreferredLifetime)});

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // Validate that the local on-link prefix is deprecated.

    sRaValidated = false;
    sExpectedPio = kPioDeprecatingLocalOnLink;

    AdvanceTime(30000);

    VerifyOrQuit(sRaValidated);

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // Change the extended PAN ID.

    oldLocalOnLink    = localOnLink;
    oldPrefixLifetime = sOnLinkLifetime;

    dataset.mExtendedPanId = kExtPanId2;
    SuccessOrQuit(otDatasetSetActive(sInstance, &dataset));

    AdvanceTime(500);
    SuccessOrQuit(sInstance->Get<BorderRouter::RoutingManager>().GetOnLinkPrefix(localOnLink));
    Log("Local on-link prefix changed to %s from %s", localOnLink.ToString().AsCString(),
        oldLocalOnLink.ToString().AsCString());

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // Validate that the old local on-link prefix is still being included
    // as PIO in the emitted RA.

    sRaValidated = false;
    sExpectedPio = kNoPio;

    AdvanceTime(30000);

    VerifyOrQuit(sRaValidated);
    VerifyOrQuit(sDeprecatingPrefixes.GetLength() == 1);
    VerifyOrQuit(sDeprecatingPrefixes[0].mPrefix == oldLocalOnLink);

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // Validate that Network Data.

    VerifyOmrPrefixInNetData(localOmr, /* aDefaultRoute */ true);
    VerifyExternalRouteInNetData(kDefaultRoute, kWithAdvPioCleared);

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // Wait for old local on-link prefix to expire.

    while (oldPrefixLifetime > 2 * kMaxRaTxInterval)
    {
        // Send same RA from router A to keep its on-link prefix alive.

        SendRouterAdvert(routerAddressA, {Pio(onLinkPrefix, kValidLitime, kPreferredLifetime)});

        // Ensure Network Data entries remain as before.

        VerifyExternalRouteInNetData(kDefaultRoute, kWithAdvPioCleared);

        // Keep checking the emitted RAs and make sure the prefix
        // is included with smaller lifetime every time.

        sRaValidated = false;

        AdvanceTime(kMaxRaTxInterval * 1000);

        VerifyOrQuit(sRaValidated);
        VerifyOrQuit(sDeprecatingPrefixes.GetLength() == 1);
        Log("Old on-link prefix is deprecating, remaining lifetime:%d", sDeprecatingPrefixes[0].mLifetime);
        VerifyOrQuit(sDeprecatingPrefixes[0].mLifetime < oldPrefixLifetime);
        oldPrefixLifetime = sDeprecatingPrefixes[0].mLifetime;
    }

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // The old on-link prefix must be expired now and should no
    // longer be seen in the emitted RA message.

    SendRouterAdvert(routerAddressA, {Pio(onLinkPrefix, kValidLitime, kPreferredLifetime)});

    sRaValidated = false;

    AdvanceTime(kMaxRaTxInterval * 1000);
    SendRouterAdvert(routerAddressA, {Pio(onLinkPrefix, kValidLitime, kPreferredLifetime)});
    AdvanceTime(kMaxRaTxInterval * 1000);
    SendRouterAdvert(routerAddressA, {Pio(onLinkPrefix, kValidLitime, kPreferredLifetime)});
    AdvanceTime(kMaxRaTxInterval * 1000);

    VerifyOrQuit(sRaValidated);
    VerifyOrQuit(sDeprecatingPrefixes.IsEmpty());
    Log("Old on-link prefix is now expired");

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // Validate the Network Data.

    VerifyOmrPrefixInNetData(localOmr, /* aDefaultRoute */ true);
    VerifyExternalRouteInNetData(kDefaultRoute, kWithAdvPioCleared);

    //= = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = =
    // Check behavior when ext PAN ID changes while the local on-link is not
    // advertised.

    Log("Changing ext PAN ID again");

    oldLocalOnLink = localOnLink;

    sRaValidated = false;
    sExpectedPio = kNoPio;

    dataset.mExtendedPanId = kExtPanId3;
    SuccessOrQuit(otDatasetSetActive(sInstance, &dataset));

    AdvanceTime(500);
    SuccessOrQuit(sInstance->Get<BorderRouter::RoutingManager>().GetOnLinkPrefix(localOnLink));
    Log("Local on-link prefix changed to %s from %s", localOnLink.ToString().AsCString(),
        oldLocalOnLink.ToString().AsCString());

    AdvanceTime(35000);
    VerifyOrQuit(sRaValidated);
    VerifyOrQuit(sDeprecatingPrefixes.IsEmpty());

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // Validate the Network Data.

    VerifyOmrPrefixInNetData(localOmr, /* aDefaultRoute */ true);
    VerifyExternalRouteInNetData(kDefaultRoute, kWithAdvPioCleared);

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // Remove the on-link prefix PIO being advertised by router A
    // and ensure local on-link prefix is advertised again.

    sRaValidated = false;
    sExpectedPio = kPioAdvertisingLocalOnLink;

    SendRouterAdvert(routerAddressA, {Pio(onLinkPrefix, kValidLitime, 0)});

    AdvanceTime(300000);
    VerifyOrQuit(sRaValidated);
    VerifyOrQuit(sDeprecatingPrefixes.IsEmpty());

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // Wait for longer than valid lifetime of PIO entry from router A.
    // Validate that default route is unpublished from network data.

    AdvanceTime(2000 * 1000);
    VerifyOmrPrefixInNetData(localOmr, /* aDefaultRoute */ false);
    VerifyExternalRouteInNetData(kUlaRoute, kWithAdvPioFlagSet);

    //= = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = =
    // Multiple PAN ID changes and multiple deprecating old prefixes.

    oldPrefixes[0] = localOnLink;

    dataset.mExtendedPanId = kExtPanId2;
    SuccessOrQuit(otDatasetSetActive(sInstance, &dataset));

    sRaValidated = false;
    sExpectedPio = kPioAdvertisingLocalOnLink;

    AdvanceTime(30000);
    SuccessOrQuit(sInstance->Get<BorderRouter::RoutingManager>().GetOnLinkPrefix(localOnLink));
    VerifyOrQuit(sRaValidated);
    VerifyOrQuit(sDeprecatingPrefixes.GetLength() == 1);
    VerifyOrQuit(sDeprecatingPrefixes.ContainsMatching(oldPrefixes[0]));

    VerifyExternalRouteInNetData(kUlaRoute, kWithAdvPioFlagSet);

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // Change the prefix again. We should see two deprecating prefixes.

    oldPrefixes[1] = localOnLink;

    dataset.mExtendedPanId = kExtPanId1;
    SuccessOrQuit(otDatasetSetActive(sInstance, &dataset));

    sRaValidated = false;
    sExpectedPio = kPioAdvertisingLocalOnLink;

    AdvanceTime(30000);
    SuccessOrQuit(sInstance->Get<BorderRouter::RoutingManager>().GetOnLinkPrefix(localOnLink));
    VerifyOrQuit(sRaValidated);
    VerifyOrQuit(sDeprecatingPrefixes.GetLength() == 2);
    VerifyOrQuit(sDeprecatingPrefixes.ContainsMatching(oldPrefixes[0]));
    VerifyOrQuit(sDeprecatingPrefixes.ContainsMatching(oldPrefixes[1]));

    VerifyExternalRouteInNetData(kUlaRoute, kWithAdvPioFlagSet);

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // Wait for 15 minutes and then change ext PAN ID again.
    // Now we should see three deprecating prefixes.

    AdvanceTime(15 * 60 * 1000);

    oldPrefixes[2] = localOnLink;

    dataset.mExtendedPanId = kExtPanId4;
    SuccessOrQuit(otDatasetSetActive(sInstance, &dataset));

    sRaValidated = false;
    sExpectedPio = kPioAdvertisingLocalOnLink;

    AdvanceTime(30000);
    SuccessOrQuit(sInstance->Get<BorderRouter::RoutingManager>().GetOnLinkPrefix(localOnLink));
    VerifyOrQuit(sRaValidated);
    VerifyOrQuit(sDeprecatingPrefixes.GetLength() == 3);
    VerifyOrQuit(sDeprecatingPrefixes.ContainsMatching(oldPrefixes[0]));
    VerifyOrQuit(sDeprecatingPrefixes.ContainsMatching(oldPrefixes[1]));
    VerifyOrQuit(sDeprecatingPrefixes.ContainsMatching(oldPrefixes[2]));

    VerifyExternalRouteInNetData(kUlaRoute, kWithAdvPioFlagSet);

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // Change ext PAN ID back to previous value of `kExtPanId1`.
    // We should still see three deprecating prefixes and the last prefix
    // at `oldPrefixes[2]` should again be treated as local on-link prefix.

    oldPrefixes[3] = localOnLink;

    dataset.mExtendedPanId = kExtPanId1;
    SuccessOrQuit(otDatasetSetActive(sInstance, &dataset));

    sRaValidated = false;
    sExpectedPio = kPioAdvertisingLocalOnLink;

    AdvanceTime(30000);
    SuccessOrQuit(sInstance->Get<BorderRouter::RoutingManager>().GetOnLinkPrefix(localOnLink));
    VerifyOrQuit(sRaValidated);
    VerifyOrQuit(sDeprecatingPrefixes.GetLength() == 3);
    VerifyOrQuit(sDeprecatingPrefixes.ContainsMatching(oldPrefixes[0]));
    VerifyOrQuit(sDeprecatingPrefixes.ContainsMatching(oldPrefixes[1]));
    VerifyOrQuit(oldPrefixes[2] == localOnLink);
    VerifyOrQuit(sDeprecatingPrefixes.ContainsMatching(oldPrefixes[3]));

    VerifyExternalRouteInNetData(kUlaRoute, kWithAdvPioFlagSet);

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // Stop BR and validate the final emitted RA to contain
    // all deprecating prefixes.

    sRaValidated = false;
    sExpectedPio = kPioDeprecatingLocalOnLink;

    SuccessOrQuit(sInstance->Get<BorderRouter::RoutingManager>().SetEnabled(false));
    AdvanceTime(100);

    VerifyOrQuit(sRaValidated);
    VerifyOrQuit(sDeprecatingPrefixes.GetLength() == 3);
    VerifyOrQuit(sDeprecatingPrefixes.ContainsMatching(oldPrefixes[0]));
    VerifyOrQuit(sDeprecatingPrefixes.ContainsMatching(oldPrefixes[1]));
    VerifyOrQuit(sDeprecatingPrefixes.ContainsMatching(oldPrefixes[3]));

    VerifyNoOmrPrefixInNetData();
    VerifyExternalRouteInNetData(kNoRoute);

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // Wait for 15 minutes while BR stays disabled and validate
    // there are no emitted RAs. We want to check that deprecating
    // prefixes continue to expire while BR is stopped.

    sRaValidated = false;
    AdvanceTime(15 * 60 * 1000);

    VerifyOrQuit(!sRaValidated);

    VerifyNoOmrPrefixInNetData();
    VerifyExternalRouteInNetData(kNoRoute);

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // Start BR again, and check that we only see the last deprecating prefix
    // at `oldPrefixes[3]` in emitted RA and the other two are expired and
    // no longer included as PIO and/or in network data.

    sRaValidated = false;
    sExpectedPio = kPioAdvertisingLocalOnLink;

    SuccessOrQuit(sInstance->Get<BorderRouter::RoutingManager>().SetEnabled(true));

    AdvanceTime(30000);

    VerifyOrQuit(sRaValidated);
    VerifyOrQuit(sDeprecatingPrefixes.GetLength() == 1);
    VerifyOrQuit(sDeprecatingPrefixes.ContainsMatching(oldPrefixes[3]));

    VerifyExternalRouteInNetData(kUlaRoute, kWithAdvPioFlagSet);

    //= = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = =
    // Validate the oldest prefix is removed when we have too many
    // back-to-back PAN ID changes.

    // Remember the oldest deprecating prefix (associated with `kExtPanId4`).
    oldLocalOnLink = oldPrefixes[3];

    SuccessOrQuit(sInstance->Get<BorderRouter::RoutingManager>().GetOnLinkPrefix(oldPrefixes[0]));
    dataset.mExtendedPanId = kExtPanId2;
    SuccessOrQuit(otDatasetSetActive(sInstance, &dataset));
    AdvanceTime(30000);

    SuccessOrQuit(sInstance->Get<BorderRouter::RoutingManager>().GetOnLinkPrefix(oldPrefixes[1]));
    dataset.mExtendedPanId = kExtPanId3;
    SuccessOrQuit(otDatasetSetActive(sInstance, &dataset));
    AdvanceTime(30000);

    SuccessOrQuit(sInstance->Get<BorderRouter::RoutingManager>().GetOnLinkPrefix(oldPrefixes[2]));
    dataset.mExtendedPanId = kExtPanId5;
    SuccessOrQuit(otDatasetSetActive(sInstance, &dataset));

    sRaValidated = false;

    AdvanceTime(30000);

    VerifyOrQuit(sRaValidated);
    SuccessOrQuit(sInstance->Get<BorderRouter::RoutingManager>().GetOnLinkPrefix(localOnLink));
    VerifyOrQuit(sDeprecatingPrefixes.GetLength() == 3);
    VerifyOrQuit(sDeprecatingPrefixes.ContainsMatching(oldPrefixes[0]));
    VerifyOrQuit(sDeprecatingPrefixes.ContainsMatching(oldPrefixes[1]));
    VerifyOrQuit(sDeprecatingPrefixes.ContainsMatching(oldPrefixes[2]));
    VerifyOrQuit(!sDeprecatingPrefixes.ContainsMatching(oldLocalOnLink));

    VerifyExternalRouteInNetData(kUlaRoute, kWithAdvPioFlagSet);

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

    SuccessOrQuit(sInstance->Get<BorderRouter::RoutingManager>().SetEnabled(false));
    VerifyOrQuit(heapAllocations == sHeapAllocatedPtrs.GetLength());

    Log("End of TestExtPanIdChange");
    FinalizeTest();
}

void TestRouterNsProbe(void)
{
    Ip6::Prefix  localOnLink;
    Ip6::Prefix  localOmr;
    Ip6::Prefix  onLinkPrefix   = PrefixFromString("2000:abba:baba::", 64);
    Ip6::Prefix  routePrefix    = PrefixFromString("2000:1234:5678::", 64);
    Ip6::Address routerAddressA = AddressFromString("fd00::aaaa");
    Ip6::Address routerAddressB = AddressFromString("fd00::bbbb");
    uint16_t     heapAllocations;

    Log("--------------------------------------------------------------------------------------------");
    Log("TestRouterNsProbe");

    InitTest();

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // Start Routing Manager. Check emitted RS and RA messages.

    sRsEmitted   = false;
    sRaValidated = false;
    sExpectedPio = kPioAdvertisingLocalOnLink;
    sExpectedRios.Clear();

    heapAllocations = sHeapAllocatedPtrs.GetLength();
    SuccessOrQuit(sInstance->Get<BorderRouter::RoutingManager>().SetEnabled(true));

    SuccessOrQuit(sInstance->Get<BorderRouter::RoutingManager>().GetOnLinkPrefix(localOnLink));
    SuccessOrQuit(sInstance->Get<BorderRouter::RoutingManager>().GetOmrPrefix(localOmr));

    Log("Local on-link prefix is %s", localOnLink.ToString().AsCString());
    Log("Local OMR prefix is %s", localOmr.ToString().AsCString());

    sExpectedRios.Add(localOmr);

    AdvanceTime(30000);

    VerifyOrQuit(sRsEmitted);
    VerifyOrQuit(sRaValidated);
    VerifyOrQuit(sExpectedRios.SawAll());
    Log("Received RA was validated");

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // Send an RA from router A with a new on-link (PIO) and route prefix (RIO).

    SendRouterAdvert(routerAddressA, {Pio(onLinkPrefix, kValidLitime, kPreferredLifetime)},
                     {Rio(routePrefix, kValidLitime, NetworkData::kRoutePreferenceMedium)});

    sExpectedPio = kPioDeprecatingLocalOnLink;

    AdvanceTime(10);

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // Check the discovered prefix table and ensure info from router A
    // is present in the table.

    VerifyPrefixTable({OnLinkPrefix(onLinkPrefix, kValidLitime, kPreferredLifetime, routerAddressA)},
                      {RoutePrefix(routePrefix, kValidLitime, NetworkData::kRoutePreferenceMedium, routerAddressA)});

    AdvanceTime(30000);

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // Send an RA from router B with same route prefix (RIO) but with
    // high route preference.

    SendRouterAdvert(routerAddressB, {Rio(routePrefix, kValidLitime, NetworkData::kRoutePreferenceHigh)});

    AdvanceTime(200);

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // Check the discovered prefix table and ensure entries from
    // both router A and B are seen.

    VerifyPrefixTable({OnLinkPrefix(onLinkPrefix, kValidLitime, kPreferredLifetime, routerAddressA)},
                      {RoutePrefix(routePrefix, kValidLitime, NetworkData::kRoutePreferenceMedium, routerAddressA),
                       RoutePrefix(routePrefix, kValidLitime, NetworkData::kRoutePreferenceHigh, routerAddressB)});

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // Check that BR emitted an NS to ensure routers are active.

    sNsEmitted = false;
    sRsEmitted = false;

    AdvanceTime(160 * 1000);

    VerifyOrQuit(sNsEmitted);
    VerifyOrQuit(!sRsEmitted);

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // Disallow responding to NS message.
    //
    // This should trigger `RoutingManager` to send RS (which will get
    // no response as well) and then remove all router entries.

    sRespondToNs = false;

    sExpectedPio = kPioAdvertisingLocalOnLink;
    sRaValidated = false;
    sNsEmitted   = false;

    AdvanceTime(240 * 1000);

    VerifyOrQuit(sNsEmitted);
    VerifyOrQuit(sRaValidated);

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // Check the discovered prefix table. We should see the on-link entry from
    // router A as deprecated and no route prefix.

    VerifyPrefixTable({OnLinkPrefix(onLinkPrefix, kValidLitime, 0, routerAddressA)});

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // Verify that no more NS is being sent (since there is no more valid
    // router entry in the table).

    sExpectedPio = kPioAdvertisingLocalOnLink;
    sRaValidated = false;
    sNsEmitted   = false;

    AdvanceTime(6 * 60 * 1000);

    VerifyOrQuit(!sNsEmitted);

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // Send an RA from router B and verify that we see router B
    // entry in prefix table.

    SendRouterAdvert(routerAddressB, {Rio(routePrefix, kValidLitime, NetworkData::kRoutePreferenceHigh)});

    VerifyPrefixTable({OnLinkPrefix(onLinkPrefix, kValidLitime, 0, routerAddressA)},
                      {RoutePrefix(routePrefix, kValidLitime, NetworkData::kRoutePreferenceHigh, routerAddressB)});

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // Wait for longer than router active time before NS probe.
    // Check again that NS are sent again.

    sRespondToNs = true;
    sNsEmitted   = false;
    sRsEmitted   = false;

    AdvanceTime(3 * 60 * 1000);

    VerifyOrQuit(sNsEmitted);
    VerifyOrQuit(!sRsEmitted);

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

    SuccessOrQuit(sInstance->Get<BorderRouter::RoutingManager>().SetEnabled(false));
    VerifyOrQuit(heapAllocations == sHeapAllocatedPtrs.GetLength());

    Log("End of TestRouterNsProbe");
    FinalizeTest();
}

void TestLearningAndCopyingOfFlags(void)
{
    Ip6::Prefix  localOnLink;
    Ip6::Prefix  localOmr;
    Ip6::Prefix  onLinkPrefix   = PrefixFromString("2000:abba:baba::", 64);
    Ip6::Address routerAddressA = AddressFromString("fd00::aaaa");
    Ip6::Address routerAddressB = AddressFromString("fd00::bbbb");
    Ip6::Address routerAddressC = AddressFromString("fd00::cccc");
    uint16_t     heapAllocations;
    RaFlags      raFlags;

    Log("--------------------------------------------------------------------------------------------");
    Log("TestLearningAndCopyingOfFlags");

    InitTest();

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // Start Routing Manager. Check emitted RS and RA messages.

    sRsEmitted   = false;
    sRaValidated = false;
    sExpectedPio = kPioAdvertisingLocalOnLink;
    sExpectedRios.Clear();

    heapAllocations = sHeapAllocatedPtrs.GetLength();
    SuccessOrQuit(sInstance->Get<BorderRouter::RoutingManager>().SetEnabled(true));

    SuccessOrQuit(sInstance->Get<BorderRouter::RoutingManager>().GetOnLinkPrefix(localOnLink));
    SuccessOrQuit(sInstance->Get<BorderRouter::RoutingManager>().GetOmrPrefix(localOmr));

    Log("Local on-link prefix is %s", localOnLink.ToString().AsCString());
    Log("Local OMR prefix is %s", localOmr.ToString().AsCString());

    sExpectedRios.Add(localOmr);

    AdvanceTime(30000);

    VerifyOrQuit(sRsEmitted);
    VerifyOrQuit(sRaValidated);
    VerifyOrQuit(sExpectedRios.SawAll());
    Log("Received RA was validated");

    VerifyDiscoveredRoutersIsEmpty();

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // Send an RA from router A with M flag set, and make sure the
    // emitted RA from BR also includes M flag.

    raFlags.Clear();
    raFlags.mManagedAddressConfigFlag = true;

    SendRouterAdvert(routerAddressA, raFlags);

    AdvanceTime(1);
    VerifyDiscoveredRouters({InfraRouter(routerAddressA, /* M */ true, /* O */ false, /* StubRouter */ false)});

    sRaValidated           = false;
    sExpectedRaHeaderFlags = kRaHeaderFlagsOnlyM;

    AdvanceTime(610 * 1000);
    VerifyOrQuit(sRaValidated);

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // Send an RA from router A without the M flag. Now the emitted
    // RA should no longer contain the M flag.

    raFlags.Clear();

    SendRouterAdvert(routerAddressA, raFlags);

    sRaValidated           = false;
    sExpectedRaHeaderFlags = kRaHeaderFlagsNone;

    AdvanceTime(1);
    VerifyDiscoveredRoutersIsEmpty();

    AdvanceTime(610 * 1000);
    VerifyOrQuit(sRaValidated);

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // Send an RA from router A with both M and StubRouter flags.
    // Since it is from a stub router, the M flag should be ignored.
    // Ensure emitted RA does not set the M flag.

    raFlags.Clear();
    raFlags.mManagedAddressConfigFlag = true;
    raFlags.mStubRouterFlag           = true;

    SendRouterAdvert(routerAddressA, raFlags);

    AdvanceTime(1);
    VerifyDiscoveredRouters({InfraRouter(routerAddressA, /* M */ true, /* O */ false, /* StubRouter */ true)});

    sRaValidated           = false;
    sExpectedRaHeaderFlags = kRaHeaderFlagsNone;

    AdvanceTime(610 * 1000);
    VerifyOrQuit(sRaValidated);

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // Send an RA from router B with O flag and check that emitted
    // RA include the same flag.

    raFlags.Clear();
    raFlags.mOtherConfigFlag = true;

    SendRouterAdvert(routerAddressB, raFlags);

    AdvanceTime(1);
    VerifyDiscoveredRouters({InfraRouter(routerAddressA, /* M */ true, /* O */ false, /* StubRouter */ true),
                             InfraRouter(routerAddressB, /* M */ false, /* O */ true, /* StubRouter */ false)});

    sRaValidated           = false;
    sExpectedRaHeaderFlags = kRaHeaderFlagsOnlyO;

    AdvanceTime(610 * 1000);
    VerifyOrQuit(sRaValidated);

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // Send an RA from router C with M flag and check that emitted
    // RA now includes both M and O flags.

    raFlags.Clear();
    raFlags.mManagedAddressConfigFlag = true;

    SendRouterAdvert(routerAddressC, {Pio(onLinkPrefix, kValidLitime, kPreferredLifetime)},
                     DefaultRoute(0, NetworkData::kRoutePreferenceMedium), raFlags);

    AdvanceTime(1);
    VerifyDiscoveredRouters({InfraRouter(routerAddressA, /* M */ true, /* O */ false, /* StubRouter */ true),
                             InfraRouter(routerAddressB, /* M */ false, /* O */ true, /* StubRouter */ false),
                             InfraRouter(routerAddressC, /* M */ true, /* O */ false, /* StubRouter */ false)});

    sRaValidated           = false;
    sExpectedPio           = kPioDeprecatingLocalOnLink;
    sExpectedRaHeaderFlags = kRaHeaderFlagsBothMAndO;

    AdvanceTime(610 * 1000);
    VerifyOrQuit(sRaValidated);

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // Stop responding to NS, this should cause all routers
    // to age and considered offline

    sRespondToNs = false;

    sExpectedRaHeaderFlags = kRaHeaderFlagsSkipChecking;

    AdvanceTime(300 * 1000);

    // Router C should be in the table since it will have a deprecating
    // on-link prefix.
    VerifyDiscoveredRouters({InfraRouter(routerAddressC, /* M */ true, /* O */ false, /* StubRouter */ false)});

    sRaValidated           = false;
    sExpectedPio           = kPioAdvertisingLocalOnLink;
    sExpectedRaHeaderFlags = kRaHeaderFlagsNone;

    AdvanceTime(610 * 1000);
    VerifyOrQuit(sRaValidated);

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

    SuccessOrQuit(sInstance->Get<BorderRouter::RoutingManager>().SetEnabled(false));
    VerifyDiscoveredRoutersIsEmpty();

    VerifyOrQuit(heapAllocations == sHeapAllocatedPtrs.GetLength());

    Log("End of TestLearningAndCopyingOfFlags");
    FinalizeTest();
}

void TestConflictingPrefix(void)
{
    static const otExtendedPanId kExtPanId1 = {{0x01, 0x02, 0x03, 0x04, 0x05, 0x6, 0x7, 0x08}};

    Ip6::Prefix          localOnLink;
    Ip6::Prefix          oldLocalOnLink;
    Ip6::Prefix          localOmr;
    Ip6::Prefix          onLinkPrefix   = PrefixFromString("2000:abba:baba::", 64);
    Ip6::Address         routerAddressA = AddressFromString("fd00::aaaa");
    Ip6::Address         routerAddressB = AddressFromString("fd00::bbbb");
    otOperationalDataset dataset;
    uint16_t             heapAllocations;

    Log("--------------------------------------------------------------------------------------------");
    Log("TestConflictingPrefix");

    InitTest();

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // Start Routing Manager. Check emitted RS and RA messages.

    sRsEmitted   = false;
    sRaValidated = false;
    sExpectedPio = kPioAdvertisingLocalOnLink;
    sExpectedRios.Clear();

    heapAllocations = sHeapAllocatedPtrs.GetLength();
    SuccessOrQuit(sInstance->Get<BorderRouter::RoutingManager>().SetEnabled(true));

    SuccessOrQuit(sInstance->Get<BorderRouter::RoutingManager>().GetOnLinkPrefix(localOnLink));
    SuccessOrQuit(sInstance->Get<BorderRouter::RoutingManager>().GetOmrPrefix(localOmr));

    Log("Local on-link prefix is %s", localOnLink.ToString().AsCString());
    Log("Local OMR prefix is %s", localOmr.ToString().AsCString());

    sExpectedRios.Add(localOmr);

    AdvanceTime(30000);

    VerifyOrQuit(sRsEmitted);
    VerifyOrQuit(sRaValidated);
    VerifyOrQuit(sExpectedRios.SawAll());
    Log("Received RA was validated");

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // Check Network Data to include the local OMR and on-link prefix.

    VerifyOmrPrefixInNetData(localOmr, /* aDefaultRoute */ false);
    VerifyExternalRouteInNetData(kUlaRoute, kWithAdvPioFlagSet);

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // Send an RA from router A with our local on-link prefix as RIO.

    Log("Send RA from router A with local on-link as RIO");
    SendRouterAdvert(routerAddressA, {Rio(localOnLink, kValidLitime, NetworkData::kRoutePreferenceMedium)});

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // Check that the local on-link prefix is still being advertised.

    sRaValidated = false;
    AdvanceTime(610000);
    VerifyOrQuit(sRaValidated);

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // Check Network Data to still include the local OMR and ULA prefix.

    VerifyOmrPrefixInNetData(localOmr, /* aDefaultRoute */ false);
    VerifyExternalRouteInNetData(kUlaRoute, kWithAdvPioFlagSet);

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // Send an RA from router A removing local on-link prefix as RIO.

    SendRouterAdvert(routerAddressA, {Rio(localOnLink, 0, NetworkData::kRoutePreferenceMedium)});

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // Verify that ULA prefix is still included in Network Data and
    // the change by router A did not cause it to be unpublished.

    AdvanceTime(10000);
    VerifyExternalRouteInNetData(kUlaRoute, kWithAdvPioFlagSet);

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // Check that the local on-link prefix is still being advertised.

    sRaValidated = false;
    AdvanceTime(610000);
    VerifyOrQuit(sRaValidated);

    VerifyExternalRouteInNetData(kUlaRoute, kWithAdvPioFlagSet);

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // Send RA from router B advertising an on-link prefix. This
    // should cause local on-link prefix to be deprecated.

    SendRouterAdvert(routerAddressB, {Pio(onLinkPrefix, kValidLitime, kPreferredLifetime)});

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // Check that the local on-link prefix is now deprecating.

    sRaValidated = false;
    sExpectedPio = kPioDeprecatingLocalOnLink;

    AdvanceTime(10000);
    VerifyOrQuit(sRaValidated);
    VerifyOrQuit(sExpectedRios.SawAll());
    Log("On-link prefix is deprecating, remaining lifetime:%d", sOnLinkLifetime);

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // Check Network Data to include the default route now due
    // the new on-link prefix from router B.

    VerifyOmrPrefixInNetData(localOmr, /* aDefaultRoute */ true);
    VerifyExternalRouteInNetData(kDefaultRoute, kWithAdvPioFlagSet);

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // Send an RA from router A again adding local on-link prefix as RIO.

    Log("Send RA from router A with local on-link as RIO");
    SendRouterAdvert(routerAddressA, {Rio(localOnLink, kValidLitime, NetworkData::kRoutePreferenceMedium)});

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // Check that the local on-link prefix is still being deprecated.

    sRaValidated = false;
    AdvanceTime(610000);
    VerifyOrQuit(sRaValidated);

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // Check Network Data remains unchanged.

    VerifyExternalRouteInNetData(kDefaultRoute, kWithAdvPioFlagSet);

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // Send an RA from router A removing the previous RIO.

    SendRouterAdvert(routerAddressA, {Rio(localOnLink, 0, NetworkData::kRoutePreferenceMedium)});

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // Check Network Data remains unchanged.

    AdvanceTime(60000);
    VerifyExternalRouteInNetData(kDefaultRoute, kWithAdvPioFlagSet);

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // Send RA from router B removing its on-link prefix.

    SendRouterAdvert(routerAddressB, {Pio(onLinkPrefix, kValidLitime, 0)});

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // Check that the local on-link prefix is once again being advertised.

    sRaValidated = false;
    sExpectedPio = kPioAdvertisingLocalOnLink;

    AdvanceTime(10000);
    VerifyOrQuit(sRaValidated);

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // Check Network Data to remain unchanged.

    VerifyExternalRouteInNetData(kDefaultRoute, kWithAdvPioFlagSet);

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // Change the extended PAN ID.

    Log("Changing ext PAN ID");

    SuccessOrQuit(otDatasetGetActive(sInstance, &dataset));

    VerifyOrQuit(dataset.mComponents.mIsExtendedPanIdPresent);

    dataset.mExtendedPanId = kExtPanId1;
    SuccessOrQuit(otDatasetSetActive(sInstance, &dataset));
    AdvanceTime(10000);

    oldLocalOnLink = localOnLink;
    SuccessOrQuit(sInstance->Get<BorderRouter::RoutingManager>().GetOnLinkPrefix(localOnLink));

    Log("Local on-link prefix is changed to %s from %s", localOnLink.ToString().AsCString(),
        oldLocalOnLink.ToString().AsCString());

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // Check Network Data contains default route due to the
    // deprecating on-link prefix from router B.

    VerifyExternalRouteInNetData(kDefaultRoute, kWithAdvPioFlagSet);

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // Send an RA from router A again adding the old local on-link prefix
    // as RIO.

    SendRouterAdvert(routerAddressA, {Rio(oldLocalOnLink, kValidLitime, NetworkData::kRoutePreferenceMedium)});

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // Check Network Data remains unchanged.

    AdvanceTime(10000);
    VerifyExternalRouteInNetData(kDefaultRoute, kWithAdvPioFlagSet);

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // Send an RA from router A removing the previous RIO.

    SendRouterAdvert(routerAddressA, {Rio(localOnLink, 0, NetworkData::kRoutePreferenceMedium)});

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // Check Network Data remains unchanged.

    AdvanceTime(10000);
    VerifyExternalRouteInNetData(kDefaultRoute, kWithAdvPioFlagSet);

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

    SuccessOrQuit(sInstance->Get<BorderRouter::RoutingManager>().SetEnabled(false));
    VerifyOrQuit(heapAllocations == sHeapAllocatedPtrs.GetLength());

    Log("End of TestConflictingPrefix");

    FinalizeTest();
}

#if OPENTHREAD_CONFIG_PLATFORM_FLASH_API_ENABLE
void TestSavedOnLinkPrefixes(void)
{
    static const otExtendedPanId kExtPanId1 = {{0x01, 0x02, 0x03, 0x04, 0x05, 0x6, 0x7, 0x08}};

    Ip6::Prefix          localOnLink;
    Ip6::Prefix          oldLocalOnLink;
    Ip6::Prefix          localOmr;
    Ip6::Prefix          onLinkPrefix   = PrefixFromString("2000:abba:baba::", 64);
    Ip6::Address         routerAddressA = AddressFromString("fd00::aaaa");
    otOperationalDataset dataset;

    Log("--------------------------------------------------------------------------------------------");
    Log("TestSavedOnLinkPrefixes");

    InitTest(/* aEnablBorderRouting */ true);

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // Check emitted RS and RA messages.

    sRsEmitted   = false;
    sRaValidated = false;
    sExpectedPio = kPioAdvertisingLocalOnLink;
    sExpectedRios.Clear();

    SuccessOrQuit(sInstance->Get<BorderRouter::RoutingManager>().GetOnLinkPrefix(localOnLink));
    SuccessOrQuit(sInstance->Get<BorderRouter::RoutingManager>().GetOmrPrefix(localOmr));

    Log("Local on-link prefix is %s", localOnLink.ToString().AsCString());
    Log("Local OMR prefix is %s", localOmr.ToString().AsCString());

    sExpectedRios.Add(localOmr);

    AdvanceTime(30000);

    VerifyOrQuit(sRsEmitted);
    VerifyOrQuit(sRaValidated);
    VerifyOrQuit(sExpectedRios.SawAll());

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // Check Network Data to include the local OMR and ULA prefix.

    VerifyOmrPrefixInNetData(localOmr, /* aDefaultRoute */ false);
    VerifyExternalRouteInNetData(kUlaRoute, kWithAdvPioFlagSet);

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // Disable the instance and re-enable it.

    Log("Disabling and re-enabling OT Instance");

    testFreeInstance(sInstance);

    InitTest(/* aEnablBorderRouting */ true, /* aAfterReset */ true);

    SuccessOrQuit(sInstance->Get<BorderRouter::RoutingManager>().SetEnabled(true));

    sExpectedPio = kPioAdvertisingLocalOnLink;

    AdvanceTime(30000);

    VerifyOrQuit(sRsEmitted);
    VerifyOrQuit(sRaValidated);
    VerifyOrQuit(sExpectedRios.SawAll());

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // Check Network Data to include the local OMR and ULA prefix.

    VerifyOmrPrefixInNetData(localOmr, /* aDefaultRoute */ false);
    VerifyExternalRouteInNetData(kUlaRoute, kWithAdvPioFlagSet);

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // Send RA from router A advertising an on-link prefix.

    SendRouterAdvert(routerAddressA, {Pio(onLinkPrefix, kValidLitime, kPreferredLifetime)});

    sRaValidated = false;
    sExpectedPio = kPioDeprecatingLocalOnLink;

    AdvanceTime(30000);

    VerifyOrQuit(sRaValidated);
    VerifyOrQuit(sDeprecatingPrefixes.GetLength() == 0);

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // Disable the instance and re-enable it.

    Log("Disabling and re-enabling OT Instance");

    testFreeInstance(sInstance);

    InitTest(/* aEnablBorderRouting */ true, /* aAfterReset */ true);

    sExpectedPio = kPioAdvertisingLocalOnLink;

    AdvanceTime(30000);

    VerifyOrQuit(sRsEmitted);
    VerifyOrQuit(sRaValidated);
    VerifyOrQuit(sExpectedRios.SawAll());

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // Check Network Data to include the local OMR and ULA prefix.

    VerifyOmrPrefixInNetData(localOmr, /* aDefaultRoute */ false);
    VerifyExternalRouteInNetData(kUlaRoute, kWithAdvPioFlagSet);

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

    Log("Changing ext PAN ID");

    oldLocalOnLink = localOnLink;

    sRaValidated = false;
    sExpectedPio = kPioAdvertisingLocalOnLink;

    SuccessOrQuit(otDatasetGetActive(sInstance, &dataset));

    VerifyOrQuit(dataset.mComponents.mIsExtendedPanIdPresent);

    dataset.mExtendedPanId = kExtPanId1;
    dataset.mActiveTimestamp.mSeconds++;
    SuccessOrQuit(otDatasetSetActive(sInstance, &dataset));

    AdvanceTime(30000);

    SuccessOrQuit(sInstance->Get<BorderRouter::RoutingManager>().GetOnLinkPrefix(localOnLink));
    Log("Local on-link prefix changed to %s from %s", localOnLink.ToString().AsCString(),
        oldLocalOnLink.ToString().AsCString());

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // Disable the instance and re-enable it.

    Log("Disabling and re-enabling OT Instance");

    testFreeInstance(sInstance);

    InitTest(/* aEnablBorderRouting */ false, /* aAfterReset */ true);

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // Start Routing Manager.

    SuccessOrQuit(sInstance->Get<BorderRouter::RoutingManager>().SetEnabled(true));

    AdvanceTime(100);

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // Send RA from router A advertising an on-link prefix.
    // This ensures the local on-link prefix is not advertised, but
    // it must be deprecated since it was advertised last time and
    // saved in `Settings`.

    SendRouterAdvert(routerAddressA, {Pio(onLinkPrefix, kValidLitime, kPreferredLifetime)});

    sRaValidated = false;
    sExpectedPio = kPioDeprecatingLocalOnLink;

    AdvanceTime(30000);

    VerifyOrQuit(sRaValidated);
    VerifyOrQuit(sDeprecatingPrefixes.GetLength() == 1);

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // Check Network Data to now use default route due to the
    // on-link prefix from router A.

    VerifyExternalRouteInNetData(kDefaultRoute, kWithAdvPioFlagSet);

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // Wait for more than 1800 seconds to let the deprecating
    // prefixes expire (keep sending RA from router A).

    for (uint16_t index = 0; index < 185; index++)
    {
        SendRouterAdvert(routerAddressA, {Pio(onLinkPrefix, kValidLitime, kPreferredLifetime)});
        AdvanceTime(10 * 1000);
    }

    VerifyExternalRouteInNetData(kDefaultRoute, kWithAdvPioCleared);

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // Disable the instance and re-enable it and restart Routing Manager.

    Log("Disabling and re-enabling OT Instance again");

    testFreeInstance(sInstance);
    InitTest(/* aEnablBorderRouting */ false, /* aAfterReset */ true);

    SuccessOrQuit(sInstance->Get<BorderRouter::RoutingManager>().SetEnabled(true));
    AdvanceTime(100);

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // Send RA from router A advertising an on-link prefix.

    SendRouterAdvert(routerAddressA, {Pio(onLinkPrefix, kValidLitime, kPreferredLifetime)});

    sRaValidated = false;
    sExpectedPio = kNoPio;

    AdvanceTime(30000);

    VerifyOrQuit(sRaValidated);
    VerifyOrQuit(sDeprecatingPrefixes.GetLength() == 0);

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // Check Network Data still contains the default route.

    VerifyExternalRouteInNetData(kDefaultRoute, kWithAdvPioCleared);

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

    Log("End of TestSavedOnLinkPrefixes");
    FinalizeTest();
}
#endif // OPENTHREAD_CONFIG_PLATFORM_FLASH_API_ENABLE

#if OPENTHREAD_CONFIG_SRP_SERVER_ENABLE
void TestAutoEnableOfSrpServer(void)
{
    Ip6::Prefix  localOnLink;
    Ip6::Prefix  localOmr;
    Ip6::Address routerAddressA = AddressFromString("fd00::aaaa");
    Ip6::Prefix  onLinkPrefix   = PrefixFromString("2000:abba:baba::", 64);
    uint16_t     heapAllocations;

    Log("--------------------------------------------------------------------------------------------");
    Log("TestAutoEnableOfSrpServer");

    InitTest();

    heapAllocations = sHeapAllocatedPtrs.GetLength();

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // Check SRP Server state and enable auto-enable mode

    otSrpServerSetAutoEnableMode(sInstance, true);
    VerifyOrQuit(otSrpServerIsAutoEnableMode(sInstance));
    VerifyOrQuit(otSrpServerGetState(sInstance) == OT_SRP_SERVER_STATE_DISABLED);

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // Start Routing Manager. Check emitted RS and RA messages.

    sRsEmitted   = false;
    sRaValidated = false;
    sExpectedPio = kPioAdvertisingLocalOnLink;
    sExpectedRios.Clear();

    SuccessOrQuit(sInstance->Get<BorderRouter::RoutingManager>().SetEnabled(true));

    SuccessOrQuit(sInstance->Get<BorderRouter::RoutingManager>().GetOnLinkPrefix(localOnLink));
    SuccessOrQuit(sInstance->Get<BorderRouter::RoutingManager>().GetOmrPrefix(localOmr));

    Log("Local on-link prefix is %s", localOnLink.ToString().AsCString());
    Log("Local OMR prefix is %s", localOmr.ToString().AsCString());

    sExpectedRios.Add(localOmr);

    AdvanceTime(30000);

    VerifyOrQuit(sRsEmitted);
    VerifyOrQuit(sRaValidated);
    VerifyOrQuit(sExpectedRios.SawAll());
    Log("Received RA was validated");

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // Validate that SRP server was auto-enabled

    VerifyOrQuit(otSrpServerGetState(sInstance) != OT_SRP_SERVER_STATE_DISABLED);
    Log("Srp::Server is enabled");

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // Signal that infra if state changed and is no longer running.
    // This should stop Routing Manager and in turn the SRP server.

    sRaValidated = false;
    sExpectedPio = kPioDeprecatingLocalOnLink;

    Log("Signal infra if is not running");
    SuccessOrQuit(otPlatInfraIfStateChanged(sInstance, kInfraIfIndex, false));
    AdvanceTime(1);

    VerifyOrQuit(sRaValidated);

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // Check that SRP server is disabled.

    VerifyOrQuit(otSrpServerGetState(sInstance) == OT_SRP_SERVER_STATE_DISABLED);
    Log("Srp::Server is disabled");

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // Signal that infra if state changed and is running again.

    sRsEmitted   = false;
    sRaValidated = false;
    sExpectedPio = kPioAdvertisingLocalOnLink;
    sExpectedRios.Add(localOmr);

    Log("Signal infra if is running");
    SuccessOrQuit(otPlatInfraIfStateChanged(sInstance, kInfraIfIndex, true));

    AdvanceTime(30000);

    VerifyOrQuit(sRsEmitted);
    VerifyOrQuit(sRaValidated);
    VerifyOrQuit(sExpectedRios.SawAll());
    Log("Received RA was validated");

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // Check that SRP server is enabled again.

    VerifyOrQuit(otSrpServerGetState(sInstance) != OT_SRP_SERVER_STATE_DISABLED);
    Log("Srp::Server is enabled");

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // Disable `RoutingManager` explicitly.

    sRaValidated = false;
    sExpectedPio = kPioDeprecatingLocalOnLink;

    Log("Disabling RoutingManager");
    SuccessOrQuit(sInstance->Get<BorderRouter::RoutingManager>().SetEnabled(false));
    AdvanceTime(1);

    VerifyOrQuit(sRaValidated);

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // Check that SRP server is also disabled.

    VerifyOrQuit(otSrpServerGetState(sInstance) == OT_SRP_SERVER_STATE_DISABLED);
    Log("Srp::Server is disabled");

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // Disable auto-enable mode on SRP server.

    otSrpServerSetAutoEnableMode(sInstance, false);
    VerifyOrQuit(!otSrpServerIsAutoEnableMode(sInstance));
    VerifyOrQuit(otSrpServerGetState(sInstance) == OT_SRP_SERVER_STATE_DISABLED);

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // Re-start Routing Manager. Check emitted RS and RA messages.
    // This cycle, router A will send a RA including a PIO.

    sRsEmitted   = false;
    sRaValidated = false;
    sExpectedPio = kNoPio;
    sExpectedRios.Clear();

    SuccessOrQuit(sInstance->Get<BorderRouter::RoutingManager>().SetEnabled(true));

    SuccessOrQuit(sInstance->Get<BorderRouter::RoutingManager>().GetOnLinkPrefix(localOnLink));
    SuccessOrQuit(sInstance->Get<BorderRouter::RoutingManager>().GetOmrPrefix(localOmr));

    Log("Local on-link prefix is %s", localOnLink.ToString().AsCString());
    Log("Local OMR prefix is %s", localOmr.ToString().AsCString());

    sExpectedRios.Add(localOmr);

    AdvanceTime(2000);

    SendRouterAdvert(routerAddressA, {Pio(onLinkPrefix, kValidLitime, kPreferredLifetime)});

    AdvanceTime(30000);

    VerifyOrQuit(sRsEmitted);
    VerifyOrQuit(sRaValidated);
    VerifyOrQuit(sExpectedRios.SawAll());
    Log("Received RA was validated");

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // Check that SRP server is still disabled.

    VerifyOrQuit(otSrpServerGetState(sInstance) == OT_SRP_SERVER_STATE_DISABLED);
    Log("Srp::Server is disabled");

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // Enable auto-enable mode on SRP server. Since `RoutingManager`
    // is already done with initial policy evaluation, the SRP server
    // must be started immediately.

    otSrpServerSetAutoEnableMode(sInstance, true);
    VerifyOrQuit(otSrpServerIsAutoEnableMode(sInstance));

    VerifyOrQuit(otSrpServerGetState(sInstance) != OT_SRP_SERVER_STATE_DISABLED);
    Log("Srp::Server is enabled");

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // Disable auto-enable mode on SRP server. It must not impact
    // its current state and it should remain enabled.

    otSrpServerSetAutoEnableMode(sInstance, false);
    VerifyOrQuit(!otSrpServerIsAutoEnableMode(sInstance));

    AdvanceTime(2000);
    VerifyOrQuit(otSrpServerGetState(sInstance) != OT_SRP_SERVER_STATE_DISABLED);
    Log("Srp::Server is enabled");

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // Signal that infra if state changed and is no longer running.
    // This should stop Routing Manager.

    sRaValidated = false;

    Log("Signal infra if is not running");
    SuccessOrQuit(otPlatInfraIfStateChanged(sInstance, kInfraIfIndex, false));
    AdvanceTime(1);

    VerifyOrQuit(sRaValidated);

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // Re-enable auto-enable mode on SRP server. Since `RoutingManager`
    // is stopped (infra if is down), the SRP serer must be stopped
    // immediately.

    otSrpServerSetAutoEnableMode(sInstance, true);
    VerifyOrQuit(otSrpServerIsAutoEnableMode(sInstance));

    VerifyOrQuit(otSrpServerGetState(sInstance) == OT_SRP_SERVER_STATE_DISABLED);
    Log("Srp::Server is disabled");

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

    VerifyOrQuit(sHeapAllocatedPtrs.GetLength() == heapAllocations);

    Log("End of TestAutoEnableOfSrpServer");
    FinalizeTest();
}
#endif // OPENTHREAD_CONFIG_SRP_SERVER_ENABLE

#if OPENTHREAD_CONFIG_NAT64_BORDER_ROUTING_ENABLE
void TestNat64PrefixSelection(void)
{
    Ip6::Prefix                     localNat64;
    Ip6::Prefix                     ailNat64 = PrefixFromString("2000:0:0:1:0:0::", 96);
    Ip6::Prefix                     localOmr;
    Ip6::Prefix                     omrPrefix = PrefixFromString("2000:0000:1111:4444::", 64);
    NetworkData::OnMeshPrefixConfig prefixConfig;
    uint16_t                        heapAllocations;

    Log("--------------------------------------------------------------------------------------------");
    Log("TestNat64PrefixSelection");

    InitTest();

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // Start Routing Manager. Check local NAT64 prefix generation.

    heapAllocations = sHeapAllocatedPtrs.GetLength();
    SuccessOrQuit(sInstance->Get<BorderRouter::RoutingManager>().SetEnabled(true));
    SuccessOrQuit(sInstance->Get<BorderRouter::RoutingManager>().GetNat64Prefix(localNat64));
    SuccessOrQuit(sInstance->Get<BorderRouter::RoutingManager>().GetOmrPrefix(localOmr));

    Log("Local nat64 prefix is %s", localNat64.ToString().AsCString());
    Log("Local OMR prefix is %s", localOmr.ToString().AsCString());

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // Enable Nat64 Prefix Manager. Check local NAT64 prefix in Network Data.

    sInstance->Get<BorderRouter::RoutingManager>().SetNat64PrefixManagerEnabled(true);

    AdvanceTime(20000);

    VerifyOmrPrefixInNetData(localOmr, /* aDefaultRoute */ false);
    VerifyNat64PrefixInNetData(localNat64);

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // AIL NAT64 prefix discovered. No infra-derived OMR prefix in Network Data.
    // Check local NAT64 prefix in Network Data.

    DiscoverNat64Prefix(ailNat64);

    AdvanceTime(20000);

    VerifyNat64PrefixInNetData(localNat64);

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // Add a medium preference OMR prefix into Network Data.
    // Check AIL NAT64 prefix published in Network Data.

    prefixConfig.Clear();
    prefixConfig.mPrefix       = omrPrefix;
    prefixConfig.mStable       = true;
    prefixConfig.mSlaac        = true;
    prefixConfig.mPreferred    = true;
    prefixConfig.mOnMesh       = true;
    prefixConfig.mDefaultRoute = false;
    prefixConfig.mPreference   = NetworkData::kRoutePreferenceMedium;

    SuccessOrQuit(otBorderRouterAddOnMeshPrefix(sInstance, &prefixConfig));
    SuccessOrQuit(otBorderRouterRegister(sInstance));

    AdvanceTime(20000);

    VerifyOmrPrefixInNetData(omrPrefix, /* aDefaultRoute */ false);
    VerifyNat64PrefixInNetData(ailNat64);

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // AIL NAT64 prefix removed.
    // Check local NAT64 prefix in Network Data.

    ailNat64.Clear();
    DiscoverNat64Prefix(ailNat64);

    AdvanceTime(20000);

    VerifyOmrPrefixInNetData(omrPrefix, /* aDefaultRoute */ false);
    VerifyNat64PrefixInNetData(localNat64);

    SuccessOrQuit(sInstance->Get<BorderRouter::RoutingManager>().SetEnabled(false));
    VerifyOrQuit(sHeapAllocatedPtrs.GetLength() == heapAllocations);

    Log("End of TestNat64PrefixSelection");
    FinalizeTest();
}
#endif // OPENTHREAD_CONFIG_NAT64_BORDER_ROUTING_ENABLE

#if OPENTHREAD_CONFIG_BORDER_ROUTING_DHCP6_PD_ENABLE
void VerifyPdOmrPrefix(const Ip6::Prefix &aPrefix)
{
    otBorderRoutingPrefixTableEntry platformPrefixInfo;

    VerifyOrQuit(otBorderRoutingGetPdOmrPrefix(sInstance, &platformPrefixInfo) == OT_ERROR_NONE);
    VerifyOrQuit(AsCoreType(&platformPrefixInfo.mPrefix) == aPrefix);
}

void VerifyNoPdOmrPrefix()
{
    otBorderRoutingPrefixTableEntry platformPrefixInfo;

    VerifyOrQuit(otBorderRoutingGetPdOmrPrefix(sInstance, &platformPrefixInfo) == OT_ERROR_NOT_FOUND);
}

void TestBorderRoutingProcessPlatfromGeneratedNd(void)
{
    Ip6::Prefix localOmr;
    uint16_t    heapAllocations;

    Log("--------------------------------------------------------------------------------------------");
    Log("TestBorderRoutingProcessPlatfromGeneratedNd");

    InitTest(/* aEnableBorderRouting */ true);
    heapAllocations = sHeapAllocatedPtrs.GetLength();

    otBorderRoutingDhcp6PdSetEnabled(sInstance, true);

    {
        SuccessOrQuit(sInstance->Get<BorderRouter::RoutingManager>().GetOmrPrefix(localOmr));
    }

    // 0. Reject invalid RA.
    Log("0. Invalid RA message.");
    {
        {
            const uint8_t testInvalidRaMessage[] = {
                0x86, 0x00, 0x00, 0x00, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
            };

            otPlatBorderRoutingProcessIcmp6Ra(sInstance, testInvalidRaMessage, sizeof(testInvalidRaMessage));
            VerifyNoPdOmrPrefix();
        }

        {
            const uint8_t testInvalidRaMessage[] = {
                0x87, 0x00, 0x00, 0x00, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
            };

            otPlatBorderRoutingProcessIcmp6Ra(sInstance, testInvalidRaMessage, sizeof(testInvalidRaMessage));
            VerifyNoPdOmrPrefix();
        }

        {
            const uint8_t testRaMessageWithInvalidPrefix[] = {
                0x86, 0x00, 0x00, 0x00, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                0x03, 0x04, 0x41, 0xc0, 0x00, 0x00, 0x10, 0xe1, 0x00, 0x00, 0x04, 0xd2, 0x00, 0x00, 0x00, 0x00,
                0x20, 0x01, 0x0d, 0xb8, 0x00, 0x01, 0x00, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
            };

            otPlatBorderRoutingProcessIcmp6Ra(sInstance, testRaMessageWithInvalidPrefix,
                                              sizeof(testRaMessageWithInvalidPrefix));
            VerifyNoPdOmrPrefix();
        }
    }

    // 1. Publish a prefix, and wait until it expired.
    Log("1. Simple RA message.");
    {
        Ip6::Prefix raPrefix = PrefixFromString("2001:db8:dead:beef::", 64);

        SendRouterAdvertToBorderRoutingProcessIcmp6Ra({Pio(raPrefix, kValidLitime, kPreferredLifetime)});

        sExpectedRios.Add(raPrefix);
        AdvanceTime(10000);

        VerifyPdOmrPrefix(raPrefix);
        VerifyOrQuit(sExpectedRios.SawAll());
        VerifyOmrPrefixInNetData(raPrefix, /* aDefaultRoute */ false);

        AdvanceTime(1500000);
        sExpectedRios.Clear();
        VerifyPdOmrPrefix(raPrefix);
        VerifyOmrPrefixInNetData(raPrefix, /* aDefaultRoute */ false);

        AdvanceTime(400000);
        // Deprecated prefixes will be removed.
        VerifyNoPdOmrPrefix();
        VerifyOmrPrefixInNetData(localOmr, /* aDefaultRoute */ false);
    }

    // 1.1. Publish a prefix, and wait until it expired.
    //      Multiple prefixes are advertised, only the smallest one will be used.
    Log("1.1. RA message with multiple prefixes.");
    {
        Ip6::Prefix raPrefix    = PrefixFromString("2001:db8:dead:beef::", 64);
        Ip6::Prefix ulaRaPrefix = PrefixFromString("fd01:db8:deaf:beef::", 64);

        SendRouterAdvertToBorderRoutingProcessIcmp6Ra({Pio(ulaRaPrefix, kValidLitime * 2, kPreferredLifetime * 2),
                                                       Pio(raPrefix, kValidLitime, kPreferredLifetime)});

        sExpectedRios.Add(raPrefix);
        AdvanceTime(10000);

        VerifyPdOmrPrefix(raPrefix);
        VerifyOrQuit(sExpectedRios.SawAll());
        VerifyOmrPrefixInNetData(raPrefix, /* aDefaultRoute */ false);

        AdvanceTime(1500000);
        sExpectedRios.Clear();
        VerifyPdOmrPrefix(raPrefix);
        VerifyOmrPrefixInNetData(raPrefix, /* aDefaultRoute */ false);

        AdvanceTime(400000);
        // Deprecated prefixes will be removed.
        VerifyNoPdOmrPrefix();
        VerifyOmrPrefixInNetData(localOmr, /* aDefaultRoute */ false);
    }

    // 2. Publish a prefix, and renew it before it expired.
    Log("2. Renew prefix lifetime.");
    {
        Ip6::Prefix raPrefix = PrefixFromString("2001:db8:1:2::", 64);

        SendRouterAdvertToBorderRoutingProcessIcmp6Ra({Pio(raPrefix, kValidLitime, kPreferredLifetime)});

        sExpectedRios.Add(raPrefix);
        AdvanceTime(10000);

        VerifyPdOmrPrefix(raPrefix);
        VerifyOrQuit(sExpectedRios.SawAll());
        VerifyOmrPrefixInNetData(raPrefix, /* aDefaultRoute */ false);

        AdvanceTime(1500000);
        sExpectedRios.Clear();
        VerifyPdOmrPrefix(raPrefix);
        VerifyOmrPrefixInNetData(raPrefix, /* aDefaultRoute */ false);

        SendRouterAdvertToBorderRoutingProcessIcmp6Ra({Pio(raPrefix, kValidLitime, kPreferredLifetime)});

        AdvanceTime(400000);
        VerifyPdOmrPrefix(raPrefix);
        VerifyOmrPrefixInNetData(raPrefix, /* aDefaultRoute */ false);

        AdvanceTime(1500000);
        VerifyNoPdOmrPrefix();
        VerifyOmrPrefixInNetData(localOmr, /* aDefaultRoute */ false);
    }

    // 3. Publish a prefix, and publish another prefix to replace it (with goodbye ra).
    Log("3. Update prefix.");
    {
        Ip6::Prefix raPrefix    = PrefixFromString("2001:db8:1:2::", 64);
        Ip6::Prefix newRaPrefix = PrefixFromString("2001:db8:3:4::", 64);

        SendRouterAdvertToBorderRoutingProcessIcmp6Ra({Pio(raPrefix, kValidLitime, kPreferredLifetime)});

        sExpectedRios.Add(raPrefix);
        sExpectedRios.Clear();
        AdvanceTime(10000);

        VerifyPdOmrPrefix(raPrefix);
        VerifyOmrPrefixInNetData(raPrefix, /* aDefaultRoute */ false);

        AdvanceTime(1000000);
        VerifyPdOmrPrefix(raPrefix);

        SendRouterAdvertToBorderRoutingProcessIcmp6Ra(
            {Pio(raPrefix, 0, 0), Pio(newRaPrefix, kValidLitime, kPreferredLifetime)});
        sExpectedRios.Add(newRaPrefix);

        AdvanceTime(1000000);
        VerifyOrQuit(sExpectedRios.SawAll());
        VerifyPdOmrPrefix(newRaPrefix);

        AdvanceTime(1000000);
        VerifyNoPdOmrPrefix();
        VerifyOmrPrefixInNetData(localOmr, /* aDefaultRoute */ false);
    }

    // 4. Short prefix will be extended to /64.
    Log("Short prefix");
    {
        // The prefix will be padded to a /64 prefix.
        Ip6::Prefix raPrefix = PrefixFromString("2001:db8:cafe:0::", 64);
        Ip6::Prefix realRaPrefix;

        realRaPrefix.Set(raPrefix.GetBytes(), 48);
        SendRouterAdvertToBorderRoutingProcessIcmp6Ra({Pio(realRaPrefix, kValidLitime, kPreferredLifetime)});

        sExpectedRios.Add(raPrefix);
        AdvanceTime(10000);

        VerifyPdOmrPrefix(raPrefix);
        VerifyOrQuit(sExpectedRios.SawAll());
        VerifyOmrPrefixInNetData(raPrefix, /* aDefaultRoute */ false);

        AdvanceTime(1500000);
        sExpectedRios.Clear();
        VerifyPdOmrPrefix(raPrefix);
        VerifyOmrPrefixInNetData(raPrefix, /* aDefaultRoute */ false);

        AdvanceTime(400000);
        // Deprecated prefixes will be removed.
        VerifyNoPdOmrPrefix();
        VerifyOmrPrefixInNetData(localOmr, /* aDefaultRoute */ false);
    }

    SuccessOrQuit(otBorderRoutingSetEnabled(sInstance, false));
    VerifyOrQuit(sHeapAllocatedPtrs.GetLength() <= heapAllocations);

    Log("End of TestBorderRoutingProcessPlatfromGeneratedNd");

    FinalizeTest();
}
#endif // OPENTHREAD_CONFIG_BORDER_ROUTING_DHCP6_PD_ENABLE

#endif // OPENTHREAD_CONFIG_BORDER_ROUTING_ENABLE

} // namespace ot

int main(void)
{
#if OPENTHREAD_CONFIG_BORDER_ROUTING_ENABLE
    ot::TestSamePrefixesFromMultipleRouters();
    ot::TestOmrSelection();
    ot::TestDefaultRoute();
    ot::TestAdvNonUlaRoute();
    ot::TestLocalOnLinkPrefixDeprecation();
#if OPENTHREAD_CONFIG_BACKBONE_ROUTER_ENABLE
    ot::TestDomainPrefixAsOmr();
#endif
    ot::TestExtPanIdChange();
    ot::TestConflictingPrefix();
    ot::TestRouterNsProbe();
    ot::TestLearningAndCopyingOfFlags();
#if OPENTHREAD_CONFIG_PLATFORM_FLASH_API_ENABLE
    ot::TestSavedOnLinkPrefixes();
#endif
#if OPENTHREAD_CONFIG_SRP_SERVER_ENABLE
    ot::TestAutoEnableOfSrpServer();
#endif
#if OPENTHREAD_CONFIG_NAT64_BORDER_ROUTING_ENABLE
    ot::TestNat64PrefixSelection();
#endif
#if OPENTHREAD_CONFIG_BORDER_ROUTING_DHCP6_PD_ENABLE
    ot::TestBorderRoutingProcessPlatfromGeneratedNd();
#endif

    printf("All tests passed\n");
#else
    printf("BORDER_ROUTING feature is not enabled\n");
#endif

    return 0;
}
