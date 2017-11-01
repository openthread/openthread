import ipv6

import network_layer
import config
import mle

def check_address_query(command_msg, source_node, destination_address):
    """Verify source_node sent a properly formatted Address Query Request message to the destination_address.
    """
    command_msg.assertCoapMessageContainsTlv(network_layer.TargetEid)

    source_rloc = source_node.get_ip6_address(config.ADDRESS_TYPE.RLOC)
    assert ipv6.ip_address(source_rloc) == command_msg.ipv6_packet.ipv6_header.source_address, \
            "Error: The IPv6 source address is not the RLOC of the originator. The source node's rloc is: " \
            + str(ipv6.ip_address(source_rloc)) + ", but the source_rloc in command msg is: " \
            + str(command_msg.ipv6_packet.ipv6_header.source_address)

    assert ipv6.ip_address(destination_address.decode('utf-8')) == command_msg.ipv6_packet.ipv6_header.destination_address, "Error: The IPv6 destination address is not expected."

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

def check_address_release(command_msg, destination_node):
    """Verify the message is a properly formatted address release destined to the given node.
    """
    command_msg.assertCoapMessageRequestUriPath('/a/ar')
    command_msg.assertCoapMessageContainsTlv(network_layer.Rloc16)
    command_msg.assertCoapMessageContainsTlv(network_layer.MacExtendedAddress)

    destination_rloc = destination_node.get_ip6_address(config.ADDRESS_TYPE.RLOC)
    assert ipv6.ip_address(destination_rloc) == command_msg.ipv6_packet.ipv6_header.destination_address, "Error: The destination is not RLOC address"

def check_link_request(command_msg):
    """Verify a properly formatted Link Request command message.
    """
    command_msg.assertMleMessageContainsTlv(mle.Challenge)
    command_msg.assertMleMessageContainsTlv(mle.LeaderData)
    command_msg.assertMleMessageContainsTlv(mle.SourceAddress)
    command_msg.assertMleMessageContainsTlv(mle.Version)

def check_link_accept(command_msg, destination_node):
    """Verify a properly formatted Link Accept command message.
    """
    command_msg.assertMleMessageContainsTlv(mle.LinkLayerFrameCounter)
    command_msg.assertMleMessageContainsTlv(mle.SourceAddress)
    command_msg.assertMleMessageContainsTlv(mle.Response)
    command_msg.assertMleMessageContainsTlv(mle.Version)
    command_msg.assertMleMessageContainsOptionalTlv(mle.MleFrameCounter)

    destination_link_local = destination_node.get_ip6_address(config.ADDRESS_TYPE.LINK_LOCAL)
    assert ipv6.ip_address(destination_link_local) == command_msg.ipv6_packet.ipv6_header.destination_address, "Error: The destination is unexpected"

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
