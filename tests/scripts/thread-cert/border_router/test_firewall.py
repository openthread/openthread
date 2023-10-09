#!/usr/bin/env python3
#
#  Copyright (c) 2021, The OpenThread Authors.
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
#  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS 'AS IS'
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
import logging
import time
import unittest
import ipaddress

import config
import pktverify
import pktverify.packet_verifier
from pktverify.consts import MA1
import thread_cert

# Test description:
#   This test verifies the functionality of firewall. OTBR will only
#   forward specific kinds of packets between the infra interface and the thread
#   interface.
#
# Topology:
#    ----------------(eth)----------------------
#           |                  |
#          BR1 (Leader)      HOST
#           |      \
#         ROUTER1  ROUTER2

BR1 = 1
ROUTER1 = 2
ROUTER2 = 3
HOST = 4


class Firewall(thread_cert.TestCase):
    USE_MESSAGE_FACTORY = False

    TOPOLOGY = {
        BR1: {
            'name': 'BR_1',
            'allowlist': [ROUTER1, ROUTER2],
            'is_otbr': True,
            'version': '1.3',
        },
        ROUTER1: {
            'name': 'Router_1',
            'allowlist': [BR1],
            'version': '1.3',
        },
        ROUTER2: {
            'name': 'Router_2',
            'allowlist': [BR1],
            'version': '1.3',
        },
        HOST: {
            'name': 'Host',
            'is_host': True,
        }
    }

    def test(self):
        br1 = self.nodes[BR1]
        self.br1 = br1
        router1 = self.nodes[ROUTER1]
        router2 = self.nodes[ROUTER2]
        host = self.nodes[HOST]

        br1.start()
        self.simulator.go(config.LEADER_STARTUP_DELAY)
        self.assertEqual('leader', br1.get_state())
        br1.set_log_level(5)

        router1.start()
        router2.start()
        host.start(start_radvd=True)
        self.simulator.go(config.ROUTER_STARTUP_DELAY)
        self.assertEqual('router', router1.get_state())
        self.assertEqual('router', router2.get_state())

        br1.set_domain_prefix(config.DOMAIN_PREFIX, 'prosD')
        br1.register_netdata()

        router1.add_ipmaddr(MA1)
        router1.register_netdata()

        self.simulator.go(20)

        def host_ping_ether(dest, interface, ttl=10, add_interface=False, add_route=False, gateway=None):
            if add_interface:
                host.bash(f'ip -6 addr add {interface}/64 dev {host.ETH_DEV} scope global')
                route = str(ipaddress.IPv6Network(f'{interface}/64', strict=False))
                host.bash(f'ip -6 route del {route} dev {host.ETH_DEV}')
            if add_route:
                host.bash(f'ip -6 route add {dest} dev {host.ETH_DEV} via {gateway}')
            self.simulator.go(2)
            result = host.ping_ether(dest, ttl=ttl, interface=interface)
            if add_route:
                host.bash(f'ip -6 route del {dest}')
            if add_interface:
                host.bash(f'ip -6 addr del {interface}/64 dev {host.ETH_DEV} scope global')
            self.simulator.go(1)
            return result

        # 1. Host pings router1's OMR from host's infra address.
        self.assertTrue(host_ping_ether(router1.get_ip6_address(config.ADDRESS_TYPE.OMR)[0], interface=host.ETH_DEV))

        # 2. Host pings router1's DUA from host's infra address.
        self.assertTrue(host_ping_ether(router1.get_ip6_address(config.ADDRESS_TYPE.DUA), interface=host.ETH_DEV))

        # 3. Host pings router1's OMR from router1's RLOC.
        self.assertFalse(
            host_ping_ether(router1.get_ip6_address(config.ADDRESS_TYPE.OMR)[0],
                            interface=router1.get_rloc(),
                            add_interface=True))

        # 4. Host pings router1's OMR from router2's OMR.
        self.assertFalse(
            host_ping_ether(router1.get_ip6_address(config.ADDRESS_TYPE.OMR)[0],
                            interface=router2.get_ip6_address(config.ADDRESS_TYPE.OMR)[0],
                            add_interface=True))

        # 5. Host pings router1's OMR from router1's ML-EID.
        self.assertFalse(
            host_ping_ether(router1.get_ip6_address(config.ADDRESS_TYPE.OMR)[0],
                            interface=router1.get_mleid(),
                            add_interface=True))

        host.bash(f'ip -6 route add {config.MESH_LOCAL_PREFIX} dev {host.ETH_DEV}')
        self.simulator.go(5)

        # 6. Host pings router1's RLOC from host's ULA address.
        self.assertFalse(
            host_ping_ether(router1.get_rloc(),
                            interface=host.get_ip6_address(config.ADDRESS_TYPE.ONLINK_ULA)[0],
                            add_route=True,
                            gateway=br1.get_ip6_address(config.ADDRESS_TYPE.BACKBONE_GUA)))

        # 7. Host pings router1's ML-EID from host's ULA address.
        self.assertFalse(
            host_ping_ether(router1.get_mleid(),
                            interface=host.get_ip6_address(config.ADDRESS_TYPE.ONLINK_ULA)[0],
                            add_route=True,
                            gateway=br1.get_ip6_address(config.ADDRESS_TYPE.BACKBONE_GUA)))

        # 8. Host pings router1's link-local address from host's infra address.
        self.assertFalse(
            host_ping_ether(router1.get_linklocal(),
                            interface=host.ETH_DEV,
                            add_route=True,
                            gateway=br1.get_ip6_address(config.ADDRESS_TYPE.BACKBONE_GUA)))

        # 9. Host pings MA1 from host's ULA address.
        self.assertTrue(host_ping_ether(MA1, ttl=10,
                                        interface=host.get_ip6_address(config.ADDRESS_TYPE.ONLINK_ULA)[0]))

        # 10. Host pings MA1 from router1's RLOC.
        self.assertFalse(host_ping_ether(MA1, ttl=10, interface=router1.get_rloc(), add_interface=True))

        # 11. Host pings MA1 from router1's OMR.
        self.assertFalse(
            host_ping_ether(MA1,
                            ttl=10,
                            interface=router1.get_ip6_address(config.ADDRESS_TYPE.OMR)[0],
                            add_interface=True))

        # 12. Host pings MA1 from router1's ML-EID.
        self.assertFalse(host_ping_ether(MA1, ttl=10, interface=router1.get_mleid(), add_interface=True))

        # 13. Router1 pings Host from router1's ML-EID.
        self.assertFalse(
            router1.ping(host.get_ip6_address(config.ADDRESS_TYPE.ONLINK_ULA)[0], interface=router1.get_mleid()))

        # 14. BR pings router1's ML-EID from BR's ML-EID.
        self.assertTrue(br1.ping_ether(router1.get_mleid(), interface=br1.get_mleid()))

        # 15. BR pings router1's OMR from BR's infra interface.
        self.assertTrue(
            br1.ping_ether(router1.get_ip6_address(config.ADDRESS_TYPE.OMR)[0],
                           interface=br1.get_ip6_address(config.ADDRESS_TYPE.ONLINK_ULA)[0]))

        # 16. Host sends a UDP packet to router1's OMR address's TMF port.
        host.udp_send_host(router1.get_ip6_address(config.ADDRESS_TYPE.OMR)[0], config.TMF_PORT, "HELLO")

        # 17. BR1 sends a UDP packet to router1's ML-EID's TMF port.
        br1.udp_send_host(router1.get_mleid(), config.TMF_PORT, "BYE")

        # 18. BR1 sends a UDP packet to its own ML-EID's TMF port.
        br1.udp_send_host(br1.get_mleid(), config.TMF_PORT, "SELF")

        self.collect_ipaddrs()
        self.collect_rlocs()
        self.collect_rloc16s()
        self.collect_extra_vars()
        self.collect_omrs()
        self.collect_duas()

    def verify(self, pv: pktverify.packet_verifier.PacketVerifier):
        pkts = pv.pkts
        vars = pv.vars
        pv.summary.show()
        logging.info(f'vars = {vars}')

        pv.verify_attached('Router_1', 'BR_1')

        # 1. Host pings router1's OMR from host's infra address.
        _pkt = pkts.filter_eth_src(vars['Host_ETH']).filter_ipv6_dst(
            vars['Router_1_OMR'][0]).filter_ping_request().must_next()
        pkts.filter_wpan_src64(vars['BR_1']).filter_wpan_dst16(
            vars['Router_1_RLOC16']).filter_ping_request(identifier=_pkt.icmpv6.echo.identifier).must_next()

        # 2. Host pings router1's DUA from host's infra address.
        _pkt = pkts.filter_eth_src(vars['Host_ETH']).filter_ipv6_dst(
            vars['Router_1_DUA']).filter_ping_request().must_next()
        pkts.filter_wpan_src64(vars['BR_1']).filter_wpan_dst16(
            vars['Router_1_RLOC16']).filter_ping_request(identifier=_pkt.icmpv6.echo.identifier).must_next()

        # 3. Host pings router1's OMR from router1's RLOC.
        _pkt = pkts.filter_eth_src(vars['Host_ETH']).filter_ipv6_src_dst(
            vars['Router_1_RLOC'], vars['Router_1_OMR'][0]).filter_ping_request().must_next()
        pkts.filter_wpan_src64(vars['BR_1']).filter_wpan_dst16(
            vars['Router_1_RLOC16']).filter_ping_request(identifier=_pkt.icmpv6.echo.identifier).must_not_next()

        # 4. Host pings router1's OMR from router2's OMR.
        _pkt = pkts.filter_eth_src(vars['Host_ETH']).filter_ipv6_src_dst(
            vars['Router_2_OMR'][0], vars['Router_1_OMR'][0]).filter_ping_request().must_next()
        pkts.filter_wpan_src64(vars['BR_1']).filter_wpan_dst64(
            vars['Router_1']).filter_ping_request(identifier=_pkt.icmpv6.echo.identifier).must_not_next()

        # 5. Host pings router1's OMR from router1's ML-EID.
        _pkt = pkts.filter_eth_src(vars['Host_ETH']).filter_ipv6_src_dst(
            vars['Router_1_MLEID'], vars['Router_1_OMR'][0]).filter_ping_request().must_next()
        pkts.filter_wpan_src64(vars['BR_1']).filter_wpan_dst16(
            vars['Router_1_RLOC16']).filter_ping_request(identifier=_pkt.icmpv6.echo.identifier).must_not_next()

        # 6. Host pings router1's RLOC from host's ULA address.
        _pkt = pkts.filter_eth_src(vars['Host_ETH']).filter_ipv6_dst(
            vars['Router_1_RLOC']).filter_ping_request().must_next()
        pkts.filter_wpan_src64(vars['BR_1']).filter_wpan_dst16(
            vars['Router_1_RLOC16']).filter_ping_request(identifier=_pkt.icmpv6.echo.identifier).must_not_next()

        # 7. Host pings router1's ML-EID from host's ULA address.
        _pkt = pkts.filter_eth_src(vars['Host_ETH']).filter_ipv6_dst(
            vars['Router_1_MLEID']).filter_ping_request().must_next()
        pkts.filter_wpan_src64(vars['BR_1']).filter_wpan_dst16(
            vars['Router_1_RLOC16']).filter_ping_request(identifier=_pkt.icmpv6.echo.identifier).must_not_next()

        # 8. Host pings router1's link-local address from host's infra address.
        # Skip this scenario as for now
        # _pkt = pkts.filter_eth_src(vars['Host_ETH']).filter_ipv6_dst(
        #     vars['Router_1_LLA']).filter_ping_request().must_next()
        pkts.filter_wpan_src64(vars['BR_1']).filter_wpan_dst16(
            vars['Router_1_RLOC16']).filter_ping_request(identifier=_pkt.icmpv6.echo.identifier).must_not_next()

        # 9. Host pings MA1 from host's ULA address.
        _pkt = pkts.filter_eth_src(vars['Host_ETH']).filter_ipv6_dst(MA1).filter_ping_request().must_next()
        pkts.filter_wpan_src64(
            vars['BR_1']).filter_AMPLFMA().filter_ping_request(identifier=_pkt.icmpv6.echo.identifier).must_next()

        # 10. Host pings MA1 from router1's RLOC.
        _pkt = pkts.filter_eth_src(vars['Host_ETH']).filter_ipv6_src_dst(vars['Router_1_RLOC'],
                                                                         MA1).filter_ping_request().must_next()
        pkts.filter_wpan_src64(
            vars['BR_1']).filter_AMPLFMA().filter_ping_request(identifier=_pkt.icmpv6.echo.identifier).must_not_next()

        # 11. Host pings MA1 from router1's OMR.
        _pkt = pkts.filter_eth_src(vars['Host_ETH']).filter_ipv6_src_dst(vars['Router_1_OMR'][0],
                                                                         MA1).filter_ping_request().must_next()
        pkts.filter_wpan_src64(
            vars['BR_1']).filter_AMPLFMA().filter_ping_request(identifier=_pkt.icmpv6.echo.identifier).must_not_next()

        # 12. Host pings MA1 from router1's ML-EID.
        _pkt = pkts.filter_eth_src(vars['Host_ETH']).filter_ipv6_src_dst(vars['Router_1_MLEID'],
                                                                         MA1).filter_ping_request().must_next()
        pkts.filter_wpan_src64(
            vars['BR_1']).filter_AMPLFMA().filter_ping_request(identifier=_pkt.icmpv6.echo.identifier).must_not_next()

        # 13. Router1 pings Host from router1's ML-EID.
        pkts.filter_eth_src(vars['BR_1_ETH']).filter_ipv6_src_dst(
            vars['Router_1_MLEID'], vars['Host_BGUA']).filter_ping_request().must_not_next()

        # 14. BR pings router1's ML-EID from BR's ML-EID.
        _pkt = pkts.filter_wpan_src64(vars['BR_1']).filter_ipv6_src_dst(
            vars['BR_1_MLEID'], vars['Router_1_MLEID']).filter_ping_request().must_next()
        pkts.filter_wpan_src64(vars['Router_1']).filter_ping_reply(identifier=_pkt.icmpv6.echo.identifier).must_next()

        # 15. BR pings router1's OMR from BR's infra interface.
        _pkt = pkts.filter_wpan_src64(vars['BR_1']).filter_ping_request().must_next()
        pkts.filter_wpan_src64(vars['Router_1']).filter_ping_reply(identifier=_pkt.icmpv6.echo.identifier).must_next()

        # 16. Host sends a UDP packet to router1's OMR address's TMF port (61631).
        # The packet should be able to reach BR1 but BR1 won't forward it to Thread.
        pkts.filter_eth_src(vars['Host_ETH']).filter_ipv6_dst(vars['Router_1_OMR'][0]).filter(
            lambda p: p.udp.dstport == config.TMF_PORT and p.udp.length == len("HELLO") + 8).must_next()
        pkts.filter_wpan_src64(vars['BR_1']).filter_ipv6_dst(vars['Router_1_OMR'][0]).filter(
            lambda p: p.udp.dstport == config.TMF_PORT and p.udp.length == len("HELLO") + 8).must_not_next()

        # 17. BR1 sends a UDP packet to router1's ML-EID's TMF port (61631).
        # The packet should be dropped by BR1, so it should not be present in Thread.
        pkts.filter_wpan_src64(vars['BR_1']).filter_ipv6_dst(vars['Router_1_MLEID']).filter(
            lambda p: p.udp.dstport == config.TMF_PORT and p.udp.length == len("BYE") + 8).must_not_next()

        # 18. BR1 sends a UDP packet to its own ML-EID's TMF port (61631).
        # The packet should be dropped by BR1, so it should not be present anywhere.
        pkts.filter_wpan_src64(vars['BR_1']).filter_ipv6_dst(vars['BR_1_MLEID']).filter(
            lambda p: p.udp.dstport == config.TMF_PORT and p.udp.length == len("SELF") + 8).must_not_next()


if __name__ == '__main__':
    unittest.main()
