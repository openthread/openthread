/*
 *  Copyright (c) 2020, The OpenThread Authors.
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

/**
 * @file
 *   This file implements CLI commands for Network Data.
 */

#include "cli_network_data.hpp"

#include <openthread/border_router.h>
#include <openthread/netdata_publisher.h>
#include <openthread/server.h>

#include "cli/cli.hpp"
#include "common/encoding.hpp"

namespace ot {
namespace Cli {

NetworkData::NetworkData(otInstance *aInstance, OutputImplementer &aOutputImplementer)
    : Utils(aInstance, aOutputImplementer)
{
#if OPENTHREAD_CONFIG_BORDER_ROUTER_SIGNAL_NETWORK_DATA_FULL
    mFullCallbackWasCalled = false;
    otBorderRouterSetNetDataFullCallback(aInstance, HandleNetdataFull, this);
#endif
}

void NetworkData::PrefixFlagsToString(const otBorderRouterConfig &aConfig, FlagsString &aString)
{
    char *flagsPtr = &aString[0];

    if (aConfig.mPreferred)
    {
        *flagsPtr++ = 'p';
    }

    if (aConfig.mSlaac)
    {
        *flagsPtr++ = 'a';
    }

    if (aConfig.mDhcp)
    {
        *flagsPtr++ = 'd';
    }

    if (aConfig.mConfigure)
    {
        *flagsPtr++ = 'c';
    }

    if (aConfig.mDefaultRoute)
    {
        *flagsPtr++ = 'r';
    }

    if (aConfig.mOnMesh)
    {
        *flagsPtr++ = 'o';
    }

    if (aConfig.mStable)
    {
        *flagsPtr++ = 's';
    }

    if (aConfig.mNdDns)
    {
        *flagsPtr++ = 'n';
    }

    if (aConfig.mDp)
    {
        *flagsPtr++ = 'D';
    }

    *flagsPtr = '\0';
}

void NetworkData::OutputPrefix(const otBorderRouterConfig &aConfig)
{
    FlagsString flagsString;

    OutputIp6Prefix(aConfig.mPrefix);

    PrefixFlagsToString(aConfig, flagsString);

    if (flagsString[0] != '\0')
    {
        OutputFormat(" %s", flagsString);
    }

    OutputLine(" %s %04x", PreferenceToString(aConfig.mPreference), aConfig.mRloc16);
}

void NetworkData::RouteFlagsToString(const otExternalRouteConfig &aConfig, FlagsString &aString)
{
    char *flagsPtr = &aString[0];

    if (aConfig.mStable)
    {
        *flagsPtr++ = 's';
    }

    if (aConfig.mNat64)
    {
        *flagsPtr++ = 'n';
    }

    if (aConfig.mAdvPio)
    {
        *flagsPtr++ = 'a';
    }

    *flagsPtr = '\0';
}

void NetworkData::OutputRoute(const otExternalRouteConfig &aConfig)
{
    FlagsString flagsString;

    OutputIp6Prefix(aConfig.mPrefix);

    RouteFlagsToString(aConfig, flagsString);

    if (flagsString[0] != '\0')
    {
        OutputFormat(" %s", flagsString);
    }

    OutputLine(" %s %04x", PreferenceToString(aConfig.mPreference), aConfig.mRloc16);
}

void NetworkData::OutputService(const otServiceConfig &aConfig)
{
    OutputFormat("%lu ", ToUlong(aConfig.mEnterpriseNumber));
    OutputBytes(aConfig.mServiceData, aConfig.mServiceDataLength);
    OutputFormat(" ");
    OutputBytes(aConfig.mServerConfig.mServerData, aConfig.mServerConfig.mServerDataLength);

    if (aConfig.mServerConfig.mStable)
    {
        OutputFormat(" s");
    }

    OutputLine(" %04x %u", aConfig.mServerConfig.mRloc16, aConfig.mServiceId);
}

/**
 * @cli netdata length
 * @code
 * netdata length
 * 23
 * Done
 * @endcode
 * @par api_copy
 * #otNetDataGetLength
 */
template <> otError NetworkData::Process<Cmd("length")>(Arg aArgs[])
{
    otError error = OT_ERROR_NONE;

    VerifyOrExit(aArgs[0].IsEmpty(), error = OT_ERROR_INVALID_ARGS);
    OutputLine("%u", otNetDataGetLength(GetInstancePtr()));

exit:
    return error;
}

template <> otError NetworkData::Process<Cmd("maxlength")>(Arg aArgs[])
{
    otError error = OT_ERROR_NONE;

    /**
     * @cli netdata maxlength
     * @code
     * netdata maxlength
     * 40
     * Done
     * @endcode
     * @par api_copy
     * #otNetDataGetMaxLength
     */
    if (aArgs[0].IsEmpty())
    {
        OutputLine("%u", otNetDataGetMaxLength(GetInstancePtr()));
    }
    /**
     * @cli netdata maxlength reset
     * @code
     * netdata maxlength reset
     * Done
     * @endcode
     * @par api_copy
     * #otNetDataResetMaxLength
     */
    else if (aArgs[0] == "reset")
    {
        otNetDataResetMaxLength(GetInstancePtr());
    }
    else
    {
        error = OT_ERROR_INVALID_ARGS;
    }

    return error;
}

#if OPENTHREAD_CONFIG_NETDATA_PUBLISHER_ENABLE
template <> otError NetworkData::Process<Cmd("publish")>(Arg aArgs[])
{
    otError error = OT_ERROR_NONE;

#if OPENTHREAD_CONFIG_TMF_NETDATA_SERVICE_ENABLE
    if (aArgs[0] == "dnssrp")
    {
        /**
         * @cli netdata publish dnssrp anycast
         * @code
         * netdata publish dnssrp anycast 1
         * Done
         * @endcode
         * @cparam netdata publish dnssrp anycast @ca{seq-num}
         * @par
         * Publishes a DNS/SRP Service Anycast Address with a sequence number. Any current
         * DNS/SRP Service entry being published from a previous `publish dnssrp{anycast|unicast}`
         * command is removed and replaced with the new arguments.
         * @par
         * `OPENTHREAD_CONFIG_TMF_NETDATA_SERVICE_ENABLE` must be enabled.
         * @csa{netdata publish dnssrp unicast (addr,port)}
         * @csa{netdata publish dnssrp unicast (mle)}
         * @sa otNetDataPublishDnsSrpServiceAnycast
         * @endcli
         */
        if (aArgs[1] == "anycast")
        {
            uint8_t sequenceNumber;

            SuccessOrExit(error = aArgs[2].ParseAsUint8(sequenceNumber));
            otNetDataPublishDnsSrpServiceAnycast(GetInstancePtr(), sequenceNumber);
            ExitNow();
        }

        if (aArgs[1] == "unicast")
        {
            otIp6Address address;
            uint16_t     port;

            /**
             * @cli netdata publish dnssrp unicast (mle)
             * @code
             * netdata publish dnssrp unicast 50152
             * Done
             * @endcode
             * @cparam netdata publish dnssrp unicast @ca{port}
             * @par
             * Publishes the device's Mesh-Local EID with a port number. MLE and port information is
             * included in the Server TLV data. To use a different Unicast address, use the
             * `netdata publish dnssrp unicast (addr,port)` command.
             * @par
             * Any current DNS/SRP Service entry being published from a previous
             * `publish dnssrp{anycast|unicast}` command is removed and replaced with the new arguments.
             * @par
             * `OPENTHREAD_CONFIG_TMF_NETDATA_SERVICE_ENABLE` must be enabled.
             * @csa{netdata publish dnssrp unicast (addr,port)}
             * @csa{netdata publish dnssrp anycast}
             * @sa otNetDataPublishDnsSrpServiceUnicastMeshLocalEid
             */
            if (aArgs[3].IsEmpty())
            {
                SuccessOrExit(error = aArgs[2].ParseAsUint16(port));
                otNetDataPublishDnsSrpServiceUnicastMeshLocalEid(GetInstancePtr(), port);
                ExitNow();
            }

            /**
             * @cli netdata publish dnssrp unicast (addr,port)
             * @code
             * netdata publish dnssrp unicast fd00::1234 51525
             * Done
             * @endcode
             * @cparam netdata publish dnssrp unicast @ca{address} @ca{port}
             * @par
             * Publishes a DNS/SRP Service Unicast Address with an address and port number. The address
             * and port information is included in Service TLV data. Any current DNS/SRP Service entry being
             * published from a previous `publish dnssrp{anycast|unicast}` command is removed and replaced
             * with the new arguments.
             * @par
             * `OPENTHREAD_CONFIG_TMF_NETDATA_SERVICE_ENABLE` must be enabled.
             * @csa{netdata publish dnssrp unicast (mle)}
             * @csa{netdata publish dnssrp anycast}
             * @sa otNetDataPublishDnsSrpServiceUnicast
             */
            SuccessOrExit(error = aArgs[2].ParseAsIp6Address(address));
            SuccessOrExit(error = aArgs[3].ParseAsUint16(port));
            otNetDataPublishDnsSrpServiceUnicast(GetInstancePtr(), &address, port);
            ExitNow();
        }
    }
#endif // OPENTHREAD_CONFIG_TMF_NETDATA_SERVICE_ENABLE

#if OPENTHREAD_CONFIG_BORDER_ROUTER_ENABLE
    /**
     * @cli netdata publish prefix
     * @code
     * netdata publish prefix fd00:1234:5678::/64 paos med
     * Done
     * @endcode
     * @cparam netdata publish prefix @ca{prefix} [@ca{padcrosnD}] [@ca{high}|@ca{med}|@ca{low}]
     * OT CLI uses mapped arguments to configure #otBorderRouterConfig values. @moreinfo{the @overview}.
     * @par
     * Publish an on-mesh prefix entry. @moreinfo{@netdata}.
     * @sa otNetDataPublishOnMeshPrefix
     */
    if (aArgs[0] == "prefix")
    {
        otBorderRouterConfig config;

        SuccessOrExit(error = ParsePrefix(aArgs + 1, config));
        error = otNetDataPublishOnMeshPrefix(GetInstancePtr(), &config);
        ExitNow();
    }

    /**
     * @cli netdata publish route
     * @code
     * netdata publish route fd00:1234:5678::/64 s high
     * Done
     * @endcode
     * @cparam publish route @ca{prefix} [@ca{sn}] [@ca{high}|@ca{med}|@ca{low}]
     * OT CLI uses mapped arguments to configure #otExternalRouteConfig values. @moreinfo{the @overview}.
     * @par
     * Publish an external route entry. @moreinfo{@netdata}.
     * @sa otNetDataPublishExternalRoute
     */
    if (aArgs[0] == "route")
    {
        otExternalRouteConfig config;

        SuccessOrExit(error = ParseRoute(aArgs + 1, config));
        error = otNetDataPublishExternalRoute(GetInstancePtr(), &config);
        ExitNow();
    }

    /**
     * @cli netdata publish replace
     * @code
     * netdata publish replace ::/0 fd00:1234:5678::/64 s high
     * Done
     * @endcode
     * @cparam netdata publish replace @ca{oldprefix} @ca{prefix} [@ca{sn}] [@ca{high}|@ca{med}|@ca{low}]
     * OT CLI uses mapped arguments to configure #otExternalRouteConfig values. @moreinfo{the @overview}.
     * @par
     * Replaces a previously published external route entry. @moreinfo{@netdata}.
     * @sa otNetDataReplacePublishedExternalRoute
     */
    if (aArgs[0] == "replace")
    {
        otIp6Prefix           prefix;
        otExternalRouteConfig config;

        SuccessOrExit(error = aArgs[1].ParseAsIp6Prefix(prefix));
        SuccessOrExit(error = ParseRoute(aArgs + 2, config));
        error = otNetDataReplacePublishedExternalRoute(GetInstancePtr(), &prefix, &config);
        ExitNow();
    }
#endif // OPENTHREAD_CONFIG_BORDER_ROUTER_ENABLE

    error = OT_ERROR_INVALID_ARGS;

exit:
    return error;
}

template <> otError NetworkData::Process<Cmd("unpublish")>(Arg aArgs[])
{
    otError error = OT_ERROR_NONE;

/**
 * @cli netdata unpublish dnssrp
 * @code
 * netdata unpublish dnssrp
 * Done
 * @endcode
 * @par api_copy
 * #otNetDataUnpublishDnsSrpService
 */
#if OPENTHREAD_CONFIG_TMF_NETDATA_SERVICE_ENABLE
    if (aArgs[0] == "dnssrp")
    {
        otNetDataUnpublishDnsSrpService(GetInstancePtr());
        ExitNow();
    }
#endif

/**
 * @cli netdata unpublish (prefix)
 * @code
 * netdata unpublish fd00:1234:5678::/64
 * Done
 * @endcode
 * @cparam netdata unpublish @ca{prefix}
 * @par api_copy
 * #otNetDataUnpublishPrefix
 * @par
 * @moreinfo{@netdata}.
 */
#if OPENTHREAD_CONFIG_BORDER_ROUTER_ENABLE
    {
        otIp6Prefix prefix;

        if (aArgs[0].ParseAsIp6Prefix(prefix) == OT_ERROR_NONE)
        {
            error = otNetDataUnpublishPrefix(GetInstancePtr(), &prefix);
            ExitNow();
        }
    }
#endif

    error = OT_ERROR_INVALID_ARGS;

exit:
    return error;
}
#endif // OPENTHREAD_CONFIG_NETDATA_PUBLISHER_ENABLE

#if OPENTHREAD_CONFIG_BORDER_ROUTER_ENABLE || OPENTHREAD_CONFIG_TMF_NETDATA_SERVICE_ENABLE
/**
 * @cli netdata register
 * @code
 * netdata register
 * Done
 * @endcode
 * @par
 * Register configured prefixes, routes, and services with the Leader.
 * @par
 * OT CLI checks for `OPENTHREAD_CONFIG_BORDER_ROUTER_ENABLE`. If OTBR is enabled, it
 * registers local Network Data with the Leader. Otherwise, it calls the CLI function `otServerRegister`.
 * @moreinfo{@netdata}.
 * @csa{prefix add}
 * @sa otBorderRouterRegister
 * @sa otServerAddService
 */
template <> otError NetworkData::Process<Cmd("register")>(Arg aArgs[])
{
    OT_UNUSED_VARIABLE(aArgs);

    otError error = OT_ERROR_NONE;

#if OPENTHREAD_CONFIG_BORDER_ROUTER_ENABLE
    SuccessOrExit(error = otBorderRouterRegister(GetInstancePtr()));
#else
    SuccessOrExit(error = otServerRegister(GetInstancePtr()));
#endif

exit:
    return error;
}
#endif

template <> otError NetworkData::Process<Cmd("steeringdata")>(Arg aArgs[])
{
    otError           error;
    otExtAddress      addr;
    otJoinerDiscerner discerner;

    VerifyOrExit(aArgs[0] == "check", error = OT_ERROR_INVALID_ARGS);

    error = ParseJoinerDiscerner(aArgs[1], discerner);

    if (error == OT_ERROR_NOT_FOUND)
    {
        discerner.mLength = 0;
        error             = aArgs[1].ParseAsHexString(addr.m8);
    }

    SuccessOrExit(error);

    /**
     * @cli netdata steeringdata check (discerner)
     * @code
     * netdata steeringdata check 0xabc/12
     * Done
     * @endcode
     * @code
     * netdata steeringdata check 0xdef/12
     * Error 23: NotFound
     * @endcode
     * @cparam netdata steeringdata check @ca{discerner}
     * *   `discerner`: The %Joiner discerner in format `{number}/{length}`.
     * @par api_copy
     * #otNetDataSteeringDataCheckJoinerWithDiscerner
     * @csa{joiner discerner}
     */
    if (discerner.mLength)
    {
        error = otNetDataSteeringDataCheckJoinerWithDiscerner(GetInstancePtr(), &discerner);
    }
    /**
     * @cli netdata steeringdata check (eui64)
     * @code
     * netdata steeringdata check d45e64fa83f81cf7
     * Done
     * @endcode
     * @cparam netdata steeringdata check @ca{eui64}
     * *   `eui64`: The IEEE EUI-64 of the %Joiner.
     * @par api_copy
     * #otNetDataSteeringDataCheckJoiner
     * @csa{eui64}
     */
    else
    {
        error = otNetDataSteeringDataCheckJoiner(GetInstancePtr(), &addr);
    }

exit:
    return error;
}

otError NetworkData::GetNextPrefix(otNetworkDataIterator *aIterator, otBorderRouterConfig *aConfig, bool aLocal)
{
    otError error;

    if (aLocal)
    {
#if OPENTHREAD_CONFIG_BORDER_ROUTER_ENABLE
        error = otBorderRouterGetNextOnMeshPrefix(GetInstancePtr(), aIterator, aConfig);
#else
        error = OT_ERROR_NOT_FOUND;
#endif
    }
    else
    {
        error = otNetDataGetNextOnMeshPrefix(GetInstancePtr(), aIterator, aConfig);
    }

    return error;
}

void NetworkData::OutputNetworkData(bool aLocal, uint16_t aRloc16)
{
    otNetworkDataIterator  iterator = OT_NETWORK_DATA_ITERATOR_INIT;
    otBorderRouterConfig   prefix;
    otExternalRouteConfig  route;
    otServiceConfig        service;
    otLowpanContextInfo    context;
    otCommissioningDataset dataset;

    OutputLine("Prefixes:");

    while (GetNextPrefix(&iterator, &prefix, aLocal) == OT_ERROR_NONE)
    {
        if ((aRloc16 == kAnyRloc16) || (aRloc16 == prefix.mRloc16))
        {
            OutputPrefix(prefix);
        }
    }

    OutputLine("Routes:");
    iterator = OT_NETWORK_DATA_ITERATOR_INIT;

    while (GetNextRoute(&iterator, &route, aLocal) == OT_ERROR_NONE)
    {
        if ((aRloc16 == kAnyRloc16) || (aRloc16 == route.mRloc16))
        {
            OutputRoute(route);
        }
    }

    OutputLine("Services:");
    iterator = OT_NETWORK_DATA_ITERATOR_INIT;

    while (GetNextService(&iterator, &service, aLocal) == OT_ERROR_NONE)
    {
        if ((aRloc16 == kAnyRloc16) || (aRloc16 == service.mServerConfig.mRloc16))
        {
            OutputService(service);
        }
    }

    VerifyOrExit(!aLocal);
    VerifyOrExit(aRloc16 == kAnyRloc16);

    OutputLine("Contexts:");
    iterator = OT_NETWORK_DATA_ITERATOR_INIT;

    while (otNetDataGetNextLowpanContextInfo(GetInstancePtr(), &iterator, &context) == OT_ERROR_NONE)
    {
        OutputIp6Prefix(context.mPrefix);
        OutputLine(" %u %c", context.mContextId, context.mCompressFlag ? 'c' : '-');
    }

    otNetDataGetCommissioningDataset(GetInstancePtr(), &dataset);

    OutputLine("Commissioning:");

    dataset.mIsSessionIdSet ? OutputFormat("%u ", dataset.mSessionId) : OutputFormat("- ");
    dataset.mIsLocatorSet ? OutputFormat("%04x ", dataset.mLocator) : OutputFormat("- ");
    dataset.mIsJoinerUdpPortSet ? OutputFormat("%u ", dataset.mJoinerUdpPort) : OutputFormat("- ");
    dataset.mIsSteeringDataSet ? OutputBytes(dataset.mSteeringData.m8, dataset.mSteeringData.mLength)
                               : OutputFormat("-");

    if (dataset.mHasExtraTlv)
    {
        OutputFormat(" e");
    }

    OutputNewLine();

exit:
    return;
}

otError NetworkData::GetNextRoute(otNetworkDataIterator *aIterator, otExternalRouteConfig *aConfig, bool aLocal)
{
    otError error;

    if (aLocal)
    {
#if OPENTHREAD_CONFIG_BORDER_ROUTER_ENABLE
        error = otBorderRouterGetNextRoute(GetInstancePtr(), aIterator, aConfig);
#else
        error = OT_ERROR_NOT_FOUND;
#endif
    }
    else
    {
        error = otNetDataGetNextRoute(GetInstancePtr(), aIterator, aConfig);
    }

    return error;
}

otError NetworkData::GetNextService(otNetworkDataIterator *aIterator, otServiceConfig *aConfig, bool aLocal)
{
    otError error;

    if (aLocal)
    {
#if OPENTHREAD_CONFIG_TMF_NETDATA_SERVICE_ENABLE
        error = otServerGetNextService(GetInstancePtr(), aIterator, aConfig);
#else
        error = OT_ERROR_NOT_FOUND;
#endif
    }
    else
    {
        error = otNetDataGetNextService(GetInstancePtr(), aIterator, aConfig);
    }

    return error;
}

otError NetworkData::OutputBinary(bool aLocal)
{
    otError error;
    uint8_t data[255];
    uint8_t len = sizeof(data);

    if (aLocal)
    {
#if OPENTHREAD_CONFIG_BORDER_ROUTER_ENABLE
        error = otBorderRouterGetNetData(GetInstancePtr(), false, data, &len);
#else
        error = OT_ERROR_NOT_IMPLEMENTED;
#endif
    }
    else
    {
        error = otNetDataGet(GetInstancePtr(), false, data, &len);
    }
    SuccessOrExit(error);

    OutputBytesLine(data, static_cast<uint8_t>(len));

exit:
    return error;
}

/**
 * @cli netdata show
 * @code
 * netdata show
 * Prefixes:
 * fd00:dead:beef:cafe::/64 paros med dc00
 * Routes:
 * fd49:7770:7fc5:0::/64 s med 4000
 * Services:
 * 44970 5d c000 s 4000 0
 * 44970 01 9a04b000000e10 s 4000 1
 * Contexts:
 * fd00:dead:beef:cafe::/64 1 c
 * Commissioning:
 * 1248 dc00 9988 00000000000120000000000000000000 e
 * Done
 * @endcode
 * @code
 * netdata show -x
 * 08040b02174703140040fd00deadbeefcafe0504dc00330007021140
 * Done
 * @endcode
 * @code
 * netdata show 0xdc00
 * Prefixes:
 * fd00:dead:beef:cafe::/64 paros med dc00
 * Routes:
 * Services:
 * Done
 * @cparam netdata show [@ca{-x}|@ca{rloc16}]
 * *   The optional `-x` argument gets Network Data as hex-encoded TLVs.
 * *   The optional `rloc16` argument gets all prefix/route/service entries associated with a given RLOC16.
 * @par
 * `netdata show` from OT CLI gets full Network Data received from the Leader. This command uses several
 * API functions to combine prefixes, routes, and services, including #otNetDataGetNextOnMeshPrefix,
 * #otNetDataGetNextRoute, #otNetDataGetNextService and #otNetDataGetNextLowpanContextInfo.
 * @par
 * On-mesh prefixes are listed under `Prefixes` header:
 * * The on-mesh prefix
 * * Flags
 *   * p: Preferred flag
 *   * a: Stateless IPv6 Address Autoconfiguration flag
 *   * d: DHCPv6 IPv6 Address Configuration flag
 *   * c: DHCPv6 Other Configuration flag
 *   * r: Default Route flag
 *   * o: On Mesh flag
 *   * s: Stable flag
 *   * n: Nd Dns flag
 *   * D: Domain Prefix flag (only available for Thread 1.2).
 * * Preference `high`, `med`, or `low`
 * * RLOC16 of device which added the on-mesh prefix
 * @par
 * External Routes are listed under `Routes` header:
 * * The route prefix
 * * Flags
 *   * s: Stable flag
 *   * n: NAT64 flag
 * * Preference `high`, `med`, or `low`
 * * RLOC16 of device which added the route prefix
 * @par
 * Service entries are listed under `Services` header:
 * * Enterprise number
 * * Service data (as hex bytes)
 * * Server data (as hex bytes)
 * * Flags
 *   * s: Stable flag
 * * RLOC16 of devices which added the service entry
 * * Service ID
 * @par
 * 6LoWPAN Context IDs are listed under `Contexts` header:
 * * The prefix
 * * Context ID
 * * Compress flag (`c` if marked or `-` otherwise).
 * @par
 * Commissioning Dataset information is printed under `Commissioning` header:
 * * Session ID if present in Dataset or `-` otherwise
 * * Border Agent RLOC16 (in hex) if present in Dataset or `-` otherwise
 * * Joiner UDP port number if present in Dataset or `-` otherwise
 * * Steering Data (as hex bytes) if present in Dataset or `-` otherwise
 * * Flags:
 *   * e: If Dataset contains any extra unknown TLV
 * @par
 * @moreinfo{@netdata}.
 * @csa{br omrprefix}
 * @csa{br onlinkprefix}
 * @sa otBorderRouterGetNetData
 */
template <> otError NetworkData::Process<Cmd("show")>(Arg aArgs[])
{
    otError  error  = OT_ERROR_INVALID_ARGS;
    uint16_t rloc16 = kAnyRloc16;
    bool     local  = false;
    bool     binary = false;

    for (uint8_t i = 0; !aArgs[i].IsEmpty(); i++)
    {
        /**
         * @cli netdata show local
         * @code
         * netdata show local
         * Prefixes:
         * fd00:dead:beef:cafe::/64 paros med dc00
         * Routes:
         * Services:
         * Done
         * @endcode
         * @code
         * netdata show local -x
         * 08040b02174703140040fd00deadbeefcafe0504dc00330007021140
         * Done
         * @endcode
         * @cparam netdata show local [@ca{-x}]
         * *   The optional `-x` argument gets local Network Data as hex-encoded TLVs.
         * @par
         * Print local Network Data to sync with the Leader.
         * @csa{netdata show}
         */
        if (aArgs[i] == "local")
        {
            local = true;
        }
        else if (aArgs[i] == "-x")
        {
            binary = true;
        }
        else
        {
            SuccessOrExit(error = aArgs[i].ParseAsUint16(rloc16));
        }
    }

    if (local || binary)
    {
        VerifyOrExit(rloc16 == kAnyRloc16, error = OT_ERROR_INVALID_ARGS);
    }

    if (binary)
    {
        error = OutputBinary(local);
    }
    else
    {
        OutputNetworkData(local, rloc16);
        error = OT_ERROR_NONE;
    }

exit:
    return error;
}

#if OPENTHREAD_CONFIG_BORDER_ROUTER_SIGNAL_NETWORK_DATA_FULL
template <> otError NetworkData::Process<Cmd("full")>(Arg aArgs[])
{
    otError error = OT_ERROR_NONE;

    /**
     * @cli netdata full
     * @code
     * netdata full
     * no
     * Done
     * @endcode
     * @par
     * Print "yes" or "no" indicating whether or not the "net data full" callback has been invoked since start of
     * Thread operation or since the last time `netdata full reset` was used to reset the flag.
     * This command requires `OPENTHREAD_CONFIG_BORDER_ROUTER_SIGNAL_NETWORK_DATA_FULL`.
     * The "net data full" callback is invoked whenever:
     * - The device is acting as a leader and receives a Network Data registration from a Border Router (BR) that it
     *   cannot add to Network Data (running out of space).
     * - The device is acting as a BR and new entries cannot be added to its local Network Data.
     * - The device is acting as a BR and tries to register its local Network Data entries with the leader, but
     *   determines that its local entries will not fit.
     * @sa otBorderRouterSetNetDataFullCallback
     */
    if (aArgs[0].IsEmpty())
    {
        OutputLine(mFullCallbackWasCalled ? "yes" : "no");
    }
    /**
     * @cli netdata full reset
     * @code
     * netdata full reset
     * Done
     * @endcode
     * @par
     * Reset the flag tracking whether "net data full" callback was invoked.
     */
    else if (aArgs[0] == "reset")
    {
        VerifyOrExit(aArgs[1].IsEmpty(), error = OT_ERROR_INVALID_ARGS);
        mFullCallbackWasCalled = false;
    }
    else
    {
        error = OT_ERROR_INVALID_ARGS;
    }

exit:
    return error;
}
#endif // OPENTHREAD_CONFIG_BORDER_ROUTER_SIGNAL_NETWORK_DATA_FULL

otError NetworkData::Process(Arg aArgs[])
{
#define CmdEntry(aCommandString)                                   \
    {                                                              \
        aCommandString, &NetworkData::Process<Cmd(aCommandString)> \
    }

    static constexpr Command kCommands[] = {
#if OPENTHREAD_CONFIG_BORDER_ROUTER_SIGNAL_NETWORK_DATA_FULL
        CmdEntry("full"),
#endif
        CmdEntry("length"),
        CmdEntry("maxlength"),
#if OPENTHREAD_CONFIG_NETDATA_PUBLISHER_ENABLE
        CmdEntry("publish"),
#endif
#if OPENTHREAD_CONFIG_BORDER_ROUTER_ENABLE || OPENTHREAD_CONFIG_TMF_NETDATA_SERVICE_ENABLE
        CmdEntry("register"),
#endif
        CmdEntry("show"),
        CmdEntry("steeringdata"),
#if OPENTHREAD_CONFIG_NETDATA_PUBLISHER_ENABLE
        CmdEntry("unpublish"),
#endif
    };

#undef CmdEntry

    static_assert(BinarySearch::IsSorted(kCommands), "kCommands is not sorted");

    otError        error = OT_ERROR_INVALID_COMMAND;
    const Command *command;

    /**
     * @cli netdata help
     * @code
     * netdata help
     * length
     * maxlength
     * publish
     * register
     * show
     * steeringdata
     * unpublish
     * Done
     * @endcode
     * @par
     * Gets a list of `netdata` CLI commands.
     * @sa @netdata
     */
    if (aArgs[0].IsEmpty() || (aArgs[0] == "help"))
    {
        OutputCommandTable(kCommands);
        ExitNow(error = aArgs[0].IsEmpty() ? error : OT_ERROR_NONE);
    }

    command = BinarySearch::Find(aArgs[0].GetCString(), kCommands);
    VerifyOrExit(command != nullptr);

    error = (this->*command->mHandler)(aArgs + 1);

exit:
    return error;
}

} // namespace Cli
} // namespace ot
