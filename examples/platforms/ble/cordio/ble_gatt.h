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
 *   This file defines the Cordio BLE stack GATT interfaces.
 */

#ifndef BLE_GATT_H
#define BLE_GATT_H
#if OPENTHREAD_ENABLE_BLE_HOST

#ifdef __cplusplus
extern "C" {
#endif

#include "att_api.h"

/**
 * This method initializes the BLE GAP module.
 *
 */
void bleGattInit(void);

/**
 * This method resets the BLE GAP module.
 *
 */
void bleGattReset(void);

/**
 * This method processes the ATT events.
 *
 */
void bleAttHandler(attEvt_t *aEvent);

/**
 * This method notifies the GATT module that the BLE GAP connection is opened.
 *
 * @param[in] aMsg  A pointer to a WSF message.
 */
void bleGattGapConnectedHandler(const wsfMsgHdr_t *aMsg);

/**
 * This method notifies the GATT module that the BLE GAP connection is closed.
 *
 * @param[in] aMsg  A pointer to a WSF message.
 */
void bleGattGapDisconnectedHandler(const wsfMsgHdr_t *aMsg);

#ifdef __cplusplus
} // extern "C"
#endif // __cplusplus

#endif // OPENTHREAD_ENABLE_BLE_HOST
#endif // BLE_GATT_H
