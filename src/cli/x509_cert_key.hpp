/*
 *  Copyright (c) 2018, The OpenThread Authors.
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
 *  This file contains the X.509 certificate and private key for Application
 *  CoAP Secure use with cipher suite ECDHE_ECDSA_WITH_AES_128_CCM8.
 */

#ifndef SRC_CLI_X509_CERT_KEY_HPP_
#define SRC_CLI_X509_CERT_KEY_HPP_

#ifdef __cplusplus
extern "C" {
#endif

#if OPENTHREAD_ENABLE_APPLICATION_COAP_SECURE
#if defined(MBEDTLS_KEY_EXCHANGE_ECDHE_ECDSA_ENABLED)

/**
 * Generate your own private key and certificate using openssl.
 *
 * 1. Generate a EC (Elliptic Curve) Private Key.
 *      'openssl ecparam -genkey -out myECKey.pem -name prime256v1 -noout'
 * 2. Generate a .X509 Certificate (Contains Public Key).
 *      'openssl req -x509 -new -key myECKey.pem -out myX509Cert.pem'
 */

/**
 * place your X.509 certificate (PEM format) for ssl session
 * with ECDHE_ECDSA_WITH_AES_128_CCM_8 here.
 */
#define OT_CLI_COAPS_X509_CERT                                         \
"-----BEGIN CERTIFICATE-----\r\n"                                      \
"MIICazCCAhKgAwIBAgIJAKIuIwCFs70OMAoGCCqGSM49BAMCMIGPMQswCQYDVQQG\r\n" \
"EwJJTzEQMA4GA1UECAwHTXlTdGF0ZTEPMA0GA1UEBwwGTXlDaXR5MRMwEQYDVQQK\r\n" \
"DApPcGVuVGhyZWFkMQ0wCwYDVQQLDARUZXN0MRMwEQYDVQQDDApDb0FQUyBUZXN0\r\n" \
"MSQwIgYJKoZIhvcNAQkBFhVteS5tYWlsQG9wZW50aHJlYWQuaW8wIBcNMTgwNjI2\r\n" \
"MDkwNTA1WhgPMjI5MjA0MTAwOTA1MDVaMIGPMQswCQYDVQQGEwJJTzEQMA4GA1UE\r\n" \
"CAwHTXlTdGF0ZTEPMA0GA1UEBwwGTXlDaXR5MRMwEQYDVQQKDApPcGVuVGhyZWFk\r\n" \
"MQ0wCwYDVQQLDARUZXN0MRMwEQYDVQQDDApDb0FQUyBUZXN0MSQwIgYJKoZIhvcN\r\n" \
"AQkBFhVteS5tYWlsQG9wZW50aHJlYWQuaW8wWTATBgcqhkjOPQIBBggqhkjOPQMB\r\n" \
"BwNCAATTaiF8x7S3hmC0Um21euSUmdgwYEtqa9u8a3CcMEcQAvStGtO6u1u8P3Ef\r\n" \
"RxbvLsQSwIgWu1v2RSP6NEBcK1Pfo1MwUTAdBgNVHQ4EFgQUh05hUYlb1kjIND9x\r\n" \
"dH9WxwVtaaQwHwYDVR0jBBgwFoAUh05hUYlb1kjIND9xdH9WxwVtaaQwDwYDVR0T\r\n" \
"AQH/BAUwAwEB/zAKBggqhkjOPQQDAgNHADBEAiA8E5pQVsqOlVnYkBftkclXwCTU\r\n" \
"0X/4eg4fRe28GcuGkQIgVXS6lIlsH26rcjzMBfOsanSzqx9+d695TGJ/FBuKrbg=\r\n" \
"-----END CERTIFICATE-----\r\n"                                        \

/**
 * place your private key (PEM format) for ssl session
 * with ECDHE_ECDSA_WITH_AES_128_CCM_8 here.
 */
#define OT_CLI_COAPS_PRIV_KEY                                           \
"-----BEGIN EC PRIVATE KEY-----\r\n"                                    \
"MHcCAQEEIIG+2tZQBootI0H065kJg/0MK9KOEX/jnZqYPRtbsjJ0oAoGCCqGSM49\r\n"  \
"AwEHoUQDQgAE02ohfMe0t4ZgtFJttXrklJnYMGBLamvbvGtwnDBHEAL0rRrTurtb\r\n"  \
"vD9xH0cW7y7EEsCIFrtb9kUj+jRAXCtT3w==\r\n"                              \
"-----END EC PRIVATE KEY-----\r\n"

#endif // MBEDTLS_KEY_EXCHANGE_ECDHE_ECDSA_ENABLED
#endif // OPENTHREAD_ENABLE_APPLICATION_COAP_SECURE

/**
 * @}
 *
 */

#ifdef __cplusplus
}  // extern "C"
#endif

#endif /* SRC_CLI_X509_CERT_KEY_HPP_ */
