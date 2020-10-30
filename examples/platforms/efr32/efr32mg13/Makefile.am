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
include $(top_srcdir)/examples/platforms/efr32/efr32_platform_defs.am

lib_LIBRARIES = libopenthread-efr32mg13.a

libopenthread_efr32mg13_a_CPPFLAGS                                            = \
    -DPLATFORM_HEADER=\"platform/base/hal/micro/cortexm3/compiler/gcc.h\"       \
    -DNVIC_CONFIG=\"platform/base/hal/micro/cortexm3/efm32/nvic-config.h\"      \
    -Wno-sign-compare                                                           \
    -I$(top_srcdir)/examples/platforms                                          \
    -I$(top_srcdir)/examples/platforms/efr32/$(PLATFORM_LOWERCASE)/$(BOARD_LOWERCASE) \
    -I$(top_srcdir)/examples/platforms/efr32/$(PLATFORM_LOWERCASE)              \
    -I$(top_srcdir)/examples/platforms/efr32/src                                \
    -I$(top_srcdir)/include                                                     \
    -I$(top_srcdir)/src/core                                                    \
    -I$(top_srcdir)/third_party/silabs/rail_config                              \
    $(SILABS_EFR32MG13_CPPFLAGS)                                                \
    $(SILABS_GSDK_CPPFLAGS)                                                     \
    -Wno-unused-parameter                                                       \
    -Wno-missing-field-initializers                                             \
    $(NULL)

PLATFORM_SOURCES                                                              = \
    $(NULL)

noinst_HEADERS                                                                = \
    src/platform-band.h                                                         \
    src/platform-efr32.h                                                        \
    $(NULL)

libopenthread_efr32mg13_a_SOURCES                                             = \
    $(PLATFORM_COMMON_SOURCES_LIST)                                             \
    $(PLATFORM_SOURCES)                                                         \
    $(NULL)

Dash = -
libopenthread_efr32mg13_a_LIBADD                                                                     = \
    $(shell find $(top_builddir)/examples/platforms/utils $(Dash)type f $(Dash)name "*.o")             \
    $(shell find $(top_builddir)/third_party/jlink/SEGGER_RTT_V640/RTT $(Dash)type f $(Dash)name "*.o")

include $(abs_top_nlbuild_autotools_dir)/automake/post.am
