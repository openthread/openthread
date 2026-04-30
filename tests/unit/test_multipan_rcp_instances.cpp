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
#include <openthread/link_raw.h>

#include "common/array.hpp"
#include "common/code_utils.hpp"
#include "instance/instance.hpp"

#include "ncp/ncp_base.hpp"

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

static struct RadioMessage sDefaultMessages[OPENTHREAD_CONFIG_MULTIPLE_INSTANCE_NUM];
static otRadioFrame        sTxFrame[OPENTHREAD_CONFIG_MULTIPLE_INSTANCE_NUM];
static ot::Instance       *sInstances[OPENTHREAD_CONFIG_MULTIPLE_INSTANCE_NUM];
static ot::Instance       *sLastInstance;

otRadioFrame *otPlatRadioGetTransmitBuffer(otInstance *aInstance)
{
    otRadioFrame *frame = nullptr;

    for (size_t i = 0; i < OPENTHREAD_CONFIG_MULTIPLE_INSTANCE_NUM; i++)
    {
        if (sInstances[i] == aInstance || sInstances[i] == nullptr)
        {
            sTxFrame[i].mPsdu = sDefaultMessages->mPsdu;
            frame             = &sTxFrame[i];

            break;
        }
    }

    return frame;
}

otError otPlatRadioTransmit(otInstance *aInstance, otRadioFrame *)
{
    sLastInstance = static_cast<ot::Instance *>(aInstance);
    return OT_ERROR_NONE;
}

otError otPlatMultipanGetActiveInstance(otInstance **aInstance)
{
    otError error = OT_ERROR_NOT_IMPLEMENTED;
    OT_UNUSED_VARIABLE(aInstance);

#if OPENTHREAD_CONFIG_MULTIPAN_RCP_ENABLE
    *aInstance = sLastInstance;
    error      = OT_ERROR_NONE;
#endif

    return error;
}

otError otPlatMultipanSetActiveInstance(otInstance *aInstance, bool aCompletePending)
{
    otError error = OT_ERROR_NOT_IMPLEMENTED;
    OT_UNUSED_VARIABLE(aInstance);
    OT_UNUSED_VARIABLE(aCompletePending);

#if OPENTHREAD_CONFIG_MULTIPAN_RCP_ENABLE
    VerifyOrExit(sLastInstance != static_cast<ot::Instance *>(aInstance), error = OT_ERROR_ALREADY);
    sLastInstance = static_cast<ot::Instance *>(aInstance);
    error         = OT_ERROR_NONE;
exit:
#endif

    return error;
}

class TestNcp : public NcpBase
{
public:
    explicit TestNcp(ot::Instance *aInstance)
        : mLastHeader(0)
        , mLastStatus(0)
        , mLastProp(0)
        , NcpBase(aInstance)
    {
        memset(mMsgBuffer, 0, kTestBufferSize);
        mTxFrameBuffer.SetFrameAddedCallback(HandleFrameAddedToNcpBuffer, this);
        mTxFrameBuffer.SetFrameRemovedCallback(nullptr, this);
    };

    explicit TestNcp(ot::Instance **aInstances, uint8_t aCount)
        : mLastHeader(0)
        , mLastStatus(0)
        , mLastProp(0)
        , NcpBase(aInstances, aCount)
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
        uint8_t iid = SPINEL_HEADER_GET_IID(mLastHeader);

        LinkRawTransmitDone(iid, &sTxFrame[iid], nullptr, OT_ERROR_NONE);
    };

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
        mLastProp   = static_cast<uint32_t>(propKey);
    }

    uint32_t getSpinelStatus() const { return mLastStatus; }
    uint32_t getSpinelProp() const { return mLastProp; }

    uint8_t getLastIid() const
    {
        /* Return as SPINEL_HEADER_IID_N format without shift */
        return SPINEL_HEADER_IID_MASK & mLastHeader;
    }

    uint8_t getLastTid() { return SPINEL_HEADER_GET_TID(mLastHeader); }

    bool gotResponse(uint8_t aIid, uint8_t aTid) { return ((aIid == getLastIid()) && (aTid == getLastTid())); }

private:
    uint8_t  mLastHeader;
    uint32_t mLastStatus;
    uint32_t mLastProp;
    uint8_t  mMsgBuffer[kTestBufferSize];
};

class TestHost
{
public:
    TestHost(TestNcp *aNcp, uint8_t aIid)
        : mNcp(aNcp)
        , mIid(aIid)
        , mTid(0)
        , mLastTxTid(0)
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

        SuccessOrQuit(mEncoder.WriteDataWithLen(sTxFrame[mIid].mPsdu, sTxFrame[mIid].mLength));
        SuccessOrQuit(mEncoder.WriteUint8(sTxFrame[mIid].mChannel));
        SuccessOrQuit(mEncoder.WriteUint8(sTxFrame[mIid].mInfo.mTxInfo.mMaxCsmaBackoffs));
        SuccessOrQuit(mEncoder.WriteUint8(sTxFrame[mIid].mInfo.mTxInfo.mMaxFrameRetries));
        SuccessOrQuit(mEncoder.WriteBool(sTxFrame[mIid].mInfo.mTxInfo.mCsmaCaEnabled));
        SuccessOrQuit(mEncoder.WriteBool(sTxFrame[mIid].mInfo.mTxInfo.mIsHeaderUpdated));
        SuccessOrQuit(mEncoder.WriteBool(sTxFrame[mIid].mInfo.mTxInfo.mIsARetx));
        SuccessOrQuit(mEncoder.WriteBool(sTxFrame[mIid].mInfo.mTxInfo.mIsSecurityProcessed));
        SuccessOrQuit(mEncoder.WriteUint32(sTxFrame[mIid].mInfo.mTxInfo.mTxDelay));
        SuccessOrQuit(mEncoder.WriteUint32(sTxFrame[mIid].mInfo.mTxInfo.mTxDelayBaseTime));

        endFrame("Transmit Frame");
    }

    void createSwitchoverRequest(uint8_t aIid, bool aForce)
    {
        startFrame(SPINEL_CMD_PROP_VALUE_SET, SPINEL_PROP_MULTIPAN_ACTIVE_INTERFACE);
        SuccessOrQuit(mEncoder.WriteUint8(aIid | (aForce ? 0 : (1 << SPINEL_MULTIPAN_INTERFACE_SOFT_SWITCH_SHIFT))));
        endFrame("Interface Switch Request Frame");
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
        mLastTxTid = mTid;
        createTransmitFrame();
        sendToRcp();
        prepareResponse(mLastTxTid);
        return static_cast<spinel_status_t>(mNcp->getSpinelStatus());
    };

    spinel_status_t requestSwitchover(uint8_t aIid, bool aForce)
    {
        mLastTxTid = mTid;
        createSwitchoverRequest(aIid, aForce);
        sendToRcp();
        prepareResponse(mLastTxTid);
        return static_cast<spinel_status_t>(mNcp->getSpinelStatus());
    };

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

    uint8_t getLastTransmitTid(void) { return mLastTxTid; }

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
    uint8_t         mLastTxTid;
    uint8_t         mBuf[kTestBufferSize];
    Spinel::Buffer  mBuffer;
    Spinel::Encoder mEncoder;
    size_t          mOffset;
};

void InitInstances(void)
{
#if OPENTHREAD_CONFIG_MULTIPLE_INSTANCE_ENABLE && OPENTHREAD_CONFIG_MULTIPLE_STATIC_INSTANCE_ENABLE
    for (size_t i = 0; i < OPENTHREAD_CONFIG_MULTIPLE_INSTANCE_NUM; i++)
    {
        sInstances[i] = testInitAdditionalInstance(i);
        VerifyOrQuit(sInstances[i] != nullptr);
    }
#endif
}

void FreeInstances(void)
{
    for (size_t i = 0; i < OPENTHREAD_CONFIG_MULTIPLE_INSTANCE_NUM; i++)
    {
        if (sInstances[i] != nullptr)
        {
            testFreeInstance(sInstances[i]);
            sInstances[i] = nullptr;
        }
    }
}

void TestNcpBaseTransmitWithLinkRawDisabled(void)
{
    printf("\tTransmit With Link Raw Disabled");
    InitInstances();

    TestNcp  ncp(sInstances, OPENTHREAD_CONFIG_MULTIPLE_INSTANCE_NUM);
    TestHost host1(&ncp, SPINEL_HEADER_IID_0);
    TestHost host2(&ncp, SPINEL_HEADER_IID_1);
    TestHost host3(&ncp, SPINEL_HEADER_IID_2);

    host1.disableRawLink();
    host2.disableRawLink();
    host3.disableRawLink();

    /* Test that the response status is Invalid State when transmit is skipped due to disabled link */
    VerifyOrQuit(host1.startTransmit() == SPINEL_STATUS_INVALID_STATE);
    VerifyOrQuit(host2.startTransmit() == SPINEL_STATUS_INVALID_STATE);
    VerifyOrQuit(host3.startTransmit() == SPINEL_STATUS_INVALID_STATE);

    FreeInstances();
    printf(" - PASS\n");
}

void TestNcpBaseTransmitWithLinkRawEnabled(void)
{
    printf("\tTransmit With Link Raw Enabled");
    InitInstances();

    TestNcp  ncp(sInstances, OPENTHREAD_CONFIG_MULTIPLE_INSTANCE_NUM);
    TestHost host(&ncp, SPINEL_HEADER_IID_0);

    host.enableRawLink();

    /* Test that the response status is OK when transmit is started successfully */
    VerifyOrQuit(host.startTransmit() == SPINEL_STATUS_OK);

    host.finishTransmit();

    FreeInstances();
    printf(" - PASS\n");
}

void TestNcpBaseTransmitWithIncorrectLinkRawEnabled(void)
{
    printf("\tTransmit With Incorrect Link Raw Enabled");
    InitInstances();

    TestNcp  ncp(sInstances, OPENTHREAD_CONFIG_MULTIPLE_INSTANCE_NUM);
    TestHost host1(&ncp, SPINEL_HEADER_IID_0);
    TestHost host2(&ncp, SPINEL_HEADER_IID_1);

    host1.disableRawLink();
    host2.enableRawLink();

    /* Test that Invalid State is reported when different endpoint was enabled */
    VerifyOrQuit(host1.startTransmit() == SPINEL_STATUS_INVALID_STATE);

    /* Test that status is OK when transmitting on the proper interface */
    VerifyOrQuit(host2.startTransmit() == SPINEL_STATUS_OK);

    host1.finishTransmit();

    FreeInstances();
    printf(" - PASS\n");
}

void TestNcpBaseTransmitOnBoth(void)
{
    printf("\tTransmit on both interfaces");
    InitInstances();

    TestNcp  ncp(sInstances, OPENTHREAD_CONFIG_MULTIPLE_INSTANCE_NUM);
    TestHost host1(&ncp, SPINEL_HEADER_IID_0);
    TestHost host2(&ncp, SPINEL_HEADER_IID_1);

    host1.enableRawLink();
    host2.enableRawLink();

    VerifyOrQuit(host1.startTransmit() == SPINEL_STATUS_OK);
    VerifyOrQuit(host2.startTransmit() == SPINEL_STATUS_OK);

    host1.finishTransmit();
    host2.finishTransmit();

    FreeInstances();
    printf(" - PASS\n");
}

void TestNcpBaseDifferentInstanceCall(void)
{
    printf("\tTransmit on both interfaces - verify instances used");

    InitInstances();

    TestNcp  ncp(sInstances, OPENTHREAD_CONFIG_MULTIPLE_INSTANCE_NUM);
    TestHost host1(&ncp, SPINEL_HEADER_IID_0);
    TestHost host2(&ncp, SPINEL_HEADER_IID_1);

    sLastInstance = nullptr;

    host1.enableRawLink();
    host2.enableRawLink();

    VerifyOrQuit(host1.startTransmit() == SPINEL_STATUS_OK);
    VerifyOrQuit(sLastInstance != nullptr);
    VerifyOrQuit(sLastInstance == sInstances[0]);

    VerifyOrQuit(host2.startTransmit() == SPINEL_STATUS_OK);
    VerifyOrQuit(sLastInstance != nullptr);
    VerifyOrQuit(sLastInstance == sInstances[1]);

    host1.finishTransmit();
    host2.finishTransmit();

    /* Test reverse order of calls to make sure it is not just a fixed order */
    sLastInstance = nullptr;
    VerifyOrQuit(host2.startTransmit() == SPINEL_STATUS_OK);
    VerifyOrQuit(sLastInstance != nullptr);
    VerifyOrQuit(sLastInstance == sInstances[1]);

    VerifyOrQuit(host1.startTransmit() == SPINEL_STATUS_OK);
    VerifyOrQuit(sLastInstance != nullptr);
    VerifyOrQuit(sLastInstance == sInstances[0]);

    host1.finishTransmit();
    host2.finishTransmit();

    printf(" - PASS\n");
}

void TestNcpBaseTransmitDoneInterface(void)
{
    printf("\tTransmit on both interfaces - verify transmit done IID");

    InitInstances();

    TestNcp  ncp(sInstances, OPENTHREAD_CONFIG_MULTIPLE_INSTANCE_NUM);
    TestHost host1(&ncp, SPINEL_HEADER_IID_0);
    TestHost host2(&ncp, SPINEL_HEADER_IID_1);

    sLastInstance = nullptr;

    host1.enableRawLink();
    host2.enableRawLink();

    VerifyOrQuit(host1.startTransmit() == SPINEL_STATUS_OK);
    VerifyOrQuit(host2.startTransmit() == SPINEL_STATUS_OK);

    otPlatRadioTxDone(sInstances[0], &sTxFrame[0], nullptr, OT_ERROR_NONE);
    VerifyOrQuit(ncp.gotResponse(SPINEL_HEADER_IID_0, host1.getLastTransmitTid()));

    otPlatRadioTxDone(sInstances[1], &sTxFrame[1], nullptr, OT_ERROR_NONE);
    VerifyOrQuit(ncp.gotResponse(SPINEL_HEADER_IID_1, host2.getLastTransmitTid()));

    /* Test reverse order of tx processing */
    VerifyOrQuit(host1.startTransmit() == SPINEL_STATUS_OK);
    VerifyOrQuit(host2.startTransmit() == SPINEL_STATUS_OK);

    otPlatRadioTxDone(sInstances[1], &sTxFrame[1], nullptr, OT_ERROR_NONE);
    VerifyOrQuit(ncp.gotResponse(SPINEL_HEADER_IID_1, host2.getLastTransmitTid()));

    otPlatRadioTxDone(sInstances[0], &sTxFrame[0], nullptr, OT_ERROR_NONE);
    VerifyOrQuit(ncp.gotResponse(SPINEL_HEADER_IID_0, host1.getLastTransmitTid()));

    printf(" - PASS\n");
}

void TestNcpBaseReceive(void)
{
    printf("\tReceive on a single interface");

    InitInstances();

    TestNcp  ncp(sInstances, OPENTHREAD_CONFIG_MULTIPLE_INSTANCE_NUM);
    TestHost host1(&ncp, SPINEL_HEADER_IID_0);

    host1.enableRawLink();

    otPlatRadioReceiveDone(sInstances[0], &sTxFrame[0], OT_ERROR_NONE);

    VerifyOrQuit(ncp.getSpinelProp() == SPINEL_PROP_STREAM_RAW);
    VerifyOrQuit(ncp.getLastTid() == 0);
    VerifyOrQuit(ncp.getLastIid() == SPINEL_HEADER_IID_0);

    printf(" - PASS\n");
}

void TestNcpBaseReceiveOnTwoInterfaces(void)
{
    printf("\tReceive on both interfaces");

    InitInstances();

    TestNcp  ncp(sInstances, OPENTHREAD_CONFIG_MULTIPLE_INSTANCE_NUM);
    TestHost host1(&ncp, SPINEL_HEADER_IID_0);
    TestHost host2(&ncp, SPINEL_HEADER_IID_1);

    sLastInstance = nullptr;

    host1.enableRawLink();
    host2.enableRawLink();

    otPlatRadioReceiveDone(sInstances[1], &sTxFrame[1], OT_ERROR_NONE);

    VerifyOrQuit(ncp.getSpinelProp() == SPINEL_PROP_STREAM_RAW);
    VerifyOrQuit(ncp.getLastTid() == 0);
    VerifyOrQuit(ncp.getLastIid() == SPINEL_HEADER_IID_1);

    otPlatRadioReceiveDone(sInstances[0], &sTxFrame[0], OT_ERROR_NONE);

    VerifyOrQuit(ncp.getSpinelProp() == SPINEL_PROP_STREAM_RAW);
    VerifyOrQuit(ncp.getLastTid() == 0);
    VerifyOrQuit(ncp.getLastIid() == SPINEL_HEADER_IID_0);

    /* reverse order */
    otPlatRadioReceiveDone(sInstances[0], &sTxFrame[0], OT_ERROR_NONE);

    VerifyOrQuit(ncp.getSpinelProp() == SPINEL_PROP_STREAM_RAW);
    VerifyOrQuit(ncp.getLastTid() == 0);
    VerifyOrQuit(ncp.getLastIid() == SPINEL_HEADER_IID_0);

    otPlatRadioReceiveDone(sInstances[1], &sTxFrame[1], OT_ERROR_NONE);

    VerifyOrQuit(ncp.getSpinelProp() == SPINEL_PROP_STREAM_RAW);
    VerifyOrQuit(ncp.getLastTid() == 0);
    VerifyOrQuit(ncp.getLastIid() == SPINEL_HEADER_IID_1);

    printf(" - PASS\n");
}

void TestNcpBaseSwitchoverRequest(void)
{
    printf("\tSwitchover requests from different interfaces");

    InitInstances();

    TestNcp  ncp(sInstances, OPENTHREAD_CONFIG_MULTIPLE_INSTANCE_NUM);
    TestHost host1(&ncp, SPINEL_HEADER_IID_0);
    TestHost host2(&ncp, SPINEL_HEADER_IID_1);
    TestHost host3(&ncp, SPINEL_HEADER_IID_2);

    sLastInstance = nullptr;

    host1.enableRawLink();
    host2.enableRawLink();
    host3.enableRawLink();

    VerifyOrQuit(host1.requestSwitchover(0, true) == 0);
    VerifyOrQuit(sLastInstance == sInstances[0]);
    VerifyOrQuit(host1.requestSwitchover(1, true) == 1);
    VerifyOrQuit(sLastInstance == sInstances[1]);
    VerifyOrQuit(host1.requestSwitchover(2, true) == 2);
    VerifyOrQuit(sLastInstance == sInstances[2]);
    VerifyOrQuit(host2.requestSwitchover(0, true) == 0);
    VerifyOrQuit(sLastInstance == sInstances[0]);
    VerifyOrQuit(host2.requestSwitchover(1, true) == 1);
    VerifyOrQuit(sLastInstance == sInstances[1]);
    VerifyOrQuit(host2.requestSwitchover(2, true) == 2);
    VerifyOrQuit(sLastInstance == sInstances[2]);
    VerifyOrQuit(host3.requestSwitchover(0, true) == 0);
    VerifyOrQuit(sLastInstance == sInstances[0]);
    VerifyOrQuit(host3.requestSwitchover(1, true) == 1);
    VerifyOrQuit(sLastInstance == sInstances[1]);
    VerifyOrQuit(host3.requestSwitchover(2, true) == 2);
    VerifyOrQuit(sLastInstance == sInstances[2]);

    printf(" - PASS\n");
}

void TestNcpBaseSwitchoverRequestFail(void)
{
    printf("\tSwitchover requests Fail - same interface");

    InitInstances();

    TestNcp  ncp(sInstances, OPENTHREAD_CONFIG_MULTIPLE_INSTANCE_NUM);
    TestHost host1(&ncp, SPINEL_HEADER_IID_0);
    TestHost host2(&ncp, SPINEL_HEADER_IID_1);

    sLastInstance = nullptr;

    host1.enableRawLink();
    host2.enableRawLink();

    VerifyOrQuit(host1.requestSwitchover(0, true) == 0);
    VerifyOrQuit(sLastInstance == sInstances[0]);

    VerifyOrQuit(host1.requestSwitchover(0, true) == SPINEL_STATUS_ALREADY);
    VerifyOrQuit(sLastInstance == sInstances[0]);

    VerifyOrQuit(host2.requestSwitchover(0, true) == SPINEL_STATUS_ALREADY);
    VerifyOrQuit(sLastInstance == sInstances[0]);

    printf(" - PASS\n");
}

void TestNcpBaseSwitchoverResponse(void)
{
    printf("\tSwitchover responses");

    InitInstances();

    TestNcp  ncp(sInstances, OPENTHREAD_CONFIG_MULTIPLE_INSTANCE_NUM);
    TestHost host1(&ncp, SPINEL_HEADER_IID_0);
    TestHost host2(&ncp, SPINEL_HEADER_IID_1);

    sLastInstance = nullptr;

    host1.enableRawLink();
    host2.enableRawLink();

    VerifyOrQuit(host1.requestSwitchover(0, true) == 0);
    VerifyOrQuit(sLastInstance == sInstances[0]);

    otPlatMultipanSwitchoverDone(sLastInstance, true);

    VerifyOrQuit(ncp.getSpinelProp() == SPINEL_PROP_LAST_STATUS);
    VerifyOrQuit(ncp.getLastTid() == 0);
    VerifyOrQuit(ncp.getLastIid() == OPENTHREAD_SPINEL_CONFIG_BROADCAST_IID);
    VerifyOrQuit(ncp.getSpinelStatus() == SPINEL_STATUS_SWITCHOVER_DONE);

    VerifyOrQuit(host1.requestSwitchover(1, true) == 1);
    VerifyOrQuit(sLastInstance == sInstances[1]);

    otPlatMultipanSwitchoverDone(sLastInstance, false);

    VerifyOrQuit(ncp.getSpinelProp() == SPINEL_PROP_LAST_STATUS);
    VerifyOrQuit(ncp.getLastTid() == 0);
    VerifyOrQuit(ncp.getLastIid() == OPENTHREAD_SPINEL_CONFIG_BROADCAST_IID);
    VerifyOrQuit(ncp.getSpinelStatus() == SPINEL_STATUS_SWITCHOVER_FAILED);

    printf(" - PASS\n");
}

///
int main(void)
{
#if OPENTHREAD_CONFIG_MULTIPAN_RCP_ENABLE && (OPENTHREAD_RADIO || OPENTHREAD_CONFIG_LINK_RAW_ENABLE)
    printf("Executing Transmit Tests\n");
    TestNcpBaseTransmitWithLinkRawDisabled();
    TestNcpBaseTransmitWithLinkRawEnabled();
    TestNcpBaseTransmitWithIncorrectLinkRawEnabled();
    TestNcpBaseTransmitOnBoth();
    TestNcpBaseDifferentInstanceCall();
    TestNcpBaseTransmitDoneInterface();
    printf("Transmit Tests - PASS\n");

    printf("Executing Receive Tests\n");
    TestNcpBaseReceive();
    TestNcpBaseReceiveOnTwoInterfaces();
    printf("Receive Tests - PASS\n");

    printf("Executing Interface Switching Tests\n");
    TestNcpBaseSwitchoverRequest();
    TestNcpBaseSwitchoverRequestFail();
    TestNcpBaseSwitchoverResponse();
    printf("Executing Interface Switching Tests - PASS\n");

    printf("\nAll tests passed\n");

#else
    printf("MULTIPAN_RCP feature and RADIO/LINK_RAW option are not enabled\n");
#endif
    return 0;
}
