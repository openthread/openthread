## MAC Properties {#prop-mac}

### PROP 48: PROP_MAC_SCAN_STATE {#prop-mac-scan-state}
* Type: Read-Write
* Packed-Encoding: `C`
* Unit: Enumeration

Possible Values:

* 0: `SCAN_STATE_IDLE`
* 1: `SCAN_STATE_BEACON`
* 2: `SCAN_STATE_ENERGY`

Set to `SCAN_STATE_BEACON` to start an active scan.
Beacons will be emitted from `PROP_MAC_SCAN_BEACON`.

Set to `SCAN_STATE_ENERGY` to start an energy scan.
Channel energy will be reported by alternating emissions
of `PROP_PHY_CHAN` and `PROP_PHY_RSSI`.

Values switches to `SCAN_STATE_IDLE` when scan is complete.

### PROP 49: PROP_MAC_SCAN_MASK {#prop-mac-scan-mask}
* Type: Read-Write
* Packed-Encoding: `A(C)`
* Unit: List of channels to scan


### PROP 50: PROP_MAC_SCAN_PERIOD {#prop-mac-scan-period}
* Type: Read-Write
* Packed-Encoding: `S` (uint16)
* Unit: milliseconds per channel

### PROP 51: PROP_MAC_SCAN_BEACON {#prop-mac-scan-beacon}
* Type: Read-Only-Stream
* Packed-Encoding: `CcDD.` (or `CcT(ESSc.)T(iCUD.).`)

Octets: | 1  |   1  |    2    |   *n*    |    2    |   *n*  
--------|----|------|---------|----------|---------|----------  
Fields: | CH | RSSI | MAC_LEN | MAC_DATA | NET_LEN | NET_DATA  

Scan beacons have two embedded structures which contain
information about the MAC layer and the NET layer. Their
format depends on the MAC and NET layer currently in use.
The format below is for an 802.15.4 MAC with Thread:

* `C`: Channel
* `c`: RSSI of the beacon
* `T`: MAC layer properties
  * `E`: Long address
  * `S`: Short address
  * `S`: PAN-ID
  * `c`: LQI
* `T`: NET layer properties
  * `i`: Protocol Number
  * `C`: Flags
  * `U`: Network Name
  * `D`: XPANID

Extra parameters may be added to each of the structures
in the future, so care should be taken to read the length
that prepends each structure.

### PROP 52: PROP_MAC_15_4_LADDR {#prop-mac-15-4-laddr}
* Type: Read-Write
* Packed-Encoding: `E`

The 802.15.4 long address of this node.

This property is only present on NCPs which implement 802.15.4

### PROP 53: PROP_MAC_15_4_SADDR {#prop-mac-15-4-saddr}
* Type: Read-Write
* Packed-Encoding: `S`

The 802.15.4 short address of this node.

This property is only present on NCPs which implement 802.15.4

### PROP 54: PROP_MAC_15_4_PANID {#prop-mac-15-4-panid}
* Type: Read-Write
* Packed-Encoding: `S`

The 802.15.4 PANID this node is associated with.

This property is only present on NCPs which implement 802.15.4

### PROP 55: PROP_MAC_RAW_STREAM_ENABLED {#prop-mac-raw-stream-enabled}
* Type: Read-Write
* Packed-Encoding: `b`

Set to true to enable raw MAC frames to be emitted from `PROP_STREAM_RAW`.
See (#prop-stream-raw).

### PROP 56: PROP_MAC_PROMISCUOUS_MODE {#prop-mac-promiscuous-mode}
* Type: Read-Write
* Packed-Encoding: `C`

Possible Values:

Id | Name                          | Description
---|-------------------------------|------------------
 0 | `MAC_PROMISCUOUS_MODE_OFF`    | Normal MAC filtering is in place.
 1 | `MAC_PROMISCUOUS_MODE_NETWORK`| All MAC packets matching network are passed up the stack.
 2 | `MAC_PROMISCUOUS_MODE_FULL`   | All decoded MAC packets are passed up the stack.

See (#prop-stream-raw).

### PROP 4864: PROP_MAC_WHITELIST  {#prop-mac-whitelist}
* Type: Read-Write
* Packed-Encoding: `A(T(Ec))`
* **OPTIONAL**

Structure Parameters:

* `E`: EUI64 address of node
* `c`: Optional RSSI-override value. The value 127 indicates
       that the RSSI-override feature is not enabled for this
       address. If this value is omitted when setting or
       inserting, it is assumed to be 127. This parameter is
       ignored when removing.

### PROP 4865: PROP_MAC_WHITELIST_ENABLED  {#prop-mac-whitelist-enabled}
* Type: Read-Write
* Packed-Encoding: `b`

### PROP 4867: SPINEL_PROP_MAC_SRC_MATCH_ENABLED  {#prop-mac-src-match-enabled}
* Type: Write
* Packed-Encoding: `b`

Set to true to enable radio source matching or false to disable it. This property
is only available if the `SPINEL_CAP_MAC_RAW` capability is present.

### PROP 4868: SPINEL_PROP_MAC_SRC_MATCH_SHORT_ADDRESSES  {#prop-mac-src-match-short-addresses}
* Type: Write
* Packed-Encoding: `A(S)`

Configures the list of short addresses used for source matching. This property
is only available if the `SPINEL_CAP_MAC_RAW` capability is present.

Structure Parameters:

* `S`: Short address for hardware generated ACKs

### PROP 4869: SPINEL_PROP_MAC_SRC_MATCH_EXTENDED_ADDRESSES  {#prop-mac-src-match-extended-addresses}
* Type: Write
* Packed-Encoding: `A(E)`

Configures the list of extended addresses used for source matching. This property
is only available if the `SPINEL_CAP_MAC_RAW` capability is present.

Structure Parameters:

* `E`: EUI64 address for hardware generated ACKs

