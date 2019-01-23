#
#  Copyright (c) 2017, The OpenThread Authors.
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

include $(abs_top_nlbuild_autotools_dir)/automake/pre.am

bin_PROGRAMS                                              = \
    cli-uart-received-fuzzer                                \
    ip6-send-fuzzer                                         \
    radio-receive-done-fuzzer                               \
    ncp-uart-received-fuzzer                                \
    $(NULL)

AM_CPPFLAGS                                               = \
    -I$(top_srcdir)/include                                 \
    -I$(top_srcdir)/src/core                                \
    $(NULL)

COMMON_LDADD                                              = \
    $(top_builddir)/src/core/libopenthread-ftd.a            \
    $(top_builddir)/third_party/mbedtls/libmbedcrypto.a     \
    $(LIB_FUZZING_ENGINE)                                   \
    $(NULL)

cli_uart_received_fuzzer_LDADD                            = \
    $(top_builddir)/src/cli/libopenthread-cli-ftd.a         \
    $(COMMON_LDADD)                                         \
    $(NULL)

cli_uart_received_fuzzer_SOURCES                          = cli_uart_received.cpp fuzzer_platform.c

ip6_send_fuzzer_LDADD                                     = $(COMMON_LDADD)
ip6_send_fuzzer_SOURCES                                   = ip6_send.cpp fuzzer_platform.c

radio_receive_done_fuzzer_LDADD                           = $(COMMON_LDADD)
radio_receive_done_fuzzer_SOURCES                         = radio_receive_done.cpp fuzzer_platform.c

ncp_uart_received_fuzzer_LDADD                            = \
    $(top_builddir)/src/ncp/libopenthread-ncp-ftd.a         \
    $(COMMON_LDADD)                                         \
    $(NULL)

ncp_uart_received_fuzzer_SOURCES                          = ncp_uart_received.cpp fuzzer_platform.c

include $(abs_top_nlbuild_autotools_dir)/automake/post.am
