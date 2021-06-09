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

otError SrpClient::Process(Arg aArgs[])
{
    otError        error = OT_ERROR_INVALID_COMMAND;
    const Command *command;

    if (aArgs[0].IsEmpty())
    {
        IgnoreError(ProcessHelp(aArgs));
        ExitNow();
    }

    command = Utils::LookupTable::Find(aArgs[0].GetCString(), sCommands);
    VerifyOrExit(command != nullptr);

    error = (this->*command->mHandler)(aArgs + 1);

exit:
    return error;
}

#if OPENTHREAD_CONFIG_SRP_CLIENT_AUTO_START_API_ENABLE

otError SrpClient::ProcessAutoStart(Arg aArgs[])
{
    otError error = OT_ERROR_NONE;
    bool    enable;

    if (aArgs[0].IsEmpty())
    {
        mInterpreter.OutputEnabledDisabledStatus(otSrpClientIsAutoStartModeEnabled(mInterpreter.mInstance));
        ExitNow();
    }

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

otError SrpClient::ProcessCallback(Arg aArgs[])
{
    otError error = OT_ERROR_NONE;

    if (aArgs[0].IsEmpty())
    {
        mInterpreter.OutputEnabledDisabledStatus(mCallbackEnabled);
        ExitNow();
    }

    error = Interpreter::ParseEnableOrDisable(aArgs[0], mCallbackEnabled);

exit:
    return error;
}

otError SrpClient::ProcessHelp(Arg aArgs[])
{
    OT_UNUSED_VARIABLE(aArgs);

    for (const Command &command : sCommands)
    {
        mInterpreter.OutputLine(command.mName);
    }

    return OT_ERROR_NONE;
}

otError SrpClient::ProcessHost(Arg aArgs[])
{
    otError error = OT_ERROR_NONE;

    if (aArgs[0].IsEmpty())
    {
        OutputHostInfo(0, *otSrpClientGetHostInfo(mInterpreter.mInstance));
        ExitNow();
    }

    if (aArgs[0] == "name")
    {
        if (aArgs[1].IsEmpty())
        {
            const char *name = otSrpClientGetHostInfo(mInterpreter.mInstance)->mName;
            mInterpreter.OutputLine("%s", (name != nullptr) ? name : "(null)");
        }
        else
        {
            uint16_t len;
            uint16_t size;
            char *   hostName;

            VerifyOrExit(aArgs[2].IsEmpty(), error = OT_ERROR_INVALID_ARGS);
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
        VerifyOrExit(aArgs[1].IsEmpty(), error = OT_ERROR_INVALID_ARGS);
        mInterpreter.OutputLine("%s",
                                otSrpClientItemStateToString(otSrpClientGetHostInfo(mInterpreter.mInstance)->mState));
    }
    else if (aArgs[0] == "address")
    {
        if (aArgs[1].IsEmpty())
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
            uint8_t       numAddresses = 0;
            otIp6Address  addresses[kMaxHostAddresses];
            uint8_t       arrayLength;
            otIp6Address *hostAddressArray;

            hostAddressArray = otSrpClientBuffersGetHostAddressesArray(mInterpreter.mInstance, &arrayLength);

            // We first make sure we can set the addresses, and if so
            // we copy the address list into the persisted address array
            // and set it again. This ensures that we do not overwrite
            // a previous list before we know it is safe to set/change
            // the address list.

            if (arrayLength > kMaxHostAddresses)
            {
                arrayLength = kMaxHostAddresses;
            }

            for (Arg *arg = &aArgs[1]; !arg->IsEmpty(); arg++)
            {
                VerifyOrExit(numAddresses < arrayLength, error = OT_ERROR_NO_BUFS);
                SuccessOrExit(error = arg->ParseAsIp6Address(addresses[numAddresses]));
                numAddresses++;
            }

            SuccessOrExit(error = otSrpClientSetHostAddresses(mInterpreter.mInstance, addresses, numAddresses));

            memcpy(hostAddressArray, addresses, numAddresses * sizeof(hostAddressArray[0]));
            IgnoreError(otSrpClientSetHostAddresses(mInterpreter.mInstance, hostAddressArray, numAddresses));
        }
    }
    else if (aArgs[0] == "remove")
    {
        bool removeKeyLease = false;

        if (!aArgs[1].IsEmpty())
        {
            SuccessOrExit(error = aArgs[1].ParseAsBool(removeKeyLease));
            VerifyOrExit(aArgs[2].IsEmpty(), error = OT_ERROR_INVALID_ARGS);
        }

        error = otSrpClientRemoveHostAndServices(mInterpreter.mInstance, removeKeyLease);
    }
    else if (aArgs[0] == "clear")
    {
        VerifyOrExit(aArgs[1].IsEmpty(), error = OT_ERROR_INVALID_ARGS);
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

otError SrpClient::ProcessLeaseInterval(Arg aArgs[])
{
    return mInterpreter.ProcessGetSet(aArgs, otSrpClientGetLeaseInterval, otSrpClientSetLeaseInterval);
}

otError SrpClient::ProcessKeyLeaseInterval(Arg aArgs[])
{
    return mInterpreter.ProcessGetSet(aArgs, otSrpClientGetKeyLeaseInterval, otSrpClientSetKeyLeaseInterval);
}

otError SrpClient::ProcessServer(Arg aArgs[])
{
    otError           error          = OT_ERROR_NONE;
    const otSockAddr *serverSockAddr = otSrpClientGetServerAddress(mInterpreter.mInstance);

    if (aArgs[0].IsEmpty())
    {
        char string[OT_IP6_SOCK_ADDR_STRING_SIZE];

        otIp6SockAddrToString(serverSockAddr, string, sizeof(string));
        mInterpreter.OutputLine(string);
        ExitNow();
    }

    VerifyOrExit(aArgs[1].IsEmpty(), error = OT_ERROR_INVALID_ARGS);

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

otError SrpClient::ProcessService(Arg aArgs[])
{
    otError error = OT_ERROR_NONE;
    bool    isRemove;

    if (aArgs[0].IsEmpty())
    {
        OutputServiceList(0, otSrpClientGetServices(mInterpreter.mInstance));
        ExitNow();
    }

    if (aArgs[0] == "add")
    {
        error = ProcessServiceAdd(aArgs);
    }
    else if ((isRemove = (aArgs[0] == "remove")) || (aArgs[0] == "clear"))
    {
        // `remove`|`clear` <instance-name> <service-name>

        const otSrpClientService *service;

        VerifyOrExit(!aArgs[2].IsEmpty() && aArgs[3].IsEmpty(), error = OT_ERROR_INVALID_ARGS);

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

        if (aArgs[1].IsEmpty())
        {
            mInterpreter.OutputEnabledDisabledStatus(otSrpClientIsServiceKeyRecordEnabled(mInterpreter.mInstance));
            ExitNow();
        }

        SuccessOrExit(error = Interpreter::ParseEnableOrDisable(aArgs[1], enable));
        VerifyOrExit(aArgs[2].IsEmpty(), error = OT_ERROR_INVALID_ARGS);
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

otError SrpClient::ProcessServiceAdd(Arg aArgs[])
{
    // `add` <instance-name> <service-name> <port> [priority] [weight] [txt]

    otSrpClientBuffersServiceEntry *entry = nullptr;
    uint16_t                        size;
    char *                          string;
    otError                         error;
    char *                          label;

    entry = otSrpClientBuffersAllocateService(mInterpreter.mInstance);

    VerifyOrExit(entry != nullptr, error = OT_ERROR_NO_BUFS);

    SuccessOrExit(error = aArgs[3].ParseAsUint16(entry->mService.mPort));

    // Successfully parsing aArgs[3] indicates that aArgs[1] and
    // aArgs[2] are also non-empty.

    string = otSrpClientBuffersGetServiceEntryInstanceNameString(entry, &size);
    SuccessOrExit(error = CopyString(string, size, aArgs[1].GetCString()));

    string = otSrpClientBuffersGetServiceEntryServiceNameString(entry, &size);
    SuccessOrExit(error = CopyString(string, size, aArgs[2].GetCString()));

    // Service subtypes are added as part of service name as a comma separated list
    // e.g., "_service._udp,_sub1,_sub2"

    label = strchr(string, ',');

    if (label != nullptr)
    {
        uint16_t     arrayLength;
        const char **subTypeLabels = otSrpClientBuffersGetSubTypeLabelsArray(entry, &arrayLength);

        // Leave the last array element as `nullptr` to indicate end of array.
        for (uint16_t index = 0; index + 1 < arrayLength; index++)
        {
            *label++             = '\0';
            subTypeLabels[index] = label;

            label = strchr(label, ',');

            if (label == nullptr)
            {
                break;
            }
        }

        VerifyOrExit(label == nullptr, error = OT_ERROR_NO_BUFS);
    }

    SuccessOrExit(error = aArgs[3].ParseAsUint16(entry->mService.mPort));

    if (!aArgs[4].IsEmpty())
    {
        SuccessOrExit(error = aArgs[4].ParseAsUint16(entry->mService.mPriority));
    }

    if (!aArgs[5].IsEmpty())
    {
        SuccessOrExit(error = aArgs[5].ParseAsUint16(entry->mService.mWeight));
    }

    if (!aArgs[6].IsEmpty())
    {
        uint8_t *txtBuffer;

        txtBuffer                     = otSrpClientBuffersGetServiceEntryTxtBuffer(entry, &size);
        entry->mTxtEntry.mValueLength = size;

        SuccessOrExit(error = aArgs[6].ParseAsHexString(entry->mTxtEntry.mValueLength, txtBuffer));
        VerifyOrExit(aArgs[7].IsEmpty(), error = OT_ERROR_INVALID_ARGS);
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
    mInterpreter.OutputFormat(aIndentSize, "instance:\"%s\", name:\"%s", aService.mInstanceName, aService.mName);

    if (aService.mSubTypeLabels != nullptr)
    {
        for (uint16_t index = 0; aService.mSubTypeLabels[index] != nullptr; index++)
        {
            mInterpreter.OutputFormat(",%s", aService.mSubTypeLabels[index]);
        }
    }

    mInterpreter.OutputLine("\", state:%s, port:%d, priority:%d, weight:%d",
                            otSrpClientItemStateToString(aService.mState), aService.mPort, aService.mPriority,
                            aService.mWeight);
}

otError SrpClient::ProcessStart(Arg aArgs[])
{
    otError    error = OT_ERROR_NONE;
    otSockAddr serverSockAddr;

    SuccessOrExit(error = aArgs[0].ParseAsIp6Address(serverSockAddr.mAddress));
    SuccessOrExit(error = aArgs[1].ParseAsUint16(serverSockAddr.mPort));
    VerifyOrExit(aArgs[2].IsEmpty(), error = OT_ERROR_INVALID_ARGS);

    error = otSrpClientStart(mInterpreter.mInstance, &serverSockAddr);

exit:
    return error;
}

otError SrpClient::ProcessState(Arg aArgs[])
{
    otError error = OT_ERROR_NONE;

    VerifyOrExit(aArgs[0].IsEmpty(), error = OT_ERROR_INVALID_ARGS);

    mInterpreter.OutputEnabledDisabledStatus(otSrpClientIsRunning(mInterpreter.mInstance));

exit:
    return error;
}

otError SrpClient::ProcessStop(Arg aArgs[])
{
    otError error = OT_ERROR_NONE;

    VerifyOrExit(aArgs[0].IsEmpty(), error = OT_ERROR_INVALID_ARGS);
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
