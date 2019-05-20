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

# Always package (e.g. for 'make dist') these subdirectories.

DIST_SUBDIRS                            = \
    ./../../examples/platforms/ble/cordio \
    $(NULL)

# Always build (e.g. for 'make all') these subdirectories.

SUBDIRS                                 = \
    ./../../examples/platforms/ble/cordio \
    $(NULL)

# Always pretty (e.g. for 'make pretty') these subdirectories.

PRETTY_SUBDIRS                          = \
    ./../../examples/platforms/ble/cordio \
    $(NULL)

lib_LIBRARIES                                 = $(NULL)

if OPENTHREAD_ENABLE_BLE_HOST
lib_LIBRARIES                                += libcordio-ble-host.a
endif # OPENTHREAD_ENABLE_BLE_HOST

if OPENTHREAD_ENABLE_BLE_CONTROLLER
lib_LIBRARIES                                += libcordio-ble-controller.a
endif # OPENTHREAD_ENABLE_BLE_CONTROLLER

# Disable -Wconversion for cordio
override CFLAGS                              := $(filter-out -Wconversion,$(CFLAGS))
override CXXFLAGS                            := $(filter-out -Wconversion,$(CXXFLAGS))

# Disable -Wunused-parameter for cordio
CFLAGS                                       += -Wno-unused-parameter
CXXFLAGS                                     += -Wno-unused-parameter

# Disable -Wpedantic for cordio
CFLAGS                                       += -Wno-pedantic
CXXFLAGS                                     += -Wno-pedantic

# Disable -Wpointer-to-int-cast for cordio
CFLAGS                                       += -Wno-pointer-to-int-cast
CXXFLAGS                                     += -Wno-pointer-to-int-cast

# Disable -Wimplicit-fallthrough for cordio
CFLAGS                                       +=-Wno-implicit-fallthrough
CXXFLAGS                                     +=-Wno-implicit-fallthrough

if OPENTHREAD_ENABLE_BLE_HOST
CORDIO_BLE_HOST_SRCDIR                        = $(top_srcdir)/third_party/cordio/TARGET_CORDIO/

libcordio_ble_host_a_CPPFLAGS                                        = \
    -I$(top_srcdir)/include                                            \
    -I$(top_srcdir)/src/core                                           \
    -I$(top_srcdir)/examples/platforms/ble/cordio                      \
    -I$(CORDIO_BLE_HOST_SRCDIR)/                                       \
    -I$(CORDIO_BLE_HOST_SRCDIR)/stack/ble-host/include                 \
    -I$(CORDIO_BLE_HOST_SRCDIR)/stack/ble-host/sources/hci/dual_chip   \
    -I$(CORDIO_BLE_HOST_SRCDIR)/stack/ble-host/sources/sec/common      \
    -I$(CORDIO_BLE_HOST_SRCDIR)/stack/ble-host/sources/stack/att       \
    -I$(CORDIO_BLE_HOST_SRCDIR)/stack/ble-host/sources/stack/cfg       \
    -I$(CORDIO_BLE_HOST_SRCDIR)/stack/ble-host/sources/stack/dm        \
    -I$(CORDIO_BLE_HOST_SRCDIR)/stack/ble-host/sources/stack/hci       \
    -I$(CORDIO_BLE_HOST_SRCDIR)/stack/ble-host/sources/stack/l2c       \
    -I$(CORDIO_BLE_HOST_SRCDIR)/stack/ble-host/sources/stack/smp       \
    -I$(CORDIO_BLE_HOST_SRCDIR)/stack/platform/include                 \
    -I$(CORDIO_BLE_HOST_SRCDIR)/stack/wsf/include                      \
    -I$(CORDIO_BLE_HOST_SRCDIR)/stack/wsf/include/util                 \
    -I$(CORDIO_BLE_HOST_SRCDIR)/stack/wsf/source/port/baremetal        \
    -I$(CORDIO_BLE_HOST_SRCDIR)/stack/wsf/source/util                  \
    -I$(CORDIO_BLE_HOST_SRCDIR)/stack_adaptation                       \
    -include cordio-config.h                                           \
    $(NULL)

libcordio_ble_host_a_SOURCES                                         = \
    TARGET_CORDIO/stack/ble-host/sources/hci/common/hci_core.c         \
    TARGET_CORDIO/stack/ble-host/sources/hci/dual_chip/hci_cmd.c       \
    TARGET_CORDIO/stack/ble-host/sources/hci/dual_chip/hci_cmd_cte.c   \
    TARGET_CORDIO/stack/ble-host/sources/hci/dual_chip/hci_cmd_past.c  \
    TARGET_CORDIO/stack/ble-host/sources/hci/dual_chip/hci_cmd_phy.c   \
    TARGET_CORDIO/stack/ble-host/sources/hci/dual_chip/hci_core_ps.c   \
    TARGET_CORDIO/stack/ble-host/sources/hci/dual_chip/hci_evt.c       \
    TARGET_CORDIO/stack/ble-host/sources/sec/common/sec_aes.c          \
    TARGET_CORDIO/stack/ble-host/sources/sec/common/sec_ccm_hci.c      \
    TARGET_CORDIO/stack/ble-host/sources/sec/common/sec_cmac_hci.c     \
    TARGET_CORDIO/stack/ble-host/sources/sec/common/sec_ecc_hci.c      \
    TARGET_CORDIO/stack/ble-host/sources/sec/common/sec_main.c         \
    TARGET_CORDIO/stack/ble-host/sources/stack/att/att_main.c          \
    TARGET_CORDIO/stack/ble-host/sources/stack/att/att_uuid.c          \
    TARGET_CORDIO/stack/ble-host/sources/stack/att/attc_disc.c         \
    TARGET_CORDIO/stack/ble-host/sources/stack/att/attc_main.c         \
    TARGET_CORDIO/stack/ble-host/sources/stack/att/attc_proc.c         \
    TARGET_CORDIO/stack/ble-host/sources/stack/att/attc_read.c         \
    TARGET_CORDIO/stack/ble-host/sources/stack/att/attc_sign.c         \
    TARGET_CORDIO/stack/ble-host/sources/stack/att/attc_write.c        \
    TARGET_CORDIO/stack/ble-host/sources/stack/att/atts_ccc.c          \
    TARGET_CORDIO/stack/ble-host/sources/stack/att/atts_csf.c          \
    TARGET_CORDIO/stack/ble-host/sources/stack/att/atts_dyn.c          \
    TARGET_CORDIO/stack/ble-host/sources/stack/att/atts_ind.c          \
    TARGET_CORDIO/stack/ble-host/sources/stack/att/atts_main.c         \
    TARGET_CORDIO/stack/ble-host/sources/stack/att/atts_proc.c         \
    TARGET_CORDIO/stack/ble-host/sources/stack/att/atts_read.c         \
    TARGET_CORDIO/stack/ble-host/sources/stack/att/atts_sign.c         \
    TARGET_CORDIO/stack/ble-host/sources/stack/att/atts_write.c        \
    TARGET_CORDIO/stack/ble-host/sources/stack/cfg/cfg_stack.c         \
    TARGET_CORDIO/stack/ble-host/sources/stack/dm/dm_adv.c             \
    TARGET_CORDIO/stack/ble-host/sources/stack/dm/dm_adv_leg.c         \
    TARGET_CORDIO/stack/ble-host/sources/stack/dm/dm_conn.c            \
    TARGET_CORDIO/stack/ble-host/sources/stack/dm/dm_conn_cte.c        \
    TARGET_CORDIO/stack/ble-host/sources/stack/dm/dm_conn_master.c     \
    TARGET_CORDIO/stack/ble-host/sources/stack/dm/dm_conn_master_leg.c \
    TARGET_CORDIO/stack/ble-host/sources/stack/dm/dm_conn_slave.c      \
    TARGET_CORDIO/stack/ble-host/sources/stack/dm/dm_conn_slave_leg.c  \
    TARGET_CORDIO/stack/ble-host/sources/stack/dm/dm_conn_sm.c         \
    TARGET_CORDIO/stack/ble-host/sources/stack/dm/dm_dev.c             \
    TARGET_CORDIO/stack/ble-host/sources/stack/dm/dm_dev_priv.c        \
    TARGET_CORDIO/stack/ble-host/sources/stack/dm/dm_main.c            \
    TARGET_CORDIO/stack/ble-host/sources/stack/dm/dm_past.c            \
    TARGET_CORDIO/stack/ble-host/sources/stack/dm/dm_phy.c             \
    TARGET_CORDIO/stack/ble-host/sources/stack/dm/dm_priv.c            \
    TARGET_CORDIO/stack/ble-host/sources/stack/dm/dm_scan.c            \
    TARGET_CORDIO/stack/ble-host/sources/stack/dm/dm_scan_leg.c        \
    TARGET_CORDIO/stack/ble-host/sources/stack/dm/dm_sec.c             \
    TARGET_CORDIO/stack/ble-host/sources/stack/dm/dm_sec_lesc.c        \
    TARGET_CORDIO/stack/ble-host/sources/stack/dm/dm_sec_master.c      \
    TARGET_CORDIO/stack/ble-host/sources/stack/dm/dm_sec_slave.c       \
    TARGET_CORDIO/stack/ble-host/sources/stack/hci/hci_main.c          \
    TARGET_CORDIO/stack/ble-host/sources/stack/l2c/l2c_coc.c           \
    TARGET_CORDIO/stack/ble-host/sources/stack/l2c/l2c_main.c          \
    TARGET_CORDIO/stack/ble-host/sources/stack/l2c/l2c_master.c        \
    TARGET_CORDIO/stack/ble-host/sources/stack/l2c/l2c_slave.c         \
    TARGET_CORDIO/stack/ble-host/sources/stack/smp/smp_act.c           \
    TARGET_CORDIO/stack/ble-host/sources/stack/smp/smp_db.c            \
    TARGET_CORDIO/stack/ble-host/sources/stack/smp/smp_main.c          \
    TARGET_CORDIO/stack/ble-host/sources/stack/smp/smp_non.c           \
    TARGET_CORDIO/stack/ble-host/sources/stack/smp/smp_sc_act.c        \
    TARGET_CORDIO/stack/ble-host/sources/stack/smp/smp_sc_main.c       \
    TARGET_CORDIO/stack/ble-host/sources/stack/smp/smpi_act.c          \
    TARGET_CORDIO/stack/ble-host/sources/stack/smp/smpi_sc_act.c       \
    TARGET_CORDIO/stack/ble-host/sources/stack/smp/smpi_sc_sm.c        \
    TARGET_CORDIO/stack/ble-host/sources/stack/smp/smpi_sm.c           \
    TARGET_CORDIO/stack/ble-host/sources/stack/smp/smpr_act.c          \
    TARGET_CORDIO/stack/ble-host/sources/stack/smp/smpr_sc_act.c       \
    TARGET_CORDIO/stack/ble-host/sources/stack/smp/smpr_sc_sm.c        \
    TARGET_CORDIO/stack/ble-host/sources/stack/smp/smpr_sm.c           \
    TARGET_CORDIO/stack/wsf/sources/port/baremetal/wsf_assert.c        \
    TARGET_CORDIO/stack/wsf/sources/port/baremetal/wsf_buf.c           \
    TARGET_CORDIO/stack/wsf/sources/port/baremetal/wsf_bufio.c         \
    TARGET_CORDIO/stack/wsf/sources/port/baremetal/wsf_detoken.c       \
    TARGET_CORDIO/stack/wsf/sources/port/baremetal/wsf_efs.c           \
    TARGET_CORDIO/stack/wsf/sources/port/baremetal/wsf_heap.c          \
    TARGET_CORDIO/stack/wsf/sources/port/baremetal/wsf_msg.c           \
    TARGET_CORDIO/stack/wsf/sources/port/baremetal/wsf_nvm.c           \
    TARGET_CORDIO/stack/wsf/sources/port/baremetal/wsf_queue.c         \
    TARGET_CORDIO/stack/wsf/sources/port/baremetal/wsf_timer.c         \
    TARGET_CORDIO/stack/wsf/sources/port/baremetal/wsf_trace.c         \
    TARGET_CORDIO/stack/wsf/sources/util/bda.c                         \
    TARGET_CORDIO/stack/wsf/sources/util/bstream.c                     \
    TARGET_CORDIO/stack/wsf/sources/util/calc128.c                     \
    TARGET_CORDIO/stack/wsf/sources/util/crc32.c                       \
    TARGET_CORDIO/stack/wsf/sources/util/fcs.c                         \
    TARGET_CORDIO/stack/wsf/sources/util/prand.c                       \
    TARGET_CORDIO/stack/wsf/sources/util/print.c                       \
    TARGET_CORDIO/stack/wsf/sources/util/terminal.c                    \
    TARGET_CORDIO/stack/wsf/sources/util/wstr.c                        \
    TARGET_CORDIO/stack_adaptation/hci_drv.c                           \
    TARGET_CORDIO/stack_adaptation/hci_tr.c                            \
    TARGET_CORDIO/stack_adaptation/hci_vs.c                            \
    TARGET_CORDIO/stack_adaptation/wsf_cs.c                            \
    TARGET_CORDIO/stack_adaptation/wsf_os.c                            \
    $(NULL)
endif # OPENTHREAD_ENABLE_BLE_HOST

if OPENTHREAD_ENABLE_BLE_CONTROLLER
CORDIO_BLE_CONTROLLER_SRCDIR = $(top_srcdir)/third_party/cordio/TARGET_CORDIO_LL/

libcordio_ble_controller_a_CPPFLAGS                                             = \
    -I$(top_srcdir)/include                                                       \
    -I$(top_srcdir)/src/core                                                      \
    -I$(top_srcdir)/examples/platforms/ble/cordio                                 \
    -I$(CORDIO_BLE_HOST_SRCDIR)/stack/wsf/include                                 \
    -I$(CORDIO_BLE_CONTROLLER_SRCDIR)/stack/controller/include/ble/               \
    -I$(CORDIO_BLE_CONTROLLER_SRCDIR)/stack/controller/include/common/            \
    -I$(CORDIO_BLE_CONTROLLER_SRCDIR)/stack/controller/sources/ble/include/       \
    -I$(CORDIO_BLE_CONTROLLER_SRCDIR)/stack/controller/ble/include/               \
    -I$(CORDIO_BLE_CONTROLLER_SRCDIR)/stack/platform/common/include/              \
    -I$(CORDIO_BLE_CONTROLLER_SRCDIR)/stack/thirdparty/uecc/                      \
    -include cordio-config.h                                                      \
    $(NULL)

libcordio_ble_controller_a_SOURCES                                              = \
    TARGET_CORDIO_LL/stack/controller/sources/ble/bb/bb_ble_adv_master.c          \
    TARGET_CORDIO_LL/stack/controller/sources/ble/bb/bb_ble_adv_slave.c           \
    TARGET_CORDIO_LL/stack/controller/sources/ble/bb/bb_ble_conn.c                \
    TARGET_CORDIO_LL/stack/controller/sources/ble/bb/bb_ble_conn_master.c         \
    TARGET_CORDIO_LL/stack/controller/sources/ble/bb/bb_ble_conn_slave.c          \
    TARGET_CORDIO_LL/stack/controller/sources/ble/bb/bb_ble_main.c                \
    TARGET_CORDIO_LL/stack/controller/sources/ble/bb/bb_ble_test.c                \
    TARGET_CORDIO_LL/stack/controller/sources/ble/init/init.c                     \
    TARGET_CORDIO_LL/stack/controller/sources/ble/init/init_ctr.c                 \
    TARGET_CORDIO_LL/stack/controller/sources/ble/lctr/lctr_act_adv_master.c      \
    TARGET_CORDIO_LL/stack/controller/sources/ble/lctr/lctr_act_adv_slave.c       \
    TARGET_CORDIO_LL/stack/controller/sources/ble/lctr/lctr_act_conn.c            \
    TARGET_CORDIO_LL/stack/controller/sources/ble/lctr/lctr_act_conn_master.c     \
    TARGET_CORDIO_LL/stack/controller/sources/ble/lctr/lctr_act_enc.c             \
    TARGET_CORDIO_LL/stack/controller/sources/ble/lctr/lctr_act_enc_master.c      \
    TARGET_CORDIO_LL/stack/controller/sources/ble/lctr/lctr_act_init_master.c     \
    TARGET_CORDIO_LL/stack/controller/sources/ble/lctr/lctr_act_phy.c             \
    TARGET_CORDIO_LL/stack/controller/sources/ble/lctr/lctr_isr_adv_master.c      \
    TARGET_CORDIO_LL/stack/controller/sources/ble/lctr/lctr_isr_adv_slave.c       \
    TARGET_CORDIO_LL/stack/controller/sources/ble/lctr/lctr_isr_conn.c            \
    TARGET_CORDIO_LL/stack/controller/sources/ble/lctr/lctr_isr_conn_master.c     \
    TARGET_CORDIO_LL/stack/controller/sources/ble/lctr/lctr_isr_conn_slave.c      \
    TARGET_CORDIO_LL/stack/controller/sources/ble/lctr/lctr_isr_init_master.c     \
    TARGET_CORDIO_LL/stack/controller/sources/ble/lctr/lctr_main_adv_master.c     \
    TARGET_CORDIO_LL/stack/controller/sources/ble/lctr/lctr_main_adv_slave.c      \
    TARGET_CORDIO_LL/stack/controller/sources/ble/lctr/lctr_main.c                \
    TARGET_CORDIO_LL/stack/controller/sources/ble/lctr/lctr_main_conn.c           \
    TARGET_CORDIO_LL/stack/controller/sources/ble/lctr/lctr_main_conn_cs2.c       \
    TARGET_CORDIO_LL/stack/controller/sources/ble/lctr/lctr_main_conn_data.c      \
    TARGET_CORDIO_LL/stack/controller/sources/ble/lctr/lctr_main_conn_master.c    \
    TARGET_CORDIO_LL/stack/controller/sources/ble/lctr/lctr_main_conn_slave.c     \
    TARGET_CORDIO_LL/stack/controller/sources/ble/lctr/lctr_main_enc_master.c     \
    TARGET_CORDIO_LL/stack/controller/sources/ble/lctr/lctr_main_enc_slave.c      \
    TARGET_CORDIO_LL/stack/controller/sources/ble/lctr/lctr_main_init_master.c    \
    TARGET_CORDIO_LL/stack/controller/sources/ble/lctr/lctr_main_master_phy.c     \
    TARGET_CORDIO_LL/stack/controller/sources/ble/lctr/lctr_main_priv.c           \
    TARGET_CORDIO_LL/stack/controller/sources/ble/lctr/lctr_main_sc.c             \
    TARGET_CORDIO_LL/stack/controller/sources/ble/lctr/lctr_main_slave_phy.c      \
    TARGET_CORDIO_LL/stack/controller/sources/ble/lctr/lctr_pdu_adv_slave.c       \
    TARGET_CORDIO_LL/stack/controller/sources/ble/lctr/lctr_pdu_conn.c            \
    TARGET_CORDIO_LL/stack/controller/sources/ble/lctr/lctr_pdu_enc.c             \
    TARGET_CORDIO_LL/stack/controller/sources/ble/lctr/lctr_sm_adv_master.c       \
    TARGET_CORDIO_LL/stack/controller/sources/ble/lctr/lctr_sm_adv_slave.c        \
    TARGET_CORDIO_LL/stack/controller/sources/ble/lctr/lctr_sm_conn_master.c      \
    TARGET_CORDIO_LL/stack/controller/sources/ble/lctr/lctr_sm_conn_slave.c       \
    TARGET_CORDIO_LL/stack/controller/sources/ble/lctr/lctr_sm_init_master.c      \
    TARGET_CORDIO_LL/stack/controller/sources/ble/lctr/lctr_sm_llcp_conn.c        \
    TARGET_CORDIO_LL/stack/controller/sources/ble/lctr/lctr_sm_llcp_conn_master.c \
    TARGET_CORDIO_LL/stack/controller/sources/ble/lctr/lctr_sm_llcp_conn_slave.c  \
    TARGET_CORDIO_LL/stack/controller/sources/ble/lctr/lctr_sm_llcp_enc_master.c  \
    TARGET_CORDIO_LL/stack/controller/sources/ble/lctr/lctr_sm_llcp_enc_slave.c   \
    TARGET_CORDIO_LL/stack/controller/sources/ble/lctr/lctr_sm_llcp_master_phy.c  \
    TARGET_CORDIO_LL/stack/controller/sources/ble/lctr/lctr_sm_llcp_slave_phy.c   \
    TARGET_CORDIO_LL/stack/controller/sources/ble/lhci/lhci_cmd_adv_master.c      \
    TARGET_CORDIO_LL/stack/controller/sources/ble/lhci/lhci_cmd_adv_priv.c        \
    TARGET_CORDIO_LL/stack/controller/sources/ble/lhci/lhci_cmd_adv_slave.c       \
    TARGET_CORDIO_LL/stack/controller/sources/ble/lhci/lhci_cmd.c                 \
    TARGET_CORDIO_LL/stack/controller/sources/ble/lhci/lhci_cmd_conn.c            \
    TARGET_CORDIO_LL/stack/controller/sources/ble/lhci/lhci_cmd_conn_master.c     \
    TARGET_CORDIO_LL/stack/controller/sources/ble/lhci/lhci_cmd_conn_priv.c       \
    TARGET_CORDIO_LL/stack/controller/sources/ble/lhci/lhci_cmd_enc_master.c      \
    TARGET_CORDIO_LL/stack/controller/sources/ble/lhci/lhci_cmd_enc_slave.c       \
    TARGET_CORDIO_LL/stack/controller/sources/ble/lhci/lhci_cmd_phy.c             \
    TARGET_CORDIO_LL/stack/controller/sources/ble/lhci/lhci_cmd_sc.c              \
    TARGET_CORDIO_LL/stack/controller/sources/ble/lhci/lhci_evt_adv_master.c      \
    TARGET_CORDIO_LL/stack/controller/sources/ble/lhci/lhci_evt_adv_slave.c       \
    TARGET_CORDIO_LL/stack/controller/sources/ble/lhci/lhci_evt.c                 \
    TARGET_CORDIO_LL/stack/controller/sources/ble/lhci/lhci_evt_conn.c            \
    TARGET_CORDIO_LL/stack/controller/sources/ble/lhci/lhci_evt_conn_cs2.c        \
    TARGET_CORDIO_LL/stack/controller/sources/ble/lhci/lhci_evt_conn_master.c     \
    TARGET_CORDIO_LL/stack/controller/sources/ble/lhci/lhci_evt_conn_priv.c       \
    TARGET_CORDIO_LL/stack/controller/sources/ble/lhci/lhci_evt_enc_master.c      \
    TARGET_CORDIO_LL/stack/controller/sources/ble/lhci/lhci_evt_enc_slave.c       \
    TARGET_CORDIO_LL/stack/controller/sources/ble/lhci/lhci_evt_phy.c             \
    TARGET_CORDIO_LL/stack/controller/sources/ble/lhci/lhci_evt_sc.c              \
    TARGET_CORDIO_LL/stack/controller/sources/ble/lhci/lhci_evt_vs.c              \
    TARGET_CORDIO_LL/stack/controller/sources/ble/lhci/lhci_init_adv_master.c     \
    TARGET_CORDIO_LL/stack/controller/sources/ble/lhci/lhci_init_adv_priv.c       \
    TARGET_CORDIO_LL/stack/controller/sources/ble/lhci/lhci_init_adv_slave.c      \
    TARGET_CORDIO_LL/stack/controller/sources/ble/lhci/lhci_init.c                \
    TARGET_CORDIO_LL/stack/controller/sources/ble/lhci/lhci_init_conn.c           \
    TARGET_CORDIO_LL/stack/controller/sources/ble/lhci/lhci_init_conn_cs2.c       \
    TARGET_CORDIO_LL/stack/controller/sources/ble/lhci/lhci_init_conn_master.c    \
    TARGET_CORDIO_LL/stack/controller/sources/ble/lhci/lhci_init_conn_priv.c      \
    TARGET_CORDIO_LL/stack/controller/sources/ble/lhci/lhci_init_enc_master.c     \
    TARGET_CORDIO_LL/stack/controller/sources/ble/lhci/lhci_init_enc_slave.c      \
    TARGET_CORDIO_LL/stack/controller/sources/ble/lhci/lhci_init_phy.c            \
    TARGET_CORDIO_LL/stack/controller/sources/ble/lhci/lhci_init_sc.c             \
    TARGET_CORDIO_LL/stack/controller/sources/ble/lhci/lhci_main.c                \
    TARGET_CORDIO_LL/stack/controller/sources/ble/ll/ll_init_adv_master.c         \
    TARGET_CORDIO_LL/stack/controller/sources/ble/ll/ll_init_adv_slave.c          \
    TARGET_CORDIO_LL/stack/controller/sources/ble/ll/ll_init.c                    \
    TARGET_CORDIO_LL/stack/controller/sources/ble/ll/ll_init_conn_cs2.c           \
    TARGET_CORDIO_LL/stack/controller/sources/ble/ll/ll_init_conn_master.c        \
    TARGET_CORDIO_LL/stack/controller/sources/ble/ll/ll_init_conn_slave.c         \
    TARGET_CORDIO_LL/stack/controller/sources/ble/ll/ll_init_enc_master.c         \
    TARGET_CORDIO_LL/stack/controller/sources/ble/ll/ll_init_enc_slave.c          \
    TARGET_CORDIO_LL/stack/controller/sources/ble/ll/ll_init_init_master.c        \
    TARGET_CORDIO_LL/stack/controller/sources/ble/ll/ll_init_master_phy.c         \
    TARGET_CORDIO_LL/stack/controller/sources/ble/ll/ll_init_priv.c               \
    TARGET_CORDIO_LL/stack/controller/sources/ble/ll/ll_init_sc.c                 \
    TARGET_CORDIO_LL/stack/controller/sources/ble/ll/ll_init_slave_phy.c          \
    TARGET_CORDIO_LL/stack/controller/sources/ble/ll/ll_main_adv_master.c         \
    TARGET_CORDIO_LL/stack/controller/sources/ble/ll/ll_main_adv_slave.c          \
    TARGET_CORDIO_LL/stack/controller/sources/ble/ll/ll_main.c                    \
    TARGET_CORDIO_LL/stack/controller/sources/ble/ll/ll_main_conn.c               \
    TARGET_CORDIO_LL/stack/controller/sources/ble/ll/ll_main_conn_master.c        \
    TARGET_CORDIO_LL/stack/controller/sources/ble/ll/ll_main_conn_slave.c         \
    TARGET_CORDIO_LL/stack/controller/sources/ble/ll/ll_main_diag.c               \
    TARGET_CORDIO_LL/stack/controller/sources/ble/ll/ll_main_dtm.c                \
    TARGET_CORDIO_LL/stack/controller/sources/ble/ll/ll_main_enc_master.c         \
    TARGET_CORDIO_LL/stack/controller/sources/ble/ll/ll_main_enc_slave.c          \
    TARGET_CORDIO_LL/stack/controller/sources/ble/ll/ll_main_phy.c                \
    TARGET_CORDIO_LL/stack/controller/sources/ble/ll/ll_main_priv.c               \
    TARGET_CORDIO_LL/stack/controller/sources/ble/ll/ll_main_sc.c                 \
    TARGET_CORDIO_LL/stack/controller/sources/ble/ll/ll_math.c                    \
    TARGET_CORDIO_LL/stack/controller/sources/ble/lmgr/lmgr_events.c              \
    TARGET_CORDIO_LL/stack/controller/sources/ble/lmgr/lmgr_main.c                \
    TARGET_CORDIO_LL/stack/controller/sources/ble/lmgr/lmgr_main_conn.c           \
    TARGET_CORDIO_LL/stack/controller/sources/ble/lmgr/lmgr_main_master.c         \
    TARGET_CORDIO_LL/stack/controller/sources/ble/lmgr/lmgr_main_priv.c           \
    TARGET_CORDIO_LL/stack/controller/sources/ble/lmgr/lmgr_main_sc.c             \
    TARGET_CORDIO_LL/stack/controller/sources/ble/lmgr/lmgr_main_slave.c          \
    TARGET_CORDIO_LL/stack/controller/sources/ble/sch/sch_ble.c                   \
    TARGET_CORDIO_LL/stack/controller/sources/ble/sch/sch_rm.c                    \
    TARGET_CORDIO_LL/stack/controller/sources/common/bb/bb_main.c                 \
    TARGET_CORDIO_LL/stack/controller/sources/common/sch/sch_list.c               \
    TARGET_CORDIO_LL/stack/controller/sources/common/sch/sch_main.c               \
    TARGET_CORDIO_LL/stack/platform/common/sources/bb/ble/bb_ble_pdufilt.c        \
    TARGET_CORDIO_LL/stack/platform/common/sources/bb/ble/bb_ble_periodiclist.c   \
    TARGET_CORDIO_LL/stack/platform/common/sources/bb/ble/bb_ble_reslist.c        \
    TARGET_CORDIO_LL/stack/platform/common/sources/bb/ble/bb_ble_whitelist.c      \
    TARGET_CORDIO_LL/stack/platform/common/sources/chci/chci_tr.c                 \
    TARGET_CORDIO_LL/stack/platform/common/sources/chci/chci_tr_serial.c          \
    TARGET_CORDIO_LL/stack/platform/common/sources/ll/lhci_cmd_vs_ext.c           \
    TARGET_CORDIO_LL/stack/platform/common/sources/ll/ll_math_ecc.c               \
    TARGET_CORDIO_LL/stack/thirdparty/uecc/uECC_ll.c                              \
    $(NULL)
endif # OPENTHREAD_ENABLE_BLE_CONTROLLER

include $(abs_top_nlbuild_autotools_dir)/automake/post.am
