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
 *   This file defines a generic OpenThread BLE driver HCI interface.
 */

#ifndef _PLATFORM_BLE_HCI_H_
#define _PLATFORM_BLE_HCI_H_

// TODO: Move to platform/posix
#include <sys/uio.h>

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <openthread/error.h>
#include <openthread/instance.h>


/**
 * Initialize the Host Controller Interface for Bluetooth Low Energy radio.
 *
 * @param[in] aInstance  The OpenThread instance structure.
 */
void otPlatBleHciInit(otInstance *aInstance);

 /**
  * Returns the HCI interface id for an OpenThread instance.
  *
  * @param[in] aInstance  The OpenThread instance structure.
  *
  * @retval   DeviceId for given OpenThread instance.
  */
int otPlatBleHciGetDeviceId(otInstance *aInstance);

/**
 * Sets the HCI interface id for an OpenThread instance.
 *
 * @param[in] aInstance  The OpenThread instance structure.
 * @param[in] aDevice    The interface identifier for the given OpenThread instance.
 */
void otPlatBleHciSetDeviceId(otInstance *aInstance, int aDeviceId);

/**
 * Reads a packet from the HCI interface.
 *
 * @param[in] aInstance  The OpenThread instance structure.
 * @param[in] aPacket    Buffer to read HCI packet to.
 * @param[in] aLength    Length of read buffer.
 *
 * @retval ::OT_ERROR_NONE         Successfully enabled.
 */
int otPlatBleHciRead(otInstance *aInstance, uint8_t *aPacket, uint16_t aLength);

/**
 * Signals that an HCI packet has been read from the HCI interface.
 *
 * @param[in] aInstance  The OpenThread instance structure.
 * @param[in] aPacket    Pointer to HCI packet.
 * @param[in] aLength    Length of HCI packet.
 */
extern void otPlatBleHciOnRead(otInstance *aInstance,
                               uint8_t *aPacket, uint16_t aLength);

/**
 * Write the given HCI packet to the HCI interface.
 *
 * @param[in] aInstance  The OpenThread instance structure.
 * @param[in] aIoVector  HCI packet composed of an array of buffers.
 * @param[in] aIoVectorSize  Number of buffers in aIoVector structure.
 */
void otPlatBleHciWritev(otInstance *aInstance,
                        const struct iovec *aIoVector, int aIoVectorSize);

/**
 * Write an HCI command packet to the HCI interface.
 *
 * @param[in] aInstance  The OpenThread instance structure.
 * @param[in] aOgf       The OpCode Group Field.
 * @param[in] aOcf       The OpCode Command Field.
 * @param[in] aPacket    Pointer to HCI command payload.
 * @param[in] aLength    Length of HCI command payload.
 */
void otPlatBleHciWriteCmd(otInstance *aInstance,
                          uint16_t aOgf, uint16_t aOcf,
                          void *aPacket, uint16_t aLength);

/**
 * Iterate the inner state machine of the HCI engine.
 *
 * @param[in] aInstance  The OpenThread instance structure.
 */
void otPlatBleHciTick(otInstance *aInstance);

#ifdef __cplusplus
}  // end of extern "C"
#endif

#endif // _PLATFORM_BLE_HCI_
