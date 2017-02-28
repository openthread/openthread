# Properties

While the majority of the properties that allow the configuration
of network connectivity are network protocol specific, there are
several properties that are required in all implementations.

Future property allocations **SHALL** be made from the
following allocation plan:

Property ID Range     | Description
:---------------------|:-----------------
0 - 127               | Reserved for frequently-used properties
128 - 15,359          | Unallocated
15,360 - 16,383       | Vendor-specific
16,384 - 1,999,999    | Unallocated
2,000,000 - 2,097,151 | Experimental use only

For an explanation of the data format encoding shorthand used
throughout this document, see (#data-packing).

## Property Sections

The currently assigned properties are broken up into several
sections, each with reserved ranges of property identifiers.
These ranges are:

Name   | Range (Inclusive)            | Documentation
-------|------------------------------|--------------
Core   | 0x00 - 0x1F, 0x1000 - 0x11FF | (#prop-core)
PHY    | 0x20 - 0x2F, 0x1200 - 0x12FF | (#prop-phy)
MAC    | 0x30 - 0x3F, 0x1300 - 0x13FF | (#prop-mac)
NET    | 0x40 - 0x4F, 0x1400 - 0x14FF | (#prop-net)
Tech   | 0x50 - 0x5F, 0x1500 - 0x15FF | Technology-specific
IPv6   | 0x60 - 0x6F, 0x1600 - 0x16FF | (#prop-ipv6)
Stream | 0x70 - 0x7F, 0x1700 - 0x17FF | (#prop-core)
Debug  |              0x4000 - 0x4400 | (#prop-debug)

Note that some of the property sections have two reserved
ranges: a primary range (which is encoded as a single byte)
and an extended range (which is encoded as two bytes).
properties which are used more frequently are generally
allocated from the former range.

{{spinel-prop-core.md}}

{{spinel-prop-phy.md}}

{{spinel-prop-mac.md}}

{{spinel-prop-net.md}}

{{spinel-prop-ipv6.md}}

{{spinel-prop-debug.md}}
