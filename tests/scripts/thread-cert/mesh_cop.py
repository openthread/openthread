
import io
import struct

from enum import IntEnum

class TlvType(IntEnum):
    STATE = 16,
    PROVISIONING_URL = 32,
    VENDOR_NAME = 33,
    VENDOR_MODEL = 34,
    VENDOR_SW_VERSION = 35,
    VENDOR_DATA = 36,
    VENDOR_STACK_VERSION = 37,

class MeshCopMessageType(IntEnum):
    JOIN_FIN_REQ = 1,
    JOIN_FIN_RSP = 2,
    JOIN_ENT_NTF = 3,
    JOIN_ENT_RSP = 4

def create_mesh_cop_message_type_set():
    return [ MeshCopMessageType.JOIN_FIN_REQ,
             MeshCopMessageType.JOIN_FIN_RSP,
             MeshCopMessageType.JOIN_ENT_NTF,
             MeshCopMessageType.JOIN_ENT_RSP ]

class State(object):

    def __init__(self, state):
        self._state = state

    @property
    def state(self):
        return self._state

    def __eq__(self, other):
        return self.state == other.state

    def __repr__(self):
        return "State(state={})".format(self.state)


class StateFactory:

    def parse(self, data):
        state = ord(data.read(1))
        return State(state)

class VendorName(object):

    def __init__(self, vendor_name):
        self._vendor_name = vendor_name

    @property
    def vendor_name(self):
        return self._vendor_name

    def __eq__(self, other):
        return self.vendor_name == other.vendor_name

    def __repr__(self):
        return "VendorName(vendor_name={})".format(self.vendor_name)

class VendorNameFactory:

    def parse(self, data):
        vendor_name = data.getvalue().decode('utf-8')
        return VendorName(vendor_name)

class VendorModel(object):

    def __init__(self, vendor_model):
        self._vendor_model = vendor_model

    @property
    def vendor_model(self):
        return self._vendor_model

    def __eq__(self, other):
        return self.vendor_model == other.vendor_model

    def __repr__(self):
        return "VendorModel(vendor_model={})".format(self.vendor_model)

class VendorModelFactory:

    def parse(self, data):
        vendor_model = data.getvalue().decode('utf-8')
        return VendorModel(vendor_model)

class VendorSWVersion(object):

    def __init__(self, vendor_sw_version):
        self._vendor_sw_version = vendor_sw_version

    @property
    def vendor_sw_version(self):
        return self._vendor_sw_version

    def __eq__(self, other):
        return self.vendor_sw_version == other.vendor_sw_version

    def __repr__(self):
        return "VendorName(vendor_sw_version={})".format(self.vendor_sw_version)

class VendorSWVersionFactory:

    def parse(self, data):
        vendor_sw_version = data.getvalue()
        return VendorSWVersion(vendor_sw_version)

class VendorStackVersion(object):

    def __init__(self, stack_vendor_oui):
        self._stack_vendor_oui = stack_vendor_oui
        return

    @property
    def stack_vendor_oui(self):
        return self._stack_vendor_oui

    def __repr__(self):
        return "VendorStackVersion(vendor_stack_version={})".format(self._stack_vendor_oui)

class VendorStackVersionFactory:

    def parse(self, data):
        stack_vendor_oui = struct.unpack(">H", data.read(2))[0]
        return VendorStackVersion(stack_vendor_oui)

class ProvisioningUrl(object):

    def __init__(self, url):
        self._url = url
    
    @property
    def url(self):
        return self._url
    
    def __repr__(self):
        return "ProvisioningUrl(url={})".format(self.url)

class ProvisioningUrlFactory:

    def parse(self, data):
        url = data.decode('utf-8')
        return ProvisioningUrl(url)

class VendorData(object):

    def __init__(self, data):
        self._vendor_data = data
    
    @property
    def vendor_data(self):
        return self._vendor_data

    def __repr__(self):
        return "Vendor(url={})".format(self.vendor_data)

class VendorDataFactory(object):

    def parse(self, data):
        return VendorData(data)

class MeshCopCommand(object):

    def __init__(self, _type, tlvs):
        self._type = _type
        self._tlvs = tlvs

    @property
    def type(self):
        return self._type

    @property
    def tlvs(self):
        return self._tlvs

    def __repr__(self):
        tlvs_str = ", ".join(["{}".format(tlv) for tlv in self.tlvs])
        return "MeshCopCommand(type={}, tlvs=[{}])".format(self.type, tlvs_str)

def create_deault_mesh_cop_msg_type_map():
    return {
        'JOIN_FIN.req': MeshCopMessageType.JOIN_FIN_REQ,
        'JOIN_FIN.rsp': MeshCopMessageType.JOIN_FIN_RSP,
        'JOIN_ENT.ntf': MeshCopMessageType.JOIN_ENT_NTF,
        'JOIN_ENT.rsp': MeshCopMessageType.JOIN_ENT_RSP
    }

class MeshCopCommandFactory:

    def __init__(self, tlvs_factories):
        self._tlvs_factories = tlvs_factories
        self._mesh_cop_msg_type_map = create_deault_mesh_cop_msg_type_map()

    def _get_length(self, data):
        return ord(data.read(1))

    def _get_tlv_factory(self, _type):
        try:
            return self._tlvs_factories[_type]
        except KeyError:
            raise KeyError("Could not find TLV factory. Unsupported TLV type: {}".format(_type))

    def _parse_tlv(self, data):
        _type = TlvType(ord(data.read(1)))
        length = self._get_length(data)
        value = data.read(length)
        factory = self._get_tlv_factory(_type)
        if factory == None:
            return None
        return factory.parse(io.BytesIO(value))

    def _get_mesh_cop_msg_type(self, msg_type_str):
        tp = self._mesh_cop_msg_type_map[msg_type_str]
        if tp == None:
            raise RuntimeError('Mesh cop message type not found: {}'.format(msg_type_str))
        return tp

    def parse(self, cmd_type_str, data):
        cmd_type = self._get_mesh_cop_msg_type(cmd_type_str)

        tlvs = []
        while data.tell() < len(data.getvalue()):
            tlv = self._parse_tlv(data)
            tlvs.append(tlv)

        return MeshCopCommand(cmd_type, tlvs)


def create_default_mesh_cop_tlv_factories():
    return {
        TlvType.STATE: StateFactory(),
        TlvType.PROVISIONING_URL: ProvisioningUrlFactory(),
        TlvType.VENDOR_NAME: VendorNameFactory(),
        TlvType.VENDOR_MODEL: VendorModelFactory(),
        TlvType.VENDOR_SW_VERSION: VendorSWVersionFactory(),
        TlvType.VENDOR_DATA: VendorDataFactory(),
        TlvType.VENDOR_STACK_VERSION: VendorStackVersionFactory()
    }
