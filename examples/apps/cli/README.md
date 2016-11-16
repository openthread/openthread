# OpenThread CLI Example

This example application demonstrates a minimal OpenThread application
that exposes the OpenThread configuration and management interfaces
via a basic command-line interface. The steps below take you through
the minimal steps required to ping one emulated Thread device from
another emulated Thread device.

## 1. Build

```bash
$ cd <path-to-openthread>
$ ./bootstrap
$ make -f examples/Makefile-posix
```

## 2. Start node 1

Spawn the process:

```bash
$ cd <path-to-openthread>/output/<platform>/bin
$ ./ot-cli-ftd 1
```

Set the PAN ID:

```bash
> panid 0x1234
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
fdde:ad00:beef:0:0:ff:fe00:0
fdde:ad00:beef:0:558:f56b:d688:799
fe80:0:0:0:f3d9:2a82:c8d8:fe43
Done
```

## 2. Start node 2

Spawn the process:

```bash
$ cd <path-to-openthread>/output/<platform>/bin
$ ./ot-cli-ftd 2
```

Set the PAN ID:

```bash
> panid 0x1234
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

Wait a few seconds and verify that the device has become a Thread Router:

```bash
> state
router
Done
```

## 3. Ping Node 1 from Node 2

```bash
> ping fdde:ad00:beef:0:558:f56b:d688:799
16 bytes from fdde:ad00:beef:0:558:f56b:d688:799: icmp_seq=1 hlim=64
```

## 4. Want more?

You may note that the example above did not include any network parameter configuration, such as the IEEE 802.15.4 PAN ID or the Thread Master Key. OpenThread currently implements default values for network parameters, however, you may use the CLI to change network parameters, other configurations, and perform other operations.

See the [OpenThread CLI Reference README.md](../../../src/cli/README.md) to explore more.
