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

# -----------------------------------------------------------------------------------------------------------------------
# Test description: MAC scan operation

test_name = __file__[:-3] if __file__.endswith('.py') else __file__
print('-' * 120)
print('Starting \'{}\''.format(test_name))


# -----------------------------------------------------------------------------------------------------------------------
def verify_scan_result_conatins_nodes(scan_result, nodes):
    table = cli.Node.parse_table(scan_result)
    verify(len(table) >= len(nodes))
    for node in nodes:
        ext_addr = node.get_ext_addr()
        for entry in table:
            if entry['MAC Address'] == ext_addr:
                verify(int(entry['PAN'], 16) == int(node.get_panid(), 16))
                verify(int(entry['Ch']) == int(node.get_channel()))
                break
        else:
            verify(False)


# -----------------------------------------------------------------------------------------------------------------------
# Creating `cli.Nodes` instances

speedup = 10
cli.Node.set_time_speedup_factor(speedup)

node1 = cli.Node()
node2 = cli.Node()
node3 = cli.Node()
node4 = cli.Node()
node5 = cli.Node()
scanner = cli.Node()

nodes = [node1, node2, node3, node4, node5]

# -----------------------------------------------------------------------------------------------------------------------
# Test implementation

node1.form('net1', panid=0x1111, channel=12)
node2.form('net2', panid=0x2222, channel=13)
node3.form('net3', panid=0x3333, channel=14)
node4.form('net4', panid=0x4444, channel=15)
node5.form('net5', panid=0x5555, channel=16)

# MAC scan

# Scan on all channels, should see all nodes
verify_scan_result_conatins_nodes(scanner.cli('scan'), nodes)

# Scan on channel 12 only, should only see node1
verify_scan_result_conatins_nodes(scanner.cli('scan 12'), [node1])

# Scan on channel 20 only, should see no result
verify_scan_result_conatins_nodes(scanner.cli('scan 20'), [])

# MLE Discover scan

scanner.interface_up()

verify_scan_result_conatins_nodes(scanner.cli('discover'), nodes)
verify_scan_result_conatins_nodes(scanner.cli('scan 12'), [node1])
verify_scan_result_conatins_nodes(scanner.cli('scan 20'), [])

scanner.form('scanner')
verify_scan_result_conatins_nodes(scanner.cli('discover'), nodes)

# -----------------------------------------------------------------------------------------------------------------------
# Test finished

cli.Node.finalize_all_nodes()

print('\'{}\' passed.'.format(test_name))
