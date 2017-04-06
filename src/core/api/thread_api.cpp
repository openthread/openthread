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
 *   This file implements the OpenThread Thread API.
 */

#define WPP_NAME "thread_api.tmh"

#include "openthread/thread.h"

#include "openthread-instance.h"
#include "common/logging.hpp"
#include "common/settings.hpp"
#include "openthread/platform/settings.h"

using namespace Thread;

#ifdef __cplusplus
extern "C" {
#endif

uint8_t otThreadGetMaxAllowedChildren(otInstance *aInstance)
{
    uint8_t aNumChildren;

    (void)aInstance->mThreadNetif.GetMle().GetChildren(&aNumChildren);

    return aNumChildren;
}

ThreadError otThreadSetMaxAllowedChildren(otInstance *aInstance, uint8_t aMaxChildren)
{
    return aInstance->mThreadNetif.GetMle().SetMaxAllowedChildren(aMaxChildren);
}

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

void otThreadSetExtendedPanId(otInstance *aInstance, const uint8_t *aExtendedPanId)
{
    uint8_t mlPrefix[8];

    aInstance->mThreadNetif.GetMac().SetExtendedPanId(aExtendedPanId);

    mlPrefix[0] = 0xfd;
    memcpy(mlPrefix + 1, aExtendedPanId, 5);
    mlPrefix[6] = 0x00;
    mlPrefix[7] = 0x00;
    aInstance->mThreadNetif.GetMle().SetMeshLocalPrefix(mlPrefix);
}

ThreadError otThreadGetLeaderRloc(otInstance *aInstance, otIp6Address *aAddress)
{
    ThreadError error;

    VerifyOrExit(aAddress != NULL, error = kThreadError_InvalidArgs);

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

ThreadError otThreadSetLinkMode(otInstance *aInstance, otLinkModeConfig aConfig)
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

const uint8_t *otThreadGetMasterKey(otInstance *aInstance, uint8_t *aKeyLength)
{
    return aInstance->mThreadNetif.GetKeyManager().GetMasterKey(aKeyLength);
}

ThreadError otThreadSetMasterKey(otInstance *aInstance, const uint8_t *aKey, uint8_t aKeyLength)
{
    return aInstance->mThreadNetif.GetKeyManager().SetMasterKey(aKey, aKeyLength);
}

const otIp6Address *otThreadGetMeshLocalEid(otInstance *aInstance)
{
    return &aInstance->mThreadNetif.GetMle().GetMeshLocal64();
}

const uint8_t *otThreadGetMeshLocalPrefix(otInstance *aInstance)
{
    return aInstance->mThreadNetif.GetMle().GetMeshLocalPrefix();
}

ThreadError otThreadSetMeshLocalPrefix(otInstance *aInstance, const uint8_t *aMeshLocalPrefix)
{
    return aInstance->mThreadNetif.GetMle().SetMeshLocalPrefix(aMeshLocalPrefix);
}

const char *otThreadGetNetworkName(otInstance *aInstance)
{
    return aInstance->mThreadNetif.GetMac().GetNetworkName();
}

ThreadError otThreadSetNetworkName(otInstance *aInstance, const char *aNetworkName)
{
    return aInstance->mThreadNetif.GetMac().SetNetworkName(aNetworkName);
}

bool otThreadIsRouterRoleEnabled(otInstance *aInstance)
{
    return aInstance->mThreadNetif.GetMle().IsRouterRoleEnabled();
}

void otThreadSetRouterRoleEnabled(otInstance *aInstance, bool aEnabled)
{
    aInstance->mThreadNetif.GetMle().SetRouterRoleEnabled(aEnabled);
}

uint8_t otThreadGetLocalLeaderWeight(otInstance *aInstance)
{
    return aInstance->mThreadNetif.GetMle().GetLeaderWeight();
}

void otThreadSetLocalLeaderWeight(otInstance *aInstance, uint8_t aWeight)
{
    aInstance->mThreadNetif.GetMle().SetLeaderWeight(aWeight);
}

uint32_t otThreadGetLocalLeaderPartitionId(otInstance *aInstance)
{
    return aInstance->mThreadNetif.GetMle().GetLeaderPartitionId();
}

void otThreadSetLocalLeaderPartitionId(otInstance *aInstance, uint32_t aPartitionId)
{
    return aInstance->mThreadNetif.GetMle().SetLeaderPartitionId(aPartitionId);
}

uint16_t otThreadGetJoinerUdpPort(otInstance *aInstance)
{
    return aInstance->mThreadNetif.GetJoinerRouter().GetJoinerUdpPort();
}

ThreadError otThreadSetJoinerUdpPort(otInstance *aInstance, uint16_t aJoinerUdpPort)
{
    return aInstance->mThreadNetif.GetJoinerRouter().SetJoinerUdpPort(aJoinerUdpPort);
}

uint32_t otThreadGetContextIdReuseDelay(otInstance *aInstance)
{
    return aInstance->mThreadNetif.GetNetworkDataLeader().GetContextIdReuseDelay();
}

void otThreadSetContextIdReuseDelay(otInstance *aInstance, uint32_t aDelay)
{
    aInstance->mThreadNetif.GetNetworkDataLeader().SetContextIdReuseDelay(aDelay);
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

uint8_t otThreadGetNetworkIdTimeout(otInstance *aInstance)
{
    return aInstance->mThreadNetif.GetMle().GetNetworkIdTimeout();
}

void otThreadSetNetworkIdTimeout(otInstance *aInstance, uint8_t aTimeout)
{
    aInstance->mThreadNetif.GetMle().SetNetworkIdTimeout((uint8_t)aTimeout);
}

uint8_t otThreadGetRouterUpgradeThreshold(otInstance *aInstance)
{
    return aInstance->mThreadNetif.GetMle().GetRouterUpgradeThreshold();
}

void otThreadSetRouterUpgradeThreshold(otInstance *aInstance, uint8_t aThreshold)
{
    aInstance->mThreadNetif.GetMle().SetRouterUpgradeThreshold(aThreshold);
}

ThreadError otThreadReleaseRouterId(otInstance *aInstance, uint8_t aRouterId)
{
    return aInstance->mThreadNetif.GetMle().ReleaseRouterId(aRouterId);
}

ThreadError otThreadBecomeDetached(otInstance *aInstance)
{
    return aInstance->mThreadNetif.GetMle().BecomeDetached();
}

ThreadError otThreadBecomeChild(otInstance *aInstance, otMleAttachFilter aFilter)
{
    return aInstance->mThreadNetif.GetMle().BecomeChild(aFilter);
}

ThreadError otThreadBecomeRouter(otInstance *aInstance)
{
    ThreadError error = kThreadError_InvalidState;

    switch (aInstance->mThreadNetif.GetMle().GetDeviceState())
    {
    case Mle::kDeviceStateDisabled:
    case Mle::kDeviceStateDetached:
        break;

    case Mle::kDeviceStateChild:
        error = aInstance->mThreadNetif.GetMle().BecomeRouter(ThreadStatusTlv::kHaveChildIdRequest);
        break;

    case Mle::kDeviceStateRouter:
    case Mle::kDeviceStateLeader:
        error = kThreadError_None;
        break;
    }

    return error;
}

ThreadError otThreadBecomeLeader(otInstance *aInstance)
{
    return aInstance->mThreadNetif.GetMle().BecomeLeader();
}

uint8_t otThreadGetRouterDowngradeThreshold(otInstance *aInstance)
{
    return aInstance->mThreadNetif.GetMle().GetRouterDowngradeThreshold();
}

void otThreadSetRouterDowngradeThreshold(otInstance *aInstance, uint8_t aThreshold)
{
    aInstance->mThreadNetif.GetMle().SetRouterDowngradeThreshold(aThreshold);
}

uint8_t otThreadGetRouterSelectionJitter(otInstance *aInstance)
{
    return aInstance->mThreadNetif.GetMle().GetRouterSelectionJitter();
}

void otThreadSetRouterSelectionJitter(otInstance *aInstance, uint8_t aRouterJitter)
{
    aInstance->mThreadNetif.GetMle().SetRouterSelectionJitter(aRouterJitter);
}

ThreadError otThreadGetChildInfoById(otInstance *aInstance, uint16_t aChildId, otChildInfo *aChildInfo)
{
    ThreadError error = kThreadError_None;

    VerifyOrExit(aChildInfo != NULL, error = kThreadError_InvalidArgs);

    error = aInstance->mThreadNetif.GetMle().GetChildInfoById(aChildId, *aChildInfo);

exit:
    return error;
}

ThreadError otThreadGetChildInfoByIndex(otInstance *aInstance, uint8_t aChildIndex, otChildInfo *aChildInfo)
{
    ThreadError error = kThreadError_None;

    VerifyOrExit(aChildInfo != NULL, error = kThreadError_InvalidArgs);

    error = aInstance->mThreadNetif.GetMle().GetChildInfoByIndex(aChildIndex, *aChildInfo);

exit:
    return error;
}

ThreadError otThreadGetNextNeighborInfo(otInstance *aInstance, otNeighborInfoIterator *aIterator, otNeighborInfo *aInfo)
{
    ThreadError error = kThreadError_None;

    VerifyOrExit((aInfo != NULL) && (aIterator != NULL), error = kThreadError_InvalidArgs);

    error = aInstance->mThreadNetif.GetMle().GetNextNeighborInfo(*aIterator, *aInfo);

exit:
    return error;
}

otDeviceRole otThreadGetDeviceRole(otInstance *aInstance)
{
    otDeviceRole rval = kDeviceRoleDisabled;

    switch (aInstance->mThreadNetif.GetMle().GetDeviceState())
    {
    case Mle::kDeviceStateDisabled:
        rval = kDeviceRoleDisabled;
        break;

    case Mle::kDeviceStateDetached:
        rval = kDeviceRoleDetached;
        break;

    case Mle::kDeviceStateChild:
        rval = kDeviceRoleChild;
        break;

    case Mle::kDeviceStateRouter:
        rval = kDeviceRoleRouter;
        break;

    case Mle::kDeviceStateLeader:
        rval = kDeviceRoleLeader;
        break;
    }

    return rval;
}

ThreadError otThreadGetEidCacheEntry(otInstance *aInstance, uint8_t aIndex, otEidCacheEntry *aEntry)
{
    ThreadError error;

    VerifyOrExit(aEntry != NULL, error = kThreadError_InvalidArgs);
    error = aInstance->mThreadNetif.GetAddressResolver().GetEntry(aIndex, *aEntry);

exit:
    return error;
}

ThreadError otThreadGetLeaderData(otInstance *aInstance, otLeaderData *aLeaderData)
{
    ThreadError error;

    VerifyOrExit(aLeaderData != NULL, error = kThreadError_InvalidArgs);

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

uint8_t otThreadGetRouterIdSequence(otInstance *aInstance)
{
    return aInstance->mThreadNetif.GetMle().GetRouterIdSequence();
}

ThreadError otThreadGetRouterInfo(otInstance *aInstance, uint16_t aRouterId, otRouterInfo *aRouterInfo)
{
    ThreadError error = kThreadError_None;

    VerifyOrExit(aRouterInfo != NULL, error = kThreadError_InvalidArgs);

    error = aInstance->mThreadNetif.GetMle().GetRouterInfo(aRouterId, *aRouterInfo);

exit:
    return error;
}

ThreadError otThreadGetParentInfo(otInstance *aInstance, otRouterInfo *aParentInfo)
{
    ThreadError error = kThreadError_None;
    Router *parent;

    VerifyOrExit(aParentInfo != NULL, error = kThreadError_InvalidArgs);

    parent = aInstance->mThreadNetif.GetMle().GetParent();
    memcpy(aParentInfo->mExtAddress.m8, parent->mMacAddr.m8, OT_EXT_ADDRESS_SIZE);

    aParentInfo->mRloc16          = parent->mValid.mRloc16;
    aParentInfo->mRouterId        = Mle::Mle::GetRouterId(parent->mValid.mRloc16);
    aParentInfo->mNextHop         = parent->mNextHop;
    aParentInfo->mPathCost        = parent->mCost;
    aParentInfo->mLinkQualityIn   = parent->mLinkInfo.GetLinkQuality(aInstance->mThreadNetif.GetMac().GetNoiseFloor());
    aParentInfo->mLinkQualityOut  = parent->mLinkQualityOut;
    aParentInfo->mAge             = static_cast<uint8_t>(Timer::MsecToSec(Timer::GetNow() - parent->mLastHeard));
    aParentInfo->mAllocated       = parent->mAllocated;
    aParentInfo->mLinkEstablished = parent->mState == Neighbor::kStateValid;

exit:
    return error;
}

ThreadError otThreadGetParentAverageRssi(otInstance *aInstance, int8_t *aParentRssi)
{
    ThreadError error = kThreadError_None;
    Router *parent;

    VerifyOrExit(aParentRssi != NULL, error = kThreadError_InvalidArgs);

    parent = aInstance->mThreadNetif.GetMle().GetParent();
    *aParentRssi = parent->mLinkInfo.GetAverageRss();

    VerifyOrExit(*aParentRssi != LinkQualityInfo::kUnknownRss, error = kThreadError_Failed);

exit:
    return error;
}

ThreadError otThreadGetParentLastRssi(otInstance *aInstance, int8_t *aLastRssi)
{
    ThreadError error = kThreadError_None;
    Router *parent;

    VerifyOrExit(aLastRssi != NULL, error = kThreadError_InvalidArgs);

    parent = aInstance->mThreadNetif.GetMle().GetParent();
    *aLastRssi = parent->mLinkInfo.GetLastRss();

    VerifyOrExit(*aLastRssi != LinkQualityInfo::kUnknownRss, error = kThreadError_Failed);

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

ThreadError otThreadSetPreferredRouterId(otInstance *aInstance, uint8_t aRouterId)
{
    return aInstance->mThreadNetif.GetMle().SetPreferredRouterId(aRouterId);
}

void otThreadSetReceiveDiagnosticGetCallback(otInstance *aInstance, otReceiveDiagnosticGetCallback aCallback,
                                             void *aCallbackContext)
{
    aInstance->mThreadNetif.GetNetworkDiagnostic().SetReceiveDiagnosticGetCallback(aCallback, aCallbackContext);
}

ThreadError otThreadSendDiagnosticGet(otInstance *aInstance, const otIp6Address *aDestination,
                                      const uint8_t aTlvTypes[], uint8_t aCount)
{
    return aInstance->mThreadNetif.GetNetworkDiagnostic().SendDiagnosticGet(*static_cast<const Ip6::Address *>
                                                                            (aDestination),
                                                                            aTlvTypes,
                                                                            aCount);
}

ThreadError otThreadSendDiagnosticReset(otInstance *aInstance, const otIp6Address *aDestination,
                                        const uint8_t aTlvTypes[], uint8_t aCount)
{
    return aInstance->mThreadNetif.GetNetworkDiagnostic().SendDiagnosticReset(*static_cast<const Ip6::Address *>
                                                                              (aDestination),
                                                                              aTlvTypes,
                                                                              aCount);
}

ThreadError otThreadSetEnabled(otInstance *aInstance, bool aEnabled)
{
    ThreadError error = kThreadError_None;

    otLogFuncEntry();

    if (aEnabled)
    {
        VerifyOrExit(aInstance->mThreadNetif.GetMac().GetPanId() != Mac::kPanIdBroadcast,
                     error = kThreadError_InvalidState);
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

    if (otPlatSettingsGet(aInstance, kKeyThreadAutoStart, 0, &autoStart, &autoStartLength) != kThreadError_None)
    {
        autoStart = 0;
    }

    return autoStart != 0;
#else
    (void)aInstance;
    return false;
#endif
}

ThreadError otThreadSetAutoStart(otInstance *aInstance, bool aStartAutomatically)
{
#if OPENTHREAD_CONFIG_ENABLE_AUTO_START_SUPPORT
    uint8_t autoStart = aStartAutomatically ? 1 : 0;
    return otPlatSettingsSet(aInstance, kKeyThreadAutoStart, &autoStart, sizeof(autoStart));
#else
    (void)aInstance;
    (void)aStartAutomatically;
    return kThreadError_NotImplemented;
#endif
}

bool otThreadIsSingleton(otInstance *aInstance)
{
    return aInstance->mThreadNetif.GetMle().IsSingleton();
}

ThreadError otThreadDiscover(otInstance *aInstance, uint32_t aScanChannels, uint16_t aPanId,
                             otHandleActiveScanResult aCallback, void *aCallbackContext)
{
    return aInstance->mThreadNetif.GetMle().Discover(aScanChannels, aPanId, false, aCallback, aCallbackContext);
}

bool otThreadIsDiscoverInProgress(otInstance *aInstance)
{
    return aInstance->mThreadNetif.GetMle().IsDiscoverInProgress();
}

#ifdef __cplusplus
}  // extern "C"
#endif
