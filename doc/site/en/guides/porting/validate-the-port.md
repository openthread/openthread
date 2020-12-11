# Validate the Port

Basic validation is necessary to verify a successful port of OpenThread to a new
hardware platform example.

## Step 1: Compile for the target platform

Demonstrate a successful build by compiling the example OpenThread application
for the target platform.

```
$ ./bootstrap
$ make -f examples/Makefile-efr32 COMMISSIONER=1 JOINER=1
```

## Step 2: Interact with the CLI

Demonstrate successful OpenThread execution and UART capability by interacting
with the CLI.

Open a terminal to `/dev/ttyACM0` (serial port settings: 115200 8-N-1). Type
`help` for a list of commands.

> Note:  The set of CLI commands will vary based on the features enabled in a
particular build. The majority of them have been elided in the example output
below.

```
> help
help
autostart
bufferinfo
...
version
whitelist
```

## Step 3: Form a Thread network

Demonstrate successful protocol timers by forming a Thread network and verifying
the node has transitioned to the Leader state.

```
> dataset init new
Done
> dataset
Active Timestamp: 1
Channel: 13
Channel Mask: 07fff800
Ext PAN ID: d63e8e3e495ebbc3
Mesh Local Prefix: fd3d:b50b:f96d:722d/64
Master Key: dfd34f0f05cad978ec4e32b0413038ff
Network Name: OpenThread-8f28
PAN ID: 0x8f28
PSKc: c23a76e98f1a6483639b1ac1271e2e27
Security Policy: 0, onrcb
Done
> dataset commit active
Done
> ifconfig up
Done
> thread start
Done
```

Wait a couple of seconds...

```
> state
leader
Done
```

## Step 4: Attach a second node 

Demonstrate successful radio communication by attaching a second node to the
newly formed Thread network, using the same Thread Master Key and PAN ID from
the first node:

```
> dataset masterkey dfd34f0f05cad978ec4e32b0413038ff
Done
> dataset panid 0x8f28
Done
> dataset commit active
Done
> routerselectionjitter 1
Done
> ifconfig up
Done
> thread start
Done
```

Wait a couple of seconds...

```
> state
router
Done
```

## Step 5: Ping between devices

Demonstrate successful data path communication by sending/receiving ICMPv6 Echo
request/response messages.

List all IPv6 addresses of Leader:

```
> ipaddr
fdde:ad00:beef:0:0:ff:fe00:fc00
fdde:ad00:beef:0:0:ff:fe00:800
fdde:ad00:beef:0:5b:3bcd:deff:7786
fe80:0:0:0:6447:6e10:cf7:ee29
Done
```

Send an ICMPv6 ping from Router to Leader's Mesh-Local EID IPv6 address:

```
> ping fdde:ad00:beef:0:5b:3bcd:deff:7786
16 bytes from fdde:ad00:beef:0:5b:3bcd:deff:7786: icmp_seq=1 hlim=64 time=24ms
```

## Step 6: Reset a device and validate reattachment

Demonstrate non-volatile functionality by resetting the device and validating
its reattachment to the same network without user intervention.

Start a Thread network:

```
> dataset init new
Done
> dataset
Active Timestamp: 1
Channel: 13
Channel Mask: 07fff800
Ext PAN ID: d63e8e3e495ebbc3
Mesh Local Prefix: fd3d:b50b:f96d:722d/64
Master Key: dfd34f0f05cad978ec4e32b0413038ff
Network Name: OpenThread-8f28
PAN ID: 0x8f28
PSKc: c23a76e98f1a6483639b1ac1271e2e27
Security Policy: 0, onrcb
Done
> dataset commit active
Done
> ifconfig up
Done
> thread start
Done
```

Wait a couple of seconds and verify that the active dataset has been stored in
non-volatile storage:

```
> dataset active
Active Timestamp: 1
Channel: 13
Channel Mask: 07fff800
Ext PAN ID: d63e8e3e495ebbc3
Mesh Local Prefix: fd3d:b50b:f96d:722d/64
Master Key: dfd34f0f05cad978ec4e32b0413038ff
Network Name: OpenThread-8f28
PAN ID: 0x8f28
PSKc: c23a76e98f1a6483639b1ac1271e2e27
Security Policy: 0, onrcb
Done
```

Reset the device:

```
> reset
> ifconfig up
Done
> thread start
Done
```

Wait a couple of seconds and verify that the device has successfully reattached
to the network:

```
> panid
0x8f28
Done
> state
router
Done
```

## Step 7: Verify random number generation

Demonstrate random number generation by executing the `factoryreset` command and
verifying a new random extended address.

```
> extaddr
a660421703f3fdc3
Done
> factoryreset
```

Wait a couple of seconds...

```
> extaddr
9a8ed90715a5f7b6
Done
```
