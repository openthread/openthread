/*
 *  Copyright (c) 2018, The OpenThread Authors.
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

#include <assert.h>

#include <openthread-core-config.h>
#include <openthread/config.h>

#define OPENTHREAD_POSIX_APP_NCP 1
#define OPENTHREAD_POSIX_APP_CLI 2

#include <openthread/diag.h>
#if OPENTHREAD_POSIX_APP == OPENTHREAD_POSIX_APP_NCP
#include <openthread/ncp.h>
#elif OPENTHREAD_POSIX_APP == OPENTHREAD_POSIX_APP_CLI
#include <openthread/cli.h>
#else
#error "Unknown posix app mode!"
#endif
#include <openthread/openthread.h>
#include <openthread/platform/logging.h>

#include "platform.h"

void otTaskletsSignalPending(otInstance *aInstance)
{
    (void)aInstance;
}

int main(int argc, char *argv[])
{
    otInstance *sInstance;

pseudo_reset:

    PlatformInit(argc, argv);

    sInstance = otInstanceInitSingle();
    assert(sInstance);

#if OPENTHREAD_POSIX_APP == OPENTHREAD_POSIX_APP_NCP
    otNcpInit(sInstance);
#elif OPENTHREAD_POSIX_APP == OPENTHREAD_POSIX_APP_CLI
    otCliUartInit(sInstance);
#endif

#if OPENTHREAD_ENABLE_DIAG
    otDiagInit(sInstance);
#endif

    while (!PlatformPseudoResetWasRequested())
    {
        otTaskletsProcess(sInstance);
        PlatformProcessDrivers(sInstance);
    }

    otInstanceFinalize(sInstance);

    goto pseudo_reset;

    return 0;
}

/*
 * Provide, if required an "otPlatLog()" function
 */
#if OPENTHREAD_CONFIG_LOG_OUTPUT == OPENTHREAD_CONFIG_LOG_OUTPUT_APP
void otPlatLog(otLogLevel aLogLevel, otLogRegion aLogRegion, const char *aFormat, ...)
{
    OT_UNUSED_VARIABLE(aLogLevel);
    OT_UNUSED_VARIABLE(aLogRegion);
    OT_UNUSED_VARIABLE(aFormat);

    va_list ap;
    va_start(ap, aFormat);
#if OPENTHREAD_POSIX_APP == OPENTHREAD_POSIX_APP_NCP
    otCliPlatLogv(aLogLevel, aLogRegion, aFormat, ap);
#elif OPENTHREAD_POSIX_APP == OPENTHREAD_POSIX_APP_CLI
    otNcpPlatLogv(aLogLevel, aLogRegion, aFormat, ap);
#endif
    va_end(ap);
}
#endif
