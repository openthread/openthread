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
 *   This file implements the CLI interpreter.
 */

#ifdef OPENTHREAD_CONFIG_FILE
#include OPENTHREAD_CONFIG_FILE
#else
#include <openthread-config.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <openthread.h>
#include <openthread-diag.h>
#include <commissioning/commissioner.h>
#include <commissioning/joiner.h>

#include "cli.hpp"
#include "cli_dataset.hpp"
#include "cli_uart.hpp"
#include <common/encoding.hpp>
#include <common/new.hpp>
#include <net/ip6.hpp>
#include <platform/random.h>
#include <platform/uart.h>

using Thread::Encoding::BigEndian::HostSwap16;
using Thread::Encoding::BigEndian::HostSwap32;

namespace Thread {

extern Ip6::Ip6 *sIp6;

namespace Cli {

const struct Command Interpreter::sCommands[] =
{
    { "help", &Interpreter::ProcessHelp },
    { "blacklist", &Interpreter::ProcessBlacklist },
    { "channel", &Interpreter::ProcessChannel },
    { "child", &Interpreter::ProcessChild },
    { "childmax", &Interpreter::ProcessChildMax },
    { "childtimeout", &Interpreter::ProcessChildTimeout },
#if OPENTHREAD_ENABLE_COMMISSIONER
    { "commissioner", &Interpreter::ProcessCommissioner },
#endif
    { "contextreusedelay", &Interpreter::ProcessContextIdReuseDelay },
    { "counter", &Interpreter::ProcessCounters },
    { "dataset", &Interpreter::ProcessDataset },
#if OPENTHREAD_ENABLE_DIAG
    { "diag", &Interpreter::ProcessDiag },
#endif
    { "discover", &Interpreter::ProcessDiscover },
    { "eidcache", &Interpreter::ProcessEidCache },
    { "eui64", &Interpreter::ProcessEui64 },
#ifdef OPENTHREAD_EXAMPLES_POSIX
    { "exit", &Interpreter::ProcessExit },
#endif
    { "extaddr", &Interpreter::ProcessExtAddress },
    { "extpanid", &Interpreter::ProcessExtPanId },
    { "hashmacaddr", &Interpreter::ProcessHashMacAddress },
    { "ifconfig", &Interpreter::ProcessIfconfig },
    { "ipaddr", &Interpreter::ProcessIpAddr },
#if OPENTHREAD_ENABLE_JOINER
    { "joiner", &Interpreter::ProcessJoiner },
#endif
    { "keysequence", &Interpreter::ProcessKeySequence },
    { "leaderdata", &Interpreter::ProcessLeaderData },
    { "leaderpartitionid", &Interpreter::ProcessLeaderPartitionId },
    { "leaderweight", &Interpreter::ProcessLeaderWeight },
    { "linkquality", &Interpreter::ProcessLinkQuality },
    { "masterkey", &Interpreter::ProcessMasterKey },
    { "mode", &Interpreter::ProcessMode },
    { "netdataregister", &Interpreter::ProcessNetworkDataRegister },
    { "networkdiagnostic", &Interpreter::ProcessNetworkDiagnostic },
    { "networkidtimeout", &Interpreter::ProcessNetworkIdTimeout },
    { "networkname", &Interpreter::ProcessNetworkName },
    { "panid", &Interpreter::ProcessPanId },
    { "parent", &Interpreter::ProcessParent },
    { "ping", &Interpreter::ProcessPing },
    { "pollperiod", &Interpreter::ProcessPollPeriod },
    { "promiscuous", &Interpreter::ProcessPromiscuous },
    { "prefix", &Interpreter::ProcessPrefix },
    { "releaserouterid", &Interpreter::ProcessReleaseRouterId },
    { "reset", &Interpreter::ProcessReset },
    { "rloc16", &Interpreter::ProcessRloc16 },
    { "route", &Interpreter::ProcessRoute },
    { "router", &Interpreter::ProcessRouter },
    { "routerdowngradethreshold", &Interpreter::ProcessRouterDowngradeThreshold },
    { "routerrole", &Interpreter::ProcessRouterRole },
    { "routerupgradethreshold", &Interpreter::ProcessRouterUpgradeThreshold },
    { "scan", &Interpreter::ProcessScan },
    { "singleton", &Interpreter::ProcessSingleton },
    { "state", &Interpreter::ProcessState },
    { "thread", &Interpreter::ProcessThread },
    { "version", &Interpreter::ProcessVersion },
    { "whitelist", &Interpreter::ProcessWhitelist },
};

Interpreter::Interpreter(otInstance *aInstance):
    sLength(8),
    sCount(1),
    sInterval(1000),
    sPingTimer(sIp6->mTimerScheduler, &Interpreter::s_HandlePingTimer, this),
    mInstance(aInstance)
{
    sIp6->mIcmp.SetEchoReplyHandler(&s_HandleEchoResponse, this);
    otSetStateChangedCallback(mInstance, &Interpreter::s_HandleNetifStateChanged, this);
}

int Interpreter::Hex2Bin(const char *aHex, uint8_t *aBin, uint16_t aBinLength)
{
    size_t hexLength = strlen(aHex);
    const char *hexEnd = aHex + hexLength;
    uint8_t *cur = aBin;
    uint8_t numChars = hexLength & 1;
    uint8_t byte = 0;

    if ((hexLength + 1) / 2 > aBinLength)
    {
        return -1;
    }

    while (aHex < hexEnd)
    {
        if ('A' <= *aHex && *aHex <= 'F')
        {
            byte |= 10 + (*aHex - 'A');
        }
        else if ('a' <= *aHex && *aHex <= 'f')
        {
            byte |= 10 + (*aHex - 'a');
        }
        else if ('0' <= *aHex && *aHex <= '9')
        {
            byte |= *aHex - '0';
        }
        else
        {
            return -1;
        }

        aHex++;
        numChars++;

        if (numChars >= 2)
        {
            numChars = 0;
            *cur++ = byte;
            byte = 0;
        }
        else
        {
            byte <<= 4;
        }
    }

    return static_cast<int>(cur - aBin);
}

void Interpreter::AppendResult(ThreadError error)
{
    if (error == kThreadError_None)
    {
        sServer->OutputFormat("Done\r\n");
    }
    else
    {
        sServer->OutputFormat("Error %d\r\n", error);
    }
}

void Interpreter::OutputBytes(const uint8_t *aBytes, uint8_t aLength)
{
    for (int i = 0; i < aLength; i++)
    {
        sServer->OutputFormat("%02x", aBytes[i]);
    }
}

ThreadError Interpreter::ParseLong(char *argv, long &value)
{
    char *endptr;
    value = strtol(argv, &endptr, 0);
    return (*endptr == '\0') ? kThreadError_None : kThreadError_Parse;
}

ThreadError Interpreter::ParseUnsignedLong(char *argv, unsigned long &value)
{
    char *endptr;
    value = strtoul(argv, &endptr, 0);
    return (*endptr == '\0') ? kThreadError_None : kThreadError_Parse;
}

void Interpreter::ProcessHelp(int argc, char *argv[])
{
    for (unsigned int i = 0; i < sizeof(sCommands) / sizeof(sCommands[0]); i++)
    {
        sServer->OutputFormat("%s\r\n", sCommands[i].mName);
    }

    (void)argc;
    (void)argv;
}

void Interpreter::ProcessBlacklist(int argc, char *argv[])
{
    ThreadError error = kThreadError_None;
    otMacBlacklistEntry entry;
    int argcur = 0;
    uint8_t extAddr[8];

    if (argcur >= argc)
    {
        if (otIsMacBlacklistEnabled(mInstance))
        {
            sServer->OutputFormat("Enabled\r\n");
        }
        else
        {
            sServer->OutputFormat("Disabled\r\n");
        }

        for (uint8_t i = 0; ; i++)
        {
            if (otGetMacBlacklistEntry(mInstance, i, &entry) != kThreadError_None)
            {
                break;
            }

            if (entry.mValid == false)
            {
                continue;
            }

            OutputBytes(entry.mExtAddress.m8, OT_EXT_ADDRESS_SIZE);

            sServer->OutputFormat("\r\n");
        }
    }
    else if (strcmp(argv[argcur], "add") == 0)
    {
        VerifyOrExit(++argcur < argc, error = kThreadError_Parse);
        VerifyOrExit(Hex2Bin(argv[argcur], extAddr, sizeof(extAddr)) == sizeof(extAddr), error = kThreadError_Parse);

        otAddMacBlacklist(mInstance, extAddr);
        VerifyOrExit(otAddMacBlacklist(mInstance, extAddr) == kThreadError_None, error = kThreadError_Parse);
    }
    else if (strcmp(argv[argcur], "clear") == 0)
    {
        otClearMacBlacklist(mInstance);
    }
    else if (strcmp(argv[argcur], "disable") == 0)
    {
        otDisableMacBlacklist(mInstance);
    }
    else if (strcmp(argv[argcur], "enable") == 0)
    {
        otEnableMacBlacklist(mInstance);
    }
    else if (strcmp(argv[argcur], "remove") == 0)
    {
        VerifyOrExit(++argcur < argc, error = kThreadError_Parse);
        VerifyOrExit(Hex2Bin(argv[argcur], extAddr, sizeof(extAddr)) == sizeof(extAddr), error = kThreadError_Parse);
        otRemoveMacBlacklist(mInstance, extAddr);
    }

exit:
    AppendResult(error);
}

void Interpreter::ProcessChannel(int argc, char *argv[])
{
    ThreadError error = kThreadError_None;
    long value;

    if (argc == 0)
    {
        sServer->OutputFormat("%d\r\n", otGetChannel(mInstance));
    }
    else
    {
        SuccessOrExit(error = ParseLong(argv[0], value));
        otSetChannel(mInstance, static_cast<uint8_t>(value));
    }

exit:
    AppendResult(error);
}

void Interpreter::ProcessChild(int argc, char *argv[])
{
    ThreadError error = kThreadError_None;
    otChildInfo childInfo;
    long value;
    bool isTable = false;

    VerifyOrExit(argc > 0, error = kThreadError_Parse);

    if (strcmp(argv[0], "list") == 0 || (isTable = (strcmp(argv[0], "table") == 0)))
    {
        if (isTable)
        {
            sServer->OutputFormat("| ID  | RLOC16 | Timeout    | Age        | LQI In | C_VN |R|S|D|N| Extended MAC     |\r\n");
            sServer->OutputFormat("+-----+--------+------------+------------+--------+------+-+-+-+-+------------------+\r\n");
        }

        for (uint8_t i = 0; ; i++)
        {
            if (otGetChildInfoByIndex(mInstance, i, &childInfo) != kThreadError_None)
            {
                sServer->OutputFormat("\r\n");
                ExitNow();
            }

            if (childInfo.mTimeout > 0)
            {
                if (isTable)
                {
                    sServer->OutputFormat("| %3d ", childInfo.mChildId);
                    sServer->OutputFormat("| 0x%04x ", childInfo.mRloc16);
                    sServer->OutputFormat("| %10d ", childInfo.mTimeout);
                    sServer->OutputFormat("| %10d ", childInfo.mAge);
                    sServer->OutputFormat("| %6d ", childInfo.mLinkQualityIn);
                    sServer->OutputFormat("| %4d ", childInfo.mNetworkDataVersion);
                    sServer->OutputFormat("|%1d", childInfo.mRxOnWhenIdle);
                    sServer->OutputFormat("|%1d", childInfo.mSecureDataRequest);
                    sServer->OutputFormat("|%1d", childInfo.mFullFunction);
                    sServer->OutputFormat("|%1d", childInfo.mFullNetworkData);
                    sServer->OutputFormat("| ");

                    for (size_t j = 0; j < sizeof(childInfo.mExtAddress); j++)
                    {
                        sServer->OutputFormat("%02x", childInfo.mExtAddress.m8[j]);
                    }

                    sServer->OutputFormat(" |\r\n");
                }
                else
                {
                    sServer->OutputFormat("%d ", childInfo.mChildId);
                }
            }
        }
    }

    SuccessOrExit(error = ParseLong(argv[0], value));
    SuccessOrExit(error = otGetChildInfoById(mInstance, static_cast<uint16_t>(value), &childInfo));

    sServer->OutputFormat("Child ID: %d\r\n", childInfo.mChildId);
    sServer->OutputFormat("Rloc: %04x\r\n", childInfo.mRloc16);
    sServer->OutputFormat("Ext Addr: ");

    for (size_t j = 0; j < sizeof(childInfo.mExtAddress); j++)
    {
        sServer->OutputFormat("%02x", childInfo.mExtAddress.m8[j]);
    }

    sServer->OutputFormat("\r\n");
    sServer->OutputFormat("Mode: ");

    if (childInfo.mRxOnWhenIdle)
    {
        sServer->OutputFormat("r");
    }

    if (childInfo.mSecureDataRequest)
    {
        sServer->OutputFormat("s");
    }

    if (childInfo.mFullFunction)
    {
        sServer->OutputFormat("d");
    }

    if (childInfo.mFullNetworkData)
    {
        sServer->OutputFormat("n");
    }

    sServer->OutputFormat("\r\n");

    sServer->OutputFormat("Net Data: %d\r\n", childInfo.mNetworkDataVersion);
    sServer->OutputFormat("Timeout: %d\r\n", childInfo.mTimeout);
    sServer->OutputFormat("Age: %d\r\n", childInfo.mAge);
    sServer->OutputFormat("LQI: %d\r\n", childInfo.mLinkQualityIn);
    sServer->OutputFormat("RSSI: %d\r\n", childInfo.mAverageRssi);

exit:
    AppendResult(error);
}

void Interpreter::ProcessChildMax(int argc, char *argv[])
{
    ThreadError error = kThreadError_None;
    long value;

    if (argc == 0)
    {
        sServer->OutputFormat("%d\r\n", otGetMaxAllowedChildren(mInstance));
    }
    else
    {
        SuccessOrExit(error = ParseLong(argv[0], value));
        SuccessOrExit(error = otSetMaxAllowedChildren(mInstance, static_cast<uint8_t>(value)));
    }

exit:
    AppendResult(error);
}

void Interpreter::ProcessChildTimeout(int argc, char *argv[])
{
    ThreadError error = kThreadError_None;
    long value;

    if (argc == 0)
    {
        sServer->OutputFormat("%d\r\n", otGetChildTimeout(mInstance));
    }
    else
    {
        SuccessOrExit(error = ParseLong(argv[0], value));
        otSetChildTimeout(mInstance, static_cast<uint32_t>(value));
    }

exit:
    AppendResult(error);
}

void Interpreter::ProcessContextIdReuseDelay(int argc, char *argv[])
{
    ThreadError error = kThreadError_None;
    long value;

    if (argc == 0)
    {
        sServer->OutputFormat("%d\r\n", otGetContextIdReuseDelay(mInstance));
    }
    else
    {
        SuccessOrExit(ParseLong(argv[0], value));
        otSetContextIdReuseDelay(mInstance, static_cast<uint32_t>(value));
    }

exit:
    AppendResult(error);
}

void Interpreter::ProcessCounters(int argc, char *argv[])
{
    if (argc == 0)
    {
        sServer->OutputFormat("mac\r\n");
        sServer->OutputFormat("Done\r\n");
    }
    else
    {
        if (strcmp(argv[0], "mac") == 0)
        {
            const otMacCounters *counters = otGetMacCounters(mInstance);
            sServer->OutputFormat("TxTotal: %d\r\n", counters->mTxTotal);
            sServer->OutputFormat("    TxAckRequested: %d\r\n", counters->mTxAckRequested);
            sServer->OutputFormat("    TxAcked: %d\r\n", counters->mTxAcked);
            sServer->OutputFormat("    TxNoAckRequested: %d\r\n", counters->mTxNoAckRequested);
            sServer->OutputFormat("    TxData: %d\r\n", counters->mTxData);
            sServer->OutputFormat("    TxDataPoll: %d\r\n", counters->mTxDataPoll);
            sServer->OutputFormat("    TxBeacon: %d\r\n", counters->mTxBeacon);
            sServer->OutputFormat("    TxBeaconRequest: %d\r\n", counters->mTxBeaconRequest);
            sServer->OutputFormat("    TxOther: %d\r\n", counters->mTxOther);
            sServer->OutputFormat("    TxRetry: %d\r\n", counters->mTxRetry);
            sServer->OutputFormat("    TxErrCca: %d\r\n", counters->mTxErrCca);
            sServer->OutputFormat("RxTotal: %d\r\n", counters->mRxTotal);
            sServer->OutputFormat("    RxData: %d\r\n", counters->mRxData);
            sServer->OutputFormat("    RxDataPoll: %d\r\n", counters->mRxDataPoll);
            sServer->OutputFormat("    RxBeacon: %d\r\n", counters->mRxBeacon);
            sServer->OutputFormat("    RxBeaconRequest: %d\r\n", counters->mRxBeaconRequest);
            sServer->OutputFormat("    RxOther: %d\r\n", counters->mRxOther);
            sServer->OutputFormat("    RxWhitelistFiltered: %d\r\n", counters->mRxWhitelistFiltered);
            sServer->OutputFormat("    RxDestAddrFiltered: %d\r\n", counters->mRxDestAddrFiltered);
            sServer->OutputFormat("    RxErrNoFrame: %d\r\n", counters->mRxErrNoFrame);
            sServer->OutputFormat("    RxErrNoUnknownNeighbor: %d\r\n", counters->mRxErrUnknownNeighbor);
            sServer->OutputFormat("    RxErrInvalidSrcAddr: %d\r\n", counters->mRxErrInvalidSrcAddr);
            sServer->OutputFormat("    RxErrSec: %d\r\n", counters->mRxErrSec);
            sServer->OutputFormat("    RxErrFcs: %d\r\n", counters->mRxErrFcs);
            sServer->OutputFormat("    RxErrOther: %d\r\n", counters->mRxErrOther);
        }
    }
}

void Interpreter::ProcessDataset(int argc, char *argv[])
{
    ThreadError error;
    error = Dataset::Process(mInstance, argc, argv, *sServer);
    AppendResult(error);
}

void Interpreter::ProcessDiscover(int argc, char *argv[])
{
    ThreadError error = kThreadError_None;
    uint32_t scanChannels = 0;
    long value;

    if (argc > 0)
    {
        SuccessOrExit(error = ParseLong(argv[0], value));
        scanChannels = 1 << value;
    }

    SuccessOrExit(error = otDiscover(mInstance, scanChannels, 0, OT_PANID_BROADCAST,
                                     &Interpreter::s_HandleActiveScanResult, this));
    sServer->OutputFormat("| J | Network Name     | Extended PAN     | PAN  | MAC Address      | Ch | dBm | LQI |\r\n");
    sServer->OutputFormat("+---+------------------+------------------+------+------------------+----+-----+-----+\r\n");

    return;

exit:
    AppendResult(error);
}

void Interpreter::ProcessEidCache(int argc, char *argv[])
{
    otEidCacheEntry entry;

    for (uint8_t i = 0; ; i++)
    {
        SuccessOrExit(otGetEidCacheEntry(mInstance, i, &entry));

        if (entry.mValid == false)
        {
            continue;
        }

        sServer->OutputFormat("%x:%x:%x:%x:%x:%x:%x:%x %04x\r\n",
                              HostSwap16(entry.mTarget.mFields.m16[0]),
                              HostSwap16(entry.mTarget.mFields.m16[1]),
                              HostSwap16(entry.mTarget.mFields.m16[2]),
                              HostSwap16(entry.mTarget.mFields.m16[3]),
                              HostSwap16(entry.mTarget.mFields.m16[4]),
                              HostSwap16(entry.mTarget.mFields.m16[5]),
                              HostSwap16(entry.mTarget.mFields.m16[6]),
                              HostSwap16(entry.mTarget.mFields.m16[7]),
                              entry.mRloc16);
    }

exit:
    (void)argc;
    (void)argv;
    AppendResult(kThreadError_None);
}

void Interpreter::ProcessEui64(int argc, char *argv[])
{
    ThreadError error = kThreadError_None;
    otExtAddress extAddress;

    VerifyOrExit(argc == 0, error = kThreadError_Parse);

    otGetFactoryAssignedIeeeEui64(mInstance, &extAddress);
    OutputBytes(extAddress.m8, OT_EXT_ADDRESS_SIZE);
    sServer->OutputFormat("\r\n");

exit:
    (void)argv;
    AppendResult(error);
}

void Interpreter::ProcessExtAddress(int argc, char *argv[])
{
    ThreadError error = kThreadError_None;

    if (argc == 0)
    {
        OutputBytes(otGetExtendedAddress(mInstance), OT_EXT_ADDRESS_SIZE);
        sServer->OutputFormat("\r\n");
    }
    else
    {
        otExtAddress extAddress;

        VerifyOrExit(Hex2Bin(argv[0], extAddress.m8, sizeof(otExtAddress)) >= 0, error = kThreadError_Parse);

        otSetExtendedAddress(mInstance, &extAddress);
    }

exit:
    AppendResult(error);
}

#ifdef OPENTHREAD_EXAMPLES_POSIX
void Interpreter::ProcessExit(int argc, char *argv[])
{
    exit(0);
    (void)argc;
    (void)argv;
}
#endif

void Interpreter::ProcessExtPanId(int argc, char *argv[])
{
    ThreadError error = kThreadError_None;

    if (argc == 0)
    {
        OutputBytes(otGetExtendedPanId(mInstance), OT_EXT_PAN_ID_SIZE);
        sServer->OutputFormat("\r\n");
    }
    else
    {
        uint8_t extPanId[8];

        VerifyOrExit(Hex2Bin(argv[0], extPanId, sizeof(extPanId)) >= 0, error = kThreadError_Parse);

        otSetExtendedPanId(mInstance, extPanId);
    }

exit:
    AppendResult(error);
}

void Interpreter::ProcessHashMacAddress(int argc, char *argv[])
{
    ThreadError error = kThreadError_None;
    otExtAddress hashMacAddress;

    VerifyOrExit(argc == 0, error = kThreadError_Parse);

    otGetHashMacAddress(mInstance, &hashMacAddress);
    OutputBytes(hashMacAddress.m8, OT_EXT_ADDRESS_SIZE);
    sServer->OutputFormat("\r\n");

exit:
    (void)argv;
    AppendResult(error);
}

void Interpreter::ProcessIfconfig(int argc, char *argv[])
{
    ThreadError error = kThreadError_None;

    if (argc == 0)
    {
        if (otIsInterfaceUp(mInstance))
        {
            sServer->OutputFormat("up\r\n");
        }
        else
        {
            sServer->OutputFormat("down\r\n");
        }
    }
    else if (strcmp(argv[0], "up") == 0)
    {
        SuccessOrExit(error = otInterfaceUp(mInstance));
    }
    else if (strcmp(argv[0], "down") == 0)
    {
        SuccessOrExit(error = otInterfaceDown(mInstance));
    }

exit:
    AppendResult(error);
}

ThreadError Interpreter::ProcessIpAddrAdd(int argc, char *argv[])
{
    ThreadError error;
    otNetifAddress aAddress;

    VerifyOrExit(argc > 0, error = kThreadError_Parse);

    SuccessOrExit(error = otIp6AddressFromString(argv[0], &aAddress.mAddress));
    aAddress.mPrefixLength = 64;
    aAddress.mPreferredLifetime = 0xffffffff;
    aAddress.mValidLifetime = 0xffffffff;
    error = otAddUnicastAddress(mInstance, &aAddress);

exit:
    return error;
}

ThreadError Interpreter::ProcessIpAddrDel(int argc, char *argv[])
{
    ThreadError error;
    struct otIp6Address address;

    VerifyOrExit(argc > 0, error = kThreadError_Parse);

    SuccessOrExit(error = otIp6AddressFromString(argv[0], &address));
    error = otRemoveUnicastAddress(mInstance, &address);

exit:
    return error;
}

void Interpreter::ProcessIpAddr(int argc, char *argv[])
{
    ThreadError error = kThreadError_None;

    if (argc == 0)
    {
        for (const otNetifAddress *addr = otGetUnicastAddresses(mInstance); addr; addr = addr->mNext)
        {
            sServer->OutputFormat("%x:%x:%x:%x:%x:%x:%x:%x\r\n",
                                  HostSwap16(addr->mAddress.mFields.m16[0]),
                                  HostSwap16(addr->mAddress.mFields.m16[1]),
                                  HostSwap16(addr->mAddress.mFields.m16[2]),
                                  HostSwap16(addr->mAddress.mFields.m16[3]),
                                  HostSwap16(addr->mAddress.mFields.m16[4]),
                                  HostSwap16(addr->mAddress.mFields.m16[5]),
                                  HostSwap16(addr->mAddress.mFields.m16[6]),
                                  HostSwap16(addr->mAddress.mFields.m16[7]));
        }
    }
    else
    {
        if (strcmp(argv[0], "add") == 0)
        {
            SuccessOrExit(error = ProcessIpAddrAdd(argc - 1, argv + 1));
        }
        else if (strcmp(argv[0], "del") == 0)
        {
            SuccessOrExit(error = ProcessIpAddrDel(argc - 1, argv + 1));
        }
    }

exit:
    AppendResult(error);
}

void Interpreter::ProcessKeySequence(int argc, char *argv[])
{
    ThreadError error = kThreadError_None;
    long value;

    if (argc == 0)
    {
        sServer->OutputFormat("%d\r\n", otGetKeySequenceCounter(mInstance));
    }
    else
    {
        SuccessOrExit(error = ParseLong(argv[0], value));
        otSetKeySequenceCounter(mInstance, static_cast<uint32_t>(value));
    }

exit:
    AppendResult(error);
}

void Interpreter::ProcessLeaderData(int argc, char *argv[])
{
    ThreadError error;
    otLeaderData leaderData;

    SuccessOrExit(error = otGetLeaderData(mInstance, &leaderData));

    sServer->OutputFormat("Partition ID: %u\r\n", leaderData.mPartitionId);
    sServer->OutputFormat("Weighting: %d\r\n", leaderData.mWeighting);
    sServer->OutputFormat("Data Version: %d\r\n", leaderData.mDataVersion);
    sServer->OutputFormat("Stable Data Version: %d\r\n", leaderData.mStableDataVersion);
    sServer->OutputFormat("Leader Router ID: %d\r\n", leaderData.mLeaderRouterId);

exit:
    (void)argc;
    (void)argv;
    AppendResult(error);
}

void Interpreter::ProcessLeaderPartitionId(int argc, char *argv[])
{
    ThreadError error = kThreadError_None;
    unsigned long value;

    if (argc == 0)
    {
        sServer->OutputFormat("%u\r\n", otGetLocalLeaderPartitionId(mInstance));
    }
    else
    {
        SuccessOrExit(error = ParseUnsignedLong(argv[0], value));
        otSetLocalLeaderPartitionId(mInstance, static_cast<uint32_t>(value));
    }

exit:
    AppendResult(error);
}

void Interpreter::ProcessLeaderWeight(int argc, char *argv[])
{
    ThreadError error = kThreadError_None;
    long value;

    if (argc == 0)
    {
        sServer->OutputFormat("%d\r\n", otGetLocalLeaderWeight(mInstance));
    }
    else
    {
        SuccessOrExit(error = ParseLong(argv[0], value));
        otSetLocalLeaderWeight(mInstance, static_cast<uint8_t>(value));
    }

exit:
    AppendResult(error);
}

void Interpreter::ProcessLinkQuality(int argc, char *argv[])
{
    ThreadError error = kThreadError_None;
    uint8_t extAddress[8];
    uint8_t linkQuality;
    long value;

    VerifyOrExit(Hex2Bin(argv[0], extAddress, OT_EXT_ADDRESS_SIZE) >= 0, error = kThreadError_Parse);

    if (argc == 1)
    {
        VerifyOrExit(otGetAssignLinkQuality(mInstance, extAddress, &linkQuality) == kThreadError_None,
                     error = kThreadError_InvalidArgs);
        sServer->OutputFormat("%d\r\n", linkQuality);
    }
    else
    {
        SuccessOrExit(error = ParseLong(argv[1], value));
        otSetAssignLinkQuality(mInstance, extAddress, static_cast<uint8_t>(value));
    }

exit:
    AppendResult(error);
}

void Interpreter::ProcessMasterKey(int argc, char *argv[])
{
    ThreadError error = kThreadError_None;

    if (argc == 0)
    {
        uint8_t keyLength;
        const uint8_t *key = otGetMasterKey(mInstance, &keyLength);

        for (int i = 0; i < keyLength; i++)
        {
            sServer->OutputFormat("%02x", key[i]);
        }

        sServer->OutputFormat("\r\n");
    }
    else
    {
        int keyLength;
        uint8_t key[OT_MASTER_KEY_SIZE];

        VerifyOrExit((keyLength = Hex2Bin(argv[0], key, sizeof(key))) == OT_MASTER_KEY_SIZE, error = kThreadError_Parse);
        SuccessOrExit(error = otSetMasterKey(mInstance, key, static_cast<uint8_t>(keyLength)));
    }

exit:
    AppendResult(error);
}

void Interpreter::ProcessMode(int argc, char *argv[])
{
    ThreadError error = kThreadError_None;
    otLinkModeConfig linkMode;

    memset(&linkMode, 0, sizeof(otLinkModeConfig));

    if (argc == 0)
    {
        linkMode = otGetLinkMode(mInstance);

        if (linkMode.mRxOnWhenIdle)
        {
            sServer->OutputFormat("r");
        }

        if (linkMode.mSecureDataRequests)
        {
            sServer->OutputFormat("s");
        }

        if (linkMode.mDeviceType)
        {
            sServer->OutputFormat("d");
        }

        if (linkMode.mNetworkData)
        {
            sServer->OutputFormat("n");
        }

        sServer->OutputFormat("\r\n");
    }
    else
    {
        for (char *arg = argv[0]; *arg != '\0'; arg++)
        {
            switch (*arg)
            {
            case 'r':
                linkMode.mRxOnWhenIdle = 1;
                break;

            case 's':
                linkMode.mSecureDataRequests = 1;
                break;

            case 'd':
                linkMode.mDeviceType = 1;
                break;

            case 'n':
                linkMode.mNetworkData = 1;
                break;

            default:
                ExitNow(error = kThreadError_Parse);
            }
        }

        SuccessOrExit(error = otSetLinkMode(mInstance, linkMode));
    }

exit:
    AppendResult(error);
}

void Interpreter::ProcessNetworkDataRegister(int argc, char *argv[])
{
    ThreadError error = kThreadError_None;
    SuccessOrExit(error = otSendServerData(mInstance));

exit:
    (void)argc;
    (void)argv;
    AppendResult(error);
}

void Interpreter::ProcessNetworkIdTimeout(int argc, char *argv[])
{
    ThreadError error = kThreadError_None;
    long value;

    if (argc == 0)
    {
        sServer->OutputFormat("%d\r\n", otGetNetworkIdTimeout(mInstance));
    }
    else
    {
        SuccessOrExit(error = ParseLong(argv[0], value));
        otSetNetworkIdTimeout(mInstance, static_cast<uint8_t>(value));
    }

exit:
    AppendResult(error);
}

void Interpreter::ProcessNetworkName(int argc, char *argv[])
{
    ThreadError error = kThreadError_None;

    if (argc == 0)
    {
        sServer->OutputFormat("%.*s\r\n", OT_NETWORK_NAME_MAX_SIZE, otGetNetworkName(mInstance));
    }
    else
    {
        SuccessOrExit(error = otSetNetworkName(mInstance, argv[0]));
    }

exit:
    AppendResult(error);
}

void Interpreter::ProcessPanId(int argc, char *argv[])
{
    ThreadError error = kThreadError_None;
    long value;

    if (argc == 0)
    {
        sServer->OutputFormat("%d\r\n", otGetPanId(mInstance));
    }
    else
    {
        SuccessOrExit(error = ParseLong(argv[0], value));
        otSetPanId(mInstance, static_cast<otPanId>(value));
    }

exit:
    AppendResult(error);
}

void Interpreter::ProcessParent(int argc, char *argv[])
{
    ThreadError error = kThreadError_None;
    otRouterInfo parentInfo;

    SuccessOrExit(error = otGetParentInfo(mInstance, &parentInfo));
    sServer->OutputFormat("Ext Addr: ");

    for (size_t i = 0; i < sizeof(parentInfo.mExtAddress); i++)
    {
        sServer->OutputFormat("%02x", parentInfo.mExtAddress.m8[i]);
    }

    sServer->OutputFormat("\r\n");

    sServer->OutputFormat("Rloc: %x\r\n", parentInfo.mRloc16);

exit:
    (void)argc;
    (void)argv;
    AppendResult(error);
}

void Interpreter::s_HandleEchoResponse(void *aContext, Message &aMessage, const Ip6::MessageInfo &aMessageInfo)
{
    static_cast<Interpreter *>(aContext)->HandleEchoResponse(aMessage, aMessageInfo);
}

void Interpreter::HandleEchoResponse(Message &aMessage, const Ip6::MessageInfo &aMessageInfo)
{
    Ip6::IcmpHeader icmp6Header;
    uint32_t timestamp = 0;

    aMessage.Read(aMessage.GetOffset(), sizeof(icmp6Header), &icmp6Header);

    sServer->OutputFormat("%d bytes from ", aMessage.GetLength() - aMessage.GetOffset());
    sServer->OutputFormat("%x:%x:%x:%x:%x:%x:%x:%x",
                          HostSwap16(aMessageInfo.GetPeerAddr().mFields.m16[0]),
                          HostSwap16(aMessageInfo.GetPeerAddr().mFields.m16[1]),
                          HostSwap16(aMessageInfo.GetPeerAddr().mFields.m16[2]),
                          HostSwap16(aMessageInfo.GetPeerAddr().mFields.m16[3]),
                          HostSwap16(aMessageInfo.GetPeerAddr().mFields.m16[4]),
                          HostSwap16(aMessageInfo.GetPeerAddr().mFields.m16[5]),
                          HostSwap16(aMessageInfo.GetPeerAddr().mFields.m16[6]),
                          HostSwap16(aMessageInfo.GetPeerAddr().mFields.m16[7]));
    sServer->OutputFormat(": icmp_seq=%d hlim=%d", icmp6Header.GetSequence(), aMessageInfo.mHopLimit);

    if (aMessage.Read(aMessage.GetOffset() + sizeof(icmp6Header), sizeof(uint32_t), &timestamp) >=
        static_cast<int>(sizeof(uint32_t)))
    {
        sServer->OutputFormat(" time=%dms", Timer::GetNow() - HostSwap32(timestamp));
    }

    sServer->OutputFormat("\r\n");
}

void Interpreter::ProcessPing(int argc, char *argv[])
{
    ThreadError error = kThreadError_None;
    uint8_t index = 1;
    long value;

    VerifyOrExit(argc > 0, error = kThreadError_Parse);
    VerifyOrExit(!sPingTimer.IsRunning(), error = kThreadError_Busy);

    memset(&sMessageInfo, 0, sizeof(sMessageInfo));
    SuccessOrExit(error = sMessageInfo.GetPeerAddr().FromString(argv[0]));
    sMessageInfo.mInterfaceId = 1;

    sLength = 8;
    sCount = 1;
    sInterval = 1000;

    while (index < argc)
    {
        SuccessOrExit(error = ParseLong(argv[index], value));

        switch (index)
        {
        case 1:
            sLength = (uint16_t)value;
            break;

        case 2:
            sCount = (uint16_t)value;
            break;

        case 3:
            sInterval = (uint32_t)value;
            sInterval = sInterval * 1000;
            break;

        default:
            ExitNow(error = kThreadError_Parse);
        }

        index++;
    }

    HandlePingTimer();

    return;

exit:
    AppendResult(error);
}

void Interpreter::s_HandlePingTimer(void *aContext)
{
    static_cast<Interpreter *>(aContext)->HandlePingTimer();
}

void Interpreter::HandlePingTimer()
{
    ThreadError error = kThreadError_None;
    uint32_t timestamp = HostSwap32(Timer::GetNow());
    Message *message;

    VerifyOrExit((message = sIp6->mIcmp.NewMessage(0)) != NULL, error = kThreadError_NoBufs);
    SuccessOrExit(error = message->Append(&timestamp, sizeof(timestamp)));
    SuccessOrExit(error = message->SetLength(sLength));

    SuccessOrExit(error = sIp6->mIcmp.SendEchoRequest(*message, sMessageInfo));
    sCount--;

exit:

    if (error != kThreadError_None && message != NULL)
    {
        message->Free();
    }

    if (sCount)
    {
        sPingTimer.Start(sInterval);
    }
}

void Interpreter::ProcessPollPeriod(int argc, char *argv[])
{
    ThreadError error = kThreadError_None;
    long value;

    if (argc == 0)
    {
        sServer->OutputFormat("%d\r\n", (otGetPollPeriod(mInstance) / 1000));  // ms->s
    }
    else
    {
        SuccessOrExit(error = ParseLong(argv[0], value));
        otSetPollPeriod(mInstance, static_cast<uint32_t>(value * 1000));  // s->ms
    }

exit:
    AppendResult(error);
}

void Interpreter::ProcessPromiscuous(int argc, char *argv[])
{
    ThreadError error = kThreadError_None;

    if (argc == 0)
    {
        if (otIsLinkPromiscuous(mInstance) && otPlatRadioGetPromiscuous(mInstance))
        {
            sServer->OutputFormat("Enabled\r\n");
        }
        else
        {
            sServer->OutputFormat("Disabled\r\n");
        }
    }
    else
    {
        if (strcmp(argv[0], "enable") == 0)
        {
            otSetLinkPcapCallback(mInstance, &s_HandleLinkPcapReceive, this);
            SuccessOrExit(error = otSetLinkPromiscuous(mInstance, true));
        }
        else if (strcmp(argv[0], "disable") == 0)
        {
            otSetLinkPcapCallback(mInstance, NULL, NULL);
            SuccessOrExit(error = otSetLinkPromiscuous(mInstance, false));
        }
    }

exit:
    AppendResult(error);
}

void Interpreter::s_HandleLinkPcapReceive(const RadioPacket *aFrame, void *aContext)
{
    static_cast<Interpreter *>(aContext)->HandleLinkPcapReceive(aFrame);
}

void Interpreter::HandleLinkPcapReceive(const RadioPacket *aFrame)
{
    sServer->OutputFormat("\r\n");

    for (size_t i = 0; i < 44; i++)
    {
        sServer->OutputFormat("=");
    }

    sServer->OutputFormat("[len = %3u]", aFrame->mLength);

    for (size_t i = 0; i < 28; i++)
    {
        sServer->OutputFormat("=");
    }

    sServer->OutputFormat("\r\n");

    for (size_t i = 0; i < aFrame->mLength; i += 16)
    {
        sServer->OutputFormat("|");

        for (size_t j = 0; j < 16; j++)
        {
            if (i + j < aFrame->mLength)
            {
                sServer->OutputFormat(" %02X", aFrame->mPsdu[i + j]);
            }
            else
            {
                sServer->OutputFormat(" ..");
            }
        }

        sServer->OutputFormat("|");

        for (size_t j = 0; j < 16; j++)
        {
            if (i + j < aFrame->mLength)
            {
                if (31 < aFrame->mPsdu[i + j] && aFrame->mPsdu[i + j] < 127)
                {
                    sServer->OutputFormat(" %c", aFrame->mPsdu[i + j]);
                }
                else
                {
                    sServer->OutputFormat(" ?");
                }
            }
            else
            {
                sServer->OutputFormat(" .");
            }
        }

        sServer->OutputFormat("|\r\n");
    }

    for (size_t i = 0; i < 83; i++)
    {
        sServer->OutputFormat("-");
    }

    sServer->OutputFormat("\r\n");
}

ThreadError Interpreter::ProcessPrefixAdd(int argc, char *argv[])
{
    ThreadError error = kThreadError_None;
    otBorderRouterConfig config;
    int argcur = 0;

    memset(&config, 0, sizeof(otBorderRouterConfig));

    char *prefixLengthStr;
    char *endptr;

    if ((prefixLengthStr = strchr(argv[argcur], '/')) == NULL)
    {
        ExitNow();
    }

    *prefixLengthStr++ = '\0';

    SuccessOrExit(error = otIp6AddressFromString(argv[argcur], &config.mPrefix.mPrefix));

    config.mPrefix.mLength = static_cast<uint8_t>(strtol(prefixLengthStr, &endptr, 0));

    if (*endptr != '\0')
    {
        ExitNow(error = kThreadError_Parse);
    }

    argcur++;

    for (; argcur < argc; argcur++)
    {
        if (strcmp(argv[argcur], "high") == 0)
        {
            config.mPreference = 1;
        }
        else if (strcmp(argv[argcur], "med") == 0)
        {
            config.mPreference = 0;
        }
        else if (strcmp(argv[argcur], "low") == 0)
        {
            config.mPreference = -1;
        }
        else
        {
            for (char *arg = argv[argcur]; *arg != '\0'; arg++)
            {
                switch (*arg)
                {
                case 'p':
                    config.mPreferred = true;
                    break;

                case 'a':
                    config.mSlaac = true;
                    break;

                case 'd':
                    config.mDhcp = true;
                    break;

                case 'c':
                    config.mConfigure = true;
                    break;

                case 'r':
                    config.mDefaultRoute = true;
                    break;

                case 'o':
                    config.mOnMesh = true;
                    break;

                case 's':
                    config.mStable = true;
                    break;

                default:
                    ExitNow(error = kThreadError_Parse);
                }
            }
        }
    }

    error = otAddBorderRouter(mInstance, &config);

exit:
    return error;
}

ThreadError Interpreter::ProcessPrefixRemove(int argc, char *argv[])
{
    ThreadError error = kThreadError_None;
    struct otIp6Prefix prefix;
    int argcur = 0;

    memset(&prefix, 0, sizeof(otIp6Prefix));

    char *prefixLengthStr;
    char *endptr;

    if ((prefixLengthStr = strchr(argv[argcur], '/')) == NULL)
    {
        ExitNow();
    }

    *prefixLengthStr++ = '\0';

    SuccessOrExit(error = otIp6AddressFromString(argv[argcur], &prefix.mPrefix));

    prefix.mLength = static_cast<uint8_t>(strtol(prefixLengthStr, &endptr, 0));

    if (*endptr != '\0')
    {
        ExitNow(error = kThreadError_Parse);
    }

    error = otRemoveBorderRouter(mInstance, &prefix);

exit:
    (void)argc;
    return error;
}

ThreadError Interpreter::ProcessPrefixList(void)
{
    otNetworkDataIterator iterator = OT_NETWORK_DATA_ITERATOR_INIT;
    otBorderRouterConfig config;

    while (otGetNextOnMeshPrefix(mInstance, true, &iterator, &config) == kThreadError_None)
    {
        sServer->OutputFormat("%x:%x:%x:%x::/%d ",
                              HostSwap16(config.mPrefix.mPrefix.mFields.m16[0]),
                              HostSwap16(config.mPrefix.mPrefix.mFields.m16[1]),
                              HostSwap16(config.mPrefix.mPrefix.mFields.m16[2]),
                              HostSwap16(config.mPrefix.mPrefix.mFields.m16[3]),
                              config.mPrefix.mLength);

        if (config.mPreferred)
        {
            sServer->OutputFormat("p");
        }

        if (config.mSlaac)
        {
            sServer->OutputFormat("a");
        }

        if (config.mDhcp)
        {
            sServer->OutputFormat("d");
        }

        if (config.mConfigure)
        {
            sServer->OutputFormat("c");
        }

        if (config.mDefaultRoute)
        {
            sServer->OutputFormat("r");
        }

        if (config.mOnMesh)
        {
            sServer->OutputFormat("o");
        }

        if (config.mStable)
        {
            sServer->OutputFormat("s");
        }

        switch (config.mPreference)
        {
        case -1:
            sServer->OutputFormat(" low\r\n");
            break;

        case 0:
            sServer->OutputFormat(" med\r\n");
            break;

        case 1:
            sServer->OutputFormat(" high\r\n");
            break;
        }
    }

    return kThreadError_None;
}

void Interpreter::ProcessPrefix(int argc, char *argv[])
{
    ThreadError error = kThreadError_None;

    if (argc == 0)
    {
        SuccessOrExit(error = ProcessPrefixList());
    }
    else if (strcmp(argv[0], "add") == 0)
    {
        SuccessOrExit(error = ProcessPrefixAdd(argc - 1, argv + 1));
    }
    else if (strcmp(argv[0], "remove") == 0)
    {
        SuccessOrExit(error = ProcessPrefixRemove(argc - 1, argv + 1));
    }
    else
    {
        ExitNow(error = kThreadError_Parse);
    }

exit:
    AppendResult(error);
}

void Interpreter::ProcessReleaseRouterId(int argc, char *argv[])
{
    ThreadError error = kThreadError_None;
    long value;

    VerifyOrExit(argc > 0, error = kThreadError_Parse);

    SuccessOrExit(error = ParseLong(argv[0], value));
    SuccessOrExit(error = otReleaseRouterId(mInstance, static_cast<uint8_t>(value)));

exit:
    AppendResult(error);
}

void Interpreter::ProcessReset(int argc, char *argv[])
{
    otPlatformReset(mInstance);
    (void)argc;
    (void)argv;
}

void Interpreter::ProcessRloc16(int argc, char *argv[])
{
    sServer->OutputFormat("%04x\r\n", otGetRloc16(mInstance));
    sServer->OutputFormat("Done\r\n");
    (void)argc;
    (void)argv;
}

ThreadError Interpreter::ProcessRouteAdd(int argc, char *argv[])
{
    ThreadError error = kThreadError_None;
    otExternalRouteConfig config;
    int argcur = 0;

    memset(&config, 0, sizeof(otExternalRouteConfig));

    char *prefixLengthStr;
    char *endptr;

    VerifyOrExit(argc > 0, error = kThreadError_Parse);

    if ((prefixLengthStr = strchr(argv[argcur], '/')) == NULL)
    {
        ExitNow();
    }

    *prefixLengthStr++ = '\0';

    SuccessOrExit(error = otIp6AddressFromString(argv[argcur], &config.mPrefix.mPrefix));

    config.mPrefix.mLength = static_cast<uint8_t>(strtol(prefixLengthStr, &endptr, 0));

    if (*endptr != '\0')
    {
        ExitNow(error = kThreadError_Parse);
    }

    argcur++;

    for (; argcur < argc; argcur++)
    {
        if (strcmp(argv[argcur], "s") == 0)
        {
            config.mStable = true;
        }
        else if (strcmp(argv[argcur], "high") == 0)
        {
            config.mPreference = 1;
        }
        else if (strcmp(argv[argcur], "med") == 0)
        {
            config.mPreference = 0;
        }
        else if (strcmp(argv[argcur], "low") == 0)
        {
            config.mPreference = -1;
        }
        else
        {
            ExitNow(error = kThreadError_Parse);
        }
    }

    error = otAddExternalRoute(mInstance, &config);

exit:
    return error;
}

ThreadError Interpreter::ProcessRouteRemove(int argc, char *argv[])
{
    ThreadError error = kThreadError_None;
    struct otIp6Prefix prefix;
    int argcur = 0;

    memset(&prefix, 0, sizeof(struct otIp6Prefix));
    char *prefixLengthStr;
    char *endptr;

    VerifyOrExit(argc > 0, error = kThreadError_Parse);

    if ((prefixLengthStr = strchr(argv[argcur], '/')) == NULL)
    {
        ExitNow();
    }

    *prefixLengthStr++ = '\0';

    SuccessOrExit(error = otIp6AddressFromString(argv[argcur], &prefix.mPrefix));

    prefix.mLength = static_cast<uint8_t>(strtol(prefixLengthStr, &endptr, 0));

    if (*endptr != '\0')
    {
        ExitNow(error = kThreadError_Parse);
    }

    error = otRemoveExternalRoute(mInstance, &prefix);

exit:
    return error;
}

void Interpreter::ProcessRoute(int argc, char *argv[])
{
    ThreadError error = kThreadError_None;

    VerifyOrExit(argc > 0, error = kThreadError_Parse);

    if (strcmp(argv[0], "add") == 0)
    {
        SuccessOrExit(error = ProcessRouteAdd(argc - 1, argv + 1));
    }
    else if (strcmp(argv[0], "remove") == 0)
    {
        SuccessOrExit(error = ProcessRouteRemove(argc - 1, argv + 1));
    }
    else
    {
        ExitNow(error = kThreadError_Parse);
    }

exit:
    AppendResult(error);
}

void Interpreter::ProcessRouter(int argc, char *argv[])
{
    ThreadError error = kThreadError_None;
    otRouterInfo routerInfo;
    long value;
    bool isTable = false;

    VerifyOrExit(argc > 0, error = kThreadError_Parse);

    if (strcmp(argv[0], "list") == 0 || (isTable = (strcmp(argv[0], "table") == 0)))
    {
        if (isTable)
        {
            sServer->OutputFormat("| ID | RLOC16 | Next Hop | Path Cost | LQI In | LQI Out | Age | Extended MAC     |\r\n");
            sServer->OutputFormat("+----+--------+----------+-----------+--------+---------+-----+------------------+\r\n");
        }

        for (uint8_t i = 0; ; i++)
        {
            if (otGetRouterInfo(mInstance, i, &routerInfo) != kThreadError_None)
            {
                sServer->OutputFormat("\r\n");
                ExitNow();
            }

            if (routerInfo.mAllocated)
            {
                if (isTable)
                {
                    sServer->OutputFormat("| %2d ", routerInfo.mRouterId);
                    sServer->OutputFormat("| 0x%04x ", routerInfo.mRloc16);
                    sServer->OutputFormat("| %8d ", routerInfo.mNextHop);
                    sServer->OutputFormat("| %9d ", routerInfo.mPathCost);
                    sServer->OutputFormat("| %6d ", routerInfo.mLinkQualityIn);
                    sServer->OutputFormat("| %7d ", routerInfo.mLinkQualityOut);
                    sServer->OutputFormat("| %3d ", routerInfo.mAge);
                    sServer->OutputFormat("| ");

                    for (size_t j = 0; j < sizeof(routerInfo.mExtAddress); j++)
                    {
                        sServer->OutputFormat("%02x", routerInfo.mExtAddress.m8[j]);
                    }

                    sServer->OutputFormat(" |\r\n");
                }
                else
                {
                    sServer->OutputFormat("%d ", i);
                }
            }
        }
    }

    SuccessOrExit(error = ParseLong(argv[0], value));
    SuccessOrExit(error = otGetRouterInfo(mInstance, static_cast<uint16_t>(value), &routerInfo));

    sServer->OutputFormat("Alloc: %d\r\n", routerInfo.mAllocated);

    if (routerInfo.mAllocated)
    {
        sServer->OutputFormat("Router ID: %d\r\n", routerInfo.mRouterId);
        sServer->OutputFormat("Rloc: %04x\r\n", routerInfo.mRloc16);
        sServer->OutputFormat("Next Hop: %04x\r\n", static_cast<uint16_t>(routerInfo.mNextHop) << 10);
        sServer->OutputFormat("Link: %d\r\n", routerInfo.mLinkEstablished);

        if (routerInfo.mLinkEstablished)
        {
            sServer->OutputFormat("Ext Addr: ");

            for (size_t j = 0; j < sizeof(routerInfo.mExtAddress); j++)
            {
                sServer->OutputFormat("%02x", routerInfo.mExtAddress.m8[j]);
            }

            sServer->OutputFormat("\r\n");
            sServer->OutputFormat("Cost: %d\r\n", routerInfo.mPathCost);
            sServer->OutputFormat("LQI In: %d\r\n", routerInfo.mLinkQualityIn);
            sServer->OutputFormat("LQI Out: %d\r\n", routerInfo.mLinkQualityOut);
            sServer->OutputFormat("Age: %d\r\n", routerInfo.mAge);
        }
    }

exit:
    AppendResult(error);
}

void Interpreter::ProcessRouterDowngradeThreshold(int argc, char *argv[])
{
    ThreadError error = kThreadError_None;
    long value;

    if (argc == 0)
    {
        sServer->OutputFormat("%d\r\n", otGetRouterDowngradeThreshold());
    }
    else
    {
        SuccessOrExit(error = ParseLong(argv[0], value));
        otSetRouterDowngradeThreshold(static_cast<uint8_t>(value));
    }

exit:
    AppendResult(error);
}

void Interpreter::ProcessRouterRole(int argc, char *argv[])
{
    ThreadError error = kThreadError_None;

    if (argc == 0)
    {
        if (otIsRouterRoleEnabled(mInstance))
        {
            sServer->OutputFormat("Enabled\r\n");
        }
        else
        {
            sServer->OutputFormat("Disabled\r\n");
        }
    }
    else if (strcmp(argv[0], "enable") == 0)
    {
        otSetRouterRoleEnabled(mInstance, true);
    }
    else if (strcmp(argv[0], "disable") == 0)
    {
        otSetRouterRoleEnabled(mInstance, false);
    }
    else
    {
        ExitNow(error = kThreadError_Parse);
    }

exit:
    AppendResult(error);
}

void Interpreter::ProcessRouterUpgradeThreshold(int argc, char *argv[])
{
    ThreadError error = kThreadError_None;
    long value;

    if (argc == 0)
    {
        sServer->OutputFormat("%d\r\n", otGetRouterUpgradeThreshold(mInstance));
    }
    else
    {
        SuccessOrExit(error = ParseLong(argv[0], value));
        otSetRouterUpgradeThreshold(mInstance, static_cast<uint8_t>(value));
    }

exit:
    AppendResult(error);
}

void Interpreter::ProcessScan(int argc, char *argv[])
{
    ThreadError error = kThreadError_None;
    uint32_t scanChannels = 0;
    long value;

    if (argc > 0)
    {
        SuccessOrExit(error = ParseLong(argv[0], value));
        scanChannels = 1 << value;
    }

    SuccessOrExit(error = otActiveScan(mInstance, scanChannels, 0, &Interpreter::s_HandleActiveScanResult, this));
    sServer->OutputFormat("| J | Network Name     | Extended PAN     | PAN  | MAC Address      | Ch | dBm | LQI |\r\n");
    sServer->OutputFormat("+---+------------------+------------------+------+------------------+----+-----+-----+\r\n");

    return;

exit:
    AppendResult(error);
}

void Interpreter::s_HandleActiveScanResult(otActiveScanResult *aResult, void *aContext)
{
    static_cast<Interpreter *>(aContext)->HandleActiveScanResult(aResult);
}

void Interpreter::HandleActiveScanResult(otActiveScanResult *aResult)
{
    if (aResult == NULL)
    {
        sServer->OutputFormat("Done\r\n");
        ExitNow();
    }

    sServer->OutputFormat("| %d ", aResult->mIsJoinable);

    sServer->OutputFormat("| %-16s ", aResult->mNetworkName.m8);

    sServer->OutputFormat("| ");
    OutputBytes(aResult->mExtendedPanId.m8, OT_EXT_PAN_ID_SIZE);
    sServer->OutputFormat(" ");

    sServer->OutputFormat("| %04x | ", aResult->mPanId);
    OutputBytes(aResult->mExtAddress.m8, OT_EXT_ADDRESS_SIZE);
    sServer->OutputFormat(" | %2d ", aResult->mChannel);
    sServer->OutputFormat("| %3d ", aResult->mRssi);
    sServer->OutputFormat("| %3d |\r\n", aResult->mLqi);

exit:
    return;
}

void Interpreter::ProcessSingleton(int argc, char *argv[])
{
    ThreadError error = kThreadError_None;

    if (otIsSingleton(mInstance))
    {
        sServer->OutputFormat("true\r\n");
    }
    else
    {
        sServer->OutputFormat("false\r\n");
    }

    (void)argc;
    (void)argv;

    AppendResult(error);
}

void Interpreter::ProcessState(int argc, char *argv[])
{
    ThreadError error = kThreadError_None;

    if (argc == 0)
    {
        switch (otGetDeviceRole(mInstance))
        {
        case kDeviceRoleDisabled:
            sServer->OutputFormat("disabled\r\n");
            break;

        case kDeviceRoleDetached:
            sServer->OutputFormat("detached\r\n");
            break;

        case kDeviceRoleChild:
            sServer->OutputFormat("child\r\n");
            break;

        case kDeviceRoleRouter:
            sServer->OutputFormat("router\r\n");
            break;

        case kDeviceRoleLeader:
            sServer->OutputFormat("leader\r\n");
            break;
        }
    }
    else
    {
        if (strcmp(argv[0], "detached") == 0)
        {
            SuccessOrExit(error = otBecomeDetached(mInstance));
        }
        else if (strcmp(argv[0], "child") == 0)
        {
            SuccessOrExit(error = otBecomeChild(mInstance, kMleAttachSamePartition));
        }
        else if (strcmp(argv[0], "router") == 0)
        {
            SuccessOrExit(error = otBecomeRouter(mInstance));
        }
        else if (strcmp(argv[0], "leader") == 0)
        {
            SuccessOrExit(error = otBecomeLeader(mInstance));
        }
        else
        {
            ExitNow(error = kThreadError_Parse);
        }
    }

exit:
    AppendResult(error);
}

void Interpreter::ProcessThread(int argc, char *argv[])
{
    ThreadError error = kThreadError_Parse;

    VerifyOrExit(argc > 0, error = kThreadError_Parse);

    if (strcmp(argv[0], "start") == 0)
    {
        SuccessOrExit(error = otThreadStart(mInstance));
    }
    else if (strcmp(argv[0], "stop") == 0)
    {
        SuccessOrExit(error = otThreadStop(mInstance));
    }

exit:
    (void)argc;
    (void)argv;
    AppendResult(error);
}

void Interpreter::ProcessVersion(int argc, char *argv[])
{
    sServer->OutputFormat("%s\r\n", otGetVersionString());
    AppendResult(kThreadError_None);
    (void)argc;
    (void)argv;
}

#if OPENTHREAD_ENABLE_COMMISSIONER

void Interpreter::ProcessCommissioner(int argc, char *argv[])
{
    ThreadError error = kThreadError_None;

    VerifyOrExit(argc > 0, error = kThreadError_Parse);

    if (strcmp(argv[0], "start") == 0)
    {
        const char *provisioningUrl;
        VerifyOrExit(argc > 1, error = kThreadError_Parse);
        provisioningUrl = argc > 2 ? argv[2] : NULL;
        otCommissionerStart(mInstance, argv[1], provisioningUrl);
    }
    else if (strcmp(argv[0], "stop") == 0)
    {
        otCommissionerStop(mInstance);
    }
    else if (strcmp(argv[0], "energy") == 0)
    {
        long mask;
        long count;
        long period;
        long scanDuration;
        otIp6Address address;

        VerifyOrExit(argc > 5, error = kThreadError_Parse);

        // mask
        SuccessOrExit(error = ParseLong(argv[1], mask));

        // count
        SuccessOrExit(error = ParseLong(argv[2], count));

        // period
        SuccessOrExit(error = ParseLong(argv[3], period));

        // scan duration
        SuccessOrExit(error = ParseLong(argv[4], scanDuration));

        // destination
        SuccessOrExit(error = otIp6AddressFromString(argv[5], &address));

        SuccessOrExit(error = otCommissionerEnergyScan(mInstance,
                                                       static_cast<uint32_t>(mask),
                                                       static_cast<uint8_t>(count),
                                                       static_cast<uint16_t>(period),
                                                       static_cast<uint16_t>(scanDuration),
                                                       &address,
                                                       Interpreter::s_HandleEnergyReport,
                                                       this));
    }
    else if (strcmp(argv[0], "panid") == 0)
    {
        long panid;
        long mask;
        otIp6Address address;

        VerifyOrExit(argc > 3, error = kThreadError_Parse);

        // panid
        SuccessOrExit(error = ParseLong(argv[1], panid));

        // mask
        SuccessOrExit(error = ParseLong(argv[2], mask));

        // destination
        SuccessOrExit(error = otIp6AddressFromString(argv[3], &address));

        SuccessOrExit(error = otCommissionerPanIdQuery(mInstance,
                                                       static_cast<uint16_t>(panid),
                                                       static_cast<uint32_t>(mask),
                                                       &address,
                                                       Interpreter::s_HandlePanIdConflict,
                                                       this));
    }
    else if (strcmp(argv[0], "mgmtget") == 0)
    {
        uint8_t tlvs[32];
        long value;
        int length = 0;

        for (uint8_t index = 1; index < argc; index++)
        {
            VerifyOrExit(static_cast<size_t>(length) < sizeof(tlvs), error = kThreadError_NoBufs);

            if (strcmp(argv[index], "locator") == 0)
            {
                tlvs[length++] = OT_MESHCOP_TLV_BORDER_AGENT_RLOC;
            }
            else if (strcmp(argv[index], "sessionid") == 0)
            {
                tlvs[length++] = OT_MESHCOP_TLV_COMM_SESSION_ID;
            }
            else if (strcmp(argv[index], "steeringdata") == 0)
            {
                tlvs[length++] = OT_MESHCOP_TLV_STEERING_DATA;
            }
            else if (strcmp(argv[index], "joinerudpport") == 0)
            {
                tlvs[length++] = OT_MESHCOP_TLV_JOINER_UDP_PORT;
            }
            else if (strcmp(argv[index], "binary") == 0)
            {
                VerifyOrExit((index + 1) < argc, error = kThreadError_Parse);
                value = static_cast<long>(strlen(argv[++index]) + 1) / 2;
                VerifyOrExit(static_cast<size_t>(value) <= (sizeof(tlvs) - static_cast<size_t>(length)), error = kThreadError_NoBufs);
                VerifyOrExit(Interpreter::Hex2Bin(argv[index], tlvs + length, static_cast<uint16_t>(value)) >= 0,
                             error = kThreadError_Parse);
                length += value;
            }
            else
            {
                ExitNow(error = kThreadError_Parse);
            }
        }

        SuccessOrExit(error = otSendMgmtCommissionerGet(mInstance, tlvs, static_cast<uint8_t>(length)));
    }
    else if (strcmp(argv[0], "mgmtset") == 0)
    {
        otCommissioningDataset dataset;
        uint8_t tlvs[32];
        long value;
        int length = 0;

        VerifyOrExit(argc > 0, error = kThreadError_Parse);

        memset(&dataset, 0, sizeof(dataset));

        for (uint8_t index = 1; index < argc; index++)
        {
            VerifyOrExit(static_cast<size_t>(length) < sizeof(tlvs), error = kThreadError_NoBufs);

            if (strcmp(argv[index], "locator") == 0)
            {
                VerifyOrExit(index < argc, error = kThreadError_Parse);
                dataset.mIsLocatorSet = true;
                SuccessOrExit(error = Interpreter::ParseLong(argv[++index], value));
                dataset.mLocator = static_cast<uint16_t>(value);
            }
            else if (strcmp(argv[index], "sessionid") == 0)
            {
                VerifyOrExit(index < argc, error = kThreadError_Parse);
                dataset.mIsSessionIdSet = true;
                SuccessOrExit(error = Interpreter::ParseLong(argv[++index], value));
                dataset.mSessionId = static_cast<uint16_t>(value);
            }
            else if (strcmp(argv[index], "steeringdata") == 0)
            {
                VerifyOrExit((index + 1) < argc, error = kThreadError_Parse);
                dataset.mIsSteeringDataSet = true;
                length = static_cast<int>((strlen(argv[++index]) + 1) / 2);
                VerifyOrExit(static_cast<size_t>(length) <= OT_STEERING_DATA_MAX_LENGTH, error = kThreadError_NoBufs);
                VerifyOrExit(Interpreter::Hex2Bin(argv[index], dataset.mSteeringData.m8, static_cast<uint16_t>(length)) >= 0,
                             error = kThreadError_Parse);
                dataset.mSteeringData.mLength = static_cast<uint8_t>(length);
                length = 0;
            }
            else if (strcmp(argv[index], "joinerudpport") == 0)
            {
                VerifyOrExit(index < argc, error = kThreadError_Parse);
                dataset.mIsJoinerUdpPortSet = true;
                SuccessOrExit(error = Interpreter::ParseLong(argv[++index], value));
                dataset.mJoinerUdpPort = static_cast<uint16_t>(value);
            }
            else if (strcmp(argv[index], "binary") == 0)
            {
                VerifyOrExit((index + 1) < argc, error = kThreadError_Parse);
                length = static_cast<int>((strlen(argv[++index]) + 1) / 2);
                VerifyOrExit(static_cast<size_t>(length) <= sizeof(tlvs), error = kThreadError_NoBufs);
                VerifyOrExit(Interpreter::Hex2Bin(argv[index], tlvs, static_cast<uint16_t>(length)) >= 0,
                             error = kThreadError_Parse);
            }
            else
            {
                ExitNow(error = kThreadError_Parse);
            }
        }

        SuccessOrExit(error = otSendMgmtCommissionerSet(mInstance, &dataset, tlvs, static_cast<uint8_t>(length)));
    }

exit:
    AppendResult(error);
}

void Interpreter::s_HandleEnergyReport(uint32_t aChannelMask, const uint8_t *aEnergyList, uint8_t aEnergyListLength,
                                       void *aContext)
{
    static_cast<Interpreter *>(aContext)->HandleEnergyReport(aChannelMask, aEnergyList, aEnergyListLength);
}

void Interpreter::HandleEnergyReport(uint32_t aChannelMask, const uint8_t *aEnergyList, uint8_t aEnergyListLength)
{
    sServer->OutputFormat("Energy: %08x ", aChannelMask);

    for (uint8_t i = 0; i < aEnergyListLength; i++)
    {
        sServer->OutputFormat("%d ", aEnergyList[i]);
    }

    sServer->OutputFormat("\r\n");
}

void Interpreter::s_HandlePanIdConflict(uint16_t aPanId, uint32_t aChannelMask, void *aContext)
{
    static_cast<Interpreter *>(aContext)->HandlePanIdConflict(aPanId, aChannelMask);
}

void Interpreter::HandlePanIdConflict(uint16_t aPanId, uint32_t aChannelMask)
{
    sServer->OutputFormat("Conflict: %04x, %08x\r\n", aPanId, aChannelMask);
}

#endif  // OPENTHREAD_ENABLE_COMMISSIONER

#if OPENTHREAD_ENABLE_JOINER

void Interpreter::ProcessJoiner(int argc, char *argv[])
{
    ThreadError error = kThreadError_None;

    VerifyOrExit(argc > 0, error = kThreadError_Parse);

    if (strcmp(argv[0], "start") == 0)
    {
        const char *provisioningUrl;
        VerifyOrExit(argc > 1, error = kThreadError_Parse);
        provisioningUrl = (argc > 2) ? argv[2] : NULL;
        otJoinerStart(mInstance, argv[1], provisioningUrl);
    }
    else if (strcmp(argv[0], "stop") == 0)
    {
        otJoinerStop(mInstance);
    }

exit:
    AppendResult(error);
}

#endif // OPENTHREAD_ENABLE_JOINER

void Interpreter::ProcessWhitelist(int argc, char *argv[])
{
    ThreadError error = kThreadError_None;
    otMacWhitelistEntry entry;
    int argcur = 0;
    uint8_t extAddr[8];
    int8_t rssi;

    if (argcur >= argc)
    {
        if (otIsMacWhitelistEnabled(mInstance))
        {
            sServer->OutputFormat("Enabled\r\n");
        }
        else
        {
            sServer->OutputFormat("Disabled\r\n");
        }

        for (uint8_t i = 0; ; i++)
        {
            if (otGetMacWhitelistEntry(mInstance, i, &entry) != kThreadError_None)
            {
                break;
            }

            if (entry.mValid == false)
            {
                continue;
            }

            OutputBytes(entry.mExtAddress.m8, OT_EXT_ADDRESS_SIZE);

            if (entry.mFixedRssi)
            {
                sServer->OutputFormat(" %d", entry.mRssi);
            }

            sServer->OutputFormat("\r\n");
        }
    }
    else if (strcmp(argv[argcur], "add") == 0)
    {
        VerifyOrExit(++argcur < argc, error = kThreadError_Parse);
        VerifyOrExit(Hex2Bin(argv[argcur], extAddr, sizeof(extAddr)) == sizeof(extAddr), error = kThreadError_Parse);

        if (++argcur < argc)
        {
            rssi = static_cast<int8_t>(strtol(argv[argcur], NULL, 0));
            VerifyOrExit(otAddMacWhitelistRssi(mInstance, extAddr, rssi) == kThreadError_None, error = kThreadError_Parse);
        }
        else
        {
            otAddMacWhitelist(mInstance, extAddr);
            VerifyOrExit(otAddMacWhitelist(mInstance, extAddr) == kThreadError_None, error = kThreadError_Parse);
        }
    }
    else if (strcmp(argv[argcur], "clear") == 0)
    {
        otClearMacWhitelist(mInstance);
    }
    else if (strcmp(argv[argcur], "disable") == 0)
    {
        otDisableMacWhitelist(mInstance);
    }
    else if (strcmp(argv[argcur], "enable") == 0)
    {
        otEnableMacWhitelist(mInstance);
    }
    else if (strcmp(argv[argcur], "remove") == 0)
    {
        VerifyOrExit(++argcur < argc, error = kThreadError_Parse);
        VerifyOrExit(Hex2Bin(argv[argcur], extAddr, sizeof(extAddr)) == sizeof(extAddr), error = kThreadError_Parse);
        otRemoveMacWhitelist(mInstance, extAddr);
    }

exit:
    AppendResult(error);
}

#if OPENTHREAD_ENABLE_DIAG
void Interpreter::ProcessDiag(int argc, char *argv[])
{
    // all diagnostics related features are processed within diagnostics module
    sServer->OutputFormat("%s\r\n", diagProcessCmd(argc, argv));
}
#endif

void Interpreter::ProcessLine(char *aBuf, uint16_t aBufLength, Server &aServer)
{
    char *argv[kMaxArgs];
    int argc = 0;
    char *cmd;

    sServer = &aServer;

    VerifyOrExit(aBuf != NULL, ;);

    for (; *aBuf == ' '; aBuf++, aBufLength--);

    for (cmd = aBuf + 1; (cmd < aBuf + aBufLength) && (cmd != NULL); ++cmd)
    {
        VerifyOrExit(argc < kMaxArgs,
                     sServer->OutputFormat("Error: too many args (max %d)\r\n", kMaxArgs));

        if (*cmd == ' ' || *cmd == '\r' || *cmd == '\n')
        {
            *cmd = '\0';
        }

        if (*(cmd - 1) == '\0' && *cmd != ' ')
        {
            argv[argc++] = cmd;
        }
    }

    cmd = aBuf;

#if OPENTHREAD_ENABLE_DIAG
    VerifyOrExit((!isDiagEnabled() || (strcmp(cmd, "diag") == 0)),
                 sServer->OutputFormat("under diagnostics mode, execute 'diag stop' before running any other commands.\r\n"));
#endif

    for (unsigned int i = 0; i < sizeof(sCommands) / sizeof(sCommands[0]); i++)
    {
        if (strcmp(cmd, sCommands[i].mName) == 0)
        {
            (this->*sCommands[i].mCommand)(argc, argv);
            break;
        }
    }

exit:
    return;
}

void Interpreter::s_HandleNetifStateChanged(uint32_t aFlags, void *aContext)
{
    static_cast<Interpreter *>(aContext)->HandleNetifStateChanged(aFlags);
}

void Interpreter::HandleNetifStateChanged(uint32_t aFlags)
{
    otNetworkDataIterator iterator;
    otBorderRouterConfig config;

    VerifyOrExit((aFlags & OT_THREAD_NETDATA_UPDATED) != 0, ;);

    // remove addresses
    for (size_t i = 0; i < sizeof(sAutoAddresses) / sizeof(sAutoAddresses[0]); i++)
    {
        otNetifAddress *address = &sAutoAddresses[i];
        bool found = false;

        if (address->mValidLifetime == 0)
        {
            continue;
        }

        iterator = OT_NETWORK_DATA_ITERATOR_INIT;

        while (otGetNextOnMeshPrefix(mInstance, false, &iterator, &config) == kThreadError_None)
        {
            if (config.mSlaac == false)
            {
                continue;
            }

            if (otIp6PrefixMatch(&config.mPrefix.mPrefix, &address->mAddress) >= config.mPrefix.mLength &&
                config.mPrefix.mLength == address->mPrefixLength)
            {
                found = true;
                break;
            }
        }

        if (!found)
        {
            otRemoveUnicastAddress(mInstance, &address->mAddress);
            address->mValidLifetime = 0;
        }
    }

    // add addresses
    iterator = OT_NETWORK_DATA_ITERATOR_INIT;

    while (otGetNextOnMeshPrefix(mInstance, false, &iterator, &config) == kThreadError_None)
    {
        bool found = false;

        if (config.mSlaac == false)
        {
            continue;
        }

        for (size_t i = 0; i < sizeof(sAutoAddresses) / sizeof(sAutoAddresses[0]); i++)
        {
            otNetifAddress *address = &sAutoAddresses[i];

            if (address->mValidLifetime == 0)
            {
                continue;
            }

            if (otIp6PrefixMatch(&config.mPrefix.mPrefix, &address->mAddress) >= config.mPrefix.mLength &&
                config.mPrefix.mLength == address->mPrefixLength)
            {
                found = true;
                break;
            }
        }

        if (!found)
        {
            for (size_t i = 0; i < sizeof(sAutoAddresses) / sizeof(sAutoAddresses[0]); i++)
            {
                otNetifAddress *address = &sAutoAddresses[i];

                if (address->mValidLifetime != 0)
                {
                    continue;
                }

                memset(address, 0, sizeof(*address));
                memcpy(&address->mAddress, &config.mPrefix.mPrefix, 8);

                for (size_t j = 8; j < sizeof(address->mAddress); j++)
                {
                    address->mAddress.mFields.m8[j] = (uint8_t)otPlatRandomGet();
                }

                address->mPrefixLength = config.mPrefix.mLength;
                address->mPreferredLifetime = config.mPreferred ? 0xffffffff : 0;
                address->mValidLifetime = 0xffffffff;
                otAddUnicastAddress(mInstance, address);
                break;
            }
        }
    }

exit:
    return;
}

void Interpreter::ProcessNetworkDiagnostic(int argc, char *argv[])
{
    ThreadError error = kThreadError_None;
    struct otIp6Address address;
    uint8_t index = 2;
    uint8_t tlvTypes[OT_NUM_NETDIAG_TLV_TYPES];
    uint8_t count = 0;

    VerifyOrExit(argc > 1 + 1, error = kThreadError_Parse);

    SuccessOrExit(error = otIp6AddressFromString(argv[1], &address));

    while (index < argc && count < sizeof(tlvTypes))
    {
        long value;
        SuccessOrExit(error = ParseLong(argv[index], value));
        tlvTypes[count++] = value;
        index++;
    }

    if (strcmp(argv[0], "get") == 0)
    {
        otSendDiagnosticGet(mInstance, &address, tlvTypes, count);
    }
    else if (strcmp(argv[0], "reset") == 0)
    {
        otSendDiagnosticReset(mInstance, &address, tlvTypes, count);
    }

exit:
    AppendResult(error);
}

}  // namespace Cli
}  // namespace Thread
