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

set(COMMON_INCLUDES
    ${PROJECT_SOURCE_DIR}/include
    ${PROJECT_SOURCE_DIR}/src
    ${PROJECT_SOURCE_DIR}/src/core
    ${PROJECT_SOURCE_DIR}/tests/nexus
    ${PROJECT_SOURCE_DIR}/tests/nexus/platform
)

set(COMMON_COMPILE_OPTIONS
    -DOPENTHREAD_FTD=1
    -DOPENTHREAD_MTD=0
    -DOPENTHREAD_RADIO=0
)

add_library(ot-nexus-platform
    platform/nexus_alarm.cpp
    platform/nexus_core.cpp
    platform/nexus_infra_if.cpp
    platform/nexus_mdns.cpp
    platform/nexus_misc.cpp
    platform/nexus_node.cpp
    platform/nexus_pcap.cpp
    platform/nexus_radio.cpp
    platform/nexus_settings.cpp
    platform/nexus_trel.cpp
)

target_include_directories(ot-nexus-platform
    PRIVATE
        ${COMMON_INCLUDES}
)

target_compile_options(ot-nexus-platform
    PRIVATE
        ${COMMON_COMPILE_OPTIONS}
        ${OT_CFLAGS}
)

target_link_libraries(ot-nexus-platform
    PRIVATE
        ot-config
        ${OT_MBEDTLS}
)

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
ot_nexus_test(5_1_1 "cert;nexus")
ot_nexus_test(5_1_2 "cert;nexus")
ot_nexus_test(5_1_3 "cert;nexus")
ot_nexus_test(5_1_4 "cert;nexus")
ot_nexus_test(5_1_5 "cert;nexus")
ot_nexus_test(5_1_6 "cert;nexus")
ot_nexus_test(5_1_7 "cert;nexus")
ot_nexus_test(5_1_8 "cert;nexus")
ot_nexus_test(5_1_9 "cert;nexus")
ot_nexus_test(5_1_10 "cert;nexus")
ot_nexus_test(5_1_11 "cert;nexus")
ot_nexus_test(5_1_12 "cert;nexus")
ot_nexus_test(5_1_13 "cert;nexus")
ot_nexus_test(5_2_1 "cert;nexus")
ot_nexus_test(5_2_3 "cert;nexus")
ot_nexus_test(5_2_4 "cert;nexus")
ot_nexus_test(5_2_5 "cert;nexus")
ot_nexus_test(5_2_6 "cert;nexus")
ot_nexus_test(5_2_7 "cert;nexus")
ot_nexus_test(5_3_1 "cert;nexus")
ot_nexus_test(5_3_2 "cert;nexus")
ot_nexus_test(5_3_3 "cert;nexus")
ot_nexus_test(5_3_4 "cert;nexus")
ot_nexus_test(5_3_5 "cert;nexus")
ot_nexus_test(5_3_6 "cert;nexus")
ot_nexus_test(5_3_7 "cert;nexus")
ot_nexus_test(5_3_8 "cert;nexus")
ot_nexus_test(5_3_9 "cert;nexus")
ot_nexus_test(5_3_10 "cert;nexus")
ot_nexus_test(5_3_11 "cert;nexus")
ot_nexus_test(5_5_1 "cert;nexus")
ot_nexus_test(5_5_2 "cert;nexus")
ot_nexus_test(5_5_3 "cert;nexus")
ot_nexus_test(5_5_4_1 "cert;nexus")
ot_nexus_test(5_5_4_2 "cert;nexus")
ot_nexus_test(5_5_5 "cert;nexus")
ot_nexus_test(5_5_7 "cert;nexus")
ot_nexus_test(5_6_1 "cert;nexus")
ot_nexus_test(5_7_1 "cert;nexus")
ot_nexus_test(5_7_2 "cert;nexus")
ot_nexus_test(5_7_3 "cert;nexus")
ot_nexus_test(5_8_2 "cert;nexus")
ot_nexus_test(5_8_3 "cert;nexus")
ot_nexus_test(6_1_1 "cert;nexus")
ot_nexus_test(6_1_2 "cert;nexus")
ot_nexus_test(6_1_3 "cert;nexus")
ot_nexus_test(6_1_4 "cert;nexus")
ot_nexus_test(6_1_5 "cert;nexus")
ot_nexus_test(6_2_1 "cert;nexus")
ot_nexus_test(6_2_2 "cert;nexus")
ot_nexus_test(6_3_1 "cert;nexus")
ot_nexus_test(6_3_2 "cert;nexus")
ot_nexus_test(6_4_1 "cert;nexus")
ot_nexus_test(6_5_1 "cert;nexus")
ot_nexus_test(7_1_1 "cert;nexus")

# Misc tests
ot_nexus_test(border_admitter "core;nexus")
ot_nexus_test(border_agent "core;nexus")
ot_nexus_test(border_agent_tracker "core;nexus")
ot_nexus_test(discover_scan "core;nexus")
ot_nexus_test(dtls "core;nexus")
ot_nexus_test(form_join "core;nexus")
ot_nexus_test(nat64_translator "core;nexus")

# Trel
ot_nexus_test(trel "trel;nexus")

# Large network
ot_nexus_test(full_network_reset "core;large_network;nexus")
ot_nexus_test(large_network "core;large_network;nexus")
