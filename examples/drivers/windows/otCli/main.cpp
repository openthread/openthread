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

#include <windows.h>
#include <stdio.h>

#include <openthread/cli.h>
#include <openthread/platform/uart.h>
#include <openthread/platform/misc.h>

bool skipNextLine = false;

int main(int argc, char *argv[])
{
    otCliUartInit(NULL);

    char cmd[1024] = "\n";
    otPlatUartReceived((uint8_t*)cmd, 1);

    for (;;)
    {
        cmd[0] = 0;
        if (NULL == fgets(cmd, sizeof(cmd), stdin))
            continue;

        size_t cmdLen = strlen(cmd);
        if (cmdLen >= sizeof(cmd)) cmdLen = sizeof(cmd);

        if (strncmp(cmd, "exit", 4) == 0) 
            break;

        skipNextLine = true;
        otPlatUartReceived((uint8_t*)cmd, (uint16_t)cmdLen);
    }

    return NO_ERROR;
}

EXTERN_C otError otPlatUartEnable(void)
{
    return OT_ERROR_NONE;
}

EXTERN_C otError otPlatUartSend(const uint8_t *aBuf, uint16_t aBufLength)
{
    otError error = OT_ERROR_NONE;

    if (!skipNextLine)
    {
        for (uint16_t i = 0; i < aBufLength; i++)
            fputc(aBuf[i], stdout);
    }

    if (aBuf[aBufLength - 1] == '\n')
        skipNextLine = false;

    otPlatUartSendDone();

    return error;
}

EXTERN_C void otPlatWakeHost(void)
{

}
