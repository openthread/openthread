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

include $(abs_top_nlbuild_autotools_dir)/automake/pre.am

lib_LIBRARIES                                                                                      = \
    libopenthread-jn5189_plat.a                                                                      \
    libopenthread-jn5189_sdk.a
    $(NULL)

# Do not enable -pedantic-errors for jn5189 driver library
override CFLAGS                                      := $(filter-out -pedantic-errors,$(CFLAGS))
override CXXFLAGS                                    := $(filter-out -pedantic-errors,$(CXXFLAGS))

# Do not enable -Wundef for jn5189 driver library
override CFLAGS    := $(filter-out -Wundef,$(CFLAGS))
override CXXFLAGS  := $(filter-out -Wundef,$(CXXFLAGS))

LIB_FLAGS                                                                                          = \
    -DCPU_JN518X                                                                                     \
    -DCPU_JN518X_REV=2                                                                               \
    -DJENNIC_CHIP_FAMILY_JN518x                                                                      \
    -DJENNIC_CHIP_FAMILY_NAME=_JN518x                                                                \
    -I$(top_srcdir)/include                                                                          \
    -I$(top_srcdir)/examples/platforms                                                               \
    -I$(top_srcdir)/src/core                                                                         \
    -I$(top_srcdir)/third_party/nxp                                                                  \
    -I$(top_srcdir)/third_party/nxp/JN5189                                                           \
    -I$(top_srcdir)/third_party/nxp/JN5189/crypto                                                    \
    -I$(top_srcdir)/third_party/nxp/JN5189/drivers                                                   \
    -I$(top_srcdir)/third_party/nxp/JN5189/rom_apis                                                  \
    -I$(top_srcdir)/third_party/nxp/JN5189/dk6_jn5180                                                \
    -I$(top_srcdir)/third_party/nxp/CMSIS/Include                                                    \
    -I$(top_srcdir)/third_party/nxp/JN5189/uMac/Include                                              \
    -I$(top_srcdir)/third_party/nxp/JN5189/Radio_JN5189/Include                                      \
    -I$(top_srcdir)/third_party/nxp/JN5189/JennicCommon/Include                                      \
    -I$(top_srcdir)/third_party/nxp/JN5189/MicroSpecific/Include                                     \
    -Wno-unknown-pragmas                                                                             \
    -Wno-sign-compare                                                                                \
    -Wno-unused-function                                                                             \
    -Wno-unused-parameter                                                                            \
    -Wno-empty-body                                                                                  \
    $(NULL)

libopenthread_jn5189_sdk_a_CPPFLAGS                                                                = \
    $(LIB_FLAGS)                                                                                     \
	$(NULL)

libopenthread_jn5189_plat_a_CPPFLAGS                                                               = \
    $(LIB_FLAGS)                                                                                     \
	$(NULL)

PLATFORM_SOURCES                                                                                   = \
    alarm.c                                                                                          \
    diag.c                                                                                           \
    flash.c                                                                                          \
    logging.c                                                                                        \
    misc.c                                                                                           \
    radio.c                                                                                          \
    entropy.c                                                                                        \
    system.c                                                                                         \
    uart.c                                                                                           \
    settings_jn5189.c                                                                                \
    @top_builddir@/third_party/nxp/JN5189/drivers/fsl_assert.c                                       \
    @top_builddir@/third_party/nxp/JN5189/crypto/aes_alt.c                                           \
    @top_builddir@/third_party/nxp/JN5189/crypto/ksdk_mbedtls.c                                      \
    $(NULL)

libopenthread_jn5189_sdk_a_SOURCES                                                                 = \
    @top_builddir@/third_party/nxp/JN5189/startup_JN5189.c                                           \
    @top_builddir@/third_party/nxp/JN5189/system_JN5189.c                                            \
    @top_builddir@/third_party/nxp/JN5189/dk6_jn5180/pin_mux.c                                       \
    @top_builddir@/third_party/nxp/JN5189/dk6_jn5180/clock_config.c                                  \
    @top_builddir@/third_party/nxp/JN5189/drivers/fsl_gpio.c                                         \
    @top_builddir@/third_party/nxp/JN5189/drivers/fsl_clock.c                                        \
    @top_builddir@/third_party/nxp/JN5189/drivers/fsl_ctimer.c                                       \
    @top_builddir@/third_party/nxp/JN5189/drivers/fsl_wtimer.c                                       \
    @top_builddir@/third_party/nxp/JN5189/drivers/fsl_flash.c                                        \
    @top_builddir@/third_party/nxp/JN5189/drivers/fsl_usart.c                                        \
    @top_builddir@/third_party/nxp/JN5189/drivers/fsl_rng.c                                          \
    @top_builddir@/third_party/nxp/JN5189/drivers/fsl_flexcomm.c                                     \
    @top_builddir@/third_party/nxp/JN5189/drivers/fsl_reset.c                                        \
    @top_builddir@/third_party/nxp/JN5189/drivers/fsl_power.c                                        \
    @top_builddir@/third_party/nxp/JN5189/drivers/fsl_debug_console.c                                \
    @top_builddir@/third_party/nxp/JN5189/drivers/aes/fsl_aes.c                                      \
    @top_builddir@/third_party/nxp/JN5189/drivers/sha/fsl_sha.c                                      \
    $(NULL)

libopenthread_jn5189_plat_a_SOURCES                                                                = \
    $(PLATFORM_SOURCES)                                                                              \
    $(NULL)

PRETTY_FILES                                                                                       = \
    $(PLATFORM_SOURCES)                                                                              \
    $(NULL)

Dash                                                                                                = -
libopenthread_jn5189_sdk_a_LIBADD                                                                   = \
    $(shell find $(top_builddir)/examples/platforms/utils $(Dash)type f $(Dash)name "*.o")
libopenthread_jn5189_plat_a_LIBADD                                                                  = \
    $(shell find $(top_builddir)/examples/platforms/utils $(Dash)type f $(Dash)name "*.o")

include $(abs_top_nlbuild_autotools_dir)/automake/post.am
