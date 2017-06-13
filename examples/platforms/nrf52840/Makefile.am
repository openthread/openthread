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

COMMONCPPFLAGS                                                                                             = \
    -DCONFIG_GPIO_AS_PINRESET                                                                                \
    -DNRF52840_XXAA                                                                                          \
    -I$(top_srcdir)/include                                                                                  \
    -I$(top_srcdir)/examples/platforms                                                                       \
    -I$(top_srcdir)/src/core                                                                                 \
    -I$(srcdir)                                                                                              \
    -I$(top_srcdir)/third_party/NordicSemiconductor                                                          \
    -I$(top_srcdir)/third_party/NordicSemiconductor/cmsis                                                    \
    -I$(top_srcdir)/third_party/NordicSemiconductor/device                                                   \
    -I$(top_srcdir)/third_party/NordicSemiconductor/drivers/clock                                            \
    -I$(top_srcdir)/third_party/NordicSemiconductor/drivers/radio                                            \
    -I$(top_srcdir)/third_party/NordicSemiconductor/drivers/radio/raal                                       \
    -I$(top_srcdir)/third_party/NordicSemiconductor/hal                                                      \
    -I$(top_srcdir)/third_party/NordicSemiconductor/segger_rtt                                               \
    $(NULL)

PLATFORM_SOURCES                                                                                           = \
    alarm.c                                                                                                  \
    flash.c                                                                                                  \
    logging.c                                                                                                \
    misc.c                                                                                                   \
    platform.c                                                                                               \
    uart.c                                                                                                   \
    platform-config.h                                                                                        \
    platform-nrf5.h                                                                                          \
    radio.c                                                                                                  \
    random.c                                                                                                 \
    $(NULL)

SINGLEPHY_SOURCES                                                                                          = \
    flash_nosd.c                                                                                             \
    @top_builddir@/third_party/NordicSemiconductor/drivers/radio/nrf_drv_radio802154_notification_direct.c   \
    @top_builddir@/third_party/NordicSemiconductor/drivers/radio/nrf_drv_radio802154_priority_drop_direct.c  \
    @top_builddir@/third_party/NordicSemiconductor/drivers/radio/nrf_drv_radio802154_request_direct.c        \
    @top_builddir@/third_party/NordicSemiconductor/drivers/radio/raal/single_phy/single_phy.c                \
    $(NULL)

SINGLEPHY_CPPFLAGS                                                                                         = \
    -DRAAL_SINGLE_PHY=1                                                                                      \
    $(NULL)

SOFTDEVICE_SOURCES                                                                                         = \
    flash_sd.c                                                                                               \
    softdevice.c                                                                                             \
    softdevice.h                                                                                             \
    @top_builddir@/third_party/NordicSemiconductor/drivers/radio/nrf_drv_radio802154_notification_swi.c      \
    @top_builddir@/third_party/NordicSemiconductor/drivers/radio/nrf_drv_radio802154_priority_drop_swi.c     \
    @top_builddir@/third_party/NordicSemiconductor/drivers/radio/nrf_drv_radio802154_request_swi.c           \
    @top_builddir@/third_party/NordicSemiconductor/drivers/radio/nrf_drv_radio802154_swi.c                   \
    @top_builddir@/third_party/NordicSemiconductor/drivers/radio/raal/softdevice/nrf_raal_softdevice.c       \
    $(NULL)

SOFTDEVICE_CPPFLAGS                                                                                        = \
    -I$(top_srcdir)/third_party/NordicSemiconductor/drivers/radio/raal/softdevice                            \
    -I$(top_srcdir)/third_party/NordicSemiconductor/softdevice/s140/headers                                  \
    -DSOFTDEVICE_PRESENT                                                                                     \
    -DS140                                                                                                   \
    -DRAAL_SOFTDEVICE=1                                                                                      \
    $(NULL)

if OPENTHREAD_ENABLE_DIAG
PLATFORM_SOURCES                                                                                          += \
    diag.c                                                                                                   \
    $(NULL)
endif

NORDICSEMI_SOURCES                                                                                         = \
    @top_builddir@/third_party/NordicSemiconductor/hal/nrf_nvmc.c                                            \
    @top_builddir@/third_party/NordicSemiconductor/device/system_nrf52840.c                                  \
    @top_builddir@/third_party/NordicSemiconductor/drivers/radio/nrf_drv_radio802154.c                       \
    @top_builddir@/third_party/NordicSemiconductor/drivers/radio/nrf_drv_radio802154_ack_pending_bit.c       \
    @top_builddir@/third_party/NordicSemiconductor/drivers/radio/nrf_drv_radio802154_critical_section.c      \
    @top_builddir@/third_party/NordicSemiconductor/drivers/radio/nrf_drv_radio802154_debug.c                 \
    @top_builddir@/third_party/NordicSemiconductor/drivers/radio/nrf_drv_radio802154_fsm.c                   \
    @top_builddir@/third_party/NordicSemiconductor/drivers/radio/nrf_drv_radio802154_pib.c                   \
    @top_builddir@/third_party/NordicSemiconductor/drivers/radio/nrf_drv_radio802154_rx_buffer.c             \
    @top_builddir@/third_party/NordicSemiconductor/segger_rtt/SEGGER_RTT.c                                   \
    @top_builddir@/third_party/NordicSemiconductor/segger_rtt/SEGGER_RTT_Conf.h                              \
    @top_builddir@/third_party/NordicSemiconductor/device/gcc_startup_nrf52840.S                             \
    $(NULL)

libopenthread_nrf52840_a_CPPFLAGS                                                                          = \
    $(COMMONCPPFLAGS)                                                                                        \
    $(SINGLEPHY_CPPFLAGS)                                                                                    \
    $(NULL)

libopenthread_nrf52840_a_SOURCES                                                                           = \
    $(PLATFORM_SOURCES)                                                                                      \
    $(NORDICSEMI_SOURCES)                                                                                    \
    $(SINGLEPHY_SOURCES)                                                                                     \
    @top_builddir@/third_party/NordicSemiconductor/drivers/clock/nrf_drv_clock.c                             \
    $(NULL)

libopenthread_nrf52840_sdk_a_CPPFLAGS                                                                      = \
    $(COMMONCPPFLAGS)                                                                                        \
    $(SINGLEPHY_CPPFLAGS)                                                                                    \
    $(NULL)

libopenthread_nrf52840_sdk_a_SOURCES                                                                       = \
    $(PLATFORM_SOURCES)                                                                                      \
    $(NORDICSEMI_SOURCES)                                                                                    \
    $(SINGLEPHY_SOURCES)                                                                                     \
    $(NULL)

libopenthread_nrf52840_softdevice_sdk_a_CPPFLAGS                                                           = \
    $(COMMONCPPFLAGS)                                                                                        \
    $(SOFTDEVICE_CPPFLAGS)                                                                                   \
    $(NULL)

libopenthread_nrf52840_softdevice_sdk_a_SOURCES                                                            = \
    $(PLATFORM_SOURCES)                                                                                      \
    $(NORDICSEMI_SOURCES)                                                                                    \
    $(SOFTDEVICE_SOURCES)                                                                                    \
    $(NULL)

noinst_HEADERS                                                                                             = \
    $(top_srcdir)/third_party/NordicSemiconductor/cmsis/arm_math.h                                           \
    $(top_srcdir)/third_party/NordicSemiconductor/cmsis/cmsis_armcc_V6.h                                     \
    $(top_srcdir)/third_party/NordicSemiconductor/cmsis/cmsis_armcc.h                                        \
    $(top_srcdir)/third_party/NordicSemiconductor/cmsis/cmsis_gcc.h                                          \
    $(top_srcdir)/third_party/NordicSemiconductor/cmsis/core_cm4.h                                           \
    $(top_srcdir)/third_party/NordicSemiconductor/cmsis/core_cmFunc.h                                        \
    $(top_srcdir)/third_party/NordicSemiconductor/cmsis/core_cmInstr.h                                       \
    $(top_srcdir)/third_party/NordicSemiconductor/cmsis/core_cmSimd.h                                        \
    $(top_srcdir)/third_party/NordicSemiconductor/device/compiler_abstraction.h                              \
    $(top_srcdir)/third_party/NordicSemiconductor/device/nrf.h                                               \
    $(top_srcdir)/third_party/NordicSemiconductor/device/nrf51_to_nrf52.h                                    \
    $(top_srcdir)/third_party/NordicSemiconductor/device/nrf51_to_nrf52840.h                                 \
    $(top_srcdir)/third_party/NordicSemiconductor/device/nrf52_to_nrf52840.h                                 \
    $(top_srcdir)/third_party/NordicSemiconductor/device/nrf52840_bitfields.h                                \
    $(top_srcdir)/third_party/NordicSemiconductor/device/nrf52840_peripherals.h                              \
    $(top_srcdir)/third_party/NordicSemiconductor/device/system_nrf52840.h                                   \
    $(top_srcdir)/third_party/NordicSemiconductor/drivers/clock/nrf_drv_clock.h                              \
    $(top_srcdir)/third_party/NordicSemiconductor/drivers/radio/nrf_drv_radio802154.h                        \
    $(top_srcdir)/third_party/NordicSemiconductor/drivers/radio/nrf_drv_radio802154_ack_pending_bit.h        \
    $(top_srcdir)/third_party/NordicSemiconductor/drivers/radio/nrf_drv_radio802154_config.h                 \
    $(top_srcdir)/third_party/NordicSemiconductor/drivers/radio/nrf_drv_radio802154_const.h                  \
    $(top_srcdir)/third_party/NordicSemiconductor/drivers/radio/nrf_drv_radio802154_critical_section.h       \
    $(top_srcdir)/third_party/NordicSemiconductor/drivers/radio/nrf_drv_radio802154_debug.h                  \
    $(top_srcdir)/third_party/NordicSemiconductor/drivers/radio/nrf_drv_radio802154_fsm.h                    \
    $(top_srcdir)/third_party/NordicSemiconductor/drivers/radio/nrf_drv_radio802154_notification.h           \
    $(top_srcdir)/third_party/NordicSemiconductor/drivers/radio/nrf_drv_radio802154_pib.h                    \
    $(top_srcdir)/third_party/NordicSemiconductor/drivers/radio/nrf_drv_radio802154_priority_drop.h          \
    $(top_srcdir)/third_party/NordicSemiconductor/drivers/radio/nrf_drv_radio802154_procedures_duration.h    \
    $(top_srcdir)/third_party/NordicSemiconductor/drivers/radio/nrf_drv_radio802154_request.h                \
    $(top_srcdir)/third_party/NordicSemiconductor/drivers/radio/nrf_drv_radio802154_rx_buffer.h              \
    $(top_srcdir)/third_party/NordicSemiconductor/drivers/radio/raal/nrf_raal_api.h                          \
    $(top_srcdir)/third_party/NordicSemiconductor/drivers/radio/raal/nrf_raal_config.h                       \
    $(top_srcdir)/third_party/NordicSemiconductor/drivers/radio/raal/softdevice/nrf_raal_softdevice.h        \
    $(top_srcdir)/third_party/NordicSemiconductor/hal/nrf_assert.h                                           \
    $(top_srcdir)/third_party/NordicSemiconductor/hal/nrf_clock.h                                            \
    $(top_srcdir)/third_party/NordicSemiconductor/hal/nrf_gpio.h                                             \
    $(top_srcdir)/third_party/NordicSemiconductor/hal/nrf_gpiote.h                                           \
    $(top_srcdir)/third_party/NordicSemiconductor/hal/nrf_egu.h                                              \
    $(top_srcdir)/third_party/NordicSemiconductor/hal/nrf_nvmc.h                                             \
    $(top_srcdir)/third_party/NordicSemiconductor/hal/nrf_peripherals.h                                      \
    $(top_srcdir)/third_party/NordicSemiconductor/hal/nrf_ppi.h                                              \
    $(top_srcdir)/third_party/NordicSemiconductor/hal/nrf_radio.h                                            \
    $(top_srcdir)/third_party/NordicSemiconductor/hal/nrf_rng.h                                              \
    $(top_srcdir)/third_party/NordicSemiconductor/hal/nrf_uart.h                                             \
    $(top_srcdir)/third_party/NordicSemiconductor/segger_rtt/SEGGER_RTT.h                                    \
    $(top_srcdir)/third_party/NordicSemiconductor/softdevice/s140/headers/ble.h                              \
    $(top_srcdir)/third_party/NordicSemiconductor/softdevice/s140/headers/ble_err.h                          \
    $(top_srcdir)/third_party/NordicSemiconductor/softdevice/s140/headers/ble_gap.h                          \
    $(top_srcdir)/third_party/NordicSemiconductor/softdevice/s140/headers/ble_gatt.h                         \
    $(top_srcdir)/third_party/NordicSemiconductor/softdevice/s140/headers/ble_gattc.h                        \
    $(top_srcdir)/third_party/NordicSemiconductor/softdevice/s140/headers/ble_gatts.h                        \
    $(top_srcdir)/third_party/NordicSemiconductor/softdevice/s140/headers/ble_hci.h                          \
    $(top_srcdir)/third_party/NordicSemiconductor/softdevice/s140/headers/ble_l2cap.h                        \
    $(top_srcdir)/third_party/NordicSemiconductor/softdevice/s140/headers/ble_ranges.h                       \
    $(top_srcdir)/third_party/NordicSemiconductor/softdevice/s140/headers/ble_types.h                        \
    $(top_srcdir)/third_party/NordicSemiconductor/softdevice/s140/headers/nrf_error.h                        \
    $(top_srcdir)/third_party/NordicSemiconductor/softdevice/s140/headers/nrf_error_sdm.h                    \
    $(top_srcdir)/third_party/NordicSemiconductor/softdevice/s140/headers/nrf_error_soc.h                    \
    $(top_srcdir)/third_party/NordicSemiconductor/softdevice/s140/headers/nrf_nvic.h                         \
    $(top_srcdir)/third_party/NordicSemiconductor/softdevice/s140/headers/nrf_sdm.h                          \
    $(top_srcdir)/third_party/NordicSemiconductor/softdevice/s140/headers/nrf_soc.h                          \
    $(top_srcdir)/third_party/NordicSemiconductor/softdevice/s140/headers/nrf_svc.h                          \
    $(top_srcdir)/third_party/NordicSemiconductor/softdevice/s140/headers/nrf52/nrf_mbr.h                    \
    $(NULL)

PRETTY_FILES                                                                                              = \
    $(PLATFORM_SOURCES)                                                                                     \
    $(NULL)

Dash                                                                                                      = -

libopenthread_nrf52840_a_LIBADD                                                                           = \
    $(shell find $(top_builddir)/examples/platforms/utils $(Dash)type f $(Dash)name "*.o")

libopenthread_nrf52840_sdk_a_LIBADD                                                                       = \
    $(shell find $(top_builddir)/examples/platforms/utils $(Dash)type f $(Dash)name "*.o")

libopenthread_nrf52840_softdevice_sdk_a_LIBADD                                                             = \
    $(shell find $(top_builddir)/examples/platforms/utils $(Dash)type f $(Dash)name "*.o")

include $(abs_top_nlbuild_autotools_dir)/automake/post.am
