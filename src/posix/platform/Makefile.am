#
#  Copyright (c) 2016-2018, The OpenThread Authors.
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

lib_LIBRARIES                             = libopenthread-posix.a

libopenthread_posix_a_CPPFLAGS            = \
    -I$(top_srcdir)/include                 \
    -I$(top_srcdir)/src/core                \
    -I$(top_srcdir)/src/ncp                 \
    -I$(top_srcdir)/src/posix/platform      \
    -D_GNU_SOURCE                           \
    $(OPENTHREAD_TARGET_DEFINES)            \
    $(NULL)

libopenthread_posix_a_SOURCES             = \
    alarm.c                                 \
    hdlc_interface.cpp                      \
    logging.c                               \
    misc.c                                  \
    netif.cpp                               \
    radio_spinel.cpp                        \
    random.c                                \
    settings.cpp                            \
    spi-stubs.c                             \
    sim.c                                   \
    system.c                                \
    uart.c                                  \
    $(NULL)

INC_NCP_SOURCES                           = \
    inc_spinel.c                            \
    inc_hdlc.cpp                            \
    $(NULL)

nodist_libopenthread_posix_a_SOURCES      = \
    $(INC_NCP_SOURCES)                      \
    $(NULL)

inc_%: ../../ncp/%
	echo '#include "$<"' > $@

if OPENTHREAD_ENABLE_PLATFORM_UDP
libopenthread_posix_a_SOURCES            += \
    udp.cpp                                 \
    $(NULL)
endif

noinst_HEADERS                            = \
    openthread-system.h                     \
    platform-posix.h                        \
    hdlc_interface.hpp                      \
    radio_spinel.hpp                        \
    $(NULL)

PRETTY_FILES                              = \
    $(libopenthread_posix_a_SOURCES)        \
    $(noinst_HEADERS)                       \
    $(NULL)

if OPENTHREAD_BUILD_COVERAGE
CLEANFILES                                = $(wildcard *.gcda *.gcno)
endif # OPENTHREAD_BUILD_COVERAGE

check_PROGRAMS = test-settings

test_settings_CPPFLAGS                    = \
    -I$(top_srcdir)/include                 \
    -I$(top_srcdir)/src/core                \
    -DSELF_TEST                             \
    $(NULL)

test_settings_SOURCES                     = \
    settings.cpp                            \
    $(NULL)

TESTS                                     = \
    test-settings                           \
    $(NULL)

include $(abs_top_nlbuild_autotools_dir)/automake/post.am
