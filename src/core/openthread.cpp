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
#include <platform/radio.h>
#include <platform/random.h>
#include <platform/misc.h>
#include <thread/thread_netif.hpp>
#include <thread/thread_uris.hpp>
#include <utils/global_address.hpp>

// Temporary definition
typedef struct otInstance
{
} otInstance;

namespace Thread {

// This needs to not be static until the NCP
// the OpenThread API is capable enough for
// of of the features in the NCP.
ThreadNetif *sThreadNetif;

#ifndef OPENTHREAD_MULTIPLE_INSTANCE
static otDEFINE_ALIGNED_VAR(sInstanceRaw, sizeof(otInstance), uint64_t);
otInstance *sInstance = NULL;
#endif

static Ip6::NetifCallback sNetifCallback;

static otDEFINE_ALIGNED_VAR(sMbedTlsRaw, sizeof(Crypto::MbedTls), uint64_t);

static otDEFINE_ALIGNED_VAR(sIp6Raw, sizeof(Ip6::Ip6), uint64_t);
Ip6::Ip6 *sIp6;

#ifdef __cplusplus
extern "C" {
#endif

static otDEFINE_ALIGNED_VAR(sThreadNetifRaw, sizeof(ThreadNetif), uint64_t);

static void HandleActiveScanResult(void *aContext, Mac::Frame *aFrame);
static void HandleEnergyScanResult(void *aContext, otEnergyScanResult *aResult);
static void HandleMleDiscover(otActiveScanResult *aResult, void *aContext);

static otHandleActiveScanResult sActiveScanCallback = NULL;
static void *sActiveScanCallbackContext = NULL;

static otHandleEnergyScanResult sEnergyScanCallback = NULL;
static void *sEnergyScanCallbackContext = NULL;

static otHandleActiveScanResult sDiscoverCallback = NULL;
static void *sDiscoverCallbackContext = NULL;

void otProcessQueuedTasklets(otInstance *)
{
    sIp6->mTaskletScheduler.ProcessQueuedTasklets();
}

bool otAreTaskletsPending(otInstance *)
{
    return sIp6->mTaskletScheduler.AreTaskletsPending();
}

uint8_t otGetChannel(otInstance *)
{
    return sThreadNetif->GetMac().GetChannel();
}

ThreadError otSetChannel(otInstance *, uint8_t aChannel)
{
    return sThreadNetif->GetMac().SetChannel(aChannel);
}

uint8_t otGetMaxAllowedChildren(otInstance *)
{
    uint8_t aNumChildren;

    (void)sThreadNetif->GetMle().GetChildren(&aNumChildren);

    return aNumChildren;
}

ThreadError otSetMaxAllowedChildren(otInstance *, uint8_t aMaxChildren)
{
    return sThreadNetif->GetMle().SetMaxAllowedChildren(aMaxChildren);
}

uint32_t otGetChildTimeout(otInstance *)
{
    return sThreadNetif->GetMle().GetTimeout();
}

void otSetChildTimeout(otInstance *, uint32_t aTimeout)
{
    sThreadNetif->GetMle().SetTimeout(aTimeout);
}

const uint8_t *otGetExtendedAddress(otInstance *)
{
    return reinterpret_cast<const uint8_t *>(sThreadNetif->GetMac().GetExtAddress());
}

ThreadError otSetExtendedAddress(otInstance *, const otExtAddress *aExtAddress)
{
    ThreadError error = kThreadError_None;

    VerifyOrExit(aExtAddress != NULL, error = kThreadError_InvalidArgs);

    SuccessOrExit(error = sThreadNetif->GetMac().SetExtAddress(*static_cast<const Mac::ExtAddress *>(aExtAddress)));
    SuccessOrExit(error = sThreadNetif->GetMle().UpdateLinkLocalAddress());

exit:
    return error;
}

const uint8_t *otGetExtendedPanId(otInstance *)
{
    return sThreadNetif->GetMac().GetExtendedPanId();
}

void otSetExtendedPanId(otInstance *, const uint8_t *aExtendedPanId)
{
    uint8_t mlPrefix[8];

    sThreadNetif->GetMac().SetExtendedPanId(aExtendedPanId);

    mlPrefix[0] = 0xfd;
    memcpy(mlPrefix + 1, aExtendedPanId, 5);
    mlPrefix[6] = 0x00;
    mlPrefix[7] = 0x00;
    sThreadNetif->GetMle().SetMeshLocalPrefix(mlPrefix);
}

void otGetFactoryAssignedIeeeEui64(otInstance *aInstance, otExtAddress *aEui64)
{
    otPlatRadioGetIeeeEui64(aInstance, aEui64->m8);
}

void otGetHashMacAddress(otInstance *, otExtAddress *aHashMacAddress)
{
    sThreadNetif->GetMac().GetHashMacAddress(static_cast<Mac::ExtAddress *>(aHashMacAddress));
}

ThreadError otGetLeaderRloc(otInstance *, otIp6Address *aAddress)
{
    ThreadError error;

    VerifyOrExit(aAddress != NULL, error = kThreadError_InvalidArgs);

    error = sThreadNetif->GetMle().GetLeaderAddress(*static_cast<Ip6::Address *>(aAddress));

exit:
    return error;
}

otLinkModeConfig otGetLinkMode(otInstance *)
{
    otLinkModeConfig config;
    uint8_t mode = sThreadNetif->GetMle().GetDeviceMode();

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

ThreadError otSetLinkMode(otInstance *, otLinkModeConfig aConfig)
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

    return sThreadNetif->GetMle().SetDeviceMode(mode);
}

const uint8_t *otGetMasterKey(otInstance *, uint8_t *aKeyLength)
{
    return sThreadNetif->GetKeyManager().GetMasterKey(aKeyLength);
}

ThreadError otSetMasterKey(otInstance *, const uint8_t *aKey, uint8_t aKeyLength)
{
    return sThreadNetif->GetKeyManager().SetMasterKey(aKey, aKeyLength);
}

int8_t otGetMaxTransmitPower(otInstance *)
{
    return sThreadNetif->GetMac().GetMaxTransmitPower();
}

void otSetMaxTransmitPower(otInstance *, int8_t aPower)
{
    sThreadNetif->GetMac().SetMaxTransmitPower(aPower);
}

const otIp6Address *otGetMeshLocalEid(otInstance *)
{
    return sThreadNetif->GetMle().GetMeshLocal64();
}

const uint8_t *otGetMeshLocalPrefix(otInstance *)
{
    return sThreadNetif->GetMle().GetMeshLocalPrefix();
}

ThreadError otSetMeshLocalPrefix(otInstance *, const uint8_t *aMeshLocalPrefix)
{
    return sThreadNetif->GetMle().SetMeshLocalPrefix(aMeshLocalPrefix);
}

ThreadError otGetNetworkDataLeader(otInstance *, bool aStable, uint8_t *aData, uint8_t *aDataLength)
{
    ThreadError error = kThreadError_None;

    VerifyOrExit(aData != NULL && aDataLength != NULL, error = kThreadError_InvalidArgs);

    sThreadNetif->GetNetworkDataLeader().GetNetworkData(aStable, aData, *aDataLength);

exit:
    return error;
}

ThreadError otGetNetworkDataLocal(otInstance *, bool aStable, uint8_t *aData, uint8_t *aDataLength)
{
    ThreadError error = kThreadError_None;

    VerifyOrExit(aData != NULL && aDataLength != NULL, error = kThreadError_InvalidArgs);

    sThreadNetif->GetNetworkDataLocal().GetNetworkData(aStable, aData, *aDataLength);

exit:
    return error;
}

const char *otGetNetworkName(otInstance *)
{
    return sThreadNetif->GetMac().GetNetworkName();
}

ThreadError otSetNetworkName(otInstance *, const char *aNetworkName)
{
    return sThreadNetif->GetMac().SetNetworkName(aNetworkName);
}

otPanId otGetPanId(otInstance *)
{
    return sThreadNetif->GetMac().GetPanId();
}

ThreadError otSetPanId(otInstance *, otPanId aPanId)
{
    ThreadError error = kThreadError_None;

    // do not allow setting PAN ID to broadcast if Thread is running
    VerifyOrExit(aPanId != Mac::kPanIdBroadcast ||
                 sThreadNetif->GetMle().GetDeviceState() != Mle::kDeviceStateDisabled,
                 error = kThreadError_InvalidState);

    error = sThreadNetif->GetMac().SetPanId(aPanId);

exit:
    return error;
}

bool otIsRouterRoleEnabled(otInstance *)
{
    return sThreadNetif->GetMle().IsRouterRoleEnabled();
}

void otSetRouterRoleEnabled(otInstance *, bool aEnabled)
{
    sThreadNetif->GetMle().SetRouterRoleEnabled(aEnabled);
}

otShortAddress otGetShortAddress(otInstance *)
{
    return sThreadNetif->GetMac().GetShortAddress();
}

uint8_t otGetLocalLeaderWeight(otInstance *)
{
    return sThreadNetif->GetMle().GetLeaderWeight();
}

void otSetLocalLeaderWeight(otInstance *, uint8_t aWeight)
{
    sThreadNetif->GetMle().SetLeaderWeight(aWeight);
}

uint32_t otGetLocalLeaderPartitionId(otInstance *)
{
    return sThreadNetif->GetMle().GetLeaderPartitionId();
}

void otSetLocalLeaderPartitionId(otInstance *, uint32_t aPartitionId)
{
    return sThreadNetif->GetMle().SetLeaderPartitionId(aPartitionId);
}

uint16_t otGetJoinerUdpPort(otInstance *)
{
    return sThreadNetif->GetJoinerRouter().GetJoinerUdpPort();
}

ThreadError otSetJoinerUdpPort(otInstance *, uint16_t aJoinerUdpPort)
{
    return sThreadNetif->GetJoinerRouter().SetJoinerUdpPort(aJoinerUdpPort);
}

ThreadError otAddBorderRouter(otInstance *, const otBorderRouterConfig *aConfig)
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

    return sThreadNetif->GetNetworkDataLocal().AddOnMeshPrefix(aConfig->mPrefix.mPrefix.mFields.m8,
                                                               aConfig->mPrefix.mLength,
                                                               aConfig->mPreference, flags, aConfig->mStable);
}

ThreadError otRemoveBorderRouter(otInstance *, const otIp6Prefix *aPrefix)
{
    return sThreadNetif->GetNetworkDataLocal().RemoveOnMeshPrefix(aPrefix->mPrefix.mFields.m8, aPrefix->mLength);
}

ThreadError otGetNextOnMeshPrefix(otInstance *, bool aLocal, otNetworkDataIterator *aIterator,
                                  otBorderRouterConfig *aConfig)
{
    ThreadError error = kThreadError_None;

    VerifyOrExit(aIterator && aConfig, error = kThreadError_InvalidArgs);

    if (aLocal)
    {
        error = sThreadNetif->GetNetworkDataLocal().GetNextOnMeshPrefix(aIterator, aConfig);
    }
    else
    {
        error = sThreadNetif->GetNetworkDataLeader().GetNextOnMeshPrefix(aIterator, aConfig);
    }

exit:
    return error;
}

ThreadError otAddExternalRoute(otInstance *, const otExternalRouteConfig *aConfig)
{
    return sThreadNetif->GetNetworkDataLocal().AddHasRoutePrefix(aConfig->mPrefix.mPrefix.mFields.m8,
                                                                 aConfig->mPrefix.mLength,
                                                                 aConfig->mPreference, aConfig->mStable);
}

ThreadError otRemoveExternalRoute(otInstance *, const otIp6Prefix *aPrefix)
{
    return sThreadNetif->GetNetworkDataLocal().RemoveHasRoutePrefix(aPrefix->mPrefix.mFields.m8, aPrefix->mLength);
}

ThreadError otSendServerData(otInstance *)
{
    return sThreadNetif->GetNetworkDataLocal().SendServerDataNotification();
}

ThreadError otAddUnsecurePort(otInstance *, uint16_t aPort)
{
    return sThreadNetif->GetIp6Filter().AddUnsecurePort(aPort);
}

ThreadError otRemoveUnsecurePort(otInstance *, uint16_t aPort)
{
    return sThreadNetif->GetIp6Filter().RemoveUnsecurePort(aPort);
}

const uint16_t *otGetUnsecurePorts(otInstance *, uint8_t *aNumEntries)
{
    return sThreadNetif->GetIp6Filter().GetUnsecurePorts(*aNumEntries);
}

uint32_t otGetContextIdReuseDelay(otInstance *)
{
    return sThreadNetif->GetNetworkDataLeader().GetContextIdReuseDelay();
}

void otSetContextIdReuseDelay(otInstance *, uint32_t aDelay)
{
    sThreadNetif->GetNetworkDataLeader().SetContextIdReuseDelay(aDelay);
}

uint32_t otGetKeySequenceCounter(otInstance *)
{
    return sThreadNetif->GetKeyManager().GetCurrentKeySequence();
}

void otSetKeySequenceCounter(otInstance *, uint32_t aKeySequenceCounter)
{
    sThreadNetif->GetKeyManager().SetCurrentKeySequence(aKeySequenceCounter);
}

uint8_t otGetNetworkIdTimeout(otInstance *)
{
    return sThreadNetif->GetMle().GetNetworkIdTimeout();
}

void otSetNetworkIdTimeout(otInstance *, uint8_t aTimeout)
{
    sThreadNetif->GetMle().SetNetworkIdTimeout(aTimeout);
}

uint8_t otGetRouterUpgradeThreshold(otInstance *)
{
    return sThreadNetif->GetMle().GetRouterUpgradeThreshold();
}

void otSetRouterUpgradeThreshold(otInstance *, uint8_t aThreshold)
{
    sThreadNetif->GetMle().SetRouterUpgradeThreshold(aThreshold);
}

ThreadError otReleaseRouterId(otInstance *, uint8_t aRouterId)
{
    return sThreadNetif->GetMle().ReleaseRouterId(aRouterId);
}

ThreadError otAddMacWhitelist(otInstance *, const uint8_t *aExtAddr)
{
    ThreadError error = kThreadError_None;

    if (sThreadNetif->GetMac().GetWhitelist().Add(*reinterpret_cast<const Mac::ExtAddress *>(aExtAddr)) == NULL)
    {
        error = kThreadError_NoBufs;
    }

    return error;
}

ThreadError otAddMacWhitelistRssi(otInstance *, const uint8_t *aExtAddr, int8_t aRssi)
{
    ThreadError error = kThreadError_None;
    otMacWhitelistEntry *entry;

    entry = sThreadNetif->GetMac().GetWhitelist().Add(*reinterpret_cast<const Mac::ExtAddress *>(aExtAddr));
    VerifyOrExit(entry != NULL, error = kThreadError_NoBufs);
    sThreadNetif->GetMac().GetWhitelist().SetFixedRssi(*entry, aRssi);

exit:
    return error;
}

void otRemoveMacWhitelist(otInstance *, const uint8_t *aExtAddr)
{
    sThreadNetif->GetMac().GetWhitelist().Remove(*reinterpret_cast<const Mac::ExtAddress *>(aExtAddr));
}

void otClearMacWhitelist(otInstance *)
{
    sThreadNetif->GetMac().GetWhitelist().Clear();
}

ThreadError otGetMacWhitelistEntry(otInstance *, uint8_t aIndex, otMacWhitelistEntry *aEntry)
{
    ThreadError error = kThreadError_None;

    VerifyOrExit(aEntry != NULL, error = kThreadError_InvalidArgs);
    error = sThreadNetif->GetMac().GetWhitelist().GetEntry(aIndex, *aEntry);

exit:
    return error;
}

void otDisableMacWhitelist(otInstance *)
{
    sThreadNetif->GetMac().GetWhitelist().Disable();
}

void otEnableMacWhitelist(otInstance *)
{
    sThreadNetif->GetMac().GetWhitelist().Enable();
}

bool otIsMacWhitelistEnabled(otInstance *)
{
    return sThreadNetif->GetMac().GetWhitelist().IsEnabled();
}

ThreadError otBecomeDetached(otInstance *)
{
    return sThreadNetif->GetMle().BecomeDetached();
}

ThreadError otBecomeChild(otInstance *, otMleAttachFilter aFilter)
{
    return sThreadNetif->GetMle().BecomeChild(aFilter);
}

ThreadError otBecomeRouter(otInstance *)
{
    return sThreadNetif->GetMle().BecomeRouter(ThreadStatusTlv::kTooFewRouters);
}

ThreadError otBecomeLeader(otInstance *)
{
    return sThreadNetif->GetMle().BecomeLeader();
}

ThreadError otAddMacBlacklist(otInstance *, const uint8_t *aExtAddr)
{
    ThreadError error = kThreadError_None;

    if (sThreadNetif->GetMac().GetBlacklist().Add(*reinterpret_cast<const Mac::ExtAddress *>(aExtAddr)) == NULL)
    {
        error = kThreadError_NoBufs;
    }

    return error;
}

void otRemoveMacBlacklist(otInstance *, const uint8_t *aExtAddr)
{
    sThreadNetif->GetMac().GetBlacklist().Remove(*reinterpret_cast<const Mac::ExtAddress *>(aExtAddr));
}

void otClearMacBlacklist(otInstance *)
{
    sThreadNetif->GetMac().GetBlacklist().Clear();
}

ThreadError otGetMacBlacklistEntry(otInstance *, uint8_t aIndex, otMacBlacklistEntry *aEntry)
{
    ThreadError error = kThreadError_None;

    VerifyOrExit(aEntry != NULL, error = kThreadError_InvalidArgs);
    error = sThreadNetif->GetMac().GetBlacklist().GetEntry(aIndex, *aEntry);

exit:
    return error;
}

void otDisableMacBlacklist(otInstance *)
{
    sThreadNetif->GetMac().GetBlacklist().Disable();
}

void otEnableMacBlacklist(otInstance *)
{
    sThreadNetif->GetMac().GetBlacklist().Enable();
}

bool otIsMacBlacklistEnabled(otInstance *)
{
    return sThreadNetif->GetMac().GetBlacklist().IsEnabled();
}

ThreadError otGetAssignLinkQuality(otInstance *, const uint8_t *aExtAddr, uint8_t *aLinkQuality)
{
    Mac::ExtAddress extAddress;

    memset(&extAddress, 0, sizeof(extAddress));
    memcpy(extAddress.m8, aExtAddr, OT_EXT_ADDRESS_SIZE);

    return sThreadNetif->GetMle().GetAssignLinkQuality(extAddress, *aLinkQuality);
}

void otSetAssignLinkQuality(otInstance *, const uint8_t *aExtAddr, uint8_t aLinkQuality)
{
    Mac::ExtAddress extAddress;

    memset(&extAddress, 0, sizeof(extAddress));
    memcpy(extAddress.m8, aExtAddr, OT_EXT_ADDRESS_SIZE);

    sThreadNetif->GetMle().SetAssignLinkQuality(extAddress, aLinkQuality);
}

void otPlatformReset(otInstance *aInstance)
{
    otPlatReset(aInstance);
}

uint8_t otGetRouterDowngradeThreshold(otInstance *)
{
    return sThreadNetif->GetMle().GetRouterDowngradeThreshold();
}

void otSetRouterDowngradeThreshold(otInstance *, uint8_t aThreshold)
{
    sThreadNetif->GetMle().SetRouterDowngradeThreshold(aThreshold);
}

uint8_t otGetRouterSelectionJitter(otInstance *)
{
    return sThreadNetif->GetMle().GetRouterSelectionJitter();
}

void otSetRouterSelectionJitter(otInstance *, uint8_t aRouterJitter)
{
    sThreadNetif->GetMle().SetRouterSelectionJitter(aRouterJitter);
}

ThreadError otGetChildInfoById(otInstance *, uint16_t aChildId, otChildInfo *aChildInfo)
{
    ThreadError error = kThreadError_None;

    VerifyOrExit(aChildInfo != NULL, error = kThreadError_InvalidArgs);

    error = sThreadNetif->GetMle().GetChildInfoById(aChildId, *aChildInfo);

exit:
    return error;
}

ThreadError otGetChildInfoByIndex(otInstance *, uint8_t aChildIndex, otChildInfo *aChildInfo)
{
    ThreadError error = kThreadError_None;

    VerifyOrExit(aChildInfo != NULL, error = kThreadError_InvalidArgs);

    error = sThreadNetif->GetMle().GetChildInfoByIndex(aChildIndex, *aChildInfo);

exit:
    return error;
}

ThreadError otGetNextNeighborInfo(otInstance *, otNeighborInfoIterator *aIterator, otNeighborInfo *aInfo)
{
    ThreadError error = kThreadError_None;

    VerifyOrExit((aInfo != NULL) && (aIterator != NULL), error = kThreadError_InvalidArgs);

    error = sThreadNetif->GetMle().GetNextNeighborInfo(*aIterator, *aInfo);

exit:
    return error;
}

otDeviceRole otGetDeviceRole(otInstance *)
{
    otDeviceRole rval = kDeviceRoleDisabled;

    switch (sThreadNetif->GetMle().GetDeviceState())
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

ThreadError otGetEidCacheEntry(otInstance *, uint8_t aIndex, otEidCacheEntry *aEntry)
{
    ThreadError error;

    VerifyOrExit(aEntry != NULL, error = kThreadError_InvalidArgs);
    error = sThreadNetif->GetAddressResolver().GetEntry(aIndex, *aEntry);

exit:
    return error;
}

ThreadError otGetLeaderData(otInstance *, otLeaderData *aLeaderData)
{
    ThreadError error;

    VerifyOrExit(aLeaderData != NULL, error = kThreadError_InvalidArgs);

    error = sThreadNetif->GetMle().GetLeaderData(*aLeaderData);

exit:
    return error;
}

uint8_t otGetLeaderRouterId(otInstance *)
{
    return sThreadNetif->GetMle().GetLeaderDataTlv().GetLeaderRouterId();
}

uint8_t otGetLeaderWeight(otInstance *)
{
    return sThreadNetif->GetMle().GetLeaderDataTlv().GetWeighting();
}

uint8_t otGetNetworkDataVersion(otInstance *)
{
    return sThreadNetif->GetMle().GetLeaderDataTlv().GetDataVersion();
}

uint32_t otGetPartitionId(otInstance *)
{
    return sThreadNetif->GetMle().GetLeaderDataTlv().GetPartitionId();
}

uint16_t otGetRloc16(otInstance *)
{
    return sThreadNetif->GetMle().GetRloc16();
}

uint8_t otGetRouterIdSequence(otInstance *)
{
    return sThreadNetif->GetMle().GetRouterIdSequence();
}

ThreadError otGetRouterInfo(otInstance *, uint16_t aRouterId, otRouterInfo *aRouterInfo)
{
    ThreadError error = kThreadError_None;

    VerifyOrExit(aRouterInfo != NULL, error = kThreadError_InvalidArgs);

    error = sThreadNetif->GetMle().GetRouterInfo(aRouterId, *aRouterInfo);

exit:
    return error;
}

ThreadError otGetParentInfo(otInstance *, otRouterInfo *aParentInfo)
{
    ThreadError error = kThreadError_None;
    Router *parent;

    VerifyOrExit(aParentInfo != NULL, error = kThreadError_InvalidArgs);

    parent = sThreadNetif->GetMle().GetParent();
    memcpy(aParentInfo->mExtAddress.m8, parent->mMacAddr.m8, OT_EXT_ADDRESS_SIZE);
    aParentInfo->mRloc16 = parent->mValid.mRloc16;

exit:
    return error;
}

uint8_t otGetStableNetworkDataVersion(otInstance *)
{
    return sThreadNetif->GetMle().GetLeaderDataTlv().GetStableDataVersion();
}

void otSetLinkPcapCallback(otInstance *, otLinkPcapCallback aPcapCallback, void *aCallbackContext)
{
    sThreadNetif->GetMac().SetPcapCallback(aPcapCallback, aCallbackContext);
}

bool otIsLinkPromiscuous(otInstance *)
{
    return sThreadNetif->GetMac().IsPromiscuous();
}

ThreadError otSetLinkPromiscuous(otInstance *, bool aPromiscuous)
{
    ThreadError error = kThreadError_None;

    // cannot enable IEEE 802.15.4 promiscuous mode if the Thread interface is enabled
    VerifyOrExit(sThreadNetif->IsUp() == false, error = kThreadError_Busy);

    sThreadNetif->GetMac().SetPromiscuous(aPromiscuous);

exit:
    return error;
}

const otMacCounters *otGetMacCounters(otInstance *)
{
    return &sThreadNetif->GetMac().GetCounters();
}

bool otIsIp6AddressEqual(const otIp6Address *a, const otIp6Address *b)
{
    return *static_cast<const Ip6::Address *>(a) == *static_cast<const Ip6::Address *>(b);
}

ThreadError otIp6AddressFromString(const char *str, otIp6Address *address)
{
    return static_cast<Ip6::Address *>(address)->FromString(str);
}

const otNetifAddress *otGetUnicastAddresses(otInstance *)
{
    return sThreadNetif->GetUnicastAddresses();
}

ThreadError otAddUnicastAddress(otInstance *, const otNetifAddress *address)
{
    return sThreadNetif->AddExternalUnicastAddress(*static_cast<const Ip6::NetifUnicastAddress *>(address));
}

ThreadError otRemoveUnicastAddress(otInstance *, const otIp6Address *address)
{
    return sThreadNetif->RemoveExternalUnicastAddress(*static_cast<const Ip6::Address *>(address));
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

ThreadError otCreateMacIid(otInstance *, otNetifAddress *aAddress, void *)
{
    memcpy(&aAddress->mAddress.mFields.m8[OT_IP6_ADDRESS_SIZE - OT_IP6_IID_SIZE],
           sThreadNetif->GetMac().GetExtAddress(), OT_IP6_IID_SIZE);
    aAddress->mAddress.mFields.m8[OT_IP6_ADDRESS_SIZE - OT_IP6_IID_SIZE] ^= 0x02;

    return kThreadError_None;
}

ThreadError otCreateSemanticallyOpaqueIid(otInstance *aInstance, otNetifAddress *aAddress, void *aContext)
{
    return static_cast<Utils::SemanticallyOpaqueIidGenerator *>(aContext)->CreateIid(aInstance, aAddress);
}

void otSetStateChangedCallback(otInstance *, otStateChangedCallback aCallback, void *aCallbackContext)
{
    sNetifCallback.Set(aCallback, aCallbackContext);
    sThreadNetif->RegisterCallback(sNetifCallback);
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

uint32_t otGetPollPeriod(otInstance *)
{
    return sThreadNetif->GetMeshForwarder().GetAssignPollPeriod();
}

void otSetPollPeriod(otInstance *, uint32_t aPollPeriod)
{
    sThreadNetif->GetMeshForwarder().SetAssignPollPeriod(aPollPeriod);
}

ThreadError otSetPreferredRouterId(otInstance *, uint8_t aRouterId)
{
    return sThreadNetif->GetMle().SetPreferredRouterId(aRouterId);
}

#ifdef OPENTHREAD_MULTIPLE_INSTANCE

otInstance *otInstanceInit(void *aInstanceBuffer, uint64_t *aInstanceBufferSize)
{
    otInstance *aInstance = NULL;

    otLogInfoApi("otInstanceInit\n");

    VerifyOrExit(aInstanceBufferSize != NULL, ;);

    // Make sure the input buffer is big enough
    VerifyOrExit(sizeof(otInstance) <= *aInstanceBufferSize, *aInstanceBufferSize = sizeof(otInstance));

    VerifyOrExit(aInstanceBuffer != NULL, ;);

    // Construct the context
    aInstance = new(aInstanceBuffer)otInstance();

    new(&sMbedTlsRaw) Crypto::MbedTls;
    sIp6 = new(&sIp6Raw) Ip6::Ip6;
    sThreadNetif = new(&sThreadNetifRaw) ThreadNetif(*sIp6);

exit:

    return aInstance;
}

#else

otInstance *otInstanceInit()
{
    otLogInfoApi("otInstanceInit\n");

    VerifyOrExit(sInstance == NULL, ;);

    // Construct the context
    sInstance = new(&sInstanceRaw)otInstance();

    new(&sMbedTlsRaw) Crypto::MbedTls;
    sIp6 = new(&sIp6Raw) Ip6::Ip6;
    sThreadNetif = new(&sThreadNetifRaw) ThreadNetif(*sIp6);

exit:

    return sInstance;
}

#endif

ThreadError otSendDiagnosticGet(otInstance *aInstance, otIp6Address *aDestination, uint8_t aTlvTypes[], uint8_t aCount)
{
    (void)aInstance;
    return sThreadNetif->GetNetworkDiagnostic().SendDiagnosticGet(*static_cast<Ip6::Address *>(aDestination), aTlvTypes,
                                                                  aCount);
}

ThreadError otSendDiagnosticReset(otInstance *aInstance, otIp6Address *aDestination, uint8_t aTlvTypes[],
                                  uint8_t aCount)
{
    (void)aInstance;
    return sThreadNetif->GetNetworkDiagnostic().SendDiagnosticReset(*static_cast<Ip6::Address *>(aDestination), aTlvTypes,
                                                                    aCount);
}

void otInstanceFinalize(otInstance *aInstance)
{
    // Ensure we are disabled
    (void)otThreadStop(aInstance);
    (void)otInterfaceDown(aInstance);

    // Nothing to actually free, since the caller supplied the buffer
    sThreadNetif = NULL;
}

ThreadError otInterfaceUp(otInstance *)
{
    ThreadError error = kThreadError_None;

    error = sThreadNetif->Up();

    return error;
}

ThreadError otInterfaceDown(otInstance *)
{
    ThreadError error = kThreadError_None;

    error = sThreadNetif->Down();

    return error;
}

bool otIsInterfaceUp(otInstance *)
{
    return sThreadNetif->IsUp();
}

ThreadError otThreadStart(otInstance *)
{
    ThreadError error = kThreadError_None;

    VerifyOrExit(sThreadNetif->GetMac().GetPanId() != Mac::kPanIdBroadcast, error = kThreadError_InvalidState);

    error = sThreadNetif->GetMle().Start();

exit:
    return error;
}

ThreadError otThreadStop(otInstance *)
{
    ThreadError error = kThreadError_None;

    error = sThreadNetif->GetMle().Stop();

    return error;
}

bool otIsSingleton(otInstance *)
{
    return sThreadNetif->GetMle().IsSingleton();
}

ThreadError otActiveScan(otInstance *aInstance, uint32_t aScanChannels, uint16_t aScanDuration,
                         otHandleActiveScanResult aCallback, void *aCallbackContext)
{
    sActiveScanCallback = aCallback;
    sActiveScanCallbackContext = aCallbackContext;
    return sThreadNetif->GetMac().ActiveScan(aScanChannels, aScanDuration, &HandleActiveScanResult, aInstance);
}

bool otIsActiveScanInProgress(otInstance *)
{
    return sThreadNetif->GetMac().IsActiveScanInProgress();
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
        sActiveScanCallback(NULL, sActiveScanCallbackContext);
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

    sActiveScanCallback(&result, sActiveScanCallbackContext);

exit:
    (void)aInstance;
    return;
}

ThreadError otEnergyScan(otInstance *aInstance, uint32_t aScanChannels, uint16_t aScanDuration,
                         otHandleEnergyScanResult aCallback, void *aCallbackContext)
{
    sEnergyScanCallback = aCallback;
    sEnergyScanCallbackContext = aCallbackContext;
    return sThreadNetif->GetMac().EnergyScan(aScanChannels, aScanDuration, &HandleEnergyScanResult, aInstance);
}

void HandleEnergyScanResult(void *aContext, otEnergyScanResult *aResult)
{
    otInstance *aInstance = static_cast<otInstance *>(aContext);

    sEnergyScanCallback(aResult, sEnergyScanCallbackContext);

    (void)aInstance;
}


bool otIsEnergyScanInProgress(otInstance *aInstance)
{
    (void)aInstance;
    return sThreadNetif->GetMac().IsEnergyScanInProgress();
}

ThreadError otDiscover(otInstance *aInstance, uint32_t aScanChannels, uint16_t aScanDuration, uint16_t aPanId,
                       otHandleActiveScanResult aCallback, void *aCallbackContext)
{
    sDiscoverCallback = aCallback;
    sDiscoverCallbackContext = aCallbackContext;
    return sThreadNetif->GetMle().Discover(aScanChannels, aScanDuration, aPanId, &HandleMleDiscover, aInstance);
}

bool otIsDiscoverInProgress(otInstance *)
{
    return sThreadNetif->GetMle().IsDiscoverInProgress();
}

void HandleMleDiscover(otActiveScanResult *aResult, void *aContext)
{
    otInstance *aInstance = static_cast<otInstance *>(aContext);
    (void)aInstance;
    sDiscoverCallback(aResult, sDiscoverCallbackContext);
}

void otSetReceiveIp6DatagramCallback(otInstance *, otReceiveIp6DatagramCallback aCallback,
                                     void *aCallbackContext)
{
    sIp6->SetReceiveDatagramCallback(aCallback, aCallbackContext);
}

bool otIsReceiveIp6DatagramFilterEnabled(otInstance *)
{
    return sIp6->IsReceiveIp6FilterEnabled();
}

void otSetReceiveIp6DatagramFilterEnabled(otInstance *, bool aEnabled)
{
    sIp6->SetReceiveIp6FilterEnabled(aEnabled);
}

ThreadError otSendIp6Datagram(otInstance *, otMessage aMessage)
{
    return sIp6->HandleDatagram(*static_cast<Message *>(aMessage), NULL, sThreadNetif->GetInterfaceId(), NULL, true);
}

otMessage otNewUdpMessage(otInstance *)
{
    return sIp6->mUdp.NewMessage(0);
}

otMessage otNewIp6Message(otInstance *, bool aLinkSecurityEnabled)
{
    Message *message = sIp6->mMessagePool.New(Message::kTypeIp6, 0);

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

ThreadError otOpenUdpSocket(otInstance *, otUdpSocket *aSocket, otUdpReceive aCallback, void *aCallbackContext)
{
    ThreadError error = kThreadError_Busy;
    Ip6::UdpSocket *socket = static_cast<Ip6::UdpSocket *>(aSocket);

    if (socket->mTransport == NULL)
    {
        socket->mTransport = &sIp6->mUdp;
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

bool otIsIcmpEchoEnabled(otInstance *)
{
    return sIp6->mIcmp.IsEchoEnabled();
}

void otSetIcmpEchoEnabled(otInstance *, bool aEnabled)
{
    sIp6->mIcmp.SetEchoEnabled(aEnabled);
}

uint8_t otIp6PrefixMatch(const otIp6Address *aFirst, const otIp6Address *aSecond)
{
    uint8_t rval;

    VerifyOrExit(aFirst != NULL && aSecond != NULL, rval = 0);

    rval = static_cast<const Ip6::Address *>(aFirst)->PrefixMatch(*static_cast<const Ip6::Address *>(aSecond));

exit:
    return rval;
}

ThreadError otGetActiveDataset(otInstance *, otOperationalDataset *aDataset)
{
    ThreadError error = kThreadError_None;

    VerifyOrExit(aDataset != NULL, error = kThreadError_InvalidArgs);

    sThreadNetif->GetActiveDataset().GetLocal().Get(*aDataset);

exit:
    return error;
}

ThreadError otSetActiveDataset(otInstance *, otOperationalDataset *aDataset)
{
    ThreadError error;

    VerifyOrExit(aDataset != NULL, error = kThreadError_InvalidArgs);

    error = sThreadNetif->GetActiveDataset().Set(*aDataset);

exit:
    return error;
}

ThreadError otGetPendingDataset(otInstance *, otOperationalDataset *aDataset)
{
    ThreadError error = kThreadError_None;

    VerifyOrExit(aDataset != NULL, error = kThreadError_InvalidArgs);

    sThreadNetif->GetPendingDataset().GetLocal().Get(*aDataset);

exit:
    return error;
}

ThreadError otSetPendingDataset(otInstance *, otOperationalDataset *aDataset)
{
    ThreadError error;

    VerifyOrExit(aDataset != NULL, error = kThreadError_InvalidArgs);

    error = sThreadNetif->GetPendingDataset().Set(*aDataset);

exit:
    return error;
}

ThreadError otSendActiveGet(otInstance *, const uint8_t *aTlvTypes, uint8_t aLength)
{
    return sThreadNetif->GetActiveDataset().SendGetRequest(aTlvTypes, aLength);
}

ThreadError otSendActiveSet(otInstance *, const otOperationalDataset *aDataset, const uint8_t *aTlvs, uint8_t aLength)
{
    return sThreadNetif->GetActiveDataset().SendSetRequest(*aDataset, aTlvs, aLength);
}

ThreadError otSendPendingGet(otInstance *, const uint8_t *aTlvTypes, uint8_t aLength)
{
    return sThreadNetif->GetPendingDataset().SendGetRequest(aTlvTypes, aLength);
}

ThreadError otSendPendingSet(otInstance *, const otOperationalDataset *aDataset, const uint8_t *aTlvs, uint8_t aLength)
{
    return sThreadNetif->GetPendingDataset().SendSetRequest(*aDataset, aTlvs, aLength);
}

#if OPENTHREAD_ENABLE_COMMISSIONER
#include <commissioning/commissioner.h>
ThreadError otCommissionerStart(otInstance *)
{
    return sThreadNetif->GetCommissioner().Start();
}

ThreadError otCommissionerStop(otInstance *)
{
    return sThreadNetif->GetCommissioner().Stop();
}

ThreadError otCommissionerAddJoiner(otInstance *, const otExtAddress *aExtAddress, const char *aPSKd)
{
    return sThreadNetif->GetCommissioner().AddJoiner(static_cast<const Mac::ExtAddress *>(aExtAddress), aPSKd);
}

ThreadError otCommissionerRemoveJoiner(otInstance *, const otExtAddress *aExtAddress)
{
    return sThreadNetif->GetCommissioner().RemoveJoiner(static_cast<const Mac::ExtAddress *>(aExtAddress));
}

ThreadError otCommissionerSetProvisioningUrl(otInstance *, const char *aProvisioningUrl)
{
    return sThreadNetif->GetCommissioner().SetProvisioningUrl(aProvisioningUrl);
}

ThreadError otCommissionerEnergyScan(otInstance *, uint32_t aChannelMask, uint8_t aCount, uint16_t aPeriod,
                                     uint16_t aScanDuration, const otIp6Address *aAddress,
                                     otCommissionerEnergyReportCallback aCallback, void *aContext)
{
    return sThreadNetif->GetCommissioner().mEnergyScan.SendQuery(aChannelMask, aCount, aPeriod, aScanDuration,
                                                                 *static_cast<const Ip6::Address *>(aAddress),
                                                                 aCallback, aContext);
}

ThreadError otCommissionerPanIdQuery(otInstance *, uint16_t aPanId, uint32_t aChannelMask,
                                     const otIp6Address *aAddress,
                                     otCommissionerPanIdConflictCallback aCallback, void *aContext)
{
    return sThreadNetif->GetCommissioner().mPanIdQuery.SendQuery(aPanId, aChannelMask,
                                                                 *static_cast<const Ip6::Address *>(aAddress),
                                                                 aCallback, aContext);
}

ThreadError otSendMgmtCommissionerGet(otInstance *, const uint8_t *aTlvs, uint8_t aLength)
{
    return sThreadNetif->GetCommissioner().SendMgmtCommissionerGetRequest(aTlvs, aLength);
}

ThreadError otSendMgmtCommissionerSet(otInstance *, const otCommissioningDataset *aDataset,
                                      const uint8_t *aTlvs, uint8_t aLength)
{
    return sThreadNetif->GetCommissioner().SendMgmtCommissionerSetRequest(*aDataset, aTlvs, aLength);
}
#endif  // OPENTHREAD_ENABLE_COMMISSIONER

#if OPENTHREAD_ENABLE_JOINER
ThreadError otJoinerStart(otInstance *, const char *aPSKd, const char *aProvisioningUrl)
{
    return sThreadNetif->GetJoiner().Start(aPSKd, aProvisioningUrl);
}

ThreadError otJoinerStop(otInstance *)
{
    return sThreadNetif->GetJoiner().Stop();
}
#endif  // OPENTHREAD_ENABLE_JOINER

#ifdef __cplusplus
}  // extern "C"
#endif

}  // namespace Thread
