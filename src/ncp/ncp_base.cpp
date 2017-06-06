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

#ifdef OPENTHREAD_CONFIG_FILE
#include OPENTHREAD_CONFIG_FILE
#else
#include <openthread-config.h>
#endif

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
#define NCP_PLAT_RESET_REASON                 (1U<<31)
#define NCP_ON_MESH_NETS_CHANGED_BIT_FLAG     (1U<<30)

enum
{
    kThreadMode_RxOnWhenIdle        = (1 << 3),
    kThreadMode_SecureDataRequest   = (1 << 2),
    kThreadMode_FullFunctionDevice  = (1 << 1),
    kThreadMode_FullNetworkData     = (1 << 0),
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
    { SPINEL_CMD_NET_SAVE, &NcpBase::CommandHandler_NET_SAVE },
    { SPINEL_CMD_NET_CLEAR, &NcpBase::CommandHandler_NET_CLEAR },
    { SPINEL_CMD_NET_RECALL, &NcpBase::CommandHandler_NET_RECALL },
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
    { SPINEL_PROP_HOST_POWER_STATE, &NcpBase::GetPropertyHandler_HOST_POWER_STATE },

    { SPINEL_PROP_PHY_ENABLED, &NcpBase::GetPropertyHandler_PHY_ENABLED },
    { SPINEL_PROP_PHY_FREQ, &NcpBase::GetPropertyHandler_PHY_FREQ },
    { SPINEL_PROP_PHY_CHAN_SUPPORTED, &NcpBase::GetPropertyHandler_PHY_CHAN_SUPPORTED },
    { SPINEL_PROP_PHY_CHAN, &NcpBase::GetPropertyHandler_PHY_CHAN },
    { SPINEL_PROP_PHY_RSSI, &NcpBase::GetPropertyHandler_PHY_RSSI },
    { SPINEL_PROP_PHY_TX_POWER, &NcpBase::GetPropertyHandler_PHY_TX_POWER },
    { SPINEL_PROP_PHY_RX_SENSITIVITY, &NcpBase::GetPropertyHandler_PHY_RX_SENSITIVITY },

    { SPINEL_PROP_MAC_SCAN_STATE, &NcpBase::GetPropertyHandler_MAC_SCAN_STATE },
    { SPINEL_PROP_MAC_SCAN_MASK, &NcpBase::GetPropertyHandler_MAC_SCAN_MASK },
    { SPINEL_PROP_MAC_SCAN_PERIOD, &NcpBase::GetPropertyHandler_MAC_SCAN_PERIOD },
    { SPINEL_PROP_MAC_15_4_PANID, &NcpBase::GetPropertyHandler_MAC_15_4_PANID },
    { SPINEL_PROP_MAC_15_4_LADDR, &NcpBase::GetPropertyHandler_MAC_15_4_LADDR },
    { SPINEL_PROP_MAC_15_4_SADDR, &NcpBase::GetPropertyHandler_MAC_15_4_SADDR },
    { SPINEL_PROP_MAC_RAW_STREAM_ENABLED, &NcpBase::GetPropertyHandler_MAC_RAW_STREAM_ENABLED },
    { SPINEL_PROP_MAC_PROMISCUOUS_MODE, &NcpBase::GetPropertyHandler_MAC_PROMISCUOUS_MODE },
    { SPINEL_PROP_MAC_EXTENDED_ADDR, &NcpBase::GetPropertyHandler_MAC_EXTENDED_ADDR },

    { SPINEL_PROP_NET_SAVED, &NcpBase::GetPropertyHandler_NET_SAVED },
    { SPINEL_PROP_NET_IF_UP, &NcpBase::GetPropertyHandler_NET_IF_UP },
    { SPINEL_PROP_NET_STACK_UP, &NcpBase::GetPropertyHandler_NET_STACK_UP },
    { SPINEL_PROP_NET_ROLE, &NcpBase::GetPropertyHandler_NET_ROLE },
    { SPINEL_PROP_NET_NETWORK_NAME, &NcpBase::GetPropertyHandler_NET_NETWORK_NAME },
    { SPINEL_PROP_NET_XPANID, &NcpBase::GetPropertyHandler_NET_XPANID },
    { SPINEL_PROP_NET_MASTER_KEY, &NcpBase::GetPropertyHandler_NET_MASTER_KEY },
    { SPINEL_PROP_NET_KEY_SEQUENCE_COUNTER, &NcpBase::GetPropertyHandler_NET_KEY_SEQUENCE_COUNTER },
    { SPINEL_PROP_NET_PARTITION_ID, &NcpBase::GetPropertyHandler_NET_PARTITION_ID },
    { SPINEL_PROP_NET_KEY_SWITCH_GUARDTIME, &NcpBase::GetPropertyHandler_NET_KEY_SWITCH_GUARDTIME},

    { SPINEL_PROP_THREAD_LEADER_ADDR, &NcpBase::GetPropertyHandler_THREAD_LEADER_ADDR },
    { SPINEL_PROP_THREAD_PARENT, &NcpBase::GetPropertyHandler_THREAD_PARENT },
    { SPINEL_PROP_THREAD_NEIGHBOR_TABLE, &NcpBase::GetPropertyHandler_THREAD_NEIGHBOR_TABLE },
    { SPINEL_PROP_THREAD_LEADER_RID, &NcpBase::GetPropertyHandler_THREAD_LEADER_RID },
    { SPINEL_PROP_THREAD_LEADER_WEIGHT, &NcpBase::GetPropertyHandler_THREAD_LEADER_WEIGHT },
#if OPENTHREAD_ENABLE_BORDER_ROUTER
    { SPINEL_PROP_THREAD_NETWORK_DATA, &NcpBase::GetPropertyHandler_THREAD_NETWORK_DATA },
    { SPINEL_PROP_THREAD_STABLE_NETWORK_DATA, &NcpBase::GetPropertyHandler_THREAD_STABLE_NETWORK_DATA },
#endif
    { SPINEL_PROP_THREAD_NETWORK_DATA_VERSION, &NcpBase::GetPropertyHandler_THREAD_NETWORK_DATA_VERSION },
    { SPINEL_PROP_THREAD_STABLE_NETWORK_DATA_VERSION, &NcpBase::GetPropertyHandler_THREAD_STABLE_NETWORK_DATA_VERSION },
    { SPINEL_PROP_THREAD_LEADER_NETWORK_DATA, &NcpBase::GetPropertyHandler_THREAD_LEADER_NETWORK_DATA },
    { SPINEL_PROP_THREAD_STABLE_LEADER_NETWORK_DATA, &NcpBase::GetPropertyHandler_THREAD_STABLE_LEADER_NETWORK_DATA },
    { SPINEL_PROP_THREAD_OFF_MESH_ROUTES, &NcpBase::GetPropertyHandler_THREAD_OFF_MESH_ROUTES },
    { SPINEL_PROP_THREAD_ASSISTING_PORTS, &NcpBase::GetPropertyHandler_THREAD_ASSISTING_PORTS },
    { SPINEL_PROP_THREAD_ALLOW_LOCAL_NET_DATA_CHANGE, &NcpBase::GetPropertyHandler_THREAD_ALLOW_LOCAL_NET_DATA_CHANGE },

#if OPENTHREAD_ENABLE_COMMISSIONER && OPENTHREAD_FTD
    { SPINEL_PROP_THREAD_COMMISSIONER_ENABLED, &NcpBase::GetPropertyHandler_THREAD_COMMISSIONER_ENABLED },
#endif

#if OPENTHREAD_ENABLE_MAC_WHITELIST
    { SPINEL_PROP_MAC_WHITELIST, &NcpBase::GetPropertyHandler_MAC_WHITELIST },
    { SPINEL_PROP_MAC_WHITELIST_ENABLED, &NcpBase::GetPropertyHandler_MAC_WHITELIST_ENABLED },
#endif
    { SPINEL_PROP_THREAD_MODE, &NcpBase::GetPropertyHandler_THREAD_MODE },
    { SPINEL_PROP_THREAD_CHILD_TIMEOUT, &NcpBase::GetPropertyHandler_THREAD_CHILD_TIMEOUT },
    { SPINEL_PROP_THREAD_RLOC16, &NcpBase::GetPropertyHandler_THREAD_RLOC16 },
    { SPINEL_PROP_THREAD_ON_MESH_NETS, &NcpBase::GetPropertyHandler_THREAD_ON_MESH_NETS },
    { SPINEL_PROP_NET_REQUIRE_JOIN_EXISTING, &NcpBase::GetPropertyHandler_NET_REQUIRE_JOIN_EXISTING },

    { SPINEL_PROP_IPV6_ML_PREFIX, &NcpBase::GetPropertyHandler_IPV6_ML_PREFIX },
    { SPINEL_PROP_IPV6_ML_ADDR, &NcpBase::GetPropertyHandler_IPV6_ML_ADDR },
    { SPINEL_PROP_IPV6_LL_ADDR, &NcpBase::GetPropertyHandler_IPV6_LL_ADDR },
    { SPINEL_PROP_IPV6_ADDRESS_TABLE, &NcpBase::GetPropertyHandler_IPV6_ADDRESS_TABLE },
    { SPINEL_PROP_IPV6_ROUTE_TABLE, &NcpBase::GetPropertyHandler_IPV6_ROUTE_TABLE },
    { SPINEL_PROP_IPV6_ICMP_PING_OFFLOAD, &NcpBase::GetPropertyHandler_IPV6_ICMP_PING_OFFLOAD },
    { SPINEL_PROP_THREAD_RLOC16_DEBUG_PASSTHRU, &NcpBase::GetPropertyHandler_THREAD_RLOC16_DEBUG_PASSTHRU },
    { SPINEL_PROP_THREAD_DISCOVERY_SCAN_JOINER_FLAG, &NcpBase::GetPropertyHandler_THREAD_DISCOVERY_SCAN_JOINER_FLAG },
    { SPINEL_PROP_THREAD_DISCOVERY_SCAN_ENABLE_FILTERING,
        &NcpBase::GetPropertyHandler_THREAD_DISCOVERY_SCAN_ENABLE_FILTERING },
    { SPINEL_PROP_THREAD_DISCOVERY_SCAN_PANID, &NcpBase::GetPropertyHandler_THREAD_DISCOVERY_SCAN_PANID },

    { SPINEL_PROP_STREAM_NET, &NcpBase::GetPropertyHandler_STREAM_NET },

#if OPENTHREAD_FTD
    { SPINEL_PROP_THREAD_CHILD_TABLE, &NcpBase::GetPropertyHandler_THREAD_CHILD_TABLE },
    { SPINEL_PROP_THREAD_LOCAL_LEADER_WEIGHT, &NcpBase::GetPropertyHandler_THREAD_LOCAL_LEADER_WEIGHT },
    { SPINEL_PROP_THREAD_ROUTER_ROLE_ENABLED, &NcpBase::GetPropertyHandler_THREAD_ROUTER_ROLE_ENABLED },
    { SPINEL_PROP_NET_PSKC, &NcpBase::GetPropertyHandler_NET_PSKC },
    { SPINEL_PROP_THREAD_CHILD_COUNT_MAX, &NcpBase::GetPropertyHandler_THREAD_CHILD_COUNT_MAX },
    { SPINEL_PROP_THREAD_ROUTER_UPGRADE_THRESHOLD, &NcpBase::GetPropertyHandler_THREAD_ROUTER_UPGRADE_THRESHOLD },
    { SPINEL_PROP_THREAD_ROUTER_DOWNGRADE_THRESHOLD, &NcpBase::GetPropertyHandler_THREAD_ROUTER_DOWNGRADE_THRESHOLD },
    { SPINEL_PROP_THREAD_CONTEXT_REUSE_DELAY, &NcpBase::GetPropertyHandler_THREAD_CONTEXT_REUSE_DELAY },
    { SPINEL_PROP_THREAD_NETWORK_ID_TIMEOUT, &NcpBase::GetPropertyHandler_THREAD_NETWORK_ID_TIMEOUT },
    { SPINEL_PROP_THREAD_ROUTER_SELECTION_JITTER, &NcpBase::GetPropertyHandler_THREAD_ROUTER_SELECTION_JITTER },
#endif

#if OPENTHREAD_ENABLE_JAM_DETECTION
    { SPINEL_PROP_JAM_DETECT_ENABLE, &NcpBase::GetPropertyHandler_JAM_DETECT_ENABLE },
    { SPINEL_PROP_JAM_DETECTED, &NcpBase::GetPropertyHandler_JAM_DETECTED },
    { SPINEL_PROP_JAM_DETECT_RSSI_THRESHOLD, &NcpBase::GetPropertyHandler_JAM_DETECT_RSSI_THRESHOLD },
    { SPINEL_PROP_JAM_DETECT_WINDOW, &NcpBase::GetPropertyHandler_JAM_DETECT_WINDOW },
    { SPINEL_PROP_JAM_DETECT_BUSY, &NcpBase::GetPropertyHandler_JAM_DETECT_BUSY },
    { SPINEL_PROP_JAM_DETECT_HISTORY_BITMAP, &NcpBase::GetPropertyHandler_JAM_DETECT_HISTORY_BITMAP },
#endif

#if OPENTHREAD_ENABLE_TMF_PROXY && OPENTHREAD_FTD
    { SPINEL_PROP_THREAD_TMF_PROXY_ENABLED, &NcpBase::GetPropertyHandler_TMF_PROXY_ENABLED },
#endif

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
    { SPINEL_PROP_CNTR_TX_PKT_UNICAST, &NcpBase::GetPropertyHandler_MAC_CNTR },
    { SPINEL_PROP_CNTR_TX_PKT_BROADCAST, &NcpBase::GetPropertyHandler_MAC_CNTR },
    { SPINEL_PROP_CNTR_TX_ERR_CCA, &NcpBase::GetPropertyHandler_MAC_CNTR },
    { SPINEL_PROP_CNTR_TX_ERR_ABORT, &NcpBase::GetPropertyHandler_MAC_CNTR },
    { SPINEL_PROP_CNTR_RX_PKT_TOTAL, &NcpBase::GetPropertyHandler_MAC_CNTR },
    { SPINEL_PROP_CNTR_RX_PKT_DATA, &NcpBase::GetPropertyHandler_MAC_CNTR },
    { SPINEL_PROP_CNTR_RX_PKT_DATA_POLL, &NcpBase::GetPropertyHandler_MAC_CNTR },
    { SPINEL_PROP_CNTR_RX_PKT_BEACON, &NcpBase::GetPropertyHandler_MAC_CNTR },
    { SPINEL_PROP_CNTR_RX_PKT_BEACON_REQ, &NcpBase::GetPropertyHandler_MAC_CNTR },
    { SPINEL_PROP_CNTR_RX_PKT_OTHER, &NcpBase::GetPropertyHandler_MAC_CNTR },
    { SPINEL_PROP_CNTR_RX_PKT_FILT_WL, &NcpBase::GetPropertyHandler_MAC_CNTR },
    { SPINEL_PROP_CNTR_RX_PKT_FILT_DA, &NcpBase::GetPropertyHandler_MAC_CNTR },
    { SPINEL_PROP_CNTR_RX_PKT_UNICAST, &NcpBase::GetPropertyHandler_MAC_CNTR },
    { SPINEL_PROP_CNTR_RX_PKT_BROADCAST, &NcpBase::GetPropertyHandler_MAC_CNTR },
    { SPINEL_PROP_CNTR_RX_ERR_EMPTY, &NcpBase::GetPropertyHandler_MAC_CNTR },
    { SPINEL_PROP_CNTR_RX_ERR_UKWN_NBR, &NcpBase::GetPropertyHandler_MAC_CNTR },
    { SPINEL_PROP_CNTR_RX_ERR_NVLD_SADDR, &NcpBase::GetPropertyHandler_MAC_CNTR },
    { SPINEL_PROP_CNTR_RX_ERR_SECURITY, &NcpBase::GetPropertyHandler_MAC_CNTR },
    { SPINEL_PROP_CNTR_RX_ERR_BAD_FCS, &NcpBase::GetPropertyHandler_MAC_CNTR },
    { SPINEL_PROP_CNTR_RX_ERR_OTHER, &NcpBase::GetPropertyHandler_MAC_CNTR },
    { SPINEL_PROP_CNTR_RX_PKT_DUP, &NcpBase::GetPropertyHandler_MAC_CNTR },

    { SPINEL_PROP_CNTR_TX_IP_SEC_TOTAL, &NcpBase::GetPropertyHandler_NCP_CNTR },
    { SPINEL_PROP_CNTR_TX_IP_INSEC_TOTAL, &NcpBase::GetPropertyHandler_NCP_CNTR },
    { SPINEL_PROP_CNTR_TX_IP_DROPPED, &NcpBase::GetPropertyHandler_NCP_CNTR },
    { SPINEL_PROP_CNTR_RX_IP_SEC_TOTAL, &NcpBase::GetPropertyHandler_NCP_CNTR },
    { SPINEL_PROP_CNTR_RX_IP_INSEC_TOTAL, &NcpBase::GetPropertyHandler_NCP_CNTR },
    { SPINEL_PROP_CNTR_RX_IP_DROPPED, &NcpBase::GetPropertyHandler_NCP_CNTR },
    { SPINEL_PROP_CNTR_TX_SPINEL_TOTAL, &NcpBase::GetPropertyHandler_NCP_CNTR },
    { SPINEL_PROP_CNTR_RX_SPINEL_TOTAL, &NcpBase::GetPropertyHandler_NCP_CNTR },
    { SPINEL_PROP_CNTR_RX_SPINEL_ERR, &NcpBase::GetPropertyHandler_NCP_CNTR },

    { SPINEL_PROP_MSG_BUFFER_COUNTERS, &NcpBase::GetPropertyHandler_MSG_BUFFER_COUNTERS },
    { SPINEL_PROP_DEBUG_TEST_ASSERT, &NcpBase::GetPropertyHandler_DEBUG_TEST_ASSERT },
    { SPINEL_PROP_DEBUG_NCP_LOG_LEVEL, &NcpBase::GetPropertyHandler_DEBUG_NCP_LOG_LEVEL },

#if OPENTHREAD_ENABLE_LEGACY
    { SPINEL_PROP_NEST_LEGACY_ULA_PREFIX, &NcpBase::GetPropertyHandler_NEST_LEGACY_ULA_PREFIX },
#endif
};

const NcpBase::SetPropertyHandlerEntry NcpBase::mSetPropertyHandlerTable[] =
{
    { SPINEL_PROP_POWER_STATE, &NcpBase::SetPropertyHandler_POWER_STATE },
    { SPINEL_PROP_HOST_POWER_STATE, &NcpBase::SetPropertyHandler_HOST_POWER_STATE },

#if OPENTHREAD_ENABLE_RAW_LINK_API
    { SPINEL_PROP_PHY_ENABLED, &NcpBase::SetPropertyHandler_PHY_ENABLED },
    { SPINEL_PROP_MAC_15_4_SADDR, &NcpBase::SetPropertyHandler_MAC_15_4_SADDR },
    { SPINEL_PROP_STREAM_RAW, &NcpBase::SetPropertyHandler_STREAM_RAW },
#endif // OPENTHREAD_ENABLE_RAW_LINK_API
    { SPINEL_PROP_PHY_TX_POWER, &NcpBase::SetPropertyHandler_PHY_TX_POWER },
    { SPINEL_PROP_PHY_CHAN, &NcpBase::SetPropertyHandler_PHY_CHAN },
    { SPINEL_PROP_MAC_PROMISCUOUS_MODE, &NcpBase::SetPropertyHandler_MAC_PROMISCUOUS_MODE },

    { SPINEL_PROP_MAC_SCAN_MASK, &NcpBase::SetPropertyHandler_MAC_SCAN_MASK },
    { SPINEL_PROP_MAC_SCAN_STATE, &NcpBase::SetPropertyHandler_MAC_SCAN_STATE },
    { SPINEL_PROP_MAC_SCAN_PERIOD, &NcpBase::SetPropertyHandler_MAC_SCAN_PERIOD },
    { SPINEL_PROP_MAC_15_4_PANID, &NcpBase::SetPropertyHandler_MAC_15_4_PANID },
    { SPINEL_PROP_MAC_15_4_LADDR, &NcpBase::SetPropertyHandler_MAC_15_4_LADDR },
    { SPINEL_PROP_MAC_RAW_STREAM_ENABLED, &NcpBase::SetPropertyHandler_MAC_RAW_STREAM_ENABLED },

    { SPINEL_PROP_NET_IF_UP, &NcpBase::SetPropertyHandler_NET_IF_UP },
    { SPINEL_PROP_NET_STACK_UP, &NcpBase::SetPropertyHandler_NET_STACK_UP },
    { SPINEL_PROP_NET_ROLE, &NcpBase::SetPropertyHandler_NET_ROLE },
    { SPINEL_PROP_NET_NETWORK_NAME, &NcpBase::SetPropertyHandler_NET_NETWORK_NAME },
    { SPINEL_PROP_NET_XPANID, &NcpBase::SetPropertyHandler_NET_XPANID },
    { SPINEL_PROP_NET_MASTER_KEY, &NcpBase::SetPropertyHandler_NET_MASTER_KEY },
    { SPINEL_PROP_NET_KEY_SEQUENCE_COUNTER, &NcpBase::SetPropertyHandler_NET_KEY_SEQUENCE_COUNTER },
    { SPINEL_PROP_NET_KEY_SWITCH_GUARDTIME, &NcpBase::SetPropertyHandler_NET_KEY_SWITCH_GUARDTIME},

    { SPINEL_PROP_THREAD_ASSISTING_PORTS, &NcpBase::SetPropertyHandler_THREAD_ASSISTING_PORTS },

    { SPINEL_PROP_STREAM_NET_INSECURE, &NcpBase::SetPropertyHandler_STREAM_NET_INSECURE },
    { SPINEL_PROP_STREAM_NET, &NcpBase::SetPropertyHandler_STREAM_NET },

    { SPINEL_PROP_IPV6_ML_PREFIX, &NcpBase::SetPropertyHandler_IPV6_ML_PREFIX },
    { SPINEL_PROP_IPV6_ICMP_PING_OFFLOAD, &NcpBase::SetPropertyHandler_IPV6_ICMP_PING_OFFLOAD },
    { SPINEL_PROP_THREAD_RLOC16_DEBUG_PASSTHRU, &NcpBase::SetPropertyHandler_THREAD_RLOC16_DEBUG_PASSTHRU },

#if OPENTHREAD_ENABLE_MAC_WHITELIST
    { SPINEL_PROP_MAC_WHITELIST, &NcpBase::SetPropertyHandler_MAC_WHITELIST },
    { SPINEL_PROP_MAC_WHITELIST_ENABLED, &NcpBase::SetPropertyHandler_MAC_WHITELIST_ENABLED },
#endif

#if OPENTHREAD_ENABLE_RAW_LINK_API
    { SPINEL_PROP_MAC_SRC_MATCH_ENABLED, &NcpBase::SetPropertyHandler_MAC_SRC_MATCH_ENABLED },
    { SPINEL_PROP_MAC_SRC_MATCH_SHORT_ADDRESSES, &NcpBase::SetPropertyHandler_MAC_SRC_MATCH_SHORT_ADDRESSES },
    { SPINEL_PROP_MAC_SRC_MATCH_EXTENDED_ADDRESSES, &NcpBase::SetPropertyHandler_MAC_SRC_MATCH_EXTENDED_ADDRESSES },
#endif
    { SPINEL_PROP_THREAD_MODE, &NcpBase::SetPropertyHandler_THREAD_MODE },
    { SPINEL_PROP_NET_REQUIRE_JOIN_EXISTING, &NcpBase::SetPropertyHandler_NET_REQUIRE_JOIN_EXISTING },
    { SPINEL_PROP_DEBUG_NCP_LOG_LEVEL, &NcpBase::SetPropertyHandler_DEBUG_NCP_LOG_LEVEL },

    { SPINEL_PROP_THREAD_DISCOVERY_SCAN_JOINER_FLAG, &NcpBase::SetPropertyHandler_THREAD_DISCOVERY_SCAN_JOINER_FLAG },
    { SPINEL_PROP_THREAD_DISCOVERY_SCAN_ENABLE_FILTERING,
        &NcpBase::SetPropertyHandler_THREAD_DISCOVERY_SCAN_ENABLE_FILTERING },
    { SPINEL_PROP_THREAD_DISCOVERY_SCAN_PANID, &NcpBase::SetPropertyHandler_THREAD_DISCOVERY_SCAN_PANID },
#if OPENTHREAD_ENABLE_BORDER_ROUTER
    { SPINEL_PROP_THREAD_ALLOW_LOCAL_NET_DATA_CHANGE, &NcpBase::SetPropertyHandler_THREAD_ALLOW_LOCAL_NET_DATA_CHANGE },
#endif
#if OPENTHREAD_FTD
    { SPINEL_PROP_NET_PSKC, &NcpBase::SetPropertyHandler_NET_PSKC },
    { SPINEL_PROP_THREAD_CHILD_TIMEOUT, &NcpBase::SetPropertyHandler_THREAD_CHILD_TIMEOUT },
    { SPINEL_PROP_THREAD_NETWORK_ID_TIMEOUT, &NcpBase::SetPropertyHandler_THREAD_NETWORK_ID_TIMEOUT },
    { SPINEL_PROP_THREAD_LOCAL_LEADER_WEIGHT, &NcpBase::SetPropertyHandler_THREAD_LOCAL_LEADER_WEIGHT },
    { SPINEL_PROP_THREAD_ROUTER_ROLE_ENABLED, &NcpBase::SetPropertyHandler_THREAD_ROUTER_ROLE_ENABLED },
    { SPINEL_PROP_THREAD_CHILD_COUNT_MAX, &NcpBase::SetPropertyHandler_THREAD_CHILD_COUNT_MAX },
    { SPINEL_PROP_THREAD_ROUTER_UPGRADE_THRESHOLD, &NcpBase::SetPropertyHandler_THREAD_ROUTER_UPGRADE_THRESHOLD },
    { SPINEL_PROP_THREAD_ROUTER_DOWNGRADE_THRESHOLD, &NcpBase::SetPropertyHandler_THREAD_ROUTER_DOWNGRADE_THRESHOLD },
    { SPINEL_PROP_THREAD_CONTEXT_REUSE_DELAY, &NcpBase::SetPropertyHandler_THREAD_CONTEXT_REUSE_DELAY },
    { SPINEL_PROP_THREAD_ROUTER_SELECTION_JITTER, &NcpBase::SetPropertyHandler_THREAD_ROUTER_SELECTION_JITTER },
    { SPINEL_PROP_THREAD_PREFERRED_ROUTER_ID, &NcpBase::SetPropertyHandler_THREAD_PREFERRED_ROUTER_ID },
#if OPENTHREAD_CONFIG_ENABLE_STEERING_DATA_SET_OOB
    { SPINEL_PROP_THREAD_STEERING_DATA, &NcpBase::SetPropertyHandler_THREAD_THREAD_STEERING_DATA },
#endif // #if OPENTHREAD_CONFIG_ENABLE_STEERING_DATA_SET_OOB
#endif // #if OPENTHREAD_FTD

#if OPENTHREAD_ENABLE_JAM_DETECTION
    { SPINEL_PROP_JAM_DETECT_ENABLE, &NcpBase::SetPropertyHandler_JAM_DETECT_ENABLE },
    { SPINEL_PROP_JAM_DETECT_RSSI_THRESHOLD, &NcpBase::SetPropertyHandler_JAM_DETECT_RSSI_THRESHOLD },
    { SPINEL_PROP_JAM_DETECT_WINDOW, &NcpBase::SetPropertyHandler_JAM_DETECT_WINDOW },
    { SPINEL_PROP_JAM_DETECT_BUSY, &NcpBase::SetPropertyHandler_JAM_DETECT_BUSY },
#endif

#if OPENTHREAD_ENABLE_TMF_PROXY && OPENTHREAD_FTD
    { SPINEL_PROP_THREAD_TMF_PROXY_ENABLED, &NcpBase::SetPropertyHandler_TMF_PROXY_ENABLED },
    { SPINEL_PROP_THREAD_TMF_PROXY_STREAM, &NcpBase::SetPropertyHandler_THREAD_TMF_PROXY_STREAM },
#endif // OPENTHREAD_ENABLE_TMF_PROXY && OPENTHREAD_FTD

#if OPENTHREAD_ENABLE_DIAG
    { SPINEL_PROP_NEST_STREAM_MFG, &NcpBase::SetPropertyHandler_NEST_STREAM_MFG },
#endif

#if OPENTHREAD_ENABLE_LEGACY
    { SPINEL_PROP_NEST_LEGACY_ULA_PREFIX, &NcpBase::SetPropertyHandler_NEST_LEGACY_ULA_PREFIX },
#endif

#if OPENTHREAD_ENABLE_COMMISSIONER && OPENTHREAD_FTD
    { SPINEL_PROP_THREAD_COMMISSIONER_ENABLED, &NcpBase::SetPropertyHandler_THREAD_COMMISSIONER_ENABLED },
#endif
};

const NcpBase::InsertPropertyHandlerEntry NcpBase::mInsertPropertyHandlerTable[] =
{
#if OPENTHREAD_ENABLE_RAW_LINK_API
    { SPINEL_PROP_MAC_SRC_MATCH_SHORT_ADDRESSES, &NcpBase::InsertPropertyHandler_MAC_SRC_MATCH_SHORT_ADDRESSES },
    { SPINEL_PROP_MAC_SRC_MATCH_EXTENDED_ADDRESSES, &NcpBase::InsertPropertyHandler_MAC_SRC_MATCH_EXTENDED_ADDRESSES },
#endif
    { SPINEL_PROP_IPV6_ADDRESS_TABLE, &NcpBase::InsertPropertyHandler_IPV6_ADDRESS_TABLE },
#if OPENTHREAD_ENABLE_BORDER_ROUTER
    { SPINEL_PROP_THREAD_OFF_MESH_ROUTES, &NcpBase::InsertPropertyHandler_THREAD_OFF_MESH_ROUTES },
    { SPINEL_PROP_THREAD_ON_MESH_NETS, &NcpBase::InsertPropertyHandler_THREAD_ON_MESH_NETS },
#endif
    { SPINEL_PROP_THREAD_ASSISTING_PORTS, &NcpBase::InsertPropertyHandler_THREAD_ASSISTING_PORTS },
#if OPENTHREAD_ENABLE_COMMISSIONER && OPENTHREAD_FTD
    { SPINEL_PROP_THREAD_JOINERS, &NcpBase::NcpBase::InsertPropertyHandler_THREAD_JOINERS },
#endif

    { SPINEL_PROP_CNTR_RESET, &NcpBase::SetPropertyHandler_CNTR_RESET },
#if OPENTHREAD_ENABLE_MAC_WHITELIST
    { SPINEL_PROP_MAC_WHITELIST, &NcpBase::InsertPropertyHandler_MAC_WHITELIST },
#endif
};

const NcpBase::RemovePropertyHandlerEntry NcpBase::mRemovePropertyHandlerTable[] =
{
#if OPENTHREAD_ENABLE_RAW_LINK_API
    { SPINEL_PROP_MAC_SRC_MATCH_SHORT_ADDRESSES, &NcpBase::RemovePropertyHandler_MAC_SRC_MATCH_SHORT_ADDRESSES },
    { SPINEL_PROP_MAC_SRC_MATCH_EXTENDED_ADDRESSES, &NcpBase::RemovePropertyHandler_MAC_SRC_MATCH_EXTENDED_ADDRESSES },
#endif
    { SPINEL_PROP_IPV6_ADDRESS_TABLE, &NcpBase::RemovePropertyHandler_IPV6_ADDRESS_TABLE },
#if OPENTHREAD_ENABLE_BORDER_ROUTER
    { SPINEL_PROP_THREAD_OFF_MESH_ROUTES, &NcpBase::RemovePropertyHandler_THREAD_OFF_MESH_ROUTES },
    { SPINEL_PROP_THREAD_ON_MESH_NETS, &NcpBase::RemovePropertyHandler_THREAD_ON_MESH_NETS },
#endif
    { SPINEL_PROP_THREAD_ASSISTING_PORTS, &NcpBase::RemovePropertyHandler_THREAD_ASSISTING_PORTS },
#if OPENTHREAD_ENABLE_MAC_WHITELIST
    { SPINEL_PROP_MAC_WHITELIST, &NcpBase::RemovePropertyHandler_MAC_WHITELIST },
#endif
#if OPENTHREAD_FTD
    { SPINEL_PROP_THREAD_ACTIVE_ROUTER_IDS, &NcpBase::RemovePropertyHandler_THREAD_ACTIVE_ROUTER_IDS },
#endif
};

// ----------------------------------------------------------------------------
// MARK: Utility Functions
// ----------------------------------------------------------------------------

static spinel_status_t ThreadErrorToSpinelStatus(otError error)
{
    spinel_status_t ret;

    switch (error)
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

static uint8_t BorderRouterConfigToFlagByte(const otBorderRouterConfig &config)
{
    uint8_t flags(0);

    if (config.mPreferred)
    {
        flags |= SPINEL_NET_FLAG_PREFERRED;
    }

    if (config.mSlaac)
    {
        flags |= SPINEL_NET_FLAG_SLAAC;
    }

    if (config.mDhcp)
    {
        flags |= SPINEL_NET_FLAG_DHCP;
    }

    if (config.mDefaultRoute)
    {
        flags |= SPINEL_NET_FLAG_DEFAULT_ROUTE;
    }

    if (config.mConfigure)
    {
        flags |= SPINEL_NET_FLAG_CONFIGURE;
    }

    if (config.mOnMesh)
    {
        flags |= SPINEL_NET_FLAG_ON_MESH;
    }

    flags |= (config.mPreference << SPINEL_NET_FLAG_PREFERENCE_OFFSET);

    return flags;
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
    mUpdateChangedPropsTask(aInstance->mIp6.mTaskletScheduler, &NcpBase::UpdateChangedProps, this),
    mChangedFlags(NCP_PLAT_RESET_REASON),
    mShouldSignalEndOfScan(false),
    mHostPowerState(SPINEL_HOST_POWER_STATE_ONLINE),
    mHostPowerStateInProgress(false),
    mHostPowerReplyFrameTag(NcpFrameBuffer::kInvalidTag),
    mHostPowerStateHeader(0),
#if OPENTHREAD_ENABLE_JAM_DETECTION
    mShouldSignalJamStateChange(false),
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
    otError errorCode = OT_ERROR_NONE;
    uint16_t length = otMessageGetLength(aMessage);

    SuccessOrExit(errorCode = OutboundFrameBegin());

    SuccessOrExit(
        errorCode = OutboundFrameFeedPacked(
                        SPINEL_DATATYPE_COMMAND_PROP_S SPINEL_DATATYPE_UINT16_S,
                        SPINEL_HEADER_FLAG | SPINEL_HEADER_IID_0,
                        SPINEL_CMD_PROP_VALUE_IS,
                        SPINEL_PROP_THREAD_TMF_PROXY_STREAM,
                        length
                    ));

    SuccessOrExit(errorCode = OutboundFrameFeedMessage(aMessage));

    SuccessOrExit(
        errorCode = OutboundFrameFeedPacked(
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

    SuccessOrExit(errorCode = OutboundFrameSend());

exit:

    if (aMessage != NULL)
    {
        otMessageFree(aMessage);
    }

    if (errorCode != OT_ERROR_NONE)
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
    otError errorCode = OT_ERROR_NONE;
    bool isSecure = otMessageIsLinkSecurityEnabled(aMessage);
    uint16_t length = otMessageGetLength(aMessage);

    SuccessOrExit(errorCode = OutboundFrameBegin());

    SuccessOrExit(
        errorCode = OutboundFrameFeedPacked(
                        SPINEL_DATATYPE_COMMAND_PROP_S SPINEL_DATATYPE_UINT16_S,
                        SPINEL_HEADER_FLAG | SPINEL_HEADER_IID_0,
                        SPINEL_CMD_PROP_VALUE_IS,
                        isSecure ? SPINEL_PROP_STREAM_NET : SPINEL_PROP_STREAM_NET_INSECURE,
                        length
                    ));

    SuccessOrExit(errorCode = OutboundFrameFeedMessage(aMessage));

    // Set the `aMessage` pointer to NULL to indicate that it does
    // not need to be freed at the exit. The `aMessage` is now owned
    // by the OutboundFrame and will be freed when the frame is either
    // successfully sent and then removed, or if the frame gets
    // discarded.
    aMessage = NULL;

    // Append any metadata (rssi, lqi, channel, etc) here!

    SuccessOrExit(errorCode = OutboundFrameSend());

exit:

    if (aMessage != NULL)
    {
        otMessageFree(aMessage);
    }

    if (errorCode != OT_ERROR_NONE)
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
    otError errorCode = OT_ERROR_NONE;
    uint16_t flags = 0;

    if (!mIsRawStreamEnabled)
    {
        goto exit;
    }

    SuccessOrExit(errorCode = OutboundFrameBegin());

    if (aFrame->mDidTX)
    {
        flags |= SPINEL_MD_FLAG_TX;
    }

    // Append frame header and frame length
    SuccessOrExit(
        errorCode = OutboundFrameFeedPacked(
                        SPINEL_DATATYPE_COMMAND_PROP_S SPINEL_DATATYPE_UINT16_S,
                        SPINEL_HEADER_FLAG | SPINEL_HEADER_IID_0,
                        SPINEL_CMD_PROP_VALUE_IS,
                        SPINEL_PROP_STREAM_RAW,
                        aFrame->mLength
                    ));

    // Append the frame contents
    SuccessOrExit(errorCode = OutboundFrameFeedData(aFrame->mPsdu, aFrame->mLength));

    // Append metadata (rssi, etc)
    SuccessOrExit(
        errorCode = OutboundFrameFeedPacked(
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

    SuccessOrExit(errorCode = OutboundFrameSend());

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
    otError errorCode;

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
        errorCode = SendPropertyUpdate(
                        SPINEL_HEADER_FLAG | SPINEL_HEADER_IID_0,
                        SPINEL_CMD_PROP_VALUE_IS,
                        SPINEL_PROP_MAC_SCAN_STATE,
                        SPINEL_DATATYPE_UINT8_S,
                        SPINEL_SCAN_STATE_IDLE
                    );

        // If we could not send the end of scan indicator message now (no
        // buffer space), we set `mShouldSignalEndOfScan` to true to send
        // it out when buffer space becomes available.
        if (errorCode != OT_ERROR_NONE)
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
    otError errorCode;

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
        errorCode = SendPropertyUpdate(
                        SPINEL_HEADER_FLAG | SPINEL_HEADER_IID_0,
                        SPINEL_CMD_PROP_VALUE_IS,
                        SPINEL_PROP_MAC_SCAN_STATE,
                        SPINEL_DATATYPE_UINT8_S,
                        SPINEL_SCAN_STATE_IDLE
                    );

        // If we could not send the end of scan indicator message now (no
        // buffer space), we set `mShouldSignalEndOfScan` to true to send
        // it out when buffer space becomes available.
        if (errorCode != OT_ERROR_NONE)
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
    otError errorCode = OT_ERROR_NONE;
    uint16_t flags = 0;

    SuccessOrExit(errorCode = OutboundFrameBegin());

    if (aFrame->mDidTX)
    {
        flags |= SPINEL_MD_FLAG_TX;
    }

    // Append frame header and frame length
    SuccessOrExit(
        errorCode = OutboundFrameFeedPacked(
                        SPINEL_DATATYPE_COMMAND_PROP_S SPINEL_DATATYPE_UINT16_S,
                        SPINEL_HEADER_FLAG | SPINEL_HEADER_IID_0,
                        SPINEL_CMD_PROP_VALUE_IS,
                        SPINEL_PROP_STREAM_RAW,
                        (aError == OT_ERROR_NONE) ? aFrame->mLength : 0
                    ));

    if (aError == OT_ERROR_NONE)
    {
        // Append the frame contents
        SuccessOrExit(errorCode = OutboundFrameFeedData(aFrame->mPsdu, aFrame->mLength));
    }

    // Append metadata (rssi, etc)
    SuccessOrExit(
        errorCode = OutboundFrameFeedPacked(
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

    SuccessOrExit(errorCode = OutboundFrameSend());

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

void NcpBase::HandleNetifStateChanged(uint32_t flags, void *context)
{
    NcpBase *obj = static_cast<NcpBase *>(context);

    obj->mChangedFlags |= flags;

    obj->mUpdateChangedPropsTask.Post();
}

void NcpBase::UpdateChangedProps(void *context)
{
    NcpBase *obj = static_cast<NcpBase *>(context);
    obj->UpdateChangedProps();
}

void NcpBase::UpdateChangedProps(void)
{
    while (mChangedFlags != 0)
    {
        if ((mChangedFlags & NCP_PLAT_RESET_REASON) != 0)
        {
            SuccessOrExit(
                SendLastStatus(
                    SPINEL_HEADER_FLAG | SPINEL_HEADER_IID_0,
                    ResetReasonToSpinelStatus(otPlatGetResetReason(mInstance))
                ));

            mChangedFlags &= ~static_cast<uint32_t>(NCP_PLAT_RESET_REASON);
        }
        else if ((mChangedFlags & OT_IP6_LL_ADDR_CHANGED) != 0)
        {
            SuccessOrExit(
                HandleCommandPropertyGet(
                    SPINEL_HEADER_FLAG | SPINEL_HEADER_IID_0,
                    SPINEL_PROP_IPV6_LL_ADDR
                ));

            mChangedFlags &= ~static_cast<uint32_t>(OT_IP6_LL_ADDR_CHANGED);
        }
        else if ((mChangedFlags & OT_IP6_ML_ADDR_CHANGED) != 0)
        {
            SuccessOrExit(
                HandleCommandPropertyGet(
                    SPINEL_HEADER_FLAG | SPINEL_HEADER_IID_0,
                    SPINEL_PROP_IPV6_ML_ADDR
                ));

            mChangedFlags &= ~static_cast<uint32_t>(OT_IP6_ML_ADDR_CHANGED);
        }
        else if ((mChangedFlags & OT_NET_ROLE) != 0)
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
                    mChangedFlags &= ~static_cast<uint32_t>(OT_NET_PARTITION_ID);
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

            mChangedFlags &= ~static_cast<uint32_t>(OT_NET_ROLE);
        }
        else if ((mChangedFlags & OT_NET_PARTITION_ID) != 0)
        {
            SuccessOrExit(
                HandleCommandPropertyGet(
                    SPINEL_HEADER_FLAG | SPINEL_HEADER_IID_0,
                    SPINEL_PROP_NET_PARTITION_ID
                ));

            mChangedFlags &= ~static_cast<uint32_t>(OT_NET_PARTITION_ID);
        }
        else if ((mChangedFlags & OT_NET_KEY_SEQUENCE_COUNTER) != 0)
        {
            SuccessOrExit(
                HandleCommandPropertyGet(
                    SPINEL_HEADER_FLAG | SPINEL_HEADER_IID_0,
                    SPINEL_PROP_NET_KEY_SEQUENCE_COUNTER
                ));

            mChangedFlags &= ~static_cast<uint32_t>(OT_NET_KEY_SEQUENCE_COUNTER);
        }
        else if ((mChangedFlags & (OT_IP6_ADDRESS_ADDED | OT_IP6_ADDRESS_REMOVED)) != 0)
        {
            SuccessOrExit(
                HandleCommandPropertyGet(
                    SPINEL_HEADER_FLAG | SPINEL_HEADER_IID_0,
                    SPINEL_PROP_IPV6_ADDRESS_TABLE
                ));

            mChangedFlags &= ~static_cast<uint32_t>(OT_IP6_ADDRESS_ADDED | OT_IP6_ADDRESS_REMOVED);
        }
        else if ((mChangedFlags & (OT_THREAD_CHILD_ADDED | OT_THREAD_CHILD_REMOVED)) != 0)
        {
            SuccessOrExit(
                HandleCommandPropertyGet(
                    SPINEL_HEADER_FLAG | SPINEL_HEADER_IID_0,
                    SPINEL_PROP_THREAD_CHILD_TABLE
                ));

            mChangedFlags &= ~static_cast<uint32_t>(OT_THREAD_CHILD_ADDED | OT_THREAD_CHILD_REMOVED);
        }
        else if ((mChangedFlags & OT_THREAD_NETDATA_UPDATED) != 0)
        {
            SuccessOrExit(
                HandleCommandPropertyGet(
                    SPINEL_HEADER_FLAG | SPINEL_HEADER_IID_0,
                    SPINEL_PROP_THREAD_LEADER_NETWORK_DATA
                ));

            mChangedFlags &= ~static_cast<uint32_t>(OT_THREAD_NETDATA_UPDATED);

            // If the network data is updated, after successfully sending (or queuing) the
            // network data spinel message, we add `NCP_ON_MESH_NETS_CHANGED_BIT_FLAG` to
            // the `mChangedFlags` so that we separately send the list of on-mesh prefixes.

            mChangedFlags |= NCP_ON_MESH_NETS_CHANGED_BIT_FLAG;
        }
        else if ((mChangedFlags & NCP_ON_MESH_NETS_CHANGED_BIT_FLAG) != 0)
        {
            SuccessOrExit(
                HandleCommandPropertyGet(
                    SPINEL_HEADER_FLAG | SPINEL_HEADER_IID_0,
                    SPINEL_PROP_THREAD_ON_MESH_NETS
                ));

            mChangedFlags &= ~static_cast<uint32_t>(NCP_ON_MESH_NETS_CHANGED_BIT_FLAG);
        }
        else if ((mChangedFlags & (OT_IP6_RLOC_ADDED | OT_IP6_RLOC_REMOVED)) != 0)
        {
            mChangedFlags &= ~static_cast<uint32_t>(OT_IP6_RLOC_ADDED | OT_IP6_RLOC_REMOVED);
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
    otError errorCode;

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
    otError errorCode = OT_ERROR_NONE;
    spinel_tid_t tid = 0;

    parsedLength = spinel_datatype_unpack(
                       buf,
                       bufLength,
                       SPINEL_DATATYPE_COMMAND_S SPINEL_DATATYPE_DATA_S,
                       &header,
                       &command,
                       &arg_ptr,
                       &arg_len
                   );

    tid = SPINEL_HEADER_GET_TID(header);

    // Receiving any message from the host has the side effect of transitioning the host power state to online.
    mHostPowerState = SPINEL_HOST_POWER_STATE_ONLINE;
    mHostPowerStateInProgress = false;

    if (parsedLength == bufLength)
    {
        errorCode = HandleCommand(header, command, arg_ptr, static_cast<uint16_t>(arg_len));

        // Check if we may have missed a `tid` in the sequence.
        if ((mNextExpectedTid != 0) && (tid != mNextExpectedTid))
        {
            mRxSpinelOutOfOrderTidCounter++;
        }

        mNextExpectedTid = SPINEL_GET_NEXT_TID(tid);
    }
    else
    {
        errorCode = SendLastStatus(header, SPINEL_STATUS_PARSE_ERROR);
    }

    if (errorCode == OT_ERROR_NO_BUFS)
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

void NcpBase::HandleFrameRemovedFromNcpBuffer(void *aContext, NcpFrameBuffer::FrameTag aFrameTag, NcpFrameBuffer *aNcpBuffer)
{
    (void)aNcpBuffer;
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

otError NcpBase::HandleCommand(uint8_t header, unsigned int command, const uint8_t *arg_ptr, uint16_t arg_len)
{
    unsigned i;
    otError errorCode = OT_ERROR_NONE;

    // Skip if this isn't a spinel frame
    VerifyOrExit((SPINEL_HEADER_FLAG & header) == SPINEL_HEADER_FLAG, errorCode = OT_ERROR_INVALID_ARGS);

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

otError NcpBase::HandleCommandPropertyGet(uint8_t header, spinel_prop_key_t key)
{
    unsigned i;
    otError errorCode = OT_ERROR_NONE;

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

otError NcpBase::HandleCommandPropertySet(uint8_t header, spinel_prop_key_t key, const uint8_t *value_ptr,
                                          uint16_t value_len)
{
    unsigned i;
    otError errorCode = OT_ERROR_NONE;

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

otError NcpBase::HandleCommandPropertyInsert(uint8_t header, spinel_prop_key_t key, const uint8_t *value_ptr,
                                             uint16_t value_len)
{
    unsigned i;
    otError errorCode = OT_ERROR_NONE;

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

otError NcpBase::HandleCommandPropertyRemove(uint8_t header, spinel_prop_key_t key, const uint8_t *value_ptr,
                                             uint16_t value_len)
{
    unsigned i;
    otError errorCode = OT_ERROR_NONE;

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


otError NcpBase::SendLastStatus(uint8_t header, spinel_status_t lastStatus)
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

otError NcpBase::SendPropertyUpdate(uint8_t header, uint8_t command, spinel_prop_key_t key,
                                    const char *pack_format, ...)
{
    otError errorCode = OT_ERROR_NONE;
    va_list     args;

    va_start(args, pack_format);
    SuccessOrExit(errorCode = OutboundFrameBegin());
    SuccessOrExit(errorCode = OutboundFrameFeedPacked(SPINEL_DATATYPE_COMMAND_PROP_S, header, command, key));
    SuccessOrExit(errorCode = OutboundFrameFeedVPacked(pack_format, args));
    SuccessOrExit(errorCode = OutboundFrameSend());

exit:
    va_end(args);
    return errorCode;
}

otError NcpBase::SendPropertyUpdate(uint8_t header, uint8_t command, spinel_prop_key_t key,
                                    const uint8_t *value_ptr, uint16_t value_len)
{
    otError errorCode = OT_ERROR_NONE;

    SuccessOrExit(errorCode = OutboundFrameBegin());
    SuccessOrExit(errorCode = OutboundFrameFeedPacked(SPINEL_DATATYPE_COMMAND_PROP_S, header, command, key));
    SuccessOrExit(errorCode = OutboundFrameFeedData(value_ptr, value_len));
    SuccessOrExit(errorCode = OutboundFrameSend());

exit:
    return errorCode;
}

otError NcpBase::SendPropertyUpdate(uint8_t header, uint8_t command, spinel_prop_key_t key, otMessage *aMessage)
{
    otError errorCode = OT_ERROR_NONE;

    SuccessOrExit(errorCode = OutboundFrameBegin());
    SuccessOrExit(errorCode = OutboundFrameFeedPacked(SPINEL_DATATYPE_COMMAND_PROP_S, header, command, key));
    SuccessOrExit(errorCode = OutboundFrameFeedMessage(aMessage));

    // Set the `aMessage` pointer to NULL to indicate that it does
    // not need to be freed at the exit. The `aMessage` is now owned
    // by the OutboundFrame and will be freed when the frame is either
    // successfully sent and then removed, or if the frame gets
    // discarded.
    aMessage = NULL;

    SuccessOrExit(errorCode = OutboundFrameSend());

exit:

    if (aMessage != NULL)
    {
        otMessageFree(aMessage);
    }

    return errorCode;
}

otError NcpBase::OutboundFrameFeedVPacked(const char *pack_format, va_list args)
{
    uint8_t buf[96];
    otError errorCode = OT_ERROR_NO_BUFS;
    spinel_ssize_t packed_len;

    packed_len = spinel_datatype_vpack(buf, sizeof(buf), pack_format, args);

    if ((packed_len > 0) && (packed_len <= static_cast<spinel_ssize_t>(sizeof(buf))))
    {
        errorCode = OutboundFrameFeedData(buf, static_cast<uint16_t>(packed_len));
    }

    return errorCode;
}

otError NcpBase::OutboundFrameFeedPacked(const char *pack_format, ...)
{
    otError errorCode;
    va_list args;

    va_start(args, pack_format);
    errorCode = OutboundFrameFeedVPacked(pack_format, args);
    va_end(args);

    return errorCode;
}

// ----------------------------------------------------------------------------
// MARK: Individual Command Handlers
// ----------------------------------------------------------------------------

otError NcpBase::CommandHandler_NOOP(uint8_t header, unsigned int command, const uint8_t *arg_ptr, uint16_t arg_len)
{
    (void)command;
    (void)arg_ptr;
    (void)arg_len;

    return SendLastStatus(header, SPINEL_STATUS_OK);
}

otError NcpBase::CommandHandler_RESET(uint8_t header, unsigned int command, const uint8_t *arg_ptr,
                                      uint16_t arg_len)
{
    otError errorCode = OT_ERROR_NONE;

    // We aren't using any of the arguments to this function.
    (void)header;
    (void)command;
    (void)arg_ptr;
    (void)arg_len;

    // Signal a platform reset. If implemented, this function
    // shouldn't return.
    otInstanceReset(mInstance);

    // We only get to this point if the
    // platform doesn't support resetting.
    // In such a case we fake it.

    otThreadSetEnabled(mInstance, false);
    otIp6SetEnabled(mInstance, false);

    errorCode = SendLastStatus(SPINEL_HEADER_FLAG | SPINEL_HEADER_IID_0, SPINEL_STATUS_RESET_SOFTWARE);

    if (errorCode != OT_ERROR_NONE)
    {
        mChangedFlags |= NCP_PLAT_RESET_REASON;
        mUpdateChangedPropsTask.Post();
    }

    return errorCode;
}

otError NcpBase::CommandHandler_PROP_VALUE_GET(uint8_t header, unsigned int command, const uint8_t *arg_ptr,
                                               uint16_t arg_len)
{
    unsigned int propKey = 0;
    spinel_ssize_t parsedLength;
    otError errorCode = OT_ERROR_NONE;

    parsedLength = spinel_datatype_unpack(arg_ptr, arg_len, SPINEL_DATATYPE_UINT_PACKED_S, &propKey);

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

otError NcpBase::CommandHandler_PROP_VALUE_SET(uint8_t header, unsigned int command, const uint8_t *arg_ptr,
                                               uint16_t arg_len)
{
    unsigned int propKey = 0;
    spinel_ssize_t parsedLength;
    const uint8_t *value_ptr;
    unsigned int value_len;
    otError errorCode = OT_ERROR_NONE;

    parsedLength = spinel_datatype_unpack(
                       arg_ptr,
                       arg_len,
                       SPINEL_DATATYPE_UINT_PACKED_S SPINEL_DATATYPE_DATA_S,
                       &propKey,
                       &value_ptr,
                       &value_len
                   );

    if (parsedLength == arg_len)
    {
        errorCode = HandleCommandPropertySet(
                        header,
                        static_cast<spinel_prop_key_t>(propKey),
                        value_ptr,
                        static_cast<uint16_t>(value_len)
                    );
    }
    else
    {
        errorCode = SendLastStatus(header, SPINEL_STATUS_PARSE_ERROR);
    }

    (void)command;

    return errorCode;
}

otError NcpBase::CommandHandler_PROP_VALUE_INSERT(uint8_t header, unsigned int command, const uint8_t *arg_ptr,
                                                  uint16_t arg_len)
{
    unsigned int propKey = 0;
    spinel_ssize_t parsedLength;
    const uint8_t *value_ptr;
    unsigned int value_len;
    otError errorCode = OT_ERROR_NONE;

    parsedLength = spinel_datatype_unpack(
                       arg_ptr,
                       arg_len,
                       SPINEL_DATATYPE_UINT_PACKED_S SPINEL_DATATYPE_DATA_S,
                       &propKey,
                       &value_ptr,
                       &value_len
                   );

    if (parsedLength == arg_len)
    {
        errorCode = HandleCommandPropertyInsert(
                        header,
                        static_cast<spinel_prop_key_t>(propKey),
                        value_ptr,
                        static_cast<uint16_t>(value_len)
                    );
    }
    else
    {
        errorCode = SendLastStatus(header, SPINEL_STATUS_PARSE_ERROR);
    }

    (void)command;

    return errorCode;
}

otError NcpBase::CommandHandler_PROP_VALUE_REMOVE(uint8_t header, unsigned int command, const uint8_t *arg_ptr,
                                                  uint16_t arg_len)
{
    unsigned int propKey = 0;
    spinel_ssize_t parsedLength;
    const uint8_t *value_ptr;
    unsigned int value_len;
    otError errorCode = OT_ERROR_NONE;

    parsedLength = spinel_datatype_unpack(
                       arg_ptr,
                       arg_len,
                       SPINEL_DATATYPE_UINT_PACKED_S SPINEL_DATATYPE_DATA_S,
                       &propKey,
                       &value_ptr,
                       &value_len
                   );

    if (parsedLength == arg_len)
    {
        errorCode = HandleCommandPropertyRemove(
                        header,
                        static_cast<spinel_prop_key_t>(propKey),
                        value_ptr,
                        static_cast<uint16_t>(value_len)
                    );
    }
    else
    {
        errorCode = SendLastStatus(header, SPINEL_STATUS_PARSE_ERROR);
    }

    (void)command;

    return errorCode;
}

otError NcpBase::CommandHandler_NET_SAVE(uint8_t header, unsigned int command, const uint8_t *arg_ptr,
                                         uint16_t arg_len)
{
    (void)command;
    (void)arg_ptr;
    (void)arg_len;

    return SendLastStatus(header, SPINEL_STATUS_UNIMPLEMENTED);
}

otError NcpBase::CommandHandler_NET_CLEAR(uint8_t header, unsigned int command, const uint8_t *arg_ptr,
                                          uint16_t arg_len)
{
    (void)command;
    (void)arg_ptr;
    (void)arg_len;

    return SendLastStatus(header, ThreadErrorToSpinelStatus(otInstanceErasePersistentInfo(mInstance)));
}

otError NcpBase::CommandHandler_NET_RECALL(uint8_t header, unsigned int command, const uint8_t *arg_ptr,
                                           uint16_t arg_len)
{
    (void)command;
    (void)arg_ptr;
    (void)arg_len;

    return SendLastStatus(header, SPINEL_STATUS_UNIMPLEMENTED);
}

// ----------------------------------------------------------------------------
// MARK: Individual Property Getters
// ----------------------------------------------------------------------------


otError NcpBase::GetPropertyHandler_LAST_STATUS(uint8_t header, spinel_prop_key_t key)
{
    return SendPropertyUpdate(header, SPINEL_CMD_PROP_VALUE_IS, key, SPINEL_DATATYPE_UINT_PACKED_S, mLastStatus);
}

otError NcpBase::GetPropertyHandler_PROTOCOL_VERSION(uint8_t header, spinel_prop_key_t key)
{
    return SendPropertyUpdate(
               header,
               SPINEL_CMD_PROP_VALUE_IS,
               key,
               (
                   SPINEL_DATATYPE_UINT_PACKED_S   // Major
                   SPINEL_DATATYPE_UINT_PACKED_S   // Minor
               ),
               SPINEL_PROTOCOL_VERSION_THREAD_MAJOR,
               SPINEL_PROTOCOL_VERSION_THREAD_MINOR
           );
}

otError NcpBase::GetPropertyHandler_INTERFACE_TYPE(uint8_t header, spinel_prop_key_t key)
{
    return SendPropertyUpdate(
               header,
               SPINEL_CMD_PROP_VALUE_IS,
               key,
               SPINEL_DATATYPE_UINT_PACKED_S,
               SPINEL_PROTOCOL_TYPE_THREAD
           );
}

otError NcpBase::GetPropertyHandler_VENDOR_ID(uint8_t header, spinel_prop_key_t key)
{
    return SendPropertyUpdate(
               header,
               SPINEL_CMD_PROP_VALUE_IS,
               key,
               SPINEL_DATATYPE_UINT_PACKED_S,
               0 // Vendor ID. Zero for unknown.
           );
}

otError NcpBase::GetPropertyHandler_CAPS(uint8_t header, spinel_prop_key_t key)
{
    otError errorCode = OT_ERROR_NONE;

    SuccessOrExit(errorCode = OutboundFrameBegin());
    SuccessOrExit(
        errorCode = OutboundFrameFeedPacked(
                        SPINEL_DATATYPE_COMMAND_PROP_S,
                        header,
                        SPINEL_CMD_PROP_VALUE_IS,
                        key
                    ));

    // Begin adding capabilities //////////////////////////////////////////////

    SuccessOrExit(errorCode = OutboundFrameFeedPacked(SPINEL_DATATYPE_UINT_PACKED_S, SPINEL_CAP_NET_THREAD_1_0));
    SuccessOrExit(errorCode = OutboundFrameFeedPacked(SPINEL_DATATYPE_UINT_PACKED_S, SPINEL_CAP_COUNTERS));

#if OPENTHREAD_ENABLE_MAC_WHITELIST
    SuccessOrExit(errorCode = OutboundFrameFeedPacked(SPINEL_DATATYPE_UINT_PACKED_S, SPINEL_CAP_MAC_WHITELIST));
#endif

#if OPENTHREAD_ENABLE_RAW_LINK_API
    SuccessOrExit(errorCode = OutboundFrameFeedPacked(SPINEL_DATATYPE_UINT_PACKED_S, SPINEL_CAP_MAC_RAW));
#endif

#if OPENTHREAD_ENABLE_JAM_DETECTION
    SuccessOrExit(errorCode = OutboundFrameFeedPacked(SPINEL_DATATYPE_UINT_PACKED_S, SPINEL_CAP_JAM_DETECT));
#endif

#if OPENTHREAD_CONFIG_ENABLE_STEERING_DATA_SET_OOB
    SuccessOrExit(errorCode = OutboundFrameFeedPacked(SPINEL_DATATYPE_UINT_PACKED_S, SPINEL_CAP_OOB_STEERING_DATA));
#endif

    // TODO: Somehow get the following capability from the radio.
    SuccessOrExit(
        errorCode = OutboundFrameFeedPacked(
                        SPINEL_DATATYPE_UINT_PACKED_S,
                        SPINEL_CAP_802_15_4_2450MHZ_OQPSK
                    ));

#if OPENTHREAD_CONFIG_MAX_CHILDREN > 0
    SuccessOrExit(errorCode = OutboundFrameFeedPacked(SPINEL_DATATYPE_UINT_PACKED_S, SPINEL_CAP_ROLE_ROUTER));
#endif

#if OPENTHREAD_ENABLE_LEGACY
    SuccessOrExit(
        errorCode = OutboundFrameFeedPacked(
                        SPINEL_DATATYPE_UINT_PACKED_S,
                        SPINEL_CAP_NEST_LEGACY_INTERFACE
                    ));
#endif

#if OPENTHREAD_ENABLE_TMF_PROXY && OPENTHREAD_FTD
    SuccessOrExit(errorCode = OutboundFrameFeedPacked(SPINEL_DATATYPE_UINT_PACKED_S, SPINEL_CAP_THREAD_TMF_PROXY));
#endif

    // End adding capabilities /////////////////////////////////////////////////

    SuccessOrExit(errorCode = OutboundFrameSend());

exit:
    return errorCode;
}

otError NcpBase::GetPropertyHandler_NCP_VERSION(uint8_t header, spinel_prop_key_t key)
{
    return SendPropertyUpdate(
               header,
               SPINEL_CMD_PROP_VALUE_IS,
               key,
               SPINEL_DATATYPE_UTF8_S,
               otGetVersionString()
           );
}

otError NcpBase::GetPropertyHandler_INTERFACE_COUNT(uint8_t header, spinel_prop_key_t key)
{
    return SendPropertyUpdate(
               header,
               SPINEL_CMD_PROP_VALUE_IS,
               key,
               SPINEL_DATATYPE_UINT8_S,
               1 // Only one interface for now
           );
}

otError NcpBase::GetPropertyHandler_POWER_STATE(uint8_t header, spinel_prop_key_t key)
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

otError NcpBase::GetPropertyHandler_HWADDR(uint8_t header, spinel_prop_key_t key)
{
    otExtAddress hwAddr;
    otLinkGetFactoryAssignedIeeeEui64(mInstance, &hwAddr);

    return SendPropertyUpdate(
               header,
               SPINEL_CMD_PROP_VALUE_IS,
               key,
               SPINEL_DATATYPE_EUI64_S,
               hwAddr.m8
           );
}

otError NcpBase::GetPropertyHandler_LOCK(uint8_t header, spinel_prop_key_t key)
{
    // TODO: Implement property lock (Needs API!)
    (void)key;

    return SendLastStatus(header, SPINEL_STATUS_UNIMPLEMENTED);
}

otError NcpBase::GetPropertyHandler_HOST_POWER_STATE(uint8_t header, spinel_prop_key_t key)
{
    return SendPropertyUpdate(
               header,
               SPINEL_CMD_PROP_VALUE_IS,
               key,
               SPINEL_DATATYPE_UINT8_S,
               mHostPowerState
           );
}

otError NcpBase::GetPropertyHandler_PHY_ENABLED(uint8_t header, spinel_prop_key_t key)
{
    return SendPropertyUpdate(
               header,
               SPINEL_CMD_PROP_VALUE_IS,
               key,
               SPINEL_DATATYPE_BOOL_S,
#if OPENTHREAD_ENABLE_RAW_LINK_API
               otLinkRawIsEnabled(mInstance)
#else
               false
#endif // OPENTHREAD_ENABLE_RAW_LINK_API
           );
}

otError NcpBase::GetPropertyHandler_PHY_FREQ(uint8_t header, spinel_prop_key_t key)
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
               header,
               SPINEL_CMD_PROP_VALUE_IS,
               key,
               SPINEL_DATATYPE_UINT32_S,
               freq_khz
           );
}

otError NcpBase::GetPropertyHandler_PHY_CHAN_SUPPORTED(uint8_t header, spinel_prop_key_t key)
{
    return GetPropertyHandler_ChannelMaskHelper(header, key, mSupportedChannelMask);
}

otError NcpBase::GetPropertyHandler_PHY_CHAN(uint8_t header, spinel_prop_key_t key)
{
    return SendPropertyUpdate(
               header,
               SPINEL_CMD_PROP_VALUE_IS,
               key,
               SPINEL_DATATYPE_UINT8_S,
               otLinkGetChannel(mInstance)
           );
}

otError NcpBase::GetPropertyHandler_PHY_RSSI(uint8_t header, spinel_prop_key_t key)
{
    return SendPropertyUpdate(
               header,
               SPINEL_CMD_PROP_VALUE_IS,
               key,
               SPINEL_DATATYPE_INT8_S,
               otPlatRadioGetRssi(mInstance)
           );
}

otError NcpBase::GetPropertyHandler_PHY_TX_POWER(uint8_t header, spinel_prop_key_t key)
{
    return SendPropertyUpdate(
               header,
               SPINEL_CMD_PROP_VALUE_IS,
               key,
               SPINEL_DATATYPE_INT8_S,
               otLinkGetMaxTransmitPower(mInstance)
           );
}

otError NcpBase::GetPropertyHandler_PHY_RX_SENSITIVITY(uint8_t header, spinel_prop_key_t key)
{
    return SendPropertyUpdate(
               header,
               SPINEL_CMD_PROP_VALUE_IS,
               key,
               SPINEL_DATATYPE_INT8_S,
               otPlatRadioGetReceiveSensitivity(mInstance)
           );
}

otError NcpBase::GetPropertyHandler_MAC_SCAN_STATE(uint8_t header, spinel_prop_key_t key)
{
    otError errorCode = OT_ERROR_NONE;

#if OPENTHREAD_ENABLE_RAW_LINK_API

    if (otLinkRawIsEnabled(mInstance))
    {
        errorCode = SendPropertyUpdate(
                        header,
                        SPINEL_CMD_PROP_VALUE_IS,
                        key,
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
            errorCode = SendPropertyUpdate(
                            header,
                            SPINEL_CMD_PROP_VALUE_IS,
                            key,
                            SPINEL_DATATYPE_UINT8_S,
                            SPINEL_SCAN_STATE_BEACON
                        );
        }
        else if (otLinkIsEnergyScanInProgress(mInstance))
        {
            errorCode = SendPropertyUpdate(
                            header,
                            SPINEL_CMD_PROP_VALUE_IS,
                            key,
                            SPINEL_DATATYPE_UINT8_S,
                            SPINEL_SCAN_STATE_ENERGY
                        );
        }
        else if (otThreadIsDiscoverInProgress(mInstance))
        {
            errorCode = SendPropertyUpdate(
                            header,
                            SPINEL_CMD_PROP_VALUE_IS,
                            key,
                            SPINEL_DATATYPE_UINT8_S,
                            SPINEL_SCAN_STATE_DISCOVER
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
    }

    return errorCode;
}

otError NcpBase::GetPropertyHandler_MAC_SCAN_PERIOD(uint8_t header, spinel_prop_key_t key)
{
    return SendPropertyUpdate(
               header,
               SPINEL_CMD_PROP_VALUE_IS,
               key,
               SPINEL_DATATYPE_UINT16_S,
               mScanPeriod
           );
}

otError NcpBase::GetPropertyHandler_ChannelMaskHelper(uint8_t header, spinel_prop_key_t key, uint32_t channel_mask)
{
    otError errorCode = OT_ERROR_NONE;

    SuccessOrExit(errorCode = OutboundFrameBegin());
    SuccessOrExit(
        errorCode = OutboundFrameFeedPacked(
                        SPINEL_DATATYPE_COMMAND_PROP_S,
                        header,
                        SPINEL_CMD_PROP_VALUE_IS,
                        key
                    ));

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

otError NcpBase::GetPropertyHandler_MAC_SCAN_MASK(uint8_t header, spinel_prop_key_t key)
{
    return GetPropertyHandler_ChannelMaskHelper(header, key, mChannelMask);
}

otError NcpBase::GetPropertyHandler_MAC_15_4_PANID(uint8_t header, spinel_prop_key_t key)
{
    return SendPropertyUpdate(
               header,
               SPINEL_CMD_PROP_VALUE_IS,
               key,
               SPINEL_DATATYPE_UINT16_S,
               otLinkGetPanId(mInstance)
           );
}

otError NcpBase::GetPropertyHandler_MAC_PROMISCUOUS_MODE(uint8_t header, spinel_prop_key_t key)
{
    return SendPropertyUpdate(
               header,
               SPINEL_CMD_PROP_VALUE_IS,
               key,
               SPINEL_DATATYPE_INT8_S,
               otPlatRadioGetPromiscuous(mInstance)
                   ? SPINEL_MAC_PROMISCUOUS_MODE_FULL
                   : SPINEL_MAC_PROMISCUOUS_MODE_OFF
           );
}

otError NcpBase::GetPropertyHandler_MAC_15_4_LADDR(uint8_t header, spinel_prop_key_t key)
{
    return SendPropertyUpdate(
               header,
               SPINEL_CMD_PROP_VALUE_IS,
               key,
               SPINEL_DATATYPE_EUI64_S,
               otLinkGetExtendedAddress(mInstance)
           );
}

otError NcpBase::GetPropertyHandler_MAC_15_4_SADDR(uint8_t header, spinel_prop_key_t key)
{
    return SendPropertyUpdate(
               header,
               SPINEL_CMD_PROP_VALUE_IS,
               key,
               SPINEL_DATATYPE_UINT16_S,
               otLinkGetShortAddress(mInstance)
           );
}

otError NcpBase::GetPropertyHandler_MAC_EXTENDED_ADDR(uint8_t header, spinel_prop_key_t key)
{
    return SendPropertyUpdate(
               header,
               SPINEL_CMD_PROP_VALUE_IS,
               key,
               SPINEL_DATATYPE_EUI64_S,
               otLinkGetExtendedAddress(mInstance)
           );
}

otError NcpBase::GetPropertyHandler_MAC_RAW_STREAM_ENABLED(uint8_t header, spinel_prop_key_t key)
{
    return SendPropertyUpdate(
               header,
               SPINEL_CMD_PROP_VALUE_IS,
               key,
               SPINEL_DATATYPE_BOOL_S,
               mIsRawStreamEnabled
           );
}

otError NcpBase::GetPropertyHandler_NET_SAVED(uint8_t header, spinel_prop_key_t key)
{
    return SendPropertyUpdate(
               header,
               SPINEL_CMD_PROP_VALUE_IS,
               key,
               SPINEL_DATATYPE_BOOL_S,
               otDatasetIsCommissioned(mInstance)
           );
}

otError NcpBase::GetPropertyHandler_NET_IF_UP(uint8_t header, spinel_prop_key_t key)
{
    return SendPropertyUpdate(
               header,
               SPINEL_CMD_PROP_VALUE_IS,
               key,
               SPINEL_DATATYPE_BOOL_S,
               otIp6IsEnabled(mInstance)
           );
}

otError NcpBase::GetPropertyHandler_NET_STACK_UP(uint8_t header, spinel_prop_key_t key)
{
    return SendPropertyUpdate(
               header,
               SPINEL_CMD_PROP_VALUE_IS,
               key,
               SPINEL_DATATYPE_BOOL_S,
               otThreadGetDeviceRole(mInstance) != OT_DEVICE_ROLE_DISABLED
           );
}

otError NcpBase::GetPropertyHandler_NET_ROLE(uint8_t header, spinel_prop_key_t key)
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
               header,
               SPINEL_CMD_PROP_VALUE_IS,
               key,
               SPINEL_DATATYPE_UINT8_S,
               role
           );
}

otError NcpBase::GetPropertyHandler_NET_NETWORK_NAME(uint8_t header, spinel_prop_key_t key)
{
    return SendPropertyUpdate(
               header,
               SPINEL_CMD_PROP_VALUE_IS,
               key,
               SPINEL_DATATYPE_UTF8_S,
               otThreadGetNetworkName(mInstance)
           );
}

otError NcpBase::GetPropertyHandler_NET_XPANID(uint8_t header, spinel_prop_key_t key)
{
    return SendPropertyUpdate(
               header,
               SPINEL_CMD_PROP_VALUE_IS,
               key,
               SPINEL_DATATYPE_DATA_S,
               otThreadGetExtendedPanId(mInstance),
               sizeof(spinel_net_xpanid_t)
           );
}

otError NcpBase::GetPropertyHandler_NET_MASTER_KEY(uint8_t header, spinel_prop_key_t key)
{
    return SendPropertyUpdate(
               header,
               SPINEL_CMD_PROP_VALUE_IS,
               key,
               SPINEL_DATATYPE_DATA_S,
               otThreadGetMasterKey(mInstance)->m8,
               OT_MASTER_KEY_SIZE
           );
}

otError NcpBase::GetPropertyHandler_NET_KEY_SEQUENCE_COUNTER(uint8_t header, spinel_prop_key_t key)
{
    return SendPropertyUpdate(
               header,
               SPINEL_CMD_PROP_VALUE_IS,
               key,
               SPINEL_DATATYPE_UINT32_S,
               otThreadGetKeySequenceCounter(mInstance)
           );
}

otError NcpBase::GetPropertyHandler_NET_PARTITION_ID(uint8_t header, spinel_prop_key_t key)
{
    return SendPropertyUpdate(
               header,
               SPINEL_CMD_PROP_VALUE_IS,
               key,
               SPINEL_DATATYPE_UINT32_S,
               otThreadGetPartitionId(mInstance)
           );
}

otError NcpBase::GetPropertyHandler_NET_KEY_SWITCH_GUARDTIME(uint8_t header, spinel_prop_key_t key)
{
    return SendPropertyUpdate(
               header,
               SPINEL_CMD_PROP_VALUE_IS,
               key,
               SPINEL_DATATYPE_UINT32_S,
               otThreadGetKeySwitchGuardTime(mInstance)
           );
}

otError NcpBase::GetPropertyHandler_THREAD_NETWORK_DATA_VERSION(uint8_t header, spinel_prop_key_t key)
{
    return SendPropertyUpdate(
               header,
               SPINEL_CMD_PROP_VALUE_IS,
               key,
               SPINEL_DATATYPE_UINT8_S,
               otNetDataGetVersion(mInstance)
           );
}

otError NcpBase::GetPropertyHandler_THREAD_STABLE_NETWORK_DATA_VERSION(uint8_t header, spinel_prop_key_t key)
{
    return SendPropertyUpdate(
               header,
               SPINEL_CMD_PROP_VALUE_IS,
               key,
               SPINEL_DATATYPE_UINT8_S,
               otNetDataGetStableVersion(mInstance)
           );
}

#if OPENTHREAD_ENABLE_BORDER_ROUTER
otError NcpBase::GetPropertyHandler_THREAD_NETWORK_DATA(uint8_t header, spinel_prop_key_t key)
{
    otError errorCode = OT_ERROR_NONE;
    uint8_t network_data[255];
    uint8_t network_data_len = 255;

    otBorderRouterGetNetData(
        mInstance,
        false, // Stable?
        network_data,
        &network_data_len
    );

    SuccessOrExit(errorCode = OutboundFrameBegin());
    SuccessOrExit(
        errorCode = OutboundFrameFeedPacked(
                        SPINEL_DATATYPE_COMMAND_PROP_S,
                        header,
                        SPINEL_CMD_PROP_VALUE_IS,
                        key
                    ));
    SuccessOrExit(errorCode = OutboundFrameFeedData(network_data, network_data_len));
    SuccessOrExit(errorCode = OutboundFrameSend());

exit:
    return errorCode;
}

otError NcpBase::GetPropertyHandler_THREAD_STABLE_NETWORK_DATA(uint8_t header, spinel_prop_key_t key)
{
    otError errorCode = OT_ERROR_NONE;
    uint8_t network_data[255];
    uint8_t network_data_len = 255;

    otBorderRouterGetNetData(
        mInstance,
        true, // Stable?
        network_data,
        &network_data_len
    );

    SuccessOrExit(errorCode = OutboundFrameBegin());
    SuccessOrExit(
        errorCode = OutboundFrameFeedPacked(
                        SPINEL_DATATYPE_COMMAND_PROP_S,
                        header,
                        SPINEL_CMD_PROP_VALUE_IS,
                        key
                    ));
    SuccessOrExit(errorCode = OutboundFrameFeedData(network_data, network_data_len));
    SuccessOrExit(errorCode = OutboundFrameSend());

exit:
    return errorCode;
}
#endif // OPENTHREAD_ENABLE_BORDER_ROUTER

otError NcpBase::GetPropertyHandler_THREAD_LEADER_NETWORK_DATA(uint8_t header, spinel_prop_key_t key)
{
    otError errorCode = OT_ERROR_NONE;
    uint8_t network_data[255];
    uint8_t network_data_len = 255;

    otNetDataGet(
        mInstance,
        false, // Stable?
        network_data,
        &network_data_len
    );

    SuccessOrExit(errorCode = OutboundFrameBegin());
    SuccessOrExit(
        errorCode = OutboundFrameFeedPacked(
                        SPINEL_DATATYPE_COMMAND_PROP_S,
                        header,
                        SPINEL_CMD_PROP_VALUE_IS,
                        key
                    ));
    SuccessOrExit(errorCode = OutboundFrameFeedData(network_data, network_data_len));
    SuccessOrExit(errorCode = OutboundFrameSend());

exit:
    return errorCode;
}

otError NcpBase::GetPropertyHandler_THREAD_STABLE_LEADER_NETWORK_DATA(uint8_t header, spinel_prop_key_t key)
{
    otError errorCode = OT_ERROR_NONE;
    uint8_t network_data[255];
    uint8_t network_data_len = 255;

    otNetDataGet(
        mInstance,
        true, // Stable?
        network_data,
        &network_data_len
    );

    SuccessOrExit(errorCode = OutboundFrameBegin());
    SuccessOrExit(
        errorCode = OutboundFrameFeedPacked(
                        SPINEL_DATATYPE_COMMAND_PROP_S,
                        header,
                        SPINEL_CMD_PROP_VALUE_IS,
                        key
                    ));
    SuccessOrExit(errorCode = OutboundFrameFeedData(network_data, network_data_len));
    SuccessOrExit(errorCode = OutboundFrameSend());

exit:
    return errorCode;
}

otError NcpBase::GetPropertyHandler_THREAD_LEADER_RID(uint8_t header, spinel_prop_key_t key)
{
    return SendPropertyUpdate(
               header,
               SPINEL_CMD_PROP_VALUE_IS,
               key,
               SPINEL_DATATYPE_UINT8_S,
               otThreadGetLeaderRouterId(mInstance)
           );
}

#if OPENTHREAD_FTD
otError NcpBase::GetPropertyHandler_THREAD_LOCAL_LEADER_WEIGHT(uint8_t header, spinel_prop_key_t key)
{
    return SendPropertyUpdate(
               header,
               SPINEL_CMD_PROP_VALUE_IS,
               key,
               SPINEL_DATATYPE_UINT8_S,
               otThreadGetLocalLeaderWeight(mInstance)
           );
}
#endif  // OPENTHREAD_FTD

otError NcpBase::GetPropertyHandler_THREAD_LEADER_WEIGHT(uint8_t header, spinel_prop_key_t key)
{
    return SendPropertyUpdate(
               header,
               SPINEL_CMD_PROP_VALUE_IS,
               key,
               SPINEL_DATATYPE_UINT8_S,
               otThreadGetLeaderWeight(mInstance)
           );
}

otError NcpBase::GetPropertyHandler_THREAD_LEADER_ADDR(uint8_t header, spinel_prop_key_t key)
{
    otError errorCode = OT_ERROR_NONE;
    otIp6Address address;

    errorCode = otThreadGetLeaderRloc(mInstance, &address);

    if (errorCode == OT_ERROR_NONE)
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

otError NcpBase::GetPropertyHandler_THREAD_PARENT(uint8_t header, spinel_prop_key_t key)
{
    otError errorCode = OT_ERROR_NONE;
    otRouterInfo parentInfo;

    errorCode = otThreadGetParentInfo(mInstance, &parentInfo);

    if (errorCode == OT_ERROR_NONE)
    {
        errorCode = SendPropertyUpdate(
                        header,
                        SPINEL_CMD_PROP_VALUE_IS,
                        key,
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
        errorCode = SendLastStatus(header, ThreadErrorToSpinelStatus(errorCode));
    }

    return errorCode;
}

#if OPENTHREAD_FTD
otError NcpBase::GetPropertyHandler_THREAD_CHILD_TABLE(uint8_t header, spinel_prop_key_t key)
{
    otError errorCode = OT_ERROR_NONE;
    otChildInfo childInfo;
    uint8_t maxChildren;
    uint8_t modeFlags;

    mDisableStreamWrite = true;

    SuccessOrExit(errorCode = OutboundFrameBegin());
    SuccessOrExit(
        errorCode = OutboundFrameFeedPacked(
                        SPINEL_DATATYPE_COMMAND_PROP_S,
                        header,
                        SPINEL_CMD_PROP_VALUE_IS,
                        key
                    ));

    maxChildren = otThreadGetMaxAllowedChildren(mInstance);

    for (uint8_t index = 0; index < maxChildren; index++)
    {
        errorCode = otThreadGetChildInfoByIndex(mInstance, index, &childInfo);

        if (errorCode != OT_ERROR_NONE)
        {
            continue;
        }

        modeFlags = 0;

        if (childInfo.mRxOnWhenIdle)
        {
            modeFlags |= kThreadMode_RxOnWhenIdle;
        }

        if (childInfo.mSecureDataRequest)
        {
            modeFlags |= kThreadMode_SecureDataRequest;
        }

        if (childInfo.mFullFunction)
        {
            modeFlags |= kThreadMode_FullFunctionDevice;
        }

        if (childInfo.mFullNetworkData)
        {
            modeFlags |= kThreadMode_FullNetworkData;
        }

        SuccessOrExit(
            errorCode = OutboundFrameFeedPacked(
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

    SuccessOrExit(errorCode = OutboundFrameSend());

exit:
    mDisableStreamWrite = false;
    return errorCode;
}
#endif  // OPENTHREAD_FTD

otError NcpBase::GetPropertyHandler_THREAD_NEIGHBOR_TABLE(uint8_t header, spinel_prop_key_t key)
{
    otError errorCode = OT_ERROR_NONE;
    otNeighborInfoIterator iter = OT_NEIGHBOR_INFO_ITERATOR_INIT;
    otNeighborInfo neighInfo;
    uint8_t modeFlags;

    mDisableStreamWrite = true;

    SuccessOrExit(errorCode = OutboundFrameBegin());
    SuccessOrExit(errorCode = OutboundFrameFeedPacked(SPINEL_DATATYPE_COMMAND_PROP_S, header, SPINEL_CMD_PROP_VALUE_IS,
                                                      key));

    while (otThreadGetNextNeighborInfo(mInstance, &iter, &neighInfo) == OT_ERROR_NONE)
    {
        modeFlags = 0;

        if (neighInfo.mRxOnWhenIdle)
        {
            modeFlags |= kThreadMode_RxOnWhenIdle;
        }

        if (neighInfo.mSecureDataRequest)
        {
            modeFlags |= kThreadMode_SecureDataRequest;
        }

        if (neighInfo.mFullFunction)
        {
            modeFlags |= kThreadMode_FullFunctionDevice;
        }

        if (neighInfo.mFullNetworkData)
        {
            modeFlags |= kThreadMode_FullNetworkData;
        }

        SuccessOrExit(
            errorCode = OutboundFrameFeedPacked(
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

    SuccessOrExit(errorCode = OutboundFrameSend());

exit:
    mDisableStreamWrite = false;
    return errorCode;
}

otError NcpBase::GetPropertyHandler_THREAD_ASSISTING_PORTS(uint8_t header, spinel_prop_key_t key)
{
    otError errorCode = OT_ERROR_NONE;
    uint8_t num_entries = 0;
    const uint16_t *ports = otIp6GetUnsecurePorts(mInstance, &num_entries);

    SuccessOrExit(errorCode = OutboundFrameBegin());
    SuccessOrExit(errorCode = OutboundFrameFeedPacked(SPINEL_DATATYPE_COMMAND_PROP_S, header, SPINEL_CMD_PROP_VALUE_IS,
                                                      key));

    for (; num_entries != 0; ports++, num_entries--)
    {
        SuccessOrExit(errorCode = OutboundFrameFeedPacked(SPINEL_DATATYPE_UINT16_S, *ports));
    }

    SuccessOrExit(errorCode = OutboundFrameSend());

exit:
    return errorCode;
}

otError NcpBase::GetPropertyHandler_THREAD_ALLOW_LOCAL_NET_DATA_CHANGE(uint8_t header, spinel_prop_key_t key)
{
    return SendPropertyUpdate(
               header,
               SPINEL_CMD_PROP_VALUE_IS,
               key,
               SPINEL_DATATYPE_BOOL_S,
               mAllowLocalNetworkDataChange
           );
}

#if OPENTHREAD_FTD
otError NcpBase::GetPropertyHandler_THREAD_ROUTER_ROLE_ENABLED(uint8_t header, spinel_prop_key_t key)
{
    return SendPropertyUpdate(
               header,
               SPINEL_CMD_PROP_VALUE_IS,
               key,
               SPINEL_DATATYPE_BOOL_S,
               otThreadIsRouterRoleEnabled(mInstance)
           );
}
#endif  // OPENTHREAD_FTD

otError NcpBase::GetPropertyHandler_THREAD_ON_MESH_NETS(uint8_t header, spinel_prop_key_t key)
{
    otError errorCode = OT_ERROR_NONE;
    otBorderRouterConfig border_router_config;
    uint8_t flags;

    mDisableStreamWrite = true;

    SuccessOrExit(errorCode = OutboundFrameBegin());
    SuccessOrExit(
        errorCode = OutboundFrameFeedPacked(
                        SPINEL_DATATYPE_COMMAND_PROP_S,
                        header,
                        SPINEL_CMD_PROP_VALUE_IS,
                        key
                    ));

    // Fill from non-local network data first
    for (otNetworkDataIterator iter = OT_NETWORK_DATA_ITERATOR_INIT ;;)
    {
        errorCode = otNetDataGetNextOnMeshPrefix(mInstance, &iter, &border_router_config);

        if (errorCode != OT_ERROR_NONE)
        {
            break;
        }

        flags = BorderRouterConfigToFlagByte(border_router_config);

        SuccessOrExit(errorCode = OutboundFrameFeedPacked(
                                      SPINEL_DATATYPE_STRUCT_S(
                                          SPINEL_DATATYPE_IPv6ADDR_S      // IPv6 Prefix
                                          SPINEL_DATATYPE_UINT8_S         // Prefix Length (in bits)
                                          SPINEL_DATATYPE_BOOL_S          // isStable
                                          SPINEL_DATATYPE_UINT8_S         // Flags
                                          SPINEL_DATATYPE_BOOL_S          // isLocal
                                      ),
                                      &border_router_config.mPrefix,
                                      64,
                                      border_router_config.mStable,
                                      flags,
                                      false
                                  ));
    }

#if OPENTHREAD_ENABLE_BORDER_ROUTER
    // Fill from local network data last
    for (otNetworkDataIterator iter = OT_NETWORK_DATA_ITERATOR_INIT ;;)
    {
        errorCode = otBorderRouterGetNextOnMeshPrefix(mInstance, &iter, &border_router_config);

        if (errorCode != OT_ERROR_NONE)
        {
            break;
        }

        flags = BorderRouterConfigToFlagByte(border_router_config);

        SuccessOrExit(errorCode = OutboundFrameFeedPacked(
                                      SPINEL_DATATYPE_STRUCT_S(
                                          SPINEL_DATATYPE_IPv6ADDR_S      // IPv6 Prefix
                                          SPINEL_DATATYPE_UINT8_S         // Prefix Length (in bits)
                                          SPINEL_DATATYPE_BOOL_S          // isStable
                                          SPINEL_DATATYPE_UINT8_S         // Flags
                                          SPINEL_DATATYPE_BOOL_S          // isLocal
                                      ),
                                      &border_router_config.mPrefix,
                                      64,
                                      border_router_config.mStable,
                                      flags,
                                      true
                                  ));
    }
#endif // OPENTHREAD_ENABLE_BORDER_ROUTER

    SuccessOrExit(errorCode = OutboundFrameSend());

exit:
    mDisableStreamWrite = false;
    return errorCode;
}

otError NcpBase::GetPropertyHandler_THREAD_DISCOVERY_SCAN_JOINER_FLAG(uint8_t header, spinel_prop_key_t key)
{
    return SendPropertyUpdate(
               header,
               SPINEL_CMD_PROP_VALUE_IS,
               key,
               SPINEL_DATATYPE_BOOL_S,
               mDiscoveryScanJoinerFlag
           );
}

otError NcpBase::GetPropertyHandler_THREAD_DISCOVERY_SCAN_ENABLE_FILTERING(uint8_t header, spinel_prop_key_t key)
{
    return SendPropertyUpdate(
               header,
               SPINEL_CMD_PROP_VALUE_IS,
               key,
               SPINEL_DATATYPE_BOOL_S,
               mDiscoveryScanEnableFiltering
           );
}

otError NcpBase::GetPropertyHandler_THREAD_DISCOVERY_SCAN_PANID(uint8_t header, spinel_prop_key_t key)
{
    return SendPropertyUpdate(
               header,
               SPINEL_CMD_PROP_VALUE_IS,
               key,
               SPINEL_DATATYPE_UINT16_S,
               mDiscoveryScanPanId
           );
}

otError NcpBase::GetPropertyHandler_IPV6_ML_PREFIX(uint8_t header, spinel_prop_key_t key)
{
    otError errorCode = OT_ERROR_NONE;
    const uint8_t *ml_prefix = otThreadGetMeshLocalPrefix(mInstance);

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
        errorCode = SendPropertyUpdate(
                        header,
                        SPINEL_CMD_PROP_VALUE_IS,
                        key,
                        SPINEL_DATATYPE_VOID_S
                    );
    }

    return errorCode;
}

otError NcpBase::GetPropertyHandler_IPV6_ML_ADDR(uint8_t header, spinel_prop_key_t key)
{
    otError errorCode = OT_ERROR_NONE;
    const otIp6Address *ml64 = otThreadGetMeshLocalEid(mInstance);

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

otError NcpBase::GetPropertyHandler_IPV6_LL_ADDR(uint8_t header, spinel_prop_key_t key)
{
    otError errorCode = OT_ERROR_NONE;
    const otIp6Address *address = otThreadGetLinkLocalIp6Address(mInstance);

    if (address)
    {
        errorCode = SendPropertyUpdate(
                        header,
                        SPINEL_CMD_PROP_VALUE_IS,
                        key,
                        SPINEL_DATATYPE_IPv6ADDR_S,
                        address
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

otError NcpBase::GetPropertyHandler_IPV6_ADDRESS_TABLE(uint8_t header, spinel_prop_key_t key)
{
    otError errorCode = OT_ERROR_NONE;

    mDisableStreamWrite = true;

    SuccessOrExit(errorCode = OutboundFrameBegin());
    SuccessOrExit(
        errorCode = OutboundFrameFeedPacked(
                        SPINEL_DATATYPE_COMMAND_PROP_S,
                        header,
                        SPINEL_CMD_PROP_VALUE_IS,
                        key
                    ));

    for (const otNetifAddress *address = otIp6GetUnicastAddresses(mInstance); address; address = address->mNext)
    {

        SuccessOrExit(
            errorCode = OutboundFrameFeedPacked(
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

    SuccessOrExit(errorCode = OutboundFrameSend());

exit:
    mDisableStreamWrite = false;
    return errorCode;
}

otError NcpBase::GetPropertyHandler_IPV6_ROUTE_TABLE(uint8_t header, spinel_prop_key_t key)
{
    // TODO: Implement get route table
    (void)key;

    return SendLastStatus(header, SPINEL_STATUS_UNIMPLEMENTED);
}

otError NcpBase::GetPropertyHandler_IPV6_ICMP_PING_OFFLOAD(uint8_t header, spinel_prop_key_t key)
{
    return SendPropertyUpdate(
               header,
               SPINEL_CMD_PROP_VALUE_IS,
               key,
               SPINEL_DATATYPE_BOOL_S,
               otIcmp6IsEchoEnabled(mInstance)
           );
}

otError NcpBase::GetPropertyHandler_THREAD_RLOC16_DEBUG_PASSTHRU(uint8_t header, spinel_prop_key_t key)
{
    // Note reverse logic: passthru enabled = filter disabled
    return SendPropertyUpdate(
               header,
               SPINEL_CMD_PROP_VALUE_IS,
               key,
               SPINEL_DATATYPE_BOOL_S,
               !otIp6IsReceiveFilterEnabled(mInstance)
           );
}

otError NcpBase::GetPropertyHandler_THREAD_OFF_MESH_ROUTES(uint8_t header, spinel_prop_key_t key)
{
    otError errorCode = OT_ERROR_NONE;
    otExternalRouteConfig external_route_config;
    otNetworkDataIterator iter = OT_NETWORK_DATA_ITERATOR_INIT;
    uint8_t flags;

    mDisableStreamWrite = true;

    SuccessOrExit(errorCode = OutboundFrameBegin());
    SuccessOrExit(
        errorCode = OutboundFrameFeedPacked(
                        SPINEL_DATATYPE_COMMAND_PROP_S,
                        header,
                        SPINEL_CMD_PROP_VALUE_IS,
                        key
                    ));

    while (otNetDataGetNextRoute(mInstance, &iter, &external_route_config) == OT_ERROR_NONE)
    {
        flags = static_cast<uint8_t>(external_route_config.mPreference);
        flags <<= SPINEL_NET_FLAG_PREFERENCE_OFFSET;

        SuccessOrExit(
            errorCode = OutboundFrameFeedPacked(
                            SPINEL_DATATYPE_STRUCT_S(
                                SPINEL_DATATYPE_IPv6ADDR_S      // IPv6 Prefix
                                SPINEL_DATATYPE_UINT8_S         // Prefix Length (in bits)
                                SPINEL_DATATYPE_BOOL_S          // isStable
                                SPINEL_DATATYPE_UINT8_S         // Flags
                                SPINEL_DATATYPE_BOOL_S          // IsLocal
                            ),
                            &external_route_config.mPrefix.mPrefix,
                            external_route_config.mPrefix.mLength,
                            external_route_config.mStable,
                            flags,
                            false
                        ));
    }

#if OPENTHREAD_ENABLE_BORDER_ROUTER
    while (otBorderRouterGetNextRoute(mInstance, &iter, &external_route_config) == OT_ERROR_NONE)
    {
        flags = static_cast<uint8_t>(external_route_config.mPreference);
        flags <<= SPINEL_NET_FLAG_PREFERENCE_OFFSET;

        SuccessOrExit(
            errorCode = OutboundFrameFeedPacked(
                            SPINEL_DATATYPE_STRUCT_S(
                                SPINEL_DATATYPE_IPv6ADDR_S      // IPv6 Prefix
                                SPINEL_DATATYPE_UINT8_S         // Prefix Length (in bits)
                                SPINEL_DATATYPE_BOOL_S          // isStable
                                SPINEL_DATATYPE_UINT8_S         // Flags
                                SPINEL_DATATYPE_BOOL_S          // IsLocal
                            ),
                            &external_route_config.mPrefix.mPrefix,
                            external_route_config.mPrefix.mLength,
                            external_route_config.mStable,
                            flags,
                            true
                        ));
    }
#endif // OPENTHREAD_ENABLE_BORDER_ROUTER

    SuccessOrExit(errorCode = OutboundFrameSend());

exit:
    mDisableStreamWrite = false;
    return errorCode;
}

otError NcpBase::GetPropertyHandler_STREAM_NET(uint8_t header, spinel_prop_key_t key)
{
    // TODO: Implement explicit data poll.
    (void)key;

    return SendLastStatus(header, SPINEL_STATUS_UNIMPLEMENTED);
}

#if OPENTHREAD_ENABLE_TMF_PROXY && OPENTHREAD_FTD
otError NcpBase::GetPropertyHandler_TMF_PROXY_ENABLED(uint8_t header, spinel_prop_key_t key)
{
    return SendPropertyUpdate(
               header,
               SPINEL_CMD_PROP_VALUE_IS,
               key,
               SPINEL_DATATYPE_BOOL_S,
               otTmfProxyIsEnabled(mInstance)
           );
}
#endif // OPENTHREAD_ENABLE_TMF_PROXY && OPENTHREAD_FTD

#if OPENTHREAD_ENABLE_JAM_DETECTION

otError NcpBase::GetPropertyHandler_JAM_DETECT_ENABLE(uint8_t header, spinel_prop_key_t key)
{
    return SendPropertyUpdate(
               header,
               SPINEL_CMD_PROP_VALUE_IS,
               key,
               SPINEL_DATATYPE_BOOL_S,
               otJamDetectionIsEnabled(mInstance)
           );
}

otError NcpBase::GetPropertyHandler_JAM_DETECTED(uint8_t header, spinel_prop_key_t key)
{
    return SendPropertyUpdate(
               header,
               SPINEL_CMD_PROP_VALUE_IS,
               key,
               SPINEL_DATATYPE_BOOL_S,
               otJamDetectionGetState(mInstance)
           );
}

otError NcpBase::GetPropertyHandler_JAM_DETECT_RSSI_THRESHOLD(uint8_t header, spinel_prop_key_t key)
{
    return SendPropertyUpdate(
               header,
               SPINEL_CMD_PROP_VALUE_IS,
               key,
               SPINEL_DATATYPE_INT8_S,
               otJamDetectionGetRssiThreshold(mInstance)
           );
}

otError NcpBase::GetPropertyHandler_JAM_DETECT_WINDOW(uint8_t header, spinel_prop_key_t key)
{
    return SendPropertyUpdate(
               header,
               SPINEL_CMD_PROP_VALUE_IS,
               key,
               SPINEL_DATATYPE_UINT8_S,
               otJamDetectionGetWindow(mInstance)
           );
}

otError NcpBase::GetPropertyHandler_JAM_DETECT_BUSY(uint8_t header, spinel_prop_key_t key)
{
    return SendPropertyUpdate(
               header,
               SPINEL_CMD_PROP_VALUE_IS,
               key,
               SPINEL_DATATYPE_UINT8_S,
               otJamDetectionGetBusyPeriod(mInstance)
           );
}

otError NcpBase::GetPropertyHandler_JAM_DETECT_HISTORY_BITMAP(uint8_t header, spinel_prop_key_t key)
{
    uint64_t historyBitmap = otJamDetectionGetHistoryBitmap(mInstance);

    return SendPropertyUpdate(
               header,
               SPINEL_CMD_PROP_VALUE_IS,
               key,
               (
                   SPINEL_DATATYPE_UINT32_S   // History bitmap - bits 0-31
                   SPINEL_DATATYPE_UINT32_S   // History bitmap - bits 32-63
               ),
               static_cast<uint32_t>(historyBitmap & 0xffffffff),
               static_cast<uint32_t>(historyBitmap >> 32)
           );
}

#endif // OPENTHREAD_ENABLE_JAM_DETECTION

otError NcpBase::GetPropertyHandler_MAC_CNTR(uint8_t header, spinel_prop_key_t key)
{
    uint32_t value;
    const otMacCounters *macCounters;
    otError errorCode = OT_ERROR_NONE;

    macCounters = otLinkGetCounters(mInstance);

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

otError NcpBase::GetPropertyHandler_NCP_CNTR(uint8_t header, spinel_prop_key_t key)
{
    uint32_t value;
    otError errorCode = OT_ERROR_NONE;

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

    case SPINEL_PROP_CNTR_RX_SPINEL_OUT_OF_ORDER_TID:
        value = mRxSpinelOutOfOrderTidCounter;
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

otError NcpBase::GetPropertyHandler_MSG_BUFFER_COUNTERS(uint8_t header, spinel_prop_key_t key)
{
    otError errorCode = OT_ERROR_NONE;
    otBufferInfo bufferInfo;

    otMessageGetBufferInfo(mInstance, &bufferInfo);

    SuccessOrExit(errorCode = OutboundFrameBegin());
    SuccessOrExit(
        errorCode = OutboundFrameFeedPacked(
                        SPINEL_DATATYPE_COMMAND_PROP_S,
                        header,
                        SPINEL_CMD_PROP_VALUE_IS,
                        key
                    ));

    SuccessOrExit(
        errorCode = OutboundFrameFeedPacked(
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

    SuccessOrExit(errorCode = OutboundFrameSend());

exit:
    return errorCode;
}

otError NcpBase::GetPropertyHandler_DEBUG_TEST_ASSERT(uint8_t header, spinel_prop_key_t key)
{
    assert(false);

    // We only get to this point if `assert(false)`
    // does not cause an NCP reset on the platform.
    // In such a case we return `false` as the
    // property value to indicate this.

    return SendPropertyUpdate(
               header,
               SPINEL_CMD_PROP_VALUE_IS,
               key,
               SPINEL_DATATYPE_BOOL_S,
               false
           );
}

otError NcpBase::GetPropertyHandler_DEBUG_NCP_LOG_LEVEL(uint8_t header, spinel_prop_key_t key)
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
               header,
               SPINEL_CMD_PROP_VALUE_IS,
               key,
               SPINEL_DATATYPE_UINT8_S,
               logLevel
           );
}

#if OPENTHREAD_ENABLE_MAC_WHITELIST

otError NcpBase::GetPropertyHandler_MAC_WHITELIST(uint8_t header, spinel_prop_key_t key)
{
    otMacWhitelistEntry entry;
    otError errorCode = OT_ERROR_NONE;

    mDisableStreamWrite = true;

    SuccessOrExit(errorCode = OutboundFrameBegin());
    SuccessOrExit(
        errorCode = OutboundFrameFeedPacked(
                        SPINEL_DATATYPE_COMMAND_PROP_S,
                        header,
                        SPINEL_CMD_PROP_VALUE_IS,
                        key
                    ));

    for (uint8_t i = 0; (i != 255) && (errorCode == OT_ERROR_NONE); i++)
    {
        errorCode = otLinkGetWhitelistEntry(mInstance, i, &entry);

        if (errorCode != OT_ERROR_NONE)
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
                errorCode = OutboundFrameFeedPacked(
                                SPINEL_DATATYPE_STRUCT_S(
                                    SPINEL_DATATYPE_EUI64_S   // Extended address
                                    SPINEL_DATATYPE_INT8_S    // Rssi
                                ),
                                entry.mExtAddress.m8,
                                entry.mRssi
                            ));
        }
    }

    SuccessOrExit(errorCode = OutboundFrameSend());

exit:
    mDisableStreamWrite = false;
    return errorCode;
}

otError NcpBase::GetPropertyHandler_MAC_WHITELIST_ENABLED(uint8_t header, spinel_prop_key_t key)
{
    return SendPropertyUpdate(
               header,
               SPINEL_CMD_PROP_VALUE_IS,
               key,
               SPINEL_DATATYPE_BOOL_S,
               otLinkIsWhitelistEnabled(mInstance)
           );
}

otError NcpBase::GetPropertyHandler_MAC_BLACKLIST(uint8_t header, spinel_prop_key_t key)
{
    otMacBlacklistEntry entry;
    otError errorCode = OT_ERROR_NONE;

    mDisableStreamWrite = true;

    SuccessOrExit(errorCode = OutboundFrameBegin());
    SuccessOrExit(
        errorCode = OutboundFrameFeedPacked(
                        SPINEL_DATATYPE_COMMAND_PROP_S,
                        header,
                        SPINEL_CMD_PROP_VALUE_IS,
                        key
                    ));

    for (uint8_t i = 0; (i != 255) && (errorCode == OT_ERROR_NONE); i++)
    {
        errorCode = otLinkGetBlacklistEntry(mInstance, i, &entry);

        if (errorCode != OT_ERROR_NONE)
        {
            break;
        }

        if (entry.mValid)
        {
            SuccessOrExit(
                errorCode = OutboundFrameFeedPacked(
                                SPINEL_DATATYPE_STRUCT_S(
                                    SPINEL_DATATYPE_EUI64_S   // Extended address
                                ),
                                entry.mExtAddress.m8
                            ));
        }
    }

    SuccessOrExit(errorCode = OutboundFrameSend());

exit:
    mDisableStreamWrite = false;
    return errorCode;
}

otError NcpBase::GetPropertyHandler_MAC_BLACKLIST_ENABLED(uint8_t header, spinel_prop_key_t key)
{
    return SendPropertyUpdate(
               header,
               SPINEL_CMD_PROP_VALUE_IS,
               key,
               SPINEL_DATATYPE_BOOL_S,
               otLinkIsBlacklistEnabled(mInstance)
           );
}


#endif // OPENTHREAD_ENABLE_MAC_WHITELIST

#if OPENTHREAD_FTD
otError NcpBase::GetPropertyHandler_NET_PSKC(uint8_t header, spinel_prop_key_t key)
{
    return SendPropertyUpdate(
               header,
               SPINEL_CMD_PROP_VALUE_IS,
               key,
               SPINEL_DATATYPE_DATA_S,
               otThreadGetPSKc(mInstance),
               sizeof(spinel_net_pskc_t)
           );
}
#endif // OPENTHREAD_FTD

otError NcpBase::GetPropertyHandler_THREAD_MODE(uint8_t header, spinel_prop_key_t key)
{
    uint8_t numeric_mode(0);
    otLinkModeConfig mode_config(otThreadGetLinkMode(mInstance));

    if (mode_config.mRxOnWhenIdle)
    {
        numeric_mode |= kThreadMode_RxOnWhenIdle;
    }

    if (mode_config.mSecureDataRequests)
    {
        numeric_mode |= kThreadMode_SecureDataRequest;
    }

    if (mode_config.mDeviceType)
    {
        numeric_mode |= kThreadMode_FullFunctionDevice;
    }

    if (mode_config.mNetworkData)
    {
        numeric_mode |= kThreadMode_FullNetworkData;
    }

    return SendPropertyUpdate(
               header,
               SPINEL_CMD_PROP_VALUE_IS,
               key,
               SPINEL_DATATYPE_UINT8_S,
               numeric_mode
           );
}

#if OPENTHREAD_FTD
otError NcpBase::GetPropertyHandler_THREAD_CHILD_COUNT_MAX(uint8_t header, spinel_prop_key_t key)
{
    return SendPropertyUpdate(
               header,
               SPINEL_CMD_PROP_VALUE_IS,
               key,
               SPINEL_DATATYPE_UINT8_S,
               otThreadGetMaxAllowedChildren(mInstance)
           );
}
#endif  // OPENTHREAD_FTD

otError NcpBase::GetPropertyHandler_THREAD_CHILD_TIMEOUT(uint8_t header, spinel_prop_key_t key)
{
    return SendPropertyUpdate(
               header,
               SPINEL_CMD_PROP_VALUE_IS,
               key,
               SPINEL_DATATYPE_UINT32_S,
               otThreadGetChildTimeout(mInstance)
           );
}

otError NcpBase::GetPropertyHandler_THREAD_RLOC16(uint8_t header, spinel_prop_key_t key)
{
    return SendPropertyUpdate(
               header,
               SPINEL_CMD_PROP_VALUE_IS,
               key,
               SPINEL_DATATYPE_UINT16_S,
               otThreadGetRloc16(mInstance)
           );
}

#if OPENTHREAD_FTD
otError NcpBase::GetPropertyHandler_THREAD_ROUTER_UPGRADE_THRESHOLD(uint8_t header, spinel_prop_key_t key)
{
    return SendPropertyUpdate(
               header,
               SPINEL_CMD_PROP_VALUE_IS,
               key,
               SPINEL_DATATYPE_UINT8_S,
               otThreadGetRouterUpgradeThreshold(mInstance)
           );
}

otError NcpBase::GetPropertyHandler_THREAD_ROUTER_DOWNGRADE_THRESHOLD(uint8_t header, spinel_prop_key_t key)
{
    return SendPropertyUpdate(
               header,
               SPINEL_CMD_PROP_VALUE_IS,
               key,
               SPINEL_DATATYPE_UINT8_S,
               otThreadGetRouterDowngradeThreshold(mInstance)
           );
}

otError NcpBase::GetPropertyHandler_THREAD_ROUTER_SELECTION_JITTER(uint8_t header, spinel_prop_key_t key)
{
    return SendPropertyUpdate(
               header,
               SPINEL_CMD_PROP_VALUE_IS,
               key,
               SPINEL_DATATYPE_UINT8_S,
               otThreadGetRouterSelectionJitter(mInstance)
           );
}

otError NcpBase::GetPropertyHandler_THREAD_CONTEXT_REUSE_DELAY(uint8_t header, spinel_prop_key_t key)
{
    return SendPropertyUpdate(
               header,
               SPINEL_CMD_PROP_VALUE_IS,
               key,
               SPINEL_DATATYPE_UINT32_S,
               otThreadGetContextIdReuseDelay(mInstance)
           );
}

otError NcpBase::GetPropertyHandler_THREAD_NETWORK_ID_TIMEOUT(uint8_t header, spinel_prop_key_t key)
{
    return SendPropertyUpdate(
               header,
               SPINEL_CMD_PROP_VALUE_IS,
               key,
               SPINEL_DATATYPE_UINT8_S,
               otThreadGetNetworkIdTimeout(mInstance)
           );
}
#endif  // OPENTHREAD_FTD

#if OPENTHREAD_ENABLE_COMMISSIONER && OPENTHREAD_FTD
otError NcpBase::GetPropertyHandler_THREAD_COMMISSIONER_ENABLED(uint8_t header, spinel_prop_key_t key)
{
    bool isEnabled = false;

    if (otCommissionerGetState(mInstance) == OT_COMMISSIONER_STATE_ACTIVE)
    {
        isEnabled = true;
    }

    return SendPropertyUpdate(
               header,
               SPINEL_CMD_PROP_VALUE_IS,
               key,
               SPINEL_DATATYPE_BOOL_S,
               isEnabled
           );
}
#endif

otError NcpBase::GetPropertyHandler_NET_REQUIRE_JOIN_EXISTING(uint8_t header, spinel_prop_key_t key)
{
    return SendPropertyUpdate(
               header,
               SPINEL_CMD_PROP_VALUE_IS,
               key,
               SPINEL_DATATYPE_BOOL_S,
               mRequireJoinExistingNetwork
           );
}

#if OPENTHREAD_ENABLE_LEGACY
otError NcpBase::GetPropertyHandler_NEST_LEGACY_ULA_PREFIX(uint8_t header, spinel_prop_key_t key)
{
    return SendPropertyUpdate(
               header,
               SPINEL_CMD_PROP_VALUE_IS,
               key,
               SPINEL_DATATYPE_DATA_S,
               mLegacyUlaPrefix,
               sizeof(mLegacyUlaPrefix)
           );
}
#endif // OPENTHREAD_ENABLE_LEGACY

// ----------------------------------------------------------------------------
// MARK: Individual Property Setters
// ----------------------------------------------------------------------------

otError NcpBase::SetPropertyHandler_POWER_STATE(uint8_t header, spinel_prop_key_t key, const uint8_t *value_ptr,
                                                uint16_t value_len)
{
    // TODO: Implement POWER_STATE
    (void)key;
    (void)value_ptr;
    (void)value_len;

    return SendLastStatus(header, SPINEL_STATUS_UNIMPLEMENTED);
}

otError NcpBase::SetPropertyHandler_HOST_POWER_STATE(uint8_t header, spinel_prop_key_t key, const uint8_t *value_ptr,
                                                     uint16_t value_len)
{
    uint8_t value;
    spinel_ssize_t parsedLength;
    otError errorCode = OT_ERROR_NONE;

    parsedLength = spinel_datatype_unpack(
                       value_ptr,
                       value_len,
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
        errorCode = OT_ERROR_PARSE;
    }

    if (errorCode == OT_ERROR_NONE)
    {
        mHostPowerStateHeader = 0;

        errorCode = HandleCommandPropertyGet(header, key);

        if (mHostPowerState != SPINEL_HOST_POWER_STATE_ONLINE)
        {
            if (errorCode == OT_ERROR_NONE)
            {
                mHostPowerReplyFrameTag = GetLastOutboundFrameTag();
            }
            else
            {
                mHostPowerReplyFrameTag = NcpFrameBuffer::kInvalidTag;
            }

            mHostPowerStateInProgress = true;
        }

        if (errorCode != OT_ERROR_NONE)
        {
            mHostPowerStateHeader = header;

            // The reply will be queued when buffer space becomes available
            // in the NCP tx buffer so we return `success` to avoid sending a
            // NOMEM status for the same tid through `mDroppedReplyTid` list.

            errorCode = OT_ERROR_NONE;
        }
    }
    else
    {
        errorCode = SendLastStatus(header, ThreadErrorToSpinelStatus(errorCode));
    }

    return errorCode;
}

#if OPENTHREAD_ENABLE_RAW_LINK_API

otError NcpBase::SetPropertyHandler_PHY_ENABLED(uint8_t header, spinel_prop_key_t key, const uint8_t *value_ptr,
                                                uint16_t value_len)
{
    bool value = false;
    spinel_ssize_t parsedLength;
    otError errorCode = OT_ERROR_NONE;

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
            // If we have raw stream enabled stop receiving
            if (mIsRawStreamEnabled)
            {
                otLinkRawSleep(mInstance);
            }

            errorCode = otLinkRawSetEnable(mInstance, false);
        }
        else
        {
            errorCode = otLinkRawSetEnable(mInstance, true);

            // If we have raw stream enabled already, start receiving
            if (errorCode == OT_ERROR_NONE && mIsRawStreamEnabled)
            {
                errorCode = otLinkRawReceive(mInstance, mCurReceiveChannel, &NcpBase::LinkRawReceiveDone);
            }
        }
    }
    else
    {
        errorCode = OT_ERROR_PARSE;
    }

    if (errorCode == OT_ERROR_NONE)
    {
        errorCode = HandleCommandPropertyGet(header, key);
    }
    else
    {
        errorCode = SendLastStatus(header, ThreadErrorToSpinelStatus(errorCode));
    }

    return errorCode;
}

#endif // OPENTHREAD_ENABLE_RAW_LINK_API

otError NcpBase::SetPropertyHandler_PHY_TX_POWER(uint8_t header, spinel_prop_key_t key, const uint8_t *value_ptr,
                                                 uint16_t value_len)
{
    int8_t value = 0;
    spinel_ssize_t parsedLength;
    otError errorCode = OT_ERROR_NONE;

    parsedLength = spinel_datatype_unpack(
                       value_ptr,
                       value_len,
                       SPINEL_DATATYPE_INT8_S,
                       &value
                   );

    if (parsedLength > 0)
    {
        otLinkSetMaxTransmitPower(mInstance, value);

        errorCode = HandleCommandPropertyGet(header, key);
    }
    else
    {
        errorCode = SendLastStatus(header, SPINEL_STATUS_PARSE_ERROR);
    }

    return errorCode;
}

otError NcpBase::SetPropertyHandler_PHY_CHAN(uint8_t header, spinel_prop_key_t key, const uint8_t *value_ptr,
                                             uint16_t value_len)
{
    unsigned int i = 0;
    spinel_ssize_t parsedLength;
    otError errorCode = OT_ERROR_NONE;

    parsedLength = spinel_datatype_unpack(
                       value_ptr,
                       value_len,
                       SPINEL_DATATYPE_UINT_PACKED_S,
                       &i
                   );

    if (parsedLength > 0)
    {
        errorCode = otLinkSetChannel(mInstance, static_cast<uint8_t>(i));

#if OPENTHREAD_ENABLE_RAW_LINK_API

        if (errorCode == OT_ERROR_NONE)
        {
            // Cache the channel. If the raw link layer isn't enabled yet, the otSetChannel call
            // doesn't call into the radio layer to set the channel. We will have to do it
            // manually whenever the radios are enabled and/or raw stream is enabled.
            mCurReceiveChannel = static_cast<uint8_t>(i);

            // Make sure we are update the receiving channel if raw link is enabled and we have raw
            // stream enabled already
            if (otLinkRawIsEnabled(mInstance) && mIsRawStreamEnabled)
            {
                errorCode = otLinkRawReceive(mInstance, mCurReceiveChannel, &NcpBase::LinkRawReceiveDone);
            }
        }

#endif // OPENTHREAD_ENABLE_RAW_LINK_API

        if (errorCode == OT_ERROR_NONE)
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

otError NcpBase::SetPropertyHandler_MAC_PROMISCUOUS_MODE(uint8_t header, spinel_prop_key_t key,
                                                         const uint8_t *value_ptr, uint16_t value_len)
{
    uint8_t i = 0;
    spinel_ssize_t parsedLength;
    otError errorCode = OT_ERROR_NONE;

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
        case SPINEL_MAC_PROMISCUOUS_MODE_OFF:
            otPlatRadioSetPromiscuous(mInstance, false);
            errorCode = OT_ERROR_NONE;
            break;

        case SPINEL_MAC_PROMISCUOUS_MODE_NETWORK:
        case SPINEL_MAC_PROMISCUOUS_MODE_FULL:
            otPlatRadioSetPromiscuous(mInstance, true);
            errorCode = OT_ERROR_NONE;
            break;

        default:
            errorCode = OT_ERROR_INVALID_ARGS;
            break;
        }

        if (errorCode == OT_ERROR_NONE)
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

otError NcpBase::SetPropertyHandler_MAC_SCAN_MASK(uint8_t header, spinel_prop_key_t key, const uint8_t *value_ptr,
                                                  uint16_t value_len)
{
    otError errorCode = OT_ERROR_NONE;
    uint32_t new_mask(0);

    for (; value_len != 0; value_len--, value_ptr++)
    {
        if ((value_ptr[0] > 31)
            || (mSupportedChannelMask & (1 << value_ptr[0])) == 0
           )
        {
            errorCode = OT_ERROR_INVALID_ARGS;
            break;
        }

        new_mask |= (1 << value_ptr[0]);
    }

    if (errorCode == OT_ERROR_NONE)
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

otError NcpBase::SetPropertyHandler_MAC_SCAN_PERIOD(uint8_t header, spinel_prop_key_t key, const uint8_t *value_ptr,
                                                    uint16_t value_len)
{
    uint16_t tmp(mScanPeriod);
    spinel_ssize_t parsedLength;
    otError errorCode = OT_ERROR_NONE;

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

otError NcpBase::SetPropertyHandler_NET_REQUIRE_JOIN_EXISTING(uint8_t header, spinel_prop_key_t key,
                                                              const uint8_t *value_ptr, uint16_t value_len)
{
    bool tmp(mRequireJoinExistingNetwork);
    spinel_ssize_t parsedLength;
    otError errorCode = OT_ERROR_NONE;

    parsedLength = spinel_datatype_unpack(
                       value_ptr,
                       value_len,
                       SPINEL_DATATYPE_BOOL_S,
                       &tmp
                   );

    if (parsedLength > 0)
    {
        mRequireJoinExistingNetwork = tmp;
        errorCode = HandleCommandPropertyGet(header, key);
    }
    else
    {
        errorCode = SendLastStatus(header, SPINEL_STATUS_PARSE_ERROR);
    }

    return errorCode;
}

bool HasOnly1BitSet(uint32_t aValue)
{
    return aValue != 0 && ((aValue & (aValue - 1)) == 0);
}

uint8_t IndexOfMSB(uint32_t aValue)
{
    uint8_t index = 0;

    while (aValue >>= 1)
    {
        index++;
    }

    return index;
}

otError NcpBase::SetPropertyHandler_MAC_SCAN_STATE(uint8_t header, spinel_prop_key_t key, const uint8_t *value_ptr,
                                                   uint16_t value_len)
{
    uint8_t state = 0;
    spinel_ssize_t parsedLength;
    otError errorCode = OT_ERROR_NONE;

    parsedLength = spinel_datatype_unpack(
                       value_ptr,
                       value_len,
                       SPINEL_DATATYPE_UINT8_S,
                       &state
                   );

    if (parsedLength > 0)
    {
        switch (state)
        {
        case SPINEL_SCAN_STATE_IDLE:
            errorCode = OT_ERROR_NONE;
            break;

        case SPINEL_SCAN_STATE_BEACON:
#if OPENTHREAD_ENABLE_RAW_LINK_API
            if (otLinkRawIsEnabled(mInstance))
            {
                errorCode = OT_ERROR_NOT_IMPLEMENTED;
            }
            else
#endif // OPENTHREAD_ENABLE_RAW_LINK_API
            {
                errorCode = otLinkActiveScan(
                                mInstance,
                                mChannelMask,
                                mScanPeriod,
                                &HandleActiveScanResult_Jump,
                                this
                            );
            }

            if (errorCode == OT_ERROR_NONE)
            {
                mShouldSignalEndOfScan = false;
            }

            break;

        case SPINEL_SCAN_STATE_ENERGY:
#if OPENTHREAD_ENABLE_RAW_LINK_API
            if (otLinkRawIsEnabled(mInstance))
            {
                // Make sure we aren't already scanning and that we have
                // only 1 bit set for the channel mask.
                if (mCurScanChannel == NCP_INVALID_SCAN_CHANNEL)
                {
                    if (HasOnly1BitSet(mChannelMask))
                    {
                        uint8_t scanChannel = IndexOfMSB(mChannelMask);
                        mCurScanChannel = (int8_t)scanChannel;

                        errorCode = otLinkRawEnergyScan(
                                        mInstance,
                                        scanChannel,
                                        mScanPeriod,
                                        LinkRawEnergyScanDone
                                    );
                    }
                    else
                    {
                        errorCode = OT_ERROR_INVALID_ARGS;
                    }
                }
                else
                {
                    errorCode = OT_ERROR_INVALID_STATE;
                }
            }
            else
#endif // OPENTHREAD_ENABLE_RAW_LINK_API
            {
                errorCode = otLinkEnergyScan(
                                mInstance,
                                mChannelMask,
                                mScanPeriod,
                                &HandleEnergyScanResult_Jump,
                                this
                            );
            }

            if (errorCode == OT_ERROR_NONE)
            {
                mShouldSignalEndOfScan = false;
            }

            break;

        case SPINEL_SCAN_STATE_DISCOVER:

            errorCode = otThreadDiscover(
                            mInstance,
                            mChannelMask,
                            mDiscoveryScanPanId,
                            mDiscoveryScanJoinerFlag,
                            mDiscoveryScanEnableFiltering,
                            &HandleActiveScanResult_Jump,
                            this
                        );

            if (errorCode == OT_ERROR_NONE)
            {
                mShouldSignalEndOfScan = false;
            }

            break;

        default:
            errorCode = OT_ERROR_INVALID_ARGS;
            break;
        }

        if (errorCode == OT_ERROR_NONE)
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

otError NcpBase::SetPropertyHandler_MAC_15_4_PANID(uint8_t header, spinel_prop_key_t key, const uint8_t *value_ptr,
                                                   uint16_t value_len)
{
    uint16_t tmp;
    spinel_ssize_t parsedLength;
    otError errorCode = OT_ERROR_NONE;

    parsedLength = spinel_datatype_unpack(
                       value_ptr,
                       value_len,
                       SPINEL_DATATYPE_UINT16_S,
                       &tmp
                   );

    if (parsedLength > 0)
    {
        errorCode = otLinkSetPanId(mInstance, tmp);

        if (errorCode == OT_ERROR_NONE)
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

otError NcpBase::SetPropertyHandler_MAC_15_4_LADDR(uint8_t header, spinel_prop_key_t key, const uint8_t *value_ptr,
                                                   uint16_t value_len)
{
    otExtAddress *tmp;
    spinel_ssize_t parsedLength;
    otError errorCode = OT_ERROR_NONE;

    parsedLength = spinel_datatype_unpack(
                       value_ptr,
                       value_len,
                       SPINEL_DATATYPE_EUI64_S,
                       &tmp
                   );

    if (parsedLength > 0)
    {
        errorCode = otLinkSetExtendedAddress(mInstance, tmp);

        if (errorCode == OT_ERROR_NONE)
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

otError NcpBase::SetPropertyHandler_MAC_RAW_STREAM_ENABLED(uint8_t header, spinel_prop_key_t key,
                                                           const uint8_t *value_ptr, uint16_t value_len)
{
    bool value = false;
    spinel_ssize_t parsedLength;
    otError errorCode = OT_ERROR_NONE;

    parsedLength = spinel_datatype_unpack(
                       value_ptr,
                       value_len,
                       SPINEL_DATATYPE_BOOL_S,
                       &value
                   );

    if (parsedLength > 0)
    {
#if OPENTHREAD_ENABLE_RAW_LINK_API

        if (otLinkRawIsEnabled(mInstance))
        {
            if (value)
            {
                errorCode = otLinkRawReceive(mInstance, mCurReceiveChannel, &NcpBase::LinkRawReceiveDone);
            }
            else
            {
                errorCode = otLinkRawSleep(mInstance);
            }
        }

#endif // OPENTHREAD_ENABLE_RAW_LINK_API
    }
    else
    {
        errorCode = OT_ERROR_PARSE;
    }

    if (errorCode == OT_ERROR_NONE)
    {
        mIsRawStreamEnabled = value;
        errorCode = HandleCommandPropertyGet(header, key);
    }
    else
    {
        errorCode = SendLastStatus(header, ThreadErrorToSpinelStatus(errorCode));
    }

    return errorCode;
}

#if OPENTHREAD_ENABLE_RAW_LINK_API

otError NcpBase::SetPropertyHandler_MAC_15_4_SADDR(uint8_t header, spinel_prop_key_t key, const uint8_t *value_ptr,
                                                   uint16_t value_len)
{
    uint16_t tmp;
    spinel_ssize_t parsedLength;
    otError errorCode = OT_ERROR_NONE;

    parsedLength = spinel_datatype_unpack(
                       value_ptr,
                       value_len,
                       SPINEL_DATATYPE_UINT16_S,
                       &tmp
                   );

    if (parsedLength > 0)
    {
        errorCode = otLinkRawSetShortAddress(mInstance, tmp);

        if (errorCode == OT_ERROR_NONE)
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

otError NcpBase::SetPropertyHandler_STREAM_RAW(uint8_t header, spinel_prop_key_t key, const uint8_t *value_ptr,
                                               uint16_t value_len)
{
    otError errorCode = OT_ERROR_NONE;

    if (otLinkRawIsEnabled(mInstance))
    {
        spinel_ssize_t parsedLength(0);
        uint8_t *frame_buffer(NULL);
        unsigned int frame_len(0);

        otRadioFrame *frame = otLinkRawGetTransmitBuffer(mInstance);

        parsedLength = spinel_datatype_unpack(
                           value_ptr,
                           value_len,
                           SPINEL_DATATYPE_DATA_WLEN_S
                           SPINEL_DATATYPE_UINT8_S
                           SPINEL_DATATYPE_INT8_S,
                           &frame_buffer,
                           &frame_len,
                           &frame->mChannel,
                           &frame->mPower
                       );

        if (parsedLength > 0 && frame_len <= OT_RADIO_FRAME_MAX_SIZE)
        {
            // Cache the transaction ID for async response
            mCurTransmitTID = SPINEL_HEADER_GET_TID(header);

            // Update frame buffer and length
            frame->mLength = static_cast<uint8_t>(frame_len);
            memcpy(frame->mPsdu, frame_buffer, frame->mLength);

            // TODO: This should be later added in the STREAM_RAW argument to allow user to directly specify it.
            frame->mMaxTxAttempts = OPENTHREAD_CONFIG_MAX_TX_ATTEMPTS_DIRECT;

            // Pass frame to the radio layer. Note, this fails if we
            // haven't enabled raw stream or are already transmitting.
            errorCode = otLinkRawTransmit(mInstance, frame, &NcpBase::LinkRawTransmitDone);
        }
        else
        {
            errorCode = OT_ERROR_PARSE;
        }
    }
    else
    {
        errorCode = OT_ERROR_INVALID_STATE;
    }

    if (errorCode == OT_ERROR_NONE)
    {
        // Don't do anything here yet. We will complete the transaction when we get a transmit done callback
    }
    else
    {
        errorCode = SendLastStatus(header, ThreadErrorToSpinelStatus(errorCode));
    }

    (void)key;

    return errorCode;
}

#endif // OPENTHREAD_ENABLE_RAW_LINK_API

otError NcpBase::SetPropertyHandler_NET_IF_UP(uint8_t header, spinel_prop_key_t key, const uint8_t *value_ptr,
                                              uint16_t value_len)
{
    bool value = false;
    spinel_ssize_t parsedLength;
    otError errorCode = OT_ERROR_NONE;

    parsedLength = spinel_datatype_unpack(
                       value_ptr,
                       value_len,
                       SPINEL_DATATYPE_BOOL_S,
                       &value
                   );

    if (parsedLength > 0)
    {
        errorCode = otIp6SetEnabled(mInstance, value);
    }
    else
    {
        errorCode = OT_ERROR_PARSE;
    }

    if (errorCode == OT_ERROR_NONE)
    {
        errorCode = HandleCommandPropertyGet(header, key);
    }
    else
    {
        errorCode = SendLastStatus(header, ThreadErrorToSpinelStatus(errorCode));
    }

    return errorCode;
}

otError NcpBase::SetPropertyHandler_NET_STACK_UP(uint8_t header, spinel_prop_key_t key, const uint8_t *value_ptr,
                                                 uint16_t value_len)
{
    bool value = false;
    spinel_ssize_t parsedLength;
    otError errorCode = OT_ERROR_NONE;

    parsedLength = spinel_datatype_unpack(
                       value_ptr,
                       value_len,
                       SPINEL_DATATYPE_BOOL_S,
                       &value
                   );

    if (parsedLength > 0)
    {
        // If the value has changed...
        if ((value != false) != (otThreadGetDeviceRole(mInstance) != OT_DEVICE_ROLE_DISABLED))
        {
            if (value != false)
            {
                errorCode = otThreadSetEnabled(mInstance, true);

#if OPENTHREAD_ENABLE_LEGACY
                mLegacyNodeDidJoin = false;

                if (mLegacyHandlers != NULL)
                {
                    if (mLegacyHandlers->mStartLegacy)
                    {
                        mLegacyHandlers->mStartLegacy();
                    }
                }

#endif // OPENTHREAD_ENABLE_LEGACY
            }
            else
            {
                errorCode = otThreadSetEnabled(mInstance, false);

#if OPENTHREAD_ENABLE_LEGACY
                mLegacyNodeDidJoin = false;

                if (mLegacyHandlers != NULL)
                {
                    if (mLegacyHandlers->mStopLegacy)
                    {
                        mLegacyHandlers->mStopLegacy();
                    }
                }

#endif // OPENTHREAD_ENABLE_LEGACY
            }
        }
    }
    else
    {
        errorCode = OT_ERROR_PARSE;
    }

    if (errorCode == OT_ERROR_NONE)
    {
        errorCode = HandleCommandPropertyGet(header, key);
    }
    else
    {
        errorCode = SendLastStatus(header, ThreadErrorToSpinelStatus(errorCode));
    }

    return errorCode;
}

otError NcpBase::SetPropertyHandler_NET_ROLE(uint8_t header, spinel_prop_key_t key, const uint8_t *value_ptr,
                                             uint16_t value_len)
{
    unsigned int i(0);
    spinel_ssize_t parsedLength;
    otError errorCode = OT_ERROR_NONE;

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
            errorCode = otThreadBecomeDetached(mInstance);
            break;

#if OPENTHREAD_FTD

        case SPINEL_NET_ROLE_ROUTER:
            errorCode = otThreadBecomeRouter(mInstance);
            break;

        case SPINEL_NET_ROLE_LEADER:
            errorCode = otThreadBecomeLeader(mInstance);
            break;
#endif  // OPENTHREAD_FTD

        case SPINEL_NET_ROLE_CHILD:
            errorCode = otThreadBecomeChild(mInstance);
            break;
        }

        if (errorCode == OT_ERROR_NONE)
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

otError NcpBase::SetPropertyHandler_NET_NETWORK_NAME(uint8_t header, spinel_prop_key_t key, const uint8_t *value_ptr,
                                                     uint16_t value_len)
{
    const char *string(NULL);
    spinel_ssize_t parsedLength;
    otError errorCode = OT_ERROR_NONE;

    parsedLength = spinel_datatype_unpack(
                       value_ptr,
                       value_len,
                       SPINEL_DATATYPE_UTF8_S,
                       &string
                   );

    if ((parsedLength > 0) && (string != NULL))
    {
        errorCode = otThreadSetNetworkName(mInstance, string);

        if (errorCode == OT_ERROR_NONE)
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

otError NcpBase::SetPropertyHandler_NET_XPANID(uint8_t header, spinel_prop_key_t key, const uint8_t *value_ptr,
                                               uint16_t value_len)
{
    const uint8_t *ptr = NULL;
    spinel_size_t len;
    spinel_ssize_t parsedLength;
    otError errorCode = OT_ERROR_NONE;

    parsedLength = spinel_datatype_unpack(
                       value_ptr,
                       value_len,
                       SPINEL_DATATYPE_DATA_S,
                       &ptr,
                       &len
                   );

    if ((parsedLength > 0) && (len == sizeof(spinel_net_xpanid_t)))
    {
        otThreadSetExtendedPanId(mInstance, ptr);
        errorCode = HandleCommandPropertyGet(header, key);
    }
    else
    {
        errorCode = SendLastStatus(header, SPINEL_STATUS_PARSE_ERROR);
    }

    return errorCode;
}

otError NcpBase::SetPropertyHandler_NET_MASTER_KEY(uint8_t header, spinel_prop_key_t key, const uint8_t *value_ptr,
                                                   uint16_t value_len)
{
    const uint8_t *ptr = NULL;
    spinel_size_t len;
    spinel_ssize_t parsedLength;
    otError errorCode = OT_ERROR_NONE;

    parsedLength = spinel_datatype_unpack(
                       value_ptr,
                       value_len,
                       SPINEL_DATATYPE_DATA_S,
                       &ptr,
                       &len
                   );

    if ((parsedLength > 0) && (len == OT_MASTER_KEY_SIZE))
    {
        errorCode = otThreadSetMasterKey(mInstance, reinterpret_cast<const otMasterKey *>(ptr));

        if (errorCode == OT_ERROR_NONE)
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

otError NcpBase::SetPropertyHandler_NET_KEY_SEQUENCE_COUNTER(uint8_t header, spinel_prop_key_t key,
                                                             const uint8_t *value_ptr, uint16_t value_len)
{
    unsigned int i(0);
    spinel_ssize_t parsedLength;
    otError errorCode = OT_ERROR_NONE;

    parsedLength = spinel_datatype_unpack(
                       value_ptr,
                       value_len,
                       SPINEL_DATATYPE_UINT32_S,
                       &i
                   );

    if (parsedLength > 0)
    {
        otThreadSetKeySequenceCounter(mInstance, i);
        errorCode = HandleCommandPropertyGet(header, key);
    }
    else
    {
        errorCode = SendLastStatus(header, SPINEL_STATUS_PARSE_ERROR);
    }

    return errorCode;
}

otError NcpBase::SetPropertyHandler_NET_KEY_SWITCH_GUARDTIME(uint8_t header, spinel_prop_key_t key,
                                                             const uint8_t *value_ptr, uint16_t value_len)
{
    unsigned int i(0);
    spinel_ssize_t parsedLength;
    otError errorCode = OT_ERROR_NONE;

    parsedLength = spinel_datatype_unpack(
                       value_ptr,
                       value_len,
                       SPINEL_DATATYPE_UINT32_S,
                       &i
                   );

    if (parsedLength > 0)
    {
        otThreadSetKeySwitchGuardTime(mInstance, i);
        errorCode = HandleCommandPropertyGet(header, key);
    }
    else
    {
        errorCode = SendLastStatus(header, SPINEL_STATUS_PARSE_ERROR);
    }

    return errorCode;
}

#if OPENTHREAD_FTD
otError NcpBase::SetPropertyHandler_THREAD_LOCAL_LEADER_WEIGHT(uint8_t header, spinel_prop_key_t key,
                                                               const uint8_t *value_ptr, uint16_t value_len)
{
    otError errorCode = OT_ERROR_NONE;
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
        otThreadSetLocalLeaderWeight(mInstance, value);
    }
    else
    {
        errorCode = OT_ERROR_PARSE;
    }

    if (errorCode == OT_ERROR_NONE)
    {
        errorCode = HandleCommandPropertyGet(header, key);
    }
    else
    {
        errorCode = SendLastStatus(header, ThreadErrorToSpinelStatus(errorCode));
    }

    return errorCode;
}
#endif  // OPENTHREAD_FTD

otError NcpBase::SetPropertyHandler_STREAM_NET_INSECURE(uint8_t header, spinel_prop_key_t key,
                                                        const uint8_t *value_ptr, uint16_t value_len)
{
    spinel_ssize_t parsedLength;
    otError errorCode = OT_ERROR_NONE;
    const uint8_t *frame_ptr(NULL);
    unsigned int frame_len(0);
    const uint8_t *meta_ptr(NULL);
    unsigned int meta_len(0);

    // STREAM_NET_INSECURE packets are not secured at layer 2.
    otMessage *message = otIp6NewMessage(mInstance, false);

    if (message == NULL)
    {
        errorCode = OT_ERROR_NO_BUFS;
    }
    else
    {
        parsedLength = spinel_datatype_unpack(
                           value_ptr,
                           value_len,
                           (
                               SPINEL_DATATYPE_DATA_WLEN_S   // Frame data
                               SPINEL_DATATYPE_DATA_S        // Meta data
                           ),
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

        errorCode = otMessageAppend(message, frame_ptr, static_cast<uint16_t>(frame_len));
    }

    if (errorCode == OT_ERROR_NONE)
    {
        // Ensure the insecure message is forwarded using direct transmission.
        otMessageSetDirectTransmission(message, true);

        errorCode = otIp6Send(mInstance, message);
    }
    else if (message)
    {
        otMessageFree(message);
    }

    if (errorCode == OT_ERROR_NONE)
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

#if OPENTHREAD_ENABLE_TMF_PROXY && OPENTHREAD_FTD
otError NcpBase::SetPropertyHandler_THREAD_TMF_PROXY_STREAM(uint8_t header, spinel_prop_key_t key,
                                                           const uint8_t *value_ptr, uint16_t value_len)
{
    spinel_ssize_t parsedLength;
    otError errorCode = OT_ERROR_NONE;
    const uint8_t *frame_ptr(NULL);
    unsigned int frame_len(0);
    uint16_t locator;
    uint16_t port;

    // THREAD_TMF_PROXY_STREAM requires layer 2 security.
    otMessage *message = otIp6NewMessage(mInstance, true);

    if (message == NULL)
    {
        errorCode = OT_ERROR_NO_BUFS;
    }
    else
    {
        parsedLength = spinel_datatype_unpack(
                           value_ptr,
                           value_len,
                           (
                               SPINEL_DATATYPE_DATA_WLEN_S  // Frame data
                               SPINEL_DATATYPE_UINT16_S     // Locator
                               SPINEL_DATATYPE_UINT16_S     // Port
                           ),
                           &frame_ptr,
                           &frame_len,
                           &locator,
                           &port
                       );

        if (parsedLength > 0)
        {
            errorCode = otMessageAppend(message, frame_ptr, static_cast<uint16_t>(frame_len));
        }
        else
        {
            errorCode = OT_ERROR_PARSE;
        }
    }

    if (errorCode == OT_ERROR_NONE)
    {
        errorCode = otTmfProxySend(mInstance, message, locator, port);
    }
    else if (message)
    {
        otMessageFree(message);
    }

    if (errorCode == OT_ERROR_NONE)
    {
        if (SPINEL_HEADER_GET_TID(header) != 0)
        {
            // Only send a successful status update if
            // there was a transaction id in the header.
            errorCode = SendLastStatus(header, SPINEL_STATUS_OK);
        }
    }
    else
    {
        errorCode = SendLastStatus(header, ThreadErrorToSpinelStatus(errorCode));
    }

    (void)key;

    return errorCode;
}
#endif // OPENTHREAD_ENABLE_TMF_PROXY && OPENTHREAD_FTD

otError NcpBase::SetPropertyHandler_STREAM_NET(uint8_t header, spinel_prop_key_t key, const uint8_t *value_ptr,
                                               uint16_t value_len)
{
    spinel_ssize_t parsedLength;
    otError errorCode = OT_ERROR_NONE;
    const uint8_t *frame_ptr(NULL);
    unsigned int frame_len(0);
    const uint8_t *meta_ptr(NULL);
    unsigned int meta_len(0);

    // STREAM_NET requires layer 2 security.
    otMessage *message = otIp6NewMessage(mInstance, true);

    if (message == NULL)
    {
        errorCode = OT_ERROR_NO_BUFS;
    }
    else
    {
        parsedLength = spinel_datatype_unpack(
                           value_ptr,
                           value_len,
                           (
                               SPINEL_DATATYPE_DATA_WLEN_S  // Frame data
                               SPINEL_DATATYPE_DATA_S       // Meta data
                           ),
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

        errorCode = otMessageAppend(message, frame_ptr, static_cast<uint16_t>(frame_len));
    }

    if (errorCode == OT_ERROR_NONE)
    {
        errorCode = otIp6Send(mInstance, message);
    }
    else if (message)
    {
        otMessageFree(message);
    }

    if (errorCode == OT_ERROR_NONE)
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

otError NcpBase::SetPropertyHandler_IPV6_ML_PREFIX(uint8_t header, spinel_prop_key_t key, const uint8_t *value_ptr,
                                                   uint16_t value_len)
{
    otError errorCode = OT_ERROR_NONE;

    if (value_len >= 8)
    {
        errorCode = otThreadSetMeshLocalPrefix(mInstance, value_ptr);
        HandleCommandPropertyGet(header, key);
    }
    else
    {
        errorCode = OT_ERROR_PARSE;
    }

    if (errorCode == OT_ERROR_NONE)
    {
        errorCode = HandleCommandPropertyGet(header, key);
    }
    else
    {
        errorCode = SendLastStatus(header, ThreadErrorToSpinelStatus(errorCode));
    }

    return errorCode;
}

otError NcpBase::SetPropertyHandler_IPV6_ICMP_PING_OFFLOAD(uint8_t header, spinel_prop_key_t key,
                                                           const uint8_t *value_ptr, uint16_t value_len)
{
    bool isEnabled(false);
    spinel_ssize_t parsedLength;
    otError errorCode = OT_ERROR_NONE;

    parsedLength = spinel_datatype_unpack(
                       value_ptr,
                       value_len,
                       SPINEL_DATATYPE_BOOL_S,
                       &isEnabled
                   );

    if (parsedLength > 0)
    {
        otIcmp6SetEchoEnabled(mInstance, isEnabled);

        errorCode = HandleCommandPropertyGet(header, key);
    }
    else
    {
        errorCode = SendLastStatus(header, SPINEL_STATUS_PARSE_ERROR);
    }

    return errorCode;
}

otError NcpBase::SetPropertyHandler_THREAD_RLOC16_DEBUG_PASSTHRU(uint8_t header, spinel_prop_key_t key,
                                                                 const uint8_t *value_ptr, uint16_t value_len)
{
    bool isEnabled(false);
    spinel_ssize_t parsedLength;
    otError errorCode = OT_ERROR_NONE;

    parsedLength = spinel_datatype_unpack(
                       value_ptr,
                       value_len,
                       SPINEL_DATATYPE_BOOL_S,
                       &isEnabled
                   );

    if (parsedLength > 0)
    {
        // Note reverse logic: passthru enabled = filter disabled
        otIp6SetReceiveFilterEnabled(mInstance, !isEnabled);

        errorCode = HandleCommandPropertyGet(header, key);
    }
    else
    {
        errorCode = SendLastStatus(header, SPINEL_STATUS_PARSE_ERROR);
    }

    return errorCode;
}

otError NcpBase::SetPropertyHandler_THREAD_DISCOVERY_SCAN_JOINER_FLAG(uint8_t header, spinel_prop_key_t key,
                                                                      const uint8_t *value_ptr, uint16_t value_len)
{
    bool joinerFlag = false;
    spinel_ssize_t parsedLength;
    otError errorCode = OT_ERROR_NONE;

    parsedLength = spinel_datatype_unpack(
                       value_ptr,
                       value_len,
                       SPINEL_DATATYPE_BOOL_S,
                       &joinerFlag
                   );

    if (parsedLength > 0)
    {
        mDiscoveryScanJoinerFlag = joinerFlag;

        errorCode = HandleCommandPropertyGet(header, key);
    }
    else
    {
        errorCode = SendLastStatus(header, SPINEL_STATUS_PARSE_ERROR);
    }

    return errorCode;
}

otError NcpBase::SetPropertyHandler_THREAD_DISCOVERY_SCAN_ENABLE_FILTERING(uint8_t header, spinel_prop_key_t key,
                                                                           const uint8_t *value_ptr,
                                                                           uint16_t value_len)
{
    bool isEnabled = false;
    spinel_ssize_t parsedLength;
    otError errorCode = OT_ERROR_NONE;

    parsedLength = spinel_datatype_unpack(
                       value_ptr,
                       value_len,
                       SPINEL_DATATYPE_BOOL_S,
                       &isEnabled
                   );

    if (parsedLength > 0)
    {
        mDiscoveryScanEnableFiltering = isEnabled;

        errorCode = HandleCommandPropertyGet(header, key);
    }
    else
    {
        errorCode = SendLastStatus(header, SPINEL_STATUS_PARSE_ERROR);
    }

    return errorCode;
}

otError NcpBase::SetPropertyHandler_THREAD_DISCOVERY_SCAN_PANID(uint8_t header, spinel_prop_key_t key,
                                                                const uint8_t *value_ptr, uint16_t value_len)
{
    uint16_t panid;
    spinel_ssize_t parsedLength;
    otError errorCode = OT_ERROR_NONE;

    parsedLength = spinel_datatype_unpack(
                       value_ptr,
                       value_len,
                       SPINEL_DATATYPE_UINT16_S,
                       &panid
                   );

    if (parsedLength > 0)
    {
        mDiscoveryScanPanId = panid;

        errorCode = HandleCommandPropertyGet(header, key);
    }
    else
    {
        errorCode = SendLastStatus(header, SPINEL_STATUS_PARSE_ERROR);
    }

    return errorCode;
}

otError NcpBase::SetPropertyHandler_THREAD_ASSISTING_PORTS(uint8_t header, spinel_prop_key_t key,
                                                           const uint8_t *value_ptr, uint16_t value_len)
{
    otError errorCode = OT_ERROR_NONE;
    uint8_t num_entries = 0;
    const uint16_t *ports = otIp6GetUnsecurePorts(mInstance, &num_entries);
    spinel_ssize_t parsedLength = 1;
    int ports_changed = 0;

    // First, we need to remove all of the current assisting ports.
    for (; num_entries != 0; ports++, num_entries--)
    {
        errorCode = otIp6RemoveUnsecurePort(mInstance, *ports);

        if (errorCode != OT_ERROR_NONE)
        {
            break;
        }

        ports_changed++;
    }

    while ((errorCode == OT_ERROR_NONE) && (parsedLength > 0) && (value_len >= 2))
    {
        uint16_t port;

        parsedLength = spinel_datatype_unpack(
                           value_ptr,
                           value_len,
                           SPINEL_DATATYPE_UINT16_S,
                           &port
                       );

        if (parsedLength > 0)
        {
            errorCode = otIp6AddUnsecurePort(mInstance, port);
        }
        else
        {
            errorCode = OT_ERROR_PARSE;
        }

        if (errorCode != OT_ERROR_NONE)
        {
            break;
        }

        value_ptr += parsedLength;
        value_len -= parsedLength;

        ports_changed++;
    }

    if (errorCode == OT_ERROR_NONE)
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

#if OPENTHREAD_ENABLE_BORDER_ROUTER
otError NcpBase::SetPropertyHandler_THREAD_ALLOW_LOCAL_NET_DATA_CHANGE(uint8_t header, spinel_prop_key_t key,
                                                                       const uint8_t *value_ptr, uint16_t value_len)
{
    bool value = false;
    spinel_ssize_t parsedLength;
    otError errorCode = OT_ERROR_NONE;
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
        errorCode = OT_ERROR_PARSE;
    }

    if (errorCode == OT_ERROR_NONE)
    {
        errorCode = HandleCommandPropertyGet(header, key);
    }
    else
    {
        errorCode = SendLastStatus(header, ThreadErrorToSpinelStatus(errorCode));
    }

    if (should_register_with_leader)
    {
        otBorderRouterRegister(mInstance);
    }

    return errorCode;
}
#endif // OPENTHREAD_ENABLE_BORDER_ROUTER

#if OPENTHREAD_FTD
otError NcpBase::SetPropertyHandler_THREAD_ROUTER_ROLE_ENABLED(uint8_t header, spinel_prop_key_t key,
                                                               const uint8_t *value_ptr, uint16_t value_len)
{
    bool isEnabled;
    spinel_ssize_t parsedLength;
    otError errorCode = OT_ERROR_NONE;

    parsedLength = spinel_datatype_unpack(
                       value_ptr,
                       value_len,
                       SPINEL_DATATYPE_BOOL_S,
                       &isEnabled
                   );

    if (parsedLength > 0)
    {
        otThreadSetRouterRoleEnabled(mInstance, isEnabled);

        errorCode = HandleCommandPropertyGet(header, key);
    }
    else
    {
        errorCode = SendLastStatus(header, SPINEL_STATUS_PARSE_ERROR);
    }

    return errorCode;
}

#if OPENTHREAD_CONFIG_ENABLE_STEERING_DATA_SET_OOB

otError NcpBase::SetPropertyHandler_THREAD_THREAD_STEERING_DATA(uint8_t header, spinel_prop_key_t key,
                                                                const uint8_t *value_ptr, uint16_t value_len)
{
    otExtAddress *extAddress;
    spinel_ssize_t parsedLength;
    otError errorCode = OT_ERROR_NONE;

    parsedLength = spinel_datatype_unpack(
                       value_ptr,
                       value_len,
                       SPINEL_DATATYPE_EUI64_S,
                       &extAddress
                   );

    if (parsedLength > 0)
    {
        errorCode = otThreadSetSteeringData(mInstance, extAddress);

        if (errorCode == OT_ERROR_NONE)
        {
            errorCode = SendPropertyUpdate(
                            header,
                            SPINEL_CMD_PROP_VALUE_IS,
                            key,
                            SPINEL_DATATYPE_EUI64_S,
                            extAddress->m8
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

#endif // #if OPENTHREAD_CONFIG_ENABLE_STEERING_DATA_SET_OOB

#endif  // #if OPENTHREAD_FTD

otError NcpBase::SetPropertyHandler_CNTR_RESET(uint8_t header, spinel_prop_key_t key, const uint8_t *value_ptr,
                                               uint16_t value_len)
{
    otError errorCode = OT_ERROR_NONE;
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
            errorCode = OT_ERROR_NOT_IMPLEMENTED;
        }
        else
        {
            errorCode = OT_ERROR_INVALID_ARGS;
        }
    }
    else
    {
        errorCode = OT_ERROR_PARSE;
    }

    (void)key;

    // There is currently no getter for PROP_CNTR_RESET, so we just
    // return SPINEL_STATUS_OK for success when the counters are reset.

    return SendLastStatus(header, ThreadErrorToSpinelStatus(errorCode));
}

#if OPENTHREAD_ENABLE_COMMISSIONER && OPENTHREAD_FTD
otError NcpBase::SetPropertyHandler_THREAD_COMMISSIONER_ENABLED(uint8_t header, spinel_prop_key_t key,
                                                                const uint8_t *value_ptr,
                                                                uint16_t value_len)
{
    bool value = false;
    spinel_ssize_t parsedLength;
    otError errorCode = OT_ERROR_NONE;
    (void)key;

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
            errorCode = otCommissionerStop(mInstance);
        }
        else
        {
            errorCode = otCommissionerStart(mInstance);
        }
    }
    else
    {
        errorCode = OT_ERROR_PARSE;
    }

    return SendLastStatus(header, ThreadErrorToSpinelStatus(errorCode));
}
#endif // OPENTHREAD_ENABLE_COMMISSIONER && OPENTHREAD_FTD

#if OPENTHREAD_ENABLE_MAC_WHITELIST

otError NcpBase::SetPropertyHandler_MAC_WHITELIST(uint8_t header, spinel_prop_key_t key, const uint8_t *value_ptr,
                                                  uint16_t value_len)
{
    otError errorCode = OT_ERROR_NONE;
    spinel_ssize_t parsedLength = 1;

    // First, clear the whitelist.
    otLinkClearWhitelist(mInstance);

    while ((errorCode == OT_ERROR_NONE)
           && (parsedLength > 0)
           && (value_len > 0)
          )
    {
        otExtAddress *ext_addr = NULL;
        int8_t rssi = RSSI_OVERRIDE_DISABLED;

        parsedLength = spinel_datatype_unpack(
                           value_ptr,
                           value_len,
                           SPINEL_DATATYPE_STRUCT_S(
                               SPINEL_DATATYPE_EUI64_S
                               SPINEL_DATATYPE_INT8_S
                           ),
                           &ext_addr,
                           &rssi
                       );

        if (parsedLength <= 0)
        {
            rssi = RSSI_OVERRIDE_DISABLED;
            parsedLength = spinel_datatype_unpack(
                               value_ptr,
                               value_len,
                               SPINEL_DATATYPE_STRUCT_S(
                                   SPINEL_DATATYPE_EUI64_S
                               ),
                               &ext_addr
                           );
        }

        if (parsedLength <= 0)
        {
            errorCode = OT_ERROR_PARSE;
            break;
        }

        if (rssi == RSSI_OVERRIDE_DISABLED)
        {
            errorCode = otLinkAddWhitelist(mInstance, ext_addr->m8);
        }
        else
        {
            errorCode = otLinkAddWhitelistRssi(mInstance, ext_addr->m8, rssi);
        }

        value_ptr += parsedLength;
        value_len -= parsedLength;
    }

    if (errorCode == OT_ERROR_NONE)
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

otError NcpBase::SetPropertyHandler_MAC_WHITELIST_ENABLED(uint8_t header, spinel_prop_key_t key,
                                                          const uint8_t *value_ptr, uint16_t value_len)
{
    bool isEnabled;
    spinel_ssize_t parsedLength;
    otError errorCode = OT_ERROR_NONE;

    parsedLength = spinel_datatype_unpack(
                       value_ptr,
                       value_len,
                       SPINEL_DATATYPE_BOOL_S,
                       &isEnabled
                   );

    if (parsedLength > 0)
    {
        otLinkSetWhitelistEnabled(mInstance, isEnabled);
        errorCode = HandleCommandPropertyGet(header, key);
    }
    else
    {
        errorCode = SendLastStatus(header, SPINEL_STATUS_PARSE_ERROR);
    }

    return errorCode;
}

otError NcpBase::SetPropertyHandler_MAC_BLACKLIST(uint8_t header, spinel_prop_key_t key, const uint8_t *value_ptr,
                                                  uint16_t value_len)
{
    otError errorCode = OT_ERROR_NONE;
    spinel_ssize_t parsedLength = 1;

    // First, clear the blacklist.
    otLinkClearBlacklist(mInstance);

    while ((errorCode == OT_ERROR_NONE)
           && (parsedLength > 0)
           && (value_len > 0)
          )
    {
        otExtAddress *ext_addr = NULL;

        parsedLength = spinel_datatype_unpack(
                           value_ptr,
                           value_len,
                           SPINEL_DATATYPE_STRUCT_S(
                               SPINEL_DATATYPE_EUI64_S
                           ),
                           &ext_addr
                       );

        if (parsedLength <= 0)
        {
            errorCode = OT_ERROR_PARSE;
            break;
        }

        errorCode = otLinkAddBlacklist(mInstance, ext_addr->m8);

        value_ptr += parsedLength;
        value_len -= parsedLength;
    }

    if (errorCode == OT_ERROR_NONE)
    {
        errorCode = HandleCommandPropertyGet(header, key);
    }
    else
    {
        errorCode = SendLastStatus(header, ThreadErrorToSpinelStatus(errorCode));

        // We had an error, but we may have actually changed
        // the state of the blacklist---so we need to report
        // those incomplete changes via an asynchronous
        // change event.
        HandleCommandPropertyGet(SPINEL_HEADER_FLAG | SPINEL_HEADER_IID_0, key);
    }

    return errorCode;
}

otError NcpBase::SetPropertyHandler_MAC_BLACKLIST_ENABLED(uint8_t header, spinel_prop_key_t key,
                                                          const uint8_t *value_ptr, uint16_t value_len)
{
    bool isEnabled;
    spinel_ssize_t parsedLength;
    otError errorCode = OT_ERROR_NONE;

    parsedLength = spinel_datatype_unpack(
                       value_ptr,
                       value_len,
                       SPINEL_DATATYPE_BOOL_S,
                       &isEnabled
                   );

    if (parsedLength > 0)
    {
        otLinkSetBlacklistEnabled(mInstance, isEnabled);
        errorCode = HandleCommandPropertyGet(header, key);
    }
    else
    {
        errorCode = SendLastStatus(header, SPINEL_STATUS_PARSE_ERROR);
    }

    return errorCode;
}

#endif // OPENTHREAD_ENABLE_MAC_WHITELIST

#if OPENTHREAD_ENABLE_RAW_LINK_API

otError NcpBase::SetPropertyHandler_MAC_SRC_MATCH_ENABLED(uint8_t header, spinel_prop_key_t key,
                                                          const uint8_t *value_ptr, uint16_t value_len)
{
    bool isEnabled;
    spinel_ssize_t parsedLength;
    otError errorCode = OT_ERROR_NONE;

    parsedLength = spinel_datatype_unpack(
                       value_ptr,
                       value_len,
                       SPINEL_DATATYPE_BOOL_S,
                       &isEnabled
                   );

    if (parsedLength > 0)
    {
        errorCode = otLinkRawSrcMatchEnable(mInstance, isEnabled);

        if (errorCode == OT_ERROR_NONE)
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

otError NcpBase::SetPropertyHandler_MAC_SRC_MATCH_SHORT_ADDRESSES(uint8_t header, spinel_prop_key_t key,
                                                                  const uint8_t *value_ptr, uint16_t value_len)
{
    otError errorCode = OT_ERROR_NONE;
    spinel_status_t errorStatus = SPINEL_STATUS_OK;
    const uint8_t *data = value_ptr;
    uint16_t data_len = value_len;

    // Clear the list first
    errorCode = otLinkRawSrcMatchClearShortEntries(mInstance);

    VerifyOrExit(errorCode == OT_ERROR_NONE,
                 errorStatus = ThreadErrorToSpinelStatus(errorCode));

    // Loop through the addresses and add them
    while (data_len >= sizeof(uint16_t))
    {
        spinel_ssize_t parsedLength;
        uint16_t short_address;

        parsedLength = spinel_datatype_unpack(
                           data,
                           data_len,
                           SPINEL_DATATYPE_UINT16_S,
                           &short_address
                       );

        VerifyOrExit(parsedLength > 0, errorStatus = SPINEL_STATUS_PARSE_ERROR);

        data += parsedLength;
        data_len -= (uint16_t)parsedLength;

        errorCode = otLinkRawSrcMatchAddShortEntry(mInstance, short_address);

        VerifyOrExit(errorCode == OT_ERROR_NONE,
                     errorStatus = ThreadErrorToSpinelStatus(errorCode));
    }

    errorCode =
        SendPropertyUpdate(
            header,
            SPINEL_CMD_PROP_VALUE_IS,
            key,
            value_ptr,
            value_len
        );

exit:

    if (errorStatus != SPINEL_STATUS_OK)
    {
        errorCode = SendLastStatus(header, errorStatus);
    }

    return errorCode;
}

otError NcpBase::SetPropertyHandler_MAC_SRC_MATCH_EXTENDED_ADDRESSES(uint8_t header, spinel_prop_key_t key,
                                                                     const uint8_t *value_ptr, uint16_t value_len)
{
    otError errorCode = OT_ERROR_NONE;
    spinel_status_t errorStatus = SPINEL_STATUS_OK;
    const uint8_t *data = value_ptr;
    uint16_t data_len = value_len;

    // Clear the list first
    errorCode = otLinkRawSrcMatchClearExtEntries(mInstance);

    VerifyOrExit(errorCode == OT_ERROR_NONE,
                 errorStatus = ThreadErrorToSpinelStatus(errorCode));

    // Loop through the addresses and add them
    while (data_len >= sizeof(otExtAddress))
    {
        spinel_ssize_t parsedLength;
        uint8_t *ext_address;

        parsedLength = spinel_datatype_unpack(
                           data,
                           data_len,
                           SPINEL_DATATYPE_EUI64_S,
                           &ext_address
                       );

        VerifyOrExit(parsedLength > 0, errorStatus = SPINEL_STATUS_PARSE_ERROR);

        data += parsedLength;
        data_len -= (uint16_t)parsedLength;

        errorCode = otLinkRawSrcMatchAddExtEntry(mInstance, ext_address);

        VerifyOrExit(errorCode == OT_ERROR_NONE,
                     errorStatus = ThreadErrorToSpinelStatus(errorCode));
    }

    errorCode =
        SendPropertyUpdate(
            header,
            SPINEL_CMD_PROP_VALUE_IS,
            key,
            value_ptr,
            value_len
        );

exit:

    if (errorStatus != SPINEL_STATUS_OK)
    {
        errorCode = SendLastStatus(header, errorStatus);
    }

    return errorCode;
}

#endif

#if OPENTHREAD_FTD
otError NcpBase::SetPropertyHandler_NET_PSKC(uint8_t header, spinel_prop_key_t key, const uint8_t *value_ptr,
                                             uint16_t value_len)
{
    const uint8_t *ptr = NULL;
    spinel_size_t len;
    spinel_ssize_t parsedLength;
    otError errorCode = OT_ERROR_NONE;

    parsedLength = spinel_datatype_unpack(
                       value_ptr,
                       value_len,
                       SPINEL_DATATYPE_DATA_S,
                       &ptr,
                       &len
                   );

    if ((parsedLength > 0) && (len == sizeof(spinel_net_pskc_t)))
    {
        errorCode = otThreadSetPSKc(mInstance, ptr);

        if (errorCode == OT_ERROR_NONE)
        {
            errorCode = HandleCommandPropertyGet(header, key);
        }

        if (errorCode)
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
#endif // OPENTHREAD_FTD

otError NcpBase::SetPropertyHandler_THREAD_MODE(uint8_t header, spinel_prop_key_t key, const uint8_t *value_ptr,
                                                uint16_t value_len)
{
    uint8_t numeric_mode = 0;
    otLinkModeConfig mode_config;
    spinel_ssize_t parsedLength;
    otError errorCode = OT_ERROR_NONE;

    parsedLength = spinel_datatype_unpack(
                       value_ptr,
                       value_len,
                       SPINEL_DATATYPE_UINT8_S,
                       &numeric_mode
                   );

    if (parsedLength > 0)
    {
        mode_config.mRxOnWhenIdle = ((numeric_mode & kThreadMode_RxOnWhenIdle) == kThreadMode_RxOnWhenIdle);
        mode_config.mSecureDataRequests =
            ((numeric_mode & kThreadMode_SecureDataRequest) == kThreadMode_SecureDataRequest);
        mode_config.mDeviceType = ((numeric_mode & kThreadMode_FullFunctionDevice) == kThreadMode_FullFunctionDevice);
        mode_config.mNetworkData = ((numeric_mode & kThreadMode_FullNetworkData) == kThreadMode_FullNetworkData);

        errorCode = otThreadSetLinkMode(mInstance, mode_config);

        if (errorCode == OT_ERROR_NONE)
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

#if OPENTHREAD_FTD
otError NcpBase::SetPropertyHandler_THREAD_CHILD_COUNT_MAX(uint8_t header, spinel_prop_key_t key,
                                                           const uint8_t *value_ptr, uint16_t value_len)
{
    uint8_t n = 0;
    spinel_ssize_t parsedLength;
    otError errorCode = OT_ERROR_NONE;

    parsedLength = spinel_datatype_unpack(
                       value_ptr,
                       value_len,
                       SPINEL_DATATYPE_UINT8_S,
                       &n
                   );

    if (parsedLength > 0)
    {
        otThreadSetMaxAllowedChildren(mInstance, n);

        errorCode = HandleCommandPropertyGet(header, key);
    }
    else
    {
        errorCode = SendLastStatus(header, SPINEL_STATUS_PARSE_ERROR);
    }

    return errorCode;
}

otError NcpBase::SetPropertyHandler_THREAD_CHILD_TIMEOUT(uint8_t header, spinel_prop_key_t key,
                                                         const uint8_t *value_ptr, uint16_t value_len)
{
    uint32_t i = 0;
    spinel_ssize_t parsedLength;
    otError errorCode = OT_ERROR_NONE;

    parsedLength = spinel_datatype_unpack(
                       value_ptr,
                       value_len,
                       SPINEL_DATATYPE_UINT32_S,
                       &i
                   );

    if (parsedLength > 0)
    {
        otThreadSetChildTimeout(mInstance, i);

        errorCode = HandleCommandPropertyGet(header, key);
    }
    else
    {
        errorCode = SendLastStatus(header, SPINEL_STATUS_PARSE_ERROR);
    }

    return errorCode;
}

otError NcpBase::SetPropertyHandler_THREAD_ROUTER_UPGRADE_THRESHOLD(uint8_t header, spinel_prop_key_t key,
                                                                    const uint8_t *value_ptr, uint16_t value_len)
{
    uint8_t i = 0;
    spinel_ssize_t parsedLength;
    otError errorCode = OT_ERROR_NONE;

    parsedLength = spinel_datatype_unpack(
                       value_ptr,
                       value_len,
                       SPINEL_DATATYPE_UINT8_S,
                       &i
                   );

    if (parsedLength > 0)
    {
        otThreadSetRouterUpgradeThreshold(mInstance, i);

        errorCode = HandleCommandPropertyGet(header, key);
    }
    else
    {
        errorCode = SendLastStatus(header, SPINEL_STATUS_PARSE_ERROR);
    }

    return errorCode;
}

otError NcpBase::SetPropertyHandler_THREAD_ROUTER_DOWNGRADE_THRESHOLD(uint8_t header, spinel_prop_key_t key,
                                                                      const uint8_t *value_ptr, uint16_t value_len)
{
    uint8_t i = 0;
    spinel_ssize_t parsedLength;
    otError errorCode = OT_ERROR_NONE;

    parsedLength = spinel_datatype_unpack(
                       value_ptr,
                       value_len,
                       SPINEL_DATATYPE_UINT8_S,
                       &i
                   );

    if (parsedLength > 0)
    {
        otThreadSetRouterDowngradeThreshold(mInstance, i);

        errorCode = HandleCommandPropertyGet(header, key);
    }
    else
    {
        errorCode = SendLastStatus(header, SPINEL_STATUS_PARSE_ERROR);
    }

    return errorCode;
}

otError NcpBase::SetPropertyHandler_THREAD_ROUTER_SELECTION_JITTER(uint8_t header, spinel_prop_key_t key,
                                                                   const uint8_t *value_ptr, uint16_t value_len)
{
    uint8_t i = 0;
    spinel_ssize_t parsedLength;
    otError errorCode = OT_ERROR_NONE;

    parsedLength = spinel_datatype_unpack(
                       value_ptr,
                       value_len,
                       SPINEL_DATATYPE_UINT8_S,
                       &i
                   );

    if (parsedLength > 0)
    {
        otThreadSetRouterSelectionJitter(mInstance, i);

        errorCode = HandleCommandPropertyGet(header, key);
    }
    else
    {
        errorCode = SendLastStatus(header, SPINEL_STATUS_PARSE_ERROR);
    }

    return errorCode;
}

otError NcpBase::SetPropertyHandler_THREAD_PREFERRED_ROUTER_ID(uint8_t header, spinel_prop_key_t key,
                                                               const uint8_t *value_ptr, uint16_t value_len)
{
    uint8_t router_id = 0;
    spinel_ssize_t parsedLength;
    otError errorCode = OT_ERROR_NONE;

    parsedLength = spinel_datatype_unpack(
                       value_ptr,
                       value_len,
                       SPINEL_DATATYPE_UINT8_S,
                       &router_id
                   );

    if (parsedLength > 0)
    {
        errorCode = otThreadSetPreferredRouterId(mInstance, router_id);

        if (errorCode == OT_ERROR_NONE)
        {
            errorCode = SendPropertyUpdate(
                            header,
                            SPINEL_CMD_PROP_VALUE_IS,
                            key,
                            SPINEL_DATATYPE_UINT8_S,
                            router_id
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
#endif  // OPENTHREAD_FTD

otError NcpBase::SetPropertyHandler_DEBUG_NCP_LOG_LEVEL(uint8_t header, spinel_prop_key_t key,
                                                        const uint8_t *value_ptr, uint16_t value_len)
{
    uint8_t spinelNcpLogLevel = 0;
    otLogLevel logLevel;
    spinel_ssize_t parsedLength;
    otError errorCode = OT_ERROR_NONE;

    parsedLength = spinel_datatype_unpack(
                       value_ptr,
                       value_len,
                       SPINEL_DATATYPE_UINT8_S,
                       &spinelNcpLogLevel
                   );

    if (parsedLength > 0)
    {
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
            errorCode = OT_ERROR_INVALID_ARGS;
            break;
        }

        if (errorCode == OT_ERROR_NONE)
        {
            errorCode = otSetDynamicLogLevel(mInstance, logLevel);
        }

        if (errorCode == OT_ERROR_NONE)
        {
            errorCode = HandleCommandPropertyGet(header, key);
        }
        else
        {
            if (errorCode == OT_ERROR_DISABLED_FEATURE)
            {
                errorCode = SendLastStatus(header, SPINEL_STATUS_INVALID_COMMAND_FOR_PROP);
            }
            else
            {
                errorCode = SendLastStatus(header, ThreadErrorToSpinelStatus(errorCode));
            }
        }
    }
    else
    {
        errorCode = SendLastStatus(header, SPINEL_STATUS_PARSE_ERROR);
    }

    return errorCode;
}

#if OPENTHREAD_FTD
otError NcpBase::SetPropertyHandler_THREAD_CONTEXT_REUSE_DELAY(uint8_t header, spinel_prop_key_t key,
                                                               const uint8_t *value_ptr, uint16_t value_len)
{
    uint32_t i = 0;
    spinel_ssize_t parsedLength;
    otError errorCode = OT_ERROR_NONE;

    parsedLength = spinel_datatype_unpack(
                       value_ptr,
                       value_len,
                       SPINEL_DATATYPE_UINT32_S,
                       &i
                   );

    if (parsedLength > 0)
    {
        otThreadSetContextIdReuseDelay(mInstance, i);

        errorCode = HandleCommandPropertyGet(header, key);
    }
    else
    {
        errorCode = SendLastStatus(header, SPINEL_STATUS_PARSE_ERROR);
    }

    return errorCode;
}

otError NcpBase::SetPropertyHandler_THREAD_NETWORK_ID_TIMEOUT(uint8_t header, spinel_prop_key_t key,
                                                              const uint8_t *value_ptr, uint16_t value_len)
{
    uint8_t i = 0;
    spinel_ssize_t parsedLength;
    otError errorCode = OT_ERROR_NONE;

    parsedLength = spinel_datatype_unpack(
                       value_ptr,
                       value_len,
                       SPINEL_DATATYPE_UINT8_S,
                       &i
                   );

    if (parsedLength > 0)
    {
        otThreadSetNetworkIdTimeout(mInstance, i);

        errorCode = HandleCommandPropertyGet(header, key);
    }
    else
    {
        errorCode = SendLastStatus(header, SPINEL_STATUS_PARSE_ERROR);
    }

    return errorCode;
}
#endif  // OPENTHREAD_FTD

#if OPENTHREAD_ENABLE_TMF_PROXY && OPENTHREAD_FTD
otError NcpBase::SetPropertyHandler_TMF_PROXY_ENABLED(uint8_t header, spinel_prop_key_t key,
                                                     const uint8_t *value_ptr, uint16_t value_len)
{
    bool isEnabled;
    spinel_ssize_t parsedLength;
    otError errorCode = OT_ERROR_NONE;

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
            SuccessOrExit(errorCode = otTmfProxyStart(mInstance, &NcpBase::HandleTmfProxyStream, this));
        }
        else
        {
            SuccessOrExit(errorCode = otTmfProxyStop(mInstance));
        }

        SuccessOrExit(errorCode = HandleCommandPropertyGet(header, key));
    }
    else
    {
        errorCode = OT_ERROR_PARSE;
    }

exit:

    if (errorCode != OT_ERROR_NONE)
    {
        errorCode = SendLastStatus(header, ThreadErrorToSpinelStatus(errorCode));
    }

    return errorCode;
}
#endif // OPENTHREAD_ENABLE_TMF_PROXY && OPENTHREAD_FTD

#if OPENTHREAD_ENABLE_JAM_DETECTION

otError NcpBase::SetPropertyHandler_JAM_DETECT_ENABLE(uint8_t header, spinel_prop_key_t key,
                                                      const uint8_t *value_ptr, uint16_t value_len)
{
    bool isEnabled;
    spinel_ssize_t parsedLength;
    otError errorCode = OT_ERROR_NONE;

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
            otJamDetectionStart(mInstance, &NcpBase::HandleJamStateChange_Jump, this);
        }
        else
        {
            otJamDetectionStop(mInstance);
        }

        errorCode = HandleCommandPropertyGet(header, key);
    }
    else
    {
        errorCode = SendLastStatus(header, SPINEL_STATUS_PARSE_ERROR);
    }

    return errorCode;
}

otError NcpBase::SetPropertyHandler_JAM_DETECT_RSSI_THRESHOLD(uint8_t header, spinel_prop_key_t key,
                                                              const uint8_t *value_ptr, uint16_t value_len)
{
    int8_t value = 0;
    spinel_ssize_t parsedLength;
    otError errorCode = OT_ERROR_NONE;

    parsedLength = spinel_datatype_unpack(
                       value_ptr,
                       value_len,
                       SPINEL_DATATYPE_INT8_S,
                       &value
                   );

    if (parsedLength > 0)
    {
        errorCode = otJamDetectionSetRssiThreshold(mInstance, value);

        if (errorCode == OT_ERROR_NONE)
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

otError NcpBase::SetPropertyHandler_JAM_DETECT_WINDOW(uint8_t header, spinel_prop_key_t key,
                                                      const uint8_t *value_ptr, uint16_t value_len)
{
    uint8_t value = 0;
    spinel_ssize_t parsedLength;
    otError errorCode = OT_ERROR_NONE;

    parsedLength = spinel_datatype_unpack(
                       value_ptr,
                       value_len,
                       SPINEL_DATATYPE_UINT8_S,
                       &value
                   );

    if (parsedLength > 0)
    {
        errorCode = otJamDetectionSetWindow(mInstance, value);

        if (errorCode == OT_ERROR_NONE)
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

otError NcpBase::SetPropertyHandler_JAM_DETECT_BUSY(uint8_t header, spinel_prop_key_t key, const uint8_t *value_ptr,
                                                    uint16_t value_len)
{
    uint8_t value = 0;
    spinel_ssize_t parsedLength;
    otError errorCode = OT_ERROR_NONE;

    parsedLength = spinel_datatype_unpack(
                       value_ptr,
                       value_len,
                       SPINEL_DATATYPE_UINT8_S,
                       &value
                   );

    if (parsedLength > 0)
    {
        errorCode = otJamDetectionSetBusyPeriod(mInstance, value);

        if (errorCode == OT_ERROR_NONE)
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

void NcpBase::HandleJamStateChange_Jump(bool aJamState, void *aContext)
{
    static_cast<NcpBase *>(aContext)->HandleJamStateChange(aJamState);
}

void NcpBase::HandleJamStateChange(bool aJamState)
{
    otError errorCode;

    errorCode = SendPropertyUpdate(
                    SPINEL_HEADER_FLAG | SPINEL_HEADER_IID_0,
                    SPINEL_CMD_PROP_VALUE_IS,
                    SPINEL_PROP_JAM_DETECTED,
                    SPINEL_DATATYPE_BOOL_S,
                    aJamState
                );

    // If we could not send the jam state change indicator (no
    // buffer space), we set `mShouldSignalJamStateChange` to true to send
    // it out when buffer space becomes available.
    if (errorCode != OT_ERROR_NONE)
    {
        mShouldSignalJamStateChange = true;
    }
}

#endif // OPENTHREAD_ENABLE_JAM_DETECTION

#if OPENTHREAD_ENABLE_DIAG
otError NcpBase::SetPropertyHandler_NEST_STREAM_MFG(uint8_t header, spinel_prop_key_t key, const uint8_t *value_ptr,
                                                    uint16_t value_len)
{
    char *string(NULL);
    char *output(NULL);
    spinel_ssize_t parsedLength;
    otError errorCode = OT_ERROR_NONE;

    parsedLength = spinel_datatype_unpack(
                       value_ptr,
                       value_len,
                       SPINEL_DATATYPE_UTF8_S,
                       &string
                   );

    if ((parsedLength > 0) && (string != NULL))
    {
        // all diagnostics related features are processed within diagnostics module
        output = otDiagProcessCmdLine(string);

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

#if OPENTHREAD_ENABLE_LEGACY
otError NcpBase::SetPropertyHandler_NEST_LEGACY_ULA_PREFIX(uint8_t header, spinel_prop_key_t key,
                                                           const uint8_t *value_ptr, uint16_t value_len)
{
    otError errorCode = OT_ERROR_NONE;
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

    if ((parsedLength > 0) && (len <= sizeof(mLegacyUlaPrefix)))
    {
        memset(mLegacyUlaPrefix, 0, sizeof(mLegacyUlaPrefix));
        memcpy(mLegacyUlaPrefix, ptr, len);

        if (mLegacyHandlers)
        {
            if (mLegacyHandlers->mSetLegacyUlaPrefix)
            {
                mLegacyHandlers->mSetLegacyUlaPrefix(mLegacyUlaPrefix);
            }
        }

        errorCode = HandleCommandPropertyGet(header, key);
    }
    else
    {
        errorCode = SendLastStatus(header, SPINEL_STATUS_PARSE_ERROR);
    }

    return errorCode;
}
#endif  // OPENTHREAD_ENABLE_LEGACY

// ----------------------------------------------------------------------------
// MARK: Individual Property Inserters
// ----------------------------------------------------------------------------

#if OPENTHREAD_ENABLE_RAW_LINK_API

otError NcpBase::InsertPropertyHandler_MAC_SRC_MATCH_SHORT_ADDRESSES(uint8_t header, spinel_prop_key_t key,
                                                                     const uint8_t *value_ptr, uint16_t value_len)
{
    spinel_ssize_t parsedLength;
    otError errorCode = OT_ERROR_NONE;
    spinel_status_t errorStatus = SPINEL_STATUS_OK;
    uint16_t short_address;

    parsedLength = spinel_datatype_unpack(
                       value_ptr,
                       value_len,
                       SPINEL_DATATYPE_UINT16_S,
                       &short_address
                   );

    VerifyOrExit(parsedLength > 0, errorStatus = SPINEL_STATUS_PARSE_ERROR);

    errorCode = otLinkRawSrcMatchAddShortEntry(mInstance, short_address);

    VerifyOrExit(errorCode == OT_ERROR_NONE,
                 errorStatus = ThreadErrorToSpinelStatus(errorCode));

    errorCode =
        SendPropertyUpdate(
            header,
            SPINEL_CMD_PROP_VALUE_INSERTED,
            key,
            value_ptr,
            value_len
        );

exit:

    if (errorStatus != SPINEL_STATUS_OK)
    {
        errorCode = SendLastStatus(header, errorStatus);
    }

    return errorCode;
}

otError NcpBase::InsertPropertyHandler_MAC_SRC_MATCH_EXTENDED_ADDRESSES(uint8_t header, spinel_prop_key_t key,
                                                                        const uint8_t *value_ptr, uint16_t value_len)
{
    spinel_ssize_t parsedLength;
    otError errorCode = OT_ERROR_NONE;
    spinel_status_t errorStatus = SPINEL_STATUS_OK;
    uint8_t *ext_address = NULL;

    parsedLength = spinel_datatype_unpack(
                       value_ptr,
                       value_len,
                       SPINEL_DATATYPE_EUI64_S,
                       &ext_address
                   );

    VerifyOrExit(parsedLength > 0, errorStatus = SPINEL_STATUS_PARSE_ERROR);

    errorCode = otLinkRawSrcMatchAddExtEntry(mInstance, ext_address);

    VerifyOrExit(errorCode == OT_ERROR_NONE,
                 errorStatus = ThreadErrorToSpinelStatus(errorCode));

    errorCode =
        SendPropertyUpdate(
            header,
            SPINEL_CMD_PROP_VALUE_INSERTED,
            key,
            value_ptr,
            value_len
        );

exit:

    if (errorStatus != SPINEL_STATUS_OK)
    {
        errorCode = SendLastStatus(header, errorStatus);
    }

    return errorCode;
}

#endif

otError NcpBase::InsertPropertyHandler_IPV6_ADDRESS_TABLE(uint8_t header, spinel_prop_key_t key,
                                                          const uint8_t *value_ptr, uint16_t value_len)
{
    spinel_ssize_t parsedLength;
    otError errorCode = OT_ERROR_NONE;
    spinel_status_t errorStatus = SPINEL_STATUS_OK;
    otNetifAddress netif_addr;
    otIp6Address *addr_ptr;
    uint32_t preferred_lifetime;
    uint32_t valid_lifetime;
    uint8_t  prefix_len;

    parsedLength = spinel_datatype_unpack(
                       value_ptr,
                       value_len,
                       (
                          SPINEL_DATATYPE_IPv6ADDR_S    // IPv6 address
                          SPINEL_DATATYPE_UINT8_S       // Prefix length (in bits)
                          SPINEL_DATATYPE_UINT32_S      // Preferred lifetime
                          SPINEL_DATATYPE_UINT32_S      // Valid lifetime
                       ),
                       &addr_ptr,
                       &prefix_len,
                       &preferred_lifetime,
                       &valid_lifetime
                   );

    VerifyOrExit(parsedLength > 0, errorStatus = SPINEL_STATUS_PARSE_ERROR);

    netif_addr.mAddress = *addr_ptr;
    netif_addr.mPrefixLength = prefix_len;
    netif_addr.mPreferred = preferred_lifetime != 0;
    netif_addr.mValid = valid_lifetime != 0;

    errorCode = otIp6AddUnicastAddress(mInstance, &netif_addr);

    VerifyOrExit(errorCode == OT_ERROR_NONE,
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

#if OPENTHREAD_ENABLE_BORDER_ROUTER
otError NcpBase::InsertPropertyHandler_THREAD_OFF_MESH_ROUTES(uint8_t header, spinel_prop_key_t key,
                                                              const uint8_t *value_ptr, uint16_t value_len)
{
    spinel_ssize_t parsedLength;
    otError errorCode = OT_ERROR_NONE;

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
                       (
                           SPINEL_DATATYPE_IPv6ADDR_S  // Route prefix
                           SPINEL_DATATYPE_UINT8_S     // Prefix length (in bits)
                           SPINEL_DATATYPE_BOOL_S      // Stable
                           SPINEL_DATATYPE_UINT8_S     // Flags
                       ),
                       &addr_ptr,
                       &ext_route_config.mPrefix.mLength,
                       &stable,
                       &flags
                   );

    if (parsedLength > 0)
    {
        ext_route_config.mPrefix.mPrefix = *addr_ptr;
        ext_route_config.mStable = stable;
        ext_route_config.mPreference = ((flags & SPINEL_NET_FLAG_PREFERENCE_MASK) >> SPINEL_NET_FLAG_PREFERENCE_OFFSET);
        errorCode = otBorderRouterAddRoute(mInstance, &ext_route_config);

        if (errorCode == OT_ERROR_NONE)
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

otError NcpBase::InsertPropertyHandler_THREAD_ON_MESH_NETS(uint8_t header, spinel_prop_key_t key,
                                                           const uint8_t *value_ptr, uint16_t value_len)
{
    spinel_ssize_t parsedLength;
    otError errorCode = OT_ERROR_NONE;

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
                       (
                           SPINEL_DATATYPE_IPv6ADDR_S  // On-mesh prefix
                           SPINEL_DATATYPE_UINT8_S     // Prefix length (in bits)
                           SPINEL_DATATYPE_BOOL_S      // Stable
                           SPINEL_DATATYPE_UINT8_S     // Flags
                       ),
                       &addr_ptr,
                       &border_router_config.mPrefix.mLength,
                       &stable,
                       &flags
                   );

    if (parsedLength > 0)
    {
        border_router_config.mPrefix.mPrefix = *addr_ptr;
        border_router_config.mStable = stable;
        border_router_config.mPreference =
            ((flags & SPINEL_NET_FLAG_PREFERENCE_MASK) >> SPINEL_NET_FLAG_PREFERENCE_OFFSET);
        border_router_config.mPreferred = ((flags & SPINEL_NET_FLAG_PREFERRED) != 0);
        border_router_config.mSlaac = ((flags & SPINEL_NET_FLAG_SLAAC) != 0);
        border_router_config.mDhcp = ((flags & SPINEL_NET_FLAG_DHCP) != 0);
        border_router_config.mConfigure = ((flags & SPINEL_NET_FLAG_CONFIGURE) != 0);
        border_router_config.mDefaultRoute = ((flags & SPINEL_NET_FLAG_DEFAULT_ROUTE) != 0);
        border_router_config.mOnMesh = ((flags & SPINEL_NET_FLAG_ON_MESH) != 0);

        errorCode = otBorderRouterAddOnMeshPrefix(mInstance, &border_router_config);

        if (errorCode == OT_ERROR_NONE)
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
#endif // OPENTHREAD_ENABLE_BORDER_ROUTER

otError NcpBase::InsertPropertyHandler_THREAD_ASSISTING_PORTS(uint8_t header, spinel_prop_key_t key,
                                                              const uint8_t *value_ptr, uint16_t value_len)
{
    spinel_ssize_t parsedLength;
    otError errorCode = OT_ERROR_NONE;
    uint16_t port;

    parsedLength = spinel_datatype_unpack(
                       value_ptr,
                       value_len,
                       SPINEL_DATATYPE_UINT16_S,
                       &port
                   );

    if (parsedLength > 0)
    {
        errorCode = otIp6AddUnsecurePort(mInstance, port);

        if (errorCode == OT_ERROR_NONE)
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

#if OPENTHREAD_ENABLE_MAC_WHITELIST

otError NcpBase::InsertPropertyHandler_MAC_WHITELIST(uint8_t header, spinel_prop_key_t key,
                                                     const uint8_t *value_ptr, uint16_t value_len)
{
    otError errorCode = OT_ERROR_NONE;
    spinel_ssize_t parsedLength;
    otExtAddress *ext_addr = NULL;
    int8_t rssi = RSSI_OVERRIDE_DISABLED;

    if (value_len > static_cast<spinel_ssize_t>(sizeof(otExtAddress)))
    {
        parsedLength = spinel_datatype_unpack(
                           value_ptr,
                           value_len,
                           SPINEL_DATATYPE_EUI64_S SPINEL_DATATYPE_INT8_S,
                           &ext_addr,
                           &rssi
                       );
    }
    else
    {
        parsedLength = spinel_datatype_unpack(
                           value_ptr,
                           value_len,
                           SPINEL_DATATYPE_EUI64_S,
                           &ext_addr
                       );
    }

    if (parsedLength > 0)
    {
        if (rssi == RSSI_OVERRIDE_DISABLED)
        {
            errorCode = otLinkAddWhitelist(mInstance, ext_addr->m8);
        }
        else
        {
            errorCode = otLinkAddWhitelistRssi(mInstance, ext_addr->m8, rssi);
        }

        if (errorCode == OT_ERROR_NONE)
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

otError NcpBase::InsertPropertyHandler_MAC_BLACKLIST(uint8_t header, spinel_prop_key_t key,
                                                     const uint8_t *value_ptr, uint16_t value_len)
{
    otError errorCode = OT_ERROR_NONE;
    spinel_ssize_t parsedLength;
    otExtAddress *ext_addr = NULL;

    parsedLength = spinel_datatype_unpack(
                       value_ptr,
                       value_len,
                       SPINEL_DATATYPE_EUI64_S,
                       &ext_addr
                   );

    if (parsedLength > 0)
    {
        errorCode = otLinkAddBlacklist(mInstance, ext_addr->m8);

        if (errorCode == OT_ERROR_NONE)
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

#endif // OPENTHREAD_ENABLE_MAC_WHITELIST

#if OPENTHREAD_ENABLE_COMMISSIONER && OPENTHREAD_FTD
otError NcpBase::InsertPropertyHandler_THREAD_JOINERS(uint8_t header, spinel_prop_key_t key,
                                                      const uint8_t *value_ptr, uint16_t value_len)
{
    spinel_ssize_t parsedLength;
    otError errorCode = OT_ERROR_NONE;

    otExtAddress *ot_ext_address = NULL;
    const char *aPSKd = NULL;
    uint32_t joiner_timeout = 0;

    VerifyOrExit(
        mAllowLocalNetworkDataChange == true,
        errorCode = SendLastStatus(header, SPINEL_STATUS_INVALID_STATE)
    );

    parsedLength = spinel_datatype_unpack(
                       value_ptr,
                       value_len,
                       (
                           SPINEL_DATATYPE_UTF8_S     // PSK
                           SPINEL_DATATYPE_UINT32_S   // Timeout
                           SPINEL_DATATYPE_EUI64_S    // Extended address
                       ),
                       &aPSKd,
                       &joiner_timeout,
                       &ot_ext_address
                   );

    if (parsedLength <= 0)
    {
        parsedLength = spinel_datatype_unpack(
                           value_ptr,
                           value_len,
                           (
                              SPINEL_DATATYPE_UTF8_S     // PSK
                              SPINEL_DATATYPE_UINT32_S   // Timeout
                           ),
                           &aPSKd,
                           &joiner_timeout
                       );
        ot_ext_address = NULL;
    }

    if (parsedLength > 0)
    {
        errorCode = otCommissionerAddJoiner(mInstance, ot_ext_address, aPSKd, joiner_timeout);

        if (errorCode == OT_ERROR_NONE)
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
#endif // OPENTHREAD_ENABLE_COMMISSIONER && OPENTHREAD_FTD

// ----------------------------------------------------------------------------
// MARK: Individual Property Removers
// ----------------------------------------------------------------------------

#if OPENTHREAD_ENABLE_RAW_LINK_API

otError NcpBase::RemovePropertyHandler_MAC_SRC_MATCH_SHORT_ADDRESSES(uint8_t header, spinel_prop_key_t key,
                                                                     const uint8_t *value_ptr, uint16_t value_len)
{
    spinel_ssize_t parsedLength;
    otError errorCode = OT_ERROR_NONE;
    uint16_t short_address;

    parsedLength = spinel_datatype_unpack(
                       value_ptr,
                       value_len,
                       SPINEL_DATATYPE_UINT16_S,
                       &short_address
                   );

    if (parsedLength > 0)
    {
        errorCode = otLinkRawSrcMatchClearShortEntry(mInstance, short_address);

        if (errorCode == OT_ERROR_NONE)
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

otError NcpBase::RemovePropertyHandler_MAC_SRC_MATCH_EXTENDED_ADDRESSES(uint8_t header, spinel_prop_key_t key,
                                                                        const uint8_t *value_ptr, uint16_t value_len)
{
    spinel_ssize_t parsedLength;
    otError errorCode = OT_ERROR_NONE;
    uint8_t *ext_address;

    parsedLength = spinel_datatype_unpack(
                       value_ptr,
                       value_len,
                       SPINEL_DATATYPE_EUI64_S,
                       &ext_address
                   );

    if (parsedLength > 0)
    {
        errorCode = otLinkRawSrcMatchClearExtEntry(mInstance, ext_address);

        if (errorCode == OT_ERROR_NONE)
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

#endif

otError NcpBase::RemovePropertyHandler_IPV6_ADDRESS_TABLE(uint8_t header, spinel_prop_key_t key,
                                                          const uint8_t *value_ptr, uint16_t value_len)
{
    spinel_ssize_t parsedLength;
    otError errorCode = OT_ERROR_NONE;
    otIp6Address *addr_ptr;

    parsedLength = spinel_datatype_unpack(
                       value_ptr,
                       value_len,
                       SPINEL_DATATYPE_IPv6ADDR_S,
                       &addr_ptr
                   );

    if (parsedLength > 0)
    {
        errorCode = otIp6RemoveUnicastAddress(mInstance, addr_ptr);

        if (errorCode == OT_ERROR_NONE)
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

#if OPENTHREAD_ENABLE_BORDER_ROUTER
otError NcpBase::RemovePropertyHandler_THREAD_OFF_MESH_ROUTES(uint8_t header, spinel_prop_key_t key,
                                                              const uint8_t *value_ptr, uint16_t value_len)
{
    spinel_ssize_t parsedLength;
    otError errorCode = OT_ERROR_NONE;

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
                       (
                           SPINEL_DATATYPE_IPv6ADDR_S     // Route prefix
                           SPINEL_DATATYPE_UINT8_S        // Prefix length (in bits)
                       ),
                       &addr_ptr,
                       &ip6_prefix.mLength
                   );

    if (parsedLength > 0)
    {
        ip6_prefix.mPrefix = *addr_ptr;
        errorCode = otBorderRouterRemoveRoute(mInstance, &ip6_prefix);

        if (errorCode == OT_ERROR_NONE)
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

otError NcpBase::RemovePropertyHandler_THREAD_ON_MESH_NETS(uint8_t header, spinel_prop_key_t key,
                                                           const uint8_t *value_ptr, uint16_t value_len)
{
    spinel_ssize_t parsedLength;
    otError errorCode = OT_ERROR_NONE;

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
                       (
                           SPINEL_DATATYPE_IPv6ADDR_S     // On-mesh prefix
                           SPINEL_DATATYPE_UINT8_S        // Prefix length (in bits)
                       ),
                       &addr_ptr,
                       &ip6_prefix.mLength
                   );

    if (parsedLength > 0)
    {
        ip6_prefix.mPrefix = *addr_ptr;
        errorCode = otBorderRouterRemoveOnMeshPrefix(mInstance, &ip6_prefix);

        if (errorCode == OT_ERROR_NONE)
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
#endif

otError NcpBase::RemovePropertyHandler_THREAD_ASSISTING_PORTS(uint8_t header, spinel_prop_key_t key,
                                                              const uint8_t *value_ptr, uint16_t value_len)
{
    spinel_ssize_t parsedLength;
    otError errorCode = OT_ERROR_NONE;
    uint16_t port;

    parsedLength = spinel_datatype_unpack(
                       value_ptr,
                       value_len,
                       SPINEL_DATATYPE_UINT16_S,
                       &port
                   );

    if (parsedLength > 0)
    {
        errorCode = otIp6RemoveUnsecurePort(mInstance, port);

        if (errorCode == OT_ERROR_NONE)
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

#if OPENTHREAD_FTD
otError NcpBase::RemovePropertyHandler_THREAD_ACTIVE_ROUTER_IDS(uint8_t header, spinel_prop_key_t key,
                                                                const uint8_t *value_ptr, uint16_t value_len)
{
    spinel_ssize_t parsedLength;
    otError errorCode = OT_ERROR_NONE;
    uint8_t router_id;

    parsedLength = spinel_datatype_unpack(
                       value_ptr,
                       value_len,
                       SPINEL_DATATYPE_UINT8_S,
                       &router_id
                   );

    if (parsedLength > 0)
    {
        errorCode = otThreadReleaseRouterId(mInstance, router_id);

        if (errorCode == OT_ERROR_NONE)
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
#endif  // OPENTHREAD_FTD

#if OPENTHREAD_ENABLE_MAC_WHITELIST

otError NcpBase::RemovePropertyHandler_MAC_WHITELIST(uint8_t header, spinel_prop_key_t key,
                                                     const uint8_t *value_ptr, uint16_t value_len)
{
    otError errorCode = OT_ERROR_NONE;
    spinel_ssize_t parsedLength;
    otExtAddress *ext_addr_ptr = NULL;

    parsedLength = spinel_datatype_unpack(
                       value_ptr,
                       value_len,
                       SPINEL_DATATYPE_EUI64_S,
                       &ext_addr_ptr
                   );

    if (parsedLength > 0)
    {
        otLinkRemoveWhitelist(mInstance, ext_addr_ptr->m8);

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

otError NcpBase::RemovePropertyHandler_MAC_BLACKLIST(uint8_t header, spinel_prop_key_t key,
                                                     const uint8_t *value_ptr, uint16_t value_len)
{
    otError errorCode = OT_ERROR_NONE;
    spinel_ssize_t parsedLength;
    otExtAddress *ext_addr_ptr = NULL;

    parsedLength = spinel_datatype_unpack(
                       value_ptr,
                       value_len,
                       SPINEL_DATATYPE_EUI64_S,
                       &ext_addr_ptr
                   );

    if (parsedLength > 0)
    {
        otLinkRemoveBlacklist(mInstance, ext_addr_ptr->m8);

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
    otError errorCode = OT_ERROR_NONE;

    if (aStreamId == 0)
    {
        aStreamId = SPINEL_PROP_STREAM_DEBUG;
    }

    if (!mDisableStreamWrite)
    {
        errorCode = SendPropertyUpdate(
                        SPINEL_HEADER_FLAG | SPINEL_HEADER_IID_0,
                        SPINEL_CMD_PROP_VALUE_IS,
                        static_cast<spinel_prop_key_t>(aStreamId),
                        aDataPtr,
                        static_cast<uint16_t>(aDataLen)
                    );
    }
    else
    {
        errorCode = OT_ERROR_INVALID_STATE;
    }

    return errorCode;
}

}  // namespace ot


// ----------------------------------------------------------------------------
// MARK: Virtual Datastream I/O (Public API)
// ----------------------------------------------------------------------------

otError otNcpStreamWrite(int aStreamId, const uint8_t *aDataPtr, int aDataLen)
{
    otError errorCode  = OT_ERROR_INVALID_STATE;
    ot::NcpBase *ncp = ot::NcpBase::GetNcpInstance();

    if (ncp != NULL)
    {
        errorCode = ncp->StreamWrite(aStreamId, aDataPtr, aDataLen);
    }

    return errorCode;
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
    (void)aHandlers;
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
    (void)aUlaPrefix;
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
    (void)aExtAddr;
#endif
}
