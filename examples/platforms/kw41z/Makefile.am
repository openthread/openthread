#
#  Copyright (c) 2017, The OpenThread Authors.
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

include $(abs_top_nlbuild_autotools_dir)/automake/pre.am

lib_LIBRARIES = libopenthread-kw41z.a

libopenthread_kw41z_a_CPPFLAGS                                                                     = \
    -DCPU_MKW41Z512VHT4                                                                              \
    -I$(top_srcdir)/include                                                                          \
    -I$(top_srcdir)/examples/platforms                                                               \
    -I$(top_srcdir)/src/core                                                                         \
    -I$(top_srcdir)/third_party/nxp                                                                  \
    -I$(top_srcdir)/third_party/nxp/CMSIS/Include                                                    \
    -I$(top_srcdir)/third_party/nxp/MKW41Z4                                                          \
    -I$(top_srcdir)/third_party/nxp/MKW41Z4/drivers                                                  \
    -I$(top_srcdir)/third_party/nxp/MKW41Z4/XCVR                                                     \
    -I$(top_srcdir)/third_party/nxp/MKW41Z4/XCVR/cfgs_kw4x_3x_2x                                     \
    -Wno-unknown-pragmas                                                                             \
    -Wno-sign-compare                                                                                \
    -Wno-unused-function                                                                             \
    -Wno-unused-parameter                                                                            \
    -Wno-empty-body                                                                                  \
    $(NULL)

PLATFORM_SOURCES                                                                                   = \
    alarm.c                                                                                          \
    diag.c                                                                                           \
    flash.c                                                                                          \
    logging.c                                                                                        \
    misc.c                                                                                           \
    radio.c                                                                                          \
    random.c                                                                                         \
    system.c                                                                                         \
    uart.c                                                                                           \
    $(NULL)

libopenthread_kw41z_a_SOURCES                                                                      = \
    @top_builddir@/third_party/nxp/MKW41Z4/startup_MKW41Z4.S                                         \
    @top_builddir@/third_party/nxp/MKW41Z4/clock_config.c                                            \
    @top_builddir@/third_party/nxp/MKW41Z4/system_MKW41Z4.c                                          \
    @top_builddir@/third_party/nxp/MKW41Z4/drivers/fsl_clock.c                                       \
    @top_builddir@/third_party/nxp/MKW41Z4/drivers/fsl_flash.c                                       \
    @top_builddir@/third_party/nxp/MKW41Z4/drivers/fsl_lpuart.c                                      \
    @top_builddir@/third_party/nxp/MKW41Z4/drivers/fsl_gpio.c                                        \
    @top_builddir@/third_party/nxp/MKW41Z4/drivers/fsl_pit.c                                         \
    @top_builddir@/third_party/nxp/MKW41Z4/drivers/fsl_trng.c                                        \
    @top_builddir@/third_party/nxp/MKW41Z4/XCVR/dbg_ram_capture.c                                    \
    @top_builddir@/third_party/nxp/MKW41Z4/XCVR/fsl_xcvr.c                                           \
    @top_builddir@/third_party/nxp/MKW41Z4/XCVR/fsl_xcvr_trim.c                                      \
    @top_builddir@/third_party/nxp/MKW41Z4/XCVR/ifr_radio.c                                          \
    @top_builddir@/third_party/nxp/MKW41Z4/XCVR/cfgs_kw4x_3x_2x/fsl_xcvr_ant_config.c                \
    @top_builddir@/third_party/nxp/MKW41Z4/XCVR/cfgs_kw4x_3x_2x/fsl_xcvr_ble_config.c                \
    @top_builddir@/third_party/nxp/MKW41Z4/XCVR/cfgs_kw4x_3x_2x/fsl_xcvr_common_config.c             \
    @top_builddir@/third_party/nxp/MKW41Z4/XCVR/cfgs_kw4x_3x_2x/fsl_xcvr_gfsk_bt_0p3_h_0p5_config.c  \
    @top_builddir@/third_party/nxp/MKW41Z4/XCVR/cfgs_kw4x_3x_2x/fsl_xcvr_gfsk_bt_0p5_h_0p32_config.c \
    @top_builddir@/third_party/nxp/MKW41Z4/XCVR/cfgs_kw4x_3x_2x/fsl_xcvr_gfsk_bt_0p5_h_0p5_config.c  \
    @top_builddir@/third_party/nxp/MKW41Z4/XCVR/cfgs_kw4x_3x_2x/fsl_xcvr_gfsk_bt_0p5_h_0p7_config.c  \
    @top_builddir@/third_party/nxp/MKW41Z4/XCVR/cfgs_kw4x_3x_2x/fsl_xcvr_gfsk_bt_0p5_h_1p0_config.c  \
    @top_builddir@/third_party/nxp/MKW41Z4/XCVR/cfgs_kw4x_3x_2x/fsl_xcvr_gfsk_bt_0p7_h_0p5_config.c  \
    @top_builddir@/third_party/nxp/MKW41Z4/XCVR/cfgs_kw4x_3x_2x/fsl_xcvr_mode_datarate_config.c      \
    @top_builddir@/third_party/nxp/MKW41Z4/XCVR/cfgs_kw4x_3x_2x/fsl_xcvr_msk_config.c                \
    @top_builddir@/third_party/nxp/MKW41Z4/XCVR/cfgs_kw4x_3x_2x/fsl_xcvr_zgbe_config.c               \
    $(NULL)

libopenthread_kw41z_a_SOURCES                                                                     += \
    $(PLATFORM_SOURCES)                                                                              \
    $(NULL)

noinst_HEADERS                                                                                     = \
    platform-kw41z.h                                                                                 \
    @top_builddir@/third_party/nxp/MKW41Z4/clock_config.h                                            \
    $(NULL)

PRETTY_FILES                                                                                       = \
    $(PLATFORM_SOURCES)                                                                              \
    $(NULL)

Dash                                                                                               = -
libopenthread_kw41z_a_LIBADD                                                                       = \
    $(shell find $(top_builddir)/examples/platforms/utils $(Dash)type f $(Dash)name "*.o")

include $(abs_top_nlbuild_autotools_dir)/automake/post.am
