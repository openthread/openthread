# Network Discovery and Formation

## Thread networks

Thread networks are identified by three unique identifiers:

*   2-byte Personal Area Network ID (PAN ID)
*   8-byte Extended Personal Area Network ID (XPAN ID)
*   A human-readable Network Name

For example, a Thread network may have the following identifiers:

Identifier | Value
---- | ----
PAN ID | `0xBEEF`
XPAN ID | `0xBEEF1111CAFE2222`
Network Name | `yourThreadCafe`

<figure class="attempt-right">
<a href="../images/ot-primer-network-active-scan_2x.png"><img src="../images/ot-primer-network-active-scan.png" srcset="../images/ot-primer-network-active-scan.png 1x, ../images/ot-primer-network-active-scan_2x.png 2x" border="0" alt="OT Active Scan" /></a>
</figure>

When creating a new Thread network, or searching for an existing one to join, a
Thread device performs an active scan for 802.15.4 networks within radio range:

1.  The device broadcasts an 802.15.4 Beacon Request on a specific Channel.
1.  In return, any Routers or Router Eligible End Devices (REEDs) in range
    broadcast a Beacon that contains their Thread network PAN ID, XPAN ID, and
    Network Name.
1.  The device repeats the previous two steps for each Channel.

Once a Thread device has discovered all networks in range, it can either attach
to an existing network, or create a new one if no networks are discovered.

<h2 style="clear:right">Mesh Link Establishment</h2>

Thread uses the Mesh Link Establishment (MLE) protocol to configure links and
disseminate information about the network to Thread devices.

In link configuration, MLE is used to:

*   Discover links to neighboring devices
*   Determine the quality of links to neighboring devices
*   Establish links to neighboring devices
*   Negotiate link parameters (device type, frame counters, timeout) with peers

MLE disseminates the following types of information to devices wishing to
establish links:

*   Leader data (Leader RLOC, Partition ID, Partition weight)
*   Network data (on-mesh prefixes, address autoconfiguration, more-specific
    routes)
*   Route propagation

Route propagation in Thread works similar to the Routing Information Protocol
(RIP), a distance-vector routing protocol.

Note: MLE only proceeds once a Thread device has obtained Thread network
credentials through Thread Commissioning. Commissioning and Security will be
covered in depth later in this Primer. For now, this page assumes that the
device has already been commissioned.

## Create a new network

If the device elects to create a new network, it selects the least busy Channel
and a PAN ID not in use by other networks, then becomes a Router and elects
itself the Leader. This device sends MLE Advertisement messages to other
802.15.4 devices to inform them of its link state, and responds to Beacon
Requests by other Thread devices performing an active scan.

## Join an existing network

If the device elects to join an existing network, it configures its Channel, PAN
ID, XPAN ID, and Network Name to match that of the target network via Thread
Commissioning, then goes through the MLE Attach process to attach as a Child
(End Device). This process is used for Child-Parent links.

Key Point: Every device, router-capable or not, initially attaches to a Thread
network as a Child (End Device).

1.  The Child sends a multicast [Parent Request](#1_parent_request) to all
    neighboring Routers and REEDs in the target network.
1.  All neighboring Routers and REEDs (if the Parent Request Scan Mask includes
    REEDs) send [Parent Responses](#2_parent_response) with information about
    themselves.
1.  The Child chooses a Parent device and sends a [Child ID
    Request](#3_child_id_request) to it.
1.  The Parent sends a [Child ID Response](#4_child_id_response) to confirm link
    establishment.

### 1. Parent Request

A Parent Request is a multicast request from the attaching device that is used
to discover neighboring Routers and Router Eligible End Devices (REEDs) in the
target network.

<figure>
<a href="../images/ot-primer-network-mle-attach-01_2x.png"><img src="../images/ot-primer-network-mle-attach-01.png" srcset="../images/ot-primer-network-mle-attach-01.png 1x, ../images/ot-primer-network-mle-attach-01_2x.png 2x" width="350" border="0" alt="OT MLE Attach Parent Request" /></a>
</figure>

<table>
  <tbody>
    <tr>
      <th colspan=2>Parent Request Message Contents</th>
    </tr>
    <tr>
      <td width="25%" style="background-color:rgb(238, 241, 242)"><b>Mode</b></td>
      <td>Describes the attaching device</td>
    </tr>
    <tr>
      <td width="25%" style="background-color:rgb(238, 241, 242)"><b>Challenge</b></td>
      <td>Tests the timeliness of the Parent Response to prevent replay
        attacks</td>
    </tr>
    <tr>
      <td width="25%" style="background-color:rgb(238, 241, 242)"><b>Scan Mask</b></td>
      <td>Limits the request to only Routers or to both Routers and REEDs</td>
    </tr>
  </tbody>
</table>

### 2. Parent Response

A Parent Response is a unicast response to a Parent Request that provides
information about a Router or REED to the attaching device.

<figure>
<a href="../images/ot-primer-network-mle-attach-02_2x.png"><img src="../images/ot-primer-network-mle-attach-02.png" srcset="../images/ot-primer-network-mle-attach-02.png 1x, ../images/ot-primer-network-mle-attach-02_2x.png 2x" width="350" border="0" alt="OT MLE Attach Parent Response" /></a>
</figure>

<table>
  <tbody>
    <tr>
      <th colspan=2>Parent Response Message Contents</th>
    </tr>
    <tr>
      <td width="25%" style="background-color:rgb(238, 241, 242)"><b>Version</b></td>
      <td>Thread protocol version</td>
    </tr>
    <tr>
      <td width="25%" style="background-color:rgb(238, 241, 242)"><b>Response</b></td>
      <td>Copy of the Parent Request Challenge</td>
    </tr>
    <tr>
      <td width="25%" style="background-color:rgb(238, 241, 242)"><b>Link Frame
        Counter</b></td>
      <td>802.15.4 Frame Counter on the Router/REED</td>
    </tr>
    <tr>
      <td width="25%" style="background-color:rgb(238, 241, 242)"><b>MLE Frame
        Counter</b></td>
      <td>MLE Frame Counter on the Router/REED</td>
    </tr>
    <tr>
      <td width="25%" style="background-color:rgb(238, 241, 242)"><b>Source
        Address</b></td>
      <td>RLOC16 of the Router/REED</td>
    </tr>
    <tr>
      <td width="25%" style="background-color:rgb(238, 241, 242)"><b>Link
        Margin</b></td>
      <td>Receive signal quality of the Router/REED</td>
    </tr>
    <tr>
      <td width="25%" style="background-color:rgb(238, 241, 242)"><b>Connectivity</b></td>
      <td>Describes the Router/REED’s level of connectivity</td>
    </tr>
    <tr>
      <td width="25%" style="background-color:rgb(238, 241, 242)"><b>Leader
        Data</b></td>
      <td>Information about the Router/REED’s Leader</td>
    </tr>
    <tr>
      <td width="25%" style="background-color:rgb(238, 241, 242)"><b>Challenge</b></td>
      <td>Tests the timeliness of the Child ID Request to prevent replay
        attacks</td>
    </tr>
  </tbody>
</table>

### 3. Child ID Request

A Child ID Request is a unicast request from the attaching device (Child) that
is sent to the Router or REED (Parent) for the purpose of establishing a
Child-Parent link. If the request is sent to a REED, it [upgrades itself to a
Router](/guides/thread-primer/router-selection#upgrade_to_a_router) before
accepting the request.

<figure>
<a href="../images/ot-primer-network-mle-attach-03_2x.png"><img src="../images/ot-primer-network-mle-attach-03.png" srcset="../images/ot-primer-network-mle-attach-03.png 1x, ../images/ot-primer-network-mle-attach-03_2x.png 2x" width="350" border="0" alt="OT MLE Attach Child ID Request" /></a>
</figure>

<table>
  <tbody>
    <tr>
      <th colspan=2>Child ID Request Message Contents</th>
    </tr>
    <tr>
      <td width="25%" style="background-color:rgb(238, 241, 242)"><b>Version</b></td>
      <td>Thread protocol version</td>
    </tr>
    <tr>
      <td width="25%" style="background-color:rgb(238, 241, 242)"><b>Response</b></td>
      <td>Copy of the Parent Response Challenge</td>
    </tr>
    <tr>
      <td width="25%" style="background-color:rgb(238, 241, 242)"><b>Link Frame
        Counter</b></td>
      <td>802.15.4 Frame Counter on the Child</td>
    </tr>
    <tr>
      <td width="25%" style="background-color:rgb(238, 241, 242)"><b>MLE Frame
        Counter</b></td><td>MLE Frame Counter on the Child</td>
    </tr>
    <tr>
      <td width="25%" style="background-color:rgb(238, 241, 242)"><b>Mode</b></td>
      <td>Describes the Child</td>
    </tr>
    <tr>
      <td width="25%" style="background-color:rgb(238, 241, 242)"><b>Timeout</b></td>
      <td>Inactivity duration before the Parent removes the Child</td>
    </tr>
    <tr>
      <td width="25%" style="background-color:rgb(238, 241, 242)"><b>Address
        Registration (MEDs and SEDs only)</b></td>
      <td>Register IPv6 addresses</td>
    </tr>
  </tbody>
</table>

### 4. Child ID Response

A Child ID Response is a unicast response from the Parent that is sent to the
Child to confirm that a Child-Parent link has been established.

<figure>
<a href="../images/ot-primer-network-mle-attach-04_2x.png"><img src="../images/ot-primer-network-mle-attach-04.png" srcset="../images/ot-primer-network-mle-attach-04.png 1x, ../images/ot-primer-network-mle-attach-04_2x.png 2x" width="350" border="0" alt="OT MLE Attach Child ID Response" /></a>
</figure>

<table>
  <tbody>
    <tr>
      <th colspan=2>Child ID Response Message Contents</th>
    </tr>
    <tr>
      <td width="25%" style="background-color:rgb(238, 241, 242)"><b>Source
        Address</b></td>
      <td>Parent's RLOC16</td>
    </tr>
    <tr>
      <td width="25%" style="background-color:rgb(238, 241, 242)"><b>Address16</b></td>
      <td>Child's RLOC16</td>
    </tr>
    <tr>
      <td width="25%" style="background-color:rgb(238, 241, 242)"><b>Leader
        Data</b></td>
      <td>Information about the Parent’s Leader (RLOC, Partition ID, Partition
        weight)</td>
    </tr>
    <tr>
      <td width="25%" style="background-color:rgb(238, 241, 242)"><b>Network
        Data</b></td>
      <td>Information about the Thread network (on-mesh prefixes, address
        autoconfiguration, more-specific routes)</td>
    </tr>
    <tr>
      <td width="25%" style="background-color:rgb(238, 241, 242)"><b>Route
        (REED only)</b></td>
      <td>Route propagation</td>
    </tr>
    <tr>
      <td width="25%" style="background-color:rgb(238, 241, 242)"><b>Timeout</b></td>
      <td>Inactivity duration before the Parent removes the Child</td>
    </tr>
    <tr>
      <td width="25%" style="background-color:rgb(238, 241, 242)"><b>Address
        Registration (MEDs and SEDs only)</b></td>
      <td>Confirm registered addresses</td>
    </tr>
  </tbody>
</table>

## Recap

What you've learned:

*   A Thread device performs an active scan for existing networks
*   Thread uses Mesh Link Establishment to configure links and disseminate
    information about network devices
*   MLE Advertisement messages inform other Thread devices about a device's
    network and link state
*   The MLE Attach process establishes Child-Parent links
