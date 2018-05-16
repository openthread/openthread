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

This document describes major version 4, minor version 3 of this protocol.

The host **MUST** only use this property from NLI 0. Behavior when used
from other NLIs is undefined.

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

The host **MUST** only use this property from NLI 0. Behavior when used
from other NLIs is undefined.

### PROP 3: PROP_INTERFACE_TYPE {#prop-interface-type}

* Type: Read-Only
* Encoding: `i`

Octets: |       1-3
--------|----------------
Fields: | INTERFACE_TYPE

This integer identifies what the network protocol for this NCP.
Currently defined values are:

 *  0: Bootloader
 *  2: ZigBee IP(TM)
 *  3: Thread(R)

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
 * 9: `CAP_GPIO`: Support for GPIO access. See (#feature-gpio-access).
 * 10: `CAP_TRNG`: Support for true random number generation. See (#feature-trng).
 * 11: `CAP_CMD_MULTI`: Support for `CMD_PROP_VALUE_MULTI_GET` ((#cmd-prop-value-multi-get)), `CMD_PROP_VALUE_MULTI_SET` ((#cmd-prop-value-multi-set), and `CMD_PROP_VALUES_ARE` ((#cmd-prop-values-are)).
 * 12: `CAP_UNSOL_UPDATE_FILTER`: Support for `PROP_UNSOL_UPDATE_FILTER` ((#prop-unsol-update-filter)) and `PROP_UNSOL_UPDATE_LIST` ((#prop-unsol-update-list)).
 * 13: `CAP_MCU_POWER_SAVE`: Support for controlling NCP's MCU power state (`PROP_MCU_POWER_STATE`).
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
 * 513: `CAP_MAC_RAW`
 * 514: `CAP_OOB_STEERING_DATA`
 * 1024: `CAP_THREAD_COMMISSIONER`
 * 1025: `CAP_THREAD_TMF_PROXY`


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

The host **MUST** only use this property from NLI 0. Behavior when used
from other NLIs is undefined.

### PROP 7: PROP_POWER_STATE {#prop-power-state} (deprecated)

* Type: Read-Write
* Packed-Encoding: `C`

Octets: |        1
--------|------------------
Fields: | POWER_STATE

This property is **deprecated**. `MCU_POWER_STATE` provides similar
functionality.

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

<!-- RQ
  -- We should consider reversing the numbering here so that 0 is
     `POWER_STATE_ONLINE`. We may also want to include some extra
     values between the defined values for future expansion, so
     that we can preserve the ordered relationship. -- -->

### PROP 8: PROP_HWADDR {#prop-hwaddr}

* Type: Read-Only\*
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

### PROP 12: PROP_HOST_POWER_STATE {#prop-host-power-state}

* Type: Read-Write
* Packed-Encoding: `C`
* Default value: 4

Octets: |        1
--------|------------------
Fields: | `HOST_POWER_STATE`

Describes the current power state of the *host*. This property is used
by the host to inform the NCP when it has changed power states. The
NCP can then use this state to determine which properties need
asynchronous updates. Enumeration is encoded as a single unsigned
byte. These states are defined in similar terms to `PROP_POWER_STATE`
((#prop-power-state)).

Defined values are:

*   0: `HOST_POWER_STATE_OFFLINE`: Host is physically powered off and
    cannot be woken by the NCP. All asynchronous commands are
    squelched.
*   1: `HOST_POWER_STATE_DEEP_SLEEP`: The host is in a low power state
    where it can be woken by the NCP but will potentially require more
    than two seconds to become fully responsive. The NCP **MUST**
    avoid sending unnecessary property updates, such as child table
    updates or non-critical messages on the debug stream. If the NCP
    needs to wake the host for traffic, the NCP **MUST** first take
    action to wake the host. Once the NCP signals to the host that it
    should wake up, the NCP **MUST** wait for some activity from the
    host (indicating that it is fully awake) before sending frames.
*   2: **RESERVED**. This value **MUST NOT** be set by the host. If
    received by the NCP, the NCP **SHOULD** consider this as a synonym
    of `HOST_POWER_STATE_DEEP_SLEEP`.
*   3: `HOST_POWER_STATE_LOW_POWER`: The host is in a low power state
    where it can be immediately woken by the NCP. The NCP **SHOULD**
    avoid sending unnecessary property updates, such as child table
    updates or non-critical messages on the debug stream.
*   4: `HOST_POWER_STATE_ONLINE`: The host is awake and responsive. No
    special filtering is performed by the NCP on asynchronous updates.
*   All other values are **RESERVED**. They MUST NOT be set by the
    host. If received by the NCP, the NCP **SHOULD** consider the value as
    a synonym of `HOST_POWER_STATE_LOW_POWER`.

<!-- RQ
  -- We should consider reversing the numbering here so that 0 is
     `POWER_STATE_ONLINE`. We may also want to include some extra
     values between the defined values for future expansion, so
     that we can preserve the ordered relationship. -- -->

After setting this power state, any further commands from the host to
the NCP will cause `HOST_POWER_STATE` to automatically revert to
`HOST_POWER_STATE_ONLINE`.

When the host is entering a low-power state, it should wait for the
response from the NCP acknowledging the command (with `CMD_VALUE_IS`).
Once that acknowledgement is received the host may enter the low-power
state.

If the NCP has the `CAP_UNSOL_UPDATE_FILTER` capability, any unsolicited
property updates masked by `PROP_UNSOL_UPDATE_FILTER` should be honored
while the host indicates it is in a low-power state. After resuming to the
`HOST_POWER_STATE_ONLINE` state, the value of `PROP_UNSOL_UPDATE_FILTER`
**MUST** be unchanged from the value assigned prior to the host indicating
it was entering a low-power state.

The host **MUST** only use this property from NLI 0. Behavior when used
from other NLIs is undefined.


### PROP 13: PROP_MCU_POWER_STATE {#prop-mcu-power-state}
* Type: Read-Write
* Packed-Encoding: `C`
* Required capability: CAP_MCU_POWER_SAVE

This property specifies the desired power state of NCP's micro-controller
(MCU) when the underlying platform's operating system enters idle mode (i.e.,
all active tasks/events are processed and the MCU can potentially enter a
energy-saving power state).

The power state primarily determines how the host should interact with the NCP
and whether the host needs an external trigger (a "poke") to NCP before it can
communicate with the NCP or not. After a reset, the MCU power state MUST be
`SPINEL_MCU_POWER_STATE_ON`.

Defined values are:

*  0: `SPINEL_MCU_POWER_STATE_ON`: NCP's MCU stays on and active all the time.
   When the NCP's desired power state is set to this value, host can send
   messages to NCP without requiring any "poke" or external triggers. MCU is
   expected to stay on and active. Note that the `ON` power state only determines
   the MCU's power mode and is not related to radio's state.

*  1: `SPINEL_MCU_POWER_STATE_LOW_POWER`:  NCP's MCU can enter low-power
   (energy-saving) state. When the NCP's desired power state is set to
   `LOW_POWER`, host is expected to "poke" the NCP (e.g., an external trigger
   like an interrupt) before it can communicate with the NCP (send a message
   to the NCP). The "poke" mechanism is determined by the platform code (based
   on NCP's interface to the host).
   While power state is set to `LOW_POWER`, NCP can still (at any time) send
   messages to host. Note that receiving a message from the NCP does NOT
   indicate that the NCP's power state has changed, i.e., host is expected to
   continue to "poke" NCP when it wants to talk to the NCP until the power
   state is explicitly changed (by setting this property to `ON`).
   Note that the `LOW_POWER` power state only determines the MCU's power mode
   and is not related to radio's state.

*  2: `SPINEL_MCU_POWER_STATE_OFF`: NCP is fully powered off.
   An NCP hardware reset (via a RESET pin) is required to bring the NCP back
   to `SPINEL_MCU_POWER_STATE_ON`. RAM is not retained after reset.

### PROP 4104: PROP_UNSOL_UPDATE_FILTER {#prop-unsol-update-filter}

* Required only if `CAP_UNSOL_UPDATE_FILTER` is set.
* Type: Read-Write
* Packed-Encoding: `A(I)`
* Default value: Empty.

Contains a list of properties which are *excluded* from generating
unsolicited value updates. This property **MUST** be empty after reset.

In other words, the host may opt-out of unsolicited property updates
for a specific property by adding that property id to this list.

Hosts **SHOULD NOT** add properties to this list which are not
present in `PROP_UNSOL_UPDATE_LIST`. If such properties are added,
the NCP **MUST** ignore the unsupported properties.

<!-- RQ
  -- The justification for the above behavior is to attempt to avoid possible
     future interop problems by explicitly making sure that unknown
     properties are ignored. Since unknown properties will obviously not be
     generating unsolicited updates, it seems fairly harmless. An
     implementation may print out a warning to the debug stream.

     Note that the error is still detectable: If you VALUE\_SET unsupported
     properties, the resulting VALUE\_IS would contain only the supported
     properties of that set(since the unsupported properties would be
     ignored). If an implementation cares that much about getting this
     right then it needs to make sure that it checks
     PROP\_UNSOL\_UPDATE\_LIST first.
  -- -->

Implementations of this property are only **REQUIRED** to support
and use the following commands:

* `CMD_PROP_VALUE_GET` ((#cmd-prop-value-get))
* `CMD_PROP_VALUE_SET` ((#cmd-prop-value-set))
* `CMD_PROP_VALUE_IS` ((#cmd-prop-value-is))

Implementations of this property **MAY** optionally support and use
the following commands:

* `CMD_PROP_VALUE_INSERT` ((#cmd-prop-value-insert))
* `CMD_PROP_VALUE_REMOVE` ((#cmd-prop-value-remove))
* `CMD_PROP_VALUE_INSERTED` ((#cmd-prop-value-inserted))
* `CMD_PROP_VALUE_REMOVED` ((#cmd-prop-value-removed))

Host implementations which are aiming to maximize their compatability across
different firmwre implementations **SHOULD NOT** assume the availability of the
optional commands for this property.

The value of this property **SHALL** be independent for each NLI.

### PROP 4105: PROP_UNSOL_UPDATE_LIST {#prop-unsol-update-list}

* Required only if `CAP_UNSOL_UPDATE_FILTER` is set.
* Type: Read-Only
* Packed-Encoding: `A(I)`

Contains a list of properties which are capable of generating
unsolicited value updates. This list can be used when populating
`PROP_UNSOL_UPDATE_FILTER` to disable all unsolicited property
updates.

This property is intended to effectively behave as a constant
for a given NCP firmware.

Note that not all properties that support unsolicited updates need to
be listed here. Scan results, for example, are only generated due to
direct action on the part of the host, so those properties **MUST NOT**
not be included in this list.

The value of this property **MAY** be different across available
NLIs.

## Stream Properties {#prop-stream}

### PROP 112: PROP_STREAM_DEBUG {#prop-stream-debug}

* Type: Read-Only-Stream
* Packed-Encoding: `D`

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
* Packed-Encoding: `dD`

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
MD_RSSI  | (dBm) RSSI                   | `c` int8   | 1     | -128
MD_NOISE | (dBm) Noise floor            | `c` int8   | 1     | -128
MD_FLAG  | Flags (defined below)        | `S` uint16 | 2     |
MD_PHY   | PHY-specific data            | `d` data   | >=2   |
MD_VEND  | Vendor-specific data         | `d` data   | >=2   |

The following fields are ignored by the NCP for packets sent to it from
the host:

* MD_NOISE
* MD_FLAG

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
* Packed-Encoding: `dD`

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

### PROP 115: PROP_STREAM_NET_INSECURE {#prop-stream-net-insecure}

* Type: Read-Write-Stream
* Packed-Encoding: `dD`

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

