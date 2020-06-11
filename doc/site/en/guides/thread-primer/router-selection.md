# Router Selection

## Connected Dominating Set

<figure class="attempt-right">
<a href="../images/ot-primer-cds_2x.png"><img src="../images/ot-primer-cds.png" srcset="../images/ot-primer-cds.png 1x, ../images/ot-primer-cds_2x.png 2x" width="350" border="0" alt="OT Connected Dominating Set" /></a><figcaption style="text-align: center"><i>Example of a Connected Dominating Set</i></figcaption>
</figure>

Routers must form a Connected Dominating Set (CDS), which means:

1.  There is a Router-only path between any two Routers.
1.  Any one Router in a Thread network can reach any other Router by staying
    entirely within the set of Routers.
1.  Every End Device in a Thread network is directly connected to a Router.

A distributed algorithm maintains the CDS, which ensures a minimum level of
redundancy. Every device initially attaches to the network as an End Device
(Child). As the state of the Thread network changes, the algorithm adds or
removes Routers to maintain the CDS.

Thread adds Routers to:

*   Increase coverage if the network is below the Router threshold of 16
*   Increase path diversity
*   Maintain a minimum level of redundancy
*   Extend connectivity and support more Children

Thread removes Routers to:

*   Reduce the Routing state below the maximum of 32 Routers
*   Allow new Routers in other parts of the network when needed

## Upgrade to a Router

After attaching to a Thread network, the Child device may elect to become a
Router. Before initiating the MLE Link Request process, the Child sends an
Address Solicit message to the Leader, asking for a Router ID. If the Leader
accepts, it responds with a Router ID and the Child upgrades itself to a Router.

The MLE Link Request process is then used to establish bi-directional
Router-Router links with neighboring Routers.

1.  The new Router sends a multicast [Link Request](#1_link_request) to
    neighboring Routers.
1.  Routers respond with [Link Accept and Request](#2_link_accept_and_request)
    messages.
1.  The new Router responds to each Router with a unicast [Link
    Accept](#3_link_accept) to establish the Router-Router link.

### 1. Link Request

A Link Request is a request from the Router to all other Routers in the Thread
network. When first becoming a Router, the device sends a multicast Link Request
to `ff02::2`. Later, after discovering the other Routers via MLE Advertisements,
the devices send unicast Link Requests.

<figure>
<a href="../images/ot-primer-network-mle-link-request-01_2x.png"><img src="../images/ot-primer-network-mle-link-request-01.png" srcset="../images/ot-primer-network-mle-link-request-01.png 1x, ../images/ot-primer-network-mle-link-request-01_2x.png 2x" width="350" border="0" alt="OT MLE Link Request" /></a>
</figure>

<table>
  <tbody>
    <tr>
      <th colspan=2>Link Request Message Contents</th>
    </tr>
    <tr>
      <td width="25%" style="background-color:rgb(238, 241, 242)"><b>Version</b></td>
      <td>Thread protocol version</td>
    </tr>
    <tr>
      <td width="25%" style="background-color:rgb(238, 241, 242)"><b>Challenge</b></td>
      <td>Tests the timeliness of the Link Response to prevent replay
        attacks</td>
    </tr>
    <tr>
      <td width="25%" style="background-color:rgb(238, 241, 242)"><b>Source
        Address</b></td>
      <td>RLOC16 of the sender</td>
    </tr>
    <tr>
      <td width="25%" style="background-color:rgb(238, 241, 242)"><b>Leader
        Data</b></td>
      <td>Information about the Router's Leader, as stored on the sender (RLOC,
        Partition ID, Partition weight)</td>
    </tr>
  </tbody>
</table>

### 2. Link Accept and Request

A Link Accept and Request is a combination of the Link Accept and Link Request
messages. Thread uses this optimization in the MLE Link Request process to
reduce the number of messages from four to three.

<figure>
<a href="../images/ot-primer-network-mle-link-request-02_2x.png"><img src="../images/ot-primer-network-mle-link-request-02.png" srcset="../images/ot-primer-network-mle-link-request-02.png 1x, ../images/ot-primer-network-mle-link-request-02_2x.png 2x" width="350" border="0" alt="OT MLE Link Accept and Request" /></a>
</figure>

### 3. Link Accept

A Link Accept is a unicast response to a Link Request from a neighboring Router
that provides information about itself and accepts the link to the neighboring
Router.

<figure>
<a href="../images/ot-primer-network-mle-link-request-03_2x.png"><img src="../images/ot-primer-network-mle-link-request-03.png" srcset="../images/ot-primer-network-mle-link-request-03.png 1x, ../images/ot-primer-network-mle-link-request-03_2x.png 2x" width="350" border="0" alt="OT MLE Link Accept" /></a>
</figure>

<table>
  <tbody>
    <tr>
      <th colspan=2>Link Accept Message Contents</th>
    </tr>
    <tr>
      <td width="25%" style="background-color:rgb(238, 241, 242)"><b>Version</b></td>
      <td>Thread protocol version</td>
    </tr>
    <tr>
      <td width="25%" style="background-color:rgb(238, 241, 242)"><b>Response</b></td>
      <td>Tests the timeliness of the Link Response to prevent replay
        attacks</td>
    </tr>
    <tr>
      <td width="25%" style="background-color:rgb(238, 241, 242)"><b>Link Frame
        Counter</b></td>
      <td>802.15.4 Frame Counter on the sender</td>
    </tr>
    <tr>
      <td width="25%" style="background-color:rgb(238, 241, 242)"><b>MLE Frame
        Counter</b></td>
      <td>MLE Frame Counter on the sender</td>
    </tr>
    <tr>
      <td width="25%" style="background-color:rgb(238, 241, 242)"><b>Source
        Address</b></td>
      <td>RLOC16 of the sender</td>
    </tr>
    <tr>
      <td width="25%" style="background-color:rgb(238, 241, 242)"><b>Leader
        Data</b></td>
      <td>Information about the Router's Leader, as stored on the sender (RLOC,
        Partition ID, Partition weight)</td>
    </tr>
  </tbody>
</table>

## Downgrade to a REED

When a Router downgrades to a REED, its Router-Router links are disconnected,
and the device initiates the MLE Attach process to establish a Child-Parent
link.

See [Join an existing
network](/guides/thread-primer/network-discovery#join_an_existing_network)
for more information on the MLE Attach process.

## One-way receive links

In some scenarios, it may be necessary to establish a one-way receive link.

After a Router reset, neighboring Routers may still have a valid receive link
with the reset Router. In this case, the reset Router sends a Link Request
message to re-establish the Router-Router link.

An End Device may also wish to establish a receive link with neighboring
non-Parent Routers to improve multicast reliability. We'll learn more about this
when we get to Multicast Routing.

## Recap

What you've learned:

*   Routers in a Thread network must form a Connected Dominating Set (CDS)
*   Thread devices are upgraded to Routers or downgraded to End Devices to
    maintain the CDS
*   The MLE Link Request process is used to establish Router-Router links
