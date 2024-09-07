/*
 *  Copyright (c) 2024, The OpenThread Authors.
 *  All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions are met:
 *  1. Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *  2. Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *  3. Neither the name of the copyright holder nor the
 *     names of its contributors may be used to endorse or promote products
 *     derived from this software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 *  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 *  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 *  ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 *  LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 *  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 *  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 *  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 *  CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 *  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 *  POSSIBILITY OF SUCH DAMAGE.
 */

/**
 * @file
 *   This file evaluates the condition of all the cli commands.
 */

#ifndef CMD_CONDITION_EVAL_H_
#define CMD_CONDITION_EVAL_H_

#include "openthread-core-config.h"

#if OPENTHREAD_FTD

#define CMD_child 1
#define CMD_childip 1
#define CMD_childmax 1
#define CMD_childrouterlinks 1
#define CMD_contextreusedelay 1
#define CMD_delaytimermin 1
#define CMD_eidcache 1
#define CMD_joinerport 1
#define CMD_leaderweight 1
#define CMD_neighbor 1
#define CMD_networkidtimeout 1
#define CMD_nexthop 1
#define CMD_parentpriority 1
#define CMD_partitionid 1
#define CMD_preferrouterid 1
#define CMD_pskc 1
#define CMD_releaserouterid 1
#define CMD_router 1
#define CMD_routerdowngradethreshold 1
#define CMD_routereligible 1
#define CMD_routerselectionjitter 1
#define CMD_routerupgradethreshold 1

#if OPENTHREAD_CONFIG_REFERENCE_DEVICE_ENABLE
#define CMD_ccm 1
#define CMD_fake 1
#define CMD_mleadvimax 1
#define CMD_routeridrange 1
#define CMD_test 1
#define CMD_tvcheck 1
#endif

#if OPENTHREAD_CONFIG_COMMISSIONER_ENABLE
#define CMD_commissioner 1
#endif

#if OPENTHREAD_CONFIG_MLE_DEVICE_PROPERTY_LEADER_WEIGHT_ENABLE
#define CMD_deviceprops 1
#endif

#if OPENTHREAD_CONFIG_MESH_DIAG_ENABLE
#define CMD_meshdiag 1
#endif

#if OPENTHREAD_CONFIG_TMF_PROXY_MLR_ENABLE && OPENTHREAD_CONFIG_COMMISSIONER_ENABLE
#define CMD_mlr 1
#endif

#if OPENTHREAD_CONFIG_PLATFORM_KEY_REFERENCES_ENABLE
#define CMD_pskcref 1
#endif

#endif // OPENTHREAD_FTD

#if OPENTHREAD_FTD || OPENTHREAD_MTD

#define CMD_bufferinfo 1
#define CMD_ccathreshold 1
#define CMD_channel 1
#define CMD_childsupervision 1
#define CMD_childtimeout 1
#define CMD_counters 1
#define CMD_dataset 1
#define CMD_debug 1
#define CMD_detach 1
#define CMD_discover 1
#define CMD_eui64 1
#define CMD_extaddr 1
#define CMD_extpanid 1
#define CMD_factoryreset 1
#define CMD_fem 1
#define CMD_ifconfig 1
#define CMD_instanceid 1
#define CMD_ipaddr 1
#define CMD_ipmaddr 1
#define CMD_keysequence 1
#define CMD_leaderdata 1
#define CMD_log 1
#define CMD_mac 1
#define CMD_mode 1
#define CMD_multiradio 1
#define CMD_netdata 1
#define CMD_netstat 1
#define CMD_networkkey 1
#define CMD_networkname 1
#define CMD_panid 1
#define CMD_parent 1
#define CMD_platform 1
#define CMD_pollperiod 1
#define CMD_promiscuous 1
#define CMD_rcp 1
#define CMD_region 1
#define CMD_rloc16 1
#define CMD_scan 1
#define CMD_singleton 1
#define CMD_state 1
#define CMD_thread 1
#define CMD_txpower 1
#define CMD_udp 1
#define CMD_unsecureport 1
#define CMD_vendor 1

#if (OPENTHREAD_CONFIG_THREAD_VERSION >= OT_THREAD_VERSION_1_2)
#define CMD_bbr 1
#define CMD_domainname 1
#endif

#if OPENTHREAD_CONFIG_REFERENCE_DEVICE_ENABLE
#define CMD_mliid 1
#if OPENTHREAD_CONFIG_DNS_CLIENT_ENABLE || OPENTHREAD_CONFIG_DNSSD_SERVER_ENABLE
#define CMD_dns 1
#endif
#endif

#if OPENTHREAD_CONFIG_BORDER_AGENT_ENABLE
#define CMD_ba 1
#endif

#if OPENTHREAD_CONFIG_BORDER_ROUTING_ENABLE
#define CMD_br 1
#endif

#if OPENTHREAD_CONFIG_COAP_API_ENABLE
#define CMD_coap 1
#endif

#if OPENTHREAD_CONFIG_COAP_SECURE_API_ENABLE
#define CMD_coaps 1
#endif

#if OPENTHREAD_CONFIG_PLATFORM_RADIO_COEX_ENABLE
#define CMD_coex 1
#endif

#if OPENTHREAD_CONFIG_MAC_CSL_RECEIVER_ENABLE
#define CMD_csl 1
#endif

#if OPENTHREAD_CONFIG_DUA_ENABLE
#define CMD_dua 1
#endif

#if OPENTHREAD_CONFIG_HISTORY_TRACKER_ENABLE
#define CMD_history 1
#endif

#if OPENTHREAD_CONFIG_JOINER_ENABLE
#define CMD_joiner 1
#endif

#if OPENTHREAD_CONFIG_MLE_LINK_METRICS_INITIATOR_ENABLE
#define CMD_linkmetrics 1
#endif

#if OPENTHREAD_CONFIG_LINK_METRICS_MANAGER_ENABLE
#define CMD_linkmetricsmgr 1
#endif

#if OPENTHREAD_CONFIG_TMF_ANYCAST_LOCATOR_ENABLE
#define CMD_locate 1
#endif

#if OPENTHREAD_CONFIG_MAC_FILTER_ENABLE
#define CMD_macfilter 1
#endif

#if OPENTHREAD_CONFIG_MULTICAST_DNS_ENABLE && OPENTHREAD_CONFIG_MULTICAST_DNS_PUBLIC_API_ENABLE
#define CMD_mdns 1
#endif

#if OPENTHREAD_CONFIG_NAT64_TRANSLATOR_ENABLE || OPENTHREAD_CONFIG_NAT64_BORDER_ROUTING_ENABLE
#define CMD_nat64 1
#endif

#if OPENTHREAD_CONFIG_TMF_NETDIAG_CLIENT_ENABLE
#define CMD_networkdiagnostic 1
#endif

#if OPENTHREAD_CONFIG_PLATFORM_KEY_REFERENCES_ENABLE
#define CMD_networkkeyref 1
#endif

#if OPENTHREAD_CONFIG_TIME_SYNC_ENABLE
#define CMD_networktime 1
#endif

#if OPENTHREAD_CONFIG_PING_SENDER_ENABLE
#define CMD_ping 1
#endif

#if OPENTHREAD_CONFIG_BORDER_ROUTER_ENABLE
#define CMD_prefix 1
#define CMD_route 1
#endif

#if OPENTHREAD_CONFIG_RADIO_STATS_ENABLE
#define CMD_radio 1
#endif

#if OPENTHREAD_CONFIG_MAC_FILTER_ENABLE && OPENTHREAD_CONFIG_RADIO_LINK_IEEE_802_15_4_ENABLE
#define CMD_radiofilter 1
#endif

#if OPENTHREAD_CONFIG_TMF_NETDATA_SERVICE_ENABLE
#define CMD_service 1
#endif

#if OPENTHREAD_CONFIG_SNTP_CLIENT_ENABLE
#define CMD_sntp 1
#endif

#if OPENTHREAD_CONFIG_SRP_CLIENT_ENABLE || OPENTHREAD_CONFIG_SRP_SERVER_ENABLE
#define CMD_srp 1
#endif

#if OPENTHREAD_CONFIG_BLE_TCAT_ENABLE && OPENTHREAD_CONFIG_CLI_BLE_SECURE_ENABLE
#define CMD_tcat 1
#endif

#if OPENTHREAD_CONFIG_TCP_ENABLE && OPENTHREAD_CONFIG_CLI_TCP_ENABLE
#define CMD_tcp 1
#endif

#if OPENTHREAD_CONFIG_TX_QUEUE_STATISTICS_ENABLE
#define CMD_timeinqueue 1
#endif

#if OPENTHREAD_CONFIG_RADIO_LINK_TREL_ENABLE
#define CMD_trel 1
#endif

#if OPENTHREAD_CONFIG_UPTIME_ENABLE
#define CMD_uptime 1
#endif

#if OPENTHREAD_CONFIG_VERHOEFF_CHECKSUM_ENABLE
#define CMD_verhoeff 1
#endif

#endif // OPENTHREAD_FTD || OPENTHREAD_MTD

#if OPENTHREAD_CONFIG_DIAG_ENABLE
#define CMD_diag 1
#endif
#define CMD_reset 1
#define CMD_version 1

#endif // CMD_CONDITION_EVAL_H_
