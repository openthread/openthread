# Frame Format #

A frame is the concatenation of the following elements:

 *  A header comprising a single octet (see (#header-format) below).
 *  A command identifier (up to three octets, see (#packed-unsigned-integer) for format)
 *  An optional command payload

Octets: |    1   | 1-3 |    *n*
--------|--------|-----|-------------
Fields: | HEADER | CMD | CMD_PAYLOAD

Each of the property operators described in the previous section is defined as a command with a different identifier and a payload according to the property type. Additional commands are defined for special purposes (see (#commands)), and the command identifier registry has values reserved for future standard expansion, application specialization, and experimental purposes.

## Header Format ##

The header comprises the following information elements packed into a single octet:

      0   1   2   3   4   5   6   7
    +---+---+---+---+---+---+---+---+
    |  FLG  |  NLI  |      TID      |
    +---+---+---+---+---+---+---+---+

<!-- RQ -- Eventually, when https://github.com/miekg/mmark/issues/95
is addressed, the above table should be swapped out with this:

| 0 | 1 | 2 | 3 | 4 | 5 | 6 | 7 |
|---|---|---|---|---|---|---|---|
|  FLG ||  NLI ||      TID   ||||
-->

### FLG: Flag ###

The Flag (FLG) field in the two most significant bits of the header octet (`FLG`) is always set to the value two (or `10` in binary). Any frame received with these bits set to any other value else MUST NOT be considered a Spinel frame.

This convention allows Spinel to be line compatible with BTLE HCI. By defining the first two bit in this way we can disambiguate between Spinel frames and HCI frames (which always start with either `0x01` or `0x04`) without any additional framing overhead.

### NLI: Network Link Identifier ###

The Network Link Identifier (NLI) field in the third and fourth most significant bits is a number between 0 and 3, which is associated by the OS with one of up to four IPv6 zone indices corresponding to conceptual IPv6 interfaces on the NCP. This allows the protocol to support IPv6 nodes connecting simultaneously to more than one IPv6 network link using a single NCP instance. The zero value of NLI is reserved, and it MUST refer to a distinguished conceptual interface provided by the NCP for its IPv6 link type. The other three NLI numbers (1, 2 and 3) MAY be dissociated from any conceptual interface.

### TID: Transaction Identifier ###

The Transaction Identifier (TID) field in the four least significant bits of the header is used for correlating responses to the
commands which generated them.

When a command is sent from the OS, any reply to that command sent by the NCP will use the same value for the TID. When the OS receives a frame that matches the TID of the command it sent, it can easily recognize that frame as the actual response to that command.

The zero value of TID is used for commands to which a correlated response is not expected or needed, such as for unsolicited update
commands sent to the OS from the NCP.

## Command Identifier (CMD) ##

The command identifier is a 21-bit unsigned integer encoded in up to three octets using the packed unsigned integer format described in (#packed-unsigned-integer). This encoding allows for up to 2,097,152 individual commands, with the first 127 commands represented as a single octet. Command identifiers larger than 2,097,151 are explicitly forbidden.

### Command Payload (Optional) ###

The operational semantics of each command definition determine whether a payload of non-zero length is included in the frame. If included in the frame, then the exact composition of a command payload is determined by solely the command identifier.

