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

namespace ot {
namespace Cli {

constexpr SrpClient::Command SrpClient::sCommands[];

static otError CopyString(char *aDest, uint16_t aDestSize, const char *aSource)
{
    // Copies a string from `aSource` to `aDestination` (char array),
    // verifying that the string fits in the destination array.

    otError error = OT_ERROR_NONE;
    size_t  len   = strlen(aSource);

    VerifyOrExit(len + 1 <= aDestSize, error = OT_ERROR_INVALID_ARGS);
    memcpy(aDest, aSource, len + 1);

exit:
    return error;
}

SrpClient::SrpClient(Interpreter &aInterpreter)
    : mInterpreter(aInterpreter)
    , mCallbackEnabled(false)
{
    otSrpClientSetCallback(mInterpreter.mInstance, SrpClient::HandleCallback, this);
}

otError SrpClient::Process(uint8_t aArgsLength, Arg aArgs[])
{
    otError        error = OT_ERROR_INVALID_COMMAND;
    const Command *command;

    VerifyOrExit(aArgsLength != 0, IgnoreError(ProcessHelp(0, nullptr)));

    command = Utils::LookupTable::Find(aArgs[0].GetCString(), sCommands);
    VerifyOrExit(command != nullptr);

    error = (this->*command->mHandler)(aArgsLength - 1, aArgs + 1);

exit:
    return error;
}

#if OPENTHREAD_CONFIG_SRP_CLIENT_AUTO_START_API_ENABLE

otError SrpClient::ProcessAutoStart(uint8_t aArgsLength, Arg aArgs[])
{
    otError error = OT_ERROR_NONE;
    bool    enable;

    if (aArgsLength == 0)
    {
        mInterpreter.OutputEnabledDisabledStatus(otSrpClientIsAutoStartModeEnabled(mInterpreter.mInstance));
        ExitNow();
    }

    VerifyOrExit(aArgsLength == 1, error = OT_ERROR_INVALID_ARGS);

    SuccessOrExit(error = Interpreter::ParseEnableOrDisable(aArgs[0], enable));

    if (enable)
    {
        otSrpClientEnableAutoStartMode(mInterpreter.mInstance, /* aCallback */ nullptr, /* aContext */ nullptr);
    }
    else
    {
        otSrpClientDisableAutoStartMode(mInterpreter.mInstance);
    }

exit:
    return error;
}

#endif // OPENTHREAD_CONFIG_SRP_CLIENT_AUTO_START_API_ENABLE

otError SrpClient::ProcessCallback(uint8_t aArgsLength, Arg aArgs[])
{
    otError error = OT_ERROR_NONE;

    if (aArgsLength == 0)
    {
        mInterpreter.OutputEnabledDisabledStatus(mCallbackEnabled);
        ExitNow();
    }

    VerifyOrExit(aArgsLength == 1, error = OT_ERROR_INVALID_ARGS);
    error = Interpreter::ParseEnableOrDisable(aArgs[0], mCallbackEnabled);

exit:
    return error;
}

otError SrpClient::ProcessHelp(uint8_t aArgsLength, Arg aArgs[])
{
    OT_UNUSED_VARIABLE(aArgsLength);
    OT_UNUSED_VARIABLE(aArgs);

    for (const Command &command : sCommands)
    {
        mInterpreter.OutputLine(command.mName);
    }

    return OT_ERROR_NONE;
}

otError SrpClient::ProcessHost(uint8_t aArgsLength, Arg aArgs[])
{
    otError error = OT_ERROR_NONE;

    if (aArgsLength == 0)
    {
        OutputHostInfo(0, *otSrpClientGetHostInfo(mInterpreter.mInstance));
        ExitNow();
    }

    if (aArgs[0] == "name")
    {
        if (aArgsLength == 1)
        {
            const char *name = otSrpClientGetHostInfo(mInterpreter.mInstance)->mName;
            mInterpreter.OutputLine("%s", (name != nullptr) ? name : "(null)");
        }
        else
        {
            uint16_t len;
            uint16_t size;
            char *   hostName;

            VerifyOrExit(aArgsLength == 2, error = OT_ERROR_INVALID_ARGS);
            hostName = otSrpClientBuffersGetHostNameString(mInterpreter.mInstance, &size);

            len = aArgs[1].GetLength();
            VerifyOrExit(len + 1 <= size, error = OT_ERROR_INVALID_ARGS);

            // We first make sure we can set the name, and if so
            // we copy it to the persisted string buffer and set
            // the host name again now with the persisted buffer.
            // This ensures that we do not overwrite a previous
            // buffer with a host name that cannot be set.

            SuccessOrExit(error = otSrpClientSetHostName(mInterpreter.mInstance, aArgs[1].GetCString()));
            memcpy(hostName, aArgs[1].GetCString(), len + 1);

            IgnoreError(otSrpClientSetHostName(mInterpreter.mInstance, hostName));
        }
    }
    else if (aArgs[0] == "state")
    {
        VerifyOrExit(aArgsLength == 1, error = OT_ERROR_INVALID_ARGS);
        mInterpreter.OutputLine("%s",
                                otSrpClientItemStateToString(otSrpClientGetHostInfo(mInterpreter.mInstance)->mState));
    }
    else if (aArgs[0] == "address")
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
            uint8_t       numAddresses = aArgsLength - 1;
            otIp6Address  addresses[kMaxHostAddresses];
            uint8_t       arrayLength;
            otIp6Address *hostAddressArray;

            hostAddressArray = otSrpClientBuffersGetHostAddressesArray(mInterpreter.mInstance, &arrayLength);

            // We first make sure we can set the addresses, and if so
            // we copy the address list into the persisted address array
            // and set it again. This ensures that we do not overwrite
            // a previous list before we know it is safe to set/change
            // the address list.

            VerifyOrExit(numAddresses <= arrayLength, error = OT_ERROR_NO_BUFS);

            for (uint8_t index = 1; index < aArgsLength; index++)
            {
                SuccessOrExit(error = aArgs[index].ParseAsIp6Address(addresses[index - 1]));
            }

            SuccessOrExit(error = otSrpClientSetHostAddresses(mInterpreter.mInstance, addresses, numAddresses));

            memcpy(hostAddressArray, addresses, numAddresses * sizeof(hostAddressArray[0]));
            IgnoreError(otSrpClientSetHostAddresses(mInterpreter.mInstance, hostAddressArray, numAddresses));
        }
    }
    else if (aArgs[0] == "remove")
    {
        bool removeKeyLease = false;

        if (aArgsLength > 1)
        {
            VerifyOrExit(aArgsLength == 2, error = OT_ERROR_INVALID_ARGS);
            SuccessOrExit(error = aArgs[1].ParseAsBool(removeKeyLease));
        }

        error = otSrpClientRemoveHostAndServices(mInterpreter.mInstance, removeKeyLease);
    }
    else if (aArgs[0] == "clear")
    {
        VerifyOrExit(aArgsLength == 1, error = OT_ERROR_INVALID_ARGS);
        otSrpClientClearHostAndServices(mInterpreter.mInstance);
        otSrpClientBuffersFreeAllServices(mInterpreter.mInstance);
    }
    else
    {
        error = OT_ERROR_INVALID_COMMAND;
    }

exit:
    return error;
}

otError SrpClient::ProcessLeaseInterval(uint8_t aArgsLength, Arg aArgs[])
{
    return mInterpreter.ProcessGetSet(aArgsLength, aArgs, otSrpClientGetLeaseInterval, otSrpClientSetLeaseInterval);
}

otError SrpClient::ProcessKeyLeaseInterval(uint8_t aArgsLength, Arg aArgs[])
{
    return mInterpreter.ProcessGetSet(aArgsLength, aArgs, otSrpClientGetKeyLeaseInterval,
                                      otSrpClientSetKeyLeaseInterval);
}

otError SrpClient::ProcessServer(uint8_t aArgsLength, Arg aArgs[])
{
    otError           error          = OT_ERROR_NONE;
    const otSockAddr *serverSockAddr = otSrpClientGetServerAddress(mInterpreter.mInstance);

    if (aArgsLength == 0)
    {
        char string[OT_IP6_SOCK_ADDR_STRING_SIZE];

        otIp6SockAddrToString(serverSockAddr, string, sizeof(string));
        mInterpreter.OutputLine(string);
        ExitNow();
    }

    VerifyOrExit(aArgsLength == 1, error = OT_ERROR_INVALID_ARGS);

    if (aArgs[0] == "address")
    {
        mInterpreter.OutputIp6Address(serverSockAddr->mAddress);
        mInterpreter.OutputLine("");
    }
    else if (aArgs[0] == "port")
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

otError SrpClient::ProcessService(uint8_t aArgsLength, Arg aArgs[])
{
    otError error = OT_ERROR_NONE;
    bool    isRemove;

    if (aArgsLength == 0)
    {
        OutputServiceList(0, otSrpClientGetServices(mInterpreter.mInstance));
        ExitNow();
    }

    if (aArgs[0] == "add")
    {
        error = ProcessServiceAdd(aArgsLength, aArgs);
    }
    else if ((isRemove = (aArgs[0] == "remove")) || (aArgs[0] == "clear"))
    {
        // `remove`|`clear` <instance-name> <service-name>

        const otSrpClientService *service;

        VerifyOrExit(aArgsLength == 3, error = OT_ERROR_INVALID_ARGS);

        for (service = otSrpClientGetServices(mInterpreter.mInstance); service != nullptr; service = service->mNext)
        {
            if ((aArgs[1] == service->mInstanceName) && (aArgs[2] == service->mName))
            {
                break;
            }
        }

        VerifyOrExit(service != nullptr, error = OT_ERROR_NOT_FOUND);

        if (isRemove)
        {
            error = otSrpClientRemoveService(mInterpreter.mInstance, const_cast<otSrpClientService *>(service));
        }
        else
        {
            SuccessOrExit(
                error = otSrpClientClearService(mInterpreter.mInstance, const_cast<otSrpClientService *>(service)));

            otSrpClientBuffersFreeService(mInterpreter.mInstance, reinterpret_cast<otSrpClientBuffersServiceEntry *>(
                                                                      const_cast<otSrpClientService *>(service)));
        }
    }
#if OPENTHREAD_CONFIG_REFERENCE_DEVICE_ENABLE
    else if (aArgs[0] == "key")
    {
        // `key [enable/disable]`

        bool enable;

        if (aArgsLength == 1)
        {
            mInterpreter.OutputEnabledDisabledStatus(otSrpClientIsServiceKeyRecordEnabled(mInterpreter.mInstance));
            ExitNow();
        }

        VerifyOrExit(aArgsLength == 2, error = OT_ERROR_INVALID_ARGS);
        SuccessOrExit(error = Interpreter::ParseEnableOrDisable(aArgs[1], enable));
        otSrpClientSetServiceKeyRecordEnabled(mInterpreter.mInstance, enable);
    }
#endif // OPENTHREAD_CONFIG_REFERENCE_DEVICE_ENABLE
    else
    {
        error = OT_ERROR_INVALID_COMMAND;
    }

exit:
    return error;
}

otError SrpClient::ProcessServiceAdd(uint8_t aArgsLength, Arg aArgs[])
{
    // `add` <instance-name> <service-name> <port> [priority] [weight] [txt]

    otSrpClientBuffersServiceEntry *entry = nullptr;
    uint16_t                        size;
    char *                          string;
    otError                         error;

    VerifyOrExit(4 <= aArgsLength && aArgsLength <= 7, error = OT_ERROR_INVALID_ARGS);

    entry = otSrpClientBuffersAllocateService(mInterpreter.mInstance);

    VerifyOrExit(entry != nullptr, error = OT_ERROR_NO_BUFS);

    string = otSrpClientBuffersGetServiceEntryInstanceNameString(entry, &size);
    SuccessOrExit(error = CopyString(string, size, aArgs[1].GetCString()));

    string = otSrpClientBuffersGetServiceEntryServiceNameString(entry, &size);
    SuccessOrExit(error = CopyString(string, size, aArgs[2].GetCString()));

    SuccessOrExit(error = aArgs[3].ParseAsUint16(entry->mService.mPort));

    if (aArgsLength >= 5)
    {
        SuccessOrExit(error = aArgs[4].ParseAsUint16(entry->mService.mPriority));
    }

    if (aArgsLength >= 6)
    {
        SuccessOrExit(error = aArgs[5].ParseAsUint16(entry->mService.mWeight));
    }

    if (aArgsLength >= 7)
    {
        uint8_t *txtBuffer;

        txtBuffer                     = otSrpClientBuffersGetServiceEntryTxtBuffer(entry, &size);
        entry->mTxtEntry.mValueLength = size;

        SuccessOrExit(error = aArgs[6].ParseAsHexString(entry->mTxtEntry.mValueLength, txtBuffer));
    }
    else
    {
        entry->mService.mNumTxtEntries = 0;
    }

    SuccessOrExit(error = otSrpClientAddService(mInterpreter.mInstance, &entry->mService));

    entry = nullptr;

exit:
    if (entry != nullptr)
    {
        otSrpClientBuffersFreeService(mInterpreter.mInstance, entry);
    }

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

otError SrpClient::ProcessStart(uint8_t aArgsLength, Arg aArgs[])
{
    otError    error = OT_ERROR_NONE;
    otSockAddr serverSockAddr;

    VerifyOrExit(aArgsLength == 2, error = OT_ERROR_INVALID_ARGS);

    SuccessOrExit(error = aArgs[0].ParseAsIp6Address(serverSockAddr.mAddress));
    SuccessOrExit(error = aArgs[1].ParseAsUint16(serverSockAddr.mPort));

    error = otSrpClientStart(mInterpreter.mInstance, &serverSockAddr);

exit:
    return error;
}

otError SrpClient::ProcessState(uint8_t aArgsLength, Arg aArgs[])
{
    OT_UNUSED_VARIABLE(aArgs);

    otError error = OT_ERROR_NONE;

    VerifyOrExit(aArgsLength == 0, error = OT_ERROR_INVALID_ARGS);

    mInterpreter.OutputEnabledDisabledStatus(otSrpClientIsRunning(mInterpreter.mInstance));

exit:
    return error;
}

otError SrpClient::ProcessStop(uint8_t aArgsLength, Arg aArgs[])
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

    // Go through removed services and free all removed services

    for (const otSrpClientService *service = aRemovedServices; service != nullptr; service = next)
    {
        next = service->mNext;

        otSrpClientBuffersFreeService(mInterpreter.mInstance, reinterpret_cast<otSrpClientBuffersServiceEntry *>(
                                                                  const_cast<otSrpClientService *>(service)));
    }
}

} // namespace Cli
} // namespace ot

#endif // OPENTHREAD_CONFIG_SRP_CLIENT_ENABLE
