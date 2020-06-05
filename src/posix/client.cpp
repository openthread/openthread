/*
 *  Copyright (c) 2019, The OpenThread Authors.
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

#include "platform/openthread-posix-config.h"

#include <openthread/platform/toolchain.h>

#ifndef HAVE_LIBEDIT
#define HAVE_LIBEDIT 0
#endif

#ifndef HAVE_LIBREADLINE
#define HAVE_LIBREADLINE 0
#endif

#define OPENTHREAD_USE_READLINE (HAVE_LIBEDIT || HAVE_LIBREADLINE)

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

#if HAVE_LIBEDIT
#include <editline/readline.h>
#elif HAVE_LIBREADLINE
#include <readline/history.h>
#include <readline/readline.h>
#endif

#include "common/code_utils.hpp"

#include "platform-posix.h"

static int sSessionFd = -1;

#if OPENTHREAD_USE_READLINE
static void InputCallback(char *aLine)
{
    if (aLine != NULL)
    {
        add_history(aLine);
        dprintf(sSessionFd, "%s\n", aLine);
        free(aLine);
    }
    else
    {
        exit(OT_EXIT_SUCCESS);
    }
}
#endif // OPENTHREAD_USE_READLINE

enum PromptState
{
    kPromptStateNewline,
    kPromptStateChar1Received,
    kPromptStatePrompted,
    kPromptStateWaitNewline,
};

static bool FindPrompt(PromptState &aState, char aChar)
{
    switch (aChar)
    {
    case '>':
        aState = aState == kPromptStateNewline ? kPromptStateChar1Received : kPromptStateWaitNewline;
        break;
    case ' ':
        aState = aState == kPromptStateChar1Received ? kPromptStatePrompted : kPromptStateWaitNewline;
        break;
    case '\r':
    case '\n':
        aState = kPromptStateNewline;
        break;
    default:
        aState = kPromptStateWaitNewline;
    }

    return aState == kPromptStatePrompted;
}

enum DoneState
{
    kDoneStateNewline,
    kDoneStateD,
    kDoneStateDo,
    kDoneStateDon,
    kDoneStateDone,
    kDoneStateDoneFound,
    kDoneStateWaitNewline,
};

static bool FindDone(DoneState &aDoneState, char aNowCharacter)
{
    switch (aNowCharacter)
    {
    case 'D':
        aDoneState = aDoneState == kDoneStateNewline ? kDoneStateD : kDoneStateWaitNewline;
        break;
    case 'o':
        aDoneState = aDoneState == kDoneStateD ? kDoneStateDo : kDoneStateWaitNewline;
        break;
    case 'n':
        aDoneState = aDoneState == kDoneStateDo ? kDoneStateDon : kDoneStateWaitNewline;
        break;
    case 'e':
        aDoneState = aDoneState == kDoneStateDon ? kDoneStateDone : kDoneStateWaitNewline;
        break;
    case '\r':
    case '\n':
        if (aDoneState == kDoneStateDone)
        {
            aDoneState = kDoneStateDoneFound;
        }
        else
        {
            aDoneState = kDoneStateNewline;
        }
        break;
    default:
        aDoneState = kDoneStateWaitNewline;
        break;
    }

    return aDoneState == kDoneStateDoneFound;
}

enum ErrorState
{
    kErrorStateNewline,
    kErrorStateE,
    kErrorStateEr,
    kErrorStateErr,
    kErrorStateErro,
    kErrorStateError,
    kErrorStateErrorFound,
    kErrorStateWaitNewline,
};

static bool FindError(ErrorState &aErrorState, char aNowCharacter)
{
    switch (aNowCharacter)
    {
    case 'E':
        aErrorState = aErrorState == kErrorStateNewline ? kErrorStateE : kErrorStateWaitNewline;
        break;
    case 'r':
        switch (aErrorState)
        {
        case kErrorStateE:
            aErrorState = kErrorStateEr;
            break;
        case kErrorStateEr:
            aErrorState = kErrorStateErr;
            break;
        case kErrorStateErro:
            aErrorState = kErrorStateError;
            break;
        default:
            aErrorState = kErrorStateWaitNewline;
        }
        break;
    case 'o':
        aErrorState = aErrorState == kErrorStateErr ? kErrorStateErro : kErrorStateWaitNewline;
        break;
    case ' ':
        aErrorState = aErrorState == kErrorStateError ? kErrorStateErrorFound : kErrorStateWaitNewline;
        break;
    case '\r':
    case '\n':
        aErrorState = kErrorStateNewline;
        break;
    default:
        aErrorState = kErrorStateWaitNewline;
        break;
    }

    return aErrorState == kErrorStateErrorFound;
}

static bool DoWrite(int aFile, const void *aBuffer, size_t aSize)
{
    bool ret = true;

    while (aSize)
    {
        ssize_t rval = write(aFile, aBuffer, aSize);
        if (rval <= 0)
        {
            VerifyOrExit((rval == -1) && (errno == EINTR), perror("write"); ret = false);
        }
        else
        {
            aBuffer = reinterpret_cast<const uint8_t *>(aBuffer) + rval;
            aSize -= rval;
        }
    }

exit:
    return ret;
}

int main(int argc, char *argv[])
{
    int         ret;
    bool        isInteractive = true;
    bool        isFinished    = false;
    DoneState   doneState     = kDoneStateNewline;
    ErrorState  errorState    = kErrorStateNewline;
    PromptState promptState   = kPromptStateNewline;

    sSessionFd = socket(AF_UNIX, SOCK_STREAM, 0);
    VerifyOrExit(sSessionFd != -1, perror("socket"); ret = OT_EXIT_FAILURE);

    {
        struct sockaddr_un sockname;

        memset(&sockname, 0, sizeof(struct sockaddr_un));
        sockname.sun_family = AF_UNIX;
        strncpy(sockname.sun_path, OPENTHREAD_POSIX_DAEMON_SOCKET_NAME, sizeof(sockname.sun_path) - 1);

        ret = connect(sSessionFd, reinterpret_cast<const struct sockaddr *>(&sockname), sizeof(struct sockaddr_un));

        if (ret == -1)
        {
            fprintf(stderr, "OpenThread daemon is not running.\n");
            ExitNow(ret = OT_EXIT_FAILURE);
        }
    }

    if (argc > 1)
    {
        for (int i = 1; i < argc; i++)
        {
            VerifyOrExit(DoWrite(sSessionFd, argv[i], strlen(argv[i])), ret = OT_EXIT_FAILURE);
            VerifyOrExit(DoWrite(sSessionFd, " ", 1), ret = OT_EXIT_FAILURE);
        }
        VerifyOrExit(DoWrite(sSessionFd, "\n", 1), ret = OT_EXIT_FAILURE);

        isInteractive = false;
    }
#if OPENTHREAD_USE_READLINE
    else
    {
        rl_instream           = stdin;
        rl_outstream          = stdout;
        rl_inhibit_completion = true;
        rl_callback_handler_install("> ", InputCallback);
        rl_already_prompted = 1;
    }
#endif

    while (!isFinished)
    {
        fd_set readFdSet;
        char   buffer[OPENTHREAD_CONFIG_DIAG_CMD_LINE_BUFFER_SIZE];
        int    maxFd = sSessionFd;

        FD_ZERO(&readFdSet);

        FD_SET(sSessionFd, &readFdSet);

        if (isInteractive)
        {
            FD_SET(STDIN_FILENO, &readFdSet);
            if (STDIN_FILENO > maxFd)
            {
                maxFd = STDIN_FILENO;
            }
        }

        ret = select(maxFd + 1, &readFdSet, NULL, NULL, NULL);

        VerifyOrExit(ret != -1, perror("select"); ret = OT_EXIT_FAILURE);

        if (ret == 0)
        {
            ExitNow(ret = OT_EXIT_SUCCESS);
        }

        if (isInteractive && FD_ISSET(STDIN_FILENO, &readFdSet))
        {
#if OPENTHREAD_USE_READLINE
            rl_callback_read_char();
#else
            VerifyOrExit(fgets(buffer, sizeof(buffer), stdin) != NULL, ret = OT_EXIT_FAILURE);

            VerifyOrExit(DoWrite(sSessionFd, buffer, strlen(buffer)), ret = OT_EXIT_FAILURE);
#endif
        }

        if (FD_ISSET(sSessionFd, &readFdSet))
        {
            ssize_t rval = read(sSessionFd, buffer, sizeof(buffer));
            VerifyOrExit(rval != -1, perror("read"); ret = OT_EXIT_FAILURE);

            if (rval == 0)
            {
                // daemon closed sSessionFd
                ExitNow(ret = isInteractive ? OT_EXIT_FAILURE : OT_EXIT_SUCCESS);
            }

            if (isInteractive)
            {
                VerifyOrExit(DoWrite(STDOUT_FILENO, buffer, static_cast<size_t>(rval)), ret = OT_EXIT_FAILURE);
            }
            else
            {
                size_t lineStart = 0;

                for (ssize_t i = 0; i < rval; i++)
                {
                    if (FindPrompt(promptState, buffer[i]))
                    {
                        doneState  = kDoneStateNewline;
                        errorState = kErrorStateNewline;
                        lineStart  = i + 1;
                        continue;
                    }

                    if (buffer[i] == '\r' || buffer[i] == '\n')
                    {
                        VerifyOrExit(DoWrite(STDOUT_FILENO, buffer + lineStart, i - lineStart + 1),
                                     ret = OT_EXIT_FAILURE);
                        lineStart = i + 1;
                    }

                    if (FindDone(doneState, buffer[i]) || FindError(errorState, buffer[i]))
                    {
                        isFinished = true;
                        ret        = OT_EXIT_SUCCESS;
                    }
                }
            }
        }
    }

exit:
    if (sSessionFd != -1)
    {
#if OPENTHREAD_USE_READLINE
        if (isInteractive)
        {
            rl_callback_handler_remove();
        }
#endif
        close(sSessionFd);
    }

    return ret;
}
