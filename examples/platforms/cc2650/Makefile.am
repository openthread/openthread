#
#  Copyright (c) 2016, Texas Instruments Incorporated
#  All rights reserved.
#
#  Redistribution and use in source and binary forms, with or without
#  modification, are permitted provided that the following conditions are met:
#
#  1) Redistributions of source code must retain the above copyright notice,
#     this list of conditions and the following disclaimer.
#
#  2) Redistributions in binary form must reproduce the above copyright notice,
#     this list of conditions and the following disclaimer in the documentation
#     and/or other materials provided with the distribution.
#
#  3) Neither the name of the ORGANIZATION nor the names of its contributors may
#     be used to endorse or promote products derived from this software without
#     specific prior written permission.
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

lib_LIBRARIES                       = libopenthread-cc2650.a

libopenthread_cc2650_a_CPPFLAGS                       = \
    -I$(top_srcdir)/include                             \
    -I$(top_srcdir)/src                                 \
    -I$(top_srcdir)/src/core                            \
    -I$(top_srcdir)/examples/platforms                  \
    -I$(top_srcdir)/examples/platforms/cc2650           \
    -I$(top_srcdir)/examples/platforms/cc2650/cc26xxware \
    -I$(top_srcdir)/examples/platforms/cc2650/cc26xxware/inc \
    -I$(top_srcdir)/examples/platforms/cc2650/cc26xxware/driverlib \
    -std=c99                                            \
    $(NULL)

libopenthread_cc2650_a_SOURCES                        = \
    cc26xxware/driverlib/adi.c            \
    cc26xxware/driverlib/aon_batmon.c     \
    cc26xxware/driverlib/aon_event.c      \
    cc26xxware/driverlib/aon_ioc.c        \
    cc26xxware/driverlib/aon_rtc.c        \
    cc26xxware/driverlib/aon_wuc.c        \
    cc26xxware/driverlib/aux_adc.c        \
    cc26xxware/driverlib/aux_smph.c       \
    cc26xxware/driverlib/aux_tdc.c        \
    cc26xxware/driverlib/aux_timer.c      \
    cc26xxware/driverlib/aux_wuc.c        \
    cc26xxware/driverlib/ccfgread.c       \
    cc26xxware/driverlib/chipinfo.c       \
    cc26xxware/driverlib/cpu.c            \
    cc26xxware/driverlib/crypto.c         \
    cc26xxware/driverlib/ddi.c            \
    cc26xxware/driverlib/debug.c          \
    cc26xxware/driverlib/driverlib_release.c \
    cc26xxware/driverlib/event.c          \
    cc26xxware/driverlib/flash.c          \
    cc26xxware/driverlib/gpio.c           \
    cc26xxware/driverlib/i2c.c            \
    cc26xxware/driverlib/i2s.c            \
    cc26xxware/driverlib/interrupt.c      \
    cc26xxware/driverlib/ioc.c            \
    cc26xxware/driverlib/osc.c            \
    cc26xxware/driverlib/prcm.c           \
    cc26xxware/driverlib/pwr_ctrl.c       \
    cc26xxware/driverlib/rfc.c            \
    cc26xxware/driverlib/rom_crypto.c     \
    cc26xxware/driverlib/setup.c          \
    cc26xxware/driverlib/smph.c           \
    cc26xxware/driverlib/ssi.c            \
    cc26xxware/driverlib/sys_ctrl.c       \
    cc26xxware/driverlib/systick.c        \
    cc26xxware/driverlib/timer.c          \
    cc26xxware/driverlib/trng.c           \
    cc26xxware/driverlib/uart.c           \
    cc26xxware/driverlib/udma.c           \
    cc26xxware/driverlib/vims.c           \
    cc26xxware/driverlib/watchdog.c       \
    alarm.c                               \
    logging.c                             \
    platform.c                            \
    radio.c                               \
    random.c                              \
    uart.c                                \
    misc.c                                \
    $(NULL)

if OPENTHREAD_ENABLE_DIAG
libopenthread_cc2650_a_SOURCES           += \
    diag.c                                  \
    $(NULL)
endif

include_HEADERS                                       = \
    cc26xxware/driverlib/adi.h            \
    cc26xxware/driverlib/aon_batmon.h     \
    cc26xxware/driverlib/aon_event.h      \
    cc26xxware/driverlib/aon_ioc.h        \
    cc26xxware/driverlib/aon_rtc.h        \
    cc26xxware/driverlib/aon_wuc.h        \
    cc26xxware/driverlib/aux_adc.h        \
    cc26xxware/driverlib/aux_smph.h       \
    cc26xxware/driverlib/aux_tdc.h        \
    cc26xxware/driverlib/aux_timer.h      \
    cc26xxware/driverlib/aux_wuc.h        \
    cc26xxware/driverlib/ccfgread.h       \
    cc26xxware/driverlib/chipinfo.h       \
    cc26xxware/driverlib/cpu.h            \
    cc26xxware/driverlib/crypto.h         \
    cc26xxware/driverlib/ddi.h            \
    cc26xxware/driverlib/debug.h          \
    cc26xxware/driverlib/driverlib_release.h \
    cc26xxware/driverlib/event.h          \
    cc26xxware/driverlib/flash.h          \
    cc26xxware/driverlib/gpio.h           \
    cc26xxware/driverlib/i2c.h            \
    cc26xxware/driverlib/i2s.h            \
    cc26xxware/driverlib/interrupt.h      \
    cc26xxware/driverlib/ioc.h            \
    cc26xxware/driverlib/osc.h            \
    cc26xxware/driverlib/prcm.h           \
    cc26xxware/driverlib/pwr_ctrl.h       \
    cc26xxware/driverlib/rf_ble_cmd.h     \
    cc26xxware/driverlib/rf_ble_mailbox.h \
    cc26xxware/driverlib/rf_common_cmd.h  \
    cc26xxware/driverlib/rf_data_entry.h  \
    cc26xxware/driverlib/rf_mailbox.h     \
    cc26xxware/driverlib/rf_prop_cmd.h    \
    cc26xxware/driverlib/rf_prop_mailbox.h \
    cc26xxware/driverlib/rfc.h            \
    cc26xxware/driverlib/rom.h            \
    cc26xxware/driverlib/rom_crypto.h     \
    cc26xxware/driverlib/setup.h          \
    cc26xxware/driverlib/smph.h           \
    cc26xxware/driverlib/ssi.h            \
    cc26xxware/driverlib/sys_ctrl.h       \
    cc26xxware/driverlib/systick.h        \
    cc26xxware/driverlib/timer.h          \
    cc26xxware/driverlib/trng.h           \
    cc26xxware/driverlib/uart.h           \
    cc26xxware/driverlib/udma.h           \
    cc26xxware/driverlib/vims.h           \
    cc26xxware/driverlib/watchdog.h       \
    $(NULL)

driverlibincdir = $(includedir)/inc
driverlibinc_HEADERS = \
    cc26xxware/inc/asmdefs.h              \
    cc26xxware/inc/hw_adi.h               \
    cc26xxware/inc/hw_adi_0_rf.h          \
    cc26xxware/inc/hw_adi_1_synth.h       \
    cc26xxware/inc/hw_adi_2_refsys.h      \
    cc26xxware/inc/hw_adi_3_refsys.h      \
    cc26xxware/inc/hw_adi_4_aux.h         \
    cc26xxware/inc/hw_aon_batmon.h        \
    cc26xxware/inc/hw_aon_event.h         \
    cc26xxware/inc/hw_aon_ioc.h           \
    cc26xxware/inc/hw_aon_rtc.h           \
    cc26xxware/inc/hw_aon_sysctl.h        \
    cc26xxware/inc/hw_aon_wuc.h           \
    cc26xxware/inc/hw_aux_aiodio.h        \
    cc26xxware/inc/hw_aux_anaif.h         \
    cc26xxware/inc/hw_aux_evctl.h         \
    cc26xxware/inc/hw_aux_sce.h           \
    cc26xxware/inc/hw_aux_smph.h          \
    cc26xxware/inc/hw_aux_tdc.h           \
    cc26xxware/inc/hw_aux_timer.h         \
    cc26xxware/inc/hw_aux_wuc.h           \
    cc26xxware/inc/hw_ccfg.h              \
    cc26xxware/inc/hw_ccfg_simple_struct.h \
    cc26xxware/inc/hw_chip_def.h          \
    cc26xxware/inc/hw_cpu_dwt.h           \
    cc26xxware/inc/hw_cpu_fpb.h           \
    cc26xxware/inc/hw_cpu_itm.h           \
    cc26xxware/inc/hw_cpu_rom_table.h     \
    cc26xxware/inc/hw_cpu_scs.h           \
    cc26xxware/inc/hw_cpu_tiprop.h        \
    cc26xxware/inc/hw_cpu_tpiu.h          \
    cc26xxware/inc/hw_crypto.h            \
    cc26xxware/inc/hw_ddi.h               \
    cc26xxware/inc/hw_ddi_0_osc.h         \
    cc26xxware/inc/hw_device.h            \
    cc26xxware/inc/hw_event.h             \
    cc26xxware/inc/hw_fcfg1.h             \
    cc26xxware/inc/hw_flash.h             \
    cc26xxware/inc/hw_gpio.h              \
    cc26xxware/inc/hw_gpt.h               \
    cc26xxware/inc/hw_i2c.h               \
    cc26xxware/inc/hw_i2s.h               \
    cc26xxware/inc/hw_ints.h              \
    cc26xxware/inc/hw_ioc.h               \
    cc26xxware/inc/hw_memmap.h            \
    cc26xxware/inc/hw_nvic.h              \
    cc26xxware/inc/hw_prcm.h              \
    cc26xxware/inc/hw_rfc_dbell.h         \
    cc26xxware/inc/hw_rfc_pwr.h           \
    cc26xxware/inc/hw_rfc_rat.h           \
    cc26xxware/inc/hw_smph.h              \
    cc26xxware/inc/hw_ssi.h               \
    cc26xxware/inc/hw_sysctl.h            \
    cc26xxware/inc/hw_trng.h              \
    cc26xxware/inc/hw_types.h             \
    cc26xxware/inc/hw_uart.h              \
    cc26xxware/inc/hw_udma.h              \
    cc26xxware/inc/hw_vims.h              \
    cc26xxware/inc/hw_wdt.h               \
	$(NULL)

if OPENTHREAD_BUILD_COVERAGE
CLEANFILES                          = $(wildcard *.gcda *.gcno)
endif # OPENTHREAD_BUILD_COVERAGE

include $(abs_top_nlbuild_autotools_dir)/automake/post.am
