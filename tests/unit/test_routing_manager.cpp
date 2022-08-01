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
#include "common/instance.hpp"
#include "net/icmp6.hpp"
#include "net/nd6.hpp"

#if OPENTHREAD_CONFIG_BORDER_ROUTING_ENABLE

using namespace ot;

// Logs a message and adds current time (sNow) as "<hours>:<min>:<secs>.<msec>"
#define Log(...)                                                                                          \
    printf("%02u:%02u:%02u.%03u " OT_FIRST_ARG(__VA_ARGS__) "\n", (sNow / 36000000), (sNow / 60000) % 60, \
           (sNow / 1000) % 60, sNow % 1000 OT_REST_ARGS(__VA_ARGS__))

static constexpr uint32_t kInfraIfIndex     = 1;
static const char         kInfraIfAddress[] = "fe80::1";

static constexpr uint32_t kValidLitime       = 2000;
static constexpr uint32_t kPreferredLifetime = 1800;

static constexpr uint16_t kMaxRaSize = 800;

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

static Ip6::Address sInfraIfAddress;

bool        sRsEmitted;         // Indicates if an RS message was emitted by BR.
bool        sRaValidated;       // Indicates if an RA was emitted by BR and successfully validated.
bool        sSawExpectedRio;    // Indicates if the emitted RA by BR contained an RIO with `sExpectedRioPrefix`
ExpectedPio sExpectedPio;       // Expected PIO in the emitted RA by BR (MUST be seen in RA to set `sRaValidated`).
Ip6::Prefix sExpectedRioPrefix; // Expected RIO prefix to see in RA (MUST be seen to set `sSawExpectedRio`).

//----------------------------------------------------------------------------------------------------------------------
// Function prototypes

void        ProcessRadioTxAndTasklets(void);
void        AdvanceTime(uint32_t aDuration);
void        LogRouterAdvert(const Icmp6Packet &aPacket);
void        ValidateRouterAdvert(const Icmp6Packet &aPacket);
const char *PreferenceToString(int8_t aPreference);
void        SendRouterAdvert(const Ip6::Address &aAddress, const Icmp6Packet &aPacket);

//----------------------------------------------------------------------------------------------------------------------
// `otPlatRadio

extern "C" {

otError otPlatRadioTransmit(otInstance *, otRadioFrame *)
{
    sRadioTxOngoing = true;

    return OT_ERROR_NONE;
}

otRadioFrame *otPlatRadioGetTransmitBuffer(otInstance *)
{
    return &sRadioTxFrame;
}

//----------------------------------------------------------------------------------------------------------------------
// `otPlatAlaram

void otPlatAlarmMilliStop(otInstance *)
{
    sAlarmOn = false;
}

void otPlatAlarmMilliStartAt(otInstance *, uint32_t aT0, uint32_t aDt)
{
    sAlarmOn   = true;
    sAlarmTime = aT0 + aDt;
}

uint32_t otPlatAlarmMilliGetNow(void)
{
    return sNow;
}

//---------------------------------------------------------------------------------------------------------------------
// otPlatInfraIf

bool otPlatInfraIfHasAddress(uint32_t aInfraIfIndex, const otIp6Address *aAddress)
{
    VerifyOrQuit(aInfraIfIndex == kInfraIfIndex);

    return AsCoreType(aAddress) == sInfraIfAddress;
}

otError otPlatInfraIfSendIcmp6Nd(uint32_t            aInfraIfIndex,
                                 const otIp6Address *aDestAddress,
                                 const uint8_t *     aBuffer,
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
        sRaValidated = true;
        break;

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

    while (sAlarmTime <= time)
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
    Ip6::Nd::RouterAdvertMessage raMsg(aPacket);

    VerifyOrQuit(raMsg.IsValid());

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

            VerifyOrQuit(sExpectedPio != kNoPio, "Received RA contain an unexpected PIO");

            SuccessOrQuit(otBorderRoutingGetOnLinkPrefix(sInstance, &localOnLink));
            VerifyOrQuit(prefix == localOnLink);

            if (sExpectedPio == kPioAdvertisingLocalOnLink)
            {
                VerifyOrQuit(pio.GetPreferredLifetime() > 0, "On link prefix is deprecated unexpectedly");
            }
            else
            {
                VerifyOrQuit(pio.GetPreferredLifetime() == 0, "On link prefix is not deprecated");
            }

            break;
        }

        case Ip6::Nd::Option::kTypeRouteInfo:
        {
            const Ip6::Nd::RouteInfoOption &rio = static_cast<const Ip6::Nd::RouteInfoOption &>(option);
            Ip6::Prefix                     prefix;

            VerifyOrQuit(rio.IsValid());
            rio.GetPrefix(prefix);

            if (prefix == sExpectedRioPrefix)
            {
                sSawExpectedRio = true;
            }

            break;
        }

        default:
            VerifyOrQuit(false, "Unexpected option type in RA msg");
        }
    }
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
                      const Pio *         aPios,
                      uint16_t            aNumPios,
                      const Rio *         aRios,
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
    OnLinkPrefix(const Ip6::Prefix & aPrefix,
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
    RoutePrefix(const Ip6::Prefix & aPrefix,
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
            Log("   on-link prefix:%s, valid:%u, preferred:%u, router:%s",
                AsCoreType(&entry.mPrefix).ToString().AsCString(), entry.mValidLifetime, entry.mPreferredLifetime,
                AsCoreType(&entry.mRouterAddress).ToString().AsCString());

            onLinkPrefixCount++;

            for (const OnLinkPrefix &onLinkPrefix : aOnLinkPrefixes)
            {
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
            Log("   route prefix:%s, valid:%u, prf:%s, router:%s", AsCoreType(&entry.mPrefix).ToString().AsCString(),
                entry.mValidLifetime, PreferenceToString(entry.mRoutePreference),
                AsCoreType(&entry.mRouterAddress).ToString().AsCString());

            routePrefixCount++;

            for (const RoutePrefix &routePrefix : aRoutePrefixes)
            {
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

    VerifyOrQuit(onLinkPrefixCount == kNumOnLinkPrefixes);
    VerifyOrQuit(routePrefixCount == kNumRoutePrefixes);
}

void InitTest(void)
{
    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // Initialize OT instance.

    sNow      = 0;
    sInstance = static_cast<Instance *>(testInitInstance());

    memset(&sRadioTxFrame, 0, sizeof(sRadioTxFrame));
    sRadioTxFrame.mPsdu = sRadioTxFramePsdu;

    SuccessOrQuit(sInfraIfAddress.FromString(kInfraIfAddress));

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // Initialize Border Router and start Thread operation.

    SuccessOrQuit(otBorderRoutingInit(sInstance, kInfraIfIndex, /* aInfraIfIsRunning */ true));

    SuccessOrQuit(otLinkSetPanId(sInstance, 0x1234));
    SuccessOrQuit(otIp6SetEnabled(sInstance, true));
    SuccessOrQuit(otThreadSetEnabled(sInstance, true));

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // Ensure device starts as leader.

    AdvanceTime(10000);

    VerifyOrQuit(otThreadGetDeviceRole(sInstance) == OT_DEVICE_ROLE_LEADER);
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

    sRsEmitted      = false;
    sRaValidated    = false;
    sSawExpectedRio = false;
    sExpectedPio    = kPioAdvertisingLocalOnLink;

    SuccessOrQuit(sInstance->Get<BorderRouter::RoutingManager>().SetEnabled(true));

    SuccessOrQuit(sInstance->Get<BorderRouter::RoutingManager>().GetOnLinkPrefix(localOnLink));
    SuccessOrQuit(sInstance->Get<BorderRouter::RoutingManager>().GetOmrPrefix(localOmr));

    Log("Local on-link prefix is %s", localOnLink.ToString().AsCString());
    Log("Local OMR prefix is %s", localOmr.ToString().AsCString());

    sExpectedRioPrefix = localOmr;

    AdvanceTime(30000);

    VerifyOrQuit(sRsEmitted);
    VerifyOrQuit(sRaValidated);
    VerifyOrQuit(sSawExpectedRio);
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

    sRaValidated    = false;
    sSawExpectedRio = false;
    sExpectedPio    = kPioDeprecatingLocalOnLink;

    AdvanceTime(10000);
    VerifyOrQuit(sRaValidated);
    VerifyOrQuit(sSawExpectedRio);

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

    testFreeInstance(sInstance);
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

    sRsEmitted      = false;
    sRaValidated    = false;
    sSawExpectedRio = false;
    sExpectedPio    = kPioAdvertisingLocalOnLink;

    SuccessOrQuit(sInstance->Get<BorderRouter::RoutingManager>().SetEnabled(true));

    SuccessOrQuit(sInstance->Get<BorderRouter::RoutingManager>().GetOnLinkPrefix(localOnLink));
    SuccessOrQuit(sInstance->Get<BorderRouter::RoutingManager>().GetOmrPrefix(localOmr));

    Log("Local on-link prefix is %s", localOnLink.ToString().AsCString());
    Log("Local OMR prefix is %s", localOmr.ToString().AsCString());

    sExpectedRioPrefix = localOmr;

    AdvanceTime(30000);

    VerifyOrQuit(sRsEmitted);
    VerifyOrQuit(sRaValidated);
    VerifyOrQuit(sSawExpectedRio);
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

    sRaValidated       = false;
    sSawExpectedRio    = false;
    sExpectedPio       = kPioAdvertisingLocalOnLink;
    sExpectedRioPrefix = omrPrefix;

    AdvanceTime(20000);

    VerifyOrQuit(sRaValidated);
    VerifyOrQuit(sSawExpectedRio);

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

    sRaValidated       = false;
    sSawExpectedRio    = false;
    sExpectedRioPrefix = localOmr;

    AdvanceTime(20000);

    VerifyOrQuit(sRaValidated);
    VerifyOrQuit(sSawExpectedRio);

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // Check Network Data. We should see that the local OMR prefix is
    // added again.

    VerifyOmrPrefixInNetData(localOmr);

    VerifyExternalRoutesInNetData({ExternalRoute(localOnLink, NetworkData::kRoutePreferenceMedium)});

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

    Log("End of TestOmrSelection");
    testFreeInstance(sInstance);
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

    sRsEmitted      = false;
    sRaValidated    = false;
    sSawExpectedRio = false;
    sExpectedPio    = kNoPio;

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

    testFreeInstance(sInstance);
}

#endif // OPENTHREAD_CONFIG_BORDER_ROUTING_ENABLE

int main(void)
{
#if OPENTHREAD_CONFIG_BORDER_ROUTING_ENABLE
    TestSamePrefixesFromMultipleRouters();
    TestOmrSelection();
    TestDefaultRoute();
    printf("All tests passed\n");
#else
    printf("BORDER_ROUTING feature is not enabled\n");
#endif

    return 0;
}
