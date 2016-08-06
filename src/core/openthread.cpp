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
#include <common/code_utils.hpp>
#include <common/debug.hpp>
#include <common/logging.hpp>
#include <common/message.hpp>
#include <common/new.hpp>
#include <common/tasklet.hpp>
#include <common/timer.hpp>
#include <net/icmp6.hpp>
#include <platform/random.h>
#include <thread/thread_netif.hpp>
#include <openthreadcontext.h>

#ifdef WINDOWS_LOGGING
#include <openthread.tmh>
#endif

otContext::otContext(void) :
    mReceiveIp6DatagramCallback(NULL),
    mEphemeralPort(Thread::Ip6::Udp::kDynamicPortMin),
    mIcmpHandlers(NULL),
#ifndef OPEN_THREAD_DRIVER
    mIsEchoEnabled(true),
#else
    mIsEchoEnabled(false),
#endif
    mNextId(1),
    mEchoClients(NULL),
    mRoutes(NULL),
    mNetifListHead(NULL),
    mNextInterfaceId(1),
    mMac(NULL),
    mTimerHead(NULL),
    mTimerTail(NULL),
    mTaskletHead(NULL),
    mTaskletTail(NULL),
    mUdpSockets(NULL),
    mEnabled(false),
    mThreadNetif(this),
    mMpl(this)
{
    mCryptoContext.mIsInitialized = false;
    Thread::Message::Init(this);
    mEnabled = true;
}

namespace Thread {

#ifdef __cplusplus
extern "C" {
#endif

static void HandleActiveScanResult(void *aContext, Mac::Frame *aFrame);
static void HandleMleDiscover(otActiveScanResult *aResult, void *aContext);

void otProcessNextTasklet(otContext *aContext)
{
    otLogFuncEntry();
    TaskletScheduler::RunNextTasklet(aContext);
    otLogFuncExit();
}

bool otAreTaskletsPending(otContext *aContext)
{
    return TaskletScheduler::AreTaskletsPending(aContext);
}

uint8_t otGetChannel(otContext *aContext)
{
    return aContext->mThreadNetif.GetMac().GetChannel();
}

ThreadError otSetChannel(otContext *aContext, uint8_t aChannel)
{
    return aContext->mThreadNetif.GetMac().SetChannel(aChannel);
}

uint32_t otGetChildTimeout(otContext *aContext)
{
    return aContext->mThreadNetif.GetMle().GetTimeout();
}

void otSetChildTimeout(otContext *aContext, uint32_t aTimeout)
{
    aContext->mThreadNetif.GetMle().SetTimeout(aTimeout);
}

const uint8_t *otGetExtendedAddress(otContext *aContext)
{
    return reinterpret_cast<const uint8_t *>(aContext->mThreadNetif.GetMac().GetExtAddress());
}

ThreadError otSetExtendedAddress(otContext *aContext, const otExtAddress *aExtAddress)
{
    ThreadError error = kThreadError_None;

    VerifyOrExit(aExtAddress != NULL, error = kThreadError_InvalidArgs);

    SuccessOrExit(error = aContext->mThreadNetif.GetMac().SetExtAddress(*static_cast<const Mac::ExtAddress *>(aExtAddress)));
    SuccessOrExit(error = aContext->mThreadNetif.GetMle().UpdateLinkLocalAddress());

exit:
    return error;
}

const uint8_t *otGetExtendedPanId(otContext *aContext)
{
    return aContext->mThreadNetif.GetMac().GetExtendedPanId();
}

void otSetExtendedPanId(otContext *aContext, const uint8_t *aExtendedPanId)
{
    uint8_t mlPrefix[8];

    aContext->mThreadNetif.GetMac().SetExtendedPanId(aExtendedPanId);

    mlPrefix[0] = 0xfd;
    memcpy(mlPrefix + 1, aExtendedPanId, 5);
    mlPrefix[6] = 0x00;
    mlPrefix[7] = 0x00;
    aContext->mThreadNetif.GetMle().SetMeshLocalPrefix(mlPrefix);
}

ThreadError otGetLeaderRloc(otContext *aContext, otIp6Address *aAddress)
{
    ThreadError error;

    VerifyOrExit(aAddress != NULL, error = kThreadError_InvalidArgs);

    error = aContext->mThreadNetif.GetMle().GetLeaderAddress(*static_cast<Ip6::Address *>(aAddress));

exit:
    return error;
}

otLinkModeConfig otGetLinkMode(otContext *aContext)
{
    otLinkModeConfig config;
    uint8_t mode = aContext->mThreadNetif.GetMle().GetDeviceMode();

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

ThreadError otSetLinkMode(otContext *aContext, otLinkModeConfig aConfig)
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

    return aContext->mThreadNetif.GetMle().SetDeviceMode(mode);
}

const uint8_t *otGetMasterKey(otContext *aContext, uint8_t *aKeyLength)
{
    return aContext->mThreadNetif.GetKeyManager().GetMasterKey(aKeyLength);
}

ThreadError otSetMasterKey(otContext *aContext, const uint8_t *aKey, uint8_t aKeyLength)
{
    return aContext->mThreadNetif.GetKeyManager().SetMasterKey(aKey, aKeyLength);
}

const otIp6Address *otGetMeshLocalEid(otContext *aContext)
{
    return aContext->mThreadNetif.GetMle().GetMeshLocal64();
}

const uint8_t *otGetMeshLocalPrefix(otContext *aContext)
{
    return aContext->mThreadNetif.GetMle().GetMeshLocalPrefix();
}

ThreadError otSetMeshLocalPrefix(otContext *aContext, const uint8_t *aMeshLocalPrefix)
{
    return aContext->mThreadNetif.GetMle().SetMeshLocalPrefix(aMeshLocalPrefix);
}

ThreadError otGetNetworkDataLeader(otContext *aContext, bool aStable, uint8_t *aData, uint8_t *aDataLength)
{
    ThreadError error = kThreadError_None;

    VerifyOrExit(aData != NULL && aDataLength != NULL, error = kThreadError_InvalidArgs);

    aContext->mThreadNetif.GetNetworkDataLeader().GetNetworkData(aStable, aData, *aDataLength);

exit:
    return error;
}

ThreadError otGetNetworkDataLocal(otContext *aContext, bool aStable, uint8_t *aData, uint8_t *aDataLength)
{
    ThreadError error = kThreadError_None;

    VerifyOrExit(aData != NULL && aDataLength != NULL, error = kThreadError_InvalidArgs);

    aContext->mThreadNetif.GetNetworkDataLocal().GetNetworkData(aStable, aData, *aDataLength);

exit:
    return error;
}

const char *otGetNetworkName(otContext *aContext)
{
    return aContext->mThreadNetif.GetMac().GetNetworkName();
}

ThreadError otSetNetworkName(otContext *aContext, const char *aNetworkName)
{
    return aContext->mThreadNetif.GetMac().SetNetworkName(aNetworkName);
}

otPanId otGetPanId(otContext *aContext)
{
    return aContext->mThreadNetif.GetMac().GetPanId();
}

ThreadError otSetPanId(otContext *aContext, otPanId aPanId)
{
    return aContext->mThreadNetif.GetMac().SetPanId(aPanId);
}

bool otIsRouterRoleEnabled(otContext *aContext)
{
    return aContext->mThreadNetif.GetMle().IsRouterRoleEnabled();
}

void otSetRouterRoleEnabled(otContext *aContext, bool aEnabled)
{
    aContext->mThreadNetif.GetMle().SetRouterRoleEnabled(aEnabled);
}

otShortAddress otGetShortAddress(otContext *aContext)
{
    return aContext->mThreadNetif.GetMac().GetShortAddress();
}

uint8_t otGetLocalLeaderWeight(otContext *aContext)
{
    return aContext->mThreadNetif.GetMle().GetLeaderWeight();
}

void otSetLocalLeaderWeight(otContext *aContext, uint8_t aWeight)
{
    aContext->mThreadNetif.GetMle().SetLeaderWeight(aWeight);
}

ThreadError otAddBorderRouter(otContext *aContext, const otBorderRouterConfig *aConfig)
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

    return aContext->mThreadNetif.GetNetworkDataLocal().AddOnMeshPrefix(aConfig->mPrefix.mPrefix.mFields.m8,
                                                               aConfig->mPrefix.mLength,
                                                               aConfig->mPreference, flags, aConfig->mStable);
}

ThreadError otRemoveBorderRouter(otContext *aContext, const otIp6Prefix *aPrefix)
{
    return aContext->mThreadNetif.GetNetworkDataLocal().RemoveOnMeshPrefix(aPrefix->mPrefix.mFields.m8, aPrefix->mLength);
}

ThreadError otAddExternalRoute(otContext *aContext, const otExternalRouteConfig *aConfig)
{
    return aContext->mThreadNetif.GetNetworkDataLocal().AddHasRoutePrefix(aConfig->mPrefix.mPrefix.mFields.m8,
                                                                 aConfig->mPrefix.mLength,
                                                                 aConfig->mPreference, aConfig->mStable);
}

ThreadError otRemoveExternalRoute(otContext *aContext, const otIp6Prefix *aPrefix)
{
    return aContext->mThreadNetif.GetNetworkDataLocal().RemoveHasRoutePrefix(aPrefix->mPrefix.mFields.m8, aPrefix->mLength);
}

ThreadError otSendServerData(otContext *aContext)
{
    Ip6::Address destination;
    aContext->mThreadNetif.GetMle().GetLeaderAddress(destination);
    return aContext->mThreadNetif.GetNetworkDataLocal().Register(destination);
}

ThreadError otAddUnsecurePort(otContext *aContext, uint16_t aPort)
{
    return aContext->mThreadNetif.GetIp6Filter().AddUnsecurePort(aPort);
}

ThreadError otRemoveUnsecurePort(otContext *aContext, uint16_t aPort)
{
    return aContext->mThreadNetif.GetIp6Filter().RemoveUnsecurePort(aPort);
}

const uint16_t *otGetUnsecurePorts(otContext *aContext, uint8_t *aNumEntries)
{
    return aContext->mThreadNetif.GetIp6Filter().GetUnsecurePorts(*aNumEntries);
}

uint32_t otGetContextIdReuseDelay(otContext *aContext)
{
    return aContext->mThreadNetif.GetNetworkDataLeader().GetContextIdReuseDelay();
}

void otSetContextIdReuseDelay(otContext *aContext, uint32_t aDelay)
{
    aContext->mThreadNetif.GetNetworkDataLeader().SetContextIdReuseDelay(aDelay);
}

uint32_t otGetKeySequenceCounter(otContext *aContext)
{
    return aContext->mThreadNetif.GetKeyManager().GetCurrentKeySequence();
}

void otSetKeySequenceCounter(otContext *aContext, uint32_t aKeySequenceCounter)
{
    aContext->mThreadNetif.GetKeyManager().SetCurrentKeySequence(aKeySequenceCounter);
}

uint32_t otGetNetworkIdTimeout(otContext *aContext)
{
    return aContext->mThreadNetif.GetMle().GetNetworkIdTimeout();
}

void otSetNetworkIdTimeout(otContext *aContext, uint32_t aTimeout)
{
    aContext->mThreadNetif.GetMle().SetNetworkIdTimeout((uint8_t)aTimeout);
}

uint8_t otGetRouterUpgradeThreshold(otContext *aContext)
{
    return aContext->mThreadNetif.GetMle().GetRouterUpgradeThreshold();
}

void otSetRouterUpgradeThreshold(otContext *aContext, uint8_t aThreshold)
{
    aContext->mThreadNetif.GetMle().SetRouterUpgradeThreshold(aThreshold);
}

ThreadError otReleaseRouterId(otContext *aContext, uint8_t aRouterId)
{
    return aContext->mThreadNetif.GetMle().ReleaseRouterId(aRouterId);
}

ThreadError otAddMacWhitelist(otContext *aContext, const uint8_t *aExtAddr)
{
    ThreadError error = kThreadError_None;

    if (aContext->mThreadNetif.GetMac().GetWhitelist().Add(*reinterpret_cast<const Mac::ExtAddress *>(aExtAddr)) == NULL)
    {
        error = kThreadError_NoBufs;
    }

    return error;
}

ThreadError otAddMacWhitelistRssi(otContext *aContext, const uint8_t *aExtAddr, int8_t aRssi)
{
    ThreadError error = kThreadError_None;
    otMacWhitelistEntry *entry;

    entry = aContext->mThreadNetif.GetMac().GetWhitelist().Add(*reinterpret_cast<const Mac::ExtAddress *>(aExtAddr));
    VerifyOrExit(entry != NULL, error = kThreadError_NoBufs);
    aContext->mThreadNetif.GetMac().GetWhitelist().SetFixedRssi(*entry, aRssi);

exit:
    return error;
}

void otRemoveMacWhitelist(otContext *aContext, const uint8_t *aExtAddr)
{
    aContext->mThreadNetif.GetMac().GetWhitelist().Remove(*reinterpret_cast<const Mac::ExtAddress *>(aExtAddr));
}

void otClearMacWhitelist(otContext *aContext)
{
    aContext->mThreadNetif.GetMac().GetWhitelist().Clear();
}

ThreadError otGetMacWhitelistEntry(otContext *aContext, uint8_t aIndex, otMacWhitelistEntry *aEntry)
{
    ThreadError error;

    VerifyOrExit(aEntry != NULL, error = kThreadError_InvalidArgs);
    error = aContext->mThreadNetif.GetMac().GetWhitelist().GetEntry(aIndex, *aEntry);

exit:
    return error;
}

void otDisableMacWhitelist(otContext *aContext)
{
    aContext->mThreadNetif.GetMac().GetWhitelist().Disable();
}

void otEnableMacWhitelist(otContext *aContext)
{
    aContext->mThreadNetif.GetMac().GetWhitelist().Enable();
}

bool otIsMacWhitelistEnabled(otContext *aContext)
{
    return aContext->mThreadNetif.GetMac().GetWhitelist().IsEnabled();
}

ThreadError otBecomeDetached(otContext *aContext)
{
    return aContext->mThreadNetif.GetMle().BecomeDetached();
}

ThreadError otBecomeChild(otContext *aContext, otMleAttachFilter aFilter)
{
    return aContext->mThreadNetif.GetMle().BecomeChild(aFilter);
}

ThreadError otBecomeRouter(otContext *aContext)
{
    return aContext->mThreadNetif.GetMle().BecomeRouter(ThreadStatusTlv::kTooFewRouters);
}

ThreadError otBecomeLeader(otContext *aContext)
{
    return aContext->mThreadNetif.GetMle().BecomeLeader();
}

ThreadError otGetChildInfoById(otContext *aContext, uint16_t aChildId, otChildInfo *aChildInfo)
{
    ThreadError error = kThreadError_None;

    VerifyOrExit(aChildInfo != NULL, error = kThreadError_InvalidArgs);

    error = aContext->mThreadNetif.GetMle().GetChildInfoById(aChildId, *aChildInfo);

exit:
    return error;
}

ThreadError otGetChildInfoByIndex(otContext *aContext, uint8_t aChildIndex, otChildInfo *aChildInfo)
{
    ThreadError error = kThreadError_None;

    VerifyOrExit(aChildInfo != NULL, error = kThreadError_InvalidArgs);

    error = aContext->mThreadNetif.GetMle().GetChildInfoByIndex(aChildIndex, *aChildInfo);

exit:
    return error;
}

otDeviceRole otGetDeviceRole(otContext *aContext)
{
    otDeviceRole rval = kDeviceRoleDisabled;

    switch (aContext->mThreadNetif.GetMle().GetDeviceState())
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

ThreadError otGetEidCacheEntry(otContext *aContext, uint8_t aIndex, otEidCacheEntry *aEntry)
{
    ThreadError error;

    VerifyOrExit(aEntry != NULL, error = kThreadError_InvalidArgs);
    error = aContext->mThreadNetif.GetAddressResolver().GetEntry(aIndex, *aEntry);

exit:
    return error;
}

ThreadError otGetLeaderData(otContext *aContext, otLeaderData *aLeaderData)
{
    ThreadError error;

    VerifyOrExit(aLeaderData != NULL, error = kThreadError_InvalidArgs);

    error = aContext->mThreadNetif.GetMle().GetLeaderData(*aLeaderData);

exit:
    return error;
}

uint8_t otGetLeaderRouterId(otContext *aContext)
{
    return aContext->mThreadNetif.GetMle().GetLeaderDataTlv().GetLeaderRouterId();
}

uint8_t otGetLeaderWeight(otContext *aContext)
{
    return aContext->mThreadNetif.GetMle().GetLeaderDataTlv().GetWeighting();
}

uint8_t otGetNetworkDataVersion(otContext *aContext)
{
    return aContext->mThreadNetif.GetMle().GetLeaderDataTlv().GetDataVersion();
}

uint32_t otGetPartitionId(otContext *aContext)
{
    return aContext->mThreadNetif.GetMle().GetLeaderDataTlv().GetPartitionId();
}

uint16_t otGetRloc16(otContext *aContext)
{
    return aContext->mThreadNetif.GetMle().GetRloc16();
}

uint8_t otGetRouterIdSequence(otContext *aContext)
{
    return aContext->mThreadNetif.GetMle().GetRouterIdSequence();
}

ThreadError otGetRouterInfo(otContext *aContext, uint16_t aRouterId, otRouterInfo *aRouterInfo)
{
    ThreadError error = kThreadError_None;

    VerifyOrExit(aRouterInfo != NULL, error = kThreadError_InvalidArgs);

    error = aContext->mThreadNetif.GetMle().GetRouterInfo(aRouterId, *aRouterInfo);

exit:
    return error;
}

uint8_t otGetStableNetworkDataVersion(otContext *aContext)
{
    return aContext->mThreadNetif.GetMle().GetLeaderDataTlv().GetStableDataVersion();
}

void otSetLinkPcapCallback(otContext *aContext, otLinkPcapCallback aPcapCallback)
{
    aContext->mThreadNetif.GetMac().SetPcapCallback(aPcapCallback);
}

bool otIsLinkPromiscuous(otContext *aContext)
{
    return aContext->mThreadNetif.GetMac().IsPromiscuous();
}

ThreadError otSetLinkPromiscuous(otContext *aContext, bool aPromiscuous)
{
    ThreadError error = kThreadError_None;

    // cannot enable IEEE 802.15.4 promiscuous mode if the Thread interface is enabled
    VerifyOrExit(aContext->mThreadNetif.IsUp() == false, error = kThreadError_Busy);

    aContext->mThreadNetif.GetMac().SetPromiscuous(aPromiscuous);

exit:
    return error;
}

const otMacCounters *otGetMacCounters(otContext *aContext)
{
    return &aContext->mThreadNetif.GetMac().GetCounters();
}

bool otIsIp6AddressEqual(const otIp6Address *a, const otIp6Address *b)
{
    return *static_cast<const Ip6::Address *>(a) == *static_cast<const Ip6::Address *>(b);
}

ThreadError otIp6AddressFromString(const char *str, otIp6Address *address)
{
    return static_cast<Ip6::Address *>(address)->FromString(str);
}

const otNetifAddress *otGetUnicastAddresses(otContext *aContext)
{
    return aContext->mThreadNetif.GetUnicastAddresses();
}

ThreadError otAddUnicastAddress(otContext *aContext, otNetifAddress *address)
{
    return aContext->mThreadNetif.AddUnicastAddress(*static_cast<Ip6::NetifUnicastAddress *>(address));
}

ThreadError otRemoveUnicastAddress(otContext *aContext, otNetifAddress *address)
{
    return aContext->mThreadNetif.RemoveUnicastAddress(*static_cast<Ip6::NetifUnicastAddress *>(address));
}

void otSetStateChangedCallback(otContext *aContext, otStateChangedCallback aCallback, void *aCallbackContext)
{
    aContext->mNetifCallback.Set(aCallback, aCallbackContext);
    aContext->mThreadNetif.RegisterCallback(aContext->mNetifCallback);
}

otContext* otEnable(void *aContextBuffer, uint64_t *aContextBufferSize)
{    
    otContext* aContext = NULL;

    otLogFuncEntry();

    otLogInfoApi("otEnable\n");
    
    VerifyOrExit(aContextBufferSize != NULL, ;);

    // Make sure the input buffer is big enough
    VerifyOrExit(cAlignedContextSize <= *aContextBufferSize, *aContextBufferSize = cAlignedContextSize);

    aContext = new(aContextBuffer)otContext();

exit:
    
    otLogFuncExit();

    return aContext;
}

ThreadError otDisable(otContext *aContext)
{
    ThreadError error = kThreadError_None;

    otLogFuncEntry();

    VerifyOrExit(aContext->mEnabled, error = kThreadError_InvalidState);

    otThreadStop(aContext);
    otInterfaceDown(aContext);
    aContext->mEnabled = false;

exit:
    
    otLogFuncExitErr(error);
    return error;
}

ThreadError otInterfaceUp(otContext *aContext)
{
    ThreadError error = kThreadError_None;

    otLogFuncEntry();

    VerifyOrExit(aContext->mEnabled, error = kThreadError_InvalidState);

    error = aContext->mThreadNetif.Up();

exit:
    
    otLogFuncExitErr(error);
    return error;
}

ThreadError otInterfaceDown(otContext *aContext)
{
    ThreadError error = kThreadError_None;

    otLogFuncEntry();

    VerifyOrExit(aContext->mEnabled, error = kThreadError_InvalidState);

    error = aContext->mThreadNetif.Down();

exit:
    
    otLogFuncExitErr(error);
    return error;
}

bool otIsInterfaceUp(otContext *aContext)
{
    return aContext->mEnabled && aContext->mThreadNetif.IsUp();
}

ThreadError otThreadStart(otContext *aContext)
{
    ThreadError error = kThreadError_None;

    otLogFuncEntry();

    VerifyOrExit(aContext->mEnabled, error = kThreadError_InvalidState);

    error = aContext->mThreadNetif.GetMle().Start();

exit:
    
    otLogFuncExitErr(error);
    return error;
}

ThreadError otThreadStop(otContext *aContext)
{
    ThreadError error = kThreadError_None;

    otLogFuncEntry();

    VerifyOrExit(aContext->mEnabled, error = kThreadError_InvalidState);

    error = aContext->mThreadNetif.GetMle().Stop();

exit:
    
    otLogFuncExitErr(error);
    return error;
}

ThreadError otActiveScan(otContext *aContext, uint32_t aScanChannels, uint16_t aScanDuration, otHandleActiveScanResult aCallback)
{
    return aContext->mThreadNetif.GetMac().ActiveScan(aScanChannels, aScanDuration, &HandleActiveScanResult,
                                             reinterpret_cast<void *>(aCallback));
}

bool otActiveScanInProgress(otContext *aContext)
{
    return aContext->mThreadNetif.GetMac().IsActiveScanInProgress();
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
        result.mNetworkName = beacon->GetNetworkName();
        result.mExtPanId = beacon->GetExtendedPanId();
    }

    handler(&result);

exit:
    return;
}

ThreadError otDiscover(otContext *aContext, uint32_t aScanChannels, uint16_t aScanDuration, uint16_t aPanId,
                       otHandleActiveScanResult aCallback)
{
    return aContext->mThreadNetif.GetMle().Discover(aScanChannels, aScanDuration, aPanId, &HandleMleDiscover,
                                           reinterpret_cast<void *>(aCallback));
}

void HandleMleDiscover(otActiveScanResult *aResult, void *aContext)
{
    otHandleActiveScanResult handler = reinterpret_cast<otHandleActiveScanResult>(aContext);
    handler(aResult);
}

void otSetReceiveIp6DatagramCallback(otContext *aContext, otReceiveIp6DatagramCallback aCallback, void *aCallbackContext)
{
    Ip6::Ip6::SetReceiveDatagramCallback(aContext, aCallback, aCallbackContext);
}

ThreadError otSendIp6Datagram(otContext *aContext, otMessage aMessage)
{
    otLogFuncEntry();
    ThreadError error = 
        Ip6::Ip6::HandleDatagram(
            *static_cast<Message *>(aMessage), 
            NULL, 
            (uint8_t)aContext->mThreadNetif.GetInterfaceId(),
            NULL, 
            true
            );
    otLogFuncExitErr(error);
    return error;
}

otMessage otNewIPv6Message(otContext *aContext, uint16_t aLength)
{
    return Ip6::Ip6::NewPrepopulatedMessage(aContext, aLength);
}

otMessage otNewUdpMessage(otContext *aContext)
{
    return Ip6::Udp::NewMessage(aContext, 0);
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

ThreadError otOpenUdpSocket(otContext *aContext, otUdpSocket *aSocket, otUdpReceive aCallback, void *aCallbackContext)
{
    Ip6::UdpSocket *socket = reinterpret_cast<Ip6::UdpSocket *>(aSocket);
    return socket->Open(aContext, aCallback, aCallbackContext);
}

ThreadError otCloseUdpSocket(otContext *aContext, otUdpSocket *aSocket)
{
    Ip6::UdpSocket *socket = reinterpret_cast<Ip6::UdpSocket *>(aSocket);
    return socket->Close(aContext);
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

bool otIsIcmpEchoEnabled(otContext *aContext)
{
    return Ip6::Icmp::IsEchoEnabled(aContext);
}

void otSetIcmpEchoEnabled(otContext *aContext, bool aEnabled)
{
    Ip6::Icmp::SetEchoEnabled(aContext, aEnabled);
}

ThreadError otGetActiveDataset(otContext *aContext, otOperationalDataset *aDataset)
{
    ThreadError error = kThreadError_None;

    VerifyOrExit(aDataset != NULL, error = kThreadError_InvalidArgs);

    aContext->mThreadNetif.GetActiveDataset().Get(*aDataset);

exit:
    return error;
}

ThreadError otSetActiveDataset(otContext *aContext, otOperationalDataset *aDataset)
{
    ThreadError error;

    VerifyOrExit(aDataset != NULL, error = kThreadError_InvalidArgs);

    error = aContext->mThreadNetif.GetActiveDataset().Set(*aDataset);

exit:
    return error;
}

ThreadError otGetPendingDataset(otContext *aContext, otOperationalDataset *aDataset)
{
    ThreadError error = kThreadError_None;

    VerifyOrExit(aDataset != NULL, error = kThreadError_InvalidArgs);

    aContext->mThreadNetif.GetPendingDataset().Get(*aDataset);

exit:
    return error;
}

ThreadError otSetPendingDataset(otContext *aContext, otOperationalDataset *aDataset)
{
    ThreadError error;

    VerifyOrExit(aDataset != NULL, error = kThreadError_InvalidArgs);

    error = aContext->mThreadNetif.GetPendingDataset().Set(*aDataset);

exit:
    return error;
}

#ifdef __cplusplus
}  // extern "C"
#endif

}  // namespace Thread
