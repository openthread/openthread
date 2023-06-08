#
#  Copyright (c) 2019, The OpenThread Authors.
#  All rights reserved.
#
#  Redistribution and use in source and binary forms, with or without
#  modification, are permitted provided that the following conditions are met:
#  1. Redistributions of source code must retain the above copyright
#     notice, this list of conditions and the following disclaimer.
#  2. Redistributions in binary form must reproduce the above copyright
#     notice, this list of conditions and the following disclaimer in the
#     documentation and/or other materials provided with the distribution.
#  3. Neither the name of the copyright holder nor the
#     names of its contributors may be used to endorse or promote products
#     derived from this software without specific prior written permission.
#
#  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
#  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
#  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
#  ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
#  LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
#  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
#  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
#  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
#  CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
#  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
#  POSSIBILITY OF SUCH DAMAGE.
#

option(OT_APP_CLI "enable CLI app" ON)
option(OT_APP_NCP "enable NCP app" ON)
option(OT_APP_RCP "enable RCP app" ON)

option(OT_FTD "enable FTD" ON)
option(OT_MTD "enable MTD" ON)
option(OT_RCP "enable RCP" ON)

option(OT_LINKER_MAP "generate .map files for example apps" ON)

message(STATUS OT_APP_CLI=${OT_APP_CLI})
message(STATUS OT_APP_NCP=${OT_APP_NCP})
message(STATUS OT_APP_RCP=${OT_APP_RCP})
message(STATUS OT_FTD=${OT_FTD})
message(STATUS OT_MTD=${OT_MTD})
message(STATUS OT_RCP=${OT_RCP})

set(OT_CONFIG_VALUES
    ""
    "ON"
    "OFF"
)

macro(ot_option name ot_config description)
    # Declare an OT cmake config with `name` mapping to OPENTHREAD_CONFIG
    # `ot_config`. Parameter `description` provides the help string for this
    # OT cmake config. There is an optional last parameter which if provided
    # determines the default value for the cmake config. If not provided
    # empty string is used which will be treated as "not specified". In this
    # case, the variable `name` would still be false but the related
    # OPENTHREAD_CONFIG is not added in `ot-config`.

    if (${ARGC} GREATER 3)
        set(${name} ${ARGN} CACHE STRING "enable ${description}")
    else()
        set(${name} "" CACHE STRING "enable ${description}")
    endif()

    set_property(CACHE ${name} PROPERTY STRINGS ${OT_CONFIG_VALUES})

    string(COMPARE EQUAL "${${name}}" "" is_empty)
    if (is_empty)
        message(STATUS "${name}=\"\"")
    elseif (${name})
        message(STATUS "${name}=ON --> ${ot_config}=1")
        target_compile_definitions(ot-config INTERFACE "${ot_config}=1")
    else()
        message(STATUS "${name}=OFF --> ${ot_config}=0")
        target_compile_definitions(ot-config INTERFACE "${ot_config}=0")
    endif()
endmacro()

ot_option(OT_15_4 OPENTHREAD_CONFIG_RADIO_LINK_IEEE_802_15_4_ENABLE "802.15.4 radio link")
ot_option(OT_ANDROID_NDK OPENTHREAD_CONFIG_ANDROID_NDK_ENABLE "enable android NDK")
ot_option(OT_ANYCAST_LOCATOR OPENTHREAD_CONFIG_TMF_ANYCAST_LOCATOR_ENABLE "anycast locator")
ot_option(OT_ASSERT OPENTHREAD_CONFIG_ASSERT_ENABLE "assert function OT_ASSERT()")
ot_option(OT_BACKBONE_ROUTER OPENTHREAD_CONFIG_BACKBONE_ROUTER_ENABLE "backbone router functionality")
ot_option(OT_BACKBONE_ROUTER_DUA_NDPROXYING OPENTHREAD_CONFIG_BACKBONE_ROUTER_DUA_NDPROXYING_ENABLE "BBR DUA ND Proxy")
ot_option(OT_BACKBONE_ROUTER_MULTICAST_ROUTING OPENTHREAD_CONFIG_BACKBONE_ROUTER_MULTICAST_ROUTING_ENABLE "BBR MR")
ot_option(OT_BORDER_AGENT OPENTHREAD_CONFIG_BORDER_AGENT_ENABLE "border agent")
ot_option(OT_BORDER_AGENT_ID OPENTHREAD_CONFIG_BORDER_AGENT_ID_ENABLE "create and save border agent ID")
ot_option(OT_BORDER_ROUTER OPENTHREAD_CONFIG_BORDER_ROUTER_ENABLE "border router")
ot_option(OT_BORDER_ROUTING OPENTHREAD_CONFIG_BORDER_ROUTING_ENABLE "border routing")
ot_option(OT_BORDER_ROUTING_DHCP6_PD OPENTHREAD_CONFIG_BORDER_ROUTING_DHCP6_PD_ENABLE "dhcpv6 pd support in border routing")
ot_option(OT_BORDER_ROUTING_COUNTERS OPENTHREAD_CONFIG_IP6_BR_COUNTERS_ENABLE "border routing counters")
ot_option(OT_CHANNEL_MANAGER OPENTHREAD_CONFIG_CHANNEL_MANAGER_ENABLE "channel manager")
ot_option(OT_CHANNEL_MONITOR OPENTHREAD_CONFIG_CHANNEL_MONITOR_ENABLE "channel monitor")
ot_option(OT_COAP OPENTHREAD_CONFIG_COAP_API_ENABLE "coap api")
ot_option(OT_COAP_BLOCK OPENTHREAD_CONFIG_COAP_BLOCKWISE_TRANSFER_ENABLE "coap block-wise transfer (RFC7959)")
ot_option(OT_COAP_OBSERVE OPENTHREAD_CONFIG_COAP_OBSERVE_API_ENABLE "coap observe (RFC7641)")
ot_option(OT_COAPS OPENTHREAD_CONFIG_COAP_SECURE_API_ENABLE "secure coap")
ot_option(OT_COMMISSIONER OPENTHREAD_CONFIG_COMMISSIONER_ENABLE "commissioner")
ot_option(OT_CSL_AUTO_SYNC OPENTHREAD_CONFIG_MAC_CSL_AUTO_SYNC_ENABLE "data polling based on csl")
ot_option(OT_CSL_DEBUG OPENTHREAD_CONFIG_MAC_CSL_DEBUG_ENABLE "csl debug")
ot_option(OT_CSL_RECEIVER OPENTHREAD_CONFIG_MAC_CSL_RECEIVER_ENABLE "csl receiver")
ot_option(OT_DATASET_UPDATER OPENTHREAD_CONFIG_DATASET_UPDATER_ENABLE "dataset updater")
ot_option(OT_DHCP6_CLIENT OPENTHREAD_CONFIG_DHCP6_CLIENT_ENABLE "DHCP6 client")
ot_option(OT_DHCP6_SERVER OPENTHREAD_CONFIG_DHCP6_SERVER_ENABLE "DHCP6 server")
ot_option(OT_DIAGNOSTIC OPENTHREAD_CONFIG_DIAG_ENABLE "diagnostic")
ot_option(OT_DNS_CLIENT OPENTHREAD_CONFIG_DNS_CLIENT_ENABLE "DNS client")
ot_option(OT_DNS_CLIENT_OVER_TCP OPENTHREAD_CONFIG_DNS_CLIENT_OVER_TCP_ENABLE  "Enable dns query over tcp")
ot_option(OT_DNS_DSO OPENTHREAD_CONFIG_DNS_DSO_ENABLE "DNS Stateful Operations (DSO)")
ot_option(OT_DNS_UPSTREAM_QUERY OPENTHREAD_CONFIG_DNS_UPSTREAM_QUERY_ENABLE "Allow sending DNS queries to upstream")
ot_option(OT_DNSSD_SERVER OPENTHREAD_CONFIG_DNSSD_SERVER_ENABLE "DNS-SD server")
ot_option(OT_DUA OPENTHREAD_CONFIG_DUA_ENABLE "Domain Unicast Address (DUA)")
ot_option(OT_ECDSA OPENTHREAD_CONFIG_ECDSA_ENABLE "ECDSA")
ot_option(OT_EXTERNAL_HEAP OPENTHREAD_CONFIG_HEAP_EXTERNAL_ENABLE "external heap")
ot_option(OT_FIREWALL OPENTHREAD_POSIX_CONFIG_FIREWALL_ENABLE "firewall")
ot_option(OT_HISTORY_TRACKER OPENTHREAD_CONFIG_HISTORY_TRACKER_ENABLE "history tracker")
ot_option(OT_IP6_FRAGM OPENTHREAD_CONFIG_IP6_FRAGMENTATION_ENABLE "ipv6 fragmentation")
ot_option(OT_JAM_DETECTION OPENTHREAD_CONFIG_JAM_DETECTION_ENABLE "jam detection")
ot_option(OT_JOINER OPENTHREAD_CONFIG_JOINER_ENABLE "joiner")
ot_option(OT_LINK_METRICS_INITIATOR OPENTHREAD_CONFIG_MLE_LINK_METRICS_INITIATOR_ENABLE "link metrics initiator")
ot_option(OT_LINK_METRICS_SUBJECT OPENTHREAD_CONFIG_MLE_LINK_METRICS_SUBJECT_ENABLE "link metrics subject")
ot_option(OT_LINK_RAW OPENTHREAD_CONFIG_LINK_RAW_ENABLE "link raw service")
ot_option(OT_LOG_LEVEL_DYNAMIC OPENTHREAD_CONFIG_LOG_LEVEL_DYNAMIC_ENABLE "dynamic log level control")
ot_option(OT_MAC_FILTER OPENTHREAD_CONFIG_MAC_FILTER_ENABLE "mac filter")
ot_option(OT_MESH_DIAG OPENTHREAD_CONFIG_MESH_DIAG_ENABLE "mesh diag")
ot_option(OT_MESSAGE_USE_HEAP OPENTHREAD_CONFIG_MESSAGE_USE_HEAP_ENABLE "heap allocator for message buffers")
ot_option(OT_MLE_LONG_ROUTES OPENTHREAD_CONFIG_MLE_LONG_ROUTES_ENABLE "MLE long routes extension (experimental)")
ot_option(OT_MLR OPENTHREAD_CONFIG_MLR_ENABLE "Multicast Listener Registration (MLR)")
ot_option(OT_MULTIPLE_INSTANCE OPENTHREAD_CONFIG_MULTIPLE_INSTANCE_ENABLE "multiple instances")
ot_option(OT_NAT64_BORDER_ROUTING OPENTHREAD_CONFIG_NAT64_BORDER_ROUTING_ENABLE "border routing NAT64")
ot_option(OT_NAT64_TRANSLATOR OPENTHREAD_CONFIG_NAT64_TRANSLATOR_ENABLE "NAT64 translator support")
ot_option(OT_NEIGHBOR_DISCOVERY_AGENT OPENTHREAD_CONFIG_NEIGHBOR_DISCOVERY_AGENT_ENABLE "neighbor discovery agent")
ot_option(OT_NETDATA_PUBLISHER OPENTHREAD_CONFIG_NETDATA_PUBLISHER_ENABLE "Network Data publisher")
ot_option(OT_NETDIAG_CLIENT OPENTHREAD_CONFIG_TMF_NETDIAG_CLIENT_ENABLE "Network Diagnostic client")
ot_option(OT_OTNS OPENTHREAD_CONFIG_OTNS_ENABLE "OTNS")
ot_option(OT_PING_SENDER OPENTHREAD_CONFIG_PING_SENDER_ENABLE "ping sender" ${OT_APP_CLI})
ot_option(OT_PLATFORM_NETIF OPENTHREAD_CONFIG_PLATFORM_NETIF_ENABLE "platform netif")
ot_option(OT_PLATFORM_UDP OPENTHREAD_CONFIG_PLATFORM_UDP_ENABLE "platform UDP")
ot_option(OT_REFERENCE_DEVICE OPENTHREAD_CONFIG_REFERENCE_DEVICE_ENABLE "test harness reference device")
ot_option(OT_SERVICE OPENTHREAD_CONFIG_TMF_NETDATA_SERVICE_ENABLE "Network Data service")
ot_option(OT_SETTINGS_RAM OPENTHREAD_SETTINGS_RAM "volatile-only storage of settings")
ot_option(OT_SLAAC OPENTHREAD_CONFIG_IP6_SLAAC_ENABLE "SLAAC address")
ot_option(OT_SNTP_CLIENT OPENTHREAD_CONFIG_SNTP_CLIENT_ENABLE "SNTP client")
ot_option(OT_SRP_CLIENT OPENTHREAD_CONFIG_SRP_CLIENT_ENABLE "SRP client")
ot_option(OT_SRP_SERVER OPENTHREAD_CONFIG_SRP_SERVER_ENABLE "SRP server")
ot_option(OT_TCP OPENTHREAD_CONFIG_TCP_ENABLE "TCP")
ot_option(OT_TIME_SYNC OPENTHREAD_CONFIG_TIME_SYNC_ENABLE "time synchronization service")
ot_option(OT_TREL OPENTHREAD_CONFIG_RADIO_LINK_TREL_ENABLE "TREL radio link for Thread over Infrastructure feature")
ot_option(OT_TX_BEACON_PAYLOAD OPENTHREAD_CONFIG_MAC_OUTGOING_BEACON_PAYLOAD_ENABLE "tx beacon payload")
ot_option(OT_UDP_FORWARD OPENTHREAD_CONFIG_UDP_FORWARD_ENABLE "UDP forward")
ot_option(OT_UPTIME OPENTHREAD_CONFIG_UPTIME_ENABLE "uptime")

option(OT_DOC "Build OpenThread documentation")

option(OT_FULL_LOGS "enable full logs")
if(OT_FULL_LOGS)
    if(NOT OT_LOG_LEVEL)
        message(STATUS "OT_FULL_LOGS=ON --> Setting LOG_LEVEL to DEBG")
        target_compile_definitions(ot-config INTERFACE "OPENTHREAD_CONFIG_LOG_LEVEL=OT_LOG_LEVEL_DEBG")
    endif()
    target_compile_definitions(ot-config INTERFACE "OPENTHREAD_CONFIG_LOG_PREPEND_LEVEL=1")
endif()

set(OT_VENDOR_NAME "" CACHE STRING "set the vendor name config")
set_property(CACHE OT_VENDOR_NAME PROPERTY STRINGS ${OT_VENDOR_NAME_VALUES})
string(COMPARE EQUAL "${OT_VENDOR_NAME}" "" is_empty)
if (is_empty)
    message(STATUS "OT_VENDOR_NAME=\"\"")
else()
    message(STATUS "OT_VENDOR_NAME=\"${OT_VENDOR_NAME}\" --> OPENTHREAD_CONFIG_NET_DIAG_VENDOR_NAME=\"${OT_VENDOR_NAME}\"")
    target_compile_definitions(ot-config INTERFACE "OPENTHREAD_CONFIG_NET_DIAG_VENDOR_NAME=\"${OT_VENDOR_NAME}\"")
endif()

set(OT_VENDOR_MODEL "" CACHE STRING "set the vendor model config")
set_property(CACHE OT_VENDOR_MODEL PROPERTY STRINGS ${OT_VENDOR_MODEL_VALUES})
string(COMPARE EQUAL "${OT_VENDOR_MODEL}" "" is_empty)
if (is_empty)
    message(STATUS "OT_VENDOR_MODEL=\"\"")
else()
    message(STATUS "OT_VENDOR_MODEL=\"${OT_VENDOR_MODEL}\" --> OPENTHREAD_CONFIG_NET_DIAG_VENDOR_MODEL=\"${OT_VENDOR_MODEL}\"")
    target_compile_definitions(ot-config INTERFACE "OPENTHREAD_CONFIG_NET_DIAG_VENDOR_MODEL=\"${OT_VENDOR_MODEL}\"")
endif()

set(OT_VENDOR_SW_VERSION "" CACHE STRING "set the vendor sw version config")
set_property(CACHE OT_VENDOR_SW_VERSION PROPERTY STRINGS ${OT_VENDOR_SW_VERSION_VALUES})
string(COMPARE EQUAL "${OT_VENDOR_SW_VERSION}" "" is_empty)
if (is_empty)
    message(STATUS "OT_VENDOR_SW_VERSION=\"\"")
else()
    message(STATUS "OT_VENDOR_SW_VERSION=\"${OT_VENDOR_SW_VERSION}\" --> OPENTHREAD_CONFIG_NET_DIAG_VENDOR_SW_VERSION=\"${OT_VENDOR_SW_VERSION}\"")
    target_compile_definitions(ot-config INTERFACE "OPENTHREAD_CONFIG_NET_DIAG_VENDOR_SW_VERSION=\"${OT_VENDOR_SW_VERSION}\"")
endif()

set(OT_POWER_SUPPLY "" CACHE STRING "set the device power supply config")
set(OT_POWER_SUPPLY_VALUES
    ""
    "BATTERY"
    "EXTERNAL"
    "EXTERNAL_STABLE"
    "EXTERNAL_UNSTABLE"
)
set_property(CACHE OT_POWER_SUPPLY PROPERTY STRINGS ${OT_POWER_SUPPLY_VALUES})
string(COMPARE EQUAL "${OT_POWER_SUPPLY}" "" is_empty)
if (is_empty)
    message(STATUS "OT_POWER_SUPPLY=\"\"")
else()
    message(STATUS "OT_POWER_SUPPLY=${OT_POWER_SUPPLY} --> OPENTHREAD_CONFIG_DEVICE_POWER_SUPPLY=OT_POWER_SUPPLY_${OT_POWER_SUPPLY}")
    target_compile_definitions(ot-config INTERFACE "OPENTHREAD_CONFIG_DEVICE_POWER_SUPPLY=OT_POWER_SUPPLY_${OT_POWER_SUPPLY}")
endif()

set(OT_MLE_MAX_CHILDREN "" CACHE STRING "set maximum number of children")
if(OT_MLE_MAX_CHILDREN MATCHES "^[0-9]+$")
    message(STATUS "OT_MLE_MAX_CHILDREN=${OT_MLE_MAX_CHILDREN}")
    target_compile_definitions(ot-config INTERFACE "OPENTHREAD_CONFIG_MLE_MAX_CHILDREN=${OT_MLE_MAX_CHILDREN}")
elseif(NOT OT_MLE_MAX_CHILDREN STREQUAL "")
    message(FATAL_ERROR "Invalid maximum number of children: ${OT_MLE_MAX_CHILDREN}")
endif()

set(OT_RCP_RESTORATION_MAX_COUNT "0" CACHE STRING "set max RCP restoration count")
if(OT_RCP_RESTORATION_MAX_COUNT MATCHES "^[0-9]+$")
    message(STATUS "OT_RCP_RESTORATION_MAX_COUNT=${OT_RCP_RESTORATION_MAX_COUNT}")
    target_compile_definitions(ot-config INTERFACE "OPENTHREAD_SPINEL_CONFIG_RCP_RESTORATION_MAX_COUNT=${OT_RCP_RESTORATION_MAX_COUNT}")
else()
    message(FATAL_ERROR "Invalid max RCP restoration count: ${OT_RCP_RESTORATION_MAX_COUNT}")
endif()

if(NOT OT_EXTERNAL_MBEDTLS)
    set(OT_MBEDTLS mbedtls)
    target_compile_definitions(ot-config INTERFACE "OPENTHREAD_CONFIG_ENABLE_BUILTIN_MBEDTLS=1")
else()
    set(OT_MBEDTLS ${OT_EXTERNAL_MBEDTLS})
    target_compile_definitions(ot-config INTERFACE "OPENTHREAD_CONFIG_ENABLE_BUILTIN_MBEDTLS=0")
endif()

option(OT_BUILTIN_MBEDTLS_MANAGEMENT "enable builtin mbedtls management" ON)
if(OT_BUILTIN_MBEDTLS_MANAGEMENT)
    target_compile_definitions(ot-config INTERFACE "OPENTHREAD_CONFIG_ENABLE_BUILTIN_MBEDTLS_MANAGEMENT=1")
else()
    target_compile_definitions(ot-config INTERFACE "OPENTHREAD_CONFIG_ENABLE_BUILTIN_MBEDTLS_MANAGEMENT=0")
endif()

if(OT_POSIX_SETTINGS_PATH)
    target_compile_definitions(ot-config INTERFACE "OPENTHREAD_CONFIG_POSIX_SETTINGS_PATH=${OT_POSIX_SETTINGS_PATH}")
endif()

#-----------------------------------------------------------------------------------------------------------------------
# Check removed/replaced options

macro(ot_removed_option name error)
    # This macro checks for a remove option and emits an error
    # if the option is set.
    get_property(is_set CACHE ${name} PROPERTY VALUE SET)
    if (is_set)
        message(FATAL_ERROR "Removed option ${name} is set - ${error}")
    endif()
endmacro()

ot_removed_option(OT_MTD_NETDIAG "- Use OT_NETDIAG_CLIENT instead - note that server function is always supported")
ot_removed_option(OT_EXCLUDE_TCPLP_LIB "- Use OT_TCP instead, OT_EXCLUDE_TCPLP_LIB is deprecated")
