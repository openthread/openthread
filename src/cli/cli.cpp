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

#include <openthread/openthread_config.h>

#include "cli.hpp"

#ifdef OTDLL
#include <assert.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include "utils/wrap_string.h"

#include <openthread/openthread.h>
#include <openthread/commissioner.h>
#include <openthread/icmp6.h>
#include <openthread/joiner.h>

#if OPENTHREAD_FTD
#include <openthread/dataset_ftd.h>
#include <openthread/thread_ftd.h>
#endif

#if OPENTHREAD_ENABLE_BORDER_ROUTER
#include <openthread/border_router.h>
#endif

#ifndef OTDLL
#include <openthread/dhcp6_client.h>
#include <openthread/dhcp6_server.h>
#include <openthread/diag.h>
#include <openthread/icmp6.h>
#include <openthread/platform/uart.h>

#include "openthread-instance.h"
#include "common/new.hpp"
#include "net/ip6.hpp"
#endif

#include "cli_dataset.hpp"
#include "cli_uart.hpp"

#if OPENTHREAD_ENABLE_APPLICATION_COAP
#include "cli_coap.hpp"
#endif

#include "common/encoding.hpp"

using ot::Encoding::BigEndian::HostSwap16;
using ot::Encoding::BigEndian::HostSwap32;

namespace ot {

namespace Cli {

const struct Command Interpreter::sCommands[] =
{
    { "help", &Interpreter::ProcessHelp },
    { "autostart", &Interpreter::ProcessAutoStart },
    { "blacklist", &Interpreter::ProcessBlacklist },
    { "bufferinfo", &Interpreter::ProcessBufferInfo },
    { "channel", &Interpreter::ProcessChannel },
#if OPENTHREAD_FTD
    { "child", &Interpreter::ProcessChild },
    { "childmax", &Interpreter::ProcessChildMax },
#endif
    { "childtimeout", &Interpreter::ProcessChildTimeout },
#if OPENTHREAD_ENABLE_APPLICATION_COAP
    { "coap", &Interpreter::ProcessCoap },
#endif
#if OPENTHREAD_ENABLE_COMMISSIONER && OPENTHREAD_FTD
    { "commissioner", &Interpreter::ProcessCommissioner },
#endif
#if OPENTHREAD_FTD
    { "contextreusedelay", &Interpreter::ProcessContextIdReuseDelay },
#endif
    { "counter", &Interpreter::ProcessCounters },
    { "dataset", &Interpreter::ProcessDataset },
#if OPENTHREAD_FTD
    { "delaytimermin", &Interpreter::ProcessDelayTimerMin},
#endif
#if OPENTHREAD_ENABLE_DIAG
    { "diag", &Interpreter::ProcessDiag },
#endif
    { "discover", &Interpreter::ProcessDiscover },
#if OPENTHREAD_ENABLE_DNS_CLIENT
    { "dns", &Interpreter::ProcessDns },
#endif
#if OPENTHREAD_FTD
    { "eidcache", &Interpreter::ProcessEidCache },
#endif
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
#if OPENTHREAD_FTD
    { "joinerport", &Interpreter::ProcessJoinerPort },
#endif
    { "keysequence", &Interpreter::ProcessKeySequence },
    { "leaderdata", &Interpreter::ProcessLeaderData },
#if OPENTHREAD_FTD
    { "leaderpartitionid", &Interpreter::ProcessLeaderPartitionId },
    { "leaderweight", &Interpreter::ProcessLeaderWeight },
#endif
    { "linkquality", &Interpreter::ProcessLinkQuality },
    { "masterkey", &Interpreter::ProcessMasterKey },
    { "mode", &Interpreter::ProcessMode },
#if OPENTHREAD_ENABLE_BORDER_ROUTER
    { "netdataregister", &Interpreter::ProcessNetworkDataRegister },
#endif
#if OPENTHREAD_FTD || OPENTHREAD_ENABLE_MTD_NETWORK_DIAGNOSTIC
    { "networkdiagnostic", &Interpreter::ProcessNetworkDiagnostic },
#endif // OPENTHREAD_FTD || OPENTHREAD_ENABLE_MTD_NETWORK_DIAGNOSTIC
#if OPENTHREAD_FTD
    { "networkidtimeout", &Interpreter::ProcessNetworkIdTimeout },
#endif
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
#if OPENTHREAD_ENABLE_BORDER_ROUTER
    { "prefix", &Interpreter::ProcessPrefix },
#endif
#if OPENTHREAD_FTD
    { "pskc", &Interpreter::ProcessPSKc },
    { "releaserouterid", &Interpreter::ProcessReleaseRouterId },
#endif
    { "reset", &Interpreter::ProcessReset },
    { "rloc16", &Interpreter::ProcessRloc16 },
#if OPENTHREAD_ENABLE_BORDER_ROUTER
    { "route", &Interpreter::ProcessRoute },
#endif
#if OPENTHREAD_FTD
    { "router", &Interpreter::ProcessRouter },
    { "routerdowngradethreshold", &Interpreter::ProcessRouterDowngradeThreshold },
    { "routerrole", &Interpreter::ProcessRouterRole },
    { "routerselectionjitter", &Interpreter::ProcessRouterSelectionJitter },
    { "routerupgradethreshold", &Interpreter::ProcessRouterUpgradeThreshold },
#endif
    { "scan", &Interpreter::ProcessScan },
    { "singleton", &Interpreter::ProcessSingleton },
    { "state", &Interpreter::ProcessState },
    { "thread", &Interpreter::ProcessThread },
    { "txpowermax", &Interpreter::ProcessTxPowerMax },
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
#if OPENTHREAD_ENABLE_APPLICATION_COAP
    mCoap(*this),
#endif
    mServer(NULL),
#ifdef OTDLL
    mApiInstance(otApiInit()),
    mInstanceIndex(0),
#else
    mLength(8),
    mCount(1),
    mInterval(1000),
    mPingTimer(aInstance->mIp6.mTimerScheduler, &Interpreter::s_HandlePingTimer, this),
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
#if OPENTHREAD_FTD || OPENTHREAD_ENABLE_MTD_NETWORK_DIAGNOSTIC
    otThreadSetReceiveDiagnosticGetCallback(mInstance, &Interpreter::s_HandleDiagnosticGetResponse, this);
#endif

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

void Interpreter::AppendResult(otError aError) const
{
    if (aError == OT_ERROR_NONE)
    {
        mServer->OutputFormat("Done\r\n");
    }
    else
    {
        mServer->OutputFormat("Error %d: %s\r\n", aError, otThreadErrorToString(aError));
    }
}

void Interpreter::OutputBytes(const uint8_t *aBytes, uint8_t aLength) const
{
    for (int i = 0; i < aLength; i++)
    {
        mServer->OutputFormat("%02x", aBytes[i]);
    }
}

otError Interpreter::ParseLong(char *argv, long &value)
{
    char *endptr;
    value = strtol(argv, &endptr, 0);
    return (*endptr == '\0') ? OT_ERROR_NONE : OT_ERROR_PARSE;
}

otError Interpreter::ParseUnsignedLong(char *argv, unsigned long &value)
{
    char *endptr;
    value = strtoul(argv, &endptr, 0);
    return (*endptr == '\0') ? OT_ERROR_NONE : OT_ERROR_PARSE;
}

void Interpreter::ProcessHelp(int argc, char *argv[])
{
    for (unsigned int i = 0; i < sizeof(sCommands) / sizeof(sCommands[0]); i++)
    {
        mServer->OutputFormat("%s\r\n", sCommands[i].mName);
    }

    (void)argc;
    (void)argv;
}

void Interpreter::ProcessAutoStart(int argc, char *argv[])
{
    otError error = OT_ERROR_NONE;

    if (argc == 0)
    {
        if (otThreadGetAutoStart(mInstance))
        {
            mServer->OutputFormat("true\r\n");
        }
        else
        {
            mServer->OutputFormat("false\r\n");
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
        error = OT_ERROR_INVALID_ARGS;
    }

    AppendResult(error);
}

void Interpreter::ProcessBlacklist(int argc, char *argv[])
{
    otError error = OT_ERROR_NONE;
    otMacBlacklistEntry entry;
    int argcur = 0;
    uint8_t extAddr[8];

    if (argcur >= argc)
    {
        if (otLinkIsBlacklistEnabled(mInstance))
        {
            mServer->OutputFormat("Enabled\r\n");
        }
        else
        {
            mServer->OutputFormat("Disabled\r\n");
        }

        for (uint8_t i = 0; ; i++)
        {
            if (otLinkGetBlacklistEntry(mInstance, i, &entry) != OT_ERROR_NONE)
            {
                break;
            }

            if (entry.mValid == false)
            {
                continue;
            }

            OutputBytes(entry.mExtAddress.m8, OT_EXT_ADDRESS_SIZE);

            mServer->OutputFormat("\r\n");
        }
    }
    else if (strcmp(argv[argcur], "add") == 0)
    {
        VerifyOrExit(++argcur < argc, error = OT_ERROR_PARSE);
        VerifyOrExit(Hex2Bin(argv[argcur], extAddr, sizeof(extAddr)) == sizeof(extAddr), error = OT_ERROR_PARSE);

        otLinkAddBlacklist(mInstance, extAddr);
        VerifyOrExit(otLinkAddBlacklist(mInstance, extAddr) == OT_ERROR_NONE, error = OT_ERROR_PARSE);
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
        VerifyOrExit(++argcur < argc, error = OT_ERROR_PARSE);
        VerifyOrExit(Hex2Bin(argv[argcur], extAddr, sizeof(extAddr)) == sizeof(extAddr), error = OT_ERROR_PARSE);
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

    mServer->OutputFormat("total: %d\r\n", bufferInfo.mTotalBuffers);
    mServer->OutputFormat("free: %d\r\n", bufferInfo.mFreeBuffers);
    mServer->OutputFormat("6lo send: %d %d\r\n", bufferInfo.m6loSendMessages, bufferInfo.m6loSendBuffers);
    mServer->OutputFormat("6lo reas: %d %d\r\n", bufferInfo.m6loReassemblyMessages, bufferInfo.m6loReassemblyBuffers);
    mServer->OutputFormat("ip6: %d %d\r\n", bufferInfo.mIp6Messages, bufferInfo.mIp6Buffers);
    mServer->OutputFormat("mpl: %d %d\r\n", bufferInfo.mMplMessages, bufferInfo.mMplBuffers);
    mServer->OutputFormat("mle: %d %d\r\n", bufferInfo.mMleMessages, bufferInfo.mMleBuffers);
    mServer->OutputFormat("arp: %d %d\r\n", bufferInfo.mArpMessages, bufferInfo.mArpBuffers);
    mServer->OutputFormat("coap: %d %d\r\n", bufferInfo.mCoapMessages, bufferInfo.mCoapBuffers);
    mServer->OutputFormat("coap secure: %d %d\r\n", bufferInfo.mCoapSecureMessages, bufferInfo.mCoapSecureBuffers);

    AppendResult(OT_ERROR_NONE);
}

void Interpreter::ProcessChannel(int argc, char *argv[])
{
    otError error = OT_ERROR_NONE;
    long value;

    if (argc == 0)
    {
        mServer->OutputFormat("%d\r\n", otLinkGetChannel(mInstance));
    }
    else
    {
        SuccessOrExit(error = ParseLong(argv[0], value));
        error = otLinkSetChannel(mInstance, static_cast<uint8_t>(value));
    }

exit:
    AppendResult(error);
}

#if OPENTHREAD_FTD
void Interpreter::ProcessChild(int argc, char *argv[])
{
    otError error = OT_ERROR_NONE;
    otChildInfo childInfo;
    uint8_t maxChildren;
    long value;
    bool isTable = false;

    VerifyOrExit(argc > 0, error = OT_ERROR_PARSE);

    if (strcmp(argv[0], "list") == 0 || (isTable = (strcmp(argv[0], "table") == 0)))
    {
        if (isTable)
        {
            mServer->OutputFormat("| ID  | RLOC16 | Timeout    | Age        | LQ In | C_VN |R|S|D|N| Extended MAC     |\r\n");
            mServer->OutputFormat("+-----+--------+------------+------------+-------+------+-+-+-+-+------------------+\r\n");
        }

        maxChildren = otThreadGetMaxAllowedChildren(mInstance);

        for (uint8_t i = 0; i < maxChildren ; i++)
        {

            switch (otThreadGetChildInfoByIndex(mInstance, i, &childInfo))
            {
            case OT_ERROR_NONE:
                break;

            case OT_ERROR_NOT_FOUND:
                continue;

            default:
                mServer->OutputFormat("\r\n");
                ExitNow();
            }

            if (childInfo.mTimeout > 0)
            {
                if (isTable)
                {
                    mServer->OutputFormat("| %3d ", childInfo.mChildId);
                    mServer->OutputFormat("| 0x%04x ", childInfo.mRloc16);
                    mServer->OutputFormat("| %10d ", childInfo.mTimeout);
                    mServer->OutputFormat("| %10d ", childInfo.mAge);
                    mServer->OutputFormat("| %5d ", childInfo.mLinkQualityIn);
                    mServer->OutputFormat("| %4d ", childInfo.mNetworkDataVersion);
                    mServer->OutputFormat("|%1d", childInfo.mRxOnWhenIdle);
                    mServer->OutputFormat("|%1d", childInfo.mSecureDataRequest);
                    mServer->OutputFormat("|%1d", childInfo.mFullFunction);
                    mServer->OutputFormat("|%1d", childInfo.mFullNetworkData);
                    mServer->OutputFormat("| ");

                    for (size_t j = 0; j < sizeof(childInfo.mExtAddress); j++)
                    {
                        mServer->OutputFormat("%02x", childInfo.mExtAddress.m8[j]);
                    }

                    mServer->OutputFormat(" |\r\n");
                }
                else
                {
                    mServer->OutputFormat("%d ", childInfo.mChildId);
                }
            }
        }

        ExitNow();
    }

    SuccessOrExit(error = ParseLong(argv[0], value));
    SuccessOrExit(error = otThreadGetChildInfoById(mInstance, static_cast<uint16_t>(value), &childInfo));

    mServer->OutputFormat("Child ID: %d\r\n", childInfo.mChildId);
    mServer->OutputFormat("Rloc: %04x\r\n", childInfo.mRloc16);
    mServer->OutputFormat("Ext Addr: ");

    for (size_t j = 0; j < sizeof(childInfo.mExtAddress); j++)
    {
        mServer->OutputFormat("%02x", childInfo.mExtAddress.m8[j]);
    }

    mServer->OutputFormat("\r\n");
    mServer->OutputFormat("Mode: ");

    if (childInfo.mRxOnWhenIdle)
    {
        mServer->OutputFormat("r");
    }

    if (childInfo.mSecureDataRequest)
    {
        mServer->OutputFormat("s");
    }

    if (childInfo.mFullFunction)
    {
        mServer->OutputFormat("d");
    }

    if (childInfo.mFullNetworkData)
    {
        mServer->OutputFormat("n");
    }

    mServer->OutputFormat("\r\n");

    mServer->OutputFormat("Net Data: %d\r\n", childInfo.mNetworkDataVersion);
    mServer->OutputFormat("Timeout: %d\r\n", childInfo.mTimeout);
    mServer->OutputFormat("Age: %d\r\n", childInfo.mAge);
    mServer->OutputFormat("Link Quality In: %d\r\n", childInfo.mLinkQualityIn);
    mServer->OutputFormat("RSSI: %d\r\n", childInfo.mAverageRssi);

exit:
    AppendResult(error);
}

void Interpreter::ProcessChildMax(int argc, char *argv[])
{
    otError error = OT_ERROR_NONE;
    long value;

    if (argc == 0)
    {
        mServer->OutputFormat("%d\r\n", otThreadGetMaxAllowedChildren(mInstance));
    }
    else
    {
        SuccessOrExit(error = ParseLong(argv[0], value));
        SuccessOrExit(error = otThreadSetMaxAllowedChildren(mInstance, static_cast<uint8_t>(value)));
    }

exit:
    AppendResult(error);
}
#endif  // OPENTHREAD_FTD

void Interpreter::ProcessChildTimeout(int argc, char *argv[])
{
    otError error = OT_ERROR_NONE;
    long value;

    if (argc == 0)
    {
        mServer->OutputFormat("%d\r\n", otThreadGetChildTimeout(mInstance));
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
    otError error;
    error = mCoap.Process(argc, argv);
    AppendResult(error);
}

#endif // OPENTHREAD_ENABLE_APPLICATION_COAP

#if OPENTHREAD_FTD
void Interpreter::ProcessContextIdReuseDelay(int argc, char *argv[])
{
    otError error = OT_ERROR_NONE;
    long value;

    if (argc == 0)
    {
        mServer->OutputFormat("%d\r\n", otThreadGetContextIdReuseDelay(mInstance));
    }
    else
    {
        SuccessOrExit(ParseLong(argv[0], value));
        otThreadSetContextIdReuseDelay(mInstance, static_cast<uint32_t>(value));
    }

exit:
    AppendResult(error);
}
#endif  // OPENTHREAD_FTD

void Interpreter::ProcessCounters(int argc, char *argv[])
{
    if (argc == 0)
    {
        mServer->OutputFormat("mac\r\n");
        mServer->OutputFormat("Done\r\n");
    }
    else
    {
        if (strcmp(argv[0], "mac") == 0)
        {
            otMacCountersPtr counters(otLinkGetCounters(mInstance));
            mServer->OutputFormat("TxTotal: %d\r\n", counters->mTxTotal);
            mServer->OutputFormat("    TxUnicast: %d\r\n", counters->mTxUnicast);
            mServer->OutputFormat("    TxBroadcast: %d\r\n", counters->mTxBroadcast);
            mServer->OutputFormat("    TxAckRequested: %d\r\n", counters->mTxAckRequested);
            mServer->OutputFormat("    TxAcked: %d\r\n", counters->mTxAcked);
            mServer->OutputFormat("    TxNoAckRequested: %d\r\n", counters->mTxNoAckRequested);
            mServer->OutputFormat("    TxData: %d\r\n", counters->mTxData);
            mServer->OutputFormat("    TxDataPoll: %d\r\n", counters->mTxDataPoll);
            mServer->OutputFormat("    TxBeacon: %d\r\n", counters->mTxBeacon);
            mServer->OutputFormat("    TxBeaconRequest: %d\r\n", counters->mTxBeaconRequest);
            mServer->OutputFormat("    TxOther: %d\r\n", counters->mTxOther);
            mServer->OutputFormat("    TxRetry: %d\r\n", counters->mTxRetry);
            mServer->OutputFormat("    TxErrCca: %d\r\n", counters->mTxErrCca);
            mServer->OutputFormat("RxTotal: %d\r\n", counters->mRxTotal);
            mServer->OutputFormat("    RxUnicast: %d\r\n", counters->mRxUnicast);
            mServer->OutputFormat("    RxBroadcast: %d\r\n", counters->mRxBroadcast);
            mServer->OutputFormat("    RxData: %d\r\n", counters->mRxData);
            mServer->OutputFormat("    RxDataPoll: %d\r\n", counters->mRxDataPoll);
            mServer->OutputFormat("    RxBeacon: %d\r\n", counters->mRxBeacon);
            mServer->OutputFormat("    RxBeaconRequest: %d\r\n", counters->mRxBeaconRequest);
            mServer->OutputFormat("    RxOther: %d\r\n", counters->mRxOther);
            mServer->OutputFormat("    RxWhitelistFiltered: %d\r\n", counters->mRxWhitelistFiltered);
            mServer->OutputFormat("    RxDestAddrFiltered: %d\r\n", counters->mRxDestAddrFiltered);
            mServer->OutputFormat("    RxDuplicated: %d\r\n", counters->mRxDuplicated);
            mServer->OutputFormat("    RxErrNoFrame: %d\r\n", counters->mRxErrNoFrame);
            mServer->OutputFormat("    RxErrNoUnknownNeighbor: %d\r\n", counters->mRxErrUnknownNeighbor);
            mServer->OutputFormat("    RxErrInvalidSrcAddr: %d\r\n", counters->mRxErrInvalidSrcAddr);
            mServer->OutputFormat("    RxErrSec: %d\r\n", counters->mRxErrSec);
            mServer->OutputFormat("    RxErrFcs: %d\r\n", counters->mRxErrFcs);
            mServer->OutputFormat("    RxErrOther: %d\r\n", counters->mRxErrOther);
        }
    }
}

void Interpreter::ProcessDataset(int argc, char *argv[])
{
    otError error;
    error = Dataset::Process(mInstance, argc, argv, *mServer);
    AppendResult(error);
}

#if OPENTHREAD_FTD
void Interpreter::ProcessDelayTimerMin(int argc, char *argv[])
{
    otError error = OT_ERROR_NONE;

    if (argc == 0)
    {
        mServer->OutputFormat("%d\r\n", (otDatasetGetDelayTimerMinimal(mInstance) / 1000));
    }
    else if (argc == 1)
    {
        unsigned long value;
        SuccessOrExit(error = ParseUnsignedLong(argv[0], value));
        SuccessOrExit(error = otDatasetSetDelayTimerMinimal(mInstance, static_cast<uint32_t>(value * 1000)));
    }
    else
    {
        error = OT_ERROR_INVALID_ARGS;
    }

exit:
    AppendResult(error);
}
#endif

void Interpreter::ProcessDiscover(int argc, char *argv[])
{
    otError error = OT_ERROR_NONE;
    uint32_t scanChannels = 0;
    long value;

    if (argc > 0)
    {
        SuccessOrExit(error = ParseLong(argv[0], value));
        scanChannels = 1 << value;
    }

    SuccessOrExit(error = otThreadDiscover(mInstance, scanChannels, OT_PANID_BROADCAST, false, false,
                                           &Interpreter::s_HandleActiveScanResult, this));
    mServer->OutputFormat("| J | Network Name     | Extended PAN     | PAN  | MAC Address      | Ch | dBm | LQI |\r\n");
    mServer->OutputFormat("+---+------------------+------------------+------+------------------+----+-----+-----+\r\n");

    return;

exit:
    AppendResult(error);
}

#if OPENTHREAD_ENABLE_DNS_CLIENT
void Interpreter::ProcessDns(int argc, char *argv[])
{
    otError error = OT_ERROR_NONE;
    long port = OT_DNS_DEFAULT_DNS_SERVER_PORT;
    Ip6::MessageInfo messageInfo;
    otDnsQuery query;

    VerifyOrExit(argc > 0, error = OT_ERROR_INVALID_ARGS);

    if (strcmp(argv[0], "resolve") == 0)
    {
        VerifyOrExit(!mResolvingInProgress, error = OT_ERROR_BUSY);
        VerifyOrExit(argc > 1, error = OT_ERROR_INVALID_ARGS);
        VerifyOrExit(strlen(argv[1]) < OT_DNS_MAX_HOSTNAME_LENGTH, error = OT_ERROR_INVALID_ARGS);

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
        ExitNow(error = OT_ERROR_INVALID_ARGS);
    }

exit:
    AppendResult(error);
}

void Interpreter::s_HandleDnsResponse(void *aContext, const char *aHostname, otIp6Address *aAddress,
                                      uint32_t aTtl, otError aResult)
{
    static_cast<Interpreter *>(aContext)->HandleDnsResponse(aHostname,
                                                            *static_cast<Ip6::Address *>(aAddress),
                                                            aTtl, aResult);
}

void Interpreter::HandleDnsResponse(const char *aHostname, Ip6::Address &aAddress, uint32_t aTtl, otError aResult)
{
    mServer->OutputFormat("DNS response for %s - ", aHostname);

    if (aResult == OT_ERROR_NONE)
    {
        mServer->OutputFormat("[%x:%x:%x:%x:%x:%x:%x:%x] TTL: %d\r\n",
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

#if OPENTHREAD_FTD
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

        mServer->OutputFormat("%x:%x:%x:%x:%x:%x:%x:%x %04x\r\n",
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
    AppendResult(OT_ERROR_NONE);
}
#endif  // OPENTHREAD_FTD

void Interpreter::ProcessEui64(int argc, char *argv[])
{
    otError error = OT_ERROR_NONE;
    otExtAddress extAddress;

    VerifyOrExit(argc == 0, error = OT_ERROR_PARSE);

    otLinkGetFactoryAssignedIeeeEui64(mInstance, &extAddress);
    OutputBytes(extAddress.m8, OT_EXT_ADDRESS_SIZE);
    mServer->OutputFormat("\r\n");

exit:
    (void)argv;
    AppendResult(error);
}

void Interpreter::ProcessExtAddress(int argc, char *argv[])
{
    otError error = OT_ERROR_NONE;

    if (argc == 0)
    {
        otBufferPtr extAddress(otLinkGetExtendedAddress(mInstance));
        OutputBytes(extAddress, OT_EXT_ADDRESS_SIZE);
        mServer->OutputFormat("\r\n");
    }
    else
    {
        otExtAddress extAddress;

        VerifyOrExit(Hex2Bin(argv[0], extAddress.m8, sizeof(otExtAddress)) >= 0, error = OT_ERROR_PARSE);

        error = otLinkSetExtendedAddress(mInstance, &extAddress);
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
    otError error = OT_ERROR_NONE;

    if (argc == 0)
    {
        otBufferPtr extPanId(otThreadGetExtendedPanId(mInstance));
        OutputBytes(extPanId, OT_EXT_PAN_ID_SIZE);
        mServer->OutputFormat("\r\n");
    }
    else
    {
        uint8_t extPanId[8];

        VerifyOrExit(Hex2Bin(argv[0], extPanId, sizeof(extPanId)) >= 0, error = OT_ERROR_PARSE);

        error = otThreadSetExtendedPanId(mInstance, extPanId);
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
    otError error = OT_ERROR_NONE;
    otExtAddress hashMacAddress;

    VerifyOrExit(argc == 0, error = OT_ERROR_PARSE);

    otLinkGetJoinerId(mInstance, &hashMacAddress);
    OutputBytes(hashMacAddress.m8, OT_EXT_ADDRESS_SIZE);
    mServer->OutputFormat("\r\n");

exit:
    (void)argv;
    AppendResult(error);
}

void Interpreter::ProcessIfconfig(int argc, char *argv[])
{
    otError error = OT_ERROR_NONE;

    if (argc == 0)
    {
        if (otIp6IsEnabled(mInstance))
        {
            mServer->OutputFormat("up\r\n");
        }
        else
        {
            mServer->OutputFormat("down\r\n");
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

otError Interpreter::ProcessIpAddrAdd(int argc, char *argv[])
{
    otError error;
    otNetifAddress aAddress;

    VerifyOrExit(argc > 0, error = OT_ERROR_PARSE);

    SuccessOrExit(error = otIp6AddressFromString(argv[0], &aAddress.mAddress));
    aAddress.mPrefixLength = 64;
    aAddress.mPreferred = true;
    aAddress.mValid = true;
    error = otIp6AddUnicastAddress(mInstance, &aAddress);

exit:
    return error;
}

otError Interpreter::ProcessIpAddrDel(int argc, char *argv[])
{
    otError error;
    struct otIp6Address address;

    VerifyOrExit(argc > 0, error = OT_ERROR_PARSE);

    SuccessOrExit(error = otIp6AddressFromString(argv[0], &address));
    error = otIp6RemoveUnicastAddress(mInstance, &address);

exit:
    return error;
}

void Interpreter::ProcessIpAddr(int argc, char *argv[])
{
    otError error = OT_ERROR_NONE;

    if (argc == 0)
    {
        otNetifAddressPtr unicastAddrs(otIp6GetUnicastAddresses(mInstance));

        for (const otNetifAddress *addr = unicastAddrs; addr; addr = addr->mNext)
        {
            mServer->OutputFormat("%x:%x:%x:%x:%x:%x:%x:%x\r\n",
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
otError Interpreter::ProcessIpMulticastAddrAdd(int argc, char *argv[])
{
    otError error;
    struct otIp6Address address;

    VerifyOrExit(argc > 0, error = OT_ERROR_PARSE);

    SuccessOrExit(error = otIp6AddressFromString(argv[0], &address));
    error = otIp6SubscribeMulticastAddress(mInstance, &address);

exit:
    return error;
}

otError Interpreter::ProcessIpMulticastAddrDel(int argc, char *argv[])
{
    otError error;
    struct otIp6Address address;

    VerifyOrExit(argc > 0, error = OT_ERROR_PARSE);

    SuccessOrExit(error = otIp6AddressFromString(argv[0], &address));
    error = otIp6UnsubscribeMulticastAddress(mInstance, &address);

exit:
    return error;
}

otError Interpreter::ProcessMulticastPromiscuous(int argc, char *argv[])
{
    otError error = OT_ERROR_NONE;

    if (argc == 0)
    {
        if (otIp6IsMulticastPromiscuousEnabled(mInstance))
        {
            mServer->OutputFormat("Enabled\r\n");
        }
        else
        {
            mServer->OutputFormat("Disabled\r\n");
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
            ExitNow(error = OT_ERROR_PARSE);
        }
    }

exit:
    return error;
}

void Interpreter::ProcessIpMulticastAddr(int argc, char *argv[])
{
    otError error = OT_ERROR_NONE;

    if (argc == 0)
    {
        for (const otNetifMulticastAddress *addr = otIp6GetMulticastAddresses(mInstance); addr; addr = addr->mNext)
        {
            mServer->OutputFormat("%x:%x:%x:%x:%x:%x:%x:%x\r\n",
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
    otError error = OT_ERROR_NONE;
    long value;

    VerifyOrExit(argc == 1 || argc == 2, error = OT_ERROR_PARSE);

    if (strcmp(argv[0], "counter") == 0)
    {
        if (argc == 1)
        {
            mServer->OutputFormat("%d\r\n", otThreadGetKeySequenceCounter(mInstance));
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
            mServer->OutputFormat("%d\r\n", otThreadGetKeySwitchGuardTime(mInstance));
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
    otError error;
    otLeaderData leaderData;

    SuccessOrExit(error = otThreadGetLeaderData(mInstance, &leaderData));

    mServer->OutputFormat("Partition ID: %u\r\n", leaderData.mPartitionId);
    mServer->OutputFormat("Weighting: %d\r\n", leaderData.mWeighting);
    mServer->OutputFormat("Data Version: %d\r\n", leaderData.mDataVersion);
    mServer->OutputFormat("Stable Data Version: %d\r\n", leaderData.mStableDataVersion);
    mServer->OutputFormat("Leader Router ID: %d\r\n", leaderData.mLeaderRouterId);

exit:
    (void)argc;
    (void)argv;
    AppendResult(error);
}

#if OPENTHREAD_FTD
void Interpreter::ProcessLeaderPartitionId(int argc, char *argv[])
{
    otError error = OT_ERROR_NONE;
    unsigned long value;

    if (argc == 0)
    {
        mServer->OutputFormat("%u\r\n", otThreadGetLocalLeaderPartitionId(mInstance));
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
    otError error = OT_ERROR_NONE;
    long value;

    if (argc == 0)
    {
        mServer->OutputFormat("%d\r\n", otThreadGetLocalLeaderWeight(mInstance));
    }
    else
    {
        SuccessOrExit(error = ParseLong(argv[0], value));
        otThreadSetLocalLeaderWeight(mInstance, static_cast<uint8_t>(value));
    }

exit:
    AppendResult(error);
}
#endif  // OPENTHREAD_FTD

void Interpreter::ProcessLinkQuality(int argc, char *argv[])
{
    otError error = OT_ERROR_NONE;
    uint8_t extAddress[8];
    uint8_t linkQuality;
    long value;

    VerifyOrExit(argc > 0, error = OT_ERROR_INVALID_ARGS);
    VerifyOrExit(Hex2Bin(argv[0], extAddress, OT_EXT_ADDRESS_SIZE) >= 0, error = OT_ERROR_PARSE);

    if (argc == 1)
    {
        VerifyOrExit(otLinkGetAssignLinkQuality(mInstance, extAddress, &linkQuality) == OT_ERROR_NONE,
                     error = OT_ERROR_INVALID_ARGS);
        mServer->OutputFormat("%d\r\n", linkQuality);
    }
    else
    {
        SuccessOrExit(error = ParseLong(argv[1], value));
        otLinkSetAssignLinkQuality(mInstance, extAddress, static_cast<uint8_t>(value));
    }

exit:
    AppendResult(error);
}

#if OPENTHREAD_FTD
void Interpreter::ProcessPSKc(int argc, char *argv[])
{
    otError error = OT_ERROR_NONE;

    if (argc == 0)
    {
        const uint8_t *currentPSKc = otThreadGetPSKc(mInstance);

        for (int i = 0; i < OT_PSKC_MAX_SIZE; i++)
        {
            mServer->OutputFormat("%02x", currentPSKc[i]);
        }

        mServer->OutputFormat("\r\n");
    }
    else
    {
        uint8_t newPSKc[OT_PSKC_MAX_SIZE];

        VerifyOrExit(Hex2Bin(argv[0], newPSKc, sizeof(newPSKc)) == OT_PSKC_MAX_SIZE, error = OT_ERROR_PARSE);
        SuccessOrExit(error = otThreadSetPSKc(mInstance, newPSKc));
    }

exit:
    AppendResult(error);
}
#endif

void Interpreter::ProcessMasterKey(int argc, char *argv[])
{
    otError error = OT_ERROR_NONE;

    if (argc == 0)
    {
        otBufferPtr key(reinterpret_cast<const uint8_t *>(otThreadGetMasterKey(mInstance)));

        for (int i = 0; i < OT_MASTER_KEY_SIZE; i++)
        {
            mServer->OutputFormat("%02x", key[i]);
        }

        mServer->OutputFormat("\r\n");
    }
    else
    {
        otMasterKey key;

        VerifyOrExit(Hex2Bin(argv[0], key.m8, sizeof(key.m8)) == OT_MASTER_KEY_SIZE, error = OT_ERROR_PARSE);
        SuccessOrExit(error = otThreadSetMasterKey(mInstance, &key));
    }

exit:
    AppendResult(error);
}

void Interpreter::ProcessMode(int argc, char *argv[])
{
    otError error = OT_ERROR_NONE;
    otLinkModeConfig linkMode;

    memset(&linkMode, 0, sizeof(otLinkModeConfig));

    if (argc == 0)
    {
        linkMode = otThreadGetLinkMode(mInstance);

        if (linkMode.mRxOnWhenIdle)
        {
            mServer->OutputFormat("r");
        }

        if (linkMode.mSecureDataRequests)
        {
            mServer->OutputFormat("s");
        }

        if (linkMode.mDeviceType)
        {
            mServer->OutputFormat("d");
        }

        if (linkMode.mNetworkData)
        {
            mServer->OutputFormat("n");
        }

        mServer->OutputFormat("\r\n");
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
                ExitNow(error = OT_ERROR_PARSE);
            }
        }

        SuccessOrExit(error = otThreadSetLinkMode(mInstance, linkMode));
    }

exit:
    AppendResult(error);
}

#if OPENTHREAD_ENABLE_BORDER_ROUTER
void Interpreter::ProcessNetworkDataRegister(int argc, char *argv[])
{
    otError error = OT_ERROR_NONE;
    SuccessOrExit(error = otBorderRouterRegister(mInstance));

exit:
    (void)argc;
    (void)argv;
    AppendResult(error);
}
#endif  // OPENTHREAD_ENABLE_BORDER_ROUTER

#if OPENTHREAD_FTD
void Interpreter::ProcessNetworkIdTimeout(int argc, char *argv[])
{
    otError error = OT_ERROR_NONE;
    long value;

    if (argc == 0)
    {
        mServer->OutputFormat("%d\r\n", otThreadGetNetworkIdTimeout(mInstance));
    }
    else
    {
        SuccessOrExit(error = ParseLong(argv[0], value));
        otThreadSetNetworkIdTimeout(mInstance, static_cast<uint8_t>(value));
    }

exit:
    AppendResult(error);
}
#endif  // OPENTHREAD_FTD

void Interpreter::ProcessNetworkName(int argc, char *argv[])
{
    otError error = OT_ERROR_NONE;

    if (argc == 0)
    {
        otStringPtr networkName(otThreadGetNetworkName(mInstance));
        mServer->OutputFormat("%.*s\r\n", OT_NETWORK_NAME_MAX_SIZE, (const char *)networkName);
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
    otError error = OT_ERROR_NONE;
    long value;

    if (argc == 0)
    {
        mServer->OutputFormat("%04x\r\n", otLinkGetPanId(mInstance));
    }
    else
    {
        SuccessOrExit(error = ParseLong(argv[0], value));
        error = otLinkSetPanId(mInstance, static_cast<otPanId>(value));
    }

exit:
    AppendResult(error);
}

void Interpreter::ProcessParent(int argc, char *argv[])
{
    otError error = OT_ERROR_NONE;
    otRouterInfo parentInfo;

    SuccessOrExit(error = otThreadGetParentInfo(mInstance, &parentInfo));
    mServer->OutputFormat("Ext Addr: ");

    for (size_t i = 0; i < sizeof(parentInfo.mExtAddress); i++)
    {
        mServer->OutputFormat("%02x", parentInfo.mExtAddress.m8[i]);
    }

    mServer->OutputFormat("\r\n");

    mServer->OutputFormat("Rloc: %x\r\n", parentInfo.mRloc16);
    mServer->OutputFormat("Link Quality In: %d\r\n", parentInfo.mLinkQualityIn);
    mServer->OutputFormat("Link Quality Out: %d\r\n", parentInfo.mLinkQualityOut);
    mServer->OutputFormat("Age: %d\r\n", parentInfo.mAge);

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
                                    const otIcmp6Header &aIcmpHeader)
{
    uint32_t timestamp = 0;

    VerifyOrExit(aIcmpHeader.mType == OT_ICMP6_TYPE_ECHO_REPLY);

    mServer->OutputFormat("%d bytes from ", aMessage.GetLength() - aMessage.GetOffset());
    mServer->OutputFormat("%x:%x:%x:%x:%x:%x:%x:%x",
                          HostSwap16(aMessageInfo.GetPeerAddr().mFields.m16[0]),
                          HostSwap16(aMessageInfo.GetPeerAddr().mFields.m16[1]),
                          HostSwap16(aMessageInfo.GetPeerAddr().mFields.m16[2]),
                          HostSwap16(aMessageInfo.GetPeerAddr().mFields.m16[3]),
                          HostSwap16(aMessageInfo.GetPeerAddr().mFields.m16[4]),
                          HostSwap16(aMessageInfo.GetPeerAddr().mFields.m16[5]),
                          HostSwap16(aMessageInfo.GetPeerAddr().mFields.m16[6]),
                          HostSwap16(aMessageInfo.GetPeerAddr().mFields.m16[7]));
    mServer->OutputFormat(": icmp_seq=%d hlim=%d", HostSwap16(aIcmpHeader.mData.m16[1]), aMessageInfo.mHopLimit);

    if (aMessage.Read(aMessage.GetOffset(), sizeof(uint32_t), &timestamp) >=
        static_cast<int>(sizeof(uint32_t)))
    {
        mServer->OutputFormat(" time=%dms", Timer::GetNow() - HostSwap32(timestamp));
    }

    mServer->OutputFormat("\r\n");

exit:
    return;
}

void Interpreter::ProcessPing(int argc, char *argv[])
{
    otError error = OT_ERROR_NONE;
    uint8_t index = 1;
    long value;

    VerifyOrExit(argc > 0, error = OT_ERROR_PARSE);
    VerifyOrExit(!mPingTimer.IsRunning(), error = OT_ERROR_BUSY);

    memset(&mMessageInfo, 0, sizeof(mMessageInfo));
    SuccessOrExit(error = mMessageInfo.GetPeerAddr().FromString(argv[0]));
    mMessageInfo.mInterfaceId = OT_NETIF_INTERFACE_ID_THREAD;

    mLength = 8;
    mCount = 1;
    mInterval = 1000;

    while (index < argc)
    {
        SuccessOrExit(error = ParseLong(argv[index], value));

        switch (index)
        {
        case 1:
            mLength = (uint16_t)value;
            break;

        case 2:
            mCount = (uint16_t)value;
            break;

        case 3:
            mInterval = (uint32_t)value;
            mInterval = mInterval * 1000;
            break;

        default:
            ExitNow(error = OT_ERROR_PARSE);
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
    otError error = OT_ERROR_NONE;
    uint32_t timestamp = HostSwap32(Timer::GetNow());

    otMessage *message;
    const otMessageInfo *messageInfo = static_cast<const otMessageInfo *>(&mMessageInfo);

    VerifyOrExit((message = otIp6NewMessage(mInstance, true)) != NULL, error = OT_ERROR_NO_BUFS);
    SuccessOrExit(error = otMessageAppend(message, &timestamp, sizeof(timestamp)));
    SuccessOrExit(error = otMessageSetLength(message, mLength));
    SuccessOrExit(error = otIcmp6SendEchoRequest(mInstance, message, messageInfo, 1));

    mCount--;

exit:

    if (error != OT_ERROR_NONE && message != NULL)
    {
        otMessageFree(message);
    }

    if (mCount)
    {
        mPingTimer.Start(mInterval);
    }
}
#endif

void Interpreter::ProcessPollPeriod(int argc, char *argv[])
{
    otError error = OT_ERROR_NONE;
    long value;

    if (argc == 0)
    {
        mServer->OutputFormat("%d\r\n", (otLinkGetPollPeriod(mInstance) / 1000));  // ms->s
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
    otError error = OT_ERROR_NONE;

    if (argc == 0)
    {
        if (otLinkIsPromiscuous(mInstance) && otPlatRadioGetPromiscuous(mInstance))
        {
            mServer->OutputFormat("Enabled\r\n");
        }
        else
        {
            mServer->OutputFormat("Disabled\r\n");
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

void Interpreter::s_HandleLinkPcapReceive(const otRadioFrame *aFrame, void *aContext)
{
    static_cast<Interpreter *>(aContext)->HandleLinkPcapReceive(aFrame);
}

void Interpreter::HandleLinkPcapReceive(const otRadioFrame *aFrame)
{
    mServer->OutputFormat("\r\n");

    for (size_t i = 0; i < 44; i++)
    {
        mServer->OutputFormat("=");
    }

    mServer->OutputFormat("[len = %3u]", aFrame->mLength);

    for (size_t i = 0; i < 28; i++)
    {
        mServer->OutputFormat("=");
    }

    mServer->OutputFormat("\r\n");

    for (size_t i = 0; i < aFrame->mLength; i += 16)
    {
        mServer->OutputFormat("|");

        for (size_t j = 0; j < 16; j++)
        {
            if (i + j < aFrame->mLength)
            {
                mServer->OutputFormat(" %02X", aFrame->mPsdu[i + j]);
            }
            else
            {
                mServer->OutputFormat(" ..");
            }
        }

        mServer->OutputFormat("|");

        for (size_t j = 0; j < 16; j++)
        {
            if (i + j < aFrame->mLength)
            {
                if (31 < aFrame->mPsdu[i + j] && aFrame->mPsdu[i + j] < 127)
                {
                    mServer->OutputFormat(" %c", aFrame->mPsdu[i + j]);
                }
                else
                {
                    mServer->OutputFormat(" ?");
                }
            }
            else
            {
                mServer->OutputFormat(" .");
            }
        }

        mServer->OutputFormat("|\r\n");
    }

    for (size_t i = 0; i < 83; i++)
    {
        mServer->OutputFormat("-");
    }

    mServer->OutputFormat("\r\n");
}
#endif

#if OPENTHREAD_ENABLE_BORDER_ROUTER
otError Interpreter::ProcessPrefixAdd(int argc, char *argv[])
{
    otError error = OT_ERROR_NONE;
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
        ExitNow(error = OT_ERROR_PARSE);
    }

    argcur++;

    for (; argcur < argc; argcur++)
    {
        if (strcmp(argv[argcur], "high") == 0)
        {
            config.mPreference = OT_ROUTE_PREFERENCE_HIGH;
        }
        else if (strcmp(argv[argcur], "med") == 0)
        {
            config.mPreference = OT_ROUTE_PREFERENCE_MED;
        }
        else if (strcmp(argv[argcur], "low") == 0)
        {
            config.mPreference = OT_ROUTE_PREFERENCE_LOW;
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
                    ExitNow(error = OT_ERROR_PARSE);
                }
            }
        }
    }

    error = otBorderRouterAddOnMeshPrefix(mInstance, &config);

exit:
    return error;
}

otError Interpreter::ProcessPrefixRemove(int argc, char *argv[])
{
    otError error = OT_ERROR_NONE;
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
        ExitNow(error = OT_ERROR_PARSE);
    }

    error = otBorderRouterRemoveOnMeshPrefix(mInstance, &prefix);

exit:
    (void)argc;
    return error;
}

otError Interpreter::ProcessPrefixList(void)
{
    otNetworkDataIterator iterator = OT_NETWORK_DATA_ITERATOR_INIT;
    otBorderRouterConfig config;

    while (otBorderRouterGetNextOnMeshPrefix(mInstance, &iterator, &config) == OT_ERROR_NONE)
    {
        mServer->OutputFormat("%x:%x:%x:%x::/%d ",
                              HostSwap16(config.mPrefix.mPrefix.mFields.m16[0]),
                              HostSwap16(config.mPrefix.mPrefix.mFields.m16[1]),
                              HostSwap16(config.mPrefix.mPrefix.mFields.m16[2]),
                              HostSwap16(config.mPrefix.mPrefix.mFields.m16[3]),
                              config.mPrefix.mLength);

        if (config.mPreferred)
        {
            mServer->OutputFormat("p");
        }

        if (config.mSlaac)
        {
            mServer->OutputFormat("a");
        }

        if (config.mDhcp)
        {
            mServer->OutputFormat("d");
        }

        if (config.mConfigure)
        {
            mServer->OutputFormat("c");
        }

        if (config.mDefaultRoute)
        {
            mServer->OutputFormat("r");
        }

        if (config.mOnMesh)
        {
            mServer->OutputFormat("o");
        }

        if (config.mStable)
        {
            mServer->OutputFormat("s");
        }

        switch (config.mPreference)
        {
        case OT_ROUTE_PREFERENCE_LOW:
            mServer->OutputFormat(" low\r\n");
            break;

        case OT_ROUTE_PREFERENCE_MED:
            mServer->OutputFormat(" med\r\n");
            break;

        case OT_ROUTE_PREFERENCE_HIGH:
            mServer->OutputFormat(" high\r\n");
            break;
        }
    }

    return OT_ERROR_NONE;
}

void Interpreter::ProcessPrefix(int argc, char *argv[])
{
    otError error = OT_ERROR_NONE;

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
        ExitNow(error = OT_ERROR_PARSE);
    }

exit:
    AppendResult(error);
}
#endif  // OPENTHREAD_ENABLE_BORDER_ROUTER

#if OPENTHREAD_FTD
void Interpreter::ProcessReleaseRouterId(int argc, char *argv[])
{
    otError error = OT_ERROR_NONE;
    long value;

    VerifyOrExit(argc > 0, error = OT_ERROR_PARSE);

    SuccessOrExit(error = ParseLong(argv[0], value));
    SuccessOrExit(error = otThreadReleaseRouterId(mInstance, static_cast<uint8_t>(value)));

exit:
    AppendResult(error);
}
#endif  // OPENTHREAD_FTD

void Interpreter::ProcessReset(int argc, char *argv[])
{
    otInstanceReset(mInstance);
    (void)argc;
    (void)argv;
}

void Interpreter::ProcessRloc16(int argc, char *argv[])
{
    mServer->OutputFormat("%04x\r\n", otThreadGetRloc16(mInstance));
    mServer->OutputFormat("Done\r\n");
    (void)argc;
    (void)argv;
}

#if OPENTHREAD_ENABLE_BORDER_ROUTER
otError Interpreter::ProcessRouteAdd(int argc, char *argv[])
{
    otError error = OT_ERROR_NONE;
    otExternalRouteConfig config;
    int argcur = 0;

    memset(&config, 0, sizeof(otExternalRouteConfig));

    char *prefixLengthStr;
    char *endptr;

    VerifyOrExit(argc > 0, error = OT_ERROR_PARSE);

    if ((prefixLengthStr = strchr(argv[argcur], '/')) == NULL)
    {
        ExitNow();
    }

    *prefixLengthStr++ = '\0';

    SuccessOrExit(error = otIp6AddressFromString(argv[argcur], &config.mPrefix.mPrefix));

    config.mPrefix.mLength = static_cast<uint8_t>(strtol(prefixLengthStr, &endptr, 0));

    if (*endptr != '\0')
    {
        ExitNow(error = OT_ERROR_PARSE);
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
            config.mPreference = OT_ROUTE_PREFERENCE_HIGH;
        }
        else if (strcmp(argv[argcur], "med") == 0)
        {
            config.mPreference = OT_ROUTE_PREFERENCE_MED;
        }
        else if (strcmp(argv[argcur], "low") == 0)
        {
            config.mPreference = OT_ROUTE_PREFERENCE_LOW;
        }
        else
        {
            ExitNow(error = OT_ERROR_PARSE);
        }
    }

    error = otBorderRouterAddRoute(mInstance, &config);

exit:
    return error;
}

otError Interpreter::ProcessRouteRemove(int argc, char *argv[])
{
    otError error = OT_ERROR_NONE;
    struct otIp6Prefix prefix;
    int argcur = 0;

    memset(&prefix, 0, sizeof(struct otIp6Prefix));
    char *prefixLengthStr;
    char *endptr;

    VerifyOrExit(argc > 0, error = OT_ERROR_PARSE);

    if ((prefixLengthStr = strchr(argv[argcur], '/')) == NULL)
    {
        ExitNow();
    }

    *prefixLengthStr++ = '\0';

    SuccessOrExit(error = otIp6AddressFromString(argv[argcur], &prefix.mPrefix));

    prefix.mLength = static_cast<uint8_t>(strtol(prefixLengthStr, &endptr, 0));

    if (*endptr != '\0')
    {
        ExitNow(error = OT_ERROR_PARSE);
    }

    error = otBorderRouterRemoveRoute(mInstance, &prefix);

exit:
    return error;
}

void Interpreter::ProcessRoute(int argc, char *argv[])
{
    otError error = OT_ERROR_NONE;

    VerifyOrExit(argc > 0, error = OT_ERROR_PARSE);

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
        ExitNow(error = OT_ERROR_PARSE);
    }

exit:
    AppendResult(error);
}
#endif  // OPENTHREAD_ENABLE_BORDER_ROUTER

#if OPENTHREAD_FTD
void Interpreter::ProcessRouter(int argc, char *argv[])
{
    otError error = OT_ERROR_NONE;
    otRouterInfo routerInfo;
    long value;
    bool isTable = false;

    VerifyOrExit(argc > 0, error = OT_ERROR_PARSE);

    if (strcmp(argv[0], "list") == 0 || (isTable = (strcmp(argv[0], "table") == 0)))
    {
        if (isTable)
        {
            mServer->OutputFormat("| ID | RLOC16 | Next Hop | Path Cost | LQ In | LQ Out | Age | Extended MAC     |\r\n");
            mServer->OutputFormat("+----+--------+----------+-----------+-------+--------+-----+------------------+\r\n");
        }

        for (uint8_t i = 0; ; i++)
        {
            if (otThreadGetRouterInfo(mInstance, i, &routerInfo) != OT_ERROR_NONE)
            {
                mServer->OutputFormat("\r\n");
                ExitNow();
            }

            if (routerInfo.mAllocated)
            {
                if (isTable)
                {
                    mServer->OutputFormat("| %2d ", routerInfo.mRouterId);
                    mServer->OutputFormat("| 0x%04x ", routerInfo.mRloc16);
                    mServer->OutputFormat("| %8d ", routerInfo.mNextHop);
                    mServer->OutputFormat("| %9d ", routerInfo.mPathCost);
                    mServer->OutputFormat("| %5d ", routerInfo.mLinkQualityIn);
                    mServer->OutputFormat("| %6d ", routerInfo.mLinkQualityOut);
                    mServer->OutputFormat("| %3d ", routerInfo.mAge);
                    mServer->OutputFormat("| ");

                    for (size_t j = 0; j < sizeof(routerInfo.mExtAddress); j++)
                    {
                        mServer->OutputFormat("%02x", routerInfo.mExtAddress.m8[j]);
                    }

                    mServer->OutputFormat(" |\r\n");
                }
                else
                {
                    mServer->OutputFormat("%d ", i);
                }
            }
        }
    }

    SuccessOrExit(error = ParseLong(argv[0], value));
    SuccessOrExit(error = otThreadGetRouterInfo(mInstance, static_cast<uint16_t>(value), &routerInfo));

    mServer->OutputFormat("Alloc: %d\r\n", routerInfo.mAllocated);

    if (routerInfo.mAllocated)
    {
        mServer->OutputFormat("Router ID: %d\r\n", routerInfo.mRouterId);
        mServer->OutputFormat("Rloc: %04x\r\n", routerInfo.mRloc16);
        mServer->OutputFormat("Next Hop: %04x\r\n", static_cast<uint16_t>(routerInfo.mNextHop) << 10);
        mServer->OutputFormat("Link: %d\r\n", routerInfo.mLinkEstablished);

        if (routerInfo.mLinkEstablished)
        {
            mServer->OutputFormat("Ext Addr: ");

            for (size_t j = 0; j < sizeof(routerInfo.mExtAddress); j++)
            {
                mServer->OutputFormat("%02x", routerInfo.mExtAddress.m8[j]);
            }

            mServer->OutputFormat("\r\n");
            mServer->OutputFormat("Cost: %d\r\n", routerInfo.mPathCost);
            mServer->OutputFormat("Link Quality In: %d\r\n", routerInfo.mLinkQualityIn);
            mServer->OutputFormat("Link Quality Out: %d\r\n", routerInfo.mLinkQualityOut);
            mServer->OutputFormat("Age: %d\r\n", routerInfo.mAge);
        }
    }

exit:
    AppendResult(error);
}

void Interpreter::ProcessRouterDowngradeThreshold(int argc, char *argv[])
{
    otError error = OT_ERROR_NONE;
    long value;

    if (argc == 0)
    {
        mServer->OutputFormat("%d\r\n", otThreadGetRouterDowngradeThreshold(mInstance));
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
    otError error = OT_ERROR_NONE;

    if (argc == 0)
    {
        if (otThreadIsRouterRoleEnabled(mInstance))
        {
            mServer->OutputFormat("Enabled\r\n");
        }
        else
        {
            mServer->OutputFormat("Disabled\r\n");
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
        ExitNow(error = OT_ERROR_PARSE);
    }

exit:
    AppendResult(error);
}

void Interpreter::ProcessRouterSelectionJitter(int argc, char *argv[])
{
    otError error = OT_ERROR_NONE;
    long value;

    if (argc == 0)
    {
        mServer->OutputFormat("%d\r\n", otThreadGetRouterSelectionJitter(mInstance));
    }
    else
    {
        SuccessOrExit(error = ParseLong(argv[0], value));
        VerifyOrExit(0 < value && value < 256, error = OT_ERROR_INVALID_ARGS);
        otThreadSetRouterSelectionJitter(mInstance, static_cast<uint8_t>(value));
    }

exit:
    AppendResult(error);
}

void Interpreter::ProcessRouterUpgradeThreshold(int argc, char *argv[])
{
    otError error = OT_ERROR_NONE;
    long value;

    if (argc == 0)
    {
        mServer->OutputFormat("%d\r\n", otThreadGetRouterUpgradeThreshold(mInstance));
    }
    else
    {
        SuccessOrExit(error = ParseLong(argv[0], value));
        otThreadSetRouterUpgradeThreshold(mInstance, static_cast<uint8_t>(value));
    }

exit:
    AppendResult(error);
}
#endif  // OPENTHREAD_FTD

void Interpreter::ProcessScan(int argc, char *argv[])
{
    otError error = OT_ERROR_NONE;
    uint32_t scanChannels = 0;
    long value;

    if (argc > 0)
    {
        SuccessOrExit(error = ParseLong(argv[0], value));
        scanChannels = 1 << value;
    }

    mServer->OutputFormat("| J | Network Name     | Extended PAN     | PAN  | MAC Address      | Ch | dBm | LQI |\r\n");
    mServer->OutputFormat("+---+------------------+------------------+------+------------------+----+-----+-----+\r\n");
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
        mServer->OutputFormat("Done\r\n");
        ExitNow();
    }

    mServer->OutputFormat("| %d ", aResult->mIsJoinable);

    mServer->OutputFormat("| %-16s ", aResult->mNetworkName.m8);

    mServer->OutputFormat("| ");
    OutputBytes(aResult->mExtendedPanId.m8, OT_EXT_PAN_ID_SIZE);
    mServer->OutputFormat(" ");

    mServer->OutputFormat("| %04x | ", aResult->mPanId);
    OutputBytes(aResult->mExtAddress.m8, OT_EXT_ADDRESS_SIZE);
    mServer->OutputFormat(" | %2d ", aResult->mChannel);
    mServer->OutputFormat("| %3d ", aResult->mRssi);
    mServer->OutputFormat("| %3d |\r\n", aResult->mLqi);

exit:
    return;
}

void Interpreter::ProcessSingleton(int argc, char *argv[])
{
    otError error = OT_ERROR_NONE;

    if (otThreadIsSingleton(mInstance))
    {
        mServer->OutputFormat("true\r\n");
    }
    else
    {
        mServer->OutputFormat("false\r\n");
    }

    (void)argc;
    (void)argv;

    AppendResult(error);
}

void Interpreter::ProcessState(int argc, char *argv[])
{
    otError error = OT_ERROR_NONE;

    if (argc == 0)
    {
        switch (otThreadGetDeviceRole(mInstance))
        {
        case OT_DEVICE_ROLE_DISABLED:
            mServer->OutputFormat("disabled\r\n");
            break;

        case OT_DEVICE_ROLE_DETACHED:
            mServer->OutputFormat("detached\r\n");
            break;

        case OT_DEVICE_ROLE_CHILD:
            mServer->OutputFormat("child\r\n");
            break;

#if OPENTHREAD_FTD

        case OT_DEVICE_ROLE_ROUTER:
            mServer->OutputFormat("router\r\n");
            break;

        case OT_DEVICE_ROLE_LEADER:
            mServer->OutputFormat("leader\r\n");
            break;
#endif  // OPENTHREAD_FTD

        default:
            mServer->OutputFormat("invalid state\r\n");
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
            SuccessOrExit(error = otThreadBecomeChild(mInstance));
        }

#if OPENTHREAD_FTD
        else if (strcmp(argv[0], "router") == 0)
        {
            SuccessOrExit(error = otThreadBecomeRouter(mInstance));
        }
        else if (strcmp(argv[0], "leader") == 0)
        {
            SuccessOrExit(error = otThreadBecomeLeader(mInstance));
        }

#endif  // OPENTHREAD_FTD
        else
        {
            ExitNow(error = OT_ERROR_PARSE);
        }
    }

exit:
    AppendResult(error);
}

void Interpreter::ProcessThread(int argc, char *argv[])
{
    otError error = OT_ERROR_PARSE;

    VerifyOrExit(argc > 0, error = OT_ERROR_PARSE);

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

void Interpreter::ProcessTxPowerMax(int argc, char *argv[])
{
    otError error = OT_ERROR_NONE;
    long value;

    if (argc == 0)
    {
        mServer->OutputFormat("%d dBm\r\n", otLinkGetMaxTransmitPower(mInstance));
    }
    else
    {
        SuccessOrExit(error = ParseLong(argv[0], value));
        otLinkSetMaxTransmitPower(mInstance, static_cast<int8_t>(value));
    }

exit:
    AppendResult(error);
}

void Interpreter::ProcessVersion(int argc, char *argv[])
{
    otStringPtr version(otGetVersionString());
    mServer->OutputFormat("%s\r\n", (const char *)version);
    AppendResult(OT_ERROR_NONE);
    (void)argc;
    (void)argv;
}

#if OPENTHREAD_ENABLE_COMMISSIONER && OPENTHREAD_FTD

void Interpreter::ProcessCommissioner(int argc, char *argv[])
{
    otError error = OT_ERROR_NONE;

    VerifyOrExit(argc > 0, error = OT_ERROR_PARSE);

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

        VerifyOrExit(argc > 2, error = OT_ERROR_PARSE);

        if (strcmp(argv[2], "*") == 0)
        {
            addrPtr = NULL;
        }
        else
        {
            VerifyOrExit(Hex2Bin(argv[2], addr.m8, sizeof(addr)) == sizeof(addr), error = OT_ERROR_PARSE);
            addrPtr = &addr;
        }

        if (strcmp(argv[1], "add") == 0)
        {
            VerifyOrExit(argc > 3, error = OT_ERROR_PARSE);
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

        VerifyOrExit(argc > 4, error = OT_ERROR_PARSE);

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

        VerifyOrExit(argc > 5, error = OT_ERROR_PARSE);

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

        VerifyOrExit(argc > 3, error = OT_ERROR_PARSE);

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
            VerifyOrExit(static_cast<size_t>(length) < sizeof(tlvs), error = OT_ERROR_NO_BUFS);

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
                VerifyOrExit(++index < argc, error = OT_ERROR_PARSE);
                value = static_cast<long>(strlen(argv[index]) + 1) / 2;
                VerifyOrExit(static_cast<size_t>(value) <= (sizeof(tlvs) - static_cast<size_t>(length)), error = OT_ERROR_NO_BUFS);
                VerifyOrExit(Interpreter::Hex2Bin(argv[index], tlvs + length, static_cast<uint16_t>(value)) >= 0,
                             error = OT_ERROR_PARSE);
                length += value;
            }
            else
            {
                ExitNow(error = OT_ERROR_PARSE);
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

        VerifyOrExit(argc > 0, error = OT_ERROR_PARSE);

        memset(&dataset, 0, sizeof(dataset));

        for (uint8_t index = 1; index < argc; index++)
        {
            VerifyOrExit(static_cast<size_t>(length) < sizeof(tlvs), error = OT_ERROR_NO_BUFS);

            if (strcmp(argv[index], "locator") == 0)
            {
                VerifyOrExit(++index < argc, error = OT_ERROR_PARSE);
                dataset.mIsLocatorSet = true;
                SuccessOrExit(error = Interpreter::ParseLong(argv[index], value));
                dataset.mLocator = static_cast<uint16_t>(value);
            }
            else if (strcmp(argv[index], "sessionid") == 0)
            {
                VerifyOrExit(++index < argc, error = OT_ERROR_PARSE);
                dataset.mIsSessionIdSet = true;
                SuccessOrExit(error = Interpreter::ParseLong(argv[index], value));
                dataset.mSessionId = static_cast<uint16_t>(value);
            }
            else if (strcmp(argv[index], "steeringdata") == 0)
            {
                VerifyOrExit(++index < argc, error = OT_ERROR_PARSE);
                dataset.mIsSteeringDataSet = true;
                length = static_cast<int>((strlen(argv[index]) + 1) / 2);
                VerifyOrExit(static_cast<size_t>(length) <= OT_STEERING_DATA_MAX_LENGTH, error = OT_ERROR_NO_BUFS);
                VerifyOrExit(Interpreter::Hex2Bin(argv[index], dataset.mSteeringData.m8, static_cast<uint16_t>(length)) >= 0,
                             error = OT_ERROR_PARSE);
                dataset.mSteeringData.mLength = static_cast<uint8_t>(length);
                length = 0;
            }
            else if (strcmp(argv[index], "joinerudpport") == 0)
            {
                VerifyOrExit(++index < argc, error = OT_ERROR_PARSE);
                dataset.mIsJoinerUdpPortSet = true;
                SuccessOrExit(error = Interpreter::ParseLong(argv[index], value));
                dataset.mJoinerUdpPort = static_cast<uint16_t>(value);
            }
            else if (strcmp(argv[index], "binary") == 0)
            {
                VerifyOrExit(++index < argc, error = OT_ERROR_PARSE);
                length = static_cast<int>((strlen(argv[index]) + 1) / 2);
                VerifyOrExit(static_cast<size_t>(length) <= sizeof(tlvs), error = OT_ERROR_NO_BUFS);
                VerifyOrExit(Interpreter::Hex2Bin(argv[index], tlvs, static_cast<uint16_t>(length)) >= 0,
                             error = OT_ERROR_PARSE);
            }
            else
            {
                ExitNow(error = OT_ERROR_PARSE);
            }
        }

        SuccessOrExit(error = otCommissionerSendMgmtSet(mInstance, &dataset, tlvs, static_cast<uint8_t>(length)));
    }
    else if (strcmp(argv[0], "sessionid") == 0)
    {
        mServer->OutputFormat("%d\r\n", otCommissionerGetSessionId(mInstance));
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
    mServer->OutputFormat("Energy: %08x ", aChannelMask);

    for (uint8_t i = 0; i < aEnergyListLength; i++)
    {
        mServer->OutputFormat("%d ", aEnergyList[i]);
    }

    mServer->OutputFormat("\r\n");
}

void OTCALL Interpreter::s_HandlePanIdConflict(uint16_t aPanId, uint32_t aChannelMask, void *aContext)
{
    static_cast<Interpreter *>(aContext)->HandlePanIdConflict(aPanId, aChannelMask);
}

void Interpreter::HandlePanIdConflict(uint16_t aPanId, uint32_t aChannelMask)
{
    mServer->OutputFormat("Conflict: %04x, %08x\r\n", aPanId, aChannelMask);
}

#endif //  OPENTHREAD_ENABLE_COMMISSIONER && OPENTHREAD_FTD

#if OPENTHREAD_ENABLE_JOINER

void Interpreter::ProcessJoiner(int argc, char *argv[])
{
    otError error = OT_ERROR_NONE;

    VerifyOrExit(argc > 0, error = OT_ERROR_PARSE);

    if (strcmp(argv[0], "start") == 0)
    {
        const char *provisioningUrl;
        VerifyOrExit(argc > 1, error = OT_ERROR_PARSE);
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

void OTCALL Interpreter::s_HandleJoinerCallback(otError aError, void *aContext)
{
    static_cast<Interpreter *>(aContext)->HandleJoinerCallback(aError);
}

void Interpreter::HandleJoinerCallback(otError aError)
{
    switch (aError)
    {
    case OT_ERROR_NONE:
        mServer->OutputFormat("Join success\r\n");
        break;

    default:
        mServer->OutputFormat("Join failed [%s]\r\n", otThreadErrorToString(aError));
        break;
    }
}

#if OPENTHREAD_FTD
void Interpreter::ProcessJoinerPort(int argc, char *argv[])
{
    otError error = OT_ERROR_NONE;
    long value;

    if (argc == 0)
    {
        mServer->OutputFormat("%d\r\n", otThreadGetJoinerUdpPort(mInstance));
    }
    else
    {
        SuccessOrExit(error = ParseLong(argv[0], value));
        error = otThreadSetJoinerUdpPort(mInstance, static_cast<uint16_t>(value));
    }

exit:
    AppendResult(error);
}
#endif

void Interpreter::ProcessWhitelist(int argc, char *argv[])
{
    otError error = OT_ERROR_NONE;
    otMacWhitelistEntry entry;
    int argcur = 0;
    uint8_t extAddr[8];
    int8_t rssi;

    if (argcur >= argc)
    {
        if (otLinkIsWhitelistEnabled(mInstance))
        {
            mServer->OutputFormat("Enabled\r\n");
        }
        else
        {
            mServer->OutputFormat("Disabled\r\n");
        }

        for (uint8_t i = 0; ; i++)
        {
            if (otLinkGetWhitelistEntry(mInstance, i, &entry) != OT_ERROR_NONE)
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
                mServer->OutputFormat(" %d", entry.mRssi);
            }

            mServer->OutputFormat("\r\n");
        }
    }
    else if (strcmp(argv[argcur], "add") == 0)
    {
        VerifyOrExit(++argcur < argc, error = OT_ERROR_PARSE);
        VerifyOrExit(Hex2Bin(argv[argcur], extAddr, sizeof(extAddr)) == sizeof(extAddr), error = OT_ERROR_PARSE);

        if (++argcur < argc)
        {
            rssi = static_cast<int8_t>(strtol(argv[argcur], NULL, 0));
            VerifyOrExit(otLinkAddWhitelistRssi(mInstance, extAddr, rssi) == OT_ERROR_NONE, error = OT_ERROR_PARSE);
        }
        else
        {
            otLinkAddWhitelist(mInstance, extAddr);
            VerifyOrExit(otLinkAddWhitelist(mInstance, extAddr) == OT_ERROR_NONE, error = OT_ERROR_PARSE);
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
        VerifyOrExit(++argcur < argc, error = OT_ERROR_PARSE);
        VerifyOrExit(Hex2Bin(argv[argcur], extAddr, sizeof(extAddr)) == sizeof(extAddr), error = OT_ERROR_PARSE);
        otLinkRemoveWhitelist(mInstance, extAddr);
    }

exit:
    AppendResult(error);
}

#if OPENTHREAD_ENABLE_DIAG
void Interpreter::ProcessDiag(int argc, char *argv[])
{
    // all diagnostics related features are processed within diagnostics module
    mServer->OutputFormat("%s\r\n", otDiagProcessCmd(argc, argv));
}
#endif

void Interpreter::ProcessLine(char *aBuf, uint16_t aBufLength, Server &aServer)
{
    char *argv[kMaxArgs];
    char *cmd;
    uint8_t argc = 0, i = 0;

    mServer = &aServer;

    VerifyOrExit(aBuf != NULL);

    for (; *aBuf == ' '; aBuf++, aBufLength--);

    for (cmd = aBuf + 1; (cmd < aBuf + aBufLength) && (cmd != NULL); ++cmd)
    {
        VerifyOrExit(argc < kMaxArgs,
                     mServer->OutputFormat("Error: too many args (max %d)\r\n", kMaxArgs));

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
                 mServer->OutputFormat("under diagnostics mode, execute 'diag stop' before running any other commands.\r\n"));
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
        AppendResult(OT_ERROR_PARSE);
    }

exit:
    return;
}

void OTCALL Interpreter::s_HandleNetifStateChanged(uint32_t aFlags, void *aContext)
{
#ifdef OTDLL
    otCliContext *cliContext = static_cast<otCliContext *>(aContext);
    cliContext->mInterpreter->HandleNetifStateChanged(cliContext->mInstance, aFlags);
#else
    static_cast<Interpreter *>(aContext)->HandleNetifStateChanged(aFlags);
#endif
}

#ifdef OTDLL
void Interpreter::HandleNetifStateChanged(otInstance *mInstance, uint32_t aFlags)
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

#if OPENTHREAD_FTD || OPENTHREAD_ENABLE_MTD_NETWORK_DIAGNOSTIC
void Interpreter::ProcessNetworkDiagnostic(int argc, char *argv[])
{
    otError error = OT_ERROR_NONE;
    struct otIp6Address address;
    uint8_t payload[2 + OT_NETWORK_DIAGNOSTIC_TYPELIST_MAX_ENTRIES];  // TypeList Type(1B), len(1B), type list
    uint8_t payloadIndex = 0;
    uint8_t paramIndex = 0;

    VerifyOrExit(argc > 1 + 1, error = OT_ERROR_PARSE);

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
#endif // OPENTHREAD_FTD || OPENTHREAD_ENABLE_MTD_NETWORK_DIAGNOSTIC

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

    mServer->OutputFormat("DIAG_GET.rsp: ");

    while (length > 0)
    {
        bytesToPrint = (length < sizeof(buf)) ? length : sizeof(buf);
        aMessage.Read(aMessage.GetOffset() + bytesPrinted, bytesToPrint, buf);

        OutputBytes(buf, static_cast<uint8_t>(bytesToPrint));

        length       -= bytesToPrint;
        bytesPrinted += bytesToPrint;
    }

    mServer->OutputFormat("\r\n");
}
#endif

}  // namespace Cli
}  // namespace ot
