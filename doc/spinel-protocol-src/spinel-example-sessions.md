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

If the host supports using vendor-specific commands, the vendor should
be verified before using them:

* CMD_VALUE_GET:PROP_VENDOR_ID
* CMD_VALUE_IS:PROP_VENDOR_ID

Fetch the capability list so that we know what features this NCP
supports:

* CMD_VALUE_GET:PROP_CAPS
* CMD_VALUE_IS:PROP_CAPS

If the NCP supports CAP_NET_SAVE, then we go ahead and recall the network:

* CMD_NET_RECALL

## Attaching to a network

<!-- RQ -- FIXME: This example session is incomplete. -->

We make the assumption that the NCP is not currently associated
with a network.

Set the network properties, if they were not already set:

* CMD_VALUE_SET:PROP_PHY_CHAN
* CMD_VALUE_IS:PROP_PHY_CHAN

* CMD_VALUE_SET:PROP_NET_XPANID
* CMD_VALUE_IS:PROP_NET_XPANID

* CMD_VALUE_SET:PROP_MAC_15_4_PANID
* CMD_VALUE_IS:PROP_MAC_15_4_PANID

* CMD_VALUE_SET:PROP_NET_NETWORK_NAME
* CMD_VALUE_IS:PROP_NET_NETWORK_NAME

* CMD_VALUE_SET:PROP_NET_MASTER_KEY
* CMD_VALUE_IS:PROP_NET_MASTER_KEY

* CMD_VALUE_SET:PROP_NET_KEY_SEQUENCE_COUNTER
* CMD_VALUE_IS:PROP_NET_KEY_SEQUENCE_COUNTER

* CMD_VALUE_SET:PROP_NET_KEY_SWITCH_GUARDTIME
* CMD_VALUE_IS:PROP_NET_KEY_SWITCH_GUARDTIME

Bring the network interface up:

* CMD_VALUE_SET:PROP_NET_IF_UP:TRUE
* CMD_VALUE_IS:PROP_NET_IF_UP:TRUE

Bring the routing stack up:

* CMD_VALUE_SET:PROP_NET_STACK_UP:TRUE
* CMD_VALUE_IS:PROP_NET_STACK_UP:TRUE

Some asynchronous events from the NCP:

* CMD_VALUE_IS:PROP_NET_ROLE
* CMD_VALUE_IS:PROP_NET_PARTITION_ID
* CMD_VALUE_IS:PROP_THREAD_ON_MESH_NETS

## Successfully joining a pre-existing network

<!-- RQ -- FIXME: This example session is incomplete. -->

This example session is identical to the above session up to the point
where we set PROP_NET_IF_UP to true. From there, the behavior changes.

* CMD_VALUE_SET:PROP_NET_REQUIRE_JOIN_EXISTING:TRUE
* CMD_VALUE_IS:PROP_NET_REQUIRE_JOIN_EXISTING:TRUE

Bring the routing stack up:

* CMD_VALUE_SET:PROP_NET_STACK_UP:TRUE
* CMD_VALUE_IS:PROP_NET_STACK_UP:TRUE

Some asynchronous events from the NCP:

* CMD_VALUE_IS:PROP_NET_ROLE
* CMD_VALUE_IS:PROP_NET_PARTITION_ID
* CMD_VALUE_IS:PROP_THREAD_ON_MESH_NETS

Now let's save the network settings to NVRAM:

* CMD_NET_SAVE

## Unsuccessfully joining a pre-existing network

This example session is identical to the above session up to the point
where we set PROP_NET_IF_UP to true. From there, the behavior changes.

* CMD_VALUE_SET:PROP_NET_REQUIRE_JOIN_EXISTING:TRUE
* CMD_VALUE_IS:PROP_NET_REQUIRE_JOIN_EXISTING:TRUE

Bring the routing stack up:

* CMD_VALUE_SET:PROP_NET_STACK_UP:TRUE
* CMD_VALUE_IS:PROP_NET_STACK_UP:TRUE

Some asynchronous events from the NCP:

* CMD_VALUE_IS:PROP_LAST_STATUS:STATUS_JOIN_NO_PEERS
* CMD_VALUE_IS:PROP_NET_STACK_UP:FALSE

## Detaching from a network

TBD

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

Some asynchronous events from the NCP:

* CMD_VALUE_IS:PROP_NET_ROLE
* CMD_VALUE_IS:PROP_NET_PARTITION_ID
* CMD_VALUE_IS:PROP_THREAD_ON_MESH_NETS

## NCP Software Reset

<!-- RQ -- FIXME: This example session is incomplete. -->

* CMD_RESET
* CMD_VALUE_IS:PROP_LAST_STATUS:STATUS_RESET_SOFTWARE

Then jump to (#ncp-initialization).

## Adding an on-mesh prefix

TBD

## Entering low-power modes

TBD

## Sniffing raw packets

<!-- RQ -- FIXME: This example session is incomplete. -->

This assumes that the NCP has been initialized.

Optionally set the channel:

* CMD_VALUE_SET:PROP_PHY_CHAN:x
* CMD_VALUE_IS:PROP_PHY_CHAN

Set the filter mode:

* CMD_VALUE_SET:PROP_MAC_PROMISCUOUS_MODE:MAC_PROMISCUOUS_MODE_MONITOR
* CMD_VALUE_IS:PROP_MAC_PROMISCUOUS_MODE:MAC_PROMISCUOUS_MODE_MONITOR

Enable the raw stream:

* CMD_VALUE_SET:PROP_MAC_RAW_STREAM_ENABLED:TRUE
* CMD_VALUE_IS:PROP_MAC_RAW_STREAM_ENABLED:TRUE

Enable the PHY directly:

* CMD_VALUE_SET:PROP_PHY_ENABLED:TRUE
* CMD_VALUE_IS:PROP_PHY_ENABLED:TRUE

Now we will get raw 802.15.4 packets asynchronously on
PROP_STREAM_RAW:

* CMD_VALUE_IS:PROP_STREAM_RAW:...
* CMD_VALUE_IS:PROP_STREAM_RAW:...
* CMD_VALUE_IS:PROP_STREAM_RAW:...

This mode may be entered even when associated with a network.
In that case, you should set `PROP_MAC_PROMISCUOUS_MODE` to
`MAC_PROMISCUOUS_MODE_PROMISCUOUS` or `MAC_PROMISCUOUS_MODE_NORMAL`, so that
you can avoid receiving packets from other networks or that are destined
for other nodes.

