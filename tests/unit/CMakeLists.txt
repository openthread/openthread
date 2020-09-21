#
#  Copyright (c) 2020, The OpenThread Authors.
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

set(COMMON_SOURCES
    test_platform.cpp
    test_util.cpp
)

set(COMMON_LIBS
    openthread-ftd
    ot-config
    ${OT_MBEDTLS}
    util
)

set(COMMON_INCLUDES
    ${PROJECT_SOURCE_DIR}/include
    ${PROJECT_SOURCE_DIR}/src
    ${PROJECT_SOURCE_DIR}/src/core
    ${PROJECT_SOURCE_DIR}/examples/platforms/simulation
)

set(COMMON_COMPILE_OPTIONS
    -DOPENTHREAD_FTD=1
    -DOPENTHREAD_SPINEL_CONFIG_OPENTHREAD_MESSAGE_ENABLE=1
)

add_executable(test-aes
    ${COMMON_SOURCES}
    test_aes.cpp
)

target_include_directories(test-aes
    PRIVATE
        ${COMMON_INCLUDES}
)

target_compile_options(test-aes
    PRIVATE
        ${COMMON_COMPILE_OPTIONS}
)

target_link_libraries(test-aes
    PRIVATE
        ${COMMON_LIBS}
)

add_test(NAME test-aes COMMAND test-aes)

add_executable(test-child
    ${COMMON_SOURCES}
    test_child.cpp
)

add_executable(test-checksum
    ${COMMON_SOURCES}
    test_checksum.cpp
)

target_include_directories(test-checksum
    PRIVATE
        ${COMMON_INCLUDES}
)

target_compile_options(test-checksum
    PRIVATE
        ${COMMON_COMPILE_OPTIONS}
)

target_link_libraries(test-checksum
    PRIVATE
        ${COMMON_LIBS}
)

add_test(NAME test-checksum COMMAND test-checksum)


target_include_directories(test-child
    PRIVATE
        ${COMMON_INCLUDES}
)

target_compile_options(test-child
    PRIVATE
        ${COMMON_COMPILE_OPTIONS}
)

target_link_libraries(test-child
    PRIVATE
        ${COMMON_LIBS}
)

add_test(NAME test-child COMMAND test-child)

add_executable(test-child-table
    ${COMMON_SOURCES}
    test_child_table.cpp
)

target_include_directories(test-child-table
    PRIVATE
        ${COMMON_INCLUDES}
)

target_compile_options(test-child-table
    PRIVATE
        ${COMMON_COMPILE_OPTIONS}
)

target_link_libraries(test-child-table
    PRIVATE
        ${COMMON_LIBS}
)

add_test(NAME test-child-table COMMAND test-child-table)

add_executable(test-flash
    ${COMMON_SOURCES}
    test_flash.cpp
)

target_include_directories(test-flash
    PRIVATE
        ${COMMON_INCLUDES}
)

target_compile_options(test-flash
    PRIVATE
        ${COMMON_COMPILE_OPTIONS}
)

target_link_libraries(test-flash
    PRIVATE
        ${COMMON_LIBS}
)

add_test(NAME test-flash COMMAND test-flash)

add_executable(test-heap
    ${COMMON_SOURCES}
    test_heap.cpp
)

target_include_directories(test-heap
    PRIVATE
        ${COMMON_INCLUDES}
)

target_compile_options(test-heap
    PRIVATE
        ${COMMON_COMPILE_OPTIONS}
)

target_link_libraries(test-heap
    PRIVATE
        ${COMMON_LIBS}
)

add_test(NAME test-heap COMMAND test-heap)

add_executable(test-hmac-sha256
    ${COMMON_SOURCES}
    test_hmac_sha256.cpp
)

target_include_directories(test-hmac-sha256
    PRIVATE
        ${COMMON_INCLUDES}
)

target_compile_options(test-hmac-sha256
    PRIVATE
        ${COMMON_COMPILE_OPTIONS}
)

target_link_libraries(test-hmac-sha256
    PRIVATE
        ${COMMON_LIBS}
)

add_test(NAME test-hmac-sha256 COMMAND test-hmac-sha256)

add_executable(test-ip6-address
    ${COMMON_SOURCES}
    test_ip6_address.cpp
)

target_include_directories(test-ip6-address
    PRIVATE
        ${COMMON_INCLUDES}
)

target_compile_options(test-ip6-address
    PRIVATE
        ${COMMON_COMPILE_OPTIONS}
)

target_link_libraries(test-ip6-address
    PRIVATE
        ${COMMON_LIBS}
)

add_test(NAME test-ip6-address COMMAND test-ip6-address)

add_executable(test-link-quality
    ${COMMON_SOURCES}
    test_link_quality.cpp
)

target_include_directories(test-link-quality
    PRIVATE
        ${COMMON_INCLUDES}
)

target_compile_options(test-link-quality
    PRIVATE
        ${COMMON_COMPILE_OPTIONS}
)

target_link_libraries(test-link-quality
    PRIVATE
        ${COMMON_LIBS}
)

add_test(NAME test-link-quality COMMAND test-link-quality)

add_executable(test-linked-list
    ${COMMON_SOURCES}
    test_linked_list.cpp
)

target_include_directories(test-linked-list
    PRIVATE
        ${COMMON_INCLUDES}
)

target_compile_options(test-linked-list
    PRIVATE
        ${COMMON_COMPILE_OPTIONS}
)

target_link_libraries(test-linked-list
    PRIVATE
        ${COMMON_LIBS}
)

add_test(NAME test-linked-list COMMAND test-linked-list)

add_executable(test-lowpan
    ${COMMON_SOURCES}
    test_lowpan.cpp
)

target_include_directories(test-lowpan
    PRIVATE
        ${COMMON_INCLUDES}
)

target_compile_options(test-lowpan
    PRIVATE
        ${COMMON_COMPILE_OPTIONS}
)

target_link_libraries(test-lowpan
    PRIVATE
        ${COMMON_LIBS}
)

add_test(NAME test-lowpan COMMAND test-lowpan)

add_executable(test-mac-frame
    ${COMMON_SOURCES}
    test_mac_frame.cpp
)

target_include_directories(test-mac-frame
    PRIVATE
        ${COMMON_INCLUDES}
)

target_compile_options(test-mac-frame
    PRIVATE
        ${COMMON_COMPILE_OPTIONS}
)

target_link_libraries(test-mac-frame
    PRIVATE
        ${COMMON_LIBS}
)

add_test(NAME test-mac-frame COMMAND test-mac-frame)

add_executable(test-message
    ${COMMON_SOURCES}
    test_message.cpp
)

target_include_directories(test-message
    PRIVATE
        ${COMMON_INCLUDES}
)

target_compile_options(test-message
    PRIVATE
        ${COMMON_COMPILE_OPTIONS}
)

target_link_libraries(test-message
    PRIVATE
        ${COMMON_LIBS}
)

add_test(NAME test-message COMMAND test-message)

add_executable(test-message-queue
    ${COMMON_SOURCES}
    test_message_queue.cpp
)

target_include_directories(test-message-queue
    PRIVATE
        ${COMMON_INCLUDES}
)

target_compile_options(test-message-queue
    PRIVATE
        ${COMMON_COMPILE_OPTIONS}
)

target_link_libraries(test-message-queue
    PRIVATE
        ${COMMON_LIBS}
)

add_test(NAME test-message-queue COMMAND test-message-queue)

add_executable(test-multicast-listeners-table
    ${COMMON_SOURCES}
    test_multicast_listeners_table.cpp
)

target_include_directories(test-multicast-listeners-table
    PRIVATE
        ${COMMON_INCLUDES}
)

target_compile_options(test-multicast-listeners-table
    PRIVATE
        ${COMMON_COMPILE_OPTIONS}
)

target_link_libraries(test-multicast-listeners-table
        PRIVATE
        ${COMMON_LIBS}
    )

add_test(NAME test-multicast-listeners-table COMMAND test-multicast-listeners-table)

add_executable(test-ndproxy-table
    ${COMMON_SOURCES}
    test_ndproxy_table.cpp
)

target_include_directories(test-ndproxy-table
    PRIVATE
        ${COMMON_INCLUDES}
)

target_compile_options(test-ndproxy-table
    PRIVATE
        ${COMMON_COMPILE_OPTIONS}
)


target_link_libraries(test-ndproxy-table
    PRIVATE
        ${COMMON_LIBS}
)

add_test(NAME test-ndproxy-table COMMAND test-ndproxy-table)


add_executable(test-netif
    ${COMMON_SOURCES}
    test_netif.cpp
)

target_include_directories(test-netif
    PRIVATE
        ${COMMON_INCLUDES}
)

target_compile_options(test-netif
    PRIVATE
        ${COMMON_COMPILE_OPTIONS}
)

target_link_libraries(test-netif
    PRIVATE
        ${COMMON_LIBS}
)

add_test(NAME test-netif COMMAND test-netif)

add_executable(test-network-data
    ${COMMON_SOURCES}
    test_network_data.cpp
)

target_include_directories(test-network-data
    PRIVATE
        ${COMMON_INCLUDES}
)

target_compile_options(test-network-data
    PRIVATE
        ${COMMON_COMPILE_OPTIONS}
)

target_link_libraries(test-network-data
    PRIVATE
        ${COMMON_LIBS}
)

add_test(NAME test-network-data COMMAND test-network-data)

add_executable(test-pool
    ${COMMON_SOURCES}
    test_pool.cpp
)

target_include_directories(test-pool
    PRIVATE
        ${COMMON_INCLUDES}
)

target_compile_options(test-pool
    PRIVATE
        ${COMMON_COMPILE_OPTIONS}
)

target_link_libraries(test-pool
    PRIVATE
        ${COMMON_LIBS}
)

add_test(NAME test-pool COMMAND test-pool)

add_executable(test-priority-queue
    ${COMMON_SOURCES}
    test_priority_queue.cpp
)

target_include_directories(test-priority-queue
    PRIVATE
        ${COMMON_INCLUDES}
)

target_compile_options(test-priority-queue
    PRIVATE
        ${COMMON_COMPILE_OPTIONS}
)

target_link_libraries(test-priority-queue
    PRIVATE
        ${COMMON_LIBS}
)

add_test(NAME test-priority-queue COMMAND test-priority-queue)

add_executable(test-pskc
    ${COMMON_SOURCES}
    test_pskc.cpp
)

target_include_directories(test-pskc
    PRIVATE
        ${COMMON_INCLUDES}
)

target_compile_options(test-pskc
    PRIVATE
        ${COMMON_COMPILE_OPTIONS}
)

target_link_libraries(test-pskc
    PRIVATE
        ${COMMON_LIBS}
)

add_test(NAME test-pskc COMMAND test-pskc)

add_executable(test-steering-data
    ${COMMON_SOURCES}
    test_steering_data.cpp
)

target_include_directories(test-steering-data
    PRIVATE
        ${COMMON_INCLUDES}
)

target_compile_options(test-steering-data
    PRIVATE
        ${COMMON_COMPILE_OPTIONS}
)

target_link_libraries(test-steering-data
    PRIVATE
        ${COMMON_LIBS}
)

add_test(NAME test-steering-data COMMAND test-steering-data)

add_executable(test-string
    ${COMMON_SOURCES}
    test_string.cpp
)

target_include_directories(test-string
    PRIVATE
        ${COMMON_INCLUDES}
)

target_compile_options(test-string
    PRIVATE
        ${COMMON_COMPILE_OPTIONS}
)

target_link_libraries(test-string
    PRIVATE
        ${COMMON_LIBS}
)

add_test(NAME test-string COMMAND test-string)

add_executable(test-timer
    ${COMMON_SOURCES}
    test_timer.cpp
)

target_include_directories(test-timer
    PRIVATE
        ${COMMON_INCLUDES}
)

target_compile_options(test-timer
    PRIVATE
        ${COMMON_COMPILE_OPTIONS}
)

target_link_libraries(test-timer
    PRIVATE
        ${COMMON_LIBS}
)

add_test(NAME test-timer COMMAND test-timer)

set_target_properties(
    test-aes
    test-checksum
    test-child
    test-child-table
    test-flash
    test-heap
    test-hmac-sha256
    test-ip6-address
    test-link-quality
    test-linked-list
    test-lowpan
    test-mac-frame
    test-message
    test-message-queue
    test-multicast-listeners-table
    test-ndproxy-table
    test-netif
    test-network-data
    test-pool
    test-priority-queue
    test-pskc
    test-steering-data
    test-string
    test-timer
    PROPERTIES
        C_STANDARD 99
        CXX_STANDARD 11
)
