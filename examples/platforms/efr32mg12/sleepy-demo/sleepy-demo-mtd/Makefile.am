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
include $(top_srcdir)/examples/platforms/Makefile.platform.am

override CFLAGS   := $(filter-out -Wconversion,$(CFLAGS))
override CXXFLAGS := $(filter-out -Wconversion,$(CXXFLAGS))

EFR32_BOARD_DIR = $(shell echo $(BOARD) | tr A-Z a-z)

SDK_SRC_DIR = $(top_srcdir)/third_party/silabs/gecko_sdk_suite/v2.7

$(top_builddir)/examples/platforms/efr32mg12/libopenthread-efr32mg12.a:
	(cd $(top_builddir)/examples/platforms/efr32mg12/ && $(MAKE) $(AM_MAKEFLAGS) libopenthread-efr32mg12.a )

bin_PROGRAMS                                                                  = \
    $(NULL)

CPPFLAGS_COMMON                                                              += \
    -DPLATFORM_HEADER=\"platform/base/hal/micro/cortexm3/compiler/gcc.h\"       \
    -DNVIC_CONFIG=\"platform/base/hal/micro/cortexm3/efm32/nvic-config.h\"      \
    -Wno-sign-compare                                                           \
    -DCORTEXM3                                                                  \
    -DPHY=EMBER_PHY_RAIL                                                        \
    -DMICRO=EMBER_MICRO_CORTEXM3_EFR32                                          \
    -DCORTEXM3_EFM32_MICRO                                                      \
    -DPLAT=EMBER_PLATFORM_CORTEXM3                                              \
    -I$(top_srcdir)/examples/platforms                                          \
    -I$(top_srcdir)/examples/platforms/efr32mg12/$(EFR32_BOARD_DIR)             \
    -I$(top_srcdir)/include                                                     \
    -I$(top_srcdir)/src/core                                                    \
    -I$(top_srcdir)/third_party/silabs/rail_config                              \
    -I$(SDK_SRC_DIR)/hardware/kit/common/bsp                                    \
    -I$(SDK_SRC_DIR)/hardware/kit/common/drivers                                \
    -I$(SDK_SRC_DIR)/hardware/kit/EFR32MG12_$(BOARD)/config                     \
    -I$(SDK_SRC_DIR)/platform/base/hal/micro/cortexm3/efm32                     \
    -I$(SDK_SRC_DIR)/platform/base/hal/micro/cortexm3/efm32/config              \
    -I$(SDK_SRC_DIR)/platform/common/inc                                        \
    -I$(SDK_SRC_DIR)/platform/CMSIS/Include                                     \
    -I$(SDK_SRC_DIR)/platform/Device/SiliconLabs/EFR32MG12P/Include             \
    -I$(SDK_SRC_DIR)/platform/emdrv/common/inc                                  \
    -I$(SDK_SRC_DIR)/platform/emdrv/gpiointerrupt/inc                           \
    -I$(SDK_SRC_DIR)/platform/emdrv/uartdrv/inc                                 \
    -I$(SDK_SRC_DIR)/platform/emdrv/uartdrv/config                              \
    -I$(SDK_SRC_DIR)/platform/emdrv/ustimer/inc                                 \
    -I$(SDK_SRC_DIR)/platform/emdrv/dmadrv/inc                                  \
    -I$(SDK_SRC_DIR)/platform/emdrv/dmadrv/config                               \
    -I$(SDK_SRC_DIR)/platform/emlib/inc                                         \
    -I$(SDK_SRC_DIR)/platform/halconfig/inc/hal-config                          \
    -I$(SDK_SRC_DIR)/platform/radio/rail_lib/chip/efr32                         \
    -I$(SDK_SRC_DIR)/platform/radio/rail_lib/chip/efr32/efr32xg1x               \
    -I$(SDK_SRC_DIR)/platform/radio/rail_lib/chip/efr32/rf/common/cortex        \
    -I$(SDK_SRC_DIR)/platform/radio/rail_lib/common                             \
    -I$(SDK_SRC_DIR)/platform/radio/rail_lib/hal                                \
    -I$(SDK_SRC_DIR)/platform/radio/rail_lib/hal/efr32                          \
    -I$(SDK_SRC_DIR)/platform/radio/rail_lib/protocol/ieee802154                \
    -I$(SDK_SRC_DIR)/platform/radio/rail_lib/plugin/pa-conversions              \
    -I$(SDK_SRC_DIR)/platform/service/mpu/inc                                   \
    -I$(SDK_SRC_DIR)/platform/service/sleeptimer/config                         \
    -I$(SDK_SRC_DIR)/platform/service/sleeptimer/inc                            \
    -I$(SDK_SRC_DIR)/util/plugin/plugin-common/fem-control                      \
    -I$(SDK_SRC_DIR)/util/third_party/mbedtls/sl_crypto/include                 \
    -Wno-unused-parameter                                                       \
    -Wno-missing-field-initializers                                             \
    $(NULL)

LDADD_COMMON                                                                 += \
    $(NULL)

LDFLAGS_COMMON                                                               += \
    $(NULL)

LIBTOOLFLAGS_COMMON                                                          += \
    $(NULL)

SOURCES_COMMON                                                               += \
    main.c                                                                      \
    $(NULL)

if OPENTHREAD_ENABLE_BUILTIN_MBEDTLS
LDADD_COMMON                                                                 += \
    $(top_builddir)/third_party/mbedtls/libmbedcrypto.a                         \
    $(NULL)
endif # OPENTHREAD_ENABLE_BUILTIN_MBEDTLS

if OPENTHREAD_ENABLE_EXECUTABLE
bin_PROGRAMS                                                                 += \
    sleepy-demo-mtd                                                             \
    $(NULL)
endif

sleepy_demo_mtd_CPPFLAGS                                                      = \
    $(CPPFLAGS_COMMON)                                                          \
    $(NULL)

sleepy_demo_mtd_LDADD                                                         = \
    $(top_builddir)/src/cli/libopenthread-cli-mtd.a                             \
    $(top_builddir)/src/core/libopenthread-mtd.a                                \
    $(LDADD_COMMON)                                                             \
    $(top_builddir)/src/core/libopenthread-mtd.a                                \
    $(LDADD_COMMON)                                                             \
    $(NULL)

sleepy_demo_mtd_LDFLAGS                                                       = \
    $(LDFLAGS_COMMON)                                                           \
    $(NULL)

sleepy_demo_mtd_LIBTOOLFLAGS                                                  = \
    $(LIBTOOLFLAGS_COMMON)                                                      \
    $(NULL)

sleepy_demo_mtd_SOURCES                                                       = \
    $(SOURCES_COMMON)                                                           \
    $(NULL)

if OPENTHREAD_ENABLE_LINKER_MAP
sleepy_demo_mtd_LDFLAGS += -Wl,-Map=sleepy-demo-mtd.map
endif

if OPENTHREAD_BUILD_COVERAGE
CPPFLAGS_COMMON                                                              += \
    -DOPENTHREAD_ENABLE_COVERAGE                                                \
    $(NULL)

CLEANFILES = $(wildcard *.gcda *.gcno)
endif # OPENTHREAD_BUILD_COVERAGE

include $(abs_top_nlbuild_autotools_dir)/automake/post.am
