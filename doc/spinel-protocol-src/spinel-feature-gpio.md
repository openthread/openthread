# Feature: GPIO Access {#feature-gpio-access}

This feature allows the host to have control over some or all of the
GPIO pins on the NCP. The host can determine which GPIOs are available
by examining `PROP_GPIO_CONFIG`, described below. This API supports a
maximum of 256 individual GPIO pins.

Support for this feature can be determined by the presence of `CAP_GPIO`.

## Properties ##

### PROP 4096: PROP\_GPIO\_CONFIG ###

*   Argument-Encoding: `A(t(CCU))`
*   Type: Read-write (Writable only using `CMD_PROP_VALUE_INSERT`,
    (#cmd-prop-value-insert))

An array of structures which contain the following fields:

*   `C`: GPIO Number
*   `C`: GPIO Configuration Flags
*   `U`: Human-readable GPIO name

GPIOs which do not have a corresponding entry are not supported.

The configuration parameter contains the configuration flags for the
GPIO:

      0   1   2   3   4   5   6   7
    +---+---+---+---+---+---+---+---+
    |DIR|PUP|PDN|TRIGGER|  RESERVED |
    +---+---+---+---+---+---+---+---+
            |O/D|
            +---+

*   `DIR`: Pin direction. Clear (0) for input, set (1) for output.
*   `PUP`: Pull-up enabled flag.
*   `PDN`/`O/D`: Flag meaning depends on pin direction:
    *   Input: Pull-down enabled.
    *   Output: Output is an open-drain.
*   `TRIGGER`: Enumeration describing how pin changes generate
    asynchronous notification commands (TBD) from the NCP to the host.
    *   0: Feature disabled for this pin
    *   1: Trigger on falling edge
    *   2: Trigger on rising edge
    *   3: Trigger on level change
*   `RESERVED`: Bits reserved for future use. Always cleared to zero
    and ignored when read.

As an optional feature, the configuration of individual pins may be
modified using the `CMD_PROP_VALUE_INSERT` command. Only the GPIO
number and flags fields MUST be present, the GPIO name (if present)
would be ignored. This command can only be used to modify the
configuration of GPIOs which are already exposed---it cannot be used
by the host to add addional GPIOs.

### PROP 4098: PROP\_GPIO\_STATE ###

*   Type: Read-Write

Contains a bit field identifying the state of the GPIOs. The length of
the data associated with these properties depends on the number of
GPIOs. If you have 10 GPIOs, you'd have two bytes. GPIOs are numbered
from most significant bit to least significant bit, so 0x80 is GPIO 0,
0x40 is GPIO 1, etc.

For GPIOs configured as inputs:

*   `CMD_PROP_VAUE_GET`: The value of the associated bit describes the
    logic level read from the pin.
*   `CMD_PROP_VALUE_SET`: The value of the associated bit is ignored
    for these pins.

For GPIOs configured as outputs:

*   `CMD_PROP_VAUE_GET`: The value of the associated bit is
    implementation specific.
*   `CMD_PROP_VALUE_SET`: The value of the associated bit determines
    the new logic level of the output. If this pin is configured as an
    open-drain, setting the associated bit to 1 will cause the pin to
    enter a Hi-Z state.

For GPIOs which are not specified in `PROP_GPIO_CONFIG`:

*   `CMD_PROP_VAUE_GET`: The value of the associated bit is
    implementation specific.
*   `CMD_PROP_VALUE_SET`: The value of the associated bit MUST be
    ignored by the NCP.

When writing, unspecified bits are assumed to be zero.

### PROP 4099: PROP\_GPIO\_STATE\_SET ###

*   Type: Write-only

Allows for the state of various output GPIOs to be set without
affecting other GPIO states. Contains a bit field identifying the
output GPIOs that should have their state set to 1.

When writing, unspecified bits are assumed to be zero. The value of
any bits for GPIOs which are not specified in `PROP_GPIO_CONFIG` MUST
be ignored.

### PROP 4100: PROP\_GPIO\_STATE\_CLEAR ###

*   Type: Write-only

Allows for the state of various output GPIOs to be cleared without
affecting other GPIO states. Contains a bit field identifying the
output GPIOs that should have their state cleared to 0.

When writing, unspecified bits are assumed to be zero. The value of
any bits for GPIOs which are not specified in `PROP_GPIO_CONFIG` MUST
be ignored.
