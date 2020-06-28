#
#  Copyright (c) 2018, The OpenThread Authors.
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

# Do not enable -Wcast-align for this platform
override CFLAGS    := $(filter-out -Wcast-align,$(CFLAGS))
override CXXFLAGS  := $(filter-out -Wcast-align,$(CXXFLAGS))

lib_LIBRARIES = libopenthread-cc1352.a

libopenthread_cc1352_a_CPPFLAGS                                                         = \
    -I$(top_srcdir)/include                                                               \
    -I$(top_srcdir)/src                                                                   \
    -I$(top_srcdir)/src/core                                                              \
    -I$(top_srcdir)/examples/platforms                                                    \
    -I$(top_srcdir)/examples/platforms/cc1352                                             \
    -I$(top_srcdir)/third_party/ti/devices/cc13x2_cc26x2                                  \
    -I$(top_srcdir)/third_party/mbedtls/repo/include                                      \
    $(MBEDTLS_CPPFLAGS)                                                                   \
    $(NULL)

PLATFORM_SOURCES                                                                        = \
    alarm.c                                                                               \
    cc1352_radio.h                                                                        \
    diag.c                                                                                \
    entropy.c                                                                             \
    flash.c                                                                               \
    logging.c                                                                             \
    misc.c                                                                                \
    openthread-core-cc1352-config.h                                                       \
    openthread-core-cc1352-config-check.h                                                 \
    platform-cc1352.h                                                                     \
    radio.c                                                                               \
    system.c                                                                              \
    uart.c                                                                                \
    crypto/aes_alt.c                                                                      \
    cc1352_ccfg.c                                                                         \
    cc1352_startup.c                                                                      \
    cxx_helpers.c                                                                         \
    $(NULL)

libopenthread_cc1352_a_SOURCES                                                          = \
    $(PLATFORM_SOURCES)                                                                   \
    $(NULL)

libopenthread_cc1352_a_DEPENDENCIES                                                     = \
    $(top_srcdir)/third_party/ti/devices/cc13x2_cc26x2/driverlib/bin/gcc/driverlib.a      \
    $(NULL)

Dash = -
libopenthread_cc1352_a_LIBADD                                                           = \
    $(shell find $(top_builddir)/examples/platforms/utils $(Dash)type f $(Dash)name "*.o")

include $(abs_top_nlbuild_autotools_dir)/automake/post.am

