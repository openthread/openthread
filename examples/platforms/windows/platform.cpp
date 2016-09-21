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
#include <assert.h>
#include <openthread.h>
#include <platform/alarm.h>
#include <platform/uart.h>
#include "platform-windows.h"

uint32_t NODE_ID = 1;
uint32_t WELLKNOWN_NODE_ID = 34;

EXTERN_C void PlatformInit(int argc, char *argv[])
{
    if (argc != 2)
    {
        exit(1);
    }

    NODE_ID = atoi(argv[1]);

    windowsAlarmInit();
    windowsRadioInit();
    windowsRandomInit();
}

EXTERN_C void PlatformProcessDrivers(otInstance *aInstance)
{
    fd_set read_fds;
    fd_set write_fds;
    int max_fd = -1;
    struct timeval timeout;
    int rval;

    FD_ZERO(&read_fds);
    FD_ZERO(&write_fds);

    static bool UartInitialized = false;
    if (!UartInitialized)
    {
        UartInitialized = true;
        otPlatUartEnable();
    }

    windowsRadioUpdateFdSet(&read_fds, &write_fds, &max_fd);
    windowsAlarmUpdateTimeout(&timeout);

    if (!otAreTaskletsPending(aInstance))
    {
        rval = select(max_fd + 1, &read_fds, &write_fds, NULL, &timeout);
        assert(rval >= 0 && errno != ETIME);
    }

    windowsRadioProcess(aInstance);
    windowsAlarmProcess(aInstance);
}

