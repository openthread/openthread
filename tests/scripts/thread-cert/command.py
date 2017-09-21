import ipv6

import network_layer
import config

def check_address_notification(command_msg, source_node, destination_node):
    """Verify source_node sent a properly formatted Address Notification command message to destination_node.
    """
    command_msg.assertCoapMessageRequestUriPath('/a/an')
    command_msg.assertCoapMessageContainsTlv(network_layer.TargetEid)
    command_msg.assertCoapMessageContainsTlv(network_layer.Rloc16)
    command_msg.assertCoapMessageContainsTlv(network_layer.MlEid)

    source_rloc = source_node.get_ip6_address(config.ADDRESS_TYPE.RLOC)
    assert ipv6.ip_address(source_rloc) == command_msg.ipv6_packet.ipv6_header.source_address, "Error: The IPv6 Source address is not the RLOC of the originator."

    destination_rloc = destination_node.get_ip6_address(config.ADDRESS_TYPE.RLOC)
    assert ipv6.ip_address(destination_rloc) == command_msg.ipv6_packet.ipv6_header.destination_address, "Error: The IPv6 Destination address is not the RLOC of the destination."

def check_address_release(command_msg, destination_node):
    """Verify the message is a properly formatted address release destined to the given node.
    """
    command_msg.assertCoapMessageRequestUriPath('/a/ar')
    command_msg.assertCoapMessageContainsTlv(network_layer.Rloc16)
    command_msg.assertCoapMessageContainsTlv(network_layer.MacExtendedAddress)

    destination_rloc = destination_node.get_ip6_address(config.ADDRESS_TYPE.RLOC)
    assert ipv6.ip_address(destination_rloc) == command_msg.ipv6_packet.ipv6_header.destination_address, "Error: The Destination is not RLOC address"
