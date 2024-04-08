#!/usr/bin/env python3
#
#  Copyright (c) 2022, The OpenThread Authors.
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

from cli import verify
from cli import verify_within
import cli
import time

# -----------------------------------------------------------------------------------------------------------------------
# Test description:
#
# Verifies `ChannelManager` channel selection procedure

test_name = __file__[:-3] if __file__.endswith('.py') else __file__
print('-' * 120)
print('Starting \'{}\''.format(test_name))

# -----------------------------------------------------------------------------------------------------------------------
# Creating `cli.Node` instances

# Run the test with 10,000 time speedup factor
speedup = 10000
cli.Node.set_time_speedup_factor(speedup)

node = cli.Node()

# -----------------------------------------------------------------------------------------------------------------------
# Form topology

node.form('chan-sel', channel=24)

verify(node.get_state() == 'leader')

# -----------------------------------------------------------------------------------------------------------------------
# Test Implementation

channel = 24


def check_channel():
    verify(int(node.get_channel()) == channel)


delay = int(node.cli('channel manager delay')[0])
# add kRequestStartJitterInterval=10000ms to expected channel manager delay
delay += 10 / speedup

check_channel()

all_channels_mask = int('0x7fff800', 0)
chan_12_to_15_mask = int('0x000f000', 0)
chan_15_to_17_mask = int('0x0038000', 0)

# Set supported channel mask to be all channels
node.cli('channel manager supported', all_channels_mask)

# Sleep for 4.5 second with speedup factor of 10,000 this is more than 12
# hours. We sleep instead of immediately checking the sample counter in
# order to not add more actions/events into simulation (specially since
# we are running at very high speedup).
time.sleep(4.5)

result = cli.Node.parse_list(node.cli('channel monitor')[:5])
verify(result['enabled'] == '1')
verify(int(result['count']) > 970)

# Issue a channel-select with quality check enabled, and verify that no
# action is taken.

node.cli('channel manager select 0')
result = cli.Node.parse_list(node.cli('channel manager'))
verify(result['channel'] == '0')

# Issue a channel-select with quality check disabled, verify that channel
# is switched to channel 11.

node.cli('channel manager select 1')
result = cli.Node.parse_list(node.cli('channel manager'))
verify(result['channel'] == '11')
channel = 11
verify_within(check_channel, delay)

# Set channels 12-15 as favorable and request a channel select, verify
# that channel is switched to 12.
#
# Even though 11 would be best, quality difference between 11 and 12
# is not high enough for selection algorithm to pick an unfavored
# channel.

node.cli('channel manager favored', chan_12_to_15_mask)

channel = 25
node.cli('channel manager change', channel)
verify_within(check_channel, delay)

node.cli('channel manager select 1')
result = cli.Node.parse_list(node.cli('channel manager'))
verify(result['channel'] == '12')
channel = 12
verify_within(check_channel, delay)

# Set channels 15-17 as favorables and request a channel select,
# verify that channel is switched to 11.
#
# This time the quality difference between 11 and 15 should be high
# enough for selection algorithm to pick the best though unfavored
# channel (i.e., channel 11).

channel = 25
node.cli('channel manager change', channel)
verify_within(check_channel, delay)

node.cli('channel manager favored', chan_15_to_17_mask)

node.cli('channel manager select 1')
result = cli.Node.parse_list(node.cli('channel manager'))
verify(result['channel'] == '11')
channel = 11
verify_within(check_channel, delay)

# Set channels 12-15 as favorable and request a channel select, verify
# that channel is not switched.

node.cli('channel manager favored', chan_12_to_15_mask)

node.cli('channel manager select 1')

result = cli.Node.parse_list(node.cli('channel manager'))
verify(result['channel'] == '11')
channel = 11
verify_within(check_channel, delay)

# Starting from channel 12 and issuing a channel select (which would
# pick 11 as best channel). However, since quality difference between
# current channel 12 and new best channel 11 is not large enough, no
# action should be taken.

channel = 12
node.cli('channel manager change', channel)
verify_within(check_channel, delay)

node.cli('channel manager favored', all_channels_mask)

node.cli('channel manager select 1')
result = cli.Node.parse_list(node.cli('channel manager'))
verify(result['channel'] == '12')
verify_within(check_channel, delay)

# -----------------------------------------------------------------------------------------------------------------------
# Auto Select

# Set channel manager cca failure rate threshold to 0
# as we cannot control cca success in simulation
node.cli('channel manager threshold 0')

# Set short channel selection interval to speedup
interval = 30
node.cli(f'channel manager interval {interval}')

# Set channels 15-17 as favorable and request a channel select, verify
# that channel is switched to 11.

channel = 25
node.cli('channel manager change', channel)
verify_within(check_channel, delay)
node.cli('channel manager favored', chan_15_to_17_mask)

# Active auto channel selection
node.cli('channel manager auto 1')

channel = 11
result = cli.Node.parse_list(node.cli('channel manager'))
verify(result['auto'] == '1')
verify(result['channel'] == str(channel))

verify_within(check_channel, delay)

# while channel selection timer is running change to channel 25,
# set channels 12-15 as favorable, wait for auto channel selection
# and verify that channel is switched to 12.

node.cli('channel manager favored', chan_12_to_15_mask)
channel = 25
node.cli('channel manager change', channel)

# wait for timeout of auto selection timer
time.sleep(2 * interval / speedup)

channel = 12
result = cli.Node.parse_list(node.cli('channel manager'))
verify(result['channel'] == str(channel))

verify_within(check_channel, delay)

# -----------------------------------------------------------------------------------------------------------------------
# Test finished

cli.Node.finalize_all_nodes()

print('\'{}\' passed.'.format(test_name))
