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

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

#if HAVE_LIBEDIT
#include <editline/readline.h>
#elif HAVE_LIBREADLINE
#include <getopt.h>
#include <readline/history.h>
#include <readline/readline.h>
#endif

#include "common/code_utils.hpp"

#include "platform-posix.h"

enum
{
    kLineBufferSize = 256,
};

static_assert(kLineBufferSize >= sizeof("> "), "kLineBufferSize is too small");
static_assert(kLineBufferSize >= sizeof("Done\r\n"), "kLineBufferSize is too small");
static_assert(kLineBufferSize >= sizeof("Error "), "kLineBufferSize is too small");

const char kDefaultThreadNetworkInterfaceName[] = "wpan0";

typedef struct ClientConfig
{
    const char *mInterfaceName; ///< Thread network interface name.
    char **     mArgs;
    uint16_t    mArgCount;
} ClientConfig;

/**
 * This enumeration defines the argument return values.
 *
 */
enum
{
    OT_POSIX_OPT_HELP           = 'h',
    OT_POSIX_OPT_INTERFACE_NAME = 'I',
};

static const struct option kClientOptions[] = {{"help", no_argument, NULL, OT_POSIX_OPT_HELP},
                                               {"interface-name", required_argument, NULL, OT_POSIX_OPT_INTERFACE_NAME},
                                               {0, 0, 0, 0}};

static int sSessionFd = -1;

#if OPENTHREAD_USE_READLINE
static void InputCallback(char *aLine)
{
    if (aLine != nullptr)
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
            aSize -= static_cast<size_t>(rval);
        }
    }

exit:
    return ret;
}

static void PrintUsage(const char *aProgramName, FILE *aStream, int aExitCode)
{
    fprintf(aStream,
            "Syntax:\n"
            "    %s [Options]\n"
            "Options:\n"
            "    -h  --help                    Display this usage information.\n"
            "    -I  --interface-name name     Thread network interface name.\n",
            aProgramName);
    exit(aExitCode);
}

static void ParseArg(int aArgCount, char *aArgVector[], ClientConfig *aConfig)
{
    memset(aConfig, 0, sizeof(*aConfig));

    optind = 1;

    while (true)
    {
        int index  = 0;
        int option = getopt_long(aArgCount, aArgVector, "hI:", kClientOptions, &index);

        if (option == -1)
        {
            break;
        }

        switch (option)
        {
        case OT_POSIX_OPT_HELP:
            PrintUsage(aArgVector[0], stdout, OT_EXIT_SUCCESS);
            break;
        case OT_POSIX_OPT_INTERFACE_NAME:
            aConfig->mInterfaceName = optarg;
            break;
        default:
            PrintUsage(aArgVector[0], stderr, 128);
            break;
        }
    }

    if (aConfig->mInterfaceName == nullptr)
    {
        aConfig->mInterfaceName = kDefaultThreadNetworkInterfaceName;
    }

    aConfig->mArgs     = &aArgVector[optind];
    aConfig->mArgCount = aArgCount - optind;
}

int main(int argc, char *argv[])
{
    int          ret;
    bool         isInteractive = true;
    bool         isFinished    = false;
    char         lineBuffer[kLineBufferSize];
    size_t       lineBufferWritePos = 0;
    bool         isBeginOfLine      = true;
    ClientConfig config;

    ParseArg(argc, argv, &config);

    sSessionFd = socket(AF_UNIX, SOCK_STREAM, 0);
    VerifyOrExit(sSessionFd != -1, perror("socket"); ret = OT_EXIT_FAILURE);

    {
        struct sockaddr_un sockname;

        memset(&sockname, 0, sizeof(struct sockaddr_un));
        sockname.sun_family = AF_UNIX;
        snprintf(sockname.sun_path, sizeof(sockname.sun_path), "%s%s%s", OPENTHREAD_POSIX_CONFIG_DAEMON_SOCKET_BASENAME,
                 config.mInterfaceName, OPENTHREAD_POSIX_CONFIG_DAEMON_SOCKET_EXTNAME);

        ret = connect(sSessionFd, reinterpret_cast<const struct sockaddr *>(&sockname), sizeof(struct sockaddr_un));

        if (ret == -1)
        {
            fprintf(stderr, "OpenThread daemon is not running.\n");
            ExitNow(ret = OT_EXIT_FAILURE);
        }
    }

    if (config.mArgCount > 0)
    {
        for (int i = 0; i < config.mArgCount; i++)
        {
            VerifyOrExit(DoWrite(sSessionFd, config.mArgs[i], strlen(config.mArgs[i])), ret = OT_EXIT_FAILURE);
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

        ret = select(maxFd + 1, &readFdSet, nullptr, nullptr, nullptr);

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
            VerifyOrExit(fgets(buffer, sizeof(buffer), stdin) != nullptr, ret = OT_EXIT_FAILURE);

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
                for (ssize_t i = 0; i < rval; i++)
                {
                    char c = buffer[i];

                    lineBuffer[lineBufferWritePos++] = c;
                    if (c == '\n' || lineBufferWritePos >= sizeof(lineBuffer) - 1)
                    {
                        char * line = lineBuffer;
                        size_t len  = lineBufferWritePos;

                        // read one line successfully or line buffer is full
                        line[len] = '\0';

                        if (isBeginOfLine && strncmp("> ", lineBuffer, 2) == 0)
                        {
                            line += 2;
                            len -= 2;
                        }

                        VerifyOrExit(DoWrite(STDOUT_FILENO, line, len), ret = OT_EXIT_FAILURE);

                        if (isBeginOfLine && (strncmp("Done\n", line, 5) == 0 || strncmp("Done\r\n", line, 6) == 0 ||
                                              strncmp("Error ", line, 6) == 0))
                        {
                            isFinished = true;
                            ret        = OT_EXIT_SUCCESS;
                            break;
                        }

                        // reset for next line
                        lineBufferWritePos = 0;
                        isBeginOfLine      = c == '\n';
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
