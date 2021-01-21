#
#  Copyright (c) 2021, The OpenThread Authors.
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
    list(APPEND OT_PLATFORM_DEFINES "SPIS_AS_SERIAL_TRANSPORT=1")
else()
    option(OT_USB "enable nrf USB as serial transport support")
    if(OT_USB)
        list(APPEND OT_PLATFORM_DEFINES "USB_CDC_AS_SERIAL_TRANSPORT=1")
    else()
        list(APPEND OT_PLATFORM_DEFINES "UART_AS_SERIAL_TRANSPORT=1")
    endif()
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
    src/transport/spi-slave.c
    src/transport/transport.c
    src/transport/uart.c
    src/transport/usb-cdc-uart.c
)

set(NRF_SINGLEPHY_SOURCES
    src/flash_nosd.c
)

set(NRF_SOFTDEVICE_SOURCES
    src/flash_sd.c
    src/softdevice.c
)

set(NRF_INCLUDES
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${PROJECT_SOURCE_DIR}/examples/platforms
    ${PROJECT_SOURCE_DIR}/src/core
)

if(OT_PLATFORM STREQUAL "nrf52811")
    set(NRF_TRANSPORT_SOURCES
        src/transport/spi-slave.c
        src/transport/transport.c
        src/transport/uart.c
    )
    set(NRF52811_3RD_LIBS
        nordicsemi-nrf52811-radio-driver
        nordicsemi-nrf52811-sdk
        jlinkrtt
    )
    include(nrf52811/nrf52811.cmake)
elseif(OT_PLATFORM STREQUAL "nrf52833")
    set(NRF52833_3RD_LIBS
        nordicsemi-nrf52833-radio-driver
        nordicsemi-nrf52833-radio-driver-softdevice
        nordicsemi-nrf52833-sdk
        jlinkrtt
    )
    include(nrf52833/nrf52833.cmake)
elseif(OT_PLATFORM STREQUAL "nrf52840")
    set(NRF52840_3RD_LIBS
        nordicsemi-nrf52840-radio-driver
        nordicsemi-nrf52840-radio-driver-softdevice
        nordicsemi-nrf52840-sdk
        jlinkrtt
    )
    include(nrf52840/nrf52840.cmake)
endif()
