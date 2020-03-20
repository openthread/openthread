#
#  Copyright (c) 2020, The OpenThread Authors.
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

set(COMMON_INCLUDES
    ${PROJECT_SOURCE_DIR}/include
    ${PROJECT_SOURCE_DIR}/src
    ${PROJECT_SOURCE_DIR}/src/core
    ${PROJECT_SOURCE_DIR}/third_party/NordicSemiconductor/drivers/radio
    ${PROJECT_SOURCE_DIR}/third_party/NordicSemiconductor/drivers/radio/fem
    ${PROJECT_SOURCE_DIR}/third_party/NordicSemiconductor/drivers/radio/fem/three_pin_gpio
    ${PROJECT_SOURCE_DIR}/third_party/NordicSemiconductor/drivers/radio/rsch
    ${PROJECT_SOURCE_DIR}/third_party/NordicSemiconductor/drivers/radio/rsch/raal
    ${PROJECT_SOURCE_DIR}/third_party/NordicSemiconductor/drivers/radio/rsch/raal/softdevice
    ${PROJECT_SOURCE_DIR}/third_party/NordicSemiconductor/drivers/radio/mac_features
    ${PROJECT_SOURCE_DIR}/third_party/NordicSemiconductor/drivers/radio/mac_features/ack_generator
)

set(COMMON_FLAG
    -DSPIS_ENABLED=1
    -DSPIS0_ENABLED=1
    -DNRFX_SPIS_ENABLED=1
    -DNRFX_SPIS0_ENABLED=1
    -DCONFIG_GPIO_AS_PINRESET
    -DENABLE_FEM=1
    -DUSE_APP_CONFIG=1
)


set(COMMON_SOURCES
    dependencies/app_util_platform.c
    drivers/clock/nrf_drv_clock.c
    drivers/power/nrf_drv_power.c
    libraries/app_error/app_error.c
    libraries/app_error/app_error_weak.c
    libraries/utf_converter/utf.c
    nrfx/drivers/src/nrfx_clock.c
    nrfx/drivers/src/nrfx_nvmc.c
    nrfx/drivers/src/nrfx_power.c
    nrfx/drivers/src/nrfx_spis.c
    nrfx/drivers/src/nrfx_systick.c
    nrfx/hal/nrf_nvmc.c
    nrfx/soc/nrfx_atomic.c
)

set(USB_SOURCES
    nrfx/drivers/src/nrfx_usbd.c
    libraries/atfifo/nrf_atfifo.c
    libraries/atomic/nrf_atomic.c
    libraries/usb/app_usbd.c
    libraries/usb/app_usbd_core.c
    libraries/usb/app_usbd_string_desc.c
    libraries/usb/app_usbd_serial_num.c
    libraries/usb/app_usbd_nrf_dfu_trigger.c
    libraries/usb/nrf_dfu_trigger_usb.c
    libraries/usb/class/cdc/acm/app_usbd_cdc_acm.c
)

set(RADIO_DRIVER_SOURCES
    drivers/radio/fal/nrf_802154_fal.c
    drivers/radio/mac_features/ack_generator/nrf_802154_ack_data.c
    drivers/radio/mac_features/ack_generator/nrf_802154_ack_generator.c
    drivers/radio/mac_features/ack_generator/nrf_802154_enh_ack_generator.c
    drivers/radio/mac_features/ack_generator/nrf_802154_imm_ack_generator.c
    drivers/radio/mac_features/nrf_802154_csma_ca.c
    drivers/radio/mac_features/nrf_802154_delayed_trx.c
    drivers/radio/mac_features/nrf_802154_filter.c
    drivers/radio/mac_features/nrf_802154_frame_parser.c
    drivers/radio/mac_features/nrf_802154_precise_ack_timeout.c
    drivers/radio/fem/three_pin_gpio/nrf_fem_three_pin_gpio.c
    drivers/radio/nrf_802154_core_hooks.c
    drivers/radio/nrf_802154_core.c
    drivers/radio/nrf_802154_critical_section.c
    drivers/radio/nrf_802154_debug.c
    drivers/radio/nrf_802154_pib.c
    drivers/radio/nrf_802154_rssi.c
    drivers/radio/nrf_802154_rx_buffer.c
    drivers/radio/nrf_802154_timer_coord.c
    drivers/radio/nrf_802154.c
    drivers/radio/platform/coex/nrf_802154_wifi_coex_none.c
    drivers/radio/platform/clock/nrf_802154_clock_ot.c
    drivers/radio/platform/hp_timer/nrf_802154_hp_timer.c
    drivers/radio/rsch/nrf_802154_rsch_crit_sect.c
    drivers/radio/rsch/nrf_802154_rsch.c
    drivers/radio/timer_scheduler/nrf_802154_timer_sched.c
)

set(SINGLE_PHY_SOURCES
    drivers/radio/nrf_802154_notification_direct.c
    drivers/radio/nrf_802154_priority_drop_direct.c
    drivers/radio/nrf_802154_request_direct.c
    drivers/radio/rsch/raal/single_phy/single_phy.c
)

set(RADIO_DRIVER_SOFTDEVICE_SOURCES
    drivers/radio/nrf_802154_notification_swi.c
    drivers/radio/nrf_802154_priority_drop_swi.c
    drivers/radio/nrf_802154_request_swi.c
    drivers/radio/nrf_802154_swi.c
    drivers/radio/rsch/raal/softdevice/nrf_raal_softdevice.c
)

set(RADIO_DRIVER_SINGLE_PHY_SOURCES
    drivers/radio/nrf_802154_notification_direct.c
    drivers/radio/nrf_802154_priority_drop_direct.c
    drivers/radio/nrf_802154_request_direct.c
    drivers/radio/rsch/raal/single_phy/single_phy.c
)

if(OT_PLATFORM_EXAMPLE)
    set(EXAMPLE_INCLUDES
        ${PROJECT_SOURCE_DIR}/third_party/NordicSemiconductor
        ${PROJECT_SOURCE_DIR}/third_party/NordicSemiconductor/cmsis
        ${PROJECT_SOURCE_DIR}/third_party/NordicSemiconductor/config
        ${PROJECT_SOURCE_DIR}/third_party/NordicSemiconductor/dependencies
        ${PROJECT_SOURCE_DIR}/third_party/NordicSemiconductor/drivers/clock
        ${PROJECT_SOURCE_DIR}/third_party/NordicSemiconductor/drivers/common
        ${PROJECT_SOURCE_DIR}/third_party/NordicSemiconductor/drivers/power
        ${PROJECT_SOURCE_DIR}/third_party/NordicSemiconductor/drivers/systick
        ${PROJECT_SOURCE_DIR}/third_party/NordicSemiconductor/drivers/usbd
        ${PROJECT_SOURCE_DIR}/third_party/NordicSemiconductor/libraries/app_error
        ${PROJECT_SOURCE_DIR}/third_party/NordicSemiconductor/libraries/atfifo
        ${PROJECT_SOURCE_DIR}/third_party/NordicSemiconductor/libraries/atomic
        ${PROJECT_SOURCE_DIR}/third_party/NordicSemiconductor/libraries/block_dev
        ${PROJECT_SOURCE_DIR}/third_party/NordicSemiconductor/libraries/delay
        ${PROJECT_SOURCE_DIR}/third_party/NordicSemiconductor/libraries/usb
        ${PROJECT_SOURCE_DIR}/third_party/NordicSemiconductor/libraries/usb/config
        ${PROJECT_SOURCE_DIR}/third_party/NordicSemiconductor/libraries/usb/class/cdc
        ${PROJECT_SOURCE_DIR}/third_party/NordicSemiconductor/libraries/usb/class/cdc/acm
        ${PROJECT_SOURCE_DIR}/third_party/NordicSemiconductor/libraries/utf_converter
        ${PROJECT_SOURCE_DIR}/third_party/NordicSemiconductor/nrfx
        ${PROJECT_SOURCE_DIR}/third_party/NordicSemiconductor/nrfx/hal
        ${PROJECT_SOURCE_DIR}/third_party/NordicSemiconductor/nrfx/drivers
        ${PROJECT_SOURCE_DIR}/third_party/NordicSemiconductor/nrfx/drivers/include
        ${PROJECT_SOURCE_DIR}/third_party/NordicSemiconductor/nrfx/mdk
        ${PROJECT_SOURCE_DIR}/third_party/NordicSemiconductor/nrfx/soc
        ${PROJECT_SOURCE_DIR}/third_party/NordicSemiconductor/softdevice/s140/headers
        ${PROJECT_SOURCE_DIR}/third_party/NordicSemiconductor/softdevice/s140/headers/nrf52
    )
    if(OT_PLATFORM_EXAMPLE STREQUAL "nrf52811")
        add_library(nordicsemi-nrf52811-sdk)
        target_compile_definitions(nordicsemi-nrf52811-sdk
            PRIVATE
                ${COMMON_FLAG}
                -DNRF52811_XXAA
                -DNRF_802154_PROJECT_CONFIG=\"platform-config.h\"
                -D__HEAP_SIZE=0
                -D__STACK_SIZE=2048
        )
        target_include_directories(nordicsemi-nrf52811-sdk
            PUBLIC
                ${COMMON_INCLUDES}
                ${EXAMPLE_INCLUDES}
                ${PROJECT_SOURCE_DIR}/examples/platforms/nrf528xx/nrf52811
                ${PROJECT_SOURCE_DIR}/third_party/NordicSemiconductor/config/nrf52811/config
        )
        set_property(SOURCE nrfx/mdk/gcc_startup_nrf52811.S PROPERTY LANGUAGE C)
        target_sources(nordicsemi-nrf52811-sdk
            PRIVATE
                ${COMMON_SOURCES}
                nrfx/mdk/gcc_startup_nrf52811.S
                nrfx/mdk/system_nrf52811.c
        )
    elseif(OT_PLATFORM_EXAMPLE STREQUAL "nrf52833")
        add_library(nordicsemi-nrf52833-sdk)
        target_compile_definitions(nordicsemi-nrf52833-sdk
            PRIVATE
                ${COMMON_FLAG}
                -DNRF52833_XXAA
                -DNRF_802154_PROJECT_CONFIG=\"platform-config.h\"
        )
        target_include_directories(nordicsemi-nrf52833-sdk
            PUBLIC
                ${COMMON_INCLUDES}
                ${EXAMPLE_INCLUDES}
                ${PROJECT_SOURCE_DIR}/examples/platforms/nrf52833
                ${PROJECT_SOURCE_DIR}/third_party/NordicSemiconductor/config/nrf52833/config
        )
        set_property(SOURCE nrfx/mdk/gcc_startup_nrf52833.S PROPERTY LANGUAGE C)
        target_sources(nordicsemi-nrf52833-sdk
            PRIVATE
                ${COMMON_SOURCES}
                ${USB_SOURCES}
                nrfx/mdk/gcc_startup_nrf52833.S
                nrfx/mdk/system_nrf52833.c
        )
    elseif(OT_PLATFORM_EXAMPLE STREQUAL "nrf52840")
        add_library(nordicsemi-nrf52840-sdk)
        target_compile_definitions(nordicsemi-nrf52840-sdk
            PRIVATE
                ${COMMON_FLAG}
                -DNRF52840_XXAA
                -DNRF_802154_PROJECT_CONFIG=\"platform-config.h\"
        )
        target_include_directories(nordicsemi-nrf52840-sdk
            PUBLIC
                ${COMMON_INCLUDES}
                ${EXAMPLE_INCLUDES}
                ${PROJECT_SOURCE_DIR}/examples/platforms/nrf528xx/nrf52840/
                ${PROJECT_SOURCE_DIR}/third_party/NordicSemiconductor/config/nrf52840/config
        )
        set_property(SOURCE nrfx/mdk/gcc_startup_nrf52840.S PROPERTY LANGUAGE C)
        target_sources(nordicsemi-nrf52840-sdk
            PRIVATE
                ${COMMON_SOURCES}
                ${USB_SOURCES}
                nrfx/mdk/gcc_startup_nrf52840.S
                nrfx/mdk/system_nrf52840.c
        )
    else()
        message(FATAL_ERROR "Platform examples unknown: ${OT_PLATFORM_EXAMPLE}")
    endif()
endif()

if(OT_PLATFORM STREQUAL "nrf52811")
    add_library(nordicsemi-nrf52811-radio-driver)
    target_compile_definitions(nordicsemi-nrf52811-radio-driver
        PRIVATE
            ${COMMON_FLAG}
            -DNRF52811_XXAA
            -DNRF_802154_PROJECT_CONFIG=\"platform-config.h\"
            -D__HEAP_SIZE=0
            -D__STACK_SIZE=2048
            -DRAAL_SINGLE_PHY=1
    )
    target_include_directories(nordicsemi-nrf52811-radio-driver
        PUBLIC
            ${COMMON_INCLUDES}
            ${PROJECT_SOURCE_DIR}/examples/platforms/nrf528xx/nrf52811
            ${PROJECT_SOURCE_DIR}/third_party/NordicSemiconductor/config/nrf52811/config
    )
    target_sources(nordicsemi-nrf52811-radio-driver
        PRIVATE
            ${RADIO_DRIVER_SOURCES}
            ${RADIO_DRIVER_SINGLE_PHY_SOURCES}
    )
elseif(OT_PLATFORM STREQUAL "nrf52833")
    add_library(nordicsemi-nrf52833-radio-driver)
    target_compile_definitions(nordicsemi-nrf52833-radio-driver
        PRIVATE
            ${COMMON_FLAG}
            -DNRF52833_XXAA
            -DNRF_802154_PROJECT_CONFIG=\"platform-config.h\"
            -DRAAL_SINGLE_PHY=1
    )
    target_include_directories(nordicsemi-nrf52833-radio-driver
        PUBLIC
            ${COMMON_INCLUDES}
            ${PROJECT_SOURCE_DIR}/examples/platforms/nrf52833
            ${PROJECT_SOURCE_DIR}/third_party/NordicSemiconductor/config/nrf52833/config
    )
    target_sources(nordicsemi-nrf52833-radio-driver
        PRIVATE
            ${RADIO_DRIVER_SOURCES}
            ${RADIO_DRIVER_SINGLE_PHY_SOURCES}
    )
    add_library(nordicsemi-nrf52833-radio-driver-softdevice)
    target_compile_definitions(nordicsemi-nrf52833-radio-driver-softdevice
        PRIVATE
            ${COMMON_FLAG}
            -DNRF52833_XXAA
            -DNRF_802154_PROJECT_CONFIG=\"platform-config.h\"
            -DRAAL_SINGLE_PHY=1
            -DSOFTDEVICE_PRESENT
            -DS140
            -DRAAL_SOFTDEVICE=1
    )
    target_include_directories(nordicsemi-nrf52833-radio-driver-softdevice
        PUBLIC
            ${COMMON_INCLUDES}
            ${PROJECT_SOURCE_DIR}/examples/platforms/nrf52833
            ${PROJECT_SOURCE_DIR}/third_party/NordicSemiconductor/config/nrf52833/config
    )
    target_sources(nordicsemi-nrf52833-radio-driver-softdevice
        PRIVATE
            ${RADIO_DRIVER_SOURCES}
            ${RADIO_DRIVER_SINGLE_PHY_SOURCES}
    )
elseif(OT_PLATFORM STREQUAL "nrf52840")
    add_library(nordicsemi-nrf52840-radio-driver)
    target_compile_definitions(nordicsemi-nrf52840-radio-driver
        PRIVATE
            ${COMMON_FLAG}
            -DNRF52840_XXAA
            -DNRF_802154_PROJECT_CONFIG=\"platform-config.h\"
            -DRAAL_SINGLE_PHY=1
    )
    target_include_directories(nordicsemi-nrf52840-radio-driver
        PUBLIC
            ${COMMON_INCLUDES}
            ${EXAMPLE_INCLUDES}
            ${OT_PUBLIC_INCLUDES}
            ${PROJECT_SOURCE_DIR}/examples/platforms/nrf528xx/nrf52840/
            ${PROJECT_SOURCE_DIR}/third_party/NordicSemiconductor/config/nrf52840/config
    )
    target_sources(nordicsemi-nrf52840-radio-driver
        PRIVATE
            ${RADIO_DRIVER_SOURCES}
            ${RADIO_DRIVER_SINGLE_PHY_SOURCES}
    )
    add_library(nordicsemi-nrf52840-radio-driver-softdevice)
    target_compile_definitions(nordicsemi-nrf52840-radio-driver-softdevice
        PRIVATE
            ${COMMON_FLAG}
            -DNRF52840_XXAA
            -DNRF_802154_PROJECT_CONFIG=\"platform-config.h\"
            -DRAAL_SINGLE_PHY=1
            -DSOFTDEVICE_PRESENT
            -DS140
            -DRAAL_SOFTDEVICE=1
    )
    target_include_directories(nordicsemi-nrf52840-radio-driver-softdevice
        PUBLIC
            ${COMMON_INCLUDES}
            ${EXAMPLE_INCLUDES}
            ${OT_PUBLIC_INCLUDES}
            ${PROJECT_SOURCE_DIR}/examples/platforms/nrf528xx/nrf52840/
            ${PROJECT_SOURCE_DIR}/examples/platforms/nrf528xx/nrf52840/
            ${PROJECT_SOURCE_DIR}/third_party/NordicSemiconductor/config/nrf52840/config
    )
    target_sources(nordicsemi-nrf52840-radio-driver-softdevice
        PRIVATE
            ${RADIO_DRIVER_SOURCES}
            ${RADIO_DRIVER_SINGLE_PHY_SOURCES}
    )
else()
    message(FATAL_ERROR "Platform unknown: ${OT_PLATFORM}")
endif()


