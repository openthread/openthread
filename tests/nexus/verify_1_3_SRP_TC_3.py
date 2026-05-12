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
from pktverify.addrs import Ipv6Addr, EthAddr
from pktverify.bytes import Bytes


def verify(pv):
    #
    # 2.3. [1.3] [CERT] Service Instance Lease - Renewal and Automatic Service/Host Removal
    #
    # 2.3.1. Purpose
    # To test the following:
    # - Handle lease renewal before lease expiration.
    # - Handle service/host lease expiration and removal of the service/host.
    #
    # 2.3.2. Topology
    # - 1. BR_1 (DUT)-Thread Border Router and the Leader
    # - 2. ED_1-Test Bed device operating as a Thread End Device, attached to BR_1
    # - 3. Eth 1-Test Bed border router device on an Adjacent Infrastructure Link
    #
    # Spec Reference           | V1.3.0 Section
    # -------------------------|---------------
    # DNS-SD Unicast Dataset   | 14.3.2.2

    pkts = pv.pkts
    pv.summary.show()

    ED_1_MLEID = Ipv6Addr(pv.vars['ED_1_MLEID_ADDR'])
    ED_1_OMR = Ipv6Addr(pv.vars['ED_1_OMR_ADDR'])
    BR_1_MLEID = Ipv6Addr(pv.vars['BR_1_MLEID_ADDR'])
    br1_srp_port = int(pv.vars['BR_1_SRP_PORT'])
    BR_1_ETH = EthAddr(pv.vars['BR_1_ETH'])

    SRP_SERVICE_NAME = '_thread-test._udp.default.service.arpa'
    SRP_INSTANCE_NAME = 'service-test-1.' + SRP_SERVICE_NAME
    SRP_HOST_NAME = 'host-test-1.default.service.arpa'
    MDNS_SERVICE_NAME = '_thread-test._udp.local'
    MDNS_INSTANCE_NAME = 'service-test-1.' + MDNS_SERVICE_NAME
    MDNS_HOST_NAME = 'host-test-1.local'

    SRP_SERVICE_PORT = 55555

    SERVICE_ID_SRP_SERVER = 93
    SRP_LEASE = 30
    SRP_KEY_LEASE = 600
    SRP_LEASE_DATA = Bytes(struct.pack('>II', SRP_LEASE, SRP_KEY_LEASE).hex())

    def verify_srp_update(pkts, dst, src, srp_port, instance_name, service_port, host_name, omr_addr, lease_data):
        return pkts.\
            filter_ipv6_dst(dst).\
            filter_ipv6_src(src).\
            filter(lambda p: p.udp.dstport == srp_port).\
            filter(lambda p: p.dns.flags.opcode == 5).\
            filter(lambda p: instance_name in verify_utils.as_list(p.dns.resp.name)).\
            filter(lambda p: service_port is None or service_port in verify_utils.as_list(p.dns.srv.port)).\
            filter(lambda p: host_name is None or host_name in verify_utils.as_list(p.dns.srv.target)).\
            filter(lambda p: omr_addr in verify_utils.as_list(p.dns.aaaa)).\
            filter(lambda p: lease_data in verify_utils.as_list(p.dns.opt.data)).\
            must_next()

    # Step 1
    # - Device: BR 1 (DUT)
    # - Description (SRP-2.3): Enable: switch on.
    # - Pass Criteria:
    #   - N/A
    print("Step 1: BR 1 (DUT) switch on.")

    # Step 2
    # - Device: Eth 1, ED 1
    # - Description (SRP-2.3): Enable
    # - Pass Criteria:
    #   - N/A
    print("Step 2: Eth 1, ED 1 Enable.")

    # Step 3
    # - Device: BR 1 (DUT)
    # - Description (SRP-2.3): Automatically adds SRP Server information in the Thread Network Data. Automatically
    #   provides an OMR prefix to the Thread Network.
    # - Pass Criteria:
    #   - The DUT's SRP Server information MUST appear in the Thread Network Data. This can be verified as specified
    #     in test case 2.1 step 3.
    #   - Specifically, it MUST include a DNS-SD Unicast Dataset as per [ThreadSpec] 14.3.2.2.
    #   - S_service_data Length MUST be 1 or 19
    print("Step 3: BR 1 (DUT) adds SRP Server information in the Thread Network Data.")
    pkts.\
        filter_mle_cmd(consts.MLE_DATA_RESPONSE).\
        filter(lambda p: SERVICE_ID_SRP_SERVER in verify_utils.as_list(p.thread_nwd.tlv.service.srp_dataset_identifier)).\
        filter(lambda p: any(l in (1, 19) for l in verify_utils.as_list(p.thread_nwd.tlv.service.s_data_len))).\
        must_next()

    # Step 4
    # - Device: ED 1
    # - Description (SRP-2.3): Harness instructs the device to send SRP Update to DUT: $ORIGIN default.service.arpa.
    #   service-test-1._thread-test._udp ( SRV 0 0 55555 host-test-1 ) host-test-1 AAAA <OMR address of ED_1> with the
    #   following options: Update Lease Option, Lease: 30 seconds, Key Lease: 10 minutes. Harness
    #   instructs/configures device to avoid automatic renewal or resending of this SRP Update.
    # - Pass Criteria:
    #   - N/A
    print("Step 4: ED 1 sends SRP Update to DUT.")
    verify_srp_update(pkts, BR_1_MLEID, ED_1_MLEID, br1_srp_port, SRP_INSTANCE_NAME, SRP_SERVICE_PORT, SRP_HOST_NAME,
                      ED_1_OMR, SRP_LEASE_DATA)

    # Step 5
    # - Device: BR 1 (DUT)
    # - Description (SRP-2.3): Automatically sends SRP Update Response.
    # - Pass Criteria:
    #   - The DUT MUST send a valid SRP Update Response, with RCODE=0 (NoError).
    print("Step 5: BR 1 (DUT) sends SRP Update Response.")
    pkts.\
        filter_ipv6_dst(ED_1_MLEID).\
        filter_ipv6_src(BR_1_MLEID).\
        filter(lambda p: p.udp.srcport == br1_srp_port).\
        filter(lambda p: p.dns.flags.response == 1).\
        filter(lambda p: p.dns.flags.rcode == 0).\
        must_next()

    # Step 6
    # - Device: ED 1
    # - Description (SRP-2.3): Harness instructs the device to unicast DNS query QType SRV to the DUT for service
    #   "service-test-1._thread-test._udp.default.service.arpa"
    # - Pass Criteria:
    #   - N/A
    print("Step 6: ED 1 sends unicast DNS query QType SRV.")
    pkts.\
        filter_ipv6_dst(BR_1_MLEID).\
        filter_ipv6_src(ED_1_MLEID).\
        filter(lambda p: p.udp.dstport == 53).\
        filter(lambda p: p.dns.flags.response == 0).\
        filter(lambda p: SRP_INSTANCE_NAME in verify_utils.as_list(p.dns.qry.name)).\
        must_next()

    # Step 7
    # - Device: BR 1 (DUT)
    # - Description (SRP-2.3): Automatically sends DNS response.
    # - Pass Criteria:
    #   - The DUT MUST send a valid DNS Response and contain in Answers section: $ORIGIN default.service.arpa.
    #     service-test-1._thread-test._udp ( SRV 0 0 55555 host-test-1 )
    #   - The DNS Response MAY contain in Additional records section: $ORIGIN default.service.arpa. host-test-1 AAAA
    #     <OMR address of ED_1>
    print("Step 7: BR 1 (DUT) sends DNS response.")
    pkts.\
        filter_ipv6_dst(ED_1_MLEID).\
        filter_ipv6_src(BR_1_MLEID).\
        filter(lambda p: p.udp.srcport == 53).\
        filter(lambda p: p.dns.flags.response == 1).\
        filter(lambda p: p.dns.flags.rcode == 0).\
        filter(lambda p: SRP_INSTANCE_NAME in verify_utils.as_list(p.dns.resp.name)).\
        filter(lambda p: SRP_SERVICE_PORT in verify_utils.as_list(p.dns.srv.port)).\
        filter(lambda p: SRP_HOST_NAME in verify_utils.as_list(p.dns.srv.target)).\
        must_next()

    # Step 8
    # - Device: Eth_1
    # - Description (SRP-2.3): Harness instructs the device to send mDNS query QType SRV for
    #   "service-test-1._thread-test._udp.local."
    # - Pass Criteria:
    #   - N/A
    print("Step 8: Eth 1 sends mDNS query QType SRV.")
    pkts.\
        filter_ipv6_dst('ff02::fb').\
        filter(lambda p: p.udp.dstport == 5353).\
        filter(lambda p: p.mdns.flags.response == 0).\
        filter(lambda p: MDNS_INSTANCE_NAME in verify_utils.as_list(p.mdns.qry.name)).\
        must_next()

    # Step 9
    # - Device: BR 1 (DUT)
    # - Description (SRP-2.3): Automatically sends mDNS Response.
    # - Pass Criteria:
    #   - The DUT MUST send a valid mDNS Response and contain in Answers section: $ORIGIN local.
    #     Service-test-1._thread-test._udp ( SRV 0 0 55555 host-test-1 )
    #   - The DNS Response MAY contain in Additional records section: $ORIGIN local. Host-test-1 AAAA <OMR address of
    #     ED_1>
    print("Step 9: BR 1 (DUT) sends mDNS Response.")
    pkts.\
        filter(lambda p: p.eth.src == BR_1_ETH).\
        filter(lambda p: p.udp.dstport == 5353).\
        filter(lambda p: p.mdns.flags.response == 1).\
        filter(lambda p: MDNS_INSTANCE_NAME in verify_utils.as_list(p.mdns.resp.name)).\
        filter(lambda p: SRP_SERVICE_PORT in verify_utils.as_list(p.mdns.srv.port)).\
        filter(lambda p: MDNS_HOST_NAME in verify_utils.as_list(p.mdns.srv.target)).\
        must_next()

    # Step 10
    # - Device: N/A
    # - Description (SRP-2.3): Harness waits for 15 seconds.
    # - Pass Criteria:
    #   - N/A
    print("Step 10: Harness waits for 15 seconds.")

    # Step 11
    # - Device: ED 1
    # - Description (SRP-2.3): Harness instructs the device to send SRP Update to the DUT to renew the service lease:
    #   $ORIGIN default.service.arpa. service-test-1._thread-test._udp ( SRV 0 0 55555 host-test-1 ) host-test-1 AAAA
    #   <OMR address of ED_1> with the following options: Update Lease Option, Lease: 30 seconds, Key Lease: 10
    #   minutes. Harness instructs/configures the device to avoid automatic renewal or resending of this SRP Update.
    # - Pass Criteria:
    #   - N/A
    print("Step 11: ED 1 sends SRP Update to renew the service lease.")
    verify_srp_update(pkts, BR_1_MLEID, ED_1_MLEID, br1_srp_port, SRP_INSTANCE_NAME, None, None, ED_1_OMR,
                      SRP_LEASE_DATA)

    # Step 12
    # - Device: BR 1 (DUT)
    # - Description (SRP-2.3): Automatically sends SRP Update Response.
    # - Pass Criteria:
    #   - The DUT MUST send a valid SRP Update Response, with RCODE=0 (NoError).
    print("Step 12: BR 1 (DUT) sends SRP Update Response.")
    pkts.\
        filter_ipv6_dst(ED_1_MLEID).\
        filter_ipv6_src(BR_1_MLEID).\
        filter(lambda p: p.udp.srcport == br1_srp_port).\
        filter(lambda p: p.dns.flags.response == 1).\
        filter(lambda p: p.dns.flags.rcode == 0).\
        must_next()

    # Step 13
    # - Device: N/A
    # - Description (SRP-2.3): Harness waits for 16 seconds.
    # - Pass Criteria:
    #   - N/A
    print("Step 13: Harness waits for 16 seconds.")

    # Step 14
    # - Device: ED 1
    # - Description (SRP-2.3): Harness instructs the device to unicast DNS query QType SRV to the DUT for service
    #   "service-test-1._thread-test._udp.default.service.arpa"
    # - Pass Criteria:
    #   - N/A
    print("Step 14: ED 1 sends unicast DNS query QType SRV.")
    pkts.\
        filter_ipv6_dst(BR_1_MLEID).\
        filter_ipv6_src(ED_1_MLEID).\
        filter(lambda p: p.udp.dstport == 53).\
        filter(lambda p: p.dns.flags.response == 0).\
        filter(lambda p: SRP_INSTANCE_NAME in verify_utils.as_list(p.dns.qry.name)).\
        must_next()

    # Step 15
    # - Device: BR 1 (DUT)
    # - Description (SRP-2.3): Automatically sends DNS response.
    # - Pass Criteria:
    #   - The DUT MUST send a valid DNS Response and contain answer record: $ORIGIN default.service.arpa.
    #     service-test-1._thread-test._udp ( SRV 0 0 55555 host-test-1 )
    #   - The DNS Response MAY contain in Additional records section: $ORIGIN default.service.arpa. host-test-1 AAAA
    #     <OMR address of ED_1>
    print("Step 15: BR 1 (DUT) sends DNS response.")
    pkts.\
        filter_ipv6_dst(ED_1_MLEID).\
        filter_ipv6_src(BR_1_MLEID).\
        filter(lambda p: p.udp.srcport == 53).\
        filter(lambda p: p.dns.flags.response == 1).\
        filter(lambda p: p.dns.flags.rcode == 0).\
        filter(lambda p: SRP_INSTANCE_NAME in verify_utils.as_list(p.dns.resp.name)).\
        filter(lambda p: SRP_SERVICE_PORT in verify_utils.as_list(p.dns.srv.port)).\
        filter(lambda p: SRP_HOST_NAME in verify_utils.as_list(p.dns.srv.target)).\
        must_next()

    # Step 16
    # - Device: Eth 1
    # - Description (SRP-2.3): Harness instructs the device to send mDNS query QType PTR for any services of type
    #   "_thread-test._udp.local."
    # - Pass Criteria:
    #   - N/A
    print("Step 16: Eth 1 sends mDNS query QType PTR.")
    pkts.\
        filter_ipv6_dst('ff02::fb').\
        filter(lambda p: p.udp.dstport == 5353).\
        filter(lambda p: p.mdns.flags.response == 0).\
        filter(lambda p: MDNS_SERVICE_NAME in verify_utils.as_list(p.mdns.qry.name)).\
        must_next()

    # Step 17
    # - Device: BR 1 (DUT)
    # - Description (SRP-2.3): Automatically sends mDNS Response.
    # - Pass Criteria:
    #   - The DUT MUST send a valid mDNS Response and contain answer record: $ORIGIN local. _thread-test._udp ( PTR
    #     service-test-1._thread-test._udp )
    #   - THe DNS Response MAY contain in Additional records section: $ORIGIN local. Service-test-1_thread-test._udp (
    #     SRV 0 0 55555 host-test-1 ) host-test-1 AAAA <OMR address of ED_1>
    print("Step 17: BR 1 (DUT) sends mDNS Response.")
    pkts.\
        filter(lambda p: p.eth.src == BR_1_ETH).\
        filter(lambda p: p.udp.dstport == 5353).\
        filter(lambda p: p.mdns.flags.response == 1).\
        filter(lambda p: MDNS_INSTANCE_NAME in verify_utils.as_list(p.mdns.ptr.domain_name)).\
        must_next()

    # Step 18
    # - Device: N/A
    # - Description (SRP-2.3): Harness waits 15 seconds for the service lease to expire. Note: Both service and host
    #   entries should expire now, after >= 31 seconds after last registration update. But the KEY record registration
    #   remains active for both names.
    # - Pass Criteria:
    #   - N/A
    print("Step 18: Harness waits 15 seconds for the service lease to expire.")

    # Step 19
    # - Device: ED 1
    # - Description (SRP-2.3): Harness instructs the device to unicast DNS query QType SRV to the DUT for service
    #   instance name "service-test-1._thread-test._udp.default.service.arpa"
    # - Pass Criteria:
    #   - N/A
    print("Step 19: ED 1 sends unicast DNS query QType SRV.")
    pkts.\
        filter_ipv6_dst(BR_1_MLEID).\
        filter_ipv6_src(ED_1_MLEID).\
        filter(lambda p: p.udp.dstport == 53).\
        filter(lambda p: p.dns.flags.response == 0).\
        filter(lambda p: SRP_INSTANCE_NAME in verify_utils.as_list(p.dns.qry.name)).\
        must_next()

    # Step 20
    # - Device: BR 1 (DUT)
    # - Description (SRP-2.3): Automatically sends DNS response but no matching SRV records. Note: only the key lease
    #   is still active on the service instance name. The DUT's response may vary as shown on the right, depending on
    #   implementation.
    # - Pass Criteria:
    #   - The DUT MUST send a valid DNS Response and MUST be one of the below:
    #   - 1. RCODE=0 (NoError) with 0 answer records.
    #   - 2. RCODE=3 (NxDomain) with 0 answer records.
    print("Step 20: BR 1 (DUT) sends DNS response (no SRV).")
    pkts.\
        filter_ipv6_dst(ED_1_MLEID).\
        filter_ipv6_src(BR_1_MLEID).\
        filter(lambda p: p.udp.srcport == 53).\
        filter(lambda p: p.dns.flags.response == 1).\
        filter(lambda p: p.dns.flags.rcode in (0, 3)).\
        filter(lambda p: p.dns.count.answers == 0).\
        must_next()

    # Step 21
    # - Device: ED 1
    # - Description (SRP-2.3): (Reserved for TBD future unicast DNS query QTYPE=ANY to the DUT for records for name
    #   "host-test-1.default.service.arpa".)
    # - Pass Criteria:
    #   - N/A
    print("Step 21: ED 1 reserved step.")

    # Step 22
    # - Device: BR 1 (DUT)
    # - Description (SRP-2.3): (Reserved for TBD future: Automatically sends DNS response.) Note: because the key
    #   lease is still active, the KEY record is optionally returned in response to the "ANY" query.
    # - Pass Criteria:
    #   - N/A
    print("Step 22: BR 1 (DUT) reserved step.")

    # Step 23
    # - Device: Eth 1
    # - Description (SRP-2.3): Harness instructs device to send mDNS query QType=SRV for
    #   "service-test-1._thread-test_udp.local."
    # - Pass Criteria:
    #   - N/A
    print("Step 23: Eth 1 sends mDNS query QType SRV.")
    pkts.\
        filter_ipv6_dst('ff02::fb').\
        filter(lambda p: p.udp.dstport == 5353).\
        filter(lambda p: p.mdns.flags.response == 0).\
        filter(lambda p: MDNS_INSTANCE_NAME in verify_utils.as_list(p.mdns.qry.name)).\
        must_next()

    def verify_optional_mdns_no_answer_response():
        with pkts.save_index():
            responses = pkts.\
                filter(lambda p: p.eth.src == BR_1_ETH).\
                filter(lambda p: p.udp.dstport == 5353).\
                filter(lambda p: p.mdns.flags.response == 1)
            p = responses.next()
            if p:
                p.must_verify(lambda p: p.mdns.flags.rcode == 0 and
                              (p.mdns.count.answers == 0 or hasattr(p.mdns, 'nsec')))

    # Step 24
    # - Device: BR 1 (DUT)
    # - Description (SRP-2.3): Does not respond to mDNS query, or automatically responds NSEC, or no-error-no-answer.
    # - Pass Criteria:
    #   - The DUT MUST perform one of below options:
    #   - 1. Sends RCODE=0 (NoError) and NSEC Answer record.
    #   - 2. MUST NOT send any mDNS response.
    #   - 3. Sends RCODE=0 (NoError) and 0 Answer records.
    print("Step 24: BR 1 (DUT) mDNS verification for Step 23.")
    verify_optional_mdns_no_answer_response()

    # Step 25
    # - Device: Eth_1
    # - Description (SRP-2.3): Harness instructs the device to send mDNS query QType=PTR for any services of type
    #   "_thread-test._udp.local."
    # - Pass Criteria:
    #   - N/A
    print("Step 25: Eth 1 sends mDNS query QType PTR.")
    pkts.\
        filter_ipv6_dst('ff02::fb').\
        filter(lambda p: p.udp.dstport == 5353).\
        filter(lambda p: p.mdns.flags.response == 0).\
        filter(lambda p: MDNS_SERVICE_NAME in verify_utils.as_list(p.mdns.qry.name)).\
        must_next()

    # Step 26
    # - Device: BR 1 (DUT)
    # - Description (SRP-2.3): Does not respond to mDNS query, or automatically responds NSEC, or no-error-no-answer.
    # - Pass Criteria:
    #   - The DUT MUST perform one of below options:
    #   - 1. Sends RCODE=0 (NoError) and NSEC Answer record.
    #   - 2. MUST NOT send any mDNS response.
    #   - 3. Sends RCODE=0 (NoError) and 0 Answer records.
    print("Step 26: BR 1 (DUT) mDNS verification for Step 25.")
    verify_optional_mdns_no_answer_response()

    # Step 27
    # - Device: Eth 1
    # - Description (SRP-2.3): Harness instructs device to send mDNS query for Qtype AAAA for name "host-test-1.local."
    # - Pass Criteria:
    #   - N/A
    print("Step 27: Eth 1 sends mDNS query for QType AAAA.")
    pkts.\
        filter_ipv6_dst('ff02::fb').\
        filter(lambda p: p.udp.dstport == 5353).\
        filter(lambda p: p.mdns.flags.response == 0).\
        filter(lambda p: MDNS_HOST_NAME in verify_utils.as_list(p.mdns.qry.name)).\
        must_next()

    # Step 28
    # - Device: BR 1 (DUT)
    # - Description (SRP-2.3): Does not respond to mDNS query, or automatically responds NSEC, or no-error-no-answer.
    #   Note: this indicates the DUT is authoritative for the host name, but has no AAAA records to answer for the
    #   name.
    # - Pass Criteria:
    #   - The DUT MUST perform one of below options:
    #   - 1. Sends RCODE=0 (NoError) and NSEC Answer record.
    #   - 2. MUST NOT send any mDNS response.
    #   - 3. Sends RCODE=0 (NoError) and 0 Answer records.
    print("Step 28: BR 1 (DUT) mDNS verification for Step 27.")
    verify_optional_mdns_no_answer_response()


if __name__ == '__main__':
    verify_utils.run_main(verify)
