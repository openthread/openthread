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

#include  "openthread/openthread_enable_defines.h"

#include <openthread/thread.h>
#include <openthread/platform/settings.h>

#include "openthread-instance.h"
#include "common/logging.hpp"
#include "common/settings.hpp"

using namespace ot;

uint32_t otThreadGetChildTimeout(otInstance *aInstance)
{
    return aInstance->mThreadNetif.GetMle().GetTimeout();
}

void otThreadSetChildTimeout(otInstance *aInstance, uint32_t aTimeout)
{
    aInstance->mThreadNetif.GetMle().SetTimeout(aTimeout);
}

const uint8_t *otThreadGetExtendedPanId(otInstance *aInstance)
{
    return aInstance->mThreadNetif.GetMac().GetExtendedPanId();
}

otError otThreadSetExtendedPanId(otInstance *aInstance, const uint8_t *aExtendedPanId)
{
    otError error = OT_ERROR_NONE;
    uint8_t mlPrefix[8];

    VerifyOrExit(aInstance->mThreadNetif.GetMle().GetRole() == OT_DEVICE_ROLE_DISABLED,
                 error = OT_ERROR_INVALID_STATE);

    aInstance->mThreadNetif.GetMac().SetExtendedPanId(aExtendedPanId);

    mlPrefix[0] = 0xfd;
    memcpy(mlPrefix + 1, aExtendedPanId, 5);
    mlPrefix[6] = 0x00;
    mlPrefix[7] = 0x00;
    aInstance->mThreadNetif.GetMle().SetMeshLocalPrefix(mlPrefix);

    aInstance->mThreadNetif.GetActiveDataset().Clear(false);
    aInstance->mThreadNetif.GetPendingDataset().Clear(false);

exit:
    return error;
}

otError otThreadGetLeaderRloc(otInstance *aInstance, otIp6Address *aAddress)
{
    otError error;

    VerifyOrExit(aAddress != NULL, error = OT_ERROR_INVALID_ARGS);

    error = aInstance->mThreadNetif.GetMle().GetLeaderAddress(*static_cast<Ip6::Address *>(aAddress));

exit:
    return error;
}

otLinkModeConfig otThreadGetLinkMode(otInstance *aInstance)
{
    otLinkModeConfig config;
    uint8_t mode = aInstance->mThreadNetif.GetMle().GetDeviceMode();

    memset(&config, 0, sizeof(otLinkModeConfig));

    if (mode & Mle::ModeTlv::kModeRxOnWhenIdle)
    {
        config.mRxOnWhenIdle = 1;
    }

    if (mode & Mle::ModeTlv::kModeSecureDataRequest)
    {
        config.mSecureDataRequests = 1;
    }

    if (mode & Mle::ModeTlv::kModeFFD)
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
    uint8_t mode = 0;

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
        mode |= Mle::ModeTlv::kModeFFD;
    }

    if (aConfig.mNetworkData)
    {
        mode |= Mle::ModeTlv::kModeFullNetworkData;
    }

    return aInstance->mThreadNetif.GetMle().SetDeviceMode(mode);
}

const otMasterKey *otThreadGetMasterKey(otInstance *aInstance)
{
    return &aInstance->mThreadNetif.GetKeyManager().GetMasterKey();
}

otError otThreadSetMasterKey(otInstance *aInstance, const otMasterKey *aKey)
{
    otError error = OT_ERROR_NONE;

    VerifyOrExit(aKey != NULL, error = OT_ERROR_INVALID_ARGS);
    VerifyOrExit(aInstance->mThreadNetif.GetMle().GetRole() == OT_DEVICE_ROLE_DISABLED,
                 error = OT_ERROR_INVALID_STATE);

    error = aInstance->mThreadNetif.GetKeyManager().SetMasterKey(*aKey);
    aInstance->mThreadNetif.GetActiveDataset().Clear(false);
    aInstance->mThreadNetif.GetPendingDataset().Clear(false);

exit:
    return error;
}

const otIp6Address *otThreadGetMeshLocalEid(otInstance *aInstance)
{
    return &aInstance->mThreadNetif.GetMle().GetMeshLocal64();
}

const uint8_t *otThreadGetMeshLocalPrefix(otInstance *aInstance)
{
    return aInstance->mThreadNetif.GetMle().GetMeshLocalPrefix();
}

otError otThreadSetMeshLocalPrefix(otInstance *aInstance, const uint8_t *aMeshLocalPrefix)
{
    otError error = OT_ERROR_NONE;

    VerifyOrExit(aInstance->mThreadNetif.GetMle().GetRole() == OT_DEVICE_ROLE_DISABLED,
                 error = OT_ERROR_INVALID_STATE);

    error = aInstance->mThreadNetif.GetMle().SetMeshLocalPrefix(aMeshLocalPrefix);
    aInstance->mThreadNetif.GetActiveDataset().Clear(false);
    aInstance->mThreadNetif.GetPendingDataset().Clear(false);

exit:
    return error;
}

const otIp6Address *otThreadGetLinkLocalIp6Address(otInstance *aInstance)
{
    return &aInstance->mThreadNetif.GetMle().GetLinkLocalAddress();
}

const char *otThreadGetNetworkName(otInstance *aInstance)
{
    return aInstance->mThreadNetif.GetMac().GetNetworkName();
}

otError otThreadSetNetworkName(otInstance *aInstance, const char *aNetworkName)
{
    otError error = OT_ERROR_NONE;

    VerifyOrExit(aInstance->mThreadNetif.GetMle().GetRole() == OT_DEVICE_ROLE_DISABLED,
                 error = OT_ERROR_INVALID_STATE);

    error = aInstance->mThreadNetif.GetMac().SetNetworkName(aNetworkName);
    aInstance->mThreadNetif.GetActiveDataset().Clear(false);
    aInstance->mThreadNetif.GetPendingDataset().Clear(false);

exit:
    return error;
}

uint32_t otThreadGetKeySequenceCounter(otInstance *aInstance)
{
    return aInstance->mThreadNetif.GetKeyManager().GetCurrentKeySequence();
}

void otThreadSetKeySequenceCounter(otInstance *aInstance, uint32_t aKeySequenceCounter)
{
    aInstance->mThreadNetif.GetKeyManager().SetCurrentKeySequence(aKeySequenceCounter);
}

uint32_t otThreadGetKeySwitchGuardTime(otInstance *aInstance)
{
    return aInstance->mThreadNetif.GetKeyManager().GetKeySwitchGuardTime();
}

void otThreadSetKeySwitchGuardTime(otInstance *aInstance, uint32_t aKeySwitchGuardTime)
{
    aInstance->mThreadNetif.GetKeyManager().SetKeySwitchGuardTime(aKeySwitchGuardTime);
}

otError otThreadBecomeDetached(otInstance *aInstance)
{
    return aInstance->mThreadNetif.GetMle().BecomeDetached();
}

otError otThreadBecomeChild(otInstance *aInstance)
{
    return aInstance->mThreadNetif.GetMle().BecomeChild(Mle::kAttachAny);
}

otError otThreadGetNextNeighborInfo(otInstance *aInstance, otNeighborInfoIterator *aIterator, otNeighborInfo *aInfo)
{
    otError error = OT_ERROR_NONE;

    VerifyOrExit((aInfo != NULL) && (aIterator != NULL), error = OT_ERROR_INVALID_ARGS);

    error = aInstance->mThreadNetif.GetMle().GetNextNeighborInfo(*aIterator, *aInfo);

exit:
    return error;
}

otDeviceRole otThreadGetDeviceRole(otInstance *aInstance)
{
    return static_cast<otDeviceRole>(aInstance->mThreadNetif.GetMle().GetRole());
}

otError otThreadGetLeaderData(otInstance *aInstance, otLeaderData *aLeaderData)
{
    otError error;

    VerifyOrExit(aLeaderData != NULL, error = OT_ERROR_INVALID_ARGS);

    error = aInstance->mThreadNetif.GetMle().GetLeaderData(*aLeaderData);

exit:
    return error;
}

uint8_t otThreadGetLeaderRouterId(otInstance *aInstance)
{
    return aInstance->mThreadNetif.GetMle().GetLeaderDataTlv().GetLeaderRouterId();
}

uint8_t otThreadGetLeaderWeight(otInstance *aInstance)
{
    return aInstance->mThreadNetif.GetMle().GetLeaderDataTlv().GetWeighting();
}

uint32_t otThreadGetPartitionId(otInstance *aInstance)
{
    return aInstance->mThreadNetif.GetMle().GetLeaderDataTlv().GetPartitionId();
}

uint16_t otThreadGetRloc16(otInstance *aInstance)
{
    return aInstance->mThreadNetif.GetMle().GetRloc16();
}

otError otThreadGetParentInfo(otInstance *aInstance, otRouterInfo *aParentInfo)
{
    otError error = OT_ERROR_NONE;
    Router *parent;

    VerifyOrExit(aParentInfo != NULL, error = OT_ERROR_INVALID_ARGS);

    parent = aInstance->mThreadNetif.GetMle().GetParent();
    memcpy(aParentInfo->mExtAddress.m8, &parent->GetExtAddress(), sizeof(aParentInfo->mExtAddress));

    aParentInfo->mRloc16          = parent->GetRloc16();
    aParentInfo->mRouterId        = Mle::Mle::GetRouterId(parent->GetRloc16());
    aParentInfo->mNextHop         = parent->GetNextHop();
    aParentInfo->mPathCost        = parent->GetCost();
    aParentInfo->mLinkQualityIn   = parent->GetLinkInfo().GetLinkQuality(aInstance->mThreadNetif.GetMac().GetNoiseFloor());
    aParentInfo->mLinkQualityOut  = parent->GetLinkQualityOut();
    aParentInfo->mAge             = static_cast<uint8_t>(Timer::MsecToSec(Timer::GetNow() - parent->GetLastHeard()));
    aParentInfo->mAllocated       = parent->IsAllocated();
    aParentInfo->mLinkEstablished = parent->GetState() == Neighbor::kStateValid;

exit:
    return error;
}

otError otThreadGetParentAverageRssi(otInstance *aInstance, int8_t *aParentRssi)
{
    otError error = OT_ERROR_NONE;
    Router *parent;

    VerifyOrExit(aParentRssi != NULL, error = OT_ERROR_INVALID_ARGS);

    parent = aInstance->mThreadNetif.GetMle().GetParent();
    *aParentRssi = parent->GetLinkInfo().GetAverageRss();

    VerifyOrExit(*aParentRssi != LinkQualityInfo::kUnknownRss, error = OT_ERROR_FAILED);

exit:
    return error;
}

otError otThreadGetParentLastRssi(otInstance *aInstance, int8_t *aLastRssi)
{
    otError error = OT_ERROR_NONE;
    Router *parent;

    VerifyOrExit(aLastRssi != NULL, error = OT_ERROR_INVALID_ARGS);

    parent = aInstance->mThreadNetif.GetMle().GetParent();
    *aLastRssi = parent->GetLinkInfo().GetLastRss();

    VerifyOrExit(*aLastRssi != LinkQualityInfo::kUnknownRss, error = OT_ERROR_FAILED);

exit:
    return error;
}

const char *otGetVersionString(void)
{
    /**
     * PLATFORM_VERSION_ATTR_PREFIX and PLATFORM_VERSION_ATTR_SUFFIX are
     * intended to be used to specify compiler directives to indicate
     * what linker section the platform version string should be stored.
     *
     * This is useful for specifying an exact locaiton of where the version
     * string will be located so that it can be easily retrieved from the
     * raw firmware image.
     *
     * If PLATFORM_VERSION_ATTR_PREFIX is unspecified, the keyword `static`
     * is used instead.
     *
     * If both are unspecified, the location of the string in the firmware
     * image will be undefined and may change.
     */

#ifdef PLATFORM_VERSION_ATTR_PREFIX
    PLATFORM_VERSION_ATTR_PREFIX
#else
    static
#endif
    const char sVersion[] =
        PACKAGE_NAME "/" PACKAGE_VERSION
#ifdef  PLATFORM_INFO
        "; " PLATFORM_INFO
#endif
#if defined(__DATE__)
        "; " __DATE__ " " __TIME__
#endif
#ifdef PLATFORM_VERSION_ATTR_SUFFIX
        PLATFORM_VERSION_ATTR_SUFFIX
#endif
        ; // Trailing semicolon to end statement.

    return sVersion;
}

#if OPENTHREAD_FTD || OPENTHREAD_ENABLE_MTD_NETWORK_DIAGNOSTIC
void otThreadSetReceiveDiagnosticGetCallback(otInstance *aInstance, otReceiveDiagnosticGetCallback aCallback,
                                             void *aCallbackContext)
{
    aInstance->mThreadNetif.GetNetworkDiagnostic().SetReceiveDiagnosticGetCallback(aCallback, aCallbackContext);
}

otError otThreadSendDiagnosticGet(otInstance *aInstance, const otIp6Address *aDestination,
                                  const uint8_t aTlvTypes[], uint8_t aCount)
{
    return aInstance->mThreadNetif.GetNetworkDiagnostic().SendDiagnosticGet(*static_cast<const Ip6::Address *>
                                                                            (aDestination),
                                                                            aTlvTypes,
                                                                            aCount);
}

otError otThreadSendDiagnosticReset(otInstance *aInstance, const otIp6Address *aDestination,
                                    const uint8_t aTlvTypes[], uint8_t aCount)
{
    return aInstance->mThreadNetif.GetNetworkDiagnostic().SendDiagnosticReset(*static_cast<const Ip6::Address *>
                                                                              (aDestination),
                                                                              aTlvTypes,
                                                                              aCount);
}
#endif // OPENTHREAD_FTD || OPENTHREAD_ENABLE_MTD_NETWORK_DIAGNOSTIC

otError otThreadSetEnabled(otInstance *aInstance, bool aEnabled)
{
    otError error = OT_ERROR_NONE;

    otLogFuncEntry();

    if (aEnabled)
    {
        VerifyOrExit(aInstance->mThreadNetif.GetMac().GetPanId() != Mac::kPanIdBroadcast,
                     error = OT_ERROR_INVALID_STATE);
        error = aInstance->mThreadNetif.GetMle().Start(true, false);
    }
    else
    {
        error = aInstance->mThreadNetif.GetMle().Stop(true);
    }

exit:
    otLogFuncExitErr(error);
    return error;
}

bool otThreadGetAutoStart(otInstance *aInstance)
{
#if OPENTHREAD_CONFIG_ENABLE_AUTO_START_SUPPORT
    uint8_t autoStart = 0;
    uint16_t autoStartLength = sizeof(autoStart);

    if (otPlatSettingsGet(aInstance, Settings::kKeyThreadAutoStart, 0, &autoStart, &autoStartLength) !=
        OT_ERROR_NONE)
    {
        autoStart = 0;
    }

    return autoStart != 0;
#else
    (void)aInstance;
    return false;
#endif
}

otError otThreadSetAutoStart(otInstance *aInstance, bool aStartAutomatically)
{
#if OPENTHREAD_CONFIG_ENABLE_AUTO_START_SUPPORT
    uint8_t autoStart = aStartAutomatically ? 1 : 0;
    return otPlatSettingsSet(aInstance, Settings::kKeyThreadAutoStart, &autoStart, sizeof(autoStart));
#else
    (void)aInstance;
    (void)aStartAutomatically;
    return OT_ERROR_NOT_IMPLEMENTED;
#endif
}

bool otThreadIsSingleton(otInstance *aInstance)
{
    return aInstance->mThreadNetif.GetMle().IsSingleton();
}

otError otThreadDiscover(otInstance *aInstance, uint32_t aScanChannels, uint16_t aPanId, bool aJoiner,
                         bool aEnableEui64Filtering, otHandleActiveScanResult aCallback, void *aCallbackContext)
{
    return aInstance->mThreadNetif.GetMle().Discover(aScanChannels, aPanId, aJoiner, aEnableEui64Filtering, aCallback,
                                                     aCallbackContext);
}

bool otThreadIsDiscoverInProgress(otInstance *aInstance)
{
    return aInstance->mThreadNetif.GetMle().IsDiscoverInProgress();
}
