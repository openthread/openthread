#
#  Copyright (c) 2016-2017, The OpenThread Authors.
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

lib_LIBRARIES                                                                                              = \
    libopenthread-nrf52840.a                                                                                 \
    libopenthread-nrf52840-sdk.a                                                                             \
    libopenthread-nrf52840-softdevice-sdk.a                                                                  \
    $(NULL)

# Do not enable -pedantic-errors for nRF52840 driver library
override CFLAGS                                      := $(filter-out -pedantic-errors,$(CFLAGS))
override CXXFLAGS                                    := $(filter-out -pedantic-errors,$(CXXFLAGS))

COMMONCPPFLAGS                                                                                             = \
    -DCONFIG_GPIO_AS_PINRESET                                                                                \
    -DNRF52840_XXAA                                                                                          \
    -I$(top_srcdir)/include                                                                                  \
    -I$(top_srcdir)/examples/platforms                                                                       \
    -I$(top_srcdir)/src/core                                                                                 \
    -I$(srcdir)                                                                                              \
    -I$(top_srcdir)/third_party/NordicSemiconductor                                                          \
    -I$(top_srcdir)/third_party/NordicSemiconductor/cmsis                                                    \
    -I$(top_srcdir)/third_party/NordicSemiconductor/dependencies                                             \
    -I$(top_srcdir)/third_party/NordicSemiconductor/drivers/clock                                            \
    -I$(top_srcdir)/third_party/NordicSemiconductor/drivers/common                                           \
    -I$(top_srcdir)/third_party/NordicSemiconductor/drivers/radio                                            \
    -I$(top_srcdir)/third_party/NordicSemiconductor/drivers/radio/hal                                        \
    -I$(top_srcdir)/third_party/NordicSemiconductor/drivers/radio/mac_features                               \
    -I$(top_srcdir)/third_party/NordicSemiconductor/drivers/radio/mac_features/ack_generator                 \
    -I$(top_srcdir)/third_party/NordicSemiconductor/drivers/radio/rsch                                       \
    -I$(top_srcdir)/third_party/NordicSemiconductor/drivers/radio/rsch/raal                                  \
    -I$(top_srcdir)/third_party/NordicSemiconductor/drivers/power                                            \
    -I$(top_srcdir)/third_party/NordicSemiconductor/drivers/systick                                          \
    -I$(top_srcdir)/third_party/NordicSemiconductor/drivers/usbd                                             \
    -I$(top_srcdir)/third_party/NordicSemiconductor/libraries/app_error                                      \
    -I$(top_srcdir)/third_party/NordicSemiconductor/libraries/atfifo                                         \
    -I$(top_srcdir)/third_party/NordicSemiconductor/libraries/atomic                                         \
    -I$(top_srcdir)/third_party/NordicSemiconductor/libraries/delay                                          \
    -I$(top_srcdir)/third_party/NordicSemiconductor/libraries/usb                                            \
    -I$(top_srcdir)/third_party/NordicSemiconductor/libraries/usb/config                                     \
    -I$(top_srcdir)/third_party/NordicSemiconductor/libraries/usb/class/cdc                                  \
    -I$(top_srcdir)/third_party/NordicSemiconductor/libraries/usb/class/cdc/acm                              \
    -I$(top_srcdir)/third_party/NordicSemiconductor/libraries/utf_converter                                  \
    -I$(top_srcdir)/third_party/NordicSemiconductor/nrfx                                                     \
    -I$(top_srcdir)/third_party/NordicSemiconductor/nrfx/hal                                                 \
    -I$(top_srcdir)/third_party/NordicSemiconductor/nrfx/drivers                                             \
    -I$(top_srcdir)/third_party/NordicSemiconductor/nrfx/drivers/include                                     \
    -I$(top_srcdir)/third_party/NordicSemiconductor/nrfx/mdk                                                 \
    -I$(top_srcdir)/third_party/NordicSemiconductor/nrfx/soc                                                 \
    -I$(top_srcdir)/third_party/NordicSemiconductor/softdevice/s140/headers                                  \
    -I$(top_srcdir)/third_party/NordicSemiconductor/softdevice/s140/headers/nrf52                            \
    $(NULL)

PLATFORM_COMMON_SOURCES                                                                                    = \
    alarm.c                                                                                                  \
    diag.c                                                                                                   \
    entropy.c                                                                                                \
    fem.c                                                                                                    \
    flash.c                                                                                                  \
    logging.c                                                                                                \
    misc.c                                                                                                   \
    openthread-core-nrf52840-config.h                                                                        \
    openthread-core-nrf52840-config-check.h                                                                  \
    platform-config.h                                                                                        \
    platform-fem.h                                                                                           \
    platform-nrf5.h                                                                                          \
    radio.c                                                                                                  \
    temp.c                                                                                                   \
    spi-slave.c                                                                                              \
    system.c                                                                                                 \
    uart.c                                                                                                   \
    usb-cdc-uart.c                                                                                           \
    $(NULL)

SINGLEPHY_SOURCES                                                                                          = \
    flash_nosd.c                                                                                             \
    $(NULL)

SOFTDEVICE_SOURCES                                                                                         = \
    flash_sd.c                                                                                               \
    platform-softdevice.h                                                                                    \
    softdevice.c                                                                                             \
    softdevice.h                                                                                             \
    $(NULL)

SOFTDEVICE_CPPFLAGS                                                                                        = \
    -I$(top_srcdir)/third_party/NordicSemiconductor/drivers/radio/rsch/raal/softdevice                       \
    -I$(top_srcdir)/third_party/NordicSemiconductor/softdevice/s140/headers                                  \
    -DSOFTDEVICE_PRESENT                                                                                     \
    -DS140                                                                                                   \
    $(NULL)

HWCRYPTO_SOURCES                                                                                           = \
    @top_builddir@/third_party/NordicSemiconductor/libraries/crypto/aes_alt.c                                \
    @top_builddir@/third_party/NordicSemiconductor/libraries/crypto/aes_alt_cc310.c                          \
    @top_builddir@/third_party/NordicSemiconductor/libraries/crypto/aes_alt_soft.c                           \
    @top_builddir@/third_party/NordicSemiconductor/libraries/crypto/cc310_mbedtls.c                          \
    @top_builddir@/third_party/NordicSemiconductor/libraries/crypto/ecp_alt_cc310.c                          \
    @top_builddir@/third_party/NordicSemiconductor/libraries/crypto/ecp_curves_alt.c                         \
    @top_builddir@/third_party/NordicSemiconductor/libraries/crypto/sha256_alt_cc310.c                       \
    $(NULL)

PLATFORM_SOURCES                                                                                           = \
    $(PLATFORM_COMMON_SOURCES)                                                                               \
    $(NULL)

libopenthread_nrf52840_a_CPPFLAGS                                                                          = \
    $(COMMONCPPFLAGS)                                                                                        \
    $(SINGLEPHY_CPPFLAGS)                                                                                    \
    $(NULL)

libopenthread_nrf52840_a_SOURCES                                                                           = \
    $(PLATFORM_SOURCES)                                                                                      \
    $(SINGLEPHY_SOURCES)                                                                                     \
    $(HWCRYPTO_SOURCES)                                                                                      \
    $(NULL)

libopenthread_nrf52840_sdk_a_CPPFLAGS                                                                      = \
    $(COMMONCPPFLAGS)                                                                                        \
    $(SINGLEPHY_CPPFLAGS)                                                                                    \
    $(NULL)

libopenthread_nrf52840_sdk_a_SOURCES                                                                       = \
    $(PLATFORM_SOURCES)                                                                                      \
    $(SINGLEPHY_SOURCES)                                                                                     \
    $(HWCRYPTO_SOURCES)                                                                                      \
    $(NULL)

libopenthread_nrf52840_softdevice_sdk_a_CPPFLAGS                                                           = \
    $(COMMONCPPFLAGS)                                                                                        \
    $(SOFTDEVICE_CPPFLAGS)                                                                                   \
    $(NULL)

libopenthread_nrf52840_softdevice_sdk_a_SOURCES                                                            = \
    $(PLATFORM_SOURCES)                                                                                      \
    $(SOFTDEVICE_SOURCES)                                                                                    \
    $(HWCRYPTO_SOURCES)                                                                                      \
    $(NULL)

PRETTY_FILES                                                                                               = \
    $(PLATFORM_COMMON_SOURCES)                                                                               \
    $(NULL)

Dash                                                                                                       = -

libopenthread_nrf52840_a_LIBADD                                                                            = \
    $(shell find $(top_builddir)/examples/platforms/utils $(Dash)type f $(Dash)name "*.o")                   \
    $(shell find $(top_builddir)/third_party/jlink/SEGGER_RTT_V640/RTT $(Dash)type f $(Dash)name "*.o")

libopenthread_nrf52840_sdk_a_LIBADD                                                                        = \
    $(shell find $(top_builddir)/examples/platforms/utils $(Dash)type f $(Dash)name "*.o")

libopenthread_nrf52840_softdevice_sdk_a_LIBADD                                                             = \
    $(shell find $(top_builddir)/examples/platforms/utils $(Dash)type f $(Dash)name "*.o")
include $(abs_top_nlbuild_autotools_dir)/automake/post.am
