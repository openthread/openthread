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

if(OT_NCP_SPI)
    list(APPEND OT_PRIVATE_DEFINES "SPIS_AS_SERIAL_TRANSPORT=1")
endif()
option(OT_USB "enable nrf USB support")
if(OT_USB)
   list(APPEND OT_PRIVATE_DEFINES "USB_CDC_AS_SERIAL_TRANSPORT=1")
else()
   list(APPEND OT_PRIVATE_DEFINES "UART_AS_SERIAL_TRANSPORT=1")
endif()

set(NRF_COMM_SOURCES
    src/alarm.c
    src/diag.c
    src/entropy.c
    src/fem.c
    src/flash.c
    src/logging.c
    src/misc.c
    src/radio.c
    src/system.c
    src/temp.c
)

set(NRF_TRANSPORT_SOURCES
    src/transport/uart.c
    src/transport/spi-slave.c
    src/transport/usb-cdc-uart.c
    src/transport/transport.c
)

set(NRF_SINGLEPHY_SOURCES
    src/flash_nosd.c
)

set(NRF_SOFTDEVICE_SOURCES
    src/flash_sd.c
    src/softdevice.c
)

set(NRF_INCLUDES
    ${PROJECT_SOURCE_DIR}/third_party/NordicSemiconductor
    ${PROJECT_SOURCE_DIR}/third_party/NordicSemiconductor/cmsis
    ${PROJECT_SOURCE_DIR}/third_party/NordicSemiconductor/config
    ${PROJECT_SOURCE_DIR}/third_party/NordicSemiconductor/config/nrf52840/config
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
    ${PROJECT_SOURCE_DIR}/third_party/NordicSemiconductor/drivers/radio
    ${PROJECT_SOURCE_DIR}/third_party/NordicSemiconductor/drivers/radio/fem/three_pin_gpio
    ${PROJECT_SOURCE_DIR}/third_party/NordicSemiconductor/drivers/radio/hal
    ${PROJECT_SOURCE_DIR}/third_party/NordicSemiconductor/drivers/radio/mac_features
    ${PROJECT_SOURCE_DIR}/third_party/NordicSemiconductor/drivers/radio/mac_features/ack_generator
    ${PROJECT_SOURCE_DIR}/third_party/NordicSemiconductor/drivers/radio/rsch
    ${PROJECT_SOURCE_DIR}/third_party/NordicSemiconductor/drivers/radio/rsch/raal
    ${PROJECT_SOURCE_DIR}/third_party/NordicSemiconductor/drivers/radio/rsch/raal/softdevice
    ${PROJECT_SOURCE_DIR}/third_party/NordicSemiconductor/drivers/radio/platform/lp_timer
    ${PROJECT_SOURCE_DIR}/third_party/NordicSemiconductor/drivers/radio/platform/temperature
)

set(NRF_NONMBEDTLS_LIBS
    ${PROJECT_SOURCE_DIR}/third_party/NordicSemiconductor/libraries/nrf_security/lib/libmbedcrypto_glue.a
    ${PROJECT_SOURCE_DIR}/third_party/NordicSemiconductor/libraries/nrf_security/lib/libmbedcrypto_glue_cc310.a
    ${PROJECT_SOURCE_DIR}/third_party/NordicSemiconductor/libraries/nrf_security/lib/libmbedcrypto_glue_vanilla.a
    ${PROJECT_SOURCE_DIR}/third_party/NordicSemiconductor/libraries/nrf_security/lib/libmbedcrypto_vanilla_backend.a
    ${PROJECT_SOURCE_DIR}/third_party/NordicSemiconductor/libraries/nrf_security/lib/libmbedtls_tls_vanilla.a
    ${PROJECT_SOURCE_DIR}/third_party/NordicSemiconductor/libraries/nrf_security/lib/libmbedtls_x509_vanilla.a
    ${PROJECT_SOURCE_DIR}/third_party/NordicSemiconductor/libraries/nrf_security/lib/libmbedtls_base_vanilla.a
    ${PROJECT_SOURCE_DIR}/third_party/NordicSemiconductor/libraries/nrf_security/lib/libnrf_cc310_platform_0.9.2.a
    ${PROJECT_SOURCE_DIR}/third_party/NordicSemiconductor/libraries/nrf_security/lib/libmbedcrypto_cc310_backend.a
    ${PROJECT_SOURCE_DIR}/third_party/NordicSemiconductor/libraries/nrf_security/lib/libmbedtls_base_vanilla.a
)


if(OT_PLATFORM STREQUAL "nrf52840")
    set(NRF52840_3RD_LIBS
        nordicsemi-nrf52840-radio-driver
        nordicsemi-nrf52840-sdk
        nordicsemi-nrf52840-radio-driver
        jlinkrtt
    )
    include(nrf52840/nrf52840.cmake)
endif()

