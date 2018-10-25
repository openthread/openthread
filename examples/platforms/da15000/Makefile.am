#
#  Copyright (c) 2016, The OpenThread Authors.
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

lib_LIBRARIES                                                                           = libopenthread-da15000.a

# Do not enable -pedantic-errors
override CFLAGS    := $(filter-out -pedantic-errors,$(CFLAGS))
override CXXFLAGS  := $(filter-out -pedantic-errors,$(CXXFLAGS))

libopenthread_da15000_a_CPPFLAGS                                                        = \
    -I$(top_srcdir)/include                                                               \
    -I$(top_srcdir)/examples/platforms                                                    \
    -I$(top_srcdir)/examples/platforms/da15000                                            \
    -I$(top_srcdir)/examples/platforms/da15000/crypto                                     \
    -I$(top_srcdir)/src/core                                                              \
    -I$(top_srcdir)/third_party/dialog/DialogSDK/bsp/config                               \
    -I$(top_srcdir)/third_party/dialog/DialogSDK/bsp/include                              \
    -I$(top_srcdir)/third_party/dialog/DialogSDK/bsp/memory/include                       \
    -I$(top_srcdir)/third_party/dialog/DialogSDK/bsp/osal                                 \
    -I$(top_srcdir)/third_party/dialog/DialogSDK/bsp/peripherals/include                  \
    -I$(top_srcdir)/third_party/dialog/DialogSDK/interfaces/ftdf/include                  \
    -I$(top_srcdir)/third_party/dialog/DialogSDK/interfaces/ftdf/src                      \
    -I$(top_srcdir)/third_party/mbedtls/repo/include                                      \
    -include$(top_srcdir)/examples/platforms/da15000/custom_config_qspi.h                 \
    -Wno-sign-compare                                                                     \
    -Wno-unused-parameter                                                                 \
    -Wno-type-limits                                                                      \
    -Wno-unused-variable                                                                  \
    -Wno-missing-field-initializers                                                       \
    -fno-strict-aliasing                                                                  \
    $(MBEDTLS_CPPFLAGS)                                                                   \
    $(NULL)

PLATFORM_SOURCES                                                                        = \
    alarm.c                                                                               \
    flash.c                                                                               \
    logging.c                                                                             \
    misc.c                                                                                \
    radio.c                                                                               \
    random.c                                                                              \
    system.c                                                                              \
    uart.c                                                                                \
    crypto/aes_alt.c                                                                      \
    $(NULL)

DIALOG_SOURCES                                                                          = \
    @top_builddir@/third_party/dialog/DialogSDK/bsp/memory/src/qspi_automode.c            \
    @top_builddir@/third_party/dialog/DialogSDK/bsp/peripherals/src/hw_aes_hash.c         \
    @top_builddir@/third_party/dialog/DialogSDK/bsp/peripherals/src/hw_cpm.c              \
    @top_builddir@/third_party/dialog/DialogSDK/bsp/peripherals/src/hw_crypto.c           \
    @top_builddir@/third_party/dialog/DialogSDK/bsp/peripherals/src/hw_dma.c              \
    @top_builddir@/third_party/dialog/DialogSDK/bsp/peripherals/src/hw_gpadc.c            \
    @top_builddir@/third_party/dialog/DialogSDK/bsp/peripherals/src/hw_gpio.c             \
    @top_builddir@/third_party/dialog/DialogSDK/bsp/peripherals/src/hw_hard_fault.c       \
    @top_builddir@/third_party/dialog/DialogSDK/bsp/peripherals/src/hw_otpc.c             \
    @top_builddir@/third_party/dialog/DialogSDK/bsp/peripherals/src/hw_qspi.c             \
    @top_builddir@/third_party/dialog/DialogSDK/bsp/peripherals/src/hw_rf.c               \
    @top_builddir@/third_party/dialog/DialogSDK/bsp/peripherals/src/hw_timer0.c           \
    @top_builddir@/third_party/dialog/DialogSDK/bsp/peripherals/src/hw_trng.c             \
    @top_builddir@/third_party/dialog/DialogSDK/bsp/peripherals/src/hw_uart.c             \
    @top_builddir@/third_party/dialog/DialogSDK/bsp/peripherals/src/hw_watchdog.c         \
    @top_builddir@/third_party/dialog/DialogSDK/bsp/peripherals/src/sys_tcs.c             \
    @top_builddir@/third_party/dialog/DialogSDK/bsp/startup/config.c                      \
    @top_builddir@/third_party/dialog/DialogSDK/bsp/startup/startup_ARMCM0.S              \
    @top_builddir@/third_party/dialog/DialogSDK/bsp/startup/system_ARMCM0.c               \
    @top_builddir@/third_party/dialog/DialogSDK/bsp/startup/vector_table.S                \
    @top_builddir@/third_party/dialog/DialogSDK/interfaces/ftdf/src/ad_ftdf.c             \
    @top_builddir@/third_party/dialog/DialogSDK/interfaces/ftdf/src/ad_ftdf_phy_api.c     \
    @top_builddir@/third_party/dialog/DialogSDK/interfaces/ftdf/src/common.c              \
    @top_builddir@/third_party/dialog/DialogSDK/interfaces/ftdf/src/data.c                \
    @top_builddir@/third_party/dialog/DialogSDK/interfaces/ftdf/src/main.c                \
    @top_builddir@/third_party/dialog/DialogSDK/interfaces/ftdf/src/power.c               \
    $(NULL)

libopenthread_da15000_a_SOURCES                                                         = \
    $(PLATFORM_SOURCES)                                                                   \
    $(DIALOG_SOURCES)                                                                     \
    $(NULL)

noinst_HEADERS                                                                          = \
    custom_config_qspi.h                                                                  \
    platform-da15000.h                                                                    \
    $(NULL)

PRETTY_FILES                                                                            = \
    $(PLATFORM_SOURCES)                                                                   \
    $(noinst_HEADERS)                                                                     \
    $(NULL)

Dash                                                                                    = -
libopenthread_da15000_a_LIBADD                                                          = \
    $(shell find $(top_builddir)/examples/platforms/utils $(Dash)type f $(Dash)name "*.o")

include $(abs_top_nlbuild_autotools_dir)/automake/post.am
