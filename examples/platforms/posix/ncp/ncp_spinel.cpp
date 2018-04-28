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

#include "platform-posix.h"

#include "ncp_spinel.hpp"

#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <pty.h>
#include <stdarg.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <syslog.h>
#include <termios.h>
#include <unistd.h>

#include <common/logging.hpp>
#include <openthread/platform/alarm-milli.h>

#include "utils/code_utils.h"

#if OPENTHREAD_ENABLE_POSIX_RADIO_NCP

static ot::NcpSpinel sNcpSpinel;

extern const char *NODE_FILE;
extern const char *NODE_CONFIG;

namespace ot {

#ifndef SOCKET_UTILS_DEFAULT_SHELL
#define SOCKET_UTILS_DEFAULT_SHELL "/bin/sh"
#endif

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
        otEXPECT(pid >= 0);
    }

    if (0 == pid)
    {
        const int kMaxCommand = 255;
        char      cmd[kMaxCommand];
        int       rval;
        const int dtablesize = getdtablesize();

        rval = setenv("SHELL", SOCKET_UTILS_DEFAULT_SHELL, 0);

        otEXPECT_ACTION(rval == 0, perror("setenv failed"));

        // Close all file descriptors larger than STDERR_FILENO.
        for (int i = (STDERR_FILENO + 1); i < dtablesize; i++)
        {
            close(i);
        }

        rval = snprintf(cmd, sizeof(cmd), "%s %s", aFile, aConfig);
        otEXPECT_ACTION(rval > 0 && static_cast<size_t>(rval) < sizeof(cmd),
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

static int OpenUart(const char *aNcpFile, const char *aNcpConfig)
{
    const int kMaxSttyCommand = 128;
    char      cmd[kMaxSttyCommand];
    int       fd = -1;
    int       rval;

    otEXPECT_ACTION(!strchr(aNcpConfig, '&') && !strchr(aNcpConfig, '|') && !strchr(aNcpConfig, ';'),
                    otLogCritPlat(mInstance, "Illegal NCP config arguments!"));

    rval = snprintf(cmd, sizeof(cmd), "stty -F %s %s", aNcpFile, aNcpConfig);
    otEXPECT_ACTION(rval > 0 && static_cast<size_t>(rval) < sizeof(cmd),
                    otLogCritPlat(mInstance, "NCP file and configuration is too long!"));

    rval = system(cmd);
    otEXPECT_ACTION(rval == 0, otLogCritPlat(mInstance, "Unable to configure serial port"));

    fd = open(aNcpFile, O_RDWR | O_NOCTTY | O_NONBLOCK);
    if (fd == -1)
    {
        perror("open uart failed");
        otEXIT_NOW();
    }

    rval = tcflush(fd, TCIOFLUSH);
    otEXPECT_ACTION(rval == 0, otLogCritPlat(mInstance, "Unable to flush serial port"));

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

NcpSpinel::NcpSpinel(void)
    : mCmdTidsInUse(0)
    , mCmdNextTid(1)
    , mStreamTid(0)
    , mWaitingTid(0)
    , mHdlcDecoder(mHdlcBuffer, sizeof(mHdlcBuffer), HandleHdlcFrame, HandleHdlcError, this)
    , mSockFd(-1)
    , mIsReady(false)
{
}

void NcpSpinel::Init(const char *aNcpFile, const char *aNcpConfig)
{
    otError     error = OT_ERROR_NONE;
    struct stat st;

    // not allowed to initialize again.
    assert(mSockFd == -1);

    otEXPECT_ACTION(stat(aNcpFile, &st) == 0, perror("stat ncp file failed"));

    if (S_ISCHR(st.st_mode))
    {
        mSockFd = OpenUart(aNcpFile, aNcpConfig);
    }
    else if (S_ISREG(st.st_mode))
    {
        mSockFd = OpenPty(aNcpFile, aNcpConfig);
    }

    otEXPECT_ACTION(mSockFd != -1, error = OT_ERROR_INVALID_ARGS);

    error = SendReset();

exit:
    if (error != OT_ERROR_NONE)
    {
        exit(EXIT_FAILURE);
    }
}

void NcpSpinel::Deinit(void)
{
    // this function is only allowed after successfully initialized.
    assert(mSockFd != -1);

    otEXPECT_ACTION(0 == close(mSockFd), perror("close NCP"));
    otEXPECT_ACTION(-1 != wait(NULL), perror("wait NCP"));

exit:
    return;
}

bool NcpSpinel::IsFrameCached(void) const
{
    return !mFrameCache.IsEmpty();
}

void NcpSpinel::HandleHdlcFrame(const uint8_t *aBuffer, uint16_t aLength)
{
    otError        error = OT_ERROR_NONE;
    uint8_t        header;
    spinel_ssize_t rval;

    rval = spinel_datatype_unpack(aBuffer, aLength, "C", &header);

    otEXPECT_ACTION(rval > 0 && (header & SPINEL_HEADER_FLAG) == SPINEL_HEADER_FLAG &&
                        SPINEL_HEADER_GET_IID(header) == 0,
                    error = OT_ERROR_PARSE);

    if (SPINEL_HEADER_GET_TID(header) == 0)
    {
        otEXPECT(OT_ERROR_NONE == mFrameCache.Push(aBuffer, aLength));
    }
    else
    {
        ProcessResponse(aBuffer, aLength);
    }

exit:
    LogIfFail(mInstance, "Error handling hdlc frame", error);
}

void NcpSpinel::ProcessNotification(const uint8_t *aBuffer, uint16_t aLength)
{
    spinel_prop_key_t key;
    spinel_size_t     len = 0;
    spinel_ssize_t    rval;
    uint8_t *         data = NULL;
    uint32_t          cmd;
    uint8_t           header;
    otError           error = OT_ERROR_NONE;

    rval = spinel_datatype_unpack(aBuffer, aLength, "CiiD", &header, &cmd, &key, &data, &len);
    otEXPECT_ACTION(rval > 0, error = OT_ERROR_PARSE);
    otEXPECT_ACTION(SPINEL_HEADER_GET_TID(header) == 0, error = OT_ERROR_PARSE);
    otEXPECT_ACTION(cmd >= SPINEL_CMD_PROP_VALUE_IS && cmd <= SPINEL_CMD_PROP_VALUE_REMOVED, error = OT_ERROR_PARSE);

    if (key == SPINEL_PROP_LAST_STATUS)
    {
        spinel_status_t status = SPINEL_STATUS_OK;

        rval = spinel_datatype_unpack(data, len, "i", &status);
        otEXPECT_ACTION(rval > 0, error = OT_ERROR_PARSE);

        if (status >= SPINEL_STATUS_RESET__BEGIN && status <= SPINEL_STATUS_RESET__END)
        {
            otLogWarnPlat(mInstance, "NCP reset for %d", status - SPINEL_STATUS_RESET__BEGIN);
        }
        else
        {
            otLogInfoPlat(mInstance, "NCP last status %d", status);
        }
    }
    else
    {
        if (cmd == SPINEL_CMD_PROP_VALUE_IS)
        {
            ProcessValueIs(key, data, static_cast<uint16_t>(len));
        }
        else
        {
            otLogInfoPlat(mInstance, "Ignored command %d", cmd);
        }
    }

exit:
    LogIfFail(mInstance, "Error processing notification", error);
}

void NcpSpinel::ProcessResponse(const uint8_t *aBuffer, uint16_t aLength)
{
    spinel_prop_key_t key;
    uint8_t *         data   = NULL;
    spinel_size_t     len    = 0;
    uint8_t           header = 0;
    uint32_t          cmd    = 0;
    spinel_ssize_t    rval   = 0;
    otError           error  = OT_ERROR_NONE;

    rval = spinel_datatype_unpack(aBuffer, aLength, "CiiD", &header, &cmd, &key, &data, &len);
    otEXPECT_ACTION(rval > 0 && cmd >= SPINEL_CMD_PROP_VALUE_IS && cmd <= SPINEL_CMD_PROP_VALUE_REMOVED,
                    error = OT_ERROR_PARSE);

    if (mWaitingTid == SPINEL_HEADER_GET_TID(header))
    {
        HandleResult(cmd, key, data, static_cast<uint16_t>(len));
        FreeTid(mWaitingTid);
        mWaitingTid = 0;
    }
    else if (mStreamTid == SPINEL_HEADER_GET_TID(header))
    {
        HandleTransmitDone(cmd, key, data, static_cast<uint16_t>(len));
        FreeTid(mStreamTid);
        mStreamTid = 0;
    }
    else
    {
        otLogWarnPlat(mInstance, "Unexpected Spinel transaction message: %u", SPINEL_HEADER_GET_TID(header));
        error = OT_ERROR_DROP;
    }

exit:
    LogIfFail(mInstance, "Error processing response", error);
}

void NcpSpinel::HandleResult(uint32_t aCommand, spinel_prop_key_t aKey, const uint8_t *aBuffer, uint16_t aLength)
{
    if (aKey == SPINEL_PROP_LAST_STATUS)
    {
        spinel_status_t status;
        spinel_ssize_t  unpacked = spinel_datatype_unpack(aBuffer, aLength, "i", &status);

        otEXPECT_ACTION(unpacked > 0, mLastError = OT_ERROR_PARSE);
        mLastError = SpinelStatusToOtError(status);
    }
    else if (aKey == mWaitingKey)
    {
        if (mFormat)
        {
            spinel_ssize_t unpacked = spinel_datatype_vunpack_in_place(aBuffer, aLength, mFormat, mArgs);

            otEXPECT_ACTION(unpacked > 0, mLastError = OT_ERROR_PARSE);
            mLastError = OT_ERROR_NONE;
        }
        else
        {
            if (aCommand == mExpectedCommand)
            {
                mLastError = OT_ERROR_NONE;
            }
            else
            {
                mLastError = OT_ERROR_DROP;
            }
        }
    }
    else
    {
        mLastError = OT_ERROR_DROP;
    }

exit:
    LogIfFail(mInstance, "Error processing result", mLastError);
}

void NcpSpinel::ProcessValueIs(spinel_prop_key_t aKey, const uint8_t *aBuffer, uint16_t aLength)
{
    otError error = OT_ERROR_NONE;

    if (aKey == SPINEL_PROP_STREAM_RAW)
    {
        otEXPECT(OT_ERROR_NONE == ParseRawStream(mReceiveFrame, aBuffer, aLength));
        mReceivedHandler(mInstance);
    }
    else if (aKey == SPINEL_PROP_MAC_ENERGY_SCAN_RESULT)
    {
        uint8_t        scanChannel;
        int8_t         maxRssi;
        spinel_ssize_t ret;

        ret = spinel_datatype_unpack(aBuffer, aLength, "Cc", &scanChannel, &maxRssi);

        otEXPECT_ACTION(ret > 0, error = OT_ERROR_PARSE);
#if !OPENTHREAD_ENABLE_DIAG
        otPlatRadioEnergyScanDone(aInstance, maxRssi);
#endif
    }
    else if (aKey == SPINEL_PROP_STREAM_DEBUG)
    {
        const char *   message = NULL;
        uint32_t       length  = 0;
        spinel_ssize_t rval;

        rval = spinel_datatype_unpack(aBuffer, aLength, SPINEL_DATATYPE_DATA_S, &message, &length);
        otEXPECT_ACTION(rval > 0 && message && length <= static_cast<uint32_t>(rval), error = OT_ERROR_PARSE);
        otEXPECT_ACTION(strnlen(message, length) != length, error = OT_ERROR_PARSE);
        otLogDebgPlat(mInstance, "NCP DEBUG INFO: %s", message);
    }

exit:
    LogIfFail(mInstance, "Failed to handle ValueIs", error);
}

otError NcpSpinel::ParseRawStream(otRadioFrame *aFrame, const uint8_t *aBuffer, uint16_t aLength)
{
    otError        error        = OT_ERROR_NONE;
    uint16_t       packetLength = 0;
    spinel_ssize_t unpacked;
    uint16_t       flags      = 0;
    int8_t         noiseFloor = -128;
    spinel_size_t  size       = OT_RADIO_FRAME_MAX_SIZE;

    unpacked = spinel_datatype_unpack(aBuffer, aLength, SPINEL_DATATYPE_UINT16_S, &packetLength);
    otEXPECT_ACTION(unpacked > 0 && packetLength <= OT_RADIO_FRAME_MAX_SIZE, error = OT_ERROR_PARSE);

    aFrame->mLength = static_cast<uint8_t>(packetLength);

    unpacked = spinel_datatype_unpack_in_place(aBuffer, aLength,
                                               SPINEL_DATATYPE_DATA_WLEN_S SPINEL_DATATYPE_INT8_S SPINEL_DATATYPE_INT8_S
                                                   SPINEL_DATATYPE_UINT16_S SPINEL_DATATYPE_STRUCT_S( // PHY-data
                                                       SPINEL_DATATYPE_UINT8_S     // 802.15.4 channel
                                                           SPINEL_DATATYPE_UINT8_S // 802.15.4 LQI
                                                       ),
                                               aFrame->mPsdu, &size, &aFrame->mRssi, &noiseFloor, &flags,
                                               &aFrame->mChannel, &aFrame->mLqi);
    otEXPECT_ACTION(unpacked > 0, error = OT_ERROR_PARSE);

exit:
    return error;
}

void NcpSpinel::Receive(void)
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

void NcpSpinel::ProcessCache(void)
{
    uint8_t        length;
    uint8_t        buffer[kMaxSpinelFrame];
    const uint8_t *frame;

    while ((frame = mFrameCache.Peek(buffer, length)) != NULL)
    {
        ProcessNotification(frame, length);
        mFrameCache.Shift();
    }
}

void NcpSpinel::Process(otRadioFrame *aFrame, bool aRead)
{
    mReceiveFrame = aFrame;
    ProcessCache();

    if (aRead)
    {
        Receive();
        ProcessCache();
    }

    mReceiveFrame = NULL;
}

otError NcpSpinel::Get(spinel_prop_key_t aKey, const char *aFormat, va_list aArgs)
{
    otError error;

    assert(mWaitingTid == 0);

    mFormat = aFormat;
    va_copy(mArgs, aArgs);
    error   = RequestV(true, SPINEL_CMD_PROP_VALUE_GET, aKey, NULL, aArgs);
    mFormat = NULL;

    return error;
}

otError NcpSpinel::Set(spinel_prop_key_t aKey, const char *aFormat, va_list aArgs)
{
    otError error;

    assert(mWaitingTid == 0);

    mExpectedCommand = SPINEL_CMD_PROP_VALUE_IS;
    error            = RequestV(true, SPINEL_CMD_PROP_VALUE_SET, aKey, aFormat, aArgs);
    mExpectedCommand = SPINEL_CMD_NOOP;

    return error;
}

otError NcpSpinel::Insert(spinel_prop_key_t aKey, const char *aFormat, va_list aArgs)
{
    otError error;

    assert(mWaitingTid == 0);

    mExpectedCommand = SPINEL_CMD_PROP_VALUE_INSERTED;
    error            = RequestV(true, SPINEL_CMD_PROP_VALUE_INSERT, aKey, aFormat, aArgs);
    mExpectedCommand = SPINEL_CMD_NOOP;

    return error;
}

otError NcpSpinel::Remove(spinel_prop_key_t aKey, const char *aFormat, va_list aArgs)
{
    otError error;

    assert(mWaitingTid == 0);

    mExpectedCommand = SPINEL_CMD_PROP_VALUE_REMOVED;
    error            = RequestV(true, SPINEL_CMD_PROP_VALUE_REMOVE, aKey, aFormat, aArgs);
    mExpectedCommand = SPINEL_CMD_NOOP;

    return error;
}

otError NcpSpinel::WaitResponse(void)
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
                Receive();
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
            otEXIT_NOW(mLastError = OT_ERROR_RESPONSE_TIMEOUT);
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
            mLastError  = OT_ERROR_RESPONSE_TIMEOUT;
        }
    } while (mWaitingTid);

    error = mLastError;

exit:
    LogIfFail(mInstance, "Error waiting response", error);
    return error;
}

spinel_tid_t NcpSpinel::GetNextTid(void)
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

otError NcpSpinel::Transmit(const otRadioFrame *aFrame, otRadioFrame *aAckFrame)
{
    otError error;

    mAckFrame = aAckFrame;
    error     = Request(true, SPINEL_CMD_PROP_VALUE_SET, SPINEL_PROP_STREAM_RAW,
                    SPINEL_DATATYPE_DATA_WLEN_S SPINEL_DATATYPE_UINT8_S SPINEL_DATATYPE_INT8_S, aFrame->mPsdu,
                    aFrame->mLength, aFrame->mChannel, aFrame->mRssi);

    return error;
}

otError NcpSpinel::SendAll(const uint8_t *aBuffer, uint16_t aLength)
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
            otEXIT_NOW(error = OT_ERROR_FAILED);
        }
        else
        {
            otEXIT_NOW(error = OT_ERROR_FAILED);
        }
    }

exit:
    return error;
}

otError NcpSpinel::SendReset(void)
{
    otError        error = OT_ERROR_NONE;
    uint8_t        buffer[kMaxSpinelFrame];
    UartTxBuffer   txBuffer;
    spinel_ssize_t packed;

    // Pack the header, command and key
    packed =
        spinel_datatype_pack(buffer, sizeof(buffer), "Ci", SPINEL_HEADER_FLAG | SPINEL_HEADER_IID_0, SPINEL_CMD_RESET);

    otEXPECT_ACTION(packed > 0 && static_cast<size_t>(packed) <= sizeof(buffer), error = OT_ERROR_NO_BUFS);

    mHdlcEncoder.Init(txBuffer);
    for (spinel_ssize_t i = 0; i < packed; i++)
    {
        error = mHdlcEncoder.Encode(buffer[i], txBuffer);
        otEXPECT(error == OT_ERROR_NONE);
    }
    mHdlcEncoder.Finalize(txBuffer);

    error = SendAll(txBuffer.GetBuffer(), txBuffer.GetLength());
    otEXPECT(error == OT_ERROR_NONE);

    sleep(1);

exit:
    return error;
}

otError NcpSpinel::SendCommand(uint32_t          aCommand,
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

    otEXPECT_ACTION(packed > 0 && static_cast<size_t>(packed) <= sizeof(buffer), error = OT_ERROR_NO_BUFS);

    offset = static_cast<uint16_t>(packed);

    // Pack the data (if any)
    if (aFormat)
    {
        packed = spinel_datatype_vpack(buffer + offset, sizeof(buffer) - offset, aFormat, args);
        otEXPECT_ACTION(packed > 0 && static_cast<size_t>(packed + offset) <= sizeof(buffer), error = OT_ERROR_NO_BUFS);

        offset += static_cast<uint16_t>(packed);
    }

    mHdlcEncoder.Init(txBuffer);
    for (uint8_t i = 0; i < offset; i++)
    {
        error = mHdlcEncoder.Encode(buffer[i], txBuffer);
        otEXPECT(error == OT_ERROR_NONE);
    }
    mHdlcEncoder.Finalize(txBuffer);

    error = SendAll(txBuffer.GetBuffer(), txBuffer.GetLength());

exit:
    return error;
}

otError NcpSpinel::RequestV(bool aWait, uint32_t command, spinel_prop_key_t aKey, const char *aFormat, va_list aArgs)
{
    otError      error = OT_ERROR_NONE;
    spinel_tid_t tid   = (aWait ? GetNextTid() : 0);

    otEXPECT_ACTION(!aWait || tid > 0, error = OT_ERROR_BUSY);

    error = SendCommand(command, aKey, tid, aFormat, aArgs);
    otEXPECT(error == OT_ERROR_NONE);

    if (aKey == SPINEL_PROP_STREAM_RAW)
    {
        // not allowed to send another frame before the last frame is done.
        assert(mStreamTid == 0);
        otEXPECT_ACTION(mStreamTid == 0, error = OT_ERROR_BUSY);
        mStreamTid = tid;
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

otError NcpSpinel::Request(bool aWait, uint32_t aCommand, spinel_prop_key_t aKey, const char *aFormat, ...)
{
    va_list args;
    va_start(args, aFormat);
    otError status = RequestV(aWait, aCommand, aKey, aFormat, args);
    va_end(args);
    return status;
}

void NcpSpinel::HandleTransmitDone(uint32_t aCommand, spinel_prop_key_t aKey, const uint8_t *aBuffer, uint16_t aLength)
{
    otError         error  = OT_ERROR_NONE;
    spinel_status_t status = SPINEL_STATUS_OK;
    spinel_ssize_t  unpacked;

    otEXPECT_ACTION(aCommand == SPINEL_CMD_PROP_VALUE_IS && aKey == SPINEL_PROP_LAST_STATUS, error = OT_ERROR_FAILED);

    unpacked = spinel_datatype_unpack(aBuffer, aLength, SPINEL_DATATYPE_UINT_PACKED_S, &status);
    otEXPECT_ACTION(unpacked > 0, error = OT_ERROR_PARSE);

    aBuffer += unpacked;
    aLength -= static_cast<uint16_t>(unpacked);

    if (status == SPINEL_STATUS_OK)
    {
        bool framePending = false;
        unpacked          = spinel_datatype_unpack(aBuffer, aLength, SPINEL_DATATYPE_BOOL_S, &framePending);
        OT_UNUSED_VARIABLE(framePending);
        otEXPECT_ACTION(unpacked > 0, error = OT_ERROR_PARSE);

        aBuffer += unpacked;
        aLength -= static_cast<spinel_size_t>(unpacked);

        if (mAckFrame)
        {
            otEXPECT_ACTION(aLength > 0, error = OT_ERROR_FAILED);
            ParseRawStream(mAckFrame, aBuffer, aLength);
        }
    }
    else
    {
        otLogWarnPlat(mInstance, "Spinel status: %d.", status);
        error = SpinelStatusToOtError(status);
    }

exit:
    LogIfFail(mInstance, "Handle transmit done failed", error);
    mTransmittedHandler(mInstance, error);
}

} // namespace ot

extern "C" {

int ncpOpen(void)
{
    sNcpSpinel.Init(NODE_FILE, NODE_CONFIG);

    if (OT_ERROR_NONE != ncpGet(SPINEL_PROP_HWADDR, SPINEL_DATATYPE_UINT64_S, &NODE_ID))
    {
        exit(EXIT_FAILURE);
    }

    return sNcpSpinel.GetFd();
}

void ncpClose(void)
{
    sNcpSpinel.Deinit();
}

void ncpProcess(otRadioFrame *aFrame, bool aRead)
{
    sNcpSpinel.Process(aFrame, aRead);
}

otError ncpSet(spinel_prop_key_t aKey, const char *aFormat, ...)
{
    va_list args;
    otError error;

    va_start(args, aFormat);
    error = sNcpSpinel.Set(aKey, aFormat, args);
    va_end(args);
    return error;
}

otError ncpInsert(spinel_prop_key_t aKey, const char *aFormat, ...)
{
    va_list args;
    otError error;

    va_start(args, aFormat);
    error = sNcpSpinel.Insert(aKey, aFormat, args);
    va_end(args);
    return error;
}

otError ncpRemove(spinel_prop_key_t aKey, const char *aFormat, ...)
{
    va_list args;
    otError error;

    va_start(args, aFormat);
    error = sNcpSpinel.Remove(aKey, aFormat, args);
    va_end(args);
    return error;
}

otError ncpGet(spinel_prop_key_t aKey, const char *aFormat, ...)
{
    va_list args;
    otError error;

    va_start(args, aFormat);
    error = sNcpSpinel.Get(aKey, aFormat, args);
    va_end(args);
    return error;
}

otError ncpTransmit(const otRadioFrame *aFrame, otRadioFrame *aAckFrame)
{
    return sNcpSpinel.Transmit(aFrame, aAckFrame);
}

bool ncpIsFrameCached(void)
{
    return sNcpSpinel.IsFrameCached();
}

otError ncpEnable(otInstance *aInstance, ReceivedHandler aReceivedHandler, TransmittedHandler aTransmittedHandler)
{
    sNcpSpinel.Bind(aInstance, aReceivedHandler, aTransmittedHandler);

    otError error = ncpSet(SPINEL_PROP_PHY_ENABLED, SPINEL_DATATYPE_BOOL_S, true);
    otEXPECT(error == OT_ERROR_NONE);

exit:
    return error;
}

otError ncpDisable(void)
{
    sNcpSpinel.Bind(NULL, NULL, NULL);
    return ncpSet(SPINEL_PROP_PHY_ENABLED, SPINEL_DATATYPE_BOOL_S, false);
}
}

#endif // OPENTHREAD_ENABLE_POSIX_RADIO_NCP
