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

#include <openthread/config.h>

#include "common/array.hpp"
#include "common/code_utils.hpp"
#include "common/instance.hpp"

#include "ncp/ncp_base.hpp"
#include "openthread/link_raw.h"

#include "test_platform.h"
#include "test_util.hpp"

using namespace ot;
using namespace ot::Ncp;

enum
{
    kTestBufferSize = 800
};

enum
{
    kTestMacScanChannelMask = 0x01
};

OT_TOOL_PACKED_BEGIN
struct RadioMessage
{
    uint8_t mChannel;
    uint8_t mPsdu[OT_RADIO_FRAME_MAX_SIZE];
} OT_TOOL_PACKED_END;

static struct RadioMessage sDefaultMessage;
static otRadioFrame        sDefaultFrame;

static otRadioFrame *sTxFrame;

otRadioFrame *otPlatRadioGetTransmitBuffer(otInstance *aInstance) { return sTxFrame; }

class TestNcp : public NcpBase
{
public:
    explicit TestNcp(ot::Instance *aInstance)
        : mLastHeader(0)
        , mLastStatus(0)
        , NcpBase(aInstance)
    {
        memset(mMsgBuffer, 0, kTestBufferSize);
        mTxFrameBuffer.SetFrameAddedCallback(HandleFrameAddedToNcpBuffer, this);
        mTxFrameBuffer.SetFrameRemovedCallback(nullptr, this);
    };

    static void HandleFrameAddedToNcpBuffer(void                    *aContext,
                                            Spinel::Buffer::FrameTag aTag,
                                            Spinel::Buffer::Priority aPriority,
                                            Spinel::Buffer          *aBuffer)
    {
        OT_UNUSED_VARIABLE(aTag);
        OT_UNUSED_VARIABLE(aPriority);

        static_cast<TestNcp *>(aContext)->HandleFrameAddedToNcpBuffer(aBuffer);
    }

    void HandleFrameAddedToNcpBuffer(Spinel::Buffer *aBuffer)
    {
        static const size_t display_size = 64;

        memset(mMsgBuffer, 0, kTestBufferSize);
        SuccessOrQuit(aBuffer->OutFrameBegin());
        aBuffer->OutFrameRead(kTestBufferSize, mMsgBuffer);
        SuccessOrQuit(aBuffer->OutFrameRemove());

        // DumpBuffer("Received Buffer", mMsgBuffer, display_size);

        updateSpinelStatus();
    }

    void Receive(uint8_t *aBuffer, size_t bufferSize) { HandleReceive(aBuffer, static_cast<uint16_t>(bufferSize)); }

    void processTransmit()
    {
        LinkRawTransmitDone(sTxFrame, nullptr, OT_ERROR_NONE);
        /* Pending commands tasklet is posted by Transmit Done callback but not handled.
           Executing callback here to handle pending commands in queue */
        processPendingCommands();
    };

    void processEnergyScan()
    {
        LinkRawEnergyScanDone(Radio::kInvalidRssi);

        /* Pending commands tasklet is posted by Energy Scan Done callback but not handled.
           Executing callback here to handle pending commands in queue */
        processPendingCommands();
    }

    void processPendingCommands()
    {
#if OPENTHREAD_CONFIG_MULTIPAN_RCP_ENABLE && (OPENTHREAD_RADIO || OPENTHREAD_CONFIG_LINK_RAW_ENABLE)
        HandlePendingCommands();
#endif
    }

    void updateSpinelStatus()
    {
        Spinel::Decoder decoder;

        uint8_t      header;
        unsigned int command;
        unsigned int propKey;
        unsigned int status;

        decoder.Init(mMsgBuffer, kTestBufferSize);

        SuccessOrQuit(decoder.ReadUint8(mLastHeader));
        SuccessOrQuit(decoder.ReadUintPacked(command));
        SuccessOrQuit(decoder.ReadUintPacked(propKey));
        SuccessOrQuit(decoder.ReadUintPacked(status));

        mLastStatus = static_cast<uint32_t>(status);
    }
    uint32_t getSpinelStatus() const { return mLastStatus; }

    uint8_t getLastIid() const
    {
        /* Return as SPINEL_HEADER_IID_N format without shift */
        return SPINEL_HEADER_IID_MASK & mLastHeader;
    }

    uint8_t getLastTid() { return SPINEL_HEADER_GET_TID(mLastHeader); }

    bool gotResponse(uint8_t aIid, uint8_t aTid) { return ((aIid == getLastIid()) && (aTid == getLastTid())); }

    size_t getPendingQueueSize()
    {
#if OPENTHREAD_CONFIG_MULTIPAN_RCP_ENABLE && (OPENTHREAD_RADIO || OPENTHREAD_CONFIG_LINK_RAW_ENABLE)
        return GetPendingCommandQueueSize();
#else
        return 0;
#endif
    };

    size_t getMaxPendingQueueSize()
    {
#if OPENTHREAD_CONFIG_MULTIPAN_RCP_ENABLE && (OPENTHREAD_RADIO || OPENTHREAD_CONFIG_LINK_RAW_ENABLE)
        return kPendingCommandQueueSize;
#else
        return 0;
#endif
    };

private:
    uint8_t  mLastHeader;
    uint32_t mLastStatus;
    uint8_t  mMsgBuffer[kTestBufferSize];
};

class TestHost
{
public:
    TestHost(TestNcp *aNcp, uint8_t aIid)
        : mNcp(aNcp)
        , mIid(aIid)
        , mTid(0)
        , mBuffer(mBuf, kTestBufferSize)
        , mEncoder(mBuffer)
        , mOffset(0)
    {
        memset(mBuf, 0, kTestBufferSize);
    };

    void createLinkEnableFrame(bool isEnabled)
    {
        startFrame(SPINEL_CMD_PROP_VALUE_SET, SPINEL_PROP_PHY_ENABLED);
        SuccessOrQuit(mEncoder.WriteBool(isEnabled));
        endFrame("Enable Frame");
    }

    void createTransmitFrame()
    {
        startFrame(SPINEL_CMD_PROP_VALUE_SET, SPINEL_PROP_STREAM_RAW);

        SuccessOrQuit(mEncoder.WriteDataWithLen(sDefaultFrame.mPsdu, sDefaultFrame.mLength));
        SuccessOrQuit(mEncoder.WriteUint8(sDefaultFrame.mChannel));
        SuccessOrQuit(mEncoder.WriteUint8(sDefaultFrame.mInfo.mTxInfo.mMaxCsmaBackoffs));
        SuccessOrQuit(mEncoder.WriteUint8(sDefaultFrame.mInfo.mTxInfo.mMaxFrameRetries));
        SuccessOrQuit(mEncoder.WriteBool(sDefaultFrame.mInfo.mTxInfo.mCsmaCaEnabled));
        SuccessOrQuit(mEncoder.WriteBool(sDefaultFrame.mInfo.mTxInfo.mIsHeaderUpdated));
        SuccessOrQuit(mEncoder.WriteBool(sDefaultFrame.mInfo.mTxInfo.mIsARetx));
        SuccessOrQuit(mEncoder.WriteBool(sDefaultFrame.mInfo.mTxInfo.mIsSecurityProcessed));
        SuccessOrQuit(mEncoder.WriteUint32(sDefaultFrame.mInfo.mTxInfo.mTxDelay));
        SuccessOrQuit(mEncoder.WriteUint32(sDefaultFrame.mInfo.mTxInfo.mTxDelayBaseTime));

        endFrame("Transmit Frame");
    }

    void createScanChannelMaskFrame(uint8_t aMask)
    {
        startFrame(SPINEL_CMD_PROP_VALUE_SET, SPINEL_PROP_MAC_SCAN_MASK);
        SuccessOrQuit(mEncoder.WriteUint8(aMask));
        endFrame("Channel Mask Frame");
    }

    void createMacScanFrame()
    {
        uint8_t state = SPINEL_SCAN_STATE_ENERGY;

        startFrame(SPINEL_CMD_PROP_VALUE_SET, SPINEL_PROP_MAC_SCAN_STATE);
        SuccessOrQuit(mEncoder.WriteUint8(state));
        endFrame("Scan State Frame");
    }

    void createReadStatusFrame()
    {
        startFrame(SPINEL_CMD_PROP_VALUE_GET, SPINEL_PROP_LAST_STATUS);
        endFrame("Read Status Frame");
    }

    void enableRawLink()
    {
        static const bool isLinkEnabled = true;
        createLinkEnableFrame(isLinkEnabled);
        sendToRcp();
    }

    void disableRawLink()
    {
        static const bool isLinkEnabled = false;
        createLinkEnableFrame(isLinkEnabled);
        sendToRcp();
    }

    spinel_status_t startTransmit()
    {
        uint8_t thisTid = mTid;
        createTransmitFrame();
        sendToRcp();
        prepareResponse(thisTid);
        return static_cast<spinel_status_t>(mNcp->getSpinelStatus());
    };

    void setScanChannelMask(uint32_t aMask)
    {
        createScanChannelMaskFrame(aMask);
        sendToRcp();
    }

    uint32_t startEnergyScan()
    {
        uint8_t thisTid = mTid;
        createMacScanFrame();
        sendToRcp();
        prepareResponse(thisTid);
        return mNcp->getSpinelStatus();
    }

    void getCommandStatus()
    {
        createReadStatusFrame();
        sendToRcp();
    }

    void finishTransmit()
    {
        /* Reset instance submac state to sleep by resetting link
           This is needed for a second transmit command to succeed
           as the HandleTimer method will not be called to reset the submac */
        disableRawLink();
        enableRawLink();

        /* Proceed with transmit done callback from ncp */
        mNcp->processTransmit();
    };

private:
    void startFrame(unsigned int aCommand, spinel_prop_key_t aKey)
    {
        uint8_t spinelHeader = SPINEL_HEADER_FLAG | mIid | mTid;

        SuccessOrQuit(mEncoder.BeginFrame(Spinel::Buffer::kPriorityLow));
        SuccessOrQuit(mEncoder.WriteUint8(spinelHeader));
        SuccessOrQuit(mEncoder.WriteUintPacked(aCommand));
        SuccessOrQuit(mEncoder.WriteUintPacked(aKey));
    }

    void endFrame(const char *aTextMessage)
    {
        static const uint16_t display_length = 64;
        SuccessOrQuit(mEncoder.EndFrame());
        // DumpBuffer(aTextMessage, mBuf, display_length);
    }

    void sendToRcp()
    {
        static const uint8_t data_offset = 2;
        size_t               frame_len   = mBuffer.OutFrameGetLength();

        mOffset += data_offset;

        mNcp->Receive(mBuf + mOffset, frame_len);

        mTid = SPINEL_GET_NEXT_TID(mTid);
        SuccessOrQuit(mBuffer.OutFrameRemove());

        mOffset += frame_len;
        mOffset %= kTestBufferSize;
    }

    void prepareResponse(uint8_t aTid)
    {
        /* Some spinel commands immediately send queued responses when command is complete
        while others require a separate command to the ncp in order to receive the response.
        If a response is needed and not immediately received. Issue a command to update the status. */

        if (!mNcp->gotResponse(mIid, aTid))
        {
            getCommandStatus();
        }
    }

    TestNcp        *mNcp;
    uint8_t         mIid;
    uint8_t         mTid;
    uint8_t         mBuf[kTestBufferSize];
    Spinel::Buffer  mBuffer;
    Spinel::Encoder mEncoder;
    size_t          mOffset;
};

void TestNcpBaseTransmitWithLinkRawDisabled()
{
    printf("\tTransmit With Link Raw Disabled - ");
    ot::Instance *instance = testInitInstance();
    VerifyOrQuit(instance != nullptr);

    // TestNcp *ncp = std::make_shared<TestNcp >(instance);
    TestNcp  ncp(instance);
    TestHost host(&ncp, SPINEL_HEADER_IID_0);

    host.disableRawLink();

    /* Test that the response status is Invalid State when transmit is skipped due to disabled link */
    VerifyOrQuit(host.startTransmit() == SPINEL_STATUS_INVALID_STATE);
    VerifyOrQuit(ncp.getPendingQueueSize() == 0);

    testFreeInstance(instance);
    printf("PASS\n");
}

void TestNcpBaseTransmitWithLinkRawEnabled()
{
    printf("\tTransmit With Link Raw Enabled - ");
    ot::Instance *instance = testInitInstance();
    VerifyOrQuit(instance != nullptr);

    TestNcp  ncp(instance);
    TestHost host(&ncp, SPINEL_HEADER_IID_0);

    host.enableRawLink();

    /* Test that the response status is OK when transmit is started successfully */
    VerifyOrQuit(host.startTransmit() == SPINEL_STATUS_OK);
    VerifyOrQuit(ncp.getPendingQueueSize() == 0);

    host.finishTransmit();

    testFreeInstance(instance);
    printf("PASS\n");
}

void TestNcpBaseTransmitWithNoBuffers()
{
    printf("\tTransmit With No Buffers - ");

    /* Initialize instance without an available tx buffer */
    sTxFrame = nullptr;

    ot::Instance *instance = testInitInstance();
    VerifyOrQuit(instance != nullptr);

    TestNcp  ncp(instance);
    TestHost host(&ncp, SPINEL_HEADER_IID_0);

    host.enableRawLink();

    /* Test that the response status is NOMEM when transmit is started without an available TxBuffer */
    VerifyOrQuit(host.startTransmit() == SPINEL_STATUS_NOMEM);
    VerifyOrQuit(ncp.getPendingQueueSize() == 0);

    testFreeInstance(instance);

    /* Reset static tx buffer reference for remaining tests */
    sTxFrame = &sDefaultFrame;
    printf("PASS\n");
}

void TestNcpBaseTransmitWhileLinkIsBusy()
{
    printf("\tTransmit While Link Is Busy - ");
    ot::Instance *instance = testInitInstance();
    VerifyOrQuit(instance != nullptr);

    TestNcp  ncp(instance);
    TestHost host(&ncp, SPINEL_HEADER_IID_0);

    host.enableRawLink();

    VerifyOrQuit(host.startTransmit() == SPINEL_STATUS_OK);
    VerifyOrQuit(ncp.getPendingQueueSize() == 0);

    /* Test that the response status is OK when transmit is already in progress
       Test that requesting a transmit when already in progress enqueues the command */
    VerifyOrQuit(host.startTransmit() == SPINEL_STATUS_OK);
    VerifyOrQuit(ncp.getPendingQueueSize() == 1);

    VerifyOrQuit(host.startTransmit() == SPINEL_STATUS_OK);
    VerifyOrQuit(ncp.getPendingQueueSize() == 2);

    /* Test that transmit command is dequeued when transmit is complete */
    host.finishTransmit();
    VerifyOrQuit(ncp.getPendingQueueSize() == 1);

    host.finishTransmit();
    VerifyOrQuit(ncp.getPendingQueueSize() == 0);

    host.finishTransmit();
    VerifyOrQuit(ncp.getPendingQueueSize() == 0);

    testFreeInstance(instance);
    printf("PASS\n");
}

void TestNcpBaseExceedPendingCommandQueueSize()
{
    printf("\tExceed Pending Command Queue Size - ");
    ot::Instance *instance = testInitInstance();
    VerifyOrQuit(instance != nullptr);

    TestNcp  ncp(instance);
    TestHost host(&ncp, SPINEL_HEADER_IID_0);

    host.enableRawLink();

    /* Test that the response status is OK and queue size increases until its maximum size */
    for (size_t i = 0; i <= ncp.getMaxPendingQueueSize(); i++)
    {
        VerifyOrQuit(host.startTransmit() == SPINEL_STATUS_OK);
        VerifyOrQuit(ncp.getPendingQueueSize() == i);
    }

    /* Test that the response status is NOMEM when requesting transmit with a full queue */
    VerifyOrQuit(host.startTransmit() == SPINEL_STATUS_NOMEM);
    VerifyOrQuit(ncp.getPendingQueueSize() == ncp.getMaxPendingQueueSize());

    /* Test that queue size decreases from maximum size to empty */
    for (size_t i = ncp.getMaxPendingQueueSize(); i > 0; i--)
    {
        VerifyOrQuit(ncp.getPendingQueueSize() == i);
        host.finishTransmit();
    }

    VerifyOrQuit(ncp.getPendingQueueSize() == 0);
    host.finishTransmit();
    VerifyOrQuit(ncp.getPendingQueueSize() == 0);

    testFreeInstance(instance);
    printf("PASS\n");
}

void TestNcpBaseEnergyScanWithLinkRawDisabled()
{
    printf("\tEnergy Scan With Link Raw Disabled - ");
    ot::Instance *instance = testInitInstance();
    VerifyOrQuit(instance != nullptr);

    TestNcp  ncp(instance);
    TestHost host(&ncp, SPINEL_HEADER_IID_0);

    host.disableRawLink();

    /* Test that the response status is OK even though energy scan is skipped due to disabled link */
    VerifyOrQuit((spinel_status_t)host.startEnergyScan() == SPINEL_STATUS_OK);
    VerifyOrQuit(ncp.getPendingQueueSize() == 0);

    /* Test that the queue size doesn't increase even though status is OK */
    VerifyOrQuit((spinel_status_t)host.startEnergyScan() == SPINEL_STATUS_OK);
    VerifyOrQuit(ncp.getPendingQueueSize() == 0);

    testFreeInstance(instance);
    printf("PASS\n");
}

void TestNcpBaseEnergyScanWithLinkRawEnabled()
{
    printf("\tEnergy Scan With Link Raw Enabled - ");
    ot::Instance *instance = testInitInstance();
    VerifyOrQuit(instance != nullptr);

    TestNcp  ncp(instance);
    TestHost host(&ncp, SPINEL_HEADER_IID_0);

    host.enableRawLink();

    /* Test that the response status is Invalid Argument when channel mask is not set */
    VerifyOrQuit((spinel_status_t)host.startEnergyScan() == SPINEL_STATUS_INVALID_ARGUMENT);
    VerifyOrQuit(ncp.getPendingQueueSize() == 0);

    testFreeInstance(instance);
    printf("PASS\n");
}

void TestNcpBaseEnergyScanWithLinkRawEnabledAndMaskSet()
{
    printf("\tEnergy Scan With Link Raw Enabled And Mask Set - ");
    ot::Instance *instance = testInitInstance();
    VerifyOrQuit(instance != nullptr);

    TestNcp  ncp(instance);
    TestHost host(&ncp, SPINEL_HEADER_IID_0);

    host.enableRawLink();
    host.setScanChannelMask(kTestMacScanChannelMask);

    /* Test that the response status is Energy Scan State when energy scan starts successfully */
    VerifyOrQuit((spinel_scan_state_t)host.startEnergyScan() == SPINEL_SCAN_STATE_ENERGY);
    VerifyOrQuit(ncp.getPendingQueueSize() == 0);

    ncp.processEnergyScan();

    testFreeInstance(instance);
    printf("PASS\n");
}

void TestNcpBaseEnergyScanWhileLinkIsBusy()
{
    printf("\tEnergy Scan While Link Is Busy - ");
    ot::Instance *instance = testInitInstance();
    VerifyOrQuit(instance != nullptr);

    TestNcp  ncp(instance);
    TestHost host(&ncp, SPINEL_HEADER_IID_0);

    otError         ret        = OT_ERROR_NONE;
    spinel_status_t spinel_ret = SPINEL_STATUS_OK;

    host.enableRawLink();
    host.setScanChannelMask(kTestMacScanChannelMask);

    VerifyOrQuit((spinel_scan_state_t)host.startEnergyScan() == SPINEL_SCAN_STATE_ENERGY);
    VerifyOrQuit(ncp.getPendingQueueSize() == 0);

    /* Test that the response status is Invalid State when energy scan is already in progress
       Test that requesting an energy scan when already in progress does not enqueue the command */
    VerifyOrQuit((spinel_status_t)host.startEnergyScan() == SPINEL_STATUS_INVALID_STATE);
    VerifyOrQuit(ncp.getPendingQueueSize() == 0);

    VerifyOrQuit((spinel_status_t)host.startEnergyScan() == SPINEL_STATUS_INVALID_STATE);
    VerifyOrQuit(ncp.getPendingQueueSize() == 0);

    ncp.processEnergyScan();
    VerifyOrQuit(ncp.getPendingQueueSize() == 0);

    /* Test that the response status is Energy Scan State when previous energy scan is complete */
    VerifyOrQuit((spinel_scan_state_t)host.startEnergyScan() == SPINEL_SCAN_STATE_ENERGY);
    VerifyOrQuit(ncp.getPendingQueueSize() == 0);

    ncp.processEnergyScan();
    VerifyOrQuit(ncp.getPendingQueueSize() == 0);

    testFreeInstance(instance);
    printf("PASS\n");
}

void TestNcpBaseEnergyScanWhileTransmitting()
{
    printf("\tEnergy Scan While Transmitting - ");
    ot::Instance *instance = testInitInstance();
    VerifyOrQuit(instance != nullptr);

    TestNcp  ncp(instance);
    TestHost host(&ncp, SPINEL_HEADER_IID_0);

    host.enableRawLink();

    VerifyOrQuit(host.startTransmit() == SPINEL_STATUS_OK);
    VerifyOrQuit(ncp.getPendingQueueSize() == 0);

    host.setScanChannelMask(kTestMacScanChannelMask);

    /* Test that the response status is Idle Scan State when transmit is in progress
       Test that requesting energy scan while transmit is active enqueues the command */
    VerifyOrQuit((spinel_scan_state_t)host.startEnergyScan() == SPINEL_SCAN_STATE_IDLE);
    VerifyOrQuit(ncp.getPendingQueueSize() == 1);

    VerifyOrQuit((spinel_scan_state_t)host.startEnergyScan() == SPINEL_SCAN_STATE_IDLE);
    VerifyOrQuit(ncp.getPendingQueueSize() == 2);

    /* Test that energy scan command is dequeued when transmit is complete */
    host.finishTransmit();
    VerifyOrQuit(ncp.getPendingQueueSize() == 1);

    /* Test that energy scan command is dequeued when energy scan is complete */
    ncp.processEnergyScan();
    VerifyOrQuit(ncp.getPendingQueueSize() == 0);

    ncp.processEnergyScan();
    VerifyOrQuit(ncp.getPendingQueueSize() == 0);

    testFreeInstance(instance);
    printf("PASS\n");
}

void TestNcpBaseTransmitWhileScanning()
{
    printf("\tTransmit While Scanning - ");
    ot::Instance *instance = testInitInstance();
    VerifyOrQuit(instance != nullptr);

    TestNcp  ncp(instance);
    TestHost host(&ncp, SPINEL_HEADER_IID_0);

    host.enableRawLink();
    host.setScanChannelMask(kTestMacScanChannelMask);

    VerifyOrQuit((spinel_scan_state_t)host.startEnergyScan() == SPINEL_SCAN_STATE_ENERGY);
    VerifyOrQuit(ncp.getPendingQueueSize() == 0);

    /* Test that the response status is OK when energy scan is in progress
       Test that requesting transmit while energy scan is active enqueues the command */
    VerifyOrQuit(host.startTransmit() == SPINEL_STATUS_OK);
    VerifyOrQuit(ncp.getPendingQueueSize() == 1);

    VerifyOrQuit(host.startTransmit() == SPINEL_STATUS_OK);
    VerifyOrQuit(ncp.getPendingQueueSize() == 2);

    /* Test that transmit command is dequeued when energy scan is complete */
    ncp.processEnergyScan();
    VerifyOrQuit(ncp.getPendingQueueSize() == 1);

    /* Test that transmit command is dequeued when transmit is complete */
    host.finishTransmit();
    VerifyOrQuit(ncp.getPendingQueueSize() == 0);

    host.finishTransmit();
    VerifyOrQuit(ncp.getPendingQueueSize() == 0);

    testFreeInstance(instance);
    printf("PASS\n");
}

void TestNcpBaseMultiHostTransmit()
{
    printf("\tMulti Host Transmit - ");
    ot::Instance *instance = testInitInstance();
    VerifyOrQuit(instance != nullptr);

    TestNcp  ncp(instance);
    TestHost host0(&ncp, SPINEL_HEADER_IID_1), host1(&ncp, SPINEL_HEADER_IID_2);

    host0.enableRawLink();
    host1.enableRawLink();

    /* Test that a host with a non-zero iid can request a transmit */
    VerifyOrQuit(host0.startTransmit() == SPINEL_STATUS_OK);
    VerifyOrQuit(ncp.getPendingQueueSize() == 0);

    host0.finishTransmit();
    VerifyOrQuit(ncp.getPendingQueueSize() == 0);

    VerifyOrQuit(host0.startTransmit() == SPINEL_STATUS_OK);
    VerifyOrQuit(ncp.getPendingQueueSize() == 0);

    host1.enableRawLink();

    /* Test that a host with a different iid can request a transmit when already in progress
       Test that command is enqueued when a separate host requests a transmit */
    VerifyOrQuit(host1.startTransmit() == SPINEL_STATUS_OK);
    VerifyOrQuit(ncp.getPendingQueueSize() == 1);

    host0.finishTransmit();
    VerifyOrQuit(ncp.getPendingQueueSize() == 0);

    host1.finishTransmit();
    VerifyOrQuit(ncp.getPendingQueueSize() == 0);

    testFreeInstance(instance);
    printf("PASS\n");
}

void TestNcpBaseMultiHostEnergyScan()
{
    printf("\tMulti Host Energy Scan - ");
    ot::Instance *instance = testInitInstance();
    VerifyOrQuit(instance != nullptr);

    TestNcp  ncp(instance);
    TestHost host0(&ncp, SPINEL_HEADER_IID_1), host1(&ncp, SPINEL_HEADER_IID_2);

    host0.enableRawLink();
    host0.setScanChannelMask(kTestMacScanChannelMask);

    host1.enableRawLink();
    host1.setScanChannelMask(kTestMacScanChannelMask);

    /* Test that a host with a non-zero iid can request an energy scan */
    VerifyOrQuit((spinel_scan_state_t)host0.startEnergyScan() == SPINEL_SCAN_STATE_ENERGY);
    VerifyOrQuit(ncp.getPendingQueueSize() == 0);

    ncp.processEnergyScan();
    VerifyOrQuit(ncp.getPendingQueueSize() == 0);

    VerifyOrQuit((spinel_scan_state_t)host0.startEnergyScan() == SPINEL_SCAN_STATE_ENERGY);
    VerifyOrQuit(ncp.getPendingQueueSize() == 0);

    /* Test that a host with a different iid will fail to request an energy scan when already in progress
       Test that command is not enqueued when a separate host requests an energy scan */
    VerifyOrQuit((spinel_status_t)host1.startEnergyScan() == SPINEL_STATUS_INVALID_STATE);
    VerifyOrQuit(ncp.getPendingQueueSize() == 0);

    ncp.processEnergyScan();
    VerifyOrQuit(ncp.getPendingQueueSize() == 0);

    /* Test that a host with a different iid can to request an energy scan when other host's scan finishes */
    VerifyOrQuit((spinel_scan_state_t)host1.startEnergyScan() == SPINEL_SCAN_STATE_ENERGY);
    VerifyOrQuit(ncp.getPendingQueueSize() == 0);

    ncp.processEnergyScan();
    VerifyOrQuit(ncp.getPendingQueueSize() == 0);

    testFreeInstance(instance);
    printf("PASS\n");
}

int main(void)
{
    sDefaultFrame.mPsdu = sDefaultMessage.mPsdu;
    sTxFrame            = &sDefaultFrame;

#if OPENTHREAD_CONFIG_MULTIPAN_RCP_ENABLE && (OPENTHREAD_RADIO || OPENTHREAD_CONFIG_LINK_RAW_ENABLE)
    printf("Executing Transmit Tests\n");
    TestNcpBaseTransmitWithLinkRawDisabled();
    TestNcpBaseTransmitWithLinkRawEnabled();
    TestNcpBaseTransmitWithNoBuffers();
    TestNcpBaseTransmitWhileLinkIsBusy();
    TestNcpBaseExceedPendingCommandQueueSize();
    printf("Transmit Tests - PASS\n");

#if OPENTHREAD_CONFIG_MAC_SOFTWARE_ENERGY_SCAN_ENABLE
    printf("Executing Energy Scan Tests\n");
    TestNcpBaseEnergyScanWithLinkRawDisabled();
    TestNcpBaseEnergyScanWithLinkRawEnabled();
    TestNcpBaseEnergyScanWithLinkRawEnabledAndMaskSet();
    TestNcpBaseEnergyScanWhileLinkIsBusy();
    TestNcpBaseEnergyScanWhileTransmitting();
    TestNcpBaseTransmitWhileScanning();
    TestNcpBaseMultiHostTransmit();
    TestNcpBaseMultiHostEnergyScan();
    printf("Energy Scan Tests - PASS\n");

#else
    printf("MAC_SOFTWARE_ENERGY_SCAN configuration not enabled - ");
    printf("Skipping Energy Scan Tests\n");

#endif
    printf("\nAll tests passed\n");

#else
    printf("MULTIPAN_RCP feature and RADIO/LINK_RAW option are not enabled\n");
#endif
    return 0;
}
