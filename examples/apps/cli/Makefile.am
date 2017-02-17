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

bin_PROGRAMS                                                           = \
    ot-cli-ftd                                                           \
    ot-cli-mtd                                                           \
    $(NULL)

CPPFLAGS_COMMON                                                        = \
    -I$(top_srcdir)/include                                              \
    -I$(top_srcdir)/src/core                                             \
    -I$(top_srcdir)/examples/platforms                                   \
    $(NULL)

LDADD_COMMON                                                           = \
    $(NULL)

LDFLAGS_COMMON                                                         = \
    $(NULL)

SOURCES_COMMON                                                         = \
    main.c                                                               \
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

if OPENTHREAD_EXAMPLES_POSIX
LDADD_COMMON                                                          += \
    $(top_builddir)/examples/platforms/posix/libopenthread-posix.a       \
    -lstdc++                                                             \
    $(NULL)
endif # OPENTHREAD_EXAMPLES_POSIX

if OPENTHREAD_EXAMPLES_CC2538
LDADD_COMMON                                                          += \
    $(top_builddir)/examples/platforms/cc2538/libopenthread-cc2538.a     \
    $(NULL)

LDFLAGS_COMMON                                                        += \
    -T $(top_srcdir)/examples/platforms/cc2538/cc2538.ld                 \
    $(NULL)
endif # OPENTHREAD_EXAMPLES_CC2538

if OPENTHREAD_EXAMPLES_CC2650
CPPFLAGS_COMMON                                                       += \
    -I$(top_srcdir)/third_party/ti/cc26xxware                            \
    $(NULL)

LDADD_COMMON                                                          +=  \
    $(top_builddir)/examples/platforms/cc2650/libopenthread-cc2650.a      \
    $(top_srcdir)/third_party/ti/cc26xxware/driverlib/bin/gcc/driverlib.a \
    $(NULL)

LDFLAGS_COMMON                                                        +=   \
    -T $(top_srcdir)/third_party/ti/cc26xxware/linker_files/cc26x0f128.lds \
    $(NULL)

endif #OPENTHREAD_EXAMPLES_CC2650

if OPENTHREAD_EXAMPLES_DA15000
LDADD_COMMON                                                          += \
    $(top_builddir)/examples/platforms/da15000/libopenthread-da15000.a   \
    $(NULL)

LDFLAGS_COMMON                                                        += \
    -T $(top_srcdir)/examples/platforms/da15000/da15000.ld               \
    $(NULL)
endif # OPENTHREAD_EXAMPLES_DA15000

if OPENTHREAD_EXAMPLES_NRF52840
LDADD_COMMON                                                          += \
    $(top_builddir)/examples/platforms/nrf52840/libopenthread-nrf52840.a \
    $(NULL)

LDFLAGS_COMMON                                                        += \
    -T $(top_srcdir)/examples/platforms/nrf52840/nrf52840.ld             \
    $(NULL)
endif

ot_cli_ftd_CPPFLAGS                                                    = \
    $(CPPFLAGS_COMMON)                                                   \
    $(NULL)

ot_cli_ftd_LDADD                                                       = \
    $(top_builddir)/src/cli/libopenthread-cli.a                          \
    $(top_builddir)/src/core/libopenthread-ftd.a                         \
    $(LDADD_COMMON)                                                      \
    $(NULL)

ot_cli_ftd_LDFLAGS                                                     = \
    $(LDFLAGS_COMMON)                                                    \
    $(NULL)

ot_cli_ftd_SOURCES                                                     = \
    $(SOURCES_COMMON)                                                    \
    $(NULL)

ot_cli_mtd_CPPFLAGS                                                    = \
    $(CPPFLAGS_COMMON)                                                   \
    $(NULL)

ot_cli_mtd_LDADD                                                       = \
    $(top_builddir)/src/cli/libopenthread-cli.a                          \
    $(top_builddir)/src/core/libopenthread-mtd.a                         \
    $(LDADD_COMMON)                                                      \
    $(NULL)

ot_cli_mtd_LDFLAGS                                                     = \
    $(LDFLAGS_COMMON)                                                    \
    $(NULL)

ot_cli_mtd_SOURCES                                                     = \
    $(SOURCES_COMMON)                                                    \
    $(NULL)

if OPENTHREAD_BUILD_COVERAGE
CLEANFILES                                                             = $(wildcard *.gcda *.gcno)
endif # OPENTHREAD_BUILD_COVERAGE

include $(abs_top_nlbuild_autotools_dir)/automake/post.am
