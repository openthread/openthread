/*
 *  Copyright (c) 2019, The OpenThread Authors.
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
 *   This file implements a network performance tool.
 */

#include <openthread/coap.h>
#include <openthread/message.h>
#include <openthread/udp.h>
#if OPENTHREAD_CONFIG_TIME_SYNC_ENABLE
#include <openthread/network_time.h>
#endif

#include "cli/cli.hpp"
#include "cli/cli_perf.hpp"
#include "cli/cli_uart.hpp"

#include "common/encoding.hpp"
#include "common/new.hpp"
#include "common/random.hpp"
#include "net/ip6_address.hpp"

#if OPENTHREAD_CONFIG_CLI_PERF_ENABLE

using ot::Encoding::BigEndian::HostSwap16;
using ot::Encoding::BigEndian::HostSwap32;

namespace ot {
namespace Cli {

const struct Perf::Command Perf::sCommands[] = {{"help", &Perf::ProcessHelp},     {"client", &Perf::ProcessClient},
                                                {"server", &Perf::ProcessServer}, {"start", &Perf::ProcessStart},
                                                {"stop", &Perf::ProcessStop},     {"show", &Perf::ProcessShow},
                                                {"clear", &Perf::ProcessClear}};

Session::Session(Perf &aPerf, const Setting &aSetting, uint8_t aRole)
    : mPerf(&aPerf)
    , mSetting(&aSetting)
    , mRole(aRole)
    , mState(kStateIdle)
{
    InitStats();
}

void Session::InitStats(void)
{
    memset(&mStats, 0, sizeof(mStats));
    mStats.mCurMinLatency   = UINT32_MAX;
    mStats.mTotalMinLatency = UINT32_MAX;
}

void Session::InitCurrentStats(void)
{
    mStats.mCurBytes         = 0;
    mStats.mCurCntError      = 0;
    mStats.mCurCntDatagram   = 0;
    mStats.mCurCntOutOfOrder = 0;
    mStats.mCurMinLatency    = UINT32_MAX;
    mStats.mCurMaxLatency    = 0;
    mStats.mCurLatency       = 0;
}

otError Session::OpenSocket(void)
{
    otError    error       = OT_ERROR_NONE;
    bool       closeSocket = false;
    otSockAddr sockaddr;

    VerifyOrExit(!mSocketValid, error = OT_ERROR_ALREADY);
    SuccessOrExit(error = otUdpOpen(mPerf->mInstance, &mSocket, Perf::s_HandleUdpReceive, GetContext()));

    SetContext(*mPerf, *this);

    switch (mRole)
    {
    case kRoleClient:
        memcpy(sockaddr.mAddress.mFields.m8, mPeerAddr.mFields.m8, sizeof(otIp6Address));
        sockaddr.mPort = mPeerPort;

        VerifyOrExit((error = otUdpConnect(&mSocket, &sockaddr)) == OT_ERROR_NONE, closeSocket = true);

        if (!(mSetting->IsFlagSet(Setting::kFlagFormatCvs) || mSetting->IsFlagSet(Setting::kFlagFormatQuiet)))
        {
            Server::sServer->OutputFormat(
                "Client connecting to  %x:%x:%x:%x:%x:%x:%x:%x , ", HostSwap16(sockaddr.mAddress.mFields.m16[0]),
                HostSwap16(sockaddr.mAddress.mFields.m16[1]), HostSwap16(sockaddr.mAddress.mFields.m16[2]),
                HostSwap16(sockaddr.mAddress.mFields.m16[3]), HostSwap16(sockaddr.mAddress.mFields.m16[4]),
                HostSwap16(sockaddr.mAddress.mFields.m16[5]), HostSwap16(sockaddr.mAddress.mFields.m16[6]),
                HostSwap16(sockaddr.mAddress.mFields.m16[7]));
            Server::sServer->OutputFormat("UDP port %d\n\r", sockaddr.mPort);
        }
        break;

    case kRoleListener:
        memset(sockaddr.mAddress.mFields.m8, 0, sizeof(otIp6Address));
        sockaddr.mPort = mLocalPort;

        VerifyOrExit((error = otUdpBind(&mSocket, &sockaddr)) == OT_ERROR_NONE, closeSocket = true);

        if (!(mSetting->IsFlagSet(Setting::kFlagFormatCvs) || mSetting->IsFlagSet(Setting::kFlagFormatQuiet)))
        {
            Server::sServer->OutputFormat("Server listening on UDP port %d\r\n", sockaddr.mPort);
        }
        break;

    case kRoleServer:
        memcpy(sockaddr.mAddress.mFields.m8, mLocalAddr.mFields.m8, sizeof(otIp6Address));
        sockaddr.mPort = mLocalPort;

        VerifyOrExit((error = otUdpBind(&mSocket, &sockaddr)) == OT_ERROR_NONE, closeSocket = true);
        break;

    default:
        error = OT_ERROR_NOT_IMPLEMENTED;
        break;
    }

exit:
    if (error == OT_ERROR_NONE)
    {
        mSocketValid = true;
    }
    else if (closeSocket)
    {
        otUdpClose(&mSocket);
    }

    return error;
}

void Session::CloseSocket(void)
{
    if (mReplySocketValid)
    {
        otUdpClose(&mReplySocket);
        mReplySocketValid = false;
    }

    if (mSocketValid)
    {
        otUdpClose(&mSocket);
        mSocketValid = false;
    }
}

otError Session::HandleFistMessage(otMessage &aMessage, const otMessageInfo &aMessageInfo)
{
    otError      error    = OT_ERROR_NONE;
    uint32_t     milliNow = TimerMilli::GetNow();
    otIp6Address sockAddr = aMessageInfo.mSockAddr;
    otIp6Address peerAddr = aMessageInfo.mPeerAddr;
    PerfHeader   perfHeader;
    PacketInfo   packetInfo;

    VerifyOrExit(mRole == kRoleServer, error = OT_ERROR_FAILED);
    VerifyOrExit(mState == kStateIdle, error = OT_ERROR_FAILED);

    packetInfo.mLength = otMessageGetLength(&aMessage) - otMessageGetOffset(&aMessage);
    VerifyOrExit(otMessageRead(&aMessage, otMessageGetOffset(&aMessage), &perfHeader, sizeof(PerfHeader)) ==
                     sizeof(PerfHeader),
                 error = OT_ERROR_PARSE);
    VerifyOrExit(!perfHeader.GetFinFlag(), error = OT_ERROR_PARSE);

    memcpy(mLocalAddr.mFields.m8, sockAddr.mFields.m8, sizeof(otIp6Address));
    memcpy(mPeerAddr.mFields.m8, peerAddr.mFields.m8, sizeof(otIp6Address));

    mLocalPort    = aMessageInfo.mSockPort;
    mPeerPort     = aMessageInfo.mPeerPort;
    mSessionId    = perfHeader.GetSessionId();
    mPrintEndTime = 0;

    /* Perf uses the formula "NumOfPackets*PacketLength/Interval" to calculate throughput Periodically.
     * The server in the first interval reveives one packet more than the subsequent intervals. Then the
     * calculated throughput of first interval is larger than the throughput in subsequenct intervals. To
     * resolve this issue, set the session start time ahead of one transmit interval to extend the first
     * interval to adjust throughput of the first interval.
     * This figure shows this case:
     * |<------------------1st' Interval-------------->|
     * |           |<----------1st Interval----------->|<----------2nd Interval----------->|
     * ------------1-----------2-----------3-----------4-----------5-----------6-----------7
     */
    mSessionStartTime = milliNow - perfHeader.GetSendingInterval() / USEC_PER_MS;
    mSessionEndTime   = milliNow + mSetting->GetInterval();

    SuccessOrExit(error = OpenSocket());

    GetPacketInfo(perfHeader, milliNow * USEC_PER_MS, packetInfo);
    UpdatePacketStats(packetInfo);

    if (mPerf->mPrintServerHeaderFlag == false)
    {
        mPerf->mPrintServerHeaderFlag = true;
        PrintServerReportHeader();
    }

    PrintConnection();

    if (perfHeader.GetEchoFlag() == true)
    {
        Message &         message  = *static_cast<Message *>(&aMessage);
        otMessagePriority priority = static_cast<otMessagePriority>(message.GetPriority());

        IgnoreReturnValue(SendReply(perfHeader, priority, packetInfo.mLength));
    }

    mState = kStateReceivingData;

exit:
    return error;
}

void Session::HandleSubsequentMessages(otMessage &aMessage, const otMessageInfo &aMessageInfo)
{
    uint32_t   milliNow = TimerMilli::GetNow();
    PacketInfo packetInfo;
    PerfHeader perfHeader;

    if ((mPeerPort != aMessageInfo.mPeerPort) ||
        (memcmp(mPeerAddr.mFields.m8, aMessageInfo.mPeerAddr.mFields.m8, sizeof(otIp6Address)) != 0))
    {
        return;
    }

    VerifyOrExit(mRole == kRoleServer);
    VerifyOrExit(mState == kStateReceivingData || mState == kStateReceivingFin);

    packetInfo.mLength = otMessageGetLength(&aMessage) - otMessageGetOffset(&aMessage);
    VerifyOrExit(otMessageRead(&aMessage, otMessageGetOffset(&aMessage), &perfHeader, sizeof(PerfHeader)) ==
                 sizeof(PerfHeader));

    if (perfHeader.GetFinFlag())
    {
        if (mState == kStateReceivingData)
        {
            mState    = kStateReceivingFin;
            mFireTime = mPerf->mTimer.GetNow();

            if (perfHeader.GetEchoFlag())
            {
                // Waiting to close the session after all FIN packets are sent back to the originator.
                mFireTime += UsToTimerTime(kMaxNumFin * kFinIntervalUs);
            }

            mPrintStartTime = mPrintEndTime;
            mPrintEndTime   = milliNow - mSessionStartTime - perfHeader.GetFinDelay();
            mFinCounter     = 0;

            mPerf->StartTimer();

            if (mStats.mCurCntDatagram != 0)
            {
                PrintServerReport();
            }

            PrintServerReportEnd();
        }
    }
    else
    {
        GetPacketInfo(perfHeader, milliNow * USEC_PER_MS, packetInfo);
        UpdatePacketStats(packetInfo);

        if (TimeCompare(mSessionEndTime, milliNow) <= 0)
        {
            mPrintStartTime = mPrintEndTime;
            mPrintEndTime   = milliNow - mSessionStartTime;

            while (TimeCompare(mSessionEndTime, milliNow) <= 0)
            {
                mSessionEndTime += mSetting->GetInterval();
            }

            PrintServerReport();
            InitCurrentStats();
        }
    }

    if (perfHeader.GetEchoFlag() == true)
    {
        Message &         message  = *static_cast<Message *>(&aMessage);
        otMessagePriority priority = static_cast<otMessagePriority>(message.GetPriority());

        IgnoreReturnValue(SendReply(perfHeader, priority, packetInfo.mLength));
    }

exit:
    return;
}

otError Session::SendReply(PerfHeader &aPerfHeader, otMessagePriority aPriority, uint16_t aLength)
{
    otError           error       = OT_ERROR_NONE;
    otMessage *       message     = NULL;
    otMessageSettings msgSettings = {true, aPriority};
    otMessageInfo     messageInfo;

    VerifyOrExit(aLength >= sizeof(PerfHeader), error = OT_ERROR_INVALID_ARGS);
    VerifyOrExit((message = otUdpNewMessage(mPerf->mInstance, &msgSettings)) != NULL, error = OT_ERROR_NO_BUFS);

    aPerfHeader.SetEchoFlag(false);
    aPerfHeader.SetReplyFlag(true);
    SuccessOrExit(error = otMessageAppend(message, &aPerfHeader, sizeof(PerfHeader)));
    SuccessOrExit(error = otMessageSetLength(message, aLength));

    if (!mReplySocketValid)
    {
        otSockAddr sockaddr;

        memset(&mReplySocket, 0, sizeof(otUdpSocket));
        SuccessOrExit(otUdpOpen(mPerf->mInstance, &mReplySocket, NULL, NULL));

        memcpy(sockaddr.mAddress.mFields.m8, mPeerAddr.mFields.m8, sizeof(otIp6Address));
        sockaddr.mPort = kUdpPort;

        if ((error = otUdpConnect(&mReplySocket, &sockaddr)) != OT_ERROR_NONE)
        {
            otUdpClose(&mReplySocket);
            ExitNow();
        }

        mReplySocketValid = true;
    }

    memset(&messageInfo, 0, sizeof(otMessageInfo));
    memcpy(messageInfo.mPeerAddr.mFields.m8, mPeerAddr.mFields.m8, sizeof(otIp6Address));
    messageInfo.mPeerPort = kUdpPort;

    error = otUdpSend(&mReplySocket, message, &messageInfo);

exit:
    if (error != OT_ERROR_NONE && message != NULL)
    {
        otMessageFree(message);
    }

    return error;
}

otError Session::SendData(void)
{
    otError           error       = OT_ERROR_NONE;
    uint32_t          milliNow    = TimerMilli::GetNow();
    otMessage *       message     = NULL;
    otMessageSettings msgSettings = {true, mSetting->GetPriority()};
    otMessageInfo     messageInfo;
    PerfHeader        perfHeader;
    uint64_t          microNow;

    VerifyOrExit(mState == kStateSendingData, error = OT_ERROR_INVALID_STATE);
    VerifyOrExit(mSetting->GetLength() >= sizeof(PerfHeader), error = OT_ERROR_INVALID_ARGS);

    if (GetSynchronizedTime(microNow) == OT_ERROR_NONE)
    {
        perfHeader.SetSyncFlag(true);
    }
    else
    {
        microNow = milliNow * USEC_PER_MS;
        perfHeader.SetSyncFlag(false);
    }

    perfHeader.SetSeqNumber(mSeqNumber++);
    perfHeader.SetTime(microNow);
    perfHeader.SetSessionId(mSessionId);
    perfHeader.SetSendingInterval(mSendingIntervalUs);
    perfHeader.SetEchoFlag(mSetting->IsFlagSet(Setting::kFlagEcho) ? true : false);
    perfHeader.SetFinFlag(false);

    VerifyOrExit((message = otUdpNewMessage(mPerf->mInstance, &msgSettings)) != NULL, error = OT_ERROR_NO_BUFS);
    SuccessOrExit(error = otMessageAppend(message, &perfHeader, sizeof(PerfHeader)));
    SuccessOrExit(error = otMessageSetLength(message, mSetting->GetLength()));

    memset(&messageInfo, 0, sizeof(otMessageInfo));
    memcpy(messageInfo.mPeerAddr.mFields.m8, mPeerAddr.mFields.m8, sizeof(otIp6Address));
    messageInfo.mPeerPort = mPeerPort;

    error = otUdpSend(&mSocket, message, &messageInfo);

    if ((mLocalPort == 0) && (mSocket.mSockName.mPort != 0))
    {
        memcpy(mLocalAddr.mFields.m8, mSocket.mSockName.mAddress.mFields.m8, sizeof(otIp6Address));
        mLocalPort = mSocket.mSockName.mPort;

        PrintConnection();
    }

exit:
    if (error == OT_ERROR_NONE)
    {
        mStats.mCurCntDatagram++;
        mStats.mTotalCntDatagram++;
        mStats.mCurBytes += mSetting->GetLength();
        mStats.mTotalBytes += mSetting->GetLength();
    }
    else
    {
        mStats.mCurCntError++;
        mStats.mTotalCntError++;
        if (message != NULL)
        {
            otMessageFree(message);
        }
    }

    if (TimeCompare(mSessionEndTime, milliNow) <= 0)
    {
        mPrintStartTime = mPrintEndTime;
        mPrintEndTime   = milliNow - mSessionStartTime;

        // If the throughput is very low, the sending intervel may larger than the setting interval. The session
        // end time needs to be updated untill it is larger than the current time.
        while (TimeCompare(mSessionEndTime, milliNow) <= 0)
        {
            mSessionEndTime += mSetting->GetInterval();
        }

        PrintClientReport();
        InitCurrentStats();
    }

    return error;
}

otError Session::SendFin(void)
{
    otError           error       = OT_ERROR_NONE;
    uint32_t          milliNow    = TimerMilli::GetNow();
    otMessageSettings msgSettings = {true, mSetting->GetPriority()};
    otMessage *       message     = NULL;
    otMessageInfo     messageInfo;
    PerfHeader        perfHeader;

    VerifyOrExit(mState == kStateSendingFin, error = OT_ERROR_INVALID_STATE);
    VerifyOrExit(mSetting->GetLength() >= sizeof(PerfHeader), error = OT_ERROR_INVALID_ARGS);

    perfHeader.SetSeqNumber(mSeqNumber);
    perfHeader.SetTime(0);
    perfHeader.SetFinDelay(static_cast<uint8_t>(milliNow - mSessionEndTime));
    perfHeader.SetSessionId(mSessionId);
    perfHeader.SetEchoFlag(mSetting->IsFlagSet(Setting::kFlagEcho));
    perfHeader.SetFinFlag(true);

    VerifyOrExit((message = otUdpNewMessage(mPerf->mInstance, &msgSettings)) != NULL, error = OT_ERROR_NO_BUFS);
    SuccessOrExit(error = otMessageAppend(message, &perfHeader, sizeof(PerfHeader)));
    SuccessOrExit(error = otMessageSetLength(message, sizeof(PerfHeader)));

    memset(&messageInfo, 0, sizeof(otMessageInfo));
    memcpy(messageInfo.mPeerAddr.mFields.m8, mPeerAddr.mFields.m8, sizeof(otIp6Address));
    messageInfo.mPeerPort = mPeerPort;

    error = otUdpSend(&mSocket, message, &messageInfo);

exit:
    if ((error != OT_ERROR_NONE) && message != NULL)
    {
        otMessageFree(message);
    }

    return error;
}

void Session::GetPacketInfo(PerfHeader &aPerfHeader, uint64_t aMicroNow, PacketInfo &aPacketInfo)
{
    uint64_t microNow;

    aPacketInfo.mSeqNumber       = aPerfHeader.GetSeqNumber();
    aPacketInfo.mRelativeLatency = static_cast<int64_t>(aMicroNow) - static_cast<int64_t>(aPerfHeader.GetTime());
    aPacketInfo.mAbsoluteLatency = kInvalidTime;

    if (aPerfHeader.GetReplyFlag())
    {
        if (GetSynchronizedTime(microNow) != OT_ERROR_NONE)
        {
            // The local time can be used to calculate the latency when user is testing round trip time.
            microNow = aMicroNow;
        }

        aPacketInfo.mAbsoluteLatency =
            microNow > aPerfHeader.GetTime() ? static_cast<uint32_t>(microNow - aPerfHeader.GetTime()) : 0;
    }
    else if ((GetSynchronizedTime(microNow) == OT_ERROR_NONE) && aPerfHeader.GetSyncFlag())
    {
        aPacketInfo.mAbsoluteLatency =
            microNow > aPerfHeader.GetTime() ? static_cast<uint32_t>(microNow - aPerfHeader.GetTime()) : 0;
    }
}

void Session::UpdatePacketStats(PacketInfo &aPacketInfo)
{
    int64_t deltaLatency = 0;

    if ((aPacketInfo.mSeqNumber != 0) && (aPacketInfo.mSeqNumber != mSeqNumber + 1))
    {
        if (aPacketInfo.mSeqNumber < mSeqNumber + 1)
        {
            mStats.mCurCntOutOfOrder++;
            mStats.mTotalCntOutOfOrder++;

            if (mStats.mCurCntError > 0)
            {
                mStats.mCurCntError--;
            }

            if (mStats.mTotalCntError > 0)
            {
                mStats.mTotalCntError--;
            }
        }
        else
        {
            mStats.mCurCntError += aPacketInfo.mSeqNumber - (mSeqNumber + 1);
            mStats.mTotalCntError += aPacketInfo.mSeqNumber - (mSeqNumber + 1);
        }
    }

    if (mStats.mTotalCntDatagram != 0)
    {
        // Caculate jitter, refer to sections 6.3.1 and A.8 of RFC 1889.
        if (aPacketInfo.mRelativeLatency > mStats.mRelativeLatency)
        {
            deltaLatency = aPacketInfo.mRelativeLatency - mStats.mRelativeLatency;
        }
        else
        {
            deltaLatency = mStats.mRelativeLatency - aPacketInfo.mRelativeLatency;
        }

        mStats.mJitter += (deltaLatency - mStats.mJitter) / 16;
    }

    mStats.mRelativeLatency = aPacketInfo.mRelativeLatency;

    if (aPacketInfo.mSeqNumber > mSeqNumber)
    {
        mSeqNumber = aPacketInfo.mSeqNumber;
    }

    mStats.mCurBytes += aPacketInfo.mLength;
    mStats.mTotalBytes += aPacketInfo.mLength;
    mStats.mCurCntDatagram++;
    mStats.mTotalCntDatagram++;

    if (aPacketInfo.mAbsoluteLatency == kInvalidTime)
    {
        mStats.mLatencyValid = false;
    }
    else
    {
        mStats.mLatencyValid = true;
        if (aPacketInfo.mAbsoluteLatency < mStats.mCurMinLatency)
        {
            mStats.mCurMinLatency = aPacketInfo.mAbsoluteLatency;
        }

        if (aPacketInfo.mAbsoluteLatency > mStats.mCurMaxLatency)
        {
            mStats.mCurMaxLatency = aPacketInfo.mAbsoluteLatency;
        }

        if (aPacketInfo.mAbsoluteLatency < mStats.mTotalMinLatency)
        {
            mStats.mTotalMinLatency = aPacketInfo.mAbsoluteLatency;
        }

        if (aPacketInfo.mAbsoluteLatency > mStats.mTotalMaxLatency)
        {
            mStats.mTotalMaxLatency = aPacketInfo.mAbsoluteLatency;
        }

        mStats.mTotalLatency += aPacketInfo.mAbsoluteLatency;
        mStats.mCurLatency += aPacketInfo.mAbsoluteLatency;
    }
}

void Session::PrintReport(Report &aReport, bool aServer)
{
    uint32_t lossRate;
    uint32_t interval  = aReport.mEndTime - aReport.mStartTime;
    uint32_t bandwidth = static_cast<uint32_t>((interval == 0) ? 0 : (aReport.mNumBytes * 8000UL / interval));

    if (aServer)
    {
        lossRate = (aReport.mCntDatagram == 0) ? 0 : 100 * aReport.mCntError / aReport.mCntDatagram;
    }

    if (aReport.mIsFormatCvs)
    {
        Server::sServer->OutputFormat("%u,", aReport.mReportType);
        Server::sServer->OutputFormat("%u,%u.%03u,%u.%03u,", aReport.mSessionId, aReport.mStartTime / MS_PER_SEC,
                                      aReport.mStartTime % MS_PER_SEC, aReport.mEndTime / MS_PER_SEC,
                                      aReport.mEndTime % MS_PER_SEC);

        Server::sServer->OutputFormat("%u,", aReport.mNumBytes);
        Server::sServer->OutputFormat("%u,", bandwidth);

        if (aServer)
        {
            Server::sServer->OutputFormat("%d.%03d,%u,%u,%u,", aReport.mJitter / USEC_PER_MS,
                                          aReport.mJitter % USEC_PER_MS, aReport.mCntError, aReport.mCntDatagram,
                                          lossRate);

            if (aReport.mLatencyValid)
            {
                uint32_t avgLatency;

                if (aReport.mCntDatagram == aReport.mCntError)
                {
                    avgLatency = 0;
                }
                else
                {
                    avgLatency = aReport.mLatency / (aReport.mCntDatagram - aReport.mCntError);
                }

                Server::sServer->OutputFormat("%u.%u,%u.%u,%u.%u", aReport.mMinLatency / USEC_PER_MS,
                                              (aReport.mMinLatency % USEC_PER_MS) / 100, avgLatency / USEC_PER_MS,
                                              (avgLatency % USEC_PER_MS) / 100, aReport.mMaxLatency / USEC_PER_MS,
                                              (aReport.mMaxLatency % USEC_PER_MS) / 100);
            }
        }

        Server::sServer->OutputFormat("\r\n");
    }
    else
    {
        Server::sServer->OutputFormat("[%3u] %2u.%03u - %2u.%03u sec  ", aReport.mSessionId,
                                      aReport.mStartTime / MS_PER_SEC, aReport.mStartTime % MS_PER_SEC,
                                      aReport.mEndTime / MS_PER_SEC, aReport.mEndTime % MS_PER_SEC);

        Server::sServer->OutputFormat("%6u Bytes  ", aReport.mNumBytes);
        Server::sServer->OutputFormat("%6u bits/sec  ", bandwidth);

        if (aServer)
        {
            Server::sServer->OutputFormat("%2d.%03dms  ", aReport.mJitter / USEC_PER_MS, aReport.mJitter % USEC_PER_MS);
            Server::sServer->OutputFormat("%4u/%4u     (%2u%%)", aReport.mCntError, aReport.mCntDatagram, lossRate);
            if (aReport.mLatencyValid)
            {
                uint32_t avgLatency;

                if (aReport.mCntDatagram == aReport.mCntError)
                {
                    avgLatency = 0;
                }
                else
                {
                    avgLatency = aReport.mLatency / (aReport.mCntDatagram - aReport.mCntError);
                }

                Server::sServer->OutputFormat(" (%u.%ums, %u.%ums, %u.%ums)", aReport.mMinLatency / USEC_PER_MS,
                                              (aReport.mMinLatency % USEC_PER_MS) / 100, avgLatency / USEC_PER_MS,
                                              (avgLatency % USEC_PER_MS) / 100, aReport.mMaxLatency / USEC_PER_MS,
                                              (aReport.mMaxLatency % USEC_PER_MS) / 100);
            }
        }

        Server::sServer->OutputFormat("\r\n");

        if (aReport.mCntOutOfOrder != 0)
        {
            Server::sServer->OutputFormat("[%3u] %2u.%03u - %2u.%03u sec  ", aReport.mSessionId,
                                          aReport.mStartTime / MS_PER_SEC, aReport.mStartTime % MS_PER_SEC,
                                          aReport.mEndTime / MS_PER_SEC, aReport.mEndTime % MS_PER_SEC);
            Server::sServer->OutputFormat("%u datagrams received out-of-order\r\n", aReport.mCntOutOfOrder);
        }
    }
}

void Session::PrintConnection(void)
{
    VerifyOrExit(!(mSetting->IsFlagSet(Setting::kFlagFormatQuiet) || mSetting->IsFlagSet(Setting::kFlagFormatCvs)));

    Server::sServer->OutputFormat("[%3u] local ", mSessionId);
    Server::sServer->OutputFormat("%x:%x:%x:%x:%x:%x:%x:%x ", HostSwap16(mLocalAddr.mFields.m16[0]),
                                  HostSwap16(mLocalAddr.mFields.m16[1]), HostSwap16(mLocalAddr.mFields.m16[2]),
                                  HostSwap16(mLocalAddr.mFields.m16[3]), HostSwap16(mLocalAddr.mFields.m16[4]),
                                  HostSwap16(mLocalAddr.mFields.m16[5]), HostSwap16(mLocalAddr.mFields.m16[6]),
                                  HostSwap16(mLocalAddr.mFields.m16[7]));
    Server::sServer->OutputFormat("port %u ", mLocalPort);

    Server::sServer->OutputFormat("connected with %x:%x:%x:%x:%x:%x:%x:%x ", HostSwap16(mPeerAddr.mFields.m16[0]),
                                  HostSwap16(mPeerAddr.mFields.m16[1]), HostSwap16(mPeerAddr.mFields.m16[2]),
                                  HostSwap16(mPeerAddr.mFields.m16[3]), HostSwap16(mPeerAddr.mFields.m16[4]),
                                  HostSwap16(mPeerAddr.mFields.m16[5]), HostSwap16(mPeerAddr.mFields.m16[6]),
                                  HostSwap16(mPeerAddr.mFields.m16[7]));
    Server::sServer->OutputFormat("port %u\r\n", mPeerPort);

exit:
    return;
}

void Session::PrintClientReportHeader(void)
{
    VerifyOrExit(!(mSetting->IsFlagSet(Setting::kFlagFormatQuiet) || mSetting->IsFlagSet(Setting::kFlagFormatCvs)));
    Server::sServer->OutputFormat("[ ID]  Interval              Transfer     Bandwidth\r\n");

exit:
    return;
}

void Session::PrintClientReport(void)
{
    Report report;

    VerifyOrExit(!mSetting->IsFlagSet(Setting::kFlagFormatQuiet));

    memset(&report, 0, sizeof(Report));

    report.mIsFormatCvs = mSetting->IsFlagSet(Setting::kFlagFormatCvs);
    report.mReportType  = kReportTypeClient;
    report.mSessionId   = mSessionId;
    report.mStartTime   = mPrintStartTime;
    report.mEndTime     = mPrintEndTime;
    report.mNumBytes    = mStats.mCurBytes;
    report.mCntError    = mStats.mCurCntError;
    report.mCntDatagram = mStats.mCurCntError + mStats.mCurCntDatagram;

    PrintReport(report, false);

exit:
    return;
}

void Session::PrintClientReportEnd(void)
{
    Report report;

    VerifyOrExit(!mSetting->IsFlagSet(Setting::kFlagFormatQuiet));

    memset(&report, 0, sizeof(Report));

    report.mIsFormatCvs = mSetting->IsFlagSet(Setting::kFlagFormatCvs);
    report.mReportType  = kReportTypeClientEnd;
    report.mSessionId   = mSessionId;
    report.mStartTime   = mPrintStartTime;
    report.mEndTime     = mPrintEndTime;
    report.mNumBytes    = mStats.mTotalBytes;
    report.mCntError    = mStats.mTotalCntError;
    report.mCntDatagram = mStats.mTotalCntError + mStats.mTotalCntDatagram;

    if (!mSetting->IsFlagSet(Setting::kFlagFormatCvs))
    {
        Server::sServer->OutputFormat("%s", COLOR_CODE_RED);
    }

    PrintReport(report, false);

    if (!mSetting->IsFlagSet(Setting::kFlagFormatCvs))
    {
        Server::sServer->OutputFormat("%s", COLOR_CODE_END);
    }

exit:
    return;
}

void Session::PrintServerReportHeader(void)
{
    uint64_t microNow;
    VerifyOrExit(!(mSetting->IsFlagSet(Setting::kFlagFormatQuiet) || mSetting->IsFlagSet(Setting::kFlagFormatCvs)));

    Server::sServer->OutputFormat("\r\n[ ID] Interval             Transfer     Bandwidth         "
                                  "Jitter    Lost/Total LossRate ");

    if (GetSynchronizedTime(microNow) == OT_ERROR_NONE)
    {
        Server::sServer->OutputFormat("Latency(min, avg, max)\r\n");
    }
    else
    {
        Server::sServer->OutputFormat("\r\n");
    }

exit:
    return;
}

void Session::PrintServerReport(void)
{
    Report report;

    VerifyOrExit(!mSetting->IsFlagSet(Setting::kFlagFormatQuiet));

    memset(&report, 0, sizeof(Report));

    report.mIsFormatCvs   = mSetting->IsFlagSet(Setting::kFlagFormatCvs);
    report.mReportType    = kReportTypeServer;
    report.mSessionId     = mSessionId;
    report.mStartTime     = mPrintStartTime;
    report.mEndTime       = mPrintEndTime;
    report.mNumBytes      = mStats.mCurBytes;
    report.mJitter        = mStats.mJitter;
    report.mCntError      = mStats.mCurCntError;
    report.mCntDatagram   = mStats.mCurCntError + mStats.mCurCntDatagram;
    report.mCntOutOfOrder = mStats.mCurCntOutOfOrder;
    report.mMinLatency    = mStats.mCurMinLatency;
    report.mMaxLatency    = mStats.mCurMaxLatency;
    report.mLatency       = mStats.mCurLatency;
    report.mLatencyValid  = mStats.mLatencyValid;

    PrintReport(report, true);

exit:
    return;
}

void Session::PrintServerReportEnd(void)
{
    Report report;

    VerifyOrExit(!mSetting->IsFlagSet(Setting::kFlagFormatQuiet));

    memset(&report, 0, sizeof(Report));

    report.mIsFormatCvs   = mSetting->IsFlagSet(Setting::kFlagFormatCvs);
    report.mReportType    = kReportTypeServerEnd;
    report.mSessionId     = mSessionId;
    report.mStartTime     = 0;
    report.mEndTime       = mPrintEndTime;
    report.mNumBytes      = mStats.mTotalBytes;
    report.mJitter        = mStats.mJitter;
    report.mCntError      = mStats.mTotalCntError;
    report.mCntDatagram   = mStats.mTotalCntError + mStats.mTotalCntDatagram;
    report.mCntOutOfOrder = mStats.mTotalCntOutOfOrder;
    report.mMinLatency    = mStats.mTotalMinLatency;
    report.mMaxLatency    = mStats.mTotalMaxLatency;
    report.mLatency       = mStats.mTotalLatency;
    report.mLatencyValid  = mStats.mLatencyValid;

    if (!mSetting->IsFlagSet(Setting::kFlagFormatCvs))
    {
        Server::sServer->OutputFormat("%s", COLOR_CODE_RED);
    }

    PrintReport(report, true);

    if (!mSetting->IsFlagSet(Setting::kFlagFormatCvs))
    {
        Server::sServer->OutputFormat("%s", COLOR_CODE_END);
    }

exit:
    return;
}

void Session::TimerFired(void)
{
    uint32_t milliNow = TimerMilli::GetNow();

    VerifyOrExit(mState == kStateSendingData || mState == kStateSendingFin || mState == kStateReceivingFin);
    VerifyOrExit(TimeCompare(mFireTime, mPerf->mTimer.GetNow()) <= 0);

    switch (mState)
    {
    case kStateSendingData:
        SendData();

        if (mSetting->GetTime() != 0)
        {
            if (mSetting->IsFlagSet(Setting::kFlagNumber))
            {
                if (mStats.mTotalCntDatagram >= mSetting->GetCount())
                {
                    mState = kStateSendingFin;
                }
            }
            else if (milliNow - mSessionStartTime >= mSetting->GetTime())
            {
                mState = kStateSendingFin;
            }
        }

        if (mState == kStateSendingData)
        {
            mFireTime += UsToTimerTime(mSendingIntervalUs);
            break;
        }

        mPrintStartTime = 0;
        mPrintEndTime   = milliNow - mSessionStartTime;
        mSessionEndTime = milliNow;
        mFinCounter     = 0;

        PrintClientReportEnd();
        if (mSetting->GetFinDelay() != 0)
        {
            mFireTime += UsToTimerTime(mSetting->GetFinDelay() * USEC_PER_SEC);
        }
        else
        {
            mFireTime += UsToTimerTime(kFinIntervalUs);
        }

        // fall through

    case kStateSendingFin:
        SendFin();
        mFinCounter++;

        if (mFinCounter >= kMaxNumFin)
        {
            CloseSocket();
            mState = kStateInvalid;
        }
        else
        {
            mFireTime += UsToTimerTime(kFinIntervalUs);
        }
        break;

    case kStateReceivingFin:
        CloseSocket();
        mState = kStateInvalid;
        break;

    default:
        break;
    }

exit:
    return;
}

bool Session::MatchMsgInfo(const otMessageInfo &aMessageInfo)
{
    bool ret = false;

    if ((aMessageInfo.mPeerPort == mPeerPort) &&
        (memcmp(aMessageInfo.mPeerAddr.mFields.m8, mPeerAddr.mFields.m8, sizeof(otIp6Address)) == 0) &&
        (aMessageInfo.mSockPort == mLocalPort))
    {
        ret = true;
    }

    return ret;
}

otError Session::GetDelayInterval(uint32_t &aInterval)
{
    otError  error = OT_ERROR_NONE;
    uint32_t now   = mPerf->mTimer.GetNow();

    VerifyOrExit(mState == kStateSendingData || mState == kStateSendingFin || mState == kStateReceivingFin,
                 error = OT_ERROR_INVALID_STATE);

    if (TimeCompare(mFireTime, now) <= 0)
    {
        aInterval = 0;
        ExitNow();
    }

    aInterval = mFireTime - now;

exit:
    return error;
}

otError Session::GetSynchronizedTime(uint64_t &aSyncTime)
{
    otError error = OT_ERROR_FAILED;

#if OPENTHREAD_CONFIG_TIME_SYNC_ENABLE
    if (otNetworkTimeGet(mPerf->mInstance, &aSyncTime) == OT_NETWORK_TIME_SYNCHRONIZED)
    {
        error = OT_ERROR_NONE;
    }
#endif

    OT_UNUSED_VARIABLE(aSyncTime);

    return error;
}

otError Session::PrepareReceive(void)
{
    otError error = OT_ERROR_NONE;

    VerifyOrExit(mRole == kRoleListener, error = OT_ERROR_FAILED);
    VerifyOrExit(mState == kStateIdle, error = OT_ERROR_FAILED);

    mLocalPort = kUdpPort;
    SuccessOrExit(error = OpenSocket());

    mState = kStateListening;

exit:
    return error;
}

otError Session::PrepareTransmit(uint8_t aSessionId)
{
    otError  error    = OT_ERROR_NONE;
    uint32_t milliNow = TimerMilli::GetNow();

    VerifyOrExit(mRole == kRoleClient, error = OT_ERROR_FAILED);
    VerifyOrExit(mState == kStateIdle, error = OT_ERROR_FAILED);

    mSendingIntervalUs =
        static_cast<uint32_t>((static_cast<uint64_t>(mSetting->GetLength()) * 8000000LL) / mSetting->GetBandwidth());
    if (mSendingIntervalUs < kMinSendingIntervalUs)
    {
        mSendingIntervalUs = kMinSendingIntervalUs;
    }

    memcpy(mPeerAddr.mFields.m8, mSetting->GetDestAddr()->mFields.m8, sizeof(otIp6Address));

    mFireTime         = mPerf->mTimer.GetNow() + UsToTimerTime(mSendingIntervalUs);
    mSessionStartTime = milliNow;
    mSessionEndTime   = milliNow + mSetting->GetInterval();
    mSessionId        = (mSetting->IsFlagSet(Setting::kFlagSessionId)) ? mSetting->GetSessionId() : aSessionId;
    mPeerPort         = kUdpPort;
    mPrintEndTime     = 0;

    SuccessOrExit(error = OpenSocket());

    mState = kStateSendingData;

exit:
    return error;
}

uint32_t Session::UsToTimerTime(uint32_t aTime)
{
#if !OPENTHREAD_CONFIG_PLATFORM_USEC_TIMER_ENABLE
    aTime = aTime / USEC_PER_MS;
#endif

    return aTime;
}

Perf::Perf(Interpreter &aInterpreter)
    : mServerRunning(false)
    , mClientRunning(false)
    , mPrintServerHeaderFlag(false)
    , mPrintClientHeaderFlag(false)
    , mTimer(*aInterpreter.mInstance, &Perf::s_HandleTimer, this)
    , mServerSetting(NULL)
    , mInterpreter(aInterpreter)
    , mInstance(aInterpreter.mInstance)
{
    memset(mSettings, 0, sizeof(mSettings));
    memset(mSessions, 0, sizeof(mSessions));
}

otError Perf::Process(int argc, char *argv[])
{
    otError error = OT_ERROR_PARSE;

    if (argc == 0)
    {
        ProcessHelp(0, NULL);
        ExitNow(error = OT_ERROR_NONE);
    }

    for (size_t i = 0; i < OT_ARRAY_LENGTH(sCommands); i++)
    {
        if (strcmp(argv[0], sCommands[i].mName) == 0)
        {
            error = (this->*sCommands[i].mCommand)(argc - 1, argv + 1);
            break;
        }
    }

exit:
    return error;
}

otError Perf::ProcessHelp(int argc, char *argv[])
{
    for (size_t i = 0; i < OT_ARRAY_LENGTH(sCommands); i++)
    {
        Server::sServer->OutputFormat("%s\r\n", sCommands[i].mName);
    }

    OT_UNUSED_VARIABLE(argc);
    OT_UNUSED_VARIABLE(argv);

    return OT_ERROR_NONE;
}

otError Perf::ProcessClient(int argc, char *argv[])
{
    otError  error   = OT_ERROR_NONE;
    Setting *setting = NULL;

    VerifyOrExit(argc >= 1, error = OT_ERROR_INVALID_ARGS);
    VerifyOrExit(!(mServerRunning || mClientRunning), error = OT_ERROR_BUSY);
    VerifyOrExit((setting = NewSetting()) != NULL, error = OT_ERROR_NO_BUFS);

    SuccessOrExit(error = SetSetting(argc, argv, true, *setting));
    VerifyOrExit(setting->IsFlagSet(Setting::kFlagDestAddr), error = OT_ERROR_INVALID_ARGS);

exit:
    if ((error != OT_ERROR_NONE) && (setting != NULL))
    {
        FreeSetting(*setting);
    }

    return error;
}

otError Perf::ProcessServer(int argc, char *argv[])
{
    otError  error   = OT_ERROR_NONE;
    Setting *setting = NULL;

    VerifyOrExit(!(mServerRunning || mClientRunning), error = OT_ERROR_BUSY);
    VerifyOrExit(mServerSetting == NULL, error = OT_ERROR_ALREADY);
    VerifyOrExit((setting = NewSetting()) != NULL, error = OT_ERROR_NO_BUFS);
    SuccessOrExit(error = SetSetting(argc, argv, false, *setting));

    mServerSetting = setting;

exit:
    if ((error != OT_ERROR_NONE) && (setting != NULL))
    {
        FreeSetting(*setting);
    }

    return error;
}

otError Perf::SetSetting(int argc, char *argv[], bool aClient, Setting &aSetting)
{
    otError error = OT_ERROR_NONE;
    long    value;

    for (int i = 0; i < argc; i += 2)
    {
        if (i == argc - 1)
        {
            ExitNow(error = OT_ERROR_INVALID_ARGS);
        }
        else if (strcmp(argv[i], "interval") == 0)
        {
            SuccessOrExit(error = Interpreter::ParseLong(argv[i + 1], value));
            VerifyOrExit(value != 0, error = OT_ERROR_INVALID_ARGS);

            aSetting.SetFlag(Setting::kFlagInterval);
            aSetting.SetInterval(static_cast<uint32_t>(value) * MS_PER_SEC);
        }
        else if (strcmp(argv[i], "format") == 0)
        {
            if (strcmp(argv[i + 1], "cvs") == 0)
            {
                aSetting.SetFlag(Setting::kFlagFormatCvs);
                aSetting.ClearFlag(Setting::kFlagFormatQuiet);
            }
            else if (strcmp(argv[i + 1], "quiet") == 0)
            {
                aSetting.SetFlag(Setting::kFlagFormatQuiet);
                aSetting.ClearFlag(Setting::kFlagFormatCvs);
            }
            else
            {
                ExitNow(error = OT_ERROR_INVALID_ARGS);
            }
        }
        else if (aClient)
        {
            if (strcmp(argv[i], "destaddr") == 0)
            {
                otIp6Address destAddr;
                SuccessOrExit(error = otIp6AddressFromString(argv[i + 1], &destAddr));
                aSetting.SetFlag(Setting::kFlagDestAddr);
                aSetting.SetDestAddr(destAddr);
            }
            else if (strcmp(argv[i], "bandwidth") == 0)
            {
                SuccessOrExit(error = Interpreter::ParseLong(argv[i + 1], value));
                VerifyOrExit(value != 0, error = OT_ERROR_INVALID_ARGS);

                aSetting.SetFlag(Setting::kFlagBandwidth);
                aSetting.SetBandwidth(static_cast<uint32_t>(value));
            }
            else if (strcmp(argv[i], "length") == 0)
            {
                SuccessOrExit(error = Interpreter::ParseLong(argv[i + 1], value));
                VerifyOrExit(static_cast<uint16_t>(value) >= sizeof(PerfHeader), error = OT_ERROR_INVALID_ARGS);
                VerifyOrExit(value <= kMaxPayloadLength, error = OT_ERROR_INVALID_ARGS);

                aSetting.SetFlag(Setting::kFlagLength);
                aSetting.SetLength(static_cast<uint16_t>(value));
            }
            else if (strcmp(argv[i], "priority") == 0)
            {
                SuccessOrExit(error = Interpreter::ParseLong(argv[i + 1], value));
                VerifyOrExit(value <= OT_MESSAGE_PRIORITY_HIGH, error = OT_ERROR_INVALID_ARGS);

                aSetting.SetFlag(Setting::kFlagPriority);
                aSetting.SetPriority(static_cast<otMessagePriority>(value));
            }
            else if (strcmp(argv[i], "time") == 0)
            {
                SuccessOrExit(error = Interpreter::ParseLong(argv[i + 1], value));

                aSetting.SetFlag(Setting::kFlagTime);
                aSetting.SetTime(static_cast<uint32_t>(value) * MS_PER_SEC);
            }
            else if (strcmp(argv[i], "count") == 0)
            {
                SuccessOrExit(error = Interpreter::ParseLong(argv[i + 1], value));
                VerifyOrExit(value != 0, error = OT_ERROR_INVALID_ARGS);

                aSetting.SetFlag(Setting::kFlagNumber);
                aSetting.SetCount(static_cast<uint32_t>(value));
            }
            else if (strcmp(argv[i], "id") == 0)
            {
                SuccessOrExit(error = Interpreter::ParseLong(argv[i + 1], value));
                VerifyOrExit(value <= 0xff, error = OT_ERROR_INVALID_ARGS);

                aSetting.SetFlag(Setting::kFlagSessionId);
                aSetting.SetSessionId(static_cast<uint8_t>(value));
            }
            else if (strcmp(argv[i], "findelay") == 0)
            {
                SuccessOrExit(error = Interpreter::ParseLong(argv[i + 1], value));
                VerifyOrExit(value <= Setting::kMaxFinDelayMs, error = OT_ERROR_INVALID_ARGS);

                aSetting.SetFlag(Setting::kFlagFinDelay);
                aSetting.SetFinDelay(static_cast<uint8_t>(value));
            }
            else if (strcmp(argv[i], "echo") == 0)
            {
                SuccessOrExit(error = Interpreter::ParseLong(argv[i + 1], value));

                if (value == 0)
                {
                    aSetting.SetFlag(Setting::kFlagEcho);
                    aSetting.SetEchoFlag(false);
                }
                else
                {
                    aSetting.SetFlag(Setting::kFlagEcho);
                    aSetting.SetEchoFlag(true);
                }
            }
            else
            {
                error = OT_ERROR_INVALID_ARGS;
                break;
            }
        }
        else
        {
            error = OT_ERROR_INVALID_ARGS;
            break;
        }
    }

exit:
    if (error == OT_ERROR_NONE)
    {
        if (aClient)
        {
            aSetting.SetFlag(Setting::kFlagClient);
        }
        else
        {
            aSetting.ClearFlag(Setting::kFlagClient);
        }
    }

    return error;
}

void Perf::PrintSetting(Setting &aSetting)
{
    const otIp6Address *destAddr = aSetting.GetDestAddr();

    if (aSetting.IsFlagSet(Setting::kFlagClient))
    {
        Server::sServer->OutputFormat("perf client ");
    }
    else
    {
        Server::sServer->OutputFormat("perf server ");
    }

    if (aSetting.IsFlagSet(Setting::kFlagDestAddr))
    {
        Server::sServer->OutputFormat("destaddr %x:%x:%x:%x:%x:%x:%x:%x ", HostSwap16(destAddr->mFields.m16[0]),
                                      HostSwap16(destAddr->mFields.m16[1]), HostSwap16(destAddr->mFields.m16[2]),
                                      HostSwap16(destAddr->mFields.m16[3]), HostSwap16(destAddr->mFields.m16[4]),
                                      HostSwap16(destAddr->mFields.m16[5]), HostSwap16(destAddr->mFields.m16[6]),
                                      HostSwap16(destAddr->mFields.m16[7]));
    }

    if (aSetting.IsFlagSet(Setting::kFlagBandwidth))
    {
        Server::sServer->OutputFormat("bandwidth %u ", aSetting.GetBandwidth());
    }

    if (aSetting.IsFlagSet(Setting::kFlagLength))
    {
        Server::sServer->OutputFormat("length %u ", aSetting.GetLength());
    }

    if (aSetting.IsFlagSet(Setting::kFlagInterval))
    {
        Server::sServer->OutputFormat("interval %u ", aSetting.GetInterval() / MS_PER_SEC);
    }

    if (aSetting.IsFlagSet(Setting::kFlagFormatCvs))
    {
        Server::sServer->OutputFormat("format cvs ");
    }
    else if (aSetting.IsFlagSet(Setting::kFlagFormatQuiet))
    {
        Server::sServer->OutputFormat("format quiet ");
    }

    if (aSetting.IsFlagSet(Setting::kFlagTime))
    {
        Server::sServer->OutputFormat("time %u ", aSetting.GetTime() / MS_PER_SEC);
    }

    if (aSetting.IsFlagSet(Setting::kFlagNumber))
    {
        Server::sServer->OutputFormat("count %u ", aSetting.GetCount());
    }

    if (aSetting.IsFlagSet(Setting::kFlagPriority))
    {
        Server::sServer->OutputFormat("priority %u ", aSetting.GetPriority());
    }

    if (aSetting.IsFlagSet(Setting::kFlagSessionId))
    {
        Server::sServer->OutputFormat("id %u ", aSetting.GetSessionId());
    }

    if (aSetting.IsFlagSet(Setting::kFlagFinDelay))
    {
        Server::sServer->OutputFormat("findelay %u ", aSetting.GetFinDelay());
    }

    if (aSetting.IsFlagSet(Setting::kFlagEcho))
    {
        Server::sServer->OutputFormat("echo %d ", aSetting.GetEchoFlag());
    }

    Server::sServer->OutputFormat("\r\n");
}

void Perf::SessionStop(uint8_t aRole)
{
    for (size_t i = 0; i < OT_ARRAY_LENGTH(mSessions); i++)
    {
        if (!mSessions[i].IsStateValid())
        {
            continue;
        }

        if (mSessions[i].GetRole() == aRole)
        {
            mSessions[i].Free();
        }
    }
}

otError Perf::ServerStart()
{
    otError  error = OT_ERROR_NONE;
    Session *session;

    VerifyOrExit(mServerSetting != NULL);
    VerifyOrExit(!mServerRunning, error = OT_ERROR_BUSY);
    VerifyOrExit((session = NewSession(*mServerSetting, Session::kRoleListener)) != NULL, error = OT_ERROR_NO_BUFS);

    if (session->PrepareReceive() != OT_ERROR_NONE)
    {
        session->Free();
        ExitNow(error = OT_ERROR_FAILED);
    }

    mServerRunning = true;

exit:
    if (error != OT_ERROR_NONE)
    {
        SessionStop(Session::kRoleListener);
    }

    return error;
}

otError Perf::ClientStart()
{
    otError  error         = OT_ERROR_NONE;
    bool     clientRunning = false;
    Session *session;

    VerifyOrExit(!mClientRunning);

    for (size_t i = 0; i < OT_ARRAY_LENGTH(mSettings); i++)
    {
        if (!mSettings[i].IsFlagSet(Setting::kFlagValid) || !mSettings[i].IsFlagSet(Setting::kFlagClient))
        {
            continue;
        }

        VerifyOrExit((session = NewSession(mSettings[i], Session::kRoleClient)) != NULL, error = OT_ERROR_NO_BUFS);
        SuccessOrExit(session->PrepareTransmit(static_cast<uint8_t>(i)));

        if (mPrintClientHeaderFlag == false)
        {
            mPrintClientHeaderFlag = true;
            session->PrintClientReportHeader();
        }

        clientRunning = true;
    }

    VerifyOrExit(clientRunning);

    mClientRunning = true;
    StartTimer();

exit:
    if (error != OT_ERROR_NONE)
    {
        SessionStop(Session::kRoleClient);
    }

    return error;
}

otError Perf::ProcessStart(int argc, char *argv[])
{
    otError error = OT_ERROR_NONE;

    VerifyOrExit(argc <= 1, error = OT_ERROR_INVALID_ARGS);
    VerifyOrExit(otThreadGetDeviceRole(mInstance) != OT_DEVICE_ROLE_DISABLED, error = OT_ERROR_INVALID_STATE);
    VerifyOrExit(!(mServerRunning && mClientRunning), error = OT_ERROR_BUSY);

    if (argc == 0)
    {
        SuccessOrExit(error = ServerStart());
        if ((error = ClientStart()) != OT_ERROR_NONE)
        {
            ServerStop();
        }
    }
    else if (strcmp(argv[0], "server") == 0)
    {
        SuccessOrExit(error = ServerStart());
    }
    else if (strcmp(argv[0], "client") == 0)
    {
        SuccessOrExit(error = ClientStart());
    }
    else
    {
        ExitNow(error = OT_ERROR_INVALID_ARGS);
    }

    VerifyOrExit(mServerRunning | mClientRunning, error = OT_ERROR_FAILED);

exit:
    return error;
}

void Perf::ServerStop()
{
    VerifyOrExit(mServerRunning);

    SessionStop(Session::kRoleListener);
    SessionStop(Session::kRoleServer);

    mServerRunning         = false;
    mPrintServerHeaderFlag = false;

    if (mClientRunning == false && mTimer.IsRunning())
    {
        mTimer.Stop();
    }

exit:
    return;
}

void Perf::ClientStop(void)
{
    VerifyOrExit(mClientRunning);

    SessionStop(Session::kRoleClient);

    mClientRunning         = false;
    mPrintClientHeaderFlag = false;

    if (mServerRunning == false && mTimer.IsRunning())
    {
        mTimer.Stop();
    }

exit:
    return;
}

Session *Perf::FindValidSession(uint8_t aRole)
{
    Session *session = NULL;

    for (size_t i = 0; i < OT_ARRAY_LENGTH(mSessions); i++)
    {
        if ((mSessions[i].IsStateValid()) && (mSessions[i].GetRole() == aRole))
        {
            session = &mSessions[i];
            break;
        }
    }

    return session;
}

void Perf::UpdateClientState(void)
{
    VerifyOrExit(mClientRunning);
    VerifyOrExit(FindValidSession(Session::kRoleClient) == NULL);

    mClientRunning         = false;
    mPrintClientHeaderFlag = false;

    if (mServerRunning == false && mTimer.IsRunning())
    {
        mTimer.Stop();
    }

exit:
    return;
}

otError Perf::ProcessStop(int argc, char *argv[])
{
    otError error = OT_ERROR_NONE;

    VerifyOrExit(argc <= 1, error = OT_ERROR_INVALID_ARGS);
    VerifyOrExit(mServerRunning || mClientRunning);

    if (argc == 0)
    {
        ServerStop();
        ClientStop();
    }
    else if (strcmp(argv[0], "server") == 0)
    {
        ServerStop();
    }
    else if (strcmp(argv[0], "client") == 0)
    {
        ClientStop();
    }
    else
    {
        ExitNow(error = OT_ERROR_INVALID_ARGS);
    }

exit:

    return error;
}

otError Perf::ProcessShow(int argc, char *argv[])
{
    for (size_t i = 0; i < OT_ARRAY_LENGTH(mSettings); i++)
    {
        if (!mSettings[i].IsFlagSet(Setting::kFlagValid))
        {
            continue;
        }

        PrintSetting(mSettings[i]);
    }

    OT_UNUSED_VARIABLE(argc);
    OT_UNUSED_VARIABLE(argv);

    return OT_ERROR_NONE;
}

otError Perf::ProcessClear(int argc, char *argv[])
{
    otError error = OT_ERROR_NONE;

    VerifyOrExit(!(mServerRunning || mClientRunning), error = OT_ERROR_BUSY);

    for (size_t i = 0; i < OT_ARRAY_LENGTH(mSettings); i++)
    {
        if (mSettings[i].IsFlagSet(Setting::kFlagValid))
        {
            FreeSetting(mSettings[i]);
        }
    }

    OT_UNUSED_VARIABLE(argc);
    OT_UNUSED_VARIABLE(argv);

exit:
    return error;
}

void Perf::s_HandleUdpReceive(void *aContext, otMessage *aMessage, const otMessageInfo *aMessageInfo)
{
    otPerfContext *context = static_cast<otPerfContext *>(aContext);
    context->mPerf->HandleUdpReceive(aMessage, aMessageInfo, *context->mSession);
}

void Perf::HandleUdpReceive(otMessage *aMessage, const otMessageInfo *aMessageInfo, Session &aSession)
{
    Session *session;

    VerifyOrExit(mServerRunning);

    if ((session = FindSession(*aMessageInfo)) == NULL)
    {
        VerifyOrExit((session = NewSession(aSession.GetSetting(), Session::kRoleServer)) != NULL);

        if (session->HandleFistMessage(*aMessage, *aMessageInfo) != OT_ERROR_NONE)
        {
            session->Free();
        }
    }
    else
    {
        session->HandleSubsequentMessages(*aMessage, *aMessageInfo);
    }

exit:
    return;
}

Setting *Perf::NewSetting()
{
    Setting *setting = NULL;

    for (size_t i = 0; i < OT_ARRAY_LENGTH(mSettings); i++)
    {
        if (!mSettings[i].IsFlagSet(Setting::kFlagValid))
        {
            setting = new (static_cast<void *>(&mSettings[i])) Setting();
            setting->SetFlag(Setting::kFlagValid);
            break;
        }
    }

    return setting;
}

void Perf::FreeSetting(Setting &aSetting)
{
    if (!aSetting.IsFlagSet(Setting::kFlagClient))
    {
        mServerSetting = NULL;
    }

    aSetting.ClearFlag(Setting::kFlagValid);
}

Session *Perf::NewSession(const Setting &aSetting, uint8_t aRole)
{
    Session *session = NULL;

    for (size_t i = 0; i < OT_ARRAY_LENGTH(mSessions); i++)
    {
        if (!mSessions[i].IsStateValid())
        {
            session = new (static_cast<void *>(&mSessions[i])) Session(*this, aSetting, aRole);
            break;
        }
    }

    return session;
}

Session *Perf::FindSession(const otMessageInfo &aMessageInfo)
{
    Session *session = NULL;

    for (size_t i = 0; i < OT_ARRAY_LENGTH(mSessions); i++)
    {
        if ((mSessions[i].IsStateValid()) && mSessions[i].MatchMsgInfo(aMessageInfo))
        {
            session = &mSessions[i];
            break;
        }
    }

    return session;
}

Perf &Perf::GetOwner(OwnerLocator &aOwnerLocator)
{
#if OPENTHREAD_ENABLE_MULTIPLE_INSTANCES
    Perf &perf = (aOwnerLocator.GetOwner<Perf>());
#else
    Perf &perf = Server::sServer->GetInterpreter().mPerf;
    OT_UNUSED_VARIABLE(aOwnerLocator);
#endif

    return perf;
}

otError Perf::FindMinDelayInterval(uint32_t &aInterval)
{
    otError  error       = OT_ERROR_NONE;
    uint32_t minInterval = UINT32_MAX;
    uint32_t interval;

    for (size_t i = 0; i < OT_ARRAY_LENGTH(mSessions); i++)
    {
        if (mSessions[i].GetDelayInterval(interval) == OT_ERROR_NONE)
        {
            if (interval < minInterval)
            {
                minInterval = interval;
            }
        }
    }

    VerifyOrExit(minInterval != UINT32_MAX, error = OT_ERROR_NOT_FOUND);
    aInterval = minInterval;

exit:
    return error;
}

void Perf::StartTimer()
{
    uint32_t interval;

    VerifyOrExit(FindMinDelayInterval(interval) == OT_ERROR_NONE);

    if (mTimer.IsRunning())
    {
        mTimer.Stop();
    }

    mTimer.Start(interval);

exit:
    return;
}

void Perf::s_HandleTimer(Timer &aTimer)
{
    GetOwner(aTimer).HandleTimer();
}

void Perf::HandleTimer(void)
{
    for (size_t i = 0; i < OT_ARRAY_LENGTH(mSessions); i++)
    {
        mSessions[i].TimerFired();
    }

    UpdateClientState();
    StartTimer();
}
} // namespace Cli
} // namespace ot
#endif // OPENTHREAD_CONFIG_CLI_PERF_ENABLE
