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
#  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS 'AS IS'
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
import logging
import unittest

import thread_cert

# Test description:
#   This test verifies `otbr-agent --radio-version` works.
#
# Topology:
#    ----------------(eth)--------------------
#           |
#          BR
#

BR = 1


class SingleBorderRouter(thread_cert.TestCase):
    USE_MESSAGE_FACTORY = False

    TOPOLOGY = {
        BR: {
            'name': 'BR',
            'is_otbr': True,
            'version': '1.2',
        },
    }

    def test(self):
        br = self.nodes[BR]
        rcp_version = br.get_rcp_version()
        logging.info('RCP version: %s', rcp_version)
        br.stop_otbr_service()

        radio_version_output = br.bash("/usr/sbin/otbr-agent -d7 -v spinel+hdlc+uart:///dev/ttyUSB0 --radio-version")
        logging.info("--radio-version output: %r", radio_version_output)
        self.assertEqual(1, len(radio_version_output))
        self.assertEqual(rcp_version, radio_version_output[0].strip())


if __name__ == '__main__':
    unittest.main()
