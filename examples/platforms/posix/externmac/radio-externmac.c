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
 *   This file implements the OpenThread platform abstraction for radio
 *   communication.
 *
 */

#include "platform-posix.h"

#if OPENTHREAD_CONFIG_USE_EXTERNAL_MAC

#include <openthread/platform/radio-mac.h>

otError otPlatMlmeGet(otInstance *aInstance, otPibAttr aAttr, uint8_t aIndex, uint8_t *aLen, uint8_t *aBuf)
{
    OT_UNUSED_VARIABLE(aInstance);
    OT_UNUSED_VARIABLE(aAttr);
    OT_UNUSED_VARIABLE(aIndex);
    OT_UNUSED_VARIABLE(aLen);
    OT_UNUSED_VARIABLE(aBuf);

    return OT_ERROR_NOT_IMPLEMENTED;
}

otError otPlatMlmeSet(otInstance *aInstance, otPibAttr aAttr, uint8_t aIndex, uint8_t aLen, const uint8_t *aBuf)
{
    OT_UNUSED_VARIABLE(aInstance);
    OT_UNUSED_VARIABLE(aAttr);
    OT_UNUSED_VARIABLE(aIndex);
    OT_UNUSED_VARIABLE(aLen);
    OT_UNUSED_VARIABLE(aBuf);

    return OT_ERROR_NOT_IMPLEMENTED;
}

otError otPlatMlmeReset(otInstance *aInstance, bool setDefaultPib)
{
    OT_UNUSED_VARIABLE(aInstance);
    OT_UNUSED_VARIABLE(setDefaultPib);

    return OT_ERROR_NOT_IMPLEMENTED;
}

otError otPlatMlmeStart(otInstance *aInstance, otStartRequest *aStartReq)
{
    OT_UNUSED_VARIABLE(aInstance);
    OT_UNUSED_VARIABLE(aStartReq);

    return OT_ERROR_NOT_IMPLEMENTED;
}

otError otPlatMlmeScan(otInstance *aInstance, otScanRequest *aScanRequest)
{
    OT_UNUSED_VARIABLE(aScanRequest);

    otPlatMlmeScanConfirm(aInstance, NULL);

    return OT_ERROR_NOT_IMPLEMENTED;
}

otError otPlatMlmePollRequest(otInstance *aInstance, otPollRequest *aPollRequest)
{
    OT_UNUSED_VARIABLE(aInstance);
    OT_UNUSED_VARIABLE(aPollRequest);

    return OT_ERROR_NOT_IMPLEMENTED;
}

otError otPlatMcpsDataRequest(otInstance *aInstance, otDataRequest *aDataRequest)
{
    otPlatMcpsDataConfirm(aInstance, aDataRequest->mMsduHandle, OT_ERROR_NOT_IMPLEMENTED);

    return OT_ERROR_NOT_IMPLEMENTED;
}

otError otPlatMcpsPurge(otInstance *aInstance, uint8_t aMsduHandle)
{
    OT_UNUSED_VARIABLE(aInstance);
    OT_UNUSED_VARIABLE(aMsduHandle);

    return OT_ERROR_NOT_IMPLEMENTED;
}

void otPlatRadioGetIeeeEui64(otInstance *aInstance, uint8_t *aIeeeEui64)
{
    memset(aIeeeEui64, 0, 8);

    (void)aInstance;
}

int8_t otPlatRadioGetReceiveSensitivity(otInstance *aInstance)
{
    OT_UNUSED_VARIABLE(aInstance);

    return -100;
}

void platformRadioInit(void)
{
}

void platformRadioDeinit(void)
{
}

int8_t otPlatRadioGetRssi(otInstance *aInstance)
{
    OT_UNUSED_VARIABLE(aInstance);

    return 0;
}

otError otPlatRadioEnable(otInstance *aInstance)
{
    OT_UNUSED_VARIABLE(aInstance);

    return OT_ERROR_NOT_IMPLEMENTED;
}

bool otPlatRadioIsEnabled(otInstance *aInstance)
{
    OT_UNUSED_VARIABLE(aInstance);

    return 0;
}

void platformRadioProcess(otInstance *aInstance)
{
    OT_UNUSED_VARIABLE(aInstance);
}

void platformRadioUpdateFdSet(fd_set *aReadFdSet, fd_set *aWriteFdSet, int *aMaxFd)
{
    OT_UNUSED_VARIABLE(aReadFdSet);
    OT_UNUSED_VARIABLE(aWriteFdSet);
    OT_UNUSED_VARIABLE(aMaxFd);
}

#endif // OPENTHREAD_POSIX_EXTERNMAC
