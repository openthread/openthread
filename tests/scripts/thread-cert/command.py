#!/usr/bin/env python
#
#  Copyright (c) 2017-2018, The OpenThread Authors.
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

import sys

import ipv6
from network_data import Prefix, BorderRouter, LowpanId
import network_layer
import config
import mle

from enum import IntEnum

class CheckType(IntEnum):
    CONTAIN = 0
    NOT_CONTAIN = 1
    OPTIONAL = 2

def check_address_query(command_msg, source_node, destination_address):
    """Verify source_node sent a properly formatted Address Query Request message to the destination_address.
    """
    command_msg.assertCoapMessageContainsTlv(network_layer.TargetEid)

    source_rloc = source_node.get_ip6_address(config.ADDRESS_TYPE.RLOC)
    assert ipv6.ip_address(source_rloc) == command_msg.ipv6_packet.ipv6_header.source_address, \
            "Error: The IPv6 source address is not the RLOC of the originator. The source node's rloc is: " \
            + str(ipv6.ip_address(source_rloc)) + ", but the source_address in command msg is: " \
            + str(command_msg.ipv6_packet.ipv6_header.source_address)

    if isinstance(destination_address, bytearray):
        destination_address = bytes(destination_address)
    elif isinstance(destination_address, str) and sys.version_info[0] == 2:
        destination_address = destination_address.decode("utf-8")

    assert ipv6.ip_address(destination_address) == command_msg.ipv6_packet.ipv6_header.destination_address, "Error: The IPv6 destination address is not expected."

def check_address_notification(command_msg, source_node, destination_node):
    """Verify source_node sent a properly formatted Address Notification command message to destination_node.
    """
    command_msg.assertCoapMessageRequestUriPath('/a/an')
    command_msg.assertCoapMessageContainsTlv(network_layer.TargetEid)
    command_msg.assertCoapMessageContainsTlv(network_layer.Rloc16)
    command_msg.assertCoapMessageContainsTlv(network_layer.MlEid)

    source_rloc = source_node.get_ip6_address(config.ADDRESS_TYPE.RLOC)
    assert ipv6.ip_address(source_rloc) == command_msg.ipv6_packet.ipv6_header.source_address, "Error: The IPv6 source address is not the RLOC of the originator."

    destination_rloc = destination_node.get_ip6_address(config.ADDRESS_TYPE.RLOC)
    assert ipv6.ip_address(destination_rloc) == command_msg.ipv6_packet.ipv6_header.destination_address, "Error: The IPv6 destination address is not the RLOC of the destination."

def check_address_error_notification(command_msg, source_node, destination_address):
    """Verify source_node sent a properly formatted Address Error Notification command message to destination_address.
    """
    command_msg.assertCoapMessageRequestUriPath('/a/ae')
    command_msg.assertCoapMessageContainsTlv(network_layer.TargetEid)
    command_msg.assertCoapMessageContainsTlv(network_layer.MlEid)

    source_rloc = source_node.get_ip6_address(config.ADDRESS_TYPE.RLOC)
    assert ipv6.ip_address(source_rloc) == command_msg.ipv6_packet.ipv6_header.source_address, \
            "Error: The IPv6 source address is not the RLOC of the originator. The source node's rloc is: " \
            + str(ipv6.ip_address(source_rloc)) + ", but the source_address in command msg is: " \
            + str(command_msg.ipv6_packet.ipv6_header.source_address)

    if isinstance(destination_address, bytearray):
        destination_address = bytes(destination_address)
    elif isinstance(destination_address, str) and sys.version_info[0] == 2:
        destination_address = destination_address.decode("utf-8")

    assert ipv6.ip_address(destination_address) == command_msg.ipv6_packet.ipv6_header.destination_address, \
            "Error: The IPv6 destination address is not expected. The destination node's rloc is: " \
            + str(ipv6.ip_address(destination_address)) + ", but the destination_address in command msg is: " \
            + str(command_msg.ipv6_packet.ipv6_header.destination_address)

def check_address_solicit(command_msg, was_router):
    command_msg.assertCoapMessageRequestUriPath('/a/as')
    command_msg.assertCoapMessageContainsTlv(network_layer.MacExtendedAddress)
    command_msg.assertCoapMessageContainsTlv(network_layer.Status)
    if was_router:
        command_msg.assertCoapMessageContainsTlv(network_layer.Rloc16)
    else:
        command_msg.assertMleMessageDoesNotContainTlv(network_layer.Rloc16)

def check_address_release(command_msg, destination_node):
    """Verify the message is a properly formatted address release destined to the given node.
    """
    command_msg.assertCoapMessageRequestUriPath('/a/ar')
    command_msg.assertCoapMessageContainsTlv(network_layer.Rloc16)
    command_msg.assertCoapMessageContainsTlv(network_layer.MacExtendedAddress)

    destination_rloc = destination_node.get_ip6_address(config.ADDRESS_TYPE.RLOC)
    assert ipv6.ip_address(destination_rloc) == command_msg.ipv6_packet.ipv6_header.destination_address, "Error: The destination is not RLOC address"

def check_tlv_request_tlv(command_msg, check_type, tlv_id):
    """Verify if TLV Request TLV contains specified TLV ID
    """
    tlv_request_tlv = command_msg.get_mle_message_tlv(mle.TlvRequest)

    if check_type == CheckType.CONTAIN:
        assert tlv_request_tlv is not None, "Error: The msg doesn't contain TLV Request TLV"
        assert any(tlv_id == tlv for tlv in tlv_request_tlv.tlvs), "Error: The msg doesn't contain TLV Request TLV ID: {}".format(tlv_id)

    elif check_type == CheckType.NOT_CONTAIN:
        if tlv_request_tlv is not None:
            assert any(tlv_id == tlv for tlv in tlv_request_tlv.tlvs) is False, "Error: The msg contains TLV Request TLV ID: {}".format(tlv_id)

    elif check_type == CheckType.OPTIONAL:
        if tlv_request_tlv is not None:
            if any(tlv_id == tlv for tlv in tlv_request_tlv.tlvs):
                print("TLV Request TLV contains TLV ID: {}".format(tlv_id))
            else:
                print("TLV Request TLV doesn't contain TLV ID: {}".format(tlv_id))
        else:
            print("The msg doesn't contain TLV Request TLV")

    else:
        raise ValueError("Invalid check type")

def check_link_request(command_msg, source_address = CheckType.OPTIONAL, leader_data = CheckType.OPTIONAL, \
    tlv_request_address16 = CheckType.OPTIONAL, tlv_request_route64 = CheckType.OPTIONAL, \
    tlv_request_link_margin = CheckType.OPTIONAL):

    """Verify a properly formatted Link Request command message.
    """
    command_msg.assertMleMessageContainsTlv(mle.Challenge)
    command_msg.assertMleMessageContainsTlv(mle.Version)

    check_mle_optional_tlv(command_msg, source_address, mle.SourceAddress)
    check_mle_optional_tlv(command_msg, leader_data, mle.LeaderData)

    check_tlv_request_tlv(command_msg, tlv_request_address16, mle.TlvType.ADDRESS16)
    check_tlv_request_tlv(command_msg, tlv_request_route64, mle.TlvType.ROUTE64)
    check_tlv_request_tlv(command_msg, tlv_request_link_margin, mle.TlvType.LINK_MARGIN)

def check_link_accept(command_msg, destination_node, \
    leader_data = CheckType.OPTIONAL, link_margin = CheckType.OPTIONAL, mle_frame_counter = CheckType.OPTIONAL, \
    challenge = CheckType.OPTIONAL, address16 = CheckType.OPTIONAL, route64 = CheckType.OPTIONAL, \
    tlv_request_link_margin = CheckType.OPTIONAL):
    """verify a properly formatted link accept command message.
    """
    command_msg.assertMleMessageContainsTlv(mle.LinkLayerFrameCounter)
    command_msg.assertMleMessageContainsTlv(mle.SourceAddress)
    command_msg.assertMleMessageContainsTlv(mle.Response)
    command_msg.assertMleMessageContainsTlv(mle.Version)

    check_mle_optional_tlv(command_msg, leader_data, mle.LeaderData)
    check_mle_optional_tlv(command_msg, link_margin, mle.LinkMargin)
    check_mle_optional_tlv(command_msg, mle_frame_counter, mle.MleFrameCounter)
    check_mle_optional_tlv(command_msg, challenge, mle.Challenge)
    check_mle_optional_tlv(command_msg, address16, mle.Address16)
    check_mle_optional_tlv(command_msg, route64, mle.Route64)

    check_tlv_request_tlv(command_msg, tlv_request_link_margin, mle.TlvType.LINK_MARGIN)

    destination_link_local = destination_node.get_ip6_address(config.ADDRESS_TYPE.LINK_LOCAL)
    assert ipv6.ip_address(destination_link_local) == command_msg.ipv6_packet.ipv6_header.destination_address, \
        "Error: The destination is unexpected"

def check_icmp_path(sniffer, path, nodes, icmp_type = ipv6.ICMP_ECHO_REQUEST):
    """Verify icmp message is forwarded along the path.
    """
    len_path = len(path)

    # Verify icmp message is forwarded to the next node of the path.
    for i in range(0, len_path):
        node_msg = sniffer.get_messages_sent_by(path[i])
        node_icmp_msg = node_msg.get_icmp_message(icmp_type)

        if i < len_path - 1:
            next_node = nodes[path[i + 1]]
            next_node_rloc16 = next_node.get_addr16()
            assert next_node_rloc16 == node_icmp_msg.mac_header.dest_address.rloc, "Error: The path is unexpected."
        else:
            return True

    return False

def check_id_set(command_msg, router_id):
    """Check the command_msg's Route64 tlv to verify router_id is an active router.
    """
    tlv = command_msg.assertMleMessageContainsTlv(mle.Route64)
    return ((tlv.router_id_mask >> (63 - router_id)) & 1)

def get_routing_cost(command_msg, router_id):
    """Check the command_msg's Route64 tlv to get the routing cost to router.
    """
    tlv = command_msg.assertMleMessageContainsTlv(mle.Route64)

    # Get router's mask pos
    # Turn the number into binary string. Need to consider the preceding 0 omitted during conversion.
    router_id_mask_str = bin(tlv.router_id_mask).replace('0b','')
    prefix_len = 64 - len(router_id_mask_str)
    routing_entry_pos = 0

    for i in range(0, router_id - prefix_len):
        if router_id_mask_str[i] == '1':
            routing_entry_pos += 1

    assert router_id_mask_str[router_id - prefix_len] == '1', "Error: The router isn't in the topology. \n" \
            + "route64 tlv is: %s. \nrouter_id is: %s. \nrouting_entry_pos is: %s. \nrouter_id_mask_str is: %s." \
            %(tlv, router_id, routing_entry_pos, router_id_mask_str)

    return tlv.link_quality_and_route_data[routing_entry_pos].route

def check_mle_optional_tlv(command_msg, type, tlv):
    if (type == CheckType.CONTAIN):
        command_msg.assertMleMessageContainsTlv(tlv)
    elif (type == CheckType.NOT_CONTAIN):
        command_msg.assertMleMessageDoesNotContainTlv(tlv)
    elif (type == CheckType.OPTIONAL):
        command_msg.assertMleMessageContainsOptionalTlv(tlv)
    else:
        raise ValueError("Invalid check type")

def check_mle_advertisement(command_msg):
    command_msg.assertSentWithHopLimit(255)
    command_msg.assertSentToDestinationAddress(config.LINK_LOCAL_ALL_NODES_ADDRESS)
    command_msg.assertMleMessageContainsTlv(mle.SourceAddress)
    command_msg.assertMleMessageContainsTlv(mle.LeaderData)
    command_msg.assertMleMessageContainsTlv(mle.Route64)

def check_parent_request(command_msg, is_first_request):
    """Verify a properly formatted Parent Request command message.
    """
    if command_msg.mle.aux_sec_hdr.key_id_mode != 0x2:
        raise ValueError("The Key Identifier Mode of the Security Control Field SHALL be set to 0x02")

    command_msg.assertSentWithHopLimit(255)
    command_msg.assertSentToDestinationAddress(config.LINK_LOCAL_ALL_ROUTERS_ADDRESS)
    command_msg.assertMleMessageContainsTlv(mle.Mode)
    command_msg.assertMleMessageContainsTlv(mle.Challenge)
    command_msg.assertMleMessageContainsTlv(mle.Version)
    scan_mask = command_msg.assertMleMessageContainsTlv(mle.ScanMask)
    if not scan_mask.router:
        raise ValueError("Parent request without R bit set")
    if is_first_request:
        if scan_mask.end_device:
            raise ValueError("First parent request with E bit set")
    elif not scan_mask.end_device:
        raise ValueError("Second parent request without E bit set")

def check_parent_response(command_msg, mle_frame_counter = CheckType.OPTIONAL):
    """Verify a properly formatted Parent Response command message.
    """
    command_msg.assertMleMessageContainsTlv(mle.Challenge)
    command_msg.assertMleMessageContainsTlv(mle.Connectivity)
    command_msg.assertMleMessageContainsTlv(mle.LeaderData)
    command_msg.assertMleMessageContainsTlv(mle.LinkLayerFrameCounter)
    command_msg.assertMleMessageContainsTlv(mle.LinkMargin)
    command_msg.assertMleMessageContainsTlv(mle.Response)
    command_msg.assertMleMessageContainsTlv(mle.SourceAddress)
    command_msg.assertMleMessageContainsTlv(mle.Version)

    check_mle_optional_tlv(command_msg, mle_frame_counter, mle.MleFrameCounter)

def check_child_id_request(command_msg, tlv_request = CheckType.OPTIONAL, \
    mle_frame_counter = CheckType.OPTIONAL, address_registration = CheckType.OPTIONAL, \
    active_timestamp = CheckType.OPTIONAL, pending_timestamp = CheckType.OPTIONAL,
    route64 = CheckType.OPTIONAL):
    """Verify a properly formatted Child Id Request command message.
    """
    if command_msg.mle.aux_sec_hdr.key_id_mode != 0x2:
        raise ValueError("The Key Identifier Mode of the Security Control Field SHALL be set to 0x02")

    command_msg.assertMleMessageContainsTlv(mle.LinkLayerFrameCounter)
    command_msg.assertMleMessageContainsTlv(mle.Mode)
    command_msg.assertMleMessageContainsTlv(mle.Response)
    command_msg.assertMleMessageContainsTlv(mle.Timeout)
    command_msg.assertMleMessageContainsTlv(mle.Version)

    check_mle_optional_tlv(command_msg, tlv_request, mle.TlvRequest)
    check_mle_optional_tlv(command_msg, mle_frame_counter, mle.MleFrameCounter)
    check_mle_optional_tlv(command_msg, address_registration, mle.AddressRegistration)
    check_mle_optional_tlv(command_msg, active_timestamp, mle.ActiveTimestamp)
    check_mle_optional_tlv(command_msg, pending_timestamp, mle.PendingTimestamp)
    check_mle_optional_tlv(command_msg, route64, mle.Route64)

    check_tlv_request_tlv(command_msg, CheckType.CONTAIN, mle.TlvType.ADDRESS16)
    check_tlv_request_tlv(command_msg, CheckType.CONTAIN, mle.TlvType.NETWORK_DATA)

def check_child_id_response(command_msg, route64 = CheckType.OPTIONAL, network_data = CheckType.OPTIONAL, \
    address_registration = CheckType.OPTIONAL, active_timestamp = CheckType.OPTIONAL, \
    pending_timestamp = CheckType.OPTIONAL, active_operational_dataset = CheckType.OPTIONAL, \
    pending_operational_dataset = CheckType.OPTIONAL):
    """Verify a properly formatted Child Id Response command message.
    """
    command_msg.assertMleMessageContainsTlv(mle.SourceAddress)
    command_msg.assertMleMessageContainsTlv(mle.LeaderData)
    command_msg.assertMleMessageContainsTlv(mle.Address16)

    check_mle_optional_tlv(command_msg, route64, mle.Route64)
    check_mle_optional_tlv(command_msg, network_data, mle.NetworkData)
    check_mle_optional_tlv(command_msg, address_registration, mle.AddressRegistration)
    check_mle_optional_tlv(command_msg, active_timestamp, mle.ActiveTimestamp)
    check_mle_optional_tlv(command_msg, pending_timestamp, mle.PendingTimestamp)
    check_mle_optional_tlv(command_msg, active_operational_dataset, mle.ActiveOperationalDataset)
    check_mle_optional_tlv(command_msg, pending_operational_dataset, mle.PendingOperationalDataset)

def check_child_update_request_by_child(command_msg):
    command_msg.assertMleMessageContainsTlv(mle.LeaderData)
    command_msg.assertMleMessageContainsTlv(mle.Mode)
    command_msg.assertMleMessageContainsTlv(mle.SourceAddress)

def check_coap_optional_tlv(coap_msg, type, tlv):
    if (type == CheckType.CONTAIN):
        coap_msg.assertCoapMessageContainsTlv(tlv)
    elif (type == CheckType.NOT_CONTAIN):
        coap_msg.assertCoapMessageDoesNotContainTlv(tlv)
    elif (type == CheckType.OPTIONAL):
        coap_msg.assertCoapMessageContainsOptionalTlv(tlv)
    else:
        raise ValueError("Invalid check type")

def check_router_id_cached(node, router_id, cached = True):
    """Verify if the node has cached any entries based on the router ID
    """
    eidcaches = node.get_eidcaches()
    if cached:
        assert any(router_id == (int(rloc, 16) >> 10) for (_, rloc) in eidcaches)
    else:
        assert any(router_id == (int(rloc, 16) >> 10) for (_, rloc) in eidcaches) is False

def contains_tlv(sub_tlvs, tlv_type):
    """Verify if a specific type of tlv is included in a sub-tlv list.
    """
    return any(isinstance(sub_tlv, tlv_type) for sub_tlv in sub_tlvs)

def check_secure_mle_key_id_mode(command_msg, key_id_mode):
    """Verify if the mle command message sets the right key id mode.
    """
    assert isinstance(command_msg.mle, mle.MleMessageSecured)
    assert command_msg.mle.aux_sec_hdr.key_id_mode == key_id_mode

def check_data_response(command_msg, network_data=CheckType.OPTIONAL, prefixes=[],
    active_timestamp=CheckType.OPTIONAL):
    """Verify a properly formatted Data Response command message.
    """
    check_secure_mle_key_id_mode(command_msg, 0x02)

    command_msg.assertMleMessageContainsTlv(mle.SourceAddress)
    command_msg.assertMleMessageContainsTlv(mle.LeaderData)
    check_mle_optional_tlv(command_msg, network_data, mle.NetworkData)
    check_mle_optional_tlv(command_msg, active_timestamp, mle.ActiveTimestamp)

    if network_data == CheckType.CONTAIN and len(prefixes) > 0:
        network_data_tlv = command_msg.get_mle_message_tlv(mle.NetworkData)
        prefix_tlvs = [tlv for tlv in network_data_tlv.tlvs if isinstance(tlv, Prefix)]
        assert len(prefix_tlvs) >= len(prefixes)
        for prefix in prefix_tlvs:
            assert contains_tlv(prefix.sub_tlvs, BorderRouter)
            assert contains_tlv(prefix.sub_tlvs, LowpanId)

def check_child_update_request_from_parent(command_msg, leader_data=CheckType.OPTIONAL,
    network_data=CheckType.OPTIONAL, challenge=CheckType.OPTIONAL,
    tlv_request=CheckType.OPTIONAL, active_timestamp=CheckType.OPTIONAL):
    """Verify a properly formatted Child Update Request(from parent) command message.
    """
    check_secure_mle_key_id_mode(command_msg, 0x02)

    command_msg.assertMleMessageContainsTlv(mle.SourceAddress)
    check_mle_optional_tlv(command_msg, leader_data, mle.LeaderData)
    check_mle_optional_tlv(command_msg, network_data, mle.NetworkData)
    check_mle_optional_tlv(command_msg, challenge, mle.Challenge)
    check_mle_optional_tlv(command_msg, tlv_request, mle.TlvRequest)
    check_mle_optional_tlv(command_msg, active_timestamp, mle.ActiveTimestamp)

def check_child_update_response_from_parent(command_msg, timeout=CheckType.OPTIONAL,
    address_registration=CheckType.OPTIONAL, address16=CheckType.OPTIONAL,
    leader_data=CheckType.OPTIONAL, network_data=CheckType.OPTIONAL, response=CheckType.OPTIONAL,
    link_layer_frame_counter=CheckType.OPTIONAL, mle_frame_counter=CheckType.OPTIONAL):
    """Verify a properly formatted Child Update Response from parent
    """
    check_secure_mle_key_id_mode(command_msg, 0x02)

    command_msg.assertMleMessageContainsTlv(mle.SourceAddress)
    command_msg.assertMleMessageContainsTlv(mle.Mode)
    check_mle_optional_tlv(command_msg, timeout, mle.Timeout)
    check_mle_optional_tlv(command_msg, address_registration, mle.AddressRegistration)
    check_mle_optional_tlv(command_msg, address16, mle.Address16)
    check_mle_optional_tlv(command_msg, leader_data, mle.LeaderData)
    check_mle_optional_tlv(command_msg, network_data, mle.NetworkData)
    check_mle_optional_tlv(command_msg, response, mle.Response)
    check_mle_optional_tlv(command_msg, link_layer_frame_counter, mle.LinkLayerFrameCounter)
    check_mle_optional_tlv(command_msg, mle_frame_counter, mle.MleFrameCounter)

def get_sub_tlv(tlvs, tlv_type):
    for sub_tlv in tlvs:
        if isinstance(sub_tlv, tlv_type):
            return sub_tlv
