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

static bool FindDone(int *aDoneState, char aNowCharacter)
{
    switch (aNowCharacter)
    {
    case 'D':
        *aDoneState = *aDoneState == 0 ? 1 : -1;
        break;
    case 'o':
        *aDoneState = *aDoneState == 1 ? 2 : -1;
        break;
    case 'n':
        *aDoneState = *aDoneState == 2 ? 3 : -1;
        break;
    case 'e':
        *aDoneState = *aDoneState == 3 ? 4 : -1;
        break;
    case '\r':
    case '\n':
        if (*aDoneState == 4)
        {
            *aDoneState = 5;
        }
        else
        {
            *aDoneState = 0;
        }
        break;
    default:
        *aDoneState = -1;
        break;
    }

    return *aDoneState == 5;
}

static bool FindError(int *aErrorState, char aNowCharacter)
{
    switch (aNowCharacter)
    {
    case 'E':
        *aErrorState = *aErrorState == 0 ? 1 : -1;
        break;
    case 'r':
        if (*aErrorState == 1 || *aErrorState == 2 || *aErrorState == 4)
        {
            (*aErrorState)++;
        }
        else
        {
            *aErrorState = -1;
        }
        break;
    case 'o':
        *aErrorState = *aErrorState == 3 ? 4 : -1;
        break;
    case ' ':
        *aErrorState = *aErrorState == 5 ? 6 : -1;
        break;
    case '\r':
    case '\n':
        *aErrorState = 0;
        break;
    default:
        *aErrorState = -1;
        break;
    }

    return *aErrorState == 6;
}

static void SendBlockingCommand(int aArgc, char *aArgv[])
{
    char buffer[OPENTHREAD_CONFIG_DIAG_CMD_LINE_BUFFER_SIZE];
    int  doneState  = 0;
    int  errorState = 0;

    for (int i = 0; i < aArgc; i++)
    {
        VerifyOrExit(write(sSessionFd, aArgv[i], strlen(aArgv[i])) >= 0, perror("Failed to send command"));
        VerifyOrExit(write(sSessionFd, " ", 1) >= 0, perror("Failed to send command"));
    }
    VerifyOrExit(write(sSessionFd, "\n", 1) >= 0, perror("Failed to send command"));

    while (true)
    {
        ssize_t rval = read(sSessionFd, buffer, sizeof(buffer));

        VerifyOrExit(rval >= 0);
        write(STDOUT_FILENO, buffer, static_cast<size_t>(rval));
        for (ssize_t i = 0; i < rval; i++)
        {
            if (FindDone(&doneState, buffer[i]) || FindError(&errorState, buffer[i]))
            {
                ExitNow();
            }
        }
    }
exit:
    return;
}

int main(int argc, char *argv[])
{
    OT_UNUSED_VARIABLE(argc);
    OT_UNUSED_VARIABLE(argv);

    int ret;

    sSessionFd = socket(AF_UNIX, SOCK_STREAM, 0);
    VerifyOrExit(sSessionFd != -1, perror("socket"); ret = OT_EXIT_FAILURE);

    {
        struct sockaddr_un sockname;

        memset(&sockname, 0, sizeof(struct sockaddr_un));
        sockname.sun_family = AF_UNIX;
        strncpy(sockname.sun_path, OPENTHREAD_POSIX_APP_SOCKET_NAME, sizeof(sockname.sun_path) - 1);

        ret = connect(sSessionFd, (const struct sockaddr *)&sockname, sizeof(struct sockaddr_un));

        if (ret == -1)
        {
            fprintf(stderr, "OpenThread daemon is not running.\n");
            ExitNow(ret = OT_EXIT_FAILURE);
        }

#if OPENTHREAD_USE_READLINE
        rl_instream           = stdin;
        rl_outstream          = stdout;
        rl_inhibit_completion = true;
        rl_callback_handler_install("> ", InputCallback);
        rl_already_prompted = 1;
#endif
    }

    if (argc > 1)
    {
        SendBlockingCommand(argc - 1, &argv[1]);
        ExitNow(ret = 0);
    }

    while (1)
    {
        fd_set  readFdSet;
        char    buffer[OPENTHREAD_CONFIG_DIAG_CMD_LINE_BUFFER_SIZE];
        ssize_t rval;
        int     maxFd = sSessionFd > STDIN_FILENO ? sSessionFd : STDIN_FILENO;

        FD_ZERO(&readFdSet);

        FD_SET(STDIN_FILENO, &readFdSet);
        FD_SET(sSessionFd, &readFdSet);

        ret = select(maxFd + 1, &readFdSet, NULL, NULL, NULL);

        VerifyOrExit(ret != -1, perror("select"); ret = OT_EXIT_FAILURE);

        if (ret == 0)
        {
            ExitNow(ret = OT_EXIT_SUCCESS);
        }

        if (FD_ISSET(STDIN_FILENO, &readFdSet))
        {
#if OPENTHREAD_USE_READLINE
            rl_callback_read_char();
#else
            VerifyOrExit(fgets(buffer, sizeof(buffer), stdin) != NULL, ret = OT_EXIT_FAILURE);

            rval = write(sSessionFd, buffer, strlen(buffer));
            VerifyOrExit(rval != -1, perror("write"); ret = OT_EXIT_FAILURE);
#endif
        }

        if (FD_ISSET(sSessionFd, &readFdSet))
        {
            rval = read(sSessionFd, buffer, sizeof(buffer));
            VerifyOrExit(rval != -1, perror("read"); ret = OT_EXIT_FAILURE);

            if (rval == 0)
            {
                // daemon closed sSessionFd
                ExitNow(ret = OT_EXIT_FAILURE);
            }
            else
            {
                IgnoreReturnValue(write(STDOUT_FILENO, buffer, static_cast<size_t>(rval)));
            }
        }
    }

exit:
    if (sSessionFd != -1)
    {
#if OPENTHREAD_USE_READLINE
        rl_callback_handler_remove();
#endif
        close(sSessionFd);
    }

    return ret;
}
