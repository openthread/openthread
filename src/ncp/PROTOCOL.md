Spinel Host Controller Interface
================================

Updated: 2016-08-17

Written by: Robert Quattlebaum <rquattle@nestlabs.com>

THIS DOCUMENT IS A WORK IN PROGRESS AND SUBJECT TO CHANGE.
See [`spinel.h`](./spinel.h) for additional protocol details.

Copyright (c) 2016 Nest Labs, All Rights Reserved

## Abstract ##

This document describes a general management protocol for enabling a host
device to communicate with and manage a network co-processor(NCP).

While initially designed to support Thread-based NCPs, the NCP protocol
has been designed with a layered approach that allows it to be easily
adapted to other network protocols.

## 0. Table of Contents ##

*   [Abstract](#abstract)
*   [0. Table of Contents](#0-table-of-contents)
*   [1. Definitions](#1-definitions)
*   [2. Introduction](#2-introduction)
*   [3. Frame Format](#3-frame-format)
    *   [3.1. Header Format](#31-header-format)
        *   [3.1.1. Flag](#311-flag)
        *   [3.1.2. Interface Identifier (IID)](#312-interface-identifier-iid)
        *   [3.1.3. Transaction Identifier (TID)](#313-transaction-identifier-tid)
        *   [3.2. Command Identifier (CMD)](#32-command-identifier-cmd)
        *   [3.3. Command Payload (Optional)](#33-command-payload-optional)
*   [4. Commands](#4-commands)
*   [5. General Properties](#5-general-properties)
*   [6. Status Codes](#6-status-codes)
*   [7. Data Packing](#7-data-packing)
    *   [7.1 Primitive Types](#71-primitive-types)
    *   [7.2 Packed Unsigned Integer](#72-packed-unsigned-integer)
    *   [7.3 Data Blobs](#73-data-blobs)
    *   [7.4 Structured Data](#74-structured-data)
    *   [7.5 Arrays](#75-arrays)
*   [A. Framing Protocol](#a-framing-protocol)
    *   [A.1. UART Recommendations](#a1-uart-recommendations)
        *   [A.1.1. HDLC-Lite](#a11-hdlc-lite)
    *   [A.2. SPI Recommendations](#a2-spi-recommendations)
        *   [A.2.1 SPI Framing Protocol](#a21-spi-framing-protocol)
    *   [A.3. I²C Recommendations](#a3-ic-recommendations)
    *   [A.4. Native USB Recommendations](#a4-native-usb-recommendations)
*   [B. Feature: Network Save](#b-feature-network-save)
    *   [B.1. Commands](#b1-commands)
*   [C. Feature: Host Buffer Offload](#c-feature-host-buffer-offload)
    *   [C.1. Commands](#c1-commands)
    *   [C.2. Properties](#c2-properties)
*   [D. Protocol: Thread](#d-protocol-thread)
    *   [D.1. PHY Properties](#d1-phy-properties)
    *   [D.2. MAC Properties](#d2-mac-properties)
    *   [D.3. NET Properties](#d3-net-properties)
    *   [D.4. THREAD Properties](#d4-thread-properties)
    *   [D.5. IPv6 Properties](#d5-ipv6-properties)

## 1. Definitions ##

 *  **NCP**: Network Control Processor
 *  **Host**: Computer or Micro-controller which controls the NCP.
 *  **TID**: Transaction Identifier (0-15)
 *  **IID**: Interface Identifier (0-3)

## 2. Introduction ##

This Network Co-Processor (NCP) protocol was designed to enable a host
device to communicate with and manage a NCP while also achieving the
following goals:

 *  Adopt a layered approach to the protocol design, allowing future
    support for other network protocols.
 *  Minimize the number of required commands/methods by providing a
    rich, property-based API.
 *  Support NCPs capable of being connected to more than one network
    at a time.
 *  Gracefully handle the addition of new features and capabilities
    without necessarily breaking backward compatibility.
 *  Be as minimal and light-weight as possible without unnecessarily
    sacrificing flexibility.

On top of this core framework, we define the properties and commands
to enable various features and network protocols.


## 3. Frame Format ##

A frame is defined simply as the concatenation of

 *  A header byte
 *  A command (up to three bytes, see *section 7.2* for format)
 *  An optional command payload

    FRAME = HEADER CMD [CMD_PAYLOAD]

Octets: |    1   | 1-3 |       *n*
--------|--------|-----|-----------------
Fields: | HEADER | CMD | *[CMD_PAYLOAD]*


### 3.1. Header Format ###

The header byte is broken down as follows:

      0   1   2   3   4   5   6   7
    +---+---+---+---+---+---+---+---+
    |  FLG  |  IID  |      TID      |
    +---+---+---+---+---+---+---+---+

#### 3.1.1. Flag ####

The flag field of the header byte (`FLG`) is always set to the value
two (or `10` in binary). Any frame received with these bits set to
any other value else MUST NOT be considered a Spinel frame.

This convention allows Spinel to be line compatible with BTLE HCI. By
defining the first two bit in this way we can disambiguate between
Spinel frames and HCI frames (which always start with either `0x01`
or `0x04`) without any additional framing overhead.

#### 3.1.2. Interface Identifier (IID) ####

The Interface Identifier (IID) is a number between 0 and 3 which
identifies which subinterface the frame is intended for. This allows
the protocol to support connecting to more than one network at once.
The first subinterface (0) is considered the primary subinterface and
MUST be supported. Support for all other subinterfaces is OPTIONAL.

#### 3.1.3. Transaction Identifier (TID) ####

The least significant bits of the header represent the Transaction
Identifier(TID). The TID is used for correlating responses to the
commands which generated them.

When a command is sent from the host, any reply to that command sent
by the NCP will use the same value for the TID. When the host receives
a frame that matches the TID of the command it sent, it can easily
recognize that frame as the actual response to that command.

The TID value of zero (0) is used for commands to which a correlated
response is not expected or needed, such as for unsolicited update
commands sent to the host from the NCP.

#### 3.2. Command Identifier (CMD) ####

The command identifier is a 21-bit unsigned integer encoded in up to
three bytes using the packed unsigned integer format described in
*section 7.2*. This encoding allows for up to 2,097,152 individual
commands, with the first 127 commands represented as a single byte.
Command identifiers larger than 2,097,151 are explicitly forbidden.

CID Range             | Description
----------------------|------------------
0 - 63                | Reserved for core commands
64 - 15,359           | *UNALLOCATED*
15,360 - 16,383       | Vendor-specific
16,384 - 1,999,999    | *UNALLOCATED*
2,000,000 - 2,097,151 | Experimental use only

#### 3.3. Command Payload (Optional) ####

Depending on the semantics of the command in question, a payload MAY
be included in the frame. The exact composition and length of the
payload is defined by the command identifier.

## 4. Commands

* CMD 0: (Host->NCP) `CMD_NOOP`
* CMD 1: (Host->NCP) `CMD_RESET`
* CMD 2: (Host->NCP) `CMD_PROP_VALUE_GET`
* CMD 3: (Host->NCP) `CMD_PROP_VALUE_SET`
* CMD 4: (Host->NCP) `CMD_PROP_VALUE_INSERT`
* CMD 5: (Host->NCP) `CMD_PROP_VALUE_REMOVE`
* CMD 6: (NCP->Host) `CMD_PROP_VALUE_IS`
* CMD 7: (NCP->Host) `CMD_PROP_VALUE_INSERTED`
* CMD 8: (NCP->Host) `CMD_PROP_VALUE_REMOVED`
* CMD 9: (Host->NCP) `CMD_NET_SAVE` (See section B.1.1.)
* CMD 10: (Host->NCP) `CMD_NET_CLEAR`  (See section B.1.2.)
* CMD 11: (Host->NCP) `CMD_NET_RECALL`  (See section B.1.3.)
* CMD 12: (NCP->Host) `CMD_HBO_OFFLOAD` (See section C.1.1.)
* CMD 13: (NCP->Host) `CMD_HBO_RECLAIM` (See section C.1.2.)
* CMD 14: (NCP->Host) `CMD_HBO_DROP` (See section C.1.3.)
* CMD 15: (Host->NCP) `CMD_HBO_OFFLOADED` (See section C.1.4.)
* CMD 16: (Host->NCP) `CMD_HBO_RECLAIMED` (See section C.1.5.)
* CMD 17: (Host->NCP) `CMD_HBO_DROPPED` (See section C.1.6.)

### 4.1. CMD 0: (Host->NCP) `CMD_NOOP`

Octets: |    1   |     1
--------|--------|----------
Fields: | HEADER | CMD_NOOP

No-Operation command. Induces the NCP to send a success status back to
the host. This is primarily used for liveliness checks.

The command payload for this command SHOULD be empty. The receiver
MUST ignore any non-empty command payload.

There is no error condition for this command.



### 4.2. CMD 1: (Host->NCP) `CMD_RESET`

Octets: |    1   |     1
--------|--------|----------
Fields: | HEADER | CMD_RESET

Reset NCP command. Causes the NCP to perform a software reset. Due to
the nature of this command, the TID is ignored. The host should
instead wait for a `CMD_PROP_VALUE_IS` command from the NCP indicating
`PROP_LAST_STATUS` has been set to `STATUS_RESET_SOFTWARE`.

The command payload for this command SHOULD be empty. The receiver
MUST ignore any non-empty command payload.

If an error occurs, the value of `PROP_LAST_STATUS` will be emitted
instead with the value set to the generated status code for the error.



### 4.3. CMD 2: (Host->NCP) `CMD_PROP_VALUE_GET`

Octets: |    1   |          1         |   1-3
--------|--------|--------------------|---------
Fields: | HEADER | CMD_PROP_VALUE_GET | PROP_ID

Get property value command. Causes the NCP to emit a
`CMD_PROP_VALUE_IS` command for the given property identifier.

The payload for this command is the property identifier encoded in the
packed unsigned integer format described in *section 7.2*.

If an error occurs, the value of `PROP_LAST_STATUS` will be emitted
instead with the value set to the generated status code for the error.



### 4.4. CMD 3: (Host->NCP) `CMD_PROP_VALUE_SET`

Octets: |    1   |          1         |   1-3   |    *n*
--------|--------|--------------------|---------|------------
Fields: | HEADER | CMD_PROP_VALUE_SET | PROP_ID | PROP_VALUE

Set property value command. Instructs the NCP to set the given
property to the specific given value, replacing any previous value.

The payload for this command is the property identifier encoded in the
packed unsigned integer format described in *section 7.2*, followed by
the property value. The exact format of the property value is defined
by the property.

If an error occurs, the value of `PROP_LAST_STATUS` will be emitted
with the value set to the generated status code for the error.



### 4.5. CMD 4: (Host->NCP) `CMD_PROP_VALUE_INSERT`

Octets: |    1   |          1            |   1-3   |    *n*
--------|--------|-----------------------|---------|------------
Fields: | HEADER | CMD_PROP_VALUE_INSERT | PROP_ID | VALUE_TO_INSERT

Insert value into property command. Instructs the NCP to insert the
given value into a list-oriented property, without removing other
items in the list. The resulting order of items in the list is defined
by the individual property being operated on.

The payload for this command is the property identifier encoded in the
packed unsigned integer format described in *section 7.2*, followed by
the value to be inserted. The exact format of the value is defined by
the property.

If an error occurs, the value of `PROP_LAST_STATUS` will be emitted
with the value set to the generated status code for the error.



### 4.6. CMD 5: (Host->NCP) `CMD_PROP_VALUE_REMOVE`

Octets: |    1   |          1            |   1-3   |    *n*
--------|--------|-----------------------|---------|------------
Fields: | HEADER | CMD_PROP_VALUE_REMOVE | PROP_ID | VALUE_TO_REMOVE

Remove value from property command. Instructs the NCP to remove the
given value from a list-oriented property, without affecting other
items in the list. The resulting order of items in the list is defined
by the individual property being operated on.

Note that this command operates *by value*, not by index!

The payload for this command is the property identifier encoded in the
packed unsigned integer format described in *section 7.2*, followed by
the value to be removed. The exact format of the value is defined by
the property.

If an error occurs, the value of `PROP_LAST_STATUS` will be emitted
with the value set to the generated status code for the error.


### 4.7. CMD 6: (NCP->Host) `CMD_PROP_VALUE_IS`

Octets: |    1   |          1        |   1-3   |    *n*
--------|--------|-------------------|---------|------------
Fields: | HEADER | CMD_PROP_VALUE_IS | PROP_ID | PROP_VALUE

Property value notification command. This command can be sent by the
NCP in response to a previous command from the host, or it can be sent
by the NCP in an unsolicited fashion to notify the host of various
state changes asynchronously.

The payload for this command is the property identifier encoded in the
packed unsigned integer format described in *section 7.2*, followed by
the current value of the given property.



### 4.8. CMD 7: (NCP->Host) `CMD_PROP_VALUE_INSERTED`

Octets: |    1   |            1            |   1-3   |    *n*
--------|--------|-------------------------|---------|------------
Fields: | HEADER | CMD_PROP_VALUE_INSERTED | PROP_ID | PROP_VALUE

Property value insertion notification command. This command can be
sent by the NCP in response to the `CMD_PROP_VALUE_INSERT` command, or
it can be sent by the NCP in an unsolicited fashion to notify the host
of various state changes asynchronously.

The payload for this command is the property identifier encoded in the
packed unsigned integer format described in *section 7.2*, followed by
the value that was inserted into the given property.

The resulting order of items in the list is defined by the given
property.

### 4.9. CMD 8: (NCP->Host) `CMD_PROP_VALUE_REMOVED`

Octets: |    1   |            1           |   1-3   |    *n*
--------|--------|------------------------|---------|------------
Fields: | HEADER | CMD_PROP_VALUE_REMOVED | PROP_ID | PROP_VALUE

Property value removal notification command. This command can be sent
by the NCP in response to the `CMD_PROP_VALUE_REMOVE` command, or it
can be sent by the NCP in an unsolicited fashion to notify the host of
various state changes asynchronously.

Note that this command operates *by value*, not by index!

The payload for this command is the property identifier encoded in the
packed unsigned integer format described in *section 7.2*, followed by
the value that was removed from the given property.

The resulting order of items in the list is defined by the given
property.






## 5. General Properties

While the majority of the properties that allow the configuration
of network connectivity are network protocol specific, there are
several properties that are required in all implementations.

* 0: `PROP_LAST_STATUS`
* 1: `PROP_PROTOCOL_VERSION`
* 2: `PROP_NCP_VERSION`
* 3: `PROP_INTERFACE_TYPE`
* 4: `PROP_VENDOR_ID`
* 5: `PROP_CAPS`
* 6: `PROP_INTERFACE_COUNT`
* 7: `PROP_POWER_STATE`
* 8: `PROP_HWADDR`
* 9: `PROP_LOCK`
* 10: `PROP_HBO_MEM_MAX` (See section C.2.1.)
* 11: `PROP_HBO_BLOCK_MAX` (See section C.2.2.)
* 112: `PROP_STREAM_DEBUG`
* 113: `PROP_STREAM_RAW`
* 114: `PROP_STREAM_NET`
* 115: `PROP_STREAM_NET_INSECURE`

Additionally, future property allocations SHALL be made from the
following allocation plan:

Property ID Range     | Description
----------------------|------------------
0 - 127               | Reserved for frequently-used core properties
128 - 15,359          | *UNALLOCATED*
15,360 - 16,383       | Vendor-specific
16,384 - 1,999,999    | *UNALLOCATED*
2,000,000 - 2,097,151 | Experimental use only

For an explanation of the data format encoding shorthand used
throughout this section, see section 7.1.

### 5.1. PROP 0: `PROP_LAST_STATUS`

* Type: Read-Only
* Encoding: `i`

Octets: |       1-3
--------|------------------
Fields: | PROP_LAST_STATUS

Describes the status of the last operation. Encoded as a packed
unsigned integer.

This property is emitted often to indicate the result status of
pretty much any Host-to-NCP operation.

It is emitted automatically at NCP startup with a value indicating
the reset reason.

See section 6 for the complete list of status codes.

### 5.2. PROP 1: `PROP_PROTOCOL_VERSION`

* Type: Read-Only
* Encoding: `ii`

Octets: |       1-3      |      1-3
--------|----------------|---------------
Fields: |  MAJOR_VERSION | MINOR_VERSION

Describes the protocol version information. This property contains
four fields, each encoded as a packed unsigned integer:

 *  Major Version Number
 *  Minor Version Number


#### 5.2.1 Major Version Number ####

The major version number is used to identify large and incompatible
differences between protocol versions.

The host MUST enter a FAULT state if it does not explicitly support
the given major version number.

#### 5.2.2 Minor Version Number ####

The minor version number is used to identify small but otherwise
compatible differences between protocol versions. A mismatch between
the advertised minor version number and the minor version that is
supported by the host SHOULD NOT be fatal to the operation of the
host.

### 5.3. PROP 2: `PROP_NCP_VERSION`

* Type: Read-Only
* Packed-Encoding: `U`

Octets: |       *n*
--------|-------------------
Fields: | `NCP_VESION_STRING`

Contains a string which describes the firmware currently running on
the NCP. Encoded as a zero-terminated UTF-8 string.

The format of the string is not strictly defined, but it is intended
to present similarly to the "User-Agent" string from HTTP. The
RECOMMENDED format of the string is as follows:

    <STACK-NAME>/<STACK-VERSION>[<BUILD_INFO>][; <OTHER_INFO>]; <BUILD_DATE_AND_TIME>

Examples:

 *  `OpenThread/1.0d26-25-gb684c7f; DEBUG; May 9 2016 18:22:04`
 *  `ConnectIP/2.0b125 s1 ALPHA; Sept 24 2015 20:49:19`

### 5.4. PROP 3: `PROP_INTERFACE_TYPE`

* Type: Read-Only
* Encoding: `i`

Octets: |       1-3
--------|----------------
Fields: | INTERFACE_TYPE

This integer identifies what the network protocol for this NCP.
Currently defined values are:

 *  1: ZigBee
 *  2: ZigBeeIP
 *  3: Thread

The host MUST enter a FAULT state if it does not recognize the
protocol given by the NCP.

### 5.5. PROP 4: `PROP_INTERFACE_VENDOR_ID`

* Type: Read-Only
* Encoding: `i`

Octets: |       1-3
--------|----------------
Fields: | VENDOR_ID

Vendor identifier.

### 5.6. PROP 2: `PROP_CAPS`

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
 * 3: `CAP_HBO`: Host Block Offload (See Section C.)
 * 4: `CAP_POWER_SAVE`
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

Additionally, future capability allocations SHALL be made from the
following allocation plan:

Capability Range      | Description
----------------------|------------------
0 - 127               | Reserved for core capabilities
128 - 15,359          | *UNALLOCATED*
15,360 - 16,383       | Vendor-specific
16,384 - 1,999,999    | *UNALLOCATED*
2,000,000 - 2,097,151 | Experimental use only


### 5.7. PROP 4: `PROP_INTERFACE_COUNT`

* Type: Read-Only
* Packed-Encoding: `C`

Octets: |       1
--------|-----------------
Fields: | `INTERFACE_COUNT`

Describes the number of concurrent interfaces supported by this NCP.
Since the concurrent interface mechanism is still TBD, this value MUST
always be one.

This value is encoded as an unsigned 8-bit integer.

### 5.8. PROP 5: `PROP_POWER_STATE`

* Type: Read-Write
* Packed-Encoding: `C`

Octets: |        1
--------|------------------
Fields: | `PROP_POWER_STATE`

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

### 5.9. PROP 6: `PROP_HWADDR`

* Type: Read-Only*
* Packed-Encoding: `E`

Octets: |    8
--------|------------
Fields: | PROP_HWADDR

The static EUI64 address of the device. This value is read-only, but
may be writable under certain vendor-defined circumstances.

### 5.10. PROP 6: `PROP_LOCK`

* Type: Read-Write
* Packed-Encoding: `b`

Octets: |    1
--------|------------
Fields: | PROP_LOCK

Property lock. Used for grouping changes to several properties to
take effect at once, or to temporarily prevent the automatic updating
of property values. When this property is set, the execution of the
NCP is effectively frozen until it is cleared.

This property is only supported if the `CAP_LOCK` capability is present.

### 5.11. PROP 112: `PROP_STREAM_DEBUG`

* Type: Read-Only-Stream
* Packed-Encoding: `U`

Octets: |    *n*
--------|------------
Fields: | UTF8_DATA

This property is a streaming property, meaning that you cannot explicitly
fetch the value of this property. The stream provides human-readable debugging
output which may be displayed in the host logs.

To receive the debugging stream, you wait for `CMD_PROP_VALUE_IS` commands for
this property from the NCP.

### 5.12. PROP 113: `PROP_STREAM_RAW`

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

Implementations may optionally support the ability to transmit arbitrary
raw packets. If this capability is supported, you may call `CMD_PROP_VALUE_SET`
on this property with the value of the raw packet.

Any data past the end of `FRAME_DATA_LEN` is considered metadata. The format
of the metadata is defined by the associated MAC and PHY being used.

### 5.13. PROP 114: `PROP_STREAM_NET`

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

Any data past the end of `FRAME_DATA_LEN` is considered metadata. The format
of the metadata is defined by the associated network protocol.

### 5.14. PROP 114: `PROP_STREAM_NET_INSECURE`

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

Any data past the end of `FRAME_DATA_LEN` is considered metadata. The format
of the metadata is defined by the associated network protocol.


## 6. Status Codes

 *  0: `STATUS_OK`: Operation has completed successfully.
 *  1: `STATUS_FAILURE`: Operation has failed for some undefined
    reason.
 *  2: `STATUS_UNIMPLEMENTED`: The given operation has not been implemented.
 *  3: `STATUS_INVALID_ARGUMENT`: An argument to the given operation is invalid.
 *  4: `STATUS_INVALID_STATE` : The given operation is invalid for the current
    state of the device.
 *  5: `STATUS_INVALID_COMMAND`: The given command is not recognized.
 *  6: `STATUS_INVALID_INTERFACE`: The given Spinel interface is not supported.
 *  7: `STATUS_INTERNAL_ERROR`: An internal runtime error has occured.
 *  8: `STATUS_SECURITY_ERROR`: A security or authentication error has occured.
 *  9: `STATUS_PARSE_ERROR`: An error has occured while parsing the command.
 *  10: `STATUS_IN_PROGRESS`: The operation is in progress and will be
    completed asynchronously.
 *  11: `STATUS_NOMEM`: The operation has been prevented due to memory
    pressure.
 *  12: `STATUS_BUSY`: The device is currently performing a mutually exclusive
    operation.
 *  13: `STATUS_PROPERTY_NOT_FOUND`: The given property is not recognized.
 *  14: `STATUS_PACKET_DROPPED`: The packet was dropped.
 *  15: `STATUS_EMPTY`: The result of the operation is empty.
 *  16: `STATUS_CMD_TOO_BIG`: The command was too large to fit in the internal
    buffer.
 *  17: `STATUS_NO_ACK`: The packet was not acknowledged.
 *  18: `STATUS_CCA_FAILURE`: The packet was not sent due to a CCA failure.
 *  19-111: *RESERVED*
 *  112-127: Reset Causes
     *  112: `STATUS_RESET_POWER_ON`
     *  113: `STATUS_RESET_EXTERNAL`
     *  114: `STATUS_RESET_SOFTWARE`
     *  115: `STATUS_RESET_FAULT`
     *  116: `STATUS_RESET_CRASH`
     *  117: `STATUS_RESET_ASSERT`
     *  118: `STATUS_RESET_OTHER`
     *  119: `STATUS_RESET_UNKNOWN`
     *  120: `STATUS_RESET_WATCHDOG`
     *  121-127: *RESERVED-RESET-CODES*
 *  128 - 15,359: *UNALLOCATED*
 *  15,360 - 16,383: Vendor-specific
 *  16,384 - 1,999,999: *UNALLOCATED*
 *  2,000,000 - 2,097,151: Experimental Use Only (MUST NEVER be used
    in production!)

## 7. Data Packing

Data serialization for properties is performed using a light-weight
data packing format which was loosely inspired by D-Bus. The format of
a serialization is defined by a specially formatted string.

Goals:

 *  Be very lightweight and favor direct representation of values.
 *  Use an easily readable and memorable format string.
 *  Support lists and structures.
 *  Allow properties to be appended to structures while maintaining
    backward compatibility.

Each primitive datatype has an ASCII character associated with it.
Structures can be represented as strings of these characters. For
example:

 *  `"C"`: A single unsigned byte.
 *  `"C6U"`: A single unsigned byte, followed by a 128-bit IPv6
    address, followed by a zero-terminated UTF8 string.
 *  `"A(6)"`: An array of IPv6 addresses

In each case, the data is represented exactly as described. For
example, an array of 10 IPv6 address is stored as 160 bytes.

### 7.1 Primitive Types

 *  0: `DATATYPE_NULL`
 *  `'.'`: `DATATYPE_VOID`: Empty data type. Used internally.
 *  `'b'`: `DATATYPE_BOOL`: Boolean value. Encoded in 8-bits as either
    `0x00` or `0x01`. All other values are illegal.
 *  `'C'`: `DATATYPE_UINT8`: Unsigned 8-bit integer.
 *  `'c'`: `DATATYPE_INT8`: Signed 8-bit integer.
 *  `'S'`: `DATATYPE_UINT16`: Unsigned 16-bit integer. (Little-endian)
 *  `'s'`: `DATATYPE_INT16`: Signed 16-bit integer. (Little-endian)
 *  `'L'`: `DATATYPE_UINT32`: Unsigned 32-bit integer. (Little-endian)
 *  `'l'`: `DATATYPE_INT32`: Signed 32-bit integer. (Little-endian)
 *  `'i'`: `DATATYPE_UINT_PACKED`: Packed Unsigned Integer. (See
    section 7.2)
 *  `'6'`: `DATATYPE_IPv6ADDR`: IPv6 Address. (Big-endian)
 *  `'E'`: `DATATYPE_EUI64`: EUI-64 Address. (Big-endian)
 *  `'e'`: `DATATYPE_EUI48`: EUI-48 Address. (Big-endian)
 *  `'D'`: `DATATYPE_DATA`: Arbitrary Data. (See section 7.3)
 *  `'U'`: `DATATYPE_UTF8`: Zero-terminated UTF8-encoded string.
 *  `'T'`: `DATATYPE_STRUCT`: Structured datatype. Compound type. (See
    section 7.4)
 *  `'A'`: `DATATYPE_ARRAY`: Array of datatypes. Compound type. (See
    section 7.5)

### 7.2 Packed Unsigned Integer

For certain types of integers, such command or property identifiers,
usually have a value on the wire that is less than 127. However, in
order to not preclude the use of values larger than 256, we would need
to add an extra byte. Doing this would add an extra byte to the vast
majority of instances, which can add up in terms of bandwidth.

The packed unsigned integer format is inspired by the encoding scheme
in UTF8 identical to the unsigned integer format in EXI.

For all values less than 127, the packed form of the number is simply
a single byte which directly represents the number. For values larger
than 127, the following process is used to encode the value:

1.  The unsigned integer is broken up into *n* 7-bit chunks and placed
    into *n* octets, leaving the most significant bit of each octet
    unused.
2.  Order the octets from least-significant to most-significant.
    (Little-endian)
3.  Clear the most significant bit of the most significant octet. Set
    the least significant bit on all other octets.

Where *n* is the smallest number of 7-bit chunks you can use to
represent the given value.

Take the value 1337, for example:

    1337 => 0x0539
         => [39 0A]
         => [B9 0A]

To decode the value, you collect the 7-bit chunks until you find an
octet with the most significant bit clear.

### 7.3 Data Blobs

Data blobs are special datatypes in that the data that they contain
does not inherently define the size of the data. This means that if
the length of the data blob isn't *implied*, then the length of the
blob must be prepended as a packed unsigned integer.

The length of a data blob is *implied* only when it is the last
datatype in a given buffer. This works because we already know the
size of the buffer, and the length of the data is simply the rest of
the size of the buffer.

For example, let's say we have a buffer that is encoded with the
datatype signature of `CLLD`. In this case, it is pretty easy to tell
where the start and end of the data blob is: the start is 9 bytes from
the start of the buffer, and its length is the length of the buffer
minus 9. (9 is the number of bytes taken up by a byte and two longs)

However, things are a little different with `CLDL`. Since our data
blob is no longer the last item in the signature, the length must be
prepended.

If you are a little confused, keep reading. This theme comes up in a a
few different ways in the following sections.

When a length is prepended, the length is encoded as a little-endian
unsigned 16-bit integer.

### 7.4 Structured Data

The structured data type is a way of bundling together a bunch of data
into a single data structure. This may at first seem useless. What is
the difference between `T(Cii)` and just `Cii`? The answer is, in that
particular case, nothing: they are stored in exactly the same way.

However, one case where the structure datatype makes a difference is
when you compare `T(Cii)L` to `CiiL`: they end up being represented
entirely differently. This is because the structured data type follows
the exact same semantics as the data blob type: if it isn't the last
datatype in a signature, *it must be prepended with a length*. This is
useful because it allows for new datatypes to be appended to the
structure's signature while remaining *backward parsing
compatibility*.

More explicitly, if you take data that was encoded with `T(Cii6)L`,
you can still decode it as `T(Cii)L`.

Let's take, for example, the property `PROP_IPv6_ADDR_TABLE`.
Conceptually it is just a list of IPv6 addresses, so we can encode it
as `A(6c)`. However, if we ever want to associate more data with the
type (like flags), we break our backward compatibility if we add
another member and use `A(6cC)`. To allow for data to be added without
breaking backward compatibility, we use the structured data type from
the start: `A(T(6c))`. Then when we add a new member to the structure
(`A(T(6cC))`), we don't break backward compatibility.

It's also worth noting that `T(Cii)L` also parses as `DL`. You could
then take the resultant data blob and parse it as `Cii`.

When a length is prepended, the length is encoded as a little-endian
unsigned 16-bit integer.

### 7.5 Arrays

An array is simply a concatenated set of *n* data encodings. For example,
the type `A(6)` is simply a list of IPv6 addresses---one after the other.

Just like the data blob type and the structured data type, the length
of the entire array must be prepended *unless* the array is the last
type in a given signature. Thus, `A(C)` (An array of unsigned bytes)
encodes identically to `D`.

When a length is prepended, the length is encoded as a little-endian
unsigned 16-bit integer.

## A. Framing Protocol

Since this NCP protocol is defined independently of the physical
transport or framing, any number of transports and framing protocols
could be used successfully. However, in the interests of compatibility,
this document provides some recommendations.

### A.1. UART Recommendations ###

The recommended default UART settings are:

* Bit rate:     115200
* Start bits:   1
* Data bits:    8
* Stop bits:    1
* Parity:       None
* Flow Control: Hardware

These values may be adjusted depending on the individual needs of
the application or product, but some sort of flow control MUST be used.
Hardware flow control is preferred over software flow control. In the
absence of hardware flow control, software flow control (XON/XOFF) MUST
be used instead.

We also recommend an arduino-style hardware reset, where the DTR
signal is coupled to the `R̅E̅S̅` pin through a 0.01µF capacitor. This
causes the NCP to automatically reset whenever the serial port is
opened. At the very least we recommend dedicating one of your host
pins to controlling the `R̅E̅S̅` pin on the NCP, so that you can
easily perform a hardware reset if necessary.

#### A.1.1. HDLC-Lite ####

*HDLC-Lite* is the recommended framing protocol for transmitting
Spinel frames over a UART. HDLC-Lite consists of only the framing,
escaping, and CRC parts of the larger HDLC protocol---all other parts
of HDLC are omitted. This protocol was chosen because it works well
with software flow control and is widely implemented.

To transmit a frame with HDLC-lite, the 16-bit CRC must first be
appended to the frame. The CRC function is defined to be CRC-16/CCITT,
otherwise known as the [KERMIT CRC][].

[KERMIT CRC]: http://reveng.sourceforge.net/crc-catalogue/16.htm#crc.cat.kermit

Individual frames are terminated with a frame delimiter octet called
the 'flag' octet (`0x7E`).

The following octets values are considered *special* and should be
escaped when present in data frames:

Octet Value | Description  
------------|-----------------------  
       0x7E | Frame Delimiter (Flag)  
       0x7D | Escape Byte  
       0x11 | XON  
       0x13 | XOFF  
       0xF8 | Vendor-Specific  

When present in a data frame, these octet values are escaped by
prepending the escape octet (`0x7D`) and XORing the value with `0x20`.

When receiving a frame, the CRC must be verified after the frame is
unescaped. If the CRC value does not match what is calculated for the
frame data, the frame MUST be discarded. The implementation MAY
indicate the failure to higher levels to handle as they see fit, but
MUST NOT attempt to process the deceived frame.

Consecutive flag octets are entirely legal and MUST NOT be treated as
a framing error. Consecutive flag octets MAY be used as a way to wake
up a sleeping NCP.

When first establishing a connection to the NCP, it is customary to
send one or more flag octets to ensure that any previously received
data is discarded.

### A.2. SPI Recommendations ###

We RECOMMEND the use of the following standard SPI signals:

*   `C̅S̅`:   (Host-to-NCP) Chip Select
*   `CLK`:  (Host-to-NCP) Clock
*   `MOSI`: Master-Output/Slave-Input
*   `MISO`: Master-Input/Slave-Output
*   `I̅N̅T̅`:  (NCP-to-Host) Host Interrupt
*   `R̅E̅S̅`:  (Host-to-NCP) NCP Hardware Reset

The `I̅N̅T̅` signal is used by the NCP to indicate to the host that
the NCP has frames pending to send to it. When asserted, the host
SHOULD initiate a SPI transaction in a timely manner.

We RECOMMEND the following SPI properties:

*   `C̅S̅` is active low.
*   `CLK` is active high.
*   `CLK` speed is larger than 500 kHz.
*   Data is valid on leading edge of `CLK`.
*   Data is sent in multiples of 8-bits (octets).
*   Octets are sent most-significant bit first.

This recommended configuration may be adjusted depending on the
individual needs of the application or product.

#### A.2.1 SPI Framing Protocol ####

Each SPI frame starts with a 5-byte frame header:

Octets: |  1  |    2     |     2  
--------|-----|----------|----------  
Fields: | HDR | RECV_LEN | DATA_LEN  

*   `HDR`: The first byte is the header byte (defined below)
*   `RECV_LEN`: The second and third bytes indicate the largest frame
    size that that device is ready to receive. If zero, then the other
    device must not send any data. (Little endian)
*   `DATA_LEN`: The fourth and fifth bytes indicate the size of the
    pending data frame to be sent to the other device. If this value
    is equal-to or less-than the number of bytes that the other device
    is willing to receive, then the data of the frame is immediately
    after the header. (Little Endian)

The `HDR` byte is defined as:

      0   1   2   3   4   5   6   7
    +---+---+---+---+---+---+---+---+
    |RST|CRC|    RESERVED   |PATTERN|
    +---+---+---+---+---+---+---+---+

*   `RST`: This bit is set when that device has been reset since the
    last time `C̅S̅` was asserted.
*   `CRC`: This bit is set when that device supports writing a 16-bit
    CRC at the end of the data. This CRC is NOT included in DATA_LEN.
*   `RESERVED`: These bits are all reserved for future used. They
    MUST be cleared to zero and MUST be ignored if set.
*   `PATTERN`: These bits are set to a fixed value to help distinguish
    valid SPI frames from garbage (by explicitly making `0xFF` and `0x00`
    invalid values). Bit 6 MUST be set to be one and bit 7 MUST be
    cleared (0). A frame received that has any other values for these bits
    MUST be dropped.

Prior to a sending or receiving a frame, the master SHOULD send a
5-octet frame with zeros for both the max receive frame size and the
the contained frame length. This will induce the slave device to
indicate the length of the frame it wants to send (if any) and
indicate the largest frame it is capable of receiving at the moment.
This allows the master to calculate the size of the next transaction.

This protocol can be used either unidirectionally or bidirectionally,
determined by the behavior of the master and the slave.

If the the master notices `PATTERN` is not set correctly, the master
should consider the transaction to have failed and try again after 10
milliseconds, retrying up to 200 times. After unsuccessfully trying
200 times in a row, the master MAY take appropriate remedial action
(like a NCP hardware reset, or indicating a communication failure to a
user interface).

### A.3. I²C Recommendations ###

TBD

### A.4. Native USB Recommendations ###

TBD

## B. Feature: Network Save

The network save feature is an optional NCP capability that, when
present, allows the host to save and recall network credentials and
state to and from nonvolatile storage.

The presence of this feature can be detected by checking for the
presence of the `CAP_NET_SAVE` capability in `PROP_CAPS`.

### B.1. Commands

#### B.1.1. CMD 9: (Host->NCP) `CMD_NET_SAVE`

Octets: |    1   |      1
--------|--------|--------------
Fields: | HEADER | CMD_NET_SAVE

Save network state command. Saves any current network credentials and
state necessary to reconnect to the current network to non-volatile
memory.

This operation affects non-volatile memory only. The current network
information stored in volatile memory is unaffected.

The response to this command is always a `CMD_PROP_VALUE_IS` for
`PROP_LAST_STATUS`, indicating the result of the operation.

This command is only available if the `CAP_NET_SAVE` capability is
set.



#### B.1.2. CMD 10: (Host->NCP) `CMD_NET_CLEAR`

Octets: |    1   |      1
--------|--------|---------------
Fields: | HEADER | CMD_NET_CLEAR

Clear saved network state command. Clears any previously saved network
credentials and state previously stored by `CMD_NET_SAVE` from
non-volatile memory.

This operation affects non-volatile memory only. The current network
information stored in volatile memory is unaffected.

The response to this command is always a `CMD_PROP_VALUE_IS` for
`PROP_LAST_STATUS`, indicating the result of the operation.

This command is only available if the `CAP_NET_SAVE` capability is
set.



#### B.1.3. CMD 11: (Host->NCP) `CMD_NET_RECALL`

Octets: |    1   |      1
--------|--------|----------------
Fields: | HEADER | CMD_NET_RECALL

Recall saved network state command. Recalls any previously saved
network credentials and state previously stored by `CMD_NET_SAVE` from
non-volatile memory.

This command will typically generated several unsolicited property
updates as the network state is loaded. At the conclusion of loading,
the authoritative response to this command is always a
`CMD_PROP_VALUE_IS` for `PROP_LAST_STATUS`, indicating the result of
the operation.

This command is only available if the `CAP_NET_SAVE` capability is
set.


## C. Feature: Host Buffer Offload

The memory on an NCP may be much more limited than the memory on
the host processor. In such situations, it is sometimes useful
for the NCP to offload buffers to the host processor temporarily
so that it can perform other operations.

Host buffer offload is an optional NCP capability that, when
present, allows the NCP to store data buffers on the host processor
that can be recalled at a later time.

The presence of this feature can be detected by the host by
checking for the presence of the `CAP_HBO`
capability in `PROP_CAPS`.

### C.1. Commands

#### C.1.1. CMD 12: (NCP->Host) `CMD_HBO_OFFLOAD`

* Argument-Encoding: `LscD`
    * `OffloadId`: 32-bit unique block identifier
    * `Expiration`: In seconds-from-now
    * `Priority`: Critical, High, Medium, Low
    * `Data`: Data to offload

#### C.1.2. CMD 13: (NCP->Host) `CMD_HBO_RECLAIM`
 *  Argument-Encoding: `Lb`
     *  `OffloadId`: 32-bit unique block identifier
     *  `KeepAfterReclaim`: If not set to true, the block will be
        dropped by the host after it is sent to the NCP.

#### C.1.3. CMD 14: (NCP->Host) `CMD_HBO_DROP`

* Argument-Encoding: `L`
    * `OffloadId`: 32-bit unique block identifier

#### C.1.1. CMD 15: (Host->NCP) `CMD_HBO_OFFLOADED`

* Argument-Encoding: `Li`
    * `OffloadId`: 32-bit unique block identifier
    * `Status`: Status code for the result of the operation.

#### C.1.2. CMD 16: (Host->NCP) `CMD_HBO_RECLAIMED`

* Argument-Encoding: `LiD`
    * `OffloadId`: 32-bit unique block identifier
    * `Status`: Status code for the result of the operation.
    * `Data`: Data that was previously offloaded (if any)

#### C.1.3. CMD 17: (Host->NCP) `CMD_HBO_DROPPED`

* Argument-Encoding: `Li`
    * `OffloadId`: 32-bit unique block identifier
    * `Status`: Status code for the result of the operation.

### C.2. Properties

#### C.2.1. PROP 6: `PROP_HBO_MEM_MAX`

* Type: Read-Write
* Packed-Encoding: `L`

Octets: |       4
--------|-----------------
Fields: | `PROP_HBO_MEM_MAX`

Describes the number of bytes that may be offloaded from the NCP to
the host. Default value is zero, so this property must be set by the
host to a non-zero value before the NCP will begin offloading blocks.

This value is encoded as an unsigned 32-bit integer.

This property is only available if the `CAP_HBO`
capability is present in `PROP_CAPS`.

#### C.2.1. PROP 7: `PROP_HBO_BLOCK_MAX`

* Type: Read-Write
* Packed-Encoding: `S`

Octets: |       2
--------|-----------------
Fields: | `PROP_HBO_BLOCK_MAX`

Describes the number of blocks that may be offloaded from the NCP to
the host. Default value is 32. Setting this value to zero will cause
host block offload to be effectively disabled.

This value is encoded as an unsigned 16-bit integer.

This property is only available if the `CAP_HBO`
capability is present in `PROP_CAPS`.




## D. Protocol: Thread

This section describes all of the properties and semantics required
for managing a thread NCP.

### D.1. PHY Properties
#### D.1.1. PROP 32: `PROP_PHY_ENABLED`
* Type: Read-Write
* Packed-Encoding: `b`

Set to `1` if the PHY is enabled, set to `0` otherwise.
May be directly enabled to bypass higher-level packet processing
in order to implement things like packet sniffers.

#### D.1.4. PROP 33: `PROP_PHY_CHAN`
* Type: Read-Write
* Packed-Encoding: `C`

Value is the current channel. Must be set to one of the
values contained in `PROP_PHY_CHAN_SUPPORTED`.

#### D.1.5. PROP 34: `PROP_PHY_CHAN_SUPPORTED`
* Type: Read-Only
* Packed-Encoding: `A(C)`
* Unit: List of channels

Value is a list of channel values that are supported by the
hardware.

#### D.1.3. PROP 35: `PROP_PHY_FREQ`
* Type: Read-Only
* Packed-Encoding: `L`
* Unit: Kilohertz

Value is the radio frequency (in kilohertz) of the
current channel.

#### D.1.6. PROP 36: `PROP_PHY_CCA_THRESHOLD`
* Type: Read-Write
* Packed-Encoding: `c`
* Unit: dBm

Value is the CCA (clear-channel assessment) threshold. Set to
-128 to disable.

When setting, the value will be rounded down to a value
that is supported by the underlying radio hardware.

#### D.1.7. PROP 37: `PROP_PHY_TX_POWER`
* Type: Read-Write
* Packed-Encoding: `c`
* Unit: dBm

Value is the transmit power of the radio.

When setting, the value will be rounded down to a value
that is supported by the underlying radio hardware.

#### D.1.6. PROP 38: `PROP_PHY_RSSI`
* Type: Read-Only
* Packed-Encoding: `c`
* Unit: dBm

Value is the current RSSI (Received signal strength indication)
from the radio. This value can be used in energy scans and for
determining the ambient noise floor for the operating environment.


### D.2. MAC Properties


#### D.2.1. PROP 48: `PROP_MAC_SCAN_STATE`
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

#### D.2.2. PROP 49: `PROP_MAC_SCAN_MASK`
* Type: Read-Write
* Packed-Encoding: `A(C)`
* Unit: List of channels to scan


#### D.2.3. PROP 50: `PROP_MAC_SCAN_PERIOD`
* Type: Read-Write
* Packed-Encoding: `A(C)`
* Unit: List of channels to scan

#### D.2.4. PROP 51: `PROP_MAC_SCAN_BEACON`
* Type: Read-Only-Stream
* Packed-Encoding: `CcDD.` (or `CcT(ESSc.)T(iCUD.).`)

Octets: |  1   |   1  |      2       |   *n*    |       2      |   *n*  
--------|------|------|--------------|----------|--------------|----------  
Fields: | CHAN | RSSI | MAC_DATA_LEN | MAC_DATA | NET_DATA_LEN | NET_DATA  

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

#### D.2.5. PROP 52: `PROP_MAC_15_4_LADDR`
* Type: Read-Write
* Packed-Encoding: `E`

The 802.15.4 long address of this node.

#### D.2.6. PROP 53: `PROP_MAC_15_4_SADDR`
* Type: Read-Write
* Packed-Encoding: `S`

The 802.15.4 short address of this node.

#### D.2.7. PROP 54: `PROP_MAC_15_4_PANID`
* Type: Read-Write
* Packed-Encoding: `S`

The 802.15.4 PANID this node is associated with.

#### D.2.8. PROP 55: `PROP_MAC_RAW_STREAM_ENABLED`
* Type: Read-Write
* Packed-Encoding: `b`

Set to true to enable raw MAC frames to be emitted from `PROP_STREAM_RAW`.

#### D.2.9. PROP 56: `PROP_MAC_FILTER_MODE`
* Type: Read-Write
* Packed-Encoding: `C`

Possible Values:

* 0: `MAC_FILTER_MODE_NORMAL`: Normal MAC filtering is in place.
* 1: `MAC_FILTER_MODE_PROMISCUOUS`: All MAC packets matching network are passed up the stack.
* 2: `MAC_FILTER_MODE_MONITOR`: All decoded MAC packets are passed up the stack.

#### D.2.10. PROP 4864: `PROP_MAC_WHITELIST`
* Type: Read-Write
* Packed-Encoding: `A(T(Ec))`

Structure Parameters:

* `E`: EUI64 address of node
* `c`: Optional RSSI-override value. The value 127 indicates
       that the RSSI-override feature is not enabled for this
       address. If this value is ommitted when setting or
       inserting, it is assumed to be 127. This parameter is
       ignored when removing.

#### D.2.11. PROP 4865: `PROP_MAC_WHITELIST_ENABLED`
* Type: Read-Write
* Packed-Encoding: `b`

### D.3. NET Properties

#### D.3.1. PROP 64: `PROP_NET_SAVED`
* Type: Read-Only
* Packed-Encoding: `b`

Returns true if there is a network state stored that can be
restored with a call to `CMD_NET_RECALL`.

#### D.3.2. PROP 65: `PROP_NET_IF_UP`
* Type: Read-Write
* Packed-Encoding: `b`

Network interface up/down status. Non-zero (set to 1) indicates up,
zero indicates down.

#### D.3.3. PROP 66: `PROP_NET_STACK_UP`
* Type: Read-Write
* Packed-Encoding: `b`
* Unit: Enumeration

Thread stack operational status. Non-zero (set to 1) indicates up,
zero indicates down.

#### D.3.4. PROP 67: `PROP_NET_ROLE`
* Type: Read-Write
* Packed-Encoding: `C`
* Unit: Enumeration

Values:

* 0: `NET_ROLE_DETACHED`
* 1: `NET_ROLE_CHILD`
* 2: `NET_ROLE_ROUTER`
* 3: `NET_ROLE_LEADER`

#### D.3.5. PROP 68: `PROP_NET_NETWORK_NAME`
* Type: Read-Write
* Packed-Encoding: `U`

#### D.3.6. PROP 69: `PROP_NET_XPANID`
* Type: Read-Write
* Packed-Encoding: `D`

#### D.3.7. PROP 70: `PROP_NET_MASTER_KEY`
* Type: Read-Write
* Packed-Encoding: `D`

#### D.3.8. PROP 71: `PROP_NET_KEY_SEQUENCE`
* Type: Read-Write
* Packed-Encoding: `L`

#### D.3.9. PROP 72: `PROP_NET_PARTITION_ID`
* Type: Read-Write
* Packed-Encoding: `L`

The partition ID of the partition that this node is a member of.



### D.4. THREAD Properties

#### D.4.1. PROP 80: `PROP_THREAD_LEADER_ADDR`
* Type: Read-Only
* Packed-Encoding: `6`

The IPv6 address of the leader. (Note: May change to long and short address of leader)

#### D.4.2. PROP 81: `PROP_THREAD_PARENT`
* Type: Read-Only
* Packed-Encoding: `ES`
* LADDR, SADDR

The long address and short address of the parent of this node.

#### D.4.3. PROP 82: `PROP_THREAD_CHILD_TABLE`
* Type: Read-Only
* Packed-Encoding: `A(T(ES))`

Table containing the long and short addresses of all
the children of this node.

#### D.4.4. PROP 83: `PROP_THREAD_LEADER_RID`
* Type: Read-Only
* Packed-Encoding: `C`

The router-id of the current leader.

#### D.4.5. PROP 84: `PROP_THREAD_LEADER_WEIGHT`
* Type: Read-Only
* Packed-Encoding: `C`

The leader weight of the current leader.

#### D.4.6. PROP 85: `PROP_THREAD_LOCAL_LEADER_WEIGHT`
* Type: Read-Write
* Packed-Encoding: `C`

The leader weight for this node.

#### D.4.7. PROP 86: `PROP_THREAD_NETWORK_DATA`
* Type: Read-Only
* Packed-Encoding: `D`

#### D.4.8. PROP 87: `PROP_THREAD_NETWORK_DATA_VERSION`
* Type: Read-Only
* Packed-Encoding: `S`

#### D.4.9. PROP 88: `PROP_THREAD_STABLE_NETWORK_DATA`
* Type: Read-Only
* Packed-Encoding: `D`

#### D.4.10. PROP 89: `PROP_THREAD_STABLE_NETWORK_DATA_VERSION`
* Type: Read-Only
* Packed-Encoding: `S`

#### D.4.11. PROP 90: `PROP_THREAD_ON_MESH_NETS`
* Type: Read-Write
* Packed-Encoding: `A(T(6CbC))`

Data per item is:

* `6`: IPv6 Prefix
* `C`: Prefix length, in bits
* `b`: Stable flag
* `C`: Other flags

#### D.4.12. PROP 91: `PROP_THREAD_LOCAL_ROUTES`
* Type: Read-Write
* Packed-Encoding: `A(T(6CbC))`

Data per item is:

* `6`: IPv6 Prefix
* `C`: Prefix length, in bits
* `b`: Stable flag
* `C`: Other flags

#### D.4.13. PROP 92: `PROP_THREAD_ASSISTING_PORTS`
* Type: Read-Write
* Packed-Encoding: `A(S)`

#### D.4.14. PROP 93: `PROP_THREAD_ALLOW_LOCAL_NET_DATA_CHANGE`
* Type: Read-Write
* Packed-Encoding: `b`

Set to true before changing local net data. Set to false when finished.
This allows changes to be aggregated into single events.

#### D.4.15. PROP 94: `PROP_THREAD_MODE`
* Type: Read-Write
* Packed-Encoding: `C`

This property contains the value of the mode
TLV for this node. The meaning of the bits in this
bitfield are defined by section 4.5.2 of the Thread
specification.

#### D.4.16. PROP 5376: `PROP_THREAD_CHILD_TIMEOUT`
* Type: Read-Write
* Packed-Encoding: `L`

Used when operating in the Child role.

#### D.4.17. PROP 5377: `PROP_THREAD_RLOC16`
* Type: Read-Write
* Packed-Encoding: `S`

#### D.4.18. PROP 5378: `PROP_THREAD_ROUTER_UPGRADE_THRESHOLD`
* Type: Read-Write
* Packed-Encoding: `C`

#### D.4.19. PROP 5379: `PROP_THREAD_CONTEXT_REUSE_DELAY`
* Type: Read-Write
* Packed-Encoding: `L`

#### D.4.20. PROP 5380: `PROP_THREAD_NETWORK_ID_TIMEOUT`
* Type: Read-Write
* Packed-Encoding: `C`

Allows you to get or set the Thread `NETWORK_ID_TIMEOUT` constant, as
defined by the Thread specification.

### D.5. IPv6 Properties

#### D.5.1. PROP 96: `PROP_IPV6_LL_ADDR`
* Type: Read-Only
* Packed-Encoding: `6`

IPv6 Address

#### D.5.2. PROP 97: `PROP_IPV6_ML_ADDR`
* Type: Read-Only
* Packed-Encoding: `6`

IPv6 Address + Prefix Length

#### D.5.3. PROP 98: `PROP_IPV6_ML_PREFIX`
* Type: Read-Write
* Packed-Encoding: `6C`

IPv6 Prefix + Prefix Length

#### D.5.4. PROP 99: `PROP_IPV6_ADDRESS_TABLE`
* Type: Read-Write
* Packed-Encoding: `A(T(6CLLC))`

Array of structures containing:

* `6`: IPv6 Address
* `C`: Network Prefix Length
* `L`: Valid Lifetime
* `L`: Preferred Lifetime
* `C`: Flags

#### D.5.5. PROP 100: `PROP_IPv6_ROUTE_TABLE`
* Type: Read-Write
* Packed-Encoding: `A(T(6C6))`

Array of structures containing:

* `6`: IPv6 Prefix
* `C`: Network Prefix Length
* `C`: Interface ID
* `C`: Flags

