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

#ifndef OPENTHREAD_CONSOLE_CLI_H_
#define OPENTHREAD_CONSOLE_CLI_H_

#include "platform/openthread-posix-config.h"

#include <stdint.h>

#include <openthread/openthread-system.h>

#include "cli/cli_config.h"

#ifndef HAVE_LIBEDIT
#define HAVE_LIBEDIT 0
#endif

#ifndef HAVE_LIBREADLINE
#define HAVE_LIBREADLINE 0
#endif

#if OPENTHREAD_CONFIG_CLI_TRANSPORT == OT_CLI_TRANSPORT_CONSOLE
#define OPENTHREAD_USE_CONSOLE 1
#if !(HAVE_LIBEDIT || HAVE_LIBREADLINE) || OPENTHREAD_POSIX_CONFIG_DAEMON_ENABLE
#error \
    "When OPENTHREAD_CONFIG_CLI_TRANSPORT=OT_CLI_TRANSPORT_CONSOLE, HAVE_LIBEDIT or HAVE_LIBREADLINE MUST be defined and OPENTHREAD_POSIX_CONFIG_DAEMON_ENABLE MUST be 0"
#endif
#endif

#ifdef __cplusplus
extern "C" {
#endif

/**
 * This function initializes CLI console.
 *
 * @param[in]  aInstance    A pointer to the OpenThread instance.
 *
 */
void otxConsoleInit(otInstance *aInstance);

/**
 * This function deinitializes CLI console
 *
 */
void otxConsoleDeinit(void);

/**
 * This function updates the file descriptor sets with file descriptors used by console.
 *
 * @param[inout]    aMainloop   A pointer to the mainloop context.
 *
 */
void otxConsoleUpdate(otSysMainloopContext *aMainloop);

/**
 * This function performs console driver processing.
 *
 * @param[in]    aMainloop      A pointer to the mainloop context.
 *
 */
void otxConsoleProcess(const otSysMainloopContext *aMainloop);

#ifdef __cplusplus
}
#endif

#endif // OPENTHREAD_CONSOLE_CLI_H_
