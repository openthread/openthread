#
#    Copyright 2016 The OpenThread Authors. All Rights Reserved.
#
#    Licensed under the Apache License, Version 2.0 (the "License");
#    you may not use this file except in compliance with the License.
#    You may obtain a copy of the License at
#
#    http://www.apache.org/licenses/LICENSE-2.0
#
#    Unless required by applicable law or agreed to in writing, software
#    distributed under the License is distributed on an "AS IS" BASIS,
#    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#    See the License for the specific language governing permissions and
#    limitations under the License.
#

include $(abs_top_nlbuild_autotools_dir)/automake/pre.am

EXTRA_DIST                                    = \
    mbedtls-config.h                            \
    repo/include                                \
    $(NULL)

lib_LIBRARIES                                 = libmbedcrypto.a

# Do not enable -Wconversion for mbedtls
override CFLAGS                              := $(filter-out -Wconversion,$(CFLAGS))
override CXXFLAGS                            := $(filter-out -Wconversion,$(CXXFLAGS))

# Do not enable -pedantic-errors for mbedtls
override CFLAGS                              := $(filter-out -pedantic-errors,$(CFLAGS))
override CXXFLAGS                            := $(filter-out -pedantic-errors,$(CXXFLAGS))

MBEDTLS_SRCDIR                                = $(top_srcdir)/third_party/mbedtls/repo

libmbedcrypto_a_CPPFLAGS                      = \
    -I$(top_srcdir)/include                     \
    -I$(top_srcdir)/src/core                    \
    -I$(MBEDTLS_SRCDIR)/include                 \
    $(MBEDTLS_CPPFLAGS)                         \
    $(NULL)

libmbedcrypto_a_SOURCES                       = \
    repo/library/md.c                           \
    repo/library/md_wrap.c                      \
    repo/library/memory_buffer_alloc.c          \
    repo/library/platform.c                     \
    repo/library/sha256.c                       \
    $(NULL)

if OPENTHREAD_EXAMPLES_EFR32
nodist_libmbedcrypto_a_SOURCES                                                                                      = \
    @top_builddir@/third_party/silabs/gecko_sdk_suite/v2.2/util/third_party/mbedtls/sl_crypto/src/crypto_aes.c        \
    @top_builddir@/third_party/silabs/gecko_sdk_suite/v2.2/util/third_party/mbedtls/sl_crypto/src/crypto_management.c \
    $(NULL)
else
libmbedcrypto_a_SOURCES                      += \
    repo/library/aes.c                          \
    $(NULL)
endif  # OPENTHREAD_EXAMPLES_EFR32

if OPENTHREAD_EXAMPLES_NRF52840
nodist_libmbedcrypto_a_SOURCES                                                       = \
    @top_builddir@/third_party/NordicSemiconductor/libraries/crypto/aes_alt.c          \
    @top_builddir@/third_party/NordicSemiconductor/libraries/crypto/aes_alt_cc310.c    \
    @top_builddir@/third_party/NordicSemiconductor/libraries/crypto/aes_alt_soft.c     \
    @top_builddir@/third_party/NordicSemiconductor/libraries/crypto/sha256_alt_cc310.c \
    @top_builddir@/third_party/NordicSemiconductor/libraries/crypto/cc310_mbedtls.c    \
    $(NULL)
endif # OPENTHREAD_EXAMPLES_NRF52840

if OPENTHREAD_ENABLE_DTLS
libmbedcrypto_a_SOURCES                      += \
    repo/library/bignum.c                       \
    repo/library/ccm.c                          \
    repo/library/cipher.c                       \
    repo/library/cipher_wrap.c                  \
    repo/library/cmac.c                         \
    repo/library/ctr_drbg.c                     \
    repo/library/debug.c                        \
    repo/library/ecjpake.c                      \
    repo/library/ecp_curves.c                   \
    repo/library/entropy.c                      \
    repo/library/entropy_poll.c                 \
    repo/library/ssl_cookie.c                   \
    repo/library/ssl_ciphersuites.c             \
    repo/library/ssl_cli.c                      \
    repo/library/ssl_srv.c                      \
    repo/library/ssl_ticket.c                   \
    repo/library/ssl_tls.c                      \
    $(NULL)

if OPENTHREAD_EXAMPLES_EFR32
nodist_libmbedcrypto_a_SOURCES                                                                              += \
    @top_builddir@/third_party/silabs/gecko_sdk_suite/v2.2/util/third_party/mbedtls/library/ecp.c              \
    @top_builddir@/third_party/silabs/gecko_sdk_suite/v2.2/util/third_party/mbedtls/sl_crypto/src/crypto_ecp.c \
    $(NULL)
else
libmbedcrypto_a_SOURCES                      += \
    repo/library/ecp.c                          \
    $(NULL)
endif  # OPENTHREAD_EXAMPLES_EFR32

if OPENTHREAD_EXAMPLES_NRF52840
nodist_libmbedcrypto_a_SOURCES                                                    += \
    @top_builddir@/third_party/NordicSemiconductor/libraries/crypto/ecp_alt_cc310.c  \
    @top_builddir@/third_party/NordicSemiconductor/libraries/crypto/ecp_curves_alt.c \
    $(NULL)
endif # OPENTHREAD_EXAMPLES_NRF52840

endif  # OPENTHREAD_ENABLE_DTLS

if OPENTHREAD_BUILD_COVERAGE
Dash                                          = -
CLEANFILES                                    = $(shell find $(top_builddir)/third_party/mbedtls $(Dash)name "*.gcda" $(Dash)o $(Dash)name "*.gcno")
endif # OPENTHREAD_BUILD_COVERAGE

include $(abs_top_nlbuild_autotools_dir)/automake/post.am
