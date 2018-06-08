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

/**
 * place your X.509 certificate (PEM format) for ssl session
 * with ECDHE_ECDSA_WITH_AES_128_CCM_8 here.
 */
#define OT_CLI_COAPS_X509_CERT                                          \
"-----BEGIN CERTIFICATE-----\r\n"                                       \
"Our Cert\r\n"                                                          \
"-----END CERTIFICATE-----\r\n"

/**
 * place your X.509 private key (PEM format) for ssl session
 * with ECDHE_ECDSA_WITH_AES_128_CCM_8 here.
 */
#define OT_CLI_COAPS_PRIV_KEY                                           \
"-----BEGIN PRIVATE KEY-----\r\n"                                       \
"Our Private Key\r\n"                                                   \
"-----END PRIVATE KEY-----\r\n"

#endif // OPENTHREAD_ENABLE_APPLICATION_COAP_SECURE

/**
 * @}
 *
 */

#ifdef __cplusplus
}  // extern "C"
#endif

#endif /* SRC_CLI_X509_CERT_KEY_HPP_ */
