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

#include <openthread/dataset_ftd.h>
#include <openthread/diag.h>
#include <openthread/icmp6.h>
#include <openthread/ncp.h>
#include <openthread/openthread.h>
#include <openthread/platform/misc.h>
#include <openthread/platform/radio.h>
#include <openthread/thread_ftd.h>

#if OPENTHREAD_ENABLE_TMF_PROXY
#include <openthread/tmf_proxy.h>
#endif

#include "common/code_utils.hpp"
#include "common/debug.hpp"
#include "common/instance.hpp"
#if OPENTHREAD_ENABLE_COMMISSIONER
#include "meshcop/commissioner.hpp"
#endif
#include "net/ip6.hpp"

#if OPENTHREAD_FTD
namespace ot {
namespace Ncp {

otError NcpBase::EncodeChildInfo(const otChildInfo &aChildInfo)
{
    otError error = OT_ERROR_NONE;
    uint8_t modeFlags;

    modeFlags = LinkFlagsToFlagByte(
                        aChildInfo.mRxOnWhenIdle,
                        aChildInfo.mSecureDataRequest,
                        aChildInfo.mFullFunction,
                        aChildInfo.mFullNetworkData
                    );

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

void NcpBase::HandleChildTableChanged(otThreadChildTableEvent aEvent, const otChildInfo *aChildInfo)
{
    GetNcpInstance()->HandleChildTableChanged(aEvent, *aChildInfo);
}

void NcpBase::HandleChildTableChanged(otThreadChildTableEvent aEvent, const otChildInfo &aChildInfo)
{
    otError error = OT_ERROR_NONE;
    uint8_t header = SPINEL_HEADER_FLAG | SPINEL_HEADER_IID_0;
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

otError NcpBase::GetPropertyHandler_THREAD_LOCAL_LEADER_WEIGHT(void)
{
    return mEncoder.WriteUint8(otThreadGetLocalLeaderWeight(mInstance));
}

otError NcpBase::GetPropertyHandler_THREAD_LEADER_WEIGHT(void)
{
    return mEncoder.WriteUint8(otThreadGetLeaderWeight(mInstance));
}

otError NcpBase::GetPropertyHandler_THREAD_CHILD_TABLE(void)
{
    otError error = OT_ERROR_NONE;
    otChildInfo childInfo;
    uint8_t maxChildren;

    maxChildren = otThreadGetMaxAllowedChildren(mInstance);

    for (uint8_t index = 0; index < maxChildren; index++)
    {
        if ((otThreadGetChildInfoByIndex(mInstance, index, &childInfo) != OT_ERROR_NONE) ||
            childInfo.mIsStateRestoring)
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

otError NcpBase::GetPropertyHandler_THREAD_ROUTER_TABLE(void)
{
    otError error = OT_ERROR_NONE;
    otRouterInfo routerInfo;
    uint8_t maxRouterId;

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

otError NcpBase::GetPropertyHandler_THREAD_CHILD_TABLE_ADDRESSES(void)
{
    otError error = OT_ERROR_NONE;
    otChildInfo childInfo;
    uint8_t maxChildren;
    otIp6Address ip6Address;
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

otError NcpBase::GetPropertyHandler_THREAD_ROUTER_ROLE_ENABLED(void)
{
    return mEncoder.WriteBool(otThreadIsRouterRoleEnabled(mInstance));
}

otError NcpBase::SetPropertyHandler_THREAD_ROUTER_ROLE_ENABLED(void)
{
    bool enabled;
    otError error = OT_ERROR_NONE;

    SuccessOrExit(error = mDecoder.ReadBool(enabled));

    otThreadSetRouterRoleEnabled(mInstance, enabled);

exit:
    return error;
}

otError NcpBase::GetPropertyHandler_NET_PSKC(void)
{
    return mEncoder.WriteData(otThreadGetPSKc(mInstance), sizeof(spinel_net_pskc_t));
}

otError NcpBase::SetPropertyHandler_NET_PSKC(void)
{
    const uint8_t *ptr = NULL;
    uint16_t len;
    otError error = OT_ERROR_NONE;

    SuccessOrExit(error = mDecoder.ReadData(ptr, len));

    VerifyOrExit(len == sizeof(spinel_net_pskc_t), error = OT_ERROR_PARSE);

    error = otThreadSetPSKc(mInstance, ptr);

exit:
    return error;
}

otError NcpBase::GetPropertyHandler_THREAD_CHILD_COUNT_MAX(void)
{
    return mEncoder.WriteUint8(otThreadGetMaxAllowedChildren(mInstance));
}

otError NcpBase::SetPropertyHandler_THREAD_CHILD_COUNT_MAX(void)
{
    uint8_t maxChildren = 0;
    otError error = OT_ERROR_NONE;

    SuccessOrExit(error = mDecoder.ReadUint8(maxChildren));

    error = otThreadSetMaxAllowedChildren(mInstance, maxChildren);

exit:
    return error;
}

otError NcpBase::GetPropertyHandler_THREAD_ROUTER_UPGRADE_THRESHOLD(void)
{
    return mEncoder.WriteUint8(otThreadGetRouterUpgradeThreshold(mInstance));
}

otError NcpBase::SetPropertyHandler_THREAD_ROUTER_UPGRADE_THRESHOLD(void)
{
    uint8_t threshold = 0;
    otError error = OT_ERROR_NONE;

    SuccessOrExit(error = mDecoder.ReadUint8(threshold));

    otThreadSetRouterUpgradeThreshold(mInstance, threshold);

exit:
    return error;
}

otError NcpBase::GetPropertyHandler_THREAD_ROUTER_DOWNGRADE_THRESHOLD(void)
{
    return mEncoder.WriteUint8(otThreadGetRouterDowngradeThreshold(mInstance));
}

otError NcpBase::SetPropertyHandler_THREAD_ROUTER_DOWNGRADE_THRESHOLD(void)
{
    uint8_t threshold = 0;
    otError error = OT_ERROR_NONE;

    SuccessOrExit(error = mDecoder.ReadUint8(threshold));

    otThreadSetRouterDowngradeThreshold(mInstance, threshold);

exit:
    return error;
}

otError NcpBase::GetPropertyHandler_THREAD_ROUTER_SELECTION_JITTER(void)
{
    return mEncoder.WriteUint8(otThreadGetRouterSelectionJitter(mInstance));
}

otError NcpBase::SetPropertyHandler_THREAD_ROUTER_SELECTION_JITTER(void)
{
    uint8_t jitter = 0;
    otError error = OT_ERROR_NONE;

    SuccessOrExit(error = mDecoder.ReadUint8(jitter));

    otThreadSetRouterSelectionJitter(mInstance, jitter);

exit:
    return error;
}

otError NcpBase::GetPropertyHandler_THREAD_CONTEXT_REUSE_DELAY(void)
{
    return mEncoder.WriteUint32(otThreadGetContextIdReuseDelay(mInstance));
}

otError NcpBase::SetPropertyHandler_THREAD_CONTEXT_REUSE_DELAY(void)
{
    uint32_t delay = 0;
    otError error = OT_ERROR_NONE;

    SuccessOrExit(error = mDecoder.ReadUint32(delay));

    otThreadSetContextIdReuseDelay(mInstance, delay);

 exit:
    return error;
}

otError NcpBase::GetPropertyHandler_THREAD_NETWORK_ID_TIMEOUT(void)
{
    return mEncoder.WriteUint8(otThreadGetNetworkIdTimeout(mInstance));
}

otError NcpBase::SetPropertyHandler_THREAD_NETWORK_ID_TIMEOUT(void)
{
    uint8_t timeout = 0;
    otError error = OT_ERROR_NONE;

    SuccessOrExit(error = mDecoder.ReadUint8(timeout));

    otThreadSetNetworkIdTimeout(mInstance, timeout);

 exit:
    return error;
}

#if OPENTHREAD_ENABLE_COMMISSIONER
otError NcpBase::GetPropertyHandler_THREAD_COMMISSIONER_ENABLED(void)
{
    return mEncoder.WriteBool(otCommissionerGetState(mInstance) == OT_COMMISSIONER_STATE_ACTIVE);
}

otError NcpBase::SetPropertyHandler_THREAD_COMMISSIONER_ENABLED(uint8_t aHeader)
{
    bool enabled = false;
    otError error = OT_ERROR_NONE;

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

otError NcpBase::InsertPropertyHandler_THREAD_JOINERS(void)
{
    otError error = OT_ERROR_NONE;
    const otExtAddress *eui64 = NULL;
    const char *aPSKd = NULL;
    uint32_t joinerTimeout = 0;

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

otError NcpBase::SetPropertyHandler_THREAD_LOCAL_LEADER_WEIGHT(void)
{
    uint8_t weight;
    otError error = OT_ERROR_NONE;

    SuccessOrExit(error = mDecoder.ReadUint8(weight));

    otThreadSetLocalLeaderWeight(mInstance, weight);

exit:
    return error;
}

#if OPENTHREAD_CONFIG_ENABLE_STEERING_DATA_SET_OOB

otError NcpBase::GetPropertyHandler_THREAD_STEERING_DATA(void)
{
    return mEncoder.WriteEui64(mSteeringDataAddress);
}

otError NcpBase::SetPropertyHandler_THREAD_STEERING_DATA(void)
{
    otError error = OT_ERROR_NONE;

    SuccessOrExit(error = mDecoder.ReadEui64(mSteeringDataAddress));

    SuccessOrExit(error = otThreadSetSteeringData(mInstance, &mSteeringDataAddress));

exit:
    return error;
}
#endif // #if OPENTHREAD_CONFIG_ENABLE_STEERING_DATA_SET_OOB

otError NcpBase::SetPropertyHandler_THREAD_CHILD_TIMEOUT(void)
{
    uint32_t timeout = 0;
    otError error = OT_ERROR_NONE;

    SuccessOrExit(error = mDecoder.ReadUint32(timeout));

    otThreadSetChildTimeout(mInstance, timeout);

exit:
    return error;
}

otError NcpBase::GetPropertyHandler_THREAD_PREFERRED_ROUTER_ID(void)
{
    return mEncoder.WriteUint8(mPreferredRouteId);
}

otError NcpBase::SetPropertyHandler_THREAD_PREFERRED_ROUTER_ID(void)
{
    otError error = OT_ERROR_NONE;

    SuccessOrExit(error = mDecoder.ReadUint8(mPreferredRouteId));

    SuccessOrExit(error = otThreadSetPreferredRouterId(mInstance, mPreferredRouteId));

exit:
    return error;
}

otError NcpBase::RemovePropertyHandler_THREAD_ACTIVE_ROUTER_IDS(void)
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

#if OPENTHREAD_ENABLE_TMF_PROXY
otError NcpBase::GetPropertyHandler_THREAD_TMF_PROXY_ENABLED(void)
{
    return mEncoder.WriteBool(otTmfProxyIsEnabled(mInstance));
}

otError NcpBase::SetPropertyHandler_THREAD_TMF_PROXY_STREAM(void)
{
    const uint8_t *framePtr = NULL;
    uint16_t frameLen = 0;
    uint16_t locator;
    uint16_t port;
    otMessage *message;
    otError error = OT_ERROR_NONE;

    // THREAD_TMF_PROXY_STREAM requires layer 2 security.
    message = otIp6NewMessage(mInstance, true);
    VerifyOrExit(message != NULL, error = OT_ERROR_NO_BUFS);

    SuccessOrExit(error = mDecoder.ReadDataWithLen(framePtr, frameLen));
    SuccessOrExit(error = mDecoder.ReadUint16(locator));
    SuccessOrExit(error = mDecoder.ReadUint16(port));

    SuccessOrExit(error = otMessageAppend(message, framePtr, static_cast<uint16_t>(frameLen)));

    error = otTmfProxySend(mInstance, message, locator, port);

    // `otTmfProxySend()` takes ownership of `message` (in both success
    // or failure cases). `message` is set to NULL so it is not freed at
    // exit.
    message = NULL;

exit:
    if (message != NULL)
    {
        otMessageFree(message);
    }

    return error;
}

otError NcpBase::SetPropertyHandler_THREAD_TMF_PROXY_ENABLED(void)
{
    bool enabled;
    otError error = OT_ERROR_NONE;

    SuccessOrExit(error = mDecoder.ReadBool(enabled));

    if (enabled)
    {
        error = otTmfProxyStart(mInstance, &NcpBase::HandleTmfProxyStream, this);
    }
    else
    {
        error = otTmfProxyStop(mInstance);
    }

exit:
    return error;
}

void NcpBase::HandleTmfProxyStream(otMessage *aMessage, uint16_t aLocator, uint16_t aPort, void *aContext)
{
    static_cast<NcpBase *>(aContext)->HandleTmfProxyStream(aMessage, aLocator, aPort);
}

void NcpBase::HandleTmfProxyStream(otMessage *aMessage, uint16_t aLocator, uint16_t aPort)
{
    otError error = OT_ERROR_NONE;
    uint16_t length = otMessageGetLength(aMessage);
    uint8_t header =  SPINEL_HEADER_FLAG | SPINEL_HEADER_IID_0;

    SuccessOrExit(error = mEncoder.BeginFrame(
                              header,
                              SPINEL_CMD_PROP_VALUE_IS,
                              SPINEL_PROP_THREAD_TMF_PROXY_STREAM
                          ));
    SuccessOrExit(error = mEncoder.WriteUint16(length));
    SuccessOrExit(error = mEncoder.WriteMessage(aMessage));

    SuccessOrExit(error = mEncoder.WriteUint16(aLocator));
    SuccessOrExit(error = mEncoder.WriteUint16(aPort));
    SuccessOrExit(error = mEncoder.EndFrame());

    // The `aMessage` is owned by the outbound frame and NCP buffer
    // after frame was finished/ended successfully. It will be freed
    // when the frame is successfully sent and removed.

    aMessage = NULL;

exit:

    if (aMessage != NULL)
    {
        otMessageFree(aMessage);
    }

    if (error != OT_ERROR_NONE)
    {
        mChangedPropsSet.AddLastStatus(SPINEL_STATUS_DROPPED);
        mUpdateChangedPropsTask.Post();
    }
}
#endif // OPENTHREAD_ENABLE_TMF_PROXY

otError NcpBase::DecodeOperationalDataset(otOperationalDataset &aDataset, const uint8_t **aTlvs, uint8_t *aTlvsLength)
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

    while (!mDecoder.IsAllReadInStruct())
    {
        unsigned int propKey;

        SuccessOrExit(error = mDecoder.OpenStruct());
        SuccessOrExit(error = mDecoder.ReadUintPacked(propKey));

        switch (static_cast<spinel_prop_key_t>(propKey))
        {
        case SPINEL_PROP_DATASET_ACTIVE_TIMESTAMP:
            SuccessOrExit(error = mDecoder.ReadUint64(aDataset.mActiveTimestamp));
            aDataset.mIsActiveTimestampSet = true;
            break;

        case SPINEL_PROP_DATASET_PENDING_TIMESTAMP:
            SuccessOrExit(error = mDecoder.ReadUint64(aDataset.mPendingTimestamp));
            aDataset.mIsPendingTimestampSet = true;
            break;

        case SPINEL_PROP_NET_MASTER_KEY:
        {
            const uint8_t *key;
            uint16_t len;

            SuccessOrExit(error = mDecoder.ReadData(key, len));
            VerifyOrExit(len == OT_MASTER_KEY_SIZE, error = OT_ERROR_INVALID_ARGS);
            memcpy(aDataset.mMasterKey.m8, key, len);
            aDataset.mIsMasterKeySet = true;
            break;
        }

        case SPINEL_PROP_NET_NETWORK_NAME:
        {
            const char *name;
            size_t len;

            SuccessOrExit(error = mDecoder.ReadUtf8(name));
            len = strlen(name);
            VerifyOrExit(len <= OT_NETWORK_NAME_MAX_SIZE, error = OT_ERROR_INVALID_ARGS);
            memcpy(aDataset.mNetworkName.m8, name, len + 1);
            aDataset.mIsNetworkNameSet = true;
            break;
        }

        case SPINEL_PROP_NET_XPANID:
        {
            const uint8_t *xpanid;
            uint16_t len;

            SuccessOrExit(error = mDecoder.ReadData(xpanid, len));
            VerifyOrExit(len == OT_EXT_PAN_ID_SIZE, error = OT_ERROR_INVALID_ARGS);
            memcpy(aDataset.mExtendedPanId.m8, xpanid, len);
            aDataset.mIsExtendedPanIdSet = true;
            break;
        }

        case SPINEL_PROP_IPV6_ML_PREFIX:
        {
            const otIp6Address *addr;
            uint8_t prefixLen;

            SuccessOrExit(error = mDecoder.ReadIp6Address(addr));
            SuccessOrExit(error = mDecoder.ReadUint8(prefixLen));
            VerifyOrExit(prefixLen == 64, error = OT_ERROR_INVALID_ARGS);
            memcpy(aDataset.mMeshLocalPrefix.m8, addr, OT_MESH_LOCAL_PREFIX_SIZE);
            aDataset.mIsMeshLocalPrefixSet = true;
            break;
        }

        case SPINEL_PROP_DATASET_DELAY_TIMER:
            SuccessOrExit(error = mDecoder.ReadUint32(aDataset.mDelay));
            aDataset.mIsDelaySet = true;
            break;

        case SPINEL_PROP_MAC_15_4_PANID:
            SuccessOrExit(error = mDecoder.ReadUint16(aDataset.mPanId));
            aDataset.mIsPanIdSet = true;
            break;

        case SPINEL_PROP_PHY_CHAN:
        {
            uint8_t channel;

            SuccessOrExit(error = mDecoder.ReadUint8(channel));
            aDataset.mChannel = channel;
            aDataset.mIsChannelSet = true;
            break;
        }

        case SPINEL_PROP_NET_PSKC:
        {
            const uint8_t *psk;
            uint16_t len;

            SuccessOrExit(error = mDecoder.ReadData(psk, len));
            VerifyOrExit(len == OT_PSKC_MAX_SIZE, error = OT_ERROR_INVALID_ARGS);
            memcpy(aDataset.mPSKc.m8, psk, OT_PSKC_MAX_SIZE);
            aDataset.mIsPSKcSet = true;
            break;
        }

        case SPINEL_PROP_DATASET_SECURITY_POLICY:
            SuccessOrExit(error = mDecoder.ReadUint16(aDataset.mSecurityPolicy.mRotationTime));
            SuccessOrExit(error = mDecoder.ReadUint8(aDataset.mSecurityPolicy.mFlags));
            aDataset.mIsSecurityPolicySet = true;
            break;

        case SPINEL_PROP_PHY_CHAN_SUPPORTED:
        {
            uint8_t channel;

            aDataset.mChannelMaskPage0 = 0;

            while (!mDecoder.IsAllReadInStruct())
            {
                SuccessOrExit(error = mDecoder.ReadUint8(channel));
                VerifyOrExit(channel <= 31, error = OT_ERROR_INVALID_ARGS);
                aDataset.mChannelMaskPage0 |= (1U << channel);
            }

            aDataset.mIsChannelMaskPage0Set = true;
            break;
        }

        case SPINEL_PROP_DATASET_RAW_TLVS:
        {
            const uint8_t *tlvs;
            uint16_t len;

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

            break;
        }

        default:
            break;
        }

        SuccessOrExit(error = mDecoder.CloseStruct());
    }

exit:
    return error;
}

otError NcpBase::SetPropertyHandler_THREAD_ACTIVE_DATASET(void)
{
    otError error = OT_ERROR_NONE;
    otOperationalDataset dataset;

    SuccessOrExit(error = DecodeOperationalDataset(dataset, NULL, NULL));
    error = otDatasetSetActive(mInstance, &dataset);

exit:
    return error;
}

otError NcpBase::SetPropertyHandler_THREAD_PENDING_DATASET(void)
{
    otError error = OT_ERROR_NONE;
    otOperationalDataset dataset;

    SuccessOrExit(error = DecodeOperationalDataset(dataset, NULL, NULL));
    error = otDatasetSetPending(mInstance, &dataset);

exit:
    return error;
}

otError NcpBase::SetPropertyHandler_THREAD_MGMT_ACTIVE_DATASET(void)
{
    otError error = OT_ERROR_NONE;
    otOperationalDataset dataset;
    const uint8_t *extraTlvs;
    uint8_t extraTlvsLength;

    SuccessOrExit(error = DecodeOperationalDataset(dataset, &extraTlvs, &extraTlvsLength));
    error = otDatasetSendMgmtActiveSet(mInstance, &dataset, extraTlvs, extraTlvsLength);

exit:
    return error;

}

otError NcpBase::SetPropertyHandler_THREAD_MGMT_PENDING_DATASET(void)
{
    otError error = OT_ERROR_NONE;
    otOperationalDataset dataset;
    const uint8_t *extraTlvs;
    uint8_t extraTlvsLength;

    SuccessOrExit(error = DecodeOperationalDataset(dataset, &extraTlvs, &extraTlvsLength));
    error = otDatasetSendMgmtPendingSet(mInstance, &dataset, extraTlvs, extraTlvsLength);

exit:
    return error;
}

}  // namespace Ncp
}  // namespace ot

#endif  // OPENTHREAD_FTD
