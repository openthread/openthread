"""
  Copyright (c) 2024, The OpenThread Authors.
  All rights reserved.

  Redistribution and use in source and binary forms, with or without
  modification, are permitted provided that the following conditions are met:
  1. Redistributions of source code must retain the above copyright
     notice, this list of conditions and the following disclaimer.
  2. Redistributions in binary form must reproduce the above copyright
     notice, this list of conditions and the following disclaimer in the
     documentation and/or other materials provided with the distribution.
  3. Neither the name of the copyright holder nor the
     names of its contributors may be used to endorse or promote products
     derived from this software without specific prior written permission.

  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
  ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
  LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
  CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
  POSSIBILITY OF SUCH DAMAGE.
"""

from bleak import BleakScanner
from bleak.backends.device import BLEDevice
from bbtc import BBTC_SERVICE_UUID
from typing import Optional


async def find_first_by_name(name):
    match_name = lambda dev, adv_data: name == dev.name
    device = await BleakScanner.find_device_by_filter(match_name)
    return device


async def find_first_by_mac(mac):
    match_mac = lambda dev, adv_data: mac.upper() == dev.address
    device = await BleakScanner.find_device_by_filter(match_mac)
    return device


async def scan_tcat_devices(adapter: Optional[str] = None):
    scanner = BleakScanner()
    tcat_devices: list[BLEDevice] = []
    discovered_devices = await scanner.discover(return_adv=True,
                                                service_uuids=[BBTC_SERVICE_UUID.lower()],
                                                adapter=adapter)
    for _, (device, _) in discovered_devices.items():
        tcat_devices.append(device)

    return tcat_devices
