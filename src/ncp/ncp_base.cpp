/*
 *    Copyright (c) 2016, The OpenThread Authors.
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

#include <openthread/config.h>

#include "ncp_base.hpp"

#include <stdarg.h>
#include <stdlib.h>

#if OPENTHREAD_ENABLE_TMF_PROXY && OPENTHREAD_FTD
#include <openthread/tmf_proxy.h>
#endif

#if OPENTHREAD_ENABLE_BORDER_ROUTER
#include <openthread/border_router.h>
#endif

#if OPENTHREAD_ENABLE_COMMISSIONER && OPENTHREAD_FTD
#include <meshcop/commissioner.hpp>
#endif

#include <openthread/diag.h>
#include <openthread/icmp6.h>

#if OPENTHREAD_ENABLE_JAM_DETECTION
#include <openthread/jam_detection.h>
#endif

#include <openthread/ncp.h>
#include <openthread/openthread.h>

#if OPENTHREAD_FTD
#include <openthread/thread_ftd.h>
#endif

#include <openthread/platform/misc.h>
#include <openthread/platform/radio.h>

#include "openthread-instance.h"
#include "common/code_utils.hpp"
#include "common/debug.hpp"
#include "net/ip6.hpp"

namespace ot {

#define NCP_INVALID_SCAN_CHANNEL              (-1)

#define NCP_CHANGED_PLATFORM_RESET            (1U << 31)
#define NCP_CHANGED_THREAD_ON_MESH_NETS       (1U << 30)
#define NCP_CHANGED_THREAD_OFF_MESH_ROUTES    (1U << 29)

#define RSSI_OVERRIDE_DISABLED        127 // Used for PROP_MAC_WHITELIST

#define IGNORE_RETURN_VALUE(s)        do { if (s){} } while (0)

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
    NCP_CMD_HANDLER_ENTRY(NET_SAVE),
    NCP_CMD_HANDLER_ENTRY(NET_CLEAR),
    NCP_CMD_HANDLER_ENTRY(NET_RECALL),
#if OPENTHREAD_CONFIG_NCP_ENABLE_PEEK_POKE
    NCP_CMD_HANDLER_ENTRY(PEEK),
    NCP_CMD_HANDLER_ENTRY(POKE),
#endif
};

#define NCP_GET_PROP_HANDLER_ENTRY(name)                 { SPINEL_PROP_##name, &NcpBase::GetPropertyHandler_##name }
#define NCP_GET_PROP_HANDLER_ENTRY_METHOD(name, method)  { SPINEL_PROP_##name, &NcpBase::GetPropertyHandler_##method }

const NcpBase::GetPropertyHandlerEntry NcpBase::mGetPropertyHandlerTable[] =
{
    NCP_GET_PROP_HANDLER_ENTRY(LAST_STATUS),
    NCP_GET_PROP_HANDLER_ENTRY(PROTOCOL_VERSION),
    NCP_GET_PROP_HANDLER_ENTRY(INTERFACE_TYPE),
    NCP_GET_PROP_HANDLER_ENTRY(VENDOR_ID),
    NCP_GET_PROP_HANDLER_ENTRY(CAPS),
    NCP_GET_PROP_HANDLER_ENTRY(NCP_VERSION),
    NCP_GET_PROP_HANDLER_ENTRY(INTERFACE_COUNT),
    NCP_GET_PROP_HANDLER_ENTRY(POWER_STATE),
    NCP_GET_PROP_HANDLER_ENTRY(HWADDR),
    NCP_GET_PROP_HANDLER_ENTRY(LOCK),
    NCP_GET_PROP_HANDLER_ENTRY(HOST_POWER_STATE),
    NCP_GET_PROP_HANDLER_ENTRY(PHY_ENABLED),
    NCP_GET_PROP_HANDLER_ENTRY(PHY_FREQ),
    NCP_GET_PROP_HANDLER_ENTRY(PHY_CHAN_SUPPORTED),
    NCP_GET_PROP_HANDLER_ENTRY(PHY_CHAN),
    NCP_GET_PROP_HANDLER_ENTRY(PHY_RSSI),
    NCP_GET_PROP_HANDLER_ENTRY(PHY_TX_POWER),
    NCP_GET_PROP_HANDLER_ENTRY(PHY_RX_SENSITIVITY),
    NCP_GET_PROP_HANDLER_ENTRY(MAC_SCAN_STATE),
    NCP_GET_PROP_HANDLER_ENTRY(MAC_SCAN_MASK),
    NCP_GET_PROP_HANDLER_ENTRY(MAC_SCAN_PERIOD),
    NCP_GET_PROP_HANDLER_ENTRY(MAC_15_4_PANID),
    NCP_GET_PROP_HANDLER_ENTRY(MAC_15_4_LADDR),
    NCP_GET_PROP_HANDLER_ENTRY(MAC_15_4_SADDR),
    NCP_GET_PROP_HANDLER_ENTRY(MAC_RAW_STREAM_ENABLED),
    NCP_GET_PROP_HANDLER_ENTRY(MAC_PROMISCUOUS_MODE),
    NCP_GET_PROP_HANDLER_ENTRY(MAC_EXTENDED_ADDR),
    NCP_GET_PROP_HANDLER_ENTRY(NET_SAVED),
    NCP_GET_PROP_HANDLER_ENTRY(NET_IF_UP),
    NCP_GET_PROP_HANDLER_ENTRY(NET_STACK_UP),
    NCP_GET_PROP_HANDLER_ENTRY(NET_ROLE),
    NCP_GET_PROP_HANDLER_ENTRY(NET_NETWORK_NAME),
    NCP_GET_PROP_HANDLER_ENTRY(NET_XPANID),
    NCP_GET_PROP_HANDLER_ENTRY(NET_MASTER_KEY),
    NCP_GET_PROP_HANDLER_ENTRY(NET_KEY_SEQUENCE_COUNTER),
    NCP_GET_PROP_HANDLER_ENTRY(NET_PARTITION_ID),
    NCP_GET_PROP_HANDLER_ENTRY(NET_KEY_SWITCH_GUARDTIME),
    NCP_GET_PROP_HANDLER_ENTRY(THREAD_LEADER_ADDR),
    NCP_GET_PROP_HANDLER_ENTRY(THREAD_PARENT),
    NCP_GET_PROP_HANDLER_ENTRY(THREAD_NEIGHBOR_TABLE),
    NCP_GET_PROP_HANDLER_ENTRY(THREAD_LEADER_RID),
    NCP_GET_PROP_HANDLER_ENTRY(THREAD_LEADER_WEIGHT),
#if OPENTHREAD_ENABLE_BORDER_ROUTER
    NCP_GET_PROP_HANDLER_ENTRY(THREAD_NETWORK_DATA),
    NCP_GET_PROP_HANDLER_ENTRY(THREAD_STABLE_NETWORK_DATA),
#endif
    NCP_GET_PROP_HANDLER_ENTRY(THREAD_NETWORK_DATA_VERSION),
    NCP_GET_PROP_HANDLER_ENTRY(THREAD_STABLE_NETWORK_DATA_VERSION),
    NCP_GET_PROP_HANDLER_ENTRY(THREAD_LEADER_NETWORK_DATA),
    NCP_GET_PROP_HANDLER_ENTRY(THREAD_STABLE_LEADER_NETWORK_DATA),
    NCP_GET_PROP_HANDLER_ENTRY(THREAD_OFF_MESH_ROUTES),
    NCP_GET_PROP_HANDLER_ENTRY(THREAD_ASSISTING_PORTS),
    NCP_GET_PROP_HANDLER_ENTRY(THREAD_ALLOW_LOCAL_NET_DATA_CHANGE),
#if OPENTHREAD_ENABLE_COMMISSIONER && OPENTHREAD_FTD
    NCP_GET_PROP_HANDLER_ENTRY(THREAD_COMMISSIONER_ENABLED),
#endif
#if OPENTHREAD_ENABLE_MAC_WHITELIST
    NCP_GET_PROP_HANDLER_ENTRY(MAC_WHITELIST),
    NCP_GET_PROP_HANDLER_ENTRY(MAC_WHITELIST_ENABLED),
    NCP_GET_PROP_HANDLER_ENTRY(MAC_BLACKLIST),
    NCP_GET_PROP_HANDLER_ENTRY(MAC_BLACKLIST_ENABLED),
#endif
    NCP_GET_PROP_HANDLER_ENTRY(THREAD_MODE),
    NCP_GET_PROP_HANDLER_ENTRY(THREAD_CHILD_TIMEOUT),
    NCP_GET_PROP_HANDLER_ENTRY(THREAD_RLOC16),
    NCP_GET_PROP_HANDLER_ENTRY(THREAD_ON_MESH_NETS),
    NCP_GET_PROP_HANDLER_ENTRY(NET_REQUIRE_JOIN_EXISTING),
    NCP_GET_PROP_HANDLER_ENTRY(IPV6_ML_PREFIX),
    NCP_GET_PROP_HANDLER_ENTRY(IPV6_ML_ADDR),
    NCP_GET_PROP_HANDLER_ENTRY(IPV6_LL_ADDR),
    NCP_GET_PROP_HANDLER_ENTRY(IPV6_ADDRESS_TABLE),
    NCP_GET_PROP_HANDLER_ENTRY(IPV6_ROUTE_TABLE),
    NCP_GET_PROP_HANDLER_ENTRY(IPV6_ICMP_PING_OFFLOAD),
    NCP_GET_PROP_HANDLER_ENTRY(THREAD_RLOC16_DEBUG_PASSTHRU),
    NCP_GET_PROP_HANDLER_ENTRY(THREAD_DISCOVERY_SCAN_JOINER_FLAG),
    NCP_GET_PROP_HANDLER_ENTRY(THREAD_DISCOVERY_SCAN_ENABLE_FILTERING),
    NCP_GET_PROP_HANDLER_ENTRY(THREAD_DISCOVERY_SCAN_PANID),
    NCP_GET_PROP_HANDLER_ENTRY(STREAM_NET),
#if OPENTHREAD_FTD
    NCP_GET_PROP_HANDLER_ENTRY(THREAD_CHILD_TABLE),
    NCP_GET_PROP_HANDLER_ENTRY(THREAD_LOCAL_LEADER_WEIGHT),
    NCP_GET_PROP_HANDLER_ENTRY(THREAD_ROUTER_ROLE_ENABLED),
    NCP_GET_PROP_HANDLER_ENTRY(NET_PSKC),
    NCP_GET_PROP_HANDLER_ENTRY(THREAD_CHILD_COUNT_MAX),
    NCP_GET_PROP_HANDLER_ENTRY(THREAD_ROUTER_UPGRADE_THRESHOLD),
    NCP_GET_PROP_HANDLER_ENTRY(THREAD_ROUTER_DOWNGRADE_THRESHOLD),
    NCP_GET_PROP_HANDLER_ENTRY(THREAD_CONTEXT_REUSE_DELAY),
    NCP_GET_PROP_HANDLER_ENTRY(THREAD_NETWORK_ID_TIMEOUT),
    NCP_GET_PROP_HANDLER_ENTRY(THREAD_ROUTER_SELECTION_JITTER),
#endif
#if OPENTHREAD_ENABLE_JAM_DETECTION
    NCP_GET_PROP_HANDLER_ENTRY(JAM_DETECT_ENABLE),
    NCP_GET_PROP_HANDLER_ENTRY(JAM_DETECTED),
    NCP_GET_PROP_HANDLER_ENTRY(JAM_DETECT_RSSI_THRESHOLD),
    NCP_GET_PROP_HANDLER_ENTRY(JAM_DETECT_WINDOW),
    NCP_GET_PROP_HANDLER_ENTRY(JAM_DETECT_BUSY),
    NCP_GET_PROP_HANDLER_ENTRY(JAM_DETECT_HISTORY_BITMAP),
#endif
#if OPENTHREAD_ENABLE_TMF_PROXY && OPENTHREAD_FTD
    NCP_GET_PROP_HANDLER_ENTRY(THREAD_TMF_PROXY_ENABLED),
#endif
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

    NCP_GET_PROP_HANDLER_ENTRY(TX_TOTAL_TIME),
    NCP_GET_PROP_HANDLER_ENTRY(RX_TOTAL_TIME),

    NCP_GET_PROP_HANDLER_ENTRY(MSG_BUFFER_COUNTERS),
    NCP_GET_PROP_HANDLER_ENTRY(DEBUG_TEST_ASSERT),
    NCP_GET_PROP_HANDLER_ENTRY(DEBUG_NCP_LOG_LEVEL),
#if OPENTHREAD_ENABLE_LEGACY
    NCP_GET_PROP_HANDLER_ENTRY(NEST_LEGACY_ULA_PREFIX),
#endif
};

#define NCP_SET_PROP_HANDLER_ENTRY(name)  { SPINEL_PROP_##name, &NcpBase::SetPropertyHandler_##name }

const NcpBase::SetPropertyHandlerEntry NcpBase::mSetPropertyHandlerTable[] =
{
    NCP_SET_PROP_HANDLER_ENTRY(POWER_STATE),
    NCP_SET_PROP_HANDLER_ENTRY(HOST_POWER_STATE),
#if OPENTHREAD_ENABLE_RAW_LINK_API
    NCP_SET_PROP_HANDLER_ENTRY(PHY_ENABLED),
    NCP_SET_PROP_HANDLER_ENTRY(MAC_15_4_SADDR),
    NCP_SET_PROP_HANDLER_ENTRY(STREAM_RAW),
#endif // OPENTHREAD_ENABLE_RAW_LINK_API
    NCP_SET_PROP_HANDLER_ENTRY(PHY_TX_POWER),
    NCP_SET_PROP_HANDLER_ENTRY(PHY_CHAN),
    NCP_SET_PROP_HANDLER_ENTRY(MAC_PROMISCUOUS_MODE),
    NCP_SET_PROP_HANDLER_ENTRY(MAC_SCAN_MASK),
    NCP_SET_PROP_HANDLER_ENTRY(MAC_SCAN_STATE),
    NCP_SET_PROP_HANDLER_ENTRY(MAC_SCAN_PERIOD),
    NCP_SET_PROP_HANDLER_ENTRY(MAC_15_4_PANID),
    NCP_SET_PROP_HANDLER_ENTRY(MAC_15_4_LADDR),
    NCP_SET_PROP_HANDLER_ENTRY(MAC_RAW_STREAM_ENABLED),
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
#if OPENTHREAD_ENABLE_MAC_WHITELIST
    NCP_SET_PROP_HANDLER_ENTRY(MAC_WHITELIST),
    NCP_SET_PROP_HANDLER_ENTRY(MAC_WHITELIST_ENABLED),
    NCP_SET_PROP_HANDLER_ENTRY(MAC_BLACKLIST),
    NCP_SET_PROP_HANDLER_ENTRY(MAC_BLACKLIST_ENABLED),
#endif
#if OPENTHREAD_ENABLE_RAW_LINK_API
    NCP_SET_PROP_HANDLER_ENTRY(MAC_SRC_MATCH_ENABLED),
    NCP_SET_PROP_HANDLER_ENTRY(MAC_SRC_MATCH_SHORT_ADDRESSES),
    NCP_SET_PROP_HANDLER_ENTRY(MAC_SRC_MATCH_EXTENDED_ADDRESSES),
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
#endif // #if OPENTHREAD_FTD
#if OPENTHREAD_ENABLE_JAM_DETECTION
    NCP_SET_PROP_HANDLER_ENTRY(JAM_DETECT_ENABLE),
    NCP_SET_PROP_HANDLER_ENTRY(JAM_DETECT_RSSI_THRESHOLD),
    NCP_SET_PROP_HANDLER_ENTRY(JAM_DETECT_WINDOW),
    NCP_SET_PROP_HANDLER_ENTRY(JAM_DETECT_BUSY),
#endif
#if OPENTHREAD_ENABLE_TMF_PROXY && OPENTHREAD_FTD
    NCP_SET_PROP_HANDLER_ENTRY(THREAD_TMF_PROXY_ENABLED),
    NCP_SET_PROP_HANDLER_ENTRY(THREAD_TMF_PROXY_STREAM),
#endif
#if OPENTHREAD_ENABLE_DIAG
    NCP_SET_PROP_HANDLER_ENTRY(NEST_STREAM_MFG),
#endif
#if OPENTHREAD_ENABLE_LEGACY
    NCP_SET_PROP_HANDLER_ENTRY(NEST_LEGACY_ULA_PREFIX),
#endif

#if OPENTHREAD_ENABLE_COMMISSIONER && OPENTHREAD_FTD
    NCP_SET_PROP_HANDLER_ENTRY(THREAD_COMMISSIONER_ENABLED),
#endif
    NCP_SET_PROP_HANDLER_ENTRY(CNTR_RESET),
};

#define NCP_INSERT_PROP_HANDLER_ENTRY(name)  { SPINEL_PROP_##name, &NcpBase::InsertPropertyHandler_##name }

const NcpBase::InsertPropertyHandlerEntry NcpBase::mInsertPropertyHandlerTable[] =
{
#if OPENTHREAD_ENABLE_RAW_LINK_API
    NCP_INSERT_PROP_HANDLER_ENTRY(MAC_SRC_MATCH_SHORT_ADDRESSES),
    NCP_INSERT_PROP_HANDLER_ENTRY(MAC_SRC_MATCH_EXTENDED_ADDRESSES),
#endif
    NCP_INSERT_PROP_HANDLER_ENTRY(IPV6_ADDRESS_TABLE),
    NCP_INSERT_PROP_HANDLER_ENTRY(THREAD_ASSISTING_PORTS),
#if OPENTHREAD_ENABLE_BORDER_ROUTER
    NCP_INSERT_PROP_HANDLER_ENTRY(THREAD_OFF_MESH_ROUTES),
    NCP_INSERT_PROP_HANDLER_ENTRY(THREAD_ON_MESH_NETS),
#endif
#if OPENTHREAD_ENABLE_COMMISSIONER && OPENTHREAD_FTD
    NCP_INSERT_PROP_HANDLER_ENTRY(THREAD_JOINERS),
#endif
#if OPENTHREAD_ENABLE_MAC_WHITELIST
    NCP_INSERT_PROP_HANDLER_ENTRY(MAC_WHITELIST),
    NCP_INSERT_PROP_HANDLER_ENTRY(MAC_BLACKLIST),
#endif
};

#define NCP_REMOVE_PROP_HANDLER_ENTRY(name)  { SPINEL_PROP_##name, &NcpBase::RemovePropertyHandler_##name }

const NcpBase::RemovePropertyHandlerEntry NcpBase::mRemovePropertyHandlerTable[] =
{
#if OPENTHREAD_ENABLE_RAW_LINK_API
    NCP_REMOVE_PROP_HANDLER_ENTRY(MAC_SRC_MATCH_SHORT_ADDRESSES),
    NCP_REMOVE_PROP_HANDLER_ENTRY(MAC_SRC_MATCH_EXTENDED_ADDRESSES),
#endif
    NCP_REMOVE_PROP_HANDLER_ENTRY(IPV6_ADDRESS_TABLE),
#if OPENTHREAD_ENABLE_BORDER_ROUTER
    NCP_REMOVE_PROP_HANDLER_ENTRY(THREAD_OFF_MESH_ROUTES),
    NCP_REMOVE_PROP_HANDLER_ENTRY(THREAD_ON_MESH_NETS),
#endif
    NCP_REMOVE_PROP_HANDLER_ENTRY(THREAD_ASSISTING_PORTS),
#if OPENTHREAD_ENABLE_MAC_WHITELIST
    NCP_REMOVE_PROP_HANDLER_ENTRY(MAC_WHITELIST),
    NCP_REMOVE_PROP_HANDLER_ENTRY(MAC_BLACKLIST),
#endif
#if OPENTHREAD_FTD
    NCP_REMOVE_PROP_HANDLER_ENTRY(THREAD_ACTIVE_ROUTER_IDS),
#endif
};

// ----------------------------------------------------------------------------
// MARK: Utility Functions
// ----------------------------------------------------------------------------

static spinel_status_t ThreadErrorToSpinelStatus(otError aError)
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

static uint8_t LinkFlagsToFlagByte(bool aRxOnWhenIdle, bool aSecureDataRequests, bool aDeviceType, bool aNetworkData)
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

#endif // OPENTHREAD_ENABLE_BORDER_ROUTER

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
    mUpdateChangedPropsTask(aInstance->mIp6.mTaskletScheduler, &NcpBase::UpdateChangedProps, this),
    mChangedFlags(NCP_CHANGED_PLATFORM_RESET),
    mShouldSignalEndOfScan(false),
    mHostPowerState(SPINEL_HOST_POWER_STATE_ONLINE),
    mHostPowerStateInProgress(false),
    mHostPowerReplyFrameTag(NcpFrameBuffer::kInvalidTag),
    mHostPowerStateHeader(0),
#if OPENTHREAD_ENABLE_JAM_DETECTION
    mShouldSignalJamStateChange(false),
#endif
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
    mCurScanChannel(NCP_INVALID_SCAN_CHANNEL),
#endif // OPENTHREAD_ENABLE_RAW_LINK_API

    mFramingErrorCounter(0),
    mRxSpinelFrameCounter(0),
    mRxSpinelOutOfOrderTidCounter(0),
    mTxSpinelFrameCounter(0),
    mInboundSecureIpFrameCounter(0),
    mInboundInsecureIpFrameCounter(0),
    mOutboundSecureIpFrameCounter(0),
    mOutboundInsecureIpFrameCounter(0),
    mDroppedOutboundIpFrameCounter(0),
    mDroppedInboundIpFrameCounter(0)
{
    assert(mInstance != NULL);

    sNcpInstance = this;

    mTxFrameBuffer.SetFrameRemovedCallback(&NcpBase::HandleFrameRemovedFromNcpBuffer, this);

    otSetStateChangedCallback(mInstance, &NcpBase::HandleNetifStateChanged, this);
    otIp6SetReceiveCallback(mInstance, &NcpBase::HandleDatagramFromStack, this);
    otIp6SetReceiveFilterEnabled(mInstance, true);
    otLinkSetPcapCallback(mInstance, &NcpBase::HandleRawFrame, static_cast<void *>(this));
    otIcmp6SetEchoEnabled(mInstance, false);

    mUpdateChangedPropsTask.Post();

#if OPENTHREAD_ENABLE_LEGACY
    mLegacyNodeDidJoin = false;
    mLegacyHandlers = NULL;
    memset(mLegacyUlaPrefix, 0, sizeof(mLegacyUlaPrefix));
#endif
}

NcpBase *NcpBase::GetNcpInstance(void)
{
    return sNcpInstance;
}

// ----------------------------------------------------------------------------
// MARK: Outbound Frame methods
// ----------------------------------------------------------------------------

otError NcpBase::OutboundFrameBegin(void)
{
    return mTxFrameBuffer.InFrameBegin();
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

#if OPENTHREAD_ENABLE_TMF_PROXY && OPENTHREAD_FTD
void NcpBase::HandleTmfProxyStream(otMessage *aMessage, uint16_t aLocator, uint16_t aPort, void *aContext)
{
    static_cast<NcpBase *>(aContext)->HandleTmfProxyStream(aMessage, aLocator, aPort);
}

void NcpBase::HandleTmfProxyStream(otMessage *aMessage, uint16_t aLocator, uint16_t aPort)
{
    otError error = OT_ERROR_NONE;
    uint16_t length = otMessageGetLength(aMessage);

    SuccessOrExit(error = OutboundFrameBegin());

    SuccessOrExit(
        error = OutboundFrameFeedPacked(
                    SPINEL_DATATYPE_COMMAND_PROP_S SPINEL_DATATYPE_UINT16_S,
                    SPINEL_HEADER_FLAG | SPINEL_HEADER_IID_0,
                    SPINEL_CMD_PROP_VALUE_IS,
                    SPINEL_PROP_THREAD_TMF_PROXY_STREAM,
                    length
                ));

    SuccessOrExit(error = OutboundFrameFeedMessage(aMessage));

    SuccessOrExit(
        error = OutboundFrameFeedPacked(
                    SPINEL_DATATYPE_UINT16_S SPINEL_DATATYPE_UINT16_S,
                    aLocator,
                    aPort
                ));

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

    if (error != OT_ERROR_NONE)
    {
        SendLastStatus(SPINEL_HEADER_FLAG | SPINEL_HEADER_IID_0, SPINEL_STATUS_DROPPED);
    }
}
#endif // OPENTHREAD_ENABLE_TMF_PROXY && OPENTHREAD_FTD

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
    bool isSecure = otMessageIsLinkSecurityEnabled(aMessage);
    uint16_t length = otMessageGetLength(aMessage);

    SuccessOrExit(error = OutboundFrameBegin());

    SuccessOrExit(
        error = OutboundFrameFeedPacked(
                    SPINEL_DATATYPE_COMMAND_PROP_S SPINEL_DATATYPE_UINT16_S,
                    SPINEL_HEADER_FLAG | SPINEL_HEADER_IID_0,
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
// MARK: Raw frame handling
// ----------------------------------------------------------------------------

void NcpBase::HandleRawFrame(const otRadioFrame *aFrame, void *aContext)
{
    static_cast<NcpBase *>(aContext)->HandleRawFrame(aFrame);
}

void NcpBase::HandleRawFrame(const otRadioFrame *aFrame)
{
    uint16_t flags = 0;

    if (!mIsRawStreamEnabled)
    {
        goto exit;
    }

    SuccessOrExit(OutboundFrameBegin());

    if (aFrame->mDidTX)
    {
        flags |= SPINEL_MD_FLAG_TX;
    }

    // Append frame header and frame length
    SuccessOrExit(
        OutboundFrameFeedPacked(
            SPINEL_DATATYPE_COMMAND_PROP_S SPINEL_DATATYPE_UINT16_S,
            SPINEL_HEADER_FLAG | SPINEL_HEADER_IID_0,
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
// MARK: Scan Results Glue
// ----------------------------------------------------------------------------

void NcpBase::HandleActiveScanResult_Jump(otActiveScanResult *aResult, void *aContext)
{
    static_cast<NcpBase *>(aContext)->HandleActiveScanResult(aResult);
}

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

        SendPropertyUpdate(
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
        // buffer space), we set `mShouldSignalEndOfScan` to true to send
        // it out when buffer space becomes available.
        if (error != OT_ERROR_NONE)
        {
            mShouldSignalEndOfScan = true;
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
        SendPropertyUpdate(
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
        // buffer space), we set `mShouldSignalEndOfScan` to true to send
        // it out when buffer space becomes available.
        if (error != OT_ERROR_NONE)
        {
            mShouldSignalEndOfScan = true;
        }
    }
}

// ----------------------------------------------------------------------------
// MARK: Raw Link-Layer Datapath Glue
// ----------------------------------------------------------------------------

#if OPENTHREAD_ENABLE_RAW_LINK_API

void NcpBase::LinkRawReceiveDone(otInstance *, otRadioFrame *aFrame, otError aError)
{
    sNcpInstance->LinkRawReceiveDone(aFrame, aError);
}

void NcpBase::LinkRawReceiveDone(otRadioFrame *aFrame, otError aError)
{
    uint16_t flags = 0;

    SuccessOrExit(OutboundFrameBegin());

    if (aFrame->mDidTX)
    {
        flags |= SPINEL_MD_FLAG_TX;
    }

    // Append frame header and frame length
    SuccessOrExit(
        OutboundFrameFeedPacked(
            SPINEL_DATATYPE_COMMAND_PROP_S SPINEL_DATATYPE_UINT16_S,
            SPINEL_HEADER_FLAG | SPINEL_HEADER_IID_0,
            SPINEL_CMD_PROP_VALUE_IS,
            SPINEL_PROP_STREAM_RAW,
            (aError == OT_ERROR_NONE) ? aFrame->mLength : 0
        ));

    if (aError == OT_ERROR_NONE)
    {
        // Append the frame contents
        SuccessOrExit(OutboundFrameFeedData(aFrame->mPsdu, aFrame->mLength));
    }

    // Append metadata (rssi, etc)
    SuccessOrExit(
        OutboundFrameFeedPacked(
            SPINEL_DATATYPE_INT8_S
            SPINEL_DATATYPE_INT8_S
            SPINEL_DATATYPE_UINT16_S
            SPINEL_DATATYPE_STRUCT_S( // PHY-data
                SPINEL_DATATYPE_UINT8_S // 802.15.4 channel
                SPINEL_DATATYPE_UINT8_S // 802.15.4 LQI
            )
            SPINEL_DATATYPE_STRUCT_S( // Vendor-data
                SPINEL_DATATYPE_UINT_PACKED_S
            ),
            aFrame->mPower,    // TX Power
            -128,              // Noise Floor (Currently unused)
            flags,             // Flags
            aFrame->mChannel,  // Receive channel
            aFrame->mLqi,      // Link quality indicator
            aError             // Receive error
        ));

    SuccessOrExit(OutboundFrameSend());

exit:
    return;
}

void NcpBase::LinkRawTransmitDone(otInstance *, otRadioFrame *aFrame, bool aFramePending, otError aError)
{
    sNcpInstance->LinkRawTransmitDone(aFrame, aFramePending, aError);
}

void NcpBase::LinkRawTransmitDone(otRadioFrame *, bool aFramePending, otError aError)
{
    if (mCurTransmitTID)
    {
        SendPropertyUpdate(
            SPINEL_HEADER_FLAG | SPINEL_HEADER_IID_0 | mCurTransmitTID,
            SPINEL_CMD_PROP_VALUE_IS,
            SPINEL_PROP_LAST_STATUS,
            SPINEL_DATATYPE_UINT_PACKED_S SPINEL_DATATYPE_BOOL_S,
            ThreadErrorToSpinelStatus(aError),
            aFramePending
        );

        // Clear cached transmit TID
        mCurTransmitTID = 0;
    }
}

void NcpBase::LinkRawEnergyScanDone(otInstance *, int8_t aEnergyScanMaxRssi)
{
    sNcpInstance->LinkRawEnergyScanDone(aEnergyScanMaxRssi);
}

void NcpBase::LinkRawEnergyScanDone(int8_t aEnergyScanMaxRssi)
{
    SendPropertyUpdate(
        SPINEL_HEADER_FLAG | SPINEL_HEADER_IID_0,
        SPINEL_CMD_PROP_VALUE_IS,
        SPINEL_PROP_MAC_ENERGY_SCAN_RESULT,
        SPINEL_DATATYPE_UINT8_S
        SPINEL_DATATYPE_INT8_S,
        mCurScanChannel,
        aEnergyScanMaxRssi
    );

    // Clear current scan channel
    mCurScanChannel = NCP_INVALID_SCAN_CHANNEL;

    // Make sure we are back listening on the original receive channel,
    // since the energy scan could have been on a different channel.
    otLinkRawReceive(mInstance, mCurReceiveChannel, &NcpBase::LinkRawReceiveDone);

    // We are finished with the scan, so send out
    // a property update indicating such.
    SendPropertyUpdate(
        SPINEL_HEADER_FLAG | SPINEL_HEADER_IID_0,
        SPINEL_CMD_PROP_VALUE_IS,
        SPINEL_PROP_MAC_SCAN_STATE,
        SPINEL_DATATYPE_UINT8_S,
        SPINEL_SCAN_STATE_IDLE
    );
}

#endif // OPENTHREAD_ENABLE_RAW_LINK_API

// ----------------------------------------------------------------------------
// MARK: Address Table Changed Glue
// ----------------------------------------------------------------------------

void NcpBase::HandleNetifStateChanged(uint32_t aFlags, void *aContext)
{
    NcpBase *obj = static_cast<NcpBase *>(aContext);

    obj->mChangedFlags |= aFlags;

    obj->mUpdateChangedPropsTask.Post();
}

void NcpBase::UpdateChangedProps(void *aContext)
{
    NcpBase *obj = static_cast<NcpBase *>(aContext);
    obj->UpdateChangedProps();
}

void NcpBase::UpdateChangedProps(void)
{
    while (mChangedFlags != 0)
    {
        if ((mChangedFlags & NCP_CHANGED_PLATFORM_RESET) != 0)
        {
            SuccessOrExit(
                SendLastStatus(
                    SPINEL_HEADER_FLAG | SPINEL_HEADER_IID_0,
                    ResetReasonToSpinelStatus(otPlatGetResetReason(mInstance))
                ));

            mChangedFlags &= ~static_cast<uint32_t>(NCP_CHANGED_PLATFORM_RESET);
        }
        else if ((mChangedFlags & OT_CHANGED_THREAD_LL_ADDR) != 0)
        {
            SuccessOrExit(
                HandleCommandPropertyGet(
                    SPINEL_HEADER_FLAG | SPINEL_HEADER_IID_0,
                    SPINEL_PROP_IPV6_LL_ADDR
                ));

            mChangedFlags &= ~static_cast<uint32_t>(OT_CHANGED_THREAD_LL_ADDR);
        }
        else if ((mChangedFlags & OT_CHANGED_THREAD_ML_ADDR) != 0)
        {
            SuccessOrExit(
                HandleCommandPropertyGet(
                    SPINEL_HEADER_FLAG | SPINEL_HEADER_IID_0,
                    SPINEL_PROP_IPV6_ML_ADDR
                ));

            mChangedFlags &= ~static_cast<uint32_t>(OT_CHANGED_THREAD_ML_ADDR);
        }
        else if ((mChangedFlags & OT_CHANGED_THREAD_ROLE) != 0)
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
                    break;
                }

                if ((otThreadGetDeviceRole(mInstance) == OT_DEVICE_ROLE_LEADER)
                  && otThreadIsSingleton(mInstance)
#if OPENTHREAD_ENABLE_LEGACY
                    && !mLegacyNodeDidJoin
#endif
                   )
                {
                    mChangedFlags &= ~static_cast<uint32_t>(OT_CHANGED_THREAD_PARTITION_ID);
                    otThreadSetEnabled(mInstance, false);

                    // TODO: It would be nice to be able to indicate
                    //   something more specific than SPINEL_STATUS_JOIN_FAILURE
                    //   here, but it isn't clear how that would work
                    //   with the current OpenThread API.

                    SuccessOrExit(
                        SendLastStatus(
                            SPINEL_HEADER_FLAG | SPINEL_HEADER_IID_0,
                            SPINEL_STATUS_JOIN_FAILURE
                        ));

                    SuccessOrExit(
                        HandleCommandPropertyGet(
                            SPINEL_HEADER_FLAG | SPINEL_HEADER_IID_0,
                            SPINEL_PROP_NET_STACK_UP
                        ));
                }

                SuccessOrExit(
                    HandleCommandPropertyGet(
                        SPINEL_HEADER_FLAG | SPINEL_HEADER_IID_0,
                        SPINEL_PROP_NET_REQUIRE_JOIN_EXISTING
                    ));
            }

            SuccessOrExit(
                HandleCommandPropertyGet(
                    SPINEL_HEADER_FLAG | SPINEL_HEADER_IID_0,
                    SPINEL_PROP_NET_ROLE
                ));

            mChangedFlags &= ~static_cast<uint32_t>(OT_CHANGED_THREAD_ROLE);
        }
        else if ((mChangedFlags & OT_CHANGED_THREAD_PARTITION_ID) != 0)
        {
            SuccessOrExit(
                HandleCommandPropertyGet(
                    SPINEL_HEADER_FLAG | SPINEL_HEADER_IID_0,
                    SPINEL_PROP_NET_PARTITION_ID
                ));

            mChangedFlags &= ~static_cast<uint32_t>(OT_CHANGED_THREAD_PARTITION_ID);
        }
        else if ((mChangedFlags & OT_CHANGED_THREAD_KEY_SEQUENCE_COUNTER) != 0)
        {
            SuccessOrExit(
                HandleCommandPropertyGet(
                    SPINEL_HEADER_FLAG | SPINEL_HEADER_IID_0,
                    SPINEL_PROP_NET_KEY_SEQUENCE_COUNTER
                ));

            mChangedFlags &= ~static_cast<uint32_t>(OT_CHANGED_THREAD_KEY_SEQUENCE_COUNTER);
        }
        else if ((mChangedFlags & (OT_CHANGED_IP6_ADDRESS_ADDED | OT_CHANGED_IP6_ADDRESS_REMOVED)) != 0)
        {
            SuccessOrExit(
                HandleCommandPropertyGet(
                    SPINEL_HEADER_FLAG | SPINEL_HEADER_IID_0,
                    SPINEL_PROP_IPV6_ADDRESS_TABLE
                ));

            mChangedFlags &= ~static_cast<uint32_t>(OT_CHANGED_IP6_ADDRESS_ADDED | OT_CHANGED_IP6_ADDRESS_REMOVED);
        }
        else if ((mChangedFlags & (OT_CHANGED_THREAD_CHILD_ADDED | OT_CHANGED_THREAD_CHILD_REMOVED)) != 0)
        {
            SuccessOrExit(
                HandleCommandPropertyGet(
                    SPINEL_HEADER_FLAG | SPINEL_HEADER_IID_0,
                    SPINEL_PROP_THREAD_CHILD_TABLE
                ));

            mChangedFlags &= ~static_cast<uint32_t>(OT_CHANGED_THREAD_CHILD_ADDED | OT_CHANGED_THREAD_CHILD_REMOVED);
        }
        else if ((mChangedFlags & OT_CHANGED_THREAD_NETDATA) != 0)
        {
            SuccessOrExit(
                HandleCommandPropertyGet(
                    SPINEL_HEADER_FLAG | SPINEL_HEADER_IID_0,
                    SPINEL_PROP_THREAD_LEADER_NETWORK_DATA
                ));

            mChangedFlags &= ~static_cast<uint32_t>(OT_CHANGED_THREAD_NETDATA);

            // If the network data is updated, after successfully sending (or queuing) the
            // network data spinel message, we add `NCP_CHANGED_THREAD_ON_MESH_NETS` and
            // `NCP_CHANGED_THREAD_OFF_MESH_ROUTES` to the `mChangedFlags` so that we
            // separately send the list of on-mesh prefixes and off-mesh routes.

            mChangedFlags |= NCP_CHANGED_THREAD_ON_MESH_NETS | NCP_CHANGED_THREAD_OFF_MESH_ROUTES;
        }
        else if ((mChangedFlags & NCP_CHANGED_THREAD_ON_MESH_NETS) != 0)
        {
            SuccessOrExit(
                HandleCommandPropertyGet(
                    SPINEL_HEADER_FLAG | SPINEL_HEADER_IID_0,
                    SPINEL_PROP_THREAD_ON_MESH_NETS
                ));

            mChangedFlags &= ~static_cast<uint32_t>(NCP_CHANGED_THREAD_ON_MESH_NETS);
        }
        else if ((mChangedFlags & NCP_CHANGED_THREAD_OFF_MESH_ROUTES) != 0)
        {
            SuccessOrExit(
                HandleCommandPropertyGet(
                    SPINEL_HEADER_FLAG | SPINEL_HEADER_IID_0,
                    SPINEL_PROP_THREAD_OFF_MESH_ROUTES
                ));

            mChangedFlags &= ~static_cast<uint32_t>(NCP_CHANGED_THREAD_OFF_MESH_ROUTES);
        }
        else if ((mChangedFlags & (OT_CHANGED_THREAD_RLOC_ADDED | OT_CHANGED_THREAD_RLOC_REMOVED)) != 0)
        {
            mChangedFlags &= ~static_cast<uint32_t>(OT_CHANGED_THREAD_RLOC_ADDED | OT_CHANGED_THREAD_RLOC_REMOVED);
        }
    }

exit:
    return;
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
                                              NcpFrameBuffer *aNcpBuffer)
{
    OT_UNUSED_VARIABLE(aNcpBuffer);
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

#if OPENTHREAD_ENABLE_JAM_DETECTION

    if (mShouldSignalJamStateChange)
    {
        SuccessOrExit(
            SendPropertyUpdate(
                SPINEL_HEADER_FLAG | SPINEL_HEADER_IID_0,
                SPINEL_CMD_PROP_VALUE_IS,
                SPINEL_PROP_JAM_DETECTED,
                SPINEL_DATATYPE_BOOL_S,
                otJamDetectionGetState(mInstance)
            ));

        mShouldSignalJamStateChange = false;
    }

#endif  // OPENTHREAD_ENABLE_JAM_DETECTION

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
    SuccessOrExit(error = OutboundFrameBegin());
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

    SuccessOrExit(error = OutboundFrameBegin());
    SuccessOrExit(error = OutboundFrameFeedPacked(SPINEL_DATATYPE_COMMAND_PROP_S, aHeader, aCommand, aKey));
    SuccessOrExit(error = OutboundFrameFeedData(aValuePtr, aValueLen));
    SuccessOrExit(error = OutboundFrameSend());

exit:
    return error;
}

otError NcpBase::SendPropertyUpdate(uint8_t aHeader, uint8_t aCommand, spinel_prop_key_t aKey, otMessage *aMessage)
{
    otError error = OT_ERROR_NONE;

    SuccessOrExit(error = OutboundFrameBegin());
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

    // We only get to this point if the
    // platform doesn't support resetting.
    // In such a case we fake it.

    otThreadSetEnabled(mInstance, false);
    otIp6SetEnabled(mInstance, false);

    error = SendLastStatus(SPINEL_HEADER_FLAG | SPINEL_HEADER_IID_0, SPINEL_STATUS_RESET_SOFTWARE);

    if (error != OT_ERROR_NONE)
    {
        mChangedFlags |= NCP_CHANGED_PLATFORM_RESET;
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

    SuccessOrExit(error = OutboundFrameBegin());
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
// MARK: Individual Property Getters
// ----------------------------------------------------------------------------


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

    SuccessOrExit(error = OutboundFrameBegin());
    SuccessOrExit(
        error = OutboundFrameFeedPacked(
                    SPINEL_DATATYPE_COMMAND_PROP_S,
                    aHeader,
                    SPINEL_CMD_PROP_VALUE_IS,
                    aKey
                ));

    // Begin adding capabilities //////////////////////////////////////////////

    SuccessOrExit(error = OutboundFrameFeedPacked(SPINEL_DATATYPE_UINT_PACKED_S, SPINEL_CAP_NET_THREAD_1_0));
    SuccessOrExit(error = OutboundFrameFeedPacked(SPINEL_DATATYPE_UINT_PACKED_S, SPINEL_CAP_COUNTERS));

#if OPENTHREAD_ENABLE_MAC_WHITELIST
    SuccessOrExit(error = OutboundFrameFeedPacked(SPINEL_DATATYPE_UINT_PACKED_S, SPINEL_CAP_MAC_WHITELIST));
#endif

#if OPENTHREAD_ENABLE_RAW_LINK_API
    SuccessOrExit(error = OutboundFrameFeedPacked(SPINEL_DATATYPE_UINT_PACKED_S, SPINEL_CAP_MAC_RAW));
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

#if OPENTHREAD_ENABLE_LEGACY
    SuccessOrExit(error = OutboundFrameFeedPacked(SPINEL_DATATYPE_UINT_PACKED_S, SPINEL_CAP_NEST_LEGACY_INTERFACE));
#endif

#if OPENTHREAD_ENABLE_TMF_PROXY && OPENTHREAD_FTD
    SuccessOrExit(error = OutboundFrameFeedPacked(SPINEL_DATATYPE_UINT_PACKED_S, SPINEL_CAP_THREAD_TMF_PROXY));
#endif

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

otError NcpBase::GetPropertyHandler_HWADDR(uint8_t aHeader, spinel_prop_key_t aKey)
{
    otExtAddress hwAddr;
    otLinkGetFactoryAssignedIeeeEui64(mInstance, &hwAddr);

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
                        mCurScanChannel == NCP_INVALID_SCAN_CHANNEL
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

otError NcpBase::GetPropertyHandler_ChannelMaskHelper(uint8_t aHeader, spinel_prop_key_t aKey, uint32_t aChannelMask)
{
    otError error = OT_ERROR_NONE;

    SuccessOrExit(error = OutboundFrameBegin());
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

    SuccessOrExit(error = OutboundFrameBegin());
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

    SuccessOrExit(error = OutboundFrameBegin());
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

    SuccessOrExit(error = OutboundFrameBegin());
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

    SuccessOrExit(error = OutboundFrameBegin());
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

#if OPENTHREAD_FTD
otError NcpBase::GetPropertyHandler_THREAD_LOCAL_LEADER_WEIGHT(uint8_t aHeader, spinel_prop_key_t aKey)
{
    return SendPropertyUpdate(
               aHeader,
               SPINEL_CMD_PROP_VALUE_IS,
               aKey,
               SPINEL_DATATYPE_UINT8_S,
               otThreadGetLocalLeaderWeight(mInstance)
           );
}
#endif  // OPENTHREAD_FTD

otError NcpBase::GetPropertyHandler_THREAD_LEADER_WEIGHT(uint8_t aHeader, spinel_prop_key_t aKey)
{
    return SendPropertyUpdate(
               aHeader,
               SPINEL_CMD_PROP_VALUE_IS,
               aKey,
               SPINEL_DATATYPE_UINT8_S,
               otThreadGetLeaderWeight(mInstance)
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

#if OPENTHREAD_FTD
otError NcpBase::GetPropertyHandler_THREAD_CHILD_TABLE(uint8_t aHeader, spinel_prop_key_t aKey)
{
    otError error = OT_ERROR_NONE;
    otChildInfo childInfo;
    uint8_t maxChildren;
    uint8_t modeFlags;

    mDisableStreamWrite = true;

    SuccessOrExit(error = OutboundFrameBegin());
    SuccessOrExit(
        error = OutboundFrameFeedPacked(
                    SPINEL_DATATYPE_COMMAND_PROP_S,
                    aHeader,
                    SPINEL_CMD_PROP_VALUE_IS,
                    aKey
                ));

    maxChildren = otThreadGetMaxAllowedChildren(mInstance);

    for (uint8_t index = 0; index < maxChildren; index++)
    {
        error = otThreadGetChildInfoByIndex(mInstance, index, &childInfo);

        if (error != OT_ERROR_NONE)
        {
            continue;
        }

        modeFlags = LinkFlagsToFlagByte(childInfo.mRxOnWhenIdle,
                                        childInfo.mSecureDataRequest,
                                        childInfo.mFullFunction,
                                        childInfo.mFullNetworkData);

        SuccessOrExit(
            error = OutboundFrameFeedPacked(
                        SPINEL_DATATYPE_STRUCT_S(
                            SPINEL_DATATYPE_EUI64_S         // EUI64 Address
                            SPINEL_DATATYPE_UINT16_S        // Rloc16
                            SPINEL_DATATYPE_UINT32_S        // Timeout
                            SPINEL_DATATYPE_UINT32_S        // Age
                            SPINEL_DATATYPE_UINT8_S         // Network Data Version
                            SPINEL_DATATYPE_UINT8_S         // Link Quality In
                            SPINEL_DATATYPE_INT8_S          // Average RSS
                            SPINEL_DATATYPE_UINT8_S         // Mode (flags)
                            SPINEL_DATATYPE_INT8_S          // Most recent RSS
                        ),
                        childInfo.mExtAddress.m8,
                        childInfo.mRloc16,
                        childInfo.mTimeout,
                        childInfo.mAge,
                        childInfo.mNetworkDataVersion,
                        childInfo.mLinkQualityIn,
                        childInfo.mAverageRssi,
                        modeFlags,
                        childInfo.mLastRssi
                    ));
    }

    SuccessOrExit(error = OutboundFrameSend());

exit:
    mDisableStreamWrite = false;
    return error;
}
#endif  // OPENTHREAD_FTD

otError NcpBase::GetPropertyHandler_THREAD_NEIGHBOR_TABLE(uint8_t aHeader, spinel_prop_key_t aKey)
{
    otError error = OT_ERROR_NONE;
    otNeighborInfoIterator iter = OT_NEIGHBOR_INFO_ITERATOR_INIT;
    otNeighborInfo neighInfo;
    uint8_t modeFlags;

    mDisableStreamWrite = true;

    SuccessOrExit(error = OutboundFrameBegin());
    SuccessOrExit(error = OutboundFrameFeedPacked(SPINEL_DATATYPE_COMMAND_PROP_S, aHeader, SPINEL_CMD_PROP_VALUE_IS,
                                                      aKey));

    while (otThreadGetNextNeighborInfo(mInstance, &iter, &neighInfo) == OT_ERROR_NONE)
    {
        modeFlags = LinkFlagsToFlagByte(neighInfo.mRxOnWhenIdle,
                                        neighInfo.mSecureDataRequest,
                                        neighInfo.mFullFunction,
                                        neighInfo.mFullNetworkData);

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

    SuccessOrExit(error = OutboundFrameBegin());
    SuccessOrExit(error = OutboundFrameFeedPacked(SPINEL_DATATYPE_COMMAND_PROP_S, aHeader, SPINEL_CMD_PROP_VALUE_IS,
                                                      aKey));

    for (; numEntries != 0; ports++, numEntries--)
    {
        SuccessOrExit(error = OutboundFrameFeedPacked(SPINEL_DATATYPE_UINT16_S, *ports));
    }

    SuccessOrExit(error = OutboundFrameSend());

exit:
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

#if OPENTHREAD_FTD
otError NcpBase::GetPropertyHandler_THREAD_ROUTER_ROLE_ENABLED(uint8_t aHeader, spinel_prop_key_t aKey)
{
    return SendPropertyUpdate(
               aHeader,
               SPINEL_CMD_PROP_VALUE_IS,
               aKey,
               SPINEL_DATATYPE_BOOL_S,
               otThreadIsRouterRoleEnabled(mInstance)
           );
}
#endif  // OPENTHREAD_FTD

otError NcpBase::GetPropertyHandler_THREAD_ON_MESH_NETS(uint8_t aHeader, spinel_prop_key_t aKey)
{
    otError error = OT_ERROR_NONE;
    otBorderRouterConfig borderRouterConfig;
    uint8_t flags;

    mDisableStreamWrite = true;

    SuccessOrExit(error = OutboundFrameBegin());
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

        SuccessOrExit(error = OutboundFrameFeedPacked(
                                  SPINEL_DATATYPE_STRUCT_S(
                                      SPINEL_DATATYPE_IPv6ADDR_S      // IPv6 Prefix
                                      SPINEL_DATATYPE_UINT8_S         // Prefix Length (in bits)
                                      SPINEL_DATATYPE_BOOL_S          // isStable
                                      SPINEL_DATATYPE_UINT8_S         // Flags
                                      SPINEL_DATATYPE_BOOL_S          // isLocal
                                  ),
                                  &borderRouterConfig.mPrefix,
                                  64,
                                  borderRouterConfig.mStable,
                                  flags,
                                  false
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

        SuccessOrExit(error = OutboundFrameFeedPacked(
                                  SPINEL_DATATYPE_STRUCT_S(
                                      SPINEL_DATATYPE_IPv6ADDR_S      // IPv6 Prefix
                                      SPINEL_DATATYPE_UINT8_S         // Prefix Length (in bits)
                                      SPINEL_DATATYPE_BOOL_S          // isStable
                                      SPINEL_DATATYPE_UINT8_S         // Flags
                                      SPINEL_DATATYPE_BOOL_S          // isLocal
                                  ),
                                  &borderRouterConfig.mPrefix,
                                  64,
                                  borderRouterConfig.mStable,
                                  flags,
                                  true
                              ));
    }
#endif // OPENTHREAD_ENABLE_BORDER_ROUTER

    SuccessOrExit(error = OutboundFrameSend());

exit:
    mDisableStreamWrite = false;
    return error;
}

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

    SuccessOrExit(error = OutboundFrameBegin());
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

otError NcpBase::GetPropertyHandler_THREAD_OFF_MESH_ROUTES(uint8_t aHeader, spinel_prop_key_t aKey)
{
    otError error = OT_ERROR_NONE;
    otExternalRouteConfig external_route_config;
    otNetworkDataIterator iter = OT_NETWORK_DATA_ITERATOR_INIT;

    mDisableStreamWrite = true;

    SuccessOrExit(error = OutboundFrameBegin());
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
                        ),
                        &external_route_config.mPrefix.mPrefix,
                        external_route_config.mPrefix.mLength,
                        external_route_config.mStable,
                        ExternalRoutePreferenceToFlagByte(external_route_config.mPreference),
                        false,
                        external_route_config.mNextHopIsThisDevice
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
                        ),
                        &external_route_config.mPrefix.mPrefix,
                        external_route_config.mPrefix.mLength,
                        external_route_config.mStable,
                        ExternalRoutePreferenceToFlagByte(external_route_config.mPreference),
                        true,
                        external_route_config.mNextHopIsThisDevice
                    ));
    }
#endif // OPENTHREAD_ENABLE_BORDER_ROUTER

    SuccessOrExit(error = OutboundFrameSend());

exit:
    mDisableStreamWrite = false;
    return error;
}

otError NcpBase::GetPropertyHandler_STREAM_NET(uint8_t aHeader, spinel_prop_key_t aKey)
{
    // TODO: Implement explicit data poll.
    OT_UNUSED_VARIABLE(aKey);

    return SendLastStatus(aHeader, SPINEL_STATUS_UNIMPLEMENTED);
}

#if OPENTHREAD_ENABLE_TMF_PROXY && OPENTHREAD_FTD
otError NcpBase::GetPropertyHandler_THREAD_TMF_PROXY_ENABLED(uint8_t aHeader, spinel_prop_key_t aKey)
{
    return SendPropertyUpdate(
               aHeader,
               SPINEL_CMD_PROP_VALUE_IS,
               aKey,
               SPINEL_DATATYPE_BOOL_S,
               otTmfProxyIsEnabled(mInstance)
           );
}
#endif // OPENTHREAD_ENABLE_TMF_PROXY && OPENTHREAD_FTD

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

    SuccessOrExit(error = OutboundFrameBegin());
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

#if OPENTHREAD_ENABLE_MAC_WHITELIST

otError NcpBase::GetPropertyHandler_MAC_WHITELIST(uint8_t aHeader, spinel_prop_key_t aKey)
{
    otMacWhitelistEntry entry;
    otError error = OT_ERROR_NONE;

    mDisableStreamWrite = true;

    SuccessOrExit(error = OutboundFrameBegin());
    SuccessOrExit(
        error = OutboundFrameFeedPacked(
                    SPINEL_DATATYPE_COMMAND_PROP_S,
                    aHeader,
                    SPINEL_CMD_PROP_VALUE_IS,
                    aKey
                ));

    for (uint8_t i = 0; (i != 255) && (error == OT_ERROR_NONE); i++)
    {
        error = otLinkGetWhitelistEntry(mInstance, i, &entry);

        if (error != OT_ERROR_NONE)
        {
            break;
        }

        if (entry.mValid)
        {
            if (!entry.mFixedRssi)
            {
                entry.mRssi = RSSI_OVERRIDE_DISABLED;
            }

            SuccessOrExit(
                error = OutboundFrameFeedPacked(
                            SPINEL_DATATYPE_STRUCT_S(
                                SPINEL_DATATYPE_EUI64_S   // Extended address
                                SPINEL_DATATYPE_INT8_S    // Rssi
                            ),
                            entry.mExtAddress.m8,
                            entry.mRssi
                        ));
        }
    }

    SuccessOrExit(error = OutboundFrameSend());

exit:
    mDisableStreamWrite = false;
    return error;
}

otError NcpBase::GetPropertyHandler_MAC_WHITELIST_ENABLED(uint8_t aHeader, spinel_prop_key_t aKey)
{
    return SendPropertyUpdate(
               aHeader,
               SPINEL_CMD_PROP_VALUE_IS,
               aKey,
               SPINEL_DATATYPE_BOOL_S,
               otLinkIsWhitelistEnabled(mInstance)
           );
}

otError NcpBase::GetPropertyHandler_MAC_BLACKLIST(uint8_t aHeader, spinel_prop_key_t aKey)
{
    otMacBlacklistEntry entry;
    otError error = OT_ERROR_NONE;

    mDisableStreamWrite = true;

    SuccessOrExit(error = OutboundFrameBegin());
    SuccessOrExit(
        error = OutboundFrameFeedPacked(
                    SPINEL_DATATYPE_COMMAND_PROP_S,
                    aHeader,
                    SPINEL_CMD_PROP_VALUE_IS,
                    aKey
                ));

    for (uint8_t i = 0; (i != 255) && (error == OT_ERROR_NONE); i++)
    {
        error = otLinkGetBlacklistEntry(mInstance, i, &entry);

        if (error != OT_ERROR_NONE)
        {
            break;
        }

        if (entry.mValid)
        {
            SuccessOrExit(
                error = OutboundFrameFeedPacked(
                            SPINEL_DATATYPE_STRUCT_S(
                                SPINEL_DATATYPE_EUI64_S   // Extended address
                            ),
                            entry.mExtAddress.m8
                        ));
        }
    }

    SuccessOrExit(error = OutboundFrameSend());

exit:
    mDisableStreamWrite = false;
    return error;
}

otError NcpBase::GetPropertyHandler_MAC_BLACKLIST_ENABLED(uint8_t aHeader, spinel_prop_key_t aKey)
{
    return SendPropertyUpdate(
               aHeader,
               SPINEL_CMD_PROP_VALUE_IS,
               aKey,
               SPINEL_DATATYPE_BOOL_S,
               otLinkIsBlacklistEnabled(mInstance)
           );
}


#endif // OPENTHREAD_ENABLE_MAC_WHITELIST

#if OPENTHREAD_FTD
otError NcpBase::GetPropertyHandler_NET_PSKC(uint8_t aHeader, spinel_prop_key_t aKey)
{
    return SendPropertyUpdate(
               aHeader,
               SPINEL_CMD_PROP_VALUE_IS,
               aKey,
               SPINEL_DATATYPE_DATA_S,
               otThreadGetPSKc(mInstance),
               sizeof(spinel_net_pskc_t)
           );
}
#endif // OPENTHREAD_FTD

otError NcpBase::GetPropertyHandler_THREAD_MODE(uint8_t aHeader, spinel_prop_key_t aKey)
{
    uint8_t numericMode;
    otLinkModeConfig modeConfig = otThreadGetLinkMode(mInstance);

    numericMode = LinkFlagsToFlagByte(modeConfig.mRxOnWhenIdle,
                                      modeConfig.mSecureDataRequests,
                                      modeConfig.mDeviceType,
                                      modeConfig.mNetworkData);

    return SendPropertyUpdate(
               aHeader,
               SPINEL_CMD_PROP_VALUE_IS,
               aKey,
               SPINEL_DATATYPE_UINT8_S,
               numericMode
           );
}

#if OPENTHREAD_FTD
otError NcpBase::GetPropertyHandler_THREAD_CHILD_COUNT_MAX(uint8_t aHeader, spinel_prop_key_t aKey)
{
    return SendPropertyUpdate(
               aHeader,
               SPINEL_CMD_PROP_VALUE_IS,
               aKey,
               SPINEL_DATATYPE_UINT8_S,
               otThreadGetMaxAllowedChildren(mInstance)
           );
}
#endif  // OPENTHREAD_FTD

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

#if OPENTHREAD_FTD
otError NcpBase::GetPropertyHandler_THREAD_ROUTER_UPGRADE_THRESHOLD(uint8_t aHeader, spinel_prop_key_t aKey)
{
    return SendPropertyUpdate(
               aHeader,
               SPINEL_CMD_PROP_VALUE_IS,
               aKey,
               SPINEL_DATATYPE_UINT8_S,
               otThreadGetRouterUpgradeThreshold(mInstance)
           );
}

otError NcpBase::GetPropertyHandler_THREAD_ROUTER_DOWNGRADE_THRESHOLD(uint8_t aHeader, spinel_prop_key_t aKey)
{
    return SendPropertyUpdate(
               aHeader,
               SPINEL_CMD_PROP_VALUE_IS,
               aKey,
               SPINEL_DATATYPE_UINT8_S,
               otThreadGetRouterDowngradeThreshold(mInstance)
           );
}

otError NcpBase::GetPropertyHandler_THREAD_ROUTER_SELECTION_JITTER(uint8_t aHeader, spinel_prop_key_t aKey)
{
    return SendPropertyUpdate(
               aHeader,
               SPINEL_CMD_PROP_VALUE_IS,
               aKey,
               SPINEL_DATATYPE_UINT8_S,
               otThreadGetRouterSelectionJitter(mInstance)
           );
}

otError NcpBase::GetPropertyHandler_THREAD_CONTEXT_REUSE_DELAY(uint8_t aHeader, spinel_prop_key_t aKey)
{
    return SendPropertyUpdate(
               aHeader,
               SPINEL_CMD_PROP_VALUE_IS,
               aKey,
               SPINEL_DATATYPE_UINT32_S,
               otThreadGetContextIdReuseDelay(mInstance)
           );
}

otError NcpBase::GetPropertyHandler_THREAD_NETWORK_ID_TIMEOUT(uint8_t aHeader, spinel_prop_key_t aKey)
{
    return SendPropertyUpdate(
               aHeader,
               SPINEL_CMD_PROP_VALUE_IS,
               aKey,
               SPINEL_DATATYPE_UINT8_S,
               otThreadGetNetworkIdTimeout(mInstance)
           );
}
#endif  // OPENTHREAD_FTD

#if OPENTHREAD_ENABLE_COMMISSIONER && OPENTHREAD_FTD
otError NcpBase::GetPropertyHandler_THREAD_COMMISSIONER_ENABLED(uint8_t aHeader, spinel_prop_key_t aKey)
{
    bool enabled = false;

    if (otCommissionerGetState(mInstance) == OT_COMMISSIONER_STATE_ACTIVE)
    {
        enabled = true;
    }

    return SendPropertyUpdate(
               aHeader,
               SPINEL_CMD_PROP_VALUE_IS,
               aKey,
               SPINEL_DATATYPE_BOOL_S,
               enabled
           );
}
#endif

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
#endif // OPENTHREAD_ENABLE_LEGACY

otError NcpBase::GetPropertyHandler_TX_TOTAL_TIME(uint8_t aHeader, spinel_prop_key_t aKey)
{
    return SendPropertyUpdate(aHeader,
                              SPINEL_CMD_PROP_VALUE_IS,
                              aKey,
                              SPINEL_DATATYPE_UINT32_S,
                              *otThreadGetTxTotalTime(mInstance));
}

otError NcpBase::GetPropertyHandler_RX_TOTAL_TIME(uint8_t aHeader, spinel_prop_key_t aKey)
{
    return SendPropertyUpdate(aHeader,
                              SPINEL_CMD_PROP_VALUE_IS,
                              aKey,
                              SPINEL_DATATYPE_UINT32_S,
                              *otThreadGetRxTotalTime(mInstance));
}

// ----------------------------------------------------------------------------
// MARK: Individual Property Setters
// ----------------------------------------------------------------------------

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

otError NcpBase::SetPropertyHandler_POWER_STATE(uint8_t aHeader, spinel_prop_key_t aKey, const uint8_t *aValuePtr,
                                                uint16_t aValueLen)
{
    // TODO: Implement POWER_STATE
    OT_UNUSED_VARIABLE(aKey);
    OT_UNUSED_VARIABLE(aValuePtr);
    OT_UNUSED_VARIABLE(aValueLen);

    return SendLastStatus(aHeader, SPINEL_STATUS_UNIMPLEMENTED);
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

#if OPENTHREAD_ENABLE_RAW_LINK_API

otError NcpBase::SetPropertyHandler_PHY_ENABLED(uint8_t aHeader, spinel_prop_key_t aKey, const uint8_t *aValuePtr,
                                                uint16_t aValueLen)
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

    if (value == false)
    {
        // If we have raw stream enabled stop receiving
        if (mIsRawStreamEnabled)
        {
            otLinkRawSleep(mInstance);
        }

        error = otLinkRawSetEnable(mInstance, false);
    }
    else
    {
        error = otLinkRawSetEnable(mInstance, true);

        // If we have raw stream enabled already, start receiving
        if (error == OT_ERROR_NONE && mIsRawStreamEnabled)
        {
            error = otLinkRawReceive(mInstance, mCurReceiveChannel, &NcpBase::LinkRawReceiveDone);
        }
    }

exit:
    return SendSetPropertyResponse(aHeader, aKey, error);
}

#endif // OPENTHREAD_ENABLE_RAW_LINK_API

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

    error = otLinkSetChannel(mInstance, static_cast<uint8_t>(channel));

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
        mShouldSignalEndOfScan = false;
        break;

    case SPINEL_SCAN_STATE_ENERGY:
#if OPENTHREAD_ENABLE_RAW_LINK_API
        if (otLinkRawIsEnabled(mInstance))
        {
            uint8_t scanChannel;

            // Make sure we aren't already scanning and that we have
            // only 1 bit set for the channel mask.
            VerifyOrExit(mCurScanChannel == NCP_INVALID_SCAN_CHANNEL, error = OT_ERROR_INVALID_STATE);
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
        mShouldSignalEndOfScan = false;
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
        mShouldSignalEndOfScan = false;
        break;

    default:
        error = OT_ERROR_INVALID_ARGS;
        break;
    }

exit:
    return SendSetPropertyResponse(aHeader, aKey, error);
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

#if OPENTHREAD_ENABLE_RAW_LINK_API

otError NcpBase::SetPropertyHandler_MAC_15_4_SADDR(uint8_t aHeader, spinel_prop_key_t aKey, const uint8_t *aValuePtr,
                                                   uint16_t aValueLen)
{
    uint16_t shortAddress;
    spinel_ssize_t parsedLength;
    otError error = OT_ERROR_NONE;

    parsedLength = spinel_datatype_unpack(
                       aValuePtr,
                       aValueLen,
                       SPINEL_DATATYPE_UINT16_S,
                       &shortAddress
                   );

    VerifyOrExit(parsedLength > 0, error = OT_ERROR_PARSE);

    error = otLinkRawSetShortAddress(mInstance, shortAddress);

exit:
    return SendSetPropertyResponse(aHeader, aKey, error);
}

otError NcpBase::SetPropertyHandler_STREAM_RAW(uint8_t aHeader, spinel_prop_key_t aKey, const uint8_t *aValuePtr,
                                               uint16_t aValueLen)
{
    uint8_t *frame_buffer = NULL;
    otRadioFrame *frame;
    unsigned int frameLen = 0;
    spinel_ssize_t parsedLength;
    otError error = OT_ERROR_NONE;

    VerifyOrExit(otLinkRawIsEnabled(mInstance), error = OT_ERROR_INVALID_STATE);

    frame = otLinkRawGetTransmitBuffer(mInstance);

    parsedLength = spinel_datatype_unpack(
                       aValuePtr,
                       aValueLen,
                       SPINEL_DATATYPE_DATA_WLEN_S
                       SPINEL_DATATYPE_UINT8_S
                       SPINEL_DATATYPE_INT8_S,
                       &frame_buffer,
                       &frameLen,
                       &frame->mChannel,
                       &frame->mPower
                   );

    VerifyOrExit(parsedLength > 0 && frameLen <= OT_RADIO_FRAME_MAX_SIZE, error = OT_ERROR_PARSE);

    // Cache the transaction ID for async response
    mCurTransmitTID = SPINEL_HEADER_GET_TID(aHeader);

    // Update frame buffer and length
    frame->mLength = static_cast<uint8_t>(frameLen);
    memcpy(frame->mPsdu, frame_buffer, frame->mLength);

    // TODO: This should be later added in the STREAM_RAW argument to allow user to directly specify it.
    frame->mMaxTxAttempts = OPENTHREAD_CONFIG_MAX_TX_ATTEMPTS_DIRECT;

    // Pass frame to the radio layer. Note, this fails if we
    // haven't enabled raw stream or are already transmitting.
    error = otLinkRawTransmit(mInstance, frame, &NcpBase::LinkRawTransmitDone);

exit:

    if (error == OT_ERROR_NONE)
    {
        // Don't do anything here yet. We will complete the transaction when we get a transmit done callback
    }
    else
    {
        error = SendLastStatus(aHeader, ThreadErrorToSpinelStatus(error));
    }

    OT_UNUSED_VARIABLE(aKey);

    return error;
}

#endif // OPENTHREAD_ENABLE_RAW_LINK_API

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

#if OPENTHREAD_FTD
otError NcpBase::SetPropertyHandler_THREAD_LOCAL_LEADER_WEIGHT(uint8_t aHeader, spinel_prop_key_t aKey,
                                                               const uint8_t *aValuePtr, uint16_t aValueLen)
{
    uint8_t weight;
    spinel_ssize_t parsedLength;
    otError error = OT_ERROR_NONE;

    parsedLength = spinel_datatype_unpack(
                       aValuePtr,
                       aValueLen,
                       SPINEL_DATATYPE_UINT8_S,
                       &weight
                   );

    VerifyOrExit(parsedLength > 0, error = OT_ERROR_PARSE);

    otThreadSetLocalLeaderWeight(mInstance, weight);

exit:
    return SendSetPropertyResponse(aHeader, aKey, error);
}
#endif  // OPENTHREAD_FTD

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

#if OPENTHREAD_ENABLE_TMF_PROXY && OPENTHREAD_FTD
otError NcpBase::SetPropertyHandler_THREAD_TMF_PROXY_STREAM(uint8_t aHeader, spinel_prop_key_t aKey,
                                                           const uint8_t *aValuePtr, uint16_t aValueLen)
{
    const uint8_t *framePtr = NULL;
    unsigned int frameLen = 0;
    uint16_t locator;
    uint16_t port;
    otMessage *message;
    spinel_ssize_t parsedLength;
    otError error = OT_ERROR_NONE;

    // THREAD_TMF_PROXY_STREAM requires layer 2 security.
    message = otIp6NewMessage(mInstance, true);
    VerifyOrExit(message != NULL, error = OT_ERROR_NO_BUFS);

    parsedLength = spinel_datatype_unpack(
                       aValuePtr,
                       aValueLen,
                       (
                           SPINEL_DATATYPE_DATA_WLEN_S  // Frame data
                           SPINEL_DATATYPE_UINT16_S     // Locator
                           SPINEL_DATATYPE_UINT16_S     // Port
                       ),
                       &framePtr,
                       &frameLen,
                       &locator,
                       &port
                   );


    VerifyOrExit(parsedLength > 0, error = OT_ERROR_PARSE);

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

    if (error == OT_ERROR_NONE)
    {
        if (SPINEL_HEADER_GET_TID(aHeader) != 0)
        {
            // Only send a successful status update if
            // there was a transaction id in the aHeader.
            error = SendLastStatus(aHeader, SPINEL_STATUS_OK);
        }
    }
    else
    {
        error = SendLastStatus(aHeader, ThreadErrorToSpinelStatus(error));
    }

    OT_UNUSED_VARIABLE(aKey);

    return error;
}
#endif // OPENTHREAD_ENABLE_TMF_PROXY && OPENTHREAD_FTD

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

otError NcpBase::SetPropertyHandler_IPV6_ML_PREFIX(uint8_t aHeader, spinel_prop_key_t aKey, const uint8_t *aValuePtr,
                                                   uint16_t aValueLen)
{
    otError error = OT_ERROR_NONE;

    VerifyOrExit(aValueLen >= 8, error = OT_ERROR_PARSE);

    error = otThreadSetMeshLocalPrefix(mInstance, aValuePtr);

exit:
    return SendSetPropertyResponse(aHeader, aKey, error);
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

#if OPENTHREAD_FTD
otError NcpBase::SetPropertyHandler_THREAD_ROUTER_ROLE_ENABLED(uint8_t aHeader, spinel_prop_key_t aKey,
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

    otThreadSetRouterRoleEnabled(mInstance, enabled);

exit:
    return SendSetPropertyResponse(aHeader, aKey, error);
}

#if OPENTHREAD_CONFIG_ENABLE_STEERING_DATA_SET_OOB
otError NcpBase::SetPropertyHandler_THREAD_STEERING_DATA(uint8_t aHeader, spinel_prop_key_t aKey,
                                                                const uint8_t *aValuePtr, uint16_t aValueLen)
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

    error = otThreadSetSteeringData(mInstance, extAddress);

exit:

    if (error == OT_ERROR_NONE)
    {
        // Note that there is no get handler for this property.
        error = SendPropertyUpdate(
                        aHeader,
                        SPINEL_CMD_PROP_VALUE_IS,
                        aKey,
                        SPINEL_DATATYPE_EUI64_S,
                        extAddress->m8
                    );
    }
    else
    {
        error = SendLastStatus(aHeader, ThreadErrorToSpinelStatus(error));
    }

    return error;
}
#endif // #if OPENTHREAD_CONFIG_ENABLE_STEERING_DATA_SET_OOB
#endif // #if OPENTHREAD_FTD

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

#if OPENTHREAD_ENABLE_COMMISSIONER && OPENTHREAD_FTD

otError NcpBase::SetPropertyHandler_THREAD_COMMISSIONER_ENABLED(uint8_t aHeader, spinel_prop_key_t aKey,
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

    if (enabled == false)
    {
        error = otCommissionerStop(mInstance);
    }
    else
    {
        error = otCommissionerStart(mInstance);
    }

exit:
    OT_UNUSED_VARIABLE(aKey);

    return SendLastStatus(aHeader, ThreadErrorToSpinelStatus(error));
}

#endif // OPENTHREAD_ENABLE_COMMISSIONER && OPENTHREAD_FTD

#if OPENTHREAD_ENABLE_MAC_WHITELIST

otError NcpBase::SetPropertyHandler_MAC_WHITELIST(uint8_t aHeader, spinel_prop_key_t aKey, const uint8_t *aValuePtr,
                                                  uint16_t aValueLen)
{
    bool reportAsync;
    spinel_ssize_t parsedLength;
    otError error = OT_ERROR_NONE;

    // First, clear the whitelist.
    otLinkClearWhitelist(mInstance);

    while (aValueLen > 0)
    {
        otExtAddress *extAddress = NULL;
        int8_t rssi = RSSI_OVERRIDE_DISABLED;

        parsedLength = spinel_datatype_unpack(
                           aValuePtr,
                           aValueLen,
                           SPINEL_DATATYPE_STRUCT_S(
                               SPINEL_DATATYPE_EUI64_S
                               SPINEL_DATATYPE_INT8_S
                           ),
                           &extAddress,
                           &rssi
                       );

        if (parsedLength <= 0)
        {
            rssi = RSSI_OVERRIDE_DISABLED;
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

        if (rssi == RSSI_OVERRIDE_DISABLED)
        {
            SuccessOrExit(error = otLinkAddWhitelist(mInstance, extAddress->m8));
        }
        else
        {
            SuccessOrExit(error = otLinkAddWhitelistRssi(mInstance, extAddress->m8, rssi));
        }

        aValuePtr += parsedLength;
        aValueLen -= parsedLength;
    }

exit:
    // If we had an error, we may have actually changed
    // the state of the whitelist---so we need to report
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

otError NcpBase::SetPropertyHandler_MAC_WHITELIST_ENABLED(uint8_t aHeader, spinel_prop_key_t aKey,
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

    otLinkSetWhitelistEnabled(mInstance, enabled);

exit:
    return SendSetPropertyResponse(aHeader, aKey, error);
}

otError NcpBase::SetPropertyHandler_MAC_BLACKLIST(uint8_t aHeader, spinel_prop_key_t aKey, const uint8_t *aValuePtr,
                                                  uint16_t aValueLen)
{
    bool reportAsync;
    spinel_ssize_t parsedLength;
    otError error = OT_ERROR_NONE;

    // First, clear the blacklist.
    otLinkClearBlacklist(mInstance);

    while (aValueLen > 0)
    {
        otExtAddress *ext_addr = NULL;

        parsedLength = spinel_datatype_unpack(
                           aValuePtr,
                           aValueLen,
                           SPINEL_DATATYPE_STRUCT_S(
                               SPINEL_DATATYPE_EUI64_S
                           ),
                           &ext_addr
                       );

        VerifyOrExit(parsedLength > 0, error = OT_ERROR_PARSE);

        SuccessOrExit(error = otLinkAddBlacklist(mInstance, ext_addr->m8));

        aValuePtr += parsedLength;
        aValueLen -= parsedLength;
    }

exit:
    // If we had an error, we may have actually changed
    // the state of the blacklist---so we need to report
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

otError NcpBase::SetPropertyHandler_MAC_BLACKLIST_ENABLED(uint8_t aHeader, spinel_prop_key_t aKey,
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

    otLinkSetBlacklistEnabled(mInstance, enabled);

exit:
    return SendSetPropertyResponse(aHeader, aKey, error);
}

#endif // OPENTHREAD_ENABLE_MAC_WHITELIST

#if OPENTHREAD_ENABLE_RAW_LINK_API

otError NcpBase::SetPropertyHandler_MAC_SRC_MATCH_ENABLED(uint8_t aHeader, spinel_prop_key_t aKey,
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

    error = otLinkRawSrcMatchEnable(mInstance, enabled);

exit:
    return SendSetPropertyResponse(aHeader, aKey, error);
}

otError NcpBase::SetPropertyHandler_MAC_SRC_MATCH_SHORT_ADDRESSES(uint8_t aHeader, spinel_prop_key_t aKey,
                                                                  const uint8_t *aValuePtr, uint16_t aValueLen)
{
    otError error = OT_ERROR_NONE;
    spinel_status_t spinelError = SPINEL_STATUS_OK;
    const uint8_t *data = aValuePtr;
    uint16_t dataLen = aValueLen;

    // Clear the list first
    error = otLinkRawSrcMatchClearShortEntries(mInstance);

    VerifyOrExit(error == OT_ERROR_NONE, spinelError = ThreadErrorToSpinelStatus(error));

    // Loop through the addresses and add them
    while (dataLen >= sizeof(uint16_t))
    {
        spinel_ssize_t parsedLength;
        uint16_t short_address;

        parsedLength = spinel_datatype_unpack(
                           data,
                           dataLen,
                           SPINEL_DATATYPE_UINT16_S,
                           &short_address
                       );

        VerifyOrExit(parsedLength > 0, spinelError = SPINEL_STATUS_PARSE_ERROR);

        data += parsedLength;
        dataLen -= (uint16_t)parsedLength;

        error = otLinkRawSrcMatchAddShortEntry(mInstance, short_address);

        VerifyOrExit(error == OT_ERROR_NONE, spinelError = ThreadErrorToSpinelStatus(error));
    }

    error =
        SendPropertyUpdate(
            aHeader,
            SPINEL_CMD_PROP_VALUE_IS,
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

otError NcpBase::SetPropertyHandler_MAC_SRC_MATCH_EXTENDED_ADDRESSES(uint8_t aHeader, spinel_prop_key_t aKey,
                                                                     const uint8_t *aValuePtr, uint16_t aValueLen)
{
    otError error = OT_ERROR_NONE;
    spinel_status_t spinelError = SPINEL_STATUS_OK;
    const uint8_t *data = aValuePtr;
    uint16_t dataLen = aValueLen;

    // Clear the list first
    error = otLinkRawSrcMatchClearExtEntries(mInstance);

    VerifyOrExit(error == OT_ERROR_NONE, spinelError = ThreadErrorToSpinelStatus(error));

    // Loop through the addresses and add them
    while (dataLen >= sizeof(otExtAddress))
    {
        spinel_ssize_t parsedLength;
        uint8_t *extAddress;

        parsedLength = spinel_datatype_unpack(
                           data,
                           dataLen,
                           SPINEL_DATATYPE_EUI64_S,
                           &extAddress
                       );

        VerifyOrExit(parsedLength > 0, spinelError = SPINEL_STATUS_PARSE_ERROR);

        data += parsedLength;
        dataLen -= (uint16_t)parsedLength;

        error = otLinkRawSrcMatchAddExtEntry(mInstance, extAddress);

        VerifyOrExit(error == OT_ERROR_NONE, spinelError = ThreadErrorToSpinelStatus(error));
    }

    error =
        SendPropertyUpdate(
            aHeader,
            SPINEL_CMD_PROP_VALUE_IS,
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

#endif

#if OPENTHREAD_FTD

otError NcpBase::SetPropertyHandler_NET_PSKC(uint8_t aHeader, spinel_prop_key_t aKey, const uint8_t *aValuePtr,
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

    VerifyOrExit((parsedLength > 0) && (len == sizeof(spinel_net_pskc_t)), error = OT_ERROR_PARSE);

    error = otThreadSetPSKc(mInstance, ptr);

exit:
    return SendSetPropertyResponse(aHeader, aKey, error);
}

#endif // OPENTHREAD_FTD

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

#if OPENTHREAD_FTD

otError NcpBase::SetPropertyHandler_THREAD_CHILD_COUNT_MAX(uint8_t aHeader, spinel_prop_key_t aKey,
                                                           const uint8_t *aValuePtr, uint16_t aValueLen)
{
    uint8_t maxChildren = 0;
    spinel_ssize_t parsedLength;
    otError error = OT_ERROR_NONE;

    parsedLength = spinel_datatype_unpack(
                       aValuePtr,
                       aValueLen,
                       SPINEL_DATATYPE_UINT8_S,
                       &maxChildren
                   );

    VerifyOrExit(parsedLength > 0, error = OT_ERROR_PARSE);

    error = otThreadSetMaxAllowedChildren(mInstance, maxChildren);

exit:
    return SendSetPropertyResponse(aHeader, aKey, error);
}

otError NcpBase::SetPropertyHandler_THREAD_CHILD_TIMEOUT(uint8_t aHeader, spinel_prop_key_t aKey,
                                                         const uint8_t *aValuePtr, uint16_t aValueLen)
{
    uint32_t timeout = 0;
    spinel_ssize_t parsedLength;
    otError error = OT_ERROR_NONE;

    parsedLength = spinel_datatype_unpack(
                       aValuePtr,
                       aValueLen,
                       SPINEL_DATATYPE_UINT32_S,
                       &timeout
                   );

    VerifyOrExit(parsedLength > 0, error = OT_ERROR_PARSE);

    otThreadSetChildTimeout(mInstance, timeout);

exit:
    return SendSetPropertyResponse(aHeader, aKey, error);
}

otError NcpBase::SetPropertyHandler_THREAD_ROUTER_UPGRADE_THRESHOLD(uint8_t aHeader, spinel_prop_key_t aKey,
                                                                    const uint8_t *aValuePtr, uint16_t aValueLen)
{
    uint8_t threshold = 0;
    spinel_ssize_t parsedLength;
    otError error = OT_ERROR_NONE;

    parsedLength = spinel_datatype_unpack(
                       aValuePtr,
                       aValueLen,
                       SPINEL_DATATYPE_UINT8_S,
                       &threshold
                   );

    VerifyOrExit(parsedLength > 0, error = OT_ERROR_PARSE);

    otThreadSetRouterUpgradeThreshold(mInstance, threshold);

exit:
    return SendSetPropertyResponse(aHeader, aKey, error);
}

otError NcpBase::SetPropertyHandler_THREAD_ROUTER_DOWNGRADE_THRESHOLD(uint8_t aHeader, spinel_prop_key_t aKey,
                                                                      const uint8_t *aValuePtr, uint16_t aValueLen)
{
    uint8_t threshold = 0;
    spinel_ssize_t parsedLength;
    otError error = OT_ERROR_NONE;

    parsedLength = spinel_datatype_unpack(
                       aValuePtr,
                       aValueLen,
                       SPINEL_DATATYPE_UINT8_S,
                       &threshold
                   );

    VerifyOrExit(parsedLength > 0, error = OT_ERROR_PARSE);

    otThreadSetRouterDowngradeThreshold(mInstance, threshold);

exit:
    return SendSetPropertyResponse(aHeader, aKey, error);
}

otError NcpBase::SetPropertyHandler_THREAD_ROUTER_SELECTION_JITTER(uint8_t aHeader, spinel_prop_key_t aKey,
                                                                   const uint8_t *aValuePtr, uint16_t aValueLen)
{
    uint8_t jitter = 0;
    spinel_ssize_t parsedLength;
    otError error = OT_ERROR_NONE;

    parsedLength = spinel_datatype_unpack(
                       aValuePtr,
                       aValueLen,
                       SPINEL_DATATYPE_UINT8_S,
                       &jitter
                   );

    VerifyOrExit(parsedLength > 0, error = OT_ERROR_PARSE);

    otThreadSetRouterSelectionJitter(mInstance, jitter);

exit:
    return SendSetPropertyResponse(aHeader, aKey, error);
}

otError NcpBase::SetPropertyHandler_THREAD_PREFERRED_ROUTER_ID(uint8_t aHeader, spinel_prop_key_t aKey,
                                                               const uint8_t *aValuePtr, uint16_t aValueLen)
{
    uint8_t routerId = 0;
    spinel_ssize_t parsedLength;
    otError error = OT_ERROR_NONE;

    parsedLength = spinel_datatype_unpack(
                       aValuePtr,
                       aValueLen,
                       SPINEL_DATATYPE_UINT8_S,
                       &routerId
                   );

    VerifyOrExit(parsedLength > 0, error = OT_ERROR_PARSE);

    error = otThreadSetPreferredRouterId(mInstance, routerId);

exit:
    if (error == OT_ERROR_NONE)
    {
        error = SendPropertyUpdate(
                        aHeader,
                        SPINEL_CMD_PROP_VALUE_IS,
                        aKey,
                        SPINEL_DATATYPE_UINT8_S,
                        routerId
                    );
    }
    else
    {
        error = SendLastStatus(aHeader, ThreadErrorToSpinelStatus(error));
    }

    return error;
}

#endif  // OPENTHREAD_FTD

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

#if OPENTHREAD_FTD

otError NcpBase::SetPropertyHandler_THREAD_CONTEXT_REUSE_DELAY(uint8_t aHeader, spinel_prop_key_t aKey,
                                                               const uint8_t *aValuePtr, uint16_t aValueLen)
{
    uint32_t delay = 0;
    spinel_ssize_t parsedLength;
    otError error = OT_ERROR_NONE;

    parsedLength = spinel_datatype_unpack(
                       aValuePtr,
                       aValueLen,
                       SPINEL_DATATYPE_UINT32_S,
                       &delay
                   );

    VerifyOrExit(parsedLength > 0, error = OT_ERROR_PARSE);

    otThreadSetContextIdReuseDelay(mInstance, delay);

 exit:
    return SendSetPropertyResponse(aHeader, aKey, error);
}

otError NcpBase::SetPropertyHandler_THREAD_NETWORK_ID_TIMEOUT(uint8_t aHeader, spinel_prop_key_t aKey,
                                                              const uint8_t *aValuePtr, uint16_t aValueLen)
{
    uint8_t timeout = 0;
    spinel_ssize_t parsedLength;
    otError error = OT_ERROR_NONE;

    parsedLength = spinel_datatype_unpack(
                       aValuePtr,
                       aValueLen,
                       SPINEL_DATATYPE_UINT8_S,
                       &timeout
                   );

    VerifyOrExit(parsedLength > 0, error = OT_ERROR_PARSE);

    otThreadSetNetworkIdTimeout(mInstance, timeout);

 exit:
    return SendSetPropertyResponse(aHeader, aKey, error);
}

#endif  // OPENTHREAD_FTD

#if OPENTHREAD_ENABLE_TMF_PROXY && OPENTHREAD_FTD

otError NcpBase::SetPropertyHandler_THREAD_TMF_PROXY_ENABLED(uint8_t aHeader, spinel_prop_key_t aKey,
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
        error = otTmfProxyStart(mInstance, &NcpBase::HandleTmfProxyStream, this);
    }
    else
    {
        error = otTmfProxyStop(mInstance);
    }

exit:
    return SendSetPropertyResponse(aHeader, aKey, error);
}

#endif // OPENTHREAD_ENABLE_TMF_PROXY && OPENTHREAD_FTD

#if OPENTHREAD_ENABLE_JAM_DETECTION

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
    otError error;

    error = SendPropertyUpdate(
                    SPINEL_HEADER_FLAG | SPINEL_HEADER_IID_0,
                    SPINEL_CMD_PROP_VALUE_IS,
                    SPINEL_PROP_JAM_DETECTED,
                    SPINEL_DATATYPE_BOOL_S,
                    aJamState
                );

    // If we could not send the jam state change indicator (no
    // buffer space), we set `mShouldSignalJamStateChange` to true to send
    // it out when buffer space becomes available.
    if (error != OT_ERROR_NONE)
    {
        mShouldSignalJamStateChange = true;
    }
}

#endif // OPENTHREAD_ENABLE_JAM_DETECTION

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

#if OPENTHREAD_ENABLE_LEGACY

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

#endif  // OPENTHREAD_ENABLE_LEGACY

// ----------------------------------------------------------------------------
// MARK: Individual Property Inserters
// ----------------------------------------------------------------------------

#if OPENTHREAD_ENABLE_RAW_LINK_API

otError NcpBase::InsertPropertyHandler_MAC_SRC_MATCH_SHORT_ADDRESSES(uint8_t aHeader, spinel_prop_key_t aKey,
                                                                     const uint8_t *aValuePtr, uint16_t aValueLen)
{
    spinel_ssize_t parsedLength;
    otError error = OT_ERROR_NONE;
    spinel_status_t spinelError = SPINEL_STATUS_OK;
    uint16_t short_address;

    parsedLength = spinel_datatype_unpack(
                       aValuePtr,
                       aValueLen,
                       SPINEL_DATATYPE_UINT16_S,
                       &short_address
                   );

    VerifyOrExit(parsedLength > 0, spinelError = SPINEL_STATUS_PARSE_ERROR);

    error = otLinkRawSrcMatchAddShortEntry(mInstance, short_address);

    VerifyOrExit(error == OT_ERROR_NONE,
                 spinelError = ThreadErrorToSpinelStatus(error));

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

otError NcpBase::InsertPropertyHandler_MAC_SRC_MATCH_EXTENDED_ADDRESSES(uint8_t aHeader, spinel_prop_key_t aKey,
                                                                        const uint8_t *aValuePtr, uint16_t aValueLen)
{
    spinel_ssize_t parsedLength;
    otError error = OT_ERROR_NONE;
    spinel_status_t spinelError = SPINEL_STATUS_OK;
    uint8_t *extAddress = NULL;

    parsedLength = spinel_datatype_unpack(
                       aValuePtr,
                       aValueLen,
                       SPINEL_DATATYPE_EUI64_S,
                       &extAddress
                   );

    VerifyOrExit(parsedLength > 0, spinelError = SPINEL_STATUS_PARSE_ERROR);

    error = otLinkRawSrcMatchAddExtEntry(mInstance, extAddress);

    VerifyOrExit(error == OT_ERROR_NONE,
                 spinelError = ThreadErrorToSpinelStatus(error));

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

#endif

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

    VerifyOrExit(error == OT_ERROR_NONE,
                 spinelError = ThreadErrorToSpinelStatus(error));

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

#if OPENTHREAD_ENABLE_BORDER_ROUTER
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
#endif // OPENTHREAD_ENABLE_BORDER_ROUTER

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

#if OPENTHREAD_ENABLE_MAC_WHITELIST

otError NcpBase::InsertPropertyHandler_MAC_WHITELIST(uint8_t aHeader, spinel_prop_key_t aKey,
                                                     const uint8_t *aValuePtr, uint16_t aValueLen)
{
    spinel_ssize_t parsedLength;
    otError error = OT_ERROR_NONE;
    spinel_status_t spinelError = SPINEL_STATUS_OK;
    otExtAddress *extAddress = NULL;
    int8_t rssi = RSSI_OVERRIDE_DISABLED;

    if (aValueLen > static_cast<spinel_ssize_t>(sizeof(otExtAddress)))
    {
        parsedLength = spinel_datatype_unpack(
                           aValuePtr,
                           aValueLen,
                           SPINEL_DATATYPE_EUI64_S SPINEL_DATATYPE_INT8_S,
                           &extAddress,
                           &rssi
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

    if (rssi == RSSI_OVERRIDE_DISABLED)
    {
        error = otLinkAddWhitelist(mInstance, extAddress->m8);
    }
    else
    {
        error = otLinkAddWhitelistRssi(mInstance, extAddress->m8, rssi);
    }

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

    error = otLinkAddBlacklist(mInstance, extAddress->m8);
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

#endif // OPENTHREAD_ENABLE_MAC_WHITELIST

#if OPENTHREAD_ENABLE_COMMISSIONER && OPENTHREAD_FTD
otError NcpBase::InsertPropertyHandler_THREAD_JOINERS(uint8_t aHeader, spinel_prop_key_t aKey,
                                                      const uint8_t *aValuePtr, uint16_t aValueLen)
{
    spinel_ssize_t parsedLength;
    otError error = OT_ERROR_NONE;
    spinel_status_t spinelError = SPINEL_STATUS_OK;
    otExtAddress *extAddress = NULL;
    const char *aPSKd = NULL;
    uint32_t joinerTimeout = 0;

    VerifyOrExit(mAllowLocalNetworkDataChange == true, spinelError =  SPINEL_STATUS_INVALID_STATE);

    parsedLength = spinel_datatype_unpack(
                       aValuePtr,
                       aValueLen,
                       (
                           SPINEL_DATATYPE_UTF8_S     // PSK
                           SPINEL_DATATYPE_UINT32_S   // Timeout
                           SPINEL_DATATYPE_EUI64_S    // Extended address
                       ),
                       &aPSKd,
                       &joinerTimeout,
                       &extAddress
                   );

    if (parsedLength <= 0)
    {
        parsedLength = spinel_datatype_unpack(
                           aValuePtr,
                           aValueLen,
                           (
                              SPINEL_DATATYPE_UTF8_S     // PSK
                              SPINEL_DATATYPE_UINT32_S   // Timeout
                           ),
                           &aPSKd,
                           &joinerTimeout
                       );
        extAddress = NULL;
    }

    VerifyOrExit(parsedLength > 0, spinelError = SPINEL_STATUS_PARSE_ERROR);

    error = otCommissionerAddJoiner(mInstance, extAddress, aPSKd, joinerTimeout);
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
#endif // OPENTHREAD_ENABLE_COMMISSIONER && OPENTHREAD_FTD

// ----------------------------------------------------------------------------
// MARK: Individual Property Removers
// ----------------------------------------------------------------------------

#if OPENTHREAD_ENABLE_RAW_LINK_API
otError NcpBase::RemovePropertyHandler_MAC_SRC_MATCH_SHORT_ADDRESSES(uint8_t aHeader, spinel_prop_key_t aKey,
                                                                     const uint8_t *aValuePtr, uint16_t aValueLen)
{
    spinel_ssize_t parsedLength;
    otError error = OT_ERROR_NONE;
    spinel_status_t spinelError = SPINEL_STATUS_OK;
    uint16_t shortAddress;

    parsedLength = spinel_datatype_unpack(
                       aValuePtr,
                       aValueLen,
                       SPINEL_DATATYPE_UINT16_S,
                       &shortAddress
                   );

    VerifyOrExit(parsedLength > 0, spinelError = SPINEL_STATUS_PARSE_ERROR);

    error = otLinkRawSrcMatchClearShortEntry(mInstance, shortAddress);
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

otError NcpBase::RemovePropertyHandler_MAC_SRC_MATCH_EXTENDED_ADDRESSES(uint8_t aHeader, spinel_prop_key_t aKey,
                                                                        const uint8_t *aValuePtr, uint16_t aValueLen)
{
    spinel_ssize_t parsedLength;
    otError error = OT_ERROR_NONE;
    spinel_status_t spinelError = SPINEL_STATUS_OK;
    uint8_t *extAddress;

    parsedLength = spinel_datatype_unpack(
                       aValuePtr,
                       aValueLen,
                       SPINEL_DATATYPE_EUI64_S,
                       &extAddress
                   );

    VerifyOrExit(parsedLength > 0, spinelError = SPINEL_STATUS_PARSE_ERROR);

    error = otLinkRawSrcMatchClearExtEntry(mInstance, extAddress);
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
#endif // #if OPENTHREAD_ENABLE_RAW_LINK_API

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

#if OPENTHREAD_ENABLE_BORDER_ROUTER
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
#endif

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

#if OPENTHREAD_FTD
otError NcpBase::RemovePropertyHandler_THREAD_ACTIVE_ROUTER_IDS(uint8_t aHeader, spinel_prop_key_t aKey,
                                                                const uint8_t *aValuePtr, uint16_t aValueLen)
{
    spinel_ssize_t parsedLength;
    otError error = OT_ERROR_NONE;
    spinel_status_t spinelError = SPINEL_STATUS_OK;
    uint8_t routerId;

    parsedLength = spinel_datatype_unpack(
                       aValuePtr,
                       aValueLen,
                       SPINEL_DATATYPE_UINT8_S,
                       &routerId
                   );

    VerifyOrExit(parsedLength > 0, spinelError = SPINEL_STATUS_PARSE_ERROR);

    error = otThreadReleaseRouterId(mInstance, routerId);
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
#endif  // OPENTHREAD_FTD

#if OPENTHREAD_ENABLE_MAC_WHITELIST

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

    otLinkRemoveWhitelist(mInstance, extAddress->m8);

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

        otLinkRemoveBlacklist(mInstance, extAddress->m8);

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

#endif // OPENTHREAD_ENABLE_MAC_WHITELIST

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

    SuccessOrExit(OutboundFrameBegin());

    SuccessOrExit(
        OutboundFrameFeedPacked(
            SPINEL_DATATYPE_COMMAND_PROP_S SPINEL_DATATYPE_DATA_S,
            SPINEL_HEADER_FLAG | SPINEL_HEADER_IID_0,
            SPINEL_CMD_PROP_VALUE_IS,
            SPINEL_PROP_NEST_LEGACY_ULA_PREFIX,
            aUlaPrefix, OT_NCP_LEGACY_ULA_PREFIX_LENGTH
        ));

    SuccessOrExit(OutboundFrameSend());

exit:
    return;
}

void NcpBase::HandleLegacyNodeDidJoin(const otExtAddress *aExtAddr)
{
    mLegacyNodeDidJoin = true;

    SuccessOrExit(OutboundFrameBegin());

    SuccessOrExit(
        OutboundFrameFeedPacked(
            SPINEL_DATATYPE_COMMAND_PROP_S SPINEL_DATATYPE_EUI64_S,
            SPINEL_HEADER_FLAG | SPINEL_HEADER_IID_0,
            SPINEL_CMD_PROP_VALUE_IS,
            SPINEL_PROP_NEST_LEGACY_JOINED_NODE,
            aExtAddr->m8
        ));

    SuccessOrExit(OutboundFrameSend());

exit:
    return;
}

#endif // OPENTHREAD_ENABLE_LEGACY

otError NcpBase::StreamWrite(int aStreamId, const uint8_t *aDataPtr, int aDataLen)
{
    otError error = OT_ERROR_NONE;

    if (aStreamId == 0)
    {
        aStreamId = SPINEL_PROP_STREAM_DEBUG;
    }

    VerifyOrExit(!mDisableStreamWrite, error = OT_ERROR_INVALID_STATE);

    error = SendPropertyUpdate(
                SPINEL_HEADER_FLAG | SPINEL_HEADER_IID_0,
                SPINEL_CMD_PROP_VALUE_IS,
                static_cast<spinel_prop_key_t>(aStreamId),
                aDataPtr,
                static_cast<uint16_t>(aDataLen)
            );

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

}  // namespace ot

// ----------------------------------------------------------------------------
// MARK: Virtual Datastream I/O (Public API)
// ----------------------------------------------------------------------------

otError otNcpStreamWrite(int aStreamId, const uint8_t *aDataPtr, int aDataLen)
{
    otError error  = OT_ERROR_INVALID_STATE;
    ot::NcpBase *ncp = ot::NcpBase::GetNcpInstance();

    if (ncp != NULL)
    {
        error = ncp->StreamWrite(aStreamId, aDataPtr, aDataLen);
    }

    return error;
}

// ----------------------------------------------------------------------------
// MARK: Peek/Poke delegate API
// ----------------------------------------------------------------------------

otError otNcpRegisterPeekPokeDelagates(otNcpDelegateAllowPeekPoke aAllowPeekDelegate,
                                       otNcpDelegateAllowPeekPoke aAllowPokeDelegate)
{
    otError error = OT_ERROR_NONE;

#if OPENTHREAD_CONFIG_NCP_ENABLE_PEEK_POKE
    ot::NcpBase *ncp = ot::NcpBase::GetNcpInstance();

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
// MARK: Legacy network APIs
// ----------------------------------------------------------------------------

void otNcpRegisterLegacyHandlers(const otNcpLegacyHandlers *aHandlers)
{
#if OPENTHREAD_ENABLE_LEGACY
    ot::NcpBase *ncp = ot::NcpBase::GetNcpInstance();

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
    ot::NcpBase *ncp = ot::NcpBase::GetNcpInstance();

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
    ot::NcpBase *ncp = ot::NcpBase::GetNcpInstance();

    if (ncp != NULL)
    {
        ncp->HandleLegacyNodeDidJoin(aExtAddr);
    }

#else
    OT_UNUSED_VARIABLE(aExtAddr);
#endif
}
