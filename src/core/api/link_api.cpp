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

#if OPENTHREAD_ENABLE_MAC_FILTER

otMacFilterAddressMode otLinkFilterGetAddressMode(otInstance *aInstance)
{
    return aInstance->mThreadNetif.GetMac().GetFilter().GetAddressMode();
}

otError otLinkFilterSetAddressMode(otInstance *aInstance, otMacFilterAddressMode aMode)
{
    return aInstance->mThreadNetif.GetMac().GetFilter().SetAddressMode(aMode);
}

otError otLinkFilterAddAddress(otInstance *aInstance, const otExtAddress *aExtAddress)
{
    otError error = OT_ERROR_NONE;

    VerifyOrExit(aExtAddress != NULL, error = OT_ERROR_INVALID_ARGS);

    error = aInstance->mThreadNetif.GetMac().GetFilter().AddAddress(*static_cast<const Mac::ExtAddress *>(aExtAddress));

exit:
    return error;
}

otError otLinkFilterAddAddressRssIn(otInstance *aInstance, const otExtAddress *aExtAddress, int8_t aRss)
{
    otError error = OT_ERROR_NONE;

    VerifyOrExit(aExtAddress != NULL, error = OT_ERROR_INVALID_ARGS);

    error = aInstance->mThreadNetif.GetMac().GetFilter().AddAddressRssIn(
                *static_cast<const Mac::ExtAddress *>(aExtAddress), aRss);

exit:
    return error;
}


otError otLinkFilterRemoveAddress(otInstance *aInstance, const otExtAddress *aExtAddress)
{
    otError error = OT_ERROR_NONE;

    VerifyOrExit(aExtAddress != NULL, error = OT_ERROR_INVALID_ARGS);

    error = aInstance->mThreadNetif.GetMac().GetFilter().RemoveAddress(
                *static_cast<const Mac::ExtAddress *>(aExtAddress));

exit:
    return error;
}

void otLinkFilterClearAddresses(otInstance *aInstance)
{
    return aInstance->mThreadNetif.GetMac().GetFilter().ClearAddresses();
}

otError otLinkFilterGetNextAddress(otInstance *aInstance, otMacFilterIterator *aIterator, otMacFilterEntry *aEntry)
{

    otError error = OT_ERROR_NONE;

    VerifyOrExit(aIterator != NULL && aEntry != NULL, error = OT_ERROR_INVALID_ARGS);

    error = aInstance->mThreadNetif.GetMac().GetFilter().GetNextAddress(*aIterator, *aEntry);

exit:
    return error;
}

otError otLinkFilterAddRssIn(otInstance *aInstance, const otExtAddress *aExtAddress, int8_t aRss)
{
    return aInstance->mThreadNetif.GetMac().GetFilter().AddRssIn(
               static_cast<const Mac::ExtAddress *>(aExtAddress), aRss);
}

otError otLinkFilterRemoveRssIn(otInstance *aInstance, const otExtAddress *aExtAddress)
{
    return aInstance->mThreadNetif.GetMac().GetFilter().RemoveRssIn(static_cast<const Mac::ExtAddress *>(aExtAddress));
}

void otLinkFilterClearRssIn(otInstance *aInstance)
{
    aInstance->mThreadNetif.GetMac().GetFilter().ClearRssIn();
}

otError otLinkFilterGetNextRssIn(otInstance *aInstance, otMacFilterIterator *aIterator, otMacFilterEntry *aEntry)
{
    otError error = OT_ERROR_NONE;

    VerifyOrExit(aIterator != NULL && aEntry != NULL, error = OT_ERROR_INVALID_ARGS);

    error = aInstance->mThreadNetif.GetMac().GetFilter().GetNextRssIn(*aIterator, *aEntry);

exit:
    return error;
}

uint8_t otLinkConvertRssToLinkQuality(otInstance *aInstance, int8_t aRss)
{
    return LinkQualityInfo::ConvertRssToLinkQuality(aInstance->mThreadNetif.GetMac().GetNoiseFloor(), aRss);
}

int8_t otLinkConvertLinkQualityToRss(otInstance *aInstance, uint8_t aLinkQuality)
{
    return LinkQualityInfo::ConvertLinkQualityToRss(aInstance->mThreadNetif.GetMac().GetNoiseFloor(), aLinkQuality);
}

#endif  // OPENTHREAD_ENABLE_MAC_FILTER

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
    return aInstance->mThreadNetif.GetMac().ActiveScan(aScanChannels, aScanDuration,
                                                       &HandleActiveScanResult, aInstance);
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
    return aInstance->mThreadNetif.GetMac().EnergyScan(aScanChannels, aScanDuration,
                                                       &HandleEnergyScanResult, aInstance);
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
