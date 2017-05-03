# Frame Format ##

A frame is defined simply as the concatenation of

 *  A header byte
 *  A command (up to three bytes, see (#packed-unsigned-integer) for format)
 *  An optional command payload

Octets: |    1   | 1-3 |    *n*
--------|--------|-----|-------------
Fields: | HEADER | CMD | CMD_PAYLOAD


## Header Format ###

The header byte is broken down as follows:

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

### FLG: Flag

The flag field of the header byte (`FLG`) is always set to the value
two (or `10` in binary). Any frame received with these bits set to
any other value else MUST NOT be considered a Spinel frame.

This convention allows Spinel to be line compatible with BTLE HCI. By
defining the first two bit in this way we can disambiguate between
Spinel frames and HCI frames (which always start with either `0x01`
or `0x04`) without any additional framing overhead.

### NLI: Network Link Identifier

The Network Link Identifier (NLI) is a number between 0 and 3, which is associated by the OS with one of up to four IPv6 zone indices corresponding to conceptual IPv6 interfaces on the NCP. This allows the protocol to support IPv6 nodes connecting simultaneously to more than one IPv6 network link using a single NCP instance. The first Network Link Identifier (0) MUST refer to a distinguished conceptual interface provided by the NCP for its IPv6 link type. The other three Network Link Identifiers (1, 2 and 3) MAY be dissociated from any conceptual interface.

### TID: Transaction Identifier

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

### Command Identifier (CMD) ####

The command identifier is a 21-bit unsigned integer encoded in up to
three bytes using the packed unsigned integer format described in
(#packed-unsigned-integer). This encoding allows for up to 2,097,152 individual
commands, with the first 127 commands represented as a single byte.
Command identifiers larger than 2,097,151 are explicitly forbidden.

CID Range             | Description
----------------------|------------------
0 - 63                | Reserved for core commands
64 - 15,359           | *UNALLOCATED*
15,360 - 16,383       | Vendor-specific
16,384 - 1,999,999    | *UNALLOCATED*
2,000,000 - 2,097,151 | Experimental use only

### Command Payload (Optional) ####

Depending on the semantics of the command in question, a payload MAY
be included in the frame. The exact composition and length of the
payload is defined by the command identifier.


