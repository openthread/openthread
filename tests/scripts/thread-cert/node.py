#!/usr/bin/env python
#
#  Copyright (c) 2016, The OpenThread Authors.
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

import os
import sys
import time
if sys.platform != 'win32':
    import node_cli
else:
    import node_api
import unittest
import config

class Node:
    def __init__(self, nodeid, is_mtd=False, simulator=None):
        self.simulator = simulator
        self.interface = None

        if sys.platform != 'win32':
            self.interface = node_cli.otCli(nodeid, is_mtd, simulator=simulator)
        else:
            self.interface = node_api.otApi(nodeid)

        self.interface.clear_whitelist()
        self.interface.disable_whitelist()
        self.interface.set_timeout(100)

    def __del__(self):
        self.destroy()

    def destroy(self):
        if self.interface:
            self.interface.destroy()
            self.interface = None

    def set_mode(self, mode):
        self.interface.set_mode(mode)

    def debug(self, level):
        self.interface.debug(level)

    def interface_up(self):
        self.interface.interface_up()

    def interface_down(self):
        self.interface.interface_down()

    def thread_start(self):
        self.interface.thread_start()

    def thread_stop(self):
        self.interface.thread_stop()

    def commissioner_start(self):
        self.interface.commissioner_start()

    def commissioner_add_joiner(self, addr, psk):
        self.interface.commissioner_add_joiner(addr, psk)

    def joiner_start(self, pskd='', provisioning_url=''):
        self.interface.joiner_start(pskd, provisioning_url)

    def start(self):
        self.interface.interface_up()
        self.interface.thread_start()

    def stop(self):
        self.interface.thread_stop()
        self.interface.interface_down()

    def clear_whitelist(self):
        self.interface.clear_whitelist()

    def enable_whitelist(self):
        self.interface.enable_whitelist()

    def disable_whitelist(self):
        self.interface.disable_whitelist()

    def add_whitelist(self, addr, rssi=None):
        self.interface.add_whitelist(addr, rssi)

    def remove_whitelist(self, addr):
        self.interface.remove_whitelist(addr)

    def get_addr16(self):
        return self.interface.get_addr16()

    def get_router_id(self):
        return self.interface.get_router_id()

    def get_addr64(self):
        return self.interface.get_addr64()

    def get_eui64(self):
        return self.interface.get_eui64()

    def get_joiner_id(self):
        return self.interface.get_joiner_id()

    def get_channel(self):
        return self.interface.get_channel()

    def set_channel(self, channel):
        self.interface.set_channel(channel)

    def get_masterkey(self):
        return self.interface.get_masterkey()

    def set_masterkey(self, masterkey):
        self.interface.set_masterkey(masterkey)

    def get_key_sequence_counter(self):
        return self.interface.get_key_sequence_counter()

    def set_key_sequence_counter(self, key_sequence_counter):
        self.interface.set_key_sequence_counter(key_sequence_counter)

    def set_key_switch_guardtime(self, key_switch_guardtime):
        self.interface.set_key_switch_guardtime(key_switch_guardtime)

    def set_network_id_timeout(self, network_id_timeout):
        self.interface.set_network_id_timeout(network_id_timeout)

    def get_network_name(self):
        return self.interface.get_network_name()

    def set_network_name(self, network_name):
        self.interface.set_network_name(network_name)

    def get_panid(self):
        return self.interface.get_panid()

    def set_panid(self, panid = config.PANID):
        self.interface.set_panid(panid)

    def get_partition_id(self):
        return self.interface.get_partition_id()

    def set_partition_id(self, partition_id):
        self.interface.set_partition_id(partition_id)

    def set_router_upgrade_threshold(self, threshold):
        self.interface.set_router_upgrade_threshold(threshold)

    def set_router_downgrade_threshold(self, threshold):
        self.interface.set_router_downgrade_threshold(threshold)

    def release_router_id(self, router_id):
        self.interface.release_router_id(router_id)

    def get_state(self):
        return self.interface.get_state()

    def set_state(self, state):
        self.interface.set_state(state)

    def get_timeout(self):
        return self.interface.get_timeout()

    def set_timeout(self, timeout):
        self.interface.set_timeout(timeout)

    def set_max_children(self, number):
        self.interface.set_max_children(number)

    def get_weight(self):
        return self.interface.get_weight()

    def set_weight(self, weight):
        self.interface.set_weight(weight)

    def add_ipaddr(self, ipaddr):
        self.interface.add_ipaddr(ipaddr)

    def get_addrs(self):
        return self.interface.get_addrs()

    def get_addr(self, prefix):
        return self.interface.get_addr(prefix)

    def get_eidcaches(self):
        return self.interface.get_eidcaches()

    def add_service(self, enterpriseNumber, serviceData, serverData):
        self.interface.add_service(enterpriseNumber, serviceData, serverData)

    def remove_service(self, enterpriseNumber, serviceData):
        self.interface.remove_service(enterpriseNumber, serviceData)

    def get_ip6_address(self, address_type):
        return self.interface.get_ip6_address(address_type)

    def get_context_reuse_delay(self):
        return self.interface.get_context_reuse_delay()

    def set_context_reuse_delay(self, delay):
        self.interface.set_context_reuse_delay(delay)

    def add_prefix(self, prefix, flags, prf = 'med'):
        self.interface.add_prefix(prefix, flags, prf)

    def remove_prefix(self, prefix):
        self.interface.remove_prefix(prefix)

    def add_route(self, prefix, prf = 'med'):
        self.interface.add_route(prefix, prf)

    def remove_route(self, prefix):
        self.interface.remove_route(prefix)

    def register_netdata(self):
        self.interface.register_netdata()

    def energy_scan(self, mask, count, period, scan_duration, ipaddr):
        self.interface.energy_scan(mask, count, period, scan_duration, ipaddr)

    def panid_query(self, panid, mask, ipaddr):
        self.interface.panid_query(panid, mask, ipaddr)

    def scan(self):
        return self.interface.scan()

    def ping(self, ipaddr, num_responses=1, size=None, timeout=5):
        return self.interface.ping(ipaddr, num_responses, size, timeout)

    def reset(self):
        return self.interface.reset()

    def set_router_selection_jitter(self, jitter):
        self.interface.set_router_selection_jitter(jitter)

    def set_active_dataset(self, timestamp, panid=None, channel=None, channel_mask=None, master_key=None):
        self.interface.set_active_dataset(timestamp, panid, channel, channel_mask, master_key)

    def set_pending_dataset(self, pendingtimestamp, activetimestamp, panid=None, channel=None):
        self.interface.set_pending_dataset(pendingtimestamp, activetimestamp, panid, channel)

    def announce_begin(self, mask, count, period, ipaddr):
        self.interface.announce_begin(mask, count, period, ipaddr)

    def send_mgmt_active_set(self, active_timestamp=None, channel=None, channel_mask=None, extended_panid=None,
                             panid=None, master_key=None, mesh_local=None, network_name=None, binary=None):
        self.interface.send_mgmt_active_set(active_timestamp, channel, channel_mask, extended_panid, panid,
                                            master_key, mesh_local, network_name, binary)

    def send_mgmt_pending_set(self, pending_timestamp=None, active_timestamp=None, delay_timer=None, channel=None,
                              panid=None, master_key=None, mesh_local=None, network_name=None):
        self.interface.send_mgmt_pending_set(pending_timestamp, active_timestamp, delay_timer, channel, panid,
                                             master_key, mesh_local, network_name)

    def coaps_start_psk(self, psk, pskIdentity):
        self.interface.coaps_start_psk(psk, pskIdentity)

    def coaps_start_x509(self):
        self.interface.coaps_start_x509()

    def coaps_set_resource_path(self, path):
        self.interface.coaps_set_resource_path(path)

    def coaps_stop(self):
        self.interface.coaps_stop()

    def coaps_connect(self, ipaddr):
        self.interface.coaps_connect(ipaddr)

    def coaps_disconnect(self):
        self.interface.coaps_disconnect()

    def coaps_get(self):
        self.interface.coaps_get()

if __name__ == '__main__':
    unittest.main()
