# OpenThread CLI - Border Router (BR)

## Command List

Usage : `br [command] ...`

- [counters](#counters)
- [disable](#disable)
- [enable](#enable)
- [help](#help)
- [init](#init)
- [nat64prefix](#nat64prefix)
- [omrprefix](#omrprefix)
- [onlinkprefix](#onlinkprefix)
- [pd](#pd)
- [peers](#peers)
- [prefixtable](#prefixtable)
- [rioprf](#rioprf)
- [routeprf](#routeprf)
- [routers](#routers)
- [state](#state)

## Command Details

### help

Usage: `br help`

Print BR command help menu.

```bash
> br help
counters
disable
enable
omrprefix
onlinkprefix
pd
peers
prefixtable
raoptions
rioprf
routeprf
routers
state
Done
```

### init

Usage: `br init <interface> <enabled>`

Initializes the Border Routing Manager on given infrastructure interface.

```bash
> br init 2 1
Done
```

### enable

Usage: `br enable`

Enable the Border Routing functionality.

```bash
> br enable
Done
```

### disable

Usage: `br disable`

Disable the Border Routing functionality.

```bash
> br disable
Done
```

### state

Usage: `br state`

Get the Border Routing state:

- `uninitialized`: Routing Manager is uninitialized.
- `disabled`: Routing Manager is initialized but disabled.
- `stopped`: Routing Manager in initialized and enabled but currently stopped.
- `running`: Routing Manager is initialized, enabled, and running.

```bash
> br state
running
```

### counters

Usage : `br counters`

Get the Border Router counter.

```bash
> br counters
Inbound Unicast: Packets 4 Bytes 320
Inbound Multicast: Packets 0 Bytes 0
Outbound Unicast: Packets 2 Bytes 160
Outbound Multicast: Packets 0 Bytes 0
RA Rx: 4
RA TxSuccess: 2
RA TxFailed: 0
RS Rx: 0
RS TxSuccess: 2
RS TxFailed: 0
Done
```

### omrprefix

Usage: `br omrprefix [local|favored]`

Get local or favored or both off-mesh-routable prefixes of the Border Router.

```bash
> br omrprefix
Local: fdfc:1ff5:1512:5622::/64
Favored: fdfc:1ff5:1512:5622::/64 prf:low
Done

> br omrprefix favored
fdfc:1ff5:1512:5622::/64 prf:low
Done

> br omrprefix local
fdfc:1ff5:1512:5622::/64
Done
```

### onlinkprefix

Usage: `br onlinkprefix [local|favored]`

Get local or favored or both on-link prefixes of the Border Router.

```bash
> br onlinkprefix
Local: fd41:2650:a6f5:0::/64
Favored: 2600::0:1234:da12::/64
Done

> br onlinkprefix favored
2600::0:1234:da12::/64
Done

> br onlinkprefix local
fd41:2650:a6f5:0::/64
Done
```

### nat64prefix

Usage: `br nat64prefix [local|favored]`

Get local or favored or both NAT64 prefixes of the Border Router.

`OPENTHREAD_CONFIG_NAT64_BORDER_ROUTING_ENABLE` is required.

```bash
> br nat64prefix
Local: fd14:1078:b3d5:b0b0:0:0::/96
Favored: fd14:1078:b3d5:b0b0:0:0::/96 prf:low
Done

> br nat64prefix favored
fd14:1078:b3d5:b0b0:0:0::/96 prf:low
Done

> br nat64prefix
fd14:1078:b3d5:b0b0:0:0::/96
Done
```

### pd

Usage: `br pd [enable|disable]`

Enable/Disable the DHCPv6 PD.

```bash
> br pd enable
Done

> br pd disable
Done
```

Usage: `br pd state`

Get the state of DHCPv6 PD.

`OPENTHREAD_CONFIG_BORDER_ROUTING_DHCP6_PD_ENABLE` is required.

- `disabled`: DHCPv6 PD is disabled on the border router.
- `stopped`: DHCPv6 PD in enabled but won't try to request and publish a prefix.
- `running`: DHCPv6 PD is enabled and will try to request and publish a prefix.

```bash
> br pd state
running
Done
```

Usage `br pd omrprefix`

Get the DHCPv6 Prefix Delegation (PD) provided off-mesh-routable (OMR) prefix.

`OPENTHREAD_CONFIG_BORDER_ROUTING_DHCP6_PD_ENABLE` is required.

```bash
> br pd omrprefix
2001:db8:cafe:0:0/64 lifetime:1800 preferred:1800
Done
```

### peers

Usage: `br peers`

Get the list of peer BRs found in the Network Data.

`OPENTHREAD_CONFIG_BORDER_ROUTING_TRACK_PEER_BR_INFO_ENABLE` is required.

Peer BRs are other devices within the Thread mesh that provide external IP connectivity. A device is considered to provide external IP connectivity if at least one of the following conditions is met regarding its Network Data entries:

- It has added at least one external route entry.
- It has added at least one prefix entry with both the default-route and on-mesh flags set.
- It has added at least one domain prefix (with both the domain and on-mesh flags set).

The list of peer BRs specifically excludes the current device, even if it is itself acting as a BR.

Info per BR entry:

- RLOC16 of the BR
- Age as the duration interval since this BR appeared in Network Data. It is formatted as `{hh}:{mm}:{ss}` for hours, minutes, seconds, if the duration is less than 24 hours. If the duration is 24 hours or more, the format is `{dd}d.{hh}:{mm}:{ss}` for days, hours, minutes, seconds.

```bash
> br peers
rloc16:0x5c00 age:00:00:49
rloc16:0xf800 age:00:01:51
Done
```

Usage: `br peers count`

Gets the number of peer BRs found in the Network Data.

The count does not include the current device, even if it is itself acting as a BR.

The output indicates the minimum age among all peer BRs. Age is formatted as `{hh}:{mm}:{ss}` for hours, minutes, seconds, if the duration is less than 24 hours. If the duration is 24 hours or more, the format is `{dd}d.{hh}:{mm}:{ss}` for days, hours, minutes, seconds.

```bash
> br peer count
2 min-age:00:00:49
Done
```

### prefixtable

Usage: `br prefixtable`

Get the discovered prefixes by Border Routing Manager on the infrastructure link.

Info per prefix entry:

- The prefix
- Whether the prefix is on-link or route
- Milliseconds since last received Router Advertisement containing this prefix
- Prefix lifetime in seconds
- Preferred lifetime in seconds only if prefix is on-link
- Route preference (low, med, high) only if prefix is route (not on-link)
- The router IPv6 address which advertising this prefix
- Flags in received Router Advertisement header:
  - M: Managed Address Config flag
  - O: Other Config flag
  - S: SNAC Router flag

```bash
> br prefixtable
prefix:fd00:1234:5678:0::/64, on-link:no, ms-since-rx:29526, lifetime:1800, route-prf:med, router:ff02:0:0:0:0:0:0:1 (M:0 O:0 S:1)
prefix:1200:abba:baba:0::/64, on-link:yes, ms-since-rx:29527, lifetime:1800, preferred:1800, router:ff02:0:0:0:0:0:0:1 (M:0 O:0 S:1)
Done
```

### raoptions

Usage: `br raoptions <options>`

Sets additional options to append at the end of emitted Router Advertisement (RA) messages. `<options>` provided as hex bytes.

```bash
> br raoptions 0400ff00020001
Done
```

### raoptions clear

Usage: `br raoptions clear`

Clear any previously set additional options to append at the end of emitted Router Advertisement (RA) messages.

```bash
> br raoptions clear
Done
```

### rioprf

Usage: `br rioprf`

Get the preference used when advertising Route Info Options (e.g., for discovered OMR prefixes) in emitted Router Advertisement message.

```bash
> br rioprf
med
Done
```

### rioprf \<prf\>

Usage: `br rioprf high|med|low`

Set the preference (which may be 'high', 'med', or 'low') to use when advertising Route Info Options (e.g., for discovered OMR prefixes) in emitted Router Advertisement message.

```bash
> br rioprf low
Done
```

### rioprf clear

Usage: `br rioprf clear`

Clear a previously set preference value for advertising Route Info Options (e.g., for discovered OMR prefixes) in emitted Router Advertisement message. When cleared BR will use device's role to determine the RIO preference: Medium preference when in router/leader role and low preference when in child role.

```bash
> br rioprf clear
Done
```

### routeprf

Usage: `br routeprf`

Get the preference used for publishing routes in Thread Network Data. This may be the automatically determined route preference, or an administratively set fixed route preference - if applicable.

```bash
> br routeprf
med
Done
```

### routeprf \<prf\>

Usage: `br routeprf high|med|low`

Set the preference (which may be 'high', 'med', or 'low') to use publishing routes in Thread Network Data. Setting a preference value overrides the automatic route preference determination. It is used only for an explicit administrative configuration of a Border Router.

```bash
> br routeprf low
Done
```

### routeprf clear

Usage: `br routeprf clear`

Clear a previously set preference value for publishing routes in Thread Network Data. When cleared BR will automatically determine the route preference based on device's role and link quality to parent (when acting as end-device).

```bash
> br routeprf clear
Done
```

### routers

Usage: `br routers`

Get the list of discovered routers by Border Routing Manager on the infrastructure link.

Info per router:

- The router IPv6 address
- Flags in received Router Advertisement header:
  - M: Managed Address Config flag
  - O: Other Config flag
  - S: SNAC Router flag (indicates whether the router is a stub router)
- Milliseconds since last received message from this router
- Reachability flag: A router is marked as unreachable if it fails to respond to multiple Neighbor Solicitation probes.
- Age: Duration interval since this router was first discovered. It is formatted as `{hh}:{mm}:{ss}` for hours, minutes, seconds, if the duration is less than 24 hours. If the duration is 24 hours or more, the format is `{dd}d.{hh}:{mm}:{ss}` for days, hours, minutes, seconds.
- `(this BR)` is appended when the router is the local device itself.
- `(peer BR)` is appended when the router is likely a peer BR connected to the same Thread mesh. This requires `OPENTHREAD_CONFIG_BORDER_ROUTING_TRACK_PEER_BR_INFO_ENABLE`.

```bash
> br routers
ff02:0:0:0:0:0:0:1 (M:0 O:0 S:1) ms-since-rx:1505 reachable:yes age:00:18:13
Done
```
