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
    hardware_entropy.c                          \
    repo/library/md.c                           \
    repo/library/md_wrap.c                      \
    repo/library/memory_buffer_alloc.c          \
    repo/library/platform.c                     \
    repo/library/sha256.c                       \
    $(NULL)

if OPENTHREAD_EXAMPLES_EFR32
libmbedcrypto_a_SOURCES                      += \
    @top_builddir@/third_party/silabs/gecko_sdk_suite/v1.0/util/third_party/mbedtls/sl_crypto/src/sl_aes.c \
    $(NULL)
else
libmbedcrypto_a_SOURCES                      += \
    repo/library/aes.c                          \
    $(NULL)
endif  # OPENTHREAD_EXAMPLES_EFR32

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
libmbedcrypto_a_SOURCES                      += \
    @top_builddir@/third_party/silabs/gecko_sdk_suite/v1.0/util/third_party/mbedtls/library/ecp.c \
    @top_builddir@/third_party/silabs/gecko_sdk_suite/v1.0/util/third_party/mbedtls/sl_crypto/src/sl_ecp.c \
    $(NULL)
else
libmbedcrypto_a_SOURCES                      += \
    repo/library/ecp.c                          \
    $(NULL)
endif  # OPENTHREAD_EXAMPLES_EFR32

#
# mbedtls patches
#

MBEDTLS_ABS_SRCDIR                           := $(abspath $(top_srcdir)/third_party/mbedtls)

EXTRA_DIST                                                         += \
    patch/0001-Fix-commissioning-problem-with-retransmissions.patch   \
    $(NULL)

repo/library/libmbedcrypto_a-ssl_tls.$(OBJEXT): $(MBEDTLS_SRCDIR)/library/ssl_tls.c.patched

$(MBEDTLS_SRCDIR)/library/ssl_tls.c.patched: $(MBEDTLS_ABS_SRCDIR)/patch/0001-Fix-commissioning-problem-with-retransmissions.patch
	chmod u+w $(MBEDTLS_ABS_SRCDIR)/repo/library/
	chmod u+w $(MBEDTLS_ABS_SRCDIR)/repo/library/ssl_tls.c
	if [ -e $@ ]; then patch -R $(MBEDTLS_ABS_SRCDIR)/repo/library/ssl_tls.c $@; fi
	patch $(MBEDTLS_ABS_SRCDIR)/repo/library/ssl_tls.c $<
	cp $(MBEDTLS_ABS_SRCDIR)/patch/0001-Fix-commissioning-problem-with-retransmissions.patch $@

all-local: libmbedcrypto.a
	if [ -e $(MBEDTLS_SRCDIR)/library/ssl_tls.c.patched ]; then patch -R $(MBEDTLS_ABS_SRCDIR)/repo/library/ssl_tls.c $(MBEDTLS_ABS_SRCDIR)/patch/0001-Fix-commissioning-problem-with-retransmissions.patch; rm -f $(MBEDTLS_SRCDIR)/library/ssl_tls.c.patched; fi

clean-local:
	if [ -e $(MBEDTLS_SRCDIR)/library/ssl_tls.c.patched ]; then patch -R $(MBEDTLS_ABS_SRCDIR)/repo/library/ssl_tls.c $(MBEDTLS_ABS_SRCDIR)/patch/0001-Fix-commissioning-problem-with-retransmissions.patch; rm -f $(MBEDTLS_SRCDIR)/library/ssl_tls.c.patched; fi

endif  # OPENTHREAD_ENABLE_DTLS

if OPENTHREAD_BUILD_COVERAGE
Dash                                          = -
CLEANFILES                                    = $(shell find $(top_builddir)/third_party/mbedtls $(Dash)name "*.gcda" $(Dash)o $(Dash)name "*.gcno")
endif # OPENTHREAD_BUILD_COVERAGE

include $(abs_top_nlbuild_autotools_dir)/automake/post.am
