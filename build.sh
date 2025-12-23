#!/bin/bash

set -euo pipefail

build_for_expect_test() {
    ./script/cmake-build simulation \
        -DBUILD_TESTING=OFF \
        -DOT_BUILD_GTEST=OFF \
        -DOT_CHANNEL_MANAGER=OFF \
        -DOT_CHANNEL_MONITOR=OFF \
        -DOT_CLI_LOG=ON \
        -DOT_CSL_RECEIVER=ON \
        -DOT_FULL_LOGS=ON \
        -DOT_LOG_LEVEL=DEBG \
        -DOT_LOG_MAX_SIZE=1024 \
        -DOT_LOG_OUTPUT=PLATFORM_DEFINED \
        -DOT_PEER_TO_PEER=ON \
        -DOT_SRP_SERVER_FAST_START_MDOE=ON \
        -DOT_WAKEUP_COORDINATOR=ON \
        -DOT_WAKEUP_END_DEVICE=ON \
        -DOT_COEX=ON \
        -DOT_ECSL_RECEIVER=ON \
        -DOT_ECSL_TRANSMITTER=ON
}

build_check() {
    ./script/cmake-build simulation

    ./script/cmake-build posix \
        -DBUILD_TESTING=OFF \
        -DOT_BUILD_GTEST=OFF \
        -DOT_CHANNEL_MANAGER=OFF \
        -DOT_CHANNEL_MONITOR=OFF \
        -DOT_CLI_LOG=ON \
        -DOT_FULL_LOGS=ON \
        -DOT_LOG_LEVEL=DEBG \
        -DOT_LOG_MAX_SIZE=1024 \
        -DOT_LOG_OUTPUT=PLATFORM_DEFINED \
        -DOT_PEER_TO_PEER=ON \
        -DOT_SRP_SERVER_FAST_START_MDOE=ON \
        -DOT_ECSL_TRANSMITTER=ON \
        -DOT_WAKEUP_COORDINATOR=ON
}

main()
{
    if [[ $# < 1 ]]; then
        build_for_expect_test
        exit 1
    fi

    if [[ "$1" == "check" ]]; then
        build_check
    fi
}

main "$@"
