# Status Codes

Status codes are sent from the NCP to the host via
`PROP_LAST_STATUS` using the `CMD_VALUE_IS` command to indicate
the return status of a previous command. As with any response,
the TID field of the FLAG byte is used to correlate the response
with the request.

Note that most successfully executed commands do not indicate
a last status of `STATUS_OK`. The usual way the NCP indicates a
successful command is to mirror the property change back to the
host. For example, if you do a `CMD_VALUE_SET` on `PROP_PHY_ENABLED`,
the NCP would indicate success by responding with a `CMD_VALUE_IS`
for `PROP_PHY_ENABLED`. If the command failed, `PROP_LAST_STATUS`
would be emitted instead.

See (#prop-last-status) for more information on `PROP_LAST_STATUS`.

 *  0: `STATUS_OK`: Operation has completed successfully.
 *  1: `STATUS_FAILURE`: Operation has failed for some undefined
    reason.
 *  2: `STATUS_UNIMPLEMENTED`: The given operation has not been implemented.
 *  3: `STATUS_INVALID_ARGUMENT`: An argument to the given operation is invalid.
 *  4: `STATUS_INVALID_STATE` : The given operation is invalid for the current
    state of the device.
 *  5: `STATUS_INVALID_COMMAND`: The given command is not recognized.
 *  6: `STATUS_INVALID_INTERFACE`: The given Spinel interface is not supported.
 *  7: `STATUS_INTERNAL_ERROR`: An internal runtime error has occurred.
 *  8: `STATUS_SECURITY_ERROR`: A security or authentication error has occurred.
 *  9: `STATUS_PARSE_ERROR`: An error has occurred while parsing the command.
 *  10: `STATUS_IN_PROGRESS`: The operation is in progress and will be
    completed asynchronously.
 *  11: `STATUS_NOMEM`: The operation has been prevented due to memory
    pressure.
 *  12: `STATUS_BUSY`: The device is currently performing a mutually exclusive
    operation.
 *  13: `STATUS_PROP_NOT_FOUND`: The given property is not recognized.
 *  14: `STATUS_PACKET_DROPPED`: The packet was dropped.
 *  15: `STATUS_EMPTY`: The result of the operation is empty.
 *  16: `STATUS_CMD_TOO_BIG`: The command was too large to fit in the internal
    buffer.
 *  17: `STATUS_NO_ACK`: The packet was not acknowledged.
 *  18: `STATUS_CCA_FAILURE`: The packet was not sent due to a CCA failure.
 *  19: `STATUS_ALREADY`: The operation is already in progress or
    the property was already set to the given value.
 *  20: `STATUS_ITEM_NOT_FOUND`: The given item could not be found in the property. 
 *  21: `STATUS_INVALID_COMMAND_FOR_PROP`: The given command cannot be performed
    on this property.
 *  22-111: RESERVED
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
     *  121-127: RESERVED-RESET-CODES
 *  128 - 15,359: UNALLOCATED
 *  15,360 - 16,383: Vendor-specific
 *  16,384 - 1,999,999: UNALLOCATED
 *  2,000,000 - 2,097,151: Experimental Use Only (MUST NEVER be used
    in production!)

