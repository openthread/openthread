/*
 *    Copyright (c) 2018, The OpenThread Authors.
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

namespace ot {
namespace Ncp {

NcpBase::PropertyHandler NcpBase::FindGetPropertyHandler(spinel_prop_key_t aKey)
{
    NcpBase::PropertyHandler handler;

    switch (aKey)
    {
        // --------------------------------------------------------------------------
        // Common Properties (Get Handler)

    case SPINEL_PROP_CAPS:
        handler = &NcpBase::HandlePropertyGet<SPINEL_PROP_CAPS>;
        break;
    case SPINEL_PROP_DEBUG_TEST_ASSERT:
        handler = &NcpBase::HandlePropertyGet<SPINEL_PROP_DEBUG_TEST_ASSERT>;
        break;
    case SPINEL_PROP_DEBUG_TEST_WATCHDOG:
        handler = &NcpBase::HandlePropertyGet<SPINEL_PROP_DEBUG_TEST_WATCHDOG>;
        break;
    case SPINEL_PROP_DEBUG_NCP_LOG_LEVEL:
        handler = &NcpBase::HandlePropertyGet<SPINEL_PROP_DEBUG_NCP_LOG_LEVEL>;
        break;
    case SPINEL_PROP_HWADDR:
        handler = &NcpBase::HandlePropertyGet<SPINEL_PROP_HWADDR>;
        break;
    case SPINEL_PROP_HOST_POWER_STATE:
        handler = &NcpBase::HandlePropertyGet<SPINEL_PROP_HOST_POWER_STATE>;
        break;
    case SPINEL_PROP_INTERFACE_COUNT:
        handler = &NcpBase::HandlePropertyGet<SPINEL_PROP_INTERFACE_COUNT>;
        break;
    case SPINEL_PROP_INTERFACE_TYPE:
        handler = &NcpBase::HandlePropertyGet<SPINEL_PROP_INTERFACE_TYPE>;
        break;
    case SPINEL_PROP_LAST_STATUS:
        handler = &NcpBase::HandlePropertyGet<SPINEL_PROP_LAST_STATUS>;
        break;
    case SPINEL_PROP_LOCK:
        handler = &NcpBase::HandlePropertyGet<SPINEL_PROP_LOCK>;
        break;
    case SPINEL_PROP_PHY_ENABLED:
        handler = &NcpBase::HandlePropertyGet<SPINEL_PROP_PHY_ENABLED>;
        break;
    case SPINEL_PROP_PHY_CHAN:
        handler = &NcpBase::HandlePropertyGet<SPINEL_PROP_PHY_CHAN>;
        break;
    case SPINEL_PROP_PHY_RSSI:
        handler = &NcpBase::HandlePropertyGet<SPINEL_PROP_PHY_RSSI>;
        break;
    case SPINEL_PROP_PHY_RX_SENSITIVITY:
        handler = &NcpBase::HandlePropertyGet<SPINEL_PROP_PHY_RX_SENSITIVITY>;
        break;
    case SPINEL_PROP_PHY_TX_POWER:
        handler = &NcpBase::HandlePropertyGet<SPINEL_PROP_PHY_TX_POWER>;
        break;
    case SPINEL_PROP_POWER_STATE:
        handler = &NcpBase::HandlePropertyGet<SPINEL_PROP_POWER_STATE>;
        break;
    case SPINEL_PROP_MCU_POWER_STATE:
        handler = &NcpBase::HandlePropertyGet<SPINEL_PROP_MCU_POWER_STATE>;
        break;
    case SPINEL_PROP_PROTOCOL_VERSION:
        handler = &NcpBase::HandlePropertyGet<SPINEL_PROP_PROTOCOL_VERSION>;
        break;
    case SPINEL_PROP_MAC_15_4_PANID:
        handler = &NcpBase::HandlePropertyGet<SPINEL_PROP_MAC_15_4_PANID>;
        break;
    case SPINEL_PROP_MAC_15_4_LADDR:
        handler = &NcpBase::HandlePropertyGet<SPINEL_PROP_MAC_15_4_LADDR>;
        break;
    case SPINEL_PROP_MAC_15_4_SADDR:
        handler = &NcpBase::HandlePropertyGet<SPINEL_PROP_MAC_15_4_SADDR>;
        break;
    case SPINEL_PROP_MAC_RAW_STREAM_ENABLED:
        handler = &NcpBase::HandlePropertyGet<SPINEL_PROP_MAC_RAW_STREAM_ENABLED>;
        break;
    case SPINEL_PROP_MAC_PROMISCUOUS_MODE:
        handler = &NcpBase::HandlePropertyGet<SPINEL_PROP_MAC_PROMISCUOUS_MODE>;
        break;
    case SPINEL_PROP_MAC_SCAN_STATE:
        handler = &NcpBase::HandlePropertyGet<SPINEL_PROP_MAC_SCAN_STATE>;
        break;
    case SPINEL_PROP_MAC_SCAN_MASK:
        handler = &NcpBase::HandlePropertyGet<SPINEL_PROP_MAC_SCAN_MASK>;
        break;
    case SPINEL_PROP_MAC_SCAN_PERIOD:
        handler = &NcpBase::HandlePropertyGet<SPINEL_PROP_MAC_SCAN_PERIOD>;
        break;
    case SPINEL_PROP_NCP_VERSION:
        handler = &NcpBase::HandlePropertyGet<SPINEL_PROP_NCP_VERSION>;
        break;
    case SPINEL_PROP_UNSOL_UPDATE_FILTER:
        handler = &NcpBase::HandlePropertyGet<SPINEL_PROP_UNSOL_UPDATE_FILTER>;
        break;
    case SPINEL_PROP_UNSOL_UPDATE_LIST:
        handler = &NcpBase::HandlePropertyGet<SPINEL_PROP_UNSOL_UPDATE_LIST>;
        break;
    case SPINEL_PROP_VENDOR_ID:
        handler = &NcpBase::HandlePropertyGet<SPINEL_PROP_VENDOR_ID>;
        break;

        // --------------------------------------------------------------------------
        // MTD (or FTD) Properties (Get Handler)

#if OPENTHREAD_MTD || OPENTHREAD_FTD
    case SPINEL_PROP_PHY_PCAP_ENABLED:
        handler = &NcpBase::HandlePropertyGet<SPINEL_PROP_PHY_PCAP_ENABLED>;
        break;
    case SPINEL_PROP_MAC_DATA_POLL_PERIOD:
        handler = &NcpBase::HandlePropertyGet<SPINEL_PROP_MAC_DATA_POLL_PERIOD>;
        break;
    case SPINEL_PROP_MAC_EXTENDED_ADDR:
        handler = &NcpBase::HandlePropertyGet<SPINEL_PROP_MAC_EXTENDED_ADDR>;
        break;
    case SPINEL_PROP_MAC_CCA_FAILURE_RATE:
        handler = &NcpBase::HandlePropertyGet<SPINEL_PROP_MAC_CCA_FAILURE_RATE>;
        break;
#if OPENTHREAD_ENABLE_MAC_FILTER
    case SPINEL_PROP_MAC_BLACKLIST:
        handler = &NcpBase::HandlePropertyGet<SPINEL_PROP_MAC_BLACKLIST>;
        break;
    case SPINEL_PROP_MAC_BLACKLIST_ENABLED:
        handler = &NcpBase::HandlePropertyGet<SPINEL_PROP_MAC_BLACKLIST_ENABLED>;
        break;
    case SPINEL_PROP_MAC_FIXED_RSS:
        handler = &NcpBase::HandlePropertyGet<SPINEL_PROP_MAC_FIXED_RSS>;
        break;
    case SPINEL_PROP_MAC_WHITELIST:
        handler = &NcpBase::HandlePropertyGet<SPINEL_PROP_MAC_WHITELIST>;
        break;
    case SPINEL_PROP_MAC_WHITELIST_ENABLED:
        handler = &NcpBase::HandlePropertyGet<SPINEL_PROP_MAC_WHITELIST_ENABLED>;
        break;
#endif
    case SPINEL_PROP_MSG_BUFFER_COUNTERS:
        handler = &NcpBase::HandlePropertyGet<SPINEL_PROP_MSG_BUFFER_COUNTERS>;
        break;
    case SPINEL_PROP_PHY_CHAN_SUPPORTED:
        handler = &NcpBase::HandlePropertyGet<SPINEL_PROP_PHY_CHAN_SUPPORTED>;
        break;
    case SPINEL_PROP_PHY_FREQ:
        handler = &NcpBase::HandlePropertyGet<SPINEL_PROP_PHY_FREQ>;
        break;
    case SPINEL_PROP_NET_IF_UP:
        handler = &NcpBase::HandlePropertyGet<SPINEL_PROP_NET_IF_UP>;
        break;
    case SPINEL_PROP_NET_KEY_SEQUENCE_COUNTER:
        handler = &NcpBase::HandlePropertyGet<SPINEL_PROP_NET_KEY_SEQUENCE_COUNTER>;
        break;
    case SPINEL_PROP_NET_KEY_SWITCH_GUARDTIME:
        handler = &NcpBase::HandlePropertyGet<SPINEL_PROP_NET_KEY_SWITCH_GUARDTIME>;
        break;
    case SPINEL_PROP_NET_MASTER_KEY:
        handler = &NcpBase::HandlePropertyGet<SPINEL_PROP_NET_MASTER_KEY>;
        break;
    case SPINEL_PROP_NET_NETWORK_NAME:
        handler = &NcpBase::HandlePropertyGet<SPINEL_PROP_NET_NETWORK_NAME>;
        break;
    case SPINEL_PROP_NET_PARTITION_ID:
        handler = &NcpBase::HandlePropertyGet<SPINEL_PROP_NET_PARTITION_ID>;
        break;
    case SPINEL_PROP_NET_REQUIRE_JOIN_EXISTING:
        handler = &NcpBase::HandlePropertyGet<SPINEL_PROP_NET_REQUIRE_JOIN_EXISTING>;
        break;
    case SPINEL_PROP_NET_ROLE:
        handler = &NcpBase::HandlePropertyGet<SPINEL_PROP_NET_ROLE>;
        break;
    case SPINEL_PROP_NET_SAVED:
        handler = &NcpBase::HandlePropertyGet<SPINEL_PROP_NET_SAVED>;
        break;
    case SPINEL_PROP_NET_STACK_UP:
        handler = &NcpBase::HandlePropertyGet<SPINEL_PROP_NET_STACK_UP>;
        break;
    case SPINEL_PROP_NET_XPANID:
        handler = &NcpBase::HandlePropertyGet<SPINEL_PROP_NET_XPANID>;
        break;
    case SPINEL_PROP_THREAD_ALLOW_LOCAL_NET_DATA_CHANGE:
        handler = &NcpBase::HandlePropertyGet<SPINEL_PROP_THREAD_ALLOW_LOCAL_NET_DATA_CHANGE>;
        break;
    case SPINEL_PROP_THREAD_ASSISTING_PORTS:
        handler = &NcpBase::HandlePropertyGet<SPINEL_PROP_THREAD_ASSISTING_PORTS>;
        break;
    case SPINEL_PROP_THREAD_CHILD_TIMEOUT:
        handler = &NcpBase::HandlePropertyGet<SPINEL_PROP_THREAD_CHILD_TIMEOUT>;
        break;
    case SPINEL_PROP_THREAD_DISCOVERY_SCAN_JOINER_FLAG:
        handler = &NcpBase::HandlePropertyGet<SPINEL_PROP_THREAD_DISCOVERY_SCAN_JOINER_FLAG>;
        break;
    case SPINEL_PROP_THREAD_DISCOVERY_SCAN_ENABLE_FILTERING:
        handler = &NcpBase::HandlePropertyGet<SPINEL_PROP_THREAD_DISCOVERY_SCAN_ENABLE_FILTERING>;
        break;
    case SPINEL_PROP_THREAD_DISCOVERY_SCAN_PANID:
        handler = &NcpBase::HandlePropertyGet<SPINEL_PROP_THREAD_DISCOVERY_SCAN_PANID>;
        break;
    case SPINEL_PROP_THREAD_LEADER_ADDR:
        handler = &NcpBase::HandlePropertyGet<SPINEL_PROP_THREAD_LEADER_ADDR>;
        break;
    case SPINEL_PROP_THREAD_LEADER_NETWORK_DATA:
        handler = &NcpBase::HandlePropertyGet<SPINEL_PROP_THREAD_LEADER_NETWORK_DATA>;
        break;
    case SPINEL_PROP_THREAD_LEADER_RID:
        handler = &NcpBase::HandlePropertyGet<SPINEL_PROP_THREAD_LEADER_RID>;
        break;
    case SPINEL_PROP_THREAD_MODE:
        handler = &NcpBase::HandlePropertyGet<SPINEL_PROP_THREAD_MODE>;
        break;
    case SPINEL_PROP_THREAD_NEIGHBOR_TABLE:
        handler = &NcpBase::HandlePropertyGet<SPINEL_PROP_THREAD_NEIGHBOR_TABLE>;
        break;
#if OPENTHREAD_CONFIG_ENABLE_TX_ERROR_RATE_TRACKING
    case SPINEL_PROP_THREAD_NEIGHBOR_TABLE_ERROR_RATES:
        handler = &NcpBase::HandlePropertyGet<SPINEL_PROP_THREAD_NEIGHBOR_TABLE_ERROR_RATES>;
        break;
#endif
    case SPINEL_PROP_THREAD_NETWORK_DATA_VERSION:
        handler = &NcpBase::HandlePropertyGet<SPINEL_PROP_THREAD_NETWORK_DATA_VERSION>;
        break;
    case SPINEL_PROP_THREAD_OFF_MESH_ROUTES:
        handler = &NcpBase::HandlePropertyGet<SPINEL_PROP_THREAD_OFF_MESH_ROUTES>;
        break;
    case SPINEL_PROP_THREAD_ON_MESH_NETS:
        handler = &NcpBase::HandlePropertyGet<SPINEL_PROP_THREAD_ON_MESH_NETS>;
        break;
    case SPINEL_PROP_THREAD_PARENT:
        handler = &NcpBase::HandlePropertyGet<SPINEL_PROP_THREAD_PARENT>;
        break;
    case SPINEL_PROP_THREAD_RLOC16:
        handler = &NcpBase::HandlePropertyGet<SPINEL_PROP_THREAD_RLOC16>;
        break;
    case SPINEL_PROP_THREAD_RLOC16_DEBUG_PASSTHRU:
        handler = &NcpBase::HandlePropertyGet<SPINEL_PROP_THREAD_RLOC16_DEBUG_PASSTHRU>;
        break;
    case SPINEL_PROP_THREAD_STABLE_LEADER_NETWORK_DATA:
        handler = &NcpBase::HandlePropertyGet<SPINEL_PROP_THREAD_STABLE_LEADER_NETWORK_DATA>;
        break;
    case SPINEL_PROP_THREAD_STABLE_NETWORK_DATA_VERSION:
        handler = &NcpBase::HandlePropertyGet<SPINEL_PROP_THREAD_STABLE_NETWORK_DATA_VERSION>;
        break;
#if OPENTHREAD_ENABLE_BORDER_ROUTER
    case SPINEL_PROP_THREAD_NETWORK_DATA:
        handler = &NcpBase::HandlePropertyGet<SPINEL_PROP_THREAD_NETWORK_DATA>;
        break;
    case SPINEL_PROP_THREAD_STABLE_NETWORK_DATA:
        handler = &NcpBase::HandlePropertyGet<SPINEL_PROP_THREAD_STABLE_NETWORK_DATA>;
        break;
#endif
    case SPINEL_PROP_THREAD_ACTIVE_DATASET:
        handler = &NcpBase::HandlePropertyGet<SPINEL_PROP_THREAD_ACTIVE_DATASET>;
        break;
    case SPINEL_PROP_THREAD_PENDING_DATASET:
        handler = &NcpBase::HandlePropertyGet<SPINEL_PROP_THREAD_PENDING_DATASET>;
        break;
    case SPINEL_PROP_IPV6_ADDRESS_TABLE:
        handler = &NcpBase::HandlePropertyGet<SPINEL_PROP_IPV6_ADDRESS_TABLE>;
        break;
    case SPINEL_PROP_IPV6_ICMP_PING_OFFLOAD:
        handler = &NcpBase::HandlePropertyGet<SPINEL_PROP_IPV6_ICMP_PING_OFFLOAD>;
        break;
    case SPINEL_PROP_IPV6_ICMP_PING_OFFLOAD_MODE:
        handler = &NcpBase::HandlePropertyGet<SPINEL_PROP_IPV6_ICMP_PING_OFFLOAD_MODE>;
        break;
    case SPINEL_PROP_IPV6_LL_ADDR:
        handler = &NcpBase::HandlePropertyGet<SPINEL_PROP_IPV6_LL_ADDR>;
        break;
    case SPINEL_PROP_IPV6_ML_PREFIX:
        handler = &NcpBase::HandlePropertyGet<SPINEL_PROP_IPV6_ML_PREFIX>;
        break;
    case SPINEL_PROP_IPV6_ML_ADDR:
        handler = &NcpBase::HandlePropertyGet<SPINEL_PROP_IPV6_ML_ADDR>;
        break;
    case SPINEL_PROP_IPV6_MULTICAST_ADDRESS_TABLE:
        handler = &NcpBase::HandlePropertyGet<SPINEL_PROP_IPV6_MULTICAST_ADDRESS_TABLE>;
        break;
    case SPINEL_PROP_IPV6_ROUTE_TABLE:
        handler = &NcpBase::HandlePropertyGet<SPINEL_PROP_IPV6_ROUTE_TABLE>;
        break;
#if OPENTHREAD_ENABLE_JAM_DETECTION
    case SPINEL_PROP_JAM_DETECT_ENABLE:
        handler = &NcpBase::HandlePropertyGet<SPINEL_PROP_JAM_DETECT_ENABLE>;
        break;
    case SPINEL_PROP_JAM_DETECTED:
        handler = &NcpBase::HandlePropertyGet<SPINEL_PROP_JAM_DETECTED>;
        break;
    case SPINEL_PROP_JAM_DETECT_RSSI_THRESHOLD:
        handler = &NcpBase::HandlePropertyGet<SPINEL_PROP_JAM_DETECT_RSSI_THRESHOLD>;
        break;
    case SPINEL_PROP_JAM_DETECT_WINDOW:
        handler = &NcpBase::HandlePropertyGet<SPINEL_PROP_JAM_DETECT_WINDOW>;
        break;
    case SPINEL_PROP_JAM_DETECT_BUSY:
        handler = &NcpBase::HandlePropertyGet<SPINEL_PROP_JAM_DETECT_BUSY>;
        break;
    case SPINEL_PROP_JAM_DETECT_HISTORY_BITMAP:
        handler = &NcpBase::HandlePropertyGet<SPINEL_PROP_JAM_DETECT_HISTORY_BITMAP>;
        break;
#endif
#if OPENTHREAD_ENABLE_CHANNEL_MONITOR
    case SPINEL_PROP_CHANNEL_MONITOR_SAMPLE_INTERVAL:
        handler = &NcpBase::HandlePropertyGet<SPINEL_PROP_CHANNEL_MONITOR_SAMPLE_INTERVAL>;
        break;
    case SPINEL_PROP_CHANNEL_MONITOR_RSSI_THRESHOLD:
        handler = &NcpBase::HandlePropertyGet<SPINEL_PROP_CHANNEL_MONITOR_RSSI_THRESHOLD>;
        break;
    case SPINEL_PROP_CHANNEL_MONITOR_SAMPLE_WINDOW:
        handler = &NcpBase::HandlePropertyGet<SPINEL_PROP_CHANNEL_MONITOR_SAMPLE_WINDOW>;
        break;
    case SPINEL_PROP_CHANNEL_MONITOR_SAMPLE_COUNT:
        handler = &NcpBase::HandlePropertyGet<SPINEL_PROP_CHANNEL_MONITOR_SAMPLE_COUNT>;
        break;
    case SPINEL_PROP_CHANNEL_MONITOR_CHANNEL_OCCUPANCY:
        handler = &NcpBase::HandlePropertyGet<SPINEL_PROP_CHANNEL_MONITOR_CHANNEL_OCCUPANCY>;
        break;
#endif
#if OPENTHREAD_ENABLE_LEGACY
    case SPINEL_PROP_NEST_LEGACY_ULA_PREFIX:
        handler = &NcpBase::HandlePropertyGet<SPINEL_PROP_NEST_LEGACY_ULA_PREFIX>;
        break;
    case SPINEL_PROP_NEST_LEGACY_LAST_NODE_JOINED:
        handler = &NcpBase::HandlePropertyGet<SPINEL_PROP_NEST_LEGACY_LAST_NODE_JOINED>;
        break;
#endif
        // MAC counters
    case SPINEL_PROP_CNTR_TX_PKT_TOTAL:
        handler = &NcpBase::HandlePropertyGet<SPINEL_PROP_CNTR_TX_PKT_TOTAL>;
        break;
    case SPINEL_PROP_CNTR_TX_PKT_ACK_REQ:
        handler = &NcpBase::HandlePropertyGet<SPINEL_PROP_CNTR_TX_PKT_ACK_REQ>;
        break;
    case SPINEL_PROP_CNTR_TX_PKT_ACKED:
        handler = &NcpBase::HandlePropertyGet<SPINEL_PROP_CNTR_TX_PKT_ACKED>;
        break;
    case SPINEL_PROP_CNTR_TX_PKT_NO_ACK_REQ:
        handler = &NcpBase::HandlePropertyGet<SPINEL_PROP_CNTR_TX_PKT_NO_ACK_REQ>;
        break;
    case SPINEL_PROP_CNTR_TX_PKT_DATA:
        handler = &NcpBase::HandlePropertyGet<SPINEL_PROP_CNTR_TX_PKT_DATA>;
        break;
    case SPINEL_PROP_CNTR_TX_PKT_DATA_POLL:
        handler = &NcpBase::HandlePropertyGet<SPINEL_PROP_CNTR_TX_PKT_DATA_POLL>;
        break;
    case SPINEL_PROP_CNTR_TX_PKT_BEACON:
        handler = &NcpBase::HandlePropertyGet<SPINEL_PROP_CNTR_TX_PKT_BEACON>;
        break;
    case SPINEL_PROP_CNTR_TX_PKT_BEACON_REQ:
        handler = &NcpBase::HandlePropertyGet<SPINEL_PROP_CNTR_TX_PKT_BEACON_REQ>;
        break;
    case SPINEL_PROP_CNTR_TX_PKT_OTHER:
        handler = &NcpBase::HandlePropertyGet<SPINEL_PROP_CNTR_TX_PKT_OTHER>;
        break;
    case SPINEL_PROP_CNTR_TX_PKT_RETRY:
        handler = &NcpBase::HandlePropertyGet<SPINEL_PROP_CNTR_TX_PKT_RETRY>;
        break;
    case SPINEL_PROP_CNTR_TX_PKT_UNICAST:
        handler = &NcpBase::HandlePropertyGet<SPINEL_PROP_CNTR_TX_PKT_UNICAST>;
        break;
    case SPINEL_PROP_CNTR_TX_PKT_BROADCAST:
        handler = &NcpBase::HandlePropertyGet<SPINEL_PROP_CNTR_TX_PKT_BROADCAST>;
        break;
    case SPINEL_PROP_CNTR_TX_ERR_CCA:
        handler = &NcpBase::HandlePropertyGet<SPINEL_PROP_CNTR_TX_ERR_CCA>;
        break;
    case SPINEL_PROP_CNTR_TX_ERR_ABORT:
        handler = &NcpBase::HandlePropertyGet<SPINEL_PROP_CNTR_TX_ERR_ABORT>;
        break;
    case SPINEL_PROP_CNTR_RX_PKT_TOTAL:
        handler = &NcpBase::HandlePropertyGet<SPINEL_PROP_CNTR_RX_PKT_TOTAL>;
        break;
    case SPINEL_PROP_CNTR_RX_PKT_DATA:
        handler = &NcpBase::HandlePropertyGet<SPINEL_PROP_CNTR_RX_PKT_DATA>;
        break;
    case SPINEL_PROP_CNTR_RX_PKT_DATA_POLL:
        handler = &NcpBase::HandlePropertyGet<SPINEL_PROP_CNTR_RX_PKT_DATA_POLL>;
        break;
    case SPINEL_PROP_CNTR_RX_PKT_BEACON:
        handler = &NcpBase::HandlePropertyGet<SPINEL_PROP_CNTR_RX_PKT_BEACON>;
        break;
    case SPINEL_PROP_CNTR_RX_PKT_BEACON_REQ:
        handler = &NcpBase::HandlePropertyGet<SPINEL_PROP_CNTR_RX_PKT_BEACON_REQ>;
        break;
    case SPINEL_PROP_CNTR_RX_PKT_OTHER:
        handler = &NcpBase::HandlePropertyGet<SPINEL_PROP_CNTR_RX_PKT_OTHER>;
        break;
    case SPINEL_PROP_CNTR_RX_PKT_FILT_WL:
        handler = &NcpBase::HandlePropertyGet<SPINEL_PROP_CNTR_RX_PKT_FILT_WL>;
        break;
    case SPINEL_PROP_CNTR_RX_PKT_FILT_DA:
        handler = &NcpBase::HandlePropertyGet<SPINEL_PROP_CNTR_RX_PKT_FILT_DA>;
        break;
    case SPINEL_PROP_CNTR_RX_PKT_UNICAST:
        handler = &NcpBase::HandlePropertyGet<SPINEL_PROP_CNTR_RX_PKT_UNICAST>;
        break;
    case SPINEL_PROP_CNTR_RX_PKT_BROADCAST:
        handler = &NcpBase::HandlePropertyGet<SPINEL_PROP_CNTR_RX_PKT_BROADCAST>;
        break;
    case SPINEL_PROP_CNTR_RX_ERR_EMPTY:
        handler = &NcpBase::HandlePropertyGet<SPINEL_PROP_CNTR_RX_ERR_EMPTY>;
        break;
    case SPINEL_PROP_CNTR_RX_ERR_UKWN_NBR:
        handler = &NcpBase::HandlePropertyGet<SPINEL_PROP_CNTR_RX_ERR_UKWN_NBR>;
        break;
    case SPINEL_PROP_CNTR_RX_ERR_NVLD_SADDR:
        handler = &NcpBase::HandlePropertyGet<SPINEL_PROP_CNTR_RX_ERR_NVLD_SADDR>;
        break;
    case SPINEL_PROP_CNTR_RX_ERR_SECURITY:
        handler = &NcpBase::HandlePropertyGet<SPINEL_PROP_CNTR_RX_ERR_SECURITY>;
        break;
    case SPINEL_PROP_CNTR_RX_ERR_BAD_FCS:
        handler = &NcpBase::HandlePropertyGet<SPINEL_PROP_CNTR_RX_ERR_BAD_FCS>;
        break;
    case SPINEL_PROP_CNTR_RX_ERR_OTHER:
        handler = &NcpBase::HandlePropertyGet<SPINEL_PROP_CNTR_RX_ERR_OTHER>;
        break;
    case SPINEL_PROP_CNTR_RX_PKT_DUP:
        handler = &NcpBase::HandlePropertyGet<SPINEL_PROP_CNTR_RX_PKT_DUP>;
        break;
    case SPINEL_PROP_CNTR_ALL_MAC_COUNTERS:
        handler = &NcpBase::HandlePropertyGet<SPINEL_PROP_CNTR_ALL_MAC_COUNTERS>;
        break;
        // NCP counters
    case SPINEL_PROP_CNTR_TX_IP_SEC_TOTAL:
        handler = &NcpBase::HandlePropertyGet<SPINEL_PROP_CNTR_TX_IP_SEC_TOTAL>;
        break;
    case SPINEL_PROP_CNTR_TX_IP_INSEC_TOTAL:
        handler = &NcpBase::HandlePropertyGet<SPINEL_PROP_CNTR_TX_IP_INSEC_TOTAL>;
        break;
    case SPINEL_PROP_CNTR_TX_IP_DROPPED:
        handler = &NcpBase::HandlePropertyGet<SPINEL_PROP_CNTR_TX_IP_DROPPED>;
        break;
    case SPINEL_PROP_CNTR_RX_IP_SEC_TOTAL:
        handler = &NcpBase::HandlePropertyGet<SPINEL_PROP_CNTR_RX_IP_SEC_TOTAL>;
        break;
    case SPINEL_PROP_CNTR_RX_IP_INSEC_TOTAL:
        handler = &NcpBase::HandlePropertyGet<SPINEL_PROP_CNTR_RX_IP_INSEC_TOTAL>;
        break;
    case SPINEL_PROP_CNTR_RX_IP_DROPPED:
        handler = &NcpBase::HandlePropertyGet<SPINEL_PROP_CNTR_RX_IP_DROPPED>;
        break;
    case SPINEL_PROP_CNTR_TX_SPINEL_TOTAL:
        handler = &NcpBase::HandlePropertyGet<SPINEL_PROP_CNTR_TX_SPINEL_TOTAL>;
        break;
    case SPINEL_PROP_CNTR_RX_SPINEL_TOTAL:
        handler = &NcpBase::HandlePropertyGet<SPINEL_PROP_CNTR_RX_SPINEL_TOTAL>;
        break;
    case SPINEL_PROP_CNTR_RX_SPINEL_ERR:
        handler = &NcpBase::HandlePropertyGet<SPINEL_PROP_CNTR_RX_SPINEL_ERR>;
        break;
        // IP counters
    case SPINEL_PROP_CNTR_IP_TX_SUCCESS:
        handler = &NcpBase::HandlePropertyGet<SPINEL_PROP_CNTR_IP_TX_SUCCESS>;
        break;
    case SPINEL_PROP_CNTR_IP_RX_SUCCESS:
        handler = &NcpBase::HandlePropertyGet<SPINEL_PROP_CNTR_IP_RX_SUCCESS>;
        break;
    case SPINEL_PROP_CNTR_IP_TX_FAILURE:
        handler = &NcpBase::HandlePropertyGet<SPINEL_PROP_CNTR_IP_TX_FAILURE>;
        break;
    case SPINEL_PROP_CNTR_IP_RX_FAILURE:
        handler = &NcpBase::HandlePropertyGet<SPINEL_PROP_CNTR_IP_RX_FAILURE>;
        break;
#if OPENTHREAD_CONFIG_ENABLE_TIME_SYNC
    case SPINEL_PROP_THREAD_NETWORK_TIME:
        handler = &NcpBase::HandlePropertyGet<SPINEL_PROP_THREAD_NETWORK_TIME>;
        break;
#endif
#if OPENTHREAD_ENABLE_CHILD_SUPERVISION
    case SPINEL_PROP_CHILD_SUPERVISION_CHECK_TIMEOUT:
        handler = &NcpBase::HandlePropertyGet<SPINEL_PROP_CHILD_SUPERVISION_CHECK_TIMEOUT>;
        break;
#endif
#if OPENTHREAD_ENABLE_POSIX_APP
    case SPINEL_PROP_RCP_VERSION:
        handler = &NcpBase::HandlePropertyGet<SPINEL_PROP_RCP_VERSION>;
        break;
#endif
#endif // OPENTHREAD_MTD || OPENTHREAD_FTD

        // --------------------------------------------------------------------------
        // FTD Only Properties (Get Handler)

#if OPENTHREAD_FTD
    case SPINEL_PROP_NET_PSKC:
        handler = &NcpBase::HandlePropertyGet<SPINEL_PROP_NET_PSKC>;
        break;
    case SPINEL_PROP_THREAD_LEADER_WEIGHT:
        handler = &NcpBase::HandlePropertyGet<SPINEL_PROP_THREAD_LEADER_WEIGHT>;
        break;
    case SPINEL_PROP_THREAD_CHILD_TABLE:
        handler = &NcpBase::HandlePropertyGet<SPINEL_PROP_THREAD_CHILD_TABLE>;
        break;
    case SPINEL_PROP_THREAD_CHILD_TABLE_ADDRESSES:
        handler = &NcpBase::HandlePropertyGet<SPINEL_PROP_THREAD_CHILD_TABLE_ADDRESSES>;
        break;
    case SPINEL_PROP_THREAD_ROUTER_TABLE:
        handler = &NcpBase::HandlePropertyGet<SPINEL_PROP_THREAD_ROUTER_TABLE>;
        break;
    case SPINEL_PROP_THREAD_LOCAL_LEADER_WEIGHT:
        handler = &NcpBase::HandlePropertyGet<SPINEL_PROP_THREAD_LOCAL_LEADER_WEIGHT>;
        break;
    case SPINEL_PROP_THREAD_ROUTER_ROLE_ENABLED:
        handler = &NcpBase::HandlePropertyGet<SPINEL_PROP_THREAD_ROUTER_ROLE_ENABLED>;
        break;
    case SPINEL_PROP_THREAD_CHILD_COUNT_MAX:
        handler = &NcpBase::HandlePropertyGet<SPINEL_PROP_THREAD_CHILD_COUNT_MAX>;
        break;
    case SPINEL_PROP_THREAD_ROUTER_UPGRADE_THRESHOLD:
        handler = &NcpBase::HandlePropertyGet<SPINEL_PROP_THREAD_ROUTER_UPGRADE_THRESHOLD>;
        break;
    case SPINEL_PROP_THREAD_ROUTER_DOWNGRADE_THRESHOLD:
        handler = &NcpBase::HandlePropertyGet<SPINEL_PROP_THREAD_ROUTER_DOWNGRADE_THRESHOLD>;
        break;
    case SPINEL_PROP_THREAD_CONTEXT_REUSE_DELAY:
        handler = &NcpBase::HandlePropertyGet<SPINEL_PROP_THREAD_CONTEXT_REUSE_DELAY>;
        break;
    case SPINEL_PROP_THREAD_NETWORK_ID_TIMEOUT:
        handler = &NcpBase::HandlePropertyGet<SPINEL_PROP_THREAD_NETWORK_ID_TIMEOUT>;
        break;
    case SPINEL_PROP_THREAD_ROUTER_SELECTION_JITTER:
        handler = &NcpBase::HandlePropertyGet<SPINEL_PROP_THREAD_ROUTER_SELECTION_JITTER>;
        break;
    case SPINEL_PROP_THREAD_PREFERRED_ROUTER_ID:
        handler = &NcpBase::HandlePropertyGet<SPINEL_PROP_THREAD_PREFERRED_ROUTER_ID>;
        break;
    case SPINEL_PROP_THREAD_ADDRESS_CACHE_TABLE:
        handler = &NcpBase::HandlePropertyGet<SPINEL_PROP_THREAD_ADDRESS_CACHE_TABLE>;
        break;
#if OPENTHREAD_ENABLE_JOINER
    case SPINEL_PROP_MESHCOP_JOINER_STATE:
        handler = &NcpBase::HandlePropertyGet<SPINEL_PROP_MESHCOP_JOINER_STATE>;
        break;
#endif
#if OPENTHREAD_ENABLE_COMMISSIONER
    case SPINEL_PROP_MESHCOP_COMMISSIONER_STATE:
        handler = &NcpBase::HandlePropertyGet<SPINEL_PROP_MESHCOP_COMMISSIONER_STATE>;
        break;
    case SPINEL_PROP_MESHCOP_COMMISSIONER_PROVISIONING_URL:
        handler = &NcpBase::HandlePropertyGet<SPINEL_PROP_MESHCOP_COMMISSIONER_PROVISIONING_URL>;
        break;
    case SPINEL_PROP_MESHCOP_COMMISSIONER_SESSION_ID:
        handler = &NcpBase::HandlePropertyGet<SPINEL_PROP_MESHCOP_COMMISSIONER_SESSION_ID>;
        break;
    case SPINEL_PROP_THREAD_COMMISSIONER_ENABLED:
        handler = &NcpBase::HandlePropertyGet<SPINEL_PROP_THREAD_COMMISSIONER_ENABLED>;
        break;
#endif
#if OPENTHREAD_CONFIG_ENABLE_STEERING_DATA_SET_OOB
    case SPINEL_PROP_THREAD_STEERING_DATA:
        handler = &NcpBase::HandlePropertyGet<SPINEL_PROP_THREAD_STEERING_DATA>;
        break;
#endif
#if OPENTHREAD_ENABLE_CHILD_SUPERVISION
    case SPINEL_PROP_CHILD_SUPERVISION_INTERVAL:
        handler = &NcpBase::HandlePropertyGet<SPINEL_PROP_CHILD_SUPERVISION_INTERVAL>;
        break;
#endif
#if OPENTHREAD_ENABLE_CHANNEL_MANAGER
    case SPINEL_PROP_CHANNEL_MANAGER_NEW_CHANNEL:
        handler = &NcpBase::HandlePropertyGet<SPINEL_PROP_CHANNEL_MANAGER_NEW_CHANNEL>;
        break;
    case SPINEL_PROP_CHANNEL_MANAGER_DELAY:
        handler = &NcpBase::HandlePropertyGet<SPINEL_PROP_CHANNEL_MANAGER_DELAY>;
        break;
    case SPINEL_PROP_CHANNEL_MANAGER_SUPPORTED_CHANNELS:
        handler = &NcpBase::HandlePropertyGet<SPINEL_PROP_CHANNEL_MANAGER_SUPPORTED_CHANNELS>;
        break;
    case SPINEL_PROP_CHANNEL_MANAGER_FAVORED_CHANNELS:
        handler = &NcpBase::HandlePropertyGet<SPINEL_PROP_CHANNEL_MANAGER_FAVORED_CHANNELS>;
        break;
    case SPINEL_PROP_CHANNEL_MANAGER_CHANNEL_SELECT:
        handler = &NcpBase::HandlePropertyGet<SPINEL_PROP_CHANNEL_MANAGER_CHANNEL_SELECT>;
        break;
    case SPINEL_PROP_CHANNEL_MANAGER_AUTO_SELECT_ENABLED:
        handler = &NcpBase::HandlePropertyGet<SPINEL_PROP_CHANNEL_MANAGER_AUTO_SELECT_ENABLED>;
        break;
    case SPINEL_PROP_CHANNEL_MANAGER_AUTO_SELECT_INTERVAL:
        handler = &NcpBase::HandlePropertyGet<SPINEL_PROP_CHANNEL_MANAGER_AUTO_SELECT_INTERVAL>;
        break;
#endif
#if OPENTHREAD_CONFIG_ENABLE_TIME_SYNC
    case SPINEL_PROP_TIME_SYNC_PERIOD:
        handler = &NcpBase::HandlePropertyGet<SPINEL_PROP_TIME_SYNC_PERIOD>;
        break;
    case SPINEL_PROP_TIME_SYNC_XTAL_THRESHOLD:
        handler = &NcpBase::HandlePropertyGet<SPINEL_PROP_TIME_SYNC_XTAL_THRESHOLD>;
        break;
#endif // OPENTHREAD_CONFIG_ENABLE_TIME_SYNC
#endif // OPENTHREAD_FTD

        // --------------------------------------------------------------------------
        // Raw Link or Radio Mode Properties (Get Handler)

#if OPENTHREAD_RADIO || OPENTHREAD_ENABLE_RAW_LINK_API
    case SPINEL_PROP_RADIO_CAPS:
        handler = &NcpBase::HandlePropertyGet<SPINEL_PROP_RADIO_CAPS>;
        break;
    case SPINEL_PROP_MAC_SRC_MATCH_ENABLED:
        handler = &NcpBase::HandlePropertyGet<SPINEL_PROP_MAC_SRC_MATCH_ENABLED>;
        break;
#endif

    default:
        handler = NULL;
    }

    return handler;
}

NcpBase::PropertyHandler NcpBase::FindSetPropertyHandler(spinel_prop_key_t aKey)
{
    NcpBase::PropertyHandler handler;

    switch (aKey)
    {
        // --------------------------------------------------------------------------
        // Common Properties (Set Handler)

    case SPINEL_PROP_POWER_STATE:
        handler = &NcpBase::HandlePropertySet<SPINEL_PROP_POWER_STATE>;
        break;
    case SPINEL_PROP_MCU_POWER_STATE:
        handler = &NcpBase::HandlePropertySet<SPINEL_PROP_MCU_POWER_STATE>;
        break;
    case SPINEL_PROP_UNSOL_UPDATE_FILTER:
        handler = &NcpBase::HandlePropertySet<SPINEL_PROP_UNSOL_UPDATE_FILTER>;
        break;
    case SPINEL_PROP_PHY_TX_POWER:
        handler = &NcpBase::HandlePropertySet<SPINEL_PROP_PHY_TX_POWER>;
        break;
    case SPINEL_PROP_PHY_CHAN:
        handler = &NcpBase::HandlePropertySet<SPINEL_PROP_PHY_CHAN>;
        break;
    case SPINEL_PROP_MAC_PROMISCUOUS_MODE:
        handler = &NcpBase::HandlePropertySet<SPINEL_PROP_MAC_PROMISCUOUS_MODE>;
        break;
    case SPINEL_PROP_MAC_15_4_PANID:
        handler = &NcpBase::HandlePropertySet<SPINEL_PROP_MAC_15_4_PANID>;
        break;
    case SPINEL_PROP_MAC_15_4_LADDR:
        handler = &NcpBase::HandlePropertySet<SPINEL_PROP_MAC_15_4_LADDR>;
        break;
    case SPINEL_PROP_MAC_RAW_STREAM_ENABLED:
        handler = &NcpBase::HandlePropertySet<SPINEL_PROP_MAC_RAW_STREAM_ENABLED>;
        break;
    case SPINEL_PROP_MAC_SCAN_MASK:
        handler = &NcpBase::HandlePropertySet<SPINEL_PROP_MAC_SCAN_MASK>;
        break;
    case SPINEL_PROP_MAC_SCAN_STATE:
        handler = &NcpBase::HandlePropertySet<SPINEL_PROP_MAC_SCAN_STATE>;
        break;
    case SPINEL_PROP_MAC_SCAN_PERIOD:
        handler = &NcpBase::HandlePropertySet<SPINEL_PROP_MAC_SCAN_PERIOD>;
        break;

        // --------------------------------------------------------------------------
        // MTD (or FTD) Properties (Set Handler)

#if OPENTHREAD_MTD || OPENTHREAD_FTD
    case SPINEL_PROP_PHY_PCAP_ENABLED:
        handler = &NcpBase::HandlePropertySet<SPINEL_PROP_PHY_PCAP_ENABLED>;
        break;
    case SPINEL_PROP_PHY_CHAN_SUPPORTED:
        handler = &NcpBase::HandlePropertySet<SPINEL_PROP_PHY_CHAN_SUPPORTED>;
        break;
    case SPINEL_PROP_MAC_DATA_POLL_PERIOD:
        handler = &NcpBase::HandlePropertySet<SPINEL_PROP_MAC_DATA_POLL_PERIOD>;
        break;
    case SPINEL_PROP_NET_IF_UP:
        handler = &NcpBase::HandlePropertySet<SPINEL_PROP_NET_IF_UP>;
        break;
    case SPINEL_PROP_NET_STACK_UP:
        handler = &NcpBase::HandlePropertySet<SPINEL_PROP_NET_STACK_UP>;
        break;
    case SPINEL_PROP_NET_ROLE:
        handler = &NcpBase::HandlePropertySet<SPINEL_PROP_NET_ROLE>;
        break;
    case SPINEL_PROP_NET_NETWORK_NAME:
        handler = &NcpBase::HandlePropertySet<SPINEL_PROP_NET_NETWORK_NAME>;
        break;
    case SPINEL_PROP_NET_XPANID:
        handler = &NcpBase::HandlePropertySet<SPINEL_PROP_NET_XPANID>;
        break;
    case SPINEL_PROP_NET_MASTER_KEY:
        handler = &NcpBase::HandlePropertySet<SPINEL_PROP_NET_MASTER_KEY>;
        break;
    case SPINEL_PROP_NET_KEY_SEQUENCE_COUNTER:
        handler = &NcpBase::HandlePropertySet<SPINEL_PROP_NET_KEY_SEQUENCE_COUNTER>;
        break;
    case SPINEL_PROP_NET_KEY_SWITCH_GUARDTIME:
        handler = &NcpBase::HandlePropertySet<SPINEL_PROP_NET_KEY_SWITCH_GUARDTIME>;
        break;
    case SPINEL_PROP_THREAD_ASSISTING_PORTS:
        handler = &NcpBase::HandlePropertySet<SPINEL_PROP_THREAD_ASSISTING_PORTS>;
        break;
    case SPINEL_PROP_STREAM_NET_INSECURE:
        handler = &NcpBase::HandlePropertySet<SPINEL_PROP_STREAM_NET_INSECURE>;
        break;
    case SPINEL_PROP_STREAM_NET:
        handler = &NcpBase::HandlePropertySet<SPINEL_PROP_STREAM_NET>;
        break;
    case SPINEL_PROP_IPV6_ML_PREFIX:
        handler = &NcpBase::HandlePropertySet<SPINEL_PROP_IPV6_ML_PREFIX>;
        break;
    case SPINEL_PROP_IPV6_ICMP_PING_OFFLOAD:
        handler = &NcpBase::HandlePropertySet<SPINEL_PROP_IPV6_ICMP_PING_OFFLOAD>;
        break;
    case SPINEL_PROP_IPV6_ICMP_PING_OFFLOAD_MODE:
        handler = &NcpBase::HandlePropertySet<SPINEL_PROP_IPV6_ICMP_PING_OFFLOAD_MODE>;
        break;
    case SPINEL_PROP_THREAD_CHILD_TIMEOUT:
        handler = &NcpBase::HandlePropertySet<SPINEL_PROP_THREAD_CHILD_TIMEOUT>;
        break;
#if OPENTHREAD_ENABLE_JOINER
    case SPINEL_PROP_MESHCOP_JOINER_COMMISSIONING:
        handler = &NcpBase::HandlePropertySet<SPINEL_PROP_MESHCOP_JOINER_COMMISSIONING>;
        break;
#endif
    case SPINEL_PROP_THREAD_RLOC16_DEBUG_PASSTHRU:
        handler = &NcpBase::HandlePropertySet<SPINEL_PROP_THREAD_RLOC16_DEBUG_PASSTHRU>;
        break;
#if OPENTHREAD_ENABLE_MAC_FILTER
    case SPINEL_PROP_MAC_WHITELIST:
        handler = &NcpBase::HandlePropertySet<SPINEL_PROP_MAC_WHITELIST>;
        break;
    case SPINEL_PROP_MAC_WHITELIST_ENABLED:
        handler = &NcpBase::HandlePropertySet<SPINEL_PROP_MAC_WHITELIST_ENABLED>;
        break;
    case SPINEL_PROP_MAC_BLACKLIST:
        handler = &NcpBase::HandlePropertySet<SPINEL_PROP_MAC_BLACKLIST>;
        break;
    case SPINEL_PROP_MAC_BLACKLIST_ENABLED:
        handler = &NcpBase::HandlePropertySet<SPINEL_PROP_MAC_BLACKLIST_ENABLED>;
        break;
    case SPINEL_PROP_MAC_FIXED_RSS:
        handler = &NcpBase::HandlePropertySet<SPINEL_PROP_MAC_FIXED_RSS>;
        break;
#endif
    case SPINEL_PROP_THREAD_MODE:
        handler = &NcpBase::HandlePropertySet<SPINEL_PROP_THREAD_MODE>;
        break;
    case SPINEL_PROP_NET_REQUIRE_JOIN_EXISTING:
        handler = &NcpBase::HandlePropertySet<SPINEL_PROP_NET_REQUIRE_JOIN_EXISTING>;
        break;
    case SPINEL_PROP_DEBUG_NCP_LOG_LEVEL:
        handler = &NcpBase::HandlePropertySet<SPINEL_PROP_DEBUG_NCP_LOG_LEVEL>;
        break;
    case SPINEL_PROP_THREAD_DISCOVERY_SCAN_JOINER_FLAG:
        handler = &NcpBase::HandlePropertySet<SPINEL_PROP_THREAD_DISCOVERY_SCAN_JOINER_FLAG>;
        break;
    case SPINEL_PROP_THREAD_DISCOVERY_SCAN_ENABLE_FILTERING:
        handler = &NcpBase::HandlePropertySet<SPINEL_PROP_THREAD_DISCOVERY_SCAN_ENABLE_FILTERING>;
        break;
    case SPINEL_PROP_THREAD_DISCOVERY_SCAN_PANID:
        handler = &NcpBase::HandlePropertySet<SPINEL_PROP_THREAD_DISCOVERY_SCAN_PANID>;
        break;
#if OPENTHREAD_ENABLE_BORDER_ROUTER
    case SPINEL_PROP_THREAD_ALLOW_LOCAL_NET_DATA_CHANGE:
        handler = &NcpBase::HandlePropertySet<SPINEL_PROP_THREAD_ALLOW_LOCAL_NET_DATA_CHANGE>;
        break;
#endif

    case SPINEL_PROP_THREAD_ACTIVE_DATASET:
        handler = &NcpBase::HandlePropertySet<SPINEL_PROP_THREAD_ACTIVE_DATASET>;
        break;
    case SPINEL_PROP_THREAD_PENDING_DATASET:
        handler = &NcpBase::HandlePropertySet<SPINEL_PROP_THREAD_PENDING_DATASET>;
        break;
    case SPINEL_PROP_THREAD_MGMT_SET_ACTIVE_DATASET:
        handler = &NcpBase::HandlePropertySet<SPINEL_PROP_THREAD_MGMT_SET_ACTIVE_DATASET>;
        break;
    case SPINEL_PROP_THREAD_MGMT_SET_PENDING_DATASET:
        handler = &NcpBase::HandlePropertySet<SPINEL_PROP_THREAD_MGMT_SET_PENDING_DATASET>;
        break;
    case SPINEL_PROP_THREAD_MGMT_GET_ACTIVE_DATASET:
        handler = &NcpBase::HandlePropertySet<SPINEL_PROP_THREAD_MGMT_GET_ACTIVE_DATASET>;
        break;
    case SPINEL_PROP_THREAD_MGMT_GET_PENDING_DATASET:
        handler = &NcpBase::HandlePropertySet<SPINEL_PROP_THREAD_MGMT_GET_PENDING_DATASET>;
        break;

#if OPENTHREAD_ENABLE_JAM_DETECTION
    case SPINEL_PROP_JAM_DETECT_ENABLE:
        handler = &NcpBase::HandlePropertySet<SPINEL_PROP_JAM_DETECT_ENABLE>;
        break;
    case SPINEL_PROP_JAM_DETECT_RSSI_THRESHOLD:
        handler = &NcpBase::HandlePropertySet<SPINEL_PROP_JAM_DETECT_RSSI_THRESHOLD>;
        break;
    case SPINEL_PROP_JAM_DETECT_WINDOW:
        handler = &NcpBase::HandlePropertySet<SPINEL_PROP_JAM_DETECT_WINDOW>;
        break;
    case SPINEL_PROP_JAM_DETECT_BUSY:
        handler = &NcpBase::HandlePropertySet<SPINEL_PROP_JAM_DETECT_BUSY>;
        break;
#endif
#if OPENTHREAD_ENABLE_LEGACY
    case SPINEL_PROP_NEST_LEGACY_ULA_PREFIX:
        handler = &NcpBase::HandlePropertySet<SPINEL_PROP_NEST_LEGACY_ULA_PREFIX>;
        break;
#endif
    case SPINEL_PROP_CNTR_RESET:
        handler = &NcpBase::HandlePropertySet<SPINEL_PROP_CNTR_RESET>;
        break;
#if OPENTHREAD_ENABLE_CHILD_SUPERVISION
    case SPINEL_PROP_CHILD_SUPERVISION_CHECK_TIMEOUT:
        handler = &NcpBase::HandlePropertySet<SPINEL_PROP_CHILD_SUPERVISION_CHECK_TIMEOUT>;
        break;
#endif
#endif // OPENTHREAD_MTD || OPENTHREAD_FTD

        // --------------------------------------------------------------------------
        // FTD Only Properties (Set Handler)

#if OPENTHREAD_FTD
    case SPINEL_PROP_NET_PSKC:
        handler = &NcpBase::HandlePropertySet<SPINEL_PROP_NET_PSKC>;
        break;
    case SPINEL_PROP_THREAD_NETWORK_ID_TIMEOUT:
        handler = &NcpBase::HandlePropertySet<SPINEL_PROP_THREAD_NETWORK_ID_TIMEOUT>;
        break;
    case SPINEL_PROP_THREAD_LOCAL_LEADER_WEIGHT:
        handler = &NcpBase::HandlePropertySet<SPINEL_PROP_THREAD_LOCAL_LEADER_WEIGHT>;
        break;
    case SPINEL_PROP_THREAD_ROUTER_ROLE_ENABLED:
        handler = &NcpBase::HandlePropertySet<SPINEL_PROP_THREAD_ROUTER_ROLE_ENABLED>;
        break;
    case SPINEL_PROP_THREAD_CHILD_COUNT_MAX:
        handler = &NcpBase::HandlePropertySet<SPINEL_PROP_THREAD_CHILD_COUNT_MAX>;
        break;
    case SPINEL_PROP_THREAD_ROUTER_UPGRADE_THRESHOLD:
        handler = &NcpBase::HandlePropertySet<SPINEL_PROP_THREAD_ROUTER_UPGRADE_THRESHOLD>;
        break;
    case SPINEL_PROP_THREAD_ROUTER_DOWNGRADE_THRESHOLD:
        handler = &NcpBase::HandlePropertySet<SPINEL_PROP_THREAD_ROUTER_DOWNGRADE_THRESHOLD>;
        break;
    case SPINEL_PROP_THREAD_CONTEXT_REUSE_DELAY:
        handler = &NcpBase::HandlePropertySet<SPINEL_PROP_THREAD_CONTEXT_REUSE_DELAY>;
        break;
    case SPINEL_PROP_THREAD_ROUTER_SELECTION_JITTER:
        handler = &NcpBase::HandlePropertySet<SPINEL_PROP_THREAD_ROUTER_SELECTION_JITTER>;
        break;
    case SPINEL_PROP_THREAD_PREFERRED_ROUTER_ID:
        handler = &NcpBase::HandlePropertySet<SPINEL_PROP_THREAD_PREFERRED_ROUTER_ID>;
        break;
#if OPENTHREAD_CONFIG_ENABLE_STEERING_DATA_SET_OOB
    case SPINEL_PROP_THREAD_STEERING_DATA:
        handler = &NcpBase::HandlePropertySet<SPINEL_PROP_THREAD_STEERING_DATA>;
        break;
#endif
#if OPENTHREAD_ENABLE_UDP_FORWARD
    case SPINEL_PROP_THREAD_UDP_FORWARD_STREAM:
        handler = &NcpBase::HandlePropertySet<SPINEL_PROP_THREAD_UDP_FORWARD_STREAM>;
        break;
#endif
#if OPENTHREAD_ENABLE_CHILD_SUPERVISION
    case SPINEL_PROP_CHILD_SUPERVISION_INTERVAL:
        handler = &NcpBase::HandlePropertySet<SPINEL_PROP_CHILD_SUPERVISION_INTERVAL>;
        break;
#endif
#if OPENTHREAD_ENABLE_COMMISSIONER
    case SPINEL_PROP_MESHCOP_COMMISSIONER_STATE:
        handler = &NcpBase::HandlePropertySet<SPINEL_PROP_MESHCOP_COMMISSIONER_STATE>;
        break;
    case SPINEL_PROP_MESHCOP_COMMISSIONER_PROVISIONING_URL:
        handler = &NcpBase::HandlePropertySet<SPINEL_PROP_MESHCOP_COMMISSIONER_PROVISIONING_URL>;
        break;
    case SPINEL_PROP_MESHCOP_COMMISSIONER_ANNOUNCE_BEGIN:
        handler = &NcpBase::HandlePropertySet<SPINEL_PROP_MESHCOP_COMMISSIONER_ANNOUNCE_BEGIN>;
        break;
    case SPINEL_PROP_MESHCOP_COMMISSIONER_ENERGY_SCAN:
        handler = &NcpBase::HandlePropertySet<SPINEL_PROP_MESHCOP_COMMISSIONER_ENERGY_SCAN>;
        break;
    case SPINEL_PROP_MESHCOP_COMMISSIONER_PAN_ID_QUERY:
        handler = &NcpBase::HandlePropertySet<SPINEL_PROP_MESHCOP_COMMISSIONER_PAN_ID_QUERY>;
        break;
    case SPINEL_PROP_MESHCOP_COMMISSIONER_MGMT_GET:
        handler = &NcpBase::HandlePropertySet<SPINEL_PROP_MESHCOP_COMMISSIONER_MGMT_GET>;
        break;
    case SPINEL_PROP_MESHCOP_COMMISSIONER_MGMT_SET:
        handler = &NcpBase::HandlePropertySet<SPINEL_PROP_MESHCOP_COMMISSIONER_MGMT_SET>;
        break;
#endif
#if OPENTHREAD_ENABLE_CHANNEL_MANAGER
    case SPINEL_PROP_CHANNEL_MANAGER_NEW_CHANNEL:
        handler = &NcpBase::HandlePropertySet<SPINEL_PROP_CHANNEL_MANAGER_NEW_CHANNEL>;
        break;
    case SPINEL_PROP_CHANNEL_MANAGER_DELAY:
        handler = &NcpBase::HandlePropertySet<SPINEL_PROP_CHANNEL_MANAGER_DELAY>;
        break;
    case SPINEL_PROP_CHANNEL_MANAGER_SUPPORTED_CHANNELS:
        handler = &NcpBase::HandlePropertySet<SPINEL_PROP_CHANNEL_MANAGER_SUPPORTED_CHANNELS>;
        break;
    case SPINEL_PROP_CHANNEL_MANAGER_FAVORED_CHANNELS:
        handler = &NcpBase::HandlePropertySet<SPINEL_PROP_CHANNEL_MANAGER_FAVORED_CHANNELS>;
        break;
    case SPINEL_PROP_CHANNEL_MANAGER_CHANNEL_SELECT:
        handler = &NcpBase::HandlePropertySet<SPINEL_PROP_CHANNEL_MANAGER_CHANNEL_SELECT>;
        break;
    case SPINEL_PROP_CHANNEL_MANAGER_AUTO_SELECT_ENABLED:
        handler = &NcpBase::HandlePropertySet<SPINEL_PROP_CHANNEL_MANAGER_AUTO_SELECT_ENABLED>;
        break;
    case SPINEL_PROP_CHANNEL_MANAGER_AUTO_SELECT_INTERVAL:
        handler = &NcpBase::HandlePropertySet<SPINEL_PROP_CHANNEL_MANAGER_AUTO_SELECT_INTERVAL>;
        break;
#endif
#if OPENTHREAD_CONFIG_ENABLE_TIME_SYNC
    case SPINEL_PROP_TIME_SYNC_PERIOD:
        handler = &NcpBase::HandlePropertySet<SPINEL_PROP_TIME_SYNC_PERIOD>;
        break;
    case SPINEL_PROP_TIME_SYNC_XTAL_THRESHOLD:
        handler = &NcpBase::HandlePropertySet<SPINEL_PROP_TIME_SYNC_XTAL_THRESHOLD>;
        break;
#endif
#endif // #if OPENTHREAD_FTD

        // --------------------------------------------------------------------------
        // Raw Link API Properties (Set Handler)

#if OPENTHREAD_RADIO || OPENTHREAD_ENABLE_RAW_LINK_API
    case SPINEL_PROP_MAC_15_4_SADDR:
        handler = &NcpBase::HandlePropertySet<SPINEL_PROP_MAC_15_4_SADDR>;
        break;
    case SPINEL_PROP_MAC_SRC_MATCH_ENABLED:
        handler = &NcpBase::HandlePropertySet<SPINEL_PROP_MAC_SRC_MATCH_ENABLED>;
        break;
    case SPINEL_PROP_MAC_SRC_MATCH_SHORT_ADDRESSES:
        handler = &NcpBase::HandlePropertySet<SPINEL_PROP_MAC_SRC_MATCH_SHORT_ADDRESSES>;
        break;
    case SPINEL_PROP_MAC_SRC_MATCH_EXTENDED_ADDRESSES:
        handler = &NcpBase::HandlePropertySet<SPINEL_PROP_MAC_SRC_MATCH_EXTENDED_ADDRESSES>;
        break;
    case SPINEL_PROP_PHY_ENABLED:
        handler = &NcpBase::HandlePropertySet<SPINEL_PROP_PHY_ENABLED>;
        break;
#endif // #if OPENTHREAD_RADIO || OPENTHREAD_ENABLE_RAW_LINK_API

    default:
        handler = NULL;
    }

    return handler;
}

NcpBase::PropertyHandler NcpBase::FindInsertPropertyHandler(spinel_prop_key_t aKey)
{
    NcpBase::PropertyHandler handler;

    switch (aKey)
    {
        // --------------------------------------------------------------------------
        // Common Properties (Insert Handler)

    case SPINEL_PROP_UNSOL_UPDATE_FILTER:
        handler = &NcpBase::HandlePropertyInsert<SPINEL_PROP_UNSOL_UPDATE_FILTER>;
        break;

        // --------------------------------------------------------------------------
        // MTD (or FTD) Properties (Insert Handler)

#if OPENTHREAD_MTD || OPENTHREAD_FTD
    case SPINEL_PROP_IPV6_ADDRESS_TABLE:
        handler = &NcpBase::HandlePropertyInsert<SPINEL_PROP_IPV6_ADDRESS_TABLE>;
        break;
    case SPINEL_PROP_IPV6_MULTICAST_ADDRESS_TABLE:
        handler = &NcpBase::HandlePropertyInsert<SPINEL_PROP_IPV6_MULTICAST_ADDRESS_TABLE>;
        break;
    case SPINEL_PROP_THREAD_ASSISTING_PORTS:
        handler = &NcpBase::HandlePropertyInsert<SPINEL_PROP_THREAD_ASSISTING_PORTS>;
        break;
#if OPENTHREAD_ENABLE_BORDER_ROUTER
    case SPINEL_PROP_THREAD_OFF_MESH_ROUTES:
        handler = &NcpBase::HandlePropertyInsert<SPINEL_PROP_THREAD_OFF_MESH_ROUTES>;
        break;
    case SPINEL_PROP_THREAD_ON_MESH_NETS:
        handler = &NcpBase::HandlePropertyInsert<SPINEL_PROP_THREAD_ON_MESH_NETS>;
        break;
#endif
#if OPENTHREAD_ENABLE_MAC_FILTER
    case SPINEL_PROP_MAC_WHITELIST:
        handler = &NcpBase::HandlePropertyInsert<SPINEL_PROP_MAC_WHITELIST>;
        break;
    case SPINEL_PROP_MAC_BLACKLIST:
        handler = &NcpBase::HandlePropertyInsert<SPINEL_PROP_MAC_BLACKLIST>;
        break;
    case SPINEL_PROP_MAC_FIXED_RSS:
        handler = &NcpBase::HandlePropertyInsert<SPINEL_PROP_MAC_FIXED_RSS>;
        break;
#endif
#endif // OPENTHREAD_MTD || OPENTHREAD_FTD

        // --------------------------------------------------------------------------
        // FTD Only Properties (Insert Handler)

#if OPENTHREAD_FTD
#if OPENTHREAD_ENABLE_COMMISSIONER
    case SPINEL_PROP_MESHCOP_COMMISSIONER_JOINERS:
        handler = &NcpBase::HandlePropertyInsert<SPINEL_PROP_MESHCOP_COMMISSIONER_JOINERS>;
        break;
    case SPINEL_PROP_THREAD_JOINERS:
        handler = &NcpBase::HandlePropertyInsert<SPINEL_PROP_THREAD_JOINERS>;
        break;
#endif
#endif // OPENTHREAD_FTD

        // --------------------------------------------------------------------------
        // Raw Link API Properties (Insert Handler)

#if OPENTHREAD_RADIO || OPENTHREAD_ENABLE_RAW_LINK_API
    case SPINEL_PROP_MAC_SRC_MATCH_SHORT_ADDRESSES:
        handler = &NcpBase::HandlePropertyInsert<SPINEL_PROP_MAC_SRC_MATCH_SHORT_ADDRESSES>;
        break;
    case SPINEL_PROP_MAC_SRC_MATCH_EXTENDED_ADDRESSES:
        handler = &NcpBase::HandlePropertyInsert<SPINEL_PROP_MAC_SRC_MATCH_EXTENDED_ADDRESSES>;
        break;
#endif

    default:
        handler = NULL;
    }

    return handler;
}

NcpBase::PropertyHandler NcpBase::FindRemovePropertyHandler(spinel_prop_key_t aKey)
{
    NcpBase::PropertyHandler handler;

    switch (aKey)
    {
        // --------------------------------------------------------------------------
        // Common Properties (Remove Handler)

    case SPINEL_PROP_UNSOL_UPDATE_FILTER:
        handler = &NcpBase::HandlePropertyRemove<SPINEL_PROP_UNSOL_UPDATE_FILTER>;
        break;

        // --------------------------------------------------------------------------
        // MTD (or FTD) Properties (Remove Handler)

#if OPENTHREAD_MTD || OPENTHREAD_FTD
    case SPINEL_PROP_IPV6_ADDRESS_TABLE:
        handler = &NcpBase::HandlePropertyRemove<SPINEL_PROP_IPV6_ADDRESS_TABLE>;
        break;
    case SPINEL_PROP_IPV6_MULTICAST_ADDRESS_TABLE:
        handler = &NcpBase::HandlePropertyRemove<SPINEL_PROP_IPV6_MULTICAST_ADDRESS_TABLE>;
        break;
#if OPENTHREAD_ENABLE_BORDER_ROUTER
    case SPINEL_PROP_THREAD_OFF_MESH_ROUTES:
        handler = &NcpBase::HandlePropertyRemove<SPINEL_PROP_THREAD_OFF_MESH_ROUTES>;
        break;
    case SPINEL_PROP_THREAD_ON_MESH_NETS:
        handler = &NcpBase::HandlePropertyRemove<SPINEL_PROP_THREAD_ON_MESH_NETS>;
        break;
#endif
    case SPINEL_PROP_THREAD_ASSISTING_PORTS:
        handler = &NcpBase::HandlePropertyRemove<SPINEL_PROP_THREAD_ASSISTING_PORTS>;
        break;
#if OPENTHREAD_ENABLE_MAC_FILTER
    case SPINEL_PROP_MAC_WHITELIST:
        handler = &NcpBase::HandlePropertyRemove<SPINEL_PROP_MAC_WHITELIST>;
        break;
    case SPINEL_PROP_MAC_BLACKLIST:
        handler = &NcpBase::HandlePropertyRemove<SPINEL_PROP_MAC_BLACKLIST>;
        break;
    case SPINEL_PROP_MAC_FIXED_RSS:
        handler = &NcpBase::HandlePropertyRemove<SPINEL_PROP_MAC_FIXED_RSS>;
        break;
#endif
#endif // OPENTHREAD_MTD || OPENTHREAD_FTD

        // --------------------------------------------------------------------------
        // FTD Only Properties (Remove Handler)

#if OPENTHREAD_FTD
    case SPINEL_PROP_THREAD_ACTIVE_ROUTER_IDS:
        handler = &NcpBase::HandlePropertyRemove<SPINEL_PROP_THREAD_ACTIVE_ROUTER_IDS>;
        break;
#if OPENTHREAD_ENABLE_COMMISSIONER
    case SPINEL_PROP_MESHCOP_COMMISSIONER_JOINERS:
        handler = &NcpBase::HandlePropertyRemove<SPINEL_PROP_MESHCOP_COMMISSIONER_JOINERS>;
        break;
#endif
#endif // OPENTHREAD_FTD

        // --------------------------------------------------------------------------
        // Raw Link API Properties (Remove Handler)

#if OPENTHREAD_RADIO || OPENTHREAD_ENABLE_RAW_LINK_API
    case SPINEL_PROP_MAC_SRC_MATCH_SHORT_ADDRESSES:
        handler = &NcpBase::HandlePropertyRemove<SPINEL_PROP_MAC_SRC_MATCH_SHORT_ADDRESSES>;
        break;
    case SPINEL_PROP_MAC_SRC_MATCH_EXTENDED_ADDRESSES:
        handler = &NcpBase::HandlePropertyRemove<SPINEL_PROP_MAC_SRC_MATCH_EXTENDED_ADDRESSES>;
        break;
#endif

    default:
        handler = NULL;
    }

    return handler;
}

} // namespace Ncp
} // namespace ot
