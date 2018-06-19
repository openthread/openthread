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

/**
 * @file
 * @brief
 *   This file includes the platform abstraction for GPIO.
 */

#ifndef OPENTHREAD_PLATFORM_GPIO_H_
#define OPENTHREAD_PLATFORM_GPIO_H_

#include <stdint.h>

#include <openthread/types.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef void (*otPlatGpioEventHandler)(void *aContext);

void otGpioInit(void);

void otPlatGpioCfgOutput(uint32_t aPin);

void otPlatGpioCfgInput(uint32_t aPin);

void otPlatGpioWrite(uint32_t aPin, uint32_t aValue);

uint32_t otPlatGpioRead(uint32_t aPin);

void otPlatGpioClear(uint32_t aPin);

void otPlatGpioSet(uint32_t aPin);

void otPlatGpioToggle(uint32_t aPin);

void otPlatGpioEnableInterrupt(uint32_t aPin, otPlatGpioEventHandler aGpioEventHandler, void *aContext);

void otPlatGpioDisableInterrupt(uint32_t aPin);

extern void otPlatGpioSignalEvent(uint32_t aPin);

#ifdef __cplusplus
}  // extern "C"
#endif

#endif  // OPENTHREAD_PLATFORM_GPIO_H_
