#! /usr/bin/env sh

# all.sh
#
# Copyright The Mbed TLS Contributors
# SPDX-License-Identifier: Apache-2.0
#
# Licensed under the Apache License, Version 2.0 (the "License"); you may
# not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
# http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
# WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.



################################################################
#### Documentation
################################################################

# Purpose
# -------
#
# To run all tests possible or available on the platform.
#
# Notes for users
# ---------------
#
# Warning: the test is destructive. It includes various build modes and
# configurations, and can and will arbitrarily change the current CMake
# configuration. The following files must be committed into git:
#    * include/mbedtls/config.h
#    * Makefile, library/Makefile, programs/Makefile, tests/Makefile,
#      programs/fuzz/Makefile
# After running this script, the CMake cache will be lost and CMake
# will no longer be initialised.
#
# The script assumes the presence of a number of tools:
#   * Basic Unix tools (Windows users note: a Unix-style find must be before
#     the Windows find in the PATH)
#   * Perl
#   * GNU Make
#   * CMake
#   * GCC and Clang (recent enough for using ASan with gcc and MemSan with clang, or valgrind)
#   * G++
#   * arm-gcc and mingw-gcc
#   * ArmCC 5 and ArmCC 6, unless invoked with --no-armcc
#   * OpenSSL and GnuTLS command line tools, recent enough for the
#     interoperability tests. If they don't support SSLv3 then a legacy
#     version of these tools must be present as well (search for LEGACY
#     below).
# See the invocation of check_tools below for details.
#
# This script must be invoked from the toplevel directory of a git
# working copy of Mbed TLS.
#
# Note that the output is not saved. You may want to run
#   script -c tests/scripts/all.sh
# or
#   tests/scripts/all.sh >all.log 2>&1
#
# Notes for maintainers
# ---------------------
#
# The bulk of the code is organized into functions that follow one of the
# following naming conventions:
#  * pre_XXX: things to do before running the tests, in order.
#  * component_XXX: independent components. They can be run in any order.
#      * component_check_XXX: quick tests that aren't worth parallelizing.
#      * component_build_XXX: build things but don't run them.
#      * component_test_XXX: build and test.
#  * support_XXX: if support_XXX exists and returns false then
#    component_XXX is not run by default.
#  * post_XXX: things to do after running the tests.
#  * other: miscellaneous support functions.
#
# Each component must start by invoking `msg` with a short informative message.
#
# The framework performs some cleanup tasks after each component. This
# means that components can assume that the working directory is in a
# cleaned-up state, and don't need to perform the cleanup themselves.
# * Run `make clean`.
# * Restore `include/mbedtks/config.h` from a backup made before running
#   the component.
# * Check out `Makefile`, `library/Makefile`, `programs/Makefile`,
#   `tests/Makefile` and `programs/fuzz/Makefile` from git.
#   This cleans up after an in-tree use of CMake.
#
# Any command that is expected to fail must be protected so that the
# script keeps running in --keep-going mode despite `set -e`. In keep-going
# mode, if a protected command fails, this is logged as a failure and the
# script will exit with a failure status once it has run all components.
# Commands can be protected in any of the following ways:
# * `make` is a function which runs the `make` command with protection.
#   Note that you must write `make VAR=value`, not `VAR=value make`,
#   because the `VAR=value make` syntax doesn't work with functions.
# * Put `report_status` before the command to protect it.
# * Put `if_build_successful` before a command. This protects it, and
#   additionally skips it if a prior invocation of `make` in the same
#   component failed.
#
# The tests are roughly in order from fastest to slowest. This doesn't
# have to be exact, but in general you should add slower tests towards
# the end and fast checks near the beginning.



################################################################
#### Initialization and command line parsing
################################################################

# Abort on errors (and uninitialised variables)
set -eu

pre_check_environment () {
    if [ -d library -a -d include -a -d tests ]; then :; else
        echo "Must be run from mbed TLS root" >&2
        exit 1
    fi
}

pre_initialize_variables () {
    CONFIG_H='include/mbedtls/config.h'
    CONFIG_BAK="$CONFIG_H.bak"
    CRYPTO_CONFIG_H='include/psa/crypto_config.h'
    CRYPTO_CONFIG_BAK="$CRYPTO_CONFIG_H.bak"

    append_outcome=0
    MEMORY=0
    FORCE=0
    QUIET=0
    KEEP_GOING=0

    # Seed value used with the --release-test option.
    #
    # See also RELEASE_SEED in basic-build-test.sh. Debugging is easier if
    # both values are kept in sync. If you change the value here because it
    # breaks some tests, you'll definitely want to change it in
    # basic-build-test.sh as well.
    RELEASE_SEED=1

    : ${MBEDTLS_TEST_OUTCOME_FILE=}
    : ${MBEDTLS_TEST_PLATFORM="$(uname -s | tr -c \\n0-9A-Za-z _)-$(uname -m | tr -c \\n0-9A-Za-z _)"}
    export MBEDTLS_TEST_OUTCOME_FILE
    export MBEDTLS_TEST_PLATFORM

    # Default commands, can be overridden by the environment
    : ${OPENSSL:="openssl"}
    : ${OPENSSL_LEGACY:="$OPENSSL"}
    : ${OPENSSL_NEXT:="$OPENSSL"}
    : ${GNUTLS_CLI:="gnutls-cli"}
    : ${GNUTLS_SERV:="gnutls-serv"}
    : ${GNUTLS_LEGACY_CLI:="$GNUTLS_CLI"}
    : ${GNUTLS_LEGACY_SERV:="$GNUTLS_SERV"}
    : ${OUT_OF_SOURCE_DIR:=./mbedtls_out_of_source_build}
    : ${ARMC5_BIN_DIR:=/usr/bin}
    : ${ARMC6_BIN_DIR:=/usr/bin}
    : ${ARM_NONE_EABI_GCC_PREFIX:=arm-none-eabi-}

    # if MAKEFLAGS is not set add the -j option to speed up invocations of make
    if [ -z "${MAKEFLAGS+set}" ]; then
        export MAKEFLAGS="-j"
    fi

    # Include more verbose output for failing tests run by CMake
    export CTEST_OUTPUT_ON_FAILURE=1

    # CFLAGS and LDFLAGS for Asan builds that don't use CMake
    ASAN_CFLAGS='-Werror -Wall -Wextra -fsanitize=address,undefined -fno-sanitize-recover=all'

    # Gather the list of available components. These are the functions
    # defined in this script whose name starts with "component_".
    # Parse the script with sed, because in sh there is no way to list
    # defined functions.
    ALL_COMPONENTS=$(sed -n 's/^ *component_\([0-9A-Z_a-z]*\) *().*/\1/p' <"$0")

    # Exclude components that are not supported on this platform.
    SUPPORTED_COMPONENTS=
    for component in $ALL_COMPONENTS; do
        case $(type "support_$component" 2>&1) in
            *' function'*)
                if ! support_$component; then continue; fi;;
        esac
        SUPPORTED_COMPONENTS="$SUPPORTED_COMPONENTS $component"
    done
}

# Test whether the component $1 is included in the command line patterns.
is_component_included()
{
    set -f
    for pattern in $COMMAND_LINE_COMPONENTS; do
        set +f
        case ${1#component_} in $pattern) return 0;; esac
    done
    set +f
    return 1
}

usage()
{
    cat <<EOF
Usage: $0 [OPTION]... [COMPONENT]...
Run mbedtls release validation tests.
By default, run all tests. With one or more COMPONENT, run only those.
COMPONENT can be the name of a component or a shell wildcard pattern.

Examples:
  $0 "check_*"
    Run all sanity checks.
  $0 --no-armcc --except test_memsan
    Run everything except builds that require armcc and MemSan.

Special options:
  -h|--help             Print this help and exit.
  --list-all-components List all available test components and exit.
  --list-components     List components supported on this platform and exit.

General options:
  -q|--quiet            Only output component names, and errors if any.
  -f|--force            Force the tests to overwrite any modified files.
  -k|--keep-going       Run all tests and report errors at the end.
  -m|--memory           Additional optional memory tests.
     --append-outcome   Append to the outcome file (if used).
     --arm-none-eabi-gcc-prefix=<string>
                        Prefix for a cross-compiler for arm-none-eabi
                        (default: "${ARM_NONE_EABI_GCC_PREFIX}")
     --armcc            Run ARM Compiler builds (on by default).
     --except           Exclude the COMPONENTs listed on the command line,
                        instead of running only those.
     --no-append-outcome    Write a new outcome file and analyze it (default).
     --no-armcc         Skip ARM Compiler builds.
     --no-force         Refuse to overwrite modified files (default).
     --no-keep-going    Stop at the first error (default).
     --no-memory        No additional memory tests (default).
     --no-quiet         Print full ouput from components.
     --out-of-source-dir=<path>  Directory used for CMake out-of-source build tests.
     --outcome-file=<path>  File where test outcomes are written (not done if
                            empty; default: \$MBEDTLS_TEST_OUTCOME_FILE).
     --random-seed      Use a random seed value for randomized tests (default).
  -r|--release-test     Run this script in release mode. This fixes the seed value to ${RELEASE_SEED}.
  -s|--seed             Integer seed value to use for this test run.

Tool path options:
     --armc5-bin-dir=<ARMC5_bin_dir_path>       ARM Compiler 5 bin directory.
     --armc6-bin-dir=<ARMC6_bin_dir_path>       ARM Compiler 6 bin directory.
     --gnutls-cli=<GnuTLS_cli_path>             GnuTLS client executable to use for most tests.
     --gnutls-serv=<GnuTLS_serv_path>           GnuTLS server executable to use for most tests.
     --gnutls-legacy-cli=<GnuTLS_cli_path>      GnuTLS client executable to use for legacy tests.
     --gnutls-legacy-serv=<GnuTLS_serv_path>    GnuTLS server executable to use for legacy tests.
     --openssl=<OpenSSL_path>                   OpenSSL executable to use for most tests.
     --openssl-legacy=<OpenSSL_path>            OpenSSL executable to use for legacy tests e.g. SSLv3.
     --openssl-next=<OpenSSL_path>              OpenSSL executable to use for recent things like ARIA
EOF
}

# remove built files as well as the cmake cache/config
cleanup()
{
    if [ -n "${MBEDTLS_ROOT_DIR+set}" ]; then
        cd "$MBEDTLS_ROOT_DIR"
    fi

    command make clean

    # Remove CMake artefacts
    find . -name .git -prune -o \
           -iname CMakeFiles -exec rm -rf {} \+ -o \
           \( -iname cmake_install.cmake -o \
              -iname CTestTestfile.cmake -o \
              -iname CMakeCache.txt \) -exec rm {} \+
    # Recover files overwritten by in-tree CMake builds
    rm -f include/Makefile include/mbedtls/Makefile programs/*/Makefile
    git update-index --no-skip-worktree Makefile library/Makefile programs/Makefile tests/Makefile programs/fuzz/Makefile
    git checkout -- Makefile library/Makefile programs/Makefile tests/Makefile programs/fuzz/Makefile

    # Remove any artifacts from the component_test_cmake_as_subdirectory test.
    rm -rf programs/test/cmake_subproject/build
    rm -f programs/test/cmake_subproject/Makefile
    rm -f programs/test/cmake_subproject/cmake_subproject

    if [ -f "$CONFIG_BAK" ]; then
        mv "$CONFIG_BAK" "$CONFIG_H"
    fi

    if [ -f "$CRYPTO_CONFIG_BAK" ]; then
        mv "$CRYPTO_CONFIG_BAK" "$CRYPTO_CONFIG_H"
    fi
}

# Executed on exit. May be redefined depending on command line options.
final_report () {
    :
}

fatal_signal () {
    cleanup
    final_report $1
    trap - $1
    kill -$1 $$
}

trap 'fatal_signal HUP' HUP
trap 'fatal_signal INT' INT
trap 'fatal_signal TERM' TERM

msg()
{
    if [ -n "${current_component:-}" ]; then
        current_section="${current_component#component_}: $1"
    else
        current_section="$1"
    fi

    if [ $QUIET -eq 1 ]; then
        return
    fi

    echo ""
    echo "******************************************************************"
    echo "* $current_section "
    printf "* "; date
    echo "******************************************************************"
}

armc6_build_test()
{
    FLAGS="$1"

    msg "build: ARM Compiler 6 ($FLAGS)"
    ARM_TOOL_VARIANT="ult" CC="$ARMC6_CC" AR="$ARMC6_AR" CFLAGS="$FLAGS" \
                    WARNING_CFLAGS='-xc -std=c99' make lib

    msg "size: ARM Compiler 6 ($FLAGS)"
    "$ARMC6_FROMELF" -z library/*.o

    make clean
}

err_msg()
{
    echo "$1" >&2
}

check_tools()
{
    for TOOL in "$@"; do
        if ! `type "$TOOL" >/dev/null 2>&1`; then
            err_msg "$TOOL not found!"
            exit 1
        fi
    done
}

check_headers_in_cpp () {
    ls include/mbedtls | grep "\.h$" >headers.txt
    <programs/test/cpp_dummy_build.cpp sed -n 's/"$//; s!^#include "mbedtls/!!p' |
    sort |
    diff headers.txt -
    rm headers.txt
}

pre_parse_command_line () {
    COMMAND_LINE_COMPONENTS=
    all_except=0
    no_armcc=

    # Note that legacy options are ignored instead of being omitted from this
    # list of options, so invocations that worked with previous version of
    # all.sh will still run and work properly.
    while [ $# -gt 0 ]; do
        case "$1" in
            --append-outcome) append_outcome=1;;
            --arm-none-eabi-gcc-prefix) shift; ARM_NONE_EABI_GCC_PREFIX="$1";;
            --armcc) no_armcc=;;
            --armc5-bin-dir) shift; ARMC5_BIN_DIR="$1";;
            --armc6-bin-dir) shift; ARMC6_BIN_DIR="$1";;
            --except) all_except=1;;
            --force|-f) FORCE=1;;
            --gnutls-cli) shift; GNUTLS_CLI="$1";;
            --gnutls-legacy-cli) shift; GNUTLS_LEGACY_CLI="$1";;
            --gnutls-legacy-serv) shift; GNUTLS_LEGACY_SERV="$1";;
            --gnutls-serv) shift; GNUTLS_SERV="$1";;
            --help|-h) usage; exit;;
            --keep-going|-k) KEEP_GOING=1;;
            --list-all-components) printf '%s\n' $ALL_COMPONENTS; exit;;
            --list-components) printf '%s\n' $SUPPORTED_COMPONENTS; exit;;
            --memory|-m) MEMORY=1;;
            --no-append-outcome) append_outcome=0;;
            --no-armcc) no_armcc=1;;
            --no-force) FORCE=0;;
            --no-keep-going) KEEP_GOING=0;;
            --no-memory) MEMORY=0;;
            --no-quiet) QUIET=0;;
            --openssl) shift; OPENSSL="$1";;
            --openssl-legacy) shift; OPENSSL_LEGACY="$1";;
            --openssl-next) shift; OPENSSL_NEXT="$1";;
            --outcome-file) shift; MBEDTLS_TEST_OUTCOME_FILE="$1";;
            --out-of-source-dir) shift; OUT_OF_SOURCE_DIR="$1";;
            --quiet|-q) QUIET=1;;
            --random-seed) unset SEED;;
            --release-test|-r) SEED=$RELEASE_SEED;;
            --seed|-s) shift; SEED="$1";;
            -*)
                echo >&2 "Unknown option: $1"
                echo >&2 "Run $0 --help for usage."
                exit 120
                ;;
            *) COMMAND_LINE_COMPONENTS="$COMMAND_LINE_COMPONENTS $1";;
        esac
        shift
    done

    # With no list of components, run everything.
    if [ -z "$COMMAND_LINE_COMPONENTS" ]; then
        all_except=1
    fi

    # --no-armcc is a legacy option. The modern way is --except '*_armcc*'.
    # Ignore it if components are listed explicitly on the command line.
    if [ -n "$no_armcc" ] && [ $all_except -eq 1 ]; then
        COMMAND_LINE_COMPONENTS="$COMMAND_LINE_COMPONENTS *_armcc*"
    fi

    # Build the list of components to run.
    RUN_COMPONENTS=
    for component in $SUPPORTED_COMPONENTS; do
        if is_component_included "$component"; [ $? -eq $all_except ]; then
            RUN_COMPONENTS="$RUN_COMPONENTS $component"
        fi
    done

    unset all_except
    unset no_armcc
}

pre_check_git () {
    if [ $FORCE -eq 1 ]; then
        rm -rf "$OUT_OF_SOURCE_DIR"
        git checkout-index -f -q $CONFIG_H
        cleanup
    else

        if [ -d "$OUT_OF_SOURCE_DIR" ]; then
            echo "Warning - there is an existing directory at '$OUT_OF_SOURCE_DIR'" >&2
            echo "You can either delete this directory manually, or force the test by rerunning"
            echo "the script as: $0 --force --out-of-source-dir $OUT_OF_SOURCE_DIR"
            exit 1
        fi

        if ! git diff --quiet include/mbedtls/config.h; then
            err_msg "Warning - the configuration file 'include/mbedtls/config.h' has been edited. "
            echo "You can either delete or preserve your work, or force the test by rerunning the"
            echo "script as: $0 --force"
            exit 1
        fi
    fi
}

pre_setup_keep_going () {
    failure_summary=
    failure_count=0
    start_red=
    end_color=
    if [ -t 1 ]; then
        case "${TERM:-}" in
            *color*|cygwin|linux|rxvt*|screen|[Eex]term*)
                start_red=$(printf '\033[31m')
                end_color=$(printf '\033[0m')
                ;;
        esac
    fi
    record_status () {
        if "$@"; then
            last_status=0
        else
            last_status=$?
            text="$current_section: $* -> $last_status"
            failure_summary="$failure_summary
$text"
            failure_count=$((failure_count + 1))
            echo "${start_red}^^^^$text^^^^${end_color}" >&2
        fi
    }
    make () {
        case "$*" in
            *test|*check)
                if [ $build_status -eq 0 ]; then
                    record_status command make "$@"
                else
                    echo "(skipped because the build failed)"
                fi
                ;;
            *)
                record_status command make "$@"
                build_status=$last_status
                ;;
        esac
    }
    final_report () {
        if [ $failure_count -gt 0 ]; then
            echo
            echo "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!"
            echo "${start_red}FAILED: $failure_count${end_color}$failure_summary"
            echo "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!"
            exit 1
        elif [ -z "${1-}" ]; then
            echo "SUCCESS :)"
        fi
        if [ -n "${1-}" ]; then
            echo "Killed by SIG$1."
        fi
    }
}

if_build_succeeded () {
    if [ $build_status -eq 0 ]; then
        record_status "$@"
    fi
}

# to be used instead of ! for commands run with
# record_status or if_build_succeeded
not() {
    ! "$@"
}

pre_setup_quiet_redirect () {
    if [ $QUIET -ne 1 ]; then
        redirect_out () {
            "$@"
        }
        redirect_err () {
            "$@"
        }
    else
        redirect_out () {
            "$@" >/dev/null
        }
        redirect_err () {
            "$@" 2>/dev/null
        }
    fi
}

pre_prepare_outcome_file () {
    case "$MBEDTLS_TEST_OUTCOME_FILE" in
      [!/]*) MBEDTLS_TEST_OUTCOME_FILE="$PWD/$MBEDTLS_TEST_OUTCOME_FILE";;
    esac
    if [ -n "$MBEDTLS_TEST_OUTCOME_FILE" ] && [ "$append_outcome" -eq 0 ]; then
        rm -f "$MBEDTLS_TEST_OUTCOME_FILE"
    fi
}

pre_print_configuration () {
    if [ $QUIET -eq 1 ]; then
        return
    fi

    msg "info: $0 configuration"
    echo "MEMORY: $MEMORY"
    echo "FORCE: $FORCE"
    echo "MBEDTLS_TEST_OUTCOME_FILE: ${MBEDTLS_TEST_OUTCOME_FILE:-(none)}"
    echo "SEED: ${SEED-"UNSET"}"
    echo
    echo "OPENSSL: $OPENSSL"
    echo "OPENSSL_LEGACY: $OPENSSL_LEGACY"
    echo "OPENSSL_NEXT: $OPENSSL_NEXT"
    echo "GNUTLS_CLI: $GNUTLS_CLI"
    echo "GNUTLS_SERV: $GNUTLS_SERV"
    echo "GNUTLS_LEGACY_CLI: $GNUTLS_LEGACY_CLI"
    echo "GNUTLS_LEGACY_SERV: $GNUTLS_LEGACY_SERV"
    echo "ARMC5_BIN_DIR: $ARMC5_BIN_DIR"
    echo "ARMC6_BIN_DIR: $ARMC6_BIN_DIR"
}

# Make sure the tools we need are available.
pre_check_tools () {
    # Build the list of variables to pass to output_env.sh.
    set env

    case " $RUN_COMPONENTS " in
        # Require OpenSSL and GnuTLS if running any tests (as opposed to
        # only doing builds). Not all tests run OpenSSL and GnuTLS, but this
        # is a good enough approximation in practice.
        *" test_"*)
            # To avoid setting OpenSSL and GnuTLS for each call to compat.sh
            # and ssl-opt.sh, we just export the variables they require.
            export OPENSSL_CMD="$OPENSSL"
            export GNUTLS_CLI="$GNUTLS_CLI"
            export GNUTLS_SERV="$GNUTLS_SERV"
            # Avoid passing --seed flag in every call to ssl-opt.sh
            if [ -n "${SEED-}" ]; then
                export SEED
            fi
            set "$@" OPENSSL="$OPENSSL" OPENSSL_LEGACY="$OPENSSL_LEGACY"
            set "$@" GNUTLS_CLI="$GNUTLS_CLI" GNUTLS_SERV="$GNUTLS_SERV"
            set "$@" GNUTLS_LEGACY_CLI="$GNUTLS_LEGACY_CLI"
            set "$@" GNUTLS_LEGACY_SERV="$GNUTLS_LEGACY_SERV"
            check_tools "$OPENSSL" "$OPENSSL_LEGACY" "$OPENSSL_NEXT" \
                        "$GNUTLS_CLI" "$GNUTLS_SERV" \
                        "$GNUTLS_LEGACY_CLI" "$GNUTLS_LEGACY_SERV"
            ;;
    esac

    case " $RUN_COMPONENTS " in
        *_doxygen[_\ ]*) check_tools "doxygen" "dot";;
    esac

    case " $RUN_COMPONENTS " in
        *_arm_none_eabi_gcc[_\ ]*) check_tools "${ARM_NONE_EABI_GCC_PREFIX}gcc";;
    esac

    case " $RUN_COMPONENTS " in
        *_mingw[_\ ]*) check_tools "i686-w64-mingw32-gcc";;
    esac

    case " $RUN_COMPONENTS " in
        *" test_zeroize "*) check_tools "gdb";;
    esac

    case " $RUN_COMPONENTS " in
        *_armcc*)
            ARMC5_CC="$ARMC5_BIN_DIR/armcc"
            ARMC5_AR="$ARMC5_BIN_DIR/armar"
            ARMC5_FROMELF="$ARMC5_BIN_DIR/fromelf"
            ARMC6_CC="$ARMC6_BIN_DIR/armclang"
            ARMC6_AR="$ARMC6_BIN_DIR/armar"
            ARMC6_FROMELF="$ARMC6_BIN_DIR/fromelf"
            check_tools "$ARMC5_CC" "$ARMC5_AR" "$ARMC5_FROMELF" \
                        "$ARMC6_CC" "$ARMC6_AR" "$ARMC6_FROMELF";;
    esac

    # past this point, no call to check_tool, only printing output
    if [ $QUIET -eq 1 ]; then
        return
    fi

    msg "info: output_env.sh"
    case $RUN_COMPONENTS in
        *_armcc*)
            set "$@" ARMC5_CC="$ARMC5_CC" ARMC6_CC="$ARMC6_CC" RUN_ARMCC=1;;
        *) set "$@" RUN_ARMCC=0;;
    esac
    "$@" scripts/output_env.sh
}



################################################################
#### Basic checks
################################################################

#
# Test Suites to be executed
#
# The test ordering tries to optimize for the following criteria:
# 1. Catch possible problems early, by running first tests that run quickly
#    and/or are more likely to fail than others (eg I use Clang most of the
#    time, so start with a GCC build).
# 2. Minimize total running time, by avoiding useless rebuilds
#
# Indicative running times are given for reference.

component_check_recursion () {
    msg "Check: recursion.pl" # < 1s
    record_status tests/scripts/recursion.pl library/*.c
}

component_check_generated_files () {
    msg "Check: freshness of generated source files" # < 1s
    record_status tests/scripts/check-generated-files.sh
}

component_check_doxy_blocks () {
    msg "Check: doxygen markup outside doxygen blocks" # < 1s
    record_status tests/scripts/check-doxy-blocks.pl
}

component_check_files () {
    msg "Check: file sanity checks (permissions, encodings)" # < 1s
    record_status tests/scripts/check_files.py
}

component_check_changelog () {
    msg "Check: changelog entries" # < 1s
    rm -f ChangeLog.new
    record_status scripts/assemble_changelog.py -o ChangeLog.new
    if [ -e ChangeLog.new ]; then
        # Show the diff for information. It isn't an error if the diff is
        # non-empty.
        diff -u ChangeLog ChangeLog.new || true
        rm ChangeLog.new
    fi
}

component_check_names () {
    msg "Check: declared and exported names (builds the library)" # < 3s
    record_status tests/scripts/check-names.sh -v
}

component_check_test_cases () {
    msg "Check: test case descriptions" # < 1s
    if [ $QUIET -eq 1 ]; then
        opt='--quiet'
    else
        opt=''
    fi
    record_status tests/scripts/check_test_cases.py $opt
    unset opt
}

component_check_doxygen_warnings () {
    msg "Check: doxygen warnings (builds the documentation)" # ~ 3s
    record_status tests/scripts/doxygen.sh
}



################################################################
#### Build and test many configurations and targets
################################################################

component_test_default_out_of_box () {
    msg "build: make, default config (out-of-box)" # ~1min
    make
    # Disable fancy stuff
    SAVE_MBEDTLS_TEST_OUTCOME_FILE="$MBEDTLS_TEST_OUTCOME_FILE"
    unset MBEDTLS_TEST_OUTCOME_FILE

    msg "test: main suites make, default config (out-of-box)" # ~10s
    make test

    msg "selftest: make, default config (out-of-box)" # ~10s
    if_build_succeeded programs/test/selftest

    export MBEDTLS_TEST_OUTCOME_FILE="$SAVE_MBEDTLS_TEST_OUTCOME_FILE"
    unset SAVE_MBEDTLS_TEST_OUTCOME_FILE
}

component_test_default_cmake_gcc_asan () {
    msg "build: cmake, gcc, ASan" # ~ 1 min 50s
    CC=gcc cmake -D CMAKE_BUILD_TYPE:String=Asan .
    make

    msg "test: main suites (inc. selftests) (ASan build)" # ~ 50s
    make test

    msg "test: selftest (ASan build)" # ~ 10s
    if_build_succeeded programs/test/selftest

    msg "test: ssl-opt.sh (ASan build)" # ~ 1 min
    if_build_succeeded tests/ssl-opt.sh

    msg "test: compat.sh (ASan build)" # ~ 6 min
    if_build_succeeded tests/compat.sh

    msg "test: context-info.sh (ASan build)" # ~ 15 sec
    if_build_succeeded tests/context-info.sh
}

component_test_full_cmake_gcc_asan () {
    msg "build: full config, cmake, gcc, ASan"
    scripts/config.py full
    CC=gcc cmake -D CMAKE_BUILD_TYPE:String=Asan .
    make

    msg "test: main suites (inc. selftests) (full config, ASan build)"
    make test

    msg "test: selftest (ASan build)" # ~ 10s
    if_build_succeeded programs/test/selftest

    msg "test: ssl-opt.sh (full config, ASan build)"
    if_build_succeeded tests/ssl-opt.sh

    msg "test: compat.sh (full config, ASan build)"
    if_build_succeeded tests/compat.sh

    msg "test: context-info.sh (full config, ASan build)" # ~ 15 sec
    if_build_succeeded tests/context-info.sh
}

component_test_psa_crypto_key_id_encodes_owner () {
    msg "build: full config - USE_PSA_CRYPTO + PSA_CRYPTO_KEY_ID_ENCODES_OWNER, cmake, gcc, ASan"
    scripts/config.py full
    scripts/config.py unset MBEDTLS_USE_PSA_CRYPTO
    scripts/config.py set MBEDTLS_PSA_CRYPTO_KEY_ID_ENCODES_OWNER
    CC=gcc cmake -D CMAKE_BUILD_TYPE:String=Asan .
    make

    msg "test: full config - USE_PSA_CRYPTO + PSA_CRYPTO_KEY_ID_ENCODES_OWNER, cmake, gcc, ASan"
    make test
}

component_test_zlib_make() {
    msg "build: zlib enabled, make"
    scripts/config.py set MBEDTLS_ZLIB_SUPPORT
    make ZLIB=1 CFLAGS='-Werror -O1'

    msg "test: main suites (zlib, make)"
    make test

    msg "test: ssl-opt.sh (zlib, make)"
    if_build_succeeded tests/ssl-opt.sh
}
support_test_zlib_make () {
    base=support_test_zlib_$$
    cat <<'EOF' > ${base}.c
#include "zlib.h"
int main(void) { return 0; }
EOF
    gcc -o ${base}.exe ${base}.c -lz 2>/dev/null
    ret=$?
    rm -f ${base}.*
    return $ret
}

component_test_zlib_cmake() {
    msg "build: zlib enabled, cmake"
    scripts/config.py set MBEDTLS_ZLIB_SUPPORT
    cmake -D ENABLE_ZLIB_SUPPORT=On -D CMAKE_BUILD_TYPE:String=Check .
    make

    msg "test: main suites (zlib, cmake)"
    make test

    msg "test: ssl-opt.sh (zlib, cmake)"
    if_build_succeeded tests/ssl-opt.sh
}
support_test_zlib_cmake () {
    support_test_zlib_make "$@"
}

component_test_ref_configs () {
    msg "test/build: ref-configs (ASan build)" # ~ 6 min 20s
    CC=gcc cmake -D CMAKE_BUILD_TYPE:String=Asan .
    record_status tests/scripts/test-ref-configs.pl
}

component_test_sslv3 () {
    msg "build: Default + SSLv3 (ASan build)" # ~ 6 min
    scripts/config.py set MBEDTLS_SSL_PROTO_SSL3
    CC=gcc cmake -D CMAKE_BUILD_TYPE:String=Asan .
    make

    msg "test: SSLv3 - main suites (inc. selftests) (ASan build)" # ~ 50s
    make test

    msg "build: SSLv3 - compat.sh (ASan build)" # ~ 6 min
    if_build_succeeded tests/compat.sh -m 'tls1 tls1_1 tls1_2 dtls1 dtls1_2'
    if_build_succeeded env OPENSSL_CMD="$OPENSSL_LEGACY" tests/compat.sh -m 'ssl3'

    msg "build: SSLv3 - ssl-opt.sh (ASan build)" # ~ 6 min
    if_build_succeeded tests/ssl-opt.sh

    msg "build: SSLv3 - context-info.sh (ASan build)" # ~ 15 sec
    if_build_succeeded tests/context-info.sh
}

component_test_no_renegotiation () {
    msg "build: Default + !MBEDTLS_SSL_RENEGOTIATION (ASan build)" # ~ 6 min
    scripts/config.py unset MBEDTLS_SSL_RENEGOTIATION
    CC=gcc cmake -D CMAKE_BUILD_TYPE:String=Asan .
    make

    msg "test: !MBEDTLS_SSL_RENEGOTIATION - main suites (inc. selftests) (ASan build)" # ~ 50s
    make test

    msg "test: !MBEDTLS_SSL_RENEGOTIATION - ssl-opt.sh (ASan build)" # ~ 6 min
    if_build_succeeded tests/ssl-opt.sh
}

component_test_no_pem_no_fs () {
    msg "build: Default + !MBEDTLS_PEM_PARSE_C + !MBEDTLS_FS_IO (ASan build)"
    scripts/config.py unset MBEDTLS_PEM_PARSE_C
    scripts/config.py unset MBEDTLS_FS_IO
    scripts/config.py unset MBEDTLS_PSA_ITS_FILE_C # requires a filesystem
    scripts/config.py unset MBEDTLS_PSA_CRYPTO_STORAGE_C # requires PSA ITS
    CC=gcc cmake -D CMAKE_BUILD_TYPE:String=Asan .
    make

    msg "test: !MBEDTLS_PEM_PARSE_C !MBEDTLS_FS_IO - main suites (inc. selftests) (ASan build)" # ~ 50s
    make test

    msg "test: !MBEDTLS_PEM_PARSE_C !MBEDTLS_FS_IO - ssl-opt.sh (ASan build)" # ~ 6 min
    if_build_succeeded tests/ssl-opt.sh
}

component_test_rsa_no_crt () {
    msg "build: Default + RSA_NO_CRT (ASan build)" # ~ 6 min
    scripts/config.py set MBEDTLS_RSA_NO_CRT
    CC=gcc cmake -D CMAKE_BUILD_TYPE:String=Asan .
    make

    msg "test: RSA_NO_CRT - main suites (inc. selftests) (ASan build)" # ~ 50s
    make test

    msg "test: RSA_NO_CRT - RSA-related part of ssl-opt.sh (ASan build)" # ~ 5s
    if_build_succeeded tests/ssl-opt.sh -f RSA

    msg "test: RSA_NO_CRT - RSA-related part of compat.sh (ASan build)" # ~ 3 min
    if_build_succeeded tests/compat.sh -t RSA

    msg "test: RSA_NO_CRT - RSA-related part of context-info.sh (ASan build)" # ~ 15 sec
    if_build_succeeded tests/context-info.sh
}

component_test_no_ctr_drbg () {
    msg "build: Full minus CTR_DRBG"
    scripts/config.py full
    scripts/config.py unset MBEDTLS_CTR_DRBG_C
    scripts/config.py unset MBEDTLS_PSA_CRYPTO_C # requires CTR_DRBG
    scripts/config.py unset MBEDTLS_PSA_CRYPTO_STORAGE_C # requires PSA Crypto
    scripts/config.py unset MBEDTLS_PSA_CRYPTO_SE_C # requires PSA Crypto
    scripts/config.py unset MBEDTLS_USE_PSA_CRYPTO # requires PSA Crypto

    CC=gcc cmake -D CMAKE_BUILD_TYPE:String=Asan .
    make

    msg "test: no CTR_DRBG"
    make test

    # no ssl-opt.sh/compat.sh as they all depend on CTR_DRBG so far
}

component_test_no_hmac_drbg () {
    msg "build: Full minus HMAC_DRBG"
    scripts/config.py full
    scripts/config.py unset MBEDTLS_HMAC_DRBG_C
    scripts/config.py unset MBEDTLS_ECDSA_DETERMINISTIC # requires HMAC_DRBG

    CC=gcc cmake -D CMAKE_BUILD_TYPE:String=Asan .
    make

    msg "test: no HMAC_DRBG"
    make test

    # No ssl-opt.sh/compat.sh as they never use HMAC_DRBG so far,
    # so there's little value in running those lengthy tests here.
}

component_test_ecp_no_internal_rng () {
    msg "build: Default plus ECP_NO_INTERNAL_RNG minus DRBG modules"
    scripts/config.py set MBEDTLS_ECP_NO_INTERNAL_RNG
    scripts/config.py unset MBEDTLS_CTR_DRBG_C
    scripts/config.py unset MBEDTLS_HMAC_DRBG_C
    scripts/config.py unset MBEDTLS_ECDSA_DETERMINISTIC # requires HMAC_DRBG
    scripts/config.py unset MBEDTLS_PSA_CRYPTO_C # requires a DRBG
    scripts/config.py unset MBEDTLS_PSA_CRYPTO_STORAGE_C # requires PSA Crypto

    CC=gcc cmake -D CMAKE_BUILD_TYPE:String=Asan .
    make

    msg "test: ECP_NO_INTERNAL_RNG, no DRBG module"
    make test

    # no SSL tests as they all depend on having a DRBG
}

component_test_ecp_restartable_no_internal_rng () {
    msg "build: Default plus ECP_RESTARTABLE and ECP_NO_INTERNAL_RNG, no DRBG"
    scripts/config.py set MBEDTLS_ECP_NO_INTERNAL_RNG
    scripts/config.py set MBEDTLS_ECP_RESTARTABLE
    scripts/config.py unset MBEDTLS_CTR_DRBG_C
    scripts/config.py unset MBEDTLS_HMAC_DRBG_C
    scripts/config.py unset MBEDTLS_ECDSA_DETERMINISTIC # requires HMAC_DRBG
    scripts/config.py unset MBEDTLS_PSA_CRYPTO_C # requires CTR_DRBG
    scripts/config.py unset MBEDTLS_PSA_CRYPTO_STORAGE_C # requires PSA Crypto

    CC=gcc cmake -D CMAKE_BUILD_TYPE:String=Asan .
    make

    msg "test: ECP_RESTARTABLE and ECP_NO_INTERNAL_RNG, no DRBG module"
    make test

    # no SSL tests as they all depend on having a DRBG
}

component_test_new_ecdh_context () {
    msg "build: new ECDH context (ASan build)" # ~ 6 min
    scripts/config.py unset MBEDTLS_ECDH_LEGACY_CONTEXT
    CC=gcc cmake -D CMAKE_BUILD_TYPE:String=Asan .
    make

    msg "test: new ECDH context - main suites (inc. selftests) (ASan build)" # ~ 50s
    make test

    msg "test: new ECDH context - ECDH-related part of ssl-opt.sh (ASan build)" # ~ 5s
    if_build_succeeded tests/ssl-opt.sh -f ECDH

    msg "test: new ECDH context - compat.sh with some ECDH ciphersuites (ASan build)" # ~ 3 min
    # Exclude some symmetric ciphers that are redundant here to gain time.
    if_build_succeeded tests/compat.sh -f ECDH -V NO -e 'ARCFOUR\|ARIA\|CAMELLIA\|CHACHA\|DES\|RC4'
}

component_test_everest () {
    msg "build: Everest ECDH context (ASan build)" # ~ 6 min
    scripts/config.py unset MBEDTLS_ECDH_LEGACY_CONTEXT
    scripts/config.py set MBEDTLS_ECDH_VARIANT_EVEREST_ENABLED
    CC=clang cmake -D CMAKE_BUILD_TYPE:String=Asan .
    make

    msg "test: Everest ECDH context - main suites (inc. selftests) (ASan build)" # ~ 50s
    make test

    msg "test: Everest ECDH context - ECDH-related part of ssl-opt.sh (ASan build)" # ~ 5s
    if_build_succeeded tests/ssl-opt.sh -f ECDH

    msg "test: Everest ECDH context - compat.sh with some ECDH ciphersuites (ASan build)" # ~ 3 min
    # Exclude some symmetric ciphers that are redundant here to gain time.
    if_build_succeeded tests/compat.sh -f ECDH -V NO -e 'ARCFOUR\|ARIA\|CAMELLIA\|CHACHA\|DES\|RC4'
}

component_test_everest_curve25519_only () {
    msg "build: Everest ECDH context, only Curve25519" # ~ 6 min
    scripts/config.py unset MBEDTLS_ECDH_LEGACY_CONTEXT
    scripts/config.py set MBEDTLS_ECDH_VARIANT_EVEREST_ENABLED
    scripts/config.py unset MBEDTLS_ECDSA_C
    scripts/config.py unset MBEDTLS_KEY_EXCHANGE_ECDH_ECDSA_ENABLED
    scripts/config.py unset MBEDTLS_KEY_EXCHANGE_ECDHE_ECDSA_ENABLED
    # Disable all curves
    for c in $(sed -n 's/#define \(MBEDTLS_ECP_DP_[0-9A-Z_a-z]*_ENABLED\).*/\1/p' <"$CONFIG_H"); do
        scripts/config.py unset "$c"
    done
    scripts/config.py set MBEDTLS_ECP_DP_CURVE25519_ENABLED

    make CFLAGS="$ASAN_CFLAGS -O2" LDFLAGS="$ASAN_CFLAGS"

    msg "test: Everest ECDH context, only Curve25519" # ~ 50s
    make test
}

component_test_small_ssl_out_content_len () {
    msg "build: small SSL_OUT_CONTENT_LEN (ASan build)"
    scripts/config.py set MBEDTLS_SSL_IN_CONTENT_LEN 16384
    scripts/config.py set MBEDTLS_SSL_OUT_CONTENT_LEN 4096
    CC=gcc cmake -D CMAKE_BUILD_TYPE:String=Asan .
    make

    msg "test: small SSL_OUT_CONTENT_LEN - ssl-opt.sh MFL and large packet tests"
    if_build_succeeded tests/ssl-opt.sh -f "Max fragment\|Large packet"
}

component_test_small_ssl_in_content_len () {
    msg "build: small SSL_IN_CONTENT_LEN (ASan build)"
    scripts/config.py set MBEDTLS_SSL_IN_CONTENT_LEN 4096
    scripts/config.py set MBEDTLS_SSL_OUT_CONTENT_LEN 16384
    CC=gcc cmake -D CMAKE_BUILD_TYPE:String=Asan .
    make

    msg "test: small SSL_IN_CONTENT_LEN - ssl-opt.sh MFL tests"
    if_build_succeeded tests/ssl-opt.sh -f "Max fragment"
}

component_test_small_ssl_dtls_max_buffering () {
    msg "build: small MBEDTLS_SSL_DTLS_MAX_BUFFERING #0"
    scripts/config.py set MBEDTLS_SSL_DTLS_MAX_BUFFERING 1000
    CC=gcc cmake -D CMAKE_BUILD_TYPE:String=Asan .
    make

    msg "test: small MBEDTLS_SSL_DTLS_MAX_BUFFERING #0 - ssl-opt.sh specific reordering test"
    if_build_succeeded tests/ssl-opt.sh -f "DTLS reordering: Buffer out-of-order hs msg before reassembling next, free buffered msg"
}

component_test_small_mbedtls_ssl_dtls_max_buffering () {
    msg "build: small MBEDTLS_SSL_DTLS_MAX_BUFFERING #1"
    scripts/config.py set MBEDTLS_SSL_DTLS_MAX_BUFFERING 190
    CC=gcc cmake -D CMAKE_BUILD_TYPE:String=Asan .
    make

    msg "test: small MBEDTLS_SSL_DTLS_MAX_BUFFERING #1 - ssl-opt.sh specific reordering test"
    if_build_succeeded tests/ssl-opt.sh -f "DTLS reordering: Buffer encrypted Finished message, drop for fragmented NewSessionTicket"
}

component_test_psa_collect_statuses () {
  msg "build+test: psa_collect_statuses" # ~30s
  scripts/config.py full
  record_status tests/scripts/psa_collect_statuses.py
  # Check that psa_crypto_init() succeeded at least once
  record_status grep -q '^0:psa_crypto_init:' tests/statuses.log
  rm -f tests/statuses.log
}

component_test_full_cmake_clang () {
    msg "build: cmake, full config, clang" # ~ 50s
    scripts/config.py full
    CC=clang cmake -D CMAKE_BUILD_TYPE:String=Check -D ENABLE_TESTING=On .
    make

    msg "test: main suites (full config, clang)" # ~ 5s
    make test

    msg "test: psa_constant_names (full config, clang)" # ~ 1s
    record_status tests/scripts/test_psa_constant_names.py

    msg "test: ssl-opt.sh default, ECJPAKE, SSL async (full config)" # ~ 1s
    if_build_succeeded tests/ssl-opt.sh -f 'Default\|ECJPAKE\|SSL async private'

    msg "test: compat.sh RC4, DES, 3DES & NULL (full config)" # ~ 2 min
    if_build_succeeded env OPENSSL_CMD="$OPENSSL_LEGACY" GNUTLS_CLI="$GNUTLS_LEGACY_CLI" GNUTLS_SERV="$GNUTLS_LEGACY_SERV" tests/compat.sh -e '^$' -f 'NULL\|DES\|RC4\|ARCFOUR'

    msg "test: compat.sh ARIA + ChachaPoly"
    if_build_succeeded env OPENSSL_CMD="$OPENSSL_NEXT" tests/compat.sh -e '^$' -f 'ARIA\|CHACHA'
}

component_test_memsan_constant_flow () {
    # This tests both (1) accesses to undefined memory, and (2) branches or
    # memory access depending on secret values. To distinguish between those:
    # - unset MBEDTLS_TEST_CONSTANT_FLOW_MEMSAN - does the failure persist?
    # - or alternatively, change the build type to MemSanDbg, which enables
    # origin tracking and nicer stack traces (which are useful for debugging
    # anyway), and check if the origin was TEST_CF_SECRET() or something else.
    msg "build: cmake MSan (clang), full config with constant flow testing"
    scripts/config.py full
    scripts/config.py set MBEDTLS_TEST_CONSTANT_FLOW_MEMSAN
    scripts/config.py unset MBEDTLS_AESNI_C # memsan doesn't grok asm
    CC=clang cmake -D CMAKE_BUILD_TYPE:String=MemSan .
    make

    msg "test: main suites (Msan + constant flow)"
    make test
}

component_test_valgrind_constant_flow () {
    # This tests both (1) everything that valgrind's memcheck usually checks
    # (heap buffer overflows, use of uninitialized memory, use-after-free,
    # etc.) and (2) branches or memory access depending on secret values,
    # which will be reported as uninitialized memory. To distinguish between
    # secret and actually uninitialized:
    # - unset MBEDTLS_TEST_CONSTANT_FLOW_VALGRIND - does the failure persist?
    # - or alternatively, build with debug info and manually run the offending
    # test suite with valgrind --track-origins=yes, then check if the origin
    # was TEST_CF_SECRET() or something else.
    msg "build: cmake release GCC, full config with constant flow testing"
    scripts/config.py full
    scripts/config.py set MBEDTLS_TEST_CONSTANT_FLOW_VALGRIND
    cmake -D CMAKE_BUILD_TYPE:String=Release .
    make

    # this only shows a summary of the results (how many of each type)
    # details are left in Testing/<date>/DynamicAnalysis.xml
    msg "test: main suites (valgrind + constant flow)"
    make memcheck
}

component_test_default_no_deprecated () {
    # Test that removing the deprecated features from the default
    # configuration leaves something consistent.
    msg "build: make, default + MBEDTLS_DEPRECATED_REMOVED" # ~ 30s
    scripts/config.py set MBEDTLS_DEPRECATED_REMOVED
    make CC=gcc CFLAGS='-O -Werror -Wall -Wextra'

    msg "test: make, default + MBEDTLS_DEPRECATED_REMOVED" # ~ 5s
    make test
}

component_test_full_no_deprecated () {
    msg "build: make, full_no_deprecated config" # ~ 30s
    scripts/config.py full_no_deprecated
    make CC=gcc CFLAGS='-O -Werror -Wall -Wextra'

    msg "test: make, full_no_deprecated config" # ~ 5s
    make test
}

component_test_full_no_deprecated_deprecated_warning () {
    # Test that there is nothing deprecated in "full_no_deprecated".
    # A deprecated feature would trigger a warning (made fatal) from
    # MBEDTLS_DEPRECATED_WARNING.
    msg "build: make, full_no_deprecated config, MBEDTLS_DEPRECATED_WARNING" # ~ 30s
    scripts/config.py full_no_deprecated
    scripts/config.py unset MBEDTLS_DEPRECATED_REMOVED
    scripts/config.py set MBEDTLS_DEPRECATED_WARNING
    make CC=gcc CFLAGS='-O -Werror -Wall -Wextra'

    msg "test: make, full_no_deprecated config, MBEDTLS_DEPRECATED_WARNING" # ~ 5s
    make test
}

component_test_full_deprecated_warning () {
    # Test that when MBEDTLS_DEPRECATED_WARNING is enabled, the build passes
    # with only certain whitelisted types of warnings.
    msg "build: make, full config + MBEDTLS_DEPRECATED_WARNING, expect warnings" # ~ 30s
    scripts/config.py full
    scripts/config.py set MBEDTLS_DEPRECATED_WARNING
    # Expect warnings from '#warning' directives in check_config.h.
    make CC=gcc CFLAGS='-O -Werror -Wall -Wextra -Wno-error=cpp' lib programs

    msg "build: make tests, full config + MBEDTLS_DEPRECATED_WARNING, expect warnings" # ~ 30s
    # Set MBEDTLS_TEST_DEPRECATED to enable tests for deprecated features.
    # By default those are disabled when MBEDTLS_DEPRECATED_WARNING is set.
    # Expect warnings from '#warning' directives in check_config.h and
    # from the use of deprecated functions in test suites.
    make CC=gcc CFLAGS='-O -Werror -Wall -Wextra -Wno-error=deprecated-declarations -Wno-error=cpp -DMBEDTLS_TEST_DEPRECATED' tests

    msg "test: full config + MBEDTLS_TEST_DEPRECATED" # ~ 30s
    make test
}

# Check that the specified libraries exist and are empty.
are_empty_libraries () {
  nm "$@" >/dev/null 2>/dev/null
  ! nm "$@" 2>/dev/null | grep -v ':$' | grep .
}

component_build_crypto_default () {
  msg "build: make, crypto only"
  scripts/config.py crypto
  make CFLAGS='-O1 -Werror'
  if_build_succeeded are_empty_libraries library/libmbedx509.* library/libmbedtls.*
}

component_build_crypto_full () {
  msg "build: make, crypto only, full config"
  scripts/config.py crypto_full
  make CFLAGS='-O1 -Werror'
  if_build_succeeded are_empty_libraries library/libmbedx509.* library/libmbedtls.*
}

component_build_crypto_baremetal () {
  msg "build: make, crypto only, baremetal config"
  scripts/config.py crypto_baremetal
  make CFLAGS='-O1 -Werror'
  if_build_succeeded are_empty_libraries library/libmbedx509.* library/libmbedtls.*
}

component_test_depends_curves () {
    msg "test/build: curves.pl (gcc)" # ~ 4 min
    record_status tests/scripts/curves.pl
}

component_test_depends_curves_psa () {
    msg "test/build: curves.pl with MBEDTLS_USE_PSA_CRYPTO defined (gcc)"
    scripts/config.py set MBEDTLS_USE_PSA_CRYPTO
    record_status tests/scripts/curves.pl
}

component_test_depends_hashes () {
    msg "test/build: depends-hashes.pl (gcc)" # ~ 2 min
    record_status tests/scripts/depends-hashes.pl
}

component_test_depends_hashes_psa () {
    msg "test/build: depends-hashes.pl with MBEDTLS_USE_PSA_CRYPTO defined (gcc)"
    scripts/config.py set MBEDTLS_USE_PSA_CRYPTO
    record_status tests/scripts/depends-hashes.pl
}

component_test_depends_pkalgs () {
    msg "test/build: depends-pkalgs.pl (gcc)" # ~ 2 min
    record_status tests/scripts/depends-pkalgs.pl
}

component_test_depends_pkalgs_psa () {
    msg "test/build: depends-pkalgs.pl with MBEDTLS_USE_PSA_CRYPTO defined (gcc)"
    scripts/config.py set MBEDTLS_USE_PSA_CRYPTO
    record_status tests/scripts/depends-pkalgs.pl
}

component_build_key_exchanges () {
    msg "test/build: key-exchanges (gcc)" # ~ 1 min
    record_status tests/scripts/key-exchanges.pl
}

component_build_default_make_gcc_and_cxx () {
    msg "build: Unix make, -Os (gcc)" # ~ 30s
    make CC=gcc CFLAGS='-Werror -Wall -Wextra -Os'

    msg "test: verify header list in cpp_dummy_build.cpp"
    record_status check_headers_in_cpp

    msg "build: Unix make, incremental g++"
    make TEST_CPP=1
}

component_test_no_use_psa_crypto_full_cmake_asan() {
    # full minus MBEDTLS_USE_PSA_CRYPTO: run the same set of tests as basic-build-test.sh
    msg "build: cmake, full config minus MBEDTLS_USE_PSA_CRYPTO, ASan"
    scripts/config.py full
    scripts/config.py set MBEDTLS_ECP_RESTARTABLE  # not using PSA, so enable restartable ECC
    scripts/config.py unset MBEDTLS_PSA_CRYPTO_C
    scripts/config.py unset MBEDTLS_USE_PSA_CRYPTO
    scripts/config.py unset MBEDTLS_PSA_ITS_FILE_C
    scripts/config.py unset MBEDTLS_PSA_CRYPTO_SE_C
    scripts/config.py unset MBEDTLS_PSA_CRYPTO_STORAGE_C
    CC=gcc cmake -D CMAKE_BUILD_TYPE:String=Asan .
    make

    msg "test: main suites (full minus MBEDTLS_USE_PSA_CRYPTO)"
    make test

    msg "test: ssl-opt.sh (full minus MBEDTLS_USE_PSA_CRYPTO)"
    if_build_succeeded tests/ssl-opt.sh

    msg "test: compat.sh default (full minus MBEDTLS_USE_PSA_CRYPTO)"
    if_build_succeeded tests/compat.sh

    msg "test: compat.sh RC4, DES & NULL (full minus MBEDTLS_USE_PSA_CRYPTO)"
    if_build_succeeded env OPENSSL_CMD="$OPENSSL_LEGACY" GNUTLS_CLI="$GNUTLS_LEGACY_CLI" GNUTLS_SERV="$GNUTLS_LEGACY_SERV" tests/compat.sh -e '3DES\|DES-CBC3' -f 'NULL\|DES\|RC4\|ARCFOUR'

    msg "test: compat.sh ARIA + ChachaPoly (full minus MBEDTLS_USE_PSA_CRYPTO)"
    if_build_succeeded env OPENSSL_CMD="$OPENSSL_NEXT" tests/compat.sh -e '^$' -f 'ARIA\|CHACHA'
}

component_test_psa_crypto_config_basic() {
    # full plus MBEDTLS_PSA_CRYPTO_CONFIG
    msg "build: full + MBEDTLS_PSA_CRYPTO_CONFIG"
    scripts/config.py full
    scripts/config.py set MBEDTLS_PSA_CRYPTO_CONFIG
    scripts/config.py set MBEDTLS_PSA_CRYPTO_DRIVERS
    scripts/config.py unset MBEDTLS_USE_PSA_CRYPTO
    # Need to define the correct symbol and include the test driver header path in order to build with the test driver
    make CC=gcc CFLAGS="$ASAN_CFLAGS -DPSA_CRYPTO_DRIVER_TEST -I../tests/include -O2" LDFLAGS="$ASAN_CFLAGS"

    msg "test: full + MBEDTLS_PSA_CRYPTO_CONFIG"
    make test
}

component_test_psa_crypto_config_no_driver() {
    # full plus MBEDTLS_PSA_CRYPTO_CONFIG
    msg "build: full + MBEDTLS_PSA_CRYPTO_CONFIG minus MBEDTLS_PSA_CRYPTO_DRIVERS"
    scripts/config.py full
    scripts/config.py set MBEDTLS_PSA_CRYPTO_CONFIG
    scripts/config.py unset MBEDTLS_PSA_CRYPTO_DRIVERS
    scripts/config.py unset MBEDTLS_USE_PSA_CRYPTO
    make CC=gcc CFLAGS="$ASAN_CFLAGS -O2" LDFLAGS="$ASAN_CFLAGS"

    msg "test: full + MBEDTLS_PSA_CRYPTO_CONFIG minus MBEDTLS_PSA_CRYPTO_DRIVERS"
    make test
}

# This should be renamed to test and updated once the accelerator ECDSA code is in place and ready to test.
component_build_psa_accel_alg_ecdsa() {
    # full plus MBEDTLS_PSA_CRYPTO_CONFIG with PSA_WANT_ALG_ECDSA
    # without MBEDTLS_ECDSA_C
    # PSA_WANT_ALG_ECDSA and PSA_WANT_ALG_DETERMINISTIC_ECDSA are already
    # set in include/psa/crypto_config.h
    msg "build: full + MBEDTLS_PSA_CRYPTO_CONFIG + PSA_WANT_ALG_ECDSA without MBEDTLS_ECDSA_C"
    scripts/config.py full
    scripts/config.py set MBEDTLS_PSA_CRYPTO_CONFIG
    scripts/config.py set MBEDTLS_PSA_CRYPTO_DRIVERS
    scripts/config.py unset MBEDTLS_USE_PSA_CRYPTO
    scripts/config.py unset MBEDTLS_ECDSA_C
    scripts/config.py unset MBEDTLS_KEY_EXCHANGE_ECDH_ECDSA_ENABLED
    scripts/config.py unset MBEDTLS_KEY_EXCHANGE_ECDHE_ECDSA_ENABLED
    # Need to define the correct symbol and include the test driver header path in order to build with the test driver
    make CC=gcc CFLAGS="$ASAN_CFLAGS -DPSA_CRYPTO_DRIVER_TEST -DMBEDTLS_PSA_ACCEL_ALG_ECDSA -DMBEDTLS_PSA_ACCEL_ALG_DETERMINISTIC_ECDSA -I../tests/include -O2" LDFLAGS="$ASAN_CFLAGS"
}

# This should be renamed to test and updated once the accelerator ECDH code is in place and ready to test.
component_build_psa_accel_alg_ecdh() {
    # full plus MBEDTLS_PSA_CRYPTO_CONFIG with PSA_WANT_ALG_ECDH
    # without MBEDTLS_ECDH_C
    msg "build: full + MBEDTLS_PSA_CRYPTO_CONFIG + PSA_WANT_ALG_ECDH without MBEDTLS_ECDH_C"
    scripts/config.py full
    scripts/config.py set MBEDTLS_PSA_CRYPTO_CONFIG
    scripts/config.py set MBEDTLS_PSA_CRYPTO_DRIVERS
    scripts/config.py unset MBEDTLS_USE_PSA_CRYPTO
    scripts/config.py unset MBEDTLS_ECDH_C
    scripts/config.py unset MBEDTLS_KEY_EXCHANGE_ECDH_RSA_ENABLED
    scripts/config.py unset MBEDTLS_KEY_EXCHANGE_ECDH_ECDSA_ENABLED
    scripts/config.py unset MBEDTLS_KEY_EXCHANGE_ECDHE_ECDSA_ENABLED
    scripts/config.py unset MBEDTLS_KEY_EXCHANGE_ECDHE_RSA_ENABLED
    scripts/config.py unset MBEDTLS_KEY_EXCHANGE_ECDHE_PSK_ENABLED
    # Need to define the correct symbol and include the test driver header path in order to build with the test driver
    make CC=gcc CFLAGS="$ASAN_CFLAGS -DPSA_CRYPTO_DRIVER_TEST -DMBEDTLS_PSA_ACCEL_ALG_ECDH -I../tests/include -O2" LDFLAGS="$ASAN_CFLAGS"
}

# This should be renamed to test and updated once the accelerator ECC key pair code is in place and ready to test.
component_build_psa_accel_key_type_ecc_key_pair() {
    # full plus MBEDTLS_PSA_CRYPTO_CONFIG with PSA_WANT_KEY_TYPE_ECC_KEY_PAIR
    msg "build: full + MBEDTLS_PSA_CRYPTO_CONFIG + PSA_WANT_KEY_TYPE_ECC_KEY_PAIR"
    scripts/config.py full
    scripts/config.py set MBEDTLS_PSA_CRYPTO_CONFIG
    scripts/config.py set MBEDTLS_PSA_CRYPTO_DRIVERS
    scripts/config.py unset MBEDTLS_USE_PSA_CRYPTO
    scripts/config.py -f include/psa/crypto_config.h set PSA_WANT_KEY_TYPE_ECC_KEY_PAIR 1
    scripts/config.py -f include/psa/crypto_config.h set PSA_WANT_KEY_TYPE_ECC_PUBLIC_KEY 1
    # Need to define the correct symbol and include the test driver header path in order to build with the test driver
    make CC=gcc CFLAGS="$ASAN_CFLAGS -DPSA_CRYPTO_DRIVER_TEST -DMBEDTLS_PSA_ACCEL_KEY_TYPE_ECC_KEY_PAIR -I../tests/include -O2" LDFLAGS="$ASAN_CFLAGS"
}

# This should be renamed to test and updated once the accelerator ECC public key code is in place and ready to test.
component_build_psa_accel_key_type_ecc_public_key() {
    # full plus MBEDTLS_PSA_CRYPTO_CONFIG with PSA_WANT_KEY_TYPE_ECC_PUBLIC_KEY
    msg "build: full + MBEDTLS_PSA_CRYPTO_CONFIG + PSA_WANT_KEY_TYPE_ECC_PUBLIC_KEY"
    scripts/config.py full
    scripts/config.py set MBEDTLS_PSA_CRYPTO_CONFIG
    scripts/config.py set MBEDTLS_PSA_CRYPTO_DRIVERS
    scripts/config.py unset MBEDTLS_USE_PSA_CRYPTO
    scripts/config.py -f include/psa/crypto_config.h set PSA_WANT_KEY_TYPE_ECC_PUBLIC_KEY 1
    scripts/config.py -f include/psa/crypto_config.h unset PSA_WANT_KEY_TYPE_ECC_KEY_PAIR
    # Need to define the correct symbol and include the test driver header path in order to build with the test driver
    make CC=gcc CFLAGS="$ASAN_CFLAGS -DPSA_CRYPTO_DRIVER_TEST -DMBEDTLS_PSA_ACCEL_KEY_TYPE_ECC_PUBLIC_KEY -I../tests/include -O2" LDFLAGS="$ASAN_CFLAGS"
}

# This should be renamed to test and updated once the accelerator HMAC code is in place and ready to test.
component_build_psa_accel_alg_hmac() {
    # full plus MBEDTLS_PSA_CRYPTO_CONFIG with PSA_WANT_ALG_HMAC
    msg "build: full + MBEDTLS_PSA_CRYPTO_CONFIG + PSA_WANT_ALG_HMAC"
    scripts/config.py full
    scripts/config.py set MBEDTLS_PSA_CRYPTO_CONFIG
    scripts/config.py set MBEDTLS_PSA_CRYPTO_DRIVERS
    scripts/config.py unset MBEDTLS_USE_PSA_CRYPTO
    # Need to define the correct symbol and include the test driver header path in order to build with the test driver
    make CC=gcc CFLAGS="$ASAN_CFLAGS -DPSA_CRYPTO_DRIVER_TEST -DMBEDTLS_PSA_ACCEL_ALG_HMAC -I../tests/include -O2" LDFLAGS="$ASAN_CFLAGS"
}

# This should be renamed to test and updated once the accelerator HKDF code is in place and ready to test.
component_build_psa_accel_alg_hkdf() {
    # full plus MBEDTLS_PSA_CRYPTO_CONFIG with PSA_WANT_ALG_HKDF
    # without MBEDTLS_HKDF_C
    msg "build: full + MBEDTLS_PSA_CRYPTO_CONFIG + PSA_WANT_ALG_HKDF without MBEDTLS_HKDF_C"
    scripts/config.py full
    scripts/config.py set MBEDTLS_PSA_CRYPTO_CONFIG
    scripts/config.py set MBEDTLS_PSA_CRYPTO_DRIVERS
    scripts/config.py unset MBEDTLS_USE_PSA_CRYPTO
    scripts/config.py unset MBEDTLS_HKDF_C
    # Make sure to unset TLS1_3_EXPERIMENTAL since it requires HKDF_C and will not build properly without it.
    scripts/config.py unset MBEDTLS_SSL_PROTO_TLS1_3_EXPERIMENTAL
    # Need to define the correct symbol and include the test driver header path in order to build with the test driver
    make CC=gcc CFLAGS="$ASAN_CFLAGS -DPSA_CRYPTO_DRIVER_TEST -DMBEDTLS_PSA_ACCEL_ALG_HKDF -I../tests/include -O2" LDFLAGS="$ASAN_CFLAGS"
}

# This should be renamed to test and updated once the accelerator MD2 code is in place and ready to test.
component_build_psa_accel_alg_md2() {
    # full plus MBEDTLS_PSA_CRYPTO_CONFIG with PSA_WANT_ALG_MD2 without other hashes
    msg "build: full + MBEDTLS_PSA_CRYPTO_CONFIG + PSA_WANT_ALG_MD2 - other hashes"
    scripts/config.py full
    scripts/config.py set MBEDTLS_PSA_CRYPTO_CONFIG
    scripts/config.py set MBEDTLS_PSA_CRYPTO_DRIVERS
    scripts/config.py unset MBEDTLS_USE_PSA_CRYPTO
    scripts/config.py -f include/psa/crypto_config.h unset PSA_WANT_ALG_MD4
    scripts/config.py -f include/psa/crypto_config.h unset PSA_WANT_ALG_MD5
    scripts/config.py -f include/psa/crypto_config.h unset PSA_WANT_ALG_RIPEMD160
    scripts/config.py -f include/psa/crypto_config.h unset PSA_WANT_ALG_SHA_1
    scripts/config.py -f include/psa/crypto_config.h unset PSA_WANT_ALG_SHA_224
    scripts/config.py -f include/psa/crypto_config.h unset PSA_WANT_ALG_SHA_256
    scripts/config.py -f include/psa/crypto_config.h unset PSA_WANT_ALG_SHA_384
    scripts/config.py -f include/psa/crypto_config.h unset PSA_WANT_ALG_SHA_512
    # Need to define the correct symbol and include the test driver header path in order to build with the test driver
    make CC=gcc CFLAGS="$ASAN_CFLAGS -DPSA_CRYPTO_DRIVER_TEST -DMBEDTLS_PSA_ACCEL_ALG_MD2 -I../tests/include -O2" LDFLAGS="$ASAN_CFLAGS"
}

# This should be renamed to test and updated once the accelerator MD4 code is in place and ready to test.
component_build_psa_accel_alg_md4() {
    # full plus MBEDTLS_PSA_CRYPTO_CONFIG with PSA_WANT_ALG_MD4 without other hashes
    msg "build: full + MBEDTLS_PSA_CRYPTO_CONFIG + PSA_WANT_ALG_MD4 - other hashes"
    scripts/config.py full
    scripts/config.py set MBEDTLS_PSA_CRYPTO_CONFIG
    scripts/config.py set MBEDTLS_PSA_CRYPTO_DRIVERS
    scripts/config.py unset MBEDTLS_USE_PSA_CRYPTO
    scripts/config.py -f include/psa/crypto_config.h unset PSA_WANT_ALG_MD2
    scripts/config.py -f include/psa/crypto_config.h unset PSA_WANT_ALG_MD5
    scripts/config.py -f include/psa/crypto_config.h unset PSA_WANT_ALG_RIPEMD160
    scripts/config.py -f include/psa/crypto_config.h unset PSA_WANT_ALG_SHA_1
    scripts/config.py -f include/psa/crypto_config.h unset PSA_WANT_ALG_SHA_224
    scripts/config.py -f include/psa/crypto_config.h unset PSA_WANT_ALG_SHA_256
    scripts/config.py -f include/psa/crypto_config.h unset PSA_WANT_ALG_SHA_384
    scripts/config.py -f include/psa/crypto_config.h unset PSA_WANT_ALG_SHA_512
    # Need to define the correct symbol and include the test driver header path in order to build with the test driver
    make CC=gcc CFLAGS="$ASAN_CFLAGS -DPSA_CRYPTO_DRIVER_TEST -DMBEDTLS_PSA_ACCEL_ALG_MD4 -I../tests/include -O2" LDFLAGS="$ASAN_CFLAGS"
}

# This should be renamed to test and updated once the accelerator MD5 code is in place and ready to test.
component_build_psa_accel_alg_md5() {
    # full plus MBEDTLS_PSA_CRYPTO_CONFIG with PSA_WANT_ALG_MD5 without other hashes
    msg "build: full + MBEDTLS_PSA_CRYPTO_CONFIG + PSA_WANT_ALG_MD5 - other hashes"
    scripts/config.py full
    scripts/config.py set MBEDTLS_PSA_CRYPTO_CONFIG
    scripts/config.py set MBEDTLS_PSA_CRYPTO_DRIVERS
    scripts/config.py unset MBEDTLS_USE_PSA_CRYPTO
    scripts/config.py -f include/psa/crypto_config.h unset PSA_WANT_ALG_MD2
    scripts/config.py -f include/psa/crypto_config.h unset PSA_WANT_ALG_MD4
    scripts/config.py -f include/psa/crypto_config.h unset PSA_WANT_ALG_RIPEMD160
    scripts/config.py -f include/psa/crypto_config.h unset PSA_WANT_ALG_SHA_1
    scripts/config.py -f include/psa/crypto_config.h unset PSA_WANT_ALG_SHA_224
    scripts/config.py -f include/psa/crypto_config.h unset PSA_WANT_ALG_SHA_256
    scripts/config.py -f include/psa/crypto_config.h unset PSA_WANT_ALG_SHA_384
    scripts/config.py -f include/psa/crypto_config.h unset PSA_WANT_ALG_SHA_512
    # Need to define the correct symbol and include the test driver header path in order to build with the test driver
    make CC=gcc CFLAGS="$ASAN_CFLAGS -DPSA_CRYPTO_DRIVER_TEST -DMBEDTLS_PSA_ACCEL_ALG_MD5 -I../tests/include -O2" LDFLAGS="$ASAN_CFLAGS"
}

# This should be renamed to test and updated once the accelerator RIPEMD160 code is in place and ready to test.
component_build_psa_accel_alg_ripemd160() {
    # full plus MBEDTLS_PSA_CRYPTO_CONFIG with PSA_WANT_ALG_RIPEMD160 without other hashes
    msg "build: full + MBEDTLS_PSA_CRYPTO_CONFIG + PSA_WANT_ALG_RIPEMD160 - other hashes"
    scripts/config.py full
    scripts/config.py set MBEDTLS_PSA_CRYPTO_CONFIG
    scripts/config.py set MBEDTLS_PSA_CRYPTO_DRIVERS
    scripts/config.py unset MBEDTLS_USE_PSA_CRYPTO
    scripts/config.py -f include/psa/crypto_config.h unset PSA_WANT_ALG_MD2
    scripts/config.py -f include/psa/crypto_config.h unset PSA_WANT_ALG_MD4
    scripts/config.py -f include/psa/crypto_config.h unset PSA_WANT_ALG_MD5
    scripts/config.py -f include/psa/crypto_config.h unset PSA_WANT_ALG_SHA_1
    scripts/config.py -f include/psa/crypto_config.h unset PSA_WANT_ALG_SHA_224
    scripts/config.py -f include/psa/crypto_config.h unset PSA_WANT_ALG_SHA_256
    scripts/config.py -f include/psa/crypto_config.h unset PSA_WANT_ALG_SHA_384
    scripts/config.py -f include/psa/crypto_config.h unset PSA_WANT_ALG_SHA_512
    # Need to define the correct symbol and include the test driver header path in order to build with the test driver
    make CC=gcc CFLAGS="$ASAN_CFLAGS -DPSA_CRYPTO_DRIVER_TEST -DMBEDTLS_PSA_ACCEL_ALG_RIPEMD160 -I../tests/include -O2" LDFLAGS="$ASAN_CFLAGS"
}

# This should be renamed to test and updated once the accelerator SHA1 code is in place and ready to test.
component_build_psa_accel_alg_sha1() {
    # full plus MBEDTLS_PSA_CRYPTO_CONFIG with PSA_WANT_ALG_SHA_1 without other hashes
    msg "build: full + MBEDTLS_PSA_CRYPTO_CONFIG + PSA_WANT_ALG_SHA_1 - other hashes"
    scripts/config.py full
    scripts/config.py set MBEDTLS_PSA_CRYPTO_CONFIG
    scripts/config.py set MBEDTLS_PSA_CRYPTO_DRIVERS
    scripts/config.py unset MBEDTLS_USE_PSA_CRYPTO
    scripts/config.py -f include/psa/crypto_config.h unset PSA_WANT_ALG_MD2
    scripts/config.py -f include/psa/crypto_config.h unset PSA_WANT_ALG_MD4
    scripts/config.py -f include/psa/crypto_config.h unset PSA_WANT_ALG_MD5
    scripts/config.py -f include/psa/crypto_config.h unset PSA_WANT_ALG_RIPEMD160
    scripts/config.py -f include/psa/crypto_config.h unset PSA_WANT_ALG_SHA_224
    scripts/config.py -f include/psa/crypto_config.h unset PSA_WANT_ALG_SHA_256
    scripts/config.py -f include/psa/crypto_config.h unset PSA_WANT_ALG_SHA_384
    scripts/config.py -f include/psa/crypto_config.h unset PSA_WANT_ALG_SHA_512
    # Need to define the correct symbol and include the test driver header path in order to build with the test driver
    make CC=gcc CFLAGS="$ASAN_CFLAGS -DPSA_CRYPTO_DRIVER_TEST -DMBEDTLS_PSA_ACCEL_ALG_SHA_1 -I../tests/include -O2" LDFLAGS="$ASAN_CFLAGS"
}

# This should be renamed to test and updated once the accelerator SHA224 code is in place and ready to test.
component_build_psa_accel_alg_sha224() {
    # full plus MBEDTLS_PSA_CRYPTO_CONFIG with PSA_WANT_ALG_SHA_224 without other hashes
    msg "build: full + MBEDTLS_PSA_CRYPTO_CONFIG + PSA_WANT_ALG_SHA_224 - other hashes"
    scripts/config.py full
    scripts/config.py set MBEDTLS_PSA_CRYPTO_CONFIG
    scripts/config.py set MBEDTLS_PSA_CRYPTO_DRIVERS
    scripts/config.py unset MBEDTLS_USE_PSA_CRYPTO
    scripts/config.py -f include/psa/crypto_config.h unset PSA_WANT_ALG_MD2
    scripts/config.py -f include/psa/crypto_config.h unset PSA_WANT_ALG_MD4
    scripts/config.py -f include/psa/crypto_config.h unset PSA_WANT_ALG_MD5
    scripts/config.py -f include/psa/crypto_config.h unset PSA_WANT_ALG_RIPEMD160
    scripts/config.py -f include/psa/crypto_config.h unset PSA_WANT_ALG_SHA_1
    scripts/config.py -f include/psa/crypto_config.h unset PSA_WANT_ALG_SHA_384
    scripts/config.py -f include/psa/crypto_config.h unset PSA_WANT_ALG_SHA_512
    # Need to define the correct symbol and include the test driver header path in order to build with the test driver
    make CC=gcc CFLAGS="$ASAN_CFLAGS -DPSA_CRYPTO_DRIVER_TEST -DMBEDTLS_PSA_ACCEL_ALG_SHA_224 -I../tests/include -O2" LDFLAGS="$ASAN_CFLAGS"
}

# This should be renamed to test and updated once the accelerator SHA256 code is in place and ready to test.
component_build_psa_accel_alg_sha256() {
    # full plus MBEDTLS_PSA_CRYPTO_CONFIG with PSA_WANT_ALG_SHA_256 without other hashes
    msg "build: full + MBEDTLS_PSA_CRYPTO_CONFIG + PSA_WANT_ALG_SHA_256 - other hashes"
    scripts/config.py full
    scripts/config.py set MBEDTLS_PSA_CRYPTO_CONFIG
    scripts/config.py set MBEDTLS_PSA_CRYPTO_DRIVERS
    scripts/config.py unset MBEDTLS_USE_PSA_CRYPTO
    scripts/config.py -f include/psa/crypto_config.h unset PSA_WANT_ALG_MD2
    scripts/config.py -f include/psa/crypto_config.h unset PSA_WANT_ALG_MD4
    scripts/config.py -f include/psa/crypto_config.h unset PSA_WANT_ALG_MD5
    scripts/config.py -f include/psa/crypto_config.h unset PSA_WANT_ALG_RIPEMD160
    scripts/config.py -f include/psa/crypto_config.h unset PSA_WANT_ALG_SHA_1
    scripts/config.py -f include/psa/crypto_config.h unset PSA_WANT_ALG_SHA_224
    scripts/config.py -f include/psa/crypto_config.h unset PSA_WANT_ALG_SHA_384
    scripts/config.py -f include/psa/crypto_config.h unset PSA_WANT_ALG_SHA_512
    # Need to define the correct symbol and include the test driver header path in order to build with the test driver
    make CC=gcc CFLAGS="$ASAN_CFLAGS -DPSA_CRYPTO_DRIVER_TEST -DMBEDTLS_PSA_ACCEL_ALG_SHA_256 -I../tests/include -O2" LDFLAGS="$ASAN_CFLAGS"
}

# This should be renamed to test and updated once the accelerator SHA384 code is in place and ready to test.
component_build_psa_accel_alg_sha384() {
    # full plus MBEDTLS_PSA_CRYPTO_CONFIG with PSA_WANT_ALG_SHA_384 without other hashes
    msg "build: full + MBEDTLS_PSA_CRYPTO_CONFIG + PSA_WANT_ALG_SHA_384 - other hashes"
    scripts/config.py full
    scripts/config.py set MBEDTLS_PSA_CRYPTO_CONFIG
    scripts/config.py set MBEDTLS_PSA_CRYPTO_DRIVERS
    scripts/config.py unset MBEDTLS_USE_PSA_CRYPTO
    scripts/config.py -f include/psa/crypto_config.h unset PSA_WANT_ALG_MD2
    scripts/config.py -f include/psa/crypto_config.h unset PSA_WANT_ALG_MD4
    scripts/config.py -f include/psa/crypto_config.h unset PSA_WANT_ALG_MD5
    scripts/config.py -f include/psa/crypto_config.h unset PSA_WANT_ALG_RIPEMD160
    scripts/config.py -f include/psa/crypto_config.h unset PSA_WANT_ALG_SHA_1
    scripts/config.py -f include/psa/crypto_config.h unset PSA_WANT_ALG_SHA_224
    scripts/config.py -f include/psa/crypto_config.h unset PSA_WANT_ALG_SHA_256
    # Need to define the correct symbol and include the test driver header path in order to build with the test driver
    make CC=gcc CFLAGS="$ASAN_CFLAGS -DPSA_CRYPTO_DRIVER_TEST -DMBEDTLS_PSA_ACCEL_ALG_SHA_384 -I../tests/include -O2" LDFLAGS="$ASAN_CFLAGS"
}

# This should be renamed to test and updated once the accelerator SHA512 code is in place and ready to test.
component_build_psa_accel_alg_sha512() {
    # full plus MBEDTLS_PSA_CRYPTO_CONFIG with PSA_WANT_ALG_SHA_512 without other hashes
    msg "build: full + MBEDTLS_PSA_CRYPTO_CONFIG + PSA_WANT_ALG_SHA_512 - other hashes"
    scripts/config.py full
    scripts/config.py set MBEDTLS_PSA_CRYPTO_CONFIG
    scripts/config.py set MBEDTLS_PSA_CRYPTO_DRIVERS
    scripts/config.py unset MBEDTLS_USE_PSA_CRYPTO
    scripts/config.py -f include/psa/crypto_config.h unset PSA_WANT_ALG_MD2
    scripts/config.py -f include/psa/crypto_config.h unset PSA_WANT_ALG_MD4
    scripts/config.py -f include/psa/crypto_config.h unset PSA_WANT_ALG_MD5
    scripts/config.py -f include/psa/crypto_config.h unset PSA_WANT_ALG_RIPEMD160
    scripts/config.py -f include/psa/crypto_config.h unset PSA_WANT_ALG_SHA_1
    scripts/config.py -f include/psa/crypto_config.h unset PSA_WANT_ALG_SHA_224
    scripts/config.py -f include/psa/crypto_config.h unset PSA_WANT_ALG_SHA_256
    scripts/config.py -f include/psa/crypto_config.h unset PSA_WANT_ALG_SHA_384
    # Need to define the correct symbol and include the test driver header path in order to build with the test driver
    make CC=gcc CFLAGS="$ASAN_CFLAGS -DPSA_CRYPTO_DRIVER_TEST -DMBEDTLS_PSA_ACCEL_ALG_SHA_512 -I../tests/include -O2" LDFLAGS="$ASAN_CFLAGS"
}

# This should be renamed to test and updated once the accelerator RSA code is in place and ready to test.
component_build_psa_accel_alg_rsa_pkcs1v15_crypt() {
    # full plus MBEDTLS_PSA_CRYPTO_CONFIG with PSA_WANT_ALG_RSA_PKCS1V15_CRYPT
    msg "build: full + MBEDTLS_PSA_CRYPTO_CONFIG + PSA_WANT_ALG_RSA_PKCS1V15_CRYPT + PSA_WANT_KEY_TYPE_RSA_PUBLIC_KEY"
    scripts/config.py full
    scripts/config.py set MBEDTLS_PSA_CRYPTO_CONFIG
    scripts/config.py set MBEDTLS_PSA_CRYPTO_DRIVERS
    scripts/config.py unset MBEDTLS_USE_PSA_CRYPTO
    scripts/config.py -f include/psa/crypto_config.h set PSA_WANT_ALG_RSA_PKCS1V15_CRYPT 1
    scripts/config.py -f include/psa/crypto_config.h unset PSA_WANT_ALG_RSA_PKCS1V15_SIGN
    scripts/config.py -f include/psa/crypto_config.h unset PSA_WANT_ALG_RSA_OAEP
    scripts/config.py -f include/psa/crypto_config.h unset PSA_WANT_ALG_RSA_PSS
    # Need to define the correct symbol and include the test driver header path in order to build with the test driver
    make CC=gcc CFLAGS="$ASAN_CFLAGS -DPSA_CRYPTO_DRIVER_TEST -DMBEDTLS_PSA_ACCEL_ALG_RSA_PKCS1V15_CRYPT -I../tests/include -O2" LDFLAGS="$ASAN_CFLAGS"
}

# This should be renamed to test and updated once the accelerator RSA code is in place and ready to test.
component_build_psa_accel_alg_rsa_pkcs1v15_sign() {
    # full plus MBEDTLS_PSA_CRYPTO_CONFIG with PSA_WANT_ALG_RSA_PKCS1V15_SIGN and PSA_WANT_KEY_TYPE_RSA_PUBLIC_KEY
    msg "build: full + MBEDTLS_PSA_CRYPTO_CONFIG + PSA_WANT_ALG_RSA_PKCS1V15_SIGN + PSA_WANT_KEY_TYPE_RSA_PUBLIC_KEY"
    scripts/config.py full
    scripts/config.py set MBEDTLS_PSA_CRYPTO_CONFIG
    scripts/config.py set MBEDTLS_PSA_CRYPTO_DRIVERS
    scripts/config.py unset MBEDTLS_USE_PSA_CRYPTO
    scripts/config.py -f include/psa/crypto_config.h set PSA_WANT_ALG_RSA_PKCS1V15_SIGN 1
    scripts/config.py -f include/psa/crypto_config.h unset PSA_WANT_ALG_RSA_PKCS1V15_CRYPT
    scripts/config.py -f include/psa/crypto_config.h unset PSA_WANT_ALG_RSA_OAEP
    scripts/config.py -f include/psa/crypto_config.h unset PSA_WANT_ALG_RSA_PSS
    # Need to define the correct symbol and include the test driver header path in order to build with the test driver
    make CC=gcc CFLAGS="$ASAN_CFLAGS -DPSA_CRYPTO_DRIVER_TEST -DMBEDTLS_PSA_ACCEL_ALG_RSA_PKCS1V15_SIGN -I../tests/include -O2" LDFLAGS="$ASAN_CFLAGS"
}

# This should be renamed to test and updated once the accelerator RSA code is in place and ready to test.
component_build_psa_accel_alg_rsa_oaep() {
    # full plus MBEDTLS_PSA_CRYPTO_CONFIG with PSA_WANT_ALG_RSA_OAEP and PSA_WANT_KEY_TYPE_RSA_PUBLIC_KEY
    msg "build: full + MBEDTLS_PSA_CRYPTO_CONFIG + PSA_WANT_ALG_RSA_OAEP + PSA_WANT_KEY_TYPE_RSA_PUBLIC_KEY"
    scripts/config.py full
    scripts/config.py set MBEDTLS_PSA_CRYPTO_CONFIG
    scripts/config.py set MBEDTLS_PSA_CRYPTO_DRIVERS
    scripts/config.py unset MBEDTLS_USE_PSA_CRYPTO
    scripts/config.py -f include/psa/crypto_config.h set PSA_WANT_ALG_RSA_OAEP 1
    scripts/config.py -f include/psa/crypto_config.h unset PSA_WANT_ALG_RSA_PKCS1V15_CRYPT
    scripts/config.py -f include/psa/crypto_config.h unset PSA_WANT_ALG_RSA_PKCS1V15_SIGN
    scripts/config.py -f include/psa/crypto_config.h unset PSA_WANT_ALG_RSA_PSS
    # Need to define the correct symbol and include the test driver header path in order to build with the test driver
    make CC=gcc CFLAGS="$ASAN_CFLAGS -DPSA_CRYPTO_DRIVER_TEST -DMBEDTLS_PSA_ACCEL_ALG_RSA_OAEP -I../tests/include -O2" LDFLAGS="$ASAN_CFLAGS"
}

# This should be renamed to test and updated once the accelerator RSA code is in place and ready to test.
component_build_psa_accel_alg_rsa_pss() {
    # full plus MBEDTLS_PSA_CRYPTO_CONFIG with PSA_WANT_ALG_RSA_PSS and PSA_WANT_KEY_TYPE_RSA_PUBLIC_KEY
    msg "build: full + MBEDTLS_PSA_CRYPTO_CONFIG + PSA_WANT_ALG_RSA_PSS + PSA_WANT_KEY_TYPE_RSA_PUBLIC_KEY"
    scripts/config.py full
    scripts/config.py set MBEDTLS_PSA_CRYPTO_CONFIG
    scripts/config.py set MBEDTLS_PSA_CRYPTO_DRIVERS
    scripts/config.py unset MBEDTLS_USE_PSA_CRYPTO
    scripts/config.py -f include/psa/crypto_config.h set PSA_WANT_ALG_RSA_PSS 1
    scripts/config.py -f include/psa/crypto_config.h unset PSA_WANT_ALG_RSA_PKCS1V15_CRYPT
    scripts/config.py -f include/psa/crypto_config.h unset PSA_WANT_ALG_RSA_PKCS1V15_SIGN
    scripts/config.py -f include/psa/crypto_config.h unset PSA_WANT_ALG_RSA_OAEP
    # Need to define the correct symbol and include the test driver header path in order to build with the test driver
    make CC=gcc CFLAGS="$ASAN_CFLAGS -DPSA_CRYPTO_DRIVER_TEST -DMBEDTLS_PSA_ACCEL_ALG_RSA_PSS -I../tests/include -O2" LDFLAGS="$ASAN_CFLAGS"
}

# This should be renamed to test and updated once the accelerator RSA code is in place and ready to test.
component_build_psa_accel_key_type_rsa_key_pair() {
    # full plus MBEDTLS_PSA_CRYPTO_CONFIG with PSA_WANT_KEY_TYPE_RSA_KEY_PAIR and PSA_WANT_ALG_RSA_PSS
    msg "build: full + MBEDTLS_PSA_CRYPTO_CONFIG + PSA_WANT_KEY_TYPE_RSA_KEY_PAIR + PSA_WANT_ALG_RSA_PSS"
    scripts/config.py full
    scripts/config.py set MBEDTLS_PSA_CRYPTO_CONFIG
    scripts/config.py set MBEDTLS_PSA_CRYPTO_DRIVERS
    scripts/config.py unset MBEDTLS_USE_PSA_CRYPTO
    scripts/config.py -f include/psa/crypto_config.h set PSA_WANT_ALG_RSA_PSS 1
    scripts/config.py -f include/psa/crypto_config.h set PSA_WANT_KEY_TYPE_RSA_KEY_PAIR 1
    # Need to define the correct symbol and include the test driver header path in order to build with the test driver
    make CC=gcc CFLAGS="$ASAN_CFLAGS -DPSA_CRYPTO_DRIVER_TEST -DMBEDTLS_PSA_ACCEL_KEY_TYPE_RSA_KEY_PAIR -I../tests/include -O2" LDFLAGS="$ASAN_CFLAGS"
}

# This should be renamed to test and updated once the accelerator RSA code is in place and ready to test.
component_build_psa_accel_key_type_rsa_public_key() {
    # full plus MBEDTLS_PSA_CRYPTO_CONFIG with PSA_WANT_KEY_TYPE_RSA_PUBLIC_KEY and PSA_WANT_ALG_RSA_PSS
    msg "build: full + MBEDTLS_PSA_CRYPTO_CONFIG + PSA_WANT_KEY_TYPE_RSA_PUBLIC_KEY + PSA_WANT_ALG_RSA_PSS"
    scripts/config.py full
    scripts/config.py set MBEDTLS_PSA_CRYPTO_CONFIG
    scripts/config.py set MBEDTLS_PSA_CRYPTO_DRIVERS
    scripts/config.py unset MBEDTLS_USE_PSA_CRYPTO
    scripts/config.py -f include/psa/crypto_config.h set PSA_WANT_ALG_RSA_PSS 1
    scripts/config.py -f include/psa/crypto_config.h set PSA_WANT_KEY_TYPE_RSA_PUBLIC_KEY 1
    # Need to define the correct symbol and include the test driver header path in order to build with the test driver
    make CC=gcc CFLAGS="$ASAN_CFLAGS -DPSA_CRYPTO_DRIVER_TEST -DMBEDTLS_PSA_ACCEL_KEY_TYPE_RSA_PUBLIC_KEY -I../tests/include -O2" LDFLAGS="$ASAN_CFLAGS"
}

component_test_check_params_functionality () {
    msg "build+test: MBEDTLS_CHECK_PARAMS functionality"
    scripts/config.py full # includes CHECK_PARAMS
    # Make MBEDTLS_PARAM_FAILED call mbedtls_param_failed().
    scripts/config.py unset MBEDTLS_CHECK_PARAMS_ASSERT
    make CC=gcc CFLAGS='-Werror -O1' all test
}

component_test_check_params_without_platform () {
    msg "build+test: MBEDTLS_CHECK_PARAMS without MBEDTLS_PLATFORM_C"
    scripts/config.py full # includes CHECK_PARAMS
    # Keep MBEDTLS_PARAM_FAILED as assert.
    scripts/config.py unset MBEDTLS_PLATFORM_EXIT_ALT
    scripts/config.py unset MBEDTLS_PLATFORM_TIME_ALT
    scripts/config.py unset MBEDTLS_PLATFORM_FPRINTF_ALT
    scripts/config.py unset MBEDTLS_PLATFORM_MEMORY
    scripts/config.py unset MBEDTLS_PLATFORM_NV_SEED_ALT
    scripts/config.py unset MBEDTLS_PLATFORM_PRINTF_ALT
    scripts/config.py unset MBEDTLS_PLATFORM_SNPRINTF_ALT
    scripts/config.py unset MBEDTLS_ENTROPY_NV_SEED
    scripts/config.py unset MBEDTLS_PLATFORM_C
    make CC=gcc CFLAGS='-Werror -O1' all test
}

component_test_check_params_silent () {
    msg "build+test: MBEDTLS_CHECK_PARAMS with alternative MBEDTLS_PARAM_FAILED()"
    scripts/config.py full # includes CHECK_PARAMS
    # Set MBEDTLS_PARAM_FAILED to nothing.
    sed -i 's/.*\(#define MBEDTLS_PARAM_FAILED( cond )\).*/\1/' "$CONFIG_H"
    make CC=gcc CFLAGS='-Werror -O1' all test
}

component_test_no_platform () {
    # Full configuration build, without platform support, file IO and net sockets.
    # This should catch missing mbedtls_printf definitions, and by disabling file
    # IO, it should catch missing '#include <stdio.h>'
    msg "build: full config except platform/fsio/net, make, gcc, C99" # ~ 30s
    scripts/config.py full
    scripts/config.py unset MBEDTLS_PLATFORM_C
    scripts/config.py unset MBEDTLS_NET_C
    scripts/config.py unset MBEDTLS_PLATFORM_MEMORY
    scripts/config.py unset MBEDTLS_PLATFORM_PRINTF_ALT
    scripts/config.py unset MBEDTLS_PLATFORM_FPRINTF_ALT
    scripts/config.py unset MBEDTLS_PLATFORM_SNPRINTF_ALT
    scripts/config.py unset MBEDTLS_PLATFORM_TIME_ALT
    scripts/config.py unset MBEDTLS_PLATFORM_EXIT_ALT
    scripts/config.py unset MBEDTLS_PLATFORM_NV_SEED_ALT
    scripts/config.py unset MBEDTLS_ENTROPY_NV_SEED
    scripts/config.py unset MBEDTLS_FS_IO
    scripts/config.py unset MBEDTLS_PSA_CRYPTO_SE_C
    scripts/config.py unset MBEDTLS_PSA_CRYPTO_STORAGE_C
    scripts/config.py unset MBEDTLS_PSA_ITS_FILE_C
    # Note, _DEFAULT_SOURCE needs to be defined for platforms using glibc version >2.19,
    # to re-enable platform integration features otherwise disabled in C99 builds
    make CC=gcc CFLAGS='-Werror -Wall -Wextra -std=c99 -pedantic -Os -D_DEFAULT_SOURCE' lib programs
    make CC=gcc CFLAGS='-Werror -Wall -Wextra -Os' test
}

component_build_no_std_function () {
    # catch compile bugs in _uninit functions
    msg "build: full config with NO_STD_FUNCTION, make, gcc" # ~ 30s
    scripts/config.py full
    scripts/config.py set MBEDTLS_PLATFORM_NO_STD_FUNCTIONS
    scripts/config.py unset MBEDTLS_ENTROPY_NV_SEED
    scripts/config.py unset MBEDTLS_PLATFORM_NV_SEED_ALT
    make CC=gcc CFLAGS='-Werror -Wall -Wextra -Os'
}

component_build_no_ssl_srv () {
    msg "build: full config except ssl_srv.c, make, gcc" # ~ 30s
    scripts/config.py full
    scripts/config.py unset MBEDTLS_SSL_SRV_C
    make CC=gcc CFLAGS='-Werror -Wall -Wextra -O1'
}

component_build_no_ssl_cli () {
    msg "build: full config except ssl_cli.c, make, gcc" # ~ 30s
    scripts/config.py full
    scripts/config.py unset MBEDTLS_SSL_CLI_C
    make CC=gcc CFLAGS='-Werror -Wall -Wextra -O1'
}

component_build_no_sockets () {
    # Note, C99 compliance can also be tested with the sockets support disabled,
    # as that requires a POSIX platform (which isn't the same as C99).
    msg "build: full config except net_sockets.c, make, gcc -std=c99 -pedantic" # ~ 30s
    scripts/config.py full
    scripts/config.py unset MBEDTLS_NET_C # getaddrinfo() undeclared, etc.
    scripts/config.py set MBEDTLS_NO_PLATFORM_ENTROPY # uses syscall() on GNU/Linux
    make CC=gcc CFLAGS='-Werror -Wall -Wextra -O1 -std=c99 -pedantic' lib
}

component_test_memory_buffer_allocator_backtrace () {
    msg "build: default config with memory buffer allocator and backtrace enabled"
    scripts/config.py set MBEDTLS_MEMORY_BUFFER_ALLOC_C
    scripts/config.py set MBEDTLS_PLATFORM_MEMORY
    scripts/config.py set MBEDTLS_MEMORY_BACKTRACE
    scripts/config.py set MBEDTLS_MEMORY_DEBUG
    CC=gcc cmake .
    make

    msg "test: MBEDTLS_MEMORY_BUFFER_ALLOC_C and MBEDTLS_MEMORY_BACKTRACE"
    make test
}

component_test_memory_buffer_allocator () {
    msg "build: default config with memory buffer allocator"
    scripts/config.py set MBEDTLS_MEMORY_BUFFER_ALLOC_C
    scripts/config.py set MBEDTLS_PLATFORM_MEMORY
    CC=gcc cmake .
    make

    msg "test: MBEDTLS_MEMORY_BUFFER_ALLOC_C"
    make test

    msg "test: ssl-opt.sh, MBEDTLS_MEMORY_BUFFER_ALLOC_C"
    # MBEDTLS_MEMORY_BUFFER_ALLOC is slow. Skip tests that tend to time out.
    if_build_succeeded tests/ssl-opt.sh -e '^DTLS proxy'
}

component_test_no_max_fragment_length () {
    # Run max fragment length tests with MFL disabled
    msg "build: default config except MFL extension (ASan build)" # ~ 30s
    scripts/config.py unset MBEDTLS_SSL_MAX_FRAGMENT_LENGTH
    CC=gcc cmake -D CMAKE_BUILD_TYPE:String=Asan .
    make

    msg "test: ssl-opt.sh, MFL-related tests"
    if_build_succeeded tests/ssl-opt.sh -f "Max fragment length"
}

component_test_asan_remove_peer_certificate () {
    msg "build: default config with MBEDTLS_SSL_KEEP_PEER_CERTIFICATE disabled (ASan build)"
    scripts/config.py unset MBEDTLS_SSL_KEEP_PEER_CERTIFICATE
    CC=gcc cmake -D CMAKE_BUILD_TYPE:String=Asan .
    make

    msg "test: !MBEDTLS_SSL_KEEP_PEER_CERTIFICATE"
    make test

    msg "test: ssl-opt.sh, !MBEDTLS_SSL_KEEP_PEER_CERTIFICATE"
    if_build_succeeded tests/ssl-opt.sh

    msg "test: compat.sh, !MBEDTLS_SSL_KEEP_PEER_CERTIFICATE"
    if_build_succeeded tests/compat.sh

    msg "test: context-info.sh, !MBEDTLS_SSL_KEEP_PEER_CERTIFICATE"
    if_build_succeeded tests/context-info.sh
}

component_test_no_max_fragment_length_small_ssl_out_content_len () {
    msg "build: no MFL extension, small SSL_OUT_CONTENT_LEN (ASan build)"
    scripts/config.py unset MBEDTLS_SSL_MAX_FRAGMENT_LENGTH
    scripts/config.py set MBEDTLS_SSL_IN_CONTENT_LEN 16384
    scripts/config.py set MBEDTLS_SSL_OUT_CONTENT_LEN 4096
    CC=gcc cmake -D CMAKE_BUILD_TYPE:String=Asan .
    make

    msg "test: MFL tests (disabled MFL extension case) & large packet tests"
    if_build_succeeded tests/ssl-opt.sh -f "Max fragment length\|Large buffer"

    msg "test: context-info.sh (disabled MFL extension case)"
    if_build_succeeded tests/context-info.sh
}

component_test_variable_ssl_in_out_buffer_len () {
    msg "build: MBEDTLS_SSL_VARIABLE_BUFFER_LENGTH enabled (ASan build)"
    scripts/config.py set MBEDTLS_SSL_VARIABLE_BUFFER_LENGTH
    CC=gcc cmake -D CMAKE_BUILD_TYPE:String=Asan .
    make

    msg "test: MBEDTLS_SSL_VARIABLE_BUFFER_LENGTH enabled"
    make test

    msg "test: ssl-opt.sh, MBEDTLS_SSL_VARIABLE_BUFFER_LENGTH enabled"
    if_build_succeeded tests/ssl-opt.sh

    msg "test: compat.sh, MBEDTLS_SSL_VARIABLE_BUFFER_LENGTH enabled"
    if_build_succeeded tests/compat.sh
}

component_test_variable_ssl_in_out_buffer_len_CID () {
    msg "build: MBEDTLS_SSL_VARIABLE_BUFFER_LENGTH and MBEDTLS_SSL_DTLS_CONNECTION_ID enabled (ASan build)"
    scripts/config.py set MBEDTLS_SSL_VARIABLE_BUFFER_LENGTH
    scripts/config.py set MBEDTLS_SSL_DTLS_CONNECTION_ID

    CC=gcc cmake -D CMAKE_BUILD_TYPE:String=Asan .
    make

    msg "test: MBEDTLS_SSL_VARIABLE_BUFFER_LENGTH and MBEDTLS_SSL_DTLS_CONNECTION_ID"
    make test

    msg "test: ssl-opt.sh, MBEDTLS_SSL_VARIABLE_BUFFER_LENGTH and MBEDTLS_SSL_DTLS_CONNECTION_ID enabled"
    if_build_succeeded tests/ssl-opt.sh

    msg "test: compat.sh, MBEDTLS_SSL_VARIABLE_BUFFER_LENGTH and MBEDTLS_SSL_DTLS_CONNECTION_ID enabled"
    if_build_succeeded tests/compat.sh
}

component_test_variable_ssl_in_out_buffer_len_record_splitting () {
    msg "build: MBEDTLS_SSL_VARIABLE_BUFFER_LENGTH and MBEDTLS_SSL_CBC_RECORD_SPLITTING enabled (ASan build)"
    scripts/config.py set MBEDTLS_SSL_VARIABLE_BUFFER_LENGTH
    scripts/config.py set MBEDTLS_SSL_CBC_RECORD_SPLITTING

    CC=gcc cmake -D CMAKE_BUILD_TYPE:String=Asan .
    make

    msg "test: MBEDTLS_SSL_VARIABLE_BUFFER_LENGTH and MBEDTLS_SSL_CBC_RECORD_SPLITTING"
    make test

    msg "test: ssl-opt.sh, MBEDTLS_SSL_VARIABLE_BUFFER_LENGTH and MBEDTLS_SSL_CBC_RECORD_SPLITTING enabled"
    if_build_succeeded tests/ssl-opt.sh

    msg "test: compat.sh, MBEDTLS_SSL_VARIABLE_BUFFER_LENGTH and MBEDTLS_SSL_CBC_RECORD_SPLITTING enabled"
    if_build_succeeded tests/compat.sh
}

component_test_ssl_alloc_buffer_and_mfl () {
    msg "build: default config with memory buffer allocator and MFL extension"
    scripts/config.py set MBEDTLS_MEMORY_BUFFER_ALLOC_C
    scripts/config.py set MBEDTLS_PLATFORM_MEMORY
    scripts/config.py set MBEDTLS_MEMORY_DEBUG
    scripts/config.py set MBEDTLS_SSL_MAX_FRAGMENT_LENGTH
    scripts/config.py set MBEDTLS_SSL_VARIABLE_BUFFER_LENGTH
    CC=gcc cmake .
    make

    msg "test: MBEDTLS_SSL_VARIABLE_BUFFER_LENGTH, MBEDTLS_MEMORY_BUFFER_ALLOC_C, MBEDTLS_MEMORY_DEBUG and MBEDTLS_SSL_MAX_FRAGMENT_LENGTH"
    make test

    msg "test: MBEDTLS_SSL_VARIABLE_BUFFER_LENGTH, MBEDTLS_MEMORY_BUFFER_ALLOC_C, MBEDTLS_MEMORY_DEBUG and MBEDTLS_SSL_MAX_FRAGMENT_LENGTH"
    if_build_succeeded tests/ssl-opt.sh -f "Handshake memory usage"
}

component_test_when_no_ciphersuites_have_mac () {
    msg "build: when no ciphersuites have MAC"
    scripts/config.py unset MBEDTLS_CIPHER_NULL_CIPHER
    scripts/config.py unset MBEDTLS_ARC4_C
    scripts/config.py unset MBEDTLS_CIPHER_MODE_CBC
    make

    msg "test: !MBEDTLS_SSL_SOME_MODES_USE_MAC"
    make test

    msg "test ssl-opt.sh: !MBEDTLS_SSL_SOME_MODES_USE_MAC"
    if_build_succeeded tests/ssl-opt.sh -f 'Default\|EtM' -e 'without EtM'
}

component_test_null_entropy () {
    msg "build: default config with  MBEDTLS_TEST_NULL_ENTROPY (ASan build)"
    scripts/config.py set MBEDTLS_TEST_NULL_ENTROPY
    scripts/config.py set MBEDTLS_NO_DEFAULT_ENTROPY_SOURCES
    scripts/config.py set MBEDTLS_ENTROPY_C
    scripts/config.py unset MBEDTLS_ENTROPY_NV_SEED
    scripts/config.py unset MBEDTLS_PLATFORM_NV_SEED_ALT
    scripts/config.py unset MBEDTLS_ENTROPY_HARDWARE_ALT
    scripts/config.py unset MBEDTLS_HAVEGE_C
    CC=gcc cmake -D CMAKE_BUILD_TYPE:String=Asan -D UNSAFE_BUILD=ON .
    make

    msg "test: MBEDTLS_TEST_NULL_ENTROPY - main suites (inc. selftests) (ASan build)"
    make test
}

component_test_no_date_time () {
    msg "build: default config without MBEDTLS_HAVE_TIME_DATE"
    scripts/config.py unset MBEDTLS_HAVE_TIME_DATE
    CC=gcc cmake
    make

    msg "test: !MBEDTLS_HAVE_TIME_DATE - main suites"
    make test
}

component_test_platform_calloc_macro () {
    msg "build: MBEDTLS_PLATFORM_{CALLOC/FREE}_MACRO enabled (ASan build)"
    scripts/config.py set MBEDTLS_PLATFORM_MEMORY
    scripts/config.py set MBEDTLS_PLATFORM_CALLOC_MACRO calloc
    scripts/config.py set MBEDTLS_PLATFORM_FREE_MACRO   free
    CC=gcc cmake -D CMAKE_BUILD_TYPE:String=Asan .
    make

    msg "test: MBEDTLS_PLATFORM_{CALLOC/FREE}_MACRO enabled (ASan build)"
    make test
}

component_test_malloc_0_null () {
    msg "build: malloc(0) returns NULL (ASan+UBSan build)"
    scripts/config.py full
    make CC=gcc CFLAGS="'-DMBEDTLS_CONFIG_FILE=\"$PWD/tests/configs/config-wrapper-malloc-0-null.h\"' $ASAN_CFLAGS -O" LDFLAGS="$ASAN_CFLAGS"

    msg "test: malloc(0) returns NULL (ASan+UBSan build)"
    make test

    msg "selftest: malloc(0) returns NULL (ASan+UBSan build)"
    # Just the calloc selftest. "make test" ran the others as part of the
    # test suites.
    if_build_succeeded programs/test/selftest calloc

    msg "test ssl-opt.sh: malloc(0) returns NULL (ASan+UBSan build)"
    # Run a subset of the tests. The choice is a balance between coverage
    # and time (including time indirectly wasted due to flaky tests).
    # The current choice is to skip tests whose description includes
    # "proxy", which is an approximation of skipping tests that use the
    # UDP proxy, which tend to be slower and flakier.
    if_build_succeeded tests/ssl-opt.sh -e 'proxy'
}

component_test_aes_fewer_tables () {
    msg "build: default config with AES_FEWER_TABLES enabled"
    scripts/config.py set MBEDTLS_AES_FEWER_TABLES
    make CC=gcc CFLAGS='-Werror -Wall -Wextra'

    msg "test: AES_FEWER_TABLES"
    make test
}

component_test_aes_rom_tables () {
    msg "build: default config with AES_ROM_TABLES enabled"
    scripts/config.py set MBEDTLS_AES_ROM_TABLES
    make CC=gcc CFLAGS='-Werror -Wall -Wextra'

    msg "test: AES_ROM_TABLES"
    make test
}

component_test_aes_fewer_tables_and_rom_tables () {
    msg "build: default config with AES_ROM_TABLES and AES_FEWER_TABLES enabled"
    scripts/config.py set MBEDTLS_AES_FEWER_TABLES
    scripts/config.py set MBEDTLS_AES_ROM_TABLES
    make CC=gcc CFLAGS='-Werror -Wall -Wextra'

    msg "test: AES_FEWER_TABLES + AES_ROM_TABLES"
    make test
}

component_test_ctr_drbg_aes_256_sha_256 () {
    msg "build: full + MBEDTLS_ENTROPY_FORCE_SHA256 (ASan build)"
    scripts/config.py full
    scripts/config.py unset MBEDTLS_MEMORY_BUFFER_ALLOC_C
    scripts/config.py set MBEDTLS_ENTROPY_FORCE_SHA256
    CC=gcc cmake -D CMAKE_BUILD_TYPE:String=Asan .
    make

    msg "test: full + MBEDTLS_ENTROPY_FORCE_SHA256 (ASan build)"
    make test
}

component_test_ctr_drbg_aes_128_sha_512 () {
    msg "build: full + MBEDTLS_CTR_DRBG_USE_128_BIT_KEY (ASan build)"
    scripts/config.py full
    scripts/config.py unset MBEDTLS_MEMORY_BUFFER_ALLOC_C
    scripts/config.py set MBEDTLS_CTR_DRBG_USE_128_BIT_KEY
    CC=gcc cmake -D CMAKE_BUILD_TYPE:String=Asan .
    make

    msg "test: full + MBEDTLS_CTR_DRBG_USE_128_BIT_KEY (ASan build)"
    make test
}

component_test_ctr_drbg_aes_128_sha_256 () {
    msg "build: full + MBEDTLS_CTR_DRBG_USE_128_BIT_KEY + MBEDTLS_ENTROPY_FORCE_SHA256 (ASan build)"
    scripts/config.py full
    scripts/config.py unset MBEDTLS_MEMORY_BUFFER_ALLOC_C
    scripts/config.py set MBEDTLS_CTR_DRBG_USE_128_BIT_KEY
    scripts/config.py set MBEDTLS_ENTROPY_FORCE_SHA256
    CC=gcc cmake -D CMAKE_BUILD_TYPE:String=Asan .
    make

    msg "test: full + MBEDTLS_CTR_DRBG_USE_128_BIT_KEY + MBEDTLS_ENTROPY_FORCE_SHA256 (ASan build)"
    make test
}

component_test_se_default () {
    msg "build: default config + MBEDTLS_PSA_CRYPTO_SE_C"
    scripts/config.py set MBEDTLS_PSA_CRYPTO_SE_C
    make CC=clang CFLAGS="$ASAN_CFLAGS -Os" LDFLAGS="$ASAN_CFLAGS"

    msg "test: default config + MBEDTLS_PSA_CRYPTO_SE_C"
    make test
}

component_test_psa_crypto_drivers () {
    msg "build: MBEDTLS_PSA_CRYPTO_DRIVERS w/ driver hooks"
    scripts/config.py set MBEDTLS_PSA_CRYPTO_DRIVERS
    # Need to define the correct symbol and include the test driver header path in order to build with the test driver
    make CC=gcc CFLAGS="$ASAN_CFLAGS -DPSA_CRYPTO_DRIVER_TEST -I../tests/include -O2" LDFLAGS="$ASAN_CFLAGS"

    msg "test: MBEDTLS_PSA_CRYPTO_DRIVERS, signature"
    make test
}

component_test_make_shared () {
    msg "build/test: make shared" # ~ 40s
    make SHARED=1 all check
    ldd programs/util/strerror | grep libmbedcrypto
}

component_test_cmake_shared () {
    msg "build/test: cmake shared" # ~ 2min
    cmake -DUSE_SHARED_MBEDTLS_LIBRARY=On .
    make
    ldd programs/util/strerror | grep libmbedcrypto
    make test
}

test_build_opt () {
    info=$1 cc=$2; shift 2
    for opt in "$@"; do
          msg "build/test: $cc $opt, $info" # ~ 30s
          make CC="$cc" CFLAGS="$opt -std=c99 -pedantic -Wall -Wextra -Werror"
          # We're confident enough in compilers to not run _all_ the tests,
          # but at least run the unit tests. In particular, runs with
          # optimizations use inline assembly whereas runs with -O0
          # skip inline assembly.
          make test # ~30s
          make clean
    done
}

component_test_clang_opt () {
    scripts/config.py full
    test_build_opt 'full config' clang -O0 -Os -O2
}

component_test_gcc_opt () {
    scripts/config.py full
    test_build_opt 'full config' gcc -O0 -Os -O2
}

component_build_mbedtls_config_file () {
    msg "build: make with MBEDTLS_CONFIG_FILE" # ~40s
    # Use the full config so as to catch a maximum of places where
    # the check of MBEDTLS_CONFIG_FILE might be missing.
    scripts/config.py full
    sed 's!"check_config.h"!"mbedtls/check_config.h"!' <"$CONFIG_H" >full_config.h
    echo '#error "MBEDTLS_CONFIG_FILE is not working"' >"$CONFIG_H"
    make CFLAGS="-I '$PWD' -DMBEDTLS_CONFIG_FILE='\"full_config.h\"'"
    rm -f full_config.h
}

component_test_m32_o0 () {
    # Build once with -O0, to compile out the i386 specific inline assembly
    msg "build: i386, make, gcc -O0 (ASan build)" # ~ 30s
    scripts/config.py full
    make CC=gcc CFLAGS="$ASAN_CFLAGS -m32 -O0" LDFLAGS="-m32 $ASAN_CFLAGS"

    msg "test: i386, make, gcc -O0 (ASan build)"
    make test
}
support_test_m32_o0 () {
    case $(uname -m) in
        *64*) true;;
        *) false;;
    esac
}

component_test_m32_o1 () {
    # Build again with -O1, to compile in the i386 specific inline assembly
    msg "build: i386, make, gcc -O1 (ASan build)" # ~ 30s
    scripts/config.py full
    make CC=gcc CFLAGS="$ASAN_CFLAGS -m32 -O1" LDFLAGS="-m32 $ASAN_CFLAGS"

    msg "test: i386, make, gcc -O1 (ASan build)"
    make test

    msg "test ssl-opt.sh, i386, make, gcc-O1"
    if_build_succeeded tests/ssl-opt.sh
}
support_test_m32_o1 () {
    support_test_m32_o0 "$@"
}

component_test_m32_everest () {
    msg "build: i386, Everest ECDH context (ASan build)" # ~ 6 min
    scripts/config.py unset MBEDTLS_ECDH_LEGACY_CONTEXT
    scripts/config.py set MBEDTLS_ECDH_VARIANT_EVEREST_ENABLED
    make CC=gcc CFLAGS="$ASAN_CFLAGS -m32 -O2" LDFLAGS="-m32 $ASAN_CFLAGS"

    msg "test: i386, Everest ECDH context - main suites (inc. selftests) (ASan build)" # ~ 50s
    make test

    msg "test: i386, Everest ECDH context - ECDH-related part of ssl-opt.sh (ASan build)" # ~ 5s
    if_build_succeeded tests/ssl-opt.sh -f ECDH

    msg "test: i386, Everest ECDH context - compat.sh with some ECDH ciphersuites (ASan build)" # ~ 3 min
    # Exclude some symmetric ciphers that are redundant here to gain time.
    if_build_succeeded tests/compat.sh -f ECDH -V NO -e 'ARCFOUR\|ARIA\|CAMELLIA\|CHACHA\|DES\|RC4'
}
support_test_m32_everest () {
    support_test_m32_o0 "$@"
}

component_test_mx32 () {
    msg "build: 64-bit ILP32, make, gcc" # ~ 30s
    scripts/config.py full
    make CC=gcc CFLAGS='-Werror -Wall -Wextra -mx32' LDFLAGS='-mx32'

    msg "test: 64-bit ILP32, make, gcc"
    make test
}
support_test_mx32 () {
    case $(uname -m) in
        amd64|x86_64) true;;
        *) false;;
    esac
}

component_test_min_mpi_window_size () {
    msg "build: Default + MBEDTLS_MPI_WINDOW_SIZE=1 (ASan build)" # ~ 10s
    scripts/config.py set MBEDTLS_MPI_WINDOW_SIZE 1
    CC=gcc cmake -D CMAKE_BUILD_TYPE:String=Asan .
    make

    msg "test: MBEDTLS_MPI_WINDOW_SIZE=1 - main suites (inc. selftests) (ASan build)" # ~ 10s
    make test
}

component_test_have_int32 () {
    msg "build: gcc, force 32-bit bignum limbs"
    scripts/config.py unset MBEDTLS_HAVE_ASM
    scripts/config.py unset MBEDTLS_AESNI_C
    scripts/config.py unset MBEDTLS_PADLOCK_C
    make CC=gcc CFLAGS='-Werror -Wall -Wextra -DMBEDTLS_HAVE_INT32'

    msg "test: gcc, force 32-bit bignum limbs"
    make test
}

component_test_have_int64 () {
    msg "build: gcc, force 64-bit bignum limbs"
    scripts/config.py unset MBEDTLS_HAVE_ASM
    scripts/config.py unset MBEDTLS_AESNI_C
    scripts/config.py unset MBEDTLS_PADLOCK_C
    make CC=gcc CFLAGS='-Werror -Wall -Wextra -DMBEDTLS_HAVE_INT64'

    msg "test: gcc, force 64-bit bignum limbs"
    make test
}

component_test_no_udbl_division () {
    msg "build: MBEDTLS_NO_UDBL_DIVISION native" # ~ 10s
    scripts/config.py full
    scripts/config.py set MBEDTLS_NO_UDBL_DIVISION
    make CFLAGS='-Werror -O1'

    msg "test: MBEDTLS_NO_UDBL_DIVISION native" # ~ 10s
    make test
}

component_test_no_64bit_multiplication () {
    msg "build: MBEDTLS_NO_64BIT_MULTIPLICATION native" # ~ 10s
    scripts/config.py full
    scripts/config.py set MBEDTLS_NO_64BIT_MULTIPLICATION
    make CFLAGS='-Werror -O1'

    msg "test: MBEDTLS_NO_64BIT_MULTIPLICATION native" # ~ 10s
    make test
}

component_test_no_strings () {
    msg "build: no strings" # ~10s
    scripts/config.py full
    # Disable options that activate a large amount of string constants.
    scripts/config.py unset MBEDTLS_DEBUG_C
    scripts/config.py unset MBEDTLS_ERROR_C
    scripts/config.py set MBEDTLS_ERROR_STRERROR_DUMMY
    scripts/config.py unset MBEDTLS_VERSION_FEATURES
    make CFLAGS='-Werror -Os'

    msg "test: no strings" # ~ 10s
    make test
}

component_build_arm_none_eabi_gcc () {
    msg "build: ${ARM_NONE_EABI_GCC_PREFIX}gcc -O1" # ~ 10s
    scripts/config.py baremetal
    make CC="${ARM_NONE_EABI_GCC_PREFIX}gcc" AR="${ARM_NONE_EABI_GCC_PREFIX}ar" LD="${ARM_NONE_EABI_GCC_PREFIX}ld" CFLAGS='-std=c99 -Werror -Wall -Wextra -O1' lib

    msg "size: ${ARM_NONE_EABI_GCC_PREFIX}gcc -O1"
    ${ARM_NONE_EABI_GCC_PREFIX}size library/*.o
}

component_build_arm_none_eabi_gcc_arm5vte () {
    msg "build: ${ARM_NONE_EABI_GCC_PREFIX}gcc -march=arm5vte" # ~ 10s
    scripts/config.py baremetal
    # Build for a target platform that's close to what Debian uses
    # for its "armel" distribution (https://wiki.debian.org/ArmEabiPort).
    # See https://github.com/ARMmbed/mbedtls/pull/2169 and comments.
    # It would be better to build with arm-linux-gnueabi-gcc but
    # we don't have that on our CI at this time.
    make CC="${ARM_NONE_EABI_GCC_PREFIX}gcc" AR="${ARM_NONE_EABI_GCC_PREFIX}ar" CFLAGS='-std=c99 -Werror -Wall -Wextra -march=armv5te -O1' LDFLAGS='-march=armv5te' SHELL='sh -x' lib

    msg "size: ${ARM_NONE_EABI_GCC_PREFIX}gcc -march=armv5te -O1"
    ${ARM_NONE_EABI_GCC_PREFIX}size library/*.o
}

component_build_arm_none_eabi_gcc_m0plus () {
    msg "build: ${ARM_NONE_EABI_GCC_PREFIX}gcc -mthumb -mcpu=cortex-m0plus" # ~ 10s
    scripts/config.py baremetal
    make CC="${ARM_NONE_EABI_GCC_PREFIX}gcc" AR="${ARM_NONE_EABI_GCC_PREFIX}ar" LD="${ARM_NONE_EABI_GCC_PREFIX}ld" CFLAGS='-std=c99 -Werror -Wall -Wextra -mthumb -mcpu=cortex-m0plus -Os' lib

    msg "size: ${ARM_NONE_EABI_GCC_PREFIX}gcc -mthumb -mcpu=cortex-m0plus -Os"
    ${ARM_NONE_EABI_GCC_PREFIX}size library/*.o
}

component_build_arm_none_eabi_gcc_no_udbl_division () {
    msg "build: ${ARM_NONE_EABI_GCC_PREFIX}gcc -DMBEDTLS_NO_UDBL_DIVISION, make" # ~ 10s
    scripts/config.py baremetal
    scripts/config.py set MBEDTLS_NO_UDBL_DIVISION
    make CC="${ARM_NONE_EABI_GCC_PREFIX}gcc" AR="${ARM_NONE_EABI_GCC_PREFIX}ar" LD="${ARM_NONE_EABI_GCC_PREFIX}ld" CFLAGS='-std=c99 -Werror -Wall -Wextra' lib
    echo "Checking that software 64-bit division is not required"
    if_build_succeeded not grep __aeabi_uldiv library/*.o
}

component_build_arm_none_eabi_gcc_no_64bit_multiplication () {
    msg "build: ${ARM_NONE_EABI_GCC_PREFIX}gcc MBEDTLS_NO_64BIT_MULTIPLICATION, make" # ~ 10s
    scripts/config.py baremetal
    scripts/config.py set MBEDTLS_NO_64BIT_MULTIPLICATION
    make CC="${ARM_NONE_EABI_GCC_PREFIX}gcc" AR="${ARM_NONE_EABI_GCC_PREFIX}ar" LD="${ARM_NONE_EABI_GCC_PREFIX}ld" CFLAGS='-std=c99 -Werror -O1 -march=armv6-m -mthumb' lib
    echo "Checking that software 64-bit multiplication is not required"
    if_build_succeeded not grep __aeabi_lmul library/*.o
}

component_build_armcc () {
    msg "build: ARM Compiler 5"
    scripts/config.py baremetal
    make CC="$ARMC5_CC" AR="$ARMC5_AR" WARNING_CFLAGS='--strict --c99' lib

    msg "size: ARM Compiler 5"
    "$ARMC5_FROMELF" -z library/*.o

    make clean

    # ARM Compiler 6 - Target ARMv7-A
    armc6_build_test "--target=arm-arm-none-eabi -march=armv7-a"

    # ARM Compiler 6 - Target ARMv7-M
    armc6_build_test "--target=arm-arm-none-eabi -march=armv7-m"

    # ARM Compiler 6 - Target ARMv8-A - AArch32
    armc6_build_test "--target=arm-arm-none-eabi -march=armv8.2-a"

    # ARM Compiler 6 - Target ARMv8-M
    armc6_build_test "--target=arm-arm-none-eabi -march=armv8-m.main"

    # ARM Compiler 6 - Target ARMv8-A - AArch64
    armc6_build_test "--target=aarch64-arm-none-eabi -march=armv8.2-a"
}

component_build_ssl_hw_record_accel() {
    msg "build: default config with MBEDTLS_SSL_HW_RECORD_ACCEL enabled"
    scripts/config.pl set MBEDTLS_SSL_HW_RECORD_ACCEL
    make CFLAGS='-Werror -O1'
}

component_test_allow_sha1 () {
    msg "build: allow SHA1 in certificates by default"
    scripts/config.py set MBEDTLS_TLS_DEFAULT_ALLOW_SHA1_IN_CERTIFICATES
    make CFLAGS='-Werror -Wall -Wextra'
    msg "test: allow SHA1 in certificates by default"
    make test
    if_build_succeeded tests/ssl-opt.sh -f SHA-1
}

component_test_tls13_experimental () {
    msg "build: default config with MBEDTLS_SSL_PROTO_TLS1_3_EXPERIMENTAL enabled"
    scripts/config.pl set MBEDTLS_SSL_PROTO_TLS1_3_EXPERIMENTAL
    CC=gcc cmake -D CMAKE_BUILD_TYPE:String=Asan .
    make
    msg "test: default config with MBEDTLS_SSL_PROTO_TLS1_3_EXPERIMENTAL enabled"
    make test
}

component_build_mingw () {
    msg "build: Windows cross build - mingw64, make (Link Library)" # ~ 30s
    make CC=i686-w64-mingw32-gcc AR=i686-w64-mingw32-ar LD=i686-w64-minggw32-ld CFLAGS='-Werror -Wall -Wextra' WINDOWS_BUILD=1 lib programs

    # note Make tests only builds the tests, but doesn't run them
    make CC=i686-w64-mingw32-gcc AR=i686-w64-mingw32-ar LD=i686-w64-minggw32-ld CFLAGS='-Werror' WINDOWS_BUILD=1 tests
    make WINDOWS_BUILD=1 clean

    msg "build: Windows cross build - mingw64, make (DLL)" # ~ 30s
    make CC=i686-w64-mingw32-gcc AR=i686-w64-mingw32-ar LD=i686-w64-minggw32-ld CFLAGS='-Werror -Wall -Wextra' WINDOWS_BUILD=1 SHARED=1 lib programs
    make CC=i686-w64-mingw32-gcc AR=i686-w64-mingw32-ar LD=i686-w64-minggw32-ld CFLAGS='-Werror -Wall -Wextra' WINDOWS_BUILD=1 SHARED=1 tests
    make WINDOWS_BUILD=1 clean
}
support_build_mingw() {
    case $(i686-w64-mingw32-gcc -dumpversion) in
        [0-5]*) false;;
        *) true;;
    esac
}

component_test_memsan () {
    msg "build: MSan (clang)" # ~ 1 min 20s
    scripts/config.py unset MBEDTLS_AESNI_C # memsan doesn't grok asm
    CC=clang cmake -D CMAKE_BUILD_TYPE:String=MemSan .
    make

    msg "test: main suites (MSan)" # ~ 10s
    make test

    msg "test: ssl-opt.sh (MSan)" # ~ 1 min
    if_build_succeeded tests/ssl-opt.sh

    # Optional part(s)

    if [ "$MEMORY" -gt 0 ]; then
        msg "test: compat.sh (MSan)" # ~ 6 min 20s
        if_build_succeeded tests/compat.sh
    fi
}

component_test_valgrind () {
    msg "build: Release (clang)"
    CC=clang cmake -D CMAKE_BUILD_TYPE:String=Release .
    make

    msg "test: main suites valgrind (Release)"
    make memcheck

    # Optional parts (slow; currently broken on OS X because programs don't
    # seem to receive signals under valgrind on OS X).
    if [ "$MEMORY" -gt 0 ]; then
        msg "test: ssl-opt.sh --memcheck (Release)"
        if_build_succeeded tests/ssl-opt.sh --memcheck
    fi

    if [ "$MEMORY" -gt 1 ]; then
        msg "test: compat.sh --memcheck (Release)"
        if_build_succeeded tests/compat.sh --memcheck
    fi

    if [ "$MEMORY" -gt 0 ]; then
        msg "test: context-info.sh --memcheck (Release)"
        if_build_succeeded tests/context-info.sh --memcheck
    fi
}

component_test_cmake_out_of_source () {
    msg "build: cmake 'out-of-source' build"
    MBEDTLS_ROOT_DIR="$PWD"
    mkdir "$OUT_OF_SOURCE_DIR"
    cd "$OUT_OF_SOURCE_DIR"
    cmake "$MBEDTLS_ROOT_DIR"
    make

    msg "test: cmake 'out-of-source' build"
    make test
    # Test an SSL option that requires an auxiliary script in test/scripts/.
    # Also ensure that there are no error messages such as
    # "No such file or directory", which would indicate that some required
    # file is missing (ssl-opt.sh tolerates the absence of some files so
    # may exit with status 0 but emit errors).
    if_build_succeeded ./tests/ssl-opt.sh -f 'Fallback SCSV: beginning of list' 2>ssl-opt.err
    if [ -s ssl-opt.err ]; then
        cat ssl-opt.err >&2
        record_status [ ! -s ssl-opt.err ]
        rm ssl-opt.err
    fi
    cd "$MBEDTLS_ROOT_DIR"
    rm -rf "$OUT_OF_SOURCE_DIR"
    unset MBEDTLS_ROOT_DIR
}

component_test_cmake_as_subdirectory () {
    msg "build: cmake 'as-subdirectory' build"
    MBEDTLS_ROOT_DIR="$PWD"

    cd programs/test/cmake_subproject
    cmake .
    make
    if_build_succeeded ./cmake_subproject

    cd "$MBEDTLS_ROOT_DIR"
    unset MBEDTLS_ROOT_DIR
}

component_test_zeroize () {
    # Test that the function mbedtls_platform_zeroize() is not optimized away by
    # different combinations of compilers and optimization flags by using an
    # auxiliary GDB script. Unfortunately, GDB does not return error values to the
    # system in all cases that the script fails, so we must manually search the
    # output to check whether the pass string is present and no failure strings
    # were printed.

    # Don't try to disable ASLR. We don't care about ASLR here. We do care
    # about a spurious message if Gdb tries and fails, so suppress that.
    gdb_disable_aslr=
    if [ -z "$(gdb -batch -nw -ex 'set disable-randomization off' 2>&1)" ]; then
        gdb_disable_aslr='set disable-randomization off'
    fi

    for optimization_flag in -O2 -O3 -Ofast -Os; do
        for compiler in clang gcc; do
            msg "test: $compiler $optimization_flag, mbedtls_platform_zeroize()"
            make programs CC="$compiler" DEBUG=1 CFLAGS="$optimization_flag"
            if_build_succeeded gdb -ex "$gdb_disable_aslr" -x tests/scripts/test_zeroize.gdb -nw -batch -nx 2>&1 | tee test_zeroize.log
            if_build_succeeded grep "The buffer was correctly zeroized" test_zeroize.log
            if_build_succeeded not grep -i "error" test_zeroize.log
            rm -f test_zeroize.log
            make clean
        done
    done

    unset gdb_disable_aslr
}

component_check_python_files () {
    msg "Lint: Python scripts"
    record_status tests/scripts/check-python-files.sh
}

component_check_generate_test_code () {
    msg "uint test: generate_test_code.py"
    # unittest writes out mundane stuff like number or tests run on stderr.
    # Our convention is to reserve stderr for actual errors, and write
    # harmless info on stdout so it can be suppress with --quiet.
    record_status ./tests/scripts/test_generate_test_code.py 2>&1
}

################################################################
#### Termination
################################################################

post_report () {
    msg "Done, cleaning up"
    cleanup

    final_report
}



################################################################
#### Run all the things
################################################################

# Run one component and clean up afterwards.
run_component () {
    # Back up the configuration in case the component modifies it.
    # The cleanup function will restore it.
    cp -p "$CONFIG_H" "$CONFIG_BAK"
    cp -p "$CRYPTO_CONFIG_H" "$CRYPTO_CONFIG_BAK"
    current_component="$1"
    export MBEDTLS_TEST_CONFIGURATION="$current_component"

    # Unconditionally create a seedfile that's sufficiently long.
    # Do this before each component, because a previous component may
    # have messed it up or shortened it.
    redirect_err dd if=/dev/urandom of=./tests/seedfile bs=64 count=1

    # Run the component code.
    if [ $QUIET -eq 1 ]; then
        # msg() is silenced, so just print the component name here
        echo "${current_component#component_}"
    fi
    redirect_out "$@"

    # Restore the build tree to a clean state.
    cleanup
    unset current_component
}

# Preliminary setup
pre_check_environment
pre_initialize_variables
pre_parse_command_line "$@"

pre_check_git

build_status=0
if [ $KEEP_GOING -eq 1 ]; then
    pre_setup_keep_going
else
    record_status () {
        "$@"
    }
fi
pre_setup_quiet_redirect
pre_prepare_outcome_file
pre_print_configuration
pre_check_tools
cleanup

# Run the requested tests.
for component in $RUN_COMPONENTS; do
    run_component "component_$component"
done

# We're done.
post_report
