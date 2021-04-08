#!/usr/bin/env python3
#
#  Copyright (c) 2021, The OpenThread Authors.
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
# This script is used to test basic TCP features.
#
import socket
import sys

SERVER_PORT = 12345


def server():
    sock = socket.socket(socket.AF_INET6, socket.SOCK_STREAM, 0)
    sock.bind(('', SERVER_PORT))
    sock.listen(5)

    print('echo server listening ...')
    while True:
        client, _ = sock.accept()
        print('client connected')

        while True:
            data = client.recv(1024)
            if data == b'':
                break

            print('echo:', repr(data))
            client.sendall(data)

        print('client disconnected')
        client.close()


def client(server_addr, num_bytes):
    sock = socket.socket(socket.AF_INET6, socket.SOCK_STREAM, 0)
    sock.connect((server_addr, SERVER_PORT))
    print('echo client connected')

    senddata = data = ''.join(chr(ord('0') + i % 10) for i in range(num_bytes)).encode('utf8')
    recvdata = b''

    while recvdata != senddata:
        n = sock.send(data)
        data = data[n:]

        recvdata += sock.recv(1024)

    print('echo client done')
    sock.close()


def main():
    run_type = sys.argv[1]
    assert run_type in ('server', 'client')

    if run_type == 'server':
        server()
    elif run_type == 'client':
        server_addr = sys.argv[2]
        try:
            num_bytes = int(sys.argv[3])
        except IndexError:
            num_bytes = 1024

        client(server_addr, num_bytes)


if __name__ == '__main__':
    main()
