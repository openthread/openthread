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

import collections
import io
import logging
import os
import Queue
import select
import socket
import threading

import message


class Sniffer:

    """ Class representing the Sniffing node, whose main task is listening and logging message exchange performed by other nodes. """

    logger = logging.getLogger("sniffer.Sniffer")

    POLL_TIMEOUT = 0.11

    RECV_BUFFER_SIZE = 4096

    BASE_PORT = 9000

    WELLKNOWN_NODE_ID = 34

    PORT_OFFSET = int(os.getenv('PORT_OFFSET', "0"))

    def __init__(self, nodeid, message_factory):
        """
        Args:
            nodeid (int): Node identifier
            message_factory (MessageFactory): Class producing messages from data bytes.
        """

        self.nodeid = nodeid
        self._message_factory = message_factory

        self._socket = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        if self._socket is None:
            raise RuntimeError("Could not create socket.")

        self._socket.bind(self.address)

        self._thread = None
        self._thread_alive_event = threading.Event()

        self._poll = select.poll()

        self._buckets = collections.defaultdict(Queue.Queue)

    def _nodeid_to_address(self, nodeid, ip_address="localhost"):
        return "", self.BASE_PORT + (self.PORT_OFFSET * self.WELLKNOWN_NODE_ID) + nodeid

    def _address_to_nodeid(self, address):
        ip_address, port = address
        return (port - self.BASE_PORT - (self.PORT_OFFSET * self.WELLKNOWN_NODE_ID))

    def _recv(self, fd):
        """ Receive data from socket with passed file descriptor. """

        data, address = socket.fromfd(fd, socket.AF_INET, socket.SOCK_DGRAM).recvfrom(self.RECV_BUFFER_SIZE)

        msg = self._message_factory.create(io.BytesIO(data))

        if msg is None:
            self.logger.debug("Received 6LowPAN fragment.")
            return

        nodeid = self._address_to_nodeid(address)

        self._buckets[nodeid].put(msg)

    @property
    def address(self):
        """ Sniffer address. """

        return self._nodeid_to_address(self.nodeid)

    def _run(self):
        """ Receive thread main loop. """

        while self._thread_alive_event.is_set():
            reported_events = self._poll.poll(self.POLL_TIMEOUT)

            for fd_event_pair in reported_events:
                fd, event = fd_event_pair

                if event & select.POLLIN or event & select.POLLPRI:
                    self._recv(fd)

                elif event & select.POLLERR:
                    self.logger.error("Error condition of some sort")
                    self._thread_alive_event.clear()
                    break

                elif event & select.POLLNVAL:
                    self.logger.error("Invalid request: descriptor not open")
                    self._thread_alive_event.clear()
                    break

    def start(self):
        """ Start sniffing. """

        self._poll.register(self._socket, select.POLLIN | select.POLLPRI | select.POLLERR | select.POLLNVAL)

        self._thread = threading.Thread(target=self._run)
        self._thread.daemon = True

        self._thread_alive_event.set()
        self._thread.start()

    def stop(self):
        """ Stop sniffing. """

        self._poll.unregister(self._socket)

        self._thread_alive_event.clear()
        self._thread.join()
        self._thread = None

    def get_messages_sent_by(self, nodeid):
        """ Get sniffed messages.

        Note! This method flushes the message queue so calling this method again will return only the newly logged messages.

        """
        bucket = self._buckets[nodeid]
        messages = []

        while not bucket.empty():
            messages.append(bucket.get_nowait())

        return message.MessagesSet(messages)
