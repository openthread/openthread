#!/usr/bin/env python
#
# Copyright (c) 2016, The OpenThread Authors.
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

'''
>> Thread Host Controller Interface
>> Device : ARM THCI
>> Class : ARM
'''

import sys
import time
import serial
from IThci import IThci
from pexpect_serial import SerialSpawn
from GRLLibs.UtilityModules.Test import Thread_Device_Role, Device_Data_Requirement, MacType
from GRLLibs.UtilityModules.enums import PlatformDiagnosticPacket_Direction, PlatformDiagnosticPacket_Type, AddressType
from GRLLibs.UtilityModules.ModuleHelper import ModuleHelper,ThreadRunner
from GRLLibs.ThreadPacket.PlatformPackets import PlatformDiagnosticPacket, PlatformPackets
from Queue import Queue


class ARM(IThci):
    firmware = 'Sep 9 2016 14:57:36'# keep the consistency with ARM firmware style
    UIStatusMsg = ''
    networkDataRequirement = ''     # indicate Thread devicde requests full or stable network data
    isPowerDown = False             # indicate if Thread device experiences a power down event
    isWhiteListEnabled = False      # indicate if Thread device enables white list filter
    isBlackListEnabled = False      # indicate if Thread device enables black list filter

    #def __init__(self, SerialPort=COMPortName, EUI=MAC_Address):
    def __init__(self, **kwargs):
        """initialize the serial port and default netowrk parameters
        Args:
            **kwargs: Arbitrary keyword aruments
                      Includes 'EUI' and 'SerialPort'
        """
        try:
            self.mac = kwargs.get('EUI')
            self.port = kwargs.get('SerialPort')
            self.UIStatusMsg = self.firmware
            self.networkName = ModuleHelper.Default_NwkName
            self.networkKey = ModuleHelper.Default_NwkKey
            self.channel = ModuleHelper.Default_Channel
            self.panId = ModuleHelper.Default_PanId
            self.xpanId = ModuleHelper.Default_XpanId
            self.AutoDUTEnable = False
            self.provisioningUrl = ''
            self.__logThread = Queue()
            self.__logThreadRunning = False
            self.intialize()
        except Exception, e:
            ModuleHelper.WriteIntoDebugLogger("initialize() Error: " + str(e))

    def __del__(self):
        """close the serial port connection"""
        try:
            self.serial.close()
        except Exception, e:
            ModuleHelper.WriteIntoDebugLogger("delete() Error: " + str(e))

    def __sendCommand(self, cmd):
        """send specific command to reference unit over serial port

        Args:
            cmd: OpenThread CLI string

        Returns:
            Done: successfully send the command to reference unit and parse it
            Value: successfully retrieve the desired value from reference unit
            Error: some errors occur, indicates by the followed specific error number
        """
        try:
            # command retransmit times
            retryTimes = 3
            while retryTimes:
                retryTimes -= 1
                try:
                    self.serial.sendline(cmd)
                    self.serial.expect(cmd + self.serial.linesep)
                except Exception as e:
                    print 'Failed to send command: %s' % str(e)
                else:
                    break

            line = None
            response = []
            while True:
                line = self.serial.readline().strip('\0\r\n\t')
                if line:
                    response.append(line)
                    if line == 'Done':
                        break
            return response
        except Exception, e:
            ModuleHelper.WriteIntoDebugLogger("sendCommand() Error: " + str(e))

    def __getIp6Address(self, addressType):
        """get specific type of IPv6 address configured on thread device

        Args:
            addressType: the specific type of IPv6 address

            link local: link local unicast IPv6 address that's within one-hop scope
            gloal: global unitcast IPv6 address
            rloc: mesh local unicast IPv6 address for routing in thread network
            mesh EID: mesh Endpoint Identifier

        Returns:
            IPv6 address string
        """
        addrType = ['link local', 'global', 'rloc', 'mesh EID']
        addrs = []
        globalAddr = []
        linkLocal64Addr = ''
        linkLocal16Addr = ''
        rlocAddr = ''
        meshEIDAddr = ''

        addrs = self.__sendCommand('ipaddr')
        for ip6Addr in addrs:
            if ip6Addr == 'Done':
                break

            ip6AddrPrefix = ip6Addr.split(':')[0]
            if ip6AddrPrefix == 'fe80':
                # link local address
                if ip6Addr.split(':')[4] != '0':
                    linkLocal64Addr = ip6Addr
                else:
                    linkLocal16Addr = ip6Addr
            elif ip6AddrPrefix == 'fd00':
                # mesh local address
                if ip6Addr.split(':')[4] == '0':
                    # rloc
                    rlocAddr = ip6Addr
                else:
                    # mesh EID
                    meshEIDAddr = ip6Addr
            else:
                # global ipv6 address
                if ip6Addr != None:
                    globalAddr.append(ip6Addr)
                else:
                    pass

        if addressType == addrType[0]:
            return linkLocal64Addr
        elif addressType == addrType[1]:
            return globalAddr
        elif addressType == addrType[2]:
            return rlocAddr
        elif addressType == addrType[3]:
            return meshEIDAddr
        else:
            pass

    def __setDeviceRole(self, role):
        """set thread device role:

        Args:
           role: thread device mode
           r: rx-on-when-idle
           s: secure IEEE 802.15.4 data request
           d: full function device
           n: full network data

        Returns:
           True: successful to set the device role
           False: fail to set the device role
        """
        print 'call __setDeviceRole'
        print role
        try:
            cmd = 'mode %s' % role
            return self.__sendCommand(cmd)[0] == 'Done'
        except Exception, e:
            ModuleHelper.WriteIntoDebugLogger("setDeviceRole() Error: " + str(e))

    def __setRouterUpgradeThreshold(self, iThreshold):
        """set router upgrade threshold

        Args:
            iThreshold: the number of active routers on the Thread network
                        partition below which a REED may decide to become a Router.

        Returns:
            True: successful to set the ROUTER_UPGRADE_THRESHOLD
            False: fail to set ROUTER_UPGRADE_THRESHOLD
        """
        print 'call __setRouterUpgradeThreshold'
        try:
            cmd = 'routerupgradethreshold %s' % str(iThreshold)
            print cmd
            return self.__sendCommand(cmd) == 'Done'
        except Exception, e:
            ModuleHelper.WriteIntoDebugLogger("setRouterUpgradeThreshold() Error: " + str(e))

    def __setRouterDowngradeThreshold(self, iThreshold):
        """set router downgrade threshold

        Args:
            iThreshold: the number of active routers on the Thread network
                        partition above which an active router may decide to
                        become a child.

        Returns:
            True: successful to set the ROUTER_DOWNGRADE_THRESHOLD
            False: fail to set ROUTER_DOWNGRADE_THRESHOLD
        """
        print 'call __setRouterDowngradeThreshold'
        try:
            cmd = 'routerdowngradethreshold %s' % str(iThreshold)
            print cmd
            return self.__sendCommand(cmd) == 'Done'
        except Exception, e:
            ModuleHelper.WriteIntoDebugLogger("setRouterDowngradeThreshold() Error: " + str(e))

    def __setRouterSelectionJitter(self, iRouterJitter):
        """set ROUTER_SELECTION_JITTER parameter for REED to upgrade to Router

        Args:
            iRouterJitter: a random period prior to request Router ID for REED

        Returns:
            True: successful to set the ROUTER_SELECTION_JITTER
            False: fail to set ROUTER_SELECTION_JITTER
        """
        print 'call _setRouterSelectionJitter'
        try:
            cmd = 'routerselectionjitter %s' % str(iRouterJitter)
            print cmd
            return self.__sendCommand(cmd) == 'Done'
        except Exception, e:
            ModuleHelper.WriteIntoDebugLogger("setRouterSelectionJitter() Error: " + str(e))

    def __enableWhiteList(self):
        """enable white list filter

        Returns:
            True: successful to enable white list filter
            False: fail to enable white list filter
        """
        print 'call enableWhiteList'
        try:
            if self.__sendCommand('whitelist enable')[0] == 'Done':
                self.isWhiteListEnabled = True
                return True
            else:
                return False
        except Exception, e:
            ModuleHelper.WriteIntoDebugLogger("enableWhiteList() Error: " + str(e))

    def __enableBlackList(self):
        """enable black list filter

        Returns:
            True: successful to enable black list filter
            False: fail to enable black list filter
        """
        print 'call enableBlackList'
        try:
            if self.__sendCommand('blacklist enable')[0] == 'Done':
                self.isBlackListEnabled = True
                return True
            else:
                return False
        except Exception, e:
            ModuleHelper.WriteIntoDebugLogger("enableBlackList() Error: " + str(e))

    def __disableWhiteList(self):
        """disable white list filter

        Returns:
            True: successful to disable white list filter
            False: fail to disable white list filter
        """
        print 'call disableWhiteList'
        try:
            if self.__sendCommand('whitelist disable')[0] == 'Done':
                self.isWhiteListEnabled = False
                return True
            else:
                return False
        except Exception, e:
            ModuleHelper.WriteIntoDebugLogger("disableWhiteList() Error: " + str(e))

    def __disableBlackList(self):
        """disable black list filter

        Returns:
            True: successful to disable black list filter
            False: fail to disable black list filter
        """
        print 'call disableBlackList'
        try:
            if self.__sendCommand('blacklist disable')[0] == 'Done':
                self.isBlackListEnabled = False
                return True
            else:
                return False
        except Exception, e:
            ModuleHelper.WriteIntoDebugLogger("disableBlackList() Error: " + str(e))

    def __startOpenThread(self):
        """start OpenThread stack

        Returns:
            True: successful to start OpenThread stack and thread interface up
            False: fail to start OpenThread stack
        """
        print 'call startOpenThread'
        try:
            if self.__sendCommand('ifconfig up')[0] == 'Done':
                return self.__sendCommand('thread start')[0] == 'Done'
            else:
                return False
        except Exception, e:
            ModuleHelper.WriteIntoDebugLogger("startOpenThread() Error: " + str(e))

    def __stopOpenThread(self):
        """stop OpenThread stack

        Returns:
            True: successful to stop OpenThread stack and thread interface down
            False: fail to stop OpenThread stack
        """
        print 'call stopOpenThread'
        try:
            if self.__sendCommand('thread stop')[0] == 'Done':
                return self.__sendCommand('ifconfig down')[0] == 'Done'
            else:
                return False
        except Exception, e:
            ModuleHelper.WriteIntoDebugLogger("stopOpenThread() Error: " + str(e))

    def __isOpenThreadRunning(self):
        """check if OpenThread is running or not

        Returns:
            True: OpenThread is running
            False: OpenThread is not running
        """
        print 'call isOpenThreadRunning'
        return self.__sendCommand('state')[0] != 'disabled'

    # rloc16 might be hex string or integer, need to return actual allocated router id
    def __convertRlocToRouterId(self, xRloc16):
        """mapping Rloc16 to router id

        Args:
            xRloc16: hex rloc16 short address

        Returns:
            actual router id allocated by leader
        """
        routerList = []
        routerList = self.__sendCommand('router list')[0].split()
        print routerList
        print xRloc16

        for index in routerList:
            router = []
            cmd = 'router %s' % index
            router = self.__sendCommand(cmd)

            for line in router:
                if 'Done' in line:
                    break
                elif 'Router ID' in line:
                    routerid = line.split()[2]
                elif 'Rloc' in line:
                    rloc16 = line.split()[1]
                else:
                    pass

            # process input rloc16
            if isinstance(xRloc16, str):
                rloc16 = '0x' + rloc16
                if rloc16 == xRloc16:
                    return routerid
            elif isinstance(xRloc16, int):
                if int(rloc16, 16) == xRloc16:
                    return routerid
            else:
                pass

        return None

    def __convertIp6PrefixStringToIp6Address(self, strIp6Prefix):
        """convert IPv6 prefix string to IPv6 dotted-quad format
           for example:
           2001000000000000 -> 2001::

        Args:
            strIp6Prefix: IPv6 address string

        Returns:
            IPv6 address dotted-quad format
        """
        return strIp6Prefix[0:4] + '::'

    def __convertLongToString(self, iValue):
        """convert a long hex integer to string
           remove '0x' and 'L' return string

        Args:
            iValue: long integer in hex format

        Returns:
            string of this long integer without "0x" and "L"
        """
        string = ''
        strValue = str(hex(iValue))

        string = strValue.lstrip('0x')
        string = string.rstrip('L')

        return string

    def __readCommissioningLogs(self, durationInSeconds):
        """read logs during the commissioning process

        Args:
            durationInSeconds: time duration for reading commissioning logs

        Returns:
            Commissioning logs
        """
        self.__logThreadRunning = True
        logs = Queue()
        t_end = time.time() + durationInSeconds
        while time.time() < t_end:
            try:
                line = self.serial.readline()
                if line:
                    print line
                    logs.put(line)
                time.sleep(0.3)

            except Exception,e:
                print e

        self.__logThreadRunning = False
        return logs

    def closeConnection(self):
        """close current serial port connection"""
        print '%s call closeConnection' % self.port
        try:
            self.serial.close()
        except Exception, e:
            ModuleHelper.WriteIntoDebugLogger("closeConnection() Error: " + str(e))

    def intialize(self):
        """initialize the serial port with baudrate, timeout parameters"""
        print '%s call intialize' % self.port
        try:
            # init serial port
            serialDevice = serial.Serial(self.port, 115200, timeout=1)
            self.serial = SerialSpawn(serialDevice)
            time.sleep(3)

            if self.serial != None:
                self.deviceConnected = True
        except Exception, e:
            ModuleHelper.WriteIntoDebugLogger("intialize() Error: " + str(e))
            self.deviceConnected = False
            sys.exit()

    def setNetworkName(self, networkName='GRL'):
        """set Thread Network name

        Args:
            networkName: the networkname string to be set

        Returns:
            True: successful to set the Thread Networkname
            False: fail to set the Thread Networkname
        """
        print '%s call setNetworkName' % self.port
        print networkName
        try:
            cmd = 'networkname %s' % networkName
            return self.__sendCommand(cmd)[0] == 'Done'
        except Exception, e:
            ModuleHelper.WriteIntoDebugLogger("setNetworkName() Error: " + str(e))

    def getNetworkName(self):
        """get Thread Network name"""
        print '%s call getNetworkname' % self.port
        return self.__sendCommand('networkname')[0]

    def setChannel(self, channel=11):
        """set channel of Thread device operates on.

        Args:
            channel:
                    (0  - 10: Reserved)
                    (11 - 26: 2.4GHz channels)
                    (27 - 65535: Reserved)

        Returns:
            True: successful to set the channel
            False: fail to set the channel
        """
        print '%s call setChannel' % self.port
        print channel
        try:
            cmd = 'channel %s' % channel
            print cmd
            return self.__sendCommand(cmd)[0] == 'Done'
        except Exception, e:
            ModuleHelper.WriteIntoDebugLogger("setChannel() Error: " + str(e))

    def getChannel(self):
        """get current channel"""
        print '%s call getChannel' % self.port
        return self.__sendCommand('channel')[0]

    def setMAC(self, xEUI):
        """set the extended addresss of Thread device

        Args:
            xEUI: extended address in hex format

        Returns:
            True: successful to set the extended address
            False: fail to set the extended address
        """
        print '%s call setMAC' % self.port
        print xEUI
        address64 = ''
        try:
            if not xEUI:
                address64 = self.mac

            if not isinstance(xEUI, str):
                address64 = self.__convertLongToString(xEUI)

                # prepend 0 at the beginning
                if len(address64) < 16:
                    address64 = address64.zfill(16)
                    print address64
            else:
                address64 = xEUI

            cmd = 'extaddr %s' % address64
            print cmd
            if self.__sendCommand(cmd)[0] == 'Done':
                self.mac = address64
                return True
            else:
                return False
        except Exception, e:
            ModuleHelper.WriteIntoDebugLogger("setMAC() Error: " + str(e))

    def getMAC(self, bType=MacType.RandomMac):
        """get one specific type of MAC address
           currently OpenThread only supports Random MAC address

        Args:
            bType: indicate which kind of MAC address is required

        Returns:
            specific type of MAC address
        """
        print '%s call getMAC' % self.port
        print bType
        # if power down happens, return extended address assigned previously
        if self.isPowerDown:
            macAddr64 = self.mac
        else:
            if bType == MacType.FactoryMac:
                macAddr64 = self.__sendCommand('eui64')[0]
            elif bType == MacType.HashMac:
                macAddr64 = self.__sendCommand('hashmacaddr')[0]
            else:
                macAddr64 = self.__sendCommand('extaddr')[0]
        print macAddr64

        return int(macAddr64, 16)

    def getLL64(self):
        """get link local unicast IPv6 address"""
        print '%s call getLL64' % self.port
        return self.__getIp6Address('link local')

    def getMLEID(self):
        """get mesh local endpoint identifier address"""
        print '%s call getMLEID' % self.port
        return self.__getIp6Address('mesh EID')

    def getRloc16(self):
        """get rloc16 short address"""
        print '%s call getRloc16' % self.port
        rloc16 = self.__sendCommand('rloc16')[0]
        return int(rloc16, 16)

    def getRloc(self):
        """get router locator unicast IPv6 address"""
        print '%s call getRloc' % self.port
        return self.__getIp6Address('rloc')

    def getGlobal(self):
        """get global unicast IPv6 address set
           if configuring mutliple entries
        """
        print '%s call getGlobal' % self.port
        return self.__getIp6Address('global')

    def setNetworkKey(self, key):
        """set Thread Network master key

        Args:
            key: Thread Network master key used in secure the MLE/802.15.4 packet

        Returns:
            True: successful to set the Thread Network master key
            False: fail to set the Thread Network master key
        """
        masterKey = ''
        print '%s call setNetworkKey' % self.port
        print key
        try:
            if not isinstance(key, str):
                masterKey = self.__convertLongToString(key)

                # prpend '0' at the beginning
                if len(masterKey) < 32:
                    masterKey = masterKey.zfill(32)
                    print masterKey

                cmd = 'masterkey %s' %masterKey
            else:
                masterKey = key
                cmd = 'masterkey %s' % masterKey

            self.networkKey = masterKey

            return self.__sendCommand(cmd)[0] == 'Done'
        except Exception, e:
            ModuleHelper.WriteIntoDebugLogger("setNetworkkey() Error: " + str(e))

    def getNetworkKey(self):
        """get the current Thread Network master key"""
        print '%s call getNetwokKey' % self.port
        return self.networkKey

    def addBlockedMAC(self, xEUI):
        """add a given extended address to the black list entry

        Args:
            xEUI: extended address in hex format

        Returns:
            True: successful to add a given extended address to the black list entry
            False: fail to addd a given extended address to the black list entry
        """
        print '%s call addBlockedMAC' % self.port
        print xEUI
        macAddr = self.__convertLongToString(xEUI)
        try:
            # if blocked device is itself
            if macAddr == self.mac:
                print 'block device itself'
                return True

            if not self.isBlackListEnabled:
                self.__enableBlackList()

            cmd = 'blacklist add %s' % macAddr
            print cmd
            return self.__sendCommand(cmd)[0] == 'Done'
        except Exception, e:
            ModuleHelper.WriteIntoDebugLogger("addBlockedMAC() Error: " + str(e))

    def addAllowMAC(self, xEUI):
        """add a given extended address to the white list entry

        Args:
            xEUI: a given extended address in hex format

        Returns:
            True: successful to add a given extended address to the white list entry
            False: fail to add a given extended address to the white list entry
        """
        print '%s call addAllowMAC' % self.port
        print xEUI
        macAddr = self.__convertLongToString(xEUI)
        try:
            if not self.isWhiteListEnabled:
                self.__enableWhiteList()

            cmd = 'whitelist add %s' % macAddr
            print cmd
            return self.__sendCommand(cmd)[0] == 'Done'
        except Exception, e:
            ModuleHelper.WriteIntoDebugLogger("addAllowMAC() Error: " + str(e))

    def clearBlockList(self):
        """clear all entries in black list table

        Returns:
            True: successful to clear the black list
            False: fail to clear the black list
        """
        print '%s call clearBlockList' % self.port

        # remove all entries in black list
        try:
            if self.__sendCommand('blacklist clear')[0] == 'Done':
                self.__disableBlackList()
                return True
            else:
                return False
        except Exception, e:
            ModuleHelper.WriteIntoDebugLogger("clearBlockList() Error: " + str(e))

    def clearAllowList(self):
        """clear all entries in white list table

        Returns:
            True: successful to clearthe white list
            False: fail to clear the white list
        """
        print '%s call clearAllowList' % self.port

        # remove all entries in white list as well as in black list
        try:
            if self.__sendCommand('whitelist clear')[0] == 'Done':
                self.__disableWhiteList()
                self.clearBlockList()
                return True
            else:
                return False
        except Exception, e:
            ModuleHelper.WriteIntoDebugLogger("clearAllowList() Error: " + str(e))

    def getDeviceRole(self):
        """get current device role in Thread Network"""
        print '%s call getDeviceRole' % self.port
        return self.__sendCommand('state')[0]

    def joinNetwork(self, eRoleId):
        """make device ready to join the Thread Network with a given role

        Args:
            eRoleId: a given device role id

        Returns:
            True: ready to set Thread Network parameter for joining desired Network
        """
        print '%s call joinNetwork' % self.port
        print eRoleId
        role = ''
        try:
            if ModuleHelper.LeaderDutChannelFound:
                self.channel = ModuleHelper.Default_Channel

            # FIXME: when Harness call setNetworkDataRequirement()?
            # only sleep end device requires stable netowrkdata now
            if eRoleId == Thread_Device_Role.Leader:
                print 'join as leader'
                role = 'rsdn'
                self.__setRouterSelectionJitter(1)
                if self.AutoDUTEnable is False:
                    # set ROUTER_UPGRADE_THRESHOLD
                    self.__setRouterUpgradeThreshold(32)
                    # set ROUTER_DOWNGRADE_THRESHOLD
                    self.__setRouterDowngradeThreshold(33)
            elif eRoleId == Thread_Device_Role.Router:
                print 'join as router'
                role = 'rsdn'
                self.__setRouterSelectionJitter(1)
                time.sleep(5) #wait for REED upgrade to Router
                if self.AutoDUTEnable is False:
                    # set ROUTER_UPGRADE_THRESHOLD
                    self.__setRouterUpgradeThreshold(33)
                    # set ROUTER_DOWNGRADE_THRESHOLD
                    self.__setRouterDowngradeThreshold(33)
            elif eRoleId == Thread_Device_Role.SED:
                print 'join as sleepy end device'
                role = 's'
                if self.AutoDUTEnable is False:
                    # set data polling rate to 15s for SED
                    self.setPollingRate(15)
            elif eRoleId == Thread_Device_Role.EndDevice:
                print 'join as end device'
                role = 'rsn'
            elif eRoleId == Thread_Device_Role.REED:
                print 'join as REED'
                role = 'rsdn'
                # set ROUTER_UPGRADE_THRESHOLD
                self.__setRouterUpgradeThreshold(0)
            elif eRoleId == Thread_Device_Role.EndDevice_FED:
                # always remain an ED, never request to be a router
                print 'join as FED'
                role = 'rsdn'
                # set ROUTER_UPGRADE_THRESHOLD
                self.__setRouterUpgradeThreshold(0)
            elif eRoleId == Thread_Device_Role.EndDevice_MED:
                print 'join as MED'
                role = 'rsn'
            else:
                pass

            # set Thread device with a given role
            self.__setDeviceRole(role)

            # start OpenThread
            self.__startOpenThread()
            time.sleep(3)

            return True
        except Exception, e:
            ModuleHelper.WriteIntoDebugLogger("joinNetwork() Error: " + str(e))

    def getNetworkFragmentID(self):
        """get current partition id of Thread Network Partition from LeaderData

        Returns:
            The Thread network Partition Id
        """
        print '%s call getNetworkFragmentID' % self.port
        if not self.__isOpenThreadRunning():
            print 'OpenThread is not running'
            return None

        leaderData = []
        leaderData = self.__sendCommand('leaderdata')
        return int(leaderData[0].split()[2], 16)

    def getParentAddress(self):
        """get Thread device's parent extended address and rloc16 short address

        Returns:
            The extended address of parent in hex format
        """
        print '%s call getParentAddress' % self.port
        parentInfo = []
        parentInfo = self.__sendCommand('parent')

        for line in parentInfo:
            if 'Done' in line:
                break
            elif 'Ext Addr' in line:
                eui = line.split()[2]
                print eui
            #elif 'Rloc' in line:
            #    rloc16 = line.split()[1]
            #    print rloc16
            else:
                pass

        return int(eui, 16)

    def powerDown(self):
        """power down the Thread device"""
        print '%s call powerDown' % self.port
        self.isPowerDown = True
        self.serial.sendline('reset')

    def powerUp(self):
        """power up the Thread device"""
        print '%s call powerUp' % self.port
        self.isPowerDown = False

        if not self.__isOpenThreadRunning():
            self.__startOpenThread()

    def reboot(self):
        """reset and rejoin to Thread Network without any timeout

        Returns:
            True: successful to reset and rejoin the Thread Network
            False: fail to reset and rejoin the Thread Network
        """
        print '%s call reboot' % self.port
        try:
            # stop OpenThread
            self.__stopOpenThread()
            # start OpenThread
            self.__startOpenThread()

            time.sleep(3)

            if self.__sendCommand('state')[0] == 'disabled':
                print '[FAIL] reboot'
                return False
            else:
                return True
        except Exception, e:
            ModuleHelper.WriteIntoDebugLogger("reboot() Error: " + str(e))

    def ping(self, destination, length=20):
        """ send ICMPv6 echo request with a given length to a unicast destination
            address

        Args:
            destination: the unicast destination address of ICMPv6 echo request
            length: the size of ICMPv6 echo request payload
        """
        print '%s call ping' % self.port
        print 'destination: %s' %destination
        try:
            cmd = 'ping %s %s' % (destination, str(length))
            print cmd
            self.serial.sendline(cmd)
            self.serial.expect(cmd + self.serial.linesep)
            # wait echo reply
            time.sleep(1)
        except Exception, e:
            ModuleHelper.WriteIntoDebugLogger("ping() Error: " + str(e))

    def multicast_Ping(self, destination, length=20):
        """send ICMPv6 echo request with a given length to a multicast destination
           address

        Args:
            destination: the multicast destination address of ICMPv6 echo request
            length: the size of ICMPv6 echo request payload
        """
        print '%s call multicast_Ping' % self.port
        print 'destination: %s' % destination
        try:
            cmd = 'ping %s %s' % (destination, str(length))
            print cmd
            self.serial.sendline(cmd)
            self.serial.expect(cmd + self.serial.linesep)
            # wait echo reply
            time.sleep(1)
        except Exception, e:
            ModuleHelper.WriteIntoDebugLogger("multicast_ping() Error: " + str(e))

    def getVersionNumber(self):
        """get OpenThread stack firmware version number"""
        print '%s call getVersionNumber' % self.port
        return self.firmware

    def setPANID(self, xPAN):
        """set Thread Network PAN ID

        Args:
            xPAN: a given PAN ID in hex format

        Returns:
            True: successful to set the Thread Network PAN ID
            False: fail to set the Thread Network PAN ID
        """
        print '%s call setPANID' % self.port
        print xPAN
        panid = ''
        try:
            if not isinstance(xPAN, str):
                panid = str(hex(xPAN))
                print panid

            cmd = 'panid %s' % panid
            print cmd
            return self.__sendCommand(cmd)[0] == 'Done'
        except Exception, e:
            ModuleHelper.WriteIntoDebugLogger("setPANID() Error: " + str(e))

    def getPANID(self):
        """get current Thread Network PAN ID"""
        print '%s call getPANID' % self.port
        return self.__sendCommand('panid')[0]

    def reset(self):
        """factory reset"""
        print '%s call reset' % self.port
        try:
            self.serial.sendline('reset')
            time.sleep(3)
        except Exception, e:
            ModuleHelper.WriteIntoDebugLogger("reset() Error: " + str(e))

    def removeRouter(self, xRouterId):
        """kickoff router with a given router id from the Thread Network

        Args:
            xRouterId: a given router id in hex format

        Returns:
            True: successful to remove the router from the Thread Network
            False: fail to remove the router from the Thread Network
        """
        print '%s call removeRouter' % self.port
        print xRouterId
        routerId = ''
        routerId = self.__convertRlocToRouterId(xRouterId)
        print routerId

        if routerId == None:
            print 'no matched xRouterId'
            return False

        try:
            cmd = 'releaserouterid %s' % routerId
            return self.__sendCommand(cmd)[0] == 'Done'
        except Exception, e:
            ModuleHelper.WriteIntoDebugLogger("removeRouter() Error: " + str(e))

    def setDefaultValues(self):
        """set default mandatory Thread Network parameter value"""
        print '%s call setDefaultValues' % self.port
        try:
            self.setChannel(ModuleHelper.Default_Channel)
            self.setMAC(self.mac)
            self.setPANID(self.panId)
            self.setXpanId(self.xpanId)
            self.setNetworkName(self.networkName)
            self.setNetworkKey(self.networkKey)
            self.isWhiteListEnabled = False
            self.isBlackListEnabled = False
            self.firmware = '2016 7 27 13:36:46'
        except Exception, e:
            ModuleHelper.WriteIntoDebugLogger("setDefaultValue() Error: " + str(e))

    def getDeviceConncetionStatus(self):
        """check if serial port connection is ready or not"""
        print '%s call getDeviceConnectionStatus' % self.port
        return self.deviceConnected

    def getPollingRate(self):
        """get data polling rate for sleepy end device"""
        print '%s call getPollingRate' % self.port
        return self.__sendCommand('pollperiod')[0]

    def setPollingRate(self, iPollingRate):
        """set data polling rate for sleepy end device

        Args:
            iPollingRate: data poll period of sleepy end device

        Returns:
            True: successful to set the data polling rate for sleepy end device
            False: fail to set the data polling rate for sleepy end device
        """
        print '%s call setPollingRate' % self.port
        print iPollingRate
        try:
            cmd = 'pollperiod %s' % str(iPollingRate)
            print cmd
            return self.__sendCommand(cmd)[0] == 'Done'
        except Exception, e:
            ModuleHelper.WriteIntoDebugLogger("setPollingRate() Error: " + str(e))

    def setLinkQuality(self, EUIadr, LinkQuality):
        """set custom link quality on a link to Thread device with a given extended address

        Args:
            EUIadr: a given extended address
            LinkQuality: a given custom link quality for child devices
                         link quality/link margin mapping table
                         3: 21 - 255 (dB)
                         2: 11 - 20 (dB)
                         1: 3 - 9 (dB)
                         0: 0 - 2 (dB)

        Returns:
            True: successful to set the link quality on a link to a Thread device
            False: fail to set the link quality on a link to a Thread device
        """
        print '%s call setLinkQuality' % self.port
        print EUIadr
        print LinkQuality
        try:
            # process EUIadr
            euiHex = hex(EUIadr)
            euiStr = str(euiHex)
            euiStr = euiStr.rstrip('L')
            address64 = ''
            if '0x' in euiStr:
                address64 = euiStr.lstrip('0x')

                # prepend 0 at the beginning
                if len(address64) < 16:
                    address64 = address64.zfill(16)
                    print address64

            cmd = 'linkquality %s %s' % (address64, str(LinkQuality))
            print cmd
            return self.__sendCommand(cmd)[0] == 'Done'
        except Exception, e:
            ModuleHelper.WriteIntoDebugLogger("setLinkQuality() Error: " + str(e))

    def removeRouterPrefix(self, prefixEntry):
        """remove the configured prefix on a border router

        Args:
            prefixEntry: a on-mesh prefix entry

        Returns:
            True: successful to remove the prefix entry from border router
            False: fail to remove the prfix entry from border router
        """
        print '%s call removeRouterPrefix' % self.port
        print prefixEntry
        prefix = self.__convertIp6PrefixStringToIp6Address(str(prefixEntry))
        try:
            prefixLen = 64
            cmd = 'prefix remove %s/%d' % (prefix, prefixLen)
            print cmd
            if self.__sendCommand(cmd)[0] == 'Done':
                # send server data ntf to leader
                return self.__sendCommand('netdataregister')[0] == 'Done'
            else:
                return False
        except Exception, e:
            ModuleHelper.WriteIntoDebugLogger("removeRouterPrefix() Error: " + str(e))

    def resetAndRejoin(self, timeout):
        """reset and join back Thread Network with a given timeout delay

        Args:
            timeout: a timeout interval before rejoin Thread Network

        Returns:
            True: successful to reset and rejoin Thread Network
            False: fail to reset and rejoin the Thread Network
        """
        print '%s call resetAndRejoin' % self.port
        print timeout
        try:
            # start OpenThread
            self.__stopOpenThread()
            # wait timeout
            time.sleep(timeout)
            # stop OpenThread
            self.__startOpenThread()

            time.sleep(3)

            if self.__sendCommand('state')[0] == 'disabled':
                print '[FAIL] reset and rejoin'
                return False
            return True
        except Exception, e:
            ModuleHelper.WriteIntoDebugLogger("resetAndRejoin() Error: " + str(e))

    def configBorderRouter(self, P_Prefix, P_stable=1, P_default=1, P_slaac_preferred=0, P_Dhcp=0, P_preference=0, P_on_mesh=1, P_nd_dns=0):
        """configure the border router with a given preifx entry parameters

        Args:
            P_Prefix: IPv6 prefix that is available on the Thread Network
            P_stable: is true if the default router is expected to be stable network data
            P_default: is true if border router offers the default route for P_Prefix
            P_slaac_preferred: is true if Thread device is allowed autoconfigure address using P_Prefix
            P_Dhcp: is true if border router is a DHCPv6 Agent
            P_preference: is two-bit signed integer indicating router preference
            P_on_mesh: is true if P_Prefix is considered to be on-mesh
            P_nd_dns: is true if border router is able to supply DNS information obtained via ND

        Returns:
            True: successful to configure the border router with a given prefix entry
            False: fail to configure the border router with a given prefix entry
        """
        print '%s call configBorderRouter' % self.port
        prefix = self.__convertIp6PrefixStringToIp6Address(str(P_Prefix))
        print prefix
        try:
            parameter = ''
            prf = ''

            if P_slaac_preferred == 1:
                parameter += 'p'
                parameter += 'a'

            if P_stable == 1:
                parameter += 's'

            if P_default == 1:
                parameter += 'r'

            if P_Dhcp == 1:
                parameter += 'd'

            if P_on_mesh == 1:
                parameter += 'o'

            if P_preference == 1:
                prf = 'high'
            elif P_preference == 0:
                prf = 'med'
            elif P_preference == -1:
                prf = 'low'
            else:
                pass

            cmd = 'prefix add %s/64 %s %s' % (prefix, parameter, prf)
            print cmd
            if self.__sendCommand(cmd)[0] == 'Done':
                # if prefix configured before starting OpenThread stack
                # do not send out server data ntf proactively
                if not self.__isOpenThreadRunning():
                    return True
                else:
                    # send server data ntf to leader
                    return self.__sendCommand('netdataregister')[0] == 'Done'
            else:
                return False
        except Exception, e:
            ModuleHelper.WriteIntoDebugLogger("configBorderRouter() Error: " + str(e))

    def setNetworkIDTimeout(self, iNwkIDTimeOut):
        """set networkid timeout for Thread device

        Args:
            iNwkIDTimeOut: a given NETWORK_ID_TIMEOUT

        Returns:
            True: successful to set NETWORK_ID_TIMEOUT
            False: fail to set NETWORK_ID_TIMEOUT
        """
        print '%s call setNetworkIDTimeout' % self.port
        print iNwkIDTimeOut
        iNwkIDTimeOut /= 1000
        try:
            cmd = 'networkidtimeout %s' % str(iNwkIDTimeOut)
            print cmd
            return self.__sendCommand(cmd)[0] == 'Done'
        except Exception, e:
            ModuleHelper.WriteIntoDebugLogger("setNetworkIDTimeout() Error: " + str(e))

    def setKeepAliveTimeOut(self, iTimeOut):
        """set keep alive timeout for device
           has been deprecated and also set SED polling rate

        Args:
            iTimeOut: data poll period for sleepy end device

        Returns:
            True: successful to set the data poll period for SED
            False: fail to set the data poll period for SED
        """
        print '%s call setKeepAliveTimeOut' % self.port
        print iTimeOut
        try:
            cmd = 'pollperiod %s' % str(iTimeOut)
            print cmd
            return self.__sendCommand(cmd)[0] == 'Done'
        except Exception, e:
            ModuleHelper.WriteIntoDebugLogger("setKeepAliveTimeOut() Error: " + str(e))

    def setKeySequenceCounter(self, iKeySequenceValue):
        """ set the Key sequence counter corresponding to Thread Network master key

        Args:
            iKeySequenceValue: key sequence value

        Returns:
            True: successful to set the key sequence
            False: fail to set the key sequence
        """
        print '%s call setKeySequenceCounter' % self.port
        print iKeySequenceValue
        try:
            cmd = 'keysequence %s' % str(iKeySequenceValue)
            if self.__sendCommand(cmd)[0] == 'Done':
                time.sleep(1)
                return True
            else:
                return False
        except Exception, e:
            ModuleHelper.WriteIntoDebugLogger("setKeySequenceCounter() Error; " + str(e))

    def getKeySequenceCounter(self):
        """get current Thread Network key sequence"""
        print '%s call getKeySequenceCounter' % self.port
        keySequence = ''
        keySequence = self.__sendCommand('keysequence')[0]
        return keySequence

    def incrementKeySequenceCounter(self, iIncrementValue=1):
        """increment the key sequence with a given value

        Args:
            iIncrementValue: specific increment value to be added

        Returns:
            True: successful to increment the key sequence with a given value
            False: fail to increment the key sequence with a given value
        """
        print '%s call incrementKeySequenceCounter' % self.port
        print iIncrementValue
        currentKeySeq = ''
        try:
            currentKeySeq = self.getKeySequenceCounter()
            keySequence = int(currentKeySeq, 10) + iIncrementValue
            print keySequence
            return self.setKeySequenceCounter(keySequence)
        except Exception, e:
            ModuleHelper.WriteIntoDebugLogger("incrementKeySequenceCounter() Error: " + str(e))

    def setNetworkDataRequirement(self, eDataRequirement):
        """set whether the Thread device requires the full network data
           or only requires the stable network data

        Args:
            eDataRequirement: is true if requiring the full network data

        Returns:
            True: successful to set the network requirement
        """
        print '%s call setNetworkDataRequirement' % self.port
        print eDataRequirement

        if eDataRequirement == Device_Data_Requirement.ALL_DATA:
            self.networkDataRequirement = 'n'
        return True

    def configExternalRouter(self, P_Prefix, P_stable, R_Preference=0):
        """configure border router with a given external route prefix entry

        Args:
            P_Prefix: IPv6 prefix for the route
            P_Stable: is true if the external route prefix is stable network data
            R_Preference: a two-bit signed integer indicating Router preference
                          1: high
                          0: medium
                         -1: low

        Returns:
            True: successful to configure the border router with a given external route prefix
            False: fail to configure the border router with a given exteranl route prefix
        """
        print '%s call configExternalRouter' % self.port
        print P_Prefix
        stable = ''
        prefix = self.__convertIp6PrefixStringToIp6Address(str(P_Prefix))
        try:
            if R_Preference == 1:
                prf = 'high'
            elif R_Preference == 0:
                prf = 'med'
            elif R_Preference == -1:
                prf = 'low'
            else:
                pass

            if P_stable:
                stable += 's'
                cmd = 'route add %s/64 %s %s' % (prefix, stable, prf)
            else:
                cmd = 'route add %s/64 %s' % (prefix, prf)
            print cmd

            if self.__sendCommand(cmd)[0] == 'Done':
                # send server data ntf to leader
                return self.__sendCommand('netdataregister')[0] == 'Done'
            else:
                False
        except Exception, e:
            ModuleHelper.WriteIntoDebugLogger("configExternalRouter() Error: " + str(e))

    def getNeighbouringRouters(self):
        """get neihbouring routers information

        Returns:
            neihbouring routers' extended address
        """
        print '%s call getNeighbouringRouters' % self.port
        try:
            routerInfo = []
            routerList = []
            routerList = self.__sendCommand('router list')[0].split()
            print routerList

            if 'Done' in routerList:
                print 'no neighbouring routers'
                return None

            for index in routerList:
                router = []
                cmd = 'router %s' % index
                router = self.__sendCommand(cmd)

                for line in router:
                    if 'Done' in line:
                        break
                    #elif 'Rloc' in line:
                    #    rloc16 = line.split()[1]
                    elif 'Ext Addr' in line:
                        eui = line.split()[2]
                        routerInfo.append(int(eui, 16))
                    #elif 'LQI In' in line:
                    #    lqi_in = line.split()[1]
                    #elif 'LQI Out' in line:
                    #    lqi_out = line.split()[1]
                    else:
                        pass

            print routerInfo
            return routerInfo
        except Exception, e:
            ModuleHelper.WriteIntoDebugLogger("getNeighbouringDevice() Error: " + str(e))

    def getChildrenInfo(self):
        """get all children information

        Returns:
            children's extended address
        """
        print '%s call getChildrenInfo' % self.port
        try:
            childrenInfoAll = []
            childrenInfo = {'EUI': 0, 'Rloc16': 0, 'MLEID': ''}
            childrenList = self.__sendCommand('child list')[0].split()
            print childrenList

            if 'Done' in childrenList:
                print 'no children'
                return None

            for index in childrenList:
                cmd = 'child %s' % index
                child = []
                child = self.__sendCommand(cmd)

                for line in child:
                    if 'Done' in line:
                        break
                    elif 'Rloc' in line:
                        rloc16 = line.split()[1]
                    elif 'Ext Addr' in line:
                        eui = line.split()[2]
                    #elif 'Child ID' in line:
                    #    child_id = line.split()[2]
                    #elif 'Mode' in line:
                    #    mode = line.split()[1]
                    else:
                        pass

                childrenInfo['EUI'] = int(eui, 16)
                childrenInfo['Rloc16'] = int(rloc16, 16)
                #children_info['MLEID'] = self.getMLEID()

                childrenInfoAll.append(childrenInfo['EUI'])
                #childrenInfoAll.append(childrenInfo)

            print childrenInfoAll
            return childrenInfoAll
        except Exception, e:
            ModuleHelper.WriteIntoDebugLogger("getChildrenInfo() Error: " + str(e))

    def setXpanId(self, xPanId):
        """set extended PAN ID of Thread Network

        Args:
            xPanId: extended PAN ID in hex format

        Returns:
            True: successful to set the extended PAN ID
            False: fail to set the extended PAN ID
        """
        xpanid = ''
        print '%s call setXpanId' % self.port
        print xPanId
        try:
            if not isinstance(xPanId, str):
                xpanid = self.__convertLongToString(xPanId)

                # prepend '0' at the beginning
                if len(xpanid) < 16:
                    xpanid = xpanid.zfill(16)
                    print xpanid
                    cmd = 'extpanid %s' % xpanid
            else:
                xpanid = xPanId
                cmd = 'extpanid %s' % xpanid

            self.xpanId = xpanid
            return self.__sendCommand(cmd)[0] == 'Done'
        except Exception, e:
            ModuleHelper.WriteIntoDebugLogger("setXpanId() Error: " + str(e))

    def getNeighbouringDevices(self):
        """gets the neighbouring devices' extended address to compute the DUT
           extended addresss automatically

        Returns:
            A list including extended address of neighbouring routers, parent
            as well as children
        """
        print '%s call getNeighbouringDevices' % self.port
        neighbourList = []

        # get parent info
        parentAddr = self.getParentAddress()
        if parentAddr != 0:
            neighbourList.append(parentAddr)

        # get ED/SED children info
        childNeighbours = self.getChildrenInfo()
        if childNeighbours != None and len(childNeighbours) > 0:
            for entry in childNeighbours:
                neighbourList.append(entry)

        # get neighbouring routers info
        routerNeighbours = self.getNeighbouringRouters()
        if routerNeighbours != None and len(routerNeighbours) > 0:
            for entry in routerNeighbours:
                neighbourList.append(entry)

        print neighbourList
        return neighbourList

    def setPartationId(self, partationId):
        """set Thread Network Partition ID

        Args:
            partitionId: partition id to be set by leader

        Returns:
            True: successful to set the Partition ID
            False: fail to set the Partition ID
        """
        print '%s call setPartationId' % self.port
        print partationId

        cmd = 'leaderpartitionid %s' %(str(hex(partationId)).rstrip('L'))
        print cmd
        return self.__sendCommand(cmd)[0] == 'Done'

    def getGUA(self, filterByPrefix=None):
        """get expected global unicast IPv6 address of Thread device

        Args:
            filterByPrefix: a given expected global IPv6 prefix to be matched

        Returns:
            a global IPv6 address that matches with filterByPrefix
            or None if no matched GUA
        """
        print '%s call getGUA' % self.port
        print filterByPrefix
        globalAddrs = []
        try:
            # get global addrs set if multiple
            globalAddrs = self.getGlobal()

            if filterByPrefix == None:
                return globalAddrs[0]
            else:
                for line in globalAddrs:
                    if line.startswith(filterByPrefix):
                        return line
                print 'no global address matched'
                return None
        except Exception, e:
            ModuleHelper.WriteIntoDebugLogger("getGUA() Error: " + str(e))

    def getShortAddress(self):
        """get Rloc16 short address of Thread device"""
        print '%s call getShortAddress' % self.port
        return self.getRloc16()

    def getULA64(self):
        """get mesh local EID of Thread device"""
        print '%s call getULA64' % self.port
        return self.getMLEID()

    def setMLPrefix(self, sMeshLocalPrefix):
        """set mesh local prefix"""
        print '%s call setMLPrefix' % self.port

    def getML16(self):
        """get mesh local 16 unicast address (Rloc)"""
        print '%s call getML16' % self.port
        return self.getRloc()

    def downgradeToDevice(self):
        pass

    def upgradeToRouter(self):
        pass

    def forceSetSlaac(self, slaacAddress):
        """force to set a slaac IPv6 address to Thread interface

        Args:
            slaacAddress: a slaac IPv6 address to be set

        Returns:
            True: successful to set slaac address to Thread interface
            False: fail to set slaac address to Thread interface
        """
        print '%s call forceSetSlaac' % self.port
        print slaacAddress
        try:
            cmd = 'ipaddr add %s' % str(slaacAddress)
            print cmd
            return self.__sendCommand(cmd)[0] == 'Done'
        except Exception, e:
            ModuleHelper.WriteIntoDebugLogger("forceSetSlaac() Error: " + str(e))

    def setSleepyNodePollTime(self):
        pass

    def enableAutoDUTObjectFlag(self):
        """set AutoDUTenable flag"""
        self.AutoDUTEnable = True

    def getChildTimeoutValue(self):
        """get child timeout"""
        return self.__sendCommand('childtimeout')[0]

    def diagnosticGet(self, strDestinationAddr, listTLV_ids=[]):
        if not listTLV_ids:
            return

        if not len(listTLV_ids):
            return

        cmd = 'networkdiagnostic get %s %s' % (strDestinationAddr, ' '.join([str(tlv) for tlv in listTLV_ids]))
        print(cmd)

        return self.__sendCommand(cmd)

    def diagnosticReset(self, strDestinationAddr, listTLV_ids=[]):
        if not listTLV_ids:
            return

        if not len(listTLV_ids):
            return

        cmd = 'networkdiagnostic reset %s %s' % (strDestinationAddr, ' '.join([str(tlv) for tlv in listTLV_ids]))
        print(cmd)

        return self.__sendCommand(cmd)

    def startNativeCommissioner(self, strPSKc='GRLpassWord'):
        pass

    def startCollapsedCommissioner(self):
        """start OpenThread stack

        Returns:
            True: successful to start OpenThread stack and thread interface up
            False: fail to start OpenThread stack
        """
        print '%s call startCollapsedCommissioner' % self.port
        return self.__startOpenThread()

    def setJoinKey(self, strPSKc):
        pass

    def scanJoiner(self, xEUI='*', strPSKd='threadjpaketest'):
        """start commissioner

        Args:
            xEUI: Joiner's extended address
            strPSKd: Joiner's PSKd for commissioning

        Returns:
            True: successful to start commissioner
            False: fail to start commissioner
        """
        print '%s call scanJoiner' % self.port
        cmd = 'commissioner start %s %s' % (strPSKd, self.provisioningUrl)
        print cmd
        if self.__sendCommand(cmd)[0] == 'Done':
            if self.__logThreadRunning == False:
                self.__logThread = ThreadRunner.run(target = self.__readCommissioningLogs, args = (120,))
            return True
        else:
            return False

    def setProvisioningUrl(self, strURL='grl.com'):
        """set provisioning Url

        Args:
            strURL: Provisioning Url string

        Returns:
            True: successful to set provisioning Url
        """
        print '%s call setProvisioningUrl' % self.port
        self.provisioningUrl = strURL;
        return True

    def allowCommission(self, strPSKc="GRLPassword"):
        pass

    def joinCommissioned(self, strPSKd='threadjpaketest', waitTime=20):
        """start joiner

        Args:
            strPSKd: Joiner's PSKd

        Returns:
            True: successful to start commissioner
            False: fail to start commissioner
        """
        print '%s call joinCommissioned' % self.port
        self.__sendCommand('ifconfig up')
        cmd = 'joiner start %s %s' %(strPSKd, self.provisioningUrl)
        print cmd
        if self.__sendCommand(cmd)[0] == "Done":
            if self.__logThreadRunning == False:
                self.__logThread = ThreadRunner.run(target = self.__readCommissioningLogs, args = (90,))
            time.sleep(90)

            self.__sendCommand('thread start')
            return True
        else:
            return False

    def getCommissioningLogs(self):
        """get Commissioning logs

        Returns:
           Commissioning logs
        """
        rawLogs = self.__logThread.get()
        ProcessedLogs = []
        payload = []
        while not rawLogs.empty():
            rawLogEach = rawLogs.get()
            print rawLogEach
            if "[THCI]" not in rawLogEach:
                continue

            EncryptedPacket = PlatformDiagnosticPacket()
            infoList = rawLogEach.split('[THCI]')[1].split(']')[0].split('|')
            for eachInfo in infoList:
                print eachInfo 
                info = eachInfo.split("=")
                infoType = info[0].strip()
                infoValue = info[1].strip()
                if "direction" in infoType:
                    EncryptedPacket.Direction = PlatformDiagnosticPacket_Direction.IN if 'recv' in infoValue \
                        else PlatformDiagnosticPacket_Direction.OUT if 'send' in infoValue \
                        else PlatformDiagnosticPacket_Direction.UNKNOWN
                elif "type" in infoType:
                    EncryptedPacket.Type = PlatformDiagnosticPacket_Type.JOIN_FIN_req if 'JOIN_FIN.req' in infoValue \
                        else PlatformDiagnosticPacket_Type.JOIN_FIN_rsp if 'JOIN_FIN.rsp' in infoValue \
                        else PlatformDiagnosticPacket_Type.JOIN_ENT_rsp if 'JOIN_ENT.ntf' in infoValue \
                        else PlatformDiagnosticPacket_Type.UNKNOWN
                elif "len" in infoType:
                    EncryptedPacket.TLVsLength = int(infoValue)
                    payloadLineCount = int(infoValue)/16 + 1
                    while payloadLineCount > 0:
                        payloadLineCount = payloadLineCount - 1
                        payloadLine = rawLogs.get()
                        payloadSplit = payloadLine.split('|')
                        for block in range(1, 3):
                            payloadBlock = payloadSplit[block]
                            payloadValues = payloadBlock.split(' ')
                            for num in range(1, 9):
                                if ".." not in payloadValues[num]:
                                    payload.append(int(payloadValues[num], 16))

                    EncryptedPacket.TLVs = PlatformPackets.read(EncryptedPacket.Type,payload) if payload != [] else []

            ProcessedLogs.append(EncryptedPacket)
        return ProcessedLogs

    def MGMT_ED_SCAN(self, sAddr, xCommissionerSessionId, listChannelMask, xCount, xPeriod, xScanDuration):
        pass

    def MGMT_PANID_QUERY(self, sAddr, xCommissionerSessionId, listChannelMask, xPanId):
        pass

    def MGMT_ANNOUNCE_BEGIN(self, sAddr, xCommissionerSessionId, listChannelMask, xCount, xPeriod):
        pass

    def MGMT_ACTIVE_GET(self, Addr='', TLVs=[]):
        pass

    def MGMT_ACTIVE_SET(self, sAddr='', xCommissioningSessionId=None, listActiveTimestamp=None, listChannelMask=None, xExtendedPanId=None,
                        sNetworkName=None, sPSKc=None, listSecurityPolicy=None, xChannel=None, sMeshLocalPrefix=None, xMasterKey=None,
                        xPanId=None, xTmfPort=None, xSteeringData=None, xBorderRouterLocator=None, xDelayTimer=None):
        pass

    def MGMT_PENDING_GET(self, Addr='', TLVs=[]):
        pass

    def MGMT_PENDING_SET(self, sAddr='', xCommissionerId=None, listPendingTimestamp=None, listActiveTimestamp=None, xDelayTimer=None,
                         xChannel=None, xPanId=None, xMasterKey=None):
        pass

    def MGMT_COMM_GET(self, Addr='ff02::1', TLVs=[]):
        pass

    def MGMT_COMM_SET(self, Addr='ff02::1', xCommissionerSessionID=None, xSteeringData=None, xBorderRouterLocator=None,
                      xChannelTlv=None, ExceedMaxPayload=False):
        pass

    def setActiveDataset(self, listActiveDataset=[]):
        pass

    def setCommisionerMode(self):
        pass

    def setPSKc(self, strPSKc):
        pass

    def setActiveTimestamp(self, xActiveTimestamp):
        pass

    def setUdpJoinerPort(self, portNumber): 
        """set Joiner UDP Port

        Args:
            portNumber: Joiner UDP Port number

        Returns:
            True: successful to set Joiner UDP Port
            False: fail to set Joiner UDP Port
        """
        print '%s call setUdpJoinerPort' % self.port
        cmd = 'joinerport %d' % portNumber
        print cmd
        return self.__sendCommand(cmd)[0] == 'Done'

    def commissionerUnregister(self):
        pass

    def sendBeacons(self, sAddr, xCommissionerSessionId, listChannelMask, xPanId):
        pass
