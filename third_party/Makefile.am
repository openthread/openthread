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

EXTRA_DIST                              = \
    nxp                                   \
    nlbuild-autotools                     \
    openthread-test-driver                \
    NordicSemiconductor                   \
    silabs                                \
    $(NULL)

# Always package (e.g. for 'make dist') these subdirectories.

DIST_SUBDIRS                            = \
    jlink                                 \
    mbedtls                               \
    $(NULL)

SUBDIRS =

if OPENTHREAD_PLATFORM_EFR32MG12
SUBDIRS                                += \
    silabs                                \
    jlink                                 \
    $(NULL)
endif

if OPENTHREAD_PLATFORM_EFR32MG13
SUBDIRS                                += \
    silabs                                \
    jlink                                 \
    $(NULL)
endif

if OPENTHREAD_PLATFORM_EFR32MG21
SUBDIRS                                += \
    silabs                                \
    jlink                                 \
    $(NULL)
endif

if OPENTHREAD_PLATFORM_NRF52811
SUBDIRS                                += \
    NordicSemiconductor                   \
    $(NULL)
endif

if OPENTHREAD_EXAMPLES_NRF52811
SUBDIRS                                += \
    jlink                                 \
    $(NULL)
endif

if OPENTHREAD_PLATFORM_NRF52833
SUBDIRS                                += \
    NordicSemiconductor                   \
    $(NULL)
endif

if OPENTHREAD_EXAMPLES_NRF52833
SUBDIRS                                += \
    jlink                                 \
    $(NULL)
endif

if OPENTHREAD_PLATFORM_NRF52840
SUBDIRS                                += \
    NordicSemiconductor                   \
    $(NULL)
endif

if OPENTHREAD_EXAMPLES_NRF52840
SUBDIRS                                += \
    jlink                                 \
    $(NULL)
endif

if OPENTHREAD_ENABLE_BUILTIN_MBEDTLS
SUBDIRS                                += \
    mbedtls                               \
    $(NULL)
endif # OPENTHREAD_ENABLE_BUILTIN_MBEDTLS

include $(abs_top_nlbuild_autotools_dir)/automake/post.am
