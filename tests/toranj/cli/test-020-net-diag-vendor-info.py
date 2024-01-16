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
# Test description: Network Diagnostics Vendor Name, Vendor Model, Vendor SW Version TLVs.
#
# Network topology
#
#      r1 ---- r2
#

test_name = __file__[:-3] if __file__.endswith('.py') else __file__
print('-' * 120)
print('Starting \'{}\''.format(test_name))

# -----------------------------------------------------------------------------------------------------------------------
# Creating `cli.Node` instances

speedup = 40
cli.Node.set_time_speedup_factor(speedup)

r1 = cli.Node()
r2 = cli.Node()

# -----------------------------------------------------------------------------------------------------------------------
# Form topology

r1.form('netdiag-vendor')
r2.join(r1)

verify(r1.get_state() == 'leader')
verify(r2.get_state() == 'router')

# -----------------------------------------------------------------------------------------------------------------------
# Test Implementation

VENDOR_NAME_TLV = 25
VENDOR_MODEL_TLV = 26
VENDOR_SW_VERSION_TLV = 27
THREAD_STACK_VERSION_TLV = 28
MLE_COUNTERS_TLV = 34

#- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
# Check setting vendor name, model, ans sw version

r1.set_vendor_name('nest')
r1.set_vendor_model('marble')
r1.set_vendor_sw_version('ot-1.3.1')

verify(r1.get_vendor_name() == 'nest')
verify(r1.get_vendor_model() == 'marble')
verify(r1.get_vendor_sw_version() == 'ot-1.3.1')

#- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
# Check invalid names (too long)

# Vendor name should accept up to 32 chars

r2.set_vendor_name('01234567890123456789012345678901')  # 32 chars

errored = False

try:
    r2.set_vendor_name('012345678901234567890123456789012')  # 33 chars
except cli.CliError as e:
    verify(e.message == 'InvalidArgs')
    errored = True

verify(errored)

# Vendor model should accept up to 32 chars

r2.set_vendor_model('01234567890123456789012345678901')  # 32 chars

errored = False

try:
    r2.set_vendor_model('012345678901234567890123456789012')  # 33 chars
except cli.CliError as e:
    verify(e.message == 'InvalidArgs')
    errored = True

verify(errored)

# Vendor SW version should accept up to 16 chars

r2.set_vendor_sw_version('0123456789012345')  # 16 chars

errored = False

try:
    r2.set_vendor_sw_version('01234567890123456')  # 17 chars
except cli.CliError as e:
    verify(e.message == 'InvalidArgs')
    errored = True

verify(errored)

#- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
# Perform net diag query

r1_rloc = r1.get_rloc_ip_addr()
r2_rloc = r2.get_rloc_ip_addr()

# Get vendor name (TLV 27)

result = r2.cli('networkdiagnostic get', r1_rloc, VENDOR_NAME_TLV)
verify(len(result) == 2)
verify(result[1].startswith("Vendor Name:"))
verify(result[1].split(':')[1].strip() == r1.get_vendor_name())

# Get vendor model (TLV 28)

result = r2.cli('networkdiagnostic get', r1_rloc, VENDOR_MODEL_TLV)
verify(len(result) == 2)
verify(result[1].startswith("Vendor Model:"))
verify(result[1].split(':')[1].strip() == r1.get_vendor_model())

# Get vendor sw version (TLV 29)

result = r2.cli('networkdiagnostic get', r1_rloc, VENDOR_SW_VERSION_TLV)
verify(len(result) == 2)
verify(result[1].startswith("Vendor SW Version:"))
verify(result[1].split(':')[1].strip() == r1.get_vendor_sw_version())

# Get thread stack version (TLV 30)

result = r2.cli('networkdiagnostic get', r1_rloc, THREAD_STACK_VERSION_TLV)
verify(len(result) == 2)
verify(result[1].startswith("Thread Stack Version:"))
verify(r1.get_version().startswith(result[1].split(':', 1)[1].strip()))

# Get all three TLVs (now from `r1`)

result = r1.cli('networkdiagnostic get', r2_rloc, VENDOR_NAME_TLV, VENDOR_MODEL_TLV, VENDOR_SW_VERSION_TLV,
                THREAD_STACK_VERSION_TLV)
verify(len(result) == 5)
for line in result[1:]:
    if line.startswith("Vendor Name:"):
        verify(line.split(':')[1].strip() == r2.get_vendor_name())
    elif line.startswith("Vendor Model:"):
        verify(line.split(':')[1].strip() == r2.get_vendor_model())
    elif line.startswith("Vendor SW Version:"):
        verify(line.split(':')[1].strip() == r2.get_vendor_sw_version())
    elif line.startswith("Thread Stack Version:"):
        verify(r2.get_version().startswith(line.split(':', 1)[1].strip()))
    else:
        verify(False)

result = r2.cli('networkdiagnostic get', r1_rloc, MLE_COUNTERS_TLV)
print(len(result) >= 1)
verify(result[1].startswith("MLE Counters:"))

# -----------------------------------------------------------------------------------------------------------------------
# Test finished

cli.Node.finalize_all_nodes()

print('\'{}\' passed.'.format(test_name))
