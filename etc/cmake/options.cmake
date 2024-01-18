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
    # Declare an (ON/OFF/unspecified) OT cmake config with `name`
    # mapping to OPENTHREAD_CONFIG `ot_config`. Parameter
    # `description` provides the help string for this OT cmake
    # config. There is an optional last parameter which if provided
    # determines the default value for the cmake config. If not
    # provided empty string is used which will be treated as "not
    # specified". In this case, the variable `name` would still be
    # false but the related OPENTHREAD_CONFIG is not added in
    # `ot-config`.

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

macro(ot_string_option name ot_config description)
    # Declare a string OT cmake config with `name` mapping to
    # OPENTHREAD_CONFIG `ot_config`. Parameter `description` provides
    # the help string for this OT cmake config. There is an optional
    # last parameter which if provided determines the default value
    # for the cmake config. If not provided empty string is used
    # which will be treated as "not specified".

    if (${ARGC} GREATER 3)
        set(${name} ${ARGN} CACHE STRING "${description}")
    else()
        set(${name} "" CACHE STRING "${description}")
    endif()

    set_property(CACHE ${name} PROPERTY STRINGS "")

    string(COMPARE EQUAL "${${name}}" "" is_empty)
    if (is_empty)
        message(STATUS "${name}=\"\"")
    else()
        message(STATUS "${name}=\"${${name}}\" --> ${ot_config}=\"${${name}}\"")
        target_compile_definitions(ot-config INTERFACE "${ot_config}=\"${${name}}\"")
    endif()
endmacro()

macro(ot_int_option name ot_config description)
    # Declares a integer-value OT cmake config with `name` mapping to
    # OPENTHREAD_CONFIG `ot_config`. There is an optional last
    # parameter which if provided determines the default value for
    # the cmake config. If not provided empty string is used which
    # will be treated as "not specified".

    if (${ARGC} GREATER 3)
        set(${name} ${ARGN} CACHE STRING "${description}")
    else()
        set(${name} "" CACHE STRING "${description}")
    endif()

    set_property(CACHE ${name} PROPERTY STRINGS "")

    string(COMPARE EQUAL "${${name}}" "" is_empty)
    if (is_empty)
        message(STATUS "${name}=\"\"")
    elseif("${${name}}" MATCHES "^[0-9]+$")
        message(STATUS "${name}=\"${${name}}\" --> ${ot_config}=${${name}}")
        target_compile_definitions(ot-config INTERFACE "${ot_config}=${${name}}")
    else()
        message(FATAL_ERROR "${name}=\"${${name}}\" - invalid value, must be integer")
    endif()
endmacro()

macro(ot_multi_option name values ot_config ot_value_prefix description)
    # Declares a multi-value OT cmake config with `name` with valid
    # values from `values` list mapping to OPENTHREAD_CONFIG
    # `ot_config`. The `ot_value_prefix` is the prefix string to
    # construct the OPENTHREAD_CONFIG value. There is an optional
    # last parameter which if provided determines the default value
    # for the cmake config. If not provided empty string is used
    # which will be treated as "not specified".

    if (${ARGC} GREATER 5)
        set(${name} ${ARGN} CACHE STRING "${description}")
    else()
        set(${name} "" CACHE STRING "${description}")
    endif()

    set_property(CACHE ${name} PROPERTY STRINGS "${${values}}")

    string(COMPARE EQUAL "${${name}}" "" is_empty)
    if (is_empty)
        message(STATUS "${name}=\"\"")
    else()
        list(FIND ${values} "${${name}}" ot_index)
        if(ot_index EQUAL -1)
            message(FATAL_ERROR "${name}=\"${${name}}\" - unknown value, valid values " "${${values}}")
        else()
            message(STATUS "${name}=\"${${name}}\" --> ${ot_config}=${ot_value_prefix}${${name}}")
            target_compile_definitions(ot-config INTERFACE "${ot_config}=${ot_value_prefix}${${name}}")
        endif()
    endif()
endmacro()

message(STATUS "- - - - - - - - - - - - - - - - ")
message(STATUS "OpenThread ON/OFF/Unspecified Configs")

ot_option(OT_15_4 OPENTHREAD_CONFIG_RADIO_LINK_IEEE_802_15_4_ENABLE "802.15.4 radio link")
ot_option(OT_ANDROID_NDK OPENTHREAD_CONFIG_ANDROID_NDK_ENABLE "enable android NDK")
ot_option(OT_ANYCAST_LOCATOR OPENTHREAD_CONFIG_TMF_ANYCAST_LOCATOR_ENABLE "anycast locator")
ot_option(OT_ASSERT OPENTHREAD_CONFIG_ASSERT_ENABLE "assert function OT_ASSERT()")
ot_option(OT_BACKBONE_ROUTER OPENTHREAD_CONFIG_BACKBONE_ROUTER_ENABLE "backbone router functionality")
ot_option(OT_BACKBONE_ROUTER_DUA_NDPROXYING OPENTHREAD_CONFIG_BACKBONE_ROUTER_DUA_NDPROXYING_ENABLE "BBR DUA ND Proxy")
ot_option(OT_BACKBONE_ROUTER_MULTICAST_ROUTING OPENTHREAD_CONFIG_BACKBONE_ROUTER_MULTICAST_ROUTING_ENABLE "BBR MR")
ot_option(OT_BLE_TCAT OPENTHREAD_CONFIG_BLE_TCAT_ENABLE "Ble based thread commissioning")
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
ot_option(OT_CSL_RECEIVER_LOCAL_TIME_SYNC OPENTHREAD_CONFIG_MAC_CSL_RECEIVER_LOCAL_TIME_SYNC "use local time for csl elapsed sync time")
ot_option(OT_DATASET_UPDATER OPENTHREAD_CONFIG_DATASET_UPDATER_ENABLE "dataset updater")
ot_option(OT_DEVICE_PROP_LEADER_WEIGHT OPENTHREAD_CONFIG_MLE_DEVICE_PROPERTY_LEADER_WEIGHT_ENABLE "device prop for leader weight")
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
ot_option(OT_LINK_METRICS_MANAGER  OPENTHREAD_CONFIG_LINK_METRICS_MANAGER_ENABLE "link metrics manager")
ot_option(OT_LINK_METRICS_SUBJECT OPENTHREAD_CONFIG_MLE_LINK_METRICS_SUBJECT_ENABLE "link metrics subject")
ot_option(OT_LINK_RAW OPENTHREAD_CONFIG_LINK_RAW_ENABLE "link raw service")
ot_option(OT_LOG_LEVEL_DYNAMIC OPENTHREAD_CONFIG_LOG_LEVEL_DYNAMIC_ENABLE "dynamic log level control")
ot_option(OT_MAC_FILTER OPENTHREAD_CONFIG_MAC_FILTER_ENABLE "mac filter")
ot_option(OT_MESH_DIAG OPENTHREAD_CONFIG_MESH_DIAG_ENABLE "mesh diag")
ot_option(OT_MESSAGE_USE_HEAP OPENTHREAD_CONFIG_MESSAGE_USE_HEAP_ENABLE "heap allocator for message buffers")
ot_option(OT_MLE_LONG_ROUTES OPENTHREAD_CONFIG_MLE_LONG_ROUTES_ENABLE "MLE long routes extension (experimental)")
ot_option(OT_MLR OPENTHREAD_CONFIG_MLR_ENABLE "Multicast Listener Registration (MLR)")
ot_option(OT_MULTIPAN_RCP OPENTHREAD_CONFIG_MULTIPAN_RCP_ENABLE "enable multi-PAN RCP")
ot_option(OT_MULTIPLE_INSTANCE OPENTHREAD_CONFIG_MULTIPLE_INSTANCE_ENABLE "multiple instances")
ot_option(OT_NAT64_BORDER_ROUTING OPENTHREAD_CONFIG_NAT64_BORDER_ROUTING_ENABLE "border routing NAT64")
ot_option(OT_NAT64_TRANSLATOR OPENTHREAD_CONFIG_NAT64_TRANSLATOR_ENABLE "NAT64 translator support")
ot_option(OT_NEIGHBOR_DISCOVERY_AGENT OPENTHREAD_CONFIG_NEIGHBOR_DISCOVERY_AGENT_ENABLE "neighbor discovery agent")
ot_option(OT_NETDATA_PUBLISHER OPENTHREAD_CONFIG_NETDATA_PUBLISHER_ENABLE "Network Data publisher")
ot_option(OT_NETDIAG_CLIENT OPENTHREAD_CONFIG_TMF_NETDIAG_CLIENT_ENABLE "Network Diagnostic client")
ot_option(OT_OPERATIONAL_DATASET_AUTO_INIT OPENTHREAD_CONFIG_OPERATIONAL_DATASET_AUTO_INIT "operational dataset auto init")
ot_option(OT_OTNS OPENTHREAD_CONFIG_OTNS_ENABLE "OTNS")
ot_option(OT_PING_SENDER OPENTHREAD_CONFIG_PING_SENDER_ENABLE "ping sender" ${OT_APP_CLI})
ot_option(OT_PLATFORM_BOOTLOADER_MODE OPENTHREAD_CONFIG_PLATFORM_BOOTLOADER_MODE_ENABLE "platform bootloader mode")
ot_option(OT_PLATFORM_KEY_REF OPENTHREAD_CONFIG_PLATFORM_KEY_REFERENCES_ENABLE "platform key reference secure storage")
ot_option(OT_PLATFORM_NETIF OPENTHREAD_CONFIG_PLATFORM_NETIF_ENABLE "platform netif")
ot_option(OT_PLATFORM_POWER_CALIBRATION OPENTHREAD_CONFIG_PLATFORM_POWER_CALIBRATION_ENABLE "power calibration")
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
ot_option(OT_TX_QUEUE_STATS OPENTHREAD_CONFIG_TX_QUEUE_STATISTICS_ENABLE "tx queue statistics")
ot_option(OT_UDP_FORWARD OPENTHREAD_CONFIG_UDP_FORWARD_ENABLE "UDP forward")
ot_option(OT_UPTIME OPENTHREAD_CONFIG_UPTIME_ENABLE "uptime")

option(OT_DOC "build OpenThread documentation")
message(STATUS "- - - - - - - - - - - - - - - - ")

# - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
# Get a list of the available platforms and output as a list to the 'arg_platforms' argument
function(ot_get_platforms arg_platforms)
    list(APPEND result "NO" "posix" "external")
    set(platforms_dir "${PROJECT_SOURCE_DIR}/examples/platforms")
    file(GLOB platforms RELATIVE "${platforms_dir}" "${platforms_dir}/*")
    foreach(platform IN LISTS platforms)
        if(IS_DIRECTORY "${platforms_dir}/${platform}")
            list(APPEND result "${platform}")
        endif()
    endforeach()
    list(REMOVE_ITEM result utils)
    list(SORT result)
    set(${arg_platforms} "${result}" PARENT_SCOPE)
endfunction()

ot_get_platforms(OT_PLATFORM_VALUES)
set(OT_PLATFORM "NO" CACHE STRING "Target platform chosen by the user at configure time")
set_property(CACHE OT_PLATFORM PROPERTY STRINGS "${OT_PLATFORM_VALUES}")
message(STATUS "OT_PLATFORM=\"${OT_PLATFORM}\"")
list(FIND OT_PLATFORM_VALUES "${OT_PLATFORM}" ot_index)
if(ot_index EQUAL -1)
    message(FATAL_ERROR "Invalid value for OT_PLATFORM - valid values are:" "${OT_PLATFORM_VALUES}")
endif()

# - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
set(OT_THREAD_VERSION_VALUES "1.1" "1.2" "1.3" "1.3.1")
set(OT_THREAD_VERSION "1.3" CACHE STRING "set Thread version")
set_property(CACHE OT_THREAD_VERSION PROPERTY STRINGS "${OT_THREAD_VERSION_VALUES}")
list(FIND OT_THREAD_VERSION_VALUES "${OT_THREAD_VERSION}" ot_index)
if(ot_index EQUAL -1)
    message(STATUS "OT_THREAD_VERSION=\"${OT_THREAD_VERSION}\"")
    message(FATAL_ERROR "Invalid value for OT_THREAD_VERSION - valid values are: " "${OT_THREAD_VERSION_VALUES}")
endif()
set(OT_VERSION_SUFFIX_LIST "1_1" "1_2" "1_3" "1_3_1")
list(GET OT_VERSION_SUFFIX_LIST ${ot_index} OT_VERSION_SUFFIX)
target_compile_definitions(ot-config INTERFACE "OPENTHREAD_CONFIG_THREAD_VERSION=OT_THREAD_VERSION_${OT_VERSION_SUFFIX}")
message(STATUS "OT_THREAD_VERSION=\"${OT_THREAD_VERSION}\" -> OPENTHREAD_CONFIG_THREAD_VERSION=OT_THREAD_VERSION_${OT_VERSION_SUFFIX}")

# - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
set(OT_LOG_LEVEL_VALUES "NONE" "CRIT" "WARN" "NOTE" "INFO" "DEBG")
ot_multi_option(OT_LOG_LEVEL OT_LOG_LEVEL_VALUES OPENTHREAD_CONFIG_LOG_LEVEL OT_LOG_LEVEL_ "set OpenThread log level")

option(OT_FULL_LOGS "enable full logs")
if(OT_FULL_LOGS)
    if(NOT OT_LOG_LEVEL)
        message(STATUS "OT_FULL_LOGS=ON --> Setting LOG_LEVEL to DEBG")
        target_compile_definitions(ot-config INTERFACE "OPENTHREAD_CONFIG_LOG_LEVEL=OT_LOG_LEVEL_DEBG")
    endif()
    target_compile_definitions(ot-config INTERFACE "OPENTHREAD_CONFIG_LOG_PREPEND_LEVEL=1")
endif()

# - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
if(OT_REFERENCE_DEVICE AND NOT OT_PLATFORM STREQUAL "posix")
    set(OT_DEFAULT_LOG_OUTPUT "APP")
else()
    set(OT_DEFAULT_LOG_OUTPUT "")
endif()
set(OT_LOG_OUTPUT_VALUES "APP" "DEBUG_UART" "NONE" "PLATFORM_DEFINED")
ot_multi_option(OT_LOG_OUTPUT OT_LOG_OUTPUT_VALUES OPENTHREAD_CONFIG_LOG_OUTPUT OPENTHREAD_CONFIG_LOG_OUTPUT_ "Set the log output" "${OT_DEFAULT_LOG_OUTPUT}")

# - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

ot_string_option(OT_VENDOR_NAME OPENTHREAD_CONFIG_NET_DIAG_VENDOR_NAME "set the vendor name config")
ot_string_option(OT_VENDOR_MODEL OPENTHREAD_CONFIG_NET_DIAG_VENDOR_MODEL "set the vendor model config")
ot_string_option(OT_VENDOR_SW_VERSION OPENTHREAD_CONFIG_NET_DIAG_VENDOR_SW_VERSION "set the vendor sw version config")

set(OT_POWER_SUPPLY_VALUES "BATTERY" "EXTERNAL" "EXTERNAL_STABLE" "EXTERNAL_UNSTABLE")
ot_multi_option(OT_POWER_SUPPLY OT_POWER_SUPPLY_VALUES OPENTHREAD_CONFIG_DEVICE_POWER_SUPPLY OT_POWER_SUPPLY_ "set the device power supply config")

ot_int_option(OT_MLE_MAX_CHILDREN OPENTHREAD_CONFIG_MLE_MAX_CHILDREN "set maximum number of children")
ot_int_option(OT_RCP_RESTORATION_MAX_COUNT OPENTHREAD_SPINEL_CONFIG_RCP_RESTORATION_MAX_COUNT "set max RCP restoration count")
ot_int_option(OT_RCP_TX_WAIT_TIME_SECS OPENTHREAD_SPINEL_CONFIG_RCP_TX_WAIT_TIME_SECS "set RCP TX wait TIME in seconds")

# - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

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
ot_removed_option(OT_POSIX_CONFIG_RCP_BUS "- Use OT_POSIX_RCP_HDLC_BUS, OT_POSIX_RCP_SPI_BUS or OT_POSIX_RCP_VENDOR_BUS instead, OT_POSIX_CONFIG_RCP_BUS is deprecated")
