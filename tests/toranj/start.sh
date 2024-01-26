#!/bin/bash
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

die()
{
    echo " *** ERROR: " "$*"
    exit 1
}

cleanup()
{
    # Clear logs and flash files
    sudo rm tmp/*.flash tmp/*.data tmp/*.swap >/dev/null 2>&1
    sudo rm ./*.log >/dev/null 2>&1

    if [ "$TORANJ_CLI" != 1 ]; then
        # Clear any wpantund instances
        sudo killall wpantund >/dev/null 2>&1

        while read -r interface; do
            sudo ip link delete "$interface" >/dev/null 2>&1
        done < <(ifconfig 2>/dev/null | grep -o "wpan[0-9]*")
    fi

    sleep 0.3
}

run()
{
    counter=0
    while true; do
        if sudo -E "${python_app}" "$1"; then
            cleanup
            return
        fi

        # We allow a failed test to be retried up to 7 attempts.
        if [ "$counter" -lt 7 ]; then
            counter=$((counter + 1))
            echo Attempt $counter running "$1" failed. Trying again.
            cleanup
            sleep 10
            continue
        fi

        echo " *** TEST FAILED"
        tail -n 40 "${log_file_name}"*.log
        exit 1
    done
}

install_wpantund()
{
    echo "Installing wpantund"
    sudo apt-get --no-install-recommends install -y dbus libdbus-1-dev
    sudo apt-get --no-install-recommends install -y autoconf-archive
    sudo apt-get --no-install-recommends install -y libarchive-tools
    sudo apt-get --no-install-recommends install -y libtool
    sudo apt-get --no-install-recommends install -y libglib2.0-dev
    sudo apt-get --no-install-recommends install -y lcov

    sudo add-apt-repository universe
    sudo apt-get update
    sudo apt-get --no-install-recommends install -y libboost-all-dev python2

    git clone --depth=1 --branch=master https://github.com/openthread/wpantund.git || die "wpandtund clone"
    cd wpantund || die "cd wpantund failed"
    ./bootstrap.sh
    ./configure
    sudo make -j2 || die "wpantund make failed"
    sudo make install || die "wpantund make install failed"
    cd .. || die "cd .. failed"
}

cd "$(dirname "$0")" || die "cd failed"

if [ "$TORANJ_NCP" = 1 ]; then
    echo "========================================================================"
    echo "Running toranj-ncp triggered by event ${TORANJ_EVENT_NAME}"

    if [ "$TORANJ_EVENT_NAME" = "pull_request" ]; then
        cd ../..
        OT_SHA_OLD="$(git cat-file -p HEAD | grep 'parent ' | head -n1 | cut -d' ' -f2)"
        git fetch --depth 1 --no-recurse-submodules origin "${OT_SHA_OLD}"
        if git diff --name-only --exit-code "${OT_SHA_OLD}" -- src/ncp; then
            echo "No changes to any of src/ncp files - skip running tests."
            echo "========================================================================"
            exit 0
        fi
        cd tests/toranj || die "cd tests/toranj failed"
        echo "There are change in src/ncp files, running toranj-ncp tests"
    fi

    echo "========================================================================"
    install_wpantund
fi

if [ -z "${top_builddir}" ]; then
    top_builddir=.
fi

if [ "$COVERAGE" = 1 ]; then
    coverage_option="--enable-coverage"
else
    coverage_option=""
fi

if [ "$TORANJ_CLI" = 1 ]; then
    app_name="cli"
    python_app="python3"
    log_file_name="ot-logs"
else
    app_name="ncp"
    python_app="python2"
    log_file_name="wpantund-logs"
fi

if [ "$TORANJ_RADIO" = "multi" ]; then
    # Build all combinations
    ./build.sh "${coverage_option}" "${app_name}"-15.4 || die "${app_name}-15.4 build failed"
    ./build.sh "${coverage_option}" "${app_name}"-trel || die "${app_name}-trel build failed"
    ./build.sh "${coverage_option}" "${app_name}"-15.4+trel || die "${app_name}-15.4+trel build failed"
else
    ./build.sh "${coverage_option}" "${app_name}"-"${TORANJ_RADIO}" || die "build failed"
fi

cleanup

if [ "$TORANJ_CLI" = 1 ]; then

    if [ "$TORANJ_RADIO" = "multi" ]; then
        run cli/test-700-multi-radio-join.py
        run cli/test-701-multi-radio-probe.py
        run cli/test-702-multi-radio-discover-by-rx.py
        run cli/test-703-multi-radio-mesh-header-msg.py
        run cli/test-704-multi-radio-scan.py
        run cli/test-705-multi-radio-discover-scan.py

        exit 0
    fi

    run cli/test-001-get-set.py
    run cli/test-002-form.py
    run cli/test-003-join.py
    run cli/test-004-scan.py
    run cli/test-005-traffic-router-to-child.py
    run cli/test-006-traffic-multi-hop.py
    run cli/test-007-off-mesh-route-traffic.py
    run cli/test-008-multicast-traffic.py
    run cli/test-009-router-table.py
    run cli/test-010-partition-merge.py
    run cli/test-011-network-data-timeout.py
    run cli/test-012-reset-recovery.py
    run cli/test-013-address-cache-parent-switch.py
    run cli/test-014-address-resolver.py
    run cli/test-015-clear-addresss-cache-for-sed.py
    run cli/test-016-child-mode-change.py
    run cli/test-017-network-data-versions.py
    run cli/test-018-next-hop-and-path-cost.py
    run cli/test-019-netdata-context-id.py
    run cli/test-020-net-diag-vendor-info.py
    run cli/test-021-br-route-prf.py
    run cli/test-022-netdata-full.py
    run cli/test-023-mesh-diag.py
    run cli/test-024-mle-adv-imax-change.py
    run cli/test-025-mesh-local-prefix-change.py
    run cli/test-026-coaps-conn-limit.py
    run cli/test-400-srp-client-server.py
    run cli/test-601-channel-manager-channel-change.py
    # Skip the "channel-select" test on a TREL only radio link, since it
    # requires energy scan which is not supported in this case.
    if [ "$TORANJ_RADIO" != "trel" ]; then
        run cli/test-602-channel-manager-channel-select.py
    fi
    run cli/test-603-channel-announce-recovery.py

    exit 0
fi

if [ "$TORANJ_RADIO" = "multi" ]; then
    run ncp/test-700-multi-radio-join.py
    run ncp/test-701-multi-radio-probe.py
    run ncp/test-702-multi-radio-discovery-by-rx.py
    run ncp/test-703-multi-radio-mesh-header-msg.py
    run ncp/test-704-multi-radio-scan.py
    run ncp/test-705-multi-radio-discover-scan.py

    exit 0
fi

run ncp/test-001-get-set.py
run ncp/test-002-form.py
run ncp/test-003-join.py
run ncp/test-004-scan.py
run ncp/test-005-discover-scan.py
run ncp/test-006-traffic-router-end-device.py
run ncp/test-007-traffic-router-sleepy.py
run ncp/test-009-insecure-traffic-join.py
run ncp/test-010-on-mesh-prefix-config-gateway.py
run ncp/test-011-child-table.py
run ncp/test-012-multi-hop-traffic.py
run ncp/test-013-off-mesh-route-traffic.py
run ncp/test-014-ip6-address-add.py
run ncp/test-015-same-prefix-on-multiple-nodes.py
run ncp/test-016-neighbor-table.py
run ncp/test-017-parent-reset-child-recovery.py
run ncp/test-019-inform-previous-parent.py
run ncp/test-020-router-table.py
run ncp/test-021-address-cache-table.py
run ncp/test-022-multicast-ip6-address.py
run ncp/test-023-multicast-traffic.py
run ncp/test-024-partition-merge.py
run ncp/test-025-network-data-timeout.py
run ncp/test-026-slaac-address-wpantund.py
run ncp/test-027-child-mode-change.py
run ncp/test-028-router-leader-reset-recovery.py
run ncp/test-029-data-poll-interval.py
run ncp/test-030-slaac-address-ncp.py
run ncp/test-031-meshcop-joiner-commissioner.py
run ncp/test-032-child-attach-with-multiple-ip-addresses.py
run ncp/test-033-mesh-local-prefix-change.py
run ncp/test-034-poor-link-parent-child-attach.py
run ncp/test-035-child-timeout-large-data-poll.py
run ncp/test-036-wpantund-host-route-management.py
run ncp/test-037-wpantund-auto-add-route-for-on-mesh-prefix.py
run ncp/test-038-clear-address-cache-for-sed.py
run ncp/test-039-address-cache-table-snoop.py
run ncp/test-040-network-data-stable-full.py
run ncp/test-041-lowpan-fragmentation.py
run ncp/test-042-meshcop-joiner-discerner.py
run ncp/test-043-meshcop-joiner-router.py
run ncp/test-100-mcu-power-state.py
run ncp/test-600-channel-manager-properties.py
run ncp/test-601-channel-manager-channel-change.py

# Skip the "channel-select" test on a TREL only radio link, since it
# requires energy scan which is not supported in this case.

if [ "$TORANJ_RADIO" != "trel" ]; then
    run ncp/test-602-channel-manager-channel-select.py
fi

run ncp/test-603-channel-manager-announce-recovery.py

exit 0
