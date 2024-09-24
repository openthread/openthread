/*
 *  Copyright (c) 2019, The OpenThread Authors.
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

#ifndef OPENTHREAD_PLATFORM_POSIX_CONFIG_H_
#define OPENTHREAD_PLATFORM_POSIX_CONFIG_H_

#include "openthread-core-config.h"

#ifdef OPENTHREAD_POSIX_CONFIG_FILE
#include OPENTHREAD_POSIX_CONFIG_FILE
#endif

/**
 * @file
 * @brief
 *   This file includes the POSIX platform-specific configurations.
 */

/**
 * @def OPENTHREAD_POSIX_CONFIG_RCP_PTY_ENABLE
 *
 * Define as 1 to enable PTY RCP support.
 */
#ifndef OPENTHREAD_POSIX_CONFIG_RCP_PTY_ENABLE
#define OPENTHREAD_POSIX_CONFIG_RCP_PTY_ENABLE 1
#endif

/**
 * @def OPENTHREAD_POSIX_CONFIG_RCP_BUS
 *
 * This setting configures what type of RCP bus to use.
 */
#ifdef OPENTHREAD_POSIX_CONFIG_RCP_BUS
#error "OPENTHREAD_POSIX_CONFIG_RCP_BUS was replaced by OPENTHREAD_POSIX_CONFIG_SPINEL_HDLC_INTERFACE_ENABLE,"\
       "OPENTHREAD_POSIX_CONFIG_SPINEL_SPI_INTERFACE_ENABLE and OPENTHREAD_POSIX_CONFIG_SPINEL_VENDOR_INTERFACE_ENABLE"
#endif

/**
 * @def OPENTHREAD_POSIX_CONFIG_SPINEL_HDLC_INTERFACE_ENABLE
 *
 * Define as 1 to enable the spinel HDLC interface.
 */
#ifndef OPENTHREAD_POSIX_CONFIG_SPINEL_HDLC_INTERFACE_ENABLE
#define OPENTHREAD_POSIX_CONFIG_SPINEL_HDLC_INTERFACE_ENABLE 1
#endif

/**
 * @def OPENTHREAD_POSIX_CONFIG_SPINEL_SPI_INTERFACE_ENABLE
 *
 * Define as 1 to enable the spinel SPI interface.
 */
#ifndef OPENTHREAD_POSIX_CONFIG_SPINEL_SPI_INTERFACE_ENABLE
#define OPENTHREAD_POSIX_CONFIG_SPINEL_SPI_INTERFACE_ENABLE 0
#endif

/**
 * @def OPENTHREAD_POSIX_CONFIG_SPINEL_VENDOR_INTERFACE_ENABLE
 *
 * Define as 1 to enable the spinel vendor interface.
 */
#ifndef OPENTHREAD_POSIX_CONFIG_SPINEL_VENDOR_INTERFACE_ENABLE
#define OPENTHREAD_POSIX_CONFIG_SPINEL_VENDOR_INTERFACE_ENABLE 0
#endif

/**
 * @def OPENTHREAD_POSIX_CONFIG_SPINEL_VENDOR_INTERFACE_URL_PROTOCOL_NAME
 *
 * Define the URL protocol name of the vendor Spinel interface.
 */
#ifndef OPENTHREAD_POSIX_CONFIG_SPINEL_VENDOR_INTERFACE_URL_PROTOCOL_NAME
#define OPENTHREAD_POSIX_CONFIG_SPINEL_VENDOR_INTERFACE_URL_PROTOCOL_NAME "spinel+vendor"
#endif

/**
 * @def OPENTHREAD_POSIX_CONFIG_MAX_POWER_TABLE_ENABLE
 *
 * Define as 1 to enable max power table support.
 */
#ifndef OPENTHREAD_POSIX_CONFIG_MAX_POWER_TABLE_ENABLE
#define OPENTHREAD_POSIX_CONFIG_MAX_POWER_TABLE_ENABLE 0
#endif

/**
 * @def OPENTHREAD_POSIX_CONFIG_MAX_MULTICAST_FORWARDING_CACHE_TABLE
 *
 * This setting configures the maximum number of Multicast Forwarding Cache table for POSIX native multicast routing.
 */
#ifndef OPENTHREAD_POSIX_CONFIG_MAX_MULTICAST_FORWARDING_CACHE_TABLE
#define OPENTHREAD_POSIX_CONFIG_MAX_MULTICAST_FORWARDING_CACHE_TABLE (OPENTHREAD_CONFIG_MAX_MULTICAST_LISTENERS * 10)
#endif

/**
 * @def OPENTHREAD_POSIX_CONFIG_BACKBONE_ROUTER_MULTICAST_ROUTING_ENABLE
 *
 * Define as 1 to enable multicast routing support.
 */
#ifndef OPENTHREAD_POSIX_CONFIG_BACKBONE_ROUTER_MULTICAST_ROUTING_ENABLE
#define OPENTHREAD_POSIX_CONFIG_BACKBONE_ROUTER_MULTICAST_ROUTING_ENABLE \
    OPENTHREAD_CONFIG_BACKBONE_ROUTER_MULTICAST_ROUTING_ENABLE
#endif

#if OPENTHREAD_POSIX_CONFIG_BACKBONE_ROUTER_MULTICAST_ROUTING_ENABLE && \
    !OPENTHREAD_CONFIG_BACKBONE_ROUTER_MULTICAST_ROUTING_ENABLE
#error \
    "OPENTHREAD_CONFIG_BACKBONE_ROUTER_MULTICAST_ROUTING_ENABLE is required for OPENTHREAD_POSIX_CONFIG_BACKBONE_ROUTER_MULTICAST_ROUTING_ENABLE"
#endif

/**
 * @def OPENTHREAD_POSIX_CONFIG_SECURE_SETTINGS_ENABLE
 *
 * Define as 1 to enable the secure settings. When defined to 1, the platform MUST implement the otPosixSecureSetting*
 * APIs defined in 'src/posix/platform/include/openthread/platform/secure_settings.h'.
 */
#ifndef OPENTHREAD_POSIX_CONFIG_SECURE_SETTINGS_ENABLE
#define OPENTHREAD_POSIX_CONFIG_SECURE_SETTINGS_ENABLE 0
#endif

/**
 * @def OPENTHREAD_POSIX_CONFIG_NETIF_PREFIX_ROUTE_METRIC
 *
 * This setting configures the prefix route metric on the Thread network interface.
 * Define as 0 to use use the default prefix route metric.
 *
 * Note: The feature works on Linux kernel v4.18+.
 */
#ifndef OPENTHREAD_POSIX_CONFIG_NETIF_PREFIX_ROUTE_METRIC
#define OPENTHREAD_POSIX_CONFIG_NETIF_PREFIX_ROUTE_METRIC 0
#endif

/**
 * @def OPENTHREAD_POSIX_CONFIG_INSTALL_OMR_ROUTES_ENABLE
 *
 * Define as 1 to add OMR routes to POSIX kernel when OMR prefixes are changed in netdata.
 *
 * Note: This feature can be used to add OMR routes with non-default priority. Unlike
 * `OPENTHREAD_POSIX_CONFIG_NETIF_PREFIX_ROUTE_METRIC`, it works on Linux kernels before v4.18.
 * However, `OPENTHREAD_POSIX_CONFIG_NETIF_PREFIX_ROUTE_METRIC` should be preferred on Linux kernel v4.18+.
 */
#ifndef OPENTHREAD_POSIX_CONFIG_INSTALL_OMR_ROUTES_ENABLE
#define OPENTHREAD_POSIX_CONFIG_INSTALL_OMR_ROUTES_ENABLE 0
#endif

/**
 * @def OPENTHREAD_POSIX_CONFIG_OMR_ROUTES_PRIORITY
 *
 * Defines the priority of OMR routes added to kernel. The larger the number, the lower the priority. We
 * need to assign a high priority to such routes so that kernel prefers the Thread link rather than infrastructure.
 * Otherwise we may unnecessarily transmit packets via infrastructure, which potentially causes looping issue.
 */
#ifndef OPENTHREAD_POSIX_CONFIG_OMR_ROUTES_PRIORITY
#define OPENTHREAD_POSIX_CONFIG_OMR_ROUTES_PRIORITY 1
#endif

/**
 * @def OPENTHREAD_POSIX_CONFIG_MAX_OMR_ROUTES_NUM
 *
 * Defines the max number of OMR routes that can be added to kernel.
 */
#ifndef OPENTHREAD_POSIX_CONFIG_MAX_OMR_ROUTES_NUM
#define OPENTHREAD_POSIX_CONFIG_MAX_OMR_ROUTES_NUM OPENTHREAD_CONFIG_IP6_SLAAC_NUM_ADDRESSES
#endif

/**
 * @def OPENTHREAD_POSIX_CONFIG_INSTALL_EXTERNAL_ROUTES_ENABLE
 *
 * Define as 1 to add external routes to POSIX kernel when external routes are changed in netdata.
 */
#ifndef OPENTHREAD_POSIX_CONFIG_INSTALL_EXTERNAL_ROUTES_ENABLE
#define OPENTHREAD_POSIX_CONFIG_INSTALL_EXTERNAL_ROUTES_ENABLE 1
#endif

/**
 * @def OPENTHREAD_POSIX_CONFIG_EXTERNAL_ROUTE_PRIORITY
 *
 * Defines the priority of external routes added to kernel. The larger the number, the lower the priority. We
 * need to assign a low priority to such routes so that kernel prefers the infra link rather than thread. Otherwise we
 * may unnecessarily transmit packets via thread, which potentially causes performance issue. In linux, normally infra
 * link routes' metric value is not greater than 1024, hence 65535 should be big enough.
 */
#ifndef OPENTHREAD_POSIX_CONFIG_EXTERNAL_ROUTE_PRIORITY
#define OPENTHREAD_POSIX_CONFIG_EXTERNAL_ROUTE_PRIORITY 65535
#endif

/**
 * @def OPENTHREAD_POSIX_CONFIG_MAX_EXTERNAL_ROUTE_NUM
 *
 * Defines the max number of external routes that can be added to kernel.
 */
#ifndef OPENTHREAD_POSIX_CONFIG_MAX_EXTERNAL_ROUTE_NUM
#define OPENTHREAD_POSIX_CONFIG_MAX_EXTERNAL_ROUTE_NUM 8
#endif

/**
 * @def OPENTHREAD_POSIX_CONFIG_NAT64_AIL_PREFIX_ENABLE
 *
 * Define as 1 to enable discovering NAT64 posix on adjacent infrastructure link.
 */
#ifndef OPENTHREAD_POSIX_CONFIG_NAT64_AIL_PREFIX_ENABLE
#define OPENTHREAD_POSIX_CONFIG_NAT64_AIL_PREFIX_ENABLE 0
#endif

/**
 * @def OPENTHREAD_POSIX_CONFIG_FIREWALL_ENABLE
 *
 * Define as 1 to enable firewall.
 *
 * The rules are implemented using ip6tables and ipset. The rules are as follows.
 *
 * ip6tables -A $OTBR_FORWARD_INGRESS_CHAIN -m pkttype --pkt-type unicast -i $THREAD_IF -p ip -j DROP
 * ip6tables -A $OTBR_FORWARD_INGRESS_CHAIN -m set --match-set otbr-ingress-deny-src src -p ip -j DROP
 * ip6tables -A $OTBR_FORWARD_INGRESS_CHAIN -m set --match-set otbr-ingress-allow-dst dst -p ip -j ACCEPT
 * ip6tables -A $OTBR_FORWARD_INGRESS_CHAIN -m pkttype --pkt-type unicast -p ip -j DROP
 * ip6tables -A $OTBR_FORWARD_INGRESS_CHAIN -p ip -j ACCEPT
 */
#ifndef OPENTHREAD_POSIX_CONFIG_FIREWALL_ENABLE
#define OPENTHREAD_POSIX_CONFIG_FIREWALL_ENABLE 0
#endif

#if OPENTHREAD_POSIX_CONFIG_FIREWALL_ENABLE
#ifndef OPENTHREAD_POSIX_CONFIG_IPSET_BINARY
#define OPENTHREAD_POSIX_CONFIG_IPSET_BINARY "ipset"
#endif
#endif

/**
 * @def OPENTHREAD_POSIX_CONFIG_THREAD_NETIF_DEFAULT_NAME
 *
 * Define the Thread default network interface name.
 */
#ifndef OPENTHREAD_POSIX_CONFIG_THREAD_NETIF_DEFAULT_NAME
#define OPENTHREAD_POSIX_CONFIG_THREAD_NETIF_DEFAULT_NAME "wpan0"
#endif

#ifdef __APPLE__

/**
 * Use built-in utun driver on mac OS
 */
#define OT_POSIX_CONFIG_MACOS_UTUN 1

/**
 * Use open-source tun driver on mac OS
 */
#define OT_POSIX_CONFIG_MACOS_TUN 2

/**
 * @def OPENTHREAD_POSIX_CONFIG_MACOS_TUN_OPTION
 *
 * This setting configures which tunnel driver to use.
 */
#ifndef OPENTHREAD_POSIX_CONFIG_MACOS_TUN_OPTION
#define OPENTHREAD_POSIX_CONFIG_MACOS_TUN_OPTION OT_POSIX_CONFIG_MACOS_UTUN
#endif

#endif // __APPLE__

//---------------------------------------------------------------------------------------------------------------------
// Removed or renamed POSIX specific configs.

#ifdef OPENTHREAD_CONFIG_POSIX_APP_TREL_INTERFACE_NAME
#error "OPENTHREAD_CONFIG_POSIX_APP_TREL_INTERFACE_NAME was removed (no longer applicable with TREL over DNS-SD)."
#endif

#ifdef OPENTHREAD_CONFIG_POSIX_TREL_USE_NETLINK_SOCKET
#error "OPENTHREAD_CONFIG_POSIX_TREL_USE_NETLINK_SOCKET was removed (no longer applicable with TREL over DNS-SD)."
#endif

/**
 * @def OPENTHREAD_POSIX_CONFIG_TREL_UDP_PORT
 *
 * This setting configures the TREL UDP port number.
 * Define as 0 to use an ephemeral port number.
 */
#ifndef OPENTHREAD_POSIX_CONFIG_TREL_UDP_PORT
#define OPENTHREAD_POSIX_CONFIG_TREL_UDP_PORT 0
#endif

/**
 * @def OPENTHREAD_POSIX_CONFIG_NAT64_CIDR
 *
 * This setting configures the NAT64 CIDR, used by NAT64 translator.
 */
#ifndef OPENTHREAD_POSIX_CONFIG_NAT64_CIDR
#define OPENTHREAD_POSIX_CONFIG_NAT64_CIDR "192.168.255.0/24"
#endif

/**
 * @def OPENTHREAD_POSIX_CONFIG_BACKTRACE_ENABLE
 *
 * Define as 1 to enable backtrace support.
 */
#ifndef OPENTHREAD_POSIX_CONFIG_BACKTRACE_ENABLE
#define OPENTHREAD_POSIX_CONFIG_BACKTRACE_ENABLE 1
#endif

/**
 * @def OPENTHREAD_POSIX_CONFIG_ANDROID_ENABLE
 *
 * Define as 1 to enable android support.
 */
#ifndef OPENTHREAD_POSIX_CONFIG_ANDROID_ENABLE
#define OPENTHREAD_POSIX_CONFIG_ANDROID_ENABLE 0
#endif

/**
 * @def OPENTHREAD_POSIX_CONFIG_INFRA_IF_ENABLE
 *
 * Defines `1` to enable the posix implementation of platform/infra_if.h APIs.
 * The default value is set to `OPENTHREAD_CONFIG_BORDER_ROUTING_ENABLE` if it's
 * not explicit defined.
 */
#ifndef OPENTHREAD_POSIX_CONFIG_INFRA_IF_ENABLE
#define OPENTHREAD_POSIX_CONFIG_INFRA_IF_ENABLE \
    (OPENTHREAD_CONFIG_BORDER_ROUTING_ENABLE || OPENTHREAD_CONFIG_BACKBONE_ROUTER_ENABLE)
#endif

/**
 * @def OPENTHREAD_POSIX_CONFIG_FACTORY_CONFIG_FILE
 *
 * Define the path of the factory config file.
 *
 * Note: The factory config file contains the persist data that configured by the factory. And it won't be changed
 *       after a device firmware update OTA is done.
 */
#ifndef OPENTHREAD_POSIX_CONFIG_FACTORY_CONFIG_FILE
#define OPENTHREAD_POSIX_CONFIG_FACTORY_CONFIG_FILE "src/posix/platform/openthread.conf.example"
#endif

/**
 * @def OPENTHREAD_POSIX_CONFIG_PRODUCT_CONFIG_FILE
 *
 * Define the path of the product config file.
 */
#ifndef OPENTHREAD_POSIX_CONFIG_PRODUCT_CONFIG_FILE
#define OPENTHREAD_POSIX_CONFIG_PRODUCT_CONFIG_FILE "src/posix/platform/openthread.conf.example"
#endif

/**
 * @def OPENTHREAD_POSIX_CONFIG_CONFIGURATION_FILE_ENABLE
 *
 * Define as 1 to enable the configuration file support.
 */
#ifndef OPENTHREAD_POSIX_CONFIG_CONFIGURATION_FILE_ENABLE
#define OPENTHREAD_POSIX_CONFIG_CONFIGURATION_FILE_ENABLE 1
#endif

/**
 * @def OPENTHREAD_POSIX_CONFIG_RCP_TIME_SYNC_INTERVAL
 *
 * This setting configures the interval (in units of microseconds) for host-rcp
 * time sync. The host will recalculate the time offset between host and RCP
 * every interval.
 */
#ifdef OPENTHREAD_POSIX_CONFIG_RCP_TIME_SYNC_INTERVAL
#error "OPENTHREAD_POSIX_CONFIG_RCP_TIME_SYNC_INTERVAL was replaced by OPENTHREAD_SPINEL_CONFIG_RCP_TIME_SYNC_INTERVAL"
#endif

/**
 * @def OPENTHREAD_POSIX_CONFIG_EXIT_ON_INFRA_NETIF_LOST_ENABLE
 *
 * Define as 1 to let the process exit when the infra network interface is lost on the POSIX platform.
 */
#ifndef OPENTHREAD_POSIX_CONFIG_EXIT_ON_INFRA_NETIF_LOST_ENABLE
#define OPENTHREAD_POSIX_CONFIG_EXIT_ON_INFRA_NETIF_LOST_ENABLE 1
#endif

/**
 * @def OPENTHREAD_POSIX_CONFIG_TREL_TX_PACKET_POOL_SIZE
 *
 * This setting configures the capacity of TREL packet pool for transmission.
 */
#ifndef OPENTHREAD_POSIX_CONFIG_TREL_TX_PACKET_POOL_SIZE
#define OPENTHREAD_POSIX_CONFIG_TREL_TX_PACKET_POOL_SIZE 5
#endif

/**
 * @def OPENTHREAD_POSIX_CONFIG_RCP_CAPS_DIAG_ENABLE
 *
 * Define as 1 to enable RCP capability diagnostic support.
 */
#ifndef OPENTHREAD_POSIX_CONFIG_RCP_CAPS_DIAG_ENABLE
#define OPENTHREAD_POSIX_CONFIG_RCP_CAPS_DIAG_ENABLE OPENTHREAD_CONFIG_DIAG_ENABLE
#endif

#endif // OPENTHREAD_PLATFORM_POSIX_CONFIG_H_
