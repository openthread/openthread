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

#include "openthread/link.h"

#include "openthread-instance.h"

using namespace Thread;

#ifdef __cplusplus
extern "C" {
#endif

static void HandleActiveScanResult(void *aContext, Mac::Frame *aFrame);
static void HandleEnergyScanResult(void *aContext, otEnergyScanResult *aResult);

uint8_t otLinkGetChannel(otInstance *aInstance)
{
    return aInstance->mThreadNetif.GetMac().GetChannel();
}

ThreadError otLinkSetChannel(otInstance *aInstance, uint8_t aChannel)
{
    return aInstance->mThreadNetif.GetMac().SetChannel(aChannel);
}

const uint8_t *otLinkGetExtendedAddress(otInstance *aInstance)
{
    return reinterpret_cast<const uint8_t *>(aInstance->mThreadNetif.GetMac().GetExtAddress());
}

ThreadError otLinkSetExtendedAddress(otInstance *aInstance, const otExtAddress *aExtAddress)
{
    ThreadError error = kThreadError_None;

    VerifyOrExit(aExtAddress != NULL, error = kThreadError_InvalidArgs);

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

ThreadError otLinkSetPanId(otInstance *aInstance, otPanId aPanId)
{
    ThreadError error = kThreadError_None;

    // do not allow setting PAN ID to broadcast if Thread is running
    VerifyOrExit(aPanId != Mac::kPanIdBroadcast ||
                 aInstance->mThreadNetif.GetMle().GetDeviceState() != Mle::kDeviceStateDisabled,
                 error = kThreadError_InvalidState);

    error = aInstance->mThreadNetif.GetMac().SetPanId(aPanId);

exit:
    return error;
}

uint32_t otLinkGetPollPeriod(otInstance *aInstance)
{
    return aInstance->mThreadNetif.GetMeshForwarder().GetAssignPollPeriod();
}

void otLinkSetPollPeriod(otInstance *aInstance, uint32_t aPollPeriod)
{
    aInstance->mThreadNetif.GetMeshForwarder().SetAssignPollPeriod(aPollPeriod);
}

ThreadError otSendMacDataRequest(otInstance *aInstance)
{
    return aInstance->mThreadNetif.GetMeshForwarder().SendMacDataRequest();
}

otShortAddress otLinkGetShortAddress(otInstance *aInstance)
{
    return aInstance->mThreadNetif.GetMac().GetShortAddress();
}

ThreadError otLinkAddWhitelist(otInstance *aInstance, const uint8_t *aExtAddr)
{
    ThreadError error = kThreadError_None;

    if (aInstance->mThreadNetif.GetMac().GetWhitelist().Add(*reinterpret_cast<const Mac::ExtAddress *>(aExtAddr)) == NULL)
    {
        error = kThreadError_NoBufs;
    }

    return error;
}

ThreadError otLinkAddWhitelistRssi(otInstance *aInstance, const uint8_t *aExtAddr, int8_t aRssi)
{
    ThreadError error = kThreadError_None;
    otMacWhitelistEntry *entry;

    entry = aInstance->mThreadNetif.GetMac().GetWhitelist().Add(*reinterpret_cast<const Mac::ExtAddress *>(aExtAddr));
    VerifyOrExit(entry != NULL, error = kThreadError_NoBufs);
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

ThreadError otLinkGetWhitelistEntry(otInstance *aInstance, uint8_t aIndex, otMacWhitelistEntry *aEntry)
{
    ThreadError error = kThreadError_None;

    VerifyOrExit(aEntry != NULL, error = kThreadError_InvalidArgs);
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

ThreadError otLinkAddBlacklist(otInstance *aInstance, const uint8_t *aExtAddr)
{
    ThreadError error = kThreadError_None;

    if (aInstance->mThreadNetif.GetMac().GetBlacklist().Add(*reinterpret_cast<const Mac::ExtAddress *>(aExtAddr)) == NULL)
    {
        error = kThreadError_NoBufs;
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

ThreadError otLinkGetBlacklistEntry(otInstance *aInstance, uint8_t aIndex, otMacBlacklistEntry *aEntry)
{
    ThreadError error = kThreadError_None;

    VerifyOrExit(aEntry != NULL, error = kThreadError_InvalidArgs);
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

ThreadError otLinkGetAssignLinkQuality(otInstance *aInstance, const uint8_t *aExtAddr, uint8_t *aLinkQuality)
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

ThreadError otLinkSetPromiscuous(otInstance *aInstance, bool aPromiscuous)
{
    ThreadError error = kThreadError_None;

    // cannot enable IEEE 802.15.4 promiscuous mode if the Thread interface is enabled
    VerifyOrExit(aInstance->mThreadNetif.IsUp() == false, error = kThreadError_InvalidState);

    aInstance->mThreadNetif.GetMac().SetPromiscuous(aPromiscuous);

exit:
    return error;
}

const otMacCounters *otLinkGetCounters(otInstance *aInstance)
{
    return &aInstance->mThreadNetif.GetMac().GetCounters();
}

ThreadError otLinkActiveScan(otInstance *aInstance, uint32_t aScanChannels, uint16_t aScanDuration,
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
    Mac::Address address;
    Mac::Beacon *beacon = NULL;
    Mac::BeaconPayload *beaconPayload = NULL;
    uint8_t payloadLength;

    memset(&result, 0, sizeof(otActiveScanResult));

    if (aFrame == NULL)
    {
        aInstance->mActiveScanCallback(NULL, aInstance->mActiveScanCallbackContext);
        ExitNow();
    }

    SuccessOrExit(aFrame->GetSrcAddr(address));
    VerifyOrExit(address.mLength == sizeof(address.mExtAddress), ;);
    memcpy(&result.mExtAddress, &address.mExtAddress, sizeof(result.mExtAddress));

    aFrame->GetSrcPanId(result.mPanId);
    result.mChannel = aFrame->GetChannel();
    result.mRssi = aFrame->GetPower();
    result.mLqi = aFrame->GetLqi();

    payloadLength = aFrame->GetPayloadLength();

    beacon = reinterpret_cast<Mac::Beacon *>(aFrame->GetPayload());
    beaconPayload = reinterpret_cast<Mac::BeaconPayload *>(beacon->GetPayload());

    if ((payloadLength >= (sizeof(*beacon) + sizeof(*beaconPayload))) && beacon->IsValid() && beaconPayload->IsValid())
    {
        result.mVersion = beaconPayload->GetProtocolVersion();
        result.mIsJoinable = beaconPayload->IsJoiningPermitted();
        result.mIsNative = beaconPayload->IsNative();
        memcpy(&result.mNetworkName, beaconPayload->GetNetworkName(), sizeof(result.mNetworkName));
        memcpy(&result.mExtendedPanId, beaconPayload->GetExtendedPanId(), sizeof(result.mExtendedPanId));
    }

    aInstance->mActiveScanCallback(&result, aInstance->mActiveScanCallbackContext);

exit:
    return;
}

ThreadError otLinkEnergyScan(otInstance *aInstance, uint32_t aScanChannels, uint16_t aScanDuration,
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


#ifdef __cplusplus
}  // extern "C"
#endif
