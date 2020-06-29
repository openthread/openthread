# Node Roles and Types

## Forwarding roles

<figure class="attempt-right">
<a href="../images/ot-primer-roles_2x.png"><img src="../images/ot-primer-roles.png" srcset="../images/ot-primer-roles.png 1x, ../images/ot-primer-roles_2x.png 2x" border="0" alt="OT Node Roles" /></a>
</figure>

In a Thread network, nodes are split into two forwarding roles:

### Router

A Router is a node that:

*   forwards packets for network devices
*   provides secure commissioning services for devices trying to join the network
*   keeps its transceiver enabled at all times

### End Device

An End Device (ED) is a node that:

*   communicates primarily with a single Router
*   does not forward packets for other network devices
*   can disable its transceiver to reduce power

Key Point: The relationship between Router and End Device is a Parent-Child
relationship. An End Device attaches to exactly one Router. The Router is always
the Parent, the End Device the Child.

## Device types

Furthermore, nodes comprise a number of types.

<figure class="attempt-right">
<a href="../images/ot-primer-taxonomy_2x.png"><img src="../images/ot-primer-taxonomy.png" srcset="../images/ot-primer-taxonomy.png 1x, ../images/ot-primer-taxonomy.png 2x" border="0" alt="OT Device Taxonomy" /></a>
</figure>

### Full Thread Device

A Full Thread Device (FTD) always has its radio on, subscribes to the
all-routers multicast address, and maintains IPv6 address mappings. There are
three types of FTDs:

*   Router
*   Router Eligible End Device (REED) — can be promoted to a Router
*   Full End Device (FED) — cannot be promoted to a Router

An FTD can operate as a Router (Parent) or an End Device (Child).

### Minimal Thread Device

A Minimal Thread Device does not subscribe to the all-routers
multicast address and forwards all messages to its Parent. There are
two types of MTDs:

*   Minimal End Device (MED) — transceiver always on, does not need to poll for
    messages from its parent
*   Sleepy End Device (SED) — normally disabled, wakes on occasion to poll for
    messages from its parent

An MTD can only operate as an End Device (Child).

### Upgrading and downgrading

When a REED is the only node in reach of a new End Device wishing to join the
Thread network, it can upgrade itself and operate as a Router:

<figure>
<a href="../images/ot-primer-router-upgrade_2x.png"><img src="../images/ot-primer-router-upgrade.png" srcset="../images/ot-primer-router-upgrade.png 1x, ../images/ot-primer-router-upgrade_2x.png 2x" border="0" width="400" alt="OT End Device to Router" /></a>
</figure>

Conversely, when a Router has no children, it can downgrade itself and operate
as an End Device:

<figure>
<a href="../images/ot-primer-router-downgrade_2x.png"><img src="../images/ot-primer-router-downgrade.png" srcset="../images/ot-primer-router-downgrade.png 1x, ../images/ot-primer-router-downgrade_2x.png 2x" border="0" width="400" alt="OT Router to End Device" /></a>
</figure>

## Other roles and types

### Thread Leader

<figure class="attempt-right">
<a href="../images/ot-primer-leader_2x.png"><img src="../images/ot-primer-leader.png" srcset="../images/ot-primer-leader.png 1x, ../images/ot-primer-leader_2x.png 2x" border="0" alt="OT Leader and Border Router" /></a>
</figure>

The Thread Leader is a Router that is responsible for managing the set of
Routers in a Thread network. It is dynamically self-elected for fault tolerance,
and aggregates and distributes network-wide configuration information.

Note: There is always a single Leader in each Thread network
[partition](#partitions).

### Border Router

A Border Router is a device that can forward information between a Thread
network and a non-Thread network (for example, Wi-Fi). It also configures a
Thread network for external connectivity.

Any device may serve as a Border Router.

Note: There can be multiple Border Routers in a Thread network.

## Partitions

<figure class="attempt-right">
<a href="../images/ot-primer-partitions_2x.png"><img src="../images/ot-primer-partitions.png" srcset="../images/ot-primer-partitions.png 1x, ../images/ot-primer-partitions_2x.png 2x" border="0" alt="OT Partitions" /></a>
</figure>

A Thread network might be composed of partitions. This occurs when a group of
Thread devices can no longer communicate with another group of Thread devices.
Each partition logically operates as a distinct Thread network with its own
Leader, Router ID assignments, and network data, while retaining the same
security credentials for all devices across all partitions.

Partitions in a Thread network do not have wireless connectivity between each
other, and if partitions regain connectivity, they automatically merge into a
single partition.

Key Point: Security credentials define the Thread network. Physical radio
connectivity defines partitions within that Thread network.

Note that the use of "Thread network" in this primer assumes a single partition.
Where necessary, key concepts and examples are clarified with the term "partition."
Partitions are covered in-depth later in this primer.

## Device limits

There are limits to the number of device types a single Thread network supports.

Role | Limit
----|----
Leader | 1
Router | 32
End Device | 511 per Router

Thread tries to keep the number of Routers between 16 and 23. If a REED attaches
as an End Device and the number of Routers in the network is below 16, it
automatically promotes itself to a Router.

## Recap

What you learned:

*   A Thread device is either a Router (Parent) or an End Device (Child)
*   A Thread device is either a Full Thread Device (maintains IPv6 address
    mappings) or a Minimal Thread Device (forwards all messages to its Parent)
*   A Router Eligible End Device can promote itself to a Router, and vice versa
*   Every Thread network partition has a Leader to manage Routers
*   A Border Router is used to connect Thread and non-Thread networks
*   A Thread network might be composed of multiple partitions
