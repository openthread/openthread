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
 * @addtogroup plat-ble-radio
 *
 * @brief
 *   This module includes the platform abstraction for BLE radio communication.
 *
 * @{
 *
 */

/**
 * @defgroup ble-radio-types Types
 *
 * @brief
 *   This module includes the platform abstraction for a BLE radio frame.
 *
 * @{
 *
 */

enum
{
    OT_RADIO_BLE_FRAME_MAX_SIZE = 257, ///< Maximum size of BLE frame (include PUD header and MIC).
};

/**
 * This structure represents the BLE radio settings.
 */
typedef struct otRadioBleSettings
{
    uint8_t  mChannel;       ///< Channel used to transmit/receive the frame.
    uint32_t mAccessAddress; ///< The access address.
    uint32_t mCrcInit;       ///< The CRC initial value.
} otRadioBleSettings;

/**
 * This structure represents a BLE radio frame.
 */
typedef struct otRadioBleFrame
{
    uint8_t *mPdu;    ///< The PDU.
    uint16_t mLength; ///< Length of the PDU.

    struct
    {
        uint32_t mTicks; ///< The timestamp when the first bit of the frame was received (unit: ticks).
        int8_t   mRssi;  ///< Received signal strength indicator in dBm for received frames.
    } mRxInfo;
} otRadioBleFrame;

/**
 * This structure represents the BLE time.
 */
typedef struct otRadioBleTime
{
    uint32_t mTicks;        ///< Transmit/receive tick time of the packet (unit: ticks).
    uint16_t mOffsetUs;     ///< The offset to the mTicks in microseconds.
    uint32_t mRxDurationUs; ///< Receive duration in microseconds.
} otRadioBleTime;

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
 * The following diagram shows time sequence of otPlatRadioBleReceiveAtTime() and otPlatRadioBleTransmitAtTifs():
 *
 *  ReceiveAtTime()  StartTime      ReceiveDone()->TransmitAtTifs()         TransmitDone()
 *    EnableTifs()      |                   ^        |                            ^
 *       |              |                   |        |                            |
 *       V              V                   |        V                            |
 *                         +----------------+                    +----------------+
 *                         | Receive Frame  |                    | Transmit Frame |
 * --------------------->--+----------------+<-------TIFS------->+----------------+--------------> time
 *
 */

/**
 * The following diagram shows time sequence of otPlatRadioBleTransmitAtTime() and otPlatRadioBleReceiveAtTifs():
 *
 *  TransmitAtTime() StartTime    TransmitDone()->ReceiveAtTifs()         ReceiveDone()
 *    EnableTifs()      |                ^           |                         ^
 *       |              |                |           |                         |
 *       V              V                |           V                         |
 *                      +----------------+                    +----------------+
 *                      | Transmit Frame |                    | Receive Frame  |
 * ---------------------+----------------+<--------TIFS------>+----------------+-----------------> time
 *
 */

/**
 * The following diagram shows time sequence of otPlatRadioBleTransmitAtTime() and otPlatRadioBleReceiveAtTime():
 *
 *  TransmitAtTime() StartTime    TransmitDone() ReceiveAtTime()  StartTime      ReceiveDone()
 *    DisableTifs()     |                ^        DisableTifs()      |                   ^
 *       |              |                |            |              |                   |
 *       V              V                |            V              V                   |
 *                      +----------------+                              +----------------+
 *                      | Transmit Frame |      ...                     | Receive Frame  |
 * ---------------------+----------------+-----     --------------------+----------------+------->time
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
otError otPlatRadioBleEnable(otInstance *aInstance);

/**
 * Disable the BLE radio.
 *
 * @param[in] aInstance  The OpenThread instance structure.
 *
 * @retval OT_ERROR_NONE  Successfully transitioned to Disabled.
 *
 */
otError otPlatRadioBleDisable(otInstance *aInstance);

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
uint32_t otPlatRadioBleGetTickNow(otInstance *aInstance);

/**
 * Get the BLE device's public address.
 *
 * @param[in]  aInstance   The OpenThread instance structure.
 * @param[out] aAddress    A pointer to the BLE public address.
 *
 */
void otPlatRadioBleGetPublicAddress(otInstance *aInstance, otPlatBleDeviceAddr *aAddress);

/**
 * Get the BLE radio's XTAL accuracy.
 *
 * @returns The BLE radio's XTAL accuracy, in ppm.
 *
 */
uint8_t otPlatRadioBleGetXtalAccuracy(otInstance *aInstance);

/**
 * Get the BLE radio's transmit power in dBm.
 *
 * @param[in] aInstance  The OpenThread instance structure.
 *
 * @retval  The transmit power in dBm
 *
 */
int8_t otPlatRadioBleGetTransmitPower(otInstance *aInstance);

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
otError otPlatRadioBleSetTransmitPower(otInstance *aInstance, int8_t aPower);

/**
 * Enable TIFS after the next Rx or TX operation.
 *
 * @param[in] aInstance  The OpenThread instance structure.
 *
 */
void otPlatRadioBleEnableTifs(otInstance *aInstance);

/**
 * Disable TIFS after the next Rx or TX operation.
 *
 * @param[in] aInstance  The OpenThread instance structure.
 *
 */
void otPlatRadioBleDisableTifs(otInstance *aInstance);

/**
 * Get the BLE radio transmit frame buffer.
 *
 * Ble controller forms the BLE frame in this buffer then calls `otPlatRadioBleTransmitAtTime()` or
 * `otPlatRadioBleTransmitAtTifs()` to request transmission.
 *
 * @param[in] aInstance  The OpenThread instance structure.
 *
 * @returns A pointer to the transmit frame buffer.
 *
 */
otRadioBleFrame *otPlatRadioBleGetTransmitBuffer(otInstance *aInstance);

/**
 * Transmit frame at the given time on the BLE radio.
 *
 * The caller must form the BLE frame in the buffer provided by `otPlatRadioBleGetTransmitBuffer()` before
 * requesting transmission. The channel, access address and CRC initial value are also included in the
 * otRadioBleFrame structure.
 *
 * @param[in] aInstance   The OpenThread instance structure.
 * @param[in] aSettings   A pointer to the BLE radio settings to be used.
 * @param[in] aStartTime  A pointer to the time when the transmission started.
 *
 * @retval OT_ERROR_NONE          Successfully transitioned to Transmit.
 * @retval OT_ERROR_INVALID_ARGS  @p aSettings or @p aStartTime is NULL.
 * @retval OT_ERROR_INVALID_STATE The radio was not in the idle state.
 *
 */
otError otPlatRadioBleTransmitAtTime(otInstance *              aInstance,
                                     const otRadioBleSettings *aSettings,
                                     const otRadioBleTime *    aStartTime);

/*
 * Transmit frame at TIFS after the last packet received.
 *
 * The caller must form the BLE frame in the buffer provided by `otPlatRadioBleGetTransmitBuffer()` before
 * requesting transmission. The channel, access address and CRC initial value are also included in the
 * otRadioBleFrame structure.
 *
 * If possible, the transmit will occur at the TIFS timing.
 * If not possible, function otPlatRadioBleTransmitDone() will be called to indicate this.
 *
 * @param[in] aInstance   The OpenThread instance structure.
 * @param[in] aSettings   A pointer to the BLE radio settings to be used.
 *
 * @retval OT_ERROR_NONE          Successfully transitioned to Transmit.
 * @retval OT_ERROR_INVALID_ARGS  @p aSettings is NULL.
 * @retval OT_ERROR_INVALID_STATE The radio was not in the idle state.
 */
otError otPlatRadioBleTransmitAtTifs(otInstance *aInstance, const otRadioBleSettings *aSettings);

/**
 * The BLE radio driver calls this function to notify BLE controller that the transmit operation has completed.
 *
 * @param[in]  aInstance  The OpenThread instance structure.
 * @param[in]  aError     OT_ERROR_NONE when the frame was transmitted,
 *                        OT_ERROR_FAILED when transmission was failed.
 *
 */
extern void otPlatRadioBleTransmitDone(otInstance *aInstance, otError aError);

/*
 * Receive frame within the given period.
 *
 * If possible, frames will be received within the period.
 * If not possible, the function otPlatRadioBleReceiveDone() will be called to indicate this.
 *
 * @param[in] aInstance   The OpenThread instance structure.
 * @param[in] aSettings   A pointer to the BLE radio settings to be used.
 * @param[in] aStartTime  A pointer to the time when the reception started and stoped.
 *
 * @retval OT_ERROR_NONE          Successfully transitioned to Transmit.
 * @retval OT_ERROR_INVALID_ARGS  @p aSettings or @p aStartTime is NULL.
 * @retval OT_ERROR_INVALID_STATE The radio was not in the idle state.
 */
otError otPlatRadioBleReceiveAtTime(otInstance *              aInstance,
                                    const otRadioBleSettings *aSettings,
                                    const otRadioBleTime *    aStartTime);

/*
 *  Receive frame at TIFS after the last packet transmitted.
 *
 * If possible, a frame will be received on the TIFD timing.
 * If not possible, the function otPlatRadioBleReceiveDone() will be called to indicate this.
 *
 * @param[in] aInstance   The OpenThread instance structure.
 * @param[in] aSettings   A pointer to the BLE radio settings to be used.
 *
 * @retval OT_ERROR_NONE          Successfully transitioned to Transmit.
 * @retval OT_ERROR_INVALID_ARGS  The radio was not in the Receive state.
 * @retval OT_ERROR_INVALID_STATE The radio was not in the idle state.
 */
otError otPlatRadioBleReceiveAtTifs(otInstance *aInstance, const otRadioBleSettings *aSettings);

/**
 * The BLE radio driver calls this function to notify BLE controller that a frame has been received.
 *
 * @param[in]  aInstance  The OpenThread instance structure.
 * @param[in]  aFrame     A pointer to received frame or NULL if no frame was received.
 * @param[in]  aError     OT_ERROR_NONE when successfully received a frame,
 *                        OT_ERROR_FAILED when no frame was received or a broken frame was received,
 *
 */
extern void otPlatRadioBleReceiveDone(otInstance *aInstance, otRadioBleFrame *aFrame, otError aError);

/*
 * Cancel a pending transmit or receive.
 *
 * @param[in]  aInstance  The OpenThread instance structure.
 *
 * @retval OT_ERROR_NONE          Successfully cancel the pending transmit or receive.
 * @retval OT_ERROR_INVALID_STATE The radio was not in the pending state.
 */
otError otPlatRadioBleCancelData(otInstance *aInstance);

/*
 * Cancel TIFS timer.
 *
 * @param[in]  aInstance  The OpenThread instance structure.
 *
 * @retval OT_ERROR_NONE          Successfully cancel the TIFS timer.
 * @retval OT_ERROR_INVALID_STATE The radio was not in the pending state.
 */
otError otPlatRadioBleCancelTifs(otInstance *aInstance);

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
