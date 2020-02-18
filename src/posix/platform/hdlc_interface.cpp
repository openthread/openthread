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
 *   This file includes the implementation for the HDLC interface to radio (RCP).
 */

#include "hdlc_interface.hpp"

#include "platform-posix.h"

#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#if OPENTHREAD_CONFIG_POSIX_APP_ENABLE_PTY_DEVICE
#ifdef __APPLE__
#include <util.h>
#else
#include <pty.h>
#endif
#endif
#include <stdarg.h>
#include <stdlib.h>
#include <sys/resource.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <syslog.h>
#include <termios.h>
#include <unistd.h>

#include "common/code_utils.hpp"
#include "common/logging.hpp"

#ifndef SOCKET_UTILS_DEFAULT_SHELL
#define SOCKET_UTILS_DEFAULT_SHELL "/bin/sh"
#endif

#ifdef __APPLE__

#ifndef B230400
#define B230400 230400
#endif

#ifndef B460800
#define B460800 460800
#endif

#ifndef B500000
#define B500000 500000
#endif

#ifndef B576000
#define B576000 576000
#endif

#ifndef B921600
#define B921600 921600
#endif

#ifndef B1000000
#define B1000000 1000000
#endif

#ifndef B1152000
#define B1152000 1152000
#endif

#ifndef B1500000
#define B1500000 1500000
#endif

#ifndef B2000000
#define B2000000 2000000
#endif

#ifndef B2500000
#define B2500000 2500000
#endif

#ifndef B3000000
#define B3000000 3000000
#endif

#ifndef B3500000
#define B3500000 3500000
#endif

#ifndef B4000000
#define B4000000 4000000
#endif

#endif // __APPLE__

#if OPENTHREAD_POSIX_RCP_UART_ENABLE

namespace ot {
namespace PosixApp {

HdlcInterface::HdlcInterface(SpinelInterface::Callbacks &aCallback, SpinelInterface::RxFrameBuffer &aFrameBuffer)
    : mCallbacks(aCallback)
    , mRxFrameBuffer(aFrameBuffer)
    , mSockFd(-1)
    , mHdlcDecoder(aFrameBuffer, HandleHdlcFrame, this)
{
}

otError HdlcInterface::Init(const otPlatformConfig &aPlatformConfig)
{
    otError     error = OT_ERROR_NONE;
    struct stat st;

    VerifyOrExit(mSockFd == -1, error = OT_ERROR_ALREADY);

    VerifyOrDie(stat(aPlatformConfig.mRadioFile, &st) == 0, OT_EXIT_INVALID_ARGUMENTS);

    if (S_ISCHR(st.st_mode))
    {
        mSockFd = OpenFile(aPlatformConfig.mRadioFile, aPlatformConfig.mRadioConfig);
        VerifyOrExit(mSockFd != -1, error = OT_ERROR_INVALID_ARGS);
    }
#if OPENTHREAD_CONFIG_POSIX_APP_ENABLE_PTY_DEVICE
    else if (S_ISREG(st.st_mode))
    {
        mSockFd = ForkPty(aPlatformConfig.mRadioFile, aPlatformConfig.mRadioConfig);
        VerifyOrExit(mSockFd != -1, error = OT_ERROR_INVALID_ARGS);
    }
#endif // OPENTHREAD_CONFIG_POSIX_APP_ENABLE_PTY_DEVICE
    else
    {
        otLogCritPlat("Radio file '%s' not supported", aPlatformConfig.mRadioFile);
        ExitNow(error = OT_ERROR_INVALID_ARGS);
    }

exit:
    return error;
}

HdlcInterface::~HdlcInterface(void)
{
    Deinit();
}

void HdlcInterface::Deinit(void)
{
    VerifyOrExit(mSockFd != -1);

    VerifyOrExit(0 == close(mSockFd), perror("close RCP"));
    VerifyOrExit(-1 != wait(NULL) || errno == ECHILD, perror("wait RCP"));

    mSockFd = -1;

exit:
    return;
}

void HdlcInterface::Read(void)
{
    uint8_t buffer[kMaxFrameSize];
    ssize_t rval;

    rval = read(mSockFd, buffer, sizeof(buffer));

    if (rval > 0)
    {
        Decode(buffer, static_cast<uint16_t>(rval));
    }
    else if ((rval < 0) && (errno != EAGAIN) && (errno != EINTR))
    {
        DieNow(OT_EXIT_ERROR_ERRNO);
    }
}

void HdlcInterface::Decode(const uint8_t *aBuffer, uint16_t aLength)
{
    mHdlcDecoder.Decode(aBuffer, aLength);
}

otError HdlcInterface::SendFrame(const uint8_t *aFrame, uint16_t aLength)
{
    otError                          error = OT_ERROR_NONE;
    Hdlc::FrameBuffer<kMaxFrameSize> encoderBuffer;
    Hdlc::Encoder                    hdlcEncoder(encoderBuffer);

    SuccessOrExit(error = hdlcEncoder.BeginFrame());
    SuccessOrExit(error = hdlcEncoder.Encode(aFrame, aLength));
    SuccessOrExit(error = hdlcEncoder.EndFrame());

    error = Write(encoderBuffer.GetFrame(), encoderBuffer.GetLength());

exit:
    return error;
}

otError HdlcInterface::Write(const uint8_t *aFrame, uint16_t aLength)
{
    otError error = OT_ERROR_NONE;
#if OPENTHREAD_POSIX_VIRTUAL_TIME
    virtualTimeSendRadioSpinelWriteEvent(aFrame, aLength);
#else
    while (aLength)
    {
        ssize_t rval = write(mSockFd, aFrame, aLength);

        if (rval > 0)
        {
            aLength -= static_cast<uint16_t>(rval);
            aFrame += static_cast<uint16_t>(rval);
            continue;
        }

        if ((rval < 0) && (errno != EAGAIN) && (errno != EWOULDBLOCK) && (errno != EINTR))
        {
            DieNow(OT_EXIT_ERROR_ERRNO);
        }

        SuccessOrExit(error = WaitForWritable());
    }

exit:
#endif // OPENTHREAD_POSIX_VIRTUAL_TIME
    return error;
}

otError HdlcInterface::WaitForFrame(const struct timeval &aTimeout)
{
    otError        error   = OT_ERROR_NONE;
    struct timeval timeout = aTimeout;

#if OPENTHREAD_POSIX_VIRTUAL_TIME
    struct Event event;

    virtualTimeSendSleepEvent(&timeout);
    virtualTimeReceiveEvent(&event);

    switch (event.mEvent)
    {
    case OT_SIM_EVENT_RADIO_SPINEL_WRITE:
        Decode(event.mData, event.mDataLength);
        break;

    case OT_SIM_EVENT_ALARM_FIRED:
        ExitNow(error = OT_ERROR_RESPONSE_TIMEOUT);
        break;

    default:
        assert(false);
        break;
    }
#else  // OPENTHREAD_POSIX_VIRTUAL_TIME
    fd_set read_fds;
    fd_set error_fds;
    int rval;

    FD_ZERO(&read_fds);
    FD_ZERO(&error_fds);
    FD_SET(mSockFd, &read_fds);
    FD_SET(mSockFd, &error_fds);

    rval = select(mSockFd + 1, &read_fds, NULL, &error_fds, &timeout);

    if (rval > 0)
    {
        if (FD_ISSET(mSockFd, &read_fds))
        {
            Read();
        }
        else if (FD_ISSET(mSockFd, &error_fds))
        {
            DieNowWithMessage("NCP error", OT_EXIT_FAILURE);
        }
        else
        {
            DieNow(OT_EXIT_FAILURE);
        }
    }
    else if (rval == 0)
    {
        ExitNow(error = OT_ERROR_RESPONSE_TIMEOUT);
    }
    else if (errno != EINTR)
    {
        DieNowWithMessage("wait response", OT_EXIT_FAILURE);
    }
#endif // OPENTHREAD_POSIX_VIRTUAL_TIME

exit:
    return error;
}

void HdlcInterface::UpdateFdSet(fd_set &aReadFdSet, fd_set &aWriteFdSet, int &aMaxFd, struct timeval &aTimeout)
{
    OT_UNUSED_VARIABLE(aWriteFdSet);
    OT_UNUSED_VARIABLE(aTimeout);

    FD_SET(mSockFd, &aReadFdSet);

    if (aMaxFd < mSockFd)
    {
        aMaxFd = mSockFd;
    }
}

void HdlcInterface::Process(const fd_set &aReadFdSet, const fd_set &aWriteFdSet)
{
    OT_UNUSED_VARIABLE(aWriteFdSet);

    if (FD_ISSET(mSockFd, &aReadFdSet))
    {
        Read();
    }
}

otError HdlcInterface::WaitForWritable(void)
{
    otError        error   = OT_ERROR_NONE;
    struct timeval timeout = {kMaxWaitTime / 1000, (kMaxWaitTime % 1000) * 1000};
    uint64_t       now     = platformGetTime();
    uint64_t       end     = now + kMaxWaitTime * US_PER_MS;
    fd_set         writeFds;
    fd_set         errorFds;
    int            rval;

    while (true)
    {
        FD_ZERO(&writeFds);
        FD_ZERO(&errorFds);
        FD_SET(mSockFd, &writeFds);
        FD_SET(mSockFd, &errorFds);

        rval = select(mSockFd + 1, NULL, &writeFds, &errorFds, &timeout);

        if (rval > 0)
        {
            if (FD_ISSET(mSockFd, &writeFds))
            {
                ExitNow();
            }
            else if (FD_ISSET(mSockFd, &errorFds))
            {
                DieNow(OT_EXIT_FAILURE);
            }
            else
            {
                assert(false);
            }
        }
        else if ((rval < 0) && (errno != EINTR))
        {
            DieNow(OT_EXIT_ERROR_ERRNO);
        }

        now = platformGetTime();

        if (end > now)
        {
            uint64_t remain = end - now;

            timeout.tv_sec  = static_cast<time_t>(remain / US_PER_S);
            timeout.tv_usec = static_cast<suseconds_t>(remain % US_PER_S);
        }
        else
        {
            break;
        }
    }

    error = OT_ERROR_FAILED;

exit:
    return error;
}

int HdlcInterface::OpenFile(const char *aFile, const char *aConfig)
{
    int fd   = -1;
    int rval = 0;

    fd = open(aFile, O_RDWR | O_NOCTTY | O_NONBLOCK | O_CLOEXEC);
    if (fd == -1)
    {
        perror("open uart failed");
        ExitNow();
    }

    if (isatty(fd))
    {
        struct termios tios;

        int  speed  = 115200;
        int  cstopb = 1;
        char parity = 'N';
        char flow   = 'N';

        VerifyOrExit((rval = tcgetattr(fd, &tios)) == 0);

        cfmakeraw(&tios);

        tios.c_cflag = CS8 | HUPCL | CREAD | CLOCAL;

        // example: 115200N1H
        if (aConfig != NULL)
        {
            sscanf(aConfig, "%u%c%d%c", &speed, &parity, &cstopb, &flow);
        }

        switch (parity)
        {
        case 'N':
            break;
        case 'E':
            tios.c_cflag |= PARENB;
            break;
        case 'O':
            tios.c_cflag |= (PARENB | PARODD);
            break;
        default:
            // not supported
            DieNow(OT_EXIT_INVALID_ARGUMENTS);
            break;
        }

        switch (cstopb)
        {
        case 1:
            tios.c_cflag &= static_cast<unsigned long>(~CSTOPB);
            break;
        case 2:
            tios.c_cflag |= CSTOPB;
            break;
        default:
            DieNow(OT_EXIT_INVALID_ARGUMENTS);
            break;
        }

        switch (speed)
        {
        case 9600:
            speed = B9600;
            break;
        case 19200:
            speed = B19200;
            break;
        case 38400:
            speed = B38400;
            break;
        case 57600:
            speed = B57600;
            break;
        case 115200:
            speed = B115200;
            break;
#ifdef B230400
        case 230400:
            speed = B230400;
            break;
#endif
#ifdef B460800
        case 460800:
            speed = B460800;
            break;
#endif
#ifdef B500000
        case 500000:
            speed = B500000;
            break;
#endif
#ifdef B576000
        case 576000:
            speed = B576000;
            break;
#endif
#ifdef B921600
        case 921600:
            speed = B921600;
            break;
#endif
#ifdef B1000000
        case 1000000:
            speed = B1000000;
            break;
#endif
#ifdef B1152000
        case 1152000:
            speed = B1152000;
            break;
#endif
#ifdef B1500000
        case 1500000:
            speed = B1500000;
            break;
#endif
#ifdef B2000000
        case 2000000:
            speed = B2000000;
            break;
#endif
#ifdef B2500000
        case 2500000:
            speed = B2500000;
            break;
#endif
#ifdef B3000000
        case 3000000:
            speed = B3000000;
            break;
#endif
#ifdef B3500000
        case 3500000:
            speed = B3500000;
            break;
#endif
#ifdef B4000000
        case 4000000:
            speed = B4000000;
            break;
#endif
        default:
            DieNow(OT_EXIT_INVALID_ARGUMENTS);
            break;
        }

        switch (flow)
        {
        case 'N':
            break;
        case 'H':
            tios.c_cflag |= CRTSCTS;
            break;
        default:
            // not supported
            DieNow(OT_EXIT_INVALID_ARGUMENTS);
            break;
        }

        VerifyOrExit((rval = cfsetspeed(&tios, static_cast<speed_t>(speed))) == 0, perror("cfsetspeed"));
        VerifyOrExit((rval = tcsetattr(fd, TCSANOW, &tios)) == 0, perror("tcsetattr"));
        VerifyOrExit((rval = tcflush(fd, TCIOFLUSH)) == 0);
    }

exit:
    if (rval != 0)
    {
        DieNow(OT_EXIT_FAILURE);
    }

    return fd;
}

#if OPENTHREAD_CONFIG_POSIX_APP_ENABLE_PTY_DEVICE
int HdlcInterface::ForkPty(const char *aCommand, const char *aArguments)
{
    int fd   = -1;
    int pid  = -1;
    int rval = -1;

    {
        struct termios tios;

        memset(&tios, 0, sizeof(tios));
        cfmakeraw(&tios);
        tios.c_cflag = CS8 | HUPCL | CREAD | CLOCAL;

        VerifyOrExit((pid = forkpty(&fd, NULL, &tios, NULL)) != -1, perror("forkpty()"));
    }

    if (0 == pid)
    {
        const int kMaxCommand = 255;
        char      cmd[kMaxCommand];

        rval = snprintf(cmd, sizeof(cmd), "exec %s %s", aCommand, aArguments);
        VerifyOrExit(rval > 0 && static_cast<size_t>(rval) < sizeof(cmd),
                     fprintf(stderr, "NCP file and configuration is too long!");
                     rval = -1);

        VerifyOrExit((rval = execl(SOCKET_UTILS_DEFAULT_SHELL, SOCKET_UTILS_DEFAULT_SHELL, "-c", cmd,
                                   static_cast<char *>(NULL))) != -1,
                     perror("execl(OT_RCP)"));
    }
    else
    {
        VerifyOrExit((rval = fcntl(fd, F_GETFL)) != -1, perror("fcntl(F_GETFL)"));
        VerifyOrExit((rval = fcntl(fd, F_SETFL, rval | O_NONBLOCK | O_CLOEXEC)) != -1, perror("fcntl(F_SETFL)"));
    }

exit:
    VerifyOrDie(rval == 0, OT_EXIT_ERROR_ERRNO);
    return fd;
}
#endif // OPENTHREAD_CONFIG_POSIX_APP_ENABLE_PTY_DEVICE

void HdlcInterface::HandleHdlcFrame(void *aContext, otError aError)
{
    static_cast<HdlcInterface *>(aContext)->HandleHdlcFrame(aError);
}

void HdlcInterface::HandleHdlcFrame(otError aError)
{
    if (aError == OT_ERROR_NONE)
    {
        mCallbacks.HandleReceivedFrame();
    }
    else
    {
        mRxFrameBuffer.DiscardFrame();
        otLogWarnPlat("Error decoding hdlc frame: %s", otThreadErrorToString(aError));
    }
}

} // namespace PosixApp
} // namespace ot
#endif // OPENTHREAD_POSIX_RCP_UART_ENABLE
