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
#include <stdlib.h>
#include <common/code_utils.hpp>
#include <ncp/ncp_base.hpp>
#include <openthread.h>
#include <openthread-config.h>
#include <stdarg.h>
#include <platform/radio.h>

namespace Thread {

extern ThreadNetif *sThreadNetif;

static NcpBase *sNcpContext = NULL;

// ----------------------------------------------------------------------------
// MARK: Command/Property Jump Tables
// ----------------------------------------------------------------------------

const NcpBase::CommandHandlerEntry NcpBase::mCommandHandlerTable[] =
{
    { SPINEL_CMD_NOOP, &NcpBase::CommandHandler_NOOP },
    { SPINEL_CMD_RESET, &NcpBase::CommandHandler_RESET },
    { SPINEL_CMD_PROP_VALUE_GET, &NcpBase::CommandHandler_PROP_VALUE_GET },
    { SPINEL_CMD_PROP_VALUE_SET, &NcpBase::CommandHandler_PROP_VALUE_SET },
    { SPINEL_CMD_PROP_VALUE_INSERT, &NcpBase::CommandHandler_PROP_VALUE_INSERT },
    { SPINEL_CMD_PROP_VALUE_REMOVE, &NcpBase::CommandHandler_PROP_VALUE_REMOVE },
};

const NcpBase::GetPropertyHandlerEntry NcpBase::mGetPropertyHandlerTable[] =
{
    { SPINEL_PROP_LAST_STATUS, &NcpBase::GetPropertyHandler_LAST_STATUS },
    { SPINEL_PROP_PROTOCOL_VERSION, &NcpBase::GetPropertyHandler_PROTOCOL_VERSION },
    { SPINEL_PROP_INTERFACE_TYPE, &NcpBase::GetPropertyHandler_INTERFACE_TYPE },
    { SPINEL_PROP_VENDOR_ID, &NcpBase::GetPropertyHandler_VENDOR_ID },
    { SPINEL_PROP_CAPS, &NcpBase::GetPropertyHandler_CAPS },
    { SPINEL_PROP_NCP_VERSION, &NcpBase::GetPropertyHandler_NCP_VERSION },
    { SPINEL_PROP_INTERFACE_COUNT, &NcpBase::GetPropertyHandler_INTERFACE_COUNT },
    { SPINEL_PROP_POWER_STATE, &NcpBase::GetPropertyHandler_POWER_STATE },
    { SPINEL_PROP_HWADDR, &NcpBase::GetPropertyHandler_HWADDR },
    { SPINEL_PROP_LOCK, &NcpBase::GetPropertyHandler_LOCK },

    { SPINEL_PROP_PHY_ENABLED, &NcpBase::GetPropertyHandler_PHY_ENABLED },
    { SPINEL_PROP_PHY_FREQ, &NcpBase::GetPropertyHandler_PHY_FREQ },
    { SPINEL_PROP_PHY_CHAN_SUPPORTED, &NcpBase::GetPropertyHandler_PHY_CHAN_SUPPORTED },
    { SPINEL_PROP_PHY_CHAN, &NcpBase::GetPropertyHandler_PHY_CHAN },
    { SPINEL_PROP_PHY_RSSI, &NcpBase::GetPropertyHandler_PHY_RSSI },

    { SPINEL_PROP_MAC_SCAN_STATE, &NcpBase::GetPropertyHandler_MAC_SCAN_STATE },
    { SPINEL_PROP_MAC_SCAN_MASK, &NcpBase::GetPropertyHandler_MAC_SCAN_MASK },
    { SPINEL_PROP_MAC_SCAN_PERIOD, &NcpBase::GetPropertyHandler_MAC_SCAN_PERIOD },
    { SPINEL_PROP_MAC_15_4_PANID, &NcpBase::GetPropertyHandler_MAC_15_4_PANID },
    { SPINEL_PROP_MAC_15_4_LADDR, &NcpBase::GetPropertyHandler_MAC_15_4_LADDR },
    { SPINEL_PROP_MAC_15_4_SADDR, &NcpBase::GetPropertyHandler_MAC_15_4_SADDR },
    { SPINEL_PROP_MAC_FILTER_MODE, &NcpBase::GetPropertyHandler_MAC_FILTER_MODE },

    { SPINEL_PROP_NET_ENABLED, &NcpBase::GetPropertyHandler_NET_ENABLED },
    { SPINEL_PROP_NET_STATE, &NcpBase::GetPropertyHandler_NET_STATE },
    { SPINEL_PROP_NET_ROLE, &NcpBase::GetPropertyHandler_NET_ROLE },
    { SPINEL_PROP_NET_NETWORK_NAME, &NcpBase::GetPropertyHandler_NET_NETWORK_NAME },
    { SPINEL_PROP_NET_XPANID, &NcpBase::GetPropertyHandler_NET_XPANID },
    { SPINEL_PROP_NET_MASTER_KEY, &NcpBase::GetPropertyHandler_NET_MASTER_KEY },
    { SPINEL_PROP_NET_KEY_SEQUENCE, &NcpBase::GetPropertyHandler_NET_KEY_SEQUENCE },
    { SPINEL_PROP_NET_PARTITION_ID, &NcpBase::GetPropertyHandler_NET_PARTITION_ID },

    { SPINEL_PROP_THREAD_LEADER_ADDR, &NcpBase::GetPropertyHandler_THREAD_LEADER_ADDR },
    { SPINEL_PROP_THREAD_LEADER_RID, &NcpBase::GetPropertyHandler_THREAD_LEADER_RID },
    { SPINEL_PROP_THREAD_LEADER_WEIGHT, &NcpBase::GetPropertyHandler_THREAD_LEADER_WEIGHT },
    { SPINEL_PROP_THREAD_LOCAL_LEADER_WEIGHT, &NcpBase::GetPropertyHandler_THREAD_LOCAL_LEADER_WEIGHT },
    { SPINEL_PROP_THREAD_NETWORK_DATA, &NcpBase::GetPropertyHandler_THREAD_NETWORK_DATA },
    { SPINEL_PROP_THREAD_NETWORK_DATA_VERSION, &NcpBase::GetPropertyHandler_THREAD_NETWORK_DATA_VERSION },
    { SPINEL_PROP_THREAD_STABLE_NETWORK_DATA, &NcpBase::GetPropertyHandler_THREAD_STABLE_NETWORK_DATA },
    { SPINEL_PROP_THREAD_STABLE_NETWORK_DATA_VERSION, &NcpBase::GetPropertyHandler_THREAD_STABLE_NETWORK_DATA_VERSION },
    { SPINEL_PROP_THREAD_LOCAL_ROUTES, &NcpBase::NcpBase::GetPropertyHandler_THREAD_LOCAL_ROUTES },
    { SPINEL_PROP_THREAD_ASSISTING_PORTS, &NcpBase::NcpBase::GetPropertyHandler_THREAD_ASSISTING_PORTS },
    { SPINEL_PROP_THREAD_ALLOW_LOCAL_NET_DATA_CHANGE, &NcpBase::GetPropertyHandler_THREAD_ALLOW_LOCAL_NET_DATA_CHANGE},

    { SPINEL_PROP_IPV6_ML_PREFIX, &NcpBase::GetPropertyHandler_IPV6_ML_PREFIX },
    { SPINEL_PROP_IPV6_ML_ADDR, &NcpBase::GetPropertyHandler_IPV6_ML_ADDR },
    { SPINEL_PROP_IPV6_LL_ADDR, &NcpBase::GetPropertyHandler_IPV6_LL_ADDR },
    { SPINEL_PROP_IPV6_ADDRESS_TABLE, &NcpBase::GetPropertyHandler_IPV6_ADDRESS_TABLE },
    { SPINEL_PROP_IPV6_ROUTE_TABLE, &NcpBase::GetPropertyHandler_IPV6_ROUTE_TABLE },

    { SPINEL_PROP_STREAM_NET, &NcpBase::GetPropertyHandler_STREAM_NET },

    { SPINEL_PROP_CNTR_TX_PKT_TOTAL, &NcpBase::GetPropertyHandler_CNTR },
    { SPINEL_PROP_CNTR_TX_PKT_ACK_REQ, &NcpBase::GetPropertyHandler_CNTR },
    { SPINEL_PROP_CNTR_TX_PKT_ACKED, &NcpBase::GetPropertyHandler_CNTR },
    { SPINEL_PROP_CNTR_TX_PKT_NO_ACK_REQ, &NcpBase::GetPropertyHandler_CNTR },
    { SPINEL_PROP_CNTR_TX_PKT_DATA, &NcpBase::GetPropertyHandler_CNTR },
    { SPINEL_PROP_CNTR_TX_PKT_DATA_POLL, &NcpBase::GetPropertyHandler_CNTR },
    { SPINEL_PROP_CNTR_TX_PKT_BEACON, &NcpBase::GetPropertyHandler_CNTR },
    { SPINEL_PROP_CNTR_TX_PKT_BEACON_REQ, &NcpBase::GetPropertyHandler_CNTR },
    { SPINEL_PROP_CNTR_TX_PKT_OTHER, &NcpBase::GetPropertyHandler_CNTR },
    { SPINEL_PROP_CNTR_TX_PKT_RETRY, &NcpBase::GetPropertyHandler_CNTR },
    { SPINEL_PROP_CNTR_TX_ERR_CCA, &NcpBase::GetPropertyHandler_CNTR },
    { SPINEL_PROP_CNTR_RX_PKT_TOTAL, &NcpBase::GetPropertyHandler_CNTR },
    { SPINEL_PROP_CNTR_RX_PKT_DATA, &NcpBase::GetPropertyHandler_CNTR },
    { SPINEL_PROP_CNTR_RX_PKT_DATA_POLL, &NcpBase::GetPropertyHandler_CNTR },
    { SPINEL_PROP_CNTR_RX_PKT_BEACON, &NcpBase::GetPropertyHandler_CNTR },
    { SPINEL_PROP_CNTR_RX_PKT_BEACON_REQ, &NcpBase::GetPropertyHandler_CNTR },
    { SPINEL_PROP_CNTR_RX_PKT_OTHER, &NcpBase::GetPropertyHandler_CNTR },
    { SPINEL_PROP_CNTR_RX_PKT_FILT_WL, &NcpBase::GetPropertyHandler_CNTR },
    { SPINEL_PROP_CNTR_RX_PKT_FILT_DA, &NcpBase::GetPropertyHandler_CNTR },
    { SPINEL_PROP_CNTR_RX_ERR_EMPTY, &NcpBase::GetPropertyHandler_CNTR },
    { SPINEL_PROP_CNTR_RX_ERR_UKWN_NBR, &NcpBase::GetPropertyHandler_CNTR },
    { SPINEL_PROP_CNTR_RX_ERR_NVLD_SADDR, &NcpBase::GetPropertyHandler_CNTR },
    { SPINEL_PROP_CNTR_RX_ERR_SECURITY, &NcpBase::GetPropertyHandler_CNTR },
    { SPINEL_PROP_CNTR_RX_ERR_BAD_FCS, &NcpBase::GetPropertyHandler_CNTR },
    { SPINEL_PROP_CNTR_RX_ERR_OTHER, &NcpBase::GetPropertyHandler_CNTR },
};

const NcpBase::SetPropertyHandlerEntry NcpBase::mSetPropertyHandlerTable[] =
{
    { SPINEL_PROP_POWER_STATE, &NcpBase::SetPropertyHandler_POWER_STATE },

    { SPINEL_PROP_PHY_ENABLED, &NcpBase::SetPropertyHandler_PHY_ENABLED },
    { SPINEL_PROP_PHY_TX_POWER, &NcpBase::SetPropertyHandler_PHY_TX_POWER },
    { SPINEL_PROP_PHY_CHAN, &NcpBase::SetPropertyHandler_PHY_CHAN },
    { SPINEL_PROP_MAC_FILTER_MODE, &NcpBase::SetPropertyHandler_MAC_FILTER_MODE },

    { SPINEL_PROP_MAC_SCAN_MASK, &NcpBase::SetPropertyHandler_MAC_SCAN_MASK },
    { SPINEL_PROP_MAC_SCAN_STATE, &NcpBase::SetPropertyHandler_MAC_SCAN_STATE },
    { SPINEL_PROP_MAC_SCAN_PERIOD, &NcpBase::SetPropertyHandler_MAC_SCAN_PERIOD },
    { SPINEL_PROP_MAC_15_4_PANID, &NcpBase::SetPropertyHandler_MAC_15_4_PANID },

    { SPINEL_PROP_NET_ENABLED, &NcpBase::SetPropertyHandler_NET_ENABLED },
    { SPINEL_PROP_NET_STATE, &NcpBase::SetPropertyHandler_NET_STATE },
    { SPINEL_PROP_NET_ROLE, &NcpBase::SetPropertyHandler_NET_ROLE },
    { SPINEL_PROP_NET_NETWORK_NAME, &NcpBase::SetPropertyHandler_NET_NETWORK_NAME },
    { SPINEL_PROP_NET_XPANID, &NcpBase::SetPropertyHandler_NET_XPANID },
    { SPINEL_PROP_NET_MASTER_KEY, &NcpBase::SetPropertyHandler_NET_MASTER_KEY },
    { SPINEL_PROP_NET_KEY_SEQUENCE, &NcpBase::SetPropertyHandler_NET_KEY_SEQUENCE },

    { SPINEL_PROP_THREAD_LOCAL_LEADER_WEIGHT, &NcpBase::SetPropertyHandler_THREAD_LOCAL_LEADER_WEIGHT },
    { SPINEL_PROP_THREAD_ASSISTING_PORTS, &NcpBase::SetPropertyHandler_THREAD_ASSISTING_PORTS },
    { SPINEL_PROP_THREAD_ALLOW_LOCAL_NET_DATA_CHANGE, &NcpBase::SetPropertyHandler_THREAD_ALLOW_LOCAL_NET_DATA_CHANGE},

    { SPINEL_PROP_STREAM_NET_INSECURE, &NcpBase::SetPropertyHandler_STREAM_NET_INSECURE },
    { SPINEL_PROP_STREAM_NET, &NcpBase::SetPropertyHandler_STREAM_NET },

    { SPINEL_PROP_IPV6_ML_PREFIX, &NcpBase::SetPropertyHandler_IPV6_ML_PREFIX },
};

const NcpBase::InsertPropertyHandlerEntry NcpBase::mInsertPropertyHandlerTable[] =
{
    { SPINEL_PROP_IPV6_ADDRESS_TABLE, &NcpBase::NcpBase::InsertPropertyHandler_IPV6_ADDRESS_TABLE },
    { SPINEL_PROP_THREAD_LOCAL_ROUTES, &NcpBase::NcpBase::InsertPropertyHandler_THREAD_LOCAL_ROUTES },
    { SPINEL_PROP_THREAD_ON_MESH_NETS, &NcpBase::NcpBase::InsertPropertyHandler_THREAD_ON_MESH_NETS },
    { SPINEL_PROP_THREAD_ASSISTING_PORTS, &NcpBase::NcpBase::InsertPropertyHandler_THREAD_ASSISTING_PORTS },

    { SPINEL_PROP_CNTR_RESET, &NcpBase::SetPropertyHandler_CNTR_RESET },
};

const NcpBase::RemovePropertyHandlerEntry NcpBase::mRemovePropertyHandlerTable[] =
{
    { SPINEL_PROP_IPV6_ADDRESS_TABLE, &NcpBase::NcpBase::RemovePropertyHandler_IPV6_ADDRESS_TABLE },
    { SPINEL_PROP_THREAD_LOCAL_ROUTES, &NcpBase::NcpBase::RemovePropertyHandler_THREAD_LOCAL_ROUTES },
    { SPINEL_PROP_THREAD_ON_MESH_NETS, &NcpBase::NcpBase::RemovePropertyHandler_THREAD_ON_MESH_NETS },
    { SPINEL_PROP_THREAD_ASSISTING_PORTS, &NcpBase::NcpBase::RemovePropertyHandler_THREAD_ASSISTING_PORTS },
};

// ----------------------------------------------------------------------------
// MARK: Utility Functions
// ----------------------------------------------------------------------------

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

    case kThreadError_NoAck:
        ret = SPINEL_STATUS_NO_ACK;
        break;

    case kThreadError_ChannelAccessFailure:
        ret = SPINEL_STATUS_CCA_FAILURE;
        break;

    default:
        ret = SPINEL_STATUS_FAILURE;
        break;
    }

    return ret;
}

// ----------------------------------------------------------------------------
// MARK: Class Boilerplate
// ----------------------------------------------------------------------------

NcpBase::NcpBase():
    mUpdateChangedPropsTask(&UpdateChangedProps, this)
{
    mSupportedChannelMask = kPhySupportedChannelMask;
    mChannelMask = mSupportedChannelMask;
    mScanPeriod = 200; // ms
    sNcpContext = this;
    mChangedFlags = 0;
    mAllowLocalNetworkDataChange = false;

    for (unsigned i = 0; i < sizeof(mNetifAddresses) / sizeof(mNetifAddresses[0]); i++)
    {
        mNetifAddresses[i].mPrefixLength = kUnusedNetifAddressPrefixLen;
    }

    assert(sThreadNetif != NULL);
    otSetStateChangedCallback(&HandleNetifStateChanged, this);
    otSetReceiveIp6DatagramCallback(&HandleDatagramFromStack);
    otSetIcmpEchoEnabled(false);
}

// ----------------------------------------------------------------------------
// MARK: Outbound Datagram Handling
// ----------------------------------------------------------------------------

void NcpBase::HandleDatagramFromStack(otMessage message)
{
    sNcpContext->HandleDatagramFromStack(*static_cast<Message *>(message));
}

void NcpBase::HandleDatagramFromStack(Message &message)
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
                            message.IsLinkSecurityEnabled()
                            ? SPINEL_PROP_STREAM_NET
                            : SPINEL_PROP_STREAM_NET_INSECURE,
                            message.GetLength()
                        );
        }

        if (errorCode == kThreadError_None)
        {
            errorCode = OutboundFrameFeedMessage(message);
        }

        // TODO: Append any metadata (rssi, lqi, channel, etc) here!

        if (errorCode == kThreadError_None)
        {
            errorCode = OutboundFrameSend();
        }

        if (errorCode != kThreadError_None)
        {
            SendLastStatus(SPINEL_HEADER_FLAG | SPINEL_HEADER_IID_0, SPINEL_STATUS_DROPPED);
        }

        Message::Free(message);
    }
    else
    {
        if (mSendQueue.Enqueue(message) != kThreadError_None)
        {
            Message::Free(message);
        }
    }
}

// ----------------------------------------------------------------------------
// MARK: Scan Results Glue
// ----------------------------------------------------------------------------

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
            "CcT(ESSC.)T(iCUD.).",
            result->mChannel,
            result->mRssi,
            result->mExtAddress.m8, // laddr
            0xFFFF,                 // saddr, Not given
            result->mPanId,
            result->mLqi,
            SPINEL_PROTOCOL_TYPE_THREAD,
            flags,
            result->mNetworkName,
            result->mExtPanId, OT_EXT_PAN_ID_SIZE
        );
    }
    else
    {
        // We are finished with the scan, so send out
        // a property update indicating such.
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

// ----------------------------------------------------------------------------
// MARK: Address Table Changed Glue
// ----------------------------------------------------------------------------

void NcpBase::HandleNetifStateChanged(uint32_t flags, void *context)
{
    NcpBase *obj = reinterpret_cast<NcpBase *>(context);

    obj->mChangedFlags |= flags;

    if (!obj->mSending)
    {
        obj->mUpdateChangedPropsTask.Post();
    }
}

void NcpBase::UpdateChangedProps(void *context)
{
    NcpBase *obj = reinterpret_cast<NcpBase *>(context);
    obj->UpdateChangedProps();
}

void NcpBase::UpdateChangedProps()
{
    if (!mSending)
    {
        if ((mChangedFlags & OT_IP6_ML_ADDR_CHANGED) != 0)
        {
            mChangedFlags &= ~OT_IP6_ML_ADDR_CHANGED;
            HandleCommandPropertyGet(
                SPINEL_HEADER_FLAG | SPINEL_HEADER_IID_0,
                SPINEL_PROP_IPV6_ML_ADDR
            );
        }
        else if ((mChangedFlags & OT_NET_STATE) != 0)
        {
            mChangedFlags &= ~OT_NET_STATE;
            HandleCommandPropertyGet(
                SPINEL_HEADER_FLAG | SPINEL_HEADER_IID_0,
                SPINEL_PROP_NET_STATE
            );
        }
        else if ((mChangedFlags & OT_NET_ROLE) != 0)
        {
            mChangedFlags &= ~OT_NET_ROLE;
            HandleCommandPropertyGet(
                SPINEL_HEADER_FLAG | SPINEL_HEADER_IID_0,
                SPINEL_PROP_NET_ROLE
            );
        }
        else if ((mChangedFlags & OT_NET_PARTITION_ID) != 0)
        {
            mChangedFlags &= ~OT_NET_PARTITION_ID;
            HandleCommandPropertyGet(
                SPINEL_HEADER_FLAG | SPINEL_HEADER_IID_0,
                SPINEL_PROP_NET_PARTITION_ID
            );
        }
        else if ((mChangedFlags & OT_NET_KEY_SEQUENCE) != 0)
        {
            mChangedFlags &= ~OT_NET_KEY_SEQUENCE;
            HandleCommandPropertyGet(
                SPINEL_HEADER_FLAG | SPINEL_HEADER_IID_0,
                SPINEL_PROP_NET_KEY_SEQUENCE
            );
        }
        else if ((mChangedFlags & (OT_IP6_ADDRESS_ADDED | OT_IP6_ADDRESS_REMOVED)) != 0)
        {
            mChangedFlags &= ~(OT_IP6_ADDRESS_ADDED | OT_IP6_ADDRESS_REMOVED);
            HandleCommandPropertyGet(
                SPINEL_HEADER_FLAG | SPINEL_HEADER_IID_0,
                SPINEL_PROP_IPV6_ADDRESS_TABLE
            );
        }
        else if ((mChangedFlags & (OT_THREAD_CHILD_ADDED | OT_THREAD_CHILD_REMOVED)) != 0)
        {
            mChangedFlags &= ~(OT_THREAD_CHILD_ADDED | OT_THREAD_CHILD_REMOVED);
            // TODO: Uncomment this once we add support for this property.
            //HandleCommandPropertyGet(
            //    SPINEL_HEADER_FLAG | SPINEL_HEADER_IID_0,
            //    SPINEL_PROP_THREAD_CHILD_TABLE
            //);
        }
    }
}

// ----------------------------------------------------------------------------
// MARK: Serial Traffic Glue
// ----------------------------------------------------------------------------

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

void NcpBase::HandleSendDone()
{
    if (mSendQueue.GetHead() != NULL)
    {
        Message &message(*mSendQueue.GetHead());
        mSendQueue.Dequeue(message);
        HandleDatagramFromStack(message);
    }

    if (mQueuedGetHeader != 0)
    {
        HandleCommandPropertyGet(mQueuedGetHeader, mQueuedGetKey);
        mQueuedGetHeader = 0;
    }

    if (!mSending)
    {
        UpdateChangedProps();
    }
}


// ----------------------------------------------------------------------------
// MARK: Inbound Command Handlers
// ----------------------------------------------------------------------------

void NcpBase::HandleCommand(uint8_t header, unsigned int command, const uint8_t *arg_ptr, uint16_t arg_len)
{
    unsigned i;

    // Skip if this isn't a spinel frame
    VerifyOrExit((SPINEL_HEADER_FLAG & header) == SPINEL_HEADER_FLAG, ;);

    // We only support IID zero for now.
    VerifyOrExit(SPINEL_HEADER_GET_IID(header) == 0, SendLastStatus(header, SPINEL_STATUS_INVALID_INTERFACE););

    for (i = 0; i < sizeof(mCommandHandlerTable) / sizeof(mCommandHandlerTable[0]); i++)
    {
        if (mCommandHandlerTable[i].mCommand == command)
        {
            break;
        }
    }

    if (i < sizeof(mCommandHandlerTable) / sizeof(mCommandHandlerTable[0]))
    {
        (this->*mCommandHandlerTable[i].mHandler)(header, command, arg_ptr, arg_len);
    }
    else
    {
        SendLastStatus(header, SPINEL_STATUS_INVALID_COMMAND);
    }

exit:
    return;
}

void NcpBase::HandleCommandPropertyGet(uint8_t header, spinel_prop_key_t key)
{
    unsigned i;

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

        ExitNow(;);
    }

    for (i = 0; i < sizeof(mGetPropertyHandlerTable) / sizeof(mGetPropertyHandlerTable[0]); i++)
    {
        if (mGetPropertyHandlerTable[i].mPropKey == key)
        {
            break;
        }
    }

    if (i < sizeof(mGetPropertyHandlerTable) / sizeof(mGetPropertyHandlerTable[0]))
    {
        (this->*mGetPropertyHandlerTable[i].mHandler)(header, key);
    }
    else
    {
        SendLastStatus(header, SPINEL_STATUS_PROP_NOT_FOUND);
    }

exit:
    return;
}

void NcpBase::HandleCommandPropertySet(uint8_t header, spinel_prop_key_t key, const uint8_t *value_ptr,
                                       uint16_t value_len)
{
    unsigned i;

    if (mSending)
    {
        // If we are currently sending, can can queue up to one
        // property get that we can execute immediately after
        // the send is complete.
        if (mQueuedGetHeader == 0)
        {
            mLastStatus = SPINEL_STATUS_FAILURE;
            mQueuedGetHeader = header;
            mQueuedGetKey = SPINEL_PROP_LAST_STATUS;
        }

        ExitNow(;);
    }

    for (i = 0; i < sizeof(mSetPropertyHandlerTable) / sizeof(mSetPropertyHandlerTable[0]); i++)
    {
        if (mSetPropertyHandlerTable[i].mPropKey == key)
        {
            break;
        }
    }

    if (i < sizeof(mSetPropertyHandlerTable) / sizeof(mSetPropertyHandlerTable[0]))
    {
        (this->*mSetPropertyHandlerTable[i].mHandler)(header, key, value_ptr, value_len);
    }
    else
    {
        SendLastStatus(header, SPINEL_STATUS_PROP_NOT_FOUND);
    }

exit:
    return;
}

void NcpBase::HandleCommandPropertyInsert(uint8_t header, spinel_prop_key_t key, const uint8_t *value_ptr,
                                          uint16_t value_len)
{
    unsigned i;

    if (mSending)
    {
        // If we are currently sending, can can queue up to one
        // property get that we can execute immediately after
        // the send is complete.
        if (mQueuedGetHeader == 0)
        {
            mLastStatus = SPINEL_STATUS_FAILURE;
            mQueuedGetHeader = header;
            mQueuedGetKey = SPINEL_PROP_LAST_STATUS;
        }

        ExitNow(;);
    }

    for (i = 0; i < sizeof(mInsertPropertyHandlerTable) / sizeof(mInsertPropertyHandlerTable[0]); i++)
    {
        if (mInsertPropertyHandlerTable[i].mPropKey == key)
        {
            break;
        }
    }

    if (i < sizeof(mInsertPropertyHandlerTable) / sizeof(mInsertPropertyHandlerTable[0]))
    {
        (this->*mInsertPropertyHandlerTable[i].mHandler)(header, key, value_ptr, value_len);
    }
    else
    {
        SendLastStatus(header, SPINEL_STATUS_PROP_NOT_FOUND);
    }

exit:
    return;
}

void NcpBase::HandleCommandPropertyRemove(uint8_t header, spinel_prop_key_t key, const uint8_t *value_ptr,
                                          uint16_t value_len)
{
    unsigned i;

    if (mSending)
    {
        // If we are currently sending, can can queue up to one
        // property get that we can execute immediately after
        // the send is complete.
        if (mQueuedGetHeader == 0)
        {
            mLastStatus = SPINEL_STATUS_FAILURE;
            mQueuedGetHeader = header;
            mQueuedGetKey = SPINEL_PROP_LAST_STATUS;
        }

        ExitNow(;);
    }

    for (i = 0; i < sizeof(mRemovePropertyHandlerTable) / sizeof(mRemovePropertyHandlerTable[0]); i++)
    {
        if (mRemovePropertyHandlerTable[i].mPropKey == key)
        {
            break;
        }
    }

    if (i < sizeof(mRemovePropertyHandlerTable) / sizeof(mRemovePropertyHandlerTable[0]))
    {
        (this->*mRemovePropertyHandlerTable[i].mHandler)(header, key, value_ptr, value_len);
    }
    else
    {
        SendLastStatus(header, SPINEL_STATUS_PROP_NOT_FOUND);
    }

exit:
    return;
}


// ----------------------------------------------------------------------------
// MARK: Outbound Command Handlers
// ----------------------------------------------------------------------------


void NcpBase::SendLastStatus(uint8_t header, spinel_status_t lastStatus)
{
    if (SPINEL_HEADER_GET_IID(header) == 0)
    {
        mLastStatus = lastStatus;
    }

    SendPropteryUpdate(
        header,
        SPINEL_CMD_PROP_VALUE_IS,
        SPINEL_PROP_LAST_STATUS,
        SPINEL_DATATYPE_UINT_PACKED_S,
        lastStatus
    );
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

    if ((packed_len > 0) && (packed_len <= static_cast<spinel_ssize_t>(sizeof(buf))))
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


// ----------------------------------------------------------------------------
// MARK: Individual Command Handlers
// ----------------------------------------------------------------------------


void NcpBase::CommandHandler_NOOP(uint8_t header, unsigned int command, const uint8_t *arg_ptr, uint16_t arg_len)
{
    SendLastStatus(header, SPINEL_STATUS_OK);
    (void)command;
    (void)arg_ptr;
    (void)arg_len;
}

void NcpBase::CommandHandler_RESET(uint8_t header, unsigned int command, const uint8_t *arg_ptr, uint16_t arg_len)
{
    // TODO: Figure out how to actually perform a reset.
    otDisable();
    otEnable();
    SendLastStatus(SPINEL_HEADER_FLAG | SPINEL_HEADER_IID_0, SPINEL_STATUS_RESET_SOFTWARE);
    (void)header;
    (void)command;
    (void)arg_ptr;
    (void)arg_len;
}

void NcpBase::CommandHandler_PROP_VALUE_GET(uint8_t header, unsigned int command, const uint8_t *arg_ptr,
                                            uint16_t arg_len)
{
    unsigned int propKey = 0;
    spinel_ssize_t parsedLength;

    parsedLength = spinel_datatype_unpack(arg_ptr, arg_len, "i", &propKey);

    if (parsedLength > 0)
    {
        HandleCommandPropertyGet(header, static_cast<spinel_prop_key_t>(propKey));
    }
    else
    {
        SendLastStatus(header, SPINEL_STATUS_PARSE_ERROR);
    }
    (void)command;
}

void NcpBase::CommandHandler_PROP_VALUE_SET(uint8_t header, unsigned int command, const uint8_t *arg_ptr,
                                            uint16_t arg_len)
{
    unsigned int propKey = 0;
    spinel_ssize_t parsedLength;
    const uint8_t *value_ptr;
    unsigned int value_len;

    parsedLength = spinel_datatype_unpack(arg_ptr, arg_len, "iD", &propKey, &value_ptr, &value_len);

    if (parsedLength == arg_len)
    {
        HandleCommandPropertySet(header, static_cast<spinel_prop_key_t>(propKey), value_ptr, value_len);
    }
    else
    {
        SendLastStatus(header, SPINEL_STATUS_PARSE_ERROR);
    }
    (void)command;
}

void NcpBase::CommandHandler_PROP_VALUE_INSERT(uint8_t header, unsigned int command, const uint8_t *arg_ptr,
                                               uint16_t arg_len)
{
    unsigned int propKey = 0;
    spinel_ssize_t parsedLength;
    const uint8_t *value_ptr;
    unsigned int value_len;

    parsedLength = spinel_datatype_unpack(arg_ptr, arg_len, "iD", &propKey, &value_ptr, &value_len);

    if (parsedLength == arg_len)
    {
        HandleCommandPropertyInsert(header, static_cast<spinel_prop_key_t>(propKey), value_ptr, value_len);
    }
    else
    {
        SendLastStatus(header, SPINEL_STATUS_PARSE_ERROR);
    }
    (void)command;
}

void NcpBase::CommandHandler_PROP_VALUE_REMOVE(uint8_t header, unsigned int command, const uint8_t *arg_ptr,
                                               uint16_t arg_len)
{
    unsigned int propKey = 0;
    spinel_ssize_t parsedLength;
    const uint8_t *value_ptr;
    unsigned int value_len;

    parsedLength = spinel_datatype_unpack(arg_ptr, arg_len, "iD", &propKey, &value_ptr, &value_len);

    if (parsedLength == arg_len)
    {
        HandleCommandPropertyRemove(header, static_cast<spinel_prop_key_t>(propKey), value_ptr, value_len);
    }
    else
    {
        SendLastStatus(header, SPINEL_STATUS_PARSE_ERROR);
    }
    (void)command;
}


// ----------------------------------------------------------------------------
// MARK: Individual Property Getters
// ----------------------------------------------------------------------------


void NcpBase::GetPropertyHandler_LAST_STATUS(uint8_t header, spinel_prop_key_t key)
{
    SendPropteryUpdate(header, SPINEL_CMD_PROP_VALUE_IS, key, SPINEL_DATATYPE_UINT_PACKED_S, mLastStatus);
}

void NcpBase::GetPropertyHandler_PROTOCOL_VERSION(uint8_t header, spinel_prop_key_t key)
{
    SendPropteryUpdate(
        header,
        SPINEL_CMD_PROP_VALUE_IS,
        key,
        SPINEL_DATATYPE_UINT_PACKED_S SPINEL_DATATYPE_UINT_PACKED_S,
        SPINEL_PROTOCOL_VERSION_THREAD_MAJOR,
        SPINEL_PROTOCOL_VERSION_THREAD_MINOR
    );
}

void NcpBase::GetPropertyHandler_INTERFACE_TYPE(uint8_t header, spinel_prop_key_t key)
{
    SendPropteryUpdate(
        header,
        SPINEL_CMD_PROP_VALUE_IS,
        key,
        SPINEL_DATATYPE_UINT_PACKED_S,
        SPINEL_PROTOCOL_TYPE_THREAD
    );
}

void NcpBase::GetPropertyHandler_VENDOR_ID(uint8_t header, spinel_prop_key_t key)
{
    SendPropteryUpdate(
        header,
        SPINEL_CMD_PROP_VALUE_IS,
        key,
        SPINEL_DATATYPE_UINT_PACKED_S,
        0 // Vendor ID. Zero for unknown.
    );
}

void NcpBase::GetPropertyHandler_CAPS(uint8_t header, spinel_prop_key_t key)
{
    ThreadError errorCode(OutboundFrameBegin());

    if (errorCode == kThreadError_None)
    {
        errorCode = OutboundFrameFeedPacked("Cii", header, SPINEL_CMD_PROP_VALUE_IS, key);
    }

    // Begin adding capabilities //////////////////////////////////////////////

    OutboundFrameFeedPacked(SPINEL_DATATYPE_UINT_PACKED_S, SPINEL_CAP_NET_THREAD_1_0);

    OutboundFrameFeedPacked(SPINEL_DATATYPE_UINT_PACKED_S, SPINEL_CAP_COUNTERS);

    // TODO: Somehow get the following capability from the radio.
    OutboundFrameFeedPacked(SPINEL_DATATYPE_UINT_PACKED_S, SPINEL_CAP_802_15_4_2450MHZ_OQPSK);

#if OPENTHREAD_CONFIG_MAX_CHILDREN > 0
    OutboundFrameFeedPacked(SPINEL_DATATYPE_UINT_PACKED_S, SPINEL_CAP_ROLE_ROUTER);
#endif

    // End adding capabilities /////////////////////////////////////////////////

    if (errorCode == kThreadError_None)
    {
        errorCode = OutboundFrameSend();
    }

    if (errorCode != kThreadError_None)
    {
        SendLastStatus(header, SPINEL_STATUS_INTERNAL_ERROR);
    }
}

void NcpBase::GetPropertyHandler_NCP_VERSION(uint8_t header, spinel_prop_key_t key)
{
    SendPropteryUpdate(
        header,
        SPINEL_CMD_PROP_VALUE_IS,
        key,
        SPINEL_DATATYPE_UTF8_S,
        PACKAGE_NAME "/" PACKAGE_VERSION "; " __DATE__ " " __TIME__
    );
}

void NcpBase::GetPropertyHandler_INTERFACE_COUNT(uint8_t header, spinel_prop_key_t key)
{
    SendPropteryUpdate(
        header,
        SPINEL_CMD_PROP_VALUE_IS,
        key,
        SPINEL_DATATYPE_UINT8_S,
        1 // Only one interface for now
    );
}

void NcpBase::GetPropertyHandler_POWER_STATE(uint8_t header, spinel_prop_key_t key)
{
    // Always online at the moment
    SendPropteryUpdate(
        header,
        SPINEL_CMD_PROP_VALUE_IS,
        key,
        SPINEL_DATATYPE_UINT8_S,
        SPINEL_POWER_STATE_ONLINE
    );
}

void NcpBase::GetPropertyHandler_HWADDR(uint8_t header, spinel_prop_key_t key)
{
    SendPropteryUpdate(
        header,
        SPINEL_CMD_PROP_VALUE_IS,
        key,
        SPINEL_DATATYPE_EUI64_S,
        otGetExtendedAddress()
    );
}

void NcpBase::GetPropertyHandler_LOCK(uint8_t header, spinel_prop_key_t key)
{
    // TODO: Implement property lock (Needs API!)
    SendLastStatus(header, SPINEL_STATUS_UNIMPLEMENTED);
    (void)key;
}

void NcpBase::GetPropertyHandler_PHY_ENABLED(uint8_t header, spinel_prop_key_t key)
{
    // TODO: Implement PHY_ENBLED (Needs API!)
    SendLastStatus(header, SPINEL_STATUS_UNIMPLEMENTED);
    (void)key;
}

void NcpBase::GetPropertyHandler_PHY_FREQ(uint8_t header, spinel_prop_key_t key)
{
    uint32_t freq_khz(0);
    const uint8_t chan(otGetChannel());

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

    SendPropteryUpdate(
        header,
        SPINEL_CMD_PROP_VALUE_IS,
        key,
        SPINEL_DATATYPE_UINT32_S,
        freq_khz
    );
}

void NcpBase::GetPropertyHandler_PHY_CHAN_SUPPORTED(uint8_t header, spinel_prop_key_t key)
{
    GetPropertyHandler_ChannelMaskHelper(header, key, mSupportedChannelMask);
}

void NcpBase::GetPropertyHandler_PHY_CHAN(uint8_t header, spinel_prop_key_t key)
{
    SendPropteryUpdate(
        header,
        SPINEL_CMD_PROP_VALUE_IS,
        key,
        SPINEL_DATATYPE_UINT8_S,
        otGetChannel()
    );
}

void NcpBase::GetPropertyHandler_PHY_RSSI(uint8_t header, spinel_prop_key_t key)
{
    SendPropteryUpdate(
        header,
        SPINEL_CMD_PROP_VALUE_IS,
        key,
        SPINEL_DATATYPE_INT8_S,
        otPlatRadioGetNoiseFloor()
    );
}

void NcpBase::GetPropertyHandler_MAC_SCAN_STATE(uint8_t header, spinel_prop_key_t key)
{
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
}

void NcpBase::GetPropertyHandler_MAC_SCAN_PERIOD(uint8_t header, spinel_prop_key_t key)
{
    SendPropteryUpdate(
        header,
        SPINEL_CMD_PROP_VALUE_IS,
        key,
        SPINEL_DATATYPE_UINT16_S,
        mScanPeriod
    );
}

void NcpBase::GetPropertyHandler_ChannelMaskHelper(uint8_t header, spinel_prop_key_t key, uint32_t channel_mask)
{
    ThreadError errorCode(OutboundFrameBegin());

    if (errorCode == kThreadError_None)
    {
        errorCode = OutboundFrameFeedPacked("Cii", header, SPINEL_CMD_PROP_VALUE_IS, key);
    }

    for (int i = 0; i < 32; i++)
    {
        if (errorCode != kThreadError_None)
        {
            break;
        }

        if (0 != (channel_mask & (1 << i)))
        {
            errorCode = OutboundFrameFeedPacked(SPINEL_DATATYPE_UINT8_S, i);
        }
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

void NcpBase::GetPropertyHandler_MAC_SCAN_MASK(uint8_t header, spinel_prop_key_t key)
{
    GetPropertyHandler_ChannelMaskHelper(header, key, mChannelMask);
}

void NcpBase::GetPropertyHandler_MAC_15_4_PANID(uint8_t header, spinel_prop_key_t key)
{
    SendPropteryUpdate(
        header,
        SPINEL_CMD_PROP_VALUE_IS,
        key,
        SPINEL_DATATYPE_UINT16_S,
        otGetPanId()
    );
}

void NcpBase::GetPropertyHandler_MAC_FILTER_MODE(uint8_t header, spinel_prop_key_t key)
{
    SendPropteryUpdate(
        header,
        SPINEL_CMD_PROP_VALUE_IS,
        key,
        SPINEL_DATATYPE_INT8_S,
        otPlatRadioGetPromiscuous()
        ? SPINEL_MAC_FILTER_MODE_15_4_PROMISCUOUS
        : SPINEL_MAC_FILTER_MODE_NORMAL
    );
}

void NcpBase::GetPropertyHandler_MAC_15_4_LADDR(uint8_t header, spinel_prop_key_t key)
{
    SendPropteryUpdate(
        header,
        SPINEL_CMD_PROP_VALUE_IS,
        key,
        SPINEL_DATATYPE_EUI64_S,
        otGetExtendedAddress()
    );
}

void NcpBase::GetPropertyHandler_MAC_15_4_SADDR(uint8_t header, spinel_prop_key_t key)
{
    SendPropteryUpdate(
        header,
        SPINEL_CMD_PROP_VALUE_IS,
        key,
        SPINEL_DATATYPE_UINT16_S,
        otGetShortAddress()
    );
}

void NcpBase::GetPropertyHandler_NET_ENABLED(uint8_t header, spinel_prop_key_t key)
{
    SendPropteryUpdate(
        header,
        SPINEL_CMD_PROP_VALUE_IS,
        key,
        SPINEL_DATATYPE_BOOL_S,
        otIsInterfaceUp()
    );
}

void NcpBase::GetPropertyHandler_NET_STATE(uint8_t header, spinel_prop_key_t key)
{
    spinel_net_state_t state(SPINEL_NET_STATE_OFFLINE);

    if (!otInterfaceUp())
    {
        state = SPINEL_NET_STATE_OFFLINE;
    }
    else
    {
        switch (otGetDeviceRole())
        {
        case kDeviceRoleDisabled:
        case kDeviceRoleDetached:
            state = SPINEL_NET_STATE_DETACHED;
            break;

        case kDeviceRoleChild:
        case kDeviceRoleRouter:
        case kDeviceRoleLeader:
            state = SPINEL_NET_STATE_ATTACHED;
            break;
        }
    }

    SendPropteryUpdate(
        header,
        SPINEL_CMD_PROP_VALUE_IS,
        key,
        SPINEL_DATATYPE_UINT8_S,
        state
    );
}

void NcpBase::GetPropertyHandler_NET_ROLE(uint8_t header, spinel_prop_key_t key)
{
    spinel_net_role_t role(SPINEL_NET_ROLE_NONE);

    switch (otGetDeviceRole())
    {
    case kDeviceRoleDisabled:
    case kDeviceRoleDetached:
        role = SPINEL_NET_ROLE_NONE;
        break;

    case kDeviceRoleChild:
        role = SPINEL_NET_ROLE_CHILD;
        break;

    case kDeviceRoleRouter:
        role = SPINEL_NET_ROLE_ROUTER;
        break;

    case kDeviceRoleLeader:
        role = SPINEL_NET_ROLE_LEADER;
        break;
    }

    SendPropteryUpdate(
        header,
        SPINEL_CMD_PROP_VALUE_IS,
        key,
        SPINEL_DATATYPE_UINT8_S,
        role
    );
}

void NcpBase::GetPropertyHandler_NET_NETWORK_NAME(uint8_t header, spinel_prop_key_t key)
{
    SendPropteryUpdate(
        header,
        SPINEL_CMD_PROP_VALUE_IS,
        key,
        SPINEL_DATATYPE_UTF8_S,
        otGetNetworkName()
    );
}

void NcpBase::GetPropertyHandler_NET_XPANID(uint8_t header, spinel_prop_key_t key)
{
    SendPropteryUpdate(
        header,
        SPINEL_CMD_PROP_VALUE_IS,
        key,
        SPINEL_DATATYPE_DATA_S,
        otGetExtendedPanId(),
        sizeof(spinel_net_xpanid_t)
    );
}

void NcpBase::GetPropertyHandler_NET_MASTER_KEY(uint8_t header, spinel_prop_key_t key)
{
    const uint8_t *ptr(NULL);
    uint8_t len(0);

    ptr = otGetMasterKey(&len);

    SendPropteryUpdate(
        header,
        SPINEL_CMD_PROP_VALUE_IS,
        key,
        SPINEL_DATATYPE_DATA_S,
        ptr,
        len
    );
}

void NcpBase::GetPropertyHandler_NET_KEY_SEQUENCE(uint8_t header, spinel_prop_key_t key)
{
    SendPropteryUpdate(
        header,
        SPINEL_CMD_PROP_VALUE_IS,
        key,
        SPINEL_DATATYPE_UINT32_S,
        otGetKeySequenceCounter()
    );
}

void NcpBase::GetPropertyHandler_NET_PARTITION_ID(uint8_t header, spinel_prop_key_t key)
{
    SendPropteryUpdate(
        header,
        SPINEL_CMD_PROP_VALUE_IS,
        key,
        SPINEL_DATATYPE_UINT32_S,
        otGetPartitionId()
    );
}

void NcpBase::GetPropertyHandler_THREAD_NETWORK_DATA_VERSION(uint8_t header, spinel_prop_key_t key)
{
    SendPropteryUpdate(
        header,
        SPINEL_CMD_PROP_VALUE_IS,
        key,
        SPINEL_DATATYPE_UINT8_S,
        otGetNetworkDataVersion()
    );
}

void NcpBase::GetPropertyHandler_THREAD_STABLE_NETWORK_DATA_VERSION(uint8_t header, spinel_prop_key_t key)
{
    SendPropteryUpdate(
        header,
        SPINEL_CMD_PROP_VALUE_IS,
        key,
        SPINEL_DATATYPE_UINT8_S,
        otGetStableNetworkDataVersion()
    );
}

void NcpBase::GetPropertyHandler_THREAD_NETWORK_DATA(uint8_t header, spinel_prop_key_t key)
{
    ThreadError errorCode(OutboundFrameBegin());
    uint8_t network_data[255];
    uint8_t network_data_len = 255;

    if (errorCode == kThreadError_None)
    {
        errorCode = OutboundFrameFeedPacked("Cii", header, SPINEL_CMD_PROP_VALUE_IS, key);
    }

    if (errorCode == kThreadError_None)
    {
        otGetNetworkDataLocal(
            false, // Stable?
            network_data,
            &network_data_len
        );

        errorCode = OutboundFrameFeedData(network_data, network_data_len);
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

void NcpBase::GetPropertyHandler_THREAD_STABLE_NETWORK_DATA(uint8_t header, spinel_prop_key_t key)
{
    ThreadError errorCode(OutboundFrameBegin());
    uint8_t network_data[255];
    uint8_t network_data_len = 255;

    if (errorCode == kThreadError_None)
    {
        errorCode = OutboundFrameFeedPacked("Cii", header, SPINEL_CMD_PROP_VALUE_IS, key);
    }

    if (errorCode == kThreadError_None)
    {
        otGetNetworkDataLocal(
            true, // Stable?
            network_data,
            &network_data_len
        );

        errorCode = OutboundFrameFeedData(network_data, network_data_len);
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

void NcpBase::GetPropertyHandler_THREAD_LEADER_RID(uint8_t header, spinel_prop_key_t key)
{
    SendPropteryUpdate(
        header,
        SPINEL_CMD_PROP_VALUE_IS,
        key,
        SPINEL_DATATYPE_UINT8_S,
        otGetLeaderRouterId()
    );
}

void NcpBase::GetPropertyHandler_THREAD_LOCAL_LEADER_WEIGHT(uint8_t header, spinel_prop_key_t key)
{
    SendPropteryUpdate(
        header,
        SPINEL_CMD_PROP_VALUE_IS,
        key,
        SPINEL_DATATYPE_UINT8_S,
        otGetLocalLeaderWeight()
    );
}

void NcpBase::GetPropertyHandler_THREAD_LEADER_WEIGHT(uint8_t header, spinel_prop_key_t key)
{
    SendPropteryUpdate(
        header,
        SPINEL_CMD_PROP_VALUE_IS,
        key,
        SPINEL_DATATYPE_UINT8_S,
        otGetLeaderWeight()
    );
}

void NcpBase::GetPropertyHandler_THREAD_LEADER_ADDR(uint8_t header, spinel_prop_key_t key)
{
    ThreadError errorCode;
    otIp6Address address;
    errorCode = otGetLeaderRloc(&address);

    if (errorCode == kThreadError_None)
    {
        SendPropteryUpdate(
            header,
            SPINEL_CMD_PROP_VALUE_IS,
            key,
            SPINEL_DATATYPE_IPv6ADDR_S,
            &address
        );
    }
    else
    {
        SendLastStatus(header, ThreadErrorToSpinelStatus(errorCode));
    }
}

void
NcpBase::GetPropertyHandler_THREAD_ASSISTING_PORTS(uint8_t header, spinel_prop_key_t key)
{
    ThreadError errorCode(OutboundFrameBegin());
    uint8_t num_entries = 0;
    const uint16_t* ports = otGetUnsecurePorts(&num_entries);

    if (errorCode == kThreadError_None)
    {
        errorCode = OutboundFrameFeedPacked("Cii", header, SPINEL_CMD_PROP_VALUE_IS, key);
    }

    for (; num_entries != 0; ports++, num_entries--)
    {
        if (errorCode != kThreadError_None)
        {
            break;
        }

        errorCode = OutboundFrameFeedPacked("S", ports);
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

void NcpBase::GetPropertyHandler_THREAD_ALLOW_LOCAL_NET_DATA_CHANGE(uint8_t header, spinel_prop_key_t key)
{
    SendPropteryUpdate(
        header,
        SPINEL_CMD_PROP_VALUE_IS,
        key,
        SPINEL_DATATYPE_BOOL_S,
        mAllowLocalNetworkDataChange
    );
}

void NcpBase::GetPropertyHandler_IPV6_ML_PREFIX(uint8_t header, spinel_prop_key_t key)
{
    const uint8_t *ml_prefix = otGetMeshLocalPrefix();

    if (ml_prefix)
    {
        otIp6Address addr;

        memcpy(addr.mFields.m8, ml_prefix, 8);

        // Zero out the last 8 bytes.
        memset(addr.mFields.m8 + 8, 0, 8);

        SendPropteryUpdate(
            header,
            SPINEL_CMD_PROP_VALUE_IS,
            key,
            SPINEL_DATATYPE_IPv6ADDR_S SPINEL_DATATYPE_UINT8_S,
            &addr,
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
}

void NcpBase::GetPropertyHandler_IPV6_ML_ADDR(uint8_t header, spinel_prop_key_t key)
{
    const otIp6Address *ml64 = otGetMeshLocalEid();

    if (ml64)
    {
        SendPropteryUpdate(
            header,
            SPINEL_CMD_PROP_VALUE_IS,
            key,
            SPINEL_DATATYPE_IPv6ADDR_S,
            ml64
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
}

void NcpBase::GetPropertyHandler_IPV6_LL_ADDR(uint8_t header, spinel_prop_key_t key)
{
    // TODO!
    SendLastStatus(header, SPINEL_STATUS_UNIMPLEMENTED);
    (void)key;
}

void NcpBase::GetPropertyHandler_IPV6_ADDRESS_TABLE(uint8_t header, spinel_prop_key_t key)
{
    ThreadError errorCode(OutboundFrameBegin());

    if (errorCode == kThreadError_None)
    {
        errorCode = OutboundFrameFeedPacked("Cii", header, SPINEL_CMD_PROP_VALUE_IS, key);
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

void NcpBase::GetPropertyHandler_IPV6_ROUTE_TABLE(uint8_t header, spinel_prop_key_t key)
{
    // TODO: Implement get route table
    SendLastStatus(header, SPINEL_STATUS_UNIMPLEMENTED);
    (void)key;
}

void NcpBase::GetPropertyHandler_THREAD_LOCAL_ROUTES(uint8_t header, spinel_prop_key_t key)
{
    // TODO: Implement get external route table
    SendLastStatus(header, SPINEL_STATUS_UNIMPLEMENTED);
    (void)key;
}

void NcpBase::GetPropertyHandler_STREAM_NET(uint8_t header, spinel_prop_key_t key)
{
    // TODO: Implement explicit data poll.
    SendLastStatus(header, SPINEL_STATUS_UNIMPLEMENTED);
    (void)key;
}

void NcpBase::GetPropertyHandler_CNTR(uint8_t header, spinel_prop_key_t key)
{
    uint32_t value;
    const otMacCounters *macCounters;

    macCounters = otGetMacCounters();

    assert(macCounters != NULL);

    switch (key)
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
        SendLastStatus(header, SPINEL_STATUS_INTERNAL_ERROR);
        goto bail;
        break;
    }

    SendPropteryUpdate(
        header,
        SPINEL_CMD_PROP_VALUE_IS,
        key,
        SPINEL_DATATYPE_UINT32_S,
        value
    );

bail:
    return;
}



// ----------------------------------------------------------------------------
// MARK: Individual Property Setters
// ----------------------------------------------------------------------------

void NcpBase::SetPropertyHandler_POWER_STATE(uint8_t header, spinel_prop_key_t key, const uint8_t *value_ptr,
                                             uint16_t value_len)
{
    // TODO: Implement POWER_STATE
    SendLastStatus(header, SPINEL_STATUS_UNIMPLEMENTED);
    (void)key;
    (void)value_ptr;
    (void)value_len;
}

void NcpBase::SetPropertyHandler_PHY_ENABLED(uint8_t header, spinel_prop_key_t key, const uint8_t *value_ptr,
                                             uint16_t value_len)
{
    bool value = false;
    spinel_ssize_t parsedLength;
    ThreadError errorCode = kThreadError_None;

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
            errorCode = otPlatRadioDisable();
        }
        else
        {
            errorCode = otPlatRadioEnable();
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
}

void NcpBase::SetPropertyHandler_PHY_TX_POWER(uint8_t header, spinel_prop_key_t key, const uint8_t *value_ptr,
                                              uint16_t value_len)
{
    // TODO: Implement PHY_TX_POWER
    SendLastStatus(header, SPINEL_STATUS_UNIMPLEMENTED);
    (void)key;
    (void)value_ptr;
    (void)value_len;
}

void NcpBase::SetPropertyHandler_PHY_CHAN(uint8_t header, spinel_prop_key_t key, const uint8_t *value_ptr,
                                          uint16_t value_len)
{
    unsigned int i = 0;
    spinel_ssize_t parsedLength;
    ThreadError errorCode = kThreadError_None;

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
}

void NcpBase::SetPropertyHandler_MAC_FILTER_MODE(uint8_t header, spinel_prop_key_t key, const uint8_t *value_ptr,
                                          uint16_t value_len)
{
    uint8_t i = 0;
    spinel_ssize_t parsedLength;
    ThreadError errorCode = kThreadError_None;

    parsedLength = spinel_datatype_unpack(
                       value_ptr,
                       value_len,
                       SPINEL_DATATYPE_UINT8_S,
                       &i
                   );

    if (parsedLength > 0)
    {
        switch (i)
        {
        case SPINEL_MAC_FILTER_MODE_NORMAL:
            otPlatRadioSetPromiscuous(false);
            errorCode = kThreadError_None;
            break;

        case SPINEL_MAC_FILTER_MODE_PROMISCUOUS:
        case SPINEL_MAC_FILTER_MODE_MONITOR:
            otPlatRadioSetPromiscuous(true);
            errorCode = kThreadError_None;
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
}

void NcpBase::SetPropertyHandler_MAC_SCAN_MASK(uint8_t header, spinel_prop_key_t key, const uint8_t *value_ptr,
                                               uint16_t value_len)
{
    ThreadError errorCode = kThreadError_None;
    uint32_t new_mask(0);

    for (; value_len != 0; value_len--, value_ptr++)
    {
        if ( (value_ptr[0] > 31)
          || (mSupportedChannelMask & (1 << value_ptr[0])) == 0
        ) {
            errorCode = kThreadError_InvalidArgs;
            break;
        }

        new_mask |= (1 << value_ptr[0]);
    }

    if (errorCode == kThreadError_None)
    {
        mChannelMask = new_mask;
        HandleCommandPropertyGet(header, key);
    }
    else
    {
        SendLastStatus(header, ThreadErrorToSpinelStatus(errorCode));
    }
}

void NcpBase::SetPropertyHandler_MAC_SCAN_PERIOD(uint8_t header, spinel_prop_key_t key, const uint8_t *value_ptr,
                                                 uint16_t value_len)
{
    uint16_t tmp(mScanPeriod);
    spinel_ssize_t parsedLength;

    parsedLength = spinel_datatype_unpack(
                       value_ptr,
                       value_len,
                       SPINEL_DATATYPE_UINT16_S,
                       &tmp
                   );

    if (parsedLength > 0)
    {
        mScanPeriod = tmp;
        HandleCommandPropertyGet(header, key);
    }
    else
    {
        SendLastStatus(header, SPINEL_STATUS_PARSE_ERROR);
    }
}

void NcpBase::SetPropertyHandler_MAC_SCAN_STATE(uint8_t header, spinel_prop_key_t key, const uint8_t *value_ptr,
                                                uint16_t value_len)
{
    uint8_t i = 0;
    spinel_ssize_t parsedLength;
    ThreadError errorCode = kThreadError_None;

    parsedLength = spinel_datatype_unpack(
                       value_ptr,
                       value_len,
                       SPINEL_DATATYPE_UINT8_S,
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
            errorCode = otActiveScan(
                            mChannelMask,
                            mScanPeriod,
                            &HandleActiveScanResult_Jump
                        );
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

}

void NcpBase::SetPropertyHandler_MAC_15_4_PANID(uint8_t header, spinel_prop_key_t key, const uint8_t *value_ptr,
                                                uint16_t value_len)
{
    uint16_t tmp;
    spinel_ssize_t parsedLength;
    ThreadError errorCode = kThreadError_None;

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
}

void NcpBase::SetPropertyHandler_NET_ENABLED(uint8_t header, spinel_prop_key_t key, const uint8_t *value_ptr,
                                             uint16_t value_len)
{
    bool value = false;
    spinel_ssize_t parsedLength;
    ThreadError errorCode = kThreadError_None;

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
}

void NcpBase::SetPropertyHandler_NET_STATE(uint8_t header, spinel_prop_key_t key, const uint8_t *value_ptr,
                                           uint16_t value_len)
{
    unsigned int i(0);
    spinel_ssize_t parsedLength;
    ThreadError errorCode = kThreadError_None;

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
            if (otIsInterfaceUp())
            {
                errorCode = otInterfaceDown();
            }

            break;

        case SPINEL_NET_STATE_DETACHED:
            if (!otIsInterfaceUp())
            {
                errorCode = otInterfaceUp();
            }

            if ((errorCode == kThreadError_None) && (otGetDeviceRole() != kDeviceRoleDisabled))
            {
                errorCode = otThreadStop();
            }

            break;

        case SPINEL_NET_STATE_ATTACHING:
        case SPINEL_NET_STATE_ATTACHED:
            if (!otIsInterfaceUp())
            {
                errorCode = otInterfaceUp();
            }

            if ((errorCode == kThreadError_None) && (otGetDeviceRole() == kDeviceRoleDetached))
            {
                errorCode = otThreadStart();

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
}

void NcpBase::SetPropertyHandler_NET_ROLE(uint8_t header, spinel_prop_key_t key, const uint8_t *value_ptr,
                                          uint16_t value_len)
{
    unsigned int i(0);
    spinel_ssize_t parsedLength;
    ThreadError errorCode = kThreadError_None;

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
}

void NcpBase::SetPropertyHandler_NET_NETWORK_NAME(uint8_t header, spinel_prop_key_t key, const uint8_t *value_ptr,
                                                  uint16_t value_len)
{
    const char *string(NULL);
    spinel_ssize_t parsedLength;
    ThreadError errorCode = kThreadError_None;

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
}

void NcpBase::SetPropertyHandler_NET_XPANID(uint8_t header, spinel_prop_key_t key, const uint8_t *value_ptr,
                                            uint16_t value_len)
{
    const uint8_t *ptr = NULL;
    spinel_size_t len;
    spinel_ssize_t parsedLength;

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
}

void NcpBase::SetPropertyHandler_NET_MASTER_KEY(uint8_t header, spinel_prop_key_t key, const uint8_t *value_ptr,
                                                uint16_t value_len)
{
    const uint8_t *ptr = NULL;
    spinel_size_t len;
    spinel_ssize_t parsedLength;
    ThreadError errorCode = kThreadError_None;

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
}

void NcpBase::SetPropertyHandler_NET_KEY_SEQUENCE(uint8_t header, spinel_prop_key_t key, const uint8_t *value_ptr,
                                                  uint16_t value_len)
{
    unsigned int i(0);
    spinel_ssize_t parsedLength;

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
}

void NcpBase::SetPropertyHandler_THREAD_LOCAL_LEADER_WEIGHT(uint8_t header, spinel_prop_key_t key,
                                                            const uint8_t *value_ptr, uint16_t value_len)
{
    ThreadError errorCode = kThreadError_None;
    uint8_t value = 0;
    spinel_ssize_t parsedLength;

    parsedLength = spinel_datatype_unpack(
                       value_ptr,
                       value_len,
                       SPINEL_DATATYPE_UINT8_S,
                       &value
                   );

    if (parsedLength > 0)
    {
        otSetLocalLeaderWeight(value);
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
}



void NcpBase::SetPropertyHandler_STREAM_NET_INSECURE(uint8_t header, spinel_prop_key_t key, const uint8_t *value_ptr,
                                                     uint16_t value_len)
{
    spinel_ssize_t parsedLength;
    ThreadError errorCode = kThreadError_None;
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
        // STREAM_NET_INSECURE packets are not secured at layer 2.
        message->SetLinkSecurityEnabled(false);

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
        // May later include TX power, allow retransmits, etc...
        (void)meta_ptr;
        (void)meta_len;
        (void)parsedLength;

        errorCode = message->Append(frame_ptr, frame_len);
    }

    if (errorCode == kThreadError_None)
    {
        errorCode = otSendIp6Datagram(message);
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
        if (message)
        {
            Message::Free(*message);
        }

        SendLastStatus(header, ThreadErrorToSpinelStatus(errorCode));
    }
    (void)key;
}

void NcpBase::SetPropertyHandler_STREAM_NET(uint8_t header, spinel_prop_key_t key, const uint8_t *value_ptr,
                                            uint16_t value_len)
{
    spinel_ssize_t parsedLength;
    ThreadError errorCode = kThreadError_None;
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
        // STREAM_NET requires layer 2 security.
        message->SetLinkSecurityEnabled(true);

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
        // May later include TX power, allow retransmits, etc...
        (void)meta_ptr;
        (void)meta_len;
        (void)parsedLength;

        errorCode = message->Append(frame_ptr, frame_len);
    }

    if (errorCode == kThreadError_None)
    {
        errorCode = otSendIp6Datagram(message);
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
        if (message)
        {
            Message::Free(*message);
        }

        SendLastStatus(header, ThreadErrorToSpinelStatus(errorCode));
    }
    (void)key;
}

void NcpBase::SetPropertyHandler_IPV6_ML_PREFIX(uint8_t header, spinel_prop_key_t key, const uint8_t *value_ptr,
                                                uint16_t value_len)
{
    ThreadError errorCode = kThreadError_None;

    if (value_len >= 8)
    {
        errorCode = otSetMeshLocalPrefix(value_ptr);
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
}

void
NcpBase::SetPropertyHandler_THREAD_ASSISTING_PORTS(uint8_t header, spinel_prop_key_t key, const uint8_t *value_ptr, uint16_t value_len)
{
    ThreadError errorCode = kThreadError_None;
    uint8_t num_entries = 0;
    const uint16_t* ports = otGetUnsecurePorts(&num_entries);
    spinel_ssize_t parsedLength = 0;
    int ports_changed = 0;

    // First, we need to remove all of the current assisting ports.
    for (; num_entries != 0; ports++, num_entries--)
    {
        errorCode = otRemoveUnsecurePort(*ports);

        if (errorCode != kThreadError_None)
        {
            break;
        }

        ports_changed++;
    }

    while ( (errorCode == kThreadError_None)
         && (parsedLength > 0)
         && (value_len >= 2)
    ) {
        uint16_t port;

        parsedLength = spinel_datatype_unpack(
                           value_ptr,
                           value_len,
                           "S",
                           &port
                       );

        if (parsedLength > 0)
        {
            errorCode = otAddUnsecurePort(port);
        }
        else
        {
            errorCode = kThreadError_Parse;
        }

        if (errorCode != kThreadError_None)
        {
            break;
        }

        ports_changed++;
    }

    if (errorCode == kThreadError_None)
    {
        HandleCommandPropertyGet(header, key);
    }
    else
    {
        SendLastStatus(header, ThreadErrorToSpinelStatus(errorCode));

        if (ports_changed)
        {
            // We had an error, but we've actually changed
            // the state of these ports---so we need to report
            // those incomplete changes via an asynchronous
            // change event.
            HandleCommandPropertyGet(SPINEL_HEADER_FLAG | SPINEL_HEADER_IID_0, key);
        }
    }
}

void
NcpBase::SetPropertyHandler_THREAD_ALLOW_LOCAL_NET_DATA_CHANGE(uint8_t header, spinel_prop_key_t key,
                                                              const uint8_t *value_ptr, uint16_t value_len)
{
    bool value = false;
    spinel_ssize_t parsedLength;
    ThreadError errorCode = kThreadError_None;
    bool should_register_with_leader = false;

    parsedLength = spinel_datatype_unpack(
                       value_ptr,
                       value_len,
                       SPINEL_DATATYPE_BOOL_S,
                       &value
                   );

    if (parsedLength > 0)
    {
        // Register any net data changes on transition from `true` to `false`.
        should_register_with_leader = (mAllowLocalNetworkDataChange == true) && (value == false);

        mAllowLocalNetworkDataChange = value;
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

    if (should_register_with_leader)
    {
        otSendServerData();
    }
}

void NcpBase::SetPropertyHandler_CNTR_RESET(uint8_t header, spinel_prop_key_t key, const uint8_t *value_ptr,
                                                uint16_t value_len)
{
    ThreadError errorCode = kThreadError_None;
    uint8_t value = 0;
    spinel_ssize_t parsedLength;

    parsedLength = spinel_datatype_unpack(
                       value_ptr,
                       value_len,
                       SPINEL_DATATYPE_UINT8_S,
                       &value
                   );

    if (parsedLength > 0)
    {
        if (value == 1)
        {
            // TODO: Implement counter reset!
            errorCode = kThreadError_NotImplemented;
        }
        else
        {
            errorCode = kThreadError_InvalidArgs;
        }
    }
    else
    {
        errorCode = kThreadError_Parse;
    }

    // There is currently no getter for PROP_CNTR_RESET, so we just
    // return SPINEL_STATUS_OK for success when the counters are reset.
    SendLastStatus(header, ThreadErrorToSpinelStatus(errorCode));
    (void)key;
}

// ----------------------------------------------------------------------------
// MARK: Individual Property Inserters
// ----------------------------------------------------------------------------


void NcpBase::InsertPropertyHandler_IPV6_ADDRESS_TABLE(uint8_t header, spinel_prop_key_t key, const uint8_t *value_ptr,
                                                       uint16_t value_len)
{
    spinel_ssize_t parsedLength;
    ThreadError errorCode = kThreadError_None;
    spinel_status_t errorStatus = SPINEL_STATUS_OK;
    otNetifAddress *netif_addr = NULL;
    otIp6Address *addr_ptr;
    uint32_t preferred_lifetime;
    uint32_t valid_lifetime;
    uint8_t  prefix_len;

    parsedLength = spinel_datatype_unpack(
                       value_ptr,
                       value_len,
                       "6CLL",
                       &addr_ptr,
                       &prefix_len,
                       &preferred_lifetime,
                       &valid_lifetime
                   );

    VerifyOrExit(parsedLength > 0, errorStatus = SPINEL_STATUS_PARSE_ERROR);

    VerifyOrExit(prefix_len != kUnusedNetifAddressPrefixLen, errorStatus = SPINEL_STATUS_INVALID_ARGUMENT);

    for (unsigned i = 0; i < sizeof(mNetifAddresses) / sizeof(mNetifAddresses[0]); i++)
    {
        if (mNetifAddresses[i].mPrefixLength != kUnusedNetifAddressPrefixLen)
        {
            // If the address matches an already added address.
            if (memcmp(&mNetifAddresses[i].mAddress, addr_ptr, sizeof(otIp6Address)) == 0)
            {
                netif_addr = &mNetifAddresses[i];
                break;
            }
        }
        else
        {
            if (netif_addr == NULL)
            {
                netif_addr = &mNetifAddresses[i];
            }
        }
    }

    VerifyOrExit(netif_addr != NULL, errorStatus = SPINEL_STATUS_NOMEM);

    netif_addr->mAddress = *addr_ptr;
    netif_addr->mPrefixLength = prefix_len;
    netif_addr->mPreferredLifetime = preferred_lifetime;
    netif_addr->mValidLifetime = valid_lifetime;

    errorCode = otAddUnicastAddress(netif_addr);

    // `kThreadError_Busy` indicates that the address was already on the list. In this case the lifetimes and prefix len
    // are updated, and the add/insert operation is considered a success.

    VerifyOrExit(errorCode == kThreadError_None || errorCode == kThreadError_Busy,
        errorStatus = ThreadErrorToSpinelStatus(errorCode));

    SendPropteryUpdate(
        header,
        SPINEL_CMD_PROP_VALUE_INSERTED,
        key,
        value_ptr,
        value_len
    );

    errorStatus = SPINEL_STATUS_OK;

exit:
    if (errorStatus != SPINEL_STATUS_OK)
    {
        SendLastStatus(header, errorStatus);
    }
}

void NcpBase::InsertPropertyHandler_THREAD_LOCAL_ROUTES(uint8_t header, spinel_prop_key_t key, const uint8_t *value_ptr,
                                                        uint16_t value_len)
{
    const static int kPreferenceOffset = 6;
    const static int kPreferenceMask = 3 << kPreferenceOffset;

    spinel_ssize_t parsedLength;
    ThreadError errorCode = kThreadError_None;

    otExternalRouteConfig ext_route_config;
    otIp6Address *addr_ptr;
    bool stable = false;
    uint8_t flags = 0;

    memset(&ext_route_config, 0, sizeof(otExternalRouteConfig));

    VerifyOrExit(
        mAllowLocalNetworkDataChange == true,
        SendLastStatus(header, SPINEL_STATUS_INVALID_STATE)
    );

    parsedLength = spinel_datatype_unpack(
                       value_ptr,
                       value_len,
                       "6CbC",
                       &addr_ptr,
                       &ext_route_config.mPrefix.mLength,
                       &stable,
                       &flags
                   );

    if (parsedLength > 0)
    {
        ext_route_config.mPrefix.mPrefix = *addr_ptr;
        ext_route_config.mStable = stable;
        ext_route_config.mPreference = ((flags & kPreferenceMask) >> kPreferenceOffset);
        errorCode = otAddExternalRoute(&ext_route_config);

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

exit:
    return;
}

void NcpBase::InsertPropertyHandler_THREAD_ON_MESH_NETS(uint8_t header, spinel_prop_key_t key, const uint8_t *value_ptr,
                                                        uint16_t value_len)
{
    const static int kPreferenceOffset = 6;
    const static int kPreferenceMask = 3 << kPreferenceOffset;
    const static int kPreferredFlag = 1 << 5;
    const static int kValidFlag = 1 << 4;
    const static int kDhcpFlag = 1 << 3;
    const static int kConfigureFlag = 1 << 2;
    const static int kDefaultRouteFlag = 1 << 1;

    spinel_ssize_t parsedLength;
    ThreadError errorCode = kThreadError_None;

    otBorderRouterConfig border_router_config;
    otIp6Address *addr_ptr;
    bool stable = false;
    uint8_t flags = 0;

    memset(&border_router_config, 0, sizeof(otBorderRouterConfig));

    VerifyOrExit(
        mAllowLocalNetworkDataChange == true,
        SendLastStatus(header, SPINEL_STATUS_INVALID_STATE)
    );

    parsedLength = spinel_datatype_unpack(
                       value_ptr,
                       value_len,
                       "6CbC",
                       &addr_ptr,
                       &border_router_config.mPrefix.mLength,
                       &stable,
                       &flags
                   );

    if (parsedLength > 0)
    {
        border_router_config.mPrefix.mPrefix = *addr_ptr;
        border_router_config.mStable = stable;
        border_router_config.mPreference = ((flags & kPreferenceMask) >> kPreferenceOffset);
        border_router_config.mSlaacPreferred = ((flags & kPreferredFlag) == kPreferredFlag);
        border_router_config.mSlaacValid = ((flags & kValidFlag) == kValidFlag);
        border_router_config.mDhcp = ((flags & kDhcpFlag) == kDhcpFlag);
        border_router_config.mConfigure = ((flags & kConfigureFlag) == kConfigureFlag);
        border_router_config.mDefaultRoute = ((flags & kDefaultRouteFlag) == kDefaultRouteFlag);

        errorCode = otAddBorderRouter(&border_router_config);

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

exit:
    return;
}

void
NcpBase::InsertPropertyHandler_THREAD_ASSISTING_PORTS(uint8_t header, spinel_prop_key_t key, const uint8_t *value_ptr, uint16_t value_len)
{
    spinel_ssize_t parsedLength;
    ThreadError errorCode = kThreadError_None;
    uint16_t port;

    parsedLength = spinel_datatype_unpack(
                       value_ptr,
                       value_len,
                       "S",
                       &port
                   );

    if (parsedLength > 0)
    {
        errorCode = otAddUnsecurePort(port);

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
}


// ----------------------------------------------------------------------------
// MARK: Individual Property Removers
// ----------------------------------------------------------------------------


void NcpBase::RemovePropertyHandler_IPV6_ADDRESS_TABLE(uint8_t header, spinel_prop_key_t key, const uint8_t *value_ptr,
                                                       uint16_t value_len)
{
    spinel_ssize_t parsedLength;
    ThreadError errorCode = kThreadError_None;
    otNetifAddress *netif_addr = NULL;
    otIp6Address *addr_ptr;

    parsedLength = spinel_datatype_unpack(
                       value_ptr,
                       value_len,
                       "6CLL",
                       &addr_ptr,
                       NULL,
                       NULL,
                       NULL
                   );

    if (parsedLength > 0)
    {
        for (unsigned i = 0; i < sizeof(mNetifAddresses) / sizeof(mNetifAddresses[0]); i++)
        {
            if (mNetifAddresses[i].mPrefixLength != kUnusedNetifAddressPrefixLen)
            {
                if (memcmp(&mNetifAddresses[i].mAddress, addr_ptr, sizeof(otIp6Address)) == 0)
                {
                    netif_addr = &mNetifAddresses[i];
                    break;
                }
            }
        }

        if (netif_addr != NULL)
        {
            errorCode = otRemoveUnicastAddress(netif_addr);

            if (errorCode == kThreadError_None)
            {
                netif_addr->mNext = NULL;

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
            SendLastStatus(header, ThreadErrorToSpinelStatus(kThreadError_NoAddress));
        }
    }
    else
    {
        SendLastStatus(header, SPINEL_STATUS_PARSE_ERROR);
    }
}

void NcpBase::RemovePropertyHandler_THREAD_LOCAL_ROUTES(uint8_t header, spinel_prop_key_t key, const uint8_t *value_ptr,
                                                        uint16_t value_len)
{
    spinel_ssize_t parsedLength;
    ThreadError errorCode = kThreadError_None;

    otIp6Prefix ip6_prefix;
    memset(&ip6_prefix, 0, sizeof(otIp6Prefix));
    otIp6Address *addr_ptr;

    VerifyOrExit(
        mAllowLocalNetworkDataChange == true,
        SendLastStatus(header, SPINEL_STATUS_INVALID_STATE)
    );

    parsedLength = spinel_datatype_unpack(
                       value_ptr,
                       value_len,
                       "6C",
                       &addr_ptr,
                       &ip6_prefix.mLength
                   );

    if (parsedLength > 0)
    {
        ip6_prefix.mPrefix = *addr_ptr;
        errorCode = otRemoveExternalRoute(&ip6_prefix);

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

exit:
    return;
}

void NcpBase::RemovePropertyHandler_THREAD_ON_MESH_NETS(uint8_t header, spinel_prop_key_t key, const uint8_t *value_ptr,
                                                        uint16_t value_len)
{
    spinel_ssize_t parsedLength;
    ThreadError errorCode = kThreadError_None;

    otIp6Prefix ip6_prefix;
    memset(&ip6_prefix, 0, sizeof(otIp6Prefix));
    otIp6Address *addr_ptr;

    VerifyOrExit(
        mAllowLocalNetworkDataChange == true,
        SendLastStatus(header, SPINEL_STATUS_INVALID_STATE)
    );

    parsedLength = spinel_datatype_unpack(
                       value_ptr,
                       value_len,
                       "6C",
                       &addr_ptr,
                       &ip6_prefix.mLength
                   );

    if (parsedLength > 0)
    {
        ip6_prefix.mPrefix = *addr_ptr;
        errorCode = otRemoveBorderRouter(&ip6_prefix);

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

exit:
    return;
}

void
NcpBase::RemovePropertyHandler_THREAD_ASSISTING_PORTS(uint8_t header, spinel_prop_key_t key, const uint8_t *value_ptr, uint16_t value_len)
{
    spinel_ssize_t parsedLength;
    ThreadError errorCode = kThreadError_None;
    uint16_t port;

    parsedLength = spinel_datatype_unpack(
                       value_ptr,
                       value_len,
                       "S",
                       &port
                   );

    if (parsedLength > 0)
    {
        errorCode = otRemoveUnsecurePort(port);

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
}


}  // namespace Thread
