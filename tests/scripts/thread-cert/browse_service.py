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
#
import sys
import json
import time
import ipaddress
import zeroconf
from zeroconf import Zeroconf, ServiceBrowser

# USAGE: python3 browse_service.py <service_type> <timeout (seconds)>
# The script will print a JSON array containing all the discovered service instances
# For example:
# python3 tests/scripts/thread-cert/browse_service.py _meshcop._udp.local. 3
#
#  {
#    "fullname": "OpenThread BorderRouter._meshcop._udp.local.",
#    "instance": "OpenThread BorderRouter",
#    "name": "_meshcop._udp.local.",
#    "addresses": [],
#    "port": 49154,
#    "weight": 0,
#    "priority": 0,
#    "host_fullname": "host-5.local.",
#    "host": "1eec0e6339fd-5",
#    "txt": {
#      "rv": "31",
#      "nn": "6f742d6272312d31",
#       ...
#    }
#  },

services = []


class Listener:

    def add_service(self, zeroconf: Zeroconf, type: str, name: str):
        services.append([type, name])

    def remove_service(self, zeroconf: Zeroconf, type: str, name: str):
        pass

    def update_service(self, zeroconf: Zeroconf, type: str, name: str):
        pass


def to_dict(service: zeroconf.ServiceInfo):
    if not hasattr(service, 'addresses'):
        service.addresses = []
    return {
        'fullname': service.name,
        'instance': service.get_name(),
        'name': service.type,
        'addresses': [str(ipaddress.ip_address(addr)) for addr in service.addresses],
        'port': service.port,
        'weight': service.weight,
        'priority': service.priority,
        'host_fullname': service.server,
        'host': service.server.rstrip('.local.'),
        'txt': dict([(key.decode('raw_unicode_escape'), value.hex()) for key, value in service.properties.items()]),
    }


def main():
    global services

    args = sys.argv[1:]
    service_type = args[0]
    timeout = int(args[1])

    zeroconf = Zeroconf()
    listener = Listener()
    browser = ServiceBrowser(zeroconf, f'{service_type}', listener)
    time.sleep(timeout)
    services = [zeroconf.get_service_info(type, name) for type, name in services]
    services = [to_dict(service) for service in filter(None, services)]
    print(json.dumps(services))
    zeroconf.close()


if __name__ == '__main__':
    main()
