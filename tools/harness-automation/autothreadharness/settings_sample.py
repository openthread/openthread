#!/usr/bin/env python
#
# Copyright (c) 2016, Nest Labs, Inc.
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are met:
# 1. Redistributions of source code must retain the above copyright
#    notice, this list of conditions and the following disclaimer.
# 2. Redistributions in binary form must reproduce the above copyright
#    notice, this list of conditions and the following disclaimer in the
#    documentation and/or other materials provided with the distribution.
# 3. Neither the name of the copyright holder nor the
#    names of its contributors may be used to endorse or promote products
#    derived from this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
# AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
# IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
# ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
# LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
# CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
# SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
# INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
# CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
# ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
# POSSIBILITY OF SUCH DAMAGE.
#


DUT_DEVICE = 'COM16'
"""str: Serial port of the DUT"""

DUT_MANUFACTURER = 'Open Thread'
"""str: Manufacturer of the DUT"""

THREAD_CHANNEL = 18
"""int: Thread channel"""

THREAD_PANID = '0xface'
"""str: Thread PAN ID"""

THREAD_NETWORKNAME = 'GRL'
"""str: Thread network name"""

THREAD_EXTPANID = '000db80000000000'
"""str: Thread extended PAN ID"""

THREAD_CHILD_TIMEOUT = 100
"""int: Child timeout in seconds"""

THREAD_SED_POLLING_INTERVAL = 100
"""int: SED polling interval in seconds"""

HARNESS_HOME = 'C:\\GRL\\Thread1.1'
"""str: Harness installation path"""

HARNESS_URL = 'http://127.0.0.1:8000'
"""str: Harness front-end url"""

APC_HOST = None
"""str: PDU controller host.

Keep this None if no APC PDU available.
"""

APC_OUTLET = 1
"""int: PDU outlet"""

TESTER_NAME = 'Thread Open'
"""str: Who are you"""

TESTER_REMARKS = 'OpenThread is great'
"""str: Any comments in the final PDF"""

GOLDEN_DEVICE_TYPE = 'ARM'
"""str: Golden device type"""

GOLDEN_DEVICES = []
"""[str]: Golden devices list.

It should be something like ['COM1', 'COM2'] on Windows
"""

OUTPUT_PATH = '.\\output'
"""str: Path to store results and logs"""
