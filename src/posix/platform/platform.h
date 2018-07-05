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

/**
 * @file
 * @brief
 *   This file defines the platform-specific initializers.
 */

#ifndef PLATFORM_H_
#define PLATFORM_H_

#include <openthread/types.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * This function performs all platform-specific initialization.
 *
 */
void PlatformInit(int argc, char *argv[]);

/**
 * This function performs all platform-specific deinitialization.
 *
 */
void PlatformDeinit(void);

/**
 * This function returns true is a pseudo-reset was requested.
 * In such a case, the main loop should shut down and re-initialize
 * the OpenThread instance.
 *
 */
bool PlatformPseudoResetWasRequested(void);

/**
 * This function performs all platform-specific processing.
 *
 * @param[in]  aInstance  The OpenThread instance structure.
 *
 */
void PlatformProcessDrivers(otInstance *aInstance);

/**
 * This function is called whenever platform drivers needs processing.
 *
 */
extern void PlatformEventSignalPending(void);

#ifdef __cplusplus
} // end of extern "C"
#endif

#endif // PLATFORM_H_
