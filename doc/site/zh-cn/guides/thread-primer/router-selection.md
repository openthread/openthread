# Router 选择

## Connected Dominating Set

<figure class="attempt-right">
<a href="../images/ot-primer-cds_2x.png"><img src="../images/ot-primer-cds.png" srcset="../images/ot-primer-cds.png 1x, ../images/ot-primer-cds_2x.png 2x" width="350" border="0" alt="OT Connected Dominating Set" /></a><figcaption style="text-align: center"><i>Example of a Connected Dominating Set</i></figcaption>
</figure>

Router 必须形成一个 CDS（Connected Dominating Set，连接支配集），这意味着：

1. 在任何两个 Router 之间都有一个 Router-only 的路径。
2. Thread 网络中的任何一个 Router 都可以通过完全位于 Router 集中而到达其他任何 Router。
3. Thread 网络中的每个 End Device 都直接连接到 Router。

Thread 使用分布式算法维护 CDS，从而确保最低程度的冗余。每个设备最初都作为 End Device（子）连接到网络。随着 Thread 网络状态的更改，算法会增添或移除 Router 以维护 CDS。

Thread 在下列情况下将会增添 Router：

* 如果网络低于 Router 阈值（16） —— 为了增加覆盖范围
* 增加路径多样性
* 保持最低程度的冗余
* 扩展连接并支持更多子节点

Thread 在下列情况下将会移除 Router：

* 将路由状态减少到最多 32 个 Router 以下
* 必要时允许在网络的其他部分使用新 Router

## 升级成 Router

子设备连接到 Thread 网络后，可以选择成为 Router。在开始 MLE Link Request 过程之前，子设备会向 Leader 发送 Address Solicit 消息，以请求一个 Router ID。如果 Leader 同意该请求，则它将响应一个 Router ID 给子设备，并且子设备会将自身升级为 Router。

然后，MLE Link Request 过程用于与相邻的 Router 建立双向 Router-Router 链路。

1. 新 Router 将发送一个多播 [Link Request](#1-Link-Request) 到相邻的 Router。
2. Router 使用 [Link Accept and Request](#2-Link-Accept-and-Request) 消息进行响应。
3. 新 Router 使用单播的 [Link Accept](#3-Link-Accept) 响应每个 Router，以建立 Router-Router 链路。

### 1. Link Request

Link Request 是从 Router 到 Thread 网络中所有其他 Router 的请求。首次成为 Router 时，设备会发送一个多播 Link Request 到 `ff02::2`。稍后，在通过 MLE Advertisement 发现其他 Router 后，设备将发送单播的 Link Request。

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
      <td>Thread 协议版本</td>
    </tr>
    <tr>
      <td width="25%" style="background-color:rgb(238, 241, 242)"><b>Challenge</b></td>
      <td>测试 Link Response 的及时性，以防止重放攻击</td>
    </tr>
    <tr>
      <td width="25%" style="background-color:rgb(238, 241, 242)"><b>Source
        Address</b></td>
      <td>发送者的 RLOC16</td>
    </tr>
    <tr>
      <td width="25%" style="background-color:rgb(238, 241, 242)"><b>Leader
        Data</b></td>
      <td>Router 的 Leader 的相关信息（RLOC, Partition ID, Partition weight）</td>
    </tr>
  </tbody>
</table>

### 2. Link Accept and Request

Link Accept and Request 是 Link Accept 和 Link Request 消息的组合。Thread 在 MLE Link Request 过程中使用此优化将消息的数量从四减少到三。

<figure>
<a href="../images/ot-primer-network-mle-link-request-02_2x.png"><img src="../images/ot-primer-network-mle-link-request-02.png" srcset="../images/ot-primer-network-mle-link-request-02.png 1x, ../images/ot-primer-network-mle-link-request-02_2x.png 2x" width="350" border="0" alt="OT MLE Link Accept and Request" /></a>
</figure>

### 3. Link Accept

Link Accept 是对来自相邻 Router 的 Link Request 的单播响应，该响应提供有关自身的信息并接受到相邻 Router 的链路。

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
      <td>Thread 协议版本</td>
    </tr>
    <tr>
      <td width="25%" style="background-color:rgb(238, 241, 242)"><b>Response</b></td>
      <td>测试 Link Response 的及时性，以防止重放攻击</td>
    </tr>
    <tr>
      <td width="25%" style="background-color:rgb(238, 241, 242)"><b>Link Frame
        Counter</b></td>
      <td>发送者上的 802.15.4 帧计数器</td>
    </tr>
    <tr>
      <td width="25%" style="background-color:rgb(238, 241, 242)"><b>MLE Frame
        Counter</b></td>
      <td>发送者上的 MLE 帧计数器</td>
    </tr>
    <tr>
      <td width="25%" style="background-color:rgb(238, 241, 242)"><b>Source
        Address</b></td>
      <td>发送者的 RLOC16</td>
    </tr>
    <tr>
      <td width="25%" style="background-color:rgb(238, 241, 242)"><b>Leader
        Data</b></td>
      <td>Router 的 Leader 的相关信息（RLOC, Partition ID, Partition weight）</td>
    </tr>
  </tbody>
</table>

## 降级成 REED

当 Router 降级成 REED 时，其 Router-Router 链路断开，并且设备开始 MLE Attach 过程以建立父子链路。

有关 MLE Attach 过程的更多信息，请参阅 [加入现有网络](/guides/thread-primer/network-discovery#加入现有网络)。

## 单向接收链路

在某些情况下，建立单向接收链路是有必要的。

在 Router 重置后，相邻 Router 可能仍具有与重置的 Router 的有效接收链路。在这种情况下，重置的 Router 发送 Link Request 消息以重新建立 Router-Router 链路。

End Device 也可能希望与相邻的 Router（非父节点）建立接收链路，以提高多播可靠性。当我们进入多播路由时，我们将学习更多与此相关的内容。

## 回顾

你应该学到了：

* Thread 网络中的 Router 必须形成 CDS
* Thread 设备将升级成 Router 或降级成 REED 以维护 CDS
* MLE Link Request 过程用于建立 Router-Router 链路
