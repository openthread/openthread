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
 *   This file defines the Cordio BLE stack initialization interfaces.
 */

#ifndef BLE_INIT_H
#define BLE_INIT_H

#if OPENTHREAD_ENABLE_BLE_HOST

#ifdef __cplusplus
extern "C" {
#endif

#include <openthread/instance.h>

/**
 * This enumeration represents the state of the BLE stack.
 */
enum
{
    kStateDisabled       = 0, ///< The BLE stack is disabled.
    kStateInitializing   = 1, ///< The BLE stack is initializing.
    kStateInitialized    = 2, ///< The BLE stack is initialized.
    kStateDeinitializing = 3, ///< The BLE stack is deinitializing.
};

/**
 * This method returns the OpenThread instance.
 *
 */
otInstance *bleGetThreadInstance(void);

/**
 * This method returns the state of the BLE stack.
 *
 */
uint8_t bleGetState(void);

#ifdef __cplusplus
} // extern "C"
#endif

#endif // OPENTHREAD_ENABLE_BLE_HOST
#endif // BLE_INIT_H
