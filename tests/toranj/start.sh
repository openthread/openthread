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

cleanup() {
    # Clear logs and flash files
    sudo rm tmp/*.flash tmp/*.data tmp/*.swap > /dev/null 2>&1
    sudo rm *.log > /dev/null 2>&1

    # Clear any wpantund instances
    sudo killall wpantund > /dev/null 2>&1

    wpan_interfaces=$(ifconfig 2>/dev/null | grep -o wpan[0-9]*)

    for interface in $wpan_interfaces; do
        sudo ip link delete $interface > /dev/null 2>&1
    done

    sleep 0.3
}

run() {
    counter=0
    while true; do

        if sudo -E python $1; then
            cleanup
            return
        fi

        # On Travis, we allow a failed test to be retried up to 3 attempts.
        if [ "$BUILD_TARGET" = "toranj-test-framework" ]; then
            if [ "$counter" -lt 2 ]; then
                counter=$((counter+1))
                echo Attempt $counter running "$1" failed. Trying again.
                cleanup
                sleep 10
                continue
            fi
        fi

        echo " *** TEST FAILED"
        tail -n 40 wpantund-logs*.log
        exit 1
    done
}

cd $(dirname $0)

# On Travis CI, the $BUILD_TARGET is defined as "toranj-test-framework".
if [ "$BUILD_TARGET" = "toranj-test-framework" ]; then
    coverage_option="--enable-coverage"
else
    coverage_option=""
fi

case $TORANJ_POSIX_APP_RCP_MODEL in
    1|yes)
        use_posix_app_with_rcp=yes
        ;;
    *)
        use_posix_app_with_rcp=no
        ;;
esac

if [ "$use_posix_app_with_rcp" = "no" ]; then
    ./build.sh ${coverage_option} ncp || die

else
    ./build.sh ${coverage_option} rcp || die
    ./build.sh ${coverage_option} posix-app || die
fi

cleanup

run test-001-get-set.py
run test-002-form.py
run test-003-join.py
run test-004-scan.py
run test-005-discover-scan.py
run test-006-traffic-router-end-device.py
run test-007-traffic-router-sleepy.py
run test-008-permit-join.py
run test-009-insecure-traffic-join.py
run test-010-on-mesh-prefix-config-gateway.py
run test-011-child-table.py
run test-012-multi-hop-traffic.py
run test-013-off-mesh-route-traffic.py
run test-014-ip6-address-add.py
run test-015-same-prefix-on-multiple-nodes.py
run test-016-neighbor-table.py
run test-017-parent-reset-child-recovery.py
run test-018-child-supervision.py
run test-019-inform-previous-parent.py
run test-020-router-table.py
run test-021-address-cache-table.py
run test-022-multicast-ip6-address.py
run test-023-multicast-traffic.py
run test-024-partition-merge.py
run test-025-network-data-timeout.py
run test-026-slaac-address.py
run test-027-child-mode-change.py
run test-028-router-leader-reset-recovery.py
run test-100-mcu-power-state.py
run test-600-channel-manager-properties.py
run test-601-channel-manager-channel-change.py
run test-602-channel-manager-channel-select.py
run test-603-channel-manager-announce-recovery.py

exit 0
