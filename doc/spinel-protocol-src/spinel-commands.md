# Commands

## CMD 0: (Host->NCP) CMD_NOOP {#cmd-noop}

Octets: |    1   |     1
--------|--------|----------
Fields: | HEADER | CMD_NOOP

No-Operation command. Induces the NCP to send a success status back to
the host. This is primarily used for liveliness checks.

The command payload for this command SHOULD be empty. The receiver
MUST ignore any non-empty command payload.

There is no error condition for this command.



## CMD 1: (Host->NCP) CMD_RESET {#cmd-reset}

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



## CMD 2: (Host->NCP) CMD_PROP_VALUE_GET {#cmd-prop-value-get}

Octets: |    1   |          1         |   1-3
--------|--------|--------------------|---------
Fields: | HEADER | CMD_PROP_VALUE_GET | PROP_ID

Get property value command. Causes the NCP to emit a
`CMD_PROP_VALUE_IS` command for the given property identifier.

The payload for this command is the property identifier encoded in the
packed unsigned integer format described in (#packed-unsigned-integer).

If an error occurs, the value of `PROP_LAST_STATUS` will be emitted
instead with the value set to the generated status code for the error.



## CMD 3: (Host->NCP) CMD_PROP_VALUE_SET {#cmd-prop-value-set}

Octets: |    1   |          1         |   1-3   |    *n*
--------|--------|--------------------|---------|------------
Fields: | HEADER | CMD_PROP_VALUE_SET | PROP_ID | VALUE

Set property value command. Instructs the NCP to set the given
property to the specific given value, replacing any previous value.

The payload for this command is the property identifier encoded in the
packed unsigned integer format described in (#packed-unsigned-integer), followed by
the property value. The exact format of the property value is defined
by the property.

If an error occurs, the value of `PROP_LAST_STATUS` will be emitted
with the value set to the generated status code for the error.



## CMD 4: (Host->NCP) CMD_PROP_VALUE_INSERT {#cmd-prop-value-insert}

Octets: |    1   |          1            |   1-3   |    *n*
--------|--------|-----------------------|---------|------------
Fields: | HEADER | CMD_PROP_VALUE_INSERT | PROP_ID | VALUE

Insert value into property command. Instructs the NCP to insert the
given value into a list-oriented property, without removing other
items in the list. The resulting order of items in the list is defined
by the individual property being operated on.

The payload for this command is the property identifier encoded in the
packed unsigned integer format described in (#packed-unsigned-integer), followed by
the value to be inserted. The exact format of the value is defined by
the property.

If the type signature of the property specified by `PROP_ID` consists
of a single structure enclosed by an array (`A(t(...))`), then the
contents of `VALUE` MUST contain the contents of the structure (`...`)
rather than the serialization of the whole item (`t(...)`).  Specifically,
the length of the structure MUST NOT be prepended to `VALUE`. This
helps to eliminate redundant data.

If an error occurs, the value of `PROP_LAST_STATUS` will be emitted
with the value set to the generated status code for the error.



## CMD 5: (Host->NCP) CMD_PROP_VALUE_REMOVE {#cmd-prop-value-remove}

Octets: |    1   |          1            |   1-3   |    *n*
--------|--------|-----------------------|---------|------------
Fields: | HEADER | CMD_PROP_VALUE_REMOVE | PROP_ID | VALUE

Remove value from property command. Instructs the NCP to remove the
given value from a list-oriented property, without affecting other
items in the list. The resulting order of items in the list is defined
by the individual property being operated on.

Note that this command operates *by value*, not by index!

The payload for this command is the property identifier encoded in the
packed unsigned integer format described in (#packed-unsigned-integer), followed by
the value to be removed. The exact format of the value is defined by
the property.

If the type signature of the property specified by `PROP_ID` consists
of a single structure enclosed by an array (`A(t(...))`), then the
contents of `VALUE` MUST contain the contents of the structure (`...`)
rather than the serialization of the whole item (`t(...)`).  Specifically,
the length of the structure MUST NOT be prepended to `VALUE`. This
helps to eliminate redundant data.

If an error occurs, the value of `PROP_LAST_STATUS` will be emitted
with the value set to the generated status code for the error.


## CMD 6: (NCP->Host) CMD_PROP_VALUE_IS {#cmd-prop-value-is}

Octets: |    1   |          1        |   1-3   |    *n*
--------|--------|-------------------|---------|------------
Fields: | HEADER | CMD_PROP_VALUE_IS | PROP_ID | VALUE

Property value notification command. This command can be sent by the
NCP in response to a previous command from the host, or it can be sent
by the NCP in an unsolicited fashion to notify the host of various
state changes asynchronously.

The payload for this command is the property identifier encoded in the
packed unsigned integer format described in (#packed-unsigned-integer), followed by
the current value of the given property.



## CMD 7: (NCP->Host) CMD_PROP_VALUE_INSERTED {#cmd-prop-value-inserted}

Octets: |    1   |            1            |   1-3   |    *n*
--------|--------|-------------------------|---------|------------
Fields: | HEADER | CMD_PROP_VALUE_INSERTED | PROP_ID | VALUE

Property value insertion notification command. This command can be
sent by the NCP in response to the `CMD_PROP_VALUE_INSERT` command, or
it can be sent by the NCP in an unsolicited fashion to notify the host
of various state changes asynchronously.

The payload for this command is the property identifier encoded in the
packed unsigned integer format described in (#packed-unsigned-integer), followed by
the value that was inserted into the given property.

If the type signature of the property specified by `PROP_ID` consists
of a single structure enclosed by an array (`A(t(...))`), then the
contents of `VALUE` MUST contain the contents of the structure (`...`)
rather than the serialization of the whole item (`t(...)`).  Specifically,
the length of the structure MUST NOT be prepended to `VALUE`. This
helps to eliminate redundant data.

The resulting order of items in the list is defined by the given
property.

## CMD 8: (NCP->Host) CMD_PROP_VALUE_REMOVED {#cmd-prop-value-removed}

Octets: |    1   |            1           |   1-3   |    *n*
--------|--------|------------------------|---------|------------
Fields: | HEADER | CMD_PROP_VALUE_REMOVED | PROP_ID | VALUE

Property value removal notification command. This command can be sent
by the NCP in response to the `CMD_PROP_VALUE_REMOVE` command, or it
can be sent by the NCP in an unsolicited fashion to notify the host of
various state changes asynchronously.

Note that this command operates *by value*, not by index!

The payload for this command is the property identifier encoded in the
packed unsigned integer format described in (#packed-unsigned-integer), followed by
the value that was removed from the given property.

If the type signature of the property specified by `PROP_ID` consists
of a single structure enclosed by an array (`A(t(...))`), then the
contents of `VALUE` MUST contain the contents of the structure (`...`)
rather than the serialization of the whole item (`t(...)`).  Specifically,
the length of the structure MUST NOT be prepended to `VALUE`. This
helps to eliminate redundant data.

The resulting order of items in the list is defined by the given
property.


## CMD 18: (Host->NCP) CMD_PEEK {#cmd-peek}

Octets: |    1   |     1    |    4    | 2
--------|--------|----------|---------|-------
Fields: | HEADER | CMD_PEEK | ADDRESS | COUNT

This command allows the NCP to fetch values from the RAM of the NCP
for debugging purposes. Upon success, `CMD_PEEK_RET` is sent from the
NCP to the host. Upon failure, `PROP_LAST_STATUS` is emitted with
the appropriate error indication.

Due to the low-level nature of this command, certain error conditions
may induce the NCP to reset.

The NCP MAY prevent certain regions of memory from being accessed.

The implementation of this command has security implications.
See (#security-considerations) for more information.

This command requires the capability `CAP_PEEK_POKE` to be present.

## CMD 19: (NCP->Host) CMD_PEEK_RET {#cmd-peek-ret}

Octets: |    1   |     1        |    4    | 2     | *n*
--------|--------|--------------|---------|-------|-------
Fields: | HEADER | CMD_PEEK_RET | ADDRESS | COUNT | BYTES

This command contains the contents of memory that was requested by
a previous call to `CMD_PEEK`.

This command requires the capability `CAP_PEEK_POKE` to be present.

## CMD 20: (Host->NCP) CMD_POKE {#cmd-poke}

Octets: |    1   |     1    |    4    | 2     | *n*
--------|--------|----------|---------|-------|-------
Fields: | HEADER | CMD_POKE | ADDRESS | COUNT | BYTES

This command writes the bytes to the specified memory address
for debugging purposes.

Due to the low-level nature of this command, certain error conditions
may induce the NCP to reset.

The implementation of this command has security implications.
See (#security-considerations) for more information.

This command requires the capability `CAP_PEEK_POKE` to be present.

## CMD 21: (Host->NCP) CMD_PROP_VALUE_MULTI_GET {#cmd-prop-value-multi-get}

*   Argument-Encoding: `A(i)`
*   Required Capability: `CAP_CMD_MULTI`

Fetch the value of multiple properties in one command. Arguments are
an array of property IDs. If all properties are fetched successfully,
a `CMD_PROP_VALUES_ARE` command is sent back to the host containing
the propertyid and value of each fetched property. The order of the
results in `CMD_PROP_VALUES_ARE` match the order of properties given
in `CMD_PROP_VALUE_GET`.

Errors fetching individual properties are reflected as indicating a
change to `PROP_LAST_STATUS` for that property's place.

Not all properties can be fetched using this method. As a general rule
of thumb, any property that blocks when getting will fail for that
individual property with `STATUS_INVALID_COMMAND_FOR_PROP`.

## CMD 22: (Host->NCP) CMD_PROP_VALUE_MULTI_SET {#cmd-prop-value-multi-set}

*   Argument-Encoding: `A(iD)`
*   Required Capability: `CAP_CMD_MULTI`

Octets: |    1   |          1               |   *n*
--------|--------|--------------------------|----------------------
Fields: | HEADER | CMD_PROP_VALUE_MULTI_SET | Property/Value Pairs

With each property/value pair being:

Octets: |    2   |   1-3   |  *n*
--------|--------|---------|------------
Fields: | LENGTH | PROP_ID | PROP_VALUE

This command sets the value of several properties at once in the given
order. The setting of properties stops at the first error, ignoring
any later properties.

The result of this command is generally `CMD_PROP_VALUES_ARE` unless
(for example) a parsing error has occured (in which case
`CMD_PROP_VALUE_IS` for `PROP_LAST_STATUS` would be the result). The
order of the results in `CMD_PROP_VALUES_ARE` match the order of
properties given in `CMD_PROP_VALUE_MULTI_SET`.

Since the processing of properties to set stops at the first error,
the resulting `CMD_PROP_VALUES_ARE` can contain fewer items than the
requested number of properties to set.

Not all properties can be set using this method. As a general rule
of thumb, any property that blocks when setting will fail for that
individual property with `STATUS_INVALID_COMMAND_FOR_PROP`.

## CMD 23: (NCP->Host) CMD_PROP_VALUES_ARE {#cmd-prop-values-are}

*   Argument-Encoding: `A(iD)`
*   Required Capability: `CAP_CMD_MULTI`

Octets: |    1   |          1          |   *n*
--------|--------|---------------------|----------------------
Fields: | HEADER | CMD_PROP_VALUES_ARE | Property/Value Pairs

With each property/value pair being:

Octets: |    2   |   1-3   |  *n*
--------|--------|---------|------------
Fields: | LENGTH | PROP_ID | PROP_VALUE

This command is emitted by the NCP as the response to both the
`CMD_PROP_VALUE_MULTI_GET` and `CMD_PROP_VALUE_MULTI_SET` commands. It
is roughly analogous to `CMD_PROP_VALUE_IS`, except that it contains
more than one property.

This command SHOULD NOT be emitted asynchronously, or in response to
any command other than `CMD_PROP_VALUE_MULTI_GET` or
`CMD_PROP_VALUE_MULTI_SET`.

The arguments are a list of structures containing the emitted property
and the associated value. These are presented in the same order as
given in the associated initiating command. In cases where getting or
setting a specific property resulted in an error, the associated slot
in this command will describe `PROP_LAST_STATUS`.
