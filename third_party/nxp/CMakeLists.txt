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

if(OT_PLATFORM STREQUAL "kw41z")
    set_property(SOURCE ${PROJECT_SOURCE_DIR}/third_party/nxp/MKW41Z4/startup_MKW41Z4.S PROPERTY LANGUAGE C)

    add_library(nxp-kw41z-driver
        MKW41Z4/clock_config.c
        MKW41Z4/drivers/fsl_clock.c
        MKW41Z4/drivers/fsl_flash.c
        MKW41Z4/drivers/fsl_lpuart.c
        MKW41Z4/drivers/fsl_gpio.c
        MKW41Z4/drivers/fsl_pit.c
        MKW41Z4/drivers/fsl_trng.c
        MKW41Z4/startup_MKW41Z4.S
        MKW41Z4/system_MKW41Z4.c
        MKW41Z4/XCVR/cfgs_kw4x_3x_2x/fsl_xcvr_ant_config.c
        MKW41Z4/XCVR/cfgs_kw4x_3x_2x/fsl_xcvr_ble_config.c
        MKW41Z4/XCVR/cfgs_kw4x_3x_2x/fsl_xcvr_common_config.c
        MKW41Z4/XCVR/cfgs_kw4x_3x_2x/fsl_xcvr_gfsk_bt_0p3_h_0p5_config.c
        MKW41Z4/XCVR/cfgs_kw4x_3x_2x/fsl_xcvr_gfsk_bt_0p5_h_0p32_config.c
        MKW41Z4/XCVR/cfgs_kw4x_3x_2x/fsl_xcvr_gfsk_bt_0p5_h_0p5_config.c
        MKW41Z4/XCVR/cfgs_kw4x_3x_2x/fsl_xcvr_gfsk_bt_0p5_h_0p7_config.c
        MKW41Z4/XCVR/cfgs_kw4x_3x_2x/fsl_xcvr_gfsk_bt_0p5_h_1p0_config.c
        MKW41Z4/XCVR/cfgs_kw4x_3x_2x/fsl_xcvr_gfsk_bt_0p7_h_0p5_config.c
        MKW41Z4/XCVR/cfgs_kw4x_3x_2x/fsl_xcvr_mode_datarate_config.c
        MKW41Z4/XCVR/cfgs_kw4x_3x_2x/fsl_xcvr_msk_config.c
        MKW41Z4/XCVR/cfgs_kw4x_3x_2x/fsl_xcvr_zgbe_config.c
        MKW41Z4/XCVR/dbg_ram_capture.c
        MKW41Z4/XCVR/fsl_xcvr.c
        MKW41Z4/XCVR/fsl_xcvr_trim.c
        MKW41Z4/XCVR/ifr_radio.c
    )

    target_compile_definitions(nxp-kw41z-driver
        PUBLIC
            ${OT_PLATFORM_DEFINES}
            -DCPU_MKW41Z512VHT4
    )

    target_compile_options(nxp-kw41z-driver
        PRIVATE
            ${OT_CFLAGS}
            -Wno-unknown-pragmas
            -Wno-sign-compare
            -Wno-unused-function
            -Wno-unused-parameter
            -Wno-empty-body
    )

    target_include_directories(nxp-kw41z-driver
        PUBLIC
            CMSIS/Include
            MKW41Z4
            MKW41Z4/drivers
            MKW41Z4/XCVR
            MKW41Z4/XCVR/cfgs_kw4x_3x_2x
    )

    target_link_libraries(nxp-kw41z-driver PRIVATE ot-config)
endif()
