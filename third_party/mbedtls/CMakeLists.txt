#
#  Copyright (c) 2019, The OpenThread Authors.
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

add_library(mbedcrypto)

target_compile_definitions(mbedcrypto PRIVATE
    ${OT_PRIVATE_DEFINES}
)

target_include_directories(mbedcrypto PRIVATE
    ${OT_PUBLIC_INCLUDES}
    ${OT_PRIVATE_INCLUDES}
    ${PROJECT_SOURCE_DIR}/src/core
)

target_sources(mbedcrypto PRIVATE
    repo/library/aes.c
    repo/library/asn1parse.c
    repo/library/asn1write.c
    repo/library/base64.c
    repo/library/bignum.c
    repo/library/ccm.c
    repo/library/cipher.c
    repo/library/cipher_wrap.c
    repo/library/cmac.c
    repo/library/ctr_drbg.c
    repo/library/debug.c
    repo/library/ecdh.c
    repo/library/ecdsa.c
    repo/library/ecjpake.c
    repo/library/ecp.c
    repo/library/ecp_curves.c
    repo/library/entropy.c
    repo/library/entropy_poll.c
    repo/library/md.c
    repo/library/md_wrap.c
    repo/library/memory_buffer_alloc.c
    repo/library/oid.c
    repo/library/pem.c
    repo/library/pk.c
    repo/library/pk_wrap.c
    repo/library/pkparse.c
    repo/library/platform.c
    repo/library/platform_util.c
    repo/library/sha256.c
    repo/library/ssl_cookie.c
    repo/library/ssl_ciphersuites.c
    repo/library/ssl_cli.c
    repo/library/ssl_srv.c
    repo/library/ssl_ticket.c
    repo/library/ssl_tls.c
    repo/library/threading.c
    repo/library/x509.c
    repo/library/x509_crt.c
)
