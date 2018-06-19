/*
 *  Copyright (c) 2018, The OpenThread Authors.
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
 *   This file implements a simple CLI for latency test.
 */

#include "cli_latency.hpp"

#include <openthread/message.h>
#include <openthread/udp.h>

#include "cli/cli.hpp"
#include "cli_uart.hpp"
#include "openthread/platform/gpio.h"
#include "common/encoding.hpp"

#define MONITOR_PIN 3
#define CLEAR_PIN_INTERVAL 2

using ot::Encoding::BigEndian::HostSwap16;

namespace ot {
namespace Cli {

LatencyTest *LatencyTest::sLatency;
uint16_t LatencyTest::sCount = 0;
uint32_t LatencyTest::sSendTimestamp[MAX_PACKET_NUM];

const struct LatencyTest::Command LatencyTest::sCommands[] =
{
    { "help", &LatencyTest::ProcessHelp },
    { "bind", &LatencyTest::ProcessBind },
    { "close", &LatencyTest::ProcessClose },
    { "connect", &LatencyTest::ProcessConnect },
    { "open",  &LatencyTest::ProcessOpen },
    { "start", &LatencyTest::ProcessStart },
    { "test", &LatencyTest::ProcessTest },
    { "result", &LatencyTest::ProcessResult },
    { "gpio", &LatencyTest::ProcessGpio }
};

otError LatencyTest::ProcessHelp(int argc, char *argv[])
{
    for (unsigned int i = 0; i < sizeof(sCommands) / sizeof(sCommands[0]); i++)
    {
        mInterpreter.mServer->OutputFormat("%s\r\n", sCommands[i].mName);
    }

    OT_UNUSED_VARIABLE(argc);
    OT_UNUSED_VARIABLE(argv);

    return OT_ERROR_NONE;
}

LatencyTest::LatencyTest(Interpreter &aInterpreter):
    mInterpreter(aInterpreter),
    mLength(1232),
    mCount(0),
    mInterval(1),
    mPingTimer(*aInterpreter.mInstance, &LatencyTest::s_HandlePingTimer, this),
    mGpioTimer(*aInterpreter.mInstance, &LatencyTest::s_HandleGpioTimer, this)
{
}

void LatencyTest::Init(void)
{
    sCount = 0;
    mLatency = 0;
    mTimeElapse = 0;
    mJitter = 0;
    mAcceptTimestamp = 0;
    mIsRun = true;

    otPlatGpioWrite(MONITOR_PIN, 0);
    otPlatGpioEnableInterrupt(MONITOR_PIN, &LatencyTest::platGpioResponse, this);

    memset(sSendTimestamp, 0, sizeof(sSendTimestamp));
    memset(mReceiveTimer, 0, sizeof(mReceiveTimer));
}

otError LatencyTest::SendUdpPacket(void)
{
    otError error;
    uint32_t timestamp = 0;
    otMessage *message;

    message = otUdpNewMessage(mInterpreter.mInstance, true);
    VerifyOrExit(message != NULL, error = OT_ERROR_NO_BUFS);

    memset(mPayload, 0, sizeof(mPayload));
    timestamp = TimerMilli::GetNow();
    
    mPayload[0] = timestamp >> 24;
    mPayload[1] = timestamp >> 16;
    mPayload[2] = timestamp >> 8;
    mPayload[3] = timestamp;
    mPayload[4] = LatencyTest::sCount >> 24;
    mPayload[5] = LatencyTest::sCount >> 16;
    mPayload[6] = LatencyTest::sCount >> 8;
    mPayload[7] = LatencyTest::sCount;

    for (uint16_t i = 8; i < mLength; i++)
    {
        mPayload[i] = 'T';
    }

    error = otMessageAppend(message, mPayload, mLength);

    SuccessOrExit(error);

    error = otUdpSend(&mSocket, message, &mMessageInfo);

    SuccessOrExit(error);
    otPlatGpioWrite(MONITOR_PIN, 1);
    mGpioTimer.Start(CLEAR_PIN_INTERVAL);

    LatencyTest::sCount++;

exit:

    if (error != OT_ERROR_NONE && message != NULL)
    {
        otMessageFree(message);
    }

    return error;

}

uint32_t LatencyTest::GetAcceptedTimestamp(otMessage *aMessage)
{
    uint8_t buf[1500];
    int length;
    uint32_t timestamp;

    length = otMessageRead(aMessage, otMessageGetOffset(aMessage), buf, sizeof(buf) - 1);
    buf[length] = '\0';
    timestamp = buf[0] << 24 | buf[1] << 16 | buf[2] << 8 | buf[3];
    return timestamp;
}

uint32_t LatencyTest::GetAcceptedCount(otMessage *aMessage)
{
    uint8_t buf[1500];
    int length;
    uint32_t count;

    length = otMessageRead(aMessage, otMessageGetOffset(aMessage), buf, sizeof(buf) - 1);
    buf[length] = '\0';
    count = buf[4] << 24 | buf[5] << 16 | buf[6] << 8 | buf[7];
    return count;
}

otError LatencyTest::ProcessBind(int argc, char *argv[])
{
    otError error;
    otSockAddr sockaddr;
    long value;

    VerifyOrExit(argc == 2, error = OT_ERROR_PARSE);

    memset(&sockaddr, 0, sizeof(sockaddr));

    error = otIp6AddressFromString(argv[0], &sockaddr.mAddress);
    SuccessOrExit(error);

    error = Interpreter::ParseLong(argv[1], value);
    SuccessOrExit(error);

    sockaddr.mPort = static_cast<uint16_t>(value);

    error = otUdpBind(&mSocket, &sockaddr);

exit:
    return error;
}

otError LatencyTest::ProcessConnect(int argc, char *argv[])
{
    otError error;
    otSockAddr sockaddr;
    long value;

    VerifyOrExit(argc == 2, error = OT_ERROR_PARSE);

    memset(&sockaddr, 0, sizeof(sockaddr));

    error = otIp6AddressFromString(argv[0], &sockaddr.mAddress);
    SuccessOrExit(error);

    error = Interpreter::ParseLong(argv[1], value);
    SuccessOrExit(error);

    sockaddr.mPort = static_cast<uint16_t>(value);

    error = otUdpConnect(&mSocket, &sockaddr);

exit:
    return error;
}

otError LatencyTest::ProcessClose(int argc, char *argv[])
{
    mIsRun = false;
    OT_UNUSED_VARIABLE(argc);
    OT_UNUSED_VARIABLE(argv);
    
    return otUdpClose(&mSocket);
}

otError LatencyTest::ProcessStart(int argc, char *argv[])
{
    otError error = OT_ERROR_NONE;
    OT_UNUSED_VARIABLE(argc);
    OT_UNUSED_VARIABLE(argv);
    Init();

    return error;
}

otError LatencyTest::ProcessOpen(int argc, char *argv[])
{
    OT_UNUSED_VARIABLE(argc);
    OT_UNUSED_VARIABLE(argv);
    Init();
    return otUdpOpen(mInterpreter.mInstance, &mSocket, HandleUdpReceive, this);
}

otError LatencyTest::Process(int argc, char *argv[])
{
    otError error = OT_ERROR_PARSE;

    for (size_t i = 0; i < sizeof(sCommands) / sizeof(sCommands[0]); i++)
    {
        if (strcmp(argv[0], sCommands[i].mName) == 0)
        {
            error = (this->*sCommands[i].mCommand)(argc - 1, argv + 1);
            break;
        }
    }

    return error;
}

void LatencyTest::HandleUdpReceive(void *aContext, otMessage *aMessage, const otMessageInfo *aMessageInfo)
{
    static_cast<LatencyTest *>(aContext)->HandleUdpReceive(aMessage, aMessageInfo);
}

void LatencyTest::HandleUdpReceive(otMessage *aMessage, const otMessageInfo *aMessageInfo)
{
    otMessageInfo messageInfo;
    otMessage *message;
    uint32_t timestamp = 0;

    uint16_t count;

    timestamp = TimerMilli::GetNow();

    memset(&messageInfo, 0, sizeof(messageInfo));
    memset(&message, 0, sizeof(message));
    
    //Get sequence number from the packet
    count = GetAcceptedCount(aMessage);

    if (mReceiveTimer[count] == 0)
        mReceiveTimer[count] = timestamp;
    LatencyTest::sCount = count + 1;

    mInterpreter.mServer->OutputFormat("hoplimit %d, amount %d, %u, %d, %d, %u, %u from ", aMessageInfo->mHopLimit, mTotalCount, timestamp, count, otMessageGetLength(aMessage) - otMessageGetOffset(aMessage), mTimeElapse, mJitter);
    mInterpreter.mServer->OutputFormat("%x:%x:%x:%x:%x:%x:%x:%x %d \r\n",
                                       HostSwap16(aMessageInfo->mPeerAddr.mFields.m16[0]),
                                       HostSwap16(aMessageInfo->mPeerAddr.mFields.m16[1]),
                                       HostSwap16(aMessageInfo->mPeerAddr.mFields.m16[2]),
                                       HostSwap16(aMessageInfo->mPeerAddr.mFields.m16[3]),
                                       HostSwap16(aMessageInfo->mPeerAddr.mFields.m16[4]),
                                       HostSwap16(aMessageInfo->mPeerAddr.mFields.m16[5]),
                                       HostSwap16(aMessageInfo->mPeerAddr.mFields.m16[6]),
                                       HostSwap16(aMessageInfo->mPeerAddr.mFields.m16[7]),
                                       aMessageInfo->mPeerPort);
}

otError LatencyTest::ProcessResult(int argc, char *argv[])
{
    OT_UNUSED_VARIABLE(argc);
    OT_UNUSED_VARIABLE(argv);

    for (int i = 1; i < LatencyTest::sCount + 1; i++)
    {
        if (sSendTimestamp[i] == 0)
            break;
        mInterpreter.mServer->OutputFormat("%d, %d, %d \r\n", LatencyTest::sSendTimestamp[i], mReceiveTimer[i], i);
    }

    return OT_ERROR_NONE;
} 


otError LatencyTest::ProcessGpio(int argc, char *argv[])
{
    OT_UNUSED_VARIABLE(argc);
    OT_UNUSED_VARIABLE(argv);

    return OT_ERROR_NONE;

}

LatencyTest &LatencyTest::GetOwner(OwnerLocator &aOwnerLocator)
{
#if OPENTHREAD_ENABLE_MULTIPLE_INSTANCES
    LatencyTest &udp = (aOwnerLocator.GetOwner<LatencyTest>());
#else
    LatencyTest &udp = Uart::sUartServer->GetInterpreter().mLatency;
    OT_UNUSED_VARIABLE(aOwnerLocator);
#endif
    return udp;
}

void LatencyTest::s_HandlePingTimer(Timer &aTimer)
{
    GetOwner(aTimer).HandlePingTimer();
}

void LatencyTest::s_HandleGpioTimer(Timer &aTimer)
{
    GetOwner(aTimer).HandleGpioTimer();
}

void LatencyTest::HandleGpioTimer()
{
    otPlatGpioWrite(MONITOR_PIN, 0);
}

void LatencyTest::HandlePingTimer()
{
    otError error = OT_ERROR_NONE;
    uint32_t interval = 0;

    if (mIsRun)
    {
        error = SendUdpPacket();
        SuccessOrExit(error);
    }

exit:
    if (error == OT_ERROR_NONE)
    {
        if (LatencyTest::sCount <= mTotalCount)
        {
            if(mInterval == 0)
            {
                interval = otPlatRandomGet() % 100 + 500;
            }
            else
            {
                interval = mInterval;
            }
            mPingTimer.Start(interval);
        }
        else
        {
            Init();
        }   
    }
    else
    {
        if(mInterval == 0)
        {
            interval = otPlatRandomGet() % 100 + 500;
        }
        else
        {
            interval = 50;
        }
        mPingTimer.Start(mInterval);
    }

}

otError LatencyTest::ProcessTest(int argc, char *argv[])
{
    otError error = OT_ERROR_NONE;
    int curArg = 0;
    long value;

    memset(&mMessageInfo, 0, sizeof(mMessageInfo));

    VerifyOrExit(argc == 5, error = OT_ERROR_PARSE);

    error = otIp6AddressFromString(argv[curArg++], &mMessageInfo.mPeerAddr);
    SuccessOrExit(error);

    error = Interpreter::ParseLong(argv[curArg++], value);
    SuccessOrExit(error);

    mMessageInfo.mPeerPort = static_cast<uint16_t>(value);

    error = Interpreter::ParseLong(argv[curArg++], value);
    SuccessOrExit(error);

    mLength = value;

    error = Interpreter::ParseLong(argv[curArg++], value);
    SuccessOrExit(error);
   
    mTotalCount = value;

    error = Interpreter::ParseLong(argv[curArg++], value);
    SuccessOrExit(error);

    mInterval = value;

    mMessageInfo.mInterfaceId = OT_NETIF_INTERFACE_ID_THREAD;

    //disable the monitor pin due to the pin is set to output
    otPlatGpioDisableInterrupt(MONITOR_PIN);

    //set the pin to output pin
    otPlatGpioCfgOutput(MONITOR_PIN);

    // set the gpio to low level
    otPlatGpioWrite(MONITOR_PIN, 0);

    HandlePingTimer();

exit:

    return error;
}

void LatencyTest::platGpioResponse(void *aContext)
{
    OT_UNUSED_VARIABLE(aContext);

    LatencyTest::sSendTimestamp[LatencyTest::sCount] = TimerMilli::GetNow();
    LatencyTest::sCount++;
}

}  // namespace Cli
}  // namespace ot
