# IPv6 寻址

让我们来看一下 Thread 如何识别网络中的每个设备，以及设备间用何种类型的地址进行相互通信。

Key Term: 在本入门教程中，术语“接口（interface）”用于标识网络内 Thread 设备的端点。通常，单个 Thread 设备具有单个 Thread 接口。

## 域

<figure class="attempt-right">
<a href="../images/ot-primer-scopes_2x.png"><img src="../images/ot-primer-scopes.png" srcset="../images/ot-primer-scopes.png 1x, ../images/ot-primer-scopes_2x.png 2x" border="0" alt="OT Scopes" /></a>
</figure>

Thread 网络中有三种域用于单播寻址：

* Link-Local — 所有通过单次射频传输可访问的接口
* Mesh-Local — 所有在同一 Thread 网络中可访问的接口
* Global — 所有从 Thread 网络外部可以访问的接口

前两个域与 Thread 网络指定的 Prefix（前缀）相对应。Link-Local 的 Prefix 为 `fe80::/16`，Mesh-Local 的 Prefix 为 `fd00::/8`。

<h2 style="clear:right">单播</h2>

单个 Thread 设备可以通过多种 IPv6 单播地址来进行标识。每种地址都有不同的功能（基于域和用例）。

在介绍每种类型之前，让我们先了解一个共同的概念，它叫作 RLOC（Routing Locator）。RLOC 根据 Thread 接口在网络拓扑中的位置来对其进行标识。

### 如何生成 RLOC

所有设备都获得一个 Router ID 和一个 Child ID。每个 Router 维护一个包含其所有子节点的表，两个 ID 的组合唯一地标识拓扑中的设备。例如，请参考以下拓扑中高亮的节点，其中 Router（五边形）中的数字是 Router ID，End Device（圆形）中的数字是 Child ID：

<figure>
<a href="../images/ot-primer-rloc-topology_2x.png"><img src="../images/ot-primer-rloc-topology.png" srcset="../images/ot-primer-rloc-topology.png 1x, ../images/ot-primer-rloc-topology_2x.png 2x" border="0" width="600" alt="OT RLOC Topology" /></a>
</figure>

每个子节点的 Router ID 对应于它的父节点（Router）。因为 Router 不会是子节点，所以 Router 的 Child ID 始终为 0。这些值对于 Thread 网络中的每个设备都是唯一的，并用于创建 RLOC16（代表 RLOC 的后 16 位）。

例如，以下是左上节点（Router ID = 1，Child ID = 1）的 RLOC16 的计算方法：

<figure>
<a href="../images/ot-primer-rloc16_2x.png"><img src="../images/ot-primer-rloc16.png" srcset="../images/ot-primer-rloc16.png 1x, ../images/ot-primer-rloc16_2x.png 2x" border="0" width="400" alt="OT RLOC16" /></a>
</figure>

RLOC16 是 IID（Interface Identifier）的一部分，IID 对应的是 IPv6 地址的后 64 位。一些 IID 可用于标识某些类型的 Thread 接口。例如，RLOC 的 IID 始终为 <code>0000:00ff:fe00:<var>RLOC16</var></code> 的形式。

RLOC 由 Mesh-Local Prefix 和 IID 组成。例如，如果 Mesh-Local Prefix 是 `fde5:8dba:82e1:1::/64`，RLOC16 = `0x401`，那么该节点的 RLOC 就是：

<figure>
<a href="../images/ot-primer-rloc_2x.png"><img src="../images/ot-primer-rloc.png" srcset="../images/ot-primer-rloc.png 1x, ../images/ot-primer-rloc_2x.png 2x" border="0" width="600" alt="OT RLOC" /></a>
</figure>

可以使用相同的逻辑来确定以上示例拓扑中所有高亮的节点的 RLOC：

<figure>
<a href="../images/ot-primer-rloc-topology-address_2x.png"><img src="../images/ot-primer-rloc-topology-address.png" srcset="../images/ot-primer-rloc-topology-address.png 1x, ../images/ot-primer-rloc-topology-address_2x.png 2x" border="0" width="600" alt="OT Topology w/ Address" /></a>
</figure>

但是，因为 RLOC 是基于节点在拓扑中的位置的，所以节点的 RLOC 会随着拓扑的变化而改变。

例如，如果 Thread 网络中的 `0x400` 节点离开了网络，那么它的子节点 `0x401` 和 `0x402` 会与其它的 Router 建立新连接，从而获得新的 RLOC16 和 RLOC：

<figure>
<a href="../images/ot-primer-rloc-topology-change_2x.png"><img src="../images/ot-primer-rloc-topology-change.png" srcset="../images/ot-primer-rloc-topology-change.png 1x, ../images/ot-primer-rloc-topology-change_2x.png 2x" border="0" width="600" alt="OT Topology after Change" /></a>
</figure>

## 单播地址类型

RLOC 只是 Thread 设备可以获得的多种 IPv6 单播地址之一。另一类用于在 Thread 网络分区内标识唯一的 Thread 接口的地址称为 EID（Endpoint Identifier）。EID 与 Thread 网络拓扑无关。

常见的单播类型如下。

<table>
  <tbody>
    <tr>
      <th colspan=2><h3>Link-Local Address (LLA)</h3></th>
    </tr>
    <tr>
      <td colspan=2 style="background-color:rgb(238, 241, 242)">一种用于标识单次射频传输可访问的 Thread 接口的 EID。</td>
    </tr>
    <tr>
      <td width="25%" style="background-color:rgb(238, 241, 242)"><b>示例</b></td><td><code>fe80::54db:881c:3845:57f4</code></td>
    </tr>
    <tr>
      <td width="25%" style="background-color:rgb(238, 241, 242)"><b>IID</b></td><td>基于 802.15.4 Extended Address</td>
    </tr>
    <tr>
      <td width="25%" style="background-color:rgb(238, 241, 242)"><b>域</b></td><td>Link-Local</td>
    </tr>
    <tr>
      <td width="25%" style="background-color:rgb(238, 241, 242)"><b>详情</b></td><td><ul><li>用于发现邻居、配置链路和交换路由信息</li><li>非可路由地址</li><li>总是带 <code>fe80::/16</code> Prefix</li></ul></td>
    </tr>
  </tbody>
</table>

<table>
  <tbody>
    <tr>
      <th colspan=2><h3>Mesh-Local EID (ML-EID)</h3></th>
    </tr>
    <tr>
      <td colspan=2 style="background-color:rgb(238, 241, 242)">一种用于标识 Thread 接口的 EID，其与网络拓扑无关。用于访问同一 Thread 分区内的 Thread 接口。也称为 ULA（Unique Local Address）。</td>
    </tr>
    <tr>
      <td width="25%" style="background-color:rgb(238, 241, 242)"><b>示例</b></td><td><code>fde5:8dba:82e1:1:416:993c:8399:35ab</code></td>
    </tr>
    <tr>
      <td width="25%" style="background-color:rgb(238, 241, 242)"><b>IID</b></td><td>在 commissioning 完成后随机生成</td>
    </tr>
    <tr>
      <td width="25%" style="background-color:rgb(238, 241, 242)"><b>域</b></td><td>Mesh-Local</td>
    </tr>
    <tr>
      <td width="25%" style="background-color:rgb(238, 241, 242)"><b>详情</b></td><td><ul><li>不会随拓扑变化而变化</li><li>应由应用程序使用</li><li>总是带 <code>fd00::/8</code> Prefix</li></ul></td>
    </tr>
  </tbody>
</table>

<table>
  <tbody>
    <tr>
      <th colspan=2><h3>Routing Locator (RLOC)</h3></th>
    </tr>
    <tr>
      <td colspan=2 style="background-color:rgb(238, 241, 242)">根据 Thread 接口在网络拓扑中的位置来对其进行标识。</td>
    </tr>
    <tr>
      <td width="25%" style="background-color:rgb(238, 241, 242)"><b>示例</b></td><td><code>fde5:8dba:82e1:1::ff:fe00:1001</code></td>
    </tr>
    <tr>
      <td width="25%" style="background-color:rgb(238, 241, 242)"><b>IID</b></td><td><code>0000:00ff:fe00:<var>RLOC16</var></code></td>
    </tr>
    <tr>
      <td width="25%" style="background-color:rgb(238, 241, 242)"><b>域</b></td><td>Mesh-Local</td>
    </tr>
    <tr>
      <td width="25%" style="background-color:rgb(238, 241, 242)"><b>详情</b></td><td><ul><li>在设备连接到网络后生成</li><li>用于在 Thread 网络中传递 IPv6 数据报</li><li>随拓扑变化而变化</li><li>通常不会由应用程序使用</li></ul></td>
    </tr>
  </tbody>
</table>

<table>
  <tbody>
    <tr>
      <th colspan=2><h3>Anycast Locator (ALOC)</h3></th>
    </tr>
    <tr>
      <td colspan=2 style="background-color:rgb(238, 241, 242)">用于标识 Thread 网络分区中一个或多个 Thread 接口的位置。如果始发者不知道目的地的 RLOC，则使用 ALOC 进行查找。</td>
    </tr>
    <tr>
      <td width="25%" style="background-color:rgb(238, 241, 242)"><b>示例</b></td><td><code>fde5:8dba:82e1:1::ff:fe00:fc01</code></td>
    </tr>
    <tr>
      <td width="25%" style="background-color:rgb(238, 241, 242)"><b>IID</b></td><td><code>0000:00ff:fe00:fc<var>XX</var></code></td>
    </tr>
    <tr>
      <td width="25%" style="background-color:rgb(238, 241, 242)"><b>域</b></td><td>Mesh-Local</td>
    </tr>
    <tr>
      <td width="25%" style="background-color:rgb(238, 241, 242)"><b>详情</b></td><td><ul><li><code>fc<var>XX</var></code> = <a href="#anycast">ALOC 目的地址</a>，用于查询对应的 RLOC</li><li>通常不会由应用程序使用</li></td>
    </tr>
  </tbody>
</table>

<table>
  <tbody>
    <tr>
      <th colspan=2><h3>Global Unicast Address (GUA)</h3></th>
    </tr>
    <tr>
      <td colspan=2 style="background-color:rgb(238, 241, 242)">一个EID，用于标识除 Thread 网络外的全局范围内的 Thread 接口。</td>
    </tr>
    <tr>
      <td width="25%" style="background-color:rgb(238, 241, 242)"><b>示例</b></td><td><code>2000::54db:881c:3845:57f4</code></td>
    </tr>
    <tr>
      <td width="25%" style="background-color:rgb(238, 241, 242)"><b>IID</b></td><td><ul><li>SLAAC — 由设备自身随机分配</li><li>DHCP — 由 DHCPv6 服务器分配</li><li>Manual — 由应用层分配</li></ul></td>
    </tr>
    <tr>
      <td width="25%" style="background-color:rgb(238, 241, 242)"><b>域</b></td><td>Global</td>
    </tr>
    <tr>
      <td width="25%" style="background-color:rgb(238, 241, 242)"><b>详情</b></td><td><ul><li>一个公开的 IPv6 地址</li><li>总是带 <code>2000::/3</code> Prefix</li></td>
    </tr>
  </tbody>
</table>

## 多播

多播用于一次将信息传达给多个设备。Thread 网络中保留了特定的地址，以提供给不同分组的设备在多播时使用。

| IPv6 地址 | 域         | 传递给          |
| --------- | ---------- | --------------- |
| `ff02::1` | Link-Local | 所有 FTD 和 MED |
| `ff02::2` | Link-Local | 所有 FTD        |
| `ff03::1` | Mesh-Local | 所有 FTD 和 MED |
| `ff03::2` | Mesh-Local | 所有 FTD        |

Key Point: FTD 和 MTD 之间的主要区别在于 FTD 订阅了 `ff03::2` 多播地址。而 MTD 没有订阅。

你可能会注意到，上面的多播表中没有将 SED 作为接收者包括在内。Thread 为所有 Thread 节点（包括 SED）定义了（link-local 和 realm-local 域）基于单播 prefix 的 IPv6 多播地址。这些多播地址基于单播 Mesh-Local prefix 构成，因 Thread 网络而异。（有关基于单播 prefix 的 IPv6 多播地址的详情，请参阅 [RFC 3306](https://tools.ietf.org/html/rfc3306)）。

Thread 设备还支持除表中所列举域之外的任意域。

## 任播

当目的地的 RLOC 未知时，可以使用任播将流量路由到 Thread 接口。ALOC（Anycast Locator）标识 Thread 分区内多个接口的位置。ALOC 的后 16 位，称为 ALOC16，其格式为 <code>0xfc<var>XX</var></code>，表示 ALOC 的类型。

例如，`0xfc01` 和 `0xfc0f` 之间的 ALOC16 保留给了 DHCPv6 Agent。如果特定的 DHCPv6 Agent RLOC 是未知的（可能是因为网络拓扑已更改），则可以将消息发送到 DHCPv6 Agent ALOC 以获取 RLOC。

Thread 定义了以下 ALOC16 值：

| ALOC16                                     | 类型                     |
| ------------------------------------------ | ------------------------ |
| `0xfc00`                                   | Leader                   |
| `0xfc01` – `0xfc0f`                        | DHCPv6 Agent             |
| `0xfc10` – `0xfc2f`                        | Service                  |
| `0xfc30` – `0xfc37`                        | Commissioner             |
| `0xfc40` – `0xfc4e`                        | Neighbor Discovery Agent |
| `0xfc38` – `0xfc3f`<br>`0xfc4f` – `0xfcff` | Reserved                 |

## 回顾

你应该学到了：

* Thread 网络包含三个域：Link-Local、Mesh-Local 和 Global
* Thread 设备具有多种单播 IPv6 地址
* RLOC 表示设备在 Thread 网络中的位置
* ML-EID 对于分区内的 Thread 设备是唯一的，并且应由应用程序使用
* Thread 使用多播将数据转发到节点组和 Router 组
* 当目的地的 RLOC 未知时，Thread 可以使用任播

要了解有关 Thread 的 IPv6 寻址的更多信息，请参阅 [Thread Specification](http://threadgroup.org/ThreadSpec) 的 5.2 和 5.3 节。
