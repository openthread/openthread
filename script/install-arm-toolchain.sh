#!/bin/bash
#
#  Copyright (c) 2026, The OpenThread Authors.
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
#  Description:
#    Downloads, verifies, and extracts GNU Arm toolchains with mirror fallback
#    and stall-protected retries for OpenThread repositories.
#

set -euo pipefail

usage()
{
    cat <<'EOF'
Usage:
  install-arm-toolchain.sh [options] <version> [<dest-dir>]
  install-arm-toolchain.sh [options] --url <url> <dest-dir>

Options:
  --arch <arch>         Host architecture (x86_64 or aarch64; default: uname -m)
  --url <url>           Explicit primary toolchain archive URL
  --mirror-url <url>    Explicit fallback mirror archive URL
  --sha256 <hex>        Expected SHA-256 digest of the archive
  --force               Re-download even if <dest-dir>/bin/*-gcc already exists
  -h, --help            Display this help text
EOF
}

die()
{
    echo "ERROR: $*" >&2
    exit 1
}

normalize_arch()
{
    case "$1" in
        x86_64 | amd64)
            NORMALIZED_ARCH="x86_64"
            ;;
        aarch64 | arm64)
            NORMALIZED_ARCH="aarch64"
            ;;
        *)
            die "Unsupported host architecture: $1"
            ;;
    esac
}

has_installed_compiler()
{
    local dir="$1"
    local candidate

    [[ -d "${dir}/bin" ]] || return 1

    for candidate in "${dir}/bin/"*-gcc; do
        if [[ -x ${candidate} ]]; then
            return 0
        fi
    done

    return 1
}

resolve_version()
{
    local version="$1"
    local arch="$2"
    local host_os

    host_os="$(uname -s)"
    [[ ${host_os} == "Linux" ]] || die "Built-in Arm toolchain versions only support Linux hosts (detected ${host_os}); pass --url <url> for other OSes"

    PRIMARY_URL=""
    MIRROR_URL=""
    ARCHIVE_SHA256=""

    case "${version}" in
        4 | 4.9 | 4_9-2015q3)
            [[ ${arch} == "x86_64" ]] || die "Arm GCC ${version} is only available for x86_64"
            PRIMARY_URL="https://launchpad.net/gcc-arm-embedded/4.9/4.9-2015-q3-update/+download/gcc-arm-none-eabi-4_9-2015q3-20150921-linux.tar.bz2"
            ;;
        5 | 5.4 | 5_4-2016q3)
            [[ ${arch} == "x86_64" ]] || die "Arm GCC ${version} is only available for x86_64"
            PRIMARY_URL="https://developer.arm.com/-/media/Files/downloads/gnu-rm/5_4-2016q3/gcc-arm-none-eabi-5_4-2016q3-20160926-linux.tar.bz2"
            MIRROR_URL="https://launchpad.net/gcc-arm-embedded/5.0/5-2016-q3-update/+download/gcc-arm-none-eabi-5_4-2016q3-20160926-linux.tar.bz2"
            ;;
        6 | 6-2017q2)
            [[ ${arch} == "x86_64" ]] || die "Arm GCC ${version} is only available for x86_64"
            PRIMARY_URL="https://developer.arm.com/-/media/Files/downloads/gnu-rm/6-2017q2/gcc-arm-none-eabi-6-2017-q2-update-linux.tar.bz2"
            MIRROR_URL="https://armkeil.blob.core.windows.net/developer/Files/downloads/gnu-rm/6-2017q2/gcc-arm-none-eabi-6-2017-q2-update-linux.tar.bz2"
            ;;
        7 | 7-2018q2)
            [[ ${arch} == "x86_64" ]] || die "Arm GCC ${version} is only available for x86_64"
            PRIMARY_URL="https://developer.arm.com/-/media/Files/downloads/gnu-rm/7-2018q2/gcc-arm-none-eabi-7-2018-q2-update-linux.tar.bz2"
            MIRROR_URL="https://armkeil.blob.core.windows.net/developer/Files/downloads/gnu-rm/7-2018q2/gcc-arm-none-eabi-7-2018-q2-update-linux.tar.bz2"
            ARCHIVE_SHA256="bb17109f0ee697254a5d4ae6e5e01440e3ea8f0277f2e8169bf95d07c7d5fe69"
            ;;
        8-gnueabihf | 8.3-2019.03-gnueabihf)
            [[ ${arch} == "x86_64" ]] || die "Arm GNU-A ${version} is only available for x86_64"
            PRIMARY_URL="https://developer.arm.com/-/media/Files/downloads/gnu-a/8.3-2019.03/binrel/gcc-arm-8.3-2019.03-x86_64-arm-linux-gnueabihf.tar.xz"
            MIRROR_URL="https://armkeil.blob.core.windows.net/developer/Files/downloads/gnu-a/8.3-2019.03/binrel/gcc-arm-8.3-2019.03-x86_64-arm-linux-gnueabihf.tar.xz"
            ;;
        9 | 9-2019q4)
            PRIMARY_URL="https://developer.arm.com/-/media/Files/downloads/gnu-rm/9-2019q4/RC2.1/gcc-arm-none-eabi-9-2019-q4-major-${arch}-linux.tar.bz2"
            MIRROR_URL="https://armkeil.blob.core.windows.net/developer/Files/downloads/gnu-rm/9-2019q4/RC2.1/gcc-arm-none-eabi-9-2019-q4-major-${arch}-linux.tar.bz2"
            ;;
        9-2020q2)
            PRIMARY_URL="https://developer.arm.com/-/media/Files/downloads/gnu-rm/9-2020q2/gcc-arm-none-eabi-9-2020-q2-update-${arch}-linux.tar.bz2"
            MIRROR_URL="https://armkeil.blob.core.windows.net/developer/Files/downloads/gnu-rm/9-2020q2/gcc-arm-none-eabi-9-2020-q2-update-${arch}-linux.tar.bz2"
            case "${arch}" in
                x86_64)
                    ARCHIVE_SHA256="5adc2ee03904571c2de79d5cfc0f7fe2a5c5f54f44da5b645c17ee57b217f11f"
                    ;;
                aarch64)
                    ARCHIVE_SHA256="1f4165c25e2cff80e29870f409862487ba470afd436e245ba3c743108e17b8ac"
                    ;;
            esac
            ;;
        10-2020q4)
            PRIMARY_URL="https://developer.arm.com/-/media/Files/downloads/gnu-rm/10-2020q4/gcc-arm-none-eabi-10-2020-q4-major-${arch}-linux.tar.bz2"
            MIRROR_URL="https://armkeil.blob.core.windows.net/developer/Files/downloads/gnu-rm/10-2020q4/gcc-arm-none-eabi-10-2020-q4-major-${arch}-linux.tar.bz2"
            ;;
        10 | 10.3 | 10.3-2021.10)
            PRIMARY_URL="https://developer.arm.com/-/media/Files/downloads/gnu-rm/10.3-2021.10/gcc-arm-none-eabi-10.3-2021.10-${arch}-linux.tar.bz2"
            MIRROR_URL="https://armkeil.blob.core.windows.net/developer/Files/downloads/gnu-rm/10.3-2021.10/gcc-arm-none-eabi-10.3-2021.10-${arch}-linux.tar.bz2"
            ;;
        11 | 11.3 | 11.3.rel1)
            PRIMARY_URL="https://developer.arm.com/-/media/Files/downloads/gnu/11.3.rel1/binrel/arm-gnu-toolchain-11.3.rel1-${arch}-arm-none-eabi.tar.xz"
            MIRROR_URL="https://armkeil.blob.core.windows.net/developer/Files/downloads/gnu/11.3.rel1/binrel/arm-gnu-toolchain-11.3.rel1-${arch}-arm-none-eabi.tar.xz"
            ;;
        12 | 12.2 | 12.2.rel1 | 12.2.Rel1)
            PRIMARY_URL="https://developer.arm.com/-/media/Files/downloads/gnu/12.2.rel1/binrel/arm-gnu-toolchain-12.2.rel1-${arch}-arm-none-eabi.tar.xz"
            MIRROR_URL="https://armkeil.blob.core.windows.net/developer/Files/downloads/gnu/12.2.rel1/binrel/arm-gnu-toolchain-12.2.rel1-${arch}-arm-none-eabi.tar.xz"
            ;;
        12.3 | 12.3.rel1 | 12.3.Rel1)
            PRIMARY_URL="https://developer.arm.com/-/media/Files/downloads/gnu/12.3.rel1/binrel/arm-gnu-toolchain-12.3.rel1-${arch}-arm-none-eabi.tar.xz"
            MIRROR_URL="https://armkeil.blob.core.windows.net/developer/Files/downloads/gnu/12.3.rel1/binrel/arm-gnu-toolchain-12.3.rel1-${arch}-arm-none-eabi.tar.xz"
            ;;
        13 | 13.3 | 13.3.rel1)
            PRIMARY_URL="https://developer.arm.com/-/media/Files/downloads/gnu/13.3.rel1/binrel/arm-gnu-toolchain-13.3.rel1-${arch}-arm-none-eabi.tar.xz"
            MIRROR_URL="https://armkeil.blob.core.windows.net/developer/Files/downloads/gnu/13.3.rel1/binrel/arm-gnu-toolchain-13.3.rel1-${arch}-arm-none-eabi.tar.xz"
            ;;
        14 | 14.2 | 14.2.rel1)
            PRIMARY_URL="https://developer.arm.com/-/media/Files/downloads/gnu/14.2.rel1/binrel/arm-gnu-toolchain-14.2.rel1-${arch}-arm-none-eabi.tar.xz"
            MIRROR_URL="https://armkeil.blob.core.windows.net/developer/Files/downloads/gnu/14.2.rel1/binrel/arm-gnu-toolchain-14.2.rel1-${arch}-arm-none-eabi.tar.xz"
            ;;
        15 | 15.2 | 15.2.rel1)
            PRIMARY_URL="https://developer.arm.com/-/media/Files/downloads/gnu/15.2.rel1/binrel/arm-gnu-toolchain-15.2.rel1-${arch}-arm-none-eabi.tar.xz"
            MIRROR_URL="https://armkeil.blob.core.windows.net/developer/files/downloads/gnu/15.2.rel1/binrel/arm-gnu-toolchain-15.2.rel1-${arch}-arm-none-eabi.tar.xz"
            ;;
        *)
            die "Unsupported Arm toolchain version '${version}'. Pass --url <url> for custom archives."
            ;;
    esac
}

download_and_extract()
{
    local url="$1"
    local sha256="$2"
    local stage_dir="$3"
    local archive_path="${stage_dir}/toolchain.archive"
    local extract_dir="${stage_dir}/extracted"
    local sha256_cmd=()

    rm -rf "${archive_path}" "${extract_dir}" || return 1
    mkdir -p "${extract_dir}" || return 1

    if [[ -n ${sha256} ]]; then
        if command -v sha256sum >/dev/null 2>&1; then
            sha256_cmd=(sha256sum -c -)
        elif command -v shasum >/dev/null 2>&1; then
            sha256_cmd=(shasum -a 256 -c -)
        else
            die "Neither sha256sum nor shasum is available for verification"
        fi
    fi

    if command -v curl >/dev/null 2>&1; then
        curl --proto '=https' --tlsv1.2 -fsSL \
            --retry 2 --retry-delay 2 --retry-max-time 80 \
            --connect-timeout 15 --max-time 75 \
            --speed-limit 1048576 --speed-time 15 \
            -o "${archive_path}" "${url}" || return 1
    elif command -v wget >/dev/null 2>&1; then
        [[ ${url} == https://* ]] || return 1
        wget -nv --https-only --secure-protocol=TLSv1_2 \
            --tries=3 --timeout=15 --waitretry=2 \
            -O "${archive_path}" "${url}" || return 1
    else
        die "Neither curl nor wget is available"
    fi

    if [[ -n ${sha256} ]]; then
        echo "${sha256}  ${archive_path}" | "${sha256_cmd[@]}" || return 1
    fi

    tar --no-same-owner -xf "${archive_path}" -C "${extract_dir}" --strip-components=1 || return 1
    chmod 0755 "${extract_dir}" || return 1
    has_installed_compiler "${extract_dir}" || return 1
}

main()
{
    local arch
    local custom_url=""
    local custom_mirror_url=""
    local custom_sha256=""
    local force=0
    local positional=()
    local version=""
    local dest_dir=""

    arch="$(uname -m)"

    while [[ $# -gt 0 ]]; do
        case "$1" in
            --arch)
                [[ $# -ge 2 && -n $2 ]] || die "--arch requires a non-empty argument"
                arch="$2"
                shift 2
                ;;
            --url)
                [[ $# -ge 2 && -n $2 ]] || die "--url requires a non-empty argument"
                custom_url="$2"
                shift 2
                ;;
            --mirror-url)
                [[ $# -ge 2 && -n $2 ]] || die "--mirror-url requires a non-empty argument"
                custom_mirror_url="$2"
                shift 2
                ;;
            --sha256)
                [[ $# -ge 2 && -n $2 ]] || die "--sha256 requires a non-empty argument"
                [[ $2 =~ ^[0-9a-fA-F]{64}$ ]] || die "Invalid SHA-256 digest: '$2'"
                custom_sha256="$2"
                shift 2
                ;;
            --force)
                force=1
                shift
                ;;
            -h | --help)
                usage
                exit 0
                ;;
            -*)
                die "Unknown option: $1"
                ;;
            *)
                positional+=("$1")
                shift
                ;;
        esac
    done

    normalize_arch "${arch}"
    arch="${NORMALIZED_ARCH}"

    local urls=()
    local sha256=""

    if [[ -n ${custom_url} ]]; then
        [[ ${#positional[@]} -eq 1 ]] || die "Expected <dest-dir> when using --url"
        dest_dir="${positional[0]}"
        urls+=("${custom_url}")
        if [[ -n ${custom_mirror_url} ]]; then
            urls+=("${custom_mirror_url}")
        elif [[ ${custom_url} == https://developer.arm.com/-/media/[Ff]iles/* ]]; then
            local rel_path="${custom_url#https://developer.arm.com/-/media/[Ff]iles/}"
            urls+=(
                "https://armkeil.blob.core.windows.net/developer/Files/${rel_path}"
                "https://armkeil.blob.core.windows.net/developer/files/${rel_path}"
            )
        fi
        sha256="${custom_sha256}"
    else
        [[ ${#positional[@]} -ge 1 && ${#positional[@]} -le 2 ]] || {
            usage >&2
            exit 1
        }
        version="${positional[0]}"
        if [[ ${#positional[@]} -eq 2 ]]; then
            dest_dir="${positional[1]}"
        else
            dest_dir="/tmp/arm-gcc-${version}"
        fi
        resolve_version "${version}" "${arch}"
        local mirror_url="${custom_mirror_url:-${MIRROR_URL}}"
        urls+=("${PRIMARY_URL}")
        if [[ -n ${mirror_url} ]]; then
            urls+=("${mirror_url}")
        fi
        sha256="${custom_sha256:-${ARCHIVE_SHA256}}"
    fi

    while [[ ${dest_dir} == */ && ${dest_dir} != "/" ]]; do
        dest_dir="${dest_dir%/}"
    done
    case "${dest_dir}" in
        "" | "/" | "." | ".." | */. | */.. | "/bin" | "/boot" | "/dev" | "/etc" | "/home" | "/lib" | "/lib64" | "/opt" | "/proc" | "/root" | "/run" | "/sbin" | "/srv" | "/sys" | "/tmp" | "/usr" | "/usr/bin" | "/usr/sbin" | "/usr/local" | "/usr/local/bin" | "/usr/local/sbin" | "/var" | "/var/tmp" | "/Applications" | "/Library" | "/System" | "/Users" | "/Volumes" | "/private" | "/private/tmp" | "/private/var" | "/private/var/tmp")
            die "Invalid destination directory: '${dest_dir}'"
            ;;
    esac

    if [[ ${force} -eq 0 ]] && has_installed_compiler "${dest_dir}"; then
        echo "Arm toolchain already installed at '${dest_dir}'; skipping download."
        return 0
    fi

    local parent_dir
    parent_dir="$(dirname "${dest_dir}")"

    (
        umask 022
        mkdir -p "${parent_dir}"
        stage_dir="$(mktemp -d "${parent_dir}/.ot-arm-toolchain.XXXXXX")"
        trap 'rm -rf "${stage_dir}"' EXIT

        installed=0
        for url in "${urls[@]}"; do
            echo "Downloading Arm toolchain from '${url}' into '${dest_dir}'..."
            if download_and_extract "${url}" "${sha256}" "${stage_dir}"; then
                installed=1
                break
            fi
            echo "WARNING: failed to download or extract Arm toolchain from ${url}" >&2
        done

        if [[ ${installed} -ne 1 ]]; then
            die "Failed to download and extract Arm toolchain into '${dest_dir}'"
        fi

        rm -rf "${dest_dir}"
        mv "${stage_dir}/extracted" "${dest_dir}"
    )

    echo "Installed Arm toolchain at '${dest_dir}'."
}

main "$@"
