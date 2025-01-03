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

#include <assert.h>
#ifdef __linux__
#include <signal.h>
#include <sys/prctl.h>
#endif

#include <openthread-core-config.h>
#include <openthread/config.h>

#include <openthread/diag.h>
#include <openthread/ncp.h>
#include <openthread/tasklet.h>

#include "openthread-system.h"

#include "lib/platform/reset_util.h"

#if OPENTHREAD_CONFIG_MULTIPAN_RCP_ENABLE
#if OPENTHREAD_CONFIG_MULTIPLE_STATIC_INSTANCE_ENABLE == 0
#error "Support for multiple OpenThread static instance is disabled."
#endif
#define ENDPOINT_CT OPENTHREAD_CONFIG_MULTIPLE_INSTANCE_NUM
#else
#define ENDPOINT_CT 1
#endif /* OPENTHREAD_CONFIG_MULTIPAN_RCP_ENABLE */

/**
 * Initializes the NCP app.
 *
 * @param[in]  aInstance  The OpenThread instance structure.
 */
extern void otAppNcpInit(otInstance *aInstance);
extern void otAppNcpInitMulti(otInstance **aInstances, uint8_t count);

#if OPENTHREAD_CONFIG_HEAP_EXTERNAL_ENABLE
OT_TOOL_WEAK void *otPlatCAlloc(size_t aNum, size_t aSize) { return calloc(aNum, aSize); }

OT_TOOL_WEAK void otPlatFree(void *aPtr) { free(aPtr); }
#endif

int main(int argc, char *argv[])
{
    otInstance *instance;

    OT_SETUP_RESET_JUMP(argv);

#ifdef __linux__
    // Ensure we terminate this process if the
    // parent process dies.
    prctl(PR_SET_PDEATHSIG, SIGHUP);
#endif

#if OPENTHREAD_CONFIG_MULTIPAN_RCP_ENABLE
    otInstance *instances[ENDPOINT_CT] = {NULL};
#elif OPENTHREAD_CONFIG_MULTIPLE_INSTANCE_ENABLE
    size_t   otInstanceBufferLength = 0;
    uint8_t *otInstanceBuffer       = NULL;
#endif

pseudo_reset:

    otSysInit(argc, argv);

#if OPENTHREAD_CONFIG_MULTIPAN_RCP_ENABLE
    for (int i = 0; i < ENDPOINT_CT; i++)
    {
        instances[i] = otInstanceInitMultiple(i);

        assert(instances[i]);
    }
    instance = instances[0];
#elif OPENTHREAD_CONFIG_MULTIPLE_INSTANCE_ENABLE

    // Call to query the buffer size
    (void)otInstanceInit(NULL, &otInstanceBufferLength);

    // Call to allocate the buffer
    otInstanceBuffer = (uint8_t *)malloc(otInstanceBufferLength);
    assert(otInstanceBuffer);

    // Initialize OpenThread with the buffer
    instance = otInstanceInit(otInstanceBuffer, &otInstanceBufferLength);
#else
    instance = otInstanceInitSingle();
#endif
    assert(instance);

#if OPENTHREAD_CONFIG_MULTIPAN_RCP_ENABLE
    otAppNcpInitMulti(instances, ENDPOINT_CT);
#else
    otAppNcpInit(instance);
#endif

    while (!otSysPseudoResetWasRequested())
    {
        otTaskletsProcess(instance);
        otSysProcessDrivers(instance);
    }

    otInstanceFinalize(instance);
#if OPENTHREAD_CONFIG_MULTIPLE_INSTANCE_ENABLE && !OPENTHREAD_CONFIG_MULTIPAN_RCP_ENABLE
    free(otInstanceBuffer);
#endif

    goto pseudo_reset;

    return 0;
}
