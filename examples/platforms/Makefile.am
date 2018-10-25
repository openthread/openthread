#
#  Copyright (c) 2017, The OpenThread Authors.
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

DIST_SUBDIRS                            = \
    cc1352                                \
    cc2538                                \
    cc2650                                \
    cc2652                                \
    da15000                               \
    efr32                                 \
    emsk                                  \
    gp712                                 \
    kw41z                                 \
    nrf52840                              \
    posix                                 \
    samr21                                \
    utils                                 \
    $(NULL)

# Always build (e.g. for 'make all') these subdirectories.

SUBDIRS                                 = \
    utils                                 \
    $(NULL)

if OPENTHREAD_EXAMPLES_CC1352
SUBDIRS                                += cc1352
endif

if OPENTHREAD_EXAMPLES_CC2538
SUBDIRS                                += cc2538
endif

if OPENTHREAD_EXAMPLES_CC2650
SUBDIRS                                += cc2650
endif

if OPENTHREAD_EXAMPLES_CC2652
SUBDIRS                                += cc2652
endif

if OPENTHREAD_EXAMPLES_DA15000
SUBDIRS                                += da15000
endif

if OPENTHREAD_EXAMPLES_EFR32
SUBDIRS                                += efr32
endif

if OPENTHREAD_EXAMPLES_EMSK
SUBDIRS                                += emsk
endif

if OPENTHREAD_EXAMPLES_GP712
SUBDIRS                                += gp712
endif

if OPENTHREAD_EXAMPLES_KW41Z
SUBDIRS                                += kw41z
endif

if OPENTHREAD_EXAMPLES_NRF52840
SUBDIRS                                += nrf52840
endif

if OPENTHREAD_EXAMPLES_POSIX
SUBDIRS                                += posix
endif

if OPENTHREAD_EXAMPLES_SAMR21
SUBDIRS                                += samr21
endif

noinst_HEADERS                          = \
    openthread-system.h                   \
    $(NULL)

# Always pretty (e.g. for 'make pretty') these subdirectories.

PRETTY_SUBDIRS                          = \
    cc1352                                \
    cc2538                                \
    cc2650                                \
    cc2652                                \
    da15000                               \
    efr32                                 \
    emsk                                  \
    gp712                                 \
    kw41z                                 \
    nrf52840                              \
    posix                                 \
    samr21                                \
    utils                                 \
    $(NULL)

include $(abs_top_nlbuild_autotools_dir)/automake/post.am
