#!/bin/bash
#
# Copyright (c) 2025 The OpenThread Authors.
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are met:
# 1. Redistributions of source code must retain the above copyright
#    notice, this list of conditions and the following disclaimer.
# 2. Redistributions in binary form must reproduce the above copyright
#    notice, this list of conditions and the following disclaimer in the
#    documentation and/or other materials provided with the distribution.
# 3. Neither the name of the copyright holder nor the
#    names of its contributors may be used to endorse or promote products
#    derived from this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
# AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
# IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
# ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
# LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
# CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
# SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
# INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
# CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
# ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
# POSSIBILITY OF SUCH DAMAGE.
#
# This script downloads and installs a specific version of clang-format and clang-tidy.

set -euo pipefail

readonly LLVM_VERSION="19.1.7"
readonly LLVM_MAJOR_VERSION="19"
readonly LLVM_ARCHIVE="LLVM-${LLVM_VERSION}-Linux-X64.tar.xz"
readonly LLVM_URL="https://github.com/llvm/llvm-project/releases/download/llvmorg-${LLVM_VERSION}/${LLVM_ARCHIVE}"
readonly LLVM_SHA256="4a5ec53951a584ed36f80240f6fbf8fdd46b4cf6c7ee87cc2d5018dc37caf679"
readonly INSTALL_DIR="/opt/llvm-${LLVM_VERSION}"
readonly BIN_DIR="/usr/local/bin"
readonly LLVM_TOOLS=(
    clang-format
    clang-tidy
    clang-apply-replacements
    run-clang-tidy
)

usage()
{
    cat <<'EOF'
Usage:
  install-llvm.sh [options]

Options:
  --force       Re-download and reinstall even if LLVM is already installed
  -h, --help    Display this help text
EOF
}

die()
{
    echo "ERROR: $*" >&2
    exit 1
}

require_privilege()
{
    if [[ ${EUID} -ne 0 ]] && ! command -v sudo >/dev/null 2>&1; then
        die "Root privileges or sudo are required"
    fi
}

run_privileged()
{
    if [[ ${EUID} -eq 0 ]]; then
        "$@"
    else
        sudo "$@"
    fi
}

validate_install_dir()
{
    local dir="$1"

    case "${dir}" in
        "" | "/" | "." | ".." | */. | */.. | "/bin" | "/boot" | "/dev" | "/etc" | "/home" | "/lib" | "/lib64" | "/opt" | "/proc" | "/root" | "/run" | "/sbin" | "/srv" | "/sys" | "/tmp" | "/usr" | "/usr/bin" | "/usr/sbin" | "/usr/local" | "/usr/local/bin" | "/usr/local/sbin" | "/var" | "/var/tmp")
            die "Invalid installation directory: '${dir}'"
            ;;
    esac
}

has_installed_llvm()
{
    local dir="$1"
    local tool

    for tool in "${LLVM_TOOLS[@]}"; do
        [[ -x "${dir}/bin/${tool}" ]] || return 1
    done

    return 0
}

symlinks_up_to_date()
{
    local tool
    local link_path
    local link_target

    for tool in "${LLVM_TOOLS[@]}"; do
        link_path="${BIN_DIR}/${tool}-${LLVM_MAJOR_VERSION}"
        [[ -L ${link_path} ]] || return 1
        link_target="$(readlink "${link_path}")" || return 1
        [[ ${link_target} == "${INSTALL_DIR}/bin/${tool}" ]] || return 1
    done

    return 0
}

ensure_symlinks()
{
    local tool
    local had_usr_local=0
    local had_bin_dir=0

    if symlinks_up_to_date; then
        return 0
    fi

    require_privilege

    echo "Creating symlinks in '${BIN_DIR}'..."
    if [[ -d /usr/local ]]; then
        had_usr_local=1
    fi
    if [[ -d ${BIN_DIR} ]]; then
        had_bin_dir=1
    fi

    if [[ ${had_bin_dir} -eq 0 ]]; then
        run_privileged mkdir -p "${BIN_DIR}"
        if [[ ${had_usr_local} -eq 0 ]]; then
            run_privileged chmod 0755 /usr/local
        fi
        run_privileged chmod 0755 "${BIN_DIR}"
    fi

    for tool in "${LLVM_TOOLS[@]}"; do
        run_privileged ln -snf "${INSTALL_DIR}/bin/${tool}" "${BIN_DIR}/${tool}-${LLVM_MAJOR_VERSION}"
    done
}

check_prerequisites()
{
    if ! command -v curl >/dev/null 2>&1 && ! command -v wget >/dev/null 2>&1; then
        die "Neither curl nor wget is available"
    fi

    if ! command -v sha256sum >/dev/null 2>&1 && ! command -v shasum >/dev/null 2>&1; then
        die "Neither sha256sum nor shasum is available for SHA-256 verification"
    fi

    command -v tar >/dev/null 2>&1 || die "tar is required to extract ${LLVM_ARCHIVE}"
    command -v xz >/dev/null 2>&1 || die "xz is required to extract ${LLVM_ARCHIVE} (install xz-utils)"
}

download_archive()
{
    local url="$1"
    local archive_path="$2"

    if command -v curl >/dev/null 2>&1; then
        curl --proto '=https' --tlsv1.2 -fsSL \
            --connect-timeout 15 --max-time 1800 \
            --retry 3 --retry-delay 2 --retry-max-time 1800 \
            --speed-limit 102400 --speed-time 30 \
            -o "${archive_path}" "${url}"
    elif command -v wget >/dev/null 2>&1; then
        [[ ${url} == https://* ]] || die "Refusing non-HTTPS download URL: ${url}"
        wget -nv --https-only --secure-protocol=TLSv1_2 \
            --tries=4 --timeout=15 --waitretry=2 \
            -O "${archive_path}" "${url}"
    else
        die "Neither curl nor wget is available"
    fi
}

verify_sha256()
{
    local archive_path="$1"
    local expected_sha256="$2"

    if command -v sha256sum >/dev/null 2>&1; then
        printf '%s  %s\n' "${expected_sha256}" "${archive_path}" | sha256sum -c -
    elif command -v shasum >/dev/null 2>&1; then
        printf '%s  %s\n' "${expected_sha256}" "${archive_path}" | shasum -a 256 -c -
    else
        die "Neither sha256sum nor shasum is available for SHA-256 verification"
    fi
}

main()
{
    local force=0
    local host_os
    local host_arch
    local tmp_parent

    while [[ $# -gt 0 ]]; do
        case "$1" in
            --force)
                force=1
                shift
                ;;
            -h | --help)
                usage
                return 0
                ;;
            -*)
                die "Unknown option: $1"
                ;;
            *)
                die "Unexpected argument: $1"
                ;;
        esac
    done

    host_os="$(uname -s)"
    if [[ ${host_os} != "Linux" ]]; then
        die "Unsupported host OS: ${host_os} (only Linux is supported)"
    fi

    host_arch="$(uname -m)"
    case "${host_arch}" in
        x86_64 | amd64) ;;
        *)
            die "Unsupported architecture: ${host_arch}"
            ;;
    esac

    validate_install_dir "${INSTALL_DIR}"

    if [[ ${force} -eq 0 ]] && has_installed_llvm "${INSTALL_DIR}"; then
        ensure_symlinks
        echo "LLVM ${LLVM_VERSION} already installed at '${INSTALL_DIR}'; skipping download."
        return 0
    fi

    require_privilege
    check_prerequisites

    tmp_parent="/var/tmp"
    if [[ ! -d ${tmp_parent} || ! -w ${tmp_parent} ]]; then
        tmp_parent="${TMPDIR:-/tmp}"
    fi

    (
        umask 022
        stage_dir=""
        trap 'if [[ -n ${stage_dir:-} ]]; then rm -rf "${stage_dir}"; fi' EXIT
        trap 'exit 130' INT
        trap 'exit 143' TERM
        trap 'exit 129' HUP
        stage_dir="$(mktemp -d "${tmp_parent}/.ot-llvm.XXXXXX")"

        archive_path="${stage_dir}/${LLVM_ARCHIVE}"
        echo "Downloading LLVM ${LLVM_VERSION} from '${LLVM_URL}'..."
        download_archive "${LLVM_URL}" "${archive_path}"
        verify_sha256 "${archive_path}" "${LLVM_SHA256}"

        if [[ ! -d /opt ]]; then
            run_privileged mkdir -p /opt
            run_privileged chmod 0755 /opt
        fi

        (
            opt_stage_dir=""
            trap 'if [[ -n ${opt_stage_dir:-} ]]; then run_privileged rm -rf "${opt_stage_dir}"; fi' EXIT
            trap 'exit 130' INT
            trap 'exit 143' TERM
            trap 'exit 129' HUP
            opt_stage_dir="$(run_privileged mktemp -d "/opt/.ot-llvm-${LLVM_VERSION}.XXXXXX")"

            echo "Extracting LLVM ${LLVM_VERSION} into '${INSTALL_DIR}'..."
            run_privileged tar --no-same-owner -xJf "${archive_path}" -C "${opt_stage_dir}" --strip-components=1
            rm -f "${archive_path}"
            run_privileged chmod 0755 "${opt_stage_dir}"

            if ! has_installed_llvm "${opt_stage_dir}"; then
                die "Extracted archive is missing expected LLVM binaries in '${opt_stage_dir}/bin'"
            fi

            run_privileged rm -rf "${INSTALL_DIR}"
            run_privileged mv "${opt_stage_dir}" "${INSTALL_DIR}"
            opt_stage_dir=""
        )
    )

    ensure_symlinks
    echo "Installed LLVM ${LLVM_VERSION} at '${INSTALL_DIR}'."
}

main "$@"
