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

# Always package (e.g. for 'make dist') these subdirectories.

DIST_SUBDIRS                                                           = \
    platform                                                             \
    $(NULL)

# Always build (e.g. for 'make all') these subdirectories.

SUBDIRS                                                                = \
    platform                                                             \
    $(NULL)

# Always pretty (e.g. for 'make pretty') these subdirectories.

PRETTY_SUBDIRS                                                         = \
    platform                                                             \
    $(NULL)

CPPFLAGS_COMMON                                                        = \
    -I$(top_srcdir)/include                                              \
    -I$(top_srcdir)/src/core                                             \
    -I$(top_srcdir)/src/posix/platform                                   \
    -D_GNU_SOURCE                                                        \
    $(OPENTHREAD_TARGET_DEFINES)                                         \
    $(NULL)

LIBTOOLFLAGS_COMMON = --preserve-dup-deps

LDADD_COMMON                                                           = \
    $(top_builddir)/src/posix/platform/libopenthread-posix.a             \
    -lutil                                                               \
    $(NULL)

if OPENTHREAD_ENABLE_BUILTIN_MBEDTLS
LDADD_COMMON                                                          += \
    $(top_builddir)/third_party/mbedtls/libmbedcrypto.a                  \
    $(NULL)
endif # OPENTHREAD_ENABLE_BUILTIN_MBEDTLS

if OPENTHREAD_ENABLE_DIAG
LDADD_COMMON                                                          += \
    $(top_builddir)/src/diag/libopenthread-diag.a                        \
    $(NULL)
endif # OPENTHREAD_ENABLE_DIAG

bin_PROGRAMS                                                           = \
    $(NULL)

if OPENTHREAD_ENABLE_EXECUTABLE
if OPENTHREAD_ENABLE_NCP
bin_PROGRAMS                                                          += \
    ot-ncp                                                               \
    $(NULL)
endif

ot_ncp_CPPFLAGS                                                        = \
    $(CPPFLAGS_COMMON)                                                   \
    -DOPENTHREAD_POSIX_APP=1                                             \
    $(NULL)

ot_ncp_SOURCES                                                         = \
    main.c                                                               \
    $(NULL)

ot_ncp_LDADD                                                           = \
    $(top_builddir)/src/ncp/libopenthread-ncp-ftd.a                      \
    $(top_builddir)/src/core/libopenthread-ftd.a                         \
    $(LDADD_COMMON)                                                      \
    $(top_builddir)/src/core/libopenthread-ftd.a                         \
    $(LDADD_COMMON)                                                      \
    $(NULL)

ot_ncp_LDFLAGS                                                         = \
    $(LDFLAGS_COMMON)                                                    \
    $(NULL)

ot_ncp_LIBTOOLFLAGS                                                    = \
    $(LIBTOOLFLAGS_COMMON)                                               \
    $(NULL)

if OPENTHREAD_ENABLE_CLI
bin_PROGRAMS                                                          += \
    ot-cli                                                               \
    $(NULL)
endif

ot_cli_CPPFLAGS                                                        = \
    $(CPPFLAGS_COMMON)                                                   \
    -DOPENTHREAD_POSIX_APP=2                                             \
    $(NULL)

ot_cli_SOURCES                                                         = \
    main.c                                                               \
    $(NULL)

ot_cli_LDADD                                                           = \
    $(top_builddir)/src/cli/libopenthread-cli-ftd.a                      \
    $(top_builddir)/src/core/libopenthread-ftd.a                         \
    $(LDADD_COMMON)                                                      \
    $(top_builddir)/src/core/libopenthread-ftd.a                         \
    $(LDADD_COMMON)                                                      \
    $(NULL)

ot_cli_LDFLAGS                                                         = \
    $(LDFLAGS_COMMON)                                                    \
    $(NULL)

ot_cli_LIBTOOLFLAGS                                                    = \
    $(LIBTOOLFLAGS_COMMON)                                               \
    $(NULL)

if OPENTHREAD_ENABLE_LINKER_MAP
ot_ncp_LDFLAGS                                                        += -Wl,-Map=ot-cli-ftd.map
ot_cli_LDFLAGS                                                        += -Wl,-Map=ot-cli-mtd.map
endif

if OPENTHREAD_BUILD_COVERAGE
CPPFLAGS_COMMON                                                       += \
    -DOPENTHREAD_ENABLE_COVERAGE                                         \
    $(NULL)

CLEANFILES                                                             = $(wildcard *.gcda *.gcno)
endif # OPENTHREAD_BUILD_COVERAGE
endif # OPENTHREAD_ENABLE_EXECUTABLE

include $(abs_top_nlbuild_autotools_dir)/automake/post.am
