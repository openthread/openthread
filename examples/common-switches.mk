#
#  Copyright (c) 2016-2017, The OpenThread Authors.
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

ifeq ($(BA_PROXY),1)
configure_OPTIONS              += --enable-border-agent-proxy
endif

ifeq ($(BORDER_ROUTER),1)
configure_OPTIONS              += --enable-border-router
endif

ifeq ($(CERT_LOG),1)
configure_OPTIONS              += --enable-cert-log
endif

ifeq ($(COAP),1)
configure_OPTIONS              += --enable-application-coap
endif

ifeq ($(COMMISSIONER),1)
configure_OPTIONS              += --enable-commissioner
endif

ifeq ($(COVERAGE),1)
configure_OPTIONS              += --enable-coverage
endif

ifeq ($(DEBUG),1)
configure_OPTIONS              += --enable-debug --enable-optimization=no
endif

ifeq ($(DHCP6_CLIENT),1)
configure_OPTIONS              += --enable-dhcp6-client
endif

ifeq ($(DHCP6_SERVER),1)
configure_OPTIONS              += --enable-dhcp6-server
endif

ifeq ($(DISABLE_DOC),1)
configure_OPTIONS              += --disable-docs
endif

ifeq ($(DNS_CLIENT),1)
configure_OPTIONS              += --enable-dns-client
endif

ifeq ($(JAM_DETECTION),1)
configure_OPTIONS              += --enable-jam-detection
endif

ifeq ($(JOINER),1)
configure_OPTIONS              += --enable-joiner
endif

ifeq ($(LEGACY),1)
configure_OPTIONS              += --enable-legacy
endif

ifeq ($(MAC_WHITELIST),1)
configure_OPTIONS              += --enable-mac-whitelist
endif

ifeq ($(MTD_NETDIAG),1)
configure_OPTIONS              += --enable-mtd-network-diagnostic
endif
