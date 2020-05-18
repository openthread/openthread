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
#include <openthread/ip6.h>
#include <openthread/udp.h>

#include "cli/cli_commissioner.hpp"
#include "cli/cli_dataset.hpp"
#include "cli/cli_joiner.hpp"
#include "cli/cli_udp.hpp"

#if OPENTHREAD_CONFIG_COAP_API_ENABLE
#include "cli/cli_coap.hpp"
#endif

#if OPENTHREAD_CONFIG_COAP_SECURE_API_ENABLE
#include "cli/cli_coap_secure.hpp"
#endif

#include "common/code_utils.hpp"
#include "common/instance.hpp"

#include <openthread/dns.h>
#include <openthread/sntp.h>
#include "common/timer.hpp"
#include "net/icmp6.hpp"

namespace ot {

/**
 * @namespace ot::Cli
 *
 * @brief
 *   This namespace contains definitions for the CLI interpreter.
 *
 */
namespace Cli {

class Interpreter;
class Server;

/**
 * This structure represents a CLI command.
 *
 */
struct Command
{
    const char *mName;                                                 ///< A pointer to the command string.
    void (Interpreter::*mCommand)(uint8_t aArgsLength, char *aArgs[]); ///< A function pointer to process the command.
};

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
    friend class UdpExample;

public:
    /**
     * Constructor
     *
     * @param[in]  aInstance  The OpenThread instance structure.
     */
    explicit Interpreter(Instance *aInstance);

    /**
     * This method interprets a CLI command.
     *
     * @param[in]  aBuf        A pointer to a string.
     * @param[in]  aBufLength  The length of the string in bytes.
     * @param[in]  aServer     A reference to the CLI server.
     *
     */
    void ProcessLine(char *aBuf, uint16_t aBufLength, Server &aServer);

    /**
     * This method parses an ASCII string as a long.
     *
     * @param[in]   aString  A pointer to the ASCII string.
     * @param[out]  aLong    A reference to where the parsed long is placed.
     *
     * @retval OT_ERROR_NONE          Successfully parsed the ASCII string.
     * @retval OT_ERROR_INVALID_ARGS  @p aString is not a valid long integer.
     *
     */
    static otError ParseLong(char *aString, long &aLong);

    /**
     * This method parses an ASCII string as an unsigned long.
     *
     * @param[in]   aString          A pointer to the ASCII string.
     * @param[out]  aUnsignedLong    A reference to where the parsed unsigned long is placed.
     *
     * @retval OT_ERROR_NONE          Successfully parsed the ASCII string.
     * @retval OT_ERROR_INVALID_ARGS  @p aString is not a valid unsigned long integer.
     *
     */
    static otError ParseUnsignedLong(char *aString, unsigned long &aUnsignedLong);

    /**
     * This method converts a hex string to binary.
     *
     * @param[in]   aHex            A pointer to the hex string.
     * @param[out]  aBin            A pointer to where the binary representation is placed.
     * @param[in]   aBinLength      Maximum length of the binary representation.
     * @param[in]   aAllowTruncate  TRUE if @p aBinLength may be less than what is required
     *                              to convert @p aHex to binary representation, FALSE otherwise.
     *
     * @returns  The number of bytes in the binary representation, or -1 if @p aHex is not a valid hex string
     *
     */
    static int Hex2Bin(const char *aHex, uint8_t *aBin, uint16_t aBinLength, bool aAllowTruncate = false);

    /**
     * Write error code the CLI console
     *
     * @param[in]  aError Error code value.
     */
    void AppendResult(otError aError) const;

    /**
     * Write a number of bytes to the CLI console as a hex string.
     *
     * @param[in]  aBytes   A pointer to data which should be printed.
     * @param[in]  aLength  @p aBytes length.
     */
    void OutputBytes(const uint8_t *aBytes, uint8_t aLength) const;

    /**
     * Write an IPv6 address to the CLI console.
     *
     * @param[in]  aAddress  A reference to the IPv6 address.
     */
    void OutputIp6Address(const otIp6Address &aAddress) const;

    /**
     * Set a user command table.
     *
     * @param[in]  aUserCommands  A pointer to an array with user commands.
     * @param[in]  aLength        @p aUserCommands length.
     */
    void SetUserCommands(const otCliCommand *aCommands, uint8_t aLength);

private:
    enum
    {
        kMaxArgs          = 32,
        kMaxAutoAddresses = 8,

        kDefaultPingInterval = 1000, // (in mses)
        kDefaultPingLength   = 8,    // (in bytes)
        kDefaultPingCount    = 1,
    };

    otError ParsePingInterval(const char *aString, uint32_t &aInterval);
    void    ProcessHelp(uint8_t aArgsLength, char *aArgs[]);
    void    ProcessBufferInfo(uint8_t aArgsLength, char *aArgs[]);
    void    ProcessChannel(uint8_t aArgsLength, char *aArgs[]);
#if (OPENTHREAD_CONFIG_THREAD_VERSION >= OT_THREAD_VERSION_1_2)
    void ProcessBackboneRouter(uint8_t aArgsLength, char *aArgs[]);

#if OPENTHREAD_FTD && OPENTHREAD_CONFIG_BACKBONE_ROUTER_ENABLE
    otError ProcessBackboneRouterLocal(uint8_t aArgsLength, char *aArgs[]);
#endif

    void ProcessDomainName(uint8_t aArgsLength, char *aArgs[]);
#endif // (OPENTHREAD_CONFIG_THREAD_VERSION >= OT_THREAD_VERSION_1_2)

#if OPENTHREAD_FTD
    void ProcessChild(uint8_t aArgsLength, char *aArgs[]);
    void ProcessChildIp(uint8_t aArgsLength, char *aArgs[]);
    void ProcessChildMax(uint8_t aArgsLength, char *aArgs[]);
#endif
    void ProcessChildTimeout(uint8_t aArgsLength, char *aArgs[]);
#if OPENTHREAD_CONFIG_COAP_API_ENABLE
    void ProcessCoap(uint8_t aArgsLength, char *aArgs[]);
#endif // OPENTHREAD_CONFIG_COAP_API_ENABLE
#if OPENTHREAD_CONFIG_COAP_SECURE_API_ENABLE
    void ProcessCoapSecure(uint8_t aArgsLength, char *aArgs[]);
#endif // OPENTHREAD_CONFIG_COAP_API_ENABLE
#if OPENTHREAD_CONFIG_PLATFORM_RADIO_COEX_ENABLE
    void ProcessCoexMetrics(uint8_t aArgsLength, char *aArgs[]);
#endif
#if OPENTHREAD_CONFIG_COMMISSIONER_ENABLE && OPENTHREAD_FTD
    void ProcessCommissioner(uint8_t aArgsLength, char *aArgs[]);
#endif
#if OPENTHREAD_FTD
    void ProcessContextIdReuseDelay(uint8_t aArgsLength, char *aArgs[]);
#endif
    void ProcessCounters(uint8_t aArgsLength, char *aArgs[]);
#if OPENTHREAD_FTD
    void ProcessDelayTimerMin(uint8_t aArgsLength, char *aArgs[]);
#endif
#if OPENTHREAD_CONFIG_DIAG_ENABLE
    void ProcessDiag(uint8_t aArgsLength, char *aArgs[]);
#endif // OPENTHREAD_CONFIG_DIAG_ENABLE
    void ProcessDiscover(uint8_t aArgsLength, char *aArgs[]);
#if OPENTHREAD_CONFIG_DNS_CLIENT_ENABLE
    void ProcessDns(uint8_t aArgsLength, char *aArgs[]);
#endif
#if OPENTHREAD_FTD
    void ProcessEidCache(uint8_t aArgsLength, char *aArgs[]);
#endif
    void ProcessEui64(uint8_t aArgsLength, char *aArgs[]);
#if OPENTHREAD_POSIX
    void ProcessExit(uint8_t aArgsLength, char *aArgs[]);
#endif
    void    ProcessLog(uint8_t aArgsLength, char *aArgs[]);
    void    ProcessExtAddress(uint8_t aArgsLength, char *aArgs[]);
    void    ProcessExtPanId(uint8_t aArgsLength, char *aArgs[]);
    void    ProcessFactoryReset(uint8_t aArgsLength, char *aArgs[]);
    void    ProcessIfconfig(uint8_t aArgsLength, char *aArgs[]);
    void    ProcessIpAddr(uint8_t aArgsLength, char *aArgs[]);
    otError ProcessIpAddrAdd(uint8_t aArgsLength, char *aArgs[]);
    otError ProcessIpAddrDel(uint8_t aArgsLength, char *aArgs[]);
    void    ProcessIpMulticastAddr(uint8_t aArgsLength, char *aArgs[]);
    otError ProcessIpMulticastAddrAdd(uint8_t aArgsLength, char *aArgs[]);
    otError ProcessIpMulticastAddrDel(uint8_t aArgsLength, char *aArgs[]);
    otError ProcessMulticastPromiscuous(uint8_t aArgsLength, char *aArgs[]);
#if OPENTHREAD_CONFIG_JOINER_ENABLE
    void ProcessJoiner(uint8_t aArgsLength, char *aArgs[]);
#endif
#if OPENTHREAD_FTD
    void ProcessJoinerPort(uint8_t aArgsLength, char *aArgs[]);
#endif
    void ProcessKeySequence(uint8_t aArgsLength, char *aArgs[]);
    void ProcessLeaderData(uint8_t aArgsLength, char *aArgs[]);
#if OPENTHREAD_FTD
    void ProcessLeaderPartitionId(uint8_t aArgsLength, char *aArgs[]);
    void ProcessLeaderWeight(uint8_t aArgsLength, char *aArgs[]);
#endif
    void ProcessMasterKey(uint8_t aArgsLength, char *aArgs[]);
    void ProcessMode(uint8_t aArgsLength, char *aArgs[]);
#if OPENTHREAD_FTD
    void ProcessNeighbor(uint8_t aArgsLength, char *aArgs[]);
#endif
#if OPENTHREAD_CONFIG_BORDER_ROUTER_ENABLE || OPENTHREAD_CONFIG_TMF_NETDATA_SERVICE_ENABLE
    void ProcessNetworkDataRegister(uint8_t aArgsLength, char *aArgs[]);
#endif
    void ProcessNetworkDataShow(uint8_t aArgsLength, char *aArgs[]);
#if OPENTHREAD_CONFIG_PLATFORM_NETIF_ENABLE
    void ProcessNetif(uint8_t aArgsLength, char *aArgs[]);
#endif
#if OPENTHREAD_CONFIG_TMF_NETDATA_SERVICE_ENABLE
    void ProcessService(uint8_t aArgsLength, char *aArgs[]);
#endif
#if OPENTHREAD_FTD || OPENTHREAD_CONFIG_TMF_NETWORK_DIAG_MTD_ENABLE
    void ProcessNetworkDiagnostic(uint8_t aArgsLength, char *aArgs[]);
#endif // OPENTHREAD_FTD || OPENTHREAD_CONFIG_TMF_NETWORK_DIAG_MTD_ENABLE
#if OPENTHREAD_FTD
    void ProcessNetworkIdTimeout(uint8_t aArgsLength, char *aArgs[]);
#endif
    void ProcessNetworkName(uint8_t aArgsLength, char *aArgs[]);
#if OPENTHREAD_CONFIG_TIME_SYNC_ENABLE
    void ProcessNetworkTime(uint8_t aArgsLength, char *aArgs[]);
#endif
    void ProcessPanId(uint8_t aArgsLength, char *aArgs[]);
    void ProcessParent(uint8_t aArgsLength, char *aArgs[]);
#if OPENTHREAD_FTD
    void ProcessParentPriority(uint8_t aArgsLength, char *aArgs[]);
#endif
    void ProcessPing(uint8_t aArgsLength, char *aArgs[]);
    void ProcessPollPeriod(uint8_t aArgsLength, char *aArgs[]);
    void SignalPingRequest(const Ip6::Address &aPeerAddress,
                           uint16_t            aPingLength,
                           uint32_t            aTimestamp,
                           uint8_t             aHopLimit);
    void SignalPingReply(const Ip6::Address &aPeerAddress,
                         uint16_t            aPingLength,
                         uint32_t            aTimestamp,
                         uint8_t             aHopLimit);

#if OPENTHREAD_CONFIG_BORDER_ROUTER_ENABLE
    void    ProcessPrefix(uint8_t aArgsLength, char *aArgs[]);
    otError ProcessPrefixAdd(uint8_t aArgsLength, char *aArgs[]);
    otError ProcessPrefixRemove(uint8_t aArgsLength, char *aArgs[]);
    otError ProcessPrefixList(void);
    void    OutputPrefix(otBorderRouterConfig &aConfig);
#endif
    void ProcessPromiscuous(uint8_t aArgsLength, char *aArgs[]);
#if OPENTHREAD_FTD
    void ProcessPreferRouterId(uint8_t aArgsLength, char *aArgs[]);
    void ProcessPskc(uint8_t aArgsLength, char *aArgs[]);
    void ProcessReleaseRouterId(uint8_t aArgsLength, char *aArgs[]);
#endif
    void ProcessReset(uint8_t aArgsLength, char *aArgs[]);
#if OPENTHREAD_CONFIG_BORDER_ROUTER_ENABLE
    void    ProcessRoute(uint8_t aArgsLength, char *aArgs[]);
    otError ProcessRouteAdd(uint8_t aArgsLength, char *aArgs[]);
    otError ProcessRouteRemove(uint8_t aArgsLength, char *aArgs[]);
    otError ProcessRouteList(void);
#endif
#if OPENTHREAD_FTD
    void ProcessRouter(uint8_t aArgsLength, char *aArgs[]);
    void ProcessRouterDowngradeThreshold(uint8_t aArgsLength, char *aArgs[]);
    void ProcessRouterEligible(uint8_t aArgsLength, char *aArgs[]);
    void ProcessRouterSelectionJitter(uint8_t aArgsLength, char *aArgs[]);
    void ProcessRouterUpgradeThreshold(uint8_t aArgsLength, char *aArgs[]);
#endif
    void ProcessRloc16(uint8_t aArgsLength, char *aArgs[]);
    void ProcessScan(uint8_t aArgsLength, char *aArgs[]);
    void ProcessSingleton(uint8_t aArgsLength, char *aArgs[]);
#if OPENTHREAD_CONFIG_SNTP_CLIENT_ENABLE
    void ProcessSntp(uint8_t aArgsLength, char *aArgs[]);
#endif
    void ProcessState(uint8_t aArgsLength, char *aArgs[]);
    void ProcessThread(uint8_t aArgsLength, char *aArgs[]);
    void ProcessDataset(uint8_t aArgsLength, char *aArgs[]);
    void ProcessTxPower(uint8_t aArgsLength, char *aArgs[]);
    void ProcessUdp(uint8_t aArgsLength, char *aArgs[]);
    void ProcessVersion(uint8_t aArgsLength, char *aArgs[]);
#if OPENTHREAD_CONFIG_MAC_FILTER_ENABLE
    void    ProcessMacFilter(uint8_t aArgsLength, char *aArgs[]);
    void    PrintMacFilter(void);
    otError ProcessMacFilterAddress(uint8_t aArgsLength, char *aArgs[]);
    otError ProcessMacFilterRss(uint8_t aArgsLength, char *aArgs[]);
#endif // OPENTHREAD_CONFIG_MAC_FILTER_ENABLE
    void    ProcessMac(uint8_t aArgsLength, char *aArgs[]);
    otError ProcessMacRetries(uint8_t aArgsLength, char *aArgs[]);

    static void HandleIcmpReceive(void *               aContext,
                                  otMessage *          aMessage,
                                  const otMessageInfo *aMessageInfo,
                                  const otIcmp6Header *aIcmpHeader);
    static void HandlePingTimer(Timer &aTimer);
    static void HandleActiveScanResult(otActiveScanResult *aResult, void *aContext);
    static void HandleEnergyScanResult(otEnergyScanResult *aResult, void *aContext);
    static void HandleLinkPcapReceive(const otRadioFrame *aFrame, bool aIsTx, void *aContext);

#if OPENTHREAD_FTD || OPENTHREAD_CONFIG_TMF_NETWORK_DIAG_MTD_ENABLE
    void        HandleDiagnosticGetResponse(const otMessage &aMessage, const Ip6::MessageInfo &aMessageInfo);
    static void HandleDiagnosticGetResponse(otMessage *aMessage, const otMessageInfo *aMessageInfo, void *aContext);
    void        OutputSpaces(uint16_t aCount);
    void        OutputMode(const otLinkModeConfig &aMode, uint16_t aColumn);
    void        OutputConnectivity(const otNetworkDiagConnectivity &aConnectivity, uint16_t aColumn);
    void        OutputRoute(const otNetworkDiagRoute &aRoute, uint16_t aColumn);
    void        OutputRouteData(const otNetworkDiagRouteData &aRouteData, uint16_t aColumn);
    void        OutputLeaderData(const otLeaderData &aLeaderData, uint16_t aColumn);
    void        OutputNetworkDiagMacCounters(const otNetworkDiagMacCounters &aMacCounters, uint16_t aColumn);
    void        OutputChildTableEntry(const otNetworkDiagChildEntry &aChildEntry, uint16_t aColumn);
#endif // OPENTHREAD_FTD || OPENTHREAD_CONFIG_TMF_NETWORK_DIAG_MTD_ENABLE

#if OPENTHREAD_CONFIG_DNS_CLIENT_ENABLE
    static void HandleDnsResponse(void *              aContext,
                                  const char *        aHostname,
                                  const otIp6Address *aAddress,
                                  uint32_t            aTtl,
                                  otError             aResult);
#endif

#if OPENTHREAD_CONFIG_SNTP_CLIENT_ENABLE
    static void HandleSntpResponse(void *aContext, uint64_t aTime, otError aResult);
#endif

    void HandleIcmpReceive(otMessage *aMessage, const otMessageInfo *aMessageInfo, const otIcmp6Header *aIcmpHeader);
    void SendPing(void);
    void HandleActiveScanResult(otActiveScanResult *aResult);
    void HandleEnergyScanResult(otEnergyScanResult *aResult);
    void HandleLinkPcapReceive(const otRadioFrame *aFrame, bool aIsTx);
#if OPENTHREAD_CONFIG_DNS_CLIENT_ENABLE
    void HandleDnsResponse(const char *aHostname, const Ip6::Address *aAddress, uint32_t aTtl, otError aResult);
#endif
#if OPENTHREAD_CONFIG_SNTP_CLIENT_ENABLE
    void HandleSntpResponse(uint64_t aTime, otError aResult);
#endif
    static Interpreter &GetOwner(OwnerLocator &aOwnerLocator);

    static const struct Command sCommands[];
    const otCliCommand *        mUserCommands;
    uint8_t                     mUserCommandsLength;
    Server *                    mServer;
    uint16_t                    mPingLength;
    uint16_t                    mPingCount;
    uint32_t                    mPingInterval;
    uint8_t                     mPingHopLimit;
    bool                        mPingAllowZeroHopLimit;
    uint16_t                    mPingIdentifier;
    otIp6Address                mPingDestAddress;
    TimerMilli                  mPingTimer;
    otIcmp6Handler              mIcmpHandler;
#if OPENTHREAD_CONFIG_DNS_CLIENT_ENABLE
    bool mResolvingInProgress;
    char mResolvingHostname[OT_DNS_MAX_HOSTNAME_LENGTH];
#endif

#if OPENTHREAD_CONFIG_SNTP_CLIENT_ENABLE
    bool mSntpQueryingInProgress;
#endif

    UdpExample mUdp;
    Dataset    mDataset;

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

    Instance *mInstance;
};

} // namespace Cli
} // namespace ot

#endif // CLI_HPP_
