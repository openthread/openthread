#!/usr/bin/python
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
    import pexpect
else:
    import ctypes
import unittest

class Node:
    def __init__(self, nodeid):
        self.nodeid = nodeid
        self.verbose = int(float(os.getenv('VERBOSE', 0)))
        self.node_type = os.getenv('NODE_TYPE', 'sim')

        if self.node_type == 'soc':
            self.__init_soc(nodeid)
        elif self.node_type == 'ncp-sim':
            self.__init_ncp_sim(nodeid)
        elif self.node_type == 'win-sim':
            self.__init_win_sim(nodeid)
        else:
            self.__init_sim(nodeid)

        if self.verbose:
            self.pexpect.logfile_read = sys.stdout

        self.clear_whitelist()
        self.disable_whitelist()
        self.set_timeout(100)

    def __init_sim(self, nodeid):
        """ Initialize a simulation node. """
        if "OT_CLI_PATH" in os.environ.keys():
            cmd = os.environ['OT_CLI_PATH']
        elif "top_builddir" in os.environ.keys():
            srcdir = os.environ['top_builddir']
            cmd = '%s/examples/apps/cli/ot-cli-ftd' % srcdir
        else:
            cmd = './ot-cli-ftd'
        cmd += ' %d' % nodeid
        print ("%s" % cmd)

        self.pexpect = pexpect.spawn(cmd, timeout=4)
        self.Api = None

        # Add delay to ensure that the process is ready to receive commands.
        time.sleep(0.2)


    def __init_ncp_sim(self, nodeid):
        """ Initialize an NCP simulation node. """
        if "top_builddir" in os.environ.keys():
            builddir = os.environ['top_builddir']
            if "top_srcdir" in os.environ.keys():
                srcdir = os.environ['top_srcdir']
            else:
                srcdir = os.path.dirname(os.path.realpath(__file__))
                srcdir += "/../../.."
            cmd = 'python %s/tools/spinel-cli/spinel-cli.py -p %s/examples/apps/ncp/ot-ncp -n' % (srcdir, builddir)
        else:
            cmd = './ot-ncp'
        cmd += ' %d' % nodeid
        print ("%s" % cmd)

        self.pexpect = pexpect.spawn(cmd, timeout=4)
        self.Api = None

        time.sleep(0.2)
        self.pexpect.expect('spinel-cli >')
        self.debug(int(os.getenv('DEBUG', '0')))
 
    def __init_soc(self, nodeid):
        """ Initialize a System-on-a-chip node connected via UART. """
        import fdpexpect
        serialPort = '/dev/ttyUSB%d' % ((nodeid-1)*2)
        self.pexpect = fdpexpect.fdspawn(os.open(serialPort, os.O_RDWR|os.O_NONBLOCK|os.O_NOCTTY))
        self.Api = None

    def __del__(self):
        if self.Api:
            self.Api.otNodeFinalize(self.otNode)
        else:
            if self.pexpect.isalive():
                self.send_command('exit')
                self.pexpect.expect(pexpect.EOF)
                self.pexpect.terminate()
                self.pexpect.close(force=True)

    def send_command(self, cmd):
        if self.Api:
            raise OSError("Unsupported on Windows!")
        print ("%d: %s" % (self.nodeid, cmd))
        self.pexpect.sendline(cmd)

    def get_commands(self):
        self.send_command('?')
        self.pexpect.expect('Commands:')
        commands = []
        while True:
            i = self.pexpect.expect(['Done', '(\S+)'])
            if i != 0:
                commands.append(self.pexpect.match.groups()[0])
            else:
                break
        return commands

    def set_mode(self, mode):   
        if self.Api:
            if self.Api.otNodeSetMode(self.otNode, mode.encode('utf-8')) != 0:
                raise OSError("otNodeSetMode failed!")
        else:     
            cmd = 'mode ' + mode
            self.send_command(cmd)
            self.pexpect.expect('Done')

    def debug(self, level):
        self.send_command('debug '+str(level))

    def interface_up(self):
        if self.Api:
            if self.Api.otNodeInterfaceUp(self.otNode) != 0:
                raise OSError("otNodeInterfaceUp failed!")
        else:    
            self.send_command('ifconfig up')
            self.pexpect.expect('Done')

    def interface_down(self):
        if self.Api:
            if self.Api.otNodeInterfaceDown(self.otNode) != 0:
                raise OSError("otNodeInterfaceDown failed!")
        else:    
            self.send_command('ifconfig down')
            self.pexpect.expect('Done')

    def thread_start(self):
        if self.Api:
            if self.Api.otNodeThreadStart(self.otNode) != 0:
                raise OSError("otNodeThreadStart failed!")
        else:    
            self.send_command('thread start')
            self.pexpect.expect('Done')

    def thread_stop(self):
        if self.Api:
            if self.Api.otNodeThreadStop(self.otNode) != 0:
                raise OSError("otNodeThreadStop failed!")
        else:    
            self.send_command('thread stop')
            self.pexpect.expect('Done')
            
    def commissioner_start(self):
        if self.Api:
            if self.Api.otNodeCommissionerStart(self.otNode) != 0:
                raise OSError("otNodeCommissionerStart failed!")
        else:    
            cmd = 'commissioner start'
            self.send_command(cmd)
            self.pexpect.expect('Done')

    def commissioner_add_joiner(self, addr, psk):
        if self.Api:
            if self.Api.otNodeCommissionerJoinerAdd(self.otNode, addr.encode('utf-8'), psk.encode('utf-8')) != 0:
                raise OSError("otNodeCommissionerJoinerAdd failed!")
        else:    
            cmd = 'commissioner joiner add ' + addr + ' ' + psk
            self.send_command(cmd)
            self.pexpect.expect('Done')

    def joiner_start(self, pskd='', provisioning_url=''):
        if self.Api:
            if self.Api.otNodeJoinerStart(self.otNode, pskd.encode('utf-8'), provisioning_url.encode('utf-8')) != 0:
                raise OSError("otNodeJoinerStart failed!")
        else:    
            cmd = 'joiner start ' + pskd + ' ' + provisioning_url
            self.send_command(cmd)
            self.pexpect.expect('Done')

    def start(self):
        self.interface_up()
        self.thread_start()

    def stop(self):
        self.thread_stop()
        self.interface_down()

    def clear_whitelist(self):
        if self.Api:
            if self.Api.otNodeClearWhitelist(self.otNode) != 0:
                raise OSError("otNodeClearWhitelist failed!")
        else:     
            self.send_command('whitelist clear')
            self.pexpect.expect('Done')

    def enable_whitelist(self):
        if self.Api:
            if self.Api.otNodeEnableWhitelist(self.otNode) != 0:
                raise OSError("otNodeEnableWhitelist failed!")
        else:     
            self.send_command('whitelist enable')
            self.pexpect.expect('Done')

    def disable_whitelist(self):
        if self.Api:
            if self.Api.otNodeDisableWhitelist(self.otNode) != 0:
                raise OSError("otNodeDisableWhitelist failed!")
        else:     
            self.send_command('whitelist disable')
            self.pexpect.expect('Done')

    def add_whitelist(self, addr, rssi=None):
        if self.Api:
            if rssi == None:
                rssi = 0
            if self.Api.otNodeAddWhitelist(self.otNode, addr.encode('utf-8'), ctypes.c_byte(rssi)) != 0:
                raise OSError("otNodeAddWhitelist failed!")
        else:     
            cmd = 'whitelist add ' + addr
            if rssi != None:
                cmd += ' ' + str(rssi)
            self.send_command(cmd)
            self.pexpect.expect('Done')

    def remove_whitelist(self, addr):
        if self.Api:
            if self.Api.otNodeRemoveWhitelist(self.otNode, addr.encode('utf-8')) != 0:
                raise OSError("otNodeRemoveWhitelist failed!")
        else:     
            cmd = 'whitelist remove ' + addr
            self.send_command(cmd)
            self.pexpect.expect('Done')

    def get_addr16(self):
        if self.Api:
            return self.Api.otNodeGetAddr16(self.otNode)
        else:
            self.send_command('rloc16')
            i = self.pexpect.expect('([0-9a-fA-F]{4})')
            if i == 0:
                addr16 = int(self.pexpect.match.groups()[0], 16)
            self.pexpect.expect('Done')
            return addr16

    def get_addr64(self):
        if self.Api:
            return self.Api.otNodeGetAddr64(self.otNode).decode('utf-8')
        else:
            self.send_command('extaddr')
            i = self.pexpect.expect('([0-9a-fA-F]{16})')
            if i == 0:
                addr64 = self.pexpect.match.groups()[0].decode("utf-8")
            self.pexpect.expect('Done')
            return addr64

    def get_hashmacaddr(self):
        if self.Api:
            return self.Api.otNodeGetHashMacAddress(self.otNode).decode('utf-8')
        else:
            self.send_command('hashmacaddr')
            i = self.pexpect.expect('([0-9a-fA-F]{16})')
            if i == 0:
                addr = self.pexpect.match.groups()[0].decode("utf-8")
            self.pexpect.expect('Done')
            return addr

    def get_channel(self):
        if self.Api:
            return self.Api.otNodeGetChannel(self.otNode)
        else:
            self.send_command('channel')
            i = self.pexpect.expect('(\d+)\r\n')
            if i == 0:
                channel = int(self.pexpect.match.groups()[0])
            self.pexpect.expect('Done')
            return channel

    def set_channel(self, channel):
        if self.Api:
            if self.Api.otNodeSetChannel(self.otNode, ctypes.c_ubyte(channel)) != 0:
                raise OSError("otNodeSetChannel failed!")
        else:     
            cmd = 'channel %d' % channel
            self.send_command(cmd)
            self.pexpect.expect('Done')

    def get_masterkey(self):
        if self.Api:
            return self.Api.otNodeGetMasterkey(self.otNode).decode("utf-8")
        else:     
            self.send_command('masterkey')
            i = self.pexpect.expect('([0-9a-fA-F]{32})')
            if i == 0:
                masterkey = self.pexpect.match.groups()[0].decode("utf-8")
            self.pexpect.expect('Done')
            return masterkey

    def set_masterkey(self, masterkey):
        if self.Api:
            if self.Api.otNodeSetMasterkey(self.otNode, masterkey.encode('utf-8')) != 0:
                raise OSError("otNodeSetMasterkey failed!")
        else:     
            cmd = 'masterkey ' + masterkey
            self.send_command(cmd)
            self.pexpect.expect('Done')

    def get_key_sequence_counter(self):
        if self.Api:
            return self.Api.otNodeGetKeySequenceCounter(self.otNode)
        else:
            self.send_command('keysequence counter')
            i = self.pexpect.expect('(\d+)\r\n')
            if i == 0:
                key_sequence_counter = int(self.pexpect.match.groups()[0])
            self.pexpect.expect('Done')
            return key_sequence_counter

    def set_key_sequence_counter(self, key_sequence_counter):
        if self.Api:
            if self.Api.otNodeSetKeySequenceCounter(self.otNode, ctypes.c_uint(key_sequence_counter)) != 0:
                raise OSError("otNodeSetKeySequenceCounter failed!")
        else:     
            cmd = 'keysequence counter %d' % key_sequence_counter
            self.send_command(cmd)
            self.pexpect.expect('Done')

    def set_key_switch_guardtime(self, key_switch_guardtime):
        if self.Api:
            if self.Api.otNodeSetKeySwitchGuardTime(self.otNode, ctypes.c_uint(key_switch_guardtime)) != 0:
                raise OSError("otNodeSetKeySwitchGuardTime failed!")
        else:     
            cmd = 'keysequence guardtime %d' % key_switch_guardtime
            self.send_command(cmd)
            self.pexpect.expect('Done')

    def set_network_id_timeout(self, network_id_timeout):
        if self.Api:
            if self.Api.otNodeSetNetworkIdTimeout(self.otNode, ctypes.c_ubyte(network_id_timeout)) != 0:
                raise OSError("otNodeSetNetworkIdTimeout failed!")
        else:     
            cmd = 'networkidtimeout %d' % network_id_timeout
            self.send_command(cmd)
            self.pexpect.expect('Done')

    def get_network_name(self):
        if self.Api:
            return self.Api.otNodeGetNetworkName(self.otNode).decode("utf-8")
        else:     
            self.send_command('networkname')
            while True:
                i = self.pexpect.expect(['Done', '(\S+)'])
                if i != 0:
                    network_name = self.pexpect.match.groups()[0].decode('utf-8')
                else:
                    break
            return network_name

    def set_network_name(self, network_name):
        if self.Api:
            if self.Api.otNodeSetNetworkName(self.otNode, network_name.encode('utf-8')) != 0:
                raise OSError("otNodeSetNetworkName failed!")
        else:     
            cmd = 'networkname ' + network_name
            self.send_command(cmd)
            self.pexpect.expect('Done')

    def get_panid(self):
        if self.Api:
            return int(self.Api.otNodeGetPanId(self.otNode))
        else:
            self.send_command('panid')
            i = self.pexpect.expect('([0-9a-fA-F]{4})')
            if i == 0:
                panid = int(self.pexpect.match.groups()[0], 16)
            self.pexpect.expect('Done')
            return panid

    def set_panid(self, panid):
        if self.Api:
            if self.Api.otNodeSetPanId(self.otNode, ctypes.c_ushort(panid)) != 0:
                raise OSError("otNodeSetPanId failed!")
        else:  
            cmd = 'panid %d' % panid
            self.send_command(cmd)
            self.pexpect.expect('Done')

    def get_partition_id(self):
        if self.Api:
            return int(self.Api.otNodeGetPartitionId(self.otNode))
        else:
            self.send_command('leaderpartitionid')
            i = self.pexpect.expect('(\d+)\r\n')
            if i == 0:
                weight = self.pexpect.match.groups()[0]
            self.pexpect.expect('Done')
            return weight

    def set_partition_id(self, partition_id):
        if self.Api:
            if self.Api.otNodeSetPartitionId(self.otNode, ctypes.c_uint(partition_id)) != 0:
                raise OSError("otNodeSetPartitionId failed!")
        else:  
            cmd = 'leaderpartitionid %d' % partition_id
            self.send_command(cmd)
            self.pexpect.expect('Done')

    def set_router_upgrade_threshold(self, threshold):
        if self.Api:
            if self.Api.otNodeSetRouterUpgradeThreshold(self.otNode, ctypes.c_ubyte(threshold)) != 0:
                raise OSError("otNodeSetRouterUpgradeThreshold failed!")
        else:  
            cmd = 'routerupgradethreshold %d' % threshold
            self.send_command(cmd)
            self.pexpect.expect('Done')

    def set_router_downgrade_threshold(self, threshold):
        if self.Api:
            if self.Api.otNodeSetRouterDowngradeThreshold(self.otNode, ctypes.c_ubyte(threshold)) != 0:
                raise OSError("otNodeSetRouterDowngradeThreshold failed!")
        else:  
            cmd = 'routerdowngradethreshold %d' % threshold
            self.send_command(cmd)
            self.pexpect.expect('Done')

    def release_router_id(self, router_id):
        if self.Api:
            if self.Api.otNodeReleaseRouterId(self.otNode, ctypes.c_ubyte(router_id)) != 0:
                raise OSError("otNodeReleaseRouterId failed!")
        else:  
            cmd = 'releaserouterid %d' % router_id
            self.send_command(cmd)
            self.pexpect.expect('Done')

    def get_state(self):
        if self.Api:
            return self.Api.otNodeGetState(self.otNode).decode('utf-8')
        else:
            states = ['detached', 'child', 'router', 'leader']
            self.send_command('state')
            match = self.pexpect.expect(states)
            self.pexpect.expect('Done')
            return states[match]

    def set_state(self, state):
        if self.Api:
            if self.Api.otNodeSetState(self.otNode, state.encode('utf-8')) != 0:
                raise OSError("otNodeSetState failed!")
        else:  
            cmd = 'state ' + state
            self.send_command(cmd)
            self.pexpect.expect('Done')

    def get_timeout(self):
        if self.Api:
            return int(self.Api.otNodeGetTimeout(self.otNode))
        else:
            self.send_command('childtimeout')
            i = self.pexpect.expect('(\d+)\r\n')
            if i == 0:
                timeout = self.pexpect.match.groups()[0]
            self.pexpect.expect('Done')
            return timeout

    def set_timeout(self, timeout):
        if self.Api:
            if self.Api.otNodeSetTimeout(self.otNode, ctypes.c_uint(timeout)) != 0:
                raise OSError("otNodeSetTimeout failed!")
        else:  
            cmd = 'childtimeout %d' % timeout
            self.send_command(cmd)
            self.pexpect.expect('Done')

    def set_max_children(self, number):
        if self.Api:
            if self.Api.otNodeSetMaxChildren(self.otNode, ctypes.c_ubyte(number)) != 0:
                raise OSError("otNodeSetMaxChildren failed!")
        else:  
            cmd = 'childmax %d' % number
            self.send_command(cmd)
            self.pexpect.expect('Done')

    def get_weight(self):
        if self.Api:
            return int(self.Api.otNodeGetWeight(self.otNode))
        else:
            self.send_command('leaderweight')
            i = self.pexpect.expect('(\d+)\r\n')
            if i == 0:
                weight = self.pexpect.match.groups()[0]
            self.pexpect.expect('Done')
            return weight

    def set_weight(self, weight):
        if self.Api:
            if self.Api.otNodeSetWeight(self.otNode, ctypes.c_ubyte(weight)) != 0:
                raise OSError("otNodeSetWeight failed!")
        else:  
            cmd = 'leaderweight %d' % weight
            self.send_command(cmd)
            self.pexpect.expect('Done')

    def add_ipaddr(self, ipaddr):
        if self.Api:
            if self.Api.otNodeAddIpAddr(self.otNode, ipaddr.encode('utf-8')) != 0:
                raise OSError("otNodeAddIpAddr failed!")
        else:  
            cmd = 'ipaddr add ' + ipaddr
            self.send_command(cmd)
            self.pexpect.expect('Done')

    def get_addrs(self):
        if self.Api:
            return self.Api.otNodeGetAddrs(self.otNode).decode("utf-8").split("\n")
        else:
            addrs = []
            self.send_command('ipaddr')

            while True:
                i = self.pexpect.expect(['(\S+:\S+)\r\n', 'Done'])
                if i == 0:
                    addrs.append(self.pexpect.match.groups()[0].decode("utf-8"))
                elif i == 1:
                    break

            return addrs

    def get_context_reuse_delay(self):
        if self.Api:
            return int(self.Api.otNodeGetContextReuseDelay(self.otNode))
        else:
            self.send_command('contextreusedelay')
            i = self.pexpect.expect('(\d+)\r\n')
            if i == 0:
                timeout = self.pexpect.match.groups()[0]
            self.pexpect.expect('Done')
            return timeout

    def set_context_reuse_delay(self, delay):
        if self.Api:
            if self.Api.otNodeSetContextReuseDelay(self.otNode, ctypes.c_uint(delay)) != 0:
                raise OSError("otNodeSetContextReuseDelay failed!")
        else:  
            cmd = 'contextreusedelay %d' % delay
            self.send_command(cmd)
            self.pexpect.expect('Done')

    def add_prefix(self, prefix, flags, prf = 'med'):
        if self.Api:
            if self.Api.otNodeAddPrefix(self.otNode, prefix.encode('utf-8'), flags.encode('utf-8'), prf.encode('utf-8')) != 0:
                raise OSError("otNodeAddPrefix failed!")
        else:  
            cmd = 'prefix add ' + prefix + ' ' + flags + ' ' + prf
            self.send_command(cmd)
            self.pexpect.expect('Done')

    def remove_prefix(self, prefix):
        if self.Api:
            if self.Api.otNodeRemovePrefix(self.otNode, prefix.encode('utf-8')) != 0:
                raise OSError("otNodeRemovePrefix failed!")
        else:  
            cmd = ' prefix remove ' + prefix
            self.send_command(cmd)
            self.pexpect.expect('Done')

    def add_route(self, prefix, prf = 'med'):
        if self.Api:
            if self.Api.otNodeAddRoute(self.otNode, prefix.encode('utf-8'), prf.encode('utf-8')) != 0:
                raise OSError("otNodeAddRoute failed!")
        else:  
            cmd = 'route add ' + prefix + ' ' + prf
            self.send_command(cmd)
            self.pexpect.expect('Done')

    def remove_route(self, prefix):
        if self.Api:
            if self.Api.otNodeRemoveRoute(self.otNode, prefix.encode('utf-8')) != 0:
                raise OSError("otNodeRemovePrefix failed!")
        else:  
            cmd = 'route remove ' + prefix
            self.send_command(cmd)
            self.pexpect.expect('Done')

    def register_netdata(self):
        if self.Api:
            if self.Api.otNodeRegisterNetdata(self.otNode) != 0:
                raise OSError("otNodeRegisterNetdata failed!")
        else:  
            self.send_command('netdataregister')
            self.pexpect.expect('Done')

    def energy_scan(self, mask, count, period, scan_duration, ipaddr):
        if self.Api:
            if self.Api.otNodeEnergyScan(self.otNode, ctypes.c_uint(mask), ctypes.c_ubyte(count), ctypes.c_ushort(period), ctypes.c_ushort(scan_duration), ipaddr.encode('utf-8')) != 0:
                raise OSError("otNodeEnergyScan failed!")
        else:  
            cmd = 'commissioner energy ' + str(mask) + ' ' + str(count) + ' ' + str(period) + ' ' + str(scan_duration) + ' ' + ipaddr
            self.send_command(cmd)
            self.pexpect.expect('Energy:', timeout=8)

    def panid_query(self, panid, mask, ipaddr):
        if self.Api:
            if self.Api.otNodePanIdQuery(self.otNode, ctypes.c_ushort(panid), ctypes.c_uint(mask), ipaddr.encode('utf-8')) != 0:
                raise OSError("otNodePanIdQuery failed!")
        else:  
            cmd = 'commissioner panid ' + str(panid) + ' ' + str(mask) + ' ' + ipaddr
            self.send_command(cmd)
            self.pexpect.expect('Conflict:', timeout=8)

    def scan(self):
        if self.Api:
            return self.Api.otNodeScan(self.otNode).decode("utf-8").split("\n")
        else:
            self.send_command('scan')

            results = []
            while True:
                i = self.pexpect.expect(['\|\s(\S+)\s+\|\s(\S+)\s+\|\s([0-9a-fA-F]{4})\s\|\s([0-9a-fA-F]{16})\s\|\s(\d+)\r\n',
                                         'Done'])
                if i == 0:
                    results.append(self.pexpect.match.groups())
                else:
                    break

            return results

    def ping(self, ipaddr, num_responses=1, size=None):
        if self.Api:
            if size == None:
                size = 100
            numberOfResponders = self.Api.otNodePing(self.otNode, ipaddr.encode('utf-8'), ctypes.c_ushort(size), ctypes.c_uint(num_responses))
            return numberOfResponders >= num_responses
        else:
            cmd = 'ping ' + ipaddr
            if size != None:
                cmd += ' ' + str(size)

            self.send_command(cmd)

            result = True
            try:
                responders = {}
                while len(responders) < num_responses:
                    i = self.pexpect.expect(['from (\S+):'])
                    if i == 0:
                        responders[self.pexpect.match.groups()[0]] = 1
                self.pexpect.expect('\n')
            except pexpect.TIMEOUT:
                result = False

            return result

    def set_router_selection_jitter(self, jitter):
        if self.Api:
            if self.Api.otNodeSetRouterSelectionJitter(self.otNode, ctypes.c_ubyte(jitter)) != 0:
                raise OSError("otNodeSetRouterSelectionJitter failed!")
        else:  
            cmd = 'routerselectionjitter %d' % jitter
            self.send_command(cmd)
            self.pexpect.expect('Done')

    def set_active_dataset(self, timestamp, panid=None, channel=None, channel_mask=None, master_key=None):
        if self.Api:
            if panid == None:
                panid = 0
            if channel == None:
                channel = 0
            if channel_mask == None:
                channel_mask = 0
            if master_key == None:
                master_key = ""
            if self.Api.otNodeSetActiveDataset(self.otNode, ctypes.c_ulonglong(timestamp), ctypes.c_ushort(panid), ctypes.c_ushort(channel), ctypes.c_uint(channel_mask), master_key.encode('utf-8')) != 0:
                raise OSError("otNodeSetActiveDataset failed!")
        else:
            self.send_command('dataset clear')
            self.pexpect.expect('Done')

            cmd = 'dataset activetimestamp %d' % timestamp
            self.send_command(cmd)
            self.pexpect.expect('Done')

            if panid != None:
                cmd = 'dataset panid %d' % panid
                self.send_command(cmd)
                self.pexpect.expect('Done')

            if channel != None:
                cmd = 'dataset channel %d' % channel
                self.send_command(cmd)
                self.pexpect.expect('Done')

            if channel_mask != None:
                cmd = 'dataset channelmask %d' % channel_mask
                self.send_command(cmd)
                self.pexpect.expect('Done')

            if master_key != None:
                cmd = 'dataset masterkey ' + master_key
                self.send_command(cmd)
                self.pexpect.expect('Done')

            self.send_command('dataset commit active')
            self.pexpect.expect('Done')

    def set_pending_dataset(self, pendingtimestamp, activetimestamp, panid=None, channel=None):
        if self.Api:
            if pendingtimestamp == None:
                pendingtimestamp = 0
            if activetimestamp == None:
                activetimestamp = 0
            if panid == None:
                panid = 0
            if channel == None:
                channel = 0
            if self.Api.otNodeSetPendingDataset(self.otNode, ctypes.c_ulonglong(activetimestamp), ctypes.c_ulonglong(pendingtimestamp), ctypes.c_ushort(panid), ctypes.c_ushort(channel)) != 0:
                raise OSError("otNodeSetPendingDataset failed!")
        else:
            self.send_command('dataset clear')
            self.pexpect.expect('Done')

            cmd = 'dataset pendingtimestamp %d' % pendingtimestamp
            self.send_command(cmd)
            self.pexpect.expect('Done')

            cmd = 'dataset activetimestamp %d' % activetimestamp
            self.send_command(cmd)
            self.pexpect.expect('Done')

            if panid != None:
                cmd = 'dataset panid %d' % panid
                self.send_command(cmd)
                self.pexpect.expect('Done')

            if channel != None:
                cmd = 'dataset channel %d' % channel
                self.send_command(cmd)
                self.pexpect.expect('Done')

            self.send_command('dataset commit pending')
            self.pexpect.expect('Done')

    def announce_begin(self, mask, count, period, ipaddr):
        if self.Api:
            if self.Api.otNodeCommissionerAnnounceBegin(self.otNode, ctypes.c_uint(mask), ctypes.c_ubyte(count), ctypes.c_ushort(period), ipaddr.encode('utf-8')) != 0:
                raise OSError("otNodeCommissionerAnnounceBegin failed!")
        else:
            cmd = 'commissioner announce ' + str(mask) + ' ' + str(count) + ' ' + str(period) + ' ' + ipaddr
            self.send_command(cmd)
            self.pexpect.expect('Done')

    def send_mgmt_active_set(self, active_timestamp=None, channel=None, channel_mask=None, extended_panid=None,
                             panid=None, master_key=None, mesh_local=None, network_name=None, binary=None):
        if self.Api:
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
        else:
            cmd = 'dataset mgmtsetcommand active '

            if active_timestamp != None:
                cmd += 'activetimestamp %d ' % active_timestamp

            if channel != None:
                cmd += 'channel %d ' % channel

            if channel_mask != None:
                cmd += 'channelmask %d ' % channel_mask

            if extended_panid != None:
                cmd += 'extpanid ' + extended_panid + ' '

            if panid != None:
                cmd += 'panid %d ' % panid

            if master_key != None:
                cmd += 'masterkey ' + master_key + ' '

            if mesh_local != None:
                cmd += 'localprefix ' + mesh_local + ' '

            if network_name != None:
                cmd += 'networkname ' + network_name + ' '

            if binary != None:
                cmd += 'binary ' + binary + ' '

            self.send_command(cmd)
            self.pexpect.expect('Done')

    def send_mgmt_pending_set(self, pending_timestamp=None, active_timestamp=None, delay_timer=None, channel=None,
                              panid=None, master_key=None, mesh_local=None, network_name=None):
        if self.Api:
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
            if self.Api.otNodeSendPendingSet(self.otNode, ctypes.c_ulonglong(active_timestamp), ctypes.c_ulonglong(pending_timestamp), ctypes.c_uint(delay_timer), ctypes.c_ushort(panid), ctypes.c_ushort(channel), master_key.encode('utf-8'), mesh_local.encode('utf-8'), network_name.encode('utf-8')) != 0:
                raise OSError("otNodeSendPendingSet failed!")
        else:
            cmd = 'dataset mgmtsetcommand pending '

            if pending_timestamp != None:
                cmd += 'pendingtimestamp %d ' % pending_timestamp

            if active_timestamp != None:
                cmd += 'activetimestamp %d ' % active_timestamp

            if delay_timer != None:
                cmd += 'delaytimer %d ' % delay_timer

            if channel != None:
                cmd += 'channel %d ' % channel

            if panid != None:
                cmd += 'panid %d ' % panid

            if master_key != None:
                cmd += 'masterkey ' + master_key + ' '

            if mesh_local != None:
                cmd += 'localprefix ' + mesh_local + ' '

            if network_name != None:
                cmd += 'networkname ' + network_name + ' '

            self.send_command(cmd)
            self.pexpect.expect('Done')

    def log(self, message):
        if self.Api:
            self.Api.otNodeLog(message)

    def __init_win_sim(self, nodeid):
        """ Initialize an Windows simulation node. """

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
        
        self.Api.otNodeGetHashMacAddress.argtypes = [ctypes.c_void_p]
        self.Api.otNodeGetHashMacAddress.restype = ctypes.c_char_p

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
                                        ctypes.c_uint]
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

if __name__ == '__main__':
    unittest.main()
