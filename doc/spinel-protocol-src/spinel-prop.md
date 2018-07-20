# Properties

{{spinel-prop-overview.md}}

## Property Numbering

While the majority of the properties that allow the configuration
of network connectivity are network protocol specific, there are
several properties that are required in all implementations.

Future property allocations **SHALL** be made from the
following allocation plan:

Property ID Range     | Description
:---------------------|:-----------------
0 - 127               | Reserved for frequently-used properties
128 - 15,359          | Technology-specific
15,360 - 16,383       | Vendor-specific
16,384 - 1,999,999    | Technology-specific
2,000,000 - 2,097,151 | Experimental use only

For an explanation of the data format encoding shorthand used
throughout this document, see (#data-packing).

## Property Sections

The currently assigned properties are broken up into several
sections, each with reserved ranges of property identifiers.
These ranges are:

Name         | Range (Inclusive)              | Description
-------------|--------------------------------|------------------------
Core         | 0x000 - 0x01F, 0x1000 - 0x11FF | Spinel core
PHY          | 0x020 - 0x02F, 0x1200 - 0x12FF | Radio PHY layer
MAC          | 0x030 - 0x03F, 0x1300 - 0x13FF | MAC layer
NET          | 0x040 - 0x04F, 0x1400 - 0x14FF | Network
Thread       | 0x050 - 0x05F, 0x1500 - 0x15FF | Thread
IPv6         | 0x060 - 0x06F, 0x1600 - 0x16FF | IPv6
Stream       | 0x070 - 0x07F, 0x1700 - 0x17FF | Stream
MeshCop      | 0x080 - 0x08F, 0x1800 - 0x18FF | Thread Mesh Commissioning
OpenThread   |                0x1900 - 0x19FF | OpenThread specific
Interface    | 0x100 - 0x1FF                  | Interface (e.g., UART)
PIB          | 0x400 - 0x4FF                  | 802.15.4 PIB
Counter      | 0x500 - 0x7FF                  | Counters (MAC, IP, etc).
Nest         |                0x3BC0 - 0x3BFF | Nest (legacy)
Vendor       |                0x3C00 - 0x3FFF | Vendor specific
Debug        |                0x4000 - 0x43FF | Debug related
Experimental |          2,000,000 - 2,097,151 | Experimental use only

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
