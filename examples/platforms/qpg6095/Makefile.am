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

# Do not enable -Wcast-align for this platform
override CFLAGS    := $(filter-out -Wcast-align,$(CFLAGS))
override CXXFLAGS  := $(filter-out -Wcast-align,$(CXXFLAGS))

lib_LIBRARIES                               = libopenthread-qpg6095.a

libopenthread_qpg6095_a_CPPFLAGS            = \
    -I$(top_srcdir)/include                   \
    -I$(top_srcdir)/examples/platforms        \
    -I$(top_srcdir)/examples/platforms/utils  \
    -I$(top_srcdir)/src/core                  \
    -lrt                                      \
    -lpthread                                 \
    $(NULL)

CLEANFILES = 

libopenthread_qpg6095_a_SOURCES             = \
    alarm.c                                   \
    alarm_qorvo.h                             \
    diag.c                                    \
    entropy.c                                 \
    logging.c                                 \
    misc.c                                    \
    misc_qorvo.h                              \
    openthread-core-qpg6095-config.h          \
    openthread-core-qpg6095-config-check.h \
    platform.c                                \
    platform_qorvo.h                          \
    radio.c                                   \
    radio_qorvo.h                             \
    random_qorvo.h                            \
    settings.cpp                              \
    settings_qorvo.h                          \
    uart.c                                    \
    uart_qorvo.h                              \
    $(NULL)

libopenthread_qpg6095_a_LIBADD              = \
    $(top_builddir)/examples/platforms/utils/libopenthread_platform_utils_a-settings_ram.o

if OPENTHREAD_BUILD_COVERAGE
CLEANFILES                               += $(wildcard *.gcda *.gcno)
endif # OPENTHREAD_BUILD_COVERAGE

include $(abs_top_nlbuild_autotools_dir)/automake/post.am
