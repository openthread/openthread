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
    # 2.15. [1.3] [CERT] Validation of SRP subtypes
    #
    # 2.15.1 Purpose
    # To test the following:
    # - 1. Handle service that includes additional subtypes of the basic service type.
    # - 2. Respond to the DNS queries for the basic service type.
    # - 3. Respond to the DNS queries for the service subtype.
    # - 4. Handle addition of new subtype to the service.
    # - 5. Handle removal of a subtype from the service.
    # - 6. Test service types using 'tcp' in <Service> part of name.
    #
    # 2.15.2 Topology
    # - 3. BR 1 (DUT) - Thread Border Router, and the Leader
    # - 4. ED 1-Test Bed device operating as a Thread End Device, attached to BR_1
    # - 5. Eth 1-Test Bed border router device on an Adjacent Infrastructure Link
    #
    # Spec Reference   | V1.1 Section | V1.3.0 Section
    # -----------------|--------------|---------------
    # SRP Server       | N/A          | 1.3

    pkts = pv.pkts
    pv.summary.show()

    ED_1_MLEID = Ipv6Addr(pv.vars['ED_1_MLEID_ADDR'])
    BR_1_MLEID = Ipv6Addr(pv.vars['BR_1_MLEID_ADDR'])
    br1_srp_port = int(pv.vars['BR_1_SRP_PORT'])

    SRP_SERVICE_NAME = '_test-type._tcp.default.service.arpa'
    SRP_SUBTYPE1_NAME = '_test-subtype._sub._test-type._tcp.default.service.arpa'
    SRP_SUBTYPE2_NAME = '_other-subtype._sub._test-type._tcp.default.service.arpa'
    SRP_INSTANCE_NAME = 'service-test-1._test-type._tcp.default.service.arpa'
    SRP_HOST_NAME = 'host-test-1.default.service.arpa'
    SRP_SERVICE_PORT = 55555

    MDNS_SERVICE_NAME = '_test-type._tcp.local'
    MDNS_SUBTYPE1_NAME = '_test-subtype._sub._test-type._tcp.local'
    MDNS_SUBTYPE2_NAME = '_other-subtype._sub._test-type._tcp.local'
    MDNS_SUBTYPE_INVALID_NAME = '_test-subtypee._sub._test-type._tcp.local'
    MDNS_INSTANCE_NAME = 'service-test-1._test-type._tcp.local'

    SERVICE_ID_SRP_SERVER = 93
    DNS_PORT = 53
    MDNS_PORT = 5353

    # Step 1
    #   Device: BR 1 (DUT)
    #   Description (SRP-2.15): Enable
    #   Pass Criteria:
    #   - N/A
    print("Step 1: BR 1 (DUT) Enable")

    # Step 2
    #   Device: Eth 1, ED 1
    #   Description (SRP-2.15): Enable
    #   Pass Criteria:
    #   - N/A
    print("Step 2: Eth 1, ED 1 Enable")

    # Step 3
    #   Device: BR 1 (DUT)
    #   Description (SRP-2.15): Automatically adds SRP Server information in the Thread Network Data.
    #   Pass Criteria:
    #   - The DUT's SRP Server information MUST appear in the Thread Network Data.
    print("Step 3: BR 1 (DUT) adds SRP Server information in Network Data")
    pkts.\
        filter_mle_cmd(consts.MLE_DATA_RESPONSE).\
        filter(lambda p: SERVICE_ID_SRP_SERVER in\
               verify_utils.as_list(p.thread_nwd.tlv.service.srp_dataset_identifier)).\
        filter(lambda p: any(l in (1, 19) for l in verify_utils.as_list(p.thread_nwd.tlv.service.s_data_len))).\
        must_next()

    # Step 4
    #   Device: ED 1
    #   Description (SRP-2.15): Harness instructs the device to send SRP Update to the DUT that includes a subtype:
    #     $ORIGIN default.service.arpa. _test-subtype._sub._test-type._tcp ( PTR service-test-1._test-type._tcp)
    #     _test-type._tcp ( PTR service-test-1._test-type._tcp) service-test-1._test-type._tcp ( SRV 8 55555
    #     host-test-1) host-test-1 AAAA <OMR address of ED 1>
    #   Pass Criteria:
    #   - N/A
    print("Step 4: ED 1 sends SRP Update with a subtype")
    pkts.\
        filter_ipv6_dst(BR_1_MLEID).\
        filter_ipv6_src(ED_1_MLEID).\
        filter(lambda p: p.udp.dstport == br1_srp_port).\
        filter(lambda p: p.dns.flags.opcode == 5).\
        filter(lambda p: SRP_SUBTYPE1_NAME in verify_utils.as_list(p.dns.resp.name)).\
        filter(lambda p: SRP_SERVICE_NAME in verify_utils.as_list(p.dns.resp.name)).\
        filter(lambda p: SRP_INSTANCE_NAME in verify_utils.as_list(p.dns.resp.name)).\
        must_next()

    # Step 5
    #   Device: BR 1 (DUT)
    #   Description (SRP-2.15): Automatically sends SRP Update Response.
    #   Pass Criteria:
    #   - The DUT MUST send a valid SRP Update Response with RCODE=0 (NoError).
    print("Step 5: BR 1 (DUT) sends SRP Update Response")
    pkts.\
        filter_ipv6_dst(ED_1_MLEID).\
        filter_ipv6_src(BR_1_MLEID).\
        filter(lambda p: p.udp.srcport == br1_srp_port).\
        filter(lambda p: p.dns.flags.response == 1).\
        filter(lambda p: p.dns.flags.rcode == 0).\
        must_next()

    # Step 6
    #   Device: ED 1
    #   Description (SRP-2.15): Harness instructs the device to unicast DNS query QType = PTR for
    #     "_test-type._tcp.default.service.arpa" service type to the DUT.
    #   Pass Criteria:
    #   - N/A
    print("Step 6: ED 1 sends unicast DNS query QType PTR for service type")
    pkts.\
        filter_ipv6_dst(BR_1_MLEID).\
        filter_ipv6_src(ED_1_MLEID).\
        filter(lambda p: p.udp.dstport == DNS_PORT).\
        filter(lambda p: p.dns.flags.response == 0).\
        filter(lambda p: SRP_SERVICE_NAME in verify_utils.as_list(p.dns.qry.name)).\
        must_next()

    # Step 7
    #   Device: BR_1 (DUT)
    #   Description (SRP-2.15): Automatically sends DNS response with service-test-1.
    #   Pass Criteria:
    #   - The DUT MUST send a valid DNS Response and contain service-test-1 as follows in the Answer section:
    #     $ORIGIN default.service.arpa. _test-type._tcp ( PTR service-test-1. test-type._tcp)
    #   - It MAY also contain in the Additional records section: $ORIGIN default.service.arpa.
    #     service-test-1._test-type._tcp ( SRV 8 8 55555 host-test-1)
    #   - host-test-1 AAAA <OMR address of ED 1>
    print("Step 7: BR 1 (DUT) sends DNS response with service-test-1")
    pkts.\
        filter_ipv6_dst(ED_1_MLEID).\
        filter_ipv6_src(BR_1_MLEID).\
        filter(lambda p: p.udp.srcport == DNS_PORT).\
        filter(lambda p: p.dns.flags.response == 1).\
        filter(lambda p: p.dns.flags.rcode == 0).\
        filter(lambda p: SRP_SERVICE_NAME in verify_utils.as_list(p.dns.resp.name)).\
        filter(lambda p: SRP_INSTANCE_NAME in verify_utils.as_list(p.dns.ptr.domain_name)).\
        filter(lambda p: SRP_INSTANCE_NAME in verify_utils.as_list(p.dns.resp.name)).\
        filter(lambda p: SRP_HOST_NAME in verify_utils.as_list(p.dns.srv.target)).\
        filter(lambda p: SRP_SERVICE_PORT in verify_utils.as_list(p.dns.srv.port)).\
        must_next()

    # Step 8
    #   Device: Eth 1
    #   Description (SRP-2.15): Harness instructs the device to send mDNS query QType PTR for _test-type._tcp.local."
    #     service type.
    #   Pass Criteria:
    #   - N/A
    print("Step 8: Eth 1 sends mDNS query QType PTR for service type")
    pkts.\
        filter(lambda p: p.udp.dstport == MDNS_PORT).\
        filter(lambda p: p.mdns.flags.response == 0).\
        filter(lambda p: MDNS_SERVICE_NAME in verify_utils.as_list(p.mdns.qry.name)).\
        must_next()

    # Step 9
    #   Device: BR 1 (DUT)
    #   Description (SRP-2.15): Automatically sends mDNS Response with service-test-1.
    #   Pass Criteria:
    #   - The DUT MUST send a valid mDNS Response with RCODE=0 (NoError) and contain service-test-1 as follows in the
    #     Answer section: $ORIGIN local. _test-type._tcp ( PTR service-test-1. test-type._tcp)
    #   - It MAY also contain in the Additional records section: $ORIGIN local. Service-test-1._test-type. tcp ( SRV 8
    #     55555 host-test-1)
    #   - host-test-1 AAAA <OMR address of ED_1>
    print("Step 9: BR 1 (DUT) sends mDNS Response with service-test-1")
    pkts.\
        filter(lambda p: p.udp.srcport == MDNS_PORT).\
        filter(lambda p: p.mdns.flags.response == 1).\
        filter(lambda p: p.mdns.flags.rcode == 0).\
        filter(lambda p: MDNS_SERVICE_NAME in verify_utils.as_list(p.mdns.resp.name)).\
        filter(lambda p: MDNS_INSTANCE_NAME in verify_utils.as_list(p.mdns.ptr.domain_name)).\
        filter(lambda p: MDNS_INSTANCE_NAME in verify_utils.as_list(p.mdns.resp.name)).\
        filter(lambda p: SRP_SERVICE_PORT in verify_utils.as_list(p.mdns.srv.port)).\
        must_next()

    # Step 10
    #   Device: ED 1
    #   Description (SRP-2.15): Harness instructs the device to unicast DNS query QType PTR for
    #     _test-subtype._sub._test-type._tcp.default.service.arpa" service type to the DUT.
    #   Pass Criteria:
    #   - N/A
    print("Step 10: ED 1 sends unicast DNS query QType PTR for service subtype")
    pkts.\
        filter_ipv6_dst(BR_1_MLEID).\
        filter_ipv6_src(ED_1_MLEID).\
        filter(lambda p: p.udp.dstport == DNS_PORT).\
        filter(lambda p: p.dns.flags.response == 0).\
        filter(lambda p: SRP_SUBTYPE1_NAME in verify_utils.as_list(p.dns.qry.name)).\
        must_next()

    # Step 11
    #   Device: BR_1 (DUT)
    #   Description (SRP-2.15): Automatically sends DNS response with service-test-1.
    #   Pass Criteria:
    #   - The DUT MUST send a valid DNS Response and contain service-test-1 as follows in the Answer records section:
    #     $ORIGIN default.service.arpa. _test-subtype._sub._test-type._tcp ( PTR service-test-1. test-type._tcp)
    #   - It MAY also contain in the Additional records section: $ORIGIN default.service.arpa.
    #     service-test-1._test-type. tcp ( SRV 55555 host-test-1)
    #   - host-test-1 AAAA <OMR address of ED_1>
    print("Step 11: BR 1 (DUT) sends DNS response with service-test-1")
    pkts.\
        filter_ipv6_dst(ED_1_MLEID).\
        filter_ipv6_src(BR_1_MLEID).\
        filter(lambda p: p.udp.srcport == DNS_PORT).\
        filter(lambda p: p.dns.flags.response == 1).\
        filter(lambda p: p.dns.flags.rcode == 0).\
        filter(lambda p: SRP_SUBTYPE1_NAME in verify_utils.as_list(p.dns.resp.name)).\
        filter(lambda p: SRP_INSTANCE_NAME in verify_utils.as_list(p.dns.ptr.domain_name)).\
        filter(lambda p: SRP_INSTANCE_NAME in verify_utils.as_list(p.dns.resp.name)).\
        filter(lambda p: SRP_HOST_NAME in verify_utils.as_list(p.dns.srv.target)).\
        filter(lambda p: SRP_SERVICE_PORT in verify_utils.as_list(p.dns.srv.port)).\
        must_next()

    # Step 12
    #   Device: Eth 1
    #   Description (SRP-2.15): Harness instructs the device to mDNS query QType PTR for
    #     "_test-subtype._sub._test-type._tcp.local" service type.
    #   Pass Criteria:
    #   - N/A
    print("Step 12: Eth 1 sends mDNS query QType PTR for service subtype")
    pkts.\
        filter(lambda p: p.udp.dstport == MDNS_PORT).\
        filter(lambda p: p.mdns.flags.response == 0).\
        filter(lambda p: MDNS_SUBTYPE1_NAME in verify_utils.as_list(p.mdns.qry.name)).\
        must_next()

    # Step 13
    #   Device: BR 1 (DUT)
    #   Description (SRP-2.15): Automatically sends mDNS Response with service-test-1.
    #   Pass Criteria:
    #   - The DUT MUST send a valid mDNS Response with RCODE=0 (NoError) and contain service-test-1 as follows in the
    #     Answer records: $ORIGIN local. _test-subtype._sub._test-type._tcp ( PTR service-test-1. test-type._tcp)
    #   - It MAY also contain in the Additional records section: $ORIGIN local. Service-test-1. test-type._tcp ( SRV 8
    #     55555 host-test-1)
    #   - host-test-1 AAAA OMR address of ED_1>
    print("Step 13: BR 1 (DUT) sends mDNS Response with service-test-1")
    pkts.\
        filter(lambda p: p.udp.srcport == MDNS_PORT).\
        filter(lambda p: p.mdns.flags.response == 1).\
        filter(lambda p: p.mdns.flags.rcode == 0).\
        filter(lambda p: MDNS_SUBTYPE1_NAME in verify_utils.as_list(p.mdns.resp.name)).\
        filter(lambda p: MDNS_INSTANCE_NAME in verify_utils.as_list(p.mdns.ptr.domain_name)).\
        filter(lambda p: MDNS_INSTANCE_NAME in verify_utils.as_list(p.mdns.resp.name)).\
        filter(lambda p: SRP_SERVICE_PORT in verify_utils.as_list(p.mdns.srv.port)).\
        must_next()

    # Step 14
    #   Device: ED 1
    #   Description (SRP-2.15): Harness instructs the device to send SRP Update to the DUT to add a new subtype to the
    #     existing service: $ORIGIN default.service.arpa. _other-subtype._sub._test-type._tcp ( PTR service-test-1.
    #     test-type._tcp) _test-subtype._sub._test-type._tcp ( PTR service-test-1._test-type._tcp) _test-type._tcp ( PTR
    #     service-test-1. test-type._tcp) service-test-1. test-type._tcp ( SRV 8 8 55555 host-test-1) host-test-1 AAAA
    #     OMR address of ED_1>
    #   Pass Criteria:
    #   - N/A
    print("Step 14: ED 1 adds a new subtype to the existing service")
    pkts.\
        filter_ipv6_dst(BR_1_MLEID).\
        filter_ipv6_src(ED_1_MLEID).\
        filter(lambda p: p.udp.dstport == br1_srp_port).\
        filter(lambda p: p.dns.flags.opcode == 5).\
        filter(lambda p: SRP_SUBTYPE1_NAME in verify_utils.as_list(p.dns.resp.name)).\
        filter(lambda p: SRP_SUBTYPE2_NAME in verify_utils.as_list(p.dns.resp.name)).\
        filter(lambda p: SRP_SERVICE_NAME in verify_utils.as_list(p.dns.resp.name)).\
        filter(lambda p: SRP_INSTANCE_NAME in verify_utils.as_list(p.dns.resp.name)).\
        must_next()

    # Step 15
    #   Device: BR 1 (DUT)
    #   Description (SRP-2.15): Automatically sends SRP Update Response.
    #   Pass Criteria:
    #   - The DUT MUST send a valid SRP Update Response with RCODE=0 (NoError).
    print("Step 15: BR 1 (DUT) sends SRP Update Response")
    pkts.\
        filter_ipv6_dst(ED_1_MLEID).\
        filter_ipv6_src(BR_1_MLEID).\
        filter(lambda p: p.udp.srcport == br1_srp_port).\
        filter(lambda p: p.dns.flags.response == 1).\
        filter(lambda p: p.dns.flags.rcode == 0).\
        must_next()

    # Step 16
    #   Device: Eth_1
    #   Description (SRP-2.15): Harness instructs the device to mDNS query QType PTR for
    #     "_other-subtype._sub._test-type._tcp.local" service type, to check that the new subtype is present.
    #   Pass Criteria:
    #   - N/A
    print("Step 16: Eth 1 sends mDNS query QType PTR for the new subtype")
    pkts.\
        filter(lambda p: p.udp.dstport == MDNS_PORT).\
        filter(lambda p: p.mdns.flags.response == 0).\
        filter(lambda p: MDNS_SUBTYPE2_NAME in verify_utils.as_list(p.mdns.qry.name)).\
        must_next()

    # Step 17
    #   Device: BR 1 (DUT)
    #   Description (SRP-2.15): Automatically sends mDNS Response with service-test-1.
    #   Pass Criteria:
    #   - The DUT MUST send a valid mDNS Response with RCODE=0 (NoError) and contain service-test-1 as follows in the
    #     Answer records section: $ORIGIN local. _other-subtype._sub._test-type._tcp ( PTR
    #     service-test-1._test-type._tcp)
    #   - It MAY also contain in the Additional records section: $ORIGIN local. Service-test-1. test-type._tcp ( SRV
    #     55555 host-test-1)
    #   - host-test-1 AAAA OMR address of ED_1>
    print("Step 17: BR 1 (DUT) sends mDNS Response with service-test-1")
    pkts.\
        filter(lambda p: p.udp.srcport == MDNS_PORT).\
        filter(lambda p: p.mdns.flags.response == 1).\
        filter(lambda p: p.mdns.flags.rcode == 0).\
        filter(lambda p: MDNS_SUBTYPE2_NAME in verify_utils.as_list(p.mdns.resp.name)).\
        filter(lambda p: MDNS_INSTANCE_NAME in verify_utils.as_list(p.mdns.ptr.domain_name)).\
        filter(lambda p: MDNS_INSTANCE_NAME in verify_utils.as_list(p.mdns.resp.name)).\
        filter(lambda p: SRP_SERVICE_PORT in verify_utils.as_list(p.mdns.srv.port)).\
        must_next()

    # Step 18
    #   Device: Eth_1
    #   Description (SRP-2.15): Harness instructs the device to mDNS query QType=PTR for
    #     "_test-subtype._sub._test-type._tcp.local" service type, to check that the originally registered subtype is
    #     still present.
    #   Pass Criteria:
    #   - N/A
    print("Step 18: Eth 1 sends mDNS query QType PTR for the original subtype")
    pkts.\
        filter(lambda p: p.udp.dstport == MDNS_PORT).\
        filter(lambda p: p.mdns.flags.response == 0).\
        filter(lambda p: MDNS_SUBTYPE1_NAME in verify_utils.as_list(p.mdns.qry.name)).\
        must_next()

    # Step 19
    #   Device: BR 1 (DUT)
    #   Description (SRP-2.15): Automatically sends mDNS Response with service-test-1.
    #   Pass Criteria:
    #   - Repeat Step 13
    print("Step 19: BR 1 (DUT) sends mDNS Response (Repeat Step 13)")
    pkts.\
        filter(lambda p: p.udp.srcport == MDNS_PORT).\
        filter(lambda p: p.mdns.flags.response == 1).\
        filter(lambda p: p.mdns.flags.rcode == 0).\
        filter(lambda p: MDNS_SUBTYPE1_NAME in verify_utils.as_list(p.mdns.resp.name)).\
        filter(lambda p: MDNS_INSTANCE_NAME in verify_utils.as_list(p.mdns.ptr.domain_name)).\
        filter(lambda p: MDNS_INSTANCE_NAME in verify_utils.as_list(p.mdns.resp.name)).\
        filter(lambda p: SRP_SERVICE_PORT in verify_utils.as_list(p.mdns.srv.port)).\
        must_next()

    # Step 20
    #   Device: Eth_1
    #   Description (SRP-2.15): Harness instructs the device to send mDNS query QType PTR for _test-type._tcp.local."
    #     service type, to check that the query for the parent type also works after new subtype registration.
    #   Pass Criteria:
    #   - N/A
    print("Step 20: Eth 1 sends mDNS query QType PTR for the parent service type")
    pkts.\
        filter(lambda p: p.udp.dstport == MDNS_PORT).\
        filter(lambda p: p.mdns.flags.response == 0).\
        filter(lambda p: MDNS_SERVICE_NAME in verify_utils.as_list(p.mdns.qry.name)).\
        must_next()

    # Step 21
    #   Device: BR 1 (DUT)
    #   Description (SRP-2.15): Automatically sends mDNS Response with service-test-1.
    #   Pass Criteria:
    #   - Repeat Step 9
    print("Step 21: BR 1 (DUT) sends mDNS Response (Repeat Step 9)")
    pkts.\
        filter(lambda p: p.udp.srcport == MDNS_PORT).\
        filter(lambda p: p.mdns.flags.response == 1).\
        filter(lambda p: p.mdns.flags.rcode == 0).\
        filter(lambda p: MDNS_SERVICE_NAME in verify_utils.as_list(p.mdns.resp.name)).\
        filter(lambda p: MDNS_INSTANCE_NAME in verify_utils.as_list(p.mdns.ptr.domain_name)).\
        filter(lambda p: MDNS_INSTANCE_NAME in verify_utils.as_list(p.mdns.resp.name)).\
        filter(lambda p: SRP_SERVICE_PORT in verify_utils.as_list(p.mdns.srv.port)).\
        must_next()

    # Step 22
    #   Device: Eth 1
    #   Description (SRP-2.15): Harness instructs the device to send mDNS query QType PTR for non-existent subtype (name
    #     with extra character added): "_test-subtypee._sub._test-type._tcp.local." service type, to check that no
    #     service answer is given by the DUT.
    #   Pass Criteria:
    #   - N/A
    print("Step 22: Eth 1 sends mDNS query QType PTR for a non-existent subtype")
    pkts.\
        filter(lambda p: p.udp.dstport == MDNS_PORT).\
        filter(lambda p: p.mdns.flags.response == 0).\
        filter(lambda p: MDNS_SUBTYPE_INVALID_NAME in verify_utils.as_list(p.mdns.qry.name)).\
        must_next()

    # Step 23
    #   Device: BR_1 (DUT)
    #   Description (SRP-2.15): Does not send an mDNS Response.
    #   Pass Criteria:
    #   - The DUT MUST NOT send an mDNS response.
    print("Step 23: BR 1 (DUT) does not send an mDNS Response")
    pkts.\
        filter(lambda p: p.udp.srcport == MDNS_PORT).\
        filter(lambda p: p.mdns.flags.response == 1).\
        filter(lambda p: MDNS_SUBTYPE_INVALID_NAME in verify_utils.as_list(p.mdns.resp.name)).\
        must_not_next()

    # Step 24
    #   Device: ED 1
    #   Description (SRP-2.15): Harness instructs the device to send SRP Update to the DUT to re-register (i.e.,
    #     update), its service with the subtype "_test-subtype" removed from the service: $ORIGIN
    #     default.service.arpa. _other-subtype._sub._test-type._tcp ( PTR service-test-1._test-type._tcp)
    #     _test-type._tcp ( PTR service-test-1._test-type._tcp) service-test-1._test-type._tcp ( SRV 55555 host-test-1)
    #     host-test-1 AAAA OMR address of ED_1> Note: such an update would automatically delete any other,
    #     pre-existing subtypes on the SRP server for this service.
    #   Pass Criteria:
    #   - N/A
    print("Step 24: ED 1 removes a subtype from the service")
    pkts.\
        filter_ipv6_dst(BR_1_MLEID).\
        filter_ipv6_src(ED_1_MLEID).\
        filter(lambda p: p.udp.dstport == br1_srp_port).\
        filter(lambda p: p.dns.flags.opcode == 5).\
        filter(lambda p: SRP_SUBTYPE2_NAME in verify_utils.as_list(p.dns.resp.name)).\
        filter(lambda p: SRP_SERVICE_NAME in verify_utils.as_list(p.dns.resp.name)).\
        filter(lambda p: SRP_INSTANCE_NAME in verify_utils.as_list(p.dns.resp.name)).\
        must_next()

    # Step 25
    #   Device: BR 1 (DUT)
    #   Description (SRP-2.15): Automatically sends SRP Update Response.
    #   Pass Criteria:
    #   - The DUT MUST send a valid SRP Update Response with RCODE=0 (NoError).
    print("Step 25: BR 1 (DUT) sends SRP Update Response")
    pkts.\
        filter_ipv6_dst(ED_1_MLEID).\
        filter_ipv6_src(BR_1_MLEID).\
        filter(lambda p: p.udp.srcport == br1_srp_port).\
        filter(lambda p: p.dns.flags.response == 1).\
        filter(lambda p: p.dns.flags.rcode == 0).\
        must_next()

    # Step 26
    #   Device: Eth 1
    #   Description (SRP-2.15): Harness instructs the device to send mDNS query QType PTR for
    #     _other-subtype._sub._test-type._tcp.local" service type, to check that this subtype is still present.
    #   Pass Criteria:
    #   - N/A
    print("Step 26: Eth 1 sends mDNS query QType PTR for the remaining subtype")
    pkts.\
        filter(lambda p: p.udp.dstport == MDNS_PORT).\
        filter(lambda p: p.mdns.flags.response == 0).\
        filter(lambda p: MDNS_SUBTYPE2_NAME in verify_utils.as_list(p.mdns.qry.name)).\
        must_next()

    # Step 27
    #   Device: BR 1 (DUT)
    #   Description (SRP-2.15): Automatically sends mDNS Response with service-test-1.
    #   Pass Criteria:
    #   - Repeat Step 17
    print("Step 27: BR 1 (DUT) sends mDNS Response (Repeat Step 17)")
    pkts.\
        filter(lambda p: p.udp.srcport == MDNS_PORT).\
        filter(lambda p: p.mdns.flags.response == 1).\
        filter(lambda p: p.mdns.flags.rcode == 0).\
        filter(lambda p: MDNS_SUBTYPE2_NAME in verify_utils.as_list(p.mdns.resp.name)).\
        filter(lambda p: MDNS_INSTANCE_NAME in verify_utils.as_list(p.mdns.ptr.domain_name)).\
        filter(lambda p: MDNS_INSTANCE_NAME in verify_utils.as_list(p.mdns.resp.name)).\
        filter(lambda p: SRP_SERVICE_PORT in verify_utils.as_list(p.mdns.srv.port)).\
        must_next()

    # Step 28
    #   Device: Eth 1
    #   Description (SRP-2.15): Harness instructs the device to send mDNS QType=PTR query for
    #     _test-subtype._sub._test-type._tcp.local." service type to check that the removed subtype is no longer present.
    #   Pass Criteria:
    #   - N/A
    print("Step 28: Eth 1 sends mDNS query QType PTR for the removed subtype")
    pkts.\
        filter(lambda p: p.udp.dstport == MDNS_PORT).\
        filter(lambda p: p.mdns.flags.response == 0).\
        filter(lambda p: MDNS_SUBTYPE1_NAME in verify_utils.as_list(p.mdns.qry.name)).\
        must_next()

    # Step 29
    #   Device: BR 1 (DUT)
    #   Description (SRP-2.15): Does not send an mDNS Response.
    #   Pass Criteria:
    #   - The DUT MUST NOT send an mDNS response.
    print("Step 29: BR 1 (DUT) does not send an mDNS Response")
    pkts.\
        filter(lambda p: p.udp.srcport == MDNS_PORT).\
        filter(lambda p: p.mdns.flags.response == 1).\
        filter(lambda p: MDNS_SUBTYPE1_NAME in verify_utils.as_list(p.mdns.resp.name)).\
        must_not_next()

    # Step 30
    #   Device: ED 1
    #   Description (SRP-2.15): Harness instructs the device to send SRP Update to the DUT which isn't a valid SRP
    #     Update. This invalid update attempts to add a subtype to the service but is missing the Service Description
    #     Instruction (SRV record): $ORIGIN default.service.arpa. _test-subtype-2._sub._test-type._tcp ( PTR
    #     service-test-1._test-type._tcp) _test-type._tcp ( PTR service-test-1._test-type._tcp) host-test-1 AAAA OMR
    #     address of ED_1> Note (TBD): sending an invalid SRP Update of this form is not supported by current reference
    #     devices. As a future extension, the Harness should construct the required DNS Update message and send it
    #     using the UDP-send API.
    #   Pass Criteria:
    #   - N/A
    print("Step 30: ED 1 sends an invalid SRP Update (skipped)")

    # Step 31
    #   Device: BR 1 (DUT)
    #   Description (SRP-2.15): Rejects the invalid SRP Update.
    #   Pass Criteria:
    #   - N/A (skipping for 1.3.0 cert due to API issue - TBD)
    #   - (Future requirement TBD: MUST send SRP Update Response with RCODE=5, REFUSED.)
    print("Step 31: BR 1 (DUT) rejects the invalid SRP Update (skipped)")


if __name__ == '__main__':
    verify_utils.run_main(verify)
