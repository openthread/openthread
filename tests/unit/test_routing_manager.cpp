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

static otInstance *sInstance;

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
const char *PreferenceToString(uint8_t aPreference);
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

const char *PreferenceToString(uint8_t aPreference)
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

void TestRoutingManager(void)
{
    Instance &                                        instance = *static_cast<Instance *>(testInitInstance());
    BorderRouter::RoutingManager &                    rm       = instance.Get<BorderRouter::RoutingManager>();
    Ip6::Prefix                                       localOnLink;
    Ip6::Prefix                                       localOmr;
    Ip6::Prefix                                       onLinkPrefix   = PrefixFromString("2000:abba:baba::", 64);
    Ip6::Prefix                                       routePrefix    = PrefixFromString("2000:1234:5678::", 64);
    Ip6::Prefix                                       omrPrefix      = PrefixFromString("2000:0000:1111:4444::", 64);
    Ip6::Address                                      routerAddressA = AddressFromString("fd00::aaaa");
    Ip6::Address                                      routerAddressB = AddressFromString("fd00::bbbb");
    BorderRouter::RoutingManager::PrefixTableIterator iter;
    BorderRouter::RoutingManager::PrefixTableEntry    entry;
    NetworkData::Iterator                             iterator;
    NetworkData::OnMeshPrefixConfig                   prefixConfig;
    NetworkData::ExternalRouteConfig                  routeConfig;
    uint8_t                                           counter;
    uint8_t                                           buffer[800];

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // Initialize OT instance.

    sNow      = 0;
    sInstance = &instance;

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

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // Start Routing Manager. Check emitted RS and RA messages.

    sRsEmitted      = false;
    sRaValidated    = false;
    sSawExpectedRio = false;
    sExpectedPio    = kPioAdvertisingLocalOnLink;

    Log("Starting RoutingManager");

    SuccessOrQuit(rm.SetEnabled(true));

    SuccessOrQuit(rm.GetOnLinkPrefix(localOnLink));
    SuccessOrQuit(rm.GetOmrPrefix(localOmr));

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

    iterator = NetworkData::kIteratorInit;

    // We expect to see OMR prefix in net data as on-mesh prefix.
    SuccessOrQuit(instance.Get<NetworkData::Leader>().GetNextOnMeshPrefix(iterator, prefixConfig));
    VerifyOrQuit(prefixConfig.GetPrefix() == localOmr);
    VerifyOrQuit(prefixConfig.mStable == true);
    VerifyOrQuit(prefixConfig.mSlaac == true);
    VerifyOrQuit(prefixConfig.mPreferred == true);
    VerifyOrQuit(prefixConfig.mOnMesh == true);
    VerifyOrQuit(prefixConfig.mDefaultRoute == false);

    VerifyOrQuit(instance.Get<NetworkData::Leader>().GetNextOnMeshPrefix(iterator, prefixConfig) == kErrorNotFound);

    iterator = NetworkData::kIteratorInit;

    // We expect to see local on-link prefix in net data as external route.
    SuccessOrQuit(instance.Get<NetworkData::Leader>().GetNextExternalRoute(iterator, routeConfig));
    VerifyOrQuit(routeConfig.GetPrefix() == localOnLink);
    VerifyOrQuit(routeConfig.mStable == true);
    VerifyOrQuit(routeConfig.mPreference == NetworkData::kRoutePreferenceMedium);

    VerifyOrQuit(instance.Get<NetworkData::Leader>().GetNextExternalRoute(iterator, routeConfig) == kErrorNotFound);

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // Send an RA from router A with a new on-link (PIO) and route prefix (RIO).

    {
        Ip6::Nd::RouterAdvertMessage raMsg(Ip6::Nd::RouterAdvertMessage::Header(), buffer);

        SuccessOrQuit(raMsg.AppendPrefixInfoOption(onLinkPrefix, kValidLitime, kPreferredLifetime));
        SuccessOrQuit(raMsg.AppendRouteInfoOption(routePrefix, kValidLitime, NetworkData::kRoutePreferenceMedium));

        SendRouterAdvert(routerAddressA, raMsg.GetAsPacket());

        Log("Send RA from router A");
        LogRouterAdvert(raMsg.GetAsPacket());
    }

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

    counter = 0;

    rm.InitPrefixTableIterator(iter);

    while (rm.GetNextPrefixTableEntry(iter, entry) == kErrorNone)
    {
        counter++;
        VerifyOrQuit(AsCoreType(&entry.mRouterAddress) == routerAddressA);

        if (entry.mIsOnLink)
        {
            VerifyOrQuit(AsCoreType(&entry.mPrefix) == onLinkPrefix);
            VerifyOrQuit(entry.mValidLifetime = kValidLitime);
            VerifyOrQuit(entry.mPreferredLifetime = kPreferredLifetime);
        }
        else
        {
            VerifyOrQuit(AsCoreType(&entry.mPrefix) == routePrefix);
            VerifyOrQuit(entry.mValidLifetime = kValidLitime);
            VerifyOrQuit(static_cast<int8_t>(entry.mRoutePreference) == NetworkData::kRoutePreferenceMedium);
        }
    }

    VerifyOrQuit(counter == 2);

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // Check Network Data to include new prefixes from router A.

    iterator = NetworkData::kIteratorInit;

    // We expect to see OMR prefix in net data as on-mesh prefix.
    SuccessOrQuit(instance.Get<NetworkData::Leader>().GetNextOnMeshPrefix(iterator, prefixConfig));
    VerifyOrQuit(prefixConfig.GetPrefix() == localOmr);
    VerifyOrQuit(prefixConfig.mStable == true);
    VerifyOrQuit(prefixConfig.mSlaac == true);
    VerifyOrQuit(prefixConfig.mPreferred == true);
    VerifyOrQuit(prefixConfig.mOnMesh == true);
    VerifyOrQuit(prefixConfig.mDefaultRoute == false);

    VerifyOrQuit(instance.Get<NetworkData::Leader>().GetNextOnMeshPrefix(iterator, prefixConfig) == kErrorNotFound);

    iterator = NetworkData::kIteratorInit;

    counter = 0;

    // We expect to see 3 entries, our local on link and new prefixes from router A.
    while (instance.Get<NetworkData::Leader>().GetNextExternalRoute(iterator, routeConfig) == kErrorNone)
    {
        VerifyOrQuit((routeConfig.GetPrefix() == localOnLink) || (routeConfig.GetPrefix() == onLinkPrefix) ||
                     (routeConfig.GetPrefix() == routePrefix));
        VerifyOrQuit(static_cast<int8_t>(routeConfig.mPreference) == NetworkData::kRoutePreferenceMedium);
        counter++;
    }

    VerifyOrQuit(counter == 3);

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // Send an RA from router B with same route prefix (RIO) but with
    // high route preference.

    {
        Ip6::Nd::RouterAdvertMessage raMsg(Ip6::Nd::RouterAdvertMessage::Header(), buffer);

        SuccessOrQuit(raMsg.AppendRouteInfoOption(routePrefix, kValidLitime, NetworkData::kRoutePreferenceHigh));

        SendRouterAdvert(routerAddressB, raMsg.GetAsPacket());

        Log("Send RA from router B");
        LogRouterAdvert(raMsg.GetAsPacket());
    }

    AdvanceTime(10000);

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // Check the discovered prefix table and ensure info from router B
    // is also included in the table.

    counter = 0;

    rm.InitPrefixTableIterator(iter);

    while (rm.GetNextPrefixTableEntry(iter, entry) == kErrorNone)
    {
        const Ip6::Address &routerAddr = AsCoreType(&entry.mRouterAddress);
        counter++;

        if (routerAddr == routerAddressA)
        {
            if (entry.mIsOnLink)
            {
                VerifyOrQuit(AsCoreType(&entry.mPrefix) == onLinkPrefix);
                VerifyOrQuit(entry.mValidLifetime = kValidLitime);
                VerifyOrQuit(entry.mPreferredLifetime = kPreferredLifetime);
            }
            else
            {
                VerifyOrQuit(AsCoreType(&entry.mPrefix) == routePrefix);
                VerifyOrQuit(entry.mValidLifetime = kValidLitime);
                VerifyOrQuit(static_cast<int8_t>(entry.mRoutePreference) == NetworkData::kRoutePreferenceMedium);
            }
        }
        else if (routerAddr == routerAddressB)
        {
            VerifyOrQuit(!entry.mIsOnLink);
            VerifyOrQuit(AsCoreType(&entry.mPrefix) == routePrefix);
            VerifyOrQuit(entry.mValidLifetime = kValidLitime);
            VerifyOrQuit(static_cast<int8_t>(entry.mRoutePreference) == NetworkData::kRoutePreferenceHigh);
        }
        else
        {
            VerifyOrQuit(false, "Unexpected entry in prefix table with unknown router address");
        }
    }

    VerifyOrQuit(counter == 3);

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // Check Network Data.

    iterator = NetworkData::kIteratorInit;

    // We expect to see OMR prefix in net data as on-mesh prefix
    SuccessOrQuit(instance.Get<NetworkData::Leader>().GetNextOnMeshPrefix(iterator, prefixConfig));
    VerifyOrQuit(prefixConfig.GetPrefix() == localOmr);
    VerifyOrQuit(prefixConfig.mStable == true);
    VerifyOrQuit(prefixConfig.mSlaac == true);
    VerifyOrQuit(prefixConfig.mPreferred == true);
    VerifyOrQuit(prefixConfig.mOnMesh == true);
    VerifyOrQuit(prefixConfig.mDefaultRoute == false);

    VerifyOrQuit(instance.Get<NetworkData::Leader>().GetNextOnMeshPrefix(iterator, prefixConfig) == kErrorNotFound);

    iterator = NetworkData::kIteratorInit;

    counter = 0;

    // We expect to see 3 entries, our local on link and new prefixes
    // from router A and B. The `routePrefix` now should have high
    // preference.

    while (instance.Get<NetworkData::Leader>().GetNextExternalRoute(iterator, routeConfig) == kErrorNone)
    {
        counter++;

        if (routeConfig.GetPrefix() == routePrefix)
        {
            VerifyOrQuit(static_cast<int8_t>(routeConfig.mPreference) == NetworkData::kRoutePreferenceHigh);
        }
        else
        {
            VerifyOrQuit((routeConfig.GetPrefix() == localOnLink) || (routeConfig.GetPrefix() == onLinkPrefix));
            VerifyOrQuit(static_cast<int8_t>(routeConfig.mPreference) == NetworkData::kRoutePreferenceMedium);
        }
    }

    VerifyOrQuit(counter == 3);

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // Send an RA from router B removing the route prefix.

    {
        Ip6::Nd::RouterAdvertMessage raMsg(Ip6::Nd::RouterAdvertMessage::Header(), buffer);

        SuccessOrQuit(raMsg.AppendRouteInfoOption(routePrefix, 0, NetworkData::kRoutePreferenceHigh));

        SendRouterAdvert(routerAddressB, raMsg.GetAsPacket());

        Log("Send RA from router B");
        LogRouterAdvert(raMsg.GetAsPacket());
    }

    AdvanceTime(10000);

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // Check the discovered prefix table and ensure info from router B
    // is now removed from the table.

    counter = 0;

    rm.InitPrefixTableIterator(iter);

    while (rm.GetNextPrefixTableEntry(iter, entry) == kErrorNone)
    {
        counter++;

        VerifyOrQuit(AsCoreType(&entry.mRouterAddress) == routerAddressA);

        if (entry.mIsOnLink)
        {
            VerifyOrQuit(AsCoreType(&entry.mPrefix) == onLinkPrefix);
            VerifyOrQuit(entry.mValidLifetime = kValidLitime);
            VerifyOrQuit(entry.mPreferredLifetime = kPreferredLifetime);
        }
        else
        {
            VerifyOrQuit(AsCoreType(&entry.mPrefix) == routePrefix);
            VerifyOrQuit(entry.mValidLifetime = kValidLitime);
            VerifyOrQuit(static_cast<int8_t>(entry.mRoutePreference) == NetworkData::kRoutePreferenceMedium);
        }
    }

    VerifyOrQuit(counter == 2);

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // Check Network Data, all prefixes should be again at medium preference.

    counter  = 0;
    iterator = NetworkData::kIteratorInit;

    while (instance.Get<NetworkData::Leader>().GetNextExternalRoute(iterator, routeConfig) == kErrorNone)
    {
        counter++;

        VerifyOrQuit((routeConfig.GetPrefix() == localOnLink) || (routeConfig.GetPrefix() == onLinkPrefix) ||
                     (routeConfig.GetPrefix() == routePrefix));
        VerifyOrQuit(static_cast<int8_t>(routeConfig.mPreference) == NetworkData::kRoutePreferenceMedium);
    }

    VerifyOrQuit(counter == 3);

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
    sExpectedRioPrefix = omrPrefix;

    AdvanceTime(20000);

    VerifyOrQuit(sRaValidated);
    VerifyOrQuit(sSawExpectedRio);

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // Check Network Data. We should now see that the local OMR prefix
    // is removed.

    iterator = NetworkData::kIteratorInit;

    // We expect to see new OMR prefix in net data.
    SuccessOrQuit(instance.Get<NetworkData::Leader>().GetNextOnMeshPrefix(iterator, prefixConfig));
    VerifyOrQuit(prefixConfig.GetPrefix() == omrPrefix);
    VerifyOrQuit(prefixConfig.mStable == true);
    VerifyOrQuit(prefixConfig.mSlaac == true);
    VerifyOrQuit(prefixConfig.mPreferred == true);
    VerifyOrQuit(prefixConfig.mOnMesh == true);
    VerifyOrQuit(prefixConfig.mDefaultRoute == false);

    VerifyOrQuit(instance.Get<NetworkData::Leader>().GetNextOnMeshPrefix(iterator, prefixConfig) == kErrorNotFound);

    counter  = 0;
    iterator = NetworkData::kIteratorInit;

    while (instance.Get<NetworkData::Leader>().GetNextExternalRoute(iterator, routeConfig) == kErrorNone)
    {
        counter++;

        VerifyOrQuit((routeConfig.GetPrefix() == localOnLink) || (routeConfig.GetPrefix() == onLinkPrefix) ||
                     (routeConfig.GetPrefix() == routePrefix));
        VerifyOrQuit(static_cast<int8_t>(routeConfig.mPreference) == NetworkData::kRoutePreferenceMedium);
    }

    VerifyOrQuit(counter == 3);

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

    iterator = NetworkData::kIteratorInit;

    // We expect to see new OMR prefix in net data as on-mesh prefix.
    SuccessOrQuit(instance.Get<NetworkData::Leader>().GetNextOnMeshPrefix(iterator, prefixConfig));
    VerifyOrQuit(prefixConfig.GetPrefix() == localOmr);
    VerifyOrQuit(prefixConfig.mStable == true);
    VerifyOrQuit(prefixConfig.mSlaac == true);
    VerifyOrQuit(prefixConfig.mPreferred == true);
    VerifyOrQuit(prefixConfig.mOnMesh == true);
    VerifyOrQuit(prefixConfig.mDefaultRoute == false);

    VerifyOrQuit(instance.Get<NetworkData::Leader>().GetNextOnMeshPrefix(iterator, prefixConfig) == kErrorNotFound);

    counter  = 0;
    iterator = NetworkData::kIteratorInit;

    while (instance.Get<NetworkData::Leader>().GetNextExternalRoute(iterator, routeConfig) == kErrorNone)
    {
        counter++;

        VerifyOrQuit((routeConfig.GetPrefix() == localOnLink) || (routeConfig.GetPrefix() == onLinkPrefix) ||
                     (routeConfig.GetPrefix() == routePrefix));
        VerifyOrQuit(static_cast<int8_t>(routeConfig.mPreference) == NetworkData::kRoutePreferenceMedium);
    }

    VerifyOrQuit(counter == 3);

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

    Log("End of test");

    testFreeInstance(&instance);
}

#endif // OPENTHREAD_CONFIG_BORDER_ROUTING_ENABLE

int main(void)
{
#if OPENTHREAD_CONFIG_BORDER_ROUTING_ENABLE
    TestRoutingManager();
    printf("All tests passed\n");
#else
    printf("BORDER_ROUTING feature is not enabled\n");
#endif

    return 0;
}
