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

set(OT_PLATFORM_LIB "openthread-samr21" PARENT_SCOPE)

if(NOT OT_CONFIG)
    set(OT_CONFIG "openthread-core-samr21-config.h")
    set(OT_CONFIG ${OT_CONFIG} PARENT_SCOPE)
endif()

list(APPEND OT_PLATFORM_DEFINES
    "OPENTHREAD_CORE_CONFIG_PLATFORM_CHECK_FILE=\"openthread-core-samr21-config-check.h\""
    "MBEDTLS_CONFIG_FILE=\"mbedtls-config.h\""
)
set(OT_PLATFORM_DEFINES ${OT_PLATFORM_DEFINES} PARENT_SCOPE)

list(APPEND OT_PRIVATE_DEFINES "MBEDTLS_USER_CONFIG_FILE=\"samr21-mbedtls-config.h\"")
list(APPEND OT_PUBLIC_INCLUDES
    "${PROJECT_SOURCE_DIR}/examples/platforms/samr21/crypto"
)
set(OT_PUBLIC_INCLUDES ${OT_PUBLIC_INCLUDES} PARENT_SCOPE)
string(REPLACE "-pedantic-errors" "" OT_CFLAGS "${OT_CFLAGS}")

list(APPEND OT_PLATFORM_DEFINES "OPENTHREAD_PROJECT_CORE_CONFIG_FILE=\"${OT_CONFIG}\"")

add_library(openthread-samr21
    crypto/aes_alt.c
    alarm.c
    cxx_helpers.c
    diag.c
    entropy.c
    flash.c
    logging.c
    misc.c
    radio.c
    system.c
    uart.c
    ${PROJECT_SOURCE_DIR}/third_party/microchip/asf/common/utils/interrupt/interrupt_sam_nvic.c
    ${PROJECT_SOURCE_DIR}/third_party/microchip/asf/common2/services/delay/sam0/cycle_counter.c
    ${PROJECT_SOURCE_DIR}/third_party/microchip/asf/sam0/drivers/extint/extint_callback.c
    ${PROJECT_SOURCE_DIR}/third_party/microchip/asf/sam0/drivers/extint/extint_sam_d_r_h/extint.c
    ${PROJECT_SOURCE_DIR}/third_party/microchip/asf/sam0/drivers/nvm/nvm.c
    ${PROJECT_SOURCE_DIR}/third_party/microchip/asf/sam0/drivers/port/port.c
    ${PROJECT_SOURCE_DIR}/third_party/microchip/asf/sam0/drivers/system/clock/clock_samd21_r21_da_ha1/clock.c
    ${PROJECT_SOURCE_DIR}/third_party/microchip/asf/sam0/drivers/system/clock/clock_samd21_r21_da_ha1/gclk.c
    ${PROJECT_SOURCE_DIR}/third_party/microchip/asf/sam0/drivers/system/interrupt/system_interrupt.c
    ${PROJECT_SOURCE_DIR}/third_party/microchip/asf/sam0/drivers/system/pinmux/pinmux.c
    ${PROJECT_SOURCE_DIR}/third_party/microchip/asf/sam0/drivers/sercom/sercom.c
    ${PROJECT_SOURCE_DIR}/third_party/microchip/asf/sam0/drivers/sercom/sercom_interrupt.c
    ${PROJECT_SOURCE_DIR}/third_party/microchip/asf/sam0/drivers/sercom/i2c/i2c_sam0/i2c_master.c
    ${PROJECT_SOURCE_DIR}/third_party/microchip/asf/sam0/drivers/sercom/spi/spi.c
    ${PROJECT_SOURCE_DIR}/third_party/microchip/asf/sam0/drivers/sercom/usart/usart.c
    ${PROJECT_SOURCE_DIR}/third_party/microchip/asf/sam0/drivers/sercom/usart/usart_interrupt.c
    ${PROJECT_SOURCE_DIR}/third_party/microchip/asf/sam0/utils/cmsis/samr21/source/gcc/startup_samr21.c
    ${PROJECT_SOURCE_DIR}/third_party/microchip/asf/sam0/utils/cmsis/samr21/source/system_samr21.c
    ${PROJECT_SOURCE_DIR}/third_party/microchip/asf/sam0/utils/syscalls/gcc/syscalls.c
    ${PROJECT_SOURCE_DIR}/third_party/microchip/asf/thirdparty/wireless/avr2130_lwmesh/source/phy/at86rf233/src/phy.c
    ${PROJECT_SOURCE_DIR}/third_party/microchip/asf/thirdparty/wireless/services/sal/at86rf2xx/src/sal.c
    ${PROJECT_SOURCE_DIR}/third_party/microchip/asf/thirdparty/wireless/services/trx_access/trx_access.c
)

set_property(TARGET openthread-samr21 PROPERTY C_STANDARD 99)

target_link_options(openthread-samr21 PUBLIC -T${PROJECT_SOURCE_DIR}/third_party/microchip/include/samr21x18a.ld)
target_link_options(openthread-samr21 PUBLIC -Wl,-Map=$<TARGET_PROPERTY:NAME>.map)

target_compile_definitions(openthread-samr21
    PUBLIC
        ${OT_PLATFORM_DEFINES}
    PRIVATE
        ${OT_PRIVATE_DEFINES}
        -DBOARD=SAMR21_XPLAINED_PRO
        -D__SAMR21G18A__
        -DARM_MATH_CM0PLUS=true
        -DPHY_AT86RF233
        -DSAL_TYPE=AT86RF2xx
        -DUSART_CALLBACK_MODE=true
        -DEXTINT_CALLBACK_MODE=true
        -DSPI_CALLBACK_MODE=false
        -DCYCLE_MODE
)

target_compile_options(openthread-samr21 PRIVATE
    ${OT_CFLAGS}
    -Werror
    -Wno-unused-parameter
    -Wno-implicit-function-declaration
    -Wno-expansion-to-defined
    -fno-strict-aliasing
)

target_include_directories(openthread-samr21
    PRIVATE
        ${OT_PUBLIC_INCLUDES}
        ${OT_PRIVATE_INCLUDES}
        ${PROJECT_SOURCE_DIR}/src/core
        ${PROJECT_SOURCE_DIR}/examples/platforms
        ${PROJECT_SOURCE_DIR}/third_party/mbedtls/repo/include
        ${PROJECT_SOURCE_DIR}/third_party/mbedtls
        ${PROJECT_SOURCE_DIR}/third_party/microchip/include
        ${PROJECT_SOURCE_DIR}/third_party/microchip/asf/common/boards
        ${PROJECT_SOURCE_DIR}/third_party/microchip/asf/common/utils
        ${PROJECT_SOURCE_DIR}/third_party/microchip/asf/common2/services/delay
        ${PROJECT_SOURCE_DIR}/third_party/microchip/asf/common2/services/delay/sam0
        ${PROJECT_SOURCE_DIR}/third_party/microchip/asf/sam0/boards
        ${PROJECT_SOURCE_DIR}/third_party/microchip/asf/sam0/boards/samr21_xplained_pro
        ${PROJECT_SOURCE_DIR}/third_party/microchip/asf/sam0/drivers/extint
        ${PROJECT_SOURCE_DIR}/third_party/microchip/asf/sam0/drivers/extint/extint_sam_d_r_h
        ${PROJECT_SOURCE_DIR}/third_party/microchip/asf/sam0/drivers/nvm
        ${PROJECT_SOURCE_DIR}/third_party/microchip/asf/sam0/drivers/port
        ${PROJECT_SOURCE_DIR}/third_party/microchip/asf/sam0/drivers/system
        ${PROJECT_SOURCE_DIR}/third_party/microchip/asf/sam0/drivers/system/clock
        ${PROJECT_SOURCE_DIR}/third_party/microchip/asf/sam0/drivers/system/clock/clock_samd21_r21_da_ha1
        ${PROJECT_SOURCE_DIR}/third_party/microchip/asf/sam0/drivers/system/interrupt
        ${PROJECT_SOURCE_DIR}/third_party/microchip/asf/sam0/drivers/system/interrupt/system_interrupt_samr21
        ${PROJECT_SOURCE_DIR}/third_party/microchip/asf/sam0/drivers/system/pinmux
        ${PROJECT_SOURCE_DIR}/third_party/microchip/asf/sam0/drivers/system/power
        ${PROJECT_SOURCE_DIR}/third_party/microchip/asf/sam0/drivers/system/power/power_sam_d_r_h
        ${PROJECT_SOURCE_DIR}/third_party/microchip/asf/sam0/drivers/system/reset
        ${PROJECT_SOURCE_DIR}/third_party/microchip/asf/sam0/drivers/system/reset/reset_sam_d_r_h
        ${PROJECT_SOURCE_DIR}/third_party/microchip/asf/sam0/drivers/sercom
        ${PROJECT_SOURCE_DIR}/third_party/microchip/asf/sam0/drivers/sercom/i2c
        ${PROJECT_SOURCE_DIR}/third_party/microchip/asf/sam0/drivers/sercom/i2c/i2c_sam0
        ${PROJECT_SOURCE_DIR}/third_party/microchip/asf/sam0/drivers/sercom/spi
        ${PROJECT_SOURCE_DIR}/third_party/microchip/asf/sam0/drivers/sercom/usart
        ${PROJECT_SOURCE_DIR}/third_party/microchip/asf/sam0/utils/cmsis/samr21/include
        ${PROJECT_SOURCE_DIR}/third_party/microchip/asf/sam0/utils/cmsis/samr21/source
        ${PROJECT_SOURCE_DIR}/third_party/microchip/asf/sam0/utils/header_files
        ${PROJECT_SOURCE_DIR}/third_party/microchip/asf/sam0/utils/preprocessor
        ${PROJECT_SOURCE_DIR}/third_party/microchip/asf/thirdparty/CMSIS/Include
        ${PROJECT_SOURCE_DIR}/third_party/microchip/asf/thirdparty/wireless/avr2130_lwmesh/source/phy/at86rf233/inc
        ${PROJECT_SOURCE_DIR}/third_party/microchip/asf/thirdparty/wireless/services/sal/inc
        ${PROJECT_SOURCE_DIR}/third_party/microchip/asf/thirdparty/wireless/services/trx_access
        ${PROJECT_SOURCE_DIR}/third_party/microchip/asf/thirdparty/wireless/services/trx_access/module_config
)
