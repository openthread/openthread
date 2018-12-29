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
* Packed-Encoding: `ESLccCC`

Information about parent of this node.

*  `E`: Extended address
*  `S`: RLOC16
*  `L`: Age (seconds since last heard from)
*  `c`: Average RSS (in dBm)
*  `c`: Last RSSI (in dBm)
*  `C`: Link Quality In
*  `C`: Link Quality Out

### PROP 82: PROP_THREAD_CHILD_TABLE
* Type: Read-Only
* Packed-Encoding: `A(t(ESLLCCcCc)`

Table containing info about all the children of this node.

Data per item is:

* `E`: Extended address
* `S`: RLOC16
* `L`: Timeout (in seconds)
* `L`: Age (in seconds)
* `L`: Network Data version
* `C`: Link Quality In
* `c`: Average RSS (in dBm)
* `C`: Mode (bit-flags)
* `c`: Last RSSI (in dBm)

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
* Packed-Encoding: `A(t(6CbCbS))`

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
* Packed-Encoding: `A(t(ESLCcCbLLc))`

Data per item is:

* `E`: Extended address
* `S`: RLOC16
* `L`: Age
* `C`: Link Quality In
* `c`: Average RSS (in dBm)
* `C`: Mode (bit-flags)
* `b`: `true` if neighbor is a child, `false` otherwise.
* `L`: Link Frame Counter
* `L`: MLE Frame Counter
* `c`: The last RSSI (in dBm)

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
* `E`: IEEE EUI-64 (optional)

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

Set to true to enable the TMF proxy. This property is deprecated.

### PROP 5394: PROP_THREAD_TMF_PROXY_STREAM {#prop-thread-tmf-proxy-stream}

* Type: Read-Write-Stream
* Packed-Encoding: `dSS`
* Required capability: `CAP_THREAD_TMF_PROXY`

This property is deprecated. Please see `SPINEL_PROP_THREAD_UDP_FORWARD_STREAM`.

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

### PROP 5400: SPINEL_PROP_THREAD_ACTIVE_DATASET (#prop-thread-active-dataset)

  * Type: Read-Write
  * Packing-Encoding: `A(t(iD))`

This property provides access to current Thread Active Operational Dataset.
A Thread device maintains the Operational Dataset that it has stored locally
and the one currently in use by the partition to which it is attached. This
property corresponds to the locally stored Dataset on the device.

Operational Dataset consists of a set of supported properties (e.g., channel,
master key, network name, PAN id, etc). Note that not all supported properties
may be present (have a value) in a Dataset.

The Dataset value is encoded as an array of structures containing pairs of
property key (as `i`) followed by the property value (as `D`). The property
value must follow the format associated with the corresponding spinel
property.

On write, any unknown/unsupported property keys must be ignored.

The following properties can be included in a Dataset list:

*   SPINEL_PROP_DATASET_ACTIVE_TIMESTAMP
*   SPINEL_PROP_PHY_CHAN
*   SPINEL_PROP_PHY_CHAN_SUPPORTED (Channel Mask Page 0)
*   SPINEL_PROP_NET_MASTER_KEY
*   SPINEL_PROP_NET_NETWORK_NAME
*   SPINEL_PROP_NET_XPANID
*   SPINEL_PROP_MAC_15_4_PANID
*   SPINEL_PROP_IPV6_ML_PREFIX
*   SPINEL_PROP_NET_PSKC
*   SPINEL_PROP_DATASET_SECURITY_POLICY

### PROP 5401: SPINEL_PROP_THREAD_PENDING_DATASET (#prop-thread-pending-dataset)

  * Type: Read-Write
  * Packing-Encoding: `A(t(iD))`

This property provide access to current Thread Pending Operational Dataset
locally stored on the device.

The formatting of this property follows the same rules as in
SPINEL_PROP_THREAD_ACTIVE_DATASET.

In addition supported properties in SPINEL_PROP_THREAD_ACTIVE_DATASET, the
following properties can also be included in the Pending Dataset:

*   SPINEL_PROP_DATASET_PENDING_TIMESTAMP
*   SPINEL_PROP_DATASET_DELAY_TIMER

### PROP 5402: SPINEL_PROP_THREAD_MGMT_SET_ACTIVE_DATASET (#prop-thread-mgmt-set-active-dataset)

  * Type: Write only
  * Packing-Encoding: `A(t(iD))`

The formatting of this property follows the same rules as in
SPINEL_PROP_THREAD_ACTIVE_DATASET.

This is write-only property. When written, it triggers a MGMT_ACTIVE_SET meshcop
command to be sent to the leader with the given Dataset. The spinel frame response
should be a `LAST_STATUS` with the status of the transmission of MGMT_ACTIVE_SET
command.

In addition to supported properties in SPINEL_PROP_THREAD_ACTIVE_DATASET, the
following property can be included in the Dataset (to allow for custom raw
TLVs):

*    SPINEL_PROP_DATASET_RAW_TLVS

### PROP 5403: SPINEL_PROP_THREAD_MGMT_SET_PENDING_DATASET (#prop-thread-mgmt-set-pending-dataset)

  * Type: Write only
  * Packing-Encoding: `A(t(iD))`

This property is similar to SPINEL_PROP_THREAD_PENDING_DATASET and follows the
same format and rules.

In addition to supported properties in SPINEL_PROP_THREAD_PENDING_DATASET, the
following property can be included the Dataset (to allow for custom raw TLVs to
be provided):

*    SPINEL_PROP_DATASET_RAW_TLVS

### PROP 5404: SPINEL_PROP_DATASET_ACTIVE_TIMESTAMP (#prop-dataset-active-timestamps)

  * Type: No direct read or write
  * Packing-Encoding: `X`

This property represents the Active Timestamp field in a Thread Operational
Dataset.

This can only be included in one of the Dataset related properties below:

*   SPINEL_PROP_THREAD_ACTIVE_DATASET
*   SPINEL_PROP_THREAD_PENDING_DATASET
*   SPINEL_PROP_THREAD_MGMT_SET_ACTIVE_DATASET
*   SPINEL_PROP_THREAD_MGMT_SET_PENDING_DATASET
*   SPINEL_PROP_THREAD_MGMT_GET_ACTIVE_DATASET
*   SPINEL_PROP_THREAD_MGMT_GET_PENDING_DATASET

### PROP 5405: SPINEL_PROP_DATASET_PENDING_TIMESTAMP (#prop-dataset-pending-timestamps)

  * Type: No direct read or write
  * Packing-Encoding: `X`

This property represents the Pending Timestamp field in a Thread Operational
Dataset.

It can only be included in one of the Pending Dataset properties:

*   SPINEL_PROP_THREAD_PENDING_DATASET
*   SPINEL_PROP_THREAD_MGMT_SET_PENDING_DATASET
*   SPINEL_PROP_THREAD_MGMT_GET_PENDING_DATASET

### PROP 5406: SPINEL_PROP_DATASET_DELAY_TIMER (#prop-dataset-delay-timer)

  * Type: No direct read or write
  * Packing-Encoding: `L`

This property represents the Delay Timer field in a Thread Operational Dataset.
Delay timer (in ms) specifies the time renaming until Thread devices overwrite
the value in the Active Operational Dataset with the corresponding values in the
Pending Operational Dataset.

It can only be included in one of the Pending Dataset properties:

*   SPINEL_PROP_THREAD_PENDING_DATASET
*   SPINEL_PROP_THREAD_MGMT_SET_PENDING_DATASET
*   SPINEL_PROP_THREAD_MGMT_GET_PENDING_DATASET

### PROP 5407: SPINEL_PROP_DATASET_SECURITY_POLICY (#prop-dataset-security-policy)

  * Type: No direct read or write
  * Packing-Encoding: `SC`

This property represents the Security Policy field in a Thread Operational
Dataset.

The content is:

*   `S` : Key Rotation Time (in units of hour)
*   `C` : Security Policy Flags (as specified in Thread 1.1 Section 8.10.1.15)

It can only be included in one of the Dataset related properties below:

*   SPINEL_PROP_THREAD_ACTIVE_DATASET
*   SPINEL_PROP_THREAD_PENDING_DATASET
*   SPINEL_PROP_THREAD_MGMT_SET_ACTIVE_DATASET
*   SPINEL_PROP_THREAD_MGMT_SET_PENDING_DATASET
*   SPINEL_PROP_THREAD_MGMT_GET_ACTIVE_DATASET
*   SPINEL_PROP_THREAD_MGMT_GET_PENDING_DATASET

### PROP 5408: SPINEL_PROP_DATASET_RAW_TLVS (#prop-dataset-raw-tlvs)

  * Type: No direct read or write
  * Packing-Encoding: `D`

This property defines extra raw TLVs that can be added to an Operational
DataSet.

It can only be included in one of the following Dataset properties:

*   SPINEL_PROP_THREAD_MGMT_SET_ACTIVE_DATASET
*   SPINEL_PROP_THREAD_MGMT_SET_PENDING_DATASET

### PROP 5409: SPINEL_PROP_THREAD_CHILD_TABLE_ADDRESSES (#prop-thread-child-table-addresses)

* Type: Read-Only
* Packing-Encoding: `A(t(ESA(6))`

This property provides the list of all addresses associated with every child
including any registered IPv6 addresses.

Data per item is:

* `E`: Extended address of the child
* `S`: RLOC16 of the child
* `A(6)`: List of IPv6 addresses registered by the child (if any)

### PROP 5410: SPINEL_PROP_THREAD_NEIGHBOR_TABLE_ERROR_RATES (#prop-thread-neighbor-table-error-rates)

* Type: Read-Only
* Packing-Encoding: `A(t(ESSScc))`
* Required capability: `CAP_ERROR_RATE_TRACKING`

This property provides link quality related info including
frame and (IPv6) message error rates for all neighbors.

With regards to message error rate, note that a larger (IPv6)
message can be fragmented and sent as multiple MAC frames. The
message transmission is considered a failure, if any of its
fragments fail after all MAC retry attempts.

Data per item is:

* `E`: Extended address of the neighbor
* `S`: RLOC16 of the neighbor
* `S`: Frame error rate (0 -> 0%, 0xffff -> 100%)
* `S`: Message error rate (0 -> 0%, 0xffff -> 100%)
* `c`: Average RSSI (in dBm)
* `c`: Last RSSI (in dBm)

### PROP 5411: SPINEL_PROP_THREAD_ADDRESS_CACHE_TABLE (#prop-thread-address-cache-table)

* Type: Read-Only
* Packing-Encoding: `A(t(6SC))`

This property provides Thread EID IPv6 address cache table.

Data per item is:

* `6` : Target IPv6 address
* `S` : RLOC16 of target
* `C` : Age (order of use, 0 indicates most recently used entry)

### PROP 5412: SPINEL_PROP_THREAD_UDP_FORWARD_STREAM (#prop-thread-udp-proxy-stream)

* Type: Write-Stream
* Packed-Encoding: `dS6S`
* Required capability: `CAP_THREAD_UDP_FORWARD`

This property helps exchange UDP packets with host.

  `d`: UDP payload
  `S`: Remote UDP port
  `6`: Remote IPv6 address
  `S`: Local UDP port

### PROP 5413: SPINEL_PROP_THREAD_MGMT_GET_ACTIVE_DATASET (#prop-thread-mgmt-get-active-dataset)

* Type: Write-Only
* Packing-format: `A(t(iD))`

The formatting of this property follows the same rules as in
SPINEL_PROP_THREAD_MGMT_SET_ACTIVE_DATASET. This property
allows the sender to not include a value associated with
properties in formating of `t(iD)`, i.e., it should accept
either a `t(iD)` or a `t(i)` encoding which in both cases
indicate the associated Dataset property should be requested
as part of MGMT_GET command.

When written, it triggers a MGMT_ACTIVE_GET meshcop command to be
sent to leader with the given Dataset. The spinel frame response
should be a `LAST_STATUS` with the status of the transmission
of MGMT_ACTIVE_GET command.

In addition to supported properties in
SPINEL_PROP_THREAD_MGMT_SET_ACTIVE_DATASET, the following property
can be optionally included in the Dataset:

*    SPINEL_PROP_DATASET_DEST_ADDRESS

### PROP 5414: SPINEL_PROP_THREAD_MGMT_GET_PENDING_DATASET (#prop-thread-mgmt-get-pending-dataset)

* Type: Write-Only
* Packing-format: `A(t(iD))`

The formatting of this property follows the same rules as in
SPINEL_PROP_THREAD_MGMT_GET_ACTIVE_DATASET.

This is write-only property. When written, it triggers a
MGMT_PENDING_GET meshcop command to be sent to leader with the
given Dataset. The spinel frame response should be a
`LAST_STATUS` with the status of the transmission of
MGMT_PENDING_GET command.

In addition to supported properties in
SPINEL_PROP_THREAD_MGMT_SET_PENDING_DATASET, the following property
can be optionally included the Dataset:

*    SPINEL_PROP_DATASET_DEST_ADDRESS

### PROP 5415: SPINEL_PROP_DATASET_DEST_ADDRESS (#prop-dataset-dest-address)

* Type: No direct read or write
* Packing-Encoding: `6`

This property specifies the IPv6 destination when sending
MGMT_GET command for either Active or Pending Dataset if not
provided, Leader ALOC address is used as default.

This can only be included in one of the Dataset related properties below:

*   SPINEL_PROP_THREAD_MGMT_GET_ACTIVE_DATASET
*   SPINEL_PROP_THREAD_MGMT_GET_PENDING_DATASET
