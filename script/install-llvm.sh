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

set -e

LLVM_VERSION="19.1.7"
ARCH=$(uname -m)

# This script is currently only used for linux with x86_64 architecture.
if [ "${ARCH}" != "x86_64" ]; then
    echo "Unsupported architecture: ${ARCH}"
    exit 1
fi

LLVM_PACKAGE="LLVM-${LLVM_VERSION}-Linux-X64"
LLVM_URL="https://github.com/llvm/llvm-project/releases/download/llvmorg-${LLVM_VERSION}/${LLVM_PACKAGE}.tar.xz"
INSTALL_DIR="/opt/llvm-${LLVM_VERSION}"
TEMP_DIR=$(mktemp -d)

cleanup()
{
    rm -rf "${TEMP_DIR}"
}

trap cleanup EXIT

cd "${TEMP_DIR}"

echo "Downloading LLVM from ${LLVM_URL}..."
wget -O llvm.tar.xz "${LLVM_URL}"

echo "Uncompressing to ${TEMP_DIR}..."
tar xf llvm.tar.xz

echo "Installing to ${INSTALL_DIR}..."
sudo mkdir -p /opt
sudo mv "${LLVM_PACKAGE}" "${INSTALL_DIR}"

echo "Creating symlinks in /usr/local/bin..."
sudo ln -sf "${INSTALL_DIR}/bin/clang-format" "/usr/local/bin/clang-format-19"
sudo ln -sf "${INSTALL_DIR}/bin/clang-tidy" "/usr/local/bin/clang-tidy-19"
sudo ln -sf "${INSTALL_DIR}/bin/clang-apply-replacements" "/usr/local/bin/clang-apply-replacements-19"

echo "Done."
