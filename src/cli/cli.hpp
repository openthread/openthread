/*
 *  Copyright (c) 2016, The OpenThread Authors.
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
 *   This file contains definitions for the CLI interpreter.
 */

#ifndef CLI_HPP_
#define CLI_HPP_

#include "openthread-core-config.h"

#include "cli_config.h"

#include <stdarg.h>

#include <openthread/cli.h>
#include <openthread/dns_client.h>
#include <openthread/ip6.h>
#include <openthread/sntp.h>
#include <openthread/udp.h>

#include "cli/cli_commissioner.hpp"
#include "cli/cli_dataset.hpp"
#include "cli/cli_joiner.hpp"
#include "cli/cli_network_data.hpp"
#include "cli/cli_srp_client.hpp"
#include "cli/cli_srp_server.hpp"
#include "cli/cli_udp.hpp"
#if OPENTHREAD_CONFIG_COAP_API_ENABLE
#include "cli/cli_coap.hpp"
#endif
#if OPENTHREAD_CONFIG_COAP_SECURE_API_ENABLE
#include "cli/cli_coap_secure.hpp"
#endif
#include "common/code_utils.hpp"
#include "common/instance.hpp"
#include "common/timer.hpp"
#include "net/icmp6.hpp"
#include "utils/lookup_table.hpp"

namespace ot {

/**
 * @namespace ot::Cli
 *
 * @brief
 *   This namespace contains definitions for the CLI interpreter.
 *
 */
namespace Cli {

/**
 * This class implements the CLI interpreter.
 *
 */
class Interpreter
{
    friend class Coap;
    friend class CoapSecure;
    friend class Commissioner;
    friend class Dataset;
    friend class Joiner;
    friend class NetworkData;
    friend class SrpClient;
    friend class SrpServer;
    friend class UdpExample;

public:
    /**
     * Constructor
     *
     * @param[in]  aInstance  The OpenThread instance structure.
     */
    explicit Interpreter(Instance *aInstance);

    /**
     * This method returns a reference to the interpreter object.
     *
     * @returns A reference to the interpreter object.
     *
     */
    static Interpreter &GetInterpreter(void)
    {
        OT_ASSERT(sInterpreter != nullptr);

        return *sInterpreter;
    }

    /**
     * This method returns whether the interpreter is initialized.
     *
     * @returns  Whether the interpreter is initialized.
     *
     */
    static bool IsInitialized(void) { return sInterpreter != nullptr; }

    /**
     * This method interprets a CLI command.
     *
     * @param[in]  aBuf        A pointer to a string.
     * @param[in]  aBufLength  The length of the string in bytes.
     *
     */
    void ProcessLine(char *aBuf, uint16_t aBufLength);

    /**
     * This method delivers raw characters to the client.
     *
     * @param[in]  aBuf        A pointer to a buffer.
     * @param[in]  aBufLength  Number of bytes in the buffer.
     *
     * @returns The number of bytes placed in the output queue.
     *
     * @retval  -1  Driver is broken.
     *
     */
    int Output(const char *aBuf, uint16_t aBufLength);

    /**
     * This method writes a number of bytes to the CLI console as a hex string.
     *
     * @param[in]  aBytes   A pointer to data which should be printed.
     * @param[in]  aLength  @p aBytes length.
     *
     */
    void OutputBytes(const uint8_t *aBytes, uint16_t aLength);

    /**
     * This method writes a number of bytes to the CLI console as a hex string.
     *
     * @tparam kBytesLength   The length of @p aBytes array.
     *
     * @param[in]  aBytes     A array of @p kBytesLength bytes which should be printed.
     *
     */
    template <uint8_t kBytesLength> void OutputBytes(const uint8_t (&aBytes)[kBytesLength])
    {
        OutputBytes(aBytes, kBytesLength);
    }

    /**
     * This method delivers formatted output to the client.
     *
     * @param[in]  aFormat  A pointer to the format string.
     * @param[in]  ...      A variable list of arguments to format.
     *
     * @returns The number of bytes placed in the output queue.
     *
     * @retval  -1  Driver is broken.
     *
     */
    int OutputFormat(const char *aFormat, ...);

    /**
     * This method delivers formatted output to the client.
     *
     * @param[in]  aFormat      A pointer to the format string.
     * @param[in]  aArguments   A variable list of arguments for format.
     *
     * @returns The number of bytes placed in the output queue.
     *
     */
    int OutputFormatV(const char *aFormat, va_list aArguments);

    /**
     * This method delivers formatted output (to which it prepends a given number indentation space chars) to the
     * client.
     *
     * @param[in]  aIndentSize   Number of indentation space chars to prepend to the string.
     * @param[in]  aFormat       A pointer to the format string.
     * @param[in]  ...           A variable list of arguments to format.
     *
     */
    void OutputFormat(uint8_t aIndentSize, const char *aFormat, ...);

    /**
     * This method delivers formatted output (to which it also appends newline `\r\n`) to the client.
     *
     * @param[in]  aFormat  A pointer to the format string.
     * @param[in]  ...      A variable list of arguments to format.
     *
     */
    void OutputLine(const char *aFormat, ...);

    /**
     * This method delivers formatted output (to which it prepends a given number indentation space chars and appends
     * newline `\r\n`) to the client.
     *
     * @param[in]  aIndentSize   Number of indentation space chars to prepend to the string.
     * @param[in]  aFormat       A pointer to the format string.
     * @param[in]  ...           A variable list of arguments to format.
     *
     */
    void OutputLine(uint8_t aIndentSize, const char *aFormat, ...);

    /**
     * This method writes a given number of space chars to the CLI console.
     *
     * @param[in] aCount  Number of space chars to output.
     *
     */
    void OutputSpaces(uint8_t aCount);

    /**
     * This method writes an Extended MAC Address to the CLI console.
     *
     * param[in] aExtAddress  The Extended MAC Address to output.
     *
     */
    void OutputExtAddress(const otExtAddress &aExtAddress) { OutputBytes(aExtAddress.m8); }

    /**
     * Write an IPv6 address to the CLI console.
     *
     * @param[in]  aAddress  A reference to the IPv6 address.
     *
     * @returns The number of bytes placed in the output queue.
     *
     * @retval  -1  Driver is broken.
     *
     */
    int OutputIp6Address(const otIp6Address &aAddress);

    /**
     * This method delivers a success or error message the client.
     *
     * If the @p aError is `OT_ERROR_PENDING` nothing will be outputted.
     *
     * @param[in]  aError  The error code.
     *
     */
    void OutputResult(otError aError);

    /**
     * This method delivers "Enabled" or "Disabled" status to the CLI client (it also appends newline `\r\n`).
     *
     * @param[in] aEnabled  A boolean indicating the status. TRUE outputs "Enabled", FALSE outputs "Disabled".
     *
     */
    void OutputEnabledDisabledStatus(bool aEnabled);

    /**
     * This method sets the user command table.
     *
     * @param[in]  aUserCommands  A pointer to an array with user commands.
     * @param[in]  aLength        @p aUserCommands length.
     * @param[in]  aContext       @p aUserCommands length.
     *
     */
    void SetUserCommands(const otCliCommand *aCommands, uint8_t aLength, void *aContext);

protected:
    static Interpreter *sInterpreter;

private:
    enum
    {
        kIndentSize       = 4,
        kMaxArgs          = 32,
        kMaxAutoAddresses = 8,

        kDefaultPingInterval = 1000, // (in mses)
        kDefaultPingLength   = 8,    // (in bytes)
        kDefaultPingCount    = 1,

        kMaxLineLength = OPENTHREAD_CONFIG_CLI_MAX_LINE_LENGTH,
    };

    struct Command
    {
        const char *mName;
        otError (Interpreter::*mHandler)(uint8_t aArgsLength, char *aArgs[]);
    };

    otError        ParsePingInterval(const char *aString, uint32_t &aInterval);
    static otError ParseJoinerDiscerner(char *aString, otJoinerDiscerner &aDiscerner);

    otError ProcessHelp(uint8_t aArgsLength, char *aArgs[]);
    otError ProcessCcaThreshold(uint8_t aArgsLength, char *aArgs[]);
    otError ProcessBufferInfo(uint8_t aArgsLength, char *aArgs[]);
    otError ProcessChannel(uint8_t aArgsLength, char *aArgs[]);
#if OPENTHREAD_CONFIG_BORDER_ROUTING_ENABLE
    otError ProcessBorderRouting(uint8_t aArgsLength, char *aArgs[]);
#endif
#if (OPENTHREAD_CONFIG_THREAD_VERSION >= OT_THREAD_VERSION_1_2)
    otError ProcessBackboneRouter(uint8_t aArgsLength, char *aArgs[]);

#if OPENTHREAD_FTD && OPENTHREAD_CONFIG_BACKBONE_ROUTER_ENABLE
    otError ProcessBackboneRouterLocal(uint8_t aArgsLength, char *aArgs[]);
#if OPENTHREAD_CONFIG_REFERENCE_DEVICE_ENABLE && OPENTHREAD_CONFIG_BACKBONE_ROUTER_MULTICAST_ROUTING_ENABLE
    otError ProcessBackboneRouterMgmtMlr(uint8_t aArgsLength, char *aArgs[]);
    void    PrintMulticastListenersTable(void);
#endif
#endif

    otError ProcessDomainName(uint8_t aArgsLength, char *aArgs[]);

#if OPENTHREAD_CONFIG_DUA_ENABLE
    otError ProcessDua(uint8_t aArgsLength, char *aArgs[]);
#endif

#endif // (OPENTHREAD_CONFIG_THREAD_VERSION >= OT_THREAD_VERSION_1_2)

#if OPENTHREAD_FTD
    otError ProcessChild(uint8_t aArgsLength, char *aArgs[]);
    otError ProcessChildIp(uint8_t aArgsLength, char *aArgs[]);
    otError ProcessChildMax(uint8_t aArgsLength, char *aArgs[]);
#endif
#if OPENTHREAD_CONFIG_CHILD_SUPERVISION_ENABLE
    otError ProcessChildSupervision(uint8_t aArgsLength, char *aArgs[]);
#endif
    otError ProcessChildTimeout(uint8_t aArgsLength, char *aArgs[]);
#if OPENTHREAD_CONFIG_COAP_API_ENABLE
    otError ProcessCoap(uint8_t aArgsLength, char *aArgs[]);
#endif
#if OPENTHREAD_CONFIG_COAP_SECURE_API_ENABLE
    otError ProcessCoapSecure(uint8_t aArgsLength, char *aArgs[]);
#endif
#if OPENTHREAD_CONFIG_PLATFORM_RADIO_COEX_ENABLE
    otError ProcessCoexMetrics(uint8_t aArgsLength, char *aArgs[]);
#endif
#if OPENTHREAD_CONFIG_COMMISSIONER_ENABLE && OPENTHREAD_FTD
    otError ProcessCommissioner(uint8_t aArgsLength, char *aArgs[]);
#endif
#if OPENTHREAD_FTD
    otError ProcessContextIdReuseDelay(uint8_t aArgsLength, char *aArgs[]);
#endif
    otError ProcessCounters(uint8_t aArgsLength, char *aArgs[]);
    otError ProcessCsl(uint8_t aArgsLength, char *aArgs[]);
#if OPENTHREAD_FTD
    otError ProcessDelayTimerMin(uint8_t aArgsLength, char *aArgs[]);
#endif
#if OPENTHREAD_CONFIG_DIAG_ENABLE
    otError ProcessDiag(uint8_t aArgsLength, char *aArgs[]);
#endif
    otError ProcessDiscover(uint8_t aArgsLength, char *aArgs[]);
#if OPENTHREAD_CONFIG_DNS_CLIENT_ENABLE
    otError ProcessDns(uint8_t aArgsLength, char *aArgs[]);
#endif
#if OPENTHREAD_FTD
    otError ProcessEidCache(uint8_t aArgsLength, char *aArgs[]);
#endif
    otError ProcessEui64(uint8_t aArgsLength, char *aArgs[]);
#if OPENTHREAD_POSIX
    otError ProcessExit(uint8_t aArgsLength, char *aArgs[]);
#endif
    otError ProcessLog(uint8_t aArgsLength, char *aArgs[]);
    otError ProcessExtAddress(uint8_t aArgsLength, char *aArgs[]);
    otError ProcessExtPanId(uint8_t aArgsLength, char *aArgs[]);
    otError ProcessFactoryReset(uint8_t aArgsLength, char *aArgs[]);
#if OPENTHREAD_FTD && OPENTHREAD_CONFIG_REFERENCE_DEVICE_ENABLE
    otError ProcessFake(uint8_t aArgsLength, char *aArgs[]);
#endif
    otError ProcessFem(uint8_t aArgsLength, char *aArgs[]);
    otError ProcessIfconfig(uint8_t aArgsLength, char *aArgs[]);
    otError ProcessIpAddr(uint8_t aArgsLength, char *aArgs[]);
    otError ProcessIpAddrAdd(uint8_t aArgsLength, char *aArgs[]);
    otError ProcessIpAddrDel(uint8_t aArgsLength, char *aArgs[]);
    otError ProcessIpMulticastAddr(uint8_t aArgsLength, char *aArgs[]);
    otError ProcessIpMulticastAddrAdd(uint8_t aArgsLength, char *aArgs[]);
    otError ProcessIpMulticastAddrDel(uint8_t aArgsLength, char *aArgs[]);
    otError ProcessMulticastPromiscuous(uint8_t aArgsLength, char *aArgs[]);
#if OPENTHREAD_CONFIG_JOINER_ENABLE
    otError ProcessJoiner(uint8_t aArgsLength, char *aArgs[]);
#endif
#if OPENTHREAD_FTD
    otError ProcessJoinerPort(uint8_t aArgsLength, char *aArgs[]);
#endif
    otError ProcessKeySequence(uint8_t aArgsLength, char *aArgs[]);
    otError ProcessLeaderData(uint8_t aArgsLength, char *aArgs[]);
#if OPENTHREAD_FTD
    otError ProcessPartitionId(uint8_t aArgsLength, char *aArgs[]);
    otError ProcessLeaderWeight(uint8_t aArgsLength, char *aArgs[]);
#endif
    otError ProcessMasterKey(uint8_t aArgsLength, char *aArgs[]);
#if OPENTHREAD_CONFIG_MLE_LINK_METRICS_ENABLE
    otError ProcessLinkMetrics(uint8_t aArgsLength, char *aArgs[]);
    otError ProcessLinkMetricsQuery(uint8_t aArgsLength, char *aArgs[]);
    otError ProcessLinkMetricsMgmt(uint8_t aArgsLength, char *aArgs[]);
    otError ProcessLinkMetricsProbe(uint8_t aArgsLength, char *aArgs[]);

    otError ParseLinkMetricsFlags(otLinkMetrics &aLinkMetrics, char *aFlags);
#endif
#if OPENTHREAD_FTD && OPENTHREAD_CONFIG_TMF_PROXY_MLR_ENABLE && OPENTHREAD_CONFIG_COMMISSIONER_ENABLE
    otError ProcessMlr(uint8_t aArgsLength, char *aArgs[]);

    otError ProcessMlrReg(uint8_t aArgsLength, char *aArgs[]);

    static void HandleMlrRegResult(void *              aContext,
                                   otError             aError,
                                   uint8_t             aMlrStatus,
                                   const otIp6Address *aFailedAddresses,
                                   uint8_t             aFailedAddressNum);
    void        HandleMlrRegResult(otError             aError,
                                   uint8_t             aMlrStatus,
                                   const otIp6Address *aFailedAddresses,
                                   uint8_t             aFailedAddressNum);
#endif
    otError ProcessMode(uint8_t aArgsLength, char *aArgs[]);
#if OPENTHREAD_FTD
    otError ProcessNeighbor(uint8_t aArgsLength, char *aArgs[]);
#endif
    otError ProcessNetworkData(uint8_t aArgsLength, char *aArgs[]);
    otError ProcessNetworkDataPrefix(void);
    otError ProcessNetworkDataRoute(void);
    otError ProcessNetworkDataService(void);
    void    OutputPrefix(const otBorderRouterConfig &aConfig);
    void    OutputRoute(const otExternalRouteConfig &aConfig);
    void    OutputService(const otServiceConfig &aConfig);

#if OPENTHREAD_CONFIG_PLATFORM_NETIF_ENABLE
    otError ProcessNetif(uint8_t aArgsLength, char *aArgs[]);
#endif
    otError ProcessNetstat(uint8_t aArgsLength, char *aArgs[]);
    int     OutputSocketAddress(const otSockAddr &aAddress);
#if OPENTHREAD_CONFIG_TMF_NETDATA_SERVICE_ENABLE
    otError ProcessService(uint8_t aArgsLength, char *aArgs[]);
    otError ProcessServiceList(void);
#endif
#if OPENTHREAD_FTD || OPENTHREAD_CONFIG_TMF_NETWORK_DIAG_MTD_ENABLE
    otError ProcessNetworkDiagnostic(uint8_t aArgsLength, char *aArgs[]);
#endif
#if OPENTHREAD_FTD
    otError ProcessNetworkIdTimeout(uint8_t aArgsLength, char *aArgs[]);
#endif
    otError ProcessNetworkName(uint8_t aArgsLength, char *aArgs[]);
#if OPENTHREAD_CONFIG_TIME_SYNC_ENABLE
    otError ProcessNetworkTime(uint8_t aArgsLength, char *aArgs[]);
#endif
    otError ProcessPanId(uint8_t aArgsLength, char *aArgs[]);
    otError ProcessParent(uint8_t aArgsLength, char *aArgs[]);
#if OPENTHREAD_FTD
    otError ProcessParentPriority(uint8_t aArgsLength, char *aArgs[]);
#endif
    otError ProcessPing(uint8_t aArgsLength, char *aArgs[]);
    otError ProcessPollPeriod(uint8_t aArgsLength, char *aArgs[]);
    void    SignalPingRequest(const Ip6::Address &aPeerAddress,
                              uint16_t            aPingLength,
                              uint32_t            aTimestamp,
                              uint8_t             aHopLimit);
    void    SignalPingReply(const Ip6::Address &aPeerAddress,
                            uint16_t            aPingLength,
                            uint32_t            aTimestamp,
                            uint8_t             aHopLimit);

#if OPENTHREAD_CONFIG_BORDER_ROUTER_ENABLE
    otError ProcessPrefix(uint8_t aArgsLength, char *aArgs[]);
    otError ProcessPrefixAdd(uint8_t aArgsLength, char *aArgs[]);
    otError ProcessPrefixRemove(uint8_t aArgsLength, char *aArgs[]);
    otError ProcessPrefixList(void);
#endif
    otError ProcessPromiscuous(uint8_t aArgsLength, char *aArgs[]);
#if OPENTHREAD_FTD
    otError ProcessPreferRouterId(uint8_t aArgsLength, char *aArgs[]);
    otError ProcessPskc(uint8_t aArgsLength, char *aArgs[]);
#endif
    otError ProcessRcp(uint8_t aArgsLength, char *aArgs[]);
    otError ProcessRegion(uint8_t aArgsLength, char *aArgs[]);
#if OPENTHREAD_FTD
    otError ProcessReleaseRouterId(uint8_t aArgsLength, char *aArgs[]);
#endif
    otError ProcessReset(uint8_t aArgsLength, char *aArgs[]);
#if OPENTHREAD_CONFIG_BORDER_ROUTER_ENABLE
    otError ProcessRoute(uint8_t aArgsLength, char *aArgs[]);
    otError ProcessRouteAdd(uint8_t aArgsLength, char *aArgs[]);
    otError ProcessRouteRemove(uint8_t aArgsLength, char *aArgs[]);
    otError ProcessRouteList(void);
#endif
#if OPENTHREAD_FTD
    otError ProcessRouter(uint8_t aArgsLength, char *aArgs[]);
    otError ProcessRouterDowngradeThreshold(uint8_t aArgsLength, char *aArgs[]);
    otError ProcessRouterEligible(uint8_t aArgsLength, char *aArgs[]);
    otError ProcessRouterSelectionJitter(uint8_t aArgsLength, char *aArgs[]);
    otError ProcessRouterUpgradeThreshold(uint8_t aArgsLength, char *aArgs[]);
#endif
    otError ProcessRloc16(uint8_t aArgsLength, char *aArgs[]);
    otError ProcessScan(uint8_t aArgsLength, char *aArgs[]);
    otError ProcessSingleton(uint8_t aArgsLength, char *aArgs[]);
#if OPENTHREAD_CONFIG_SNTP_CLIENT_ENABLE
    otError ProcessSntp(uint8_t aArgsLength, char *aArgs[]);
#endif
#if OPENTHREAD_CONFIG_SRP_CLIENT_ENABLE || OPENTHREAD_CONFIG_SRP_SERVER_ENABLE
    otError ProcessSrp(uint8_t aArgsLength, char *aArgs[]);
#endif
    otError ProcessState(uint8_t aArgsLength, char *aArgs[]);
    otError ProcessThread(uint8_t aArgsLength, char *aArgs[]);
    otError ProcessDataset(uint8_t aArgsLength, char *aArgs[]);
    otError ProcessTxPower(uint8_t aArgsLength, char *aArgs[]);
    otError ProcessUdp(uint8_t aArgsLength, char *aArgs[]);
    otError ProcessUnsecurePort(uint8_t aArgsLength, char *aArgs[]);
    otError ProcessVersion(uint8_t aArgsLength, char *aArgs[]);
#if OPENTHREAD_CONFIG_MAC_FILTER_ENABLE
    otError ProcessMacFilter(uint8_t aArgsLength, char *aArgs[]);
    void    PrintMacFilter(void);
    otError ProcessMacFilterAddress(uint8_t aArgsLength, char *aArgs[]);
    otError ProcessMacFilterRss(uint8_t aArgsLength, char *aArgs[]);
#endif
    otError ProcessMac(uint8_t aArgsLength, char *aArgs[]);
    otError ProcessMacRetries(uint8_t aArgsLength, char *aArgs[]);
#if OPENTHREAD_CONFIG_REFERENCE_DEVICE_ENABLE
    otError ProcessMacSend(uint8_t aArgsLength, char *aArgs[]);
#endif

    static void HandleIcmpReceive(void *               aContext,
                                  otMessage *          aMessage,
                                  const otMessageInfo *aMessageInfo,
                                  const otIcmp6Header *aIcmpHeader);
    static void HandlePingTimer(Timer &aTimer);
    static void HandleActiveScanResult(otActiveScanResult *aResult, void *aContext);
    static void HandleEnergyScanResult(otEnergyScanResult *aResult, void *aContext);
    static void HandleLinkPcapReceive(const otRadioFrame *aFrame, bool aIsTx, void *aContext);

#if OPENTHREAD_FTD || OPENTHREAD_CONFIG_TMF_NETWORK_DIAG_MTD_ENABLE
    void HandleDiagnosticGetResponse(otError aError, const otMessage *aMessage, const Ip6::MessageInfo *aMessageInfo);
    static void HandleDiagnosticGetResponse(otError              aError,
                                            otMessage *          aMessage,
                                            const otMessageInfo *aMessageInfo,
                                            void *               aContext);

    void OutputMode(uint8_t aIndentSize, const otLinkModeConfig &aMode);
    void OutputConnectivity(uint8_t aIndentSize, const otNetworkDiagConnectivity &aConnectivity);
    void OutputRoute(uint8_t aIndentSize, const otNetworkDiagRoute &aRoute);
    void OutputRouteData(uint8_t aIndentSize, const otNetworkDiagRouteData &aRouteData);
    void OutputLeaderData(uint8_t aIndentSize, const otLeaderData &aLeaderData);
    void OutputNetworkDiagMacCounters(uint8_t aIndentSize, const otNetworkDiagMacCounters &aMacCounters);
    void OutputChildTableEntry(uint8_t aIndentSize, const otNetworkDiagChildEntry &aChildEntry);
#endif

#if OPENTHREAD_CONFIG_DNS_CLIENT_ENABLE
    otError     GetDnsServerAddress(uint8_t aArgsLength, char *aArgs[], otSockAddr &aAddress, uint8_t aStartArgsIndex);
    static void HandleDnsAddressResponse(otError aError, const otDnsAddressResponse *aResponse, void *aContext);
    void        HandleDnsAddressResponse(otError aError, const otDnsAddressResponse *aResponse);
#if OPENTHREAD_CONFIG_DNS_CLIENT_SERVICE_DISCOVERY_ENABLE
    void        OutputDnsServiceInfo(uint8_t aIndentSize, const otDnsServiceInfo &aServiceInfo);
    static void HandleDnsBrowseResponse(otError aError, const otDnsBrowseResponse *aResponse, void *aContext);
    void        HandleDnsBrowseResponse(otError aError, const otDnsBrowseResponse *aResponse);
    static void HandleDnsServiceResponse(otError aError, const otDnsServiceResponse *aResponse, void *aContext);
    void        HandleDnsServiceResponse(otError aError, const otDnsServiceResponse *aResponse);
#endif
#endif

#if OPENTHREAD_CONFIG_SNTP_CLIENT_ENABLE
    static void HandleSntpResponse(void *aContext, uint64_t aTime, otError aResult);
#endif

    void HandleIcmpReceive(otMessage *aMessage, const otMessageInfo *aMessageInfo, const otIcmp6Header *aIcmpHeader);
    void SendPing(void);
    void HandleActiveScanResult(otActiveScanResult *aResult);
    void HandleEnergyScanResult(otEnergyScanResult *aResult);
    void HandleLinkPcapReceive(const otRadioFrame *aFrame, bool aIsTx);
#if OPENTHREAD_CONFIG_SNTP_CLIENT_ENABLE
    void HandleSntpResponse(uint64_t aTime, otError aResult);
#endif
#if OPENTHREAD_CONFIG_MLE_LINK_METRICS_ENABLE
    void PrintLinkMetricsValue(const otLinkMetricsValues *aMetricsValues);

    static void HandleLinkMetricsReport(const otIp6Address *       aAddress,
                                        const otLinkMetricsValues *aMetricsValues,
                                        uint8_t                    aStatus,
                                        void *                     aContext);

    void HandleLinkMetricsReport(const otIp6Address *       aAddress,
                                 const otLinkMetricsValues *aMetricsValues,
                                 uint8_t                    aStatus);

    static void HandleLinkMetricsMgmtResponse(const otIp6Address *aAddress, uint8_t aStatus, void *aContext);

    void HandleLinkMetricsMgmtResponse(const otIp6Address *aAddress, uint8_t aStatus);

    static void HandleLinkMetricsEnhAckProbingIe(otShortAddress             aShortAddress,
                                                 const otExtAddress *       aExtAddress,
                                                 const otLinkMetricsValues *aMetricsValues,
                                                 void *                     aContext);

    void HandleLinkMetricsEnhAckProbingIe(otShortAddress             aShortAddress,
                                          const otExtAddress *       aExtAddress,
                                          const otLinkMetricsValues *aMetricsValues);

    const char *LinkMetricsStatusToStr(uint8_t aStatus);
#endif // OPENTHREAD_CONFIG_MLE_LINK_METRICS_ENABLE

    static Interpreter &GetOwner(InstanceLocator &aInstanceLocator);

    static void HandleDiscoveryRequest(const otThreadDiscoveryRequestInfo *aInfo, void *aContext)
    {
        static_cast<Interpreter *>(aContext)->HandleDiscoveryRequest(*aInfo);
    }
    void HandleDiscoveryRequest(const otThreadDiscoveryRequestInfo &aInfo);

    static constexpr Command sCommands[] = {
#if (OPENTHREAD_CONFIG_THREAD_VERSION >= OT_THREAD_VERSION_1_2)
        {"bbr", &Interpreter::ProcessBackboneRouter},
#endif
#if OPENTHREAD_CONFIG_BORDER_ROUTING_ENABLE
        {"br", &Interpreter::ProcessBorderRouting},
#endif
        {"bufferinfo", &Interpreter::ProcessBufferInfo},
        {"ccathreshold", &Interpreter::ProcessCcaThreshold},
        {"channel", &Interpreter::ProcessChannel},
#if OPENTHREAD_FTD
        {"child", &Interpreter::ProcessChild},
        {"childip", &Interpreter::ProcessChildIp},
        {"childmax", &Interpreter::ProcessChildMax},
#endif
#if OPENTHREAD_CONFIG_CHILD_SUPERVISION_ENABLE
        {"childsupervision", &Interpreter::ProcessChildSupervision},
#endif
        {"childtimeout", &Interpreter::ProcessChildTimeout},
#if OPENTHREAD_CONFIG_COAP_API_ENABLE
        {"coap", &Interpreter::ProcessCoap},
#endif
#if OPENTHREAD_CONFIG_COAP_SECURE_API_ENABLE
        {"coaps", &Interpreter::ProcessCoapSecure},
#endif
#if OPENTHREAD_CONFIG_PLATFORM_RADIO_COEX_ENABLE
        {"coex", &Interpreter::ProcessCoexMetrics},
#endif
#if OPENTHREAD_CONFIG_COMMISSIONER_ENABLE && OPENTHREAD_FTD
        {"commissioner", &Interpreter::ProcessCommissioner},
#endif
#if OPENTHREAD_FTD
        {"contextreusedelay", &Interpreter::ProcessContextIdReuseDelay},
#endif
        {"counters", &Interpreter::ProcessCounters},
#if OPENTHREAD_CONFIG_MAC_CSL_RECEIVER_ENABLE
        {"csl", &Interpreter::ProcessCsl},
#endif
        {"dataset", &Interpreter::ProcessDataset},
#if OPENTHREAD_FTD
        {"delaytimermin", &Interpreter::ProcessDelayTimerMin},
#endif
#if OPENTHREAD_CONFIG_DIAG_ENABLE
        {"diag", &Interpreter::ProcessDiag},
#endif
        {"discover", &Interpreter::ProcessDiscover},
#if OPENTHREAD_CONFIG_DNS_CLIENT_ENABLE
        {"dns", &Interpreter::ProcessDns},
#endif
#if (OPENTHREAD_CONFIG_THREAD_VERSION >= OT_THREAD_VERSION_1_2)
        {"domainname", &Interpreter::ProcessDomainName},
#endif
#if OPENTHREAD_CONFIG_DUA_ENABLE
        {"dua", &Interpreter::ProcessDua},
#endif
#if OPENTHREAD_FTD
        {"eidcache", &Interpreter::ProcessEidCache},
#endif
        {"eui64", &Interpreter::ProcessEui64},
#if OPENTHREAD_POSIX
        {"exit", &Interpreter::ProcessExit},
#endif
        {"extaddr", &Interpreter::ProcessExtAddress},
        {"extpanid", &Interpreter::ProcessExtPanId},
        {"factoryreset", &Interpreter::ProcessFactoryReset},
#if OPENTHREAD_FTD && OPENTHREAD_CONFIG_REFERENCE_DEVICE_ENABLE
        {"fake", &Interpreter::ProcessFake},
#endif
        {"fem", &Interpreter::ProcessFem},
        {"help", &Interpreter::ProcessHelp},
        {"ifconfig", &Interpreter::ProcessIfconfig},
        {"ipaddr", &Interpreter::ProcessIpAddr},
        {"ipmaddr", &Interpreter::ProcessIpMulticastAddr},
#if OPENTHREAD_CONFIG_JOINER_ENABLE
        {"joiner", &Interpreter::ProcessJoiner},
#endif
#if OPENTHREAD_FTD
        {"joinerport", &Interpreter::ProcessJoinerPort},
#endif
        {"keysequence", &Interpreter::ProcessKeySequence},
        {"leaderdata", &Interpreter::ProcessLeaderData},
#if OPENTHREAD_FTD
        {"leaderweight", &Interpreter::ProcessLeaderWeight},
#endif
#if OPENTHREAD_CONFIG_MLE_LINK_METRICS_ENABLE
        {"linkmetrics", &Interpreter::ProcessLinkMetrics},
#endif
        {"log", &Interpreter::ProcessLog},
        {"mac", &Interpreter::ProcessMac},
#if OPENTHREAD_CONFIG_MAC_FILTER_ENABLE
        {"macfilter", &Interpreter::ProcessMacFilter},
#endif
        {"masterkey", &Interpreter::ProcessMasterKey},
#if (OPENTHREAD_FTD && OPENTHREAD_CONFIG_TMF_PROXY_MLR_ENABLE) && OPENTHREAD_CONFIG_COMMISSIONER_ENABLE
        {"mlr", &Interpreter::ProcessMlr},
#endif
        {"mode", &Interpreter::ProcessMode},
#if OPENTHREAD_FTD
        {"neighbor", &Interpreter::ProcessNeighbor},
#endif
        {"netdata", &Interpreter::ProcessNetworkData},
#if OPENTHREAD_CONFIG_PLATFORM_NETIF_ENABLE
        {"netif", &Interpreter::ProcessNetif},
#endif
        {"netstat", &Interpreter::ProcessNetstat},
#if OPENTHREAD_FTD || OPENTHREAD_CONFIG_TMF_NETWORK_DIAG_MTD_ENABLE
        {"networkdiagnostic", &Interpreter::ProcessNetworkDiagnostic},
#endif
#if OPENTHREAD_FTD
        {"networkidtimeout", &Interpreter::ProcessNetworkIdTimeout},
#endif
        {"networkname", &Interpreter::ProcessNetworkName},
#if OPENTHREAD_CONFIG_TIME_SYNC_ENABLE
        {"networktime", &Interpreter::ProcessNetworkTime},
#endif
        {"panid", &Interpreter::ProcessPanId},
        {"parent", &Interpreter::ProcessParent},
#if OPENTHREAD_FTD
        {"parentpriority", &Interpreter::ProcessParentPriority},
        {"partitionid", &Interpreter::ProcessPartitionId},
#endif
        {"ping", &Interpreter::ProcessPing},
        {"pollperiod", &Interpreter::ProcessPollPeriod},
#if OPENTHREAD_FTD
        {"preferrouterid", &Interpreter::ProcessPreferRouterId},
#endif
#if OPENTHREAD_CONFIG_BORDER_ROUTER_ENABLE
        {"prefix", &Interpreter::ProcessPrefix},
#endif
        {"promiscuous", &Interpreter::ProcessPromiscuous},
#if OPENTHREAD_FTD
        {"pskc", &Interpreter::ProcessPskc},
#endif
        {"rcp", &Interpreter::ProcessRcp},
        {"region", &Interpreter::ProcessRegion},
#if OPENTHREAD_FTD
        {"releaserouterid", &Interpreter::ProcessReleaseRouterId},
#endif
        {"reset", &Interpreter::ProcessReset},
        {"rloc16", &Interpreter::ProcessRloc16},
#if OPENTHREAD_CONFIG_BORDER_ROUTER_ENABLE
        {"route", &Interpreter::ProcessRoute},
#endif
#if OPENTHREAD_FTD
        {"router", &Interpreter::ProcessRouter},
        {"routerdowngradethreshold", &Interpreter::ProcessRouterDowngradeThreshold},
        {"routereligible", &Interpreter::ProcessRouterEligible},
        {"routerselectionjitter", &Interpreter::ProcessRouterSelectionJitter},
        {"routerupgradethreshold", &Interpreter::ProcessRouterUpgradeThreshold},
#endif
        {"scan", &Interpreter::ProcessScan},
#if OPENTHREAD_CONFIG_TMF_NETDATA_SERVICE_ENABLE
        {"service", &Interpreter::ProcessService},
#endif
        {"singleton", &Interpreter::ProcessSingleton},
#if OPENTHREAD_CONFIG_SNTP_CLIENT_ENABLE
        {"sntp", &Interpreter::ProcessSntp},
#endif
#if OPENTHREAD_CONFIG_SRP_CLIENT_ENABLE || OPENTHREAD_CONFIG_SRP_SERVER_ENABLE
        {"srp", &Interpreter::ProcessSrp},
#endif
        {"state", &Interpreter::ProcessState},
        {"thread", &Interpreter::ProcessThread},
        {"txpower", &Interpreter::ProcessTxPower},
        {"udp", &Interpreter::ProcessUdp},
        {"unsecureport", &Interpreter::ProcessUnsecurePort},
        {"version", &Interpreter::ProcessVersion},
    };

    static_assert(Utils::LookupTable::IsSorted(sCommands), "Command Table is not sorted");

    Instance *          mInstance;
    const otCliCommand *mUserCommands;
    uint8_t             mUserCommandsLength;
    void *              mUserCommandsContext;
    uint16_t            mPingLength;
    uint16_t            mPingCount;
    uint32_t            mPingInterval;
    uint8_t             mPingHopLimit;
    bool                mPingAllowZeroHopLimit;
    uint16_t            mPingIdentifier;
    otIp6Address        mPingDestAddress;
    TimerMilli          mPingTimer;
    otIcmp6Handler      mIcmpHandler;
#if OPENTHREAD_CONFIG_SNTP_CLIENT_ENABLE
    bool mSntpQueryingInProgress;
#endif

    Dataset     mDataset;
    NetworkData mNetworkData;
    UdpExample  mUdp;

#if OPENTHREAD_CONFIG_COAP_API_ENABLE
    Coap mCoap;
#endif

#if OPENTHREAD_CONFIG_COAP_SECURE_API_ENABLE
    CoapSecure mCoapSecure;
#endif

#if OPENTHREAD_CONFIG_COMMISSIONER_ENABLE && OPENTHREAD_FTD
    Commissioner mCommissioner;
#endif

#if OPENTHREAD_CONFIG_JOINER_ENABLE
    Joiner mJoiner;
#endif

#if OPENTHREAD_CONFIG_SRP_CLIENT_ENABLE
    SrpClient mSrpClient;
#endif

#if OPENTHREAD_CONFIG_SRP_SERVER_ENABLE
    SrpServer mSrpServer;
#endif
};

} // namespace Cli
} // namespace ot

#endif // CLI_HPP_
