# 网络发现与形成

## Thread 网络

Thread 网络由三个唯一的标识符标识：

* 2 字节的 PAN ID（Personal Area Network ID，个域网标识符）
* 8 字节的 XPAN ID（Extended Personal Area Network ID，扩展个域网标识符）
* 方便人类阅读的 Network Name（网络名称）

例如，一个 Thread 网络可能具有以下标识符：

| 标识符       | 值                   |
| ------------ | -------------------- |
| PAN ID       | `0xBEEF`             |
| XPAN ID      | `0xBEEF1111CAFE2222` |
| Network Name | `yourThreadCafe`     |

<figure class="attempt-right">
<a href="../images/ot-primer-network-active-scan_2x.png"><img src="../images/ot-primer-network-active-scan.png" srcset="../images/ot-primer-network-active-scan.png 1x, ../images/ot-primer-network-active-scan_2x.png 2x" border="0" alt="OT Active Scan" /></a>
</figure>

在创建新的 Thread 网络或搜索现有的网络时，Thread 设备会主动扫描射频范围内的 802.15.4 网络：

1. 设备在特定 Channel 上广播 802.15.4 信标请求（Beacon Request）。
2. 范围内的所有 Router 或 REED 都会广播包含其 Thread 网络 PAN ID、XPAN ID 和 Network Name 的信标（Beacon），以作为回应。
3. 设备为每个 Channel 重复前两个步骤。

Thread 设备发现范围内的所有网络后，可以选择连接到现有的网络，也可以在未发现任何网络的情况下创建新的网络。

<h2 style="clear:right">Mesh Link Establishment</h2>

Thread 使用 MLE（Mesh Link Establishment）协议来配置链路并将网络的相关信息传播到 Thread 设备。

在链路配置中，MLE 用于：

* 发现相邻设备的链路
* 确认到相邻设备的链路质量
* 建立到相邻设备的链路
* 与对端协商链路参数（设备类型、帧计数器、超时）

MLE 将以下类型的信息传播给希望建立链路的设备：

* Leader data（Leader RLOC, Partition ID（分区标识符）, Partition weight（分区权重））
* Network data（on-mesh prefixes, address autoconfiguration（地址自动配置）, more-specific routes（具体路由））
* Route propagation（路由传播）

Thread 中路由传播的工作原理类似于 RIP（Routing Information Protocol，路由信息协议），RIP 是一种距离矢量路由协议。

Note: 仅当 Thread 设备通过 Thread Commissioning 获得 Thread 网络凭据后，才会继续进行 MLE 过程。Commissioning 和安全性将在本教程的后续部分中深入介绍。目前，假定设备已通过 Commissioning。

## 创建新网络

如果设备选择创建新网络，它将选择最不繁忙的 Channel 和其他网络未使用的 PAN ID，然后成为 Router 并选举自己为 Leader。该设备将 MLE Advertisement 消息发送到其他 802.15.4 设备，以通知其链路状态，并响应其他执行主动扫描的 Thread 设备所发出的信标请求。

## 加入现有网络

如果设备选择加入到现有的网络，则会通过 Thread Commissioning 将其 Channel、PAN ID、XPAN ID 和 Network Name 配置为与目标网络相同，然后进行 MLE Attach 过程以作为子节点（End Device）进行加入。此过程用于“父子链路（Child-Parent link）”。

Key Point: 每个设备（无论是否具有充当 Router 的能力），最初都作为子设备（End Device）连接到 Thread 网络。

1. 子节点向目标网络中的所有相邻的 Router 和 REED 发送多播 [Parent Request](#1-Parent-Request)。
2. 所有相邻的 Router 和 REED（如果 Parent Request Scan Mask（父节点请求扫描掩码）包括了 REED）都应发送 [Parent Response](#2-Parent-Response) 以将其自身的信息告诉给子节点。
3. 子节点选择一个父节点，并向其发送 [Child ID Request](#3-Child-ID-Request)。
4. 父节点发送 [Child ID Response](#4-Child-ID-Response) 以确认链路建立。

### 1. Parent Request

Parent Request 是来自待连接设备的多播请求，用于发现目标网络中的相邻的 Router 和 REED。

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
      <td>描述待连接设备</td>
    </tr>
    <tr>
      <td width="25%" style="background-color:rgb(238, 241, 242)"><b>Challenge</b></td>
      <td>测试 Parent Response 的时效性，以防止重放攻击</td>
    </tr>
    <tr>
      <td width="25%" style="background-color:rgb(238, 241, 242)"><b>Scan Mask</b></td>
      <td>将请求限制为仅 Router 或 Router 和 REED</td>
    </tr>
  </tbody>
</table>

### 2. Parent Response

Parent Response 是对 Parent Request 的单播响应，它向待连接设备提供有关 Router 或 REED 的信息。

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
      <td>Thread 协议版本</td>
    </tr>
    <tr>
      <td width="25%" style="background-color:rgb(238, 241, 242)"><b>Response</b></td>
      <td>Parent Request Challenge 的副本</td>
    </tr>
    <tr>
      <td width="25%" style="background-color:rgb(238, 241, 242)"><b>Link Frame
        Counter</b></td>
      <td>Router/REED 上的 802.15.4 帧计数器</td>
    </tr>
    <tr>
      <td width="25%" style="background-color:rgb(238, 241, 242)"><b>MLE Frame
        Counter</b></td>
      <td>Router/REED 上的 MLE 帧计数器</td>
    </tr>
    <tr>
      <td width="25%" style="background-color:rgb(238, 241, 242)"><b>Source
        Address</b></td>
      <td>Router/REED 的 RLOC16</td>
    </tr>
    <tr>
      <td width="25%" style="background-color:rgb(238, 241, 242)"><b>Link
        Margin</b></td>
      <td>Router/REED 的接收信号质量</td>
    </tr>
    <tr>
      <td width="25%" style="background-color:rgb(238, 241, 242)"><b>Connectivity</b></td>
      <td>描述 Router/REED 的连通性</td>
    </tr>
    <tr>
      <td width="25%" style="background-color:rgb(238, 241, 242)"><b>Leader
        Data</b></td>
      <td>有关 Router/REED 的 Leader 的信息</td>
    </tr>
    <tr>
      <td width="25%" style="background-color:rgb(238, 241, 242)"><b>Challenge</b></td>
      <td>测试 Child ID Request 的时效性，以防止重放攻击</td>
    </tr>
  </tbody>
</table>

### 3. Child ID Request

Child ID Request 是来自待连接设备（子）的单播请求，该单播请求被发送到 Router（父）或 REED（父），目的是建立父子链路。如果将请求发送到 REED，则 REED 会在接受请求之前将自身升级为 Router。

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
      <td>Thread 协议版本</td>
    </tr>
    <tr>
      <td width="25%" style="background-color:rgb(238, 241, 242)"><b>Response</b></td>
      <td>Parent Response Challenge 的副本</td>
    </tr>
    <tr>
      <td width="25%" style="background-color:rgb(238, 241, 242)"><b>Link Frame
        Counter</b></td>
      <td>Child 上的 802.15.4 帧计数器</td>
    </tr>
    <tr>
      <td width="25%" style="background-color:rgb(238, 241, 242)"><b>MLE Frame
        Counter</b></td><td>Child 上的 MLE 帧计数器</td>
    </tr>
    <tr>
      <td width="25%" style="background-color:rgb(238, 241, 242)"><b>Mode</b></td>
      <td>描述子节点</td>
    </tr>
    <tr>
      <td width="25%" style="background-color:rgb(238, 241, 242)"><b>Timeout</b></td>
      <td>父节点移除子节点之前的闲置时间</td>
    </tr>
    <tr>
      <td width="25%" style="background-color:rgb(238, 241, 242)"><b>Address
        Registration (MEDs and SEDs only)</b></td>
      <td>注册 IPv6 地址</td>
    </tr>
  </tbody>
</table>

### 4. Child ID Response

Child ID Response 是父节点对 Child ID Request 的单播响应，该响应发送给对应的子节点以确认父子链路的建立。

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
      <td>父节点的 RLOC16</td>
    </tr>
    <tr>
      <td width="25%" style="background-color:rgb(238, 241, 242)"><b>Address16</b></td>
      <td>子节点的 RLOC16</td>
    </tr>
    <tr>
      <td width="25%" style="background-color:rgb(238, 241, 242)"><b>Leader
        Data</b></td>
      <td>父节点的 Leader 的相关信息（RLOC, Partition ID, Partition weight）</td>
    </tr>
    <tr>
      <td width="25%" style="background-color:rgb(238, 241, 242)"><b>Network
        Data</b></td>
      <td>Thread 网络的相关信息（on-mesh prefixes, address autoconfiguration, more-specific routes）</td>
    </tr>
    <tr>
      <td width="25%" style="background-color:rgb(238, 241, 242)"><b>Route
        (REED only)</b></td>
      <td>路由传播</td>
    </tr>
    <tr>
      <td width="25%" style="background-color:rgb(238, 241, 242)"><b>Timeout</b></td>
      <td>父节点移除子节点之前的闲置时间</td>
    </tr>
    <tr>
      <td width="25%" style="background-color:rgb(238, 241, 242)"><b>Address
        Registration (MEDs and SEDs only)</b></td>
      <td>确认已注册地址</td>
    </tr>
  </tbody>
</table>

## 回顾

你应该学到了：

* Thread 设备通过主动扫描发现现有网络
* Thread 使用 MLE 来配置链路并分发有关网络设备的信息
* MLE Advertisement 消息通知其他 Thread 设备有关设备的网络和链路状态
* MLE Attach 过程建立了父子链路
