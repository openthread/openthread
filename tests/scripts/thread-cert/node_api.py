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
import ctypes

class otApi:
    def __init__(self, nodeid):
        self.verbose = int(float(os.getenv('VERBOSE', 0)))
        self.__init_dll(nodeid)

    def __del__(self):
        self.Api.otNodeFinalize(self.otNode)

    def set_mode(self, mode):
        if self.Api.otNodeSetMode(self.otNode, mode.encode('utf-8')) != 0:
            raise OSError("otNodeSetMode failed!")

    def interface_up(self):
        if self.Api.otNodeInterfaceUp(self.otNode) != 0:
            raise OSError("otNodeInterfaceUp failed!")

    def interface_down(self):
        if self.Api.otNodeInterfaceDown(self.otNode) != 0:
            raise OSError("otNodeInterfaceDown failed!")

    def thread_start(self):
        if self.Api.otNodeThreadStart(self.otNode) != 0:
            raise OSError("otNodeThreadStart failed!")

    def thread_stop(self):
        if self.Api.otNodeThreadStop(self.otNode) != 0:
            raise OSError("otNodeThreadStop failed!")

    def commissioner_start(self):
        if self.Api.otNodeCommissionerStart(self.otNode) != 0:
            raise OSError("otNodeCommissionerStart failed!")

    def commissioner_add_joiner(self, addr, psk):
        if self.Api.otNodeCommissionerJoinerAdd(self.otNode, addr.encode('utf-8'), psk.encode('utf-8')) != 0:
            raise OSError("otNodeCommissionerJoinerAdd failed!")

    def joiner_start(self, pskd='', provisioning_url=''):
        if self.Api.otNodeJoinerStart(self.otNode, pskd.encode('utf-8'), provisioning_url.encode('utf-8')) != 0:
            raise OSError("otNodeJoinerStart failed!")

    def clear_whitelist(self):
        if self.Api.otNodeClearWhitelist(self.otNode) != 0:
            raise OSError("otNodeClearWhitelist failed!")

    def enable_whitelist(self):
        if self.Api.otNodeEnableWhitelist(self.otNode) != 0:
            raise OSError("otNodeEnableWhitelist failed!")

    def disable_whitelist(self):
        if self.Api.otNodeDisableWhitelist(self.otNode) != 0:
            raise OSError("otNodeDisableWhitelist failed!")

    def add_whitelist(self, addr, rssi=None):
        if rssi == None:
            rssi = 0
        if self.Api.otNodeAddWhitelist(self.otNode, addr.encode('utf-8'), ctypes.c_byte(rssi)) != 0:
            raise OSError("otNodeAddWhitelist failed!")

    def remove_whitelist(self, addr):
        if self.Api.otNodeRemoveWhitelist(self.otNode, addr.encode('utf-8')) != 0:
            raise OSError("otNodeRemoveWhitelist failed!")

    def get_addr16(self):
        return self.Api.otNodeGetAddr16(self.otNode)

    def get_addr64(self):
        return self.Api.otNodeGetAddr64(self.otNode).decode('utf-8')

    def get_eui64(self):
        return self.Api.otNodeGetEui64(self.otNode).decode('utf-8')

    def get_joiner_id(self):
        return self.Api.otNodeGetJoinerId(self.otNode).decode('utf-8')

    def get_channel(self):
        return self.Api.otNodeGetChannel(self.otNode)

    def set_channel(self, channel):
        if self.Api.otNodeSetChannel(self.otNode, ctypes.c_ubyte(channel)) != 0:
            raise OSError("otNodeSetChannel failed!")

    def get_masterkey(self):
        return self.Api.otNodeGetMasterkey(self.otNode).decode("utf-8")

    def set_masterkey(self, masterkey):
        if self.Api.otNodeSetMasterkey(self.otNode, masterkey.encode('utf-8')) != 0:
            raise OSError("otNodeSetMasterkey failed!")

    def get_key_sequence_counter(self):
        return self.Api.otNodeGetKeySequenceCounter(self.otNode)

    def set_key_sequence_counter(self, key_sequence_counter):
        if self.Api.otNodeSetKeySequenceCounter(self.otNode, ctypes.c_uint(key_sequence_counter)) != 0:
            raise OSError("otNodeSetKeySequenceCounter failed!")

    def set_key_switch_guardtime(self, key_switch_guardtime):
        if self.Api.otNodeSetKeySwitchGuardTime(self.otNode, ctypes.c_uint(key_switch_guardtime)) != 0:
            raise OSError("otNodeSetKeySwitchGuardTime failed!")

    def set_network_id_timeout(self, network_id_timeout):
        if self.Api.otNodeSetNetworkIdTimeout(self.otNode, ctypes.c_ubyte(network_id_timeout)) != 0:
            raise OSError("otNodeSetNetworkIdTimeout failed!")

    def get_network_name(self):
        return self.Api.otNodeGetNetworkName(self.otNode).decode("utf-8")

    def set_network_name(self, network_name):
        if self.Api.otNodeSetNetworkName(self.otNode, network_name.encode('utf-8')) != 0:
            raise OSError("otNodeSetNetworkName failed!")

    def get_panid(self):
        return int(self.Api.otNodeGetPanId(self.otNode))

    def set_panid(self, panid):
        if self.Api.otNodeSetPanId(self.otNode, ctypes.c_ushort(panid)) != 0:
            raise OSError("otNodeSetPanId failed!")

    def get_partition_id(self):
        return int(self.Api.otNodeGetPartitionId(self.otNode))

    def set_partition_id(self, partition_id):
        if self.Api.otNodeSetPartitionId(self.otNode, ctypes.c_uint(partition_id)) != 0:
            raise OSError("otNodeSetPartitionId failed!")

    def set_router_upgrade_threshold(self, threshold):
        if self.Api.otNodeSetRouterUpgradeThreshold(self.otNode, ctypes.c_ubyte(threshold)) != 0:
            raise OSError("otNodeSetRouterUpgradeThreshold failed!")

    def set_router_downgrade_threshold(self, threshold):
        if self.Api.otNodeSetRouterDowngradeThreshold(self.otNode, ctypes.c_ubyte(threshold)) != 0:
            raise OSError("otNodeSetRouterDowngradeThreshold failed!")

    def release_router_id(self, router_id):
        if self.Api.otNodeReleaseRouterId(self.otNode, ctypes.c_ubyte(router_id)) != 0:
            raise OSError("otNodeReleaseRouterId failed!")

    def get_state(self):
        return self.Api.otNodeGetState(self.otNode).decode('utf-8')

    def set_state(self, state):
        if self.Api.otNodeSetState(self.otNode, state.encode('utf-8')) != 0:
            raise OSError("otNodeSetState failed!")

    def get_timeout(self):
        return int(self.Api.otNodeGetTimeout(self.otNode))

    def set_timeout(self, timeout):
        if self.Api.otNodeSetTimeout(self.otNode, ctypes.c_uint(timeout)) != 0:
            raise OSError("otNodeSetTimeout failed!")

    def set_max_children(self, number):
        if self.Api.otNodeSetMaxChildren(self.otNode, ctypes.c_ubyte(number)) != 0:
            raise OSError("otNodeSetMaxChildren failed!")

    def get_weight(self):
        return int(self.Api.otNodeGetWeight(self.otNode))

    def set_weight(self, weight):
        if self.Api.otNodeSetWeight(self.otNode, ctypes.c_ubyte(weight)) != 0:
            raise OSError("otNodeSetWeight failed!")

    def add_ipaddr(self, ipaddr):
        if self.Api.otNodeAddIpAddr(self.otNode, ipaddr.encode('utf-8')) != 0:
            raise OSError("otNodeAddIpAddr failed!")

    def get_addrs(self):
        return self.Api.otNodeGetAddrs(self.otNode).decode("utf-8").split("\n")

    def add_service(self, enterpriseNumber, serviceData, serverData):
        raise OSError("otServerAddService wrapper not implemented!")

    def remove_service(self, enterpriseNumber, serviceData):
        raise OSError("otServerRemoveService wrapper not implemented!")

    def get_context_reuse_delay(self):
        return int(self.Api.otNodeGetContextReuseDelay(self.otNode))

    def set_context_reuse_delay(self, delay):
        if self.Api.otNodeSetContextReuseDelay(self.otNode, ctypes.c_uint(delay)) != 0:
            raise OSError("otNodeSetContextReuseDelay failed!")

    def add_prefix(self, prefix, flags, prf = 'med'):
        if self.Api.otNodeAddPrefix(self.otNode, prefix.encode('utf-8'), flags.encode('utf-8'), prf.encode('utf-8')) != 0:
            raise OSError("otNodeAddPrefix failed!")

    def remove_prefix(self, prefix):
        if self.Api.otNodeRemovePrefix(self.otNode, prefix.encode('utf-8')) != 0:
            raise OSError("otNodeRemovePrefix failed!")

    def add_route(self, prefix, prf = 'med'):
        if self.Api.otNodeAddRoute(self.otNode, prefix.encode('utf-8'), prf.encode('utf-8')) != 0:
            raise OSError("otNodeAddRoute failed!")

    def remove_route(self, prefix):
        if self.Api.otNodeRemoveRoute(self.otNode, prefix.encode('utf-8')) != 0:
            raise OSError("otNodeRemovePrefix failed!")

    def register_netdata(self):
        if self.Api.otNodeRegisterNetdata(self.otNode) != 0:
            raise OSError("otNodeRegisterNetdata failed!")

    def energy_scan(self, mask, count, period, scan_duration, ipaddr):
        if self.Api.otNodeEnergyScan(self.otNode, ctypes.c_uint(mask), ctypes.c_ubyte(count), ctypes.c_ushort(period), ctypes.c_ushort(scan_duration), ipaddr.encode('utf-8')) != 0:
            raise OSError("otNodeEnergyScan failed!")

    def panid_query(self, panid, mask, ipaddr):
        if self.Api.otNodePanIdQuery(self.otNode, ctypes.c_ushort(panid), ctypes.c_uint(mask), ipaddr.encode('utf-8')) != 0:
            raise OSError("otNodePanIdQuery failed!")

    def scan(self):
        return self.Api.otNodeScan(self.otNode).decode("utf-8").split("\n")

    def ping(self, ipaddr, num_responses=1, size=None, timeout=5000):
        if size == None:
            size = 100
        numberOfResponders = self.Api.otNodePing(self.otNode, ipaddr.encode('utf-8'), ctypes.c_ushort(size),
                                                 ctypes.c_uint(num_responses), ctypes.c_uint16(timeout))
        return numberOfResponders >= num_responses

    def set_router_selection_jitter(self, jitter):
        if self.Api.otNodeSetRouterSelectionJitter(self.otNode, ctypes.c_ubyte(jitter)) != 0:
            raise OSError("otNodeSetRouterSelectionJitter failed!")

    def set_active_dataset(self, timestamp, panid=None, channel=None, channel_mask=None, master_key=None):
        if panid == None:
            panid = 0
        if channel == None:
            channel = 0
        if channel_mask == None:
            channel_mask = 0
        if master_key == None:
            master_key = ""
        if self.Api.otNodeSetActiveDataset(
                self.otNode,
                ctypes.c_ulonglong(timestamp),
                ctypes.c_ushort(panid),
                ctypes.c_ushort(channel),
                ctypes.c_uint(channel_mask),
                master_key.encode('utf-8')
            ) != 0:
            raise OSError("otNodeSetActiveDataset failed!")

    def set_pending_dataset(self, pendingtimestamp, activetimestamp, panid=None, channel=None):
        if pendingtimestamp == None:
            pendingtimestamp = 0
        if activetimestamp == None:
            activetimestamp = 0
        if panid == None:
            panid = 0
        if channel == None:
            channel = 0
        if self.Api.otNodeSetPendingDataset(
                self.otNode,
                ctypes.c_ulonglong(activetimestamp),
                ctypes.c_ulonglong(pendingtimestamp),
                ctypes.c_ushort(panid),
                ctypes.c_ushort(channel)
            ) != 0:
            raise OSError("otNodeSetPendingDataset failed!")

    def announce_begin(self, mask, count, period, ipaddr):
        if self.Api.otNodeCommissionerAnnounceBegin(self.otNode, ctypes.c_uint(mask), ctypes.c_ubyte(count), ctypes.c_ushort(period), ipaddr.encode('utf-8')) != 0:
            raise OSError("otNodeCommissionerAnnounceBegin failed!")

    def send_mgmt_active_set(self, active_timestamp=None, channel=None, channel_mask=None, extended_panid=None,
                             panid=None, master_key=None, mesh_local=None, network_name=None, binary=None):
        if active_timestamp == None:
            active_timestamp = 0
        if panid == None:
            panid = 0
        if channel == None:
            channel = 0
        if channel_mask == None:
            channel_mask = 0
        if extended_panid == None:
            extended_panid = ""
        if master_key == None:
            master_key = ""
        if mesh_local == None:
            mesh_local = ""
        if network_name == None:
            network_name = ""
        if binary == None:
            binary = ""
        if self.Api.otNodeSendActiveSet(
                self.otNode,
                ctypes.c_ulonglong(active_timestamp),
                ctypes.c_ushort(panid),
                ctypes.c_ushort(channel),
                ctypes.c_uint(channel_mask),
                extended_panid.encode('utf-8'),
                master_key.encode('utf-8'),
                mesh_local.encode('utf-8'),
                network_name.encode('utf-8'),
                binary.encode('utf-8')
            ) != 0:
            raise OSError("otNodeSendActiveSet failed!")

    def send_mgmt_pending_set(self, pending_timestamp=None, active_timestamp=None, delay_timer=None, channel=None,
                              panid=None, master_key=None, mesh_local=None, network_name=None):
        if pending_timestamp == None:
            pending_timestamp = 0
        if active_timestamp == None:
            active_timestamp = 0
        if delay_timer == None:
            delay_timer = 0
        if panid == None:
            panid = 0
        if channel == None:
            channel = 0
        if master_key == None:
            master_key = ""
        if mesh_local == None:
            mesh_local = ""
        if network_name == None:
            network_name = ""
        if self.Api.otNodeSendPendingSet(
                self.otNode,
                ctypes.c_ulonglong(active_timestamp),
                ctypes.c_ulonglong(pending_timestamp),
                ctypes.c_uint(delay_timer),
                ctypes.c_ushort(panid),
                ctypes.c_ushort(channel),
                master_key.encode('utf-8'),
                mesh_local.encode('utf-8'),
                network_name.encode('utf-8')
            ) != 0:
            raise OSError("otNodeSendPendingSet failed!")

    def log(self, message):
        self.Api.otNodeLog(message)

    def __init_dll(self, nodeid):
        """ Initialize the API from a Windows DLL. """

        # Load the DLL
        self.Api = ctypes.WinDLL("otnodeapi.dll")
        if self.Api == None:
            raise OSError("Failed to load otnodeapi.dll!")

        # Define the functions
        self.Api.otNodeLog.argtypes = [ctypes.c_char_p]

        self.Api.otNodeInit.argtypes = [ctypes.c_uint]
        self.Api.otNodeInit.restype = ctypes.c_void_p

        self.Api.otNodeFinalize.argtypes = [ctypes.c_void_p]

        self.Api.otNodeSetMode.argtypes = [ctypes.c_void_p,
                                           ctypes.c_char_p]

        self.Api.otNodeInterfaceUp.argtypes = [ctypes.c_void_p]

        self.Api.otNodeInterfaceDown.argtypes = [ctypes.c_void_p]

        self.Api.otNodeThreadStart.argtypes = [ctypes.c_void_p]

        self.Api.otNodeThreadStop.argtypes = [ctypes.c_void_p]

        self.Api.otNodeCommissionerStart.argtypes = [ctypes.c_void_p]

        self.Api.otNodeCommissionerJoinerAdd.argtypes = [ctypes.c_void_p,
                                                         ctypes.c_char_p,
                                                         ctypes.c_char_p]

        self.Api.otNodeCommissionerStop.argtypes = [ctypes.c_void_p]

        self.Api.otNodeJoinerStart.argtypes = [ctypes.c_void_p,
                                               ctypes.c_char_p,
                                               ctypes.c_char_p]

        self.Api.otNodeJoinerStop.argtypes = [ctypes.c_void_p]

        self.Api.otNodeClearWhitelist.argtypes = [ctypes.c_void_p]

        self.Api.otNodeEnableWhitelist.argtypes = [ctypes.c_void_p]

        self.Api.otNodeDisableWhitelist.argtypes = [ctypes.c_void_p]

        self.Api.otNodeAddWhitelist.argtypes = [ctypes.c_void_p,
                                                ctypes.c_char_p,
                                                ctypes.c_byte]

        self.Api.otNodeRemoveWhitelist.argtypes = [ctypes.c_void_p,
                                                   ctypes.c_char_p]

        self.Api.otNodeGetAddr16.argtypes = [ctypes.c_void_p]
        self.Api.otNodeGetAddr16.restype = ctypes.c_ushort

        self.Api.otNodeGetAddr64.argtypes = [ctypes.c_void_p]
        self.Api.otNodeGetAddr64.restype = ctypes.c_char_p

        self.Api.otNodeGetEui64.argtypes = [ctypes.c_void_p]
        self.Api.otNodeGetEui64.restype = ctypes.c_char_p

        self.Api.otNodeGetJoinerId.argtypes = [ctypes.c_void_p]
        self.Api.otNodeGetJoinerId.restype = ctypes.c_char_p

        self.Api.otNodeSetChannel.argtypes = [ctypes.c_void_p,
                                              ctypes.c_ubyte]

        self.Api.otNodeGetChannel.argtypes = [ctypes.c_void_p]
        self.Api.otNodeGetChannel.restype = ctypes.c_ubyte

        self.Api.otNodeSetMasterkey.argtypes = [ctypes.c_void_p,
                                                ctypes.c_char_p]

        self.Api.otNodeGetMasterkey.argtypes = [ctypes.c_void_p]
        self.Api.otNodeGetMasterkey.restype = ctypes.c_char_p

        self.Api.otNodeGetKeySequenceCounter.argtypes = [ctypes.c_void_p]
        self.Api.otNodeGetKeySequenceCounter.restype = ctypes.c_uint

        self.Api.otNodeSetKeySequenceCounter.argtypes = [ctypes.c_void_p,
                                                         ctypes.c_uint]

        self.Api.otNodeSetKeySwitchGuardTime.argtypes = [ctypes.c_void_p,
                                                         ctypes.c_uint]

        self.Api.otNodeSetNetworkIdTimeout.argtypes = [ctypes.c_void_p,
                                                       ctypes.c_ubyte]

        self.Api.otNodeGetNetworkName.argtypes = [ctypes.c_void_p]
        self.Api.otNodeGetNetworkName.restype = ctypes.c_char_p

        self.Api.otNodeSetNetworkName.argtypes = [ctypes.c_void_p,
                                                  ctypes.c_char_p]

        self.Api.otNodeGetPanId.argtypes = [ctypes.c_void_p]
        self.Api.otNodeGetPanId.restype = ctypes.c_ushort

        self.Api.otNodeSetPanId.argtypes = [ctypes.c_void_p,
                                            ctypes.c_ushort]

        self.Api.otNodeGetPartitionId.argtypes = [ctypes.c_void_p]
        self.Api.otNodeGetPartitionId.restype = ctypes.c_uint

        self.Api.otNodeSetPartitionId.argtypes = [ctypes.c_void_p,
                                                  ctypes.c_uint]

        self.Api.otNodeSetRouterUpgradeThreshold.argtypes = [ctypes.c_void_p,
                                                             ctypes.c_ubyte]

        self.Api.otNodeSetRouterDowngradeThreshold.argtypes = [ctypes.c_void_p,
                                                               ctypes.c_ubyte]

        self.Api.otNodeReleaseRouterId.argtypes = [ctypes.c_void_p,
                                                   ctypes.c_ubyte]

        self.Api.otNodeGetState.argtypes = [ctypes.c_void_p]
        self.Api.otNodeGetState.restype = ctypes.c_char_p

        self.Api.otNodeSetState.argtypes = [ctypes.c_void_p,
                                            ctypes.c_char_p]

        self.Api.otNodeGetTimeout.argtypes = [ctypes.c_void_p]
        self.Api.otNodeGetTimeout.restype = ctypes.c_uint

        self.Api.otNodeSetTimeout.argtypes = [ctypes.c_void_p,
                                            ctypes.c_uint]

        self.Api.otNodeGetWeight.argtypes = [ctypes.c_void_p]
        self.Api.otNodeGetWeight.restype = ctypes.c_ubyte

        self.Api.otNodeSetWeight.argtypes = [ctypes.c_void_p,
                                             ctypes.c_ubyte]

        self.Api.otNodeAddIpAddr.argtypes = [ctypes.c_void_p,
                                             ctypes.c_char_p]

        self.Api.otNodeGetAddrs.argtypes = [ctypes.c_void_p]
        self.Api.otNodeGetAddrs.restype = ctypes.c_char_p

        self.Api.otNodeGetContextReuseDelay.argtypes = [ctypes.c_void_p]
        self.Api.otNodeGetContextReuseDelay.restype = ctypes.c_uint

        self.Api.otNodeSetContextReuseDelay.argtypes = [ctypes.c_void_p,
                                                        ctypes.c_uint]

        self.Api.otNodeAddPrefix.argtypes = [ctypes.c_void_p,
                                             ctypes.c_char_p,
                                             ctypes.c_char_p,
                                             ctypes.c_char_p]

        self.Api.otNodeRemovePrefix.argtypes = [ctypes.c_void_p,
                                                ctypes.c_char_p]

        self.Api.otNodeAddRoute.argtypes = [ctypes.c_void_p,
                                            ctypes.c_char_p,
                                            ctypes.c_char_p]

        self.Api.otNodeRemoveRoute.argtypes = [ctypes.c_void_p,
                                               ctypes.c_char_p]

        self.Api.otNodeRegisterNetdata.argtypes = [ctypes.c_void_p]

        self.Api.otNodeEnergyScan.argtypes = [ctypes.c_void_p,
                                              ctypes.c_uint,
                                              ctypes.c_ubyte,
                                              ctypes.c_ushort,
                                              ctypes.c_ushort,
                                              ctypes.c_char_p]

        self.Api.otNodePanIdQuery.argtypes = [ctypes.c_void_p,
                                              ctypes.c_ushort,
                                              ctypes.c_uint,
                                              ctypes.c_char_p]

        self.Api.otNodeScan.argtypes = [ctypes.c_void_p]
        self.Api.otNodeScan.restype = ctypes.c_char_p

        self.Api.otNodePing.argtypes = [ctypes.c_void_p,
                                        ctypes.c_char_p,
                                        ctypes.c_ushort,
                                        ctypes.c_uint,
                                        ctypes.c_uint16]
        self.Api.otNodePing.restype = ctypes.c_uint

        self.Api.otNodeSetRouterSelectionJitter.argtypes = [ctypes.c_void_p,
                                                            ctypes.c_ubyte]

        self.Api.otNodeCommissionerAnnounceBegin.argtypes = [ctypes.c_void_p,
                                                             ctypes.c_uint,
                                                             ctypes.c_ubyte,
                                                             ctypes.c_ushort,
                                                             ctypes.c_char_p]

        self.Api.otNodeSetActiveDataset.argtypes = [ctypes.c_void_p,
                                                    ctypes.c_ulonglong,
                                                    ctypes.c_ushort,
                                                    ctypes.c_ushort,
                                                    ctypes.c_uint,
                                                    ctypes.c_char_p]

        self.Api.otNodeSetPendingDataset.argtypes = [ctypes.c_void_p,
                                                     ctypes.c_ulonglong,
                                                     ctypes.c_ulonglong,
                                                     ctypes.c_ushort,
                                                     ctypes.c_ushort]

        self.Api.otNodeSendPendingSet.argtypes = [ctypes.c_void_p,
                                                  ctypes.c_ulonglong,
                                                  ctypes.c_ulonglong,
                                                  ctypes.c_uint,
                                                  ctypes.c_ushort,
                                                  ctypes.c_ushort,
                                                  ctypes.c_char_p,
                                                  ctypes.c_char_p,
                                                  ctypes.c_char_p]

        self.Api.otNodeSendActiveSet.argtypes = [ctypes.c_void_p,
                                                 ctypes.c_ulonglong,
                                                 ctypes.c_ushort,
                                                 ctypes.c_ushort,
                                                 ctypes.c_uint,
                                                 ctypes.c_char_p,
                                                 ctypes.c_char_p,
                                                 ctypes.c_char_p,
                                                 ctypes.c_char_p,
                                                 ctypes.c_char_p]

        self.Api.otNodeSetMaxChildren.argtypes = [ctypes.c_void_p,
                                                  ctypes.c_ubyte]

        # Initialize a new node
        self.otNode = self.Api.otNodeInit(ctypes.c_uint(nodeid))
        if self.otNode == None:
            raise OSError("otNodeInit failed!")
