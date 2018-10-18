/*
 *    Copyright (c) 2016-2017, The OpenThread Authors.
 *    All rights reserved.
 *
 *    Redistribution and use in source and binary forms, with or without
 *    modification, are permitted provided that the following conditions are met:
 *    1. Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *    2. Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *    3. Neither the name of the copyright holder nor the
 *       names of its contributors may be used to endorse or promote products
 *       derived from this software without specific prior written permission.
 *
 *    THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 *    ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 *    WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 *    DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY
 *    DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 *    (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 *    LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 *    ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 *    (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 *    SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/**
 * @file
 *   This file implements full thread device specified Spinel interface to the OpenThread stack.
 */

#include "ncp_base.hpp"
#include <openthread/config.h>

#if OPENTHREAD_ENABLE_CHANNEL_MANAGER
#include <openthread/channel_manager.h>
#endif
#if OPENTHREAD_ENABLE_CHILD_SUPERVISION
#include <openthread/child_supervision.h>
#endif
#include <openthread/dataset_ftd.h>
#include <openthread/diag.h>
#include <openthread/icmp6.h>
#include <openthread/ncp.h>
#include <openthread/thread_ftd.h>
#include <openthread/platform/misc.h>

#include "common/code_utils.hpp"
#include "common/debug.hpp"
#include "common/instance.hpp"
#if OPENTHREAD_ENABLE_COMMISSIONER
#include "meshcop/commissioner.hpp"
#endif

#if OPENTHREAD_FTD
namespace ot {
namespace Ncp {

otError NcpBase::EncodeChildInfo(const otChildInfo &aChildInfo)
{
    otError error = OT_ERROR_NONE;
    uint8_t modeFlags;

    modeFlags = LinkFlagsToFlagByte(aChildInfo.mRxOnWhenIdle, aChildInfo.mSecureDataRequest,
                                    aChildInfo.mFullThreadDevice, aChildInfo.mFullNetworkData);

    SuccessOrExit(error = mEncoder.WriteEui64(aChildInfo.mExtAddress));
    SuccessOrExit(error = mEncoder.WriteUint16(aChildInfo.mRloc16));
    SuccessOrExit(error = mEncoder.WriteUint32(aChildInfo.mTimeout));
    SuccessOrExit(error = mEncoder.WriteUint32(aChildInfo.mAge));
    SuccessOrExit(error = mEncoder.WriteUint8(aChildInfo.mNetworkDataVersion));
    SuccessOrExit(error = mEncoder.WriteUint8(aChildInfo.mLinkQualityIn));
    SuccessOrExit(error = mEncoder.WriteInt8(aChildInfo.mAverageRssi));
    SuccessOrExit(error = mEncoder.WriteUint8(modeFlags));
    SuccessOrExit(error = mEncoder.WriteInt8(aChildInfo.mLastRssi));

exit:
    return error;
}

// ----------------------------------------------------------------------------
// MARK: Property/Status Changed
// ----------------------------------------------------------------------------

void NcpBase::HandleParentResponseInfo(otThreadParentResponseInfo *aInfo, void *aContext)
{
    VerifyOrExit(aInfo && aContext);

    static_cast<NcpBase *>(aContext)->HandleParentResponseInfo(aInfo);

exit:
    return;
}

void NcpBase::HandleParentResponseInfo(const otThreadParentResponseInfo *aInfo)
{
    VerifyOrExit(aInfo);

    SuccessOrExit(mEncoder.BeginFrame(SPINEL_HEADER_FLAG | SPINEL_HEADER_IID_0, SPINEL_CMD_PROP_VALUE_IS,
                                      SPINEL_PROP_PARENT_RESPONSE_INFO));

    SuccessOrExit(mEncoder.WriteEui64(aInfo->mExtAddr));
    SuccessOrExit(mEncoder.WriteUint16(aInfo->mRloc16));
    SuccessOrExit(mEncoder.WriteInt8(aInfo->mRssi));
    SuccessOrExit(mEncoder.WriteInt8(aInfo->mPriority));
    SuccessOrExit(mEncoder.WriteUint8(aInfo->mLinkQuality3));
    SuccessOrExit(mEncoder.WriteUint8(aInfo->mLinkQuality2));
    SuccessOrExit(mEncoder.WriteUint8(aInfo->mLinkQuality1));
    SuccessOrExit(mEncoder.WriteBool(aInfo->mIsAttached));

    SuccessOrExit(mEncoder.EndFrame());

exit:
    return;
}

void NcpBase::HandleChildTableChanged(otThreadChildTableEvent aEvent, const otChildInfo *aChildInfo)
{
    GetNcpInstance()->HandleChildTableChanged(aEvent, *aChildInfo);
}

void NcpBase::HandleChildTableChanged(otThreadChildTableEvent aEvent, const otChildInfo &aChildInfo)
{
    otError      error   = OT_ERROR_NONE;
    uint8_t      header  = SPINEL_HEADER_FLAG | SPINEL_HEADER_IID_0;
    unsigned int command = 0;

    VerifyOrExit(!mChangedPropsSet.IsPropertyFiltered(SPINEL_PROP_THREAD_CHILD_TABLE));

    VerifyOrExit(!aChildInfo.mIsStateRestoring);

    switch (aEvent)
    {
    case OT_THREAD_CHILD_TABLE_EVENT_CHILD_ADDED:
        command = SPINEL_CMD_PROP_VALUE_INSERTED;
        break;

    case OT_THREAD_CHILD_TABLE_EVENT_CHILD_REMOVED:
        command = SPINEL_CMD_PROP_VALUE_REMOVED;
        break;

    default:
        ExitNow();
    }

    SuccessOrExit(error = mEncoder.BeginFrame(header, command, SPINEL_PROP_THREAD_CHILD_TABLE));
    SuccessOrExit(error = EncodeChildInfo(aChildInfo));
    SuccessOrExit(error = mEncoder.EndFrame());

exit:

    // If the frame can not be added (out of NCP buffer space), we remember
    // to send an async `LAST_STATUS(NOMEM)` when buffer space becomes
    // available. Also `mShouldEmitChildTableUpdate` flag is set to `true` so
    // that the entire child table is later emitted as `VALUE_IS` spinel frame
    // update from `ProcessThreadChangedFlags()`.

    if (error != OT_ERROR_NONE)
    {
        mShouldEmitChildTableUpdate = true;

        mChangedPropsSet.AddLastStatus(SPINEL_STATUS_NOMEM);
        mUpdateChangedPropsTask.Post();
    }
}

// ----------------------------------------------------------------------------
// MARK: Individual Property Handlers
// ----------------------------------------------------------------------------

template <> otError NcpBase::HandlePropertyGet<SPINEL_PROP_THREAD_LOCAL_LEADER_WEIGHT>(void)
{
    return mEncoder.WriteUint8(otThreadGetLocalLeaderWeight(mInstance));
}

template <> otError NcpBase::HandlePropertyGet<SPINEL_PROP_THREAD_LEADER_WEIGHT>(void)
{
    return mEncoder.WriteUint8(otThreadGetLeaderWeight(mInstance));
}

template <> otError NcpBase::HandlePropertyGet<SPINEL_PROP_THREAD_CHILD_TABLE>(void)
{
    otError     error = OT_ERROR_NONE;
    otChildInfo childInfo;
    uint8_t     maxChildren;

    maxChildren = otThreadGetMaxAllowedChildren(mInstance);

    for (uint8_t index = 0; index < maxChildren; index++)
    {
        if ((otThreadGetChildInfoByIndex(mInstance, index, &childInfo) != OT_ERROR_NONE) || childInfo.mIsStateRestoring)
        {
            continue;
        }

        SuccessOrExit(error = mEncoder.OpenStruct());
        SuccessOrExit(error = EncodeChildInfo(childInfo));
        SuccessOrExit(error = mEncoder.CloseStruct());
    }

exit:
    return error;
}

template <> otError NcpBase::HandlePropertyGet<SPINEL_PROP_THREAD_ROUTER_TABLE>(void)
{
    otError      error = OT_ERROR_NONE;
    otRouterInfo routerInfo;
    uint8_t      maxRouterId;

    maxRouterId = otThreadGetMaxRouterId(mInstance);

    for (uint8_t routerId = 0; routerId <= maxRouterId; routerId++)
    {
        if ((otThreadGetRouterInfo(mInstance, routerId, &routerInfo) != OT_ERROR_NONE) || !routerInfo.mAllocated)
        {
            continue;
        }

        SuccessOrExit(error = mEncoder.OpenStruct());

        SuccessOrExit(error = mEncoder.WriteEui64(routerInfo.mExtAddress));
        SuccessOrExit(error = mEncoder.WriteUint16(routerInfo.mRloc16));
        SuccessOrExit(error = mEncoder.WriteUint8(routerInfo.mRouterId));
        SuccessOrExit(error = mEncoder.WriteUint8(routerInfo.mNextHop));
        SuccessOrExit(error = mEncoder.WriteUint8(routerInfo.mPathCost));
        SuccessOrExit(error = mEncoder.WriteUint8(routerInfo.mLinkQualityIn));
        SuccessOrExit(error = mEncoder.WriteUint8(routerInfo.mLinkQualityOut));
        SuccessOrExit(error = mEncoder.WriteUint8(routerInfo.mAge));
        SuccessOrExit(error = mEncoder.WriteBool(routerInfo.mLinkEstablished));

        SuccessOrExit(error = mEncoder.CloseStruct());
    }

exit:
    return error;
}

template <> otError NcpBase::HandlePropertyGet<SPINEL_PROP_THREAD_CHILD_TABLE_ADDRESSES>(void)
{
    otError                   error = OT_ERROR_NONE;
    otChildInfo               childInfo;
    uint8_t                   maxChildren;
    otIp6Address              ip6Address;
    otChildIp6AddressIterator iterator = OT_CHILD_IP6_ADDRESS_ITERATOR_INIT;

    maxChildren = otThreadGetMaxAllowedChildren(mInstance);

    for (uint8_t childIndex = 0; childIndex < maxChildren; childIndex++)
    {
        if ((otThreadGetChildInfoByIndex(mInstance, childIndex, &childInfo) != OT_ERROR_NONE) ||
            childInfo.mIsStateRestoring)
        {
            continue;
        }

        SuccessOrExit(error = mEncoder.OpenStruct());

        SuccessOrExit(error = mEncoder.WriteEui64(childInfo.mExtAddress));
        SuccessOrExit(error = mEncoder.WriteUint16(childInfo.mRloc16));

        iterator = OT_CHILD_IP6_ADDRESS_ITERATOR_INIT;

        while (otThreadGetChildNextIp6Address(mInstance, childIndex, &iterator, &ip6Address) == OT_ERROR_NONE)
        {
            SuccessOrExit(error = mEncoder.WriteIp6Address(ip6Address));
        }

        SuccessOrExit(error = mEncoder.CloseStruct());
    }

exit:
    return error;
}

template <> otError NcpBase::HandlePropertyGet<SPINEL_PROP_THREAD_ROUTER_ROLE_ENABLED>(void)
{
    return mEncoder.WriteBool(otThreadIsRouterRoleEnabled(mInstance));
}

template <> otError NcpBase::HandlePropertySet<SPINEL_PROP_THREAD_ROUTER_ROLE_ENABLED>(void)
{
    bool    enabled;
    otError error = OT_ERROR_NONE;

    SuccessOrExit(error = mDecoder.ReadBool(enabled));

    otThreadSetRouterRoleEnabled(mInstance, enabled);

exit:
    return error;
}

template <> otError NcpBase::HandlePropertyGet<SPINEL_PROP_NET_PSKC>(void)
{
    return mEncoder.WriteData(otThreadGetPSKc(mInstance), sizeof(spinel_net_pskc_t));
}

template <> otError NcpBase::HandlePropertySet<SPINEL_PROP_NET_PSKC>(void)
{
    const uint8_t *ptr = NULL;
    uint16_t       len;
    otError        error = OT_ERROR_NONE;

    SuccessOrExit(error = mDecoder.ReadData(ptr, len));

    VerifyOrExit(len == sizeof(spinel_net_pskc_t), error = OT_ERROR_PARSE);

    error = otThreadSetPSKc(mInstance, ptr);

exit:
    return error;
}

template <> otError NcpBase::HandlePropertyGet<SPINEL_PROP_THREAD_CHILD_COUNT_MAX>(void)
{
    return mEncoder.WriteUint8(otThreadGetMaxAllowedChildren(mInstance));
}

template <> otError NcpBase::HandlePropertySet<SPINEL_PROP_THREAD_CHILD_COUNT_MAX>(void)
{
    uint8_t maxChildren = 0;
    otError error       = OT_ERROR_NONE;

    SuccessOrExit(error = mDecoder.ReadUint8(maxChildren));

    error = otThreadSetMaxAllowedChildren(mInstance, maxChildren);

exit:
    return error;
}

template <> otError NcpBase::HandlePropertyGet<SPINEL_PROP_THREAD_ROUTER_UPGRADE_THRESHOLD>(void)
{
    return mEncoder.WriteUint8(otThreadGetRouterUpgradeThreshold(mInstance));
}

template <> otError NcpBase::HandlePropertySet<SPINEL_PROP_THREAD_ROUTER_UPGRADE_THRESHOLD>(void)
{
    uint8_t threshold = 0;
    otError error     = OT_ERROR_NONE;

    SuccessOrExit(error = mDecoder.ReadUint8(threshold));

    otThreadSetRouterUpgradeThreshold(mInstance, threshold);

exit:
    return error;
}

template <> otError NcpBase::HandlePropertyGet<SPINEL_PROP_THREAD_ROUTER_DOWNGRADE_THRESHOLD>(void)
{
    return mEncoder.WriteUint8(otThreadGetRouterDowngradeThreshold(mInstance));
}

template <> otError NcpBase::HandlePropertySet<SPINEL_PROP_THREAD_ROUTER_DOWNGRADE_THRESHOLD>(void)
{
    uint8_t threshold = 0;
    otError error     = OT_ERROR_NONE;

    SuccessOrExit(error = mDecoder.ReadUint8(threshold));

    otThreadSetRouterDowngradeThreshold(mInstance, threshold);

exit:
    return error;
}

template <> otError NcpBase::HandlePropertyGet<SPINEL_PROP_THREAD_ROUTER_SELECTION_JITTER>(void)
{
    return mEncoder.WriteUint8(otThreadGetRouterSelectionJitter(mInstance));
}

template <> otError NcpBase::HandlePropertySet<SPINEL_PROP_THREAD_ROUTER_SELECTION_JITTER>(void)
{
    uint8_t jitter = 0;
    otError error  = OT_ERROR_NONE;

    SuccessOrExit(error = mDecoder.ReadUint8(jitter));

    otThreadSetRouterSelectionJitter(mInstance, jitter);

exit:
    return error;
}

template <> otError NcpBase::HandlePropertyGet<SPINEL_PROP_THREAD_CONTEXT_REUSE_DELAY>(void)
{
    return mEncoder.WriteUint32(otThreadGetContextIdReuseDelay(mInstance));
}

template <> otError NcpBase::HandlePropertySet<SPINEL_PROP_THREAD_CONTEXT_REUSE_DELAY>(void)
{
    uint32_t delay = 0;
    otError  error = OT_ERROR_NONE;

    SuccessOrExit(error = mDecoder.ReadUint32(delay));

    otThreadSetContextIdReuseDelay(mInstance, delay);

exit:
    return error;
}

template <> otError NcpBase::HandlePropertyGet<SPINEL_PROP_THREAD_NETWORK_ID_TIMEOUT>(void)
{
    return mEncoder.WriteUint8(otThreadGetNetworkIdTimeout(mInstance));
}

template <> otError NcpBase::HandlePropertySet<SPINEL_PROP_THREAD_NETWORK_ID_TIMEOUT>(void)
{
    uint8_t timeout = 0;
    otError error   = OT_ERROR_NONE;

    SuccessOrExit(error = mDecoder.ReadUint8(timeout));

    otThreadSetNetworkIdTimeout(mInstance, timeout);

exit:
    return error;
}

#if OPENTHREAD_ENABLE_COMMISSIONER

template <> otError NcpBase::HandlePropertyGet<SPINEL_PROP_MESHCOP_COMMISSIONER_STATE>(void)
{
    uint8_t state = SPINEL_MESHCOP_COMMISSIONER_STATE_DISABLED;

    switch (otCommissionerGetState(mInstance))
    {
    case OT_COMMISSIONER_STATE_DISABLED:
        state = SPINEL_MESHCOP_COMMISSIONER_STATE_DISABLED;
        break;

    case OT_COMMISSIONER_STATE_PETITION:
        state = SPINEL_MESHCOP_COMMISSIONER_STATE_PETITION;
        break;

    case OT_COMMISSIONER_STATE_ACTIVE:
        state = SPINEL_MESHCOP_COMMISSIONER_STATE_ACTIVE;
        break;
    }

    return mEncoder.WriteUint8(state);
}

template <> otError NcpBase::HandlePropertySet<SPINEL_PROP_MESHCOP_COMMISSIONER_STATE>(void)
{
    uint8_t state;
    otError error = OT_ERROR_NONE;

    SuccessOrExit(error = mDecoder.ReadUint8(state));

    switch (state)
    {
    case SPINEL_MESHCOP_COMMISSIONER_STATE_DISABLED:
        error = otCommissionerStop(mInstance);
        break;

    case SPINEL_MESHCOP_COMMISSIONER_STATE_ACTIVE:
        error = otCommissionerStart(mInstance);
        break;

    default:
        error = OT_ERROR_INVALID_ARGS;
        break;
    }

exit:
    return error;
}

template <> otError NcpBase::HandlePropertyInsert<SPINEL_PROP_MESHCOP_COMMISSIONER_JOINERS>(void)
{
    otError             error = OT_ERROR_NONE;
    const otExtAddress *eui64;
    uint32_t            timeout;
    const char *        psk;

    SuccessOrExit(error = mDecoder.OpenStruct());

    if (!mDecoder.IsAllReadInStruct())
    {
        SuccessOrExit(error = mDecoder.ReadEui64(eui64));
    }
    else
    {
        // Empty struct indicates any Joiner (no EUI64 is given so NULL is used.).
        eui64 = NULL;
    }

    SuccessOrExit(error = mDecoder.CloseStruct());

    SuccessOrExit(error = mDecoder.ReadUint32(timeout));
    SuccessOrExit(error = mDecoder.ReadUtf8(psk));

    error = otCommissionerAddJoiner(mInstance, eui64, psk, timeout);

exit:
    return error;
}

template <> otError NcpBase::HandlePropertyRemove<SPINEL_PROP_MESHCOP_COMMISSIONER_JOINERS>(void)
{
    otError             error = OT_ERROR_NONE;
    const otExtAddress *eui64;

    SuccessOrExit(error = mDecoder.OpenStruct());

    if (!mDecoder.IsAllReadInStruct())
    {
        SuccessOrExit(error = mDecoder.ReadEui64(eui64));
    }
    else
    {
        // Empty struct indicates any Joiner (no EUI64 is given so NULL is used.).
        eui64 = NULL;
    }

    SuccessOrExit(error = mDecoder.CloseStruct());

    error = otCommissionerRemoveJoiner(mInstance, eui64);

exit:
    return error;
}

template <> otError NcpBase::HandlePropertyGet<SPINEL_PROP_MESHCOP_COMMISSIONER_PROVISIONING_URL>(void)
{
    otError     error  = OT_ERROR_NONE;
    uint16_t    length = 0;
    const char *url    = otCommissionerGetProvisioningUrl(mInstance, &length);

    if (url != NULL && length > 0)
    {
        SuccessOrExit(error = mEncoder.WriteData(reinterpret_cast<const uint8_t *>(url), length));
    }

    SuccessOrExit(error = mEncoder.WriteUint8(0));

exit:
    return error;
}

template <> otError NcpBase::HandlePropertySet<SPINEL_PROP_MESHCOP_COMMISSIONER_PROVISIONING_URL>(void)
{
    otError     error = OT_ERROR_NONE;
    const char *url;

    SuccessOrExit(error = mDecoder.ReadUtf8(url));

    error = otCommissionerSetProvisioningUrl(mInstance, url);

exit:
    return error;
}

template <> otError NcpBase::HandlePropertyGet<SPINEL_PROP_MESHCOP_COMMISSIONER_SESSION_ID>(void)
{
    return mEncoder.WriteUint16(otCommissionerGetSessionId(mInstance));
}

template <> otError NcpBase::HandlePropertySet<SPINEL_PROP_MESHCOP_COMMISSIONER_ANNOUNCE_BEGIN>(void)
{
    otError             error = OT_ERROR_NONE;
    uint32_t            channelMask;
    uint8_t             count;
    uint16_t            period;
    const otIp6Address *address;

    SuccessOrExit(error = mDecoder.ReadUint32(channelMask));
    SuccessOrExit(error = mDecoder.ReadUint8(count));
    SuccessOrExit(error = mDecoder.ReadUint16(period));
    SuccessOrExit(error = mDecoder.ReadIp6Address(address));

    error = otCommissionerAnnounceBegin(mInstance, channelMask, count, period, address);

exit:
    return error;
}

template <> otError NcpBase::HandlePropertySet<SPINEL_PROP_MESHCOP_COMMISSIONER_ENERGY_SCAN>(void)
{
    otError             error = OT_ERROR_NONE;
    uint32_t            channelMask;
    uint8_t             count;
    uint16_t            period;
    uint16_t            scanDuration;
    const otIp6Address *address;

    SuccessOrExit(error = mDecoder.ReadUint32(channelMask));
    SuccessOrExit(error = mDecoder.ReadUint8(count));
    SuccessOrExit(error = mDecoder.ReadUint16(period));
    SuccessOrExit(error = mDecoder.ReadUint16(scanDuration));
    SuccessOrExit(error = mDecoder.ReadIp6Address(address));

    error = otCommissionerEnergyScan(mInstance, channelMask, count, period, scanDuration, address,
                                     &NcpBase::HandleCommissionerEnergyReport_Jump, this);

exit:
    return error;
}

void NcpBase::HandleCommissionerEnergyReport_Jump(uint32_t       aChannelMask,
                                                  const uint8_t *aEnergyData,
                                                  uint8_t        aLength,
                                                  void *         aContext)
{
    static_cast<NcpBase *>(aContext)->HandleCommissionerEnergyReport(aChannelMask, aEnergyData, aLength);
}

void NcpBase::HandleCommissionerEnergyReport(uint32_t aChannelMask, const uint8_t *aEnergyData, uint8_t aLength)
{
    otError error = OT_ERROR_NONE;

    SuccessOrExit(error = mEncoder.BeginFrame(SPINEL_HEADER_FLAG | SPINEL_HEADER_IID_0, SPINEL_CMD_PROP_VALUE_INSERTED,
                                              SPINEL_PROP_MESHCOP_COMMISSIONER_ENERGY_SCAN_RESULT));
    SuccessOrExit(error = mEncoder.WriteUint32(aChannelMask));
    SuccessOrExit(error = mEncoder.WriteDataWithLen(aEnergyData, aLength));
    SuccessOrExit(error = mEncoder.EndFrame());

exit:

    if (error != OT_ERROR_NONE)
    {
        mChangedPropsSet.AddLastStatus(SPINEL_STATUS_NOMEM);
        mUpdateChangedPropsTask.Post();
    }
}

template <> otError NcpBase::HandlePropertySet<SPINEL_PROP_MESHCOP_COMMISSIONER_PAN_ID_QUERY>(void)
{
    otError             error = OT_ERROR_NONE;
    uint16_t            panId;
    uint32_t            channelMask;
    const otIp6Address *address;

    SuccessOrExit(error = mDecoder.ReadUint16(panId));
    SuccessOrExit(error = mDecoder.ReadUint32(channelMask));
    SuccessOrExit(error = mDecoder.ReadIp6Address(address));

    error = otCommissionerPanIdQuery(mInstance, panId, channelMask, address,
                                     &NcpBase::HandleCommissionerPanIdConflict_Jump, this);

exit:
    return error;
}

void NcpBase::HandleCommissionerPanIdConflict_Jump(uint16_t aPanId, uint32_t aChannelMask, void *aContext)
{
    static_cast<NcpBase *>(aContext)->HandleCommissionerPanIdConflict(aPanId, aChannelMask);
}

void NcpBase::HandleCommissionerPanIdConflict(uint16_t aPanId, uint32_t aChannelMask)
{
    otError error = OT_ERROR_NONE;

    SuccessOrExit(error = mEncoder.BeginFrame(SPINEL_HEADER_FLAG | SPINEL_HEADER_IID_0, SPINEL_CMD_PROP_VALUE_INSERTED,
                                              SPINEL_PROP_MESHCOP_COMMISSIONER_PAN_ID_CONFLICT_RESULT));

    SuccessOrExit(error = mEncoder.WriteUint16(aPanId));
    SuccessOrExit(error = mEncoder.WriteUint32(aChannelMask));
    SuccessOrExit(error = mEncoder.EndFrame());

exit:

    if (error != OT_ERROR_NONE)
    {
        mChangedPropsSet.AddLastStatus(SPINEL_STATUS_NOMEM);
        mUpdateChangedPropsTask.Post();
    }
}

template <> otError NcpBase::HandlePropertySet<SPINEL_PROP_MESHCOP_COMMISSIONER_MGMT_GET>(void)
{
    otError        error = OT_ERROR_NONE;
    const uint8_t *tlvs;
    uint16_t       length;

    SuccessOrExit(error = mDecoder.ReadDataWithLen(tlvs, length));
    VerifyOrExit(length <= 255, error = OT_ERROR_INVALID_ARGS);

    error = otCommissionerSendMgmtGet(mInstance, tlvs, static_cast<uint8_t>(length));

exit:
    return error;
}

template <> otError NcpBase::HandlePropertySet<SPINEL_PROP_MESHCOP_COMMISSIONER_MGMT_SET>(void)
{
    otError                error = OT_ERROR_NONE;
    const uint8_t *        tlvs;
    uint16_t               length;
    otCommissioningDataset dataset;

    SuccessOrExit(error = mDecoder.ReadDataWithLen(tlvs, length));
    VerifyOrExit(length <= 255, error = OT_ERROR_INVALID_ARGS);

    memset(&dataset, 0, sizeof(otCommissioningDataset));
    error = otCommissionerSendMgmtSet(mInstance, &dataset, tlvs, static_cast<uint8_t>(length));

exit:
    return error;
}

// SPINEL_PROP_THREAD_COMMISSIONER_ENABLED is replaced by SPINEL_PROP_MESHCOP_COMMISSIONER_STATE. Please use the new
// property. The old property/implementation remains for backward compatibility.

template <> otError NcpBase::HandlePropertyGet<SPINEL_PROP_THREAD_COMMISSIONER_ENABLED>(void)
{
    return mEncoder.WriteBool(otCommissionerGetState(mInstance) == OT_COMMISSIONER_STATE_ACTIVE);
}

otError NcpBase::HandlePropertySet_SPINEL_PROP_THREAD_COMMISSIONER_ENABLED(uint8_t aHeader)
{
    bool    enabled = false;
    otError error   = OT_ERROR_NONE;

    SuccessOrExit(error = mDecoder.ReadBool(enabled));

    if (enabled == false)
    {
        error = otCommissionerStop(mInstance);
    }
    else
    {
        error = otCommissionerStart(mInstance);
    }

exit:
    return PrepareLastStatusResponse(aHeader, ThreadErrorToSpinelStatus(error));
}

// SPINEL_PROP_THREAD_JOINERS is replaced by SPINEL_PROP_MESHCOP_COMMISSIONER_JOINERS. Please us the new property.
// The old property/implementation remains for backward compatibility.

template <> otError NcpBase::HandlePropertyInsert<SPINEL_PROP_THREAD_JOINERS>(void)
{
    otError             error         = OT_ERROR_NONE;
    const otExtAddress *eui64         = NULL;
    const char *        aPSKd         = NULL;
    uint32_t            joinerTimeout = 0;

    SuccessOrExit(error = mDecoder.ReadUtf8(aPSKd));
    SuccessOrExit(error = mDecoder.ReadUint32(joinerTimeout));

    if (mDecoder.ReadEui64(eui64) != OT_ERROR_NONE)
    {
        eui64 = NULL;
    }

    error = otCommissionerAddJoiner(mInstance, eui64, aPSKd, joinerTimeout);

exit:
    return error;
}
#endif // OPENTHREAD_ENABLE_COMMISSIONER

template <> otError NcpBase::HandlePropertySet<SPINEL_PROP_THREAD_LOCAL_LEADER_WEIGHT>(void)
{
    uint8_t weight;
    otError error = OT_ERROR_NONE;

    SuccessOrExit(error = mDecoder.ReadUint8(weight));

    otThreadSetLocalLeaderWeight(mInstance, weight);

exit:
    return error;
}

#if OPENTHREAD_CONFIG_ENABLE_STEERING_DATA_SET_OOB

template <> otError NcpBase::HandlePropertyGet<SPINEL_PROP_THREAD_STEERING_DATA>(void)
{
    return mEncoder.WriteEui64(mSteeringDataAddress);
}

template <> otError NcpBase::HandlePropertySet<SPINEL_PROP_THREAD_STEERING_DATA>(void)
{
    otError error = OT_ERROR_NONE;

    SuccessOrExit(error = mDecoder.ReadEui64(mSteeringDataAddress));

    SuccessOrExit(error = otThreadSetSteeringData(mInstance, &mSteeringDataAddress));

exit:
    return error;
}
#endif // #if OPENTHREAD_CONFIG_ENABLE_STEERING_DATA_SET_OOB

template <> otError NcpBase::HandlePropertyGet<SPINEL_PROP_THREAD_PREFERRED_ROUTER_ID>(void)
{
    return mEncoder.WriteUint8(mPreferredRouteId);
}

template <> otError NcpBase::HandlePropertySet<SPINEL_PROP_THREAD_PREFERRED_ROUTER_ID>(void)
{
    otError error = OT_ERROR_NONE;

    SuccessOrExit(error = mDecoder.ReadUint8(mPreferredRouteId));

    SuccessOrExit(error = otThreadSetPreferredRouterId(mInstance, mPreferredRouteId));

exit:
    return error;
}

template <> otError NcpBase::HandlePropertyRemove<SPINEL_PROP_THREAD_ACTIVE_ROUTER_IDS>(void)
{
    otError error = OT_ERROR_NONE;
    uint8_t routerId;

    SuccessOrExit(error = mDecoder.ReadUint8(routerId));

    error = otThreadReleaseRouterId(mInstance, routerId);

    // `INVALID_STATE` is returned when router ID was not allocated (i.e. not in the list)
    // in such a case, the "remove" operation can be considered successful.

    if (error == OT_ERROR_INVALID_STATE)
    {
        error = OT_ERROR_NONE;
    }

exit:
    return error;
}

template <> otError NcpBase::HandlePropertyGet<SPINEL_PROP_THREAD_ADDRESS_CACHE_TABLE>(void)
{
    otError         error = OT_ERROR_NONE;
    otEidCacheEntry entry;

    for (uint8_t index = 0;; index++)
    {
        SuccessOrExit(otThreadGetEidCacheEntry(mInstance, index, &entry));

        if (!entry.mValid)
        {
            continue;
        }

        SuccessOrExit(error = mEncoder.OpenStruct());
        SuccessOrExit(error = mEncoder.WriteIp6Address(entry.mTarget));
        SuccessOrExit(error = mEncoder.WriteUint16(entry.mRloc16));
        SuccessOrExit(error = mEncoder.WriteUint8(entry.mAge));
        SuccessOrExit(error = mEncoder.CloseStruct());
    }

exit:
    return error;
}

otError NcpBase::DecodeOperationalDataset(otOperationalDataset &aDataset,
                                          const uint8_t **      aTlvs,
                                          uint8_t *             aTlvsLength,
                                          const otIp6Address ** aDestIpAddress,
                                          bool                  aAllowEmptyValue)
{
    otError error = OT_ERROR_NONE;

    memset(&aDataset, 0, sizeof(otOperationalDataset));

    if (aTlvs != NULL)
    {
        *aTlvs = NULL;
    }

    if (aTlvsLength != NULL)
    {
        *aTlvsLength = 0;
    }

    if (aDestIpAddress != NULL)
    {
        *aDestIpAddress = NULL;
    }

    while (!mDecoder.IsAllReadInStruct())
    {
        unsigned int propKey;

        SuccessOrExit(error = mDecoder.OpenStruct());
        SuccessOrExit(error = mDecoder.ReadUintPacked(propKey));

        switch (static_cast<spinel_prop_key_t>(propKey))
        {
        case SPINEL_PROP_DATASET_ACTIVE_TIMESTAMP:

            if (!aAllowEmptyValue || !mDecoder.IsAllReadInStruct())
            {
                SuccessOrExit(error = mDecoder.ReadUint64(aDataset.mActiveTimestamp));
            }

            aDataset.mComponents.mIsActiveTimestampPresent = true;
            break;

        case SPINEL_PROP_DATASET_PENDING_TIMESTAMP:

            if (!aAllowEmptyValue || !mDecoder.IsAllReadInStruct())
            {
                SuccessOrExit(error = mDecoder.ReadUint64(aDataset.mPendingTimestamp));
            }

            aDataset.mComponents.mIsPendingTimestampPresent = true;
            break;

        case SPINEL_PROP_NET_MASTER_KEY:

            if (!aAllowEmptyValue || !mDecoder.IsAllReadInStruct())
            {
                const uint8_t *key;
                uint16_t       len;

                SuccessOrExit(error = mDecoder.ReadData(key, len));
                VerifyOrExit(len == OT_MASTER_KEY_SIZE, error = OT_ERROR_INVALID_ARGS);
                memcpy(aDataset.mMasterKey.m8, key, len);
            }

            aDataset.mComponents.mIsMasterKeyPresent = true;
            break;

        case SPINEL_PROP_NET_NETWORK_NAME:

            if (!aAllowEmptyValue || !mDecoder.IsAllReadInStruct())
            {
                const char *name;
                size_t      len;

                SuccessOrExit(error = mDecoder.ReadUtf8(name));
                len = strlen(name);
                VerifyOrExit(len <= OT_NETWORK_NAME_MAX_SIZE, error = OT_ERROR_INVALID_ARGS);
                memcpy(aDataset.mNetworkName.m8, name, len + 1);
            }

            aDataset.mComponents.mIsNetworkNamePresent = true;
            break;

        case SPINEL_PROP_NET_XPANID:

            if (!aAllowEmptyValue || !mDecoder.IsAllReadInStruct())
            {
                const uint8_t *xpanid;
                uint16_t       len;

                SuccessOrExit(error = mDecoder.ReadData(xpanid, len));
                VerifyOrExit(len == OT_EXT_PAN_ID_SIZE, error = OT_ERROR_INVALID_ARGS);
                memcpy(aDataset.mExtendedPanId.m8, xpanid, len);
            }

            aDataset.mComponents.mIsExtendedPanIdPresent = true;
            break;

        case SPINEL_PROP_IPV6_ML_PREFIX:

            if (!aAllowEmptyValue || !mDecoder.IsAllReadInStruct())
            {
                const otIp6Address *addr;
                uint8_t             prefixLen;

                SuccessOrExit(error = mDecoder.ReadIp6Address(addr));
                SuccessOrExit(error = mDecoder.ReadUint8(prefixLen));
                VerifyOrExit(prefixLen == 64, error = OT_ERROR_INVALID_ARGS);
                memcpy(aDataset.mMeshLocalPrefix.m8, addr, OT_MESH_LOCAL_PREFIX_SIZE);
            }

            aDataset.mComponents.mIsMeshLocalPrefixPresent = true;
            break;

        case SPINEL_PROP_DATASET_DELAY_TIMER:

            if (!aAllowEmptyValue || !mDecoder.IsAllReadInStruct())
            {
                SuccessOrExit(error = mDecoder.ReadUint32(aDataset.mDelay));
            }

            aDataset.mComponents.mIsDelayPresent = true;
            break;

        case SPINEL_PROP_MAC_15_4_PANID:

            if (!aAllowEmptyValue || !mDecoder.IsAllReadInStruct())
            {
                SuccessOrExit(error = mDecoder.ReadUint16(aDataset.mPanId));
            }

            aDataset.mComponents.mIsPanIdPresent = true;
            break;

        case SPINEL_PROP_PHY_CHAN:

            if (!aAllowEmptyValue || !mDecoder.IsAllReadInStruct())
            {
                uint8_t channel;

                SuccessOrExit(error = mDecoder.ReadUint8(channel));
                aDataset.mChannel = channel;
            }

            aDataset.mComponents.mIsChannelPresent = true;
            break;

        case SPINEL_PROP_NET_PSKC:

            if (!aAllowEmptyValue || !mDecoder.IsAllReadInStruct())
            {
                const uint8_t *psk;
                uint16_t       len;

                SuccessOrExit(error = mDecoder.ReadData(psk, len));
                VerifyOrExit(len == OT_PSKC_MAX_SIZE, error = OT_ERROR_INVALID_ARGS);
                memcpy(aDataset.mPSKc.m8, psk, OT_PSKC_MAX_SIZE);
            }

            aDataset.mComponents.mIsPSKcPresent = true;
            break;

        case SPINEL_PROP_DATASET_SECURITY_POLICY:

            if (!aAllowEmptyValue || !mDecoder.IsAllReadInStruct())
            {
                SuccessOrExit(error = mDecoder.ReadUint16(aDataset.mSecurityPolicy.mRotationTime));
                SuccessOrExit(error = mDecoder.ReadUint8(aDataset.mSecurityPolicy.mFlags));
            }

            aDataset.mComponents.mIsSecurityPolicyPresent = true;
            break;

        case SPINEL_PROP_PHY_CHAN_SUPPORTED:

            if (!aAllowEmptyValue || !mDecoder.IsAllReadInStruct())
            {
                uint8_t channel;

                aDataset.mChannelMaskPage0 = 0;

                while (!mDecoder.IsAllReadInStruct())
                {
                    SuccessOrExit(error = mDecoder.ReadUint8(channel));
                    VerifyOrExit(channel <= 31, error = OT_ERROR_INVALID_ARGS);
                    aDataset.mChannelMaskPage0 |= (1U << channel);
                }
            }

            aDataset.mComponents.mIsChannelMaskPage0Present = true;
            break;

        case SPINEL_PROP_DATASET_RAW_TLVS:

            if (!aAllowEmptyValue || !mDecoder.IsAllReadInStruct())
            {
                const uint8_t *tlvs;
                uint16_t       len;

                SuccessOrExit(error = mDecoder.ReadData(tlvs, len));
                VerifyOrExit(len <= 255, error = OT_ERROR_INVALID_ARGS);

                if (aTlvs != NULL)
                {
                    *aTlvs = tlvs;
                }

                if (aTlvsLength != NULL)
                {
                    *aTlvsLength = static_cast<uint8_t>(len);
                }
            }

            break;

        case SPINEL_PROP_DATASET_DEST_ADDRESS:

            if (!aAllowEmptyValue || !mDecoder.IsAllReadInStruct())
            {
                const otIp6Address *addr;

                SuccessOrExit(error = mDecoder.ReadIp6Address(addr));

                if (aDestIpAddress != NULL)
                {
                    *aDestIpAddress = addr;
                }
            }

            break;

        default:
            break;
        }

        SuccessOrExit(error = mDecoder.CloseStruct());
    }

exit:
    return error;
}

template <> otError NcpBase::HandlePropertySet<SPINEL_PROP_THREAD_ACTIVE_DATASET>(void)
{
    otError              error = OT_ERROR_NONE;
    otOperationalDataset dataset;

    SuccessOrExit(error = DecodeOperationalDataset(dataset));
    error = otDatasetSetActive(mInstance, &dataset);

exit:
    return error;
}

template <> otError NcpBase::HandlePropertySet<SPINEL_PROP_THREAD_PENDING_DATASET>(void)
{
    otError              error = OT_ERROR_NONE;
    otOperationalDataset dataset;

    SuccessOrExit(error = DecodeOperationalDataset(dataset));
    error = otDatasetSetPending(mInstance, &dataset);

exit:
    return error;
}

template <> otError NcpBase::HandlePropertySet<SPINEL_PROP_THREAD_MGMT_SET_ACTIVE_DATASET>(void)
{
    otError              error = OT_ERROR_NONE;
    otOperationalDataset dataset;
    const uint8_t *      extraTlvs;
    uint8_t              extraTlvsLength;

    SuccessOrExit(error = DecodeOperationalDataset(dataset, &extraTlvs, &extraTlvsLength));
    error = otDatasetSendMgmtActiveSet(mInstance, &dataset, extraTlvs, extraTlvsLength);

exit:
    return error;
}

template <> otError NcpBase::HandlePropertySet<SPINEL_PROP_THREAD_MGMT_SET_PENDING_DATASET>(void)
{
    otError              error = OT_ERROR_NONE;
    otOperationalDataset dataset;
    const uint8_t *      extraTlvs;
    uint8_t              extraTlvsLength;

    SuccessOrExit(error = DecodeOperationalDataset(dataset, &extraTlvs, &extraTlvsLength));
    error = otDatasetSendMgmtPendingSet(mInstance, &dataset, extraTlvs, extraTlvsLength);

exit:
    return error;
}

template <> otError NcpBase::HandlePropertySet<SPINEL_PROP_THREAD_MGMT_GET_ACTIVE_DATASET>(void)
{
    otError              error = OT_ERROR_NONE;
    otOperationalDataset dataset;
    const uint8_t *      extraTlvs;
    uint8_t              extraTlvsLength;
    const otIp6Address * destIpAddress;

    SuccessOrExit(error = DecodeOperationalDataset(dataset, &extraTlvs, &extraTlvsLength, &destIpAddress, true));
    error = otDatasetSendMgmtActiveGet(mInstance, &dataset.mComponents, extraTlvs, extraTlvsLength, destIpAddress);

exit:
    return error;
}

template <> otError NcpBase::HandlePropertySet<SPINEL_PROP_THREAD_MGMT_GET_PENDING_DATASET>(void)
{
    otError              error = OT_ERROR_NONE;
    otOperationalDataset dataset;
    const uint8_t *      extraTlvs;
    uint8_t              extraTlvsLength;
    const otIp6Address * destIpAddress;

    SuccessOrExit(error = DecodeOperationalDataset(dataset, &extraTlvs, &extraTlvsLength, &destIpAddress, true));
    error = otDatasetSendMgmtPendingGet(mInstance, &dataset.mComponents, extraTlvs, extraTlvsLength, destIpAddress);

exit:
    return error;
}

#if OPENTHREAD_ENABLE_CHILD_SUPERVISION

template <> otError NcpBase::HandlePropertyGet<SPINEL_PROP_CHILD_SUPERVISION_INTERVAL>(void)
{
    return mEncoder.WriteUint16(otChildSupervisionGetInterval(mInstance));
}

template <> otError NcpBase::HandlePropertySet<SPINEL_PROP_CHILD_SUPERVISION_INTERVAL>(void)
{
    otError  error = OT_ERROR_NONE;
    uint16_t interval;

    SuccessOrExit(error = mDecoder.ReadUint16(interval));
    otChildSupervisionSetInterval(mInstance, interval);

exit:
    return error;
}

#endif // OPENTHREAD_ENABLE_CHILD_SUPERVISION

#if OPENTHREAD_ENABLE_CHANNEL_MANAGER

template <> otError NcpBase::HandlePropertyGet<SPINEL_PROP_CHANNEL_MANAGER_NEW_CHANNEL>(void)
{
    return mEncoder.WriteUint8(otChannelManagerGetRequestedChannel(mInstance));
}

template <> otError NcpBase::HandlePropertySet<SPINEL_PROP_CHANNEL_MANAGER_NEW_CHANNEL>(void)
{
    uint8_t channel;
    otError error = OT_ERROR_NONE;

    SuccessOrExit(error = mDecoder.ReadUint8(channel));

    otChannelManagerRequestChannelChange(mInstance, channel);

exit:
    return error;
}

template <> otError NcpBase::HandlePropertyGet<SPINEL_PROP_CHANNEL_MANAGER_DELAY>(void)
{
    return mEncoder.WriteUint16(otChannelManagerGetDelay(mInstance));
}

template <> otError NcpBase::HandlePropertySet<SPINEL_PROP_CHANNEL_MANAGER_DELAY>(void)
{
    uint16_t delay;
    otError  error = OT_ERROR_NONE;

    SuccessOrExit(error = mDecoder.ReadUint16(delay));

    error = otChannelManagerSetDelay(mInstance, delay);

exit:
    return error;
}

template <> otError NcpBase::HandlePropertyGet<SPINEL_PROP_CHANNEL_MANAGER_SUPPORTED_CHANNELS>(void)
{
    return EncodeChannelMask(otChannelManagerGetSupportedChannels(mInstance));
}

template <> otError NcpBase::HandlePropertySet<SPINEL_PROP_CHANNEL_MANAGER_SUPPORTED_CHANNELS>(void)
{
    uint32_t channelMask = 0;
    otError  error       = OT_ERROR_NONE;

    SuccessOrExit(error = DecodeChannelMask(channelMask));
    otChannelManagerSetSupportedChannels(mInstance, channelMask);

exit:
    return error;
}

template <> otError NcpBase::HandlePropertyGet<SPINEL_PROP_CHANNEL_MANAGER_FAVORED_CHANNELS>(void)
{
    return EncodeChannelMask(otChannelManagerGetFavoredChannels(mInstance));
}

template <> otError NcpBase::HandlePropertySet<SPINEL_PROP_CHANNEL_MANAGER_FAVORED_CHANNELS>(void)
{
    uint32_t channelMask = 0;
    otError  error       = OT_ERROR_NONE;

    SuccessOrExit(error = DecodeChannelMask(channelMask));
    otChannelManagerSetFavoredChannels(mInstance, channelMask);

exit:
    return error;
}

template <> otError NcpBase::HandlePropertyGet<SPINEL_PROP_CHANNEL_MANAGER_CHANNEL_SELECT>(void)
{
    return mEncoder.WriteBool(false);
}

template <> otError NcpBase::HandlePropertySet<SPINEL_PROP_CHANNEL_MANAGER_CHANNEL_SELECT>(void)
{
    bool    skipQualityCheck = false;
    otError error            = OT_ERROR_NONE;

    SuccessOrExit(error = mDecoder.ReadBool(skipQualityCheck));
    error = otChannelManagerRequestChannelSelect(mInstance, skipQualityCheck);

exit:
    return error;
}

template <> otError NcpBase::HandlePropertyGet<SPINEL_PROP_CHANNEL_MANAGER_AUTO_SELECT_ENABLED>(void)
{
    return mEncoder.WriteBool(otChannelManagerGetAutoChannelSelectionEnabled(mInstance));
}

template <> otError NcpBase::HandlePropertySet<SPINEL_PROP_CHANNEL_MANAGER_AUTO_SELECT_ENABLED>(void)
{
    bool    enabled = false;
    otError error   = OT_ERROR_NONE;

    SuccessOrExit(error = mDecoder.ReadBool(enabled));
    otChannelManagerSetAutoChannelSelectionEnabled(mInstance, enabled);

exit:
    return error;
}

template <> otError NcpBase::HandlePropertyGet<SPINEL_PROP_CHANNEL_MANAGER_AUTO_SELECT_INTERVAL>(void)
{
    return mEncoder.WriteUint32(otChannelManagerGetAutoChannelSelectionInterval(mInstance));
}

template <> otError NcpBase::HandlePropertySet<SPINEL_PROP_CHANNEL_MANAGER_AUTO_SELECT_INTERVAL>(void)
{
    uint32_t interval;
    otError  error = OT_ERROR_NONE;

    SuccessOrExit(error = mDecoder.ReadUint32(interval));
    error = otChannelManagerSetAutoChannelSelectionInterval(mInstance, interval);

exit:
    return error;
}

#endif // OPENTHREAD_ENABLE_CHANNEL_MANAGER

#if OPENTHREAD_CONFIG_ENABLE_TIME_SYNC
template <> otError NcpBase::HandlePropertyGet<SPINEL_PROP_TIME_SYNC_PERIOD>(void)
{
    return mEncoder.WriteUint16(otNetworkTimeGetSyncPeriod(mInstance));
}

template <> otError NcpBase::HandlePropertySet<SPINEL_PROP_TIME_SYNC_PERIOD>(void)
{
    otError  error = OT_ERROR_NONE;
    uint16_t timeSyncPeriod;

    SuccessOrExit(error = mDecoder.ReadUint16(timeSyncPeriod));

    SuccessOrExit(error = otNetworkTimeSetSyncPeriod(mInstance, timeSyncPeriod));

exit:
    return error;
}

template <> otError NcpBase::HandlePropertyGet<SPINEL_PROP_TIME_SYNC_XTAL_THRESHOLD>(void)
{
    return mEncoder.WriteUint16(otNetworkTimeGetXtalThreshold(mInstance));
}

template <> otError NcpBase::HandlePropertySet<SPINEL_PROP_TIME_SYNC_XTAL_THRESHOLD>(void)
{
    otError  error = OT_ERROR_NONE;
    uint16_t xtalThreshold;

    SuccessOrExit(error = mDecoder.ReadUint16(xtalThreshold));

    SuccessOrExit(error = otNetworkTimeSetXtalThreshold(mInstance, xtalThreshold));

exit:
    return error;
}
#endif // OPENTHREAD_CONFIG_ENABLE_TIME_SYNC

} // namespace Ncp
} // namespace ot

#endif // OPENTHREAD_FTD
