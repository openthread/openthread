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

EXTRA_DIST                                          = \
    stack/cordio_stack/ble-host/include               \
    stack/cordio_stack/platform/common/include        \
    stack/cordio_stack/wsf/common/include             \
    stack/cordio_stack/wsf/common/include/util        \
    stack/cordio_stack/ble-host/sources/hci/dual_chip \
    stack/mbed_os_adaptation                          \
    $(NULL)

lib_LIBRARIES                                 = libcordio.a

# Do not enable -Wconversion for cordio
override CFLAGS                              := $(filter-out -Wconversion,$(CFLAGS))
override CXXFLAGS                            := $(filter-out -Wconversion,$(CXXFLAGS))

# Enable -Wno-unused-parameter for cordio
CFLAGS                                       += -Wno-unused-parameter
CXXFLAGS                                     += -Wno-unused-parameter

CORDIO_SRCDIR                                = $(top_srcdir)/third_party/cordio/stack

libcordio_a_CPPFLAGS                                                  = \
    -I$(CORDIO_SRCDIR)/cordio_stack/ble-host/include                    \
    -I$(CORDIO_SRCDIR)/cordio_stack/ble-host/sources/hci/dual_chip      \
    -I$(CORDIO_SRCDIR)/cordio_stack/ble-host/sources/sec/commom         \
    -I$(CORDIO_SRCDIR)/cordio_stack/ble-host/sources/stack/att          \
    -I$(CORDIO_SRCDIR)/cordio_stack/ble-host/sources/stack/cfg          \
    -I$(CORDIO_SRCDIR)/cordio_stack/ble-host/sources/stack/dm           \
    -I$(CORDIO_SRCDIR)/cordio_stack/ble-host/sources/stack/hci          \
    -I$(CORDIO_SRCDIR)/cordio_stack/ble-host/sources/stack/l2c          \
    -I$(CORDIO_SRCDIR)/cordio_stack/ble-host/sources/stack/smp          \
    -I$(CORDIO_SRCDIR)/cordio_stack/platform/common/include             \
    -I$(CORDIO_SRCDIR)/cordio_stack/wsf/common/include                  \
    -I$(CORDIO_SRCDIR)/cordio_stack/wsf/common/include/util             \
    -I$(CORDIO_SRCDIR)/mbed_os_adaptation                               \
    -I$(CORDIO_SRCDIR)/cordio_stack/ble-host/sources/stack/cfg          \
    $(CORDIO_CPPFLAGS)                                                 \
    $(NULL)

libcordio_a_SOURCES                                                   = \
    stack/cordio_stack/ble-host/sources/hci/common/hci_core.c           \
    stack/cordio_stack/ble-host/sources/hci/dual_chip/hci_cmd.c         \
    stack/cordio_stack/ble-host/sources/hci/dual_chip/hci_cmd_ae.c      \
    stack/cordio_stack/ble-host/sources/hci/dual_chip/hci_cmd_phy.c     \
    stack/cordio_stack/ble-host/sources/hci/dual_chip/hci_core_ps.c     \
    stack/cordio_stack/ble-host/sources/hci/dual_chip/hci_evt.c         \
    stack/cordio_stack/ble-host/sources/hci/dual_chip/hci_vs_ae.c       \
    stack/cordio_stack/ble-host/sources/sec/common/sec_aes.c            \
    stack/cordio_stack/ble-host/sources/sec/common/sec_cmac.c           \
    stack/cordio_stack/ble-host/sources/sec/common/sec_ecc_hci.c        \
    stack/cordio_stack/ble-host/sources/sec/common/sec_main.c           \
    stack/cordio_stack/ble-host/sources/stack/att/att_main.c            \
    stack/cordio_stack/ble-host/sources/stack/att/att_uuid.c            \
    stack/cordio_stack/ble-host/sources/stack/att/attc_disc.c           \
    stack/cordio_stack/ble-host/sources/stack/att/attc_main.c           \
    stack/cordio_stack/ble-host/sources/stack/att/attc_proc.c           \
    stack/cordio_stack/ble-host/sources/stack/att/attc_read.c           \
    stack/cordio_stack/ble-host/sources/stack/att/attc_sign.c           \
    stack/cordio_stack/ble-host/sources/stack/att/attc_write.c          \
    stack/cordio_stack/ble-host/sources/stack/att/atts_ccc.c            \
    stack/cordio_stack/ble-host/sources/stack/att/atts_dyn.c            \
    stack/cordio_stack/ble-host/sources/stack/att/atts_ind.c            \
    stack/cordio_stack/ble-host/sources/stack/att/atts_main.c           \
    stack/cordio_stack/ble-host/sources/stack/att/atts_proc.c           \
    stack/cordio_stack/ble-host/sources/stack/att/atts_read.c           \
    stack/cordio_stack/ble-host/sources/stack/att/atts_sign.c           \
    stack/cordio_stack/ble-host/sources/stack/att/atts_write.c          \
    stack/cordio_stack/ble-host/sources/stack/cfg/cfg_stack.c           \
    stack/cordio_stack/ble-host/sources/stack/dm/dm_adv.c               \
    stack/cordio_stack/ble-host/sources/stack/dm/dm_adv_ae.c            \
    stack/cordio_stack/ble-host/sources/stack/dm/dm_adv_leg.c           \
    stack/cordio_stack/ble-host/sources/stack/dm/dm_conn.c              \
    stack/cordio_stack/ble-host/sources/stack/dm/dm_conn_master.c       \
    stack/cordio_stack/ble-host/sources/stack/dm/dm_conn_master_ae.c    \
    stack/cordio_stack/ble-host/sources/stack/dm/dm_conn_master_leg.c   \
    stack/cordio_stack/ble-host/sources/stack/dm/dm_conn_slave.c        \
    stack/cordio_stack/ble-host/sources/stack/dm/dm_conn_slave_ae.c     \
    stack/cordio_stack/ble-host/sources/stack/dm/dm_conn_slave_leg.c    \
    stack/cordio_stack/ble-host/sources/stack/dm/dm_conn_sm.c           \
    stack/cordio_stack/ble-host/sources/stack/dm/dm_dev.c               \
    stack/cordio_stack/ble-host/sources/stack/dm/dm_dev_priv.c          \
    stack/cordio_stack/ble-host/sources/stack/dm/dm_main.c              \
    stack/cordio_stack/ble-host/sources/stack/dm/dm_phy.c               \
    stack/cordio_stack/ble-host/sources/stack/dm/dm_priv.c              \
    stack/cordio_stack/ble-host/sources/stack/dm/dm_scan.c              \
    stack/cordio_stack/ble-host/sources/stack/dm/dm_scan_ae.c           \
    stack/cordio_stack/ble-host/sources/stack/dm/dm_scan_leg.c          \
    stack/cordio_stack/ble-host/sources/stack/dm/dm_sec.c               \
    stack/cordio_stack/ble-host/sources/stack/dm/dm_sec_lesc.c          \
    stack/cordio_stack/ble-host/sources/stack/dm/dm_sec_master.c        \
    stack/cordio_stack/ble-host/sources/stack/dm/dm_sec_slave.c         \
    stack/cordio_stack/ble-host/sources/stack/dm/dm_sync_ae.c           \
    stack/cordio_stack/ble-host/sources/stack/hci/hci_main.c            \
    stack/cordio_stack/ble-host/sources/stack/l2c/l2c_coc.c             \
    stack/cordio_stack/ble-host/sources/stack/l2c/l2c_main.c            \
    stack/cordio_stack/ble-host/sources/stack/l2c/l2c_master.c          \
    stack/cordio_stack/ble-host/sources/stack/l2c/l2c_slave.c           \
    stack/cordio_stack/ble-host/sources/stack/smp/smp_act.c             \
    stack/cordio_stack/ble-host/sources/stack/smp/smp_db.c              \
    stack/cordio_stack/ble-host/sources/stack/smp/smp_main.c            \
    stack/cordio_stack/ble-host/sources/stack/smp/smp_non.c             \
    stack/cordio_stack/ble-host/sources/stack/smp/smp_sc_act.c          \
    stack/cordio_stack/ble-host/sources/stack/smp/smp_sc_main.c         \
    stack/cordio_stack/ble-host/sources/stack/smp/smpi_act.c            \
    stack/cordio_stack/ble-host/sources/stack/smp/smpi_sc_act.c         \
    stack/cordio_stack/ble-host/sources/stack/smp/smpi_sc_sm.c          \
    stack/cordio_stack/ble-host/sources/stack/smp/smpi_sm.c             \
    stack/cordio_stack/ble-host/sources/stack/smp/smpr_act.c            \
    stack/cordio_stack/ble-host/sources/stack/smp/smpr_sc_act.c         \
    stack/cordio_stack/ble-host/sources/stack/smp/smpr_sc_sm.c          \
    stack/cordio_stack/ble-host/sources/stack/smp/smpr_sm.c             \
    stack/cordio_stack/wsf/baremetal/sources/wsf_assert.c               \
    stack/cordio_stack/wsf/baremetal/sources/wsf_buf.c                  \
    stack/cordio_stack/wsf/baremetal/sources/wsf_efs.c                  \
    stack/cordio_stack/wsf/baremetal/sources/wsf_msg.c                  \
    stack/cordio_stack/wsf/baremetal/sources/wsf_queue.c                \
    stack/cordio_stack/wsf/baremetal/sources/wsf_timer.c                \
    stack/cordio_stack/wsf/baremetal/sources/wsf_trace.c                \
    stack/cordio_stack/wsf/common/sources/util/bda.c                    \
    stack/cordio_stack/wsf/common/sources/util/bstream.c                \
    stack/cordio_stack/wsf/common/sources/util/calc128.c                \
    stack/cordio_stack/wsf/common/sources/util/crc32.c                  \
    stack/cordio_stack/wsf/common/sources/util/print.c                  \
    stack/cordio_stack/wsf/common/sources/util/terminal.c               \
    stack/cordio_stack/wsf/common/sources/util/wstr.c                   \
    stack/mbed_os_adaptation/hci_drv.c                                  \
    stack/mbed_os_adaptation/hci_tr.c                                   \
    stack/mbed_os_adaptation/hci_vs.c                                   \
    stack/mbed_os_adaptation/wsf_cs.c                                   \
    stack/mbed_os_adaptation/wsf_os.c                                   \
    $(NULL)

include $(abs_top_nlbuild_autotools_dir)/automake/post.am
