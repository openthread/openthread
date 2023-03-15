/*
 *  Copyright (c) 2023, The OpenThread Authors.
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

#include "cli_bdx_perf.hpp"

#if OPENTHREAD_CONFIG_BDX_PERF_ENABLE

#include "openthread/thread.h"

#include "common/encoding.hpp"
#include "common/instance.hpp"
#include "common/num_utils.hpp"
#include "common/timer.hpp"

namespace ot {
namespace Cli {

static constexpr uint32_t kDataMsgMagicHeader = 0x768539e9;
static constexpr uint32_t kAckMsgMagicHeader  = 0x894a58e6;

BdxPerf::BdxPerf(const NewMsgApi              aNewMsgApi,
                 const SendMsgApi             aSendMsgApi,
                 const StartListeningApi      aStartListeningApi,
                 const StopListeningApi       aStopListeningApi,
                 const TimerFireAtApi         aTimerFireAtApi,
                 const TimerStopApi           aTimerStopApi,
                 const ReportBdxPerfResultApi aReportBdxPerfResultApi,
                 void                        *aApiContext)
    : mNewMsgApi(aNewMsgApi)
    , mSendMsgApi(aSendMsgApi)
    , mStartListeningApi(aStartListeningApi)
    , mStopListeningApi(aStopListeningApi)
    , mTimerFireAtApi(aTimerFireAtApi)
    , mTimerStopApi(aTimerStopApi)
    , mReportBdxPerfResultApi(aReportBdxPerfResultApi)
    , mApiContext(aApiContext)
    , mState(kIdle)
    , mTimerSeriesId(kMaxSendSeries)
{
    for (auto &mSendSerie : mSendSeries)
    {
        mSendSerie.Reset();
    }
}

otError BdxPerf::ReceiverStart(otSockAddr &aSockAddr)
{
    otError error = OT_ERROR_NONE;

    VerifyOrExit(mState == kIdle, error = OT_ERROR_INVALID_STATE);

    SuccessOrExit(error = mStartListeningApi(aSockAddr, HandleReceivedMessage, mApiContext));
    mState = kReceiverOn;

exit:
    return error;
}

otError BdxPerf::ReceiverStop(void)
{
    mState = kIdle;
    return mStopListeningApi(mApiContext);
}

otError BdxPerf::SenderStart(const otSockAddr &aPeerAddr,
                             const otSockAddr &aSockAddr,
                             uint8_t           aSeriesId,
                             uint16_t          aDataSize,
                             uint16_t          aAckSize,
                             uint16_t          aMsgCount)
{
    otError error = OT_ERROR_NONE;

    VerifyOrExit(mState != kReceiverOn, error = OT_ERROR_INVALID_STATE);
    VerifyOrExit(aSeriesId < kMaxSendSeries && aDataSize < kMaxPlSize && aAckSize < kMaxPlSize && aMsgCount > 0,
                 error = OT_ERROR_INVALID_ARGS);
    VerifyOrExit(mSendSeries[aSeriesId].mDataSize == 0, error = OT_ERROR_ALREADY);

    SuccessOrExit(error = mStartListeningApi(aSockAddr, HandleReceivedMessage, mApiContext));

    mSendSeries[aSeriesId].mDataSize  = aDataSize;
    mSendSeries[aSeriesId].mAckSize   = aAckSize;
    mSendSeries[aSeriesId].mMsgCount  = aMsgCount;
    mSendSeries[aSeriesId].mStartTime = TimerMilli::GetNow();
    mSendSeries[aSeriesId].mPeerAddr  = aPeerAddr;
    IgnoreError(SendDataMessage(aSeriesId, aDataSize, aAckSize));

    mState = kSenderOn;
    AckTimerFireAt(aSeriesId, TimerMilli::GetNow() + kAckWaitTimeMs);

exit:
    return error;
}

otError BdxPerf::SenderStop(uint8_t aSeriesId)
{
    mSendSeries[aSeriesId].Reset();
    RescheduleAckTimer();

    return OT_ERROR_NONE;
}

void BdxPerf::HandleTimer(void)
{
    uint8_t seriesId = mTimerSeriesId;
    mTimerSeriesId   = kMaxSendSeries;

    mSendSeries[seriesId].mWaitAckTime.SetValue(0);
    mSendSeries[seriesId].mIsWaitAckTimeSet = false;
    mSendSeries[seriesId].mMsgCount--;
    mSendSeries[seriesId].mMsgSeqId++;
    if (mSendSeries[seriesId].mMsgCount == 0)
    {
        FinalizeSendSeries(seriesId);
    }
    else
    {
        IgnoreError(SendDataMessage(seriesId, mSendSeries[seriesId].mDataSize, mSendSeries[seriesId].mAckSize));
        AckTimerFireAt(seriesId, TimerMilli::GetNow() + kAckWaitTimeMs);
    }
}

void BdxPerf::HandleReceivedMessage(void *aContext, otMessage *aMessage, const otMessageInfo *aMessageInfo)
{
    static_cast<BdxPerf *>(aContext)->HandleReceivedMessage(aMessage, aMessageInfo);
}

void BdxPerf::HandleReceivedMessage(otMessage *aMessage, const otMessageInfo *aMessageInfo)
{
    if (mState == kReceiverOn)
    {
        HandleReceivedData(aMessage, aMessageInfo);
    }
    else if (mState == kSenderOn)
    {
        HandleReceivedAck(aMessage, aMessageInfo);
    }
}

void BdxPerf::HandleReceivedData(otMessage *aMessage, const otMessageInfo *aMessageInfo)
{
    uint8_t  buf[1500];
    uint16_t length;
    uint32_t magicHeader;
    uint8_t  seriesId;
    uint16_t seqId;
    uint16_t dataPlSize;
    uint16_t ackPlSize;
    uint16_t offset = 0;

    length      = otMessageRead(aMessage, otMessageGetOffset(aMessage), buf, sizeof(buf) - 1);
    buf[length] = '\0';

    magicHeader = Encoding::LittleEndian::ReadUint32(buf);
    VerifyOrExit(magicHeader == kDataMsgMagicHeader);
    offset += sizeof(magicHeader);

    seriesId = buf[offset];
    offset += sizeof(seriesId);

    seqId = Encoding::LittleEndian::ReadUint16(buf + offset);
    offset += sizeof(seqId);

    // Skip the size of data payload
    offset += sizeof(dataPlSize);

    ackPlSize = Encoding::LittleEndian::ReadUint16(buf + offset);

    IgnoreError(SendAckMessage(*aMessageInfo, seriesId, seqId, ackPlSize));
exit:
    return;
}

void BdxPerf::HandleReceivedAck(otMessage *aMessage, const otMessageInfo *aMessageInfo)
{
    uint8_t  buf[1500];
    uint16_t length;
    uint32_t magicHeader;
    uint8_t  seriesId;
    uint16_t seqId;
    uint16_t offset = 0;

    OT_UNUSED_VARIABLE(aMessageInfo);

    length      = otMessageRead(aMessage, otMessageGetOffset(aMessage), buf, sizeof(buf) - 1);
    buf[length] = '\0';

    magicHeader = Encoding::LittleEndian::ReadUint32(buf);
    VerifyOrExit(magicHeader == kAckMsgMagicHeader);
    offset += sizeof(magicHeader);

    seriesId = buf[offset];
    offset += sizeof(seriesId);

    seqId = Encoding::LittleEndian::ReadUint16(buf + offset);

    if (mSendSeries[seriesId].mDataSize == 0)
    {
        // Received ACK for inactive send series
        ExitNow();
    }
    if (mSendSeries[seriesId].mMsgCount == 0)
    {
        // Remaining message count incorrect
        ExitNow();
    }

    if (seqId != mSendSeries[seriesId].mMsgSeqId)
    {
        // Received expired ACK, discarded
        ExitNow();
    }

    AckTimerCancel(seriesId);
    mSendSeries[seriesId].mMsgCount--;
    mSendSeries[seriesId].mMsgTransferred++;
    mSendSeries[seriesId].mMsgSeqId++;
    if (mSendSeries[seriesId].mMsgCount == 0)
    {
        FinalizeSendSeries(seriesId);
    }
    else
    {
        IgnoreError(SendDataMessage(seriesId, mSendSeries[seriesId].mDataSize, mSendSeries[seriesId].mAckSize));
        AckTimerFireAt(seriesId, TimerMilli::GetNow() + kAckWaitTimeMs);
    }

exit:
    return;
}

otError BdxPerf::PrepareMessagePayload(otMessage &aMessage, size_t aPlSize)
{
    otError error = OT_ERROR_NONE;

    static constexpr char kPayloadString[] = "OpenThread";

    while (aPlSize > 0)
    {
        size_t length = Min(aPlSize, strlen(kPayloadString));
        SuccessOrExit(error = otMessageAppend(&aMessage, kPayloadString, static_cast<uint16_t>(length)));
        aPlSize -= length;
    }

exit:
    return error;
}

otError BdxPerf::SendDataMessage(uint8_t aSeriesId, uint16_t aDataPlSize, uint16_t aAckPlSize)
{
    otError       error   = OT_ERROR_NONE;
    otMessage    *message = nullptr;
    otMessageInfo messageInfo;
    uint16_t      seqId;

    OT_ASSERT(aSeriesId < kMaxSendSeries);
    seqId = mSendSeries[aSeriesId].mMsgSeqId;

    memset(&messageInfo, 0, sizeof(messageInfo));
    messageInfo.mPeerAddr = mSendSeries[aSeriesId].mPeerAddr.mAddress;
    messageInfo.mPeerPort = mSendSeries[aSeriesId].mPeerAddr.mPort;

    message = mNewMsgApi(mApiContext);
    VerifyOrExit(message != nullptr, error = OT_ERROR_NO_BUFS);

    // Magic Header, Series Id, Data Payload Size, Ack Payload Size, Payload
    SuccessOrExit(error = otMessageAppend(message, &kDataMsgMagicHeader, sizeof(kDataMsgMagicHeader)));
    SuccessOrExit(error = otMessageAppend(message, &aSeriesId, sizeof(aSeriesId)));
    SuccessOrExit(error = otMessageAppend(message, &seqId, sizeof(seqId)));
    SuccessOrExit(error = otMessageAppend(message, &aDataPlSize, sizeof(aDataPlSize)));
    SuccessOrExit(error = otMessageAppend(message, &aAckPlSize, sizeof(aAckPlSize)));
    SuccessOrExit(error = PrepareMessagePayload(*message, aDataPlSize));

    SuccessOrExit(error = mSendMsgApi(*message, messageInfo, mApiContext));
    message = nullptr;

exit:
    if (message != nullptr)
    {
        otMessageFree(message);
    }

    return error;
}

otError BdxPerf::SendAckMessage(const otMessageInfo &aMessageInfo, uint8_t aSeriesId, uint16_t aSeqId, uint16_t aPlSize)
{
    otError       error   = OT_ERROR_NONE;
    otMessage    *message = nullptr;
    otMessageInfo messageInfo;

    memset(&messageInfo, 0, sizeof(messageInfo));
    messageInfo.mPeerAddr = aMessageInfo.mPeerAddr;
    messageInfo.mPeerPort = aMessageInfo.mPeerPort;

    message = mNewMsgApi(mApiContext);
    VerifyOrExit(message != nullptr, error = OT_ERROR_NO_BUFS);

    // Magic Header, Series Id, Payload
    SuccessOrExit(error = otMessageAppend(message, &kAckMsgMagicHeader, sizeof(kAckMsgMagicHeader)));
    SuccessOrExit(error = otMessageAppend(message, &aSeriesId, sizeof(aSeriesId)));
    SuccessOrExit(error = otMessageAppend(message, &aSeqId, sizeof(aSeqId)));
    SuccessOrExit(error = PrepareMessagePayload(*message, aPlSize));

    SuccessOrExit(error = mSendMsgApi(*message, messageInfo, mApiContext));
    message = nullptr;

exit:
    if (message != nullptr)
    {
        otMessageFree(message);
    }

    return error;
}

void BdxPerf::AckTimerFireAt(uint8_t aSeriesId, TimeMilli aTime)
{
    OT_ASSERT(!mSendSeries[aSeriesId].mIsWaitAckTimeSet);

    mSendSeries[aSeriesId].mWaitAckTime      = aTime;
    mSendSeries[aSeriesId].mIsWaitAckTimeSet = true;

    RescheduleAckTimer();
}

void BdxPerf::AckTimerCancel(uint8_t aSeriesId)
{
    mSendSeries[aSeriesId].mWaitAckTime.SetValue(0);
    mSendSeries[aSeriesId].mIsWaitAckTimeSet = false;

    RescheduleAckTimer();
}

void BdxPerf::RescheduleAckTimer(void)
{
    uint8_t   seriesId          = kMaxSendSeries;
    TimeMilli nextFireTime      = TimeMilli(0);
    bool      isNextFireTimeSet = false;

    for (uint8_t i = 0; i < kMaxSendSeries; i++)
    {
        if (mSendSeries[i].mIsWaitAckTimeSet && (!isNextFireTimeSet || mSendSeries[i].mWaitAckTime < nextFireTime))
        {
            nextFireTime      = mSendSeries[i].mWaitAckTime;
            isNextFireTimeSet = true;
            seriesId          = i;
        }
    }

    if (mTimerSeriesId != seriesId)
    {
        if (isNextFireTimeSet)
        {
            mTimerFireAtApi(nextFireTime, mApiContext);
            mTimerSeriesId = seriesId;
        }
        else
        {
            mTimerStopApi(mApiContext);
            mTimerSeriesId = kMaxSendSeries;
        }
    }
}

void BdxPerf::FinalizeSendSeries(uint8_t aSeriesId)
{
    BdxPerfResult result;

    result.mSeriesId = aSeriesId;
    result.mTimeCost = Max(static_cast<uint32_t>(1), TimerMilli::GetNow() - mSendSeries[aSeriesId].mStartTime);
    result.mBytesTransferred =
        mSendSeries[aSeriesId].mMsgTransferred * (mSendSeries[aSeriesId].mDataSize + kDataMsgHeaderSize);
    result.mPacketLoss   = mSendSeries[aSeriesId].mMsgSeqId - mSendSeries[aSeriesId].mMsgTransferred;
    result.mTotalPackets = mSendSeries[aSeriesId].mMsgSeqId;

    mReportBdxPerfResultApi(result, mApiContext);

    mSendSeries[aSeriesId].Reset();
}

CliBdxPerf::CliBdxPerf(otInstance *aInstance, OutputImplementer &aOutputImplementer)
    : Output(aInstance, aOutputImplementer)
    , mTimer(*static_cast<Instance *>(aInstance), HandleTimer, this)
    , mBdxPerf(NewMsg, SendMsg, StartListening, StopListening, TimerFireAt, TimerStop, ReportBdxPerfResult, this)
{
    memset(&mSocket, 0, sizeof(mSocket));
}

template <> otError CliBdxPerf::Process<Cmd("receiver")>(Arg aArgs[])
{
    otError error = OT_ERROR_INVALID_ARGS;

    if (aArgs[0] == "start")
    {
        otSockAddr sockAddr;
        SuccessOrExit(error = aArgs[1].ParseAsIp6Address(sockAddr.mAddress));
        SuccessOrExit(error = aArgs[2].ParseAsUint16(sockAddr.mPort));
        VerifyOrExit(aArgs[3].IsEmpty(), error = OT_ERROR_INVALID_ARGS);

        error = mBdxPerf.ReceiverStart(sockAddr);
    }
    else if (aArgs[0] == "stop")
    {
        error = mBdxPerf.ReceiverStop();
    }

exit:
    return error;
}

template <> otError CliBdxPerf::Process<Cmd("sender")>(Arg aArgs[])
{
    otError error = OT_ERROR_INVALID_ARGS;

    if (aArgs[0] == "start")
    {
        uint8_t             seriesId;
        otSockAddr          peerAddr;
        otSockAddr          sockAddr;
        uint16_t            dataSize;
        uint16_t            ackSize;
        uint16_t            msgCount;
        const otIp6Address *mleid = nullptr;

        SuccessOrExit(error = aArgs[1].ParseAsUint8(seriesId));
        SuccessOrExit(error = aArgs[2].ParseAsIp6Address(peerAddr.mAddress));
        SuccessOrExit(error = aArgs[3].ParseAsUint16(peerAddr.mPort));
        SuccessOrExit(error = aArgs[4].ParseAsUint16(dataSize));
        SuccessOrExit(error = aArgs[5].ParseAsUint16(ackSize));
        SuccessOrExit(error = aArgs[6].ParseAsUint16(msgCount));

        mleid = otThreadGetMeshLocalEid(GetInstancePtr());
        VerifyOrExit(mleid != nullptr, error = OT_ERROR_INVALID_STATE);
        sockAddr.mAddress = *mleid;

        error = mBdxPerf.SenderStart(peerAddr, sockAddr, seriesId, dataSize, ackSize, msgCount);
    }
    else if (aArgs[0] == "stop")
    {
        uint8_t seriesId;
        SuccessOrExit(error = aArgs[1].ParseAsUint8(seriesId));

        error = mBdxPerf.SenderStop(seriesId);
    }

exit:
    return error;
}

otError CliBdxPerf::Process(Arg aArgs[])
{
#define CmdEntry(aCommandString)                                  \
    {                                                             \
        aCommandString, &CliBdxPerf::Process<Cmd(aCommandString)> \
    }

    static constexpr Command kCommands[] = {
        CmdEntry("receiver"),
        CmdEntry("sender"),
    };

    static_assert(BinarySearch::IsSorted(kCommands), "kCommands is not sorted");

    otError        error = OT_ERROR_INVALID_COMMAND;
    const Command *command;

    if (aArgs[0].IsEmpty() || (aArgs[0] == "help"))
    {
        OutputCommandTable(kCommands);
        ExitNow(error = aArgs[0].IsEmpty() ? error : OT_ERROR_NONE);
    }

    command = BinarySearch::Find(aArgs[0].GetCString(), kCommands);
    VerifyOrExit(command != nullptr);

    error = (this->*command->mHandler)(aArgs + 1);

exit:
    return error;
}

void CliBdxPerf::HandleTimer(Timer &aTimer)
{
    static_cast<CliBdxPerf *>(static_cast<TimerMilliContext &>(aTimer).GetContext())->HandleTimer();
}

void CliBdxPerf::HandleTimer(void) { mBdxPerf.HandleTimer(); }

otError CliBdxPerf::StartListening(const otSockAddr &aSockAddr, otUdpReceive aReceiveHandler, void *aContext)
{
    return static_cast<CliBdxPerf *>(aContext)->StartListening(aSockAddr, aReceiveHandler);
}

otError CliBdxPerf::StartListening(const otSockAddr &aSockAddr, otUdpReceive aReceiveHandler)
{
    otError error = OT_ERROR_NONE;
    VerifyOrExit(!otUdpIsOpen(GetInstancePtr(), &mSocket), error = OT_ERROR_ALREADY);

    SuccessOrExit(error = otUdpOpen(GetInstancePtr(), &mSocket, aReceiveHandler, &mBdxPerf));
    SuccessOrExit(error = otUdpBind(GetInstancePtr(), &mSocket, &aSockAddr, OT_NETIF_THREAD));
exit:
    return OT_ERROR_NONE;
}

otError CliBdxPerf::StopListening(void *aContext) { return static_cast<CliBdxPerf *>(aContext)->StopListening(); }

otError CliBdxPerf::StopListening(void) { return otUdpClose(GetInstancePtr(), &mSocket); }

otMessage *CliBdxPerf::NewMsg(void *aContext) { return static_cast<CliBdxPerf *>(aContext)->NewMsg(); }

otMessage *CliBdxPerf::NewMsg(void)
{
    otMessageSettings messageSettings = {true, OT_MESSAGE_PRIORITY_NORMAL};
    return otUdpNewMessage(GetInstancePtr(), &messageSettings);
}

otError CliBdxPerf::SendMsg(otMessage &aMessage, const otMessageInfo &aMessageInfo, void *aContext)
{
    return static_cast<CliBdxPerf *>(aContext)->SendMsg(aMessage, aMessageInfo);
}

otError CliBdxPerf::SendMsg(otMessage &aMessage, const otMessageInfo &aMessageInfo)
{
    return otUdpSend(GetInstancePtr(), &mSocket, &aMessage, &aMessageInfo);
}

void CliBdxPerf::TimerFireAt(TimeMilli aTime, void *aContext)
{
    static_cast<CliBdxPerf *>(aContext)->TimerFireAt(aTime);
}

void CliBdxPerf::TimerFireAt(TimeMilli aTime) { mTimer.FireAt(aTime); }

void CliBdxPerf::TimerStop(void *aContext) { static_cast<CliBdxPerf *>(aContext)->TimerStop(); }

void CliBdxPerf::TimerStop(void) { mTimer.Stop(); }

void CliBdxPerf::ReportBdxPerfResult(const BdxPerf::BdxPerfResult &aBdxPerfResult, void *aContext)
{
    static_cast<CliBdxPerf *>(aContext)->ReportBdxPerfResult(aBdxPerfResult);
}

void CliBdxPerf::ReportBdxPerfResult(const BdxPerf::BdxPerfResult &aBdxPerfResult)
{
    uint32_t timeCostInt           = aBdxPerfResult.mTimeCost / 1000;
    uint32_t timeCostDec           = aBdxPerfResult.mTimeCost % 1000;
    uint32_t packetLossRatePercent = aBdxPerfResult.mPacketLoss * 100 / aBdxPerfResult.mTotalPackets;
    uint32_t tput                  = aBdxPerfResult.mBytesTransferred * 1000 / aBdxPerfResult.mTimeCost;
    OutputLine("BDX Series %d completed successfully.", aBdxPerfResult.mSeriesId);
    OutputLine("Time used: %lu.%03lus", ToUlong(timeCostInt), ToUlong(timeCostDec));
    OutputLine("Total bytes transferred: %lu", ToUlong(aBdxPerfResult.mBytesTransferred));
    OutputLine("Packet loss: %lu, Packet loss rate: %lu%%", ToUlong(aBdxPerfResult.mPacketLoss),
               ToUlong(packetLossRatePercent));
    OutputLine("Average BDX UDP throughput: %lu Bytes/s", ToUlong(tput));
}

} // namespace Cli
} // namespace ot

#endif // OPENTHREAD_CONFIG_BDX_PERF_ENABLE
