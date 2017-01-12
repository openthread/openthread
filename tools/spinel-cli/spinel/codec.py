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
Module providing a Spienl coder / decoder class.
"""

from __future__ import print_function

import os
import sys
import time
import logging
import threading
import traceback

is_py2 = sys.version[0] == '2'
if is_py2:
    import Queue as Queue
else:
    import queue as Queue

from struct import pack
from struct import unpack
from collections import namedtuple
from collections import defaultdict

import ipaddress

import spinel.util as util
import spinel.config as CONFIG
from spinel.const import kThread
from spinel.const import SPINEL
from spinel.const import SPINEL_LAST_STATUS_MAP
from spinel.hdlc import Hdlc



FEATURE_USE_HDLC = 1
FEATURE_USE_SLACC = 1

TIMEOUT_PROP = 2

#=========================================
#   SpinelCodec
#=========================================

#  0: DATATYPE_NULL
#'.': DATATYPE_VOID: Empty data type. Used internally.
#'b': DATATYPE_BOOL: Boolean value. Encoded in 8-bits as either 0x00 or 0x01.
#                    All other values are illegal.
#'C': DATATYPE_UINT8: Unsigned 8-bit integer.
#'c': DATATYPE_INT8: Signed 8-bit integer.
#'S': DATATYPE_UINT16: Unsigned 16-bit integer. (Little-endian)
#'s': DATATYPE_INT16: Signed 16-bit integer. (Little-endian)
#'L': DATATYPE_UINT32: Unsigned 32-bit integer. (Little-endian)
#'l': DATATYPE_INT32: Signed 32-bit integer. (Little-endian)
#'i': DATATYPE_UINT_PACKED: Packed Unsigned Integer. (See section 7.2)
#'6': DATATYPE_IPv6ADDR: IPv6 Address. (Big-endian)
#'E': DATATYPE_EUI64: EUI-64 Address. (Big-endian)
#'e': DATATYPE_EUI48: EUI-48 Address. (Big-endian)
#'D': DATATYPE_DATA: Arbitrary Data. (See section 7.3)
#'U': DATATYPE_UTF8: Zero-terminated UTF8-encoded string.
#'T': DATATYPE_STRUCT: Structured datatype. Compound type. (See section 7.4)
#'A': DATATYPE_ARRAY: Array of datatypes. Compound type. (See section 7.5)


class SpinelCodec(object):
    """ A general coder / decoder class for Spinel protocol. """

    @classmethod
    def parse_b(cls, payload): return unpack("<B", payload[:1])[0]

    @classmethod
    def parse_c(cls, payload): return unpack("<b", payload[:1])[0]

    @classmethod
    def parse_C(cls, payload): return unpack("<B", payload[:1])[0]

    @classmethod
    def parse_s(cls, payload): return unpack("<h", payload[:2])[0]

    @classmethod
    def parse_S(cls, payload): return unpack("<H", payload[:2])[0]

    @classmethod
    def parse_l(cls, payload): return unpack("<l", payload[:4])[0]

    @classmethod
    def parse_L(cls, payload): return unpack("<L", payload[:4])[0]

    @classmethod
    def parse_6(cls, payload): return payload[:16]

    @classmethod
    def parse_E(cls, payload): return payload[:8]

    @classmethod
    def parse_e(cls, payload): return payload[:6]

    @classmethod
    def parse_U(cls, payload): return payload[:-1]  # strip null

    @classmethod
    def parse_D(cls, payload): return payload

    @classmethod
    def parse_i(cls, payload):
        """ Decode EXI integer format. """
        value = 0
        value_len = 0
        value_mul = 1

        while value_len < 4:
            byte = ord(payload[value_len])
            value += (byte & 0x7F) * value_mul
            if byte < 0x80:
                break
            value_mul *= 0x80
            value_len += 1

        return (value, value_len + 1)

    @classmethod
    def parse_field(cls, payload, spinel_format):
        map_decode = {
            'b': cls.parse_b,
            'c': cls.parse_c,
            'C': cls.parse_C,
            's': cls.parse_s,
            'S': cls.parse_S,
            'L': cls.parse_L,
            'l': cls.parse_l,
            '6': cls.parse_6,
            'E': cls.parse_E,
            'e': cls.parse_e,
            'U': cls.parse_U,
            'D': cls.parse_D,
            'i': cls.parse_i,
        }
        try:
            return map_decode[spinel_format[0]](payload)
        except KeyError:
            print(traceback.format_exc())
            return None

    @classmethod
    def parse_fields(cls, payload, spinel_format):
        map_lengths = {
            'b': 1,
            'c': 1,
            'C': 1,
            's': 2,
            'S': 2,
            'l': 4,
            'L': 4,
            '6': 16,
            'E': 8,
            'e': 6,
        }
        result = []

        idx = 0
        while idx < len(spinel_format):
            format = spinel_format[idx]

            if format == 'T':
                if spinel_format[idx+1] != '(':
                    raise ValueError('Invalid structure format')
                # TODO: count parentheses. There is a problem with nested structs.
                struct_end = idx + spinel_format[idx:].index(')')

                struct_format = spinel_format[idx+2:struct_end]
                struct_len = sum([map_lengths[c] for c in struct_format])

                result.append(cls.parse_fields(payload, struct_format))
                payload = payload[struct_len:]

                idx = struct_end + 1
            else:
                result.append(cls.parse_field(payload, format))
                payload = payload[map_lengths[format]:]

                idx += 1

        return tuple(result)

    @classmethod
    def encode_i(cls, data):
        """ Encode EXI integer format. """
        result = ""
        while data:
            value = data & 0x7F
            data >>= 7
            if data:
                value |= 0x80
            result = result + pack("<B", value)
        return result

    @classmethod
    def encode_b(cls, value): return pack('B', value)

    @classmethod
    def encode_c(cls, value): return pack('B', value)

    @classmethod
    def encode_C(cls, value): return pack('B', value)

    @classmethod
    def encode_s(cls, value): return pack('<h', value)

    @classmethod
    def encode_S(cls, value): return pack('<H', value)

    @classmethod
    def encode_l(cls, value): return pack('<l', value)

    @classmethod
    def encode_L(cls, value): return pack('<L', value)

    @classmethod
    def encode_6(cls, value): return value[:16]

    @classmethod
    def encode_E(cls, value): return value[:8]

    @classmethod
    def encode_e(cls, value): return value[:6]

    @classmethod
    def encode_U(cls, value): return value + '\0'

    @classmethod
    def encode_D(cls, value): return value

    @classmethod
    def encode_field(cls, code, value):
        map_encode = {
            'b': cls.encode_b,
            'c': cls.encode_c,
            'C': cls.encode_C,
            's': cls.encode_s,
            'S': cls.encode_S,
            'L': cls.encode_L,
            'l': cls.encode_l,
            '6': cls.encode_6,
            'E': cls.encode_E,
            'e': cls.encode_e,
            'U': cls.encode_U,
            'D': cls.encode_D,
            'i': cls.encode_i,
        }
        try:
            return map_encode[code](value)
        except KeyError:
            print(traceback.format_exc())
            return None

    def next_code(self, spinel_format):
        code = spinel_format[0]
        spinel_format = spinel_format[1:]
        # TODO: Handle T() and A()
        return code, spinel_format

    def encode_fields(self, spinel_format, *fields):
        packed = ""
        for field in fields:
            code, spinel_format = self.next_code(spinel_format)
            if not code:
                break
            packed += self.encode_field(code, field)
        return packed

    def encode_packet(self, command_id, payload=None, tid=SPINEL.HEADER_DEFAULT):
        """ Encode the given payload as a Spinel frame. """
        header = pack(">B", tid)
        cmd = self.encode_i(command_id)
        pkt = header + cmd + payload
        return pkt


#=========================================

class SpinelPropertyHandler(SpinelCodec):

    def LAST_STATUS(self, _, payload): return self.parse_i(payload)[0]

    def PROTOCOL_VERSION(self, _wpan_api, payload): pass

    def NCP_VERSION(self, _, payload): return self.parse_U(payload)

    def INTERFACE_TYPE(self, _, payload): return self.parse_i(payload)[0]

    def VENDOR_ID(self, _, payload): return self.parse_i(payload)[0]

    def CAPS(self, _wpan_api, payload): pass

    def INTERFACE_COUNT(self, _, payload): return self.parse_C(payload)

    def POWER_STATE(self, _, payload): return self.parse_C(payload)

    def HWADDR(self, _, payload): return self.parse_E(payload)

    def LOCK(self, _, payload): return self.parse_b(payload)

    def HBO_MEM_MAX(self, _, payload): return self.parse_L(payload)

    def HBO_BLOCK_MAX(self, _, payload): return self.parse_S(payload)

    def PHY_ENABLED(self, _, payload): return self.parse_b(payload)

    def PHY_CHAN(self, _, payload): return self.parse_C(payload)

    def PHY_CHAN_SUPPORTED(self, _wpan_api, payload): pass

    def PHY_FREQ(self, _, payload): return self.parse_L(payload)

    def PHY_CCA_THRESHOLD(self, _, payload): return self.parse_c(payload)

    def PHY_TX_POWER(self, _, payload): return self.parse_c(payload)

    def PHY_RSSI(self, _, payload): return self.parse_c(payload)

    def MAC_SCAN_STATE(self, _, payload): return self.parse_C(payload)

    def MAC_SCAN_MASK(self, _, payload): return self.parse_U(payload)

    def MAC_SCAN_PERIOD(self, _, payload): return self.parse_S(payload)

    def MAC_SCAN_BEACON(self, _, payload): return self.parse_U(payload)

    def MAC_15_4_LADDR(self, _, payload): return self.parse_E(payload)

    def MAC_15_4_SADDR(self, _, payload): return self.parse_S(payload)

    def MAC_15_4_PANID(self, _, payload): return self.parse_S(payload)

    def MAC_FILTER_MODE(self, _, payload): return self.parse_C(payload)

    def MAC_RAW_STREAM_ENABLED(self, _, payload):
        return self.parse_b(payload)

    def MAC_WHITELIST(self, _, payload): pass

    def MAC_WHITELIST_ENABLED(self, _, payload):
        return self.parse_b(payload)

    def NET_SAVED(self, _, payload): return self.parse_b(payload)

    def NET_IF_UP(self, _, payload): return self.parse_b(payload)

    def NET_STACK_UP(self, _, payload): return self.parse_C(payload)

    def NET_ROLE(self, _, payload): return self.parse_C(payload)

    def NET_NETWORK_NAME(self, _, payload): return self.parse_U(payload)

    def NET_XPANID(self, _, payload): return self.parse_D(payload)

    def NET_MASTER_KEY(self, _, payload): return self.parse_D(payload)

    def NET_KEY_SEQUENCE_COUNTER(self, _, payload): return self.parse_L(payload)

    def NET_PARTITION_ID(self, _, payload): return self.parse_L(payload)

    def NET_KEY_SWITCH_GUARDTIME(self, _, payload): return self.parse_L(payload)

    def THREAD_LEADER_ADDR(self, _, payload): return self.parse_6(payload)

    def THREAD_PARENT(self, _wpan_api, payload): return self.parse_fields(payload, "ES")

    def THREAD_CHILD_TABLE(self, _, payload): return self.parse_D(payload)

    def THREAD_LEADER_RID(self, _, payload): return self.parse_C(payload)

    def THREAD_LEADER_WEIGHT(self, _, payload):
        return self.parse_C(payload)

    def THREAD_LOCAL_LEADER_WEIGHT(self, _, payload):
        return self.parse_C(payload)

    def THREAD_NETWORK_DATA(self, _, payload):
        return self.parse_D(payload)

    def THREAD_NETWORK_DATA_VERSION(self, _wpan_api, payload):
        return self.parse_C(payload)

    def THREAD_STABLE_NETWORK_DATA(self, _wpan_api, payload): pass

    def THREAD_STABLE_NETWORK_DATA_VERSION(self, _wpan_api, payload):
        return self.parse_C(payload)

    def __init__(self):
        self.autoAddresses = set()

        self.wpan_api = None
        self.__queue_prefix = Queue.Queue()
        self.prefix_thread = threading.Thread(target=self.__run_prefix_handler)
        self.prefix_thread.setDaemon(True)
        self.prefix_thread.start()

    def handle_ipaddr_remove(self, ipaddr):
        valid = 1
        preferred = 1
        flags = 0
        prefix_len = 64  # always use /64

        arr = self.encode_fields('6CLLC',
                                 ipaddr.ip.packed,
                                 prefix_len,
                                 valid,
                                 preferred,
                                 flags)

        self.wpan_api.prop_remove_async(SPINEL.PROP_IPV6_ADDRESS_TABLE,
                                        arr, str(len(arr)) + 's',
                                        SPINEL.HEADER_EVENT_HANDLER)

    def handle_ipaddr_insert(self, prefix, prefix_len, _stable, flags, _is_local):
        """ Add an ip address for each prefix on prefix change. """

        ipaddr_str = str(ipaddress.IPv6Address(prefix)) + \
            str(self.wpan_api.nodeid)
        if CONFIG.DEBUG_LOG_PROP:
            print("\n>>>> new PREFIX add ipaddr: " + ipaddr_str)

        valid = 1
        preferred = 1
        flags = 0
        ipaddr = ipaddress.IPv6Interface(unicode(ipaddr_str))
        self.autoAddresses.add(ipaddr)

        arr = self.encode_fields('6CLLC',
                                 ipaddr.ip.packed,
                                 prefix_len,
                                 valid,
                                 preferred,
                                 flags)

        self.wpan_api.prop_insert_async(SPINEL.PROP_IPV6_ADDRESS_TABLE,
                                        arr, str(len(arr)) + 's',
                                        SPINEL.HEADER_EVENT_HANDLER)

    def handle_prefix_change(self, payload):
        """ Automatically ipaddr add / remove addresses for each new prefix. """
        # As done by cli.cpp Interpreter::HandleNetifStateChanged

        # First parse payload and extract slaac prefix information.
        pay = payload
        Prefix = namedtuple("Prefix", "prefix prefixlen stable flags is_local")
        prefixes = []
        slaacPrefixSet = set()
        while len(pay) >= 22:
            (_structlen) = unpack('<H', pay[:2])
            pay = pay[2:]
            prefix = Prefix(*unpack('16sBBBB', pay[:20]))
            if prefix.flags & kThread.PrefixSlaacFlag:
                net6 = ipaddress.IPv6Network(prefix.prefix)
                net6 = net6.supernet(new_prefix=prefix.prefixlen)
                slaacPrefixSet.add(net6)
                prefixes.append(prefix)
            pay = pay[20:]

        for prefix in prefixes:
            self.handle_ipaddr_insert(*prefix)

        if CONFIG.DEBUG_LOG_PROP:
            print("\n========= PREFIX ============")
            print("ipaddrs: " + str(self.autoAddresses))
            print("slaac prefix set: " + str(slaacPrefixSet))
            print("==============================\n")

        # ==> ipaddrs - query current addresses
        #
        # for ipaddr in ipaddrs:
        #     if lifetime > 0 and not in slaac prefixes
        #             ==> remove
        for ipaddr in self.autoAddresses:
            if not any(ipaddr in prefix for prefix in slaacPrefixSet):
                self.handle_ipaddr_remove(ipaddr)

        # for slaac prefix in prefixes:
        #     if no ipaddr with lifetime > 0 in prefix:
        #          ==> add

    def __run_prefix_handler(self):
        while 1:
            (wpan_api, payload) = self.__queue_prefix.get(True)
            self.wpan_api = wpan_api
            self.handle_prefix_change(payload)
            self.__queue_prefix.task_done()

    def THREAD_ON_MESH_NETS(self, wpan_api, payload):
        if FEATURE_USE_SLACC:
            # Kick prefix handler thread to allow serial rx thread to work.
            self.__queue_prefix.put_nowait((wpan_api, payload))

        return self.parse_D(payload)

    def THREAD_LOCAL_ROUTES(self, _wpan_api, payload): pass

    def THREAD_ASSISTING_PORTS(self, _wpan_api, payload): pass

    def THREAD_ALLOW_LOCAL_NET_DATA_CHANGE(self, _, payload):
        return self.parse_b(payload)

    def THREAD_MODE(self, _, payload): return self.parse_C(payload)

    def THREAD_CHILD_COUNT_MAX(self, _, payload): return self.parse_C(payload)

    def THREAD_CHILD_TIMEOUT(self, _, payload): return self.parse_L(payload)

    def THREAD_RLOC16(self, _, payload): return self.parse_S(payload)

    def THREAD_ROUTER_UPGRADE_THRESHOLD(self, _, payload):
        return self.parse_C(payload)

    def THREAD_ROUTER_DOWNGRADE_THRESHOLD(self, _, payload):
        return self.parse_C(payload)

    def THREAD_ROUTER_SELECTION_JITTER(self, _, payload):
        return self.parse_C(payload)

    def THREAD_CONTEXT_REUSE_DELAY(self, _, payload):
        return self.parse_L(payload)

    def THREAD_NETWORK_ID_TIMEOUT(self, _, payload):
        return self.parse_C(payload)

    def THREAD_ACTIVE_ROUTER_IDS(self, _, payload):
        return self.parse_D(payload)

    def THREAD_RLOC16_DEBUG_PASSTHRU(self, _, payload):
        return self.parse_b(payload)

    def MESHCOP_JOINER_ENABLE(self, _, payload):
        return self.parse_b(payload)

    def MESHCOP_JOINER_CREDENTIAL(self, _, payload):
        return self.parse_D(payload)

    def MESHCOP_JOINER_URL(self, _, payload):
        return self.parse_U(payload)

    def MESHCOP_BORDER_AGENT_ENABLE(self, _, payload):
        return self.parse_b(payload)

    def IPV6_LL_ADDR(self, _, payload): return self.parse_6(payload)

    def IPV6_ML_ADDR(self, _, payload): return self.parse_6(payload)

    def IPV6_ML_PREFIX(self, _, payload): return self.parse_E(payload)

    def IPV6_ADDRESS_TABLE(self, _, payload): return self.parse_D(payload)

    def IPV6_ROUTE_TABLE(self, _, payload): return self.parse_D(payload)

    def IPv6_ICMP_PING_OFFLOAD(self, _, payload):
        return self.parse_b(payload)

    def STREAM_DEBUG(self, _, payload): return self.parse_U(payload)

    def STREAM_RAW(self, _, payload): return self.parse_D(payload)

    def STREAM_NET(self, _, payload): return self.parse_D(payload)

    def STREAM_NET_INSECURE(self, _, payload): return self.parse_D(payload)

    def PIB_PHY_CHANNELS_SUPPORTED(self, _wpan_api, payload): pass

    def PIB_MAC_PROMISCUOUS_MODE(self, _wpan_api, payload): pass

    def PIB_MAC_SECURITY_ENABLED(self, _wpan_api, payload): pass

    def MSG_BUFFER_COUNTERS(self, _wpan_api, payload): return self.parse_fields(payload, "T(SSSSSSSSSSSSSSSS)")

#=========================================


class SpinelCommandHandler(SpinelCodec):

    def handle_prop(self, wpan_api, name, payload, tid):
        (prop_id, prop_len) = self.parse_i(payload)

        try:
            handler = SPINEL_PROP_DISPATCH[prop_id]
            prop_name = handler.__name__
            prop_value = handler(wpan_api, payload[prop_len:])

            if CONFIG.DEBUG_LOG_PROP:

                # Generic output
                if isinstance(prop_value, basestring):
                    prop_value_str = util.hexify_str(prop_value)
                    logging.debug("PROP_VALUE_%s [tid=%d]: %s = %s",
                                  name, (tid & 0xF), prop_name, prop_value_str)
                else:
                    prop_value_str = str(prop_value)

                    logging.debug("PROP_VALUE_%s [tid=%d]: %s = %s",
                                  name, (tid & 0xF), prop_name, prop_value_str)

                # Extend output for certain properties.
                if prop_id == SPINEL.PROP_LAST_STATUS:
                    logging.debug(SPINEL_LAST_STATUS_MAP[prop_value])

            if CONFIG.DEBUG_LOG_PKT:
                if ((prop_id == SPINEL.PROP_STREAM_NET) or
                        (prop_id == SPINEL.PROP_STREAM_NET_INSECURE)):
                    logging.debug("PROP_VALUE_" + name + ": " + prop_name)

                elif prop_id == SPINEL.PROP_STREAM_DEBUG:
                    logging.debug("DEBUG: " + prop_value)

            if wpan_api:
                wpan_api.queue_add(prop_id, prop_value, tid)
            else:
                print("no wpan_api")

        except Exception as _ex:
            prop_name = "Property Unknown"
            logging.info("\n%s (%i): ", prop_name, prop_id)
            print(traceback.format_exc())

    def PROP_VALUE_IS(self, wpan_api, payload, tid):
        self.handle_prop(wpan_api, "IS", payload, tid)

    def PROP_VALUE_INSERTED(self, wpan_api, payload, tid):
        self.handle_prop(wpan_api, "INSERTED", payload, tid)

    def PROP_VALUE_REMOVED(self, wpan_api, payload, tid):
        self.handle_prop(wpan_api, "REMOVED", payload, tid)


WPAN_CMD_HANDLER = SpinelCommandHandler()

SPINEL_COMMAND_DISPATCH = {
    SPINEL.RSP_PROP_VALUE_IS: WPAN_CMD_HANDLER.PROP_VALUE_IS,
    SPINEL.RSP_PROP_VALUE_INSERTED: WPAN_CMD_HANDLER.PROP_VALUE_INSERTED,
    SPINEL.RSP_PROP_VALUE_REMOVED: WPAN_CMD_HANDLER.PROP_VALUE_REMOVED,
}

WPAN_PROP_HANDLER = SpinelPropertyHandler()

SPINEL_PROP_DISPATCH = {
    SPINEL.PROP_LAST_STATUS:           WPAN_PROP_HANDLER.LAST_STATUS,
    SPINEL.PROP_PROTOCOL_VERSION:      WPAN_PROP_HANDLER.PROTOCOL_VERSION,
    SPINEL.PROP_NCP_VERSION:           WPAN_PROP_HANDLER.NCP_VERSION,
    SPINEL.PROP_INTERFACE_TYPE:        WPAN_PROP_HANDLER.INTERFACE_TYPE,
    SPINEL.PROP_VENDOR_ID:             WPAN_PROP_HANDLER.VENDOR_ID,
    SPINEL.PROP_CAPS:                  WPAN_PROP_HANDLER.CAPS,
    SPINEL.PROP_INTERFACE_COUNT:       WPAN_PROP_HANDLER.INTERFACE_COUNT,
    SPINEL.PROP_POWER_STATE:           WPAN_PROP_HANDLER.POWER_STATE,
    SPINEL.PROP_HWADDR:                WPAN_PROP_HANDLER.HWADDR,
    SPINEL.PROP_LOCK:                  WPAN_PROP_HANDLER.LOCK,
    SPINEL.PROP_HBO_MEM_MAX:           WPAN_PROP_HANDLER.HBO_MEM_MAX,
    SPINEL.PROP_HBO_BLOCK_MAX:         WPAN_PROP_HANDLER.HBO_BLOCK_MAX,

    SPINEL.PROP_PHY_ENABLED:           WPAN_PROP_HANDLER.PHY_ENABLED,
    SPINEL.PROP_PHY_CHAN:              WPAN_PROP_HANDLER.PHY_CHAN,
    SPINEL.PROP_PHY_CHAN_SUPPORTED:    WPAN_PROP_HANDLER.PHY_CHAN_SUPPORTED,
    SPINEL.PROP_PHY_FREQ:              WPAN_PROP_HANDLER.PHY_FREQ,
    SPINEL.PROP_PHY_CCA_THRESHOLD:     WPAN_PROP_HANDLER.PHY_CCA_THRESHOLD,
    SPINEL.PROP_PHY_TX_POWER:          WPAN_PROP_HANDLER.PHY_TX_POWER,
    SPINEL.PROP_PHY_RSSI:              WPAN_PROP_HANDLER.PHY_RSSI,

    SPINEL.PROP_MAC_SCAN_STATE:        WPAN_PROP_HANDLER.MAC_SCAN_STATE,
    SPINEL.PROP_MAC_SCAN_MASK:         WPAN_PROP_HANDLER.MAC_SCAN_MASK,
    SPINEL.PROP_MAC_SCAN_PERIOD:       WPAN_PROP_HANDLER.MAC_SCAN_PERIOD,
    SPINEL.PROP_MAC_SCAN_BEACON:       WPAN_PROP_HANDLER.MAC_SCAN_BEACON,
    SPINEL.PROP_MAC_15_4_LADDR:        WPAN_PROP_HANDLER.MAC_15_4_LADDR,
    SPINEL.PROP_MAC_15_4_SADDR:        WPAN_PROP_HANDLER.MAC_15_4_SADDR,
    SPINEL.PROP_MAC_15_4_PANID:        WPAN_PROP_HANDLER.MAC_15_4_PANID,
    SPINEL.PROP_MAC_RAW_STREAM_ENABLED: WPAN_PROP_HANDLER.MAC_RAW_STREAM_ENABLED,
    SPINEL.PROP_MAC_FILTER_MODE:       WPAN_PROP_HANDLER.MAC_FILTER_MODE,

    SPINEL.PROP_MAC_WHITELIST:         WPAN_PROP_HANDLER.MAC_WHITELIST,
    SPINEL.PROP_MAC_WHITELIST_ENABLED: WPAN_PROP_HANDLER.MAC_WHITELIST_ENABLED,

    SPINEL.PROP_NET_SAVED:             WPAN_PROP_HANDLER.NET_SAVED,
    SPINEL.PROP_NET_IF_UP:             WPAN_PROP_HANDLER.NET_IF_UP,
    SPINEL.PROP_NET_STACK_UP:          WPAN_PROP_HANDLER.NET_STACK_UP,
    SPINEL.PROP_NET_ROLE:              WPAN_PROP_HANDLER.NET_ROLE,
    SPINEL.PROP_NET_NETWORK_NAME:      WPAN_PROP_HANDLER.NET_NETWORK_NAME,
    SPINEL.PROP_NET_XPANID:            WPAN_PROP_HANDLER.NET_XPANID,
    SPINEL.PROP_NET_MASTER_KEY:        WPAN_PROP_HANDLER.NET_MASTER_KEY,
    SPINEL.PROP_NET_KEY_SEQUENCE_COUNTER: WPAN_PROP_HANDLER.NET_KEY_SEQUENCE_COUNTER,
    SPINEL.PROP_NET_PARTITION_ID:      WPAN_PROP_HANDLER.NET_PARTITION_ID,
    SPINEL.PROP_NET_KEY_SWITCH_GUARDTIME: WPAN_PROP_HANDLER.NET_KEY_SWITCH_GUARDTIME,

    SPINEL.PROP_THREAD_LEADER_ADDR: WPAN_PROP_HANDLER.THREAD_LEADER_ADDR,
    SPINEL.PROP_THREAD_PARENT: WPAN_PROP_HANDLER.THREAD_PARENT,
    SPINEL.PROP_THREAD_CHILD_TABLE:  WPAN_PROP_HANDLER.THREAD_CHILD_TABLE,
    SPINEL.PROP_THREAD_LEADER_RID: WPAN_PROP_HANDLER.THREAD_LEADER_RID,
    SPINEL.PROP_THREAD_LEADER_WEIGHT: WPAN_PROP_HANDLER.THREAD_LEADER_WEIGHT,
    SPINEL.PROP_THREAD_LOCAL_LEADER_WEIGHT: WPAN_PROP_HANDLER.THREAD_LOCAL_LEADER_WEIGHT,
    SPINEL.PROP_THREAD_NETWORK_DATA: WPAN_PROP_HANDLER.THREAD_NETWORK_DATA,
    SPINEL.PROP_THREAD_NETWORK_DATA_VERSION: WPAN_PROP_HANDLER.THREAD_NETWORK_DATA_VERSION,
    SPINEL.PROP_THREAD_STABLE_NETWORK_DATA: WPAN_PROP_HANDLER.THREAD_STABLE_NETWORK_DATA,
    SPINEL.PROP_THREAD_STABLE_NETWORK_DATA_VERSION:
        WPAN_PROP_HANDLER.THREAD_STABLE_NETWORK_DATA_VERSION,
    SPINEL.PROP_THREAD_ON_MESH_NETS: WPAN_PROP_HANDLER.THREAD_ON_MESH_NETS,
    SPINEL.PROP_THREAD_LOCAL_ROUTES: WPAN_PROP_HANDLER.THREAD_LOCAL_ROUTES,
    SPINEL.PROP_THREAD_ASSISTING_PORTS: WPAN_PROP_HANDLER.THREAD_ASSISTING_PORTS,
    SPINEL.PROP_THREAD_ALLOW_LOCAL_NET_DATA_CHANGE:
        WPAN_PROP_HANDLER.THREAD_ALLOW_LOCAL_NET_DATA_CHANGE,
    SPINEL.PROP_THREAD_MODE: WPAN_PROP_HANDLER.THREAD_MODE,
    SPINEL.PROP_THREAD_CHILD_COUNT_MAX: WPAN_PROP_HANDLER.THREAD_CHILD_COUNT_MAX,
    SPINEL.PROP_THREAD_CHILD_TIMEOUT: WPAN_PROP_HANDLER.THREAD_CHILD_TIMEOUT,
    SPINEL.PROP_THREAD_RLOC16: WPAN_PROP_HANDLER.THREAD_RLOC16,
    SPINEL.PROP_THREAD_ROUTER_UPGRADE_THRESHOLD: WPAN_PROP_HANDLER.THREAD_ROUTER_UPGRADE_THRESHOLD,
    SPINEL.PROP_THREAD_ROUTER_DOWNGRADE_THRESHOLD:
        WPAN_PROP_HANDLER.THREAD_ROUTER_DOWNGRADE_THRESHOLD,
    SPINEL.PROP_THREAD_ROUTER_SELECTION_JITTER: WPAN_PROP_HANDLER.THREAD_ROUTER_SELECTION_JITTER,
    SPINEL.PROP_THREAD_CONTEXT_REUSE_DELAY: WPAN_PROP_HANDLER.THREAD_CONTEXT_REUSE_DELAY,
    SPINEL.PROP_THREAD_NETWORK_ID_TIMEOUT: WPAN_PROP_HANDLER.THREAD_NETWORK_ID_TIMEOUT,
    SPINEL.PROP_THREAD_ACTIVE_ROUTER_IDS: WPAN_PROP_HANDLER.THREAD_ACTIVE_ROUTER_IDS,
    SPINEL.PROP_THREAD_RLOC16_DEBUG_PASSTHRU: WPAN_PROP_HANDLER.THREAD_RLOC16_DEBUG_PASSTHRU,

    SPINEL.PROP_MESHCOP_JOINER_ENABLE: WPAN_PROP_HANDLER.MESHCOP_JOINER_ENABLE,
    SPINEL.PROP_MESHCOP_JOINER_CREDENTIAL: WPAN_PROP_HANDLER.MESHCOP_JOINER_CREDENTIAL,
    SPINEL.PROP_MESHCOP_JOINER_URL: WPAN_PROP_HANDLER.MESHCOP_JOINER_URL,
    SPINEL.PROP_MESHCOP_BORDER_AGENT_ENABLE: WPAN_PROP_HANDLER.MESHCOP_BORDER_AGENT_ENABLE,


    SPINEL.PROP_IPV6_LL_ADDR:           WPAN_PROP_HANDLER.IPV6_LL_ADDR,
    SPINEL.PROP_IPV6_ML_ADDR:           WPAN_PROP_HANDLER.IPV6_ML_ADDR,
    SPINEL.PROP_IPV6_ML_PREFIX:         WPAN_PROP_HANDLER.IPV6_ML_PREFIX,
    SPINEL.PROP_IPV6_ADDRESS_TABLE:     WPAN_PROP_HANDLER.IPV6_ADDRESS_TABLE,
    SPINEL.PROP_IPV6_ROUTE_TABLE:       WPAN_PROP_HANDLER.IPV6_ROUTE_TABLE,
    SPINEL.PROP_IPv6_ICMP_PING_OFFLOAD: WPAN_PROP_HANDLER.IPv6_ICMP_PING_OFFLOAD,

    SPINEL.PROP_STREAM_DEBUG:           WPAN_PROP_HANDLER.STREAM_DEBUG,
    SPINEL.PROP_STREAM_RAW:             WPAN_PROP_HANDLER.STREAM_RAW,
    SPINEL.PROP_STREAM_NET:             WPAN_PROP_HANDLER.STREAM_NET,
    SPINEL.PROP_STREAM_NET_INSECURE:    WPAN_PROP_HANDLER.STREAM_NET_INSECURE,

    SPINEL.PROP_PIB_15_4_PHY_CHANNELS_SUPPORTED: WPAN_PROP_HANDLER.PIB_PHY_CHANNELS_SUPPORTED,
    SPINEL.PROP_PIB_15_4_MAC_PROMISCUOUS_MODE: WPAN_PROP_HANDLER.PIB_MAC_PROMISCUOUS_MODE,
    SPINEL.PROP_PIB_15_4_MAC_SECURITY_ENABLED: WPAN_PROP_HANDLER.PIB_MAC_SECURITY_ENABLED,
    
    SPINEL.PROP_MSG_BUFFER_COUNTERS: WPAN_PROP_HANDLER.MSG_BUFFER_COUNTERS
}


class WpanApi(SpinelCodec):
    """ Helper class to format wpan command packets """

    def __init__(self, stream, nodeid, use_hdlc=FEATURE_USE_HDLC):
        self.stream = stream
        self.nodeid = nodeid

        self.use_hdlc = use_hdlc
        if self.use_hdlc:
            self.hdlc = Hdlc(self.stream)

        # PARSER state
        self.rx_pkt = []
        self.callback = defaultdict(list)  # Map prop_id to list of callbacks.

        # Fire up threads
        self._reader_alive = True
        self.tid_filter = set()
        self.__queue_prop = defaultdict(Queue.Queue)  # Map tid to Queue.
        self.queue_register()
        self.__start_reader()

    def __del__(self):
        self._reader_alive = False

    def __start_reader(self):
        """Start reader thread"""
        self._reader_alive = True
        # start serial->console thread
        self.receiver_thread = threading.Thread(target=self.stream_rx)
        self.receiver_thread.setDaemon(True)
        self.receiver_thread.start()

    def transact(self, command_id, payload="", tid=SPINEL.HEADER_DEFAULT):
        pkt = self.encode_packet(command_id, payload, tid)
        if CONFIG.DEBUG_LOG_SERIAL:
            msg = "TX Pay: (%i) %s " % (len(pkt), util.hexify_bytes(pkt))
            logging.debug(msg)

        if self.use_hdlc:
            pkt = self.hdlc.encode(pkt)
        self.stream_tx(pkt)

    def parse_rx(self, pkt):
        if not pkt:
            return

        if CONFIG.DEBUG_LOG_SERIAL:
            msg = "RX Pay: (%i) %s " % (
                len(pkt), str(map(util.hexify_int, pkt)))
            logging.debug(msg)

        length = len(pkt) - 2
        if length < 0:
            return

        spkt = "".join(map(chr, pkt))

        tid = self.parse_C(spkt[:1])
        (cmd_id, cmd_length) = self.parse_i(spkt[1:])
        pay_start = cmd_length + 1
        payload = spkt[pay_start:]

        try:
            handler = SPINEL_COMMAND_DISPATCH[cmd_id]
            cmd_name = handler.__name__
            handler(self, payload, tid)

        except Exception as _ex:
            print(traceback.format_exc())
            cmd_name = "CB_Unknown"
            logging.info("\n%s (%i): ", cmd_name, cmd_id)

        if CONFIG.DEBUG_CMD_RESPONSE:
            logging.info("\n%s (%i): ", cmd_name, cmd_id)
            logging.info("===> %s", util.hexify_str(payload))

    def stream_tx(self, pkt):
        # Encapsulate lagging and Framer support in self.stream class.
        self.stream.write(pkt)

    def stream_rx(self):
        """ Recieve thread and parser. """
        while self._reader_alive:
            if self.use_hdlc:
                self.rx_pkt = self.hdlc.collect()
            else:
                # size=None: Assume stream will always deliver packets
                pkt = self.stream.read(None)
                self.rx_pkt = util.packed_to_array(pkt)

            self.parse_rx(self.rx_pkt)

    class PropertyItem(object):
        """ Queue item for NCP response to property commands. """

        def __init__(self, prop, value, tid):
            self.prop = prop
            self.value = value
            self.tid = tid

    def callback_register(self, prop, cb):
        self.callback[prop].append(cb)

    def queue_register(self, tid=SPINEL.HEADER_DEFAULT):
        self.tid_filter.add(tid)
        return self.__queue_prop[tid]

    def queue_wait_prepare(self, _prop_id, tid=SPINEL.HEADER_DEFAULT):
        self.queue_clear(tid)

    def queue_add(self, prop, value, tid):
        cb_list = self.callback[prop]

        # Asynchronous handlers can consume message and not add to queue.
        if len(cb_list) > 0:
            consumed = cb_list[0](prop, value, tid)
            if consumed: return

        if tid not in self.tid_filter:
            return
        item = self.PropertyItem(prop, value, tid)
        self.__queue_prop[tid].put_nowait(item)

    def queue_clear(self, tid):
        with self.__queue_prop[tid].mutex:
            self.__queue_prop[tid].queue.clear()

    def queue_get(self, tid, timeout = None):
        try:
            if (timeout):
                item = self.__queue_prop[tid].get(True, timeout)
            else:
                item = self.__queue_prop[tid].get_nowait()
        except Queue.Empty:
            item = None
        return item

    def queue_wait_for_prop(self, _prop, tid=SPINEL.HEADER_DEFAULT, timeout=TIMEOUT_PROP):
        start = time.time()
        item = self.queue_get(tid, timeout)
        if (_prop is not None):
            while (item and (item.prop != _prop)):
                self.__queue_prop[tid].put_nowait(item)
                reminder = timeout - (time.time() - start)
                if (reminder <= 0.0):
                    item = None
                    break
                item = self.queue_get(tid, reminder)
                # self.__queue_prop[tid].task_done()
        return item

    def ip_send(self, pkt):
        pay = self.encode_i(SPINEL.PROP_STREAM_NET)

        pkt_len = len(pkt)
        pay += pack("<H", pkt_len)          # Start with length of IPv6 packet

        pkt_len += 2                       # Increment to include length word
        pay += pack("%ds" % pkt_len, pkt)  # Append packet after length

        self.transact(SPINEL.CMD_PROP_VALUE_SET, pay)

    def cmd_reset(self):
        self.queue_wait_prepare(None, SPINEL.HEADER_ASYNC)
        self.transact(SPINEL.CMD_RESET, "", SPINEL.HEADER_DEFAULT)
        result = self.queue_wait_for_prop(SPINEL.PROP_LAST_STATUS, SPINEL.HEADER_ASYNC, 5)
        return (result is not None and result.value == 114)

    def cmd_send(self, command_id, payload="", tid=SPINEL.HEADER_DEFAULT):
        self.queue_wait_prepare(None, tid)
        self.transact(command_id, payload, tid)
        self.queue_wait_for_prop(None, tid)

    def prop_change_async(self, cmd, prop_id, value, py_format='B',
                          tid=SPINEL.HEADER_DEFAULT):
        pay = self.encode_i(prop_id)
        if py_format != None:
            pay += pack(py_format, value)
        self.transact(cmd, pay, tid)

    def prop_insert_async(self, prop_id, value, py_format='B',
                          tid=SPINEL.HEADER_DEFAULT):
        self.prop_change_async(SPINEL.CMD_PROP_VALUE_INSERT, prop_id,
                               value, py_format, tid)

    def prop_remove_async(self, prop_id, value, py_format='B',
                          tid=SPINEL.HEADER_DEFAULT):
        self.prop_change_async(SPINEL.CMD_PROP_VALUE_REMOVE, prop_id,
                               value, py_format, tid)

    def __prop_change_value(self, cmd, prop_id, value, py_format='B',
                            tid=SPINEL.HEADER_DEFAULT):
        """ Utility routine to change a property value over SPINEL. """
        self.queue_wait_prepare(prop_id, tid)

        pay = self.encode_i(prop_id)
        if py_format != None:
            pay += pack(py_format, value)
        self.transact(cmd, pay, tid)

        result = self.queue_wait_for_prop(prop_id, tid)
        if result:
            return result.value
        else:
            return None

    def prop_get_value(self, prop_id, tid=SPINEL.HEADER_DEFAULT):
        """ Blocking routine to get a property value over SPINEL. """
        if CONFIG.DEBUG_LOG_PROP:
            handler = SPINEL_PROP_DISPATCH[prop_id]
            prop_name = handler.__name__
            print("PROP_VALUE_GET [tid=%d]: %s" % (tid & 0xF, prop_name))
        return self.__prop_change_value(SPINEL.CMD_PROP_VALUE_GET, prop_id,
                                        None, None, tid)

    def prop_set_value(self, prop_id, value, py_format='B',
                       tid=SPINEL.HEADER_DEFAULT):
        """ Blocking routine to set a property value over SPINEL. """
        if CONFIG.DEBUG_LOG_PROP:
            handler = SPINEL_PROP_DISPATCH[prop_id]
            prop_name = handler.__name__
            print("PROP_VALUE_SET [tid=%d]: %s" % (tid & 0xF, prop_name))
        return self.__prop_change_value(SPINEL.CMD_PROP_VALUE_SET, prop_id,
                                        value, py_format, tid)

    def prop_insert_value(self, prop_id, value, py_format='B',
                          tid=SPINEL.HEADER_DEFAULT):
        """ Blocking routine to insert a property value over SPINEL. """
        if CONFIG.DEBUG_LOG_PROP:
            handler = SPINEL_PROP_DISPATCH[prop_id]
            prop_name = handler.__name__
            print("PROP_VALUE_INSERT [tid=%d]: %s" % (tid & 0xF, prop_name))
        return self.__prop_change_value(SPINEL.CMD_PROP_VALUE_INSERT, prop_id,
                                        value, py_format, tid)

    def prop_remove_value(self, prop_id, value, py_format='B',
                          tid=SPINEL.HEADER_DEFAULT):
        """ Blocking routine to remove a property value over SPINEL. """
        if CONFIG.DEBUG_LOG_PROP:
            handler = SPINEL_PROP_DISPATCH[prop_id]
            prop_name = handler.__name__
            print("PROP_VALUE_REMOVE [tid=%d]: %s" % (tid & 0xF, prop_name))
        return self.__prop_change_value(SPINEL.CMD_PROP_VALUE_REMOVE, prop_id,
                                        value, py_format, tid)

    def get_ipaddrs(self, tid=SPINEL.HEADER_DEFAULT):
        """
        Return current list of ip addresses for the device.
        """
        value = self.prop_get_value(SPINEL.PROP_IPV6_ADDRESS_TABLE, tid)
        # TODO: clean up table parsing to be less hard-coded magic.
        if value is None:
            return None
        size = 0x1B
        addrs = [value[i:i + size] for i in xrange(0, len(value), size)]
        ipaddrs = []
        for addr in addrs:
            addr = addr[2:18]
            ipaddrs.append(ipaddress.IPv6Address(addr))
        return ipaddrs
