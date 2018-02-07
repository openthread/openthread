#!/bin/sh
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

die() {
    echo " *** ERROR: " $*
    exit 1
}

failed() {
    echo " *** TEST FAILED: ERROR: " $*
    tail -n 30 wpantund-logs*.log
    exit 1
}

clean() {
    sudo rm tmp/*.flash > /dev/null 2>&1
    sudo rm *.log > /dev/null 2>&1
}

cd $(dirname $0)
cd ../..

# Build OpenThread posix mode with required configuration

./bootstrap || die
./configure                             \
    CPPFLAGS='-DOPENTHREAD_PROJECT_CORE_CONFIG_FILE=\"../tests/toranj/openthread-core-toranj-config.h\"' \
    --enable-ncp-app=all                \
    --with-ncp-bus=uart                 \
    --with-examples=posix               \
    --enable-border-router              \
    --enable-child-supervision          \
    --enable-diag                       \
    --enable-jam-detection              \
    --enable-legacy                     \
    --enable-mac-filter                 \
    --enable-service                    \
    --enable-channel-monitor            \
    --disable-docs                      \
    --disable-test || die

make -j 8 || die

# Run all the tests

cd tests/toranj

clean; sudo python test-001-get-set.py                                   || failed
clean; sudo python test-002-form.py                                      || failed
clean; sudo python test-003-join.py                                      || failed
clean; sudo python test-004-scan.py                                      || failed
clean; sudo python test-005-discover-scan.py                             || failed
clean; sudo python test-006-traffic-router-end-device.py                 || failed
clean; sudo python test-007-traffic-router-sleepy.py                     || failed
clean; sudo python test-008-permit-join.py                               || failed
