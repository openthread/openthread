## Core Properties {#prop-core}

### PROP 0: PROP_LAST_STATUS {#prop-last-status}

* Type: Read-Only
* Encoding: `i`

Octets: |    1-3
-------:|-------------
Fields: | `STATUS_CODE`

Describes the status of the last NCP operation. Encoded as a packed unsigned integer.

This property is emitted often to indicate the result status of pretty much any OS-to-NCP operation. It is also emitted automatically at NCP startup with a value indicating the reset reason.

See (#status-codes) for the complete list of status codes.

### PROP 1: PROP_PROTOCOL_VERSION {#prop-protocol-version}

* Type: Read-Only
* Encoding: `ii`

Octets: |       1-3       |      1-3
--------|-----------------|---------------
Fields: | `MAJOR_VERSION` | `MINOR_VERSION`

Describes the protocol version information. This property contains four fields, each encoded as a packed unsigned integer:

 *  Major Version Number
 *  Minor Version Number

This document describes major version 4, minor version 3 of this protocol.

The OS **MUST** use NLI 0 with commands using this property. The NCP **SHOULD NOT** process commands using this property if NLI is not zero. The operational semantics of this property when NLI is not zero are not specified.

#### Major Version Number

The major version number is used to identify backward incompatible differences between protocol versions.

The OS **MUST** enter a FAULT state if the given major version number is unsupportable.

#### Minor Version Number

The minor version number is used to identify backward-compatible differences between protocol versions. A mismatch between the advertised minor version number and the minor version that is supported by the OS **SHOULD NOT** be fatal to the operation of the OS.

### PROP 2: PROP_NCP_VERSION {#prop-ncp-version}

* Type: Read-Only
* Packed-Encoding: `U`

Octets: |       *n*
--------|-------------------
Fields: | `NCP_VESION_STRING`

Contains a string which describes the firmware currently running on the NCP. Encoded as a zero-terminated UTF-8 string.

The format of the string is not strictly defined, but it is intended to present similarly to the "User-Agent" string from HTTP. The
RECOMMENDED format of the string is as follows:

  `STACK-NAME/STACK-VERSION[BUILD_INFO][; OTHER_INFO]; BUILD_DATE`

Examples:

 *  `OpenThread/1.0d26-25-gb684c7f; DEBUG; May 9 2016 18:22:04`
 *  `ConnectIP/2.0b125 s1 ALPHA; Sept 24 2015 20:49:19`

The OS **MUST** use NLI 0 with commands using this property. The NCP **SHOULD NOT** process commands using this property if NLI is not zero. The operational semantics of this property when NLI is not zero are not specified.

### PROP 3: PROP_INTERFACE_TYPE {#prop-interface-type}

* Type: Read-Only
* Encoding: `i`

Octets: |       1-3
:-------|:---------------
Fields: | `INTERFACE_TYPE`

This unsigned packed integer identifies the network protocol implemented by this NCP. A registry of network interface type codes is maintained by IANA with a reservation policy of Specification Required. The initial content of the registry is shown in the table below:

Code    | Network protocol
:-------|:----------------
0       | Bootloader
2       | ZigBee IP(TM)
3       | Thread(R)

The OS **MUST** enter a FAULT state if it does not recognize the network protocol given by the NCP.

### PROP 4: PROP_INTERFACE_VENDOR_ID {#prop-interface-vendor-id}

* Type: Read-Only
* Encoding: `i`

Octets: |       1-3
:-------|:---------------
Fields: | `VENDOR_ID`

Vendor identifier.

EDITOR: I have no idea how to write the IANA registry creation text for this.

### PROP 5: PROP_CAPS {#prop-caps}

* Type: Read-Only
* Packed-Encoding: `A(i)`

Octets: |  1-3  |  1-3  | ...
:-------|:------|:------|:----
Fields: | `CAP` | `CAP` | ...

Describes the supported capabilities of this NCP. Encoded as a list of packed unsigned integers. A registry of capability codes is maintained by IANA with various reservation policies in effect for different ranges of values as shown in the table below.

Capability Range        | Description
:-----------------------|:-----------------
0 - 1027                | Standards Action
1027 - 15,359           | Unassigned
15,360 - 16,383         | Private Use
16,384 - 1,999,999      | Unassigned
2,000,000 - 2,097,151   | Experimental Use

The initial content of the registry is shown in the table below:

Code    | Name                      | Description
:-------|:--------------------------|:----------------------------------
   1    | `CAP_LOCK`                | EDITOR: to be provided.
   2    | `CAP_NET_SAVE`            | EDITOR: to be provided.
   3    | `CAP_HBO`                 | Host Buffer Offload. See (#feature-host-buffer-offload).
   4    | `CAP_POWER_SAVE`          | EDITOR: to be provided.
   5    | `CAP_COUNTERS`            | EDITOR: to be provided.
   7    | `CAP_PEEK_POKE`           | Peek/poke debugging commands.
   8    | `CAP_WRITABLE_RAW_STREAM` | `PROP_STREAM_RAW` is writable.
   9    | `CAP_GPIO`                | Support for GPIO access. See (#feature-gpio-access).
  10    | `CAP_TRNG`                | Support for true random number generation. See (#feature-trng).
  11    | `CAP_CMD_MULTI`           | Support for `CMD_PROP_VALUE_MULTI_GET`, `CMD_PROP_VALUE_MULTI_SET` and
                                        `CMD_PROP_VALUES_ARE`.
  12    | `CAP_UNSOL_UPDATE_FILTER` | Support for `PROP_UNSOL_UPDATE_FILTER` and `PROP_UNSOL_UPDATE_LIST`.
  48    | `CAP_ROLE_ROUTER`         | EDITOR: to be provided.
  49    | `CAP_ROLE_SLEEPY`         | EDITOR: to be provided.
 512    | `CAP_MAC_WHITELIST`       | EDITOR: to be provided.
 513    | `CAP_MAC_RAW`             | EDITOR: to be provided.
 514    | `CAP_OOB_STEERING_DATA`   | EDITOR: to be provided.

### PROP 6: PROP_INTERFACE_COUNT {#prop-interface-count}

* Type: Read-Only
* Packed-Encoding: `C`

Octets: |       1
:-------|:----------------
Fields: | `INTERFACE_COUNT`

Describes the number of concurrent interfaces supported by this NCP. Since the concurrent interface mechanism is still TBD, this value **MUST** always be one.

This value is encoded as an unsigned 8-bit integer.

The OS **MUST** use NLI 0 with commands using this property. The NCP **SHOULD NOT** process commands using this property if NLI is not zero. The operational semantics of this property when NLI is not zero are not specified.

### PROP 7: PROP_POWER_STATE {#prop-power-state}

* Type: Read-Write
* Packed-Encoding: `C`

Octets: |        1
:-------|:-----------------
Fields: | `POWER_STATE`

A single octet coded that indicates the current power state of the NCP. Setting this property allows controls of the current NCP power state. The following table enumerates the standard codes and their significance.

Code    | Name                      | Significance
:-------|:--------------------------|:------------------------------------------------------
    0   | `POWER_STATE_OFFLINE`     | NCP is physically powered off. (Enumerated for completeness sake, not expected on the wire)
    1   | `POWER_STATE_DEEP_SLEEP`  | NCP is not powered to detect any events on physical network media.
    2   | `POWER_STATE_STANDBY`     | NCP is powered only to detect certain events on physical network media that signal to wake.
    3   | `POWER_STATE_LOW_POWER`   | NCP is powered only for limited responsiveness for power conservation purposes.
    4   | `POWER_STATE_ONLINE`      | NCP is powered for full responsiveness.

EDITOR: We should consider reversing the numbering here so that 0 is `POWER_STATE_ONLINE`. We may also want to include some extra values between the defined values for future expansion, so that we can preserve the ordered relationship.

### PROP 8: PROP_HWADDR {#prop-hwaddr}

* Type: Read-Only
* Packed-Encoding: `E`

Octets: |    8
:-------|:-----------
Fields: | `HWADDR`

The EUI-64 format of the link-layer address of the device.

### PROP 9: PROP_LOCK {#prop-lock}

* Type: Read-Write
* Packed-Encoding: `b`

Octets: |    1
:-------|:-----------
Fields: | `LOCK`

Property transaction lock. Used for grouping transactional changes to several properties for simultaneous commit, or to temporarily prevent the automatic updating of property values. When this property is set, the execution of the NCP is effectively frozen until it is cleared. There is no support for transaction rollback.

This property is only supported if the `CAP_LOCK` capability is present.

Unlike most other properties, setting this property to true when the value of the property is already true **MUST** fail with a last status of `STATUS_ALREADY`.

### PROP 10: PROP_HOST_POWER_STATE {#prop-host-power-state}

* Type: Read-Write
* Packed-Encoding: `C`
* Default value: 4

Octets: |        1
:-------:------------------
Fields: | `HOST_POWER_STATE`

Describes the current power state of the *OS*. This property is used by the OS to inform the NCP when it has changed power states. The NCP can then use this state to determine which properties need asynchronous updates. Enumeration is encoded as a single unsigned octet.

The following table enumerates the standard codes and their significance.

Code| Name                          | Significance
:---|:------------------------------|:------------------------------------------------------
 0  | `HOST_POWER_STATE_OFFLINE`    | OS is physically powered off and cannot be awakened by the NCP.
 1  | `HOST_POWER_STATE_DEEP_SLEEP` | OS is in a deep low power state and will require a long time to wake.
 3  | `HOST_POWER_STATE_LOW_POWER`  | OS is in a low power state and can be awakened quickly.
 4  | `HOST_POWER_STATE_ONLINE`     | OS is powered for full responsiveness.

EDITOR: We should consider reversing the numbering here so that 0 is `POWER_STATE_ONLINE`. We may also want to include some additional reserved values between the defined values for future expansion, so that we can preserve the ordered relationship. See the similar editorial comment at (#prop-power-state).

After the OS sends `CMD_PROP_VALUE_SET` for this property with a value other than `HOST_POWER_STATE_ONLINE`, it **SHOULD** wait for the NCP to acknowledge the property update (with a `CMD_VALUE_IS` command) before entering the specified power state.

On the NCP receiving any command when the state is not `HOST_POWER_STATE_ONLINE`, it **MUST** silently update the state to the `HOST_POWER_STATE_ONLINE` value.

When the state is not `HOST_POWER_STATE_ONLINE`, the NCP **SHOULD NOT** send any commands except important notifications that warrant awakening the OS host, and the NCP **MUST NOT** send any informative messages on the debug stream.

When the state is `HOST_POWER_STATE_DEEP_SLEEP`, the NCP **MUST NOT** send any commands, including any commands that contain network packets, prior to signaling the host explicitly to awaken and receiving a signal to update the state to `HOST_POWER_STATE_ONLINE`. As noted above, reception of any Spinel command is a signal to set the state to `HOST_POWER_STATE_ONLINE`.

The OS **MUST NOT** send a value of `HOST_POWER_STATE` other than one of the standard codes defined here. If the NCP receives a value other than a standard code, then it **SHOULD** set the state to `HOST_POWER_STATE_LOW_POWER`.

If the NCP has the `CAP_UNSOL_UPDATE_FILTER` capability, any unsolicited property updates masked by `PROP_UNSOL_UPDATE_FILTER` should be honored while the OS indicates it is in a low-power state. After resuming to the `HOST_POWER_STATE_ONLINE` state, the value of `PROP_UNSOL_UPDATE_FILTER` **MUST** be unchanged from the value assigned prior to the OS indicating it was entering a low-power state.

The OS **MUST** use NLI 0 with commands using this property. The NCP **SHOULD NOT** process commands using this property if NLI is not zero. The operational semantics of this property when NLI is not zero are not specified.

### PROP 4104: PROP_UNSOL_UPDATE_FILTER {#prop-unsol-update-filter}

* Required only if `CAP_UNSOL_UPDATE_FILTER` is set.
* Type: Read-Write
* Packed-Encoding: `A(I)`
* Default value: Empty.

Contains a list of properties which are *excluded* from generating unsolicited value updates. This property **MUST** be empty after NCP reset.

In other words, the OS may opt-out of unsolicited property updates for a specific property by adding that property id to this list.

The OS **SHOULD NOT** add properties to this list which are not present in `PROP_UNSOL_UPDATE_LIST`. If such properties are added, the NCP **MUST** ignore the unsupported properties.

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

The NCP is **REQUIRED** to support and use the following commands with this property:

* `CMD_PROP_VALUE_GET` ((#cmd-prop-value-get))
* `CMD_PROP_VALUE_SET` ((#cmd-prop-value-set))
* `CMD_PROP_VALUE_IS` ((#cmd-prop-value-is))

If the NCP advertises the `CAP_CMD_MULTI` capability, then it **SHOULD** also support and use the following commands with this property:

* `CMD_PROP_VALUE_INSERT` ((#cmd-prop-value-insert))
* `CMD_PROP_VALUE_REMOVE` ((#cmd-prop-value-remove))
* `CMD_PROP_VALUE_INSERTED` ((#cmd-prop-value-inserted))
* `CMD_PROP_VALUE_REMOVED` ((#cmd-prop-value-removed))

The value of this property **MUST NOT** depend on the NLI used.

### PROP 4105: PROP_UNSOL_UPDATE_LIST {#prop-unsol-update-list}

* Required only if `CAP_UNSOL_UPDATE_FILTER` is set.
* Type: Read-Only
* Packed-Encoding: `A(I)`

Contains a list of properties which are capable of generating unsolicited value updates. This list can be used when populating `PROP_UNSOL_UPDATE_FILTER` to disable all unsolicited property updates.

The NCP **MUST NOT** change the value of this property after sending a `CMD_VALUE_IS` for `PROP_LAST_STATUS` with any of the `STATUS_RESET_xxxxx` status codes.

Note: not all properties that support unsolicited updates need to be listed here. Some properties, network media scan results for example, are only generated due to direct action on the part of the OS, so those properties **SHOULD NOT** not be included in this list.

The value of this property **MAY** depend on the NLI used.

## Stream Properties {#prop-stream}

### PROP 112: PROP_STREAM_DEBUG {#prop-stream-debug}

* Type: Read-Only-Stream
* Packed-Encoding: `D`

Octets: |    *n*
:-------|:-----------
Fields: | `UTF8_DATA`

This property is a streaming property, meaning that you cannot explicitly fetch the value of this property. The stream provides human-readable debugging output which may be displayed in the OS logs.

The location of newline characters is not assumed by the OS: it is the NCP's responsibility to insert newline characters where needed, just like with any other text stream.

To receive the debugging stream entails the OS waiting for `CMD_PROP_VALUE_IS` commands from the NCP for this property.

The value of this property **MUST NOT** depend on the NLI used.

### PROP 113: PROP_STREAM_RAW {#prop-stream-raw}

* Type: Read-Write-Stream
* Packed-Encoding: `dD`

Octets: |        2       |     *n*    |       *n*
:-------|:---------------|:-----------|:---------------
Fields: | FRAME_DATA_LEN | FRAME_DATA | FRAME_METADATA

This stream provides the capability of sending and receiving raw packets to and from the network. The exact format of the frame metadata and data is dependent on the MAC and PHY being used.

This property is a streaming property, meaning that you cannot explicitly fetch the value of this property. To receive traffic entails the OS wait foring `CMD_PROP_VALUE_IS` commands with this property identifier from the NCP.

Implementations **MAY** support the ability to transmit arbitrary raw packets. Support for this feature is indicated by the presence of the `CAP_WRITABLE_RAW_STREAM` capability.

If the capability `CAP_WRITABLE_RAW_STREAM` is set, then packets written to this stream with `CMD_PROP_VALUE_SET` will be sent out over the radio. This allows the caller to use the network directly, with the full network layer stack being implemented on the OS instead of the NCP.

#### Frame Metadata Format {#frame-metadata-format}

Any data past the end of `FRAME_DATA_LEN` is considered metadata and is **OPTIONAL**. Frame metadata **MAY** be empty or partially specified. The operational semantics of using frame metadata is not specified in the basis protocol, i.e. the specification of metadata formats is left to specializations of Spinel for specific network technologies.

### PROP 114: PROP_STREAM_NET {#prop-stream-net}

* Type: Read-Write-Stream
* Packed-Encoding: `dD`

Octets: |         2        |     *n*      |       *n*
:-------|:-----------------|--------------|----------------
Fields: | `FRAME_DATA_LEN` | `FRAME_DATA` | `FRAME_METADATA`

This stream provides the capability of sending and receiving data packets to and from the currently attached network. The exact format of the frame metadata and data is dependent on the network protocol being used.

This property is a streaming property, meaning that you cannot explicitly fetch the value of this property. To receive traffic entails to OS waiting for `CMD_PROP_VALUE_IS` commands with this property identifier from the NCP.

To transmit network packets entails the OS sending `CMD_PROP_VALUE_SET` on this property with the value of the packet.

Any data past the end of `FRAME_DATA_LEN` is considered metadata, the format of which is described in (#frame-metadata-format).

### PROP 115: PROP_STREAM_NET_INSECURE {#prop-stream-net-insecure}

* Type: Read-Write-Stream
* Packed-Encoding: `dD`

Octets: |         2        |     *n*      |       *n*
:-------|:-----------------|--------------|----------------
Fields: | `FRAME_DATA_LEN` | `FRAME_DATA` | `FRAME_METADATA`

This stream provides the capability of sending and receiving plaintext non-authenticated data packets to and from the currently attached network. The exact format of the frame metadata and data is dependent on the network protocol being used.

This property is a streaming property, meaning that you cannot explicitly fetch the value of this property. To receive traffic entails to OS waiting for `CMD_PROP_VALUE_IS` commands with this property identifier from the NCP.

To transmit network packets entails the OS sending `CMD_PROP_VALUE_SET` on this property with the value of the packet.

Any data past the end of `FRAME_DATA_LEN` is considered metadata, the format of which is described in (#frame-metadata-format).
