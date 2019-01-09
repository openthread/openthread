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
    -DENABLE_FEM=1                                                                                           \
    -DNRF_802154_PROJECT_CONFIG=\"platform-config.h\"                                                        \
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
    usb-cdc-uart.c                                                                                           \
    $(NULL)

SINGLEPHY_SOURCES                                                                                          = \
    flash_nosd.c                                                                                             \
    @top_builddir@/third_party/NordicSemiconductor/drivers/radio/nrf_802154_notification_direct.c            \
    @top_builddir@/third_party/NordicSemiconductor/drivers/radio/nrf_802154_priority_drop_direct.c           \
    @top_builddir@/third_party/NordicSemiconductor/drivers/radio/nrf_802154_request_direct.c                 \
    @top_builddir@/third_party/NordicSemiconductor/drivers/radio/rsch/raal/single_phy/single_phy.c           \
    $(NULL)

SINGLEPHY_CPPFLAGS                                                                                         = \
    -DRAAL_SINGLE_PHY=1                                                                                      \
    $(NULL)

SOFTDEVICE_SOURCES                                                                                         = \
    flash_sd.c                                                                                               \
    softdevice.c                                                                                             \
    softdevice.h                                                                                             \
    @top_builddir@/third_party/NordicSemiconductor/drivers/radio/nrf_802154_notification_swi.c               \
    @top_builddir@/third_party/NordicSemiconductor/drivers/radio/nrf_802154_priority_drop_swi.c              \
    @top_builddir@/third_party/NordicSemiconductor/drivers/radio/nrf_802154_request_swi.c                    \
    @top_builddir@/third_party/NordicSemiconductor/drivers/radio/nrf_802154_swi.c                            \
    @top_builddir@/third_party/NordicSemiconductor/drivers/radio/rsch/raal/softdevice/nrf_raal_softdevice.c  \
    $(NULL)

SOFTDEVICE_CPPFLAGS                                                                                        = \
    -I$(top_srcdir)/third_party/NordicSemiconductor/drivers/radio/rsch/raal/softdevice                       \
    -I$(top_srcdir)/third_party/NordicSemiconductor/softdevice/s140/headers                                  \
    -DSOFTDEVICE_PRESENT                                                                                     \
    -DS140                                                                                                   \
    -DRAAL_SOFTDEVICE=1                                                                                      \
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

RADIO_DRIVER_SOURCES                                                                                         = \
    @top_builddir@/third_party/NordicSemiconductor/drivers/radio/nrf_802154.c                                  \
    @top_builddir@/third_party/NordicSemiconductor/drivers/radio/nrf_802154_ack_pending_bit.c                  \
    @top_builddir@/third_party/NordicSemiconductor/drivers/radio/nrf_802154_core.c                             \
    @top_builddir@/third_party/NordicSemiconductor/drivers/radio/nrf_802154_core_hooks.c                       \
    @top_builddir@/third_party/NordicSemiconductor/drivers/radio/nrf_802154_critical_section.c                 \
    @top_builddir@/third_party/NordicSemiconductor/drivers/radio/nrf_802154_debug.c                            \
    @top_builddir@/third_party/NordicSemiconductor/drivers/radio/nrf_802154_pib.c                              \
    @top_builddir@/third_party/NordicSemiconductor/drivers/radio/nrf_802154_revision.c                         \
    @top_builddir@/third_party/NordicSemiconductor/drivers/radio/nrf_802154_rssi.c                             \
    @top_builddir@/third_party/NordicSemiconductor/drivers/radio/nrf_802154_rx_buffer.c                        \
    @top_builddir@/third_party/NordicSemiconductor/drivers/radio/nrf_802154_timer_coord.c                      \
    @top_builddir@/third_party/NordicSemiconductor/drivers/radio/fem/nrf_fem_control.c                         \
    @top_builddir@/third_party/NordicSemiconductor/drivers/radio/mac_features/nrf_802154_precise_ack_timeout.c \
    @top_builddir@/third_party/NordicSemiconductor/drivers/radio/mac_features/nrf_802154_csma_ca.c             \
    @top_builddir@/third_party/NordicSemiconductor/drivers/radio/mac_features/nrf_802154_delayed_trx.c         \
    @top_builddir@/third_party/NordicSemiconductor/drivers/radio/mac_features/nrf_802154_filter.c              \
    @top_builddir@/third_party/NordicSemiconductor/drivers/radio/platform/clock/nrf_802154_clock_sdk.c         \
    @top_builddir@/third_party/NordicSemiconductor/drivers/radio/platform/hp_timer/nrf_802154_hp_timer.c       \
    @top_builddir@/third_party/NordicSemiconductor/drivers/radio/rsch/nrf_802154_rsch.c                        \
    @top_builddir@/third_party/NordicSemiconductor/drivers/radio/rsch/nrf_802154_rsch_crit_sect.c              \
    @top_builddir@/third_party/NordicSemiconductor/drivers/radio/timer_scheduler/nrf_802154_timer_sched.c      \
    $(NULL)

NORDICSEMI_SOURCES                                                                                         = \
    @top_builddir@/third_party/NordicSemiconductor/dependencies/app_util_platform.c                          \
    @top_builddir@/third_party/NordicSemiconductor/drivers/clock/nrf_drv_clock.c                             \
    @top_builddir@/third_party/NordicSemiconductor/drivers/power/nrf_drv_power.c                             \
    @top_builddir@/third_party/NordicSemiconductor/drivers/usbd/nrf_drv_usbd.c                               \
    @top_builddir@/third_party/NordicSemiconductor/libraries/app_error/app_error.c                           \
    @top_builddir@/third_party/NordicSemiconductor/libraries/app_error/app_error_weak.c                      \
    @top_builddir@/third_party/NordicSemiconductor/libraries/atfifo/nrf_atfifo.c                             \
    @top_builddir@/third_party/NordicSemiconductor/libraries/atomic/nrf_atomic.c                             \
    @top_builddir@/third_party/NordicSemiconductor/libraries/usb/app_usbd.c                                  \
    @top_builddir@/third_party/NordicSemiconductor/libraries/usb/app_usbd_core.c                             \
    @top_builddir@/third_party/NordicSemiconductor/libraries/usb/app_usbd_string_desc.c                      \
    @top_builddir@/third_party/NordicSemiconductor/libraries/usb/app_usbd_serial_num.c                       \
    @top_builddir@/third_party/NordicSemiconductor/libraries/usb/app_usbd_nrf_dfu_trigger.c                  \
    @top_builddir@/third_party/NordicSemiconductor/libraries/usb/nrf_dfu_trigger_usb.c                       \
    @top_builddir@/third_party/NordicSemiconductor/libraries/usb/class/cdc/acm/app_usbd_cdc_acm.c            \
    @top_builddir@/third_party/NordicSemiconductor/libraries/utf_converter/utf.c                             \
    @top_builddir@/third_party/NordicSemiconductor/nrfx/drivers/src/nrfx_clock.c                             \
    @top_builddir@/third_party/NordicSemiconductor/nrfx/drivers/src/nrfx_power.c                             \
    @top_builddir@/third_party/NordicSemiconductor/nrfx/drivers/src/nrfx_systick.c                           \
    @top_builddir@/third_party/NordicSemiconductor/nrfx/hal/nrf_nvmc.c                                       \
    @top_builddir@/third_party/NordicSemiconductor/nrfx/mdk/gcc_startup_nrf52840.S                           \
    @top_builddir@/third_party/NordicSemiconductor/nrfx/mdk/system_nrf52840.c                                \
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

libopenthread_nrf52840_a_CPPFLAGS                                                                          = \
    $(COMMONCPPFLAGS)                                                                                        \
    $(SINGLEPHY_CPPFLAGS)                                                                                    \
    $(NULL)

libopenthread_nrf52840_a_SOURCES                                                                           = \
    $(PLATFORM_SOURCES)                                                                                      \
    $(RADIO_DRIVER_SOURCES)                                                                                  \
    $(NORDICSEMI_SOURCES)                                                                                    \
    $(SINGLEPHY_SOURCES)                                                                                     \
    $(HWCRYPTO_SOURCES)                                                                                      \
    $(NULL)

libopenthread_nrf52840_sdk_a_CPPFLAGS                                                                      = \
    $(COMMONCPPFLAGS)                                                                                        \
    $(SINGLEPHY_CPPFLAGS)                                                                                    \
    $(NULL)

libopenthread_nrf52840_sdk_a_SOURCES                                                                       = \
    $(PLATFORM_SOURCES)                                                                                      \
    $(RADIO_DRIVER_SOURCES)                                                                                  \
    $(SINGLEPHY_SOURCES)                                                                                     \
    $(HWCRYPTO_SOURCES)                                                                                      \
    $(NULL)

libopenthread_nrf52840_softdevice_sdk_a_CPPFLAGS                                                           = \
    $(COMMONCPPFLAGS)                                                                                        \
    $(SOFTDEVICE_CPPFLAGS)                                                                                   \
    $(NULL)

libopenthread_nrf52840_softdevice_sdk_a_SOURCES                                                            = \
    $(PLATFORM_SOURCES)                                                                                      \
    $(RADIO_DRIVER_SOURCES)                                                                                  \
    $(SOFTDEVICE_SOURCES)                                                                                    \
    $(HWCRYPTO_SOURCES)                                                                                      \
    $(NULL)

noinst_HEADERS                                                                                               = \
    $(top_srcdir)/third_party/NordicSemiconductor/cmsis/arm_math.h                                             \
    $(top_srcdir)/third_party/NordicSemiconductor/cmsis/cmsis_armcc_V6.h                                       \
    $(top_srcdir)/third_party/NordicSemiconductor/cmsis/cmsis_armcc.h                                          \
    $(top_srcdir)/third_party/NordicSemiconductor/cmsis/cmsis_gcc.h                                            \
    $(top_srcdir)/third_party/NordicSemiconductor/cmsis/core_cm4.h                                             \
    $(top_srcdir)/third_party/NordicSemiconductor/cmsis/core_cmFunc.h                                          \
    $(top_srcdir)/third_party/NordicSemiconductor/cmsis/core_cmInstr.h                                         \
    $(top_srcdir)/third_party/NordicSemiconductor/cmsis/core_cmSimd.h                                          \
    $(top_srcdir)/third_party/NordicSemiconductor/dependencies/app_util_platform.h                             \
    $(top_srcdir)/third_party/NordicSemiconductor/dependencies/app_util.h                                      \
    $(top_srcdir)/third_party/NordicSemiconductor/dependencies/apply_old_config.h                              \
    $(top_srcdir)/third_party/NordicSemiconductor/dependencies/nordic_common.h                                 \
    $(top_srcdir)/third_party/NordicSemiconductor/dependencies/nrf_assert.h                                    \
    $(top_srcdir)/third_party/NordicSemiconductor/dependencies/nrf_error.h                                     \
    $(top_srcdir)/third_party/NordicSemiconductor/dependencies/nrf_log_instance.h                              \
    $(top_srcdir)/third_party/NordicSemiconductor/dependencies/nrf_log_internal.h                              \
    $(top_srcdir)/third_party/NordicSemiconductor/dependencies/nrf_log_types.h                                 \
    $(top_srcdir)/third_party/NordicSemiconductor/dependencies/nrf_log.h                                       \
    $(top_srcdir)/third_party/NordicSemiconductor/dependencies/nrf_log_ctrl.h                                  \
    $(top_srcdir)/third_party/NordicSemiconductor/dependencies/nrf_log_ctrl_internal.h                         \
    $(top_srcdir)/third_party/NordicSemiconductor/dependencies/nrf_section.h                                   \
    $(top_srcdir)/third_party/NordicSemiconductor/dependencies/nrfx_config.h                                   \
    $(top_srcdir)/third_party/NordicSemiconductor/dependencies/nrfx_glue.h                                     \
    $(top_srcdir)/third_party/NordicSemiconductor/dependencies/nrfx_log.h                                      \
    $(top_srcdir)/third_party/NordicSemiconductor/dependencies/sdk_common.h                                    \
    $(top_srcdir)/third_party/NordicSemiconductor/dependencies/sdk_config.h                                    \
    $(top_srcdir)/third_party/NordicSemiconductor/dependencies/sdk_errors.h                                    \
    $(top_srcdir)/third_party/NordicSemiconductor/dependencies/sdk_macros.h                                    \
    $(top_srcdir)/third_party/NordicSemiconductor/dependencies/sdk_os.h                                        \
    $(top_srcdir)/third_party/NordicSemiconductor/dependencies/sdk_resources.h                                 \
    $(top_srcdir)/third_party/NordicSemiconductor/drivers/clock/nrf_drv_clock.h                                \
    $(top_srcdir)/third_party/NordicSemiconductor/drivers/common/nrf_drv_common.h                              \
    $(top_srcdir)/third_party/NordicSemiconductor/drivers/power/nrf_drv_power.h                                \
    $(top_srcdir)/third_party/NordicSemiconductor/drivers/systick/nrf_drv_systick.h                            \
    $(top_srcdir)/third_party/NordicSemiconductor/drivers/usbd/nrf_drv_usbd_errata.h                           \
    $(top_srcdir)/third_party/NordicSemiconductor/drivers/usbd/nrf_drv_usbd.h                                  \
    $(top_srcdir)/third_party/NordicSemiconductor/drivers/radio/nrf_802154.h                                   \
    $(top_srcdir)/third_party/NordicSemiconductor/drivers/radio/nrf_802154_ack_pending_bit.h                   \
    $(top_srcdir)/third_party/NordicSemiconductor/drivers/radio/nrf_802154_core.h                              \
    $(top_srcdir)/third_party/NordicSemiconductor/drivers/radio/nrf_802154_core_hooks.h                        \
    $(top_srcdir)/third_party/NordicSemiconductor/drivers/radio/nrf_802154_config.h                            \
    $(top_srcdir)/third_party/NordicSemiconductor/drivers/radio/nrf_802154_const.h                             \
    $(top_srcdir)/third_party/NordicSemiconductor/drivers/radio/nrf_802154_critical_section.h                  \
    $(top_srcdir)/third_party/NordicSemiconductor/drivers/radio/nrf_802154_debug.h                             \
    $(top_srcdir)/third_party/NordicSemiconductor/drivers/radio/nrf_802154_notification.h                      \
    $(top_srcdir)/third_party/NordicSemiconductor/drivers/radio/nrf_802154_pib.h                               \
    $(top_srcdir)/third_party/NordicSemiconductor/drivers/radio/nrf_802154_priority_drop.h                     \
    $(top_srcdir)/third_party/NordicSemiconductor/drivers/radio/nrf_802154_procedures_duration.h               \
    $(top_srcdir)/third_party/NordicSemiconductor/drivers/radio/nrf_802154_request.h                           \
    $(top_srcdir)/third_party/NordicSemiconductor/drivers/radio/nrf_802154_revision.h                          \
    $(top_srcdir)/third_party/NordicSemiconductor/drivers/radio/nrf_802154_rssi.h                              \
    $(top_srcdir)/third_party/NordicSemiconductor/drivers/radio/nrf_802154_rx_buffer.h                         \
    $(top_srcdir)/third_party/NordicSemiconductor/drivers/radio/nrf_802154_timer_coord.h                       \
    $(top_srcdir)/third_party/NordicSemiconductor/drivers/radio/nrf_802154_swi.h                               \
    $(top_srcdir)/third_party/NordicSemiconductor/drivers/radio/nrf_802154_utils.h                             \
    $(top_srcdir)/third_party/NordicSemiconductor/drivers/radio/fem/nrf_fem_control_api.h                      \
    $(top_srcdir)/third_party/NordicSemiconductor/drivers/radio/fem/nrf_fem_control_config.h                   \
    $(top_srcdir)/third_party/NordicSemiconductor/drivers/radio/hal/nrf_radio.h                                \
    $(top_srcdir)/third_party/NordicSemiconductor/drivers/radio/mac_features/nrf_802154_ack_timeout.h          \
    $(top_srcdir)/third_party/NordicSemiconductor/drivers/radio/mac_features/nrf_802154_csma_ca.h              \
    $(top_srcdir)/third_party/NordicSemiconductor/drivers/radio/mac_features/nrf_802154_filter.h               \
    $(top_srcdir)/third_party/NordicSemiconductor/drivers/radio/platform/clock/nrf_802154_clock.h              \
    $(top_srcdir)/third_party/NordicSemiconductor/drivers/radio/platform/temperature/nrf_802154_temperature.h  \
    $(top_srcdir)/third_party/NordicSemiconductor/drivers/radio/platform/hp_timer/nrf_802154_hp_timer.h        \
    $(top_srcdir)/third_party/NordicSemiconductor/drivers/radio/platform/lp_timer/nrf_802154_lp_timer.h        \
    $(top_srcdir)/third_party/NordicSemiconductor/drivers/radio/rsch/nrf_802154_rsch.h                         \
    $(top_srcdir)/third_party/NordicSemiconductor/drivers/radio/rsch/nrf_802154_wifi_coex.h                    \
    $(top_srcdir)/third_party/NordicSemiconductor/drivers/radio/rsch/nrf_802154_rsch_crit_sect.h               \
    $(top_srcdir)/third_party/NordicSemiconductor/drivers/radio/rsch/raal/nrf_raal_api.h                       \
    $(top_srcdir)/third_party/NordicSemiconductor/drivers/radio/rsch/raal/nrf_raal_config.h                    \
    $(top_srcdir)/third_party/NordicSemiconductor/drivers/radio/rsch/raal/softdevice/nrf_raal_softdevice.h     \
    $(top_srcdir)/third_party/NordicSemiconductor/drivers/radio/timer_scheduler/nrf_802154_timer_sched.h       \
    $(top_srcdir)/third_party/NordicSemiconductor/libraries/app_error/app_error_weak.h                         \
    $(top_srcdir)/third_party/NordicSemiconductor/libraries/app_error/app_error.h                              \
    $(top_srcdir)/third_party/NordicSemiconductor/libraries/atfifo/nrf_atfifo_internal.h                       \
    $(top_srcdir)/third_party/NordicSemiconductor/libraries/atfifo/nrf_atfifo.h                                \
    $(top_srcdir)/third_party/NordicSemiconductor/libraries/atomic/nrf_atomic_internal.h                       \
    $(top_srcdir)/third_party/NordicSemiconductor/libraries/atomic/nrf_atomic.h                                \
    $(top_srcdir)/third_party/NordicSemiconductor/libraries/delay/nrf_delay.h                                  \
    $(top_srcdir)/third_party/NordicSemiconductor/libraries/usb/app_usbd_class_base.h                          \
    $(top_srcdir)/third_party/NordicSemiconductor/libraries/usb/app_usbd_core.h                                \
    $(top_srcdir)/third_party/NordicSemiconductor/libraries/usb/app_usbd_descriptor.h                          \
    $(top_srcdir)/third_party/NordicSemiconductor/libraries/usb/app_usbd_langid.h                              \
    $(top_srcdir)/third_party/NordicSemiconductor/libraries/usb/app_usbd_request.h                             \
    $(top_srcdir)/third_party/NordicSemiconductor/libraries/usb/app_usbd_serial_num.h                          \
    $(top_srcdir)/third_party/NordicSemiconductor/libraries/usb/app_usbd_string_desc.h                         \
    $(top_srcdir)/third_party/NordicSemiconductor/libraries/usb/app_usbd_types.h                               \
    $(top_srcdir)/third_party/NordicSemiconductor/libraries/usb/app_usbd.h                                     \
    $(top_srcdir)/third_party/NordicSemiconductor/libraries/usb/app_usbd_nrf_dfu_trigger.h                     \
    $(top_srcdir)/third_party/NordicSemiconductor/libraries/usb/app_usbd_nrf_dfu_trigger_types.h               \
    $(top_srcdir)/third_party/NordicSemiconductor/libraries/usb/app_usbd_nrf_dfu_trigger_internal.h            \
    $(top_srcdir)/third_party/NordicSemiconductor/libraries/usb/class/cdc/app_usbd_cdc_desc.h                  \
    $(top_srcdir)/third_party/NordicSemiconductor/libraries/usb/class/cdc/app_usbd_cdc_types.h                 \
    $(top_srcdir)/third_party/NordicSemiconductor/libraries/usb/class/cdc/acm/app_usbd_cdc_acm_internal.h      \
    $(top_srcdir)/third_party/NordicSemiconductor/libraries/usb/class/cdc/acm/app_usbd_cdc_acm.h               \
    $(top_srcdir)/third_party/NordicSemiconductor/libraries/usb/config/app_usbd_string_config.h                \
    $(top_srcdir)/third_party/NordicSemiconductor/libraries/utf_converter/utf.h                                \
    $(top_srcdir)/third_party/NordicSemiconductor/nrfx/drivers/nrfx_common.h                                   \
    $(top_srcdir)/third_party/NordicSemiconductor/nrfx/drivers/nrfx_errors.h                                   \
    $(top_srcdir)/third_party/NordicSemiconductor/nrfx/drivers/include/nrfx_clock.h                            \
    $(top_srcdir)/third_party/NordicSemiconductor/nrfx/drivers/include/nrfx_power_clock.h                      \
    $(top_srcdir)/third_party/NordicSemiconductor/nrfx/drivers/include/nrfx_power.h                            \
    $(top_srcdir)/third_party/NordicSemiconductor/nrfx/drivers/include/nrfx_systick.h                          \
    $(top_srcdir)/third_party/NordicSemiconductor/nrfx/hal/nrf_clock.h                                         \
    $(top_srcdir)/third_party/NordicSemiconductor/nrfx/hal/nrf_gpio.h                                          \
    $(top_srcdir)/third_party/NordicSemiconductor/nrfx/hal/nrf_gpiote.h                                        \
    $(top_srcdir)/third_party/NordicSemiconductor/nrfx/hal/nrf_egu.h                                           \
    $(top_srcdir)/third_party/NordicSemiconductor/nrfx/hal/nrf_nvmc.h                                          \
    $(top_srcdir)/third_party/NordicSemiconductor/nrfx/hal/nrf_ppi.h                                           \
    $(top_srcdir)/third_party/NordicSemiconductor/nrfx/hal/nrf_power.h                                         \
    $(top_srcdir)/third_party/NordicSemiconductor/nrfx/hal/nrf_rng.h                                           \
    $(top_srcdir)/third_party/NordicSemiconductor/nrfx/hal/nrf_spis.h                                          \
    $(top_srcdir)/third_party/NordicSemiconductor/nrfx/hal/nrf_uart.h                                          \
    $(top_srcdir)/third_party/NordicSemiconductor/nrfx/hal/nrf_usbd.h                                          \
    $(top_srcdir)/third_party/NordicSemiconductor/nrfx/mdk/compiler_abstraction.h                              \
    $(top_srcdir)/third_party/NordicSemiconductor/nrfx/mdk/nrf.h                                               \
    $(top_srcdir)/third_party/NordicSemiconductor/nrfx/mdk/nrf_peripherals.h                                   \
    $(top_srcdir)/third_party/NordicSemiconductor/nrfx/mdk/nrf51_to_nrf52.h                                    \
    $(top_srcdir)/third_party/NordicSemiconductor/nrfx/mdk/nrf51_to_nrf52840.h                                 \
    $(top_srcdir)/third_party/NordicSemiconductor/nrfx/mdk/nrf52_to_nrf52840.h                                 \
    $(top_srcdir)/third_party/NordicSemiconductor/nrfx/mdk/nrf52840.h                                          \
    $(top_srcdir)/third_party/NordicSemiconductor/nrfx/mdk/nrf52840_bitfields.h                                \
    $(top_srcdir)/third_party/NordicSemiconductor/nrfx/mdk/nrf52840_peripherals.h                              \
    $(top_srcdir)/third_party/NordicSemiconductor/nrfx/mdk/system_nrf52840.h                                   \
    $(top_srcdir)/third_party/NordicSemiconductor/softdevice/s140/headers/ble.h                                \
    $(top_srcdir)/third_party/NordicSemiconductor/softdevice/s140/headers/ble_err.h                            \
    $(top_srcdir)/third_party/NordicSemiconductor/softdevice/s140/headers/ble_gap.h                            \
    $(top_srcdir)/third_party/NordicSemiconductor/softdevice/s140/headers/ble_gatt.h                           \
    $(top_srcdir)/third_party/NordicSemiconductor/softdevice/s140/headers/ble_gattc.h                          \
    $(top_srcdir)/third_party/NordicSemiconductor/softdevice/s140/headers/ble_gatts.h                          \
    $(top_srcdir)/third_party/NordicSemiconductor/softdevice/s140/headers/ble_hci.h                            \
    $(top_srcdir)/third_party/NordicSemiconductor/softdevice/s140/headers/ble_l2cap.h                          \
    $(top_srcdir)/third_party/NordicSemiconductor/softdevice/s140/headers/ble_ranges.h                         \
    $(top_srcdir)/third_party/NordicSemiconductor/softdevice/s140/headers/ble_types.h                          \
    $(top_srcdir)/third_party/NordicSemiconductor/softdevice/s140/headers/nrf_error.h                          \
    $(top_srcdir)/third_party/NordicSemiconductor/softdevice/s140/headers/nrf_error_sdm.h                      \
    $(top_srcdir)/third_party/NordicSemiconductor/softdevice/s140/headers/nrf_error_soc.h                      \
    $(top_srcdir)/third_party/NordicSemiconductor/softdevice/s140/headers/nrf_nvic.h                           \
    $(top_srcdir)/third_party/NordicSemiconductor/softdevice/s140/headers/nrf_sdm.h                            \
    $(top_srcdir)/third_party/NordicSemiconductor/softdevice/s140/headers/nrf_soc.h                            \
    $(top_srcdir)/third_party/NordicSemiconductor/softdevice/s140/headers/nrf_svc.h                            \
    $(top_srcdir)/third_party/NordicSemiconductor/softdevice/s140/headers/nrf52/nrf_mbr.h                      \
    $(NULL)

PRETTY_FILES                                                                                               = \
    $(PLATFORM_COMMON_SOURCES)                                                                               \
    $(PLATFORM_DIAG_SOURCES)                                                                                 \
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
