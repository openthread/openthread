/*
 *  Copyright (c) 2016, Nest Labs, Inc.
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

#include <openthread.h>
#include <openthread-config.h>
#include <common/code_utils.hpp>
#include <common/debug.hpp>
#include <common/logging.hpp>
#include <common/message.hpp>
#include <common/new.hpp>
#include <common/tasklet.hpp>
#include <common/timer.hpp>
#include <net/icmp6.hpp>
#include <net/ip6.hpp>
#include <platform/random.h>
#include <platform/misc.h>
#include <thread/thread_netif.hpp>

namespace Thread {

// This needs to not be static until the NCP
// the OpenThread API is capable enough for
// of of the features in the NCP.
ThreadNetif *sThreadNetif;

static Ip6::NetifCallback sNetifCallback;
static bool mEnabled = false;

#ifdef __cplusplus
extern "C" {
#endif

static otDEFINE_ALIGNED_VAR(sThreadNetifRaw, sizeof(ThreadNetif), uint64_t);

static void HandleActiveScanResult(void *aContext, Mac::Frame *aFrame);
static void HandleMleDiscover(otActiveScanResult *aResult, void *aContext);

void otProcessNextTasklet(void)
{
    TaskletScheduler::RunNextTasklet();
}

bool otAreTaskletsPending(void)
{
    return TaskletScheduler::AreTaskletsPending();
}

uint8_t otGetChannel(void)
{
    return sThreadNetif->GetMac().GetChannel();
}

ThreadError otSetChannel(uint8_t aChannel)
{
    return sThreadNetif->GetMac().SetChannel(aChannel);
}

uint32_t otGetChildTimeout(void)
{
    return sThreadNetif->GetMle().GetTimeout();
}

void otSetChildTimeout(uint32_t aTimeout)
{
    sThreadNetif->GetMle().SetTimeout(aTimeout);
}

const uint8_t *otGetExtendedAddress(void)
{
    return reinterpret_cast<const uint8_t *>(sThreadNetif->GetMac().GetExtAddress());
}

ThreadError otSetExtendedAddress(const otExtAddress *aExtAddress)
{
    ThreadError error = kThreadError_None;

    VerifyOrExit(aExtAddress != NULL, error = kThreadError_InvalidArgs);

    SuccessOrExit(error = sThreadNetif->GetMac().SetExtAddress(*static_cast<const Mac::ExtAddress *>(aExtAddress)));
    SuccessOrExit(error = sThreadNetif->GetMle().UpdateLinkLocalAddress());

exit:
    return error;
}

const uint8_t *otGetExtendedPanId(void)
{
    return sThreadNetif->GetMac().GetExtendedPanId();
}

void otSetExtendedPanId(const uint8_t *aExtendedPanId)
{
    uint8_t mlPrefix[8];

    sThreadNetif->GetMac().SetExtendedPanId(aExtendedPanId);

    mlPrefix[0] = 0xfd;
    memcpy(mlPrefix + 1, aExtendedPanId, 5);
    mlPrefix[6] = 0x00;
    mlPrefix[7] = 0x00;
    sThreadNetif->GetMle().SetMeshLocalPrefix(mlPrefix);
}

ThreadError otGetLeaderRloc(otIp6Address *aAddress)
{
    ThreadError error;

    VerifyOrExit(aAddress != NULL, error = kThreadError_InvalidArgs);

    error = sThreadNetif->GetMle().GetLeaderAddress(*static_cast<Ip6::Address *>(aAddress));

exit:
    return error;
}

otLinkModeConfig otGetLinkMode(void)
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

ThreadError otSetLinkMode(otLinkModeConfig aConfig)
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

const uint8_t *otGetMasterKey(uint8_t *aKeyLength)
{
    return sThreadNetif->GetKeyManager().GetMasterKey(aKeyLength);
}

ThreadError otSetMasterKey(const uint8_t *aKey, uint8_t aKeyLength)
{
    return sThreadNetif->GetKeyManager().SetMasterKey(aKey, aKeyLength);
}

int8_t otGetMaxTransmitPower(void)
{
    return sThreadNetif->GetMac().GetMaxTransmitPower();
}

void otSetMaxTransmitPower(int8_t aPower)
{
    sThreadNetif->GetMac().SetMaxTransmitPower(aPower);
}

const otIp6Address *otGetMeshLocalEid(void)
{
    return sThreadNetif->GetMle().GetMeshLocal64();
}

const uint8_t *otGetMeshLocalPrefix(void)
{
    return sThreadNetif->GetMle().GetMeshLocalPrefix();
}

ThreadError otSetMeshLocalPrefix(const uint8_t *aMeshLocalPrefix)
{
    return sThreadNetif->GetMle().SetMeshLocalPrefix(aMeshLocalPrefix);
}

ThreadError otGetNetworkDataLeader(bool aStable, uint8_t *aData, uint8_t *aDataLength)
{
    ThreadError error = kThreadError_None;

    VerifyOrExit(aData != NULL && aDataLength != NULL, error = kThreadError_InvalidArgs);

    sThreadNetif->GetNetworkDataLeader().GetNetworkData(aStable, aData, *aDataLength);

exit:
    return error;
}

ThreadError otGetNetworkDataLocal(bool aStable, uint8_t *aData, uint8_t *aDataLength)
{
    ThreadError error = kThreadError_None;

    VerifyOrExit(aData != NULL && aDataLength != NULL, error = kThreadError_InvalidArgs);

    sThreadNetif->GetNetworkDataLocal().GetNetworkData(aStable, aData, *aDataLength);

exit:
    return error;
}

const char *otGetNetworkName(void)
{
    return sThreadNetif->GetMac().GetNetworkName();
}

ThreadError otSetNetworkName(const char *aNetworkName)
{
    return sThreadNetif->GetMac().SetNetworkName(aNetworkName);
}

otPanId otGetPanId(void)
{
    return sThreadNetif->GetMac().GetPanId();
}

ThreadError otSetPanId(otPanId aPanId)
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

bool otIsRouterRoleEnabled(void)
{
    return sThreadNetif->GetMle().IsRouterRoleEnabled();
}

void otSetRouterRoleEnabled(bool aEnabled)
{
    sThreadNetif->GetMle().SetRouterRoleEnabled(aEnabled);
}

otShortAddress otGetShortAddress(void)
{
    return sThreadNetif->GetMac().GetShortAddress();
}

uint8_t otGetLocalLeaderWeight(void)
{
    return sThreadNetif->GetMle().GetLeaderWeight();
}

void otSetLocalLeaderWeight(uint8_t aWeight)
{
    sThreadNetif->GetMle().SetLeaderWeight(aWeight);
}

uint32_t otGetLocalLeaderPartitionId(void)
{
    return sThreadNetif->GetMle().GetLeaderPartitionId();
}

void otSetLocalLeaderPartitionId(uint32_t aPartitionId)
{
    return sThreadNetif->GetMle().SetLeaderPartitionId(aPartitionId);
}

ThreadError otAddBorderRouter(const otBorderRouterConfig *aConfig)
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

ThreadError otRemoveBorderRouter(const otIp6Prefix *aPrefix)
{
    return sThreadNetif->GetNetworkDataLocal().RemoveOnMeshPrefix(aPrefix->mPrefix.mFields.m8, aPrefix->mLength);
}

ThreadError otGetNextOnMeshPrefix(bool aLocal, otNetworkDataIterator *aIterator, otBorderRouterConfig *aConfig)
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

ThreadError otAddExternalRoute(const otExternalRouteConfig *aConfig)
{
    return sThreadNetif->GetNetworkDataLocal().AddHasRoutePrefix(aConfig->mPrefix.mPrefix.mFields.m8,
                                                                 aConfig->mPrefix.mLength,
                                                                 aConfig->mPreference, aConfig->mStable);
}

ThreadError otRemoveExternalRoute(const otIp6Prefix *aPrefix)
{
    return sThreadNetif->GetNetworkDataLocal().RemoveHasRoutePrefix(aPrefix->mPrefix.mFields.m8, aPrefix->mLength);
}

ThreadError otSendServerData(void)
{
    Ip6::Address destination;
    sThreadNetif->GetMle().GetLeaderAddress(destination);
    return sThreadNetif->GetNetworkDataLocal().Register(destination);
}

ThreadError otAddUnsecurePort(uint16_t aPort)
{
    return sThreadNetif->GetIp6Filter().AddUnsecurePort(aPort);
}

ThreadError otRemoveUnsecurePort(uint16_t aPort)
{
    return sThreadNetif->GetIp6Filter().RemoveUnsecurePort(aPort);
}

const uint16_t *otGetUnsecurePorts(uint8_t *aNumEntries)
{
    return sThreadNetif->GetIp6Filter().GetUnsecurePorts(*aNumEntries);
}

uint32_t otGetContextIdReuseDelay(void)
{
    return sThreadNetif->GetNetworkDataLeader().GetContextIdReuseDelay();
}

void otSetContextIdReuseDelay(uint32_t aDelay)
{
    sThreadNetif->GetNetworkDataLeader().SetContextIdReuseDelay(aDelay);
}

uint32_t otGetKeySequenceCounter(void)
{
    return sThreadNetif->GetKeyManager().GetCurrentKeySequence();
}

void otSetKeySequenceCounter(uint32_t aKeySequenceCounter)
{
    sThreadNetif->GetKeyManager().SetCurrentKeySequence(aKeySequenceCounter);
}

uint8_t otGetNetworkIdTimeout(void)
{
    return sThreadNetif->GetMle().GetNetworkIdTimeout();
}

void otSetNetworkIdTimeout(uint8_t aTimeout)
{
    sThreadNetif->GetMle().SetNetworkIdTimeout(aTimeout);
}

uint8_t otGetRouterUpgradeThreshold(void)
{
    return sThreadNetif->GetMle().GetRouterUpgradeThreshold();
}

void otSetRouterUpgradeThreshold(uint8_t aThreshold)
{
    sThreadNetif->GetMle().SetRouterUpgradeThreshold(aThreshold);
}

ThreadError otReleaseRouterId(uint8_t aRouterId)
{
    return sThreadNetif->GetMle().ReleaseRouterId(aRouterId);
}

ThreadError otAddMacWhitelist(const uint8_t *aExtAddr)
{
    ThreadError error = kThreadError_None;

    if (sThreadNetif->GetMac().GetWhitelist().Add(*reinterpret_cast<const Mac::ExtAddress *>(aExtAddr)) == NULL)
    {
        error = kThreadError_NoBufs;
    }

    return error;
}

ThreadError otAddMacWhitelistRssi(const uint8_t *aExtAddr, int8_t aRssi)
{
    ThreadError error = kThreadError_None;
    otMacWhitelistEntry *entry;

    entry = sThreadNetif->GetMac().GetWhitelist().Add(*reinterpret_cast<const Mac::ExtAddress *>(aExtAddr));
    VerifyOrExit(entry != NULL, error = kThreadError_NoBufs);
    sThreadNetif->GetMac().GetWhitelist().SetFixedRssi(*entry, aRssi);

exit:
    return error;
}

void otRemoveMacWhitelist(const uint8_t *aExtAddr)
{
    sThreadNetif->GetMac().GetWhitelist().Remove(*reinterpret_cast<const Mac::ExtAddress *>(aExtAddr));
}

void otClearMacWhitelist(void)
{
    sThreadNetif->GetMac().GetWhitelist().Clear();
}

ThreadError otGetMacWhitelistEntry(uint8_t aIndex, otMacWhitelistEntry *aEntry)
{
    ThreadError error = kThreadError_None;

    VerifyOrExit(aEntry != NULL, error = kThreadError_InvalidArgs);
    error = sThreadNetif->GetMac().GetWhitelist().GetEntry(aIndex, *aEntry);

exit:
    return error;
}

void otDisableMacWhitelist(void)
{
    sThreadNetif->GetMac().GetWhitelist().Disable();
}

void otEnableMacWhitelist(void)
{
    sThreadNetif->GetMac().GetWhitelist().Enable();
}

bool otIsMacWhitelistEnabled(void)
{
    return sThreadNetif->GetMac().GetWhitelist().IsEnabled();
}

ThreadError otBecomeDetached(void)
{
    return sThreadNetif->GetMle().BecomeDetached();
}

ThreadError otBecomeChild(otMleAttachFilter aFilter)
{
    return sThreadNetif->GetMle().BecomeChild(aFilter);
}

ThreadError otBecomeRouter(void)
{
    return sThreadNetif->GetMle().BecomeRouter(ThreadStatusTlv::kTooFewRouters);
}

ThreadError otBecomeLeader(void)
{
    return sThreadNetif->GetMle().BecomeLeader();
}

ThreadError otAddMacBlacklist(const uint8_t *aExtAddr)
{
    ThreadError error = kThreadError_None;

    if (sThreadNetif->GetMac().GetBlacklist().Add(*reinterpret_cast<const Mac::ExtAddress *>(aExtAddr)) == NULL)
    {
        error = kThreadError_NoBufs;
    }

    return error;
}

void otRemoveMacBlacklist(const uint8_t *aExtAddr)
{
    sThreadNetif->GetMac().GetBlacklist().Remove(*reinterpret_cast<const Mac::ExtAddress *>(aExtAddr));
}

void otClearMacBlacklist(void)
{
    sThreadNetif->GetMac().GetBlacklist().Clear();
}

ThreadError otGetMacBlacklistEntry(uint8_t aIndex, otMacBlacklistEntry *aEntry)
{
    ThreadError error = kThreadError_None;

    VerifyOrExit(aEntry != NULL, error = kThreadError_InvalidArgs);
    error = sThreadNetif->GetMac().GetBlacklist().GetEntry(aIndex, *aEntry);

exit:
    return error;
}

void otDisableMacBlacklist(void)
{
    sThreadNetif->GetMac().GetBlacklist().Disable();
}

void otEnableMacBlacklist(void)
{
    sThreadNetif->GetMac().GetBlacklist().Enable();
}

bool otIsMacBlacklistEnabled(void)
{
    return sThreadNetif->GetMac().GetBlacklist().IsEnabled();
}

ThreadError otGetAssignLinkQuality(const uint8_t *aExtAddr, uint8_t *aLinkQuality)
{
    Mac::ExtAddress extAddress;

    memset(&extAddress, 0, sizeof(extAddress));
    memcpy(extAddress.m8, aExtAddr, OT_EXT_ADDRESS_SIZE);

    return sThreadNetif->GetMle().GetAssignLinkQuality(extAddress, *aLinkQuality);
}

void otSetAssignLinkQuality(const uint8_t *aExtAddr, uint8_t aLinkQuality)
{
    Mac::ExtAddress extAddress;

    memset(&extAddress, 0, sizeof(extAddress));
    memcpy(extAddress.m8, aExtAddr, OT_EXT_ADDRESS_SIZE);

    sThreadNetif->GetMle().SetAssignLinkQuality(extAddress, aLinkQuality);
}

void otPlatformReset(void)
{
    otPlatReset();
}

ThreadError otGetChildInfoById(uint16_t aChildId, otChildInfo *aChildInfo)
{
    ThreadError error = kThreadError_None;

    VerifyOrExit(aChildInfo != NULL, error = kThreadError_InvalidArgs);

    error = sThreadNetif->GetMle().GetChildInfoById(aChildId, *aChildInfo);

exit:
    return error;
}

ThreadError otGetChildInfoByIndex(uint8_t aChildIndex, otChildInfo *aChildInfo)
{
    ThreadError error = kThreadError_None;

    VerifyOrExit(aChildInfo != NULL, error = kThreadError_InvalidArgs);

    error = sThreadNetif->GetMle().GetChildInfoByIndex(aChildIndex, *aChildInfo);

exit:
    return error;
}

otDeviceRole otGetDeviceRole(void)
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

ThreadError otGetEidCacheEntry(uint8_t aIndex, otEidCacheEntry *aEntry)
{
    ThreadError error;

    VerifyOrExit(aEntry != NULL, error = kThreadError_InvalidArgs);
    error = sThreadNetif->GetAddressResolver().GetEntry(aIndex, *aEntry);

exit:
    return error;
}

ThreadError otGetLeaderData(otLeaderData *aLeaderData)
{
    ThreadError error;

    VerifyOrExit(aLeaderData != NULL, error = kThreadError_InvalidArgs);

    error = sThreadNetif->GetMle().GetLeaderData(*aLeaderData);

exit:
    return error;
}

uint8_t otGetLeaderRouterId(void)
{
    return sThreadNetif->GetMle().GetLeaderDataTlv().GetLeaderRouterId();
}

uint8_t otGetLeaderWeight(void)
{
    return sThreadNetif->GetMle().GetLeaderDataTlv().GetWeighting();
}

uint8_t otGetNetworkDataVersion(void)
{
    return sThreadNetif->GetMle().GetLeaderDataTlv().GetDataVersion();
}

uint32_t otGetPartitionId(void)
{
    return sThreadNetif->GetMle().GetLeaderDataTlv().GetPartitionId();
}

uint16_t otGetRloc16(void)
{
    return sThreadNetif->GetMle().GetRloc16();
}

uint8_t otGetRouterIdSequence(void)
{
    return sThreadNetif->GetMle().GetRouterIdSequence();
}

ThreadError otGetRouterInfo(uint16_t aRouterId, otRouterInfo *aRouterInfo)
{
    ThreadError error = kThreadError_None;

    VerifyOrExit(aRouterInfo != NULL, error = kThreadError_InvalidArgs);

    error = sThreadNetif->GetMle().GetRouterInfo(aRouterId, *aRouterInfo);

exit:
    return error;
}

ThreadError otGetParentInfo(otRouterInfo *aParentInfo)
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

uint8_t otGetStableNetworkDataVersion(void)
{
    return sThreadNetif->GetMle().GetLeaderDataTlv().GetStableDataVersion();
}

void otSetLinkPcapCallback(otLinkPcapCallback aPcapCallback)
{
    sThreadNetif->GetMac().SetPcapCallback(aPcapCallback);
}

bool otIsLinkPromiscuous(void)
{
    return sThreadNetif->GetMac().IsPromiscuous();
}

ThreadError otSetLinkPromiscuous(bool aPromiscuous)
{
    ThreadError error = kThreadError_None;

    // cannot enable IEEE 802.15.4 promiscuous mode if the Thread interface is enabled
    VerifyOrExit(sThreadNetif->IsUp() == false, error = kThreadError_Busy);

    sThreadNetif->GetMac().SetPromiscuous(aPromiscuous);

exit:
    return error;
}

const otMacCounters *otGetMacCounters(void)
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

const otNetifAddress *otGetUnicastAddresses(void)
{
    return sThreadNetif->GetUnicastAddresses();
}

ThreadError otAddUnicastAddress(otNetifAddress *address)
{
    return sThreadNetif->AddUnicastAddress(*static_cast<Ip6::NetifUnicastAddress *>(address));
}

ThreadError otRemoveUnicastAddress(otNetifAddress *address)
{
    return sThreadNetif->RemoveUnicastAddress(*static_cast<Ip6::NetifUnicastAddress *>(address));
}

void otSetStateChangedCallback(otStateChangedCallback aCallback, void *aContext)
{
    sNetifCallback.Set(aCallback, aContext);
    sThreadNetif->RegisterCallback(sNetifCallback);
}

const char *otGetVersionString(void)
{
    static const char sVersion[] =
        PACKAGE_NAME "/" PACKAGE_VERSION "; "
#ifdef  PLATFORM_INFO
        PLATFORM_INFO "; "
#endif
        __DATE__ " " __TIME__;

    return sVersion;
}

uint32_t otGetPollPeriod()
{
    return sThreadNetif->GetMeshForwarder().GetAssignPollPeriod();
}

void otSetPollPeriod(uint32_t aPollPeriod)
{
    sThreadNetif->GetMeshForwarder().SetAssignPollPeriod(aPollPeriod);
}

ThreadError otEnable(void)
{
    ThreadError error = kThreadError_None;

    VerifyOrExit(!mEnabled, error = kThreadError_InvalidState);

    otLogInfoApi("otEnable\n");
    Message::Init();
    sThreadNetif = new(&sThreadNetifRaw) ThreadNetif;
    Ip6::Ip6::Init();
    mEnabled = true;

exit:
    return error;
}

ThreadError otDisable(void)
{
    ThreadError error = kThreadError_None;

    VerifyOrExit(mEnabled, error = kThreadError_InvalidState);

    otThreadStop();
    otInterfaceDown();
    mEnabled = false;

exit:
    return error;
}

ThreadError otInterfaceUp(void)
{
    ThreadError error = kThreadError_None;

    VerifyOrExit(mEnabled, error = kThreadError_InvalidState);

    error = sThreadNetif->Up();

exit:
    return error;
}

ThreadError otInterfaceDown(void)
{
    ThreadError error = kThreadError_None;

    VerifyOrExit(mEnabled, error = kThreadError_InvalidState);

    error = sThreadNetif->Down();

exit:
    return error;
}

bool otIsInterfaceUp(void)
{
    return mEnabled && sThreadNetif->IsUp();
}

ThreadError otThreadStart(void)
{
    ThreadError error = kThreadError_None;

    VerifyOrExit(mEnabled, error = kThreadError_InvalidState);
    VerifyOrExit(sThreadNetif->GetMac().GetPanId() != Mac::kPanIdBroadcast, error = kThreadError_InvalidState);

    error = sThreadNetif->GetMle().Start();

exit:
    return error;
}

ThreadError otThreadStop(void)
{
    ThreadError error = kThreadError_None;

    VerifyOrExit(mEnabled, error = kThreadError_InvalidState);

    error = sThreadNetif->GetMle().Stop();

exit:
    return error;
}

bool otIsSingleton(void)
{
    return mEnabled && sThreadNetif->GetMle().IsSingleton();
}

ThreadError otActiveScan(uint32_t aScanChannels, uint16_t aScanDuration, otHandleActiveScanResult aCallback)
{
    return sThreadNetif->GetMac().ActiveScan(aScanChannels, aScanDuration, &HandleActiveScanResult,
                                             reinterpret_cast<void *>(aCallback));
}

bool otActiveScanInProgress(void)
{
    return sThreadNetif->GetMac().IsActiveScanInProgress();
}

void HandleActiveScanResult(void *aContext, Mac::Frame *aFrame)
{
    otHandleActiveScanResult handler = reinterpret_cast<otHandleActiveScanResult>(aContext);
    otActiveScanResult result;
    Mac::Address address;
    Mac::Beacon *beacon;
    uint8_t payloadLength;

    memset(&result, 0, sizeof(otActiveScanResult));

    if (aFrame == NULL)
    {
        handler(NULL);
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

    handler(&result);

exit:
    return;
}

ThreadError otDiscover(uint32_t aScanChannels, uint16_t aScanDuration, uint16_t aPanId,
                       otHandleActiveScanResult aCallback)
{
    return sThreadNetif->GetMle().Discover(aScanChannels, aScanDuration, aPanId, &HandleMleDiscover,
                                           reinterpret_cast<void *>(aCallback));
}

void HandleMleDiscover(otActiveScanResult *aResult, void *aContext)
{
    otHandleActiveScanResult handler = reinterpret_cast<otHandleActiveScanResult>(aContext);
    handler(aResult);
}

void otSetReceiveIp6DatagramCallback(otReceiveIp6DatagramCallback aCallback)
{
    Ip6::Ip6::SetReceiveDatagramCallback(aCallback);
}

ThreadError otSendIp6Datagram(otMessage aMessage)
{
    return Ip6::Ip6::HandleDatagram(*static_cast<Message *>(aMessage), NULL, sThreadNetif->GetInterfaceId(),
                                    NULL, true);
}

otMessage otNewUdpMessage(void)
{
    return Ip6::Udp::NewMessage(0);
}

ThreadError otFreeMessage(otMessage aMessage)
{
    return Message::Free(*static_cast<Message *>(aMessage));
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

int otAppendMessage(otMessage aMessage, const void *aBuf, uint16_t aLength)
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

ThreadError otOpenUdpSocket(otUdpSocket *aSocket, otUdpReceive aCallback, void *aContext)
{
    Ip6::UdpSocket *socket = reinterpret_cast<Ip6::UdpSocket *>(aSocket);
    return socket->Open(aCallback, aContext);
}

ThreadError otCloseUdpSocket(otUdpSocket *aSocket)
{
    Ip6::UdpSocket *socket = reinterpret_cast<Ip6::UdpSocket *>(aSocket);
    return socket->Close();
}

ThreadError otBindUdpSocket(otUdpSocket *aSocket, otSockAddr *aSockName)
{
    Ip6::UdpSocket *socket = reinterpret_cast<Ip6::UdpSocket *>(aSocket);
    return socket->Bind(*reinterpret_cast<const Ip6::SockAddr *>(aSockName));
}

ThreadError otSendUdp(otUdpSocket *aSocket, otMessage aMessage, const otMessageInfo *aMessageInfo)
{
    Ip6::UdpSocket *socket = reinterpret_cast<Ip6::UdpSocket *>(aSocket);
    return socket->SendTo(*reinterpret_cast<Message *>(aMessage),
                          *reinterpret_cast<const Ip6::MessageInfo *>(aMessageInfo));
}

bool otIsIcmpEchoEnabled(void)
{
    return Ip6::Icmp::IsEchoEnabled();
}

void otSetIcmpEchoEnabled(bool aEnabled)
{
    Ip6::Icmp::SetEchoEnabled(aEnabled);
}

uint8_t otIp6PrefixMatch(const otIp6Address *aFirst, const otIp6Address *aSecond)
{
    uint8_t rval;

    VerifyOrExit(aFirst != NULL && aSecond != NULL, rval = 0);

    rval = static_cast<const Ip6::Address *>(aFirst)->PrefixMatch(*static_cast<const Ip6::Address *>(aSecond));

exit:
    return rval;
}

ThreadError otGetActiveDataset(otOperationalDataset *aDataset)
{
    ThreadError error = kThreadError_None;

    VerifyOrExit(aDataset != NULL, error = kThreadError_InvalidArgs);

    sThreadNetif->GetActiveDataset().Get(*aDataset);

exit:
    return error;
}

ThreadError otSetActiveDataset(otOperationalDataset *aDataset)
{
    ThreadError error;

    VerifyOrExit(aDataset != NULL, error = kThreadError_InvalidArgs);

    error = sThreadNetif->GetActiveDataset().Set(*aDataset);

exit:
    return error;
}

ThreadError otGetPendingDataset(otOperationalDataset *aDataset)
{
    ThreadError error = kThreadError_None;

    VerifyOrExit(aDataset != NULL, error = kThreadError_InvalidArgs);

    sThreadNetif->GetPendingDataset().Get(*aDataset);

exit:
    return error;
}

ThreadError otSetPendingDataset(otOperationalDataset *aDataset)
{
    ThreadError error;

    VerifyOrExit(aDataset != NULL, error = kThreadError_InvalidArgs);

    error = sThreadNetif->GetPendingDataset().Set(*aDataset);

exit:
    return error;
}

#ifdef __cplusplus
}  // extern "C"
#endif

}  // namespace Thread
