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
 *   This file defines a generic OpenThread BLE driver HOST interface.
 *
 */

#ifndef OT_PLATFORM_BLE_H_
#define OT_PLATFORM_BLE_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

#include <openthread/error.h>

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

enum
{
    /**
     * The size of the Bluetooth Device Address [bytes].
     */
    OT_BLE_ADDRESS_LENGTH = 6,

    /**
     * Time slot duration on PHY layer in microseconds (0.625ms).
     */
    OT_BLE_TIMESLOT_UNIT = 625,

    /**
     * Unit used to calculate connection interval (1.25ms)
     */
    OT_BLE_CONN_INTERVAL_UNIT = 2 * OT_BLE_TIMESLOT_UNIT,

    /**
     * Minimum allowed connection interval in OT_BLE_CONN_INTERVAL_UNIT units (7.5ms).
     * See v4.2 [Vol 2, Part E] page 946
     */
    OT_BLE_CONN_INTERVAL_MIN = 0x0006,

    /**
     * Maximum allowed connection interval in OT_BLE_CONN_INTERVAL_UNIT units (4s).
     * See v4.2 [Vol 2, Part E] page 946
     */
    OT_BLE_CONN_INTERVAL_MAX = 0x0C80,

    /**
     * Maximum allowed slave latency in units of connection events.
     * See v4.2 [Vol 2, Part E] page 946
     */
    OT_BLE_CONN_SLAVE_LATENCY_MAX = 0x01F3,

    /**
     * Minimum allowed connection timeout in units of 10ms (100ms).
     * See v4.2 [Vol 2, Part E] page 946
     */
    OT_BLE_CONN_SUPERVISOR_TIMEOUT_MIN = 0x000A,

    /**
     * Maximum allowed connection timeout (32s).
     * See v4.2 [Vol 2, Part E] page 946
     */
    OT_BLE_CONN_SUPERVISOR_TIMEOUT_MAX = 0x0C80,

    /**
     * Unit used to calculate connection supervisor timeout (10ms).
     */
    OT_BLE_CONN_SUPERVISOR_UNIT = 16 * OT_BLE_TIMESLOT_UNIT,

    /**
     * Maximum length of the device name characteristic [bytes].
     */
    OT_BLE_DEV_NAME_MAX_LENGTH = 248,

    /**
     * Maximum length of advertising data [bytes].
     */
    OT_BLE_ADV_DATA_MAX_LENGTH = 31,

    /**
     * Maximum length of scan response data [bytes].
     */
    OT_BLE_SCAN_RESPONSE_MAX_LENGTH = 31,

    /**
     * Minimum allowed interval for advertising packet in OT_BLE_ADV_INTERVAL_UNIT units (20ms).
     */
    OT_BLE_ADV_INTERVAL_MIN = 0x0020,

    /**
     * Maximum allowed interval for advertising packet in OT_BLE_ADV_INTERVAL_UNIT units (10.24s).
     */
    OT_BLE_ADV_INTERVAL_MAX = 0x4000,

    /**
     * Unit used to calculate interval duration (0.625ms).
     */
    OT_BLE_ADV_INTERVAL_UNIT = OT_BLE_TIMESLOT_UNIT,

    /**
     * Minimum allowed scan interval (2.5ms).
     */
    OT_BLE_SCAN_INTERVAL_MIN = 0x0004,

    /**
     * Maximum allowed scan interval (10.24s).
     */
    OT_BLE_SCAN_INTERVAL_MAX = 0x4000,

    /**
     * Unit used to calculate scan interval duration (0.625ms).
     */
    OT_BLE_SCAN_INTERVAL_UNIT = OT_BLE_TIMESLOT_UNIT,

    /**
     * Minimum allowed scan window in OT_BLE_TIMESLOT_UNIT units (2.5ms).
     */
    OT_BLE_SCAN_WINDOW_MIN = 0x0004,

    /**
     * Maximum allowed scan window in OT_BLE_TIMESLOT_UNIT units (10.24s).
     */
    OT_BLE_SCAN_WINDOW_MAX = 0x4000,

    /**
     * Unit used to calculate scan window duration (0.625ms).
     */
    OT_BLE_SCAN_WINDOW_UNIT = OT_BLE_TIMESLOT_UNIT,

    /**
     * BLE HCI code for remote user terminated connection.
     */
    OT_BLE_HCI_REMOTE_USER_TERMINATED = 0x13,

    /**
     * Value of invalid/unknown handle.
     */
    OT_BLE_INVALID_HANDLE = 0x0000,

    /**
     * Maximum size of BLE Characteristic [bytes].
     */
    OT_BLE_CHARACTERISTIC_MAX_LENGTH = 512,

    /**
     * Maximum value of ATT_MTU [bytes].
     */
    OT_BLE_ATT_MTU_MAX = 511,

    /**
     * Length of full BLE UUID in bytes.
     */
    OT_BLE_UUID_LENGTH = 16,

    /**
     * Uuid of Client Configuration Characteristic Descriptor.
     */
    OT_BLE_UUID_CCCD = 0x2902,

};

/**
 * This enum represents BLE Device Address types.
 *
 */
typedef enum otPlatBleAddressType
{
    OT_BLE_ADDRESS_TYPE_PUBLIC                        = 0, ///< Bluetooth public device address.
    OT_BLE_ADDRESS_TYPE_RANDOM_STATIC                 = 1, ///< Bluetooth random static address.
    OT_BLE_ADDRESS_TYPE_RANDOM_PRIVATE_RESOLVABLE     = 2, ///< Bluetooth random private resolvable address.
    OT_BLE_ADDRESS_TYPE_RANDOM_PRIVATE_NON_RESOLVABLE = 3, ///< Bluetooth random private non-resolvable address.
} otPlatBleAddressType;

/**
 * This enumeration defines the characterstic properties flags for a
 * Client Characteristic Configuration Descriptor (CCCD).
 *
 * See v4.2 [Vol 3, Part G] 3.3.1.1 Characteristic Properties - Table 3.5
 */
typedef enum otPlatBleCccdFlags
{
    /**
     * If set, permits broadcasts of the Characteristic Value using Characteristic Configuration Descriptor.
     */
    OT_BLE_CHAR_PROP_BROADCAST = (1 << 0),

    /**
     * If set, permits reads of the Characteristic Value.
     */
    OT_BLE_CHAR_PROP_READ = (1 << 1),

    /**
     * If set, permit writes of the Characteristic Value without response.
     */
    OT_BLE_CHAR_PROP_WRITE_NO_RESPONSE = (1 << 2),

    /**
     * If set, permits writes of the Characteristic Value with response.
     */
    OT_BLE_CHAR_PROP_WRITE = (1 << 3),

    /**
     * If set, permits notifications of a Characteristic Value without acknowledgement.
     */
    OT_BLE_CHAR_PROP_NOTIFY = (1 << 4),

    /**
     * If set, permits indications of a Characteristic Value with acknowledgement.
     */
    OT_BLE_CHAR_PROP_INDICATE = (1 << 5),

    /**
     * If set, permits signed writes to the Characteristic Value.
     */
    OT_BLE_CHAR_PROP_AUTH_SIGNED_WRITE = (1 << 6),

    /**
     * If set, additional characteristic properties are defined in the Characteristic Extended Properties Descriptor.
     */
    OT_BLE_CHAR_PROP_EXTENDED = (1 << 7),

} otPlatBleCccdFlags;

/**
 * This structure represents BLE Device Address.
 *
 */
typedef struct otPlatBleDeviceAddr
{
    uint8_t mAddrType;                    ///< Bluetooth device address type.
    uint8_t mAddr[OT_BLE_ADDRESS_LENGTH]; ///< An 48-bit address of Bluetooth device in LSB format.
} otPlatBleDeviceAddr;

/**
 * This enumeration defines flags for BLE advertisement mode.
 *
 */
typedef enum otPlatBleAdvMode
{
    /**
     * If set, advertising device will allow connections to be initiated.
     */
    OT_BLE_ADV_MODE_CONNECTABLE = (1 << 0),

    /**
     * If set, advertising device will respond to scan requests.
     */
    OT_BLE_ADV_MODE_SCANNABLE = (1 << 1),
} otPlatBleAdvMode;

/**
 * This structure represents BLE connection parameters.
 *
 */
typedef struct otPlatBleGapConnParams
{
    /**
     * Preferred minimum connection interval in OT_BLE_TIMESLOT_UNIT units.
     * Shall be in OT_BLE_CONN_INTERVAL_MIN and OT_BLE_CONN_INTERVAL_MAX range.
     */
    uint16_t mConnMinInterval;

    /**
     * Preferred maximum connection interval in OT_BLE_TIMESLOT_UNIT units.
     * Shall be in OT_BLE_CONN_INTERVAL_MIN and OT_BLE_CONN_INTERVAL_MAX range.
     */
    uint16_t mConnMaxInterval;

    /**
     * Slave Latency in number of connection events.
     * Shall not exceed OT_BLE_CONN_SLAVE_LATENCY_MAX value.
     */
    uint16_t mConnSlaveLatency;

    /**
     * Defines connection timeout parameter in OT_BLE_CONN_SUPERVISOR_UNIT units. Shall be in
     * OT_BLE_CONN_SUPERVISOR_TIMEOUT_MIN and OT_BLE_CONN_SUPERVISOR_TIMEOUT_MIN range.
     */
    uint16_t mConnSupervisionTimeout;

} otPlatBleGapConnParams;

/**
 * This enumeration represents BLE UUID value.
 *
 */
typedef enum otPlatBleUuidType
{
    OT_BLE_UUID_TYPE_16  = 0, ///< UUID represented by 16-bit value.
    OT_BLE_UUID_TYPE_32  = 1, ///< UUID represented by 32-bit value.
    OT_BLE_UUID_TYPE_128 = 2, ///< UUID represented by 128-bit value.
} otPlatBleUuidType;

/**
 * This structure represents BLE UUID value.
 *
 */
typedef union otPlatBleUuidValue
{
    uint8_t *mUuid128; ///< A pointer to 128-bit UUID value.
    uint32_t mUuid32;  ///< A 32-bit UUID value.
    uint16_t mUuid16;  ///< A 16-bit UUID value.
} otPlatBleUuidValue;

/**
 * This structure represents BLE UUID.
 *
 */
typedef struct otPlatBleUuid
{
    otPlatBleUuidType  mType;  ///< A type of UUID in @p mValue.
    otPlatBleUuidValue mValue; ///< A value of UUID.
} otPlatBleUuid;

/**
 * This structure represents GATT Characteristic.
 *
 */
typedef struct otPlatBleGattCharacteristic
{
    otPlatBleUuid mUuid;        ///< [in]  A UUID value of a characteristic.
    uint16_t      mHandleValue; ///< [out] Characteristic value handle.
    uint16_t      mHandleCccd;  ///< [out] CCCD handle or OT_BLE_INVALID_HANDLE if CCCD is not present.
    uint8_t       mProperties;  ///< [in]  Characteristic properties.
} otPlatBleGattCharacteristic;

/**
 * This structure represents GATT Descriptor.
 *
 */
typedef struct otPlatBleGattDescriptor
{
    otPlatBleUuid mUuid;   ///< A UUID value of descriptor.
    uint16_t      mHandle; ///< Descriptor handle.
} otPlatBleGattDescriptor;

/**
 * This structure represents an BLE packet.
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
 * @retval ::OT_ERROR_NONE         Successfully enabled.
 * @retval ::OT_ERROR_FAILED       The BLE radio could not be enabled.
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
 * @retval ::OT_ERROR_NONE        Successfully transitioned to disabled.
 * @retval ::OT_ERROR_FAILED      The BLE radio could not be disabled.
 */
otError otPlatBleDisable(otInstance *aInstance);

/**
 * Check whether Bluetooth Low Energy radio is enabled or not.
 *
 * @param[in] aInstance  The OpenThread instance structure.
 *
 * @retval ::true   Bluetooth Low Energy radio is enabled.
 * @retval ::false  Bluetooth Low Energy radio is disabled.
 */
bool otPlatBleIsEnabled(otInstance *aInstance);

/****************************************************************************
 * @section Bluetooth Low Energy GAP.
 ***************************************************************************/

/**
 * Gets Bluetooth Device Address.
 *
 * @param[in]  aInstance  The OpenThread instance structure.
 * @param[out] aAddress   The pointer to Bluetooth Device Address.
 *
 * @retval ::OT_ERROR_NONE          Request has been successfully done.
 * @retval ::OT_ERROR_INVALID_ARGS  Invalid parameters has been supplied.
 */
otError otPlatBleGapAddressGet(otInstance *aInstance, otPlatBleDeviceAddr *aAddress);

/**
 * Sets Bluetooth Device Address.
 *
 * @param[in]  aInstance  The OpenThread instance structure.
 * @param[in] aAddress    The pointer to Bluetooth Device Address.
 *
 * @retval ::OT_ERROR_NONE          Request has been successfully done.
 * @retval ::OT_ERROR_INVALID_ARGS  Invalid parameters has been supplied.
 */
otError otPlatBleGapAddressSet(otInstance *aInstance, const otPlatBleDeviceAddr *aAddress);

/**
 * Sets BLE device name and appearance that is visible as GATT Based service.
 *
 * The BLE Host stack should set the security mode 1, level 1 (no security) for
 * those characteristics.
 *
 * @param[in] aInstance    The OpenThread instance structure.
 * @param[in] aDeviceName  A pointer to device name string (null terminated).
 *                         Shall not exceed OT_BLE_DEV_NAME_MAX_LENGTH.
 * @param[in] aAppearance  The value of appearance characteristic.
 *
 * @retval ::OT_ERROR_NONE          Connection parameters have been
 *                                  successfully set.
 * @retval ::OT_ERROR_INVALID_ARGS  Invalid parameters has been supplied.
 */
otError otPlatBleGapServiceSet(otInstance *aInstance, const char *aDeviceName, uint16_t aAppearance);

/**
 * Sets desired BLE Connection Parameters.
 *
 * @param[in] aInstance    The OpenThread instance structure.
 * @param[in] aConnParams  A pointer to connection parameters structure.
 *
 * @retval ::OT_ERROR_NONE          Connection parameters have been
 *                                  successfully set.
 * @retval ::OT_ERROR_INVALID_ARGS  Invalid connection parameters have been
 *                                  supplied.
 */
otError otPlatBleGapConnParamsSet(otInstance *aInstance, const otPlatBleGapConnParams *aConnParams);

/**
 * Sets BLE Advertising packet content.
 *
 * @note This function shall be used only for BLE Peripheral role.
 *
 * @param[in] aInstance       The OpenThread instance structure.
 * @param[in] aAdvData        A pointer to advertising data content in raw format.
 * @param[in] aAdvDataLength  The size of advertising data.
 *                            Shall not exceed OT_BLE_ADV_DATA_MAX_LENGTH.
 *
 * @retval ::OT_ERROR_NONE          Advertising data has been successfully set.
 * @retval ::OT_ERROR_INVALID_ARGS  Invalid advertising data has been supplied.
 */
otError otPlatBleGapAdvDataSet(otInstance *aInstance, const uint8_t *aAdvData, uint8_t aAdvDataLength);

/**
 * Sets BLE Scan Response packet content.
 *
 * @note This function shall be used only for BLE Peripheral role.
 *
 * @param[in] aInstance            The OpenThread instance structure.
 * @param[in] aScanResponse        A pointer to scan response data in raw format.
 * @param[in] aScanResponseLength  The size of scan response data.
 *                                 Shall not exceed OT_BLE_SCAN_RESPONSE_MAX_LENGTH.
 *
 * @retval ::OT_ERROR_NONE         Scan response data has been successfully set.
 * @retval ::OT_ERROR_INVALID_ARGS Invalid scan response data has been supplied.
 */
otError otPlatBleGapScanResponseSet(otInstance *aInstance, const uint8_t *aScanResponse, uint8_t aScanResponseLength);

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
 *                       OT_BLE_ADV_INTERVAL_MAX range.
 * @param[in] aType      The advertisement properties as a bitmask:
 *                       whether it is connectable | scannable.
 *
 * @retval ::OT_ERROR_NONE           Advertising procedure has been started.
 * @retval ::OT_ERROR_INVALID_STATE  BLE Device is in invalid state.
 * @retval ::OT_ERROR_INVALID_ARGS   Invalid interval value has been supplied.
 */
otError otPlatBleGapAdvStart(otInstance *aInstance, uint16_t aInterval, uint8_t aType);

/**
 * Stops BLE Advertising procedure.
 *
 * @note This function shall be used only for BLE Peripheral role.
 *
 * @param[in] aInstance  The OpenThread instance structure.
 *
 * @retval ::OT_ERROR_NONE           Advertising procedure has been stopped.
 * @retval ::OT_ERROR_INVALID_STATE  BLE Device is in invalid state.
 */
otError otPlatBleGapAdvStop(otInstance *aInstance);

/**
 * The BLE driver calls this method to notify OpenThread that BLE Device has
 * been connected.
 *
 * @param[in]  aInstance     The OpenThread instance structure.
 * @param[in]  aConnectionId The identifier of the open connection.
 *
 */
extern void otPlatBleGapOnConnected(otInstance *aInstance, uint16_t aConnectionId);

/**
 * The BLE driver calls this method to notify OpenThread that the BLE Device
 * has been disconnected.
 *
 * @param[in]  aInstance     The OpenThread instance structure.
 * @param[in]  aConnectionId The identifier of the closed connection.
 *
 */
extern void otPlatBleGapOnDisconnected(otInstance *aInstance, uint16_t aConnectionId);

/**
 * Starts BLE Scanning procedure.
 *
 * @note This function shall be used only for BLE Central role.
 *
 * @param[in] aInstance  The OpenThread instance structure.
 * @param[in] aInterval  The scanning interval in OT_BLE_SCAN_INTERVAL_UNIT
 *                       units. Shall be in OT_BLE_SCAN_INTERVAL_MIN and
 *                       OT_BLE_SCAN_INTERVAL_MAX range.
 * @param[in] aWindow    The scanning window in OT_BLE_SCAN_WINDOW_UNIT units.
 *                       Shall be in OT_BLE_SCAN_WINDOW_MIN and OT_BLE_SCAN_WINDOW_MAX
 *                       range.
 *
 * @retval ::OT_ERROR_NONE           Scanning procedure has been started.
 * @retval ::OT_ERROR_INVALID_STATE  BLE Device is in invalid state.
 * @retval ::OT_ERROR_INVALID_ARGS   Invalid interval or window value has been
 *                                   supplied.
 */
otError otPlatBleGapScanStart(otInstance *aInstance, uint16_t aInterval, uint16_t aWindow);

/**
 * Stops BLE Scanning procedure.
 *
 * @note This function shall be used only for BLE Central role.
 *
 * @param[in] aInstance  The OpenThread instance structure.
 *
 * @retval ::OT_ERROR_NONE           Scanning procedure has been stopped.
 * @retval ::OT_ERROR_INVALID_STATE  BLE Device is in invalid state.
 */
otError otPlatBleGapScanStop(otInstance *aInstance);

/**
 * The BLE driver calls this method to notify OpenThread that an advertisement
 * packet has been received.
 *
 * @param[in]  aInstance  The OpenThread instance structure.
 * @param[in]  aAddress   An address of the advertising device.
 * @param[in]  aPacket    A pointer to the received packet.
 *
 */
extern void otPlatBleGapOnAdvReceived(otInstance *aInstance, otPlatBleDeviceAddr *aAddress, otBleRadioPacket *aPacket);

/**
 * The BLE driver calls this method to notify OpenThread that a scan response packet has been received.
 *
 * @param[in]  aInstance  The OpenThread instance structure.
 * @param[in]  aAddress   An address of the advertising device.
 * @param[in]  aPacket    A pointer to the received packet.
 *
 */
extern void otPlatBleGapOnScanRespReceived(otInstance *         aInstance,
                                           otPlatBleDeviceAddr *aAddress,
                                           otBleRadioPacket *   aPacket);

/**
 * Starts BLE Connection procedure.
 *
 * @note This function shall be used only for BLE Central role.
 *
 * @param[in] aInstance  The OpenThread instance structure.
 * @param[in] aAddress   An address of the advertising device.
 * @param[in] aInterval  The scanning interval in OT_BLE_SCAN_INTERVAL_UNIT units. Shall be in
 *                       OT_BLE_SCAN_INTERVAL_MIN and OT_BLE_SCAN_INTERVAL_MAX range.
 * @param[in] aWindow    The scanning window in OT_BLE_SCAN_WINDOW_UNIT units. Shall be in
 *                       OT_BLE_SCAN_WINDOW_MIN and OT_BLE_SCAN_WINDOW_MAX range.
 *
 * @retval ::OT_ERROR_NONE           Connection procedure has been started.
 * @retval ::OT_ERROR_INVALID_STATE  BLE Device is in invalid state.
 * @retval ::OT_ERROR_INVALID_ARGS   Invalid address, interval or window value has been supplied.
 */
otError otPlatBleGapConnect(otInstance *aInstance, otPlatBleDeviceAddr *aAddress, uint16_t aInterval, uint16_t aWindow);

/**
 * Disconnects BLE connection.
 *
 * The BLE device shall indicate the OT_BLE_HCI_REMOTE_USER_TERMINATED HCI code reason.
 *
 * @param[in] aInstance  The OpenThread instance structure.
 *
 * @retval ::OT_ERROR_NONE           Disconnection procedure has been started.
 * @retval ::OT_ERROR_INVALID_STATE  BLE Device is in invalid state.
 */
otError otPlatBleGapDisconnect(otInstance *aInstance);

/*******************************************************************************
 * @section Bluetooth Low Energy GATT Common.
 *******************************************************************************/

/**
 * Registers vendor specific UUID Base.
 *
 * @param[in]  aInstance  The OpenThread instance structure.
 * @param[in]  aUuid      A pointer to vendor specific 128-bit UUID Base.
 *
 */
otError otPlatBleGattVendorUuidRegister(otInstance *aInstance, const otPlatBleUuid *aUuid);

/**
 * Reads currently use value of ATT_MTU.
 *
 * @param[in]   aInstance  The OpenThread instance structure.
 * @param[out]  aMtu       A pointer contains current ATT_MTU value.
 *
 * @retval ::OT_ERROR_NONE     ATT_MTU value has been placed in @p aMtu.
 * @retval ::OT_ERROR_FAILED   BLE Device cannot determine its ATT_MTU.
 */
otError otPlatBleGattMtuGet(otInstance *aInstance, uint16_t *aMtu);

/*******************************************************************************
 * @section Bluetooth Low Energy GATT Client.
 ******************************************************************************/

/**
 * Sends ATT Read Request.
 *
 * @note This function shall be used only for GATT Client.
 *
 * @param[in] aInstance  The OpenThread instance structure.
 * @param[in] aHandle    The handle of the attribute to be read.
 *
 * @retval ::OT_ERROR_NONE           ATT Write Request has been sent.
 * @retval ::OT_ERROR_INVALID_STATE  BLE Device is in invalid state.
 * @retval ::OT_ERROR_INVALID_ARGS   Invalid handle value has been supplied.
 * @retval ::OT_ERROR_NO_BUFS        No available internal buffer found.
 */
otError otPlatBleGattClientRead(otInstance *aInstance, uint16_t aHandle);

/**
 * The BLE driver calls this method to notify OpenThread that ATT Read Response
 * packet has been received.
 *
 * This method is called only if @p otPlatBleGattClientRead was previously requested.
 *
 * @note This function shall be used only for GATT Client.
 *
 * @param[in] aInstance  The OpenThread instance structure.
 * @param[in] aPacket    A pointer to the packet contains read value.
 *
 */
extern void otPlatBleGattClientOnReadResponse(otInstance *aInstance, otBleRadioPacket *aPacket);

/**
 * Sends ATT Write Request.
 *
 * @note This function shall be used only for GATT Client.
 *
 * @param[in] aInstance  The OpenThread instance structure.
 * @param[in] aHandle    The handle of the attribute to be written.
 * @param[in] aPacket    A pointer to the packet contains value to be
 *                       written to the attribute.
 *
 * @retval ::OT_ERROR_NONE           ATT Write Request has been sent.
 * @retval ::OT_ERROR_INVALID_STATE  BLE Device is in invalid state.
 * @retval ::OT_ERROR_INVALID_ARGS   Invalid handle value, data or data length has been supplied.
 * @retval ::OT_ERROR_NO_BUFS        No available internal buffer found.
 */
otError otPlatBleGattClientWrite(otInstance *aInstance, uint16_t aHandle, otBleRadioPacket *aPacket);

/**
 * The BLE driver calls this method to notify OpenThread that ATT Write Response
 * packet has been received.
 *
 * This method is called only if @p otPlatBleGattClientWrite was previously requested.
 *
 * @note This function shall be used only for GATT Client.
 *
 * @param[in] aInstance  The OpenThread instance structure.
 * @param[in] aHandle    The handle on which ATT Write Response has been sent.
 *
 */

extern void otPlatBleGattClientOnWriteResponse(otInstance *aInstance, uint16_t aHandle);
/**
 * Subscribes for characteristic indications.
 *
 * @note This function shall be used only for GATT Client.
 *
 * @param[in] aInstance    The OpenThread instance structure.
 * @param[in] aHandle      The handle of the attribute to be written.
 * @param[in] aSubscribing True if subscribing, otherwise unsubscribing.
 *
 * @retval ::OT_ERROR_NONE           Subscription has been sent.
 * @retval ::OT_ERROR_INVALID_STATE  BLE Device is in invalid state.
 * @retval ::OT_ERROR_INVALID_ARGS   Invalid handle value, data or data length has been supplied.
 * @retval ::OT_ERROR_NO_BUFS        No available internal buffer found.
 */
otError otPlatBleGattClientSubscribeRequest(otInstance *aInstance, uint16_t aHandle, bool aSubscribing);

/**
 * The BLE driver calls this method to notify OpenThread that subscribe response
 * has been received.
 *
 * This method is called only if @p otPlatBleGattClienSubscribe was previously requested.
 *
 * @note This function shall be used only for GATT Client.
 *
 * @param[in] aInstance  The OpenThread instance structure.
 * @param[in] aHandle    The handle on which ATT Write Response has been sent.
 *
 */
extern void otPlatBleGattClientOnSubscribeResponse(otInstance *aInstance, uint16_t aHandle);

/**
 * The BLE driver calls this method to notify OpenThread that an ATT Handle Value
 * Indication has been received.
 *
 * @note This function shall be used only for GATT Client.
 *
 * @param[in] aInstance  The OpenThread instance structure.
 * @param[in] aHandle    The handle on which ATT Handle Value Indication has been sent.
 * @param[in] aPacket    A pointer to the packet contains indicated value.
 *
 */
extern void otPlatBleGattClientOnIndication(otInstance *aInstance, uint16_t aHandle, otBleRadioPacket *aPacket);

/**
 * Performs GATT Primary Service Discovery of all services available.
 *
 * @note This function shall be used only for GATT Client.
 *
 * @param[in] aInstance  The OpenThread instance structure.
 *
 * @retval ::OT_ERROR_NONE           Service Discovery procedure has been started.
 * @retval ::OT_ERROR_INVALID_STATE  BLE Device is in invalid state.
 * @retval ::OT_ERROR_INVALID_ARGS   Invalid service UUID has been provided.
 * @retval ::OT_ERROR_NO_BUFS        No available internal buffer found.
 */
otError otPlatBleGattClientServicesDiscover(otInstance *aInstance);

/**
 * Performs GATT Primary Service Discovery by UUID procedure of specific service.
 *
 * @note This function shall be used only for GATT Client.
 *
 * @param[in] aInstance  The OpenThread instance structure.
 * @param[in] aUuid      The UUID of a service to be registered.
 *
 * @retval ::OT_ERROR_NONE           Service Discovery procedure has been started.
 * @retval ::OT_ERROR_INVALID_STATE  BLE Device is in invalid state.
 * @retval ::OT_ERROR_INVALID_ARGS   Invalid service UUID has been provided.
 * @retval ::OT_ERROR_NO_BUFS        No available internal buffer found.
 */
otError otPlatBleGattClientServiceDiscover(otInstance *aInstance, const otPlatBleUuid *aUuid);

/**
 * The BLE driver calls this method to notify OpenThread that the next entry
 * from GATT Primary Service Discovery has been found.
 *
 * @note This function shall be used only for GATT Client.
 *
 * @param[in] aInstance     The OpenThread instance structure.
 * @param[in] aStartHandle  The start handle of a service.
 * @param[in] aEndHandle    The end handle of a service.
 * @param[in] aServiceUuid  The Uuid16 for the service entry.
 * @param[in] aError        The value of OT_ERROR_NONE indicates that service has been found
 *                          and structure @p aStartHandle and @p aEndHandle contain valid handles.
 *                          OT_ERROR_NOT_FOUND error should be set if service has not been found.
 *                          Otherwise error indicates the reason of failure is used.
 *
 */
extern void otPlatBleGattClientOnServiceDiscovered(otInstance *aInstance,
                                                   uint16_t    aStartHandle,
                                                   uint16_t    aEndHandle,
                                                   uint16_t    aServiceUuid,
                                                   otError     aError);

/**
 * Performs GATT Characteristic Discovery of a service.
 *
 * @note This function shall be used only for GATT Client.
 *
 * @param[in] aInstance     The OpenThread instance structure.
 * @param[in] aStartHandle  The start handle of a service.
 * @param[in] aEndHandle    The end handle of a service.
 *
 * @retval ::OT_ERROR_NONE           Characteristic Discovery procedure has been started.
 * @retval ::OT_ERROR_INVALID_STATE  BLE Device is in invalid state.
 * @retval ::OT_ERROR_INVALID_ARGS   Invalid start or end handle has been provided.
 * @retval ::OT_ERROR_NO_BUFS        No available internal buffer found.
 */
otError otPlatBleGattClientCharacteristicsDiscover(otInstance *aInstance, uint16_t aStartHandle, uint16_t aEndHandle);

/**
 * The BLE driver calls this method to notify OpenThread that GATT Characteristic
 * Discovery of a service has been done.
 *
 * In case of success, all elements inside @p aChars should have a valid mHandleValue value.
 *
 * @note This function shall be used only for GATT Client.
 *
 * @param[in] aInstance  The OpenThread instance structure.
 * @param[in] aChars     A pointer to discovered characteristic list.
 * @param[in] aCount     Number of characteristics in @p aChar list.
 * @param[in] aError     The value of OT_ERROR_NONE indicates that at least one characteristic
 *                       has been found and the total number of them is stored in @p aCount.
 *                       OT_ERROR_NOT_FOUND error should be set if no charactertistics are found.
 *                       Otherwise error indicates the reason of failure is used.
 *
 */
extern void otPlatBleGattClientOnCharacteristicsDiscoverDone(otInstance *                 aInstance,
                                                             otPlatBleGattCharacteristic *aChars,
                                                             uint16_t                     aCount,
                                                             otError                      aError);

/**
 * Performs GATT Descriptor Discovery.
 *
 * @note This function shall be used only for GATT Client.
 *
 * @param[in] aInstance     The OpenThread instance structure.
 * @param[in] aStartHandle  The start handle.
 * @param[in] aEndHandle    The end handle.
 *
 * @retval ::OT_ERROR_NONE           Descriptor Discovery procedure has been started.
 * @retval ::OT_ERROR_INVALID_STATE  BLE Device is in invalid state.
 * @retval ::OT_ERROR_INVALID_ARGS   Invalid start or end handle has been provided.
 * @retval ::OT_ERROR_NO_BUFS        No available internal buffer found.
 */
otError otPlatBleGattClientDescriptorsDiscover(otInstance *aInstance, uint16_t aStartHandle, uint16_t aEndHandle);

/**
 * The BLE driver calls this method to notify OpenThread that GATT Descriptor
 * Discovery has been done.
 *
 * @note This function shall be used only for GATT Client.
 *
 * @param[in] aInstance  The OpenThread instance structure.
 * @param[in] aDescs     A pointer to discovered descriptor list.
 * @param[in] aCount     Number of descriptors in @p aDescs list.
 * @param[in] aError     The value of OT_ERROR_NONE indicates that at least one descriptor
 *                       has been found and the total number of them is stored in @p aCount.
 *                       OT_ERROR_NOT_FOUND error should be set if no descriptors are found.
 *                       Otherwise error indicates the reason of failure is used.
 *
 */
extern void otPlatBleGattClientOnDescriptorsDiscoverDone(otInstance *             aInstance,
                                                         otPlatBleGattDescriptor *aDescs,
                                                         uint16_t                 aCount,
                                                         otError                  aError);

/**
 * Sends Exchange MTU Request.
 *
 * @note This function shall be used only for GATT Client.
 *
 * @param[in] aInstance  The OpenThread instance structure.
 * @param[in] aMtu       A value of GATT Client receive MTU size.
 *
 * @retval ::OT_ERROR_NONE           Exchange MTU Request has been sent.
 * @retval ::OT_ERROR_INVALID_STATE  BLE Device is in invalid state.
 * @retval ::OT_ERROR_INVALID_ARGS   Invalid aMtu has been provided.
 * @retval ::OT_ERROR_NO_BUFS        No available internal buffer found.
 */
otError otPlatBleGattClientMtuExchangeRequest(otInstance *aInstance, uint16_t aMtu);

/**
 * The BLE driver calls this method to notify OpenThread that Exchange MTU
 * Response has been received.
 *
 * @note This function shall be used only for GATT Client.
 *
 * @param[in] aInstance  The OpenThread instance structure.
 * @param[in] aMtu       Attribute server receive MTU size.
 * @param[in] aError     The value of OT_ERROR_NONE indicates that valid
 *                       Exchange MTU Response has been received. Otherwise error
 *                       indicates the reason of failure is used.
 *
 */
extern void otPlatBleGattClientOnMtuExchangeResponse(otInstance *aInstance, uint16_t aMtu, otError aError);

/*******************************************************************************
 * @section Bluetooth Low Energy GATT Server.
 ******************************************************************************/

/**
 * Registers GATT Service.
 *
 * @note This function shall be used only for GATT Server.
 *
 * @param[in]   aInstance  The OpenThread instance structure.
 * @param[in]   aUuid      The UUID of a service.
 * @param[out]  aHandle    The start handle of a service.
 *
 * @retval ::OT_ERROR_NONE           Service has been successfully registered.
 * @retval ::OT_ERROR_INVALID_STATE  BLE Device is in invalid state.
 * @retval ::OT_ERROR_INVALID_ARGS   Invalid service UUID has been provided.
 * @retval ::OT_ERROR_NO_BUFS        No available internal buffer found.
 */
otError otPlatBleGattServerServiceRegister(otInstance *aInstance, const otPlatBleUuid *aUuid, uint16_t *aHandle);

/**
 * Registers GATT Characteristic with maximum length of 128 octets.
 *
 * @note This function shall be used only for GATT Server.
 *
 * @param[in]     aInstance       The OpenThread instance structure.
 * @param[in]     aServiceHandle  The start handle of a service.
 * @param[inout]  aChar           As an input parameter the valid mUuid and mProperties have to be provided.
 *                                In case of success, the value of mValueHandle is filled.
 * @param[in]     aCccd           If set, method has to create Client Characteristic Configuration Descriptor
 *                                and put its handle into mHandleCccd parameter of @p aChar.
 *
 * @retval ::OT_ERROR_NONE           Characteristic has been successfully registered.
 * @retval ::OT_ERROR_INVALID_STATE  BLE Device is in invalid state.
 * @retval ::OT_ERROR_INVALID_ARGS   Invalid service handle or characteristic UUID has been provided.
 * @retval ::OT_ERROR_NO_BUFS        No available internal buffer found.
 */
otError otPlatBleGattServerCharacteristicRegister(otInstance *                 aInstance,
                                                  uint16_t                     aServiceHandle,
                                                  otPlatBleGattCharacteristic *aChar,
                                                  bool                         aCccd);

/**
 * Sends ATT Handle Value Indication.
 *
 * @note This function shall be used only for GATT Server.
 *
 * @param[in] aInstance   The OpenThread instance structure.
 * @param[in] aHandle     The handle of the attribute to be indicated.
 * @param[in] aPacket     A pointer to the packet contains value to be indicated.
 *
 * @retval ::OT_ERROR_NONE           ATT Handle Value Indication has been sent.
 * @retval ::OT_ERROR_INVALID_STATE  BLE Device is in invalid state.
 * @retval ::OT_ERROR_INVALID_ARGS   Invalid handle value, data or data length has been supplied.
 * @retval ::OT_ERROR_NO_BUFS        No available internal buffer found.
 */
otError otPlatBleGattServerIndicate(otInstance *aInstance, uint16_t aHandle, otBleRadioPacket *aPacket);

/**
 * The BLE driver calls this method to notify OpenThread that an ATT Handle
 * Value Confirmation has been received.
 *
 * This method is called only if @p otPlatBleGattServerIndicate was previously requested.
 *
 * @note This function shall be used only for GATT Server.
 *
 * @param[in] aInstance  The OpenThread instance structure.
 * @param[in] aHandle    The handle on which ATT Handle Value Confirmation has been sent.
 *
 */
extern void otPlatBleGattServerOnIndicationConfirmation(otInstance *aInstance, uint16_t aHandle);

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
extern void otPlatBleGattServerOnWriteRequest(otInstance *aInstance, uint16_t aHandle, otBleRadioPacket *aPacket);

/**
 * The BLE driver calls this method to notify OpenThread that an ATT Subscription
 * Request packet has been received.
 *
 * @note This function shall be used only for GATT Server.
 *
 * @param[in] aInstance    The OpenThread instance structure.
 * @param[in] aHandle      The handle of the attribute to be written.
 * @param[in] aSubscribing True if subscribing, otherwise unsubscribing.
 *
 */
extern void otPlatBleGattServerOnSubscribeRequest(otInstance *aInstance, uint16_t aHandle, bool aSubscribing);

/**
 * @}
 *
 */

#ifdef __cplusplus
} // end of extern "C"
#endif

#endif // OT_PLATFORM_BLE_H_
