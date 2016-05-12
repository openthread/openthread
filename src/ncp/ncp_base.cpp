/*
 *    Copyright (c) 2016, Nest Labs, Inc.
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
 *   This file implements a Spinel interface to the OpenThread stack.
 */

#include <assert.h>
#include <common/code_utils.hpp>
#include <ncp/ncp.hpp>
#include <openthread.h>
#include <openthread-config.h>
#include <stdarg.h>
#include <platform/radio.h>

namespace Thread {

extern ThreadNetif *sThreadNetif;

static spinel_status_t ThreadErrorToSpinelStatus(ThreadError error)
{
    spinel_status_t ret;

    switch (error)
    {
    case kThreadError_None:
        ret = SPINEL_STATUS_OK;
        break;

    case kThreadError_Failed:
        ret = SPINEL_STATUS_FAILURE;
        break;

    case kThreadError_Drop:
        ret = SPINEL_STATUS_DROPPED;
        break;

    case kThreadError_NoBufs:
        ret = SPINEL_STATUS_NOMEM;
        break;

    case kThreadError_Busy:
        ret = SPINEL_STATUS_BUSY;
        break;

    case kThreadError_Parse:
        ret = SPINEL_STATUS_PARSE_ERROR;
        break;

    case kThreadError_InvalidArgs:
        ret = SPINEL_STATUS_INVALID_ARGUMENT;
        break;

    case kThreadError_NotImplemented:
        ret = SPINEL_STATUS_UNIMPLEMENTED;
        break;

    case kThreadError_InvalidState:
        ret = SPINEL_STATUS_INVALID_STATE;
        break;

    default:
        ret = SPINEL_STATUS_FAILURE;
        break;
    }

    return ret;
}

NcpBase::NcpBase():
    mNetifHandler(&HandleUnicastAddressesChanged, this),
    mUpdateAddressesTask(&RunUpdateAddressesTask, this)
{
    mChannelMask = (0xFFFF << 11); // Default to all channels
}

ThreadError NcpBase::Start()
{
    assert(sThreadNetif != NULL);
    sThreadNetif->RegisterHandler(mNetifHandler);
    Ip6::Ip6::SetNcpReceivedHandler(&HandleReceivedDatagram, this);
    return kThreadError_None;
}

ThreadError NcpBase::Stop()
{
    return kThreadError_None;
}

void NcpBase::HandleReceivedDatagram(void *context, Message &message)
{
    NcpBase *obj = reinterpret_cast<NcpBase *>(context);
    obj->HandleReceivedDatagram(message);
}

void NcpBase::HandleReceivedDatagram(Message &message)
{
    ThreadError errorCode;

    if (mSending == false)
    {
        errorCode = OutboundFrameBegin();

        if (errorCode == kThreadError_None)
        {
            errorCode = OutboundFrameFeedPacked(
                            "CiiS",
                            SPINEL_HEADER_FLAG | SPINEL_HEADER_IID_0,
                            SPINEL_CMD_PROP_VALUE_IS,
                            SPINEL_PROP_STREAM_NET,
                            message.GetLength()
                        );
        }

        if (errorCode == kThreadError_None)
        {
            errorCode = OutboundFrameFeedMessage(message);
        }

        // TODO: Append any metadata here!

        if (errorCode == kThreadError_None)
        {
            errorCode = OutboundFrameSend();
        }

        if (errorCode != kThreadError_None)
        {
            SendLastStatus(SPINEL_HEADER_FLAG | SPINEL_HEADER_IID_0, SPINEL_STATUS_DROPPED);
        }
    }
    else
    {
        if (mSendQueue.Enqueue(message) != kThreadError_None)
        {
            Message::Free(message);
        }
    }
}

static NcpBase *gActiveScanContextHack = NULL;

void NcpBase::HandleActiveScanResult_Jump(otActiveScanResult *result)
{
    if (gActiveScanContextHack)
    {
        gActiveScanContextHack->HandleActiveScanResult(result);
    }
}

void NcpBase::HandleActiveScanResult(otActiveScanResult *result)
{
    VerifyOrExit(mSending == false, ;);

    if (result)
    {
        uint8_t flags = (result->mVersion << SPINEL_BEACON_THREAD_FLAG_VERSION_SHIFT);

        if (result->mIsJoinable)
        {
            flags |= SPINEL_BEACON_THREAD_FLAG_JOINABLE;
        }

        if (result->mIsNative)
        {
            flags |= SPINEL_BEACON_THREAD_FLAG_NATIVE;
        }

        //chan,rssi,(laddr,saddr,panid,lqi),(proto,flags,networkid,xpanid) [icT(ESSC)T(iCUD.).]
        NcpBase::SendPropteryUpdate(
            SPINEL_HEADER_FLAG | SPINEL_HEADER_IID_0,
            SPINEL_CMD_PROP_VALUE_INSERTED,
            SPINEL_PROP_MAC_SCAN_BEACON,
            "CcT(ESSC)T(iCUD.).",
            result->mChannel,//chan
            result->mRssi,//rssi
            result->mExtAddress.m8,// laddr
            0xFFFF, // saddr, Not given
            result->mPanId,//panid
            0xFF,//lqi, not given
            SPINEL_PROTOCOL_TYPE_THREAD,//proto
            flags,
            result->mNetworkName,//networkid
            result->mExtPanId, sizeof(result->mExtPanId) //xpanid
        );
    }
    else
    {
        SendPropteryUpdate(
            SPINEL_HEADER_FLAG | SPINEL_HEADER_IID_0,
            SPINEL_CMD_PROP_VALUE_IS,
            SPINEL_PROP_MAC_SCAN_STATE,
            SPINEL_DATATYPE_UINT8_S,
            SPINEL_SCAN_STATE_IDLE
        );
    }

exit:
    return;
}


void NcpBase::HandleUnicastAddressesChanged(void *context)
{
    NcpBase *obj = reinterpret_cast<NcpBase *>(context);
    obj->mUpdateAddressesTask.Post();
}

void NcpBase::RunUpdateAddressesTask(void *context)
{
    NcpBase *obj = reinterpret_cast<NcpBase *>(context);
    obj->RunUpdateAddressesTask();
}

void NcpBase::RunUpdateAddressesTask()
{
    VerifyOrExit(mSending == false, ;);

    // It would really be preferable to have inserted/removed notifications
    // for the individual addresses, rather than a single "changed" event.
    HandleCommandPropertyGet(SPINEL_HEADER_FLAG | SPINEL_HEADER_IID_0, SPINEL_PROP_IPV6_ADDRESS_TABLE);

    HandleCommandPropertyGet(SPINEL_HEADER_FLAG | SPINEL_HEADER_IID_0, SPINEL_PROP_NET_STATE);

exit:
    return;
}

// ============================================================
//     Serial channel message callbacks
// ============================================================

void NcpBase::HandleReceive(void *context, const uint8_t *buf, uint16_t bufLength)
{
    NcpBase *obj = reinterpret_cast<NcpBase *>(context);
    obj->HandleReceive(buf, bufLength);
}

void NcpBase::HandleReceive(const uint8_t *buf, uint16_t bufLength)
{
    uint8_t header = 0;
    unsigned int command = 0;
    spinel_ssize_t parsedLength;
    const uint8_t *arg_ptr = NULL;
    unsigned int arg_len = 0;

    parsedLength = spinel_datatype_unpack(buf, bufLength, "CiD", &header, &command, &arg_ptr, &arg_len);

    if (parsedLength == bufLength)
    {
        HandleCommand(header, command, arg_ptr, static_cast<uint16_t>(arg_len));
    }
    else
    {
        SendLastStatus(header, SPINEL_STATUS_PARSE_ERROR);
    }
}

void NcpBase::HandleCommand(uint8_t header, unsigned int command, const uint8_t *arg_ptr, uint16_t arg_len)
{
    unsigned int propKey = 0;
    spinel_ssize_t parsedLength;
    const uint8_t *value_ptr;
    unsigned int value_len;

    if ((SPINEL_HEADER_FLAG & header) != SPINEL_HEADER_FLAG)
    {
        // Skip
        return;
    }

    // We only support IID zero for now.
    if (SPINEL_HEADER_GET_IID(header) != 0)
    {
        SendLastStatus(header, SPINEL_STATUS_INVALID_INTERFACE);
        return;
    }

    switch (command)
    {
    case SPINEL_CMD_NOOP:
        SendLastStatus(header, SPINEL_STATUS_OK);
        break;

    case SPINEL_CMD_RESET:
        // TODO: Figure out how to actually perform a reset.
        SendLastStatus(0, SPINEL_STATUS_RESET_SOFTWARE);
        break;

    case SPINEL_CMD_PROP_VALUE_GET:
        parsedLength = spinel_datatype_unpack(arg_ptr, arg_len, "i", &propKey);

        if (parsedLength > 0)
        {
            HandleCommandPropertyGet(header, static_cast<spinel_prop_key_t>(propKey));
        }
        else
        {
            SendLastStatus(header, SPINEL_STATUS_PARSE_ERROR);
        }

        break;

    case SPINEL_CMD_PROP_VALUE_SET:
        parsedLength = spinel_datatype_unpack(arg_ptr, arg_len, "iD", &propKey, &value_ptr, &value_len);

        if (parsedLength == arg_len)
        {
            HandleCommandPropertySet(header, static_cast<spinel_prop_key_t>(propKey), value_ptr, value_len);
        }
        else
        {
            SendLastStatus(header, SPINEL_STATUS_PARSE_ERROR);
        }

        break;

    case SPINEL_CMD_PROP_VALUE_INSERT:
        parsedLength = spinel_datatype_unpack(arg_ptr, arg_len, "iD", &propKey, &value_ptr, &value_len);

        if (parsedLength == arg_len)
        {
            HandleCommandPropertyInsert(header, static_cast<spinel_prop_key_t>(propKey), value_ptr, value_len);
        }
        else
        {
            SendLastStatus(header, SPINEL_STATUS_PARSE_ERROR);
        }

        break;

    case SPINEL_CMD_PROP_VALUE_REMOVE:
        parsedLength = spinel_datatype_unpack(arg_ptr, arg_len, "iD", &propKey, &value_ptr, &value_len);

        if (parsedLength == arg_len)
        {
            HandleCommandPropertyRemove(header, static_cast<spinel_prop_key_t>(propKey), value_ptr, value_len);
        }
        else
        {
            SendLastStatus(header, SPINEL_STATUS_PARSE_ERROR);
        }

        break;

    default:
        SendLastStatus(header, SPINEL_STATUS_INVALID_COMMAND);
        break;
    }
}

void NcpBase::HandleCommandPropertyGet(uint8_t header, spinel_prop_key_t key)
{
    const uint8_t *ptr;
    uint8_t len, tmp;

    if (mSending)
    {
        // If we are currently sending, can can queue up to one
        // property get that we can execute immediately after
        // the send is complete.
        if (mQueuedGetHeader == 0)
        {
            mQueuedGetHeader = header;
            mQueuedGetKey = key;
        }

        return;
    }

    switch (key)
    {
    case SPINEL_PROP_LOCK:
    case SPINEL_PROP_MAC_SCAN_MASK:
    case SPINEL_PROP_PHY_TX_POWER:
        SendLastStatus(header, SPINEL_STATUS_UNIMPLEMENTED);
        break;

    case SPINEL_PROP_LAST_STATUS:
        SendPropteryUpdate(header, SPINEL_CMD_PROP_VALUE_IS, key, SPINEL_DATATYPE_UINT_PACKED_S, mLastStatus);
        break;

    case SPINEL_PROP_PROTOCOL_VERSION:
        SendPropteryUpdate(
            header,
            SPINEL_CMD_PROP_VALUE_IS,
            key,
            SPINEL_DATATYPE_UINT_PACKED_S SPINEL_DATATYPE_UINT_PACKED_S SPINEL_DATATYPE_UINT_PACKED_S,
            SPINEL_PROTOCOL_TYPE_THREAD,
            SPINEL_PROTOCOL_VERSION_THREAD_MAJOR,
            SPINEL_PROTOCOL_VERSION_THREAD_MINOR
        );
        break;

    case SPINEL_PROP_CAPABILITIES:
        SendPropteryUpdate(
            header,
            SPINEL_CMD_PROP_VALUE_IS,
            key,
            SPINEL_DATATYPE_UINT_PACKED_S
            SPINEL_DATATYPE_UINT_PACKED_S
            SPINEL_DATATYPE_UINT_PACKED_S,
            SPINEL_CAP_ROLE_ROUTER,
            SPINEL_CAP_NET_THREAD_1_0,
            SPINEL_CAP_802_15_4_2450MHZ_OQPSK
        );
        break;

    case SPINEL_PROP_NCP_VERSION:
        SendPropteryUpdate(
            header,
            SPINEL_CMD_PROP_VALUE_IS,
            key,
            SPINEL_DATATYPE_UTF8_S,
            PACKAGE_NAME "/" PACKAGE_VERSION "; " __DATE__ " " __TIME__
        );
        break;

    case SPINEL_PROP_INTERFACE_COUNT:
        SendPropteryUpdate(
            header,
            SPINEL_CMD_PROP_VALUE_IS,
            key,
            SPINEL_DATATYPE_UINT8_S,
            1 // Only one interface for now
        );
        break;

    case SPINEL_PROP_POWER_STATE:
        // Always online at the moment
        SendPropteryUpdate(
            header,
            SPINEL_CMD_PROP_VALUE_IS,
            key,
            SPINEL_DATATYPE_UINT8_S,
            SPINEL_POWER_STATE_ONLINE
        );
        break;

    case SPINEL_PROP_NET_NETWORK_NAME:
        SendPropteryUpdate(
            header,
            SPINEL_CMD_PROP_VALUE_IS,
            key,
            SPINEL_DATATYPE_UTF8_S,
            otGetNetworkName()
        );
        break;

    case SPINEL_PROP_MAC_15_4_PANID:
        SendPropteryUpdate(
            header,
            SPINEL_CMD_PROP_VALUE_IS,
            key,
            SPINEL_DATATYPE_UINT16_S,
            otGetPanId()
        );
        break;

    case SPINEL_PROP_PHY_FREQ:
    {
        uint32_t freq_khz;
        uint8_t chan = otGetChannel();

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
            freq_khz = 2405 - (5000 * 11) + 5000 * (chan);
        }
        else
        {
            SendLastStatus(header, SPINEL_STATUS_FAILURE);
            return;
        }

        SendPropteryUpdate(
            header,
            SPINEL_CMD_PROP_VALUE_IS,
            key,
            SPINEL_DATATYPE_UINT32_S,
            &freq_khz
        );
        break;
    }

    case SPINEL_PROP_PHY_CHAN_SUPPORTED:
    {
        static const uint8_t supported_channels[] =
        {
            11, 12, 13, 14, 15, 16, 17, 18,
            19, 20, 21, 22, 23, 24, 25, 26
        };
        SendPropteryUpdate(
            header,
            SPINEL_CMD_PROP_VALUE_IS,
            key,
            SPINEL_DATATYPE_DATA_S,
            supported_channels,
            sizeof(supported_channels)
        );
        break;
    }

    case SPINEL_PROP_PHY_CHAN:
        SendPropteryUpdate(
            header,
            SPINEL_CMD_PROP_VALUE_IS,
            key,
            SPINEL_DATATYPE_UINT8_S,
            otGetChannel()
        );
        break;

    case SPINEL_PROP_HWADDR:
    case SPINEL_PROP_MAC_15_4_LADDR:
        // TODO: Figure out how these cases should be differentiated.
        SendPropteryUpdate(
            header,
            SPINEL_CMD_PROP_VALUE_IS,
            key,
            SPINEL_DATATYPE_EUI64_S,
            otGetExtendedAddress()
        );
        break;

    case SPINEL_PROP_MAC_15_4_SADDR:
        SendPropteryUpdate(
            header,
            SPINEL_CMD_PROP_VALUE_IS,
            key,
            SPINEL_DATATYPE_UINT16_S,
            sThreadNetif->GetMac().GetShortAddress()
        );
        break;

    case SPINEL_PROP_NET_XPANID:
        SendPropteryUpdate(
            header,
            SPINEL_CMD_PROP_VALUE_IS,
            key,
            SPINEL_DATATYPE_DATA_S,
            otGetExtendedPanId(),
            sizeof(spinel_net_xpanid_t)
        );
        break;

    case SPINEL_PROP_NET_MASTER_KEY:
        ptr = otGetMasterKey(&len);
        SendPropteryUpdate(
            header,
            SPINEL_CMD_PROP_VALUE_IS,
            key,
            SPINEL_DATATYPE_DATA_S,
            ptr,
            len
        );
        break;

    case SPINEL_PROP_NET_KEY_SEQUENCE:
        SendPropteryUpdate(
            header,
            SPINEL_CMD_PROP_VALUE_IS,
            key,
            SPINEL_DATATYPE_UINT32_S,
            otGetKeySequenceCounter()
        );
        break;

    case SPINEL_PROP_PHY_RSSI:
        SendPropteryUpdate(
            header,
            SPINEL_CMD_PROP_VALUE_IS,
            key,
            SPINEL_DATATYPE_INT8_S,
            otPlatRadioGetNoiseFloor()
        );
        break;

    case SPINEL_PROP_NET_PARTITION_ID:
        SendPropteryUpdate(
            header,
            SPINEL_CMD_PROP_VALUE_IS,
            key,
            SPINEL_DATATYPE_UINT32_S,
            otGetPartitionId()
        );
        break;

    case SPINEL_PROP_NET_ENABLED:
        tmp = (otGetDeviceRole() != kDeviceRoleDisabled);
        SendPropteryUpdate(
            header,
            SPINEL_CMD_PROP_VALUE_IS,
            key,
            SPINEL_DATATYPE_UINT8_S,
            tmp
        );
        break;

    case SPINEL_PROP_NET_STATE:
        switch (otGetDeviceRole())
        {
        case kDeviceRoleDisabled:
            tmp = SPINEL_NET_STATE_OFFLINE;
            break;

        case kDeviceRoleDetached:
            tmp = SPINEL_NET_STATE_DETACHED;
            break;

        case kDeviceRoleChild:
        case kDeviceRoleRouter:
        case kDeviceRoleLeader:
            tmp = SPINEL_NET_STATE_ATTACHED;
            break;
        }

        SendPropteryUpdate(
            header,
            SPINEL_CMD_PROP_VALUE_IS,
            key,
            SPINEL_DATATYPE_UINT8_S,
            tmp
        );
        break;

    case SPINEL_PROP_NET_ROLE:
        switch (otGetDeviceRole())
        {
        case kDeviceRoleDisabled:
        case kDeviceRoleDetached:
            tmp = SPINEL_NET_ROLE_NONE;
            break;

        case kDeviceRoleChild:
            tmp = SPINEL_NET_ROLE_CHILD;
            break;

        case kDeviceRoleRouter:
            tmp = SPINEL_NET_ROLE_ROUTER;
            break;

        case kDeviceRoleLeader:
            tmp = SPINEL_NET_ROLE_LEADER;
            break;
        }

        SendPropteryUpdate(
            header,
            SPINEL_CMD_PROP_VALUE_IS,
            key,
            SPINEL_DATATYPE_UINT8_S,
            tmp
        );
        break;

    case SPINEL_PROP_THREAD_LEADER:
    {
        ThreadError errorCode;
        Ip6::Address address;
        errorCode = sThreadNetif->GetMle().GetLeaderAddress(address);

        if (errorCode)
        {
            SendLastStatus(header, ThreadErrorToSpinelStatus(errorCode));
        }
        else
        {
            SendPropteryUpdate(
                header,
                SPINEL_CMD_PROP_VALUE_IS,
                key,
                SPINEL_DATATYPE_IPv6ADDR_S,
                &address
            );
        }

        break;
    }

    case SPINEL_PROP_IPV6_ML_PREFIX:
    {
        const uint8_t *ml_prefix = sThreadNetif->GetMle().GetMeshLocalPrefix();

        if (ml_prefix)
        {
            SendPropteryUpdate(
                header,
                SPINEL_CMD_PROP_VALUE_IS,
                key,
                SPINEL_DATATYPE_IPv6ADDR_S SPINEL_DATATYPE_UINT8_S,
                ml_prefix,
                64
            );
        }
        else
        {
            SendPropteryUpdate(
                header,
                SPINEL_CMD_PROP_VALUE_IS,
                key,
                SPINEL_DATATYPE_VOID_S
            );
        }

        break;
    }

    case SPINEL_PROP_IPV6_ADDRESS_TABLE:
        HandleCommandPropertyGetAddressList(header);
        break;

    case SPINEL_PROP_IPV6_ROUTE_TABLE:
        HandleCommandPropertyGetRoutingTable(header);
        break;

    case SPINEL_PROP_MAC_SCAN_STATE:
        if (otActiveScanInProgress())
        {
            SendPropteryUpdate(
                header,
                SPINEL_CMD_PROP_VALUE_IS,
                key,
                SPINEL_DATATYPE_UINT8_S,
                SPINEL_SCAN_STATE_BEACON
            );
        }
        else
        {
            SendPropteryUpdate(
                header,
                SPINEL_CMD_PROP_VALUE_IS,
                key,
                SPINEL_DATATYPE_UINT8_S,
                SPINEL_SCAN_STATE_IDLE
            );
        }

        break;

    case SPINEL_PROP_STREAM_NET:
    case SPINEL_PROP_STREAM_NET_INSECURE:
    case SPINEL_PROP_STREAM_DEBUG:
    case SPINEL_PROP_STREAM_RAW:
    case SPINEL_PROP_MAC_SCAN_BEACON:
        // These properties don't have a "Getter"
        SendLastStatus(header, SPINEL_STATUS_FAILURE);
        break;

    default:
        SendLastStatus(header, SPINEL_STATUS_PROPERTY_NOT_FOUND);
        break;
    }
}

void NcpBase::HandleCommandPropertySet(uint8_t header, spinel_prop_key_t key, const uint8_t *value_ptr,
                                       uint16_t value_len)
{
    const uint8_t *ptr = NULL;
    unsigned int i = 0;
    uint16_t tmp;
    const char *string = NULL;
    spinel_ssize_t parsedLength;
    spinel_size_t len;
    ThreadError errorCode = kThreadError_None;

    switch (key)
    {
    case SPINEL_PROP_LAST_STATUS:
    case SPINEL_PROP_PROTOCOL_VERSION:
    case SPINEL_PROP_CAPABILITIES:
    case SPINEL_PROP_NCP_VERSION:
    case SPINEL_PROP_STREAM_DEBUG:
    case SPINEL_PROP_MAC_SCAN_BEACON:
    case SPINEL_PROP_NET_PARTITION_ID:
    case SPINEL_PROP_PHY_FREQ:
    case SPINEL_PROP_IPV6_ADDRESS_TABLE:
    case SPINEL_PROP_IPV6_ROUTE_TABLE:
    case SPINEL_PROP_PHY_RSSI:
    case SPINEL_PROP_INTERFACE_COUNT:
        // These properties don't have a "Setter"
        SendLastStatus(header, SPINEL_STATUS_FAILURE);
        break;

    case SPINEL_PROP_MAC_SCAN_MASK:
    case SPINEL_PROP_POWER_STATE:
    case SPINEL_PROP_PHY_TX_POWER:
        SendLastStatus(header, SPINEL_STATUS_UNIMPLEMENTED);
        break;

    case SPINEL_PROP_IPV6_ML_PREFIX:
        parsedLength = spinel_datatype_unpack(
                           value_ptr,
                           value_len,
                           SPINEL_DATATYPE_IPv6ADDR_S,
                           &ptr
                       );

        if (parsedLength > 0)
        {
            errorCode = sThreadNetif->GetMle().SetMeshLocalPrefix(ptr);
            HandleCommandPropertyGet(header, key);
        }
        else
        {
            errorCode = kThreadError_Parse;
        }

        if (errorCode == kThreadError_None)
        {
            HandleCommandPropertyGet(header, key);
        }
        else
        {
            SendLastStatus(header, ThreadErrorToSpinelStatus(errorCode));
        }

        break;

    case SPINEL_PROP_NET_ENABLED:
    {
        bool value = false;

        parsedLength = spinel_datatype_unpack(
                           value_ptr,
                           value_len,
                           SPINEL_DATATYPE_BOOL_S,
                           &value
                       );

        if (parsedLength > 0)
        {
            if (value == false)
            {
                errorCode = otDisable();
            }
            else
            {
                errorCode = otEnable();
            }
        }
        else
        {
            errorCode = kThreadError_Parse;
        }

        if (errorCode == kThreadError_None)
        {
            HandleCommandPropertyGet(header, key);
        }
        else
        {
            SendLastStatus(header, ThreadErrorToSpinelStatus(errorCode));
        }

        break;
    }

    case SPINEL_PROP_NET_STATE:
        parsedLength = spinel_datatype_unpack(
                           value_ptr,
                           value_len,
                           SPINEL_DATATYPE_UINT_PACKED_S,
                           &i
                       );

        if (parsedLength > 0)
        {
            switch (i)
            {
            case SPINEL_NET_STATE_OFFLINE:
                if (otGetDeviceRole() != kDeviceRoleDisabled)
                {
                    errorCode = otDisable();
                }

                break;

            case SPINEL_NET_STATE_DETACHED:
                if (otGetDeviceRole() == kDeviceRoleDisabled)
                {
                    errorCode = otEnable();

                    if (errorCode == kThreadError_None)
                    {
                        errorCode = otBecomeDetached();
                    }
                }
                else if (otGetDeviceRole() != kDeviceRoleDetached)
                {
                    errorCode = otBecomeDetached();
                }

                break;

            case SPINEL_NET_STATE_ATTACHING:
            case SPINEL_NET_STATE_ATTACHED:
                if (otGetDeviceRole() == kDeviceRoleDisabled)
                {
                    errorCode = otEnable();
                }

                if (otGetDeviceRole() == kDeviceRoleDetached)
                {
                    errorCode = otBecomeRouter();

                    if (errorCode == kThreadError_None)
                    {
                        SendPropteryUpdate(
                            header,
                            SPINEL_CMD_PROP_VALUE_IS,
                            key,
                            SPINEL_DATATYPE_UINT8_S,
                            SPINEL_NET_STATE_ATTACHING
                        );
                        return;
                    }
                }

                break;
            }

            if (errorCode == kThreadError_None)
            {
                HandleCommandPropertyGet(header, key);
            }
            else
            {
                SendLastStatus(header, ThreadErrorToSpinelStatus(errorCode));
            }
        }
        else
        {
            SendLastStatus(header, SPINEL_STATUS_PARSE_ERROR);
        }

        break;

    case SPINEL_PROP_NET_ROLE:
        parsedLength = spinel_datatype_unpack(
                           value_ptr,
                           value_len,
                           SPINEL_DATATYPE_UINT_PACKED_S,
                           &i
                       );

        if (parsedLength > 0)
        {
            switch (i)
            {
            case SPINEL_NET_ROLE_NONE:
                errorCode = kThreadError_InvalidArgs;
                break;

            case SPINEL_NET_ROLE_ROUTER:
                errorCode = otBecomeRouter();
                break;

            case SPINEL_NET_ROLE_LEADER:
                errorCode = otBecomeLeader();
                break;

            case SPINEL_NET_ROLE_CHILD:
                errorCode = otBecomeChild(kMleAttachAnyPartition);
                break;
            }

            if (errorCode == kThreadError_None)
            {
                HandleCommandPropertyGet(header, key);
            }
            else
            {
                SendLastStatus(header, ThreadErrorToSpinelStatus(errorCode));
            }
        }
        else
        {
            SendLastStatus(header, SPINEL_STATUS_PARSE_ERROR);
        }

        break;

    case SPINEL_PROP_MAC_SCAN_STATE:
        parsedLength = spinel_datatype_unpack(
                           value_ptr,
                           value_len,
                           SPINEL_DATATYPE_UINT_PACKED_S,
                           &i
                       );

        if (parsedLength > 0)
        {
            switch (i)
            {
            case SPINEL_SCAN_STATE_IDLE:
                errorCode = kThreadError_None;
                break;

            case SPINEL_SCAN_STATE_BEACON:
                gActiveScanContextHack = this;
                errorCode = otActiveScan((mChannelMask >> kPhyMinChannel), 200, &HandleActiveScanResult_Jump);
                break;

            case SPINEL_SCAN_STATE_ENERGY:
                errorCode = kThreadError_NotImplemented;
                break;

            default:
                errorCode = kThreadError_InvalidArgs;
                break;
            }

            if (errorCode == kThreadError_None)
            {
                HandleCommandPropertyGet(header, key);
            }
            else
            {
                SendLastStatus(header, ThreadErrorToSpinelStatus(errorCode));
            }
        }
        else
        {
            SendLastStatus(header, SPINEL_STATUS_PARSE_ERROR);
        }

        break;

    case SPINEL_PROP_STREAM_NET_INSECURE:
        SendLastStatus(header, SPINEL_STATUS_UNIMPLEMENTED);
        break;

    case SPINEL_PROP_STREAM_NET:
    {
        const uint8_t *frame_ptr(NULL);
        unsigned int frame_len(0);
        const uint8_t *meta_ptr(NULL);
        unsigned int meta_len(0);
        Message *message(Ip6::Ip6::NewMessage(0));

        if (message == NULL)
        {
            errorCode = kThreadError_NoBufs;
        }
        else
        {
            parsedLength = spinel_datatype_unpack(
                               value_ptr,
                               value_len,
                               SPINEL_DATATYPE_DATA_S SPINEL_DATATYPE_DATA_S,
                               &frame_ptr,
                               &frame_len,
                               &meta_ptr,
                               &meta_len
                           );

            // We ignore metadata for now.
            (void)meta_ptr;
            (void)meta_len;

            errorCode = message->Append(frame_ptr, frame_len);
        }

        if (errorCode == kThreadError_None)
        {
            errorCode = Ip6::Ip6::HandleDatagram(*message, NULL, sThreadNetif->GetInterfaceId(), NULL, true);
        }

        if (errorCode == kThreadError_None)
        {
            if (SPINEL_HEADER_GET_TID(header) != 0)
            {
                // Only send a successful status update if
                // there was a transaction id in the header.
                SendLastStatus(header, SPINEL_STATUS_OK);
            }
        }
        else
        {
            SendLastStatus(header, ThreadErrorToSpinelStatus(errorCode));
        }

        break;
    }


    case SPINEL_PROP_NET_NETWORK_NAME:
        parsedLength = spinel_datatype_unpack(
                           value_ptr,
                           value_len,
                           SPINEL_DATATYPE_UTF8_S,
                           &string
                       );

        if ((parsedLength > 0) && (string != NULL))
        {
            errorCode = otSetNetworkName(string);

            if (errorCode == kThreadError_None)
            {
                HandleCommandPropertyGet(header, key);
            }
            else
            {
                SendLastStatus(header, ThreadErrorToSpinelStatus(errorCode));
            }
        }
        else
        {
            SendLastStatus(header, SPINEL_STATUS_PARSE_ERROR);
        }

        break;

    case SPINEL_PROP_PHY_CHAN:
        parsedLength = spinel_datatype_unpack(
                           value_ptr,
                           value_len,
                           SPINEL_DATATYPE_UINT_PACKED_S,
                           &i
                       );

        if (parsedLength > 0)
        {
            errorCode = otSetChannel(static_cast<uint8_t>(i));

            if (errorCode == kThreadError_None)
            {
                HandleCommandPropertyGet(header, key);
            }
            else
            {
                SendLastStatus(header, ThreadErrorToSpinelStatus(errorCode));
            }
        }
        else
        {
            SendLastStatus(header, SPINEL_STATUS_PARSE_ERROR);
        }

        break;


    case SPINEL_PROP_MAC_15_4_PANID:
        parsedLength = spinel_datatype_unpack(
                           value_ptr,
                           value_len,
                           SPINEL_DATATYPE_UINT16_S,
                           &tmp
                       );

        if (parsedLength > 0)
        {
            errorCode = otSetPanId(tmp);

            if (errorCode == kThreadError_None)
            {
                HandleCommandPropertyGet(header, key);
            }
            else
            {
                SendLastStatus(header, ThreadErrorToSpinelStatus(errorCode));
            }
        }
        else
        {
            SendLastStatus(header, SPINEL_STATUS_PARSE_ERROR);
        }

        break;


    case SPINEL_PROP_NET_XPANID:
        parsedLength = spinel_datatype_unpack(
                           value_ptr,
                           value_len,
                           SPINEL_DATATYPE_DATA_S,
                           &ptr,
                           &len
                       );

        if ((parsedLength > 0) && (len == sizeof(spinel_net_xpanid_t)))
        {
            otSetExtendedPanId(ptr);
            HandleCommandPropertyGet(header, key);
        }
        else
        {
            SendLastStatus(header, SPINEL_STATUS_PARSE_ERROR);
        }

        break;

    case SPINEL_PROP_NET_MASTER_KEY:
        parsedLength = spinel_datatype_unpack(
                           value_ptr,
                           value_len,
                           SPINEL_DATATYPE_DATA_S,
                           &ptr,
                           &len
                       );

        if ((parsedLength > 0) && (len < 100))
        {
            errorCode = otSetMasterKey(ptr, static_cast<uint8_t>(len));

            if (errorCode == kThreadError_None)
            {
                HandleCommandPropertyGet(header, key);
            }
            else
            {
                SendLastStatus(header, ThreadErrorToSpinelStatus(errorCode));
            }
        }
        else
        {
            SendLastStatus(header, SPINEL_STATUS_PARSE_ERROR);
        }

        break;

    case SPINEL_PROP_NET_KEY_SEQUENCE:
        parsedLength = spinel_datatype_unpack(
                           value_ptr,
                           value_len,
                           SPINEL_DATATYPE_UINT32_S,
                           &i
                       );

        if (parsedLength > 0)
        {
            otSetKeySequenceCounter(i);
            HandleCommandPropertyGet(header, key);
        }
        else
        {
            SendLastStatus(header, SPINEL_STATUS_PARSE_ERROR);
        }

        break;

    default:
        SendLastStatus(header, SPINEL_STATUS_PROPERTY_NOT_FOUND);
        break;
    }

}

void NcpBase::HandleCommandPropertyGetAddressList(uint8_t header)
{
    ThreadError errorCode;

    errorCode = OutboundFrameBegin();

    if (errorCode == kThreadError_None)
    {
        errorCode = OutboundFrameFeedPacked("Cii", header, SPINEL_CMD_PROP_VALUE_IS, SPINEL_PROP_IPV6_ADDRESS_TABLE);
    }

    for (const otNetifAddress *address = otGetUnicastAddresses(); address; address = address->mNext)
    {
        if (errorCode != kThreadError_None)
        {
            break;
        }

        errorCode = OutboundFrameFeedPacked(
                        "T(6CLL).",
                        &address->mAddress,
                        address->mPrefixLength,
                        address->mPreferredLifetime,
                        address->mValidLifetime
                    );
    }

    if (errorCode == kThreadError_None)
    {
        errorCode = OutboundFrameSend();
    }

    if (errorCode != kThreadError_None)
    {
        SendLastStatus(header, SPINEL_STATUS_INTERNAL_ERROR);
    }
}

void NcpBase::HandleCommandPropertyGetRoutingTable(uint8_t header)
{
    SendLastStatus(header, SPINEL_STATUS_UNIMPLEMENTED);
}

void NcpBase::HandleCommandPropertyInsert(uint8_t header, spinel_prop_key_t key, const uint8_t *value_ptr,
                                          uint16_t value_len)
{
    spinel_ssize_t parsedLength;
    ThreadError errorCode = kThreadError_None;

    switch (key)
    {
    case SPINEL_PROP_IPV6_ADDRESS_TABLE:
    {
        otNetifAddress netif_addr = {};
        otIp6Address *addr_ptr;

        parsedLength = spinel_datatype_unpack(
                           value_ptr,
                           value_len,
                           "6CLL",
                           &addr_ptr,
                           &netif_addr.mPrefixLength,
                           &netif_addr.mPreferredLifetime,
                           &netif_addr.mValidLifetime
                       );

        if (parsedLength > 0)
        {
            netif_addr.mAddress = *addr_ptr;
            errorCode = otAddUnicastAddress(&netif_addr);

            if (errorCode == kThreadError_None)
            {
                SendPropteryUpdate(
                    header,
                    SPINEL_CMD_PROP_VALUE_INSERTED,
                    key,
                    value_ptr,
                    value_len
                );
            }
            else
            {
                SendLastStatus(header, ThreadErrorToSpinelStatus(errorCode));
            }
        }
        else
        {
            SendLastStatus(header, SPINEL_STATUS_PARSE_ERROR);
        }

        break;
    }

    default:
        SendLastStatus(header, SPINEL_STATUS_FAILURE);
        break;
    }
}

void NcpBase::HandleCommandPropertyRemove(uint8_t header, spinel_prop_key_t key, const uint8_t *value_ptr,
                                          uint16_t value_len)
{
    spinel_ssize_t parsedLength;
    ThreadError errorCode = kThreadError_None;

    switch (key)
    {
    case SPINEL_PROP_IPV6_ADDRESS_TABLE:
    {
        otNetifAddress netif_addr = {};
        otIp6Address *addr_ptr;

        parsedLength = spinel_datatype_unpack(
                           value_ptr,
                           value_len,
                           "6CLL",
                           &addr_ptr,
                           &netif_addr.mPrefixLength,
                           &netif_addr.mPreferredLifetime,
                           &netif_addr.mValidLifetime
                       );

        if (parsedLength > 0)
        {
            netif_addr.mAddress = *addr_ptr;
            errorCode = otRemoveUnicastAddress(&netif_addr);

            if (errorCode == kThreadError_None)
            {
                SendPropteryUpdate(
                    header,
                    SPINEL_CMD_PROP_VALUE_REMOVED,
                    key,
                    value_ptr,
                    value_len
                );
            }
            else
            {
                SendLastStatus(header, ThreadErrorToSpinelStatus(errorCode));
            }
        }
        else
        {
            SendLastStatus(header, SPINEL_STATUS_PARSE_ERROR);
        }

        break;
    }

    default:
        SendLastStatus(header, SPINEL_STATUS_FAILURE);
        break;
    }
}

void NcpBase::SendLastStatus(uint8_t header, spinel_status_t lastStatus)
{
    if (SPINEL_HEADER_GET_IID(header) == 0)
    {
        mLastStatus = lastStatus;
    }

    SendPropteryUpdate(header, SPINEL_CMD_PROP_VALUE_IS, SPINEL_PROP_LAST_STATUS, "i", lastStatus);
}

void NcpBase::SendPropteryUpdate(uint8_t header, uint8_t command, spinel_prop_key_t key, const char *pack_format, ...)
{
    ThreadError errorCode;
    va_list args;

    errorCode = OutboundFrameBegin();

    if (errorCode == kThreadError_None)
    {
        errorCode = OutboundFrameFeedPacked("Cii", header, command, key);
    }

    if (errorCode == kThreadError_None)
    {
        va_start(args, pack_format);
        errorCode = OutboundFrameFeedVPacked(pack_format, args);
        va_end(args);
    }

    if (errorCode == kThreadError_None)
    {
        errorCode = OutboundFrameSend();
    }
}

void NcpBase::SendPropteryUpdate(uint8_t header, uint8_t command, spinel_prop_key_t key, const uint8_t *value_ptr,
                                 uint16_t value_len)
{
    ThreadError errorCode;

    errorCode = OutboundFrameBegin();

    if (errorCode == kThreadError_None)
    {
        errorCode = OutboundFrameFeedPacked("Cii", header, command, key);
    }

    if (errorCode == kThreadError_None)
    {
        errorCode = OutboundFrameFeedData(value_ptr, value_len);
    }

    if (errorCode == kThreadError_None)
    {
        errorCode = OutboundFrameSend();
    }
}

void NcpBase::SendPropteryUpdate(uint8_t header, uint8_t command, spinel_prop_key_t key, Message &message)
{
    ThreadError errorCode;

    errorCode = OutboundFrameBegin();

    if (errorCode == kThreadError_None)
    {
        errorCode = OutboundFrameFeedPacked("Cii", header, command, key);
    }

    if (errorCode == kThreadError_None)
    {
        errorCode = OutboundFrameFeedMessage(message);
    }

    if (errorCode == kThreadError_None)
    {
        errorCode = OutboundFrameSend();
    }
}

ThreadError
NcpBase::OutboundFrameFeedVPacked(const char *pack_format, va_list args)
{
    uint8_t buf[64];
    ThreadError errorCode = kThreadError_NoBufs;
    spinel_ssize_t packed_len;

    packed_len = spinel_datatype_vpack(buf, sizeof(buf), pack_format, args);

    if ((packed_len > 0) && (packed_len <= sizeof(buf)))
    {
        errorCode = OutboundFrameFeedData(buf, packed_len);
    }

    return errorCode;
}

ThreadError
NcpBase::OutboundFrameFeedPacked(const char *pack_format, ...)
{
    ThreadError errorCode;
    va_list args;

    va_start(args, pack_format);
    errorCode = OutboundFrameFeedVPacked(pack_format, args);
    va_end(args);

    return errorCode;
}

void NcpBase::HandleSendDone(void *context)
{
    NcpBase *obj = reinterpret_cast<Ncp *>(context);
    obj->HandleSendDone();
}

void NcpBase::HandleSendDone()
{
    if (mSendQueue.GetHead() != NULL)
    {
        Message &message(*mSendQueue.GetHead());
        HandleReceivedDatagram(message);
        mSendQueue.Dequeue(message);
    }

    if (mQueuedGetHeader != 0)
    {
        HandleCommandPropertyGet(mQueuedGetHeader, mQueuedGetKey);
        mQueuedGetHeader = 0;
    }
}

}  // namespace Thread
