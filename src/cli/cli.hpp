/*
 *  Copyright (c) 2016, Nest Labs, Inc.
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

#include <stdarg.h>

#include <cli/cli_server.hpp>
#include <net/icmp6.hpp>
#include <common/timer.hpp>
#include <openthread-config.h>

namespace Thread {

/**
 * @namespace Thread::Cli
 *
 * @brief
 *   This namespace contains definitions for the CLI interpreter.
 *
 */
namespace Cli {

/**
 * This structure represents a CLI command.
 *
 */
struct Command
{
    const char *mName;                         ///< A pointer to the command string.
    void (*mCommand)(int argc, char *argv[]);  ///< A function pointer to process the command.
};

/**
 * This class implements the CLI interpreter.
 *
 */
class Interpreter
{
public:
    /**
     * This method initializes the CLI interpreter.
     *
     */
    static void Init(void);

    /**
     * This method interprets a CLI command.
     *
     * @param[in]  aBuf        A pointer to a string.
     * @param[in]  aBufLength  The length of the string in bytes.
     * @param[in]  aServer     A reference to the CLI server.
     *
     */
    static void ProcessLine(char *aBuf, uint16_t aBufLength, Server &aServer);

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
        kMaxArgs = 8,
    };

    static void AppendResult(ThreadError error);
    static void OutputBytes(const uint8_t *aBytes, uint8_t aLength);

    static void ProcessHelp(int argc, char *argv[]);
    static void ProcessChannel(int argc, char *argv[]);
    static void ProcessChild(int argc, char *argv[]);
    static void ProcessChildTimeout(int argc, char *argv[]);
    static void ProcessContextIdReuseDelay(int argc, char *argv[]);
    static void ProcessCounters(int argc, char *argv[]);
    static void ProcessDataset(int argc, char *argv[]);
    static void ProcessDiscover(int argc, char *argv[]);
    static void ProcessEidCache(int argc, char *argv[]);
    static void ProcessExtAddress(int argc, char *argv[]);
    static void ProcessExtPanId(int argc, char *argv[]);
    static void ProcessIfconfig(int argc, char *argv[]);
    static void ProcessIpAddr(int argc, char *argv[]);
    static ThreadError ProcessIpAddrAdd(int argc, char *argv[]);
    static ThreadError ProcessIpAddrDel(int argc, char *argv[]);
    static void ProcessKeySequence(int argc, char *argv[]);
    static void ProcessLeaderData(int argc, char *argv[]);
    static void ProcessLeaderWeight(int argc, char *argv[]);
    static void ProcessMasterKey(int argc, char *argv[]);
    static void ProcessMode(int argc, char *argv[]);
    static void ProcessNetworkDataRegister(int argc, char *argv[]);
    static void ProcessNetworkIdTimeout(int argc, char *argv[]);
    static void ProcessNetworkName(int argc, char *argv[]);
    static void ProcessPanId(int argc, char *argv[]);
    static void ProcessPing(int argc, char *argv[]);
    static void ProcessPrefix(int argc, char *argv[]);
    static ThreadError ProcessPrefixAdd(int argc, char *argv[]);
    static ThreadError ProcessPrefixRemove(int argc, char *argv[]);
    static void ProcessReleaseRouterId(int argc, char *argv[]);
    static void ProcessRoute(int argc, char *argv[]);
    static void ProcessRouter(int argc, char *argv[]);
    static ThreadError ProcessRouteAdd(int argc, char *argv[]);
    static ThreadError ProcessRouteRemove(int argc, char *argv[]);
    static void ProcessRouterUpgradeThreshold(int argc, char *argv[]);
    static void ProcessRloc16(int argc, char *argv[]);
    static void ProcessScan(int argc, char *argv[]);
    static void ProcessState(int argc, char *argv[]);
    static void ProcessThread(int argc, char *argv[]);
    static void ProcessVersion(int argc, char *argv[]);
    static void ProcessWhitelist(int argc, char *argv[]);

#if OPENTHREAD_ENABLE_DIAG
    static void ProcessDiag(int argc, char *argv[]);
#endif

    static void HandleEchoResponse(void *aContext, Message &aMessage, const Ip6::MessageInfo &aMessageInfo);
    static void HandlePingTimer(void *aContext);
    static void HandleActiveScanResult(otActiveScanResult *aResult);

    static const struct Command sCommands[];
    static otNetifAddress sAddress;

    static Ip6::SockAddr sSockAddr;
    static Ip6::IcmpEcho *sIcmpEcho;
    static Server *sServer;
    static uint8_t sEchoRequest[];
    static uint16_t sLength;
    static uint16_t sCount;
    static uint32_t sInterval;
    static Timer *sPingTimer;
};

}  // namespace Cli
}  // namespace Thread

#endif  // CLI_HPP_
