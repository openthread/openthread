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
 *   This defines the eventing mechanism between NimBLE host task
 *   and the main openthread task.
 */

#ifdef __cplusplus
extern "C" {
#endif

#include "platform-posix.h"
#include <sched.h>

#include <common/logging.hpp>

#include "ble_event.h"

#define PIPE_READ 0
#define PIPE_WRITE 1

static int sPipeFd[2]; ///< file descriptors for waking ot

void platformBleInit()
{
    int err = pipe2(sPipeFd, O_NONBLOCK);
    if (err)
    {
        fprintf(stderr, "ERROR: platformBleInit unable to open pipe.\n");
        exit(EXIT_FAILURE);
    }
}

void platformBleProcess(otInstance *aInstance)
{
    OT_UNUSED_VARIABLE(aInstance);

    const int     flags  = POLLIN | POLLRDNORM | POLLERR | POLLNVAL | POLLHUP;
    struct pollfd pollfd = {sPipeFd[PIPE_READ], flags, 0};

    if (poll(&pollfd, 1, 0) > 0 && (pollfd.revents & flags) != 0)
    {
        int       len = 1;
        BleEvent *event;
        while (len > 0)
        {
            len = read(sPipeFd[PIPE_READ], &event, sizeof(event));
            if (len > 0)
            {
                event->Dispatch();
                delete event;
            }
        }
    }
}

void platformBleSignal(BleEvent *event)
{
    ssize_t count;
    count = write(sPipeFd[PIPE_WRITE], &event, sizeof(event));
    OT_UNUSED_VARIABLE(count);
    sched_yield();
}

void platformBleUpdateFdSet(fd_set *aReadFdSet, int *aMaxFd)
{
    if (aReadFdSet != NULL && sPipeFd[PIPE_READ])
    {
        FD_SET(sPipeFd[PIPE_READ], aReadFdSet);

        if (aMaxFd != NULL && *aMaxFd < sPipeFd[PIPE_READ])
        {
            *aMaxFd = sPipeFd[PIPE_READ];
        }
    }
}

// ==================================================================
//             EVENT DISPATCH
// ==================================================================

BleEvent *event_otPlatBleGapOnConnected(otInstance *aInstance, uint16_t aConnectionId)
{
    return new BleEventGapOnConnected(aInstance, aConnectionId);
}

BleEvent *event_otPlatBleGapOnDisconnected(otInstance *aInstance, uint16_t aConnectionId)
{
    return new BleEventGapOnDisconnected(aInstance, aConnectionId);
}

BleEvent *event_otPlatBleGapOnAdvReceived(otInstance *         aInstance,
                                          otPlatBleDeviceAddr *aAddress,
                                          otBleRadioPacket *   aPacket)
{
    return new BleEventGapOnAdvReceived(aInstance, aAddress, aPacket);
}

BleEvent *event_otPlatBleGapOnScanRespReceived(otInstance *         aInstance,
                                               otPlatBleDeviceAddr *aAddress,
                                               otBleRadioPacket *   aPacket)
{
    return new BleEventGapOnScanRespReceived(aInstance, aAddress, aPacket);
}

BleEvent *event_otPlatBleGattClientOnReadResponse(otInstance *aInstance, otBleRadioPacket *aPacket)
{
    return new BleEventGattClientOnReadResponse(aInstance, aPacket);
}

BleEvent *event_otPlatBleGattClientOnWriteResponse(otInstance *aInstance, uint16_t aHandle)
{
    return new BleEventGattClientOnWriteResponse(aInstance, aHandle);
}

BleEvent *event_otPlatBleGattClientOnIndication(otInstance *aInstance, uint16_t aHandle, otBleRadioPacket *aPacket)
{
    return new BleEventGattClientOnIndication(aInstance, aHandle, aPacket);
}

BleEvent *event_otPlatBleGattClientOnSubscribeResponse(otInstance *aInstance, uint16_t aHandle)
{
    return new BleEventGattClientOnSubscribeResponse(aInstance, aHandle);
}

BleEvent *event_otPlatBleGattClientOnServiceDiscovered(otInstance *aInstance,
                                                       uint16_t    aStartHandle,
                                                       uint16_t    aEndHandle,
                                                       uint16_t    aServiceUuid,
                                                       otError     aError)
{
    return new BleEventGattClientOnServiceDiscovered(aInstance, aStartHandle, aEndHandle, aServiceUuid, aError);
}

BleEvent *event_otPlatBleGattClientOnCharacteristicsDiscoverDone(otInstance *                 aInstance,
                                                                 otPlatBleGattCharacteristic *aChars,
                                                                 uint16_t                     aCount,
                                                                 otError                      aError)
{
    return new BleEventGattClientOnCharacteristicsDiscoverDone(aInstance, aChars, aCount, aError);
}

BleEvent *event_otPlatBleGattClientOnDescriptorsDiscoverDone(otInstance *             aInstance,
                                                             otPlatBleGattDescriptor *aDescs,
                                                             uint16_t                 aCount,
                                                             otError                  aError)
{
    return new BleEventGattClientOnDescriptorsDiscoverDone(aInstance, aDescs, aCount, aError);
}

BleEvent *event_otPlatBleGattClientOnMtuExchangeResponse(otInstance *aInstance, uint16_t aMtu, otError aError)
{
    return new BleEventGattClientOnMtuExchangeResponse(aInstance, aMtu, aError);
}

BleEvent *event_otPlatBleGattServerOnReadRequest(otInstance *aInstance, uint16_t aHandle, otBleRadioPacket *aPacket)
{
    return new BleEventGattServerOnReadRequest(aInstance, aHandle, aPacket);
}

BleEvent *event_otPlatBleGattServerOnWriteRequest(otInstance *aInstance, uint16_t aHandle, otBleRadioPacket *aPacket)
{
    return new BleEventGattServerOnWriteRequest(aInstance, aHandle, aPacket);
}

BleEvent *event_otPlatBleGattServerOnSubscribeRequest(otInstance *aInstance, uint16_t aHandle, bool aSubscribing)
{
    return new BleEventGattServerOnSubscribeRequest(aInstance, aHandle, aSubscribing);
}

BleEvent *event_otPlatBleGattServerOnIndicationConfirmation(otInstance *aInstance, uint16_t aHandle)
{
    return new BleEventGattServerOnIndicationConfirmation(aInstance, aHandle);
}

BleEvent *event_otPlatBleL2capOnDisconnect(otInstance *aInstance, uint16_t aLocalCid, uint16_t aPeerCid)
{
    return new BleEventL2capOnDisconnect(aInstance, aLocalCid, aPeerCid);
}

BleEvent *event_otPlatBleL2capOnConnectionRequest(otInstance *aInstance,
                                                  uint16_t    aPsm,
                                                  uint16_t    aMtu,
                                                  uint16_t    aPeerCid)
{
    return new BleEventL2capOnConnectionRequest(aInstance, aPsm, aMtu, aPeerCid);
}

BleEvent *event_otPlatBleL2capOnConnectionResponse(otInstance *        aInstance,
                                                   otPlatBleL2capError aResult,
                                                   uint16_t            aMtu,
                                                   uint16_t            aPeerCid)
{
    return new BleEventL2capOnConnectionResponse(aInstance, aResult, aMtu, aPeerCid);
    ;
}

BleEvent *event_otPlatBleL2capOnSduReceived(otInstance *      aInstance,
                                            uint16_t          aLocalCid,
                                            uint16_t          aPeerCid,
                                            otBleRadioPacket *aPacket)
{
    return new BleEventL2capOnSduReceived(aInstance, aLocalCid, aPeerCid, aPacket);
}

BleEvent *event_otPlatBleL2capOnSduSent(otInstance *aInstance)
{
    return new BleEventL2capOnSduSent(aInstance);
}

#ifdef __cplusplus
}
#endif
