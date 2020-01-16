#
#  Copyright (c) 2019, The OpenThread Authors.
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

# Use automake includes since we cannot use SUBDIRS feature due to  cleanup
# errors - few targets may use the same source file but dependency file is 
# created only once which leads to errors when auto-generated Makefile tries
# to remove .Po files that were already removed

if OPENTHREAD_PLATFORM_NRF52811
include nrf52811/Makefile.am
endif

if OPENTHREAD_PLATFORM_NRF52833
include nrf52833/Makefile.am
endif

if OPENTHREAD_PLATFORM_NRF52840
include nrf52840/Makefile.am
endif

include $(abs_top_nlbuild_autotools_dir)/automake/pre.am

PRETTY_FILES                            = \
    src/platform-fem.h                    \
    src/platform-nrf5.h                   \
    $(wildcard $(srcdir)/**/*.c)          \
    $(wildcard $(srcdir)/**/*.h)          \
    $(NULL)

include $(abs_top_nlbuild_autotools_dir)/automake/post.am
