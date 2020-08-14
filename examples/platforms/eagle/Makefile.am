#
#  Copyright (c) 2016, The OpenThread Authors.
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

lib_LIBRARIES                             = libopenthread-eagle.a

libopenthread_eagle_a_CPPFLAGS           = \
    -I$(top_srcdir)/include                 \
    -I$(top_srcdir)/examples/platforms      \
    -I$(top_srcdir)/src/core                \
    $(NULL)

PLATFORM_SOURCES                          = \
    alarm.c                                 \
    entropy.c                               \
    flash_wrap.c                                 \
    misc.c                                  \
    radio.c                                 \
    system.c                                \
    logging.c                               \
    uart_wrap.c                                  \
	irq_handler.c            \
	openthread-core-eagle-config-check.h    \
	openthread-core-eagle-config.h          \
	bootstrap/core_v5.h                           \
	platform-eagle.h                        \
drivers/analog.h                     \
drivers/core.h                        \
drivers/dfifo.h \
drivers/compiler.h                   \
drivers/dma.h                        \
drivers/driver_9518.h                \
drivers/flash.h                      \
drivers/gpio.h                       \
drivers/mspi.h                       \
drivers/plic.h                         \
drivers/pwm.h                         \
common/assert.h \
common/bit.h \
common/common.h \
common/fifo.h \
common/string.h \
common/printf.h \
drivers/spi.h                       \
drivers/sys.h                      \
drivers/i2c.h                   \
drivers/timer.h                      \
drivers/uart.h                      \
drivers/watchdog.h                      \
drivers/rf.h                         \
common/types.h                       \
drivers/putchar.h                      \
drivers/trng.h       \
drivers/stimer.h \
drivers/reg_include/analog_reg.h           \
drivers/reg_include/dma_reg.h              \
drivers/reg_include/fifo_reg.h             \
drivers/reg_include/gpio_reg.h             \
drivers/reg_include/hspi_reg.h             \
drivers/reg_include/i2c_reg.h              \
drivers/reg_include/mspi_reg.h             \
drivers/reg_include/plic_reg.h             \
drivers/reg_include/pspi_reg.h             \
drivers/reg_include/pwm_reg.h              \
drivers/reg_include/register_9518.h        \
drivers/reg_include/rf_reg.h               \
drivers/reg_include/soc.h                  \
drivers/reg_include/spi_reg.h              \
drivers/reg_include/timer_reg.h            \
drivers/reg_include/trng_reg.h             \
drivers/reg_include/uart_reg.h             \
drivers/reg_include/stimer_reg.h \
    $(NULL)

libopenthread_eagle_a_SOURCES            = \
    $(PLATFORM_SOURCES)                     \
    $(NULL)

PRETTY_FILES                             = \
    $(PLATFORM_SOURCES)                    \
    $(NULL)

Dash                                      = -
libopenthread_eagle_a_LIBADD             = \
    $(shell find $(top_builddir)/examples/platforms/utils $(Dash)type f $(Dash)name "*.o")

include $(abs_top_nlbuild_autotools_dir)/automake/post.am
