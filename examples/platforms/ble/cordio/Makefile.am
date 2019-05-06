#
#  Copyright (c) 2019, The OpenThread Authors.
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

lib_LIBRARIES                        = libopenthread-platform-ble.a

CORDIO_BLE_HOST_SRCDIR               = $(top_srcdir)/third_party/cordio/TARGET_CORDIO/

libopenthread_platform_ble_a_CPPFLAGS                               = \
    -I$(top_srcdir)/include                                           \
    -I$(top_srcdir)/examples/platforms                                \
    -I$(top_srcdir)/examples/platforms/ble                            \
    -I$(top_srcdir)/src/core                                          \
    -I$(CORDIO_BLE_HOST_SRCDIR)/stack/ble-host/include                \
    -I$(CORDIO_BLE_HOST_SRCDIR)/stack/ble-host/sources/hci/dual_chip  \
    -I$(CORDIO_BLE_HOST_SRCDIR)/stack/ble-host/sources/stack/cfg      \
    -I$(CORDIO_BLE_HOST_SRCDIR)/stack/platform/include                \
    -I$(CORDIO_BLE_HOST_SRCDIR)/stack/wsf/include                     \
    -I$(CORDIO_BLE_HOST_SRCDIR)/stack/wsf/include/util                \
    -I$(CORDIO_BLE_HOST_SRCDIR)/stack_adaptation                      \
    $(NULL)

#if OPENTHREAD_ENABLE_BLE_CONTROLLER

CORDIO_BLE_CONTROLLER_SRCDIR         = $(top_srcdir)/third_party/cordio/TARGET_CORDIO_LL/

CORDIO_BLE_CONTROLLER_CPPFLAGS                                            = \
    -I$(CORDIO_BLE_CONTROLLER_SRCDIR)/stack/controller/include/ble/         \
    -I$(CORDIO_BLE_CONTROLLER_SRCDIR)/stack/controller/include/common/      \
    -I$(CORDIO_BLE_CONTROLLER_SRCDIR)/stack/controller/sources/ble/include/ \
    -I$(CORDIO_BLE_CONTROLLER_SRCDIR)/stack/controller/ble/include/         \
    -I$(CORDIO_BLE_CONTROLLER_SRCDIR)/stack/controller/sources/ble/lctr/    \
    -I$(CORDIO_BLE_CONTROLLER_SRCDIR)/stack/platform/common/include/        \
    $(NULL)

libopenthread_platform_ble_a_CPPFLAGS += $(CORDIO_BLE_CONTROLLER_CPPFLAGS)
#endif

libopenthread_platform_ble_a_SOURCES  = \
    ble_bb_driver.c                     \
    ble_controller_init.c               \
    ble_gap.c                           \
    ble_gatt.c                          \
    ble_hci_driver.c                    \
    ble_l2cap.c                         \
    ble_wsf.c                           \
    ble_init.c                          \
    $(NULL)

noinst_HEADERS                        = \
    ble_controller_init.h               \
    ble_gap.h                           \
    ble_gatt.h                          \
    ble_hci_driver.h                    \
    ble_l2cap.h                         \
    ble_init.h                          \
    ble_utils.h                         \
    $(NULL)

include $(abs_top_nlbuild_autotools_dir)/automake/post.am
