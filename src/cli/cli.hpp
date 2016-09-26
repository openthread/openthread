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

#ifdef OPENTHREAD_CONFIG_FILE
#include OPENTHREAD_CONFIG_FILE
#else
#include <openthread-config.h>
#endif

#include <stdarg.h>

#include <cli/cli_server.hpp>
#include <net/icmp6.hpp>
#include <common/timer.hpp>

namespace Thread {

/**
 * @namespace Thread::Cli
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
public:

    /**
     * Constructor
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
     * @retval kThreadError_None   Successfully parsed the ASCII string.
     * @retval kThreadError_Parse  Could not parse the ASCII string.
     *
     */
    static ThreadError ParseLong(char *aString, long &aLong);

    /**
     * This method parses an ASCII string as an unsigned long.
     *
     * @param[in]   aString          A pointer to the ASCII string.
     * @param[out]  aUnsignedLong    A reference to where the parsed unsigned long is placed.
     *
     * @retval kThreadError_None   Successfully parsed the ASCII string.
     * @retval kThreadError_Parse  Could not parse the ASCII string.
     *
     */
    static ThreadError ParseUnsignedLong(char *aString, unsigned long &aUnsignedLong);

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
    };

    void AppendResult(ThreadError error);
    void OutputBytes(const uint8_t *aBytes, uint8_t aLength);

    void ProcessHelp(int argc, char *argv[]);
    void ProcessBlacklist(int argc, char *argv[]);
    void ProcessChannel(int argc, char *argv[]);
    void ProcessChild(int argc, char *argv[]);
    void ProcessChildTimeout(int argc, char *argv[]);
    void ProcessChildMax(int argc, char *argv[]);
#if OPENTHREAD_ENABLE_COMMISSIONER
    void ProcessCommissioner(int argc, char *argv[]);
#endif  // OPENTHREAD_ENABLE_COMMISSIONER
    void ProcessContextIdReuseDelay(int argc, char *argv[]);
    void ProcessCounters(int argc, char *argv[]);
    void ProcessDataset(int argc, char *argv[]);
#if OPENTHREAD_ENABLE_DIAG
    void ProcessDiag(int argc, char *argv[]);
#endif  // OPENTHREAD_ENABLE_DIAG
    void ProcessDiscover(int argc, char *argv[]);
    void ProcessEidCache(int argc, char *argv[]);
    void ProcessEui64(int argc, char *argv[]);
#ifdef OPENTHREAD_EXAMPLES_POSIX
    void ProcessExit(int argc, char *argv[]);
#endif
    void ProcessExtAddress(int argc, char *argv[]);
    void ProcessExtPanId(int argc, char *argv[]);
    void ProcessHashMacAddress(int argc, char *argv[]);
    void ProcessIfconfig(int argc, char *argv[]);
    void ProcessIpAddr(int argc, char *argv[]);
    ThreadError ProcessIpAddrAdd(int argc, char *argv[]);
    ThreadError ProcessIpAddrDel(int argc, char *argv[]);
#if OPENTHREAD_ENABLE_JOINER
    void ProcessJoiner(int argc, char *argv[]);
#endif  // OPENTHREAD_ENABLE_JOINER
    void ProcessJoinerPort(int argc, char *argv[]);
    void ProcessKeySequence(int argc, char *argv[]);
    void ProcessLeaderData(int argc, char *argv[]);
    void ProcessLeaderPartitionId(int argc, char *argv[]);
    void ProcessLeaderWeight(int argc, char *argv[]);
    void ProcessLinkQuality(int argc, char *argv[]);
    void ProcessMasterKey(int argc, char *argv[]);
    void ProcessMode(int argc, char *argv[]);
    void ProcessNetworkDataRegister(int argc, char *argv[]);
    void ProcessNetworkDiagnostic(int argc, char *argv[]);
    void ProcessNetworkIdTimeout(int argc, char *argv[]);
    void ProcessNetworkName(int argc, char *argv[]);
    void ProcessPanId(int argc, char *argv[]);
    void ProcessParent(int argc, char *argv[]);
    void ProcessPing(int argc, char *argv[]);
    void ProcessPollPeriod(int argc, char *argv[]);
    void ProcessPrefix(int argc, char *argv[]);
    ThreadError ProcessPrefixAdd(int argc, char *argv[]);
    ThreadError ProcessPrefixRemove(int argc, char *argv[]);
    ThreadError ProcessPrefixList(void);
    void ProcessPromiscuous(int argc, char *argv[]);
    void ProcessReleaseRouterId(int argc, char *argv[]);
    void ProcessReset(int argc, char *argv[]);
    void ProcessRoute(int argc, char *argv[]);
    void ProcessRouter(int argc, char *argv[]);
    void ProcessRouterDowngradeThreshold(int argc, char *argv[]);
    void ProcessRouterRole(int argc, char *argv[]);
    ThreadError ProcessRouteAdd(int argc, char *argv[]);
    ThreadError ProcessRouteRemove(int argc, char *argv[]);
    void ProcessRouterUpgradeThreshold(int argc, char *argv[]);
    void ProcessRloc16(int argc, char *argv[]);
    void ProcessScan(int argc, char *argv[]);
    void ProcessSingleton(int argc, char *argv[]);
    void ProcessState(int argc, char *argv[]);
    void ProcessThread(int argc, char *argv[]);
    void ProcessVersion(int argc, char *argv[]);
    void ProcessWhitelist(int argc, char *argv[]);

    static void s_HandleEchoResponse(void *aContext, Message &aMessage, const Ip6::MessageInfo &aMessageInfo);
    static void s_HandlePingTimer(void *aContext);
    static void s_HandleActiveScanResult(otActiveScanResult *aResult, void *aContext);
    static void s_HandleNetifStateChanged(uint32_t aFlags, void *aContext);
    static void s_HandleLinkPcapReceive(const RadioPacket *aFrame, void *aContext);
    static void s_HandleEnergyReport(uint32_t aChannelMask, const uint8_t *aEnergyList, uint8_t aEnergyListLength,
                                     void *aContext);
    static void s_HandlePanIdConflict(uint16_t aPanId, uint32_t aChannelMask, void *aContext);

    void HandleEchoResponse(Message &aMessage, const Ip6::MessageInfo &aMessageInfo);
    void HandlePingTimer();
    void HandleActiveScanResult(otActiveScanResult *aResult);
    void HandleNetifStateChanged(uint32_t aFlags);
    void HandleLinkPcapReceive(const RadioPacket *aFrame);
    void HandleEnergyReport(uint32_t aChannelMask, const uint8_t *aEnergyList, uint8_t aEnergyListLength);
    void HandlePanIdConflict(uint16_t aPanId, uint32_t aChannelMask);

    static const struct Command sCommands[];

    Ip6::MessageInfo sMessageInfo;
    Server *sServer;
    uint16_t sLength;
    uint16_t sCount;
    uint32_t sInterval;
    Timer sPingTimer;

    otNetifAddress sAutoAddresses[8];

    otInstance *mInstance;
};

}  // namespace Cli
}  // namespace Thread

#endif  // CLI_HPP_
