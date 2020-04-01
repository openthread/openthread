# OpenThread CLI - Operational Datasets

## Overview

Thread network configuration parameters are managed using Active and Pending Operational Dataset objects.

### Active Operational Dataset

The Active Operational Dataset includes parameters that are currently in use across an entire Thread network. The Active Operational Dataset contains:

* Active Timestamp
* Channel
* Channel Mask
* Extended PAN ID
* Mesh-Local Prefix
* Network Name
* PAN ID
* PSKc
* Security Policy

### Pending Operational Dataset

The Pending Operational Dataset is used to communicate changes to the Active Operational Dataset before they take effect. The Pending Operational Dataset contains all the parameters from the Active Operational Dataset, with the addition of:

* Delay Timer
* Pending Timestamp

## Quick Start

### Form Network

1. Generate and view new network configuration.

    ```bash
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

Only the Master Key is required for a device to attach to a Thread network.

While not required, specifying the channel avoids the need to search across multiple channels, improving both latency and efficiency of the attach process.

After the device successfully attaches to a Thread network, the device will retrieve the complete Active Operational Dataset.

1. Create a partial Active Operational Dataset.

    ```bash
    > dataset masterkey dfd34f0f05cad978ec4e32b0413038ff
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
- [masterkey](#masterkey)
- [meshlocalprefix](#meshlocalprefix)
- [mgmtgetcommand](#mgmtgetcommand)
- [mgmtsetcommand](#mgmtsetcommand)
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
masterkey
meshlocalprefix
mgmtgetcommand
mgmtsetcommand
networkname
panid
pending
pendingtimestamp
pskc
securitypolicy
Done
```

### active

Usage: `dataset active`

Print Active Operational Dataset.

```bash
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

### activetimestamp

Usage: `dataset activetimestamp <timestamp>`

Set active timestamp.

```bash
> dataset activetimestamp 123456789
Done
```

### channel

Usage: `channel <channel>`

Set channel.

```bash
> dataset channel 12
Done
```

### channelmask

Usage: `dataset channelmask <channelmask>`

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

Usage: `dataset delay <delay>`

Set delay timer value.

```bash
> dataset delay 1000
Done
```

### extpanid

Usage: `dataset extpanid <extpanid>`

Set extended panid.

```bash
> dataset extpanid 000db80123456789
Done
```

### init

Usage: `dataset init <active|new|pending>`

Initialize operational dataset buffer.

```bash
> dataset init new
Done
```

### masterkey

Usage: `dataset masterkey <key>`

Set master key.

```bash
> dataset masterkey 00112233445566778899aabbccddeeff
Done
```

### meshlocalprefix

Usage: `dataset meshlocalprefix <prefix>`

Set mesh local prefix.

```bash
> dataset meshlocalprefix fd00:db8::
Done
```

### mgmtgetcommand

Usage: `dataset mgmtgetcommand <active|pending> [address <destination>] [TLV list] [binary]`

Send MGMT_ACTIVE_GET or MGMT_PENDING_GET.

```bash
> dataset mgmtgetcommand active address fdde:ad00:beef:0:558:f56b:d688:799 activetimestamp binary 0c030001ff
Done
```

### mgmtsetcommand

Usage: `dataset mgmtsetcommand <active|pending> [TLV Type list] [binary]`

Send MGMT_ACTIVE_SET or MGMT_PENDING_SET.

```bash
> dataset mgmtsetcommand active activetimestamp 123 binary 0c030001ff
Done
```

### networkname

Usage: `dataset networkname <name>`

Set network name.

```bash
> dataset networkname OpenThread
Done
```

### panid

Usage: `dataset panid <panid>`

Set panid.

```bash
> dataset panid 0x1234
Done
```

### pending

Usage: `dataset pending`

Print Pending Operational Dataset.

```bash
> dataset pending
Pending Timestamp: 2
Active Timestamp: 15
Channel: 16
Channel Mask: 07fff800
Delay: 58706
Ext PAN ID: d63e8e3e495ebbc3
Mesh Local Prefix: fd3d:b50b:f96d:722d/64
Master Key: dfd34f0f05cad978ec4e32b0413038ff
Network Name: OpenThread-8f28
PAN ID: 0x8f28
PSKc: c23a76e98f1a6483639b1ac1271e2e27
Security Policy: 0, onrcb
Done
```

### pendingtimestamp

Usage: `dataset pendingtimestamp <timestamp>`

Set pending timestamp.

```bash
> dataset pendingtimestamp 123456789
Done
```

### pskc

Usage: `dataset pskc <key>`

Set pskc with hex format.

```bash
> dataset pskc 67c0c203aa0b042bfb5381c47aef4d9e
Done
```

### securitypolicy

Usage: `dataset securitypolicy <rotationtime> [onrcb]`

Set security policy.

* o: Obtaining the Master Key for out-of-band commissioning is enabled.
* n: Native Commissioning using PSKc is allowed.
* r: Thread 1.x Routers are enabled.
* c: External Commissioner authentication is allowed using PSKc.
* b: Thread 1.x Beacons are enabled.

```bash
> dataset securitypolicy 672 onrcb
Done
```
