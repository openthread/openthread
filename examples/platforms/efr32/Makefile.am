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

# Use automake includes since we cannot use SUBDIRS feature due to cleanup
# errors - few targets may use the same source file but dependency file is
# created only once which leads to errors when auto-generated Makefile tries
# to remove .Po files that were already removed


# Always package (e.g. for 'make dist') these subdirectories.
DIST_SUBDIRS = sleepy-demo

# Always build (e.g. for 'make all') these subdirectories.
SUBDIRS = $(NULL)

if OPENTHREAD_EXAMPLES_EFR32MG1
include efr32mg1/Makefile.am
# SUBDIRS = sleepy-demo
# NOTE: The sleepy-demo app currently does not fit on this platform.
#
# .../arm-none-eabi/bin/ld: sleepy-demo-mtd section `.bss' will not fit in region `RAM'
# .../arm-none-eabi/bin/ld: sleepy-demo-mtd section `.nvm' will not fit in region `FLASH'
# .../arm-none-eabi/bin/ld: region RAM overflowed with stack
# .../arm-none-eabi/bin/ld: region `FLASH' overflowed by 33428 bytes
# .../arm-none-eabi/bin/ld: region `RAM' overflowed by 4536 bytes

else # not OPENTHREAD_EXAMPLES_EFR32MG1

if OPENTHREAD_EXAMPLES_EFR32MG12
include efr32mg12/Makefile.am
endif

if OPENTHREAD_EXAMPLES_EFR32MG13
include efr32mg13/Makefile.am
endif

if OPENTHREAD_EXAMPLES_EFR32MG21
include efr32mg21/Makefile.am
endif

if OPENTHREAD_ENABLE_EXECUTABLE
SUBDIRS += sleepy-demo
endif

endif # OPENTHREAD_EXAMPLES_EFR32MG1
