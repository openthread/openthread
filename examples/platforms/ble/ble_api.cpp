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

//---------------------------GAP---------------------------//

otError otPlatBleGapAddressGet(otInstance *aInstance, otPlatBleDeviceAddr *aAddress);
otError otPlatBleGapAddressSet(otInstance *aInstance, const otPlatBleDeviceAddr *aAddress);
otError otPlatBleGapServiceSet(otInstance *aInstance, const char *aDeviceName, uint16_t aAppearance);
otError otPlatBleGapAdvDataSet(otInstance *aInstance, const uint8_t *aAdvData, uint8_t aAdvDataLength);
otError otPlatBleGapScanResponseSet(otInstance *aInstance, const uint8_t *aScanResponse, uint8_t aScanResponseLength);
otError otPlatBleGapAdvStart(otInstance *aInstance, uint16_t aInterval, uint8_t aType);
otError otPlatBleGapAdvStop(otInstance *aInstance);
otError otPlatBleGapScanStart(otInstance *aInstance, uint16_t aInterval, uint16_t aWindow);
otError otPlatBleGapScanStop(otInstance *aInstance);

otError otPlatBleGapConnParamsSet(otInstance *aInstance, const otPlatBleGapConnParams *aConnParams)
{

}

otError otPlatBleGapConnect(otInstance *aInstance, otPlatBleDeviceAddr *aAddress, uint16_t aInterval, uint16_t aWindow);
otError otPlatBleGapDisconnect(otInstance *aInstance);
