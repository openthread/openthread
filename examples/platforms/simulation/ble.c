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

#include "platform-simulation.h"

#include <errno.h>

#include <stdio.h>
#include <stdlib.h>

#include <openthread/error.h>
#include <openthread/tcat.h>
#include <openthread/platform/ble.h>

#include "utils/code_utils.h"

#define PLAT_BLE_MSG_DATA_MAX 2048
static uint8_t sBleBuffer[PLAT_BLE_MSG_DATA_MAX];

static int sFd = -1;

static const uint16_t kPortBase = 10000;
static uint16_t       sPort     = 0;
struct sockaddr_in    sSockaddr;

static void initFds(void)
{
    int                fd;
    int                one = 1;
    struct sockaddr_in sockaddr;

    memset(&sockaddr, 0, sizeof(sockaddr));

    sPort                    = (uint16_t)(kPortBase + gNodeId);
    sockaddr.sin_family      = AF_INET;
    sockaddr.sin_port        = htons(sPort);
    sockaddr.sin_addr.s_addr = inet_addr("127.0.0.1");

    otEXPECT_ACTION((fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) != -1, perror("socket(sFd)"));

    otEXPECT_ACTION(setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(one)) != -1,
                    perror("setsockopt(sFd, SO_REUSEADDR)"));
    otEXPECT_ACTION(setsockopt(fd, SOL_SOCKET, SO_REUSEPORT, &one, sizeof(one)) != -1,
                    perror("setsockopt(sFd, SO_REUSEPORT)"));

    otEXPECT_ACTION(bind(fd, (struct sockaddr *)&sockaddr, sizeof(sockaddr)) != -1, perror("bind(sFd)"));

    // Fd is successfully initialized.
    sFd = fd;

exit:
    if (sFd == -1)
    {
        exit(EXIT_FAILURE);
    }
}

static void deinitFds(void)
{
    if (sFd != -1)
    {
        close(sFd);
        sFd = -1;
    }
}

otError otPlatBleGetAdvertisementBuffer(otInstance *aInstance, uint8_t **aAdvertisementBuffer)
{
    OT_UNUSED_VARIABLE(aInstance);
    static uint8_t sAdvertisementBuffer[OT_TCAT_ADVERTISEMENT_MAX_LEN];

    *aAdvertisementBuffer = sAdvertisementBuffer;

    return OT_ERROR_NONE;
}

otError otPlatBleEnable(otInstance *aInstance)
{
    OT_UNUSED_VARIABLE(aInstance);
    initFds();
    return OT_ERROR_NONE;
}

otError otPlatBleDisable(otInstance *aInstance)
{
    deinitFds();
    OT_UNUSED_VARIABLE(aInstance);
    return OT_ERROR_NONE;
}

otError otPlatBleGapAdvStart(otInstance *aInstance, uint16_t aInterval)
{
    OT_UNUSED_VARIABLE(aInstance);
    OT_UNUSED_VARIABLE(aInterval);
    return OT_ERROR_NONE;
}

otError otPlatBleGapAdvStop(otInstance *aInstance)
{
    OT_UNUSED_VARIABLE(aInstance);
    return OT_ERROR_NONE;
}

otError otPlatBleGapDisconnect(otInstance *aInstance)
{
    OT_UNUSED_VARIABLE(aInstance);
    return OT_ERROR_NONE;
}

otError otPlatBleGattMtuGet(otInstance *aInstance, uint16_t *aMtu)
{
    OT_UNUSED_VARIABLE(aInstance);
    *aMtu = PLAT_BLE_MSG_DATA_MAX - 1;
    return OT_ERROR_NONE;
}

otError otPlatBleGattServerIndicate(otInstance *aInstance, uint16_t aHandle, const otBleRadioPacket *aPacket)
{
    OT_UNUSED_VARIABLE(aInstance);
    OT_UNUSED_VARIABLE(aHandle);

    ssize_t rval;
    otError error = OT_ERROR_NONE;

    otEXPECT_ACTION(sFd != -1, error = OT_ERROR_INVALID_STATE);
    rval = sendto(sFd, (const char *)aPacket->mValue, aPacket->mLength, 0, (struct sockaddr *)&sSockaddr,
                  sizeof(sSockaddr));
    if (rval == -1)
    {
        perror("BLE simulation sendto failed.");
    }

exit:
    return error;
}

void platformBleDeinit(void) { deinitFds(); }

void platformBleUpdateFdSet(fd_set *aReadFdSet, fd_set *aWriteFdSet, struct timeval *aTimeout, int *aMaxFd)
{
    OT_UNUSED_VARIABLE(aTimeout);
    OT_UNUSED_VARIABLE(aWriteFdSet);

    if (aReadFdSet != NULL && sFd != -1)
    {
        FD_SET(sFd, aReadFdSet);

        if (aMaxFd != NULL && *aMaxFd < sFd)
        {
            *aMaxFd = sFd;
        }
    }
}

void platformBleProcess(otInstance *aInstance, const fd_set *aReadFdSet, const fd_set *aWriteFdSet)
{
    OT_UNUSED_VARIABLE(aWriteFdSet);

    otEXPECT(sFd != -1);

    if (FD_ISSET(sFd, aReadFdSet))
    {
        socklen_t len = sizeof(sSockaddr);
        ssize_t   rval;
        memset(&sSockaddr, 0, sizeof(sSockaddr));
        rval = recvfrom(sFd, sBleBuffer, sizeof(sBleBuffer), 0, (struct sockaddr *)&sSockaddr, &len);
        if (rval > 0)
        {
            otBleRadioPacket myPacket;
            myPacket.mValue  = sBleBuffer;
            myPacket.mLength = (uint16_t)rval;
            myPacket.mPower  = 0;
            otPlatBleGattServerOnWriteRequest(
                aInstance, 0,
                &myPacket); // TODO consider passing otPlatBleGattServerOnWriteRequest as a callback function
        }
        else if (rval == 0)
        {
            // socket is closed, which should not happen
            assert(false);
        }
        else if (errno != EINTR && errno != EAGAIN)
        {
            perror("recvfrom BLE simulation failed");
            exit(EXIT_FAILURE);
        }
    }
exit:
    return;
}

OT_TOOL_WEAK void otPlatBleGattServerOnWriteRequest(otInstance             *aInstance,
                                                    uint16_t                aHandle,
                                                    const otBleRadioPacket *aPacket)
{
    OT_UNUSED_VARIABLE(aInstance);
    OT_UNUSED_VARIABLE(aHandle);
    OT_UNUSED_VARIABLE(aPacket);
    assert(false);
    /* In case of rcp there is a problem with linking to otPlatBleGattServerOnWriteRequest
     * which is available in FTD/MTD library.
     */
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
