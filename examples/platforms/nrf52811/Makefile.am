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

lib_LIBRARIES                                                                                              = \
    libopenthread-nrf52811.a                                                                                 \
    libopenthread-nrf52811-sdk.a                                                                             \
    $(NULL)

# Do not enable -pedantic-errors for nRF52811 driver library
override CFLAGS                                      := $(filter-out -pedantic-errors,$(CFLAGS))
override CXXFLAGS                                    := $(filter-out -pedantic-errors,$(CXXFLAGS))

COMMONCPPFLAGS                                                                                             = \
    -DCONFIG_GPIO_AS_PINRESET                                                                                \
    -DNRF52811_XXAA                                                                                          \
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
    -I$(top_srcdir)/third_party/NordicSemiconductor/libraries/app_error                                      \
    -I$(top_srcdir)/third_party/NordicSemiconductor/libraries/atfifo                                         \
    -I$(top_srcdir)/third_party/NordicSemiconductor/libraries/atomic                                         \
    -I$(top_srcdir)/third_party/NordicSemiconductor/libraries/delay                                          \
    -I$(top_srcdir)/third_party/NordicSemiconductor/libraries/utf_converter                                  \
    -I$(top_srcdir)/third_party/NordicSemiconductor/nrfx                                                     \
    -I$(top_srcdir)/third_party/NordicSemiconductor/nrfx/hal                                                 \
    -I$(top_srcdir)/third_party/NordicSemiconductor/nrfx/drivers                                             \
    -I$(top_srcdir)/third_party/NordicSemiconductor/nrfx/drivers/include                                     \
    -I$(top_srcdir)/third_party/NordicSemiconductor/nrfx/mdk                                                 \
    -I$(top_srcdir)/third_party/NordicSemiconductor/nrfx/soc                                                 \
    $(NULL)

PLATFORM_COMMON_SOURCES                                                                                    = \
    alarm.c                                                                                                  \
    fem.c                                                                                                    \
    flash.c                                                                                                  \
    logging.c                                                                                                \
    misc.c                                                                                                   \
    platform-config.h                                                                                        \
    platform-fem.h                                                                                           \
    platform-nrf5.h                                                                                          \
    radio.c                                                                                                  \
    random.c                                                                                                 \
    temp.c                                                                                                   \
    spi-slave.c                                                                                              \
    system.c                                                                                                 \
    uart.c                                                                                                   \
    $(NULL)

SINGLEPHY_SOURCES                                                                                          = \
    flash_nosd.c                                                                                             \
    $(NULL)

SINGLEPHY_CPPFLAGS                                                                                         = \
    -DRAAL_SINGLE_PHY=1                                                                                      \
    $(NULL)

PLATFORM_DIAG_SOURCES                                                                                      = \
    diag.c                                                                                                   \
    $(NULL)

PLATFORM_SOURCES                                                                                           = \
    $(PLATFORM_COMMON_SOURCES)                                                                               \
    $(NULL)

if OPENTHREAD_ENABLE_DIAG
PLATFORM_SOURCES                                                                                          += \
    $(PLATFORM_DIAG_SOURCES)                                                                                 \
    $(NULL)
endif

libopenthread_nrf52811_a_CPPFLAGS                                                                          = \
    $(COMMONCPPFLAGS)                                                                                        \
    $(SINGLEPHY_CPPFLAGS)                                                                                    \
    $(NULL)

libopenthread_nrf52811_a_SOURCES                                                                           = \
    $(PLATFORM_SOURCES)                                                                                      \
    $(SINGLEPHY_SOURCES)                                                                                     \
    $(NULL)

libopenthread_nrf52811_sdk_a_CPPFLAGS                                                                      = \
    $(COMMONCPPFLAGS)                                                                                        \
    $(SINGLEPHY_CPPFLAGS)                                                                                    \
    $(NULL)

libopenthread_nrf52811_sdk_a_SOURCES                                                                       = \
    $(PLATFORM_SOURCES)                                                                                      \
    $(SINGLEPHY_SOURCES)                                                                                     \
    $(NULL)

PRETTY_FILES                                                                                               = \
    $(PLATFORM_COMMON_SOURCES)                                                                               \
    $(PLATFORM_DIAG_SOURCES)                                                                                 \
    $(NULL)
Dash                                                                                                       = -

libopenthread_nrf52811_a_LIBADD                                                                            = \
    $(shell find $(top_builddir)/examples/platforms/utils $(Dash)type f $(Dash)name "*.o")                   \
    $(shell find $(top_builddir)/third_party/jlink/SEGGER_RTT_V640/RTT $(Dash)type f $(Dash)name "*.o")

libopenthread_nrf52811_sdk_a_LIBADD                                                                        = \
    $(shell find $(top_builddir)/examples/platforms/utils $(Dash)type f $(Dash)name "*.o")

include $(abs_top_nlbuild_autotools_dir)/automake/post.am
