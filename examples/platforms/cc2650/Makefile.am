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

lib_LIBRARIES                                         = libopenthread-cc2650.a

# Do not enable -pedantic-errors for cc26xx driverlib
override CFLAGS                                      := $(filter-out -pedantic-errors,$(CFLAGS))
override CXXFLAGS                                    := $(filter-out -pedantic-errors,$(CXXFLAGS))

libopenthread_cc2650_a_CPPFLAGS                       = \
    -I$(top_srcdir)/include                             \
    -I$(top_srcdir)/src                                 \
    -I$(top_srcdir)/src/core                            \
    -I$(top_srcdir)/examples/platforms                  \
    -I$(top_srcdir)/examples/platforms/cc2650           \
    -I$(top_srcdir)/third_party/ti/devices/cc26x0       \
    -I$(top_srcdir)/third_party/mbedtls/repo/include    \
    $(MBEDTLS_CPPFLAGS)                                 \
    $(NULL)

PLATFORM_SOURCES                                      = \
    alarm.c                                             \
    diag.c                                              \
    flash.c                                             \
    misc.c                                              \
    logging.c                                           \
    radio.c                                             \
    random.c                                            \
    system.c                                            \
    uart.c                                              \
    crypto/sha256_alt.c                                 \
    crypto/aes_alt.c                                    \
    cxx_helpers.c                                       \
    cc2650_ccfg.c                                       \
    cc2650_startup.c                                    \
    $(NULL)

libopenthread_cc2650_a_SOURCES                        = \
   $(PLATFORM_SOURCES)                                  \
   $(NULL)

libopenthread_cc2650_a_DEPENDENCIES                   =                       \
    $(top_srcdir)/third_party/ti/devices/cc26x0/driverlib/bin/gcc/driverlib.a \
    $(NULL)

noinst_HEADERS                                        = \
    platform-cc2650.h                                   \
    $(NULL)

PRETTY_FILES                                          = \
    $(PLATFORM_SOURCES)                                 \
    $(noinst_HEADERS)                                   \
    $(NULL)

Dash                                                  = -
libopenthread_cc2650_a_LIBADD                         = \
    $(shell find $(top_builddir)/examples/platforms/utils $(Dash)type f $(Dash)name "*.o")

include $(abs_top_nlbuild_autotools_dir)/automake/post.am
