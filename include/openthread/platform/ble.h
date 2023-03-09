/*
 *  Copyright (c) 2023, The OpenThread Authors.
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
 *   This file defines a OpenThread BLE GATT peripheral interface driver.
 *
 */

#ifndef OPENTHREAD_PLATFORM_BLE_H_
#define OPENTHREAD_PLATFORM_BLE_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

#include <openthread/error.h>
#include <openthread/instance.h>

/**
 * @addtogroup plat-ble
 *
 * @brief
 *   This module includes the platform abstraction for BLE Host communication.
 *   The platform needs to implement Bluetooth LE 4.2 or higher.
 *
 * @{
 *
 */

/**
 * Time slot duration on PHY layer in microseconds (0.625ms).
 *
 */

#define OT_BLE_TIMESLOT_UNIT 625

/**
 * Minimum allowed interval for advertising packet in OT_BLE_ADV_INTERVAL_UNIT units (20ms).
 *
 */

#define OT_BLE_ADV_INTERVAL_MIN 0x0020

/**
 * Maximum allowed interval for advertising packet in OT_BLE_ADV_INTERVAL_UNIT units (10.24s).
 *
 */

#define OT_BLE_ADV_INTERVAL_MAX 0x4000

/**
 * Default interval for advertising packet (ms).
 *
 */

#define OT_BLE_ADV_INTERVAL_DEFAULT 100

/**
 * Unit used to calculate interval duration (0.625ms).
 *
 */

#define OT_BLE_ADV_INTERVAL_UNIT OT_BLE_TIMESLOT_UNIT

/**
 * Maximum allowed ATT MTU size (must be >= 23).
 *
 */

#define OT_BLE_ATT_MTU_MAX 67

/**
 * Default power value for BLE.
 */

#define OT_BLE_DEFAULT_POWER 0

/**
 * Represents a BLE packet.
 *
 */
typedef struct otBleRadioPacket
{
    uint8_t *mValue;  ///< The value of an attribute
    uint16_t mLength; ///< Length of the @p mValue.
    int8_t   mPower;  ///< Transmit/receive power in dBm.
} otBleRadioPacket;

/*******************************************************************************
 * @section Bluetooth Low Energy management.
 ******************************************************************************/

/**
 * Enable the Bluetooth Low Energy radio.
 *
 * @note BLE Device should use the highest ATT_MTU supported that does not
 * exceed OT_BLE_ATT_MTU_MAX octets.
 *
 * @param[in] aInstance  The OpenThread instance structure.
 *
 * @retval OT_ERROR_NONE         Successfully enabled.
 * @retval OT_ERROR_FAILED       The BLE radio could not be enabled.
 */
otError otPlatBleEnable(otInstance *aInstance);

/**
 * Disable the Bluetooth Low Energy radio.
 *
 * When disabled, the BLE stack will flush event queues and not generate new
 * events. The BLE peripheral is turned off or put into a low power sleep
 * state. Any dynamic memory used by the stack should be released,
 * but static memory may remain reserved.
 *
 * @param[in] aInstance  The OpenThread instance structure.
 *
 * @retval OT_ERROR_NONE        Successfully transitioned to disabled.
 * @retval OT_ERROR_FAILED      The BLE radio could not be disabled.
 *
 */
otError otPlatBleDisable(otInstance *aInstance);

/****************************************************************************
 * @section Bluetooth Low Energy GAP.
 ***************************************************************************/

/**
 * Starts BLE Advertising procedure.
 *
 * The BLE device shall use undirected advertising with no filter applied.
 * A single BLE Advertising packet must be sent on all advertising
 * channels (37, 38 and 39).
 *
 * @note This function shall be used only for BLE Peripheral role.
 *
 * @param[in] aInstance  The OpenThread instance structure.
 * @param[in] aInterval  The interval between subsequent advertising packets
 *                       in OT_BLE_ADV_INTERVAL_UNIT units.
 *                       Shall be within OT_BLE_ADV_INTERVAL_MIN and
 *                       OT_BLE_ADV_INTERVAL_MAX range or OT_BLE_ADV_INTERVAL_DEFAULT
 *                       for a default value set at compile time.
 *
 * @retval OT_ERROR_NONE           Advertising procedure has been started.
 * @retval OT_ERROR_INVALID_STATE  BLE Device is in invalid state.
 * @retval OT_ERROR_INVALID_ARGS   Invalid interval value has been supplied.
 *
 */
otError otPlatBleGapAdvStart(otInstance *aInstance, uint16_t aInterval);

/**
 * Stops BLE Advertising procedure.
 *
 * @note This function shall be used only for BLE Peripheral role.
 *
 * @param[in] aInstance  The OpenThread instance structure.
 *
 * @retval OT_ERROR_NONE           Advertising procedure has been stopped.
 * @retval OT_ERROR_INVALID_STATE  BLE Device is in invalid state.
 *
 */
otError otPlatBleGapAdvStop(otInstance *aInstance);

/**
 * The BLE driver calls this method to notify OpenThread that a BLE Central Device has
 * been connected.
 *
 * @param[in]  aInstance     The OpenThread instance structure.
 * @param[in]  aConnectionId The identifier of the open connection.
 *
 */
extern void otPlatBleGapOnConnected(otInstance *aInstance, uint16_t aConnectionId);

/**
 * The BLE driver calls this method to notify OpenThread that the BLE Central Device
 * has been disconnected.
 *
 * @param[in]  aInstance     The OpenThread instance structure.
 * @param[in]  aConnectionId The identifier of the closed connection.
 *
 */
extern void otPlatBleGapOnDisconnected(otInstance *aInstance, uint16_t aConnectionId);

/**
 * Disconnects BLE connection.
 *
 * The BLE device shall use the Remote User Terminated Connection (0x13) reason
 * code when disconnecting from the peer BLE device..
 *
 * @param[in] aInstance  The OpenThread instance structure.
 *
 * @retval OT_ERROR_NONE           Disconnection procedure has been started.
 * @retval OT_ERROR_INVALID_STATE  BLE Device is in invalid state.
 *
 */
otError otPlatBleGapDisconnect(otInstance *aInstance);

/*******************************************************************************
 * @section Bluetooth Low Energy GATT Common.
 *******************************************************************************/

/**
 * Reads currently use value of ATT_MTU.
 *
 * @param[in]   aInstance  The OpenThread instance structure.
 * @param[out]  aMtu       A pointer to output the current ATT_MTU value.
 *
 * @retval OT_ERROR_NONE     ATT_MTU value has been placed in @p aMtu.
 * @retval OT_ERROR_FAILED   BLE Device cannot determine its ATT_MTU.
 *
 */
otError otPlatBleGattMtuGet(otInstance *aInstance, uint16_t *aMtu);

/**
 * The BLE driver calls this method to notify OpenThread that ATT_MTU has been updated.
 *
 * @param[in]  aInstance     The OpenThread instance structure.
 * @param[in]  aMtu          The updated ATT_MTU value.
 *
 */
extern void otPlatBleGattOnMtuUpdate(otInstance *aInstance, uint16_t aMtu);

/*******************************************************************************
 * @section Bluetooth Low Energy GATT Server.
 ******************************************************************************/

/**
 * Sends ATT Handle Value Indication.
 *
 * @note This function shall be used only for GATT Server.
 *
 * @param[in] aInstance   The OpenThread instance structure.
 * @param[in] aHandle     The handle of the attribute to be indicated.
 * @param[in] aPacket     A pointer to the packet contains value to be indicated.
 *
 * @retval OT_ERROR_NONE           ATT Handle Value Indication has been sent.
 * @retval OT_ERROR_INVALID_STATE  BLE Device is in invalid state.
 * @retval OT_ERROR_INVALID_ARGS   Invalid handle value, data or data length has been supplied.
 * @retval OT_ERROR_NO_BUFS        No available internal buffer found.
 *
 */
otError otPlatBleGattServerIndicate(otInstance *aInstance, uint16_t aHandle, const otBleRadioPacket *aPacket);

/**
 * The BLE driver calls this method to notify OpenThread that an ATT Write Request
 * packet has been received.
 *
 * @note This function shall be used only for GATT Server.
 *
 * @param[in] aInstance   The OpenThread instance structure.
 * @param[in] aHandle     The handle of the attribute to be written.
 * @param[in] aPacket     A pointer to the packet contains value to be written to the attribute.
 *
 */
extern void otPlatBleGattServerOnWriteRequest(otInstance *aInstance, uint16_t aHandle, const otBleRadioPacket *aPacket);

/**
 * @}
 *
 */

#ifdef __cplusplus
} // end of extern "C"
#endif

#endif // OPENTHREAD_PLATFORM_BLE_H_
