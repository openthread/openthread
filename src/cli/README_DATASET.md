# OpenThread CLI - Operational Datasets

## Overview

Thread network configuration parameters are managed using Active and Pending Operational Dataset objects.

### WARNING - Restrictions for production use!

The CLI commands to write/change the Active and Pending Operational Datasets may allow setting invalid parameters, or invalid combinations of parameters, for testing purposes. These CLI commands can only be used:

- To configure network parameters for the first device in a newly created Thread network.
- For testing (not applicable to production devices).

In production Thread networks, the correct method to write/change Operational Datasets is via a [Commissioner](README_COMMISSIONER.md) that performs [commissioning](README_COMMISSIONING.md). Production devices that are not an active Commissioner and are part of a Thread network MUST NOT modify the Operational Datasets in any way.

### Active Operational Dataset

The Active Operational Dataset includes parameters that are currently in use across an entire Thread network. The Active Operational Dataset contains:

- Active Timestamp
- Channel
- Wake-up Channel
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
   Wake-up Channel: 16
   Channel Mask: 0x07fff800
   Ext PAN ID: 39758ec8144b07fb
   Mesh Local Prefix: fdf1:f1ad:d079:7dc0::/64
   Network Key: f366cec7a446bab978d90d27abe38f23
   Network Name: OpenThread-5938
   PAN ID: 0x5938
   PSKc: 3ca67c969efb0d0c74a4d8ee923b576c
   Security Policy: 672 onrc 0
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
   Wake-up Channel: 16
   Channel Mask: 0x07fff800
   Ext PAN ID: 39758ec8144b07fb
   Mesh Local Prefix: fdf1:f1ad:d079:7dc0::/64
   Network Key: f366cec7a446bab978d90d27abe38f23
   Network Name: OpenThread-5938
   PAN ID: 0x5938
   PSKc: 3ca67c969efb0d0c74a4d8ee923b576c
   Security Policy: 672 onrc 0
   Done
   ```

### Using the Dataset Updater to update Operational Dataset

Dataset Updater can be used for a delayed update of network parameters on all devices of a Thread Network.

1. Clear the dataset buffer and add the Dataset fields to update.

   ```bash
   > dataset clear
   Done

   > dataset channel 12
   Done
   ```

2. Set the delay timer parameter (example uses 5 minutes or 300000 ms). Check the resulting dataset. There is no need to specify active or pending timestamps because the Dataset Updater will handle this. If specified the `dataset updater start` will issue an error.

   ```bash
   > dataset delay 300000

   > dataset
   Channel: 12
   Delay: 30000
   Done
   ```

3. Start the Dataset Updater, which will prepare a Pending Operation Dataset and inform the Leader to distribute it to other devices.

   ```bash
   > dataset updater start
   Done

   > dataset updater
   Enabled
   ```

4. After about 5 minutes, the changes are applied to the Active Operational Dataset on the Leader. This can also be checked at other devices on the network: these should have applied the new Dataset too, at approximately the same time as the Leader has done this.

   ```bash
   > dataset active
   Active Timestamp: 10
   Channel: 12
   Channel Mask: 0x07fff800
   Ext PAN ID: 324a71d90cdc8345
   Mesh Local Prefix: fd7d:da74:df5e:80c::/64
   Network Key: be768535bac1b8d228960038311d6ca2
   Network Name: OpenThread-bcaf
   PAN ID: 0xbcaf
   PSKc: e79b274ab22414a814ed5cce6a30be67
   Security Policy: 672 onrc 0
   Done
   ```

### Using the Pending Operational Dataset for Delayed Dataset Updates

The Pending Operational Dataset can be used for a delayed update of network parameters on all devices of a Thread Network. If certain Active Operational Dataset parameters need to be changed, but the change would impact the connectivity of the network, delaying the update helps to let all devices receive the new parameters before the update is applied. Examples of such parameters are the channel, PAN ID, certain Security Policy bits, or Network Key.

The delay timer determines the time period after which the Pending Operational Dataset takes effect and becomes the Active Operational Dataset. The following example shows how a Pending Operational Dataset with delay timer can be set at a Leader device. The Leader will initiate the distribution of the Pending Operational Dataset to the rest of the devices in the network.

Normally, an active Commissioner will set a new Pending Operational Dataset. For testing purposes, we will do this in the example directly on the Leader using the CLI - so without using a Commissioner.

1. The main parameter to change is the channel. We can display the current Active Operational Dataset to see that the current channel is 16.

   ```bash
   > dataset active
   Active Timestamp: 1691070443
   Channel: 16
   Channel Mask: 0x07fff800
   Ext PAN ID: 324a71d90cdc8345
   Mesh Local Prefix: fd7d:da74:df5e:80c::/64
   Network Key: be768535bac1b8d228960038311d6ca2
   Network Name: OpenThread-bcaf
   PAN ID: 0xbcaf
   PSKc: e79b274ab22414a814ed5cce6a30be67
   Security Policy: 672 onrc 0
   Done
   ```

2. Create a new Dataset in the dataset buffer, by copying the Active Operational Dataset. Then change the channel number to 12 and increase the timestamp.

   ```bash
   > dataset init active
   Done
   > dataset activetimestamp 1696177379
   Done
   > dataset pendingtimestamp 1696177379
   Done
   > dataset channel 12
   Done
   ```

3. Set the delay timer parameter to 5 minutes (300000 ms). Show the resulting Dataset that's ready to be used.

   ```bash
   > dataset delay 300000
   Done
   > dataset
   Pending Timestamp: 1696177379
   Active Timestamp: 1696177379
   Channel: 12
   Channel Mask: 0x07fff800
   Delay: 300000
   Ext PAN ID: 324a71d90cdc8345
   Mesh Local Prefix: fd7d:da74:df5e:80c::/64
   Network Key: be768535bac1b8d228960038311d6ca2
   Network Name: OpenThread-bcaf
   PAN ID: 0xbcaf
   PSKc: e79b274ab22414a814ed5cce6a30be67
   Security Policy: 672 onrc 0
   Done
   ```

4. Commit the new Dataset as the Pending Operational Dataset. This also starts the delay timer countdown. The Leader then starts the distribution of the Pending Operational Dataset to other devices in the network.

   ```bash
   > dataset commit pending
   Done
   ```

5. To verify that the delay timer is counting down, display the Pending Operational Dataset after a few seconds.

   ```bash
   > dataset pending
   Pending Timestamp: 1696177379
   Active Timestamp: 1696177379
   Channel: 12
   Channel Mask: 0x07fff800
   Delay: 293051
   Ext PAN ID: 324a71d90cdc8345
   Mesh Local Prefix: fd7d:da74:df5e:80c::/64
   Network Key: be768535bac1b8d228960038311d6ca2
   Network Name: OpenThread-bcaf
   PAN ID: 0xbcaf
   PSKc: e79b274ab22414a814ed5cce6a30be67
   Security Policy: 672 onrc 0
   Done
   ```

   This shows that indeed the delay timer has started counting down from its initial value `300000`. The same can be optionally checked on other devices in the network.

6) After about 5 minutes, check that the Pending Operational Dataset has been applied at the Leader. This can also be checked at other devices on the network: these should have applied the new Dataset too, at approximately the same time as the Leader has done this.

   ```bash
   > dataset active
   Active Timestamp: 1696177379
   Channel: 12
   Channel Mask: 0x07fff800
   Ext PAN ID: 324a71d90cdc8345
   Mesh Local Prefix: fd7d:da74:df5e:80c::/64
   Network Key: be768535bac1b8d228960038311d6ca2
   Network Name: OpenThread-bcaf
   PAN ID: 0xbcaf
   PSKc: e79b274ab22414a814ed5cce6a30be67
   Security Policy: 672 onrc 0
   Done
   ```

   This shows that the Active Operational Dataset has now been updated to use channel 12. And the Pending Operational Dataset is no longer present, as can be seen by this command:

   ```bash
   > dataset pending
   Error 23: NotFound
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
- [tlvs](#tlvs)
- [updater](#updater)
- [wakeupchannel](#wakeupchannel)

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
set
tlvs
wakeupchannel
Done
```

### active

Usage: `dataset active [-x]`

Print Active Operational Dataset in human-readable form.

```bash
> dataset active
Active Timestamp: 1
Channel: 15
Wake-up Channel: 16
Channel Mask: 0x07fff800
Ext PAN ID: 39758ec8144b07fb
Mesh Local Prefix: fdf1:f1ad:d079:7dc0::/64
Network Key: f366cec7a446bab978d90d27abe38f23
Network Name: OpenThread-5938
PAN ID: 0x5938
PSKc: 3ca67c969efb0d0c74a4d8ee923b576c
Security Policy: 672 onrc 0
Done
```

Print Active Operational Dataset as hex-encoded TLVs.

```bash
> dataset active -x
0e08000000000001000000030000164a0300001735060004001fffe00208b182e6a17996cecc0708fd3f363fa8f1b0bc0510ebb6f6a447c96e1542176df3a834ac0e030f4f70656e5468726561642d3663393901026c99041096e9cdfe1eb1363a3676e2b94df0271b0c0402a0f7f8
Done
```

### activetimestamp

Usage: `dataset activetimestamp [timestamp]`

Get active timestamp seconds. It represents a "Unix time", in number of seconds since Jan 1st, 1970.

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

Get delay timer value. The timer value is in milliseconds.

```bash
> dataset delay
1000
Done
```

Set delay timer value.

```bash
> dataset delay 100000
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

Initialize operational dataset buffer. Use `new` to initialize with randomly selected values:

```bash
> dataset init new
Done
```

Use `active` or `pending` to initialize the dataset buffer with a copy of the current Active Operational Dataset or Pending Operational Dataset, respectively:

```bash
> dataset init active
Done
```

Use the `tlvs` option to initialize the dataset buffer from a string of hex-encoded TLVs:

```bash
> dataset init tlvs 0e080000000000010000000300001235060004001fffe002088665f03e6e42e7750708fda576e5f9a5bd8c0510506071d8391be671569e080d52870fd5030f4f70656e5468726561642d633538640102c58d04108a926cf8b13275a012ceedeeae40910d0c0402a0f7f8
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
> dataset mgmtsetcommand active activetimestamp 123 securitypolicy 1 onrc 0
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
> dataset networkname
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
Security Policy: 672 onrc 0
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

Get pending timestamp seconds. It represents a "Unix time", in number of seconds since Jan 1st, 1970.

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

Get PSKc.

```bash
> dataset pskc
67c0c203aa0b042bfb5381c47aef4d9e
Done
```

Set PSKc.

With `-p`(**only for FTD**) generate PSKc from \<passphrase\> (UTF-8 encoded) together with network name and extended PAN ID in the dataset buffer if set or values in the current stack if not, otherwise set PSKc as \<key\> (hex format).

```bash
> dataset pskc 67c0c203aa0b042bfb5381c47aef4d9e
Done
> dataset pskc -p 123456
Done
```

### securitypolicy

Usage: `dataset securitypolicy [<rotationtime> [onrcCepR] [versionthreshold]]`

Get security policy.

```bash
> dataset securitypolicy
672 onrc 0
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

If the `versionthreshold` parameter is not provided, a default value of zero is assumed.

```bash
> dataset securitypolicy 672 onrc 0
Done
```

### set

Usage: `dataset set <active|pending> <dataset>`

Set the Active Operational Dataset using hex-encoded TLVs.

```bash
> dataset set active 0e080000000000010000000300000f35060004001fffe0020839758ec8144b07fb0708fdf1f1add0797dc00510f366cec7a446bab978d90d27abe38f23030f4f70656e5468726561642d353933380102593804103ca67c969efb0d0c74a4d8ee923b576c0c0402a0f7f8
Done
```

Set the Pending Operational Dataset using hex-encoded TLVs.

```bash
> dataset set pending 0e0800000000000100003308000000000002000034040000b512000300001a35060004001fffe00208a74182f4d3f4de410708fd46c1b9e15955740510ed916e454d96fd00184f10a6f5c9e1d3030f4f70656e5468726561642d626666380102bff80410264f78414adc683191863d968f72d1b70c0402a0f7f8
Done
```

### tlvs

Usage: `dataset tlvs`

Convert the Operational Dataset to hex-encoded TLVs.

```bash
> dataset
Active Timestamp: 1
Channel: 22
Channel Mask: 0x07fff800
Ext PAN ID: d196fa2040e973b6
Mesh Local Prefix: fdbb:c310:c48f:3a39::/64
Network Key: 9929154dbc363218bcd22f907caf5c15
Network Name: OpenThread-de2b
PAN ID: 0xde2b
PSKc: 15b2c16f7ba92ed4bc7b1ee054f1553f
Security Policy: 672 onrc 0
Done

> dataset tlvs
0e080000000000010000000300001635060004001fffe00208d196fa2040e973b60708fdbbc310c48f3a3905109929154dbc363218bcd22f907caf5c15030f4f70656e5468726561642d646532620102de2b041015b2c16f7ba92ed4bc7b1ee054f1553f0c0402a0f7f8
Done
```

### updater

Usage: `dataset updater`

Requires `OPENTHREAD_CONFIG_DATASET_UPDATER_ENABLE`.

Indicate whether there is an ongoing Operation Dataset update request.

```bash
> dataset updater
Enabled
```

### updater start

Usage: `dataset updater start`

Requires `OPENTHREAD_CONFIG_DATASET_UPDATER_ENABLE`.

Request network to update its Operation Dataset to the current operational dataset buffer.

The current operational dataset buffer should contain the fields to be updated with their new values. It must not contain Active or Pending Timestamp fields. The Delay field is optional. If not provided, a default value (1000 ms) is used.

```bash
> channel
19
Done

> dataset clear
Done
> dataset channel 15
Done
> dataset
Channel: 15
Done

> dataset updater start
Done
> dataset updater
Enabled
Done

Dataset update complete: OK

> channel
15
Done
```

### updater cancel

Usage: `dataset updater cancel`

Requires `OPENTHREAD_CONFIG_DATASET_UPDATER_ENABLE`.

Cancels an ongoing (if any) Operational Dataset update request.

```bash
> dataset updater start
Done
> dataset updater
Enabled
Done
>
> dataset updater cancel
Done
> dataset updater
Disabled
Done
```

### wakeupchannel

Usage: `wakeupchannel [channel]`

Get wake-up channel.

```bash
> dataset wakeupchannel
13
Done
```

Set wake-up channel.

```bash
> dataset wakeupchannel 13
Done
```
