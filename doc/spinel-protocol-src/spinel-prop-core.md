## Core Properties {#prop-core}

### PROP 0: PROP_LAST_STATUS {#prop-last-status}

* Type: Read-Only
* Encoding: `i`

Octets: |    1-3
-------:|-------------
Fields: | LAST_STATUS

Describes the status of the last operation. Encoded as a packed
unsigned integer.

This property is emitted often to indicate the result status of
pretty much any Host-to-NCP operation.

It is emitted automatically at NCP startup with a value indicating
the reset reason.

See (#status-codes) for the complete list of status codes.

### PROP 1: PROP_PROTOCOL_VERSION {#prop-protocol-version}

* Type: Read-Only
* Encoding: `ii`

Octets: |       1-3      |      1-3
--------|----------------|---------------
Fields: |  MAJOR_VERSION | MINOR_VERSION

Describes the protocol version information. This property contains
four fields, each encoded as a packed unsigned integer:

 *  Major Version Number
 *  Minor Version Number

This document describes major version 4, minor version 1 of this protocol.

#### Major Version Number

The major version number is used to identify large and incompatible
differences between protocol versions.

The host MUST enter a FAULT state if it does not explicitly support
the given major version number.

#### Minor Version Number

The minor version number is used to identify small but otherwise
compatible differences between protocol versions. A mismatch between
the advertised minor version number and the minor version that is
supported by the host SHOULD NOT be fatal to the operation of the
host.

### PROP 2: PROP_NCP_VERSION {#prop-ncp-version}

* Type: Read-Only
* Packed-Encoding: `U`

Octets: |       *n*
--------|-------------------
Fields: | NCP_VESION_STRING

Contains a string which describes the firmware currently running on
the NCP. Encoded as a zero-terminated UTF-8 string.

The format of the string is not strictly defined, but it is intended
to present similarly to the "User-Agent" string from HTTP. The
RECOMMENDED format of the string is as follows:

    STACK-NAME/STACK-VERSION[BUILD_INFO][; OTHER_INFO]; BUILD_DATE_AND_TIME

Examples:

 *  `OpenThread/1.0d26-25-gb684c7f; DEBUG; May 9 2016 18:22:04`
 *  `ConnectIP/2.0b125 s1 ALPHA; Sept 24 2015 20:49:19`

### PROP 3: PROP_INTERFACE_TYPE {#prop-interface-type}

* Type: Read-Only
* Encoding: `i`

Octets: |       1-3
--------|----------------
Fields: | INTERFACE_TYPE

This integer identifies what the network protocol for this NCP.
Currently defined values are:

 *  0: Bootloader
 *  2: ZigBeeIP
 *  3: Thread

The host MUST enter a FAULT state if it does not recognize the
protocol given by the NCP.

### PROP 4: PROP_INTERFACE_VENDOR_ID {#prop-interface-vendor-id}

* Type: Read-Only
* Encoding: `i`

Octets: |       1-3
--------|----------------
Fields: | VENDOR_ID

Vendor identifier.

### PROP 5: PROP_CAPS {#prop-caps}

* Type: Read-Only
* Packed-Encoding: `A(i)`

Octets: |  1-3  |  1-3  | ...
--------|-------|-------|-----
Fields: | CAP_1 | CAP_2 | ...

Describes the supported capabilities of this NCP. Encoded as a list of
packed unsigned integers.

A capability is defined as a 21-bit integer that describes a subset of
functionality which is supported by the NCP.

Currently defined values are:

 * 1: `CAP_LOCK`
 * 2: `CAP_NET_SAVE`
 * 3: `CAP_HBO`: Host Buffer Offload. See (#feature-host-buffer-offload).
 * 4: `CAP_POWER_SAVE`
 * 5: `CAP_COUNTERS`
 * 6: `CAP_JAM_DETECT`: Jamming detection. See (#feature-jam-detect)
 * 7: `CAP_PEEK_POKE`: PEEK/POKE debugging commands.
 * 8: `CAP_WRITABLE_RAW_STREAM`: `PROP_STREAM_RAW` is writable.
 * 16: `CAP_802_15_4_2003`
 * 17: `CAP_802_15_4_2006`
 * 18: `CAP_802_15_4_2011`
 * 21: `CAP_802_15_4_PIB`
 * 24: `CAP_802_15_4_2450MHZ_OQPSK`
 * 25: `CAP_802_15_4_915MHZ_OQPSK`
 * 26: `CAP_802_15_4_868MHZ_OQPSK`
 * 27: `CAP_802_15_4_915MHZ_BPSK`
 * 28: `CAP_802_15_4_868MHZ_BPSK`
 * 29: `CAP_802_15_4_915MHZ_ASK`
 * 30: `CAP_802_15_4_868MHZ_ASK`
 * 48: `CAP_ROLE_ROUTER`
 * 49: `CAP_ROLE_SLEEPY`
 * 52: `CAP_NET_THREAD_1_0`
 * 512: `CAP_MAC_WHITELIST`

Additionally, future capability allocations SHALL be made from the
following allocation plan:

Capability Range      | Description
----------------------|------------------
0 - 127               | Reserved for core capabilities
128 - 15,359          | *UNALLOCATED*
15,360 - 16,383       | Vendor-specific
16,384 - 1,999,999    | *UNALLOCATED*
2,000,000 - 2,097,151 | Experimental use only


### PROP 6: PROP_INTERFACE_COUNT {#prop-interface-count}

* Type: Read-Only
* Packed-Encoding: `C`

Octets: |       1
--------|-----------------
Fields: | `INTERFACE_COUNT`

Describes the number of concurrent interfaces supported by this NCP.
Since the concurrent interface mechanism is still TBD, this value MUST
always be one.

This value is encoded as an unsigned 8-bit integer.

### PROP 7: PROP_POWER_STATE {#prop-power-state}

* Type: Read-Write
* Packed-Encoding: `C`

Octets: |        1
--------|------------------
Fields: | POWER_STATE

Describes the current power state of the NCP. By writing to this
property you can manage the lower state of the NCP. Enumeration is
encoded as a single unsigned byte.

Defined values are:

 *  0: `POWER_STATE_OFFLINE`: NCP is physically powered off.
    (Enumerated for completeness sake, not expected on the wire)
 *  1: `POWER_STATE_DEEP_SLEEP`: Almost everything on the NCP is shut
    down, but can still be resumed via a command or interrupt.
 *  2: `POWER_STATE_STANDBY`: NCP is in the lowest power state that
    can still be awoken by an event from the radio (e.g. waiting for
    alarm)
 *  3: `POWER_STATE_LOW_POWER`: NCP is responsive (and possibly
    connected), but using less power. (e.g. "Sleepy" child node)
 *  4: `POWER_STATE_ONLINE`: NCP is fully powered. (e.g. "Parent"
    node)

### PROP 8: PROP_HWADDR {#prop-hwaddr}

* Type: Read-Only*
* Packed-Encoding: `E`

Octets: |    8
--------|------------
Fields: | HWADDR

The static EUI64 address of the device, used as a serial number.
This value is read-only, but may be writable under certain
vendor-defined circumstances.

### PROP 9: PROP_LOCK {#prop-lock}

* Type: Read-Write
* Packed-Encoding: `b`

Octets: |    1
--------|------------
Fields: | LOCK

Property lock. Used for grouping changes to several properties to
take effect at once, or to temporarily prevent the automatic updating
of property values. When this property is set, the execution of the
NCP is effectively frozen until it is cleared.

This property is only supported if the `CAP_LOCK` capability is present.

Unlike most other properties, setting this property to true when the
value of the property is already true **MUST** fail with a last status
of `STATUS_ALREADY`.

## Stream Properties {#prop-stream}

### PROP 112: PROP_STREAM_DEBUG {#prop-stream-debug}

* Type: Read-Only-Stream
* Packed-Encoding: `U`

Octets: |    *n*
--------|------------
Fields: | UTF8_DATA

This property is a streaming property, meaning that you cannot explicitly
fetch the value of this property. The stream provides human-readable debugging
output which may be displayed in the host logs.

The location of newline characters is not assumed by the host: it is
the NCP's responsibility to insert newline characters where needed,
just like with any other text stream.

To receive the debugging stream, you wait for `CMD_PROP_VALUE_IS`
commands for this property from the NCP.

### PROP 113: PROP_STREAM_RAW {#prop-stream-raw}

* Type: Read-Write-Stream
* Packed-Encoding: `DD`

Octets: |        2       |     *n*    |       *n*
--------|----------------|------------|----------------
Fields: | FRAME_DATA_LEN | FRAME_DATA | FRAME_METADATA

This stream provides the capability of sending and receiving raw packets
to and from the radio. The exact format of the frame metadata and data is
dependent on the MAC and PHY being used.

This property is a streaming property, meaning that you cannot explicitly
fetch the value of this property. To receive traffic, you wait for
`CMD_PROP_VALUE_IS` commands with this property id from the NCP.

Implementations may OPTIONALLY support the ability to transmit arbitrary
raw packets. Support for this feature is indicated by the presence of the
`CAP_WRITABLE_RAW_STREAM` capability.

If the capability `CAP_WRITABLE_RAW_STREAM` is set, then packets written
to this stream with `CMD_PROP_VALUE_SET` will be sent out over the radio.
This allows the caller to use the radio directly, with the stack being
implemented on the host instead of the NCP.

#### Frame Metadata Format {#frame-metadata-format}

Any data past the end of `FRAME_DATA_LEN` is considered metadata and is
OPTIONAL. Frame metadata MAY be empty or partially specified. Partially
specified metadata MUST be accepted. Default values are used for all
unspecified fields.

The same general format is used for `PROP_STREAM_RAW`, `PROP_STREAM_NET`,
and `PROP_STREAM_NET_INSECURE`. It can be used for frames sent from the
NCP to the host as well as frames sent from the host to the NCP.

The frame metadata field consists of the following fields:

 Field   | Description                  | Type       | Len   | Default
:--------|:-----------------------------|:-----------|-------|----------
MD_POWER | (dBm) RSSI/TX-Power          | `c` int8   | 1     | -128
MD_NOISE | (dBm) Noise floor            | `c` int8   | 1     | -128
MD_FLAG  | Flags (defined below)        | `S` uint16 | 2     |
MD_PHY   | PHY-specific data            | `D` data   | >=2   |
MD_VEND  | Vendor-specific data         | `D` data   | >=2   |

The following fields are ignored by the NCP for packets sent to it from
the host:

* MD_NOISE
* MD_FLAG

When specifying `MD_POWER` for a packet to be transmitted, the actual
transmit power is never larger than the current value of `PROP_PHY_TX_POWER`
((#prop-phy-tx-power)). When left unspecified (or set to the value -128),
an appropriate transmit power will be chosen by the NCP.

The bit values in `MD_FLAG` are defined as follows:

 Bit     | Mask   | Name              | Description if set
---------|--------|:------------------|:----------------
15       | 0x0001 | MD_FLAG_TX        | Packet was transmitted, not received.
13       | 0x0004 | MD_FLAG_BAD_FCS   | Packet was received with bad FCS
12       | 0x0008 | MD_FLAG_DUPE      | Packet seems to be a duplicate
0-11, 14 | 0xFFF2 | MD_FLAG_RESERVED  | Flags reserved for future use.

The format of `MD_PHY` is specified by the PHY layer currently in use,
and may contain information such as the channel, LQI, antenna, or other
pertainent information.

### PROP 114: PROP_STREAM_NET {#prop-stream-net}

* Type: Read-Write-Stream
* Packed-Encoding: `DD`

Octets: |        2       |     *n*    |       *n*
--------|----------------|------------|----------------
Fields: | FRAME_DATA_LEN | FRAME_DATA | FRAME_METADATA

This stream provides the capability of sending and receiving data packets
to and from the currently attached network. The exact format of the frame
metadata and data is dependent on the network protocol being used.

This property is a streaming property, meaning that you cannot explicitly
fetch the value of this property. To receive traffic, you wait for
`CMD_PROP_VALUE_IS` commands with this property id from the NCP.

To send network packets, you call `CMD_PROP_VALUE_SET` on this property with
the value of the packet.

Any data past the end of `FRAME_DATA_LEN` is considered metadata, the
format of which is described in (#frame-metadata-format).

### PROP 114: PROP_STREAM_NET_INSECURE {#prop-stream-net-insecure}

* Type: Read-Write-Stream
* Packed-Encoding: `DD`

Octets: |        2       |     *n*    |       *n*
--------|----------------|------------|----------------
Fields: | FRAME_DATA_LEN | FRAME_DATA | FRAME_METADATA

This stream provides the capability of sending and receiving unencrypted
and unauthenticated data packets to and from nearby devices for the
purposes of device commissioning. The exact format of the frame
metadata and data is dependent on the network protocol being used.

This property is a streaming property, meaning that you cannot explicitly
fetch the value of this property. To receive traffic, you wait for
`CMD_PROP_VALUE_IS` commands with this property id from the NCP.

To send network packets, you call `CMD_PROP_VALUE_SET` on this property with
the value of the packet.

Any data past the end of `FRAME_DATA_LEN` is considered metadata, the
format of which is described in (#frame-metadata-format).

