/*
 *  Copyright (c) 2022, The OpenThread Authors.
 *  All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions are met:
 *  1. Redistributions of source code must strain the above copyright
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

#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <sys/select.h>
#include <unistd.h>

#include "cli.hpp"

static ot::Fct::Cli sCli;

int main(int argc, char *argv[])
{
    if (argc >= 2)
    {
        const int                     kMaxArgs = 20;
        ot::Utils::CmdLineParser::Arg args[kMaxArgs + 1];

        if (argc - 1 > kMaxArgs)
        {
            fprintf(stderr, "Too many arguments!\r\n");
            exit(EXIT_FAILURE);
        }

        for (int i = 0; i < argc - 1; i++)
        {
            args[i].SetCString(argv[i + 1]);
        }
        args[argc - 1].Clear();

        sCli.ProcessCommand(args);
    }
    else
    {
        fd_set rset;
        int    maxFd;
        int    ret;

        sCli.OutputPrompt();

        while (true)
        {
            FD_ZERO(&rset);
            FD_SET(STDIN_FILENO, &rset);
            maxFd = STDIN_FILENO + 1;

            ret = select(maxFd, &rset, nullptr, nullptr, nullptr);

            if ((ret == -1) && (errno != EINTR))
            {
                fprintf(stderr, "select: %s\n", strerror(errno));
                break;
            }
            else if (ret > 0)
            {
                if (FD_ISSET(STDIN_FILENO, &rset))
                {
                    const int kBufferSize = 512;
                    char      buffer[kBufferSize];

                    if (fgets(buffer, sizeof(buffer), stdin) != nullptr)
                    {
                        sCli.ProcessLine(buffer);
                    }
                    else
                    {
                        break;
                    }
                }
            }
        }
    }

    return EXIT_SUCCESS;
}
