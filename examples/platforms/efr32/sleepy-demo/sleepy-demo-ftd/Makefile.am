#
#  Copyright (c) 2020, The OpenThread Authors.
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
include $(top_srcdir)/examples/platforms/Makefile.platform.am

override CFLAGS   := $(filter-out -Wconversion,$(CFLAGS))
override CXXFLAGS := $(filter-out -Wconversion,$(CXXFLAGS))

bin_PROGRAMS                                                                  = \
    $(NULL)

CPPFLAGS_COMMON                                                              += \
    -DNVIC_CONFIG=\"platform/base/hal/micro/cortexm3/efm32/nvic-config.h\"      \
    -DPLATFORM_HEADER=\"platform/base/hal/micro/cortexm3/compiler/gcc.h\"       \
    -Wno-sign-compare                                                           \
    -I$(top_srcdir)/examples/platforms                                          \
    -I$(top_srcdir)/examples/platforms/efr32/$(PLATFORM_LOWERCASE)/$(BOARD_LOWERCASE) \
    -I$(top_srcdir)/examples/platforms/efr32/$(PLATFORM_LOWERCASE)              \
    -I$(top_srcdir)/examples/platforms/efr32/src                                \
    -I$(top_srcdir)/include                                                     \
    -I$(top_srcdir)/src/core                                                    \
    -I$(top_srcdir)/third_party/silabs/rail_config                              \
    $(SILABS_GSDK_CPPFLAGS)                                                     \
    -Wno-unused-parameter                                                       \
    -Wno-missing-field-initializers                                             \
    $(NULL)

LDADD_COMMON                                                                 += \
    $(NULL)

LDFLAGS_COMMON                                                               += \
    $(NULL)

LIBTOOLFLAGS_COMMON                                                          += \
    $(NULL)

SOURCES_COMMON                                                               += \
    main.c                                                                      \
    $(NULL)

if OPENTHREAD_ENABLE_EXECUTABLE
bin_PROGRAMS                                                                 += \
    sleepy-demo-ftd                                                             \
    $(NULL)
endif


# NOTE: The order of the -I flags should always be in order from part-specific to general.
#  This will make sure the correct header files are used for a part.
sleepy_demo_ftd_CPPFLAGS = $(NULL)

if OPENTHREAD_EXAMPLES_EFR32MG1
sleepy_demo_ftd_CPPFLAGS                                                     += \
    $(SILABS_EFR32MG1_CPPFLAGS)
endif

if OPENTHREAD_EXAMPLES_EFR32MG12
sleepy_demo_ftd_CPPFLAGS                                                     += \
    $(SILABS_EFR32MG12_CPPFLAGS)
endif

if OPENTHREAD_EXAMPLES_EFR32MG13
sleepy_demo_ftd_CPPFLAGS                                                     += \
    $(SILABS_EFR32MG13_CPPFLAGS)
endif

if OPENTHREAD_EXAMPLES_EFR32MG21
sleepy_demo_ftd_CPPFLAGS                                                     += \
    $(SILABS_EFR32MG21_CPPFLAGS)
endif

sleepy_demo_ftd_CPPFLAGS += $(CPPFLAGS_COMMON)



sleepy_demo_ftd_LDADD                                                         = \
    $(top_builddir)/src/cli/libopenthread-cli-ftd.a                             \
    $(top_builddir)/src/core/libopenthread-ftd.a                                \
    $(LDADD_COMMON)                                                             \
    $(top_builddir)/src/core/libopenthread-ftd.a                                \
    $(LDADD_COMMON)                                                             \
    $(NULL)

sleepy_demo_ftd_LDFLAGS                                                       = \
    $(LDFLAGS_COMMON)                                                           \
    $(NULL)

sleepy_demo_ftd_LIBTOOLFLAGS                                                  = \
    $(LIBTOOLFLAGS_COMMON)                                                      \
    $(NULL)

sleepy_demo_ftd_SOURCES                                                       = \
    $(SOURCES_COMMON)                                                           \
    $(NULL)

if OPENTHREAD_ENABLE_LINKER_MAP
sleepy_demo_ftd_LDFLAGS += -Wl,-Map=sleepy-demo-ftd.map
endif

if OPENTHREAD_BUILD_COVERAGE
CPPFLAGS_COMMON                                                              += \
    -DOPENTHREAD_ENABLE_COVERAGE                                                \
    $(NULL)

CLEANFILES = $(wildcard *.gcda *.gcno)
endif # OPENTHREAD_BUILD_COVERAGE

include $(abs_top_nlbuild_autotools_dir)/automake/post.am
