# OpenThread CLI Example

This example application exposes OpenThread configuration and management APIs via a simple command-line interface. The steps below take you through the minimal steps required to ping one emulated Thread device from another emulated Thread device.

## 1. Build

```bash
$ cd <path-to-openthread>
$ ./bootstrap
$ make -f examples/Makefile-simulation
```

## 2. Start node 1

Spawn the process:

```bash
$ cd <path-to-openthread>/output/<platform>/bin
$ ./ot-cli-ftd 1
```

Generate, view, and commit a new Active Operational Dataset:

```bash
> dataset init new
Done
> dataset
Active Timestamp: 1
Channel: 13
Channel Mask: 0x07fff800
Ext PAN ID: d63e8e3e495ebbc3
Mesh Local Prefix: fd3d:b50b:f96d:722d::/64
Network Key: dfd34f0f05cad978ec4e32b0413038ff
Network Name: OpenThread-8f28
PAN ID: 0x8f28
PSKc: c23a76e98f1a6483639b1ac1271e2e27
Security Policy: 0, onrc
Done
> dataset commit active
Done
```

Bring up the IPv6 interface:

```bash
> ifconfig up
Done
```

Start Thread protocol operation:

```bash
> thread start
Done
```

Wait a few seconds and verify that the device has become a Thread Leader:

```bash
> state
leader
Done
```

View IPv6 addresses assigned to Node 1's Thread interface:

```bash
> ipaddr
fd3d:b50b:f96d:722d:0:ff:fe00:fc00
fd3d:b50b:f96d:722d:0:ff:fe00:c00
fd3d:b50b:f96d:722d:7a73:bff6:9093:9117
fe80:0:0:0:6c41:9001:f3d6:4148
Done
```

## 2. Start node 2

Spawn the process:

```bash
$ cd <path-to-openthread>/output/<platform>/bin
$ ./ot-cli-ftd 2
```

Configure Thread Network Key from Node 1's Active Operational Dataset:

```bash
> dataset networkkey dfd34f0f05cad978ec4e32b0413038ff
Done
> dataset commit active
Done
```

Bring up the IPv6 interface:

```bash
> ifconfig up
Done
```

Start Thread protocol operation:

```bash
> thread start
Done
```

Wait a few seconds and verify that the device has become a Thread Child or Router:

```bash
> state
child
Done
```

## 3. Ping Node 1 from Node 2

```bash
> ping fd3d:b50b:f96d:722d:7a73:bff6:9093:9117
16 bytes from fd3d:b50b:f96d:722d:558:f56b:d688:799: icmp_seq=1 hlim=64 time=24ms
```

## 4. Explore More

See the [OpenThread CLI Reference README.md](../../../src/cli/README.md) to explore more.
