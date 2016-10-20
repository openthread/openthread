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
""" Utility class for creating TUN network interfaces on Linux and OSX. """

import os
import sys
import fcntl
import struct
import logging
import threading
import traceback
import subprocess

from select import select

import spinel.util as util
import spinel.config as CONFIG

IFF_TUN = 0x0001
IFF_TAP = 0x0002
IFF_NO_PI = 0x1000
IFF_TUNSETIFF = 0x400454ca
IFF_TUNSETOWNER = IFF_TUNSETIFF + 2

class TunInterface(object):
    """ Utility class for creating a TUN network interface. """

    def __init__(self, identifier):
        self.identifier = identifier
        self.ifname = "tun" + str(self.identifier)
        self.tun = None
        self.fd = None

        platform = sys.platform
        if platform == "linux" or platform == "linux2":
            self.__init_linux()
        elif platform == "darwin":
            self.__init_osx()

        self.ifconfig("up")
        #self.ifconfig("inet6 add fd00::1/64")
        self.__start_tun_thread()

    def __init_osx(self):
        logging.info("TUN: Starting osx " + self.ifname)
        filename = "/dev/" + self.ifname
        self.tun = os.open(filename, os.O_RDWR)
        self.fd = self.tun
        # trick osx to auto-assign a link local address
        self.addr_add("fe80::1")
        self.addr_del("fe80::1")

    def __init_linux(self):
        logging.info("TUN: Starting linux " + self.ifname)
        self.tun = open("/dev/net/tun", "r+b")
        self.fd = self.tun.fileno()

        ifr = struct.pack("16sH", self.ifname, IFF_TUN | IFF_NO_PI)
        fcntl.ioctl(self.tun, IFF_TUNSETIFF, ifr)      # Name interface tun#
        fcntl.ioctl(self.tun, IFF_TUNSETOWNER, 1000)   # Allow non-sudo access

    def close(self):
        """ Close this tunnel interface. """
        if self.tun:
            os.close(self.fd)
            self.fd = None
            self.tun = None

    @classmethod
    def command(cls, cmd):
        """ Utility to make a system call. """
        subprocess.check_call(cmd, shell=True)

    def ifconfig(self, args):
        """ Bring interface up and/or assign addresses. """
        self.command('ifconfig ' + self.ifname + ' ' + args)

    def ping6(self, args):
        """ Ping an address. """
        cmd = 'ping6 ' + args
        print cmd
        self.command(cmd)

    def addr_add(self, addr):
        """ Add the given IPv6 address to the tunnel interface. """
        self.ifconfig('inet6 add ' + addr)

    def addr_del(self, addr):
        """ Delete the given IPv6 address from the tunnel interface. """
        platform = sys.platform
        if platform == "linux" or platform == "linux2":
            self.ifconfig('inet6 del ' + addr)
        elif platform == "darwin":
            self.ifconfig('inet6 delete ' + addr)

    def write(self, packet):
        #global gWpanApi
        #gWpanApi.ip_send(packet)
        # os.write(self.fd, packet)    # Loop back
        if CONFIG.DEBUG_TUN:
            logging.debug("\nTUN: TX (" + str(len(packet)) +
                          ") " + util.hexify_str(packet))

    def __run_tun_thread(self):
        while self.fd:
            try:
                ready_fd = select([self.fd], [], [])[0][0]
                if ready_fd == self.fd:
                    packet = os.read(self.fd, 4000)
                    if CONFIG.DEBUG_TUN:
                        logging.debug("\nTUN: RX (" + str(len(packet)) + ") " +
                                      util.hexify_str(packet))
                    self.write(packet)
            except:
                print traceback.format_exc()
                break

        logging.info("TUN: exiting")
        if self.fd:
            os.close(self.fd)
            self.fd = None

    def __start_tun_thread(self):
        """Start reader thread"""
        self._reader_alive = True
        self.receiver_thread = threading.Thread(target=self.__run_tun_thread)
        self.receiver_thread.setDaemon(True)
        self.receiver_thread.start()
