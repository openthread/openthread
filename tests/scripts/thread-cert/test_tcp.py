#!/usr/bin/env python3
#
#  Copyright (c) 2020, The OpenThread Authors.
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

import os
import unittest

import ipaddress
from collections import Counter

import config
import thread_cert

LEADER = 1
ROUTER = 2
FED = 3
MED = 4
SED = 5

# Test Purpose and Description:
# -----------------------------
# The purpose of this test case is to show that when the Leader is rebooted for a time period shorter than the leader timeout, it does not trigger network partitioning and remains the leader when it reattaches to the network.
#
# Test Topology:
# -------------
#    LEADER
#     |
#    ROUTER--SED
#     |  |
#   FED  MED
#

LISTEN_PORT = 12345

SED_RTT_MIN = 1000
SED_RTT_MAX = 120000


class TestTcp(thread_cert.TestCase):
    SUPPORT_NCP = False
    USE_MESSAGE_FACTORY = False

    TOPOLOGY = {
        LEADER: {
            'mode': 'rdn',
            'allowlist': [ROUTER]
        },
        ROUTER: {
            'mode': 'rdn',
            'router_selection_jitter': 1,
            'allowlist': [LEADER, FED, MED, SED]
        },
        FED: {
            'mode': 'rdn',
            'router_eligible': False,
            'allowlist': [ROUTER],
            'timeout': config.DEFAULT_CHILD_TIMEOUT,
        },
        MED: {
            'mode': 'rn',
            'router_eligible': False,
            'allowlist': [ROUTER],
            'timeout': config.DEFAULT_CHILD_TIMEOUT,
        },
        SED: {
            'mode': 'n',
            'allowlist': [ROUTER],
            'timeout': config.DEFAULT_CHILD_TIMEOUT,
        },
    }

    def _bootstrap(self, enable_fed=False, enable_med=False, enable_sed=False):
        self.nodes[LEADER].start()
        self.simulator.go(5)
        self.assertEqual(self.nodes[LEADER].get_state(), 'leader')

        self.nodes[ROUTER].start()
        self.simulator.go(5)
        self.assertEqual(self.nodes[ROUTER].get_state(), 'router')

        if enable_fed:
            self.nodes[FED].start()
            self.simulator.go(5)
            self.assertEqual(self.nodes[FED].get_state(), 'child')

        if enable_med:
            self.nodes[MED].start()
            self.simulator.go(5)
            self.assertEqual(self.nodes[MED].get_state(), 'child')

        if enable_sed:
            self.nodes[SED].start()
            self.nodes[SED].set_pollperiod(1000)
            self.simulator.go(5)
            self.assertEqual(self.nodes[SED].get_state(), 'child')

    def testSimple(self):
        self._bootstrap()
        client, server = ROUTER, LEADER
        self._setup_connection(client, server)
        self._verify_send_recv(client, server, 1024)
        self._shutdown_successively(client, server)

    def testBasic(self):
        self._test_basic(ROUTER, LEADER)

    def _test_basic(self, client, server, **kwargs):
        self._bootstrap(**kwargs)

        self._setup_connection(client, server)

        for datasize in (1, 10, 100, 1000, 10000):
            self._verify_send_recv(client, server, datasize)
            self._verify_send_recv(server, client, datasize)
            self.simulator.go(10)

        self._shutdown_successively(client, server)

    def testMultihop(self):
        self._test_basic(FED, LEADER, enable_fed=True)

    def testMED(self):
        self._test_basic(MED, LEADER, enable_med=True)

    def testSED(self):
        self._test_basic(SED, LEADER, enable_sed=True)

    def testZeroWindow(self):
        self._bootstrap()

        client, server = ROUTER, LEADER
        self._setup_connection(client, server)

        for i in range(13):
            data = b'A' * 200
            self.nodes[client]._tcp_send(data)
            self.simulator.go(10)

        self.simulator.go(1000)
        self.assertEqual('ESTABLISHED', self.nodes[server].tcp_state())
        # The client should not close socket due to Zero Window
        self.assertEqual('ESTABLISHED', self.nodes[client].tcp_state())

        while True:
            data = self.nodes[server].tcp_recv()
            if not data:
                break

            self.simulator.go(60)

        self.assertEqual('ESTABLISHED', self.nodes[server].tcp_state())
        self.assertEqual('ESTABLISHED', self.nodes[client].tcp_state())

        self._verify_send_recv(client, server, 100)
        self._shutdown_successively(client, server)

    def testSegmentSize(self):
        if not os.getenv("TEST_SEGMENT_SIZE"):
            self.skipTest("TEST_SEGMENT_SIZE is not set")

        self._bootstrap()

        client, server = ROUTER, LEADER
        self._setup_connection(client, server)

        for datasize in range(1, 512):
            self._verify_send_recv(client, server, datasize)

    def testServerShutdownFirst(self):
        self._bootstrap()

        client, server = ROUTER, LEADER
        self._setup_connection(client, server)
        self._shutdown_successively(server, client)

    def testShutdownTogether(self):
        self._bootstrap()

        client, server = ROUTER, LEADER
        self._setup_connection(client, server)
        self._shutdown_together(client, server)

    def testGoodput(self):
        self._test_goodput(nsteps=10, step_interval=0.1, segment_drop_prob=0)

    def testGoodputDrop10(self):
        self._test_goodput(nsteps=10, step_interval=1, segment_drop_prob=10)

    def _test_goodput(self, nsteps=10, step_interval=0.1, segment_drop_prob: int = 0):
        self._bootstrap()
        client, server = ROUTER, LEADER
        self._setup_connection(client, server)

        if segment_drop_prob:
            self.nodes[client].tcp_set_segment_random_drop_prob(segment_drop_prob)
            self.nodes[server].tcp_set_segment_random_drop_prob(segment_drop_prob)

        self.nodes[server].tcp_swallow()
        self.nodes[client].tcp_emit()

        last_swallow = 0
        for i in range(nsteps):
            self.simulator.go(step_interval)

            self.assertEqual('ESTABLISHED', self.nodes[client].tcp_state())
            self.assertEqual('ESTABLISHED', self.nodes[server].tcp_state())

            emit_counters = self.nodes[client].tcp_counters()
            swallow_counters = self.nodes[server].tcp_counters()
            self.assertGreaterEqual(emit_counters['emit'], swallow_counters['swallow'])
            print(f'TCP goodput: {(swallow_counters["swallow"] - last_swallow) / step_interval / 1000:.1f}KB/s')
            last_swallow = swallow_counters["swallow"]

        self._shutdown_successively(client, server, delay=100)

    def testRST(self):
        self._bootstrap()

        client, server = ROUTER, LEADER
        self._setup_connection(client, server)

        self.nodes[client].tcp_reset_next_segment()
        self.nodes[server].tcp_send(b'C')
        self.simulator.go(10)

        self.assertEqual('ESTABLISHED', self.nodes[client].tcp_state())
        self.assertEqual('CLOSED', self.nodes[server].tcp_state())

        self.nodes[client].tcp_close()
        self.nodes[server].tcp_close()
        self.simulator.go(10)

    def testAbort(self):
        self._bootstrap()

        client, server = ROUTER, LEADER
        self._setup_connection(client, server)
        self._abort(client, server)

        self._setup_connection(client, server)
        self._abort(server, client)

    def testTcpReliable(self):
        self._bootstrap()

        client, server = ROUTER, LEADER
        self.nodes[client].tcp_set_segment_random_drop_prob(10)
        self.nodes[server].tcp_set_segment_random_drop_prob(10)

        for i in range(10):
            print(f'TCP reliable test {i + 1}')
            self._test_tcp_reliable(client, server, datalen=1024)

        self._test_tcp_reliable(client, server, datalen=10 * 1024)

    def testOpenSocketMultipleTimes(self):
        self._bootstrap()

        client, server = ROUTER, LEADER
        self._setup_connection(client, server)
        self._verify_send_recv(client, server, 100)
        self._shutdown_successively(server, client, delay=100)

        self._setup_connection(client, server)
        self._verify_send_recv(client, server, 100)
        self._shutdown_successively(client, server)

        self._setup_connection(client, server)
        self._verify_send_recv(client, server, 100)
        self._shutdown_together(client, server)

        self._setup_connection(client, server)
        self._verify_send_recv(client, server, 100)
        self._shutdown_together(client, server)

    def testEcho(self):
        self._test_echo(10240)

    def testEchoLargeData(self):
        self._test_echo(102400)

    def testEchoRandomDrop(self):
        self._test_echo(10240, 10)

    def testEchoOneByte(self):
        self._bootstrap()

        client, server = ROUTER, LEADER
        self._setup_connection(client, server)
        self.nodes[server].tcp_echo()
        self.simulator.go(3)

        server_counters = self.nodes[server].tcp_counters()

        self.assertEqual(1, self.nodes[client].tcp_send(b'A'))
        self.simulator.go(1)

        self.assertEqual(b'A', self.nodes[client].tcp_recv())

        server_counters = self.nodes[server].tcp_counters() - server_counters
        print(server_counters)

        self.assertEqual(Counter({'echo': 1, 'tx_seg': 1, 'rx_seg': 1, 'rx_ack': 1}), server_counters)

        self._shutdown_successively(client, server)

    def testEchoForEver(self):
        if not os.getenv('ECHO_FOREVER'):
            self.skipTest('Skip this test since ECHO_FOREVER is not set')

        round = 1
        while True:
            print('=== TCP echo round %d ===' % round)
            self._test_echo(1024000, 10, report_speed_interval=10)
            round += 1

    def testTcpRetransmissionTimeout(self):
        self._bootstrap()
        client, server = ROUTER, LEADER
        self._setup_connection(client, server)
        self._verify_send_recv(client, server, 1000)

        self.nodes[server].tcp_set_segment_random_drop_prob(100)
        self.nodes[client].tcp_send(b'abc')
        self.simulator.go(1000)
        self.assertEqual("ESTABLISHED", self.nodes[server].tcp_state())
        # Client should have closed TCP socket due to retransmission timeout
        self.assertEqual("CLOSED", self.nodes[client].tcp_state())

    def testConnectInvalidAddr(self):
        self._bootstrap()
        client, server = ROUTER, LEADER
        self.nodes[client].tcp_init()
        self.assertRaises(Exception, lambda: self.nodes[client].tcp_connect("ff05::1", LISTEN_PORT))
        self.assertRaises(Exception, lambda: self.nodes[client].tcp_connect("::", LISTEN_PORT))
        self.assertRaises(Exception, lambda: self.nodes[client].tcp_connect(self.nodes[server].get_rloc(), 0))

    def testConnectWrongAddr(self):
        self._bootstrap()
        client, server = ROUTER, LEADER
        self.nodes[server].tcp_init()
        server_rloc = self.nodes[server].get_rloc()
        self.nodes[server].tcp_bind(server_rloc, LISTEN_PORT)
        self.nodes[server].tcp_listen()

        self.assertEqual("LISTEN", self.nodes[server].tcp_state())

        self.nodes[client].tcp_init()
        self.nodes[client].tcp_connect(self.nodes[server].get_mleid(), LISTEN_PORT)

        # client should send SYN, but server can't not receive it (wrong address)
        self.assertEqual("SYN-SENT", self.nodes[client].tcp_state())
        self.assertEqual("LISTEN", self.nodes[server].tcp_state())

        self.simulator.go(3)

        # server should send RST which reset client back to CLOSED state
        self.assertEqual("LISTEN", self.nodes[server].tcp_state())
        # Client should have closed TCP socket due to retransmission timeout
        self.assertEqual("CLOSED", self.nodes[client].tcp_state())

        self.nodes[client].tcp_init()
        self.nodes[client].tcp_connect(server_rloc, LISTEN_PORT + 1)

        # client should send SYN, but server can't not receive it (wrong port)
        self.assertEqual("SYN-SENT", self.nodes[client].tcp_state())
        self.assertEqual("LISTEN", self.nodes[server].tcp_state())

        self.simulator.go(3)

        # server should send RST which reset client back to CLOSED state
        self.assertEqual("LISTEN", self.nodes[server].tcp_state())
        # Client should have closed TCP socket due to retransmission timeout
        self.assertEqual("CLOSED", self.nodes[client].tcp_state())

    def testSockAddrs_Bind(self):
        self._bootstrap()
        client, server = ROUTER, LEADER
        self.nodes[server].tcp_init()

        IPV6_UNSPECIFIED = ipaddress.IPv6Address("::")

        sockinfo = self.nodes[server].tcp_info()
        self.assertEqual(sockinfo['LocalAddr'], (IPV6_UNSPECIFIED, 0))
        self.assertEqual(sockinfo['PeerAddr'], (IPV6_UNSPECIFIED, 0))

        server_rloc = self.nodes[server].get_rloc()
        self.nodes[server].tcp_bind(server_rloc, LISTEN_PORT)

        sockinfo = self.nodes[server].tcp_info()
        self.assertEqual(sockinfo['LocalAddr'], (ipaddress.IPv6Address(server_rloc), LISTEN_PORT))
        self.assertEqual(sockinfo['PeerAddr'], (IPV6_UNSPECIFIED, 0))

    def testSockAddrs(self):
        self._bootstrap()
        client, server = ROUTER, LEADER
        self.nodes[server].tcp_init()

        IPV6_UNSPECIFIED = ipaddress.IPv6Address("::")

        sockinfo = self.nodes[server].tcp_info()
        self.assertEqual(sockinfo['LocalAddr'], (IPV6_UNSPECIFIED, 0))
        self.assertEqual(sockinfo['PeerAddr'], (IPV6_UNSPECIFIED, 0))

        self.nodes[server].tcp_bind("::", LISTEN_PORT)

        sockinfo = self.nodes[server].tcp_info()
        self.assertEqual(sockinfo['LocalAddr'], (IPV6_UNSPECIFIED, LISTEN_PORT))
        self.assertEqual(sockinfo['PeerAddr'], (IPV6_UNSPECIFIED, 0))

        self.nodes[server].tcp_listen()

        self.nodes[client].tcp_init()
        sockinfo = self.nodes[client].tcp_info()
        self.assertEqual(sockinfo['LocalAddr'], (IPV6_UNSPECIFIED, 0))
        self.assertEqual(sockinfo['PeerAddr'], (IPV6_UNSPECIFIED, 0))

        server_ip = self.nodes[server].get_mleid()
        self.nodes[client].tcp_connect(server_ip, LISTEN_PORT)

        sockinfo = self.nodes[client].tcp_info()
        self.assertNotEqual(sockinfo['LocalAddr'][0], IPV6_UNSPECIFIED)
        self.assertNotEqual(sockinfo['LocalAddr'][1], 0)
        self.assertEqual(sockinfo['PeerAddr'], (ipaddress.IPv6Address(server_ip), LISTEN_PORT))

        client_addr, client_port = sockinfo['LocalAddr']

        self.simulator.go(1)

        self.assertEqual('ESTABLISHED', self.nodes[client].tcp_state())
        self.assertEqual('ESTABLISHED', self.nodes[server].tcp_state())

        sockinfo = self.nodes[server].tcp_info()
        self.assertEqual(sockinfo['LocalAddr'], (ipaddress.IPv6Address(server_ip), LISTEN_PORT))
        self.assertEqual(sockinfo['PeerAddr'], (client_addr, client_port))

    def _test_echo(self, datasize, drop_prob=0, report_speed_interval=10):
        self._bootstrap()

        client, server = ROUTER, LEADER
        self._setup_connection(client, server)
        self.nodes[server].tcp_echo()
        self.simulator.go(3)

        if drop_prob > 0:
            self.nodes[client].tcp_set_segment_random_drop_prob(drop_prob)
            self.nodes[server].tcp_set_segment_random_drop_prob(drop_prob)

        senddata = self._random_data(datasize)
        recvdata = b''
        leftdata = senddata
        time_step = 0.01

        kbps = [0, 0]
        while recvdata != senddata:
            n = self.nodes[client].tcp_send(leftdata)
            leftdata = leftdata[n:]

            data = self.nodes[client].tcp_recv()
            recvdata += data

            self.nodes[server].tcp_state()  # allow server to output CLI

            self.assertTrue(len(recvdata) <= len(senddata))
            self.assertTrue(senddata[:len(recvdata)] == recvdata)

            self.simulator.go(time_step)
            kbps[0] += time_step
            kbps[1] += len(data)

            if kbps[0] >= report_speed_interval:
                self.assertTrue(kbps[1] > 0, 'must be sending something in %d seconds' % kbps[0])
                print(f'TCP echo {kbps[1] / kbps[0] / 1000:.1f}KB/s')
                kbps = [0, 0]

        self.nodes[client].tcp_set_segment_random_drop_prob(0)
        self.nodes[server].tcp_set_segment_random_drop_prob(0)

        self._shutdown_successively(client, server)

    def _test_tcp_reliable(self, client, server, datalen):
        self._setup_connection(client, server, delay=100)

        self._verify_send_recv(client, server, datalen, timeout=100 + datalen / 10)
        self._shutdown_successively(client, server, delay=100)

    def _abort(self, one, other):
        self.nodes[one].tcp_abort()
        self.simulator.go(3)
        self.assertEqual('CLOSED', self.nodes[one].tcp_state())
        self.assertEqual('CLOSED', self.nodes[other].tcp_state())

    def _verify_send_recv(self, src, dst, datasize: int, timeout=1000):
        data = self._random_data(datasize)
        senddata = data
        recvdata = b''
        elapsed_time = 0
        time_step = 0.01

        while timeout > 0 and recvdata != senddata:
            nr = self.nodes[src].tcp_send(data)
            data = data[nr:]

            recvdata += self.nodes[dst].tcp_recv()

            if len(recvdata) > len(senddata) or senddata[:len(recvdata)] != recvdata:
                break

            self.simulator.go(time_step)
            timeout -= time_step
            elapsed_time += time_step

            print(
                f'TCP: send {(datasize - len(data)) / elapsed_time / 1000:.1f}KB/s, recv {len(recvdata) / elapsed_time / 1000:.1f}KB/s'
            )

        self.assertEqual(len(senddata), len(recvdata))
        self.assertEqual(senddata, recvdata)

    def _shutdown_successively(self, first: int, second: int, delay=10):
        self.nodes[first].tcp_close()
        self.simulator.go(delay)
        self.assertEqual('FIN-WAIT-2', self.nodes[first].tcp_state())

        self.nodes[second].tcp_close()
        self.simulator.go(delay)

        self.assertEqual('TIME-WAIT', self.nodes[first].tcp_state())
        self.assertEqual('CLOSED', self.nodes[second].tcp_state())

        self.simulator.go(240)
        self.assertEqual('CLOSED', self.nodes[first].tcp_state())

    def _shutdown_together(self, client, server):
        self.nodes[server].tcp_close()
        self.nodes[client].tcp_close()
        self.simulator.go(10)
        self.assertEqual('TIME-WAIT', self.nodes[server].tcp_state())
        self.assertEqual('TIME-WAIT', self.nodes[client].tcp_state())

        self.simulator.go(240)
        self.assertEqual('CLOSED', self.nodes[server].tcp_state())
        self.assertEqual('CLOSED', self.nodes[client].tcp_state())

    def _verify_tcp_state(self, nodeid, state: str, timeout=1000):
        while timeout > 0:
            self.simulator.go(10)
            timeout -= 10

            cur_state = self.nodes[nodeid].tcp_state()
            if cur_state == state:
                break

            print('Expect state: %s, current state: %s' % (state, cur_state))

    def _setup_connection(self, client, server, delay=10):
        self.nodes[server].tcp_init()
        self.nodes[client].tcp_init()

        self.assertEqual('CLOSED', self.nodes[server].tcp_state())
        self.assertEqual('CLOSED', self.nodes[client].tcp_state())
        self.assertEqual(0, self.nodes[server].tcp_send(b'abc'))
        self.assertEqual(0, self.nodes[client].tcp_send(b'abc'))

        SERVER_RLOC = self.nodes[server].get_ip6_address(config.ADDRESS_TYPE.RLOC)
        self.nodes[server].tcp_bind('::', LISTEN_PORT)
        self.nodes[server].tcp_listen()

        self.assertEqual('LISTEN', self.nodes[server].tcp_state())
        self.assertEqual(0, self.nodes[server].tcp_send(b'abc'))
        self.assertEqual(0, self.nodes[client].tcp_send(b'abc'))

        self.nodes[client].tcp_connect(SERVER_RLOC, LISTEN_PORT)
        self.simulator.go(delay)

        self.assertEqual('ESTABLISHED', self.nodes[client].tcp_state())
        self.assertEqual('ESTABLISHED', self.nodes[server].tcp_state())

        if client == SED:
            self.nodes[server].tcp_config_rtt(SED_RTT_MIN, SED_RTT_MAX)

        if server == SED:
            self.nodes[client].tcp_config_rtt(SED_RTT_MIN, SED_RTT_MAX)

    def _random_data(self, datasize: int) -> bytes:
        return bytes([ord('A') + (i % 26) for i in range(datasize)])


if __name__ == '__main__':
    unittest.main()
