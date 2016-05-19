#
#  Copyright (c) 2016, Nest Labs, Inc.
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

#
# Local headers to build against and distribute but not to install
# since they are not part of the package.
#
noinst_HEADERS                                                 = \
    test_util.h                                                  \
    $(NULL)

#
# Other files we do want to distribute with the package.
#
EXTRA_DIST                                                     = \
    $(NULL)

if OPENTHREAD_BUILD_TESTS
# C preprocessor option flags that will apply to all compiled objects in this
# makefile.

AM_CPPFLAGS                                                    = \
    -I$(top_srcdir)/include                                      \
    -I$(top_srcdir)/src                                          \
    -I$(top_srcdir)/src/core                                     \
    -I$(top_srcdir)/third_party/mbedtls/repo/include             \
    $(NULL)

COMMON_LDADD                                                   =  \
    $(top_builddir)/src/core/libopenthread.a                      \
    $(top_builddir)/examples/platform/posix/libopenthread-posix.a \
    $(top_builddir)/third_party/mbedtls/libmbedcrypto.a           \
    -lpthread                                                     \
    $(NULL)

# Test applications that should be run when the 'check' target is run.

check_PROGRAMS                                                 = \
    test-aes                                                     \
    test-hmac-sha256                                             \
    test-mac-frame                                               \
    test-message                                                 \
    $(NULL)

# Test applications and scripts that should be built and run when the
# 'check' target is run.

TESTS                                                          = \
    $(check_PROGRAMS)                                            \
    $(NULL)

# The additional environment variables and their values that will be
# made available to all programs and scripts in TESTS.

TESTS_ENVIRONMENT                                              = \
    $(NULL)

# Source, compiler, and linker options for test programs.

test_aes_LDADD               = $(COMMON_LDADD)
test_aes_SOURCES             = test_aes.cpp

test_hmac_sha256_LDADD       = $(COMMON_LDADD)
test_hmac_sha256_SOURCES     = test_hmac_sha256.cpp

test_mac_frame_LDADD         = $(COMMON_LDADD)
test_mac_frame_SOURCES       = test_mac_frame.cpp

test_message_LDADD           = $(COMMON_LDADD)
test_message_SOURCES         = test_message.cpp

endif # OPENTHREAD_BUILD_TESTS

include $(abs_top_nlbuild_autotools_dir)/automake/post.am
