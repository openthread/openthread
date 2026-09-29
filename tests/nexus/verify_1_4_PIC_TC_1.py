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
    # 10.1. IPv6 Connectivity using DHCPv6-PD delegated OMR prefix
    #
    # 10.1.1. Purpose
    # - To verify that BR DUT:
    #   - uses DHCPv6-PD client functionality to obtain an OMR prefix for its Thread Network from the upstream CPE
    #     router
    #   - Offers IPv6 public internet connectivity to/from Thread Devices that configured an OMR address
    #   - Offers IPv6 local network connectivity to/from Thread Devices that configured an OMR address
    #   - Operates DNS recursive resolver to look up IPv6 server addresses on public internet
    #
    # 10.1.2. Topology
    # - BR_1 (DUT) - Border Router
    # - Router_1 - Thread Router Reference Device, attached to BR_1
    # - ED_1 - Thread Reference Device, End Device (e.g. FED/REED) role, attached to Router_1
    # - Eth_1 - Adjacent Infrastructure Link Linux Reference Device
    #   - Has HTTP web server with resource ‘/test2.html’.
    # - Eth_2 - Adjacent Infrastructure Link SPIFF Reference Device

    pkts = pv.pkts
    pv.summary.show()

    BR_1 = pv.vars['BR_1']
    ROUTER_1 = pv.vars['ROUTER_1']
    ED_1 = pv.vars['ED_1']
    ETH_1_ADDR = pv.vars['ETH_1_ADDR']
    ETH_2_ADDR = pv.vars['ETH_2_ADDR']
    INTERNET_SERVER_ADDR = pv.vars['INTERNET_SERVER_ADDR']
    DELEGATED_PREFIX = pv.vars['DELEGATED_PREFIX']

    # Step 1
    # - Device: Eth_1, Eth_2
    # - Description (PIC-10.1): Enable
    # - Pass Criteria:
    #   - N/A
    print("Step 1: Enable Eth_1 and Eth_2")

    # Step 2
    # - Device: Eth_2 (SPIFF)
    # - Description (PIC-10.1): Harness does not configure the device, other than the default configuration. Note: in
    #   previous versions of the test plan, explicit configuration was provided in this step. Note: Eth_1 will
    #   automatically configure a GUA using SLAAC and the RA-advertised prefix.
    # - Pass Criteria:
    #   - N/A
    print("Step 2: Eth_2 default configuration")

    # Step 3
    # - Device: BR_1 (DUT), Router_1, ED_1
    # - Description (PIC-10.1): Enable
    # - Pass Criteria:
    #   - N/A
    print("Step 3: Enable BR_1, Router_1, ED_1")

    # Step 4
    # - Device: BR_1 (DUT)
    # - Description (PIC-10.1): Automatically uses DHCPv6-PD client function to obtain a delegated prefix from the
    #   DHCPv6 server. It configures this prefix as the OMR prefix.
    # - Pass Criteria:
    #   - An OMR prefix OMR_1 MUST appear in Thread Network Data in a Prefix TLV:
    #   - It MUST be subprefix of 2005:1234:abcd:0::/56.
    #   - It MUST be a /64 prefix
    #   - Border Router sub-TLV:
    #     - Prf bits = P_preference = '00' (medium)
    #     - R = P_default = '1'
    print("Step 4: BR_1 obtains OMR prefix via DHCPv6-PD")
    # Verify the prefix in Net Data is a subprefix of 2005:1234:abcd:0::/56
    # 2005:1234:abcd:0001::/64 matches the first 8 bytes of the expanded address.
    omr_prefix_pattern = Ipv6Addr("2005:1234:abcd:0001::")[:8]
    omr_prefix_pkt = pkts.filter_wpan_src64(BR_1). \
        filter_mle_cmd(consts.MLE_DATA_RESPONSE). \
        filter(lambda p: any(pre.startswith(omr_prefix_pattern) for pre in p.thread_nwd.tlv.prefix)). \
        must_next()

    OMR_PREFIX = None
    for pre in omr_prefix_pkt.thread_nwd.tlv.prefix:
        if pre.startswith(omr_prefix_pattern):
            OMR_PREFIX = pre
            break
    assert OMR_PREFIX is not None

    # Step 4b
    # - Device: BR_1 (DUT)
    # - Description (PIC-10.1): Automatically advertises the route to its OMR prefix on the AIL, as a stub router (aka
    #   IETF SNAC router). Note: see 9.6.2 for Stub Router / SNAC Router flag detail.The flag is bit 6 of RA flags as
    #   specified by the TEMPORARY allocation made by IANA (link).
    # - Pass Criteria:
    #   - The DUT MUST advertise the route in multicast ND RA messages:
    #   - IPv6 destination MUST be ff02::1
    #   - SNAC Router flag set to '1'
    #   - Route Information Option (RIO)
    #     - Prefix: OMR_1
    #     - Prf bits: '00' or '11'
    #     - Route Lifetime > 0
    print("Step 4b: BR_1 advertises route to OMR prefix on AIL")
    pkts.filter_ipv6_dst("ff02::1"). \
        filter_icmpv6_nd_ra(). \
        filter(lambda p: verify_utils.check_ra_has_rio(p, OMR_PREFIX)). \
        must_next()

    # Step 5
    # - Device: ED_1
    # - Description (PIC-10.1): Harness instructs device to perform DNS query QType=AAAA, name “threadgroup.org”.
    #   Automatically, the DNS query gets routed to BR_1.
    # - Pass Criteria:
    #   - N/A
    print("Step 5: ED_1 performs DNS query for threadgroup.org")
    dns_query = pkts.filter(lambda p: 'dns' in p.layer_names). \
        filter(lambda p: any(name.rstrip('.') == "threadgroup.org" for name in verify_utils.as_list(p.dns.qry.name))). \
        must_next()

    # Step 6
    # - Device: BR_1 (DUT)
    # - Description (PIC-10.1): Automatically processes the DNS query by requesting upstream to the Eth_2 DNS server.
    #   Then, it responds back with the answer to ED_1.
    # - Pass Criteria:
    #   - N/A
    print("Step 6: BR_1 processes DNS query upstream")
    pkts.filter_ipv6_dst(ETH_2_ADDR). \
        filter(lambda p: 'dns' in p.layer_names). \
        filter(lambda p: any(name.rstrip('.') == "threadgroup.org" for name in verify_utils.as_list(p.dns.qry.name))). \
        must_next()

    # Step 7
    # - Device: ED_1
    # - Description (PIC-10.1): Successfully receives DNS query result: threadgroup.org AAAA 2002:1234::1234
    # - Pass Criteria:
    #   - ED_1 MUST receive DNS query answer from DUT.
    print("Step 7: ED_1 receives DNS result")
    # Just look for the response anywhere in the pcap after the query
    pkts.filter(lambda p: 'dns' in p.layer_names). \
        filter(lambda p: any(name.rstrip('.') == "threadgroup.org" for name in verify_utils.as_list(p.dns.resp.name))). \
        must_next()

    # Step 8
    # - Device: ED_1
    # - Description (PIC-10.1): Harness instructs device to send ICMpv6 ping request to internet server, using resolved
    #   address above. It is routed via the DUT to the Eth_2 simulated network.
    # - Pass Criteria:
    #   - ED_1 MUST receive ICMPv6 ping response from simulated server.
    print("Step 8: ED_1 pings internet server")
    ping_req = pkts.filter_ipv6_dst(INTERNET_SERVER_ADDR). \
        filter_ping_request(). \
        must_next()
    # Look for the reply from internet server
    pkts.filter_ipv6_src(INTERNET_SERVER_ADDR). \
        filter_ping_reply(identifier=ping_req.icmpv6.echo.identifier). \
        must_next()
    # ED_1 receives response
    pkts.filter_ipv6_src(INTERNET_SERVER_ADDR). \
        filter_ping_reply(identifier=ping_req.icmpv6.echo.identifier). \
        must_next()

    # Step 8a
    # - Device: ED_1
    # - Description (PIC-10.1): Repeats same using a UDP ping.
    # - Pass Criteria:
    #   - ED_1 MUST receive UDP ping response from simulated server.
    print("Step 8a: ED_1 sends UDP ping to internet server")
    pkts.filter_ipv6_dst(INTERNET_SERVER_ADDR). \
        filter(lambda p: 'udp' in p.layer_names). \
        must_next()

    # Step 9
    # - Device: ED_1
    # - Description (PIC-10.1): Harness instructs device to sends HTTP GET request to server. If ED_1 is a Linux device,
    #   it may use 'curl' to save the file: curl -o test.html http://threadgroup.org/test.html. In case ED_1 is a
    #   non-Linux Thread CLI device, it can use the OT CLI commands to send the HTTP GET request: tcp init, tcp connect
    #   2002:1234::1234 80, tcp send -x
    #   474554202F746573742E68746D6C20485454502F312E310D0A486F73743A2074687265616467726F75702E6F72670D0A0D0A, tcp
    #   sendend, tcp deinit
    # - Pass Criteria:
    #   - In case Linux 'curl' was used: File test.html MUST equal the test.html file as stored on the simulated server
    #     on Eth_2.
    #   - In case OT CLI was used: Output on the CLI MUST be the HTML file test.html as stored on the simulator server
    #     on Eth_2.
    print("Step 9: ED_1 sends HTTP GET to internet server")
    pkts.filter_ipv6_dst(INTERNET_SERVER_ADDR). \
        filter(lambda p: 'tcp' in p.layer_names). \
        must_next()

    # Step 10
    # - Device: ED_1
    # - Description (PIC-10.1): Harness instructs device to send ping request to local server Eth_1. Automatically, it
    #   is routed via the DUT.
    # - Pass Criteria:
    #   - ED_1 MUST receive ping response from Eth_1.
    print("Step 10: ED_1 pings local server Eth_1")
    ping_req = pkts.filter_ipv6_dst(ETH_1_ADDR). \
        filter_ping_request(). \
        must_next()
    pkts.filter_ipv6_src(ETH_1_ADDR). \
        filter_ping_reply(identifier=ping_req.icmpv6.echo.identifier). \
        must_next()

    # Step 11
    # - Device: ED_1
    # - Description (PIC-10.1): Harness instructs device to send HTTP GET request to local server Eth_1. If ED_1 is a
    #   Linux device, it may use 'curl' to save the file: curl -o test2.html http:///test2.html. In case ED_1 is a
    #   non-Linux Thread CLI device, it can use the OT CLI commands to send the HTTP GET request: tcp init, tcp connect
    #   <Eth_1_GUA_IPv6_addr> 80, tcp send -x
    #   474554202F74657374322E68746D6C20485454502F312E310D0A486F73743A206C6F63616C746573742E6F72670D0A0D0A, tcp
    #   sendend, tcp deinit
    # - Pass Criteria:
    #   - In case Linux 'curl' was used: File test2.html MUST equal the test2.html file as stored on the Eth_1 server.
    #   - In case OT CLI was used: Output on the CLI MUST be the HTML file test.html as stored on the simulator server
    #     on Eth_2.
    print("Step 11: ED_1 sends HTTP GET to local server Eth_1")
    pkts.filter_ipv6_dst(ETH_1_ADDR). \
        filter(lambda p: 'tcp' in p.layer_names). \
        must_next()


if __name__ == '__main__':
    verify_utils.run_main(verify)
