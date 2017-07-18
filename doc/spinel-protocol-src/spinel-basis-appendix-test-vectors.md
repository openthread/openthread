# Test Vectors

## Test Vector: Packed Unsigned Integer

Decimal Value | Packet Octet Encoding
-------------:|:----------------------
            0 | `00`
            1 | `01`
          127 | `7F`
          128 | `80 01`
          129 | `81 01`
        1,337 | `B9 0A`
       16,383 | `FF 7F`
       16,384 | `80 80 01`
       16,385 | `81 80 01`
    2,097,151 | `FF FF 7F`

<!-- RQ -- The PUI test-vector encodings need to be verified. -->

## Test Vector: Reset Command

* NLI: 0
* TID: 0
* CMD: 1 (`CMD_RESET`)

Frame:

    80 01

## Test Vector: Reset Notification

* NLI: 0
* TID: 0
* CMD: 6 (`CMD_VALUE_IS`)
* PROP: 0 (`PROP_LAST_STATUS`)
* VALUE: 114 (`STATUS_RESET_SOFTWARE`)

Frame:

    80 06 00 72

## Test Vector: Inbound IPv6 Packet

CMD_VALUE_IS(PROP_STREAM_NET)

<!-- RQ -- FIXME: This test vector is incomplete. -->

## Test Vector: Outbound IPv6 Packet

CMD_VALUE_SET(PROP_STREAM_NET)

<!-- RQ -- FIXME: This test vector is incomplete. -->
