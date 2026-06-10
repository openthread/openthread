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
# Test description: Test MLE discover scan with nodes supporting different radios
#

test_name = __file__[:-3] if __file__.endswith('.py') else __file__
print('-' * 120)
print('Starting \'{}\''.format(test_name))

# -----------------------------------------------------------------------------------------------------------------------
# Creating `cli.Node` instances

speedup = 10
cli.Node.set_time_speedup_factor(speedup)

n1 = cli.Node(cli.RADIO_15_4)
n2 = cli.Node(cli.RADIO_TREL)
n3 = cli.Node(cli.RADIO_15_4_TREL)
s1 = cli.Node(cli.RADIO_15_4)
s2 = cli.Node(cli.RADIO_TREL)
s3 = cli.Node(cli.RADIO_15_4_TREL)

# -----------------------------------------------------------------------------------------------------------------------
# Build network topology

n1.form("n1", channel='20')
n2.form("n2", channel='21')
n3.form("n3", channel='22')

# -----------------------------------------------------------------------------------------------------------------------
# Test Implementation

verify(n1.multiradio_get_radios() == '[15.4]')
verify(n2.multiradio_get_radios() == '[TREL]')
verify(n3.multiradio_get_radios() == '[15.4, TREL]')
verify(s1.multiradio_get_radios() == '[15.4]')
verify(s2.multiradio_get_radios() == '[TREL]')
verify(s3.multiradio_get_radios() == '[15.4, TREL]')


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


# - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
# Scan by scanner nodes (no network)

s1.interface_up()
s2.interface_up()
s3.interface_up()

# Scan by s1 (15.4 only), expect to see n1(15.4) and n3(15.4+trel)

verify_scan_result_conatins_nodes(s1.cli('discover'), [n1, n3])

# Scan by s2 (trel only), expect to see n2(trel) and n3(15.4+trel)
verify_scan_result_conatins_nodes(s2.cli('discover'), [n2, n3])

# Scan by s3 (trel+15.4), expect to see all nodes
verify_scan_result_conatins_nodes(s3.cli('discover'), [n1, n2, n3])

# - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
# Scan by the nodes

# Scan by n1 (15.4 only), expect to see only n3(15.4+trel)
verify_scan_result_conatins_nodes(n1.cli('discover'), [n3])

# Scan by n2 (trel only), expect to see only n3(15.4+trel)
verify_scan_result_conatins_nodes(n2.cli('discover'), [n3])

# Scan by n3 (15.4+trel), expect to see n1(15.4) and n2(trel)
verify_scan_result_conatins_nodes(n3.cli('discover'), [n1, n2])

# -----------------------------------------------------------------------------------------------------------------------
# Test finished

cli.Node.finalize_all_nodes()

print('\'{}\' passed.'.format(test_name))
