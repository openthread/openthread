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

# OpenThread Features (Makefile default configuration).

BIG_ENDIAN          ?= 0
BORDER_AGENT        ?= 0
BORDER_ROUTER       ?= 0
COAP                ?= 0
COAPS               ?= 0
COMMISSIONER        ?= 0
COVERAGE            ?= 0
CHANNEL_MANAGER     ?= 0
CHANNEL_MONITOR     ?= 0
CHILD_SUPERVISION   ?= 0
DEBUG               ?= 0
DHCP6_CLIENT        ?= 0
DHCP6_SERVER        ?= 0
DIAGNOSTIC          ?= 0
DISABLE_DOC         ?= 0
DISABLE_TOOLS       ?= 0
DNS_CLIENT          ?= 0
DYNAMIC_LOG_LEVEL   ?= 0
ECDSA               ?= 0
EXTERNAL_HEAP       ?= 0
IP6_FRAGM           ?= 0
JAM_DETECTION       ?= 0
JOINER              ?= 0
LEGACY              ?= 0
ifeq ($(REFERENCE_DEVICE),1)
LOG_OUTPUT          ?= APP
endif
LINK_RAW            ?= 0
MAC_FILTER          ?= 0
MTD_NETDIAG         ?= 0
PLATFORM_UDP        ?= 0
REFERENCE_DEVICE    ?= 0
SERVICE             ?= 0
SETTINGS_RAM        ?= 0
# SLAAC is enabled by default
SLAAC               ?= 1
SNTP_CLIENT         ?= 0
TIME_SYNC           ?= 0
UDP_FORWARD         ?= 0


ifeq ($(BIG_ENDIAN),1)
COMMONCFLAGS                   += -DBYTE_ORDER_BIG_ENDIAN=1
endif

ifeq ($(BORDER_AGENT),1)
COMMONCFLAGS                   += -DOPENTHREAD_CONFIG_BORDER_AGENT_ENABLE=1
endif

ifeq ($(BORDER_ROUTER),1)
COMMONCFLAGS                   += -DOPENTHREAD_CONFIG_BORDER_ROUTER_ENABLE=1
endif

ifeq ($(COAP),1)
COMMONCFLAGS                   += -DOPENTHREAD_CONFIG_COAP_API_ENABLE=1
endif

ifeq ($(COAPS),1)
COMMONCFLAGS                   += -DOPENTHREAD_CONFIG_COAP_SECURE_API_ENABLE=1
endif

ifeq ($(COMMISSIONER),1)
COMMONCFLAGS                   += -DOPENTHREAD_CONFIG_COMMISSIONER_ENABLE=1
endif

ifeq ($(COVERAGE),1)
configure_OPTIONS              += --enable-coverage
endif

ifeq ($(CHANNEL_MANAGER),1)
COMMONCFLAGS                   += -DOPENTHREAD_CONFIG_CHANNEL_MANAGER_ENABLE=1
endif

ifeq ($(CHANNEL_MONITOR),1)
COMMONCFLAGS                   += -DOPENTHREAD_CONFIG_CHANNEL_MONITOR_ENABLE=1
endif

ifeq ($(CHILD_SUPERVISION),1)
COMMONCFLAGS                   += -DOPENTHREAD_CONFIG_CHILD_SUPERVISION_ENABLE=1
endif

ifeq ($(DEBUG),1)
configure_OPTIONS              += --enable-debug --disable-optimization
endif

ifeq ($(DHCP6_CLIENT),1)
COMMONCFLAGS                   += -DOPENTHREAD_CONFIG_DHCP6_CLIENT_ENABLE=1
endif

ifeq ($(DHCP6_SERVER),1)
COMMONCFLAGS                   += -DOPENTHREAD_CONFIG_DHCP6_SERVER_ENABLE=1
endif

ifeq ($(DIAGNOSTIC),1)
COMMONCFLAGS                   += -DOPENTHREAD_CONFIG_DIAG_ENABLE=1
endif

ifeq ($(DISABLE_DOC),1)
configure_OPTIONS              += --disable-docs
endif

ifeq ($(DISABLE_TOOLS),1)
configure_OPTIONS              += --disable-tools
endif

ifeq ($(DNS_CLIENT),1)
COMMONCFLAGS                   += -DOPENTHREAD_CONFIG_DNS_CLIENT_ENABLE=1
endif

ifeq ($(DYNAMIC_LOG_LEVEL),1)
COMMONCFLAGS                   += -DOPENTHREAD_CONFIG_LOG_LEVEL_DYNAMIC_ENABLE=1
endif

ifeq ($(ECDSA),1)
COMMONCFLAGS                   += -DOPENTHREAD_CONFIG_ECDSA_ENABLE=1
endif

ifeq ($(EXTERNAL_HEAP),1)
COMMONCFLAGS                   += -DOPENTHREAD_CONFIG_HEAP_EXTERNAL_ENABLE=1
endif

ifeq ($(IP6_FRAGM),1)
COMMONCFLAGS                   += -DOPENTHREAD_CONFIG_IP6_FRAGMENTATION_ENABLE=1
endif

ifeq ($(JAM_DETECTION),1)
COMMONCFLAGS                   += -DOPENTHREAD_CONFIG_JAM_DETECTION_ENABLE=1
endif

ifeq ($(JOINER),1)
COMMONCFLAGS                   += -DOPENTHREAD_CONFIG_JOINER_ENABLE=1
endif

ifeq ($(LEGACY),1)
COMMONCFLAGS                   += -DOPENTHREAD_CONFIG_LEGACY_ENABLE=1
endif

ifeq ($(LINK_RAW),1)
COMMONCFLAGS                   += -DOPENTHREAD_CONFIG_LINK_RAW_ENABLE=1
endif

ifneq ($(LOG_OUTPUT),)
COMMONCFLAGS                   += -DOPENTHREAD_CONFIG_LOG_OUTPUT=OPENTHREAD_CONFIG_LOG_OUTPUT_$(LOG_OUTPUT)
endif

ifeq ($(MAC_FILTER),1)
COMMONCFLAGS                   += -DOPENTHREAD_CONFIG_MAC_FILTER_ENABLE=1
endif

ifeq ($(MTD_NETDIAG),1)
COMMONCFLAGS                   += -DOPENTHREAD_CONFIG_TMF_NETWORK_DIAG_MTD_ENABLE=1
endif

ifeq ($(PLATFORM_UDP),1)
COMMONCFLAGS                   += -DOPENTHREAD_CONFIG_PLATFORM_UDP_ENABLE=1
endif

# Enable features only required for reference device during certification.
ifeq ($(REFERENCE_DEVICE),1)
COMMONCFLAGS                   += -DOPENTHREAD_CONFIG_REFERENCE_DEVICE_ENABLE=1
endif

ifeq ($(SERVICE),1)
COMMONCFLAGS                   += -DOPENTHREAD_CONFIG_TMF_NETDATA_SERVICE_ENABLE=1
endif

ifeq ($(SLAAC),1)
COMMONCFLAGS                   += -DOPENTHREAD_CONFIG_IP6_SLAAC_ENABLE=1
endif

ifeq ($(SNTP_CLIENT),1)
COMMONCFLAGS                   += -DOPENTHREAD_CONFIG_SNTP_CLIENT_ENABLE=1
endif

ifeq ($(TIME_SYNC),1)
COMMONCFLAGS                   += -DOPENTHREAD_CONFIG_TIME_SYNC_ENABLE=1 -DOPENTHREAD_CONFIG_MAC_HEADER_IE_SUPPORT=1
endif

ifeq ($(UDP_FORWARD),1)
COMMONCFLAGS                   += -DOPENTHREAD_CONFIG_UDP_FORWARD_ENABLE=1
endif

ifeq ($(DISABLE_BUILTIN_MBEDTLS),1)
configure_OPTIONS              += --disable-builtin-mbedtls
endif

ifneq ($(BUILTIN_MBEDTLS_MANAGEMENT),)
COMMONCFLAGS                   += -DOPENTHREAD_CONFIG_ENABLE_BUILTIN_MBEDTLS_MANAGEMENT=$(BUILTIN_MBEDTLS_MANAGEMENT)
endif

ifeq ($(DISABLE_EXECUTABLE),1)
configure_OPTIONS              += --enable-executable=no
endif

ifeq ($(DEBUG_UART),1)
CFLAGS   += -DOPENTHREAD_CONFIG_ENABLE_DEBUG_UART=1
CXXFLAGS += -DOPENTHREAD_CONFIG_ENABLE_DEBUG_UART=1
endif

ifeq ($(DEBUG_UART_LOG),1)
CFLAGS   += -DOPENTHREAD_CONFIG_LOG_OUTPUT=OPENTHREAD_CONFIG_LOG_OUTPUT_DEBUG_UART
CXXFLAGS += -DOPENTHREAD_CONFIG_LOG_OUTPUT=OPENTHREAD_CONFIG_LOG_OUTPUT_DEBUG_UART
endif

ifeq ($(SETTINGS_RAM),1)
COMMONCFLAGS += -DOPENTHREAD_SETTINGS_RAM=1
else
COMMONCFLAGS += -DOPENTHREAD_SETTINGS_RAM=0
endif

ifeq ($(FULL_LOGS),1)
# HINT: Add more here, or comment out ones you do not need/want
LOG_FLAGS += -DOPENTHREAD_CONFIG_LOG_LEVEL=OT_LOG_LEVEL_DEBG
LOG_FLAGS += -DOPENTHREAD_CONFIG_LOG_API=1
LOG_FLAGS += -DOPENTHREAD_CONFIG_LOG_ARP=1
LOG_FLAGS += -DOPENTHREAD_CONFIG_LOG_CLI=1
LOG_FLAGS += -DOPENTHREAD_CONFIG_LOG_COAP=1
LOG_FLAGS += -DOPENTHREAD_CONFIG_LOG_ICMP=1
LOG_FLAGS += -DOPENTHREAD_CONFIG_LOG_IP6=1
LOG_FLAGS += -DOPENTHREAD_CONFIG_LOG_MAC=1
LOG_FLAGS += -DOPENTHREAD_CONFIG_LOG_MEM=1
LOG_FLAGS += -DOPENTHREAD_CONFIG_LOG_MLE=1
LOG_FLAGS += -DOPENTHREAD_CONFIG_LOG_NETDATA=1
LOG_FLAGS += -DOPENTHREAD_CONFIG_LOG_NETDIAG=1
LOG_FLAGS += -DOPENTHREAD_CONFIG_LOG_PKT_DUMP=1
LOG_FLAGS += -DOPENTHREAD_CONFIG_LOG_PLATFORM=1
LOG_FLAGS += -DOPENTHREAD_CONFIG_LOG_PREPEND_LEVEL=1
LOG_FLAGS += -DOPENTHREAD_CONFIG_LOG_PREPEND_REGION=1
endif

CFLAGS += ${LOG_FLAGS}
CXXFLAGS += ${LOG_FLAGS}
