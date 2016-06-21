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

#include "cli.hpp"
#include <common/encoding.hpp>
#include <platform/serial.h>

using Thread::Encoding::BigEndian::HostSwap16;

namespace Thread {
namespace Cli {

const struct Command Interpreter::sCommands[] =
{
    { "help", &ProcessHelp },
    { "channel", &ProcessChannel },
    { "childtimeout", &ProcessChildTimeout },
    { "contextreusedelay", &ProcessContextIdReuseDelay },
    { "counter", &ProcessCounters },
    { "extaddr", &ProcessExtAddress },
    { "extpanid", &ProcessExtPanId },
    { "ipaddr", &ProcessIpAddr },
    { "keysequence", &ProcessKeySequence },
    { "leaderweight", &ProcessLeaderWeight },
    { "masterkey", &ProcessMasterKey },
    { "mode", &ProcessMode },
    { "netdataregister", &ProcessNetworkDataRegister },
    { "networkidtimeout", &ProcessNetworkIdTimeout },
    { "networkname", &ProcessNetworkName },
    { "panid", &ProcessPanId },
    { "ping", &ProcessPing },
    { "prefix", &ProcessPrefix },
    { "releaserouterid", &ProcessReleaseRouterId },
    { "rloc16", &ProcessRloc16 },
    { "route", &ProcessRoute },
    { "routerupgradethreshold", &ProcessRouterUpgradeThreshold },
    { "scan", &ProcessScan },
    { "start", &ProcessStart },
    { "state", &ProcessState },
    { "stop", &ProcessStop },
    { "whitelist", &ProcessWhitelist },
};

otNetifAddress Interpreter::sAddress;

Ip6::IcmpEcho Interpreter::sIcmpEcho(&HandleEchoResponse, NULL);
Ip6::SockAddr Interpreter::sSockAddr;
Server *Interpreter::sServer;
uint8_t Interpreter::sEchoRequest[1500];

int Interpreter::Hex2Bin(const char *aHex, uint8_t *aBin, uint16_t aBinLength)
{
    uint16_t hexLength = strlen(aHex);
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

    return cur - aBin;
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

ThreadError Interpreter::ParseLong(char *argv, long &value)
{
    char *endptr;
    value = strtol(argv, &endptr, 0);
    return (*endptr == '\0') ? kThreadError_None : kThreadError_Parse;
}

void Interpreter::ProcessHelp(int argc, char *argv[])
{
    for (unsigned int i = 0; i < sizeof(sCommands) / sizeof(sCommands[0]); i++)
    {
        sServer->OutputFormat("%s\r\n", sCommands[i].mName);
    }
}

void Interpreter::ProcessChannel(int argc, char *argv[])
{
    ThreadError error = kThreadError_None;
    long value;

    if (argc == 0)
    {
        sServer->OutputFormat("%d\r\n", otGetChannel());
    }
    else
    {
        SuccessOrExit(error = ParseLong(argv[0], value));
        otSetChannel(value);
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
        sServer->OutputFormat("%d\r\n", otGetChildTimeout());
    }
    else
    {
        SuccessOrExit(error = ParseLong(argv[0], value));
        otSetChildTimeout(value);
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
        sServer->OutputFormat("%d\r\n", otGetContextIdReuseDelay());
    }
    else
    {
        SuccessOrExit(ParseLong(argv[0], value));
        otSetContextIdReuseDelay(value);
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
            const otMacCounters *counters = otGetMacCounters();
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

void Interpreter::ProcessExtAddress(int argc, char *argv[])
{
    const uint8_t *extAddress = otGetExtendedAddress();
    sServer->OutputFormat("%02x%02x%02x%02x%02x%02x%02x%02x\r\n",
                          extAddress[0], extAddress[1], extAddress[2], extAddress[3],
                          extAddress[4], extAddress[5], extAddress[6], extAddress[7]);
    AppendResult(kThreadError_None);
}

void Interpreter::ProcessExtPanId(int argc, char *argv[])
{
    ThreadError error = kThreadError_None;

    if (argc == 0)
    {
        const uint8_t *extPanId = otGetExtendedPanId();
        sServer->OutputFormat("%02x%02x%02x%02x%02x%02x%02x%02x\r\n",
                              extPanId[0], extPanId[1], extPanId[2], extPanId[3],
                              extPanId[4], extPanId[5], extPanId[6], extPanId[7]);
    }
    else
    {
        uint8_t extPanId[8];

        VerifyOrExit(Hex2Bin(argv[0], extPanId, sizeof(extPanId)) >= 0, error = kThreadError_Parse);

        otSetExtendedPanId(extPanId);
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
    error = otAddUnicastAddress(&sAddress);

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
    error = otRemoveUnicastAddress(&sAddress);

exit:
    return error;
}

void Interpreter::ProcessIpAddr(int argc, char *argv[])
{
    ThreadError error = kThreadError_None;

    if (argc == 0)
    {
        for (const otNetifAddress *addr = otGetUnicastAddresses(); addr; addr = addr->mNext)
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
        sServer->OutputFormat("%d\r\n", otGetKeySequenceCounter());
    }
    else
    {
        SuccessOrExit(error = ParseLong(argv[0], value));
        otSetKeySequenceCounter(value);
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
        sServer->OutputFormat("%d\r\n", otGetLocalLeaderWeight());
    }
    else
    {
        SuccessOrExit(error = ParseLong(argv[0], value));
        otSetLocalLeaderWeight(value);
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
        const uint8_t *key = otGetMasterKey(&keyLength);

        for (int i = 0; i < keyLength; i++)
        {
            sServer->OutputFormat("%02x", key[i]);
        }

        sServer->OutputFormat("\r\n");
    }
    else
    {
        int8_t keyLength;
        uint8_t key[16];

        VerifyOrExit((keyLength = Hex2Bin(argv[0], key, sizeof(key))) >= 0, error = kThreadError_Parse);
        SuccessOrExit(error = otSetMasterKey(key, keyLength));
    }

exit:
    AppendResult(error);
}

void Interpreter::ProcessMode(int argc, char *argv[])
{
    ThreadError error = kThreadError_None;
    otLinkModeConfig linkMode = {};

    if (argc == 0)
    {
        linkMode = otGetLinkMode();

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

        SuccessOrExit(error = otSetLinkMode(linkMode));
    }

exit:
    AppendResult(error);
}

void Interpreter::ProcessNetworkDataRegister(int argc, char *argv[])
{
    ThreadError error = kThreadError_None;
    SuccessOrExit(error = otSendServerData());

exit:
    AppendResult(error);
}

void Interpreter::ProcessNetworkIdTimeout(int argc, char *argv[])
{
    ThreadError error = kThreadError_None;
    long value;

    if (argc == 0)
    {
        sServer->OutputFormat("%d\r\n", otGetNetworkIdTimeout());
    }
    else
    {
        SuccessOrExit(error = ParseLong(argv[0], value));
        otSetNetworkIdTimeout(value);
    }

exit:
    AppendResult(error);
}

void Interpreter::ProcessNetworkName(int argc, char *argv[])
{
    ThreadError error = kThreadError_None;

    if (argc == 0)
    {
        sServer->OutputFormat("%.*s\r\n", OT_NETWORK_NAME_SIZE, otGetNetworkName());
    }
    else
    {
        SuccessOrExit(error = otSetNetworkName(argv[0]));
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
        sServer->OutputFormat("%d\r\n", otGetPanId());
    }
    else
    {
        SuccessOrExit(error = ParseLong(argv[0], value));
        otSetPanId(value);
    }

exit:
    AppendResult(error);
}

void Interpreter::HandleEchoResponse(void *aContext, Message &aMessage, const Ip6::MessageInfo &aMessageInfo)
{
    Ip6::IcmpHeader icmp6Header;

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
    sServer->OutputFormat(": icmp_seq=%d hlim=%d\r\n", icmp6Header.GetSequence(), aMessageInfo.mHopLimit);
}

void Interpreter::ProcessPing(int argc, char *argv[])
{
    ThreadError error = kThreadError_None;
    long length = 8;

    VerifyOrExit(argc > 0, error = kThreadError_Parse);

    memset(&sSockAddr, 0, sizeof(sSockAddr));
    SuccessOrExit(error = sSockAddr.GetAddress().FromString(argv[0]));
    sSockAddr.mScopeId = 1;

    if (argc > 1)
    {
        SuccessOrExit(error = ParseLong(argv[1], length));
    }

    sIcmpEcho.SendEchoRequest(sSockAddr, sEchoRequest, length);

    return;

exit:
    AppendResult(error);
}

ThreadError Interpreter::ProcessPrefixAdd(int argc, char *argv[])
{
    ThreadError error = kThreadError_None;
    otBorderRouterConfig config = {};
    int argcur = 0;

    char *prefixLengthStr;
    char *endptr;

    if ((prefixLengthStr = strchr(argv[argcur], '/')) == NULL)
    {
        ExitNow();
    }

    *prefixLengthStr++ = '\0';

    SuccessOrExit(error = otIp6AddressFromString(argv[argcur], &config.mPrefix.mPrefix));

    config.mPrefix.mLength = strtol(prefixLengthStr, &endptr, 0);

    if (*endptr != '\0')
    {
        ExitNow(error = kThreadError_Parse);
    }

    if (++argcur < argc)
    {
        for (char *arg = argv[argcur]; *arg != '\0'; arg++)
        {
            switch (*arg)
            {
            case 'p':
                config.mSlaacPreferred = true;
                break;

            case 'v':
                config.mSlaacValid = true;
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

            case 's':
                config.mStable = true;
                break;

            default:
                ExitNow();
            }
        }
    }

    if (++argcur < argc)
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
            ExitNow(error = kThreadError_Parse);
        }
    }

    error = otAddBorderRouter(&config);

exit:
    return error;
}

ThreadError Interpreter::ProcessPrefixRemove(int argc, char *argv[])
{
    ThreadError error = kThreadError_None;
    struct otIp6Prefix prefix = {};
    int argcur = 0;

    char *prefixLengthStr;
    char *endptr;

    if ((prefixLengthStr = strchr(argv[argcur], '/')) == NULL)
    {
        ExitNow();
    }

    *prefixLengthStr++ = '\0';

    SuccessOrExit(error = otIp6AddressFromString(argv[argcur], &prefix.mPrefix));

    prefix.mLength = strtol(prefixLengthStr, &endptr, 0);

    if (*endptr != '\0')
    {
        ExitNow(error = kThreadError_Parse);
    }

    error = otRemoveBorderRouter(&prefix);

exit:
    return error;
}

void Interpreter::ProcessPrefix(int argc, char *argv[])
{
    ThreadError error = kThreadError_None;

    VerifyOrExit(argc > 0, error = kThreadError_Parse);

    if (strcmp(argv[0], "add") == 0)
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
    SuccessOrExit(error = otReleaseRouterId(value));

exit:
    AppendResult(error);
}

void Interpreter::ProcessRloc16(int argc, char *argv[])
{
    sServer->OutputFormat("%04x\r\n", otGetRloc16());
    sServer->OutputFormat("Done\r\n");
}

ThreadError Interpreter::ProcessRouteAdd(int argc, char *argv[])
{
    ThreadError error = kThreadError_None;
    otExternalRouteConfig config = {};
    int argcur = 0;

    char *prefixLengthStr;
    char *endptr;

    VerifyOrExit(argc > 0, error = kThreadError_Parse);

    if ((prefixLengthStr = strchr(argv[argcur], '/')) == NULL)
    {
        ExitNow();
    }

    *prefixLengthStr++ = '\0';

    SuccessOrExit(error = otIp6AddressFromString(argv[argcur], &config.mPrefix.mPrefix));

    config.mPrefix.mLength = strtol(prefixLengthStr, &endptr, 0);

    if (*endptr != '\0')
    {
        ExitNow(error = kThreadError_Parse);
    }

    if (++argcur < argc)
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

    error = otAddExternalRoute(&config);

exit:
    return error;
}

ThreadError Interpreter::ProcessRouteRemove(int argc, char *argv[])
{
    ThreadError error = kThreadError_None;
    struct otIp6Prefix prefix = {};
    int argcur = 0;

    char *prefixLengthStr;
    char *endptr;

    VerifyOrExit(argc > 0, error = kThreadError_Parse);

    if ((prefixLengthStr = strchr(argv[argcur], '/')) == NULL)
    {
        ExitNow();
    }

    *prefixLengthStr++ = '\0';

    SuccessOrExit(error = otIp6AddressFromString(argv[argcur], &prefix.mPrefix));

    prefix.mLength = strtol(prefixLengthStr, &endptr, 0);

    if (*endptr != '\0')
    {
        ExitNow(error = kThreadError_Parse);
    }

    error = otRemoveExternalRoute(&prefix);

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

void Interpreter::ProcessRouterUpgradeThreshold(int argc, char *argv[])
{
    ThreadError error = kThreadError_None;
    long value;

    if (argc == 0)
    {
        sServer->OutputFormat("%d\r\n", otGetRouterUpgradeThreshold());
    }
    else
    {
        SuccessOrExit(error = ParseLong(argv[0], value));
        otSetRouterUpgradeThreshold(value);
    }

exit:
    AppendResult(error);
}

void Interpreter::ProcessScan(int argc, char *argv[])
{
    ThreadError error = kThreadError_None;
    uint16_t scanChannels = 0;
    long value;

    if (argc > 0)
    {
        SuccessOrExit(error = ParseLong(argv[0], value));
        scanChannels = 1 << (value - kPhyMinChannel);
    }

    SuccessOrExit(error = otActiveScan(scanChannels, 0, &HandleActiveScanResult));
    sServer->OutputFormat("| J | Network Name     | Extended PAN     | PAN  | MAC Address      | Ch | dBm | LQI |\r\n");
    sServer->OutputFormat("+---+------------------+------------------+------+------------------+----+-----+-----+\r\n");

    return;

exit:
    AppendResult(error);
}

void Interpreter::HandleActiveScanResult(otActiveScanResult *aResult)
{
    const uint8_t *bytes;

    if (aResult == NULL)
    {
        sServer->OutputFormat("Done\r\n");
        ExitNow();
    }

    sServer->OutputFormat("| %d ", aResult->mIsJoinable);

    if (aResult->mNetworkName != NULL)
    {
        sServer->OutputFormat("| %-16s ", aResult->mNetworkName);
    }
    else
    {
        sServer->OutputFormat("| ---------------- ");
    }

    if (aResult->mExtPanId != NULL)
    {
        bytes = aResult->mExtPanId;
        sServer->OutputFormat("| %02x%02x%02x%02x%02x%02x%02x%02x ",
                              bytes[0], bytes[1], bytes[2], bytes[3], bytes[4], bytes[5], bytes[6], bytes[7]);
    }
    else
    {
        sServer->OutputFormat("| ---------------- ");
    }

    sServer->OutputFormat("| %04x ", aResult->mPanId);

    bytes = aResult->mExtAddress.m8;
    sServer->OutputFormat("| %02x%02x%02x%02x%02x%02x%02x%02x ",
                          bytes[0], bytes[1], bytes[2], bytes[3], bytes[4], bytes[5], bytes[6], bytes[7]);
    sServer->OutputFormat("| %2d ", aResult->mChannel);
    sServer->OutputFormat("| %3d ", aResult->mRssi);
    sServer->OutputFormat("| %3d |\r\n", aResult->mLqi);

exit:
    return;
}

void Interpreter::ProcessStart(int argc, char *argv[])
{
    ThreadError error = kThreadError_None;

    SuccessOrExit(error = otEnable());

exit:
    AppendResult(error);
}

void Interpreter::ProcessState(int argc, char *argv[])
{
    ThreadError error = kThreadError_None;

    if (argc == 0)
    {
        switch (otGetDeviceRole())
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
            SuccessOrExit(error = otBecomeDetached());
        }
        else if (strcmp(argv[0], "child") == 0)
        {
            SuccessOrExit(error = otBecomeChild(kMleAttachSamePartition));
        }
        else if (strcmp(argv[0], "router") == 0)
        {
            SuccessOrExit(error = otBecomeRouter());
        }
        else if (strcmp(argv[0], "leader") == 0)
        {
            SuccessOrExit(error = otBecomeLeader());
        }
        else
        {
            ExitNow(error = kThreadError_Parse);
        }
    }

exit:
    AppendResult(error);
}

void Interpreter::ProcessStop(int argc, char *argv[])
{
    ThreadError error = kThreadError_None;

    SuccessOrExit(error = otDisable());

exit:
    AppendResult(error);
}

void Interpreter::ProcessWhitelist(int argc, char *argv[])
{
    ThreadError error = kThreadError_None;
    int argcur = 0;
    uint8_t extAddr[8];
    int8_t rssi;

    if (argcur >= argc)
    {
        ExitNow(error = kThreadError_Parse);
    }
    else if (strcmp(argv[argcur], "add") == 0)
    {
        VerifyOrExit(++argcur < argc, error = kThreadError_Parse);
        VerifyOrExit(Hex2Bin(argv[argcur], extAddr, sizeof(extAddr)) == sizeof(extAddr), error = kThreadError_Parse);

        if (++argcur < argc)
        {
            rssi = strtol(argv[argcur], NULL, 0);
            VerifyOrExit(otAddMacWhitelistRssi(extAddr, rssi) == kThreadError_None, error = kThreadError_Parse);
        }
        else
        {
            otAddMacWhitelist(extAddr);
            VerifyOrExit(otAddMacWhitelist(extAddr) == kThreadError_None, error = kThreadError_Parse);
        }
    }
    else if (strcmp(argv[argcur], "clear") == 0)
    {
        otClearMacWhitelist();
    }
    else if (strcmp(argv[argcur], "disable") == 0)
    {
        otDisableMacWhitelist();
    }
    else if (strcmp(argv[argcur], "enable") == 0)
    {
        otEnableMacWhitelist();
    }
    else if (strcmp(argv[argcur], "remove") == 0)
    {
        VerifyOrExit(++argcur < argc, error = kThreadError_Parse);
        VerifyOrExit(Hex2Bin(argv[argcur], extAddr, sizeof(extAddr)) == sizeof(extAddr), error = kThreadError_Parse);
        otRemoveMacWhitelist(extAddr);
    }

exit:
    AppendResult(error);
}

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

    for (unsigned int i = 0; i < sizeof(sCommands) / sizeof(sCommands[0]); i++)
    {
        if (strcmp(cmd, sCommands[i].mName) == 0)
        {
            sCommands[i].mCommand(argc, argv);
            break;
        }
    }

exit:
    return;
}

}  // namespace Cli
}  // namespace Thread
