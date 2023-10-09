/*
 *  Copyright (c) 2023, The OpenThread Authors.
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
 *   This file implements CLI for DNS (client and server/resolver).
 */

#include "cli_dns.hpp"

#include "cli/cli.hpp"

#if OPENTHREAD_CLI_DNS_ENABLE

namespace ot {
namespace Cli {

#if OPENTHREAD_CONFIG_REFERENCE_DEVICE_ENABLE

template <> otError Dns::Process<Cmd("compression")>(Arg aArgs[])
{
    otError error = OT_ERROR_NONE;

    /**
     * @cli dns compression
     * @code
     * dns compression
     * Enabled
     * @endcode
     * @cparam dns compression [@ca{enable|disable}]
     * @par api_copy
     * #otDnsIsNameCompressionEnabled
     * @par
     * By default DNS name compression is enabled. When disabled,
     * DNS names are appended as full and never compressed. This
     * is applicable to OpenThread's DNS and SRP client/server
     * modules."
     * `OPENTHREAD_CONFIG_REFERENCE_DEVICE_ENABLE` is required.
     */
    if (aArgs[0].IsEmpty())
    {
        OutputEnabledDisabledStatus(otDnsIsNameCompressionEnabled());
    }
    /**
     * @cli dns compression (enable,disable)
     * @code
     * dns compression enable
     * Enabled
     * @endcode
     * @code
     * dns compression disable
     * Done
     * dns compression
     * Disabled
     * Done
     * @endcode
     * @cparam dns compression [@ca{enable|disable}]
     * @par
     * Set the "DNS name compression" mode.
     * @par
     * By default DNS name compression is enabled. When disabled,
     * DNS names are appended as full and never compressed. This
     * is applicable to OpenThread's DNS and SRP client/server
     * modules."
     * `OPENTHREAD_CONFIG_REFERENCE_DEVICE_ENABLE` is required.
     * @sa otDnsSetNameCompressionEnabled
     */
    else
    {
        bool enable;

        SuccessOrExit(error = Interpreter::ParseEnableOrDisable(aArgs[0], enable));
        otDnsSetNameCompressionEnabled(enable);
    }

exit:
    return error;
}

#endif // OPENTHREAD_CONFIG_REFERENCE_DEVICE_ENABLE

#if OPENTHREAD_CONFIG_DNS_CLIENT_ENABLE

template <> otError Dns::Process<Cmd("config")>(Arg aArgs[])
{
    otError error = OT_ERROR_NONE;

    /**
     * @cli dns config
     * @code
     * dns config
     * Server: [fd00:0:0:0:0:0:0:1]:1234
     * ResponseTimeout: 5000 ms
     * MaxTxAttempts: 2
     * RecursionDesired: no
     * ServiceMode: srv
     * Nat64Mode: allow
     * Done
     * @endcode
     * @par api_copy
     * #otDnsClientGetDefaultConfig
     * @par
     * The config includes the server IPv6 address and port, response
     * timeout in msec (wait time to rx response), maximum tx attempts
     * before reporting failure, boolean flag to indicate whether the server
     * can resolve the query recursively or not.
     * `OPENTHREAD_CONFIG_DNS_CLIENT_ENABLE` is required.
     */
    if (aArgs[0].IsEmpty())
    {
        const otDnsQueryConfig *defaultConfig = otDnsClientGetDefaultConfig(GetInstancePtr());

        OutputFormat("Server: ");
        OutputSockAddrLine(defaultConfig->mServerSockAddr);
        OutputLine("ResponseTimeout: %lu ms", ToUlong(defaultConfig->mResponseTimeout));
        OutputLine("MaxTxAttempts: %u", defaultConfig->mMaxTxAttempts);
        OutputLine("RecursionDesired: %s",
                   (defaultConfig->mRecursionFlag == OT_DNS_FLAG_RECURSION_DESIRED) ? "yes" : "no");
        OutputLine("ServiceMode: %s", DnsConfigServiceModeToString(defaultConfig->mServiceMode));
#if OPENTHREAD_CONFIG_DNS_CLIENT_NAT64_ENABLE
        OutputLine("Nat64Mode: %s", (defaultConfig->mNat64Mode == OT_DNS_NAT64_ALLOW) ? "allow" : "disallow");
#endif
#if OPENTHREAD_CONFIG_DNS_CLIENT_OVER_TCP_ENABLE
        OutputLine("TransportProtocol: %s", (defaultConfig->mTransportProto == OT_DNS_TRANSPORT_UDP) ? "udp" : "tcp");
#endif
    }
    /**
     * @cli dns config (set)
     * @code
     * dns config fd00::1 1234 5000 2 0
     * Done
     * @endcode
     * @code
     * dns config
     * Server: [fd00:0:0:0:0:0:0:1]:1234
     * ResponseTimeout: 5000 ms
     * MaxTxAttempts: 2
     * RecursionDesired: no
     * Done
     * @endcode
     * @code
     * dns config fd00::2
     * Done
     * @endcode
     * @code
     * dns config
     * Server: [fd00:0:0:0:0:0:0:2]:53
     * ResponseTimeout: 3000 ms
     * MaxTxAttempts: 3
     * RecursionDesired: yes
     * Done
     * @endcode
     * @par api_copy
     * #otDnsClientSetDefaultConfig
     * @cparam dns config [@ca{dns-server-IP}] [@ca{dns-server-port}] <!--
     * -->                [@ca{response-timeout-ms}] [@ca{max-tx-attempts}] <!--
     * -->                [@ca{recursion-desired-boolean}] [@ca{service-mode}]
     * @par
     * We can leave some of the fields as unspecified (or use value zero). The
     * unspecified fields are replaced by the corresponding OT config option
     * definitions `OPENTHREAD_CONFIG_DNS_CLIENT_DEFAULT` to form the default
     * query config.
     * `OPENTHREAD_CONFIG_DNS_CLIENT_ENABLE` is required.
     */
    else
    {
        otDnsQueryConfig  queryConfig;
        otDnsQueryConfig *config = &queryConfig;

        SuccessOrExit(error = GetDnsConfig(aArgs, config));
        otDnsClientSetDefaultConfig(GetInstancePtr(), config);
    }

exit:
    return error;
}

/**
 * @cli dns resolve
 * @code
 * dns resolve ipv6.google.com
 * DNS response for ipv6.google.com - 2a00:1450:401b:801:0:0:0:200e TTL: 300
 * @endcode
 * @code
 * dns resolve example.com 8.8.8.8
 * Synthesized IPv6 DNS server address: fdde:ad00:beef:2:0:0:808:808
 * DNS response for example.com. - fd4c:9574:3720:2:0:0:5db8:d822 TTL:20456
 * Done
 * @endcode
 * @cparam dns resolve @ca{hostname} [@ca{dns-server-IP}] <!--
 * -->                 [@ca{dns-server-port}] [@ca{response-timeout-ms}] <!--
 * -->                 [@ca{max-tx-attempts}] [@ca{recursion-desired-boolean}]
 * @par api_copy
 * #otDnsClientResolveAddress
 * @par
 * Send DNS Query to obtain IPv6 address for given hostname.
 * @par
 * The parameters after hostname are optional. Any unspecified (or zero) value
 * for these optional parameters is replaced by the value from the current default
 * config (dns config).
 * @par
 * The DNS server IP can be an IPv4 address, which will be synthesized to an
 * IPv6 address using the preferred NAT64 prefix from the network data.
 * @par
 * Note: The command will return InvalidState when the DNS server IP is an IPv4
 * address but the preferred NAT64 prefix is unavailable.
 * `OPENTHREAD_CONFIG_DNS_CLIENT_ENABLE` is required.
 */
template <> otError Dns::Process<Cmd("resolve")>(Arg aArgs[])
{
    otError           error = OT_ERROR_NONE;
    otDnsQueryConfig  queryConfig;
    otDnsQueryConfig *config = &queryConfig;

    VerifyOrExit(!aArgs[0].IsEmpty(), error = OT_ERROR_INVALID_ARGS);
    SuccessOrExit(error = GetDnsConfig(aArgs + 1, config));
    SuccessOrExit(error = otDnsClientResolveAddress(GetInstancePtr(), aArgs[0].GetCString(), &HandleDnsAddressResponse,
                                                    this, config));
    error = OT_ERROR_PENDING;

exit:
    return error;
}

#if OPENTHREAD_CONFIG_DNS_CLIENT_NAT64_ENABLE
template <> otError Dns::Process<Cmd("resolve4")>(Arg aArgs[])
{
    otError           error = OT_ERROR_NONE;
    otDnsQueryConfig  queryConfig;
    otDnsQueryConfig *config = &queryConfig;

    VerifyOrExit(!aArgs[0].IsEmpty(), error = OT_ERROR_INVALID_ARGS);
    SuccessOrExit(error = GetDnsConfig(aArgs + 1, config));
    SuccessOrExit(error = otDnsClientResolveIp4Address(GetInstancePtr(), aArgs[0].GetCString(),
                                                       &HandleDnsAddressResponse, this, config));
    error = OT_ERROR_PENDING;

exit:
    return error;
}
#endif

#if OPENTHREAD_CONFIG_DNS_CLIENT_SERVICE_DISCOVERY_ENABLE

/**
 * @cli dns browse
 * @code
 * dns browse _service._udp.example.com
 * DNS browse response for _service._udp.example.com.
 * inst1
 *     Port:1234, Priority:1, Weight:2, TTL:7200
 *     Host:host.example.com.
 *     HostAddress:fd00:0:0:0:0:0:0:abcd TTL:7200
 *     TXT:[a=6531, b=6c12] TTL:7300
 * instance2
 *     Port:1234, Priority:1, Weight:2, TTL:7200
 *     Host:host.example.com.
 *     HostAddress:fd00:0:0:0:0:0:0:abcd TTL:7200
 *     TXT:[a=1234] TTL:7300
 * Done
 * @endcode
 * @code
 * dns browse _airplay._tcp.default.service.arpa
 * DNS browse response for _airplay._tcp.default.service.arpa.
 * Mac mini
 *     Port:7000, Priority:0, Weight:0, TTL:10
 *     Host:Mac-mini.default.service.arpa.
 *     HostAddress:fd97:739d:386a:1:1c2e:d83c:fcbe:9cf4 TTL:10
 * Done
 * @endcode
 * @cparam dns browse @ca{service-name} [@ca{dns-server-IP}] [@ca{dns-server-port}] <!--
 * -->                [@ca{response-timeout-ms}] [@ca{max-tx-attempts}] <!--
 * -->                [@ca{recursion-desired-boolean}]
 * @sa otDnsClientBrowse
 * @par
 * Send a browse (service instance enumeration) DNS query to get the list of services for
 * given service-name
 * @par
 * The parameters after `service-name` are optional. Any unspecified (or zero) value
 * for these optional parameters is replaced by the value from the current default
 * config (`dns config`).
 * @par
 * Note: The DNS server IP can be an IPv4 address, which will be synthesized to an IPv6
 * address using the preferred NAT64 prefix from the network data. The command will return
 * `InvalidState` when the DNS server IP is an IPv4 address but the preferred NAT64 prefix
 * is unavailable. When testing DNS-SD discovery proxy, the zone is not `local` and
 * instead should be `default.service.arpa`.
 * `OPENTHREAD_CONFIG_DNS_CLIENT_SERVICE_DISCOVERY_ENABLE` is required.
 */
template <> otError Dns::Process<Cmd("browse")>(Arg aArgs[])
{
    otError           error = OT_ERROR_NONE;
    otDnsQueryConfig  queryConfig;
    otDnsQueryConfig *config = &queryConfig;

    VerifyOrExit(!aArgs[0].IsEmpty(), error = OT_ERROR_INVALID_ARGS);
    SuccessOrExit(error = GetDnsConfig(aArgs + 1, config));
    SuccessOrExit(
        error = otDnsClientBrowse(GetInstancePtr(), aArgs[0].GetCString(), &HandleDnsBrowseResponse, this, config));
    error = OT_ERROR_PENDING;

exit:
    return error;
}

/**
 * @cli dns service
 * @cparam dns service @ca{service-instance-label} @ca{service-name} <!--
 * -->                 [@ca{DNS-server-IP}] [@ca{DNS-server-port}] <!--
 * -->                 [@ca{response-timeout-ms}] [@ca{max-tx-attempts}] <!--
 * -->                 [@ca{recursion-desired-boolean}]
 * @par api_copy
 * #otDnsClientResolveService
 * @par
 * Send a service instance resolution DNS query for a given service instance.
 * Service instance label is provided first, followed by the service name
 * (note that service instance label can contain dot '.' character).
 * @par
 * The parameters after `service-name` are optional. Any unspecified (or zero)
 * value for these optional parameters is replaced by the value from the
 * current default config (`dns config`).
 * @par
 * Note: The DNS server IP can be an IPv4 address, which will be synthesized
 * to an IPv6 address using the preferred NAT64 prefix from the network data.
 * The command will return `InvalidState` when the DNS server IP is an IPv4
 * address but the preferred NAT64 prefix is unavailable.
 * `OPENTHREAD_CONFIG_DNS_CLIENT_SERVICE_DISCOVERY_ENABLE` is required.
 */
template <> otError Dns::Process<Cmd("service")>(Arg aArgs[])
{
    return ProcessService(aArgs, otDnsClientResolveService);
}

/**
 * @cli dns servicehost
 * @cparam dns servicehost @ca{service-instance-label} @ca{service-name} <!--
 * -->                 [@ca{DNS-server-IP}] [@ca{DNS-server-port}] <!--
 * -->                 [@ca{response-timeout-ms}] [@ca{max-tx-attempts}] <!--
 * -->                 [@ca{recursion-desired-boolean}]
 * @par api_copy
 * #otDnsClientResolveServiceAndHostAddress
 * @par
 * Send a service instance resolution DNS query for a given service instance
 * with potential follow-up host name resolution.
 * Service instance label is provided first, followed by the service name
 * (note that service instance label can contain dot '.' character).
 * @par
 * The parameters after `service-name` are optional. Any unspecified (or zero)
 * value for these optional parameters is replaced by the value from the
 * current default config (`dns config`).
 * @par
 * Note: The DNS server IP can be an IPv4 address, which will be synthesized
 * to an IPv6 address using the preferred NAT64 prefix from the network data.
 * The command will return `InvalidState` when the DNS server IP is an IPv4
 * address but the preferred NAT64 prefix is unavailable.
 * `OPENTHREAD_CONFIG_DNS_CLIENT_SERVICE_DISCOVERY_ENABLE` is required.
 */
template <> otError Dns::Process<Cmd("servicehost")>(Arg aArgs[])
{
    return ProcessService(aArgs, otDnsClientResolveServiceAndHostAddress);
}

otError Dns::ProcessService(Arg aArgs[], ResolveServiceFn aResolveServiceFn)
{
    otError           error = OT_ERROR_NONE;
    otDnsQueryConfig  queryConfig;
    otDnsQueryConfig *config = &queryConfig;

    VerifyOrExit(!aArgs[1].IsEmpty(), error = OT_ERROR_INVALID_ARGS);
    SuccessOrExit(error = GetDnsConfig(aArgs + 2, config));
    SuccessOrExit(error = aResolveServiceFn(GetInstancePtr(), aArgs[0].GetCString(), aArgs[1].GetCString(),
                                            &HandleDnsServiceResponse, this, config));
    error = OT_ERROR_PENDING;

exit:
    return error;
}

#endif // OPENTHREAD_CONFIG_DNS_CLIENT_SERVICE_DISCOVERY_ENABLE

//----------------------------------------------------------------------------------------------------------------------

void Dns::OutputResult(otError aError) { Interpreter::GetInterpreter().OutputResult(aError); }

otError Dns::GetDnsConfig(Arg aArgs[], otDnsQueryConfig *&aConfig)
{
    // This method gets the optional DNS config from `aArgs[]`.
    // The format: `[server IP address] [server port] [timeout]
    // [max tx attempt] [recursion desired] [service mode]
    // [transport]`

    otError error = OT_ERROR_NONE;
    bool    recursionDesired;
    bool    nat64SynthesizedAddress;

    memset(aConfig, 0, sizeof(otDnsQueryConfig));

    VerifyOrExit(!aArgs[0].IsEmpty(), aConfig = nullptr);

    SuccessOrExit(error = Interpreter::ParseToIp6Address(GetInstancePtr(), aArgs[0], aConfig->mServerSockAddr.mAddress,
                                                         nat64SynthesizedAddress));
    if (nat64SynthesizedAddress)
    {
        OutputFormat("Synthesized IPv6 DNS server address: ");
        OutputIp6AddressLine(aConfig->mServerSockAddr.mAddress);
    }

    VerifyOrExit(!aArgs[1].IsEmpty());
    SuccessOrExit(error = aArgs[1].ParseAsUint16(aConfig->mServerSockAddr.mPort));

    VerifyOrExit(!aArgs[2].IsEmpty());
    SuccessOrExit(error = aArgs[2].ParseAsUint32(aConfig->mResponseTimeout));

    VerifyOrExit(!aArgs[3].IsEmpty());
    SuccessOrExit(error = aArgs[3].ParseAsUint8(aConfig->mMaxTxAttempts));

    VerifyOrExit(!aArgs[4].IsEmpty());
    SuccessOrExit(error = aArgs[4].ParseAsBool(recursionDesired));
    aConfig->mRecursionFlag = recursionDesired ? OT_DNS_FLAG_RECURSION_DESIRED : OT_DNS_FLAG_NO_RECURSION;

    VerifyOrExit(!aArgs[5].IsEmpty());
    SuccessOrExit(error = ParseDnsServiceMode(aArgs[5], aConfig->mServiceMode));

    VerifyOrExit(!aArgs[6].IsEmpty());

    if (aArgs[6] == "tcp")
    {
        aConfig->mTransportProto = OT_DNS_TRANSPORT_TCP;
    }
    else if (aArgs[6] == "udp")
    {
        aConfig->mTransportProto = OT_DNS_TRANSPORT_UDP;
    }
    else
    {
        error = OT_ERROR_INVALID_ARGS;
    }

exit:
    return error;
}

const char *const Dns::kServiceModeStrings[] = {
    "unspec",      // OT_DNS_SERVICE_MODE_UNSPECIFIED      (0)
    "srv",         // OT_DNS_SERVICE_MODE_SRV              (1)
    "txt",         // OT_DNS_SERVICE_MODE_TXT              (2)
    "srv_txt",     // OT_DNS_SERVICE_MODE_SRV_TXT          (3)
    "srv_txt_sep", // OT_DNS_SERVICE_MODE_SRV_TXT_SEPARATE (4)
    "srv_txt_opt", // OT_DNS_SERVICE_MODE_SRV_TXT_OPTIMIZE (5)
};

static_assert(OT_DNS_SERVICE_MODE_UNSPECIFIED == 0, "OT_DNS_SERVICE_MODE_UNSPECIFIED value is incorrect");
static_assert(OT_DNS_SERVICE_MODE_SRV == 1, "OT_DNS_SERVICE_MODE_SRV value is incorrect");
static_assert(OT_DNS_SERVICE_MODE_TXT == 2, "OT_DNS_SERVICE_MODE_TXT value is incorrect");
static_assert(OT_DNS_SERVICE_MODE_SRV_TXT == 3, "OT_DNS_SERVICE_MODE_SRV_TXT value is incorrect");
static_assert(OT_DNS_SERVICE_MODE_SRV_TXT_SEPARATE == 4, "OT_DNS_SERVICE_MODE_SRV_TXT_SEPARATE value is incorrect");
static_assert(OT_DNS_SERVICE_MODE_SRV_TXT_OPTIMIZE == 5, "OT_DNS_SERVICE_MODE_SRV_TXT_OPTIMIZE value is incorrect");

const char *Dns::DnsConfigServiceModeToString(otDnsServiceMode aMode) const
{
    return Stringify(aMode, kServiceModeStrings);
}

otError Dns::ParseDnsServiceMode(const Arg &aArg, otDnsServiceMode &aMode) const
{
    otError error = OT_ERROR_NONE;

    if (aArg == "def")
    {
        aMode = OT_DNS_SERVICE_MODE_UNSPECIFIED;
        ExitNow();
    }

    for (size_t index = 0; index < OT_ARRAY_LENGTH(kServiceModeStrings); index++)
    {
        if (aArg == kServiceModeStrings[index])
        {
            aMode = static_cast<otDnsServiceMode>(index);
            ExitNow();
        }
    }

    error = OT_ERROR_INVALID_ARGS;

exit:
    return error;
}

void Dns::HandleDnsAddressResponse(otError aError, const otDnsAddressResponse *aResponse, void *aContext)
{
    static_cast<Dns *>(aContext)->HandleDnsAddressResponse(aError, aResponse);
}

void Dns::HandleDnsAddressResponse(otError aError, const otDnsAddressResponse *aResponse)
{
    char         hostName[OT_DNS_MAX_NAME_SIZE];
    otIp6Address address;
    uint32_t     ttl;

    IgnoreError(otDnsAddressResponseGetHostName(aResponse, hostName, sizeof(hostName)));

    OutputFormat("DNS response for %s - ", hostName);

    if (aError == OT_ERROR_NONE)
    {
        uint16_t index = 0;

        while (otDnsAddressResponseGetAddress(aResponse, index, &address, &ttl) == OT_ERROR_NONE)
        {
            OutputIp6Address(address);
            OutputFormat(" TTL:%lu ", ToUlong(ttl));
            index++;
        }
    }

    OutputNewLine();
    OutputResult(aError);
}

#if OPENTHREAD_CONFIG_DNS_CLIENT_SERVICE_DISCOVERY_ENABLE

void Dns::OutputDnsServiceInfo(uint8_t aIndentSize, const otDnsServiceInfo &aServiceInfo)
{
    OutputLine(aIndentSize, "Port:%d, Priority:%d, Weight:%d, TTL:%lu", aServiceInfo.mPort, aServiceInfo.mPriority,
               aServiceInfo.mWeight, ToUlong(aServiceInfo.mTtl));
    OutputLine(aIndentSize, "Host:%s", aServiceInfo.mHostNameBuffer);
    OutputFormat(aIndentSize, "HostAddress:");
    OutputIp6Address(aServiceInfo.mHostAddress);
    OutputLine(" TTL:%lu", ToUlong(aServiceInfo.mHostAddressTtl));
    OutputFormat(aIndentSize, "TXT:");

    if (!aServiceInfo.mTxtDataTruncated)
    {
        OutputDnsTxtData(aServiceInfo.mTxtData, aServiceInfo.mTxtDataSize);
    }
    else
    {
        OutputFormat("[");
        OutputBytes(aServiceInfo.mTxtData, aServiceInfo.mTxtDataSize);
        OutputFormat("...]");
    }

    OutputLine(" TTL:%lu", ToUlong(aServiceInfo.mTxtDataTtl));
}

void Dns::HandleDnsBrowseResponse(otError aError, const otDnsBrowseResponse *aResponse, void *aContext)
{
    static_cast<Dns *>(aContext)->HandleDnsBrowseResponse(aError, aResponse);
}

void Dns::HandleDnsBrowseResponse(otError aError, const otDnsBrowseResponse *aResponse)
{
    char             name[OT_DNS_MAX_NAME_SIZE];
    char             label[OT_DNS_MAX_LABEL_SIZE];
    uint8_t          txtBuffer[kMaxTxtDataSize];
    otDnsServiceInfo serviceInfo;

    IgnoreError(otDnsBrowseResponseGetServiceName(aResponse, name, sizeof(name)));

    OutputLine("DNS browse response for %s", name);

    if (aError == OT_ERROR_NONE)
    {
        uint16_t index = 0;

        while (otDnsBrowseResponseGetServiceInstance(aResponse, index, label, sizeof(label)) == OT_ERROR_NONE)
        {
            OutputLine("%s", label);
            index++;

            serviceInfo.mHostNameBuffer     = name;
            serviceInfo.mHostNameBufferSize = sizeof(name);
            serviceInfo.mTxtData            = txtBuffer;
            serviceInfo.mTxtDataSize        = sizeof(txtBuffer);

            if (otDnsBrowseResponseGetServiceInfo(aResponse, label, &serviceInfo) == OT_ERROR_NONE)
            {
                OutputDnsServiceInfo(kIndentSize, serviceInfo);
            }

            OutputNewLine();
        }
    }

    OutputResult(aError);
}

void Dns::HandleDnsServiceResponse(otError aError, const otDnsServiceResponse *aResponse, void *aContext)
{
    static_cast<Dns *>(aContext)->HandleDnsServiceResponse(aError, aResponse);
}

void Dns::HandleDnsServiceResponse(otError aError, const otDnsServiceResponse *aResponse)
{
    char             name[OT_DNS_MAX_NAME_SIZE];
    char             label[OT_DNS_MAX_LABEL_SIZE];
    uint8_t          txtBuffer[kMaxTxtDataSize];
    otDnsServiceInfo serviceInfo;

    IgnoreError(otDnsServiceResponseGetServiceName(aResponse, label, sizeof(label), name, sizeof(name)));

    OutputLine("DNS service resolution response for %s for service %s", label, name);

    if (aError == OT_ERROR_NONE)
    {
        serviceInfo.mHostNameBuffer     = name;
        serviceInfo.mHostNameBufferSize = sizeof(name);
        serviceInfo.mTxtData            = txtBuffer;
        serviceInfo.mTxtDataSize        = sizeof(txtBuffer);

        if (otDnsServiceResponseGetServiceInfo(aResponse, &serviceInfo) == OT_ERROR_NONE)
        {
            OutputDnsServiceInfo(/* aIndentSize */ 0, serviceInfo);
            OutputNewLine();
        }
    }

    OutputResult(aError);
}

#endif // OPENTHREAD_CONFIG_DNS_CLIENT_SERVICE_DISCOVERY_ENABLE
#endif // OPENTHREAD_CONFIG_DNS_CLIENT_ENABLE

#if OPENTHREAD_CONFIG_DNSSD_SERVER_ENABLE

template <> otError Dns::Process<Cmd("server")>(Arg aArgs[])
{
    otError error = OT_ERROR_NONE;

    if (aArgs[0].IsEmpty())
    {
        error = OT_ERROR_INVALID_ARGS;
    }
#if OPENTHREAD_CONFIG_DNS_UPSTREAM_QUERY_ENABLE
    /**
     * @cli dns server upstream
     * @code
     * dns server upstream
     * Enabled
     * Done
     * @endcode
     * @par api_copy
     * #otDnssdUpstreamQueryIsEnabled
     */
    else if (aArgs[0] == "upstream")
    {
        /**
         * @cli dns server upstream {enable|disable}
         * @code
         * dns server upstream enable
         * Done
         * @endcode
         * @cparam dns server upstream @ca{enable|disable}
         * @par api_copy
         * #otDnssdUpstreamQuerySetEnabled
         */
        error = Interpreter::GetInterpreter().ProcessEnableDisable(aArgs + 1, otDnssdUpstreamQueryIsEnabled,
                                                                   otDnssdUpstreamQuerySetEnabled);
    }
#endif // OPENTHREAD_CONFIG_DNS_UPSTREAM_QUERY_ENABLE
    else
    {
        ExitNow(error = OT_ERROR_INVALID_COMMAND);
    }

exit:
    return error;
}

#endif // OPENTHREAD_CONFIG_DNSSD_SERVER_ENABLE

otError Dns::Process(Arg aArgs[])
{
#define CmdEntry(aCommandString)                           \
    {                                                      \
        aCommandString, &Dns::Process<Cmd(aCommandString)> \
    }

    static constexpr Command kCommands[] = {

#if OPENTHREAD_CONFIG_DNS_CLIENT_ENABLE && OPENTHREAD_CONFIG_DNS_CLIENT_SERVICE_DISCOVERY_ENABLE
        CmdEntry("browse"),
#endif
#if OPENTHREAD_CONFIG_REFERENCE_DEVICE_ENABLE
        CmdEntry("compression"),
#endif
#if OPENTHREAD_CONFIG_DNS_CLIENT_ENABLE
        CmdEntry("config"),
        CmdEntry("resolve"),
#if OPENTHREAD_CONFIG_DNS_CLIENT_NAT64_ENABLE
        CmdEntry("resolve4"),
#endif
#endif // OPENTHREAD_CONFIG_DNS_CLIENT_ENABLE
#if OPENTHREAD_CONFIG_DNSSD_SERVER_ENABLE
        CmdEntry("server"),
#endif
#if OPENTHREAD_CONFIG_DNS_CLIENT_ENABLE && OPENTHREAD_CONFIG_DNS_CLIENT_SERVICE_DISCOVERY_ENABLE
        CmdEntry("service"),
        CmdEntry("servicehost"),
#endif
    };

#undef CmdEntry

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

#endif // OPENTHREAD_CLI_DNS_ENABLE
