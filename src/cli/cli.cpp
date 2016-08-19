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
 *   This file implements the CLI interpreter.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <openthread.h>
#include <openthreadcontext.h>
#include <openthread-config.h>
#include <openthread-diag.h>

#include "cli.hpp"
#include "cli_dataset.hpp"
#include "cli_uart.hpp"
#include <common/encoding.hpp>
#include <common/new.hpp>
#include <platform/random.h>
#include <platform/uart.h>

using Thread::Encoding::BigEndian::HostSwap16;
using Thread::Encoding::BigEndian::HostSwap32;

namespace Thread {
namespace Cli {

const struct Command Interpreter::sCommands[] =
{
    { "help", &Interpreter::ProcessHelp },
    { "blacklist", &Interpreter::ProcessBlacklist },
    { "channel", &Interpreter::ProcessChannel },
    { "child", &Interpreter::ProcessChild },
    { "childtimeout", &Interpreter::ProcessChildTimeout },
    { "contextreusedelay", &Interpreter::ProcessContextIdReuseDelay },
    { "counter", &Interpreter::ProcessCounters },
    { "dataset", &Interpreter::ProcessDataset },
    { "discover", &Interpreter::ProcessDiscover },
    { "eidcache", &Interpreter::ProcessEidCache },
    { "extaddr", &Interpreter::ProcessExtAddress },
    { "extpanid", &Interpreter::ProcessExtPanId },
    { "ifconfig", &Interpreter::ProcessIfconfig },
    { "ipaddr", &Interpreter::ProcessIpAddr },
    { "keysequence", &Interpreter::ProcessKeySequence },
    { "leaderdata", &Interpreter::ProcessLeaderData },
    { "leaderpartitionid", &Interpreter::ProcessLeaderPartitionId },
    { "leaderweight", &Interpreter::ProcessLeaderWeight },
    { "linkquality", &Interpreter::ProcessLinkQuality },
    { "masterkey", &Interpreter::ProcessMasterKey },
    { "mode", &Interpreter::ProcessMode },
    { "netdataregister", &Interpreter::ProcessNetworkDataRegister },
    { "networkidtimeout", &Interpreter::ProcessNetworkIdTimeout },
    { "networkname", &Interpreter::ProcessNetworkName },
    { "panid", &Interpreter::ProcessPanId },
    { "parent", &Interpreter::ProcessParent },
    { "ping", &Interpreter::ProcessPing },
    { "pollperiod", &Interpreter::ProcessPollPeriod },
    { "prefix", &Interpreter::ProcessPrefix },
    { "releaserouterid", &Interpreter::ProcessReleaseRouterId },
    { "reset", &Interpreter::ProcessReset },
    { "rloc16", &Interpreter::ProcessRloc16 },
    { "route", &Interpreter::ProcessRoute },
    { "router", &Interpreter::ProcessRouter },
    { "routerupgradethreshold", &Interpreter::ProcessRouterUpgradeThreshold },
    { "scan", &Interpreter::ProcessScan },
    { "singleton", &Interpreter::ProcessSingleton },
    { "state", &Interpreter::ProcessState },
    { "thread", &Interpreter::ProcessThread },
    { "version", &Interpreter::ProcessVersion },
    { "whitelist", &Interpreter::ProcessWhitelist },
#if OPENTHREAD_ENABLE_DIAG
    { "diag", &Interpreter::ProcessDiag },
#endif
};

static otNetifAddress sAutoAddresses[8];

Interpreter::Interpreter(otContext *aContext):
    sIcmpEcho(aContext, &Interpreter::s_HandleEchoResponse, this),
    sLength(8),
    sCount(1),
    sInterval(1000),
    sPingTimer(aContext, &Interpreter::s_HandlePingTimer, this),
    mContext(aContext)
{
    otSetStateChangedCallback(mContext, &Interpreter::s_HandleNetifStateChanged, this);
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
        if (otIsMacBlacklistEnabled(mContext))
        {
            sServer->OutputFormat("Enabled\r\n");
        }
        else
        {
            sServer->OutputFormat("Disabled\r\n");
        }

        for (uint8_t i = 0; ; i++)
        {
            if (otGetMacBlacklistEntry(mContext, i, &entry) != kThreadError_None)
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

        otAddMacBlacklist(mContext, extAddr);
        VerifyOrExit(otAddMacBlacklist(mContext, extAddr) == kThreadError_None, error = kThreadError_Parse);
    }
    else if (strcmp(argv[argcur], "clear") == 0)
    {
        otClearMacBlacklist(mContext);
    }
    else if (strcmp(argv[argcur], "disable") == 0)
    {
        otDisableMacBlacklist(mContext);
    }
    else if (strcmp(argv[argcur], "enable") == 0)
    {
        otEnableMacBlacklist(mContext);
    }
    else if (strcmp(argv[argcur], "remove") == 0)
    {
        VerifyOrExit(++argcur < argc, error = kThreadError_Parse);
        VerifyOrExit(Hex2Bin(argv[argcur], extAddr, sizeof(extAddr)) == sizeof(extAddr), error = kThreadError_Parse);
        otRemoveMacBlacklist(mContext, extAddr);
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
        sServer->OutputFormat("%d\r\n", otGetChannel(mContext));
    }
    else
    {
        SuccessOrExit(error = ParseLong(argv[0], value));
        otSetChannel(mContext, static_cast<uint8_t>(value));
    }

exit:
    AppendResult(error);
}

void Interpreter::ProcessChild(int argc, char *argv[])
{
    ThreadError error = kThreadError_None;
    otChildInfo childInfo;
    long value;

    VerifyOrExit(argc > 0, error = kThreadError_Parse);

    if (strcmp(argv[0], "list") == 0)
    {
        for (uint8_t i = 0; ; i++)
        {
            if (otGetChildInfoByIndex(mContext, i, &childInfo) != kThreadError_None)
            {
                sServer->OutputFormat("\r\n");
                ExitNow();
            }

            if (childInfo.mTimeout > 0)
            {
                sServer->OutputFormat("%d ", childInfo.mChildId);
            }
        }
    }

    SuccessOrExit(error = ParseLong(argv[0], value));
    SuccessOrExit(error = otGetChildInfoById(mContext, static_cast<uint8_t>(value), &childInfo));

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

void Interpreter::ProcessChildTimeout(int argc, char *argv[])
{
    ThreadError error = kThreadError_None;
    long value;

    if (argc == 0)
    {
        sServer->OutputFormat("%d\r\n", otGetChildTimeout(mContext));
    }
    else
    {
        SuccessOrExit(error = ParseLong(argv[0], value));
        otSetChildTimeout(mContext, static_cast<uint32_t>(value));
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
        sServer->OutputFormat("%d\r\n", otGetContextIdReuseDelay(mContext));
    }
    else
    {
        SuccessOrExit(ParseLong(argv[0], value));
        otSetContextIdReuseDelay(mContext, static_cast<uint32_t>(value));
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
            const otMacCounters *counters = otGetMacCounters(mContext);
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
    error = Dataset::Process(mContext, argc, argv, *sServer);
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

    SuccessOrExit(error = otDiscover(mContext, scanChannels, 0, OT_PANID_BROADCAST,
                                     &Interpreter::s_HandleActiveScanResult, NULL));
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
        SuccessOrExit(otGetEidCacheEntry(mContext, i, &entry));

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

void Interpreter::ProcessExtAddress(int argc, char *argv[])
{
    ThreadError error = kThreadError_None;

    if (argc == 0)
    {
        OutputBytes(otGetExtendedAddress(mContext), OT_EXT_ADDRESS_SIZE);
        sServer->OutputFormat("\r\n");
    }
    else
    {
        otExtAddress extAddress;

        VerifyOrExit(Hex2Bin(argv[0], extAddress.m8, sizeof(otExtAddress)) >= 0, error = kThreadError_Parse);

        otSetExtendedAddress(mContext, &extAddress);
    }

exit:
    AppendResult(error);
}

void Interpreter::ProcessExtPanId(int argc, char *argv[])
{
    ThreadError error = kThreadError_None;

    if (argc == 0)
    {
        OutputBytes(otGetExtendedPanId(mContext), OT_EXT_PAN_ID_SIZE);
        sServer->OutputFormat("\r\n");
    }
    else
    {
        uint8_t extPanId[8];

        VerifyOrExit(Hex2Bin(argv[0], extPanId, sizeof(extPanId)) >= 0, error = kThreadError_Parse);

        otSetExtendedPanId(mContext, extPanId);
    }

exit:
    AppendResult(error);
}

void Interpreter::ProcessIfconfig(int argc, char *argv[])
{
    ThreadError error = kThreadError_None;

    if (argc == 0)
    {
        if (otIsInterfaceUp(mContext))
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
        SuccessOrExit(error = otInterfaceUp(mContext));
    }
    else if (strcmp(argv[0], "down") == 0)
    {
        SuccessOrExit(error = otInterfaceDown(mContext));
    }

exit:
    AppendResult(error);
}

ThreadError Interpreter::ProcessIpAddrAdd(int argc, char *argv[])
{
    ThreadError error;

    VerifyOrExit(argc > 0, error = kThreadError_Parse);

    SuccessOrExit(error = otIp6AddressFromString(argv[0], &sAddress.mAddress));
    sAddress.mPrefixLength = 64;
    sAddress.mPreferredLifetime = 0xffffffff;
    sAddress.mValidLifetime = 0xffffffff;
    error = otAddUnicastAddress(mContext, &sAddress);

exit:
    return error;
}

ThreadError Interpreter::ProcessIpAddrDel(int argc, char *argv[])
{
    ThreadError error;
    struct otIp6Address address;

    VerifyOrExit(argc > 0, error = kThreadError_Parse);

    SuccessOrExit(error = otIp6AddressFromString(argv[0], &address));
    VerifyOrExit(otIsIp6AddressEqual(&address, &sAddress.mAddress), error = kThreadError_Parse);
    error = otRemoveUnicastAddress(mContext, &sAddress);

exit:
    return error;
}

void Interpreter::ProcessIpAddr(int argc, char *argv[])
{
    ThreadError error = kThreadError_None;

    if (argc == 0)
    {
        for (const otNetifAddress *addr = otGetUnicastAddresses(mContext); addr; addr = addr->mNext)
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
        sServer->OutputFormat("%d\r\n", otGetKeySequenceCounter(mContext));
    }
    else
    {
        SuccessOrExit(error = ParseLong(argv[0], value));
        otSetKeySequenceCounter(mContext, static_cast<uint32_t>(value));
    }

exit:
    AppendResult(error);
}

void Interpreter::ProcessLeaderData(int argc, char *argv[])
{
    ThreadError error;
    otLeaderData leaderData;

    SuccessOrExit(error = otGetLeaderData(mContext, &leaderData));

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
        sServer->OutputFormat("%u\r\n", otGetLocalLeaderPartitionId(mContext));
    }
    else
    {
        SuccessOrExit(error = ParseUnsignedLong(argv[0], value));
        otSetLocalLeaderPartitionId(mContext, static_cast<uint32_t>(value));
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
        sServer->OutputFormat("%d\r\n", otGetLocalLeaderWeight(mContext));
    }
    else
    {
        SuccessOrExit(error = ParseLong(argv[0], value));
        otSetLocalLeaderWeight(mContext, static_cast<uint8_t>(value));
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
        VerifyOrExit(otGetAssignLinkQuality(mContext, extAddress, &linkQuality) == kThreadError_None,
                     error = kThreadError_InvalidArgs);
        sServer->OutputFormat("%d\r\n", linkQuality);
    }
    else
    {
        SuccessOrExit(error = ParseLong(argv[1], value));
        otSetAssignLinkQuality(mContext, extAddress, static_cast<uint8_t>(value));
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
        const uint8_t *key = otGetMasterKey(mContext, &keyLength);

        for (int i = 0; i < keyLength; i++)
        {
            sServer->OutputFormat("%02x", key[i]);
        }

        sServer->OutputFormat("\r\n");
    }
    else
    {
        int keyLength;
        uint8_t key[16];

        VerifyOrExit((keyLength = Hex2Bin(argv[0], key, sizeof(key))) >= 0, error = kThreadError_Parse);
        SuccessOrExit(error = otSetMasterKey(mContext, key, static_cast<uint8_t>(keyLength)));
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
        linkMode = otGetLinkMode(mContext);

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

        SuccessOrExit(error = otSetLinkMode(mContext, linkMode));
    }

exit:
    AppendResult(error);
}

void Interpreter::ProcessNetworkDataRegister(int argc, char *argv[])
{
    ThreadError error = kThreadError_None;
    SuccessOrExit(error = otSendServerData(mContext));

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
        sServer->OutputFormat("%d\r\n", otGetNetworkIdTimeout(mContext));
    }
    else
    {
        SuccessOrExit(error = ParseLong(argv[0], value));
        otSetNetworkIdTimeout(mContext, static_cast<uint8_t>(value));
    }

exit:
    AppendResult(error);
}

void Interpreter::ProcessNetworkName(int argc, char *argv[])
{
    ThreadError error = kThreadError_None;

    if (argc == 0)
    {
        sServer->OutputFormat("%.*s\r\n", OT_NETWORK_NAME_MAX_SIZE, otGetNetworkName(mContext));
    }
    else
    {
        SuccessOrExit(error = otSetNetworkName(mContext, argv[0]));
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
        sServer->OutputFormat("%d\r\n", otGetPanId(mContext));
    }
    else
    {
        SuccessOrExit(error = ParseLong(argv[0], value));
        otSetPanId(mContext, static_cast<otPanId>(value));
    }

exit:
    AppendResult(error);
}

void Interpreter::ProcessParent(int argc, char *argv[])
{
    ThreadError error = kThreadError_None;
    otRouterInfo parentInfo;

    SuccessOrExit(error = otGetParentInfo(mContext, &parentInfo));
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
    Interpreter *pThis = (Interpreter *)aContext;

    pThis->HandleEchoResponse(aMessage, aMessageInfo);
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

    memset(&sSockAddr, 0, sizeof(sSockAddr));
    SuccessOrExit(error = sSockAddr.GetAddress().FromString(argv[0]));
    sSockAddr.mScopeId = 1;

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
    Interpreter *pThis = (Interpreter *)aContext;

    pThis->HandlePingTimer();
}

void Interpreter::HandlePingTimer()
{
    uint32_t timestamp = HostSwap32(Timer::GetNow());

    memcpy(sEchoRequest, &timestamp, sizeof(timestamp));
    sIcmpEcho.SendEchoRequest(mContext, sSockAddr, sEchoRequest, sLength);
    sCount--;

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
        sServer->OutputFormat("%d\r\n", (otGetPollPeriod(mContext) / 1000));  // ms->s
    }
    else
    {
        SuccessOrExit(error = ParseLong(argv[0], value));
        otSetPollPeriod(mContext, static_cast<uint32_t>(value * 1000));  // s->ms
    }

exit:
    AppendResult(error);
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

    error = otAddBorderRouter(mContext, &config);

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

    error = otRemoveBorderRouter(mContext, &prefix);

exit:
    (void)argc;
    return error;
}

ThreadError Interpreter::ProcessPrefixList(void)
{
    otNetworkDataIterator iterator = OT_NETWORK_DATA_ITERATOR_INIT;
    otBorderRouterConfig config;

    while (otGetNextOnMeshPrefix(mContext, true, &iterator, &config) == kThreadError_None)
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
    SuccessOrExit(error = otReleaseRouterId(mContext, static_cast<uint8_t>(value)));

exit:
    AppendResult(error);
}

void Interpreter::ProcessReset(int argc, char *argv[])
{
    otPlatformReset(mContext);
    (void)argc;
    (void)argv;
}

void Interpreter::ProcessRloc16(int argc, char *argv[])
{
    sServer->OutputFormat("%04x\r\n", otGetRloc16(mContext));
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

    error = otAddExternalRoute(mContext, &config);

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

    error = otRemoveExternalRoute(mContext, &prefix);

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

    VerifyOrExit(argc > 0, error = kThreadError_Parse);

    if (strcmp(argv[0], "list") == 0)
    {
        for (uint8_t i = 0; ; i++)
        {
            if (otGetRouterInfo(mContext, i, &routerInfo) != kThreadError_None)
            {
                sServer->OutputFormat("\r\n");
                ExitNow();
            }

            if (routerInfo.mAllocated)
            {
                sServer->OutputFormat("%d ", i);
            }
        }
    }

    SuccessOrExit(error = ParseLong(argv[0], value));
    SuccessOrExit(error = otGetRouterInfo(mContext, static_cast<uint8_t>(value), &routerInfo));

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

void Interpreter::ProcessRouterUpgradeThreshold(int argc, char *argv[])
{
    ThreadError error = kThreadError_None;
    long value;

    if (argc == 0)
    {
        sServer->OutputFormat("%d\r\n", otGetRouterUpgradeThreshold(mContext));
    }
    else
    {
        SuccessOrExit(error = ParseLong(argv[0], value));
        otSetRouterUpgradeThreshold(mContext, static_cast<uint8_t>(value));
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

    SuccessOrExit(error = otActiveScan(mContext, scanChannels, 0, &Interpreter::s_HandleActiveScanResult, NULL));
    sServer->OutputFormat("| J | Network Name     | Extended PAN     | PAN  | MAC Address      | Ch | dBm | LQI |\r\n");
    sServer->OutputFormat("+---+------------------+------------------+------+------------------+----+-----+-----+\r\n");

    return;

exit:
    AppendResult(error);
}

void Interpreter::s_HandleActiveScanResult(otActiveScanResult *aResult, void *aContext)
{
    reinterpret_cast<Interpreter *>(aContext)->mInterpreter.HandleActiveScanResult(aResult);
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

    if (otIsSingleton(mContext))
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
        switch (otGetDeviceRole(mContext))
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
            SuccessOrExit(error = otBecomeDetached(mContext));
        }
        else if (strcmp(argv[0], "child") == 0)
        {
            SuccessOrExit(error = otBecomeChild(mContext, kMleAttachSamePartition));
        }
        else if (strcmp(argv[0], "router") == 0)
        {
            SuccessOrExit(error = otBecomeRouter(mContext));
        }
        else if (strcmp(argv[0], "leader") == 0)
        {
            SuccessOrExit(error = otBecomeLeader(mContext));
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
        SuccessOrExit(error = otThreadStart(mContext));
    }
    else if (strcmp(argv[0], "stop") == 0)
    {
        SuccessOrExit(error = otThreadStop(mContext));
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

void Interpreter::ProcessWhitelist(int argc, char *argv[])
{
    ThreadError error = kThreadError_None;
    otMacWhitelistEntry entry;
    int argcur = 0;
    uint8_t extAddr[8];
    int8_t rssi;

    if (argcur >= argc)
    {
        if (otIsMacWhitelistEnabled(mContext))
        {
            sServer->OutputFormat("Enabled\r\n");
        }
        else
        {
            sServer->OutputFormat("Disabled\r\n");
        }

        for (uint8_t i = 0; ; i++)
        {
            if (otGetMacWhitelistEntry(mContext, i, &entry) != kThreadError_None)
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
            VerifyOrExit(otAddMacWhitelistRssi(mContext, extAddr, rssi) == kThreadError_None, error = kThreadError_Parse);
        }
        else
        {
            otAddMacWhitelist(mContext, extAddr);
            VerifyOrExit(otAddMacWhitelist(mContext, extAddr) == kThreadError_None, error = kThreadError_Parse);
        }
    }
    else if (strcmp(argv[argcur], "clear") == 0)
    {
        otClearMacWhitelist(mContext);
    }
    else if (strcmp(argv[argcur], "disable") == 0)
    {
        otDisableMacWhitelist(mContext);
    }
    else if (strcmp(argv[argcur], "enable") == 0)
    {
        otEnableMacWhitelist(mContext);
    }
    else if (strcmp(argv[argcur], "remove") == 0)
    {
        VerifyOrExit(++argcur < argc, error = kThreadError_Parse);
        VerifyOrExit(Hex2Bin(argv[argcur], extAddr, sizeof(extAddr)) == sizeof(extAddr), error = kThreadError_Parse);
        otRemoveMacWhitelist(mContext, extAddr);
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
    ((Interpreter *)aContext)->HandleNetifStateChanged(aFlags);
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

        while (otGetNextOnMeshPrefix(mContext, false, &iterator, &config) == kThreadError_None)
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
            otRemoveUnicastAddress(mContext, address);
            address->mValidLifetime = 0;
        }
    }

    // add addresses
    iterator = OT_NETWORK_DATA_ITERATOR_INIT;

    while (otGetNextOnMeshPrefix(mContext, false, &iterator, &config) == kThreadError_None)
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
                otAddUnicastAddress(mContext, address);
                break;
            }
        }
    }

exit:
    return;
}

}  // namespace Cli
}  // namespace Thread
