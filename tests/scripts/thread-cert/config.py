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

import ipv6
import lowpan
import message
import mle
import net_crypto
import network_data
import sniffer


DEFAULT_MASTER_KEY = bytearray([0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77,
                                0x88, 0x99, 0xaa, 0xbb, 0xcc, 0xdd, 0xee, 0xff])


def create_default_network_data_prefix_sub_tlvs_factories():
    return {
        0: network_data.HasRouteFactory(
            routes_factory=network_data.RoutesFactory(
                route_factory=network_data.RouteFactory())
        ),
        2: network_data.BorderRouterFactory(),
        3: network_data.LowpanIdFactory()
    }


def create_default_network_data_prefix_sub_tlvs_factory():
    return network_data.PrefixSubTlvsFactory(
        sub_tlvs_factories=create_default_network_data_prefix_sub_tlvs_factories())


def create_default_network_data_service_sub_tlvs_factories():
    return {
        6: network_data.ServerFactory()
    }


def create_default_network_data_service_sub_tlvs_factory():
    return network_data.ServiceSubTlvsFactory(
        sub_tlvs_factories=create_default_network_data_service_sub_tlvs_factories())


def create_default_network_data_tlvs_factories():
    return {
        1: network_data.PrefixFactory(
            sub_tlvs_factory=create_default_network_data_prefix_sub_tlvs_factory()
        ),
        5: network_data.ServiceFactory(
            sub_tlvs_factory=create_default_network_data_service_sub_tlvs_factory()
        )
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


def create_default_ipv6_hop_by_hop_options_factories():
    return {
        109: ipv6.MPLOptionFactory()
    }


def create_default_ipv6_hop_by_hop_options_factory():
    return ipv6.HopByHopOptionsFactory(
        options_factories=create_default_ipv6_hop_by_hop_options_factories())


def create_default_ipv6_udp_dst_port_factories(master_key):
    mle_message_factory = create_default_mle_message_factory(master_key)

    return {
        19788: mle_message_factory,

        # TODO: Improve CoAP support
        61631: ipv6.UDPBytesPayloadFactory(),
        49152: ipv6.UDPBytesPayloadFactory(),
        49153: ipv6.UDPBytesPayloadFactory(),
        49154: ipv6.UDPBytesPayloadFactory()
    }


def create_default_ipv6_icmp_body_factories():
    return {
        0: ipv6.ICMPv6DestinationUnreachableFactory(),
        128: ipv6.ICMPv6EchoBodyFactory(),
        129: ipv6.ICMPv6EchoBodyFactory()
    }


def create_default_ipv6_upper_layer_factories(master_key):
    return {
        17: ipv6.UDPDatagramFactory(
            udp_header_factory=ipv6.UDPHeaderFactory(),
            dst_port_factories=create_default_ipv6_udp_dst_port_factories(master_key)
        ),
        58: ipv6.ICMPv6Factory(
            body_factories=create_default_ipv6_icmp_body_factories()
        )
    }


def create_default_lowpan_extension_headers_factories():
    return {
        0: lowpan.LowpanHopByHopFactory(
            hop_by_hop_options_factory=create_default_ipv6_hop_by_hop_options_factory()
        )
    }


def create_default_ipv6_extension_headers_factories():
    return {
        0:  ipv6.HopByHopFactory(
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
    context_manager[0] = lowpan.Context("fd00:0db8::/64")

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


def create_default_thread_sniffer(nodeid):
    return sniffer.Sniffer(nodeid, create_default_thread_message_factory())
