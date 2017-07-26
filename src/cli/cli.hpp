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

#include <openthread/config.h>

#include <stdarg.h>

#include <openthread/openthread.h>
#include <openthread/ip6.h>
#include <openthread/udp.h>

#include "cli/cli_server.hpp"
#include "cli/cli_udp_example.hpp"

#if OPENTHREAD_ENABLE_APPLICATION_COAP
#include <coap/coap_header.hpp>
#include "cli/cli_coap.hpp"
#endif

#include "common/code_utils.hpp"
#include "common/context.hpp"

#ifndef OTDLL
#include <openthread/dhcp6_client.h>
#include <openthread/dns.h>
#include "common/timer.hpp"
#include "net/icmp6.hpp"
#endif

#ifdef OTDLL
#define MAX_CLI_OT_INSTANCES 64
#endif

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

/**
 * This structure represents a CLI command.
 *
 */
struct Command
{
    const char *mName;                         ///< A pointer to the command string.
    void (Interpreter::*mCommand)(int argc, char *argv[]);  ///< A function pointer to process the command.
};

/**
 * This class implements the CLI interpreter.
 *
 */
class Interpreter
{
    friend class Coap;
    friend class Udp;

public:

    /**
     * Constructor
     *
     * @param[in]  aInstance  The OpenThread instance structure.
     */
    Interpreter(otInstance *aInstance);

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
     * @param[in]   aHex        A pointer to the hex string.
     * @param[out]  aBin        A pointer to where the binary representation is placed.
     * @param[in]   aBinLength  Maximum length of the binary representation.
     *
     * @returns The number of bytes in the binary representation.
     */
    static int Hex2Bin(const char *aHex, uint8_t *aBin, uint16_t aBinLength);

private:
    enum
    {
        kMaxArgs = 32,
        kMaxAutoAddresses = 8,
        kDefaultJoinerTimeout = 120,    ///< Default timeout for Joiners, in seconds.
    };

    void AppendResult(otError error) const;
    void OutputBytes(const uint8_t *aBytes, uint8_t aLength) const;

    void ProcessHelp(int argc, char *argv[]);
    void ProcessAutoStart(int argc, char *argv[]);
    void ProcessBufferInfo(int argc, char *argv[]);
    void ProcessChannel(int argc, char *argv[]);
#if OPENTHREAD_FTD
    void ProcessChild(int argc, char *argv[]);
    void ProcessChildMax(int argc, char *argv[]);
#endif
    void ProcessChildTimeout(int argc, char *argv[]);
#if OPENTHREAD_ENABLE_APPLICATION_COAP
    void ProcessCoap(int argc, char *argv[]);
#endif  //OPENTHREAD_ENABLE_APPLICATION_COAP
#if OPENTHREAD_ENABLE_COMMISSIONER && OPENTHREAD_FTD
    void ProcessCommissioner(int argc, char *argv[]);
#endif  // OPENTHREAD_ENABLE_COMMISSIONER && OPENTHREAD_FTD
#if OPENTHREAD_FTD
    void ProcessContextIdReuseDelay(int argc, char *argv[]);
#endif
    void ProcessCounters(int argc, char *argv[]);
    void ProcessDataset(int argc, char *argv[]);
#if OPENTHREAD_FTD
    void ProcessDelayTimerMin(int argc, char *argv[]);
#endif
#if OPENTHREAD_ENABLE_DIAG
    void ProcessDiag(int argc, char *argv[]);
#endif  // OPENTHREAD_ENABLE_DIAG
    void ProcessDiscover(int argc, char *argv[]);
#if OPENTHREAD_ENABLE_DNS_CLIENT
    void ProcessDns(int argc, char *argv[]);
#endif
#if OPENTHREAD_FTD
    void ProcessEidCache(int argc, char *argv[]);
#endif
    void ProcessEui64(int argc, char *argv[]);
#ifdef OPENTHREAD_EXAMPLES_POSIX
    void ProcessExit(int argc, char *argv[]);
#endif
    void ProcessExtAddress(int argc, char *argv[]);
    void ProcessExtPanId(int argc, char *argv[]);
    void ProcessFactoryReset(int argc, char *argv[]);
    void ProcessHashMacAddress(int argc, char *argv[]);
    void ProcessIfconfig(int argc, char *argv[]);
    void ProcessIpAddr(int argc, char *argv[]);
    otError ProcessIpAddrAdd(int argc, char *argv[]);
    otError ProcessIpAddrDel(int argc, char *argv[]);
    void ProcessIpMulticastAddr(int argc, char *argv[]);
#ifndef OTDLL
    otError ProcessIpMulticastAddrAdd(int argc, char *argv[]);
    otError ProcessIpMulticastAddrDel(int argc, char *argv[]);
    otError ProcessMulticastPromiscuous(int argc, char *argv[]);
#endif
#if OPENTHREAD_ENABLE_JOINER
    void ProcessJoiner(int argc, char *argv[]);
#endif  // OPENTHREAD_ENABLE_JOINER
#if OPENTHREAD_FTD
    void ProcessJoinerPort(int argc, char *argv[]);
#endif
    void ProcessKeySequence(int argc, char *argv[]);
    void ProcessLeaderData(int argc, char *argv[]);
#if OPENTHREAD_FTD
    void ProcessLeaderPartitionId(int argc, char *argv[]);
    void ProcessLeaderWeight(int argc, char *argv[]);
#endif
    void ProcessLinkQuality(int argc, char *argv[]);
    void ProcessMasterKey(int argc, char *argv[]);
    void ProcessMode(int argc, char *argv[]);
#if OPENTHREAD_ENABLE_BORDER_ROUTER
    void ProcessNetworkDataRegister(int argc, char *argv[]);
#endif
#if OPENTHREAD_FTD || OPENTHREAD_ENABLE_MTD_NETWORK_DIAGNOSTIC
    void ProcessNetworkDiagnostic(int argc, char *argv[]);
#endif // OPENTHREAD_FTD || OPENTHREAD_ENABLE_MTD_NETWORK_DIAGNOSTIC
#if OPENTHREAD_FTD
    void ProcessNetworkIdTimeout(int argc, char *argv[]);
#endif
    void ProcessNetworkName(int argc, char *argv[]);
    void ProcessPanId(int argc, char *argv[]);
    void ProcessParent(int argc, char *argv[]);
#if OPENTHREAD_FTD
    void ProcessParentPriority(int argc, char *argv[]);
#endif
    void ProcessPing(int argc, char *argv[]);
    void ProcessPollPeriod(int argc, char *argv[]);
#if OPENTHREAD_ENABLE_BORDER_ROUTER
    void ProcessPrefix(int argc, char *argv[]);
    otError ProcessPrefixAdd(int argc, char *argv[]);
    otError ProcessPrefixRemove(int argc, char *argv[]);
    otError ProcessPrefixList(void);
#endif
    void ProcessPromiscuous(int argc, char *argv[]);
#if OPENTHREAD_FTD
    void ProcessPSKc(int argc, char *argv[]);
    void ProcessReleaseRouterId(int argc, char *argv[]);
#endif
    void ProcessReset(int argc, char *argv[]);
#if OPENTHREAD_ENABLE_BORDER_ROUTER
    void ProcessRoute(int argc, char *argv[]);
    otError ProcessRouteAdd(int argc, char *argv[]);
    otError ProcessRouteRemove(int argc, char *argv[]);
#endif
#if OPENTHREAD_FTD
    void ProcessRouter(int argc, char *argv[]);
    void ProcessRouterDowngradeThreshold(int argc, char *argv[]);
    void ProcessRouterRole(int argc, char *argv[]);
    void ProcessRouterSelectionJitter(int argc, char *argv[]);
    void ProcessRouterUpgradeThreshold(int argc, char *argv[]);
#endif
    void ProcessRloc16(int argc, char *argv[]);
    void ProcessScan(int argc, char *argv[]);
    void ProcessSingleton(int argc, char *argv[]);
    void ProcessState(int argc, char *argv[]);
    void ProcessThread(int argc, char *argv[]);
    void ProcessTxPowerMax(int argc, char *argv[]);
    void ProcessVersion(int argc, char *argv[]);
#if OPENTHREAD_ENABLE_MAC_FILTER
    void ProcessMacFilter(int argc, char *argv[]);
    void PrintMacFilter(void);
    otError ProcessMacFilterAddress(int argc, char *argv[]);
#ifndef OTDLL
    otError ProcessMacFilterRss(int argc, char *argv[]);
#endif  // OTDLL
#endif  // OPENTHREAD_ENABLE_MAC_FILTER

#ifdef OTDLL
    void ProcessInstanceList(int argc, char *argv[]);
    void ProcessInstance(int argc, char *argv[]);
#endif

#ifndef OTDLL
    void ProcessUdp(int argc, char *argv[]);
#endif

#ifndef OTDLL
    static void s_HandleIcmpReceive(void *aContext, otMessage *aMessage, const otMessageInfo *aMessageInfo,
                                    const otIcmp6Header *aIcmpHeader);
    static void s_HandlePingTimer(Timer &aTimer);
#endif
    static void OTCALL s_HandleActiveScanResult(otActiveScanResult *aResult, void *aContext);
    static void OTCALL s_HandleNetifStateChanged(uint32_t aFlags, void *aContext);
#ifndef OTDLL
    static void s_HandleLinkPcapReceive(const otRadioFrame *aFrame, void *aContext);
#endif
    static void OTCALL s_HandleEnergyReport(uint32_t aChannelMask, const uint8_t *aEnergyList, uint8_t aEnergyListLength,
                                            void *aContext);
    static void OTCALL s_HandlePanIdConflict(uint16_t aPanId, uint32_t aChannelMask, void *aContext);
#ifndef OTDLL
    static void OTCALL s_HandleDiagnosticGetResponse(otMessage *aMessage, const otMessageInfo *aMessageInfo,
                                                     void *aContext);
#endif
    static void OTCALL s_HandleJoinerCallback(otError aError, void *aContext);

#if OPENTHREAD_ENABLE_DNS_CLIENT
    static void s_HandleDnsResponse(void *aContext, const char *aHostname, otIp6Address *aAddress,
                                    uint32_t aTtl, otError aResult);
#endif

#ifndef OTDLL
    void HandleIcmpReceive(Message &aMessage, const Ip6::MessageInfo &aMessageInfo,
                           const otIcmp6Header &aIcmpHeader);
    void HandlePingTimer();
#endif
    void HandleActiveScanResult(otActiveScanResult *aResult);
#ifdef OTDLL
    void HandleNetifStateChanged(otInstance *aInstance, uint32_t aFlags);
#else
    void HandleNetifStateChanged(uint32_t aFlags);
#endif
#ifndef OTDLL
    void HandleLinkPcapReceive(const otRadioFrame *aFrame);
#endif
    void HandleEnergyReport(uint32_t aChannelMask, const uint8_t *aEnergyList, uint8_t aEnergyListLength);
    void HandlePanIdConflict(uint16_t aPanId, uint32_t aChannelMask);
#ifndef OTDLL
    void HandleDiagnosticGetResponse(Message &aMessage, const Ip6::MessageInfo &aMessageInfo);
#endif
    void HandleJoinerCallback(otError aError);

#if OPENTHREAD_ENABLE_DNS_CLIENT
    void HandleDnsResponse(const char *aHostname, Ip6::Address &aAddress, uint32_t aTtl, otError aResult);
#endif

    static Interpreter &GetOwner(const Context &aContext);

#if OPENTHREAD_ENABLE_APPLICATION_COAP

    Coap mCoap;

#endif // OPENTHREAD_ENABLE_APPLICATION_COAP

    static const struct Command sCommands[];

    Server *mServer;

#ifdef OTDLL

    void CacheInstances();

    otApiInstance *mApiInstance;

    struct otCliContext
    {
        Interpreter *mInterpreter;
        otInstance  *mInstance;
    };
    otCliContext mInstances[MAX_CLI_OT_INSTANCES];
    uint8_t mInstancesLength;
    uint8_t mInstanceIndex;

#else

    Ip6::MessageInfo mMessageInfo;

    uint16_t mLength;
    uint16_t mCount;
    uint32_t mInterval;
    TimerMilli mPingTimer;

    otNetifAddress  mSlaacAddresses[OPENTHREAD_CONFIG_NUM_SLAAC_ADDRESSES];
#if OPENTHREAD_ENABLE_DHCP6_CLIENT
    otDhcpAddress  mDhcpAddresses[OPENTHREAD_CONFIG_NUM_DHCP_PREFIXES];
#endif // OPENTHREAD_ENABLE_DHCP6_CLIENT

    otIcmp6Handler mIcmpHandler;
#if OPENTHREAD_ENABLE_DNS_CLIENT
    bool mResolvingInProgress;
    char mResolvingHostname[OT_DNS_MAX_HOSTNAME_LENGTH];
#endif

    Udp mUdp;

#endif

    otInstance *mInstance;
};

}  // namespace Cli
}  // namespace ot

#endif  // CLI_HPP_
