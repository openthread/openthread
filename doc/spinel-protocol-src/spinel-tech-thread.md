# Technology: Thread

This section describes all of the properties and semantics required
for managing a Thread NCP.

Thread NCPs have the following requirements:

* The property `PROP_INTERFACE_TYPE` must be 3.
* The non-optional properties in the following sections **MUST** be
  implemented: CORE, PHY, MAC, NET, and IPV6.

All serious implementations of an NCP **SHOULD** also support the network
save feature (See (#feature-network-save)).

## Thread Capabilities

The Thread technology defines the following capabilities:

* `CAP_NET_THREAD_1_0` - Indicates that the NCP implements v1.0 of the Thread standard.
* `CAP_NET_THREAD_1_1` - Indicates that the NCP implements v1.1 of the Thread standard.

## Thread Properties

Properties for Thread are allocated out of the `Tech` property
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
* `C`: Prefix length, in bits
* `b`: Stable flag
* `C`: Thread flags
* `b`: "Is defined locally" flag. Set if this network was locally
  defined. Assumed to be true for set, insert and replace. Clear if
  the on mesh network was defined by another node.

### PROP 91: PROP_THREAD_LOCAL_ROUTES
* Type: Read-Write
* Packed-Encoding: `A(t(6CbC))`

Data per item is:

* `6`: IPv6 Prefix
* `C`: Prefix length, in bits
* `b`: Stable flag
* `C`: Other flags

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
bitfield are defined by section 4.5.2 of the Thread
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

Allows you to get or set the Thread `NETWORK_ID_TIMEOUT` constant, as
defined by the Thread specification.

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
This parameter can only be set when Thread protocol operation
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

* Type: Read-Write
* Packed-Encoding: `A(T(ED.))`

Data per item is:

* `E`: Extended/long address
* `D`: PSKd

Passess Extended address and PSKd to the NCP in the commissioning process.


### PROP 5392: PROP_THREAD_COMMISSIONER_ENABLED {#prop-thread-commissioner-enabled}

* Type: Read-Write
* Packed-Encoding: `b`

Set to true to enable the native commissioner. It is mandatory before adding the joiner to the network.
