#!/usr/bin/env python3
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

from pktverify.addrs import Ipv6Addr
from pktverify.bytes import Bytes

DOMAIN_PREFIX = Bytes('fd00:7d03:7d03:7d03')
BACKBONE_IPV6_PREFIX = Bytes('2001:0db8:0001:0000')

LINK_LOCAL_All_THREAD_NODES_MULTICAST_ADDRESS = Ipv6Addr('ff32:40:fdde:ad00:beef:0:0:1')
REALM_LOCAL_All_THREAD_NODES_MULTICAST_ADDRESS = Ipv6Addr('ff33:40:fdde:ad00:beef:0:0:1')
REALM_LOCAL_ALL_ROUTERS_ADDRESS = Ipv6Addr('ff03::2')
LINK_LOCAL_ALL_NODES_MULTICAST_ADDRESS = Ipv6Addr('ff02::1')
LINK_LOCAL_ALL_ROUTERS_MULTICAST_ADDRESS = Ipv6Addr('ff02::2')
LINK_LOCAL_ALL_BBRS_MULTICAST_ADDRESS = Ipv6Addr('ff32:40:fd00:7d03:7d03:7d03:0:3')

# MA in Test Plan, make sure these are same as ../config.py
MA1 = Ipv6Addr('ff04::1234:777a:1')
MA1g = Ipv6Addr('ff0e::1234:777a:1')
MA2 = Ipv6Addr('ff05::1234:777a:1')
MA3 = Ipv6Addr('ff0e::1234:777a:3')
MA4 = Ipv6Addr('ff05::1234:777a:4')
MA5 = Ipv6Addr('ff03::1234:777a:5')
MA6 = Ipv6Addr('ff02::1')
MAe1 = Ipv6Addr('fd0e::1234:777a:1')
MAe2 = Ipv6Addr('::')
MAe3 = Ipv6Addr('cafe::e0ff')
ALL_MPL_FORWARDERS_MA = Ipv6Addr('ff03::fc')

LINK_LOCAL_PREFIX = Bytes("fe80")
DEFAULT_MESH_LOCAL_PREFIX = Bytes("fd00:0db8:0000:0000")

# COAP methods
COAP_CODE_POST = 2
COAP_CODE_ACK = 68

MLE_LINK_REQUEST = 0
MLE_LINK_ACCEPT = 1
MLE_LINK_ACCEPT_AND_REQUEST = 2
MLE_ADVERTISEMENT = 4
MLE_DATA_RESPONSE = 8
MLE_PARENT_REQUEST = 9
MLE_PARENT_RESPONSE = 10
MLE_CHILD_ID_REQUEST = 11
MLE_CHILD_ID_RESPONSE = 12
MLE_CHILD_UPDATE_REQUEST = 13
MLE_CHILD_UPDATE_RESPONSE = 14

# COAP URI
ADDR_QRY_URI = '/a/aq'
ADDR_NTF_URI = '/a/an'
ADDR_ERR_URI = '/a/ae'
ADDR_SOL_URI = '/a/as'
ADDR_REL_URI = '/a/ar'
SVR_DATA_URI = '/a/sd'
ND_DATA_URI = '/a/nd'

# TLV
SOURCE_ADDRESS_TLV = 0
MODE_TLV = 1
TIMEOUT_TLV = 2
CHALLENGE_TLV = 3
RESPONSE_TLV = 4
LINK_LAYER_FRAME_COUNTER_TLV = 5
LINK_QUALITY_TLV = 6
PARAMETER_TLV = 7
MLE_FRAME_COUNTER_TLV = 8
ROUTE64_TLV = 9
ADDRESS16_TLV = 10
LEADER_DATA_TLV = 11
NETWORK_DATA_TLV = 12
TLV_REQUEST_TLV = 13
SCAN_MASK_TLV = 14
CONNECTIVITY_TLV = 15
LINK_MARGIN_TLV = 16
STATUS_TLV = 17
VERSION_TLV = 18
ADDRESS_REGISTRATION_TLV = 19
CHANNEL_TLV = 20
PAN_ID_TLV = 21
ACTIVE_TIMESTAMP_TLV = 22
PENDING_TIMESTAMP_TLV = 23
ACTIVE_OPERATION_DATASET_TLV = 24
PENDING_OPERATION_DATASET_TLV = 25
THREAD_DISCOVERY_TLV = 26

# DUA related constants

ADDRESS_QUERY_INITIAL_RETRY_DELAY = 15
ADDRESS_QUERY_MAX_RETRY_DELAY = 8
ADDRESS_QUERY_TIMEOUT = 3
ADVERTISEMENT_I_MAX = 32
ADVERTISEMENT_I_MIN = 1

CONTEXT_ID_REUSE_DELAY = 48

DATA_RESUBMIT_DELAY = 300

DUA_DAD_PERIOD = 100
DUA_DAD_QUERY_TIMEOUT = 1.0
DUA_DAD_REPEATS = 2
DUA_RECENT_TIME = 20
FAILED_ROUTER_TRANSMISSIONS = 4
ID_REUSE_DELAY = 100
ID_SEQUENCE_PERIOD = 10
INFINITE_COST_TIMEOUT = 90

REAL_LAYER_NAMES = {
    'mle',
    'coap',
    'wpan',
    'eth',
    'tcp',
    'udp',
    'ip',
    'ipv6',
    'icmpv6',
    '6lowpan',
    'arp',
    'thread_bl',
    'thread_address',
    'thread_nm',
    'ssdp',
    'dns',
    'igmp',
    'mdns',
}

FAKE_LAYER_NAMES = {'thread_nwd', 'thread_meshcop'}

VALID_LAYER_NAMES = REAL_LAYER_NAMES | FAKE_LAYER_NAMES

AUTO_SEEK_BACK_MAX_DURATION = 0.01

# Wireshark configs
WIRESHARK_OVERRIDE_PREFS = {
    '6lowpan.context0': 'fd00:db8::/64',
    '6lowpan.context1': 'fd00:7d03:7d03:7d03::/64',
    'wpan.802154_fcs_ok': 'FALSE',
    'wpan.802154_sec_suite': 'AES-128 Encryption, 32-bit Integrity Protection',
    'thread.thr_seq_ctr': '00000000',
    'uat:ieee802154_keys': '"00112233445566778899aabbccddeeff","1","Thread hash"',
}

WIRESHARK_DECODE_AS_ENTRIES = {
    'udp.port==61631': 'coap',
}

TIMEOUT_JOIN_NETWORK = 10
TIMEOUT_DUA_REGISTRATION = 10
TIMEOUT_DUA_DAD = 15
TIMEOUT_HOST_READY = 10
TIMEOUT_CHILD_DETACH = 120
TIMEOUT_REGISTER_MA = 5

if __name__ == '__main__':
    from pktverify.addrs import Ipv6Addr

    assert Ipv6Addr("fe80:0000:0000:0000:0200:0000:0000:0004").startswith(LINK_LOCAL_PREFIX)
    assert Ipv6Addr("fd00:0db8:0000:0000:0000:00ff:fe00:8001").startswith(DEFAULT_MESH_LOCAL_PREFIX)
