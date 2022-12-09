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

#include <openthread/thread.h>

#include "border_router/routing_manager.hpp"
#include "common/arg_macros.hpp"
#include "common/array.hpp"
#include "common/instance.hpp"
#include "common/time.hpp"
#include "net/icmp6.hpp"
#include "net/nd6.hpp"

#if OPENTHREAD_CONFIG_BORDER_ROUTING_ENABLE

using namespace ot;

// Logs a message and adds current time (sNow) as "<hours>:<min>:<secs>.<msec>"
#define Log(...)                                                                                         \
    printf("%02u:%02u:%02u.%03u " OT_FIRST_ARG(__VA_ARGS__) "\n", (sNow / 3600000), (sNow / 60000) % 60, \
           (sNow / 1000) % 60, sNow % 1000 OT_REST_ARGS(__VA_ARGS__))

static constexpr uint32_t kInfraIfIndex     = 1;
static const char         kInfraIfAddress[] = "fe80::1";

static constexpr uint32_t kValidLitime       = 2000;
static constexpr uint32_t kPreferredLifetime = 1800;

static constexpr uint16_t kMaxRaSize              = 800;
static constexpr uint16_t kMaxDeprecatingPrefixes = 16;

static ot::Instance *sInstance;

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

extern "C" {

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

    VerifyOrQuit(raMsg.IsValid());

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

        default:
            VerifyOrQuit(false, "Unexpected option type in RA msg");
        }
    }

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

void LogRouterAdvert(const Icmp6Packet &aPacket)
{
    Ip6::Nd::RouterAdvertMessage raMsg(aPacket);

    VerifyOrQuit(raMsg.IsValid());

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

        default:
            VerifyOrQuit(false, "Bad option type in RA msg");
        }
    }
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

void SendRouterAdvert(const Ip6::Address &aAddress, const Icmp6Packet &aPacket)
{
    otPlatInfraIfRecvIcmp6Nd(sInstance, kInfraIfIndex, &aAddress, aPacket.GetBytes(), aPacket.GetLength());
}

void SendNeighborAdvert(const Ip6::Address &aAddress, const Ip6::Nd::NeighborAdvertMessage &aNaMessage)
{
    Log("Sending NA from %s", aAddress.ToString().AsCString());
    otPlatInfraIfRecvIcmp6Nd(sInstance, kInfraIfIndex, &aAddress, reinterpret_cast<const uint8_t *>(&aNaMessage),
                             sizeof(aNaMessage));
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

void VerifyOmrPrefixInNetData(const Ip6::Prefix &aOmrPrefix, bool aDefaultRoute = false)
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

struct ExternalRoute
{
    ExternalRoute(const Ip6::Prefix &aPrefix, RoutePreference aPreference)
        : mPrefix(aPrefix)
        , mPreference(aPreference)
    {
    }

    const Ip6::Prefix &mPrefix;
    RoutePreference    mPreference;
};

template <uint16_t kLength> void VerifyExternalRoutesInNetData(const ExternalRoute (&aExternRoutes)[kLength])
{
    otNetworkDataIterator            iterator = OT_NETWORK_DATA_ITERATOR_INIT;
    NetworkData::ExternalRouteConfig routeConfig;
    uint16_t                         counter;

    Log("VerifyExternalRoutesInNetData()");

    counter = 0;

    while (otNetDataGetNextRoute(sInstance, &iterator, &routeConfig) == kErrorNone)
    {
        bool didFind = false;

        counter++;

        Log("   prefix:%s, prf:%s", routeConfig.GetPrefix().ToString().AsCString(),
            PreferenceToString(routeConfig.mPreference));

        for (const ExternalRoute &externalRoute : aExternRoutes)
        {
            if (externalRoute.mPrefix == routeConfig.GetPrefix())
            {
                VerifyOrQuit(static_cast<int8_t>(routeConfig.mPreference) == externalRoute.mPreference);
                didFind = true;
                break;
            }
        }

        VerifyOrQuit(didFind);
    }

    VerifyOrQuit(counter == kLength);
}

void VerifyNoExternalRouteInNetData(void)
{
    otNetworkDataIterator            iterator = OT_NETWORK_DATA_ITERATOR_INIT;
    NetworkData::ExternalRouteConfig routeConfig;

    Log("VerifyNoExternalRouteInNetData()");
    VerifyOrQuit(otNetDataGetNextRoute(sInstance, &iterator, &routeConfig) != kErrorNone);
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

void SendRouterAdvert(const Ip6::Address &aRouterAddress,
                      const Pio          *aPios,
                      uint16_t            aNumPios,
                      const Rio          *aRios,
                      uint16_t            aNumRios,
                      const DefaultRoute &aDefaultRoute)
{
    Ip6::Nd::RouterAdvertMessage::Header header;
    uint8_t                              buffer[kMaxRaSize];

    header.SetRouterLifetime(aDefaultRoute.mLifetime);
    header.SetDefaultRouterPreference(aDefaultRoute.mPreference);

    {
        Ip6::Nd::RouterAdvertMessage raMsg(header, buffer);

        for (; aNumPios > 0; aPios++, aNumPios--)
        {
            SuccessOrQuit(
                raMsg.AppendPrefixInfoOption(aPios->mPrefix, aPios->mValidLifetime, aPios->mPreferredLifetime));
        }

        for (; aNumRios > 0; aRios++, aNumRios--)
        {
            SuccessOrQuit(raMsg.AppendRouteInfoOption(aRios->mPrefix, aRios->mValidLifetime, aRios->mPreference));
        }

        SendRouterAdvert(aRouterAddress, raMsg.GetAsPacket());
        Log("Sending RA from router %s", aRouterAddress.ToString().AsCString());
        LogRouterAdvert(raMsg.GetAsPacket());
    }
}

template <uint16_t kNumPios, uint16_t kNumRios>
void SendRouterAdvert(const Ip6::Address &aRouterAddress,
                      const Pio (&aPios)[kNumPios],
                      const Rio (&aRios)[kNumRios],
                      const DefaultRoute &aDefaultRoute = DefaultRoute(0, NetworkData::kRoutePreferenceMedium))
{
    SendRouterAdvert(aRouterAddress, aPios, kNumPios, aRios, kNumRios, aDefaultRoute);
}

template <uint16_t kNumPios>
void SendRouterAdvert(const Ip6::Address &aRouterAddress,
                      const Pio (&aPios)[kNumPios],
                      const DefaultRoute &aDefaultRoute = DefaultRoute(0, NetworkData::kRoutePreferenceMedium))
{
    SendRouterAdvert(aRouterAddress, aPios, kNumPios, nullptr, 0, aDefaultRoute);
}

template <uint16_t kNumRios>
void SendRouterAdvert(const Ip6::Address &aRouterAddress,
                      const Rio (&aRios)[kNumRios],
                      const DefaultRoute &aDefaultRoute = DefaultRoute(0, NetworkData::kRoutePreferenceMedium))
{
    SendRouterAdvert(aRouterAddress, nullptr, 0, aRios, kNumRios, aDefaultRoute);
}

void SendRouterAdvert(const Ip6::Address &aRouterAddress, const DefaultRoute &aDefaultRoute)
{
    SendRouterAdvert(aRouterAddress, nullptr, 0, nullptr, 0, aDefaultRoute);
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
                AsCoreType(&entry.mRouterAddress).ToString().AsCString(), entry.mMsecSinceLastUpdate / 1000);

            onLinkPrefixCount++;

            for (uint16_t index = 0; index < aNumOnLinkPrefixes; index++)
            {
                const OnLinkPrefix &onLinkPrefix = aOnLinkPrefixes[index];

                if ((onLinkPrefix.mPrefix == AsCoreType(&entry.mPrefix)) &&
                    (AsCoreType(&entry.mRouterAddress) == onLinkPrefix.mRouterAddress))
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
                PreferenceToString(entry.mRoutePreference), AsCoreType(&entry.mRouterAddress).ToString().AsCString(),
                entry.mMsecSinceLastUpdate / 1000);

            routePrefixCount++;

            for (uint16_t index = 0; index < aNumRoutePrefixes; index++)
            {
                const RoutePrefix &routePrefix = aRoutePrefixes[index];

                if ((routePrefix.mPrefix == AsCoreType(&entry.mPrefix)) &&
                    (AsCoreType(&entry.mRouterAddress) == routePrefix.mRouterAddress))
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

void InitTest(bool aEnablBorderRouting = false)
{
    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // Initialize OT instance.

    sNow      = 0;
    sInstance = static_cast<Instance *>(testInitInstance());

    memset(&sRadioTxFrame, 0, sizeof(sRadioTxFrame));
    sRadioTxFrame.mPsdu = sRadioTxFramePsdu;

    SuccessOrQuit(sInfraIfAddress.FromString(kInfraIfAddress));

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // Initialize and start Border Router and Thread operation.

    SuccessOrQuit(otBorderRoutingInit(sInstance, kInfraIfIndex, /* aInfraIfIsRunning */ true));

    SuccessOrQuit(otLinkSetPanId(sInstance, 0x1234));
    SuccessOrQuit(otIp6SetEnabled(sInstance, true));
    SuccessOrQuit(otThreadSetEnabled(sInstance, true));
    SuccessOrQuit(otBorderRoutingSetEnabled(sInstance, aEnablBorderRouting));

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // Ensure device starts as leader.

    AdvanceTime(10000);

    VerifyOrQuit(otThreadGetDeviceRole(sInstance) == OT_DEVICE_ROLE_LEADER);

    // Reset all test flags
    sRsEmitted   = false;
    sRaValidated = false;
    sExpectedPio = kNoPio;
    sExpectedRios.Clear();
    sRespondToNs = true;
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

    Log("--------------------------------------------------------------------------------------------");
    Log("TestSamePrefixesFromMultipleRouters");

    InitTest();

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
    // Check Network Data to include the local OMR and on-link prefix.

    VerifyOmrPrefixInNetData(localOmr);
    VerifyExternalRoutesInNetData({ExternalRoute(localOnLink, NetworkData::kRoutePreferenceMedium)});

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

    VerifyOmrPrefixInNetData(localOmr);
    VerifyExternalRoutesInNetData({ExternalRoute(localOnLink, NetworkData::kRoutePreferenceMedium),
                                   ExternalRoute(onLinkPrefix, NetworkData::kRoutePreferenceMedium),
                                   ExternalRoute(routePrefix, NetworkData::kRoutePreferenceMedium)});

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

    VerifyOmrPrefixInNetData(localOmr);

    // We expect to see 3 entries, our local on link and new prefixes
    // from router A and B. The `routePrefix` now should have high
    // preference.

    VerifyExternalRoutesInNetData({ExternalRoute(routePrefix, NetworkData::kRoutePreferenceHigh),
                                   ExternalRoute(localOnLink, NetworkData::kRoutePreferenceMedium),
                                   ExternalRoute(onLinkPrefix, NetworkData::kRoutePreferenceMedium)});

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
    // Check Network Data, all prefixes should be again at medium preference.

    VerifyExternalRoutesInNetData({ExternalRoute(localOnLink, NetworkData::kRoutePreferenceMedium),
                                   ExternalRoute(onLinkPrefix, NetworkData::kRoutePreferenceMedium),
                                   ExternalRoute(routePrefix, NetworkData::kRoutePreferenceMedium)});

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

    Log("End of TestSamePrefixesFromMultipleRouters");

    FinalizeTest();
}

void TestOmrSelection(void)
{
    Ip6::Prefix                     localOnLink;
    Ip6::Prefix                     localOmr;
    Ip6::Prefix                     omrPrefix = PrefixFromString("2000:0000:1111:4444::", 64);
    NetworkData::OnMeshPrefixConfig prefixConfig;

    Log("--------------------------------------------------------------------------------------------");
    Log("TestOmrSelection");

    InitTest();

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
    // Check Network Data to include the local OMR and on-link prefix.

    VerifyOmrPrefixInNetData(localOmr);
    VerifyExternalRoutesInNetData({ExternalRoute(localOnLink, NetworkData::kRoutePreferenceMedium)});

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
    // Make sure BR emits RA with new OMR prefix now.

    sRaValidated = false;
    sExpectedPio = kPioAdvertisingLocalOnLink;
    sExpectedRios.Clear();
    sExpectedRios.Add(omrPrefix);

    AdvanceTime(20000);

    VerifyOrQuit(sRaValidated);
    VerifyOrQuit(sExpectedRios.SawAll());

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // Check Network Data. We should now see that the local OMR prefix
    // is removed.

    VerifyOmrPrefixInNetData(omrPrefix);

    VerifyExternalRoutesInNetData({ExternalRoute(localOnLink, NetworkData::kRoutePreferenceMedium)});

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // Remove the OMR prefix previously added in net data.

    SuccessOrQuit(otBorderRouterRemoveOnMeshPrefix(sInstance, &omrPrefix));
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

    VerifyOmrPrefixInNetData(localOmr);

    VerifyExternalRoutesInNetData({ExternalRoute(localOnLink, NetworkData::kRoutePreferenceMedium)});

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

    Log("End of TestOmrSelection");
    FinalizeTest();
}

void TestDefaultRoute(void)
{
    Ip6::Prefix                     localOnLink;
    Ip6::Prefix                     localOmr;
    Ip6::Prefix                     onLinkPrefix   = PrefixFromString("2000:abba:baba::", 64);
    Ip6::Prefix                     routePrefix    = PrefixFromString("2000:1234:5678::", 64);
    Ip6::Prefix                     omrPrefix      = PrefixFromString("2000:0000:1111:4444::", 64);
    Ip6::Prefix                     defaultRoute   = PrefixFromString("::", 0);
    Ip6::Address                    routerAddressA = AddressFromString("fd00::aaaa");
    Ip6::Address                    routerAddressB = AddressFromString("fd00::bbbb");
    NetworkData::OnMeshPrefixConfig prefixConfig;

    Log("--------------------------------------------------------------------------------------------");
    Log("TestDefaultRoute");

    InitTest();

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // Start Routing Manager

    SuccessOrQuit(sInstance->Get<BorderRouter::RoutingManager>().SetEnabled(true));

    SuccessOrQuit(sInstance->Get<BorderRouter::RoutingManager>().GetOnLinkPrefix(localOnLink));
    SuccessOrQuit(sInstance->Get<BorderRouter::RoutingManager>().GetOmrPrefix(localOmr));

    AdvanceTime(500);

    Log("Local on-link prefix is %s", localOnLink.ToString().AsCString());
    Log("Local OMR prefix is %s", localOmr.ToString().AsCString());

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // Send an RA from router A and B adding an onlink prefix
    // and routePrefix from A, and a default route from B.

    SendRouterAdvert(routerAddressA, {Pio(onLinkPrefix, kValidLitime, kPreferredLifetime)},
                     {Rio(routePrefix, kValidLitime, NetworkData::kRoutePreferenceMedium)});

    SendRouterAdvert(routerAddressB, DefaultRoute(kValidLitime, NetworkData::kRoutePreferenceLow));

    sRsEmitted   = false;
    sRaValidated = false;
    sExpectedPio = kNoPio;
    sExpectedRios.Clear();

    AdvanceTime(10000);

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // Check the discovered prefix table and ensure info from router B
    // is now included in the table.

    VerifyPrefixTable({OnLinkPrefix(onLinkPrefix, kValidLitime, kPreferredLifetime, routerAddressA)},
                      {RoutePrefix(routePrefix, kValidLitime, NetworkData::kRoutePreferenceMedium, routerAddressA),
                       RoutePrefix(defaultRoute, kValidLitime, NetworkData::kRoutePreferenceLow, routerAddressB)});

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // Check Network Data. We should not see default route in
    // Network Data yet (since there is no OMR prefix with default
    // route flag).

    VerifyExternalRoutesInNetData({ExternalRoute(onLinkPrefix, NetworkData::kRoutePreferenceMedium),
                                   ExternalRoute(routePrefix, NetworkData::kRoutePreferenceMedium)});

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // Add an OMR prefix directly into Network Data with default route
    // flag.

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
    // Check Network Data. We should now see default route from
    // router B.

    VerifyExternalRoutesInNetData({ExternalRoute(onLinkPrefix, NetworkData::kRoutePreferenceMedium),
                                   ExternalRoute(routePrefix, NetworkData::kRoutePreferenceMedium),
                                   ExternalRoute(defaultRoute, NetworkData::kRoutePreferenceLow)});

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // Send an RA from router B adding a default route
    // now also as ::/0 prefix RIO with a high preference.

    SendRouterAdvert(routerAddressB, {Rio(defaultRoute, kValidLitime, NetworkData::kRoutePreferenceHigh)},
                     DefaultRoute(kValidLitime, NetworkData::kRoutePreferenceLow));

    AdvanceTime(10000);

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // Check the discovered prefix table and ensure default route
    // entry from router B is now correctly updated to use high
    // preference (RIO overrides the default route info from header).

    VerifyPrefixTable({OnLinkPrefix(onLinkPrefix, kValidLitime, kPreferredLifetime, routerAddressA)},
                      {RoutePrefix(routePrefix, kValidLitime, NetworkData::kRoutePreferenceMedium, routerAddressA),
                       RoutePrefix(defaultRoute, kValidLitime, NetworkData::kRoutePreferenceHigh, routerAddressB)});

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // Check Network Data. We should now see default route from
    // router B included with high preference.

    VerifyExternalRoutesInNetData({ExternalRoute(onLinkPrefix, NetworkData::kRoutePreferenceMedium),
                                   ExternalRoute(routePrefix, NetworkData::kRoutePreferenceMedium),
                                   ExternalRoute(defaultRoute, NetworkData::kRoutePreferenceHigh)});

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // Remove the OMR prefix previously added with default route
    // flag.

    SuccessOrQuit(otBorderRouterRemoveOnMeshPrefix(sInstance, &omrPrefix));
    SuccessOrQuit(otBorderRouterRegister(sInstance));

    AdvanceTime(10000);

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // Check Network Data. The default route ::/0 should be now
    // removed since the OMR prefix with default route is removed.

    VerifyExternalRoutesInNetData({ExternalRoute(onLinkPrefix, NetworkData::kRoutePreferenceMedium),
                                   ExternalRoute(routePrefix, NetworkData::kRoutePreferenceMedium)});

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // Re-add the OMR prefix with default route flag.

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
    // Check Network Data. We should again see the default route from
    // router B included with high preference.

    VerifyExternalRoutesInNetData({ExternalRoute(onLinkPrefix, NetworkData::kRoutePreferenceMedium),
                                   ExternalRoute(routePrefix, NetworkData::kRoutePreferenceMedium),
                                   ExternalRoute(defaultRoute, NetworkData::kRoutePreferenceHigh)});

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // Send an RA from router B removing default route
    // in both header and in RIO.

    SendRouterAdvert(routerAddressB, {Rio(defaultRoute, 0, NetworkData::kRoutePreferenceHigh)},
                     DefaultRoute(0, NetworkData::kRoutePreferenceMedium));
    AdvanceTime(10000);

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // Check the discovered prefix table and ensure default route
    // entry from router B is no longer present.

    VerifyPrefixTable({OnLinkPrefix(onLinkPrefix, kValidLitime, kPreferredLifetime, routerAddressA)},
                      {RoutePrefix(routePrefix, kValidLitime, NetworkData::kRoutePreferenceMedium, routerAddressA)});

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // Check Network Data. The default route ::/0 should be now
    // removed since router B stopped advertising it.

    VerifyExternalRoutesInNetData({ExternalRoute(onLinkPrefix, NetworkData::kRoutePreferenceMedium),
                                   ExternalRoute(routePrefix, NetworkData::kRoutePreferenceMedium)});

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

    Log("End of TestDefaultRoute");

    FinalizeTest();
}

void TestLocalOnLinkPrefixDeprecation(void)
{
    static constexpr uint32_t kMaxRaTxInterval = 601; // In seconds

    Ip6::Prefix  localOnLink;
    Ip6::Prefix  localOmr;
    Ip6::Prefix  onLinkPrefix   = PrefixFromString("2000:abba:baba::", 64);
    Ip6::Address routerAddressA = AddressFromString("fd00::aaaa");
    uint32_t     localOnLinkLifetime;

    Log("--------------------------------------------------------------------------------------------");
    Log("TestLocalOnLinkPrefixDeprecation");

    InitTest();

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // Start Routing Manager. Check emitted RS and RA messages.

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

    VerifyOmrPrefixInNetData(localOmr);
    VerifyExternalRoutesInNetData({ExternalRoute(localOnLink, NetworkData::kRoutePreferenceMedium)});

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

    VerifyOmrPrefixInNetData(localOmr);
    VerifyExternalRoutesInNetData({ExternalRoute(localOnLink, NetworkData::kRoutePreferenceMedium),
                                   ExternalRoute(onLinkPrefix, NetworkData::kRoutePreferenceMedium)});

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // Wait for local on-link prefix to expire

    while (localOnLinkLifetime > kMaxRaTxInterval)
    {
        // Send same RA from router A to keep the on-link prefix alive.

        SendRouterAdvert(routerAddressA, {Pio(onLinkPrefix, kValidLitime, kPreferredLifetime)});

        // Ensure Network Data entries remain as before. Mainly we still
        // see the deprecating local on-link prefix.

        VerifyOmrPrefixInNetData(localOmr);
        VerifyExternalRoutesInNetData({ExternalRoute(localOnLink, NetworkData::kRoutePreferenceMedium),
                                       ExternalRoute(onLinkPrefix, NetworkData::kRoutePreferenceMedium)});

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
    // Check Network Data and make sure the the expired local on-link prefix
    // is removed.

    VerifyOmrPrefixInNetData(localOmr);
    VerifyExternalRoutesInNetData({ExternalRoute(onLinkPrefix, NetworkData::kRoutePreferenceMedium)});

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

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

    Log("--------------------------------------------------------------------------------------------");
    Log("TestDomainPrefixAsOmr");

    InitTest();

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
    // Check Network Data to include the local OMR and on-link prefix.

    VerifyOmrPrefixInNetData(localOmr);
    VerifyExternalRoutesInNetData({ExternalRoute(localOnLink, NetworkData::kRoutePreferenceMedium)});

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
    VerifyOrQuit(sExpectedRios[1].mLifetime == 0);

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

    VerifyOmrPrefixInNetData(domainPrefix);
    VerifyExternalRoutesInNetData({ExternalRoute(localOnLink, NetworkData::kRoutePreferenceMedium)});

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

    VerifyOmrPrefixInNetData(localOmr);
    VerifyExternalRoutesInNetData({ExternalRoute(localOnLink, NetworkData::kRoutePreferenceMedium)});

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

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

    Log("--------------------------------------------------------------------------------------------");
    Log("TestExtPanIdChange");

    InitTest();

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // Start Routing Manager. Check emitted RS and RA messages.

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
    // Validate the Network Data to contain both the current and old
    // local on-link prefixes.

    VerifyOmrPrefixInNetData(localOmr);
    VerifyExternalRoutesInNetData({ExternalRoute(localOnLink, NetworkData::kRoutePreferenceMedium),
                                   ExternalRoute(oldLocalOnLink, NetworkData::kRoutePreferenceMedium)});

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
    VerifyNoExternalRouteInNetData();

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
        // Ensure Network Data entries remain as before. Mainly we still
        // see the deprecating local on-link prefix.

        VerifyExternalRoutesInNetData({ExternalRoute(localOnLink, NetworkData::kRoutePreferenceMedium),
                                       ExternalRoute(oldLocalOnLink, NetworkData::kRoutePreferenceMedium)});

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

    AdvanceTime(2 * kMaxRaTxInterval * 1000);

    VerifyOrQuit(sRaValidated);
    VerifyOrQuit(sDeprecatingPrefixes.IsEmpty());
    Log("Old on-link prefix is now expired");

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // Validate the Network Data now only contains the current local
    // on-link prefix.

    VerifyOmrPrefixInNetData(localOmr);
    VerifyExternalRoutesInNetData({ExternalRoute(localOnLink, NetworkData::kRoutePreferenceMedium)});

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
    // Validate that Network Data contains the old local on-link
    // prefix along with entry from router A.

    VerifyOmrPrefixInNetData(localOmr);
    VerifyExternalRoutesInNetData({ExternalRoute(oldLocalOnLink, NetworkData::kRoutePreferenceMedium),
                                   ExternalRoute(onLinkPrefix, NetworkData::kRoutePreferenceMedium)});

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // Wait for old local on-link prefix to expire.

    while (oldPrefixLifetime > 2 * kMaxRaTxInterval)
    {
        // Send same RA from router A to keep its on-link prefix alive.

        SendRouterAdvert(routerAddressA, {Pio(onLinkPrefix, kValidLitime, kPreferredLifetime)});

        // Ensure Network Data entries remain as before. Mainly we still
        // see the deprecating old local on-link prefix.

        VerifyExternalRoutesInNetData({ExternalRoute(oldLocalOnLink, NetworkData::kRoutePreferenceMedium),
                                       ExternalRoute(onLinkPrefix, NetworkData::kRoutePreferenceMedium)});

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

    AdvanceTime(2 * kMaxRaTxInterval * 1000);

    VerifyOrQuit(sRaValidated);
    VerifyOrQuit(sDeprecatingPrefixes.IsEmpty());
    Log("Old on-link prefix is now expired");

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // Validate the Network Data to now only contains entry from router A.

    VerifyOmrPrefixInNetData(localOmr);
    VerifyExternalRoutesInNetData({ExternalRoute(onLinkPrefix, NetworkData::kRoutePreferenceMedium)});

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
    // Validate the Network Data to now only contains entry from router A.

    VerifyOmrPrefixInNetData(localOmr);
    VerifyExternalRoutesInNetData({ExternalRoute(onLinkPrefix, NetworkData::kRoutePreferenceMedium)});

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
    // Validate that it is unpublished from network data.

    AdvanceTime(2000 * 1000);
    VerifyOmrPrefixInNetData(localOmr);
    VerifyExternalRoutesInNetData({ExternalRoute(localOnLink, NetworkData::kRoutePreferenceMedium)});

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

    VerifyExternalRoutesInNetData({ExternalRoute(localOnLink, NetworkData::kRoutePreferenceMedium),
                                   ExternalRoute(oldPrefixes[0], NetworkData::kRoutePreferenceMedium)});

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

    VerifyExternalRoutesInNetData({ExternalRoute(localOnLink, NetworkData::kRoutePreferenceMedium),
                                   ExternalRoute(oldPrefixes[0], NetworkData::kRoutePreferenceMedium),
                                   ExternalRoute(oldPrefixes[1], NetworkData::kRoutePreferenceMedium)});

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

    VerifyExternalRoutesInNetData({ExternalRoute(localOnLink, NetworkData::kRoutePreferenceMedium),
                                   ExternalRoute(oldPrefixes[0], NetworkData::kRoutePreferenceMedium),
                                   ExternalRoute(oldPrefixes[1], NetworkData::kRoutePreferenceMedium),
                                   ExternalRoute(oldPrefixes[2], NetworkData::kRoutePreferenceMedium)});

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

    VerifyExternalRoutesInNetData({ExternalRoute(localOnLink, NetworkData::kRoutePreferenceMedium),
                                   ExternalRoute(oldPrefixes[0], NetworkData::kRoutePreferenceMedium),
                                   ExternalRoute(oldPrefixes[1], NetworkData::kRoutePreferenceMedium),
                                   ExternalRoute(oldPrefixes[3], NetworkData::kRoutePreferenceMedium)});

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
    VerifyNoExternalRouteInNetData();

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // Wait for 15 minutes while BR stays disabled and validate
    // there are no emitted RAs. We want to check that deprecating
    // prefixes continue to expire while BR is stopped.

    sRaValidated = false;
    AdvanceTime(15 * 60 * 1000);

    VerifyOrQuit(!sRaValidated);

    VerifyNoOmrPrefixInNetData();
    VerifyNoExternalRouteInNetData();

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

    VerifyExternalRoutesInNetData({ExternalRoute(localOnLink, NetworkData::kRoutePreferenceMedium),
                                   ExternalRoute(oldPrefixes[3], NetworkData::kRoutePreferenceMedium)});

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

    VerifyExternalRoutesInNetData({ExternalRoute(localOnLink, NetworkData::kRoutePreferenceMedium),
                                   ExternalRoute(oldPrefixes[0], NetworkData::kRoutePreferenceMedium),
                                   ExternalRoute(oldPrefixes[1], NetworkData::kRoutePreferenceMedium),
                                   ExternalRoute(oldPrefixes[2], NetworkData::kRoutePreferenceMedium)});

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

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

    Log("--------------------------------------------------------------------------------------------");
    Log("TestRouterNsProbe");

    InitTest();

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

    Log("End of TestRouterNsProbe");
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

    Log("--------------------------------------------------------------------------------------------");
    Log("TestConflictingPrefix");

    InitTest();

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
    // Check Network Data to include the local OMR and on-link prefix.

    VerifyOmrPrefixInNetData(localOmr);
    VerifyExternalRoutesInNetData({ExternalRoute(localOnLink, NetworkData::kRoutePreferenceMedium)});

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
    // Check Network Data to still include the local OMR and on-link prefix.

    VerifyOmrPrefixInNetData(localOmr);
    VerifyExternalRoutesInNetData({ExternalRoute(localOnLink, NetworkData::kRoutePreferenceMedium)});

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // Send an RA from router A removing local on-link prefix as RIO.

    SendRouterAdvert(routerAddressA, {Rio(localOnLink, 0, NetworkData::kRoutePreferenceMedium)});

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // Verify that on-link prefix is still included in Network Data and
    // the change by router A did not cause it to be unpublished.

    AdvanceTime(10000);
    VerifyExternalRoutesInNetData({ExternalRoute(localOnLink, NetworkData::kRoutePreferenceMedium)});

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // Check that the local on-link prefix is still being advertised.

    sRaValidated = false;
    AdvanceTime(610000);
    VerifyOrQuit(sRaValidated);

    VerifyExternalRoutesInNetData({ExternalRoute(localOnLink, NetworkData::kRoutePreferenceMedium)});

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
    // Check Network Data to include the new on-link prefix from router B and
    // the deprecating local on-link prefix.

    VerifyOmrPrefixInNetData(localOmr);
    VerifyExternalRoutesInNetData({ExternalRoute(localOnLink, NetworkData::kRoutePreferenceMedium),
                                   ExternalRoute(onLinkPrefix, NetworkData::kRoutePreferenceMedium)});

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

    VerifyExternalRoutesInNetData({ExternalRoute(localOnLink, NetworkData::kRoutePreferenceMedium),
                                   ExternalRoute(onLinkPrefix, NetworkData::kRoutePreferenceMedium)});

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // Send an RA from router A removing the previous RIO.

    SendRouterAdvert(routerAddressA, {Rio(localOnLink, 0, NetworkData::kRoutePreferenceMedium)});

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // Check Network Data remains unchanged and still contains
    // the deprecating local on-link prefix.

    AdvanceTime(60000);
    VerifyExternalRoutesInNetData({ExternalRoute(localOnLink, NetworkData::kRoutePreferenceMedium),
                                   ExternalRoute(onLinkPrefix, NetworkData::kRoutePreferenceMedium)});

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
    // Check Network Data still contains both prefixes.

    VerifyExternalRoutesInNetData({ExternalRoute(localOnLink, NetworkData::kRoutePreferenceMedium),
                                   ExternalRoute(onLinkPrefix, NetworkData::kRoutePreferenceMedium)});

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
    // Check Network Data contains old and new local on-link prefix
    // and deprecating prefix from router B.

    VerifyExternalRoutesInNetData({ExternalRoute(localOnLink, NetworkData::kRoutePreferenceMedium),
                                   ExternalRoute(oldLocalOnLink, NetworkData::kRoutePreferenceMedium),
                                   ExternalRoute(onLinkPrefix, NetworkData::kRoutePreferenceMedium)});

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // Send an RA from router A again adding the old local on-link prefix
    // as RIO.

    SendRouterAdvert(routerAddressA, {Rio(oldLocalOnLink, kValidLitime, NetworkData::kRoutePreferenceMedium)});

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // Check Network Data remains unchanged.

    AdvanceTime(10000);
    VerifyExternalRoutesInNetData({ExternalRoute(localOnLink, NetworkData::kRoutePreferenceMedium),
                                   ExternalRoute(oldLocalOnLink, NetworkData::kRoutePreferenceMedium),
                                   ExternalRoute(onLinkPrefix, NetworkData::kRoutePreferenceMedium)});

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // Send an RA from router A removing the previous RIO.

    SendRouterAdvert(routerAddressA, {Rio(localOnLink, 0, NetworkData::kRoutePreferenceMedium)});

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // Check Network Data remains unchanged.

    AdvanceTime(10000);
    VerifyExternalRoutesInNetData({ExternalRoute(localOnLink, NetworkData::kRoutePreferenceMedium),
                                   ExternalRoute(oldLocalOnLink, NetworkData::kRoutePreferenceMedium),
                                   ExternalRoute(onLinkPrefix, NetworkData::kRoutePreferenceMedium)});

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

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
    // Check Network Data to include the local OMR and on-link prefix.

    VerifyOmrPrefixInNetData(localOmr);
    VerifyExternalRoutesInNetData({ExternalRoute(localOnLink, NetworkData::kRoutePreferenceMedium)});

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // Disable the instance and re-enable it.

    Log("Disabling and re-enabling OT Instance");

    testFreeInstance(sInstance);

    InitTest(/* aEnablBorderRouting */ true);

    SuccessOrQuit(sInstance->Get<BorderRouter::RoutingManager>().SetEnabled(true));

    sExpectedPio = kPioAdvertisingLocalOnLink;

    AdvanceTime(30000);

    VerifyOrQuit(sRsEmitted);
    VerifyOrQuit(sRaValidated);
    VerifyOrQuit(sExpectedRios.SawAll());

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // Check Network Data to include the local OMR and on-link prefix.

    VerifyOmrPrefixInNetData(localOmr);
    VerifyExternalRoutesInNetData({ExternalRoute(localOnLink, NetworkData::kRoutePreferenceMedium)});

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

    InitTest(/* aEnablBorderRouting */ true);

    sExpectedPio = kPioAdvertisingLocalOnLink;

    AdvanceTime(30000);

    VerifyOrQuit(sRsEmitted);
    VerifyOrQuit(sRaValidated);
    VerifyOrQuit(sExpectedRios.SawAll());

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // Check Network Data to include the local OMR and on-link prefix.

    VerifyOmrPrefixInNetData(localOmr);
    VerifyExternalRoutesInNetData({ExternalRoute(localOnLink, NetworkData::kRoutePreferenceMedium)});

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

    InitTest();

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
    // Check the saved prefixes are in netdata and being
    // deprecated.

    VerifyExternalRoutesInNetData({ExternalRoute(localOnLink, NetworkData::kRoutePreferenceMedium),
                                   ExternalRoute(oldLocalOnLink, NetworkData::kRoutePreferenceMedium),
                                   ExternalRoute(onLinkPrefix, NetworkData::kRoutePreferenceMedium)});

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // Wait for more than 1800 seconds to let the deprecating
    // prefixes expire (keep sending RA from router A).

    for (uint16_t index = 0; index < 185; index++)
    {
        SendRouterAdvert(routerAddressA, {Pio(onLinkPrefix, kValidLitime, kPreferredLifetime)});
        AdvanceTime(10 * 1000);
    }

    VerifyExternalRoutesInNetData({ExternalRoute(onLinkPrefix, NetworkData::kRoutePreferenceMedium)});

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // Disable the instance and re-enable it and restart Routing Manager.

    Log("Disabling and re-enabling OT Instance again");

    testFreeInstance(sInstance);
    InitTest();

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
    // Check that previously saved on-link prefixes are no longer
    // seen in Network Data (indicating that they were removed from
    // `Settings`).

    VerifyExternalRoutesInNetData({ExternalRoute(onLinkPrefix, NetworkData::kRoutePreferenceMedium)});

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

    Log("--------------------------------------------------------------------------------------------");
    Log("TestAutoEnableOfSrpServer");

    InitTest();

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

    Log("End of TestAutoEnableOfSrpServer");
    FinalizeTest();
}
#endif // OPENTHREAD_CONFIG_SRP_SERVER_ENABLE

#endif // OPENTHREAD_CONFIG_BORDER_ROUTING_ENABLE

int main(void)
{
#if OPENTHREAD_CONFIG_BORDER_ROUTING_ENABLE
    TestSamePrefixesFromMultipleRouters();
    TestOmrSelection();
    TestDefaultRoute();
    TestLocalOnLinkPrefixDeprecation();
#if OPENTHREAD_CONFIG_BACKBONE_ROUTER_ENABLE
    TestDomainPrefixAsOmr();
#endif
    TestExtPanIdChange();
    TestConflictingPrefix();
    TestRouterNsProbe();
#if OPENTHREAD_CONFIG_PLATFORM_FLASH_API_ENABLE
    TestSavedOnLinkPrefixes();
#endif
#if OPENTHREAD_CONFIG_SRP_SERVER_ENABLE
    TestAutoEnableOfSrpServer();
#endif

    printf("All tests passed\n");
#else
    printf("BORDER_ROUTING feature is not enabled\n");
#endif

    return 0;
}
