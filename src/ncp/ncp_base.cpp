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

#include <openthread/config.h>

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
// MARK: Command/Property Jump Tables
// ----------------------------------------------------------------------------

#define NCP_CMD_HANDLER_ENTRY(name)  { SPINEL_CMD_##name, &NcpBase::CommandHandler_##name }

const NcpBase::CommandHandlerEntry NcpBase::mCommandHandlerTable[] =
{
    NCP_CMD_HANDLER_ENTRY(NOOP),
    NCP_CMD_HANDLER_ENTRY(RESET),
    NCP_CMD_HANDLER_ENTRY(PROP_VALUE_GET),
    NCP_CMD_HANDLER_ENTRY(PROP_VALUE_SET),
    NCP_CMD_HANDLER_ENTRY(PROP_VALUE_INSERT),
    NCP_CMD_HANDLER_ENTRY(PROP_VALUE_REMOVE),
#if OPENTHREAD_CONFIG_NCP_ENABLE_PEEK_POKE
    NCP_CMD_HANDLER_ENTRY(PEEK),
    NCP_CMD_HANDLER_ENTRY(POKE),
#endif
#if OPENTHREAD_MTD || OPENTHREAD_FTD
    NCP_CMD_HANDLER_ENTRY(NET_SAVE),
    NCP_CMD_HANDLER_ENTRY(NET_CLEAR),
    NCP_CMD_HANDLER_ENTRY(NET_RECALL),
#endif
};

#define NCP_GET_PROP_HANDLER_ENTRY(name)                 { SPINEL_PROP_##name, &NcpBase::GetPropertyHandler_##name }
#define NCP_GET_PROP_HANDLER_ENTRY_METHOD(name, method)  { SPINEL_PROP_##name, &NcpBase::GetPropertyHandler_##method }

const NcpBase::GetPropertyHandlerEntry NcpBase::mGetPropertyHandlerTable[] =
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
    NCP_GET_PROP_HANDLER_ENTRY(STREAM_NET),
    // MAC counters
    NCP_GET_PROP_HANDLER_ENTRY_METHOD(CNTR_TX_PKT_TOTAL,        MAC_CNTR),
    NCP_GET_PROP_HANDLER_ENTRY_METHOD(CNTR_TX_PKT_ACK_REQ,      MAC_CNTR),
    NCP_GET_PROP_HANDLER_ENTRY_METHOD(CNTR_TX_PKT_ACKED,        MAC_CNTR),
    NCP_GET_PROP_HANDLER_ENTRY_METHOD(CNTR_TX_PKT_NO_ACK_REQ,   MAC_CNTR),
    NCP_GET_PROP_HANDLER_ENTRY_METHOD(CNTR_TX_PKT_DATA,         MAC_CNTR),
    NCP_GET_PROP_HANDLER_ENTRY_METHOD(CNTR_TX_PKT_DATA_POLL,    MAC_CNTR),
    NCP_GET_PROP_HANDLER_ENTRY_METHOD(CNTR_TX_PKT_BEACON,       MAC_CNTR),
    NCP_GET_PROP_HANDLER_ENTRY_METHOD(CNTR_TX_PKT_BEACON_REQ,   MAC_CNTR),
    NCP_GET_PROP_HANDLER_ENTRY_METHOD(CNTR_TX_PKT_OTHER,        MAC_CNTR),
    NCP_GET_PROP_HANDLER_ENTRY_METHOD(CNTR_TX_PKT_RETRY,        MAC_CNTR),
    NCP_GET_PROP_HANDLER_ENTRY_METHOD(CNTR_TX_PKT_UNICAST,      MAC_CNTR),
    NCP_GET_PROP_HANDLER_ENTRY_METHOD(CNTR_TX_PKT_BROADCAST,    MAC_CNTR),
    NCP_GET_PROP_HANDLER_ENTRY_METHOD(CNTR_TX_ERR_CCA,          MAC_CNTR),
    NCP_GET_PROP_HANDLER_ENTRY_METHOD(CNTR_TX_ERR_ABORT,        MAC_CNTR),
    NCP_GET_PROP_HANDLER_ENTRY_METHOD(CNTR_RX_PKT_TOTAL,        MAC_CNTR),
    NCP_GET_PROP_HANDLER_ENTRY_METHOD(CNTR_RX_PKT_DATA,         MAC_CNTR),
    NCP_GET_PROP_HANDLER_ENTRY_METHOD(CNTR_RX_PKT_DATA_POLL,    MAC_CNTR),
    NCP_GET_PROP_HANDLER_ENTRY_METHOD(CNTR_RX_PKT_BEACON,       MAC_CNTR),
    NCP_GET_PROP_HANDLER_ENTRY_METHOD(CNTR_RX_PKT_BEACON_REQ,   MAC_CNTR),
    NCP_GET_PROP_HANDLER_ENTRY_METHOD(CNTR_RX_PKT_OTHER,        MAC_CNTR),
    NCP_GET_PROP_HANDLER_ENTRY_METHOD(CNTR_RX_PKT_FILT_WL,      MAC_CNTR),
    NCP_GET_PROP_HANDLER_ENTRY_METHOD(CNTR_RX_PKT_FILT_DA,      MAC_CNTR),
    NCP_GET_PROP_HANDLER_ENTRY_METHOD(CNTR_RX_PKT_UNICAST,      MAC_CNTR),
    NCP_GET_PROP_HANDLER_ENTRY_METHOD(CNTR_RX_PKT_BROADCAST,    MAC_CNTR),
    NCP_GET_PROP_HANDLER_ENTRY_METHOD(CNTR_RX_ERR_EMPTY,        MAC_CNTR),
    NCP_GET_PROP_HANDLER_ENTRY_METHOD(CNTR_RX_ERR_UKWN_NBR,     MAC_CNTR),
    NCP_GET_PROP_HANDLER_ENTRY_METHOD(CNTR_RX_ERR_NVLD_SADDR,   MAC_CNTR),
    NCP_GET_PROP_HANDLER_ENTRY_METHOD(CNTR_RX_ERR_SECURITY,     MAC_CNTR),
    NCP_GET_PROP_HANDLER_ENTRY_METHOD(CNTR_RX_ERR_BAD_FCS,      MAC_CNTR),
    NCP_GET_PROP_HANDLER_ENTRY_METHOD(CNTR_RX_ERR_OTHER,        MAC_CNTR),
    NCP_GET_PROP_HANDLER_ENTRY_METHOD(CNTR_RX_PKT_DUP,          MAC_CNTR),
    // NCP counters
    NCP_GET_PROP_HANDLER_ENTRY_METHOD(CNTR_TX_IP_SEC_TOTAL,     NCP_CNTR),
    NCP_GET_PROP_HANDLER_ENTRY_METHOD(CNTR_TX_IP_INSEC_TOTAL,   NCP_CNTR),
    NCP_GET_PROP_HANDLER_ENTRY_METHOD(CNTR_TX_IP_DROPPED,       NCP_CNTR),
    NCP_GET_PROP_HANDLER_ENTRY_METHOD(CNTR_RX_IP_SEC_TOTAL,     NCP_CNTR),
    NCP_GET_PROP_HANDLER_ENTRY_METHOD(CNTR_RX_IP_INSEC_TOTAL,   NCP_CNTR),
    NCP_GET_PROP_HANDLER_ENTRY_METHOD(CNTR_RX_IP_DROPPED,       NCP_CNTR),
    NCP_GET_PROP_HANDLER_ENTRY_METHOD(CNTR_TX_SPINEL_TOTAL,     NCP_CNTR),
    NCP_GET_PROP_HANDLER_ENTRY_METHOD(CNTR_RX_SPINEL_TOTAL,     NCP_CNTR),
    NCP_GET_PROP_HANDLER_ENTRY_METHOD(CNTR_RX_SPINEL_ERR,       NCP_CNTR),
    // IP counters
    NCP_GET_PROP_HANDLER_ENTRY_METHOD(CNTR_IP_TX_SUCCESS,       IP_CNTR),
    NCP_GET_PROP_HANDLER_ENTRY_METHOD(CNTR_IP_RX_SUCCESS,       IP_CNTR),
    NCP_GET_PROP_HANDLER_ENTRY_METHOD(CNTR_IP_TX_FAILURE,       IP_CNTR),
    NCP_GET_PROP_HANDLER_ENTRY_METHOD(CNTR_IP_RX_FAILURE,       IP_CNTR),
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

const NcpBase::SetPropertyHandlerEntry NcpBase::mSetPropertyHandlerTable[] =
{
    NCP_SET_PROP_HANDLER_ENTRY(POWER_STATE),
    NCP_SET_PROP_HANDLER_ENTRY(HOST_POWER_STATE),
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
    NCP_SET_PROP_HANDLER_ENTRY(STREAM_RAW),
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
#if OPENTHREAD_ENABLE_DIAG
    NCP_SET_PROP_HANDLER_ENTRY(NEST_STREAM_MFG),
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
#if OPENTHREAD_ENABLE_COMMISSIONER
    NCP_SET_PROP_HANDLER_ENTRY(THREAD_COMMISSIONER_ENABLED),
#endif
#if OPENTHREAD_ENABLE_TMF_PROXY
    NCP_SET_PROP_HANDLER_ENTRY(THREAD_TMF_PROXY_ENABLED),
    NCP_SET_PROP_HANDLER_ENTRY(THREAD_TMF_PROXY_STREAM),
#endif
#endif // #if OPENTHREAD_FTD
};

#define NCP_INSERT_PROP_HANDLER_ENTRY(name)  { SPINEL_PROP_##name, &NcpBase::InsertPropertyHandler_##name }

const NcpBase::InsertPropertyHandlerEntry NcpBase::mInsertPropertyHandlerTable[] =
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

const NcpBase::RemovePropertyHandlerEntry NcpBase::mRemovePropertyHandlerTable[] =
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
    mHostPowerStateInProgress(false),
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
// MARK: Outbound Frame methods
// ----------------------------------------------------------------------------

otError NcpBase::OutboundFrameBegin(uint8_t aHeader)
{
    NcpFrameBuffer::Priority priority;

    // Non-zero tid indicates this is a response to a spinel command.
    if (SPINEL_HEADER_GET_TID(aHeader) != 0)
    {
        priority = NcpFrameBuffer::kPriorityHigh;
    }
    else
    {
        priority = NcpFrameBuffer::kPriorityLow;
    }

    return mTxFrameBuffer.InFrameBegin(priority);
}

otError NcpBase::OutboundFrameFeedData(const uint8_t *aDataBuffer, uint16_t aDataBufferLength)
{
    return mTxFrameBuffer.InFrameFeedData(aDataBuffer, aDataBufferLength);
}

otError NcpBase::OutboundFrameFeedMessage(otMessage *aMessage)
{
    return mTxFrameBuffer.InFrameFeedMessage(aMessage);
}

otError NcpBase::OutboundFrameEnd(void)
{
    return mTxFrameBuffer.InFrameEnd();
}

NcpFrameBuffer::FrameTag NcpBase::GetLastOutboundFrameTag(void)
{
    return mTxFrameBuffer.InFrameGetLastTag();
}

// ----------------------------------------------------------------------------
// MARK: Serial Traffic Glue
// ----------------------------------------------------------------------------

otError NcpBase::OutboundFrameSend(void)
{
    otError error;

    SuccessOrExit(error = OutboundFrameEnd());

    mTxSpinelFrameCounter++;

exit:
    return error;
}

void NcpBase::HandleReceive(const uint8_t *aBuf, uint16_t aBufLength)
{
    uint8_t header = 0;
    unsigned int command = 0;
    spinel_ssize_t parsedLength;
    const uint8_t *argPtr = NULL;
    unsigned int argLen = 0;
    otError error = OT_ERROR_NONE;
    spinel_tid_t tid = 0;

    parsedLength = spinel_datatype_unpack(
                       aBuf,
                       aBufLength,
                       SPINEL_DATATYPE_COMMAND_S SPINEL_DATATYPE_DATA_S,
                       &header,
                       &command,
                       &argPtr,
                       &argLen
                   );

    tid = SPINEL_HEADER_GET_TID(header);

    // Receiving any message from the host has the side effect of transitioning the host power state to online.
    mHostPowerState = SPINEL_HOST_POWER_STATE_ONLINE;
    mHostPowerStateInProgress = false;

    if (parsedLength == aBufLength)
    {
        error = HandleCommand(header, command, argPtr, static_cast<uint16_t>(argLen));

        // Check if we may have missed a `tid` in the sequence.
        if ((mNextExpectedTid != 0) && (tid != mNextExpectedTid))
        {
            mRxSpinelOutOfOrderTidCounter++;
        }

        mNextExpectedTid = SPINEL_GET_NEXT_TID(tid);
    }
    else
    {
        error = SendLastStatus(header, SPINEL_STATUS_PARSE_ERROR);
    }

    if (error == OT_ERROR_NO_BUFS)
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
            GetPropertyHandler_HOST_POWER_STATE(
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

    error = SendPropertyUpdate(
                SPINEL_HEADER_FLAG | SPINEL_HEADER_IID_0,
                SPINEL_CMD_PROP_VALUE_IS,
                streamPropKey,
                aDataPtr,
                static_cast<uint16_t>(aDataLen)
            );

    if (error != OT_ERROR_NONE)
    {
        mChangedPropsSet.AddLastStatus(SPINEL_STATUS_NOMEM);
        mUpdateChangedPropsTask.Post();
    }

exit:
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

    SuccessOrExit(OutboundFrameBegin(header));

    if (aFrame->mDidTX)
    {
        flags |= SPINEL_MD_FLAG_TX;
    }

    // Append frame header and frame length
    SuccessOrExit(
        OutboundFrameFeedPacked(
            SPINEL_DATATYPE_COMMAND_PROP_S SPINEL_DATATYPE_UINT16_S,
            header,
            SPINEL_CMD_PROP_VALUE_IS,
            SPINEL_PROP_STREAM_RAW,
            aFrame->mLength
        ));

    // Append the frame contents
    SuccessOrExit(OutboundFrameFeedData(aFrame->mPsdu, aFrame->mLength));

    // Append metadata (rssi, etc)
    SuccessOrExit(
        OutboundFrameFeedPacked(
            SPINEL_DATATYPE_INT8_S
            SPINEL_DATATYPE_INT8_S
            SPINEL_DATATYPE_UINT16_S
            SPINEL_DATATYPE_STRUCT_S(  // PHY-data
                SPINEL_DATATYPE_NULL_S // Empty for now
            )
            SPINEL_DATATYPE_STRUCT_S(  // Vendor-data
                SPINEL_DATATYPE_NULL_S // Empty for now
            ),
            aFrame->mPower,   // TX Power
            -128,             // Noise Floor (Currently unused)
            flags             // Flags

           // Skip PHY and Vendor data for now
        ));

    SuccessOrExit(OutboundFrameSend());

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
// MARK: Inbound Command Handlers
// ----------------------------------------------------------------------------

otError NcpBase::HandleCommand(uint8_t aHeader, unsigned int aCommand, const uint8_t *aArgPtr, uint16_t aArgLen)
{
    unsigned i;
    otError error = OT_ERROR_NONE;

    // Skip if this isn't a spinel frame
    VerifyOrExit((SPINEL_HEADER_FLAG & aHeader) == SPINEL_HEADER_FLAG, error = OT_ERROR_INVALID_ARGS);

    // We only support IID zero for now.
    VerifyOrExit(
        SPINEL_HEADER_GET_IID(aHeader) == 0,
        error = SendLastStatus(aHeader, SPINEL_STATUS_INVALID_INTERFACE)
    );
#if OPENTHREAD_ENABLE_SPINEL_VENDOR_SUPPORT
    if (aCommand >= SPINEL_CMD_VENDOR__BEGIN && aCommand < SPINEL_CMD_VENDOR__END)
    {
        error = VendorCommandHandler(aHeader, aCommand, aArgPtr, aArgLen);
    }
    else
#endif // OPENTHREAD_ENABLE_SPINEL_VENDOR_SUPPORT
    {
        for (i = 0; i < sizeof(mCommandHandlerTable) / sizeof(mCommandHandlerTable[0]); i++)
        {
            if (mCommandHandlerTable[i].mCommand == aCommand)
            {
                break;
            }
        }

        if (i < sizeof(mCommandHandlerTable) / sizeof(mCommandHandlerTable[0]))
        {
            error = (this->*mCommandHandlerTable[i].mHandler)(aHeader, aCommand, aArgPtr, aArgLen);
        }
        else
        {
            error = SendLastStatus(aHeader, SPINEL_STATUS_INVALID_COMMAND);
        }
    }

exit:
    return error;
}

otError NcpBase::HandleCommandPropertyGet(uint8_t aHeader, spinel_prop_key_t aKey)
{
    unsigned i;
    otError error = OT_ERROR_NONE;

    for (i = 0; i < sizeof(mGetPropertyHandlerTable) / sizeof(mGetPropertyHandlerTable[0]); i++)
    {
        if (mGetPropertyHandlerTable[i].mPropKey == aKey)
        {
            break;
        }
    }

    if (i < sizeof(mGetPropertyHandlerTable) / sizeof(mGetPropertyHandlerTable[0]))
    {
        error = (this->*mGetPropertyHandlerTable[i].mHandler)(aHeader, aKey);
    }
    else
    {
        error = SendLastStatus(aHeader, SPINEL_STATUS_PROP_NOT_FOUND);
    }

    return error;
}

otError NcpBase::HandleCommandPropertySet(uint8_t aHeader, spinel_prop_key_t aKey, const uint8_t *aValuePtr,
                                          uint16_t aValueLen)
{
    unsigned i;
    otError error = OT_ERROR_NONE;

    for (i = 0; i < sizeof(mSetPropertyHandlerTable) / sizeof(mSetPropertyHandlerTable[0]); i++)
    {
        if (mSetPropertyHandlerTable[i].mPropKey == aKey)
        {
            break;
        }
    }

    if (i < sizeof(mSetPropertyHandlerTable) / sizeof(mSetPropertyHandlerTable[0]))
    {
        error = (this->*mSetPropertyHandlerTable[i].mHandler)(aHeader, aKey, aValuePtr, aValueLen);
    }
    else
    {
        error = SendLastStatus(aHeader, SPINEL_STATUS_PROP_NOT_FOUND);
    }

    return error;
}

otError NcpBase::HandleCommandPropertyInsert(uint8_t aHeader, spinel_prop_key_t aKey, const uint8_t *aValuePtr,
                                             uint16_t aValueLen)
{
    unsigned i;
    otError error = OT_ERROR_NONE;

    for (i = 0; i < sizeof(mInsertPropertyHandlerTable) / sizeof(mInsertPropertyHandlerTable[0]); i++)
    {
        if (mInsertPropertyHandlerTable[i].mPropKey == aKey)
        {
            break;
        }
    }

    if (i < sizeof(mInsertPropertyHandlerTable) / sizeof(mInsertPropertyHandlerTable[0]))
    {
        error = (this->*mInsertPropertyHandlerTable[i].mHandler)(aHeader, aKey, aValuePtr, aValueLen);
    }
    else
    {
        error = SendLastStatus(aHeader, SPINEL_STATUS_PROP_NOT_FOUND);
    }

    return error;
}

otError NcpBase::HandleCommandPropertyRemove(uint8_t aHeader, spinel_prop_key_t aKey, const uint8_t *aValuePtr,
                                             uint16_t aValueLen)
{
    unsigned i;
    otError error = OT_ERROR_NONE;

    for (i = 0; i < sizeof(mRemovePropertyHandlerTable) / sizeof(mRemovePropertyHandlerTable[0]); i++)
    {
        if (mRemovePropertyHandlerTable[i].mPropKey == aKey)
        {
            break;
        }
    }

    if (i < sizeof(mRemovePropertyHandlerTable) / sizeof(mRemovePropertyHandlerTable[0]))
    {
        error = (this->*mRemovePropertyHandlerTable[i].mHandler)(aHeader, aKey, aValuePtr, aValueLen);
    }
    else
    {
        error = SendLastStatus(aHeader, SPINEL_STATUS_PROP_NOT_FOUND);
    }

    return error;
}


// ----------------------------------------------------------------------------
// MARK: Outbound Command Handlers
// ----------------------------------------------------------------------------

otError NcpBase::SendLastStatus(uint8_t aHeader, spinel_status_t aLastStatus)
{
    if (SPINEL_HEADER_GET_IID(aHeader) == 0)
    {
        mLastStatus = aLastStatus;
    }

    return SendPropertyUpdate(
               aHeader,
               SPINEL_CMD_PROP_VALUE_IS,
               SPINEL_PROP_LAST_STATUS,
               SPINEL_DATATYPE_UINT_PACKED_S,
               aLastStatus
           );
}

otError NcpBase::SendPropertyUpdate(uint8_t aHeader, uint8_t aCommand, spinel_prop_key_t aKey,
                                    const char *aPackFormat, ...)
{
    otError error = OT_ERROR_NONE;
    va_list args;

    va_start(args, aPackFormat);
    SuccessOrExit(error = OutboundFrameBegin(aHeader));
    SuccessOrExit(error = OutboundFrameFeedPacked(SPINEL_DATATYPE_COMMAND_PROP_S, aHeader, aCommand, aKey));
    SuccessOrExit(error = OutboundFrameFeedVPacked(aPackFormat, args));
    SuccessOrExit(error = OutboundFrameSend());

exit:
    va_end(args);
    return error;
}

otError NcpBase::SendPropertyUpdate(uint8_t aHeader, uint8_t aCommand, spinel_prop_key_t aKey,
                                    const uint8_t *aValuePtr, uint16_t aValueLen)
{
    otError error = OT_ERROR_NONE;

    SuccessOrExit(error = OutboundFrameBegin(aHeader));
    SuccessOrExit(error = OutboundFrameFeedPacked(SPINEL_DATATYPE_COMMAND_PROP_S, aHeader, aCommand, aKey));
    SuccessOrExit(error = OutboundFrameFeedData(aValuePtr, aValueLen));
    SuccessOrExit(error = OutboundFrameSend());

exit:
    return error;
}

otError NcpBase::SendPropertyUpdate(uint8_t aHeader, uint8_t aCommand, spinel_prop_key_t aKey, otMessage *aMessage)
{
    otError error = OT_ERROR_NONE;

    SuccessOrExit(error = OutboundFrameBegin(aHeader));
    SuccessOrExit(error = OutboundFrameFeedPacked(SPINEL_DATATYPE_COMMAND_PROP_S, aHeader, aCommand, aKey));
    SuccessOrExit(error = OutboundFrameFeedMessage(aMessage));

    // Set the `aMessage` pointer to NULL to indicate that it does
    // not need to be freed at the exit. The `aMessage` is now owned
    // by the OutboundFrame and will be freed when the frame is either
    // successfully sent and then removed, or if the frame gets
    // discarded.
    aMessage = NULL;

    SuccessOrExit(error = OutboundFrameSend());

exit:

    if (aMessage != NULL)
    {
        otMessageFree(aMessage);
    }

    return error;
}

otError NcpBase::SendSetPropertyResponse(uint8_t aHeader, spinel_prop_key_t aKey, otError aError)
{
    if (aError == OT_ERROR_NONE)
    {
        aError = HandleCommandPropertyGet(aHeader, aKey);
    }
    else
    {
        aError = SendLastStatus(aHeader, ThreadErrorToSpinelStatus(aError));
    }

    return aError;
}

otError NcpBase::OutboundFrameFeedVPacked(const char *aPackFormat, va_list aArgs)
{
    uint8_t buf[96];
    otError error = OT_ERROR_NO_BUFS;
    spinel_ssize_t packedLen;

    packedLen = spinel_datatype_vpack(buf, sizeof(buf), aPackFormat, aArgs);

    if ((packedLen > 0) && (packedLen <= static_cast<spinel_ssize_t>(sizeof(buf))))
    {
        error = OutboundFrameFeedData(buf, static_cast<uint16_t>(packedLen));
    }

    return error;
}

otError NcpBase::OutboundFrameFeedPacked(const char *aPackFormat, ...)
{
    otError error;
    va_list args;

    va_start(args, aPackFormat);
    error = OutboundFrameFeedVPacked(aPackFormat, args);
    va_end(args);

    return error;
}

// ----------------------------------------------------------------------------
// MARK: Individual Command Handlers
// ----------------------------------------------------------------------------

otError NcpBase::CommandHandler_NOOP(uint8_t aHeader, unsigned int aCommand, const uint8_t *aArgPtr, uint16_t aArgLen)
{
    OT_UNUSED_VARIABLE(aCommand);
    OT_UNUSED_VARIABLE(aArgPtr);
    OT_UNUSED_VARIABLE(aArgLen);

    return SendLastStatus(aHeader, SPINEL_STATUS_OK);
}

otError NcpBase::CommandHandler_RESET(uint8_t aHeader, unsigned int aCommand, const uint8_t *aArgPtr,
                                      uint16_t aArgLen)
{
    otError error = OT_ERROR_NONE;

    // We aren't using any of the arguments to this function.
    OT_UNUSED_VARIABLE(aHeader);
    OT_UNUSED_VARIABLE(aCommand);
    OT_UNUSED_VARIABLE(aArgPtr);
    OT_UNUSED_VARIABLE(aArgLen);

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

otError NcpBase::CommandHandler_PROP_VALUE_GET(uint8_t aHeader, unsigned int aCommand, const uint8_t *aArgPtr,
                                               uint16_t aArgLen)
{
    unsigned int propKey = 0;
    spinel_ssize_t parsedLength;
    otError error = OT_ERROR_NONE;

    parsedLength = spinel_datatype_unpack(aArgPtr, aArgLen, SPINEL_DATATYPE_UINT_PACKED_S, &propKey);

    if (parsedLength > 0)
    {
        error = HandleCommandPropertyGet(aHeader, static_cast<spinel_prop_key_t>(propKey));
    }
    else
    {
        error = SendLastStatus(aHeader, SPINEL_STATUS_PARSE_ERROR);
    }

    OT_UNUSED_VARIABLE(aCommand);

    return error;
}

otError NcpBase::CommandHandler_PROP_VALUE_SET(uint8_t aHeader, unsigned int aCommand, const uint8_t *aArgPtr,
                                               uint16_t aArgLen)
{
    unsigned int propKey = 0;
    spinel_ssize_t parsedLength;
    const uint8_t *valuePtr;
    unsigned int valueLen;
    otError error = OT_ERROR_NONE;

    parsedLength = spinel_datatype_unpack(
                       aArgPtr,
                       aArgLen,
                       SPINEL_DATATYPE_UINT_PACKED_S SPINEL_DATATYPE_DATA_S,
                       &propKey,
                       &valuePtr,
                       &valueLen
                   );

    if (parsedLength == aArgLen)
    {
        error = HandleCommandPropertySet(
                        aHeader,
                        static_cast<spinel_prop_key_t>(propKey),
                        valuePtr,
                        static_cast<uint16_t>(valueLen)
                    );
    }
    else
    {
        error = SendLastStatus(aHeader, SPINEL_STATUS_PARSE_ERROR);
    }

    OT_UNUSED_VARIABLE(aCommand);

    return error;
}

otError NcpBase::CommandHandler_PROP_VALUE_INSERT(uint8_t aHeader, unsigned int aCommand, const uint8_t *aArgPtr,
                                                  uint16_t aArgLen)
{
    unsigned int propKey = 0;
    spinel_ssize_t parsedLength;
    const uint8_t *valuePtr;
    unsigned int valueLen;
    otError error = OT_ERROR_NONE;

    parsedLength = spinel_datatype_unpack(
                       aArgPtr,
                       aArgLen,
                       SPINEL_DATATYPE_UINT_PACKED_S SPINEL_DATATYPE_DATA_S,
                       &propKey,
                       &valuePtr,
                       &valueLen
                   );

    if (parsedLength == aArgLen)
    {
        error = HandleCommandPropertyInsert(
                    aHeader,
                    static_cast<spinel_prop_key_t>(propKey),
                    valuePtr,
                    static_cast<uint16_t>(valueLen)
                );
    }
    else
    {
        error = SendLastStatus(aHeader, SPINEL_STATUS_PARSE_ERROR);
    }

    OT_UNUSED_VARIABLE(aCommand);

    return error;
}

otError NcpBase::CommandHandler_PROP_VALUE_REMOVE(uint8_t aHeader, unsigned int aCommand, const uint8_t *aArgPtr,
                                                  uint16_t aArgLen)
{
    unsigned int propKey = 0;
    spinel_ssize_t parsedLength;
    const uint8_t *valuePtr;
    unsigned int valueLen;
    otError error = OT_ERROR_NONE;

    parsedLength = spinel_datatype_unpack(
                       aArgPtr,
                       aArgLen,
                       SPINEL_DATATYPE_UINT_PACKED_S SPINEL_DATATYPE_DATA_S,
                       &propKey,
                       &valuePtr,
                       &valueLen
                   );

    if (parsedLength == aArgLen)
    {
        error = HandleCommandPropertyRemove(
                    aHeader,
                    static_cast<spinel_prop_key_t>(propKey),
                    valuePtr,
                    static_cast<uint16_t>(valueLen)
                );
    }
    else
    {
        error = SendLastStatus(aHeader, SPINEL_STATUS_PARSE_ERROR);
    }

    OT_UNUSED_VARIABLE(aCommand);

    return error;
}

#if OPENTHREAD_CONFIG_NCP_ENABLE_PEEK_POKE

otError NcpBase::CommandHandler_PEEK(uint8_t aHeader, unsigned int aCommand, const uint8_t *aArgPtr, uint16_t aArgLen)
{
    spinel_ssize_t parsedLength;
    uint32_t address;
    uint16_t count;
    spinel_status_t spinelError = SPINEL_STATUS_OK;
    otError error = OT_ERROR_NONE;

    parsedLength = spinel_datatype_unpack(
                       aArgPtr,
                       aArgLen,
                       (
                           SPINEL_DATATYPE_UINT32_S   // Address
                           SPINEL_DATATYPE_UINT16_S   // Count
                       ),
                       &address,
                       &count
                   );

    VerifyOrExit(parsedLength == aArgLen, spinelError = SPINEL_STATUS_PARSE_ERROR);

    VerifyOrExit(count != 0, spinelError = SPINEL_STATUS_INVALID_ARGUMENT);

    if (mAllowPeekDelegate != NULL)
    {
        VerifyOrExit(mAllowPeekDelegate(address, count), spinelError = SPINEL_STATUS_INVALID_ARGUMENT);
    }

    SuccessOrExit(error = OutboundFrameBegin(aHeader));
    SuccessOrExit(
        error = OutboundFrameFeedPacked(
                    (
                        SPINEL_DATATYPE_COMMAND_S   // Header and Command
                        SPINEL_DATATYPE_UINT32_S    // Address
                        SPINEL_DATATYPE_UINT16_S    // Count
                    ),
                    aHeader,
                    SPINEL_CMD_PEEK_RET,
                    address,
                    count
                ));
    SuccessOrExit(error = OutboundFrameFeedData(reinterpret_cast<const uint8_t *>(address), count));
    SuccessOrExit(error = OutboundFrameSend());

exit:
    if (spinelError != SPINEL_STATUS_OK)
    {
        error = SendLastStatus(aHeader, spinelError);
    }

    OT_UNUSED_VARIABLE(aCommand);

    return error;
}

otError NcpBase::CommandHandler_POKE(uint8_t aHeader, unsigned int aCommand, const uint8_t *aArgPtr, uint16_t aArgLen)
{
    spinel_ssize_t parsedLength;
    uint32_t address;
    uint16_t count;
    const uint8_t *dataPtr = NULL;
    spinel_size_t dataLen;
    spinel_status_t spinelError = SPINEL_STATUS_OK;
    otError error = OT_ERROR_NONE;

    parsedLength = spinel_datatype_unpack(
                       aArgPtr,
                       aArgLen,
                       (
                           SPINEL_DATATYPE_UINT32_S   // Address
                           SPINEL_DATATYPE_UINT16_S   // Count
                           SPINEL_DATATYPE_DATA_S     // Bytes
                       ),
                       &address,
                       &count,
                       &dataPtr,
                       &dataLen
                   );

    VerifyOrExit(parsedLength == aArgLen, spinelError = SPINEL_STATUS_PARSE_ERROR);

    VerifyOrExit(count != 0, spinelError = SPINEL_STATUS_INVALID_ARGUMENT);
    VerifyOrExit(count <= dataLen, spinelError = SPINEL_STATUS_INVALID_ARGUMENT);

    if (mAllowPokeDelegate != NULL)
    {
        VerifyOrExit(mAllowPokeDelegate(address, count), spinelError = SPINEL_STATUS_INVALID_ARGUMENT);
    }

    memcpy(reinterpret_cast<uint8_t *>(address), dataPtr, count);

exit:
    error = SendLastStatus(aHeader, spinelError);

    OT_UNUSED_VARIABLE(aCommand);

    return error;
}

#endif // OPENTHREAD_CONFIG_NCP_ENABLE_PEEK_POKE


// ----------------------------------------------------------------------------
// MARK: Individual Property Getters and Setters
// ----------------------------------------------------------------------------

otError NcpBase::GetPropertyHandler_PHY_ENABLED(uint8_t aHeader, spinel_prop_key_t aKey)
{
    return SendPropertyUpdate(
               aHeader,
               SPINEL_CMD_PROP_VALUE_IS,
               aKey,
               SPINEL_DATATYPE_BOOL_S,
#if OPENTHREAD_ENABLE_RAW_LINK_API
               otLinkRawIsEnabled(mInstance)
#else
               false
#endif // OPENTHREAD_ENABLE_RAW_LINK_API
           );
}

otError NcpBase::GetPropertyHandler_PHY_CHAN(uint8_t aHeader, spinel_prop_key_t aKey)
{
    return SendPropertyUpdate(
               aHeader,
               SPINEL_CMD_PROP_VALUE_IS,
               aKey,
               SPINEL_DATATYPE_UINT8_S,
               otLinkGetChannel(mInstance)
           );
}

otError NcpBase::SetPropertyHandler_PHY_CHAN(uint8_t aHeader, spinel_prop_key_t aKey, const uint8_t *aValuePtr,
                                             uint16_t aValueLen)
{
    unsigned int channel = 0;
    spinel_ssize_t parsedLength;
    otError error = OT_ERROR_NONE;

    parsedLength = spinel_datatype_unpack(
                       aValuePtr,
                       aValueLen,
                       SPINEL_DATATYPE_UINT_PACKED_S,
                       &channel
                   );

    VerifyOrExit(parsedLength > 0, error = OT_ERROR_PARSE);

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
    return SendSetPropertyResponse(aHeader, aKey, error);
}

otError NcpBase::GetPropertyHandler_MAC_PROMISCUOUS_MODE(uint8_t aHeader, spinel_prop_key_t aKey)
{
    return SendPropertyUpdate(
               aHeader,
               SPINEL_CMD_PROP_VALUE_IS,
               aKey,
               SPINEL_DATATYPE_INT8_S,
               otPlatRadioGetPromiscuous(mInstance)
                   ? SPINEL_MAC_PROMISCUOUS_MODE_FULL
                   : SPINEL_MAC_PROMISCUOUS_MODE_OFF
           );
}

otError NcpBase::SetPropertyHandler_MAC_PROMISCUOUS_MODE(uint8_t aHeader, spinel_prop_key_t aKey,
                                                         const uint8_t *aValuePtr, uint16_t aValueLen)
{
    uint8_t mode = 0;
    spinel_ssize_t parsedLength;
    otError error = OT_ERROR_NONE;

    parsedLength = spinel_datatype_unpack(
                       aValuePtr,
                       aValueLen,
                       SPINEL_DATATYPE_UINT8_S,
                       &mode
                   );

    VerifyOrExit(parsedLength > 0, error = OT_ERROR_PARSE);

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
    return SendSetPropertyResponse(aHeader, aKey, error);
}

otError NcpBase::GetPropertyHandler_MAC_15_4_PANID(uint8_t aHeader, spinel_prop_key_t aKey)
{
    return SendPropertyUpdate(
               aHeader,
               SPINEL_CMD_PROP_VALUE_IS,
               aKey,
               SPINEL_DATATYPE_UINT16_S,
               otLinkGetPanId(mInstance)
           );
}

otError NcpBase::SetPropertyHandler_MAC_15_4_PANID(uint8_t aHeader, spinel_prop_key_t aKey, const uint8_t *aValuePtr,
                                                   uint16_t aValueLen)
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

    error = otLinkSetPanId(mInstance, panid);

exit:
    return SendSetPropertyResponse(aHeader, aKey, error);
}

otError NcpBase::GetPropertyHandler_MAC_15_4_LADDR(uint8_t aHeader, spinel_prop_key_t aKey)
{
    return SendPropertyUpdate(
               aHeader,
               SPINEL_CMD_PROP_VALUE_IS,
               aKey,
               SPINEL_DATATYPE_EUI64_S,
               otLinkGetExtendedAddress(mInstance)
           );
}

otError NcpBase::SetPropertyHandler_MAC_15_4_LADDR(uint8_t aHeader, spinel_prop_key_t aKey, const uint8_t *aValuePtr,
                                                   uint16_t aValueLen)
{
    otExtAddress *extAddress;
    spinel_ssize_t parsedLength;
    otError error = OT_ERROR_NONE;

    parsedLength = spinel_datatype_unpack(
                       aValuePtr,
                       aValueLen,
                       SPINEL_DATATYPE_EUI64_S,
                       &extAddress
                   );

    VerifyOrExit(parsedLength > 0, error = OT_ERROR_PARSE);

    error = otLinkSetExtendedAddress(mInstance, extAddress);

exit:
    return SendSetPropertyResponse(aHeader, aKey, error);
}

otError NcpBase::GetPropertyHandler_MAC_15_4_SADDR(uint8_t aHeader, spinel_prop_key_t aKey)
{
    return SendPropertyUpdate(
               aHeader,
               SPINEL_CMD_PROP_VALUE_IS,
               aKey,
               SPINEL_DATATYPE_UINT16_S,
               otLinkGetShortAddress(mInstance)
           );
}

otError NcpBase::GetPropertyHandler_MAC_RAW_STREAM_ENABLED(uint8_t aHeader, spinel_prop_key_t aKey)
{
    return SendPropertyUpdate(
               aHeader,
               SPINEL_CMD_PROP_VALUE_IS,
               aKey,
               SPINEL_DATATYPE_BOOL_S,
               mIsRawStreamEnabled
           );
}

otError NcpBase::SetPropertyHandler_MAC_RAW_STREAM_ENABLED(uint8_t aHeader, spinel_prop_key_t aKey,
                                                           const uint8_t *aValuePtr, uint16_t aValueLen)
{
    bool value = false;
    spinel_ssize_t parsedLength;
    otError error = OT_ERROR_NONE;

    parsedLength = spinel_datatype_unpack(
                       aValuePtr,
                       aValueLen,
                       SPINEL_DATATYPE_BOOL_S,
                       &value
                   );

    VerifyOrExit(parsedLength > 0, error = OT_ERROR_PARSE);

#if OPENTHREAD_ENABLE_RAW_LINK_API

    if (otLinkRawIsEnabled(mInstance))
    {
        if (value)
        {
            error = otLinkRawReceive(mInstance, mCurReceiveChannel, &NcpBase::LinkRawReceiveDone);
        }
        else
        {
            error = otLinkRawSleep(mInstance);
        }
    }

#endif // OPENTHREAD_ENABLE_RAW_LINK_API

    mIsRawStreamEnabled = value;

exit:
    return SendSetPropertyResponse(aHeader, aKey, error);
}

otError NcpBase::GetPropertyHandler_UNSOL_UPDATE_FILTER(uint8_t aHeader, spinel_prop_key_t aKey)
{
    otError error = OT_ERROR_NONE;
    uint8_t numEntries;
    const ChangedPropsSet::Entry *entry;

    SuccessOrExit(error = OutboundFrameBegin(aHeader));
    SuccessOrExit(
        error = OutboundFrameFeedPacked(
                    SPINEL_DATATYPE_COMMAND_PROP_S,
                    aHeader,
                    SPINEL_CMD_PROP_VALUE_IS,
                    aKey
                ));

    entry = mChangedPropsSet.GetSupportedEntries(numEntries);

    for (uint8_t index = 0; index < numEntries; index++, entry++)
    {
        if (mChangedPropsSet.IsEntryFiltered(index))
        {
            SuccessOrExit(error = OutboundFrameFeedPacked(SPINEL_DATATYPE_UINT_PACKED_S, entry->mPropKey));
        }
    }

    SuccessOrExit(error = OutboundFrameSend());

exit:
    return error;
}

otError NcpBase::SetPropertyHandler_UNSOL_UPDATE_FILTER(uint8_t aHeader, spinel_prop_key_t aKey,
                                                        const uint8_t *aValuePtr, uint16_t aValueLen)
{
    spinel_ssize_t parsedLength;
    unsigned int propKey;
    otError error = OT_ERROR_NONE;
    bool reportAsync;

    // First clear the current filter.
    mChangedPropsSet.ClearFilter();

    while (aValueLen > 0)
    {
        parsedLength = spinel_datatype_unpack(
                           aValuePtr,
                           aValueLen,
                           SPINEL_DATATYPE_UINT_PACKED_S,
                           &propKey
                       );

        VerifyOrExit(parsedLength > 0, error = OT_ERROR_PARSE);

        IgnoreReturnValue(mChangedPropsSet.EnablePropertyFilter(static_cast<spinel_prop_key_t>(propKey), true));

        aValuePtr += parsedLength;
        aValueLen -= parsedLength;
    }

exit:
    // If we had an error, we may have actually changed
    // the state of the filter---so we need to report
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

otError NcpBase::InsertPropertyHandler_UNSOL_UPDATE_FILTER(uint8_t aHeader, spinel_prop_key_t aKey,
                                                           const uint8_t *aValuePtr, uint16_t aValueLen)
{
    spinel_ssize_t parsedLength;
    otError error = OT_ERROR_NONE;
    spinel_status_t spinelError = SPINEL_STATUS_OK;
    unsigned int propKey;

    parsedLength = spinel_datatype_unpack(
                       aValuePtr,
                       aValueLen,
                       SPINEL_DATATYPE_UINT_PACKED_S,
                       &propKey
                   );

    VerifyOrExit(parsedLength > 0, spinelError = SPINEL_STATUS_PARSE_ERROR);

    error = mChangedPropsSet.EnablePropertyFilter(static_cast<spinel_prop_key_t>(propKey), true);
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

otError NcpBase::RemovePropertyHandler_UNSOL_UPDATE_FILTER(uint8_t aHeader, spinel_prop_key_t aKey,
                                                           const uint8_t *aValuePtr, uint16_t aValueLen)
{
    spinel_ssize_t parsedLength;
    otError error = OT_ERROR_NONE;
    spinel_status_t spinelError = SPINEL_STATUS_OK;
    unsigned int propKey;

    parsedLength = spinel_datatype_unpack(
                       aValuePtr,
                       aValueLen,
                       SPINEL_DATATYPE_UINT_PACKED_S,
                       &propKey
                   );

    VerifyOrExit(parsedLength > 0, spinelError = SPINEL_STATUS_PARSE_ERROR);

    error = mChangedPropsSet.EnablePropertyFilter(static_cast<spinel_prop_key_t>(propKey), false);
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

otError NcpBase::GetPropertyHandler_LAST_STATUS(uint8_t aHeader, spinel_prop_key_t aKey)
{
    return SendPropertyUpdate(aHeader, SPINEL_CMD_PROP_VALUE_IS, aKey, SPINEL_DATATYPE_UINT_PACKED_S, mLastStatus);
}

otError NcpBase::GetPropertyHandler_PROTOCOL_VERSION(uint8_t aHeader, spinel_prop_key_t aKey)
{
    return SendPropertyUpdate(
               aHeader,
               SPINEL_CMD_PROP_VALUE_IS,
               aKey,
               (
                   SPINEL_DATATYPE_UINT_PACKED_S   // Major
                   SPINEL_DATATYPE_UINT_PACKED_S   // Minor
               ),
               SPINEL_PROTOCOL_VERSION_THREAD_MAJOR,
               SPINEL_PROTOCOL_VERSION_THREAD_MINOR
           );
}

otError NcpBase::GetPropertyHandler_INTERFACE_TYPE(uint8_t aHeader, spinel_prop_key_t aKey)
{
    return SendPropertyUpdate(
               aHeader,
               SPINEL_CMD_PROP_VALUE_IS,
               aKey,
               SPINEL_DATATYPE_UINT_PACKED_S,
               SPINEL_PROTOCOL_TYPE_THREAD
           );
}

otError NcpBase::GetPropertyHandler_VENDOR_ID(uint8_t aHeader, spinel_prop_key_t aKey)
{
    return SendPropertyUpdate(
               aHeader,
               SPINEL_CMD_PROP_VALUE_IS,
               aKey,
               SPINEL_DATATYPE_UINT_PACKED_S,
               0 // Vendor ID. Zero for unknown.
           );
}

otError NcpBase::GetPropertyHandler_CAPS(uint8_t aHeader, spinel_prop_key_t aKey)
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

    // Begin adding capabilities //////////////////////////////////////////////

    SuccessOrExit(error = OutboundFrameFeedPacked(SPINEL_DATATYPE_UINT_PACKED_S, SPINEL_CAP_COUNTERS));
    SuccessOrExit(error = OutboundFrameFeedPacked(SPINEL_DATATYPE_UINT_PACKED_S, SPINEL_CAP_UNSOL_UPDATE_FILTER));

#if OPENTHREAD_ENABLE_RAW_LINK_API
    SuccessOrExit(error = OutboundFrameFeedPacked(SPINEL_DATATYPE_UINT_PACKED_S, SPINEL_CAP_MAC_RAW));
#endif

#if OPENTHREAD_MTD || OPENTHREAD_FTD
    SuccessOrExit(error = OutboundFrameFeedPacked(SPINEL_DATATYPE_UINT_PACKED_S, SPINEL_CAP_NET_THREAD_1_0));
#if OPENTHREAD_ENABLE_MAC_FILTER
    SuccessOrExit(error = OutboundFrameFeedPacked(SPINEL_DATATYPE_UINT_PACKED_S, SPINEL_CAP_MAC_WHITELIST));
#endif

#if OPENTHREAD_ENABLE_JAM_DETECTION
    SuccessOrExit(error = OutboundFrameFeedPacked(SPINEL_DATATYPE_UINT_PACKED_S, SPINEL_CAP_JAM_DETECT));
#endif

#if OPENTHREAD_CONFIG_ENABLE_STEERING_DATA_SET_OOB
    SuccessOrExit(error = OutboundFrameFeedPacked(SPINEL_DATATYPE_UINT_PACKED_S, SPINEL_CAP_OOB_STEERING_DATA));
#endif

#if OPENTHREAD_CONFIG_NCP_ENABLE_PEEK_POKE
    SuccessOrExit(error = OutboundFrameFeedPacked(SPINEL_DATATYPE_UINT_PACKED_S, SPINEL_CAP_PEEK_POKE));
#endif

    // TODO: Somehow get the following capability from the radio.
    SuccessOrExit(error = OutboundFrameFeedPacked(SPINEL_DATATYPE_UINT_PACKED_S, SPINEL_CAP_802_15_4_2450MHZ_OQPSK));

#if OPENTHREAD_CONFIG_MAX_CHILDREN > 0
    SuccessOrExit(error = OutboundFrameFeedPacked(SPINEL_DATATYPE_UINT_PACKED_S, SPINEL_CAP_ROLE_ROUTER));
#endif

    SuccessOrExit(error = OutboundFrameFeedPacked(SPINEL_DATATYPE_UINT_PACKED_S, SPINEL_CAP_ROLE_SLEEPY));

#if OPENTHREAD_ENABLE_LEGACY
    SuccessOrExit(error = OutboundFrameFeedPacked(SPINEL_DATATYPE_UINT_PACKED_S, SPINEL_CAP_NEST_LEGACY_INTERFACE));
#endif

#if OPENTHREAD_ENABLE_TMF_PROXY
    SuccessOrExit(error = OutboundFrameFeedPacked(SPINEL_DATATYPE_UINT_PACKED_S, SPINEL_CAP_THREAD_TMF_PROXY));
#endif

#endif // OPENTHREAD_MTD || OPENTHREAD_FTD
    // End adding capabilities /////////////////////////////////////////////////

    SuccessOrExit(error = OutboundFrameSend());

exit:
    return error;
}

otError NcpBase::GetPropertyHandler_NCP_VERSION(uint8_t aHeader, spinel_prop_key_t aKey)
{
    return SendPropertyUpdate(
               aHeader,
               SPINEL_CMD_PROP_VALUE_IS,
               aKey,
               SPINEL_DATATYPE_UTF8_S,
               otGetVersionString()
           );
}

otError NcpBase::GetPropertyHandler_INTERFACE_COUNT(uint8_t aHeader, spinel_prop_key_t aKey)
{
    return SendPropertyUpdate(
               aHeader,
               SPINEL_CMD_PROP_VALUE_IS,
               aKey,
               SPINEL_DATATYPE_UINT8_S,
               1 // Only one interface for now
           );
}

otError NcpBase::GetPropertyHandler_POWER_STATE(uint8_t aHeader, spinel_prop_key_t aKey)
{
    // Always online at the moment
    return SendPropertyUpdate(
               aHeader,
               SPINEL_CMD_PROP_VALUE_IS,
               aKey,
               SPINEL_DATATYPE_UINT8_S,
               SPINEL_POWER_STATE_ONLINE
           );
}

otError NcpBase::SetPropertyHandler_POWER_STATE(uint8_t aHeader, spinel_prop_key_t aKey, const uint8_t *aValuePtr,
                                                uint16_t aValueLen)
{
    // TODO: Implement POWER_STATE
    OT_UNUSED_VARIABLE(aKey);
    OT_UNUSED_VARIABLE(aValuePtr);
    OT_UNUSED_VARIABLE(aValueLen);

    return SendLastStatus(aHeader, SPINEL_STATUS_UNIMPLEMENTED);
}

otError NcpBase::GetPropertyHandler_HWADDR(uint8_t aHeader, spinel_prop_key_t aKey)
{
    otExtAddress hwAddr;
    otPlatRadioGetIeeeEui64(mInstance, hwAddr.m8);

    return SendPropertyUpdate(
               aHeader,
               SPINEL_CMD_PROP_VALUE_IS,
               aKey,
               SPINEL_DATATYPE_EUI64_S,
               hwAddr.m8
           );
}

otError NcpBase::GetPropertyHandler_LOCK(uint8_t aHeader, spinel_prop_key_t aKey)
{
    // TODO: Implement property lock (Needs API!)
    OT_UNUSED_VARIABLE(aKey);

    return SendLastStatus(aHeader, SPINEL_STATUS_UNIMPLEMENTED);
}

otError NcpBase::GetPropertyHandler_HOST_POWER_STATE(uint8_t aHeader, spinel_prop_key_t aKey)
{
    return SendPropertyUpdate(
               aHeader,
               SPINEL_CMD_PROP_VALUE_IS,
               aKey,
               SPINEL_DATATYPE_UINT8_S,
               mHostPowerState
           );
}

otError NcpBase::SetPropertyHandler_HOST_POWER_STATE(uint8_t aHeader, spinel_prop_key_t aKey, const uint8_t *aValuePtr,
                                                     uint16_t aValueLen)
{
    uint8_t value;
    spinel_ssize_t parsedLength;
    otError error = OT_ERROR_NONE;

    parsedLength = spinel_datatype_unpack(
                       aValuePtr,
                       aValueLen,
                       SPINEL_DATATYPE_UINT8_S,
                       &value
                   );

    if (parsedLength > 0)
    {
        switch (value)
        {
        case SPINEL_HOST_POWER_STATE_OFFLINE:
        case SPINEL_HOST_POWER_STATE_DEEP_SLEEP:
        case SPINEL_HOST_POWER_STATE_LOW_POWER:
        case SPINEL_HOST_POWER_STATE_ONLINE:
            // Adopt the requested power state.
            mHostPowerState = static_cast<spinel_host_power_state_t>(value);
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
    }
    else
    {
        error = OT_ERROR_PARSE;
    }

    if (error == OT_ERROR_NONE)
    {
        mHostPowerStateHeader = 0;

        error = HandleCommandPropertyGet(aHeader, aKey);

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

otError NcpBase::GetPropertyHandler_UNSOL_UPDATE_LIST(uint8_t aHeader, spinel_prop_key_t aKey)
{
    otError error = OT_ERROR_NONE;
    uint8_t numEntries;
    const ChangedPropsSet::Entry *entry;

    SuccessOrExit(error = OutboundFrameBegin(aHeader));
    SuccessOrExit(
        error = OutboundFrameFeedPacked(
                    SPINEL_DATATYPE_COMMAND_PROP_S,
                    aHeader,
                    SPINEL_CMD_PROP_VALUE_IS,
                    aKey
                ));

    entry = mChangedPropsSet.GetSupportedEntries(numEntries);

    for (uint8_t index = 0; index < numEntries; index++, entry++)
    {
        if (entry->mFilterable)
        {
            SuccessOrExit(error = OutboundFrameFeedPacked(SPINEL_DATATYPE_UINT_PACKED_S, entry->mPropKey));
        }
    }

    SuccessOrExit(error = OutboundFrameSend());

exit:
    return error;
}

otError NcpBase::GetPropertyHandler_PHY_RX_SENSITIVITY(uint8_t aHeader, spinel_prop_key_t aKey)
{
    return SendPropertyUpdate(
               aHeader,
               SPINEL_CMD_PROP_VALUE_IS,
               aKey,
               SPINEL_DATATYPE_INT8_S,
               otPlatRadioGetReceiveSensitivity(mInstance)
           );
}

otError NcpBase::GetPropertyHandler_PHY_TX_POWER(uint8_t aHeader, spinel_prop_key_t aKey)
{
    return SendPropertyUpdate(
               aHeader,
               SPINEL_CMD_PROP_VALUE_IS,
               aKey,
               SPINEL_DATATYPE_INT8_S,
               otLinkGetMaxTransmitPower(mInstance)
           );
}

otError NcpBase::SetPropertyHandler_PHY_TX_POWER(uint8_t aHeader, spinel_prop_key_t aKey, const uint8_t *aValuePtr,
                                                 uint16_t aValueLen)
{
    int8_t value = 0;
    spinel_ssize_t parsedLength;
    otError error = OT_ERROR_NONE;

    parsedLength = spinel_datatype_unpack(
                       aValuePtr,
                       aValueLen,
                       SPINEL_DATATYPE_INT8_S,
                       &value
                   );

    VerifyOrExit(parsedLength > 0, error = OT_ERROR_PARSE);

    otLinkSetMaxTransmitPower(mInstance, value);

exit:
    return SendSetPropertyResponse(aHeader, aKey, error);
}

otError NcpBase::GetPropertyHandler_DEBUG_TEST_ASSERT(uint8_t aHeader, spinel_prop_key_t aKey)
{
    assert(false);

    // We only get to this point if `assert(false)`
    // does not cause an NCP reset on the platform.
    // In such a case we return `false` as the
    // property value to indicate this.

    OT_UNREACHABLE_CODE(
        return SendPropertyUpdate(
                aHeader,
                SPINEL_CMD_PROP_VALUE_IS,
                aKey,
                SPINEL_DATATYPE_BOOL_S,
                false
            );
    )
}

otError NcpBase::GetPropertyHandler_DEBUG_NCP_LOG_LEVEL(uint8_t aHeader, spinel_prop_key_t aKey)
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

    return SendPropertyUpdate(
               aHeader,
               SPINEL_CMD_PROP_VALUE_IS,
               aKey,
               SPINEL_DATATYPE_UINT8_S,
               logLevel
           );
}

otError NcpBase::SetPropertyHandler_DEBUG_NCP_LOG_LEVEL(uint8_t aHeader, spinel_prop_key_t aKey,
                                                        const uint8_t *aValuePtr, uint16_t aValueLen)
{
    uint8_t spinelNcpLogLevel = 0;
    otLogLevel logLevel;
    spinel_ssize_t parsedLength;
    otError error = OT_ERROR_NONE;

    parsedLength = spinel_datatype_unpack(
                       aValuePtr,
                       aValueLen,
                       SPINEL_DATATYPE_UINT8_S,
                       &spinelNcpLogLevel
                   );

    VerifyOrExit(parsedLength > 0, error = OT_ERROR_PARSE);

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
    return SendSetPropertyResponse(aHeader, aKey, error);
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
