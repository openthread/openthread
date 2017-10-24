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
 *   This file implements general thread device required Spinel interface to the OpenThread stack.
 */

#include "ncp_base.hpp"

#include <stdarg.h>
#include <stdlib.h>

#include <openthread/diag.h>
#include <openthread/icmp6.h>

#include <openthread/ncp.h>
#include <openthread/openthread.h>

#include <openthread/platform/misc.h>
#include <openthread/platform/radio.h>

#include "openthread-instance.h"
#include "common/code_utils.hpp"
#include "common/debug.hpp"
#include "mac/mac_frame.hpp"
#include "net/ip6.hpp"

namespace ot {
namespace Ncp {

// ----------------------------------------------------------------------------
// MARK: Property Handler Jump Tables and Methods
// ----------------------------------------------------------------------------

#define NCP_GET_PROP_HANDLER_ENTRY(name)                 { SPINEL_PROP_##name, &NcpBase::GetPropertyHandler_##name }

const NcpBase::PropertyHandlerEntry NcpBase::mGetPropertyHandlerTable[] =
{
    NCP_GET_PROP_HANDLER_ENTRY(CAPS),
    NCP_GET_PROP_HANDLER_ENTRY(DEBUG_TEST_ASSERT),
    NCP_GET_PROP_HANDLER_ENTRY(DEBUG_NCP_LOG_LEVEL),
    NCP_GET_PROP_HANDLER_ENTRY(HWADDR),
    NCP_GET_PROP_HANDLER_ENTRY(HOST_POWER_STATE),
    NCP_GET_PROP_HANDLER_ENTRY(INTERFACE_COUNT),
    NCP_GET_PROP_HANDLER_ENTRY(INTERFACE_TYPE),
    NCP_GET_PROP_HANDLER_ENTRY(LAST_STATUS),
    NCP_GET_PROP_HANDLER_ENTRY(LOCK),
    NCP_GET_PROP_HANDLER_ENTRY(PHY_ENABLED),
    NCP_GET_PROP_HANDLER_ENTRY(PHY_CHAN),
    NCP_GET_PROP_HANDLER_ENTRY(PHY_RX_SENSITIVITY),
    NCP_GET_PROP_HANDLER_ENTRY(PHY_TX_POWER),
    NCP_GET_PROP_HANDLER_ENTRY(POWER_STATE),
    NCP_GET_PROP_HANDLER_ENTRY(PROTOCOL_VERSION),
    NCP_GET_PROP_HANDLER_ENTRY(MAC_15_4_PANID),
    NCP_GET_PROP_HANDLER_ENTRY(MAC_15_4_LADDR),
    NCP_GET_PROP_HANDLER_ENTRY(MAC_15_4_SADDR),
    NCP_GET_PROP_HANDLER_ENTRY(MAC_RAW_STREAM_ENABLED),
    NCP_GET_PROP_HANDLER_ENTRY(MAC_PROMISCUOUS_MODE),
    NCP_GET_PROP_HANDLER_ENTRY(NCP_VERSION),
    NCP_GET_PROP_HANDLER_ENTRY(UNSOL_UPDATE_FILTER),
    NCP_GET_PROP_HANDLER_ENTRY(UNSOL_UPDATE_LIST),
    NCP_GET_PROP_HANDLER_ENTRY(VENDOR_ID),
#if OPENTHREAD_MTD || OPENTHREAD_FTD
    NCP_GET_PROP_HANDLER_ENTRY(MAC_DATA_POLL_PERIOD),
    NCP_GET_PROP_HANDLER_ENTRY(MAC_EXTENDED_ADDR),
    NCP_GET_PROP_HANDLER_ENTRY(MAC_SCAN_STATE),
    NCP_GET_PROP_HANDLER_ENTRY(MAC_SCAN_MASK),
    NCP_GET_PROP_HANDLER_ENTRY(MAC_SCAN_PERIOD),
#if OPENTHREAD_ENABLE_MAC_FILTER
    NCP_GET_PROP_HANDLER_ENTRY(MAC_BLACKLIST),
    NCP_GET_PROP_HANDLER_ENTRY(MAC_BLACKLIST_ENABLED),
    NCP_GET_PROP_HANDLER_ENTRY(MAC_FIXED_RSS),
    NCP_GET_PROP_HANDLER_ENTRY(MAC_WHITELIST),
    NCP_GET_PROP_HANDLER_ENTRY(MAC_WHITELIST_ENABLED),
#endif
    NCP_GET_PROP_HANDLER_ENTRY(MSG_BUFFER_COUNTERS),
    NCP_GET_PROP_HANDLER_ENTRY(PHY_CHAN_SUPPORTED),
    NCP_GET_PROP_HANDLER_ENTRY(PHY_FREQ),
    NCP_GET_PROP_HANDLER_ENTRY(PHY_RSSI),
    NCP_GET_PROP_HANDLER_ENTRY(NET_IF_UP),
    NCP_GET_PROP_HANDLER_ENTRY(NET_KEY_SEQUENCE_COUNTER),
    NCP_GET_PROP_HANDLER_ENTRY(NET_KEY_SWITCH_GUARDTIME),
    NCP_GET_PROP_HANDLER_ENTRY(NET_MASTER_KEY),
    NCP_GET_PROP_HANDLER_ENTRY(NET_NETWORK_NAME),
    NCP_GET_PROP_HANDLER_ENTRY(NET_PARTITION_ID),
    NCP_GET_PROP_HANDLER_ENTRY(NET_REQUIRE_JOIN_EXISTING),
    NCP_GET_PROP_HANDLER_ENTRY(NET_ROLE),
    NCP_GET_PROP_HANDLER_ENTRY(NET_SAVED),
    NCP_GET_PROP_HANDLER_ENTRY(NET_STACK_UP),
    NCP_GET_PROP_HANDLER_ENTRY(NET_XPANID),
    NCP_GET_PROP_HANDLER_ENTRY(THREAD_ALLOW_LOCAL_NET_DATA_CHANGE),
    NCP_GET_PROP_HANDLER_ENTRY(THREAD_ASSISTING_PORTS),
    NCP_GET_PROP_HANDLER_ENTRY(THREAD_CHILD_TIMEOUT),
    NCP_GET_PROP_HANDLER_ENTRY(THREAD_DISCOVERY_SCAN_JOINER_FLAG),
    NCP_GET_PROP_HANDLER_ENTRY(THREAD_DISCOVERY_SCAN_ENABLE_FILTERING),
    NCP_GET_PROP_HANDLER_ENTRY(THREAD_DISCOVERY_SCAN_PANID),
    NCP_GET_PROP_HANDLER_ENTRY(THREAD_LEADER_ADDR),
    NCP_GET_PROP_HANDLER_ENTRY(THREAD_LEADER_NETWORK_DATA),
    NCP_GET_PROP_HANDLER_ENTRY(THREAD_LEADER_RID),
    NCP_GET_PROP_HANDLER_ENTRY(THREAD_MODE),
    NCP_GET_PROP_HANDLER_ENTRY(THREAD_NEIGHBOR_TABLE),
    NCP_GET_PROP_HANDLER_ENTRY(THREAD_NETWORK_DATA_VERSION),
    NCP_GET_PROP_HANDLER_ENTRY(THREAD_OFF_MESH_ROUTES),
    NCP_GET_PROP_HANDLER_ENTRY(THREAD_ON_MESH_NETS),
    NCP_GET_PROP_HANDLER_ENTRY(THREAD_PARENT),
    NCP_GET_PROP_HANDLER_ENTRY(THREAD_RLOC16),
    NCP_GET_PROP_HANDLER_ENTRY(THREAD_RLOC16_DEBUG_PASSTHRU),
    NCP_GET_PROP_HANDLER_ENTRY(THREAD_STABLE_LEADER_NETWORK_DATA),
    NCP_GET_PROP_HANDLER_ENTRY(THREAD_STABLE_NETWORK_DATA_VERSION),
#if OPENTHREAD_ENABLE_BORDER_ROUTER
    NCP_GET_PROP_HANDLER_ENTRY(THREAD_NETWORK_DATA),
    NCP_GET_PROP_HANDLER_ENTRY(THREAD_STABLE_NETWORK_DATA),
#endif
    NCP_GET_PROP_HANDLER_ENTRY(IPV6_ADDRESS_TABLE),
    NCP_GET_PROP_HANDLER_ENTRY(IPV6_ICMP_PING_OFFLOAD),
    NCP_GET_PROP_HANDLER_ENTRY(IPV6_LL_ADDR),
    NCP_GET_PROP_HANDLER_ENTRY(IPV6_ML_PREFIX),
    NCP_GET_PROP_HANDLER_ENTRY(IPV6_ML_ADDR),
    NCP_GET_PROP_HANDLER_ENTRY(IPV6_MULTICAST_ADDRESS_TABLE),
    NCP_GET_PROP_HANDLER_ENTRY(IPV6_ROUTE_TABLE),
#if OPENTHREAD_ENABLE_JAM_DETECTION
    NCP_GET_PROP_HANDLER_ENTRY(JAM_DETECT_ENABLE),
    NCP_GET_PROP_HANDLER_ENTRY(JAM_DETECTED),
    NCP_GET_PROP_HANDLER_ENTRY(JAM_DETECT_RSSI_THRESHOLD),
    NCP_GET_PROP_HANDLER_ENTRY(JAM_DETECT_WINDOW),
    NCP_GET_PROP_HANDLER_ENTRY(JAM_DETECT_BUSY),
    NCP_GET_PROP_HANDLER_ENTRY(JAM_DETECT_HISTORY_BITMAP),
#endif
#if OPENTHREAD_ENABLE_LEGACY
    NCP_GET_PROP_HANDLER_ENTRY(NEST_LEGACY_ULA_PREFIX),
    NCP_GET_PROP_HANDLER_ENTRY(NEST_LEGACY_LAST_NODE_JOINED),
#endif
    // MAC counters
    NCP_GET_PROP_HANDLER_ENTRY(CNTR_TX_PKT_TOTAL),
    NCP_GET_PROP_HANDLER_ENTRY(CNTR_TX_PKT_ACK_REQ),
    NCP_GET_PROP_HANDLER_ENTRY(CNTR_TX_PKT_ACKED),
    NCP_GET_PROP_HANDLER_ENTRY(CNTR_TX_PKT_NO_ACK_REQ),
    NCP_GET_PROP_HANDLER_ENTRY(CNTR_TX_PKT_DATA),
    NCP_GET_PROP_HANDLER_ENTRY(CNTR_TX_PKT_DATA_POLL),
    NCP_GET_PROP_HANDLER_ENTRY(CNTR_TX_PKT_BEACON),
    NCP_GET_PROP_HANDLER_ENTRY(CNTR_TX_PKT_BEACON_REQ),
    NCP_GET_PROP_HANDLER_ENTRY(CNTR_TX_PKT_OTHER),
    NCP_GET_PROP_HANDLER_ENTRY(CNTR_TX_PKT_RETRY),
    NCP_GET_PROP_HANDLER_ENTRY(CNTR_TX_PKT_UNICAST),
    NCP_GET_PROP_HANDLER_ENTRY(CNTR_TX_PKT_BROADCAST),
    NCP_GET_PROP_HANDLER_ENTRY(CNTR_TX_ERR_CCA),
    NCP_GET_PROP_HANDLER_ENTRY(CNTR_TX_ERR_ABORT),
    NCP_GET_PROP_HANDLER_ENTRY(CNTR_RX_PKT_TOTAL),
    NCP_GET_PROP_HANDLER_ENTRY(CNTR_RX_PKT_DATA),
    NCP_GET_PROP_HANDLER_ENTRY(CNTR_RX_PKT_DATA_POLL),
    NCP_GET_PROP_HANDLER_ENTRY(CNTR_RX_PKT_BEACON),
    NCP_GET_PROP_HANDLER_ENTRY(CNTR_RX_PKT_BEACON_REQ),
    NCP_GET_PROP_HANDLER_ENTRY(CNTR_RX_PKT_OTHER),
    NCP_GET_PROP_HANDLER_ENTRY(CNTR_RX_PKT_FILT_WL),
    NCP_GET_PROP_HANDLER_ENTRY(CNTR_RX_PKT_FILT_DA),
    NCP_GET_PROP_HANDLER_ENTRY(CNTR_RX_PKT_UNICAST),
    NCP_GET_PROP_HANDLER_ENTRY(CNTR_RX_PKT_BROADCAST),
    NCP_GET_PROP_HANDLER_ENTRY(CNTR_RX_ERR_EMPTY),
    NCP_GET_PROP_HANDLER_ENTRY(CNTR_RX_ERR_UKWN_NBR),
    NCP_GET_PROP_HANDLER_ENTRY(CNTR_RX_ERR_NVLD_SADDR),
    NCP_GET_PROP_HANDLER_ENTRY(CNTR_RX_ERR_SECURITY),
    NCP_GET_PROP_HANDLER_ENTRY(CNTR_RX_ERR_BAD_FCS),
    NCP_GET_PROP_HANDLER_ENTRY(CNTR_RX_ERR_OTHER),
    NCP_GET_PROP_HANDLER_ENTRY(CNTR_RX_PKT_DUP),
    // NCP counters
    NCP_GET_PROP_HANDLER_ENTRY(CNTR_TX_IP_SEC_TOTAL),
    NCP_GET_PROP_HANDLER_ENTRY(CNTR_TX_IP_INSEC_TOTAL),
    NCP_GET_PROP_HANDLER_ENTRY(CNTR_TX_IP_DROPPED),
    NCP_GET_PROP_HANDLER_ENTRY(CNTR_RX_IP_SEC_TOTAL),
    NCP_GET_PROP_HANDLER_ENTRY(CNTR_RX_IP_INSEC_TOTAL),
    NCP_GET_PROP_HANDLER_ENTRY(CNTR_RX_IP_DROPPED),
    NCP_GET_PROP_HANDLER_ENTRY(CNTR_TX_SPINEL_TOTAL),
    NCP_GET_PROP_HANDLER_ENTRY(CNTR_RX_SPINEL_TOTAL),
    NCP_GET_PROP_HANDLER_ENTRY(CNTR_RX_SPINEL_ERR),
    // IP counters
    NCP_GET_PROP_HANDLER_ENTRY(CNTR_IP_TX_SUCCESS),
    NCP_GET_PROP_HANDLER_ENTRY(CNTR_IP_RX_SUCCESS),
    NCP_GET_PROP_HANDLER_ENTRY(CNTR_IP_TX_FAILURE),
    NCP_GET_PROP_HANDLER_ENTRY(CNTR_IP_RX_FAILURE),
#endif // OPENTHREAD_MTD || OPENTHREAD_FTD

#if OPENTHREAD_FTD
    NCP_GET_PROP_HANDLER_ENTRY(NET_PSKC),
    NCP_GET_PROP_HANDLER_ENTRY(THREAD_LEADER_WEIGHT),
    NCP_GET_PROP_HANDLER_ENTRY(THREAD_CHILD_TABLE),
    NCP_GET_PROP_HANDLER_ENTRY(THREAD_LOCAL_LEADER_WEIGHT),
    NCP_GET_PROP_HANDLER_ENTRY(THREAD_ROUTER_ROLE_ENABLED),
    NCP_GET_PROP_HANDLER_ENTRY(THREAD_CHILD_COUNT_MAX),
    NCP_GET_PROP_HANDLER_ENTRY(THREAD_ROUTER_UPGRADE_THRESHOLD),
    NCP_GET_PROP_HANDLER_ENTRY(THREAD_ROUTER_DOWNGRADE_THRESHOLD),
    NCP_GET_PROP_HANDLER_ENTRY(THREAD_CONTEXT_REUSE_DELAY),
    NCP_GET_PROP_HANDLER_ENTRY(THREAD_NETWORK_ID_TIMEOUT),
    NCP_GET_PROP_HANDLER_ENTRY(THREAD_ROUTER_SELECTION_JITTER),
#if OPENTHREAD_ENABLE_COMMISSIONER
    NCP_GET_PROP_HANDLER_ENTRY(THREAD_COMMISSIONER_ENABLED),
#endif
#if OPENTHREAD_ENABLE_TMF_PROXY
    NCP_GET_PROP_HANDLER_ENTRY(THREAD_TMF_PROXY_ENABLED),
#endif
#endif // OPENTHREAD_FTD
};

#define NCP_SET_PROP_HANDLER_ENTRY(name)  { SPINEL_PROP_##name, &NcpBase::SetPropertyHandler_##name }

const NcpBase::PropertyHandlerEntry NcpBase::mSetPropertyHandlerTable[] =
{
    NCP_SET_PROP_HANDLER_ENTRY(POWER_STATE),
    NCP_SET_PROP_HANDLER_ENTRY(UNSOL_UPDATE_FILTER),
    NCP_SET_PROP_HANDLER_ENTRY(PHY_TX_POWER),
    NCP_SET_PROP_HANDLER_ENTRY(PHY_CHAN),
    NCP_SET_PROP_HANDLER_ENTRY(MAC_PROMISCUOUS_MODE),
    NCP_SET_PROP_HANDLER_ENTRY(MAC_15_4_PANID),
    NCP_SET_PROP_HANDLER_ENTRY(MAC_15_4_LADDR),
    NCP_SET_PROP_HANDLER_ENTRY(MAC_RAW_STREAM_ENABLED),
#if OPENTHREAD_ENABLE_RAW_LINK_API
    NCP_SET_PROP_HANDLER_ENTRY(MAC_15_4_SADDR),
    NCP_SET_PROP_HANDLER_ENTRY(MAC_SRC_MATCH_ENABLED),
    NCP_SET_PROP_HANDLER_ENTRY(MAC_SRC_MATCH_SHORT_ADDRESSES),
    NCP_SET_PROP_HANDLER_ENTRY(MAC_SRC_MATCH_EXTENDED_ADDRESSES),
    NCP_SET_PROP_HANDLER_ENTRY(PHY_ENABLED),
#endif // OPENTHREAD_ENABLE_RAW_LINK_API
#if OPENTHREAD_MTD || OPENTHREAD_FTD
    NCP_SET_PROP_HANDLER_ENTRY(MAC_DATA_POLL_PERIOD),
    NCP_SET_PROP_HANDLER_ENTRY(MAC_SCAN_MASK),
    NCP_SET_PROP_HANDLER_ENTRY(MAC_SCAN_STATE),
    NCP_SET_PROP_HANDLER_ENTRY(MAC_SCAN_PERIOD),
    NCP_SET_PROP_HANDLER_ENTRY(NET_IF_UP),
    NCP_SET_PROP_HANDLER_ENTRY(NET_STACK_UP),
    NCP_SET_PROP_HANDLER_ENTRY(NET_ROLE),
    NCP_SET_PROP_HANDLER_ENTRY(NET_NETWORK_NAME),
    NCP_SET_PROP_HANDLER_ENTRY(NET_XPANID),
    NCP_SET_PROP_HANDLER_ENTRY(NET_MASTER_KEY),
    NCP_SET_PROP_HANDLER_ENTRY(NET_KEY_SEQUENCE_COUNTER),
    NCP_SET_PROP_HANDLER_ENTRY(NET_KEY_SWITCH_GUARDTIME),
    NCP_SET_PROP_HANDLER_ENTRY(THREAD_ASSISTING_PORTS),
    NCP_SET_PROP_HANDLER_ENTRY(STREAM_NET_INSECURE),
    NCP_SET_PROP_HANDLER_ENTRY(STREAM_NET),
    NCP_SET_PROP_HANDLER_ENTRY(IPV6_ML_PREFIX),
    NCP_SET_PROP_HANDLER_ENTRY(IPV6_ICMP_PING_OFFLOAD),
    NCP_SET_PROP_HANDLER_ENTRY(THREAD_RLOC16_DEBUG_PASSTHRU),
#if OPENTHREAD_ENABLE_MAC_FILTER
    NCP_SET_PROP_HANDLER_ENTRY(MAC_WHITELIST),
    NCP_SET_PROP_HANDLER_ENTRY(MAC_WHITELIST_ENABLED),
    NCP_SET_PROP_HANDLER_ENTRY(MAC_BLACKLIST),
    NCP_SET_PROP_HANDLER_ENTRY(MAC_BLACKLIST_ENABLED),
    NCP_SET_PROP_HANDLER_ENTRY(MAC_FIXED_RSS),
#endif
    NCP_SET_PROP_HANDLER_ENTRY(THREAD_MODE),
    NCP_SET_PROP_HANDLER_ENTRY(NET_REQUIRE_JOIN_EXISTING),
    NCP_SET_PROP_HANDLER_ENTRY(DEBUG_NCP_LOG_LEVEL),
    NCP_SET_PROP_HANDLER_ENTRY(THREAD_DISCOVERY_SCAN_JOINER_FLAG),
    NCP_SET_PROP_HANDLER_ENTRY(THREAD_DISCOVERY_SCAN_ENABLE_FILTERING),
    NCP_SET_PROP_HANDLER_ENTRY(THREAD_DISCOVERY_SCAN_PANID),
#if OPENTHREAD_ENABLE_BORDER_ROUTER
    NCP_SET_PROP_HANDLER_ENTRY(THREAD_ALLOW_LOCAL_NET_DATA_CHANGE),
#endif
#if OPENTHREAD_ENABLE_JAM_DETECTION
    NCP_SET_PROP_HANDLER_ENTRY(JAM_DETECT_ENABLE),
    NCP_SET_PROP_HANDLER_ENTRY(JAM_DETECT_RSSI_THRESHOLD),
    NCP_SET_PROP_HANDLER_ENTRY(JAM_DETECT_WINDOW),
    NCP_SET_PROP_HANDLER_ENTRY(JAM_DETECT_BUSY),
#endif
#if OPENTHREAD_ENABLE_LEGACY
    NCP_SET_PROP_HANDLER_ENTRY(NEST_LEGACY_ULA_PREFIX),
#endif
    NCP_SET_PROP_HANDLER_ENTRY(CNTR_RESET),
#endif // OPENTHREAD_MTD || OPENTHREAD_FTD
#if OPENTHREAD_FTD
    NCP_SET_PROP_HANDLER_ENTRY(NET_PSKC),
    NCP_SET_PROP_HANDLER_ENTRY(THREAD_CHILD_TIMEOUT),
    NCP_SET_PROP_HANDLER_ENTRY(THREAD_NETWORK_ID_TIMEOUT),
    NCP_SET_PROP_HANDLER_ENTRY(THREAD_LOCAL_LEADER_WEIGHT),
    NCP_SET_PROP_HANDLER_ENTRY(THREAD_ROUTER_ROLE_ENABLED),
    NCP_SET_PROP_HANDLER_ENTRY(THREAD_CHILD_COUNT_MAX),
    NCP_SET_PROP_HANDLER_ENTRY(THREAD_ROUTER_UPGRADE_THRESHOLD),
    NCP_SET_PROP_HANDLER_ENTRY(THREAD_ROUTER_DOWNGRADE_THRESHOLD),
    NCP_SET_PROP_HANDLER_ENTRY(THREAD_CONTEXT_REUSE_DELAY),
    NCP_SET_PROP_HANDLER_ENTRY(THREAD_ROUTER_SELECTION_JITTER),
    NCP_SET_PROP_HANDLER_ENTRY(THREAD_PREFERRED_ROUTER_ID),
#if OPENTHREAD_CONFIG_ENABLE_STEERING_DATA_SET_OOB
    NCP_SET_PROP_HANDLER_ENTRY(THREAD_STEERING_DATA),
#endif
#if OPENTHREAD_ENABLE_TMF_PROXY
    NCP_SET_PROP_HANDLER_ENTRY(THREAD_TMF_PROXY_ENABLED),
    NCP_SET_PROP_HANDLER_ENTRY(THREAD_TMF_PROXY_STREAM),
#endif
#endif // #if OPENTHREAD_FTD
};

#define NCP_INSERT_PROP_HANDLER_ENTRY(name)  { SPINEL_PROP_##name, &NcpBase::InsertPropertyHandler_##name }

const NcpBase::PropertyHandlerEntry NcpBase::mInsertPropertyHandlerTable[] =
{
    NCP_INSERT_PROP_HANDLER_ENTRY(UNSOL_UPDATE_FILTER),
#if OPENTHREAD_ENABLE_RAW_LINK_API
    NCP_INSERT_PROP_HANDLER_ENTRY(MAC_SRC_MATCH_SHORT_ADDRESSES),
    NCP_INSERT_PROP_HANDLER_ENTRY(MAC_SRC_MATCH_EXTENDED_ADDRESSES),
#endif
#if OPENTHREAD_MTD || OPENTHREAD_FTD
    NCP_INSERT_PROP_HANDLER_ENTRY(IPV6_ADDRESS_TABLE),
    NCP_INSERT_PROP_HANDLER_ENTRY(IPV6_MULTICAST_ADDRESS_TABLE),
    NCP_INSERT_PROP_HANDLER_ENTRY(THREAD_ASSISTING_PORTS),
#if OPENTHREAD_ENABLE_BORDER_ROUTER
    NCP_INSERT_PROP_HANDLER_ENTRY(THREAD_OFF_MESH_ROUTES),
    NCP_INSERT_PROP_HANDLER_ENTRY(THREAD_ON_MESH_NETS),
#endif
#if OPENTHREAD_ENABLE_MAC_FILTER
    NCP_INSERT_PROP_HANDLER_ENTRY(MAC_WHITELIST),
    NCP_INSERT_PROP_HANDLER_ENTRY(MAC_BLACKLIST),
    NCP_INSERT_PROP_HANDLER_ENTRY(MAC_FIXED_RSS),
#endif
#endif // OPENTHREAD_MTD || OPENTHREAD_FTD
#if OPENTHREAD_FTD
#if OPENTHREAD_ENABLE_COMMISSIONER
    NCP_INSERT_PROP_HANDLER_ENTRY(THREAD_JOINERS),
#endif
#endif // OPENTHREAD_FTD
};

#define NCP_REMOVE_PROP_HANDLER_ENTRY(name)  { SPINEL_PROP_##name, &NcpBase::RemovePropertyHandler_##name }

const NcpBase::PropertyHandlerEntry NcpBase::mRemovePropertyHandlerTable[] =
{
    NCP_REMOVE_PROP_HANDLER_ENTRY(UNSOL_UPDATE_FILTER),
#if OPENTHREAD_ENABLE_RAW_LINK_API
    NCP_REMOVE_PROP_HANDLER_ENTRY(MAC_SRC_MATCH_SHORT_ADDRESSES),
    NCP_REMOVE_PROP_HANDLER_ENTRY(MAC_SRC_MATCH_EXTENDED_ADDRESSES),
#endif
#if OPENTHREAD_MTD || OPENTHREAD_FTD
    NCP_REMOVE_PROP_HANDLER_ENTRY(IPV6_ADDRESS_TABLE),
    NCP_REMOVE_PROP_HANDLER_ENTRY(IPV6_MULTICAST_ADDRESS_TABLE),
#if OPENTHREAD_ENABLE_BORDER_ROUTER
    NCP_REMOVE_PROP_HANDLER_ENTRY(THREAD_OFF_MESH_ROUTES),
    NCP_REMOVE_PROP_HANDLER_ENTRY(THREAD_ON_MESH_NETS),
#endif
    NCP_REMOVE_PROP_HANDLER_ENTRY(THREAD_ASSISTING_PORTS),
#if OPENTHREAD_ENABLE_MAC_FILTER
    NCP_REMOVE_PROP_HANDLER_ENTRY(MAC_WHITELIST),
    NCP_REMOVE_PROP_HANDLER_ENTRY(MAC_BLACKLIST),
    NCP_REMOVE_PROP_HANDLER_ENTRY(MAC_FIXED_RSS),
#endif
#endif // OPENTHREAD_MTD || OPENTHREAD_FTD
#if OPENTHREAD_FTD
    NCP_REMOVE_PROP_HANDLER_ENTRY(THREAD_ACTIVE_ROUTER_IDS),
#endif
};

// ----------------------------------------------------------------------------
// MARK: Utility Functions
// ----------------------------------------------------------------------------

spinel_status_t NcpBase::ThreadErrorToSpinelStatus(otError aError)
{
    spinel_status_t ret;

    switch (aError)
    {
    case OT_ERROR_NONE:
        ret = SPINEL_STATUS_OK;
        break;

    case OT_ERROR_FAILED:
        ret = SPINEL_STATUS_FAILURE;
        break;

    case OT_ERROR_DROP:
        ret = SPINEL_STATUS_DROPPED;
        break;

    case OT_ERROR_NO_BUFS:
        ret = SPINEL_STATUS_NOMEM;
        break;

    case OT_ERROR_BUSY:
        ret = SPINEL_STATUS_BUSY;
        break;

    case OT_ERROR_PARSE:
        ret = SPINEL_STATUS_PARSE_ERROR;
        break;

    case OT_ERROR_INVALID_ARGS:
        ret = SPINEL_STATUS_INVALID_ARGUMENT;
        break;

    case OT_ERROR_NOT_IMPLEMENTED:
        ret = SPINEL_STATUS_UNIMPLEMENTED;
        break;

    case OT_ERROR_INVALID_STATE:
        ret = SPINEL_STATUS_INVALID_STATE;
        break;

    case OT_ERROR_NO_ACK:
        ret = SPINEL_STATUS_NO_ACK;
        break;

    case OT_ERROR_CHANNEL_ACCESS_FAILURE:
        ret = SPINEL_STATUS_CCA_FAILURE;
        break;

    case OT_ERROR_ALREADY:
        ret = SPINEL_STATUS_ALREADY;
        break;

    case OT_ERROR_NOT_FOUND:
        ret = SPINEL_STATUS_ITEM_NOT_FOUND;
        break;

    case OT_ERROR_DISABLED_FEATURE:
        ret = SPINEL_STATUS_INVALID_COMMAND_FOR_PROP;
        break;

    default:
        // Unknown error code. Wrap it as a Spinel status and return that.
        ret = static_cast<spinel_status_t>(SPINEL_STATUS_STACK_NATIVE__BEGIN + static_cast<uint32_t>(aError));
        break;
    }

    return ret;
}

static spinel_status_t ResetReasonToSpinelStatus(otPlatResetReason aReason)
{
    spinel_status_t ret;

    switch (aReason)
    {
    case OT_PLAT_RESET_REASON_POWER_ON:
        ret = SPINEL_STATUS_RESET_POWER_ON;
        break;

    case OT_PLAT_RESET_REASON_EXTERNAL:
        ret = SPINEL_STATUS_RESET_EXTERNAL;
        break;

    case OT_PLAT_RESET_REASON_SOFTWARE:
        ret = SPINEL_STATUS_RESET_SOFTWARE;
        break;

    case OT_PLAT_RESET_REASON_FAULT:
        ret = SPINEL_STATUS_RESET_FAULT;
        break;

    case OT_PLAT_RESET_REASON_CRASH:
        ret = SPINEL_STATUS_RESET_CRASH;
        break;

    case OT_PLAT_RESET_REASON_ASSERT:
        ret = SPINEL_STATUS_RESET_ASSERT;
        break;

    case OT_PLAT_RESET_REASON_WATCHDOG:
        ret = SPINEL_STATUS_RESET_WATCHDOG;
        break;

    case OT_PLAT_RESET_REASON_OTHER:
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

NcpBase *NcpBase::sNcpInstance = NULL;

NcpBase::NcpBase(otInstance *aInstance):
    mInstance(aInstance),
    mTxFrameBuffer(mTxBuffer, sizeof(mTxBuffer)),
    mEncoder(mTxFrameBuffer),
    mDecoder(),
    mHostPowerStateInProgress(false),
    mLastStatus(SPINEL_STATUS_OK),
    mSupportedChannelMask(OT_RADIO_SUPPORTED_CHANNELS),
    mChannelMask(OT_RADIO_SUPPORTED_CHANNELS),
    mScanPeriod(200), // ms
    mDiscoveryScanJoinerFlag(false),
    mDiscoveryScanEnableFiltering(false),
    mDiscoveryScanPanId(0xffff),
    mUpdateChangedPropsTask(*aInstance, &NcpBase::UpdateChangedProps, this),
    mThreadChangedFlags(0),
    mChangedPropsSet(),
    mHostPowerState(SPINEL_HOST_POWER_STATE_ONLINE),
    mHostPowerReplyFrameTag(NcpFrameBuffer::kInvalidTag),
    mHostPowerStateHeader(0),
#if OPENTHREAD_CONFIG_NCP_ENABLE_PEEK_POKE
    mAllowPeekDelegate(NULL),
    mAllowPokeDelegate(NULL),
#endif
    mDroppedReplyTid(0),
    mDroppedReplyTidBitSet(0),
    mNextExpectedTid(0),
    mAllowLocalNetworkDataChange(false),
    mRequireJoinExistingNetwork(false),
    mIsRawStreamEnabled(false),
    mDisableStreamWrite(false),
#if OPENTHREAD_ENABLE_RAW_LINK_API
    mCurTransmitTID(0),
    mCurReceiveChannel(OPENTHREAD_CONFIG_DEFAULT_CHANNEL),
    mCurScanChannel(kInvalidScanChannel),
#endif // OPENTHREAD_ENABLE_RAW_LINK_API
#if OPENTHREAD_MTD || OPENTHREAD_FTD
    mInboundSecureIpFrameCounter(0),
    mInboundInsecureIpFrameCounter(0),
    mOutboundSecureIpFrameCounter(0),
    mOutboundInsecureIpFrameCounter(0),
    mDroppedOutboundIpFrameCounter(0),
    mDroppedInboundIpFrameCounter(0),
#endif // OPENTHREAD_MTD || OPENTHREAD_FTD
    mFramingErrorCounter(0),
    mRxSpinelFrameCounter(0),
    mRxSpinelOutOfOrderTidCounter(0),
    mTxSpinelFrameCounter(0)
{
    assert(mInstance != NULL);

    sNcpInstance = this;

    mTxFrameBuffer.SetFrameRemovedCallback(&NcpBase::HandleFrameRemovedFromNcpBuffer, this);

#if OPENTHREAD_MTD || OPENTHREAD_FTD
    otMessageQueueInit(&mMessageQueue);
    otSetStateChangedCallback(mInstance, &NcpBase::HandleNetifStateChanged, this);
    otIp6SetReceiveCallback(mInstance, &NcpBase::HandleDatagramFromStack, this);
    otIp6SetReceiveFilterEnabled(mInstance, true);
    otLinkSetPcapCallback(mInstance, &NcpBase::HandleRawFrame, static_cast<void *>(this));
    otIcmp6SetEchoEnabled(mInstance, false);
#if OPENTHREAD_ENABLE_LEGACY
    mLegacyNodeDidJoin = false;
    mLegacyHandlers = NULL;
    memset(mLegacyUlaPrefix, 0, sizeof(mLegacyUlaPrefix));
    memset(&mLegacyLastJoinedNode, 0, sizeof(mLegacyLastJoinedNode));
#endif
#endif // OPENTHREAD_MTD || OPENTHREAD_FTD
    mChangedPropsSet.AddLastStatus(SPINEL_STATUS_RESET_UNKNOWN);
    mUpdateChangedPropsTask.Post();
}

NcpBase *NcpBase::GetNcpInstance(void)
{
    return sNcpInstance;
}

// ----------------------------------------------------------------------------
// MARK: Serial Traffic Glue
// ----------------------------------------------------------------------------

NcpFrameBuffer::FrameTag NcpBase::GetLastOutboundFrameTag(void)
{
    return mTxFrameBuffer.InFrameGetLastTag();
}

void NcpBase::HandleReceive(const uint8_t *aBuf, uint16_t aBufLength)
{
    otError parseError = OT_ERROR_NONE;
    otError responseError = OT_ERROR_NONE;
    uint8_t header = 0;
    unsigned int command = 0;
    spinel_tid_t tid = 0;

    // Initialize the decoder with the newly received spinel frame.
    mDecoder.Init(aBuf, aBufLength);

    // Receiving any message from the host has the side effect of transitioning the host power state to online.
    mHostPowerState = SPINEL_HOST_POWER_STATE_ONLINE;
    mHostPowerStateInProgress = false;

    SuccessOrExit(parseError = mDecoder.ReadUint8(header));
    SuccessOrExit(parseError = mDecoder.ReadUintPacked(command));

    responseError = HandleCommand(header, command);

    tid = SPINEL_HEADER_GET_TID(header);

    // Check if we may have missed a `tid` in the sequence.
    if ((mNextExpectedTid != 0) && (tid != mNextExpectedTid))
    {
        mRxSpinelOutOfOrderTidCounter++;
    }

    mNextExpectedTid = SPINEL_GET_NEXT_TID(tid);

exit:

    if (parseError != OT_ERROR_NONE)
    {
        responseError = SendLastStatus(header, SPINEL_STATUS_PARSE_ERROR);
    }

    if (responseError == OT_ERROR_NO_BUFS)
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

void NcpBase::HandleFrameRemovedFromNcpBuffer(void *aContext, NcpFrameBuffer::FrameTag aFrameTag,
                                              NcpFrameBuffer::Priority aPriority, NcpFrameBuffer *aNcpBuffer)
{
    OT_UNUSED_VARIABLE(aNcpBuffer);
    OT_UNUSED_VARIABLE(aPriority);

    static_cast<NcpBase *>(aContext)->HandleFrameRemovedFromNcpBuffer(aFrameTag);
}

void NcpBase::HandleFrameRemovedFromNcpBuffer(NcpFrameBuffer::FrameTag aFrameTag)
{
    if (mHostPowerStateInProgress == true)
    {
        if (aFrameTag == mHostPowerReplyFrameTag)
        {
            mHostPowerStateInProgress = false;
        }
    }

    // Space is now available in ncp tx frame buffer.

    while (mDroppedReplyTid != 0)
    {
        SuccessOrExit(
            SendLastStatus(
                SPINEL_HEADER_FLAG | SPINEL_HEADER_IID_0 | mDroppedReplyTid,
                SPINEL_STATUS_NOMEM
            ));

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

    if (mHostPowerStateHeader)
    {
        SuccessOrExit(
            HandleCommandPropertyGet(
                mHostPowerStateHeader,
                SPINEL_PROP_HOST_POWER_STATE
            ));

        mHostPowerStateHeader = 0;

        if (mHostPowerState != SPINEL_HOST_POWER_STATE_ONLINE)
        {
            mHostPowerReplyFrameTag = GetLastOutboundFrameTag();
            mHostPowerStateInProgress = true;
        }
    }

#if OPENTHREAD_MTD || OPENTHREAD_FTD
    SuccessOrExit(SendQueuedDatagramMessages());
#endif

    UpdateChangedProps();

exit:
    return;
}

bool NcpBase::ShouldWakeHost(void)
{
    return (mHostPowerState != SPINEL_HOST_POWER_STATE_ONLINE && !mHostPowerStateInProgress);
}

bool NcpBase::ShouldDeferHostSend(void)
{
    return (mHostPowerState == SPINEL_HOST_POWER_STATE_DEEP_SLEEP && !mHostPowerStateInProgress);
}

void NcpBase::IncrementFrameErrorCounter(void)
{
    mFramingErrorCounter++;
}

otError NcpBase::StreamWrite(int aStreamId, const uint8_t *aDataPtr, int aDataLen)
{
    otError error = OT_ERROR_NONE;
    uint8_t header = SPINEL_HEADER_FLAG | SPINEL_HEADER_IID_0;
    spinel_prop_key_t streamPropKey;

    if (aStreamId == 0)
    {
        streamPropKey = SPINEL_PROP_STREAM_DEBUG;
    }
    else
    {
        streamPropKey = static_cast<spinel_prop_key_t>(aStreamId);
    }

    VerifyOrExit(!mDisableStreamWrite, error = OT_ERROR_INVALID_STATE);
    VerifyOrExit(!mChangedPropsSet.IsPropertyFiltered(streamPropKey), error = OT_ERROR_INVALID_STATE);

    SuccessOrExit(error = mEncoder.BeginFrame(header, SPINEL_CMD_PROP_VALUE_IS, streamPropKey));
    SuccessOrExit(error = mEncoder.WriteData(aDataPtr, static_cast<uint16_t>(aDataLen)));
    SuccessOrExit(error = mEncoder.EndFrame());

exit:

    if (error == OT_ERROR_NO_BUFS)
    {
        mChangedPropsSet.AddLastStatus(SPINEL_STATUS_NOMEM);
        mUpdateChangedPropsTask.Post();
    }

    return error;
}

#if OPENTHREAD_CONFIG_NCP_ENABLE_PEEK_POKE

void NcpBase::RegisterPeekPokeDelagates(otNcpDelegateAllowPeekPoke aAllowPeekDelegate,
                                        otNcpDelegateAllowPeekPoke aAllowPokeDelegate)
{
    mAllowPeekDelegate = aAllowPeekDelegate;
    mAllowPokeDelegate = aAllowPokeDelegate;
}

#endif // OPENTHREAD_CONFIG_NCP_ENABLE_PEEK_POKE

// ----------------------------------------------------------------------------
// MARK: Raw frame handling
// ----------------------------------------------------------------------------

void NcpBase::HandleRawFrame(const otRadioFrame *aFrame, void *aContext)
{
    static_cast<NcpBase *>(aContext)->HandleRawFrame(aFrame);
}

void NcpBase::HandleRawFrame(const otRadioFrame *aFrame)
{
    uint16_t flags = 0;
    uint8_t header = SPINEL_HEADER_FLAG | SPINEL_HEADER_IID_0;

    if (!mIsRawStreamEnabled)
    {
        goto exit;
    }

    if (aFrame->mDidTX)
    {
        flags |= SPINEL_MD_FLAG_TX;
    }

    // Append frame header and frame length
    SuccessOrExit(mEncoder.BeginFrame(header, SPINEL_CMD_PROP_VALUE_IS, SPINEL_PROP_STREAM_RAW));
    SuccessOrExit(mEncoder.WriteUint16(aFrame->mLength));

    // Append the frame contents
    SuccessOrExit(mEncoder.WriteData(aFrame->mPsdu, aFrame->mLength));

    // Append metadata (rssi, etc)
    SuccessOrExit(mEncoder.WriteInt8(aFrame->mPower)); // TX Power
    SuccessOrExit(mEncoder.WriteInt8(-128));           // Noise floor (Currently unused)
    SuccessOrExit(mEncoder.WriteUint16(flags));        // Flags

    SuccessOrExit(mEncoder.OpenStruct());              // PHY-data
    // Empty for now
    SuccessOrExit(mEncoder.CloseStruct());

    SuccessOrExit(mEncoder.OpenStruct());              // Vendor-data
    // Empty for now
    SuccessOrExit(mEncoder.CloseStruct());

    SuccessOrExit(mEncoder.EndFrame());

exit:
    return;
}

// ----------------------------------------------------------------------------
// MARK: Property/Status Changed
// ----------------------------------------------------------------------------

void NcpBase::UpdateChangedProps(Tasklet &aTasklet)
{
    OT_UNUSED_VARIABLE(aTasklet);
    GetNcpInstance()->UpdateChangedProps();
}

void NcpBase::UpdateChangedProps(void)
{
    uint8_t numEntries;
    spinel_prop_key_t propKey;
    const ChangedPropsSet::Entry *entry;

#if OPENTHREAD_MTD || OPENTHREAD_FTD
    ProcessThreadChangedFlags();
#endif

    VerifyOrExit(!mChangedPropsSet.IsEmpty());

    entry = mChangedPropsSet.GetSupportedEntries(numEntries);

    for (uint8_t index = 0; index < numEntries; index++, entry++)
    {
        if (!mChangedPropsSet.IsEntryChanged(index))
        {
            continue;
        }

        propKey = entry->mPropKey;

        if (propKey == SPINEL_PROP_LAST_STATUS)
        {
            spinel_status_t status = entry->mStatus;

            if (status == SPINEL_STATUS_RESET_UNKNOWN)
            {
                status = ResetReasonToSpinelStatus(otPlatGetResetReason(mInstance));
            }

            SuccessOrExit(SendLastStatus(SPINEL_HEADER_FLAG | SPINEL_HEADER_IID_0, status));
        }
        else
        {
            SuccessOrExit(HandleCommandPropertyGet(SPINEL_HEADER_FLAG | SPINEL_HEADER_IID_0, propKey));
        }

        mChangedPropsSet.RemoveEntry(index);
        VerifyOrExit(!mChangedPropsSet.IsEmpty());
    }

exit:
    return;
}

// ----------------------------------------------------------------------------
// MARK: Inbound Command Handler
// ----------------------------------------------------------------------------

otError NcpBase::HandleCommand(uint8_t aHeader, unsigned int aCommand)
{
    otError error = OT_ERROR_NONE;

    // Skip if this isn't a spinel frame
    VerifyOrExit((SPINEL_HEADER_FLAG & aHeader) == SPINEL_HEADER_FLAG, error = OT_ERROR_INVALID_ARGS);

    mDisableStreamWrite = true;

    // We only support IID zero for now.
    VerifyOrExit(SPINEL_HEADER_GET_IID(aHeader) == 0, error = SendLastStatus(aHeader, SPINEL_STATUS_INVALID_INTERFACE));

    switch (aCommand)
    {
    case SPINEL_CMD_NOOP:
        error = CommandHandler_NOOP(aHeader);
        break;

    case SPINEL_CMD_RESET:
        error = CommandHandler_RESET(aHeader);
        break;

    case SPINEL_CMD_PROP_VALUE_GET:
    case SPINEL_CMD_PROP_VALUE_SET:
    case SPINEL_CMD_PROP_VALUE_INSERT:
    case SPINEL_CMD_PROP_VALUE_REMOVE:
        error = CommandHandler_PROP_VALUE_update(aHeader, aCommand);
        break;

#if OPENTHREAD_CONFIG_NCP_ENABLE_PEEK_POKE
    case SPINEL_CMD_PEEK:
        error = CommandHandler_PEEK(aHeader);
        break;

    case SPINEL_CMD_POKE:
        error = CommandHandler_POKE(aHeader);
        break;
#endif

#if OPENTHREAD_MTD || OPENTHREAD_FTD
    case SPINEL_CMD_NET_SAVE:
        error = CommandHandler_NET_SAVE(aHeader);
        break;

    case SPINEL_CMD_NET_CLEAR:
        error = CommandHandler_NET_CLEAR(aHeader);
        break;

    case SPINEL_CMD_NET_RECALL:
        error = CommandHandler_NET_RECALL(aHeader);
        break;
#endif // OPENTHREAD_MTD || OPENTHREAD_FTD

    default:

#if OPENTHREAD_ENABLE_SPINEL_VENDOR_SUPPORT
        if (aCommand >= SPINEL_CMD_VENDOR__BEGIN && aCommand < SPINEL_CMD_VENDOR__END)
        {
            error = VendorCommandHandler(aHeader, aCommand);
            break;
        }
#endif

        error = SendLastStatus(aHeader, SPINEL_STATUS_INVALID_COMMAND);
        break;
    }

exit:
    mDisableStreamWrite = false;
    return error;
}

// ----------------------------------------------------------------------------
// MARK: Property Get/Set/Insert/Remove Commands
// ----------------------------------------------------------------------------

NcpBase::PropertyHandler NcpBase::FindPropertyHandler(spinel_prop_key_t aKey, const PropertyHandlerEntry *aTableEntry,
                                                      size_t aTableLen)
{
    PropertyHandler handler = NULL;

    while (aTableLen--)
    {
        if (aTableEntry->mPropKey == aKey)
        {
            handler = aTableEntry->mHandler;
            break;
        }

        aTableEntry++;
    }

    return handler;
}

NcpBase::PropertyHandler NcpBase::FindGetPropertyHandler(spinel_prop_key_t aKey)
{
    return FindPropertyHandler(
               aKey,
               mGetPropertyHandlerTable,
               sizeof(mGetPropertyHandlerTable) / sizeof(mGetPropertyHandlerTable[0])
           );

}

NcpBase::PropertyHandler NcpBase::FindSetPropertyHandler(spinel_prop_key_t aKey)
{
    return FindPropertyHandler(
               aKey,
               mSetPropertyHandlerTable,
               sizeof(mSetPropertyHandlerTable) / sizeof(mSetPropertyHandlerTable[0])
           );
}

NcpBase::PropertyHandler NcpBase::FindInsertPropertyHandler(spinel_prop_key_t aKey)
{
   return FindPropertyHandler(
               aKey,
               mInsertPropertyHandlerTable,
               sizeof(mInsertPropertyHandlerTable) / sizeof(mInsertPropertyHandlerTable[0])
           );
}

NcpBase::PropertyHandler NcpBase::FindRemovePropertyHandler(spinel_prop_key_t aKey)
{
    return FindPropertyHandler(
               aKey,
               mRemovePropertyHandlerTable,
               sizeof(mRemovePropertyHandlerTable) / sizeof(mRemovePropertyHandlerTable[0])
           );
}

otError NcpBase::HandleCommandPropertyGet(uint8_t aHeader, spinel_prop_key_t aKey)
{
    otError error = OT_ERROR_NONE;
    PropertyHandler handler = FindGetPropertyHandler(aKey);

    VerifyOrExit(handler != NULL, error = SendLastStatus(aHeader, SPINEL_STATUS_PROP_NOT_FOUND));

    SuccessOrExit(error = mEncoder.BeginFrame(aHeader, SPINEL_CMD_PROP_VALUE_IS, aKey));
    SuccessOrExit(error = (this->*handler)());
    SuccessOrExit(error = mEncoder.EndFrame());

exit:
    return error;
}

// Returns `true` and updates the `aError` on success.
bool NcpBase::HandlePropertySetForSpecialProperties(uint8_t aHeader, spinel_prop_key_t aKey, otError &aError)
{
    bool didHandle = true;

    // Here the properties that require special treatment are handled.
    // These properties are expected to form the response from their
    // set handler directly.

    switch (aKey)
    {
    case SPINEL_PROP_HOST_POWER_STATE:
        ExitNow(aError = SetPropertyHandler_HOST_POWER_STATE(aHeader));

#if OPENTHREAD_ENABLE_DIAG
    case SPINEL_PROP_NEST_STREAM_MFG:
        ExitNow(aError = SetPropertyHandler_NEST_STREAM_MFG(aHeader));
#endif

#if OPENTHREAD_FTD && OPENTHREAD_ENABLE_COMMISSIONER
    case SPINEL_PROP_THREAD_COMMISSIONER_ENABLED:
        ExitNow(aError = SetPropertyHandler_THREAD_COMMISSIONER_ENABLED(aHeader));
#endif

#if OPENTHREAD_ENABLE_RAW_LINK_API
    case SPINEL_PROP_STREAM_RAW:
        ExitNow(aError = SetPropertyHandler_STREAM_RAW(aHeader));
#endif

    default:
        didHandle = false;
        break;
    }

exit:
    return didHandle;
}

otError NcpBase::HandleCommandPropertySet(uint8_t aHeader, spinel_prop_key_t aKey)
{
    otError error = OT_ERROR_NONE;
    PropertyHandler handler = FindSetPropertyHandler(aKey);
    const uint8_t *valuePtr;
    uint16_t valueLen;

    if (handler == NULL)
    {
        // If there is no "set" handler, check if this property is one of the
        // ones that require different treatment.

        bool didHandle = HandlePropertySetForSpecialProperties(aHeader, aKey, error);

        VerifyOrExit(!didHandle);

        ExitNow(error = SendLastStatus(aHeader, SPINEL_STATUS_PROP_NOT_FOUND));
    }

    // Save current read position in the decoder. Read the entire
    // content as a data blob (which can be used in forming the response
    // if there is no associated get handler), then reset the read
    // position back so that the handler method can parse the
    // content.

    mDecoder.SavePosition();
    mDecoder.ReadData(valuePtr, valueLen);
    mDecoder.ResetToSaved();

    error = (this->*handler)();

    VerifyOrExit(error == OT_ERROR_NONE, error = SendLastStatus(aHeader, ThreadErrorToSpinelStatus(error)));

    // Prepare the response.

    if ((handler = FindGetPropertyHandler(aKey)) != NULL)
    {
        SuccessOrExit(error = mEncoder.BeginFrame(aHeader, SPINEL_CMD_PROP_VALUE_IS, aKey));
        SuccessOrExit(error = (this->*handler)());
        SuccessOrExit(error = mEncoder.EndFrame());
    }
    else
    {
        if ((aKey == SPINEL_PROP_STREAM_NET) || (aKey ==  SPINEL_PROP_STREAM_NET_INSECURE)
#if OPENTHREAD_FTD && OPENTHREAD_ENABLE_TMF_PROXY
            || (aKey == SPINEL_PROP_THREAD_TMF_PROXY_STREAM)
#endif
           )
        {
            // Only send a successful status for `STREAM` properties
            // if the transaction id (TID) is non-zero.

            if (SPINEL_HEADER_GET_TID(aHeader) != 0)
            {
                error = SendLastStatus(aHeader, SPINEL_STATUS_OK);
            }

            ExitNow();
        }

        // If there is no get handler for this property, echo the same
        // value from input frame in the response.

        SuccessOrExit(error = mEncoder.BeginFrame(aHeader, SPINEL_CMD_PROP_VALUE_IS, aKey));
        SuccessOrExit(error = mEncoder.WriteData(valuePtr, valueLen));
        SuccessOrExit(error = mEncoder.EndFrame());
    }

exit:
    return error;
}

otError NcpBase::HandleCommandPropertyInsertRemove(uint8_t aHeader, spinel_prop_key_t aKey, unsigned int aCommand)
{
    otError error = OT_ERROR_NONE;
    PropertyHandler handler = NULL;
    unsigned int responseCommand = 0;
    const uint8_t *valuePtr;
    uint16_t valueLen;

    switch (aCommand)
    {
    case SPINEL_CMD_PROP_VALUE_INSERT:
        handler = FindInsertPropertyHandler(aKey);
        responseCommand = SPINEL_CMD_PROP_VALUE_INSERTED;
        break;

    case SPINEL_CMD_PROP_VALUE_REMOVE:
        handler = FindRemovePropertyHandler(aKey);
        responseCommand = SPINEL_CMD_PROP_VALUE_REMOVED;
        break;

    default:
        assert(false);
        break;
    }

    VerifyOrExit(handler != NULL, error = SendLastStatus(aHeader, SPINEL_STATUS_PROP_NOT_FOUND));

    // Save current read position in the decoder. Read the entire
    // content as a data blob (which is used in forming the response
    // in case of success), then reset the read position back so
    // that the `PropertyHandler` method can parse the content.

    mDecoder.SavePosition();
    mDecoder.ReadData(valuePtr, valueLen);
    mDecoder.ResetToSaved();

    error = (this->*handler)();

    VerifyOrExit(error == OT_ERROR_NONE, error = SendLastStatus(aHeader, ThreadErrorToSpinelStatus(error)));

    // Prepare the response
    SuccessOrExit(error = mEncoder.BeginFrame(aHeader, responseCommand, aKey));
    SuccessOrExit(error = mEncoder.WriteData(valuePtr, valueLen));
    SuccessOrExit(error = mEncoder.EndFrame());

exit:
    return error;
}

// ----------------------------------------------------------------------------
// MARK: Outbound Frame Methods
// ----------------------------------------------------------------------------

otError NcpBase::SendLastStatus(uint8_t aHeader, spinel_status_t aLastStatus)
{
    otError error = OT_ERROR_NONE;

    if (SPINEL_HEADER_GET_IID(aHeader) == 0)
    {
        mLastStatus = aLastStatus;
    }

    SuccessOrExit(error = mEncoder.BeginFrame(aHeader, SPINEL_CMD_PROP_VALUE_IS, SPINEL_PROP_LAST_STATUS));
    SuccessOrExit(error = mEncoder.WriteUintPacked(aLastStatus));
    SuccessOrExit(error = mEncoder.EndFrame());

exit:
    return error;
}

// ----------------------------------------------------------------------------
// MARK: Individual Command Handlers
// ----------------------------------------------------------------------------

otError NcpBase::CommandHandler_NOOP(uint8_t aHeader)
{
    return SendLastStatus(aHeader, SPINEL_STATUS_OK);
}

otError NcpBase::CommandHandler_RESET(uint8_t aHeader)
{
    otError error = OT_ERROR_NONE;

    OT_UNUSED_VARIABLE(aHeader);

    // Signal a platform reset. If implemented, this function
    // shouldn't return.
    otInstanceReset(mInstance);

#if OPENTHREAD_MTD || OPENTHREAD_FTD
    // We only get to this point if the
    // platform doesn't support resetting.
    // In such a case we fake it.

    otThreadSetEnabled(mInstance, false);
    otIp6SetEnabled(mInstance, false);
#endif

    error = SendLastStatus(SPINEL_HEADER_FLAG | SPINEL_HEADER_IID_0, SPINEL_STATUS_RESET_SOFTWARE);

    if (error != OT_ERROR_NONE)
    {
        mChangedPropsSet.AddLastStatus(SPINEL_STATUS_RESET_UNKNOWN);
        mUpdateChangedPropsTask.Post();
    }

    return error;
}

otError NcpBase::CommandHandler_PROP_VALUE_update(uint8_t aHeader, unsigned int aCommand)
{
    otError error = OT_ERROR_NONE;
    unsigned int propKey = 0;

    error = mDecoder.ReadUintPacked(propKey);

    VerifyOrExit(error == OT_ERROR_NONE, error = SendLastStatus(aHeader, ThreadErrorToSpinelStatus(error)));

    switch (aCommand)
    {
    case SPINEL_CMD_PROP_VALUE_GET:
        error = HandleCommandPropertyGet(aHeader, static_cast<spinel_prop_key_t>(propKey));
        break;

    case SPINEL_CMD_PROP_VALUE_SET:
        error = HandleCommandPropertySet(aHeader, static_cast<spinel_prop_key_t>(propKey));
        break;

    case SPINEL_CMD_PROP_VALUE_INSERT:
    case SPINEL_CMD_PROP_VALUE_REMOVE:
        error = HandleCommandPropertyInsertRemove(aHeader, static_cast<spinel_prop_key_t>(propKey), aCommand);
        break;

    default:
        break;
    }

exit:
    return error;
}

#if OPENTHREAD_CONFIG_NCP_ENABLE_PEEK_POKE

otError NcpBase::CommandHandler_PEEK(uint8_t aHeader)
{
    otError parseError = OT_ERROR_NONE;
    otError responseError = OT_ERROR_NONE;
    uint32_t address;
    uint16_t count;

    SuccessOrExit(parseError = mDecoder.ReadUint32(address));
    SuccessOrExit(parseError = mDecoder.ReadUint16(count));

    VerifyOrExit(count != 0, parseError = OT_ERROR_INVALID_ARGS);

    if (mAllowPeekDelegate != NULL)
    {
        VerifyOrExit(mAllowPeekDelegate(address, count), parseError = OT_ERROR_INVALID_ARGS);
    }

    SuccessOrExit(responseError = mEncoder.BeginFrame(aHeader, SPINEL_CMD_PEEK_RET));
    SuccessOrExit(responseError = mEncoder.WriteUint32(address));
    SuccessOrExit(responseError = mEncoder.WriteUint16(count));
    SuccessOrExit(responseError = mEncoder.WriteData(reinterpret_cast<const uint8_t *>(address), count));
    SuccessOrExit(responseError = mEncoder.EndFrame());

exit:
    if (parseError != OT_ERROR_NONE)
    {
        responseError = SendLastStatus(aHeader, ThreadErrorToSpinelStatus(parseError));
    }

    return responseError;
}

otError NcpBase::CommandHandler_POKE(uint8_t aHeader)
{
    otError parseError = OT_ERROR_NONE;
    uint32_t address;
    uint16_t count;
    const uint8_t *dataPtr = NULL;
    uint16_t dataLen;

    SuccessOrExit(parseError = mDecoder.ReadUint32(address));
    SuccessOrExit(parseError = mDecoder.ReadUint16(count));
    SuccessOrExit(parseError = mDecoder.ReadData(dataPtr, dataLen));

    VerifyOrExit(count != 0, parseError = OT_ERROR_INVALID_ARGS);
    VerifyOrExit(count <= dataLen, parseError = OT_ERROR_INVALID_ARGS);

    if (mAllowPokeDelegate != NULL)
    {
        VerifyOrExit(mAllowPokeDelegate(address, count), parseError = OT_ERROR_INVALID_ARGS);
    }

    memcpy(reinterpret_cast<uint8_t *>(address), dataPtr, count);

exit:
    return SendLastStatus(aHeader, ThreadErrorToSpinelStatus(parseError));
}

#endif // OPENTHREAD_CONFIG_NCP_ENABLE_PEEK_POKE


// ----------------------------------------------------------------------------
// MARK: Individual Property Getters and Setters
// ----------------------------------------------------------------------------

otError NcpBase::GetPropertyHandler_PHY_ENABLED(void)
{
#if OPENTHREAD_ENABLE_RAW_LINK_API
        return mEncoder.WriteBool(otLinkRawIsEnabled(mInstance));
#else
        return mEncoder.WriteBool(false);
#endif
}

otError NcpBase::GetPropertyHandler_PHY_CHAN(void)
{
    return mEncoder.WriteUint8(otLinkGetChannel(mInstance));
}

otError NcpBase::SetPropertyHandler_PHY_CHAN(void)
{
    unsigned int channel = 0;
    otError error = OT_ERROR_NONE;

    SuccessOrExit(error = mDecoder.ReadUintPacked(channel));

#if OPENTHREAD_MTD || OPENTHREAD_FTD
    error = otLinkSetChannel(mInstance, static_cast<uint8_t>(channel));
#endif

#if OPENTHREAD_ENABLE_RAW_LINK_API

    SuccessOrExit(error);

    // Cache the channel. If the raw link layer isn't enabled yet, the otSetChannel call
    // doesn't call into the radio layer to set the channel. We will have to do it
    // manually whenever the radios are enabled and/or raw stream is enabled.
    mCurReceiveChannel = static_cast<uint8_t>(channel);

    // Make sure we are update the receiving channel if raw link is enabled and we have raw
    // stream enabled already
    if (otLinkRawIsEnabled(mInstance) && mIsRawStreamEnabled)
    {
        error = otLinkRawReceive(mInstance, mCurReceiveChannel, &NcpBase::LinkRawReceiveDone);
    }

#endif // OPENTHREAD_ENABLE_RAW_LINK_API

exit:
    return error;
}

otError NcpBase::GetPropertyHandler_MAC_PROMISCUOUS_MODE(void)
{
   return mEncoder.WriteUint8(otPlatRadioGetPromiscuous(mInstance)
                        ? SPINEL_MAC_PROMISCUOUS_MODE_FULL
                        : SPINEL_MAC_PROMISCUOUS_MODE_OFF
                    );
}

otError NcpBase::SetPropertyHandler_MAC_PROMISCUOUS_MODE(void)
{
    uint8_t mode = 0;
    otError error = OT_ERROR_NONE;

    SuccessOrExit(error = mDecoder.ReadUint8(mode));

    switch (mode)
    {
    case SPINEL_MAC_PROMISCUOUS_MODE_OFF:
        otPlatRadioSetPromiscuous(mInstance, false);
        break;

    case SPINEL_MAC_PROMISCUOUS_MODE_NETWORK:
    case SPINEL_MAC_PROMISCUOUS_MODE_FULL:
        otPlatRadioSetPromiscuous(mInstance, true);
        break;

    default:
        error = OT_ERROR_INVALID_ARGS;
        break;
    }

exit:
    return error;
}

otError NcpBase::GetPropertyHandler_MAC_15_4_PANID(void)
{
    return mEncoder.WriteUint16(otLinkGetPanId(mInstance));
}

otError NcpBase::SetPropertyHandler_MAC_15_4_PANID(void)
{
    uint16_t panid;
    otError error = OT_ERROR_NONE;

    SuccessOrExit(error = mDecoder.ReadUint16(panid));

    error = otLinkSetPanId(mInstance, panid);

exit:
    return error;
}

otError NcpBase::GetPropertyHandler_MAC_15_4_LADDR(void)
{
    return mEncoder.WriteEui64(*otLinkGetExtendedAddress(mInstance));
}

otError NcpBase::SetPropertyHandler_MAC_15_4_LADDR(void)
{
    const otExtAddress *extAddress;
    otError error = OT_ERROR_NONE;

    SuccessOrExit(error = mDecoder.ReadEui64(extAddress));

    error = otLinkSetExtendedAddress(mInstance, extAddress);

exit:
    return error;
}

otError NcpBase::GetPropertyHandler_MAC_15_4_SADDR(void)
{
    return mEncoder.WriteUint16(otLinkGetShortAddress(mInstance));
}

otError NcpBase::GetPropertyHandler_MAC_RAW_STREAM_ENABLED(void)
{
    return mEncoder.WriteBool(mIsRawStreamEnabled);
}

otError NcpBase::SetPropertyHandler_MAC_RAW_STREAM_ENABLED(void)
{
    bool enabled = false;
    otError error = OT_ERROR_NONE;

    SuccessOrExit(error = mDecoder.ReadBool(enabled));

#if OPENTHREAD_ENABLE_RAW_LINK_API

    if (otLinkRawIsEnabled(mInstance))
    {
        if (enabled)
        {
            error = otLinkRawReceive(mInstance, mCurReceiveChannel, &NcpBase::LinkRawReceiveDone);
        }
        else
        {
            error = otLinkRawSleep(mInstance);
        }
    }

#endif // OPENTHREAD_ENABLE_RAW_LINK_API

    mIsRawStreamEnabled = enabled;

exit:
    return error;
}

otError NcpBase::GetPropertyHandler_UNSOL_UPDATE_FILTER(void)
{
    otError error = OT_ERROR_NONE;
    uint8_t numEntries;
    const ChangedPropsSet::Entry *entry;

    entry = mChangedPropsSet.GetSupportedEntries(numEntries);

    for (uint8_t index = 0; index < numEntries; index++, entry++)
    {
        if (mChangedPropsSet.IsEntryFiltered(index))
        {
            SuccessOrExit(error = mEncoder.WriteUintPacked(entry->mPropKey));
        }
    }

exit:
    return error;
}

otError NcpBase::SetPropertyHandler_UNSOL_UPDATE_FILTER(void)
{
    unsigned int propKey;
    otError error = OT_ERROR_NONE;

    // First clear the current filter.
    mChangedPropsSet.ClearFilter();

    while (mDecoder.GetRemainingLengthInStruct() > 0)
    {
        SuccessOrExit(mDecoder.ReadUintPacked(propKey));

        IgnoreReturnValue(mChangedPropsSet.EnablePropertyFilter(static_cast<spinel_prop_key_t>(propKey), true));
    }

exit:
    // If we had an error, we may have actually changed
    // the state of the filter, So we need to report
    // those incomplete changes via an asynchronous
    // change event.

    if (error != OT_ERROR_NONE)
    {
        HandleCommandPropertyGet(SPINEL_HEADER_FLAG | SPINEL_HEADER_IID_0, SPINEL_PROP_UNSOL_UPDATE_FILTER);
    }

    return error;
}

otError NcpBase::InsertPropertyHandler_UNSOL_UPDATE_FILTER(void)
{
    otError error = OT_ERROR_NONE;
    unsigned int propKey;

    SuccessOrExit(error = mDecoder.ReadUintPacked(propKey));

    error = mChangedPropsSet.EnablePropertyFilter(static_cast<spinel_prop_key_t>(propKey), true);

exit:
    return error;
}

otError NcpBase::RemovePropertyHandler_UNSOL_UPDATE_FILTER(void)
{
    otError error = OT_ERROR_NONE;
    unsigned int propKey;

    SuccessOrExit(error = mDecoder.ReadUintPacked(propKey));

    error = mChangedPropsSet.EnablePropertyFilter(static_cast<spinel_prop_key_t>(propKey), false);

exit:
    return error;
}

otError NcpBase::GetPropertyHandler_LAST_STATUS(void)
{
    return mEncoder.WriteUintPacked(mLastStatus);
}

otError NcpBase::GetPropertyHandler_PROTOCOL_VERSION(void)
{
    otError error = OT_ERROR_NONE;

    SuccessOrExit(error = mEncoder.WriteUintPacked(SPINEL_PROTOCOL_VERSION_THREAD_MAJOR));
    SuccessOrExit(error = mEncoder.WriteUintPacked(SPINEL_PROTOCOL_VERSION_THREAD_MINOR));

exit:
    return error;
}

otError NcpBase::GetPropertyHandler_INTERFACE_TYPE(void)
{
    return mEncoder.WriteUintPacked(SPINEL_PROTOCOL_TYPE_THREAD);
}

otError NcpBase::GetPropertyHandler_VENDOR_ID(void)
{
    return mEncoder.WriteUintPacked(0);   // Vendor ID. Zero for unknown.
}

otError NcpBase::GetPropertyHandler_CAPS(void)
{
    otError error = OT_ERROR_NONE;

    SuccessOrExit(error = mEncoder.WriteUintPacked(SPINEL_CAP_COUNTERS));
    SuccessOrExit(error = mEncoder.WriteUintPacked(SPINEL_CAP_UNSOL_UPDATE_FILTER));

#if OPENTHREAD_ENABLE_RAW_LINK_API
    SuccessOrExit(error = mEncoder.WriteUintPacked(SPINEL_CAP_MAC_RAW));
#endif

#if OPENTHREAD_MTD || OPENTHREAD_FTD

    SuccessOrExit(error = mEncoder.WriteUintPacked(SPINEL_CAP_NET_THREAD_1_0));
#if OPENTHREAD_ENABLE_MAC_FILTER
    SuccessOrExit(error = mEncoder.WriteUintPacked(SPINEL_CAP_MAC_WHITELIST));
#endif

#if OPENTHREAD_ENABLE_JAM_DETECTION
    SuccessOrExit(error = mEncoder.WriteUintPacked(SPINEL_CAP_JAM_DETECT));
#endif

#if OPENTHREAD_CONFIG_ENABLE_STEERING_DATA_SET_OOB
    SuccessOrExit(error = mEncoder.WriteUintPacked(SPINEL_CAP_OOB_STEERING_DATA));
#endif

#if OPENTHREAD_CONFIG_NCP_ENABLE_PEEK_POKE
    SuccessOrExit(error = mEncoder.WriteUintPacked(SPINEL_CAP_PEEK_POKE));
#endif

    // TODO: Somehow get the following capability from the radio.
    SuccessOrExit(error = mEncoder.WriteUintPacked(SPINEL_CAP_802_15_4_2450MHZ_OQPSK));

#if OPENTHREAD_CONFIG_MAX_CHILDREN > 0
    SuccessOrExit(error = mEncoder.WriteUintPacked(SPINEL_CAP_ROLE_ROUTER));
#endif

    SuccessOrExit(error = mEncoder.WriteUintPacked(SPINEL_CAP_ROLE_SLEEPY));

#if OPENTHREAD_ENABLE_LEGACY
    SuccessOrExit(error = mEncoder.WriteUintPacked(SPINEL_CAP_NEST_LEGACY_INTERFACE));
#endif

#if OPENTHREAD_ENABLE_TMF_PROXY
    SuccessOrExit(error = mEncoder.WriteUintPacked(SPINEL_CAP_THREAD_TMF_PROXY));
#endif

#endif // OPENTHREAD_MTD || OPENTHREAD_FTD

exit:
    return error;
}

otError NcpBase::GetPropertyHandler_NCP_VERSION(void)
{
    return mEncoder.WriteUtf8(otGetVersionString());
}

otError NcpBase::GetPropertyHandler_INTERFACE_COUNT(void)
{
    return mEncoder.WriteUint8(1); // Only one interface for now
}

otError NcpBase::GetPropertyHandler_POWER_STATE(void)
{
    return mEncoder.WriteUint8(SPINEL_POWER_STATE_ONLINE);   // Always online at the moment
}

otError NcpBase::SetPropertyHandler_POWER_STATE(void)
{
    return OT_ERROR_NOT_IMPLEMENTED;
}

otError NcpBase::GetPropertyHandler_HWADDR(void)
{
    otExtAddress hwAddr;
    otLinkGetFactoryAssignedIeeeEui64(mInstance, &hwAddr);

    return mEncoder.WriteEui64(hwAddr);
}

otError NcpBase::GetPropertyHandler_LOCK(void)
{
    // TODO: Implement property lock (Needs API!)
    return mEncoder.OverwriteWithLastStatusError(SPINEL_STATUS_UNIMPLEMENTED);
}

otError NcpBase::GetPropertyHandler_HOST_POWER_STATE(void)
{
    return mEncoder.WriteUint8(mHostPowerState);
}

// Setting `HOST_POWER_STATE` is treated and implemented differently from other
// handlers as it requires two special behaviors (a) the response frame for the
// set operation should be tracked and only when it is delivered we can assume
// that host is sleep (b) the response is critical so if there is no spinel
// buffer to prepare the response, the current spinel header is saved to
// prepare and send the response as soon as buffer space becomes available.
otError NcpBase::SetPropertyHandler_HOST_POWER_STATE(uint8_t aHeader)
{
    uint8_t powerState;
    otError error = OT_ERROR_NONE;

    error = mDecoder.ReadUint8(powerState);

    if (error == OT_ERROR_NONE)
    {
        switch (powerState)
        {
        case SPINEL_HOST_POWER_STATE_OFFLINE:
        case SPINEL_HOST_POWER_STATE_DEEP_SLEEP:
        case SPINEL_HOST_POWER_STATE_LOW_POWER:
        case SPINEL_HOST_POWER_STATE_ONLINE:
            // Adopt the requested power state.
            mHostPowerState = static_cast<spinel_host_power_state_t>(powerState);
            break;

        case SPINEL_HOST_POWER_STATE_RESERVED:
            // Per the specification, treat this as synonymous with SPINEL_HOST_POWER_STATE_DEEP_SLEEP.
            mHostPowerState = SPINEL_HOST_POWER_STATE_DEEP_SLEEP;
            break;

        default:
            // Per the specification, treat unrecognized values as synonymous with SPINEL_HOST_POWER_STATE_LOW_POWER.
            mHostPowerState = SPINEL_HOST_POWER_STATE_LOW_POWER;
            break;
        }

        mHostPowerStateHeader = 0;

        error = HandleCommandPropertyGet(aHeader, SPINEL_PROP_HOST_POWER_STATE);

        if (mHostPowerState != SPINEL_HOST_POWER_STATE_ONLINE)
        {
            if (error == OT_ERROR_NONE)
            {
                mHostPowerReplyFrameTag = GetLastOutboundFrameTag();
            }
            else
            {
                mHostPowerReplyFrameTag = NcpFrameBuffer::kInvalidTag;
            }

            mHostPowerStateInProgress = true;
        }

        if (error != OT_ERROR_NONE)
        {
            mHostPowerStateHeader = aHeader;

            // The reply will be queued when buffer space becomes available
            // in the NCP tx buffer so we return `success` to avoid sending a
            // NOMEM status for the same tid through `mDroppedReplyTid` list.

            error = OT_ERROR_NONE;
        }
    }
    else
    {
        error = SendLastStatus(aHeader, ThreadErrorToSpinelStatus(error));
    }

    return error;
}

otError NcpBase::GetPropertyHandler_UNSOL_UPDATE_LIST(void)
{
    otError error = OT_ERROR_NONE;
    uint8_t numEntries;
    const ChangedPropsSet::Entry *entry;

    entry = mChangedPropsSet.GetSupportedEntries(numEntries);

    for (uint8_t index = 0; index < numEntries; index++, entry++)
    {
        if (entry->mFilterable)
        {
            SuccessOrExit(error = mEncoder.WriteUintPacked(entry->mPropKey));
        }
    }

exit:
    return error;
}

otError NcpBase::GetPropertyHandler_PHY_RX_SENSITIVITY(void)
{
    return mEncoder.WriteInt8(otPlatRadioGetReceiveSensitivity(mInstance));
}

otError NcpBase::GetPropertyHandler_PHY_TX_POWER(void)
{
    return mEncoder.WriteInt8(otLinkGetMaxTransmitPower(mInstance));
}

otError NcpBase::SetPropertyHandler_PHY_TX_POWER(void)
{
    int8_t txPower = 0;
    otError error = OT_ERROR_NONE;

    SuccessOrExit(error = mDecoder.ReadInt8(txPower));

    otLinkSetMaxTransmitPower(mInstance, txPower);

exit:
    return error;
}

otError NcpBase::GetPropertyHandler_DEBUG_TEST_ASSERT(void)
{
    assert(false);

    // We only get to this point if `assert(false)`
    // does not cause an NCP reset on the platform.
    // In such a case we return `false` as the
    // property value to indicate this.

    OT_UNREACHABLE_CODE(
        return mEncoder.WriteBool(false);
    )
}

otError NcpBase::GetPropertyHandler_DEBUG_NCP_LOG_LEVEL(void)
{
    uint8_t logLevel = 0;

    switch (otGetDynamicLogLevel(mInstance))
    {
    case OT_LOG_LEVEL_NONE:
        logLevel = SPINEL_NCP_LOG_LEVEL_EMERG;
        break;

    case OT_LOG_LEVEL_CRIT:
        logLevel = SPINEL_NCP_LOG_LEVEL_CRIT;
        break;

    case OT_LOG_LEVEL_WARN:
        logLevel = SPINEL_NCP_LOG_LEVEL_WARN;
        break;

    case OT_LOG_LEVEL_INFO:
        logLevel = SPINEL_NCP_LOG_LEVEL_INFO;
        break;

    case OT_LOG_LEVEL_DEBG:
        logLevel = SPINEL_NCP_LOG_LEVEL_DEBUG;
        break;
    }

    return mEncoder.WriteUint8(logLevel);
}

otError NcpBase::SetPropertyHandler_DEBUG_NCP_LOG_LEVEL(void)
{
    uint8_t spinelNcpLogLevel = 0;
    otLogLevel logLevel;
    otError error = OT_ERROR_NONE;

    SuccessOrExit(error = mDecoder.ReadUint8(spinelNcpLogLevel));

    switch (spinelNcpLogLevel)
    {
    case SPINEL_NCP_LOG_LEVEL_EMERG:
    case SPINEL_NCP_LOG_LEVEL_ALERT:
        logLevel = OT_LOG_LEVEL_NONE;
        break;

    case SPINEL_NCP_LOG_LEVEL_CRIT:
        logLevel = OT_LOG_LEVEL_CRIT;
        break;

    case SPINEL_NCP_LOG_LEVEL_ERR:
    case SPINEL_NCP_LOG_LEVEL_WARN:
        logLevel = OT_LOG_LEVEL_WARN;
        break;

    case SPINEL_NCP_LOG_LEVEL_NOTICE:
    case SPINEL_NCP_LOG_LEVEL_INFO:
        logLevel = OT_LOG_LEVEL_INFO;
        break;

    case SPINEL_NCP_LOG_LEVEL_DEBUG:
        logLevel = OT_LOG_LEVEL_DEBG;
        break;

    default:
        ExitNow(error = OT_ERROR_INVALID_ARGS);
        break;
    }

    error = otSetDynamicLogLevel(mInstance, logLevel);

exit:
    return error;
}

}  // namespace Ncp
}  // namespace ot

// ----------------------------------------------------------------------------
// MARK: Peek/Poke delegate API
// ----------------------------------------------------------------------------

otError otNcpRegisterPeekPokeDelagates(otNcpDelegateAllowPeekPoke aAllowPeekDelegate,
                                       otNcpDelegateAllowPeekPoke aAllowPokeDelegate)
{
    otError error = OT_ERROR_NONE;

#if OPENTHREAD_CONFIG_NCP_ENABLE_PEEK_POKE
    ot::Ncp::NcpBase *ncp = ot::Ncp::NcpBase::GetNcpInstance();

    if (ncp != NULL)
    {
        ncp->RegisterPeekPokeDelagates(aAllowPeekDelegate, aAllowPokeDelegate);
    }
#else
    OT_UNUSED_VARIABLE(aAllowPeekDelegate);
    OT_UNUSED_VARIABLE(aAllowPokeDelegate);

    error = OT_ERROR_DISABLED_FEATURE;

#endif // OPENTHREAD_CONFIG_NCP_ENABLE_PEEK_POKE

    return error;
}

// ----------------------------------------------------------------------------
// MARK: Virtual Datastream I/O (Public API)
// ----------------------------------------------------------------------------

otError otNcpStreamWrite(int aStreamId, const uint8_t *aDataPtr, int aDataLen)
{
    otError error  = OT_ERROR_INVALID_STATE;
    ot::Ncp::NcpBase *ncp = ot::Ncp::NcpBase::GetNcpInstance();

    if (ncp != NULL)
    {
        error = ncp->StreamWrite(aStreamId, aDataPtr, aDataLen);
    }

    return error;
}


extern "C" void otNcpPlatLogv(otLogLevel aLogLevel, otLogRegion aLogRegion, const char *aFormat, va_list ap)
{
    char logString[128];
    int charsWritten;

    if ((charsWritten = vsnprintf(logString, sizeof(logString), aFormat, ap)) > 0)
    {
        if (charsWritten > static_cast<int>(sizeof(logString) - 1))
        {
            charsWritten = static_cast<int>(sizeof(logString) - 1);
        }

        otNcpStreamWrite(0, reinterpret_cast<uint8_t *>(logString), charsWritten);
    }

    OT_UNUSED_VARIABLE(aLogLevel);
    OT_UNUSED_VARIABLE(aLogRegion);
}
