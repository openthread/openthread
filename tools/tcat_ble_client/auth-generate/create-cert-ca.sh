#!/bin/bash
#
#  Copyright (c) 2024, The OpenThread Authors.
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

# Create the certificate of an example CA for TCAT. This single CA creates both the
# TCAT Device certificates, as well as the TCAT Commissioner certificates that
# work for those TCAT Devices.

if [ $# -ne 1 ]; then
    echo "Usage: ./create-cert-ca.sh <NameOfCA>"
    exit 1
fi
set -eu

# days certificate is valid
((VALIDITY = 20 * 365))

NAME=${1}

# create csr
openssl req -new -key "ca/${NAME}_key.pem" -out "${NAME}.csr" \
    -subj "/CN=TCAT Example CA '${NAME}'/O=Example Inc/L=Example City/ST=CA/C=US"

# self-sign csr
mkdir -p output
openssl x509 -set_serial 0x01 -extfile "ext/${NAME}.ext" \
    -extensions "${NAME}" -req -in "${NAME}.csr" \
    -signkey "ca/${NAME}_key.pem" -out "ca/${NAME}_cert.pem" \
    -days "${VALIDITY}" -sha256

# delete temp files
rm -f "${NAME}.csr"

# show result
openssl x509 -text -in "ca/${NAME}_cert.pem"
