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

#include "openthread-core-config.h"

#if OPENTHREAD_FTD || OPENTHREAD_MTD

#include "instance/instance.hpp"

using namespace ot;

uint32_t otThreadGetChildTimeout(otInstance *aInstance)
{
    return AsCoreType(aInstance).Get<Mle::MleRouter>().GetTimeout();
}

void otThreadSetChildTimeout(otInstance *aInstance, uint32_t aTimeout)
{
    AsCoreType(aInstance).Get<Mle::MleRouter>().SetTimeout(aTimeout);
}

const otExtendedPanId *otThreadGetExtendedPanId(otInstance *aInstance)
{
    return &AsCoreType(aInstance).Get<MeshCoP::ExtendedPanIdManager>().GetExtPanId();
}

otError otThreadSetExtendedPanId(otInstance *aInstance, const otExtendedPanId *aExtendedPanId)
{
    Error                         error    = kErrorNone;
    Instance                     &instance = AsCoreType(aInstance);
    const MeshCoP::ExtendedPanId &extPanId = AsCoreType(aExtendedPanId);

    VerifyOrExit(instance.Get<Mle::MleRouter>().IsDisabled(), error = kErrorInvalidState);

    instance.Get<MeshCoP::ExtendedPanIdManager>().SetExtPanId(extPanId);

    instance.Get<MeshCoP::ActiveDatasetManager>().Clear();
    instance.Get<MeshCoP::PendingDatasetManager>().Clear();

exit:
    return error;
}

otError otThreadGetLeaderRloc(otInstance *aInstance, otIp6Address *aLeaderRloc)
{
    Error error = kErrorNone;

    VerifyOrExit(!AsCoreType(aInstance).Get<Mle::Mle>().HasRloc16(Mle::kInvalidRloc16), error = kErrorDetached);
    AsCoreType(aInstance).Get<Mle::Mle>().GetLeaderRloc(AsCoreType(aLeaderRloc));

exit:
    return error;
}

otLinkModeConfig otThreadGetLinkMode(otInstance *aInstance)
{
    otLinkModeConfig config;

    AsCoreType(aInstance).Get<Mle::MleRouter>().GetDeviceMode().Get(config);

    return config;
}

otError otThreadSetLinkMode(otInstance *aInstance, otLinkModeConfig aConfig)
{
    return AsCoreType(aInstance).Get<Mle::MleRouter>().SetDeviceMode(Mle::DeviceMode(aConfig));
}

void otThreadGetNetworkKey(otInstance *aInstance, otNetworkKey *aNetworkKey)
{
    AsCoreType(aInstance).Get<KeyManager>().GetNetworkKey(AsCoreType(aNetworkKey));
}

#if OPENTHREAD_CONFIG_PLATFORM_KEY_REFERENCES_ENABLE
otNetworkKeyRef otThreadGetNetworkKeyRef(otInstance *aInstance)
{
    return AsCoreType(aInstance).Get<KeyManager>().GetNetworkKeyRef();
}
#endif

otError otThreadSetNetworkKey(otInstance *aInstance, const otNetworkKey *aKey)
{
    Error     error    = kErrorNone;
    Instance &instance = AsCoreType(aInstance);

    VerifyOrExit(instance.Get<Mle::MleRouter>().IsDisabled(), error = kErrorInvalidState);

    instance.Get<KeyManager>().SetNetworkKey(AsCoreType(aKey));

    instance.Get<MeshCoP::ActiveDatasetManager>().Clear();
    instance.Get<MeshCoP::PendingDatasetManager>().Clear();

exit:
    return error;
}

#if OPENTHREAD_CONFIG_PLATFORM_KEY_REFERENCES_ENABLE
otError otThreadSetNetworkKeyRef(otInstance *aInstance, otNetworkKeyRef aKeyRef)
{
    Error     error    = kErrorNone;
    Instance &instance = AsCoreType(aInstance);

    VerifyOrExit(aKeyRef != 0, error = kErrorInvalidArgs);

    VerifyOrExit(instance.Get<Mle::MleRouter>().IsDisabled(), error = kErrorInvalidState);

    instance.Get<KeyManager>().SetNetworkKeyRef((aKeyRef));
    instance.Get<MeshCoP::ActiveDatasetManager>().Clear();
    instance.Get<MeshCoP::PendingDatasetManager>().Clear();

exit:
    return error;
}
#endif

const otIp6Address *otThreadGetRloc(otInstance *aInstance)
{
    return &AsCoreType(aInstance).Get<Mle::MleRouter>().GetMeshLocalRloc();
}

const otIp6Address *otThreadGetMeshLocalEid(otInstance *aInstance)
{
    return &AsCoreType(aInstance).Get<Mle::MleRouter>().GetMeshLocalEid();
}

const otMeshLocalPrefix *otThreadGetMeshLocalPrefix(otInstance *aInstance)
{
    return &AsCoreType(aInstance).Get<Mle::MleRouter>().GetMeshLocalPrefix();
}

otError otThreadSetMeshLocalPrefix(otInstance *aInstance, const otMeshLocalPrefix *aMeshLocalPrefix)
{
    Error error = kErrorNone;

    VerifyOrExit(AsCoreType(aInstance).Get<Mle::MleRouter>().IsDisabled(), error = kErrorInvalidState);

    AsCoreType(aInstance).Get<Mle::MleRouter>().SetMeshLocalPrefix(AsCoreType(aMeshLocalPrefix));
    AsCoreType(aInstance).Get<MeshCoP::ActiveDatasetManager>().Clear();
    AsCoreType(aInstance).Get<MeshCoP::PendingDatasetManager>().Clear();

exit:
    return error;
}

const otIp6Address *otThreadGetLinkLocalIp6Address(otInstance *aInstance)
{
    return &AsCoreType(aInstance).Get<Mle::MleRouter>().GetLinkLocalAddress();
}

const otIp6Address *otThreadGetLinkLocalAllThreadNodesMulticastAddress(otInstance *aInstance)
{
    return &AsCoreType(aInstance).Get<Mle::MleRouter>().GetLinkLocalAllThreadNodesAddress();
}

const otIp6Address *otThreadGetRealmLocalAllThreadNodesMulticastAddress(otInstance *aInstance)
{
    return &AsCoreType(aInstance).Get<Mle::MleRouter>().GetRealmLocalAllThreadNodesAddress();
}

otError otThreadGetServiceAloc(otInstance *aInstance, uint8_t aServiceId, otIp6Address *aServiceAloc)
{
    Error error = kErrorNone;

    VerifyOrExit(!AsCoreType(aInstance).Get<Mle::Mle>().HasRloc16(Mle::kInvalidRloc16), error = kErrorDetached);
    AsCoreType(aInstance).Get<Mle::Mle>().GetServiceAloc(aServiceId, AsCoreType(aServiceAloc));

exit:
    return error;
}

const char *otThreadGetNetworkName(otInstance *aInstance)
{
    return AsCoreType(aInstance).Get<MeshCoP::NetworkNameManager>().GetNetworkName().GetAsCString();
}

otError otThreadSetNetworkName(otInstance *aInstance, const char *aNetworkName)
{
    Error error = kErrorNone;

    VerifyOrExit(AsCoreType(aInstance).Get<Mle::MleRouter>().IsDisabled(), error = kErrorInvalidState);

#if !OPENTHREAD_CONFIG_ALLOW_EMPTY_NETWORK_NAME
    // Thread interfaces support a zero length name internally for backwards compatibility, but new names
    // must be at least one valid character long.
    VerifyOrExit(nullptr != aNetworkName && aNetworkName[0] != '\0', error = kErrorInvalidArgs);
#endif

    error = AsCoreType(aInstance).Get<MeshCoP::NetworkNameManager>().SetNetworkName(aNetworkName);
    AsCoreType(aInstance).Get<MeshCoP::ActiveDatasetManager>().Clear();
    AsCoreType(aInstance).Get<MeshCoP::PendingDatasetManager>().Clear();

exit:
    return error;
}

#if (OPENTHREAD_CONFIG_THREAD_VERSION >= OT_THREAD_VERSION_1_2)
const char *otThreadGetDomainName(otInstance *aInstance)
{
    return AsCoreType(aInstance).Get<MeshCoP::NetworkNameManager>().GetDomainName().GetAsCString();
}

otError otThreadSetDomainName(otInstance *aInstance, const char *aDomainName)
{
    Error error = kErrorNone;

    VerifyOrExit(AsCoreType(aInstance).Get<Mle::MleRouter>().IsDisabled(), error = kErrorInvalidState);

    error = AsCoreType(aInstance).Get<MeshCoP::NetworkNameManager>().SetDomainName(aDomainName);

exit:
    return error;
}

#if OPENTHREAD_CONFIG_DUA_ENABLE
otError otThreadSetFixedDuaInterfaceIdentifier(otInstance *aInstance, const otIp6InterfaceIdentifier *aIid)
{
    Error error = kErrorNone;

    if (aIid)
    {
        error = AsCoreType(aInstance).Get<DuaManager>().SetFixedDuaInterfaceIdentifier(AsCoreType(aIid));
    }
    else
    {
        AsCoreType(aInstance).Get<DuaManager>().ClearFixedDuaInterfaceIdentifier();
    }

    return error;
}

const otIp6InterfaceIdentifier *otThreadGetFixedDuaInterfaceIdentifier(otInstance *aInstance)
{
    Instance                       &instance = AsCoreType(aInstance);
    const otIp6InterfaceIdentifier *iid      = nullptr;

    if (instance.Get<DuaManager>().IsFixedDuaInterfaceIdentifierSet())
    {
        iid = &instance.Get<DuaManager>().GetFixedDuaInterfaceIdentifier();
    }

    return iid;
}
#endif // OPENTHREAD_CONFIG_DUA_ENABLE

#endif // (OPENTHREAD_CONFIG_THREAD_VERSION >= OT_THREAD_VERSION_1_2)

uint32_t otThreadGetKeySequenceCounter(otInstance *aInstance)
{
    return AsCoreType(aInstance).Get<KeyManager>().GetCurrentKeySequence();
}

void otThreadSetKeySequenceCounter(otInstance *aInstance, uint32_t aKeySequenceCounter)
{
    AsCoreType(aInstance).Get<KeyManager>().SetCurrentKeySequence(
        aKeySequenceCounter, KeyManager::kForceUpdate | KeyManager::kGuardTimerUnchanged);
}

uint16_t otThreadGetKeySwitchGuardTime(otInstance *aInstance)
{
    return AsCoreType(aInstance).Get<KeyManager>().GetKeySwitchGuardTime();
}

void otThreadSetKeySwitchGuardTime(otInstance *aInstance, uint16_t aKeySwitchGuardTime)
{
    AsCoreType(aInstance).Get<KeyManager>().SetKeySwitchGuardTime(aKeySwitchGuardTime);
}

otError otThreadBecomeDetached(otInstance *aInstance)
{
    return AsCoreType(aInstance).Get<Mle::MleRouter>().BecomeDetached();
}

otError otThreadBecomeChild(otInstance *aInstance) { return AsCoreType(aInstance).Get<Mle::MleRouter>().BecomeChild(); }

otError otThreadGetNextNeighborInfo(otInstance *aInstance, otNeighborInfoIterator *aIterator, otNeighborInfo *aInfo)
{
    AssertPointerIsNotNull(aIterator);

    return AsCoreType(aInstance).Get<NeighborTable>().GetNextNeighborInfo(*aIterator, AsCoreType(aInfo));
}

otDeviceRole otThreadGetDeviceRole(otInstance *aInstance)
{
    return MapEnum(AsCoreType(aInstance).Get<Mle::MleRouter>().GetRole());
}

const char *otThreadDeviceRoleToString(otDeviceRole aRole) { return Mle::RoleToString(MapEnum(aRole)); }

otError otThreadGetLeaderData(otInstance *aInstance, otLeaderData *aLeaderData)
{
    Error error = kErrorNone;

    AssertPointerIsNotNull(aLeaderData);

    VerifyOrExit(AsCoreType(aInstance).Get<Mle::MleRouter>().IsAttached(), error = kErrorDetached);
    *aLeaderData = AsCoreType(aInstance).Get<Mle::MleRouter>().GetLeaderData();

exit:
    return error;
}

uint8_t otThreadGetLeaderRouterId(otInstance *aInstance)
{
    return AsCoreType(aInstance).Get<Mle::MleRouter>().GetLeaderId();
}

uint8_t otThreadGetLeaderWeight(otInstance *aInstance)
{
    return AsCoreType(aInstance).Get<Mle::MleRouter>().GetLeaderData().GetWeighting();
}

uint32_t otThreadGetPartitionId(otInstance *aInstance)
{
    return AsCoreType(aInstance).Get<Mle::MleRouter>().GetLeaderData().GetPartitionId();
}

uint16_t otThreadGetRloc16(otInstance *aInstance) { return AsCoreType(aInstance).Get<Mle::MleRouter>().GetRloc16(); }

otError otThreadGetParentInfo(otInstance *aInstance, otRouterInfo *aParentInfo)
{
    return AsCoreType(aInstance).Get<Mle::Mle>().GetParentInfo(AsCoreType(aParentInfo));
}

otError otThreadGetParentAverageRssi(otInstance *aInstance, int8_t *aParentRssi)
{
    Error error = kErrorNone;

    AssertPointerIsNotNull(aParentRssi);

    *aParentRssi = AsCoreType(aInstance).Get<Mle::MleRouter>().GetParent().GetLinkInfo().GetAverageRss();

    VerifyOrExit(*aParentRssi != Radio::kInvalidRssi, error = kErrorFailed);

exit:
    return error;
}

otError otThreadGetParentLastRssi(otInstance *aInstance, int8_t *aLastRssi)
{
    Error error = kErrorNone;

    AssertPointerIsNotNull(aLastRssi);

    *aLastRssi = AsCoreType(aInstance).Get<Mle::MleRouter>().GetParent().GetLinkInfo().GetLastRss();

    VerifyOrExit(*aLastRssi != Radio::kInvalidRssi, error = kErrorFailed);

exit:
    return error;
}

otError otThreadSearchForBetterParent(otInstance *aInstance)
{
    return AsCoreType(aInstance).Get<Mle::Mle>().SearchForBetterParent();
}

otError otThreadSetEnabled(otInstance *aInstance, bool aEnabled)
{
    Error error = kErrorNone;

    if (aEnabled)
    {
        error = AsCoreType(aInstance).Get<Mle::MleRouter>().Start();
    }
    else
    {
        AsCoreType(aInstance).Get<Mle::MleRouter>().Stop();
    }

    return error;
}

uint16_t otThreadGetVersion(void) { return kThreadVersion; }

bool otThreadIsSingleton(otInstance *aInstance)
{
    bool isSingleton = false;

#if OPENTHREAD_FTD
    isSingleton = AsCoreType(aInstance).Get<Mle::MleRouter>().IsSingleton();
#else
    OT_UNUSED_VARIABLE(aInstance);
#endif

    return isSingleton;
}

otError otThreadDiscover(otInstance              *aInstance,
                         uint32_t                 aScanChannels,
                         uint16_t                 aPanId,
                         bool                     aJoiner,
                         bool                     aEnableEui64Filtering,
                         otHandleActiveScanResult aCallback,
                         void                    *aCallbackContext)
{
    return AsCoreType(aInstance).Get<Mle::DiscoverScanner>().Discover(
        Mac::ChannelMask(aScanChannels), aPanId, aJoiner, aEnableEui64Filtering,
        /* aFilterIndexes (use hash of factory EUI64) */ nullptr, aCallback, aCallbackContext);
}

otError otThreadSetJoinerAdvertisement(otInstance    *aInstance,
                                       uint32_t       aOui,
                                       const uint8_t *aAdvData,
                                       uint8_t        aAdvDataLength)
{
    return AsCoreType(aInstance).Get<Mle::DiscoverScanner>().SetJoinerAdvertisement(aOui, aAdvData, aAdvDataLength);
}

bool otThreadIsDiscoverInProgress(otInstance *aInstance)
{
    return AsCoreType(aInstance).Get<Mle::DiscoverScanner>().IsInProgress();
}

const otIpCounters *otThreadGetIp6Counters(otInstance *aInstance)
{
    return &AsCoreType(aInstance).Get<MeshForwarder>().GetCounters();
}

void otThreadResetIp6Counters(otInstance *aInstance) { AsCoreType(aInstance).Get<MeshForwarder>().ResetCounters(); }

#if OPENTHREAD_CONFIG_TX_QUEUE_STATISTICS_ENABLE
const uint32_t *otThreadGetTimeInQueueHistogram(otInstance *aInstance, uint16_t *aNumBins, uint32_t *aBinInterval)
{
    AssertPointerIsNotNull(aNumBins);
    AssertPointerIsNotNull(aBinInterval);

    return AsCoreType(aInstance).Get<MeshForwarder>().GetTimeInQueueHistogram(*aNumBins, *aBinInterval);
}

uint32_t otThreadGetMaxTimeInQueue(otInstance *aInstance)
{
    return AsCoreType(aInstance).Get<MeshForwarder>().GetMaxTimeInQueue();
}

void otThreadResetTimeInQueueStat(otInstance *aInstance)
{
    return AsCoreType(aInstance).Get<MeshForwarder>().ResetTimeInQueueStat();
}
#endif

const otMleCounters *otThreadGetMleCounters(otInstance *aInstance)
{
    return &AsCoreType(aInstance).Get<Mle::MleRouter>().GetCounters();
}

void otThreadResetMleCounters(otInstance *aInstance) { AsCoreType(aInstance).Get<Mle::MleRouter>().ResetCounters(); }

#if OPENTHREAD_CONFIG_MLE_PARENT_RESPONSE_CALLBACK_API_ENABLE
void otThreadRegisterParentResponseCallback(otInstance                    *aInstance,
                                            otThreadParentResponseCallback aCallback,
                                            void                          *aContext)
{
    AsCoreType(aInstance).Get<Mle::MleRouter>().RegisterParentResponseStatsCallback(aCallback, aContext);
}
#endif

#if OPENTHREAD_CONFIG_TMF_ANYCAST_LOCATOR_ENABLE
otError otThreadLocateAnycastDestination(otInstance                    *aInstance,
                                         const otIp6Address            *aAnycastAddress,
                                         otThreadAnycastLocatorCallback aCallback,
                                         void                          *aContext)
{
    return AsCoreType(aInstance).Get<AnycastLocator>().Locate(AsCoreType(aAnycastAddress), aCallback, aContext);
}

bool otThreadIsAnycastLocateInProgress(otInstance *aInstance)
{
    return AsCoreType(aInstance).Get<AnycastLocator>().IsInProgress();
}
#endif

otError otThreadDetachGracefully(otInstance *aInstance, otDetachGracefullyCallback aCallback, void *aContext)
{
    return AsCoreType(aInstance).Get<Mle::MleRouter>().DetachGracefully(aCallback, aContext);
}

#if OPENTHREAD_CONFIG_DYNAMIC_STORE_FRAME_AHEAD_COUNTER_ENABLE
void otThreadSetStoreFrameCounterAhead(otInstance *aInstance, uint32_t aStoreFrameCounterAhead)
{
    return AsCoreType(aInstance).Get<Mle::Mle>().SetStoreFrameCounterAhead(aStoreFrameCounterAhead);
}

uint32_t otThreadGetStoreFrameCounterAhead(otInstance *aInstance)
{
    return AsCoreType(aInstance).Get<Mle::Mle>().GetStoreFrameCounterAhead();
}
#endif

#endif // OPENTHREAD_FTD || OPENTHREAD_MTD

#if OPENTHREAD_CONFIG_UPTIME_ENABLE
void otConvertDurationInSecondsToString(uint32_t aDuration, char *aBuffer, uint16_t aSize)
{
    StringWriter writer(aBuffer, aSize);

    Uptime::UptimeToString(Uptime::SecToMsec(aDuration), writer, /* aIncludeMsec */ false);
}
#endif
