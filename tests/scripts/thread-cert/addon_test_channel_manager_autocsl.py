#!/usr/bin/env python3
#
#  Copyright (c) 2023, The OpenThread Authors.
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

import unittest

import config
import mle
import thread_cert
from pktverify import consts

LEADER = 1
ED = 2
SSED = 3


class SSED_CslChannelManager(thread_cert.TestCase):
    TOPOLOGY = {
        LEADER: {
            'version': '1.2',
        },
        ED: {
            'version': '1.2',
            'is_mtd': False,
            'mode': 'rn',
        },
        SSED: {
            'version': '1.2',
            'is_mtd': True,
            'mode': '-',
        },
    }
    """All nodes are created with default configurations"""

    def test(self):

        self.nodes[SSED].set_csl_period(consts.CSL_DEFAULT_PERIOD)
        self.nodes[SSED].set_csl_timeout(consts.CSL_DEFAULT_TIMEOUT)

        self.nodes[SSED].get_csl_info()

        self.nodes[LEADER].start()
        self.simulator.go(config.LEADER_STARTUP_DELAY)
        self.assertEqual(self.nodes[LEADER].get_state(), 'leader')
        channel = self.nodes[LEADER].get_channel()

        self.nodes[SSED].start()
        self.simulator.go(7)
        self.assertEqual(self.nodes[SSED].get_state(), 'child')

        csl_channel = 0
        csl_config = self.nodes[SSED].get_csl_info()
        self.assertTrue(int(csl_config['channel']) == csl_channel)
        self.assertTrue(csl_config['period'] == '500000us')

        print('SSED rloc:%s' % self.nodes[SSED].get_rloc())
        self.assertTrue(self.nodes[LEADER].ping(self.nodes[SSED].get_rloc()))

        # let channel monitor collect >970 sample counts
        self.simulator.go(980 * 41)
        results = self.nodes[SSED].get_channel_monitor_info()
        self.assertTrue(int(results['count']) > 970)

        # Configure channel manager channel masks
        # Set cca threshold to 0 as we cannot change cca assessment in simulation.
        # and shorten interval to speedup test
        all_channels_mask = int('0x7fff800', 0)
        chan_12_to_15_mask = int('0x000f000', 0)
        interval = 30
        self.nodes[SSED].set_channel_manager_supported(all_channels_mask)
        self.nodes[SSED].set_channel_manager_favored(chan_12_to_15_mask)
        self.nodes[SSED].set_channel_manager_cca_threshold('0x0000')
        self.nodes[SSED].set_channel_manager_interval(interval)

        # enable channel manager auto-select and check
        # network channel is not changed by channel manager on SSED
        # and also csl_channel is unchanged
        self.nodes[SSED].set_channel_manager_auto_enable(True)
        self.simulator.go(interval + 1)
        results = self.nodes[SSED].get_channel_manager_config()
        self.assertTrue(int(results['auto']) == 1)
        self.assertTrue(results['cca threshold'] == '0x0000')
        self.assertTrue(int(results['interval']) == interval)
        self.assertTrue('11-26' in results['supported'])
        self.simulator.go(1)
        self.assertTrue(self.nodes[SSED].get_channel() == channel)
        csl_config = self.nodes[SSED].get_csl_info()
        self.assertTrue(int(csl_config['channel']) == csl_channel)

        # check SSED can change csl channel
        csl_channel = 25
        self.flush_all()
        self.nodes[SSED].set_csl_channel(csl_channel)
        self.simulator.go(1)
        ssed_messages = self.simulator.get_messages_sent_by(SSED)
        self.assertIsNotNone(ssed_messages.next_mle_message(mle.CommandType.CHILD_UPDATE_REQUEST))
        self.simulator.go(1)
        csl_config = self.nodes[SSED].get_csl_info()
        self.assertTrue(int(csl_config['channel']) == csl_channel)
        self.simulator.go(1)
        self.assertTrue(self.nodes[LEADER].ping(self.nodes[SSED].get_rloc()))

        # enable channel manager autocsl-select in addition
        # and check csl channel changed to best favored channel 12
        csl_channel = 12
        self.nodes[SSED].set_channel_manager_autocsl_enable(True)
        self.simulator.go(interval + 1)
        results = self.nodes[SSED].get_channel_manager_config()
        self.assertTrue(int(results['autocsl']) == 1)
        self.assertTrue(int(results['channel']) == csl_channel)
        csl_config = self.nodes[SSED].get_csl_info()
        self.assertTrue(int(csl_config['channel']) == csl_channel)
        self.simulator.go(1)
        self.assertTrue(self.nodes[LEADER].ping(self.nodes[SSED].get_rloc()))


if __name__ == '__main__':
    unittest.main()
