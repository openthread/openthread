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
#    Installs code formatting and linting tools used by script/make-pretty
#    across OpenThread repositories.
#

set -euo pipefail

script_dir="$(cd "$(dirname "$0")" && pwd)"
readonly script_dir

target_dir="$(git rev-parse --show-toplevel 2>/dev/null || pwd)"
readonly target_dir

readonly DEFAULT_YAPF_VERSION="0.43.0"
readonly DEFAULT_PRETTIER_VERSION="2.0.4"
readonly NPM_INSTALL_ATTEMPTS=3
readonly NPM_INSTALL_RETRY_DELAY=10

usage()
{
    cat <<EOF
Usage:
  install-pretty-tools.sh [options]

Options:
  --llvm                      Install LLVM formatting/linting tools (default)
  --no-llvm                   Skip LLVM installation
  --shell                     Install shellcheck and shfmt (default)
  --no-shell                  Skip shellcheck and shfmt installation
  --python                    Install yapf via pip (default)
  --no-python                 Skip yapf installation
  --yapf-version <ver>        yapf version to install (default: ${DEFAULT_YAPF_VERSION})
  --markdown                  Install prettier via npm (default)
  --no-markdown               Skip prettier installation
  --prettier-version <ver>    prettier version to install (default: ${DEFAULT_PRETTIER_VERSION})
  --iwyu                      Install include-what-you-use (iwyu)
  --no-iwyu                   Skip iwyu installation (default)
  -h, --help                  Display this help text
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

is_dpkg_installed()
{
    local pkg="$1"
    local dpkg_output=""

    if ! command -v dpkg >/dev/null 2>&1; then
        return 1
    fi

    dpkg_output="$(dpkg -s "${pkg}" 2>/dev/null)" || return 1
    [[ $'\n'${dpkg_output}$'\n' == *$'\nStatus: install ok installed\n'* ]]
}

install_apt_packages()
{
    if [[ $# -eq 0 ]]; then
        return 0
    fi

    command -v apt-get >/dev/null 2>&1 || die "apt-get is required to install missing packages: $*"
    require_privilege

    echo "Installing APT packages: $*..."
    run_privileged apt-get update
    run_privileged env DEBIAN_FRONTEND=noninteractive apt-get --no-install-recommends install -y "$@"
}

install_llvm_tools()
{
    local llvm_script="${script_dir}/install-llvm.sh"

    [[ -f ${llvm_script} ]] || die "Missing helper script: ${llvm_script}"
    bash "${llvm_script}"
}

install_python_tools()
{
    local yapf_version="$1"
    local installed_version=""
    local pip_help=""
    local -a pip_args=(
        install
        --retries 5
        --timeout 30
    )

    if [[ -z ${VIRTUAL_ENV:-} && -x ${target_dir}/.venv/bin/python3 ]]; then
        export VIRTUAL_ENV="${target_dir}/.venv"
        export PATH="${VIRTUAL_ENV}/bin:${PATH}"
    fi

    command -v python3 >/dev/null 2>&1 || die "python3 is required to install yapf"

    installed_version="$(python3 -m yapf --version 2>/dev/null || true)"
    if [[ ${installed_version} == "yapf ${yapf_version}" ]]; then
        echo "yapf ${yapf_version} already installed; skipping."
        return 0
    fi

    python3 -m pip --version >/dev/null 2>&1 || die "python3 -m pip is required to install yapf"

    if [[ -z ${VIRTUAL_ENV:-} ]]; then
        pip_help="$(python3 -m pip install --help 2>/dev/null || true)"
        if [[ ${pip_help} == *--break-system-packages* ]]; then
            pip_args+=(--break-system-packages)
        fi
    fi

    echo "Installing yapf==${yapf_version}..."
    python3 -m pip "${pip_args[@]}" "yapf==${yapf_version}"
}

has_installed_prettier()
{
    local prettier_version="$1"
    local installed_version=""

    if [[ -x ${target_dir}/node_modules/.bin/prettier ]]; then
        installed_version="$("${target_dir}/node_modules/.bin/prettier" --version 2>/dev/null || true)"
        if [[ ${installed_version} == "${prettier_version}" ]]; then
            return 0
        fi
    fi

    if command -v npx >/dev/null 2>&1; then
        installed_version="$(
            if cd "${target_dir}"; then
                npx --offline --no-install -- "prettier@${prettier_version}" --version 2>/dev/null || true
            fi
        )"
        if [[ ${installed_version} == "${prettier_version}" ]]; then
            return 0
        fi
    fi

    return 1
}

install_markdown_tools()
{
    local prettier_version="$1"
    local attempt=1
    local exit_code=0

    command -v npx >/dev/null 2>&1 || die "npx is required to run prettier@${prettier_version}"

    if has_installed_prettier "${prettier_version}"; then
        echo "prettier@${prettier_version} already installed; skipping."
        return 0
    fi

    command -v npm >/dev/null 2>&1 || die "npm is required to install prettier@${prettier_version}"

    while true; do
        echo "Installing prettier@${prettier_version} (attempt ${attempt}/${NPM_INSTALL_ATTEMPTS})..."
        exit_code=0
        (
            cd "${target_dir}" && npm --prefix "${target_dir}" install --no-save --no-package-lock --no-audit --no-fund \
                --fetch-retries=2 --fetch-retry-mintimeout=5000 --fetch-retry-maxtimeout=15000 --fetch-timeout=30000 \
                "prettier@${prettier_version}"
        ) || exit_code=$?
        if [[ ${exit_code} -eq 0 ]]; then
            return 0
        fi

        if [[ ${attempt} -ge ${NPM_INSTALL_ATTEMPTS} ]]; then
            die "Failed to install prettier@${prettier_version} after ${attempt} attempts (exit code ${exit_code})"
        fi

        echo "npm install failed (exit code ${exit_code}); retrying in ${NPM_INSTALL_RETRY_DELAY}s..." >&2
        attempt=$((attempt + 1))
        sleep "${NPM_INSTALL_RETRY_DELAY}"
    done
}

main()
{
    local install_llvm=1
    local install_shell=1
    local install_python=1
    local install_markdown=1
    local install_iwyu=0
    local yapf_version="${DEFAULT_YAPF_VERSION}"
    local prettier_version="${DEFAULT_PRETTIER_VERSION}"
    local -a apt_packages=()

    while [[ $# -gt 0 ]]; do
        case "$1" in
            --llvm)
                install_llvm=1
                shift
                ;;
            --no-llvm)
                install_llvm=0
                shift
                ;;
            --shell)
                install_shell=1
                shift
                ;;
            --no-shell)
                install_shell=0
                shift
                ;;
            --python)
                install_python=1
                shift
                ;;
            --no-python)
                install_python=0
                shift
                ;;
            --yapf-version)
                [[ $# -ge 2 && -n $2 && $2 != -* ]] || die "--yapf-version requires a non-empty version argument"
                yapf_version="$2"
                shift 2
                ;;
            --yapf-version=*)
                yapf_version="${1#*=}"
                [[ -n ${yapf_version} && ${yapf_version} != -* ]] || die "--yapf-version requires a non-empty version argument"
                shift
                ;;
            --markdown)
                install_markdown=1
                shift
                ;;
            --no-markdown)
                install_markdown=0
                shift
                ;;
            --prettier-version)
                [[ $# -ge 2 && -n $2 && $2 != -* ]] || die "--prettier-version requires a non-empty version argument"
                prettier_version="$2"
                shift 2
                ;;
            --prettier-version=*)
                prettier_version="${1#*=}"
                [[ -n ${prettier_version} && ${prettier_version} != -* ]] || die "--prettier-version requires a non-empty version argument"
                shift
                ;;
            --iwyu)
                install_iwyu=1
                shift
                ;;
            --no-iwyu)
                install_iwyu=0
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

    if [[ ${install_llvm} -eq 1 ]]; then
        if command -v dpkg >/dev/null 2>&1 && ! is_dpkg_installed ca-certificates; then
            apt_packages+=(ca-certificates)
        fi
        if ! command -v curl >/dev/null 2>&1; then
            apt_packages+=(curl)
        fi
        if ! command -v xz >/dev/null 2>&1; then
            apt_packages+=(xz-utils)
        fi
    fi

    if [[ ${install_shell} -eq 1 ]]; then
        if ! command -v shellcheck >/dev/null 2>&1; then
            apt_packages+=(shellcheck)
        fi
        if ! command -v shfmt >/dev/null 2>&1; then
            apt_packages+=(shfmt)
        fi
    fi

    if [[ ${install_iwyu} -eq 1 ]]; then
        if ! command -v iwyu >/dev/null 2>&1 || ! command -v fix_include >/dev/null 2>&1; then
            apt_packages+=(iwyu)
        fi
    fi

    if [[ ${#apt_packages[@]} -gt 0 ]]; then
        install_apt_packages "${apt_packages[@]}"
    fi

    if [[ ${install_llvm} -eq 1 ]]; then
        install_llvm_tools
    fi

    if [[ ${install_python} -eq 1 ]]; then
        install_python_tools "${yapf_version}"
    fi

    if [[ ${install_markdown} -eq 1 ]]; then
        install_markdown_tools "${prettier_version}"
    fi
}

main "$@"
