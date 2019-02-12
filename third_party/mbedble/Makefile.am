#
#    Copyright 2019 The OpenThread Authors. All Rights Reserved.
#
#    Licensed under the Apache License, Version 2.0 (the "License");
#    you may not use this file except in compliance with the License.
#    You may obtain a copy of the License at
#
#    http://www.apache.org/licenses/LICENSE-2.0
#
#    Unless required by applicable law or agreed to in writing, software
#    distributed under the License is distributed on an "AS IS" BASIS,
#    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#    See the License for the specific language governing permissions and
#    limitations under the License.
#

include $(abs_top_nlbuild_autotools_dir)/automake/pre.am

EXTRA_DIST                                    = \
    cordio/cordio_stack/ble-host/include        \
    cordio/cordio_stack/platform/common/include \
    cordio/cordio_stack/wsf/common/include      \
    cordio/cordio_stack/wsf/common/include/util \
    cordio/cordio_stack/ble-host/sources/hci/dual_chip      \
    cordio/mbed_os_adaptation                   \
    $(NULL)

lib_LIBRARIES                                 = libmbedble.a

# Do not enable -Wconversion for mbedble
override CFLAGS                              := $(filter-out -Wconversion,$(CFLAGS))
override CXXFLAGS                            := $(filter-out -Wconversion,$(CXXFLAGS))

# Enable -Wno-unused-parameter for mbedble
CFLAGS                                       += -Wno-unused-parameter
CXXFLAGS                                     += -Wno-unused-parameter

MBEDBLE_SRCDIR                                = $(top_srcdir)/third_party/mbedble/cordio

libmbedble_a_CPPFLAGS                                                  = \
    -I$(MBEDBLE_SRCDIR)/cordio_stack/ble-host/include                    \
    -I$(MBEDBLE_SRCDIR)/cordio_stack/ble-host/sources/hci/dual_chip      \
    -I$(MBEDBLE_SRCDIR)/cordio_stack/ble-host/sources/sec/commom         \
    -I$(MBEDBLE_SRCDIR)/cordio_stack/ble-host/sources/stack/att          \
    -I$(MBEDBLE_SRCDIR)/cordio_stack/ble-host/sources/stack/cfg          \
    -I$(MBEDBLE_SRCDIR)/cordio_stack/ble-host/sources/stack/dm           \
    -I$(MBEDBLE_SRCDIR)/cordio_stack/ble-host/sources/stack/hci          \
    -I$(MBEDBLE_SRCDIR)/cordio_stack/ble-host/sources/stack/l2c          \
    -I$(MBEDBLE_SRCDIR)/cordio_stack/ble-host/sources/stack/smp          \
    -I$(MBEDBLE_SRCDIR)/cordio_stack/platform/common/include             \
    -I$(MBEDBLE_SRCDIR)/cordio_stack/wsf/common/include                  \
    -I$(MBEDBLE_SRCDIR)/cordio_stack/wsf/common/include/util             \
    -I$(MBEDBLE_SRCDIR)/mbed_os_adaptation                               \
    -I$(MBEDBLE_SRCDIR)/cordio_stack/ble-host/sources/stack/cfg          \
    $(MBEDBLE_CPPFLAGS)                                                  \
    $(NULL)

libmbedble_a_SOURCES                                                   = \
    cordio/cordio_stack/ble-host/sources/hci/common/hci_core.c           \
    cordio/cordio_stack/ble-host/sources/hci/dual_chip/hci_cmd.c         \
    cordio/cordio_stack/ble-host/sources/hci/dual_chip/hci_cmd_ae.c      \
    cordio/cordio_stack/ble-host/sources/hci/dual_chip/hci_cmd_phy.c     \
    cordio/cordio_stack/ble-host/sources/hci/dual_chip/hci_core_ps.c     \
    cordio/cordio_stack/ble-host/sources/hci/dual_chip/hci_evt.c         \
    cordio/cordio_stack/ble-host/sources/hci/dual_chip/hci_vs_ae.c       \
    cordio/cordio_stack/ble-host/sources/sec/common/sec_aes.c            \
    cordio/cordio_stack/ble-host/sources/sec/common/sec_cmac.c           \
    cordio/cordio_stack/ble-host/sources/sec/common/sec_ecc_hci.c        \
    cordio/cordio_stack/ble-host/sources/sec/common/sec_main.c           \
    cordio/cordio_stack/ble-host/sources/stack/att/att_main.c            \
    cordio/cordio_stack/ble-host/sources/stack/att/att_uuid.c            \
    cordio/cordio_stack/ble-host/sources/stack/att/attc_disc.c           \
    cordio/cordio_stack/ble-host/sources/stack/att/attc_main.c           \
    cordio/cordio_stack/ble-host/sources/stack/att/attc_proc.c           \
    cordio/cordio_stack/ble-host/sources/stack/att/attc_read.c           \
    cordio/cordio_stack/ble-host/sources/stack/att/attc_sign.c           \
    cordio/cordio_stack/ble-host/sources/stack/att/attc_write.c          \
    cordio/cordio_stack/ble-host/sources/stack/att/atts_ccc.c            \
    cordio/cordio_stack/ble-host/sources/stack/att/atts_dyn.c            \
    cordio/cordio_stack/ble-host/sources/stack/att/atts_ind.c            \
    cordio/cordio_stack/ble-host/sources/stack/att/atts_main.c           \
    cordio/cordio_stack/ble-host/sources/stack/att/atts_proc.c           \
    cordio/cordio_stack/ble-host/sources/stack/att/atts_read.c           \
    cordio/cordio_stack/ble-host/sources/stack/att/atts_sign.c           \
    cordio/cordio_stack/ble-host/sources/stack/att/atts_write.c          \
    cordio/cordio_stack/ble-host/sources/stack/cfg/cfg_stack.c           \
    cordio/cordio_stack/ble-host/sources/stack/dm/dm_adv.c               \
    cordio/cordio_stack/ble-host/sources/stack/dm/dm_adv_ae.c            \
    cordio/cordio_stack/ble-host/sources/stack/dm/dm_adv_leg.c           \
    cordio/cordio_stack/ble-host/sources/stack/dm/dm_conn.c              \
    cordio/cordio_stack/ble-host/sources/stack/dm/dm_conn_master.c       \
    cordio/cordio_stack/ble-host/sources/stack/dm/dm_conn_master_ae.c    \
    cordio/cordio_stack/ble-host/sources/stack/dm/dm_conn_master_leg.c   \
    cordio/cordio_stack/ble-host/sources/stack/dm/dm_conn_slave.c        \
    cordio/cordio_stack/ble-host/sources/stack/dm/dm_conn_slave_ae.c     \
    cordio/cordio_stack/ble-host/sources/stack/dm/dm_conn_slave_leg.c    \
    cordio/cordio_stack/ble-host/sources/stack/dm/dm_conn_sm.c           \
    cordio/cordio_stack/ble-host/sources/stack/dm/dm_dev.c               \
    cordio/cordio_stack/ble-host/sources/stack/dm/dm_dev_priv.c          \
    cordio/cordio_stack/ble-host/sources/stack/dm/dm_main.c              \
    cordio/cordio_stack/ble-host/sources/stack/dm/dm_phy.c               \
    cordio/cordio_stack/ble-host/sources/stack/dm/dm_priv.c              \
    cordio/cordio_stack/ble-host/sources/stack/dm/dm_scan.c              \
    cordio/cordio_stack/ble-host/sources/stack/dm/dm_scan_ae.c           \
    cordio/cordio_stack/ble-host/sources/stack/dm/dm_scan_leg.c          \
    cordio/cordio_stack/ble-host/sources/stack/dm/dm_sec.c               \
    cordio/cordio_stack/ble-host/sources/stack/dm/dm_sec_lesc.c          \
    cordio/cordio_stack/ble-host/sources/stack/dm/dm_sec_master.c        \
    cordio/cordio_stack/ble-host/sources/stack/dm/dm_sec_slave.c         \
    cordio/cordio_stack/ble-host/sources/stack/dm/dm_sync_ae.c           \
    cordio/cordio_stack/ble-host/sources/stack/hci/hci_main.c            \
    cordio/cordio_stack/ble-host/sources/stack/l2c/l2c_coc.c             \
    cordio/cordio_stack/ble-host/sources/stack/l2c/l2c_main.c            \
    cordio/cordio_stack/ble-host/sources/stack/l2c/l2c_master.c          \
    cordio/cordio_stack/ble-host/sources/stack/l2c/l2c_slave.c           \
    cordio/cordio_stack/ble-host/sources/stack/smp/smp_act.c             \
    cordio/cordio_stack/ble-host/sources/stack/smp/smp_db.c              \
    cordio/cordio_stack/ble-host/sources/stack/smp/smp_main.c            \
    cordio/cordio_stack/ble-host/sources/stack/smp/smp_non.c             \
    cordio/cordio_stack/ble-host/sources/stack/smp/smp_sc_act.c          \
    cordio/cordio_stack/ble-host/sources/stack/smp/smp_sc_main.c         \
    cordio/cordio_stack/ble-host/sources/stack/smp/smpi_act.c            \
    cordio/cordio_stack/ble-host/sources/stack/smp/smpi_sc_act.c         \
    cordio/cordio_stack/ble-host/sources/stack/smp/smpi_sc_sm.c          \
    cordio/cordio_stack/ble-host/sources/stack/smp/smpi_sm.c             \
    cordio/cordio_stack/ble-host/sources/stack/smp/smpr_act.c            \
    cordio/cordio_stack/ble-host/sources/stack/smp/smpr_sc_act.c         \
    cordio/cordio_stack/ble-host/sources/stack/smp/smpr_sc_sm.c          \
    cordio/cordio_stack/ble-host/sources/stack/smp/smpr_sm.c             \
    cordio/cordio_stack/wsf/baremetal/sources/wsf_assert.c               \
    cordio/cordio_stack/wsf/baremetal/sources/wsf_buf.c                  \
    cordio/cordio_stack/wsf/baremetal/sources/wsf_efs.c                  \
    cordio/cordio_stack/wsf/baremetal/sources/wsf_msg.c                  \
    cordio/cordio_stack/wsf/baremetal/sources/wsf_queue.c                \
    cordio/cordio_stack/wsf/baremetal/sources/wsf_timer.c                \
    cordio/cordio_stack/wsf/baremetal/sources/wsf_trace.c                \
    cordio/cordio_stack/wsf/common/sources/util/bda.c                    \
    cordio/cordio_stack/wsf/common/sources/util/bstream.c                \
    cordio/cordio_stack/wsf/common/sources/util/calc128.c                \
    cordio/cordio_stack/wsf/common/sources/util/crc32.c                  \
    cordio/cordio_stack/wsf/common/sources/util/print.c                  \
    cordio/cordio_stack/wsf/common/sources/util/terminal.c               \
    cordio/cordio_stack/wsf/common/sources/util/wstr.c                   \
    cordio/mbed_os_adaptation/hci_drv.c                                  \
    cordio/mbed_os_adaptation/hci_tr.c                                   \
    cordio/mbed_os_adaptation/hci_vs.c                                   \
    cordio/mbed_os_adaptation/wsf_cs.c                                   \
    cordio/mbed_os_adaptation/wsf_os.c                                   \
    $(NULL)

if OPENTHREAD_BUILD_COVERAGE
Dash                                          = -
CLEANFILES                                    = $(shell find $(top_builddir)/third_party/mbedble $(Dash)name "*.gcda" $(Dash)o $(Dash)name "*.gcno")
endif # OPENTHREAD_BUILD_COVERAGE

include $(abs_top_nlbuild_autotools_dir)/automake/post.am
