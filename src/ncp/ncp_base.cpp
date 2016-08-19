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
#include <ncp/ncp.h>
#include <ncp/ncp_base.hpp>
#include <openthread.h>
#include <openthread-diag.h>
#include <stdarg.h>
#include <platform/radio.h>
#include <platform/misc.h>

namespace Thread
{

extern ThreadNetif *sThreadNetif;

static NcpBase *sNcpContext = NULL;

#define NCP_PLAT_RESET_REASON        (1U<<31)

enum
{
    kThreadModeTLV_Receiver    = (1 << 3),
    kThreadModeTLV_Secure      = (1 << 2),
    kThreadModeTLV_DeviceType  = (1 << 1),
    kThreadModeTLV_NetworkData = (1 << 0),
};

#define RSSI_OVERRIDE_DISABLED        127 // Used for PROP_MAC_WHITELIST

#define IGNORE_RETURN_VALUE(s)        do { if (s){} } while (0)

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

    { SPINEL_PROP_NET_IF_UP, &NcpBase::GetPropertyHandler_NET_IF_UP },
    { SPINEL_PROP_NET_STACK_UP, &NcpBase::GetPropertyHandler_NET_STACK_UP },
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

    { SPINEL_PROP_MAC_WHITELIST, &NcpBase::GetPropertyHandler_MAC_WHITELIST },
    { SPINEL_PROP_MAC_WHITELIST_ENABLED, &NcpBase::GetPropertyHandler_MAC_WHITELIST_ENABLED },
    { SPINEL_PROP_THREAD_MODE, &NcpBase::GetPropertyHandler_THREAD_MODE },
    { SPINEL_PROP_THREAD_CHILD_TIMEOUT, &NcpBase::GetPropertyHandler_THREAD_CHILD_TIMEOUT },
    { SPINEL_PROP_THREAD_RLOC16, &NcpBase::GetPropertyHandler_THREAD_RLOC16 },
    { SPINEL_PROP_THREAD_ROUTER_UPGRADE_THRESHOLD, &NcpBase::GetPropertyHandler_THREAD_ROUTER_UPGRADE_THRESHOLD },
    { SPINEL_PROP_THREAD_CONTEXT_REUSE_DELAY, &NcpBase::GetPropertyHandler_THREAD_CONTEXT_REUSE_DELAY },
    { SPINEL_PROP_THREAD_NETWORK_ID_TIMEOUT, &NcpBase::GetPropertyHandler_THREAD_NETWORK_ID_TIMEOUT },

    { SPINEL_PROP_IPV6_ML_PREFIX, &NcpBase::GetPropertyHandler_IPV6_ML_PREFIX },
    { SPINEL_PROP_IPV6_ML_ADDR, &NcpBase::GetPropertyHandler_IPV6_ML_ADDR },
    { SPINEL_PROP_IPV6_LL_ADDR, &NcpBase::GetPropertyHandler_IPV6_LL_ADDR },
    { SPINEL_PROP_IPV6_ADDRESS_TABLE, &NcpBase::GetPropertyHandler_IPV6_ADDRESS_TABLE },
    { SPINEL_PROP_IPV6_ROUTE_TABLE, &NcpBase::GetPropertyHandler_IPV6_ROUTE_TABLE },

    { SPINEL_PROP_STREAM_NET, &NcpBase::GetPropertyHandler_STREAM_NET },

    { SPINEL_PROP_CNTR_TX_PKT_TOTAL, &NcpBase::GetPropertyHandler_MAC_CNTR },
    { SPINEL_PROP_CNTR_TX_PKT_ACK_REQ, &NcpBase::GetPropertyHandler_MAC_CNTR },
    { SPINEL_PROP_CNTR_TX_PKT_ACKED, &NcpBase::GetPropertyHandler_MAC_CNTR },
    { SPINEL_PROP_CNTR_TX_PKT_NO_ACK_REQ, &NcpBase::GetPropertyHandler_MAC_CNTR },
    { SPINEL_PROP_CNTR_TX_PKT_DATA, &NcpBase::GetPropertyHandler_MAC_CNTR },
    { SPINEL_PROP_CNTR_TX_PKT_DATA_POLL, &NcpBase::GetPropertyHandler_MAC_CNTR },
    { SPINEL_PROP_CNTR_TX_PKT_BEACON, &NcpBase::GetPropertyHandler_MAC_CNTR },
    { SPINEL_PROP_CNTR_TX_PKT_BEACON_REQ, &NcpBase::GetPropertyHandler_MAC_CNTR },
    { SPINEL_PROP_CNTR_TX_PKT_OTHER, &NcpBase::GetPropertyHandler_MAC_CNTR },
    { SPINEL_PROP_CNTR_TX_PKT_RETRY, &NcpBase::GetPropertyHandler_MAC_CNTR },
    { SPINEL_PROP_CNTR_TX_ERR_CCA, &NcpBase::GetPropertyHandler_MAC_CNTR },
    { SPINEL_PROP_CNTR_RX_PKT_TOTAL, &NcpBase::GetPropertyHandler_MAC_CNTR },
    { SPINEL_PROP_CNTR_RX_PKT_DATA, &NcpBase::GetPropertyHandler_MAC_CNTR },
    { SPINEL_PROP_CNTR_RX_PKT_DATA_POLL, &NcpBase::GetPropertyHandler_MAC_CNTR },
    { SPINEL_PROP_CNTR_RX_PKT_BEACON, &NcpBase::GetPropertyHandler_MAC_CNTR },
    { SPINEL_PROP_CNTR_RX_PKT_BEACON_REQ, &NcpBase::GetPropertyHandler_MAC_CNTR },
    { SPINEL_PROP_CNTR_RX_PKT_OTHER, &NcpBase::GetPropertyHandler_MAC_CNTR },
    { SPINEL_PROP_CNTR_RX_PKT_FILT_WL, &NcpBase::GetPropertyHandler_MAC_CNTR },
    { SPINEL_PROP_CNTR_RX_PKT_FILT_DA, &NcpBase::GetPropertyHandler_MAC_CNTR },
    { SPINEL_PROP_CNTR_RX_ERR_EMPTY, &NcpBase::GetPropertyHandler_MAC_CNTR },
    { SPINEL_PROP_CNTR_RX_ERR_UKWN_NBR, &NcpBase::GetPropertyHandler_MAC_CNTR },
    { SPINEL_PROP_CNTR_RX_ERR_NVLD_SADDR, &NcpBase::GetPropertyHandler_MAC_CNTR },
    { SPINEL_PROP_CNTR_RX_ERR_SECURITY, &NcpBase::GetPropertyHandler_MAC_CNTR },
    { SPINEL_PROP_CNTR_RX_ERR_BAD_FCS, &NcpBase::GetPropertyHandler_MAC_CNTR },
    { SPINEL_PROP_CNTR_RX_ERR_OTHER, &NcpBase::GetPropertyHandler_MAC_CNTR },

    { SPINEL_PROP_CNTR_TX_IP_SEC_TOTAL, &NcpBase::GetPropertyHandler_NCP_CNTR },
    { SPINEL_PROP_CNTR_TX_IP_INSEC_TOTAL, &NcpBase::GetPropertyHandler_NCP_CNTR },
    { SPINEL_PROP_CNTR_TX_IP_DROPPED, &NcpBase::GetPropertyHandler_NCP_CNTR },
    { SPINEL_PROP_CNTR_RX_IP_SEC_TOTAL, &NcpBase::GetPropertyHandler_NCP_CNTR },
    { SPINEL_PROP_CNTR_RX_IP_INSEC_TOTAL, &NcpBase::GetPropertyHandler_NCP_CNTR },
    { SPINEL_PROP_CNTR_RX_IP_DROPPED, &NcpBase::GetPropertyHandler_NCP_CNTR },
    { SPINEL_PROP_CNTR_TX_SPINEL_TOTAL, &NcpBase::GetPropertyHandler_NCP_CNTR },
    { SPINEL_PROP_CNTR_RX_SPINEL_TOTAL, &NcpBase::GetPropertyHandler_NCP_CNTR },
    { SPINEL_PROP_CNTR_RX_SPINEL_ERR, &NcpBase::GetPropertyHandler_NCP_CNTR },
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

    { SPINEL_PROP_NET_IF_UP, &NcpBase::SetPropertyHandler_NET_IF_UP },
    { SPINEL_PROP_NET_STACK_UP, &NcpBase::SetPropertyHandler_NET_STACK_UP },
    { SPINEL_PROP_NET_ROLE, &NcpBase::SetPropertyHandler_NET_ROLE },
    { SPINEL_PROP_NET_NETWORK_NAME, &NcpBase::SetPropertyHandler_NET_NETWORK_NAME },
    { SPINEL_PROP_NET_XPANID, &NcpBase::SetPropertyHandler_NET_XPANID },
    { SPINEL_PROP_NET_MASTER_KEY, &NcpBase::SetPropertyHandler_NET_MASTER_KEY },
    { SPINEL_PROP_NET_KEY_SEQUENCE, &NcpBase::SetPropertyHandler_NET_KEY_SEQUENCE },

    { SPINEL_PROP_THREAD_LOCAL_LEADER_WEIGHT, &NcpBase::SetPropertyHandler_THREAD_LOCAL_LEADER_WEIGHT },
    { SPINEL_PROP_THREAD_ASSISTING_PORTS, &NcpBase::SetPropertyHandler_THREAD_ASSISTING_PORTS },
    { SPINEL_PROP_THREAD_ALLOW_LOCAL_NET_DATA_CHANGE, &NcpBase::SetPropertyHandler_THREAD_ALLOW_LOCAL_NET_DATA_CHANGE},
    { SPINEL_PROP_THREAD_NETWORK_ID_TIMEOUT, &NcpBase::SetPropertyHandler_THREAD_NETWORK_ID_TIMEOUT },

    { SPINEL_PROP_STREAM_NET_INSECURE, &NcpBase::SetPropertyHandler_STREAM_NET_INSECURE },
    { SPINEL_PROP_STREAM_NET, &NcpBase::SetPropertyHandler_STREAM_NET },

    { SPINEL_PROP_IPV6_ML_PREFIX, &NcpBase::SetPropertyHandler_IPV6_ML_PREFIX },

    { SPINEL_PROP_MAC_WHITELIST, &NcpBase::SetPropertyHandler_MAC_WHITELIST },
    { SPINEL_PROP_MAC_WHITELIST_ENABLED, &NcpBase::SetPropertyHandler_MAC_WHITELIST_ENABLED },
    { SPINEL_PROP_THREAD_MODE, &NcpBase::SetPropertyHandler_THREAD_MODE },
    { SPINEL_PROP_THREAD_CHILD_TIMEOUT, &NcpBase::SetPropertyHandler_THREAD_CHILD_TIMEOUT },
    { SPINEL_PROP_THREAD_ROUTER_UPGRADE_THRESHOLD, &NcpBase::SetPropertyHandler_THREAD_ROUTER_UPGRADE_THRESHOLD },
    { SPINEL_PROP_THREAD_CONTEXT_REUSE_DELAY, &NcpBase::SetPropertyHandler_THREAD_CONTEXT_REUSE_DELAY },

#if OPENTHREAD_ENABLE_DIAG
    { SPINEL_PROP_NEST_STREAM_MFG, &NcpBase::SetPropertyHandler_NEST_STREAM_MFG },
#endif
};

const NcpBase::InsertPropertyHandlerEntry NcpBase::mInsertPropertyHandlerTable[] =
{
    { SPINEL_PROP_IPV6_ADDRESS_TABLE, &NcpBase::NcpBase::InsertPropertyHandler_IPV6_ADDRESS_TABLE },
    { SPINEL_PROP_THREAD_LOCAL_ROUTES, &NcpBase::NcpBase::InsertPropertyHandler_THREAD_LOCAL_ROUTES },
    { SPINEL_PROP_THREAD_ON_MESH_NETS, &NcpBase::NcpBase::InsertPropertyHandler_THREAD_ON_MESH_NETS },
    { SPINEL_PROP_THREAD_ASSISTING_PORTS, &NcpBase::NcpBase::InsertPropertyHandler_THREAD_ASSISTING_PORTS },

    { SPINEL_PROP_CNTR_RESET, &NcpBase::SetPropertyHandler_CNTR_RESET },

    { SPINEL_PROP_MAC_WHITELIST, &NcpBase::InsertPropertyHandler_MAC_WHITELIST },
};

const NcpBase::RemovePropertyHandlerEntry NcpBase::mRemovePropertyHandlerTable[] =
{
    { SPINEL_PROP_IPV6_ADDRESS_TABLE, &NcpBase::NcpBase::RemovePropertyHandler_IPV6_ADDRESS_TABLE },
    { SPINEL_PROP_THREAD_LOCAL_ROUTES, &NcpBase::NcpBase::RemovePropertyHandler_THREAD_LOCAL_ROUTES },
    { SPINEL_PROP_THREAD_ON_MESH_NETS, &NcpBase::NcpBase::RemovePropertyHandler_THREAD_ON_MESH_NETS },
    { SPINEL_PROP_THREAD_ASSISTING_PORTS, &NcpBase::NcpBase::RemovePropertyHandler_THREAD_ASSISTING_PORTS },
    { SPINEL_PROP_MAC_WHITELIST, &NcpBase::RemovePropertyHandler_MAC_WHITELIST },
    { SPINEL_PROP_THREAD_ACTIVE_ROUTER_IDS, &NcpBase::RemovePropertyHandler_THREAD_ACTIVE_ROUTER_IDS },
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

    case kThreadError_Already:
        ret = SPINEL_STATUS_ALREADY;
        break;

    case kThreadError_NotFound:
        ret = SPINEL_STATUS_ITEM_NOT_FOUND;
        break;


    default:
        // Unknown error code. Wrap it as a Spinel status and return that.
        ret = static_cast<spinel_status_t>(SPINEL_STATUS_STACK_NATIVE__BEGIN + error);
        break;
    }

    return ret;
}

static spinel_status_t ResetReasonToSpinelStatus(otPlatResetReason reason)
{
    spinel_status_t ret;
    switch (reason)
    {
    case kPlatResetReason_PowerOn:
        ret = SPINEL_STATUS_RESET_POWER_ON;
        break;
    case kPlatResetReason_External:
        ret = SPINEL_STATUS_RESET_EXTERNAL;
        break;
    case kPlatResetReason_Software:
        ret = SPINEL_STATUS_RESET_SOFTWARE;
        break;
    case kPlatResetReason_Fault:
        ret = SPINEL_STATUS_RESET_FAULT;
        break;
    case kPlatResetReason_Crash:
        ret = SPINEL_STATUS_RESET_CRASH;
        break;
    case kPlatResetReason_Assert:
        ret = SPINEL_STATUS_RESET_ASSERT;
        break;
    case kPlatResetReason_Watchdog:
        ret = SPINEL_STATUS_RESET_WATCHDOG;
        break;
    case kPlatResetReason_Other:
        ret = SPINEL_STATUS_RESET_OTHER;
        break;
    default:
        ret = SPINEL_STATUS_RESET_UNKNOWN;
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
    mShouldSignalEndOfScan = false;
    sNcpContext = this;
    mChangedFlags = NCP_PLAT_RESET_REASON;
    mAllowLocalNetworkDataChange = false;

    mFramingErrorCounter = 0;
    mRxSpinelFrameCounter = 0;
    mTxSpinelFrameCounter = 0;
    mInboundSecureIpFrameCounter = 0;
    mInboundInsecureIpFrameCounter = 0;
    mOutboundSecureIpFrameCounter = 0;
    mOutboundInsecureIpFrameCounter = 0;
    mDroppedOutboundIpFrameCounter = 0;
    mDroppedInboundIpFrameCounter = 0;

    for (unsigned i = 0; i < sizeof(mNetifAddresses) / sizeof(mNetifAddresses[0]); i++)
    {
        mNetifAddresses[i].mPrefixLength = kUnusedNetifAddressPrefixLen;
    }

    assert(sThreadNetif != NULL);
    otSetStateChangedCallback(&HandleNetifStateChanged, this);
    otSetReceiveIp6DatagramCallback(&HandleDatagramFromStack, this);
    otSetIcmpEchoEnabled(false);
}

// ----------------------------------------------------------------------------
// MARK: Outbound Datagram Handling
// ----------------------------------------------------------------------------

void NcpBase::HandleDatagramFromStack(otMessage aMessage, void *aContext)
{
    reinterpret_cast<NcpBase*>(aContext)->HandleDatagramFromStack(*static_cast<Message *>(aMessage));
}

void NcpBase::HandleDatagramFromStack(Message &aMessage)
{
    ThreadError errorCode = kThreadError_None;
    Message *message = &aMessage;
    bool isSecure = message->IsLinkSecurityEnabled();

    SuccessOrExit(errorCode = OutboundFrameBegin());

    SuccessOrExit(
        errorCode = OutboundFrameFeedPacked(
            "CiiS",
            SPINEL_HEADER_FLAG | SPINEL_HEADER_IID_0,
            SPINEL_CMD_PROP_VALUE_IS,
            isSecure
            ? SPINEL_PROP_STREAM_NET
            : SPINEL_PROP_STREAM_NET_INSECURE,
            message->GetLength()
    ));

    SuccessOrExit(errorCode = OutboundFrameFeedMessage(*message));

    // Set the message pointer to NULL, to indicate that it does not need to be freed at the exit.
    // The message is now owned by the OutboundFrame and will be freed when the frame is either successfully sent and
    // then removed, or if the frame gets discarded.
    message = NULL;

    // Append any metadata (rssi, lqi, channel, etc) here!

    SuccessOrExit(errorCode = OutboundFrameSend());

exit:

    if (message != NULL)
    {
        Message::Free(*message);
    }

    if (errorCode != kThreadError_None)
    {
        SendLastStatus(SPINEL_HEADER_FLAG | SPINEL_HEADER_IID_0, SPINEL_STATUS_DROPPED);
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
// MARK: Scan Results Glue
// ----------------------------------------------------------------------------

void NcpBase::HandleActiveScanResult_Jump(otActiveScanResult *result, void *aContext)
{
    reinterpret_cast<NcpBase*>(aContext)->HandleActiveScanResult(result);
}

void NcpBase::HandleActiveScanResult(otActiveScanResult *result)
{
    ThreadError errorCode;

    if (result)
    {
        uint8_t flags = static_cast<uint8_t>(result->mVersion << SPINEL_BEACON_THREAD_FLAG_VERSION_SHIFT);

        if (result->mIsJoinable)
        {
            flags |= SPINEL_BEACON_THREAD_FLAG_JOINABLE;
        }

        if (result->mIsNative)
        {
            flags |= SPINEL_BEACON_THREAD_FLAG_NATIVE;
        }

        //chan,rssi,(laddr,saddr,panid,lqi),(proto,flags,networkid,xpanid) [icT(ESSC)T(iCUD.).]
        NcpBase::SendPropertyUpdate(
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
            result->mNetworkName.m8,
            result->mExtendedPanId.m8, OT_EXT_PAN_ID_SIZE
        );
    }
    else
    {
        // We are finished with the scan, so send out
        // a property update indicating such.
        errorCode = SendPropertyUpdate(
            SPINEL_HEADER_FLAG | SPINEL_HEADER_IID_0,
            SPINEL_CMD_PROP_VALUE_IS,
            SPINEL_PROP_MAC_SCAN_STATE,
            SPINEL_DATATYPE_UINT8_S,
            SPINEL_SCAN_STATE_IDLE
        );

        // If we could not send the end of scan inidciator message now (no
        // buffer space), we set `mShouldSignalEndOfScan` to true to send
        // it out when buffer space becomes available.
        if (errorCode != kThreadError_None)
        {
            mShouldSignalEndOfScan = true;
        }
    }
}

// ----------------------------------------------------------------------------
// MARK: Address Table Changed Glue
// ----------------------------------------------------------------------------

void NcpBase::HandleNetifStateChanged(uint32_t flags, void *context)
{
    NcpBase *obj = reinterpret_cast<NcpBase *>(context);

    obj->mChangedFlags |= flags;

    obj->mUpdateChangedPropsTask.Post();
}

void NcpBase::UpdateChangedProps(void *context)
{
    NcpBase *obj = reinterpret_cast<NcpBase *>(context);
    obj->UpdateChangedProps();
}

void NcpBase::UpdateChangedProps(void)
{
    while (mChangedFlags != 0)
    {
        if ((mChangedFlags & NCP_PLAT_RESET_REASON) != 0)
        {
            SuccessOrExit(SendLastStatus(
                SPINEL_HEADER_FLAG | SPINEL_HEADER_IID_0,
                ResetReasonToSpinelStatus(otPlatGetResetReason())
            ));
            mChangedFlags &= ~static_cast<uint32_t>(NCP_PLAT_RESET_REASON);
        }
        else if ((mChangedFlags & OT_IP6_LL_ADDR_CHANGED) != 0)
        {
            SuccessOrExit(HandleCommandPropertyGet(
                SPINEL_HEADER_FLAG | SPINEL_HEADER_IID_0,
                SPINEL_PROP_IPV6_LL_ADDR
            ));
            mChangedFlags &= ~static_cast<uint32_t>(OT_IP6_LL_ADDR_CHANGED);
        }
        else if ((mChangedFlags & OT_IP6_ML_ADDR_CHANGED) != 0)
        {
            SuccessOrExit(HandleCommandPropertyGet(
                              SPINEL_HEADER_FLAG | SPINEL_HEADER_IID_0,
                              SPINEL_PROP_IPV6_ML_ADDR
                          ));
            mChangedFlags &= ~static_cast<uint32_t>(OT_IP6_ML_ADDR_CHANGED);
        }
        else if ((mChangedFlags & OT_NET_ROLE) != 0)
        {
            SuccessOrExit(HandleCommandPropertyGet(
                              SPINEL_HEADER_FLAG | SPINEL_HEADER_IID_0,
                              SPINEL_PROP_NET_ROLE
                          ));
            mChangedFlags &= ~static_cast<uint32_t>(OT_NET_ROLE);

        }
        else if ((mChangedFlags & OT_NET_PARTITION_ID) != 0)
        {
            SuccessOrExit(HandleCommandPropertyGet(
                              SPINEL_HEADER_FLAG | SPINEL_HEADER_IID_0,
                              SPINEL_PROP_NET_PARTITION_ID
                          ));
            mChangedFlags &= ~static_cast<uint32_t>(OT_NET_PARTITION_ID);
        }
        else if ((mChangedFlags & OT_NET_KEY_SEQUENCE) != 0)
        {
            SuccessOrExit(HandleCommandPropertyGet(
                              SPINEL_HEADER_FLAG | SPINEL_HEADER_IID_0,
                              SPINEL_PROP_NET_KEY_SEQUENCE
                          ));
            mChangedFlags &= ~static_cast<uint32_t>(OT_NET_KEY_SEQUENCE);
        }
        else if ((mChangedFlags & (OT_IP6_ADDRESS_ADDED | OT_IP6_ADDRESS_REMOVED)) != 0)
        {
            SuccessOrExit(HandleCommandPropertyGet(
                              SPINEL_HEADER_FLAG | SPINEL_HEADER_IID_0,
                              SPINEL_PROP_IPV6_ADDRESS_TABLE
                          ));
            mChangedFlags &= ~static_cast<uint32_t>(OT_IP6_ADDRESS_ADDED | OT_IP6_ADDRESS_REMOVED);
        }
        else if ((mChangedFlags & (OT_THREAD_CHILD_ADDED | OT_THREAD_CHILD_REMOVED)) != 0)
        {
            // TODO: Uncomment this once we add support for this property.
            //SuccessOrExit(HandleCommandPropertyGet(
            //    SPINEL_HEADER_FLAG | SPINEL_HEADER_IID_0,
            //    SPINEL_PROP_THREAD_CHILD_TABLE
            //));
            mChangedFlags &= ~static_cast<uint32_t>(OT_THREAD_CHILD_ADDED | OT_THREAD_CHILD_REMOVED);
        }
        else if ((mChangedFlags & OT_THREAD_NETDATA_UPDATED) != 0)
        {
            // TODO: Handle the netdata changed event.

            mChangedFlags &= ~static_cast<uint32_t>(OT_THREAD_NETDATA_UPDATED);
        }
    }

exit:
    return;
}

// ----------------------------------------------------------------------------
// MARK: Serial Traffic Glue
// ----------------------------------------------------------------------------

ThreadError NcpBase::OutboundFrameSend(void)
{
    ThreadError errorCode;

    SuccessOrExit(errorCode = OutboundFrameEnd());

    mTxSpinelFrameCounter++;

exit:
    return errorCode;
}

void NcpBase::HandleReceive(const uint8_t *buf, uint16_t bufLength)
{
    uint8_t header = 0;
    unsigned int command = 0;
    spinel_ssize_t parsedLength;
    const uint8_t *arg_ptr = NULL;
    unsigned int arg_len = 0;
    ThreadError errorCode = kThreadError_None;
    spinel_tid_t tid = 0;

    parsedLength = spinel_datatype_unpack(buf, bufLength, "CiD", &header, &command, &arg_ptr, &arg_len);

    if (parsedLength == bufLength)
    {
        errorCode = HandleCommand(header, command, arg_ptr, static_cast<uint16_t>(arg_len));
    }
    else
    {
        errorCode = SendLastStatus(header, SPINEL_STATUS_PARSE_ERROR);
    }

    if (errorCode == kThreadError_NoBufs)
    {
        // If we cannot send a response due to buffer space not being
        // available, we remember the TID of command so to send an
        // error status when buffer space becomes available later.

        // Valid TID range is 1-15 (zero being used as special case
        // where no reply is expected). TIDs for dropped reply are
        // stored in two variables:  `mDroppedReplyTidBitSet` which
        // is a bit set (bits 1-15 correspond to TID values 1-15).
        // The first/next dropped TID value in the set is stored in
        // `mDroppedReplyTid` (with value zero indicating that there
        // is no dropped reply).

        tid = SPINEL_HEADER_GET_TID(header);

        if (tid != 0)
        {
            if (mDroppedReplyTid == 0)
            {
                mDroppedReplyTid = tid;
            }

            mDroppedReplyTidBitSet  |= (1 << tid);
        }
    }

    mRxSpinelFrameCounter++;
}

void NcpBase::HandleSpaceAvailableInTxBuffer(void)
{
    while (mDroppedReplyTid != 0)
    {
        SuccessOrExit(
            SendLastStatus(
                SPINEL_HEADER_FLAG | SPINEL_HEADER_IID_0 | mDroppedReplyTid,
                SPINEL_STATUS_NOMEM
            )
        );

        mDroppedReplyTidBitSet &= ~(1 << mDroppedReplyTid);

        if (mDroppedReplyTidBitSet == 0)
        {
            mDroppedReplyTid = 0;

            break;
        }

        do
        {
            mDroppedReplyTid = SPINEL_GET_NEXT_TID(mDroppedReplyTid);
        }
        while ((mDroppedReplyTidBitSet & (1 << mDroppedReplyTid)) == 0);
    }

    if (mShouldSignalEndOfScan)
    {
        SuccessOrExit(
            SendPropertyUpdate(
                SPINEL_HEADER_FLAG | SPINEL_HEADER_IID_0,
                SPINEL_CMD_PROP_VALUE_IS,
                SPINEL_PROP_MAC_SCAN_STATE,
                SPINEL_DATATYPE_UINT8_S,
                SPINEL_SCAN_STATE_IDLE
        ));

        mShouldSignalEndOfScan = false;
    }

    UpdateChangedProps();

exit:
    return;
}

void NcpBase::IncrementFrameErrorCounter(void)
{
    mFramingErrorCounter++;
}

// ----------------------------------------------------------------------------
// MARK: Inbound Command Handlers
// ----------------------------------------------------------------------------

ThreadError NcpBase::HandleCommand(uint8_t header, unsigned int command, const uint8_t *arg_ptr, uint16_t arg_len)
{
    unsigned i;
    ThreadError errorCode = kThreadError_None;

    // Skip if this isn't a spinel frame
    VerifyOrExit((SPINEL_HEADER_FLAG & header) == SPINEL_HEADER_FLAG, errorCode = kThreadError_InvalidArgs);

    // We only support IID zero for now.
    VerifyOrExit(
        SPINEL_HEADER_GET_IID(header) == 0,
        errorCode = SendLastStatus(header, SPINEL_STATUS_INVALID_INTERFACE)
    );

    for (i = 0; i < sizeof(mCommandHandlerTable) / sizeof(mCommandHandlerTable[0]); i++)
    {
        if (mCommandHandlerTable[i].mCommand == command)
        {
            break;
        }
    }

    if (i < sizeof(mCommandHandlerTable) / sizeof(mCommandHandlerTable[0]))
    {
        errorCode = (this->*mCommandHandlerTable[i].mHandler)(header, command, arg_ptr, arg_len);
    }
    else
    {
        errorCode = SendLastStatus(header, SPINEL_STATUS_INVALID_COMMAND);
    }

exit:
    return errorCode;
}

ThreadError NcpBase::HandleCommandPropertyGet(uint8_t header, spinel_prop_key_t key)
{
    unsigned i;
    ThreadError errorCode = kThreadError_None;

    for (i = 0; i < sizeof(mGetPropertyHandlerTable) / sizeof(mGetPropertyHandlerTable[0]); i++)
    {
        if (mGetPropertyHandlerTable[i].mPropKey == key)
        {
            break;
        }
    }

    if (i < sizeof(mGetPropertyHandlerTable) / sizeof(mGetPropertyHandlerTable[0]))
    {
        errorCode = (this->*mGetPropertyHandlerTable[i].mHandler)(header, key);
    }
    else
    {
        errorCode = SendLastStatus(header, SPINEL_STATUS_PROP_NOT_FOUND);
    }

    return errorCode;
}

ThreadError NcpBase::HandleCommandPropertySet(uint8_t header, spinel_prop_key_t key, const uint8_t *value_ptr,
                                              uint16_t value_len)
{
    unsigned i;
    ThreadError errorCode = kThreadError_None;

    for (i = 0; i < sizeof(mSetPropertyHandlerTable) / sizeof(mSetPropertyHandlerTable[0]); i++)
    {
        if (mSetPropertyHandlerTable[i].mPropKey == key)
        {
            break;
        }
    }

    if (i < sizeof(mSetPropertyHandlerTable) / sizeof(mSetPropertyHandlerTable[0]))
    {
        errorCode = (this->*mSetPropertyHandlerTable[i].mHandler)(header, key, value_ptr, value_len);
    }
    else
    {
        errorCode = SendLastStatus(header, SPINEL_STATUS_PROP_NOT_FOUND);
    }

    return errorCode;
}

ThreadError NcpBase::HandleCommandPropertyInsert(uint8_t header, spinel_prop_key_t key, const uint8_t *value_ptr,
                                                 uint16_t value_len)
{
    unsigned i;
    ThreadError errorCode = kThreadError_None;

    for (i = 0; i < sizeof(mInsertPropertyHandlerTable) / sizeof(mInsertPropertyHandlerTable[0]); i++)
    {
        if (mInsertPropertyHandlerTable[i].mPropKey == key)
        {
            break;
        }
    }

    if (i < sizeof(mInsertPropertyHandlerTable) / sizeof(mInsertPropertyHandlerTable[0]))
    {
        errorCode = (this->*mInsertPropertyHandlerTable[i].mHandler)(header, key, value_ptr, value_len);
    }
    else
    {
        errorCode = SendLastStatus(header, SPINEL_STATUS_PROP_NOT_FOUND);
    }

    return errorCode;
}

ThreadError NcpBase::HandleCommandPropertyRemove(uint8_t header, spinel_prop_key_t key, const uint8_t *value_ptr,
                                                 uint16_t value_len)
{
    unsigned i;
    ThreadError errorCode = kThreadError_None;

    for (i = 0; i < sizeof(mRemovePropertyHandlerTable) / sizeof(mRemovePropertyHandlerTable[0]); i++)
    {
        if (mRemovePropertyHandlerTable[i].mPropKey == key)
        {
            break;
        }
    }

    if (i < sizeof(mRemovePropertyHandlerTable) / sizeof(mRemovePropertyHandlerTable[0]))
    {
        errorCode = (this->*mRemovePropertyHandlerTable[i].mHandler)(header, key, value_ptr, value_len);
    }
    else
    {
        errorCode = SendLastStatus(header, SPINEL_STATUS_PROP_NOT_FOUND);
    }

    return errorCode;
}


// ----------------------------------------------------------------------------
// MARK: Outbound Command Handlers
// ----------------------------------------------------------------------------


ThreadError NcpBase::SendLastStatus(uint8_t header, spinel_status_t lastStatus)
{
    if (SPINEL_HEADER_GET_IID(header) == 0)
    {
        mLastStatus = lastStatus;
    }

    return SendPropertyUpdate(
               header,
               SPINEL_CMD_PROP_VALUE_IS,
               SPINEL_PROP_LAST_STATUS,
               SPINEL_DATATYPE_UINT_PACKED_S,
               lastStatus
           );
}

ThreadError NcpBase::SendPropertyUpdate(uint8_t header, uint8_t command, spinel_prop_key_t key,
                                        const char *pack_format, ...)
{
    ThreadError errorCode = kThreadError_None;
    va_list     args;

    va_start(args, pack_format);
    SuccessOrExit(errorCode = OutboundFrameBegin());
    SuccessOrExit(errorCode = OutboundFrameFeedPacked("Cii", header, command, key));
    SuccessOrExit(errorCode = OutboundFrameFeedVPacked(pack_format, args));
    SuccessOrExit(errorCode = OutboundFrameSend());

exit:
    va_end(args);
    return errorCode;
}

ThreadError NcpBase::SendPropertyUpdate(uint8_t header, uint8_t command, spinel_prop_key_t key,
                                        const uint8_t *value_ptr, uint16_t value_len)
{
    ThreadError errorCode = kThreadError_None;

    SuccessOrExit(errorCode = OutboundFrameBegin());
    SuccessOrExit(errorCode = OutboundFrameFeedPacked("Cii", header, command, key));
    SuccessOrExit(errorCode = OutboundFrameFeedData(value_ptr, value_len));
    SuccessOrExit(errorCode = OutboundFrameSend());

exit:
    return errorCode;
}

ThreadError NcpBase::SendPropertyUpdate(uint8_t header, uint8_t command, spinel_prop_key_t key, Message &aMessage)
{
    ThreadError errorCode = kThreadError_None;
    Message    *message = &aMessage;

    SuccessOrExit(errorCode = OutboundFrameBegin());
    SuccessOrExit(errorCode = OutboundFrameFeedPacked("Cii", header, command, key));
    SuccessOrExit(errorCode = OutboundFrameFeedMessage(*message));

    // Set the message pointer to NULL, to indicate that it does not need to be freed at the exit.
    // The message is now owned by the OutboundFrame and will be freed when the frame is either successfully sent and
    // then removed, or if the frame gets discarded.
    message = NULL;

    SuccessOrExit(errorCode = OutboundFrameSend());

exit:

    if (message != NULL)
    {
        Message::Free(*message);
    }

    return errorCode;
}

ThreadError NcpBase::OutboundFrameFeedVPacked(const char *pack_format, va_list args)
{
    uint8_t buf[64];
    ThreadError errorCode = kThreadError_NoBufs;
    spinel_ssize_t packed_len;

    packed_len = spinel_datatype_vpack(buf, sizeof(buf), pack_format, args);

    if ((packed_len > 0) && (packed_len <= static_cast<spinel_ssize_t>(sizeof(buf))))
    {
        errorCode = OutboundFrameFeedData(buf, static_cast<uint16_t>(packed_len));
    }

    return errorCode;
}

ThreadError NcpBase::OutboundFrameFeedPacked(const char *pack_format, ...)
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

ThreadError NcpBase::CommandHandler_NOOP(uint8_t header, unsigned int command, const uint8_t *arg_ptr, uint16_t arg_len)
{
    (void)command;
    (void)arg_ptr;
    (void)arg_len;

    return SendLastStatus(header, SPINEL_STATUS_OK);
}

ThreadError NcpBase::CommandHandler_RESET(uint8_t header, unsigned int command, const uint8_t *arg_ptr,
                                          uint16_t arg_len)
{
    // We aren't using any of the arguments to this function.
    (void)header;
    (void)command;
    (void)arg_ptr;
    (void)arg_len;

    // Signal a platform reset. If implemented, this function
    // shouldn't return.
    otPlatReset();

    // We only get to this point if the
    // platform doesn't support resetting.
    // In such a case we fake it.

    mChangedFlags |= NCP_PLAT_RESET_REASON;
    otDisable();
    mUpdateChangedPropsTask.Post();

    return SendLastStatus(SPINEL_HEADER_FLAG | SPINEL_HEADER_IID_0, SPINEL_STATUS_RESET_SOFTWARE);
}

ThreadError NcpBase::CommandHandler_PROP_VALUE_GET(uint8_t header, unsigned int command, const uint8_t *arg_ptr,
                                                   uint16_t arg_len)
{
    unsigned int propKey = 0;
    spinel_ssize_t parsedLength;
    ThreadError errorCode = kThreadError_None;

    parsedLength = spinel_datatype_unpack(arg_ptr, arg_len, "i", &propKey);

    if (parsedLength > 0)
    {
        errorCode = HandleCommandPropertyGet(header, static_cast<spinel_prop_key_t>(propKey));
    }
    else
    {
        errorCode = SendLastStatus(header, SPINEL_STATUS_PARSE_ERROR);
    }

    (void)command;

    return errorCode;
}

ThreadError NcpBase::CommandHandler_PROP_VALUE_SET(uint8_t header, unsigned int command, const uint8_t *arg_ptr,
                                                   uint16_t arg_len)
{
    unsigned int propKey = 0;
    spinel_ssize_t parsedLength;
    const uint8_t *value_ptr;
    unsigned int value_len;
    ThreadError errorCode = kThreadError_None;

    parsedLength = spinel_datatype_unpack(arg_ptr, arg_len, "iD", &propKey, &value_ptr, &value_len);

    if (parsedLength == arg_len)
    {
        errorCode = HandleCommandPropertySet(header, static_cast<spinel_prop_key_t>(propKey), value_ptr,
                                             static_cast<uint16_t>(value_len));
    }
    else
    {
        errorCode = SendLastStatus(header, SPINEL_STATUS_PARSE_ERROR);
    }

    (void)command;

    return errorCode;
}

ThreadError NcpBase::CommandHandler_PROP_VALUE_INSERT(uint8_t header, unsigned int command, const uint8_t *arg_ptr,
                                                      uint16_t arg_len)
{
    unsigned int propKey = 0;
    spinel_ssize_t parsedLength;
    const uint8_t *value_ptr;
    unsigned int value_len;
    ThreadError errorCode = kThreadError_None;

    parsedLength = spinel_datatype_unpack(arg_ptr, arg_len, "iD", &propKey, &value_ptr, &value_len);

    if (parsedLength == arg_len)
    {
        errorCode = HandleCommandPropertyInsert(header, static_cast<spinel_prop_key_t>(propKey), value_ptr,
                                                static_cast<uint16_t>(value_len));
    }
    else
    {
        errorCode = SendLastStatus(header, SPINEL_STATUS_PARSE_ERROR);
    }

    (void)command;

    return errorCode;
}

ThreadError NcpBase::CommandHandler_PROP_VALUE_REMOVE(uint8_t header, unsigned int command, const uint8_t *arg_ptr,
                                                      uint16_t arg_len)
{
    unsigned int propKey = 0;
    spinel_ssize_t parsedLength;
    const uint8_t *value_ptr;
    unsigned int value_len;
    ThreadError errorCode = kThreadError_None;

    parsedLength = spinel_datatype_unpack(arg_ptr, arg_len, "iD", &propKey, &value_ptr, &value_len);

    if (parsedLength == arg_len)
    {
        errorCode = HandleCommandPropertyRemove(header, static_cast<spinel_prop_key_t>(propKey), value_ptr,
                                                static_cast<uint16_t>(value_len));
    }
    else
    {
        errorCode = SendLastStatus(header, SPINEL_STATUS_PARSE_ERROR);
    }

    (void)command;

    return errorCode;
}


// ----------------------------------------------------------------------------
// MARK: Individual Property Getters
// ----------------------------------------------------------------------------


ThreadError NcpBase::GetPropertyHandler_LAST_STATUS(uint8_t header, spinel_prop_key_t key)
{
    return SendPropertyUpdate(header, SPINEL_CMD_PROP_VALUE_IS, key, SPINEL_DATATYPE_UINT_PACKED_S, mLastStatus);
}

ThreadError NcpBase::GetPropertyHandler_PROTOCOL_VERSION(uint8_t header, spinel_prop_key_t key)
{
    return SendPropertyUpdate(
               header,
               SPINEL_CMD_PROP_VALUE_IS,
               key,
               SPINEL_DATATYPE_UINT_PACKED_S SPINEL_DATATYPE_UINT_PACKED_S,
               SPINEL_PROTOCOL_VERSION_THREAD_MAJOR,
               SPINEL_PROTOCOL_VERSION_THREAD_MINOR
           );
}

ThreadError NcpBase::GetPropertyHandler_INTERFACE_TYPE(uint8_t header, spinel_prop_key_t key)
{
    return SendPropertyUpdate(
               header,
               SPINEL_CMD_PROP_VALUE_IS,
               key,
               SPINEL_DATATYPE_UINT_PACKED_S,
               SPINEL_PROTOCOL_TYPE_THREAD
           );
}

ThreadError NcpBase::GetPropertyHandler_VENDOR_ID(uint8_t header, spinel_prop_key_t key)
{
    return SendPropertyUpdate(
               header,
               SPINEL_CMD_PROP_VALUE_IS,
               key,
               SPINEL_DATATYPE_UINT_PACKED_S,
               0 // Vendor ID. Zero for unknown.
           );
}

ThreadError NcpBase::GetPropertyHandler_CAPS(uint8_t header, spinel_prop_key_t key)
{
    ThreadError errorCode = kThreadError_None;

    SuccessOrExit(errorCode = OutboundFrameBegin());

    SuccessOrExit(errorCode = OutboundFrameFeedPacked("Cii", header, SPINEL_CMD_PROP_VALUE_IS, key));

    // Begin adding capabilities //////////////////////////////////////////////

    SuccessOrExit(errorCode = OutboundFrameFeedPacked(SPINEL_DATATYPE_UINT_PACKED_S, SPINEL_CAP_NET_THREAD_1_0));

    SuccessOrExit(errorCode = OutboundFrameFeedPacked(SPINEL_DATATYPE_UINT_PACKED_S, SPINEL_CAP_COUNTERS));

    SuccessOrExit(errorCode = OutboundFrameFeedPacked(SPINEL_DATATYPE_UINT_PACKED_S, SPINEL_CAP_MAC_WHITELIST));

    // TODO: Somehow get the following capability from the radio.
    SuccessOrExit(errorCode = OutboundFrameFeedPacked(SPINEL_DATATYPE_UINT_PACKED_S,
                                                      SPINEL_CAP_802_15_4_2450MHZ_OQPSK));

#if OPENTHREAD_CONFIG_MAX_CHILDREN > 0
    SuccessOrExit(errorCode = OutboundFrameFeedPacked(SPINEL_DATATYPE_UINT_PACKED_S, SPINEL_CAP_ROLE_ROUTER));
#endif

    // End adding capabilities /////////////////////////////////////////////////

    SuccessOrExit(errorCode = OutboundFrameSend());

exit:
    return errorCode;
}

ThreadError NcpBase::GetPropertyHandler_NCP_VERSION(uint8_t header, spinel_prop_key_t key)
{
    return SendPropertyUpdate(
               header,
               SPINEL_CMD_PROP_VALUE_IS,
               key,
               SPINEL_DATATYPE_UTF8_S,
               otGetVersionString()
           );
}

ThreadError NcpBase::GetPropertyHandler_INTERFACE_COUNT(uint8_t header, spinel_prop_key_t key)
{
    return SendPropertyUpdate(
               header,
               SPINEL_CMD_PROP_VALUE_IS,
               key,
               SPINEL_DATATYPE_UINT8_S,
               1 // Only one interface for now
           );
}

ThreadError NcpBase::GetPropertyHandler_POWER_STATE(uint8_t header, spinel_prop_key_t key)
{
    // Always online at the moment
    return SendPropertyUpdate(
               header,
               SPINEL_CMD_PROP_VALUE_IS,
               key,
               SPINEL_DATATYPE_UINT8_S,
               SPINEL_POWER_STATE_ONLINE
           );
}

ThreadError NcpBase::GetPropertyHandler_HWADDR(uint8_t header, spinel_prop_key_t key)
{
    return SendPropertyUpdate(
               header,
               SPINEL_CMD_PROP_VALUE_IS,
               key,
               SPINEL_DATATYPE_EUI64_S,
               otGetExtendedAddress()
           );
}

ThreadError NcpBase::GetPropertyHandler_LOCK(uint8_t header, spinel_prop_key_t key)
{
    // TODO: Implement property lock (Needs API!)
    (void)key;

    return SendLastStatus(header, SPINEL_STATUS_UNIMPLEMENTED);
}

ThreadError NcpBase::GetPropertyHandler_PHY_ENABLED(uint8_t header, spinel_prop_key_t key)
{
    // TODO: Implement PHY_ENBLED (Needs API!)
    (void)key;

    return SendLastStatus(header, SPINEL_STATUS_UNIMPLEMENTED);
}

ThreadError NcpBase::GetPropertyHandler_PHY_FREQ(uint8_t header, spinel_prop_key_t key)
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

    return SendPropertyUpdate(
               header,
               SPINEL_CMD_PROP_VALUE_IS,
               key,
               SPINEL_DATATYPE_UINT32_S,
               freq_khz
           );
}

ThreadError NcpBase::GetPropertyHandler_PHY_CHAN_SUPPORTED(uint8_t header, spinel_prop_key_t key)
{
    return GetPropertyHandler_ChannelMaskHelper(header, key, mSupportedChannelMask);
}

ThreadError NcpBase::GetPropertyHandler_PHY_CHAN(uint8_t header, spinel_prop_key_t key)
{
    return SendPropertyUpdate(
               header,
               SPINEL_CMD_PROP_VALUE_IS,
               key,
               SPINEL_DATATYPE_UINT8_S,
               otGetChannel()
           );
}

ThreadError NcpBase::GetPropertyHandler_PHY_RSSI(uint8_t header, spinel_prop_key_t key)
{
    return SendPropertyUpdate(
               header,
               SPINEL_CMD_PROP_VALUE_IS,
               key,
               SPINEL_DATATYPE_INT8_S,
               otPlatRadioGetRssi()
           );
}

ThreadError NcpBase::GetPropertyHandler_MAC_SCAN_STATE(uint8_t header, spinel_prop_key_t key)
{
    ThreadError errorCode = kThreadError_None;

    if (otIsActiveScanInProgress())
    {
        errorCode = SendPropertyUpdate(
                        header,
                        SPINEL_CMD_PROP_VALUE_IS,
                        key,
                        SPINEL_DATATYPE_UINT8_S,
                        SPINEL_SCAN_STATE_BEACON
                    );
    }
    else
    {
        errorCode = SendPropertyUpdate(
                        header,
                        SPINEL_CMD_PROP_VALUE_IS,
                        key,
                        SPINEL_DATATYPE_UINT8_S,
                        SPINEL_SCAN_STATE_IDLE
                    );
    }

    return errorCode;
}

ThreadError NcpBase::GetPropertyHandler_MAC_SCAN_PERIOD(uint8_t header, spinel_prop_key_t key)
{
    return SendPropertyUpdate(
               header,
               SPINEL_CMD_PROP_VALUE_IS,
               key,
               SPINEL_DATATYPE_UINT16_S,
               mScanPeriod
           );
}

ThreadError NcpBase::GetPropertyHandler_ChannelMaskHelper(uint8_t header, spinel_prop_key_t key, uint32_t channel_mask)
{
    ThreadError errorCode = kThreadError_None;

    SuccessOrExit(errorCode = OutboundFrameBegin());

    SuccessOrExit(errorCode = OutboundFrameFeedPacked("Cii", header, SPINEL_CMD_PROP_VALUE_IS, key));

    for (int i = 0; i < 32; i++)
    {
        if (0 != (channel_mask & (1 << i)))
        {
            SuccessOrExit(errorCode = OutboundFrameFeedPacked(SPINEL_DATATYPE_UINT8_S, i));
        }
    }

    SuccessOrExit(errorCode = OutboundFrameSend());

exit:
    return errorCode;
}

ThreadError NcpBase::GetPropertyHandler_MAC_SCAN_MASK(uint8_t header, spinel_prop_key_t key)
{
    return GetPropertyHandler_ChannelMaskHelper(header, key, mChannelMask);
}

ThreadError NcpBase::GetPropertyHandler_MAC_15_4_PANID(uint8_t header, spinel_prop_key_t key)
{
    return SendPropertyUpdate(
               header,
               SPINEL_CMD_PROP_VALUE_IS,
               key,
               SPINEL_DATATYPE_UINT16_S,
               otGetPanId()
           );
}

ThreadError NcpBase::GetPropertyHandler_MAC_FILTER_MODE(uint8_t header, spinel_prop_key_t key)
{
    return SendPropertyUpdate(
               header,
               SPINEL_CMD_PROP_VALUE_IS,
               key,
               SPINEL_DATATYPE_INT8_S,
               otPlatRadioGetPromiscuous()
               ? SPINEL_MAC_FILTER_MODE_15_4_PROMISCUOUS
               : SPINEL_MAC_FILTER_MODE_NORMAL
           );
}

ThreadError NcpBase::GetPropertyHandler_MAC_15_4_LADDR(uint8_t header, spinel_prop_key_t key)
{
    return SendPropertyUpdate(
               header,
               SPINEL_CMD_PROP_VALUE_IS,
               key,
               SPINEL_DATATYPE_EUI64_S,
               otGetExtendedAddress()
           );
}

ThreadError NcpBase::GetPropertyHandler_MAC_15_4_SADDR(uint8_t header, spinel_prop_key_t key)
{
    return SendPropertyUpdate(
               header,
               SPINEL_CMD_PROP_VALUE_IS,
               key,
               SPINEL_DATATYPE_UINT16_S,
               otGetShortAddress()
           );
}

ThreadError NcpBase::GetPropertyHandler_NET_IF_UP(uint8_t header, spinel_prop_key_t key)
{
    return SendPropertyUpdate(
               header,
               SPINEL_CMD_PROP_VALUE_IS,
               key,
               SPINEL_DATATYPE_BOOL_S,
               otIsInterfaceUp()
           );
}

ThreadError NcpBase::GetPropertyHandler_NET_STACK_UP(uint8_t header, spinel_prop_key_t key)
{
    return SendPropertyUpdate(
               header,
               SPINEL_CMD_PROP_VALUE_IS,
               key,
               SPINEL_DATATYPE_BOOL_S,
               otGetDeviceRole() != kDeviceRoleDisabled
           );
}

ThreadError NcpBase::GetPropertyHandler_NET_ROLE(uint8_t header, spinel_prop_key_t key)
{
    spinel_net_role_t role(SPINEL_NET_ROLE_DETACHED);

    switch (otGetDeviceRole())
    {
    case kDeviceRoleDisabled:
    case kDeviceRoleDetached:
        role = SPINEL_NET_ROLE_DETACHED;
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

    return SendPropertyUpdate(
               header,
               SPINEL_CMD_PROP_VALUE_IS,
               key,
               SPINEL_DATATYPE_UINT8_S,
               role
           );
}

ThreadError NcpBase::GetPropertyHandler_NET_NETWORK_NAME(uint8_t header, spinel_prop_key_t key)
{
    return SendPropertyUpdate(
               header,
               SPINEL_CMD_PROP_VALUE_IS,
               key,
               SPINEL_DATATYPE_UTF8_S,
               otGetNetworkName()
           );
}

ThreadError NcpBase::GetPropertyHandler_NET_XPANID(uint8_t header, spinel_prop_key_t key)
{
    return SendPropertyUpdate(
               header,
               SPINEL_CMD_PROP_VALUE_IS,
               key,
               SPINEL_DATATYPE_DATA_S,
               otGetExtendedPanId(),
               sizeof(spinel_net_xpanid_t)
           );
}

ThreadError NcpBase::GetPropertyHandler_NET_MASTER_KEY(uint8_t header, spinel_prop_key_t key)
{
    const uint8_t *ptr(NULL);
    uint8_t len(0);

    ptr = otGetMasterKey(&len);

    return SendPropertyUpdate(
               header,
               SPINEL_CMD_PROP_VALUE_IS,
               key,
               SPINEL_DATATYPE_DATA_S,
               ptr,
               len
           );
}

ThreadError NcpBase::GetPropertyHandler_NET_KEY_SEQUENCE(uint8_t header, spinel_prop_key_t key)
{
    return SendPropertyUpdate(
               header,
               SPINEL_CMD_PROP_VALUE_IS,
               key,
               SPINEL_DATATYPE_UINT32_S,
               otGetKeySequenceCounter()
           );
}

ThreadError NcpBase::GetPropertyHandler_NET_PARTITION_ID(uint8_t header, spinel_prop_key_t key)
{
    return SendPropertyUpdate(
               header,
               SPINEL_CMD_PROP_VALUE_IS,
               key,
               SPINEL_DATATYPE_UINT32_S,
               otGetPartitionId()
           );
}

ThreadError NcpBase::GetPropertyHandler_THREAD_NETWORK_DATA_VERSION(uint8_t header, spinel_prop_key_t key)
{
    return SendPropertyUpdate(
               header,
               SPINEL_CMD_PROP_VALUE_IS,
               key,
               SPINEL_DATATYPE_UINT8_S,
               otGetNetworkDataVersion()
           );
}

ThreadError NcpBase::GetPropertyHandler_THREAD_STABLE_NETWORK_DATA_VERSION(uint8_t header, spinel_prop_key_t key)
{
    return SendPropertyUpdate(
               header,
               SPINEL_CMD_PROP_VALUE_IS,
               key,
               SPINEL_DATATYPE_UINT8_S,
               otGetStableNetworkDataVersion()
           );
}

ThreadError NcpBase::GetPropertyHandler_THREAD_NETWORK_DATA(uint8_t header, spinel_prop_key_t key)
{
    ThreadError errorCode = kThreadError_None;
    uint8_t network_data[255];
    uint8_t network_data_len = 255;

    SuccessOrExit(errorCode = OutboundFrameBegin());
    SuccessOrExit(errorCode = OutboundFrameFeedPacked("Cii", header, SPINEL_CMD_PROP_VALUE_IS, key));
    otGetNetworkDataLocal(
        false, // Stable?
        network_data,
        &network_data_len
    );
    SuccessOrExit(errorCode = OutboundFrameFeedData(network_data, network_data_len));
    SuccessOrExit(errorCode = OutboundFrameSend());

exit:
    return errorCode;
}

ThreadError NcpBase::GetPropertyHandler_THREAD_STABLE_NETWORK_DATA(uint8_t header, spinel_prop_key_t key)
{
    ThreadError errorCode = kThreadError_None;
    uint8_t network_data[255];
    uint8_t network_data_len = 255;


    SuccessOrExit(errorCode = OutboundFrameBegin());

    SuccessOrExit(errorCode = OutboundFrameFeedPacked("Cii", header, SPINEL_CMD_PROP_VALUE_IS, key));
    otGetNetworkDataLocal(
        true, // Stable?
        network_data,
        &network_data_len
    );

    SuccessOrExit(errorCode = OutboundFrameFeedData(network_data, network_data_len));
    SuccessOrExit(errorCode = OutboundFrameSend());

exit:
    return errorCode;
}

ThreadError NcpBase::GetPropertyHandler_THREAD_LEADER_RID(uint8_t header, spinel_prop_key_t key)
{
    return SendPropertyUpdate(
               header,
               SPINEL_CMD_PROP_VALUE_IS,
               key,
               SPINEL_DATATYPE_UINT8_S,
               otGetLeaderRouterId()
           );
}

ThreadError NcpBase::GetPropertyHandler_THREAD_LOCAL_LEADER_WEIGHT(uint8_t header, spinel_prop_key_t key)
{
    return SendPropertyUpdate(
               header,
               SPINEL_CMD_PROP_VALUE_IS,
               key,
               SPINEL_DATATYPE_UINT8_S,
               otGetLocalLeaderWeight()
           );
}

ThreadError NcpBase::GetPropertyHandler_THREAD_LEADER_WEIGHT(uint8_t header, spinel_prop_key_t key)
{
    return SendPropertyUpdate(
               header,
               SPINEL_CMD_PROP_VALUE_IS,
               key,
               SPINEL_DATATYPE_UINT8_S,
               otGetLeaderWeight()
           );
}

ThreadError NcpBase::GetPropertyHandler_THREAD_LEADER_ADDR(uint8_t header, spinel_prop_key_t key)
{
    ThreadError errorCode = kThreadError_None;
    otIp6Address address;

    errorCode = otGetLeaderRloc(&address);

    if (errorCode == kThreadError_None)
    {
        errorCode = SendPropertyUpdate(
                        header,
                        SPINEL_CMD_PROP_VALUE_IS,
                        key,
                        SPINEL_DATATYPE_IPv6ADDR_S,
                        &address
                    );
    }
    else
    {
        errorCode = SendLastStatus(header, ThreadErrorToSpinelStatus(errorCode));
    }

    return errorCode;
}

ThreadError NcpBase::GetPropertyHandler_THREAD_ASSISTING_PORTS(uint8_t header, spinel_prop_key_t key)
{
    ThreadError errorCode = kThreadError_None;
    uint8_t num_entries = 0;
    const uint16_t *ports = otGetUnsecurePorts(&num_entries);

    SuccessOrExit(errorCode = OutboundFrameBegin());

    SuccessOrExit(errorCode = OutboundFrameFeedPacked("Cii", header, SPINEL_CMD_PROP_VALUE_IS, key));

    for (; num_entries != 0; ports++, num_entries--)
    {
        SuccessOrExit(errorCode = OutboundFrameFeedPacked("S", *ports));
    }

    SuccessOrExit(errorCode = OutboundFrameSend());

exit:
    return errorCode;
}


ThreadError NcpBase::GetPropertyHandler_THREAD_ALLOW_LOCAL_NET_DATA_CHANGE(uint8_t header, spinel_prop_key_t key)
{
    return SendPropertyUpdate(
        header,
        SPINEL_CMD_PROP_VALUE_IS,
        key,
        SPINEL_DATATYPE_BOOL_S,
        mAllowLocalNetworkDataChange
    );
}


ThreadError NcpBase::GetPropertyHandler_IPV6_ML_PREFIX(uint8_t header, spinel_prop_key_t key)
{
    ThreadError errorCode = kThreadError_None;
    const uint8_t *ml_prefix = otGetMeshLocalPrefix();

    if (ml_prefix)
    {
        otIp6Address addr;

        memcpy(addr.mFields.m8, ml_prefix, 8);

        // Zero out the last 8 bytes.
        memset(addr.mFields.m8 + 8, 0, 8);

        errorCode = SendPropertyUpdate(
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
        errorCode = SendPropertyUpdate(
                        header,
                        SPINEL_CMD_PROP_VALUE_IS,
                        key,
                        SPINEL_DATATYPE_VOID_S
                    );
    }

    return errorCode;
}

ThreadError NcpBase::GetPropertyHandler_IPV6_ML_ADDR(uint8_t header, spinel_prop_key_t key)
{
    ThreadError errorCode = kThreadError_None;
    const otIp6Address *ml64 = otGetMeshLocalEid();

    if (ml64)
    {
        errorCode = SendPropertyUpdate(
                        header,
                        SPINEL_CMD_PROP_VALUE_IS,
                        key,
                        SPINEL_DATATYPE_IPv6ADDR_S,
                        ml64
                    );
    }
    else
    {
        errorCode = SendPropertyUpdate(
                        header,
                        SPINEL_CMD_PROP_VALUE_IS,
                        key,
                        SPINEL_DATATYPE_VOID_S
                    );
    }

    return errorCode;
}

ThreadError NcpBase::GetPropertyHandler_IPV6_LL_ADDR(uint8_t header, spinel_prop_key_t key)
{
    // TODO!
    (void)key;

    return SendLastStatus(header, SPINEL_STATUS_UNIMPLEMENTED);
}

ThreadError NcpBase::GetPropertyHandler_IPV6_ADDRESS_TABLE(uint8_t header, spinel_prop_key_t key)
{
    ThreadError errorCode = kThreadError_None;

    SuccessOrExit(errorCode = OutboundFrameBegin());


    SuccessOrExit(errorCode = OutboundFrameFeedPacked("Cii", header, SPINEL_CMD_PROP_VALUE_IS, key));

    for (const otNetifAddress *address = otGetUnicastAddresses(); address; address = address->mNext)
    {

        SuccessOrExit(errorCode = OutboundFrameFeedPacked(
                                      "T(6CLL).",
                                      &address->mAddress,
                                      address->mPrefixLength,
                                      address->mPreferredLifetime,
                                      address->mValidLifetime
                                  ));
    }

    SuccessOrExit(errorCode = OutboundFrameSend());

exit:
    return errorCode;
}

ThreadError NcpBase::GetPropertyHandler_IPV6_ROUTE_TABLE(uint8_t header, spinel_prop_key_t key)
{
    // TODO: Implement get route table
    (void)key;

    return SendLastStatus(header, SPINEL_STATUS_UNIMPLEMENTED);
}

ThreadError NcpBase::GetPropertyHandler_THREAD_LOCAL_ROUTES(uint8_t header, spinel_prop_key_t key)
{
    // TODO: Implement get external route table
    (void)key;

    return SendLastStatus(header, SPINEL_STATUS_UNIMPLEMENTED);
}

ThreadError NcpBase::GetPropertyHandler_STREAM_NET(uint8_t header, spinel_prop_key_t key)
{
    // TODO: Implement explicit data poll.
    (void)key;

    return SendLastStatus(header, SPINEL_STATUS_UNIMPLEMENTED);
}

ThreadError NcpBase::GetPropertyHandler_MAC_CNTR(uint8_t header, spinel_prop_key_t key)
{
    uint32_t value;
    const otMacCounters *macCounters;
    ThreadError errorCode = kThreadError_None;

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
        errorCode = SendLastStatus(header, SPINEL_STATUS_INTERNAL_ERROR);
        goto bail;
        break;
    }

    errorCode = SendPropertyUpdate(
                    header,
                    SPINEL_CMD_PROP_VALUE_IS,
                    key,
                    SPINEL_DATATYPE_UINT32_S,
                    value
                );

bail:
    return errorCode;
}

ThreadError NcpBase::GetPropertyHandler_NCP_CNTR(uint8_t header, spinel_prop_key_t key)
{
    uint32_t value;
    ThreadError errorCode = kThreadError_None;

    switch (key)
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

    case SPINEL_PROP_CNTR_RX_SPINEL_ERR:
        value = mFramingErrorCounter;
        break;

    default:
        errorCode = SendLastStatus(header, SPINEL_STATUS_INTERNAL_ERROR);
        goto bail;
        break;
    }

    errorCode = SendPropertyUpdate(
                    header,
                    SPINEL_CMD_PROP_VALUE_IS,
                    key,
                    SPINEL_DATATYPE_UINT32_S,
                    value
                );

bail:
    return errorCode;
}

ThreadError NcpBase::GetPropertyHandler_MAC_WHITELIST(uint8_t header, spinel_prop_key_t key)
{
    otMacWhitelistEntry entry;
    ThreadError errorCode = kThreadError_None;

    SuccessOrExit(errorCode = OutboundFrameBegin());

    SuccessOrExit(errorCode = OutboundFrameFeedPacked("Cii", header, SPINEL_CMD_PROP_VALUE_IS, key));

    for (uint8_t i = 0; (i != 255) && (errorCode == kThreadError_None); i++)
    {
        errorCode = otGetMacWhitelistEntry(i, &entry);

        if (errorCode != kThreadError_None)
        {
            break;
        }

        if (entry.mValid)
        {
            if (!entry.mFixedRssi)
            {
                entry.mRssi = RSSI_OVERRIDE_DISABLED;
            }

            SuccessOrExit(errorCode = OutboundFrameFeedPacked("T(Ec).", entry.mExtAddress.m8, entry.mRssi));
        }
    }

    SuccessOrExit(errorCode = OutboundFrameSend());

exit:
    return errorCode;
}

ThreadError NcpBase::GetPropertyHandler_MAC_WHITELIST_ENABLED(uint8_t header, spinel_prop_key_t key)
{
    return SendPropertyUpdate(
               header,
               SPINEL_CMD_PROP_VALUE_IS,
               key,
               SPINEL_DATATYPE_BOOL_S,
               otIsMacWhitelistEnabled()
           );
}

ThreadError NcpBase::GetPropertyHandler_THREAD_MODE(uint8_t header, spinel_prop_key_t key)
{
    uint8_t numeric_mode(0);
    otLinkModeConfig mode_config(otGetLinkMode());

    if (mode_config.mRxOnWhenIdle)
    {
        numeric_mode |= kThreadModeTLV_Receiver;
    }

    if (mode_config.mSecureDataRequests)
    {
        numeric_mode |= kThreadModeTLV_Secure;
    }

    if (mode_config.mDeviceType)
    {
        numeric_mode |= kThreadModeTLV_DeviceType;
    }

    if (mode_config.mNetworkData)
    {
        numeric_mode |= kThreadModeTLV_NetworkData;
    }

    return SendPropertyUpdate(
               header,
               SPINEL_CMD_PROP_VALUE_IS,
               key,
               SPINEL_DATATYPE_UINT8_S,
               numeric_mode
           );
}

ThreadError NcpBase::GetPropertyHandler_THREAD_CHILD_TIMEOUT(uint8_t header, spinel_prop_key_t key)
{
    return SendPropertyUpdate(
               header,
               SPINEL_CMD_PROP_VALUE_IS,
               key,
               SPINEL_DATATYPE_UINT32_S,
               otGetChildTimeout()
           );
}

ThreadError NcpBase::GetPropertyHandler_THREAD_RLOC16(uint8_t header, spinel_prop_key_t key)
{
    return SendPropertyUpdate(
               header,
               SPINEL_CMD_PROP_VALUE_IS,
               key,
               SPINEL_DATATYPE_UINT16_S,
               otGetRloc16()
           );
}

ThreadError NcpBase::GetPropertyHandler_THREAD_ROUTER_UPGRADE_THRESHOLD(uint8_t header, spinel_prop_key_t key)
{
    return SendPropertyUpdate(
               header,
               SPINEL_CMD_PROP_VALUE_IS,
               key,
               SPINEL_DATATYPE_UINT8_S,
               otGetRouterUpgradeThreshold()
           );
}

ThreadError NcpBase::GetPropertyHandler_THREAD_CONTEXT_REUSE_DELAY(uint8_t header, spinel_prop_key_t key)
{
    return SendPropertyUpdate(
               header,
               SPINEL_CMD_PROP_VALUE_IS,
               key,
               SPINEL_DATATYPE_UINT32_S,
               otGetContextIdReuseDelay()
           );
}

ThreadError NcpBase::GetPropertyHandler_THREAD_NETWORK_ID_TIMEOUT(uint8_t header, spinel_prop_key_t key)
{
    return SendPropertyUpdate(
               header,
               SPINEL_CMD_PROP_VALUE_IS,
               key,
               SPINEL_DATATYPE_UINT8_S,
               otGetNetworkIdTimeout()
           );
}


// ----------------------------------------------------------------------------
// MARK: Individual Property Setters
// ----------------------------------------------------------------------------

ThreadError NcpBase::SetPropertyHandler_POWER_STATE(uint8_t header, spinel_prop_key_t key, const uint8_t *value_ptr,
                                                    uint16_t value_len)
{
    // TODO: Implement POWER_STATE
    (void)key;
    (void)value_ptr;
    (void)value_len;

    return SendLastStatus(header, SPINEL_STATUS_UNIMPLEMENTED);
}

ThreadError NcpBase::SetPropertyHandler_PHY_ENABLED(uint8_t header, spinel_prop_key_t key, const uint8_t *value_ptr,
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
        errorCode = HandleCommandPropertyGet(header, key);
    }
    else
    {
        errorCode = SendLastStatus(header, ThreadErrorToSpinelStatus(errorCode));
    }

    return errorCode;
}

ThreadError NcpBase::SetPropertyHandler_PHY_TX_POWER(uint8_t header, spinel_prop_key_t key, const uint8_t *value_ptr,
                                                     uint16_t value_len)
{
    // TODO: Implement PHY_TX_POWER
    (void)key;
    (void)value_ptr;
    (void)value_len;

    return SendLastStatus(header, SPINEL_STATUS_UNIMPLEMENTED);
}

ThreadError NcpBase::SetPropertyHandler_PHY_CHAN(uint8_t header, spinel_prop_key_t key, const uint8_t *value_ptr,
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
            errorCode = HandleCommandPropertyGet(header, key);
        }
        else
        {
            errorCode = SendLastStatus(header, ThreadErrorToSpinelStatus(errorCode));
        }
    }
    else
    {
        errorCode = SendLastStatus(header, SPINEL_STATUS_PARSE_ERROR);
    }

    return errorCode;
}

ThreadError NcpBase::SetPropertyHandler_MAC_FILTER_MODE(uint8_t header, spinel_prop_key_t key, const uint8_t *value_ptr,
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
            errorCode = HandleCommandPropertyGet(header, key);
        }
        else
        {
            errorCode = SendLastStatus(header, ThreadErrorToSpinelStatus(errorCode));
        }
    }
    else
    {
        errorCode = SendLastStatus(header, SPINEL_STATUS_PARSE_ERROR);
    }

    return errorCode;
}

ThreadError NcpBase::SetPropertyHandler_MAC_SCAN_MASK(uint8_t header, spinel_prop_key_t key, const uint8_t *value_ptr,
                                                      uint16_t value_len)
{
    ThreadError errorCode = kThreadError_None;
    uint32_t new_mask(0);

    for (; value_len != 0; value_len--, value_ptr++)
    {
        if ((value_ptr[0] > 31)
            || (mSupportedChannelMask & (1 << value_ptr[0])) == 0
           )
        {
            errorCode = kThreadError_InvalidArgs;
            break;
        }

        new_mask |= (1 << value_ptr[0]);
    }

    if (errorCode == kThreadError_None)
    {
        mChannelMask = new_mask;
        errorCode = HandleCommandPropertyGet(header, key);
    }
    else
    {
        errorCode = SendLastStatus(header, ThreadErrorToSpinelStatus(errorCode));
    }

    return errorCode;
}

ThreadError NcpBase::SetPropertyHandler_MAC_SCAN_PERIOD(uint8_t header, spinel_prop_key_t key, const uint8_t *value_ptr,
                                                        uint16_t value_len)
{
    uint16_t tmp(mScanPeriod);
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
        mScanPeriod = tmp;
        errorCode = HandleCommandPropertyGet(header, key);
    }
    else
    {
        errorCode = SendLastStatus(header, SPINEL_STATUS_PARSE_ERROR);
    }

    return errorCode;
}

ThreadError NcpBase::SetPropertyHandler_MAC_SCAN_STATE(uint8_t header, spinel_prop_key_t key, const uint8_t *value_ptr,
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
            mShouldSignalEndOfScan = false;
            errorCode = otActiveScan(
                            mChannelMask,
                            mScanPeriod,
                            &HandleActiveScanResult_Jump,
                            this
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
            errorCode = HandleCommandPropertyGet(header, key);
        }
        else
        {
            errorCode = SendLastStatus(header, ThreadErrorToSpinelStatus(errorCode));
        }
    }
    else
    {
        errorCode = SendLastStatus(header, SPINEL_STATUS_PARSE_ERROR);
    }

    return errorCode;
}

ThreadError NcpBase::SetPropertyHandler_MAC_15_4_PANID(uint8_t header, spinel_prop_key_t key, const uint8_t *value_ptr,
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
            errorCode = HandleCommandPropertyGet(header, key);
        }
        else
        {
            errorCode = SendLastStatus(header, ThreadErrorToSpinelStatus(errorCode));
        }
    }
    else
    {
        errorCode = SendLastStatus(header, SPINEL_STATUS_PARSE_ERROR);
    }

    return errorCode;
}

ThreadError NcpBase::SetPropertyHandler_NET_IF_UP(uint8_t header, spinel_prop_key_t key, const uint8_t *value_ptr,
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
            errorCode = otInterfaceDown();
        }
        else
        {
            errorCode = otInterfaceUp();
        }
    }
    else
    {
        errorCode = kThreadError_Parse;
    }

    if (errorCode == kThreadError_None)
    {
        errorCode = HandleCommandPropertyGet(header, key);
    }
    else
    {
        errorCode = SendLastStatus(header, ThreadErrorToSpinelStatus(errorCode));
    }

    return errorCode;
}

ThreadError NcpBase::SetPropertyHandler_NET_STACK_UP(uint8_t header, spinel_prop_key_t key, const uint8_t *value_ptr,
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
        // If the value has changed...
        if ((value != false) != (otGetDeviceRole() != kDeviceRoleDisabled))
        {
            if (value != false)
            {
                errorCode = otThreadStart();
            }
            else
            {
                errorCode = otThreadStop();
            }
        }
    }
    else
    {
        errorCode = kThreadError_Parse;
    }

    if (errorCode == kThreadError_None)
    {
        errorCode = HandleCommandPropertyGet(header, key);
    }
    else
    {
        errorCode = SendLastStatus(header, ThreadErrorToSpinelStatus(errorCode));
    }

    return errorCode;
}

ThreadError NcpBase::SetPropertyHandler_NET_ROLE(uint8_t header, spinel_prop_key_t key, const uint8_t *value_ptr,
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
        case SPINEL_NET_ROLE_DETACHED:
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
            errorCode = HandleCommandPropertyGet(header, key);
        }
        else
        {
            errorCode = SendLastStatus(header, ThreadErrorToSpinelStatus(errorCode));
        }
    }
    else
    {
        errorCode = SendLastStatus(header, SPINEL_STATUS_PARSE_ERROR);
    }

    return errorCode;
}

ThreadError NcpBase::SetPropertyHandler_NET_NETWORK_NAME(uint8_t header, spinel_prop_key_t key,
                                                         const uint8_t *value_ptr,
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
            errorCode = HandleCommandPropertyGet(header, key);
        }
        else
        {
            errorCode = SendLastStatus(header, ThreadErrorToSpinelStatus(errorCode));
        }
    }
    else
    {
        errorCode = SendLastStatus(header, SPINEL_STATUS_PARSE_ERROR);
    }

    return errorCode;
}

ThreadError NcpBase::SetPropertyHandler_NET_XPANID(uint8_t header, spinel_prop_key_t key, const uint8_t *value_ptr,
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

    if ((parsedLength > 0) && (len == sizeof(spinel_net_xpanid_t)))
    {
        otSetExtendedPanId(ptr);
        errorCode = HandleCommandPropertyGet(header, key);
    }
    else
    {
        errorCode = SendLastStatus(header, SPINEL_STATUS_PARSE_ERROR);
    }

    return errorCode;
}

ThreadError NcpBase::SetPropertyHandler_NET_MASTER_KEY(uint8_t header, spinel_prop_key_t key, const uint8_t *value_ptr,
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
            errorCode = HandleCommandPropertyGet(header, key);
        }
        else
        {
            errorCode = SendLastStatus(header, ThreadErrorToSpinelStatus(errorCode));
        }
    }
    else
    {
        errorCode = SendLastStatus(header, SPINEL_STATUS_PARSE_ERROR);
    }

    return errorCode;
}

ThreadError NcpBase::SetPropertyHandler_NET_KEY_SEQUENCE(uint8_t header, spinel_prop_key_t key,
                                                         const uint8_t *value_ptr,
                                                         uint16_t value_len)
{
    unsigned int i(0);
    spinel_ssize_t parsedLength;
    ThreadError errorCode = kThreadError_None;

    parsedLength = spinel_datatype_unpack(
                       value_ptr,
                       value_len,
                       SPINEL_DATATYPE_UINT32_S,
                       &i
                   );

    if (parsedLength > 0)
    {
        otSetKeySequenceCounter(i);
        errorCode = HandleCommandPropertyGet(header, key);
    }
    else
    {
        errorCode = SendLastStatus(header, SPINEL_STATUS_PARSE_ERROR);
    }

    return errorCode;
}

ThreadError NcpBase::SetPropertyHandler_THREAD_LOCAL_LEADER_WEIGHT(uint8_t header, spinel_prop_key_t key,
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
        errorCode = HandleCommandPropertyGet(header, key);
    }
    else
    {
        errorCode = SendLastStatus(header, ThreadErrorToSpinelStatus(errorCode));
    }

    return errorCode;
}



ThreadError NcpBase::SetPropertyHandler_STREAM_NET_INSECURE(uint8_t header, spinel_prop_key_t key,
                                                            const uint8_t *value_ptr, uint16_t value_len)
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

        errorCode = message->Append(frame_ptr, static_cast<uint16_t>(frame_len));
    }

    if (errorCode == kThreadError_None)
    {
        errorCode = otSendIp6Datagram(message);
    }
    else if (message)
    {
        Message::Free(*message);
    }

    if (errorCode == kThreadError_None)
    {
        mInboundInsecureIpFrameCounter++;

        if (SPINEL_HEADER_GET_TID(header) != 0)
        {
            // Only send a successful status update if
            // there was a transaction id in the header.
            errorCode = SendLastStatus(header, SPINEL_STATUS_OK);
        }
    }
    else
    {
        mDroppedInboundIpFrameCounter++;

        errorCode = SendLastStatus(header, ThreadErrorToSpinelStatus(errorCode));
    }

    (void)key;

    return errorCode;
}

ThreadError NcpBase::SetPropertyHandler_STREAM_NET(uint8_t header, spinel_prop_key_t key, const uint8_t *value_ptr,
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

        errorCode = message->Append(frame_ptr, static_cast<uint16_t>(frame_len));
    }

    if (errorCode == kThreadError_None)
    {
        errorCode = otSendIp6Datagram(message);
    }
    else if (message)
    {
        Message::Free(*message);
    }

    if (errorCode == kThreadError_None)
    {
        mInboundSecureIpFrameCounter++;

        if (SPINEL_HEADER_GET_TID(header) != 0)
        {
            // Only send a successful status update if
            // there was a transaction id in the header.
            errorCode = SendLastStatus(header, SPINEL_STATUS_OK);
        }
    }
    else
    {
        mDroppedInboundIpFrameCounter++;

        errorCode = SendLastStatus(header, ThreadErrorToSpinelStatus(errorCode));

    }

    (void)key;

    return errorCode;
}

ThreadError NcpBase::SetPropertyHandler_IPV6_ML_PREFIX(uint8_t header, spinel_prop_key_t key, const uint8_t *value_ptr,
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
        errorCode = HandleCommandPropertyGet(header, key);
    }
    else
    {
        errorCode = SendLastStatus(header, ThreadErrorToSpinelStatus(errorCode));
    }

    return errorCode;
}

ThreadError NcpBase::SetPropertyHandler_THREAD_ASSISTING_PORTS(uint8_t header, spinel_prop_key_t key,
                                                               const uint8_t *value_ptr, uint16_t value_len)
{
    ThreadError errorCode = kThreadError_None;
    uint8_t num_entries = 0;
    const uint16_t *ports = otGetUnsecurePorts(&num_entries);
    spinel_ssize_t parsedLength = 1;
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

    while ((errorCode == kThreadError_None)
           && (parsedLength > 0)
           && (value_len >= 2)
          )
    {
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

        value_ptr += parsedLength;
        value_len -= parsedLength;

        ports_changed++;
    }

    if (errorCode == kThreadError_None)
    {
        errorCode = HandleCommandPropertyGet(header, key);
    }
    else
    {
        errorCode = SendLastStatus(header, ThreadErrorToSpinelStatus(errorCode));

        if (ports_changed)
        {
            // We had an error, but we've actually changed
            // the state of these ports---so we need to report
            // those incomplete changes via an asynchronous
            // change event.
            HandleCommandPropertyGet(SPINEL_HEADER_FLAG | SPINEL_HEADER_IID_0, key);
        }
    }

    return errorCode;
}

ThreadError NcpBase::SetPropertyHandler_THREAD_ALLOW_LOCAL_NET_DATA_CHANGE(uint8_t header, spinel_prop_key_t key,
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
        errorCode = HandleCommandPropertyGet(header, key);
    }
    else
    {
        errorCode = SendLastStatus(header, ThreadErrorToSpinelStatus(errorCode));
    }

    if (should_register_with_leader)
    {
        otSendServerData();
    }

    return errorCode;
}

ThreadError NcpBase::SetPropertyHandler_CNTR_RESET(uint8_t header, spinel_prop_key_t key, const uint8_t *value_ptr,
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

    (void)key;

    // There is currently no getter for PROP_CNTR_RESET, so we just
    // return SPINEL_STATUS_OK for success when the counters are reset.

    return SendLastStatus(header, ThreadErrorToSpinelStatus(errorCode));
}

ThreadError NcpBase::SetPropertyHandler_MAC_WHITELIST(uint8_t header, spinel_prop_key_t key, const uint8_t *value_ptr, uint16_t value_len)
{
    ThreadError errorCode = kThreadError_None;
    spinel_ssize_t parsedLength = 1;

    // First, clear the whitelist.
    otClearMacWhitelist();

    while ((errorCode == kThreadError_None)
           && (parsedLength > 0)
           && (value_len > 0)
          )
    {
        otExtAddress *ext_addr = NULL;
        int8_t rssi = RSSI_OVERRIDE_DISABLED;

        parsedLength = spinel_datatype_unpack(
                           value_ptr,
                           value_len,
                           "T(Ec).",
                           &ext_addr,
                           &rssi
                       );

        if (parsedLength <= 0)
        {
            rssi = RSSI_OVERRIDE_DISABLED;
            parsedLength = spinel_datatype_unpack(
                               value_ptr,
                               value_len,
                               "T(E).",
                               &ext_addr
                           );
        }

        if (parsedLength <= 0)
        {
            errorCode = kThreadError_Parse;
            break;
        }

        if (rssi == RSSI_OVERRIDE_DISABLED)
        {
            errorCode = otAddMacWhitelist(ext_addr->m8);
        }
        else
        {
            errorCode = otAddMacWhitelistRssi(ext_addr->m8, rssi);
        }

        value_ptr += parsedLength;
        value_len -= parsedLength;
    }

    if (errorCode == kThreadError_None)
    {
        errorCode = HandleCommandPropertyGet(header, key);
    }
    else
    {
        errorCode = SendLastStatus(header, ThreadErrorToSpinelStatus(errorCode));

        // We had an error, but we may have actually changed
        // the state of the whitelist---so we need to report
        // those incomplete changes via an asynchronous
        // change event.
        HandleCommandPropertyGet(SPINEL_HEADER_FLAG | SPINEL_HEADER_IID_0, key);
    }

    return errorCode;
}

ThreadError NcpBase::SetPropertyHandler_MAC_WHITELIST_ENABLED(uint8_t header, spinel_prop_key_t key, const uint8_t *value_ptr, uint16_t value_len)
{
    bool isEnabled;
    spinel_ssize_t parsedLength;
    ThreadError errorCode = kThreadError_None;

    parsedLength = spinel_datatype_unpack(
                       value_ptr,
                       value_len,
                       SPINEL_DATATYPE_BOOL_S,
                       &isEnabled
                   );

    if (parsedLength > 0)
    {
        if (isEnabled)
        {
            otEnableMacWhitelist();
        }
        else
        {
            otDisableMacWhitelist();
        }

        errorCode = HandleCommandPropertyGet(header, key);
    }
    else
    {
        errorCode = SendLastStatus(header, SPINEL_STATUS_PARSE_ERROR);
    }

    return errorCode;
}

ThreadError NcpBase::SetPropertyHandler_THREAD_MODE(uint8_t header, spinel_prop_key_t key, const uint8_t *value_ptr, uint16_t value_len)
{
    uint8_t numeric_mode = 0;
    otLinkModeConfig mode_config;
    spinel_ssize_t parsedLength;
    ThreadError errorCode = kThreadError_None;

    parsedLength = spinel_datatype_unpack(
                       value_ptr,
                       value_len,
                       SPINEL_DATATYPE_UINT8_S,
                       &numeric_mode
                   );

    if (parsedLength > 0)
    {
        mode_config.mRxOnWhenIdle = ((numeric_mode & kThreadModeTLV_Receiver) == kThreadModeTLV_Receiver);
        mode_config.mSecureDataRequests = ((numeric_mode & kThreadModeTLV_Secure) == kThreadModeTLV_Secure);
        mode_config.mDeviceType = ((numeric_mode & kThreadModeTLV_DeviceType) == kThreadModeTLV_DeviceType);
        mode_config.mNetworkData = ((numeric_mode & kThreadModeTLV_NetworkData) == kThreadModeTLV_NetworkData);

        errorCode = otSetLinkMode(mode_config);

        if (errorCode == kThreadError_None)
        {
            errorCode = HandleCommandPropertyGet(header, key);
        }
        else
        {
            errorCode = SendLastStatus(header, ThreadErrorToSpinelStatus(errorCode));
        }
    }
    else
    {
        errorCode = SendLastStatus(header, SPINEL_STATUS_PARSE_ERROR);
    }

    return errorCode;
}

ThreadError NcpBase::SetPropertyHandler_THREAD_CHILD_TIMEOUT(uint8_t header, spinel_prop_key_t key, const uint8_t *value_ptr, uint16_t value_len)
{
    uint32_t i = 0;
    spinel_ssize_t parsedLength;
    ThreadError errorCode = kThreadError_None;

    parsedLength = spinel_datatype_unpack(
                       value_ptr,
                       value_len,
                       SPINEL_DATATYPE_UINT32_S,
                       &i
                   );

    if (parsedLength > 0)
    {
        otSetChildTimeout(i);

        errorCode = HandleCommandPropertyGet(header, key);
    }
    else
    {
        errorCode = SendLastStatus(header, SPINEL_STATUS_PARSE_ERROR);
    }

    return errorCode;
}

ThreadError NcpBase::SetPropertyHandler_THREAD_ROUTER_UPGRADE_THRESHOLD(uint8_t header, spinel_prop_key_t key, const uint8_t *value_ptr, uint16_t value_len)
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
        otSetRouterUpgradeThreshold(i);

        errorCode = HandleCommandPropertyGet(header, key);
    }
    else
    {
        errorCode = SendLastStatus(header, SPINEL_STATUS_PARSE_ERROR);
    }

    return errorCode;
}

ThreadError NcpBase::SetPropertyHandler_THREAD_CONTEXT_REUSE_DELAY(uint8_t header, spinel_prop_key_t key, const uint8_t *value_ptr, uint16_t value_len)
{
    uint32_t i = 0;
    spinel_ssize_t parsedLength;
    ThreadError errorCode = kThreadError_None;

    parsedLength = spinel_datatype_unpack(
                       value_ptr,
                       value_len,
                       SPINEL_DATATYPE_UINT32_S,
                       &i
                   );

    if (parsedLength > 0)
    {
        otSetContextIdReuseDelay(i);

        errorCode = HandleCommandPropertyGet(header, key);
    }
    else
    {
        errorCode = SendLastStatus(header, SPINEL_STATUS_PARSE_ERROR);
    }

    return errorCode;
}

ThreadError NcpBase::SetPropertyHandler_THREAD_NETWORK_ID_TIMEOUT(uint8_t header, spinel_prop_key_t key, const uint8_t *value_ptr, uint16_t value_len)
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
        otSetNetworkIdTimeout(i);

        errorCode = HandleCommandPropertyGet(header, key);
    }
    else
    {
        errorCode = SendLastStatus(header, SPINEL_STATUS_PARSE_ERROR);
    }

    return errorCode;
}

#if OPENTHREAD_ENABLE_DIAG
ThreadError NcpBase::SetPropertyHandler_NEST_STREAM_MFG(uint8_t header, spinel_prop_key_t key, const uint8_t *value_ptr, uint16_t value_len)
{
    char *string(NULL);
    char *output(NULL);
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
        // all diagnostics related features are processed within diagnostics module
        output = diagProcessCmdLine(string);

        errorCode = SendPropertyUpdate(
                        header,
                        SPINEL_CMD_PROP_VALUE_IS,
                        key,
                        reinterpret_cast<uint8_t *>(output),
                        static_cast<uint16_t>(strlen(output) + 1)
                    );
    }
    else
    {
        errorCode = SendLastStatus(header, SPINEL_STATUS_PARSE_ERROR);
    }

    return errorCode;
}
#endif

// ----------------------------------------------------------------------------
// MARK: Individual Property Inserters
// ----------------------------------------------------------------------------


ThreadError NcpBase::InsertPropertyHandler_IPV6_ADDRESS_TABLE(uint8_t header, spinel_prop_key_t key,
                                                              const uint8_t *value_ptr, uint16_t value_len)
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

    errorCode = SendPropertyUpdate(
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
        errorCode = SendLastStatus(header, errorStatus);
    }

    return errorCode;
}

ThreadError NcpBase::InsertPropertyHandler_THREAD_LOCAL_ROUTES(uint8_t header, spinel_prop_key_t key,
                                                               const uint8_t *value_ptr, uint16_t value_len)
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
        errorCode = SendLastStatus(header, SPINEL_STATUS_INVALID_STATE)
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
            errorCode = SendPropertyUpdate(
                            header,
                            SPINEL_CMD_PROP_VALUE_INSERTED,
                            key,
                            value_ptr,
                            value_len
                        );
        }
        else
        {
            errorCode = SendLastStatus(header, ThreadErrorToSpinelStatus(errorCode));
        }
    }
    else
    {
        errorCode = SendLastStatus(header, SPINEL_STATUS_PARSE_ERROR);
    }

exit:
    return errorCode;
}

ThreadError NcpBase::InsertPropertyHandler_THREAD_ON_MESH_NETS(uint8_t header, spinel_prop_key_t key,
                                                               const uint8_t *value_ptr, uint16_t value_len)
{
    const static int kPreferenceOffset = 6;
    const static int kPreferenceMask = 3 << kPreferenceOffset;
    const static int kPreferredFlag = 1 << 5;
    const static int kSlaacFlag = 1 << 4;
    const static int kDhcpFlag = 1 << 3;
    const static int kConfigureFlag = 1 << 2;
    const static int kDefaultRouteFlag = 1 << 1;
    const static int kOnMeshFlag = 1 << 0;

    spinel_ssize_t parsedLength;
    ThreadError errorCode = kThreadError_None;

    otBorderRouterConfig border_router_config;
    otIp6Address *addr_ptr;
    bool stable = false;
    uint8_t flags = 0;

    memset(&border_router_config, 0, sizeof(otBorderRouterConfig));

    VerifyOrExit(
        mAllowLocalNetworkDataChange == true,
        errorCode = SendLastStatus(header, SPINEL_STATUS_INVALID_STATE)
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
        border_router_config.mPreferred = ((flags & kPreferredFlag) == kPreferredFlag);
        border_router_config.mSlaac = ((flags & kSlaacFlag) == kSlaacFlag);
        border_router_config.mDhcp = ((flags & kDhcpFlag) == kDhcpFlag);
        border_router_config.mConfigure = ((flags & kConfigureFlag) == kConfigureFlag);
        border_router_config.mDefaultRoute = ((flags & kDefaultRouteFlag) == kDefaultRouteFlag);
        border_router_config.mOnMesh = ((flags & kOnMeshFlag) == kOnMeshFlag);

        errorCode = otAddBorderRouter(&border_router_config);

        if (errorCode == kThreadError_None)
        {
            errorCode = SendPropertyUpdate(
                            header,
                            SPINEL_CMD_PROP_VALUE_INSERTED,
                            key,
                            value_ptr,
                            value_len
                        );
        }
        else
        {
            errorCode = SendLastStatus(header, ThreadErrorToSpinelStatus(errorCode));
        }
    }
    else
    {
        errorCode = SendLastStatus(header, SPINEL_STATUS_PARSE_ERROR);
    }

exit:
    return errorCode;
}

ThreadError NcpBase::InsertPropertyHandler_THREAD_ASSISTING_PORTS(uint8_t header, spinel_prop_key_t key,
                                                                  const uint8_t *value_ptr, uint16_t value_len)
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
            errorCode = SendPropertyUpdate(
                            header,
                            SPINEL_CMD_PROP_VALUE_INSERTED,
                            key,
                            value_ptr,
                            value_len
                        );
        }
        else
        {
            errorCode = SendLastStatus(header, ThreadErrorToSpinelStatus(errorCode));
        }
    }
    else
    {
        errorCode = SendLastStatus(header, SPINEL_STATUS_PARSE_ERROR);
    }

    return errorCode;
}

ThreadError NcpBase::InsertPropertyHandler_MAC_WHITELIST(uint8_t header, spinel_prop_key_t key, const uint8_t *value_ptr, uint16_t value_len)
{
    ThreadError errorCode = kThreadError_None;
    spinel_ssize_t parsedLength;
    otExtAddress *ext_addr = NULL;
    int8_t rssi = RSSI_OVERRIDE_DISABLED;


    if (value_len > static_cast<spinel_ssize_t>(sizeof(ext_addr)))
    {
        parsedLength = spinel_datatype_unpack(
            value_ptr,
            value_len,
            "Ec",
            &ext_addr,
            &rssi
        );
    }
    else
    {
        parsedLength = spinel_datatype_unpack(
            value_ptr,
            value_len,
            "E",
            &ext_addr
        );
    }

    if (parsedLength > 0)
    {
        if (rssi == RSSI_OVERRIDE_DISABLED)
        {
            errorCode = otAddMacWhitelist(ext_addr->m8);
        }
        else
        {
            errorCode = otAddMacWhitelistRssi(ext_addr->m8, rssi);
        }

        if (errorCode == kThreadError_None)
        {
            errorCode = SendPropertyUpdate(
                            header,
                            SPINEL_CMD_PROP_VALUE_INSERTED,
                            key,
                            value_ptr,
                            value_len
                        );
        }
        else
        {
            errorCode = SendLastStatus(header, ThreadErrorToSpinelStatus(errorCode));
        }
    }
    else
    {
        errorCode = SendLastStatus(header, SPINEL_STATUS_PARSE_ERROR);
    }

    return errorCode;
}

// ----------------------------------------------------------------------------
// MARK: Individual Property Removers
// ----------------------------------------------------------------------------


ThreadError NcpBase::RemovePropertyHandler_IPV6_ADDRESS_TABLE(uint8_t header, spinel_prop_key_t key,
                                                              const uint8_t *value_ptr, uint16_t value_len)
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

                errorCode = SendPropertyUpdate(
                                header,
                                SPINEL_CMD_PROP_VALUE_REMOVED,
                                key,
                                value_ptr,
                                value_len
                            );
            }
            else
            {
                errorCode = SendLastStatus(header, ThreadErrorToSpinelStatus(errorCode));
            }
        }
        else
        {
            errorCode = SendLastStatus(header, ThreadErrorToSpinelStatus(kThreadError_NoAddress));
        }
    }
    else
    {
        errorCode = SendLastStatus(header, SPINEL_STATUS_PARSE_ERROR);
    }

    return errorCode;
}

ThreadError NcpBase::RemovePropertyHandler_THREAD_LOCAL_ROUTES(uint8_t header, spinel_prop_key_t key,
                                                               const uint8_t *value_ptr, uint16_t value_len)
{
    spinel_ssize_t parsedLength;
    ThreadError errorCode = kThreadError_None;

    otIp6Prefix ip6_prefix;
    memset(&ip6_prefix, 0, sizeof(otIp6Prefix));
    otIp6Address *addr_ptr;

    VerifyOrExit(
        mAllowLocalNetworkDataChange == true,
        errorCode = SendLastStatus(header, SPINEL_STATUS_INVALID_STATE)
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
            errorCode = SendPropertyUpdate(
                            header,
                            SPINEL_CMD_PROP_VALUE_REMOVED,
                            key,
                            value_ptr,
                            value_len
                        );
        }
        else
        {
            errorCode = SendLastStatus(header, ThreadErrorToSpinelStatus(errorCode));
        }
    }
    else
    {
        errorCode = SendLastStatus(header, SPINEL_STATUS_PARSE_ERROR);
    }

exit:
    return errorCode;
}

ThreadError NcpBase::RemovePropertyHandler_THREAD_ON_MESH_NETS(uint8_t header, spinel_prop_key_t key,
                                                               const uint8_t *value_ptr, uint16_t value_len)
{
    spinel_ssize_t parsedLength;
    ThreadError errorCode = kThreadError_None;

    otIp6Prefix ip6_prefix;
    memset(&ip6_prefix, 0, sizeof(otIp6Prefix));
    otIp6Address *addr_ptr;

    VerifyOrExit(
        mAllowLocalNetworkDataChange == true,
        errorCode = SendLastStatus(header, SPINEL_STATUS_INVALID_STATE)
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
            errorCode = SendPropertyUpdate(
                            header,
                            SPINEL_CMD_PROP_VALUE_REMOVED,
                            key,
                            value_ptr,
                            value_len
                        );
        }
        else
        {
            errorCode = SendLastStatus(header, ThreadErrorToSpinelStatus(errorCode));
        }
    }
    else
    {
        errorCode = SendLastStatus(header, SPINEL_STATUS_PARSE_ERROR);
    }

exit:
    return errorCode;
}

ThreadError NcpBase::RemovePropertyHandler_THREAD_ASSISTING_PORTS(uint8_t header, spinel_prop_key_t key,
                                                                  const uint8_t *value_ptr, uint16_t value_len)
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
            errorCode = SendPropertyUpdate(
                            header,
                            SPINEL_CMD_PROP_VALUE_REMOVED,
                            key,
                            value_ptr,
                            value_len
                        );
        }
        else
        {
            errorCode = SendLastStatus(header, ThreadErrorToSpinelStatus(errorCode));
        }
    }
    else
    {
        errorCode = SendLastStatus(header, SPINEL_STATUS_PARSE_ERROR);
    }

    return errorCode;
}

ThreadError NcpBase::RemovePropertyHandler_THREAD_ACTIVE_ROUTER_IDS(uint8_t header, spinel_prop_key_t key,
                                                                  const uint8_t *value_ptr, uint16_t value_len)
{
    spinel_ssize_t parsedLength;
    ThreadError errorCode = kThreadError_None;
    uint8_t router_id;

    parsedLength = spinel_datatype_unpack(
                       value_ptr,
                       value_len,
                       "C",
                       &router_id
                   );

    if (parsedLength > 0)
    {
        errorCode = otReleaseRouterId(router_id);

        if (errorCode == kThreadError_None)
        {
            errorCode = SendPropertyUpdate(
                            header,
                            SPINEL_CMD_PROP_VALUE_REMOVED,
                            key,
                            value_ptr,
                            value_len
                        );
        }
        else
        {
            errorCode = SendLastStatus(header, ThreadErrorToSpinelStatus(errorCode));
        }
    }
    else
    {
        errorCode = SendLastStatus(header, SPINEL_STATUS_PARSE_ERROR);
    }

    return errorCode;
}

ThreadError NcpBase::RemovePropertyHandler_MAC_WHITELIST(uint8_t header, spinel_prop_key_t key, const uint8_t *value_ptr, uint16_t value_len)
{
    ThreadError errorCode = kThreadError_None;
    spinel_ssize_t parsedLength;
    otExtAddress ext_addr;

    parsedLength = spinel_datatype_unpack(
                       value_ptr,
                       value_len,
                       "E",
                       &ext_addr
                   );

    if (parsedLength > 0)
    {
        otRemoveMacWhitelist(ext_addr.m8);

        errorCode = SendPropertyUpdate(
                        header,
                        SPINEL_CMD_PROP_VALUE_REMOVED,
                        key,
                        value_ptr,
                        value_len
                    );
    }
    else
    {
        errorCode = SendLastStatus(header, SPINEL_STATUS_PARSE_ERROR);
    }

    return errorCode;
}

}  // namespace Thread


// ----------------------------------------------------------------------------
// MARK: Virtual Datastream I/O (Public API)
// ----------------------------------------------------------------------------

ThreadError otNcpStreamWrite(int aStreamId, const uint8_t* aDataPtr, int aDataLen)
{
    if (aStreamId == 0)
    {
        aStreamId = SPINEL_PROP_STREAM_DEBUG;
    }

    return Thread::sNcpContext->SendPropertyUpdate(
        SPINEL_HEADER_FLAG | SPINEL_HEADER_IID_0,
        SPINEL_CMD_PROP_VALUE_IS,
        static_cast<spinel_prop_key_t>(aStreamId),
        aDataPtr,
        static_cast<uint16_t>(aDataLen)
    );
}
