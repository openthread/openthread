# Status Codes

Status codes are sent from the NCP to the OS via `PROP_LAST_STATUS` using the `CMD_VALUE_IS` command to indicate the return status of a previous command. As with any response, the TID field of the FLAG byte is used to correlate the response with the request.

Note that most successfully executed commands do not indicate a last status of `STATUS_OK`. The usual way the NCP indicates a successful command is to mirror the property change back to the OS. For example, if you do a `CMD_VALUE_SET` on `PROP_PHY_ENABLED`, the NCP would indicate success by responding with a `CMD_VALUE_IS` for `PROP_PHY_ENABLED`. If the command failed, `PROP_LAST_STATUS` would be emitted instead.

See (#prop-last-status) for more information on `PROP_LAST_STATUS`.

IANA maintains a registry of Spinel `STATUS_CODE` numbers, with varying registration policies assigned for different ranges according to the following table:

Status Code Range     | Reservation Policy
:---------------------|:-----------------
0 - 127               | Standards Action
128 - 15,359          | Unassigned
15,360 - 16,383       | Private Use
16,384 - 1,999,999    | Unassigned
2,000,000 - 2,097,151 | Experimental Use

The Spinel basis protoocol defines some standard status codes. Their names, assigned numbers and a short description of their operational semantics are shown in the following table:

Status Code | Name                              | Description
:-----------|:----------------------------------|:----------------------------------------------------------------------------
  0         | `STATUS_OK`                       | Operation has completed successfully.
  1         | `STATUS_FAILURE`                  | Operation has failed for some undefined reason.
  2         | `STATUS_UNIMPLEMENTED`            | The given operation has not been implemented.
  3         | `STATUS_INVALID_ARGUMENT`         | An argument to the given operation is invalid.
  4         | `STATUS_INVALID_STATE`            | The given operation is invalid for the current state of the device.
  5         | `STATUS_INVALID_COMMAND`          | The given command is not recognized.
  6         | `STATUS_INVALID_INTERFACE`        | The given network link identifier is not supported.
  7         | `STATUS_INTERNAL_ERROR`           | An internal runtime error has occurred.
  8         | `STATUS_SECURITY_ERROR`           | A security or authentication error has occurred.
  9         | `STATUS_PARSE_ERROR`              | An error has occurred while parsing the command.
 10         | `STATUS_IN_PROGRESS`              | An error has occurred while parsing the command.
 11         | `STATUS_NOMEM`                    | The operation has been prevented due to memory pressure.
 12         | `STATUS_BUSY`                     | The device is currently performing a mutually exclusive operation.
 13         | `STATUS_PROP_NOT_FOUND`           | The given property is not recognized.
 14         | `STATUS_PACKET_DROPPED`           | The packet was dropped.
 15         | `STATUS_EMPTY`                    | The result of the operation is empty.
 16         | `STATUS_CMD_TOO_BIG`              | The command was too large to fit in the internal buffer.
 17         | `STATUS_NO_ACK`                   | The packet was not acknowledged.
 18         | `STATUS_CCA_FAILURE`              | The packet was not sent due to a CCA failure.
 19         | `STATUS_ALREADY`                  | The operation is already in progress, or the property already has the value.
 20         | `STATUS_ITEM_NOT_FOUND`           | The given item could not be found in the property. 
 21         | `STATUS_INVALID_COMMAND_FOR_PROP` | The given command cannot be performed on this property.
112         | `STATUS_RESET_POWER_ON`           | Cold power-on start.
113         | `STATUS_RESET_EXTERNAL`           | External device reset.
114         | `STATUS_RESET_SOFTWARE`           | Internal software orderly reset.
115         | `STATUS_RESET_FAULT`              | Internal software abortive reset.
116         | `STATUS_RESET_CRASH`              | Unrecoverable software execution failure.
117         | `STATUS_RESET_ASSERT`             | Software invariant property not respected.
118         | `STATUS_RESET_OTHER`              | Unspecified cause.
119         | `STATUS_RESET_UNKNOWN`            | Failure while recovering cause of reset.
120         | `STATUS_RESET_WATCHDOG`           | Software failed to make sufficient progress.

EDITOR: The `STATUS_CCA_FAILURE` status is a technology-specific status code. Should not be in the basis protocol.
