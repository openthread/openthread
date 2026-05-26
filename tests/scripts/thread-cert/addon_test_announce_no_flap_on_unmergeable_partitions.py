#!/usr/bin/env python3
#
#  Copyright (c) 2024, The OpenThread Authors.
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
# Regression test: two FTDs on the same channel and PAN ID, with
# identical network credentials but different Active Dataset
# Timestamps, and an RF link too weak to merge their partitions
# (Advertisement rejected with LinkMarginLow at mle_router.cpp), must
# not enter an endless role flap from MLE Announce processing.
#
# Without the fix in `Mle::AnnounceHandler::HandleAnnounce`, the
# lower-timestamp node loops Leader -> Detached -> Disabled -> Detached
# -> Leader as it pointlessly Stop()/Start()s on receipt of each
# Announce, even though the announced channel/PAN ID already match its
# current MAC parameters. With the fix, both nodes remain stable
# Leaders of their separate partitions.

import unittest

import thread_cert

LEADER_OLD = 1
LEADER_NEW = 2

CHANNEL = 11
PANID = 0xface
EXTPANID = 'dead00beef00cafe'
NETWORK_KEY = '00112233445566778899aabbccddeeff'
MESH_LOCAL_PREFIX = 'fd00:0db8::'

# Both datasets are identical except for the Active Timestamp; the
# higher value drives Announces that, without the fix, restart the
# lower-timestamp peer.
TIMESTAMP_OLD = 1
TIMESTAMP_NEW = 1000

# RSSI 1 dB under the default partition-merge link-margin threshold
# (OPENTHREAD_CONFIG_MLE_PARTITION_MERGE_MARGIN_MIN = 10 with the
# simulation radio receive sensitivity / noise floor at -100 dBm, see
# examples/platforms/simulation/radio.c SIM_RECEIVE_SENSITIVITY).
# Advertisements from a different partition are dropped with
# OT_ERROR_LINK_MARGIN_LOW at mle_router.cpp:1172; Announce processing
# is not link-margin gated.
WEAK_LINK_RSSI = -91

# Long enough to cover at least one AnnounceSender trickle interval
# (OPENTHREAD_CONFIG_ANNOUNCE_SENDER_INTERVAL = 688 s by default) plus
# headroom for reactive kSendAnnouceBack exchanges to amplify into a
# visible flap if the bug is present.
SIM_WINDOW_S = 20 * 60


class AnnounceNoFlapOnUnmergeablePartitions(thread_cert.TestCase):
    SUPPORT_NCP = False

    # Two FTDs with identical credentials but different Active
    # Timestamps.  No `allowlist` here -- nodes are isolated and
    # linked manually in `test()` because the framework only knows
    # how to wire allowlist before `start()`, and we need each node
    # to attach as Leader of its own partition first.
    TOPOLOGY = {
        LEADER_OLD: {
            'name': 'LEADER_OLD',
            'active_dataset': {
                'timestamp': TIMESTAMP_OLD,
                'channel': CHANNEL,
                'panid': PANID,
                'extended_panid': EXTPANID,
                'network_key': NETWORK_KEY,
                'mesh_local_prefix': MESH_LOCAL_PREFIX,
            },
            'mode': 'rdn',
        },
        LEADER_NEW: {
            'name': 'LEADER_NEW',
            'active_dataset': {
                'timestamp': TIMESTAMP_NEW,
                'channel': CHANNEL,
                'panid': PANID,
                'extended_panid': EXTPANID,
                'network_key': NETWORK_KEY,
                'mesh_local_prefix': MESH_LOCAL_PREFIX,
            },
            'mode': 'rdn',
        },
    }

    def test(self):
        # Bring up LEADER_OLD; it becomes Leader of its own partition.
        self.nodes[LEADER_OLD].start()
        self.simulator.go(10)
        self.assertEqual(self.nodes[LEADER_OLD].get_state(), 'leader')

        # Isolate LEADER_NEW radio before bringing it up so it cannot
        # hear LEADER_OLD and is forced to form its own partition
        # rather than attach as a child.
        self.nodes[LEADER_NEW].radiofilter_enable()
        self.nodes[LEADER_NEW].start()
        self.simulator.go(10)
        self.assertEqual(self.nodes[LEADER_NEW].get_state(), 'leader')

        partition_old = self.nodes[LEADER_OLD].get_partition_id()
        partition_new = self.nodes[LEADER_NEW].get_partition_id()
        self.assertNotEqual(partition_old, partition_new, 'Test setup failed: nodes share a partition ID before the '
                            'weak link is wired up.')

        # Wire a weak link between the two leaders. Cross-traffic now
        # flows but Advertisements between different partitions fail
        # the link-margin check (mle_router.cpp:1172). Announces are
        # not link-margin gated, so they reach the peer and -- without
        # the fix -- trigger the role-flap path.
        addr_old = self.nodes[LEADER_OLD].get_addr64()
        addr_new = self.nodes[LEADER_NEW].get_addr64()

        self.nodes[LEADER_OLD].add_allowlist(addr_new, rssi=WEAK_LINK_RSSI)
        self.nodes[LEADER_OLD].enable_allowlist()

        self.nodes[LEADER_NEW].add_allowlist(addr_old, rssi=WEAK_LINK_RSSI)
        self.nodes[LEADER_NEW].enable_allowlist()
        self.nodes[LEADER_NEW].radiofilter_disable()

        # Let Announces flow. AnnounceSender trickle interval
        # (~11.5 min default) plus jitter plus reactive amplification
        # means the buggy flap, if present, becomes visible well
        # inside this window.
        self.simulator.go(SIM_WINDOW_S)

        # Both nodes must still be Leader of their original partitions.
        # Any Stop()/Start() flap from buggy Announce handling would
        # have allocated a fresh partition ID on re-attach.
        self.assertEqual(self.nodes[LEADER_OLD].get_state(), 'leader',
                         'LEADER_OLD no longer leader -- role flapped during the '
                         'test window.')
        self.assertEqual(self.nodes[LEADER_NEW].get_state(), 'leader',
                         'LEADER_NEW no longer leader -- role flapped during the '
                         'test window.')

        self.assertEqual(
            self.nodes[LEADER_OLD].get_partition_id(), partition_old,
            'LEADER_OLD re-created its partition during the test '
            'window (indicates Stop()/Start() flap from buggy '
            'Announce handling on attached nodes with matching '
            'channel/PAN ID).')
        self.assertEqual(
            self.nodes[LEADER_NEW].get_partition_id(), partition_new,
            'LEADER_NEW re-created its partition during the test '
            'window (indicates Stop()/Start() flap from buggy '
            'Announce handling on attached nodes with matching '
            'channel/PAN ID).')


if __name__ == '__main__':
    unittest.main()
