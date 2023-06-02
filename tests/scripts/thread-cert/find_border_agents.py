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

import ipaddress
import sys
import time
from zeroconf import IPVersion, ServiceBrowser, ServiceStateChange, Zeroconf, DNSAddress, DNSService, DNSText


def on_service_state_change(zeroconf, service_type, name, state_change):
    if state_change is ServiceStateChange.Added:
        zeroconf.get_service_info(service_type, name)


class BorderAgent(object):
    alias = None
    server_name = None
    addr = None
    port = None
    thread_status = None

    def __init__(self, alias):
        self.alias = alias

    def __repr__(self):
        return str([self.alias, self.addr, self.port, self.thread_status])


def get_ipaddr_priority(addr: ipaddress.IPv6Address):
    # calculate the priority of IPv6 addresses in order: Global > non Global > Link local
    if addr.is_link_local:
        return 0

    if not addr.is_global:
        return 1

    return 2


def parse_cache(cache):
    border_agents = []

    # Find all border routers
    for ptr in cache.get('_meshcop._udp.local.', []):
        border_agents.append(BorderAgent(ptr.alias))

    # Find server name, port and Thread Interface status for each border router
    for ba in border_agents:
        for record in cache.get(ba.alias.lower(), []):
            if isinstance(record, DNSService):
                ba.server_name = record.server
                ba.port = record.port
            elif isinstance(record, DNSText):
                text = bytearray(record.text)
                sb = text.split(b'sb=')[1][0:4]
                ba.thread_status = (sb[3] & 0x18) >> 3

    # Find IPv6 address for each border router
    for ba in border_agents:
        for record in cache.get(ba.server_name.lower(), []):
            if isinstance(record, DNSAddress):
                addr = ipaddress.ip_address(record.address)
                if not isinstance(addr, ipaddress.IPv6Address) or addr.is_multicast or addr.is_loopback:
                    continue

                if not ba.addr or get_ipaddr_priority(addr) > get_ipaddr_priority(ipaddress.IPv6Address(ba.addr)):
                    ba.addr = str(addr)

    return border_agents


def main():
    # Browse border agents
    zeroconf = Zeroconf(ip_version=IPVersion.V6Only)
    ServiceBrowser(zeroconf, "_meshcop._udp.local.", handlers=[on_service_state_change])
    time.sleep(2)
    cache = zeroconf.cache.cache
    zeroconf.close()

    border_agents = parse_cache(cache)
    for ba in border_agents:
        print(ba)


if __name__ == '__main__':
    main()
