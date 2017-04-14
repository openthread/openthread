# Properties

This section defines the syntax and operational semantics of the Spinel properties common to the basis protocol.

Spinel NCP implementations MAY send any of the following commands, at any time, using any of the `PROP_ID` values defined for the basis protocol:

* `CMD_PROP_VALUE_IS`
* `CMD_PROP_VALUE_INSERTED`
* `CMD_PROP_VALUE_REMOVED`
* `CMD_PROP_VALUE_ARE`

Spinel OS implementations MAY send any of the following commands, at any time, using any of the `PROP_ID` values defined for the basis protocol:

* `CMD_PROP_VALUE_GET`
* `CMD_PROP_VALUE_SET`
* `CMD_PROP_VALUE_INSERT`
* `CMD_PROP_VALUE_REMOVE`
* `CMD_PROP_VALUE_MULTI_GET`
* `CMD_PROP_VALUE_MULTI_SET`

Spinel NCP implementations SHOULD implement the operational semantics for all the properties defined here, except where noted that a property is REQUIRED for the NCP to implement. If the NCP implementation receives one of the commands above with a `PROP_ID` value that it does not implement, then it MUST reply with a `CMD_PROP_VALUE_IS` for the `PROP_LAST_STATUS` property identifier with a `STATUS` value of `STATUS_PROP_NOT_FOUND`.

## Property Identifiers

IANA maintains a registry of Spinel `PROP_ID` property identifier numbers, with varying registration policies assigned for different ranges according to the following table:

Property ID Range     | Description
:---------------------|:-----------------
0 - 127               | Standards Action
128 - 4,095           | Unassigned
4,096 - 6,143         | Standards Action
6,144 - 15,359        | Unassigned
15,360 - 16,383       | Private Use
16,384 - 17,407       | Standards Action
17,408 - 1,999,999    | Unassigned
2,000,000 - 2,097,151 | Experimental Use

## Property Identifier Sections

Standard property identifier numbers are assigned in a hierarchy according to their purpose, as shown in the table below:

Name   | Primary    | Extended          | Documentation
:------|:-----------|:------------------|:-------------
Core   | 0 - 31     | 4,096 - 4,607     | (#prop-core)
PHY    | 32 - 47    | 4,608 - 4,863     | Physical (PHY) layer specific
MAC    | 48 - 63    | 4,864 - 5,119     | Media access (MAC) layer specific
NET    | 64 - 79    | 5,120 - 5,375     | (#prop-net)
Tech   | 80 - 95    | 5,376 - 5,631     | Technology specific
IPv6   | 96 - 111   | 5,632 - 5,887     | (#prop-ipv6)
Stream | 112 - 127  | 5,888 - 6,143     | (#prop-core)
Debug  | no primary | 16,384 - 17,407   | (#prop-debug)

Note: most of the property identifier sections have two reserved ranges: a "primary" range (which is encoded as a single octet) and an "extended" range (which is encoded as two octets). Properties used very frequently are generally allocated from the "primary" range.

EDITOR: the IANA registration template for Spinel standard properties identifiers requires the "Section Name" and "Range Identifier" parameters to facilitate the assignment of a suitable number from the appropriate range.

{{spinel-basis-detail-properties-core.md}}

{{spinel-basis-detail-properties-net.md}}

{{spinel-basis-detail-properties-ipv6.md}}

{{spinel-basis-detail-properties-debug.md}}
