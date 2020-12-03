#
#  Copyright (c) 2019, The OpenThread Authors.
#  Copyright (c) 2019, NXP.
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

lib_LIBRARIES                                                                                                        = \
    libopenthread-k32w061_plat.a                                                                                       \
    libopenthread-k32w061_sdk.a
    $(NULL)

# Do not enable -pedantic-errors for k32w061 driver library
override CFLAGS                                      := $(filter-out -pedantic-errors,$(CFLAGS))
override CXXFLAGS                                    := $(filter-out -pedantic-errors,$(CXXFLAGS))

# Do not enable -Wcast-align for k32w061 driver library
override CFLAGS                                      := $(filter-out -Wcast-align,$(CFLAGS))
override CXXFLAGS                                    := $(filter-out -Wcast-align,$(CXXFLAGS))

LIB_FLAGS                                                                                                            = \
    -DCPU_K32W061HN                                                                                                    \
    -DCPU_JN518X                                                                                                       \
    -DCPU_JN518X_REV=2                                                                                                 \
    -DJENNIC_CHIP_FAMILY_JN518x                                                                                        \
    -DJENNIC_CHIP_FAMILY_NAME=_JN518x                                                                                  \
    -DgPWR_LDOMEM_0_9V_PD=0                                                                                            \
    -DNO_SYSCORECLK_UPD=0                                                                                              \
    -I$(top_srcdir)/include                                                                                            \
    -I$(top_srcdir)/examples/platforms                                                                                 \
    -I$(top_srcdir)/src/core                                                                                           \
    -I$(top_srcdir)/third_party/nxp                                                                                    \
    -I$(top_srcdir)/third_party/nxp/K32W061DK6                                                                         \
    -I$(top_srcdir)/third_party/nxp/K32W061DK6/devices/K32W061                                                         \
    -I$(top_srcdir)/third_party/nxp/K32W061DK6/middleware/mbedtls/port/ksdk                                            \
    -I$(top_srcdir)/third_party/nxp/K32W061DK6/components/serial_manager                                               \
    -I$(top_srcdir)/third_party/nxp/K32W061DK6/components/uart                                                         \
    -I$(top_srcdir)/third_party/nxp/K32W061DK6/devices/K32W061/drivers                                                 \
    -I$(top_srcdir)/third_party/nxp/K32W061DK6/devices/K32W061/utilities/debug_console                                 \
    -I$(top_srcdir)/third_party/nxp/K32W061DK6/devices/K32W061/utilities/str                                           \
    -I$(top_srcdir)/third_party/nxp/K32W061DK6/CMSIS/Include                                                           \
    -I$(top_srcdir)/third_party/nxp/K32W061DK6/middleware/wireless/ieee-802.15.4/uMac/Include                          \
    -I$(top_srcdir)/third_party/nxp/K32W061DK6/middleware/wireless/framework/XCVR/DK6/Build/Include                    \
    -I$(top_srcdir)/third_party/nxp/K32W061DK6/middleware/wireless/framework/XCVR/DK6                                  \
    -I$(top_srcdir)/third_party/nxp/K32W061DK6/middleware/wireless/framework/Common/                                   \
    -I$(top_srcdir)/third_party/nxp/K32W061DK6/middleware/wireless/framework/FunctionLib/                              \
    -I$(top_srcdir)/third_party/nxp/K32W061DK6/middleware/wireless/framework/Panic/Interface/                          \
    -I$(top_srcdir)/third_party/nxp/K32W061DK6/middleware/wireless/framework/MemManager/Interface                      \
    -I$(top_srcdir)/third_party/nxp/K32W061DK6/middleware/wireless/framework/SerialManager/Source                      \
    -I$(top_srcdir)/third_party/nxp/K32W061DK6/middleware/wireless/framework/TimersManager/Source                      \
    -I$(top_srcdir)/third_party/nxp/K32W061DK6/middleware/wireless/framework/PDM/Include                               \
    -I$(top_srcdir)/third_party/nxp/K32W061DK6/middleware/wireless/framework/Lists                                     \
    -I$(top_srcdir)/third_party/nxp/K32W061DK6/middleware/wireless/framework/OSAbstraction/Interface                   \
    -I$(top_srcdir)/third_party/nxp/K32W061DK6/middleware/wireless/framework/Flash/Internal                            \
    -I$(top_srcdir)/third_party/nxp/K32W061DK6/boards/k32w061dk6/wireless_examples/openthread/enablement               \
    -Wno-unknown-pragmas                                                                                               \
    -Wno-sign-compare                                                                                                  \
    -Wno-unused-function                                                                                               \
    -Wno-unused-parameter                                                                                              \
    -Wno-empty-body                                                                                                    \
    -Wno-missing-field-initializers                                                                                    \
    -Wno-clobbered                                                                                                     \
    -fno-strict-aliasing                                                                                               \
    $(NULL)

libopenthread_k32w061_sdk_a_CPPFLAGS                                                                                 = \
    $(LIB_FLAGS)                                                                                                       \
    $(NULL)

libopenthread_k32w061_plat_a_CPPFLAGS                                                                                = \
    $(LIB_FLAGS)                                                                                                       \
	$(NULL)

PLATFORM_SOURCES                                                                                                     = \
    src/alarm.c                                                                                                        \
    src/diag.c                                                                                                         \
    src/logging.c                                                                                                      \
    src/misc.c                                                                                                         \
    src/radio.c                                                                                                        \
    src/entropy.c                                                                                                      \
    src/system.c                                                                                                       \
    src/uart.c                                                                                                         \
    src/settings_k32w.c                                                                                                \
    @top_builddir@/third_party/nxp/K32W061DK6/devices/K32W061/utilities/fsl_assert.c                                   \
    @top_builddir@/third_party/nxp/K32W061DK6/middleware/mbedtls/port/ksdk/aes_alt.c                                   \
    @top_builddir@/third_party/nxp/K32W061DK6/middleware/mbedtls/port/ksdk/ksdk_mbedtls.c                              \
    $(NULL)

libopenthread_k32w061_sdk_a_SOURCES                                                                                  = \
    @top_builddir@/third_party/nxp/K32W061DK6/devices/K32W061/mcuxpresso/startup_k32w061.c                             \
    @top_builddir@/third_party/nxp/K32W061DK6/devices/K32W061/system_K32W061.c                                         \
    @top_builddir@/third_party/nxp/K32W061DK6/boards/k32w061dk6/wireless_examples/openthread/enablement/pin_mux.c      \
    @top_builddir@/third_party/nxp/K32W061DK6/boards/k32w061dk6/wireless_examples/openthread/enablement/clock_config.c \
    @top_builddir@/third_party/nxp/K32W061DK6/components/serial_manager/serial_manager.c                               \
    @top_builddir@/third_party/nxp/K32W061DK6/components/serial_manager/serial_port_uart.c                             \
    @top_builddir@/third_party/nxp/K32W061DK6/components/uart/usart_adapter.c                                          \
    @top_builddir@/third_party/nxp/K32W061DK6/devices/K32W061/drivers/fsl_gpio.c                                       \
    @top_builddir@/third_party/nxp/K32W061DK6/devices/K32W061/drivers/fsl_clock.c                                      \
    @top_builddir@/third_party/nxp/K32W061DK6/devices/K32W061/drivers/fsl_ctimer.c                                     \
    @top_builddir@/third_party/nxp/K32W061DK6/devices/K32W061/drivers/fsl_wtimer.c                                     \
    @top_builddir@/third_party/nxp/K32W061DK6/devices/K32W061/drivers/fsl_flash.c                                      \
    @top_builddir@/third_party/nxp/K32W061DK6/devices/K32W061/drivers/fsl_usart.c                                      \
    @top_builddir@/third_party/nxp/K32W061DK6/devices/K32W061/drivers/fsl_rng.c                                        \
    @top_builddir@/third_party/nxp/K32W061DK6/devices/K32W061/drivers/fsl_flexcomm.c                                   \
    @top_builddir@/third_party/nxp/K32W061DK6/devices/K32W061/drivers/fsl_reset.c                                      \
    @top_builddir@/third_party/nxp/K32W061DK6/devices/K32W061/drivers/fsl_power.c                                      \
    @top_builddir@/third_party/nxp/K32W061DK6/devices/K32W061/drivers/fsl_aes.c                                        \
    @top_builddir@/third_party/nxp/K32W061DK6/devices/K32W061/drivers/fsl_sha.c                                        \
    @top_builddir@/third_party/nxp/K32W061DK6/devices/K32W061/utilities/debug_console/fsl_debug_console.c              \
    @top_builddir@/third_party/nxp/K32W061DK6/devices/K32W061/utilities/str/fsl_str.c                                  \
    @top_builddir@/third_party/nxp/K32W061DK6/middleware/wireless/framework/Common/MicroInt_arm_sdk2.c                 \
    @top_builddir@/third_party/nxp/K32W061DK6/middleware/wireless/framework/FunctionLib/FunctionLib.c                  \
    @top_builddir@/third_party/nxp/K32W061DK6/middleware/wireless/framework/Reset/Reset.c                              \
    @top_builddir@/third_party/nxp/K32W061DK6/middleware/wireless/framework/MemManager/Source/MemManager.c             \
    @top_builddir@/third_party/nxp/K32W061DK6/middleware/wireless/framework/PDM/pdm_port.c                             \
    @top_builddir@/third_party/nxp/K32W061DK6/middleware/wireless/framework/Lists/GenericList.c                        \
    @top_builddir@/third_party/nxp/K32W061DK6/middleware/wireless/framework/Flash/Internal/Flash_Adapter.c             \
    $(NULL)

libopenthread_k32w061_plat_a_SOURCES                                                                                 = \
    $(PLATFORM_SOURCES)                                                                                                \
    $(NULL)

PRETTY_FILES                                                                                                         = \
    $(PLATFORM_SOURCES)                                                                                                \
    $(NULL)

Dash                                                                                                                 = -
libopenthread_k32w061_sdk_a_LIBADD                                                                                   = \
    $(shell find $(top_builddir)/examples/platforms/utils $(Dash)type f $(Dash)name "*.o")
libopenthread_k32w061_plat_a_LIBADD                                                                                  = \
    $(shell find $(top_builddir)/examples/platforms/utils $(Dash)type f $(Dash)name "*.o")

include $(abs_top_nlbuild_autotools_dir)/automake/post.am
