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

otError NcpBase::GetPropertyHandler_MAC_DATA_POLL_PERIOD(void)
{
    return mEncoder.WriteUint32(otLinkGetPollPeriod(mInstance));
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

otError NcpBase::GetPropertyHandler_MAC_EXTENDED_ADDR(void)
{
    return mEncoder.WriteEui64(*otLinkGetExtendedAddress(mInstance));
}

otError NcpBase::GetPropertyHandler_PHY_FREQ(void)
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

    return mEncoder.WriteUint32(freq_khz);
}

otError NcpBase::GetPropertyHandler_PHY_CHAN_SUPPORTED(void)
{
    return GetPropertyHandler_ChannelMaskHelper(mSupportedChannelMask);
}

otError NcpBase::GetPropertyHandler_PHY_RSSI(void)
{
    return mEncoder.WriteInt8(otPlatRadioGetRssi(mInstance));
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

otError NcpBase::GetPropertyHandler_NET_SAVED(void)
{
    return mEncoder.WriteBool(otDatasetIsCommissioned(mInstance));
}

otError NcpBase::GetPropertyHandler_NET_IF_UP(void)
{
    return mEncoder.WriteBool(otIp6IsEnabled(mInstance));
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

otError NcpBase::GetPropertyHandler_NET_STACK_UP(void)
{
    return mEncoder.WriteBool(otThreadGetDeviceRole(mInstance) != OT_DEVICE_ROLE_DISABLED);
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

otError NcpBase::GetPropertyHandler_NET_ROLE(void)
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

    return mEncoder.WriteUint8(role);
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

otError NcpBase::GetPropertyHandler_NET_NETWORK_NAME(void)
{
    return mEncoder.WriteUtf8(otThreadGetNetworkName(mInstance));
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

otError NcpBase::GetPropertyHandler_NET_XPANID(void)
{
    return mEncoder.WriteData(otThreadGetExtendedPanId(mInstance), sizeof(spinel_net_xpanid_t));
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

otError NcpBase::GetPropertyHandler_NET_MASTER_KEY(void)
{
    return mEncoder.WriteData(otThreadGetMasterKey(mInstance)->m8, OT_MASTER_KEY_SIZE);
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

otError NcpBase::GetPropertyHandler_NET_KEY_SEQUENCE_COUNTER(void)
{
    return mEncoder.WriteUint32(otThreadGetKeySequenceCounter(mInstance));
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

otError NcpBase::GetPropertyHandler_NET_PARTITION_ID(void)
{
    return mEncoder.WriteUint32(otThreadGetPartitionId(mInstance));
}

otError NcpBase::GetPropertyHandler_NET_KEY_SWITCH_GUARDTIME(void)
{
    return mEncoder.WriteUint32(otThreadGetKeySwitchGuardTime(mInstance));
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

otError NcpBase::GetPropertyHandler_THREAD_NETWORK_DATA_VERSION(void)
{
    return mEncoder.WriteUint8(otNetDataGetVersion(mInstance));
}

otError NcpBase::GetPropertyHandler_THREAD_STABLE_NETWORK_DATA_VERSION(void)
{
    return mEncoder.WriteUint8(otNetDataGetStableVersion(mInstance));
}

#if OPENTHREAD_ENABLE_BORDER_ROUTER
otError NcpBase::GetPropertyHandler_THREAD_NETWORK_DATA(void)
{
    uint8_t networkData[255];
    uint8_t networkDataLen = 255;

    otBorderRouterGetNetData(
        mInstance,
        false, // Stable?
        networkData,
        &networkDataLen
    );

    return mEncoder.WriteData(networkData, networkDataLen);
}

otError NcpBase::GetPropertyHandler_THREAD_STABLE_NETWORK_DATA(void)
{
    uint8_t networkData[255];
    uint8_t networkDataLen = 255;

    otBorderRouterGetNetData(
        mInstance,
        true, // Stable?
        networkData,
        &networkDataLen
    );

    return mEncoder.WriteData(networkData, networkDataLen);
}
#endif // OPENTHREAD_ENABLE_BORDER_ROUTER

otError NcpBase::GetPropertyHandler_THREAD_LEADER_NETWORK_DATA(void)
{
    uint8_t networkData[255];
    uint8_t networkDataLen = 255;

    otNetDataGet(
        mInstance,
        false, // Stable?
        networkData,
        &networkDataLen
    );

    return mEncoder.WriteData(networkData, networkDataLen);
}

otError NcpBase::GetPropertyHandler_THREAD_STABLE_LEADER_NETWORK_DATA(void)
{
    uint8_t networkData[255];
    uint8_t networkDataLen = 255;

    otNetDataGet(
        mInstance,
        true, // Stable?
        networkData,
        &networkDataLen
    );

    return mEncoder.WriteData(networkData, networkDataLen);
}

otError NcpBase::GetPropertyHandler_THREAD_LEADER_RID(void)
{
    return mEncoder.WriteUint8(otThreadGetLeaderRouterId(mInstance));
}

otError NcpBase::GetPropertyHandler_THREAD_LEADER_ADDR(void)
{
    otError error = OT_ERROR_NONE;
    otIp6Address address;

    error = otThreadGetLeaderRloc(mInstance, &address);

    if (error == OT_ERROR_NONE)
    {
        error = mEncoder.WriteIp6Address(address);
    }
    else
    {
        error = mEncoder.OverwriteWithLastStatusError(ThreadErrorToSpinelStatus(error));
    }

    return error;
}

otError NcpBase::GetPropertyHandler_THREAD_PARENT(void)
{
    otError error = OT_ERROR_NONE;

    otRouterInfo parentInfo;

    error = otThreadGetParentInfo(mInstance, &parentInfo);

    if (error == OT_ERROR_NONE)
    {
        SuccessOrExit(error = mEncoder.WriteEui64(parentInfo.mExtAddress)); // Parent's extended address
        SuccessOrExit(error = mEncoder.WriteUint16(parentInfo.mRloc16));
    }
    else
    {
        error = mEncoder.OverwriteWithLastStatusError(ThreadErrorToSpinelStatus(error));
    }

exit:
    return error;
}

otError NcpBase::GetPropertyHandler_THREAD_NEIGHBOR_TABLE(void)
{
    otError error = OT_ERROR_NONE;
    otNeighborInfoIterator iter = OT_NEIGHBOR_INFO_ITERATOR_INIT;
    otNeighborInfo neighInfo;
    uint8_t modeFlags;

    while (otThreadGetNextNeighborInfo(mInstance, &iter, &neighInfo) == OT_ERROR_NONE)
    {
        modeFlags = LinkFlagsToFlagByte(
                        neighInfo.mRxOnWhenIdle,
                        neighInfo.mSecureDataRequest,
                        neighInfo.mFullFunction,
                        neighInfo.mFullNetworkData
                    );

        SuccessOrExit(error = mEncoder.OpenStruct());

        SuccessOrExit(error = mEncoder.WriteEui64(neighInfo.mExtAddress));
        SuccessOrExit(error = mEncoder.WriteUint16(neighInfo.mRloc16));
        SuccessOrExit(error = mEncoder.WriteUint32(neighInfo.mAge));
        SuccessOrExit(error = mEncoder.WriteUint8(neighInfo.mLinkQualityIn));
        SuccessOrExit(error = mEncoder.WriteInt8(neighInfo.mAverageRssi));
        SuccessOrExit(error = mEncoder.WriteUint8(modeFlags));
        SuccessOrExit(error = mEncoder.WriteBool(neighInfo.mIsChild));
        SuccessOrExit(error = mEncoder.WriteUint32(neighInfo.mLinkFrameCounter));
        SuccessOrExit(error = mEncoder.WriteUint32(neighInfo.mMleFrameCounter));
        SuccessOrExit(error = mEncoder.WriteInt8(neighInfo.mLastRssi));

        SuccessOrExit(error = mEncoder.CloseStruct());
    }

exit:
    return error;
}

otError NcpBase::GetPropertyHandler_THREAD_ASSISTING_PORTS(void)
{
    otError error = OT_ERROR_NONE;
    uint8_t numEntries = 0;
    const uint16_t *ports = otIp6GetUnsecurePorts(mInstance, &numEntries);

    for (; numEntries != 0; ports++, numEntries--)
    {
        SuccessOrExit(error = mEncoder.WriteUint16(*ports));
    }

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

otError NcpBase::GetPropertyHandler_THREAD_ALLOW_LOCAL_NET_DATA_CHANGE(void)
{
    return mEncoder.WriteBool(mAllowLocalNetworkDataChange);
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

otError NcpBase::GetPropertyHandler_THREAD_ON_MESH_NETS(void)
{
    otError error = OT_ERROR_NONE;
    otBorderRouterConfig borderRouterConfig;
    otNetworkDataIterator iter = OT_NETWORK_DATA_ITERATOR_INIT;

    // Fill from non-local network data first
    while (otNetDataGetNextOnMeshPrefix(mInstance, &iter, &borderRouterConfig) == OT_ERROR_NONE)
    {
        SuccessOrExit(error = mEncoder.OpenStruct());

        SuccessOrExit(error = mEncoder.WriteIp6Address(borderRouterConfig.mPrefix.mPrefix));
        SuccessOrExit(error = mEncoder.WriteUint8(borderRouterConfig.mPrefix.mLength));
        SuccessOrExit(error = mEncoder.WriteBool(borderRouterConfig.mStable));
        SuccessOrExit(error = mEncoder.WriteUint8(BorderRouterConfigToFlagByte(borderRouterConfig)));
        SuccessOrExit(error = mEncoder.WriteBool(false));   // isLocal
        SuccessOrExit(error = mEncoder.WriteUint16(borderRouterConfig.mRloc16));

        SuccessOrExit(error = mEncoder.CloseStruct());
    }

#if OPENTHREAD_ENABLE_BORDER_ROUTER

    iter = OT_NETWORK_DATA_ITERATOR_INIT;

    // Fill from local network data last
    while (otBorderRouterGetNextOnMeshPrefix(mInstance, &iter, &borderRouterConfig) == OT_ERROR_NONE)
    {
        SuccessOrExit(error = mEncoder.OpenStruct());

        SuccessOrExit(error = mEncoder.WriteIp6Address(borderRouterConfig.mPrefix.mPrefix));
        SuccessOrExit(error = mEncoder.WriteUint8(borderRouterConfig.mPrefix.mLength));
        SuccessOrExit(error = mEncoder.WriteBool(borderRouterConfig.mStable));
        SuccessOrExit(error = mEncoder.WriteUint8(BorderRouterConfigToFlagByte(borderRouterConfig)));
        SuccessOrExit(error = mEncoder.WriteBool(true));   // isLocal
        SuccessOrExit(error = mEncoder.WriteUint16(borderRouterConfig.mRloc16));

        SuccessOrExit(error = mEncoder.CloseStruct());
    }
#endif // OPENTHREAD_ENABLE_BORDER_ROUTER

exit:
    return error;
}

#if OPENTHREAD_ENABLE_BORDER_ROUTER
otError NcpBase::InsertPropertyHandler_THREAD_ON_MESH_NETS(const uint8_t *aValuePtr, uint16_t aValueLen)
{
    spinel_ssize_t parsedLength;
    otError error = OT_ERROR_NONE;
    otBorderRouterConfig borderRouterConfig;
    otIp6Address *addrPtr;
    bool stable = false;
    uint8_t flags = 0;

    memset(&borderRouterConfig, 0, sizeof(otBorderRouterConfig));

    VerifyOrExit(mAllowLocalNetworkDataChange == true, error = OT_ERROR_INVALID_STATE);

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

    VerifyOrExit(parsedLength > 0, error = OT_ERROR_PARSE);

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

exit:
    return error;
}

otError NcpBase::RemovePropertyHandler_THREAD_ON_MESH_NETS(const uint8_t *aValuePtr, uint16_t aValueLen)
{
    spinel_ssize_t parsedLength;
    otError error = OT_ERROR_NONE;
    otIp6Prefix ip6Prefix;
    otIp6Address *addrPtr;

    memset(&ip6Prefix, 0, sizeof(otIp6Prefix));

    VerifyOrExit(mAllowLocalNetworkDataChange == true, error = OT_ERROR_INVALID_STATE);

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

    VerifyOrExit(parsedLength > 0, error = OT_ERROR_PARSE);

    ip6Prefix.mPrefix = *addrPtr;

    error = otBorderRouterRemoveOnMeshPrefix(mInstance, &ip6Prefix);

    // If prefix was not on the list, "remove" command can be considred
    // successful.

    if (error == OT_ERROR_NOT_FOUND)
    {
        error = OT_ERROR_NONE;
    }

exit:
    return error;
}
#endif // OPENTHREAD_ENABLE_BORDER_ROUTER

otError NcpBase::GetPropertyHandler_THREAD_DISCOVERY_SCAN_JOINER_FLAG(void)
{
    return mEncoder.WriteBool(mDiscoveryScanJoinerFlag);
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

otError NcpBase::GetPropertyHandler_THREAD_DISCOVERY_SCAN_ENABLE_FILTERING(void)
{
    return mEncoder.WriteBool(mDiscoveryScanEnableFiltering);
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

otError NcpBase::GetPropertyHandler_THREAD_DISCOVERY_SCAN_PANID(void)
{
    return mEncoder.WriteUint16(mDiscoveryScanPanId);
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

otError NcpBase::GetPropertyHandler_IPV6_ML_PREFIX(void)
{
    otError error = OT_ERROR_NONE;
    const uint8_t *mlPrefix = otThreadGetMeshLocalPrefix(mInstance);
    otIp6Address addr;

    VerifyOrExit(mlPrefix != NULL); // If `mlPrefix` is NULL send empty response.

    memcpy(addr.mFields.m8, mlPrefix, 8);

    // Zero out the last 8 bytes.
    memset(addr.mFields.m8 + 8, 0, 8);

    SuccessOrExit(error = mEncoder.WriteIp6Address(addr));      // Mesh local prefix
    SuccessOrExit(error = mEncoder.WriteUint8(64));             // Prefix length (in bits)

exit:
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

otError NcpBase::GetPropertyHandler_IPV6_ML_ADDR(void)
{
    otError error = OT_ERROR_NONE;
    const otIp6Address *ml64 = otThreadGetMeshLocalEid(mInstance);

    VerifyOrExit(ml64 != NULL);
    SuccessOrExit(error = mEncoder.WriteIp6Address(*ml64));

exit:
    return error;
}

otError NcpBase::GetPropertyHandler_IPV6_LL_ADDR(void)
{
    otError error = OT_ERROR_NONE;
    const otIp6Address *address = otThreadGetLinkLocalIp6Address(mInstance);

    VerifyOrExit(address != NULL);
    SuccessOrExit(error = mEncoder.WriteIp6Address(*address));

exit:
    return error;
}

otError NcpBase::GetPropertyHandler_IPV6_ADDRESS_TABLE(void)
{
    otError error = OT_ERROR_NONE;

    for (const otNetifAddress *address = otIp6GetUnicastAddresses(mInstance); address; address = address->mNext)
    {
        SuccessOrExit(error = mEncoder.OpenStruct());

        SuccessOrExit(error = mEncoder.WriteIp6Address(address->mAddress));
        SuccessOrExit(error = mEncoder.WriteUint8(address->mPrefixLength));
        SuccessOrExit(error = mEncoder.WriteUint32(address->mPreferred ? 0xffffffff : 0));
        SuccessOrExit(error = mEncoder.WriteUint32(address->mValid ? 0xffffffff : 0));

        SuccessOrExit(error = mEncoder.CloseStruct());
    }

exit:
    return error;
}

otError NcpBase::InsertPropertyHandler_IPV6_ADDRESS_TABLE(const uint8_t *aValuePtr, uint16_t aValueLen)
{
    spinel_ssize_t parsedLength;
    otError error = OT_ERROR_NONE;
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

    VerifyOrExit(parsedLength > 0, error = OT_ERROR_PARSE);

    netifAddr.mAddress = *addrPtr;
    netifAddr.mPrefixLength = prefixLen;
    netifAddr.mPreferred = preferredLifetime != 0;
    netifAddr.mValid = validLifetime != 0;

    error = otIp6AddUnicastAddress(mInstance, &netifAddr);

exit:
    return error;
}

otError NcpBase::RemovePropertyHandler_IPV6_ADDRESS_TABLE(const uint8_t *aValuePtr, uint16_t aValueLen)
{
    spinel_ssize_t parsedLength;
    otError error = OT_ERROR_NONE;
    otIp6Address *addrPtr;

    parsedLength = spinel_datatype_unpack(
                       aValuePtr,
                       aValueLen,
                       SPINEL_DATATYPE_IPv6ADDR_S,
                       &addrPtr
                   );

    VerifyOrExit(parsedLength > 0, error = OT_ERROR_PARSE);

    error = otIp6RemoveUnicastAddress(mInstance, addrPtr);

    // If address was not on the list, "remove" command is successful.
    if (error == OT_ERROR_NOT_FOUND)
    {
        error = OT_ERROR_NONE;
    }

exit:
    return error;
}

otError NcpBase::GetPropertyHandler_IPV6_ROUTE_TABLE(void)
{
    // TODO: Implement get route table
    return mEncoder.OverwriteWithLastStatusError(SPINEL_STATUS_UNIMPLEMENTED);
}

otError NcpBase::GetPropertyHandler_IPV6_ICMP_PING_OFFLOAD(void)
{
    return mEncoder.WriteBool(otIcmp6IsEchoEnabled(mInstance));
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

otError NcpBase::GetPropertyHandler_IPV6_MULTICAST_ADDRESS_TABLE(void)
{
    otError error = OT_ERROR_NONE;
    const otNetifMulticastAddress *address;

    for (address = otIp6GetMulticastAddresses(mInstance); address; address = address->mNext)
    {
        SuccessOrExit(error = mEncoder.OpenStruct());
        SuccessOrExit(error = mEncoder.WriteIp6Address(address->mAddress));
        SuccessOrExit(error = mEncoder.CloseStruct());
    }

exit:
    return error;
}

otError NcpBase::InsertPropertyHandler_IPV6_MULTICAST_ADDRESS_TABLE(const uint8_t *aValuePtr, uint16_t aValueLen)
{
    spinel_ssize_t parsedLength;
    otError error = OT_ERROR_NONE;
    otIp6Address *addrPtr;

    parsedLength = spinel_datatype_unpack(
                       aValuePtr,
                       aValueLen,
                       SPINEL_DATATYPE_IPv6ADDR_S,    // IPv6 address
                       &addrPtr
                   );

    VerifyOrExit(parsedLength > 0, error = OT_ERROR_PARSE);

    error = otIp6SubscribeMulticastAddress(mInstance, addrPtr);

    if (error == OT_ERROR_ALREADY)
    {
        error = OT_ERROR_NONE;
    }

exit:
    return error;
}

otError NcpBase::RemovePropertyHandler_IPV6_MULTICAST_ADDRESS_TABLE(const uint8_t *aValuePtr, uint16_t aValueLen)
{
    spinel_ssize_t parsedLength;
    otError error = OT_ERROR_NONE;
    otIp6Address *addrPtr;

    parsedLength = spinel_datatype_unpack(
                       aValuePtr,
                       aValueLen,
                       SPINEL_DATATYPE_IPv6ADDR_S,
                       &addrPtr
                   );

    VerifyOrExit(parsedLength > 0, error = OT_ERROR_PARSE);

    error = otIp6UnsubscribeMulticastAddress(mInstance, addrPtr);

    // If the address was not on the list, "remove" command is successful,
    // and we respond with a `SPINEL_STATUS_OK` status.
    if (error == OT_ERROR_NOT_FOUND)
    {
        error = OT_ERROR_NONE;
    }

exit:
    return error;
}

otError NcpBase::GetPropertyHandler_THREAD_RLOC16_DEBUG_PASSTHRU(void)
{
    // Note reverse logic: passthru enabled = filter disabled
    return mEncoder.WriteBool(!otIp6IsReceiveFilterEnabled(mInstance));
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

otError NcpBase::GetPropertyHandler_THREAD_OFF_MESH_ROUTES(void)
{
    otError error = OT_ERROR_NONE;
    otExternalRouteConfig routeConfig;
    otNetworkDataIterator iter = OT_NETWORK_DATA_ITERATOR_INIT;

    while (otNetDataGetNextRoute(mInstance, &iter, &routeConfig) == OT_ERROR_NONE)
    {
        SuccessOrExit(error = mEncoder.OpenStruct());

        SuccessOrExit(error = mEncoder.WriteIp6Address(routeConfig.mPrefix.mPrefix));
        SuccessOrExit(error = mEncoder.WriteUint8(routeConfig.mPrefix.mLength));
        SuccessOrExit(error = mEncoder.WriteBool(routeConfig.mStable));
        SuccessOrExit(error = mEncoder.WriteUint8(ExternalRoutePreferenceToFlagByte(routeConfig.mPreference)));
        SuccessOrExit(error = mEncoder.WriteBool(false));  // IsLocal
        SuccessOrExit(error = mEncoder.WriteBool(routeConfig.mNextHopIsThisDevice));
        SuccessOrExit(error = mEncoder.WriteUint16(routeConfig.mRloc16));

        SuccessOrExit(error = mEncoder.CloseStruct());
    }

#if OPENTHREAD_ENABLE_BORDER_ROUTER

    iter = OT_NETWORK_DATA_ITERATOR_INIT;

    while (otBorderRouterGetNextRoute(mInstance, &iter, &routeConfig) == OT_ERROR_NONE)
    {
        SuccessOrExit(error = mEncoder.OpenStruct());

        SuccessOrExit(error = mEncoder.WriteIp6Address(routeConfig.mPrefix.mPrefix));
        SuccessOrExit(error = mEncoder.WriteUint8(routeConfig.mPrefix.mLength));
        SuccessOrExit(error = mEncoder.WriteBool(routeConfig.mStable));
        SuccessOrExit(error = mEncoder.WriteUint8(ExternalRoutePreferenceToFlagByte(routeConfig.mPreference)));
        SuccessOrExit(error = mEncoder.WriteBool(true));  // IsLocal
        SuccessOrExit(error = mEncoder.WriteBool(routeConfig.mNextHopIsThisDevice));
        SuccessOrExit(error = mEncoder.WriteUint16(routeConfig.mRloc16));

        SuccessOrExit(error = mEncoder.CloseStruct());
    }
#endif // OPENTHREAD_ENABLE_BORDER_ROUTER

exit:
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

otError NcpBase::InsertPropertyHandler_THREAD_OFF_MESH_ROUTES(const uint8_t *aValuePtr, uint16_t aValueLen)
{
    spinel_ssize_t parsedLength;
    otError error = OT_ERROR_NONE;
    otExternalRouteConfig routeConfig;
    otIp6Address *addrPtr;
    bool stable = false;
    uint8_t flags = 0;

    memset(&routeConfig, 0, sizeof(otExternalRouteConfig));

    VerifyOrExit(mAllowLocalNetworkDataChange == true, error = OT_ERROR_INVALID_STATE);

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

    VerifyOrExit(parsedLength > 0, error = OT_ERROR_PARSE);

    routeConfig.mPrefix.mPrefix = *addrPtr;
    routeConfig.mStable = stable;
    routeConfig.mPreference = FlagByteToExternalRoutePreference(flags);

    error = otBorderRouterAddRoute(mInstance, &routeConfig);

exit:
    return error;
}

otError NcpBase::RemovePropertyHandler_THREAD_OFF_MESH_ROUTES(const uint8_t *aValuePtr, uint16_t aValueLen)
{
    spinel_ssize_t parsedLength;
    otError error = OT_ERROR_NONE;
    otIp6Prefix ip6Prefix;
    otIp6Address *addrPtr;

    memset(&ip6Prefix, 0, sizeof(otIp6Prefix));

    VerifyOrExit(mAllowLocalNetworkDataChange == true, error = OT_ERROR_INVALID_STATE);

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

    VerifyOrExit(parsedLength > 0, error = OT_ERROR_PARSE);

    ip6Prefix.mPrefix = *addrPtr;

    error = otBorderRouterRemoveRoute(mInstance, &ip6Prefix);

    // If the route prefix was not on the list, "remove" command is successful.
    if (error == OT_ERROR_NOT_FOUND)
    {
        error = OT_ERROR_NONE;
    }

exit:
    return error;
}
#endif // OPENTHREAD_ENABLE_BORDER_ROUTER

otError NcpBase::GetPropertyHandler_STREAM_NET(void)
{
    // TODO: Implement explicit data poll.
    return mEncoder.OverwriteWithLastStatusError(SPINEL_STATUS_UNIMPLEMENTED);
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

otError NcpBase::GetPropertyHandler_JAM_DETECT_ENABLE(void)
{
    return mEncoder.WriteBool(otJamDetectionIsEnabled(mInstance));
}

otError NcpBase::GetPropertyHandler_JAM_DETECTED(void)
{
    return mEncoder.WriteBool(otJamDetectionGetState(mInstance));
}

otError NcpBase::GetPropertyHandler_JAM_DETECT_RSSI_THRESHOLD(void)
{
    return mEncoder.WriteInt8(otJamDetectionGetRssiThreshold(mInstance));
}

otError NcpBase::GetPropertyHandler_JAM_DETECT_WINDOW(void)
{
    return mEncoder.WriteUint8(otJamDetectionGetWindow(mInstance));
}

otError NcpBase::GetPropertyHandler_JAM_DETECT_BUSY(void)
{
    return mEncoder.WriteUint8(otJamDetectionGetBusyPeriod(mInstance));
}

otError NcpBase::GetPropertyHandler_JAM_DETECT_HISTORY_BITMAP(void)
{
    otError error = OT_ERROR_NONE;
    uint64_t historyBitmap = otJamDetectionGetHistoryBitmap(mInstance);

    // History bitmap - bits 0-31
    SuccessOrExit(error = mEncoder.WriteUint32(static_cast<uint32_t>(historyBitmap & 0xffffffff)));

    // // History bitmap - bits 32-63
    SuccessOrExit(error = mEncoder.WriteUint32(static_cast<uint32_t>(historyBitmap >> 32)));

exit:
    return error;
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

otError NcpBase::GetPropertyHandler_CNTR_TX_PKT_TOTAL(void)
{
    return mEncoder.WriteUint32(otLinkGetCounters(mInstance)->mTxTotal);
}

otError NcpBase::GetPropertyHandler_CNTR_TX_PKT_ACK_REQ(void)
{
    return mEncoder.WriteUint32(otLinkGetCounters(mInstance)->mTxAckRequested);
}

otError NcpBase::GetPropertyHandler_CNTR_TX_PKT_ACKED(void)
{
    return mEncoder.WriteUint32(otLinkGetCounters(mInstance)->mTxAcked);
}

otError NcpBase::GetPropertyHandler_CNTR_TX_PKT_NO_ACK_REQ(void)
{
    return mEncoder.WriteUint32(otLinkGetCounters(mInstance)->mTxNoAckRequested);
}

otError NcpBase::GetPropertyHandler_CNTR_TX_PKT_DATA(void)
{
    return mEncoder.WriteUint32(otLinkGetCounters(mInstance)->mTxData);
}

otError NcpBase::GetPropertyHandler_CNTR_TX_PKT_DATA_POLL(void)
{
    return mEncoder.WriteUint32(otLinkGetCounters(mInstance)->mTxDataPoll);
}

otError NcpBase::GetPropertyHandler_CNTR_TX_PKT_BEACON(void)
{
    return mEncoder.WriteUint32(otLinkGetCounters(mInstance)->mTxBeacon);
}

otError NcpBase::GetPropertyHandler_CNTR_TX_PKT_BEACON_REQ(void)
{
    return mEncoder.WriteUint32(otLinkGetCounters(mInstance)->mTxBeaconRequest);
}

otError NcpBase::GetPropertyHandler_CNTR_TX_PKT_OTHER(void)
{
    return mEncoder.WriteUint32(otLinkGetCounters(mInstance)->mTxOther);
}

otError NcpBase::GetPropertyHandler_CNTR_TX_PKT_RETRY(void)
{
    return mEncoder.WriteUint32(otLinkGetCounters(mInstance)->mTxRetry);
}

otError NcpBase::GetPropertyHandler_CNTR_TX_ERR_CCA(void)
{
    return mEncoder.WriteUint32(otLinkGetCounters(mInstance)->mTxErrCca);
}

otError NcpBase::GetPropertyHandler_CNTR_TX_PKT_UNICAST(void)
{
    return mEncoder.WriteUint32(otLinkGetCounters(mInstance)->mTxUnicast);
}

otError NcpBase::GetPropertyHandler_CNTR_TX_PKT_BROADCAST(void)
{
    return mEncoder.WriteUint32(otLinkGetCounters(mInstance)->mTxBroadcast);
}

otError NcpBase::GetPropertyHandler_CNTR_TX_ERR_ABORT(void)
{
    return mEncoder.WriteUint32(otLinkGetCounters(mInstance)->mTxErrAbort);
}

otError NcpBase::GetPropertyHandler_CNTR_RX_PKT_TOTAL(void)
{
    return mEncoder.WriteUint32(otLinkGetCounters(mInstance)->mRxTotal);
}

otError NcpBase::GetPropertyHandler_CNTR_RX_PKT_DATA(void)
{
    return mEncoder.WriteUint32(otLinkGetCounters(mInstance)->mRxData);
}

otError NcpBase::GetPropertyHandler_CNTR_RX_PKT_DATA_POLL(void)
{
    return mEncoder.WriteUint32(otLinkGetCounters(mInstance)->mRxDataPoll);
}

otError NcpBase::GetPropertyHandler_CNTR_RX_PKT_BEACON(void)
{
    return mEncoder.WriteUint32(otLinkGetCounters(mInstance)->mRxBeacon);
}

otError NcpBase::GetPropertyHandler_CNTR_RX_PKT_BEACON_REQ(void)
{
    return mEncoder.WriteUint32(otLinkGetCounters(mInstance)->mRxBeaconRequest);
}

otError NcpBase::GetPropertyHandler_CNTR_RX_PKT_OTHER(void)
{
    return mEncoder.WriteUint32(otLinkGetCounters(mInstance)->mRxOther);
}

otError NcpBase::GetPropertyHandler_CNTR_RX_PKT_FILT_WL(void)
{
    return mEncoder.WriteUint32(otLinkGetCounters(mInstance)->mRxWhitelistFiltered);
}

otError NcpBase::GetPropertyHandler_CNTR_RX_PKT_FILT_DA(void)
{
    return mEncoder.WriteUint32(otLinkGetCounters(mInstance)->mRxDestAddrFiltered);
}

otError NcpBase::GetPropertyHandler_CNTR_RX_PKT_DUP(void)
{
    return mEncoder.WriteUint32(otLinkGetCounters(mInstance)->mRxDuplicated);
}

otError NcpBase::GetPropertyHandler_CNTR_RX_PKT_UNICAST(void)
{
    return mEncoder.WriteUint32(otLinkGetCounters(mInstance)->mRxUnicast);
}

otError NcpBase::GetPropertyHandler_CNTR_RX_PKT_BROADCAST(void)
{
    return mEncoder.WriteUint32(otLinkGetCounters(mInstance)->mRxBroadcast);
}

otError NcpBase::GetPropertyHandler_CNTR_RX_ERR_EMPTY(void)
{
    return mEncoder.WriteUint32(otLinkGetCounters(mInstance)->mRxErrNoFrame);
}

otError NcpBase::GetPropertyHandler_CNTR_RX_ERR_UKWN_NBR(void)
{
    return mEncoder.WriteUint32(otLinkGetCounters(mInstance)->mRxErrUnknownNeighbor);
}

otError NcpBase::GetPropertyHandler_CNTR_RX_ERR_NVLD_SADDR(void)
{
    return mEncoder.WriteUint32(otLinkGetCounters(mInstance)->mRxErrInvalidSrcAddr);
}

otError NcpBase::GetPropertyHandler_CNTR_RX_ERR_SECURITY(void)
{
    return mEncoder.WriteUint32(otLinkGetCounters(mInstance)->mRxErrSec);
}

otError NcpBase::GetPropertyHandler_CNTR_RX_ERR_BAD_FCS(void)
{
    return mEncoder.WriteUint32(otLinkGetCounters(mInstance)->mRxErrFcs);
}

otError NcpBase::GetPropertyHandler_CNTR_RX_ERR_OTHER(void)
{
    return mEncoder.WriteUint32(otLinkGetCounters(mInstance)->mRxErrOther);
}

otError NcpBase::GetPropertyHandler_CNTR_TX_IP_SEC_TOTAL(void)
{
    return mEncoder.WriteUint32(mInboundSecureIpFrameCounter);
}

otError NcpBase::GetPropertyHandler_CNTR_TX_IP_INSEC_TOTAL(void)
{
    return mEncoder.WriteUint32(mInboundInsecureIpFrameCounter);
}

otError NcpBase::GetPropertyHandler_CNTR_TX_IP_DROPPED(void)
{
    return mEncoder.WriteUint32(mDroppedInboundIpFrameCounter);
}

otError NcpBase::GetPropertyHandler_CNTR_RX_IP_SEC_TOTAL(void)
{
    return mEncoder.WriteUint32(mOutboundSecureIpFrameCounter);
}

otError NcpBase::GetPropertyHandler_CNTR_RX_IP_INSEC_TOTAL(void)
{
    return mEncoder.WriteUint32(mOutboundInsecureIpFrameCounter);
}

otError NcpBase::GetPropertyHandler_CNTR_RX_IP_DROPPED(void)
{
    return mEncoder.WriteUint32(mDroppedOutboundIpFrameCounter);
}

otError NcpBase::GetPropertyHandler_CNTR_TX_SPINEL_TOTAL(void)
{
    return mEncoder.WriteUint32(mTxSpinelFrameCounter);
}

otError NcpBase::GetPropertyHandler_CNTR_RX_SPINEL_TOTAL(void)
{
    return mEncoder.WriteUint32(mRxSpinelFrameCounter);
}

otError NcpBase::GetPropertyHandler_CNTR_RX_SPINEL_OUT_OF_ORDER_TID(void)
{
    return mEncoder.WriteUint32(mRxSpinelOutOfOrderTidCounter);
}

otError NcpBase::GetPropertyHandler_CNTR_RX_SPINEL_ERR(void)
{
    return mEncoder.WriteUint32(mFramingErrorCounter);
}

otError NcpBase::GetPropertyHandler_CNTR_IP_TX_SUCCESS(void)
{
    return mEncoder.WriteUint32(otThreadGetIp6Counters(mInstance)->mTxSuccess);
}

otError NcpBase::GetPropertyHandler_CNTR_IP_RX_SUCCESS(void)
{
    return mEncoder.WriteUint32(otThreadGetIp6Counters(mInstance)->mRxSuccess);
}

otError NcpBase::GetPropertyHandler_CNTR_IP_TX_FAILURE(void)
{
    return mEncoder.WriteUint32(otThreadGetIp6Counters(mInstance)->mTxFailure);
}

otError NcpBase::GetPropertyHandler_CNTR_IP_RX_FAILURE(void)
{
    return mEncoder.WriteUint32(otThreadGetIp6Counters(mInstance)->mRxFailure);
}

otError NcpBase::GetPropertyHandler_MSG_BUFFER_COUNTERS(void)
{
    otError error = OT_ERROR_NONE;
    otBufferInfo bufferInfo;

    otMessageGetBufferInfo(mInstance, &bufferInfo);

    SuccessOrExit(mEncoder.WriteUint16(bufferInfo.mTotalBuffers));
    SuccessOrExit(mEncoder.WriteUint16(bufferInfo.mFreeBuffers));
    SuccessOrExit(mEncoder.WriteUint16(bufferInfo.m6loSendMessages));
    SuccessOrExit(mEncoder.WriteUint16(bufferInfo.m6loSendBuffers));
    SuccessOrExit(mEncoder.WriteUint16(bufferInfo.m6loReassemblyMessages));
    SuccessOrExit(mEncoder.WriteUint16(bufferInfo.m6loReassemblyBuffers));
    SuccessOrExit(mEncoder.WriteUint16(bufferInfo.mIp6Messages));
    SuccessOrExit(mEncoder.WriteUint16(bufferInfo.mIp6Buffers));
    SuccessOrExit(mEncoder.WriteUint16(bufferInfo.mMplMessages));
    SuccessOrExit(mEncoder.WriteUint16(bufferInfo.mMplBuffers));
    SuccessOrExit(mEncoder.WriteUint16(bufferInfo.mMleMessages));
    SuccessOrExit(mEncoder.WriteUint16(bufferInfo.mMleBuffers));
    SuccessOrExit(mEncoder.WriteUint16(bufferInfo.mArpMessages));
    SuccessOrExit(mEncoder.WriteUint16(bufferInfo.mArpBuffers));
    SuccessOrExit(mEncoder.WriteUint16(bufferInfo.mCoapMessages));
    SuccessOrExit(mEncoder.WriteUint16(bufferInfo.mCoapBuffers));

exit:
    return error;
}

#if OPENTHREAD_ENABLE_MAC_FILTER

otError NcpBase::GetPropertyHandler_MAC_WHITELIST(void)
{
    otMacFilterEntry entry;
    otMacFilterIterator iterator = OT_MAC_FILTER_ITERATOR_INIT;
    otError error = OT_ERROR_NONE;

    while (otLinkFilterGetNextAddress(mInstance, &iterator, &entry) == OT_ERROR_NONE)
    {
        SuccessOrExit(error = mEncoder.OpenStruct());

        SuccessOrExit(error = mEncoder.WriteEui64(entry.mExtAddress));
        SuccessOrExit(error = mEncoder.WriteInt8(entry.mRssIn));

        SuccessOrExit(error = mEncoder.CloseStruct());
    }

exit:
    return error;
}

otError NcpBase::GetPropertyHandler_MAC_WHITELIST_ENABLED(void)
{
    return mEncoder.WriteBool(otLinkFilterGetAddressMode(mInstance) == OT_MAC_FILTER_ADDRESS_MODE_WHITELIST);
}

otError NcpBase::GetPropertyHandler_MAC_BLACKLIST(void)
{
    otMacFilterEntry entry;
    otMacFilterIterator iterator = OT_MAC_FILTER_ITERATOR_INIT;
    otError error = OT_ERROR_NONE;

    while (otLinkFilterGetNextAddress(mInstance, &iterator, &entry) == OT_ERROR_NONE)
    {
        SuccessOrExit(error = mEncoder.OpenStruct());
        SuccessOrExit(error = mEncoder.WriteEui64(entry.mExtAddress));
        SuccessOrExit(error = mEncoder.CloseStruct());
    }

exit:
    return error;
}

otError NcpBase::GetPropertyHandler_MAC_BLACKLIST_ENABLED(void)
{
    return mEncoder.WriteBool(otLinkFilterGetAddressMode(mInstance) == OT_MAC_FILTER_ADDRESS_MODE_BLACKLIST);
}

otError NcpBase::GetPropertyHandler_MAC_FIXED_RSS(void)
{
    otMacFilterEntry entry;
    otMacFilterIterator iterator = OT_MAC_FILTER_ITERATOR_INIT;
    otError error = OT_ERROR_NONE;

    while (otLinkFilterGetNextRssIn(mInstance, &iterator, &entry) == OT_ERROR_NONE)
    {
        SuccessOrExit(error = mEncoder.OpenStruct());

        SuccessOrExit(error = mEncoder.WriteEui64(entry.mExtAddress));
        SuccessOrExit(error = mEncoder.WriteInt8(entry.mRssIn));

        SuccessOrExit(error = mEncoder.CloseStruct());
    }

exit:
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

otError NcpBase::GetPropertyHandler_THREAD_MODE(void)
{
    uint8_t numericMode;
    otLinkModeConfig modeConfig = otThreadGetLinkMode(mInstance);

    numericMode = LinkFlagsToFlagByte(
                     modeConfig.mRxOnWhenIdle,
                     modeConfig.mSecureDataRequests,
                     modeConfig.mDeviceType,
                     modeConfig.mNetworkData
                  );

    return mEncoder.WriteUint8(numericMode);
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

otError NcpBase::GetPropertyHandler_THREAD_CHILD_TIMEOUT(void)
{
    return mEncoder.WriteUint32(otThreadGetChildTimeout(mInstance));
}

otError NcpBase::GetPropertyHandler_THREAD_RLOC16(void)
{
    return mEncoder.WriteUint16(otThreadGetRloc16(mInstance));
}

otError NcpBase::GetPropertyHandler_NET_REQUIRE_JOIN_EXISTING(void)
{
    return mEncoder.WriteBool(mRequireJoinExistingNetwork);
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
    spinel_status_t spinelError = SPINEL_STATUS_OK;

    parsedLength = spinel_datatype_unpack(
                       aValuePtr,
                       aValueLen,
                       SPINEL_DATATYPE_UTF8_S,
                       &string
                   );



    VerifyOrExit((parsedLength > 0) && (string != NULL), spinelError = SPINEL_STATUS_PARSE_ERROR);

    // All diagnostics related features are processed within diagnostics module
    output = otDiagProcessCmdLine(string);

    SuccessOrExit(error = mEncoder.BeginFrame(aHeader, SPINEL_CMD_PROP_VALUE_IS, aKey));
    SuccessOrExit(error = mEncoder.WriteUtf8(output));
    SuccessOrExit(error = mEncoder.EndFrame());

exit:

    if (spinelError != SPINEL_STATUS_OK)
    {
        error = SendLastStatus(aHeader, spinelError);
    }

    return error;
}

#endif // OPENTHREAD_ENABLE_DIAG

otError NcpBase::InsertPropertyHandler_THREAD_ASSISTING_PORTS(const uint8_t *aValuePtr, uint16_t aValueLen)
{
    spinel_ssize_t parsedLength;
    otError error = OT_ERROR_NONE;
    uint16_t port;

    parsedLength = spinel_datatype_unpack(
                       aValuePtr,
                       aValueLen,
                       SPINEL_DATATYPE_UINT16_S,
                       &port
                   );

    VerifyOrExit(parsedLength > 0, error = OT_ERROR_PARSE);

    error = otIp6AddUnsecurePort(mInstance, port);
exit:
    return error;
}

#if OPENTHREAD_ENABLE_MAC_FILTER

otError NcpBase::InsertPropertyHandler_MAC_WHITELIST(const uint8_t *aValuePtr, uint16_t aValueLen)
{
    spinel_ssize_t parsedLength;
    otError error = OT_ERROR_NONE;
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

    VerifyOrExit(parsedLength > 0, error = OT_ERROR_PARSE);

    error = otLinkFilterAddAddress(mInstance, extAddress);

    if (error == OT_ERROR_ALREADY)
    {
        error = OT_ERROR_NONE;
    }

    SuccessOrExit(error);

    if (rss != OT_MAC_FILTER_FIXED_RSS_DISABLED)
    {
        error = otLinkFilterAddRssIn(mInstance, extAddress, rss);
    }

exit:
    return error;
}

otError NcpBase::InsertPropertyHandler_MAC_BLACKLIST(const uint8_t *aValuePtr, uint16_t aValueLen)
{
    spinel_ssize_t parsedLength;
    otError error = OT_ERROR_NONE;
    otExtAddress *extAddress = NULL;

    parsedLength = spinel_datatype_unpack(
                       aValuePtr,
                       aValueLen,
                       SPINEL_DATATYPE_EUI64_S,
                       &extAddress
                   );

    VerifyOrExit(parsedLength > 0, error = OT_ERROR_PARSE);

    error = otLinkFilterAddAddress(mInstance, extAddress);

    if (error == OT_ERROR_ALREADY)
    {
        error = OT_ERROR_NONE;
    }

exit:
    return error;
}

otError NcpBase::InsertPropertyHandler_MAC_FIXED_RSS(const uint8_t *aValuePtr, uint16_t aValueLen)
{
    spinel_ssize_t parsedLength;
    otError error = OT_ERROR_NONE;
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

    VerifyOrExit(parsedLength > 0, error = OT_ERROR_PARSE);

    error = otLinkFilterAddRssIn(mInstance, extAddress, rss);

exit:
    return error;
}

#endif // OPENTHREAD_ENABLE_MAC_FILTER

otError NcpBase::RemovePropertyHandler_THREAD_ASSISTING_PORTS(const uint8_t *aValuePtr, uint16_t aValueLen)
{
    spinel_ssize_t parsedLength;
    otError error = OT_ERROR_NONE;
    uint16_t port;

    parsedLength = spinel_datatype_unpack(
                       aValuePtr,
                       aValueLen,
                       SPINEL_DATATYPE_UINT16_S,
                       &port
                   );

    VerifyOrExit(parsedLength > 0, error = OT_ERROR_PARSE);

    error = otIp6RemoveUnsecurePort(mInstance, port);

    // If unsecure port was not on the list, "remove" command is successful.
    if (error == OT_ERROR_NOT_FOUND)
    {
        error = OT_ERROR_NONE;
    }

exit:
    return error;
}

#if OPENTHREAD_ENABLE_MAC_FILTER

otError NcpBase::RemovePropertyHandler_MAC_WHITELIST(const uint8_t *aValuePtr, uint16_t aValueLen)
{
    spinel_ssize_t parsedLength;
    otError error = OT_ERROR_NONE;
    otExtAddress *extAddress = NULL;

    parsedLength = spinel_datatype_unpack(
                       aValuePtr,
                       aValueLen,
                       SPINEL_DATATYPE_EUI64_S,
                       &extAddress
                   );

    VerifyOrExit(parsedLength > 0, error = OT_ERROR_PARSE);

    error = otLinkFilterRemoveAddress(mInstance, extAddress);

    if (error == OT_ERROR_NOT_FOUND)
    {
        error = OT_ERROR_NONE;
    }

exit:
    return error;
}

otError NcpBase::RemovePropertyHandler_MAC_BLACKLIST(const uint8_t *aValuePtr, uint16_t aValueLen)
{
    spinel_ssize_t parsedLength;
    otError error = OT_ERROR_NONE;
    otExtAddress *extAddress = NULL;

    parsedLength = spinel_datatype_unpack(
                       aValuePtr,
                       aValueLen,
                       SPINEL_DATATYPE_EUI64_S,
                       &extAddress
                   );

    VerifyOrExit(parsedLength > 0, error = OT_ERROR_PARSE);

    error = otLinkFilterRemoveAddress(mInstance, extAddress);

    if (error == OT_ERROR_NOT_FOUND)
    {
        error = OT_ERROR_NONE;
    }

exit:
    return error;
}

otError NcpBase::RemovePropertyHandler_MAC_FIXED_RSS(const uint8_t *aValuePtr, uint16_t aValueLen)
{
    spinel_ssize_t parsedLength;
    otError error = OT_ERROR_NONE;
    otExtAddress *extAddress = NULL;

    if (aValueLen > 0)
    {
        parsedLength = spinel_datatype_unpack(
                aValuePtr,
                aValueLen,
                SPINEL_DATATYPE_EUI64_S,
                &extAddress
                );

        VerifyOrExit(parsedLength > 0, error = OT_ERROR_PARSE);
    }

    error = otLinkFilterRemoveRssIn(mInstance, extAddress);

    if (error == OT_ERROR_NOT_FOUND)
    {
        error = OT_ERROR_NONE;
    }

exit:
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
otError NcpBase::GetPropertyHandler_NEST_LEGACY_ULA_PREFIX(void)
{
    return mEncoder.WriteData(mLegacyUlaPrefix, sizeof(mLegacyUlaPrefix));
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

otError NcpBase::GetPropertyHandler_NEST_LEGACY_LAST_NODE_JOINED(void)
{

    if (!mLegacyNodeDidJoin)
    {
        memset(&mLegacyLastJoinedNode, 0, sizeof(mLegacyLastJoinedNode));
    }

    return mEncoder.WriteEui64(mLegacyLastJoinedNode);
}
#endif // OPENTHREAD_ENABLE_LEGACY

otError NcpBase::GetPropertyHandler_ChannelMaskHelper(uint32_t aChannelMask)
{
    otError error = OT_ERROR_NONE;

    for (uint8_t i = 0; i < 32; i++)
    {
        if (0 != (aChannelMask & (1 << i)))
        {
            SuccessOrExit(error = mEncoder.WriteUint8(i));
        }
    }

exit:
    return error;
}


otError NcpBase::GetPropertyHandler_MAC_SCAN_MASK(void)
{
    return GetPropertyHandler_ChannelMaskHelper(mChannelMask);
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

otError NcpBase::GetPropertyHandler_MAC_SCAN_PERIOD(void)
{
    return mEncoder.WriteUint16(mScanPeriod);
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

otError NcpBase::GetPropertyHandler_MAC_SCAN_STATE(void)
{
    uint8_t scanState = SPINEL_SCAN_STATE_IDLE;

#if OPENTHREAD_ENABLE_RAW_LINK_API

    if (otLinkRawIsEnabled(mInstance))
    {
        scanState = (mCurScanChannel == kInvalidScanChannel) ? SPINEL_SCAN_STATE_IDLE : SPINEL_SCAN_STATE_ENERGY;
    }
    else

#endif // OPENTHREAD_ENABLE_RAW_LINK_API

    {
        if (otLinkIsActiveScanInProgress(mInstance))
        {
            scanState = SPINEL_SCAN_STATE_BEACON;

        }
        else if (otLinkIsEnergyScanInProgress(mInstance))
        {
            scanState = SPINEL_SCAN_STATE_ENERGY;
        }
        else if (otThreadIsDiscoverInProgress(mInstance))
        {
            scanState = SPINEL_SCAN_STATE_DISCOVER;
        }
        else
        {
            scanState = SPINEL_SCAN_STATE_IDLE;
        }
    }

    return mEncoder.WriteUint8(scanState);
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
    otError error = OT_ERROR_NONE;

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

        SuccessOrExit(error = mEncoder.BeginFrame(
                                  SPINEL_HEADER_FLAG | SPINEL_HEADER_IID_0,
                                  SPINEL_CMD_PROP_VALUE_INSERTED,
                                  SPINEL_PROP_MAC_SCAN_BEACON
                              ));
        SuccessOrExit(error = mEncoder.WriteUint8(aResult->mChannel));
        SuccessOrExit(error = mEncoder.WriteInt8(aResult->mRssi));

        SuccessOrExit(error = mEncoder.OpenStruct());                       // "mac-layer data"
        SuccessOrExit(error = mEncoder.WriteEui64(aResult->mExtAddress));
        SuccessOrExit(error = mEncoder.WriteUint16(0xffff));                // short address, not given
        SuccessOrExit(error = mEncoder.WriteUint16(aResult->mPanId));
        SuccessOrExit(error = mEncoder.WriteUint8(aResult->mLqi));
        SuccessOrExit(error = mEncoder.CloseStruct());

        SuccessOrExit(error = mEncoder.OpenStruct());                       // "net-layer data"
        SuccessOrExit(error = mEncoder.WriteUintPacked(SPINEL_PROTOCOL_TYPE_THREAD));  // type
        SuccessOrExit(error = mEncoder.WriteUint8(flags));
        SuccessOrExit(error = mEncoder.WriteUtf8(aResult->mNetworkName.m8));
        SuccessOrExit(error = mEncoder.WriteDataWithLen(aResult->mExtendedPanId.m8, OT_EXT_PAN_ID_SIZE));
        SuccessOrExit(error = mEncoder.WriteDataWithLen(aResult->mSteeringData.m8, aResult->mSteeringData.mLength));
        SuccessOrExit(error = mEncoder.CloseStruct());

        SuccessOrExit(error = mEncoder.EndFrame());
    }
    else
    {
        // We are finished with the scan, send an unsolicated
        // scan state update.
        mChangedPropsSet.AddProperty(SPINEL_PROP_MAC_SCAN_STATE);
        mUpdateChangedPropsTask.Post();
    }

exit:

    if (error != OT_ERROR_NONE)
    {
        // We ran out of buffer adding a scan result so remember to send
        // an async `LAST_STATUS(NOMEM)` when buffer space becomes
        // available.
        mChangedPropsSet.AddLastStatus(SPINEL_STATUS_NOMEM);
        mUpdateChangedPropsTask.Post();
    }
}

void NcpBase::HandleEnergyScanResult_Jump(otEnergyScanResult *aResult, void *aContext)
{
    static_cast<NcpBase *>(aContext)->HandleEnergyScanResult(aResult);
}

void NcpBase::HandleEnergyScanResult(otEnergyScanResult *aResult)
{
    otError error = OT_ERROR_NONE;

    if (aResult)
    {
        SuccessOrExit(error = mEncoder.BeginFrame(
                                  SPINEL_HEADER_FLAG | SPINEL_HEADER_IID_0,
                                  SPINEL_CMD_PROP_VALUE_INSERTED,
                                  SPINEL_PROP_MAC_SCAN_BEACON
                              ));
        SuccessOrExit(error = mEncoder.WriteUint8(aResult->mChannel));
        SuccessOrExit(error = mEncoder.WriteInt8(aResult->mMaxRssi));
        SuccessOrExit(error = mEncoder.EndFrame());
    }
    else
    {
        // We are finished with the scan, send an unsolicated
        // scan state update.
        mChangedPropsSet.AddProperty(SPINEL_PROP_MAC_SCAN_STATE);
        mUpdateChangedPropsTask.Post();
    }

exit:

    if (error != OT_ERROR_NONE)
    {
        mChangedPropsSet.AddLastStatus(SPINEL_STATUS_NOMEM);
        mUpdateChangedPropsTask.Post();
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

    SuccessOrExit(error = mEncoder.BeginFrame(
                              header,
                              SPINEL_CMD_PROP_VALUE_IS,
                              isSecure ? SPINEL_PROP_STREAM_NET : SPINEL_PROP_STREAM_NET_INSECURE
                          ));

    SuccessOrExit(error = mEncoder.WriteUint16(length));
    SuccessOrExit(error = mEncoder.WriteMessage(aMessage));

    // Set the `aMessage` pointer to NULL to indicate that it does
    // not need to be freed at the exit. The `aMessage` is now owned
    // by the OutboundFrame and will be freed when the frame is either
    // successfully sent and then removed, or if the frame gets
    // discarded.
    aMessage = NULL;

    // Append any metadata (rssi, lqi, channel, etc) here!

    SuccessOrExit(error = mEncoder.EndFrame());

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
