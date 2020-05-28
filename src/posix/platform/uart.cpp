/*
 *  Copyright (c) 2016, The OpenThread Authors.
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

#if OPENTHREAD_POSIX_CONFIG_DAEMON_ENABLE
#include <fcntl.h>
#include <signal.h>
#include <string.h>
#include <sys/file.h>
#include <sys/socket.h>
#include <sys/un.h>
#endif

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <openthread/platform/uart.h>

#include "common/code_utils.hpp"

#define OPENTHREAD_POSIX_DAEMON_SOCKET_LOCK OPENTHREAD_POSIX_CONFIG_DAEMON_SOCKET_BASENAME ".lock"

#if OPENTHREAD_POSIX_CONFIG_DAEMON_ENABLE
static int sUartSocket    = -1;
static int sUartLock      = -1;
static int sSessionSocket = -1;
#endif

static bool           sEnabled     = false;
static const uint8_t *sWriteBuffer = NULL;
static uint16_t       sWriteLength = 0;

otError otPlatUartEnable(void)
{
    otError error = OT_ERROR_NONE;
#if OPENTHREAD_POSIX_CONFIG_DAEMON_ENABLE
    struct sockaddr_un sockname;
    int                ret;

    // This allows implementing pseudo reset.
    VerifyOrExit(sUartSocket == -1, OT_NOOP);

    sUartSocket = SocketWithCloseExec(AF_UNIX, SOCK_STREAM, 0, kSocketNonBlock);

    if (sUartSocket == -1)
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
    assert(sizeof(OPENTHREAD_POSIX_DAEMON_SOCKET_NAME) < sizeof(sockname.sun_path));
    strncpy(sockname.sun_path, OPENTHREAD_POSIX_DAEMON_SOCKET_NAME, sizeof(sockname.sun_path) - 1);

    ret = bind(sUartSocket, (const struct sockaddr *)&sockname, sizeof(struct sockaddr_un));

    if (ret == -1)
    {
        DieNowWithMessage("bind", OT_EXIT_ERROR_ERRNO);
    }

    //
    // only accept 1 connection.
    //
    ret = listen(sUartSocket, 1);
    if (ret == -1)
    {
        DieNowWithMessage("listen", OT_EXIT_ERROR_ERRNO);
    }

exit:
#endif // OPENTHREAD_POSIX_CONFIG_DAEMON_ENABLE

    sEnabled = true;
    return error;
}

otError otPlatUartDisable(void)
{
    otError error = OT_ERROR_NONE;
    sEnabled      = false;

#if OPENTHREAD_POSIX_CONFIG_DAEMON_ENABLE
    if (sSessionSocket != -1)
    {
        close(sSessionSocket);
        sSessionSocket = -1;
    }

    if (sUartSocket != -1)
    {
        close(sUartSocket);
        sUartSocket = -1;
    }

    if (sUartLock != -1)
    {
        (void)flock(sUartLock, LOCK_UN);
        close(sUartLock);
        sUartLock = -1;
    }
#endif // OPENTHREAD_POSIX_CONFIG_DAEMON_ENABLE

    return error;
}

otError otPlatUartSend(const uint8_t *aBuf, uint16_t aBufLength)
{
    otError error = OT_ERROR_NONE;

    assert(sEnabled);
    VerifyOrExit(sWriteLength == 0, error = OT_ERROR_BUSY);

    sWriteBuffer = aBuf;
    sWriteLength = aBufLength;

exit:
    return error;
}

otError otPlatUartFlush(void)
{
    return OT_ERROR_NOT_IMPLEMENTED;
}

void platformUartUpdateFdSet(fd_set *aReadFdSet, fd_set *aWriteFdSet, fd_set *aErrorFdSet, int *aMaxFd)
{
    VerifyOrExit(sEnabled, OT_NOOP);

    if (aReadFdSet != NULL)
    {
#if OPENTHREAD_POSIX_CONFIG_DAEMON_ENABLE
        int fd = (sSessionSocket == -1 ? sUartSocket : sSessionSocket);
#else
        int fd = STDIN_FILENO;
#endif

        FD_SET(fd, aReadFdSet);

        if (aErrorFdSet != NULL)
        {
            FD_SET(fd, aErrorFdSet);
        }

        if (aMaxFd != NULL && *aMaxFd < fd)
        {
            *aMaxFd = fd;
        }
    }
    if ((aWriteFdSet != NULL) && (sWriteLength > 0))
    {
#if OPENTHREAD_POSIX_CONFIG_DAEMON_ENABLE
        int fd = (sSessionSocket == -1 ? sUartSocket : sSessionSocket);
#else
        int fd = STDOUT_FILENO;
#endif

        FD_SET(fd, aWriteFdSet);

        if (aErrorFdSet != NULL)
        {
            FD_SET(fd, aErrorFdSet);
        }

        if (aMaxFd != NULL && *aMaxFd < fd)
        {
            *aMaxFd = fd;
        }
    }

exit:
    return;
}

void platformUartProcess(const fd_set *aReadFdSet, const fd_set *aWriteFdSet, const fd_set *aErrorFdSet)
{
    ssize_t rval;
    int     fd;

    VerifyOrExit(sEnabled, OT_NOOP);
#if OPENTHREAD_POSIX_CONFIG_DAEMON_ENABLE
    if (FD_ISSET(sUartSocket, aErrorFdSet))
    {
        DieNowWithMessage("socket", OT_EXIT_FAILURE);
    }
    else if (FD_ISSET(sUartSocket, aReadFdSet))
    {
        sSessionSocket = accept(sUartSocket, NULL, NULL);
    }

    if (sSessionSocket == -1 && sWriteBuffer != NULL)
    {
        IgnoreReturnValue(write(STDERR_FILENO, sWriteBuffer, sWriteLength));
        sWriteBuffer = NULL;
        sWriteLength = 0;
        otPlatUartSendDone();
    }

    VerifyOrExit(sSessionSocket != -1, OT_NOOP);

    if (FD_ISSET(sSessionSocket, aErrorFdSet))
    {
        close(sSessionSocket);
        sSessionSocket = -1;
    }

    VerifyOrExit(sSessionSocket != -1, OT_NOOP);

    fd = sSessionSocket;
#else  // OPENTHREAD_POSIX_CONFIG_DAEMON_ENABLE
    if (FD_ISSET(STDIN_FILENO, aErrorFdSet))
    {
        DieNowWithMessage("stdin", OT_EXIT_FAILURE);
    }

    if (FD_ISSET(STDOUT_FILENO, aErrorFdSet))
    {
        DieNowWithMessage("stdout", OT_EXIT_FAILURE);
    }

    fd = STDIN_FILENO;
#endif // OPENTHREAD_POSIX_CONFIG_DAEMON_ENABLE

    if (FD_ISSET(fd, aReadFdSet))
    {
        uint8_t buffer[256];

        rval = read(fd, buffer, sizeof(buffer));

        if (rval > 0)
        {
            otPlatUartReceived(buffer, (uint16_t)rval);
        }
        else if (rval <= 0)
        {
#if OPENTHREAD_POSIX_CONFIG_DAEMON_ENABLE
            if (rval < 0)
            {
                perror("UART read");
            }
            close(sSessionSocket);
            sSessionSocket = -1;
            ExitNow();
#else
            DieNowWithMessage("UART read", (rval < 0) ? OT_EXIT_ERROR_ERRNO : OT_EXIT_FAILURE);
#endif
        }
    }

#if !OPENTHREAD_POSIX_CONFIG_DAEMON_ENABLE
    fd = STDOUT_FILENO;
#endif

    if ((sWriteLength > 0) && (FD_ISSET(fd, aWriteFdSet)))
    {
#if OPENTHREAD_POSIX_CONFIG_DAEMON_ENABLE
        // Don't die on SIGPIPE
#if !defined(MSG_NOSIGNAL)
        // some platforms (mac OS, Solaris) don't have MSG_NOSIGNAL
        // SOME of those (mac OS, but NOT Solaris) support SO_NOSIGPIPE
        // if we have SO_NOSIGPIPE, then set it.  otherwise, we're going
        // to simply ignore it.
#if defined(SO_NOSIGPIPE)
        int err;
        int flag;

        flag = 1;
        err  = setsockopt(fd, SOL_SOCKET, SO_NOSIGPIPE, &flag, sizeof(flag));
        VerifyOrDie(err == 0, OT_EXIT_ERROR_ERRNO);
#else
#warning "no support for MSG_NOSIGNAL or SO_NOSIGPIPE"
#endif
#define MSG_NOSIGNAL 0
#endif // !defined(MSG_NOSIGNAL)
        rval = send(fd, sWriteBuffer, sWriteLength, MSG_NOSIGNAL);
#else
        rval = write(fd, sWriteBuffer, sWriteLength);
#endif // OPENTHREAD_POSIX_CONFIG_DAEMON_ENABLE

        if (rval < 0)
        {
#if OPENTHREAD_POSIX_CONFIG_DAEMON_ENABLE
            perror("UART write");
            close(sSessionSocket);
            sSessionSocket = -1;
            ExitNow();
#else
            DieNowWithMessage("UART write", OT_EXIT_ERROR_ERRNO);
#endif
        }

        VerifyOrExit(rval > 0, OT_NOOP);

        sWriteBuffer += (uint16_t)rval;
        sWriteLength -= (uint16_t)rval;

        if (sWriteLength == 0)
        {
            otPlatUartSendDone();
        }
    }

exit:
    return;
}
