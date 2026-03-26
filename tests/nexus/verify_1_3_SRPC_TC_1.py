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
    # 3.1. [1.3] [CERT] [COMPONENT] Re-register service with active SRP server - Multiple BRs - No AIL
    #
    # 3.1.1. Purpose
    # To test the following:
    # - DNS/SRP Unicast Dataset(s) offered in the Thread network
    # - Detects unresponsiveness of the SRP server and sends SRP update to another active SRP server
    # - Detects a second, numerically lower, SRP server and DUT stays with its current SRP server and does not switch
    #   to the numerically lowest SRP server
    # - Registered service is automatically re-registered with active SRP server before timeout
    # - In the absence of an OMR prefix, mesh-local (ML-EID) address will be used by SRP client.
    #
    # Spec Reference   | V1.1 Section | V1.3.0 Section
    # -----------------|--------------|---------------
    # SRP Client       | N/A          | 2.3.2

    pkts = pv.pkts
    pv.summary.show()

    BR_1_MLEID = pv.vars['BR_1_MLEID']
    BR_2_MLEID = pv.vars['BR_2_MLEID']
    ED_1 = pv.vars['ED_1']
    ED_1_MLEID = pv.vars['ED_1_MLEID']

    SRP_SERVICE_NAME = '_thread-test._udp.default.service.arpa'
    SRP_INSTANCE_NAME = 'service-test-1.' + SRP_SERVICE_NAME

    # Step 0
    # Device: Router 1
    # Description (SRPC-3.1): Enable. Automatically, becomes Leader.
    # Pass Criteria:
    #   N/A
    print("Step 0: Router 1 becomes Leader.")

    # Step 1
    # Device: BR_1
    # Description (SRPC-3.1): Enable: Harness configures the device with a numerically lower ML-EID address than
    #   BR_2 will have. Note: due to absence of adjacent infrastructure link (AIL), no OMR address is configured by
    #   the BR.
    # Pass Criteria:
    #   N/A
    print("Step 1: BR_1 joins the network.")

    # Step 2
    # Device: ED 1 (DUT)
    # Description (SRPC-3.1): Enable; automatically connects to Router_1.
    # Pass Criteria:
    #   N/A
    print("Step 2: ED_1 joins the network.")

    # Step 3
    # Device: BR 1
    # Description (SRPC-3.1): Automatically adds its SRP Server information in the Thread Network Data by using a
    #   DNS/SRP Unicast Dataset, indicating its ML-EID address.
    # Pass Criteria:
    #   N/A
    print("Step 3: BR_1 adds its SRP Server information.")

    # Step 4
    # Device: ED 1 (DUT)
    # Description (SRPC-3.1): Harness instructs DUT to add a service to the device: $ORIGIN default.service.arpa.
    #   service-test-1._thread-test._udp ( SRV 33333 host-test-1 ) host-test-1 AAAA <ML-EID address of ED_1> with the
    #   following options: Update Lease Option Lease: 30 seconds Key Lease: 10 minutes
    # Pass Criteria:
    #   - The DUT ED_1 MUST send SRP Update to BR_1 containing the service-test-1.
    #   - The IPv6 address in the AAAA record of the SRP Update MUST be an ML-EID, specifically the ML-EID of the DUT.
    #   - It MUST NOT contain any other IPv6 addresses in AAAA records.
    print("Step 4: ED_1 sends SRP Update to BR_1.")
    pkts.filter_wpan_src64(ED_1).\
        filter_ipv6_dst(BR_1_MLEID).\
        filter(lambda p: p.dns.flags.opcode == consts.DNS_OPCODE_UPDATE).\
        filter(lambda p: SRP_INSTANCE_NAME in verify_utils.as_list(p.dns.resp.name)).\
        filter(lambda p: ED_1_MLEID in verify_utils.as_list(p.dns.aaaa)).\
        filter(lambda p: len(verify_utils.as_list(p.dns.aaaa)) == 1).\
        must_next()

    # Step 5
    # Device: BR_1
    # Description (SRPC-3.1): Automatically sends DNS Response (Rcode=0, NoError).
    # Pass Criteria:
    #   N/A
    print("Step 5: BR_1 sends DNS Response.")
    pkts.filter_ipv6_src(BR_1_MLEID).\
        filter_ipv6_dst(ED_1_MLEID).\
        filter(lambda p: p.dns.flags.response == 1).\
        must_next()

    # Step 6
    # Device: BR 2
    # Description (SRPC-3.1): Enable. Harness configures device with a numerically higher ML-EID address than BR 1.
    #   This ensures that the BR 2 SRP service will be less preferred to the DUT. Note: due to absence of adjacent
    #   infrastructure link (AIL), no OMR address is configured by the BR.
    # Pass Criteria:
    #   N/A
    print("Step 6: BR_2 joins the network.")

    # Step 7
    # Device: BR 2
    # Description (SRPC-3.1): Harness instructs the device to add its SRP Server information in the Thread Network
    #   Data using a DNS/SRP Unicast Dataset. Note: normally BR_2 does not add this information, since an SRP server
    #   is already advertised on the Thread Network. The Harness overrides this behavior.
    # Pass Criteria:
    #   - The DUT MUST NOT send SRP Update to BR_2.
    print("Step 7: BR_2 adds its SRP Server information. MUST NOT send SRP Update to BR_2.")

    # Step 8
    # Device: BR 1
    # Description (SRPC-3.1): Harness deactivates the device.
    # Pass Criteria:
    #   N/A
    print("Step 8: BR_1 is deactivated.")

    # Step 8b
    # Device: N/A
    # Description (SRPC-3.1): Harness waits 30 seconds. Within this time, the next step must happen.
    # Pass Criteria:
    #   N/A
    print("Step 8b: Wait 30 seconds.")

    # Step 9
    # Device: ED 1 (DUT)
    # Description (SRPC-3.1): Automatically sends SRP Update to BR_1 to renew service.
    # Pass Criteria:
    #   - The DUT MUST send SRP Update to BR_1 as in step 4.
    print("Step 9: ED_1 sends SRP Update to BR_1 to renew service.")

    # To verify Step 7 and Step 9:
    # We ensure that the NEXT SRP Update from ED_1 is indeed to BR_1, not BR_2.
    pkts.filter_wpan_src64(ED_1).\
        filter(lambda p: p.dns.flags.opcode == consts.DNS_OPCODE_UPDATE).\
        must_next().\
        must_verify(lambda p: p.ipv6.dst == BR_1_MLEID).\
        must_verify(lambda p: SRP_INSTANCE_NAME in verify_utils.as_list(p.dns.resp.name)).\
        must_verify(lambda p: ED_1_MLEID in verify_utils.as_list(p.dns.aaaa)).\
        must_verify(lambda p: len(verify_utils.as_list(p.dns.aaaa)) == 1)

    # Step 10
    # Device: BR 1
    # Description (SRPC-3.1): Does not respond.
    # Pass Criteria:
    #   N/A
    print("Step 10: BR_1 does not respond.")

    # Step 11
    # Device: ED 1 (DUT)
    # Description (SRPC-3.1): Automatically sends SRP Update to BR_2
    # Pass Criteria:
    #   - The DUT MUST send SRP Update like in step 4 but now to BR_2
    print("Step 11: ED_1 sends SRP Update to BR_2.")
    pkts.filter_wpan_src64(ED_1).\
        filter_ipv6_dst(BR_2_MLEID).\
        filter(lambda p: p.dns.flags.opcode == consts.DNS_OPCODE_UPDATE).\
        filter(lambda p: SRP_INSTANCE_NAME in verify_utils.as_list(p.dns.resp.name)).\
        filter(lambda p: ED_1_MLEID in verify_utils.as_list(p.dns.aaaa)).\
        filter(lambda p: len(verify_utils.as_list(p.dns.aaaa)) == 1).\
        must_next()

    # Step 12
    # Device: BR 2
    # Description (SRPC-3.1): Automatically sends SRP Update Response (Rcode=0, NoError).
    # Pass Criteria:
    #   N/A
    print("Step 12: BR_2 sends SRP Update Response.")
    pkts.filter_ipv6_src(BR_2_MLEID).\
        filter_ipv6_dst(ED_1_MLEID).\
        filter(lambda p: p.dns.flags.response == 1).\
        must_next()

    # Step 13
    # Device: BR_1
    # Description (SRPC-3.1): Harness re-enables the device.
    # Pass Criteria:
    #   - N/A
    print("Step 13: BR_1 is re-enabled.")

    # Step 14
    # Device: BR_1
    # Description (SRPC-3.1): Harness instructs the device to add its SRP Server information in the Thread Network
    #   Data using a DNS/SRP Unicast Dataset. Note: normally BR_1 does not add this information, since an SRP server
    #   is already advertised on the Thread Network. The Harness overrides this behavior.
    # Pass Criteria:
    #   - N/A
    print("Step 14: BR_1 adds its SRP Server information.")

    # Step 15
    # Device: ED 1 (DUT)
    # Description (SRPC-3.1): Does not send SRP Update to BR_1, but stays with its current SRP server BR 2..
    # Pass Criteria:
    #   - The DUT MUST NOT send SRP Update to BR_1.
    print("Step 15: ED_1 stays with BR_2. MUST NOT send SRP Update to BR_1.")
    pkts.filter_wpan_src64(ED_1).\
        filter_ipv6_dst(BR_1_MLEID).\
        filter(lambda p: p.dns.flags.opcode == consts.DNS_OPCODE_UPDATE).\
        must_not_next()


if __name__ == '__main__':
    verify_utils.run_main(verify)
