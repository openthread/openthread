#
#    Copyright 2016 Nest Labs Inc. All Rights Reserved.
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

lib_LIBRARIES                                 = libmbedcrypto.a

MBEDTLS_DIR                                   = $(top_srcdir)/third_party/mbedtls/repo

libmbedcrypto_a_CPPFLAGS                      = \
    -I$(top_srcdir)/include                     \
    -I$(top_srcdir)/src/core                    \
    -I$(MBEDTLS_DIR)/include                    \
    -I$(MBEDTLS_DIR)/include/mbedtls            \
    -DMBEDTLS_CONFIG_FILE=\"mbedtls-config.h\"  \
    $(NULL)

libmbedcrypto_a_SOURCES                       = \
    mbedcrypto.c                                \
    repo/library/aes.c                          \
    repo/library/md.c                           \
    repo/library/md_wrap.c                      \
    repo/library/memory_buffer_alloc.c          \
    repo/library/platform.c                     \
    repo/library/sha256.c                       \
    $(NULL)

include_HEADERS                               = \
    $(NULL)

install-headers: install-includeHEADERS

if OPENTHREAD_BUILD_COVERAGE
CLEANFILES                                    = $(wildcard *.gcda *.gcno)
endif # OPENTHREAD_BUILD_COVERAGE

all:

include $(abs_top_nlbuild_autotools_dir)/automake/post.am
