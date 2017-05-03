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

## Test Vector: Scan Beacon

* NLI: 0
* TID: 0
* CMD: 7 (`CMD_VALUE_INSERTED`)
* PROP: 51 (`PROP_MAC_SCAN_BEACON`)
* VALUE: Structure, encoded as `Cct(ESSc)t(iCUd)`
    * CHAN: 15
    * RSSI: -60dBm
    * MAC_DATA: (0D 00 B6 40 D4 8C E9 38 F9 52 FF FF D2 04 00)
        * Long address: B6:40:D4:8C:E9:38:F9:52
        * Short address: 0xFFFF
        * PAN-ID: 0x04D2
        * LQI: 0
    * NET_DATA: (13 00 03 20 73 70 69 6E 65 6C 00 08 00 DE AD 00 BE EF 00 CA FE)
        * Protocol Number: 3
        * Flags: 0x20
        * Network Name: `spinel`
        * XPANID: `DE AD 00 BE EF 00 CA FE`

Frame:

    80 07 33 0F C4 0D 00 B6 40 D4 8C E9 38 F9 52 FF FF D2 04 00
    13 00 03 20 73 70 69 6E 65 6C 00 08 00 DE AD 00 BE EF 00 CA
    FE

## Test Vector: Inbound IPv6 Packet

CMD_VALUE_IS(PROP_STREAM_NET)

<!-- RQ -- FIXME: This test vector is incomplete. -->

## Test Vector: Outbound IPv6 Packet

CMD_VALUE_SET(PROP_STREAM_NET)

<!-- RQ -- FIXME: This test vector is incomplete. -->

## Test Vector: Fetch list of on-mesh networks

* NLI: 0
* TID: 4
* CMD: 2 (`CMD_VALUE_GET`)
* PROP: 90 (`PROP_THREAD_ON_MESH_NETS`)

Frame:

    84 02 5A

## Test Vector: Returned list of on-mesh networks

* NLI: 0
* TID: 4
* CMD: 6 (`CMD_VALUE_IS`)
* PROP: 90 (`PROP_THREAD_ON_MESH_NETS`)
* VALUE: Array of structures, encoded as `A(t(6CbC))`

IPv6 Prefix  | Prefix Length | Stable Flag | Other Flags
-------------|---------------|-------------|--------------
2001:DB8:1:: | 64            | True        | ??
2001:DB8:2:: | 64            | False       | ??

Frame:

    84 06 5A 13 00 20 01 0D B8 00 01 00 00 00 00 00 00 00 00 00
    00 40 01 ?? 13 00 20 01 0D B8 00 02 00 00 00 00 00 00 00 00
    00 00 40 00 ??

<!-- TODO: This test vector is incomplete. -->

## Test Vector: Adding an on-mesh network

* NLI: 0
* TID: 5
* CMD: 4 (`CMD_VALUE_INSERT`)
* PROP: 90 (`PROP_THREAD_ON_MESH_NETS`)
* VALUE: Structure, encoded as `6CbCb`

IPv6 Prefix  | Prefix Length | Stable Flag | Other Flags
-------------|---------------|-------------|--------------
2001:DB8:3:: | 64            | True        | ??

Frame:

    85 03 5A 20 01 0D B8 00 03 00 00 00 00 00 00 00 00 00 00 40
    01 ?? 01

<!-- RQ -- FIXME: This test vector is incomplete. -->

## Test Vector: Insertion notification of an on-mesh network

* NLI: 0
* TID: 5
* CMD: 7 (`CMD_VALUE_INSERTED`)
* PROP: 90 (`PROP_THREAD_ON_MESH_NETS`)
* VALUE: Structure, encoded as `6CbCb`

IPv6 Prefix  | Prefix Length | Stable Flag | Other Flags
-------------|---------------|-------------|--------------
2001:DB8:3:: | 64            | True        | ??

Frame:

    85 07 5A 20 01 0D B8 00 03 00 00 00 00 00 00 00 00 00 00 40
    01 ?? 01

<!-- RQ -- FIXME: This test vector is incomplete. -->

## Test Vector: Removing a local on-mesh network

* NLI: 0
* TID: 6
* CMD: 5 (`CMD_VALUE_REMOVE`)
* PROP: 90 (`PROP_THREAD_ON_MESH_NETS`)
* VALUE: IPv6 Prefix `2001:DB8:3::`

Frame:

    86 05 5A 20 01 0D B8 00 03 00 00 00 00 00 00 00 00 00 00

## Test Vector: Removal notification of an on-mesh network

* NLI: 0
* TID: 6
* CMD: 8 (`CMD_VALUE_REMOVED`)
* PROP: 90 (`PROP_THREAD_ON_MESH_NETS`)
* VALUE: IPv6 Prefix `2001:DB8:3::`

Frame:

    86 08 5A 20 01 0D B8 00 03 00 00 00 00 00 00 00 00 00 00


