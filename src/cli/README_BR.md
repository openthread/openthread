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
- [prefixtable](#prefixtable)
- [rioprf](#rioprf)
- [routeprf](#routeprf)
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
prefixtable
rioprf
routeprf
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

### prefixtable

Usage: `br prefixtable`

Get the discovered prefixes by Border Routing Manager on the infrastructure link.

```bash
> br prefixtable
prefix:fd00:1234:5678:0::/64, on-link:no, ms-since-rx:29526, lifetime:1800, route-prf:med, router:ff02:0:0:0:0:0:0:1
prefix:1200:abba:baba:0::/64, on-link:yes, ms-since-rx:29527, lifetime:1800, preferred:1800, router:ff02:0:0:0:0:0:0:1
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
