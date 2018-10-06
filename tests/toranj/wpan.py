#!/usr/bin/env python
#
#  Copyright (c) 2018, The OpenThread Authors.
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
import os
import time
import re
import random
import weakref
import subprocess
import socket
import asyncore
import inspect

#----------------------------------------------------------------------------------------------------------------------
# wpantund properties

WPAN_STATE                                     = 'NCP:State'
WPAN_NAME                                      = 'Network:Name'
WPAN_PANID                                     = 'Network:PANID'
WPAN_XPANID                                    = 'Network:XPANID'
WPAN_KEY                                       = 'Network:Key'
WPAN_KEY_INDEX                                 = 'Network:KeyIndex'
WPAN_CHANNEL                                   = 'NCP:Channel'
WPAN_HW_ADDRESS                                = 'NCP:HardwareAddress'
WPAN_EXT_ADDRESS                               = 'NCP:ExtendedAddress'
WPAN_POLL_INTERVAL                             = 'NCP:SleepyPollInterval'
WPAN_NODE_TYPE                                 = 'Network:NodeType'
WPAN_ROLE                                      = 'Network:Role'
WPAN_PARTITION_ID                              = 'Network:PartitionId'
WPAN_NCP_VERSION                               = 'NCP:Version'
WPAN_NCP_MCU_POWER_STATE                       = "NCP:MCUPowerState"
WPAN_NETWORK_ALLOW_JOIN                        = 'com.nestlabs.internal:Network:AllowingJoin'
WPAN_NETWORK_PASSTHRU_PORT                     = 'com.nestlabs.internal:Network:PassthruPort'

WPAN_IP6_LINK_LOCAL_ADDRESS                    = "IPv6:LinkLocalAddress"
WPAN_IP6_MESH_LOCAL_ADDRESS                    = "IPv6:MeshLocalAddress"
WPAN_IP6_MESH_LOCAL_PREFIX                     = "IPv6:MeshLocalPrefix"
WPAN_IP6_ALL_ADDRESSES                         = "IPv6:AllAddresses"
WPAN_IP6_MULTICAST_ADDRESSES                   = "IPv6:MulticastAddresses"

WPAN_THREAD_RLOC16                             = "Thread:RLOC16"
WPAN_THREAD_ROUTER_ID                          = "Thread:RouterID"
WPAN_THREAD_LEADER_ADDRESS                     = "Thread:Leader:Address"
WPAN_THREAD_LEADER_ROUTER_ID                   = "Thread:Leader:RouterID"
WPAN_THREAD_LEADER_WEIGHT                      = "Thread:Leader:Weight"
WPAN_THREAD_LEADER_LOCAL_WEIGHT                = "Thread:Leader:LocalWeight"
WPAN_THREAD_LEADER_NETWORK_DATA                = "Thread:Leader:NetworkData"
WPAN_THREAD_STABLE_LEADER_NETWORK_DATA         = "Thread:Leader:StableNetworkData"
WPAN_THREAD_NETWORK_DATA                       = "Thread:NetworkData"
WPAN_THREAD_CHILD_TABLE                        = "Thread:ChildTable"
WPAN_THREAD_CHILD_TABLE_ASVALMAP               = "Thread:ChildTable:AsValMap"
WPAN_THREAD_CHILD_TABLE_ADDRESSES              = "Thread:ChildTable:Addresses"
WPAN_THREAD_NEIGHBOR_TABLE                     = "Thread:NeighborTable"
WPAN_THREAD_NEIGHBOR_TABLE_ASVALMAP            = "Thread:NeighborTable:AsValMap"
WPAN_THREAD_NEIGHBOR_TABLE_ERR_RATES           = "Thread:NeighborTable:ErrorRates"
WPAN_THREAD_NEIGHBOR_TABLE_ERR_RATES_AVVALMAP  = "Thread:NeighborTable:ErrorRates:AsValMap"
WPAN_THREAD_ROUTER_TABLE                       = "Thread:RouterTable"
WPAN_THREAD_ROUTER_TABLE_ASVALMAP              = "Thread:RouterTable:AsValMap"
WPAN_THREAD_CHILD_TIMEOUT                      = "Thread:ChildTimeout"
WPAN_THREAD_PARENT                             = "Thread:Parent"
WPAN_THREAD_PARENT_ASVALMAP                    = "Thread:Parent:AsValMap"
WPAN_THREAD_NETWORK_DATA_VERSION               = "Thread:NetworkDataVersion"
WPAN_THREAD_STABLE_NETWORK_DATA                = "Thread:StableNetworkData"
WPAN_THREAD_STABLE_NETWORK_DATA_VERSION        = "Thread:StableNetworkDataVersion"
WPAN_THREAD_PREFERRED_ROUTER_ID                = "Thread:PreferredRouterID"
WPAN_THREAD_COMMISSIONER_ENABLED               = "Thread:Commissioner:Enabled"
WPAN_THREAD_DEVICE_MODE                        = "Thread:DeviceMode"
WPAN_THREAD_OFF_MESH_ROUTES                    = "Thread:OffMeshRoutes"
WPAN_THREAD_ON_MESH_PREFIXES                   = "Thread:OnMeshPrefixes"
WPAN_THREAD_ROUTER_ROLE_ENABLED                = "Thread:RouterRole:Enabled"
WPAN_THREAD_CONFIG_FILTER_RLOC_ADDRESSES       = "Thread:Config:FilterRLOCAddresses"
WPAN_THREAD_ROUTER_UPGRADE_THRESHOLD           = "Thread:RouterUpgradeThreshold"
WPAN_THREAD_ROUTER_DOWNGRADE_THRESHOLD         = "Thread:RouterDowngradeThreshold"
WPAN_THREAD_ACTIVE_DATASET                     = "Thread:ActiveDataset"
WPAN_THREAD_ACTIVE_DATASET_ASVALMAP            = "Thread:ActiveDataset:AsValMap"
WPAN_THREAD_PENDING_DATASET                    = "Thread:PendingDataset"
WPAN_THREAD_PENDING_DATASET_ASVALMAP           = "Thread:PendingDataset:AsValMap"
WPAN_THREAD_ADDRESS_CACHE_TABLE                = "Thread:AddressCacheTable"
WPAN_THREAD_ADDRESS_CACHE_TABLE_ASVALMAP       = "Thread:AddressCacheTable:AsValMap"

WPAN_OT_LOG_LEVEL                              = "OpenThread:LogLevel"
WPAN_OT_STEERING_DATA_ADDRESS                  = "OpenThread:SteeringData:Address"
WPAN_OT_STEERING_DATA_SET_WHEN_JOINABLE        = "OpenThread:SteeringData:SetWhenJoinable"
WPAN_OT_MSG_BUFFER_COUNTERS                    = "OpenThread:MsgBufferCounters"
WPAN_OT_MSG_BUFFER_COUNTERS_AS_STRING          = "OpenThread:MsgBufferCounters:AsString"
WPAN_OT_DEBUG_TEST_ASSERT                      = "OpenThread:Debug:TestAssert"
WPAN_OT_DEBUG_TEST_WATCHDOG                    = "OpenThread:Debug:TestWatchdog"

WPAN_NCP_COUNTER_ALL_MAC                       = "NCP:Counter:AllMac"
WPAN_NCP_COUNTER_ALL_MAC_ASVALMAP              = "NCP:Counter:AllMac:AsValMap"

WPAN_MAC_WHITELIST_ENABLED                     = "MAC:Whitelist:Enabled"
WPAN_MAC_WHITELIST_ENTRIES                     = "MAC:Whitelist:Entries"
WPAN_MAC_WHITELIST_ENTRIES_ASVALMAP            = "MAC:Whitelist:Entries:AsValMap"
WPAN_MAC_BLACKLIST_ENABLED                     = "MAC:Blacklist:Enabled"
WPAN_MAC_BLACKLIST_ENTRIES                     = "MAC:Blacklist:Entries"
WPAN_MAC_BLACKLIST_ENTRIES_ASVALMAP            = "MAC:Blacklist:Entries:AsValMap"

WPAN_CHILD_SUPERVISION_INTERVAL                = "ChildSupervision:Interval"
WPAN_CHILD_SUPERVISION_CHECK_TIMEOUT           = "ChildSupervision:CheckTimeout"

WPAN_JAM_DETECTION_STATUS                      = "JamDetection:Status"
WPAN_JAM_DETECTION_ENABLE                      = "JamDetection:Enable"
WPAN_JAM_DETECTION_RSSI_THRESHOLD              = "JamDetection:RssiThreshold"
WPAN_JAM_DETECTION_WINDOW                      = "JamDetection:Window"
WPAN_JAM_DETECTION_BUSY_PERIOD                 = "JamDetection:BusyPeriod"
WPAN_JAM_DETECTION_DEBUG_HISTORY_BITMAP        = "JamDetection:Debug:HistoryBitmap"

WPAN_CHANNEL_MONITOR_SAMPLE_INTERVAL           = "ChannelMonitor:SampleInterval"
WPAN_CHANNEL_MONITOR_RSSI_THRESHOLD            = "ChannelMonitor:RssiThreshold"
WPAN_CHANNEL_MONITOR_SAMPLE_WINDOW             = "ChannelMonitor:SampleWindow"
WPAN_CHANNEL_MONITOR_SAMPLE_COUNT              = "ChannelMonitor:SampleCount"
WPAN_CHANNEL_MONITOR_CHANNEL_QUALITY           = "ChannelMonitor:ChannelQuality"
WPAN_CHANNEL_MONITOR_CHANNEL_QUALITY_ASVALMAP  = "ChannelMonitor:ChannelQuality:AsValMap"

WPAN_CHANNEL_MANAGER_NEW_CHANNEL               = "ChannelManager:NewChannel"
WPAN_CHANNEL_MANAGER_DELAY                     = "ChannelManager:Delay"
WPAN_CHANNEL_MANAGER_CHANNEL_SELECT            = "ChannelManager:ChannelSelect"
WPAN_CHANNEL_MANAGER_AUTO_SELECT_ENABLED       = "ChannelManager:AutoSelect:Enabled"
WPAN_CHANNEL_MANAGER_AUTO_SELECT_INTERVAL      = "ChannelManager:AutoSelect:Interval"
WPAN_CHANNEL_MANAGER_SUPPORTED_CHANNEL_MASK    = "ChannelManager:SupportedChannelMask"
WPAN_CHANNEL_MANAGER_FAVORED_CHANNEL_MASK      = "ChannelManager:FavoredChannelMask"

#----------------------------------------------------------------------------------------------------------------------
# Valid state values

STATE_UNINITIALIZED                            =  '"uninitialized"'
STATE_FAULT                                    =  '"uninitialized:fault"'
STATE_UPGRADING                                =  '"uninitialized:upgrading"'
STATE_DEEP_SLEEP                               =  '"offline:deep-sleep"'
STATE_OFFLINE                                  =  '"offline"'
STATE_COMMISSIONED                             =  '"offline:commissioned"'
STATE_ASSOCIATING                              =  '"associating"'
STATE_CREDENTIALS_NEEDED                       =  '"associating:credentials-needed"'
STATE_ASSOCIATED                               =  '"associated"'
STATE_ISOLATED                                 =  '"associated:no-parent"'
STATE_NETWAKE_ASLEEP                           =  '"associated:netwake-asleep"'
STATE_NETWAKE_WAKING                           =  '"associated:netwake-waking"'

#-----------------------------------------------------------------------------------------------------------------------
# MCU Power state from `WPAN_NCP_MCU_POWER_STATE`

MCU_POWER_STATE_ON                             = '"on"'
MCU_POWER_STATE_LOW_POWER                      = '"low-power"'
MCU_POWER_STATE_OFF                            = '"off"'

#-----------------------------------------------------------------------------------------------------------------------
# Node types (from `WPAN_NODE_TYPE` property)

NODE_TYPE_UNKNOWN                              = '"unknown"'
NODE_TYPE_LEADER                               = '"leader"'
NODE_TYPE_ROUTER                               = '"router"'
NODE_TYPE_END_DEVICE                           = '"end-device"'
NODE_TYPE_SLEEPY_END_DEVICE                    = '"sleepy-end-device"'
NODE_TYPE_COMMISSIONER                         = '"commissioner"'
NODE_TYPE_NEST_LURKER                          = '"nl-lurker"'

#-----------------------------------------------------------------------------------------------------------------------
# Node types used by `Node.join()`

JOIN_TYPE_ROUTER                               = 'r'
JOIN_TYPE_END_DEVICE                           = 'e'
JOIN_TYPE_SLEEPY_END_DEVICE                    = 's'

#-----------------------------------------------------------------------------------------------------------------------
# Bit Flags for Thread Device Mode `WPAN_THREAD_DEVICE_MODE`

THREAD_MODE_FLAG_FULL_NETWORK_DATA   = (1 << 0)
THREAD_MODE_FLAG_FULL_THREAD_DEV     = (1 << 1)
THREAD_MODE_FLAG_SECURE_DATA_REQUEST = (1 << 2)
THREAD_MODE_FLAG_RX_ON_WHEN_IDLE     = (1 << 3)

#-----------------------------------------------------------------------------------------------------------------------

def _log(text, new_line=True, flush=True):
    sys.stdout.write(text)
    if new_line:
        sys.stdout.write('\n')
    if flush:
        sys.stdout.flush()

#-----------------------------------------------------------------------------------------------------------------------
# Node class

class Node(object):
    """ A wpantund OT NCP instance """

    _VERBOSE = False        # defines the default verbosity setting (can be changed per `Node`)
    _SPEED_UP_FACTOR = 1    # defines the default time speed up factor

    # path to `wpantund`, `wpanctl`, `ot-ncp-ftd`,`ot-ncp` and `ot-ncp-radio`
    _WPANTUND = '/usr/local/sbin/wpantund'
    _WPANCTL  = '/usr/local/bin/wpanctl'
    _OT_NCP_FTD = '../../examples/apps/ncp/ot-ncp-ftd'

    _OT_NCP_FTD_POSIX_APP = '../../src/posix/ot-ncp'
    _OT_NCP_RADIO = '../../examples/apps/ncp/ot-ncp-radio'

    # Environment variable used to determine how to run OpenThread
    # If set to 1, then posix-app (`ot-ncp`) is used along with a posix RCP `ot-ncp-radio`.
    # Otherwise, the posix NCP `ot-ncp-ftd` is used
    _POSIX_APP_ENV_VAR = 'TORANJ_POSIX_APP_RCP_MODEL'

    _TUND_LOG_TO_FILE = True            # determines if the wpantund logs are saved in file or sent to stdout
    _TUND_LOG_FNAME = 'wpantund-logs'   # name of wpantund log file (if # name of wpantund _TUND_LOG_TO_FILE is True)

    # interface name
    _INTFC_NAME_PREFIX = 'utun' if sys.platform == 'darwin' else 'wpan'
    _START_INDEX = 4 if sys.platform == 'darwin' else 1

    _cur_index = _START_INDEX
    _all_nodes = weakref.WeakSet()

    def __init__(self, verbose=_VERBOSE):
        """Creates a new `Node` instance"""

        index = Node._cur_index
        Node._cur_index += 1

        self._index = index
        self._interface_name = self._INTFC_NAME_PREFIX + str(index)
        self._verbose = verbose

        # Check if env variable `TORANJ_POSIX_APP_RCP_MODEL` is defined
        # and use it to determine if to use operate in "posix-ncp-app".
        if self._POSIX_APP_ENV_VAR in os.environ:
            use_posix_app_with_rcp = (os.environ[self._POSIX_APP_ENV_VAR] in ['1', 'yes'])
        else:
            use_posix_app_with_rcp = False

        if use_posix_app_with_rcp:
            ncp_socket_path = 'system:{} -s {} {} {}'.format(self._OT_NCP_FTD_POSIX_APP, self._SPEED_UP_FACTOR,
                self._OT_NCP_RADIO, index)
        else:
            ncp_socket_path = 'system:{} {} {}'.format(self._OT_NCP_FTD, index, self._SPEED_UP_FACTOR)

        cmd = self._WPANTUND + \
               ' -o Config:NCP:SocketPath \"{}\"'.format(ncp_socket_path) + \
               ' -o Config:TUN:InterfaceName {}'.format(self._interface_name) + \
               ' -o Config:NCP:DriverName spinel' + \
               ' -o Daemon:SyslogMask \"all -debug\"'

        if Node._TUND_LOG_TO_FILE:
            self._tund_log_file = open(self._TUND_LOG_FNAME + str(index) + '.log', 'wb')
        else:
            self._tund_log_file = None

        if self._verbose:
            _log('$ Node{}.__init__() cmd: {}'.format(index, cmd))

        self._wpantund_process = subprocess.Popen(cmd, shell=True, stderr=self._tund_log_file)

        self._wpanctl_cmd = self._WPANCTL + ' -I ' + self._interface_name + ' '

        self._recvers = weakref.WeakValueDictionary()  # map from local_port to `AsyncReceiver` object
        Node._all_nodes.add(self)

    def __del__(self):
        self._wpantund_process.poll()
        if self._wpantund_process.returncode is None:
            self._wpantund_process.terminate()
            self._wpantund_process.wait()

    def __repr__(self):
        return 'Node (index={}, interface_name={})'.format(self._index, self._interface_name)

    @property
    def index(self):
        return self._index

    @property
    def interface_name(self):
        return self._interface_name

    @property
    def tund_log_file(self):
        return self._tund_log_file

    #------------------------------------------------------------------------------------------------------------------
    # Executing a `wpanctl` command

    def wpanctl(self, cmd):
        """ Runs a wpanctl command on the given wpantund/OT-NCP instance and returns the output """

        if self._verbose:
            _log('$ Node{}.wpanctl(\'{}\')'.format(self._index, cmd), new_line=False)

        result = subprocess.check_output(self._wpanctl_cmd + cmd, shell=True, stderr=subprocess.STDOUT)

        if len(result) >= 1 and result[-1] == '\n':  # remove the last char if it is '\n',
            result = result[:-1]

        if self._verbose:
            if '\n' in result:
                _log(':')
                for line in result.splitlines():
                    _log('     ' + line)
            else:
                _log(' -> \'{}\''.format(result))

        return result

    #------------------------------------------------------------------------------------------------------------------
    # APIs matching `wpanctl` commands.

    def get(self, prop_name, value_only=True):
        return self.wpanctl('get ' + ('-v ' if value_only else '') + prop_name)

    def set(self, prop_name, value, binary_data=False):
        return self._update_prop('set', prop_name, value, binary_data)

    def add(self, prop_name, value, binary_data=False):
        return self._update_prop('add', prop_name, value, binary_data)

    def remove(self, prop_name, value, binary_data=False):
        return self._update_prop('remove', prop_name, value, binary_data)

    def _update_prop(self, action, prop_name, value, binary_data):
        return self.wpanctl(action + ' ' + prop_name + ' ' + ('-d ' if binary_data else '') +
                            '-v ' + value)  # use -v to handle values starting with `-`.

    def reset(self):
        return self.wpanctl('reset')

    def status(self):
        return self.wpanctl('status')

    def leave(self):
        return self.wpanctl('leave')

    def form(self, name, channel=None, channel_mask=None, panid=None, xpanid=None, key=None, key_index=None,
            node_type=None, mesh_local_prefix=None, legacy_prefix=None):
        return self.wpanctl('form \"' + name + '\"' +
                            (' -c {}'.format(channel) if channel is not None else '') +
                            (' -m {}'.format(channel_mask) if channel_mask is not None else '') +
                            (' -p {}'.format(panid) if panid is not None else '') +
                            (' -x {}'.format(xpanid) if xpanid is not None else '') +
                            (' -k {}'.format(key) if key is not None else '') +
                            (' -i {}'.format(key_index) if key_index is not None else '') +
                            (' -T {}'.format(node_type) if node_type is not None else '') +
                            (' -M {}'.format(mesh_local_prefix) if mesh_local_prefix is not None else '') +
                            (' -L {}'.format(legacy_prefix) if legacy_prefix is not None else ''))


    def join(self, name, channel=None, node_type=None, panid=None, xpanid=None, key=None):
        return self.wpanctl('join \"' + name + '\"' +
                            (' -c {}'.format(channel) if channel is not None else '') +
                            (' -T {}'.format(node_type) if node_type is not None else '') +
                            (' -p {}'.format(panid) if panid is not None else '') +
                            (' -x {}'.format(xpanid) if xpanid is not None else '') +
                            (' -k {}'.format(key) if key is not None else '') +
                            (' -n'))

    def active_scan(self, channel=None):
        return self.wpanctl('scan' +
                            (' -c {}'.format(channel) if channel is not None else ''))

    def energy_scan(self, channel=None):
        return self.wpanctl('scan -e' +
                            (' -c {}'.format(channel) if channel is not None else ''))

    def discover_scan(self, channel=None, joiner_only=False, enable_filtering=False, panid_filter=None):
        return self.wpanctl('scan -d' +
                            (' -c {}'.format(channel) if channel is not None else '') +
                            (' -j' if joiner_only else '') +
                            (' -e' if enable_filtering else '') +
                            (' -p {}'.format(panid_filter) if panid_filter is not None else ''))

    def permit_join(self, duration_sec=None, port=None, udp=True, tcp=True):
        if not udp and not tcp:  # incorrect use!
            return ''
        traffic_type = ''
        if udp and not tcp:
            traffic_type = ' --udp'
        if tcp and not udp:
            traffic_type = ' --tcp'
        if port is not None and duration_sec is None:
            duration_sec = '240'

        return self.wpanctl('permit-join' +
                            (' {}'.format(duration_sec) if duration_sec is not None else '') +
                            (' {}'.format(port) if port is not None else '') +
                            traffic_type)

    def config_gateway(self, prefix, default_route=False, priority=None):
        return self.wpanctl('config-gateway ' + prefix +
                            (' -d' if default_route else '') +
                            (' -P {}'.format(priority) if priority is not None else ''))

    def add_prefix(self, prefix, prefix_len=None, priority=None, stable=True, on_mesh=False, slaac=False, dhcp=False,
            configure=False, default_route=False, preferred=False):
        return self.wpanctl('add-prefix ' + prefix +
                            (' -l {}'.format(prefix_len) if prefix_len is not None else '') +
                            (' -P {}'.format(priority) if priority is not None else '') +
                            (' -s' if stable else '') +
                            (' -f' if preferred else '') +
                            (' -a' if slaac else '') +
                            (' -d' if dhcp else '') +
                            (' -c' if configure else '') +
                            (' -r' if default_route else '') +
                            (' -o' if on_mesh else ''))

    def remove_prefix(self, prefix, prefix_len=None):
        return self.wpanctl('remove-prefix ' + prefix +
                            (' -l {}'.format(prefix_len) if prefix_len is not None else ''))

    def add_route(self, route_prefix, prefix_len=None, priority=None, stable=True):
        """route priority [(>0 for high, 0 for medium, <0 for low)]"""
        return self.wpanctl('add-route ' + route_prefix +
                            (' -l {}'.format(prefix_len) if prefix_len is not None else '') +
                            (' -p {}'.format(priority) if priority is not None else '') +
                            ('' if stable else '-n'))

    def remove_route(self, route_prefix, prefix_len=None, priority=None, stable=True):
        """route priority [(>0 for high, 0 for medium, <0 for low)]"""
        return self.wpanctl('remove-route ' + route_prefix +
                            (' -l {}'.format(prefix_len) if prefix_len is not None else '') +
                            (' -p {}'.format(priority) if priority is not None else ''))

    #------------------------------------------------------------------------------------------------------------------
    # Helper methods

    def is_associated(self):
        return self.get(WPAN_STATE) == STATE_ASSOCIATED

    def join_node(self, node, node_type=JOIN_TYPE_ROUTER, should_set_key=True):
        """Join a network specified by another node, `node` should be a Node"""

        if not node.is_associated():
            return "{} is not associated".format(node)

        return self.join(
            node.get(WPAN_NAME)[1:-1],
            channel=node.get(WPAN_CHANNEL),
            node_type=node_type,
            panid=node.get(WPAN_PANID),
            xpanid=node.get(WPAN_XPANID),
            key=node.get(WPAN_KEY)[1:-1] if should_set_key else None)

    def whitelist_node(self, node):
        """Adds a given node (of type `Node`) to the whitelist of `self` and enables whitelisting on `self`"""

        self.add(WPAN_MAC_WHITELIST_ENTRIES, node.get(WPAN_EXT_ADDRESS)[1:-1])
        self.set(WPAN_MAC_WHITELIST_ENABLED, '1')

    def un_whitelist_node(self, node):
        """Removes a given node (of node `Node) from the whitelist"""
        self.remove(WPAN_MAC_WHITELIST_ENTRIES, node.get(WPAN_EXT_ADDRESS)[1:-1])

    def is_in_scan_result(self, scan_result):
        """Checks if node is in the scan results
           `scan_result` must be an array of `ScanResult` object (see `parse_scan_result`).
        """
        joinable = (self.get(WPAN_NETWORK_ALLOW_JOIN) == 'true')
        panid = self.get(WPAN_PANID)
        xpanid = self.get(WPAN_XPANID)[2:]
        name = self.get(WPAN_NAME)[1:-1]
        channel = self.get(WPAN_CHANNEL)
        ext_address = self.get(WPAN_EXT_ADDRESS)[1:-1]

        for item in scan_result:
            if all( [item.network_name == name,
                     item.panid == panid,
                     item.xpanid == xpanid,
                     item.channel == channel,
                     item.ext_address == ext_address,
                     (item.type == ScanResult.TYPE_DISCOVERY_SCAN) or (item.joinable == joinable) ] ):
                return True

        return False

    def find_ip6_address_with_prefix(self, prefix):
        """Find an IPv6 address on node matching a given prefix.
           `prefix` should be an string containing the prefix.
           Returns a string containing the IPv6 address matching the prefix or empty string if no address found.
        """
        if len(prefix) > 2 and prefix[-1] == ':' and prefix[-2] == ':':
            prefix = prefix[:-1]
        all_addrs = parse_list(self.get(WPAN_IP6_ALL_ADDRESSES))
        matched_addr = [addr for addr in all_addrs if addr.startswith(prefix)]
        return matched_addr[0] if len(matched_addr) >= 1 else ''

    def add_ip6_address_on_interface(self, address, prefix_len=64):
        """Adds an IPv6 interface on the network interface.
           `address` should be string containing the IPv6 address.
           `prefix_len` is an `int` specifying the prefix length.
           NOTE: this method uses linux `ip` command.
        """
        cmd = 'ip -6 addr add '+ address + '/{} dev '.format(prefix_len) + self.interface_name
        if self._verbose:
            _log('$ Node{} \'{}\')'.format(self._index, cmd))

        result = subprocess.check_output(cmd, shell=True, stderr=subprocess.STDOUT)
        return result

    def remove_ip6_address_on_interface(self, address, prefix_len=64):
        """Removes an IPv6 interface on the network interface.
           `address` should be string containing the IPv6 address.
           `prefix_len` is an `int` specifying the prefix length.
           NOTE: this method uses linux `ip` command.
        """
        cmd = 'ip -6 addr del '+ address + '/{} dev '.format(prefix_len) + self.interface_name
        if self._verbose:
            _log('$ Node{} \'{}\')'.format(self._index, cmd))

        result = subprocess.check_output(cmd, shell=True, stderr=subprocess.STDOUT)
        return result

    #------------------------------------------------------------------------------------------------------------------
    # class methods

    @classmethod
    def init_all_nodes(cls, disable_logs=True, wait_time=15):
        """Issues a `wpanctl.leave` on all `Node` objects and waits for them to be ready"""
        random.seed(123456)
        time.sleep(0.5)
        for node in Node._all_nodes:
            start_time = time.time()
            while True:
                try:
                    node._wpantund_process.poll()
                    if node._wpantund_process.returncode is not None:
                        print 'Node {} wpantund instance has terminated unexpectedly'.format(node)
                    if disable_logs:
                        node.set(WPAN_OT_LOG_LEVEL, '0')
                    node.leave()
                except subprocess.CalledProcessError as e:
                    if (node._verbose):
                        _log(' -> \'{}\' exit code: {}'.format(e.output, e.returncode))
                    interval = time.time() - start_time
                    if interval > wait_time:
                        print 'Took too long to init node {} ({}>{} sec)'.format(node, interval, wait_time)
                        raise
                except:
                    raise
                else:
                    break
                time.sleep(0.4)

    @classmethod
    def finalize_all_nodes(cls):
        """Finalizes all previously created `Node` instances (stops the wpantund process)"""
        for node in Node._all_nodes:
            node._wpantund_process.terminate()
            node._wpantund_process.wait()

    @classmethod
    def set_time_speedup_factor(cls, factor):
        """Sets up the time speed up factor - should be set before creating any `Node` objects"""
        if len(Node._all_nodes) != 0:
            raise Node._NodeError('set_time_speedup_factor() cannot be called after creating a `Node`')
        Node._SPEED_UP_FACTOR = factor

    #------------------------------------------------------------------------------------------------------------------
    # IPv6 message Sender and Receiver class

    class _NodeError(Exception):
        pass

    def prepare_tx(self, src, dst, data=40, count=1, mcast_hops=None):
        """Prepares an IPv6 msg transmission.

        - `src` and `dst` can be either a string containing IPv6 address, or a tuple (ipv6 address as string, port),
           if no port is given, a random port number is used.
        - `data` can be either a string containing the message to be sent, or an int indicating size of the message (a
           random message with the given length will be used).
        - `count` gives number of times the message will be sent (default is 1).
        - `mcast_hops` specifies multicast hop limit (only applicable for multicast tx).

        Returns an `AsyncSender` object.

        """
        if isinstance(src, tuple):
            src_addr = src[0]
            src_port = src[1]
        else:
            src_addr = src
            src_port = random.randint(49152, 65535)

        if isinstance(dst, tuple):
            dst_addr = dst[0]
            dst_port = dst[1]
        else:
            dst_addr = dst
            dst_port = random.randint(49152, 65535)

        if isinstance(data, int):
            # create a random message with the given length.
            all_chars = 'ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789.,><?;:[]=-+)(*&^%$#@'
            msg = ''.join(random.choice(all_chars) for _ in range(data))
        else:
            msg = data

        return AsyncSender(self, src_addr, src_port, dst_addr, dst_port, msg, count, mcast_hops)

    def _get_receiver(self, local_port):
        # Gets or creates a receiver (an `AsyncReceiver`) tied to given port number
        if local_port in self._recvers:
            receiver = self._recvers[local_port]
        else:
            receiver = AsyncReceiver(self, local_port)
            self._recvers[local_port] = receiver
        return receiver

    def _remove_recver(self, recvr):
        # Removes a receiver from weak dictionary - called when the receiver is done and its socket is closed
        local_port = recvr.local_port
        if local_port in self._recvers:
            del self._recvers[local_port]

    def prepare_rx(self, sender):
        """Prepare to receive messages from a sender (an `AsyncSender`)"""
        receiver = self._get_receiver(sender.dst_port)
        receiver._add_sender(sender.src_addr, sender.src_port, sender.msg, sender.count)
        return receiver

    def preapre_listener(self, local_port, timeout=1):
        """Prepares a listener (an `AsyncReceiver`) listening on the given `local_port` for given `timeout` (sec)"""
        receiver = self._get_receiver(local_port)
        receiver._set_listen_timeout(timeout)
        return receiver

    @staticmethod
    def perform_async_tx_rx(timeout=20):
        """Called to perform all previously prepared async rx/listen and tx operations"""
        try:
            start_time = time.time()
            while asyncore.socket_map:
                elapsed_time = time.time() - start_time
                if elapsed_time > timeout:
                    print 'Performing aysnc tx/tx took too long ({}>{} sec)'.format(elapsed_time, timeout)
                    raise Node._NodeError('perform_tx_rx timed out ({}>{} sec)'.format(elapsed_time, timeout))
                # perform a single asyncore loop
                asyncore.loop(timeout=0.5, count=1)
        except:
            print 'Failed to perform async rx/tx'
            raise

#-----------------------------------------------------------------------------------------------------------------------
# `AsyncSender` and `AsyncReceiver classes

_SO_BINDTODEVICE = 25

def _is_ipv6_addr_link_local(ip_addr):
    """Indicates if a given IPv6 address is link-local"""
    return ip_addr.lower().startswith('fe80::')

def _create_socket_address(ip_address, port):
    """Convert a given IPv6 address (string) and port number into a socket address"""
    # `socket.getaddrinfo()` returns a list of `(family, socktype, proto, canonname, sockaddr)` where `sockaddr`
    # (at index 4) can be used as input in socket methods (like `sendto()`, `bind()`, etc.).
    return socket.getaddrinfo(ip_address, port)[0][4]

class AsyncSender(asyncore.dispatcher):
    """ An IPv6 async message sender - use `Node.prepare_tx()` to create one"""

    def __init__(self, node, src_addr, src_port, dst_addr, dst_port, msg, count, mcast_hops=None):
        self._node = node
        self._src_addr = src_addr
        self._src_port = src_port
        self._dst_addr = dst_addr
        self._dst_port = dst_port
        self._msg = msg
        self._count = count
        self._dst_sock_addr = _create_socket_address(dst_addr, dst_port)
        self._tx_buffer = self._msg
        self._tx_counter = 0

        # Create a socket, bind it to the node's interface
        sock = socket.socket(socket.AF_INET6, socket.SOCK_DGRAM)
        sock.setsockopt(socket.SOL_SOCKET, _SO_BINDTODEVICE, node.interface_name + '\0')
        sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEPORT, 1)

        # Set the IPV6_MULTICAST_HOPS
        if mcast_hops is not None:
            sock.setsockopt(socket.IPPROTO_IPV6, socket.IPV6_MULTICAST_HOPS, mcast_hops)

        # Bind the socket to the given src address
        if _is_ipv6_addr_link_local(src_addr):
            # If src is a link local address it requires the interface name to be specified.
            src_sock_addr = _create_socket_address(src_addr + '%' + node.interface_name, src_port)
        else:
            src_sock_addr = _create_socket_address(src_addr, src_port)
        sock.bind(src_sock_addr)

        asyncore.dispatcher.__init__(self, sock)

    # Property getters

    @property
    def node(self):
        return self._node

    @property
    def src_addr(self):
        return self._src_addr

    @property
    def src_port(self):
        return self._src_port

    @property
    def dst_addr(self):
        return self._dst_addr

    @property
    def dst_port(self):
        return self._dst_port

    @property
    def msg(self):
        return self._msg

    @property
    def count(self):
        return self._count

    @property
    def was_successful(self):
        """Indicates if the transmission of IPv6 messages finished successfully"""
        return self._tx_counter == self._count

    # asyncore.dispatcher callbacks

    def readable(self):
        return False

    def writable(self):
        return True

    def handle_write(self):
        sent_len = self.sendto(self._tx_buffer, self._dst_sock_addr)

        if self._node._verbose:
            if sent_len < 30:
                info_text = '{} bytes ("{}")'.format(sent_len, self._tx_buffer[:sent_len])
            else:
                info_text = '{} bytes'.format(sent_len)
            _log('- Node{} sent {} to [{}]:{} from [{}]:{}'.format(self._node._index, info_text,
                                                                   self._dst_addr, self._dst_port,
                                                                   self._src_addr, self._src_port))

        self._tx_buffer = self._tx_buffer[sent_len:]

        if len(self._tx_buffer) == 0:
            self._tx_counter += 1
            if self._tx_counter < self._count:
                self._tx_buffer = self._msg
            else:
                self.handle_close()

    def handle_close(self):
        self.close()

#- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

class AsyncReceiver(asyncore.dispatcher):
    """ An IPv6 async message receiver - use `prepare_rx()` to create one"""

    _MAX_RECV_SIZE = 2048

    class _SenderInfo(object):
        def __init__(self, sender_addr, sender_port, msg, count):
            self._sender_addr = sender_addr
            self._sender_port = sender_port
            self._msg = msg
            self._count = count
            self._rx_counter = 0

        def _check_received(self, msg, sender_addr, sender_port):
            if self._msg == msg and self._sender_addr == sender_addr and self._sender_port == sender_port:
                self._rx_counter += 1
            return self._did_recv_all()

        def _did_recv_all(self):
            return self._rx_counter >= self._count

    def __init__(self, node, local_port):
        self._node = node
        self._local_port = local_port
        self._senders = []        # list of `_SenderInfo` objects
        self._all_rx = []         # contains all received messages as a list of (pkt, (src_addr, src_port))
        self._timeout = 0         # listen timeout (zero means forever)
        self._started = False
        self._start_time = 0

        # Create a socket, bind it to the node's interface
        sock = socket.socket(socket.AF_INET6, socket.SOCK_DGRAM)
        sock.setsockopt(socket.SOL_SOCKET, _SO_BINDTODEVICE, node.interface_name + '\0')
        sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEPORT, 1)

        # Bind the socket to any IPv6 address with the given local port
        local_sock_addr = _create_socket_address('::', local_port)
        sock.bind(local_sock_addr)

        asyncore.dispatcher.__init__(self, sock)

    def _add_sender(self, sender_addr, sender_port, msg, count):
        self._senders.append(AsyncReceiver._SenderInfo(sender_addr, sender_port, msg, count))

    def _set_listen_timeout(self, timeout):
        self._timeout = timeout

    # Property getters

    @property
    def node(self):
        return self._node

    @property
    def local_port(self):
        return self._local_port

    @property
    def all_rx_msg(self):
        """returns all received messages as a list of (msg, (src_addr, src_port))"""
        return self._all_rx

    @property
    def was_successful(self):
        """Indicates if all expected IPv6 messages were received successfully"""
        return len(self._senders) == 0 or all([sender._did_recv_all() for sender in self._senders])

    # asyncore.dispatcher callbacks

    def readable(self):
        if not self._started:
            self._start_time = time.time()
            self._started = True
        if self._timeout != 0 and time.time() - self._start_time >= self._timeout:
            self.handle_close()
            if self._node._verbose:
                _log('- Node{} finished listening on port {} for {} sec, received {} msg(s)'.format(
                    self._node._index, self._local_port, self._timeout, len(self._all_rx)))
            return False
        return True

    def writable(self):
        return False

    def handle_read(self):
        (msg, src_sock_addr) = self.recvfrom(AsyncReceiver._MAX_RECV_SIZE)
        src_addr = src_sock_addr[0]
        src_port = src_sock_addr[1]

        if (_is_ipv6_addr_link_local(src_addr)):
            if '%' in src_addr:
                src_addr = src_addr.split('%')[0]   # remove the interface name from address

        if self._node._verbose:
            if len(msg) < 30:
                info_text = '{} bytes ("{}")'.format(len(msg), msg)
            else:
                info_text = '{} bytes'.format(len(msg))
            _log('- Node{} received {} on port {} from [{}]:{}'.format(self._node._index, info_text,
                                                                       self._local_port,
                                                                       src_addr, src_port))

        self._all_rx.append((msg, (src_addr, src_port)))

        if all([sender._check_received(msg, src_addr, src_port) for sender in self._senders]):
            self.handle_close()

    def handle_close(self):
        self.close()
         # remove the receiver from the node once the socket is closed
        self._node._remove_recver(self)

#-----------------------------------------------------------------------------------------------------------------------
class VerifyError(Exception):
    pass

_is_in_verify_within = False

def verify(condition):
    """Verifies that a `condition` is true, otherwise raises a VerifyError"""
    global _is_in_verify_within
    if not condition:
        calling_frame = inspect.currentframe().f_back
        error_message = 'verify() failed at line {} in "{}"'.format(calling_frame.f_lineno, calling_frame.f_code.co_filename)
        if not _is_in_verify_within:
            print error_message
        raise VerifyError(error_message)

def verify_within(condition_checker_func, wait_time, delay_time=0.1):
    """Verifies that a given function `condition_checker_func` passes successfully within a given wait timeout.
       `wait_time` is maximum time waiting for condition_checker to pass (in seconds).
       `delay_time` specifies a delay interval added between failed attempts (in seconds).
    """
    global _is_in_verify_within
    start_time = time.time()
    old_is_in_verify_within = _is_in_verify_within
    _is_in_verify_within = True
    while True:
        try:
            condition_checker_func()
        except VerifyError as e:
            if time.time() - start_time > wait_time:
                print 'Took too long to pass the condition ({}>{} sec)'.format(time.time() - start_time, wait_time)
                print e.message
                raise e
        except:
            raise
        else:
            break
        if delay_time != 0:
            time.sleep(delay_time)
    _is_in_verify_within = old_is_in_verify_within

#-----------------------------------------------------------------------------------------------------------------------
# Parsing `wpanctl` output

#- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
class ScanResult(object):
    """ This object encapsulates a scan result (active/discover/energy scan)"""

    TYPE_ACTIVE_SCAN     = 'active-scan'
    TYPE_DISCOVERY_SCAN  = 'discover-scan'
    TYPE_ENERGY_SCAN     = 'energy-scan'

    def __init__(self, result_text):

        items = [item.strip() for item in result_text.split('|')]

        if len(items) == 8:
            self._type         = ScanResult.TYPE_ACTIVE_SCAN
            self._index        = items[0]
            self._joinable     = (items[1] == 'YES')
            self._network_name = items[2][1:-1]
            self._panid        = items[3]
            self._channel      = items[4]
            self._xpanid       = items[5]
            self._ext_address  = items[6]
            self._rssi         = items[7]
        elif len(items) == 7:
            self._type         = ScanResult.TYPE_DISCOVERY_SCAN
            self._index        = items[0]
            self._network_name = items[1][1:-1]
            self._panid        = items[2]
            self._channel      = items[3]
            self._xpanid       = items[4]
            self._ext_address  = items[5]
            self._rssi         = items[6]
        elif len(items) == 2:
            self._type         = ScanResult.TYPE_ENERGY_SCAN
            self._channel      = items[0]
            self._rssi         = items[1]
        else:
            raise ValueError('"{}" does not seem to be a valid scan result string'.result_text)

    @property
    def type(self):
        return self._type

    @property
    def joinable(self):
        return self._joinable

    @property
    def network_name(self):
        return self._network_name

    @property
    def panid(self):
        return self._panid

    @property
    def channel(self):
        return self._channel

    @property
    def xpanid(self):
        return self._xpanid

    @property
    def ext_address(self):
        return self._ext_address

    @property
    def rssi(self):
        return self._rssi

    def __repr__(self):
        return 'ScanResult({})'.format(self.__dict__)

def parse_scan_result(scan_result):
    """ Parses scan result string and returns an array of `ScanResult` objects"""
    return [ ScanResult(item) for item in scan_result.split('\n')[2:] ]  # skip first two lines which are table headers

def parse_list(list_string):
    """
    Parses IPv6/prefix/route list string (output of wpanctl get for properties WPAN_IP6_ALL_ADDRESSES,
    IP6_MULTICAST_ADDRESSES, WPAN_THREAD_ON_MESH_PREFIXES, ...)
    Returns an array of strings each containing an IPv6/prefix/route entry.
    """
    # List string example (get(WPAN_IP6_ALL_ADDRESSES) output):
    #
    # '[\n
    # \t"fdf4:5632:4940:0:8798:8701:85d4:e2be     prefix_len:64   origin:ncp      valid:forever   preferred:forever"\n
    # \t"fe80::2092:9358:97ea:71c6                prefix_len:64   origin:ncp      valid:forever   preferred:forever"\n
    # ]'
    #
    # We split the lines ('\n' as separator) and skip the first and last lines which are '['  and ']'.
    # For each line, skip the first two characters (which are '\t"') and last character ('"'), then split the string
    # using whitespace as separator. The first entry is the IPv6 address.
    #
    return [line[2:-1].split()[0] for line in list_string.split('\n')[1:-1]]

#- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
class OnMeshPrefix(object):
    """ This object encapsulates an on-mesh prefix"""

    def __init__(self, text):

        # Example of expected text:
        #
        # '\t"fd00:abba:cafe::       prefix_len:64   origin:user     stable:yes flags:0x31'
        # ' [on-mesh:1 def-route:0 config:0 dhcp:0 slaac:1 pref:1 prio:med] rloc:0x0000"'

        m = re.match('\t"([0-9a-fA-F:]+)\s*prefix_len:(\d+)\s+origin:(\w*)\s+stable:(\w*).* \[' +
                    'on-mesh:(\d)\s+def-route:(\d)\s+config:(\d)\s+dhcp:(\d)\s+slaac:(\d)\s+pref:(\d)\s+prio:(\w*)\]' +
                    '\s+rloc:(0x[0-9a-fA-F]+)',
                     text)
        verify(m is not None)
        data = m.groups()

        self._prefix     = data[0]
        self._prefix_len = data[1]
        self._origin     = data[2]
        self._stable     = (data[3] == 'yes')
        self._on_mesh    = (data[4] == '1')
        self._def_route  = (data[5] == '1')
        self._config     = (data[6] == '1')
        self._dhcp       = (data[7] == '1')
        self._slaac      = (data[8] == '1')
        self._preferred  = (data[9] == '1')
        self._priority   = (data[10])
        self._rloc16     = (data[11])

    @property
    def prefix(self):
        return self._prefix

    @property
    def prefix_len(self):
        return self._prefix_len

    @property
    def origin(self):
        return self._origin

    @property
    def priority(self):
        return self._priority

    def is_stable(self):
        return self._stable

    def is_on_mesh(self):
        return self._on_mesh

    def is_def_route(self):
        return self._def_route

    def is_config(self):
        return self._config

    def is_dhcp(self):
        return self._dhcp

    def is_slaac(self):
        return self._slaac

    def is_preferred(self):
        return self._preferred

    def rloc16(self):
        return self._rloc16

    def __repr__(self):
        return 'OnMeshPrefix({})'.format(self.__dict__)

def parse_on_mesh_prefix_result(on_mesh_prefix_list):
    """ Parses on-mesh prefix list string and returns an array of `OnMeshPrefix` objects"""
    return [ OnMeshPrefix(item) for item in on_mesh_prefix_list.split('\n')[1:-1] ]

#- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
class ChildEntry(object):
    """ This object encapsulates a child entry"""

    def __init__(self, text):

        # Example of expected text:
        #
        # `\t"E24C5F67F4B8CBB9, RLOC16:d402, NetDataVer:175, LQIn:3, AveRssi:-20, LastRssi:-20, Timeout:120, Age:0, `
        # `RxOnIdle:no, FTD:no, SecDataReq:yes, FullNetData:yes"`
        #

        # We get rid of the first two chars `\t"' and last char '"', split the rest using whitespace as separator.
        # Then remove any ',' at end of items in the list.
        items = [item[:-1] if item[-1] ==',' else item for item in text[2:-1].split()]

        # First item in the extended address
        self._ext_address = items[0]

        # Convert the rest into a dictionary by splitting using ':' as separator
        dict = {item.split(':')[0] : item.split(':')[1] for item in items[1:]}

        self._rloc16        = dict['RLOC16']
        self._timeout       = dict['Timeout']
        self._rx_on_idle    = (dict['RxOnIdle'] == 'yes')
        self._ftd           = (dict['FTD'] == 'yes')
        self._sec_data_req  = (dict['SecDataReq'] == 'yes')
        self._full_net_data = (dict['FullNetData'] == 'yes')

    @property
    def ext_address(self):
        return self._ext_address

    @property
    def rloc16(self):
        return self._rloc16

    @property
    def timeout(self):
        return self._timeout

    def is_rx_on_when_idle(self):
        return self._rx_on_idle

    def is_ftd(self):
        return self._ftd

    def is_sec_data_req(self):
        return self._sec_data_req

    def is_full_net_data(self):
        return self._full_net_data

    def __repr__(self):
        return 'ChildEntry({})'.format(self.__dict__)

def parse_child_table_result(child_table_list):
    """ Parses child table list string and returns an array of `ChildEntry` objects"""
    return [ ChildEntry(item) for item in child_table_list.split('\n')[1:-1] ]

#- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
class NeighborEntry(object):
    """ This object encapsulates a neighbor entry"""

    def __init__(self, text):

        # Example of expected text:
        #
        # `\t"5AC95ED4646D6565, RLOC16:9403, LQIn:3, AveRssi:-20, LastRssi:-20, Age:0, LinkFC:8, MleFC:0, IsChild:yes, '
        # 'RxOnIdle:no, FTD:no, SecDataReq:yes, FullNetData:yes"'
        #

        # We get rid of the first two chars `\t"' and last char '"', split the rest using whitespace as separator.
        # Then remove any ',' at end of items in the list.
        items = [item[:-1] if item[-1] ==',' else item for item in text[2:-1].split()]

        # First item in the extended address
        self._ext_address = items[0]

        # Convert the rest into a dictionary by splitting the text using ':' as separator
        dict = {item.split(':')[0] : item.split(':')[1] for item in items[1:]}

        self._rloc16     = dict['RLOC16']
        self._is_child   = (dict['IsChild'] == 'yes')
        self._rx_on_idle = (dict['RxOnIdle'] == 'yes')
        self._ftd        = (dict['FTD'] == 'yes')

    @property
    def ext_address(self):
        return self._ext_address

    @property
    def rloc16(self):
        return self._rloc16

    def is_rx_on_when_idle(self):
        return self._rx_on_idle

    def is_ftd(self):
        return self._ftd

    def is_child(self):
        return self._is_child

    def __repr__(self):
        return 'NeighborEntry({})'.format(self.__dict__)

def parse_neighbor_table_result(neighbor_table_list):
    """ Parses neighbor table list string and returns an array of `NeighborEntry` objects"""
    return [ NeighborEntry(item) for item in neighbor_table_list.split('\n')[1:-1] ]

#- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
class RouterTableEntry(object):
    """ This object encapsulates a router table entry"""

    def __init__(self, text):

        # Example of expected text:
        #
        # `\t"8A970B3251810826, RLOC16:4000, RouterId:16, NextHop:43, PathCost:1, LQIn:3, LQOut:3, Age:3, LinkEst:yes"`
        #

        # We get rid of the first two chars `\t"' and last char '"', split the rest using whitespace as separator.
        # Then remove any ',' at end of items in the list.
        items = [item[:-1] if item[-1] ==',' else item for item in text[2:-1].split()]

        # First item in the extended address
        self._ext_address = items[0]

        # Convert the rest into a dictionary by splitting the text using ':' as separator
        dict = {item.split(':')[0] : item.split(':')[1] for item in items[1:]}

        self._rloc16    = int(dict['RLOC16'], 16)
        self._router_id = int(dict['RouterId'], 0)
        self._next_hop  = int(dict['NextHop'], 0)
        self._path_cost = int(dict['PathCost'], 0)
        self._age       = int(dict['Age'], 0)
        self._le        = (dict['LinkEst'] == 'yes')

    @property
    def ext_address(self):
        return self._ext_address

    @property
    def rloc16(self):
        return self._rloc16

    @property
    def router_id(self):
        return self._router_id

    @property
    def next_hop(self):
        return self._next_hop

    @property
    def path_cost(self):
        return self._path_cost

    def is_link_established(self):
        return self._le

    def __repr__(self):
        return 'RouterTableEntry({})'.format(self.__dict__)

def parse_router_table_result(router_table_list):
    """ Parses router table list string and returns an array of `RouterTableEntry` objects"""
    return [ RouterTableEntry(item) for item in router_table_list.split('\n')[1:-1] ]

#- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
class AddressCacheEntry(object):
    """ This object encapsulates an address cache entry"""

    def __init__(self, text):

        # Example of expected text:
        #
        # '\t"fd00:1234::d427:a1d9:6204:dbae -> 0x9c00, age:0"'
        #

        # We get rid of the first two chars `\t"' and last char '"', split the rest using whitespace as separator.
        # Then remove any ',' at end of items in the list.
        items = [item[:-1] if item[-1] ==',' else item for item in text[2:-1].split()]

        # First item in the extended address
        self._address = items[0]
        self._rloc16    = int(items[2], 16)

        # Convert the rest into a dictionary by splitting the text using ':' as separator
        dict = {item.split(':')[0] : item.split(':')[1] for item in items[3:]}

        self._age       = int(dict['age'], 0)

    @property
    def address(self):
        return self._address

    @property
    def rloc16(self):
        return self._rloc16

    @property
    def age(self):
        return self._age

    def __repr__(self):
        return 'AddressCacheEntry({})'.format(self.__dict__)

def parse_address_cache_table_result(addr_cache_table_list):
    """ Parses address cache table list string and returns an array of `AddressCacheEntry` objects"""
    return [ AddressCacheEntry(item) for item in addr_cache_table_list.split('\n')[1:-1] ]
