# IPv6 Addressing

Let's take a look at how Thread identifies each device in the network, and what
types of addresses they use to communicate with each other.

Key Term: In this primer, the term "interface" is used to identify an endpoint
of a Thread device within a network. Typically, a single Thread device has
a single Thread interface.

## Scopes

<figure class="attempt-right">
<a href="../images/ot-primer-scopes_2x.png"><img src="../images/ot-primer-scopes.png" srcset="../images/ot-primer-scopes.png 1x, ../images/ot-primer-scopes_2x.png 2x" border="0" alt="OT Scopes" /></a>
</figure>

There are three scopes in a Thread network for unicast addressing:

*   Link-Local — all interfaces reachable by a single radio transmission
*   Mesh-Local — all interfaces reachable within the same Thread network
*   Global — all interfaces reachable from outside a Thread network

The first two scopes correspond to prefixes designated by a Thread network.
Link-Local have prefixes of `fe80::/16`, while Mesh-Local have prefixes of
`fd00::/8`.

<h2 style="clear:right">Unicast</h2>

There are multiple IPv6 unicast addresses that identify a single Thread device.
Each has a different function based on the scope and use case.

Before we detail each type, let's learn more about a common one, called the
Routing Locator (RLOC). The RLOC identifies a Thread interface, based on its
location in the network topology.

### How a Routing Locator is generated

All devices are assigned a Router ID and a Child ID. Each Router maintains a
table of all their Children, the combination of which uniquely identifies a
device within the topology.  For example, consider the highlighted nodes in the
following topology, where the number in a Router (pentagon) is the Router ID,
and the number in an End Device (circle) is the Child ID:

<figure>
<a href="../images/ot-primer-rloc-topology_2x.png"><img src="../images/ot-primer-rloc-topology.png" srcset="../images/ot-primer-rloc-topology.png 1x, ../images/ot-primer-rloc-topology_2x.png 2x" border="0" width="600" alt="OT RLOC Topology" /></a>
</figure>

Each Child's Router ID corresponds to their Parent (Router). Because a Router is
not a Child, the Child ID for a Router is always 0. Together, these values are
unique for each device in the Thread network, and are used to create the RLOC16,
which represents the last 16 bits of the RLOC.

For example, here's how the RLOC16 is calculated for the upper-left node (Router
ID = 1 and Child ID = 1):

<figure>
<a href="../images/ot-primer-rloc16_2x.png"><img src="../images/ot-primer-rloc16.png" srcset="../images/ot-primer-rloc16.png 1x, ../images/ot-primer-rloc16_2x.png 2x" border="0" width="400" alt="OT RLOC16" /></a>
</figure>

The RLOC16 is part of the Interface Identifier (IID), which corresponds to the
last 64 bits of the IPv6 address. Some IIDs can be used to identify some types
of Thread interfaces. For example, the IID for RLOCs is always of the form
<code>0000:00ff:fe00:<var>RLOC16</var></code>.

The IID, combined with a Mesh-Local Prefix, results in the RLOC. For example,
using a Mesh-Local Prefix of `fde5:8dba:82e1:1::/64`, the RLOC for a node where
RLOC16 = `0x401` is:

<figure>
<a href="../images/ot-primer-rloc_2x.png"><img src="../images/ot-primer-rloc.png" srcset="../images/ot-primer-rloc.png 1x, ../images/ot-primer-rloc_2x.png 2x" border="0" width="600" alt="OT RLOC" /></a>
</figure>

This same logic can be used to determine the RLOC for all highlighted nodes in the sample topology above:

<figure>
<a href="../images/ot-primer-rloc-topology-address_2x.png"><img src="../images/ot-primer-rloc-topology-address.png" srcset="../images/ot-primer-rloc-topology-address.png 1x, ../images/ot-primer-rloc-topology-address_2x.png 2x" border="0" width="600" alt="OT Topology w/ Address" /></a>
</figure>

However, because the RLOC is based on the location of the node in the topology,
the RLOC of a node can change as the topology changes.

For example, perhaps node `0x400` is removed from the Thread network. Nodes
`0x401` and `0x402` establish new links to different Routers, and as a result
they are each assigned a new RLOC16 and RLOC:

<figure>
<a href="../images/ot-primer-rloc-topology-change_2x.png"><img src="../images/ot-primer-rloc-topology-change.png" srcset="../images/ot-primer-rloc-topology-change.png 1x, ../images/ot-primer-rloc-topology-change_2x.png 2x" border="0" width="600" alt="OT Topology after Change" /></a>
</figure>

## Unicast address types
The RLOC is just one of many IPv6 unicast addresses a Thread device can have.
Another category of addresses are called Endpoint Identifiers (EIDs), which
identify a unique Thread interface within a Thread network partition. EIDs are
independent of Thread network topology.

Common unicast types are detailed below.

<table>
  <tbody>
    <tr>
      <th colspan=2><h3>Link-Local Address (LLA)</h3></th>
    </tr>
    <tr>
      <td colspan=2 style="background-color:rgb(238, 241, 242)">An EID that identifies a Thread interface reachable by a single radio transmission.</td>
    </tr>
    <tr>
      <td width="25%" style="background-color:rgb(238, 241, 242)"><b>Example</b></td><td><code>fe80::54db:881c:3845:57f4</code></td>
    </tr>
    <tr>
      <td width="25%" style="background-color:rgb(238, 241, 242)"><b>IID</b></td><td>Based on 802.15.4 Extended Address</td>
    </tr>
    <tr>
      <td width="25%" style="background-color:rgb(238, 241, 242)"><b>Scope</b></td><td>Link-Local</td>
    </tr>
    <tr>
      <td width="25%" style="background-color:rgb(238, 241, 242)"><b>Details</b></td><td><ul><li>Used to discover neighbors, configure links, and exchange routing information</li><li>Not a routable address</li><li>Always has a prefix of <code>fe80::/16</code></li></ul></td>
    </tr>
  </tbody>
</table>

<table>
  <tbody>
    <tr>
      <th colspan=2><h3>Mesh-Local EID (ML-EID)</h3></th>
    </tr>
    <tr>
      <td colspan=2 style="background-color:rgb(238, 241, 242)">An EID that identifies a Thread interface, independent of network topology. Used to reach a Thread interface within the same Thread partition. Also called a Unique Local Address (ULA).</td>
    </tr>
    <tr>
      <td width="25%" style="background-color:rgb(238, 241, 242)"><b>Example</b></td><td><code>fde5:8dba:82e1:1:416:993c:8399:35ab</code></td>
    </tr>
    <tr>
      <td width="25%" style="background-color:rgb(238, 241, 242)"><b>IID</b></td><td>Random, chosen after commissioning is complete</td>
    </tr>
    <tr>
      <td width="25%" style="background-color:rgb(238, 241, 242)"><b>Scope</b></td><td>Mesh-Local</td>
    </tr>
    <tr>
      <td width="25%" style="background-color:rgb(238, 241, 242)"><b>Details</b></td><td><ul><li>Does not change as the topology changes</li><li>Should be used by applications</li><li>Always has a prefix <code>fd00::/8</code></li></ul></td>
    </tr>
  </tbody>
</table>

<table>
  <tbody>
    <tr>
      <th colspan=2><h3>Routing Locator (RLOC)</h3></th>
    </tr>
    <tr>
      <td colspan=2 style="background-color:rgb(238, 241, 242)">Identifies a Thread interface, based on its location in the network topology.</td>
    </tr>
    <tr>
      <td width="25%" style="background-color:rgb(238, 241, 242)"><b>Example</b></td><td><code>fde5:8dba:82e1:1::ff:fe00:1001</code></td>
    </tr>
    <tr>
      <td width="25%" style="background-color:rgb(238, 241, 242)"><b>IID</b></td><td><code>0000:00ff:fe00:<var>RLOC16</var></code></td>
    </tr>
    <tr>
      <td width="25%" style="background-color:rgb(238, 241, 242)"><b>Scope</b></td><td>Mesh-Local</td>
    </tr>
    <tr>
      <td width="25%" style="background-color:rgb(238, 241, 242)"><b>Details</b></td><td><ul><li>Generated once a device attaches to a network</li><li>For delivering IPv6 datagrams within a Thread network</li><li>Changes as the topology changes</li><li>Generally not used by applications</li></ul></td>
    </tr>
  </tbody>
</table>

<table>
  <tbody>
    <tr>
      <th colspan=2><h3>Anycast Locator (ALOC)</h3></th>
    </tr>
    <tr>
      <td colspan=2 style="background-color:rgb(238, 241, 242)">Identifies a Thread interface via RLOC lookup, when the RLOC of a destination is not known.</td>
    </tr>
    <tr>
      <td width="25%" style="background-color:rgb(238, 241, 242)"><b>Example</b></td><td><code>fde5:8dba:82e1:1::ff:fe00:fc01</code></td>
    </tr>
    <tr>
      <td width="25%" style="background-color:rgb(238, 241, 242)"><b>IID</b></td><td><code>0000:00ff:fe00:fc<var>XX</var></code></td>
    </tr>
    <tr>
      <td width="25%" style="background-color:rgb(238, 241, 242)"><b>Scope</b></td><td>Mesh-Local</td>
    </tr>
    <tr>
      <td width="25%" style="background-color:rgb(238, 241, 242)"><b>Details</b></td><td><ul><li><code>fc<var>XX</var></code> = <a href="#anycast">ALOC destination</a>, which looks up the appropriate RLOC</li><li>Generally not used by applications</li></td>
    </tr>
  </tbody>
</table>

<table>
  <tbody>
    <tr>
      <th colspan=2><h3>Global Unicast Address (GUA)</h3></th>
    </tr>
    <tr>
      <td colspan=2 style="background-color:rgb(238, 241, 242)">An EID that identifies a Thread interface on a global scope, beyond a Thread network.</td>
    </tr>
    <tr>
      <td width="25%" style="background-color:rgb(238, 241, 242)"><b>Example</b></td><td><code>2000::54db:881c:3845:57f4</code></td>
    </tr>
    <tr>
      <td width="25%" style="background-color:rgb(238, 241, 242)"><b>IID</b></td><td><ul><li>SLAAC — Randomly assigned by the device itself</li><li>DHCP — Assigned by a DHCPv6 server</li><li>Manual — Assigned by the application layer</li></ul></td>
    </tr>
    <tr>
      <td width="25%" style="background-color:rgb(238, 241, 242)"><b>Scope</b></td><td>Global</td>
    </tr>
    <tr>
      <td width="25%" style="background-color:rgb(238, 241, 242)"><b>Details</b></td><td><ul><li>A public IPv6 address</li><li>Always has a prefix of <code>2000::/3</code></li></td>
    </tr>
  </tbody>
</table>

## Multicast

Multicast is used to communicate information to multiple devices at once. In a
Thread network, specific addresses are reserved for multicast use with different
groups of devices, depending on the scope.

| IPv6 Address | Scope      | Delivered to      |
| ------------ | ---------- | ----------------- |
| `ff02::1`    | Link-Local | All FTDs and MEDs |
| `ff02::2`    | Link-Local | All FTDs          |
| `ff03::1`    | Mesh-Local | All FTDs and MEDs |
| `ff03::2`    | Mesh-Local | All FTDs          |

Key Point: A major difference between FTDs and MTDs are that FTDs subscribe to
the `ff03::2` multicast address. MTDs do not.

You might notice that Sleepy End Devices (SEDs) are not included as a
recipient in the multicast table above. Instead, Thread defines
link-local and realm-local scope unicast prefix-based IPv6 multicast
address used for All Thread Nodes, including SEDs. These multicast
addresses vary by Thread network, because it is built on the unicast
Mesh-Local prefix (see [RFC 3306](https://tools.ietf.org/html/rfc3306)
for more details on unicast-prefix-based IPv6 multicast addresses).

Arbitrary scopes beyond those already listed are also supported for Thread
devices.


## Anycast

Anycast is used to route traffic to a Thread interface when the RLOC of a
destination is not known. An Anycast Locator (ALOC) identifies the location of
multiple interfaces within a Thread partition. The last 16 bits of an ALOC,
called the ALOC16, is in the format of <code>0xfc<var>XX</var></code>, which
represents the type of ALOC.

For example, an ALOC16 between `0xfc01` and `0xfc0f` is reserved for DHCPv6
Agents. If the specific DHCPv6 Agent RLOC is unknown (perhaps because the
network topology has changed), a message can be sent to a DHCPv6 Agent ALOC to
obtain the RLOC.

Thread defines the following ALOC16 values:

| ALOC16                                     | Type                     |
| ------------------------------------------ | ------------------------ |
| `0xfc00`                                   | Leader                   |
| `0xfc01` – `0xfc0f`                        | DHCPv6 Agent             |
| `0xfc10` – `0xfc2f`                        | Service                  |
| `0xfc30` – `0xfc37`                        | Commissioner             |
| `0xfc40` – `0xfc4e`                        | Neighbor Discovery Agent |
| `0xfc38` – `0xfc3f`<br>`0xfc4f` – `0xfcff` | Reserved                 |

## Recap

What you've learned:

*   A Thread network consists of three scopes: Link-Local, Mesh-Local, and Global
*   A Thread device has multiple unicast IPv6 addresses
    *   An RLOC represents a device's location in the Thread network
    *   An ML-EID is unique to a Thread device within a partition and should be used by applications
*   Thread uses multicast to forward data to groups of nodes and routers
*   Thread uses anycast when the RLOC of a destination is unknown

To learn more about Thread's IPv6 addressing, see sections 5.2 and 5.3 of the
[Thread Specification](http://threadgroup.org/ThreadSpec).
