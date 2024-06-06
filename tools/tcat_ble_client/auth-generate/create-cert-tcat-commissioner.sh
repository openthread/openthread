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

# Script to generate a TCAT Commissioner X509v3 certificate.

if [ $# -ne 2 ]; then
    echo "Usage: ./create-cert-tcat-commissioner.sh <NameOfCommissioner> <NameOfCA>"
    exit 1
fi
set -eu

# number of days certificate is valid
((VALIDITY = "14"))
echo "create-cert-tcat-commissioner.sh - Using validity param -days ${VALIDITY}"

NAME=${1}
CANAME=${2}
((ID = ${NAME:0-1}))
CACERTFILE="ca/${CANAME}_cert.pem"

echo "  TCAT commissioner name   : ${NAME}"
echo "  TCAT commissioner CA name: ${CANAME}"
echo "  Numeric serial ID        : ${ID}"

# create csr for TCAT Commissioner
openssl req -new -key "keys/${NAME}_key.pem" -out "${NAME}.csr" -subj \
    "/CN=TCAT Example ${NAME}/serialNumber=3523-1543-000${ID}"

# sign csr by CA
mkdir -p "output/${NAME}"
openssl x509 -set_serial "92429${ID}" -CAform PEM -CA "${CACERTFILE}" \
    -CAkey "ca/${CANAME}_key.pem" -extfile "ext/${NAME}.ext" -extensions \
    "${NAME}" -req -in "${NAME}.csr" -out "output/${NAME}/commissioner_cert.pem" \
    -days "${VALIDITY}" -sha256

# delete temp files
rm -f "${NAME}.csr"

# copy supporting files, for immediate use by TCAT Commissioner as 'cert_path'
cp "${CACERTFILE}" "output/${NAME}/ca_cert.pem"
cp "keys/${NAME}_key.pem" "output/${NAME}/commissioner_key.pem"
