# OpenThread CLI - Operational Datasets

## Overview

Thread network configuration parameters are managed using Active and Pending Operational Dataset objects.

### Active Operational Dataset

The Active Operational Dataset includes parameters that are currently in use across an entire Thread network. The Active Operational Dataset contains:

- Active Timestamp
- Channel
- Channel Mask
- Extended PAN ID
- Mesh-Local Prefix
- Network Name
- PAN ID
- PSKc
- Security Policy

### Pending Operational Dataset

The Pending Operational Dataset is used to communicate changes to the Active Operational Dataset before they take effect. The Pending Operational Dataset contains all the parameters from the Active Operational Dataset, with the addition of:

- Delay Timer
- Pending Timestamp

## Quick Start

### Form Network

1. Generate and view new network configuration.

   ```bash
   > dataset init new
   Done
   > dataset
   Active Timestamp: 1
   Channel: 15
   Channel Mask: 0x07fff800
   Ext PAN ID: 39758ec8144b07fb
   Mesh Local Prefix: fdf1:f1ad:d079:7dc0::/64
   Network Key: f366cec7a446bab978d90d27abe38f23
   Network Name: OpenThread-5938
   PAN ID: 0x5938
   PSKc: 3ca67c969efb0d0c74a4d8ee923b576c
   Security Policy: 672 onrc
   Done
   ```

2. Commit new dataset to the Active Operational Dataset in non-volatile storage.

   ```bash
   dataset commit active
   Done
   ```

3. Enable Thread interface

   ```bash
   > ifconfig up
   Done
   > thread start
   Done
   ```

### Attach to Existing Network

Only the Network Key is required for a device to attach to a Thread network.

While not required, specifying the channel avoids the need to search across multiple channels, improving both latency and efficiency of the attach process.

After the device successfully attaches to a Thread network, the device will retrieve the complete Active Operational Dataset.

1. Create a partial Active Operational Dataset.

   ```bash
   > dataset networkkey dfd34f0f05cad978ec4e32b0413038ff
   Done
   > dataset commit active
   Done
   ```

2. Enable Thread interface.

   ```bash
   > ifconfig up
   Done
   > thread start
   Done
   ```

3. After attaching, validate that the device received the complete Active Operational Dataset.

   ```bash
   > dataset active
   Active Timestamp: 1
   Channel: 15
   Channel Mask: 0x07fff800
   Ext PAN ID: 39758ec8144b07fb
   Mesh Local Prefix: fdf1:f1ad:d079:7dc0::/64
   Network Key: f366cec7a446bab978d90d27abe38f23
   Network Name: OpenThread-5938
   PAN ID: 0x5938
   PSKc: 3ca67c969efb0d0c74a4d8ee923b576c
   Security Policy: 672 onrc
   Done
   ```

## Command List

- [help](#help)
- [active](#active)
- [activetimestamp](#activetimestamp)
- [channel](#channel)
- [channelmask](#channelmask)
- [clear](#clear)
- [commit](#commit)
- [delay](#delay)
- [extpanid](#extpanid)
- [init](#init)
- [meshlocalprefix](#meshlocalprefix)
- [mgmtgetcommand](#mgmtgetcommand)
- [mgmtsetcommand](#mgmtsetcommand)
- [networkkey](#networkkey)
- [networkname](#networkname)
- [panid](#panid)
- [pending](#pending)
- [pendingtimestamp](#pendingtimestamp)
- [pskc](#pskc)
- [securitypolicy](#securitypolicy)

## Command Details

### help

Usage: `dataset help`

Print dataset help menu.

```bash
> dataset help
help
active
activetimestamp
channel
channelmask
clear
commit
delay
extpanid
init
meshlocalprefix
mgmtgetcommand
mgmtsetcommand
networkkey
networkname
panid
pending
pendingtimestamp
pskc
securitypolicy
Done
```

### active

Usage: `dataset active [-x]`

Print Active Operational Dataset in human-readable form.

```bash
> dataset active
Active Timestamp: 1
Channel: 15
Channel Mask: 0x07fff800
Ext PAN ID: 39758ec8144b07fb
Mesh Local Prefix: fdf1:f1ad:d079:7dc0::/64
Network Key: f366cec7a446bab978d90d27abe38f23
Network Name: OpenThread-5938
PAN ID: 0x5938
PSKc: 3ca67c969efb0d0c74a4d8ee923b576c
Security Policy: 672 onrc
Done
```

Print Active Operational Dataset as hex-encoded TLVs.

```bash
> dataset active -x
0e080000000000010000000300000f35060004001fffe0020839758ec8144b07fb0708fdf1f1add0797dc00510f366cec7a446bab978d90d27abe38f23030f4f70656e5468726561642d353933380102593804103ca67c969efb0d0c74a4d8ee923b576c0c0402a0f7f8
Done
```

### activetimestamp

Usage: `dataset activetimestamp [timestamp]`

Get active timestamp seconds.

```bash
> dataset activetimestamp
123456789
Done
```

Set active timestamp seconds.

```bash
> dataset activetimestamp 123456789
Done
```

### channel

Usage: `channel [channel]`

Get channel.

```bash
> dataset channel
12
Done
```

Set channel.

```bash
> dataset channel 12
Done
```

### channelmask

Usage: `dataset channelmask [channelmask]`

Get channel mask.

```bash
> dataset channelmask
0x07fff800
Done
```

Set channel mask.

```bash
> dataset channelmask 0x07fff800
Done
```

### clear

Usage: `dataset clear`

Reset operational dataset buffer.

```bash
> dataset clear
Done
```

### commit

Usage: `dataset commit <active|pending>`

Commit operational dataset buffer to active/pending operational dataset.

```bash
> dataset commit active
Done
```

### delay

Usage: `dataset delay [delay]`

Get delay timer value.

```bash
> dataset delay
1000
Done
```

Set delay timer value.

```bash
> dataset delay 1000
Done
```

### extpanid

Usage: `dataset extpanid [extpanid]`

Get extended panid.

```bash
> dataset extpanid
000db80123456789
Done
```

Set extended panid.

**NOTE** The commissioning credential in the dataset buffer becomes stale after changing this value. Use [pskc](#pskc) to reset.

```bash
> dataset extpanid 000db80123456789
Done
```

### init

Usage: `dataset init <active|new|pending|tlvs <hex-encoded TLVs>>`

Initialize operational dataset buffer.

```bash
> dataset init new
Done
```

### meshlocalprefix

Usage: `dataset meshlocalprefix [prefix]`

Get mesh local prefix.

```bash
> dataset meshlocalprefix
fd00:db8:0:0::/64
Done
```

Set mesh local prefix.

```bash
> dataset meshlocalprefix fd00:db8::
Done
```

### mgmtgetcommand

Usage: `dataset mgmtgetcommand <active|pending> [address <destination>] [TLV list] [-x]`

Send MGMT_ACTIVE_GET or MGMT_PENDING_GET.

```bash
> dataset mgmtgetcommand active address fdde:ad00:beef:0:558:f56b:d688:799 activetimestamp securitypolicy
Done
```

### mgmtsetcommand

Usage: `dataset mgmtsetcommand <active|pending> [TLV Type list] [-x]`

Send MGMT_ACTIVE_SET or MGMT_PENDING_SET.

```bash
> dataset mgmtsetcommand active activetimestamp 123 securitypolicy 1 onrc
Done
```

### networkkey

Usage: `dataset networkkey [key]`

Get network key

```bash
> dataset networkkey
00112233445566778899aabbccddeeff
Done
```

Set network key.

```bash
> dataset networkkey 00112233445566778899aabbccddeeff
Done
```

### networkname

Usage: `dataset networkname [name]`

Get network name.

```bash
> datset networkname
OpenThread
Done
```

Set network name.

**NOTE** The commissioning credential in the dataset buffer becomes stale after changing this value. Use [pskc](#pskc) to reset.

```bash
> dataset networkname OpenThread
Done
```

### panid

Usage: `dataset panid [panid]`

Get panid.

```bash
> dataset panid
0x1234
Done
```

Set panid.

```bash
> dataset panid 0x1234
Done
```

### pending

Usage: `dataset pending [-x]`

Print Pending Operational Dataset in human-readable form.

```bash
> dataset pending
Pending Timestamp: 2
Active Timestamp: 1
Channel: 26
Channel Mask: 0x07fff800
Delay: 58706
Ext PAN ID: a74182f4d3f4de41
Mesh Local Prefix: fd46:c1b9:e159:5574::/64
Network Key: ed916e454d96fd00184f10a6f5c9e1d3
Network Name: OpenThread-bff8
PAN ID: 0xbff8
PSKc: 264f78414adc683191863d968f72d1b7
Security Policy: 672 onrc
Done
```

Print Pending Operational Dataset as hex-encoded TLVs.

```bash
> dataset pending -x
0e0800000000000100003308000000000002000034040000b512000300001a35060004001fffe00208a74182f4d3f4de410708fd46c1b9e15955740510ed916e454d96fd00184f10a6f5c9e1d3030f4f70656e5468726561642d626666380102bff80410264f78414adc683191863d968f72d1b70c0402a0f7f8
Done
```

### pendingtimestamp

Usage: `dataset pendingtimestamp [timestamp]`

Get pending timestamp seconds.

```bash
> dataset pendingtimestamp
123456789
Done
```

Set pending timestamp seconds.

```bash
> dataset pendingtimestamp 123456789
Done
```

### pskc

Usage: `pskc [-p] [<key>|<passphrase>]`

Get pskc.

```bash
> dataset pskc
67c0c203aa0b042bfb5381c47aef4d9e
Done
```

Set pskc.

With `-p`(**only for FTD**) generate pskc from \<passphrase\> (UTF-8 encoded) together with network name and extended PAN ID in the dataset buffer if set or values in the current stack if not, otherwise set pskc as \<key\> (hex format).

```bash
> dataset pskc 67c0c203aa0b042bfb5381c47aef4d9e
Done
> dataset pskc -p 123456
Done
```

### securitypolicy

Usage: `dataset securitypolicy [<rotationtime> [onrcCepR]]`

Get security policy.

```bash
> dataset securitypolicy
672 onrc
Done
```

Set security policy.

- o: Obtaining the Network Key for out-of-band commissioning is enabled.
- n: Native Commissioning using PSKc is allowed.
- r: Thread 1.x Routers are enabled.
- c: External Commissioner authentication is allowed using PSKc.
- C: Thread 1.2 Commercial Commissioning is enabled.
- e: Thread 1.2 Autonomous Enrollment is enabled.
- p: Thread 1.2 Network Key Provisioning is enabled.
- R: Non-CCM routers are allowed in Thread 1.2 CCM networks.

```bash
> dataset securitypolicy 672 onrc
Done
```

### set

Usage: `dataset set <active|pending> <dataset>`

Set the Active Operational Dataset using hex-encoded TLVs.

```bash
dataset set active 0e080000000000010000000300000f35060004001fffe0020839758ec8144b07fb0708fdf1f1add0797dc00510f366cec7a446bab978d90d27abe38f23030f4f70656e5468726561642d353933380102593804103ca67c969efb0d0c74a4d8ee923b576c0c0402a0f7f8
Done
```

Set the Pending Operational Dataset using hex-encoded TLVs.

```bash
dataset set pending 0e0800000000000100003308000000000002000034040000b512000300001a35060004001fffe00208a74182f4d3f4de410708fd46c1b9e15955740510ed916e454d96fd00184f10a6f5c9e1d3030f4f70656e5468726561642d626666380102bff80410264f78414adc683191863d968f72d1b70c0402a0f7f8
Done
```
