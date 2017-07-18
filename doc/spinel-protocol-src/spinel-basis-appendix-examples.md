# Example Sessions

## NCP Initialization

<!-- RQ -- FIXME: This example session is incomplete. -->

Check the protocol version to see if it is supported:

* CMD_VALUE_GET:PROP_PROTOCOL_VERSION
* CMD_VALUE_IS:PROP_PROTOCOL_VERSION

Check the NCP version to see if a firmware update may be necessary:

* CMD_VALUE_GET:PROP_NCP_VERSION
* CMD_VALUE_IS:PROP_NCP_VERSION

Check interface type to make sure that it is what we expect:

* CMD_VALUE_GET:PROP_INTERFACE_TYPE
* CMD_VALUE_IS:PROP_INTERFACE_TYPE

If the OS supports using vendor-specific commands, the vendor should be verified before using them:

* CMD_VALUE_GET:PROP_VENDOR_ID
* CMD_VALUE_IS:PROP_VENDOR_ID

Fetch the capability list so that we know what features this NCP supports:

* CMD_VALUE_GET:PROP_CAPS
* CMD_VALUE_IS:PROP_CAPS

If the NCP supports CAP_NET_SAVE, then we go ahead and recall the network:

* CMD_NET_RECALL

## Attaching to a network

<!-- RQ -- FIXME: This example session is incomplete. -->

We make the assumption that the NCP is already associated with a network at physical and media access layers. The basis layer steps proceed after the initial phase of initializing the specific network layer stack.

Bring the network interface up:

* CMD_VALUE_SET:PROP_NET_IF_UP:TRUE
* CMD_VALUE_IS:PROP_NET_IF_UP:TRUE

Bring the routing stack up:

* CMD_VALUE_SET:PROP_NET_STACK_UP:TRUE
* CMD_VALUE_IS:PROP_NET_STACK_UP:TRUE

## Detaching from a network

This is the reverse of the previous case.

Bring the routing stack up:

* CMD_VALUE_SET:PROP_NET_STACK_UP:FALSE
* CMD_VALUE_IS:PROP_NET_STACK_UP:FALSE

Bring the network interface up:

* CMD_VALUE_SET:PROP_NET_IF_UP:FALSE
* CMD_VALUE_IS:PROP_NET_IF_UP:FALSE

## Attaching to a saved network

<!-- RQ -- FIXME: This example session is incomplete. -->

Recall the saved network if you haven't already done so:

* CMD_NET_RECALL

Bring the network interface up:

* CMD_VALUE_SET:PROP_NET_IF_UP:TRUE
* CMD_VALUE_IS:PROP_NET_IF_UP:TRUE

Bring the routing stack up:

* CMD_VALUE_SET:PROP_NET_STACK_UP:TRUE
* CMD_VALUE_IS:PROP_NET_STACK_UP:TRUE


## NCP Software Reset

<!-- RQ -- FIXME: This example session is incomplete. -->

* CMD_RESET
* CMD_VALUE_IS:PROP_LAST_STATUS:STATUS_RESET_SOFTWARE

Then jump to (#ncp-initialization).
