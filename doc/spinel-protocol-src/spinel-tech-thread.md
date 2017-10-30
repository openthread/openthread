# Technology: Thread(R) {#tech-thread}

This section describes all of the properties and semantics required
for managing a Thread(R) NCP.

Thread(R) NCPs have the following requirements:

* The property `PROP_INTERFACE_TYPE` must be 3.
* The non-optional properties in the following sections **MUST** be
  implemented: CORE, PHY, MAC, NET, and IPV6.

All serious implementations of an NCP **SHOULD** also support the network
save feature (See (#feature-network-save)).

## Capabilities {#thread-caps}

The Thread(R) technology defines the following capabilities:

* `CAP_NET_THREAD_1_0` - Indicates that the NCP implements v1.0 of the Thread(R) standard.
* `CAP_NET_THREAD_1_1` - Indicates that the NCP implements v1.1 of the Thread(R) standard.

## Properties {#thread-properties}

Properties for Thread(R) are allocated out of the `Tech` property
section (see (#property-sections)).

### PROP 80: PROP_THREAD_LEADER_ADDR
* Type: Read-Only
* Packed-Encoding: `6`

The IPv6 address of the leader. (Note: May change to long and short address of leader)

### PROP 81: PROP_THREAD_PARENT
* Type: Read-Only
* Packed-Encoding: `ES`
* LADDR, SADDR

The long address and short address of the parent of this node.

### PROP 82: PROP_THREAD_CHILD_TABLE
* Type: Read-Only
* Packed-Encoding: `A(t(ES))`

Table containing the long and short addresses of all
the children of this node.

### PROP 83: PROP_THREAD_LEADER_RID
* Type: Read-Only
* Packed-Encoding: `C`

The router-id of the current leader.

### PROP 84: PROP_THREAD_LEADER_WEIGHT
* Type: Read-Only
* Packed-Encoding: `C`

The leader weight of the current leader.

### PROP 85: PROP_THREAD_LOCAL_LEADER_WEIGHT
* Type: Read-Write
* Packed-Encoding: `C`

The leader weight for this node.

### PROP 86: PROP_THREAD_NETWORK_DATA
* Type: Read-Only
* Packed-Encoding: `D`

The local network data.

### PROP 87: PROP_THREAD_NETWORK_DATA_VERSION
* Type: Read-Only
* Packed-Encoding: `S`

### PROP 88: PROP_THREAD_STABLE_NETWORK_DATA
* Type: Read-Only
* Packed-Encoding: `D`

The local stable network data.

### PROP 89: PROP_THREAD_STABLE_NETWORK_DATA_VERSION
* Type: Read-Only
* Packed-Encoding: `S`

### PROP 90: PROP_THREAD_ON_MESH_NETS
* Type: Read-Write
* Packed-Encoding: `A(t(6CbCb))`

Data per item is:

* `6`: IPv6 Prefix
* `C`: Prefix length in bits
* `b`: Stable flag
* `C`: TLV flags
* `b`: "Is defined locally" flag. Set if this network was locally
  defined. Assumed to be true for set, insert and replace. Clear if
  the on mesh network was defined by another node.
* `S`: The RLOC16 of the device that registered this on-mesh prefix entry.
  This value is not used and ignored when adding an on-mesh prefix.

### PROP 91: PROP_THREAD_OFF_MESH_ROUTES
* Type: Read-Write
* Packed-Encoding: `A(t(6CbCbb))`

Data per item is:

* `6`: Route Prefix
* `C`: Prefix length in bits
* `b`: Stable flag
* `C`: Route preference flags
* `b`: "Is defined locally" flag. Set if this route info was locally
  defined as part of local network data. Assumed to be true for set,
  insert and replace. Clear if the route is part of partition's network
  data.
* `b`: "Next hop is this device" flag. Set if the next hop for the
  route is this device itself (i.e., route was added by this device)
  This value is ignored when adding an external route. For any added
  route the next hop is this device.
* `S`: The RLOC16 of the device that registered this route entry.
  This value is not used and ignored when adding a route.

### PROP 92: PROP_THREAD_ASSISTING_PORTS
* Type: Read-Write
* Packed-Encoding: `A(S)`

### PROP 93: PROP_THREAD_ALLOW_LOCAL_NET_DATA_CHANGE
* Type: Read-Write
* Packed-Encoding: `b`

Set to true before changing local net data. Set to false when finished.
This allows changes to be aggregated into single events.

### PROP 94: PROP_THREAD_MODE
* Type: Read-Write
* Packed-Encoding: `C`

This property contains the value of the mode
TLV for this node. The meaning of the bits in this
bitfield are defined by section 4.5.2 of the Thread(R)
specification.

### PROP 5376: PROP_THREAD_CHILD_TIMEOUT
* Type: Read-Write
* Packed-Encoding: `L`

Used when operating in the Child role.

### PROP 5377: PROP_THREAD_RLOC16
* Type: Read-Write
* Packed-Encoding: `S`

### PROP 5378: PROP_THREAD_ROUTER_UPGRADE_THRESHOLD
* Type: Read-Write
* Packed-Encoding: `C`

### PROP 5379: PROP_THREAD_CONTEXT_REUSE_DELAY
* Type: Read-Write
* Packed-Encoding: `L`

### PROP 5380: PROP_THREAD_NETWORK_ID_TIMEOUT
* Type: Read-Write
* Packed-Encoding: `C`

Allows you to get or set the Thread(R) `NETWORK_ID_TIMEOUT` constant, as
defined by the Thread(R) specification.

### PROP 5381: PROP_THREAD_ACTIVE_ROUTER_IDS
* Type: Read-Write/Write-Only
* Packed-Encoding: `A(C)` (List of active thread router ids)

Note that some implementations may not support `CMD_GET_VALUE`
router ids, but may support `CMD_REMOVE_VALUE` when the node is
a leader.

### PROP 5382: PROP_THREAD_RLOC16_DEBUG_PASSTHRU
* Type: Read-Write
* Packed-Encoding: `b`

Allow the HOST to directly observe all IPv6 packets received by the NCP,
including ones sent to the RLOC16 address.

Default value is `false`.

### PROP 5383: PROP_THREAD_ROUTER_ROLE_ENABLED
* Type: Read-Write
* Packed-Encoding: `b`

Allow the HOST to indicate whether or not the router role is enabled.
If current role is a router, setting this property to `false` starts
a re-attach process as an end-device.

### PROP 5384: PROP_THREAD_ROUTER_DOWNGRADE_THRESHOLD
* Type: Read-Write
* Packed-Encoding: `C`

### PROP 5385: PROP_THREAD_ROUTER_SELECTION_JITTER
* Type: Read-Write
* Packed-Encoding: `C`

Specifies the self imposed random delay in seconds a REED waits before
registering to become an Active Router.

### PROP 5386: PROP_THREAD_PREFERRED_ROUTER_ID
* Type: Write-Only
* Packed-Encoding: `C`

Specifies the preferred Router Id. Upon becoming a router/leader the node
attempts to use this Router Id. If the preferred Router Id is not set or
if it can not be used, a randomly generated router id is picked. This
property can be set only when the device role is either detached or
disabled.

### PROP 5387: PROP_THREAD_NEIGHBOR_TABLE
* Type: Read-Only
* Packed-Encoding: `A(t(ESLCcCbLL))`

Data per item is:

* `E`: Extended/long address
* `S`: RLOC16
* `L`: Age
* `C`: Link Quality In
* `c`: Average RSS
* `C`: Mode (bit-flags)
* `b`: `true` if neighbor is a child, `false` otherwise.
* `L`: Link Frame Counter
* `L`: MLE Frame Counter

### PROP 5388: PROP_THREAD_CHILD_COUNT_MAX
* Type: Read-Write
* Packed-Encoding: `C`

Specifies the maximum number of children currently allowed.
This parameter can only be set when Thread(R) protocol operation
has been stopped.

### PROP 5389: PROP_THREAD_LEADER_NETWORK_DATA
* Type: Read-Only
* Packed-Encoding: `D`

The leader network data.

### PROP 5390: PROP_THREAD_STABLE_LEADER_NETWORK_DATA
* Type: Read-Only
* Packed-Encoding: `D`

The stable leader network data.

### PROP 5391: PROP_THREAD_JOINERS {#prop-thread-joiners}

* Type: Insert/Remove Only (optionally Read-Write)
* Packed-Encoding: `A(t(ULE))`
* Required capability: `CAP_THREAD_COMMISSIONER`

Data per item is:

* `U`: PSKd
* `L`: Timeout in seconds
* `E`: Extended/long address (optional)

Passess Pre-Shared Key for the Device to the NCP in the commissioning process.
When the Extended address is ommited all Devices which provided a valid PSKd
are allowed to join the Thread(R) Network.

### PROP 5392: PROP_THREAD_COMMISSIONER_ENABLED {#prop-thread-commissioner-enabled}

* Type: Write only (optionally Read-Write)
* Packed-Encoding: `b`
* Required capability: `CAP_THREAD_COMMISSIONER`

Set to true to enable the native commissioner. It is mandatory before adding the joiner to the network.

### PROP 5393: PROP_THREAD_TMF_PROXY_ENABLED {#prop-thread-tmf-proxy-enabled}

* Type: Read-Write
* Packed-Encoding: `b`
* Required capability: `CAP_THREAD_TMF_PROXY`

Set to true to enable the TMF proxy.

### PROP 5394: PROP_THREAD_TMF_PROXY_STREAM {#prop-thread-tmf-proxy-stream}

* Type: Read-Write-Stream
* Packed-Encoding: `dSS`
* Required capability: `CAP_THREAD_TMF_PROXY`

Data per item is:

* `d`: CoAP frame
* `S`: source/destination RLOC/ALOC
* `S`: source/destination port

Octects: | 2      | *n*  |    2    |  2
---------|--------|------|---------|-------
Fields:  | Length | CoAP | locator | port

This property allows the host to send and receive TMF messages from
the NCP's RLOC address and support Thread-specific border router functions.


### PROP 5395: PROP_THREAD_DISOVERY_SCAN_JOINER_FLAG {#prop-thread-discovery-scan-joiner-flag}

* Type: Read-Write
* Packed-Encoding:: `b`

This property specifies the value used in Thread(R) MLE Discovery Request
TLV during discovery scan operation. Default value is `false`.

### PROP 5396: PROP_THREAD_DISCOVERY_SCAN_ENABLE_FILTERING {#prop-thread-discovery-scan-enable-filtering}

* Type: Read-Write
* Packed-Encoding:: `b`

This property is used to enable/disable EUI64 filtering during discovery
scan operation. Default value is `false`.

### PROP 5397: PROP_THREAD_DISCOVERY_SCAN_PANID {#prop-thread-discovery-scan-panid}

* Type: Read-write
* Packed-Encoding:: `S`

This property specifies the PANID used for filtering during discovery
scan operation. Default value is `0xffff` (broadcast PANID) which disables
PANID filtering.

### PROP 5398: PROP_THREAD_STEERING_DATA {#prop-thread-steering-data}

* Type: Write-Only
* Packed-Encoding: `E`
* Required capability: `CAP_OOB_STEERING_DATA`

This property can be used to set the steering data for MLE Discovery
Response messages.

* All zeros to clear the steering data (indicating no steering data).
* All 0xFFs to set the steering data (bloom filter) to accept/allow all.
* A specific EUI64 which is then added to steering data/bloom filter.

### PROP 5399: SPINEL_PROP_THREAD_ROUTER_TABLE {#prop-thread-router-table}

* Type: Read-Only
* Packed-Encoding: `A(t(ESCCCCCCb)`

Data per item is:

*  `E`: IEEE 802.15.4 Extended Address
*  `S`: RLOC16
*  `C`: Router ID
*  `C`: Next hop to router
*  `C`: Path cost to router
*  `C`: Link Quality In
*  `C`: Link Quality Out
*  `C`: Age (seconds since last heard)
*  `b`: Link established with Router ID or not.
