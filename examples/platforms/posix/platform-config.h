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
 *   This file includes the platform-specific configuration.
 *
 */

/**
 * @def OPENTHREAD_POSIX_UART_BAUDRATE
 *
 * This setting configures the baudrate of the UART.
 *
 */
#ifndef OPENTHREAD_POSIX_UART_BAUDRATE
#define OPENTHREAD_POSIX_UART_BAUDRATE B115200
#endif

/**
 * @def OT_BLE_MAX_NUM_SERVICES
 *
 * This setting configures the maximum number of Bluetooth GATT services.
 *
 */
#ifndef OT_BLE_MAX_NUM_SERVICES
#define OT_BLE_MAX_NUM_SERVICES 2
#endif

/**
 * @def OT_BLE_MAX_NUM_CHARACTERISTICS
 *
 * This setting configures the maximum number of Bluetooth GATT characteristics.
 *
 */
#ifndef OT_BLE_MAX_NUM_CHARACTERISTICS
#define OT_BLE_MAX_NUM_CHARACTERISTICS 5
#endif

/**
 * @def OT_BLE_MAX_NUM_UUIDS
 *
 * This setting configures the maximum number of Bluetooth GATT UUIDs.
 *
 */
#ifndef OT_BLE_MAX_NUM_UUIDS
#define OT_BLE_MAX_NUM_UUIDS (OT_BLE_MAX_NUM_SERVICES + OT_BLE_MAX_NUM_CHARACTERISTICS)
#endif
