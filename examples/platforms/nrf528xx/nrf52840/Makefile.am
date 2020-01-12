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

# Do not enable -Wundef for nRF52840 driver library
override CFLAGS                                      := $(filter-out -Wundef,$(CFLAGS))
override CXXFLAGS                                    := $(filter-out -Wundef,$(CXXFLAGS))

COMMONCPPFLAGS                                                                                             = \
    -DCONFIG_GPIO_AS_PINRESET                                                                                \
    -DNRF52840_XXAA                                                                                          \
    -DUSE_APP_CONFIG=1                                                                                       \
    -I$(srcdir)                                                                                              \
    -I$(top_srcdir)/include                                                                                  \
    -I$(top_srcdir)/examples/platforms                                                                       \
    -I$(top_srcdir)/examples/platforms/nrf528xx/src                                                          \
    -I$(top_srcdir)/src/core                                                                                 \
    -I$(top_srcdir)/third_party/NordicSemiconductor/drivers/radio                                            \
    -I$(top_srcdir)/third_party/NordicSemiconductor/drivers/radio/fem/three_pin_gpio                         \
    -I$(top_srcdir)/third_party/NordicSemiconductor/drivers/radio/hal                                        \
    -I$(top_srcdir)/third_party/NordicSemiconductor/drivers/radio/mac_features                               \
    -I$(top_srcdir)/third_party/NordicSemiconductor/drivers/radio/mac_features/ack_generator                 \
    -I$(top_srcdir)/third_party/NordicSemiconductor/drivers/radio/rsch                                       \
    -I$(top_srcdir)/third_party/NordicSemiconductor/drivers/radio/rsch/raal                                  \
    -I$(top_srcdir)/third_party/NordicSemiconductor/drivers/radio/rsch/raal/softdevice                       \
    -I$(top_srcdir)/third_party/NordicSemiconductor/drivers/radio/platform/lp_timer                          \
    -I$(top_srcdir)/third_party/NordicSemiconductor/drivers/radio/platform/temperature                       \
    -Wno-unused-parameter                                                                                    \
    $(NULL)

# Only reference the SDK header files included in third_party/NordicSemiconductor
# when building the example applications.
#
# When building just the platform libraries, the caller of configure is expected
# to provide the correct -I arguments to locate the necessary header files in an
# external copy of the Nordic nRF5 SDK.
#
# Note that an exception is made for the 802.15.4 radio driver, which is always
# built using the sources in third_party/NordicSemiconductor/drivers/radio.
if OPENTHREAD_ENABLE_EXAMPLES
COMMONCPPFLAGS                                                                                            += \
    -I$(top_srcdir)/third_party/NordicSemiconductor                                                          \
    -I$(top_srcdir)/third_party/NordicSemiconductor/cmsis                                                    \
    -I$(top_srcdir)/third_party/NordicSemiconductor/config                                                   \
    -I$(top_srcdir)/third_party/NordicSemiconductor/config/nrf52840/config                                   \
    -I$(top_srcdir)/third_party/NordicSemiconductor/dependencies                                             \
    -I$(top_srcdir)/third_party/NordicSemiconductor/drivers/clock                                            \
    -I$(top_srcdir)/third_party/NordicSemiconductor/drivers/common                                           \
    -I$(top_srcdir)/third_party/NordicSemiconductor/drivers/power                                            \
    -I$(top_srcdir)/third_party/NordicSemiconductor/drivers/systick                                          \
    -I$(top_srcdir)/third_party/NordicSemiconductor/drivers/usbd                                             \
    -I$(top_srcdir)/third_party/NordicSemiconductor/libraries/app_error                                      \
    -I$(top_srcdir)/third_party/NordicSemiconductor/libraries/atfifo                                         \
    -I$(top_srcdir)/third_party/NordicSemiconductor/libraries/atomic                                         \
    -I$(top_srcdir)/third_party/NordicSemiconductor/libraries/block_dev                                      \
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
    -Wno-unused-parameter                                                                                    \
    $(NULL)
endif

PLATFORM_COMMON_SOURCES                                                                                    = \
    src/alarm.c                                                                                              \
    src/diag.c                                                                                               \
    src/entropy.c                                                                                            \
    src/fem.c                                                                                                \
    src/flash.c                                                                                              \
    src/logging.c                                                                                            \
    src/misc.c                                                                                               \
    src/radio.c                                                                                              \
    src/spi-slave.c                                                                                          \
    src/system.c                                                                                             \
    src/temp.c                                                                                               \
    src/uart.c                                                                                               \
    src/usb-cdc-uart.c                                                                                       \
    $(NULL)

SINGLEPHY_SOURCES                                                                                          = \
    src/flash_nosd.c                                                                                         \
    $(NULL)

SOFTDEVICE_SOURCES                                                                                         = \
    src/flash_sd.c                                                                                           \
    src/softdevice.c                                                                                         \
    src/softdevice.h                                                                                         \
    $(NULL)

SOFTDEVICE_CPPFLAGS                                                                                        = \
    -DSOFTDEVICE_PRESENT                                                                                     \
    -DS140                                                                                                   \
    $(NULL)

PLATFORM_SOURCES                                                                                           = \
    $(PLATFORM_COMMON_SOURCES)                                                                               \
    $(NULL)

libopenthread_nrf52840_a_CPPFLAGS                                                                          = \
    $(COMMONCPPFLAGS)                                                                                        \
    $(SINGLEPHY_CPPFLAGS)                                                                                    \
    -DPLATFORM_OPENTHREAD_VANILLA                                                                             \
    $(NULL)

libopenthread_nrf52840_a_SOURCES                                                                           = \
    $(PLATFORM_SOURCES)                                                                                      \
    $(SINGLEPHY_SOURCES)                                                                                     \
    $(NULL)

libopenthread_nrf52840_sdk_a_CPPFLAGS                                                                      = \
    $(COMMONCPPFLAGS)                                                                                        \
    $(SINGLEPHY_CPPFLAGS)                                                                                    \
    $(NULL)

libopenthread_nrf52840_sdk_a_SOURCES                                                                       = \
    $(PLATFORM_SOURCES)                                                                                      \
    $(SINGLEPHY_SOURCES)                                                                                     \
    $(NULL)

libopenthread_nrf52840_softdevice_sdk_a_CPPFLAGS                                                           = \
    $(COMMONCPPFLAGS)                                                                                        \
    $(SOFTDEVICE_CPPFLAGS)                                                                                   \
    $(NULL)

libopenthread_nrf52840_softdevice_sdk_a_SOURCES                                                            = \
    $(PLATFORM_SOURCES)                                                                                      \
    $(SOFTDEVICE_SOURCES)                                                                                    \
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
