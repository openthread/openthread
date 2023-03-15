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

#include "cli/cli_bdx_perf.hpp"

#include "test_platform.h"
#include "test_util.hpp"
#include "common/instance.hpp"
#include "common/message.hpp"
#include "openthread/error.h"

namespace ot {

static constexpr uint32_t kDataMsgMagicHeader = 0x768539e9;
static constexpr uint32_t kAckMsgMagicHeader  = 0x894a58e6;

class TestBdxPerf
{
private:
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
    static constexpr uint8_t kAckMsgHeaderSize = sizeof(uint32_t) + sizeof(uint8_t) + sizeof(uint16_t);

    TestBdxPerf()
    {
        mOtInstance = static_cast<Instance *>(testInitInstance());
        Reset();
    }
    ~TestBdxPerf() = default;

    otMessage *TestNewMsg(void) { return mOtInstance->Get<MessagePool>().Allocate(Message::kTypeIp6); }

    otError TestSendMsg(otMessage &aMessage, const otMessageInfo &aMessageInfo)
    {
        (void)aMessageInfo;
        memset(mBdxPerfSendBuf, 0, sizeof(mBdxPerfSendBuf));
        mBufContentLength =
            otMessageRead(&aMessage, otMessageGetOffset(&aMessage), mBdxPerfSendBuf, sizeof(mBdxPerfSendBuf) - 1);
        mBdxPerfSendBuf[mBufContentLength] = '\0';

        otMessageFree(&aMessage);
        return OT_ERROR_NONE;
    }

    otError TestStartListening(const otSockAddr &aSockAddr, otUdpReceive aReceiveHandler)
    {
        otError error = OT_ERROR_NONE;
        VerifyOrExit(!mListening, error = OT_ERROR_ALREADY);

        mSockAddr              = aSockAddr;
        mBdxPerfReceiveHandler = aReceiveHandler;
        mListening             = true;
    exit:
        return OT_ERROR_NONE;
    }

    otError TestStopListening(void)
    {
        mListening = false;
        return OT_ERROR_NONE;
    }

    void TestTimerFireAt(TimeMilli aTime)
    {
        mBdxPerfFireTime = aTime;
        mIsTimeActive    = true;
    }

    void TestTimerStop(void) { mIsTimeActive = false; }

    void TestReportBdxPerfResult(const Cli::BdxPerf::BdxPerfResult &aBdxPerfResult) { mBdxPerfResult = aBdxPerfResult; }

    void TestReceiver(Cli::BdxPerf &aBdxPerf);

    void TestSender(Cli::BdxPerf &aBdxPerf);

    void FillDataMessageToBdxPerf(otMessage *aMessage,
                                  uint8_t    aSeriesId,
                                  uint16_t   aSeqId,
                                  uint16_t   aDataPlSize,
                                  uint16_t   aAckPlSize);

    void FillAckMessageToBdxPerf(otMessage *aMessage, uint8_t aSeriesId, uint16_t aSeqId, uint16_t aAckPlSize);

    void PrepareMessagePayload(otMessage *aMessage, size_t aPlSize);

    void CheckDataMessage(uint8_t aSeriesId, uint16_t aSeqId, uint16_t aDataPlSize, uint16_t aAckPlSize);
    void CheckAckMessage(uint8_t aSeriesId, uint16_t aSeqId, uint16_t aAckPlSize);
    void CheckBufEmpty(void);
    void Reset(void);

    Instance                   *mOtInstance;
    otSockAddr                  mSockAddr;
    otUdpReceive                mBdxPerfReceiveHandler;
    bool                        mListening;
    uint8_t                     mBdxPerfSendBuf[1500];
    uint16_t                    mBufContentLength;
    TimeMilli                   mBdxPerfFireTime;
    bool                        mIsTimeActive;
    Cli::BdxPerf::BdxPerfResult mBdxPerfResult;

public:
    TestBdxPerf(const TestBdxPerf &)            = delete;
    TestBdxPerf &operator=(const TestBdxPerf &) = delete;

    static TestBdxPerf &GetInstance(void)
    {
        static TestBdxPerf instance;
        return instance;
    }

    static otMessage *TestNewMsg(void *aContext) { return static_cast<TestBdxPerf *>(aContext)->TestNewMsg(); }

    static otError TestSendMsg(otMessage &aMessage, const otMessageInfo &aMessageInfo, void *aContext)
    {
        return static_cast<TestBdxPerf *>(aContext)->TestSendMsg(aMessage, aMessageInfo);
    }

    static otError TestStartListening(const otSockAddr &aSockAddr, otUdpReceive aReceiveHandler, void *aContext)
    {
        return static_cast<TestBdxPerf *>(aContext)->TestStartListening(aSockAddr, aReceiveHandler);
    }

    static otError TestStopListening(void *aContext)
    {
        return static_cast<TestBdxPerf *>(aContext)->TestStopListening();
    }

    static void TestTimerFireAt(TimeMilli aTime, void *aContext)
    {
        static_cast<TestBdxPerf *>(aContext)->TestTimerFireAt(aTime);
    }

    static void TestTimerStop(void *aContext) { static_cast<TestBdxPerf *>(aContext)->TestTimerStop(); }

    static void TestReportBdxPerfResult(const Cli::BdxPerf::BdxPerfResult &aBdxPerfResult, void *aContext)
    {
        static_cast<TestBdxPerf *>(aContext)->TestReportBdxPerfResult(aBdxPerfResult);
    }

    void Test(Cli::BdxPerf &aBdxPerf);
};

void TestBdxPerf::FillDataMessageToBdxPerf(otMessage *aMessage,
                                           uint8_t    aSeriesId,
                                           uint16_t   aSeqId,
                                           uint16_t   aDataPlSize,
                                           uint16_t   aAckPlSize)
{
    SuccessOrQuit(otMessageAppend(aMessage, &kDataMsgMagicHeader, sizeof(kDataMsgMagicHeader)));
    SuccessOrQuit(otMessageAppend(aMessage, &aSeriesId, sizeof(aSeriesId)));
    SuccessOrQuit(otMessageAppend(aMessage, &aSeqId, sizeof(aSeqId)));
    SuccessOrQuit(otMessageAppend(aMessage, &aDataPlSize, sizeof(aDataPlSize)));
    SuccessOrQuit(otMessageAppend(aMessage, &aAckPlSize, sizeof(aAckPlSize)));
    PrepareMessagePayload(aMessage, aDataPlSize);
}

void TestBdxPerf::FillAckMessageToBdxPerf(otMessage *aMessage, uint8_t aSeriesId, uint16_t aSeqId, uint16_t aAckPlSize)
{
    SuccessOrQuit(otMessageAppend(aMessage, &kAckMsgMagicHeader, sizeof(kAckMsgMagicHeader)));
    SuccessOrQuit(otMessageAppend(aMessage, &aSeriesId, sizeof(aSeriesId)));
    SuccessOrQuit(otMessageAppend(aMessage, &aSeqId, sizeof(aSeqId)));
    SuccessOrQuit(otMessageAppend(aMessage, &aAckPlSize, sizeof(aAckPlSize)));
    PrepareMessagePayload(aMessage, aAckPlSize);
}

void TestBdxPerf::PrepareMessagePayload(otMessage *aMessage, size_t aPlSize)
{
    static constexpr char kPayloadString[] = "OpenThread";

    while (aPlSize > 0)
    {
        size_t length = Min(aPlSize, strlen(kPayloadString));
        SuccessOrQuit(otMessageAppend(aMessage, kPayloadString, length));
        aPlSize -= length;
    }
}

void TestBdxPerf::CheckDataMessage(uint8_t aSeriesId, uint16_t aSeqId, uint16_t aDataPlSize, uint16_t aAckPlSize)
{
    uint16_t offset = 0;
    uint32_t magicHeader;
    uint8_t  seriesId;
    uint16_t seqId;
    uint16_t dataPlSize;
    uint16_t ackPlSize;

    VerifyOrQuit(mBufContentLength == kDataMsgHeaderSize + aDataPlSize);

    magicHeader = Encoding::LittleEndian::ReadUint32(mBdxPerfSendBuf);
    VerifyOrQuit(magicHeader == kDataMsgMagicHeader);
    offset += sizeof(magicHeader);

    seriesId = mBdxPerfSendBuf[offset];
    VerifyOrQuit(seriesId == aSeriesId);
    offset += sizeof(seriesId);

    seqId = Encoding::LittleEndian::ReadUint16(mBdxPerfSendBuf + offset);
    printf("seqId:%d aSeqId:%d\n", seqId, aSeqId);
    VerifyOrQuit(seqId == aSeqId);
    offset += sizeof(aSeqId);

    dataPlSize = Encoding::LittleEndian::ReadUint16(mBdxPerfSendBuf + offset);
    VerifyOrQuit(dataPlSize == aDataPlSize);
    offset += sizeof(dataPlSize);

    ackPlSize = Encoding::LittleEndian::ReadUint16(mBdxPerfSendBuf + offset);
    VerifyOrQuit(ackPlSize == aAckPlSize);
    offset += sizeof(ackPlSize);
}

void TestBdxPerf::CheckAckMessage(uint8_t aSeriesId, uint16_t aSeqId, uint16_t aAckPlSize)
{
    uint16_t offset = 0;
    uint32_t magicHeader;
    uint8_t  seriesId;
    uint16_t seqId;

    VerifyOrQuit(mBufContentLength == kAckMsgHeaderSize + aAckPlSize);

    magicHeader = Encoding::LittleEndian::ReadUint32(mBdxPerfSendBuf);
    VerifyOrQuit(magicHeader == kAckMsgMagicHeader);
    offset += sizeof(magicHeader);

    seriesId = mBdxPerfSendBuf[offset];
    VerifyOrQuit(seriesId == aSeriesId);
    offset += sizeof(seriesId);

    seqId = Encoding::LittleEndian::ReadUint16(mBdxPerfSendBuf + offset);
    VerifyOrQuit(seqId == aSeqId);
}

void TestBdxPerf::CheckBufEmpty(void)
{
    VerifyOrQuit(mBufContentLength == 0);
    for (uint8_t i = 0; i < kDataMsgHeaderSize; i++)
    {
        VerifyOrQuit(mBdxPerfSendBuf[i] == 0);
    }
}

void TestBdxPerf::Reset(void)
{
    memset(&mSockAddr, 0, sizeof(mSockAddr));
    mBdxPerfReceiveHandler = nullptr;
    mListening             = false;
    memset(&mBdxPerfSendBuf, 0, sizeof(mBdxPerfSendBuf));
    mBufContentLength = 0;
    mBdxPerfFireTime.SetValue(0);
    mIsTimeActive = false;
}

void TestBdxPerf::TestReceiver(Cli::BdxPerf &aBdxPerf)
{
    printf("TestReceiver \n");

    otError       error = OT_ERROR_NONE;
    otSockAddr    sockAddr;
    otSockAddr    peerAddr;
    otMessage    *message = nullptr;
    otMessageInfo messageInfo;

    // Start the receiver for the first time
    SuccessOrQuit(aBdxPerf.ReceiverStart(sockAddr));

    // Start the receiver for the second time
    error = aBdxPerf.ReceiverStart(sockAddr);
    VerifyOrQuit(error == OT_ERROR_INVALID_STATE);

    // Start the sender when receiver is running
    error = aBdxPerf.SenderStart(peerAddr, sockAddr, 0, 100, 10, 100);
    VerifyOrQuit(error == OT_ERROR_INVALID_STATE);

    // Send a few messages to the receiver
    for (uint16_t i = 0; i < 99; i++)
    {
        message = mOtInstance->Get<MessagePool>().Allocate(Message::kTypeIp6);
        VerifyOrQuit(message != nullptr);

        FillDataMessageToBdxPerf(message, 0, i, 1000, 50);
        mBdxPerfReceiveHandler(&aBdxPerf, message, &messageInfo);
        CheckAckMessage(0, i, 50);

        otMessageFree(message);
    }

    // Stop the receiver
    SuccessOrQuit(aBdxPerf.ReceiverStop());

    // Send a few messages to the receiver, should not get any Acks.
    memset(mBdxPerfSendBuf, 0, sizeof(mBdxPerfSendBuf));
    mBufContentLength = 0;
    for (uint16_t i = 0; i < 99; i++)
    {
        message = mOtInstance->Get<MessagePool>().Allocate(Message::kTypeIp6);
        VerifyOrQuit(message != nullptr);

        FillDataMessageToBdxPerf(message, 0, i, 1000, 50);
        mBdxPerfReceiveHandler(&aBdxPerf, message, &messageInfo);
        CheckBufEmpty();

        otMessageFree(message);
    }
}

void TestBdxPerf::TestSender(Cli::BdxPerf &aBdxPerf)
{
    otError    error = OT_ERROR_NONE;
    otSockAddr sockAddr;
    otSockAddr peerAddr;

    // --------------------------------------------------------------
    // Sender starts with invalid args
    uint8_t  invalidSeriesId = Cli::BdxPerf::kMaxSendSeries + 1;
    uint16_t invalidPlSize   = Cli::BdxPerf::kMaxPlSize + 1;
    uint16_t invalidMsgCount = 0;

    // - Invalid Series Id
    error = aBdxPerf.SenderStart(peerAddr, sockAddr, invalidSeriesId, 100, 10, 100);
    VerifyOrQuit(error == OT_ERROR_INVALID_ARGS);

    // - Invalid Payload Size
    error = aBdxPerf.SenderStart(peerAddr, sockAddr, 0, invalidPlSize, 0, 100);
    VerifyOrQuit(error == OT_ERROR_INVALID_ARGS);
    error = aBdxPerf.SenderStart(peerAddr, sockAddr, 0, 1, invalidPlSize, 100);
    VerifyOrQuit(error == OT_ERROR_INVALID_ARGS);
    error = aBdxPerf.SenderStart(peerAddr, sockAddr, 0, invalidPlSize, invalidPlSize, 100);
    VerifyOrQuit(error == OT_ERROR_INVALID_ARGS);

    // - Invalid Message Count
    error = aBdxPerf.SenderStart(peerAddr, sockAddr, 0, 1000, 50, invalidMsgCount);
    VerifyOrQuit(error == OT_ERROR_INVALID_ARGS);

    // - All Invalid
    error = aBdxPerf.SenderStart(peerAddr, sockAddr, invalidSeriesId, invalidPlSize, invalidPlSize, invalidMsgCount);
    VerifyOrQuit(error == OT_ERROR_INVALID_ARGS);

    // --------------------------------------------------------------
    // Sender starts at wrong state
    // - Start the receiver and then start the sender
    SuccessOrQuit(aBdxPerf.ReceiverStart(sockAddr));
    error = aBdxPerf.SenderStart(peerAddr, sockAddr, 0, 1000, 50, 1);
    VerifyOrQuit(error == OT_ERROR_INVALID_STATE);
    SuccessOrQuit(aBdxPerf.ReceiverStop());

    // --------------------------------------------------------------
    // Sender starts with active series Id
    // - Start the sender and then start the sender with same seriesId again
    SuccessOrQuit(aBdxPerf.SenderStart(peerAddr, sockAddr, 0, 1000, 50, 1));
    error = aBdxPerf.SenderStart(peerAddr, sockAddr, 0, 1000, 50, 1);
    VerifyOrQuit(error == OT_ERROR_ALREADY);

    // - Start another series successfully
    SuccessOrQuit(aBdxPerf.SenderStart(peerAddr, sockAddr, 1, 1000, 50, 1));

    // - Stop the first series and start again successfully
    SuccessOrQuit(aBdxPerf.SenderStop(0));
    SuccessOrQuit(aBdxPerf.SenderStart(peerAddr, sockAddr, 0, 1000, 50, 1));

    SuccessOrQuit(aBdxPerf.SenderStop(0));
    SuccessOrQuit(aBdxPerf.SenderStop(1));
    VerifyOrQuit(!mIsTimeActive);

    // --------------------------------------------------------------
    // One series completes without any packet loss
    printf("TestSender: One series completes without any packet loss\n");
    otMessage    *message = nullptr;
    otMessageInfo messageInfo;
    uint8_t       seriesIdA  = 0;
    uint16_t      dataPlSize = 1000;
    uint16_t      ackPlSize  = 50;
    uint16_t      msgCount   = 500;
    SuccessOrQuit(aBdxPerf.SenderStart(peerAddr, sockAddr, seriesIdA, dataPlSize, ackPlSize, msgCount));
    for (uint16_t i = 0; i < msgCount; i++)
    {
        message = mOtInstance->Get<MessagePool>().Allocate(Message::kTypeIp6);
        VerifyOrQuit(message != nullptr);
        CheckDataMessage(seriesIdA, i, dataPlSize, ackPlSize);
        // Timer should be active at this moment
        VerifyOrQuit(mIsTimeActive);

        // Send Ack to the sender
        FillAckMessageToBdxPerf(message, 0, i, ackPlSize);
        mBdxPerfReceiveHandler(&aBdxPerf, message, &messageInfo);
        otMessageFree(message);
    }
    // Timer should be inactive at this moment
    VerifyOrQuit(!mIsTimeActive);
    VerifyOrQuit(mBdxPerfResult.mSeriesId == seriesIdA);
    VerifyOrQuit(mBdxPerfResult.mBytesTransferred == msgCount * (dataPlSize + kDataMsgHeaderSize));
    VerifyOrQuit(mBdxPerfResult.mPacketLoss == 0);
    VerifyOrQuit(mBdxPerfResult.mTotalPackets == msgCount);

    // --------------------------------------------------------------
    // One series completes with a few packet loss
    printf("TestSender: One series completes with a few packet loss\n");
    SuccessOrQuit(aBdxPerf.SenderStart(peerAddr, sockAddr, seriesIdA, dataPlSize, ackPlSize, msgCount));
    for (uint16_t i = 0; i < msgCount; i++)
    {
        CheckDataMessage(seriesIdA, i, dataPlSize, ackPlSize);
        // Timer should be active at this moment
        VerifyOrQuit(mIsTimeActive);

        // Make half of the messages lost
        if (i & 1)
        {
            message = mOtInstance->Get<MessagePool>().Allocate(Message::kTypeIp6);
            VerifyOrQuit(message != nullptr);
            // Send Ack to the sender
            FillAckMessageToBdxPerf(message, 0, i, ackPlSize);
            mBdxPerfReceiveHandler(&aBdxPerf, message, &messageInfo);
            otMessageFree(message);
        }
        else
        {
            // Timer fires
            aBdxPerf.HandleTimer();
        }
    }
    // Timer should be inactive at this moment
    VerifyOrQuit(!mIsTimeActive);
    VerifyOrQuit(mBdxPerfResult.mSeriesId == seriesIdA);
    VerifyOrQuit(mBdxPerfResult.mBytesTransferred == msgCount * (dataPlSize + kDataMsgHeaderSize) / 2);
    VerifyOrQuit(mBdxPerfResult.mPacketLoss == msgCount / 2);
    VerifyOrQuit(mBdxPerfResult.mTotalPackets == msgCount);

    // --------------------------------------------------------------
    // Two series completes without any packet loss
    printf("TestSender: Two series completes without any packet loss\n");
    seriesIdA            = 0;
    uint8_t seriesIdB    = 1;
    uint8_t lastSeriesId = 0;
    msgCount             = 10;
    // Shuffle the receiving sequence
    uint8_t  ackRecvSeq[20] = {0, 1, 0, 1, 1, 0, 1, 0, 0, 0, 1, 1, 0, 0, 0, 1, 1, 1, 0, 1};
    uint16_t seqs[2]        = {0, 0};
    SuccessOrQuit(aBdxPerf.SenderStart(peerAddr, sockAddr, seriesIdA, dataPlSize, ackPlSize, msgCount));
    SuccessOrQuit(aBdxPerf.SenderStart(peerAddr, sockAddr, seriesIdB, dataPlSize, ackPlSize, msgCount));
    lastSeriesId = seriesIdB;
    for (uint16_t i = 0; i < msgCount * 2; i++)
    {
        uint8_t id = ackRecvSeq[i];
        // Send Ack to the sender
        message = mOtInstance->Get<MessagePool>().Allocate(Message::kTypeIp6);
        VerifyOrQuit(message != nullptr);
        FillAckMessageToBdxPerf(message, id, seqs[id], ackPlSize);
        mBdxPerfReceiveHandler(&aBdxPerf, message, &messageInfo);
        otMessageFree(message);

        seqs[id]++;

        if (seqs[id] == msgCount)
        {
            VerifyOrQuit(mBdxPerfResult.mSeriesId == id);
            VerifyOrQuit(mBdxPerfResult.mBytesTransferred == msgCount * (dataPlSize + kDataMsgHeaderSize));
            VerifyOrQuit(mBdxPerfResult.mPacketLoss == 0);
            VerifyOrQuit(mBdxPerfResult.mTotalPackets == msgCount);
        }
        else
        {
            CheckDataMessage(id, seqs[id], dataPlSize, ackPlSize);
            // Timer should be active at this moment
            VerifyOrQuit(mIsTimeActive);
        }
    }
    // Timer should be inactive at this moment
    VerifyOrQuit(!mIsTimeActive);
}

void TestBdxPerf::Test(Cli::BdxPerf &aBdxPerf)
{
    TestReceiver(aBdxPerf);
    Reset();
    TestSender(aBdxPerf);
}

} // namespace ot

int main(void)
{
    using namespace ot;

    TestBdxPerf &testBdxPerf = TestBdxPerf::GetInstance();
    Cli::BdxPerf bdxPerf =
        Cli::BdxPerf(TestBdxPerf::TestNewMsg, TestBdxPerf::TestSendMsg, TestBdxPerf::TestStartListening,
                     TestBdxPerf::TestStopListening, TestBdxPerf::TestTimerFireAt, TestBdxPerf::TestTimerStop,
                     TestBdxPerf::TestReportBdxPerfResult, &testBdxPerf);

    testBdxPerf.Test(bdxPerf);

    printf("All tests passed\n");
    return 0;
}
