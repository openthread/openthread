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

CORDIO_SRCDIR                       = $(top_srcdir)/third_party/cordio/stack/

libopenthread_platform_ble_a_CPPFLAGS                             = \
    -I$(top_srcdir)/include                                         \
    -I$(top_srcdir)/examples/platforms                              \
    -I$(top_srcdir)/src/core                                        \
    -I$(CORDIO_SRCDIR)/cordio_stack/ble-host/include               \
    -I$(CORDIO_SRCDIR)/cordio_stack/ble-host/sources/hci/dual_chip \
    -I$(CORDIO_SRCDIR)/cordio_stack/ble-host/sources/stack/cfg     \
    -I$(CORDIO_SRCDIR)/cordio_stack/platform/common/include        \
    -I$(CORDIO_SRCDIR)/cordio_stack/wsf/common/include             \
    -I$(CORDIO_SRCDIR)/cordio_stack/wsf/common/include/util        \
    -I$(CORDIO_SRCDIR)/mbed_os_adaptation                          \
    $(NULL)

libopenthread_platform_ble_a_SOURCES  = \
    ble_gap.c                           \
    ble_gatt.c                          \
    ble_hci_driver.c                    \
    ble_l2cap.c                         \
    ble_mgmt.c                          \
    $(NULL)

noinst_HEADERS                        = \
    ble_gap.h                           \
    ble_gatt.h                          \
    ble_hci_driver.h                    \
    ble_l2cap.h                         \
    ble_mgmt.h                          \
    ble_utils.h                         \
    $(NULL)

include $(abs_top_nlbuild_autotools_dir)/automake/post.am
