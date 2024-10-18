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

#include <openthread/cli.h>
#include <openthread/diag.h>
#include <openthread/tasklet.h>
#include <openthread/platform/logging.h>
#include <openthread/platform/misc.h>

#include "openthread-system.h"
#include "cli/cli_config.h"
#include "common/code_utils.hpp"

#include "lib/platform/reset_util.h"

/**
 * Initializes the CLI app.
 *
 * @param[in]  aInstance  The OpenThread instance structure.
 */
extern void otAppCliInit(otInstance *aInstance);

#if OPENTHREAD_CONFIG_HEAP_EXTERNAL_ENABLE
OT_TOOL_WEAK void *otPlatCAlloc(size_t aNum, size_t aSize) { return calloc(aNum, aSize); }

OT_TOOL_WEAK void otPlatFree(void *aPtr) { free(aPtr); }
#endif

#if OPENTHREAD_POSIX && !defined(FUZZING_BUILD_MODE_UNSAFE_FOR_PRODUCTION)
static otError ProcessExit(void *aContext, uint8_t aArgsLength, char *aArgs[])
{
    OT_UNUSED_VARIABLE(aContext);
    OT_UNUSED_VARIABLE(aArgsLength);
    OT_UNUSED_VARIABLE(aArgs);

    exit(EXIT_SUCCESS);
}

#if OPENTHREAD_EXAMPLES_SIMULATION
extern otError ProcessNodeIdFilter(void *aContext, uint8_t aArgsLength, char *aArgs[]);
#endif

static const otCliCommand kCommands[] = {
    {"exit", ProcessExit},
#if OPENTHREAD_EXAMPLES_SIMULATION
    /*
     * The CLI command `nodeidfilter` only works for simulation in real time.
     *
     * It can be used either as an allow list or a deny list. Once the filter is cleared, the first `nodeidfilter allow`
     * or `nodeidfilter deny` will determine whether it is set up as an allow or deny list. Subsequent calls should
     * use the same sub-command to add new node IDs, e.g., if we first call `nodeidfilter allow` (which sets the filter
     * up  as an allow list), a subsequent `nodeidfilter deny` will result in `InvalidState` error.
     *
     * The usage of the command `nodeidfilter`:
     *     - `nodeidfilter deny <nodeid>` :  It denies the connection to a specified node (use as deny-list).
     *     - `nodeidfilter allow <nodeid> :  It allows the connection to a specified node (use as allow-list).
     *     - `nodeidfilter clear`         :  It restores the filter state to default.
     *     - `nodeidfilter`               :  Outputs filter mode (allow-list or deny-list) and filtered node IDs.
     */
    {"nodeidfilter", ProcessNodeIdFilter},
#endif
};
#endif // OPENTHREAD_POSIX && !defined(FUZZING_BUILD_MODE_UNSAFE_FOR_PRODUCTION)

int main(int argc, char *argv[])
{
    otInstance *instance;

#ifdef __linux__
    // Ensure we terminate this process if the
    // parent process dies.
    prctl(PR_SET_PDEATHSIG, SIGHUP);
#endif

    OT_SETUP_RESET_JUMP(argv);

#if OPENTHREAD_CONFIG_MULTIPLE_INSTANCE_ENABLE
    size_t   otInstanceBufferLength = 0;
    uint8_t *otInstanceBuffer       = NULL;
#endif

pseudo_reset:

    otSysInit(argc, argv);

#if OPENTHREAD_CONFIG_MULTIPLE_INSTANCE_ENABLE
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

    otAppCliInit(instance);

#if OPENTHREAD_POSIX && !defined(FUZZING_BUILD_MODE_UNSAFE_FOR_PRODUCTION)
    IgnoreError(otCliSetUserCommands(kCommands, OT_ARRAY_LENGTH(kCommands), instance));
#endif

#if OPENTHREAD_CONFIG_PLATFORM_LOG_CRASH_DUMP_ENABLE
    IgnoreError(otPlatLogCrashDump());
#endif

    while (!otSysPseudoResetWasRequested())
    {
        otTaskletsProcess(instance);
        otSysProcessDrivers(instance);
    }

    otInstanceFinalize(instance);
#if OPENTHREAD_CONFIG_MULTIPLE_INSTANCE_ENABLE
    free(otInstanceBuffer);
#endif

    goto pseudo_reset;

    return 0;
}

#if OPENTHREAD_CONFIG_LOG_OUTPUT == OPENTHREAD_CONFIG_LOG_OUTPUT_APP
void otPlatLog(otLogLevel aLogLevel, otLogRegion aLogRegion, const char *aFormat, ...)
{
    va_list ap;

    va_start(ap, aFormat);
    otCliPlatLogv(aLogLevel, aLogRegion, aFormat, ap);
    va_end(ap);
}
#endif
