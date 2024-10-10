/*
 *  Copyright (c) 2024, The OpenThread Authors.
 *  All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions are met:
 *  1. Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *  2. Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *  3. Neither the name of the copyright holder nor the
 *     names of its contributors may be used to endorse or promote products
 *     derived from this software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 *  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 *  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 *  ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 *  LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 *  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 *  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 *  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 *  CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 *  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 *  POSSIBILITY OF SUCH DAMAGE.
 */

/**
 * @file
 * @brief
 *  This file contains the X.509 certificate and private key for the Initial
 *  Device Identifier (IDevID), the hardware-stored identity used for cBRSKI
 *  onboarding. Use with cipher suite ECDHE_ECDSA_WITH_AES_128_CCM8.
 */

#ifndef IDEVID_X509_CERT_KEY_HPP_
#define IDEVID_X509_CERT_KEY_HPP_

#if OPENTHREAD_CONFIG_CCM_ENABLE && defined(MBEDTLS_KEY_EXCHANGE_ECDHE_ECDSA_ENABLED)

// NOTE: by using C++ raw string literals R"( the PEM files can be
// directly copy-pasted into the source code below.
// TODO: more efficient DER storage and access via platform call to secure storage module.
// FIXME: for proper testing, each CCM Joiner should have its unique IDevID identity.

static const char kIdevidCert[] = R"(
-----BEGIN CERTIFICATE-----
MIICMTCCAdegAwIBAgIUL9gkaIiJEqK+SGYNJ1qBI5t6HZEwCgYIKoZIzj0EAwIw
UTEbMBkGA1UEAwwSVGVzdFZlbmRvciBNQVNBIENBMRMwEQYDVQQKDApUZXN0VmVu
ZG9yMRAwDgYDVQQHDAdVdHJlY2h0MQswCQYDVQQGEwJOTDAgFw0yNDA4MjcwOTA4
NTZaGA85OTk5MTIzMTA5MDg1NlowNTEeMBwGA1UEAwwVVGVzdFZlbmRvciBJb1Qg
ZGV2aWNlMRMwEQYDVQQFEwpBODZEMzMwMDAxMFkwEwYHKoZIzj0CAQYIKoZIzj0D
AQcDQgAE/db+2nX/F6iz/xuBjBlHhHUQHfWVSHCXxk41M/iYg8HtjvviBwyWhYyx
dWiEvEfW2QIMVdwYlGSDNotczEizhqOBpjCBozAMBgNVHRMBAf8EAjAAMAsGA1Ud
DwQEAwIE8DAfBgNVHSMEGDAWgBRvB7XOH6Gq/0MFGxp+TUslfXUNGjAbBgNVHREE
FDASoBAGCSsGAQQBgt8qAqADAgEDMCkGCCsGAQUFBwEgBB0WG21hc2EuaW90Y29u
c3VsdGFuY3kubmw6OTQ0MzAdBgNVHQ4EFgQUgBG4X/8NfclCxihjk2230u4qJLQw
CgYIKoZIzj0EAwIDSAAwRQIgC/QQ8mu4XNVDJ7mLSTfvKb+mF7vPaeLBM8INpvGj
s7ACIQCADSabmPlaazWh09xuTLqsMvACc+9bVYG0xTuEDzEk1A==
-----END CERTIFICATE-----
)";

// WARNING: only public test keys can be included here. For
// production use, secure private key storage is needed.
static const char kIdevidPrivateKey[] = R"(
-----BEGIN EC PRIVATE KEY-----
MHcCAQEEIEnOyDlxfQF5IiwOT0a8QJ7iSRSOL0JmudUdfMk3fIC9oAoGCCqGSM49
AwEHoUQDQgAE/db+2nX/F6iz/xuBjBlHhHUQHfWVSHCXxk41M/iYg8HtjvviBwyW
hYyxdWiEvEfW2QIMVdwYlGSDNotczEizhg==
-----END EC PRIVATE KEY-----
)";

static const char kIdevidCaCert[] = R"(
-----BEGIN CERTIFICATE-----
MIIB5DCCAYmgAwIBAgIUdbjHysVPRSwbyvD67wWvlF3nL5UwCgYIKoZIzj0EAwIw
UTEbMBkGA1UEAwwSVGVzdFZlbmRvciBNQVNBIENBMRMwEQYDVQQKDApUZXN0VmVu
ZG9yMRAwDgYDVQQHDAdVdHJlY2h0MQswCQYDVQQGEwJOTDAeFw0yNDA4MjcwOTA4
NTZaFw0zNDA4MjUwOTA4NTZaMFExGzAZBgNVBAMMElRlc3RWZW5kb3IgTUFTQSBD
QTETMBEGA1UECgwKVGVzdFZlbmRvcjEQMA4GA1UEBwwHVXRyZWNodDELMAkGA1UE
BhMCTkwwWTATBgcqhkjOPQIBBggqhkjOPQMBBwNCAAR6Y9P6Dtz/dCAih6eOMXD9
nX+vIy5+/JfhPsc8WsDfONo4tFxhYFrzoaBTXKSlM34T0D9rC0QA19zrvkZ+sIlS
oz8wPTAPBgNVHRMBAf8EBTADAQH/MAsGA1UdDwQEAwIChDAdBgNVHQ4EFgQUbwe1
zh+hqv9DBRsafk1LJX11DRowCgYIKoZIzj0EAwIDSQAwRgIhAO+9NwXl5pNwB6qh
Dh0DOcMi/qAn5iLS3JAlsPcRX++aAiEAxtHX0tV8j5OWUdJUPCBoiw+31C3zSR8N
oObN4e2izJc=
-----END CERTIFICATE-----
)";

#endif // OPENTHREAD_CONFIG_CCM_ENABLE && defined(MBEDTLS_KEY_EXCHANGE_ECDHE_ECDSA_ENABLED)
#endif /* IDEVID_X509_CERT_KEY_HPP_ */
