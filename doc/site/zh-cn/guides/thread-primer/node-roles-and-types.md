# 节点角色和类型

## 转发角色

<figure class="attempt-right">
<a href="../images/ot-primer-roles_2x.png"><img src="../images/ot-primer-roles.png" srcset="../images/ot-primer-roles.png 1x, ../images/ot-primer-roles_2x.png 2x" border="0" alt="OT Node Roles" /></a>
</figure>

在 Thread 网络中，节点分成两种转发角色：Router 和 End Device。

### Router

Router 节点的行为如下：

* 为网络设备转发数据包
* 为尝试加入网络的设备提供安全的 commissioning 服务
* 始终打开它的收发器

### End Device

End Device 节点的行为如下：

* 主要与单个 Router 进行通信
* 不会为其他网络设备转发数据包
* 可以关闭它的收发器来降低功耗

Key Point: Router 和 End Device 之间的关系称为父子关系。End Device 正确地依附到一个 Router 上，其中 Router 始终作为父节点，End Device 则始终是子节点。

## 设备类型

此外，节点有许多种类型。

<figure class="attempt-right">
<a href="../images/ot-primer-taxonomy_2x.png"><img src="../images/ot-primer-taxonomy.png" srcset="../images/ot-primer-taxonomy.png 1x, ../images/ot-primer-taxonomy.png 2x" border="0" alt="OT Device Taxonomy" /></a>
</figure>

### Full Thread Device

一个 FTD（Full Thread Device）总是打开它的射频收发器，它订阅所有 Router 的多播地址，并维护 IPv6 地址映射。FTD 有三种类型：

* Router
* REED（Router Eligible End Device）— 可以升级为 Router
* FED（Full End Device）— 无法升级为 Router

FTD 可以作为 Router（父）或 End Device（子）。

### Minimal Thread Device

MTD（Minimal Thread Device）不会订阅 all-routers 多播地址，并且它会将它的所有消息发送给它的父节点。MTD 有两种类型：

* MED（Minimal End Device）— 始终打开自身的收发器，无需从父节点中轮询消息
* SED（Sleepy End Device）— 通常会关闭自身的收发器（睡眠），偶然会打开收发器（唤醒）以从父节点中轮询消息

MTD 只能作为 End Device（子）。

### 升级和降级

当待加入设备仅能与某个 REED 通信时, 则该 REED 可以升级成为 Router：

<figure>
<a href="../images/ot-primer-router-upgrade_2x.png"><img src="../images/ot-primer-router-upgrade.png" srcset="../images/ot-primer-router-upgrade.png 1x, ../images/ot-primer-router-upgrade_2x.png 2x" border="0" width="400" alt="OT End Device to Router" /></a>
</figure>

相反，当一个 Router 没有子节点时，它可以降级成 End Device：

<figure>
<a href="../images/ot-primer-router-downgrade_2x.png"><img src="../images/ot-primer-router-downgrade.png" srcset="../images/ot-primer-router-downgrade.png 1x, ../images/ot-primer-router-downgrade_2x.png 2x" border="0" width="400" alt="OT Router to End Device" /></a>
</figure>

## 其他角色和类型

### Thread Leader

<figure class="attempt-right">
<a href="../images/ot-primer-leader_2x.png"><img src="../images/ot-primer-leader.png" srcset="../images/ot-primer-leader.png 1x, ../images/ot-primer-leader_2x.png 2x" border="0" alt="OT Leader and Border Router" /></a>
</figure>

Thread Leader 是一个 Router，它负责管理 Thread 网络中的 Router。Thread Leader 是动态自选的（提高容错率），它负责汇总和分发全网络的配置信息。

Note: 每个 Thread 网络[分区](#分区)中总是只有一个 Leader。

### Border Router

Border Router 是一种可以在 Thread 网络和其他网络（如 Wi-Fi）之间转发信息的设备。它还为外部连接配置 Thread 网络。

任何设备都可以充当 Border Router。

Note: 一个 Thread 网络中可以有多个 Border Router。

## 分区

<figure class="attempt-right">
<a href="../images/ot-primer-partitions_2x.png"><img src="../images/ot-primer-partitions.png" srcset="../images/ot-primer-partitions.png 1x, ../images/ot-primer-partitions_2x.png 2x" border="0" alt="OT Partitions" /></a>
</figure>

一个 Thread 网络可能由多个分区组成。当一组 Thread 设备不能再与另一组 Thread 设备通信时，会发生这种情况。每个分区在逻辑上均作为独立的 Thread 网络来运行，它们具有各自的 Leader、Router ID 分配和网络数据，同时分区前相同的安全凭证都将被保留下来。

当分区之间可以连通时，它们会自动合并。

Key Point: 安全凭证（security credentials）定义了 Thread 网络。物理无线电的连通性定义了该 Thread 网络中的分区。

请注意，在本入门教程中一般将 Thread 网络假定成单个分区。在必要时，将使用“分区”一词来阐明关键概念和示例。本教程稍后将详细介绍分区。

## 设备限制

单个 Thread 网络所支持的设备类型数量是有限制的。

| 角色       | 限制               |
| ---------- | ------------------ |
| Leader     | 1                  |
| Router     | 32                 |
| End Device | 511（每个 Router） |

Thread 会尝试将 Router 的数量保持在 16 ～ 23 之间。如果一个 REED 作为 End Device 加入，并且网络中的 Router 数量低于 16，那么它将自动升级为 Router。

## 回顾

你应该学到了：

* Thread 设备可以是 Router（父）或 End Device（子）
* Thread 设备可以是 FTD（维护 IPv6 地址映射），也可以是 MTD（将所有消息发送给其父节点）
* REED 可以升级为 Router，Router 也可以降级为 REED
* 每个 Thread 网络分区都有一个 Leader 来管理 Router
* Border Router 用于连接 Thread 和其他网络
* 一个 Thread 网络可能由多个分区组成
