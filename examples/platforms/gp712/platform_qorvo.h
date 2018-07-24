/*
 *  Copyright (c) 2017, The OpenThread Authors.
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
 *   This file includes the platform-specific initializers.
 *
 */

#ifndef PLATFORM_QORVO_H_
#define PLATFORM_QORVO_H_

#include <openthread/config.h>
#include <openthread-core-config.h>

#include <stdint.h>

typedef void     (*qorvoPlatPollFunction_t)(uint8_t);
typedef uint8_t  (*qorvoPlatGotoSleepCheckCallback_t) (void);

/**
 * This function registers a callback to a file descriptor.
 *
 * @param[in]  fd            The file descriptor.
 * @param[in]  pollFunction  The callback which should be called when data is ready or needed for the file descriptor.
 *
 */
void qorvoPlatRegisterPollFunction(int fd, qorvoPlatPollFunction_t pollFunction);

/**
 * This function unregisters a callback for a file descriptor.
 *
 * @param[in]  fd            The file descriptor.
 *
 */
void qorvoPlatUnRegisterPollFunction(int fd);

/**
 * This function initializes the platform.
 *
 * @param[in]  gotoSleepCheckCallback  The callback which needs to return if sleep is allowed.
 *
 */
void qorvoPlatInit(qorvoPlatGotoSleepCheckCallback_t gotoSleepCheckCallback);

/**
 * This function runs the main loop of the platform once.
 *
 * @param[in]  canGoToSleep  Indicates if the platform can got to sleep.
 *
 */
void qorvoPlatMainLoop(bool canGoToSleep);

#endif  // PLATFORM_QORVO_H_
