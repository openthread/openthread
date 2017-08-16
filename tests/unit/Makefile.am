#
#  Copyright (c) 2016, The OpenThread Authors.
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
noinst_HEADERS                                                      = \
    test_platform.h                                                   \
    test_lowpan.hpp                                                   \
    test_util.h                                                       \
    test_util.hpp                                                     \
    $(NULL)

#
# Other files we do want to distribute with the package.
#
EXTRA_DIST                                                          = \
    $(NULL)

if OPENTHREAD_BUILD_TESTS
# C preprocessor option flags that will apply to all compiled objects in this
# makefile.

AM_CPPFLAGS                                                         = \
    -DOPENTHREAD_FTD=1                                                \
    -I$(top_srcdir)/include                                           \
    -I$(top_srcdir)/src                                               \
    -I$(top_srcdir)/src/core                                          \
    $(NULL)

if OPENTHREAD_ENABLE_NCP

COMMON_LDADD                                                        = \
    $(top_builddir)/src/ncp/libopenthread-ncp-ftd.a                   \
    $(top_builddir)/src/core/libopenthread-ftd.a                      \
    -lpthread                                                         \
    $(NULL)

else

COMMON_LDADD                                                        = \
    $(top_builddir)/src/core/libopenthread-ftd.a                      \
    -lpthread                                                         \
    $(NULL)

endif # OPENTHREAD_ENABLE_NCP

if OPENTHREAD_ENABLE_BUILTIN_MBEDTLS
COMMON_LDADD                                                       += \
    $(top_builddir)/third_party/mbedtls/libmbedcrypto.a               \
    $(NULL)
endif # OPENTHREAD_ENABLE_BUILTIN_MBEDTLS

# Test applications that should be run when the 'check' target is run.

check_PROGRAMS                                                      = \
    test-aes                                                          \
    test-fuzz                                                         \
    test-heap                                                         \
    test-hmac-sha256                                                  \
    test-lowpan                                                       \
    test-link-quality                                                 \
    test-mac-frame                                                    \
    test-message                                                      \
    test-message-queue                                                \
    test-priority-queue                                               \
    test-pskc                                                         \
    test-strlcat                                                      \
    test-strlcpy                                                      \
    test-strnlen                                                      \
    test-timer                                                        \
    test-toolchain                                                    \
    $(NULL)

XFAIL_TESTS                                                         = \
    $(NULL)

if OPENTHREAD_ENABLE_DIAG
check_PROGRAMS                                                     += \
    test-diag                                                         \
    $(NULL)
endif # OPENTHREAD_ENABLE_DIAG

if OPENTHREAD_ENABLE_NCP
check_PROGRAMS                                                     += \
    test-ncp-buffer                                                   \
    $(NULL)
endif # OPENTHREAD_ENABLE_NCP

if OPENTHREAD_WITH_ADDRESS_SANITIZER
check_PROGRAMS                 += test-address-sanitizer
XFAIL_TESTS                    += test-address-sanitizer
test_address_sanitizer_SOURCES  = test_address_sanitizer.cpp
endif # OPENTHREAD_WITH_ADDRESS_SANITIZER


# Test applications and scripts that should be built and run when the
# 'check' target is run.

TESTS                                                               = \
    $(check_PROGRAMS)                                                 \
    $(NULL)

# The additional environment variables and their values that will be
# made available to all programs and scripts in TESTS.

TESTS_ENVIRONMENT                                                   = \
    $(NULL)

# Source, compiler, and linker options for test programs.

test_aes_LDADD               = $(COMMON_LDADD)
test_aes_SOURCES             = test_platform.cpp test_aes.cpp

test_fuzz_LDADD              = $(COMMON_LDADD)
test_fuzz_SOURCES            = test_platform.cpp test_fuzz.cpp

test_heap_LDADD              = $(COMMON_LDADD)
test_heap_SOURCES            = test_platform.cpp test_heap.cpp

test_hmac_sha256_LDADD       = $(COMMON_LDADD)
test_hmac_sha256_SOURCES     = test_platform.cpp test_hmac_sha256.cpp

test_link_quality_LDADD      = $(COMMON_LDADD)
test_link_quality_SOURCES    = test_platform.cpp test_link_quality.cpp

test_lowpan_LDADD            = $(COMMON_LDADD)
test_lowpan_SOURCES          = test_platform.cpp test_lowpan.cpp test_util.cpp

test_mac_frame_LDADD         = $(COMMON_LDADD)
test_mac_frame_SOURCES       = test_platform.cpp test_mac_frame.cpp

test_message_LDADD           = $(COMMON_LDADD)
test_message_SOURCES         = test_platform.cpp test_message.cpp

test_message_queue_LDADD     = $(COMMON_LDADD)
test_message_queue_SOURCES   = test_platform.cpp test_message_queue.cpp

test_ncp_buffer_LDADD        = $(COMMON_LDADD)
test_ncp_buffer_SOURCES      = test_platform.cpp test_ncp_buffer.cpp

test_priority_queue_LDADD    = $(COMMON_LDADD)
test_priority_queue_SOURCES  = test_platform.cpp test_priority_queue.cpp

test_pskc_LDADD              = $(COMMON_LDADD)
test_pskc_SOURCES            = test_platform.cpp test_pskc.cpp

test_strlcat_LDADD           = $(COMMON_LDADD)
test_strlcat_SOURCES         = test_strlcat.c

test_strlcpy_LDADD           = $(COMMON_LDADD)
test_strlcpy_SOURCES         = test_strlcpy.c

test_strnlen_LDADD           = $(COMMON_LDADD)
test_strnlen_SOURCES         = test_strnlen.c

test_timer_LDADD             = $(COMMON_LDADD)
test_timer_SOURCES           = test_platform.cpp test_timer.cpp

test_toolchain_LDADD         = $(COMMON_LDADD)
test_toolchain_SOURCES       = test_platform.cpp test_toolchain.cpp test_toolchain_c.c

if OPENTHREAD_ENABLE_DIAG
test_diag_LDADD              = $(top_builddir)/src/diag/libopenthread-diag.a                  \
                               $(top_builddir)/examples/platforms/posix/libopenthread-posix.a \
                               $(NULL)
test_diag_SOURCES            = test_diag.cpp
endif

if OPENTHREAD_BUILD_COVERAGE
CLEANFILES                   = $(wildcard *.gcda *.gcno)
endif # OPENTHREAD_BUILD_COVERAGE

endif # OPENTHREAD_BUILD_TESTS

include $(abs_top_nlbuild_autotools_dir)/automake/post.am
