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

set(OT_PLATFORM_LIB "openthread-kw41z" PARENT_SCOPE)

if(NOT OT_CONFIG)
    set(OT_CONFIG "openthread-core-kw41z-config.h")
    set(OT_CONFIG ${OT_CONFIG} PARENT_SCOPE)
endif()

list(APPEND OT_PLATFORM_DEFINES
    "OPENTHREAD_CORE_CONFIG_PLATFORM_CHECK_FILE=\"openthread-core-kw41z-config-check.h\""
    "OPENTHREAD_SETTINGS_RAM=0"
)
set(OT_PLATFORM_DEFINES ${OT_PLATFORM_DEFINES} PARENT_SCOPE)

list(APPEND OT_PLATFORM_DEFINES "OPENTHREAD_PROJECT_CORE_CONFIG_FILE=\"${OT_CONFIG}\"")

add_library(openthread-kw41z
    alarm.c
    diag.c
    entropy.c
    flash.c
    misc.c
    radio.c
    system.c
    logging.c
    uart.c
    ${PROJECT_SOURCE_DIR}/third_party/nxp/MKW41Z4/startup_MKW41Z4.S
    ${PROJECT_SOURCE_DIR}/third_party/nxp/MKW41Z4/clock_config.c
    ${PROJECT_SOURCE_DIR}/third_party/nxp/MKW41Z4/system_MKW41Z4.c
    ${PROJECT_SOURCE_DIR}/third_party/nxp/MKW41Z4/drivers/fsl_clock.c
    ${PROJECT_SOURCE_DIR}/third_party/nxp/MKW41Z4/drivers/fsl_flash.c
    ${PROJECT_SOURCE_DIR}/third_party/nxp/MKW41Z4/drivers/fsl_lpuart.c
    ${PROJECT_SOURCE_DIR}/third_party/nxp/MKW41Z4/drivers/fsl_gpio.c
    ${PROJECT_SOURCE_DIR}/third_party/nxp/MKW41Z4/drivers/fsl_pit.c
    ${PROJECT_SOURCE_DIR}/third_party/nxp/MKW41Z4/drivers/fsl_trng.c
    ${PROJECT_SOURCE_DIR}/third_party/nxp/MKW41Z4/XCVR/dbg_ram_capture.c
    ${PROJECT_SOURCE_DIR}/third_party/nxp/MKW41Z4/XCVR/fsl_xcvr.c
    ${PROJECT_SOURCE_DIR}/third_party/nxp/MKW41Z4/XCVR/fsl_xcvr_trim.c
    ${PROJECT_SOURCE_DIR}/third_party/nxp/MKW41Z4/XCVR/ifr_radio.c
    ${PROJECT_SOURCE_DIR}/third_party/nxp/MKW41Z4/XCVR/cfgs_kw4x_3x_2x/fsl_xcvr_ant_config.c
    ${PROJECT_SOURCE_DIR}/third_party/nxp/MKW41Z4/XCVR/cfgs_kw4x_3x_2x/fsl_xcvr_ble_config.c
    ${PROJECT_SOURCE_DIR}/third_party/nxp/MKW41Z4/XCVR/cfgs_kw4x_3x_2x/fsl_xcvr_common_config.c
    ${PROJECT_SOURCE_DIR}/third_party/nxp/MKW41Z4/XCVR/cfgs_kw4x_3x_2x/fsl_xcvr_gfsk_bt_0p3_h_0p5_config.c
    ${PROJECT_SOURCE_DIR}/third_party/nxp/MKW41Z4/XCVR/cfgs_kw4x_3x_2x/fsl_xcvr_gfsk_bt_0p5_h_0p32_config.c
    ${PROJECT_SOURCE_DIR}/third_party/nxp/MKW41Z4/XCVR/cfgs_kw4x_3x_2x/fsl_xcvr_gfsk_bt_0p5_h_0p5_config.c
    ${PROJECT_SOURCE_DIR}/third_party/nxp/MKW41Z4/XCVR/cfgs_kw4x_3x_2x/fsl_xcvr_gfsk_bt_0p5_h_0p7_config.c
    ${PROJECT_SOURCE_DIR}/third_party/nxp/MKW41Z4/XCVR/cfgs_kw4x_3x_2x/fsl_xcvr_gfsk_bt_0p5_h_1p0_config.c
    ${PROJECT_SOURCE_DIR}/third_party/nxp/MKW41Z4/XCVR/cfgs_kw4x_3x_2x/fsl_xcvr_gfsk_bt_0p7_h_0p5_config.c
    ${PROJECT_SOURCE_DIR}/third_party/nxp/MKW41Z4/XCVR/cfgs_kw4x_3x_2x/fsl_xcvr_mode_datarate_config.c
    ${PROJECT_SOURCE_DIR}/third_party/nxp/MKW41Z4/XCVR/cfgs_kw4x_3x_2x/fsl_xcvr_msk_config.c
    ${PROJECT_SOURCE_DIR}/third_party/nxp/MKW41Z4/XCVR/cfgs_kw4x_3x_2x/fsl_xcvr_zgbe_config.c
    $<TARGET_OBJECTS:openthread-platform-utils>
)

set_property(TARGET openthread-kw41z PROPERTY C_STANDARD 99)

target_link_libraries(openthread-kw41z PRIVATE openthread-platform-utils)
target_link_options(openthread-kw41z PRIVATE -T${PROJECT_SOURCE_DIR}/examples/platforms/kw41z/MKW41Z512xxx4.ld)
target_link_options(openthread-kw41z PUBLIC -Wl,-Map=$<TARGET_PROPERTY:NAME>.map)

target_compile_definitions(openthread-kw41z
    PUBLIC
        ${OT_PLATFORM_DEFINES}
	-DCPU_MKW41Z512VHT4
    PRIVATE
        ${OT_PRIVATE_DEFINES}
)

target_compile_options(openthread-kw41z PRIVATE
    ${OT_CFLAGS}
    -Wno-unknown-pragmas
    -Wno-sign-compare
    -Wno-unused-function
    -Wno-unused-parameter
    -Wno-empty-body
)

target_include_directories(openthread-kw41z PRIVATE
    ${OT_PUBLIC_INCLUDES}
    ${OT_PRIVATE_INCLUDES}
    ${PROJECT_SOURCE_DIR}/examples/platforms
    ${PROJECT_SOURCE_DIR}/src/core
    PUBLIC
        ${PROJECT_SOURCE_DIR}/third_party/nxp
        ${PROJECT_SOURCE_DIR}/third_party/nxp/CMSIS/Include
        ${PROJECT_SOURCE_DIR}/third_party/nxp/MKW41Z4
        ${PROJECT_SOURCE_DIR}/third_party/nxp/MKW41Z4/drivers
        ${PROJECT_SOURCE_DIR}/third_party/nxp/MKW41Z4/XCVR
        ${PROJECT_SOURCE_DIR}/third_party/nxp/MKW41Z4/XCVR/cfgs_kw4x_3x_2x
)
