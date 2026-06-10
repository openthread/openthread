# OpenThread CLI - Commissioner

## Overview

The Commissioner is an entity that can add new Thread devices securely to a Thread network. It also can manage a Thread network by changing network configuration parameters ([Operational Datasets](README_DATASET.md)) or sending specific management/diagnostic commands to selected Thread devices. Before a Commissioner can do its tasks, it has to petition to the Leader to get permission to become the Commissioner.

## Quick Start

See [README_COMMISSIONING.md](README_COMMISSIONING.md).

## Command List

- [help](#help)
- [announce](#announce)
- [energy](#energy)
- [joiner add](#joiner-add)
- [joiner remove](#joiner-remove)
- [joiner table](#joiner-table)
- [mgmtget](#mgmtget)
- [mgmtset](#mgmtset)
- [panid](#panid)
- [provisioningurl](#provisioningurl)
- [sessionid](#sessionid)
- [start](#start)
- [state](#state)
- [stop](#stop)

## Command Details

### help

Usage: `commissioner help`

Print commissioner help menu.

```bash
> commissioner help
help
announce
energy
joiner
mgmtget
mgmtset
panid
provisioningurl
sessionid
start
stop
Done
```

### announce

Usage: `commissioner announce <mask> <count> <period> <destination>`

Send a `MGMT_ANNOUNCE_BEGIN` message.

- mask: Bitmask identifying channels to send MLE Announce messages.
- count: Number of MLE Announce transmissions per channel.
- period: Period between successive MLE Announce transmissions (milliseconds).
- destination: IPv6 destination for the message (may be multicast).

```bash
> commissioner announce 0x00050000 2 32 fdde:ad00:beef:0:0:ff:fe00:c00
Done
```

### energy

Usage: `commissioner energy <mask> <count> <period> <scanDuration> <destination>`

Send a `MGMT_ED_SCAN` message.

- mask: Bitmask identifying channels to perform IEEE 802.15.4 ED Scans.
- count: Number of IEEE 802.15.4 ED Scans per channel.
- period: Period between successive IEEE 802.15.4 ED Scans (milliseconds).
- scanDuration: IEEE 802.15.4 ScanDuration to use when performing an IEEE 802.15.4 ED Scan (milliseconds).
- destination: IPv6 destination for the message (may be multicast).

The contents of `MGMT_ED_REPORT` messages (i.e. Channel Mask and Energy List) are printed as they are received.

```bash
> commissioner energy 0x00050000 2 32 1000 fdde:ad00:beef:0:0:ff:fe00:c00
Done
Energy: 00050000 0 0 0 0
```

### joiner add

Usage: `commissioner joiner add <eui64>|<discerner> <pskd> [timeout]`

Add a Joiner entry.

- eui64: The IEEE EUI-64 of the Joiner or '\*' to match any Joiner.
- discerner: The Joiner discerner in format `number/length`.
- pskd: Pre-Shared Key for the Joiner.
- timeout: joiner timeout in seconds.

```bash
> commissioner joiner add d45e64fa83f81cf7 J01NME
Done
```

```bash
> commissioner joiner add 0xabc/12 J01NME
Done
```

### joiner remove

Usage: `commissioner joiner remove <eui64>|<discerner>`

Remove a Joiner entry.

- eui64: The IEEE EUI-64 of the Joiner or '\*' to match any Joiner.
- discerner: The Joiner discerner in format `number/length`.

```bash
> commissioner joiner remove d45e64fa83f81cf7
Done
```

```bash
> commissioner joiner remove 0xabc/12
Done
```

### joiner table

Usage: `commissioner joiner table`

List all Joiner entries.

```bash
> commissioner joiner table
| ID                    | PSKd                             | Expiration |
+-----------------------+----------------------------------+------------+
|                     * |                           J01NME |      81015 |
|      d45e64fa83f81cf7 |                           J01NME |     101204 |
| 0x0000000000000abc/12 |                           J01NME |     114360 |
Done
```

### mgmtget

Usage: `commissioner mgmtget [locator] [sessionid] [steeringdata] [joinerudpport] [-x <TLV Types>]`

Send a `MGMT_GET` message to the Leader.

```bash
> commissioner mgmtget locator sessionid
Done
```

### mgmtset

Usage: `commissioner mgmtset [locator <locator>] [sessionid <sessionid>] [steeringdata <steeringdata>] [joinerudpport <joinerudpport>] [-x <TLVs>]`

Send a `MGMT_SET` message to the Leader.

```bash
> commissioner mgmtset joinerudpport 9988
Done
```

### panid

Usage: `commissioner panid <panid> <mask> <destination>`

Send a `MGMT_PANID_QUERY` message.

- panid: PAN ID to check for conflicts.
- mask: Bitmask identifying channels to perform IEEE 802.15.4 Active Scans.
- destination: IPv6 destination for the message (may be multicast).

The contents of `MGMT_PANID_CONFLICT` messages (i.e. PAN ID and Channel Mask) are printed as they are received.

```bash
> commissioner panid 0xdead 0x7fff800 fdde:ad00:beef:0:0:ff:fe00:c00
Done
Conflict: dead, 00000800
```

### provisioningurl

Usage: `commissioner provisioningurl <provisioningurl>`

Set the Provisioning URL.

```bash
> commissioner provisioningurl http://github.com/openthread/openthread
Done
```

### sessionid

Usage: `commissioner sessionid`

Get current commissioner session id.

```bash
> commissioner sessionid
0
Done
```

### id

Usage: `commissioner id`

Get the commissioner id.

```bash
> commissioner id
OpenThread Commissioner
Done
```

### id \<name\>

Set the commissioner id.

```bash
> commissioner id "Custom Commissioner Id"
Done
```

### start

Usage: `commissioner start`

Start the Commissioner role.

This command will cause the device to send `LEAD_PET` and `LEAD_KA` messages.

```bash
> commissioner start
Commissioner: petitioning
Done
Commissioner: active
```

### state

Usage: `commissioner state`

Get Commissioner state.

This command will return the current Commissioner state.

```bash
> commissioner state
active
Done
```

### stop

Usage: `commissioner stop`

Stop the Commissioner role.

This command will cause the device to send `LEAD_KA[Reject]` messages.

```bash
> commissioner stop
Done
```
