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
    const char *mName;                                     ///< A pointer to the command string.
    void (Interpreter::*mCommand)(int argc, char *argv[]); ///< A function pointer to process the command.
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
     * @retval OT_ERROR_NONE   Successfully parsed the ASCII string.
     * @retval OT_ERROR_PARSE  Could not parse the ASCII string.
     *
     */
    static otError ParseLong(char *aString, long &aLong);

    /**
     * This method parses an ASCII string as an unsigned long.
     *
     * @param[in]   aString          A pointer to the ASCII string.
     * @param[out]  aUnsignedLong    A reference to where the parsed unsigned long is placed.
     *
     * @retval OT_ERROR_NONE   Successfully parsed the ASCII string.
     * @retval OT_ERROR_PARSE  Could not parse the ASCII string.
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
     *
     * @returns The number of bytes in the binary representation.
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
    void    ProcessHelp(int argc, char *argv[]);
    void    ProcessBufferInfo(int argc, char *argv[]);
    void    ProcessChannel(int argc, char *argv[]);
#if OPENTHREAD_FTD
    void ProcessChild(int argc, char *argv[]);
    void ProcessChildIp(int argc, char *argv[]);
    void ProcessChildMax(int argc, char *argv[]);
#endif
    void ProcessChildTimeout(int argc, char *argv[]);
#if OPENTHREAD_CONFIG_COAP_API_ENABLE
    void ProcessCoap(int argc, char *argv[]);
#endif // OPENTHREAD_CONFIG_COAP_API_ENABLE
#if OPENTHREAD_CONFIG_COAP_SECURE_API_ENABLE
    void ProcessCoapSecure(int argc, char *argv[]);
#endif // OPENTHREAD_CONFIG_COAP_API_ENABLE
#if OPENTHREAD_CONFIG_PLATFORM_RADIO_COEX_ENABLE
    void ProcessCoexMetrics(int argc, char *argv[]);
#endif
#if OPENTHREAD_CONFIG_COMMISSIONER_ENABLE && OPENTHREAD_FTD
    void ProcessCommissioner(int argc, char *argv[]);
#endif
#if OPENTHREAD_FTD
    void ProcessContextIdReuseDelay(int argc, char *argv[]);
#endif
    void ProcessCounters(int argc, char *argv[]);
#if OPENTHREAD_FTD
    void ProcessDelayTimerMin(int argc, char *argv[]);
#endif
#if OPENTHREAD_CONFIG_DIAG_ENABLE
    void ProcessDiag(int argc, char *argv[]);
#endif // OPENTHREAD_CONFIG_DIAG_ENABLE
    void ProcessDiscover(int argc, char *argv[]);
#if OPENTHREAD_CONFIG_DNS_CLIENT_ENABLE
    void ProcessDns(int argc, char *argv[]);
#endif
#if OPENTHREAD_FTD
    void ProcessEidCache(int argc, char *argv[]);
#endif
    void ProcessEui64(int argc, char *argv[]);
#if OPENTHREAD_POSIX
    void ProcessExit(int argc, char *argv[]);
#endif
#if (OPENTHREAD_CONFIG_LOG_OUTPUT == OPENTHREAD_CONFIG_LOG_OUTPUT_DEBUG_UART) && OPENTHREAD_POSIX
    void ProcessLogFilename(int argc, char *argv[]);
#endif
    void    ProcessExtAddress(int argc, char *argv[]);
    void    ProcessExtPanId(int argc, char *argv[]);
    void    ProcessFactoryReset(int argc, char *argv[]);
    void    ProcessIfconfig(int argc, char *argv[]);
    void    ProcessIpAddr(int argc, char *argv[]);
    otError ProcessIpAddrAdd(int argc, char *argv[]);
    otError ProcessIpAddrDel(int argc, char *argv[]);
    void    ProcessIpMulticastAddr(int argc, char *argv[]);
    otError ProcessIpMulticastAddrAdd(int argc, char *argv[]);
    otError ProcessIpMulticastAddrDel(int argc, char *argv[]);
    otError ProcessMulticastPromiscuous(int argc, char *argv[]);
#if OPENTHREAD_CONFIG_JOINER_ENABLE
    void ProcessJoiner(int argc, char *argv[]);
#endif
#if OPENTHREAD_FTD
    void ProcessJoinerPort(int argc, char *argv[]);
#endif
    void ProcessKeySequence(int argc, char *argv[]);
    void ProcessLeaderData(int argc, char *argv[]);
#if OPENTHREAD_FTD
    void ProcessLeaderPartitionId(int argc, char *argv[]);
    void ProcessLeaderWeight(int argc, char *argv[]);
#endif
    void ProcessMasterKey(int argc, char *argv[]);
    void ProcessMode(int argc, char *argv[]);
#if OPENTHREAD_FTD
    void ProcessNeighbor(int argc, char *argv[]);
#endif
#if OPENTHREAD_CONFIG_BORDER_ROUTER_ENABLE || OPENTHREAD_CONFIG_TMF_NETDATA_SERVICE_ENABLE
    void ProcessNetworkDataRegister(int argc, char *argv[]);
#endif
    void ProcessNetworkDataShow(int argc, char *argv[]);
#if OPENTHREAD_CONFIG_TMF_NETDATA_SERVICE_ENABLE
    void ProcessService(int argc, char *argv[]);
#endif
#if OPENTHREAD_FTD || OPENTHREAD_CONFIG_TMF_NETWORK_DIAG_MTD_ENABLE
    void ProcessNetworkDiagnostic(int argc, char *argv[]);
#endif // OPENTHREAD_FTD || OPENTHREAD_CONFIG_TMF_NETWORK_DIAG_MTD_ENABLE
#if OPENTHREAD_FTD
    void ProcessNetworkIdTimeout(int argc, char *argv[]);
#endif
    void ProcessNetworkName(int argc, char *argv[]);
#if OPENTHREAD_CONFIG_TIME_SYNC_ENABLE
    void ProcessNetworkTime(int argc, char *argv[]);
#endif
    void ProcessPanId(int argc, char *argv[]);
    void ProcessParent(int argc, char *argv[]);
#if OPENTHREAD_FTD
    void ProcessParentPriority(int argc, char *argv[]);
#endif
    void ProcessPing(int argc, char *argv[]);
    void ProcessPollPeriod(int argc, char *argv[]);
#if OPENTHREAD_CONFIG_BORDER_ROUTER_ENABLE
    void    ProcessPrefix(int argc, char *argv[]);
    otError ProcessPrefixAdd(int argc, char *argv[]);
    otError ProcessPrefixRemove(int argc, char *argv[]);
    otError ProcessPrefixList(void);
#endif
    void ProcessPromiscuous(int argc, char *argv[]);
#if OPENTHREAD_FTD
    void ProcessPskc(int argc, char *argv[]);
    void ProcessReleaseRouterId(int argc, char *argv[]);
#endif
    void ProcessReset(int argc, char *argv[]);
#if OPENTHREAD_CONFIG_BORDER_ROUTER_ENABLE
    void    ProcessRoute(int argc, char *argv[]);
    otError ProcessRouteAdd(int argc, char *argv[]);
    otError ProcessRouteRemove(int argc, char *argv[]);
    otError ProcessRouteList(void);
#endif
#if OPENTHREAD_FTD
    void ProcessRouter(int argc, char *argv[]);
    void ProcessRouterDowngradeThreshold(int argc, char *argv[]);
    void ProcessRouterEligible(int argc, char *argv[]);
    void ProcessRouterSelectionJitter(int argc, char *argv[]);
    void ProcessRouterUpgradeThreshold(int argc, char *argv[]);
#endif
    void ProcessRloc16(int argc, char *argv[]);
    void ProcessScan(int argc, char *argv[]);
    void ProcessSingleton(int argc, char *argv[]);
#if OPENTHREAD_CONFIG_SNTP_CLIENT_ENABLE
    void ProcessSntp(int argc, char *argv[]);
#endif
    void ProcessState(int argc, char *argv[]);
    void ProcessThread(int argc, char *argv[]);
    void ProcessDataset(int argc, char *argv[]);
    void ProcessTxPower(int argc, char *argv[]);
    void ProcessUdp(int argc, char *argv[]);
    void ProcessVersion(int argc, char *argv[]);
#if OPENTHREAD_CONFIG_MAC_FILTER_ENABLE
    void    ProcessMacFilter(int argc, char *argv[]);
    void    PrintMacFilter(void);
    otError ProcessMacFilterAddress(int argc, char *argv[]);
    otError ProcessMacFilterRss(int argc, char *argv[]);
#endif // OPENTHREAD_CONFIG_MAC_FILTER_ENABLE
    void    ProcessMac(int argc, char *argv[]);
    otError ProcessMacRetries(int argc, char *argv[]);

    static void HandleIcmpReceive(void *               aContext,
                                  otMessage *          aMessage,
                                  const otMessageInfo *aMessageInfo,
                                  const otIcmp6Header *aIcmpHeader);
    static void HandlePingTimer(Timer &aTimer);
    static void HandleActiveScanResult(otActiveScanResult *aResult, void *aContext);
    static void HandleEnergyScanResult(otEnergyScanResult *aResult, void *aContext);
    static void HandleLinkPcapReceive(const otRadioFrame *aFrame, bool aIsTx, void *aContext);
    static void HandleDiagnosticGetResponse(otMessage *aMessage, const otMessageInfo *aMessageInfo, void *aContext);

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
    void HandleDiagnosticGetResponse(Message &aMessage, const Ip6::MessageInfo &aMessageInfo);
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
