# SRP Coder

This document specifies SRP coder used to encode an SRP message into a compact, compressed format, reducing the message size. On the server, the received coded message can be decoded to reconstruct an exact replica of the original message.

The SRP protocol uses DNS resource record format to convey information about registered services and host addresses. The use of these records and adherence to the DNS Update general format introduces overhead and can lead to larger SRP message sizes, which is undesirable for Thread network. The SRP coder aims to optimize the message, resulting in much smaller messages containing all the same information reducing overall protocol overhead.

## Compact Integer Format

This format efficiently encodes unsigned integers of varying bit-lengths (e.g., `uint16`, `uint32`). The unsigned integer value is encoded as one or more segments:

- The first segment can be 2-bit to 8-bit long. Its bit-length is specified based on the context where the compact encoding is used. If not explicitly specified, an 8-bit length MUST be assumed for the first segment.
- Subsequent segments are one byte (8-bit) long.

The most significant bit in each segment (regardless of its bit-length) is the "continuation bit":

- A value of `1` indicates more segments follow.
- A value of `0` indicates that this is the last segment.

The remaining bits after the most significant bit provide the numerical `uint` bit value chunks and follow big-endian order.

```
    First segment (2-8 bits)          Next segments (8 bits)
   +---+---+---+---+---+---+         +---+---+---+---+---+---+---+---+
   | C |  int value        |         | C |        7-bit value        |
   +---+---+---+---+---+---+         +---+---+---+---+---+---+---+---+
```

Similar integer coding schemes ([Variable-Length Quantity (VLQ)](https://en.wikipedia.org/wiki/Variable-length_quantity)) are used in other protocols.

### Examples

Assuming the first segment is one byte (8-bit) long:

- Numbers less than (2^7) 128 are encoded as a single byte.
- Numbers between 128 and (2^14 - 1) 16,383 are encoded as two bytes.
- Numbers between 16,384 and (2^21 - 1) 2,097,151 are encoded as three bytes.

Value `4460 = 0x1234` is encoded as two bytes: `[0xa4, 0x34]`

```
                             (0x1234)
  +---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+
  | 0   0   0   1   0   0   1   0 | 0   0   1   1   0   1   0   0 |
  +---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+
          /         7-bit            / \         7-bit             \
         /                          /   \                           \
        /                          /     \                           \
   +---+---+---+---+---+---+---+---+  +---+---+---+---+---+---+---+---+
   | 1 | 0   1   0   0   1   0   0 |  | 0 | 0   1   1   0   1   0   0 |
   +---+---+---+---+---+---+---+---+  +---+---+---+---+---+---+---+---+
              (0xa4)                             (0x34)
```

Assuming the first segment is 6-bit long:

- Numbers less than (2^5) 32 are encoded in the first segment.
- Numbers between 32 and (2^12 - 1) 4,095 use two segments (the first 6-bit segment and an additional byte).

## Offset Definition

Numerical "offset" values are sometimes used in the following sections to refer to previous data within the coded message. These offset values are always relative to the start of the coded message, where the first byte is considered offset zero (`0`).

## DNS Name and Label Format

### Label Dispatch

```
    0   1   2   3   4   5   6   7
  +---+---+---+---+---+---+---+---+
  | Type  ... | ...               |
  +---+---+---+---+---+---+---+---+

```

- `00 <6-bit len>`: Standard label encoding, where the lower 6 bits indicate the label's length (number of characters). This is followed by the label characters, totaling `len` in number.
- `01 <6-bit len>`: Similar to the above, but the label begins with an underscore `'_'`. The `len` value here doesn't include the underscore itself, which isn't explicitly encoded after the dispatch byte.
- `10 <6-bit offset first seg>`: This references a previously encoded label within the message. The 6-bit value in the dispatch byte serves as the first segment of a compact `uint` representing the offset to that label. This offset must point to the label dispatch byte of the previous label.
  - Note: This mechanism applies to a single label only. Any subsequent labels are encoded directly after the current one. This differs from traditional DNS pointer name compression, where a pointer indicates that the remaining labels should be read from the specified offset.
- `110 <5-bit code>`: Commonly used constant labels. Codes are:
  - `0`: `_udp`
  - `1`: `_tcp`
  - `2`: `_matter`
  - `3`: `_matterc`
  - `4`: `_matterd`
  - `5`: `_hap`
- `111 <5-bit code>`: Commonly Used Generative Patterns
  - `0`: `<hex_value>` - 16-character uppercase hexadecimal 64-bit value.
    - Example: `DAAFF10F39B00F32`
    - Encoding: After the label dispatch byte, the 64-bit value is encoded as 8-byte binary format (big-endian order).
  - `1`: `<hex_value_1>-<hex_value_2>` - Two 16-character uppercase hexadecimals, separated by a hyphen `-`
    - Example `AABBCCDDEEFF0011-1122334455667788`.
    - Encoding: After the label dispatch byte, the two 64-bit values are encoded (each as 8-byte binary format big-endian).
  - `2`: Subtype label `_<char><hex_value>` - starting with an underscore `_`, followed by a single character `<char>`, ending with a 16-character uppercase hexadecimal 64-bit value `<hex_value>`.
    - Example: `_IAA557733CC00EE11`
    - Encoding: After the label dispatch byte, `<char>` is encoded as a single byte, followed by the 8-byte big-endian binary representation of the 64-bit value `<hex_value>`.
  - `3`: Subtype Label - same as the previous case, `_<char><hex_value>` - starting with an underscore `_`, followed by a single character `<char>`, ending with a 16-character uppercase hexadecimal 64-bit value `<hex_value>`.
    - Example: `_IAA557733CC00EE11`
    - This is similar to the previous case, except instead of directly encoding the 8-byte value, it is replaced by an offset, allowing reuse of an earlier encoding of the same 64-bit value.
    - Encoding: After the label dispatch byte, `<char>` is encoded as a single byte. This is followed by an offset encoded using the compact `uint` format. The offset points to the first byte of an earlier occurrence of the same 8-byte sequence within the message.

When multiple labels are encoded, the end is indicated by an empty label (encoded as a single `0x00` byte).

#### Examples

`_service._udp` is encoded using `10` bytes (vs normal DNS encoding of `15`):

```
  +----+----+----+----+----+----+----+----+----+----+
  | 47 | s  | e  | r  | v  | i  | c  | e  | C0 | 00 |
  +----+----+----+----+----+----+----+----+----+----+
```

- `0x47` (`0b0100_0111`) - label starting with `_` with `len=7`.
- `0xC0` (`0b110_0000`) - Commonly used constant label - code 0 for `_udp`.
- `0x00` end.

`2906C908D115D362-8FC7772401CD0696` (matter service instance label example) encoded using `17` bytes (vs uncompressed DNS encoding of `34` bytes).

- `0xE1` (`0b1110_0001`) - Commonly used generative patterns (code 1)
- `0x29` `0x06` `0xC9` `0x08` `0xD1` `0x15` `0xD3` `0x62` (first 64-bit value)
- `0x8F` `0xC7` `0x77` `0x24` `0x01` `0xCD` `0x06` `0x96` (second 64-bit value)

## Message Format

- [Header Block](#header-block)
- Zero or more
  - [Add Service Block](#add-service-block) or [Remove Service Block](#remove-service-block)
- [Host Block](#host-block)
- [Footer Block](#footer-block)

## Header Block

### Header Dispatch

```
    0      1     2     3     4     5     6     7
  +-----+-----+-----+-----+-----+-----+-----+-----+
  |  0  |  0  |  1  |  0  |  1  |  1  |  Z  |  T  |
  +-----+-----+-----+-----+-----+-----+-----+-----+
```

- The initial bits in dispatch byte are fixed to differentiate it from the general DNS header used in SRP updates.
- `Z` flag (Zone)
  - `0`: Zone name is elided - use the default `default.service.arpa`.
  - `1`: Zone name is encoded within the message.
- `T` flag (Default TTL)
  - `0`: Default TTL is elided - use `7200` (2 hours).
  - `1`: Default TTL is encoded in the message using compact integer format.

### Header Block Format

- Message ID: Two bytes, big-endian.
- Dispatch byte.
- Zone (domain) name: Included if not elided (indicated by the `Z` flag).
- Default TTL value: Encoded using compact integer format if not elided (indicated by the `T` flag).
- Host labels: Excludes the zone name and ends with an empty label.

The zone (domain name) when not elided is encoded as a sequence of labels following the rules from the "DNS Name and Label Format" section. In particular, the labels must end with an empty label (encoded as a single `0x00` byte).

#### Distinguishing from SRP Update Message

An SRP Update message uses the following DNS Update header format (RFC2136 section 2.2):

```
                                  1  1  1  1  1  1
    0  1  2  3  4  5  6  7  8  9  0  1  2  3  4  5
  +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
  |                      ID                       |
  +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
  |QR|   Opcode  |          Z         |   RCODE   |
  +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+

```

The first 3 bytes can be used to differentiate a coded message from an SRP Update message. In the coded message, the message ID is encoded in the first two bytes, similar to a general DNS header. The fixed initial bit pattern in the header dispatch byte, when mapped to the DNS header, corresponds to `QR = 0`, `Opcode = 0b0101 = 5`, with bit 5 also set. An SRP Update message also employs `Opcode = 5` but with zero flags (`Z`). Setting bit 5 to `1` is used to identify a coded message.

## General Dispatch Byte Markers

Following the header, each subsequent block begins with a dispatch byte. The initial bits within a dispatch byte designate its type:

- `00`: Add service
- `01`: Remove service
- `10`: Host
- `110`: Footer

Any unused/reserved bits in dispatch bytes MUST be set to zero during encoding and its value MUST be ignored during decoding.

## Add Service Block

### Add Service Dispatch

```
    0      1     2     3     4     5     6     7
  +-----+-----+-----+-----+-----+-----+-----+-----+
  |  0  |  0  | PT  | ST  | SUB | PRI | WGT | TXT |
  +-----+-----+-----+-----+-----+-----+-----+-----+
```

- `PT` flag (PTR TTL) - `0`: Elide PTR TTL (use default TTL from header), `1`: TTL encoded.
- `ST` flag (SRV/TXT TTL) - `0`: Elide SRV/TXT TTL (use default TTL from header), `1`: TTL encoded.
- `SUB` flag (sub-type) - `0`: No sub-types, `1`: Sub-types labels encoded.
- `PRI` flag (priority) - `0`: Elide priority (use zero), `1`: Priority encoded.
- `WGT` flag (weight) - `0`: Elide weight (use zero), `1`: Weight encoded.
- `TXT` flag (TXT data) - `0`: No TXT data (elided), `1`: TXT data encoded.

### Add Service Block Format

- "Add Service Dispatch" byte
- PTR TTL (if not elided)
- SRV/TXT TTL (if not elided)
- Service instance label
- Service name labels (excluding the zone name).
- SubType labels (if not elided) - A series of labels, one per subtype, terminated by a single empty label (i.e., single `0x00` byte).
- Port
- Priority (if not elided)
- Weight (if not elided)
- TXT Data block - if not elided.

TTLs, port, priority, and weight fields (when not elided) are encoded using the compact integer format.

TXT Data block (when not elided) starts with a TXT Data Dispatch byte.

#### TXT Data Dispatch

- `0 <7-bit len - first segment>`: TXT data is encoded directly. The data size is encoded using the compact integer format, with the first segment utilizing the 7 bits in the dispatch byte. This is followed by the TXT data bytes of the specified length.
- `1 <7-bit offset - first segment>`: Refers to previously encoded TXT data within the message. The offset is encoded using the compact integer format, with the first segment utilizing the 7 bits in the dispatch byte. The offset MUST point to a previous existing TXT data block within the message.

## Remove Service Block

### Remove Service Dispatch

```
    0      1     2     3     4     5     6     7
  +-----+-----+-----+-----+-----+-----+-----+-----+
  |  0  |  1  |     (unused)                      |
  +-----+-----+-----+-----+-----+-----+-----+-----+
```

### Remove Service Block Format

- “Remove Service Dispatch” byte
- Service instance label
- Service name labels (excludes zone name)

## Host Block

### Host Dispatch

```
    0      1     2     3     4     5     6     7
  +-----+-----+-----+-----+-----+-----+-----+-----+
  |  1  |  0  | AT  | ADR | KT  | KEY | (unused)  |
  +-----+-----+-----+-----+-----+-----+-----+-----+
```

- `AT` flag (Address TTL) - `0` AAAA TTL elided (use default from header), `1`: TTL encoded.
- `ADR` flag (Address list) - `0`: Address list elided (no addresses), `1`: Address list encoded.
- 'KT flag (Key TTL) - `0`: Key TTL elided (use default from header), `1`: TTL encoded
- `KEY` flag (Key) - `0`: Key elided, `1`: Key encoded.

### Host Block Format

- "Host Dispatch" byte
- AAAA record TTL (if not elided)
- Address list (if not empty based on `ADR` flag) - Starts with Address Dispatch byte
- Key TTL (if not elided)
- Key (if not elided) - 64 bytes

TTL fields (when not elided) are encoded using the compact integer format.

When the address list is not empty, each address begins with an Address Dispatch byte, followed by the specific information indicated by that dispatch byte.

### Address Dispatch

```
    0      1     2     3     4     5     6     7
  +-----+-----+-----+-----+-----+-----+-----+-----+
  |  C  |  M  | (unused)  |    Context ID         |
  +-----+-----+-----+-----+-----+-----+-----+-----+
```

- `C` flag (Context compression)
  - `0`: IPv6 address fully encoded (16 bytes) after dispatch.
  - `1`: Context compression used. Context ID provided in lower 4 bits. Address IID (8 bytes) encoded after dispatch.
- `M` flag (More addresses)
  - `0`: This is the last address in the list.
  - `1`: There are more addresses in the list.

When context compression is used, the corresponding prefix is determined using Thread Network Data and the specified Context ID, similar to 6LoWPAN IPv6 Header compression. Specifically, to determine the Context ID from Thread Network Data, the encoder MUST check that the "compress" bit in the corresponding 6LoWPAN ID TLV is set before using the Context ID and compressed format. The decoder MUST NOT check the "compress" bit when uncompressing the address.

## Footer Block

### Footer Dispatch

```
    0      1     2     3     4     5     6     7
  +-----+-----+-----+-----+-----+-----+-----+-----+
  |  1  |  1  |  0  | LS  | KLS |     |   SIG     |
  +-----+-----+-----+-----+-----+-----+-----+-----+
```

- `LS` flag (Lease) - `0`: Lease elided (assume `7200`). `1`: Lease encoded
- `KLS` flag (Key lease) - `0`: Key lease elided (assume `1209600`). `1`: Key lease encoded
- `SIG` code (2 bit value)
  - `00` - signature elided
  - `01` - Full signature (64 bytes)
  - `10` - Short signature (TBD)
  - `11` - Unused

### Footer Block Format

- "Footer Dispatch" byte
- Lease value (if not elided)
- Key Lease value (based on the dispatch)
- Signature bytes (based on dispatch)

Lease and key lease fields (when not elided) are encoded using the compact integer format.

Note that the recommended lease interval of `7200` (2 hours) and key lease interval of `1209600` (14 days) are elided. These are default values often used by SRP clients.

## Decoding Rules

Decoding rules govern the reconstruction of the SRP message. They dictate the sequence in which resource records are added and the proper usage of DNS name compression (pointer names). This defines a consistent and unique format for the SRP message, which is essential for signature validation. The encoder MUST calculate the appended signature over an SRP message that adheres to the same prescribed order and format. This guarantees that the signature can be accurately validated against the reconstructed message, ensuring it matches the original message used for signature generation.

Domain name (e.g., "default.service.arpa") is included in the SRP update message within the Zone section. Subsequent references to this zone name within other DNS names MUST use pointers (DNS name compression).

The first occurrence of the host name (e.g., in an SRV record or an AAAA record) directly appends its labels (preceding the zone name). For any subsequent appearances of the host name (in SRV, AAAA, KEY records), pointer names (name compression) MUST be employed to refer to its first appearance.

#### An "Add Service Block" is decoded as follows:

1. A PTR record mapping the service name to the service instance.
2. PTR records for any sub-types (if present).
3. A "Delete all RRsets from a name" record for the service instance name.
4. An SRV record for the service instance.
5. A TXT record for the service instance.

- The service type labels (e.g., `_matter._tcp`) in the first PTR record MUST be appended directly, even if the same service name appears earlier in the message. Pointer names (compressed names) MUST NOT be used in this case.
- The service instance name in the first PTR record appends the instance label directly, followed by a pointer to the service type name from the PTR record name. After the first PTR record, a pointer to this name MUST be used to refer to the service instance name.
- The first sub-type PTR appends the sub-type label, followed by the `_sub` label with a pointer to the service name. Any subsequent sub-type appends the sub-type label, followed by a pointer to the `._sub` name from the first sub-type PTR.
- If TXT data is elided, a TXT record is still appended with a single zero byte (empty TXT data).

#### A “Remove Service block” is decoded as:

1. A PTR as “Delete an RR from an RRSet" where class is set to NONE and TTL to zero (RFC 2136 - section 2.5.4)

#### The “Host Block” is decoded as follows:

1. A “Delete all RRsets from a name” for the host name.
2. For each address in order they appear in the encoded message, an AAAA record is appended.
3. If key is provided, a KEY record is appended for the host name

#### The “Footer Block” is decoded as follows:

1. An OPT record is appended with single Lease Option (in Additional Data section)
   - Empty (root domain) is used for OPT RR name
   - The long variant of Lease Option is used setting lease and key lease intervals
2. If signature is provided, a SIG record is appended following SRP protocol specification.
