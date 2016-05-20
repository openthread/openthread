Spinel Host Controller Interface
================================

Updated: 2016-05-18

Written by: Robert Quattlebaum <rquattle@nestlabs.com>

THIS DOCUMENT IS A WORK IN PROGRESS AND SUBJECT TO CHANGE.

Copyright (c) 2016 Nest Labs, All Rights Reserved

## 0. Abstract ##

This document describes a general management protocol for enabling a host
device to communicate with and manage a network co-processor(NCP).

While initially designed to support Thread-based NCPs, the NCP protocol
has been designed with a layered approach that allows it to be easily
adapted to other network protocols.

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
 *  A command (up to three bytes)
 *  An optional command payload

    FRAME = HEADER CMD [CMD_PAYLOAD]

Octets: |    1   | 1-3 |       *n*
--------|--------|-----|-----------------
Fields: | HEADER | CMD | *[CMD_PAYLOAD]*


### 3.1. Header Format ###

The header byte is broken down as follows:

     0 1 2 3 4 5 6 7
    +-+-+-+-+-+-+-+-+
    |1|R|IID|  TID  |
    +-+-+-+-+-+-+-+-+

#### 3.1.1. Flag Bit ####

The most significant header bit is always set to 1 to allow this
protocol to be line compatible with BTLE HCI. By setting the first
bit, we can disambiguate between Spinel frames and HCI frames (which
always start with either `0x01` or `0x04`) without any additional
framing overhead.

#### 3.1.2. Reserved Bit ####

The reserved bit (`R`) is reserved for future use. The sender MUST
set this bit to zero, and the receiver MUST ignore frames with this
bit set.

#### 3.1.3. Interface Identifier (IID) ####

The Interface Identifier (IID) is a number between 0 and 3 which
identifies which subinterface the frame is intended for. This allows
the protocol to support connecting to more than one network at once.
The first subinterface (0) is considered the primary subinterface and
MUST be supported. Support for all other subinterfaces is OPTIONAL.

#### 3.1.4. Transaction Identifier (TID) ####

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
section 7.2. This encoding allows for up to 2,097,152 individual
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
the host. This is primarily used for livliness checks.

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
packed unsigned integer format described in section 7.2.

If an error occurs, the value of `PROP_LAST_STATUS` will be emitted
instead with the value set to the generated status code for the error.



### 4.4. CMD 3: (Host->NCP) `CMD_PROP_VALUE_SET`

Octets: |    1   |          1         |   1-3   |    *n*
--------|--------|--------------------|---------|------------
Fields: | HEADER | CMD_PROP_VALUE_SET | PROP_ID | PROP_VALUE

Set property value command. Instructs the NCP to set the given
property to the specific given value, replacing any previous value.

The payload for this command is the property identifier encoded in the
packed unsigned integer format described in section 7.2, followed by
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
packed unsigned integer format described in section 7.2, followed by
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
packed unsigned integer format described in section 7.2, followed by
the value to be inserted. The exact format of the value is defined by
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
packed unsigned integer format described in section 7.2, followed by
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
packed unsigned integer format described in section 7.2, followed by
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
packed unsigned integer format described in section 7.2, followed by
the value that was removed from the given property.

The resulting order of items in the list is defined by the given
property.






## 5. General Properties

While the majority of the properties that allow the configuration
of network connectivity are network protocol specific, there are
several properties that are required in all implementations.

* 0: `PROP_LAST_STATUS`
* 1: `PROP_PROTOCOL_VERSION`
* 2: `PROP_CAPABILITIES`
* 3: `PROP_NCP_VERSION`
* 4: `PROP_INTERFACE_COUNT`
* 5: `PROP_POWER_STATE`
* 6: `PROP_HWADDR`
* 7: `PROP_LOCK`
* 8: `PROP_HBO_MEM_MAX` (See section C.2.1.)
* 9: `PROP_HBO_BLOCK_MAX` (See section C.2.2.)
* 112: `PROP_STREAM_DEBUG`
* 113: `PROP_STREAM_RAW`
* 114: `PROP_STREAM_NET`
* 115: `PROP_STREAM_NET_INSECURE`




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
* Encoding: `iiii`

Octets: |       1-3      |      1-3      |      1-3      |        1-3
--------|----------------|---------------|---------------|-------------------
Fields: | INTERFACE_TYPE | MAJOR_VERSION | MINOR_VERSION | VENDOR_IDENTIFIER

Describes the protocol version information. This property contains
four fields, each encoded as a packed unsigned integer:

 *  Interface Type
 *  Major Version Number
 *  Minor Version Number
 *  Vendor Identifier

#### 5.2.1 Interface Type ####

This integer identifies what the network protocol for this NCP.
Currently defined values are:

 *  1: ZigBee
 *  2: ZigBeeIP
 *  3: Thread
 *  TBD: BlueTooth Low Energy (BTLE)

The host MUST enter a FAULT state if it does not recognize the
protocol given by the NCP.

#### 5.2.2 Major Version Number ####

The major version number is used to identify large and incompatible
differences between protocol versions.

The host MUST enter a FAULT state if it does not explicitly support
the given major version number.

#### 5.2.3 Minor Version Number ####

The minor version number is used to identify small but otherwise
compatible differences between protocol versions. A mismatch between
the advertised minor version number and the minor version that is
supported by the host SHOULD NOT be fatal to the operation of the
host.

#### 5.2.3 Vendor Identifier ####


### 5.3. PROP 2: `PROP_CAPABILITIES`

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
64 - 15,359           | *UNALLOCATED*
15,360 - 16,383       | Vendor-specific
16,384 - 1,999,999    | *UNALLOCATED*
2,000,000 - 2,097,151 | Experimental use only

### 5.4. PROP 3: `PROP_NCP_VERSION`

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

### 5.5. PROP 4: `PROP_INTERFACE_COUNT`

* Type: Read-Only
* Packed-Encoding: `C`

Octets: |       1
--------|-----------------
Fields: | `INTERFACE_COUNT`

Describes the number of concurrent interfaces supported by this NCP.
Since the concurrent interface mechanism is still TBD, this value MUST
always be one.

This value is encoded as an unsigned 8-bit integer.

### 5.6. PROP 5: `PROP_POWER_STATE`

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

### 5.7. PROP 6: `PROP_HWADDR`

* Type: Read-Only*
* Packed-Encoding: `E`

Octets: |    8
--------|------------
Fields: | PROP_HWADDR

The static EUI64 address of the device. This value is read-only, but
may be writable under certain vendor-defined circumstances.

### 5.8. PROP 6: `PROP_LOCK`

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

### 5.9. PROP 112: `PROP_STREAM_DEBUG`

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

### 5.10. PROP 113: `PROP_STREAM_RAW`

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

### 5.11. PROP 114: `PROP_STREAM_NET`

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

### 5.12. PROP 114: `PROP_STREAM_NET_INSECURE`

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
 *  2: `STATUS_UNIMPLEMENTED`
 *  3: `STATUS_INVALID_ARGUMENT`
 *  4: `STATUS_INVALID_STATE`
 *  5: `STATUS_INVALID_COMMAND`
 *  6: `STATUS_INVALID_INTERFACE`
 *  7: `STATUS_INTERNAL_ERROR`
 *  8: `STATUS_SECURITY_ERROR`
 *  9: `STATUS_PARSE_ERROR`
 *  10: `STATUS_IN_PROGRESS`
 *  11: `STATUS_NOMEM`
 *  12: `STATUS_BUSY`
 *  13: `STATUS_PROPERTY_NOT_FOUND`
 *  14: `STATUS_PACKET_DROPPED`
 *  15: `STATUS_EMPTY`
 *  16-111: *RESERVED*
 *  112-127: Reset Causes
     *  112: `STATUS_RESET_POWER_ON`
     *  113: `STATUS_RESET_EXTERNAL`
     *  114: `STATUS_RESET_SOFTWARE`
     *  115: `STATUS_RESET_FAULT`
     *  116: `STATUS_RESET_CRASH`
     *  117: `STATUS_RESET_ASSERT`
     *  118: `STATUS_RESET_OTHER`
     *  119-127: *RESERVED-RESET-CODES*
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

However, things are a little different with `CLDL`. Since our datablob
is no longer the last item in the signature, the length must be
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

Since this NCP protocol is defined independently of framing, any
number of framing protocols could be used successfully. However,
in the interests of cross-compatibility, we recommend using
HDLC-Lite for framing when using this NCP protocol with a UART.

A SPI-specific framing mechanism is currently TBD.

## B. Feature: Network Save

The network save feature is an optional NCP capability that, when
present, allows the host to save and recall network credentials and
state to and from nonvolatile storage.

The presence of this feature can be detected by checking for the
presence of the `CAP_NET_SAVE` capability in `PROP_CAPABILITIES`.

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
capability in `PROP_CAPABILITIES`.

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
capability is present in `PROP_CAPABILITIES`.

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
capability is present in `PROP_CAPABILITIES`.




## D. Protocol: Thread

This section describes all of the properties and semantics required
for managing a thread NCP.

### D.1. PHY Properties
#### D.1.1. PROP x: `PROP_PHY_ENABLED`
* Type: Read-Only
* Packed-Encoding: `b`

#### D.1.4. PROP x: `PROP_PHY_CHAN`
* Type: Read-Write
* Packed-Encoding: `C`

#### D.1.5. PROP x: `PROP_PHY_CHAN_SUPPORTED`
* Type: Read-Only
* Packed-Encoding: `A(C)`
* Unit: List of channels

#### D.1.3. PROP x: `PROP_PHY_FREQ`
* Type: Read-Only
* Packed-Encoding: `L`
* Unit: Kilohertz

#### D.1.6. PROP x: `PROP_PHY_CCA_THRESHOLD`
* Type: Read-Write
* Packed-Encoding: `c`
* Unit: dBm

#### D.1.7. PROP x: `PROP_PHY_TX_POWER`
* Type: Read-Write
* Packed-Encoding: `c`
* Unit: dBm

#### D.1.6. PROP x: `PROP_PHY_RSSI`
* Type: Read-Only
* Packed-Encoding: `c`
* Unit: dBm

#### D.1.7. PROP x: `PROP_PHY_RAW_STREAM_ENABLED`
* Type: Read-Write
* Packed-Encoding: `b`

Set to true to enable raw frames to be emitted from `PROP_STREAM_RAW`.



### D.2. MAC Properties


#### D.2.1. PROP x: `PROP_MAC_SCAN_STATE`
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

#### D.2.2. PROP x: `PROP_MAC_SCAN_MASK`
* Type: Read-Write
* Packed-Encoding: `A(C)`
* Unit: List of channels to scan

#### D.2.3. PROP x: `PROP_MAC_SCAN_BEACON`
* Type: Read-Only-Stream
* Packed-Encoding: `CcT(ESSC)T(i).`

chan,rssi,(laddr,saddr,panid,lqi),(proto,xtra)

chan,rssi,(laddr,saddr,panid,lqi),(proto,flags,networkid,xpanid) [CcT(ESSC)T(iCUD.).]

#### D.2.4. PROP x: `PROP_MAC_15_4_LADDR`
* Type: Read-Write
* Packed-Encoding: `E`

#### D.2.5. PROP x: `PROP_MAC_15_4_SADDR`
* Type: Read-Write
* Packed-Encoding: `S`

#### D.2.6. PROP x: `PROP_MAC_15_4_PANID`
* Type: Read-Write
* Packed-Encoding: `S`





### D.3. NET Properties

#### D.3.1. PROP x: `PROP_NET_SAVED`
* Type: Read-Only
* Packed-Encoding: `b`

#### D.3.2. PROP x: `PROP_NET_ENABLED`
* Type: Read-Only
* Packed-Encoding: `b`

#### D.3.3. PROP x: `PROP_NET_STATE`
* Type: Read-Write
* Packed-Encoding: `C`
* Unit: Enumeration

Values:

* 0: `NET_STATE_OFFLINE`
* 1: `NET_STATE_DETACHED`
* 2: `NET_STATE_ATTACHING`
* 3: `NET_STATE_ATTACHED`

#### D.3.4. PROP x: `PROP_NET_ROLE`
* Type: Read-Write
* Packed-Encoding: `C`
* Unit: Enumeration

Values:

* 0: `NET_ROLE_NONE`
* 1: `NET_ROLE_CHILD`
* 2: `NET_ROLE_ROUTER`
* 3: `NET_ROLE_LEADER`

#### D.3.5. PROP x: `PROP_NET_NETWORK_NAME`
* Type: Read-Write
* Packed-Encoding: `U`

#### D.3.6. PROP x: `PROP_NET_XPANID`
* Type: Read-Write
* Packed-Encoding: `D`

#### D.3.7. PROP x: `PROP_NET_MASTER_KEY`
* Type: Read-Write
* Packed-Encoding: `D`

#### D.3.8. PROP x: `PROP_NET_KEY_SEQUENCE`
* Type: Read-Write
* Packed-Encoding: `L`

#### D.3.9. PROP x: `PROP_NET_PARTITION_ID`
* Type: Read-Write
* Packed-Encoding: `L`



#### D.4. THREAD Properties


#### D.4.1. PROP x: `PROP_THREAD_LEADER`
* Type: Read-Write
* Packed-Encoding: `6`

#### D.4.2. PROP x: `PROP_THREAD_PARENT`
* Type: Read-Write
* Packed-Encoding: `ES`
* LADDR, SADDR

#### D.4.3. PROP x: `PROP_THREAD_CHILD_TABLE`
* Type: Read-Write
* Packed-Encoding: `A(T(ES))`
* LADDR, SADDR



### D.5. IPv6 Properties

#### D.5.1. PROP x: `PROP_IPV6_LL_ADDR`
* Type: Read-Only
* Packed-Encoding: `6`

IPv6 Address

#### D.5.2. PROP x: `PROP_IPV6_ML_ADDR`
* Type: Read-Only
* Packed-Encoding: `6`

IPv6 Address + Prefix Length

#### D.5.2. PROP x: `PROP_IPV6_ML_PREFIX`
* Type: Read-Write
* Packed-Encoding: `6C`

IPv6 Prefix + Prefix Length

#### D.5.3. PROP x: `PROP_IPV6_ADDRESS_TABLE`

* Type: Read-Write
* Packed-Encoding: `A(T(6CLLC))`

Array of structures containing:

* `6`: IPv6 Address
* `C`: Network Prefix Length
* `L`: Valid Lifetime
* `L`: Preferred Lifetime
* `C`: Flags

#### D.4.3. PROP x: `PROP_IPv6_ROUTE_TABLE`
* Type: Read-Write
* Packed-Encoding: `A(T(6C6))`

Array of structures containing:

* `6`: IPv6 Address
* `C`: Network Prefix Length
* `6`: Next Hop
