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
 *   This file implements minimal thread device required Spinel interface to the OpenThread stack.
 */

#include <openthread/config.h>
#include "ncp_base.hpp"

#if OPENTHREAD_ENABLE_BORDER_ROUTER
#include <openthread/border_router.h>
#endif
#include <openthread/diag.h>
#include <openthread/icmp6.h>
#if OPENTHREAD_ENABLE_JAM_DETECTION
#include <openthread/jam_detection.h>
#endif
#include <openthread/ncp.h>
#include <openthread/openthread.h>
#include <openthread/platform/misc.h>
#include <openthread/platform/radio.h>
#if OPENTHREAD_FTD
#include <openthread/thread_ftd.h>
#endif

#include "openthread-instance.h"
#include "common/code_utils.hpp"
#include "common/debug.hpp"
#include "net/ip6.hpp"

#if OPENTHREAD_MTD || OPENTHREAD_FTD

namespace ot {
namespace Ncp {

#if OPENTHREAD_ENABLE_RAW_LINK_API

static bool HasOnly1BitSet(uint32_t aValue)
{
    return aValue != 0 && ((aValue & (aValue - 1)) == 0);
}

static uint8_t IndexOfMSB(uint32_t aValue)
{
    uint8_t index = 0;

    while (aValue >>= 1)
    {
        index++;
    }

    return index;
}

#endif // OPENTHREAD_ENABLE_RAW_LINK_API

static uint8_t BorderRouterConfigToFlagByte(const otBorderRouterConfig &aConfig)
{
    uint8_t flags(0);

    if (aConfig.mPreferred)
    {
        flags |= SPINEL_NET_FLAG_PREFERRED;
    }

    if (aConfig.mSlaac)
    {
        flags |= SPINEL_NET_FLAG_SLAAC;
    }

    if (aConfig.mDhcp)
    {
        flags |= SPINEL_NET_FLAG_DHCP;
    }

    if (aConfig.mDefaultRoute)
    {
        flags |= SPINEL_NET_FLAG_DEFAULT_ROUTE;
    }

    if (aConfig.mConfigure)
    {
        flags |= SPINEL_NET_FLAG_CONFIGURE;
    }

    if (aConfig.mOnMesh)
    {
        flags |= SPINEL_NET_FLAG_ON_MESH;
    }

    flags |= (aConfig.mPreference << SPINEL_NET_FLAG_PREFERENCE_OFFSET);

    return flags;
}

static uint8_t ExternalRoutePreferenceToFlagByte(int aPreference)
{
    uint8_t flags;

    switch (aPreference)
    {
    case OT_ROUTE_PREFERENCE_LOW:
        flags = SPINEL_ROUTE_PREFERENCE_LOW;
        break;

    case OT_ROUTE_PREFERENCE_MED:
        flags = SPINEL_ROUTE_PREFERENCE_MEDIUM;
        break;

    case OT_ROUTE_PREFERENCE_HIGH:
        flags = SPINEL_ROUTE_PREFERENCE_HIGH;
        break;

    default:
        flags = SPINEL_ROUTE_PREFERENCE_MEDIUM;
        break;
    }

    return flags;
}

uint8_t NcpBase::LinkFlagsToFlagByte(bool aRxOnWhenIdle, bool aSecureDataRequests, bool aDeviceType, bool aNetworkData)
{
    uint8_t flags(0);

    if (aRxOnWhenIdle)
    {
        flags |= SPINEL_THREAD_MODE_RX_ON_WHEN_IDLE;
    }

    if (aSecureDataRequests)
    {
        flags |= SPINEL_THREAD_MODE_SECURE_DATA_REQUEST;
    }

    if (aDeviceType)
    {
        flags |= SPINEL_THREAD_MODE_FULL_FUNCTION_DEV;
    }

    if (aNetworkData)
    {
        flags |= SPINEL_THREAD_MODE_FULL_NETWORK_DATA;
    }

    return flags;
}

otError NcpBase::GetPropertyHandler_MAC_DATA_POLL_PERIOD(uint8_t aHeader, spinel_prop_key_t aKey)
{
    return SendPropertyUpdate(
               aHeader,
               SPINEL_CMD_PROP_VALUE_IS,
               aKey,
               SPINEL_DATATYPE_UINT32_S,
               otLinkGetPollPeriod(mInstance)
           );
}

otError NcpBase::SetPropertyHandler_MAC_DATA_POLL_PERIOD(uint8_t aHeader, spinel_prop_key_t aKey,
                                                         const uint8_t *aValuePtr, uint16_t aValueLen)
{
    uint32_t pollPeriod;
    spinel_ssize_t parsedLength;
    otError error = OT_ERROR_NONE;

    parsedLength = spinel_datatype_unpack(
                       aValuePtr,
                       aValueLen,
                       SPINEL_DATATYPE_UINT32_S,
                       &pollPeriod
                   );

    VerifyOrExit(parsedLength > 0, error = OT_ERROR_PARSE);

    otLinkSetPollPeriod(mInstance, pollPeriod);

exit:
    return SendSetPropertyResponse(aHeader, aKey, error);
}

otError NcpBase::GetPropertyHandler_MAC_BEACON_JOINABLE(uint8_t aHeader, spinel_prop_key_t aKey)
{
    return SendPropertyUpdate(
               aHeader,
               SPINEL_CMD_PROP_VALUE_IS,
               aKey,
               SPINEL_DATATYPE_BOOL_S,
               otLinkGetBeaconJoinableFlag(mInstance)
           );
}

otError NcpBase::SetPropertyHandler_MAC_BEACON_JOINABLE(uint8_t aHeader, spinel_prop_key_t aKey,
                                                         const uint8_t *aValuePtr, uint16_t aValueLen)
{
    bool joinableFlag;
    spinel_ssize_t parsedLength;
    otError error = OT_ERROR_NONE;

    parsedLength = spinel_datatype_unpack(
                       aValuePtr,
                       aValueLen,
                       SPINEL_DATATYPE_BOOL_S,
                       &joinableFlag
                   );

    VerifyOrExit(parsedLength > 0, error = OT_ERROR_PARSE);

    otLinkSetBeaconJoinableFlag(mInstance, joinableFlag);

exit:
    return SendSetPropertyResponse(aHeader, aKey, error);
}

otError NcpBase::GetPropertyHandler_MAC_EXTENDED_ADDR(uint8_t aHeader, spinel_prop_key_t aKey)
{
    return SendPropertyUpdate(
               aHeader,
               SPINEL_CMD_PROP_VALUE_IS,
               aKey,
               SPINEL_DATATYPE_EUI64_S,
               otLinkGetExtendedAddress(mInstance)
           );
}

otError NcpBase::GetPropertyHandler_PHY_FREQ(uint8_t aHeader, spinel_prop_key_t aKey)
{
    uint32_t freq_khz(0);
    const uint8_t chan(otLinkGetChannel(mInstance));

    if (chan == 0)
    {
        freq_khz = 868300;
    }
    else if (chan < 11)
    {
        freq_khz = 906000 - (2000 * 1) + 2000 * (chan);
    }
    else if (chan < 26)
    {
        freq_khz = 2405000 - (5000 * 11) + 5000 * (chan);
    }

    return SendPropertyUpdate(
               aHeader,
               SPINEL_CMD_PROP_VALUE_IS,
               aKey,
               SPINEL_DATATYPE_UINT32_S,
               freq_khz
           );
}

otError NcpBase::GetPropertyHandler_PHY_CHAN_SUPPORTED(uint8_t aHeader, spinel_prop_key_t aKey)
{
    return GetPropertyHandler_ChannelMaskHelper(aHeader, aKey, mSupportedChannelMask);
}

otError NcpBase::GetPropertyHandler_PHY_RSSI(uint8_t aHeader, spinel_prop_key_t aKey)
{
    return SendPropertyUpdate(
               aHeader,
               SPINEL_CMD_PROP_VALUE_IS,
               aKey,
               SPINEL_DATATYPE_INT8_S,
               otPlatRadioGetRssi(mInstance)
           );
}

otError NcpBase::CommandHandler_NET_SAVE(uint8_t aHeader, unsigned int aCommand, const uint8_t *aArgPtr,
                                         uint16_t aArgLen)
{
    OT_UNUSED_VARIABLE(aCommand);
    OT_UNUSED_VARIABLE(aArgPtr);
    OT_UNUSED_VARIABLE(aArgLen);

    return SendLastStatus(aHeader, SPINEL_STATUS_UNIMPLEMENTED);
}

otError NcpBase::CommandHandler_NET_CLEAR(uint8_t aHeader, unsigned int aCommand, const uint8_t *aArgPtr,
                                          uint16_t aArgLen)
{
    OT_UNUSED_VARIABLE(aCommand);
    OT_UNUSED_VARIABLE(aArgPtr);
    OT_UNUSED_VARIABLE(aArgLen);

    return SendLastStatus(aHeader, ThreadErrorToSpinelStatus(otInstanceErasePersistentInfo(mInstance)));
}

otError NcpBase::CommandHandler_NET_RECALL(uint8_t aHeader, unsigned int aCommand, const uint8_t *aArgPtr,
                                           uint16_t aArgLen)
{
    OT_UNUSED_VARIABLE(aCommand);
    OT_UNUSED_VARIABLE(aArgPtr);
    OT_UNUSED_VARIABLE(aArgLen);

    return SendLastStatus(aHeader, SPINEL_STATUS_UNIMPLEMENTED);
}

otError NcpBase::GetPropertyHandler_NET_SAVED(uint8_t aHeader, spinel_prop_key_t aKey)
{
    return SendPropertyUpdate(
               aHeader,
               SPINEL_CMD_PROP_VALUE_IS,
               aKey,
               SPINEL_DATATYPE_BOOL_S,
               otDatasetIsCommissioned(mInstance)
           );
}

otError NcpBase::GetPropertyHandler_NET_IF_UP(uint8_t aHeader, spinel_prop_key_t aKey)
{
    return SendPropertyUpdate(
               aHeader,
               SPINEL_CMD_PROP_VALUE_IS,
               aKey,
               SPINEL_DATATYPE_BOOL_S,
               otIp6IsEnabled(mInstance)
           );
}

otError NcpBase::SetPropertyHandler_NET_IF_UP(uint8_t aHeader, spinel_prop_key_t aKey, const uint8_t *aValuePtr,
                                              uint16_t aValueLen)
{
    bool enabled = false;
    spinel_ssize_t parsedLength;
    otError error = OT_ERROR_NONE;

    parsedLength = spinel_datatype_unpack(
                       aValuePtr,
                       aValueLen,
                       SPINEL_DATATYPE_BOOL_S,
                       &enabled
                   );

    VerifyOrExit(parsedLength > 0, error = OT_ERROR_PARSE);

    error = otIp6SetEnabled(mInstance, enabled);

exit:
    return SendSetPropertyResponse(aHeader, aKey, error);
}

otError NcpBase::GetPropertyHandler_NET_STACK_UP(uint8_t aHeader, spinel_prop_key_t aKey)
{
    return SendPropertyUpdate(
               aHeader,
               SPINEL_CMD_PROP_VALUE_IS,
               aKey,
               SPINEL_DATATYPE_BOOL_S,
               otThreadGetDeviceRole(mInstance) != OT_DEVICE_ROLE_DISABLED
           );
}

otError NcpBase::SetPropertyHandler_NET_STACK_UP(uint8_t aHeader, spinel_prop_key_t aKey, const uint8_t *aValuePtr,
                                                 uint16_t aValueLen)
{
    bool enabled = false;
    spinel_ssize_t parsedLength;
    otError error = OT_ERROR_NONE;

    parsedLength = spinel_datatype_unpack(
                       aValuePtr,
                       aValueLen,
                       SPINEL_DATATYPE_BOOL_S,
                       &enabled
                   );

    VerifyOrExit(parsedLength > 0, error = OT_ERROR_PARSE);

    // If the value has changed...
    if ((enabled != false) != (otThreadGetDeviceRole(mInstance) != OT_DEVICE_ROLE_DISABLED))
    {
        if (enabled != false)
        {
            error = otThreadSetEnabled(mInstance, true);

#if OPENTHREAD_ENABLE_LEGACY
            mLegacyNodeDidJoin = false;

            if ((mLegacyHandlers != NULL) && (mLegacyHandlers->mStartLegacy != NULL))
            {
                mLegacyHandlers->mStartLegacy();
            }
#endif // OPENTHREAD_ENABLE_LEGACY
        }
        else
        {
            error = otThreadSetEnabled(mInstance, false);

#if OPENTHREAD_ENABLE_LEGACY
            mLegacyNodeDidJoin = false;

            if ((mLegacyHandlers != NULL) && (mLegacyHandlers->mStopLegacy != NULL))
            {
                mLegacyHandlers->mStopLegacy();
            }
#endif // OPENTHREAD_ENABLE_LEGACY
        }
    }

exit:
    return SendSetPropertyResponse(aHeader, aKey, error);
}

otError NcpBase::GetPropertyHandler_NET_ROLE(uint8_t aHeader, spinel_prop_key_t aKey)
{
    spinel_net_role_t role(SPINEL_NET_ROLE_DETACHED);

    switch (otThreadGetDeviceRole(mInstance))
    {
    case OT_DEVICE_ROLE_DISABLED:
    case OT_DEVICE_ROLE_DETACHED:
        role = SPINEL_NET_ROLE_DETACHED;
        break;

    case OT_DEVICE_ROLE_CHILD:
        role = SPINEL_NET_ROLE_CHILD;
        break;

    case OT_DEVICE_ROLE_ROUTER:
        role = SPINEL_NET_ROLE_ROUTER;
        break;

    case OT_DEVICE_ROLE_LEADER:
        role = SPINEL_NET_ROLE_LEADER;
        break;
    }

    return SendPropertyUpdate(
               aHeader,
               SPINEL_CMD_PROP_VALUE_IS,
               aKey,
               SPINEL_DATATYPE_UINT8_S,
               role
           );
}

otError NcpBase::SetPropertyHandler_NET_ROLE(uint8_t aHeader, spinel_prop_key_t aKey, const uint8_t *aValuePtr,
                                             uint16_t aValueLen)
{
    unsigned int role = 0;
    spinel_ssize_t parsedLength;
    otError error = OT_ERROR_NONE;

    parsedLength = spinel_datatype_unpack(
                       aValuePtr,
                       aValueLen,
                       SPINEL_DATATYPE_UINT_PACKED_S,
                       &role
                   );

    VerifyOrExit(parsedLength > 0, error = OT_ERROR_PARSE);


    switch (role)
    {
    case SPINEL_NET_ROLE_DETACHED:
        error = otThreadBecomeDetached(mInstance);
        break;

#if OPENTHREAD_FTD
    case SPINEL_NET_ROLE_ROUTER:
        error = otThreadBecomeRouter(mInstance);
        break;

    case SPINEL_NET_ROLE_LEADER:
        error = otThreadBecomeLeader(mInstance);
        break;
#endif  // OPENTHREAD_FTD

    case SPINEL_NET_ROLE_CHILD:
        error = otThreadBecomeChild(mInstance);
        break;
    }

exit:
    return SendSetPropertyResponse(aHeader, aKey, error);
}

otError NcpBase::GetPropertyHandler_NET_NETWORK_NAME(uint8_t aHeader, spinel_prop_key_t aKey)
{
    return SendPropertyUpdate(
               aHeader,
               SPINEL_CMD_PROP_VALUE_IS,
               aKey,
               SPINEL_DATATYPE_UTF8_S,
               otThreadGetNetworkName(mInstance)
           );
}

otError NcpBase::SetPropertyHandler_NET_NETWORK_NAME(uint8_t aHeader, spinel_prop_key_t aKey, const uint8_t *aValuePtr,
                                                     uint16_t aValueLen)
{
    const char *string = NULL;
    spinel_ssize_t parsedLength;
    otError error = OT_ERROR_NONE;

    parsedLength = spinel_datatype_unpack(
                       aValuePtr,
                       aValueLen,
                       SPINEL_DATATYPE_UTF8_S,
                       &string
                   );

    VerifyOrExit((parsedLength > 0) && (string != NULL), error = OT_ERROR_PARSE);

    error = otThreadSetNetworkName(mInstance, string);

exit:
    return SendSetPropertyResponse(aHeader, aKey, error);
}

otError NcpBase::GetPropertyHandler_NET_XPANID(uint8_t aHeader, spinel_prop_key_t aKey)
{
    return SendPropertyUpdate(
               aHeader,
               SPINEL_CMD_PROP_VALUE_IS,
               aKey,
               SPINEL_DATATYPE_DATA_S,
               otThreadGetExtendedPanId(mInstance),
               sizeof(spinel_net_xpanid_t)
           );
}

otError NcpBase::SetPropertyHandler_NET_XPANID(uint8_t aHeader, spinel_prop_key_t aKey, const uint8_t *aValuePtr,
                                               uint16_t aValueLen)
{
    const uint8_t *ptr = NULL;
    spinel_size_t len;
    spinel_ssize_t parsedLength;
    otError error = OT_ERROR_NONE;

    parsedLength = spinel_datatype_unpack(
                       aValuePtr,
                       aValueLen,
                       SPINEL_DATATYPE_DATA_S,
                       &ptr,
                       &len
                   );

    VerifyOrExit((parsedLength > 0) && (len == sizeof(spinel_net_xpanid_t)), error = OT_ERROR_PARSE);

    error = otThreadSetExtendedPanId(mInstance, ptr);

exit:
    return SendSetPropertyResponse(aHeader, aKey, error);
}

otError NcpBase::GetPropertyHandler_NET_MASTER_KEY(uint8_t aHeader, spinel_prop_key_t aKey)
{
    return SendPropertyUpdate(
               aHeader,
               SPINEL_CMD_PROP_VALUE_IS,
               aKey,
               SPINEL_DATATYPE_DATA_S,
               otThreadGetMasterKey(mInstance)->m8,
               OT_MASTER_KEY_SIZE
           );
}

otError NcpBase::SetPropertyHandler_NET_MASTER_KEY(uint8_t aHeader, spinel_prop_key_t aKey, const uint8_t *aValuePtr,
                                                   uint16_t aValueLen)
{
    const uint8_t *ptr = NULL;
    spinel_size_t len;
    spinel_ssize_t parsedLength;
    otError error = OT_ERROR_NONE;

    parsedLength = spinel_datatype_unpack(
                       aValuePtr,
                       aValueLen,
                       SPINEL_DATATYPE_DATA_S,
                       &ptr,
                       &len
                   );

    VerifyOrExit((parsedLength > 0) && (len == OT_MASTER_KEY_SIZE), error = OT_ERROR_PARSE);

    error = otThreadSetMasterKey(mInstance, reinterpret_cast<const otMasterKey *>(ptr));

exit:
    return SendSetPropertyResponse(aHeader, aKey, error);
}

otError NcpBase::GetPropertyHandler_NET_KEY_SEQUENCE_COUNTER(uint8_t aHeader, spinel_prop_key_t aKey)
{
    return SendPropertyUpdate(
               aHeader,
               SPINEL_CMD_PROP_VALUE_IS,
               aKey,
               SPINEL_DATATYPE_UINT32_S,
               otThreadGetKeySequenceCounter(mInstance)
           );
}

otError NcpBase::SetPropertyHandler_NET_KEY_SEQUENCE_COUNTER(uint8_t aHeader, spinel_prop_key_t aKey,
                                                             const uint8_t *aValuePtr, uint16_t aValueLen)
{
    unsigned int keySeqCounter;
    spinel_ssize_t parsedLength;
    otError error = OT_ERROR_NONE;

    parsedLength = spinel_datatype_unpack(
                       aValuePtr,
                       aValueLen,
                       SPINEL_DATATYPE_UINT32_S,
                       &keySeqCounter
                   );

    VerifyOrExit(parsedLength > 0, error = OT_ERROR_PARSE);

    otThreadSetKeySequenceCounter(mInstance, keySeqCounter);

exit:
    return SendSetPropertyResponse(aHeader, aKey, error);
}

otError NcpBase::GetPropertyHandler_NET_PARTITION_ID(uint8_t aHeader, spinel_prop_key_t aKey)
{
    return SendPropertyUpdate(
               aHeader,
               SPINEL_CMD_PROP_VALUE_IS,
               aKey,
               SPINEL_DATATYPE_UINT32_S,
               otThreadGetPartitionId(mInstance)
           );
}

otError NcpBase::GetPropertyHandler_NET_KEY_SWITCH_GUARDTIME(uint8_t aHeader, spinel_prop_key_t aKey)
{
    return SendPropertyUpdate(
               aHeader,
               SPINEL_CMD_PROP_VALUE_IS,
               aKey,
               SPINEL_DATATYPE_UINT32_S,
               otThreadGetKeySwitchGuardTime(mInstance)
           );
}

otError NcpBase::SetPropertyHandler_NET_KEY_SWITCH_GUARDTIME(uint8_t aHeader, spinel_prop_key_t aKey,
                                                             const uint8_t *aValuePtr, uint16_t aValueLen)
{
    unsigned int keyGuardTime;
    spinel_ssize_t parsedLength;
    otError error = OT_ERROR_NONE;

    parsedLength = spinel_datatype_unpack(
                       aValuePtr,
                       aValueLen,
                       SPINEL_DATATYPE_UINT32_S,
                       &keyGuardTime
                   );

    VerifyOrExit(parsedLength > 0, error = OT_ERROR_PARSE);

    otThreadSetKeySwitchGuardTime(mInstance, keyGuardTime);

exit:
    return SendSetPropertyResponse(aHeader, aKey, error);
}

otError NcpBase::GetPropertyHandler_THREAD_NETWORK_DATA_VERSION(uint8_t aHeader, spinel_prop_key_t aKey)
{
    return SendPropertyUpdate(
               aHeader,
               SPINEL_CMD_PROP_VALUE_IS,
               aKey,
               SPINEL_DATATYPE_UINT8_S,
               otNetDataGetVersion(mInstance)
           );
}

otError NcpBase::GetPropertyHandler_THREAD_STABLE_NETWORK_DATA_VERSION(uint8_t aHeader, spinel_prop_key_t aKey)
{
    return SendPropertyUpdate(
               aHeader,
               SPINEL_CMD_PROP_VALUE_IS,
               aKey,
               SPINEL_DATATYPE_UINT8_S,
               otNetDataGetStableVersion(mInstance)
           );
}

#if OPENTHREAD_ENABLE_BORDER_ROUTER
otError NcpBase::GetPropertyHandler_THREAD_NETWORK_DATA(uint8_t aHeader, spinel_prop_key_t aKey)
{
    otError error = OT_ERROR_NONE;
    uint8_t networkData[255];
    uint8_t networkDataLen = 255;

    otBorderRouterGetNetData(
        mInstance,
        false, // Stable?
        networkData,
        &networkDataLen
    );

    SuccessOrExit(error = OutboundFrameBegin(aHeader));
    SuccessOrExit(
        error = OutboundFrameFeedPacked(
                    SPINEL_DATATYPE_COMMAND_PROP_S,
                    aHeader,
                    SPINEL_CMD_PROP_VALUE_IS,
                    aKey
                ));
    SuccessOrExit(error = OutboundFrameFeedData(networkData, networkDataLen));
    SuccessOrExit(error = OutboundFrameSend());

exit:
    return error;
}

otError NcpBase::GetPropertyHandler_THREAD_STABLE_NETWORK_DATA(uint8_t aHeader, spinel_prop_key_t aKey)
{
    otError error = OT_ERROR_NONE;
    uint8_t networkData[255];
    uint8_t networkDataLen = 255;

    otBorderRouterGetNetData(
        mInstance,
        true, // Stable?
        networkData,
        &networkDataLen
    );

    SuccessOrExit(error = OutboundFrameBegin(aHeader));
    SuccessOrExit(
        error = OutboundFrameFeedPacked(
                    SPINEL_DATATYPE_COMMAND_PROP_S,
                    aHeader,
                    SPINEL_CMD_PROP_VALUE_IS,
                    aKey
                ));
    SuccessOrExit(error = OutboundFrameFeedData(networkData, networkDataLen));
    SuccessOrExit(error = OutboundFrameSend());

exit:
    return error;
}
#endif // OPENTHREAD_ENABLE_BORDER_ROUTER

otError NcpBase::GetPropertyHandler_THREAD_LEADER_NETWORK_DATA(uint8_t aHeader, spinel_prop_key_t aKey)
{
    otError error = OT_ERROR_NONE;
    uint8_t networkData[255];
    uint8_t networkDataLen = 255;

    otNetDataGet(
        mInstance,
        false, // Stable?
        networkData,
        &networkDataLen
    );

    SuccessOrExit(error = OutboundFrameBegin(aHeader));
    SuccessOrExit(
        error = OutboundFrameFeedPacked(
                    SPINEL_DATATYPE_COMMAND_PROP_S,
                    aHeader,
                    SPINEL_CMD_PROP_VALUE_IS,
                    aKey
                ));
    SuccessOrExit(error = OutboundFrameFeedData(networkData, networkDataLen));
    SuccessOrExit(error = OutboundFrameSend());

exit:
    return error;
}

otError NcpBase::GetPropertyHandler_THREAD_STABLE_LEADER_NETWORK_DATA(uint8_t aHeader, spinel_prop_key_t aKey)
{
    otError error = OT_ERROR_NONE;
    uint8_t networkData[255];
    uint8_t networkDataLen = 255;

    otNetDataGet(
        mInstance,
        true, // Stable?
        networkData,
        &networkDataLen
    );

    SuccessOrExit(error = OutboundFrameBegin(aHeader));
    SuccessOrExit(
        error = OutboundFrameFeedPacked(
                    SPINEL_DATATYPE_COMMAND_PROP_S,
                    aHeader,
                    SPINEL_CMD_PROP_VALUE_IS,
                    aKey
                ));
    SuccessOrExit(error = OutboundFrameFeedData(networkData, networkDataLen));
    SuccessOrExit(error = OutboundFrameSend());

exit:
    return error;
}

otError NcpBase::GetPropertyHandler_THREAD_LEADER_RID(uint8_t aHeader, spinel_prop_key_t aKey)
{
    return SendPropertyUpdate(
               aHeader,
               SPINEL_CMD_PROP_VALUE_IS,
               aKey,
               SPINEL_DATATYPE_UINT8_S,
               otThreadGetLeaderRouterId(mInstance)
           );
}



otError NcpBase::GetPropertyHandler_THREAD_LEADER_ADDR(uint8_t aHeader, spinel_prop_key_t aKey)
{
    otError error = OT_ERROR_NONE;
    otIp6Address address;

    error = otThreadGetLeaderRloc(mInstance, &address);

    if (error == OT_ERROR_NONE)
    {
        error = SendPropertyUpdate(
                    aHeader,
                    SPINEL_CMD_PROP_VALUE_IS,
                    aKey,
                    SPINEL_DATATYPE_IPv6ADDR_S,
                    &address
                );
    }
    else
    {
        error = SendLastStatus(aHeader, ThreadErrorToSpinelStatus(error));
    }

    return error;
}

otError NcpBase::GetPropertyHandler_THREAD_PARENT(uint8_t aHeader, spinel_prop_key_t aKey)
{
    otError error = OT_ERROR_NONE;
    otRouterInfo parentInfo;

    error = otThreadGetParentInfo(mInstance, &parentInfo);

    if (error == OT_ERROR_NONE)
    {
        error = SendPropertyUpdate(
                    aHeader,
                    SPINEL_CMD_PROP_VALUE_IS,
                    aKey,
                    (
                        SPINEL_DATATYPE_EUI64_S   // Parent's extended address
                        SPINEL_DATATYPE_UINT16_S  // Parent's rloc16
                    ),
                    parentInfo.mExtAddress.m8,
                    parentInfo.mRloc16
                );
    }
    else
    {
        error = SendLastStatus(aHeader, ThreadErrorToSpinelStatus(error));
    }

    return error;
}

otError NcpBase::GetPropertyHandler_THREAD_NEIGHBOR_TABLE(uint8_t aHeader, spinel_prop_key_t aKey)
{
    otError error = OT_ERROR_NONE;
    otNeighborInfoIterator iter = OT_NEIGHBOR_INFO_ITERATOR_INIT;
    otNeighborInfo neighInfo;
    uint8_t modeFlags;

    mDisableStreamWrite = true;

    SuccessOrExit(error = OutboundFrameBegin(aHeader));
    SuccessOrExit(
        error = OutboundFrameFeedPacked(
                    SPINEL_DATATYPE_COMMAND_PROP_S,
                    aHeader,
                    SPINEL_CMD_PROP_VALUE_IS,
                    aKey
                ));

    while (otThreadGetNextNeighborInfo(mInstance, &iter, &neighInfo) == OT_ERROR_NONE)
    {
        modeFlags = LinkFlagsToFlagByte(
                        neighInfo.mRxOnWhenIdle,
                        neighInfo.mSecureDataRequest,
                        neighInfo.mFullFunction,
                        neighInfo.mFullNetworkData
                    );

        SuccessOrExit(
            error = OutboundFrameFeedPacked(
                        SPINEL_DATATYPE_STRUCT_S(
                            SPINEL_DATATYPE_EUI64_S         // EUI64 Address
                            SPINEL_DATATYPE_UINT16_S        // Rloc16
                            SPINEL_DATATYPE_UINT32_S        // Age
                            SPINEL_DATATYPE_UINT8_S         // Link Quality In
                            SPINEL_DATATYPE_INT8_S          // Average RSS
                            SPINEL_DATATYPE_UINT8_S         // Mode (flags)
                            SPINEL_DATATYPE_BOOL_S          // Is Child
                            SPINEL_DATATYPE_UINT32_S        // Link Frame Counter
                            SPINEL_DATATYPE_UINT32_S        // MLE Frame Counter
                            SPINEL_DATATYPE_INT8_S          // Most recent RSS
                        ),
                        neighInfo.mExtAddress.m8,
                        neighInfo.mRloc16,
                        neighInfo.mAge,
                        neighInfo.mLinkQualityIn,
                        neighInfo.mAverageRssi,
                        modeFlags,
                        neighInfo.mIsChild,
                        neighInfo.mLinkFrameCounter,
                        neighInfo.mMleFrameCounter,
                        neighInfo.mLastRssi
                    ));
    }

    SuccessOrExit(error = OutboundFrameSend());

exit:
    mDisableStreamWrite = false;
    return error;
}

otError NcpBase::GetPropertyHandler_THREAD_ASSISTING_PORTS(uint8_t aHeader, spinel_prop_key_t aKey)
{
    otError error = OT_ERROR_NONE;
    uint8_t numEntries = 0;
    const uint16_t *ports = otIp6GetUnsecurePorts(mInstance, &numEntries);

    SuccessOrExit(error = OutboundFrameBegin(aHeader));
    SuccessOrExit(
        error = OutboundFrameFeedPacked(
                    SPINEL_DATATYPE_COMMAND_PROP_S,
                    aHeader,
                    SPINEL_CMD_PROP_VALUE_IS,
                    aKey
                ));

    for (; numEntries != 0; ports++, numEntries--)
    {
        SuccessOrExit(error = OutboundFrameFeedPacked(SPINEL_DATATYPE_UINT16_S, *ports));
    }

    SuccessOrExit(error = OutboundFrameSend());

exit:
    return error;
}

otError NcpBase::SetPropertyHandler_THREAD_ASSISTING_PORTS(uint8_t aHeader, spinel_prop_key_t aKey,
                                                           const uint8_t *aValuePtr, uint16_t aValueLen)
{
    uint8_t numEntries = 0;
    const uint16_t *ports = otIp6GetUnsecurePorts(mInstance, &numEntries);
    spinel_ssize_t parsedLength;
    bool portsChanged = false;
    otError error = OT_ERROR_NONE;

    // First, we need to remove all of the current assisting ports.
    for (; numEntries != 0; ports++, numEntries--)
    {
        SuccessOrExit(error = otIp6RemoveUnsecurePort(mInstance, *ports));
        portsChanged = true;
    }

    while (aValueLen >= 2)
    {
        uint16_t port;

        parsedLength = spinel_datatype_unpack(
                           aValuePtr,
                           aValueLen,
                           SPINEL_DATATYPE_UINT16_S,
                           &port
                       );

        VerifyOrExit(parsedLength > 0, error = OT_ERROR_PARSE);

        SuccessOrExit(error = otIp6AddUnsecurePort(mInstance, port));

        aValuePtr += parsedLength;
        aValueLen -= parsedLength;

        portsChanged = true;
    }

    // No error happened so the new state of ports will
    // be reported in the response.
    portsChanged = false;

exit:
    error = SendSetPropertyResponse(aHeader, aKey, error);

    if (portsChanged)
    {
        // We had an error, but we've actually changed
        // the state of these ports---so we need to report
        // those incomplete changes via an asynchronous
        // change event.
        HandleCommandPropertyGet(SPINEL_HEADER_FLAG | SPINEL_HEADER_IID_0, aKey);
    }

    return error;
}

otError NcpBase::GetPropertyHandler_THREAD_ALLOW_LOCAL_NET_DATA_CHANGE(uint8_t aHeader, spinel_prop_key_t aKey)
{
    return SendPropertyUpdate(
               aHeader,
               SPINEL_CMD_PROP_VALUE_IS,
               aKey,
               SPINEL_DATATYPE_BOOL_S,
               mAllowLocalNetworkDataChange
           );
}

#if OPENTHREAD_ENABLE_BORDER_ROUTER
otError NcpBase::SetPropertyHandler_THREAD_ALLOW_LOCAL_NET_DATA_CHANGE(uint8_t aHeader, spinel_prop_key_t aKey,
                                                                       const uint8_t *aValuePtr, uint16_t aValueLen)
{
    bool value = false;
    spinel_ssize_t parsedLength;
    otError error = OT_ERROR_NONE;
    bool shouldRegisterWithLeader = false;

    parsedLength = spinel_datatype_unpack(
                       aValuePtr,
                       aValueLen,
                       SPINEL_DATATYPE_BOOL_S,
                       &value
                   );

    VerifyOrExit(parsedLength > 0, error = OT_ERROR_PARSE);

    // Register any net data changes on transition from `true` to `false`.
    shouldRegisterWithLeader = (mAllowLocalNetworkDataChange == true) && (value == false);

    mAllowLocalNetworkDataChange = value;

exit:
    error = SendSetPropertyResponse(aHeader, aKey, error);

    if (shouldRegisterWithLeader)
    {
        otBorderRouterRegister(mInstance);
    }

    return error;
}
#endif // OPENTHREAD_ENABLE_BORDER_ROUTER

otError NcpBase::GetPropertyHandler_THREAD_ON_MESH_NETS(uint8_t aHeader, spinel_prop_key_t aKey)
{
    otError error = OT_ERROR_NONE;
    otBorderRouterConfig borderRouterConfig;
    uint8_t flags;

    mDisableStreamWrite = true;

    SuccessOrExit(error = OutboundFrameBegin(aHeader));
    SuccessOrExit(
        error = OutboundFrameFeedPacked(
                    SPINEL_DATATYPE_COMMAND_PROP_S,
                    aHeader,
                    SPINEL_CMD_PROP_VALUE_IS,
                    aKey
                ));

    // Fill from non-local network data first
    for (otNetworkDataIterator iter = OT_NETWORK_DATA_ITERATOR_INIT ;;)
    {
        error = otNetDataGetNextOnMeshPrefix(mInstance, &iter, &borderRouterConfig);

        if (error != OT_ERROR_NONE)
        {
            break;
        }

        flags = BorderRouterConfigToFlagByte(borderRouterConfig);

        SuccessOrExit(
            error = OutboundFrameFeedPacked(
                        SPINEL_DATATYPE_STRUCT_S(
                            SPINEL_DATATYPE_IPv6ADDR_S      // IPv6 Prefix
                            SPINEL_DATATYPE_UINT8_S         // Prefix Length (in bits)
                            SPINEL_DATATYPE_BOOL_S          // isStable
                            SPINEL_DATATYPE_UINT8_S         // Flags
                            SPINEL_DATATYPE_BOOL_S          // isLocal
                            SPINEL_DATATYPE_UINT16_S        // RLOC16
                        ),
                        &borderRouterConfig.mPrefix,
                        64,
                        borderRouterConfig.mStable,
                        flags,
                        false,
                        borderRouterConfig.mRloc16
                    ));
    }

#if OPENTHREAD_ENABLE_BORDER_ROUTER
    // Fill from local network data last
    for (otNetworkDataIterator iter = OT_NETWORK_DATA_ITERATOR_INIT ;;)
    {
        error = otBorderRouterGetNextOnMeshPrefix(mInstance, &iter, &borderRouterConfig);

        if (error != OT_ERROR_NONE)
        {
            break;
        }

        flags = BorderRouterConfigToFlagByte(borderRouterConfig);

        SuccessOrExit(
            error = OutboundFrameFeedPacked(
                        SPINEL_DATATYPE_STRUCT_S(
                            SPINEL_DATATYPE_IPv6ADDR_S      // IPv6 Prefix
                            SPINEL_DATATYPE_UINT8_S         // Prefix Length (in bits)
                            SPINEL_DATATYPE_BOOL_S          // isStable
                            SPINEL_DATATYPE_UINT8_S         // Flags
                            SPINEL_DATATYPE_BOOL_S          // isLocal
                            SPINEL_DATATYPE_UINT16_S        // RLOC16
                        ),
                        &borderRouterConfig.mPrefix,
                        64,
                        borderRouterConfig.mStable,
                        flags,
                        true,
                        borderRouterConfig.mRloc16
                    ));
    }
#endif // OPENTHREAD_ENABLE_BORDER_ROUTER

    SuccessOrExit(error = OutboundFrameSend());

exit:
    mDisableStreamWrite = false;
    return error;
}

#if OPENTHREAD_ENABLE_BORDER_ROUTER
otError NcpBase::InsertPropertyHandler_THREAD_ON_MESH_NETS(uint8_t aHeader, spinel_prop_key_t aKey,
                                                           const uint8_t *aValuePtr, uint16_t aValueLen)
{
    spinel_ssize_t parsedLength;
    otError error = OT_ERROR_NONE;
    spinel_status_t spinelError = SPINEL_STATUS_OK;
    otBorderRouterConfig borderRouterConfig;
    otIp6Address *addrPtr;
    bool stable = false;
    uint8_t flags = 0;

    memset(&borderRouterConfig, 0, sizeof(otBorderRouterConfig));

    VerifyOrExit(mAllowLocalNetworkDataChange == true, spinelError = SPINEL_STATUS_INVALID_STATE);

    parsedLength = spinel_datatype_unpack(
                       aValuePtr,
                       aValueLen,
                       (
                           SPINEL_DATATYPE_IPv6ADDR_S  // On-mesh prefix
                           SPINEL_DATATYPE_UINT8_S     // Prefix length (in bits)
                           SPINEL_DATATYPE_BOOL_S      // Stable
                           SPINEL_DATATYPE_UINT8_S     // Flags
                       ),
                       &addrPtr,
                       &borderRouterConfig.mPrefix.mLength,
                       &stable,
                       &flags
                   );

    VerifyOrExit(parsedLength > 0, spinelError = SPINEL_STATUS_PARSE_ERROR);

    borderRouterConfig.mPrefix.mPrefix = *addrPtr;
    borderRouterConfig.mStable = stable;
    borderRouterConfig.mPreference =
        ((flags & SPINEL_NET_FLAG_PREFERENCE_MASK) >> SPINEL_NET_FLAG_PREFERENCE_OFFSET);
    borderRouterConfig.mPreferred = ((flags & SPINEL_NET_FLAG_PREFERRED) != 0);
    borderRouterConfig.mSlaac = ((flags & SPINEL_NET_FLAG_SLAAC) != 0);
    borderRouterConfig.mDhcp = ((flags & SPINEL_NET_FLAG_DHCP) != 0);
    borderRouterConfig.mConfigure = ((flags & SPINEL_NET_FLAG_CONFIGURE) != 0);
    borderRouterConfig.mDefaultRoute = ((flags & SPINEL_NET_FLAG_DEFAULT_ROUTE) != 0);
    borderRouterConfig.mOnMesh = ((flags & SPINEL_NET_FLAG_ON_MESH) != 0);

    error = otBorderRouterAddOnMeshPrefix(mInstance, &borderRouterConfig);
    VerifyOrExit(error == OT_ERROR_NONE, spinelError = ThreadErrorToSpinelStatus(error));

    error = SendPropertyUpdate(
                aHeader,
                SPINEL_CMD_PROP_VALUE_INSERTED,
                aKey,
                aValuePtr,
                aValueLen
            );

exit:

    if (spinelError != SPINEL_STATUS_OK)
    {
        error = SendLastStatus(aHeader, spinelError);
    }

    return error;
}

otError NcpBase::RemovePropertyHandler_THREAD_ON_MESH_NETS(uint8_t aHeader, spinel_prop_key_t aKey,
                                                           const uint8_t *aValuePtr, uint16_t aValueLen)
{
    spinel_ssize_t parsedLength;
    otError error = OT_ERROR_NONE;
    spinel_status_t spinelError = SPINEL_STATUS_OK;
    otIp6Prefix ip6Prefix;
    otIp6Address *addrPtr;

    memset(&ip6Prefix, 0, sizeof(otIp6Prefix));

    VerifyOrExit(mAllowLocalNetworkDataChange == true, spinelError = SPINEL_STATUS_INVALID_STATE);

    parsedLength = spinel_datatype_unpack(
                       aValuePtr,
                       aValueLen,
                       (
                           SPINEL_DATATYPE_IPv6ADDR_S     // On-mesh prefix
                           SPINEL_DATATYPE_UINT8_S        // Prefix length (in bits)
                       ),
                       &addrPtr,
                       &ip6Prefix.mLength
                   );

    VerifyOrExit(parsedLength > 0, spinelError = SPINEL_STATUS_PARSE_ERROR);

    ip6Prefix.mPrefix = *addrPtr;

    error = otBorderRouterRemoveOnMeshPrefix(mInstance, &ip6Prefix);

    VerifyOrExit(error == OT_ERROR_NONE, spinelError = ThreadErrorToSpinelStatus(error));

    error = SendPropertyUpdate(
                aHeader,
                SPINEL_CMD_PROP_VALUE_REMOVED,
                aKey,
                aValuePtr,
                aValueLen
            );

exit:

    if (spinelError != SPINEL_STATUS_OK)
    {
        error = SendLastStatus(aHeader, spinelError);
    }

    return error;
}
#endif // OPENTHREAD_ENABLE_BORDER_ROUTER

otError NcpBase::GetPropertyHandler_THREAD_DISCOVERY_SCAN_JOINER_FLAG(uint8_t aHeader, spinel_prop_key_t aKey)
{
    return SendPropertyUpdate(
               aHeader,
               SPINEL_CMD_PROP_VALUE_IS,
               aKey,
               SPINEL_DATATYPE_BOOL_S,
               mDiscoveryScanJoinerFlag
           );
}

otError NcpBase::SetPropertyHandler_THREAD_DISCOVERY_SCAN_JOINER_FLAG(uint8_t aHeader, spinel_prop_key_t aKey,
                                                                      const uint8_t *aValuePtr, uint16_t aValueLen)
{
    bool joinerFlag = false;
    spinel_ssize_t parsedLength;
    otError error = OT_ERROR_NONE;

    parsedLength = spinel_datatype_unpack(
                       aValuePtr,
                       aValueLen,
                       SPINEL_DATATYPE_BOOL_S,
                       &joinerFlag
                   );

    VerifyOrExit(parsedLength > 0, error = OT_ERROR_PARSE);

    mDiscoveryScanJoinerFlag = joinerFlag;

exit:
    return SendSetPropertyResponse(aHeader, aKey, error);
}

otError NcpBase::GetPropertyHandler_THREAD_DISCOVERY_SCAN_ENABLE_FILTERING(uint8_t aHeader, spinel_prop_key_t aKey)
{
    return SendPropertyUpdate(
               aHeader,
               SPINEL_CMD_PROP_VALUE_IS,
               aKey,
               SPINEL_DATATYPE_BOOL_S,
               mDiscoveryScanEnableFiltering
           );
}

otError NcpBase::SetPropertyHandler_THREAD_DISCOVERY_SCAN_ENABLE_FILTERING(uint8_t aHeader, spinel_prop_key_t aKey,
                                                                           const uint8_t *aValuePtr,
                                                                           uint16_t aValueLen)
{
    bool enabled = false;
    spinel_ssize_t parsedLength;
    otError error = OT_ERROR_NONE;

    parsedLength = spinel_datatype_unpack(
                       aValuePtr,
                       aValueLen,
                       SPINEL_DATATYPE_BOOL_S,
                       &enabled
                   );

    VerifyOrExit(parsedLength > 0, error = OT_ERROR_PARSE);

    mDiscoveryScanEnableFiltering = enabled;

exit:
    return SendSetPropertyResponse(aHeader, aKey, error);
}

otError NcpBase::GetPropertyHandler_THREAD_DISCOVERY_SCAN_PANID(uint8_t aHeader, spinel_prop_key_t aKey)
{
    return SendPropertyUpdate(
               aHeader,
               SPINEL_CMD_PROP_VALUE_IS,
               aKey,
               SPINEL_DATATYPE_UINT16_S,
               mDiscoveryScanPanId
           );
}

otError NcpBase::SetPropertyHandler_THREAD_DISCOVERY_SCAN_PANID(uint8_t aHeader, spinel_prop_key_t aKey,
                                                                const uint8_t *aValuePtr, uint16_t aValueLen)
{
    uint16_t panid;
    spinel_ssize_t parsedLength;
    otError error = OT_ERROR_NONE;

    parsedLength = spinel_datatype_unpack(
                       aValuePtr,
                       aValueLen,
                       SPINEL_DATATYPE_UINT16_S,
                       &panid
                   );

    VerifyOrExit(parsedLength > 0, error = OT_ERROR_PARSE);

    mDiscoveryScanPanId = panid;

exit:
    return SendSetPropertyResponse(aHeader, aKey, error);
}

otError NcpBase::GetPropertyHandler_IPV6_ML_PREFIX(uint8_t aHeader, spinel_prop_key_t aKey)
{
    otError error = OT_ERROR_NONE;
    const uint8_t *mlPrefix = otThreadGetMeshLocalPrefix(mInstance);

    if (mlPrefix)
    {
        otIp6Address addr;

        memcpy(addr.mFields.m8, mlPrefix, 8);

        // Zero out the last 8 bytes.
        memset(addr.mFields.m8 + 8, 0, 8);

        error = SendPropertyUpdate(
                    aHeader,
                    SPINEL_CMD_PROP_VALUE_IS,
                    aKey,
                    (
                        SPINEL_DATATYPE_IPv6ADDR_S  // Mesh-local IPv6 address
                        SPINEL_DATATYPE_UINT8_S     // Prefix length (in bits)
                    ),
                    &addr,
                    64
                );
    }
    else
    {
        error = SendPropertyUpdate(
                    aHeader,
                    SPINEL_CMD_PROP_VALUE_IS,
                    aKey,
                    SPINEL_DATATYPE_VOID_S
                );
    }

    return error;
}

otError NcpBase::SetPropertyHandler_IPV6_ML_PREFIX(uint8_t aHeader, spinel_prop_key_t aKey, const uint8_t *aValuePtr,
                                                   uint16_t aValueLen)
{
    otError error = OT_ERROR_NONE;

    VerifyOrExit(aValueLen >= 8, error = OT_ERROR_PARSE);

    error = otThreadSetMeshLocalPrefix(mInstance, aValuePtr);

exit:
    return SendSetPropertyResponse(aHeader, aKey, error);
}

otError NcpBase::GetPropertyHandler_IPV6_ML_ADDR(uint8_t aHeader, spinel_prop_key_t aKey)
{
    otError error = OT_ERROR_NONE;
    const otIp6Address *ml64 = otThreadGetMeshLocalEid(mInstance);

    if (ml64)
    {
        error = SendPropertyUpdate(
                    aHeader,
                    SPINEL_CMD_PROP_VALUE_IS,
                    aKey,
                    SPINEL_DATATYPE_IPv6ADDR_S,
                    ml64
                );
    }
    else
    {
        error = SendPropertyUpdate(
                    aHeader,
                    SPINEL_CMD_PROP_VALUE_IS,
                    aKey,
                    SPINEL_DATATYPE_VOID_S
                );
    }

    return error;
}

otError NcpBase::GetPropertyHandler_IPV6_LL_ADDR(uint8_t aHeader, spinel_prop_key_t aKey)
{
    otError error = OT_ERROR_NONE;
    const otIp6Address *address = otThreadGetLinkLocalIp6Address(mInstance);

    if (address)
    {
        error = SendPropertyUpdate(
                    aHeader,
                    SPINEL_CMD_PROP_VALUE_IS,
                    aKey,
                    SPINEL_DATATYPE_IPv6ADDR_S,
                    address
                );
    }
    else
    {
        error = SendPropertyUpdate(
                    aHeader,
                    SPINEL_CMD_PROP_VALUE_IS,
                    aKey,
                    SPINEL_DATATYPE_VOID_S
                );
    }

    return error;
}

otError NcpBase::GetPropertyHandler_IPV6_ADDRESS_TABLE(uint8_t aHeader, spinel_prop_key_t aKey)
{
    otError error = OT_ERROR_NONE;

    mDisableStreamWrite = true;

    SuccessOrExit(error = OutboundFrameBegin(aHeader));
    SuccessOrExit(
        error = OutboundFrameFeedPacked(
                    SPINEL_DATATYPE_COMMAND_PROP_S,
                    aHeader,
                    SPINEL_CMD_PROP_VALUE_IS,
                    aKey
                ));

    for (const otNetifAddress *address = otIp6GetUnicastAddresses(mInstance); address; address = address->mNext)
    {

        SuccessOrExit(
            error = OutboundFrameFeedPacked(
                        SPINEL_DATATYPE_STRUCT_S(
                            SPINEL_DATATYPE_IPv6ADDR_S  // IPv6 address
                            SPINEL_DATATYPE_UINT8_S     // Prefix length (in bits)
                            SPINEL_DATATYPE_UINT32_S    // Preferred lifetime
                            SPINEL_DATATYPE_UINT32_S    // Valid lifetime
                        ),
                        &address->mAddress,
                        address->mPrefixLength,
                        address->mPreferred ? 0xffffffff : 0,
                        address->mValid ? 0xffffffff : 0
                    ));
    }

    SuccessOrExit(error = OutboundFrameSend());

exit:
    mDisableStreamWrite = false;
    return error;
}

otError NcpBase::InsertPropertyHandler_IPV6_ADDRESS_TABLE(uint8_t aHeader, spinel_prop_key_t aKey,
                                                          const uint8_t *aValuePtr, uint16_t aValueLen)
{
    spinel_ssize_t parsedLength;
    otError error = OT_ERROR_NONE;
    spinel_status_t spinelError = SPINEL_STATUS_OK;
    otNetifAddress netifAddr;
    otIp6Address *addrPtr;
    uint32_t preferredLifetime;
    uint32_t validLifetime;
    uint8_t  prefixLen;

    parsedLength = spinel_datatype_unpack(
                       aValuePtr,
                       aValueLen,
                       (
                          SPINEL_DATATYPE_IPv6ADDR_S    // IPv6 address
                          SPINEL_DATATYPE_UINT8_S       // Prefix length (in bits)
                          SPINEL_DATATYPE_UINT32_S      // Preferred lifetime
                          SPINEL_DATATYPE_UINT32_S      // Valid lifetime
                       ),
                       &addrPtr,
                       &prefixLen,
                       &preferredLifetime,
                       &validLifetime
                   );

    VerifyOrExit(parsedLength > 0, spinelError = SPINEL_STATUS_PARSE_ERROR);

    netifAddr.mAddress = *addrPtr;
    netifAddr.mPrefixLength = prefixLen;
    netifAddr.mPreferred = preferredLifetime != 0;
    netifAddr.mValid = validLifetime != 0;

    error = otIp6AddUnicastAddress(mInstance, &netifAddr);

    VerifyOrExit(error == OT_ERROR_NONE, spinelError = ThreadErrorToSpinelStatus(error));

    error = SendPropertyUpdate(
                    aHeader,
                    SPINEL_CMD_PROP_VALUE_INSERTED,
                    aKey,
                    aValuePtr,
                    aValueLen
                );

    spinelError = SPINEL_STATUS_OK;

exit:

    if (spinelError != SPINEL_STATUS_OK)
    {
        error = SendLastStatus(aHeader, spinelError);
    }

    return error;
}

otError NcpBase::RemovePropertyHandler_IPV6_ADDRESS_TABLE(uint8_t aHeader, spinel_prop_key_t aKey,
                                                          const uint8_t *aValuePtr, uint16_t aValueLen)
{
    spinel_ssize_t parsedLength;
    otError error = OT_ERROR_NONE;
    spinel_status_t spinelError = SPINEL_STATUS_OK;
    otIp6Address *addrPtr;

    parsedLength = spinel_datatype_unpack(
                       aValuePtr,
                       aValueLen,
                       SPINEL_DATATYPE_IPv6ADDR_S,
                       &addrPtr
                   );

    VerifyOrExit(parsedLength > 0, spinelError = SPINEL_STATUS_PARSE_ERROR);

    error = otIp6RemoveUnicastAddress(mInstance, addrPtr);
    VerifyOrExit(error == OT_ERROR_NONE, spinelError = ThreadErrorToSpinelStatus(error));

    error = SendPropertyUpdate(
                aHeader,
                SPINEL_CMD_PROP_VALUE_REMOVED,
                aKey,
                aValuePtr,
                aValueLen
            );

exit:

    if (spinelError != SPINEL_STATUS_OK)
    {
        error = SendLastStatus(aHeader, spinelError);
    }

    return error;
}

otError NcpBase::GetPropertyHandler_IPV6_ROUTE_TABLE(uint8_t aHeader, spinel_prop_key_t aKey)
{
    // TODO: Implement get route table
    OT_UNUSED_VARIABLE(aKey);

    return SendLastStatus(aHeader, SPINEL_STATUS_UNIMPLEMENTED);
}

otError NcpBase::GetPropertyHandler_IPV6_ICMP_PING_OFFLOAD(uint8_t aHeader, spinel_prop_key_t aKey)
{
    return SendPropertyUpdate(
               aHeader,
               SPINEL_CMD_PROP_VALUE_IS,
               aKey,
               SPINEL_DATATYPE_BOOL_S,
               otIcmp6IsEchoEnabled(mInstance)
           );
}

otError NcpBase::SetPropertyHandler_IPV6_ICMP_PING_OFFLOAD(uint8_t aHeader, spinel_prop_key_t aKey,
                                                           const uint8_t *aValuePtr, uint16_t aValueLen)
{
    bool enabled = false;
    spinel_ssize_t parsedLength;
    otError error = OT_ERROR_NONE;

    parsedLength = spinel_datatype_unpack(
                       aValuePtr,
                       aValueLen,
                       SPINEL_DATATYPE_BOOL_S,
                       &enabled
                   );

    VerifyOrExit(parsedLength > 0, error = OT_ERROR_PARSE);

    otIcmp6SetEchoEnabled(mInstance, enabled);

exit:
    return SendSetPropertyResponse(aHeader, aKey, error);
}

otError NcpBase::GetPropertyHandler_IPV6_MULTICAST_ADDRESS_TABLE(uint8_t aHeader, spinel_prop_key_t aKey)
{
    otError error = OT_ERROR_NONE;
    const otNetifMulticastAddress *address;

    mDisableStreamWrite = true;

    SuccessOrExit(error = OutboundFrameBegin(aHeader));
    SuccessOrExit(
        error = OutboundFrameFeedPacked(
                    SPINEL_DATATYPE_COMMAND_PROP_S,
                    aHeader,
                    SPINEL_CMD_PROP_VALUE_IS,
                    aKey
                ));

    for (address = otIp6GetMulticastAddresses(mInstance); address; address = address->mNext)
    {
        SuccessOrExit(
            error = OutboundFrameFeedPacked(
                        SPINEL_DATATYPE_STRUCT_S(
                            SPINEL_DATATYPE_IPv6ADDR_S  // IPv6 address
                        ),
                        &address->mAddress
                    ));
    }

    SuccessOrExit(error = OutboundFrameSend());

exit:
    mDisableStreamWrite = false;
    return error;
}

otError NcpBase::InsertPropertyHandler_IPV6_MULTICAST_ADDRESS_TABLE(uint8_t aHeader, spinel_prop_key_t aKey,
                                                                   const uint8_t *aValuePtr, uint16_t aValueLen)
{
    spinel_ssize_t parsedLength;
    otError error = OT_ERROR_NONE;
    spinel_status_t spinelError = SPINEL_STATUS_OK;
    otIp6Address *addrPtr;

    parsedLength = spinel_datatype_unpack(
                       aValuePtr,
                       aValueLen,
                       SPINEL_DATATYPE_IPv6ADDR_S,    // IPv6 address
                       &addrPtr
                   );

    VerifyOrExit(parsedLength > 0, spinelError = SPINEL_STATUS_PARSE_ERROR);

    error = otIp6SubscribeMulticastAddress(mInstance, addrPtr);
    VerifyOrExit(error == OT_ERROR_NONE, spinelError = ThreadErrorToSpinelStatus(error));

    error = SendPropertyUpdate(
                    aHeader,
                    SPINEL_CMD_PROP_VALUE_INSERTED,
                    aKey,
                    aValuePtr,
                    aValueLen
                );

    spinelError = SPINEL_STATUS_OK;

exit:

    if (spinelError != SPINEL_STATUS_OK)
    {
        error = SendLastStatus(aHeader, spinelError);
    }

    return error;
}

otError NcpBase::RemovePropertyHandler_IPV6_MULTICAST_ADDRESS_TABLE(uint8_t aHeader, spinel_prop_key_t aKey,
                                                                    const uint8_t *aValuePtr, uint16_t aValueLen)
{
    spinel_ssize_t parsedLength;
    otError error = OT_ERROR_NONE;
    spinel_status_t spinelError = SPINEL_STATUS_OK;
    otIp6Address *addrPtr;

    parsedLength = spinel_datatype_unpack(
                       aValuePtr,
                       aValueLen,
                       SPINEL_DATATYPE_IPv6ADDR_S,
                       &addrPtr
                   );

    VerifyOrExit(parsedLength > 0, spinelError = SPINEL_STATUS_PARSE_ERROR);

    error = otIp6UnsubscribeMulticastAddress(mInstance, addrPtr);
    VerifyOrExit(error == OT_ERROR_NONE, spinelError = ThreadErrorToSpinelStatus(error));

    error = SendPropertyUpdate(
                aHeader,
                SPINEL_CMD_PROP_VALUE_REMOVED,
                aKey,
                aValuePtr,
                aValueLen
            );

exit:

    if (spinelError != SPINEL_STATUS_OK)
    {
        error = SendLastStatus(aHeader, spinelError);
    }

    return error;
}

otError NcpBase::GetPropertyHandler_THREAD_RLOC16_DEBUG_PASSTHRU(uint8_t aHeader, spinel_prop_key_t aKey)
{
    // Note reverse logic: passthru enabled = filter disabled
    return SendPropertyUpdate(
               aHeader,
               SPINEL_CMD_PROP_VALUE_IS,
               aKey,
               SPINEL_DATATYPE_BOOL_S,
               !otIp6IsReceiveFilterEnabled(mInstance)
           );
}

otError NcpBase::SetPropertyHandler_THREAD_RLOC16_DEBUG_PASSTHRU(uint8_t aHeader, spinel_prop_key_t aKey,
                                                                 const uint8_t *aValuePtr, uint16_t aValueLen)
{
    bool enabled = false;
    spinel_ssize_t parsedLength;
    otError error = OT_ERROR_NONE;

    parsedLength = spinel_datatype_unpack(
                       aValuePtr,
                       aValueLen,
                       SPINEL_DATATYPE_BOOL_S,
                       &enabled
                   );

    VerifyOrExit(parsedLength > 0, error = OT_ERROR_PARSE);

    // Note reverse logic: passthru enabled = filter disabled
    otIp6SetReceiveFilterEnabled(mInstance, !enabled);

exit:
    return SendSetPropertyResponse(aHeader, aKey, error);
}

otError NcpBase::GetPropertyHandler_THREAD_OFF_MESH_ROUTES(uint8_t aHeader, spinel_prop_key_t aKey)
{
    otError error = OT_ERROR_NONE;
    otExternalRouteConfig external_route_config;
    otNetworkDataIterator iter = OT_NETWORK_DATA_ITERATOR_INIT;

    mDisableStreamWrite = true;

    SuccessOrExit(error = OutboundFrameBegin(aHeader));
    SuccessOrExit(
        error = OutboundFrameFeedPacked(
                    SPINEL_DATATYPE_COMMAND_PROP_S,
                    aHeader,
                    SPINEL_CMD_PROP_VALUE_IS,
                    aKey
                ));

    while (otNetDataGetNextRoute(mInstance, &iter, &external_route_config) == OT_ERROR_NONE)
    {
        SuccessOrExit(
            error = OutboundFrameFeedPacked(
                        SPINEL_DATATYPE_STRUCT_S(
                            SPINEL_DATATYPE_IPv6ADDR_S      // IPv6 Prefix
                            SPINEL_DATATYPE_UINT8_S         // Prefix Length (in bits)
                            SPINEL_DATATYPE_BOOL_S          // IsStable
                            SPINEL_DATATYPE_UINT8_S         // Route Preference Flags
                            SPINEL_DATATYPE_BOOL_S          // IsLocal
                            SPINEL_DATATYPE_BOOL_S          // NextHopIsThisDevice
                            SPINEL_DATATYPE_UINT16_S        // RLOC16
                        ),
                        &external_route_config.mPrefix.mPrefix,
                        external_route_config.mPrefix.mLength,
                        external_route_config.mStable,
                        ExternalRoutePreferenceToFlagByte(external_route_config.mPreference),
                        false,
                        external_route_config.mNextHopIsThisDevice,
                        external_route_config.mRloc16
                    ));
    }

#if OPENTHREAD_ENABLE_BORDER_ROUTER
    while (otBorderRouterGetNextRoute(mInstance, &iter, &external_route_config) == OT_ERROR_NONE)
    {
        SuccessOrExit(
            error = OutboundFrameFeedPacked(
                        SPINEL_DATATYPE_STRUCT_S(
                            SPINEL_DATATYPE_IPv6ADDR_S      // IPv6 Prefix
                            SPINEL_DATATYPE_UINT8_S         // Prefix Length (in bits)
                            SPINEL_DATATYPE_BOOL_S          // IsStable
                            SPINEL_DATATYPE_UINT8_S         // Route Preference Flags
                            SPINEL_DATATYPE_BOOL_S          // IsLocal
                            SPINEL_DATATYPE_BOOL_S          // NextHopIsThisDevice
                            SPINEL_DATATYPE_UINT16_S        // RLOC16
                        ),
                        &external_route_config.mPrefix.mPrefix,
                        external_route_config.mPrefix.mLength,
                        external_route_config.mStable,
                        ExternalRoutePreferenceToFlagByte(external_route_config.mPreference),
                        true,
                        external_route_config.mNextHopIsThisDevice,
                        external_route_config.mRloc16
                    ));
    }
#endif // OPENTHREAD_ENABLE_BORDER_ROUTER

    SuccessOrExit(error = OutboundFrameSend());

exit:
    mDisableStreamWrite = false;
    return error;
}

#if OPENTHREAD_ENABLE_BORDER_ROUTER
static int FlagByteToExternalRoutePreference(uint8_t aFlags)
{
    int route_preference = 0;

    switch (aFlags & SPINEL_NET_FLAG_PREFERENCE_MASK)
    {
    case SPINEL_ROUTE_PREFERENCE_HIGH:
        route_preference = OT_ROUTE_PREFERENCE_HIGH;
        break;

    case SPINEL_ROUTE_PREFERENCE_MEDIUM:
        route_preference = OT_ROUTE_PREFERENCE_MED;
        break;

    case SPINEL_ROUTE_PREFERENCE_LOW:
        route_preference = OT_ROUTE_PREFERENCE_LOW;
        break;
    }

    return route_preference;
}

otError NcpBase::InsertPropertyHandler_THREAD_OFF_MESH_ROUTES(uint8_t aHeader, spinel_prop_key_t aKey,
                                                              const uint8_t *aValuePtr, uint16_t aValueLen)
{
    spinel_ssize_t parsedLength;
    otError error = OT_ERROR_NONE;
    spinel_status_t spinelError = SPINEL_STATUS_OK;
    otExternalRouteConfig routeConfig;
    otIp6Address *addrPtr;
    bool stable = false;
    uint8_t flags = 0;

    memset(&routeConfig, 0, sizeof(otExternalRouteConfig));

    VerifyOrExit(mAllowLocalNetworkDataChange == true, spinelError = SPINEL_STATUS_INVALID_STATE);

    parsedLength = spinel_datatype_unpack(
                       aValuePtr,
                       aValueLen,
                       (
                           SPINEL_DATATYPE_IPv6ADDR_S  // Route prefix
                           SPINEL_DATATYPE_UINT8_S     // Prefix length (in bits)
                           SPINEL_DATATYPE_BOOL_S      // Stable
                           SPINEL_DATATYPE_UINT8_S     // Flags (Route Preference)
                       ),
                       &addrPtr,
                       &routeConfig.mPrefix.mLength,
                       &stable,
                       &flags
                   );

    VerifyOrExit(parsedLength > 0, spinelError = SPINEL_STATUS_PARSE_ERROR);

    routeConfig.mPrefix.mPrefix = *addrPtr;
    routeConfig.mStable = stable;
    routeConfig.mPreference = FlagByteToExternalRoutePreference(flags);

    error = otBorderRouterAddRoute(mInstance, &routeConfig);
    VerifyOrExit(error == OT_ERROR_NONE, spinelError = ThreadErrorToSpinelStatus(error));

    error = SendPropertyUpdate(
                aHeader,
                SPINEL_CMD_PROP_VALUE_INSERTED,
                aKey,
                aValuePtr,
                aValueLen
            );

exit:

    if (spinelError != SPINEL_STATUS_OK)
    {
        error = SendLastStatus(aHeader, spinelError);
    }

    return error;
}

otError NcpBase::RemovePropertyHandler_THREAD_OFF_MESH_ROUTES(uint8_t aHeader, spinel_prop_key_t aKey,
                                                              const uint8_t *aValuePtr, uint16_t aValueLen)
{
    spinel_ssize_t parsedLength;
    otError error = OT_ERROR_NONE;
    spinel_status_t spinelError = SPINEL_STATUS_OK;
    otIp6Prefix ip6Prefix;
    otIp6Address *addrPtr;

    memset(&ip6Prefix, 0, sizeof(otIp6Prefix));

    VerifyOrExit(mAllowLocalNetworkDataChange == true, spinelError = SPINEL_STATUS_INVALID_STATE);

    parsedLength = spinel_datatype_unpack(
                       aValuePtr,
                       aValueLen,
                       (
                           SPINEL_DATATYPE_IPv6ADDR_S     // Route prefix
                           SPINEL_DATATYPE_UINT8_S        // Prefix length (in bits)
                       ),
                       &addrPtr,
                       &ip6Prefix.mLength
                   );

    VerifyOrExit(parsedLength > 0, spinelError = SPINEL_STATUS_PARSE_ERROR);

    ip6Prefix.mPrefix = *addrPtr;

    error = otBorderRouterRemoveRoute(mInstance, &ip6Prefix);
    VerifyOrExit(error == OT_ERROR_NONE, spinelError = ThreadErrorToSpinelStatus(error));

    error = SendPropertyUpdate(
                aHeader,
                SPINEL_CMD_PROP_VALUE_REMOVED,
                aKey,
                aValuePtr,
                aValueLen
            );

exit:

    if (spinelError != SPINEL_STATUS_OK)
    {
        error = SendLastStatus(aHeader, spinelError);
    }

    return error;
}
#endif // OPENTHREAD_ENABLE_BORDER_ROUTER

otError NcpBase::GetPropertyHandler_STREAM_NET(uint8_t aHeader, spinel_prop_key_t aKey)
{
    // TODO: Implement explicit data poll.
    OT_UNUSED_VARIABLE(aKey);

    return SendLastStatus(aHeader, SPINEL_STATUS_UNIMPLEMENTED);
}

otError NcpBase::SetPropertyHandler_STREAM_NET(uint8_t aHeader, spinel_prop_key_t aKey, const uint8_t *aValuePtr,
                                               uint16_t aValueLen)
{
    const uint8_t *framePtr = NULL;
    unsigned int frameLen = 0;
    const uint8_t *metaPtr = NULL;
    unsigned int metaLen = 0;
    otMessage *message;
    spinel_ssize_t parsedLength;
    otError error = OT_ERROR_NONE;

    // STREAM_NET requires layer 2 security.
    message = otIp6NewMessage(mInstance, true);
    VerifyOrExit(message != NULL, error = OT_ERROR_NO_BUFS);

    parsedLength = spinel_datatype_unpack(
                       aValuePtr,
                       aValueLen,
                       (
                           SPINEL_DATATYPE_DATA_WLEN_S  // Frame data
                           SPINEL_DATATYPE_DATA_S       // Meta data
                       ),
                       &framePtr,
                       &frameLen,
                       &metaPtr,
                       &metaLen
                   );

    // We ignore metadata for now.
    // May later include TX power, allow retransmits, etc...
    OT_UNUSED_VARIABLE(metaPtr);
    OT_UNUSED_VARIABLE(metaLen);
    OT_UNUSED_VARIABLE(parsedLength);

    SuccessOrExit(error = otMessageAppend(message, framePtr, static_cast<uint16_t>(frameLen)));

    error = otIp6Send(mInstance, message);

    // `otIp6Send()` takes ownership of `message` (in both success or
    // failure cases). `message` is set to NULL so it is not freed at
    // exit.
    message = NULL;

exit:

    if (message != NULL)
    {
        otMessageFree(message);
    }

    if (error == OT_ERROR_NONE)
    {
        mInboundSecureIpFrameCounter++;

        if (SPINEL_HEADER_GET_TID(aHeader) != 0)
        {
            // Only send a successful status update if
            // there was a transaction id in the aHeader.
            error = SendLastStatus(aHeader, SPINEL_STATUS_OK);
        }
    }
    else
    {
        mDroppedInboundIpFrameCounter++;

        error = SendLastStatus(aHeader, ThreadErrorToSpinelStatus(error));
    }

    OT_UNUSED_VARIABLE(aKey);

    return error;
}

#if OPENTHREAD_ENABLE_JAM_DETECTION

otError NcpBase::GetPropertyHandler_JAM_DETECT_ENABLE(uint8_t aHeader, spinel_prop_key_t aKey)
{
    return SendPropertyUpdate(
               aHeader,
               SPINEL_CMD_PROP_VALUE_IS,
               aKey,
               SPINEL_DATATYPE_BOOL_S,
               otJamDetectionIsEnabled(mInstance)
           );
}

otError NcpBase::GetPropertyHandler_JAM_DETECTED(uint8_t aHeader, spinel_prop_key_t aKey)
{
    return SendPropertyUpdate(
               aHeader,
               SPINEL_CMD_PROP_VALUE_IS,
               aKey,
               SPINEL_DATATYPE_BOOL_S,
               otJamDetectionGetState(mInstance)
           );
}

otError NcpBase::GetPropertyHandler_JAM_DETECT_RSSI_THRESHOLD(uint8_t aHeader, spinel_prop_key_t aKey)
{
    return SendPropertyUpdate(
               aHeader,
               SPINEL_CMD_PROP_VALUE_IS,
               aKey,
               SPINEL_DATATYPE_INT8_S,
               otJamDetectionGetRssiThreshold(mInstance)
           );
}

otError NcpBase::GetPropertyHandler_JAM_DETECT_WINDOW(uint8_t aHeader, spinel_prop_key_t aKey)
{
    return SendPropertyUpdate(
               aHeader,
               SPINEL_CMD_PROP_VALUE_IS,
               aKey,
               SPINEL_DATATYPE_UINT8_S,
               otJamDetectionGetWindow(mInstance)
           );
}

otError NcpBase::GetPropertyHandler_JAM_DETECT_BUSY(uint8_t aHeader, spinel_prop_key_t aKey)
{
    return SendPropertyUpdate(
               aHeader,
               SPINEL_CMD_PROP_VALUE_IS,
               aKey,
               SPINEL_DATATYPE_UINT8_S,
               otJamDetectionGetBusyPeriod(mInstance)
           );
}

otError NcpBase::GetPropertyHandler_JAM_DETECT_HISTORY_BITMAP(uint8_t aHeader, spinel_prop_key_t aKey)
{
    uint64_t historyBitmap = otJamDetectionGetHistoryBitmap(mInstance);

    return SendPropertyUpdate(
               aHeader,
               SPINEL_CMD_PROP_VALUE_IS,
               aKey,
               (
                   SPINEL_DATATYPE_UINT32_S   // History bitmap - bits 0-31
                   SPINEL_DATATYPE_UINT32_S   // History bitmap - bits 32-63
               ),
               static_cast<uint32_t>(historyBitmap & 0xffffffff),
               static_cast<uint32_t>(historyBitmap >> 32)
           );
}

otError NcpBase::SetPropertyHandler_JAM_DETECT_ENABLE(uint8_t aHeader, spinel_prop_key_t aKey,
                                                      const uint8_t *aValuePtr, uint16_t aValueLen)
{
    bool enabled;
    spinel_ssize_t parsedLength;
    otError error = OT_ERROR_NONE;

    parsedLength = spinel_datatype_unpack(
                       aValuePtr,
                       aValueLen,
                       SPINEL_DATATYPE_BOOL_S,
                       &enabled
                   );

    VerifyOrExit(parsedLength > 0, error = OT_ERROR_PARSE);

    if (enabled)
    {
        otJamDetectionStart(mInstance, &NcpBase::HandleJamStateChange_Jump, this);
    }
    else
    {
        otJamDetectionStop(mInstance);
    }

exit:
    return SendSetPropertyResponse(aHeader, aKey, error);
}

otError NcpBase::SetPropertyHandler_JAM_DETECT_RSSI_THRESHOLD(uint8_t aHeader, spinel_prop_key_t aKey,
                                                              const uint8_t *aValuePtr, uint16_t aValueLen)
{
    int8_t threshold = 0;
    spinel_ssize_t parsedLength;
    otError error = OT_ERROR_NONE;

    parsedLength = spinel_datatype_unpack(
                       aValuePtr,
                       aValueLen,
                       SPINEL_DATATYPE_INT8_S,
                       &threshold
                   );

    VerifyOrExit(parsedLength > 0, error = OT_ERROR_PARSE);

    error = otJamDetectionSetRssiThreshold(mInstance, threshold);

exit:
    return SendSetPropertyResponse(aHeader, aKey, error);
}

otError NcpBase::SetPropertyHandler_JAM_DETECT_WINDOW(uint8_t aHeader, spinel_prop_key_t aKey,
                                                      const uint8_t *aValuePtr, uint16_t aValueLen)
{
    uint8_t window = 0;
    spinel_ssize_t parsedLength;
    otError error = OT_ERROR_NONE;

    parsedLength = spinel_datatype_unpack(
                       aValuePtr,
                       aValueLen,
                       SPINEL_DATATYPE_UINT8_S,
                       &window
                   );

    VerifyOrExit(parsedLength > 0, error = OT_ERROR_PARSE);

    error = otJamDetectionSetWindow(mInstance, window);

exit:
    return SendSetPropertyResponse(aHeader, aKey, error);
}

otError NcpBase::SetPropertyHandler_JAM_DETECT_BUSY(uint8_t aHeader, spinel_prop_key_t aKey, const uint8_t *aValuePtr,
                                                    uint16_t aValueLen)
{
    uint8_t busy = 0;
    spinel_ssize_t parsedLength;
    otError error = OT_ERROR_NONE;

    parsedLength = spinel_datatype_unpack(
                       aValuePtr,
                       aValueLen,
                       SPINEL_DATATYPE_UINT8_S,
                       &busy
                   );

    VerifyOrExit(parsedLength > 0, error = OT_ERROR_PARSE);

    error = otJamDetectionSetBusyPeriod(mInstance, busy);

exit:
    return SendSetPropertyResponse(aHeader, aKey, error);
}

void NcpBase::HandleJamStateChange_Jump(bool aJamState, void *aContext)
{
    static_cast<NcpBase *>(aContext)->HandleJamStateChange(aJamState);
}

void NcpBase::HandleJamStateChange(bool aJamState)
{
    OT_UNUSED_VARIABLE(aJamState);

    mChangedPropsSet.AddProperty(SPINEL_PROP_JAM_DETECTED);
    mUpdateChangedPropsTask.Post();
}

#endif // OPENTHREAD_ENABLE_JAM_DETECTION

otError NcpBase::GetPropertyHandler_MAC_CNTR(uint8_t aHeader, spinel_prop_key_t aKey)
{
    uint32_t value;
    const otMacCounters *macCounters;
    otError error = OT_ERROR_NONE;

    macCounters = otLinkGetCounters(mInstance);

    assert(macCounters != NULL);

    switch (aKey)
    {
    case SPINEL_PROP_CNTR_TX_PKT_TOTAL:
        value = macCounters->mTxTotal;
        break;

    case SPINEL_PROP_CNTR_TX_PKT_ACK_REQ:
        value = macCounters->mTxAckRequested;
        break;

    case SPINEL_PROP_CNTR_TX_PKT_ACKED:
        value = macCounters->mTxAcked;
        break;

    case SPINEL_PROP_CNTR_TX_PKT_NO_ACK_REQ:
        value = macCounters->mTxNoAckRequested;
        break;

    case SPINEL_PROP_CNTR_TX_PKT_DATA:
        value = macCounters->mTxData;
        break;

    case SPINEL_PROP_CNTR_TX_PKT_DATA_POLL:
        value = macCounters->mTxDataPoll;
        break;

    case SPINEL_PROP_CNTR_TX_PKT_BEACON:
        value = macCounters->mTxBeacon;
        break;

    case SPINEL_PROP_CNTR_TX_PKT_BEACON_REQ:
        value = macCounters->mTxBeaconRequest;
        break;

    case SPINEL_PROP_CNTR_TX_PKT_OTHER:
        value = macCounters->mTxOther;
        break;

    case SPINEL_PROP_CNTR_TX_PKT_RETRY:
        value = macCounters->mTxRetry;
        break;

    case SPINEL_PROP_CNTR_TX_ERR_CCA:
        value = macCounters->mTxErrCca;
        break;

    case SPINEL_PROP_CNTR_TX_PKT_UNICAST:
        value = macCounters->mTxUnicast;
        break;

    case SPINEL_PROP_CNTR_TX_PKT_BROADCAST:
        value = macCounters->mTxBroadcast;
        break;

    case SPINEL_PROP_CNTR_TX_ERR_ABORT:
        value = macCounters->mTxErrAbort;
        break;

    case SPINEL_PROP_CNTR_RX_PKT_TOTAL:
        value = macCounters->mRxTotal;
        break;

    case SPINEL_PROP_CNTR_RX_PKT_DATA:
        value = macCounters->mRxData;
        break;

    case SPINEL_PROP_CNTR_RX_PKT_DATA_POLL:
        value = macCounters->mRxDataPoll;
        break;

    case SPINEL_PROP_CNTR_RX_PKT_BEACON:
        value = macCounters->mRxBeacon;
        break;

    case SPINEL_PROP_CNTR_RX_PKT_BEACON_REQ:
        value = macCounters->mRxBeaconRequest;
        break;

    case SPINEL_PROP_CNTR_RX_PKT_OTHER:
        value = macCounters->mRxOther;
        break;

    case SPINEL_PROP_CNTR_RX_PKT_FILT_WL:
        value = macCounters->mRxWhitelistFiltered;
        break;

    case SPINEL_PROP_CNTR_RX_PKT_FILT_DA:
        value = macCounters->mRxDestAddrFiltered;
        break;

    case SPINEL_PROP_CNTR_RX_PKT_DUP:
        value = macCounters->mRxDuplicated;
        break;

    case SPINEL_PROP_CNTR_RX_PKT_UNICAST:
        value = macCounters->mRxUnicast;
        break;

    case SPINEL_PROP_CNTR_RX_PKT_BROADCAST:
        value = macCounters->mRxBroadcast;
        break;

    case SPINEL_PROP_CNTR_RX_ERR_EMPTY:
        value = macCounters->mRxErrNoFrame;
        break;

    case SPINEL_PROP_CNTR_RX_ERR_UKWN_NBR:
        value = macCounters->mRxErrUnknownNeighbor;
        break;

    case SPINEL_PROP_CNTR_RX_ERR_NVLD_SADDR:
        value = macCounters->mRxErrInvalidSrcAddr;
        break;

    case SPINEL_PROP_CNTR_RX_ERR_SECURITY:
        value = macCounters->mRxErrSec;
        break;

    case SPINEL_PROP_CNTR_RX_ERR_BAD_FCS:
        value = macCounters->mRxErrFcs;
        break;

    case SPINEL_PROP_CNTR_RX_ERR_OTHER:
        value = macCounters->mRxErrOther;
        break;

    default:
        error = SendLastStatus(aHeader, SPINEL_STATUS_INTERNAL_ERROR);
        ExitNow();
    }

    error = SendPropertyUpdate(
                aHeader,
                SPINEL_CMD_PROP_VALUE_IS,
                aKey,
                SPINEL_DATATYPE_UINT32_S,
                value
            );

exit:
    return error;
}

otError NcpBase::GetPropertyHandler_NCP_CNTR(uint8_t aHeader, spinel_prop_key_t aKey)
{
    uint32_t value;
    otError error = OT_ERROR_NONE;

    switch (aKey)
    {
    case SPINEL_PROP_CNTR_TX_IP_SEC_TOTAL:
        value = mInboundSecureIpFrameCounter;
        break;

    case SPINEL_PROP_CNTR_TX_IP_INSEC_TOTAL:
        value = mInboundInsecureIpFrameCounter;
        break;

    case SPINEL_PROP_CNTR_TX_IP_DROPPED:
        value = mDroppedInboundIpFrameCounter;
        break;

    case SPINEL_PROP_CNTR_RX_IP_SEC_TOTAL:
        value = mOutboundSecureIpFrameCounter;
        break;

    case SPINEL_PROP_CNTR_RX_IP_INSEC_TOTAL:
        value = mOutboundInsecureIpFrameCounter;
        break;

    case SPINEL_PROP_CNTR_RX_IP_DROPPED:
        value = mDroppedOutboundIpFrameCounter;
        break;

    case SPINEL_PROP_CNTR_TX_SPINEL_TOTAL:
        value = mTxSpinelFrameCounter;
        break;

    case SPINEL_PROP_CNTR_RX_SPINEL_TOTAL:
        value = mRxSpinelFrameCounter;
        break;

    case SPINEL_PROP_CNTR_RX_SPINEL_OUT_OF_ORDER_TID:
        value = mRxSpinelOutOfOrderTidCounter;
        break;

    case SPINEL_PROP_CNTR_RX_SPINEL_ERR:
        value = mFramingErrorCounter;
        break;

    default:
        error = SendLastStatus(aHeader, SPINEL_STATUS_INTERNAL_ERROR);
        ExitNow();
    }

    error = SendPropertyUpdate(
                aHeader,
                SPINEL_CMD_PROP_VALUE_IS,
                aKey,
                SPINEL_DATATYPE_UINT32_S,
                value
            );

exit:
    return error;
}

otError NcpBase::GetPropertyHandler_IP_CNTR(uint8_t aHeader, spinel_prop_key_t aKey)
{
    uint32_t value;
    otError error = OT_ERROR_NONE;

    const otIpCounters *counters = otThreadGetIp6Counters(mInstance);

    switch (aKey)
    {
    case SPINEL_PROP_CNTR_IP_TX_SUCCESS:
        value = counters->mTxSuccess;
        break;

    case SPINEL_PROP_CNTR_IP_RX_SUCCESS:
        value = counters->mRxSuccess;
        break;

    case SPINEL_PROP_CNTR_IP_TX_FAILURE:
        value = counters->mTxFailure;
        break;

    case SPINEL_PROP_CNTR_IP_RX_FAILURE:
        value = counters->mRxFailure;
        break;

    default:
        error = SendLastStatus(aHeader, SPINEL_STATUS_INTERNAL_ERROR);
        ExitNow();
        break;
    }

    error = SendPropertyUpdate(
                aHeader,
                SPINEL_CMD_PROP_VALUE_IS,
                aKey,
                SPINEL_DATATYPE_UINT32_S,
                value
            );

exit:
    return error;
}

otError NcpBase::GetPropertyHandler_MSG_BUFFER_COUNTERS(uint8_t aHeader, spinel_prop_key_t aKey)
{
    otError error = OT_ERROR_NONE;
    otBufferInfo bufferInfo;

    otMessageGetBufferInfo(mInstance, &bufferInfo);

    SuccessOrExit(error = OutboundFrameBegin(aHeader));
    SuccessOrExit(
        error = OutboundFrameFeedPacked(
                    SPINEL_DATATYPE_COMMAND_PROP_S,
                    aHeader,
                    SPINEL_CMD_PROP_VALUE_IS,
                    aKey
                ));

    SuccessOrExit(
        error = OutboundFrameFeedPacked(
                    (
                        SPINEL_DATATYPE_UINT16_S    // Total buffers
                        SPINEL_DATATYPE_UINT16_S    // Free buffers
                        SPINEL_DATATYPE_UINT16_S    // Lowpan send messages
                        SPINEL_DATATYPE_UINT16_S    // Lowpan send buffers
                        SPINEL_DATATYPE_UINT16_S    // Lowpan reassembly messages
                        SPINEL_DATATYPE_UINT16_S    // Lowpan reassembly buffers
                        SPINEL_DATATYPE_UINT16_S    // Ip6 messages
                        SPINEL_DATATYPE_UINT16_S    // Ip6 buffers
                        SPINEL_DATATYPE_UINT16_S    // Mpl messages
                        SPINEL_DATATYPE_UINT16_S    // Mpl buffers
                        SPINEL_DATATYPE_UINT16_S    // Mle messages
                        SPINEL_DATATYPE_UINT16_S    // Mle buffers
                        SPINEL_DATATYPE_UINT16_S    // Arp messages
                        SPINEL_DATATYPE_UINT16_S    // Arp buffers
                        SPINEL_DATATYPE_UINT16_S    // Coap messages
                        SPINEL_DATATYPE_UINT16_S    // Coap buffers
                    ),
                    bufferInfo.mTotalBuffers,
                    bufferInfo.mFreeBuffers,
                    bufferInfo.m6loSendMessages,
                    bufferInfo.m6loSendBuffers,
                    bufferInfo.m6loReassemblyMessages,
                    bufferInfo.m6loReassemblyBuffers,
                    bufferInfo.mIp6Messages,
                    bufferInfo.mIp6Buffers,
                    bufferInfo.mMplMessages,
                    bufferInfo.mMplBuffers,
                    bufferInfo.mMleMessages,
                    bufferInfo.mMleBuffers,
                    bufferInfo.mArpMessages,
                    bufferInfo.mArpBuffers,
                    bufferInfo.mCoapMessages,
                    bufferInfo.mCoapBuffers
                ));

    SuccessOrExit(error = OutboundFrameSend());

exit:
    return error;
}

#if OPENTHREAD_ENABLE_MAC_FILTER

otError NcpBase::GetPropertyHandler_MAC_WHITELIST(uint8_t aHeader, spinel_prop_key_t aKey)
{
    otMacFilterEntry entry;
    otMacFilterIterator iterator = OT_MAC_FILTER_ITERATOR_INIT;
    otError error = OT_ERROR_NONE;

    mDisableStreamWrite = true;

    SuccessOrExit(error = OutboundFrameBegin(aHeader));
    SuccessOrExit(
        error = OutboundFrameFeedPacked(
                    SPINEL_DATATYPE_COMMAND_PROP_S,
                    aHeader,
                    SPINEL_CMD_PROP_VALUE_IS,
                    aKey
                ));

    while (otLinkFilterGetNextAddress(mInstance, &iterator, &entry) == OT_ERROR_NONE)
    {
        SuccessOrExit(
            error = OutboundFrameFeedPacked(
                SPINEL_DATATYPE_STRUCT_S(
                    SPINEL_DATATYPE_EUI64_S   // Extended address
                    SPINEL_DATATYPE_INT8_S    // Rss
                ),
                entry.mExtAddress.m8,
                entry.mRssIn
            ));
    }

    SuccessOrExit(error = OutboundFrameSend());

exit:
    mDisableStreamWrite = false;
    return error;
}

otError NcpBase::GetPropertyHandler_MAC_WHITELIST_ENABLED(uint8_t aHeader, spinel_prop_key_t aKey)
{
    otMacFilterAddressMode mode = otLinkFilterGetAddressMode(mInstance);
    bool isEnabled = (mode == OT_MAC_FILTER_ADDRESS_MODE_WHITELIST) ? true : false;

    return SendPropertyUpdate(
               aHeader,
               SPINEL_CMD_PROP_VALUE_IS,
               aKey,
               SPINEL_DATATYPE_BOOL_S,
               isEnabled
           );
}

otError NcpBase::GetPropertyHandler_MAC_BLACKLIST(uint8_t aHeader, spinel_prop_key_t aKey)
{
    otMacFilterEntry entry;
    otMacFilterIterator iterator = OT_MAC_FILTER_ITERATOR_INIT;
    otError error = OT_ERROR_NONE;

    mDisableStreamWrite = true;

    SuccessOrExit(error = OutboundFrameBegin(aHeader));
    SuccessOrExit(
        error = OutboundFrameFeedPacked(
                    SPINEL_DATATYPE_COMMAND_PROP_S,
                    aHeader,
                    SPINEL_CMD_PROP_VALUE_IS,
                    aKey
                ));

    while (otLinkFilterGetNextAddress(mInstance, &iterator, &entry) == OT_ERROR_NONE)
    {
        SuccessOrExit(
            error = OutboundFrameFeedPacked(
                        SPINEL_DATATYPE_STRUCT_S(
                            SPINEL_DATATYPE_EUI64_S   // Extended address
                        ),
                        entry.mExtAddress.m8
                    ));
    }

    SuccessOrExit(error = OutboundFrameSend());

exit:
    mDisableStreamWrite = false;
    return error;
}

otError NcpBase::GetPropertyHandler_MAC_BLACKLIST_ENABLED(uint8_t aHeader, spinel_prop_key_t aKey)
{
    otMacFilterAddressMode mode = otLinkFilterGetAddressMode(mInstance);
    bool isEnabled = (mode == OT_MAC_FILTER_ADDRESS_MODE_BLACKLIST) ? true : false;

    return SendPropertyUpdate(
               aHeader,
               SPINEL_CMD_PROP_VALUE_IS,
               aKey,
               SPINEL_DATATYPE_BOOL_S,
               isEnabled
           );
}

otError NcpBase::GetPropertyHandler_MAC_FIXED_RSS(uint8_t aHeader, spinel_prop_key_t aKey)
{
    otMacFilterEntry entry;
    otMacFilterIterator iterator = OT_MAC_FILTER_ITERATOR_INIT;
    otError error = OT_ERROR_NONE;

    mDisableStreamWrite = true;

    SuccessOrExit(error = OutboundFrameBegin(aHeader));
    SuccessOrExit(
        error = OutboundFrameFeedPacked(
                    SPINEL_DATATYPE_COMMAND_PROP_S,
                    aHeader,
                    SPINEL_CMD_PROP_VALUE_IS,
                    aKey
                ));

    while (otLinkFilterGetNextRssIn(mInstance, &iterator, &entry) == OT_ERROR_NONE)
    {
        SuccessOrExit(
            error = OutboundFrameFeedPacked(
                        SPINEL_DATATYPE_STRUCT_S(
                            SPINEL_DATATYPE_EUI64_S   // Extended address
                            SPINEL_DATATYPE_INT8_S    // Rss
                        ),
                        entry.mExtAddress.m8,
                        entry.mRssIn
                    ));
    }

    SuccessOrExit(error = OutboundFrameSend());

exit:
    mDisableStreamWrite = false;
    return error;
}

otError NcpBase::SetPropertyHandler_MAC_WHITELIST(uint8_t aHeader, spinel_prop_key_t aKey, const uint8_t *aValuePtr,
                                                  uint16_t aValueLen)
{
    bool reportAsync;
    spinel_ssize_t parsedLength;
    otError error = OT_ERROR_NONE;

    // First, clear the address filter entries.
    otLinkFilterClearAddresses(mInstance);

    while (aValueLen > 0)
    {
        otExtAddress *extAddress = NULL;
        int8_t rss;

        parsedLength = spinel_datatype_unpack(
                           aValuePtr,
                           aValueLen,
                           SPINEL_DATATYPE_STRUCT_S(
                               SPINEL_DATATYPE_EUI64_S
                               SPINEL_DATATYPE_INT8_S
                           ),
                           &extAddress,
                           &rss
                       );

        if (parsedLength <= 0)
        {
            rss = OT_MAC_FILTER_FIXED_RSS_DISABLED;
            parsedLength = spinel_datatype_unpack(
                               aValuePtr,
                               aValueLen,
                               SPINEL_DATATYPE_STRUCT_S(
                                   SPINEL_DATATYPE_EUI64_S
                               ),
                               &extAddress
                           );
        }

        VerifyOrExit(parsedLength > 0, error = OT_ERROR_PARSE);

        error = otLinkFilterAddAddress(mInstance, extAddress);

        VerifyOrExit(error == OT_ERROR_NONE || error == OT_ERROR_ALREADY);

        if (rss != OT_MAC_FILTER_FIXED_RSS_DISABLED)
        {
            SuccessOrExit(error = otLinkFilterAddRssIn(mInstance, extAddress, rss));
        }

        aValuePtr += parsedLength;
        aValueLen -= parsedLength;
    }

exit:
    // If we had an error, we may have actually changed
    // the state of the whitelist---so we need to report
    // those incomplete changes via an asynchronous
    // change event.
    reportAsync = (error != OT_ERROR_NONE && error != OT_ERROR_ALREADY);

    error = SendSetPropertyResponse(aHeader, aKey, error);

    if (reportAsync)
    {
        HandleCommandPropertyGet(SPINEL_HEADER_FLAG | SPINEL_HEADER_IID_0, aKey);
    }

    return error;
}

otError NcpBase::SetPropertyHandler_MAC_WHITELIST_ENABLED(uint8_t aHeader, spinel_prop_key_t aKey,
                                                          const uint8_t *aValuePtr, uint16_t aValueLen)
{
    bool enabled;
    spinel_ssize_t parsedLength;
    otError error = OT_ERROR_NONE;
    otMacFilterAddressMode mode = OT_MAC_FILTER_ADDRESS_MODE_DISABLED;

    parsedLength = spinel_datatype_unpack(
                       aValuePtr,
                       aValueLen,
                       SPINEL_DATATYPE_BOOL_S,
                       &enabled
                   );

    VerifyOrExit(parsedLength > 0, error = OT_ERROR_PARSE);

    if (enabled)
    {
        mode = OT_MAC_FILTER_ADDRESS_MODE_WHITELIST;
    }

    error = otLinkFilterSetAddressMode(mInstance, mode);

exit:
    return SendSetPropertyResponse(aHeader, aKey, error);
}

otError NcpBase::SetPropertyHandler_MAC_BLACKLIST(uint8_t aHeader, spinel_prop_key_t aKey, const uint8_t *aValuePtr,
                                                  uint16_t aValueLen)
{
    bool reportAsync;
    spinel_ssize_t parsedLength;
    otError error = OT_ERROR_NONE;

    // First, clear the address filter entries.
    otLinkFilterClearAddresses(mInstance);

    while (aValueLen > 0)
    {
        otExtAddress *extAddress = NULL;

        parsedLength = spinel_datatype_unpack(
                           aValuePtr,
                           aValueLen,
                           SPINEL_DATATYPE_STRUCT_S(
                               SPINEL_DATATYPE_EUI64_S
                           ),
                           &extAddress
                       );

        VerifyOrExit(parsedLength > 0, error = OT_ERROR_PARSE);

        error = otLinkFilterRemoveAddress(mInstance, extAddress);

        VerifyOrExit(error == OT_ERROR_NONE || error == OT_ERROR_ALREADY);

        aValuePtr += parsedLength;
        aValueLen -= parsedLength;
    }

exit:
    // If we had an error, we may have actually changed
    // the state of the blacklist---so we need to report
    // those incomplete changes via an asynchronous
    // change event.
    reportAsync = (error != OT_ERROR_NONE && error != OT_ERROR_ALREADY);

    error = SendSetPropertyResponse(aHeader, aKey, error);

    if (reportAsync)
    {
       HandleCommandPropertyGet(SPINEL_HEADER_FLAG | SPINEL_HEADER_IID_0, aKey);
    }

    return error;
}

otError NcpBase::SetPropertyHandler_MAC_BLACKLIST_ENABLED(uint8_t aHeader, spinel_prop_key_t aKey,
                                                          const uint8_t *aValuePtr, uint16_t aValueLen)
{
    bool enabled;
    spinel_ssize_t parsedLength;
    otError error = OT_ERROR_NONE;
    otMacFilterAddressMode mode = OT_MAC_FILTER_ADDRESS_MODE_DISABLED;

    parsedLength = spinel_datatype_unpack(
                       aValuePtr,
                       aValueLen,
                       SPINEL_DATATYPE_BOOL_S,
                       &enabled
                   );

    VerifyOrExit(parsedLength > 0, error = OT_ERROR_PARSE);

    if (enabled)
    {
        mode = OT_MAC_FILTER_ADDRESS_MODE_BLACKLIST;
    }

    error = otLinkFilterSetAddressMode(mInstance, mode);

exit:
    return SendSetPropertyResponse(aHeader, aKey, error);
}

otError NcpBase::SetPropertyHandler_MAC_FIXED_RSS(uint8_t aHeader, spinel_prop_key_t aKey, const uint8_t *aValuePtr,
                                                  uint16_t aValueLen)
{
    bool reportAsync;
    spinel_ssize_t parsedLength;
    otError error = OT_ERROR_NONE;

    // First, clear the address filter entries.
    otLinkFilterClearRssIn(mInstance);

    while (aValueLen > 0)
    {
        otExtAddress *extAddress;
        int8_t rss;

        parsedLength = spinel_datatype_unpack(
                aValuePtr,
                aValueLen,
                SPINEL_DATATYPE_STRUCT_S(
                    SPINEL_DATATYPE_EUI64_S
                    SPINEL_DATATYPE_INT8_S
                    ),
                &extAddress,
                &rss
                );

        if (parsedLength < 0)
        {
            extAddress = NULL;
            parsedLength = spinel_datatype_unpack(
                    aValuePtr,
                    aValueLen,
                    SPINEL_DATATYPE_STRUCT_S(
                        SPINEL_DATATYPE_INT8_S
                        ),
                    &rss
                    );
        }

        VerifyOrExit(parsedLength > 0, error = OT_ERROR_PARSE);

        SuccessOrExit(error = otLinkFilterAddRssIn(mInstance, extAddress, rss));

        aValuePtr += parsedLength;
        aValueLen -= parsedLength;
    }

exit:
    // If we had an error, we may have actually changed
    // the state of the RssIn filter---so we need to report
    // those incomplete changes via an asynchronous
    // change event.
    reportAsync = (error != OT_ERROR_NONE);

    error = SendSetPropertyResponse(aHeader, aKey, error);

    if (reportAsync)
    {
        HandleCommandPropertyGet(SPINEL_HEADER_FLAG | SPINEL_HEADER_IID_0, aKey);
    }

    return error;
}
#endif // OPENTHREAD_ENABLE_MAC_FILTER

otError NcpBase::GetPropertyHandler_THREAD_MODE(uint8_t aHeader, spinel_prop_key_t aKey)
{
    uint8_t numericMode;
    otLinkModeConfig modeConfig = otThreadGetLinkMode(mInstance);

    numericMode = LinkFlagsToFlagByte(
                     modeConfig.mRxOnWhenIdle,
                     modeConfig.mSecureDataRequests,
                     modeConfig.mDeviceType,
                     modeConfig.mNetworkData
                  );

    return SendPropertyUpdate(
               aHeader,
               SPINEL_CMD_PROP_VALUE_IS,
               aKey,
               SPINEL_DATATYPE_UINT8_S,
               numericMode
           );
}

otError NcpBase::SetPropertyHandler_THREAD_MODE(uint8_t aHeader, spinel_prop_key_t aKey, const uint8_t *aValuePtr,
                                                uint16_t aValueLen)
{
    uint8_t numericMode = 0;
    otLinkModeConfig modeConfig;
    spinel_ssize_t parsedLength;
    otError error = OT_ERROR_NONE;

    parsedLength = spinel_datatype_unpack(
                       aValuePtr,
                       aValueLen,
                       SPINEL_DATATYPE_UINT8_S,
                       &numericMode
                   );

    VerifyOrExit(parsedLength > 0, error = OT_ERROR_PARSE);

    modeConfig.mRxOnWhenIdle = ((numericMode & SPINEL_THREAD_MODE_RX_ON_WHEN_IDLE) == SPINEL_THREAD_MODE_RX_ON_WHEN_IDLE);
    modeConfig.mSecureDataRequests =
        ((numericMode & SPINEL_THREAD_MODE_SECURE_DATA_REQUEST) == SPINEL_THREAD_MODE_SECURE_DATA_REQUEST);
    modeConfig.mDeviceType = ((numericMode & SPINEL_THREAD_MODE_FULL_FUNCTION_DEV) == SPINEL_THREAD_MODE_FULL_FUNCTION_DEV);
    modeConfig.mNetworkData = ((numericMode & SPINEL_THREAD_MODE_FULL_NETWORK_DATA) == SPINEL_THREAD_MODE_FULL_NETWORK_DATA);

    error = otThreadSetLinkMode(mInstance, modeConfig);

exit:
    return SendSetPropertyResponse(aHeader, aKey, error);
}


otError NcpBase::GetPropertyHandler_THREAD_CHILD_TIMEOUT(uint8_t aHeader, spinel_prop_key_t aKey)
{
    return SendPropertyUpdate(
               aHeader,
               SPINEL_CMD_PROP_VALUE_IS,
               aKey,
               SPINEL_DATATYPE_UINT32_S,
               otThreadGetChildTimeout(mInstance)
           );
}

otError NcpBase::GetPropertyHandler_THREAD_RLOC16(uint8_t aHeader, spinel_prop_key_t aKey)
{
    return SendPropertyUpdate(
               aHeader,
               SPINEL_CMD_PROP_VALUE_IS,
               aKey,
               SPINEL_DATATYPE_UINT16_S,
               otThreadGetRloc16(mInstance)
           );
}

otError NcpBase::GetPropertyHandler_NET_REQUIRE_JOIN_EXISTING(uint8_t aHeader, spinel_prop_key_t aKey)
{
    return SendPropertyUpdate(
               aHeader,
               SPINEL_CMD_PROP_VALUE_IS,
               aKey,
               SPINEL_DATATYPE_BOOL_S,
               mRequireJoinExistingNetwork
           );
}

otError NcpBase::SetPropertyHandler_NET_REQUIRE_JOIN_EXISTING(uint8_t aHeader, spinel_prop_key_t aKey,
                                                              const uint8_t *aValuePtr, uint16_t aValueLen)
{
    bool value = mRequireJoinExistingNetwork;
    spinel_ssize_t parsedLength;
    otError error = OT_ERROR_NONE;

    parsedLength = spinel_datatype_unpack(
                       aValuePtr,
                       aValueLen,
                       SPINEL_DATATYPE_BOOL_S,
                       &value
                   );

    VerifyOrExit(parsedLength > 0, error = OT_ERROR_PARSE);

    mRequireJoinExistingNetwork = value;

exit:
    return SendSetPropertyResponse(aHeader, aKey, error);
}

otError NcpBase::SetPropertyHandler_STREAM_NET_INSECURE(uint8_t aHeader, spinel_prop_key_t aKey,
                                                        const uint8_t *aValuePtr, uint16_t aValueLen)
{
    const uint8_t *framePtr = NULL;
    unsigned int frameLen = 0;
    const uint8_t *metaPtr = NULL;
    unsigned int metaLen = 0;
    otMessage *message;
    spinel_ssize_t parsedLength;
    otError error = OT_ERROR_NONE;

    // STREAM_NET_INSECURE packets are not secured at layer 2.
    message = otIp6NewMessage(mInstance, false);
    VerifyOrExit(message != NULL, error = OT_ERROR_NO_BUFS);

    parsedLength = spinel_datatype_unpack(
                       aValuePtr,
                       aValueLen,
                       (
                           SPINEL_DATATYPE_DATA_WLEN_S   // Frame data
                           SPINEL_DATATYPE_DATA_S        // Meta data
                       ),
                       &framePtr,
                       &frameLen,
                       &metaPtr,
                       &metaLen
                   );

    // We ignore metadata for now.
    // May later include TX power, allow retransmits, etc...
    OT_UNUSED_VARIABLE(metaPtr);
    OT_UNUSED_VARIABLE(metaLen);
    OT_UNUSED_VARIABLE(parsedLength);

    SuccessOrExit(error = otMessageAppend(message, framePtr, static_cast<uint16_t>(frameLen)));

    // Ensure the insecure message is forwarded using direct transmission.
    otMessageSetDirectTransmission(message, true);

    error = otIp6Send(mInstance, message);

    // `otIp6Send()` takes ownership of `message` (in both success or
    // failure cases). `message` is set to NULL so it is not freed at
    // exit.
    message = NULL;

exit:
    if (message != NULL)
    {
        otMessageFree(message);
    }

    if (error == OT_ERROR_NONE)
    {
        mInboundInsecureIpFrameCounter++;

        if (SPINEL_HEADER_GET_TID(aHeader) != 0)
        {
            // Only send a successful status update if
            // there was a transaction id in the aHeader.
            error = SendLastStatus(aHeader, SPINEL_STATUS_OK);
        }
    }
    else
    {
        mDroppedInboundIpFrameCounter++;

        error = SendLastStatus(aHeader, ThreadErrorToSpinelStatus(error));
    }

    OT_UNUSED_VARIABLE(aKey);

    return error;
}


otError NcpBase::SetPropertyHandler_CNTR_RESET(uint8_t aHeader, spinel_prop_key_t aKey, const uint8_t *aValuePtr,
                                               uint16_t aValueLen)
{
    uint8_t value = 0;
    spinel_ssize_t parsedLength;
    otError error = OT_ERROR_NONE;

    parsedLength = spinel_datatype_unpack(
                       aValuePtr,
                       aValueLen,
                       SPINEL_DATATYPE_UINT8_S,
                       &value
                   );

    VerifyOrExit(parsedLength > 0, error = OT_ERROR_PARSE);

    VerifyOrExit(value == 1, error = OT_ERROR_INVALID_ARGS);

    // TODO: Implement counter reset!
    error = OT_ERROR_NOT_IMPLEMENTED;

exit:
    OT_UNUSED_VARIABLE(aKey);

    // There is currently no getter for PROP_CNTR_RESET, so we just
    // return SPINEL_STATUS_OK for success when the counters are reset.

    return SendLastStatus(aHeader, ThreadErrorToSpinelStatus(error));
}


#if OPENTHREAD_ENABLE_DIAG

otError NcpBase::SetPropertyHandler_NEST_STREAM_MFG(uint8_t aHeader, spinel_prop_key_t aKey, const uint8_t *aValuePtr,
                                                    uint16_t aValueLen)
{
    char *string = NULL;
    char *output = NULL;
    spinel_ssize_t parsedLength;
    otError error = OT_ERROR_NONE;

    parsedLength = spinel_datatype_unpack(
                       aValuePtr,
                       aValueLen,
                       SPINEL_DATATYPE_UTF8_S,
                       &string
                   );

    if ((parsedLength > 0) && (string != NULL))
    {
        // all diagnostics related features are processed within diagnostics module
        output = otDiagProcessCmdLine(string);

        error = SendPropertyUpdate(
                        aHeader,
                        SPINEL_CMD_PROP_VALUE_IS,
                        aKey,
                        reinterpret_cast<uint8_t *>(output),
                        static_cast<uint16_t>(strlen(output) + 1)
                    );
    }
    else
    {
        error = SendLastStatus(aHeader, SPINEL_STATUS_PARSE_ERROR);
    }

    return error;
}

#endif // OPENTHREAD_ENABLE_DIAG

otError NcpBase::InsertPropertyHandler_THREAD_ASSISTING_PORTS(uint8_t aHeader, spinel_prop_key_t aKey,
                                                              const uint8_t *aValuePtr, uint16_t aValueLen)
{
    spinel_ssize_t parsedLength;
    otError error = OT_ERROR_NONE;
    spinel_status_t spinelError = SPINEL_STATUS_OK;
    uint16_t port;

    parsedLength = spinel_datatype_unpack(
                       aValuePtr,
                       aValueLen,
                       SPINEL_DATATYPE_UINT16_S,
                       &port
                   );

    VerifyOrExit(parsedLength > 0, spinelError = SPINEL_STATUS_PARSE_ERROR);

    error = otIp6AddUnsecurePort(mInstance, port);
    VerifyOrExit(error == OT_ERROR_NONE, spinelError = ThreadErrorToSpinelStatus(error));

    error = SendPropertyUpdate(
                aHeader,
                SPINEL_CMD_PROP_VALUE_INSERTED,
                aKey,
                aValuePtr,
                aValueLen
            );

exit:

    if (spinelError != SPINEL_STATUS_OK)
    {
        error = SendLastStatus(aHeader, spinelError);
    }

    return error;
}

#if OPENTHREAD_ENABLE_MAC_FILTER

otError NcpBase::InsertPropertyHandler_MAC_WHITELIST(uint8_t aHeader, spinel_prop_key_t aKey,
                                                     const uint8_t *aValuePtr, uint16_t aValueLen)
{
    spinel_ssize_t parsedLength;
    otError error = OT_ERROR_NONE;
    spinel_status_t spinelError = SPINEL_STATUS_OK;
    otExtAddress *extAddress = NULL;
    int8_t rss = OT_MAC_FILTER_FIXED_RSS_DISABLED;

    if (aValueLen > static_cast<spinel_ssize_t>(sizeof(otExtAddress)))
    {
        parsedLength = spinel_datatype_unpack(
                           aValuePtr,
                           aValueLen,
                           SPINEL_DATATYPE_EUI64_S SPINEL_DATATYPE_INT8_S,
                           &extAddress,
                           &rss
                       );
    }
    else
    {
        parsedLength = spinel_datatype_unpack(
                           aValuePtr,
                           aValueLen,
                           SPINEL_DATATYPE_EUI64_S,
                           &extAddress
                       );
    }

    VerifyOrExit(parsedLength > 0, spinelError = SPINEL_STATUS_PARSE_ERROR);

    error = otLinkFilterAddAddress(mInstance, extAddress);

    VerifyOrExit(error == OT_ERROR_NONE || error == OT_ERROR_ALREADY, spinelError = ThreadErrorToSpinelStatus(error));

    if (rss != OT_MAC_FILTER_FIXED_RSS_DISABLED)
    {
        error = otLinkFilterAddRssIn(mInstance, extAddress, rss);
        VerifyOrExit(error == OT_ERROR_NONE, spinelError = ThreadErrorToSpinelStatus(error));
    }

    error = SendPropertyUpdate(
                aHeader,
                SPINEL_CMD_PROP_VALUE_INSERTED,
                aKey,
                aValuePtr,
                aValueLen
            );

exit:

    if (spinelError != SPINEL_STATUS_OK)
    {
        error = SendLastStatus(aHeader, spinelError);
    }

    return error;
}

otError NcpBase::InsertPropertyHandler_MAC_BLACKLIST(uint8_t aHeader, spinel_prop_key_t aKey,
                                                     const uint8_t *aValuePtr, uint16_t aValueLen)
{
    spinel_ssize_t parsedLength;
    otError error = OT_ERROR_NONE;
    spinel_status_t spinelError = SPINEL_STATUS_OK;
    otExtAddress *extAddress = NULL;

    parsedLength = spinel_datatype_unpack(
                       aValuePtr,
                       aValueLen,
                       SPINEL_DATATYPE_EUI64_S,
                       &extAddress
                   );

    VerifyOrExit(parsedLength > 0, spinelError = SPINEL_STATUS_PARSE_ERROR);

    error = otLinkFilterAddAddress(mInstance, extAddress);

    VerifyOrExit(error == OT_ERROR_NONE || error == OT_ERROR_ALREADY, spinelError = ThreadErrorToSpinelStatus(error));

    error = SendPropertyUpdate(
                aHeader,
                SPINEL_CMD_PROP_VALUE_INSERTED,
                aKey,
                aValuePtr,
                aValueLen
            );

exit:

    if (spinelError != SPINEL_STATUS_OK)
    {
        error = SendLastStatus(aHeader, spinelError);
    }

    return error;
}

otError NcpBase::InsertPropertyHandler_MAC_FIXED_RSS(uint8_t aHeader, spinel_prop_key_t aKey,
                                                     const uint8_t *aValuePtr, uint16_t aValueLen)
{
    spinel_ssize_t parsedLength;
    otError error = OT_ERROR_NONE;
    spinel_status_t spinelError = SPINEL_STATUS_OK;
    otExtAddress *extAddress = NULL;
    int8_t rss = OT_MAC_FILTER_FIXED_RSS_DISABLED;

    if (aValueLen > sizeof(int8_t))
    {
        parsedLength = spinel_datatype_unpack(
                           aValuePtr,
                           aValueLen,
                           SPINEL_DATATYPE_EUI64_S SPINEL_DATATYPE_INT8_S,
                           &extAddress,
                           &rss
                       );
    }
    else
    {
        parsedLength = spinel_datatype_unpack(
                           aValuePtr,
                           aValueLen,
                           SPINEL_DATATYPE_INT8_S,
                           &rss
                       );
    }

    VerifyOrExit(parsedLength > 0, spinelError = SPINEL_STATUS_PARSE_ERROR);

    error = otLinkFilterAddRssIn(mInstance, extAddress, rss);

    VerifyOrExit(error == OT_ERROR_NONE, spinelError = ThreadErrorToSpinelStatus(error));

    error = SendPropertyUpdate(
                aHeader,
                SPINEL_CMD_PROP_VALUE_INSERTED,
                aKey,
                aValuePtr,
                aValueLen
            );

exit:

    if (spinelError != SPINEL_STATUS_OK)
    {
        error = SendLastStatus(aHeader, spinelError);
    }

    return error;
}

#endif // OPENTHREAD_ENABLE_MAC_FILTER

otError NcpBase::RemovePropertyHandler_THREAD_ASSISTING_PORTS(uint8_t aHeader, spinel_prop_key_t aKey,
                                                              const uint8_t *aValuePtr, uint16_t aValueLen)
{
    spinel_ssize_t parsedLength;
    otError error = OT_ERROR_NONE;
    spinel_status_t spinelError = SPINEL_STATUS_OK;
    uint16_t port;

    parsedLength = spinel_datatype_unpack(
                       aValuePtr,
                       aValueLen,
                       SPINEL_DATATYPE_UINT16_S,
                       &port
                   );

    VerifyOrExit(parsedLength > 0, spinelError = SPINEL_STATUS_PARSE_ERROR);

    error = otIp6RemoveUnsecurePort(mInstance, port);
    VerifyOrExit(error == OT_ERROR_NONE, spinelError = ThreadErrorToSpinelStatus(error));

    error = SendPropertyUpdate(
                aHeader,
                SPINEL_CMD_PROP_VALUE_REMOVED,
                aKey,
                aValuePtr,
                aValueLen
            );

exit:

    if (spinelError != SPINEL_STATUS_OK)
    {
        error = SendLastStatus(aHeader, spinelError);
    }

    return error;
}

#if OPENTHREAD_ENABLE_MAC_FILTER

otError NcpBase::RemovePropertyHandler_MAC_WHITELIST(uint8_t aHeader, spinel_prop_key_t aKey,
                                                     const uint8_t *aValuePtr, uint16_t aValueLen)
{
    spinel_ssize_t parsedLength;
    otError error = OT_ERROR_NONE;
    spinel_status_t spinelError = SPINEL_STATUS_OK;
    otExtAddress *extAddress = NULL;

    parsedLength = spinel_datatype_unpack(
                       aValuePtr,
                       aValueLen,
                       SPINEL_DATATYPE_EUI64_S,
                       &extAddress
                   );

    VerifyOrExit(parsedLength > 0, spinelError = SPINEL_STATUS_PARSE_ERROR);

    error = otLinkFilterRemoveAddress(mInstance, extAddress);

    VerifyOrExit(error == OT_ERROR_NONE, spinelError = ThreadErrorToSpinelStatus(error));

    error = SendPropertyUpdate(
                aHeader,
                SPINEL_CMD_PROP_VALUE_REMOVED,
                aKey,
                aValuePtr,
                aValueLen
            );

exit:

    if (spinelError != SPINEL_STATUS_OK)
    {
        error = SendLastStatus(aHeader, spinelError);
    }

    return error;
}

otError NcpBase::RemovePropertyHandler_MAC_BLACKLIST(uint8_t aHeader, spinel_prop_key_t aKey,
                                                     const uint8_t *aValuePtr, uint16_t aValueLen)
{
    spinel_ssize_t parsedLength;
    otError error = OT_ERROR_NONE;
    spinel_status_t spinelError = SPINEL_STATUS_OK;
    otExtAddress *extAddress = NULL;

    parsedLength = spinel_datatype_unpack(
                       aValuePtr,
                       aValueLen,
                       SPINEL_DATATYPE_EUI64_S,
                       &extAddress
                   );

    VerifyOrExit(parsedLength > 0, spinelError = SPINEL_STATUS_PARSE_ERROR);

    error = otLinkFilterRemoveAddress(mInstance, extAddress);

    VerifyOrExit(error == OT_ERROR_NONE, spinelError = ThreadErrorToSpinelStatus(error));

    error = SendPropertyUpdate(
                aHeader,
                SPINEL_CMD_PROP_VALUE_REMOVED,
                aKey,
                aValuePtr,
                aValueLen
            );

exit:

    if (spinelError != SPINEL_STATUS_OK)
    {
        error = SendLastStatus(aHeader, spinelError);
    }

    return error;
}

otError NcpBase::RemovePropertyHandler_MAC_FIXED_RSS(uint8_t aHeader, spinel_prop_key_t aKey,
                                                     const uint8_t *aValuePtr, uint16_t aValueLen)
{
    spinel_ssize_t parsedLength;
    otError error = OT_ERROR_NONE;
    spinel_status_t spinelError = SPINEL_STATUS_OK;
    otExtAddress *extAddress = NULL;

    if (aValueLen > 0)
    {
        parsedLength = spinel_datatype_unpack(
                aValuePtr,
                aValueLen,
                SPINEL_DATATYPE_EUI64_S,
                &extAddress
                );

        VerifyOrExit(parsedLength > 0, spinelError = SPINEL_STATUS_PARSE_ERROR);
    }

    error = otLinkFilterRemoveRssIn(mInstance, extAddress);

    VerifyOrExit(error == OT_ERROR_NONE, spinelError = ThreadErrorToSpinelStatus(error));

    error = SendPropertyUpdate(
                aHeader,
                SPINEL_CMD_PROP_VALUE_REMOVED,
                aKey,
                aValuePtr,
                aValueLen
            );

exit:

    if (spinelError != SPINEL_STATUS_OK)
    {
        error = SendLastStatus(aHeader, spinelError);
    }

    return error;
}

#endif // OPENTHREAD_ENABLE_MAC_FILTER

#if OPENTHREAD_ENABLE_LEGACY

void NcpBase::RegisterLegacyHandlers(const otNcpLegacyHandlers *aHandlers)
{
    mLegacyHandlers = aHandlers;
    bool isEnabled;

    VerifyOrExit(mLegacyHandlers != NULL);

    isEnabled = (otThreadGetDeviceRole(mInstance) != OT_DEVICE_ROLE_DISABLED);

    if (isEnabled)
    {
        if (mLegacyHandlers->mStartLegacy)
        {
            mLegacyHandlers->mStartLegacy();
        }
    }
    else
    {
        if (mLegacyHandlers->mStopLegacy)
        {
            mLegacyHandlers->mStopLegacy();
        }
    }

    if (mLegacyHandlers->mSetLegacyUlaPrefix)
    {
        mLegacyHandlers->mSetLegacyUlaPrefix(mLegacyUlaPrefix);
    }

exit:
    return;
}

void NcpBase::HandleDidReceiveNewLegacyUlaPrefix(const uint8_t *aUlaPrefix)
{
    memcpy(mLegacyUlaPrefix, aUlaPrefix, OT_NCP_LEGACY_ULA_PREFIX_LENGTH);
    mChangedPropsSet.AddProperty(SPINEL_PROP_NEST_LEGACY_ULA_PREFIX);
    mUpdateChangedPropsTask.Post();
}

void NcpBase::HandleLegacyNodeDidJoin(const otExtAddress *aExtAddr)
{
    mLegacyNodeDidJoin = true;
    mLegacyLastJoinedNode = *aExtAddr;
    mChangedPropsSet.AddProperty(SPINEL_PROP_NEST_LEGACY_LAST_NODE_JOINED);
    mUpdateChangedPropsTask.Post();
}

#endif // OPENTHREAD_ENABLE_LEGACY

#if OPENTHREAD_ENABLE_LEGACY
otError NcpBase::GetPropertyHandler_NEST_LEGACY_ULA_PREFIX(uint8_t aHeader, spinel_prop_key_t aKey)
{
    return SendPropertyUpdate(
               aHeader,
               SPINEL_CMD_PROP_VALUE_IS,
               aKey,
               SPINEL_DATATYPE_DATA_S,
               mLegacyUlaPrefix,
               sizeof(mLegacyUlaPrefix)
           );
}

otError NcpBase::SetPropertyHandler_NEST_LEGACY_ULA_PREFIX(uint8_t aHeader, spinel_prop_key_t aKey,
                                                           const uint8_t *aValuePtr, uint16_t aValueLen)
{
    const uint8_t *ptr = NULL;
    spinel_size_t len;
    spinel_ssize_t parsedLength;
    otError error = OT_ERROR_NONE;

    parsedLength = spinel_datatype_unpack(
                       aValuePtr,
                       aValueLen,
                       SPINEL_DATATYPE_DATA_S,
                       &ptr,
                       &len
                   );

    VerifyOrExit((parsedLength > 0) && (len <= sizeof(mLegacyUlaPrefix)), error = OT_ERROR_PARSE);

    memset(mLegacyUlaPrefix, 0, sizeof(mLegacyUlaPrefix));
    memcpy(mLegacyUlaPrefix, ptr, len);

    if ((mLegacyHandlers != NULL) && (mLegacyHandlers->mSetLegacyUlaPrefix != NULL))
    {
        mLegacyHandlers->mSetLegacyUlaPrefix(mLegacyUlaPrefix);
    }

exit:
    return SendSetPropertyResponse(aHeader, aKey, error);
}

otError NcpBase::GetPropertyHandler_NEST_LEGACY_LAST_NODE_JOINED(uint8_t aHeader, spinel_prop_key_t aKey)
{
    if (!mLegacyNodeDidJoin)
    {
        memset(&mLegacyLastJoinedNode, 0, sizeof(mLegacyLastJoinedNode));
    }

    return SendPropertyUpdate(
               aHeader,
               SPINEL_CMD_PROP_VALUE_IS,
               aKey,
               SPINEL_DATATYPE_EUI64_S,
               &mLegacyLastJoinedNode,
               sizeof(mLegacyLastJoinedNode)
           );
}
#endif // OPENTHREAD_ENABLE_LEGACY

otError NcpBase::GetPropertyHandler_ChannelMaskHelper(uint8_t aHeader, spinel_prop_key_t aKey, uint32_t aChannelMask)
{
    otError error = OT_ERROR_NONE;

    SuccessOrExit(error = OutboundFrameBegin(aHeader));
    SuccessOrExit(
        error = OutboundFrameFeedPacked(
                    SPINEL_DATATYPE_COMMAND_PROP_S,
                    aHeader,
                    SPINEL_CMD_PROP_VALUE_IS,
                    aKey
                ));

    for (int i = 0; i < 32; i++)
    {
        if (0 != (aChannelMask & (1 << i)))
        {
            SuccessOrExit(error = OutboundFrameFeedPacked(SPINEL_DATATYPE_UINT8_S, i));
        }
    }

    SuccessOrExit(error = OutboundFrameSend());

exit:
    return error;
}


otError NcpBase::GetPropertyHandler_MAC_SCAN_MASK(uint8_t aHeader, spinel_prop_key_t aKey)
{
    return GetPropertyHandler_ChannelMaskHelper(aHeader, aKey, mChannelMask);
}

otError NcpBase::SetPropertyHandler_MAC_SCAN_MASK(uint8_t aHeader, spinel_prop_key_t aKey, const uint8_t *aValuePtr,
                                                  uint16_t aValueLen)
{
    uint32_t newMask = 0;
    otError error = OT_ERROR_NONE;

    for (; aValueLen != 0; aValueLen--, aValuePtr++)
    {
        VerifyOrExit(aValuePtr[0] <= 31, error = OT_ERROR_INVALID_ARGS);
        VerifyOrExit((mSupportedChannelMask & (1 << aValuePtr[0])) != 0, error = OT_ERROR_INVALID_ARGS);

        newMask |= (1 << aValuePtr[0]);
    }

    mChannelMask = newMask;

exit:
    return SendSetPropertyResponse(aHeader, aKey, error);
}

otError NcpBase::GetPropertyHandler_MAC_SCAN_PERIOD(uint8_t aHeader, spinel_prop_key_t aKey)
{
    return SendPropertyUpdate(
               aHeader,
               SPINEL_CMD_PROP_VALUE_IS,
               aKey,
               SPINEL_DATATYPE_UINT16_S,
               mScanPeriod
           );
}

otError NcpBase::SetPropertyHandler_MAC_SCAN_PERIOD(uint8_t aHeader, spinel_prop_key_t aKey, const uint8_t *aValuePtr,
                                                    uint16_t aValueLen)
{
    uint16_t period = mScanPeriod;
    spinel_ssize_t parsedLength;
    otError error = OT_ERROR_NONE;

    parsedLength = spinel_datatype_unpack(
                       aValuePtr,
                       aValueLen,
                       SPINEL_DATATYPE_UINT16_S,
                       &period
                   );

    VerifyOrExit(parsedLength > 0, error = OT_ERROR_PARSE);

    mScanPeriod = period;

exit:
    return SendSetPropertyResponse(aHeader, aKey, error);
}

otError NcpBase::GetPropertyHandler_MAC_SCAN_STATE(uint8_t aHeader, spinel_prop_key_t aKey)
{
    otError error = OT_ERROR_NONE;

#if OPENTHREAD_ENABLE_RAW_LINK_API

    if (otLinkRawIsEnabled(mInstance))
    {
        error = SendPropertyUpdate(
                        aHeader,
                        SPINEL_CMD_PROP_VALUE_IS,
                        aKey,
                        SPINEL_DATATYPE_UINT8_S,
                        mCurScanChannel == kInvalidScanChannel
                            ? SPINEL_SCAN_STATE_IDLE
                            : SPINEL_SCAN_STATE_ENERGY
                    );
    }
    else

#endif // OPENTHREAD_ENABLE_RAW_LINK_API
    {
        if (otLinkIsActiveScanInProgress(mInstance))
        {
            error = SendPropertyUpdate(
                            aHeader,
                            SPINEL_CMD_PROP_VALUE_IS,
                            aKey,
                            SPINEL_DATATYPE_UINT8_S,
                            SPINEL_SCAN_STATE_BEACON
                        );
        }
        else if (otLinkIsEnergyScanInProgress(mInstance))
        {
            error = SendPropertyUpdate(
                            aHeader,
                            SPINEL_CMD_PROP_VALUE_IS,
                            aKey,
                            SPINEL_DATATYPE_UINT8_S,
                            SPINEL_SCAN_STATE_ENERGY
                        );
        }
        else if (otThreadIsDiscoverInProgress(mInstance))
        {
            error = SendPropertyUpdate(
                            aHeader,
                            SPINEL_CMD_PROP_VALUE_IS,
                            aKey,
                            SPINEL_DATATYPE_UINT8_S,
                            SPINEL_SCAN_STATE_DISCOVER
                        );
        }
        else
        {
            error = SendPropertyUpdate(
                            aHeader,
                            SPINEL_CMD_PROP_VALUE_IS,
                            aKey,
                            SPINEL_DATATYPE_UINT8_S,
                            SPINEL_SCAN_STATE_IDLE
                        );
        }
    }

    return error;
}

otError NcpBase::SetPropertyHandler_MAC_SCAN_STATE(uint8_t aHeader, spinel_prop_key_t aKey, const uint8_t *aValuePtr,
                                                   uint16_t aValueLen)
{
    uint8_t state = 0;
    spinel_ssize_t parsedLength;
    otError error = OT_ERROR_NONE;

    parsedLength = spinel_datatype_unpack(
                       aValuePtr,
                       aValueLen,
                       SPINEL_DATATYPE_UINT8_S,
                       &state
                   );

    VerifyOrExit(parsedLength > 0, error = OT_ERROR_PARSE);

    switch (state)
    {
    case SPINEL_SCAN_STATE_IDLE:
        error = OT_ERROR_NONE;
        break;

    case SPINEL_SCAN_STATE_BEACON:
#if OPENTHREAD_ENABLE_RAW_LINK_API
        if (otLinkRawIsEnabled(mInstance))
        {
            error = OT_ERROR_NOT_IMPLEMENTED;
        }
        else
#endif // OPENTHREAD_ENABLE_RAW_LINK_API
        {
            error = otLinkActiveScan(
                        mInstance,
                        mChannelMask,
                        mScanPeriod,
                        &HandleActiveScanResult_Jump,
                        this
                    );
        }

        SuccessOrExit(error);
        break;

    case SPINEL_SCAN_STATE_ENERGY:
#if OPENTHREAD_ENABLE_RAW_LINK_API
        if (otLinkRawIsEnabled(mInstance))
        {
            uint8_t scanChannel;

            // Make sure we aren't already scanning and that we have
            // only 1 bit set for the channel mask.
            VerifyOrExit(mCurScanChannel == kInvalidScanChannel, error = OT_ERROR_INVALID_STATE);
            VerifyOrExit(HasOnly1BitSet(mChannelMask), error = OT_ERROR_INVALID_ARGS);

            scanChannel = IndexOfMSB(mChannelMask);
            mCurScanChannel = (int8_t)scanChannel;

            error = otLinkRawEnergyScan(
                        mInstance,
                        scanChannel,
                        mScanPeriod,
                        LinkRawEnergyScanDone
                    );
        }
        else
#endif // OPENTHREAD_ENABLE_RAW_LINK_API
        {
            error = otLinkEnergyScan(
                        mInstance,
                        mChannelMask,
                        mScanPeriod,
                        &HandleEnergyScanResult_Jump,
                        this
                    );
        }

        SuccessOrExit(error);
        break;

    case SPINEL_SCAN_STATE_DISCOVER:
        error = otThreadDiscover(
                    mInstance,
                    mChannelMask,
                    mDiscoveryScanPanId,
                    mDiscoveryScanJoinerFlag,
                    mDiscoveryScanEnableFiltering,
                    &HandleActiveScanResult_Jump,
                    this
                );

        SuccessOrExit(error);
        break;

    default:
        error = OT_ERROR_INVALID_ARGS;
        break;
    }

exit:
    return SendSetPropertyResponse(aHeader, aKey, error);
}

void NcpBase::HandleActiveScanResult_Jump(otActiveScanResult *aResult, void *aContext)
{
    static_cast<NcpBase *>(aContext)->HandleActiveScanResult(aResult);
}

// ----------------------------------------------------------------------------
// MARK: Scan Results Glue
// ----------------------------------------------------------------------------

void NcpBase::HandleActiveScanResult(otActiveScanResult *aResult)
{
    otError error;

    if (aResult)
    {
        uint8_t flags = static_cast<uint8_t>(aResult->mVersion << SPINEL_BEACON_THREAD_FLAG_VERSION_SHIFT);

        if (aResult->mIsJoinable)
        {
            flags |= SPINEL_BEACON_THREAD_FLAG_JOINABLE;
        }

        if (aResult->mIsNative)
        {
            flags |= SPINEL_BEACON_THREAD_FLAG_NATIVE;
        }

        error = SendPropertyUpdate(
                    SPINEL_HEADER_FLAG | SPINEL_HEADER_IID_0,
                    SPINEL_CMD_PROP_VALUE_INSERTED,
                    SPINEL_PROP_MAC_SCAN_BEACON,
                    SPINEL_DATATYPE_MAC_SCAN_RESULT_S(
                        SPINEL_802_15_4_DATATYPE_MAC_SCAN_RESULT_V1_S,
                        SPINEL_NET_DATATYPE_MAC_SCAN_RESULT_V2_S
                    ),
                    aResult->mChannel,                                              // Channel
                    aResult->mRssi,                                                 // RSSI
                                                                                    // "mac-layer data"
                    aResult->mExtAddress.m8,                                        //      laddr
                    0xFFFF,                                                         //      saddr, not given
                    aResult->mPanId,                                                //      panid
                    aResult->mLqi,                                                  //      lqi
                                                                                    // "net-layer data"
                    SPINEL_PROTOCOL_TYPE_THREAD,                                    //      type
                    flags,                                                          //      flags
                    aResult->mNetworkName.m8,                                       //      network name
                    aResult->mExtendedPanId.m8, OT_EXT_PAN_ID_SIZE,                 //      xpanid
                    aResult->mSteeringData.m8, aResult->mSteeringData.mLength       //      steering data
                );

        if (error != OT_ERROR_NONE)
        {
            mChangedPropsSet.AddLastStatus(SPINEL_STATUS_NOMEM);
            mUpdateChangedPropsTask.Post();
        }
    }
    else
    {
        // We are finished with the scan, so send out
        // a property update indicating such.
        error = SendPropertyUpdate(
                    SPINEL_HEADER_FLAG | SPINEL_HEADER_IID_0,
                    SPINEL_CMD_PROP_VALUE_IS,
                    SPINEL_PROP_MAC_SCAN_STATE,
                    SPINEL_DATATYPE_UINT8_S,
                    SPINEL_SCAN_STATE_IDLE
                );

        // If we could not send the end of scan indicator message now (no
        // buffer space), we add the MAC_SCAN_STATE property to changed
        // property set so that the update is sent when buffer becomes
        // available.
        if (error != OT_ERROR_NONE)
        {
            mChangedPropsSet.AddProperty(SPINEL_PROP_MAC_SCAN_STATE);
        }
    }
}

void NcpBase::HandleEnergyScanResult_Jump(otEnergyScanResult *aResult, void *aContext)
{
    static_cast<NcpBase *>(aContext)->HandleEnergyScanResult(aResult);
}

void NcpBase::HandleEnergyScanResult(otEnergyScanResult *aResult)
{
    otError error;

    if (aResult)
    {
        error = SendPropertyUpdate(
                    SPINEL_HEADER_FLAG | SPINEL_HEADER_IID_0,
                    SPINEL_CMD_PROP_VALUE_INSERTED,
                    SPINEL_PROP_MAC_ENERGY_SCAN_RESULT,
                    (
                        SPINEL_DATATYPE_UINT8_S   // Channel
                        SPINEL_DATATYPE_INT8_S    // Rssi
                    ),
                    aResult->mChannel,
                    aResult->mMaxRssi
                );

        if (error != OT_ERROR_NONE)
        {
            mChangedPropsSet.AddLastStatus(SPINEL_STATUS_NOMEM);
            mUpdateChangedPropsTask.Post();
        }
    }
    else
    {
        // We are finished with the scan, so send out
        // a property update indicating such.
        error = SendPropertyUpdate(
                    SPINEL_HEADER_FLAG | SPINEL_HEADER_IID_0,
                    SPINEL_CMD_PROP_VALUE_IS,
                    SPINEL_PROP_MAC_SCAN_STATE,
                    SPINEL_DATATYPE_UINT8_S,
                    SPINEL_SCAN_STATE_IDLE
                );

        // If we could not send the end of scan indicator message now (no
        // buffer space), we add the MAC_SCAN_STATE property to changed
        // property set so that the update is sent when buffer becomes
        // available.
        if (error != OT_ERROR_NONE)
        {
            mChangedPropsSet.AddProperty(SPINEL_PROP_MAC_SCAN_STATE);
        }
    }
}


// ----------------------------------------------------------------------------
// MARK: Outbound Datagram Handling
// ----------------------------------------------------------------------------

void NcpBase::HandleDatagramFromStack(otMessage *aMessage, void *aContext)
{
    static_cast<NcpBase *>(aContext)->HandleDatagramFromStack(aMessage);
}

void NcpBase::HandleDatagramFromStack(otMessage *aMessage)
{
    otError error = OT_ERROR_NONE;
    uint8_t header = SPINEL_HEADER_FLAG | SPINEL_HEADER_IID_0;
    bool isSecure = otMessageIsLinkSecurityEnabled(aMessage);
    uint16_t length = otMessageGetLength(aMessage);

    SuccessOrExit(error = OutboundFrameBegin(header));

    SuccessOrExit(
        error = OutboundFrameFeedPacked(
                    SPINEL_DATATYPE_COMMAND_PROP_S SPINEL_DATATYPE_UINT16_S,
                    header,
                    SPINEL_CMD_PROP_VALUE_IS,
                    isSecure ? SPINEL_PROP_STREAM_NET : SPINEL_PROP_STREAM_NET_INSECURE,
                    length
                ));

    SuccessOrExit(error = OutboundFrameFeedMessage(aMessage));

    // Set the `aMessage` pointer to NULL to indicate that it does
    // not need to be freed at the exit. The `aMessage` is now owned
    // by the OutboundFrame and will be freed when the frame is either
    // successfully sent and then removed, or if the frame gets
    // discarded.
    aMessage = NULL;

    // Append any metadata (rssi, lqi, channel, etc) here!

    SuccessOrExit(error = OutboundFrameSend());

exit:

    if (aMessage != NULL)
    {
        otMessageFree(aMessage);
    }

    if (error != OT_ERROR_NONE)
    {
        mChangedPropsSet.AddLastStatus(SPINEL_STATUS_DROPPED);
        mUpdateChangedPropsTask.Post();
        mDroppedOutboundIpFrameCounter++;
    }
    else
    {
        if (isSecure)
        {
            mOutboundSecureIpFrameCounter++;
        }
        else
        {
            mOutboundInsecureIpFrameCounter++;
        }
    }
}


// ----------------------------------------------------------------------------
// MARK: Property/Status Changed
// ----------------------------------------------------------------------------

void NcpBase::HandleNetifStateChanged(uint32_t aFlags, void *aContext)
{
    NcpBase *ncp = static_cast<NcpBase *>(aContext);

    ncp->mThreadChangedFlags |= aFlags;
    ncp->mUpdateChangedPropsTask.Post();
}

void NcpBase::ProcessThreadChangedFlags(void)
{
    static const struct
    {
        uint32_t mThreadFlag;
        spinel_prop_key_t mPropKey;
    } kFlags[] =
    {
        { OT_CHANGED_IP6_ADDRESS_ADDED,           SPINEL_PROP_IPV6_ADDRESS_TABLE             },
        { OT_CHANGED_IP6_ADDRESS_REMOVED,         SPINEL_PROP_IPV6_ADDRESS_TABLE             },
        { OT_CHANGED_THREAD_ROLE,                 SPINEL_PROP_NET_ROLE                       },
        { OT_CHANGED_THREAD_LL_ADDR,              SPINEL_PROP_IPV6_LL_ADDR                   },
        { OT_CHANGED_THREAD_ML_ADDR,              SPINEL_PROP_IPV6_ML_ADDR                   },
        { OT_CHANGED_THREAD_PARTITION_ID,         SPINEL_PROP_NET_PARTITION_ID               },
        { OT_CHANGED_THREAD_KEY_SEQUENCE_COUNTER, SPINEL_PROP_NET_KEY_SEQUENCE_COUNTER       },
        { OT_CHANGED_THREAD_NETDATA,              SPINEL_PROP_THREAD_LEADER_NETWORK_DATA     },
        { OT_CHANGED_THREAD_CHILD_ADDED,          SPINEL_PROP_THREAD_CHILD_TABLE             },
        { OT_CHANGED_THREAD_CHILD_REMOVED,        SPINEL_PROP_THREAD_CHILD_TABLE             },
        { OT_CHANGED_IP6_MULTICAST_SUBSRCRIBED,   SPINEL_PROP_IPV6_MULTICAST_ADDRESS_TABLE   },
        { OT_CHANGED_IP6_MULTICAST_UNSUBSRCRIBED, SPINEL_PROP_IPV6_MULTICAST_ADDRESS_TABLE   },
    };

    VerifyOrExit(mThreadChangedFlags != 0);

    // If thread role has changed, check for possible "join" error.

    if ((mThreadChangedFlags & OT_CHANGED_THREAD_ROLE) != 0)
    {
        if (mRequireJoinExistingNetwork)
        {
            switch (otThreadGetDeviceRole(mInstance))
            {
            case OT_DEVICE_ROLE_DETACHED:
            case OT_DEVICE_ROLE_DISABLED:
                break;

            default:
                mRequireJoinExistingNetwork = false;
                mChangedPropsSet.AddProperty(SPINEL_PROP_NET_REQUIRE_JOIN_EXISTING);
                break;
            }

            if ((otThreadGetDeviceRole(mInstance) == OT_DEVICE_ROLE_LEADER)
                && otThreadIsSingleton(mInstance)
#if OPENTHREAD_ENABLE_LEGACY
                && !mLegacyNodeDidJoin
#endif
               )
            {
                mThreadChangedFlags &= ~static_cast<uint32_t>(OT_CHANGED_THREAD_PARTITION_ID);
                otThreadSetEnabled(mInstance, false);

                mChangedPropsSet.AddProperty(SPINEL_PROP_NET_STACK_UP);
                mChangedPropsSet.AddLastStatus(SPINEL_STATUS_JOIN_FAILURE);
            }
        }
    }

    // Convert OT_CHANGED flags to corresponding NCP property update.

    for (unsigned i = 0; i < sizeof(kFlags) / sizeof(kFlags[0]); i++)
    {
        uint32_t threadFlag = kFlags[i].mThreadFlag;

        if (mThreadChangedFlags & threadFlag)
        {
            mChangedPropsSet.AddProperty(kFlags[i].mPropKey);

            if (threadFlag == OT_CHANGED_THREAD_NETDATA)
            {
                mChangedPropsSet.AddProperty(SPINEL_PROP_THREAD_ON_MESH_NETS);
                mChangedPropsSet.AddProperty(SPINEL_PROP_THREAD_OFF_MESH_ROUTES);
            }

            mThreadChangedFlags &= ~threadFlag;
            VerifyOrExit(mThreadChangedFlags != 0);
        }
    }

    // Clear any remaining ThreadFlag that has no matching
    // NCP property update (e.g., OT_CHANGED_THREAD_RLOC_ADDED)

    mThreadChangedFlags = 0;

exit:
    return;
}


}  // namespace Ncp
}  // namespace ot

// ----------------------------------------------------------------------------
// MARK: Legacy network APIs
// ----------------------------------------------------------------------------

void otNcpRegisterLegacyHandlers(const otNcpLegacyHandlers *aHandlers)
{
#if OPENTHREAD_ENABLE_LEGACY
    ot::Ncp::NcpBase *ncp = ot::Ncp::NcpBase::GetNcpInstance();

    if (ncp != NULL)
    {
        ncp->RegisterLegacyHandlers(aHandlers);
    }

#else
    OT_UNUSED_VARIABLE(aHandlers);
#endif
}

void otNcpHandleDidReceiveNewLegacyUlaPrefix(const uint8_t *aUlaPrefix)
{
#if OPENTHREAD_ENABLE_LEGACY
    ot::Ncp::NcpBase *ncp = ot::Ncp::NcpBase::GetNcpInstance();

    if (ncp != NULL)
    {
        ncp->HandleDidReceiveNewLegacyUlaPrefix(aUlaPrefix);
    }

#else
    OT_UNUSED_VARIABLE(aUlaPrefix);
#endif
}

void otNcpHandleLegacyNodeDidJoin(const otExtAddress *aExtAddr)
{
#if OPENTHREAD_ENABLE_LEGACY
    ot::Ncp::NcpBase *ncp = ot::Ncp::NcpBase::GetNcpInstance();

    if (ncp != NULL)
    {
        ncp->HandleLegacyNodeDidJoin(aExtAddr);
    }

#else
    OT_UNUSED_VARIABLE(aExtAddr);
#endif
}

#endif // OPENTHREAD_MTD || OPENTHREAD_FTD
