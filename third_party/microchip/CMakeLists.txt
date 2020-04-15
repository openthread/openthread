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

add_library(samr21-driver
    asf/common/utils/interrupt/interrupt_sam_nvic.c
    asf/common2/services/delay/sam0/cycle_counter.c
    asf/sam0/drivers/extint/extint_callback.c
    asf/sam0/drivers/extint/extint_sam_d_r_h/extint.c
    asf/sam0/drivers/nvm/nvm.c
    asf/sam0/drivers/port/port.c
    asf/sam0/drivers/system/clock/clock_samd21_r21_da_ha1/clock.c
    asf/sam0/drivers/system/clock/clock_samd21_r21_da_ha1/gclk.c
    asf/sam0/drivers/system/interrupt/system_interrupt.c
    asf/sam0/drivers/system/pinmux/pinmux.c
    asf/sam0/drivers/sercom/sercom.c
    asf/sam0/drivers/sercom/sercom_interrupt.c
    asf/sam0/drivers/sercom/i2c/i2c_sam0/i2c_master.c
    asf/sam0/drivers/sercom/spi/spi.c
    asf/sam0/drivers/sercom/usart/usart.c
    asf/sam0/drivers/sercom/usart/usart_interrupt.c
    asf/sam0/utils/cmsis/samr21/source/gcc/startup_samr21.c
    asf/sam0/utils/cmsis/samr21/source/system_samr21.c
    asf/sam0/utils/syscalls/gcc/syscalls.c
    asf/thirdparty/wireless/avr2130_lwmesh/source/phy/at86rf233/src/phy.c
    asf/thirdparty/wireless/services/sal/at86rf2xx/src/sal.c
    asf/thirdparty/wireless/services/trx_access/trx_access.c
)

string(REPLACE "-pedantic-errors" "" OT_CFLAGS "${OT_CFLAGS}")

target_link_options(samr21-driver
    PUBLIC
        -T${PROJECT_SOURCE_DIR}/third_party/microchip/include/samr21x18a.ld
)

target_compile_definitions(samr21-driver
    PUBLIC
        ${OT_PLATFORM_DEFINES}
        -DBOARD=SAMR21_XPLAINED_PRO
        -D__SAMR21G18A__
        -DARM_MATH_CM0PLUS=true
        -DPHY_AT86RF233
        -DSAL_TYPE=AT86RF2xx
        -DUSART_CALLBACK_MODE=true
        -DEXTINT_CALLBACK_MODE=true
        -DSPI_CALLBACK_MODE=false
        -DCYCLE_MODE
    PRIVATE
        ${OT_PRIVATE_DEFINES}
)

target_compile_options(samr21-driver
    PUBLIC
        -Werror
        -Wno-unused-parameter
        -Wno-implicit-function-declaration
        -Wno-expansion-to-defined
        -fno-strict-aliasing
    PRIVATE
        ${OT_CFLAGS}
)

target_include_directories(samr21-driver
    PUBLIC
        include
        asf/common/boards
        asf/common/utils
        asf/common2/services/delay
        asf/common2/services/delay/sam0
        asf/sam0/boards
        asf/sam0/boards/samr21_xplained_pro
        asf/sam0/drivers/extint
        asf/sam0/drivers/extint/extint_sam_d_r_h
        asf/sam0/drivers/nvm
        asf/sam0/drivers/port
        asf/sam0/drivers/system
        asf/sam0/drivers/system/clock
        asf/sam0/drivers/system/clock/clock_samd21_r21_da_ha1
        asf/sam0/drivers/system/interrupt
        asf/sam0/drivers/system/interrupt/system_interrupt_samr21
        asf/sam0/drivers/system/pinmux
        asf/sam0/drivers/system/power
        asf/sam0/drivers/system/power/power_sam_d_r_h
        asf/sam0/drivers/system/reset
        asf/sam0/drivers/system/reset/reset_sam_d_r_h
        asf/sam0/drivers/sercom
        asf/sam0/drivers/sercom/i2c
        asf/sam0/drivers/sercom/i2c/i2c_sam0
        asf/sam0/drivers/sercom/spi
        asf/sam0/drivers/sercom/usart
        asf/sam0/utils/cmsis/samr21/include
        asf/sam0/utils/cmsis/samr21/source
        asf/sam0/utils/header_files
        asf/sam0/utils/preprocessor
        asf/thirdparty/CMSIS/Include
        asf/thirdparty/wireless/avr2130_lwmesh/source/phy/at86rf233/inc
        asf/thirdparty/wireless/services/sal/inc
        asf/thirdparty/wireless/services/trx_access
        asf/thirdparty/wireless/services/trx_access/module_config
)
