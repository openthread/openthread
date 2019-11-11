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
#include <openthread-core-config.h>
#include <openthread/config.h>

#include <openthread/cli.h>
#include <openthread/diag.h>
#include <openthread/tasklet.h>
#include <openthread/platform/logging.h>

#include "openthread-system.h"

#if OPENTHREAD_EXAMPLES_POSIX
#include <setjmp.h>
#include <unistd.h>

jmp_buf gResetJump;

void __gcov_flush();
#endif

#if OPENTHREAD_CONFIG_MULTIPLE_INSTANCE_ENABLE
void *otPlatCAlloc(size_t aNum, size_t aSize)
{
    return calloc(aNum, aSize);
}

void otPlatFree(void *aPtr)
{
    free(aPtr);
}
#endif

void otTaskletsSignalPending(otInstance *aInstance)
{
    OT_UNUSED_VARIABLE(aInstance);
}

int main(int argc, char *argv[])
{
    otInstance *instance;

#if OPENTHREAD_EXAMPLES_POSIX
    if (setjmp(gResetJump))
    {
        alarm(0);
#if OPENTHREAD_ENABLE_COVERAGE
        __gcov_flush();
#endif
        execvp(argv[0], argv);
    }
#endif

#if OPENTHREAD_CONFIG_MULTIPLE_INSTANCE_ENABLE
    size_t   otInstanceBufferLength = 0;
    uint8_t *otInstanceBuffer       = NULL;
#endif

pseudo_reset:

    otSysInit(argc, argv);


//------------------------------------------------------------------------------
#define RUN_OT_NVM3_UNIT_TEST 1
#if RUN_OT_NVM3_UNIT_TEST

//OT NVM3 Unit Tests (Tests OT Settings Objects)

{
    #include <openthread/platform/settings.h>
    #include <assert.h>
    #include <string.h>

    uint8_t     data[60];
    for (uint8_t i = 0; i < sizeof(data); ++i)
    {
        data[i] = i;
    }

    otPlatSettingsInit(instance);

    // verify empty situation
    otPlatSettingsWipe(instance);
    {
        uint8_t  value[sizeof(data)];
        uint16_t length = sizeof(value);

        assert(otPlatSettingsGet(instance, 0, 0, value, &length) == OT_ERROR_NOT_FOUND);
        assert(otPlatSettingsDelete(instance, 0, 0) == OT_ERROR_NOT_FOUND);
        assert(otPlatSettingsDelete(instance, 0, -1) == OT_ERROR_NOT_FOUND);
    }

    // verify write one record
    assert(otPlatSettingsSet(instance, 0, data, sizeof(data) / 2) == OT_ERROR_NONE);
    {
        uint8_t  value[sizeof(data)];
        uint16_t length = sizeof(value);

        assert(otPlatSettingsGet(instance, 0, 0, NULL, NULL) == OT_ERROR_NONE);
        assert(otPlatSettingsGet(instance, 0, 0, NULL, &length) == OT_ERROR_NONE);
        assert(length == sizeof(data) / 2);

        length = sizeof(value);
        assert(otPlatSettingsGet(instance, 0, 0, value, &length) == OT_ERROR_NONE);
        assert(length == sizeof(data) / 2);
        assert(0 == memcmp(value, data, length));

        // insufficient buffer
        length -= 1;
        value[length] = 0;
        assert(otPlatSettingsGet(instance, 0, 0, value, &length) == OT_ERROR_NONE);
        // verify length becomes the actual length of the record
        assert(length == sizeof(data) / 2);
        // verify this byte is not changed.
        assert(value[length - 1] == 0);   // i.e. checks value[29] == 0.

        // wrong index
        assert(otPlatSettingsGet(instance, 0, 1, NULL, NULL) == OT_ERROR_NOT_FOUND);
        // wrong key
        assert(otPlatSettingsGet(instance, 1, 0, NULL, NULL) == OT_ERROR_NOT_FOUND);
    }
    otPlatSettingsWipe(instance);

    // verify write two records
    assert(otPlatSettingsSet(instance, 0, data, sizeof(data)) == OT_ERROR_NONE);
    assert(otPlatSettingsAdd(instance, 0, data, sizeof(data) / 2) == OT_ERROR_NONE);
    {
        uint8_t  value[sizeof(data)];
        uint16_t length = sizeof(value);

        assert(otPlatSettingsGet(instance, 0, 1, value, &length) == OT_ERROR_NONE);
        assert(length == sizeof(data) / 2);
        assert(0 == memcmp(value, data, length));

        length = sizeof(value);
        assert(otPlatSettingsGet(instance, 0, 0, value, &length) == OT_ERROR_NONE);
        assert(length == sizeof(data));
        assert(0 == memcmp(value, data, length));
    }
    otPlatSettingsWipe(instance);

    // verify write two records of different keys
    assert(otPlatSettingsSet(instance, 0, data, sizeof(data)) == OT_ERROR_NONE);
    assert(otPlatSettingsAdd(instance, 1, data, sizeof(data) / 2) == OT_ERROR_NONE);
    {
        uint8_t  value[sizeof(data)];
        uint16_t length = sizeof(value);

        assert(otPlatSettingsGet(instance, 1, 0, value, &length) == OT_ERROR_NONE);
        assert(length == sizeof(data) / 2);
        assert(0 == memcmp(value, data, length));

        length = sizeof(value);
        assert(otPlatSettingsGet(instance, 0, 0, value, &length) == OT_ERROR_NONE);
        assert(length == sizeof(data));
        assert(0 == memcmp(value, data, length));
    }
    otPlatSettingsWipe(instance);

    // verify delete record
    assert(otPlatSettingsAdd(instance, 0, data, sizeof(data)) == OT_ERROR_NONE);
    assert(otPlatSettingsAdd(instance, 0, data, sizeof(data) / 2) == OT_ERROR_NONE);
    assert(otPlatSettingsAdd(instance, 0, data, sizeof(data) / 3) == OT_ERROR_NONE);
    {
        uint8_t  value[sizeof(data)];
        uint16_t length = sizeof(value);

        // wrong key
        assert(otPlatSettingsDelete(instance, 1, 0) == OT_ERROR_NOT_FOUND);
        assert(otPlatSettingsDelete(instance, 1, -1) == OT_ERROR_NOT_FOUND);

        // wrong index
        assert(otPlatSettingsDelete(instance, 0, 3) == OT_ERROR_NOT_FOUND);

        // delete one record
        assert(otPlatSettingsDelete(instance, 0, 1) == OT_ERROR_NONE);
        assert(otPlatSettingsGet(instance, 0, 1, value, &length) == OT_ERROR_NONE);
        assert(length == sizeof(data) / 3);
        assert(0 == memcmp(value, data, length));

        // delete all records
        assert(otPlatSettingsDelete(instance, 0, -1) == OT_ERROR_NONE);
        assert(otPlatSettingsGet(instance, 0, 0, NULL, NULL) == OT_ERROR_NOT_FOUND);
    }
    otPlatSettingsWipe(instance);

    // verify delete all records of a type
    assert(otPlatSettingsAdd(instance, 0, data, sizeof(data)) == OT_ERROR_NONE);
    assert(otPlatSettingsAdd(instance, 1, data, sizeof(data) / 2) == OT_ERROR_NONE);
    assert(otPlatSettingsAdd(instance, 0, data, sizeof(data) / 3) == OT_ERROR_NONE);
    {
        uint8_t  value[sizeof(data)];
        uint16_t length = sizeof(value);

        assert(otPlatSettingsDelete(instance, 0, -1) == OT_ERROR_NONE);
        assert(otPlatSettingsGet(instance, 0, 0, value, &length) == OT_ERROR_NOT_FOUND);
        assert(otPlatSettingsGet(instance, 1, 0, value, &length) == OT_ERROR_NONE);
        assert(length == sizeof(data) / 2);
        assert(0 == memcmp(value, data, length));

        assert(otPlatSettingsDelete(instance, 0, 0) == OT_ERROR_NOT_FOUND);
        assert(otPlatSettingsGet(instance, 0, 0, NULL, NULL) == OT_ERROR_NOT_FOUND);
    }
    otPlatSettingsWipe(instance);
    otPlatSettingsDeinit(instance);
}
#endif // RUN_OT_NVM3_UNIT_TEST

//------------------------------------------------------------------------------


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

    otCliUartInit(instance);

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
    otCliPlatLogv(aLogLevel, aLogRegion, aFormat, ap);
    va_end(ap);
}
#endif
