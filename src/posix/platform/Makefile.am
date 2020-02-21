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
    -I$(top_srcdir)/src                     \
    -I$(top_srcdir)/src/core                \
    -I$(top_srcdir)/src/posix/platform      \
    -D_GNU_SOURCE                           \
    -DOPENTHREAD_FTD=1                      \
    -DOPENTHREAD_MTD=0                      \
    -DOPENTHREAD_RADIO=0                    \
    $(NULL)

libopenthread_posix_a_SOURCES             = \
    alarm.cpp                               \
    entropy.cpp                             \
    hdlc_interface.cpp                      \
    logging.cpp                             \
    misc.cpp                                \
    netif.cpp                               \
    radio_spinel.cpp                        \
    settings.cpp                            \
    spi_interface.cpp                       \
    system.cpp                              \
    uart.cpp                                \
    udp.cpp                                 \
    virtual_time.cpp                        \
    $(NULL)

noinst_HEADERS                            = \
    hdlc_interface.hpp                      \
    openthread-posix-config.h               \
    platform-posix.h                        \
    radio_spinel.hpp                        \
    spinel_interface.hpp                    \
    $(NULL)

openthread_HEADERS                        = \
    openthread-system.h                     \
    $(NULL)

openthreaddir = $(includedir)/openthread
dist_openthread_HEADERS = $(openthread_headers)

PRETTY_FILES                              = \
    $(libopenthread_posix_a_SOURCES)        \
    $(noinst_HEADERS)                       \
    $(openthread_HEADERS)                   \
    $(NULL)

if OPENTHREAD_BUILD_COVERAGE
CLEANFILES                                = $(wildcard *.gcda *.gcno)
endif # OPENTHREAD_BUILD_COVERAGE

check_PROGRAMS = test-settings

test_settings_CPPFLAGS                    = \
    -I$(top_srcdir)/include                 \
    -I$(top_srcdir)/src/core                \
    -DOPENTHREAD_CONFIG_LOG_PLATFORM=0      \
    -DSELF_TEST                             \
    $(NULL)

test_settings_SOURCES                     = \
    settings.cpp                            \
    $(NULL)

TESTS                                     = \
    test-settings                           \
    $(NULL)

include $(abs_top_nlbuild_autotools_dir)/automake/post.am
