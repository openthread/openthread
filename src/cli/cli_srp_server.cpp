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
 *   This file implements a simple CLI for the SRP server.
 */

#include "cli_srp_server.hpp"

#include <inttypes.h>

#include "cli/cli.hpp"
#include "common/string.hpp"

#if OPENTHREAD_CONFIG_SRP_SERVER_ENABLE

namespace ot {
namespace Cli {

/**
 * @cli srp server addrmode (get,set)
 * @code
 * srp server addrmode anycast
 * Done
 * @endcode
 * @code
 * srp server addrmode
 * anycast
 * Done
 * @endcode
 * @cparam srp server addrmode [@ca{anycast}|@ca{unicast}]
 * @par
 * Gets or sets the address mode used by the SRP server.
 * @par
 * The address mode tells the SRP server how to determine its address and port number,
 * which then get published in the Thread network data.
 * @sa otSrpServerGetAddressMode
 * @sa otSrpServerSetAddressMode
 */
template <> otError SrpServer::Process<Cmd("addrmode")>(Arg aArgs[])
{
    otError error = OT_ERROR_INVALID_ARGS;

    if (aArgs[0].IsEmpty())
    {
        switch (otSrpServerGetAddressMode(GetInstancePtr()))
        {
        case OT_SRP_SERVER_ADDRESS_MODE_UNICAST:
            OutputLine("unicast");
            break;

        case OT_SRP_SERVER_ADDRESS_MODE_ANYCAST:
            OutputLine("anycast");
            break;
        }

        error = OT_ERROR_NONE;
    }
    else if (aArgs[0] == "unicast")
    {
        error = otSrpServerSetAddressMode(GetInstancePtr(), OT_SRP_SERVER_ADDRESS_MODE_UNICAST);
    }
    else if (aArgs[0] == "anycast")
    {
        error = otSrpServerSetAddressMode(GetInstancePtr(), OT_SRP_SERVER_ADDRESS_MODE_ANYCAST);
    }

    return error;
}

#if OPENTHREAD_CONFIG_BORDER_ROUTING_ENABLE

/**
 * @cli srp server auto (enable,disable)
 * @code
 * srp server auto enable
 * Done
 * @endcode
 * @code
 * srp server auto
 * Enabled
 * Done
 * @endcode
 * @cparam srp server auto [@ca{enable}|@ca{disable}]
 * @par
 * Enables or disables the auto-enable mode on the SRP server.
 * @par
 * When this mode is enabled, the Border Routing Manager controls if and when
 * to enable or disable the SRP server.
 * @par
 * This command requires that `OPENTHREAD_CONFIG_BORDER_ROUTING_ENABLE` be enabled.
 * @moreinfo{@srp}.
 * @sa otSrpServerIsAutoEnableMode
 * @sa otSrpServerSetAutoEnableMode
 */
template <> otError SrpServer::Process<Cmd("auto")>(Arg aArgs[])
{
    return ProcessEnableDisable(aArgs, otSrpServerIsAutoEnableMode, otSrpServerSetAutoEnableMode);
}
#endif

/**
 * @cli srp server domain (get,set)
 * @code
 * srp server domain thread.service.arpa.
 * Done
 * @endcode
 * @code
 * srp server domain
 * thread.service.arpa.
 * Done
 * @endcode
 * @cparam srp server domain [@ca{domain-name}]
 * @par
 * Gets or sets the domain name of the SRP server.
 * @sa otSrpServerGetDomain
 * @sa otSrpServerSetDomain
 */
template <> otError SrpServer::Process<Cmd("domain")>(Arg aArgs[])
{
    otError error = OT_ERROR_NONE;

    if (aArgs[0].IsEmpty())
    {
        OutputLine("%s", otSrpServerGetDomain(GetInstancePtr()));
    }
    else
    {
        error = otSrpServerSetDomain(GetInstancePtr(), aArgs[0].GetCString());
    }

    return error;
}

/**
 * @cli srp server state
 * @code
 * srp server state
 * running
 * Done
 * @endcode
 * @par
 * Returns one of the following possible states of the SRP server:
 *  * `disabled`: The SRP server is not enabled.
 *  * `stopped`: The SRP server is enabled but not active due to existing
 *               SRP servers that are already active in the Thread network.
 *               The SRP server may become active when the existing
 *               SRP servers are no longer active within the Thread network.
 *  * `running`: The SRP server is active and can handle service registrations.
 * @par
 * @moreinfo{@srp}.
 * @sa otSrpServerGetState
 */
template <> otError SrpServer::Process<Cmd("state")>(Arg aArgs[])
{
    static const char *const kStateStrings[] = {
        "disabled", // (0) OT_SRP_SERVER_STATE_DISABLED
        "running",  // (1) OT_SRP_SERVER_STATE_RUNNING
        "stopped",  // (2) OT_SRP_SERVER_STATE_STOPPED
    };

    OT_UNUSED_VARIABLE(aArgs);

    static_assert(0 == OT_SRP_SERVER_STATE_DISABLED, "OT_SRP_SERVER_STATE_DISABLED value is incorrect");
    static_assert(1 == OT_SRP_SERVER_STATE_RUNNING, "OT_SRP_SERVER_STATE_RUNNING value is incorrect");
    static_assert(2 == OT_SRP_SERVER_STATE_STOPPED, "OT_SRP_SERVER_STATE_STOPPED value is incorrect");

    OutputLine("%s", Stringify(otSrpServerGetState(GetInstancePtr()), kStateStrings));

    return OT_ERROR_NONE;
}

template <> otError SrpServer::Process<Cmd("enable")>(Arg aArgs[])
{
    OT_UNUSED_VARIABLE(aArgs);

    otSrpServerSetEnabled(GetInstancePtr(), /* aEnabled */ true);

    return OT_ERROR_NONE;
}

/**
 * @cli srp server (enable,disable)
 * @code
 * srp server disable
 * Done
 * @endcode
 * @cparam srp server [@ca{enable}|@ca{disable}]
 * @par
 * Enables or disables the SRP server. @moreinfo{@srp}.
 * @sa otSrpServerSetEnabled
 */
template <> otError SrpServer::Process<Cmd("disable")>(Arg aArgs[])
{
    OT_UNUSED_VARIABLE(aArgs);

    otSrpServerSetEnabled(GetInstancePtr(), /* aEnabled */ false);

    return OT_ERROR_NONE;
}

template <> otError SrpServer::Process<Cmd("ttl")>(Arg aArgs[])
{
    otError              error = OT_ERROR_NONE;
    otSrpServerTtlConfig ttlConfig;

    if (aArgs[0].IsEmpty())
    {
        otSrpServerGetTtlConfig(GetInstancePtr(), &ttlConfig);
        OutputLine("min ttl: %lu", ToUlong(ttlConfig.mMinTtl));
        OutputLine("max ttl: %lu", ToUlong(ttlConfig.mMaxTtl));
    }
    else
    {
        SuccessOrExit(error = aArgs[0].ParseAsUint32(ttlConfig.mMinTtl));
        SuccessOrExit(error = aArgs[1].ParseAsUint32(ttlConfig.mMaxTtl));
        VerifyOrExit(aArgs[2].IsEmpty(), error = OT_ERROR_INVALID_ARGS);

        error = otSrpServerSetTtlConfig(GetInstancePtr(), &ttlConfig);
    }

exit:
    return error;
}

/**
 * @cli srp server lease (get,set)
 * @code
 * srp server lease 1800 7200 86400 1209600
 * Done
 * @endcode
 * @code
 * srp server lease
 * min lease: 1800
 * max lease: 7200
 * min key-lease: 86400
 * max key-lease: 1209600
 * Done
 * @endcode
 * @cparam srp server lease [@ca{min-lease} @ca{max-lease} @ca{min-key-lease} @ca{max-key-lease}]
 * @par
 * Gets or sets the SRP server lease values in number of seconds.
 * @sa otSrpServerGetLeaseConfig
 * @sa otSrpServerSetLeaseConfig
 */
template <> otError SrpServer::Process<Cmd("lease")>(Arg aArgs[])
{
    otError                error = OT_ERROR_NONE;
    otSrpServerLeaseConfig leaseConfig;

    if (aArgs[0].IsEmpty())
    {
        otSrpServerGetLeaseConfig(GetInstancePtr(), &leaseConfig);
        OutputLine("min lease: %lu", ToUlong(leaseConfig.mMinLease));
        OutputLine("max lease: %lu", ToUlong(leaseConfig.mMaxLease));
        OutputLine("min key-lease: %lu", ToUlong(leaseConfig.mMinKeyLease));
        OutputLine("max key-lease: %lu", ToUlong(leaseConfig.mMaxKeyLease));
    }
    else
    {
        SuccessOrExit(error = aArgs[0].ParseAsUint32(leaseConfig.mMinLease));
        SuccessOrExit(error = aArgs[1].ParseAsUint32(leaseConfig.mMaxLease));
        SuccessOrExit(error = aArgs[2].ParseAsUint32(leaseConfig.mMinKeyLease));
        SuccessOrExit(error = aArgs[3].ParseAsUint32(leaseConfig.mMaxKeyLease));
        VerifyOrExit(aArgs[4].IsEmpty(), error = OT_ERROR_INVALID_ARGS);

        error = otSrpServerSetLeaseConfig(GetInstancePtr(), &leaseConfig);
    }

exit:
    return error;
}

/**
 * @cli srp server host
 * @code
 * srp server host
 * srp-api-test-1.default.service.arpa.
 *     deleted: false
 *     addresses: [fdde:ad00:beef:0:0:ff:fe00:fc10]
 * srp-api-test-0.default.service.arpa.
 *     deleted: false
 *     addresses: [fdde:ad00:beef:0:0:ff:fe00:fc10]
 * Done
 * @endcode
 * @par
 * Returns information about all registered hosts. @moreinfo{@srp}.
 * @sa otSrpServerGetNextHost
 * @sa otSrpServerHostGetAddresses
 * @sa otSrpServerHostGetFullName
 */
template <> otError SrpServer::Process<Cmd("host")>(Arg aArgs[])
{
    otError                error = OT_ERROR_NONE;
    const otSrpServerHost *host;

    VerifyOrExit(aArgs[0].IsEmpty(), error = OT_ERROR_INVALID_ARGS);

    host = nullptr;
    while ((host = otSrpServerGetNextHost(GetInstancePtr(), host)) != nullptr)
    {
        const otIp6Address *addresses;
        uint8_t             addressesNum;
        bool                isDeleted = otSrpServerHostIsDeleted(host);

        OutputLine("%s", otSrpServerHostGetFullName(host));
        OutputLine(kIndentSize, "deleted: %s", isDeleted ? "true" : "false");
        if (isDeleted)
        {
            continue;
        }

        OutputSpaces(kIndentSize);
        OutputFormat("addresses: [");

        addresses = otSrpServerHostGetAddresses(host, &addressesNum);

        for (uint8_t i = 0; i < addressesNum; ++i)
        {
            OutputIp6Address(addresses[i]);
            if (i < addressesNum - 1)
            {
                OutputFormat(", ");
            }
        }

        OutputLine("]");
    }

exit:
    return error;
}

void SrpServer::OutputHostAddresses(const otSrpServerHost *aHost)
{
    const otIp6Address *addresses;
    uint8_t             addressesNum;

    addresses = otSrpServerHostGetAddresses(aHost, &addressesNum);

    OutputFormat("[");
    for (uint8_t i = 0; i < addressesNum; ++i)
    {
        if (i != 0)
        {
            OutputFormat(", ");
        }

        OutputIp6Address(addresses[i]);
    }
    OutputFormat("]");
}

/**
 * @cli srp server service
 * @code
 * srp server service
 * srp-api-test-1._ipps._tcp.default.service.arpa.
 *     deleted: false
 *     subtypes: (null)
 *     port: 49152
 *     priority: 0
 *     weight: 0
 *     ttl: 7200
 *     lease: 7200
 *     key-lease: 1209600
 *     TXT: [616263, xyz=585960]
 *     host: srp-api-test-1.default.service.arpa.
 *     addresses: [fdde:ad00:beef:0:0:ff:fe00:fc10]
 * srp-api-test-0._ipps._tcp.default.service.arpa.
 *     deleted: false
 *     subtypes: _sub1,_sub2
 *     port: 49152
 *     priority: 0
 *     weight: 0
 *     ttl: 3600
 *     lease: 3600
 *     key-lease: 1209600
 *     TXT: [616263, xyz=585960]
 *     host: srp-api-test-0.default.service.arpa.
 *     addresses: [fdde:ad00:beef:0:0:ff:fe00:fc10]
 * Done
 * @endcode
 * @par
 * Returns information about registered services.
 * @par
 * The `TXT` record is displayed
 * as an array of entries. If an entry contains a key, the key is printed in
 * ASCII format. The value portion is printed in hexadecimal bytes.
 * @moreinfo{@srp}.
 * @sa otSrpServerServiceGetInstanceName
 * @sa otSrpServerServiceGetServiceName
 * @sa otSrpServerServiceGetSubTypeServiceNameAt
 */
template <> otError SrpServer::Process<Cmd("service")>(Arg aArgs[])
{
    otError                error = OT_ERROR_NONE;
    const otSrpServerHost *host  = nullptr;

    VerifyOrExit(aArgs[0].IsEmpty(), error = OT_ERROR_INVALID_ARGS);

    while ((host = otSrpServerGetNextHost(GetInstancePtr(), host)) != nullptr)
    {
        const otSrpServerService *service = nullptr;

        while ((service = otSrpServerHostGetNextService(host, service)) != nullptr)
        {
            bool                 isDeleted = otSrpServerServiceIsDeleted(service);
            const uint8_t       *txtData;
            uint16_t             txtDataLength;
            bool                 hasSubType = false;
            otSrpServerLeaseInfo leaseInfo;

            OutputLine("%s", otSrpServerServiceGetInstanceName(service));
            OutputLine(kIndentSize, "deleted: %s", isDeleted ? "true" : "false");

            if (isDeleted)
            {
                continue;
            }

            otSrpServerServiceGetLeaseInfo(service, &leaseInfo);

            OutputFormat(kIndentSize, "subtypes: ");

            for (uint16_t index = 0;; index++)
            {
                char        subLabel[OT_DNS_MAX_LABEL_SIZE];
                const char *subTypeName = otSrpServerServiceGetSubTypeServiceNameAt(service, index);

                if (subTypeName == nullptr)
                {
                    break;
                }

                IgnoreError(otSrpServerParseSubTypeServiceName(subTypeName, subLabel, sizeof(subLabel)));
                OutputFormat("%s%s", hasSubType ? "," : "", subLabel);
                hasSubType = true;
            }

            OutputLine(hasSubType ? "" : "(null)");

            OutputLine(kIndentSize, "port: %u", otSrpServerServiceGetPort(service));
            OutputLine(kIndentSize, "priority: %u", otSrpServerServiceGetPriority(service));
            OutputLine(kIndentSize, "weight: %u", otSrpServerServiceGetWeight(service));
            OutputLine(kIndentSize, "ttl: %lu", ToUlong(otSrpServerServiceGetTtl(service)));
            OutputLine(kIndentSize, "lease: %lu", ToUlong(leaseInfo.mLease / 1000));
            OutputLine(kIndentSize, "key-lease: %lu", ToUlong(leaseInfo.mKeyLease / 1000));

            txtData = otSrpServerServiceGetTxtData(service, &txtDataLength);
            OutputFormat(kIndentSize, "TXT: ");
            OutputDnsTxtData(txtData, txtDataLength);
            OutputNewLine();

            OutputLine(kIndentSize, "host: %s", otSrpServerHostGetFullName(host));

            OutputFormat(kIndentSize, "addresses: ");
            OutputHostAddresses(host);
            OutputNewLine();
        }
    }

exit:
    return error;
}

/**
 * @cli srp server seqnum (get,set)
 * @code
 * srp server seqnum 20
 * Done
 * @endcode
 * @code
 * srp server seqnum
 * 20
 * Done
 * @endcode
 * @cparam srp server seqnum [@ca{seqnum}]
 * @par
 * Gets or sets the sequence number used with the anycast address mode.
 * The sequence number is included in the "DNS/SRP Service Anycast Address"
 * entry that is published in the Network Data.
 * @sa otSrpServerGetAnycastModeSequenceNumber
 * @sa otSrpServerSetAnycastModeSequenceNumber
 */
template <> otError SrpServer::Process<Cmd("seqnum")>(Arg aArgs[])
{
    otError error = OT_ERROR_NONE;

    if (aArgs[0].IsEmpty())
    {
        OutputLine("%u", otSrpServerGetAnycastModeSequenceNumber(GetInstancePtr()));
    }
    else
    {
        uint8_t sequenceNumber;

        SuccessOrExit(error = aArgs[0].ParseAsUint8(sequenceNumber));
        error = otSrpServerSetAnycastModeSequenceNumber(GetInstancePtr(), sequenceNumber);
    }

exit:
    return error;
}

otError SrpServer::Process(Arg aArgs[])
{
#define CmdEntry(aCommandString)                                 \
    {                                                            \
        aCommandString, &SrpServer::Process<Cmd(aCommandString)> \
    }

    static constexpr Command kCommands[] = {
        CmdEntry("addrmode"),
#if OPENTHREAD_CONFIG_BORDER_ROUTING_ENABLE
        CmdEntry("auto"),
#endif
        CmdEntry("disable"),
        CmdEntry("domain"),
        CmdEntry("enable"),
        CmdEntry("host"),
        CmdEntry("lease"),
        CmdEntry("seqnum"),
        CmdEntry("service"),
        CmdEntry("state"),
        CmdEntry("ttl"),
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

#endif // OPENTHREAD_CONFIG_SRP_SERVER_ENABLE
