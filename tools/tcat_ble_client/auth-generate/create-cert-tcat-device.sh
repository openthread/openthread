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

# Script to generate a TCAT Device X509v3 certificate.

if [ $# -ne 2 ]; then
    echo "Usage: ./create-cert-tcat-device.sh <NameOfDevice> <NameOfCA>"
    exit 1
fi
set -eu

# days certificate is valid
SECONDS1=$(date +%s)                               # time now
SECONDS2=$(date --date="2999-12-31 23:59:59Z" +%s) # target end time
((VALIDITY = "(${SECONDS2}-${SECONDS1})/(24*3600)"))
echo "create-cert-tcat-device.sh - Using validity param -days ${VALIDITY}"

NAME="${1}"
CANAME="${2}"
CACERTFILE="ca/${CANAME}_cert.pem"
((ID = ${NAME:0-1}))
((SERIAL = 13800 + ID))

echo "  TCAT device name   : ${NAME}"
echo "  TCAT device CA name: ${CANAME}"
echo "  Numeric serial ID  : ${ID}"

# create csr for device.
# conform to 802.1AR guidelines, using only CN + serialNumber when
# manufacturer is already present as CA. CN is not even mandatory, but just good practice.
openssl req -new -key "keys/${NAME}_key.pem" -out "${NAME}.csr" -subj \
    "/CN=TCAT Example ${NAME}/serialNumber=4723-9833-000${ID}"

# sign csr by CA
mkdir -p "output/${NAME}"
openssl x509 -set_serial "${SERIAL}" -CAform PEM -CA "${CACERTFILE}" \
    -CAkey "ca/${CANAME}_key.pem" -extfile "ext/${NAME}.ext" -extensions \
    "${NAME}" -req -in "${NAME}.csr" -out "output/${NAME}/device_cert.pem" \
    -days "${VALIDITY}" -sha256

# delete temp files
rm -f "${NAME}.csr"

# copy supporting files, for immediate use by TCAT Commissioner as 'cert_path'.
# Normally a Commissioner must not use Device certs, but for testing purposes this
# option is provided here.
cp "output/${NAME}/device_cert.pem" "output/${NAME}/commissioner_cert.pem"
cp "${CACERTFILE}" "output/${NAME}/ca_cert.pem"
cp "keys/${NAME}_key.pem" "output/${NAME}/commissioner_key.pem"
cp "keys/${NAME}_key.pem" "output/${NAME}/device_key.pem"
