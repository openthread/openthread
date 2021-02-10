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
 *   This file implements a simple CLI for the SRP Client.
 */

#include "cli_srp_client.hpp"

#if OPENTHREAD_CONFIG_SRP_CLIENT_ENABLE

#include <string.h>

#include "cli/cli.hpp"
#include "utils/parse_cmdline.hpp"

using ot::Utils::CmdLineParser::ParseAsBool;
using ot::Utils::CmdLineParser::ParseAsHexString;
using ot::Utils::CmdLineParser::ParseAsIp6Address;
using ot::Utils::CmdLineParser::ParseAsUint16;
using ot::Utils::CmdLineParser::ParseAsUint32;

namespace ot {
namespace Cli {

constexpr SrpClient::Command SrpClient::sCommands[];

template <uint16_t kDestSize> static otError CopyString(char (&aDestination)[kDestSize], const char *aSource)
{
    // Copies a string from `aSource` to `aDestination` (char array),
    // verifying that the string fits in the destination array.

    otError error = OT_ERROR_NONE;
    size_t  len   = strlen(aSource);

    VerifyOrExit(len + 1 <= kDestSize, error = OT_ERROR_INVALID_ARGS);
    memcpy(aDestination, aSource, len + 1);

exit:
    return error;
}

SrpClient::SrpClient(Interpreter &aInterpreter)
    : mInterpreter(aInterpreter)
    , mCallbackEnabled(false)
{
    for (Service &service : mServicePool)
    {
        service.MarkAsNotInUse();
    }

    memset(mHostAddresses, 0, sizeof(mHostAddresses));

    otSrpClientSetCallback(mInterpreter.mInstance, SrpClient::HandleCallback, this);
}

otError SrpClient::Process(uint8_t aArgsLength, char *aArgs[])
{
    otError        error = OT_ERROR_INVALID_COMMAND;
    const Command *command;

    VerifyOrExit(aArgsLength != 0, IgnoreError(ProcessHelp(0, nullptr)));

    command = Utils::LookupTable::Find(aArgs[0], sCommands);
    VerifyOrExit(command != nullptr);

    error = (this->*command->mHandler)(aArgsLength - 1, aArgs + 1);

exit:
    return error;
}

otError SrpClient::ProcessCallback(uint8_t aArgsLength, char *aArgs[])
{
    otError error = OT_ERROR_NONE;

    if (aArgsLength == 0)
    {
        mInterpreter.OutputEnabledDisabledStatus(mCallbackEnabled);
        ExitNow();
    }

    VerifyOrExit(aArgsLength == 1, error = OT_ERROR_INVALID_ARGS);

    if (strcmp(aArgs[0], "enable") == 0)
    {
        mCallbackEnabled = true;
    }
    else if (strcmp(aArgs[0], "disable") == 0)
    {
        mCallbackEnabled = false;
    }
    else
    {
        error = OT_ERROR_INVALID_ARGS;
    }

exit:
    return error;
}

otError SrpClient::ProcessHelp(uint8_t aArgsLength, char *aArgs[])
{
    OT_UNUSED_VARIABLE(aArgsLength);
    OT_UNUSED_VARIABLE(aArgs);

    for (const Command &command : sCommands)
    {
        mInterpreter.OutputLine(command.mName);
    }

    return OT_ERROR_NONE;
}

otError SrpClient::ProcessHost(uint8_t aArgsLength, char *aArgs[])
{
    otError error = OT_ERROR_NONE;

    if (aArgsLength == 0)
    {
        OutputHostInfo(0, *otSrpClientGetHostInfo(mInterpreter.mInstance));
        ExitNow();
    }

    if (strcmp(aArgs[0], "name") == 0)
    {
        if (aArgsLength == 1)
        {
            const char *name = otSrpClientGetHostInfo(mInterpreter.mInstance)->mName;
            mInterpreter.OutputLine("%s", (name != nullptr) ? name : "(null)");
        }
        else
        {
            VerifyOrExit(aArgsLength == 2, error = OT_ERROR_INVALID_ARGS);
            SuccessOrExit(error = CopyString(mHostName, aArgs[1]));
            error = otSrpClientSetHostName(mInterpreter.mInstance, mHostName);
        }
    }
    else if (strcmp(aArgs[0], "state") == 0)
    {
        VerifyOrExit(aArgsLength == 1, error = OT_ERROR_INVALID_ARGS);
        mInterpreter.OutputLine("%s",
                                otSrpClientItemStateToString(otSrpClientGetHostInfo(mInterpreter.mInstance)->mState));
    }
    else if (strcmp(aArgs[0], "address") == 0)
    {
        if (aArgsLength == 1)
        {
            const otSrpClientHostInfo *hostInfo = otSrpClientGetHostInfo(mInterpreter.mInstance);

            for (uint8_t index = 0; index < hostInfo->mNumAddresses; index++)
            {
                mInterpreter.OutputIp6Address(hostInfo->mAddresses[index]);
                mInterpreter.OutputLine("");
            }
        }
        else
        {
            uint8_t      numAddresses;
            otIp6Address addresses[kMaxHostAddresses];

            numAddresses = 0;
            memset(addresses, 0, sizeof(addresses));

            while (aArgsLength > 1)
            {
                VerifyOrExit(numAddresses < kMaxHostAddresses, error = OT_ERROR_NO_BUFS);
                aArgsLength--;
                aArgs++;
                SuccessOrExit(error = ParseAsIp6Address(aArgs[0], addresses[numAddresses++]));
            }

            VerifyOrExit(numAddresses > 0, error = OT_ERROR_INVALID_ARGS);
            memcpy(mHostAddresses, addresses, sizeof(addresses));

            error = otSrpClientSetHostAddresses(mInterpreter.mInstance, mHostAddresses, numAddresses);
        }
    }
    else if (strcmp(aArgs[0], "remove") == 0)
    {
        bool removeKeyLease = false;

        if (aArgsLength > 1)
        {
            VerifyOrExit(aArgsLength == 2, error = OT_ERROR_INVALID_ARGS);
            SuccessOrExit(error = ParseAsBool(aArgs[1], removeKeyLease));
        }

        error = otSrpClientRemoveHostAndServices(mInterpreter.mInstance, removeKeyLease);
    }
    else if (strcmp(aArgs[0], "clear") == 0)
    {
        VerifyOrExit(aArgsLength == 1, error = OT_ERROR_INVALID_ARGS);
        otSrpClientClearHostAndServices(mInterpreter.mInstance);

        for (Service &poolEntry : mServicePool)
        {
            poolEntry.MarkAsNotInUse();
        }
    }
    else
    {
        error = OT_ERROR_INVALID_COMMAND;
    }

exit:
    return error;
}

otError SrpClient::ProcessLeaseInterval(uint8_t aArgsLength, char *aArgs[])
{
    otError  error = OT_ERROR_NONE;
    uint32_t interval;

    if (aArgsLength == 0)
    {
        mInterpreter.OutputLine("%u", otSrpClientGetLeaseInterval(mInterpreter.mInstance));
        ExitNow();
    }

    VerifyOrExit(aArgsLength == 1, error = OT_ERROR_INVALID_ARGS);
    SuccessOrExit(error = ParseAsUint32(aArgs[0], interval));
    otSrpClientSetLeaseInterval(mInterpreter.mInstance, interval);

exit:
    return error;
}

otError SrpClient::ProcessKeyLeaseInterval(uint8_t aArgsLength, char *aArgs[])
{
    otError  error = OT_ERROR_NONE;
    uint32_t interval;

    if (aArgsLength == 0)
    {
        mInterpreter.OutputLine("%u", otSrpClientGetKeyLeaseInterval(mInterpreter.mInstance));
        ExitNow();
    }

    VerifyOrExit(aArgsLength == 1, error = OT_ERROR_INVALID_ARGS);
    SuccessOrExit(error = ParseAsUint32(aArgs[0], interval));
    otSrpClientSetKeyLeaseInterval(mInterpreter.mInstance, interval);

exit:
    return error;
}

otError SrpClient::ProcessServer(uint8_t aArgsLength, char *aArgs[])
{
    otError           error          = OT_ERROR_NONE;
    const otSockAddr *serverSockAddr = otSrpClientGetServerAddress(mInterpreter.mInstance);

    if (aArgsLength == 0)
    {
        mInterpreter.OutputFormat("[");
        mInterpreter.OutputIp6Address(serverSockAddr->mAddress);
        mInterpreter.OutputLine("]:%u", serverSockAddr->mPort);
        ExitNow();
    }

    VerifyOrExit(aArgsLength == 1, error = OT_ERROR_INVALID_ARGS);

    if (strcmp(aArgs[0], "address") == 0)
    {
        mInterpreter.OutputIp6Address(serverSockAddr->mAddress);
        mInterpreter.OutputLine("");
    }
    else if (strcmp(aArgs[0], "port") == 0)
    {
        mInterpreter.OutputLine("%u", serverSockAddr->mPort);
    }
    else
    {
        error = OT_ERROR_INVALID_COMMAND;
    }

exit:
    return error;
}

otError SrpClient::ProcessService(uint8_t aArgsLength, char *aArgs[])
{
    otError error = OT_ERROR_NONE;

    if (aArgsLength == 0)
    {
        OutputServiceList(0, otSrpClientGetServices(mInterpreter.mInstance));
        ExitNow();
    }

    if (strcmp(aArgs[0], "add") == 0)
    {
        // `add` <instance-name> <service-name> <port> [priority] [weight] [txt]

        Service *entry = nullptr;

        VerifyOrExit(4 <= aArgsLength && aArgsLength <= 7, error = OT_ERROR_INVALID_ARGS);

        for (Service &poolEntry : mServicePool)
        {
            if (!poolEntry.IsInUse())
            {
                entry = &poolEntry;
                break;
            }
        }

        VerifyOrExit(entry != nullptr, error = OT_ERROR_NO_BUFS);

        memset(&entry->mService, 0, sizeof(entry->mService));

        SuccessOrExit(error = CopyString(entry->mInstanceName, aArgs[1]));
        entry->mService.mInstanceName = entry->mInstanceName;

        SuccessOrExit(error = CopyString(entry->mServiceName, aArgs[2]));
        entry->mService.mName = entry->mServiceName;

        SuccessOrExit(error = ParseAsUint16(aArgs[3], entry->mService.mPort));

        if (aArgsLength >= 5)
        {
            SuccessOrExit(error = ParseAsUint16(aArgs[4], entry->mService.mPriority));
        }

        if (aArgsLength >= 6)
        {
            SuccessOrExit(error = ParseAsUint16(aArgs[5], entry->mService.mWeight));
        }

        if (aArgsLength >= 7)
        {
            entry->mService.mNumTxtEntries = 1;
            entry->mService.mTxtEntries    = &entry->mTxtEntry;
            entry->mTxtEntry.mKey          = nullptr; // Treat`mValue` as an already encoded TXT-DATA
            entry->mTxtEntry.mKeyLength    = 0;
            entry->mTxtEntry.mValue        = entry->mTxtBuffer;
            entry->mTxtEntry.mValueLength  = sizeof(entry->mTxtBuffer);

            SuccessOrExit(error = ParseAsHexString(aArgs[6], entry->mTxtEntry.mValueLength, entry->mTxtBuffer));
        }

        error = otSrpClientAddService(mInterpreter.mInstance, &entry->mService);

        if (error != OT_ERROR_NONE)
        {
            entry->MarkAsNotInUse();
        }
    }
    else if (strcmp(aArgs[0], "remove") == 0)
    {
        // `remove` <instance-name> <service-name>

        Service *entry = nullptr;

        VerifyOrExit(aArgsLength == 3, error = OT_ERROR_INVALID_ARGS);

        for (Service &poolEntry : mServicePool)
        {
            if (poolEntry.IsInUse() && (strcmp(aArgs[1], poolEntry.mInstanceName) == 0) &&
                (strcmp(aArgs[2], poolEntry.mServiceName) == 0))
            {
                entry = &poolEntry;
                break;
            }
        }

        VerifyOrExit(entry != nullptr, error = OT_ERROR_NOT_FOUND);

        error = otSrpClientRemoveService(mInterpreter.mInstance, &entry->mService);
    }
    else
    {
        error = OT_ERROR_INVALID_COMMAND;
    }

exit:
    return error;
}

void SrpClient::OutputHostInfo(uint8_t aIndentSize, const otSrpClientHostInfo &aHostInfo)
{
    mInterpreter.OutputFormat(aIndentSize, "name:");

    if (aHostInfo.mName != nullptr)
    {
        mInterpreter.OutputFormat("\"%s\"", aHostInfo.mName);
    }
    else
    {
        mInterpreter.OutputFormat("(null)");
    }

    mInterpreter.OutputFormat(", state:%s, addrs:[", otSrpClientItemStateToString(aHostInfo.mState));

    for (uint8_t index = 0; index < aHostInfo.mNumAddresses; index++)
    {
        if (index > 0)
        {
            mInterpreter.OutputFormat(", ");
        }

        mInterpreter.OutputIp6Address(aHostInfo.mAddresses[index]);
    }

    mInterpreter.OutputLine("]");
}

void SrpClient::OutputServiceList(uint8_t aIndentSize, const otSrpClientService *aServices)
{
    while (aServices != nullptr)
    {
        OutputService(aIndentSize, *aServices);
        aServices = aServices->mNext;
    }
}

void SrpClient::OutputService(uint8_t aIndentSize, const otSrpClientService &aService)
{
    mInterpreter.OutputLine(aIndentSize, "instance:\"%s\", name:\"%s\", state:%s, port:%d, priority:%d, weight:%d",
                            aService.mInstanceName, aService.mName, otSrpClientItemStateToString(aService.mState),
                            aService.mPort, aService.mPriority, aService.mWeight);
}

otError SrpClient::ProcessStart(uint8_t aArgsLength, char *aArgs[])
{
    otError    error = OT_ERROR_NONE;
    otSockAddr serverSockAddr;

    VerifyOrExit(aArgsLength == 2, error = OT_ERROR_INVALID_ARGS);

    SuccessOrExit(error = ParseAsIp6Address(aArgs[0], serverSockAddr.mAddress));
    SuccessOrExit(error = ParseAsUint16(aArgs[1], serverSockAddr.mPort));

    error = otSrpClientStart(mInterpreter.mInstance, &serverSockAddr);

exit:
    return error;
}

otError SrpClient::ProcessStop(uint8_t aArgsLength, char *aArgs[])
{
    OT_UNUSED_VARIABLE(aArgs);

    otError error = OT_ERROR_NONE;

    VerifyOrExit(aArgsLength == 0, error = OT_ERROR_INVALID_ARGS);
    otSrpClientStop(mInterpreter.mInstance);

exit:
    return error;
}

void SrpClient::HandleCallback(otError                    aError,
                               const otSrpClientHostInfo *aHostInfo,
                               const otSrpClientService * aServices,
                               const otSrpClientService * aRemovedServices,
                               void *                     aContext)
{
    static_cast<SrpClient *>(aContext)->HandleCallback(aError, aHostInfo, aServices, aRemovedServices);
}

void SrpClient::HandleCallback(otError                    aError,
                               const otSrpClientHostInfo *aHostInfo,
                               const otSrpClientService * aServices,
                               const otSrpClientService * aRemovedServices)
{
    otSrpClientService *next;

    if (mCallbackEnabled)
    {
        mInterpreter.OutputLine("SRP client callback - error:%s", otThreadErrorToString(aError));
        mInterpreter.OutputLine("Host info:");
        OutputHostInfo(kIndentSize, *aHostInfo);

        mInterpreter.OutputLine("Service list:");
        OutputServiceList(kIndentSize, aServices);

        if (aRemovedServices != nullptr)
        {
            mInterpreter.OutputLine("Removed service list:");
            OutputServiceList(kIndentSize, aRemovedServices);
        }
    }

    // Go through removed services and mark the corresponding entry in
    // `mServicePool` as "not in use".

    for (const otSrpClientService *service = aRemovedServices; service != nullptr; service = next)
    {
        next = service->mNext;

        for (Service &poolEntry : mServicePool)
        {
            if (service == &poolEntry.mService)
            {
                poolEntry.MarkAsNotInUse();
                break;
            }
        }
    }
}

} // namespace Cli
} // namespace ot

#endif // OPENTHREAD_CONFIG_SRP_CLIENT_ENABLE
