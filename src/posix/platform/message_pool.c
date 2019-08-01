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

/**
 * @file
 * @brief
 *   This file includes a message pool implemented by malloc
 */

#include <stdlib.h>
#include <string.h>

#include <openthread-core-config.h>
#include <openthread/platform/messagepool.h>

#if OPENTHREAD_CONFIG_PLATFORM_MESSAGE_MANAGEMENT

#define PLAT_ALLOC_ALIGN 32

static size_t sBufferSize;

void otPlatMessagePoolInit(otInstance *aInstance, uint16_t aMinNumFreeBuffers, size_t aBufferSize)
{
    (void)aInstance;
    (void)aMinNumFreeBuffers;
    (void)aBufferSize;

    sBufferSize = ((aBufferSize - 1) / PLAT_ALLOC_ALIGN + 1) * PLAT_ALLOC_ALIGN;
}

otMessage *otPlatMessagePoolNew(otInstance *aInstance)
{
    (void)aInstance;

    otMessage *message = (otMessage *)(aligned_alloc(PLAT_ALLOC_ALIGN, sBufferSize));

    if (message != NULL)
    {
        memset(message, 0, sBufferSize);
    }
    return message;
}

void otPlatMessagePoolFree(otInstance *aInstance, otMessage *aBuffer)
{
    (void)aInstance;

    free(aBuffer);
}

uint16_t otPlatMessagePoolNumFreeBuffers(otInstance *aInstance)
{
    (void)aInstance;

    return UINT16_MAX;
}

#endif // OPENTHREAD_CONFIG_PLATFORM_MESSAGE_MANAGEMENT
