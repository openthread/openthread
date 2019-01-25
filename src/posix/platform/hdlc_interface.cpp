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

#include "openthread-core-config.h"
#include "platform-posix.h"

#include "hdlc_interface.hpp"

#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#if OPENTHREAD_CONFIG_POSIX_APP_ENABLE_PTY_DEVICE
#ifdef OPENTHREAD_TARGET_DARWIN
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

#include <common/code_utils.hpp>
#include <common/logging.hpp>

#ifndef SOCKET_UTILS_DEFAULT_SHELL
#define SOCKET_UTILS_DEFAULT_SHELL "/bin/sh"
#endif

namespace ot {
namespace PosixApp {

HdlcInterface::HdlcInterface(Callbacks &aCallbacks)
    : mCallbacks(aCallbacks)
    , mSockFd(-1)
    , mIsDecoding(false)
    , mRxFrameBuffer()
    , mHdlcDecoder(mRxFrameBuffer, HandleHdlcFrame, this)
{
}

otError HdlcInterface::Init(const char *aRadioFile, const char *aRadioConfig)
{
    otError     error = OT_ERROR_NONE;
    struct stat st;

    VerifyOrExit(mSockFd == -1, error = OT_ERROR_ALREADY);

    VerifyOrExit(stat(aRadioFile, &st) == 0, perror("stat ncp file failed"); error = OT_ERROR_INVALID_ARGS);

    if (S_ISCHR(st.st_mode))
    {
        mSockFd = OpenFile(aRadioFile, aRadioConfig);
        VerifyOrExit(mSockFd != -1, error = OT_ERROR_INVALID_ARGS);
    }
#if OPENTHREAD_CONFIG_POSIX_APP_ENABLE_PTY_DEVICE
    else if (S_ISREG(st.st_mode))
    {
        mSockFd = ForkPty(aRadioFile, aRadioConfig);
        VerifyOrExit(mSockFd != -1, error = OT_ERROR_INVALID_ARGS);
    }
#endif // OPENTHREAD_CONFIG_POSIX_APP_ENABLE_PTY_DEVICE
    else
    {
        otLogCritPlat("Radio file '%s' not supported", aRadioFile);
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

    VerifyOrExit(0 == close(mSockFd), perror("close NCP"));
    VerifyOrExit(-1 != wait(NULL), perror("wait NCP"));

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
        perror("HdlcInterface::Read()");
        exit(OT_EXIT_FAILURE);
    }
}

void HdlcInterface::Decode(const uint8_t *aBuffer, uint16_t aLength)
{
    mIsDecoding = true;
    mHdlcDecoder.Decode(aBuffer, aLength);
    mIsDecoding = false;
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
    otSimSendRadioSpinelWriteEvent(aFrame, aLength);
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
            perror("HdlcInterface::Write()");
            exit(OT_EXIT_FAILURE);
        }

        SuccessOrExit(error = WaitForWritable());
    }

exit:
#endif // OPENTHREAD_POSIX_VIRTUAL_TIME
    return error;
}

otError HdlcInterface::WaitForWritable(void)
{
    otError        error   = OT_ERROR_NONE;
    struct timeval timeout = {kMaxWaitTime / 1000, (kMaxWaitTime % 1000) * 1000};
    struct timeval end;
    struct timeval now;
    fd_set         writeFds;
    fd_set         errorFds;
    int            rval;

    otSysGetTime(&now);
    timeradd(&now, &timeout, &end);

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
                fprintf(stderr, "HdlcInterface::WaitForWritable(): socket error\n\r");
                exit(OT_EXIT_FAILURE);
            }
            else
            {
                fprintf(stderr, "HdlcInterface::WaitForWritable(): select error\n\r");
                exit(OT_EXIT_FAILURE);
            }
        }
        else if ((rval < 0) && (errno != EINTR))
        {
            perror("HdlcInterface::WaitForWritable()");
            exit(OT_EXIT_FAILURE);
        }

        otSysGetTime(&now);

        if (timercmp(&end, &now, >))
        {
            timersub(&end, &now, &timeout);
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

    fd = open(aFile, O_RDWR | O_NOCTTY | O_NONBLOCK);
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

        VerifyOrExit((rval = tcgetattr(fd, &tios)) == 0);

        cfmakeraw(&tios);

        tios.c_cflag = CS8 | HUPCL | CREAD | CLOCAL;

        // example: 115200N1
        sscanf(aConfig, "%u%c%d", &speed, &parity, &cstopb);

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
            assert(false);
            exit(OT_EXIT_INVALID_ARGUMENTS);
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
            assert(false);
            exit(OT_EXIT_INVALID_ARGUMENTS);
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
            assert(false);
            exit(OT_EXIT_INVALID_ARGUMENTS);
            break;
        }

        VerifyOrExit((rval = cfsetspeed(&tios, static_cast<speed_t>(speed))) == 0, perror("cfsetspeed"));
        VerifyOrExit((rval = tcsetattr(fd, TCSANOW, &tios)) == 0, perror("tcsetattr"));
        VerifyOrExit((rval = tcflush(fd, TCIOFLUSH)) == 0);
    }

exit:
    if (rval != 0)
    {
        exit(OT_EXIT_FAILURE);
    }

    return fd;
}

#if OPENTHREAD_CONFIG_POSIX_APP_ENABLE_PTY_DEVICE
int HdlcInterface::ForkPty(const char *aCommand, const char *aArguments)
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
        const int     kMaxCommand = 255;
        char          cmd[kMaxCommand];
        int           rval;
        struct rlimit limit;

        rval = getrlimit(RLIMIT_NOFILE, &limit);
        rval = setenv("SHELL", SOCKET_UTILS_DEFAULT_SHELL, 0);

        VerifyOrExit(rval == 0, perror("setenv failed"));

        // Close all file descriptors larger than STDERR_FILENO.
        for (rlim_t i = (STDERR_FILENO + 1); i < limit.rlim_cur; i++)
        {
            close(static_cast<int>(i));
        }

        rval = snprintf(cmd, sizeof(cmd), "exec %s %s", aCommand, aArguments);
        VerifyOrExit(rval > 0 && static_cast<size_t>(rval) < sizeof(cmd),
                     otLogCritPlat("NCP file and configuration is too long!"));

        execl(getenv("SHELL"), getenv("SHELL"), "-c", cmd, NULL);
        perror("open pty failed");
        exit(OT_EXIT_INVALID_ARGUMENTS);
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
#endif // OPENTHREAD_CONFIG_POSIX_APP_ENABLE_PTY_DEVICE

void HdlcInterface::HandleHdlcFrame(void *aContext, otError aError)
{
    static_cast<HdlcInterface *>(aContext)->HandleHdlcFrame(aError);
}

void HdlcInterface::HandleHdlcFrame(otError aError)
{
    if (aError == OT_ERROR_NONE)
    {
        mCallbacks.HandleReceivedFrame(*this);
    }
    else
    {
        mRxFrameBuffer.DiscardFrame();
        otLogWarnPlat("Error decoding hdlc frame: %s", otThreadErrorToString(aError));
    }
}

} // namespace PosixApp
} // namespace ot
