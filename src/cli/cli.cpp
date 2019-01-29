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

#include "cli.hpp"

#ifdef OTDLL
#include <assert.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include "mac/channel_mask.hpp"
#include "utils/parse_cmdline.hpp"
#include "utils/wrap_string.h"

#include <openthread/commissioner.h>
#include <openthread/icmp6.h>
#include <openthread/joiner.h>
#include <openthread/link.h>
#if OPENTHREAD_CONFIG_ENABLE_TIME_SYNC
#include <openthread/network_time.h>
#endif

#if OPENTHREAD_FTD
#include <openthread/dataset_ftd.h>
#include <openthread/thread_ftd.h>
#endif

#if OPENTHREAD_ENABLE_BORDER_ROUTER
#include <openthread/border_router.h>
#endif
#if OPENTHREAD_ENABLE_SERVICE
#include <openthread/server.h>
#endif

#ifndef OTDLL
#include <openthread/diag.h>
#include <openthread/icmp6.h>
#include <openthread/platform/uart.h>

#include "common/new.hpp"
#include "net/ip6.hpp"
#endif

#include "cli_dataset.hpp"

#if OPENTHREAD_ENABLE_APPLICATION_COAP
#include "cli_coap.hpp"
#endif

#if OPENTHREAD_ENABLE_APPLICATION_COAP_SECURE
#include "cli_coap_secure.hpp"
#endif

#if OPENTHREAD_ENABLE_CHANNEL_MANAGER && OPENTHREAD_FTD
#include <openthread/channel_manager.h>
#endif

#if OPENTHREAD_ENABLE_CHANNEL_MONITOR
#include <openthread/channel_monitor.h>
#endif

#if (OPENTHREAD_CONFIG_LOG_OUTPUT == OPENTHREAD_CONFIG_LOG_OUTPUT_DEBUG_UART) && OPENTHREAD_POSIX
#include <openthread/platform/debug_uart.h>
#endif

#include "cli_server.hpp"
#include "common/encoding.hpp"

using ot::Encoding::BigEndian::HostSwap16;
using ot::Encoding::BigEndian::HostSwap32;

namespace ot {

namespace Cli {

const struct Command Interpreter::sCommands[] = {
    {"help", &Interpreter::ProcessHelp},
    {"autostart", &Interpreter::ProcessAutoStart},
    {"bufferinfo", &Interpreter::ProcessBufferInfo},
    {"channel", &Interpreter::ProcessChannel},
#if OPENTHREAD_FTD
    {"child", &Interpreter::ProcessChild},
    {"childmax", &Interpreter::ProcessChildMax},
#endif
    {"childtimeout", &Interpreter::ProcessChildTimeout},
#if OPENTHREAD_ENABLE_APPLICATION_COAP
    {"coap", &Interpreter::ProcessCoap},
#endif
#if OPENTHREAD_ENABLE_APPLICATION_COAP_SECURE
    {"coaps", &Interpreter::ProcessCoapSecure},
#endif
#if OPENTHREAD_ENABLE_COMMISSIONER && OPENTHREAD_FTD
    {"commissioner", &Interpreter::ProcessCommissioner},
#endif
#if OPENTHREAD_FTD
    {"contextreusedelay", &Interpreter::ProcessContextIdReuseDelay},
#endif
    {"counter", &Interpreter::ProcessCounters},
    {"dataset", &Interpreter::ProcessDataset},
#if OPENTHREAD_FTD
    {"delaytimermin", &Interpreter::ProcessDelayTimerMin},
#endif
#if OPENTHREAD_ENABLE_DIAG
    {"diag", &Interpreter::ProcessDiag},
#endif
    {"discover", &Interpreter::ProcessDiscover},
#if OPENTHREAD_ENABLE_DNS_CLIENT
    {"dns", &Interpreter::ProcessDns},
#endif
#if OPENTHREAD_FTD
    {"eidcache", &Interpreter::ProcessEidCache},
#endif
    {"eui64", &Interpreter::ProcessEui64},
#if OPENTHREAD_POSIX
    {"exit", &Interpreter::ProcessExit},
#endif
#if (OPENTHREAD_CONFIG_LOG_OUTPUT == OPENTHREAD_CONFIG_LOG_OUTPUT_DEBUG_UART) && OPENTHREAD_POSIX
    {"logfilename", &Interpreter::ProcessLogFilename},
#endif
    {"extaddr", &Interpreter::ProcessExtAddress},
    {"extpanid", &Interpreter::ProcessExtPanId},
    {"factoryreset", &Interpreter::ProcessFactoryReset},
    {"ifconfig", &Interpreter::ProcessIfconfig},
#ifdef OTDLL
    {"instance", &Interpreter::ProcessInstance},
    {"instancelist", &Interpreter::ProcessInstanceList},
#endif
    {"ipaddr", &Interpreter::ProcessIpAddr},
#ifndef OTDLL
    {"ipmaddr", &Interpreter::ProcessIpMulticastAddr},
#endif
#if OPENTHREAD_ENABLE_JOINER
    {"joiner", &Interpreter::ProcessJoiner},
    {"joinerid", &Interpreter::ProcessJoinerId},
#endif
#if OPENTHREAD_FTD
    {"joinerport", &Interpreter::ProcessJoinerPort},
#endif
    {"keysequence", &Interpreter::ProcessKeySequence},
    {"leaderdata", &Interpreter::ProcessLeaderData},
#if OPENTHREAD_FTD
    {"leaderpartitionid", &Interpreter::ProcessLeaderPartitionId},
    {"leaderweight", &Interpreter::ProcessLeaderWeight},
#endif
#if OPENTHREAD_ENABLE_MAC_FILTER
    {"macfilter", &Interpreter::ProcessMacFilter},
#endif
    {"masterkey", &Interpreter::ProcessMasterKey},
    {"mode", &Interpreter::ProcessMode},
#if OPENTHREAD_FTD
    {"neighbor", &Interpreter::ProcessNeighbor},
#endif
#if OPENTHREAD_ENABLE_BORDER_ROUTER || OPENTHREAD_ENABLE_SERVICE
    {"netdataregister", &Interpreter::ProcessNetworkDataRegister},
#endif
#if OPENTHREAD_ENABLE_SERVICE
    {"netdatashow", &Interpreter::ProcessNetworkDataShow},
#endif
#if OPENTHREAD_FTD || OPENTHREAD_ENABLE_MTD_NETWORK_DIAGNOSTIC
    {"networkdiagnostic", &Interpreter::ProcessNetworkDiagnostic},
#endif // OPENTHREAD_FTD || OPENTHREAD_ENABLE_MTD_NETWORK_DIAGNOSTIC
#if OPENTHREAD_FTD
    {"networkidtimeout", &Interpreter::ProcessNetworkIdTimeout},
#endif
    {"networkname", &Interpreter::ProcessNetworkName},
#if OPENTHREAD_CONFIG_ENABLE_TIME_SYNC
    {"networktime", &Interpreter::ProcessNetworkTime},
#endif
    {"panid", &Interpreter::ProcessPanId},
    {"parent", &Interpreter::ProcessParent},
#if OPENTHREAD_FTD
    {"parentpriority", &Interpreter::ProcessParentPriority},
#endif
#ifndef OTDLL
    {"ping", &Interpreter::ProcessPing},
#endif
    {"pollperiod", &Interpreter::ProcessPollPeriod},
#ifndef OTDLL
    {"promiscuous", &Interpreter::ProcessPromiscuous},
#endif
#if OPENTHREAD_ENABLE_BORDER_ROUTER
    {"prefix", &Interpreter::ProcessPrefix},
#endif
#if OPENTHREAD_FTD
    {"pskc", &Interpreter::ProcessPSKc},
    {"releaserouterid", &Interpreter::ProcessReleaseRouterId},
#endif
    {"reset", &Interpreter::ProcessReset},
    {"rloc16", &Interpreter::ProcessRloc16},
#if OPENTHREAD_ENABLE_BORDER_ROUTER
    {"route", &Interpreter::ProcessRoute},
#endif
#if OPENTHREAD_FTD
    {"router", &Interpreter::ProcessRouter},
    {"routerdowngradethreshold", &Interpreter::ProcessRouterDowngradeThreshold},
    {"routerrole", &Interpreter::ProcessRouterRole},
    {"routerselectionjitter", &Interpreter::ProcessRouterSelectionJitter},
    {"routerupgradethreshold", &Interpreter::ProcessRouterUpgradeThreshold},
#endif
    {"scan", &Interpreter::ProcessScan},
#if OPENTHREAD_ENABLE_SERVICE
    {"service", &Interpreter::ProcessService},
#endif
    {"singleton", &Interpreter::ProcessSingleton},
#if OPENTHREAD_ENABLE_SNTP_CLIENT
    {"sntp", &Interpreter::ProcessSntp},
#endif
    {"state", &Interpreter::ProcessState},
    {"thread", &Interpreter::ProcessThread},
#ifndef OTDLL
    {"txpower", &Interpreter::ProcessTxPower},
    {"udp", &Interpreter::ProcessUdp},
#endif
    {"version", &Interpreter::ProcessVersion},
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
    otPtr(T *_ptr)
        : ptr(_ptr)
    {
    }
    ~otPtr()
    {
        if (ptr)
        {
            otFreeMemory(ptr);
        }
    }
    T *get() const { return ptr; }
       operator T *() const { return ptr; }
    T *operator->() const { return ptr; }
};

typedef otPtr<const otMacCounters>  otMacCountersPtr;
typedef otPtr<const otNetifAddress> otNetifAddressPtr;
typedef otPtr<const uint8_t>        otBufferPtr;
typedef otPtr<const char>           otStringPtr;

Interpreter::Interpreter(Instance *aInstance)
    : mUserCommands(NULL)
    , mUserCommandsLength(0)
    , mServer(NULL)
#ifdef OTDLL
    , mApiInstance(otApiInit())
    , mInstanceIndex(0)
#else
    , mLength(8)
    , mCount(1)
    , mInterval(1000)
    , mPingTimer(*aInstance, &Interpreter::s_HandlePingTimer, this)
#if OPENTHREAD_ENABLE_DNS_CLIENT
    , mResolvingInProgress(0)
#endif
    , mUdp(*this)
#endif
    , mDataset(*this)
#if OPENTHREAD_ENABLE_APPLICATION_COAP
    , mCoap(*this)
#endif
#if OPENTHREAD_ENABLE_APPLICATION_COAP_SECURE
    , mCoapSecure(*this)
#endif
    , mInstance(aInstance)
{
#ifdef OTDLL
    // On Windows, mInstance represents the current selected otInstance
    // which should be NULL now.
    assert(aInstance = NULL);
    assert(mApiInstance);
    CacheInstances();
#else
#if OPENTHREAD_FTD || OPENTHREAD_ENABLE_MTD_NETWORK_DIAGNOSTIC
    otThreadSetReceiveDiagnosticGetCallback(mInstance, &Interpreter::s_HandleDiagnosticGetResponse, this);
#endif

    mIcmpHandler.mReceiveCallback = Interpreter::s_HandleIcmpReceive;
    mIcmpHandler.mContext         = this;
    otIcmp6RegisterHandler(mInstance, &mIcmpHandler);

#if OPENTHREAD_ENABLE_DNS_CLIENT
    memset(mResolvingHostname, 0, sizeof(mResolvingHostname));
#endif // OPENTHREAD_ENABLE_DNS_CLIENT

#endif
}

int Interpreter::Hex2Bin(const char *aHex, uint8_t *aBin, uint16_t aBinLength)
{
    size_t      hexLength = strlen(aHex);
    const char *hexEnd    = aHex + hexLength;
    uint8_t *   cur       = aBin;
    uint8_t     numChars  = hexLength & 1;
    uint8_t     byte      = 0;
    int         rval;

    VerifyOrExit((hexLength + 1) / 2 <= aBinLength, rval = -1);

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
            ExitNow(rval = -1);
        }

        aHex++;
        numChars++;

        if (numChars >= 2)
        {
            numChars = 0;
            *cur++   = byte;
            byte     = 0;
        }
        else
        {
            byte <<= 4;
        }
    }

    rval = static_cast<int>(cur - aBin);

exit:
    return rval;
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
    OT_UNUSED_VARIABLE(argc);
    OT_UNUSED_VARIABLE(argv);

    for (unsigned int i = 0; i < OT_ARRAY_LENGTH(sCommands); i++)
    {
        mServer->OutputFormat("%s\r\n", sCommands[i].mName);
    }

    for (unsigned int i = 0; i < mUserCommandsLength; i++)
    {
        mServer->OutputFormat("%s\r\n", mUserCommands[i].mName);
    }
}

void Interpreter::ProcessAutoStart(int argc, char *argv[])
{
    otError error = OT_ERROR_NONE;

    if (argc == 0)
    {
        if (otThreadGetAutoStart(mInstance))
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
        error = otThreadSetAutoStart(mInstance, true);
    }
    else if (strcmp(argv[0], "disable") == 0)
    {
        error = otThreadSetAutoStart(mInstance, false);
    }
    else
    {
        error = OT_ERROR_INVALID_ARGS;
    }

    AppendResult(error);
}

void Interpreter::ProcessBufferInfo(int argc, char *argv[])
{
    OT_UNUSED_VARIABLE(argc);
    OT_UNUSED_VARIABLE(argv);

    otBufferInfo bufferInfo;

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
    mServer->OutputFormat("application coap: %d %d\r\n", bufferInfo.mApplicationCoapMessages,
                          bufferInfo.mApplicationCoapBuffers);

    AppendResult(OT_ERROR_NONE);
}

void Interpreter::ProcessChannel(int argc, char *argv[])
{
    otError error = OT_ERROR_NONE;
    long    value;

    if (argc == 0)
    {
        mServer->OutputFormat("%d\r\n", otLinkGetChannel(mInstance));
    }
#if OPENTHREAD_ENABLE_CHANNEL_MONITOR
    else if (strcmp(argv[0], "monitor") == 0)
    {
        if (argc == 1)
        {
            mServer->OutputFormat("enabled: %d\r\n", otChannelMonitorIsEnabled(mInstance));
            if (otChannelMonitorIsEnabled(mInstance))
            {
                mServer->OutputFormat("interval: %lu\r\n", otChannelMonitorGetSampleInterval(mInstance));
                mServer->OutputFormat("threshold: %d\r\n", otChannelMonitorGetRssiThreshold(mInstance));
                mServer->OutputFormat("window: %lu\r\n", otChannelMonitorGetSampleWindow(mInstance));
                mServer->OutputFormat("count: %lu\r\n", otChannelMonitorGetSampleCount(mInstance));

                mServer->OutputFormat("occupancies:\r\n");
                for (uint8_t channel = OT_RADIO_CHANNEL_MIN; channel <= OT_RADIO_CHANNEL_MAX; channel++)
                {
                    uint32_t occupancy = otChannelMonitorGetChannelOccupancy(mInstance, channel);

                    mServer->OutputFormat("ch %d (0x%04x) ", channel, occupancy);
                    occupancy = (occupancy * 10000) / 0xffff;
                    mServer->OutputFormat("%2d.%02d%% busy\r\n", occupancy / 100, occupancy % 100);
                }
                mServer->OutputFormat("\r\n");
            }
        }
        else if (strcmp(argv[1], "start") == 0)
        {
            error = otChannelMonitorSetEnabled(mInstance, true);
        }
        else if (strcmp(argv[1], "stop") == 0)
        {
            error = otChannelMonitorSetEnabled(mInstance, false);
        }
        else
        {
            ExitNow(error = OT_ERROR_INVALID_ARGS);
        }
    }
#endif
#if OPENTHREAD_ENABLE_CHANNEL_MANAGER && OPENTHREAD_FTD
    else if (strcmp(argv[0], "manager") == 0)
    {
        if (argc == 1)
        {
            mServer->OutputFormat("channel: %d\r\n", otChannelManagerGetRequestedChannel(mInstance));
            mServer->OutputFormat("auto: %d\r\n", otChannelManagerGetAutoChannelSelectionEnabled(mInstance));

            if (otChannelManagerGetAutoChannelSelectionEnabled(mInstance))
            {
                Mac::ChannelMask supportedMask(otChannelManagerGetSupportedChannels(mInstance));
                Mac::ChannelMask favoredMask(otChannelManagerGetFavoredChannels(mInstance));

                mServer->OutputFormat("delay: %d\r\n", otChannelManagerGetDelay(mInstance));
                mServer->OutputFormat("interval: %lu\r\n", otChannelManagerGetAutoChannelSelectionInterval(mInstance));
                mServer->OutputFormat("supported: %s\r\n", supportedMask.ToString().AsCString());
                mServer->OutputFormat("favored: %s\r\n", supportedMask.ToString().AsCString());
            }
        }
        else if (strcmp(argv[1], "change") == 0)
        {
            VerifyOrExit(argc > 2, error = OT_ERROR_INVALID_ARGS);
            SuccessOrExit(error = ParseLong(argv[2], value));
            otChannelManagerRequestChannelChange(mInstance, static_cast<uint8_t>(value));
        }
        else if (strcmp(argv[1], "select") == 0)
        {
            VerifyOrExit(argc > 2, error = OT_ERROR_INVALID_ARGS);
            SuccessOrExit(error = ParseLong(argv[2], value));
            error = otChannelManagerRequestChannelSelect(mInstance, (value != 0) ? true : false);
        }
        else if (strcmp(argv[1], "auto") == 0)
        {
            VerifyOrExit(argc > 2, error = OT_ERROR_INVALID_ARGS);
            SuccessOrExit(error = ParseLong(argv[2], value));
            otChannelManagerSetAutoChannelSelectionEnabled(mInstance, (value != 0) ? true : false);
        }
        else if (strcmp(argv[1], "delay") == 0)
        {
            VerifyOrExit(argc > 2, error = OT_ERROR_INVALID_ARGS);
            SuccessOrExit(error = ParseLong(argv[2], value));
            error = otChannelManagerSetDelay(mInstance, static_cast<uint8_t>(value));
        }
        else if (strcmp(argv[1], "interval") == 0)
        {
            VerifyOrExit(argc > 2, error = OT_ERROR_INVALID_ARGS);
            SuccessOrExit(error = ParseLong(argv[2], value));
            error = otChannelManagerSetAutoChannelSelectionInterval(mInstance, static_cast<uint32_t>(value));
        }
        else if (strcmp(argv[1], "supported") == 0)
        {
            VerifyOrExit(argc > 2, error = OT_ERROR_INVALID_ARGS);
            SuccessOrExit(error = ParseLong(argv[2], value));
            otChannelManagerSetSupportedChannels(mInstance, static_cast<uint32_t>(value));
        }
        else if (strcmp(argv[1], "favored") == 0)
        {
            VerifyOrExit(argc > 2, error = OT_ERROR_INVALID_ARGS);
            SuccessOrExit(error = ParseLong(argv[2], value));
            otChannelManagerSetFavoredChannels(mInstance, static_cast<uint32_t>(value));
        }
        else
        {
            ExitNow(error = OT_ERROR_INVALID_ARGS);
        }
    }
#endif
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
    otError     error = OT_ERROR_NONE;
    otChildInfo childInfo;
    long        value;
    bool        isTable;

    VerifyOrExit(argc > 0, error = OT_ERROR_INVALID_ARGS);

    isTable = (strcmp(argv[0], "table") == 0);

    if (isTable || strcmp(argv[0], "list") == 0)
    {
        uint8_t maxChildren;

        if (isTable)
        {
            mServer->OutputFormat(
                "| ID  | RLOC16 | Timeout    | Age        | LQ In | C_VN |R|S|D|N| Extended MAC     |\r\n");
            mServer->OutputFormat(
                "+-----+--------+------------+------------+-------+------+-+-+-+-+------------------+\r\n");
        }

        maxChildren = otThreadGetMaxAllowedChildren(mInstance);

        for (uint8_t i = 0; i < maxChildren; i++)
        {
            if ((otThreadGetChildInfoByIndex(mInstance, i, &childInfo) != OT_ERROR_NONE) || childInfo.mIsStateRestoring)
            {
                continue;
            }

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
                mServer->OutputFormat("|%1d", childInfo.mFullThreadDevice);
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

        mServer->OutputFormat("\r\n");
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

    if (childInfo.mFullThreadDevice)
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
    long    value;

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
#endif // OPENTHREAD_FTD

void Interpreter::ProcessChildTimeout(int argc, char *argv[])
{
    otError error = OT_ERROR_NONE;
    long    value;

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

#if OPENTHREAD_ENABLE_APPLICATION_COAP_SECURE

void Interpreter::ProcessCoapSecure(int argc, char *argv[])
{
    otError error;
    error = mCoapSecure.Process(argc, argv);
    AppendResult(error);
}

#endif // OPENTHREAD_ENABLE_APPLICATION_COAP_SECURE

#if OPENTHREAD_FTD
void Interpreter::ProcessContextIdReuseDelay(int argc, char *argv[])
{
    otError error = OT_ERROR_NONE;
    long    value;

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
#endif // OPENTHREAD_FTD

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
            mServer->OutputFormat("    TxErrBusyChannel: %d\r\n", counters->mTxErrBusyChannel);
            mServer->OutputFormat("RxTotal: %d\r\n", counters->mRxTotal);
            mServer->OutputFormat("    RxUnicast: %d\r\n", counters->mRxUnicast);
            mServer->OutputFormat("    RxBroadcast: %d\r\n", counters->mRxBroadcast);
            mServer->OutputFormat("    RxData: %d\r\n", counters->mRxData);
            mServer->OutputFormat("    RxDataPoll: %d\r\n", counters->mRxDataPoll);
            mServer->OutputFormat("    RxBeacon: %d\r\n", counters->mRxBeacon);
            mServer->OutputFormat("    RxBeaconRequest: %d\r\n", counters->mRxBeaconRequest);
            mServer->OutputFormat("    RxOther: %d\r\n", counters->mRxOther);
            mServer->OutputFormat("    RxAddressFiltered: %d\r\n", counters->mRxAddressFiltered);
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
    otError  error        = OT_ERROR_NONE;
    uint32_t scanChannels = 0;
    long     value;

    if (argc > 0)
    {
        SuccessOrExit(error = ParseLong(argv[0], value));
        VerifyOrExit((0 <= value) && (value < static_cast<long>(sizeof(scanChannels) * CHAR_BIT)),
                     error = OT_ERROR_INVALID_ARGS);
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
    otError          error = OT_ERROR_NONE;
    long             port  = OT_DNS_DEFAULT_SERVER_PORT;
    Ip6::MessageInfo messageInfo;
    otDnsQuery       query;

    VerifyOrExit(argc > 0, error = OT_ERROR_INVALID_ARGS);

    if (strcmp(argv[0], "resolve") == 0)
    {
        VerifyOrExit(!mResolvingInProgress, error = OT_ERROR_BUSY);
        VerifyOrExit(argc > 1, error = OT_ERROR_INVALID_ARGS);
        VerifyOrExit(strlen(argv[1]) < OT_DNS_MAX_HOSTNAME_LENGTH, error = OT_ERROR_INVALID_ARGS);

        strcpy(mResolvingHostname, argv[1]);

        messageInfo.mInterfaceId = OT_NETIF_INTERFACE_ID_THREAD;

        if (argc > 2)
        {
            SuccessOrExit(error = messageInfo.GetPeerAddr().FromString(argv[2]));
        }
        else
        {
            // Use IPv6 address of default DNS server.
            SuccessOrExit(error = messageInfo.GetPeerAddr().FromString(OT_DNS_DEFAULT_SERVER_IP));
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
    }
    else
    {
        ExitNow(error = OT_ERROR_INVALID_ARGS);
    }

exit:
    if (error != OT_ERROR_NONE)
    {
        AppendResult(error);
    }
}

void Interpreter::s_HandleDnsResponse(void *        aContext,
                                      const char *  aHostname,
                                      otIp6Address *aAddress,
                                      uint32_t      aTtl,
                                      otError       aResult)
{
    static_cast<Interpreter *>(aContext)->HandleDnsResponse(aHostname, *static_cast<Ip6::Address *>(aAddress), aTtl,
                                                            aResult);
}

void Interpreter::HandleDnsResponse(const char *aHostname, Ip6::Address &aAddress, uint32_t aTtl, otError aResult)
{
    mServer->OutputFormat("DNS response for %s - ", aHostname);

    if (aResult == OT_ERROR_NONE)
    {
        mServer->OutputFormat("[%x:%x:%x:%x:%x:%x:%x:%x] TTL: %d\r\n", HostSwap16(aAddress.mFields.m16[0]),
                              HostSwap16(aAddress.mFields.m16[1]), HostSwap16(aAddress.mFields.m16[2]),
                              HostSwap16(aAddress.mFields.m16[3]), HostSwap16(aAddress.mFields.m16[4]),
                              HostSwap16(aAddress.mFields.m16[5]), HostSwap16(aAddress.mFields.m16[6]),
                              HostSwap16(aAddress.mFields.m16[7]), aTtl);
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
    OT_UNUSED_VARIABLE(argc);
    OT_UNUSED_VARIABLE(argv);

    otEidCacheEntry entry;

    for (uint8_t i = 0;; i++)
    {
        SuccessOrExit(otThreadGetEidCacheEntry(mInstance, i, &entry));

        if (entry.mValid == false)
        {
            continue;
        }

        mServer->OutputFormat("%x:%x:%x:%x:%x:%x:%x:%x %04x\r\n", HostSwap16(entry.mTarget.mFields.m16[0]),
                              HostSwap16(entry.mTarget.mFields.m16[1]), HostSwap16(entry.mTarget.mFields.m16[2]),
                              HostSwap16(entry.mTarget.mFields.m16[3]), HostSwap16(entry.mTarget.mFields.m16[4]),
                              HostSwap16(entry.mTarget.mFields.m16[5]), HostSwap16(entry.mTarget.mFields.m16[6]),
                              HostSwap16(entry.mTarget.mFields.m16[7]), entry.mRloc16);
    }

exit:
    AppendResult(OT_ERROR_NONE);
}
#endif // OPENTHREAD_FTD

void Interpreter::ProcessEui64(int argc, char *argv[])
{
    OT_UNUSED_VARIABLE(argv);

    otError      error = OT_ERROR_NONE;
    otExtAddress extAddress;

    VerifyOrExit(argc == 0, error = OT_ERROR_INVALID_ARGS);

    otLinkGetFactoryAssignedIeeeEui64(mInstance, &extAddress);
    OutputBytes(extAddress.m8, OT_EXT_ADDRESS_SIZE);
    mServer->OutputFormat("\r\n");

exit:
    AppendResult(error);
}

void Interpreter::ProcessExtAddress(int argc, char *argv[])
{
    otError error = OT_ERROR_NONE;

    if (argc == 0)
    {
        otBufferPtr extAddress(reinterpret_cast<const uint8_t *>(otLinkGetExtendedAddress(mInstance)));
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

#if OPENTHREAD_POSIX
void Interpreter::ProcessExit(int argc, char *argv[])
{
    OT_UNUSED_VARIABLE(argc);
    OT_UNUSED_VARIABLE(argv);

    exit(EXIT_SUCCESS);
}
#endif

#if (OPENTHREAD_CONFIG_LOG_OUTPUT == OPENTHREAD_CONFIG_LOG_OUTPUT_DEBUG_UART) && OPENTHREAD_POSIX

void Interpreter::ProcessLogFilename(int argc, char *argv[])
{
    otError error = OT_ERROR_NONE;

    VerifyOrExit(argc == 1, error = OT_ERROR_INVALID_ARGS);

    SuccessOrExit(error = otPlatDebugUart_logfile(argv[0]));

exit:
    AppendResult(error);
}
#endif

void Interpreter::ProcessExtPanId(int argc, char *argv[])
{
    otError error = OT_ERROR_NONE;

    if (argc == 0)
    {
        otBufferPtr extPanId(reinterpret_cast<const uint8_t *>(otThreadGetExtendedPanId(mInstance)));
        OutputBytes(extPanId, OT_EXT_PAN_ID_SIZE);
        mServer->OutputFormat("\r\n");
    }
    else
    {
        otExtendedPanId extPanId;

        VerifyOrExit(Hex2Bin(argv[0], extPanId.m8, sizeof(extPanId)) >= 0, error = OT_ERROR_PARSE);

        error = otThreadSetExtendedPanId(mInstance, &extPanId);
    }

exit:
    AppendResult(error);
}

void Interpreter::ProcessFactoryReset(int argc, char *argv[])
{
    OT_UNUSED_VARIABLE(argc);
    OT_UNUSED_VARIABLE(argv);

    otInstanceFactoryReset(mInstance);
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
    else
    {
        ExitNow(error = OT_ERROR_INVALID_ARGS);
    }

exit:
    AppendResult(error);
}

otError Interpreter::ProcessIpAddrAdd(int argc, char *argv[])
{
    otError        error;
    otNetifAddress aAddress;

    VerifyOrExit(argc > 0, error = OT_ERROR_INVALID_ARGS);

    SuccessOrExit(error = otIp6AddressFromString(argv[0], &aAddress.mAddress));
    aAddress.mPrefixLength = 64;
    aAddress.mPreferred    = true;
    aAddress.mValid        = true;
    error                  = otIp6AddUnicastAddress(mInstance, &aAddress);

exit:
    return error;
}

otError Interpreter::ProcessIpAddrDel(int argc, char *argv[])
{
    otError             error;
    struct otIp6Address address;

    VerifyOrExit(argc > 0, error = OT_ERROR_INVALID_ARGS);

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
            mServer->OutputFormat("%x:%x:%x:%x:%x:%x:%x:%x\r\n", HostSwap16(addr->mAddress.mFields.m16[0]),
                                  HostSwap16(addr->mAddress.mFields.m16[1]), HostSwap16(addr->mAddress.mFields.m16[2]),
                                  HostSwap16(addr->mAddress.mFields.m16[3]), HostSwap16(addr->mAddress.mFields.m16[4]),
                                  HostSwap16(addr->mAddress.mFields.m16[5]), HostSwap16(addr->mAddress.mFields.m16[6]),
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
        else
        {
            ExitNow(error = OT_ERROR_INVALID_ARGS);
        }
    }

exit:
    AppendResult(error);
}

#ifndef OTDLL
otError Interpreter::ProcessIpMulticastAddrAdd(int argc, char *argv[])
{
    otError             error;
    struct otIp6Address address;

    VerifyOrExit(argc > 0, error = OT_ERROR_INVALID_ARGS);

    SuccessOrExit(error = otIp6AddressFromString(argv[0], &address));
    error = otIp6SubscribeMulticastAddress(mInstance, &address);

exit:
    return error;
}

otError Interpreter::ProcessIpMulticastAddrDel(int argc, char *argv[])
{
    otError             error;
    struct otIp6Address address;

    VerifyOrExit(argc > 0, error = OT_ERROR_INVALID_ARGS);

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
            ExitNow(error = OT_ERROR_INVALID_ARGS);
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
            mServer->OutputFormat("%x:%x:%x:%x:%x:%x:%x:%x\r\n", HostSwap16(addr->mAddress.mFields.m16[0]),
                                  HostSwap16(addr->mAddress.mFields.m16[1]), HostSwap16(addr->mAddress.mFields.m16[2]),
                                  HostSwap16(addr->mAddress.mFields.m16[3]), HostSwap16(addr->mAddress.mFields.m16[4]),
                                  HostSwap16(addr->mAddress.mFields.m16[5]), HostSwap16(addr->mAddress.mFields.m16[6]),
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
        else
        {
            ExitNow(error = OT_ERROR_INVALID_ARGS);
        }
    }

exit:
    AppendResult(error);
}
#endif

void Interpreter::ProcessKeySequence(int argc, char *argv[])
{
    otError error = OT_ERROR_NONE;
    long    value;

    VerifyOrExit(argc == 1 || argc == 2, error = OT_ERROR_INVALID_ARGS);

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
    else
    {
        ExitNow(error = OT_ERROR_INVALID_ARGS);
    }

exit:
    AppendResult(error);
}

void Interpreter::ProcessLeaderData(int argc, char *argv[])
{
    OT_UNUSED_VARIABLE(argc);
    OT_UNUSED_VARIABLE(argv);

    otError      error;
    otLeaderData leaderData;

    SuccessOrExit(error = otThreadGetLeaderData(mInstance, &leaderData));

    mServer->OutputFormat("Partition ID: %u\r\n", leaderData.mPartitionId);
    mServer->OutputFormat("Weighting: %d\r\n", leaderData.mWeighting);
    mServer->OutputFormat("Data Version: %d\r\n", leaderData.mDataVersion);
    mServer->OutputFormat("Stable Data Version: %d\r\n", leaderData.mStableDataVersion);
    mServer->OutputFormat("Leader Router ID: %d\r\n", leaderData.mLeaderRouterId);

exit:
    AppendResult(error);
}

#if OPENTHREAD_FTD
void Interpreter::ProcessLeaderPartitionId(int argc, char *argv[])
{
    otError       error = OT_ERROR_NONE;
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
    long    value;

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
#endif // OPENTHREAD_FTD

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
    otError          error = OT_ERROR_NONE;
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

#if OPENTHREAD_FTD
void Interpreter::ProcessNeighbor(int argc, char *argv[])
{
    otError                error = OT_ERROR_NONE;
    otNeighborInfo         neighborInfo;
    bool                   isTable;
    otNeighborInfoIterator iterator = OT_NEIGHBOR_INFO_ITERATOR_INIT;

    VerifyOrExit(argc > 0, error = OT_ERROR_INVALID_ARGS);

    isTable = (strcmp(argv[0], "table") == 0);

    if (isTable || strcmp(argv[0], "list") == 0)
    {
        if (isTable)
        {
            mServer->OutputFormat("| Role | RLOC16 | Age | Avg RSSI | Last RSSI |R|S|D|N| Extended MAC     |\r\n");
            mServer->OutputFormat("+------+--------+-----+----------+-----------+-+-+-+-+------------------+\r\n");
        }

        while (otThreadGetNextNeighborInfo(mInstance, &iterator, &neighborInfo) == OT_ERROR_NONE)
        {
            if (isTable)
            {
                mServer->OutputFormat("| %3c  ", neighborInfo.mIsChild ? 'C' : 'R');
                mServer->OutputFormat("| 0x%04x ", neighborInfo.mRloc16);
                mServer->OutputFormat("| %3d ", neighborInfo.mAge);
                mServer->OutputFormat("| %8d ", neighborInfo.mAverageRssi);
                mServer->OutputFormat("| %9d ", neighborInfo.mLastRssi);
                mServer->OutputFormat("|%1d", neighborInfo.mRxOnWhenIdle);
                mServer->OutputFormat("|%1d", neighborInfo.mSecureDataRequest);
                mServer->OutputFormat("|%1d", neighborInfo.mFullThreadDevice);
                mServer->OutputFormat("|%1d", neighborInfo.mFullNetworkData);
                mServer->OutputFormat("| ");

                for (size_t j = 0; j < sizeof(neighborInfo.mExtAddress); j++)
                {
                    mServer->OutputFormat("%02x", neighborInfo.mExtAddress.m8[j]);
                }

                mServer->OutputFormat(" |\r\n");
            }
            else
            {
                mServer->OutputFormat("0x%04x ", neighborInfo.mRloc16);
            }
        }

        mServer->OutputFormat("\r\n");
    }
    else
    {
        ExitNow(error = OT_ERROR_INVALID_ARGS);
    }

exit:
    AppendResult(error);
}
#endif

#if OPENTHREAD_ENABLE_SERVICE
void Interpreter::ProcessNetworkDataShow(int argc, char *argv[])
{
    OT_UNUSED_VARIABLE(argc);
    OT_UNUSED_VARIABLE(argv);

    otError error = OT_ERROR_NONE;
    uint8_t data[255];
    uint8_t len = sizeof(data);

    SuccessOrExit(error = otNetDataGet(mInstance, false, data, &len));

    this->OutputBytes(data, static_cast<uint8_t>(len));
    mServer->OutputFormat("\r\n");

exit:
    AppendResult(error);
}

void Interpreter::ProcessService(int argc, char *argv[])
{
    otError error = OT_ERROR_NONE;

    VerifyOrExit(argc > 0, error = OT_ERROR_INVALID_ARGS);

    if (strcmp(argv[0], "add") == 0)
    {
        otServiceConfig cfg;
        long            enterpriseNumber;
        size_t          length;

        VerifyOrExit(argc > 3, error = OT_ERROR_INVALID_ARGS);

        SuccessOrExit(error = ParseLong(argv[1], enterpriseNumber));
        cfg.mEnterpriseNumber = static_cast<uint32_t>(enterpriseNumber);

        length = strlen(argv[2]);
        VerifyOrExit(length <= sizeof(cfg.mServiceData), error = OT_ERROR_NO_BUFS);
        cfg.mServiceDataLength = static_cast<uint8_t>(length);
        memcpy(cfg.mServiceData, argv[2], cfg.mServiceDataLength);

        length = strlen(argv[3]);
        VerifyOrExit(length <= sizeof(cfg.mServerConfig.mServerData), error = OT_ERROR_NO_BUFS);
        cfg.mServerConfig.mServerDataLength = static_cast<uint8_t>(length);
        memcpy(cfg.mServerConfig.mServerData, argv[3], cfg.mServerConfig.mServerDataLength);

        cfg.mServerConfig.mStable = true;

        SuccessOrExit(error = otServerAddService(mInstance, &cfg));
    }
    else if (strcmp(argv[0], "remove") == 0)
    {
        long enterpriseNumber = 0;

        VerifyOrExit(argc > 2, error = OT_ERROR_INVALID_ARGS);

        SuccessOrExit(error = ParseLong(argv[1], enterpriseNumber));

        SuccessOrExit(error = otServerRemoveService(mInstance, static_cast<uint32_t>(enterpriseNumber),
                                                    reinterpret_cast<uint8_t *>(argv[2]),
                                                    static_cast<uint8_t>(strlen(argv[2]))));
    }
    else
    {
        ExitNow(error = OT_ERROR_INVALID_ARGS);
    }

exit:
    AppendResult(error);
}
#endif

#if OPENTHREAD_ENABLE_BORDER_ROUTER || OPENTHREAD_ENABLE_SERVICE
void Interpreter::ProcessNetworkDataRegister(int argc, char *argv[])
{
    OT_UNUSED_VARIABLE(argc);
    OT_UNUSED_VARIABLE(argv);

    otError error = OT_ERROR_NONE;
#if OPENTHREAD_ENABLE_BORDER_ROUTER
    SuccessOrExit(error = otBorderRouterRegister(mInstance));
#else
    SuccessOrExit(error = otServerRegister(mInstance));
#endif

exit:
    AppendResult(error);
}
#endif // OPENTHREAD_ENABLE_BORDER_ROUTER || OPENTHREAD_ENABLE_SERVICE

#if OPENTHREAD_FTD
void Interpreter::ProcessNetworkIdTimeout(int argc, char *argv[])
{
    otError error = OT_ERROR_NONE;
    long    value;

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
#endif // OPENTHREAD_FTD

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

#if OPENTHREAD_CONFIG_ENABLE_TIME_SYNC
void Interpreter::ProcessNetworkTime(int argc, char *argv[])
{
    otError error = OT_ERROR_NONE;
    long    value;

    if (argc == 0)
    {
        uint64_t            time;
        otNetworkTimeStatus networkTimeStatus;

        networkTimeStatus = otNetworkTimeGet(mInstance, time);

        mServer->OutputFormat("Network Time:     %luus", time);

        switch (networkTimeStatus)
        {
        case OT_NETWORK_TIME_UNSYNCHRONIZED:
            mServer->OutputFormat(" (unsynchronized)\r\n");
            break;

        case OT_NETWORK_TIME_RESYNC_NEEDED:
            mServer->OutputFormat(" (resync needed)\r\n");
            break;

        case OT_NETWORK_TIME_SYNCHRONIZED:
            mServer->OutputFormat(" (synchronized)\r\n");
            break;

        default:
            break;
        }

        mServer->OutputFormat("Time Sync Period: %ds\r\n", otNetworkTimeGetSyncPeriod(mInstance));
        mServer->OutputFormat("XTAL Threshold:   %dppm\r\n", otNetworkTimeGetXtalThreshold(mInstance));
    }
    else if (argc == 2)
    {
        SuccessOrExit(error = ParseLong(argv[0], value));
        SuccessOrExit(error = otNetworkTimeSetSyncPeriod(mInstance, static_cast<uint16_t>(value)));

        SuccessOrExit(error = ParseLong(argv[1], value));
        SuccessOrExit(error = otNetworkTimeSetXtalThreshold(mInstance, static_cast<uint16_t>(value)));
    }
    else
    {
        ExitNow(error = OT_ERROR_INVALID_ARGS);
    }

exit:
    AppendResult(error);
}
#endif // OPENTHREAD_CONFIG_ENABLE_TIME_SYNC

void Interpreter::ProcessPanId(int argc, char *argv[])
{
    otError error = OT_ERROR_NONE;
    long    value;

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
    OT_UNUSED_VARIABLE(argc);
    OT_UNUSED_VARIABLE(argv);

    otError      error = OT_ERROR_NONE;
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
    AppendResult(error);
}

#if OPENTHREAD_FTD
void Interpreter::ProcessParentPriority(int argc, char *argv[])
{
    otError error = OT_ERROR_NONE;
    long    value;

    if (argc == 0)
    {
        mServer->OutputFormat("%d\r\n", otThreadGetParentPriority(mInstance));
    }
    else
    {
        SuccessOrExit(error = ParseLong(argv[0], value));
        error = otThreadSetParentPriority(mInstance, static_cast<int8_t>(value));
    }

exit:
    AppendResult(error);
}
#endif

#ifndef OTDLL
void Interpreter::s_HandleIcmpReceive(void *               aContext,
                                      otMessage *          aMessage,
                                      const otMessageInfo *aMessageInfo,
                                      const otIcmp6Header *aIcmpHeader)
{
    static_cast<Interpreter *>(aContext)->HandleIcmpReceive(*static_cast<Message *>(aMessage),
                                                            *static_cast<const Ip6::MessageInfo *>(aMessageInfo),
                                                            *static_cast<const Ip6::IcmpHeader *>(aIcmpHeader));
}

void Interpreter::HandleIcmpReceive(Message &               aMessage,
                                    const Ip6::MessageInfo &aMessageInfo,
                                    const otIcmp6Header &   aIcmpHeader)
{
    uint32_t timestamp = 0;

    VerifyOrExit(aIcmpHeader.mType == OT_ICMP6_TYPE_ECHO_REPLY);

    mServer->OutputFormat("%d bytes from ", aMessage.GetLength() - aMessage.GetOffset() + sizeof(otIcmp6Header));

    mServer->OutputFormat(
        "%x:%x:%x:%x:%x:%x:%x:%x", HostSwap16(aMessageInfo.GetPeerAddr().mFields.m16[0]),
        HostSwap16(aMessageInfo.GetPeerAddr().mFields.m16[1]), HostSwap16(aMessageInfo.GetPeerAddr().mFields.m16[2]),
        HostSwap16(aMessageInfo.GetPeerAddr().mFields.m16[3]), HostSwap16(aMessageInfo.GetPeerAddr().mFields.m16[4]),
        HostSwap16(aMessageInfo.GetPeerAddr().mFields.m16[5]), HostSwap16(aMessageInfo.GetPeerAddr().mFields.m16[6]),
        HostSwap16(aMessageInfo.GetPeerAddr().mFields.m16[7]));
    mServer->OutputFormat(": icmp_seq=%d hlim=%d", HostSwap16(aIcmpHeader.mData.m16[1]), aMessageInfo.mHopLimit);

    if (aMessage.Read(aMessage.GetOffset(), sizeof(uint32_t), &timestamp) >= static_cast<int>(sizeof(uint32_t)))
    {
        mServer->OutputFormat(" time=%dms", TimerMilli::GetNow() - HostSwap32(timestamp));
    }

    mServer->OutputFormat("\r\n");

exit:
    return;
}

void Interpreter::ProcessPing(int argc, char *argv[])
{
    otError error = OT_ERROR_NONE;
    uint8_t index = 1;
    long    value;

    VerifyOrExit(argc > 0, error = OT_ERROR_INVALID_ARGS);

    if (strcmp(argv[0], "stop") == 0)
    {
        if (!mPingTimer.IsRunning())
        {
            error = OT_ERROR_INVALID_STATE;
        }
        else
        {
            mPingTimer.Stop();
        }

        ExitNow();
    }

    VerifyOrExit(!mPingTimer.IsRunning(), error = OT_ERROR_BUSY);

    SuccessOrExit(error = mMessageInfo.GetPeerAddr().FromString(argv[0]));
    mMessageInfo.mInterfaceId = OT_NETIF_INTERFACE_ID_THREAD;

    mLength   = 8;
    mCount    = 1;
    mInterval = 1000;

    while (index < argc)
    {
        SuccessOrExit(error = ParseLong(argv[index], value));

        switch (index)
        {
        case 1:
            mLength = static_cast<uint16_t>(value);
            break;

        case 2:
            mCount = static_cast<uint16_t>(value);
            break;

        case 3:
            VerifyOrExit(value <= Timer::kMaxDt / 1000, error = OT_ERROR_INVALID_ARGS);
            mInterval = static_cast<uint32_t>(value) * 1000;
            break;

        default:
            ExitNow(error = OT_ERROR_INVALID_ARGS);
        }

        index++;
    }

    HandlePingTimer();

    return;

exit:
    AppendResult(error);
}

void Interpreter::s_HandlePingTimer(Timer &aTimer)
{
    GetOwner(aTimer).HandlePingTimer();
}

void Interpreter::HandlePingTimer()
{
    otError  error     = OT_ERROR_NONE;
    uint32_t timestamp = HostSwap32(TimerMilli::GetNow());

    otMessage *          message;
    const otMessageInfo *messageInfo = static_cast<const otMessageInfo *>(&mMessageInfo);

    VerifyOrExit((message = otIp6NewMessage(mInstance, NULL)) != NULL, error = OT_ERROR_NO_BUFS);
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
    long    value;

    if (argc == 0)
    {
        mServer->OutputFormat("%d\r\n", otLinkGetPollPeriod(mInstance));
    }
    else
    {
        SuccessOrExit(error = ParseLong(argv[0], value));
        error = otLinkSetPollPeriod(mInstance, static_cast<uint32_t>(value));
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
        else
        {
            ExitNow(error = OT_ERROR_INVALID_ARGS);
        }
    }

exit:
    AppendResult(error);
}

void Interpreter::s_HandleLinkPcapReceive(const otRadioFrame *aFrame, bool aIsTx, void *aContext)
{
    static_cast<Interpreter *>(aContext)->HandleLinkPcapReceive(aFrame, aIsTx);
}

void Interpreter::HandleLinkPcapReceive(const otRadioFrame *aFrame, bool aIsTx)
{
    OT_UNUSED_VARIABLE(aIsTx);

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
    otError              error = OT_ERROR_NONE;
    otBorderRouterConfig config;
    int                  argcur = 0;

    VerifyOrExit(argc > 0, error = OT_ERROR_INVALID_ARGS);

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
    OT_UNUSED_VARIABLE(argc);

    otError            error = OT_ERROR_NONE;
    struct otIp6Prefix prefix;
    int                argcur = 0;

    VerifyOrExit(argc > 0, error = OT_ERROR_INVALID_ARGS);

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
    return error;
}

otError Interpreter::ProcessPrefixList(void)
{
    otNetworkDataIterator iterator = OT_NETWORK_DATA_ITERATOR_INIT;
    otBorderRouterConfig  config;

    while (otBorderRouterGetNextOnMeshPrefix(mInstance, &iterator, &config) == OT_ERROR_NONE)
    {
        mServer->OutputFormat("%x:%x:%x:%x::/%d ", HostSwap16(config.mPrefix.mPrefix.mFields.m16[0]),
                              HostSwap16(config.mPrefix.mPrefix.mFields.m16[1]),
                              HostSwap16(config.mPrefix.mPrefix.mFields.m16[2]),
                              HostSwap16(config.mPrefix.mPrefix.mFields.m16[3]), config.mPrefix.mLength);

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
        ExitNow(error = OT_ERROR_INVALID_ARGS);
    }

exit:
    AppendResult(error);
}
#endif // OPENTHREAD_ENABLE_BORDER_ROUTER

#if OPENTHREAD_FTD
void Interpreter::ProcessReleaseRouterId(int argc, char *argv[])
{
    otError error = OT_ERROR_NONE;
    long    value;

    VerifyOrExit(argc > 0, error = OT_ERROR_INVALID_ARGS);

    SuccessOrExit(error = ParseLong(argv[0], value));
    SuccessOrExit(error = otThreadReleaseRouterId(mInstance, static_cast<uint8_t>(value)));

exit:
    AppendResult(error);
}
#endif // OPENTHREAD_FTD

void Interpreter::ProcessReset(int argc, char *argv[])
{
    OT_UNUSED_VARIABLE(argc);
    OT_UNUSED_VARIABLE(argv);

    otInstanceReset(mInstance);
}

void Interpreter::ProcessRloc16(int argc, char *argv[])
{
    OT_UNUSED_VARIABLE(argc);
    OT_UNUSED_VARIABLE(argv);

    mServer->OutputFormat("%04x\r\n", otThreadGetRloc16(mInstance));
    mServer->OutputFormat("Done\r\n");
}

#if OPENTHREAD_ENABLE_BORDER_ROUTER
otError Interpreter::ProcessRouteAdd(int argc, char *argv[])
{
    otError               error = OT_ERROR_NONE;
    otExternalRouteConfig config;
    int                   argcur = 0;

    memset(&config, 0, sizeof(otExternalRouteConfig));

    char *prefixLengthStr;
    char *endptr;

    VerifyOrExit(argc > 0, error = OT_ERROR_INVALID_ARGS);

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
    otError            error = OT_ERROR_NONE;
    struct otIp6Prefix prefix;
    int                argcur = 0;

    memset(&prefix, 0, sizeof(struct otIp6Prefix));
    char *prefixLengthStr;
    char *endptr;

    VerifyOrExit(argc > 0, error = OT_ERROR_INVALID_ARGS);

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

otError Interpreter::ProcessRouteList(void)
{
    otNetworkDataIterator iterator = OT_NETWORK_DATA_ITERATOR_INIT;
    otExternalRouteConfig config;

    while (otBorderRouterGetNextRoute(mInstance, &iterator, &config) == OT_ERROR_NONE)
    {
        mServer->OutputFormat("%x:%x:%x:%x::/%d ", HostSwap16(config.mPrefix.mPrefix.mFields.m16[0]),
                              HostSwap16(config.mPrefix.mPrefix.mFields.m16[1]),
                              HostSwap16(config.mPrefix.mPrefix.mFields.m16[2]),
                              HostSwap16(config.mPrefix.mPrefix.mFields.m16[3]), config.mPrefix.mLength);

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

void Interpreter::ProcessRoute(int argc, char *argv[])
{
    otError error = OT_ERROR_NONE;

    if (argc == 0)
    {
        SuccessOrExit(error = ProcessRouteList());
    }
    else if (strcmp(argv[0], "add") == 0)
    {
        SuccessOrExit(error = ProcessRouteAdd(argc - 1, argv + 1));
    }
    else if (strcmp(argv[0], "remove") == 0)
    {
        SuccessOrExit(error = ProcessRouteRemove(argc - 1, argv + 1));
    }
    else
    {
        ExitNow(error = OT_ERROR_INVALID_ARGS);
    }

exit:
    AppendResult(error);
}
#endif // OPENTHREAD_ENABLE_BORDER_ROUTER

#if OPENTHREAD_FTD
void Interpreter::ProcessRouter(int argc, char *argv[])
{
    otError      error = OT_ERROR_NONE;
    otRouterInfo routerInfo;
    long         value;
    bool         isTable;

    VerifyOrExit(argc > 0, error = OT_ERROR_INVALID_ARGS);

    isTable = (strcmp(argv[0], "table") == 0);

    if (isTable || strcmp(argv[0], "list") == 0)
    {
        uint8_t maxRouterId;

        if (isTable)
        {
            mServer->OutputFormat(
                "| ID | RLOC16 | Next Hop | Path Cost | LQ In | LQ Out | Age | Extended MAC     |\r\n");
            mServer->OutputFormat(
                "+----+--------+----------+-----------+-------+--------+-----+------------------+\r\n");
        }

        maxRouterId = otThreadGetMaxRouterId(mInstance);

        for (uint8_t i = 0; i <= maxRouterId; i++)
        {
            if (otThreadGetRouterInfo(mInstance, i, &routerInfo) != OT_ERROR_NONE)
            {
                continue;
            }

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

        mServer->OutputFormat("\r\n");
        ExitNow();
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
    long    value;

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
        ExitNow(error = OT_ERROR_INVALID_ARGS);
    }

exit:
    AppendResult(error);
}

void Interpreter::ProcessRouterSelectionJitter(int argc, char *argv[])
{
    otError error = OT_ERROR_NONE;
    long    value;

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
    long    value;

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
#endif // OPENTHREAD_FTD

void Interpreter::ProcessScan(int argc, char *argv[])
{
    otError  error        = OT_ERROR_NONE;
    uint32_t scanChannels = 0;
    uint16_t scanDuration = 0;
    bool     energyScan   = false;
    long     value;

    if (argc > 0)
    {
        if (strcmp(argv[0], "energy") == 0)
        {
            energyScan = true;

            if (argc > 1)
            {
                SuccessOrExit(error = ParseLong(argv[1], value));
                scanDuration = static_cast<uint16_t>(value);
            }
        }
        else
        {
            SuccessOrExit(error = ParseLong(argv[0], value));
            VerifyOrExit((0 <= value) && (value < static_cast<long>(sizeof(scanChannels) * CHAR_BIT)),
                         error = OT_ERROR_INVALID_ARGS);
            scanChannels = 1 << value;
        }
    }

    if (energyScan)
    {
        mServer->OutputFormat("| Ch | RSSI |\r\n");
        mServer->OutputFormat("+----+------+\r\n");
        SuccessOrExit(error = otLinkEnergyScan(mInstance, scanChannels, scanDuration,
                                               &Interpreter::s_HandleEnergyScanResult, this));
    }
    else
    {
        mServer->OutputFormat(
            "| J | Network Name     | Extended PAN     | PAN  | MAC Address      | Ch | dBm | LQI |\r\n");
        mServer->OutputFormat(
            "+---+------------------+------------------+------+------------------+----+-----+-----+\r\n");
        SuccessOrExit(error = otLinkActiveScan(mInstance, scanChannels, scanDuration,
                                               &Interpreter::s_HandleActiveScanResult, this));
    }

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

void OTCALL Interpreter::s_HandleEnergyScanResult(otEnergyScanResult *aResult, void *aContext)
{
    static_cast<Interpreter *>(aContext)->HandleEnergyScanResult(aResult);
}

void Interpreter::HandleEnergyScanResult(otEnergyScanResult *aResult)
{
    if (aResult == NULL)
    {
        mServer->OutputFormat("Done\r\n");
        ExitNow();
    }

    mServer->OutputFormat("| %2d | %4d |\r\n", aResult->mChannel, aResult->mMaxRssi);

exit:
    return;
}

void Interpreter::ProcessSingleton(int argc, char *argv[])
{
    OT_UNUSED_VARIABLE(argc);
    OT_UNUSED_VARIABLE(argv);

    otError error = OT_ERROR_NONE;

    if (otThreadIsSingleton(mInstance))
    {
        mServer->OutputFormat("true\r\n");
    }
    else
    {
        mServer->OutputFormat("false\r\n");
    }

    AppendResult(error);
}

#if OPENTHREAD_ENABLE_SNTP_CLIENT
void Interpreter::ProcessSntp(int argc, char *argv[])
{
    otError          error = OT_ERROR_NONE;
    long             port  = OT_SNTP_DEFAULT_SERVER_PORT;
    Ip6::MessageInfo messageInfo;
    otSntpQuery      query;

    VerifyOrExit(argc > 0, error = OT_ERROR_INVALID_ARGS);

    if (strcmp(argv[0], "query") == 0)
    {
        VerifyOrExit(!mSntpQueryingInProgress, error = OT_ERROR_BUSY);

        messageInfo.mInterfaceId = OT_NETIF_INTERFACE_ID_THREAD;

        if (argc > 1)
        {
            SuccessOrExit(error = messageInfo.GetPeerAddr().FromString(argv[1]));
        }
        else
        {
            // Use IPv6 address of default SNTP server.
            SuccessOrExit(error = messageInfo.GetPeerAddr().FromString(OT_SNTP_DEFAULT_SERVER_IP));
        }

        if (argc > 2)
        {
            SuccessOrExit(error = ParseLong(argv[2], port));
        }

        messageInfo.SetPeerPort(static_cast<uint16_t>(port));

        query.mMessageInfo = static_cast<const otMessageInfo *>(&messageInfo);

        SuccessOrExit(error = otSntpClientQuery(mInstance, &query, &Interpreter::s_HandleSntpResponse, this));

        mSntpQueryingInProgress = true;
    }
    else
    {
        ExitNow(error = OT_ERROR_INVALID_ARGS);
    }

exit:
    if (error != OT_ERROR_NONE)
    {
        AppendResult(error);
    }
}

void Interpreter::s_HandleSntpResponse(void *aContext, uint64_t aTime, otError aResult)
{
    static_cast<Interpreter *>(aContext)->HandleSntpResponse(aTime, aResult);
}

void Interpreter::HandleSntpResponse(uint64_t aTime, otError aResult)
{
    if (aResult == OT_ERROR_NONE)
    {
        // Some Embedded C libraries do not support printing of 64-bit unsigned integers.
        // To simplify, unix epoch time and era number are printed separately.
        mServer->OutputFormat("SNTP response - Unix time: %ld (era: %ld)\r\n",
                              static_cast<uint32_t>(aTime & UINT32_MAX), static_cast<uint32_t>(aTime >> 32));
    }
    else
    {
        mServer->OutputFormat("SNTP error - %s\r\n", otThreadErrorToString(aResult));
    }

    mSntpQueryingInProgress = false;
}
#endif

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
#endif // OPENTHREAD_FTD

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

#endif // OPENTHREAD_FTD
        else
        {
            ExitNow(error = OT_ERROR_INVALID_ARGS);
        }
    }

exit:
    AppendResult(error);
}

void Interpreter::ProcessThread(int argc, char *argv[])
{
    OT_UNUSED_VARIABLE(argc);
    OT_UNUSED_VARIABLE(argv);

    otError error = OT_ERROR_NONE;

    VerifyOrExit(argc > 0, error = OT_ERROR_INVALID_ARGS);

    if (strcmp(argv[0], "start") == 0)
    {
        SuccessOrExit(error = otThreadSetEnabled(mInstance, true));
    }
    else if (strcmp(argv[0], "stop") == 0)
    {
        SuccessOrExit(error = otThreadSetEnabled(mInstance, false));
    }
    else
    {
        ExitNow(error = OT_ERROR_INVALID_ARGS);
    }

exit:
    AppendResult(error);
}

void Interpreter::ProcessDataset(int argc, char *argv[])
{
    otError error;
    error = mDataset.Process(argc, argv);
    AppendResult(error);
}

#ifndef OTDLL
void Interpreter::ProcessTxPower(int argc, char *argv[])
{
    otError error = OT_ERROR_NONE;

    if (argc == 0)
    {
        int8_t power;

        SuccessOrExit(error = otPlatRadioGetTransmitPower(mInstance, &power));
        mServer->OutputFormat("%d dBm\r\n", power);
    }
    else
    {
        long value;

        SuccessOrExit(error = ParseLong(argv[0], value));
        SuccessOrExit(error = otPlatRadioSetTransmitPower(mInstance, static_cast<int8_t>(value)));
    }

exit:
    AppendResult(error);
}

void Interpreter::ProcessUdp(int argc, char *argv[])
{
    otError error;
    error = mUdp.Process(argc, argv);
    AppendResult(error);
}
#endif

void Interpreter::ProcessVersion(int argc, char *argv[])
{
    OT_UNUSED_VARIABLE(argc);
    OT_UNUSED_VARIABLE(argv);

    otStringPtr version(otGetVersionString());
    mServer->OutputFormat("%s\r\n", (const char *)version);
    AppendResult(OT_ERROR_NONE);
}

#if OPENTHREAD_ENABLE_COMMISSIONER && OPENTHREAD_FTD

void Interpreter::ProcessCommissioner(int argc, char *argv[])
{
    otError error = OT_ERROR_NONE;

    VerifyOrExit(argc > 0, error = OT_ERROR_INVALID_ARGS);

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
        otExtAddress        addr;
        const otExtAddress *addrPtr;

        VerifyOrExit(argc > 2, error = OT_ERROR_INVALID_ARGS);

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
            VerifyOrExit(argc > 3, error = OT_ERROR_INVALID_ARGS);
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
        long         mask;
        long         count;
        long         period;
        otIp6Address address;

        VerifyOrExit(argc > 4, error = OT_ERROR_INVALID_ARGS);

        // mask
        SuccessOrExit(error = ParseLong(argv[1], mask));

        // count
        SuccessOrExit(error = ParseLong(argv[2], count));

        // period
        SuccessOrExit(error = ParseLong(argv[3], period));

        // destination
        SuccessOrExit(error = otIp6AddressFromString(argv[4], &address));

        SuccessOrExit(error = otCommissionerAnnounceBegin(mInstance, static_cast<uint32_t>(mask),
                                                          static_cast<uint8_t>(count), static_cast<uint16_t>(period),
                                                          &address));
    }
    else if (strcmp(argv[0], "energy") == 0)
    {
        long         mask;
        long         count;
        long         period;
        long         scanDuration;
        otIp6Address address;

        VerifyOrExit(argc > 5, error = OT_ERROR_INVALID_ARGS);

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

        SuccessOrExit(error =
                          otCommissionerEnergyScan(mInstance, static_cast<uint32_t>(mask), static_cast<uint8_t>(count),
                                                   static_cast<uint16_t>(period), static_cast<uint16_t>(scanDuration),
                                                   &address, Interpreter::s_HandleEnergyReport, this));
    }
    else if (strcmp(argv[0], "panid") == 0)
    {
        long         panid;
        long         mask;
        otIp6Address address;

        VerifyOrExit(argc > 3, error = OT_ERROR_INVALID_ARGS);

        // panid
        SuccessOrExit(error = ParseLong(argv[1], panid));

        // mask
        SuccessOrExit(error = ParseLong(argv[2], mask));

        // destination
        SuccessOrExit(error = otIp6AddressFromString(argv[3], &address));

        SuccessOrExit(error =
                          otCommissionerPanIdQuery(mInstance, static_cast<uint16_t>(panid), static_cast<uint32_t>(mask),
                                                   &address, Interpreter::s_HandlePanIdConflict, this));
    }
    else if (strcmp(argv[0], "mgmtget") == 0)
    {
        uint8_t tlvs[32];
        long    value;
        int     length = 0;

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
                VerifyOrExit(++index < argc, error = OT_ERROR_INVALID_ARGS);
                value = static_cast<long>(strlen(argv[index]) + 1) / 2;
                VerifyOrExit(static_cast<size_t>(value) <= (sizeof(tlvs) - static_cast<size_t>(length)),
                             error = OT_ERROR_NO_BUFS);
                VerifyOrExit(Interpreter::Hex2Bin(argv[index], tlvs + length, static_cast<uint16_t>(value)) >= 0,
                             error = OT_ERROR_PARSE);
                length += value;
            }
            else
            {
                ExitNow(error = OT_ERROR_INVALID_ARGS);
            }
        }

        SuccessOrExit(error = otCommissionerSendMgmtGet(mInstance, tlvs, static_cast<uint8_t>(length)));
    }
    else if (strcmp(argv[0], "mgmtset") == 0)
    {
        otCommissioningDataset dataset;
        uint8_t                tlvs[32];
        long                   value;
        int                    length = 0;

        VerifyOrExit(argc > 0, error = OT_ERROR_INVALID_ARGS);

        memset(&dataset, 0, sizeof(dataset));

        for (uint8_t index = 1; index < argc; index++)
        {
            VerifyOrExit(static_cast<size_t>(length) < sizeof(tlvs), error = OT_ERROR_NO_BUFS);

            if (strcmp(argv[index], "locator") == 0)
            {
                VerifyOrExit(++index < argc, error = OT_ERROR_INVALID_ARGS);
                dataset.mIsLocatorSet = true;
                SuccessOrExit(error = Interpreter::ParseLong(argv[index], value));
                dataset.mLocator = static_cast<uint16_t>(value);
            }
            else if (strcmp(argv[index], "sessionid") == 0)
            {
                VerifyOrExit(++index < argc, error = OT_ERROR_INVALID_ARGS);
                dataset.mIsSessionIdSet = true;
                SuccessOrExit(error = Interpreter::ParseLong(argv[index], value));
                dataset.mSessionId = static_cast<uint16_t>(value);
            }
            else if (strcmp(argv[index], "steeringdata") == 0)
            {
                VerifyOrExit(++index < argc, error = OT_ERROR_INVALID_ARGS);
                dataset.mIsSteeringDataSet = true;
                length                     = static_cast<int>((strlen(argv[index]) + 1) / 2);
                VerifyOrExit(static_cast<size_t>(length) <= OT_STEERING_DATA_MAX_LENGTH, error = OT_ERROR_NO_BUFS);
                VerifyOrExit(
                    Interpreter::Hex2Bin(argv[index], dataset.mSteeringData.m8, static_cast<uint16_t>(length)) >= 0,
                    error = OT_ERROR_PARSE);
                dataset.mSteeringData.mLength = static_cast<uint8_t>(length);
                length                        = 0;
            }
            else if (strcmp(argv[index], "joinerudpport") == 0)
            {
                VerifyOrExit(++index < argc, error = OT_ERROR_INVALID_ARGS);
                dataset.mIsJoinerUdpPortSet = true;
                SuccessOrExit(error = Interpreter::ParseLong(argv[index], value));
                dataset.mJoinerUdpPort = static_cast<uint16_t>(value);
            }
            else if (strcmp(argv[index], "binary") == 0)
            {
                VerifyOrExit(++index < argc, error = OT_ERROR_INVALID_ARGS);
                length = static_cast<int>((strlen(argv[index]) + 1) / 2);
                VerifyOrExit(static_cast<size_t>(length) <= sizeof(tlvs), error = OT_ERROR_NO_BUFS);
                VerifyOrExit(Interpreter::Hex2Bin(argv[index], tlvs, static_cast<uint16_t>(length)) >= 0,
                             error = OT_ERROR_PARSE);
            }
            else
            {
                ExitNow(error = OT_ERROR_INVALID_ARGS);
            }
        }

        SuccessOrExit(error = otCommissionerSendMgmtSet(mInstance, &dataset, tlvs, static_cast<uint8_t>(length)));
    }
    else if (strcmp(argv[0], "sessionid") == 0)
    {
        mServer->OutputFormat("%d\r\n", otCommissionerGetSessionId(mInstance));
    }
    else
    {
        ExitNow(error = OT_ERROR_INVALID_ARGS);
    }

exit:
    AppendResult(error);
}

void OTCALL Interpreter::s_HandleEnergyReport(uint32_t       aChannelMask,
                                              const uint8_t *aEnergyList,
                                              uint8_t        aEnergyListLength,
                                              void *         aContext)
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

    VerifyOrExit(argc > 0, error = OT_ERROR_INVALID_ARGS);

    if (strcmp(argv[0], "start") == 0)
    {
        const char *provisioningUrl;
        VerifyOrExit(argc > 1, error = OT_ERROR_INVALID_ARGS);
        provisioningUrl = (argc > 2) ? argv[2] : NULL;
        otJoinerStart(mInstance, argv[1], provisioningUrl, PACKAGE_NAME, OPENTHREAD_CONFIG_PLATFORM_INFO,
                      PACKAGE_VERSION, NULL, &Interpreter::s_HandleJoinerCallback, this);
    }
    else if (strcmp(argv[0], "stop") == 0)
    {
        otJoinerStop(mInstance);
    }
    else
    {
        ExitNow(error = OT_ERROR_INVALID_ARGS);
    }

exit:
    AppendResult(error);
}

void Interpreter::ProcessJoinerId(int argc, char *argv[])
{
    OT_UNUSED_VARIABLE(argv);

    otError      error = OT_ERROR_NONE;
    otExtAddress joinerId;

    VerifyOrExit(argc == 0, error = OT_ERROR_INVALID_ARGS);

    otJoinerGetId(mInstance, &joinerId);
    OutputBytes(joinerId.m8, sizeof(joinerId));
    mServer->OutputFormat("\r\n");

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
    long    value;

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

#if OPENTHREAD_ENABLE_MAC_FILTER
void Interpreter::ProcessMacFilter(int argc, char *argv[])
{
    otError error = OT_ERROR_NONE;

    if (argc == 0)
    {
        PrintMacFilter();
    }
    else
    {
        if (strcmp(argv[0], "addr") == 0)
        {
            error = ProcessMacFilterAddress(argc - 1, argv + 1);
        }

#ifndef OTDLL
        else if (strcmp(argv[0], "rss") == 0)
        {
            error = ProcessMacFilterRss(argc - 1, argv + 1);
        }

#endif
        else
        {
            error = OT_ERROR_INVALID_ARGS;
        }
    }

    AppendResult(error);
}

void Interpreter::PrintMacFilter(void)
{
    otMacFilterEntry       entry;
    otMacFilterIterator    iterator = OT_MAC_FILTER_ITERATOR_INIT;
    otMacFilterAddressMode mode     = otLinkFilterGetAddressMode(mInstance);

    if (mode == OT_MAC_FILTER_ADDRESS_MODE_DISABLED)
    {
        mServer->OutputFormat("Address Mode: Disabled\r\n");
    }
    else if (mode == OT_MAC_FILTER_ADDRESS_MODE_WHITELIST)
    {
        mServer->OutputFormat("Address Mode: Whitelist\r\n");
    }
    else if (mode == OT_MAC_FILTER_ADDRESS_MODE_BLACKLIST)
    {
        mServer->OutputFormat("Address Mode: Blacklist\r\n");
    }

    while (otLinkFilterGetNextAddress(mInstance, &iterator, &entry) == OT_ERROR_NONE)
    {
        OutputBytes(entry.mExtAddress.m8, OT_EXT_ADDRESS_SIZE);

        if (entry.mRssIn != OT_MAC_FILTER_FIXED_RSS_DISABLED)
        {
#ifndef OTDLL

            mServer->OutputFormat(" : rss %d (lqi %d)", entry.mRssIn,
                                  otLinkConvertRssToLinkQuality(mInstance, entry.mRssIn));

#else

            mServer->OutputFormat(" : rss %d", entry.mRssIn);

#endif // OTDLL
        }

        mServer->OutputFormat("\r\n");
    }

#ifndef OTDLL

    iterator = OT_MAC_FILTER_ITERATOR_INIT;
    mServer->OutputFormat("RssIn List:\r\n");

    while (otLinkFilterGetNextRssIn(mInstance, &iterator, &entry) == OT_ERROR_NONE)
    {
        uint8_t i = 0;

        for (; i < OT_EXT_ADDRESS_SIZE; i++)
        {
            if (entry.mExtAddress.m8[i] != 0xff)
            {
                break;
            }
        }

        if (i == OT_EXT_ADDRESS_SIZE)
        {
            mServer->OutputFormat("Default rss : %d (lqi %d)\r\n", entry.mRssIn,
                                  otLinkConvertRssToLinkQuality(mInstance, entry.mRssIn));
        }
        else
        {
            OutputBytes(entry.mExtAddress.m8, OT_EXT_ADDRESS_SIZE);
            mServer->OutputFormat(" : rss %d (lqi %d)\r\n", entry.mRssIn,
                                  otLinkConvertRssToLinkQuality(mInstance, entry.mRssIn));
        }
    }

#endif // OTDLL
}

otError Interpreter::ProcessMacFilterAddress(int argc, char *argv[])
{
    otError                error = OT_ERROR_NONE;
    otExtAddress           extAddr;
    otMacFilterEntry       entry;
    otMacFilterIterator    iterator = OT_MAC_FILTER_ITERATOR_INIT;
    otMacFilterAddressMode mode     = otLinkFilterGetAddressMode(mInstance);
    long                   value;

    if (argc == 0)
    {
        if (mode == OT_MAC_FILTER_ADDRESS_MODE_DISABLED)
        {
            mServer->OutputFormat("Disabled\r\n");
        }
        else if (mode == OT_MAC_FILTER_ADDRESS_MODE_WHITELIST)
        {
            mServer->OutputFormat("Whitelist\r\n");
        }
        else if (mode == OT_MAC_FILTER_ADDRESS_MODE_BLACKLIST)
        {
            mServer->OutputFormat("Blacklist\r\n");
        }

        while (otLinkFilterGetNextAddress(mInstance, &iterator, &entry) == OT_ERROR_NONE)
        {
            OutputBytes(entry.mExtAddress.m8, OT_EXT_ADDRESS_SIZE);

            if (entry.mRssIn != OT_MAC_FILTER_FIXED_RSS_DISABLED)
            {
#ifndef OTDLL

                mServer->OutputFormat(" : rss %d (lqi %d)", entry.mRssIn,
                                      otLinkConvertRssToLinkQuality(mInstance, entry.mRssIn));

#else

                mServer->OutputFormat(" : rss %d", entry.mRssIn);

#endif // OTDLL
            }

            mServer->OutputFormat("\r\n");
        }
    }
    else
    {
        if (strcmp(argv[0], "disable") == 0)
        {
            VerifyOrExit(argc == 1, error = OT_ERROR_INVALID_ARGS);
            SuccessOrExit(error = otLinkFilterSetAddressMode(mInstance, OT_MAC_FILTER_ADDRESS_MODE_DISABLED));
        }
        else if (strcmp(argv[0], "whitelist") == 0)
        {
            VerifyOrExit(argc == 1, error = OT_ERROR_INVALID_ARGS);
            SuccessOrExit(error = otLinkFilterSetAddressMode(mInstance, OT_MAC_FILTER_ADDRESS_MODE_WHITELIST));
        }
        else if (strcmp(argv[0], "blacklist") == 0)
        {
            VerifyOrExit(argc == 1, error = OT_ERROR_INVALID_ARGS);
            SuccessOrExit(error = otLinkFilterSetAddressMode(mInstance, OT_MAC_FILTER_ADDRESS_MODE_BLACKLIST));
        }
        else if (strcmp(argv[0], "add") == 0)
        {
            VerifyOrExit(argc >= 2, error = OT_ERROR_INVALID_ARGS);
            VerifyOrExit(Hex2Bin(argv[1], extAddr.m8, OT_EXT_ADDRESS_SIZE) == OT_EXT_ADDRESS_SIZE,
                         error = OT_ERROR_PARSE);

            error = otLinkFilterAddAddress(mInstance, &extAddr);

            VerifyOrExit(error == OT_ERROR_NONE || error == OT_ERROR_ALREADY);

            if (argc > 2)
            {
                int8_t rss = 0;
                VerifyOrExit(argc == 3, error = OT_ERROR_INVALID_ARGS);
                SuccessOrExit(error = ParseLong(argv[2], value));
                rss = static_cast<int8_t>(value);
                SuccessOrExit(error = otLinkFilterAddRssIn(mInstance, &extAddr, rss));
            }
        }
        else if (strcmp(argv[0], "remove") == 0)
        {
            VerifyOrExit(argc == 2, error = OT_ERROR_INVALID_ARGS);
            VerifyOrExit(Hex2Bin(argv[1], extAddr.m8, OT_EXT_ADDRESS_SIZE) == OT_EXT_ADDRESS_SIZE,
                         error = OT_ERROR_PARSE);
            SuccessOrExit(error = otLinkFilterRemoveAddress(mInstance, &extAddr));
        }
        else if (strcmp(argv[0], "clear") == 0)
        {
            VerifyOrExit(argc == 1, error = OT_ERROR_INVALID_ARGS);
            otLinkFilterClearAddresses(mInstance);
        }
        else
        {
            error = OT_ERROR_INVALID_ARGS;
        }
    }

exit:
    return error;
}

#ifndef OTDLL

otError Interpreter::ProcessMacFilterRss(int argc, char *argv[])
{
    otError             error = OT_ERROR_NONE;
    otMacFilterEntry    entry;
    otMacFilterIterator iterator = OT_MAC_FILTER_ITERATOR_INIT;
    otExtAddress        extAddr;
    long                value;
    int8_t              rss;

    if (argc == 0)
    {
        while (otLinkFilterGetNextRssIn(mInstance, &iterator, &entry) == OT_ERROR_NONE)
        {
            uint8_t i = 0;

            for (; i < OT_EXT_ADDRESS_SIZE; i++)
            {
                if (entry.mExtAddress.m8[i] != 0xff)
                {
                    break;
                }
            }

            if (i == OT_EXT_ADDRESS_SIZE)
            {
                mServer->OutputFormat("Default rss: %d (lqi %d)\r\n", entry.mRssIn,
                                      otLinkConvertRssToLinkQuality(mInstance, entry.mRssIn));
            }
            else
            {
                OutputBytes(entry.mExtAddress.m8, OT_EXT_ADDRESS_SIZE);
                mServer->OutputFormat(" : rss %d (lqi %d)\r\n", entry.mRssIn,
                                      otLinkConvertRssToLinkQuality(mInstance, entry.mRssIn));
            }
        }
    }
    else
    {
        if (strcmp(argv[0], "add-lqi") == 0)
        {
            uint8_t linkquality = 0;
            VerifyOrExit(argc == 3, error = OT_ERROR_INVALID_ARGS);
            SuccessOrExit(error = ParseLong(argv[2], value));
            linkquality = static_cast<uint8_t>(value);
            VerifyOrExit(linkquality <= 3, error = OT_ERROR_PARSE);
            rss = otLinkConvertLinkQualityToRss(mInstance, linkquality);

            if (strcmp(argv[1], "*") == 0)
            {
                SuccessOrExit(error = otLinkFilterAddRssIn(mInstance, NULL, rss));
            }
            else
            {
                VerifyOrExit(Hex2Bin(argv[1], extAddr.m8, OT_EXT_ADDRESS_SIZE) == OT_EXT_ADDRESS_SIZE,
                             error = OT_ERROR_PARSE);

                SuccessOrExit(error = otLinkFilterAddRssIn(mInstance, &extAddr, rss));
            }
        }
        else if (strcmp(argv[0], "add") == 0)
        {
            VerifyOrExit(argc == 3, error = OT_ERROR_INVALID_ARGS);
            SuccessOrExit(error = ParseLong(argv[2], value));
            rss = static_cast<int8_t>(value);

            if (strcmp(argv[1], "*") == 0)
            {
                SuccessOrExit(error = otLinkFilterAddRssIn(mInstance, NULL, rss));
            }
            else
            {
                VerifyOrExit(Hex2Bin(argv[1], extAddr.m8, OT_EXT_ADDRESS_SIZE) == OT_EXT_ADDRESS_SIZE,
                             error = OT_ERROR_PARSE);

                SuccessOrExit(error = otLinkFilterAddRssIn(mInstance, &extAddr, rss));
            }
        }
        else if (strcmp(argv[0], "remove") == 0)
        {
            VerifyOrExit(argc == 2, error = OT_ERROR_INVALID_ARGS);

            if (strcmp(argv[1], "*") == 0)
            {
                SuccessOrExit(error = otLinkFilterRemoveRssIn(mInstance, NULL));
            }
            else
            {
                VerifyOrExit(Hex2Bin(argv[1], extAddr.m8, OT_EXT_ADDRESS_SIZE) == OT_EXT_ADDRESS_SIZE,
                             error = OT_ERROR_PARSE);

                SuccessOrExit(error = otLinkFilterRemoveRssIn(mInstance, &extAddr));
            }
        }
        else if (strcmp(argv[0], "clear") == 0)
        {
            otLinkFilterClearRssIn(mInstance);
        }
        else
        {
            error = OT_ERROR_INVALID_ARGS;
        }
    }

exit:
    return error;
}

#endif // OTDLL

#endif // OPENTHREAD_ENABLE_MAC_FILTER

#if OPENTHREAD_ENABLE_DIAG
void Interpreter::ProcessDiag(int argc, char *argv[])
{
    char output[OPENTHREAD_CONFIG_DIAG_OUTPUT_BUFFER_SIZE];

    // all diagnostics related features are processed within diagnostics module
    output[sizeof(output) - 1] = '\0';
    otDiagProcessCmd(argc, argv, output, sizeof(output) - 1);
    mServer->Output(output, static_cast<uint16_t>(strlen(output)));
}
#endif

void Interpreter::ProcessLine(char *aBuf, uint16_t aBufLength, Server &aServer)
{
    char *  argv[kMaxArgs] = {NULL};
    char *  cmd;
    uint8_t argc = 0, i = 0;

    mServer = &aServer;

    VerifyOrExit(aBuf != NULL && strnlen(aBuf, aBufLength + 1) <= aBufLength);

    VerifyOrExit(Utils::CmdLineParser::ParseCmd(aBuf, argc, argv, kMaxArgs) == OT_ERROR_NONE,
                 mServer->OutputFormat("Error: too many args (max %d)\r\n", kMaxArgs));
    VerifyOrExit(argc >= 1, mServer->OutputFormat("Error: no given command.\r\n"));

    cmd = argv[0];

#if OPENTHREAD_ENABLE_DIAG
    VerifyOrExit(
        (!otDiagIsEnabled() || (strcmp(cmd, "diag") == 0)),
        mServer->OutputFormat("under diagnostics mode, execute 'diag stop' before running any other commands.\r\n"));
#endif

    for (i = 0; i < OT_ARRAY_LENGTH(sCommands); i++)
    {
        if (strcmp(cmd, sCommands[i].mName) == 0)
        {
            (this->*sCommands[i].mCommand)(argc - 1, &argv[1]);
            break;
        }
    }

    // Check user defined commands if built-in command
    // has not been found
    if (i == OT_ARRAY_LENGTH(sCommands))
    {
        for (i = 0; i < mUserCommandsLength; i++)
        {
            if (strcmp(cmd, mUserCommands[i].mName) == 0)
            {
                mUserCommands[i].mCommand(argc - 1, &argv[1]);
                break;
            }
        }

        if (i == mUserCommandsLength)
        {
            AppendResult(OT_ERROR_PARSE);
        }
    }

exit:
    return;
}

#if OPENTHREAD_FTD || OPENTHREAD_ENABLE_MTD_NETWORK_DIAGNOSTIC
void Interpreter::ProcessNetworkDiagnostic(int argc, char *argv[])
{
    otError             error = OT_ERROR_NONE;
    struct otIp6Address address;
    uint8_t             tlvTypes[OT_NETWORK_DIAGNOSTIC_TYPELIST_MAX_ENTRIES];
    uint8_t             count     = 0;
    uint8_t             argvIndex = 0;

    // Include operation, address and type tlv list.
    VerifyOrExit(argc > 2, error = OT_ERROR_INVALID_ARGS);

    SuccessOrExit(error = otIp6AddressFromString(argv[1], &address));

    argvIndex = 2;

    while (argvIndex < argc && count < sizeof(tlvTypes))
    {
        long value;
        SuccessOrExit(error = ParseLong(argv[argvIndex++], value));
        tlvTypes[count++] = static_cast<uint8_t>(value);
    }

    if (strcmp(argv[0], "get") == 0)
    {
        otThreadSendDiagnosticGet(mInstance, &address, tlvTypes, count);
        ExitNow();
    }
    else if (strcmp(argv[0], "reset") == 0)
    {
        otThreadSendDiagnosticReset(mInstance, &address, tlvTypes, count);
        AppendResult(OT_ERROR_NONE);
    }
    else
    {
        ExitNow(error = OT_ERROR_INVALID_ARGS);
    }

exit:
    if (error != OT_ERROR_NONE)
    {
        AppendResult(error);
    }
}
#endif // OPENTHREAD_FTD || OPENTHREAD_ENABLE_MTD_NETWORK_DIAGNOSTIC

#ifndef OTDLL
void Interpreter::s_HandleDiagnosticGetResponse(otMessage *aMessage, const otMessageInfo *aMessageInfo, void *aContext)
{
    static_cast<Interpreter *>(aContext)->HandleDiagnosticGetResponse(
        *static_cast<Message *>(aMessage), *static_cast<const Ip6::MessageInfo *>(aMessageInfo));
}

void Interpreter::HandleDiagnosticGetResponse(Message &aMessage, const Ip6::MessageInfo &)
{
    uint8_t  buf[16];
    uint16_t bytesToPrint;
    uint16_t bytesPrinted = 0;
    uint16_t length       = aMessage.GetLength() - aMessage.GetOffset();

    mServer->OutputFormat("DIAG_GET.rsp/ans: ");

    while (length > 0)
    {
        bytesToPrint = (length < sizeof(buf)) ? length : sizeof(buf);
        aMessage.Read(aMessage.GetOffset() + bytesPrinted, bytesToPrint, buf);

        OutputBytes(buf, static_cast<uint8_t>(bytesToPrint));

        length -= bytesToPrint;
        bytesPrinted += bytesToPrint;
    }

    mServer->OutputFormat("\r\n");
}
#endif

void Interpreter::SetUserCommands(const otCliCommand *aCommands, uint8_t aLength)
{
    mUserCommands       = aCommands;
    mUserCommandsLength = aLength;
}

Interpreter &Interpreter::GetOwner(OwnerLocator &aOwnerLocator)
{
#if OPENTHREAD_ENABLE_MULTIPLE_INSTANCES
    Interpreter &interpreter = (aOwnerLocator.GetOwner<Interpreter>());
#else
    OT_UNUSED_VARIABLE(aOwnerLocator);

    Interpreter &interpreter = Server::sServer->GetInterpreter();
#endif
    return interpreter;
}

extern "C" void otCliSetUserCommands(const otCliCommand *aUserCommands, uint8_t aLength)
{
    Server::sServer->GetInterpreter().SetUserCommands(aUserCommands, aLength);
}

extern "C" void otCliOutputBytes(const uint8_t *aBytes, uint8_t aLength)
{
    Server::sServer->GetInterpreter().OutputBytes(aBytes, aLength);
}

extern "C" void otCliOutputFormat(const char *aFmt, ...)
{
    va_list aAp;
    va_start(aAp, aFmt);
    Server::sServer->OutputFormatV(aFmt, aAp);
    va_end(aAp);
}

extern "C" void otCliOutput(const char *aString, uint16_t aLength)
{
    Server::sServer->Output(aString, aLength);
}

extern "C" void otCliAppendResult(otError aError)
{
    Server::sServer->GetInterpreter().AppendResult(aError);
}

extern "C" void otCliPlatLogv(otLogLevel aLogLevel, otLogRegion aLogRegion, const char *aFormat, va_list aArgs)
{
    OT_UNUSED_VARIABLE(aLogLevel);
    OT_UNUSED_VARIABLE(aLogRegion);

    VerifyOrExit(Server::sServer != NULL);

    Server::sServer->OutputFormatV(aFormat, aArgs);
    Server::sServer->OutputFormat("\r\n");

exit:
    return;
}

} // namespace Cli
} // namespace ot
