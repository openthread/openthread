#!/usr/bin/env python
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
from enum import Enum

import coap
import ipv6
import lowpan
import message
import mle
import net_crypto
import network_data
import network_layer
import simulator
import sniffer

MESH_LOCAL_PREFIX = 'fdde:ad00:beef::/64'
MESH_LOCAL_PREFIX_REGEX_PATTERN = '^fdde:ad00:beef:(0){0,4}:'
ROUTING_LOCATOR = '64/:0:ff:fe00:/16'
ROUTING_LOCATOR_REGEX_PATTERN = '.*:(0)?:0{0,2}ff:fe00:\w{1,4}$'
LINK_LOCAL = 'fe80:/112'
LINK_LOCAL_REGEX_PATTERN = '^fe80:.*'
ALOC_FLAG_REGEX_PATTERN = '.*:fc..$'
LINK_LOCAL_All_THREAD_NODES_MULTICAST_ADDRESS = 'ff32:40:fdde:ad00:beef:0:0:1'
REALM_LOCAL_All_THREAD_NODES_MULTICAST_ADDRESS = 'ff33:40:fdde:ad00:beef:0:0:1'
REALM_LOCAL_ALL_ROUTERS_ADDRESS = 'ff03::2'
LINK_LOCAL_ALL_NODES_ADDRESS = 'ff02::1'
LINK_LOCAL_ALL_ROUTERS_ADDRESS = 'ff02::2'

DEFAULT_MASTER_KEY = bytearray([0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77,
                                0x88, 0x99, 0xaa, 0xbb, 0xcc, 0xdd, 0xee, 0xff])

ADDRESS_TYPE = Enum('ADDRESS_TYPE', ('LINK_LOCAL', 'GLOBAL', 'RLOC', 'ALOC', 'ML_EID'))
RSSI = {'LINK_QULITY_0': -100, 'LINK_QULITY_1': -95, 'LINK_QULITY_2': -85, 'LINK_QULITY_3': -65}

SNIFFER_ID = int(os.getenv('SNIFFER_ID', 34))
PANID = 0xface

MAX_NEIGHBOR_AGE = 100
INFINITE_COST_TIMEOUT = 90

MAX_ADVERTISEMENT_INTERVAL = 32
MLE_END_DEVICE_TIMEOUT = 100

AQ_TIMEOUT = 3
ADDRESS_QUERY_INITIAL_RETRY_DELAY = 15
DEFAULT_CHILD_TIMEOUT = 6
VIRTUAL_TIME = int(os.getenv('VIRTUAL_TIME', 0))

LEADER_NOTIFY_SED_BY_CHILD_UPDATE_REQUEST = True

def create_default_network_data_prefix_sub_tlvs_factories():
    return {
        network_data.TlvType.HAS_ROUTE: network_data.HasRouteFactory(
            routes_factory=network_data.RoutesFactory(
                route_factory=network_data.RouteFactory())
        ),
        network_data.TlvType.BORDER_ROUTER: network_data.BorderRouterFactory(),
        network_data.TlvType.LOWPAN_ID: network_data.LowpanIdFactory()
    }


def create_default_network_data_prefix_sub_tlvs_factory():
    return network_data.PrefixSubTlvsFactory(
        sub_tlvs_factories=create_default_network_data_prefix_sub_tlvs_factories())


def create_default_network_data_service_sub_tlvs_factories():
    return {
        network_data.TlvType.SERVER: network_data.ServerFactory()
    }


def create_default_network_data_service_sub_tlvs_factory():
    return network_data.ServiceSubTlvsFactory(
        sub_tlvs_factories=create_default_network_data_service_sub_tlvs_factories())


def create_default_network_data_commissioning_data_sub_tlvs_factories():
    return {
        network_data.MeshcopTlvType.STEERING_DATA: network_data.SteeringDataFactory(),
        network_data.MeshcopTlvType.BORDER_AGENT_LOCATOR: network_data.BorderAgentLocatorFactory(),
        network_data.MeshcopTlvType.COMMISSIONER_SESSION_ID: network_data.CommissionerSessionIdFactory(),
        network_data.MeshcopTlvType.COMMISSIONER_UDP_PORT: network_data.CommissionerUdpPortFactory(),
    }


def create_default_network_data_commissioning_data_sub_tlvs_factory():
    return network_data.CommissioningDataSubTlvsFactory(
        sub_tlvs_factories=create_default_network_data_commissioning_data_sub_tlvs_factories())


def create_default_network_data_tlvs_factories():
    return {
        network_data.TlvType.PREFIX: network_data.PrefixFactory(
            sub_tlvs_factory=create_default_network_data_prefix_sub_tlvs_factory()
        ),
        network_data.TlvType.SERVICE: network_data.ServiceFactory(
            sub_tlvs_factory=create_default_network_data_service_sub_tlvs_factory()
        ),
        network_data.TlvType.COMMISSIONING: network_data.CommissioningDataFactory(
            sub_tlvs_factory=create_default_network_data_commissioning_data_sub_tlvs_factory()
        ),
    }


def create_default_network_data_tlvs_factory():
    return network_data.NetworkDataTlvsFactory(
        sub_tlvs_factories=create_default_network_data_tlvs_factories())


def create_default_mle_tlv_route64_factory():
    return mle.Route64Factory(
        link_quality_and_route_data_factory=mle.LinkQualityAndRouteDataFactory())


def create_default_mle_tlv_network_data_factory():
    return mle.NetworkDataFactory(
        network_data_tlvs_factory=create_default_network_data_tlvs_factory())


def create_default_mle_tlv_address_registration_factory():
    return mle.AddressRegistrationFactory(
        addr_compressed_factory=mle.AddressCompressedFactory(),
        addr_full_factory=mle.AddressFullFactory())


def create_default_mle_tlvs_factories():
    return {
        mle.TlvType.SOURCE_ADDRESS: mle.SourceAddressFactory(),
        mle.TlvType.MODE: mle.ModeFactory(),
        mle.TlvType.TIMEOUT: mle.TimeoutFactory(),
        mle.TlvType.CHALLENGE: mle.ChallengeFactory(),
        mle.TlvType.RESPONSE: mle.ResponseFactory(),
        mle.TlvType.LINK_LAYER_FRAME_COUNTER: mle.LinkLayerFrameCounterFactory(),
        mle.TlvType.MLE_FRAME_COUNTER: mle.MleFrameCounterFactory(),
        mle.TlvType.ROUTE64: create_default_mle_tlv_route64_factory(),
        mle.TlvType.ADDRESS16: mle.Address16Factory(),
        mle.TlvType.LEADER_DATA: mle.LeaderDataFactory(),
        mle.TlvType.NETWORK_DATA: create_default_mle_tlv_network_data_factory(),
        mle.TlvType.TLV_REQUEST: mle.TlvRequestFactory(),
        mle.TlvType.SCAN_MASK: mle.ScanMaskFactory(),
        mle.TlvType.CONNECTIVITY: mle.ConnectivityFactory(),
        mle.TlvType.LINK_MARGIN: mle.LinkMarginFactory(),
        mle.TlvType.STATUS: mle.StatusFactory(),
        mle.TlvType.VERSION: mle.VersionFactory(),
        mle.TlvType.ADDRESS_REGISTRATION: create_default_mle_tlv_address_registration_factory(),
        mle.TlvType.CHANNEL: mle.ChannelFactory(),
        mle.TlvType.PANID: mle.PanIdFactory(),
        mle.TlvType.ACTIVE_TIMESTAMP: mle.ActiveTimestampFactory(),
        mle.TlvType.PENDING_TIMESTAMP: mle.PendingTimestampFactory(),
        mle.TlvType.ACTIVE_OPERATIONAL_DATASET: mle.ActiveOperationalDatasetFactory(),
        mle.TlvType.PENDING_OPERATIONAL_DATASET: mle.PendingOperationalDatasetFactory(),
        mle.TlvType.THREAD_DISCOVERY: mle.ThreadDiscoveryFactory()
    }


def create_default_mle_crypto_engine(master_key):
    return net_crypto.CryptoEngine(crypto_material_creator=net_crypto.MleCryptoMaterialCreator(master_key))


def create_default_mle_message_factory(master_key):
    return mle.MleMessageFactory(
        aux_sec_hdr_factory=net_crypto.AuxiliarySecurityHeaderFactory(),
        mle_command_factory=mle.MleCommandFactory(
            tlvs_factories=create_default_mle_tlvs_factories()),
        crypto_engine=create_default_mle_crypto_engine(master_key))


def create_deafult_network_tlvs_factories():
    return {
        network_layer.TlvType.TARGET_EID: network_layer.TargetEidFactory(),
        network_layer.TlvType.MAC_EXTENDED_ADDRESS: network_layer.MacExtendedAddressFactory(),
        network_layer.TlvType.RLOC16: network_layer.Rloc16Factory(),
        network_layer.TlvType.ML_EID: network_layer.MlEidFactory(),
        network_layer.TlvType.STATUS: network_layer.StatusFactory(),
        network_layer.TlvType.TIME_SINCE_LAST_TRANSACTION: network_layer.TimeSinceLastTransactionFactory(),
        network_layer.TlvType.ROUTER_MASK: network_layer.RouterMaskFactory(),
        network_layer.TlvType.ND_OPTION: network_layer.NdOptionFactory(),
        network_layer.TlvType.ND_DATA: network_layer.NdDataFactory(),
        network_layer.TlvType.THREAD_NETWORK_DATA: network_layer.ThreadNetworkDataFactory(create_default_network_data_tlvs_factory()),

        # Routing information are distributed in a Thread network by MLE Routing TLV
        # which is in fact MLE Route64 TLV. Thread specificaton v1.1. - Chapter 5.20
        network_layer.TlvType.MLE_ROUTING: create_default_mle_tlv_route64_factory()
    }


def create_default_network_tlvs_factory():
    return network_layer.NetworkLayerTlvsFactory(
        tlvs_factories=create_deafult_network_tlvs_factories())


def create_default_uri_path_based_payload_factories():
    network_layer_tlvs_factory = create_default_network_tlvs_factory()

    return {
        "/a/as": network_layer_tlvs_factory,
        "/a/aq": network_layer_tlvs_factory,
        "/a/ar": network_layer_tlvs_factory,
        "/a/ae": network_layer_tlvs_factory,
        "/a/an": network_layer_tlvs_factory,
        "/a/sd": network_layer_tlvs_factory
    }


def create_default_coap_message_factory():
    return coap.CoapMessageFactory(options_factory=coap.CoapOptionsFactory(),
                                   uri_path_based_payload_factories=create_default_uri_path_based_payload_factories(),
                                   message_id_to_uri_path_binder=coap.CoapMessageIdToUriPathBinder())


def create_default_ipv6_hop_by_hop_options_factories():
    return {
        109: ipv6.MPLOptionFactory()
    }


def create_default_ipv6_hop_by_hop_options_factory():
    return ipv6.HopByHopOptionsFactory(
        options_factories=create_default_ipv6_hop_by_hop_options_factories())


def create_default_based_on_src_dst_ports_udp_payload_factory(master_key):
    mle_message_factory = create_default_mle_message_factory(master_key)
    coap_message_factory = create_default_coap_message_factory()

    return ipv6.UdpBasedOnSrcDstPortsPayloadFactory(
        src_dst_port_based_payload_factories={
            19788: mle_message_factory,
            61631: coap_message_factory
        }
    )


def create_default_ipv6_icmp_body_factories():
    return {
        ipv6.ICMP_DESTINATION_UNREACHABLE: ipv6.ICMPv6DestinationUnreachableFactory(),
        ipv6.ICMP_ECHO_REQUEST: ipv6.ICMPv6EchoBodyFactory(),
        ipv6.ICMP_ECHO_RESPONSE: ipv6.ICMPv6EchoBodyFactory(),
        "default": ipv6.BytesPayloadFactory()
    }


def create_default_ipv6_upper_layer_factories(master_key):
    return {
        ipv6.IPV6_NEXT_HEADER_UDP: ipv6.UDPDatagramFactory(
            udp_header_factory=ipv6.UDPHeaderFactory(),
            udp_payload_factory=create_default_based_on_src_dst_ports_udp_payload_factory(master_key)
        ),
        ipv6.IPV6_NEXT_HEADER_ICMP: ipv6.ICMPv6Factory(
            body_factories=create_default_ipv6_icmp_body_factories()
        )
    }


def create_default_lowpan_extension_headers_factories():
    return {
        ipv6.IPV6_NEXT_HEADER_HOP_BY_HOP: lowpan.LowpanHopByHopFactory(
            hop_by_hop_options_factory=create_default_ipv6_hop_by_hop_options_factory()
        )
    }


def create_default_ipv6_extension_headers_factories():
    return {
        ipv6.IPV6_NEXT_HEADER_HOP_BY_HOP:  ipv6.HopByHopFactory(
            hop_by_hop_options_factory=create_default_ipv6_hop_by_hop_options_factory())
    }


def create_default_ipv6_packet_factory(master_key):
    return ipv6.IPv6PacketFactory(
        ehf=create_default_ipv6_extension_headers_factories(),
        ulpf=create_default_ipv6_upper_layer_factories(master_key)
    )


def create_default_lowpan_decompressor(context_manager):
    return lowpan.LowpanDecompressor(
        lowpan_ip_header_factory=lowpan.LowpanIpv6HeaderFactory(
            context_manager=context_manager
        ),
        lowpan_extension_headers_factory=lowpan.LowpanExtensionHeadersFactory(
            ext_headers_factories=create_default_lowpan_extension_headers_factories()
        ),
        lowpan_udp_header_factory=lowpan.LowpanUdpHeaderFactory()
    )


def create_default_thread_context_manager():
    context_manager = lowpan.ContextManager()
    context_manager[0] = lowpan.Context(MESH_LOCAL_PREFIX)

    return context_manager


def create_default_lowpan_parser(context_manager, master_key=DEFAULT_MASTER_KEY):
    return lowpan.LowpanParser(
        lowpan_mesh_header_factory=lowpan.LowpanMeshHeaderFactory(),
        lowpan_decompressor=create_default_lowpan_decompressor(context_manager),
        lowpan_fragements_buffers_manager=lowpan.LowpanFragmentsBuffersManager(),
        ipv6_packet_factory=create_default_ipv6_packet_factory(master_key)
    )


def create_default_thread_message_factory(master_key=DEFAULT_MASTER_KEY):
    context_manager = create_default_thread_context_manager()
    lowpan_parser = create_default_lowpan_parser(context_manager, master_key)

    return message.MessageFactory(lowpan_parser=lowpan_parser)


def create_default_thread_sniffer(nodeid=SNIFFER_ID):
    return sniffer.Sniffer(nodeid, create_default_thread_message_factory())


def create_default_simulator():
    if VIRTUAL_TIME:
        return simulator.VirtualTime()
    return simulator.RealTime()
