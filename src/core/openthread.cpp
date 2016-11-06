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
 *   This file implements the top-level interface to the OpenThread stack.
 */

#define WPP_NAME "openthread.tmh"

#ifdef OPENTHREAD_CONFIG_FILE
#include OPENTHREAD_CONFIG_FILE
#else
#include <openthread-config.h>
#endif

#include <openthread.h>
#include <common/code_utils.hpp>
#include <common/debug.hpp>
#include <common/logging.hpp>
#include <common/message.hpp>
#include <common/new.hpp>
#include <common/tasklet.hpp>
#include <common/timer.hpp>
#include <crypto/mbedtls.hpp>
#include <net/icmp6.hpp>
#include <net/ip6.hpp>
#include <platform/settings.h>
#include <platform/radio.h>
#include <platform/random.h>
#include <platform/misc.h>
#include <thread/thread_netif.hpp>
#include <thread/thread_uris.hpp>
#include <utils/slaac_address.hpp>
#include <openthread-instance.h>
#include <coap/coap_header.hpp>
#include <coap/coap_client.hpp>

#ifndef OPENTHREAD_MULTIPLE_INSTANCE
static otDEFINE_ALIGNED_VAR(sInstanceRaw, sizeof(otInstance), uint64_t);
otInstance *sInstance = NULL;
#endif

void OT_CDECL operator delete(void *, size_t) throw() { }
void OT_CDECL operator delete(void *) throw() { }

otInstance::otInstance(void) :
    mReceiveIp6DatagramCallback(NULL),
    mReceiveIp6DatagramCallbackContext(NULL),
    mActiveScanCallback(NULL),
    mActiveScanCallbackContext(NULL),
    mDiscoverCallback(NULL),
    mDiscoverCallbackContext(NULL),
    mThreadNetif(mIp6)
{
}

namespace Thread {

#ifdef __cplusplus
extern "C" {
#endif

static void HandleActiveScanResult(void *aContext, Mac::Frame *aFrame);
static void HandleEnergyScanResult(void *aContext, otEnergyScanResult *aResult);
static void HandleMleDiscover(otActiveScanResult *aResult, void *aContext);

void otProcessQueuedTasklets(otInstance *aInstance)
{
    otLogFuncEntry();
    aInstance->mIp6.mTaskletScheduler.ProcessQueuedTasklets();
    otLogFuncExit();
}

bool otAreTaskletsPending(otInstance *aInstance)
{
    return aInstance->mIp6.mTaskletScheduler.AreTaskletsPending();
}

uint8_t otGetChannel(otInstance *aInstance)
{
    return aInstance->mThreadNetif.GetMac().GetChannel();
}

ThreadError otSetChannel(otInstance *aInstance, uint8_t aChannel)
{
    return aInstance->mThreadNetif.GetMac().SetChannel(aChannel);
}

uint8_t otGetMaxAllowedChildren(otInstance *aInstance)
{
    uint8_t aNumChildren;

    (void)aInstance->mThreadNetif.GetMle().GetChildren(&aNumChildren);

    return aNumChildren;
}

ThreadError otSetMaxAllowedChildren(otInstance *aInstance, uint8_t aMaxChildren)
{
    return aInstance->mThreadNetif.GetMle().SetMaxAllowedChildren(aMaxChildren);
}

uint32_t otGetChildTimeout(otInstance *aInstance)
{
    return aInstance->mThreadNetif.GetMle().GetTimeout();
}

void otSetChildTimeout(otInstance *aInstance, uint32_t aTimeout)
{
    aInstance->mThreadNetif.GetMle().SetTimeout(aTimeout);
}

const uint8_t *otGetExtendedAddress(otInstance *aInstance)
{
    return reinterpret_cast<const uint8_t *>(aInstance->mThreadNetif.GetMac().GetExtAddress());
}

ThreadError otSetExtendedAddress(otInstance *aInstance, const otExtAddress *aExtAddress)
{
    ThreadError error = kThreadError_None;

    VerifyOrExit(aExtAddress != NULL, error = kThreadError_InvalidArgs);

    aInstance->mThreadNetif.GetMac().SetExtAddress(*static_cast<const Mac::ExtAddress *>(aExtAddress));

    SuccessOrExit(error = aInstance->mThreadNetif.GetMle().UpdateLinkLocalAddress());

exit:
    return error;
}

const uint8_t *otGetExtendedPanId(otInstance *aInstance)
{
    return aInstance->mThreadNetif.GetMac().GetExtendedPanId();
}

void otSetExtendedPanId(otInstance *aInstance, const uint8_t *aExtendedPanId)
{
    uint8_t mlPrefix[8];

    aInstance->mThreadNetif.GetMac().SetExtendedPanId(aExtendedPanId);

    mlPrefix[0] = 0xfd;
    memcpy(mlPrefix + 1, aExtendedPanId, 5);
    mlPrefix[6] = 0x00;
    mlPrefix[7] = 0x00;
    aInstance->mThreadNetif.GetMle().SetMeshLocalPrefix(mlPrefix);
}

void otGetFactoryAssignedIeeeEui64(otInstance *aInstance, otExtAddress *aEui64)
{
    otPlatRadioGetIeeeEui64(aInstance, aEui64->m8);
}

void otGetHashMacAddress(otInstance *aInstance, otExtAddress *aHashMacAddress)
{
    aInstance->mThreadNetif.GetMac().GetHashMacAddress(static_cast<Mac::ExtAddress *>(aHashMacAddress));
}

ThreadError otGetLeaderRloc(otInstance *aInstance, otIp6Address *aAddress)
{
    ThreadError error;

    VerifyOrExit(aAddress != NULL, error = kThreadError_InvalidArgs);

    error = aInstance->mThreadNetif.GetMle().GetLeaderAddress(*static_cast<Ip6::Address *>(aAddress));

exit:
    return error;
}

otLinkModeConfig otGetLinkMode(otInstance *aInstance)
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

ThreadError otSetLinkMode(otInstance *aInstance, otLinkModeConfig aConfig)
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

const uint8_t *otGetMasterKey(otInstance *aInstance, uint8_t *aKeyLength)
{
    return aInstance->mThreadNetif.GetKeyManager().GetMasterKey(aKeyLength);
}

ThreadError otSetMasterKey(otInstance *aInstance, const uint8_t *aKey, uint8_t aKeyLength)
{
    return aInstance->mThreadNetif.GetKeyManager().SetMasterKey(aKey, aKeyLength);
}

int8_t otGetMaxTransmitPower(otInstance *aInstance)
{
    return aInstance->mThreadNetif.GetMac().GetMaxTransmitPower();
}

void otSetMaxTransmitPower(otInstance *aInstance, int8_t aPower)
{
    aInstance->mThreadNetif.GetMac().SetMaxTransmitPower(aPower);
}

const otIp6Address *otGetMeshLocalEid(otInstance *aInstance)
{
    return &aInstance->mThreadNetif.GetMle().GetMeshLocal64();
}

const uint8_t *otGetMeshLocalPrefix(otInstance *aInstance)
{
    return aInstance->mThreadNetif.GetMle().GetMeshLocalPrefix();
}

ThreadError otSetMeshLocalPrefix(otInstance *aInstance, const uint8_t *aMeshLocalPrefix)
{
    return aInstance->mThreadNetif.GetMle().SetMeshLocalPrefix(aMeshLocalPrefix);
}

ThreadError otGetNetworkDataLeader(otInstance *aInstance, bool aStable, uint8_t *aData, uint8_t *aDataLength)
{
    ThreadError error = kThreadError_None;

    VerifyOrExit(aData != NULL && aDataLength != NULL, error = kThreadError_InvalidArgs);

    aInstance->mThreadNetif.GetNetworkDataLeader().GetNetworkData(aStable, aData, *aDataLength);

exit:
    return error;
}

ThreadError otGetNetworkDataLocal(otInstance *aInstance, bool aStable, uint8_t *aData, uint8_t *aDataLength)
{
    ThreadError error = kThreadError_None;

    VerifyOrExit(aData != NULL && aDataLength != NULL, error = kThreadError_InvalidArgs);

    aInstance->mThreadNetif.GetNetworkDataLocal().GetNetworkData(aStable, aData, *aDataLength);

exit:
    return error;
}

const char *otGetNetworkName(otInstance *aInstance)
{
    return aInstance->mThreadNetif.GetMac().GetNetworkName();
}

ThreadError otSetNetworkName(otInstance *aInstance, const char *aNetworkName)
{
    return aInstance->mThreadNetif.GetMac().SetNetworkName(aNetworkName);
}

otPanId otGetPanId(otInstance *aInstance)
{
    return aInstance->mThreadNetif.GetMac().GetPanId();
}

ThreadError otSetPanId(otInstance *aInstance, otPanId aPanId)
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

bool otIsRouterRoleEnabled(otInstance *aInstance)
{
    return aInstance->mThreadNetif.GetMle().IsRouterRoleEnabled();
}

void otSetRouterRoleEnabled(otInstance *aInstance, bool aEnabled)
{
    aInstance->mThreadNetif.GetMle().SetRouterRoleEnabled(aEnabled);
}

otShortAddress otGetShortAddress(otInstance *aInstance)
{
    return aInstance->mThreadNetif.GetMac().GetShortAddress();
}

uint8_t otGetLocalLeaderWeight(otInstance *aInstance)
{
    return aInstance->mThreadNetif.GetMle().GetLeaderWeight();
}

void otSetLocalLeaderWeight(otInstance *aInstance, uint8_t aWeight)
{
    aInstance->mThreadNetif.GetMle().SetLeaderWeight(aWeight);
}

uint32_t otGetLocalLeaderPartitionId(otInstance *aInstance)
{
    return aInstance->mThreadNetif.GetMle().GetLeaderPartitionId();
}

void otSetLocalLeaderPartitionId(otInstance *aInstance, uint32_t aPartitionId)
{
    return aInstance->mThreadNetif.GetMle().SetLeaderPartitionId(aPartitionId);
}

uint16_t otGetJoinerUdpPort(otInstance *aInstance)
{
    return aInstance->mThreadNetif.GetJoinerRouter().GetJoinerUdpPort();
}

ThreadError otSetJoinerUdpPort(otInstance *aInstance, uint16_t aJoinerUdpPort)
{
    return aInstance->mThreadNetif.GetJoinerRouter().SetJoinerUdpPort(aJoinerUdpPort);
}

ThreadError otAddBorderRouter(otInstance *aInstance, const otBorderRouterConfig *aConfig)
{
    uint8_t flags = 0;

    if (aConfig->mPreferred)
    {
        flags |= NetworkData::BorderRouterEntry::kPreferredFlag;
    }

    if (aConfig->mSlaac)
    {
        flags |= NetworkData::BorderRouterEntry::kSlaacFlag;
    }

    if (aConfig->mDhcp)
    {
        flags |= NetworkData::BorderRouterEntry::kDhcpFlag;
    }

    if (aConfig->mConfigure)
    {
        flags |= NetworkData::BorderRouterEntry::kConfigureFlag;
    }

    if (aConfig->mDefaultRoute)
    {
        flags |= NetworkData::BorderRouterEntry::kDefaultRouteFlag;
    }

    if (aConfig->mOnMesh)
    {
        flags |= NetworkData::BorderRouterEntry::kOnMeshFlag;
    }

    return aInstance->mThreadNetif.GetNetworkDataLocal().AddOnMeshPrefix(aConfig->mPrefix.mPrefix.mFields.m8,
                                                                         aConfig->mPrefix.mLength,
                                                                         aConfig->mPreference, flags, aConfig->mStable);
}

ThreadError otRemoveBorderRouter(otInstance *aInstance, const otIp6Prefix *aPrefix)
{
    return aInstance->mThreadNetif.GetNetworkDataLocal().RemoveOnMeshPrefix(aPrefix->mPrefix.mFields.m8, aPrefix->mLength);
}

ThreadError otGetNextOnMeshPrefix(otInstance *aInstance, bool aLocal, otNetworkDataIterator *aIterator,
                                  otBorderRouterConfig *aConfig)
{
    ThreadError error = kThreadError_None;

    VerifyOrExit(aIterator && aConfig, error = kThreadError_InvalidArgs);

    if (aLocal)
    {
        error = aInstance->mThreadNetif.GetNetworkDataLocal().GetNextOnMeshPrefix(aIterator, aConfig);
    }
    else
    {
        error = aInstance->mThreadNetif.GetNetworkDataLeader().GetNextOnMeshPrefix(aIterator, aConfig);
    }

exit:
    return error;
}

ThreadError otAddExternalRoute(otInstance *aInstance, const otExternalRouteConfig *aConfig)
{
    return aInstance->mThreadNetif.GetNetworkDataLocal().AddHasRoutePrefix(aConfig->mPrefix.mPrefix.mFields.m8,
                                                                           aConfig->mPrefix.mLength,
                                                                           aConfig->mPreference, aConfig->mStable);
}

ThreadError otRemoveExternalRoute(otInstance *aInstance, const otIp6Prefix *aPrefix)
{
    return aInstance->mThreadNetif.GetNetworkDataLocal().RemoveHasRoutePrefix(aPrefix->mPrefix.mFields.m8,
                                                                              aPrefix->mLength);
}

ThreadError otSendServerData(otInstance *aInstance)
{
    return aInstance->mThreadNetif.GetNetworkDataLocal().SendServerDataNotification();
}

ThreadError otAddUnsecurePort(otInstance *aInstance, uint16_t aPort)
{
    return aInstance->mThreadNetif.GetIp6Filter().AddUnsecurePort(aPort);
}

ThreadError otRemoveUnsecurePort(otInstance *aInstance, uint16_t aPort)
{
    return aInstance->mThreadNetif.GetIp6Filter().RemoveUnsecurePort(aPort);
}

const uint16_t *otGetUnsecurePorts(otInstance *aInstance, uint8_t *aNumEntries)
{
    return aInstance->mThreadNetif.GetIp6Filter().GetUnsecurePorts(*aNumEntries);
}

uint32_t otGetContextIdReuseDelay(otInstance *aInstance)
{
    return aInstance->mThreadNetif.GetNetworkDataLeader().GetContextIdReuseDelay();
}

void otSetContextIdReuseDelay(otInstance *aInstance, uint32_t aDelay)
{
    aInstance->mThreadNetif.GetNetworkDataLeader().SetContextIdReuseDelay(aDelay);
}

uint32_t otGetKeySequenceCounter(otInstance *aInstance)
{
    return aInstance->mThreadNetif.GetKeyManager().GetCurrentKeySequence();
}

void otSetKeySequenceCounter(otInstance *aInstance, uint32_t aKeySequenceCounter)
{
    aInstance->mThreadNetif.GetKeyManager().SetCurrentKeySequence(aKeySequenceCounter);
}

uint32_t otGetKeySwitchGuardTime(otInstance *aInstance)
{
    return aInstance->mThreadNetif.GetKeyManager().GetKeySwitchGuardTime();
}

void otSetKeySwitchGuardTime(otInstance *aInstance, uint32_t aKeySwitchGuardTime)
{
    aInstance->mThreadNetif.GetKeyManager().SetKeySwitchGuardTime(aKeySwitchGuardTime);
}

uint8_t otGetNetworkIdTimeout(otInstance *aInstance)
{
    return aInstance->mThreadNetif.GetMle().GetNetworkIdTimeout();
}

void otSetNetworkIdTimeout(otInstance *aInstance, uint8_t aTimeout)
{
    aInstance->mThreadNetif.GetMle().SetNetworkIdTimeout((uint8_t)aTimeout);
}

uint8_t otGetRouterUpgradeThreshold(otInstance *aInstance)
{
    return aInstance->mThreadNetif.GetMle().GetRouterUpgradeThreshold();
}

void otSetRouterUpgradeThreshold(otInstance *aInstance, uint8_t aThreshold)
{
    aInstance->mThreadNetif.GetMle().SetRouterUpgradeThreshold(aThreshold);
}

ThreadError otReleaseRouterId(otInstance *aInstance, uint8_t aRouterId)
{
    return aInstance->mThreadNetif.GetMle().ReleaseRouterId(aRouterId);
}

ThreadError otAddMacWhitelist(otInstance *aInstance, const uint8_t *aExtAddr)
{
    ThreadError error = kThreadError_None;

    if (aInstance->mThreadNetif.GetMac().GetWhitelist().Add(*reinterpret_cast<const Mac::ExtAddress *>(aExtAddr)) == NULL)
    {
        error = kThreadError_NoBufs;
    }

    return error;
}

ThreadError otAddMacWhitelistRssi(otInstance *aInstance, const uint8_t *aExtAddr, int8_t aRssi)
{
    ThreadError error = kThreadError_None;
    otMacWhitelistEntry *entry;

    entry = aInstance->mThreadNetif.GetMac().GetWhitelist().Add(*reinterpret_cast<const Mac::ExtAddress *>(aExtAddr));
    VerifyOrExit(entry != NULL, error = kThreadError_NoBufs);
    aInstance->mThreadNetif.GetMac().GetWhitelist().SetFixedRssi(*entry, aRssi);

exit:
    return error;
}

void otRemoveMacWhitelist(otInstance *aInstance, const uint8_t *aExtAddr)
{
    aInstance->mThreadNetif.GetMac().GetWhitelist().Remove(*reinterpret_cast<const Mac::ExtAddress *>(aExtAddr));
}

void otClearMacWhitelist(otInstance *aInstance)
{
    aInstance->mThreadNetif.GetMac().GetWhitelist().Clear();
}

ThreadError otGetMacWhitelistEntry(otInstance *aInstance, uint8_t aIndex, otMacWhitelistEntry *aEntry)
{
    ThreadError error = kThreadError_None;

    VerifyOrExit(aEntry != NULL, error = kThreadError_InvalidArgs);
    error = aInstance->mThreadNetif.GetMac().GetWhitelist().GetEntry(aIndex, *aEntry);

exit:
    return error;
}

void otDisableMacWhitelist(otInstance *aInstance)
{
    aInstance->mThreadNetif.GetMac().GetWhitelist().Disable();
}

void otEnableMacWhitelist(otInstance *aInstance)
{
    aInstance->mThreadNetif.GetMac().GetWhitelist().Enable();
}

bool otIsMacWhitelistEnabled(otInstance *aInstance)
{
    return aInstance->mThreadNetif.GetMac().GetWhitelist().IsEnabled();
}

ThreadError otBecomeDetached(otInstance *aInstance)
{
    return aInstance->mThreadNetif.GetMle().BecomeDetached();
}

ThreadError otBecomeChild(otInstance *aInstance, otMleAttachFilter aFilter)
{
    return aInstance->mThreadNetif.GetMle().BecomeChild(aFilter);
}

ThreadError otBecomeRouter(otInstance *aInstance)
{
    return aInstance->mThreadNetif.GetMle().BecomeRouter(ThreadStatusTlv::kTooFewRouters);
}

ThreadError otBecomeLeader(otInstance *aInstance)
{
    return aInstance->mThreadNetif.GetMle().BecomeLeader();
}

ThreadError otAddMacBlacklist(otInstance *aInstance, const uint8_t *aExtAddr)
{
    ThreadError error = kThreadError_None;

    if (aInstance->mThreadNetif.GetMac().GetBlacklist().Add(*reinterpret_cast<const Mac::ExtAddress *>(aExtAddr)) == NULL)
    {
        error = kThreadError_NoBufs;
    }

    return error;
}

void otRemoveMacBlacklist(otInstance *aInstance, const uint8_t *aExtAddr)
{
    aInstance->mThreadNetif.GetMac().GetBlacklist().Remove(*reinterpret_cast<const Mac::ExtAddress *>(aExtAddr));
}

void otClearMacBlacklist(otInstance *aInstance)
{
    aInstance->mThreadNetif.GetMac().GetBlacklist().Clear();
}

ThreadError otGetMacBlacklistEntry(otInstance *aInstance, uint8_t aIndex, otMacBlacklistEntry *aEntry)
{
    ThreadError error = kThreadError_None;

    VerifyOrExit(aEntry != NULL, error = kThreadError_InvalidArgs);
    error = aInstance->mThreadNetif.GetMac().GetBlacklist().GetEntry(aIndex, *aEntry);

exit:
    return error;
}

void otDisableMacBlacklist(otInstance *aInstance)
{
    aInstance->mThreadNetif.GetMac().GetBlacklist().Disable();
}

void otEnableMacBlacklist(otInstance *aInstance)
{
    aInstance->mThreadNetif.GetMac().GetBlacklist().Enable();
}

bool otIsMacBlacklistEnabled(otInstance *aInstance)
{
    return aInstance->mThreadNetif.GetMac().GetBlacklist().IsEnabled();
}

ThreadError otGetAssignLinkQuality(otInstance *aInstance, const uint8_t *aExtAddr, uint8_t *aLinkQuality)
{
    Mac::ExtAddress extAddress;

    memset(&extAddress, 0, sizeof(extAddress));
    memcpy(extAddress.m8, aExtAddr, OT_EXT_ADDRESS_SIZE);

    return aInstance->mThreadNetif.GetMle().GetAssignLinkQuality(extAddress, *aLinkQuality);
}

void otSetAssignLinkQuality(otInstance *aInstance, const uint8_t *aExtAddr, uint8_t aLinkQuality)
{
    Mac::ExtAddress extAddress;

    memset(&extAddress, 0, sizeof(extAddress));
    memcpy(extAddress.m8, aExtAddr, OT_EXT_ADDRESS_SIZE);

    aInstance->mThreadNetif.GetMle().SetAssignLinkQuality(extAddress, aLinkQuality);
}

void otPlatformReset(otInstance *aInstance)
{
    otPlatReset(aInstance);
}

void otFactoryReset(otInstance *aInstance)
{
    otPlatSettingsWipe(aInstance);
    otPlatReset(aInstance);
}

uint8_t otGetRouterDowngradeThreshold(otInstance *aInstance)
{
    return aInstance->mThreadNetif.GetMle().GetRouterDowngradeThreshold();
}

void otSetRouterDowngradeThreshold(otInstance *aInstance, uint8_t aThreshold)
{
    aInstance->mThreadNetif.GetMle().SetRouterDowngradeThreshold(aThreshold);
}

uint8_t otGetRouterSelectionJitter(otInstance *aInstance)
{
    return aInstance->mThreadNetif.GetMle().GetRouterSelectionJitter();
}

void otSetRouterSelectionJitter(otInstance *aInstance, uint8_t aRouterJitter)
{
    aInstance->mThreadNetif.GetMle().SetRouterSelectionJitter(aRouterJitter);
}

ThreadError otGetChildInfoById(otInstance *aInstance, uint16_t aChildId, otChildInfo *aChildInfo)
{
    ThreadError error = kThreadError_None;

    VerifyOrExit(aChildInfo != NULL, error = kThreadError_InvalidArgs);

    error = aInstance->mThreadNetif.GetMle().GetChildInfoById(aChildId, *aChildInfo);

exit:
    return error;
}

ThreadError otGetChildInfoByIndex(otInstance *aInstance, uint8_t aChildIndex, otChildInfo *aChildInfo)
{
    ThreadError error = kThreadError_None;

    VerifyOrExit(aChildInfo != NULL, error = kThreadError_InvalidArgs);

    error = aInstance->mThreadNetif.GetMle().GetChildInfoByIndex(aChildIndex, *aChildInfo);

exit:
    return error;
}

ThreadError otGetNextNeighborInfo(otInstance *aInstance, otNeighborInfoIterator *aIterator, otNeighborInfo *aInfo)
{
    ThreadError error = kThreadError_None;

    VerifyOrExit((aInfo != NULL) && (aIterator != NULL), error = kThreadError_InvalidArgs);

    error = aInstance->mThreadNetif.GetMle().GetNextNeighborInfo(*aIterator, *aInfo);

exit:
    return error;
}

otDeviceRole otGetDeviceRole(otInstance *aInstance)
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

ThreadError otGetEidCacheEntry(otInstance *aInstance, uint8_t aIndex, otEidCacheEntry *aEntry)
{
    ThreadError error;

    VerifyOrExit(aEntry != NULL, error = kThreadError_InvalidArgs);
    error = aInstance->mThreadNetif.GetAddressResolver().GetEntry(aIndex, *aEntry);

exit:
    return error;
}

ThreadError otGetLeaderData(otInstance *aInstance, otLeaderData *aLeaderData)
{
    ThreadError error;

    VerifyOrExit(aLeaderData != NULL, error = kThreadError_InvalidArgs);

    error = aInstance->mThreadNetif.GetMle().GetLeaderData(*aLeaderData);

exit:
    return error;
}

uint8_t otGetLeaderRouterId(otInstance *aInstance)
{
    return aInstance->mThreadNetif.GetMle().GetLeaderDataTlv().GetLeaderRouterId();
}

uint8_t otGetLeaderWeight(otInstance *aInstance)
{
    return aInstance->mThreadNetif.GetMle().GetLeaderDataTlv().GetWeighting();
}

uint8_t otGetNetworkDataVersion(otInstance *aInstance)
{
    return aInstance->mThreadNetif.GetMle().GetLeaderDataTlv().GetDataVersion();
}

uint32_t otGetPartitionId(otInstance *aInstance)
{
    return aInstance->mThreadNetif.GetMle().GetLeaderDataTlv().GetPartitionId();
}

uint16_t otGetRloc16(otInstance *aInstance)
{
    return aInstance->mThreadNetif.GetMle().GetRloc16();
}

uint8_t otGetRouterIdSequence(otInstance *aInstance)
{
    return aInstance->mThreadNetif.GetMle().GetRouterIdSequence();
}

ThreadError otGetRouterInfo(otInstance *aInstance, uint16_t aRouterId, otRouterInfo *aRouterInfo)
{
    ThreadError error = kThreadError_None;

    VerifyOrExit(aRouterInfo != NULL, error = kThreadError_InvalidArgs);

    error = aInstance->mThreadNetif.GetMle().GetRouterInfo(aRouterId, *aRouterInfo);

exit:
    return error;
}

ThreadError otGetParentInfo(otInstance *aInstance, otRouterInfo *aParentInfo)
{
    ThreadError error = kThreadError_None;
    Router *parent;

    VerifyOrExit(aParentInfo != NULL, error = kThreadError_InvalidArgs);

    parent = aInstance->mThreadNetif.GetMle().GetParent();
    memcpy(aParentInfo->mExtAddress.m8, parent->mMacAddr.m8, OT_EXT_ADDRESS_SIZE);
    aParentInfo->mRloc16 = parent->mValid.mRloc16;

exit:
    return error;
}

uint8_t otGetStableNetworkDataVersion(otInstance *aInstance)
{
    return aInstance->mThreadNetif.GetMle().GetLeaderDataTlv().GetStableDataVersion();
}

void otSetLinkPcapCallback(otInstance *aInstance, otLinkPcapCallback aPcapCallback, void *aCallbackContext)
{
    aInstance->mThreadNetif.GetMac().SetPcapCallback(aPcapCallback, aCallbackContext);
}

bool otIsLinkPromiscuous(otInstance *aInstance)
{
    return aInstance->mThreadNetif.GetMac().IsPromiscuous();
}

ThreadError otSetLinkPromiscuous(otInstance *aInstance, bool aPromiscuous)
{
    ThreadError error = kThreadError_None;

    // cannot enable IEEE 802.15.4 promiscuous mode if the Thread interface is enabled
    VerifyOrExit(aInstance->mThreadNetif.IsUp() == false, error = kThreadError_InvalidState);

    aInstance->mThreadNetif.GetMac().SetPromiscuous(aPromiscuous);

exit:
    return error;
}

const otMacCounters *otGetMacCounters(otInstance *aInstance)
{
    return &aInstance->mThreadNetif.GetMac().GetCounters();
}

void otGetMessageBufferInfo(otInstance *aInstance, otBufferInfo *aBufferInfo)
{
    aBufferInfo->mTotalBuffers = OPENTHREAD_CONFIG_NUM_MESSAGE_BUFFERS;

    aBufferInfo->mFreeBuffers = aInstance->mThreadNetif.GetIp6().mMessagePool.GetFreeBufferCount();

    aInstance->mThreadNetif.GetMeshForwarder().GetSendQueue().GetInfo(aBufferInfo->m6loSendMessages,
                                                                      aBufferInfo->m6loSendBuffers);

    aInstance->mThreadNetif.GetMeshForwarder().GetReassemblyQueue().GetInfo(aBufferInfo->m6loReassemblyMessages,
                                                                            aBufferInfo->m6loReassemblyBuffers);

    aInstance->mThreadNetif.GetMeshForwarder().GetResolvingQueue().GetInfo(aBufferInfo->mArpMessages,
                                                                           aBufferInfo->mArpBuffers);

    aInstance->mThreadNetif.GetIp6().GetSendQueue().GetInfo(aBufferInfo->mIp6Messages,
                                                            aBufferInfo->mIp6Buffers);

    aInstance->mThreadNetif.GetIp6().mMpl.GetBufferedMessageSet().GetInfo(aBufferInfo->mMplMessages,
                                                                          aBufferInfo->mMplBuffers);

    aInstance->mThreadNetif.GetMle().GetMessageQueue().GetInfo(aBufferInfo->mMleMessages,
                                                               aBufferInfo->mMleBuffers);

    aInstance->mThreadNetif.GetCoapClient().GetRequestMessages().GetInfo(aBufferInfo->mCoapClientMessages,
                                                                         aBufferInfo->mCoapClientBuffers);
}

bool otIsIp6AddressEqual(const otIp6Address *a, const otIp6Address *b)
{
    return *static_cast<const Ip6::Address *>(a) == *static_cast<const Ip6::Address *>(b);
}

ThreadError otIp6AddressFromString(const char *str, otIp6Address *address)
{
    return static_cast<Ip6::Address *>(address)->FromString(str);
}

const otNetifAddress *otGetUnicastAddresses(otInstance *aInstance)
{
    return aInstance->mThreadNetif.GetUnicastAddresses();
}

ThreadError otAddUnicastAddress(otInstance *aInstance, const otNetifAddress *address)
{
    return aInstance->mThreadNetif.AddExternalUnicastAddress(*static_cast<const Ip6::NetifUnicastAddress *>(address));
}

ThreadError otRemoveUnicastAddress(otInstance *aInstance, const otIp6Address *address)
{
    return aInstance->mThreadNetif.RemoveExternalUnicastAddress(*static_cast<const Ip6::Address *>(address));
}

#if OPENTHREAD_ENABLE_DHCP6_CLIENT
void otDhcp6ClientUpdate(otInstance *aInstance, otNetifAddress *aAddresses, uint32_t aNumAddresses, void *aContext)
{
    aInstance->mThreadNetif.GetDhcp6Client().UpdateAddresses(aInstance, aAddresses, aNumAddresses, aContext);
}
#endif  // OPENTHREAD_ENABLE_DHCP6_CLIENT

const otNetifMulticastAddress *otGetMulticastAddresses(otInstance *aInstance)
{
    return aInstance->mThreadNetif.GetMulticastAddresses();
}

ThreadError otSubscribeMulticastAddress(otInstance *aInstance, const otIp6Address *aAddress)
{
    return aInstance->mThreadNetif.SubscribeExternalMulticast(*static_cast<const Ip6::Address *>(aAddress));
}

ThreadError otUnsubscribeMulticastAddress(otInstance *aInstance, const otIp6Address *aAddress)
{
    return aInstance->mThreadNetif.UnsubscribeExternalMulticast(*static_cast<const Ip6::Address *>(aAddress));
}

bool otIsMulticastPromiscuousModeEnabled(otInstance *aInstance)
{
    return aInstance->mThreadNetif.IsMulticastPromiscuousModeEnabled();
}

void otEnableMulticastPromiscuousMode(otInstance *aInstance)
{
    aInstance->mThreadNetif.EnableMulticastPromiscuousMode();
}

void otDisableMulticastPromiscuousMode(otInstance *aInstance)
{
    aInstance->mThreadNetif.DisableMulticastPromiscuousMode();
}

void otSlaacUpdate(otInstance *aInstance, otNetifAddress *aAddresses, uint32_t aNumAddresses,
                   otSlaacIidCreate aIidCreate, void *aContext)
{
    Utils::Slaac::UpdateAddresses(aInstance, aAddresses, aNumAddresses, aIidCreate, aContext);
}

ThreadError otCreateRandomIid(otInstance *aInstance, otNetifAddress *aAddress, void *aContext)
{
    return Utils::Slaac::CreateRandomIid(aInstance, aAddress, aContext);
}

ThreadError otCreateMacIid(otInstance *aInstance, otNetifAddress *aAddress, void *)
{
    memcpy(&aAddress->mAddress.mFields.m8[OT_IP6_ADDRESS_SIZE - OT_IP6_IID_SIZE],
           aInstance->mThreadNetif.GetMac().GetExtAddress(), OT_IP6_IID_SIZE);
    aAddress->mAddress.mFields.m8[OT_IP6_ADDRESS_SIZE - OT_IP6_IID_SIZE] ^= 0x02;

    return kThreadError_None;
}

ThreadError otCreateSemanticallyOpaqueIid(otInstance *aInstance, otNetifAddress *aAddress, void *aContext)
{
    return static_cast<Utils::SemanticallyOpaqueIidGenerator *>(aContext)->CreateIid(aInstance, aAddress);
}

ThreadError otSetStateChangedCallback(otInstance *aInstance, otStateChangedCallback aCallback, void *aCallbackContext)
{
    ThreadError error = kThreadError_NoBufs;

    for (size_t i = 0; i < OPENTHREAD_CONFIG_MAX_STATECHANGE_HANDLERS; i++)
    {
        if (aInstance->mNetifCallback[i].IsFree())
        {
            aInstance->mNetifCallback[i].Set(aCallback, aCallbackContext);
            error = aInstance->mThreadNetif.RegisterCallback(aInstance->mNetifCallback[i]);
            break;
        }
    }

    return error;
}

void otRemoveStateChangeCallback(otInstance *aInstance, otStateChangedCallback aCallback, void *aCallbackContext)
{
    for (size_t i = 0; i < OPENTHREAD_CONFIG_MAX_STATECHANGE_HANDLERS; i++)
    {
        if (aInstance->mNetifCallback[i].IsServing(aCallback, aCallbackContext))
        {
            aInstance->mThreadNetif.RemoveCallback(aInstance->mNetifCallback[i]);
            aInstance->mNetifCallback[i].Free();
            break;
        }
    }
}

const char *otGetVersionString(void)
{
    static const char sVersion[] =
        PACKAGE_NAME "/" PACKAGE_VERSION
#ifdef  PLATFORM_INFO
        "; " PLATFORM_INFO
#endif
#if defined(__DATE__)
        "; " __DATE__ " " __TIME__;
#else
        ;
#endif

    return sVersion;
}

uint32_t otGetPollPeriod(otInstance *aInstance)
{
    return aInstance->mThreadNetif.GetMeshForwarder().GetAssignPollPeriod();
}

void otSetPollPeriod(otInstance *aInstance, uint32_t aPollPeriod)
{
    aInstance->mThreadNetif.GetMeshForwarder().SetAssignPollPeriod(aPollPeriod);
}

ThreadError otSetPreferredRouterId(otInstance *aInstance, uint8_t aRouterId)
{
    return aInstance->mThreadNetif.GetMle().SetPreferredRouterId(aRouterId);
}

#ifdef OPENTHREAD_MULTIPLE_INSTANCE

otInstance *otInstanceInit(void *aInstanceBuffer, size_t *aInstanceBufferSize)
{
    otInstance *aInstance = NULL;

    otLogFuncEntry();
    otLogInfoApi("otInstanceInit");

    VerifyOrExit(aInstanceBufferSize != NULL, ;);

    // Make sure the input buffer is big enough
    VerifyOrExit(sizeof(otInstance) <= *aInstanceBufferSize, *aInstanceBufferSize = sizeof(otInstance));

    VerifyOrExit(aInstanceBuffer != NULL, ;);

    // Construct the context
    aInstance = new(aInstanceBuffer)otInstance();

    // restore datasets
    otPlatSettingsInit(aInstance);

    aInstance->mThreadNetif.GetActiveDataset().Restore();

    aInstance->mThreadNetif.GetPendingDataset().Restore();

exit:

    otLogFuncExit();
    return aInstance;
}

#else

otInstance *otInstanceInit()
{
    otLogFuncEntry();

    otLogInfoApi("otInstanceInit");

    VerifyOrExit(sInstance == NULL, ;);

    // Construct the context
    sInstance = new(&sInstanceRaw)otInstance();

    // restore datasets
    otPlatSettingsInit(sInstance);

    sInstance->mThreadNetif.GetActiveDataset().Restore();

    sInstance->mThreadNetif.GetPendingDataset().Restore();

exit:

    otLogFuncExit();
    return sInstance;
}

#endif

ThreadError otSendDiagnosticGet(otInstance *aInstance, const otIp6Address *aDestination, const uint8_t aTlvTypes[],
                                uint8_t aCount)
{
    return aInstance->mThreadNetif.GetNetworkDiagnostic().SendDiagnosticGet(*static_cast<const Ip6::Address *>
                                                                            (aDestination),
                                                                            aTlvTypes,
                                                                            aCount);
}

ThreadError otSendDiagnosticReset(otInstance *aInstance, const otIp6Address *aDestination, const uint8_t aTlvTypes[],
                                  uint8_t aCount)
{
    return aInstance->mThreadNetif.GetNetworkDiagnostic().SendDiagnosticReset(*static_cast<const Ip6::Address *>
                                                                              (aDestination),
                                                                              aTlvTypes,
                                                                              aCount);
}

void otInstanceFinalize(otInstance *aInstance)
{
    otLogFuncEntry();

    // Ensure we are disabled
    (void)otThreadStop(aInstance);
    (void)otInterfaceDown(aInstance);

    // Free the otInstance structure
    delete aInstance;

#ifndef OPENTHREAD_MULTIPLE_INSTANCE
    sInstance = NULL;
#endif
    otLogFuncExit();
}

ThreadError otInterfaceUp(otInstance *aInstance)
{
    ThreadError error = kThreadError_None;

    otLogFuncEntry();

    error = aInstance->mThreadNetif.Up();

    otLogFuncExitErr(error);
    return error;
}

ThreadError otInterfaceDown(otInstance *aInstance)
{
    ThreadError error = kThreadError_None;

    otLogFuncEntry();

    error = aInstance->mThreadNetif.Down();

    otLogFuncExitErr(error);
    return error;
}

bool otIsInterfaceUp(otInstance *aInstance)
{
    return aInstance->mThreadNetif.IsUp();
}

ThreadError otThreadStart(otInstance *aInstance)
{
    ThreadError error = kThreadError_None;

    otLogFuncEntry();

    VerifyOrExit(aInstance->mThreadNetif.GetMac().GetPanId() != Mac::kPanIdBroadcast, error = kThreadError_InvalidState);

    error = aInstance->mThreadNetif.GetMle().Start(true);

exit:
    otLogFuncExitErr(error);
    return error;
}

ThreadError otThreadStop(otInstance *aInstance)
{
    ThreadError error = kThreadError_None;

    otLogFuncEntry();

    error = aInstance->mThreadNetif.GetMle().Stop(true);

    otLogFuncExitErr(error);
    return error;
}

bool otIsSingleton(otInstance *aInstance)
{
    return aInstance->mThreadNetif.GetMle().IsSingleton();
}

ThreadError otActiveScan(otInstance *aInstance, uint32_t aScanChannels, uint16_t aScanDuration,
                         otHandleActiveScanResult aCallback, void *aCallbackContext)
{
    aInstance->mActiveScanCallback = aCallback;
    aInstance->mActiveScanCallbackContext = aCallbackContext;
    return aInstance->mThreadNetif.GetMac().ActiveScan(aScanChannels, aScanDuration, &HandleActiveScanResult, aInstance);
}

bool otIsActiveScanInProgress(otInstance *aInstance)
{
    return aInstance->mThreadNetif.GetMac().IsActiveScanInProgress();
}

void HandleActiveScanResult(void *aContext, Mac::Frame *aFrame)
{
    otInstance *aInstance = static_cast<otInstance *>(aContext);
    otActiveScanResult result;
    Mac::Address address;
    Mac::Beacon *beacon;
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

    if (payloadLength >= sizeof(*beacon) && beacon->IsValid())
    {
        result.mVersion = beacon->GetProtocolVersion();
        result.mIsJoinable = beacon->IsJoiningPermitted();
        result.mIsNative = beacon->IsNative();
        memcpy(&result.mNetworkName, beacon->GetNetworkName(), sizeof(result.mNetworkName));
        memcpy(&result.mExtendedPanId, beacon->GetExtendedPanId(), sizeof(result.mExtendedPanId));
    }

    aInstance->mActiveScanCallback(&result, aInstance->mActiveScanCallbackContext);

exit:
    return;
}

ThreadError otEnergyScan(otInstance *aInstance, uint32_t aScanChannels, uint16_t aScanDuration,
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

bool otIsEnergyScanInProgress(otInstance *aInstance)
{
    return aInstance->mThreadNetif.GetMac().IsEnergyScanInProgress();
}

ThreadError otDiscover(otInstance *aInstance, uint32_t aScanChannels, uint16_t aScanDuration, uint16_t aPanId,
                       otHandleActiveScanResult aCallback, void *aCallbackContext)
{
    aInstance->mDiscoverCallback = aCallback;
    aInstance->mDiscoverCallbackContext = aCallbackContext;
    return aInstance->mThreadNetif.GetMle().Discover(aScanChannels, aScanDuration, aPanId, &HandleMleDiscover, aInstance);
}

bool otIsDiscoverInProgress(otInstance *aInstance)
{
    return aInstance->mThreadNetif.GetMle().IsDiscoverInProgress();
}

void HandleMleDiscover(otActiveScanResult *aResult, void *aContext)
{
    otInstance *aInstance = static_cast<otInstance *>(aContext);
    aInstance->mDiscoverCallback(aResult, aInstance->mDiscoverCallbackContext);
}

void otSetReceiveIp6DatagramCallback(otInstance *aInstance, otReceiveIp6DatagramCallback aCallback,
                                     void *aCallbackContext)
{
    aInstance->mIp6.SetReceiveDatagramCallback(aCallback, aCallbackContext);
}

bool otIsReceiveIp6DatagramFilterEnabled(otInstance *aInstance)
{
    return aInstance->mIp6.IsReceiveIp6FilterEnabled();
}

void otSetReceiveIp6DatagramFilterEnabled(otInstance *aInstance, bool aEnabled)
{
    aInstance->mIp6.SetReceiveIp6FilterEnabled(aEnabled);
}

ThreadError otSendIp6Datagram(otInstance *aInstance, otMessage aMessage)
{
    ThreadError error;

    otLogFuncEntry();

    error = aInstance->mIp6.HandleDatagram(*static_cast<Message *>(aMessage), NULL,
                                           aInstance->mThreadNetif.GetInterfaceId(), NULL, true);

    otLogFuncExitErr(error);

    return error;
}

otMessage otNewUdpMessage(otInstance *aInstance, bool aLinkSecurityEnabled)
{
    Message *message = aInstance->mIp6.mUdp.NewMessage(0);

    if (message)
    {
        message->SetLinkSecurityEnabled(aLinkSecurityEnabled);
    }

    return message;
}

otMessage otNewIp6Message(otInstance *aInstance, bool aLinkSecurityEnabled)
{
    Message *message = aInstance->mIp6.mMessagePool.New(Message::kTypeIp6, 0);

    if (message)
    {
        message->SetLinkSecurityEnabled(aLinkSecurityEnabled);
    }

    return message;
}

ThreadError otFreeMessage(otMessage aMessage)
{
    return static_cast<Message *>(aMessage)->Free();
}

uint16_t otGetMessageLength(otMessage aMessage)
{
    Message *message = static_cast<Message *>(aMessage);
    return message->GetLength();
}

ThreadError otSetMessageLength(otMessage aMessage, uint16_t aLength)
{
    Message *message = static_cast<Message *>(aMessage);
    return message->SetLength(aLength);
}

uint16_t otGetMessageOffset(otMessage aMessage)
{
    Message *message = static_cast<Message *>(aMessage);
    return message->GetOffset();
}

ThreadError otSetMessageOffset(otMessage aMessage, uint16_t aOffset)
{
    Message *message = static_cast<Message *>(aMessage);
    return message->SetOffset(aOffset);
}

ThreadError otAppendMessage(otMessage aMessage, const void *aBuf, uint16_t aLength)
{
    Message *message = static_cast<Message *>(aMessage);
    return message->Append(aBuf, aLength);
}

int otReadMessage(otMessage aMessage, uint16_t aOffset, void *aBuf, uint16_t aLength)
{
    Message *message = static_cast<Message *>(aMessage);
    return message->Read(aOffset, aLength, aBuf);
}

int otWriteMessage(otMessage aMessage, uint16_t aOffset, const void *aBuf, uint16_t aLength)
{
    Message *message = static_cast<Message *>(aMessage);
    return message->Write(aOffset, aLength, aBuf);
}

ThreadError otOpenUdpSocket(otInstance *aInstance, otUdpSocket *aSocket, otUdpReceive aCallback, void *aCallbackContext)
{
    ThreadError error = kThreadError_InvalidArgs;
    Ip6::UdpSocket *socket = static_cast<Ip6::UdpSocket *>(aSocket);

    if (socket->mTransport == NULL)
    {
        socket->mTransport = &aInstance->mIp6.mUdp;
        error = socket->Open(aCallback, aCallbackContext);
    }

    return error;
}

ThreadError otCloseUdpSocket(otUdpSocket *aSocket)
{
    ThreadError error = kThreadError_InvalidState;
    Ip6::UdpSocket *socket = static_cast<Ip6::UdpSocket *>(aSocket);

    if (socket->mTransport != NULL)
    {
        error = socket->Close();

        if (error == kThreadError_None)
        {
            socket->mTransport = NULL;
        }
    }

    return error;
}

ThreadError otBindUdpSocket(otUdpSocket *aSocket, otSockAddr *aSockName)
{
    Ip6::UdpSocket *socket = static_cast<Ip6::UdpSocket *>(aSocket);
    return socket->Bind(*static_cast<const Ip6::SockAddr *>(aSockName));
}

ThreadError otSendUdp(otUdpSocket *aSocket, otMessage aMessage, const otMessageInfo *aMessageInfo)
{
    Ip6::UdpSocket *socket = static_cast<Ip6::UdpSocket *>(aSocket);
    return socket->SendTo(*static_cast<Message *>(aMessage),
                          *static_cast<const Ip6::MessageInfo *>(aMessageInfo));
}

bool otIsIcmpEchoEnabled(otInstance *aInstance)
{
    return aInstance->mIp6.mIcmp.IsEchoEnabled();
}

void otSetIcmpEchoEnabled(otInstance *aInstance, bool aEnabled)
{
    aInstance->mIp6.mIcmp.SetEchoEnabled(aEnabled);
}

uint8_t otIp6PrefixMatch(const otIp6Address *aFirst, const otIp6Address *aSecond)
{
    uint8_t rval;

    VerifyOrExit(aFirst != NULL && aSecond != NULL, rval = 0);

    rval = static_cast<const Ip6::Address *>(aFirst)->PrefixMatch(*static_cast<const Ip6::Address *>(aSecond));

exit:
    return rval;
}

ThreadError otGetActiveDataset(otInstance *aInstance, otOperationalDataset *aDataset)
{
    ThreadError error = kThreadError_None;

    VerifyOrExit(aDataset != NULL, error = kThreadError_InvalidArgs);

    aInstance->mThreadNetif.GetActiveDataset().GetLocal().Get(*aDataset);

exit:
    return error;
}

ThreadError otSetActiveDataset(otInstance *aInstance, const otOperationalDataset *aDataset)
{
    ThreadError error;

    VerifyOrExit(aDataset != NULL, error = kThreadError_InvalidArgs);

    error = aInstance->mThreadNetif.GetActiveDataset().Set(*aDataset);

exit:
    return error;
}

ThreadError otGetPendingDataset(otInstance *aInstance, otOperationalDataset *aDataset)
{
    ThreadError error = kThreadError_None;

    VerifyOrExit(aDataset != NULL, error = kThreadError_InvalidArgs);

    aInstance->mThreadNetif.GetPendingDataset().GetLocal().Get(*aDataset);

exit:
    return error;
}

ThreadError otSetPendingDataset(otInstance *aInstance, const otOperationalDataset *aDataset)
{
    ThreadError error;

    VerifyOrExit(aDataset != NULL, error = kThreadError_InvalidArgs);

    error = aInstance->mThreadNetif.GetPendingDataset().Set(*aDataset);

exit:
    return error;
}

ThreadError otSendActiveGet(otInstance *aInstance, const uint8_t *aTlvTypes, uint8_t aLength,
                            const otIp6Address *aAddress)
{
    return aInstance->mThreadNetif.GetActiveDataset().SendGetRequest(aTlvTypes, aLength, aAddress);
}

ThreadError otSendActiveSet(otInstance *aInstance, const otOperationalDataset *aDataset, const uint8_t *aTlvs,
                            uint8_t aLength)
{
    return aInstance->mThreadNetif.GetActiveDataset().SendSetRequest(*aDataset, aTlvs, aLength);
}

ThreadError otSendPendingGet(otInstance *aInstance, const uint8_t *aTlvTypes, uint8_t aLength,
                             const otIp6Address *aAddress)
{
    return aInstance->mThreadNetif.GetPendingDataset().SendGetRequest(aTlvTypes, aLength, aAddress);
}

ThreadError otSendPendingSet(otInstance *aInstance, const otOperationalDataset *aDataset, const uint8_t *aTlvs,
                             uint8_t aLength)
{
    return aInstance->mThreadNetif.GetPendingDataset().SendSetRequest(*aDataset, aTlvs, aLength);
}

#if OPENTHREAD_ENABLE_COMMISSIONER
#include <commissioning/commissioner.h>
ThreadError otCommissionerStart(otInstance *aInstance)
{
    return aInstance->mThreadNetif.GetCommissioner().Start();
}

ThreadError otCommissionerStop(otInstance *aInstance)
{
    return aInstance->mThreadNetif.GetCommissioner().Stop();
}

ThreadError otCommissionerAddJoiner(otInstance *aInstance, const otExtAddress *aExtAddress, const char *aPSKd)
{
    return aInstance->mThreadNetif.GetCommissioner().AddJoiner(static_cast<const Mac::ExtAddress *>(aExtAddress), aPSKd);
}

ThreadError otCommissionerRemoveJoiner(otInstance *aInstance, const otExtAddress *aExtAddress)
{
    return aInstance->mThreadNetif.GetCommissioner().RemoveJoiner(static_cast<const Mac::ExtAddress *>(aExtAddress));
}

ThreadError otCommissionerSetProvisioningUrl(otInstance *aInstance, const char *aProvisioningUrl)
{
    return aInstance->mThreadNetif.GetCommissioner().SetProvisioningUrl(aProvisioningUrl);
}

ThreadError otCommissionerAnnounceBegin(otInstance *aInstance, uint32_t aChannelMask, uint8_t aCount, uint16_t aPeriod,
                                        const otIp6Address *aAddress)
{
    return aInstance->mThreadNetif.GetCommissioner().mAnnounceBegin.SendRequest(aChannelMask, aCount, aPeriod,
                                                                                *static_cast<const Ip6::Address *>(aAddress));
}

ThreadError otCommissionerEnergyScan(otInstance *aInstance, uint32_t aChannelMask, uint8_t aCount, uint16_t aPeriod,
                                     uint16_t aScanDuration, const otIp6Address *aAddress,
                                     otCommissionerEnergyReportCallback aCallback, void *aContext)
{
    return aInstance->mThreadNetif.GetCommissioner().mEnergyScan.SendQuery(aChannelMask, aCount, aPeriod, aScanDuration,
                                                                           *static_cast<const Ip6::Address *>(aAddress),
                                                                           aCallback, aContext);
}

ThreadError otCommissionerPanIdQuery(otInstance *aInstance, uint16_t aPanId, uint32_t aChannelMask,
                                     const otIp6Address *aAddress,
                                     otCommissionerPanIdConflictCallback aCallback, void *aContext)
{
    return aInstance->mThreadNetif.GetCommissioner().mPanIdQuery.SendQuery(
               aPanId, aChannelMask, *static_cast<const Ip6::Address *>(aAddress), aCallback, aContext);
}

ThreadError otSendMgmtCommissionerGet(otInstance *aInstance, const uint8_t *aTlvs, uint8_t aLength)
{
    return aInstance->mThreadNetif.GetCommissioner().SendMgmtCommissionerGetRequest(aTlvs, aLength);
}

ThreadError otSendMgmtCommissionerSet(otInstance *aInstance, const otCommissioningDataset *aDataset,
                                      const uint8_t *aTlvs, uint8_t aLength)
{
    return aInstance->mThreadNetif.GetCommissioner().SendMgmtCommissionerSetRequest(*aDataset, aTlvs, aLength);
}
#endif  // OPENTHREAD_ENABLE_COMMISSIONER

#if OPENTHREAD_ENABLE_JOINER
ThreadError otJoinerStart(otInstance *aInstance, const char *aPSKd, const char *aProvisioningUrl)
{
    return aInstance->mThreadNetif.GetJoiner().Start(aPSKd, aProvisioningUrl);
}

ThreadError otJoinerStop(otInstance *aInstance)
{
    return aInstance->mThreadNetif.GetJoiner().Stop();
}
#endif  // OPENTHREAD_ENABLE_JOINER

void otCoapHeaderInit(otCoapHeader *aHeader, otCoapType aType, otCoapCode aCode)
{
    Coap::Header *header = static_cast<Coap::Header *>(aHeader);
    header->Init(aType, aCode);
}

void otCoapHeaderSetToken(otCoapHeader *aHeader, const uint8_t *aToken, uint8_t aTokenLength)
{
    static_cast<Coap::Header *>(aHeader)->SetToken(aToken, aTokenLength);
}

ThreadError otCoapHeaderAppendOption(otCoapHeader *aHeader, const otCoapOption *aOption)
{
    return static_cast<Coap::Header *>(aHeader)->AppendOption(*static_cast<const Coap::Header::Option *>(aOption));
}

void otCoapHeaderSetPayloadMarker(otCoapHeader *aHeader)
{
    static_cast<Coap::Header *>(aHeader)->SetPayloadMarker();
}

const otCoapOption *otCoapGetCurrentOption(const otCoapHeader *aHeader)
{
    return static_cast<const otCoapOption *>(static_cast<const Coap::Header *>(aHeader)->GetCurrentOption());
}

const otCoapOption *otCoapGetNextOption(otCoapHeader *aHeader)
{
    return static_cast<const otCoapOption *>(static_cast<Coap::Header *>(aHeader)->GetNextOption());
}

otMessage otNewCoapMessage(otInstance *aInstance, const otCoapHeader *aHeader)
{
    return aInstance->mThreadNetif.GetCoapClient().NewMessage(*(static_cast<const Coap::Header *>(aHeader)));
}

ThreadError otSendCoapMessage(otInstance *aInstance, otMessage aMessage, const otMessageInfo *aMessageInfo,
                              otCoapResponseHandler aHandler, void *aContext)
{
    return aInstance->mThreadNetif.GetCoapClient().SendMessage(
               *static_cast<Message *>(aMessage),
               *static_cast<const Ip6::MessageInfo *>(aMessageInfo),
               aHandler, aContext);
}

#ifdef __cplusplus
}  // extern "C"
#endif

}  // namespace Thread
