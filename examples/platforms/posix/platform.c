/*
 *  Copyright (c) 2016, Nest Labs, Inc.
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

/**
 * @file
 * @brief
 *   This file includes the platform-specific initializers.
 */

#include <assert.h>
#include <errno.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>

#include <openthread.h>
#include <platform/alarm.h>
#include <platform/uart.h>
#include "platform-posix.h"

uint32_t NODE_ID = 1;
uint32_t WELLKNOWN_NODE_ID = 34;

void PlatformInit(int argc, char *argv[])
{
    char *endptr;

    if (argc != 2)
    {
        exit(EXIT_FAILURE);
    }

    NODE_ID = (uint32_t)strtol(argv[1], &endptr, 0);

    if (*endptr != '\0')
    {
        fprintf(stderr, "Invalid NODE_ID: %s\n", argv[1]);
        exit(EXIT_FAILURE);
    }

    posixAlarmInit();
    posixRadioInit();
    posixRandomInit();
    otPlatUartEnable();
}

void PlatformProcessDrivers(otInstance *aInstance)
{
    fd_set read_fds;
    fd_set write_fds;
    fd_set error_fds;
    int max_fd = -1;
    struct timeval timeout;
    int rval;

    FD_ZERO(&read_fds);
    FD_ZERO(&write_fds);
    FD_ZERO(&error_fds);

    posixUartUpdateFdSet(&read_fds, &write_fds, &error_fds, &max_fd);
    posixRadioUpdateFdSet(&read_fds, &write_fds, &max_fd);
    posixAlarmUpdateTimeout(&timeout);

    if (!otAreTaskletsPending(aInstance))
    {
        rval = select(max_fd + 1, &read_fds, &write_fds, &error_fds, &timeout);

        if ((rval < 0) && (errno != EINTR))
        {
            perror("select");
            exit(EXIT_FAILURE);
        }
    }

    posixUartProcess();
    posixRadioProcess(aInstance);
    posixAlarmProcess(aInstance);
}

