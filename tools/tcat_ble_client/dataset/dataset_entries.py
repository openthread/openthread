"""
   Copyright (c) 2023 Nordic Semiconductor ASA

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.
"""

import struct
import inspect
from typing import List
from abc import ABC, abstractmethod

from tlv.dataset_tlv import MeshcopTlvType
from tlv.tlv import TLV


class DatasetEntry(ABC):

    def __init__(self, type: MeshcopTlvType):
        self.type = type
        self.length = None
        self.maxlen = None

    def print_content(self, indent: int = 0, excluded_fields: List[str] = []):
        excluded_fields += ['length', 'maxlen', 'type']
        indentation = " " * 4 * indent
        for attr_name in dir(self):
            if not attr_name.startswith('_') and attr_name not in excluded_fields:
                value = getattr(self, attr_name)
                if not inspect.ismethod(value):
                    if isinstance(value, bytes):
                        value = value.hex()
                    print(f'{indentation}{attr_name}: {value}')

    @abstractmethod
    def to_tlv(self) -> TLV:
        pass

    @abstractmethod
    def set_from_tlv(self, tlv: TLV):
        pass

    @abstractmethod
    def set(self, args: List[str]):
        pass


class ActiveTimestamp(DatasetEntry):

    def __init__(self):
        super().__init__(MeshcopTlvType.ACTIVETIMESTAMP)
        self.length = 8  # spec defined
        self.seconds = 0
        self.ubit = 0
        self.ticks = 0

    def set(self, args: List[str]):
        if len(args) == 0:
            raise ValueError('No argument for ActiveTimestamp')
        self._seconds = int(args[0])

    def set_from_tlv(self, tlv: TLV):
        (value,) = struct.unpack('>Q', tlv.value)
        self.ubit = value & 0x1
        self.ticks = (value >> 1) & 0x7FFF
        self.seconds = (value >> 16) & 0xFFFF

    def to_tlv(self):
        value = (self.seconds << 16) | (self.ticks << 1) | self.ubit
        tlv = struct.pack('>BBQ', self.type.value, self.length, value)
        return TLV.from_bytes(tlv)


class PendingTimestamp(DatasetEntry):

    def __init__(self):
        super().__init__(MeshcopTlvType.PENDINGTIMESTAMP)
        self.length = 8  # spec defined
        self.seconds = 0
        self.ubit = 0
        self.ticks = 0

    def set(self, args: List[str]):
        if len(args) == 0:
            raise ValueError('No argument for PendingTimestamp')
        self._seconds = int(args[0])

    def set_from_tlv(self, tlv: TLV):
        (value,) = struct.unpack('>Q', tlv.value)
        self.ubit = value & 0x1
        self.ticks = (value >> 1) & 0x7FFF
        self.seconds = (value >> 16) & 0xFFFF

    def to_tlv(self):
        value = (self.seconds << 16) | (self.ticks << 1) | self.ubit
        tlv = struct.pack('>BBQ', self.type.value, self.length, value)
        return TLV.from_bytes(tlv)


class NetworkKey(DatasetEntry):

    def __init__(self):
        super().__init__(MeshcopTlvType.NETWORKKEY)
        self.length = 16  # spec defined
        self.data: str = ''

    def set(self, args: List[str]):
        if len(args) == 0:
            raise ValueError('No argument for NetworkKey')
        if args[0].startswith('0x'):
            args[0] = args[0][2:]
        nk = args[0]
        if len(nk) != self.length * 2:  # need length * 2 hex characters
            raise ValueError('Invalid length of NetworkKey')
        self.data = nk

    def set_from_tlv(self, tlv: TLV):
        self.data = tlv.value.hex()

    def to_tlv(self):
        if len(self.data) != self.length * 2:  # need length * 2 hex characters
            raise ValueError('Invalid length of NetworkKey')
        value = bytes.fromhex(self.data)
        tlv = struct.pack('>BB', self.type.value, self.length) + value
        return TLV.from_bytes(tlv)


class NetworkName(DatasetEntry):

    def __init__(self):
        super().__init__(MeshcopTlvType.NETWORKNAME)
        self.maxlen = 16
        self.data: str = ''

    def set(self, args: List[str]):
        if len(args) == 0:
            raise ValueError('No argument for NetworkName')
        nn = args[0]
        if len(nn) > self.maxlen:
            raise ValueError('Invalid length of NetworkName')
        self.data = nn

    def set_from_tlv(self, tlv: TLV):
        self.data = tlv.value.decode('utf-8')

    def to_tlv(self):
        length_value = len(self.data)
        value = self.data.encode('utf-8')
        tlv = struct.pack('>BB', self.type.value, length_value) + value
        return TLV.from_bytes(tlv)


class ExtPanID(DatasetEntry):

    def __init__(self):
        super().__init__(MeshcopTlvType.EXTPANID)
        self.length = 8  # spec defined
        self.data: str = ''

    def set(self, args: List[str]):
        if len(args) == 0:
            raise ValueError('No argument for ExtPanID')
        if args[0].startswith('0x'):
            args[0] = args[0][2:]
        epid = args[0]
        if len(epid) != self.length * 2:  # need length*2 hex characters
            raise ValueError('Invalid length of ExtPanID')
        self.data = epid

    def set_from_tlv(self, tlv: TLV):
        self.data = tlv.value.hex()

    def to_tlv(self):
        if len(self.data) != self.length * 2:  # need length*2 hex characters
            raise ValueError('Invalid length of ExtPanID')

        value = bytes.fromhex(self.data)
        tlv = struct.pack('>BB', self.type.value, self.length) + value
        return TLV.from_bytes(tlv)


class MeshLocalPrefix(DatasetEntry):

    def __init__(self):
        super().__init__(MeshcopTlvType.MESHLOCALPREFIX)
        self.length = 8  # spec defined
        self.data = ''

    def set(self, args: List[str]):
        if len(args) == 0:
            raise ValueError('No argument for MeshLocalPrefix')
        if args[0].startswith('0x'):
            args[0] = args[0][2:]
        mlp = args[0]
        if len(mlp) != self.length * 2:  # need length*2 hex characters
            raise ValueError('Invalid length of MeshLocalPrefix')
        self.data = mlp

    def set_from_tlv(self, tlv: TLV):
        self.data = tlv.value.hex()

    def to_tlv(self):
        if len(self.data) != self.length * 2:  # need length*2 hex characters
            raise ValueError('Invalid length of MeshLocalPrefix')

        value = bytes.fromhex(self.data)
        tlv = struct.pack('>BB', self.type.value, self.length) + value
        return TLV.from_bytes(tlv)


class DelayTimer(DatasetEntry):

    def __init__(self):
        super().__init__(MeshcopTlvType.DELAYTIMER)
        self.length = 4  # spec defined
        self.time_remaining = 0

    def set(self, args: List[str]):
        if len(args) == 0:
            raise ValueError('No argument for DelayTimer')
        dt = int(args[0])
        self.time_remaining = dt

    def set_from_tlv(self, tlv: TLV):
        self.time_remaining = tlv.value

    def to_tlv(self):
        value = self.time_remaining
        tlv = struct.pack('>BBI', self.type.value, self.length, value)
        return TLV.from_bytes(tlv)


class PanID(DatasetEntry):

    def __init__(self):
        super().__init__(MeshcopTlvType.PANID)
        self.length = 2  # spec defined
        self.data: str = ''

    def set(self, args: List[str]):
        if len(args) == 0:
            raise ValueError('No argument for PanID')
        if args[0].startswith('0x'):
            args[0] = args[0][2:]
        pid = args[0]
        if len(pid) != self.length * 2:  # need length*2 hex characters
            raise ValueError('Invalid length of PanID')
        self.data = pid

    def set_from_tlv(self, tlv: TLV):
        self.data = tlv.value.hex()

    def to_tlv(self):
        if len(self.data) != self.length * 2:  # need length*2 hex characters
            raise ValueError('Invalid length of PanID')

        value = bytes.fromhex(self.data)
        tlv = struct.pack('>BB', self.type.value, self.length) + value
        return TLV.from_bytes(tlv)


class Channel(DatasetEntry):

    def __init__(self):
        super().__init__(MeshcopTlvType.CHANNEL)
        self.length = 3  # spec defined
        self.channel_page = 0
        self.channel = 0

    def set(self, args: List[str]):
        if len(args) == 0:
            raise ValueError('No argument for Channel')
        channel = int(args[0])
        self.channel = channel

    def set_from_tlv(self, tlv: TLV):
        self.channel = int.from_bytes(tlv.value[1:3], byteorder='big')
        self.channel_page = tlv.value[0]

    def to_tlv(self):
        tlv = struct.pack('>BBB', self.type.value, self.length, self.channel_page)
        tlv += struct.pack('>H', self.channel)
        return TLV.from_bytes(tlv)


class Pskc(DatasetEntry):

    def __init__(self):
        super().__init__(MeshcopTlvType.PSKC)
        self.maxlen = 16
        self.data = ''

    def set(self, args: List[str]):
        if len(args) == 0:
            raise ValueError('No argument for Pskc')
        if args[0].startswith('0x'):
            args[0] = args[0][2:]
        pskc = args[0]
        if (len(pskc) > self.maxlen * 2):
            raise ValueError('Invalid length of Pskc. Can be max ' f'{self.length * 2} hex characters.')
        self.data = pskc

    def set_from_tlv(self, tlv: TLV):
        self.data = tlv.value.hex()

    def to_tlv(self):
        # should not exceed max length*2 hex characters
        if (len(self.data) > self.maxlen * 2):
            raise ValueError('Invalid length of Pskc')

        length_value = len(self.data) // 2
        value = bytes.fromhex(self.data)
        tlv = struct.pack('>BB', self.type.value, length_value) + value
        return TLV.from_bytes(tlv)


class SecurityPolicy(DatasetEntry):

    def __init__(self):
        super().__init__(MeshcopTlvType.SECURITYPOLICY)
        self.length = 4  # spec defined
        self.rotation_time = 0
        self.out_of_band = 0  # o
        self.native = 0  # n
        self.routers_1_2 = 0  # r
        self.external_commissioners = 0  # c
        self.reserved = 0
        self.commercial_commissioning_off = 0  # C
        self.autonomous_enrollment_off = 0  # e
        self.networkkey_provisioning_off = 0  # p
        self.thread_over_ble = 0
        self.non_ccm_routers_off = 0  # R
        self.rsv = 0b111
        self.version_threshold = 0

    def set(self, args: List[str]):
        if len(args) == 0:
            raise ValueError('No argument for SecurityPolicy')
        rotation_time, flags, version_threshold = args + [None] * (3 - len(args))
        self.rotation_time = int(rotation_time) & 0xffff

        if flags:
            self.out_of_band = 1 if 'o' in flags else 0
            self.native = 1 if 'n' in flags else 0
            self.routers_1_2 = 1 if 'r' in flags else 0
            self.external_commissioners = 1 if 'c' in flags else 0
            self.commercial_commissioning_off = 0 if 'C' in flags else 1
            self.autonomous_enrollment_off = 0 if 'e' in flags else 1
            self.networkkey_provisioning_off = 0 if 'p' in flags else 1
            self.non_ccm_routers_off = 0 if 'R' in flags else 1

        if version_threshold:
            self.version_threshold = int(version_threshold) & 0b111

    def set_from_tlv(self, tlv: TLV):
        value = int.from_bytes(tlv.value, byteorder='big')

        self.rotation_time = (value >> 16) & 0xFFFF
        self.out_of_band = (value >> 15) & 0x1
        self.native = (value >> 14) & 0x1
        self.routers_1_2 = (value >> 13) & 0x1
        self.external_commissioners = (value >> 12) & 0x1
        self.reserved = (value >> 11) & 0x1
        self.commercial_commissioning_off = (value >> 10) & 0x1
        self.autonomous_enrollment_off = (value >> 9) & 0x1
        self.networkkey_provisioning_off = (value >> 8) & 0x1
        self.thread_over_ble = (value >> 7) & 0x1
        self.non_ccm_routers_off = (value >> 6) & 0x1
        self.rsv = (value >> 3) & 0x7
        self.version_threshold = value & 0x7

    def to_tlv(self):
        value = self.rotation_time << 16
        value |= self.out_of_band << 15
        value |= self.native << 14
        value |= self.routers_1_2 << 13
        value |= self.external_commissioners << 12
        value |= self.reserved << 11
        value |= self.commercial_commissioning_off << 10
        value |= self.autonomous_enrollment_off << 9
        value |= self.networkkey_provisioning_off << 8
        value |= self.thread_over_ble << 7
        value |= self.non_ccm_routers_off << 6
        value |= self.rsv << 3
        value |= self.version_threshold
        tlv = struct.pack('>BBI', self.type.value, self.length, value)
        return TLV.from_bytes(tlv)

    def print_content(self, indent: int = 0):
        flags = ''
        if self.out_of_band:
            flags += 'o'
        if self.native:
            flags += 'n'
        if self.routers_1_2:
            flags += 'r'
        if self.external_commissioners:
            flags += 'c'
        if not self.commercial_commissioning_off:
            flags += 'C'
        if not self.autonomous_enrollment_off:
            flags += 'e'
        if not self.networkkey_provisioning_off:
            flags += 'p'
        if not self.non_ccm_routers_off:
            flags += 'R'
        indentation = " " * 4 * indent
        print(f'{indentation}rotation_time: {self.rotation_time}')
        print(f'{indentation}flags: {flags}')
        print(f'{indentation}version_threshold: {self.version_threshold}')


class ChannelMask(DatasetEntry):

    def __init__(self):
        super().__init__(MeshcopTlvType.CHANNELMASK)
        self.entries: List[ChannelMaskEntry] = []

    def set(self, args: List[str]):
        # to remain consistent with the OpenThread CLI API,
        # provided hex string is value of the first channel mask entry
        if len(args) == 0:
            raise ValueError('No argument for ChannelMask')
        if args[0].startswith('0x'):
            args[0] = args[0][2:]
        channelmsk = bytes.fromhex(args[0])
        self.entries = [ChannelMaskEntry()]
        self.entries[0].channel_mask = channelmsk

    def print_content(self, indent: int = 0):
        super().print_content(indent=indent, excluded_fields=['entries'])
        indentation = " " * 4 * indent
        for i, entry in enumerate(self.entries):
            print(f'{indentation}ChannelMaskEntry {i}')
            entry.print_content(indent=indent + 1)

    def set_from_tlv(self, tlv: TLV):
        self.entries = []
        for mask_entry_tlv in TLV.parse_tlvs(tlv.value):
            new_entry = ChannelMaskEntry()
            new_entry.set_from_tlv(mask_entry_tlv)
            self.entries.append(new_entry)

    def to_tlv(self):
        tlv_value = b''.join(mask_entry.to_tlv().to_bytes() for mask_entry in self.entries)
        tlv = struct.pack('>BB', self.type.value, len(tlv_value)) + tlv_value
        return TLV.from_bytes(tlv)


class ChannelMaskEntry(DatasetEntry):

    def __init__(self):
        self.channel_page = 0
        self.channel_mask: bytes = None

    def set(self, args: List[str]):
        pass

    def set_from_tlv(self, tlv: TLV):
        self.channel_page = tlv.type
        self.mask_length = len(tlv.value)
        self.channel_mask = tlv.value

    def to_tlv(self):
        mask_len = len(self.channel_mask)
        tlv = struct.pack('>BB', self.channel_page, mask_len) + self.channel_mask
        return TLV.from_bytes(tlv)


ENTRY_CLASSES = {
    MeshcopTlvType.ACTIVETIMESTAMP: ActiveTimestamp,
    MeshcopTlvType.PENDINGTIMESTAMP: PendingTimestamp,
    MeshcopTlvType.NETWORKKEY: NetworkKey,
    MeshcopTlvType.NETWORKNAME: NetworkName,
    MeshcopTlvType.EXTPANID: ExtPanID,
    MeshcopTlvType.MESHLOCALPREFIX: MeshLocalPrefix,
    MeshcopTlvType.DELAYTIMER: DelayTimer,
    MeshcopTlvType.PANID: PanID,
    MeshcopTlvType.CHANNEL: Channel,
    MeshcopTlvType.PSKC: Pskc,
    MeshcopTlvType.SECURITYPOLICY: SecurityPolicy,
    MeshcopTlvType.CHANNELMASK: ChannelMask
}


def create_dataset_entry(type: MeshcopTlvType, args=None):
    entry_class = ENTRY_CLASSES.get(type)
    if not entry_class:
        raise ValueError(f"Invalid configuration type: {type}")

    res = entry_class()
    if args:
        res.set(args)
    return res
