/*
 *  Copyright (c) 2016, The OpenThread Authors.
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
 *   This file implements the OpenThread Link API.
 */

#include <openthread/config.h>

#include <openthread/link.h>

#include "openthread-instance.h"

using namespace ot;

static void HandleActiveScanResult(void *aContext, Mac::Frame *aFrame);
static void HandleEnergyScanResult(void *aContext, otEnergyScanResult *aResult);

uint8_t otLinkGetChannel(otInstance *aInstance)
{
    return aInstance->mThreadNetif.GetMac().GetChannel();
}

otError otLinkSetChannel(otInstance *aInstance, uint8_t aChannel)
{
    otError error;

    VerifyOrExit(aInstance->mThreadNetif.GetMle().GetRole() == OT_DEVICE_ROLE_DISABLED,
                 error = OT_ERROR_INVALID_STATE);

    error = aInstance->mThreadNetif.GetMac().SetChannel(aChannel);
    aInstance->mThreadNetif.GetActiveDataset().Clear();
    aInstance->mThreadNetif.GetPendingDataset().Clear();

exit:
    return error;
}

const uint8_t *otLinkGetExtendedAddress(otInstance *aInstance)
{
    return reinterpret_cast<const uint8_t *>(aInstance->mThreadNetif.GetMac().GetExtAddress());
}

otError otLinkSetExtendedAddress(otInstance *aInstance, const otExtAddress *aExtAddress)
{
    otError error = OT_ERROR_NONE;

    VerifyOrExit(aExtAddress != NULL, error = OT_ERROR_INVALID_ARGS);
    VerifyOrExit(aInstance->mThreadNetif.GetMle().GetRole() == OT_DEVICE_ROLE_DISABLED,
                 error = OT_ERROR_INVALID_STATE);

    aInstance->mThreadNetif.GetMac().SetExtAddress(*static_cast<const Mac::ExtAddress *>(aExtAddress));

    SuccessOrExit(error = aInstance->mThreadNetif.GetMle().UpdateLinkLocalAddress());

exit:
    return error;
}

void otLinkGetFactoryAssignedIeeeEui64(otInstance *aInstance, otExtAddress *aEui64)
{
    otPlatRadioGetIeeeEui64(aInstance, aEui64->m8);
}

void otLinkGetJoinerId(otInstance *aInstance, otExtAddress *aHashMacAddress)
{
    aInstance->mThreadNetif.GetMac().GetHashMacAddress(static_cast<Mac::ExtAddress *>(aHashMacAddress));
}

int8_t otLinkGetMaxTransmitPower(otInstance *aInstance)
{
    return aInstance->mThreadNetif.GetMac().GetMaxTransmitPower();
}

void otLinkSetMaxTransmitPower(otInstance *aInstance, int8_t aPower)
{
    aInstance->mThreadNetif.GetMac().SetMaxTransmitPower(aPower);
    otPlatRadioSetDefaultTxPower(aInstance, aPower);
}

otPanId otLinkGetPanId(otInstance *aInstance)
{
    return aInstance->mThreadNetif.GetMac().GetPanId();
}

otError otLinkSetPanId(otInstance *aInstance, otPanId aPanId)
{
    otError error = OT_ERROR_NONE;

    VerifyOrExit(aInstance->mThreadNetif.GetMle().GetRole() == OT_DEVICE_ROLE_DISABLED,
                 error = OT_ERROR_INVALID_STATE);

    error = aInstance->mThreadNetif.GetMac().SetPanId(aPanId);
    aInstance->mThreadNetif.GetActiveDataset().Clear();
    aInstance->mThreadNetif.GetPendingDataset().Clear();

exit:
    return error;
}

uint32_t otLinkGetPollPeriod(otInstance *aInstance)
{
    return aInstance->mThreadNetif.GetMeshForwarder().GetDataPollManager().GetKeepAlivePollPeriod();
}

void otLinkSetPollPeriod(otInstance *aInstance, uint32_t aPollPeriod)
{
    aInstance->mThreadNetif.GetMeshForwarder().GetDataPollManager().SetExternalPollPeriod(aPollPeriod);
}

otError otLinkSendDataRequest(otInstance *aInstance)
{
    return aInstance->mThreadNetif.GetMeshForwarder().GetDataPollManager().SendDataPoll();
}

otShortAddress otLinkGetShortAddress(otInstance *aInstance)
{
    return aInstance->mThreadNetif.GetMac().GetShortAddress();
}

otError otLinkAddWhitelist(otInstance *aInstance, const uint8_t *aExtAddr)
{
    otError error = OT_ERROR_NONE;

    if (aInstance->mThreadNetif.GetMac().GetWhitelist().Add(*reinterpret_cast<const Mac::ExtAddress *>(aExtAddr)) == NULL)
    {
        error = OT_ERROR_NO_BUFS;
    }

    return error;
}

otError otLinkAddWhitelistRssi(otInstance *aInstance, const uint8_t *aExtAddr, int8_t aRssi)
{
    otError error = OT_ERROR_NONE;
    otMacWhitelistEntry *entry;

    entry = aInstance->mThreadNetif.GetMac().GetWhitelist().Add(*reinterpret_cast<const Mac::ExtAddress *>(aExtAddr));
    VerifyOrExit(entry != NULL, error = OT_ERROR_NO_BUFS);
    aInstance->mThreadNetif.GetMac().GetWhitelist().SetFixedRssi(*entry, aRssi);

exit:
    return error;
}

void otLinkRemoveWhitelist(otInstance *aInstance, const uint8_t *aExtAddr)
{
    aInstance->mThreadNetif.GetMac().GetWhitelist().Remove(*reinterpret_cast<const Mac::ExtAddress *>(aExtAddr));
}

void otLinkClearWhitelist(otInstance *aInstance)
{
    aInstance->mThreadNetif.GetMac().GetWhitelist().Clear();
}

otError otLinkGetWhitelistEntry(otInstance *aInstance, uint8_t aIndex, otMacWhitelistEntry *aEntry)
{
    otError error = OT_ERROR_NONE;

    VerifyOrExit(aEntry != NULL, error = OT_ERROR_INVALID_ARGS);
    error = aInstance->mThreadNetif.GetMac().GetWhitelist().GetEntry(aIndex, *aEntry);

exit:
    return error;
}

void otLinkSetWhitelistEnabled(otInstance *aInstance, bool aEnabled)
{
    aInstance->mThreadNetif.GetMac().GetWhitelist().SetEnabled(aEnabled);
}

bool otLinkIsWhitelistEnabled(otInstance *aInstance)
{
    return aInstance->mThreadNetif.GetMac().GetWhitelist().IsEnabled();
}

otError otLinkAddBlacklist(otInstance *aInstance, const uint8_t *aExtAddr)
{
    otError error = OT_ERROR_NONE;

    if (aInstance->mThreadNetif.GetMac().GetBlacklist().Add(*reinterpret_cast<const Mac::ExtAddress *>(aExtAddr)) == NULL)
    {
        error = OT_ERROR_NO_BUFS;
    }

    return error;
}

void otLinkRemoveBlacklist(otInstance *aInstance, const uint8_t *aExtAddr)
{
    aInstance->mThreadNetif.GetMac().GetBlacklist().Remove(*reinterpret_cast<const Mac::ExtAddress *>(aExtAddr));
}

void otLinkClearBlacklist(otInstance *aInstance)
{
    aInstance->mThreadNetif.GetMac().GetBlacklist().Clear();
}

otError otLinkGetBlacklistEntry(otInstance *aInstance, uint8_t aIndex, otMacBlacklistEntry *aEntry)
{
    otError error = OT_ERROR_NONE;

    VerifyOrExit(aEntry != NULL, error = OT_ERROR_INVALID_ARGS);
    error = aInstance->mThreadNetif.GetMac().GetBlacklist().GetEntry(aIndex, *aEntry);

exit:
    return error;
}

void otLinkSetBlacklistEnabled(otInstance *aInstance, bool aEnabled)
{
    aInstance->mThreadNetif.GetMac().GetBlacklist().SetEnabled(aEnabled);
}

bool otLinkIsBlacklistEnabled(otInstance *aInstance)
{
    return aInstance->mThreadNetif.GetMac().GetBlacklist().IsEnabled();
}

otError otLinkGetAssignLinkQuality(otInstance *aInstance, const uint8_t *aExtAddr, uint8_t *aLinkQuality)
{
    Mac::ExtAddress extAddress;

    memset(&extAddress, 0, sizeof(extAddress));
    memcpy(extAddress.m8, aExtAddr, OT_EXT_ADDRESS_SIZE);

    return aInstance->mThreadNetif.GetMle().GetAssignLinkQuality(extAddress, *aLinkQuality);
}

void otLinkSetAssignLinkQuality(otInstance *aInstance, const uint8_t *aExtAddr, uint8_t aLinkQuality)
{
    Mac::ExtAddress extAddress;

    memset(&extAddress, 0, sizeof(extAddress));
    memcpy(extAddress.m8, aExtAddr, OT_EXT_ADDRESS_SIZE);

    aInstance->mThreadNetif.GetMle().SetAssignLinkQuality(extAddress, aLinkQuality);
}

void otLinkSetPcapCallback(otInstance *aInstance, otLinkPcapCallback aPcapCallback, void *aCallbackContext)
{
    aInstance->mThreadNetif.GetMac().SetPcapCallback(aPcapCallback, aCallbackContext);
}

bool otLinkIsPromiscuous(otInstance *aInstance)
{
    return aInstance->mThreadNetif.GetMac().IsPromiscuous();
}

otError otLinkSetPromiscuous(otInstance *aInstance, bool aPromiscuous)
{
    otError error = OT_ERROR_NONE;

    // cannot enable IEEE 802.15.4 promiscuous mode if the Thread interface is enabled
    VerifyOrExit(aInstance->mThreadNetif.IsUp() == false, error = OT_ERROR_INVALID_STATE);

    aInstance->mThreadNetif.GetMac().SetPromiscuous(aPromiscuous);

exit:
    return error;
}

const otMacCounters *otLinkGetCounters(otInstance *aInstance)
{
    return &aInstance->mThreadNetif.GetMac().GetCounters();
}

otError otLinkActiveScan(otInstance *aInstance, uint32_t aScanChannels, uint16_t aScanDuration,
                         otHandleActiveScanResult aCallback, void *aCallbackContext)
{
    aInstance->mActiveScanCallback = aCallback;
    aInstance->mActiveScanCallbackContext = aCallbackContext;
    return aInstance->mThreadNetif.GetMac().ActiveScan(aScanChannels, aScanDuration, &HandleActiveScanResult, aInstance);
}

bool otLinkIsActiveScanInProgress(otInstance *aInstance)
{
    return aInstance->mThreadNetif.GetMac().IsActiveScanInProgress();
}

void HandleActiveScanResult(void *aContext, Mac::Frame *aFrame)
{
    otInstance *aInstance = static_cast<otInstance *>(aContext);
    otActiveScanResult result;

    VerifyOrExit(aFrame != NULL, aInstance->mActiveScanCallback(NULL, aInstance->mActiveScanCallbackContext));
    aInstance->mThreadNetif.GetMac().ConvertBeaconToActiveScanResult(aFrame, result);
    aInstance->mActiveScanCallback(&result, aInstance->mActiveScanCallbackContext);

exit:
    return;
}

otError otLinkEnergyScan(otInstance *aInstance, uint32_t aScanChannels, uint16_t aScanDuration,
                         otHandleEnergyScanResult aCallback, void *aCallbackContext)
{
    aInstance->mEnergyScanCallback = aCallback;
    aInstance->mEnergyScanCallbackContext = aCallbackContext;
    return aInstance->mThreadNetif.GetMac().EnergyScan(aScanChannels, aScanDuration, &HandleEnergyScanResult, aInstance);
}

void HandleEnergyScanResult(void *aContext, otEnergyScanResult *aResult)
{
    otInstance *aInstance = static_cast<otInstance *>(aContext);

    aInstance->mEnergyScanCallback(aResult, aInstance->mEnergyScanCallbackContext);
}

bool otLinkIsEnergyScanInProgress(otInstance *aInstance)
{
    return aInstance->mThreadNetif.GetMac().IsEnergyScanInProgress();
}

bool otLinkIsInTransmitState(otInstance *aInstance)
{
    return aInstance->mThreadNetif.GetMac().IsInTransmitState();
}
