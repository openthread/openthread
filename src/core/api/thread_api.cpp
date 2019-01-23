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
 *   This file implements the OpenThread Thread API (for both FTD and MTD).
 */

#define WPP_NAME "thread_api.tmh"

#include "openthread-core-config.h"

#include <openthread/thread.h>

#include "common/instance.hpp"
#include "common/logging.hpp"
#include "common/settings.hpp"

using namespace ot;

uint32_t otThreadGetChildTimeout(otInstance *aInstance)
{
    Instance &instance = *static_cast<Instance *>(aInstance);

    return instance.GetThreadNetif().GetMle().GetTimeout();
}

void otThreadSetChildTimeout(otInstance *aInstance, uint32_t aTimeout)
{
    Instance &instance = *static_cast<Instance *>(aInstance);

    instance.GetThreadNetif().GetMle().SetTimeout(aTimeout);
}

const otExtendedPanId *otThreadGetExtendedPanId(otInstance *aInstance)
{
    Instance &instance = *static_cast<Instance *>(aInstance);

    return &instance.GetThreadNetif().GetMac().GetExtendedPanId();
}

otError otThreadSetExtendedPanId(otInstance *aInstance, const otExtendedPanId *aExtendedPanId)
{
    otError           error    = OT_ERROR_NONE;
    Instance &        instance = *static_cast<Instance *>(aInstance);
    otMeshLocalPrefix prefix;

    VerifyOrExit(instance.GetThreadNetif().GetMle().GetRole() == OT_DEVICE_ROLE_DISABLED,
                 error = OT_ERROR_INVALID_STATE);

    instance.GetThreadNetif().GetMac().SetExtendedPanId(*aExtendedPanId);

    prefix.m8[0] = 0xfd;
    memcpy(&prefix.m8[1], aExtendedPanId->m8, 5);
    prefix.m8[6] = 0x00;
    prefix.m8[7] = 0x00;
    instance.GetThreadNetif().GetMle().SetMeshLocalPrefix(prefix);

    instance.GetThreadNetif().GetActiveDataset().Clear();
    instance.GetThreadNetif().GetPendingDataset().Clear();

exit:
    return error;
}

otError otThreadGetLeaderRloc(otInstance *aInstance, otIp6Address *aAddress)
{
    otError   error;
    Instance &instance = *static_cast<Instance *>(aInstance);

    VerifyOrExit(aAddress != NULL, error = OT_ERROR_INVALID_ARGS);

    error = instance.GetThreadNetif().GetMle().GetLeaderAddress(*static_cast<Ip6::Address *>(aAddress));

exit:
    return error;
}

otLinkModeConfig otThreadGetLinkMode(otInstance *aInstance)
{
    otLinkModeConfig config;
    Instance &       instance = *static_cast<Instance *>(aInstance);
    uint8_t          mode     = instance.GetThreadNetif().GetMle().GetDeviceMode();

    memset(&config, 0, sizeof(otLinkModeConfig));

    if (mode & Mle::ModeTlv::kModeRxOnWhenIdle)
    {
        config.mRxOnWhenIdle = 1;
    }

    if (mode & Mle::ModeTlv::kModeSecureDataRequest)
    {
        config.mSecureDataRequests = 1;
    }

    if (mode & Mle::ModeTlv::kModeFullThreadDevice)
    {
        config.mDeviceType = 1;
    }

    if (mode & Mle::ModeTlv::kModeFullNetworkData)
    {
        config.mNetworkData = 1;
    }

    return config;
}

otError otThreadSetLinkMode(otInstance *aInstance, otLinkModeConfig aConfig)
{
    uint8_t   mode     = 0;
    Instance &instance = *static_cast<Instance *>(aInstance);

    if (aConfig.mRxOnWhenIdle)
    {
        mode |= Mle::ModeTlv::kModeRxOnWhenIdle;
    }

    if (aConfig.mSecureDataRequests)
    {
        mode |= Mle::ModeTlv::kModeSecureDataRequest;
    }

    if (aConfig.mDeviceType)
    {
        mode |= Mle::ModeTlv::kModeFullThreadDevice;
    }

    if (aConfig.mNetworkData)
    {
        mode |= Mle::ModeTlv::kModeFullNetworkData;
    }

    return instance.GetThreadNetif().GetMle().SetDeviceMode(mode);
}

const otMasterKey *otThreadGetMasterKey(otInstance *aInstance)
{
    Instance &instance = *static_cast<Instance *>(aInstance);

    return &instance.GetThreadNetif().GetKeyManager().GetMasterKey();
}

otError otThreadSetMasterKey(otInstance *aInstance, const otMasterKey *aKey)
{
    otError   error    = OT_ERROR_NONE;
    Instance &instance = *static_cast<Instance *>(aInstance);

    VerifyOrExit(aKey != NULL, error = OT_ERROR_INVALID_ARGS);
    VerifyOrExit(instance.GetThreadNetif().GetMle().GetRole() == OT_DEVICE_ROLE_DISABLED,
                 error = OT_ERROR_INVALID_STATE);

    error = instance.GetThreadNetif().GetKeyManager().SetMasterKey(*aKey);
    instance.GetThreadNetif().GetActiveDataset().Clear();
    instance.GetThreadNetif().GetPendingDataset().Clear();

exit:
    return error;
}

const otIp6Address *otThreadGetMeshLocalEid(otInstance *aInstance)
{
    Instance &instance = *static_cast<Instance *>(aInstance);

    return &instance.GetThreadNetif().GetMle().GetMeshLocal64();
}

const otMeshLocalPrefix *otThreadGetMeshLocalPrefix(otInstance *aInstance)
{
    Instance &instance = *static_cast<Instance *>(aInstance);

    return &instance.GetThreadNetif().GetMle().GetMeshLocalPrefix();
}

otError otThreadSetMeshLocalPrefix(otInstance *aInstance, const otMeshLocalPrefix *aMeshLocalPrefix)
{
    otError   error    = OT_ERROR_NONE;
    Instance &instance = *static_cast<Instance *>(aInstance);

    VerifyOrExit(instance.GetThreadNetif().GetMle().GetRole() == OT_DEVICE_ROLE_DISABLED,
                 error = OT_ERROR_INVALID_STATE);

    instance.GetThreadNetif().GetMle().SetMeshLocalPrefix(*aMeshLocalPrefix);
    instance.GetThreadNetif().GetActiveDataset().Clear();
    instance.GetThreadNetif().GetPendingDataset().Clear();

exit:
    return error;
}

const otIp6Address *otThreadGetLinkLocalIp6Address(otInstance *aInstance)
{
    Instance &instance = *static_cast<Instance *>(aInstance);

    return &instance.GetThreadNetif().GetMle().GetLinkLocalAddress();
}

const char *otThreadGetNetworkName(otInstance *aInstance)
{
    Instance &instance = *static_cast<Instance *>(aInstance);

    return instance.GetThreadNetif().GetMac().GetNetworkName();
}

otError otThreadSetNetworkName(otInstance *aInstance, const char *aNetworkName)
{
    otError   error    = OT_ERROR_NONE;
    Instance &instance = *static_cast<Instance *>(aInstance);

    VerifyOrExit(instance.GetThreadNetif().GetMle().GetRole() == OT_DEVICE_ROLE_DISABLED,
                 error = OT_ERROR_INVALID_STATE);

    error = instance.GetThreadNetif().GetMac().SetNetworkName(aNetworkName);
    instance.GetThreadNetif().GetActiveDataset().Clear();
    instance.GetThreadNetif().GetPendingDataset().Clear();

exit:
    return error;
}

uint32_t otThreadGetKeySequenceCounter(otInstance *aInstance)
{
    Instance &instance = *static_cast<Instance *>(aInstance);

    return instance.GetThreadNetif().GetKeyManager().GetCurrentKeySequence();
}

void otThreadSetKeySequenceCounter(otInstance *aInstance, uint32_t aKeySequenceCounter)
{
    Instance &instance = *static_cast<Instance *>(aInstance);

    instance.GetThreadNetif().GetKeyManager().SetCurrentKeySequence(aKeySequenceCounter);
}

uint32_t otThreadGetKeySwitchGuardTime(otInstance *aInstance)
{
    Instance &instance = *static_cast<Instance *>(aInstance);

    return instance.GetThreadNetif().GetKeyManager().GetKeySwitchGuardTime();
}

void otThreadSetKeySwitchGuardTime(otInstance *aInstance, uint32_t aKeySwitchGuardTime)
{
    Instance &instance = *static_cast<Instance *>(aInstance);

    instance.GetThreadNetif().GetKeyManager().SetKeySwitchGuardTime(aKeySwitchGuardTime);
}

otError otThreadBecomeDetached(otInstance *aInstance)
{
    Instance &instance = *static_cast<Instance *>(aInstance);

    return instance.GetThreadNetif().GetMle().BecomeDetached();
}

otError otThreadBecomeChild(otInstance *aInstance)
{
    Instance &instance = *static_cast<Instance *>(aInstance);

    return instance.GetThreadNetif().GetMle().BecomeChild(Mle::kAttachAny);
}

otError otThreadGetNextNeighborInfo(otInstance *aInstance, otNeighborInfoIterator *aIterator, otNeighborInfo *aInfo)
{
    otError   error    = OT_ERROR_NONE;
    Instance &instance = *static_cast<Instance *>(aInstance);

    VerifyOrExit((aInfo != NULL) && (aIterator != NULL), error = OT_ERROR_INVALID_ARGS);

    error = instance.GetThreadNetif().GetMle().GetNextNeighborInfo(*aIterator, *aInfo);

exit:
    return error;
}

otDeviceRole otThreadGetDeviceRole(otInstance *aInstance)
{
    Instance &instance = *static_cast<Instance *>(aInstance);

    return static_cast<otDeviceRole>(instance.GetThreadNetif().GetMle().GetRole());
}

otError otThreadGetLeaderData(otInstance *aInstance, otLeaderData *aLeaderData)
{
    otError   error;
    Instance &instance = *static_cast<Instance *>(aInstance);

    VerifyOrExit(aLeaderData != NULL, error = OT_ERROR_INVALID_ARGS);

    error = instance.GetThreadNetif().GetMle().GetLeaderData(*aLeaderData);

exit:
    return error;
}

uint8_t otThreadGetLeaderRouterId(otInstance *aInstance)
{
    Instance &instance = *static_cast<Instance *>(aInstance);

    return instance.GetThreadNetif().GetMle().GetLeaderDataTlv().GetLeaderRouterId();
}

uint8_t otThreadGetLeaderWeight(otInstance *aInstance)
{
    Instance &instance = *static_cast<Instance *>(aInstance);

    return instance.GetThreadNetif().GetMle().GetLeaderDataTlv().GetWeighting();
}

uint32_t otThreadGetPartitionId(otInstance *aInstance)
{
    Instance &instance = *static_cast<Instance *>(aInstance);

    return instance.GetThreadNetif().GetMle().GetLeaderDataTlv().GetPartitionId();
}

uint16_t otThreadGetRloc16(otInstance *aInstance)
{
    Instance &instance = *static_cast<Instance *>(aInstance);

    return instance.GetThreadNetif().GetMle().GetRloc16();
}

otError otThreadGetParentInfo(otInstance *aInstance, otRouterInfo *aParentInfo)
{
    otError   error    = OT_ERROR_NONE;
    Instance &instance = *static_cast<Instance *>(aInstance);
    Router *  parent;

    VerifyOrExit(aParentInfo != NULL, error = OT_ERROR_INVALID_ARGS);

    parent                       = instance.GetThreadNetif().GetMle().GetParent();
    aParentInfo->mExtAddress     = parent->GetExtAddress();
    aParentInfo->mRloc16         = parent->GetRloc16();
    aParentInfo->mRouterId       = Mle::Mle::GetRouterId(parent->GetRloc16());
    aParentInfo->mNextHop        = parent->GetNextHop();
    aParentInfo->mPathCost       = parent->GetCost();
    aParentInfo->mLinkQualityIn  = parent->GetLinkInfo().GetLinkQuality();
    aParentInfo->mLinkQualityOut = parent->GetLinkQualityOut();
    aParentInfo->mAge = static_cast<uint8_t>(TimerMilli::MsecToSec(TimerMilli::GetNow() - parent->GetLastHeard()));
    aParentInfo->mAllocated       = true;
    aParentInfo->mLinkEstablished = parent->GetState() == Neighbor::kStateValid;

exit:
    return error;
}

otError otThreadGetParentAverageRssi(otInstance *aInstance, int8_t *aParentRssi)
{
    otError   error    = OT_ERROR_NONE;
    Instance &instance = *static_cast<Instance *>(aInstance);
    Router *  parent;

    VerifyOrExit(aParentRssi != NULL, error = OT_ERROR_INVALID_ARGS);

    parent       = instance.GetThreadNetif().GetMle().GetParent();
    *aParentRssi = parent->GetLinkInfo().GetAverageRss();

    VerifyOrExit(*aParentRssi != OT_RADIO_RSSI_INVALID, error = OT_ERROR_FAILED);

exit:
    return error;
}

otError otThreadGetParentLastRssi(otInstance *aInstance, int8_t *aLastRssi)
{
    otError   error    = OT_ERROR_NONE;
    Instance &instance = *static_cast<Instance *>(aInstance);
    Router *  parent;

    VerifyOrExit(aLastRssi != NULL, error = OT_ERROR_INVALID_ARGS);

    parent     = instance.GetThreadNetif().GetMle().GetParent();
    *aLastRssi = parent->GetLinkInfo().GetLastRss();

    VerifyOrExit(*aLastRssi != OT_RADIO_RSSI_INVALID, error = OT_ERROR_FAILED);

exit:
    return error;
}

#if OPENTHREAD_FTD || OPENTHREAD_ENABLE_MTD_NETWORK_DIAGNOSTIC
void otThreadSetReceiveDiagnosticGetCallback(otInstance *                   aInstance,
                                             otReceiveDiagnosticGetCallback aCallback,
                                             void *                         aCallbackContext)
{
    Instance &instance = *static_cast<Instance *>(aInstance);

    instance.GetThreadNetif().GetNetworkDiagnostic().SetReceiveDiagnosticGetCallback(aCallback, aCallbackContext);
}

otError otThreadSendDiagnosticGet(otInstance *        aInstance,
                                  const otIp6Address *aDestination,
                                  const uint8_t       aTlvTypes[],
                                  uint8_t             aCount)
{
    Instance &instance = *static_cast<Instance *>(aInstance);

    return instance.GetThreadNetif().GetNetworkDiagnostic().SendDiagnosticGet(
        *static_cast<const Ip6::Address *>(aDestination), aTlvTypes, aCount);
}

otError otThreadSendDiagnosticReset(otInstance *        aInstance,
                                    const otIp6Address *aDestination,
                                    const uint8_t       aTlvTypes[],
                                    uint8_t             aCount)
{
    Instance &instance = *static_cast<Instance *>(aInstance);

    return instance.GetThreadNetif().GetNetworkDiagnostic().SendDiagnosticReset(
        *static_cast<const Ip6::Address *>(aDestination), aTlvTypes, aCount);
}
#endif // OPENTHREAD_FTD || OPENTHREAD_ENABLE_MTD_NETWORK_DIAGNOSTIC

otError otThreadSetEnabled(otInstance *aInstance, bool aEnabled)
{
    otError   error    = OT_ERROR_NONE;
    Instance &instance = *static_cast<Instance *>(aInstance);

    if (aEnabled)
    {
        VerifyOrExit(instance.GetThreadNetif().GetMac().GetPanId() != Mac::kPanIdBroadcast,
                     error = OT_ERROR_INVALID_STATE);
        error = instance.GetThreadNetif().GetMle().Start(/* aAnnounceAttach */ false);
    }
    else
    {
        error = instance.GetThreadNetif().GetMle().Stop(true);
    }

exit:
    return error;
}

bool otThreadGetAutoStart(otInstance *aInstance)
{
#if OPENTHREAD_CONFIG_ENABLE_AUTO_START_SUPPORT
    uint8_t   autoStart = 0;
    Instance &instance  = *static_cast<Instance *>(aInstance);

    if (instance.GetSettings().ReadThreadAutoStart(autoStart) != OT_ERROR_NONE)
    {
        autoStart = 0;
    }

    return autoStart != 0;
#else
    OT_UNUSED_VARIABLE(aInstance);
    return false;
#endif
}

otError otThreadSetAutoStart(otInstance *aInstance, bool aStartAutomatically)
{
#if OPENTHREAD_CONFIG_ENABLE_AUTO_START_SUPPORT
    uint8_t   autoStart = aStartAutomatically ? 1 : 0;
    Instance &instance  = *static_cast<Instance *>(aInstance);

    return instance.GetSettings().SaveThreadAutoStart(autoStart);
#else
    OT_UNUSED_VARIABLE(aInstance);
    OT_UNUSED_VARIABLE(aStartAutomatically);
    return OT_ERROR_NOT_IMPLEMENTED;
#endif
}

bool otThreadIsSingleton(otInstance *aInstance)
{
    Instance &instance = *static_cast<Instance *>(aInstance);

    return instance.GetThreadNetif().GetMle().IsSingleton();
}

otError otThreadDiscover(otInstance *             aInstance,
                         uint32_t                 aScanChannels,
                         uint16_t                 aPanId,
                         bool                     aJoiner,
                         bool                     aEnableEui64Filtering,
                         otHandleActiveScanResult aCallback,
                         void *                   aCallbackContext)
{
    Instance &instance = *static_cast<Instance *>(aInstance);

    return instance.GetThreadNetif().GetMle().Discover(aScanChannels, aPanId, aJoiner, aEnableEui64Filtering, aCallback,
                                                       aCallbackContext);
}

bool otThreadIsDiscoverInProgress(otInstance *aInstance)
{
    Instance &instance = *static_cast<Instance *>(aInstance);

    return instance.GetThreadNetif().GetMle().IsDiscoverInProgress();
}

const otIpCounters *otThreadGetIp6Counters(otInstance *aInstance)
{
    Instance &instance = *static_cast<Instance *>(aInstance);

    return &instance.GetThreadNetif().GetMeshForwarder().GetCounters();
}

const otMleCounters *otThreadGetMleCounters(otInstance *aInstance)
{
    Instance &instance = *static_cast<Instance *>(aInstance);

    return &instance.GetThreadNetif().GetMle().GetCounters();
}

void otThreadResetMleCounters(otInstance *aInstance)
{
    Instance &instance = *static_cast<Instance *>(aInstance);

    instance.GetThreadNetif().GetMle().ResetCounters();
}

otError otThreadRegisterParentResponseCallback(otInstance *                   aInstance,
                                               otThreadParentResponseCallback aCallback,
                                               void *                         aContext)
{
#if OPENTHREAD_FTD || OPENTHREAD_MTD
    Instance &instance = *static_cast<Instance *>(aInstance);

    instance.GetThreadNetif().GetMle().RegisterParentResponseStatsCallback(aCallback, aContext);

    return OT_ERROR_NONE;
#else
    OT_UNUSED_VARIABLE(aInstance);
    OT_UNUSED_VARIABLE(aCallback);
    OT_UNUSED_VARIABLE(aContext);

    return OT_ERROR_DISABLED_FEATURE;
#endif
}
