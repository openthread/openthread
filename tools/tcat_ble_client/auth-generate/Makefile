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

# Makefile for creating TCAT example certificates (except CA).

# Select here which named CAs from the 'ca' directory are used for signing.
# NOTE: private CA, so its private key is not present in this repo.
ca := TcatCertCa
otherCa := ca

.PHONY: all check-ca-key check-other-ca-key test clean

all: DeviceCert1 DeviceCert2 CommCert1 CommCert2 CommCert3 CommCert4 test

check-ca-key:
	@if [ ! -f "ca/$(strip $(ca))_key.pem" ]; then \
	    echo "ERROR: CA private key 'ca/$(strip $(ca))_key.pem' not found."; \
	    echo "  The default CA '$(strip $(ca))' is privately maintained by Thread Group"; \
	    echo "  and its private key is intentionally not included in this repository."; \
	    echo "  To generate certificates, update var 'ca' in the Makefile to your own CA name"; \
	    echo "  and place the CA certificate (<name>_cert.pem) and private key (<name>_key.pem)"; \
	    echo "  in the 'ca' directory. See ../GENERATING_CERTIFICATES.md for details."; \
	    exit 1; \
	fi

check-other-ca-key:
	@if [ ! -f "ca/$(strip $(otherCa))_key.pem" ]; then \
	    echo "ERROR: CA private key 'ca/$(strip $(otherCa))_key.pem' not found."; \
	    echo "  Update var 'otherCa' in the Makefile to a CA whose private key is present"; \
	    echo "  in the 'ca' directory, or add the key file (<name>_key.pem) there."; \
	    echo "  See ../GENERATING_CERTIFICATES.md for details."; \
	    exit 1; \
	fi

DeviceCert1 DeviceCert2: check-ca-key ext/DeviceCert1.ext ext/DeviceCert2.ext
	./create-cert-tcat-device.sh $@ $(ca)

CommCert1 CommCert2 CommCert4: check-ca-key ext/CommCert1.ext ext/CommCert2.ext ext/CommCert4.ext
	./create-cert-tcat-commissioner.sh $@ $(ca)

CommCert3: check-other-ca-key ext/CommCert3.ext
	./create-cert-tcat-commissioner.sh $@ $(otherCa)

test:
	@echo "Testing certificate chains..."
	@for name in CommCert1 CommCert2 CommCert4 DeviceCert1 DeviceCert2; do \
	    dir="output/$$name"; \
	    if [ ! -d "$$dir" ]; then \
	        echo "SKIP $$name: output directory not found (run 'make' first)"; \
	        continue; \
	    fi; \
	    openssl verify -CAfile "ca/$(strip $(ca))_cert.pem" "$$dir/commissioner_cert.pem" || exit 1; \
	    if [ -f "$$dir/device_cert.pem" ]; then \
	        openssl verify -CAfile "ca/$(strip $(ca))_cert.pem" "$$dir/device_cert.pem" || exit 1; \
	    fi; \
	done
	@if [ -d "output/CommCert3" ]; then \
	    openssl verify -CAfile "ca/$(strip $(otherCa))_cert.pem" "output/CommCert3/commissioner_cert.pem" || exit 1; \
	else \
	    echo "SKIP CommCert3: output directory not found (run 'make' first)"; \
	fi
	@echo "All certificate chain tests passed."

clean:
	rm -rf ./output
