#!/usr/bin/env python3
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

import config
import ipaddress
import os
import sys
import pexpect
import pexpect.popen_spawn
import re
import simulator
import socket
import time
import unittest


class Node:

    def __init__(self, nodeid, is_mtd=False, simulator=None):
        self.nodeid = nodeid
        self.verbose = int(float(os.getenv('VERBOSE', 0)))
        self.node_type = os.getenv('NODE_TYPE', 'sim')
        self.simulator = simulator
        if self.simulator:
            self.simulator.add_node(self)

        mode = os.environ.get('USE_MTD') == '1' and is_mtd and 'mtd' or 'ftd'

        if self.node_type == 'soc':
            self.__init_soc(nodeid)
        elif self.node_type == 'ncp-sim':
            # TODO use mode after ncp-mtd is available.
            self.__init_ncp_sim(nodeid, 'ftd')
        else:
            self.__init_sim(nodeid, mode)

        if self.verbose:
            self.pexpect.logfile_read = sys.stdout.buffer

        self._initialized = True

    def __init_sim(self, nodeid, mode):
        """ Initialize a simulation node. """
        if 'OT_CLI_PATH' in os.environ.keys():
            cmd = os.environ['OT_CLI_PATH']
        elif 'top_builddir' in os.environ.keys():
            srcdir = os.environ['top_builddir']
            cmd = '%s/examples/apps/cli/ot-cli-%s' % (srcdir, mode)
        else:
            cmd = './ot-cli-%s' % mode

        if 'RADIO_DEVICE' in os.environ:
            cmd += ' -v %s' % os.environ['RADIO_DEVICE']
            os.environ['NODE_ID'] = str(nodeid)

        cmd += ' %d' % nodeid
        print("%s" % cmd)

        self.pexpect = pexpect.popen_spawn.PopenSpawn(cmd, timeout=4)

        # Add delay to ensure that the process is ready to receive commands.
        timeout = 0.4
        while timeout > 0:
            self.pexpect.send('\r\n')
            try:
                self.pexpect.expect('> ', timeout=0.1)
                break
            except pexpect.TIMEOUT:
                timeout -= 0.1

    def __init_ncp_sim(self, nodeid, mode):
        """ Initialize an NCP simulation node. """
        if 'RADIO_DEVICE' in os.environ:
            args = ' %s' % os.environ['RADIO_DEVICE']
            os.environ['NODE_ID'] = str(nodeid)
        else:
            args = ''

        if 'OT_NCP_PATH' in os.environ.keys():
            cmd = 'spinel-cli.py -p "%s%s" -n' % (
                os.environ['OT_NCP_PATH'],
                args,
            )
        elif "top_builddir" in os.environ.keys():
            builddir = os.environ['top_builddir']
            cmd = 'spinel-cli.py -p "%s/examples/apps/ncp/ot-ncp-%s%s" -n' % (
                builddir,
                mode,
                args,
            )
        else:
            cmd = 'spinel-cli.py -p "./ot-ncp-%s%s" -n' % (mode, args)
        cmd += ' %d' % nodeid
        print("%s" % cmd)

        self.pexpect = pexpect.spawn(cmd, timeout=4)

        # Add delay to ensure that the process is ready to receive commands.
        time.sleep(0.2)
        self._expect('spinel-cli >')
        self.debug(int(os.getenv('DEBUG', '0')))

    def _expect(self, pattern, timeout=-1, *args, **kwargs):
        """ Process simulator events until expected the pattern. """
        if timeout == -1:
            timeout = self.pexpect.timeout

        assert timeout > 0

        while timeout > 0:
            try:
                return self.pexpect.expect(pattern, 0.1, *args, **kwargs)
            except pexpect.TIMEOUT:
                timeout -= 0.1
                self.simulator.go(0)
                if timeout <= 0:
                    raise

    def __init_soc(self, nodeid):
        """ Initialize a System-on-a-chip node connected via UART. """
        import fdpexpect

        serialPort = '/dev/ttyUSB%d' % ((nodeid - 1) * 2)
        self.pexpect = fdpexpect.fdspawn(
            os.open(serialPort, os.O_RDWR | os.O_NONBLOCK | os.O_NOCTTY))

    def __del__(self):
        self.destroy()

    def destroy(self):
        if not self._initialized:
            return

        if (hasattr(self.pexpect, 'proc') and self.pexpect.proc.poll() is None
                or
                not hasattr(self.pexpect, 'proc') and self.pexpect.isalive()):
            print("%d: exit" % self.nodeid)
            self.pexpect.send('exit\n')
            self.pexpect.expect(pexpect.EOF)
            self.pexpect.wait()
            self._initialized = False

    def read_cert_messages_in_commissioning_log(self, timeout=-1):
        """Get the log of the traffic after DTLS handshake.
        """
        format_str = br"=+?\[\[THCI\].*?type=%s.*?\].*?=+?[\s\S]+?-{40,}"
        join_fin_req = format_str % br"JOIN_FIN\.req"
        join_fin_rsp = format_str % br"JOIN_FIN\.rsp"
        dummy_format_str = br"\[THCI\].*?type=%s.*?"
        join_ent_ntf = dummy_format_str % br"JOIN_ENT\.ntf"
        join_ent_rsp = dummy_format_str % br"JOIN_ENT\.rsp"
        pattern = (b"(" + join_fin_req + b")|(" + join_fin_rsp + b")|(" +
                   join_ent_ntf + b")|(" + join_ent_rsp + b")")

        messages = []
        # There are at most 4 cert messages both for joiner and commissioner
        for _ in range(0, 4):
            try:
                self._expect(pattern, timeout=timeout)
                log = self.pexpect.match.group(0)
                messages.append(self._extract_cert_message(log))
            except BaseException:
                break
        return messages

    def _extract_cert_message(self, log):
        res = re.search(br"direction=\w+", log)
        assert res
        direction = res.group(0).split(b'=')[1].strip()

        res = re.search(br"type=\S+", log)
        assert res
        type = res.group(0).split(b'=')[1].strip()

        payload = bytearray([])
        payload_len = 0
        if type in [b"JOIN_FIN.req", b"JOIN_FIN.rsp"]:
            res = re.search(br"len=\d+", log)
            assert res
            payload_len = int(res.group(0).split(b'=')[1].strip())

            hex_pattern = br"\|(\s([0-9a-fA-F]{2}|\.\.))+?\s+?\|"
            while True:
                res = re.search(hex_pattern, log)
                if not res:
                    break
                data = [
                    int(hex, 16)
                    for hex in res.group(0)[1:-1].split(b' ')
                    if hex and hex != b'..'
                ]
                payload += bytearray(data)
                log = log[res.end() - 1:]

        assert len(payload) == payload_len
        return (direction, type, payload)

    def send_command(self, cmd, go=True):
        print("%d: %s" % (self.nodeid, cmd))
        self.pexpect.send(cmd + '\n')
        if go:
            self.simulator.go(0, nodeid=self.nodeid)
        sys.stdout.flush()

    def get_commands(self):
        self.send_command('?')
        self._expect('Commands:')
        commands = []
        while True:
            i = self._expect(['Done', r'(\S+)'])
            if i != 0:
                commands.append(self.pexpect.match.groups()[0])
            else:
                break
        return commands

    def set_mode(self, mode):
        cmd = 'mode %s' % mode
        self.send_command(cmd)
        self._expect('Done')

    def debug(self, level):
        # `debug` command will not trigger interaction with simulator
        self.send_command('debug %d' % level, go=False)

    def start(self):
        self.interface_up()
        self.thread_start()

    def stop(self):
        self.thread_stop()
        self.interface_down()

    def interface_up(self):
        self.send_command('ifconfig up')
        self._expect('Done')

    def interface_down(self):
        self.send_command('ifconfig down')
        self._expect('Done')

    def thread_start(self):
        self.send_command('thread start')
        self._expect('Done')

    def thread_stop(self):
        self.send_command('thread stop')
        self._expect('Done')

    def commissioner_start(self):
        cmd = 'commissioner start'
        self.send_command(cmd)
        self._expect('Done')

    def commissioner_add_joiner(self, addr, psk):
        cmd = 'commissioner joiner add %s %s' % (addr, psk)
        self.send_command(cmd)
        self._expect('Done')

    def joiner_start(self, pskd='', provisioning_url=''):
        cmd = 'joiner start %s %s' % (pskd, provisioning_url)
        self.send_command(cmd)
        self._expect('Done')

    def clear_whitelist(self):
        cmd = 'macfilter addr clear'
        self.send_command(cmd)
        self._expect('Done')

    def enable_whitelist(self):
        cmd = 'macfilter addr whitelist'
        self.send_command(cmd)
        self._expect('Done')

    def disable_whitelist(self):
        cmd = 'macfilter addr disable'
        self.send_command(cmd)
        self._expect('Done')

    def add_whitelist(self, addr, rssi=None):
        cmd = 'macfilter addr add %s' % addr

        if rssi is not None:
            cmd += ' %s' % rssi

        self.send_command(cmd)
        self._expect('Done')

    def remove_whitelist(self, addr):
        cmd = 'macfilter addr remove %s' % addr
        self.send_command(cmd)
        self._expect('Done')

    def get_addr16(self):
        self.send_command('rloc16')
        i = self._expect('([0-9a-fA-F]{4})')
        if i == 0:
            addr16 = int(self.pexpect.match.groups()[0], 16)
        self._expect('Done')
        return addr16

    def get_router_id(self):
        rloc16 = self.get_addr16()
        return rloc16 >> 10

    def get_addr64(self):
        self.send_command('extaddr')
        i = self._expect('([0-9a-fA-F]{16})')
        if i == 0:
            addr64 = self.pexpect.match.groups()[0].decode("utf-8")

        self._expect('Done')
        return addr64

    def get_eui64(self):
        self.send_command('eui64')
        i = self._expect('([0-9a-fA-F]{16})')
        if i == 0:
            addr64 = self.pexpect.match.groups()[0].decode("utf-8")

        self._expect('Done')
        return addr64

    def get_joiner_id(self):
        self.send_command('joiner id')
        i = self._expect('([0-9a-fA-F]{16})')
        if i == 0:
            addr = self.pexpect.match.groups()[0].decode("utf-8")
        self._expect('Done')
        return addr

    def get_channel(self):
        self.send_command('channel')
        i = self._expect(r'(\d+)\r?\n')
        if i == 0:
            channel = int(self.pexpect.match.groups()[0])
        self._expect('Done')
        return channel

    def set_channel(self, channel):
        cmd = 'channel %d' % channel
        self.send_command(cmd)
        self._expect('Done')

    def get_masterkey(self):
        self.send_command('masterkey')
        i = self._expect('([0-9a-fA-F]{32})')
        if i == 0:
            masterkey = self.pexpect.match.groups()[0].decode("utf-8")
        self._expect('Done')
        return masterkey

    def set_masterkey(self, masterkey):
        cmd = 'masterkey %s' % masterkey
        self.send_command(cmd)
        self._expect('Done')

    def get_key_sequence_counter(self):
        self.send_command('keysequence counter')
        i = self._expect(r'(\d+)\r?\n')
        if i == 0:
            key_sequence_counter = int(self.pexpect.match.groups()[0])
        self._expect('Done')
        return key_sequence_counter

    def set_key_sequence_counter(self, key_sequence_counter):
        cmd = 'keysequence counter %d' % key_sequence_counter
        self.send_command(cmd)
        self._expect('Done')

    def set_key_switch_guardtime(self, key_switch_guardtime):
        cmd = 'keysequence guardtime %d' % key_switch_guardtime
        self.send_command(cmd)
        self._expect('Done')

    def set_network_id_timeout(self, network_id_timeout):
        cmd = 'networkidtimeout %d' % network_id_timeout
        self.send_command(cmd)
        self._expect('Done')

    def get_network_name(self):
        self.send_command('networkname')
        while True:
            i = self._expect(['Done', r'(\S+)'])
            if i != 0:
                network_name = self.pexpect.match.groups()[0].decode('utf-8')
            else:
                break
        return network_name

    def set_network_name(self, network_name):
        cmd = 'networkname %s' % network_name
        self.send_command(cmd)
        self._expect('Done')

    def get_panid(self):
        self.send_command('panid')
        i = self._expect('([0-9a-fA-F]{4})')
        if i == 0:
            panid = int(self.pexpect.match.groups()[0], 16)
        self._expect('Done')
        return panid

    def set_panid(self, panid=config.PANID):
        cmd = 'panid %d' % panid
        self.send_command(cmd)
        self._expect('Done')

    def get_partition_id(self):
        self.send_command('leaderpartitionid')
        i = self._expect(r'(\d+)\r?\n')
        if i == 0:
            weight = self.pexpect.match.groups()[0]
        self._expect('Done')
        return weight

    def set_partition_id(self, partition_id):
        cmd = 'leaderpartitionid %d' % partition_id
        self.send_command(cmd)
        self._expect('Done')

    def set_router_upgrade_threshold(self, threshold):
        cmd = 'routerupgradethreshold %d' % threshold
        self.send_command(cmd)
        self._expect('Done')

    def set_router_downgrade_threshold(self, threshold):
        cmd = 'routerdowngradethreshold %d' % threshold
        self.send_command(cmd)
        self._expect('Done')

    def release_router_id(self, router_id):
        cmd = 'releaserouterid %d' % router_id
        self.send_command(cmd)
        self._expect('Done')

    def get_state(self):
        states = [r'\ndetached', r'\nchild', r'\nrouter', r'\nleader']
        self.send_command('state')
        match = self._expect(states)
        self._expect('Done')
        return states[match].strip(r'\n')

    def set_state(self, state):
        cmd = 'state %s' % state
        self.send_command(cmd)
        self._expect('Done')

    def get_timeout(self):
        self.send_command('childtimeout')
        i = self._expect(r'(\d+)\r?\n')
        if i == 0:
            timeout = self.pexpect.match.groups()[0]
        self._expect('Done')
        return timeout

    def set_timeout(self, timeout):
        cmd = 'childtimeout %d' % timeout
        self.send_command(cmd)
        self._expect('Done')

    def set_max_children(self, number):
        cmd = 'childmax %d' % number
        self.send_command(cmd)
        self._expect('Done')

    def get_weight(self):
        self.send_command('leaderweight')
        i = self._expect(r'(\d+)\r?\n')
        if i == 0:
            weight = self.pexpect.match.groups()[0]
        self._expect('Done')
        return weight

    def set_weight(self, weight):
        cmd = 'leaderweight %d' % weight
        self.send_command(cmd)
        self._expect('Done')

    def add_ipaddr(self, ipaddr):
        cmd = 'ipaddr add %s' % ipaddr
        self.send_command(cmd)
        self._expect('Done')

    def get_addrs(self):
        addrs = []
        self.send_command('ipaddr')

        while True:
            i = self._expect([r'(\S+(:\S*)+)\r?\n', 'Done'])
            if i == 0:
                addrs.append(self.pexpect.match.groups()[0].decode("utf-8"))
            elif i == 1:
                break

        return addrs

    def get_mleid(self):
        addr = None
        cmd = 'ipaddr mleid'
        self.send_command(cmd)
        i = self._expect(r'(\S+(:\S*)+)\r?\n')
        if i == 0:
            addr = self.pexpect.match.groups()[0].decode("utf-8")
        self._expect('Done')
        return addr

    def get_linklocal(self):
        addr = None
        cmd = 'ipaddr linklocal'
        self.send_command(cmd)
        i = self._expect(r'(\S+(:\S*)+)\r?\n')
        if i == 0:
            addr = self.pexpect.match.groups()[0].decode("utf-8")
        self._expect('Done')
        return addr

    def get_rloc(self):
        addr = None
        cmd = 'ipaddr rloc'
        self.send_command(cmd)
        i = self._expect(r'(\S+(:\S*)+)\r?\n')
        if i == 0:
            addr = self.pexpect.match.groups()[0].decode("utf-8")
        self._expect('Done')
        return addr

    def get_addr(self, prefix):
        network = ipaddress.ip_network(u'%s' % str(prefix))
        addrs = self.get_addrs()

        for addr in addrs:
            if isinstance(addr, bytearray):
                addr = bytes(addr)
            ipv6_address = ipaddress.ip_address(addr)
            if ipv6_address in network:
                return ipv6_address.exploded

        return None

    def get_addr_leader_aloc(self):
        addrs = self.get_addrs()
        for addr in addrs:
            segs = addr.split(':')
            if (segs[4] == '0' and segs[5] == 'ff' and segs[6] == 'fe00' and
                    segs[7] == 'fc00'):
                return addr
        return None

    def get_eidcaches(self):
        eidcaches = []
        self.send_command('eidcache')

        while True:
            i = self._expect([r'([a-fA-F0-9\:]+) ([a-fA-F0-9]+)\r?\n', 'Done'])
            if i == 0:
                eid = self.pexpect.match.groups()[0].decode("utf-8")
                rloc = self.pexpect.match.groups()[1].decode("utf-8")
                eidcaches.append((eid, rloc))
            elif i == 1:
                break

        return eidcaches

    def add_service(self, enterpriseNumber, serviceData, serverData):
        cmd = 'service add %s %s %s' % (
            enterpriseNumber,
            serviceData,
            serverData,
        )
        self.send_command(cmd)
        self._expect('Done')

    def remove_service(self, enterpriseNumber, serviceData):
        cmd = 'service remove %s %s' % (enterpriseNumber, serviceData)
        self.send_command(cmd)
        self._expect('Done')

    def __getLinkLocalAddress(self):
        for ip6Addr in self.get_addrs():
            if re.match(config.LINK_LOCAL_REGEX_PATTERN, ip6Addr, re.I):
                return ip6Addr

        return None

    def __getGlobalAddress(self):
        global_address = []
        for ip6Addr in self.get_addrs():
            if ((not re.match(config.LINK_LOCAL_REGEX_PATTERN, ip6Addr, re.I))
                    and (not re.match(config.MESH_LOCAL_PREFIX_REGEX_PATTERN,
                                      ip6Addr, re.I)) and
                (not re.match(config.ROUTING_LOCATOR_REGEX_PATTERN, ip6Addr,
                              re.I))):
                global_address.append(ip6Addr)

        return global_address

    def __getRloc(self):
        for ip6Addr in self.get_addrs():
            if (re.match(config.MESH_LOCAL_PREFIX_REGEX_PATTERN, ip6Addr, re.I)
                    and re.match(config.ROUTING_LOCATOR_REGEX_PATTERN, ip6Addr,
                                 re.I) and
                    not (re.match(config.ALOC_FLAG_REGEX_PATTERN, ip6Addr,
                                  re.I))):
                return ip6Addr
        return None

    def __getAloc(self):
        aloc = []
        for ip6Addr in self.get_addrs():
            if (re.match(config.MESH_LOCAL_PREFIX_REGEX_PATTERN, ip6Addr, re.I)
                    and re.match(config.ROUTING_LOCATOR_REGEX_PATTERN, ip6Addr,
                                 re.I) and
                    re.match(config.ALOC_FLAG_REGEX_PATTERN, ip6Addr, re.I)):
                aloc.append(ip6Addr)

        return aloc

    def __getMleid(self):
        for ip6Addr in self.get_addrs():
            if re.match(
                    config.MESH_LOCAL_PREFIX_REGEX_PATTERN, ip6Addr,
                    re.I) and not (re.match(
                        config.ROUTING_LOCATOR_REGEX_PATTERN, ip6Addr, re.I)):
                return ip6Addr

        return None

    def get_ip6_address(self, address_type):
        """Get specific type of IPv6 address configured on thread device.

        Args:
            address_type: the config.ADDRESS_TYPE type of IPv6 address.

        Returns:
            IPv6 address string.
        """
        if address_type == config.ADDRESS_TYPE.LINK_LOCAL:
            return self.__getLinkLocalAddress()
        elif address_type == config.ADDRESS_TYPE.GLOBAL:
            return self.__getGlobalAddress()
        elif address_type == config.ADDRESS_TYPE.RLOC:
            return self.__getRloc()
        elif address_type == config.ADDRESS_TYPE.ALOC:
            return self.__getAloc()
        elif address_type == config.ADDRESS_TYPE.ML_EID:
            return self.__getMleid()
        else:
            return None

        return None

    def get_context_reuse_delay(self):
        self.send_command('contextreusedelay')
        i = self._expect(r'(\d+)\r?\n')
        if i == 0:
            timeout = self.pexpect.match.groups()[0]
        self._expect('Done')
        return timeout

    def set_context_reuse_delay(self, delay):
        cmd = 'contextreusedelay %d' % delay
        self.send_command(cmd)
        self._expect('Done')

    def add_prefix(self, prefix, flags, prf='med'):
        cmd = 'prefix add %s %s %s' % (prefix, flags, prf)
        self.send_command(cmd)
        self._expect('Done')

    def remove_prefix(self, prefix):
        cmd = 'prefix remove %s' % prefix
        self.send_command(cmd)
        self._expect('Done')

    def add_route(self, prefix, prf='med'):
        cmd = 'route add %s %s' % (prefix, prf)
        self.send_command(cmd)
        self._expect('Done')

    def remove_route(self, prefix):
        cmd = 'route remove %s' % prefix
        self.send_command(cmd)
        self._expect('Done')

    def register_netdata(self):
        self.send_command('netdataregister')
        self._expect('Done')

    def energy_scan(self, mask, count, period, scan_duration, ipaddr):
        cmd = 'commissioner energy %d %d %d %d %s' % (
            mask,
            count,
            period,
            scan_duration,
            ipaddr,
        )
        self.send_command(cmd)

        if isinstance(self.simulator, simulator.VirtualTime):
            self.simulator.go(8)
            timeout = 1
        else:
            timeout = 8

        self._expect('Energy:', timeout=timeout)

    def panid_query(self, panid, mask, ipaddr):
        cmd = 'commissioner panid %d %d %s' % (panid, mask, ipaddr)
        self.send_command(cmd)

        if isinstance(self.simulator, simulator.VirtualTime):
            self.simulator.go(8)
            timeout = 1
        else:
            timeout = 8

        self._expect('Conflict:', timeout=timeout)

    def scan(self):
        self.send_command('scan')

        results = []
        while True:
            i = self._expect([
                r'\|\s(\S+)\s+\|\s(\S+)\s+\|\s([0-9a-fA-F]{4})\s\|\s([0-9a-fA-F]{16})\s\|\s(\d+)\r?\n',
                'Done',
            ])
            if i == 0:
                results.append(self.pexpect.match.groups())
            else:
                break

        return results

    def ping(self, ipaddr, num_responses=1, size=None, timeout=5):
        cmd = 'ping %s' % ipaddr
        if size is not None:
            cmd += ' %d' % size

        self.send_command(cmd)

        if isinstance(self.simulator, simulator.VirtualTime):
            self.simulator.go(timeout)

        result = True
        try:
            responders = {}
            while len(responders) < num_responses:
                i = self._expect([r'from (\S+):'])
                if i == 0:
                    responders[self.pexpect.match.groups()[0]] = 1
            self._expect('\n')
        except (pexpect.TIMEOUT, socket.timeout):
            result = False
            if isinstance(self.simulator, simulator.VirtualTime):
                self.simulator.sync_devices()

        return result

    def reset(self):
        self.send_command('reset')
        time.sleep(0.1)

    def set_router_selection_jitter(self, jitter):
        cmd = 'routerselectionjitter %d' % jitter
        self.send_command(cmd)
        self._expect('Done')

    def set_active_dataset(
        self,
        timestamp,
        panid=None,
        channel=None,
        channel_mask=None,
        master_key=None,
    ):
        self.send_command('dataset clear')
        self._expect('Done')

        cmd = 'dataset activetimestamp %d' % timestamp
        self.send_command(cmd)
        self._expect('Done')

        if panid is not None:
            cmd = 'dataset panid %d' % panid
            self.send_command(cmd)
            self._expect('Done')

        if channel is not None:
            cmd = 'dataset channel %d' % channel
            self.send_command(cmd)
            self._expect('Done')

        if channel_mask is not None:
            cmd = 'dataset channelmask %d' % channel_mask
            self.send_command(cmd)
            self._expect('Done')

        if master_key is not None:
            cmd = 'dataset masterkey %s' % master_key
            self.send_command(cmd)
            self._expect('Done')

        self.send_command('dataset commit active')
        self._expect('Done')

    def set_pending_dataset(self,
                            pendingtimestamp,
                            activetimestamp,
                            panid=None,
                            channel=None):
        self.send_command('dataset clear')
        self._expect('Done')

        cmd = 'dataset pendingtimestamp %d' % pendingtimestamp
        self.send_command(cmd)
        self._expect('Done')

        cmd = 'dataset activetimestamp %d' % activetimestamp
        self.send_command(cmd)
        self._expect('Done')

        if panid is not None:
            cmd = 'dataset panid %d' % panid
            self.send_command(cmd)
            self._expect('Done')

        if channel is not None:
            cmd = 'dataset channel %d' % channel
            self.send_command(cmd)
            self._expect('Done')

        self.send_command('dataset commit pending')
        self._expect('Done')

    def announce_begin(self, mask, count, period, ipaddr):
        cmd = 'commissioner announce %d %d %d %s' % (
            mask,
            count,
            period,
            ipaddr,
        )
        self.send_command(cmd)
        self._expect('Done')

    def send_mgmt_active_set(
        self,
        active_timestamp=None,
        channel=None,
        channel_mask=None,
        extended_panid=None,
        panid=None,
        master_key=None,
        mesh_local=None,
        network_name=None,
        binary=None,
    ):
        cmd = 'dataset mgmtsetcommand active '

        if active_timestamp is not None:
            cmd += 'activetimestamp %d ' % active_timestamp

        if channel is not None:
            cmd += 'channel %d ' % channel

        if channel_mask is not None:
            cmd += 'channelmask %d ' % channel_mask

        if extended_panid is not None:
            cmd += 'extpanid %s ' % extended_panid

        if panid is not None:
            cmd += 'panid %d ' % panid

        if master_key is not None:
            cmd += 'masterkey %s ' % master_key

        if mesh_local is not None:
            cmd += 'localprefix %s ' % mesh_local

        if network_name is not None:
            cmd += 'networkname %s ' % network_name

        if binary is not None:
            cmd += 'binary %s ' % binary

        self.send_command(cmd)
        self._expect('Done')

    def send_mgmt_pending_set(
        self,
        pending_timestamp=None,
        active_timestamp=None,
        delay_timer=None,
        channel=None,
        panid=None,
        master_key=None,
        mesh_local=None,
        network_name=None,
    ):
        cmd = 'dataset mgmtsetcommand pending '
        if pending_timestamp is not None:
            cmd += 'pendingtimestamp %d ' % pending_timestamp

        if active_timestamp is not None:
            cmd += 'activetimestamp %d ' % active_timestamp

        if delay_timer is not None:
            cmd += 'delaytimer %d ' % delay_timer

        if channel is not None:
            cmd += 'channel %d ' % channel

        if panid is not None:
            cmd += 'panid %d ' % panid

        if master_key is not None:
            cmd += 'masterkey %s ' % master_key

        if mesh_local is not None:
            cmd += 'localprefix %s ' % mesh_local

        if network_name is not None:
            cmd += 'networkname %s ' % network_name

        self.send_command(cmd)
        self._expect('Done')

    def coaps_start_psk(self, psk, pskIdentity):
        cmd = 'coaps psk %s %s' % (psk, pskIdentity)
        self.send_command(cmd)
        self._expect('Done')

        cmd = 'coaps start'
        self.send_command(cmd)
        self._expect('Done')

    def coaps_start_x509(self):
        cmd = 'coaps x509'
        self.send_command(cmd)
        self._expect('Done')

        cmd = 'coaps start'
        self.send_command(cmd)
        self._expect('Done')

    def coaps_set_resource_path(self, path):
        cmd = 'coaps resource %s' % path
        self.send_command(cmd)
        self._expect('Done')

    def coaps_stop(self):
        cmd = 'coaps stop'
        self.send_command(cmd)

        if isinstance(self.simulator, simulator.VirtualTime):
            self.simulator.go(5)
            timeout = 1
        else:
            timeout = 5

        self._expect('Done', timeout=timeout)

    def coaps_connect(self, ipaddr):
        cmd = 'coaps connect %s' % ipaddr
        self.send_command(cmd)

        if isinstance(self.simulator, simulator.VirtualTime):
            self.simulator.go(5)
            timeout = 1
        else:
            timeout = 5

        self._expect('coaps connected', timeout=timeout)

    def coaps_disconnect(self):
        cmd = 'coaps disconnect'
        self.send_command(cmd)
        self._expect('Done')
        self.simulator.go(5)

    def coaps_get(self):
        cmd = 'coaps get test'
        self.send_command(cmd)

        if isinstance(self.simulator, simulator.VirtualTime):
            self.simulator.go(5)
            timeout = 1
        else:
            timeout = 5

        self._expect('coaps response', timeout=timeout)

    def commissioner_mgmtset(self, tlvs_binary):
        cmd = 'commissioner mgmtset binary %s' % tlvs_binary
        self.send_command(cmd)
        self._expect('Done')

    def bytes_to_hex_str(self, src):
        return ''.join(format(x, '02x') for x in src)

    def commissioner_mgmtset_with_tlvs(self, tlvs):
        payload = bytearray()
        for tlv in tlvs:
            payload += tlv.to_hex()
        self.commissioner_mgmtset(self.bytes_to_hex_str(payload))

    def udp_start(self, local_ipaddr, local_port):
        cmd = 'udp open'
        self.send_command(cmd)
        self._expect('Done')

        cmd = 'udp bind %s %s' % (local_ipaddr, local_port)
        self.send_command(cmd)
        self._expect('Done')

    def udp_stop(self):
        cmd = 'udp close'
        self.send_command(cmd)
        self._expect('Done')

    def udp_send(self, bytes, ipaddr, port, success=True):
        cmd = 'udp send %s %d -s %d ' % (ipaddr, port, bytes)
        self.send_command(cmd)
        if success:
            self._expect('Done')
        else:
            self._expect('Error')

    def udp_check_rx(self, bytes_should_rx):
        self._expect('%d bytes' % bytes_should_rx)

    def router_list(self):
        cmd = 'router list'
        self.send_command(cmd)
        self._expect([r'(\d+)((\s\d+)*)'])

        g = self.pexpect.match.groups()
        router_list = g[0] + ' ' + g[1]
        router_list = [int(x) for x in router_list.split()]
        self._expect('Done')
        return router_list

    def router_table(self):
        cmd = 'router table'
        self.send_command(cmd)

        self._expect(r'(.*)Done')
        g = self.pexpect.match.groups()
        output = g[0]
        lines = output.strip().split('\n')
        lines = [l.strip() for l in lines]
        router_table = {}
        for i, line in enumerate(lines):
            if not line.startswith('|') or not line.endswith('|'):
                if i not in (0, 2):
                    # should not happen
                    print("unexpected line %d: %s" % (i, line))

                continue

            line = line[1:][:-1]
            line = [x.strip() for x in line.split('|')]
            if len(line) != 8:
                print("unexpected line %d: %s" % (i, line))
                continue

            try:
                int(line[0])
            except ValueError:
                if i != 1:
                    print("unexpected line %d: %s" % (i, line))
                continue

            id = int(line[0])
            rloc16 = int(line[1], 16)
            nexthop = int(line[2])
            pathcost = int(line[3])
            lqin = int(line[4])
            lqout = int(line[5])
            age = int(line[6])
            emac = str(line[7])

            router_table[id] = {
                'rloc16': rloc16,
                'nexthop': nexthop,
                'pathcost': pathcost,
                'lqin': lqin,
                'lqout': lqout,
                'age': age,
                'emac': emac,
            }

        return router_table


if __name__ == '__main__':
    unittest.main()
