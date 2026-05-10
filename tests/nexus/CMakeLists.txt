#
#  Copyright (c) 2024, The OpenThread Authors.
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

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(COMMON_INCLUDES
    ${PROJECT_SOURCE_DIR}/include
    ${PROJECT_SOURCE_DIR}/src
    ${PROJECT_SOURCE_DIR}/src/core
    ${PROJECT_SOURCE_DIR}/tests/nexus
    ${PROJECT_SOURCE_DIR}/tests/nexus/platform
    ${PROJECT_SOURCE_DIR}/examples/platforms/utils
    ${CMAKE_CURRENT_BINARY_DIR}
)

set(COMMON_COMPILE_OPTIONS
    -DOPENTHREAD_FTD=1
    -DOPENTHREAD_MTD=0
    -DOPENTHREAD_RADIO=0
    -DOPENTHREAD_CONFIG_USE_STD_NEW=1
)

option(OT_NEXUS_GRPC "Enable Nexus gRPC" OFF)

if(OT_NEXUS_GRPC AND NOT EMSCRIPTEN)
    find_package(gRPC CONFIG REQUIRED)
    find_package(Protobuf REQUIRED)

    # Find gRPC C++ plugin
    find_program(GRPC_CPP_PLUGIN grpc_cpp_plugin)
    if(NOT GRPC_CPP_PLUGIN)
        message(FATAL_ERROR "grpc_cpp_plugin not found! Please install protobuf-compiler-grpc")
    endif()

    # Generate simulation.pb.cc/h and simulation.grpc.pb.cc/h
    set(PROTO_FILE platform/simulation.proto)
    get_filename_component(PROTO_ABS_FILE ${PROTO_FILE} ABSOLUTE)
    get_filename_component(PROTO_PATH ${PROTO_ABS_FILE} PATH)

    set(SIMULATION_PROTO_SRCS
        ${CMAKE_CURRENT_BINARY_DIR}/simulation.pb.cc
        ${CMAKE_CURRENT_BINARY_DIR}/simulation.grpc.pb.cc
    )
    set(SIMULATION_PROTO_HDRS
        ${CMAKE_CURRENT_BINARY_DIR}/simulation.pb.h
        ${CMAKE_CURRENT_BINARY_DIR}/simulation.grpc.pb.h
    )

    add_custom_command(
        OUTPUT ${SIMULATION_PROTO_SRCS} ${SIMULATION_PROTO_HDRS}
        COMMAND ${Protobuf_PROTOC_EXECUTABLE}
        ARGS --cpp_out ${CMAKE_CURRENT_BINARY_DIR}
             --grpc_out ${CMAKE_CURRENT_BINARY_DIR}
             --plugin=protoc-gen-grpc=${GRPC_CPP_PLUGIN}
             -I ${PROTO_PATH}
             ${PROTO_ABS_FILE}
        DEPENDS ${PROTO_ABS_FILE}
        COMMENT "Generating simulation.pb.cc/h and simulation.grpc.pb.cc/h from ${PROTO_FILE}"
        VERBATIM
    )
endif()

set(NEXUS_PLATFORM_SOURCES
    platform/nexus_alarm.cpp
    platform/nexus_core.cpp
    platform/nexus_dns.cpp
    platform/nexus_infra_if.cpp
    platform/nexus_logging.cpp
    platform/nexus_mdns.cpp
    platform/nexus_misc.cpp
    platform/nexus_node.cpp
    platform/nexus_pcap.cpp
    platform/nexus_radio.cpp
    platform/nexus_radio_model.cpp
    platform/nexus_settings.cpp
    platform/nexus_sim.cpp
    platform/nexus_trel.cpp
    platform/nexus_udp.cpp
    ../../examples/platforms/utils/mac_frame.cpp
)

if(OT_NEXUS_GRPC)
    list(APPEND NEXUS_PLATFORM_SOURCES ${SIMULATION_PROTO_SRCS} platform/nexus_grpc.cpp)
endif()

add_library(ot-nexus-platform
    ${NEXUS_PLATFORM_SOURCES}
)

if(OT_NEXUS_GRPC)
    set_source_files_properties(${SIMULATION_PROTO_SRCS} PROPERTIES
        COMPILE_OPTIONS "-Wno-pedantic;-Wno-error"
    )
endif()

target_include_directories(ot-nexus-platform
    PRIVATE
        ${COMMON_INCLUDES}
)

target_compile_options(ot-nexus-platform
    PRIVATE
        ${COMMON_COMPILE_OPTIONS}
        ${OT_CFLAGS}
)

if(EMSCRIPTEN)
    target_link_options(ot-nexus-platform PRIVATE "--bind")
endif()

if(OT_NEXUS_GRPC)
    target_compile_definitions(ot-nexus-platform
        PRIVATE
            OPENTHREAD_NEXUS_CONFIG_GRPC_ENABLE=1
    )
endif()

target_link_libraries(ot-nexus-platform
    PRIVATE
        ot-config
        ${OT_MBEDTLS}
)

if(OT_NEXUS_GRPC)
    target_link_libraries(ot-nexus-platform
        PUBLIC
            protobuf::libprotobuf
            gRPC::grpc++
    )
endif()

set(COMMON_LIBS
    ot-nexus-platform
    openthread-ftd
    ot-nexus-platform
    ${OT_MBEDTLS}
    ot-config
    openthread-ftd
)

#----------------------------------------------------------------------------------------------------------------------

macro(ot_nexus_test name labels)

    # Macro to add an OpenThread nexus test.
    #
    # The test target will be named `nexus_{name}` and compiled from the source
    # file `test_{name}.cpp`. The `labels` argument assigns categories to the
    # test, allowing us to run specific subsets using `ctest -L {label}`.

    add_executable(nexus_${name}
        test_${name}.cpp ${ARGN}
    )

    target_include_directories(nexus_${name}
    PRIVATE
        ${COMMON_INCLUDES}
    )

    target_link_libraries(nexus_${name}
    PRIVATE
        ${COMMON_LIBS}
    )

    target_compile_options(nexus_${name}
    PRIVATE
        ${COMMON_COMPILE_OPTIONS}
        ${OT_CFLAGS}
    )

    add_test(NAME nexus_${name} COMMAND nexus_${name})

    set_tests_properties(nexus_${name} PROPERTIES LABELS "${labels}")
endmacro()


#----------------------------------------------------------------------------------------------------------------------
# Cert tests
ot_nexus_test(1_1_5_1_1 "cert;nexus")
ot_nexus_test(1_1_5_1_2 "cert;nexus")
ot_nexus_test(1_1_5_1_3 "cert;nexus")
ot_nexus_test(1_1_5_1_4 "cert;nexus")
ot_nexus_test(1_1_5_1_5 "cert;nexus")
ot_nexus_test(1_1_5_1_6 "cert;nexus")
ot_nexus_test(1_1_5_1_7 "cert;nexus")
ot_nexus_test(1_1_5_1_8 "cert;nexus")
ot_nexus_test(1_1_5_1_9 "cert;nexus")
ot_nexus_test(1_1_5_1_10 "cert;nexus")
ot_nexus_test(1_1_5_1_11 "cert;nexus")
ot_nexus_test(1_1_5_1_12 "cert;nexus")
ot_nexus_test(1_1_5_1_13 "cert;nexus")
ot_nexus_test(1_1_5_2_1 "cert;nexus")
ot_nexus_test(1_1_5_2_3 "cert;nexus")
ot_nexus_test(1_1_5_2_4 "cert;nexus")
ot_nexus_test(1_1_5_2_5 "cert;nexus")
ot_nexus_test(1_1_5_2_6 "cert;nexus")
ot_nexus_test(1_1_5_2_7 "cert;nexus")
ot_nexus_test(1_1_5_3_1 "cert;nexus")
ot_nexus_test(1_1_5_3_2 "cert;nexus")
ot_nexus_test(1_1_5_3_3 "cert;nexus")
ot_nexus_test(1_1_5_3_4 "cert;nexus")
ot_nexus_test(1_1_5_3_5 "cert;nexus")
ot_nexus_test(1_1_5_3_6 "cert;nexus")
ot_nexus_test(1_1_5_3_7 "cert;nexus")
ot_nexus_test(1_1_5_3_8 "cert;nexus")
ot_nexus_test(1_1_5_3_9 "cert;nexus")
ot_nexus_test(1_1_5_3_10 "cert;nexus")
ot_nexus_test(1_1_5_3_11 "cert;nexus")
ot_nexus_test(1_1_5_5_1 "cert;nexus")
ot_nexus_test(1_1_5_5_2 "cert;nexus")
ot_nexus_test(1_1_5_5_3 "cert;nexus")
ot_nexus_test(1_1_5_5_4_1 "cert;nexus")
ot_nexus_test(1_1_5_5_4_2 "cert;nexus")
ot_nexus_test(1_1_5_5_5 "cert;nexus")
ot_nexus_test(1_1_5_5_7 "cert;nexus")
ot_nexus_test(1_1_5_6_1 "cert;nexus")
ot_nexus_test(1_1_5_6_2 "cert;nexus")
ot_nexus_test(1_1_5_6_3 "cert;nexus")
ot_nexus_test(1_1_5_6_4 "cert;nexus")
ot_nexus_test(1_1_5_6_5 "cert;nexus")
ot_nexus_test(1_1_5_6_6 "cert;nexus")
ot_nexus_test(1_1_5_6_7 "cert;nexus")
ot_nexus_test(1_1_5_6_9 "cert;nexus")
ot_nexus_test(1_1_5_7_1 "cert;nexus")
ot_nexus_test(1_1_5_7_2 "cert;nexus")
ot_nexus_test(1_1_5_7_3 "cert;nexus")
ot_nexus_test(1_1_5_8_2 "cert;nexus")
ot_nexus_test(1_1_5_8_3 "cert;nexus")
ot_nexus_test(1_1_5_8_4 "cert;nexus")
ot_nexus_test(1_1_6_1_1 "cert;nexus")
ot_nexus_test(1_1_6_1_2 "cert;nexus")
ot_nexus_test(1_1_6_1_3 "cert;nexus")
ot_nexus_test(1_1_6_1_4 "cert;nexus")
ot_nexus_test(1_1_6_1_5 "cert;nexus")
ot_nexus_test(1_1_6_1_6 "cert;nexus")
ot_nexus_test(1_1_6_1_7 "cert;nexus")
ot_nexus_test(1_1_6_2_1 "cert;nexus")
ot_nexus_test(1_1_6_2_2 "cert;nexus")
ot_nexus_test(1_1_6_3_1 "cert;nexus")
ot_nexus_test(1_1_6_3_2 "cert;nexus")
ot_nexus_test(1_1_6_4_1 "cert;nexus")
ot_nexus_test(1_1_6_4_2 "cert;nexus")
ot_nexus_test(1_1_6_5_1 "cert;nexus")
ot_nexus_test(1_1_6_5_2 "cert;nexus")
ot_nexus_test(1_1_6_5_3 "cert;nexus")
ot_nexus_test(1_1_6_6_1 "cert;nexus")
ot_nexus_test(1_1_6_6_2 "cert;nexus")
ot_nexus_test(1_1_7_1_1 "cert;nexus")
ot_nexus_test(1_1_7_1_2 "cert;nexus")
ot_nexus_test(1_1_7_1_3 "cert;nexus")
ot_nexus_test(1_1_7_1_4 "cert;nexus")
ot_nexus_test(1_1_7_1_5 "cert;nexus")
ot_nexus_test(1_1_7_1_6 "cert;nexus")
ot_nexus_test(1_1_7_1_7 "cert;nexus")
ot_nexus_test(1_1_7_1_8 "cert;nexus")
ot_nexus_test(1_1_8_1_1 "cert;nexus")
ot_nexus_test(1_1_8_1_2 "cert;nexus")
ot_nexus_test(1_1_8_1_6 "cert;nexus")
ot_nexus_test(1_1_8_2_1 "cert;nexus")
ot_nexus_test(1_1_8_2_2 "cert;nexus")
ot_nexus_test(1_1_8_3_1 "cert;nexus")
ot_nexus_test(1_1_9_2_1 "cert;nexus")
ot_nexus_test(1_1_9_2_2 "cert;nexus")
ot_nexus_test(1_1_9_2_3 "cert;nexus")
ot_nexus_test(1_1_9_2_4 "cert;nexus")
ot_nexus_test(1_1_9_2_5 "cert;nexus")
ot_nexus_test(1_1_9_2_6 "cert;nexus")
ot_nexus_test(1_1_9_2_7 "cert;nexus")
ot_nexus_test(1_1_9_2_8 "cert;nexus")
ot_nexus_test(1_1_9_2_9 "cert;nexus")
ot_nexus_test(1_1_9_2_10 "cert;nexus")
ot_nexus_test(1_1_9_2_11 "cert;nexus")
ot_nexus_test(1_1_9_2_12 "cert;nexus")
ot_nexus_test(1_1_9_2_13 "cert;nexus")
ot_nexus_test(1_1_9_2_14 "cert;nexus")
ot_nexus_test(1_1_9_2_15 "cert;nexus")
ot_nexus_test(1_1_9_2_16 "cert;nexus")
ot_nexus_test(1_1_9_2_17 "cert;nexus")
ot_nexus_test(1_1_9_2_18 "cert;nexus")
ot_nexus_test(1_1_9_2_19 "cert;nexus")
ot_nexus_test(1_2_LP_5_2_1 "cert;nexus")
ot_nexus_test(1_2_LP_5_3_1 "cert;nexus")
ot_nexus_test(1_2_LP_5_3_2 "cert;nexus")
ot_nexus_test(1_2_LP_5_3_3 "cert;nexus")
ot_nexus_test(1_2_LP_5_3_4 "cert;nexus")
ot_nexus_test(1_2_LP_5_3_5 "cert;nexus")
ot_nexus_test(1_2_LP_5_3_6 "cert;nexus")
ot_nexus_test(1_2_LP_5_3_7 "cert;nexus")
ot_nexus_test(1_2_LP_5_3_8 "cert;nexus")
ot_nexus_test(1_2_LP_7_1_1 "cert;nexus")
ot_nexus_test(1_2_LP_7_1_2 "cert;nexus")
ot_nexus_test(1_2_LP_7_2_1 "cert;nexus")
ot_nexus_test(1_2_LP_7_2_2 "cert;nexus")
ot_nexus_test(1_2_MATN_TC_1 "cert;nexus")
ot_nexus_test(1_2_MATN_TC_2 "cert;nexus")
ot_nexus_test(1_2_MATN_TC_3 "cert;nexus")
ot_nexus_test(1_2_MATN_TC_4 "cert;nexus")
ot_nexus_test(1_2_MATN_TC_5 "cert;nexus")
ot_nexus_test(1_2_MATN_TC_7 "cert;nexus")
ot_nexus_test(1_2_MATN_TC_9 "cert;nexus")
ot_nexus_test(1_2_MATN_TC_10 "cert;nexus")
ot_nexus_test(1_2_MATN_TC_12 "cert;nexus")
ot_nexus_test(1_2_MATN_TC_15 "cert;nexus")
ot_nexus_test(1_2_MATN_TC_16 "cert;nexus")
ot_nexus_test(1_2_MATN_TC_19 "cert;nexus")
ot_nexus_test(1_2_MATN_TC_20 "cert;nexus")
ot_nexus_test(1_2_MATN_TC_21 "cert;nexus")
ot_nexus_test(1_2_MATN_TC_22 "cert;nexus")
ot_nexus_test(1_2_MATN_TC_23 "cert;nexus")
ot_nexus_test(1_2_MATN_TC_26 "cert;nexus")
ot_nexus_test(1_2_BBR_TC_1 "cert;nexus")
ot_nexus_test(1_2_BBR_TC_2 "cert;nexus")
ot_nexus_test(1_2_BBR_TC_3 "cert;nexus")
ot_nexus_test(1_3_DBR_TC_1 "cert;nexus")
ot_nexus_test(1_3_DBR_TC_2 "cert;nexus")
ot_nexus_test(1_3_DBR_TC_3 "cert;nexus")
ot_nexus_test(1_3_DBR_TC_6 "cert;nexus")
ot_nexus_test(1_3_DBR_TC_7A "cert;nexus")
ot_nexus_test(1_3_DBR_TC_7B "cert;nexus")
ot_nexus_test(1_3_DBR_TC_7C "cert;nexus")
ot_nexus_test(1_3_DBR_TC_8 "cert;nexus")
ot_nexus_test(1_3_DBR_TC_10 "cert;nexus")
ot_nexus_test(1_3_DPR_TC_1 "cert;nexus")
ot_nexus_test(1_3_DPR_TC_2 "cert;nexus")
ot_nexus_test(1_3_SRP_TC_1 "cert;nexus")
ot_nexus_test(1_3_SRP_TC_2 "cert;nexus")
ot_nexus_test(1_3_SRP_TC_3 "cert;nexus")
ot_nexus_test(1_3_SRP_TC_4 "cert;nexus")
ot_nexus_test(1_3_SRP_TC_5 "cert;nexus")
ot_nexus_test(1_3_SRP_TC_6 "cert;nexus")
ot_nexus_test(1_3_SRP_TC_8 "cert;nexus")
ot_nexus_test(1_3_SRP_TC_11 "cert;nexus")
ot_nexus_test(1_3_SRP_TC_12 "cert;nexus")
ot_nexus_test(1_3_SRP_TC_13 "cert;nexus")
ot_nexus_test(1_3_SRP_TC_15 "cert;nexus")
ot_nexus_test(1_3_SRPC_TC_1 "cert;nexus")
ot_nexus_test(1_3_SRPC_TC_4 "cert;nexus")
ot_nexus_test(1_3_SRPC_TC_5 "cert;nexus")
ot_nexus_test(1_3_SRPC_TC_7 "cert;nexus")
ot_nexus_test(1_3_GEN_TC_1 "cert;nexus")
ot_nexus_test(1_3_GEN_TC_2 "cert;nexus")
ot_nexus_test(1_3_DIAG_TC_1 "cert;nexus")
ot_nexus_test(1_3_DIAG_TC_2 "cert;nexus")
ot_nexus_test(1_4_TREL_TC_1 "cert;nexus")
ot_nexus_test(1_4_TREL_TC_2 "cert;nexus")
ot_nexus_test(1_4_TREL_TC_3 "cert;nexus")
ot_nexus_test(1_4_TREL_TC_4 "cert;nexus")
ot_nexus_test(1_4_TREL_TC_5 "cert;nexus")
ot_nexus_test(1_4_TREL_TC_6 "cert;nexus")
ot_nexus_test(1_4_DNS_TC_1 "cert;nexus")
ot_nexus_test(1_4_DNS_TC_3 "cert;nexus")
ot_nexus_test(1_4_DNS_TC_5 "cert;nexus")
ot_nexus_test(1_4_PIC_TC_1 "cert;nexus")
ot_nexus_test(1_4_PIC_TC_3 "cert;nexus")
ot_nexus_test(1_4_PIC_TC_4 "cert;nexus")
ot_nexus_test(1_4_CS_TC_3 "cert;nexus")
ot_nexus_test(inform_previous_parent_on_reattach "cert;nexus")

# Misc tests
ot_nexus_test(anycast "core;nexus")
ot_nexus_test(anycast_locator "core;nexus")
ot_nexus_test(br_upgrade_router_role "core;nexus")
ot_nexus_test(border_admitter "core;nexus")
ot_nexus_test(border_agent "core;nexus")
ot_nexus_test(border_agent_tracker "core;nexus")
ot_nexus_test(child_supervision "core;nexus")
ot_nexus_test(coap_block "core;nexus")
ot_nexus_test(coap_observe "core;nexus")
ot_nexus_test(coaps "core;nexus")
ot_nexus_test(compact_route_tlv "core;nexus")
ot_nexus_test(dataset_updater "core;nexus")
ot_nexus_test(discover_scan "core;nexus")
ot_nexus_test(dnssd "core;nexus")
ot_nexus_test(dns_client_config_auto_start "core;nexus")
ot_nexus_test(dnssd_name_with_special_chars "core;nexus")
ot_nexus_test(dtls "core;nexus")
ot_nexus_test(fed_rx_only_link_establishment "core;nexus")
ot_nexus_test(form_join "core;nexus")
ot_nexus_test(history_tracker "core;nexus")
ot_nexus_test(ipv6_fragmentation "core;nexus")
ot_nexus_test(ipv6_recursion "core;nexus")
ot_nexus_test(ipv6_source_selection "core;nexus")
ot_nexus_test(key_rotation_guard_time "core;nexus")
ot_nexus_test(leader_reboot_multiple_link_request "core;nexus")
ot_nexus_test(log_override "core;nexus")
ot_nexus_test(mac_scan "core;nexus")
ot_nexus_test(mle_router_role_allowed "core;nexus")
ot_nexus_test(mle_blocking_downgrade "core;nexus")
ot_nexus_test(mle_msg_key_seq_jump "core;nexus")
ot_nexus_test(mlr_redundant "core;nexus")
ot_nexus_test(nat64_translator "core;nexus")
ot_nexus_test(netdata_publisher "core;nexus")
ot_nexus_test(on_mesh_prefix "core;nexus")
ot_nexus_test(pbbr_aloc "core;nexus")
ot_nexus_test(ping_lla_src "core;nexus")
ot_nexus_test(radio_filter "core;nexus")
ot_nexus_test(reed_address_solicit_rejected "core;nexus")
ot_nexus_test(reset "core;nexus")
ot_nexus_test(router_downgrade_on_sec_policy_change "core;nexus")
ot_nexus_test(router_multicast_link_request "core;nexus")
ot_nexus_test(router_reattach "core;nexus")
ot_nexus_test(router_reboot_multiple_link_request "core;nexus")
ot_nexus_test(service "core;nexus")
ot_nexus_test(srp_auto_start "core;nexus")
ot_nexus_test(srp_client_change_lease "core;nexus")
ot_nexus_test(srp_client_remove_host "core;nexus")
ot_nexus_test(srp_client_save_server_info "core;nexus")
ot_nexus_test(srp_lease "core;nexus")
ot_nexus_test(srp_many_services_mtu_check "core;nexus")
ot_nexus_test(srp_register_services_diff_lease "core;nexus")
ot_nexus_test(srp_scale "core;nexus")
ot_nexus_test(srp_server_anycast_mode "core;nexus")
ot_nexus_test(srp_server_reboot_port "core;nexus")
ot_nexus_test(srp_ttl "core;nexus")
ot_nexus_test(tmf_origin "core;nexus")
ot_nexus_test(zero_len_external_route "core;nexus")

# Trel
ot_nexus_test(trel "trel;nexus")

# Long-routes
ot_nexus_test(long_routes "long_routes;nexus")

# Grpc
if(OT_NEXUS_GRPC)
    ot_nexus_test(grpc "core;nexus")
    target_compile_definitions(nexus_grpc PRIVATE OPENTHREAD_NEXUS_CONFIG_GRPC_ENABLE=1)
endif()

# Large network
ot_nexus_test(full_network_reset "core;large_network;nexus")
ot_nexus_test(large_network "core;large_network;nexus")

# Live Demo Persistent Server
if(EMSCRIPTEN)
    set(NEXUS_WASM_LINK_OPTIONS
        "--bind"
        "-sEXPORT_ES6=1"
        "-sMODULARIZE=1"
        "-sEXPORTED_RUNTIME_METHODS=['ccall','cwrap']"
        "-sSTACK_SIZE=4194304"
        "-sALLOW_MEMORY_GROWTH=1"
    )

    add_executable(nexus_live_demo
        platform/nexus_wasm.cpp
    )
    target_link_options(nexus_live_demo PRIVATE ${NEXUS_WASM_LINK_OPTIONS})
    set_target_properties(nexus_live_demo PROPERTIES CXX_STANDARD 17)
elseif(OT_NEXUS_GRPC)
    add_executable(nexus_live_demo
        platform/nexus_native.cpp
    )
endif()

if(TARGET nexus_live_demo)
    target_include_directories(nexus_live_demo PRIVATE ${COMMON_INCLUDES})
    target_link_libraries(nexus_live_demo PRIVATE ${COMMON_LIBS})
    target_compile_options(nexus_live_demo PRIVATE ${COMMON_COMPILE_OPTIONS} ${OT_CFLAGS})
    if(EMSCRIPTEN)
        add_test(NAME nexus_wasm_bindings COMMAND node ${CMAKE_CURRENT_SOURCE_DIR}/test_wasm_bindings.mjs ${CMAKE_CURRENT_BINARY_DIR}/nexus_live_demo.js)
    elseif(OT_NEXUS_GRPC)
        target_compile_definitions(nexus_live_demo PRIVATE OPENTHREAD_NEXUS_CONFIG_GRPC_ENABLE=1)
    endif()
endif()
