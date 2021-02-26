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

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <openthread/ncp.h>
#include <openthread/platform/misc.h>

#include "common/code_utils.hpp"

#if OPENTHREAD_POSIX_APP_TYPE == OT_POSIX_APP_TYPE_NCP
static const uint8_t *sWriteBuffer = nullptr;
static uint16_t       sWriteLength = 0;

static int ncpHdlcSend(const uint8_t *aBuf, uint16_t aBufLength)
{
    sWriteBuffer = aBuf;
    sWriteLength = aBufLength;

    return aBufLength;
}

extern "C" void otAppNcpInit(otInstance *aInstance)
{
    otNcpHdlcInit(aInstance, ncpHdlcSend);
}

extern "C" void otAppNcpUpdate(otSysMainloopContext *aContext)
{
    FD_SET(STDIN_FILENO, &aContext->mReadFdSet);
    FD_SET(STDIN_FILENO, &aContext->mErrorFdSet);

    if (aContext->mMaxFd < STDIN_FILENO)
    {
        aContext->mMaxFd = STDIN_FILENO;
    }

    if (sWriteLength > 0)
    {
        FD_SET(STDOUT_FILENO, &aContext->mWriteFdSet);
        FD_SET(STDOUT_FILENO, &aContext->mErrorFdSet);

        if (aContext->mMaxFd < STDOUT_FILENO)
        {
            aContext->mMaxFd = STDOUT_FILENO;
        }
    }
}

extern "C" void otAppNcpProcess(const otSysMainloopContext *aContext)
{
    ssize_t rval;

    if (FD_ISSET(STDIN_FILENO, &aContext->mErrorFdSet))
    {
        DieNowWithMessage("stdin", OT_EXIT_FAILURE);
    }

    if (FD_ISSET(STDOUT_FILENO, &aContext->mErrorFdSet))
    {
        DieNowWithMessage("stdout", OT_EXIT_FAILURE);
    }

    if (FD_ISSET(STDIN_FILENO, &aContext->mReadFdSet))
    {
        uint8_t buffer[256];

        rval = read(STDIN_FILENO, buffer, sizeof(buffer));

        if (rval > 0)
        {
            otNcpHdlcReceive(buffer, static_cast<uint16_t>(rval));
        }
        else if (rval <= 0)
        {
            DieNowWithMessage("UART read", (rval < 0) ? OT_EXIT_ERROR_ERRNO : OT_EXIT_FAILURE);
        }
    }

    if ((FD_ISSET(STDOUT_FILENO, &aContext->mWriteFdSet)))
    {
        if (sWriteLength > 0)
        {
            rval = write(STDOUT_FILENO, sWriteBuffer, sWriteLength);

            if (rval < 0)
            {
                DieNow(OT_EXIT_ERROR_ERRNO);
            }

            sWriteBuffer += rval;
            sWriteLength -= static_cast<uint16_t>(rval);
        }

        if (sWriteLength == 0)
        {
            otNcpHdlcSendDone();
        }
    }
}
#endif // OPENTHREAD_POSIX_APP_TYPE == OT_POSIX_APP_TYPE_NCP
