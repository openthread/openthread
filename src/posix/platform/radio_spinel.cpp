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
 *   This file implements the spinel based radio transceiver.
 */

#include "openthread-core-config.h"
#include "platform-posix.h"

#include "radio_spinel.hpp"

#include <assert.h>
#include <errno.h>
#include <stdarg.h>
#include <stdlib.h>
#include <sys/resource.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <syslog.h>
#include <termios.h>
#include <unistd.h>

#include <common/code_utils.hpp>
#include <common/encoding.hpp>
#include <common/logging.hpp>
#include <openthread/platform/alarm-milli.h>
#include <openthread/platform/diag.h>
#include <openthread/platform/radio.h>

static ot::PosixApp::RadioSpinel sRadioSpinel;

namespace ot {
namespace PosixApp {

static otError SpinelStatusToOtError(spinel_status_t aError)
{
    otError ret;

    switch (aError)
    {
    case SPINEL_STATUS_OK:
        ret = OT_ERROR_NONE;
        break;

    case SPINEL_STATUS_FAILURE:
        ret = OT_ERROR_FAILED;
        break;

    case SPINEL_STATUS_DROPPED:
        ret = OT_ERROR_DROP;
        break;

    case SPINEL_STATUS_NOMEM:
        ret = OT_ERROR_NO_BUFS;
        break;

    case SPINEL_STATUS_BUSY:
        ret = OT_ERROR_BUSY;
        break;

    case SPINEL_STATUS_PARSE_ERROR:
        ret = OT_ERROR_PARSE;
        break;

    case SPINEL_STATUS_INVALID_ARGUMENT:
        ret = OT_ERROR_INVALID_ARGS;
        break;

    case SPINEL_STATUS_UNIMPLEMENTED:
        ret = OT_ERROR_NOT_IMPLEMENTED;
        break;

    case SPINEL_STATUS_INVALID_STATE:
        ret = OT_ERROR_INVALID_STATE;
        break;

    case SPINEL_STATUS_NO_ACK:
        ret = OT_ERROR_NO_ACK;
        break;

    case SPINEL_STATUS_CCA_FAILURE:
        ret = OT_ERROR_CHANNEL_ACCESS_FAILURE;
        break;

    case SPINEL_STATUS_ALREADY:
        ret = OT_ERROR_ALREADY;
        break;

    case SPINEL_STATUS_PROP_NOT_FOUND:
    case SPINEL_STATUS_ITEM_NOT_FOUND:
        ret = OT_ERROR_NOT_FOUND;
        break;

    default:
        if (aError >= SPINEL_STATUS_STACK_NATIVE__BEGIN && aError <= SPINEL_STATUS_STACK_NATIVE__END)
        {
            ret = static_cast<otError>(aError - SPINEL_STATUS_STACK_NATIVE__BEGIN);
        }
        else
        {
            ret = OT_ERROR_FAILED;
        }
        break;
    }

    return ret;
}

static void LogIfFail(const char *aText, otError aError)
{
    OT_UNUSED_VARIABLE(aText);
    OT_UNUSED_VARIABLE(aError);

    if (aError != OT_ERROR_NONE)
    {
        otLogWarnPlat("%s: %s", aText, otThreadErrorToString(aError));
    }
}

void HdlcInterface::Callbacks::HandleReceivedFrame(HdlcInterface &aInterface)
{
    static_cast<RadioSpinel *>(this)->HandleSpinelFrame(aInterface.GetRxFrameBuffer());
}

RadioSpinel::RadioSpinel(void)
    : mInstance(NULL)
    , mHdlcInterface(*this)
    , mCmdTidsInUse(0)
    , mCmdNextTid(1)
    , mTxRadioTid(0)
    , mWaitingTid(0)
    , mWaitingKey(SPINEL_PROP_LAST_STATUS)
    , mPropertyFormat(NULL)
    , mExpectedCommand(0)
    , mError(OT_ERROR_NONE)
    , mTransmitFrame(NULL)
    , mShortAddress(0)
    , mPanId(0xffff)
    , mRadioCaps(0)
    , mChannel(0)
    , mRxSensitivity(0)
    , mState(kStateDisabled)
    , mIsPromiscuous(false)
    , mIsReady(false)
    , mSupportsLogStream(false)
#if OPENTHREAD_ENABLE_DIAG
    , mDiagMode(false)
    , mDiagOutput(NULL)
    , mDiagOutputMaxLen(0)
#endif
{
    mVersion[0] = '\0';
}

void RadioSpinel::Init(const char *aRadioFile, const char *aRadioConfig)
{
    otError error = OT_ERROR_NONE;

    SuccessOrExit(error = mHdlcInterface.Init(aRadioFile, aRadioConfig));

    SuccessOrExit(error = SendReset());
    SuccessOrExit(error = WaitResponse());
    VerifyOrExit(mIsReady, error = OT_ERROR_FAILED);

    SuccessOrExit(error = CheckSpinelVersion());
    SuccessOrExit(error = CheckCapabilities());
    SuccessOrExit(error = CheckRadioCapabilities());

    SuccessOrExit(error = Get(SPINEL_PROP_NCP_VERSION, SPINEL_DATATYPE_UTF8_S, mVersion, sizeof(mVersion)));
    SuccessOrExit(error = Get(SPINEL_PROP_HWADDR, SPINEL_DATATYPE_UINT64_S, &gNodeId));

    gNodeId = ot::Encoding::BigEndian::HostSwap64(gNodeId);

    mRxRadioFrame.mPsdu  = mRxPsdu;
    mTxRadioFrame.mPsdu  = mTxPsdu;
    mAckRadioFrame.mPsdu = mAckPsdu;

exit:
    SuccessOrDie(error);
}

otError RadioSpinel::CheckSpinelVersion(void)
{
    otError      error = OT_ERROR_NONE;
    unsigned int versionMajor;
    unsigned int versionMinor;

    SuccessOrExit(error =
                      Get(SPINEL_PROP_PROTOCOL_VERSION, (SPINEL_DATATYPE_UINT_PACKED_S SPINEL_DATATYPE_UINT_PACKED_S),
                          &versionMajor, &versionMinor));

    if ((versionMajor != SPINEL_PROTOCOL_VERSION_THREAD_MAJOR) ||
        (versionMinor != SPINEL_PROTOCOL_VERSION_THREAD_MINOR))
    {
        otLogCritPlat("Spinel version mismatch - PosixApp:%d.%d, RCP:%d.%d", SPINEL_PROTOCOL_VERSION_THREAD_MAJOR,
                      SPINEL_PROTOCOL_VERSION_THREAD_MINOR, versionMajor, versionMinor);
        exit(OT_EXIT_RADIO_SPINEL_INCOMPATIBLE);
    }

exit:
    return error;
}

otError RadioSpinel::CheckCapabilities(void)
{
    otError        error = OT_ERROR_NONE;
    uint8_t        capsBuffer[kCapsBufferSize];
    const uint8_t *capsData         = capsBuffer;
    spinel_size_t  capsLength       = sizeof(capsBuffer);
    bool           supportsRawRadio = false;

    SuccessOrExit(error = Get(SPINEL_PROP_CAPS, SPINEL_DATATYPE_DATA_S, capsBuffer, &capsLength));

    while (capsLength > 0)
    {
        unsigned int   capability;
        spinel_ssize_t unpacked;

        unpacked = spinel_datatype_unpack(capsData, capsLength, SPINEL_DATATYPE_UINT_PACKED_S, &capability);
        VerifyOrExit(unpacked > 0, error = OT_ERROR_FAILED);

        if (capability == SPINEL_CAP_OPENTHREAD_LOG_METADATA)
        {
            mSupportsLogStream = true;
        }

        if (capability == SPINEL_CAP_MAC_RAW)
        {
            supportsRawRadio = true;
        }

        capsData += unpacked;
        capsLength -= static_cast<spinel_size_t>(unpacked);
    }

    if (!supportsRawRadio)
    {
        otLogCritPlat("RCP capability list does not include support for radio/raw mode");
        exit(OT_EXIT_RADIO_SPINEL_INCOMPATIBLE);
    }

exit:
    return error;
}

otError RadioSpinel::CheckRadioCapabilities(void)
{
    const otRadioCaps kRequiredRadioCaps =
        OT_RADIO_CAPS_ACK_TIMEOUT | OT_RADIO_CAPS_TRANSMIT_RETRIES | OT_RADIO_CAPS_CSMA_BACKOFF;

    otError      error = OT_ERROR_NONE;
    unsigned int caps;

    SuccessOrExit(error = Get(SPINEL_PROP_RADIO_CAPS, SPINEL_DATATYPE_UINT_PACKED_S, &caps));
    mRadioCaps = static_cast<otRadioCaps>(caps);

    if ((mRadioCaps & kRequiredRadioCaps) != kRequiredRadioCaps)
    {
        otLogCritPlat("RCP does not support required capabilities: ack-timeout:%s, tx-retries:%s, CSMA-backoff:%s",
                      (mRadioCaps & OT_RADIO_CAPS_ACK_TIMEOUT) ? "yes" : "no",
                      (mRadioCaps & OT_RADIO_CAPS_TRANSMIT_RETRIES) ? "yes" : "no",
                      (mRadioCaps & OT_RADIO_CAPS_CSMA_BACKOFF) ? "yes" : "no");

        exit(OT_EXIT_RADIO_SPINEL_INCOMPATIBLE);
    }

exit:
    return error;
}

void RadioSpinel::Deinit(void)
{
    mHdlcInterface.Deinit();
}

void RadioSpinel::HandleSpinelFrame(HdlcInterface::RxFrameBuffer &aFrameBuffer)
{
    otError        error = OT_ERROR_NONE;
    uint8_t        header;
    spinel_ssize_t unpacked;

    unpacked = spinel_datatype_unpack(aFrameBuffer.GetFrame(), aFrameBuffer.GetLength(), "C", &header);

    VerifyOrExit(unpacked > 0 && (header & SPINEL_HEADER_FLAG) == SPINEL_HEADER_FLAG &&
                     SPINEL_HEADER_GET_IID(header) == 0,
                 error = OT_ERROR_PARSE);

    if (SPINEL_HEADER_GET_TID(header) == 0)
    {
        HandleNotification(aFrameBuffer);
    }
    else
    {
        HandleResponse(aFrameBuffer.GetFrame(), aFrameBuffer.GetLength());
        aFrameBuffer.DiscardFrame();
    }

exit:
    if (error != OT_ERROR_NONE)
    {
        aFrameBuffer.DiscardFrame();
        otLogWarnPlat("Error handling hdlc frame: %s", otThreadErrorToString(error));
    }
}

void RadioSpinel::HandleNotification(HdlcInterface::RxFrameBuffer &aFrameBuffer)
{
    spinel_prop_key_t key;
    spinel_size_t     len = 0;
    spinel_ssize_t    unpacked;
    uint8_t *         data = NULL;
    uint32_t          cmd;
    uint8_t           header;
    otError           error           = OT_ERROR_NONE;
    bool              shouldSaveFrame = false;

    unpacked = spinel_datatype_unpack(aFrameBuffer.GetFrame(), aFrameBuffer.GetLength(), "CiiD", &header, &cmd, &key,
                                      &data, &len);
    VerifyOrExit(unpacked > 0, error = OT_ERROR_PARSE);
    VerifyOrExit(SPINEL_HEADER_GET_TID(header) == 0, error = OT_ERROR_PARSE);

    switch (cmd)
    {
    case SPINEL_CMD_PROP_VALUE_IS:
        // Some spinel properties cannot be handled during `WaitResponse()`, we must cache these events.
        // `mWaitingTid` is released immediately after received the response. And `mWaitingKey` is be set
        // to `SPINEL_PROP_LAST_STATUS` at the end of `WaitResponse()`.

        if (!IsSafeToHandleNow(key))
        {
            ExitNow(shouldSaveFrame = true);
        }

        HandleValueIs(key, data, static_cast<uint16_t>(len));
        break;

    case SPINEL_CMD_PROP_VALUE_INSERTED:
    case SPINEL_CMD_PROP_VALUE_REMOVED:
        otLogInfoPlat("Ignored command %d", cmd);
        break;

    default:
        ExitNow(error = OT_ERROR_PARSE);
    }

exit:
    if (shouldSaveFrame)
    {
        aFrameBuffer.SaveFrame();
    }
    else
    {
        aFrameBuffer.DiscardFrame();
    }

    LogIfFail("Error processing notification", error);
}

void RadioSpinel::HandleNotification(const uint8_t *aFrame, uint16_t aLength)
{
    spinel_prop_key_t key;
    spinel_size_t     len = 0;
    spinel_ssize_t    unpacked;
    uint8_t *         data = NULL;
    uint32_t          cmd;
    uint8_t           header;
    otError           error = OT_ERROR_NONE;

    unpacked = spinel_datatype_unpack(aFrame, aLength, "CiiD", &header, &cmd, &key, &data, &len);
    VerifyOrExit(unpacked > 0, error = OT_ERROR_PARSE);
    VerifyOrExit(SPINEL_HEADER_GET_TID(header) == 0, error = OT_ERROR_PARSE);
    VerifyOrExit(cmd == SPINEL_CMD_PROP_VALUE_IS);
    HandleValueIs(key, data, static_cast<uint16_t>(len));

exit:
    LogIfFail("Error processing saved notification", error);
}

void RadioSpinel::HandleResponse(const uint8_t *aBuffer, uint16_t aLength)
{
    spinel_prop_key_t key;
    uint8_t *         data   = NULL;
    spinel_size_t     len    = 0;
    uint8_t           header = 0;
    uint32_t          cmd    = 0;
    spinel_ssize_t    rval   = 0;
    otError           error  = OT_ERROR_NONE;

    rval = spinel_datatype_unpack(aBuffer, aLength, "CiiD", &header, &cmd, &key, &data, &len);
    VerifyOrExit(rval > 0 && cmd >= SPINEL_CMD_PROP_VALUE_IS && cmd <= SPINEL_CMD_PROP_VALUE_REMOVED,
                 error = OT_ERROR_PARSE);

    if (mWaitingTid == SPINEL_HEADER_GET_TID(header))
    {
        HandleWaitingResponse(cmd, key, data, static_cast<uint16_t>(len));
        FreeTid(mWaitingTid);
        mWaitingTid = 0;
    }
    else if (mTxRadioTid == SPINEL_HEADER_GET_TID(header))
    {
        if (mState == kStateTransmitting)
        {
            HandleTransmitDone(cmd, key, data, static_cast<uint16_t>(len));
        }

        FreeTid(mTxRadioTid);
        mTxRadioTid = 0;
    }
    else
    {
        otLogWarnPlat("Unexpected Spinel transaction message: %u", SPINEL_HEADER_GET_TID(header));
        error = OT_ERROR_DROP;
    }

exit:
    LogIfFail("Error processing response", error);
}

void RadioSpinel::HandleWaitingResponse(uint32_t          aCommand,
                                        spinel_prop_key_t aKey,
                                        const uint8_t *   aBuffer,
                                        uint16_t          aLength)
{
    if (aKey == SPINEL_PROP_LAST_STATUS)
    {
        spinel_status_t status;
        spinel_ssize_t  unpacked = spinel_datatype_unpack(aBuffer, aLength, "i", &status);

        VerifyOrExit(unpacked > 0, mError = OT_ERROR_PARSE);
        mError = SpinelStatusToOtError(status);
    }
#if OPENTHREAD_ENABLE_DIAG
    else if (aKey == SPINEL_PROP_NEST_STREAM_MFG)
    {
        spinel_ssize_t unpacked;

        VerifyOrExit(mDiagOutput != NULL);
        unpacked =
            spinel_datatype_unpack_in_place(aBuffer, aLength, SPINEL_DATATYPE_UTF8_S, mDiagOutput, &mDiagOutputMaxLen);
        VerifyOrExit(unpacked > 0, mError = OT_ERROR_PARSE);
    }
#endif
    else if (aKey == mWaitingKey)
    {
        if (mPropertyFormat)
        {
            spinel_ssize_t unpacked =
                spinel_datatype_vunpack_in_place(aBuffer, aLength, mPropertyFormat, mPropertyArgs);

            VerifyOrExit(unpacked > 0, mError = OT_ERROR_PARSE);
            mError = OT_ERROR_NONE;
        }
        else
        {
            if (aCommand == mExpectedCommand)
            {
                mError = OT_ERROR_NONE;
            }
            else
            {
                mError = OT_ERROR_DROP;
            }
        }
    }
    else
    {
        mError = OT_ERROR_DROP;
    }

exit:
    LogIfFail("Error processing result", mError);
}

void RadioSpinel::HandleValueIs(spinel_prop_key_t aKey, const uint8_t *aBuffer, uint16_t aLength)
{
    otError error = OT_ERROR_NONE;

    if (aKey == SPINEL_PROP_STREAM_RAW)
    {
        SuccessOrExit(error = ParseRadioFrame(mRxRadioFrame, aBuffer, aLength));
        RadioReceive();
    }
    else if (aKey == SPINEL_PROP_LAST_STATUS)
    {
        spinel_status_t status = SPINEL_STATUS_OK;
        spinel_ssize_t  unpacked;

        unpacked = spinel_datatype_unpack(aBuffer, aLength, "i", &status);
        VerifyOrExit(unpacked > 0, error = OT_ERROR_PARSE);

        if (status >= SPINEL_STATUS_RESET__BEGIN && status <= SPINEL_STATUS_RESET__END)
        {
            otLogCritPlat("RCP reset: %s", spinel_status_to_cstr(status));
            mIsReady = true;

            // If RCP crashes/resets while radio was enabled, posix app exits.
            VerifyOrDie(!IsEnabled(), OT_EXIT_RADIO_SPINEL_RESET);
        }
        else
        {
            otLogInfoPlat("RCP last status: %s", spinel_status_to_cstr(status));
        }
    }
    else if (aKey == SPINEL_PROP_MAC_ENERGY_SCAN_RESULT)
    {
        uint8_t        scanChannel;
        int8_t         maxRssi;
        spinel_ssize_t unpacked;

        unpacked = spinel_datatype_unpack(aBuffer, aLength, "Cc", &scanChannel, &maxRssi);

        VerifyOrExit(unpacked > 0, error = OT_ERROR_PARSE);
        otPlatRadioEnergyScanDone(mInstance, maxRssi);
    }
    else if (aKey == SPINEL_PROP_STREAM_DEBUG)
    {
        char           logStream[OPENTHREAD_CONFIG_NCP_SPINEL_LOG_MAX_SIZE + 1];
        unsigned int   len = sizeof(logStream);
        spinel_ssize_t unpacked;

        unpacked = spinel_datatype_unpack_in_place(aBuffer, aLength, SPINEL_DATATYPE_DATA_S, logStream, &len);
        assert(len < sizeof(logStream));
        VerifyOrExit(unpacked > 0, error = OT_ERROR_PARSE);
        logStream[len] = '\0';
        otLogDebgPlat("RCP => %s", logStream);
    }
    else if ((aKey == SPINEL_PROP_STREAM_LOG) && mSupportsLogStream)
    {
        const char *   logString;
        spinel_ssize_t unpacked;
        uint8_t        logLevel;

        unpacked = spinel_datatype_unpack(aBuffer, aLength, SPINEL_DATATYPE_UTF8_S, &logString);
        VerifyOrExit(unpacked >= 0, error = OT_ERROR_PARSE);
        aBuffer += unpacked;
        aLength -= unpacked;

        unpacked = spinel_datatype_unpack(aBuffer, aLength, SPINEL_DATATYPE_UINT8_S, &logLevel);
        VerifyOrExit(unpacked > 0, error = OT_ERROR_PARSE);

        switch (logLevel)
        {
        case SPINEL_NCP_LOG_LEVEL_EMERG:
        case SPINEL_NCP_LOG_LEVEL_ALERT:
        case SPINEL_NCP_LOG_LEVEL_CRIT:
            otLogCritPlat("RCP => %s", logString);
            break;

        case SPINEL_NCP_LOG_LEVEL_ERR:
        case SPINEL_NCP_LOG_LEVEL_WARN:
            otLogWarnPlat("RCP => %s", logString);
            break;

        case SPINEL_NCP_LOG_LEVEL_NOTICE:
            otLogNotePlat("RCP => %s", logString);
            break;

        case SPINEL_NCP_LOG_LEVEL_INFO:
            otLogInfoPlat("RCP => %s", logString);
            break;

        case SPINEL_NCP_LOG_LEVEL_DEBUG:
        default:
            otLogDebgPlat("RCP => %s", logString);
            break;
        }
    }

exit:
    LogIfFail("Failed to handle ValueIs", error);
}

otError RadioSpinel::ParseRadioFrame(otRadioFrame &aFrame, const uint8_t *aBuffer, uint16_t aLength)
{
    otError        error        = OT_ERROR_NONE;
    uint16_t       packetLength = 0;
    spinel_ssize_t unpacked;
    uint16_t       flags      = 0;
    int8_t         noiseFloor = -128;
    spinel_size_t  size       = OT_RADIO_FRAME_MAX_SIZE;

    unpacked = spinel_datatype_unpack(aBuffer, aLength, SPINEL_DATATYPE_UINT16_S, &packetLength);
    VerifyOrExit(unpacked > 0 && packetLength <= OT_RADIO_FRAME_MAX_SIZE, error = OT_ERROR_PARSE);

    aFrame.mLength = static_cast<uint8_t>(packetLength);

    unpacked = spinel_datatype_unpack_in_place(aBuffer, aLength,
                                               SPINEL_DATATYPE_DATA_WLEN_S SPINEL_DATATYPE_INT8_S SPINEL_DATATYPE_INT8_S
                                                   SPINEL_DATATYPE_UINT16_S SPINEL_DATATYPE_STRUCT_S( // PHY-data
                                                       SPINEL_DATATYPE_UINT8_S     // 802.15.4 channel
                                                           SPINEL_DATATYPE_UINT8_S // 802.15.4 LQI
                                                       ),
                                               aFrame.mPsdu, &size, &aFrame.mInfo.mRxInfo.mRssi, &noiseFloor, &flags,
                                               &aFrame.mChannel, &aFrame.mInfo.mRxInfo.mLqi);
    VerifyOrExit(unpacked > 0, error = OT_ERROR_PARSE);

exit:
    LogIfFail("Handle radio frame failed", error);
    return error;
}

void RadioSpinel::ProcessFrameQueue(void)
{
    uint8_t *frame;
    uint16_t length;

    while (mHdlcInterface.GetRxFrameBuffer().ReadSavedFrame(frame, length) == OT_ERROR_NONE)
    {
        HandleNotification(frame, length);
    }
}

void RadioSpinel::RadioReceive(void)
{
    if (!mIsPromiscuous)
    {
        switch (mState)
        {
        case kStateDisabled:
        case kStateSleep:
            ExitNow();

        case kStateReceive:
        case kStateTransmitPending:
        case kStateTransmitting:
        case kStateTransmitDone:
            break;
        }
    }

#if OPENTHREAD_ENABLE_DIAG
    if (otPlatDiagModeGet())
    {
        otPlatDiagRadioReceiveDone(mInstance, &mRxRadioFrame, OT_ERROR_NONE);
    }
    else
#endif
    {
        otPlatRadioReceiveDone(mInstance, &mRxRadioFrame, OT_ERROR_NONE);
    }

exit:
    return;
}

void RadioSpinel::UpdateFdSet(fd_set &aReadFdSet, fd_set &aWriteFdSet, int &aMaxFd, struct timeval &aTimeout)
{
    int sockFd = mHdlcInterface.GetSocket();

    FD_SET(sockFd, &aReadFdSet);

    if (aMaxFd < sockFd)
    {
        aMaxFd = sockFd;
    }

    if (mState == kStateTransmitPending)
    {
        FD_SET(sockFd, &aWriteFdSet);
    }

    if (mHdlcInterface.GetRxFrameBuffer().HasSavedFrame() || (mState == kStateTransmitDone))
    {
        aTimeout.tv_sec  = 0;
        aTimeout.tv_usec = 0;
    }
}

void RadioSpinel::Process(const fd_set &aReadFdSet, const fd_set &aWriteFdSet)
{
    if (mHdlcInterface.GetRxFrameBuffer().HasSavedFrame())
    {
        // Handle frames received and saved during `WaitResponse()`
        ProcessFrameQueue();
    }

    if (FD_ISSET(mHdlcInterface.GetSocket(), &aReadFdSet))
    {
        mHdlcInterface.Read();
        ProcessFrameQueue();
    }

    mHdlcInterface.GetRxFrameBuffer().ClearReadFrames();

    if (mState == kStateTransmitDone)
    {
        mState = kStateReceive;

#if OPENTHREAD_ENABLE_DIAG
        if (otPlatDiagModeGet())
        {
            otPlatDiagRadioTransmitDone(mInstance, mTransmitFrame, mTxError);
        }
        else
#endif
        {
            otPlatRadioTxDone(mInstance, mTransmitFrame, (mAckRadioFrame.mLength != 0) ? &mAckRadioFrame : NULL,
                              mTxError);
        }
    }

    if (FD_ISSET(mHdlcInterface.GetSocket(), &aWriteFdSet))
    {
        if (mState == kStateTransmitPending)
        {
            RadioTransmit();
        }
    }
}

otError RadioSpinel::SetPromiscuous(bool aEnable)
{
    otError error;

    uint8_t mode = (aEnable ? SPINEL_MAC_PROMISCUOUS_MODE_NETWORK : SPINEL_MAC_PROMISCUOUS_MODE_OFF);
    SuccessOrExit(error = Set(SPINEL_PROP_MAC_PROMISCUOUS_MODE, SPINEL_DATATYPE_UINT8_S, mode));
    mIsPromiscuous = aEnable;

exit:
    return error;
}

otError RadioSpinel::SetShortAddress(uint16_t aAddress)
{
    otError error = OT_ERROR_NONE;

    VerifyOrExit(mShortAddress != aAddress);
    SuccessOrExit(error = sRadioSpinel.Set(SPINEL_PROP_MAC_15_4_SADDR, SPINEL_DATATYPE_UINT16_S, aAddress));
    mShortAddress = aAddress;

exit:
    return error;
}

otError RadioSpinel::GetIeeeEui64(uint8_t *aIeeeEui64)
{
    return Get(SPINEL_PROP_HWADDR, SPINEL_DATATYPE_EUI64_S, aIeeeEui64);
}

otError RadioSpinel::SetExtendedAddress(const otExtAddress &aExtAddress)
{
    otError error;

    SuccessOrExit(error = Set(SPINEL_PROP_MAC_15_4_LADDR, SPINEL_DATATYPE_EUI64_S, aExtAddress.m8));
    mExtendedAddress = aExtAddress;

exit:
    return error;
}

otError RadioSpinel::SetPanId(uint16_t aPanId)
{
    otError error = OT_ERROR_NONE;

    VerifyOrExit(mPanId != aPanId);
    SuccessOrExit(error = Set(SPINEL_PROP_MAC_15_4_PANID, SPINEL_DATATYPE_UINT16_S, aPanId));
    mPanId = aPanId;

exit:
    return error;
}

otError RadioSpinel::EnableSrcMatch(bool aEnable)
{
    return Set(SPINEL_PROP_MAC_SRC_MATCH_ENABLED, SPINEL_DATATYPE_BOOL_S, aEnable);
}

otError RadioSpinel::AddSrcMatchShortEntry(const uint16_t aShortAddress)
{
    return Insert(SPINEL_PROP_MAC_SRC_MATCH_SHORT_ADDRESSES, SPINEL_DATATYPE_UINT16_S, aShortAddress);
}

otError RadioSpinel::AddSrcMatchExtEntry(const otExtAddress &aExtAddress)
{
    return Insert(SPINEL_PROP_MAC_SRC_MATCH_EXTENDED_ADDRESSES, SPINEL_DATATYPE_EUI64_S, aExtAddress.m8);
}

otError RadioSpinel::ClearSrcMatchShortEntry(const uint16_t aShortAddress)
{
    return Remove(SPINEL_PROP_MAC_SRC_MATCH_SHORT_ADDRESSES, SPINEL_DATATYPE_UINT16_S, aShortAddress);
}

otError RadioSpinel::ClearSrcMatchExtEntry(const otExtAddress &aExtAddress)
{
    return Remove(SPINEL_PROP_MAC_SRC_MATCH_EXTENDED_ADDRESSES, SPINEL_DATATYPE_EUI64_S, aExtAddress.m8);
}

otError RadioSpinel::ClearSrcMatchShortEntries(void)
{
    return Set(SPINEL_PROP_MAC_SRC_MATCH_SHORT_ADDRESSES, NULL);
}

otError RadioSpinel::ClearSrcMatchExtEntries(void)
{
    return Set(SPINEL_PROP_MAC_SRC_MATCH_EXTENDED_ADDRESSES, NULL);
}

otError RadioSpinel::GetTransmitPower(int8_t &aPower)
{
    otError error = Get(SPINEL_PROP_PHY_TX_POWER, SPINEL_DATATYPE_INT8_S, &aPower);

    LogIfFail("Get transmit power failed", error);
    return error;
}

int8_t RadioSpinel::GetRssi(void)
{
    int8_t  rssi  = OT_RADIO_RSSI_INVALID;
    otError error = Get(SPINEL_PROP_PHY_RSSI, SPINEL_DATATYPE_INT8_S, &rssi);

    LogIfFail("Get RSSI failed", error);
    return rssi;
}

otError RadioSpinel::SetTransmitPower(int8_t aPower)
{
    otError error = Set(SPINEL_PROP_PHY_TX_POWER, SPINEL_DATATYPE_INT8_S, aPower);
    LogIfFail("Set transmit power failed", error);
    return error;
}

otError RadioSpinel::EnergyScan(uint8_t aScanChannel, uint16_t aScanDuration)
{
    otError error;

    VerifyOrExit(mRadioCaps & OT_RADIO_CAPS_ENERGY_SCAN, error = OT_ERROR_NOT_CAPABLE);

    SuccessOrExit(error = Set(SPINEL_PROP_MAC_SCAN_MASK, SPINEL_DATATYPE_DATA_S, &aScanChannel, sizeof(uint8_t)));
    SuccessOrExit(error = Set(SPINEL_PROP_MAC_SCAN_PERIOD, SPINEL_DATATYPE_UINT16_S, aScanDuration));
    SuccessOrExit(error = Set(SPINEL_PROP_MAC_SCAN_STATE, SPINEL_DATATYPE_UINT8_S, SPINEL_SCAN_STATE_ENERGY));

exit:
    return error;
}

otError RadioSpinel::Get(spinel_prop_key_t aKey, const char *aFormat, ...)
{
    otError error;

    assert(mWaitingTid == 0);

    mPropertyFormat = aFormat;
    va_start(mPropertyArgs, aFormat);
    error = RequestV(true, SPINEL_CMD_PROP_VALUE_GET, aKey, NULL, mPropertyArgs);
    va_end(mPropertyArgs);
    mPropertyFormat = NULL;

    return error;
}

otError RadioSpinel::Set(spinel_prop_key_t aKey, const char *aFormat, ...)
{
    otError error;

    assert(mWaitingTid == 0);

    mExpectedCommand = SPINEL_CMD_PROP_VALUE_IS;
    va_start(mPropertyArgs, aFormat);
    error = RequestV(true, SPINEL_CMD_PROP_VALUE_SET, aKey, aFormat, mPropertyArgs);
    va_end(mPropertyArgs);
    mExpectedCommand = SPINEL_CMD_NOOP;

    return error;
}

otError RadioSpinel::Insert(spinel_prop_key_t aKey, const char *aFormat, ...)
{
    otError error;

    assert(mWaitingTid == 0);

    mExpectedCommand = SPINEL_CMD_PROP_VALUE_INSERTED;
    va_start(mPropertyArgs, aFormat);
    error = RequestV(true, SPINEL_CMD_PROP_VALUE_INSERT, aKey, aFormat, mPropertyArgs);
    va_end(mPropertyArgs);
    mExpectedCommand = SPINEL_CMD_NOOP;

    return error;
}

otError RadioSpinel::Remove(spinel_prop_key_t aKey, const char *aFormat, ...)
{
    otError error;

    assert(mWaitingTid == 0);

    mExpectedCommand = SPINEL_CMD_PROP_VALUE_REMOVED;
    va_start(mPropertyArgs, aFormat);
    error = RequestV(true, SPINEL_CMD_PROP_VALUE_REMOVE, aKey, aFormat, mPropertyArgs);
    va_end(mPropertyArgs);
    mExpectedCommand = SPINEL_CMD_NOOP;

    return error;
}

otError RadioSpinel::WaitResponse(void)
{
    struct timeval end;
    struct timeval now;
    struct timeval timeout = {kMaxWaitTime / 1000, (kMaxWaitTime % 1000) * 1000};

    otSysGetTime(&now);
    timeradd(&now, &timeout, &end);

    do
    {
#if OPENTHREAD_POSIX_VIRTUAL_TIME
        struct Event event;

        otSimSendSleepEvent(&timeout);
        otSimReceiveEvent(&event);

        switch (event.mEvent)
        {
        case OT_SIM_EVENT_RADIO_SPINEL_WRITE:
            mHdlcInterface.ProcessReadData(event.mData, event.mDataLength);
            break;

        case OT_SIM_EVENT_ALARM_FIRED:
            FreeTid(mWaitingTid);
            mWaitingTid = 0;
            ExitNow(mError = OT_ERROR_RESPONSE_TIMEOUT);
            break;

        default:
            assert(false);
            break;
        }
#else  // OPENTHREAD_POSIX_VIRTUAL_TIME
        int    sockFd = mHdlcInterface.GetSocket();
        fd_set read_fds;
        fd_set error_fds;
        int    rval;

        FD_ZERO(&read_fds);
        FD_ZERO(&error_fds);
        FD_SET(sockFd, &read_fds);
        FD_SET(sockFd, &error_fds);

        rval = select(sockFd + 1, &read_fds, NULL, &error_fds, &timeout);

        if (rval > 0)
        {
            if (FD_ISSET(sockFd, &read_fds))
            {
                mHdlcInterface.Read();
            }
            else if (FD_ISSET(sockFd, &error_fds))
            {
                fprintf(stderr, "NCP error\r\n");
                exit(OT_EXIT_FAILURE);
            }
            else
            {
                assert(false);
                exit(OT_EXIT_FAILURE);
            }
        }
        else if (rval == 0)
        {
            FreeTid(mWaitingTid);
            mWaitingTid = 0;
            ExitNow(mError = OT_ERROR_RESPONSE_TIMEOUT);
        }
        else if (errno != EINTR)
        {
            perror("wait response");
            exit(OT_EXIT_FAILURE);
        }
#endif // OPENTHREAD_POSIX_VIRTUAL_TIME

        otSysGetTime(&now);
        if (timercmp(&end, &now, >))
        {
            timersub(&end, &now, &timeout);
        }
        else
        {
            mWaitingTid = 0;
            mError      = OT_ERROR_RESPONSE_TIMEOUT;
        }
    } while (mWaitingTid || !mIsReady);

exit:
    LogIfFail("Error waiting response", mError);
    // This indicates end of waiting repsonse.
    mWaitingKey = SPINEL_PROP_LAST_STATUS;
    return mError;
}

spinel_tid_t RadioSpinel::GetNextTid(void)
{
    spinel_tid_t tid = 0;

    if (((1 << mCmdNextTid) & mCmdTidsInUse) == 0)
    {
        tid         = mCmdNextTid;
        mCmdNextTid = SPINEL_GET_NEXT_TID(mCmdNextTid);
        mCmdTidsInUse |= (1 << tid);
    }

    return tid;
}

/**
 * This method delivers the radio frame to transceiver.
 *
 * otPlatRadioTxStarted() is triggered immediately for now, which may be earlier than real started time.
 *
 */
void RadioSpinel::RadioTransmit(void)
{
    otError error;

    assert(mTransmitFrame != NULL);
    otPlatRadioTxStarted(mInstance, mTransmitFrame);
    assert(mState == kStateTransmitPending);

    error = Request(true, SPINEL_CMD_PROP_VALUE_SET, SPINEL_PROP_STREAM_RAW,
                    SPINEL_DATATYPE_DATA_WLEN_S SPINEL_DATATYPE_UINT8_S SPINEL_DATATYPE_INT8_S, mTransmitFrame->mPsdu,
                    mTransmitFrame->mLength, mTransmitFrame->mChannel, mTransmitFrame->mInfo.mRxInfo.mRssi);

    if (error == OT_ERROR_NONE)
    {
        mState = kStateTransmitting;
    }
    else
    {
        mState = kStateReceive;

#if OPENTHREAD_ENABLE_DIAG

        if (otPlatDiagModeGet())
        {
            otPlatDiagRadioTransmitDone(mInstance, mTransmitFrame, error);
        }
        else
#endif
        {
            otPlatRadioTxDone(mInstance, mTransmitFrame, NULL, error);
        }
    }
}

otError RadioSpinel::SendReset(void)
{
    otError        error = OT_ERROR_NONE;
    uint8_t        buffer[kMaxSpinelFrame];
    spinel_ssize_t packed;

    // Pack the header, command and key
    packed =
        spinel_datatype_pack(buffer, sizeof(buffer), "Ci", SPINEL_HEADER_FLAG | SPINEL_HEADER_IID_0, SPINEL_CMD_RESET);

    VerifyOrExit(packed > 0 && static_cast<size_t>(packed) <= sizeof(buffer), error = OT_ERROR_NO_BUFS);

    SuccessOrExit(error = mHdlcInterface.SendFrame(buffer, static_cast<uint16_t>(packed)));

    sleep(0);

exit:
    return error;
}

otError RadioSpinel::SendCommand(uint32_t          aCommand,
                                 spinel_prop_key_t aKey,
                                 spinel_tid_t      tid,
                                 const char *      aFormat,
                                 va_list           args)
{
    otError        error = OT_ERROR_NONE;
    uint8_t        buffer[kMaxSpinelFrame];
    spinel_ssize_t packed;
    uint16_t       offset;

    // Pack the header, command and key
    packed = spinel_datatype_pack(buffer, sizeof(buffer), "Cii", SPINEL_HEADER_FLAG | SPINEL_HEADER_IID_0 | tid,
                                  aCommand, aKey);

    VerifyOrExit(packed > 0 && static_cast<size_t>(packed) <= sizeof(buffer), error = OT_ERROR_NO_BUFS);

    offset = static_cast<uint16_t>(packed);

    // Pack the data (if any)
    if (aFormat)
    {
        packed = spinel_datatype_vpack(buffer + offset, sizeof(buffer) - offset, aFormat, args);
        VerifyOrExit(packed > 0 && static_cast<size_t>(packed + offset) <= sizeof(buffer), error = OT_ERROR_NO_BUFS);

        offset += static_cast<uint16_t>(packed);
    }

    error = mHdlcInterface.SendFrame(buffer, offset);

exit:
    return error;
}

otError RadioSpinel::RequestV(bool aWait, uint32_t command, spinel_prop_key_t aKey, const char *aFormat, va_list aArgs)
{
    otError      error = OT_ERROR_NONE;
    spinel_tid_t tid   = (aWait ? GetNextTid() : 0);

    VerifyOrExit(!aWait || tid > 0, error = OT_ERROR_BUSY);

    error = SendCommand(command, aKey, tid, aFormat, aArgs);
    VerifyOrExit(error == OT_ERROR_NONE);

    if (aKey == SPINEL_PROP_STREAM_RAW)
    {
        // not allowed to send another frame before the last frame is done.
        assert(mTxRadioTid == 0);
        VerifyOrExit(mTxRadioTid == 0, error = OT_ERROR_BUSY);
        mTxRadioTid = tid;
    }
    else if (aWait)
    {
        mWaitingKey = aKey;
        mWaitingTid = tid;
        error       = WaitResponse();
    }

exit:
    return error;
}

otError RadioSpinel::Request(bool aWait, uint32_t aCommand, spinel_prop_key_t aKey, const char *aFormat, ...)
{
    va_list args;
    va_start(args, aFormat);
    otError status = RequestV(aWait, aCommand, aKey, aFormat, args);
    va_end(args);
    return status;
}

void RadioSpinel::HandleTransmitDone(uint32_t          aCommand,
                                     spinel_prop_key_t aKey,
                                     const uint8_t *   aBuffer,
                                     uint16_t          aLength)
{
    otError         error  = OT_ERROR_NONE;
    spinel_status_t status = SPINEL_STATUS_OK;
    spinel_ssize_t  unpacked;

    VerifyOrExit(aCommand == SPINEL_CMD_PROP_VALUE_IS && aKey == SPINEL_PROP_LAST_STATUS, error = OT_ERROR_FAILED);

    unpacked = spinel_datatype_unpack(aBuffer, aLength, SPINEL_DATATYPE_UINT_PACKED_S, &status);
    VerifyOrExit(unpacked > 0, error = OT_ERROR_PARSE);

    aBuffer += unpacked;
    aLength -= static_cast<uint16_t>(unpacked);

    if (status == SPINEL_STATUS_OK)
    {
        bool framePending = false;
        unpacked          = spinel_datatype_unpack(aBuffer, aLength, SPINEL_DATATYPE_BOOL_S, &framePending);
        OT_UNUSED_VARIABLE(framePending);
        VerifyOrExit(unpacked > 0, error = OT_ERROR_PARSE);

        aBuffer += unpacked;
        aLength -= static_cast<spinel_size_t>(unpacked);

        if (aLength > 0)
        {
            SuccessOrExit(error = ParseRadioFrame(mAckRadioFrame, aBuffer, aLength));
        }
        else
        {
            mAckRadioFrame.mLength = 0;
        }
    }
    else
    {
        otLogWarnPlat("Spinel status: %d.", status);
        error = SpinelStatusToOtError(status);
    }

exit:
    mState   = kStateTransmitDone;
    mTxError = error;
    LogIfFail("Handle transmit done failed", error);
}

otError RadioSpinel::Transmit(otRadioFrame &aFrame)
{
    otError error = OT_ERROR_INVALID_STATE;

    VerifyOrExit(mState == kStateReceive);
    mState         = kStateTransmitPending;
    error          = OT_ERROR_NONE;
    mTransmitFrame = &aFrame;

exit:
    return error;
}

otError RadioSpinel::Receive(uint8_t aChannel)
{
    otError error = OT_ERROR_NONE;

    VerifyOrExit(mState != kStateDisabled, error = OT_ERROR_INVALID_STATE);

    if (mChannel != aChannel)
    {
        error = Set(SPINEL_PROP_PHY_CHAN, SPINEL_DATATYPE_UINT8_S, aChannel);
        VerifyOrExit(error == OT_ERROR_NONE);
        mChannel = aChannel;
    }

    if (mState == kStateSleep)
    {
        error = Set(SPINEL_PROP_MAC_RAW_STREAM_ENABLED, SPINEL_DATATYPE_BOOL_S, true);
        VerifyOrExit(error == OT_ERROR_NONE);
    }

    if (mTxRadioTid != 0)
    {
        FreeTid(mTxRadioTid);
        mTxRadioTid = 0;
    }

    mState = kStateReceive;

exit:
    assert(error == OT_ERROR_NONE);
    return error;
}

otError RadioSpinel::Sleep(void)
{
    otError error = OT_ERROR_NONE;

    switch (mState)
    {
    case kStateReceive:
        error = sRadioSpinel.Set(SPINEL_PROP_MAC_RAW_STREAM_ENABLED, SPINEL_DATATYPE_BOOL_S, false);
        VerifyOrExit(error == OT_ERROR_NONE);

        mState = kStateSleep;
        break;

    case kStateSleep:
        break;

    default:
        error = OT_ERROR_INVALID_STATE;
        break;
    }

exit:
    return error;
}

otError RadioSpinel::Enable(otInstance *aInstance)
{
    otError error = OT_ERROR_NONE;

    if (!otPlatRadioIsEnabled(mInstance))
    {
        mInstance = aInstance;

        SuccessOrExit(error = Set(SPINEL_PROP_PHY_ENABLED, SPINEL_DATATYPE_BOOL_S, true));
        SuccessOrExit(error = Set(SPINEL_PROP_MAC_15_4_PANID, SPINEL_DATATYPE_UINT16_S, mPanId));
        SuccessOrExit(error = Set(SPINEL_PROP_MAC_15_4_SADDR, SPINEL_DATATYPE_UINT16_S, mShortAddress));

        error = Get(SPINEL_PROP_PHY_RX_SENSITIVITY, SPINEL_DATATYPE_INT8_S, &mRxSensitivity);
        VerifyOrExit(error == OT_ERROR_NONE);

        mState = kStateSleep;
    }

exit:
    assert(error == OT_ERROR_NONE);
    return error;
}

otError RadioSpinel::Disable(void)
{
    otError error = OT_ERROR_NONE;

    if (otPlatRadioIsEnabled(mInstance))
    {
        mInstance = NULL;
        error     = sRadioSpinel.Set(SPINEL_PROP_PHY_ENABLED, SPINEL_DATATYPE_BOOL_S, false);
        VerifyOrExit(error == OT_ERROR_NONE);

        mState = kStateDisabled;
    }

exit:
    return error;
}

#if OPENTHREAD_ENABLE_DIAG
otError RadioSpinel::PlatDiagProcess(const char *aString, char *aOutput, size_t aOutputMaxLen)
{
    otError error;

    mDiagOutput       = aOutput;
    mDiagOutputMaxLen = aOutputMaxLen;

    error = Set(SPINEL_PROP_NEST_STREAM_MFG, SPINEL_DATATYPE_UTF8_S, aString);

    mDiagOutput       = NULL;
    mDiagOutputMaxLen = 0;

    return error;
}
#endif

} // namespace PosixApp
} // namespace ot

void otPlatRadioGetIeeeEui64(otInstance *aInstance, uint8_t *aIeeeEui64)
{
    SuccessOrDie(sRadioSpinel.GetIeeeEui64(aIeeeEui64));
    OT_UNUSED_VARIABLE(aInstance);
}

void otPlatRadioSetPanId(otInstance *aInstance, uint16_t panid)
{
    SuccessOrDie(sRadioSpinel.SetPanId(panid));
    OT_UNUSED_VARIABLE(aInstance);
}

void otPlatRadioSetExtendedAddress(otInstance *aInstance, const otExtAddress *aAddress)
{
    otExtAddress addr;

    for (size_t i = 0; i < sizeof(addr); i++)
    {
        addr.m8[i] = aAddress->m8[sizeof(addr) - 1 - i];
    }

    SuccessOrDie(sRadioSpinel.SetExtendedAddress(addr));
    OT_UNUSED_VARIABLE(aInstance);
}

void otPlatRadioSetShortAddress(otInstance *aInstance, uint16_t aAddress)
{
    SuccessOrDie(sRadioSpinel.SetShortAddress(aAddress));
    OT_UNUSED_VARIABLE(aInstance);
}

void otPlatRadioSetPromiscuous(otInstance *aInstance, bool aEnable)
{
    SuccessOrDie(sRadioSpinel.SetPromiscuous(aEnable));
    OT_UNUSED_VARIABLE(aInstance);
}

void platformRadioInit(const char *aRadioFile, const char *aRadioConfig)
{
    sRadioSpinel.Init(aRadioFile, aRadioConfig);
}

void platformRadioDeinit(void)
{
    sRadioSpinel.Deinit();
}

bool otPlatRadioIsEnabled(otInstance *aInstance)
{
    OT_UNUSED_VARIABLE(aInstance);
    return sRadioSpinel.IsEnabled();
}

otError otPlatRadioEnable(otInstance *aInstance)
{
    return sRadioSpinel.Enable(aInstance);
}

otError otPlatRadioDisable(otInstance *aInstance)
{
    OT_UNUSED_VARIABLE(aInstance);
    return sRadioSpinel.Disable();
}

otError otPlatRadioSleep(otInstance *aInstance)
{
    OT_UNUSED_VARIABLE(aInstance);
    return sRadioSpinel.Sleep();
}

otError otPlatRadioReceive(otInstance *aInstance, uint8_t aChannel)
{
    OT_UNUSED_VARIABLE(aInstance);
    return sRadioSpinel.Receive(aChannel);
}

otError otPlatRadioTransmit(otInstance *aInstance, otRadioFrame *aFrame)
{
    OT_UNUSED_VARIABLE(aInstance);
    return sRadioSpinel.Transmit(*aFrame);
}

otRadioFrame *otPlatRadioGetTransmitBuffer(otInstance *aInstance)
{
    OT_UNUSED_VARIABLE(aInstance);
    return &sRadioSpinel.GetTransmitFrame();
}

int8_t otPlatRadioGetRssi(otInstance *aInstance)
{
    OT_UNUSED_VARIABLE(aInstance);
    return sRadioSpinel.GetRssi();
}

otRadioCaps otPlatRadioGetCaps(otInstance *aInstance)
{
    OT_UNUSED_VARIABLE(aInstance);
    return sRadioSpinel.GetRadioCaps();
}

const char *otPlatRadioGetVersionString(otInstance *aInstance)
{
    OT_UNUSED_VARIABLE(aInstance);
    return sRadioSpinel.GetVersion();
}

bool otPlatRadioGetPromiscuous(otInstance *aInstance)
{
    OT_UNUSED_VARIABLE(aInstance);
    return sRadioSpinel.IsPromiscuous();
}

void platformRadioUpdateFdSet(fd_set *aReadFdSet, fd_set *aWriteFdSet, int *aMaxFd, struct timeval *aTimeout)
{
    sRadioSpinel.UpdateFdSet(*aReadFdSet, *aWriteFdSet, *aMaxFd, *aTimeout);
}

void platformRadioProcess(otInstance *aInstance, const fd_set *aReadFdSet, const fd_set *aWriteFdSet)
{
    sRadioSpinel.Process(*aReadFdSet, *aWriteFdSet);
    OT_UNUSED_VARIABLE(aInstance);
}

void otPlatRadioEnableSrcMatch(otInstance *aInstance, bool aEnable)
{
    SuccessOrDie(sRadioSpinel.EnableSrcMatch(aEnable));
    OT_UNUSED_VARIABLE(aInstance);
}

otError otPlatRadioAddSrcMatchShortEntry(otInstance *aInstance, uint16_t aShortAddress)
{
    OT_UNUSED_VARIABLE(aInstance);
    return sRadioSpinel.AddSrcMatchShortEntry(aShortAddress);
}

otError otPlatRadioAddSrcMatchExtEntry(otInstance *aInstance, const otExtAddress *aExtAddress)
{
    otExtAddress addr;

    for (size_t i = 0; i < sizeof(addr); i++)
    {
        addr.m8[i] = aExtAddress->m8[sizeof(addr) - 1 - i];
    }

    OT_UNUSED_VARIABLE(aInstance);
    return sRadioSpinel.AddSrcMatchExtEntry(addr);
}

otError otPlatRadioClearSrcMatchShortEntry(otInstance *aInstance, uint16_t aShortAddress)
{
    OT_UNUSED_VARIABLE(aInstance);
    return sRadioSpinel.ClearSrcMatchShortEntry(aShortAddress);
}

otError otPlatRadioClearSrcMatchExtEntry(otInstance *aInstance, const otExtAddress *aExtAddress)
{
    otExtAddress addr;

    for (size_t i = 0; i < sizeof(addr); i++)
    {
        addr.m8[i] = aExtAddress->m8[sizeof(addr) - 1 - i];
    }

    OT_UNUSED_VARIABLE(aInstance);
    return sRadioSpinel.ClearSrcMatchExtEntry(addr);
}

void otPlatRadioClearSrcMatchShortEntries(otInstance *aInstance)
{
    SuccessOrDie(sRadioSpinel.ClearSrcMatchShortEntries());
    OT_UNUSED_VARIABLE(aInstance);
}

void otPlatRadioClearSrcMatchExtEntries(otInstance *aInstance)
{
    SuccessOrDie(sRadioSpinel.ClearSrcMatchExtEntries());
    OT_UNUSED_VARIABLE(aInstance);
}

otError otPlatRadioEnergyScan(otInstance *aInstance, uint8_t aScanChannel, uint16_t aScanDuration)
{
    OT_UNUSED_VARIABLE(aInstance);
    return sRadioSpinel.EnergyScan(aScanChannel, aScanDuration);
}

otError otPlatRadioGetTransmitPower(otInstance *aInstance, int8_t *aPower)
{
    assert(aPower != NULL);
    OT_UNUSED_VARIABLE(aInstance);
    return sRadioSpinel.GetTransmitPower(*aPower);
}

otError otPlatRadioSetTransmitPower(otInstance *aInstance, int8_t aPower)
{
    OT_UNUSED_VARIABLE(aInstance);
    return sRadioSpinel.SetTransmitPower(aPower);
}

int8_t otPlatRadioGetReceiveSensitivity(otInstance *aInstance)
{
    OT_UNUSED_VARIABLE(aInstance);
    return sRadioSpinel.GetReceiveSensitivity();
}

#if OPENTHREAD_POSIX_VIRTUAL_TIME
void ot::PosixApp::RadioSpinel::Process(const Event &aEvent)
{
    if (mHdlcInterface.GetRxFrameBuffer().HasSavedFrame())
    {
        ProcessFrameQueue();
    }

    // The current event can be other event types
    if (aEvent.mEvent == OT_SIM_EVENT_RADIO_SPINEL_WRITE)
    {
        mHdlcInterface.ProcessReadData(aEvent.mData, aEvent.mDataLength);
        ProcessFrameQueue();
    }

    mHdlcInterface.GetRxFrameBuffer().ClearReadFrames();

    if (mState == kStateTransmitDone)
    {
        mState = kStateReceive;

#if OPENTHREAD_ENABLE_DIAG
        if (otPlatDiagModeGet())
        {
            otPlatDiagRadioTransmitDone(mInstance, mTransmitFrame, mTxError);
        }
        else
#endif
        {
            otPlatRadioTxDone(mInstance, mTransmitFrame, (mAckRadioFrame.mLength != 0) ? &mAckRadioFrame : NULL,
                              mTxError);
        }
    }

    if (mState == kStateTransmitPending)
    {
        RadioTransmit();
    }
}

void ot::PosixApp::RadioSpinel::Update(struct timeval &aTimeout)
{
    // Prevent sleep event when transmitting
    if (mState == kStateTransmitPending)
    {
        aTimeout.tv_sec  = 0;
        aTimeout.tv_usec = 0;
    }
}

void otSimRadioSpinelUpdate(struct timeval *aTimeout)
{
    sRadioSpinel.Update(*aTimeout);
}

void otSimRadioSpinelProcess(otInstance *aInstance, const struct Event *aEvent)
{
    sRadioSpinel.Process(*aEvent);
    OT_UNUSED_VARIABLE(aInstance);
}
#endif // OPENTHREAD_POSIX_VIRTUAL_TIME

#if OPENTHREAD_ENABLE_DIAG
void otPlatDiagProcess(otInstance *aInstance, int argc, char *argv[], char *aOutput, size_t aOutputMaxLen)
{
    // deliver the platform specific diags commands to radio only ncp.
    OT_UNUSED_VARIABLE(aInstance);
    char  cmd[OPENTHREAD_CONFIG_DIAG_CMD_LINE_BUFFER_SIZE] = {'\0'};
    char *cur                                              = cmd;
    char *end                                              = cmd + sizeof(cmd);

    for (int index = 0; index < argc; index++)
    {
        cur += snprintf(cur, static_cast<size_t>(end - cur), "%s ", argv[index]);
    }

    sRadioSpinel.PlatDiagProcess(cmd, aOutput, aOutputMaxLen);
}

void otPlatDiagModeSet(bool aMode)
{
    SuccessOrExit(sRadioSpinel.PlatDiagProcess(aMode ? "start" : "stop", NULL, 0));
    sRadioSpinel.SetDiagEnabled(aMode);

exit:
    return;
}

bool otPlatDiagModeGet(void)
{
    return sRadioSpinel.IsDiagEnabled();
}

void otPlatDiagTxPowerSet(int8_t aTxPower)
{
    char cmd[OPENTHREAD_CONFIG_DIAG_CMD_LINE_BUFFER_SIZE];

    snprintf(cmd, sizeof(cmd), "power %d", aTxPower);
    SuccessOrExit(sRadioSpinel.PlatDiagProcess(cmd, NULL, 0));

exit:
    return;
}

void otPlatDiagChannelSet(uint8_t aChannel)
{
    char cmd[OPENTHREAD_CONFIG_DIAG_CMD_LINE_BUFFER_SIZE];

    snprintf(cmd, sizeof(cmd), "channel %d", aChannel);
    SuccessOrExit(sRadioSpinel.PlatDiagProcess(cmd, NULL, 0));

exit:
    return;
}

void otPlatDiagRadioReceived(otInstance *aInstance, otRadioFrame *aFrame, otError aError)
{
    OT_UNUSED_VARIABLE(aInstance);
    OT_UNUSED_VARIABLE(aFrame);
    OT_UNUSED_VARIABLE(aError);
}

void otPlatDiagAlarmCallback(otInstance *aInstance)
{
    OT_UNUSED_VARIABLE(aInstance);
}
#endif // OPENTHREAD_ENABLE_DIAG
