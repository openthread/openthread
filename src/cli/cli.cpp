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

#ifdef OTDLL
#include <assert.h>
#endif

#include "openthread/openthread.h"
#include "openthread/commissioner.h"
#include "openthread/joiner.h"

#ifndef OTDLL
#include <openthread-instance.h>
#include "openthread/diag.h"
#include "openthread/icmp6.h"

#include <common/new.hpp>
#include <net/ip6.hpp>
#include "openthread/dhcp6_client.h"
#include "openthread/dhcp6_server.h"
#include "openthread/platform/uart.h"
#endif

#include <common/encoding.hpp>

#include "cli.hpp"
#include "cli_dataset.hpp"
#include "cli_uart.hpp"
#if OPENTHREAD_ENABLE_APPLICATION_COAP
#include "cli_coap.hpp"
#endif

using Thread::Encoding::BigEndian::HostSwap16;
using Thread::Encoding::BigEndian::HostSwap32;

namespace Thread {

namespace Cli {

const struct Command Interpreter::sCommands[] =
{
    { "help", &Interpreter::ProcessHelp },
    { "autostart", &Interpreter::ProcessAutoStart },
    { "blacklist", &Interpreter::ProcessBlacklist },
    { "bufferinfo", &Interpreter::ProcessBufferInfo },
    { "channel", &Interpreter::ProcessChannel },
    { "child", &Interpreter::ProcessChild },
    { "childmax", &Interpreter::ProcessChildMax },
    { "childtimeout", &Interpreter::ProcessChildTimeout },
#if OPENTHREAD_ENABLE_APPLICATION_COAP
    { "coap", &Interpreter::ProcessCoap },
#endif
#if OPENTHREAD_ENABLE_COMMISSIONER && OPENTHREAD_FTD
    { "commissioner", &Interpreter::ProcessCommissioner },
#endif
    { "contextreusedelay", &Interpreter::ProcessContextIdReuseDelay },
    { "counter", &Interpreter::ProcessCounters },
    { "dataset", &Interpreter::ProcessDataset },
    { "delaytimermin", &Interpreter::ProcessDelayTimerMin},
#if OPENTHREAD_ENABLE_DIAG
    { "diag", &Interpreter::ProcessDiag },
#endif
    { "discover", &Interpreter::ProcessDiscover },
#if OPENTHREAD_ENABLE_DNS_CLIENT
    { "dns", &Interpreter::ProcessDns },
#endif
    { "eidcache", &Interpreter::ProcessEidCache },
    { "eui64", &Interpreter::ProcessEui64 },
#ifdef OPENTHREAD_EXAMPLES_POSIX
    { "exit", &Interpreter::ProcessExit },
#endif
    { "extaddr", &Interpreter::ProcessExtAddress },
    { "extpanid", &Interpreter::ProcessExtPanId },
    { "factoryreset", &Interpreter::ProcessFactoryReset },
    { "hashmacaddr", &Interpreter::ProcessHashMacAddress },
    { "ifconfig", &Interpreter::ProcessIfconfig },
#ifdef OTDLL
    { "instance", &Interpreter::ProcessInstance },
    { "instancelist", &Interpreter::ProcessInstanceList },
#endif
    { "ipaddr", &Interpreter::ProcessIpAddr },
#ifndef OTDLL
    { "ipmaddr", &Interpreter::ProcessIpMulticastAddr },
#endif
#if OPENTHREAD_ENABLE_JOINER
    { "joiner", &Interpreter::ProcessJoiner },
#endif
    { "joinerport", &Interpreter::ProcessJoinerPort },
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
#ifndef OTDLL
    { "ping", &Interpreter::ProcessPing },
#endif
    { "pollperiod", &Interpreter::ProcessPollPeriod },
#ifndef OTDLL
    { "promiscuous", &Interpreter::ProcessPromiscuous },
#endif
    { "prefix", &Interpreter::ProcessPrefix },
    { "releaserouterid", &Interpreter::ProcessReleaseRouterId },
    { "reset", &Interpreter::ProcessReset },
    { "rloc16", &Interpreter::ProcessRloc16 },
    { "route", &Interpreter::ProcessRoute },
    { "router", &Interpreter::ProcessRouter },
    { "routerdowngradethreshold", &Interpreter::ProcessRouterDowngradeThreshold },
    { "routerrole", &Interpreter::ProcessRouterRole },
    { "routerselectionjitter", &Interpreter::ProcessRouterSelectionJitter },
    { "routerupgradethreshold", &Interpreter::ProcessRouterUpgradeThreshold },
    { "scan", &Interpreter::ProcessScan },
    { "singleton", &Interpreter::ProcessSingleton },
    { "state", &Interpreter::ProcessState },
    { "thread", &Interpreter::ProcessThread },
    { "version", &Interpreter::ProcessVersion },
    { "whitelist", &Interpreter::ProcessWhitelist },
};

#ifdef OTDLL
uint32_t otPlatRandomGet(void)
{
    return (uint32_t)rand();
}
#else
void otFreeMemory(const void *)
{
    // No-op on systems running OpenThread in-proc
}
#endif

template <class T> class otPtr
{
    T *ptr;
public:
    otPtr(T *_ptr) : ptr(_ptr) { }
    ~otPtr() { if (ptr) { otFreeMemory(ptr); } }
    T *get() const { return ptr; }
    operator T *() const { return ptr; }
    T *operator->() const { return ptr; }
};

typedef otPtr<const otMacCounters> otMacCountersPtr;
typedef otPtr<const otNetifAddress> otNetifAddressPtr;
typedef otPtr<const uint8_t> otBufferPtr;
typedef otPtr<const char> otStringPtr;

Interpreter::Interpreter(otInstance *aInstance):
    sServer(NULL),
#ifdef OTDLL
    mApiInstance(otApiInit()),
    mInstanceIndex(0),
#else
    sLength(8),
    sCount(1),
    sInterval(1000),
    sPingTimer(aInstance->mIp6.mTimerScheduler, &Interpreter::s_HandlePingTimer, this),
#if OPENTHREAD_ENABLE_DNS_CLIENT
    mResolvingInProgress(0),
#endif
#endif
    mInstance(aInstance)
{
#ifdef OTDLL
    assert(mApiInstance);
    CacheInstances();
#else
    memset(mSlaacAddresses, 0, sizeof(mSlaacAddresses));
    otSetStateChangedCallback(mInstance, &Interpreter::s_HandleNetifStateChanged, this);
    otThreadSetReceiveDiagnosticGetCallback(mInstance, &Interpreter::s_HandleDiagnosticGetResponse, this);

    mIcmpHandler.mReceiveCallback = Interpreter::s_HandleIcmpReceive;
    mIcmpHandler.mContext         = this;
    otIcmp6RegisterHandler(mInstance, &mIcmpHandler);

#if OPENTHREAD_ENABLE_DHCP6_CLIENT
    memset(mDhcpAddresses, 0, sizeof(mDhcpAddresses));
#endif  // OPENTHREAD_ENABLE_DHCP6_CLIENT

#if OPENTHREAD_ENABLE_DNS_CLIENT
    memset(mResolvingHostname, 0, sizeof(mResolvingHostname));
#endif // OPENTHREAD_ENABLE_DNS_CLIENT

#endif
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

void Interpreter::AppendResult(ThreadError aError)
{
    if (aError == kThreadError_None)
    {
        sServer->OutputFormat("Done\r\n");
    }
    else
    {
        sServer->OutputFormat("Error %d: %s\r\n", aError, otThreadErrorToString(aError));
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

void Interpreter::ProcessAutoStart(int argc, char *argv[])
{
    ThreadError error = kThreadError_None;

    if (argc == 0)
    {
        if (otThreadGetAutoStart(mInstance))
        {
            sServer->OutputFormat("true\r\n");
        }
        else
        {
            sServer->OutputFormat("false\r\n");
        }
    }
    else if (strcmp(argv[0], "true") == 0)
    {
        error = otThreadSetAutoStart(mInstance, true);
    }
    else if (strcmp(argv[0], "false") == 0)
    {
        error = otThreadSetAutoStart(mInstance, false);
    }
    else
    {
        error = kThreadError_InvalidArgs;
    }

    AppendResult(error);
}

void Interpreter::ProcessBlacklist(int argc, char *argv[])
{
    ThreadError error = kThreadError_None;
    otMacBlacklistEntry entry;
    int argcur = 0;
    uint8_t extAddr[8];

    if (argcur >= argc)
    {
        if (otLinkIsBlacklistEnabled(mInstance))
        {
            sServer->OutputFormat("Enabled\r\n");
        }
        else
        {
            sServer->OutputFormat("Disabled\r\n");
        }

        for (uint8_t i = 0; ; i++)
        {
            if (otLinkGetBlacklistEntry(mInstance, i, &entry) != kThreadError_None)
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

        otLinkAddBlacklist(mInstance, extAddr);
        VerifyOrExit(otLinkAddBlacklist(mInstance, extAddr) == kThreadError_None, error = kThreadError_Parse);
    }
    else if (strcmp(argv[argcur], "clear") == 0)
    {
        otLinkClearBlacklist(mInstance);
    }
    else if (strcmp(argv[argcur], "disable") == 0)
    {
        otLinkSetBlacklistEnabled(mInstance, false);
    }
    else if (strcmp(argv[argcur], "enable") == 0)
    {
        otLinkSetBlacklistEnabled(mInstance, true);
    }
    else if (strcmp(argv[argcur], "remove") == 0)
    {
        VerifyOrExit(++argcur < argc, error = kThreadError_Parse);
        VerifyOrExit(Hex2Bin(argv[argcur], extAddr, sizeof(extAddr)) == sizeof(extAddr), error = kThreadError_Parse);
        otLinkRemoveBlacklist(mInstance, extAddr);
    }

exit:
    AppendResult(error);
}

void Interpreter::ProcessBufferInfo(int argc, char *argv[])
{
    otBufferInfo bufferInfo;
    (void)argc;
    (void)argv;

    otMessageGetBufferInfo(mInstance, &bufferInfo);

    sServer->OutputFormat("total: %d\r\n", bufferInfo.mTotalBuffers);
    sServer->OutputFormat("free: %d\r\n", bufferInfo.mFreeBuffers);
    sServer->OutputFormat("6lo send: %d %d\r\n", bufferInfo.m6loSendMessages, bufferInfo.m6loSendBuffers);
    sServer->OutputFormat("6lo reas: %d %d\r\n", bufferInfo.m6loReassemblyMessages, bufferInfo.m6loReassemblyBuffers);
    sServer->OutputFormat("ip6: %d %d\r\n", bufferInfo.mIp6Messages, bufferInfo.mIp6Buffers);
    sServer->OutputFormat("mpl: %d %d\r\n", bufferInfo.mMplMessages, bufferInfo.mMplBuffers);
    sServer->OutputFormat("mle: %d %d\r\n", bufferInfo.mMleMessages, bufferInfo.mMleBuffers);
    sServer->OutputFormat("arp: %d %d\r\n", bufferInfo.mArpMessages, bufferInfo.mArpBuffers);
    sServer->OutputFormat("coap client: %d %d\r\n", bufferInfo.mCoapClientMessages, bufferInfo.mCoapClientBuffers);
    sServer->OutputFormat("coap server: %d %d\r\n", bufferInfo.mCoapServerMessages, bufferInfo.mCoapServerBuffers);

    AppendResult(kThreadError_None);
}

void Interpreter::ProcessChannel(int argc, char *argv[])
{
    ThreadError error = kThreadError_None;
    long value;

    if (argc == 0)
    {
        sServer->OutputFormat("%d\r\n", otLinkGetChannel(mInstance));
    }
    else
    {
        SuccessOrExit(error = ParseLong(argv[0], value));
        otLinkSetChannel(mInstance, static_cast<uint8_t>(value));
    }

exit:
    AppendResult(error);
}

void Interpreter::ProcessChild(int argc, char *argv[])
{
    ThreadError error = kThreadError_None;
    otChildInfo childInfo;
    uint8_t maxChildren;
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

        maxChildren = otThreadGetMaxAllowedChildren(mInstance);

        for (uint8_t i = 0; i < maxChildren ; i++)
        {

            switch (otThreadGetChildInfoByIndex(mInstance, i, &childInfo))
            {
            case kThreadError_None:
                break;

            case kThreadError_NotFound:
                continue;

            default:
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

        ExitNow();
    }

    SuccessOrExit(error = ParseLong(argv[0], value));
    SuccessOrExit(error = otThreadGetChildInfoById(mInstance, static_cast<uint16_t>(value), &childInfo));

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
        sServer->OutputFormat("%d\r\n", otThreadGetMaxAllowedChildren(mInstance));
    }
    else
    {
        SuccessOrExit(error = ParseLong(argv[0], value));
        SuccessOrExit(error = otThreadSetMaxAllowedChildren(mInstance, static_cast<uint8_t>(value)));
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
        sServer->OutputFormat("%d\r\n", otThreadGetChildTimeout(mInstance));
    }
    else
    {
        SuccessOrExit(error = ParseLong(argv[0], value));
        otThreadSetChildTimeout(mInstance, static_cast<uint32_t>(value));
    }

exit:
    AppendResult(error);
}

#if OPENTHREAD_ENABLE_APPLICATION_COAP

void Interpreter::ProcessCoap(int argc, char *argv[])
{
    ThreadError error;
    error = Coap::Process(mInstance, argc, argv, *sServer);
    AppendResult(error);
}

#endif // OPENTHREAD_ENABLE_APPLICATION_COAP

void Interpreter::ProcessContextIdReuseDelay(int argc, char *argv[])
{
    ThreadError error = kThreadError_None;
    long value;

    if (argc == 0)
    {
        sServer->OutputFormat("%d\r\n", otThreadGetContextIdReuseDelay(mInstance));
    }
    else
    {
        SuccessOrExit(ParseLong(argv[0], value));
        otThreadSetContextIdReuseDelay(mInstance, static_cast<uint32_t>(value));
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
            otMacCountersPtr counters(otLinkGetCounters(mInstance));
            sServer->OutputFormat("TxTotal: %d\r\n", counters->mTxTotal);
            sServer->OutputFormat("    TxUnicast: %d\r\n", counters->mTxUnicast);
            sServer->OutputFormat("    TxBroadcast: %d\r\n", counters->mTxBroadcast);
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
            sServer->OutputFormat("    RxUnicast: %d\r\n", counters->mRxUnicast);
            sServer->OutputFormat("    RxBroadcast: %d\r\n", counters->mRxBroadcast);
            sServer->OutputFormat("    RxData: %d\r\n", counters->mRxData);
            sServer->OutputFormat("    RxDataPoll: %d\r\n", counters->mRxDataPoll);
            sServer->OutputFormat("    RxBeacon: %d\r\n", counters->mRxBeacon);
            sServer->OutputFormat("    RxBeaconRequest: %d\r\n", counters->mRxBeaconRequest);
            sServer->OutputFormat("    RxOther: %d\r\n", counters->mRxOther);
            sServer->OutputFormat("    RxWhitelistFiltered: %d\r\n", counters->mRxWhitelistFiltered);
            sServer->OutputFormat("    RxDestAddrFiltered: %d\r\n", counters->mRxDestAddrFiltered);
            sServer->OutputFormat("    RxDuplicated: %d\r\n", counters->mRxDuplicated);
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

void Interpreter::ProcessDelayTimerMin(int argc, char *argv[])
{
    ThreadError error = kThreadError_None;

    if (argc == 0)
    {
        sServer->OutputFormat("%d\r\n", (otDatasetGetDelayTimerMinimal(mInstance) / 1000));
    }
    else if (argc == 1)
    {
        unsigned long value;
        SuccessOrExit(error = ParseUnsignedLong(argv[0], value));
        SuccessOrExit(error = otDatasetSetDelayTimerMinimal(mInstance, static_cast<uint32_t>(value * 1000)));
    }
    else
    {
        error = kThreadError_InvalidArgs;
    }

exit:
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

    SuccessOrExit(error = otThreadDiscover(mInstance, scanChannels, OT_PANID_BROADCAST,
                                           &Interpreter::s_HandleActiveScanResult, this));
    sServer->OutputFormat("| J | Network Name     | Extended PAN     | PAN  | MAC Address      | Ch | dBm | LQI |\r\n");
    sServer->OutputFormat("+---+------------------+------------------+------+------------------+----+-----+-----+\r\n");

    return;

exit:
    AppendResult(error);
}

#if OPENTHREAD_ENABLE_DNS_CLIENT
void Interpreter::ProcessDns(int argc, char *argv[])
{
    ThreadError error = kThreadError_None;
    long port = OT_DNS_DEFAULT_DNS_SERVER_PORT;
    Ip6::MessageInfo messageInfo;
    otDnsQuery query;

    VerifyOrExit(argc > 0, error = kThreadError_InvalidArgs);

    if (strcmp(argv[0], "resolve") == 0)
    {
        VerifyOrExit(!mResolvingInProgress, error = kThreadError_Busy);
        VerifyOrExit(argc > 1, error = kThreadError_InvalidArgs);
        VerifyOrExit(strlen(argv[1]) < OT_DNS_MAX_HOSTNAME_LENGTH, error = kThreadError_InvalidArgs);

        strcpy(mResolvingHostname, argv[1]);

        memset(&messageInfo, 0, sizeof(messageInfo));
        messageInfo.mInterfaceId = OT_NETIF_INTERFACE_ID_THREAD;

        if (argc > 2)
        {
            SuccessOrExit(error = messageInfo.GetPeerAddr().FromString(argv[2]));
        }
        else
        {
            // Use IPv6 address of default DNS server.
            SuccessOrExit(error = messageInfo.GetPeerAddr().FromString(OT_DNS_DEFAULT_DNS_SERVER_IP));
        }

        if (argc > 3)
        {
            SuccessOrExit(error = ParseLong(argv[3], port));
        }

        messageInfo.SetPeerPort(static_cast<uint16_t>(port));

        query.mHostname    = mResolvingHostname;
        query.mMessageInfo = static_cast<const otMessageInfo *>(&messageInfo);
        query.mNoRecursion = false;

        SuccessOrExit(error = otDnsClientQuery(mInstance, &query, &Interpreter::s_HandleDnsResponse, this));

        mResolvingInProgress = true;
        return;
    }
    else
    {
        ExitNow(error = kThreadError_InvalidArgs);
    }

exit:
    AppendResult(error);
}

void Interpreter::s_HandleDnsResponse(void *aContext, const char *aHostname, otIp6Address *aAddress,
                                      uint32_t aTtl, ThreadError aResult)
{
    static_cast<Interpreter *>(aContext)->HandleDnsResponse(aHostname,
                                                            *static_cast<Ip6::Address *>(aAddress),
                                                            aTtl, aResult);
}

void Interpreter::HandleDnsResponse(const char *aHostname, Ip6::Address &aAddress, uint32_t aTtl, ThreadError aResult)
{
    sServer->OutputFormat("DNS response for %s - ", aHostname);

    if (aResult == kThreadError_None)
    {
        sServer->OutputFormat("[%x:%x:%x:%x:%x:%x:%x:%x] TTL: %d\r\n",
                              HostSwap16(aAddress.mFields.m16[0]),
                              HostSwap16(aAddress.mFields.m16[1]),
                              HostSwap16(aAddress.mFields.m16[2]),
                              HostSwap16(aAddress.mFields.m16[3]),
                              HostSwap16(aAddress.mFields.m16[4]),
                              HostSwap16(aAddress.mFields.m16[5]),
                              HostSwap16(aAddress.mFields.m16[6]),
                              HostSwap16(aAddress.mFields.m16[7]),
                              aTtl);
    }
    else
    {
        AppendResult(aResult);
    }

    mResolvingInProgress = false;
}
#endif

void Interpreter::ProcessEidCache(int argc, char *argv[])
{
    otEidCacheEntry entry;

    for (uint8_t i = 0; ; i++)
    {
        SuccessOrExit(otThreadGetEidCacheEntry(mInstance, i, &entry));

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

    otLinkGetFactoryAssignedIeeeEui64(mInstance, &extAddress);
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
        otBufferPtr extAddress(otLinkGetExtendedAddress(mInstance));
        OutputBytes(extAddress, OT_EXT_ADDRESS_SIZE);
        sServer->OutputFormat("\r\n");
    }
    else
    {
        otExtAddress extAddress;

        VerifyOrExit(Hex2Bin(argv[0], extAddress.m8, sizeof(otExtAddress)) >= 0, error = kThreadError_Parse);

        otLinkSetExtendedAddress(mInstance, &extAddress);
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
        otBufferPtr extPanId(otThreadGetExtendedPanId(mInstance));
        OutputBytes(extPanId, OT_EXT_PAN_ID_SIZE);
        sServer->OutputFormat("\r\n");
    }
    else
    {
        uint8_t extPanId[8];

        VerifyOrExit(Hex2Bin(argv[0], extPanId, sizeof(extPanId)) >= 0, error = kThreadError_Parse);

        otThreadSetExtendedPanId(mInstance, extPanId);
    }

exit:
    AppendResult(error);
}

void Interpreter::ProcessFactoryReset(int argc, char *argv[])
{
    otInstanceFactoryReset(mInstance);
    (void)argc;
    (void)argv;
}

void Interpreter::ProcessHashMacAddress(int argc, char *argv[])
{
    ThreadError error = kThreadError_None;
    otExtAddress hashMacAddress;

    VerifyOrExit(argc == 0, error = kThreadError_Parse);

    otLinkGetJoinerId(mInstance, &hashMacAddress);
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
        if (otIp6IsEnabled(mInstance))
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
        SuccessOrExit(error = otIp6SetEnabled(mInstance, true));
    }
    else if (strcmp(argv[0], "down") == 0)
    {
        SuccessOrExit(error = otIp6SetEnabled(mInstance, false));
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
    aAddress.mPreferred = true;
    aAddress.mValid = true;
    error = otIp6AddUnicastAddress(mInstance, &aAddress);

exit:
    return error;
}

ThreadError Interpreter::ProcessIpAddrDel(int argc, char *argv[])
{
    ThreadError error;
    struct otIp6Address address;

    VerifyOrExit(argc > 0, error = kThreadError_Parse);

    SuccessOrExit(error = otIp6AddressFromString(argv[0], &address));
    error = otIp6RemoveUnicastAddress(mInstance, &address);

exit:
    return error;
}

void Interpreter::ProcessIpAddr(int argc, char *argv[])
{
    ThreadError error = kThreadError_None;

    if (argc == 0)
    {
        otNetifAddressPtr unicastAddrs(otIp6GetUnicastAddresses(mInstance));

        for (const otNetifAddress *addr = unicastAddrs; addr; addr = addr->mNext)
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

#ifndef OTDLL
ThreadError Interpreter::ProcessIpMulticastAddrAdd(int argc, char *argv[])
{
    ThreadError error;
    struct otIp6Address address;

    VerifyOrExit(argc > 0, error = kThreadError_Parse);

    SuccessOrExit(error = otIp6AddressFromString(argv[0], &address));
    error = otIp6SubscribeMulticastAddress(mInstance, &address);

exit:
    return error;
}

ThreadError Interpreter::ProcessIpMulticastAddrDel(int argc, char *argv[])
{
    ThreadError error;
    struct otIp6Address address;

    VerifyOrExit(argc > 0, error = kThreadError_Parse);

    SuccessOrExit(error = otIp6AddressFromString(argv[0], &address));
    error = otIp6UnsubscribeMulticastAddress(mInstance, &address);

exit:
    return error;
}

ThreadError Interpreter::ProcessMulticastPromiscuous(int argc, char *argv[])
{
    ThreadError error = kThreadError_None;

    if (argc == 0)
    {
        if (otIp6IsMulticastPromiscuousEnabled(mInstance))
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
            otIp6SetMulticastPromiscuousEnabled(mInstance, true);
        }
        else if (strcmp(argv[0], "disable") == 0)
        {
            otIp6SetMulticastPromiscuousEnabled(mInstance, false);
        }
        else
        {
            ExitNow(error = kThreadError_Parse);
        }
    }

exit:
    return error;
}

void Interpreter::ProcessIpMulticastAddr(int argc, char *argv[])
{
    ThreadError error = kThreadError_None;

    if (argc == 0)
    {
        for (const otNetifMulticastAddress *addr = otIp6GetMulticastAddresses(mInstance); addr; addr = addr->mNext)
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
            SuccessOrExit(error = ProcessIpMulticastAddrAdd(argc - 1, argv + 1));
        }
        else if (strcmp(argv[0], "del") == 0)
        {
            SuccessOrExit(error = ProcessIpMulticastAddrDel(argc - 1, argv + 1));
        }
        else if (strcmp(argv[0], "promiscuous") == 0)
        {
            SuccessOrExit(error = ProcessMulticastPromiscuous(argc - 1, argv + 1));
        }
    }

exit:
    AppendResult(error);
}
#endif

void Interpreter::ProcessKeySequence(int argc, char *argv[])
{
    ThreadError error = kThreadError_None;
    long value;

    VerifyOrExit(argc == 1 || argc == 2, error = kThreadError_Parse);

    if (strcmp(argv[0], "counter") == 0)
    {
        if (argc == 1)
        {
            sServer->OutputFormat("%d\r\n", otThreadGetKeySequenceCounter(mInstance));
        }
        else
        {
            SuccessOrExit(error = ParseLong(argv[1], value));
            otThreadSetKeySequenceCounter(mInstance, static_cast<uint32_t>(value));
        }
    }
    else if (strcmp(argv[0], "guardtime") == 0)
    {
        if (argc == 1)
        {
            sServer->OutputFormat("%d\r\n", otThreadGetKeySwitchGuardTime(mInstance));
        }
        else
        {
            SuccessOrExit(error = ParseLong(argv[1], value));
            otThreadSetKeySwitchGuardTime(mInstance, static_cast<uint32_t>(value));
        }
    }

exit:
    AppendResult(error);
}

void Interpreter::ProcessLeaderData(int argc, char *argv[])
{
    ThreadError error;
    otLeaderData leaderData;

    SuccessOrExit(error = otThreadGetLeaderData(mInstance, &leaderData));

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
        sServer->OutputFormat("%u\r\n", otThreadGetLocalLeaderPartitionId(mInstance));
    }
    else
    {
        SuccessOrExit(error = ParseUnsignedLong(argv[0], value));
        otThreadSetLocalLeaderPartitionId(mInstance, static_cast<uint32_t>(value));
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
        sServer->OutputFormat("%d\r\n", otThreadGetLocalLeaderWeight(mInstance));
    }
    else
    {
        SuccessOrExit(error = ParseLong(argv[0], value));
        otThreadSetLocalLeaderWeight(mInstance, static_cast<uint8_t>(value));
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

    VerifyOrExit(argc > 0, error = kThreadError_InvalidArgs);
    VerifyOrExit(Hex2Bin(argv[0], extAddress, OT_EXT_ADDRESS_SIZE) >= 0, error = kThreadError_Parse);

    if (argc == 1)
    {
        VerifyOrExit(otLinkGetAssignLinkQuality(mInstance, extAddress, &linkQuality) == kThreadError_None,
                     error = kThreadError_InvalidArgs);
        sServer->OutputFormat("%d\r\n", linkQuality);
    }
    else
    {
        SuccessOrExit(error = ParseLong(argv[1], value));
        otLinkSetAssignLinkQuality(mInstance, extAddress, static_cast<uint8_t>(value));
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
        otBufferPtr key(otThreadGetMasterKey(mInstance, &keyLength));

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
        SuccessOrExit(error = otThreadSetMasterKey(mInstance, key, static_cast<uint8_t>(keyLength)));
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
        linkMode = otThreadGetLinkMode(mInstance);

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

        SuccessOrExit(error = otThreadSetLinkMode(mInstance, linkMode));
    }

exit:
    AppendResult(error);
}

void Interpreter::ProcessNetworkDataRegister(int argc, char *argv[])
{
    ThreadError error = kThreadError_None;
    SuccessOrExit(error = otNetDataRegister(mInstance));

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
        sServer->OutputFormat("%d\r\n", otThreadGetNetworkIdTimeout(mInstance));
    }
    else
    {
        SuccessOrExit(error = ParseLong(argv[0], value));
        otThreadSetNetworkIdTimeout(mInstance, static_cast<uint8_t>(value));
    }

exit:
    AppendResult(error);
}

void Interpreter::ProcessNetworkName(int argc, char *argv[])
{
    ThreadError error = kThreadError_None;

    if (argc == 0)
    {
        otStringPtr networkName(otThreadGetNetworkName(mInstance));
        sServer->OutputFormat("%.*s\r\n", OT_NETWORK_NAME_MAX_SIZE, (const char *)networkName);
    }
    else
    {
        SuccessOrExit(error = otThreadSetNetworkName(mInstance, argv[0]));
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
        sServer->OutputFormat("%04x\r\n", otLinkGetPanId(mInstance));
    }
    else
    {
        SuccessOrExit(error = ParseLong(argv[0], value));
        otLinkSetPanId(mInstance, static_cast<otPanId>(value));
    }

exit:
    AppendResult(error);
}

void Interpreter::ProcessParent(int argc, char *argv[])
{
    ThreadError error = kThreadError_None;
    otRouterInfo parentInfo;

    SuccessOrExit(error = otThreadGetParentInfo(mInstance, &parentInfo));
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

#ifndef OTDLL
void Interpreter::s_HandleIcmpReceive(void *aContext, otMessage *aMessage, const otMessageInfo *aMessageInfo,
                                      const otIcmp6Header *aIcmpHeader)
{
    static_cast<Interpreter *>(aContext)->HandleIcmpReceive(*static_cast<Message *>(aMessage),
                                                            *static_cast<const Ip6::MessageInfo *>(aMessageInfo),
                                                            *static_cast<const Ip6::IcmpHeader *>(aIcmpHeader));
}

void Interpreter::HandleIcmpReceive(Message &aMessage, const Ip6::MessageInfo &aMessageInfo,
                                    const Ip6::IcmpHeader &aIcmpHeader)
{
    uint32_t timestamp = 0;

    VerifyOrExit(aIcmpHeader.GetType() == kIcmp6TypeEchoReply);

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
    sServer->OutputFormat(": icmp_seq=%d hlim=%d", aIcmpHeader.GetSequence(), aMessageInfo.mHopLimit);

    if (aMessage.Read(aMessage.GetOffset(), sizeof(uint32_t), &timestamp) >=
        static_cast<int>(sizeof(uint32_t)))
    {
        sServer->OutputFormat(" time=%dms", Timer::GetNow() - HostSwap32(timestamp));
    }

    sServer->OutputFormat("\r\n");

exit:
    return;
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
    sMessageInfo.mInterfaceId = OT_NETIF_INTERFACE_ID_THREAD;

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

    otMessage *message;
    const otMessageInfo *messageInfo = static_cast<const otMessageInfo *>(&sMessageInfo);

    VerifyOrExit((message = otIp6NewMessage(mInstance, true)) != NULL, error = kThreadError_NoBufs);
    SuccessOrExit(error = otMessageAppend(message, &timestamp, sizeof(timestamp)));
    SuccessOrExit(error = otMessageSetLength(message, sLength));
    SuccessOrExit(error = otIcmp6SendEchoRequest(mInstance, message, messageInfo, 1));

    sCount--;

exit:

    if (error != kThreadError_None && message != NULL)
    {
        otMessageFree(message);
    }

    if (sCount)
    {
        sPingTimer.Start(sInterval);
    }
}
#endif

void Interpreter::ProcessPollPeriod(int argc, char *argv[])
{
    ThreadError error = kThreadError_None;
    long value;

    if (argc == 0)
    {
        sServer->OutputFormat("%d\r\n", (otLinkGetPollPeriod(mInstance) / 1000));  // ms->s
    }
    else
    {
        SuccessOrExit(error = ParseLong(argv[0], value));
        otLinkSetPollPeriod(mInstance, static_cast<uint32_t>(value * 1000));  // s->ms
    }

exit:
    AppendResult(error);
}

#ifndef OTDLL
void Interpreter::ProcessPromiscuous(int argc, char *argv[])
{
    ThreadError error = kThreadError_None;

    if (argc == 0)
    {
        if (otLinkIsPromiscuous(mInstance) && otPlatRadioGetPromiscuous(mInstance))
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
            otLinkSetPcapCallback(mInstance, &s_HandleLinkPcapReceive, this);
            SuccessOrExit(error = otLinkSetPromiscuous(mInstance, true));
        }
        else if (strcmp(argv[0], "disable") == 0)
        {
            otLinkSetPcapCallback(mInstance, NULL, NULL);
            SuccessOrExit(error = otLinkSetPromiscuous(mInstance, false));
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
#endif

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
            config.mPreference = kRoutePreferenceHigh;
        }
        else if (strcmp(argv[argcur], "med") == 0)
        {
            config.mPreference = kRoutePreferenceMedium;
        }
        else if (strcmp(argv[argcur], "low") == 0)
        {
            config.mPreference = kRoutePreferenceLow;
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

    error = otNetDataAddPrefixInfo(mInstance, &config);

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

    error = otNetDataRemovePrefixInfo(mInstance, &prefix);

exit:
    (void)argc;
    return error;
}

ThreadError Interpreter::ProcessPrefixList(void)
{
    otNetworkDataIterator iterator = OT_NETWORK_DATA_ITERATOR_INIT;
    otBorderRouterConfig config;

    while (otNetDataGetNextPrefixInfo(mInstance, true, &iterator, &config) == kThreadError_None)
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
        case kRoutePreferenceLow:
            sServer->OutputFormat(" low\r\n");
            break;

        case kRoutePreferenceMedium:
            sServer->OutputFormat(" med\r\n");
            break;

        case kRoutePreferenceHigh:
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
    SuccessOrExit(error = otThreadReleaseRouterId(mInstance, static_cast<uint8_t>(value)));

exit:
    AppendResult(error);
}

void Interpreter::ProcessReset(int argc, char *argv[])
{
    otInstanceReset(mInstance);
    (void)argc;
    (void)argv;
}

void Interpreter::ProcessRloc16(int argc, char *argv[])
{
    sServer->OutputFormat("%04x\r\n", otThreadGetRloc16(mInstance));
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
            config.mPreference = kRoutePreferenceHigh;
        }
        else if (strcmp(argv[argcur], "med") == 0)
        {
            config.mPreference = kRoutePreferenceMedium;
        }
        else if (strcmp(argv[argcur], "low") == 0)
        {
            config.mPreference = kRoutePreferenceLow;
        }
        else
        {
            ExitNow(error = kThreadError_Parse);
        }
    }

    error = otNetDataAddRoute(mInstance, &config);

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

    error = otNetDataRemoveRoute(mInstance, &prefix);

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
            if (otThreadGetRouterInfo(mInstance, i, &routerInfo) != kThreadError_None)
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
    SuccessOrExit(error = otThreadGetRouterInfo(mInstance, static_cast<uint16_t>(value), &routerInfo));

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
        sServer->OutputFormat("%d\r\n", otThreadGetRouterDowngradeThreshold(mInstance));
    }
    else
    {
        SuccessOrExit(error = ParseLong(argv[0], value));
        otThreadSetRouterDowngradeThreshold(mInstance, static_cast<uint8_t>(value));
    }

exit:
    AppendResult(error);
}

void Interpreter::ProcessRouterRole(int argc, char *argv[])
{
    ThreadError error = kThreadError_None;

    if (argc == 0)
    {
        if (otThreadIsRouterRoleEnabled(mInstance))
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
        otThreadSetRouterRoleEnabled(mInstance, true);
    }
    else if (strcmp(argv[0], "disable") == 0)
    {
        otThreadSetRouterRoleEnabled(mInstance, false);
    }
    else
    {
        ExitNow(error = kThreadError_Parse);
    }

exit:
    AppendResult(error);
}

void Interpreter::ProcessRouterSelectionJitter(int argc, char *argv[])
{
    ThreadError error = kThreadError_None;
    long value;

    if (argc == 0)
    {
        sServer->OutputFormat("%d\r\n", otThreadGetRouterSelectionJitter(mInstance));
    }
    else
    {
        SuccessOrExit(error = ParseLong(argv[0], value));
        VerifyOrExit(0 < value && value < 256, error = kThreadError_InvalidArgs);
        otThreadSetRouterSelectionJitter(mInstance, static_cast<uint8_t>(value));
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
        sServer->OutputFormat("%d\r\n", otThreadGetRouterUpgradeThreshold(mInstance));
    }
    else
    {
        SuccessOrExit(error = ParseLong(argv[0], value));
        otThreadSetRouterUpgradeThreshold(mInstance, static_cast<uint8_t>(value));
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

    sServer->OutputFormat("| J | Network Name     | Extended PAN     | PAN  | MAC Address      | Ch | dBm | LQI |\r\n");
    sServer->OutputFormat("+---+------------------+------------------+------+------------------+----+-----+-----+\r\n");
    SuccessOrExit(error = otLinkActiveScan(mInstance, scanChannels, 0, &Interpreter::s_HandleActiveScanResult, this));

    return;

exit:
    AppendResult(error);
}

void OTCALL Interpreter::s_HandleActiveScanResult(otActiveScanResult *aResult, void *aContext)
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

    if (otThreadIsSingleton(mInstance))
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
        switch (otThreadGetDeviceRole(mInstance))
        {
        case kDeviceRoleOffline:
            sServer->OutputFormat("offline\r\n");
            break;

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
            SuccessOrExit(error = otThreadBecomeDetached(mInstance));
        }
        else if (strcmp(argv[0], "child") == 0)
        {
            SuccessOrExit(error = otThreadBecomeChild(mInstance, kMleAttachSamePartition1));
        }
        else if (strcmp(argv[0], "router") == 0)
        {
            SuccessOrExit(error = otThreadBecomeRouter(mInstance));
        }
        else if (strcmp(argv[0], "leader") == 0)
        {
            SuccessOrExit(error = otThreadBecomeLeader(mInstance));
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
        SuccessOrExit(error = otThreadSetEnabled(mInstance, true));
    }
    else if (strcmp(argv[0], "stop") == 0)
    {
        SuccessOrExit(error = otThreadSetEnabled(mInstance, false));
    }

exit:
    (void)argc;
    (void)argv;
    AppendResult(error);
}

void Interpreter::ProcessVersion(int argc, char *argv[])
{
    otStringPtr version(otGetVersionString());
    sServer->OutputFormat("%s\r\n", (const char *)version);
    AppendResult(kThreadError_None);
    (void)argc;
    (void)argv;
}

#if OPENTHREAD_ENABLE_COMMISSIONER && OPENTHREAD_FTD

void Interpreter::ProcessCommissioner(int argc, char *argv[])
{
    ThreadError error = kThreadError_None;

    VerifyOrExit(argc > 0, error = kThreadError_Parse);

    if (strcmp(argv[0], "start") == 0)
    {
        SuccessOrExit(error = otCommissionerStart(mInstance));
    }
    else if (strcmp(argv[0], "stop") == 0)
    {
        SuccessOrExit(error = otCommissionerStop(mInstance));
    }
    else if (strcmp(argv[0], "joiner") == 0)
    {
        otExtAddress addr;
        const otExtAddress *addrPtr;

        VerifyOrExit(argc > 2, error = kThreadError_Parse);

        if (strcmp(argv[2], "*") == 0)
        {
            addrPtr = NULL;
        }
        else
        {
            VerifyOrExit(Hex2Bin(argv[2], addr.m8, sizeof(addr)) == sizeof(addr), error = kThreadError_Parse);
            addrPtr = &addr;
        }

        if (strcmp(argv[1], "add") == 0)
        {
            VerifyOrExit(argc > 3, error = kThreadError_Parse);
            // Timeout parameter is optional - if not specified, use default value.
            unsigned long timeout = kDefaultJoinerTimeout;

            if (argc > 4)
            {
                SuccessOrExit(error = ParseUnsignedLong(argv[4], timeout));
            }

            SuccessOrExit(error = otCommissionerAddJoiner(mInstance, addrPtr, argv[3], static_cast<uint32_t>(timeout)));
        }
        else if (strcmp(argv[1], "remove") == 0)
        {
            SuccessOrExit(error = otCommissionerRemoveJoiner(mInstance, addrPtr));
        }
    }
    else if (strcmp(argv[0], "provisioningurl") == 0)
    {
        SuccessOrExit(error = otCommissionerSetProvisioningUrl(mInstance, (argc > 1) ? argv[1] : NULL));
    }
    else if (strcmp(argv[0], "announce") == 0)
    {
        long mask;
        long count;
        long period;
        otIp6Address address;

        VerifyOrExit(argc > 4, error = kThreadError_Parse);

        // mask
        SuccessOrExit(error = ParseLong(argv[1], mask));

        // count
        SuccessOrExit(error = ParseLong(argv[2], count));

        // period
        SuccessOrExit(error = ParseLong(argv[3], period));

        // destination
        SuccessOrExit(error = otIp6AddressFromString(argv[4], &address));

        SuccessOrExit(error = otCommissionerAnnounceBegin(mInstance,
                                                          static_cast<uint32_t>(mask),
                                                          static_cast<uint8_t>(count),
                                                          static_cast<uint16_t>(period),
                                                          &address));
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
                VerifyOrExit(++index < argc, error = kThreadError_Parse);
                value = static_cast<long>(strlen(argv[index]) + 1) / 2;
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

        SuccessOrExit(error = otCommissionerSendMgmtGet(mInstance, tlvs, static_cast<uint8_t>(length)));
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
                VerifyOrExit(++index < argc, error = kThreadError_Parse);
                dataset.mIsLocatorSet = true;
                SuccessOrExit(error = Interpreter::ParseLong(argv[index], value));
                dataset.mLocator = static_cast<uint16_t>(value);
            }
            else if (strcmp(argv[index], "sessionid") == 0)
            {
                VerifyOrExit(++index < argc, error = kThreadError_Parse);
                dataset.mIsSessionIdSet = true;
                SuccessOrExit(error = Interpreter::ParseLong(argv[index], value));
                dataset.mSessionId = static_cast<uint16_t>(value);
            }
            else if (strcmp(argv[index], "steeringdata") == 0)
            {
                VerifyOrExit(++index < argc, error = kThreadError_Parse);
                dataset.mIsSteeringDataSet = true;
                length = static_cast<int>((strlen(argv[index]) + 1) / 2);
                VerifyOrExit(static_cast<size_t>(length) <= OT_STEERING_DATA_MAX_LENGTH, error = kThreadError_NoBufs);
                VerifyOrExit(Interpreter::Hex2Bin(argv[index], dataset.mSteeringData.m8, static_cast<uint16_t>(length)) >= 0,
                             error = kThreadError_Parse);
                dataset.mSteeringData.mLength = static_cast<uint8_t>(length);
                length = 0;
            }
            else if (strcmp(argv[index], "joinerudpport") == 0)
            {
                VerifyOrExit(++index < argc, error = kThreadError_Parse);
                dataset.mIsJoinerUdpPortSet = true;
                SuccessOrExit(error = Interpreter::ParseLong(argv[index], value));
                dataset.mJoinerUdpPort = static_cast<uint16_t>(value);
            }
            else if (strcmp(argv[index], "binary") == 0)
            {
                VerifyOrExit(++index < argc, error = kThreadError_Parse);
                length = static_cast<int>((strlen(argv[index]) + 1) / 2);
                VerifyOrExit(static_cast<size_t>(length) <= sizeof(tlvs), error = kThreadError_NoBufs);
                VerifyOrExit(Interpreter::Hex2Bin(argv[index], tlvs, static_cast<uint16_t>(length)) >= 0,
                             error = kThreadError_Parse);
            }
            else
            {
                ExitNow(error = kThreadError_Parse);
            }
        }

        SuccessOrExit(error = otCommissionerSendMgmtSet(mInstance, &dataset, tlvs, static_cast<uint8_t>(length)));
    }
    else if (strcmp(argv[0], "sessionid") == 0)
    {
        sServer->OutputFormat("%d\r\n", otCommissionerGetSessionId(mInstance));
    }

exit:
    AppendResult(error);
}

void OTCALL Interpreter::s_HandleEnergyReport(uint32_t aChannelMask, const uint8_t *aEnergyList,
                                              uint8_t aEnergyListLength,
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

void OTCALL Interpreter::s_HandlePanIdConflict(uint16_t aPanId, uint32_t aChannelMask, void *aContext)
{
    static_cast<Interpreter *>(aContext)->HandlePanIdConflict(aPanId, aChannelMask);
}

void Interpreter::HandlePanIdConflict(uint16_t aPanId, uint32_t aChannelMask)
{
    sServer->OutputFormat("Conflict: %04x, %08x\r\n", aPanId, aChannelMask);
}

#endif  //  OPENTHREAD_ENABLE_COMMISSIONER && OPENTHREAD_FTD

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
        otJoinerStart(mInstance, argv[1], provisioningUrl,
                      PACKAGE_NAME, PLATFORM_INFO, PACKAGE_VERSION, NULL,
                      &Interpreter::s_HandleJoinerCallback, this);
    }
    else if (strcmp(argv[0], "stop") == 0)
    {
        otJoinerStop(mInstance);
    }

exit:
    AppendResult(error);
}

#endif // OPENTHREAD_ENABLE_JOINER

void OTCALL Interpreter::s_HandleJoinerCallback(ThreadError aError, void *aContext)
{
    static_cast<Interpreter *>(aContext)->HandleJoinerCallback(aError);
}

void Interpreter::HandleJoinerCallback(ThreadError aError)
{
    switch (aError)
    {
    case kThreadError_None:
        sServer->OutputFormat("Join success\r\n");
        break;

    default:
        sServer->OutputFormat("Join failed [%s]\r\n", otThreadErrorToString(aError));
        break;
    }
}

void Interpreter::ProcessJoinerPort(int argc, char *argv[])
{
    ThreadError error = kThreadError_None;
    long value;

    if (argc == 0)
    {
        sServer->OutputFormat("%d\r\n", otThreadGetJoinerUdpPort(mInstance));
    }
    else
    {
        SuccessOrExit(error = ParseLong(argv[0], value));
        error = otThreadSetJoinerUdpPort(mInstance, static_cast<uint16_t>(value));
    }

exit:
    AppendResult(error);
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
        if (otLinkIsWhitelistEnabled(mInstance))
        {
            sServer->OutputFormat("Enabled\r\n");
        }
        else
        {
            sServer->OutputFormat("Disabled\r\n");
        }

        for (uint8_t i = 0; ; i++)
        {
            if (otLinkGetWhitelistEntry(mInstance, i, &entry) != kThreadError_None)
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
            VerifyOrExit(otLinkAddWhitelistRssi(mInstance, extAddr, rssi) == kThreadError_None, error = kThreadError_Parse);
        }
        else
        {
            otLinkAddWhitelist(mInstance, extAddr);
            VerifyOrExit(otLinkAddWhitelist(mInstance, extAddr) == kThreadError_None, error = kThreadError_Parse);
        }
    }
    else if (strcmp(argv[argcur], "clear") == 0)
    {
        otLinkClearWhitelist(mInstance);
    }
    else if (strcmp(argv[argcur], "disable") == 0)
    {
        otLinkSetWhitelistEnabled(mInstance, false);
    }
    else if (strcmp(argv[argcur], "enable") == 0)
    {
        otLinkSetWhitelistEnabled(mInstance, true);
    }
    else if (strcmp(argv[argcur], "remove") == 0)
    {
        VerifyOrExit(++argcur < argc, error = kThreadError_Parse);
        VerifyOrExit(Hex2Bin(argv[argcur], extAddr, sizeof(extAddr)) == sizeof(extAddr), error = kThreadError_Parse);
        otLinkRemoveWhitelist(mInstance, extAddr);
    }

exit:
    AppendResult(error);
}

#if OPENTHREAD_ENABLE_DIAG
void Interpreter::ProcessDiag(int argc, char *argv[])
{
    // all diagnostics related features are processed within diagnostics module
    sServer->OutputFormat("%s\r\n", otDiagProcessCmd(argc, argv));
}
#endif

void Interpreter::ProcessLine(char *aBuf, uint16_t aBufLength, Server &aServer)
{
    char *argv[kMaxArgs];
    char *cmd;
    uint8_t argc = 0, i = 0;

    sServer = &aServer;

    VerifyOrExit(aBuf != NULL);

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
    VerifyOrExit((!otDiagIsEnabled() || (strcmp(cmd, "diag") == 0)),
                 sServer->OutputFormat("under diagnostics mode, execute 'diag stop' before running any other commands.\r\n"));
#endif

    for (i = 0; i < sizeof(sCommands) / sizeof(sCommands[0]); i++)
    {
        if (strcmp(cmd, sCommands[i].mName) == 0)
        {
            (this->*sCommands[i].mCommand)(argc, argv);
            break;
        }
    }

    // Error prompt for unsupported commands
    if (i == sizeof(sCommands) / sizeof(sCommands[0]))
    {
        AppendResult(kThreadError_Parse);
    }

exit:
    return;
}

void OTCALL Interpreter::s_HandleNetifStateChanged(uint32_t aFlags, void *aContext)
{
#ifdef OTDLL
    otCliContext *cliContext = static_cast<otCliContext *>(aContext);
    cliContext->aInterpreter->HandleNetifStateChanged(cliContext->aInstance, aFlags);
#else
    static_cast<Interpreter *>(aContext)->HandleNetifStateChanged(aFlags);
#endif
}

#ifdef OTDLL
void Interpreter::HandleNetifStateChanged(otInstance *aInstance, uint32_t aFlags)
#else
void Interpreter::HandleNetifStateChanged(uint32_t aFlags)
#endif
{
    VerifyOrExit((aFlags & OT_THREAD_NETDATA_UPDATED) != 0);

#ifndef OTDLL
    otIp6SlaacUpdate(mInstance, mSlaacAddresses, sizeof(mSlaacAddresses) / sizeof(mSlaacAddresses[0]),
                     otIp6CreateRandomIid, NULL);
#if OPENTHREAD_ENABLE_DHCP6_SERVER
    otDhcp6ServerUpdate(mInstance);
#endif  // OPENTHREAD_ENABLE_DHCP6_SERVER

#if OPENTHREAD_ENABLE_DHCP6_CLIENT
    otDhcp6ClientUpdate(mInstance, mDhcpAddresses, sizeof(mDhcpAddresses) / sizeof(mDhcpAddresses[0]), NULL);
#endif  // OPENTHREAD_ENABLE_DHCP6_CLIENT
#endif

exit:
    return;
}

void Interpreter::ProcessNetworkDiagnostic(int argc, char *argv[])
{
    ThreadError error = kThreadError_None;
    struct otIp6Address address;
    uint8_t payload[2 + OT_NETWORK_DIAGNOSTIC_TYPELIST_MAX_ENTRIES];  // TypeList Type(1B), len(1B), type list
    uint8_t payloadIndex = 0;
    uint8_t paramIndex = 0;

    VerifyOrExit(argc > 1 + 1, error = kThreadError_Parse);

    SuccessOrExit(error = otIp6AddressFromString(argv[1], &address));

    payloadIndex = 2;
    paramIndex = 2;

    while (paramIndex < argc && payloadIndex < sizeof(payload))
    {
        long value;
        SuccessOrExit(error = ParseLong(argv[paramIndex++], value));
        payload[payloadIndex++] = static_cast<uint8_t>(value);
    }

    payload[0] = OT_NETWORK_DIAGNOSTIC_TYPELIST_TYPE;  // TypeList TLV Type
    payload[1] = payloadIndex - 2;  // length

    if (strcmp(argv[0], "get") == 0)
    {
        otThreadSendDiagnosticGet(mInstance, &address, payload, payloadIndex);
        return;
    }
    else if (strcmp(argv[0], "reset") == 0)
    {
        otThreadSendDiagnosticReset(mInstance, &address, payload, payloadIndex);
    }

exit:
    AppendResult(error);
}

#ifndef OTDLL
void Interpreter::s_HandleDiagnosticGetResponse(otMessage *aMessage, const otMessageInfo *aMessageInfo,
                                                void *aContext)
{
    static_cast<Interpreter *>(aContext)->HandleDiagnosticGetResponse(*static_cast<Message *>(aMessage),
                                                                      *static_cast<const Ip6::MessageInfo *>(aMessageInfo));
}

void Interpreter::HandleDiagnosticGetResponse(Message &aMessage, const Ip6::MessageInfo &)
{
    uint8_t buf[16];
    uint16_t bytesToPrint;
    uint16_t bytesPrinted = 0;
    uint16_t length = aMessage.GetLength() - aMessage.GetOffset();

    sServer->OutputFormat("DIAG_GET.rsp: ");

    while (length > 0)
    {
        bytesToPrint = (length < sizeof(buf)) ? length : sizeof(buf);
        aMessage.Read(aMessage.GetOffset() + bytesPrinted, bytesToPrint, buf);

        OutputBytes(buf, static_cast<uint8_t>(bytesToPrint));

        length       -= bytesToPrint;
        bytesPrinted += bytesToPrint;
    }

    sServer->OutputFormat("\r\n");
}
#endif

}  // namespace Cli
}  // namespace Thread
