#!/usr/bin/env python
#
# Copyright (c) 2022, The OpenThread Authors.
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

import ipaddress
import netifaces
import os
import paramiko
import select
import socket
import struct
import subprocess
import time
import winreg as wr

from ISniffer import ISniffer
from THCI.OpenThread import watched
from simulation.config import (
    REMOTE_PORT,
    REMOTE_USERNAME,
    REMOTE_PASSWORD,
    REMOTE_OT_PATH,
    REMOTE_SNIFFER_OUTPUT_PREFIX,
    EDITCAP_PATH,
)

DISCOVERY_ADDR = ('ff02::114', 12345)

IFNAME = 'WLAN'

SCAN_TIME = 3

# `socket.IPPROTO_IPV6` only exists in Python 3, so the constant is manually defined.
IPPROTO_IPV6 = 41

# The subkey represents the class of network adapter devices supported by the system,
# which is used to filter physical network cards and avoid virtual network adapters.
WINREG_KEY = r'SYSTEM\CurrentControlSet\Control\Network\{4d36e972-e325-11ce-bfc1-08002be10318}'


class SimSniffer(ISniffer):

    @watched
    def __init__(self, **kwargs):
        self.channel = kwargs.get('channel')
        self.ipaddr = kwargs.get('addressofDevice')
        self.is_active = False
        self._local_pcapng_location = None
        self._ssh = None
        self._remote_pcap_location = None
        self._remote_pid = None

    def __repr__(self):
        return '%r' % self.__dict__

    def _get_connection_name_from_guid(self, iface_guids):
        iface_names = ['(unknown)' for i in range(len(iface_guids))]
        reg = wr.ConnectRegistry(None, wr.HKEY_LOCAL_MACHINE)
        reg_key = wr.OpenKey(reg, WINREG_KEY)
        for i in range(len(iface_guids)):
            try:
                reg_subkey = wr.OpenKey(reg_key, iface_guids[i] + r'\Connection')
                iface_names[i] = wr.QueryValueEx(reg_subkey, 'Name')[0]
            except Exception, e:
                pass
        return iface_names

    def _find_index(self, iface_name):
        ifaces_guid = netifaces.interfaces()
        absolute_iface_name = self._get_connection_name_from_guid(ifaces_guid)
        try:
            _required_iface_index = absolute_iface_name.index(iface_name)
            _required_guid = ifaces_guid[_required_iface_index]
            ip = netifaces.ifaddresses(_required_guid)[netifaces.AF_INET6][-1]['addr']
            self.log('Local IP: %s', ip)
            return int(ip.split('%')[1])
        except Exception, e:
            self.log('%r', e)
            self.log('Interface %s not found', iface_name)
            return None

    def _encode_address_port(self, addr, port):
        port = str(port)
        if isinstance(ipaddress.ip_address(addr), ipaddress.IPv6Address):
            return '[' + addr + ']:' + port
        return addr + ':' + port

    @watched
    def discoverSniffer(self):
        sock = socket.socket(socket.AF_INET6, socket.SOCK_DGRAM)

        # Define the output interface of the socket
        ifn = self._find_index(IFNAME)
        if ifn is None:
            self.log('%s interface has not enabled IPv6.', IFNAME)
            return []
        ifn = struct.pack('I', ifn)
        sock.setsockopt(IPPROTO_IPV6, socket.IPV6_MULTICAST_IF, ifn)

        # Send the request
        sock.sendto(('Sniffer').encode(), DISCOVERY_ADDR)

        # Scan for responses
        devs = set()
        start = time.time()
        while time.time() - start < SCAN_TIME:
            if select.select([sock], [], [], 1)[0]:
                addr, _ = sock.recvfrom(1024)
                devs.add(addr)
            else:
                # Re-send the request, due to unreliability of UDP especially on WLAN
                sock.sendto(('Sniffer').encode(), DISCOVERY_ADDR)

        devs = [SimSniffer(addressofDevice=addr, channel=None) for addr in devs]
        self.log('List of SimSniffers: %r', devs)

        return devs

    @watched
    def startSniffer(self, channelToCapture, captureFileLocation, includeEthernet=False):
        self.channel = channelToCapture
        self._local_pcapng_location = captureFileLocation
        self._remote_pcap_location = os.path.join(REMOTE_SNIFFER_OUTPUT_PREFIX, self.ipaddr.split('@')[0] + '.pcap')

        self._ssh = paramiko.SSHClient()
        self._ssh.set_missing_host_key_policy(paramiko.AutoAddPolicy())
        remote_ip = self.ipaddr.split('@')[1]
        self._ssh.connect(remote_ip, port=REMOTE_PORT, username=REMOTE_USERNAME, password=REMOTE_PASSWORD)

        _, stdout, _ = self._ssh.exec_command(
            'echo $$ && exec python3 %s -o %s -c %d' %
            (os.path.join(REMOTE_OT_PATH, 'tools/harness-simulation/posix/sniffer_sim/sniffer.py'),
             self._remote_pcap_location, self.channel))
        self._remote_pid = int(stdout.readline())

        self.log('local pcapng location = %s', self._local_pcapng_location)
        self.log('remote pcap location = %s', self._remote_pcap_location)
        self.log('remote pid = %d', self._remote_pid)

        self.is_active = True

    @watched
    def stopSniffer(self):
        if not self.is_active:
            return
        self.is_active = False

        assert self._ssh is not None
        self._ssh.exec_command('kill -s TERM %d' % self._remote_pid)
        # Wait to make sure the file is closed
        time.sleep(3)

        # Truncate suffix from .pcapng to .pcap
        local_pcap_location = self._local_pcapng_location[:-2]

        with self._ssh.open_sftp() as sftp:
            sftp.get(self._remote_pcap_location, local_pcap_location)

        self._ssh.close()

        cmd = [EDITCAP_PATH, '-F', 'pcapng', local_pcap_location, self._local_pcapng_location]
        self.log('running editcap: %r', cmd)
        subprocess.Popen(cmd).wait()
        self.log('editcap done')

        self._local_pcapng_location = None
        self._ssh = None
        self._remote_pcap_location = None
        self._remote_pid = None

    @watched
    def setChannel(self, channelToCapture):
        self.channel = channelToCapture

    @watched
    def getChannel(self):
        return self.channel

    @watched
    def validateFirmwareVersion(self, device):
        return True

    @watched
    def isSnifferCapturing(self):
        return self.is_active

    @watched
    def getSnifferAddress(self):
        return self.ipaddr

    @watched
    def globalReset(self):
        pass

    def log(self, fmt, *args):
        try:
            msg = fmt % args
            print('%s - %s - %s' % ('SimSniffer', time.strftime('%b %d %H:%M:%S'), msg))
        except Exception:
            pass
