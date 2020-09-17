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
#include <openthread/server.h>

#include "cli/cli.hpp"
#include "common/encoding.hpp"

using ot::Encoding::BigEndian::HostSwap16;

namespace ot {
namespace Cli {

const struct NetworkData::Command NetworkData::sCommands[] = {
    {"help", &NetworkData::ProcessHelp},
#if OPENTHREAD_CONFIG_BORDER_ROUTER_ENABLE || OPENTHREAD_CONFIG_TMF_NETDATA_SERVICE_ENABLE
    {"register", &NetworkData::ProcessRegister},
#endif
    {"show", &NetworkData::ProcessShow},
    {"steeringdata", &NetworkData::ProcessSteeringData},
};

NetworkData::NetworkData(Interpreter &aInterpreter)
    : mInterpreter(aInterpreter)
{
}

void NetworkData::OutputPrefix(const otBorderRouterConfig &aConfig)
{
    mInterpreter.OutputFormat("%x:%x:%x:%x::/%d ", HostSwap16(aConfig.mPrefix.mPrefix.mFields.m16[0]),
                              HostSwap16(aConfig.mPrefix.mPrefix.mFields.m16[1]),
                              HostSwap16(aConfig.mPrefix.mPrefix.mFields.m16[2]),
                              HostSwap16(aConfig.mPrefix.mPrefix.mFields.m16[3]), aConfig.mPrefix.mLength);

    if (aConfig.mPreferred)
    {
        mInterpreter.OutputFormat("p");
    }

    if (aConfig.mSlaac)
    {
        mInterpreter.OutputFormat("a");
    }

    if (aConfig.mDhcp)
    {
        mInterpreter.OutputFormat("d");
    }

    if (aConfig.mConfigure)
    {
        mInterpreter.OutputFormat("c");
    }

    if (aConfig.mDefaultRoute)
    {
        mInterpreter.OutputFormat("r");
    }

    if (aConfig.mOnMesh)
    {
        mInterpreter.OutputFormat("o");
    }

    if (aConfig.mStable)
    {
        mInterpreter.OutputFormat("s");
    }

    if (aConfig.mNdDns)
    {
        mInterpreter.OutputFormat("n");
    }

    if (aConfig.mDp)
    {
        mInterpreter.OutputFormat("D");
    }

    switch (aConfig.mPreference)
    {
    case OT_ROUTE_PREFERENCE_LOW:
        mInterpreter.OutputFormat(" low");
        break;

    case OT_ROUTE_PREFERENCE_MED:
        mInterpreter.OutputFormat(" med");
        break;

    case OT_ROUTE_PREFERENCE_HIGH:
        mInterpreter.OutputFormat(" high");
        break;
    }

    mInterpreter.OutputLine(" %04x", aConfig.mRloc16);
}

void NetworkData::OutputRoute(const otExternalRouteConfig &aConfig)
{
    mInterpreter.OutputFormat("%x:%x:%x:%x::/%d ", HostSwap16(aConfig.mPrefix.mPrefix.mFields.m16[0]),
                              HostSwap16(aConfig.mPrefix.mPrefix.mFields.m16[1]),
                              HostSwap16(aConfig.mPrefix.mPrefix.mFields.m16[2]),
                              HostSwap16(aConfig.mPrefix.mPrefix.mFields.m16[3]), aConfig.mPrefix.mLength);

    if (aConfig.mStable)
    {
        mInterpreter.OutputFormat("s ");
    }

    switch (aConfig.mPreference)
    {
    case OT_ROUTE_PREFERENCE_LOW:
        mInterpreter.OutputFormat("low");
        break;

    case OT_ROUTE_PREFERENCE_MED:
        mInterpreter.OutputFormat("med");
        break;

    case OT_ROUTE_PREFERENCE_HIGH:
        mInterpreter.OutputFormat("high");
        break;
    }

    mInterpreter.OutputLine(" %04x", aConfig.mRloc16);
}

void NetworkData::OutputService(const otServiceConfig &aConfig)
{
    mInterpreter.OutputFormat("%u ", aConfig.mEnterpriseNumber);
    mInterpreter.OutputBytes(aConfig.mServiceData, aConfig.mServiceDataLength);
    mInterpreter.OutputFormat(" ");
    mInterpreter.OutputBytes(aConfig.mServerConfig.mServerData, aConfig.mServerConfig.mServerDataLength);

    if (aConfig.mServerConfig.mStable)
    {
        mInterpreter.OutputFormat(" s");
    }

    mInterpreter.OutputLine(" %04x", aConfig.mServerConfig.mRloc16);
}

otError NetworkData::ProcessHelp(uint8_t aArgsLength, char *aArgs[])
{
    OT_UNUSED_VARIABLE(aArgsLength);
    OT_UNUSED_VARIABLE(aArgs);

    for (const Command &command : sCommands)
    {
        mInterpreter.OutputLine("%s", command.mName);
    }

    return OT_ERROR_NONE;
}

#if OPENTHREAD_CONFIG_BORDER_ROUTER_ENABLE || OPENTHREAD_CONFIG_TMF_NETDATA_SERVICE_ENABLE
otError NetworkData::ProcessRegister(uint8_t aArgsLength, char *aArgs[])
{
    OT_UNUSED_VARIABLE(aArgsLength);
    OT_UNUSED_VARIABLE(aArgs);

    otError error = OT_ERROR_NONE;

#if OPENTHREAD_CONFIG_BORDER_ROUTER_ENABLE
    SuccessOrExit(error = otBorderRouterRegister(mInterpreter.mInstance));
#else
    SuccessOrExit(error = otServerRegister(mInterpreter.mInstance));
#endif

exit:
    return error;
}
#endif

otError NetworkData::ProcessSteeringData(uint8_t aArgsLength, char *aArgs[])
{
    otError           error = OT_ERROR_INVALID_ARGS;
    otExtAddress      addr;
    otJoinerDiscerner discerner;

    VerifyOrExit((aArgsLength > 1) && (strcmp(aArgs[0], "check") == 0), OT_NOOP);

    discerner.mLength = 0;

    error = Interpreter::ParseJoinerDiscerner(aArgs[1], discerner);

    if (error == OT_ERROR_NOT_FOUND)
    {
        VerifyOrExit(Interpreter::Hex2Bin(aArgs[1], addr.m8, sizeof(addr)) == sizeof(addr), OT_NOOP);
    }
    else if (error != OT_ERROR_NONE)
    {
        ExitNow();
    }

    if (discerner.mLength)
    {
        error = otNetDataSteeringDataCheckJoinerWithDiscerner(mInterpreter.mInstance, &discerner);
    }
    else
    {
        error = otNetDataSteeringDataCheckJoiner(mInterpreter.mInstance, &addr);
    }

exit:
    return error;
}

void NetworkData::OutputPrefixes(void)
{
    otNetworkDataIterator iterator = OT_NETWORK_DATA_ITERATOR_INIT;
    otBorderRouterConfig  config;

    mInterpreter.OutputLine("Prefixes:");

    while (otNetDataGetNextOnMeshPrefix(mInterpreter.mInstance, &iterator, &config) == OT_ERROR_NONE)
    {
        OutputPrefix(config);
    }
}

void NetworkData::OutputRoutes(void)
{
    otNetworkDataIterator iterator = OT_NETWORK_DATA_ITERATOR_INIT;
    otExternalRouteConfig config;

    mInterpreter.OutputLine("Routes:");

    while (otNetDataGetNextRoute(mInterpreter.mInstance, &iterator, &config) == OT_ERROR_NONE)
    {
        OutputRoute(config);
    }
}

void NetworkData::OutputServices(void)
{
    otNetworkDataIterator iterator = OT_NETWORK_DATA_ITERATOR_INIT;
    otServiceConfig       config;

    mInterpreter.OutputLine("Services:");

    while (otNetDataGetNextService(mInterpreter.mInstance, &iterator, &config) == OT_ERROR_NONE)
    {
        OutputService(config);
    }
}

otError NetworkData::OutputBinary(void)
{
    otError error = OT_ERROR_NONE;
    uint8_t data[255];
    uint8_t len = sizeof(data);

    SuccessOrExit(error = otNetDataGet(mInterpreter.mInstance, false, data, &len));

    mInterpreter.OutputBytes(data, static_cast<uint8_t>(len));
    mInterpreter.OutputLine("");

exit:
    return error;
}

otError NetworkData::ProcessShow(uint8_t aArgsLength, char *aArgs[])
{
    otError error;

    if (aArgsLength == 0)
    {
        OutputPrefixes();
        OutputRoutes();
        OutputServices();
        error = OT_ERROR_NONE;
    }
    else if (strcmp(aArgs[0], "-x") == 0)
    {
        error = OutputBinary();
    }
    else
    {
        error = OT_ERROR_INVALID_ARGS;
    }

    return error;
}

otError NetworkData::Process(uint8_t aArgsLength, char *aArgs[])
{
    otError error = OT_ERROR_INVALID_COMMAND;

    if (aArgsLength < 1)
    {
        IgnoreError(ProcessHelp(0, nullptr));
        error = OT_ERROR_INVALID_ARGS;
    }
    else
    {
        for (const Command &command : sCommands)
        {
            if (strcmp(aArgs[0], command.mName) == 0)
            {
                error = (this->*command.mCommand)(aArgsLength - 1, aArgs + 1);
                break;
            }
        }
    }
    return error;
}

} // namespace Cli
} // namespace ot
