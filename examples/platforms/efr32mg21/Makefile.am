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

lib_LIBRARIES                                 = libopenthread-efr32mg21.a

# Do not enable -Wconversion for rail
override CFLAGS                              := $(filter-out -Wconversion,$(CFLAGS))
override CXXFLAGS                            := $(filter-out -Wconversion,$(CXXFLAGS))

# Do not enable -pedantic-errors for rail
override CFLAGS                              := $(filter-out -pedantic-errors,$(CFLAGS))
override CXXFLAGS                            := $(filter-out -pedantic-errors,$(CXXFLAGS))

EFR32_BOARD_DIR                               = $(shell echo $(BOARD) | tr A-Z a-z)

EFR32MG_SDK_SRCDIR                            = $(top_srcdir)/third_party/silabs/gecko_sdk_suite/v2.6

libopenthread_efr32mg21_a_CPPFLAGS                                            = \
    -D__START=main                                                              \
    -D__STARTUP_CLEAR_BSS                                                       \
    -DPLATFORM_HEADER=\"@top_builddir@/third_party/silabs/gecko_sdk_suite/v2.6/platform/base/hal/micro/cortexm3/compiler/gcc.h\" \
    -Wno-sign-compare                                                           \
    -DCORTEXM3                                                                  \
    -DPHY=EMBER_PHY_RAIL                                                        \
    -DMICRO=EMBER_MICRO_CORTEXM3_EFR32                                          \
    -DCORTEXM3_EFM32_MICRO                                                      \
    -DPLAT=EMBER_PLATFORM_CORTEXM3                                              \
    -I$(top_srcdir)/include                                                     \
    -I$(top_srcdir)/examples/platforms                                          \
    -I$(top_srcdir)/examples/platforms/efr32mg21/$(EFR32_BOARD_DIR)             \
    -I$(top_srcdir)/src/core                                                    \
    -I$(top_srcdir)/third_party/silabs/rail_config                              \
    -I$(EFR32MG_SDK_SRCDIR)                                                     \
    -I$(EFR32MG_SDK_SRCDIR)/platform/radio/rail_lib/common                      \
    -I$(EFR32MG_SDK_SRCDIR)/platform/radio/rail_lib/chip/efr32                  \
    -I$(EFR32MG_SDK_SRCDIR)/platform/radio/rail_lib/protocol/ieee802154         \
    -I$(EFR32MG_SDK_SRCDIR)/platform/radio/rail_lib/chip/efr32/rf/common/cortex \
    -I$(EFR32MG_SDK_SRCDIR)/platform/radio/rail_lib/hal                         \
    -I$(EFR32MG_SDK_SRCDIR)/platform/radio/rail_lib/hal/efr32                   \
    -I$(EFR32MG_SDK_SRCDIR)/platform/radio/rail_lib/plugin/pa-conversions       \
    -I$(EFR32MG_SDK_SRCDIR)/hardware/kit/common/bsp                             \
    -I$(EFR32MG_SDK_SRCDIR)/hardware/kit/EFR32MG21_$(BOARD)/config              \
    -I$(EFR32MG_SDK_SRCDIR)/platform/base/                                      \
    -I$(EFR32MG_SDK_SRCDIR)/platform/base/hal                                   \
    -I$(EFR32MG_SDK_SRCDIR)/platform/base/hal/micro/cortexm3/efm32              \
    -I$(EFR32MG_SDK_SRCDIR)/platform/base/hal/micro/cortexm3/efm32/config       \
    -I$(EFR32MG_SDK_SRCDIR)/platform/base/hal/plugin                            \
    -I$(EFR32MG_SDK_SRCDIR)/platform/CMSIS/Include                              \
    -I$(EFR32MG_SDK_SRCDIR)/platform/Device/SiliconLabs/EFR32MG21/Include       \
    -I$(EFR32MG_SDK_SRCDIR)/platform/emdrv/common/inc                           \
    -I$(EFR32MG_SDK_SRCDIR)/platform/emdrv/gpiointerrupt/inc                    \
    -I$(EFR32MG_SDK_SRCDIR)/platform/emdrv/uartdrv/inc                          \
    -I$(EFR32MG_SDK_SRCDIR)/platform/emdrv/uartdrv/config                       \
    -I$(EFR32MG_SDK_SRCDIR)/platform/emdrv/ustimer/inc                          \
    -I$(EFR32MG_SDK_SRCDIR)/platform/emdrv/dmadrv/inc                           \
    -I$(EFR32MG_SDK_SRCDIR)/platform/emdrv/dmadrv/config                        \
    -I$(EFR32MG_SDK_SRCDIR)/platform/emdrv/rtcdrv/inc                           \
    -I$(EFR32MG_SDK_SRCDIR)/platform/emlib/inc                                  \
    -I$(EFR32MG_SDK_SRCDIR)/platform/halconfig/inc/hal-config                   \
    -I$(EFR32MG_SDK_SRCDIR)/util/plugin/plugin-common/fem-control               \
    -Wno-unused-parameter                                                       \
    -Wno-missing-field-initializers                                             \
    $(NULL)

PLATFORM_SOURCES                                                              = \
    alarm.c                                                                     \
    diag.c                                                                      \
    entropy.c                                                                   \
    fem-control.c                                                               \
    flash.c                                                                     \
    logging.c                                                                   \
    misc.c                                                                      \
    openthread-core-efr32-config.h                                              \
    platform-efr32.h                                                            \
    platform-band.h                                                             \
    radio.c                                                                     \
    rail_config.h                                                               \
    startup-gcc.c                                                               \
    system.c                                                                    \
    uart.c                                                                      \
    $(NULL)

noinst_HEADERS                                                                = \
    platform-efr32.h                                                            \
    platform-band.h                                                             \
    $(NULL)

libopenthread_efr32mg21_a_SOURCES                                             = \
    $(PLATFORM_SOURCES)                                                         \
    $(NULL)

PRETTY_FILES                                                                  = \
    $(PLATFORM_SOURCES)                                                         \
    $(NULL)

Dash                                                                                                 = -
libopenthread_efr32mg21_a_LIBADD                                                                     = \
    $(shell find $(top_builddir)/examples/platforms/utils $(Dash)type f $(Dash)name "*.o")             \
    $(shell find $(top_builddir)/third_party/jlink/SEGGER_RTT_V640/RTT $(Dash)type f $(Dash)name "*.o")

DIST_SUBDIRS                                                                  = \
    sleepy-demo                                                                 \
    $(NULL)
    
SUBDIRS                                                                       = \
    sleepy-demo                                                                 \
    $(NULL)

PRETTY_SUBDIRS                                                                = \
    sleepy-demo                                                                 \
    $(NULL)
    
include $(abs_top_nlbuild_autotools_dir)/automake/post.am
