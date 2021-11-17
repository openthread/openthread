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

from zeroconf import Zeroconf, ServiceInfo
import sys
import time

# USAGE: python3 publish_mdns-service.py
# This script registers a service by MDNS.


def parse_addresses(arg: str):
    return arg.split(',')


def parse_properties(arg: str):
    entries = arg.split(' ')
    print(entries)
    properties = {}
    for entry in entries:
        if entry:
            key, value = entry.split('=')
            properties[key] = value
    return properties


if __name__ == '__main__':
    instance = sys.argv[1]
    service = sys.argv[2]
    hostname = sys.argv[3]
    addresses = parse_addresses(sys.argv[4])
    port = int(sys.argv[5])
    properties = parse_properties(sys.argv[6])
    timeout = int(sys.argv[7])

    zeroconf = Zeroconf()
    service_info = ServiceInfo(f'{service}.local.',
                               f'{instance}.{service}.local.',
                               properties=properties,
                               server=hostname,
                               port=port,
                               parsed_addresses=addresses)
    zeroconf.register_service(service_info)
    time.sleep(timeout)
    zeroconf.close()
