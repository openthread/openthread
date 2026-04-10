#!/usr/bin/env python3
#
#  Copyright (c) 2026, The OpenThread Authors.
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

import sys
import os

# Add the current directory to sys.path to find verify_utils
CUR_DIR = os.path.dirname(os.path.abspath(__file__))
sys.path.append(CUR_DIR)

import verify_utils
from pktverify import consts
from pktverify.addrs import Ipv6Addr


def verify(pv):
    # 8.6. [1.4] [CERT] mDNS Discovery of TREL Service
    #
    # 8.6.1. Purpose
    # This test covers mDNS discovery of TREL Service on the BR DUT, and validation of the DNS-SD parameters and fields
    #   advertised.
    #
    # 8.6.2. Topology
    # - 1. BR Leader (DUT) - Support multi-radio (TREL and 15.4)
    # - 2. Eth_1 - Reference device on the Adjacent Infrastructure Link performing mDNS discovery
    #
    # Spec Reference   | Section
    # -----------------|----------
    # TREL Discovery   | 15.6.3.1

    pkts = pv.pkts
    pv.summary.show()

    BR_DUT = pv.vars['BR_DUT']
    Eth_1 = pv.vars['Eth_1']

    TREL_SERVICE_TYPE = '_trel._udp.local'
    TREL_INSTANCE_NAME = pv.vars['TREL_INSTANCE_NAME']
    TREL_FULL_SERVICE_NAME = TREL_INSTANCE_NAME + '.' + TREL_SERVICE_TYPE

    BR_EXT_ADDR_BYTES = bytes.fromhex(pv.vars['BR_EXT_ADDR'])
    XPANID_BYTES = bytes.fromhex(pv.vars['XPANID'])

    XA_RECORD = b'xa=' + BR_EXT_ADDR_BYTES
    XP_RECORD = b'xp=' + XPANID_BYTES

    GUA_PREFIX = '910b:1234::/64'

    GUA_PREFIX_ADDR = Ipv6Addr(GUA_PREFIX.split('/')[0])

    # Step 1
    # - Device: Eth_1, BR (DUT)
    # - Description (TREL-8.6): Form the topology. Eth_1 advertises an on-link prefix using multicast ND RA PIO, such
    #   as 910b:1234::/64. Wait for the DUT to become Leader. The DUT automatically configures an IPv6 address on the
    #   AIL based on the advertised prefix.
    # - Pass Criteria:
    #   - N/A
    print("Step 1: Form the topology")

    # Step 2
    # - Device: Eth_1
    # - Description (TREL-8.6): Harness instructs device to send multicast mDNS query QType=PTR for name
    #   “_trel._udp.local”. Note: for test reliability purposes, it's allowed to retransmit this query one additional
    #   time.
    # - Pass Criteria:
    #   - N/A
    print("Step 2: Eth_1 sends mDNS query PTR for _trel._udp.local")
    pkts.filter_eth_src(pv.vars['Eth_1_ETH']).\
        filter(lambda p: p.udp.dstport == 5353).\
        filter(lambda p: p.mdns.flags.response == 0).\
        filter(lambda p: TREL_SERVICE_TYPE in verify_utils.as_list(p.mdns.qry.name)).\
        filter(lambda p: consts.DNS_TYPE_PTR in verify_utils.as_list(p.mdns.qry.type)).\
        must_next()

    # Step 3
    # - Device: BR (DUT)
    # - Description (TREL-8.6): Automatically responds with mDNS response. Note: the response can be unicast or
    #   multicast.
    # - Pass Criteria:
    #   - The DUT MUST send an mDNS response (unicast and/or multicast).
    #   - Eth_1 MUST receive the mDNS response containing the TREL service, as follows, in the Answer Record section:
    #     $ORIGIN local. _trel._udp PTR <name>._trel._udp
    #   - The response MAY contain the following in the Additional Record section or in the Answer Record section:
    #     $ORIGIN local. <name>._trel._udp ( SRV <val1> <val2> <port> <hostname> TXT xa=<extaddr>;xp=<xpanid> )
    #     <hostname> AAAA <link-local address> AAAA <global address>
    #   - Where: <name> is a service instance name selected by BR DUT.
    #   - Where: <val1> is a value >= 0.
    #   - Where: <val2> is a value >= 0.
    #   - Where: <port> is a port number selected by BR DUT.
    #   - Where: <hostname> is a hostname selected by BR DUT.
    #   - Where: <global address> is an address starting with prefix 910b:1234::/64.
    #   - Where: <link-local address> is an IPv6 address starting with fe80::/10.
    #   - Where: <extaddr> is the extended address of BR DUT, binary encoded. It MUST NOT be ASCII hexadecimal encoded.
    #   - Where: <xpanid> is the extended PAN ID of the Thread Network, binary encoded. It MUST NOT be ASCII
    #     hexadecimal encoded.
    #   - Verify that <xpanid> in the TXT record equals the configured Extended PAN ID (XPANID) of the Thread Network.
    print("Step 3: BR (DUT) responds with mDNS response")
    pkts.filter_eth_src(pv.vars['BR_DUT_ETH']).\
        filter(lambda p: p.udp.srcport == 5353).\
        filter(lambda p: p.mdns.flags.response == 1).\
        filter(lambda p: TREL_SERVICE_TYPE in verify_utils.as_list(p.mdns.resp.name)).\
        filter(lambda p: TREL_FULL_SERVICE_NAME in verify_utils.as_list(p.mdns.ptr.domain_name)).\
        filter(lambda p: TREL_FULL_SERVICE_NAME in verify_utils.as_list(p.mdns.resp.name)).\
        filter(lambda p: consts.DNS_TYPE_SRV in verify_utils.as_list(p.mdns.resp.type)).\
        filter(lambda p: all(v >= 0 for v in verify_utils.as_list(p.mdns.srv.priority))).\
        filter(lambda p: all(v >= 0 for v in verify_utils.as_list(p.mdns.srv.weight))).\
        filter(lambda p: all(v > 0 for v in verify_utils.as_list(p.mdns.srv.port))).\
        filter(lambda p: consts.DNS_TYPE_TXT in verify_utils.as_list(p.mdns.resp.type)).\
        filter(lambda p: XA_RECORD in verify_utils.as_list(p.mdns.txt)).\
        filter(lambda p: XP_RECORD in verify_utils.as_list(p.mdns.txt)).\
        filter(lambda p: any(addr.is_link_local for addr in verify_utils.as_list(p.mdns.aaaa))).\
        filter(lambda p: any(addr[:8] == GUA_PREFIX_ADDR[:8] for addr in verify_utils.as_list(p.mdns.aaaa))).\
        must_next()

    # Step 4
    # - Device: Eth_1
    # - Description (TREL-8.6): Harness instructs device to send multicast mDNS query QType=SRV for name
    #   “<name>._trel._udp.local”, the service found in the previous step. Note: <name> is taken from the previous
    #   step response for the PTR record. Note: for test reliability purposes, it's allowed to retransmit this query
    #   one additional time.
    # - Pass Criteria:
    #   - N/A
    print("Step 4: Eth_1 sends mDNS query SRV for service instance")
    pkts.filter_eth_src(pv.vars['Eth_1_ETH']).\
        filter(lambda p: p.udp.dstport == 5353).\
        filter(lambda p: p.mdns.flags.response == 0).\
        filter(lambda p: TREL_FULL_SERVICE_NAME in verify_utils.as_list(p.mdns.qry.name)).\
        filter(lambda p: consts.DNS_TYPE_SRV in verify_utils.as_list(p.mdns.qry.type)).\
        must_next()

    # Step 5
    # - Device: BR (DUT)
    # - Description (TREL-8.6): Automatically responds with mDNS response. Note: the response can be unicast or
    #   multicast.
    # - Pass Criteria:
    #   - The DUT MUST send mDNS response (unicast and/or multicast)
    #   - Eth_1 MUST receive the mDNS response containing the TREL service, as follows, in the Answer Record section:
    #     $ORIGIN local. <name>._trel._udp ( SRV <val1> <val2> <port> <hostname> )
    #   - It MAY contain in the Additional Record section or in the Answer Record section the following: <hostname>
    #     AAAA <link-local address> AAAA <global address>
    #   - Also it MAY contain in the Additional Record section or the Answer Record section the following:
    #     <name>._trel._udp TXT xa=<extaddr>;xp=<xpanid>
    #   - Where: all the names in pointy brackets are as specified in step 3. For all items present, the same
    #     validation as in step 3 MUST be performed.
    print("Step 5: BR (DUT) responds with mDNS response")
    pkts.filter_eth_src(pv.vars['BR_DUT_ETH']).\
        filter(lambda p: p.udp.srcport == 5353).\
        filter(lambda p: p.mdns.flags.response == 1).\
        filter(lambda p: TREL_FULL_SERVICE_NAME in verify_utils.as_list(p.mdns.resp.name)).\
        filter(lambda p: consts.DNS_TYPE_SRV in verify_utils.as_list(p.mdns.resp.type)).\
        filter(lambda p: all(v >= 0 for v in verify_utils.as_list(p.mdns.srv.priority))).\
        filter(lambda p: all(v >= 0 for v in verify_utils.as_list(p.mdns.srv.weight))).\
        filter(lambda p: all(v > 0 for v in verify_utils.as_list(p.mdns.srv.port))).\
        filter(lambda p: not hasattr(p.mdns, 'txt') or (XA_RECORD in verify_utils.as_list(p.mdns.txt) and
                                                        XP_RECORD in verify_utils.as_list(p.mdns.txt))).\
        filter(lambda p: not hasattr(p.mdns, 'aaaa') or (any(addr.is_link_local for addr in verify_utils.as_list(p.mdns.aaaa)) and
                                                         any(addr[:8] == GUA_PREFIX_ADDR[:8] for addr in verify_utils.as_list(p.mdns.aaaa)))).\
        must_next()


if __name__ == '__main__':
    verify_utils.run_main(verify)
