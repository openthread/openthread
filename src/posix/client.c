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

#include "openthread-core-config.h"

#include <openthread/platform/toolchain.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

#include "code_utils.h"
#include "platform-posix.h"

int main(int argc, char *argv[])
{
    OT_UNUSED_VARIABLE(argc);
    OT_UNUSED_VARIABLE(argv);

    int ret;
    int session = -1;

    session = socket(AF_UNIX, SOCK_STREAM, 0);
    otEXPECT_ACTION(session != -1, perror("socket"); ret = OT_EXIT_FAILURE);

    {
        struct sockaddr_un sockname;

        memset(&sockname, 0, sizeof(struct sockaddr_un));
        sockname.sun_family = AF_UNIX;
        strncpy(sockname.sun_path, OPENTHREAD_POSIX_APP_SOCKET_NAME, sizeof(sockname.sun_path) - 1);

        ret = connect(session, (const struct sockaddr *)&sockname, sizeof(struct sockaddr_un));

        if (ret == -1)
        {
            fprintf(stderr, "OpenThread daemon is not running.\n");
            otEXIT_NOW(ret = OT_EXIT_FAILURE);
        }
    }

    while (1)
    {
        char   buffer[OPENTHREAD_CONFIG_DIAG_CMD_LINE_BUFFER_SIZE];
        fd_set readFdSet;
        int    maxFd = STDIN_FILENO;

        FD_ZERO(&readFdSet);

        FD_SET(STDIN_FILENO, &readFdSet);
        FD_SET(session, &readFdSet);

        maxFd = session > maxFd ? session : maxFd;

        ret = select(maxFd + 1, &readFdSet, NULL, NULL, NULL);

        otEXPECT_ACTION(ret != -1, perror("select"); ret = OT_EXIT_FAILURE);

        if (ret == 0)
        {
            otEXIT_NOW(ret = OT_EXIT_SUCCESS);
        }

        if (FD_ISSET(STDIN_FILENO, &readFdSet))
        {
            otEXPECT_ACTION(fgets(buffer, sizeof(buffer), stdin) != NULL, ret = OT_EXIT_FAILURE);

            ret = (int)write(session, buffer, strlen(buffer));
            otEXPECT_ACTION(ret != -1, perror("write"); ret = OT_EXIT_FAILURE);
        }

        if (FD_ISSET(session, &readFdSet))
        {
            ret = (int)read(session, buffer, sizeof(buffer));
            otEXPECT_ACTION(ret != -1, perror("read"); ret = OT_EXIT_FAILURE);

            if (ret == 0)
            {
                // daemon closed session
                otEXIT_NOW(ret = OT_EXIT_FAILURE);
            }
            else
            {
                buffer[ret] = 0;
                printf("%s", buffer);
                fflush(stdout);
            }
        }
    }

exit:
    if (session != -1)
    {
        close(session);
    }

    return ret;
}
