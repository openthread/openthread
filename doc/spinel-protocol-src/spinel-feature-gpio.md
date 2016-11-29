# Feature: Basic GPIO Access

The length of the data associated with these properties depends on the
number of GPIOs. If you have 10 GPIOs, you'd have two bytes. You determine
the number of GPIOs available by examining PROP_GPIO_AVAILABLE, described
below. This API isn't intended to support every possible GPIO state, it is
intended for basic reading and writing.

## Properties

### PROP 4096: PROP_GPIO_AVAILABLE

* Type: Read-only

Contains a bit field identifying which GPIOs are supported. Cleared bits
are not supported. Set bits are supported.

### PROP 4097: PROP_GPIO_DIRECTION

* Type: Read-only (Optionally read/write)

Contains a bit field identifying which GPIOs are configured as outputs.
Cleared bits are inputs. Set bits are outputs.

### PROP 4098: PROP_GPIO_STATE

* Type: Read-Write

Contains a bit field identifying the state of the GPIOs. For GPIOs
configured as inputs, this is the read logic level. For GPIOs configured
as outputs, this is the logic level of the output.

### PROP 4099: PROP_GPIO_STATE_SET

* Type: Write-only

Allows for the state of various output GPIOs to be set without affecting
other GPIO states. Contains a bit field identifying the output GPIOs that
should have their state set to 1.

### PROP 4100: PROP_GPIO_STATE_CLEAR

* Type: Write-only

Allows for the state of various output GPIOs to be cleared without affecting
other GPIO states. Contains a bit field identifying the output GPIOs that
should have their state cleared to 0.
