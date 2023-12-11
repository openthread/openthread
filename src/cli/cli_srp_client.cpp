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

SrpClient::SrpClient(otInstance *aInstance, OutputImplementer &aOutputImplementer)
    : Output(aInstance, aOutputImplementer)
    , mCallbackEnabled(false)
{
    otSrpClientSetCallback(GetInstancePtr(), SrpClient::HandleCallback, this);
}

#if OPENTHREAD_CONFIG_SRP_CLIENT_AUTO_START_API_ENABLE

template <> otError SrpClient::Process<Cmd("autostart")>(Arg aArgs[])
{
    otError error = OT_ERROR_NONE;
    bool    enable;

    /**
     * @cli srp client autostart (get)
     * @code
     * srp client autostart
     * Disabled
     * Done
     * @endcode
     * @par
     * Indicates the current state of auto-start mode (enabled or disabled).
     * @sa otSrpClientIsAutoStartModeEnabled
     * @sa @srp
     */
    if (aArgs[0].IsEmpty())
    {
        OutputEnabledDisabledStatus(otSrpClientIsAutoStartModeEnabled(GetInstancePtr()));
        ExitNow();
    }

    SuccessOrExit(error = Interpreter::ParseEnableOrDisable(aArgs[0], enable));

    /**
     * @cli srp client autostart enable
     * @code
     * srp client autostart enable
     * Done
     * @endcode
     * @par
     * Enables auto-start mode.
     * @par
     * When auto-start is enabled, the SRP client monitors Thread
     * network data to discover SRP servers, to select the preferred
     * server, and to automatically start and stop the client when
     * an SRP server is detected.
     * @par
     * Three categories of network data entries indicate the presence of an SRP sever,
     * and are preferred in the following order:
     *  -# Unicast entries in which the server address is included in the service
     *  data. If there are multiple options, the option with the lowest numerical
     *  IPv6 address is preferred.
     *  -# Anycast entries that each have a sequence number. The largest sequence
     *  number as specified by Serial Number Arithmetic Logic
     *  in RFC-1982 is preferred.
     *  -# Unicast entries in which the server address information is included
     *  with the server data. If there are multiple options, the option with the
     *  lowest numerical IPv6 address is preferred.
     * @sa otSrpClientEnableAutoStartMode
     */
    if (enable)
    {
        otSrpClientEnableAutoStartMode(GetInstancePtr(), /* aCallback */ nullptr, /* aContext */ nullptr);
    }
    /**
     * @cli srp client autostart disable
     * @code
     * srp client autostart disable
     * Done
     * @endcode
     * @par
     * Disables the auto-start mode.
     * @par
     * Disabling auto-start mode does not stop a running client.
     * However, the SRP client stops monitoring Thread network data.
     * @sa otSrpClientDisableAutoStartMode
     */
    else
    {
        otSrpClientDisableAutoStartMode(GetInstancePtr());
    }

exit:
    return error;
}

#endif // OPENTHREAD_CONFIG_SRP_CLIENT_AUTO_START_API_ENABLE

/**
 * @cli srp client callback (get,enable,disable)
 * @code
 * srp client callback enable
 * Done
 * @endcode
 * @code
 * srp client callback
 * Enabled
 * Done
 * @endcode
 * @cparam srp client callback [@ca{enable}|@ca{disable}]
 * @par
 * Gets or enables/disables printing callback events from the SRP client.
 * @sa otSrpClientSetCallback
 * @sa @srp
 */
template <> otError SrpClient::Process<Cmd("callback")>(Arg aArgs[])
{
    otError error = OT_ERROR_NONE;

    if (aArgs[0].IsEmpty())
    {
        OutputEnabledDisabledStatus(mCallbackEnabled);
        ExitNow();
    }

    error = Interpreter::ParseEnableOrDisable(aArgs[0], mCallbackEnabled);

exit:
    return error;
}

template <> otError SrpClient::Process<Cmd("host")>(Arg aArgs[])
{
    otError error = OT_ERROR_NONE;

    /**
     * @cli srp client host
     * @code
     * srp client host
     * name:"dev4312", state:Registered, addrs:[fd00:0:0:0:0:0:0:1234, fd00:0:0:0:0:0:0:beef]
     * Done
     * @endcode
     * @par api_copy
     * #otSrpClientGetHostInfo
     */
    if (aArgs[0].IsEmpty())
    {
        OutputHostInfo(0, *otSrpClientGetHostInfo(GetInstancePtr()));
    }
    /**
     * @cli srp client host name (get,set)
     * @code
     * srp client host name dev4312
     * Done
     * @endcode
     * @code
     * srp client host name
     * dev4312
     * Done
     * @endcode
     * @cparam srp client host name [@ca{name}]
     * To set the client host name when the host has either been removed or not yet
     * registered with the server, use the `name` parameter.
     * @par
     * Gets or sets the host name of the SRP client.
     * @sa otSrpClientSetHostName
     * @sa @srp
     */
    else if (aArgs[0] == "name")
    {
        if (aArgs[1].IsEmpty())
        {
            const char *name = otSrpClientGetHostInfo(GetInstancePtr())->mName;
            OutputLine("%s", (name != nullptr) ? name : "(null)");
        }
        else
        {
            uint16_t len;
            uint16_t size;
            char    *hostName;

            VerifyOrExit(aArgs[2].IsEmpty(), error = OT_ERROR_INVALID_ARGS);
            hostName = otSrpClientBuffersGetHostNameString(GetInstancePtr(), &size);

            len = aArgs[1].GetLength();
            VerifyOrExit(len + 1 <= size, error = OT_ERROR_INVALID_ARGS);

            // We first make sure we can set the name, and if so
            // we copy it to the persisted string buffer and set
            // the host name again now with the persisted buffer.
            // This ensures that we do not overwrite a previous
            // buffer with a host name that cannot be set.

            SuccessOrExit(error = otSrpClientSetHostName(GetInstancePtr(), aArgs[1].GetCString()));
            memcpy(hostName, aArgs[1].GetCString(), len + 1);

            IgnoreError(otSrpClientSetHostName(GetInstancePtr(), hostName));
        }
    }
    /**
     * @cli srp client host state
     * @code
     * srp client host state
     * Registered
     * Done
     * @endcode
     * @par
     * Returns the state of the SRP client host. Possible states:
     *   * `ToAdd`: Item to be added/registered.
     *	 * `Adding`: Item is being added/registered.
     *   * `ToRefresh`: Item to be refreshed for lease renewal.
     *   * `Refreshing`: Item is beig refreshed.
     *   * `ToRemove`: Item to be removed.
     *   * `Removing`: Item is being removed.
     *   * `Registered`: Item is registered with server.
     *   * `Removed`: Item has been removed.
     */
    else if (aArgs[0] == "state")
    {
        VerifyOrExit(aArgs[1].IsEmpty(), error = OT_ERROR_INVALID_ARGS);
        OutputLine("%s", otSrpClientItemStateToString(otSrpClientGetHostInfo(GetInstancePtr())->mState));
    }
    else if (aArgs[0] == "address")
    {
        /**
         * @cli srp client host address (get)
         * @code
         * srp client host address
         * auto
         * Done
         * @endcode
         * @code
         * srp client host address
         * fd00:0:0:0:0:0:0:1234
         * fd00:0:0:0:0:0:0:beef
         * Done
         * @endcode
         * @par
         * Indicates whether auto address mode is enabled. If auto address mode is not
         * enabled, then the list of SRP client host addresses is returned.
         * @sa otSrpClientGetHostInfo
         * @sa @srp
         */
        if (aArgs[1].IsEmpty())
        {
            const otSrpClientHostInfo *hostInfo = otSrpClientGetHostInfo(GetInstancePtr());

            if (hostInfo->mAutoAddress)
            {
                OutputLine("auto");
            }
            else
            {
                for (uint8_t index = 0; index < hostInfo->mNumAddresses; index++)
                {
                    OutputIp6AddressLine(hostInfo->mAddresses[index]);
                }
            }
        }
        /**
         * @cli srp client host address (set)
         * @code
         * srp client host address auto
         * Done
         * @endcode
         * @code
         * srp client host address fd00::cafe
         * Done
         * @endcode
         * @cparam srp client host address [auto|@ca{address...}]
         *   * Use the `auto` parameter to enable auto host address mode.
         *     When enabled, the client automatically uses all preferred Thread
         *     `netif` unicast addresses except for link-local and mesh-local
         *     addresses. If there is no valid address, the mesh local
         *     EID address gets added. The SRP client automatically
         *     re-registers if addresses on the Thread `netif` are
         *     added or removed or marked as non-preferred.
         *   * Explicitly specify the list of host addresses, separating
         *     each address by a space. You can set this list while the client is
         *     running. This will also disable auto host address mode.
         * @par
         * Enable auto host address mode or explicitly set the list of host
         * addresses.
         * @sa otSrpClientEnableAutoHostAddress
         * @sa otSrpClientSetHostAddresses
         * @sa @srp
         */
        else if (aArgs[1] == "auto")
        {
            error = otSrpClientEnableAutoHostAddress(GetInstancePtr());
        }
        else
        {
            uint8_t       numAddresses = 0;
            otIp6Address  addresses[kMaxHostAddresses];
            uint8_t       arrayLength;
            otIp6Address *hostAddressArray;

            hostAddressArray = otSrpClientBuffersGetHostAddressesArray(GetInstancePtr(), &arrayLength);

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

            SuccessOrExit(error = otSrpClientSetHostAddresses(GetInstancePtr(), addresses, numAddresses));

            memcpy(hostAddressArray, addresses, numAddresses * sizeof(hostAddressArray[0]));
            IgnoreError(otSrpClientSetHostAddresses(GetInstancePtr(), hostAddressArray, numAddresses));
        }
    }
    /**
     * @cli srp client host remove
     * @code
     * srp client host remove 1
     * Done
     * @endcode
     * @cparam srp client host remove [@ca{removekeylease}] [@ca{sendunregtoserver}]
     *   * The parameter `removekeylease` is an optional boolean value that indicates
     *     whether the host key lease should also be removed (default is `false`).
     *   * The parameter `sendunregtoserver` is an optional boolean value that indicates
     *     whether the client host should send an "update" message to the server
     *     even when the client host information has not yet been registered with the
     *     server (default is `false`). This parameter can be specified only if the
     *     `removekeylease` parameter is specified first in the command.
     * @par
     * Removes SRP client host information and all services from the SRP server.
     * @sa otSrpClientRemoveHostAndServices
     * @sa otSrpClientSetHostName
     * @sa @srp
     */
    else if (aArgs[0] == "remove")
    {
        bool removeKeyLease    = false;
        bool sendUnregToServer = false;

        if (!aArgs[1].IsEmpty())
        {
            SuccessOrExit(error = aArgs[1].ParseAsBool(removeKeyLease));

            if (!aArgs[2].IsEmpty())
            {
                SuccessOrExit(error = aArgs[2].ParseAsBool(sendUnregToServer));
                VerifyOrExit(aArgs[3].IsEmpty(), error = OT_ERROR_INVALID_ARGS);
            }
        }

        error = otSrpClientRemoveHostAndServices(GetInstancePtr(), removeKeyLease, sendUnregToServer);
    }
    /**
     * @cli srp client host clear
     * @code
     * srp client host clear
     * Done
     * @endcode
     * @par
     * Clears all host information and all services.
     * @sa otSrpClientBuffersFreeAllServices
     * @sa otSrpClientClearHostAndServices
     */
    else if (aArgs[0] == "clear")
    {
        VerifyOrExit(aArgs[1].IsEmpty(), error = OT_ERROR_INVALID_ARGS);
        otSrpClientClearHostAndServices(GetInstancePtr());
        otSrpClientBuffersFreeAllServices(GetInstancePtr());
    }
    else
    {
        error = OT_ERROR_INVALID_COMMAND;
    }

exit:
    return error;
}

/**
 * @cli srp client leaseinterval (get,set)
 * @code
 * srp client leaseinterval 3600
 * Done
 * @endcode
 * @code
 * srp client leaseinterval
 * 3600
 * Done
 * @endcode
 * @cparam srp client leaseinterval [@ca{interval}]
 * @par
 * Gets or sets the lease interval in seconds.
 * @sa otSrpClientGetLeaseInterval
 * @sa otSrpClientSetLeaseInterval
 */
template <> otError SrpClient::Process<Cmd("leaseinterval")>(Arg aArgs[])
{
    return Interpreter::GetInterpreter().ProcessGetSet(aArgs, otSrpClientGetLeaseInterval, otSrpClientSetLeaseInterval);
}

/**
 * @cli srp client keyleaseinterval (get,set)
 * @code
 * srp client keyleaseinterval 864000
 * Done
 * @endcode
 * @code
 * srp client keyleaseinterval
 * 864000
 * Done
 * @endcode
 * @cparam srp client keyleaseinterval [@ca{interval}]
 * @par
 * Gets or sets the key lease interval in seconds.
 * @sa otSrpClientGetKeyLeaseInterval
 * @sa otSrpClientSetKeyLeaseInterval
 */
template <> otError SrpClient::Process<Cmd("keyleaseinterval")>(Arg aArgs[])
{
    return Interpreter::GetInterpreter().ProcessGetSet(aArgs, otSrpClientGetKeyLeaseInterval,
                                                       otSrpClientSetKeyLeaseInterval);
}

template <> otError SrpClient::Process<Cmd("server")>(Arg aArgs[])
{
    otError           error          = OT_ERROR_NONE;
    const otSockAddr *serverSockAddr = otSrpClientGetServerAddress(GetInstancePtr());

    /**
     * @cli srp client server
     * @code
     * srp client server
     * &lsqb;fd00:0:0:0:d88a:618b:384d:e760&rsqb;:4724
     * Done
     * @endcode
     * @par
     * Gets the socket address (IPv6 address and port number) of the SRP server
     * that is being used by the SRP client. If the client is not running, the address
     * is unspecified (all zeros) with a port number of 0.
     * @sa otSrpClientGetServerAddress
     * @sa @srp
     */
    if (aArgs[0].IsEmpty())
    {
        OutputSockAddrLine(*serverSockAddr);
        ExitNow();
    }

    VerifyOrExit(aArgs[1].IsEmpty(), error = OT_ERROR_INVALID_ARGS);

    /**
     * @cli srp client server address
     * @code
     * srp client server address
     * fd00:0:0:0:d88a:618b:384d:e760
     * Done
     * @endcode
     * @par
     * Returns the server's IPv6 address.
     */
    if (aArgs[0] == "address")
    {
        OutputIp6AddressLine(serverSockAddr->mAddress);
    }
    /**
     * @cli srp client server port
     * @code
     * srp client server port
     * 4724
     * Done
     * @endcode
     * @par
     * Returns the server's port number.
     */
    else if (aArgs[0] == "port")
    {
        OutputLine("%u", serverSockAddr->mPort);
    }
    else
    {
        error = OT_ERROR_INVALID_COMMAND;
    }

exit:
    return error;
}

template <> otError SrpClient::Process<Cmd("service")>(Arg aArgs[])
{
    otError error = OT_ERROR_NONE;
    bool    isRemove;

    /**
     * @cli srp client service
     * @code
     * srp client service
     * instance:"ins2", name:"_test2._udp,_sub1,_sub2", state:Registered, port:111, priority:1, weight:1
     * instance:"ins1", name:"_test1._udp", state:Registered, port:777, priority:0, weight:0
     * Done
     * @endcode
     * @par api_copy
     * #otSrpClientGetServices
     */
    if (aArgs[0].IsEmpty())
    {
        OutputServiceList(0, otSrpClientGetServices(GetInstancePtr()));
    }
    /**
     * @cli srp client service add
     * @code
     * srp client service add ins1 _test1._udp 777
     * Done
     * @endcode
     * @code
     * srp client service add ins2 _test2._udp,_sub1,_sub2 111 1 1
     * Done
     * @endcode
     * @cparam srp client service add @ca{instancename} @ca{servicename} <!--
     * * -->                          @ca{port} [@ca{priority}] <!--
     * * -->                          [@ca{weight}] [@ca{txt}]
     * The `servicename` parameter can optionally include a list of service subtype labels that are
     * separated by commas. The examples here use generic naming. The `priority` and `weight` (both are `uint16_t`
     * values) parameters are optional, and if not provided zero is used. The optional `txt` parameter sets the TXT data
     * associated with the service. The `txt` value must be in hex-string format and is treated as an already encoded
     * TXT data byte sequence.
     * @par
     * Adds a service with a given instance name, service name, and port number.
     * @sa otSrpClientAddService
     * @sa @srp
     */
    else if (aArgs[0] == "add")
    {
        error = ProcessServiceAdd(aArgs);
    }
    /**
     * @cli srp client service remove
     * @code
     * srp client service remove ins2 _test2._udp
     * Done
     * @endcode
     * @cparam srp client service remove @ca{instancename} @ca{servicename}
     * @par
     * Requests a service to be unregistered with the SRP server.
     * @sa otSrpClientRemoveService
     */
    /**
     * @cli srp client service name clear
     * @code
     * srp client service clear ins2 _test2._udp
     * Done
     * @endcode
     * @cparam srp client service clear @ca{instancename} @ca{servicename}
     * @par
     * Clears a service, immediately removing it from the client service list,
     * with no interaction with the SRP server.
     * @sa otSrpClientClearService
     */
    else if ((isRemove = (aArgs[0] == "remove")) || (aArgs[0] == "clear"))
    {
        // `remove`|`clear` <instance-name> <service-name>

        const otSrpClientService *service;

        VerifyOrExit(!aArgs[2].IsEmpty() && aArgs[3].IsEmpty(), error = OT_ERROR_INVALID_ARGS);

        for (service = otSrpClientGetServices(GetInstancePtr()); service != nullptr; service = service->mNext)
        {
            if ((aArgs[1] == service->mInstanceName) && (aArgs[2] == service->mName))
            {
                break;
            }
        }

        VerifyOrExit(service != nullptr, error = OT_ERROR_NOT_FOUND);

        if (isRemove)
        {
            error = otSrpClientRemoveService(GetInstancePtr(), const_cast<otSrpClientService *>(service));
        }
        else
        {
            SuccessOrExit(error = otSrpClientClearService(GetInstancePtr(), const_cast<otSrpClientService *>(service)));

            otSrpClientBuffersFreeService(GetInstancePtr(), reinterpret_cast<otSrpClientBuffersServiceEntry *>(
                                                                const_cast<otSrpClientService *>(service)));
        }
    }
#if OPENTHREAD_CONFIG_REFERENCE_DEVICE_ENABLE
    /**
     * @cli srp client service key (get,set)
     * @code
     * srp client service key enable
     * Done
     * @endcode
     * @code
     * srp client service key
     * Enabled
     * Done
     * @endcode
     * @par
     * Gets or sets the service key record inclusion mode in the SRP client.
     * This command is intended for testing only, and requires that
     * `OPENTHREAD_CONFIG_REFERENCE_DEVICE_ENABLE` be enabled.
     * @sa otSrpClientIsServiceKeyRecordEnabled
     * @sa @srp
     */
    else if (aArgs[0] == "key")
    {
        // `key [enable/disable]`

        error = Interpreter::GetInterpreter().ProcessEnableDisable(aArgs + 1, otSrpClientIsServiceKeyRecordEnabled,
                                                                   otSrpClientSetServiceKeyRecordEnabled);
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
    // `add` <instance-name> <service-name> <port> [priority] [weight] [txt] [lease] [key-lease]

    otSrpClientBuffersServiceEntry *entry = nullptr;
    uint16_t                        size;
    char                           *string;
    otError                         error;
    char                           *label;

    entry = otSrpClientBuffersAllocateService(GetInstancePtr());

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

    if (!aArgs[6].IsEmpty() && (aArgs[6] != "-"))
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

    if (!aArgs[7].IsEmpty())
    {
        SuccessOrExit(error = aArgs[7].ParseAsUint32(entry->mService.mLease));
    }

    if (!aArgs[8].IsEmpty())
    {
        SuccessOrExit(error = aArgs[8].ParseAsUint32(entry->mService.mKeyLease));
        VerifyOrExit(aArgs[9].IsEmpty(), error = OT_ERROR_INVALID_ARGS);
    }

    SuccessOrExit(error = otSrpClientAddService(GetInstancePtr(), &entry->mService));

    entry = nullptr;

exit:
    if (entry != nullptr)
    {
        otSrpClientBuffersFreeService(GetInstancePtr(), entry);
    }

    return error;
}

void SrpClient::OutputHostInfo(uint8_t aIndentSize, const otSrpClientHostInfo &aHostInfo)
{
    OutputFormat(aIndentSize, "name:");

    if (aHostInfo.mName != nullptr)
    {
        OutputFormat("\"%s\"", aHostInfo.mName);
    }
    else
    {
        OutputFormat("(null)");
    }

    OutputFormat(", state:%s, addrs:", otSrpClientItemStateToString(aHostInfo.mState));

    if (aHostInfo.mAutoAddress)
    {
        OutputLine("auto");
    }
    else
    {
        OutputFormat("[");

        for (uint8_t index = 0; index < aHostInfo.mNumAddresses; index++)
        {
            if (index > 0)
            {
                OutputFormat(", ");
            }

            OutputIp6Address(aHostInfo.mAddresses[index]);
        }

        OutputLine("]");
    }
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
    OutputFormat(aIndentSize, "instance:\"%s\", name:\"%s", aService.mInstanceName, aService.mName);

    if (aService.mSubTypeLabels != nullptr)
    {
        for (uint16_t index = 0; aService.mSubTypeLabels[index] != nullptr; index++)
        {
            OutputFormat(",%s", aService.mSubTypeLabels[index]);
        }
    }

    OutputLine("\", state:%s, port:%d, priority:%d, weight:%d", otSrpClientItemStateToString(aService.mState),
               aService.mPort, aService.mPriority, aService.mWeight);
}

/**
 * @cli srp client start
 * @code
 * srp client start fd00::d88a:618b:384d:e760 4724
 * Done
 * @endcode
 * @cparam srp client start @ca{serveraddr} @ca{serverport}
 * @par
 * Starts the SRP client operation.
 * @sa otSrpClientStart
 * @sa @srp
 */
template <> otError SrpClient::Process<Cmd("start")>(Arg aArgs[])
{
    otError    error = OT_ERROR_NONE;
    otSockAddr serverSockAddr;

    SuccessOrExit(error = aArgs[0].ParseAsIp6Address(serverSockAddr.mAddress));
    SuccessOrExit(error = aArgs[1].ParseAsUint16(serverSockAddr.mPort));
    VerifyOrExit(aArgs[2].IsEmpty(), error = OT_ERROR_INVALID_ARGS);

    error = otSrpClientStart(GetInstancePtr(), &serverSockAddr);

exit:
    return error;
}

/**
 * @cli srp client state
 * @code
 * srp client state
 * Enabled
 * Done
 * @endcode
 * @par api_copy
 * #otSrpClientIsRunning
 * @sa @srp
 */
template <> otError SrpClient::Process<Cmd("state")>(Arg aArgs[])
{
    otError error = OT_ERROR_NONE;

    VerifyOrExit(aArgs[0].IsEmpty(), error = OT_ERROR_INVALID_ARGS);

    OutputEnabledDisabledStatus(otSrpClientIsRunning(GetInstancePtr()));

exit:
    return error;
}

/**
 * @cli srp client stop
 * @code
 * srp client stop
 * Done
 * @endcode
 * @par api_copy
 * #otSrpClientStop
 */
template <> otError SrpClient::Process<Cmd("stop")>(Arg aArgs[])
{
    otError error = OT_ERROR_NONE;

    VerifyOrExit(aArgs[0].IsEmpty(), error = OT_ERROR_INVALID_ARGS);
    otSrpClientStop(GetInstancePtr());

exit:
    return error;
}

/**
 * @cli srp client ttl (get,set)
 * @code
 * srp client ttl 3600
 * Done
 * @endcode
 * @code
 * srp client ttl
 * 3600
 * Done
 * @endcode
 * @cparam srp client ttl [@ca{value}]
 * @par
 * Gets or sets the `ttl`(time to live) value in seconds.
 * @sa otSrpClientGetTtl
 * @sa otSrpClientSetTtl
 */
template <> otError SrpClient::Process<Cmd("ttl")>(Arg aArgs[])
{
    return Interpreter::GetInterpreter().ProcessGetSet(aArgs, otSrpClientGetTtl, otSrpClientSetTtl);
}

void SrpClient::HandleCallback(otError                    aError,
                               const otSrpClientHostInfo *aHostInfo,
                               const otSrpClientService  *aServices,
                               const otSrpClientService  *aRemovedServices,
                               void                      *aContext)
{
    static_cast<SrpClient *>(aContext)->HandleCallback(aError, aHostInfo, aServices, aRemovedServices);
}

void SrpClient::HandleCallback(otError                    aError,
                               const otSrpClientHostInfo *aHostInfo,
                               const otSrpClientService  *aServices,
                               const otSrpClientService  *aRemovedServices)
{
    otSrpClientService *next;

    if (mCallbackEnabled)
    {
        OutputLine("SRP client callback - error:%s", otThreadErrorToString(aError));
        OutputLine("Host info:");
        OutputHostInfo(kIndentSize, *aHostInfo);

        OutputLine("Service list:");
        OutputServiceList(kIndentSize, aServices);

        if (aRemovedServices != nullptr)
        {
            OutputLine("Removed service list:");
            OutputServiceList(kIndentSize, aRemovedServices);
        }
    }

    // Go through removed services and free all removed services

    for (const otSrpClientService *service = aRemovedServices; service != nullptr; service = next)
    {
        next = service->mNext;

        otSrpClientBuffersFreeService(GetInstancePtr(), reinterpret_cast<otSrpClientBuffersServiceEntry *>(
                                                            const_cast<otSrpClientService *>(service)));
    }
}

otError SrpClient::Process(Arg aArgs[])
{
#define CmdEntry(aCommandString)                                 \
    {                                                            \
        aCommandString, &SrpClient::Process<Cmd(aCommandString)> \
    }

    static constexpr Command kCommands[] = {
        CmdEntry("autostart"),     CmdEntry("callback"), CmdEntry("host"),    CmdEntry("keyleaseinterval"),
        CmdEntry("leaseinterval"), CmdEntry("server"),   CmdEntry("service"), CmdEntry("start"),
        CmdEntry("state"),         CmdEntry("stop"),     CmdEntry("ttl"),
    };

    static_assert(BinarySearch::IsSorted(kCommands), "kCommands is not sorted");

    otError        error = OT_ERROR_INVALID_COMMAND;
    const Command *command;

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

#endif // OPENTHREAD_CONFIG_SRP_CLIENT_ENABLE
