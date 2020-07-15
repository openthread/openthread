/*
 *  Copyright (c) 2020, The OpenThread Authors.
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
 *  This file defines the top-level functions for the legacy network apis.
 */

#ifndef OPENTHREAD_LEGACY_H_
#define OPENTHREAD_LEGACY_H_

#include "openthread/platform/radio.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @addtogroup api-message
 *
 * @brief
 *   This module includes functions that manipulate OpenThread message buffers.
 *
 * @{
 *
 */

#define OT_LEGACY_ULA_PREFIX_LENGTH 8 ///< Legacy ULA size (in bytes)

/**
 * This method initializes the legacy processing.
 *
 */
void otLegacyInit(void);

/**
 * This method starts the legacy processing.
 *
 * Will be called when Thread network is enabled.
 *
 */
void otLegacyStart(void);

/**
 * This method stops the legacy processing.
 *
 * Will be called when Thread network is disabled.
 *
 */
void otLegacyStop(void);

/**
 * Defines handler (function pointer) type for setting the legacy ULA prefix.
 *
 * @param[in] aUlaPrefix   A pointer to buffer containing the legacy ULA prefix.
 *
 * Invoked to set the legacy ULA prefix.
 *
 */
void otSetLegacyUlaPrefix(const uint8_t *aUlaPrefix);

/**
 * This callback is invoked by the legacy stack to notify that a new
 * legacy node did join the network.
 *
 * @param[in]   aExtAddr    A pointer to the extended address of the joined node.
 *
 */
void otHandleLegacyNodeDidJoin(const otExtAddress *aExtAddr);

/**
 * This callback is invoked by the legacy stack to notify that the
 * legacy ULA prefix has changed.
 *
 * @param[in]    aUlaPrefix  A pointer to the received ULA prefix.
 *
 */
void otHandleDidReceiveNewLegacyUlaPrefix(const uint8_t *aUlaPrefix);

/**
 * @}
 *
 */

#ifdef __cplusplus
} // end of extern "C"
#endif

#endif
