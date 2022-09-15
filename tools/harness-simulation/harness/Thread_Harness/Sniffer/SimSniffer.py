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

import grpc
import ipaddress
import itertools
import json
import netifaces
import select
import socket
import struct
import threading
import time
import win32api
import winreg as wr

from GRLLibs.UtilityModules.ModuleHelper import ModuleHelper
from GRLLibs.UtilityModules.SnifferManager import SnifferManager
from GRLLibs.UtilityModules.TopologyManager import TopologyManager
from Sniffer.ISniffer import ISniffer
from THCI.OpenThread import watched
from simulation.Sniffer.proto import sniffer_pb2
from simulation.Sniffer.proto import sniffer_pb2_grpc

DISCOVERY_ADDR = ('ff02::114', 12345)
IFNAME = ISniffer.ethernet_interface_name

SCAN_TIME = 3

# `socket.IPPROTO_IPV6` only exists in Python 3, so the constant is manually defined.
IPPROTO_IPV6 = 41

# The subkey represents the class of network adapter devices supported by the system,
# which is used to filter physical network cards and avoid virtual network adapters.
WINREG_KEY = r'SYSTEM\CurrentControlSet\Control\Network\{4d36e972-e325-11ce-bfc1-08002be10318}'


# When Harness requires an RF shield box, it will pop up a message box via `ModuleHelper.UIMsgBox`
# Replace the function to add and remove an RF enclosure simulation automatically without popping up a window
def UIMsgBoxDecorator(UIMsgBox, replaceFuncs):

    @staticmethod
    def UIMsgBoxWrapper(msg='Confirm ??',
                        title='User Input Required',
                        inputRequired=False,
                        default='',
                        choices=None,
                        timeout=None,
                        isPrompt=False):
        func = replaceFuncs.get((msg, title))
        if func is None:
            return UIMsgBox(msg, title, inputRequired, default, choices, timeout, isPrompt)
        else:
            return func()

    return UIMsgBoxWrapper


class DeviceManager:

    def __init__(self):
        deviceManager = TopologyManager.m_DeviceManager
        self._devices = {'DUT': deviceManager.AutoDUTDeviceObject.THCI_Object}
        for device_obj in deviceManager.DeviceList.values():
            if device_obj.IsDeviceUsed:
                self._devices[device_obj.DeviceTopologyInfo] = device_obj.THCI_Object

    def __getitem__(self, deviceName):
        device = self._devices.get(deviceName)
        if device is None:
            raise KeyError('Device Name "%s" not found' % deviceName)
        return device


class RFEnclosureManager:

    def __init__(self, deviceNames1, deviceNames2, snifferPartitionId):
        self.deviceNames1 = deviceNames1
        self.deviceNames2 = deviceNames2
        self.snifferPartitionId = snifferPartitionId

    def placeRFEnclosure(self):
        devices = DeviceManager()
        self._partition1 = [devices[role] for role in self.deviceNames1]
        self._partition2 = [devices[role] for role in self.deviceNames2]
        sniffer_denied_partition = self._partition1 if self.snifferPartitionId == 2 else self._partition2

        for device1 in self._partition1:
            for device2 in self._partition2:
                device1.addBlockedNodeId(device2.node_id)
                device2.addBlockedNodeId(device1.node_id)

        for sniffer in SnifferManager.SnifferObjects.values():
            if sniffer.isSnifferCapturing():
                sniffer.filterNodes(device.node_id for device in sniffer_denied_partition)

    def removeRFEnclosure(self):
        for device in itertools.chain(self._partition1, self._partition2):
            device.clearBlockedNodeIds()

        for sniffer in SnifferManager.SnifferObjects.values():
            if sniffer.isSnifferCapturing():
                sniffer.filterNodes(())

        self._partition1 = None
        self._partition2 = None


class SimSniffer(ISniffer):
    replaced_msgbox = False

    @watched
    def __init__(self, **kwargs):
        self.channel = kwargs.get('channel')
        self.addr_port = kwargs.get('addressofDevice')
        self.is_active = False
        self._local_pcapng_location = None

        if self.addr_port is not None:
            # Replace `ModuleHelper.UIMsgBox` only when simulation devices exist
            self._replaceMsgBox()

            self._sniffer = grpc.insecure_channel(self.addr_port)
            self._stub = sniffer_pb2_grpc.SnifferStub(self._sniffer)

            # Close the sniffer only when Harness exits
            win32api.SetConsoleCtrlHandler(self.__disconnect, True)

    @watched
    def _replaceMsgBox(self):
        # Replace the function only once
        if SimSniffer.replaced_msgbox:
            return
        SimSniffer.replaced_msgbox = True

        test_9_2_9_leader = RFEnclosureManager(['Router_1', 'Router_2'], ['DUT', 'Commissioner'], 1)
        test_9_2_9_router = RFEnclosureManager(['DUT', 'Router_2'], ['Leader', 'Commissioner'], 1)
        test_9_2_10_router = RFEnclosureManager(['Leader', 'Commissioner'], ['DUT', 'MED_1', 'SED_1'], 2)

        # Alter the behavior of `ModuleHelper.UIMsgBox` only when it comes to the following test cases:
        #   - Leader 9.2.9
        #   - Router 9.2.9
        #   - Router 9.2.10
        ModuleHelper.UIMsgBox = UIMsgBoxDecorator(
            ModuleHelper.UIMsgBox,
            replaceFuncs={
                ("Place [Router1, Router2 and Sniffer] <br/> or <br/>[DUT and Commissioner] <br/>in an RF enclosure ", "Shield Devices"):
                    test_9_2_9_leader.placeRFEnclosure,
                ("Remove [Router1, Router2 and Sniffer] <br/> or <br/>[DUT and Commissioner] <br/>from RF enclosure ", "Unshield Devices"):
                    test_9_2_9_leader.removeRFEnclosure,
                ("Place [DUT,Router2 and sniffer] <br/> or <br/>[Leader and Commissioner] <br/>in an RF enclosure ", "Shield Devices"):
                    test_9_2_9_router.placeRFEnclosure,
                ("Remove [DUT, Router2 and sniffer] <br/> or <br/>[Leader and Commissioner] <br/>from RF enclosure ", "Unshield Devices"):
                    test_9_2_9_router.removeRFEnclosure,
                ("Place the <br/> [Leader and Commissioner] devices <br/> in an RF enclosure ", "Shield Devices"):
                    test_9_2_10_router.placeRFEnclosure,
                ("Remove <br/>[Leader and Commissioner] <br/>from RF enclosure ", "Unshield Devices"):
                    test_9_2_10_router.removeRFEnclosure,
            })

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
                data, _ = sock.recvfrom(1024)
                data = json.loads(data)
                devs.add((data['add'], data['por']))
            else:
                # Re-send the request, due to unreliability of UDP especially on WLAN
                sock.sendto(('Sniffer').encode(), DISCOVERY_ADDR)

        devs = [SimSniffer(addressofDevice=self._encode_address_port(addr, port), channel=None) for addr, port in devs]
        self.log('List of SimSniffers: %r', devs)

        return devs

    @watched
    def startSniffer(self, channelToCapture, captureFileLocation, includeEthernet=False):
        self.channel = channelToCapture
        self._local_pcapng_location = captureFileLocation

        response = self._stub.Start(sniffer_pb2.StartRequest(channel=self.channel, includeEthernet=includeEthernet))
        if response.status != sniffer_pb2.OK:
            raise RuntimeError('startSniffer error: %s' % sniffer_pb2.Status.Name(response.status))

        self._thread = threading.Thread(target=self._file_sync_main_loop)
        self._thread.setDaemon(True)
        self._thread.start()

        self.is_active = True

    @watched
    def _file_sync_main_loop(self):
        with open(self._local_pcapng_location, 'wb') as f:
            for response in self._stub.TransferPcapng(sniffer_pb2.TransferPcapngRequest()):
                f.write(response.content)
                f.flush()

    @watched
    def stopSniffer(self):
        if not self.is_active:
            return

        response = self._stub.Stop(sniffer_pb2.StopRequest())
        if response.status != sniffer_pb2.OK:
            raise RuntimeError('stopSniffer error: %s' % sniffer_pb2.Status.Name(response.status))

        self._thread.join()

        self.is_active = False

    @watched
    def filterNodes(self, nodeids):
        if not self.is_active:
            return

        request = sniffer_pb2.FilterNodesRequest()
        request.nodeids.extend(nodeids)

        response = self._stub.FilterNodes(request)
        if response.status != sniffer_pb2.OK:
            raise RuntimeError('filterNodes error: %s' % sniffer_pb2.Status.Name(response.status))

    def __disconnect(self, dwCtrlType):
        if self._sniffer is not None:
            self._sniffer.close()

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
        return self.addr_port

    @watched
    def globalReset(self):
        pass

    def log(self, fmt, *args):
        try:
            msg = fmt % args
            print('%s - %s - %s' % ('SimSniffer', time.strftime('%b %d %H:%M:%S'), msg))
        except Exception:
            pass
