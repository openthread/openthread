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
import pexpect
import unittest

class Node():
    def __init__(self, nodeid=1):
        """ Initialize an NCP simulation node. """
        self.nodeid = nodeid
        self.verbose = int(float(os.getenv('VERBOSE', 0)))

        if "top_builddir" in os.environ.keys():
            builddir = os.environ['top_builddir']
            if "top_srcdir" in os.environ.keys():
                srcdir = os.environ['top_srcdir']
            else:
                srcdir = os.path.dirname(os.path.realpath(__file__))
                srcdir += "/../../.."
            cmd = 'python %s/tools/spinel-cli/spinel-cli.py -p %s/examples/apps/ncp/ot-ncp -n' % (srcdir, builddir)
        else:
            cmd = '../../../examples/apps/ncp/ot-ncp'
        cmd += ' %d' % nodeid
        print ("%s" % cmd)

        self.pexpect = pexpect.spawn(cmd, timeout=2)
        time.sleep(0.1)
        self.pexpect.expect('spinel-cli >')

        if self.verbose:
            self.pexpect.logfile_read = sys.stdout


    def __del__(self):
        if self.pexpect.isalive():
            self.send_command('exit')
            self.pexpect.expect(pexpect.EOF)
            self.pexpect.terminate()
            self.pexpect.close(force=True)

    def send_command(self, cmd):
        print ("%d: %s" % (self.nodeid, cmd))
        self.pexpect.sendline(cmd)
