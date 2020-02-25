#
#  Copyright (c) 2016-2019, The OpenThread Authors.
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


lib_LIBRARIES                                                                                                           = \
    $(NULL)

# Build radio drivers when building platform libraries.
if OPENTHREAD_PLATFORM_NRF52811
lib_LIBRARIES                                                                                                          += \
    libnordicsemi-nrf52811-radio-driver.a                                                                                 \
    $(NULL)
endif

if OPENTHREAD_PLATFORM_NRF52833
lib_LIBRARIES                                                                                                          += \
    libnordicsemi-nrf52833-radio-driver.a                                                                                 \
    libnordicsemi-nrf52833-radio-driver-softdevice.a                                                                      \
    $(NULL)
endif

if OPENTHREAD_PLATFORM_NRF52840
lib_LIBRARIES                                                                                                          += \
    libnordicsemi-nrf52840-radio-driver.a                                                                                 \
    libnordicsemi-nrf52840-radio-driver-softdevice.a                                                                      \
    $(NULL)
endif

# Only build the SDK library when building the example applications.
if OPENTHREAD_EXAMPLES_NRF52811
lib_LIBRARIES                                                                                                          += \
    libnordicsemi-nrf52811-sdk.a                                                                                          \
    $(NULL)
endif

if OPENTHREAD_EXAMPLES_NRF52833
lib_LIBRARIES                                                                                                          += \
    libnordicsemi-nrf52833-sdk.a                                                                                          \
    $(NULL)
endif

if OPENTHREAD_EXAMPLES_NRF52840
lib_LIBRARIES                                                                                                          += \
    libnordicsemi-nrf52840-sdk.a                                                                                          \
    $(NULL)
endif

# Do not enable -pedantic-errors for Nordic Semiconductor driver library
override CFLAGS   := $(filter-out -pedantic-errors,$(CFLAGS))
override CXXFLAGS := $(filter-out -pedantic-errors,$(CXXFLAGS))

# Do not enable -Wundef for Nordic Semiconductor driver library
override CFLAGS   := $(filter-out -Wundef,$(CFLAGS))
override CXXFLAGS := $(filter-out -Wundef,$(CXXFLAGS))

COMMONCPPFLAGS                                                                                                          = \
    -DCONFIG_GPIO_AS_PINRESET                                                                                             \
    -DENABLE_FEM=1                                                                                                        \
    -DUSE_APP_CONFIG=1                                                                                                    \
    -I$(srcdir)                                                                                                           \
    -I$(top_srcdir)/include                                                                                               \
    -I$(top_srcdir)/src/core                                                                                              \
    -I$(top_srcdir)/third_party/NordicSemiconductor/drivers/radio                                                         \
    -I$(top_srcdir)/third_party/NordicSemiconductor/drivers/radio/fem                                                     \
    -I$(top_srcdir)/third_party/NordicSemiconductor/drivers/radio/fem/three_pin_gpio                                      \
    -I$(top_srcdir)/third_party/NordicSemiconductor/drivers/radio/hal                                                     \
    -I$(top_srcdir)/third_party/NordicSemiconductor/drivers/radio/rsch                                                    \
    -I$(top_srcdir)/third_party/NordicSemiconductor/drivers/radio/rsch/raal                                               \
    -I$(top_srcdir)/third_party/NordicSemiconductor/drivers/radio/rsch/raal/softdevice                                    \
    -I$(top_srcdir)/third_party/NordicSemiconductor/drivers/radio/mac_features                                            \
    -I$(top_srcdir)/third_party/NordicSemiconductor/drivers/radio/mac_features/ack_generator                              \
    $(NULL)

# Only reference the SDK header files included in third_party/NordicSemiconductor
# when building the example applications.
#
# When building just the platform libraries, the caller of configure is expected
# to provide the correct -I arguments to locate the necessary header files in an
# external copy of the Nordic nRF5 SDK.
#
# Note that an exception is made for the 802.15.4 radio driver, which is always built
# using the sources in third_party/NordicSemiconductor/drivers/radio.
if OPENTHREAD_ENABLE_EXAMPLES
COMMONCPPFLAGS                                                                                                         += \
    -I$(top_srcdir)/third_party/NordicSemiconductor                                                                       \
    -I$(top_srcdir)/third_party/NordicSemiconductor/cmsis                                                                 \
    -I$(top_srcdir)/third_party/NordicSemiconductor/config                                                                \
    -I$(top_srcdir)/third_party/NordicSemiconductor/dependencies                                                          \
    -I$(top_srcdir)/third_party/NordicSemiconductor/drivers/clock                                                         \
    -I$(top_srcdir)/third_party/NordicSemiconductor/drivers/common                                                        \
    -I$(top_srcdir)/third_party/NordicSemiconductor/drivers/power                                                         \
    -I$(top_srcdir)/third_party/NordicSemiconductor/drivers/systick                                                       \
    -I$(top_srcdir)/third_party/NordicSemiconductor/drivers/usbd                                                          \
    -I$(top_srcdir)/third_party/NordicSemiconductor/libraries/app_error                                                   \
    -I$(top_srcdir)/third_party/NordicSemiconductor/libraries/atfifo                                                      \
    -I$(top_srcdir)/third_party/NordicSemiconductor/libraries/atomic                                                      \
    -I$(top_srcdir)/third_party/NordicSemiconductor/libraries/block_dev                                                   \
    -I$(top_srcdir)/third_party/NordicSemiconductor/libraries/delay                                                       \
    -I$(top_srcdir)/third_party/NordicSemiconductor/libraries/usb                                                         \
    -I$(top_srcdir)/third_party/NordicSemiconductor/libraries/usb/config                                                  \
    -I$(top_srcdir)/third_party/NordicSemiconductor/libraries/usb/class/cdc                                               \
    -I$(top_srcdir)/third_party/NordicSemiconductor/libraries/usb/class/cdc/acm                                           \
    -I$(top_srcdir)/third_party/NordicSemiconductor/libraries/utf_converter                                               \
    -I$(top_srcdir)/third_party/NordicSemiconductor/nrfx                                                                  \
    -I$(top_srcdir)/third_party/NordicSemiconductor/nrfx/hal                                                              \
    -I$(top_srcdir)/third_party/NordicSemiconductor/nrfx/drivers                                                          \
    -I$(top_srcdir)/third_party/NordicSemiconductor/nrfx/drivers/include                                                  \
    -I$(top_srcdir)/third_party/NordicSemiconductor/nrfx/mdk                                                              \
    -I$(top_srcdir)/third_party/NordicSemiconductor/nrfx/soc                                                              \
    -I$(top_srcdir)/third_party/NordicSemiconductor/softdevice/s140/headers                                               \
    -I$(top_srcdir)/third_party/NordicSemiconductor/softdevice/s140/headers/nrf52                                         \
    -Wno-unused-parameter                                                                                                 \
    $(NULL)

NRF52811_CPPFLAGS                                                                                                       = \
    -DNRF52811_XXAA                                                                                                       \
    -DNRF_802154_PROJECT_CONFIG=\"platform-config.h\"                                                                     \
    -D__HEAP_SIZE=0                                                                                                       \
    -D__STACK_SIZE=2048                                                                                                   \
    -I$(top_srcdir)/examples/platforms/nrf528xx/nrf52811                                                                  \
    -I$(top_srcdir)/third_party/NordicSemiconductor/config/nrf52811/config                                                \
    $(NULL)
endif

NRF52833_CPPFLAGS                                                                                                       = \
    -DNRF52833_XXAA                                                                                                       \
    -DNRF_802154_PROJECT_CONFIG=\"platform-config.h\"                                                                     \
    -I$(top_srcdir)/examples/platforms/nrf52833                                                                           \
    -I$(top_srcdir)/third_party/NordicSemiconductor/config/nrf52833/config                                                \
    $(NULL)

NRF52840_CPPFLAGS                                                                                                       = \
    -DNRF52840_XXAA                                                                                                       \
    -DNRF_802154_PROJECT_CONFIG=\"platform-config.h\"                                                                     \
    -I$(top_srcdir)/examples/platforms/nrf528xx/nrf52840/                                                                 \
    -I$(top_srcdir)/third_party/NordicSemiconductor/config/nrf52840/config                                                \
    $(NULL)

NORDICSEMI_COMMON_SOURCES                                                                                               = \
    dependencies/app_util_platform.c                                                                                      \
    drivers/clock/nrf_drv_clock.c                                                                                         \
    drivers/power/nrf_drv_power.c                                                                                         \
    libraries/app_error/app_error.c                                                                                       \
    libraries/app_error/app_error_weak.c                                                                                  \
    libraries/utf_converter/utf.c                                                                                         \
    nrfx/drivers/src/nrfx_clock.c                                                                                         \
    nrfx/drivers/src/nrfx_power.c                                                                                         \
    nrfx/drivers/src/nrfx_spis.c                                                                                          \
    nrfx/drivers/src/nrfx_systick.c                                                                                       \
    nrfx/hal/nrf_nvmc.c                                                                                                   \
    nrfx/soc/nrfx_atomic.c                                                                                                \
    $(NULL)

NORDICSEMI_USB_SOURCES                                                                                                  = \
    nrfx/drivers/src/nrfx_usbd.c                                                                                          \
    libraries/atfifo/nrf_atfifo.c                                                                                         \
    libraries/atomic/nrf_atomic.c                                                                                         \
    libraries/usb/app_usbd.c                                                                                              \
    libraries/usb/app_usbd_core.c                                                                                         \
    libraries/usb/app_usbd_string_desc.c                                                                                  \
    libraries/usb/app_usbd_serial_num.c                                                                                   \
    libraries/usb/app_usbd_nrf_dfu_trigger.c                                                                              \
    libraries/usb/nrf_dfu_trigger_usb.c                                                                                   \
    libraries/usb/class/cdc/acm/app_usbd_cdc_acm.c                                                                        \
    $(NULL)

NORDICSEMI_NRF52811_SOURCES                                                                                             = \
    nrfx/mdk/gcc_startup_nrf52811.S                                                                                       \
    nrfx/mdk/system_nrf52811.c                                                                                            \
    $(NULL)

NORDICSEMI_NRF52833_SOURCES                                                                                             = \
    nrfx/mdk/gcc_startup_nrf52833.S                                                                                       \
    nrfx/mdk/system_nrf52833.c                                                                                            \
    $(NULL)

NORDICSEMI_NRF52840_SOURCES                                                                                             = \
    nrfx/mdk/gcc_startup_nrf52840.S                                                                                       \
    nrfx/mdk/system_nrf52840.c                                                                                            \
    $(NULL)

RADIO_DRIVER_SOURCES                                                                                                    = \
    drivers/radio/fal/nrf_802154_fal.c                                                                                    \
    drivers/radio/mac_features/ack_generator/nrf_802154_ack_data.c                                                        \
    drivers/radio/mac_features/ack_generator/nrf_802154_ack_generator.c                                                   \
    drivers/radio/mac_features/ack_generator/nrf_802154_enh_ack_generator.c                                               \
    drivers/radio/mac_features/ack_generator/nrf_802154_imm_ack_generator.c                                               \
    drivers/radio/mac_features/nrf_802154_csma_ca.c                                                                       \
    drivers/radio/mac_features/nrf_802154_delayed_trx.c                                                                   \
    drivers/radio/mac_features/nrf_802154_filter.c                                                                        \
    drivers/radio/mac_features/nrf_802154_frame_parser.c                                                                  \
    drivers/radio/mac_features/nrf_802154_precise_ack_timeout.c                                                           \
    drivers/radio/fem/three_pin_gpio/nrf_fem_three_pin_gpio.c                                                             \
    drivers/radio/nrf_802154_core_hooks.c                                                                                 \
    drivers/radio/nrf_802154_core.c                                                                                       \
    drivers/radio/nrf_802154_critical_section.c                                                                           \
    drivers/radio/nrf_802154_debug.c                                                                                      \
    drivers/radio/nrf_802154_pib.c                                                                                        \
    drivers/radio/nrf_802154_rssi.c                                                                                       \
    drivers/radio/nrf_802154_rx_buffer.c                                                                                  \
    drivers/radio/nrf_802154_timer_coord.c                                                                                \
    drivers/radio/nrf_802154.c                                                                                            \
    drivers/radio/platform/coex/nrf_802154_wifi_coex_none.c                                                               \
    drivers/radio/platform/clock/nrf_802154_clock_ot.c                                                                    \
    drivers/radio/platform/hp_timer/nrf_802154_hp_timer.c                                                                 \
    drivers/radio/rsch/nrf_802154_rsch_crit_sect.c                                                                        \
    drivers/radio/rsch/nrf_802154_rsch.c                                                                                  \
    drivers/radio/timer_scheduler/nrf_802154_timer_sched.c                                                                \
    $(NULL)

RADIO_DRIVER_SINGLE_PHY_SOURCES                                                                                         = \
    drivers/radio/nrf_802154_notification_direct.c                                                                        \
    drivers/radio/nrf_802154_priority_drop_direct.c                                                                       \
    drivers/radio/nrf_802154_request_direct.c                                                                             \
    drivers/radio/rsch/raal/single_phy/single_phy.c                                                                       \
    $(NULL)

RADIO_DRIVER_SINGLE_PHY_CPPFLAGS                                                                                        = \
    -DRAAL_SINGLE_PHY=1                                                                                                   \
    $(NULL)

RADIO_DRIVER_SOFTDEVICE_SOURCES                                                                                         = \
    drivers/radio/nrf_802154_notification_swi.c                                                                           \
    drivers/radio/nrf_802154_priority_drop_swi.c                                                                          \
    drivers/radio/nrf_802154_request_swi.c                                                                                \
    drivers/radio/nrf_802154_swi.c                                                                                        \
    drivers/radio/rsch/raal/softdevice/nrf_raal_softdevice.c                                                              \
    $(NULL)

RADIO_DRIVER_SOFTDEVICE_CPPFLAGS                                                                                        = \
    -DRAAL_SOFTDEVICE=1                                                                                                   \
    $(NULL)

SOFTDEVICE_CPPFLAGS                                                                                                     = \
    -DSOFTDEVICE_PRESENT                                                                                                  \
    -DS140                                                                                                                \
    -DRAAL_SOFTDEVICE=1                                                                                                   \
    $(NULL)

libnordicsemi_nrf52811_sdk_a_CPPFLAGS                                                                                   = \
    $(COMMONCPPFLAGS)                                                                                                     \
    $(NRF52811_CPPFLAGS)                                                                                                  \
    $(NULL)

libnordicsemi_nrf52811_sdk_a_SOURCES                                                                                    = \
    $(NORDICSEMI_COMMON_SOURCES)                                                                                          \
    $(NORDICSEMI_NRF52811_SOURCES)                                                                                        \
    $(NULL)

libnordicsemi_nrf52811_radio_driver_a_CPPFLAGS                                                                          = \
    $(COMMONCPPFLAGS)                                                                                                     \
    $(NRF52811_CPPFLAGS)                                                                                                  \
    $(RADIO_DRIVER_SINGLE_PHY_CPPFLAGS)                                                                                   \
    $(NULL)

libnordicsemi_nrf52811_radio_driver_a_SOURCES                                                                           = \
    $(RADIO_DRIVER_SOURCES)                                                                                               \
    $(RADIO_DRIVER_SINGLE_PHY_SOURCES)                                                                                    \
    $(NULL)

libnordicsemi_nrf52833_sdk_a_CPPFLAGS                                                                                   = \
    $(COMMONCPPFLAGS)                                                                                                     \
    $(NRF52833_CPPFLAGS)                                                                                                  \
    $(NULL)

libnordicsemi_nrf52833_sdk_a_SOURCES                                                                                    = \
    $(NORDICSEMI_COMMON_SOURCES)                                                                                          \
    $(NORDICSEMI_USB_SOURCES)                                                                                             \
    $(NORDICSEMI_NRF52833_SOURCES)                                                                                        \
    $(NULL)

libnordicsemi_nrf52833_radio_driver_a_CPPFLAGS                                                                          = \
    $(COMMONCPPFLAGS)                                                                                                     \
    $(NRF52833_CPPFLAGS)                                                                                                  \
    $(RADIO_DRIVER_SINGLE_PHY_CPPFLAGS)                                                                                   \
    $(NULL)

libnordicsemi_nrf52833_radio_driver_a_SOURCES                                                                           = \
    $(RADIO_DRIVER_SOURCES)                                                                                               \
    $(RADIO_DRIVER_SINGLE_PHY_SOURCES)                                                                                    \
    $(NULL)

libnordicsemi_nrf52833_radio_driver_softdevice_a_CPPFLAGS                                                               = \
    $(COMMONCPPFLAGS)                                                                                                     \
    $(NRF52833_CPPFLAGS)                                                                                                  \
    $(SOFTDEVICE_CPPFLAGS)                                                                                                \
    $(RADIO_DRIVER_SOFTDEVICE_CPPFLAGS)                                                                                   \
    $(NULL)

libnordicsemi_nrf52833_radio_driver_softdevice_a_SOURCES                                                                = \
    $(RADIO_DRIVER_SOURCES)                                                                                               \
    $(RADIO_DRIVER_SOFTDEVICE_SOURCES)                                                                                    \
    $(NULL)

libnordicsemi_nrf52840_sdk_a_CPPFLAGS                                                                                   = \
    $(COMMONCPPFLAGS)                                                                                                     \
    $(NRF52840_CPPFLAGS)                                                                                                  \
    $(NULL)

libnordicsemi_nrf52840_sdk_a_SOURCES                                                                                    = \
    $(NORDICSEMI_COMMON_SOURCES)                                                                                          \
    $(NORDICSEMI_USB_SOURCES)                                                                                             \
    $(NORDICSEMI_NRF52840_SOURCES)                                                                                        \
    $(NULL)

libnordicsemi_nrf52840_radio_driver_a_CPPFLAGS                                                                          = \
    $(COMMONCPPFLAGS)                                                                                                     \
    $(NRF52840_CPPFLAGS)                                                                                                  \
    $(RADIO_DRIVER_SINGLE_PHY_CPPFLAGS)                                                                                   \
    $(NULL)

libnordicsemi_nrf52840_radio_driver_a_SOURCES                                                                           = \
    $(RADIO_DRIVER_SOURCES)                                                                                               \
    $(RADIO_DRIVER_SINGLE_PHY_SOURCES)                                                                                    \
    $(NULL)

libnordicsemi_nrf52840_radio_driver_softdevice_a_CPPFLAGS                                                               = \
    $(COMMONCPPFLAGS)                                                                                                     \
    $(NRF52840_CPPFLAGS)                                                                                                  \
    $(SOFTDEVICE_CPPFLAGS)                                                                                                \
    $(RADIO_DRIVER_SOFTDEVICE_CPPFLAGS)                                                                                   \
    $(NULL)

libnordicsemi_nrf52840_radio_driver_softdevice_a_SOURCES                                                                = \
    $(RADIO_DRIVER_SOURCES)                                                                                               \
    $(RADIO_DRIVER_SOFTDEVICE_SOURCES)                                                                                    \
    $(NULL)

noinst_HEADERS                                                                                                          = \
    $(top_srcdir)/third_party/NordicSemiconductor/cmsis/arm_math.h                                                        \
    $(top_srcdir)/third_party/NordicSemiconductor/cmsis/cmsis_armcc_V6.h                                                  \
    $(top_srcdir)/third_party/NordicSemiconductor/cmsis/cmsis_armcc.h                                                     \
    $(top_srcdir)/third_party/NordicSemiconductor/cmsis/cmsis_gcc.h                                                       \
    $(top_srcdir)/third_party/NordicSemiconductor/cmsis/core_cm4.h                                                        \
    $(top_srcdir)/third_party/NordicSemiconductor/cmsis/core_cmFunc.h                                                     \
    $(top_srcdir)/third_party/NordicSemiconductor/cmsis/core_cmInstr.h                                                    \
    $(top_srcdir)/third_party/NordicSemiconductor/cmsis/core_cmSimd.h                                                     \
    $(top_srcdir)/third_party/NordicSemiconductor/config/app_config.h                                                     \
    $(top_srcdir)/third_party/NordicSemiconductor/config/nrf52811/config/sdk_config.h                                     \
    $(top_srcdir)/third_party/NordicSemiconductor/config/nrf52833/config/sdk_config.h                                     \
    $(top_srcdir)/third_party/NordicSemiconductor/config/nrf52840/config/sdk_config.h                                     \
    $(top_srcdir)/third_party/NordicSemiconductor/dependencies/app_util_platform.h                                        \
    $(top_srcdir)/third_party/NordicSemiconductor/dependencies/app_util.h                                                 \
    $(top_srcdir)/third_party/NordicSemiconductor/dependencies/legacy/apply_old_config.h                                  \
    $(top_srcdir)/third_party/NordicSemiconductor/dependencies/nordic_common.h                                            \
    $(top_srcdir)/third_party/NordicSemiconductor/dependencies/nrf_assert.h                                               \
    $(top_srcdir)/third_party/NordicSemiconductor/dependencies/nrf_error.h                                                \
    $(top_srcdir)/third_party/NordicSemiconductor/dependencies/nrf_log_instance.h                                         \
    $(top_srcdir)/third_party/NordicSemiconductor/dependencies/nrf_log_internal.h                                         \
    $(top_srcdir)/third_party/NordicSemiconductor/dependencies/nrf_log_types.h                                            \
    $(top_srcdir)/third_party/NordicSemiconductor/dependencies/nrf_log.h                                                  \
    $(top_srcdir)/third_party/NordicSemiconductor/dependencies/nrf_log_ctrl.h                                             \
    $(top_srcdir)/third_party/NordicSemiconductor/dependencies/nrf_log_ctrl_internal.h                                    \
    $(top_srcdir)/third_party/NordicSemiconductor/dependencies/nrf_section.h                                              \
    $(top_srcdir)/third_party/NordicSemiconductor/dependencies/nrfx_config.h                                              \
    $(top_srcdir)/third_party/NordicSemiconductor/dependencies/nrfx_glue.h                                                \
    $(top_srcdir)/third_party/NordicSemiconductor/dependencies/nrfx_log.h                                                 \
    $(top_srcdir)/third_party/NordicSemiconductor/dependencies/sdk_common.h                                               \
    $(top_srcdir)/third_party/NordicSemiconductor/dependencies/sdk_errors.h                                               \
    $(top_srcdir)/third_party/NordicSemiconductor/dependencies/sdk_macros.h                                               \
    $(top_srcdir)/third_party/NordicSemiconductor/dependencies/sdk_os.h                                                   \
    $(top_srcdir)/third_party/NordicSemiconductor/dependencies/sdk_resources.h                                            \
    $(top_srcdir)/third_party/NordicSemiconductor/drivers/clock/nrf_drv_clock.h                                           \
    $(top_srcdir)/third_party/NordicSemiconductor/drivers/common/nrf_drv_common.h                                         \
    $(top_srcdir)/third_party/NordicSemiconductor/drivers/power/nrf_drv_power.h                                           \
    $(top_srcdir)/third_party/NordicSemiconductor/drivers/systick/nrf_drv_systick.h                                       \
    $(top_srcdir)/third_party/NordicSemiconductor/drivers/usbd/nrf_drv_usbd_errata.h                                      \
    $(top_srcdir)/third_party/NordicSemiconductor/drivers/usbd/nrf_drv_usbd.h                                             \
    $(top_srcdir)/third_party/NordicSemiconductor/drivers/radio/fal/nrf_802154_fal.h                                      \
    $(top_srcdir)/third_party/NordicSemiconductor/drivers/radio/fem/none/nrf_fem_config.h                                 \
    $(top_srcdir)/third_party/NordicSemiconductor/drivers/radio/fem/nrf_fem_control_config.h                              \
    $(top_srcdir)/third_party/NordicSemiconductor/drivers/radio/fem/nrf_fem_protocol_api.h                                \
    $(top_srcdir)/third_party/NordicSemiconductor/drivers/radio/fem/nrf_fem_protocol_legacy_api.h                         \
    $(top_srcdir)/third_party/NordicSemiconductor/drivers/radio/fem/simple_gpio/nrf_fem_config.h                          \
    $(top_srcdir)/third_party/NordicSemiconductor/drivers/radio/fem/three_pin_gpio/nrf_fem_config.h                       \
    $(top_srcdir)/third_party/NordicSemiconductor/drivers/radio/mac_features/ack_generator/nrf_802154_ack_data.h          \
    $(top_srcdir)/third_party/NordicSemiconductor/drivers/radio/mac_features/ack_generator/nrf_802154_ack_generator.h     \
    $(top_srcdir)/third_party/NordicSemiconductor/drivers/radio/mac_features/ack_generator/nrf_802154_enh_ack_generator.h \
    $(top_srcdir)/third_party/NordicSemiconductor/drivers/radio/mac_features/ack_generator/nrf_802154_imm_ack_generator.h \
    $(top_srcdir)/third_party/NordicSemiconductor/drivers/radio/mac_features/nrf_802154_ack_timeout.h                     \
    $(top_srcdir)/third_party/NordicSemiconductor/drivers/radio/mac_features/nrf_802154_csma_ca.h                         \
    $(top_srcdir)/third_party/NordicSemiconductor/drivers/radio/mac_features/nrf_802154_delayed_trx.h                     \
    $(top_srcdir)/third_party/NordicSemiconductor/drivers/radio/mac_features/nrf_802154_filter.h                          \
    $(top_srcdir)/third_party/NordicSemiconductor/drivers/radio/mac_features/nrf_802154_frame_parser.h                    \
    $(top_srcdir)/third_party/NordicSemiconductor/drivers/radio/nrf_802154_config.h                                       \
    $(top_srcdir)/third_party/NordicSemiconductor/drivers/radio/nrf_802154_const.h                                        \
    $(top_srcdir)/third_party/NordicSemiconductor/drivers/radio/nrf_802154_core.h                                         \
    $(top_srcdir)/third_party/NordicSemiconductor/drivers/radio/nrf_802154_core_hooks.h                                   \
    $(top_srcdir)/third_party/NordicSemiconductor/drivers/radio/nrf_802154_critical_section.h                             \
    $(top_srcdir)/third_party/NordicSemiconductor/drivers/radio/nrf_802154_debug_core.h                                   \
    $(top_srcdir)/third_party/NordicSemiconductor/drivers/radio/nrf_802154_debug.h                                        \
    $(top_srcdir)/third_party/NordicSemiconductor/drivers/radio/nrf_802154.h                                              \
    $(top_srcdir)/third_party/NordicSemiconductor/drivers/radio/nrf_802154_notification.h                                 \
    $(top_srcdir)/third_party/NordicSemiconductor/drivers/radio/nrf_802154_peripherals.h                                  \
    $(top_srcdir)/third_party/NordicSemiconductor/drivers/radio/nrf_802154_pib.h                                          \
    $(top_srcdir)/third_party/NordicSemiconductor/drivers/radio/nrf_802154_priority_drop.h                                \
    $(top_srcdir)/third_party/NordicSemiconductor/drivers/radio/nrf_802154_procedures_duration.h                          \
    $(top_srcdir)/third_party/NordicSemiconductor/drivers/radio/nrf_802154_request.h                                      \
    $(top_srcdir)/third_party/NordicSemiconductor/drivers/radio/nrf_802154_rssi.h                                         \
    $(top_srcdir)/third_party/NordicSemiconductor/drivers/radio/nrf_802154_rx_buffer.h                                    \
    $(top_srcdir)/third_party/NordicSemiconductor/drivers/radio/nrf_802154_swi.h                                          \
    $(top_srcdir)/third_party/NordicSemiconductor/drivers/radio/nrf_802154_timer_coord.h                                  \
    $(top_srcdir)/third_party/NordicSemiconductor/drivers/radio/nrf_802154_types.h                                        \
    $(top_srcdir)/third_party/NordicSemiconductor/drivers/radio/nrf_802154_utils.h                                        \
    $(top_srcdir)/third_party/NordicSemiconductor/drivers/radio/platform/clock/nrf_802154_clock.h                         \
    $(top_srcdir)/third_party/NordicSemiconductor/drivers/radio/platform/coex/nrf_802154_wifi_coex.h                      \
    $(top_srcdir)/third_party/NordicSemiconductor/drivers/radio/platform/hp_timer/nrf_802154_hp_timer.h                   \
    $(top_srcdir)/third_party/NordicSemiconductor/drivers/radio/platform/lp_timer/nrf_802154_lp_timer.h                   \
    $(top_srcdir)/third_party/NordicSemiconductor/drivers/radio/platform/random/nrf_802154_random.h                       \
    $(top_srcdir)/third_party/NordicSemiconductor/drivers/radio/platform/temperature/nrf_802154_temperature.h             \
    $(top_srcdir)/third_party/NordicSemiconductor/drivers/radio/rsch/nrf_802154_rsch_crit_sect.h                          \
    $(top_srcdir)/third_party/NordicSemiconductor/drivers/radio/rsch/nrf_802154_rsch.h                                    \
    $(top_srcdir)/third_party/NordicSemiconductor/drivers/radio/rsch/raal/nrf_raal_api.h                                  \
    $(top_srcdir)/third_party/NordicSemiconductor/drivers/radio/rsch/raal/nrf_raal_config.h                               \
    $(top_srcdir)/third_party/NordicSemiconductor/drivers/radio/rsch/raal/simulator/nrf_802154_debug.h                    \
    $(top_srcdir)/third_party/NordicSemiconductor/drivers/radio/rsch/raal/softdevice/nrf_802154_debug.h                   \
    $(top_srcdir)/third_party/NordicSemiconductor/drivers/radio/rsch/raal/softdevice/nrf_raal_softdevice.h                \
    $(top_srcdir)/third_party/NordicSemiconductor/drivers/radio/timer_scheduler/nrf_802154_timer_sched.h                  \
    $(top_srcdir)/third_party/NordicSemiconductor/libraries/app_error/app_error_weak.h                                    \
    $(top_srcdir)/third_party/NordicSemiconductor/libraries/app_error/app_error.h                                         \
    $(top_srcdir)/third_party/NordicSemiconductor/libraries/atfifo/nrf_atfifo_internal.h                                  \
    $(top_srcdir)/third_party/NordicSemiconductor/libraries/atfifo/nrf_atfifo.h                                           \
    $(top_srcdir)/third_party/NordicSemiconductor/libraries/atomic/nrf_atomic_internal.h                                  \
    $(top_srcdir)/third_party/NordicSemiconductor/libraries/atomic/nrf_atomic.h                                           \
    $(top_srcdir)/third_party/NordicSemiconductor/libraries/block_dev/nrf_block_dev.h                                     \
    $(top_srcdir)/third_party/NordicSemiconductor/libraries/delay/nrf_delay.h                                             \
    $(top_srcdir)/third_party/NordicSemiconductor/libraries/usb/app_usbd_class_base.h                                     \
    $(top_srcdir)/third_party/NordicSemiconductor/libraries/usb/app_usbd_core.h                                           \
    $(top_srcdir)/third_party/NordicSemiconductor/libraries/usb/app_usbd_descriptor.h                                     \
    $(top_srcdir)/third_party/NordicSemiconductor/libraries/usb/app_usbd_langid.h                                         \
    $(top_srcdir)/third_party/NordicSemiconductor/libraries/usb/app_usbd_request.h                                        \
    $(top_srcdir)/third_party/NordicSemiconductor/libraries/usb/app_usbd_serial_num.h                                     \
    $(top_srcdir)/third_party/NordicSemiconductor/libraries/usb/app_usbd_string_desc.h                                    \
    $(top_srcdir)/third_party/NordicSemiconductor/libraries/usb/app_usbd_types.h                                          \
    $(top_srcdir)/third_party/NordicSemiconductor/libraries/usb/app_usbd.h                                                \
    $(top_srcdir)/third_party/NordicSemiconductor/libraries/usb/app_usbd_nrf_dfu_trigger.h                                \
    $(top_srcdir)/third_party/NordicSemiconductor/libraries/usb/app_usbd_nrf_dfu_trigger_types.h                          \
    $(top_srcdir)/third_party/NordicSemiconductor/libraries/usb/app_usbd_nrf_dfu_trigger_internal.h                       \
    $(top_srcdir)/third_party/NordicSemiconductor/libraries/usb/class/cdc/app_usbd_cdc_desc.h                             \
    $(top_srcdir)/third_party/NordicSemiconductor/libraries/usb/class/cdc/app_usbd_cdc_types.h                            \
    $(top_srcdir)/third_party/NordicSemiconductor/libraries/usb/class/cdc/acm/app_usbd_cdc_acm_internal.h                 \
    $(top_srcdir)/third_party/NordicSemiconductor/libraries/usb/class/cdc/acm/app_usbd_cdc_acm.h                          \
    $(top_srcdir)/third_party/NordicSemiconductor/libraries/usb/config/app_usbd_string_config.h                           \
    $(top_srcdir)/third_party/NordicSemiconductor/libraries/utf_converter/utf.h                                           \
    $(top_srcdir)/third_party/NordicSemiconductor/nrfx/drivers/nrfx_common.h                                              \
    $(top_srcdir)/third_party/NordicSemiconductor/nrfx/drivers/nrfx_errors.h                                              \
    $(top_srcdir)/third_party/NordicSemiconductor/nrfx/drivers/include/nrfx_clock.h                                       \
    $(top_srcdir)/third_party/NordicSemiconductor/nrfx/drivers/include/nrfx_power_clock.h                                 \
    $(top_srcdir)/third_party/NordicSemiconductor/nrfx/drivers/include/nrfx_power.h                                       \
    $(top_srcdir)/third_party/NordicSemiconductor/nrfx/drivers/include/nrfx_spis.h                                        \
    $(top_srcdir)/third_party/NordicSemiconductor/nrfx/drivers/include/nrfx_systick.h                                     \
    $(top_srcdir)/third_party/NordicSemiconductor/nrfx/drivers/include/nrfx_usbd.h                                        \
    $(top_srcdir)/third_party/NordicSemiconductor/nrfx/drivers/src/nrfx_usbd_errata.h                                     \
    $(top_srcdir)/third_party/NordicSemiconductor/nrfx/hal/nrf_clock.h                                                    \
    $(top_srcdir)/third_party/NordicSemiconductor/nrfx/hal/nrf_gpio.h                                                     \
    $(top_srcdir)/third_party/NordicSemiconductor/nrfx/hal/nrf_gpiote.h                                                   \
    $(top_srcdir)/third_party/NordicSemiconductor/nrfx/hal/nrf_egu.h                                                      \
    $(top_srcdir)/third_party/NordicSemiconductor/nrfx/hal/nrf_nvmc.h                                                     \
    $(top_srcdir)/third_party/NordicSemiconductor/nrfx/hal/nrf_ppi.h                                                      \
    $(top_srcdir)/third_party/NordicSemiconductor/nrfx/hal/nrf_power.h                                                    \
    $(top_srcdir)/third_party/NordicSemiconductor/nrfx/hal/nrf_rng.h                                                      \
    $(top_srcdir)/third_party/NordicSemiconductor/nrfx/hal/nrf_spis.h                                                     \
    $(top_srcdir)/third_party/NordicSemiconductor/nrfx/hal/nrf_uart.h                                                     \
    $(top_srcdir)/third_party/NordicSemiconductor/nrfx/hal/nrf_usbd.h                                                     \
    $(top_srcdir)/third_party/NordicSemiconductor/nrfx/mdk/compiler_abstraction.h                                         \
    $(top_srcdir)/third_party/NordicSemiconductor/nrfx/mdk/nrf.h                                                          \
    $(top_srcdir)/third_party/NordicSemiconductor/nrfx/mdk/nrf_peripherals.h                                              \
    $(top_srcdir)/third_party/NordicSemiconductor/nrfx/mdk/nrf51_to_nrf52.h                                               \
    $(top_srcdir)/third_party/NordicSemiconductor/nrfx/mdk/nrf51_to_nrf52810.h                                            \
    $(top_srcdir)/third_party/NordicSemiconductor/nrfx/mdk/nrf52_to_nrf52810.h                                            \
    $(top_srcdir)/third_party/NordicSemiconductor/nrfx/mdk/nrf51_to_nrf52840.h                                            \
    $(top_srcdir)/third_party/NordicSemiconductor/nrfx/mdk/nrf52_to_nrf52833.h                                            \
    $(top_srcdir)/third_party/NordicSemiconductor/nrfx/mdk/nrf52_to_nrf52840.h                                            \
    $(top_srcdir)/third_party/NordicSemiconductor/nrfx/mdk/nrf52810_to_nrf52811.h                                         \
    $(top_srcdir)/third_party/NordicSemiconductor/nrfx/mdk/nrf52811.h                                                     \
    $(top_srcdir)/third_party/NordicSemiconductor/nrfx/mdk/nrf52811_bitfields.h                                           \
    $(top_srcdir)/third_party/NordicSemiconductor/nrfx/mdk/nrf52811_peripherals.h                                         \
    $(top_srcdir)/third_party/NordicSemiconductor/nrfx/mdk/nrf52833.h                                                     \
    $(top_srcdir)/third_party/NordicSemiconductor/nrfx/mdk/nrf52833_bitfields.h                                           \
    $(top_srcdir)/third_party/NordicSemiconductor/nrfx/mdk/nrf52833_peripherals.h                                         \
    $(top_srcdir)/third_party/NordicSemiconductor/nrfx/mdk/system_nrf52833.h                                              \
    $(top_srcdir)/third_party/NordicSemiconductor/nrfx/mdk/nrf52840.h                                                     \
    $(top_srcdir)/third_party/NordicSemiconductor/nrfx/mdk/nrf52840_bitfields.h                                           \
    $(top_srcdir)/third_party/NordicSemiconductor/nrfx/mdk/nrf52840_peripherals.h                                         \
    $(top_srcdir)/third_party/NordicSemiconductor/nrfx/mdk/system_nrf52840.h                                              \
    $(top_srcdir)/third_party/NordicSemiconductor/softdevice/s140/headers/ble.h                                           \
    $(top_srcdir)/third_party/NordicSemiconductor/softdevice/s140/headers/ble_err.h                                       \
    $(top_srcdir)/third_party/NordicSemiconductor/softdevice/s140/headers/ble_gap.h                                       \
    $(top_srcdir)/third_party/NordicSemiconductor/softdevice/s140/headers/ble_gatt.h                                      \
    $(top_srcdir)/third_party/NordicSemiconductor/softdevice/s140/headers/ble_gattc.h                                     \
    $(top_srcdir)/third_party/NordicSemiconductor/softdevice/s140/headers/ble_gatts.h                                     \
    $(top_srcdir)/third_party/NordicSemiconductor/softdevice/s140/headers/ble_hci.h                                       \
    $(top_srcdir)/third_party/NordicSemiconductor/softdevice/s140/headers/ble_l2cap.h                                     \
    $(top_srcdir)/third_party/NordicSemiconductor/softdevice/s140/headers/ble_ranges.h                                    \
    $(top_srcdir)/third_party/NordicSemiconductor/softdevice/s140/headers/ble_types.h                                     \
    $(top_srcdir)/third_party/NordicSemiconductor/softdevice/s140/headers/nrf_error.h                                     \
    $(top_srcdir)/third_party/NordicSemiconductor/softdevice/s140/headers/nrf_error_sdm.h                                 \
    $(top_srcdir)/third_party/NordicSemiconductor/softdevice/s140/headers/nrf_error_soc.h                                 \
    $(top_srcdir)/third_party/NordicSemiconductor/softdevice/s140/headers/nrf_nvic.h                                      \
    $(top_srcdir)/third_party/NordicSemiconductor/softdevice/s140/headers/nrf_sdm.h                                       \
    $(top_srcdir)/third_party/NordicSemiconductor/softdevice/s140/headers/nrf_soc.h                                       \
    $(top_srcdir)/third_party/NordicSemiconductor/softdevice/s140/headers/nrf_svc.h                                       \
    $(top_srcdir)/third_party/NordicSemiconductor/softdevice/s140/headers/nrf52/nrf_mbr.h                                 \
    $(NULL)

include $(abs_top_nlbuild_autotools_dir)/automake/post.am
