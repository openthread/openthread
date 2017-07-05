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

#ifdef OPENTHREAD_CONFIG_FILE
#include OPENTHREAD_CONFIG_FILE
#else
#include <openthread-config.h>
#endif

#include <openthread-core-config.h>
#include <openthread.h>
#include <openthread-diag.h>
#include <openthread-tasklet.h>
#include <cli/cli-uart.h>
#include <platform/platform.h>
#include <assert.h>

void otSignalTaskletPending(otInstance *aInstance)
{
    (void)aInstance;
}

int main(int argc, char *argv[])
{
    otInstance *sInstance;

#ifdef OPENTHREAD_MULTIPLE_INSTANCE
    uint64_t otInstanceBufferLength = 0;
    uint8_t *otInstanceBuffer = NULL;
#endif

    PlatformInit(argc, argv);

#ifdef OPENTHREAD_MULTIPLE_INSTANCE
    // Call to query the buffer size
    (void)otInstanceInit(NULL, &otInstanceBufferLength);

    // Call to allocate the buffer
    otInstanceBuffer = (uint8_t *)malloc(otInstanceBufferLength);
    assert(otInstanceBuffer);

    // Initialize Openthread with the buffer
    sInstance = otInstanceInit(otInstanceBuffer, &otInstanceBufferLength);
#else
    sInstance = otInstanceInit();
#endif
    assert(sInstance);

    otCliUartInit(sInstance);

#if OPENTHREAD_ENABLE_DIAG
    diagInit(sInstance);
#endif

    while (1)
    {
        otProcessQueuedTasklets(sInstance);
        PlatformProcessDrivers(sInstance);
    }

    // otInstanceFinalize(sInstance);
#ifdef OPENTHREAD_MULTIPLE_INSTANCE
    // free(otInstanceBuffer);
#endif

    return 0;
}
