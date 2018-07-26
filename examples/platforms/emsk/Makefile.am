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

lib_LIBRARIES                                            = libopenthread-emsk.a

# Do not enable -Wconversion for emsk
override CFLAGS                                         := $(filter-out -Wconversion,$(CFLAGS))
override CXXFLAGS                                       := $(filter-out -Wconversion,$(CXXFLAGS))

# Do not enable -pedantic-errors for emsk
override CFLAGS                                         := $(filter-out -pedantic-errors,$(CFLAGS))
override CXXFLAGS                                       := $(filter-out -pedantic-errors,$(CXXFLAGS))

# Do not enable -Werror for emsk
override CFLAGS                                         := $(filter-out -Werror,$(CFLAGS))
override CXXFLAGS                                       := $(filter-out -Werror,$(CXXFLAGS))

# Do not enable -Wformat for emsk
override CFLAGS                                         := $(filter-out -Wformat,$(CFLAGS))
override CXXFLAGS                                       := $(filter-out -Wformat,$(CXXFLAGS))

COMMONCPPFLAGS                                                                                    = \
    -I$(top_srcdir)/include                                                                         \
    -I$(top_srcdir)/examples/platforms                                                              \
    -I$(top_srcdir)/src/core                                                                        \
    -I$(top_srcdir)/third_party/synopsys/embarc_emsk_bsp                                            \
    -I$(top_srcdir)/examples/platforms/emsk                                                         \
    -I$(top_srcdir)/third_party/synopsys/embarc_emsk_bsp/library/clib                               \
    -Wno-unknown-pragmas                                                                            \
    -Wno-unused-function                                                                            \
    -Wno-unused-parameter                                                                           \
    -Wno-unused-variable                                                                            \
    -Wno-implicit-function-declaration                                                              \
    -Wno-unused-variable                                                                            \
    -Wno-unused-but-set-variable                                                                    \
    -Wno-type-limits                                                                                \
    -Wno-sign-compare                                                                               \
    -Wno-unused-label                                                                               \
    $(NULL)

PLATFORM_SOURCES                                                                                  = \
    alarm.c                                                                                         \
    flash.c                                                                                         \
    logging.c                                                                                       \
    misc.c                                                                                          \
    radio.c                                                                                         \
    random.c                                                                                        \
    system.c                                                                                        \
    uart.c                                                                                          \
    platform-emsk.h                                                                                 \
    $(NULL)

SYNOPSYS_SOURCES                                                                                  = \
    @top_builddir@/third_party/synopsys/embarc_emsk_bsp/arc/startup/arc_cxx_support.c               \
    @top_builddir@/third_party/synopsys/embarc_emsk_bsp/arc/startup/arc_startup.S                   \
    @top_builddir@/third_party/synopsys/embarc_emsk_bsp/arc/arc_cache.c                             \
    @top_builddir@/third_party/synopsys/embarc_emsk_bsp/arc/arc_exc_asm.S                           \
    @top_builddir@/third_party/synopsys/embarc_emsk_bsp/arc/arc_exception.c                         \
    @top_builddir@/third_party/synopsys/embarc_emsk_bsp/arc/arc_timer.c                             \
    @top_builddir@/third_party/synopsys/embarc_emsk_bsp/board/emsk/common/emsk_init.c               \
    @top_builddir@/third_party/synopsys/embarc_emsk_bsp/board/emsk/common/emsk_timer.c              \
    @top_builddir@/third_party/synopsys/embarc_emsk_bsp/board/emsk/common/mux.c                     \
    @top_builddir@/third_party/synopsys/embarc_emsk_bsp/board/emsk/gpio/dw_gpio_obj.c               \
    @top_builddir@/third_party/synopsys/embarc_emsk_bsp/board/emsk/gpio/emsk_gpio.c                 \
    @top_builddir@/third_party/synopsys/embarc_emsk_bsp/board/emsk/pmrf/mrf24j40.c                  \
    @top_builddir@/third_party/synopsys/embarc_emsk_bsp/board/emsk/pmrf/pmrf.c                      \
    @top_builddir@/third_party/synopsys/embarc_emsk_bsp/board/emsk/spi/dw_spi_obj.c                 \
    @top_builddir@/third_party/synopsys/embarc_emsk_bsp/board/emsk/spiflash/spiflash.c              \
    @top_builddir@/third_party/synopsys/embarc_emsk_bsp/board/emsk/uart/dw_uart_obj.c               \
    @top_builddir@/third_party/synopsys/embarc_emsk_bsp/board/board.c                               \
    @top_builddir@/third_party/synopsys/embarc_emsk_bsp/common/xprintf/xprintf.c                    \
    @top_builddir@/third_party/synopsys/embarc_emsk_bsp/common/console_io.c                         \
    @top_builddir@/third_party/synopsys/embarc_emsk_bsp/device/designware/gpio/dw_gpio.c            \
    @top_builddir@/third_party/synopsys/embarc_emsk_bsp/device/designware/spi/dw_spi.c              \
    @top_builddir@/third_party/synopsys/embarc_emsk_bsp/device/designware/uart/dw_uart.c            \
    @top_builddir@/third_party/synopsys/embarc_emsk_bsp/library/clib/embARC_misc.c                  \
    @top_builddir@/third_party/synopsys/embarc_emsk_bsp/library/clib/embARC_sbrk.c                  \
    @top_builddir@/third_party/synopsys/embarc_emsk_bsp/library/clib/embARC_syscalls.c              \
    @top_builddir@/third_party/synopsys/embarc_emsk_bsp/library/clib/embARC_target.c                \
    @top_builddir@/third_party/synopsys/embarc_emsk_bsp/library/clib/fatfs_dirent.c                 \
    @top_builddir@/third_party/synopsys/embarc_emsk_bsp/library/clib/malloc.c                       \
    @top_builddir@/third_party/synopsys/embarc_emsk_bsp/library/clib/ya_getopt.c                    \
    $(NULL)

libopenthread_emsk_a_CPPFLAGS                                                                     = \
    $(COMMONCPPFLAGS)                                                                               \
    $(NULL)

libopenthread_emsk_a_SOURCES                                                                      = \
    $(PLATFORM_SOURCES)                                                                             \
    $(SYNOPSYS_SOURCES)                                                                             \
    $(NULL)

noinst_HEADERS                                                                                    = \
    $(top_srcdir)/third_party/synopsys/embarc_emsk_bsp/board/emsk/common/emsk_timer.h               \
    $(top_srcdir)/third_party/synopsys/embarc_emsk_bsp/board/emsk/common/mux.h                      \
    $(top_srcdir)/third_party/synopsys/embarc_emsk_bsp/board/emsk/common/mux_hal.h                  \
    $(top_srcdir)/third_party/synopsys/embarc_emsk_bsp/board/emsk/gpio/dw_gpio_obj.h                \
    $(top_srcdir)/third_party/synopsys/embarc_emsk_bsp/board/emsk/gpio/emsk_gpio.h                  \
    $(top_srcdir)/third_party/synopsys/embarc_emsk_bsp/board/emsk/pmrf/mrf24j40.h                   \
    $(top_srcdir)/third_party/synopsys/embarc_emsk_bsp/board/emsk/pmrf/pmrf.h                       \
    $(top_srcdir)/third_party/synopsys/embarc_emsk_bsp/board/emsk/spi/dw_spi_obj.h                  \
    $(top_srcdir)/third_party/synopsys/embarc_emsk_bsp/board/emsk/spiflash/spiflash.h               \
    $(top_srcdir)/third_party/synopsys/embarc_emsk_bsp/board/emsk/uart/dw_uart_obj.h                \
    $(top_srcdir)/third_party/synopsys/embarc_emsk_bsp/board/emsk/emsk.h                            \
    $(top_srcdir)/third_party/synopsys/embarc_emsk_bsp/board/emsk/emsk_hardware.h                   \
    $(top_srcdir)/third_party/synopsys/embarc_emsk_bsp/common/xprintf/xprintf.h                     \
    $(top_srcdir)/third_party/synopsys/embarc_emsk_bsp/common/console_io.h                          \
    $(top_srcdir)/third_party/synopsys/embarc_emsk_bsp/device/designware/gpio/dw_gpio.h             \
    $(top_srcdir)/third_party/synopsys/embarc_emsk_bsp/device/designware/spi/dw_spi.h               \
    $(top_srcdir)/third_party/synopsys/embarc_emsk_bsp/device/designware/spi/dw_spi_hal.h           \
    $(top_srcdir)/third_party/synopsys/embarc_emsk_bsp/device/designware/spi/dw_spi_hal_cfg.h       \
    $(top_srcdir)/third_party/synopsys/embarc_emsk_bsp/device/designware/uart/dw_uart.h             \
    $(top_srcdir)/third_party/synopsys/embarc_emsk_bsp/device/designware/uart/dw_uart_hal.h         \
    $(top_srcdir)/third_party/synopsys/embarc_emsk_bsp/device/device_hal/inc/dev_common.h           \
    $(top_srcdir)/third_party/synopsys/embarc_emsk_bsp/device/device_hal/inc/dev_gpio.h             \
    $(top_srcdir)/third_party/synopsys/embarc_emsk_bsp/device/device_hal/inc/dev_spi.h              \
    $(top_srcdir)/third_party/synopsys/embarc_emsk_bsp/device/device_hal/inc/dev_uart.h             \
    $(top_srcdir)/third_party/synopsys/embarc_emsk_bsp/inc/arc/arc.h                                \
    $(top_srcdir)/third_party/synopsys/embarc_emsk_bsp/inc/arc/arc_asm_common.h                     \
    $(top_srcdir)/third_party/synopsys/embarc_emsk_bsp/inc/arc/arc_builtin.h                        \
    $(top_srcdir)/third_party/synopsys/embarc_emsk_bsp/inc/arc/arc_cache.h                          \
    $(top_srcdir)/third_party/synopsys/embarc_emsk_bsp/inc/arc/arc_em.h                             \
    $(top_srcdir)/third_party/synopsys/embarc_emsk_bsp/inc/arc/arc_exception.h                      \
    $(top_srcdir)/third_party/synopsys/embarc_emsk_bsp/inc/arc/arc_feature_config.h                 \
    $(top_srcdir)/third_party/synopsys/embarc_emsk_bsp/inc/arc/arc_timer.h                          \
    $(top_srcdir)/third_party/synopsys/embarc_emsk_bsp/inc/embARC_error.h                           \
    $(top_srcdir)/third_party/synopsys/embarc_emsk_bsp/inc/embARC_toolchain.h                       \
    $(top_srcdir)/third_party/synopsys/embarc_emsk_bsp/arc_core_config.h                            \
    $(top_srcdir)/third_party/synopsys/embarc_emsk_bsp/embARC_BSP_config.h                          \
    $(top_srcdir)/third_party/synopsys/embarc_emsk_bsp/library/clib/dirent.h                        \
    $(top_srcdir)/third_party/synopsys/embarc_emsk_bsp/library/clib/embARC_syscalls.h               \
    $(top_srcdir)/third_party/synopsys/embarc_emsk_bsp/library/clib/embARC_target.h                 \
    $(top_srcdir)/third_party/synopsys/embarc_emsk_bsp/library/clib/fatfs_dirent.h                  \
    $(top_srcdir)/third_party/synopsys/embarc_emsk_bsp/library/clib/ya_getopt.h                     \
    $(NULL)

PRETTY_FILES                                                                                      = \
    $(PLATFORM_SOURCES)                                                                             \
    $(NULL)

Dash                                                                                              = -

libopenthread_emsk_a_LIBADD                                                                       = \
    $(shell find $(top_builddir)/examples/platforms/utils $(Dash)type f $(Dash)name "*.o")

include $(abs_top_nlbuild_autotools_dir)/automake/post.am
