#
#  Copyright (c) 2017, The OpenThread Authors.
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

lib_LIBRARIES = libopenthread-samr21.a

override CFLAGS                              := $(filter-out -pedantic-errors,$(CFLAGS))
override CXXFLAGS                            := $(filter-out -pedantic-errors,$(CXXFLAGS))

#SAMR21_BOARD=USER_BOARD
#SAMR21_CPU=__SAMR21E18A__

SAMR21_BOARD=SAMR21_XPLAINED_PRO
SAMR21_CPU=__SAMR21G18A__

libopenthread_samr21_a_CPPFLAGS                                                                              = \
    -D ARM_MATH_CM0PLUS=true                                                                                   \
    -D BOARD=$(SAMR21_BOARD)                                                                                   \
    -D $(SAMR21_CPU)                                                                                           \
    -D PHY_AT86RF233                                                                                           \
    -D SAL_TYPE=AT86RF2xx                                                                                      \
    -D USART_CALLBACK_MODE=true                                                                                \
    -D EXTINT_CALLBACK_MODE=true                                                                               \
    -D SPI_CALLBACK_MODE=false                                                                                 \
    -D CYCLE_MODE                                                                                              \
    -I$(top_srcdir)/include                                                                                    \
    -I$(top_srcdir)/examples/platforms                                                                         \
    -I$(top_srcdir)/src/core                                                                                   \
    -I$(top_srcdir)/third_party/microchip/include                                                              \
    -I$(top_srcdir)/third_party/microchip/asf/common/boards                                                    \
    -I$(top_srcdir)/third_party/microchip/asf/common/utils                                                     \
    -I$(top_srcdir)/third_party/microchip/asf/common2/services/delay                                           \
    -I$(top_srcdir)/third_party/microchip/asf/common2/services/delay/sam0                                      \
    -I$(top_srcdir)/third_party/microchip/asf/sam0/boards                                                      \
    -I$(top_srcdir)/third_party/microchip/asf/sam0/boards/samr21_xplained_pro                                  \
    -I$(top_srcdir)/third_party/microchip/asf/sam0/drivers/extint                                              \
    -I$(top_srcdir)/third_party/microchip/asf/sam0/drivers/extint/extint_sam_d_r_h                             \
    -I$(top_srcdir)/third_party/microchip/asf/sam0/drivers/nvm                                                 \
    -I$(top_srcdir)/third_party/microchip/asf/sam0/drivers/port                                                \
    -I$(top_srcdir)/third_party/microchip/asf/sam0/drivers/system                                              \
    -I$(top_srcdir)/third_party/microchip/asf/sam0/drivers/system/clock                                        \
    -I$(top_srcdir)/third_party/microchip/asf/sam0/drivers/system/clock/clock_samd21_r21_da_ha1                \
    -I$(top_srcdir)/third_party/microchip/asf/sam0/drivers/system/interrupt                                    \
    -I$(top_srcdir)/third_party/microchip/asf/sam0/drivers/system/interrupt/system_interrupt_samr21            \
    -I$(top_srcdir)/third_party/microchip/asf/sam0/drivers/system/pinmux                                       \
    -I$(top_srcdir)/third_party/microchip/asf/sam0/drivers/system/power                                        \
    -I$(top_srcdir)/third_party/microchip/asf/sam0/drivers/system/power/power_sam_d_r_h                        \
    -I$(top_srcdir)/third_party/microchip/asf/sam0/drivers/system/reset                                        \
    -I$(top_srcdir)/third_party/microchip/asf/sam0/drivers/system/reset/reset_sam_d_r_h                        \
    -I$(top_srcdir)/third_party/microchip/asf/sam0/drivers/sercom                                              \
    -I$(top_srcdir)/third_party/microchip/asf/sam0/drivers/sercom/i2c                                          \
    -I$(top_srcdir)/third_party/microchip/asf/sam0/drivers/sercom/i2c/i2c_sam0                                 \
    -I$(top_srcdir)/third_party/microchip/asf/sam0/drivers/sercom/spi                                          \
    -I$(top_srcdir)/third_party/microchip/asf/sam0/drivers/sercom/usart                                        \
    -I$(top_srcdir)/third_party/microchip/asf/sam0/utils                                                       \
    -I$(top_srcdir)/third_party/microchip/asf/sam0/utils/cmsis/samr21/include                                  \
    -I$(top_srcdir)/third_party/microchip/asf/sam0/utils/cmsis/samr21/source                                   \
    -I$(top_srcdir)/third_party/microchip/asf/sam0/utils/header_files                                          \
    -I$(top_srcdir)/third_party/microchip/asf/sam0/utils/preprocessor                                          \
    -I$(top_srcdir)/third_party/microchip/asf/thirdparty/CMSIS/Include                                         \
    -I$(top_srcdir)/third_party/microchip/asf/thirdparty/wireless/avr2130_lwmesh/source/phy/at86rf233/inc      \
    -I$(top_srcdir)/third_party/microchip/asf/thirdparty/wireless/services/sal/inc                             \
    -I$(top_srcdir)/third_party/microchip/asf/thirdparty/wireless/services/trx_access                          \
    -I$(top_srcdir)/third_party/microchip/asf/thirdparty/wireless/services/trx_access/module_config            \
    -Werror                                                                                                    \
    -Wno-unused-parameter                                                                                      \
    -Wno-implicit-function-declaration                                                                         \
    -Wno-expansion-to-defined                                                                                  \
    -fno-strict-aliasing                                                                                       \
    $(NULL)

PLATFORM_SOURCES                                                                                             = \
    alarm.c                                                                                                    \
    diag.c                                                                                                     \
    cxx_helpers.c                                                                                              \
    flash.c                                                                                                    \
    logging.c                                                                                                  \
    misc.c                                                                                                     \
    radio.c                                                                                                    \
    random.c                                                                                                   \
    system.c                                                                                                   \
    uart.c                                                                                                     \
    $(NULL)

libopenthread_samr21_a_SOURCES                                                                               = \
    $(PLATFORM_SOURCES)                                                                                        \
    $(NULL)

nodist_libopenthread_samr21_a_SOURCES                                                                        = \
    @top_builddir@/third_party/microchip/asf/common/utils/interrupt/interrupt_sam_nvic.c                       \
    @top_builddir@/third_party/microchip/asf/common2/services/delay/sam0/cycle_counter.c                       \
    @top_builddir@/third_party/microchip/asf/sam0/drivers/extint/extint_callback.c                             \
    @top_builddir@/third_party/microchip/asf/sam0/drivers/extint/extint_sam_d_r_h/extint.c                     \
    @top_builddir@/third_party/microchip/asf/sam0/drivers/nvm/nvm.c                                            \
    @top_builddir@/third_party/microchip/asf/sam0/drivers/port/port.c                                          \
    @top_builddir@/third_party/microchip/asf/sam0/drivers/system/clock/clock_samd21_r21_da_ha1/clock.c         \
    @top_builddir@/third_party/microchip/asf/sam0/drivers/system/clock/clock_samd21_r21_da_ha1/gclk.c          \
    @top_builddir@/third_party/microchip/asf/sam0/drivers/system/interrupt/system_interrupt.c                  \
    @top_builddir@/third_party/microchip/asf/sam0/drivers/system/pinmux/pinmux.c                               \
    @top_builddir@/third_party/microchip/asf/sam0/drivers/sercom/sercom.c                                      \
    @top_builddir@/third_party/microchip/asf/sam0/drivers/sercom/sercom_interrupt.c                            \
    @top_builddir@/third_party/microchip/asf/sam0/drivers/sercom/i2c/i2c_sam0/i2c_master.c                     \
    @top_builddir@/third_party/microchip/asf/sam0/drivers/sercom/spi/spi.c                                     \
    @top_builddir@/third_party/microchip/asf/sam0/drivers/sercom/usart/usart.c                                 \
    @top_builddir@/third_party/microchip/asf/sam0/drivers/sercom/usart/usart_interrupt.c                       \
    @top_builddir@/third_party/microchip/asf/sam0/utils/cmsis/samr21/source/gcc/startup_samr21.c               \
    @top_builddir@/third_party/microchip/asf/sam0/utils/cmsis/samr21/source/system_samr21.c                    \
    @top_builddir@/third_party/microchip/asf/sam0/utils/syscalls/gcc/syscalls.c                                \
    @top_builddir@/third_party/microchip/asf/thirdparty/wireless/avr2130_lwmesh/source/phy/at86rf233/src/phy.c \
    @top_builddir@/third_party/microchip/asf/thirdparty/wireless/services/sal/at86rf2xx/src/sal.c              \
    @top_builddir@/third_party/microchip/asf/thirdparty/wireless/services/trx_access/trx_access.c              \
    $(NULL)

noinst_HEADERS                                                                                               = \
    $(NULL)

PRETTY_FILES                                                                                                 = \
    $(PLATFORM_SOURCES)                                                                                        \
    $(NULL)

Dash                                                                                                         = -
libopenthread_samr21_a_LIBADD                                                                                = \
    $(shell find $(top_builddir)/examples/platforms/utils $(Dash)type f $(Dash)name "*.o")

include $(abs_top_nlbuild_autotools_dir)/automake/post.am
