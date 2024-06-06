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

#include <openthread/tcat.h>
#include <openthread/platform/ble.h>

otError otPlatBleEnable(otInstance *aInstance)
{
    OT_UNUSED_VARIABLE(aInstance);
    return OT_ERROR_NOT_IMPLEMENTED;
}

otError otPlatBleDisable(otInstance *aInstance)
{
    OT_UNUSED_VARIABLE(aInstance);
    return OT_ERROR_NOT_IMPLEMENTED;
}

otError otPlatBleGetAdvertisementBuffer(otInstance *aInstance, uint8_t **aAdvertisementBuffer)
{
    OT_UNUSED_VARIABLE(aInstance);
    static uint8_t sAdvertisementBuffer[OT_TCAT_ADVERTISEMENT_MAX_LEN];

    *aAdvertisementBuffer = sAdvertisementBuffer;

    return OT_ERROR_NONE;
}

otError otPlatBleGapAdvStart(otInstance *aInstance, uint16_t aInterval)
{
    OT_UNUSED_VARIABLE(aInstance);
    OT_UNUSED_VARIABLE(aInterval);
    return OT_ERROR_NOT_IMPLEMENTED;
}

otError otPlatBleGapAdvStop(otInstance *aInstance)
{
    OT_UNUSED_VARIABLE(aInstance);
    return OT_ERROR_NOT_IMPLEMENTED;
}

otError otPlatBleGapDisconnect(otInstance *aInstance)
{
    OT_UNUSED_VARIABLE(aInstance);
    return OT_ERROR_NOT_IMPLEMENTED;
}

otError otPlatBleGattMtuGet(otInstance *aInstance, uint16_t *aMtu)
{
    OT_UNUSED_VARIABLE(aInstance);
    OT_UNUSED_VARIABLE(aMtu);
    return OT_ERROR_NOT_IMPLEMENTED;
}

otError otPlatBleGattServerIndicate(otInstance *aInstance, uint16_t aHandle, const otBleRadioPacket *aPacket)
{
    OT_UNUSED_VARIABLE(aInstance);
    OT_UNUSED_VARIABLE(aHandle);
    OT_UNUSED_VARIABLE(aPacket);
    return OT_ERROR_NOT_IMPLEMENTED;
}

void otPlatBleGetLinkCapabilities(otInstance *aInstance, otBleLinkCapabilities *aBleLinkCapabilities)
{
    OT_UNUSED_VARIABLE(aInstance);

    aBleLinkCapabilities->mGattNotifications = 1;
    aBleLinkCapabilities->mL2CapDirect       = 0;
    aBleLinkCapabilities->mRsv               = 0;
}

otError otPlatBleGapAdvSetData(otInstance *aInstance, uint8_t *aAdvertisementData, uint16_t aAdvertisementLen)
{
    OT_UNUSED_VARIABLE(aInstance);
    OT_UNUSED_VARIABLE(aAdvertisementData);
    OT_UNUSED_VARIABLE(aAdvertisementLen);
    return OT_ERROR_NONE;
}

bool otPlatBleSupportsMultiRadio(otInstance *aInstance)
{
    OT_UNUSED_VARIABLE(aInstance);
    return false;
}
