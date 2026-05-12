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


def verify(pv):
    # 2.11. [1.3] [CERT] Recovery after reboot
    #
    # 2.11.1. Purpose
    # To test the following:
    # - 1. Handle SRP update after reboot of Thread device.
    # - 2. Handle SRP update after reboot of Border Router
    # - 3. Respond to mDNS queries on infrastructure interface after reboot of infrastructure device.
    #
    # 2.11.2. Topology
    # - 1. BR_1 (DUT) - Thread Border Router, and the Leader
    # - 2. ED 1-Test Bed device operating as a Thread End Device, attached to BR_1
    # - 3. Eth 1-Test Bed border router device on an Adjacent Infrastructure Link
    #
    # Spec Reference | V1.1 Section | V1.3.0 Section
    # ---------------|--------------|---------------
    # SRP Server     | N/A          | 1.3

    pkts = pv.pkts
    pv.summary.show()

    BR_1_MLEID = pv.vars['BR_1_MLEID']
    ED_1_MLEID = pv.vars['ED_1_MLEID']
    BR_1_SRP_PORT_1 = int(pv.vars['BR_1_SRP_PORT_1'])
    BR_1_SRP_PORT_2 = int(pv.vars['BR_1_SRP_PORT_2'])
    BR_1_ETH = pv.vars['BR_1_ETH']
    ETH_1_ETH = pv.vars['Eth_1_ETH']

    SRP_SERVICE_NAME = '_thread-test._udp.default.service.arpa'
    SRP_INSTANCE_NAME = 'service-test-1.' + SRP_SERVICE_NAME
    SRP_HOST_NAME = 'host-test-1.default.service.arpa'
    MDNS_SERVICE_NAME = '_thread-test._udp.local'
    MDNS_INSTANCE_NAME = 'service-test-1.' + MDNS_SERVICE_NAME

    SRP_SERVICE_PORT = 1500
    SRP_SERVICE_PRIORITY = 8
    SRP_SERVICE_WEIGHT = 8

    DNS_TYPE_SRV = 33
    SERVICE_ID_SRP_UNICAST_DNS = 93

    # Step 1
    # - Device: BR 1 (DUT)
    # - Description (SRP-2.11): Enable
    # - Pass Criteria:
    #   - N/A
    print("Step 1: BR 1 (DUT) Enable")

    # Step 2
    # - Device: Eth 1, ED 1
    # - Description (SRP-2.11): Enable
    # - Pass Criteria:
    #   - N/A
    print("Step 2: Eth 1, ED 1 Enable")

    # Step 3
    # - Device: BR 1 (DUT)
    # - Description (SRP-2.11): Automatically adds SRP Server information in the Thread Network Data.
    # - Pass Criteria:
    #   - The DUT's SRP Server information MUST appear in the Thread Network Data. This MUST be a
    #     DNS/SRP Unicast Dataset, or a DNS/SRP Anycast Dataset, or both.
    #   - In case of Anycast Dataset: record the SRP-SN value for later use.
    print("Step 3: BR 1 (DUT) Automatically adds SRP Server information in the Thread Network Data.")
    pkts.\
      filter_mle_cmd(consts.MLE_DATA_RESPONSE).\
      filter(lambda p: SERVICE_ID_SRP_UNICAST_DNS in verify_utils.as_list(p.thread_nwd.tlv.service.srp_dataset_identifier)).\
      must_next()

    # Step 4
    # - Device: ED_1
    # - Description (SRP-2.11): Harness instructs the device to send SRP Update: $ORIGIN
    #   default.service.arpa. service-test-1._thread-test._udp ( SRV 8 1500 host-test-1 ) host-test-1
    #   AAAA <OMR address of ED_1> KEY <public key of ED_1> with the following options: Update Lease
    #   Option, Lease: 5 minutes, Key Lease: 10 minutes
    # - Pass Criteria:
    #   - N/A
    print("Step 4: ED_1 sends SRP Update")
    pkts.\
      filter_ipv6_dst(BR_1_MLEID).\
      filter_ipv6_src(ED_1_MLEID).\
      filter(lambda p: p.udp.dstport == BR_1_SRP_PORT_1).\
      filter(lambda p: p.dns.flags.opcode == consts.DNS_OPCODE_UPDATE).\
      filter(lambda p: SRP_INSTANCE_NAME in verify_utils.as_list(p.dns.resp.name)).\
      filter(lambda p: SRP_SERVICE_PORT in verify_utils.as_list(p.dns.srv.port)).\
      filter(lambda p: SRP_SERVICE_PRIORITY in verify_utils.as_list(p.dns.srv.priority)).\
      filter(lambda p: SRP_SERVICE_WEIGHT in verify_utils.as_list(p.dns.srv.weight)).\
      filter(lambda p: SRP_HOST_NAME in verify_utils.as_list(p.dns.srv.target)).\
      must_next()

    # Step 5
    # - Device: BR 1 (DUT)
    # - Description (SRP-2.11): Automatically sends SRP Update Response.
    # - Pass Criteria:
    #   - The DUT MUST send a valid SRP Update Response, with RCODE=0 (NoError).
    print("Step 5: BR 1 (DUT) Automatically sends SRP Update Response.")
    pkts.\
      filter_ipv6_dst(ED_1_MLEID).\
      filter_ipv6_src(BR_1_MLEID).\
      filter(lambda p: p.udp.srcport == BR_1_SRP_PORT_1).\
      filter(lambda p: p.dns.flags.response == 1).\
      filter(lambda p: p.dns.flags.rcode == 0).\
      must_next()

    # Step 6
    # - Device: Eth_1
    # - Description (SRP-2.11): Harness instructs the device to send mDNS query QType SRV for
    #   "service-test-1. thread-test, udp.local.".
    # - Pass Criteria:
    #   - N/A
    print("Step 6: Eth_1 sends mDNS query QType SRV")
    pkts.\
      filter(lambda p: p.eth.src == ETH_1_ETH).\
      filter(lambda p: p.udp.dstport == 5353).\
      filter(lambda p: p.mdns.flags.response == 0).\
      filter(lambda p: MDNS_INSTANCE_NAME in verify_utils.as_list(p.mdns.qry.name)).\
      filter(lambda p: DNS_TYPE_SRV in verify_utils.as_list(p.mdns.qry.type)).\
      must_next()

    # Step 7
    # - Device: BR 1 (DUT)
    # - Description (SRP-2.11): Automatically sends mDNS Response.
    # - Pass Criteria:
    #   - The DUT MUST send a valid mDNS Response, with RCODE=0 (NoError); and contains
    #     service-test-1 answer record: SORIGIN local. service-test-1. thread-test._udp ( SRV 8 8 1500
    #     host-test-1 )
    print("Step 7: BR 1 (DUT) Automatically sends mDNS Response.")
    pkts.\
      filter(lambda p: p.eth.src == BR_1_ETH).\
      filter(lambda p: p.udp.dstport == 5353).\
      filter(lambda p: p.mdns.flags.response == 1).\
      filter(lambda p: MDNS_INSTANCE_NAME in verify_utils.as_list(p.mdns.resp.name)).\
      filter(lambda p: SRP_SERVICE_PORT in verify_utils.as_list(p.mdns.srv.port)).\
      filter(lambda p: SRP_SERVICE_PRIORITY in verify_utils.as_list(p.mdns.srv.priority)).\
      filter(lambda p: SRP_SERVICE_WEIGHT in verify_utils.as_list(p.mdns.srv.weight)).\
      must_next()

    # Step 8
    # - Device: ED 1
    # - Description (SRP-2.11): Harness instructs the device to reboot/reset. Note: ED 1 MUST
    #   persist its public/private keypair that is used for signing SRP Updates. So it is not a
    #   factory-reset.
    # - Pass Criteria:
    #   - N/A
    print("Step 8: ED 1 reboot/reset")

    # Step 9
    # - Device: ED 1
    # - Description (SRP-2.11): Harness instructs the device to send SRP Update, as in step 4.
    # - Pass Criteria:
    #   - N/A
    print("Step 9: ED 1 sends SRP Update")
    pkts.\
      filter_ipv6_dst(BR_1_MLEID).\
      filter_ipv6_src(ED_1_MLEID).\
      filter(lambda p: p.udp.dstport == BR_1_SRP_PORT_1).\
      filter(lambda p: p.dns.flags.opcode == consts.DNS_OPCODE_UPDATE).\
      filter(lambda p: SRP_INSTANCE_NAME in verify_utils.as_list(p.dns.resp.name)).\
      filter(lambda p: SRP_SERVICE_PORT in verify_utils.as_list(p.dns.srv.port)).\
      filter(lambda p: SRP_SERVICE_PRIORITY in verify_utils.as_list(p.dns.srv.priority)).\
      filter(lambda p: SRP_SERVICE_WEIGHT in verify_utils.as_list(p.dns.srv.weight)).\
      filter(lambda p: SRP_HOST_NAME in verify_utils.as_list(p.dns.srv.target)).\
      must_next()

    # Step 10
    # - Device: BR 1 (DUT)
    # - Description (SRP-2.11): Automatically sends SRP Update Response.
    # - Pass Criteria:
    #   - The DUT MUST send a valid SRP Update Response, with RCODE=0 (NoError).
    print("Step 10: BR 1 (DUT) Automatically sends SRP Update Response.")
    pkts.\
      filter_ipv6_dst(ED_1_MLEID).\
      filter_ipv6_src(BR_1_MLEID).\
      filter(lambda p: p.udp.srcport == BR_1_SRP_PORT_1).\
      filter(lambda p: p.dns.flags.response == 1).\
      filter(lambda p: p.dns.flags.rcode == 0).\
      must_next()

    # Step 11
    # - Device: Eth_1
    # - Description (SRP-2.11): Harness instructs the device to send mDNS query QType=SRV for
    #   "service-test-1._thread-test_udp.local.".
    # - Pass Criteria:
    #   - N/A
    print("Step 11: Eth_1 sends mDNS query QType=SRV")
    pkts.\
      filter(lambda p: p.eth.src == ETH_1_ETH).\
      filter(lambda p: p.udp.dstport == 5353).\
      filter(lambda p: p.mdns.flags.response == 0).\
      filter(lambda p: MDNS_INSTANCE_NAME in verify_utils.as_list(p.mdns.qry.name)).\
      filter(lambda p: DNS_TYPE_SRV in verify_utils.as_list(p.mdns.qry.type)).\
      must_next()

    # Step 12
    # - Device: BR 1 (DUT)
    # - Description (SRP-2.11): Automatically sends mDNS Response.
    # - Pass Criteria:
    #   - The DUT MUST send a valid mDNS Response, with RCODE=0 (NoError); and contains
    #     service-test-1 answer record: SORIGIN local. service-test-1. thread-test._udp ( SRV 1500
    #     host-test-1 )
    print("Step 12: BR 1 (DUT) Automatically sends mDNS Response.")
    pkts.\
      filter(lambda p: p.eth.src == BR_1_ETH).\
      filter(lambda p: p.udp.dstport == 5353).\
      filter(lambda p: p.mdns.flags.response == 1).\
      filter(lambda p: MDNS_INSTANCE_NAME in verify_utils.as_list(p.mdns.resp.name)).\
      filter(lambda p: SRP_SERVICE_PORT in verify_utils.as_list(p.mdns.srv.port)).\
      filter(lambda p: SRP_SERVICE_PRIORITY in verify_utils.as_list(p.mdns.srv.priority)).\
      filter(lambda p: SRP_SERVICE_WEIGHT in verify_utils.as_list(p.mdns.srv.weight)).\
      must_next()

    # Step 13
    # - Device: BR 1 (DUT)
    # - Description (SRP-2.11): User is instructed to reboot the border router (DUT). Note: In case
    #   DUT uses an Anycast Dataset, this will include a different SRP-SN (compared to the previous
    #   SRP-SN recorded in step 3). This will then trigger re-registration in the next step. In
    #   case DUT uses a Unicast Dataset, this will include a different port number (compared to the
    #   previous port value used in step 3). This will then trigger re-registration.
    # - Pass Criteria:
    #   - After reboot, the DUT's SRP Server information MUST appear in the Thread Network Data.
    #     This MUST be a DNS/SRP Unicast Dataset, or DNS/SRP Anycast Dataset, or both.
    #   - For an Anycast Dataset, the SRP-SN SHOULD be different but MAY be equal (in case DUT
    #     persistently stores data, only).
    #   - For a Unicast Dataset, the port number MUST differ.
    print("Step 13: BR 1 (DUT) reboot")
    pkts.\
      filter_mle_cmd(consts.MLE_DATA_RESPONSE).\
      filter(lambda p: SERVICE_ID_SRP_UNICAST_DNS in verify_utils.as_list(p.thread_nwd.tlv.service.srp_dataset_identifier)).\
      must_next()

    # Verify that the SRP server port has changed after reboot
    print(f"Checking SRP port change: {BR_1_SRP_PORT_1} -> {BR_1_SRP_PORT_2}")
    assert BR_1_SRP_PORT_1 != BR_1_SRP_PORT_2, f"SRP port MUST differ after reboot (was {BR_1_SRP_PORT_1})"

    # Step 14
    # - Device: ED_1
    # - Description (SRP-2.11): Automatically re-registers using SRP Update, as in step 4.
    # - Pass Criteria:
    #   - MUST automatically re-register with SRP Update as in step 4.
    print("Step 14: ED_1 Automatically re-registers using SRP Update")
    pkts.\
      filter_ipv6_dst(BR_1_MLEID).\
      filter_ipv6_src(ED_1_MLEID).\
      filter(lambda p: p.udp.dstport == BR_1_SRP_PORT_2).\
      filter(lambda p: p.dns.flags.opcode == consts.DNS_OPCODE_UPDATE).\
      filter(lambda p: SRP_INSTANCE_NAME in verify_utils.as_list(p.dns.resp.name)).\
      filter(lambda p: SRP_SERVICE_PORT in verify_utils.as_list(p.dns.srv.port)).\
      filter(lambda p: SRP_SERVICE_PRIORITY in verify_utils.as_list(p.dns.srv.priority)).\
      filter(lambda p: SRP_SERVICE_WEIGHT in verify_utils.as_list(p.dns.srv.weight)).\
      filter(lambda p: SRP_HOST_NAME in verify_utils.as_list(p.dns.srv.target)).\
      must_next()

    # Step 15
    # - Device: BR 1 (DUT)
    # - Description (SRP-2.11): Automatically sends SRP Update Response.
    # - Pass Criteria:
    #   - The DUT MUST send a valid SRP Update Response, with RCODE=0 (NoError).
    print("Step 15: BR 1 (DUT) Automatically sends SRP Update Response.")
    pkts.\
      filter_ipv6_dst(ED_1_MLEID).\
      filter_ipv6_src(BR_1_MLEID).\
      filter(lambda p: p.udp.srcport == BR_1_SRP_PORT_2).\
      filter(lambda p: p.dns.flags.response == 1).\
      filter(lambda p: p.dns.flags.rcode == 0).\
      must_next()

    # Step 16
    # - Device: Eth_1
    # - Description (SRP-2.11): Harness instructs the device to send mDNS query QType SRV for
    #   "service-test-1. thread-test_udp.local.".
    # - Pass Criteria:
    #   - N/A
    print("Step 16: Eth_1 sends mDNS query QType SRV")
    pkts.\
      filter(lambda p: p.eth.src == ETH_1_ETH).\
      filter(lambda p: p.udp.dstport == 5353).\
      filter(lambda p: p.mdns.flags.response == 0).\
      filter(lambda p: MDNS_INSTANCE_NAME in verify_utils.as_list(p.mdns.qry.name)).\
      filter(lambda p: DNS_TYPE_SRV in verify_utils.as_list(p.mdns.qry.type)).\
      must_next()

    # Step 17
    # - Device: BR 1 (DUT)
    # - Description (SRP-2.11): Automatically sends mDNS Response.
    # - Pass Criteria:
    #   - The DUT MUST send a valid mDNS Response, with RCODE=0 (NoError); and contains
    #     service-test-1 answer record: SORIGIN local. service-test-1._thread-test._udp ( SRV 8 1500
    #     host-test-1 )
    print("Step 17: BR 1 (DUT) Automatically sends mDNS Response.")
    pkts.\
      filter(lambda p: p.eth.src == BR_1_ETH).\
      filter(lambda p: p.udp.dstport == 5353).\
      filter(lambda p: p.mdns.flags.response == 1).\
      filter(lambda p: MDNS_INSTANCE_NAME in verify_utils.as_list(p.mdns.resp.name)).\
      filter(lambda p: SRP_SERVICE_PORT in verify_utils.as_list(p.mdns.srv.port)).\
      filter(lambda p: SRP_SERVICE_PRIORITY in verify_utils.as_list(p.mdns.srv.priority)).\
      filter(lambda p: SRP_SERVICE_WEIGHT in verify_utils.as_list(p.mdns.srv.weight)).\
      must_next()

    # Step 18
    # - Device: Eth 1
    # - Description (SRP-2.11): Harness instructs the device to reset
    # - Pass Criteria:
    #   - N/A
    print("Step 18: Eth 1 reset")

    # Step 19
    # - Device: ED 1
    # - Description (SRP-2.11): Harness instructs the device to send SRP Update, as in step 4.
    # - Pass Criteria:
    #   - N/A
    print("Step 19: ED 1 sends SRP Update")
    pkts.\
      filter_ipv6_dst(BR_1_MLEID).\
      filter_ipv6_src(ED_1_MLEID).\
      filter(lambda p: p.udp.dstport == BR_1_SRP_PORT_2).\
      filter(lambda p: p.dns.flags.opcode == consts.DNS_OPCODE_UPDATE).\
      filter(lambda p: SRP_INSTANCE_NAME in verify_utils.as_list(p.dns.resp.name)).\
      filter(lambda p: SRP_SERVICE_PORT in verify_utils.as_list(p.dns.srv.port)).\
      filter(lambda p: SRP_SERVICE_PRIORITY in verify_utils.as_list(p.dns.srv.priority)).\
      filter(lambda p: SRP_SERVICE_WEIGHT in verify_utils.as_list(p.dns.srv.weight)).\
      filter(lambda p: SRP_HOST_NAME in verify_utils.as_list(p.dns.srv.target)).\
      must_next()

    # Step 20
    # - Device: BR 1 (DUT)
    # - Description (SRP-2.11): Automatically sends SRP Update Response.
    # - Pass Criteria:
    #   - The DUT MUST send a valid SRP Update Response, with RCODE=0 (NoError).
    print("Step 20: BR 1 (DUT) Automatically sends SRP Update Response.")
    pkts.\
      filter_ipv6_dst(ED_1_MLEID).\
      filter_ipv6_src(BR_1_MLEID).\
      filter(lambda p: p.udp.srcport == BR_1_SRP_PORT_2).\
      filter(lambda p: p.dns.flags.response == 1).\
      filter(lambda p: p.dns.flags.rcode == 0).\
      must_next()

    # Step 21
    # - Device: Eth 1
    # - Description (SRP-2.11): Harness instructs the device to send mDNS query QType SRV for
    #   "service-test-1. thread-test._udp.local.".
    # - Pass Criteria:
    #   - N/A
    print("Step 21: Eth 1 sends mDNS query QType SRV")
    pkts.\
      filter(lambda p: p.eth.src == ETH_1_ETH).\
      filter(lambda p: p.udp.dstport == 5353).\
      filter(lambda p: p.mdns.flags.response == 0).\
      filter(lambda p: MDNS_INSTANCE_NAME in verify_utils.as_list(p.mdns.qry.name)).\
      filter(lambda p: DNS_TYPE_SRV in verify_utils.as_list(p.mdns.qry.type)).\
      must_next()

    # Step 22
    # - Device: BR 1 (DUT)
    # - Description (SRP-2.11): Automatically sends mDNS Response.
    # - Pass Criteria:
    #   - The DUT MUST send a valid mDNS Response, with RCODE=0 (NoError); and contains
    #     service-test-1. SORIGIN local. service-test-1. thread-test._udp ( SRV 8 8 1500 host-test-1 )
    print("Step 22: BR 1 (DUT) Automatically sends mDNS Response.")
    pkts.\
      filter(lambda p: p.eth.src == BR_1_ETH).\
      filter(lambda p: p.udp.dstport == 5353).\
      filter(lambda p: p.mdns.flags.response == 1).\
      filter(lambda p: MDNS_INSTANCE_NAME in verify_utils.as_list(p.mdns.resp.name)).\
      filter(lambda p: SRP_SERVICE_PORT in verify_utils.as_list(p.mdns.srv.port)).\
      filter(lambda p: SRP_SERVICE_PRIORITY in verify_utils.as_list(p.mdns.srv.priority)).\
      filter(lambda p: SRP_SERVICE_WEIGHT in verify_utils.as_list(p.mdns.srv.weight)).\
      must_next()


if __name__ == '__main__':
    verify_utils.run_main(verify)
