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

extern "C" {
#include "platform-posix.h"
}

#include "radio_spinel.hpp"

#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <pty.h>
#include <stdarg.h>
#include <stdlib.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <syslog.h>
#include <termios.h>
#include <unistd.h>

#include <common/code_utils.hpp>
#include <common/logging.hpp>
#include <openthread/platform/alarm-milli.h>
#include <openthread/platform/diag.h>
#include <openthread/platform/radio.h>

#ifndef SOCKET_UTILS_DEFAULT_SHELL
#define SOCKET_UTILS_DEFAULT_SHELL "/bin/sh"
#endif

enum
{
    IEEE802154_MIN_LENGTH = 5,
    IEEE802154_MAX_LENGTH = 127,
    IEEE802154_ACK_LENGTH = 5,

    IEEE802154_BROADCAST = 0xffff,

    IEEE802154_FRAME_TYPE_ACK    = 2 << 0,
    IEEE802154_FRAME_TYPE_MACCMD = 3 << 0,
    IEEE802154_FRAME_TYPE_MASK   = 7 << 0,

    IEEE802154_SECURITY_ENABLED  = 1 << 3,
    IEEE802154_FRAME_PENDING     = 1 << 4,
    IEEE802154_ACK_REQUEST       = 1 << 5,
    IEEE802154_PANID_COMPRESSION = 1 << 6,

    IEEE802154_DST_ADDR_NONE  = 0 << 2,
    IEEE802154_DST_ADDR_SHORT = 2 << 2,
    IEEE802154_DST_ADDR_EXT   = 3 << 2,
    IEEE802154_DST_ADDR_MASK  = 3 << 2,

    IEEE802154_SRC_ADDR_NONE  = 0 << 6,
    IEEE802154_SRC_ADDR_SHORT = 2 << 6,
    IEEE802154_SRC_ADDR_EXT   = 3 << 6,
    IEEE802154_SRC_ADDR_MASK  = 3 << 6,

    IEEE802154_DSN_OFFSET     = 2,
    IEEE802154_DSTPAN_OFFSET  = 3,
    IEEE802154_DSTADDR_OFFSET = 5,

    IEEE802154_SEC_LEVEL_MASK = 7 << 0,

    IEEE802154_KEY_ID_MODE_0    = 0 << 3,
    IEEE802154_KEY_ID_MODE_1    = 1 << 3,
    IEEE802154_KEY_ID_MODE_2    = 2 << 3,
    IEEE802154_KEY_ID_MODE_3    = 3 << 3,
    IEEE802154_KEY_ID_MODE_MASK = 3 << 3,

    IEEE802154_MACCMD_DATA_REQ = 4,
};

enum
{
    kIdle,
    kSent,
    kDone,
};

static ot::RadioSpinel sRadioSpinel;

static inline otPanId getDstPan(const uint8_t *frame)
{
    return static_cast<otPanId>((frame[IEEE802154_DSTPAN_OFFSET + 1] << 8) | frame[IEEE802154_DSTPAN_OFFSET]);
}

static inline otShortAddress getShortAddress(const uint8_t *frame)
{
    return static_cast<otShortAddress>((frame[IEEE802154_DSTADDR_OFFSET + 1] << 8) | frame[IEEE802154_DSTADDR_OFFSET]);
}

static inline void getExtAddress(const uint8_t *frame, otExtAddress *address)
{
    size_t i;

    for (i = 0; i < sizeof(otExtAddress); i++)
    {
        address->m8[i] = frame[IEEE802154_DSTADDR_OFFSET + (sizeof(otExtAddress) - 1 - i)];
    }
}

static inline bool isAckRequested(const uint8_t *frame)
{
    return (frame[0] & IEEE802154_ACK_REQUEST) != 0;
}

static inline void SuccessOrDie(otError aError)
{
    if (aError != OT_ERROR_NONE)
    {
        exit(EXIT_FAILURE);
    }
}

namespace ot {

/**
 * This function returns if the property changed event is *UNSAFE* to be handled during `WaitResponse()`.
 *
 * If property could trigger another call to `ncpSet()`, it's unsafe.
 *
 * @param[in] aKey The identifier of the property.
 *
 * @returns Whether this property is *UNSAFE* to be handled during 'WaitResponse()`.
 *
 */
static bool ShouldDefer(spinel_prop_key_t aKey)
{
    return aKey == SPINEL_PROP_STREAM_RAW || aKey == SPINEL_PROP_MAC_ENERGY_SCAN_RESULT;
}

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

static void LogIfFail(otInstance *aInstance, const char *aText, otError aError)
{
    if (aError != OT_ERROR_NONE)
    {
        otLogWarnPlat(aInstance, "%s: %s", aText, otThreadErrorToString(aError));
    }

    OT_UNUSED_VARIABLE(aInstance);
    OT_UNUSED_VARIABLE(aText);
    OT_UNUSED_VARIABLE(aError);
}

static int OpenPty(const char *aFile, const char *aConfig)
{
    int fd  = -1;
    int pid = -1;

    {
        struct termios tios;

        memset(&tios, 0, sizeof(tios));
        cfmakeraw(&tios);
        tios.c_cflag = CS8 | HUPCL | CREAD | CLOCAL;

        pid = forkpty(&fd, NULL, &tios, NULL);
        VerifyOrExit(pid >= 0);
    }

    if (0 == pid)
    {
        const int kMaxCommand = 255;
        char      cmd[kMaxCommand];
        int       rval;
        const int dtablesize = getdtablesize();

        rval = setenv("SHELL", SOCKET_UTILS_DEFAULT_SHELL, 0);

        VerifyOrExit(rval == 0, perror("setenv failed"));

        // Close all file descriptors larger than STDERR_FILENO.
        for (int i = (STDERR_FILENO + 1); i < dtablesize; i++)
        {
            close(i);
        }

        rval = snprintf(cmd, sizeof(cmd), "%s %s", aFile, aConfig);
        VerifyOrExit(rval > 0 && static_cast<size_t>(rval) < sizeof(cmd),
                     otLogCritPlat(mInstance, "NCP file and configuration is too long!"));

        execl(getenv("SHELL"), getenv("SHELL"), "-c", cmd, NULL);
        perror("open pty failed");

        exit(EXIT_FAILURE);
    }
    else
    {
        int rval = fcntl(fd, F_GETFL);

        if (rval != -1)
        {
            rval = fcntl(fd, F_SETFL, rval | O_NONBLOCK);
        }

        if (rval == -1)
        {
            perror("set nonblock failed");
            close(fd);
            fd = -1;
        }
    }

exit:
    return fd;
}

static int OpenUart(const char *aRadioFile, const char *aRadioConfig)
{
    const int kMaxSttyCommand = 128;
    char      cmd[kMaxSttyCommand];
    int       fd = -1;
    int       rval;

    VerifyOrExit(!strchr(aRadioConfig, '&') && !strchr(aRadioConfig, '|') && !strchr(aRadioConfig, ';'),
                 otLogCritPlat(mInstance, "Illegal NCP config arguments!"));

    rval = snprintf(cmd, sizeof(cmd), "stty -F %s %s", aRadioFile, aRadioConfig);
    VerifyOrExit(rval > 0 && static_cast<size_t>(rval) < sizeof(cmd),
                 otLogCritPlat(mInstance, "NCP file and configuration is too long!"));

    rval = system(cmd);
    VerifyOrExit(rval == 0, otLogCritPlat(mInstance, "Unable to configure serial port"));

    fd = open(aRadioFile, O_RDWR | O_NOCTTY | O_NONBLOCK);
    if (fd == -1)
    {
        perror("open uart failed");
        ExitNow();
    }

    rval = tcflush(fd, TCIOFLUSH);
    VerifyOrExit(rval == 0, otLogCritPlat(mInstance, "Unable to flush serial port"));

exit:
    return fd;
}

class UartTxBuffer : public Hdlc::Encoder::BufferWriteIterator
{
public:
    UartTxBuffer(void)
    {
        mWritePointer    = mBuffer;
        mRemainingLength = sizeof(mBuffer);
    }

    uint16_t       GetLength(void) const { return static_cast<uint16_t>(mWritePointer - mBuffer); }
    const uint8_t *GetBuffer(void) const { return mBuffer; }

private:
    enum
    {
        kUartTxBufferSize = 512, // Uart tx buffer size.
    };
    uint8_t mBuffer[kUartTxBufferSize];
};

RadioSpinel::RadioSpinel(void)
    : mCmdTidsInUse(0)
    , mCmdNextTid(1)
    , mTxRadioTid(0)
    , mWaitingTid(0)
    , mWaitingKey(SPINEL_PROP_LAST_STATUS)
    , mHdlcDecoder(mHdlcBuffer, sizeof(mHdlcBuffer), HandleSpinelFrame, HandleHdlcError, this)
    , mRxSensitivity(0)
    , mTxState(kIdle)
    , mSockFd(-1)
    , mState(OT_RADIO_STATE_DISABLED)
    , mAckWait(false)
    , mPromiscuous(false)
    , mIsReady(false)
{
}

void RadioSpinel::Init(const char *aRadioFile, const char *aRadioConfig)
{
    otError     error = OT_ERROR_NONE;
    struct stat st;

    // not allowed to initialize again.
    assert(mSockFd == -1);

    VerifyOrExit(stat(aRadioFile, &st) == 0, perror("stat ncp file failed"));

    if (S_ISCHR(st.st_mode))
    {
        mSockFd = OpenUart(aRadioFile, aRadioConfig);
    }
    else if (S_ISREG(st.st_mode))
    {
        mSockFd = OpenPty(aRadioFile, aRadioConfig);
    }

    VerifyOrExit(mSockFd != -1, error = OT_ERROR_INVALID_ARGS);
    error = SendReset();
    VerifyOrExit(error == OT_ERROR_NONE);
    error = WaitResponse();
    VerifyOrExit(error == OT_ERROR_NONE);
    VerifyOrExit(mIsReady, error = OT_ERROR_FAILED);

    if (OT_ERROR_NONE != Get(SPINEL_PROP_HWADDR, SPINEL_DATATYPE_UINT64_S, &NODE_ID))
    {
        exit(EXIT_FAILURE);
    }

    assert(mSockFd != -1);

    mRxRadioFrame.mPsdu = mRxPsdu;
    mTxRadioFrame.mPsdu = mTxPsdu;

exit:
    if (error != OT_ERROR_NONE)
    {
        exit(EXIT_FAILURE);
    }
}

void RadioSpinel::Deinit(void)
{
    // this function is only allowed after successfully initialized.
    assert(mSockFd != -1);

    VerifyOrExit(0 == close(mSockFd), perror("close NCP"));
    VerifyOrExit(-1 != wait(NULL), perror("wait NCP"));

exit:
    return;
}

void RadioSpinel::HandleSpinelFrame(const uint8_t *aBuffer, uint16_t aLength)
{
    otError        error = OT_ERROR_NONE;
    uint8_t        header;
    spinel_ssize_t rval;

    rval = spinel_datatype_unpack(aBuffer, aLength, "C", &header);

    VerifyOrExit(rval > 0 && (header & SPINEL_HEADER_FLAG) == SPINEL_HEADER_FLAG && SPINEL_HEADER_GET_IID(header) == 0,
                 error = OT_ERROR_PARSE);

    if (SPINEL_HEADER_GET_TID(header) == 0)
    {
        HandleNotification(aBuffer, aLength);
    }
    else
    {
        HandleResponse(aBuffer, aLength);
    }

exit:
    LogIfFail(mInstance, "Error handling hdlc frame", error);
}

void RadioSpinel::HandleHdlcError(void *aContext, otError aError, uint8_t *aBuffer, uint16_t aLength)
{
    otLogWarnPlat(static_cast<RadioSpinel *>(aContext)->mInstance, "Error decoding hdlc frame: %s",
                  otThreadErrorToString(aError));
    OT_UNUSED_VARIABLE(aContext);
    OT_UNUSED_VARIABLE(aError);
    OT_UNUSED_VARIABLE(aBuffer);
    OT_UNUSED_VARIABLE(aLength);
}

void RadioSpinel::HandleNotification(const uint8_t *aBuffer, uint16_t aLength)
{
    spinel_prop_key_t key;
    spinel_size_t     len = 0;
    spinel_ssize_t    unpacked;
    uint8_t *         data = NULL;
    uint32_t          cmd;
    uint8_t           header;
    otError           error = OT_ERROR_NONE;

    unpacked = spinel_datatype_unpack(aBuffer, aLength, "CiiD", &header, &cmd, &key, &data, &len);
    VerifyOrExit(unpacked > 0, error = OT_ERROR_PARSE);
    VerifyOrExit(SPINEL_HEADER_GET_TID(header) == 0, error = OT_ERROR_PARSE);

    switch (cmd)
    {
    case SPINEL_CMD_PROP_VALUE_IS:
        // Some spinel properties cannot be handled during `WaitResponse()`, we must cache these events.
        // `mWaitingTid` is released immediately after received the response. And `mWaitingKey` is be set
        // to `SPINEL_PROP_LAST_STATUS` at the end of `WaitResponse()`.
        VerifyOrExit(mWaitingKey == SPINEL_PROP_LAST_STATUS || !ShouldDefer(key),
                     error = mFrameQueue.Push(aBuffer, aLength));
        HandleValueIs(key, data, static_cast<uint16_t>(len));
        break;

    case SPINEL_CMD_PROP_VALUE_INSERTED:
    case SPINEL_CMD_PROP_VALUE_REMOVED:
        otLogInfoPlat(mInstance, "Ignored command %d", cmd);
        break;

    default:
        ExitNow(error = OT_ERROR_PARSE);
    }

exit:
    LogIfFail(mInstance, "Error processing notification", error);
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
        HandleTransmitDone(cmd, key, data, static_cast<uint16_t>(len));
        FreeTid(mTxRadioTid);
        mTxRadioTid = 0;
    }
    else
    {
        otLogWarnPlat(mInstance, "Unexpected Spinel transaction message: %u", SPINEL_HEADER_GET_TID(header));
        error = OT_ERROR_DROP;
    }

exit:
    LogIfFail(mInstance, "Error processing response", error);
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
    LogIfFail(mInstance, "Error processing result", mError);
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
            otLogInfoPlat(mInstance, "NCP reset: %s", spinel_status_to_cstr(status));
            mIsReady = true;
        }
        else
        {
            otLogInfoPlat(mInstance, "NCP last status: %s", spinel_status_to_cstr(status));
        }
    }
    else if (aKey == SPINEL_PROP_MAC_ENERGY_SCAN_RESULT)
    {
        uint8_t        scanChannel;
        int8_t         maxRssi;
        spinel_ssize_t unpacked;

        unpacked = spinel_datatype_unpack(aBuffer, aLength, "Cc", &scanChannel, &maxRssi);

        VerifyOrExit(unpacked > 0, error = OT_ERROR_PARSE);
#if !OPENTHREAD_ENABLE_DIAG
        otPlatRadioEnergyScanDone(aInstance, maxRssi);
#endif
    }
    else if (aKey == SPINEL_PROP_STREAM_DEBUG)
    {
        const char *   message = NULL;
        spinel_ssize_t unpacked;

        unpacked = spinel_datatype_unpack(aBuffer, aLength, SPINEL_DATATYPE_UTF8_S, &message);
        VerifyOrExit(unpacked > 0 && message, error = OT_ERROR_PARSE);
        otLogDebgPlat(mInstance, "NCP DEBUG INFO: %s", message);
    }

exit:
    LogIfFail(mInstance, "Failed to handle ValueIs", error);
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
    LogIfFail(mInstance, "Handle radio frame failed", error);
    return error;
}

void RadioSpinel::ReadAll(void)
{
    uint8_t buf[kMaxSpinelFrame];
    ssize_t rval = read(mSockFd, buf, sizeof(buf));

    if (rval < 0)
    {
        perror("read spinel");
        if (errno != EAGAIN)
        {
            abort();
        }
    }

    if (rval > 0)
    {
        mHdlcDecoder.Decode(buf, static_cast<uint16_t>(rval));
    }
}

void RadioSpinel::ProcessFrameQueue(void)
{
    uint8_t        length;
    uint8_t        buffer[kMaxSpinelFrame];
    const uint8_t *frame;

    while ((frame = mFrameQueue.Peek(buffer, length)) != NULL)
    {
        HandleNotification(frame, length);
        mFrameQueue.Shift();
    }
}

void RadioSpinel::RadioReceive(void)
{
    otError        error = OT_ERROR_NONE;
    otPanId        dstpan;
    otShortAddress shortAddress;
    otExtAddress   extAddress;

    VerifyOrExit(mPromiscuous == false, error = OT_ERROR_NONE);
    VerifyOrExit((mState == OT_RADIO_STATE_RECEIVE || mState == OT_RADIO_STATE_TRANSMIT), error = OT_ERROR_DROP);

    switch (mRxRadioFrame.mPsdu[1] & IEEE802154_DST_ADDR_MASK)
    {
    case IEEE802154_DST_ADDR_NONE:
        break;

    case IEEE802154_DST_ADDR_SHORT:
        dstpan       = getDstPan(mRxRadioFrame.mPsdu);
        shortAddress = getShortAddress(mRxRadioFrame.mPsdu);
        VerifyOrExit((dstpan == IEEE802154_BROADCAST || dstpan == mPanid) &&
                         (shortAddress == IEEE802154_BROADCAST || shortAddress == mShortAddress),
                     error = OT_ERROR_ABORT);
        break;

    case IEEE802154_DST_ADDR_EXT:
        dstpan = getDstPan(mRxRadioFrame.mPsdu);
        getExtAddress(mRxRadioFrame.mPsdu, &extAddress);
        VerifyOrExit((dstpan == IEEE802154_BROADCAST || dstpan == mPanid) &&
                         memcmp(&extAddress, mExtendedAddress.m8, sizeof(extAddress)) == 0,
                     error = OT_ERROR_ABORT);
        break;

    default:
        error = OT_ERROR_ABORT;
        goto exit;
    }

exit:

#if OPENTHREAD_ENABLE_DIAG

    if (otPlatDiagModeGet())
    {
        otPlatDiagRadioReceiveDone(mInstance, error == OT_ERROR_NONE ? &mRxRadioFrame : NULL, error);
    }
    else
#endif
    {
        otPlatRadioReceiveDone(mInstance, error == OT_ERROR_NONE ? &mRxRadioFrame : NULL, error);
    }
}

void RadioSpinel::UpdateFdSet(fd_set &aReadFdSet, fd_set &aWriteFdSet, int &aMaxFd, struct timeval &aTimeout)
{
    if ((mState != OT_RADIO_STATE_TRANSMIT || mTxState == kSent))
    {
        FD_SET(mSockFd, &aReadFdSet);

        if (aMaxFd < mSockFd)
        {
            aMaxFd = mSockFd;
        }
    }

    if (mState == OT_RADIO_STATE_TRANSMIT && mTxState == kIdle)
    {
        FD_SET(mSockFd, &aWriteFdSet);

        if (aMaxFd < mSockFd)
        {
            aMaxFd = mSockFd;
        }
    }

    if (!mFrameQueue.IsEmpty())
    {
        aTimeout.tv_sec  = 0;
        aTimeout.tv_usec = 0;
    }
}

void RadioSpinel::Process(const fd_set &aReadFdSet, const fd_set &aWriteFdSet)
{
    if (FD_ISSET(mSockFd, &aReadFdSet) || !mFrameQueue.IsEmpty())
    {
        ProcessFrameQueue();

        if (FD_ISSET(mSockFd, &aReadFdSet))
        {
            ReadAll();
            ProcessFrameQueue();
        }

        if (mState == OT_RADIO_STATE_TRANSMIT && mTxState == kDone)
        {
            mState = OT_RADIO_STATE_RECEIVE;
            otPlatRadioTxDone(mInstance, mTransmitFrame, (mAckWait ? &mRxRadioFrame : NULL), mTxError);
        }
    }

    if (FD_ISSET(mSockFd, &aWriteFdSet))
    {
        if (mState == OT_RADIO_STATE_TRANSMIT && mTxState == kIdle)
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
    mPromiscuous = aEnable;

exit:
    return error;
}

otError RadioSpinel::SetShortAddress(uint16_t aAddress)
{
    otError error;

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
    otError error;
    SuccessOrExit(error = Set(SPINEL_PROP_MAC_15_4_PANID, SPINEL_DATATYPE_UINT16_S, aPanId));
    mPanid = aPanId;

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
    otError error = Get(SPINEL_PROP_PHY_TX_POWER, SPINEL_DATATYPE_INT8_S, aPower);

    LogIfFail(mInstance, "Get transmit power failed", error);
    return error;
}

otError RadioSpinel::SetTransmitPower(int8_t aPower)
{
    otError error = Set(SPINEL_PROP_PHY_TX_POWER, SPINEL_DATATYPE_INT8_S, aPower);
    LogIfFail(mInstance, "Set transmit power failed", error);
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
    otError        error = OT_ERROR_NONE;
    struct timeval end;
    struct timeval now;
    struct timeval timeout = {kMaxWaitTime / 1000, (kMaxWaitTime % 1000) * 1000};

    gettimeofday(&now, NULL);
    timeradd(&now, &timeout, &end);

    do
    {
        fd_set read_fds;
        fd_set error_fds;
        int    rval;

        FD_ZERO(&read_fds);
        FD_ZERO(&error_fds);
        FD_SET(mSockFd, &read_fds);
        FD_SET(mSockFd, &error_fds);

        rval = select(mSockFd + 1, &read_fds, NULL, &error_fds, &timeout);

        if (rval > 0)
        {
            if (FD_ISSET(mSockFd, &read_fds))
            {
                ReadAll();
            }
            else if (FD_ISSET(mSockFd, &error_fds))
            {
                exit(EXIT_FAILURE);
            }
            else
            {
                assert(false);
                exit(EXIT_FAILURE);
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
            exit(EXIT_FAILURE);
        }

        gettimeofday(&now, NULL);
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

    error = mError;

exit:
    LogIfFail(mInstance, "Error waiting response", error);
    // This indicates end of waiting repsonse.
    mWaitingKey = SPINEL_PROP_LAST_STATUS;
    return error;
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
 * This method delievers the radio frame to transceiver.
 *
 * otPlatRadioTxStarted() is triggered immediately for now, which may be earlier than real started time.
 *
 */
void RadioSpinel::RadioTransmit(void)
{
    otError error;

    assert(mTransmitFrame != NULL);
    otPlatRadioTxStarted(mInstance, mTransmitFrame);
    assert(mTxState == kIdle);

    mAckWait = isAckRequested(mTransmitFrame->mPsdu);
    error    = Request(true, SPINEL_CMD_PROP_VALUE_SET, SPINEL_PROP_STREAM_RAW,
                    SPINEL_DATATYPE_DATA_WLEN_S SPINEL_DATATYPE_UINT8_S SPINEL_DATATYPE_INT8_S, mTransmitFrame->mPsdu,
                    mTransmitFrame->mLength, mTransmitFrame->mChannel, mTransmitFrame->mInfo.mRxInfo.mRssi);

    if (error)
    {
        mState = OT_RADIO_STATE_RECEIVE;

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

        mTxState = kIdle;
    }
    else
    {
        mTxState = kSent;
    }
}

otError RadioSpinel::WriteAll(const uint8_t *aBuffer, uint16_t aLength)
{
    otError error = OT_ERROR_NONE;

    while (aLength)
    {
        int rval = write(mSockFd, aBuffer, aLength);

        if (rval > 0)
        {
            aLength -= static_cast<uint16_t>(rval);
            aBuffer += static_cast<uint16_t>(rval);
        }
        else if (rval < 0)
        {
            perror("send command failed");
            ExitNow(error = OT_ERROR_FAILED);
        }
        else
        {
            ExitNow(error = OT_ERROR_FAILED);
        }
    }

exit:
    return error;
}

otError RadioSpinel::SendReset(void)
{
    otError        error = OT_ERROR_NONE;
    uint8_t        buffer[kMaxSpinelFrame];
    UartTxBuffer   txBuffer;
    spinel_ssize_t packed;

    // Pack the header, command and key
    packed =
        spinel_datatype_pack(buffer, sizeof(buffer), "Ci", SPINEL_HEADER_FLAG | SPINEL_HEADER_IID_0, SPINEL_CMD_RESET);

    VerifyOrExit(packed > 0 && static_cast<size_t>(packed) <= sizeof(buffer), error = OT_ERROR_NO_BUFS);

    mHdlcEncoder.Init(txBuffer);
    for (spinel_ssize_t i = 0; i < packed; i++)
    {
        error = mHdlcEncoder.Encode(buffer[i], txBuffer);
        VerifyOrExit(error == OT_ERROR_NONE);
    }
    mHdlcEncoder.Finalize(txBuffer);

    error = WriteAll(txBuffer.GetBuffer(), txBuffer.GetLength());
    VerifyOrExit(error == OT_ERROR_NONE);

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
    UartTxBuffer   txBuffer;
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

    mHdlcEncoder.Init(txBuffer);
    for (uint8_t i = 0; i < offset; i++)
    {
        error = mHdlcEncoder.Encode(buffer[i], txBuffer);
        VerifyOrExit(error == OT_ERROR_NONE);
    }
    mHdlcEncoder.Finalize(txBuffer);

    error = WriteAll(txBuffer.GetBuffer(), txBuffer.GetLength());

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

        if (mAckWait)
        {
            VerifyOrExit(aLength > 0, error = OT_ERROR_FAILED);
            SuccessOrExit(error = ParseRadioFrame(mRxRadioFrame, aBuffer, aLength));
        }
    }
    else
    {
        otLogWarnPlat(mInstance, "Spinel status: %d.", status);
        error = SpinelStatusToOtError(status);
    }

exit:
    mTxState = kDone;
    mTxError = error;
    LogIfFail(mInstance, "Handle transmit done failed", error);
}

otError RadioSpinel::Transmit(otRadioFrame &aFrame)
{
    otError error = OT_ERROR_INVALID_STATE;

    VerifyOrExit(mState == OT_RADIO_STATE_RECEIVE);
    mState         = OT_RADIO_STATE_TRANSMIT;
    error          = OT_ERROR_NONE;
    mTransmitFrame = &aFrame;

exit:
    return error;
}

otError RadioSpinel::Receive(uint8_t aChannel)
{
    otError error = OT_ERROR_NONE;

    VerifyOrExit(mState != OT_RADIO_STATE_DISABLED, error = OT_ERROR_INVALID_STATE);

    if (mChannel != aChannel)
    {
        error = Set(SPINEL_PROP_PHY_CHAN, SPINEL_DATATYPE_UINT8_S, aChannel);
        VerifyOrExit(error == OT_ERROR_NONE);
        mChannel = aChannel;
    }

    if (mState == OT_RADIO_STATE_SLEEP)
    {
        error = Set(SPINEL_PROP_MAC_RAW_STREAM_ENABLED, SPINEL_DATATYPE_BOOL_S, true);
        VerifyOrExit(error == OT_ERROR_NONE);
    }

    mTxState = kIdle;
    mState   = OT_RADIO_STATE_RECEIVE;

exit:
    assert(error == OT_ERROR_NONE);
    return error;
}

otError RadioSpinel::Sleep(void)
{
    otError error = OT_ERROR_NONE;

    switch (mState)
    {
    case OT_RADIO_STATE_RECEIVE:
        error = sRadioSpinel.Set(SPINEL_PROP_MAC_RAW_STREAM_ENABLED, SPINEL_DATATYPE_BOOL_S, false);
        VerifyOrExit(error == OT_ERROR_NONE);

        mState = OT_RADIO_STATE_SLEEP;
        break;

    case OT_RADIO_STATE_SLEEP:
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

        error = Set(SPINEL_PROP_PHY_ENABLED, SPINEL_DATATYPE_BOOL_S, true);
        VerifyOrExit(error == OT_ERROR_NONE);

        error = Get(SPINEL_PROP_PHY_RX_SENSITIVITY, SPINEL_DATATYPE_INT8_S, &mRxSensitivity);
        VerifyOrExit(error == OT_ERROR_NONE);

        mState = OT_RADIO_STATE_SLEEP;
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

        mState = OT_RADIO_STATE_DISABLED;
    }

exit:
    return error;
}

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
    sRadioSpinel.SetShortAddress(aAddress);
    OT_UNUSED_VARIABLE(aInstance);
}

void otPlatRadioSetPromiscuous(otInstance *aInstance, bool aEnable)
{
    sRadioSpinel.SetPromiscuous(aEnable);
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
    return 0;
}

otRadioCaps otPlatRadioGetCaps(otInstance *aInstance)
{
    OT_UNUSED_VARIABLE(aInstance);
    return static_cast<otRadioCaps>(OT_RADIO_CAPS_ACK_TIMEOUT | OT_RADIO_CAPS_TRANSMIT_RETRIES |
                                    OT_RADIO_CAPS_CSMA_BACKOFF);
}

bool otPlatRadioGetPromiscuous(otInstance *aInstance)
{
    OT_UNUSED_VARIABLE(aInstance);
    return sRadioSpinel.GetPromiscuous();
}

void platformRadioUpdateFdSet(fd_set *aReadFdSet, fd_set *aWriteFdSet, int *aMaxFd, struct timeval *aTimeout)
{
    sRadioSpinel.UpdateFdSet(*aReadFdSet, *aWriteFdSet, *aMaxFd, *aTimeout);
}

void platformRadioProcess(otInstance *aInstance, fd_set *aReadFdSet, fd_set *aWriteFdSet)
{
    sRadioSpinel.Process(*aReadFdSet, *aWriteFdSet);
    OT_UNUSED_VARIABLE(aInstance);
}

void otPlatRadioEnableSrcMatch(otInstance *aInstance, bool aEnable)
{
    SuccessOrDie(sRadioSpinel.EnableSrcMatch(aEnable));
    OT_UNUSED_VARIABLE(aInstance);
}

otError otPlatRadioAddSrcMatchShortEntry(otInstance *aInstance, const uint16_t aShortAddress)
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

otError otPlatRadioClearSrcMatchShortEntry(otInstance *aInstance, const uint16_t aShortAddress)
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
    OT_UNUSED_VARIABLE(aScanChannel);
    OT_UNUSED_VARIABLE(aScanDuration);

    return OT_ERROR_NOT_IMPLEMENTED;
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
