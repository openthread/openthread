/*
 *  Copyright (c) 2021, The OpenThread Authors.
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

#include "openthread-posix-config.h"
#include "platform-posix.h"

#include <fcntl.h>
#include <signal.h>
#include <stdarg.h>
#include <string.h>
#include <sys/file.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

#include <openthread/cli.h>

#include "cli/cli_config.h"
#include "common/code_utils.hpp"

#if OPENTHREAD_POSIX_CONFIG_DAEMON_ENABLE

#define OPENTHREAD_POSIX_DAEMON_SOCKET_LOCK OPENTHREAD_POSIX_CONFIG_DAEMON_SOCKET_BASENAME ".lock"
static_assert(sizeof(OPENTHREAD_POSIX_DAEMON_SOCKET_NAME) < sizeof(sockaddr_un::sun_path),
              "OpenThread daemon socket name too long!");

static int sListenSocket  = -1;
static int sUartLock      = -1;
static int sSessionSocket = -1;

static int OutputFormatV(void *aContext, const char *aFormat, va_list aArguments)
{
    OT_UNUSED_VARIABLE(aContext);

    char buf[OPENTHREAD_CONFIG_CLI_MAX_LINE_LENGTH + 1];
    int  rval;

    buf[OPENTHREAD_CONFIG_CLI_MAX_LINE_LENGTH] = '\0';

    rval = vsnprintf(buf, sizeof(buf) - 1, aFormat, aArguments);

    VerifyOrExit(rval >= 0, otLogWarnPlat("Failed to format CLI output: %s", strerror(errno)));

    VerifyOrExit(sSessionSocket != -1, otLogDebgPlat("%s", buf));

#if defined(__linux__)
    // Don't die on SIGPIPE
    rval = send(sSessionSocket, buf, static_cast<size_t>(rval), MSG_NOSIGNAL);
#else
    rval = write(sSessionSocket, buf, static_cast<size_t>(rval));
#endif

    if (rval < 0)
    {
        otLogWarnPlat("Failed to write CLI output: %s", strerror(errno));
        close(sSessionSocket);
        sSessionSocket = -1;
    }

exit:
    return rval;
}

static void InitializeSessionSocket(void)
{
    int newSessionSocket;
    int rval;

    VerifyOrExit((newSessionSocket = accept(sListenSocket, nullptr, nullptr)) != -1, rval = -1);

    VerifyOrExit((rval = fcntl(newSessionSocket, F_GETFD, 0)) != -1);

    rval |= FD_CLOEXEC;

    VerifyOrExit((rval = fcntl(newSessionSocket, F_SETFD, rval)) != -1);

#ifndef __linux__
    // some platforms (macOS, Solaris) don't have MSG_NOSIGNAL
    // SOME of those (macOS, but NOT Solaris) support SO_NOSIGPIPE
    // if we have SO_NOSIGPIPE, then set it. Otherwise, we're going
    // to simply ignore it.
#if defined(SO_NOSIGPIPE)
    rval = setsockopt(newSessionSocket, SOL_SOCKET, SO_NOSIGPIPE, &rval, sizeof(rval));
    VerifyOrExit(rval != -1);
#else
#warning "no support for MSG_NOSIGNAL or SO_NOSIGPIPE"
#endif
#endif // __linux__

    if (sSessionSocket != -1)
    {
        close(sSessionSocket);
    }
    sSessionSocket = newSessionSocket;

exit:
    if (rval == -1)
    {
        otLogWarnPlat("Failed to initialize session socket: %s", strerror(errno));
        if (newSessionSocket != -1)
        {
            close(newSessionSocket);
        }
    }
    else
    {
        otLogInfoPlat("Session socket is ready", strerror(errno));
    }
}

void platformDaemonEnable(otInstance *aInstance)
{
    struct sockaddr_un sockname;
    int                ret;

    // This allows implementing pseudo reset.
    VerifyOrExit(sListenSocket == -1);

    sListenSocket = SocketWithCloseExec(AF_UNIX, SOCK_STREAM, 0, kSocketNonBlock);

    if (sListenSocket == -1)
    {
        DieNow(OT_EXIT_FAILURE);
    }

    sUartLock = open(OPENTHREAD_POSIX_DAEMON_SOCKET_LOCK, O_CREAT | O_RDONLY | O_CLOEXEC, 0600);

    if (sUartLock == -1)
    {
        DieNowWithMessage("open", OT_EXIT_ERROR_ERRNO);
    }

    if (flock(sUartLock, LOCK_EX | LOCK_NB) == -1)
    {
        DieNowWithMessage("flock", OT_EXIT_ERROR_ERRNO);
    }

    memset(&sockname, 0, sizeof(struct sockaddr_un));

    (void)unlink(OPENTHREAD_POSIX_DAEMON_SOCKET_NAME);

    sockname.sun_family = AF_UNIX;
    strncpy(sockname.sun_path, OPENTHREAD_POSIX_DAEMON_SOCKET_NAME, sizeof(sockname.sun_path) - 1);

    ret = bind(sListenSocket, (const struct sockaddr *)&sockname, sizeof(struct sockaddr_un));

    if (ret == -1)
    {
        DieNowWithMessage("bind", OT_EXIT_ERROR_ERRNO);
    }

    //
    // only accept 1 connection.
    //
    ret = listen(sListenSocket, 1);
    if (ret == -1)
    {
        DieNowWithMessage("listen", OT_EXIT_ERROR_ERRNO);
    }

    otCliInit(aInstance, OutputFormatV, aInstance);

exit:
    return;
}

void platformDaemonDisable(void)
{
    if (sSessionSocket != -1)
    {
        close(sSessionSocket);
        sSessionSocket = -1;
    }

    if (sListenSocket != -1)
    {
        close(sListenSocket);
        sListenSocket = -1;
    }

    if (gPlatResetReason != OT_PLAT_RESET_REASON_SOFTWARE)
    {
        otLogCritPlat("Removing daemon socket: %s", OPENTHREAD_POSIX_DAEMON_SOCKET_NAME);
        (void)unlink(OPENTHREAD_POSIX_DAEMON_SOCKET_NAME);
    }

    if (sUartLock != -1)
    {
        (void)flock(sUartLock, LOCK_UN);
        close(sUartLock);
        sUartLock = -1;
    }
}

void platformDaemonUpdate(otSysMainloopContext *aContext)
{
    if (sListenSocket != -1)
    {
        FD_SET(sListenSocket, &aContext->mReadFdSet);
        FD_SET(sListenSocket, &aContext->mErrorFdSet);

        if (aContext->mMaxFd < sListenSocket)
        {
            aContext->mMaxFd = sListenSocket;
        }
    }

    if (sSessionSocket != -1)
    {
        FD_SET(sSessionSocket, &aContext->mReadFdSet);
        FD_SET(sSessionSocket, &aContext->mErrorFdSet);

        if (aContext->mMaxFd < sSessionSocket)
        {
            aContext->mMaxFd = sSessionSocket;
        }
    }

    return;
}

void platformDaemonProcess(const otSysMainloopContext *aContext)
{
    ssize_t rval;

    VerifyOrExit(sListenSocket != -1);

    if (FD_ISSET(sListenSocket, &aContext->mErrorFdSet))
    {
        DieNowWithMessage("daemon socket error", OT_EXIT_FAILURE);
    }
    else if (FD_ISSET(sListenSocket, &aContext->mReadFdSet))
    {
        InitializeSessionSocket();
    }

    VerifyOrExit(sSessionSocket != -1);

    if (FD_ISSET(sSessionSocket, &aContext->mErrorFdSet))
    {
        close(sSessionSocket);
        sSessionSocket = -1;
    }
    else if (FD_ISSET(sSessionSocket, &aContext->mReadFdSet))
    {
        uint8_t buffer[OPENTHREAD_CONFIG_CLI_MAX_LINE_LENGTH];

        rval = read(sSessionSocket, buffer, sizeof(buffer));

        if (rval > 0)
        {
            buffer[rval] = '\0';
            otCliInputLine(reinterpret_cast<char *>(buffer));
            otCliOutputFormat("> ");
        }
        else
        {
            if (rval < 0)
            {
                otLogWarnPlat("Daemon read: %s", strerror(errno));
            }
            close(sSessionSocket);
            sSessionSocket = -1;
        }
    }

exit:
    return;
}

#endif // OPENTHREAD_POSIX_CONFIG_DAEMON_ENABLE
