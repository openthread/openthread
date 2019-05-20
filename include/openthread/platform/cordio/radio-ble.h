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
 *   This file defines the BLE radio interface for OpenThread.
 *
 */

#ifndef OPENTHREAD_PLATFORM_RADIO_BLE_H_
#define OPENTHREAD_PLATFORM_RADIO_BLE_H_

#include <stdint.h>

#include <openthread/error.h>
#include <openthread/instance.h>
#include <openthread/platform/ble.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @addtogroup plat-cordio-radio
 *
 * @brief
 *   This module includes the platform abstraction for Cordio BLE radio communication.
 *
 * @{
 *
 */

/**
 * @defgroup cordio-radio-types Types
 *
 * @brief
 *   This module includes the platform abstraction for a BLE radio frame.
 *
 * @{
 *
 */

enum
{
    OT_RADIO_BLE_FRAME_MAX_SIZE = 257, ///< Maximum size of BLE frame (include PUD header).
};

/**
 * This structure represents BLE radio channelization parameters.
 */
typedef struct otRadioBleChannelParams
{
    uint8_t  mChannel;       ///< Channel used to transmit/receive the frame.
    uint32_t mAccessAddress; ///< Access address.
    uint32_t mCrcInit;       ///< CRC initial value.
} otRadioBleChannelParams;

/**
 * This structure represents buffer descriptor.
 */
typedef struct otRadioBleBufferDescriptor
{
    uint8_t *mBuffer; ///< Pointer to buffer.
    uint16_t mLength; ///< Length of buffer.
} otRadioBleBufferDescriptor;

/**
 * This structure represents representing BLE radio frame receive information
 */
typedef struct otRadioBleRxInfo
{
    uint32_t mTicks; ///< The timestamp when the first bit of the frame was received (unit: ticks).
    int8_t   mRssi;  ///< Received signal strength indicator in dBm for received frames.
} otRadioBleRxInfo;

/**
 * This structure represents BLE time.
 */
typedef struct otRadioBleTime
{
    uint32_t mTicks;        ///< Transmit/receive tick time of the packet (unit: ticks).
    uint16_t mOffsetUs;     ///< The offset to the mTicks in microseconds.
    uint32_t mRxDurationUs; ///< Receive duration in microseconds.
} otRadioBleTime;

/**
 * This enumeration defines the error code of the BLE radio.
 *
 */
typedef enum
{
    OT_BLE_RADIO_ERROR_NONE,
    OT_BLE_RADIO_ERROR_CRC,
    OT_BLE_RADIO_ERROR_RX_TIMEOUT,
    OT_BLE_RADIO_ERROR_FAILED,
} otRadioBleError;

/**
 * This structure represents the state of a BLE radio.
 * Initially, a radio is in the Disabled state.
 */
enum
{
    OT_BLE_RADIO_STATE_DISABLED,
    OT_BLE_RADIO_STATE_IDLE,
    OT_BLE_RADIO_STATE_WAITING_TRANSMIT,
    OT_BLE_RADIO_STATE_WAITING_TRANSMIT_TIFS,
    OT_BLE_RADIO_STATE_TRANSMIT,
    OT_BLE_RADIO_STATE_WAITING_RECEIVE,
    OT_BLE_RADIO_STATE_WAITING_RECEIVE_TIFS,
    OT_BLE_RADIO_STATE_RECEIVE,
};

/*
 * The following are valid BLE radio state transitions:
 *                                                 CancelData()
 *                                      +--------------------------------------------------->+
 *                                      |          TifsDisabled                              |
 *                                      |         +----------------------------------------->+
 *                                      |         |                  CancelTifs() or Timeout |
 *                                      |         |                             +----------->|
 * +----+                   +---------+ |  +----+ |            +--------------+ |  +----+    |  +----+
 * |    | TransmitAtTime()  |         | |  |    | |TifsEnabled |              | |  |    |    V  |    |
 * |IDLE| ----------------> | WAIT_TX |--->| TX |------------->| WAIT_RX_TIFS |--->| RX |------>|IDLE|
 * |    |                   |         |    |    |              |              |    |    |       |    |
 * +----+                   +---------+    +----+              +--------------+    +----+       +----+
 *
 *
 *
 *                                                 CancelData() or Timeout
 *                                      +--------------------------------------------------->+
 *                                      |          TifsDisabled                              |
 *                                      |         +----------------------------------------->+
 *                                      |         |                             CancelTifs() |
 *                                      |         |                             +----------->|
 * +----+                   +---------+ |  +----+ |            +--------------+ |  +----+    |  +----+
 * |    | ReceiveAtTime()   |         | |  |    | |TifsEnabled |              | |  |    |    V  |    |
 * |IDLE| ----------------> | WAIT_RX |--->| RX |------------->| WAIT_TX_TIFS |--->| TX |------>|IDLE|
 * |    |                   |         |    |    |              |              |    |    |       |    |
 * +----+                   +---------+    +----+              +--------------+    +----+       +----+
 *
 * +--------+ Enable() +----+
 * |        |--------->|    |
 * |DISABLED|          |IDLE|
 * |        |<-------- |    |
 * +--------+ Disable()+----+
 */

/**
 * The following diagram shows time sequence of otCordioPlatRadioReceiveAtTime() and otCordioPlatRadioTransmitAtTifs():
 *
 *    EnableTifs()  StartTime      ReceiveDone()->TransmitAtTifs()         TransmitDone()
 *  ReceiveAtTime()     |                   ^        |                            ^
 *       |              |                   |        |                            |
 *       V              V                   |        V                            |
 *                         +----------------+                    +----------------+
 *                         | Receive Frame  |                    | Transmit Frame |
 * --------------------->--+----------------+<-------TIFS------->+----------------+--------------> time
 *
 */

/**
 * The following diagram shows time sequence of otCordioPlatRadioTransmitAtTime() and otCordioPlatRadioReceiveAtTifs():
 *
 *    EnableTifs()   StartTime    TransmitDone()->ReceiveAtTifs()         ReceiveDone()
 *  TransmitAtTime()    |                ^           |                         ^
 *       |              |                |           |                         |
 *       V              V                |           V                         |
 *                      +----------------+                    +----------------+
 *                      | Transmit Frame |                    | Receive Frame  |
 * ---------------------+----------------+<--------TIFS------>+----------------+-----------------> time
 *
 */

/**
 * The following diagram shows time sequence of otCordioPlatRadioTransmitAtTime() and otCordioPlatRadioReceiveAtTime() when
 * TIFS timer is disabled:
 *
 *   DisableTifs()   StartTime    TransmitDone()  DisableTifs()  StartTime          ReceiveDone()
 *  TransmitAtTime()    |                ^       ReceiveAtTime()     |                   ^
 *       |              |                |            |              |                   |
 *       V              V                |            V              V                   |
 *                      +----------------+                              +----------------+
 *                      | Transmit Frame |      ...                     | Receive Frame  |
 * ---------------------+----------------+-----     --------------------+----------------+-------> time
 *
 */

/**
 * Enable the BLE radio.
 *
 * @param[in] aInstance  The OpenThread instance structure.
 *
 * @retval OT_ERROR_NONE     Successfully enabled.
 * @retval OT_ERROR_FAILED   The radio could not be enabled.
 *
 */
otError otCordioPlatRadioEnable(otInstance *aInstance);

/**
 * Disable the BLE radio.
 *
 * @param[in] aInstance  The OpenThread instance structure.
 *
 * @retval OT_ERROR_NONE  Successfully transitioned to Disabled.
 *
 */
otError otCordioPlatRadioDisable(otInstance *aInstance);

/**
 * Get the current BLE Radio tick value.
 *
 * The clock should increment at the rate OPENTHREAD_CONFIG_BLE_BB_CLK_RATE_HZ
 * (wrapping as appropriate) whenever the radio is enabled.
 *
 * @param[in] aInstance  The OpenThread instance structure.
 *
 * @retval The current BLE radio tick value.
 *
 */
uint32_t otCordioPlatRadioGetTickNow(otInstance *aInstance);

/**
 * Get the BLE device's public address.
 *
 * @param[in]  aInstance   The OpenThread instance structure.
 * @param[out] aAddress    A pointer to the BLE public address.
 *
 */
void otCordioPlatRadioGetPublicAddress(otInstance *aInstance, otPlatBleDeviceAddr *aAddress);

/**
 * Get the BLE radio's XTAL accuracy.
 *
 * @returns The BLE radio's XTAL accuracy, in ppm.
 *
 */
uint8_t otCordioPlatRadioGetXtalAccuracy(otInstance *aInstance);

/**
 * Get the BLE radio's transmit power in dBm.
 *
 * @param[in] aInstance  The OpenThread instance structure.
 *
 * @retval  The transmit power in dBm
 *
 */
int8_t otCordioPlatRadioGetTransmitPower(otInstance *aInstance);

/**
 * Set the BLE radio's transmit power in dBm.
 *
 * @param[in] aInstance  The OpenThread instance structure.
 * @param[in] aPower     The transmit power in dBm.
 *
 * @retval OT_ERROR_NONE          Successfully set the transmit power.
 * @retval OT_ERROR_INVALID_ARGS  @p aPower is not supported.
 *
 */
otError otCordioPlatRadioSetTransmitPower(otInstance *aInstance, int8_t aPower);

/**
 * Set the BLE radio channelization parameters.
 *
 * @param[in] aInstance        The OpenThread instance structure.
 * @param[in] aChannelParams   A pointer to the channelization parameters.
 *
 * @retval OT_ERROR_NONE          Successfully set the parameters.
 * @retval OT_ERROR_INVALID_ARGS  @p aChannelParams is not supported or @p aChannelParams is NULL.
 *
 */
otError otCordioPlatRadioSetChannelParameters(otInstance *aInstance, const otRadioBleChannelParams *aChannelParams);

/**
 * Enable TIFS timer after the next receive or transmit operation.
 *
 * @param[in] aInstance  The OpenThread instance structure.
 *
 */
void otCordioPlatRadioEnableTifs(otInstance *aInstance);

/**
 * Disable TIFS timer after the next receive or transmit operation.
 *
 * @param[in] aInstance  The OpenThread instance structure.
 *
 */
void otCordioPlatRadioDisableTifs(otInstance *aInstance);

/**
 * Transmit frame at the given time on the BLE radio.
 *
 * @param[in] aInstance               The OpenThread instance structure.
 * @param[in] aBufferDescriptors      A pointer to the sending buffer descriptors.
 * @param[in] aNumBufferDescriptors   The number of buffer descriptors.
 * @param[in] aStartTime              A pointer to the time when the transmission started.
 *
 * @retval OT_ERROR_NONE          Successfully set the transmission timer and wait to transmit frame.
 * @retval OT_ERROR_INVALID_ARGS  @p aBufferDescriptors or @p aStartTime is NULL.
 * @retval OT_ERROR_INVALID_STATE The radio was not in IDLE state.
 *
 */
otError otCordioPlatRadioTransmitAtTime(otInstance *                aInstance,
                                     otRadioBleBufferDescriptor *aBufferDescriptors,
                                     uint8_t                     aNumBufferDescriptors,
                                     const otRadioBleTime *      aStartTime);

/*
 * Transmit frame at TIFS after the last packet received.
 *
 * @note If possible, the transmission will occur at the TIFS timing. If not possible, function
 *       otCordioPlatRadioTransmitDone() will be called to indicate this.
 *
 * @param[in] aInstance               The OpenThread instance structure.
 * @param[in] aBufferDescriptors      A pointer to the sending buffer descriptors.
 * @param[in] aNumBufferDescriptors   The number of buffer descriptors.
 *
 * @retval OT_ERROR_NONE          Successfully set the transmission frame and wait to transmit frame.
 * @retval OT_ERROR_INVALID_ARGS  @p aBufferDescriptors or @p aStartTime is NULL.
 * @retval OT_ERROR_INVALID_STATE The radio was not in WAITING_TRANSMIT_TIFS state.
 */
otError otCordioPlatRadioTransmitAtTifs(otInstance *                aInstance,
                                     otRadioBleBufferDescriptor *aBufferDescriptors,
                                     uint8_t                     aNumBufferDescriptors);

/**
 * The BLE radio driver calls this function to notify BLE controller that the transmit operation has completed.
 *
 * @param[in]  aInstance  The OpenThread instance structure.
 * @param[in]  aError     OT_BLE_RADIO_ERROR_NONE when the frame was successfully transmitted.
 *                        OT_BLE_RADIO_ERROR_FAILED when transmission was failed.
 *
 */
extern void otCordioPlatRadioTransmitDone(otInstance *aInstance, otRadioBleError aError);

/*
 * Receive frame within the given period.
 *
 * @note If possible, frames will be received within the period.  If not possible, the function
 *       otCordioPlatRadioReceiveDone() will be called to indicate this.
 *
 * @param[in] aInstance           The OpenThread instance structure.
 * @param[in] aBufferDescriptor   A pointer to a buffer descriptor used to store the received frame.
 * @param[in] aStartTime          A pointer to the time structure which represents the start time and end time of
 *                                the reception.
 *
 * @retval OT_ERROR_NONE          Successfully set the reception timer and wait to receive frame.
 * @retval OT_ERROR_INVALID_ARGS  @p aBufferDescriptor or @p aStartTime is NULL.
 * @retval OT_ERROR_INVALID_STATE The radio was not in IDLE state.
 */
otError otCordioPlatRadioReceiveAtTime(otInstance *                aInstance,
                                    otRadioBleBufferDescriptor *aBufferDescriptor,
                                    const otRadioBleTime *      aStartTime);

/*
 *  Receive frame at TIFS after the last packet transmitted.
 *
 * @note If possible, a frame will be received on the TIFS timing. If not possible, the function
 *       otCordioPlatRadioReceiveDone() will be called to indicate this.
 *
 * @param[in] aInstance           The OpenThread instance structure.
 * @param[in] aBufferDescriptor   A pointer to a buffer descriptor used to store the received frame.
 *
 * @retval OT_ERROR_NONE          Successfully set the reception frame buffer and wait to receive frame.
 * @retval OT_ERROR_INVALID_ARGS  @p aBufferDescriptor is NULL.
 * @retval OT_ERROR_INVALID_STATE The radio was not in WAITING_RECEIVE_TIFS state.
 */
otError otCordioPlatRadioReceiveAtTifs(otInstance *aInstance, otRadioBleBufferDescriptor *aBufferDescriptor);

/**
 * The BLE radio driver calls this function to notify BLE controller that a frame has been received.
 *
 * @param[in]  aInstance  The OpenThread instance structure.
 * @param[in]  aRxInfo    A pointer to the received frame information or NULL if an error frame was received.
 * @param[in]  aError     OT_BLE_RADIO_ERROR_NONE when successfully received a frame,
 *                        OT_BLE_RADIO_ERROR_CRC when received a frame with error CRC.
 *                        OT_BLE_RADIO_ERROR_RX_TIMEOUT when no frame was received.
 *                        OT_BLE_RADIO_ERROR_FAILED when reception was failed.
 *
 */
extern void otCordioPlatRadioReceiveDone(otInstance *aInstance, otRadioBleRxInfo *aRxInfo, otRadioBleError aError);

/*
 * Cancel a pending transmit or receive when the radio is in WAIT_TRANSMIT or WAIT_RECEIVE state.
 *
 * @param[in]  aInstance  The OpenThread instance structure.
 *
 */
void otCordioPlatRadioCancelData(otInstance *aInstance);

/*
 * Cancel TIFS timer when the radio is in WAIT_TRANSMIT_TIFS or WAIT_RECEIVE_TITS state.
 *
 * @param[in]  aInstance  The OpenThread instance structure.
 *
 */
void otCordioPlatRadioCancelTifs(otInstance *aInstance);

/*
 * Enable BLE radio driver interrupt.
 *
 */
void otCordioPlatRadioEnableInterrupt(void);

/*
 * Disable BLE radio driver interrupt.
 *
 */
void otCordioPlatRadioDisableInterrupt(void);

/***
 * @}
 *
 */

/**
 * @}
 *
 */

#ifdef __cplusplus
} // end of extern "C"
#endif

#endif // OPENTHREAD_PLATFORM_RADIO_BLE_H_
