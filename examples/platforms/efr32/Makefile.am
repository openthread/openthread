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

lib_LIBRARIES                                 = libopenthread-efr32.a

# Do not enable -Wconversion for rail
override CFLAGS                              := $(filter-out -Wconversion,$(CFLAGS))
override CXXFLAGS                            := $(filter-out -Wconversion,$(CXXFLAGS))

# Do not enable -pedantic-errors for rail
override CFLAGS                              := $(filter-out -pedantic-errors,$(CFLAGS))
override CXXFLAGS                            := $(filter-out -pedantic-errors,$(CXXFLAGS))

EFR32MG_SDK_SRCDIR                            = $(top_srcdir)/third_party/silabs/gecko_sdk_suite/v1.0

libopenthread_efr32_a_CPPFLAGS                                                = \
    -DEFR32MG12P432F1024GL125                                                   \
    -D__START=main                                                              \
    -D__STARTUP_CLEAR_BSS                                                       \
    -I$(top_srcdir)/include                                                     \
    -I$(top_srcdir)/examples/platforms                                          \
    -I$(top_srcdir)/src/core                                                    \
    -I$(EFR32MG_SDK_SRCDIR)/platform/radio/rail_lib/common                      \
    -I$(EFR32MG_SDK_SRCDIR)/platform/radio/rail_lib/chip/efr32                  \
    -I$(EFR32MG_SDK_SRCDIR)/platform/radio/rail_lib/protocol/ieee802154         \
    -I$(EFR32MG_SDK_SRCDIR)/platform/radio/rail_lib/chip/efr32/rf/common/cortex \
    -I$(EFR32MG_SDK_SRCDIR)/platform/radio/rail_lib/hal/efr32                   \
    -I$(EFR32MG_SDK_SRCDIR)/hardware/kit/common/bsp                             \
    -I$(EFR32MG_SDK_SRCDIR)/hardware/kit/EFR32MG12_BRD4161A/config              \
    -I$(EFR32MG_SDK_SRCDIR)/platform/CMSIS/Include                              \
    -I$(EFR32MG_SDK_SRCDIR)/platform/Device/SiliconLabs/EFR32MG12P/Include      \
    -I$(EFR32MG_SDK_SRCDIR)/platform/emdrv/common/inc                           \
    -I$(EFR32MG_SDK_SRCDIR)/platform/emdrv/uartdrv/inc                          \
    -I$(EFR32MG_SDK_SRCDIR)/platform/emdrv/ustimer/inc                          \
    -I$(EFR32MG_SDK_SRCDIR)/platform/emdrv/dmadrv/inc                           \
    -I$(EFR32MG_SDK_SRCDIR)/platform/emdrv/rtcdrv/inc                           \
    -I$(EFR32MG_SDK_SRCDIR)/platform/emlib/inc                                  \
    -Wno-unused-parameter                                                       \
    $(NULL)

PLATFORM_SOURCES                                                              = \
    alarm.c                                                                     \
    flash.c                                                                     \
    logging.c                                                                   \
    misc.c                                                                      \
    platform.c                                                                  \
    radio.c                                                                     \
    random.c                                                                    \
    startup-gcc.c                                                               \
    uart.c                                                                      \
    $(NULL)

if OPENTHREAD_ENABLE_DIAG
PLATFORM_SOURCES                                                             += \
    diag.c                                                                      \
    $(NULL)
endif

EFR32MG12P_SOURCES                                                                                                             =  \
    @top_builddir@/third_party/silabs/gecko_sdk_suite/v1.0/hardware/kit/common/bsp/bsp_bcc.c                                      \
    @top_builddir@/third_party/silabs/gecko_sdk_suite/v1.0/hardware/kit/common/bsp/bsp_stk.c                                      \
    @top_builddir@/third_party/silabs/gecko_sdk_suite/v1.0/platform/Device/SiliconLabs/EFR32MG12P/Source/system_efr32mg12p.c      \
    @top_builddir@/third_party/silabs/gecko_sdk_suite/v1.0/platform/Device/SiliconLabs/EFR32MG12P/Source/GCC/startup_efr32mg12p.c \
    @top_builddir@/third_party/silabs/gecko_sdk_suite/v1.0/platform/emdrv/rtcdrv/src/rtcdriver.c                                  \
    @top_builddir@/third_party/silabs/gecko_sdk_suite/v1.0/platform/emdrv/ustimer/src/ustimer.c                                   \
    @top_builddir@/third_party/silabs/gecko_sdk_suite/v1.0/platform/emlib/src/em_adc.c                                            \
    @top_builddir@/third_party/silabs/gecko_sdk_suite/v1.0/platform/emlib/src/em_cmu.c                                            \
    @top_builddir@/third_party/silabs/gecko_sdk_suite/v1.0/platform/emlib/src/em_core.c                                           \
    @top_builddir@/third_party/silabs/gecko_sdk_suite/v1.0/platform/emlib/src/em_emu.c                                            \
    @top_builddir@/third_party/silabs/gecko_sdk_suite/v1.0/platform/emlib/src/em_gpio.c                                           \
    @top_builddir@/third_party/silabs/gecko_sdk_suite/v1.0/platform/emlib/src/em_rmu.c                                            \
    @top_builddir@/third_party/silabs/gecko_sdk_suite/v1.0/platform/emlib/src/em_rtcc.c                                           \
    @top_builddir@/third_party/silabs/gecko_sdk_suite/v1.0/platform/emlib/src/em_system.c                                         \
    @top_builddir@/third_party/silabs/gecko_sdk_suite/v1.0/platform/emlib/src/em_timer.c                                          \
    @top_builddir@/third_party/silabs/gecko_sdk_suite/v1.0/platform/emlib/src/em_usart.c                                          \
    @top_builddir@/third_party/silabs/gecko_sdk_suite/v1.0/platform/emlib/src/em_msc.c                                            \
    @top_builddir@/third_party/silabs/gecko_sdk_suite/v1.0/platform/emlib/src/em_crypto.c                                         \
    $(NULL)

noinst_HEADERS                                                                = \
    platform-efr32.h                                                            \
    $(NULL)

libopenthread_efr32_a_SOURCES                                                 = \
    $(PLATFORM_SOURCES)                                                         \
    $(EFR32MG12P_SOURCES)                                                       \
    $(NULL)

PRETTY_FILES                                                                  = \
    $(PLATFORM_SOURCES)                                                         \
    $(NULL)

Dash                                                                          = -
libopenthread_efr32_a_LIBADD                                                  = \
    $(shell find $(top_builddir)/examples/platforms/utils $(Dash)type f $(Dash)name "*.o")

include $(abs_top_nlbuild_autotools_dir)/automake/post.am
