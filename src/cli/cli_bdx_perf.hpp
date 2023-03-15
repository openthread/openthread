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

/**
 * @file
 *   This file contains definitions for BDX Perf feature classes.
 */

#ifndef CLI_BDX_PERF_HPP_
#define CLI_BDX_PERF_HPP_

#include "openthread-core-config.h"

#include <openthread/udp.h>

#include "cli/cli_output.hpp"
#include "common/time.hpp"
#include "common/timer.hpp"
#include "net/ip6_headers.hpp"
#include "net/ip6_types.hpp"
#include "net/udp6.hpp"
#include "utils/parse_cmdline.hpp"

namespace ot {
namespace Cli {

/**
 * This class implements the BDX Perf logic.
 *
 */
class BdxPerf
{
public:
    static constexpr uint8_t kMaxSendSeries = 3;
    // Magic Header (4 bytes)
    // Series Id (1 byte)
    // Sequence Id (2 byte)
    // Data Payload Size (2 bytes)
    // Ack Payload Size (2 bytes)
    static constexpr uint8_t kDataMsgHeaderSize =
        sizeof(uint32_t) + sizeof(uint8_t) + sizeof(uint16_t) + sizeof(uint16_t) + sizeof(uint16_t);
    // Magic Header (4 bytes)
    // Series Id (1 byte)
    // Sequence Id (2 byte)
    static constexpr uint8_t  kAckMsgHeaderSize = sizeof(uint32_t) + sizeof(uint8_t) + sizeof(uint16_t);
    static constexpr uint16_t kMaxPlSize =
        Ip6::kMaxDatagramLength - sizeof(Ip6::Header) - sizeof(Ip6::Udp::Header) - kDataMsgHeaderSize;

    struct BdxPerfResult
    {
        uint8_t  mSeriesId;
        uint32_t mTimeCost;
        uint32_t mBytesTransferred;
        uint32_t mPacketLoss;
        uint32_t mTotalPackets;
    };

    typedef otMessage *(*NewMsgApi)(void *aContext);
    typedef otError (*SendMsgApi)(otMessage &aMessage, const otMessageInfo &aMessageInfo, void *aContext);
    typedef otError (*StartListeningApi)(const otSockAddr &aSockAddr, otUdpReceive aReceiveHandler, void *aContext);
    typedef otError (*StopListeningApi)(void *aContext);
    typedef void (*TimerFireAtApi)(TimeMilli aTime, void *aContext);
    typedef void (*TimerStopApi)(void *aContext);
    typedef void (*ReportBdxPerfResultApi)(const BdxPerfResult &aBdxPerfResult, void *aContext);

    explicit BdxPerf(NewMsgApi              aNewMsgApi,
                     SendMsgApi             aSendMsgApi,
                     StartListeningApi      aStartListeningApi,
                     StopListeningApi       aStopListeningApi,
                     TimerFireAtApi         aTimerFireAtApi,
                     TimerStopApi           aTimerStopApi,
                     ReportBdxPerfResultApi aReportBdxPerfResult,
                     void                  *aApiContext);

    otError ReceiverStart(otSockAddr &aSockAddr);
    otError ReceiverStop(void);
    otError SenderStart(const otSockAddr &aPeerAddr,
                        const otSockAddr &aSockAddr,
                        uint8_t           aSeriesId,
                        uint16_t          aDataSize,
                        uint16_t          aAckSize,
                        uint16_t          aMsgCount);
    otError SenderStop(uint8_t aSeriesId);
    void    HandleTimer(void);

private:
    static constexpr uint32_t kAckWaitTimeMs = 2000;

    enum State : uint8_t
    {
        kIdle,
        kSenderOn,
        kReceiverOn,
    };

    struct SenderSeries
    {
        uint16_t   mDataSize;
        uint16_t   mAckSize;
        uint16_t   mMsgCount;
        uint16_t   mMsgSeqId;
        uint16_t   mMsgTransferred;
        TimeMilli  mStartTime;
        TimeMilli  mWaitAckTime;
        bool       mIsWaitAckTimeSet;
        otSockAddr mPeerAddr;

        void Reset(void)
        {
            mDataSize       = 0;
            mAckSize        = 0;
            mMsgCount       = 0;
            mMsgSeqId       = 0;
            mMsgTransferred = 0;
            mStartTime.SetValue(0);
            mWaitAckTime.SetValue(0);
            mIsWaitAckTimeSet = false;
            memset(&mPeerAddr, 0, sizeof(mPeerAddr));
        }
    };

    static void HandleReceivedMessage(void *aContext, otMessage *aMessage, const otMessageInfo *aMessageInfo);
    void        HandleReceivedMessage(otMessage *aMessage, const otMessageInfo *aMessageInfo);
    void        HandleReceivedData(otMessage *aMessage, const otMessageInfo *aMessageInfo);
    void        HandleReceivedAck(otMessage *aMessage, const otMessageInfo *aMessageInfo);

    static otError PrepareMessagePayload(otMessage &aMessage, size_t aPlSize);
    otError        SendDataMessage(uint8_t aSeriesId, uint16_t aDataPlSize, uint16_t aAckPlSize);
    otError SendAckMessage(const otMessageInfo &aMessageInfo, uint8_t aSeriesId, uint16_t aSeqId, uint16_t aPlSize);
    void    AckTimerFireAt(uint8_t aSeriesId, TimeMilli aTime);
    void    AckTimerCancel(uint8_t aSeriesId);
    void    RescheduleAckTimer(void);
    void    FinalizeSendSeries(uint8_t aSeriesId);

    const NewMsgApi              mNewMsgApi;
    const SendMsgApi             mSendMsgApi;
    const StartListeningApi      mStartListeningApi;
    const StopListeningApi       mStopListeningApi;
    const TimerFireAtApi         mTimerFireAtApi;
    const TimerStopApi           mTimerStopApi;
    const ReportBdxPerfResultApi mReportBdxPerfResultApi;
    void *const                  mApiContext;

    State        mState;
    SenderSeries mSendSeries[kMaxSendSeries];
    uint8_t      mTimerSeriesId;
};

class CliBdxPerf : private Output
{
public:
    typedef Utils::CmdLineParser::Arg Arg;

    CliBdxPerf(otInstance *aInstance, OutputImplementer &aOutputImplementer);

    otError Process(Arg aArgs[]);

private:
    using Command = CommandEntry<CliBdxPerf>;

    template <CommandId kCommandId> otError Process(Arg aArgs[]);

    static void HandleTimer(Timer &aTimer);
    void        HandleTimer(void);

    // APIs for BdxPerf
    static otMessage *NewMsg(void *aContext);
    otMessage        *NewMsg(void);
    static otError    SendMsg(otMessage &aMessage, const otMessageInfo &aMessageInfo, void *aContext);
    otError           SendMsg(otMessage &aMessage, const otMessageInfo &aMessageInfo);
    static otError    StartListening(const otSockAddr &aSockAddr, otUdpReceive aReceiveHandler, void *aContext);
    otError           StartListening(const otSockAddr &aSockAddr, otUdpReceive aReceiveHandler);
    static otError    StopListening(void *aContext);
    otError           StopListening(void);
    static void       TimerFireAt(TimeMilli aTime, void *aContext);
    void              TimerFireAt(TimeMilli aTime);
    static void       TimerStop(void *aContext);
    void              TimerStop(void);
    static void       ReportBdxPerfResult(const BdxPerf::BdxPerfResult &aBdxPerfResult, void *aContext);
    void              ReportBdxPerfResult(const BdxPerf::BdxPerfResult &aBdxPerfResult);

    otUdpSocket       mSocket;
    TimerMilliContext mTimer;

    BdxPerf mBdxPerf;
};

} // namespace Cli
} // namespace ot

#endif // CLI_BDX_PERF_HPP_
