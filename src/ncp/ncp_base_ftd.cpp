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

#include "openthread-instance.h"
#include "common/code_utils.hpp"
#include "common/debug.hpp"
#if OPENTHREAD_ENABLE_COMMISSIONER
#include "meshcop/commissioner.hpp"
#endif
#include "net/ip6.hpp"

#if OPENTHREAD_FTD
namespace ot {
namespace Ncp {

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
    uint8_t modeFlags;

    maxChildren = otThreadGetMaxAllowedChildren(mInstance);

    for (uint8_t index = 0; index < maxChildren; index++)
    {
        if (otThreadGetChildInfoByIndex(mInstance, index, &childInfo) != OT_ERROR_NONE)
        {
            continue;
        }

        modeFlags = LinkFlagsToFlagByte(
                        childInfo.mRxOnWhenIdle,
                        childInfo.mSecureDataRequest,
                        childInfo.mFullFunction,
                        childInfo.mFullNetworkData
                    );

        SuccessOrExit(error = mEncoder.OpenStruct());

        SuccessOrExit(error = mEncoder.WriteEui64(childInfo.mExtAddress));
        SuccessOrExit(error = mEncoder.WriteUint16(childInfo.mRloc16));
        SuccessOrExit(error = mEncoder.WriteUint32(childInfo.mTimeout));
        SuccessOrExit(error = mEncoder.WriteUint32(childInfo.mAge));
        SuccessOrExit(error = mEncoder.WriteUint8(childInfo.mNetworkDataVersion));
        SuccessOrExit(error = mEncoder.WriteUint8(childInfo.mLinkQualityIn));
        SuccessOrExit(error = mEncoder.WriteInt8(childInfo.mAverageRssi));
        SuccessOrExit(error = mEncoder.WriteUint8(modeFlags));
        SuccessOrExit(error = mEncoder.WriteInt8(childInfo.mLastRssi));

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
    return SendLastStatus(aHeader, ThreadErrorToSpinelStatus(error));
}

otError NcpBase::InsertPropertyHandler_THREAD_JOINERS(void)
{
    otError error = OT_ERROR_NONE;
    const otExtAddress *extAddress = NULL;
    const char *aPSKd = NULL;
    uint32_t joinerTimeout = 0;

    VerifyOrExit(mAllowLocalNetworkDataChange == true, error = OT_ERROR_INVALID_STATE);

    SuccessOrExit(error = mDecoder.ReadUtf8(aPSKd));
    SuccessOrExit(error = mDecoder.ReadUint32(joinerTimeout));

    if (mDecoder.ReadEui64(extAddress) != OT_ERROR_NONE)
    {
        extAddress = NULL;
    }


    error = otCommissionerAddJoiner(mInstance, extAddress, aPSKd, joinerTimeout);

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
otError NcpBase::SetPropertyHandler_THREAD_STEERING_DATA(void)
{
    const otExtAddress *extAddress;
    otError error = OT_ERROR_NONE;

    SuccessOrExit(error = mDecoder.ReadEui64(extAddress));

    SuccessOrExit(error = otThreadSetSteeringData(mInstance, extAddress));

    // Note that there is no get handler for this property
    // so the response becomes `VALUE_IS` echo of the
    // received content.

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

otError NcpBase::SetPropertyHandler_THREAD_PREFERRED_ROUTER_ID(void)
{
    uint8_t routerId = 0;
    otError error = OT_ERROR_NONE;

    SuccessOrExit(error = mDecoder.ReadUint8(routerId));

    SuccessOrExit(error = otThreadSetPreferredRouterId(mInstance, routerId));

    // Note that there is no get handler for this property
    // so the response becomes `VALUE_IS` echo of the
    // received content.

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

}  // namespace Ncp
}  // namespace ot

#endif  // OPENTHREAD_FTD
