#!/usr/bin/python -u
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
"""
   Sniffer tool that outputs raw pcap.

   Real-time stream to wireshark:
       ./sniffer.py | wireshark -k -i -

   Save stream to file or pipe:
       ./sniffer.py > trace.pcap
"""

import sys
import optparse

import spinel.util as util
import spinel.config as CONFIG
from spinel.const import SPINEL
from spinel.codec import WpanApi
from spinel.stream import StreamOpen
from spinel.pcap import PcapCodec


# Nodeid is required to execute ot-ncp for its sim radio socket port.
# This is maximum that works for MacOS.
DEFAULT_NODEID = 34    # same as WELLKNOWN_NODE_ID
DEFAULT_CHANNEL = 11

def parse_args():
    """ Parse command line arguments for this applications. """

    args = sys.argv[1:]

    opt_parser = optparse.OptionParser()
    opt_parser.add_option("-u", "--uart", action="store",
                          dest="uart", type="string")
    opt_parser.add_option("-p", "--pipe", action="store",
                          dest="pipe", type="string")
    opt_parser.add_option("-s", "--socket", action="store",
                          dest="socket", type="string")
    opt_parser.add_option("-n", "--nodeid", action="store",
                          dest="nodeid", type="string", default=str(DEFAULT_NODEID))

    opt_parser.add_option("-q", "--quiet", action="store_true", dest="quiet")
    opt_parser.add_option("-v", "--verbose", action="store_false", dest="verbose")
    opt_parser.add_option("-d", "--debug", action="store",
                          dest="debug", type="int", default=CONFIG.DEBUG_ENABLE)
    opt_parser.add_option("-x", "--hex", action="store_true", dest="hex")

    opt_parser.add_option("-c", "--channel", action="store",
                          dest="channel", type="int", default=DEFAULT_CHANNEL)

    return opt_parser.parse_args(args)

def sniffer_init(wpan_api, options):
    """" Send spinel commands to initialize sniffer node. """
    wpan_api.queue_register(SPINEL.HEADER_DEFAULT)
    wpan_api.queue_register(SPINEL.HEADER_ASYNC)

    wpan_api.cmd_send(SPINEL.CMD_RESET)
    wpan_api.prop_set_value(SPINEL.PROP_MAC_FILTER_MODE, SPINEL.MAC_FILTER_MODE_MONITOR)
    wpan_api.prop_set_value(SPINEL.PROP_PHY_CHAN, options.channel)
    wpan_api.prop_set_value(SPINEL.PROP_MAC_15_4_PANID, 0xFFFF, 'H')
    wpan_api.prop_set_value(SPINEL.PROP_MAC_RAW_STREAM_ENABLED, 1)
    wpan_api.prop_set_value(SPINEL.PROP_NET_IF_UP, 1)

def main():
    """ Top-level main for sniffer host-side tool. """
    (options, remaining_args) = parse_args()

    if options.debug:
        CONFIG.debug_set_level(options.debug)

    # Set default stream to pipe
    stream_type = 'p'
    stream_descriptor = "../../examples/apps/ncp/ot-ncp "+options.nodeid

    if options.uart:
        stream_type = 'u'
        stream_descriptor = options.uart
    elif options.socket:
        stream_type = 's'
        stream_descriptor = options.socket
    elif options.pipe:
        stream_type = 'p'
        stream_descriptor = options.pipe
        if options.nodeid:
            stream_descriptor += " "+str(options.nodeid)
    else:
        if len(remaining_args) > 0:
            stream_descriptor = " ".join(remaining_args)

    stream = StreamOpen(stream_type, stream_descriptor, False)
    if stream is None: exit()
    wpan_api = WpanApi(stream, options.nodeid)
    sniffer_init(wpan_api, options)

    pcap = PcapCodec()
    hdr = pcap.encode_header()
    if options.hex:
        hdr = util.hexify_str(hdr)+"\n"
    sys.stdout.write(hdr)
    sys.stdout.flush()

    try:
        tid = SPINEL.HEADER_ASYNC
        prop_id = SPINEL.PROP_STREAM_RAW
        while True:
            result = wpan_api.queue_wait_for_prop(prop_id, tid)
            if result and result.prop == prop_id:
                length = wpan_api.parse_S(result.value)
                pkt = result.value[2:2+length]
                pkt = pcap.encode_frame(pkt)
                if options.hex:
                    pkt = util.hexify_str(pkt)+"\n"
                sys.stdout.write(pkt)
                sys.stdout.flush()

    except KeyboardInterrupt:
        pass

    if wpan_api:
        wpan_api.stream.close()


if __name__ == "__main__":
    main()
