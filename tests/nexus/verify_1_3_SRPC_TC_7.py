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
import struct

# Add the current directory to sys.path to find verify_utils
CUR_DIR = os.path.dirname(os.path.abspath(__file__))
sys.path.append(CUR_DIR)

import verify_utils
from pktverify import consts
from pktverify import layer_fields
from pktverify.addrs import Ipv6Addr
from pktverify.bytes import Bytes

# Monkey-patch dns.key.public_key field
layer_fields._LAYER_FIELDS['dns.key.public_key'] = layer_fields._list(layer_fields._bytes)
layer_fields._layer_containers.add('dns.key')


def verify(pv):
    # 3.7. [1.3] [CERT] [COMPONENT] Thread Device Reboots - Re-registers service with same KEY
    #
    # 3.7.1. Purpose
    # To verify that the Thread Component DUT:
    # - Is able to re-register a service again after reboot.
    # - Uses the same key for signing the SRP Update, as indicated in the KEY record.
    #
    # 3.7.2. Topology
    # - BR 1 Border Router Reference Device and Leader
    # - Router 1-Thread Router reference device
    # - TD_1 (DUT)-Any Thread Device (FTD or MTD): Thread Component DUT
    # - Eth 1-IPv6 host reference device on the AIL
    #
    # Spec Reference   | V1.1 Section | V1.3.0 Section
    # -----------------|--------------|---------------
    # SRP Client       | N/A          | 2.3.2

    pkts = pv.pkts
    pv.summary.show()

    BR_1_MLEID = Ipv6Addr(pv.vars['BR_1_MLEID_ADDR'])
    TD_1_MLEID = Ipv6Addr(pv.vars['TD_1_MLEID_ADDR'])
    TD_1_OMR_ADDR = Ipv6Addr(pv.vars['TD_1_OMR_ADDR'])
    BR_1_SRP_PORT = int(pv.vars['BR_1_SRP_PORT'])

    SRP_SERVICE_NAME = '_thread-test._udp.default.service.arpa'
    SRP_INSTANCE_NAME = 'service-test-1.' + SRP_SERVICE_NAME
    SRP_SERVICE_PORT = 33333

    SRP_LEASE_1H_1D = Bytes(struct.pack('>II', 3600, 86400).hex())

    # Step 1
    # - Device: Eth 1, BR 1, Router 1, TD 1
    # - Description (SRPC-3.7): Enable & form topology
    # - Pass Criteria:
    #   - Single Thread Network is formed
    print("Step 1: Enable & form topology")

    # Step 2
    # - Device: TD 1 (DUT)
    # - Description (SRPC-3.7): Harness instructs the DUT to register the service: $ORIGIN default.service.arpa.
    #   service-test-1._thread-test._udp ( SRV 33333 host-test-1 ) host-test-1 AAAA <OMR address of TD_1> with the
    #   following options: Update Lease Option Lease: 60 minutes Key Lease: 1 day
    # - Pass Criteria:
    #   - The DUT MUST send an SRP Update to BR_1
    #   - BR_1 MUST respond Rcode=0 (NoError).
    print("Step 2: Harness instructs the DUT to register the service")
    update1 = pkts.filter_ipv6_dst(BR_1_MLEID).\
        filter_ipv6_src(TD_1_MLEID).\
        filter(lambda p: p.udp.dstport == BR_1_SRP_PORT).\
        filter(lambda p: p.dns.flags.opcode == consts.DNS_OPCODE_UPDATE).\
        filter(lambda p: SRP_INSTANCE_NAME in verify_utils.as_list(p.dns.resp.name)).\
        filter(lambda p: SRP_SERVICE_PORT in verify_utils.as_list(p.dns.srv.port)).\
        filter(lambda p: TD_1_OMR_ADDR in verify_utils.as_list(p.dns.aaaa)).\
        filter(lambda p: p.dns.opt.data is not None).\
        filter(lambda p: any(data.format_compact() == SRP_LEASE_1H_1D.format_compact() for data in p.dns.opt.data)).\
        must_next()

    pkts.filter_ipv6_src(BR_1_MLEID).\
        filter_ipv6_dst(TD_1_MLEID).\
        filter(lambda p: p.udp.srcport == BR_1_SRP_PORT).\
        filter(lambda p: p.dns.flags.response == 1).\
        filter(lambda p: p.dns.flags.rcode == consts.DNS_RCODE_NOERROR).\
        must_next()

    key1 = update1.dns.key.public_key

    # Step 3
    # - Device: TD 1 (DUT)
    # - Description (SRPC-3.7): Harness instructs device to reboot.
    # - Pass Criteria:
    #   - N/A
    print("Step 3: Harness instructs device to reboot.")

    # Step 4
    # - Device: TD 1 (DUT)
    # - Description (SRPC-3.7): (Repeat step 2)
    # - Pass Criteria:
    #   - The DUT MUST send an SRP Update to BR_1
    #   - The KEY record in the SRP Update MUST have an equal value to the KEY record that was sent in step 2
    #   - BR_1 MUST respond Rcode=0 (NoError).
    print("Step 4: (Repeat step 2)")
    update2 = pkts.filter_ipv6_dst(BR_1_MLEID).\
        filter_ipv6_src(TD_1_MLEID).\
        filter(lambda p: p.udp.dstport == BR_1_SRP_PORT).\
        filter(lambda p: p.dns.flags.opcode == consts.DNS_OPCODE_UPDATE).\
        filter(lambda p: SRP_INSTANCE_NAME in verify_utils.as_list(p.dns.resp.name)).\
        filter(lambda p: SRP_SERVICE_PORT in verify_utils.as_list(p.dns.srv.port)).\
        filter(lambda p: TD_1_OMR_ADDR in verify_utils.as_list(p.dns.aaaa)).\
        filter(lambda p: p.dns.opt.data is not None).\
        filter(lambda p: any(data.format_compact() == SRP_LEASE_1H_1D.format_compact() for data in p.dns.opt.data)).\
        filter(lambda p: p.dns.key.public_key == key1).\
        must_next()

    pkts.filter_ipv6_src(BR_1_MLEID).\
        filter_ipv6_dst(TD_1_MLEID).\
        filter(lambda p: p.udp.srcport == BR_1_SRP_PORT).\
        filter(lambda p: p.dns.flags.response == 1).\
        filter(lambda p: p.dns.flags.rcode == consts.DNS_RCODE_NOERROR).\
        must_next()


if __name__ == '__main__':
    verify_utils.run_main(verify)
