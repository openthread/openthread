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

#include "openthread-core-config.h"
#include "platform-posix.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <openthread/platform/uart.h>

#include "code_utils.h"

static uint8_t        sReceiveBuffer[128];
static const uint8_t *sWriteBuffer;
static uint16_t       sWriteLength;

void platformUartRestore(void)
{
}

otError otPlatUartEnable(void)
{
    return OT_ERROR_NONE;
}

otError otPlatUartDisable(void)
{
    return OT_ERROR_NONE;
}

otError otPlatUartSend(const uint8_t *aBuf, uint16_t aBufLength)
{
    otError error = OT_ERROR_NONE;

    otEXPECT_ACTION(sWriteLength == 0, error = OT_ERROR_BUSY);

    sWriteBuffer = aBuf;
    sWriteLength = aBufLength;

exit:
    return error;
}

void platformUartUpdateFdSet(fd_set *aReadFdSet, fd_set *aWriteFdSet, fd_set *aErrorFdSet, int *aMaxFd)
{
    if (aReadFdSet != NULL)
    {
        FD_SET(STDIN_FILENO, aReadFdSet);

        if (aErrorFdSet != NULL)
        {
            FD_SET(STDIN_FILENO, aErrorFdSet);
        }

        if (aMaxFd != NULL && *aMaxFd < STDIN_FILENO)
        {
            *aMaxFd = STDIN_FILENO;
        }
    }

    if ((aWriteFdSet != NULL) && (sWriteLength > 0))
    {
        FD_SET(STDOUT_FILENO, aWriteFdSet);

        if (aErrorFdSet != NULL)
        {
            FD_SET(STDOUT_FILENO, aErrorFdSet);
        }

        if (aMaxFd != NULL && *aMaxFd < STDOUT_FILENO)
        {
            *aMaxFd = STDOUT_FILENO;
        }
    }
}

void platformUartProcess(const fd_set *aReadFdSet, const fd_set *aWriteFdSet, const fd_set *aErrorFdSet)
{
    ssize_t rval;

    if (FD_ISSET(STDIN_FILENO, aErrorFdSet))
    {
        perror("stdin");
        exit(OT_EXIT_FAILURE);
    }

    if (FD_ISSET(STDOUT_FILENO, aErrorFdSet))
    {
        perror("stdout");
        exit(OT_EXIT_FAILURE);
    }

    if (FD_ISSET(STDIN_FILENO, aReadFdSet))
    {
        rval = read(STDIN_FILENO, sReceiveBuffer, sizeof(sReceiveBuffer));

        if (rval > 0)
        {
            otPlatUartReceived(sReceiveBuffer, (uint16_t)rval);
        }
        else if (rval < 0)
        {
            perror("UART read");
            exit(OT_EXIT_FAILURE);
        }
        else
        {
            fprintf(stderr, "UART ended\r\n");
            exit(OT_EXIT_SUCCESS);
        }
    }

    if ((sWriteLength > 0) && (FD_ISSET(STDOUT_FILENO, aWriteFdSet)))
    {
        rval = write(STDOUT_FILENO, sWriteBuffer, sWriteLength);

        if (rval <= 0)
        {
            perror("UART write");
            exit(OT_EXIT_FAILURE);
        }

        sWriteBuffer += (uint16_t)rval;
        sWriteLength -= (uint16_t)rval;

        if (sWriteLength == 0)
        {
            otPlatUartSendDone();
        }
    }
}
