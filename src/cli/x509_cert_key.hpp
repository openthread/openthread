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
 * place your X.509 certificate (PEM format) for ssl session
 * with ECDHE_ECDSA_WITH_AES_128_CCM_8 here.
 */
#define OT_CLI_COAPS_X509_CERT                                          \
"-----BEGIN CERTIFICATE-----\r\n"                                       \
"MIIEsDCCBBGgAwIBAgIJAPAq7RSX0Q7IMAoGCCqGSM49BAMCMIGNMQswCQYDVQQG\r\n"  \
"EwJYWTEQMA4GA1UECAwHTXlTdGF0ZTEPMA0GA1UEBwwGTXlDaXR5MRIwEAYDVQQK\r\n"  \
"DAlNeUNvbXBhbnkxEjAQBgNVBAsMCU15T3JnVW5pdDESMBAGA1UEAwwJbXlvcmcu\r\n"  \
"aW50MR8wHQYJKoZIhvcNAQkBFhBteW1haWxAbXlvcmcuaW50MB4XDTE4MDYxMjEy\r\n"  \
"MTU1N1oXDTQ1MTAyODEyMTU1N1owgY0xCzAJBgNVBAYTAlhZMRAwDgYDVQQIDAdN\r\n"  \
"eVN0YXRlMQ8wDQYDVQQHDAZNeUNpdHkxEjAQBgNVBAoMCU15Q29tcGFueTESMBAG\r\n"  \
"A1UECwwJTXlPcmdVbml0MRIwEAYDVQQDDAlteW9yZy5pbnQxHzAdBgkqhkiG9w0B\r\n"  \
"CQEWEG15bWFpbEBteW9yZy5pbnQwggJcMIIBzwYHKoZIzj0CATCCAcICAQEwTQYH\r\n"  \
"KoZIzj0BAQJCAf//////////////////////////////////////////////////\r\n"  \
"////////////////////////////////////MIGeBEIB////////////////////\r\n"  \
"////////////////////////////////////////////////////////////////\r\n"  \
"//wEQVGVPrlhjhyaH5KaIaC2hUDuotpyW5mzFfO4tImRjvEJ4VYZOVHsfpN7FlLA\r\n"  \
"vTuxvwc1c9+IPSw08e9FH9RrUD8AAxUA0J6IACkcuFOWzGcXOTKEqqDaZLoEgYUE\r\n"  \
"AMaFjga3BATpzZ4+y2YjlbRCnGSBOQU/tSH4KK9ga009uqFLXnfv51ko/h3BJ6L/\r\n"  \
"qN4zSLPBhWpCm/l+fjHC5b1mARg5KWp4mjvABFyKX7QsfRvZmPVESVebRGgXr70X\r\n"  \
"Jz5mLJfucple9CZAxVC5AT+tB2E1PHCGonLCQIi+lHaf0WZQAkIB////////////\r\n"  \
"///////////////////////////////6UYaHg78vlmt/zAFI9wml0Du1ybiJnEeu\r\n"  \
"u2+3HpE4ZAkCAQEDgYYABADSbto4G4UCjvdYwjaWXr7IeTREKDvgHsYWQfFkzsPw\r\n"  \
"2m8PKTPrRAfkti7QMo/Ue62iVfABHAvTW44bNGB6q0T82AEhlx2ocD1lxaZ67Nqz\r\n"  \
"R9egv6M7zZTqTjQncEgt5QLcPCX5nPWmH0vN+2PAp0RA51jpJ6HKdhtOZLvW7Wyq\r\n"  \
"avJyDqNTMFEwHQYDVR0OBBYEFPJw0iZP/FwiqwwoqhhrvGynk8KyMB8GA1UdIwQY\r\n"  \
"MBaAFPJw0iZP/FwiqwwoqhhrvGynk8KyMA8GA1UdEwEB/wQFMAMBAf8wCgYIKoZI\r\n"  \
"zj0EAwIDgYwAMIGIAkIBkwmvY3kTYmJ5zm4zGbJFBVJPHY8Aj3CCyv24ikC4dRSq\r\n"  \
"eyF6btzLElaWdeGOC50IVGJlmvLOErL/6ePFedabzCkCQgHFrpWCu8kzYdZGK1KW\r\n"  \
"660AOSLP8ruy+jebK8+m4LhvlVBo5Wu9F4VcAXUy6J/SBkVEZbqO9GdcTnBLLuTC\r\n"  \
"HJmPzA==\r\n"                                                          \
"-----END CERTIFICATE-----\r\n"

/**
 * place your private key (PEM format) for ssl session
 * with ECDHE_ECDSA_WITH_AES_128_CCM_8 here.
 */
#define OT_CLI_COAPS_PRIV_KEY                                           \
"-----BEGIN PRIVATE KEY-----\r\n"                                       \
"MIICnQIBAQRCAOmk5bpZU16TW8LNkTte/ENBa7yKjB2DO+DTGLDFlTrBun85hFHa\r\n"  \
"/DMRPJ6CVEjCfaXOVnEudMka4K1Tv9xCAg53oIIBxjCCAcICAQEwTQYHKoZIzj0B\r\n"  \
"AQJCAf//////////////////////////////////////////////////////////\r\n"  \
"////////////////////////////MIGeBEIB////////////////////////////\r\n"  \
"//////////////////////////////////////////////////////////wEQVGV\r\n"  \
"PrlhjhyaH5KaIaC2hUDuotpyW5mzFfO4tImRjvEJ4VYZOVHsfpN7FlLAvTuxvwc1\r\n"  \
"c9+IPSw08e9FH9RrUD8AAxUA0J6IACkcuFOWzGcXOTKEqqDaZLoEgYUEAMaFjga3\r\n"  \
"BATpzZ4+y2YjlbRCnGSBOQU/tSH4KK9ga009uqFLXnfv51ko/h3BJ6L/qN4zSLPB\r\n"  \
"hWpCm/l+fjHC5b1mARg5KWp4mjvABFyKX7QsfRvZmPVESVebRGgXr70XJz5mLJfu\r\n"  \
"cple9CZAxVC5AT+tB2E1PHCGonLCQIi+lHaf0WZQAkIB////////////////////\r\n"  \
"///////////////////////6UYaHg78vlmt/zAFI9wml0Du1ybiJnEeuu2+3HpE4\r\n"  \
"ZAkCAQGhgYkDgYYABADSbto4G4UCjvdYwjaWXr7IeTREKDvgHsYWQfFkzsPw2m8P\r\n"  \
"KTPrRAfkti7QMo/Ue62iVfABHAvTW44bNGB6q0T82AEhlx2ocD1lxaZ67NqzR9eg\r\n"  \
"v6M7zZTqTjQncEgt5QLcPCX5nPWmH0vN+2PAp0RA51jpJ6HKdhtOZLvW7WyqavJy\r\n"  \
"Dg==\r\n"                                                              \
"-----END PRIVATE KEY-----\r\n"

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
