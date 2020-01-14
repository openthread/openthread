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

#include "console_cli.h"

#include "openthread/config.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#ifndef HAVE_LIBEDIT
#define HAVE_LIBEDIT 0
#endif

#ifndef HAVE_LIBREADLINE
#define HAVE_LIBREADLINE 0
#endif

#if HAVE_LIBEDIT || HAVE_LIBREADLINE
#if HAVE_LIBEDIT
#include <editline/readline.h>
#elif HAVE_LIBREADLINE
#include <readline/history.h>
#include <readline/readline.h>
#else
#error "Missing readline library"
#endif

#include <openthread/cli.h>

#include "openthread-core-config.h"
#include "platform-posix.h"

static const char sPrompt[] = "> ";

static int sReadFd;

static void InputCallback(char *aLine)
{
    if (aLine != NULL)
    {
        size_t len;

        if ((len = strlen(aLine)) > 0)
        {
            add_history(aLine);
            otCliConsoleInputLine(aLine, (uint16_t)len);
        }
        free(aLine);
    }
    else
    {
        exit(OT_EXIT_SUCCESS);
    }
}

static int OutputCallback(const char *aBuffer, uint16_t aLength, void *aContext)
{
    (void)aContext;

    return (int)write(STDOUT_FILENO, aBuffer, aLength);
}

void otxConsoleInit(otInstance *aInstance)
{
    rl_instream           = stdin;
    rl_outstream          = stdout;
    rl_inhibit_completion = true;
    sReadFd               = fileno(rl_instream);
    rl_callback_handler_install(sPrompt, InputCallback);
    otCliConsoleInit(aInstance, OutputCallback, NULL);
}

void otxConsoleDeinit(void)
{
    rl_callback_handler_remove();
}

void otxConsoleUpdate(otSysMainloopContext *aMainloop)
{
    FD_SET(sReadFd, &aMainloop->mReadFdSet);
    FD_SET(sReadFd, &aMainloop->mErrorFdSet);

    if (aMainloop->mMaxFd < sReadFd)
    {
        aMainloop->mMaxFd = sReadFd;
    }
}

void otxConsoleProcess(const otSysMainloopContext *aMainloop)
{
    if (FD_ISSET(sReadFd, &aMainloop->mErrorFdSet))
    {
        perror("console error");
        exit(OT_EXIT_FAILURE);
    }

    if (FD_ISSET(sReadFd, &aMainloop->mReadFdSet))
    {
        rl_callback_read_char();
    }
}

#endif // HAVE_LIBEDIT || HAVE_LIBREADLINE
