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
    # 10.4 IPv4 Connectivity using BR built-in NAT64
    # 10.4.1. Purpose
    # - To verify that BR DUT:
    #   - Offers IPv4 internet connectivity to/from Thread Devices using its built-in NAT64 translator, when the
    #     infrastructure network does not provide a NAT64 translator.
    #   - Offers IPv4 local network connectivity to/from Thread Devices using its built-in NAT64 translator
    #   - Operates DNS recursive resolver to look up IPv4 server addresses
    #   - Withdraws its own NAT64 prefix when another Border Router advertises a preferred NAT64 prefix. TBD - to
    #     be added.
    # - Note: the present test requires a /96 NAT64 prefix, which is stricter than what the specification requires.
    #   RFC 6052 (Section 2.2) allows other lengths as well; however the present test case is not yet written to
    #   support all these different options.
    # 10.4.2. Topology
    # - BR_1 (DUT) - Border Router
    # - Router_1 - Thread Router Reference Device
    # - ED_1 - Thread Reference Device, End Device (e.g. FED/REED) role
    # - Eth_1 - Adjacent Infrastructure Link Linux Reference Device on AIL
    #   - Has HTTP web server with resource ‘/test2.html’.
    # - Eth_2 - Adjacent Infrastructure Link SPIFF Reference Device on AIL
    #   - Has HTTP web server on simulated-internet with resource ‘/testv4.html’

    pkts = pv.pkts
    pv.summary.show()

    BR_1 = pv.vars['BR_1']
    ROUTER_1 = pv.vars['ROUTER_1']
    ED_1 = pv.vars['ED_1']
    ETH_2_ADDR = pv.vars['ETH_2_ADDR']
    INTERNET_SERVER_ADDR = pv.vars['INTERNET_SERVER_ADDR']
    LOCAL_SERVER_ADDR = pv.vars['LOCAL_SERVER_ADDR']
    NAT64_PREFIX = pv.vars['NAT64_PREFIX']

    # Step 1
    # - Device: Eth_1, Eth_2
    # - Description (PIC-10.4): Enable Harness configures Eth_1 with IPv4 address 192.168.1.4.
    # - Pass Criteria: N/A
    print("Step 1: Enable Eth_1 and Eth_2")

    # Step 2
    # - Device: Eth_2
    # - Description (PIC-10.4): Harness instructs device to: Enable DHCP IPv4 server.
    # - Pass Criteria: N/A
    print("Step 2: Eth_2 default configuration")

    # Step 3
    # - Device: BR_1 (DUT), Router_1, ED_1
    # - Description (PIC-10.4): Enable
    # - Pass Criteria: N/A
    print("Step 3: Enable BR_1, Router_1, ED_1")

    # Step 4
    # - Device: BR_1 (DUT)
    # - Description (PIC-10.4): Automatically configures an IPv4 address on the AIL; and identifies the IPv4 gateway
    #   on Eth_2. Automatically configures a NAT64 prefix in the Thread Network Data. Note: the check for length 96
    #   is not due to it being required by, but because the current test script is only designed to handle this
    #   case. Other cases are TBD. See note in Section 10.4.1 above.
    # - Pass Criteria:
    #   - The DUT MUST add one NAT64 prefix in the Thread Network Data, which is a route entry published as follows:
    #     - Prefix TLV
    #       - Prefix Length field MUST be 96.
    print("Step 4: BR_1 configures IPv4 address and NAT64 prefix")
    nat64_prefix_pattern = Ipv6Addr(NAT64_PREFIX.split('/')[0])[:12]
    pkts.filter_wpan_src64(BR_1). \
        filter_mle_cmd(consts.MLE_DATA_RESPONSE). \
        filter(lambda p: any(pre.startswith(nat64_prefix_pattern) for pre in p.thread_nwd.tlv.prefix)). \
        must_next()

    # Step 5
    # - Device: ED_1
    # - Description (PIC-10.4): Harness instructs device to perform DNS query QType=ANY, name “threadv4.org”.
    #   Automatically, the DNS query gets routed to the DUT.
    # - Pass Criteria: N/A
    print("Step 5: ED_1 performs DNS query for threadv4.org")
    dns_query = pkts.filter(lambda p: 'dns' in p.layer_names). \
        filter(lambda p: any(name.rstrip('.') == "threadv4.org" for name in verify_utils.as_list(p.dns.qry.name))). \
        filter(lambda p: any(t == consts.DNS_TYPE_ANY for t in verify_utils.as_list(p.dns.qry.type))). \
        must_next()

    # Step 6
    # - Device: BR_1 (DUT)
    # - Description (PIC-10.4): Automatically processes the DNS query by requesting upstream to the Eth_2 DNS server.
    #   Then, it responds back with the answer to ED_1.
    # - Pass Criteria: N/A
    print("Step 6: BR_1 processes DNS query upstream")
    pkts.filter_ipv6_dst(ETH_2_ADDR). \
        filter(lambda p: 'dns' in p.layer_names). \
        filter(lambda p: any(name.rstrip('.') == "threadv4.org" for name in verify_utils.as_list(p.dns.qry.name))). \
        filter(lambda p: any(t == consts.DNS_TYPE_ANY for t in verify_utils.as_list(p.dns.qry.type))). \
        must_next()

    # Step 7
    # - Device: ED_1
    # - Description (PIC-10.4): Successfully receives DNS query result.
    # - Pass Criteria:
    #   - ED_1 MUST receive DNS query answer from DUT:
    #     - threadv4.org A 38.106.217.165
    print("Step 7: ED_1 receives DNS result")
    # Verify the response is received
    pkts.filter(lambda p: 'dns' in p.layer_names). \
        filter(lambda p: any(name.rstrip('.') == "threadv4.org" for name in verify_utils.as_list(p.dns.resp.name))). \
        must_next()

    # Step 8
    # - Device: ED_1
    # - Description (PIC-10.4): Harness instructs device to send ICMPv6 ping request to internet server, using
    #   synthesized IPv6 address: <NAT64-prefix>:266a:d9a5 Request is routed via the DUT to the Eth_2 simulated
    #   network. The DUT translates the above IPv6 address into the IPv4 destination address 38.106.217.165.
    # - Pass Criteria:
    #   - ED_1 MUST receive ICMPv6 ping response from simulated server.
    print("Step 8: ED_1 pings internet server")
    ping_req = pkts.filter_ipv6_dst(INTERNET_SERVER_ADDR). \
        filter_ping_request(). \
        must_next()
    # ED_1 receives response
    pkts.filter_ipv6_src(INTERNET_SERVER_ADDR). \
        filter_ping_reply(identifier=ping_req.icmpv6.echo.identifier). \
        must_next()

    # Step 8a
    # - Device: ED_1
    # - Description (PIC-10.4): Repeats same using a UDP ping.
    # - Pass Criteria:
    #   - ED_1 MUST receive UDP ping response from simulated server.
    print("Step 8a: ED_1 sends UDP ping to internet server")
    pkts.filter_ipv6_dst(INTERNET_SERVER_ADDR). \
        filter(lambda p: 'udp' in p.layer_names). \
        must_next()

    # Step 9
    # - Device: ED_1
    # - Description (PIC-10.4): Harness instructs device to send HTTP GET request to server. If ED_1 is a Linux
    #   device, it may use 'curl' to save the file: curl -o testv4.html http://threadv4.org/testv4.html In case ED_1
    #   is a non-Linux Thread CLI device, it can use the OT CLI commands to send the HTTP GET request: tcp init
    #   tcp connect <NAT64-prefix>:266a:d9a5 80 tcp send -x 474554202F74... tcp sendend tcp deinit
    # - Pass Criteria:
    #   - In case Linux 'curl' was used: File testv4.html MUST equal the testv4.html file as stored on the simulated
    #     server on Eth_2.
    #   - In case OT CLI was used: Output on the CLI MUST be the HTML file testv4.html as stored on the simulator
    #     server on Eth_2.
    print("Step 9: ED_1 sends HTTP GET to internet server")
    pkts.filter_ipv6_dst(INTERNET_SERVER_ADDR). \
        filter(lambda p: 'tcp' in p.layer_names). \
        must_next()

    # Step 10
    # - Device: ED_1
    # - Description (PIC-10.4): Harness instructs device to send ping request to local IPv4 server Eth_1 which has
    #   IPv4 destination address 192.168.1.4; using synthesized IPv6 address: <NAT64-prefix>:c0a8:0104 Automatically,
    #   it is routed via BR_1 DUT to Eth_1.
    # - Pass Criteria:
    #   - ED_1 MUST receive ping response from Eth_1.
    print("Step 10: ED_1 pings local server Eth_1")
    ping_req = pkts.filter_ipv6_dst(LOCAL_SERVER_ADDR). \
        filter_ping_request(). \
        must_next()
    # ED_1 receives response
    pkts.filter_ipv6_src(LOCAL_SERVER_ADDR). \
        filter_ping_reply(identifier=ping_req.icmpv6.echo.identifier). \
        must_next()

    # Step 11
    # - Device: ED_1
    # - Description (PIC-10.4): Harness instructs device to send HTTP GET request to local server Eth_1. In case ED_1
    #   is a Linux device, it can use 'curl' to save the file locally: curl -o test2.html http:///test2.html In case
    #   ED_1 is a non-Linux Thread CLI device, it can use the OT CLI commands to send the HTTP GET request: tcp init
    #   tcp connect <NAT64-prefix>:c0a8:0104 80 tcp send -x 474554202F... tcp sendend tcp deinit
    # - Pass Criteria:
    #   - In case Linux 'curl' was used: File test2.html MUST equal the test2.html file as stored on the Eth_1 server.
    #   - In case OT CLI was used: Output on the CLI MUST be the HTML file test2.html as stored on the server on Eth_1.
    print("Step 11: ED_1 sends HTTP GET to local server Eth_1")
    pkts.filter_ipv6_dst(LOCAL_SERVER_ADDR). \
        filter(lambda p: 'tcp' in p.layer_names). \
        must_next()


if __name__ == '__main__':
    verify_utils.run_main(verify)
