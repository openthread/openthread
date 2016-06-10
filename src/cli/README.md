# OpenThread CLI reference

The OpenThread CLI exposes configuration and management APIs via a
command line interface. Use the CLI to play with OpenThread, which 
can also be used with additional application code. The
OpenThread test scripts use the CLI to execute test cases.

## OpenThread command list

* [channel](#channel)
* [childtimeout](#childtimeout)
* [contextreusedelay](#contextreusedelay)
* [extaddr](#extaddr)
* [extpanid](#extpanid)
* [ipaddr](#ipaddr)
* [keysequence](#keysequence)
* [leaderweight](#leaderweight)
* [masterkey](#masterkey)
* [mode](#mode)
* [netdataregister](#netdataregister)
* [networkidtimeout](#networkidtimeout)
* [networkname](#networkname)
* [panid](#panid)
* [ping](#ping)
* [prefix](#prefix)
* [releaserouterid](#releaserouterid)
* [rloc16](#rloc16)
* [route](#route)
* [routerupgradethreshold](#routerupgradethreshold)
* [scan](#scan)
* [start](#start)
* [state](#state)
* [stop](#stop)
* [whitelist](#whitelist)

## OpenThread command details

### channel

Get the IEEE 802.15.4 channel value.

```bash
> channel
11
Done
```

### channel \<channel\>

Set the IEEE 802.15.4 channel value.

```bash
> channel 11
Done
```

### childtimeout

Get the OpenThread child timeout value.

```bash
> childtimeout
300
Done
```

### childtimeout \<timeout\>

Set the OpenThread child timeout value.

```bash
> childtimeout 300
Done
```

### contextreusedelay

Get the CONTEXT_ID_REUSE_DELAY value.

```bash
> contextreusedelay
11
Done
```

### contextreusedelay \<delay\>

Set the CONTEXT_ID_REUSE_DELAY value.

```bash
> contextreusedelay 11
Done
```

### extaddr

Get the IEEE 802.15.4 extended address.

```bash
> extaddr
0xdead00beef00cafe
Done
```

### extpanid

Get the OpenThread extended PAN ID value.

```bash
> extpanid
0xdead00beef00cafe
Done
```

### extpanid \<extpanid\>

Set the OpenThread extended PAN ID value.

```bash
> extpanid dead00beef00cafe
Done
```

### ipaddr

List all IPv6 addresses assigned to the OpenThread interface.

```bash
> ipaddr
fdde:ad00:beef:0:0:ff:fe00:0
fe80:0:0:0:0:ff:fe00:0
fdde:ad00:beef:0:558:f56b:d688:799
fe80:0:0:0:f3d9:2a82:c8d8:fe43
Done
```

### ipaddr add \<ipaddr\>

Add an IPv6 address to the OpenThread interface.

```bash
> ipaddr add 2001::dead:beef:cafe
Done
```

### ipaddr del \<ipaddr\>

Delete an IPv6 address from the OpenThread interface.

```bash
> ipaddr del 2001::dead:beef:cafe
Done
```

### keysequence

Get the OpenThread key sequence.

```bash
> keysequence
10
Done
```

### keysequence \<keysequence\>

Set the OpenThread key sequence.

```bash
> keysequence 10
Done
```

### leaderweight

Get the OpenThread leader weight.

```bash
> leaderweight
128
Done
```

### leaderweight \<weight\>

Set the OpenThread leader weight.

```bash
> leaderweight 128
Done
```

### masterkey

Get the OpenThread master key value.

```bash
> masterkey
00112233445566778899aabbccddeeff
Done
```

### masterkey \<key\>

Set the OpenThread master key value.

```bash
> masterkey 00112233445566778899aabbccddeeff
Done
```

### mode

Get the OpenThread device mode value.

* r: rx-on-when-idle
* s: Secure IEEE 802.15.4 data requests
* d: Full function device
* n: Full network data

```bash
> mode
rsdn
Done
```

### mode [rsdn]

Set the OpenThread device mode value.

* r: rx-on-when-idle
* s: Secure IEEE 802.15.4 data requests
* d: Full function device
* n: Full network data

```bash
> mode rsdn
Done
```

### netdataregister

Register local network data with OpenThread leader.

```bash
> netdataregister
Done
```

### networkidtimeout

Get the NETWORK_ID_TIMEOUT parameter used in the Router role.  

```bash
> networkidtimeout
120
Done
```

### networkidtimeout \<timeout\>

Set the NETWORK_ID_TIMEOUT parameter used in the Router role.  

```bash
> networkidtimeout 120
Done
```

### networkname

Get the OpenThread network name.  

```bash
> networkname
OpenThread
Done
```

### networkname \<name\>

Set the OpenThread network name.  

```bash
> networkname OpenThread
Done
```

### panid

Get the IEEE 802.15.4 PAN ID value.

```bash
> panid
0xdead
Done
```

### panid \<panid\>

Set the IEEE 802.15.4 PAN ID value.

```bash
> panid 0xdead
Done
```

### ping \<ipaddr\> [size]

Send an ICMPv6 echo request.

```bash
> ping fdde:ad00:beef:0:558:f56b:d688:799
16 bytes from fdde:ad00:beef:0:558:f56b:d688:799: icmp_seq=1 hlim=64
```

### prefix add \<prefix\> [pvdcsr] [prf]

Add a valid prefix to the network data.

* p: Stateless IPv6 address autoconfiguration preferred flag
* v: Stateless IPv6 address autoconfiguration valid flag
* d: DHCPv6 IPv6 address configuration flag
* c: DHCPv6 other configuration flag
* s: Stable flag
* r: Default route flag
* prf: Default router preference, which can be 'high', 'med', or 'low'.

```bash
> prefix add 2001:dead:beef:cafe::/64 pvsr 0
Done
```

### prefix remove \<prefix\>

Invalidate a prefix in the network data.

```bash
> prefix remove 2001:dead:beef:cafe::/64
Done
```

### releaserouterid \<routerid\>
Release a Router ID that has been allocated by the device in the Leader role.
```bash
> releaserouterid 16
Done
```

### rloc16

Get the OpenThread RLOC16 value.

```bash
> rloc16
0xdead
Done
```

### route add \<prefix\> [s] [prf]

Add a valid prefix to the network data.

* s: Stable flag
* prf: Default router preference, which may be: 'high', 'med', or 'low'.

```bash
> route add 2001:dead:beef:cafe::/64 pvsr 0
Done
```

### route remove \<prefix\>

Invalidate a prefix in the network data.

```bash
> route remove 2001:dead:beef:cafe::/64
Done
```

### routerupgradethreshold

Get the ROUTER_UPGRADE_THRESHOLD value.

```bash
> routerupgradethreshold
16
Done
```

### routerupgradethreshold \<threshold\>

Set the ROUTER_UPGRADE_THRESHOLD value.

```bash
> routerupgradethreshold 16
Done
```

### scan \[channel\]

Perform an IEEE 802.15.4 active scan.

* channel: The channel to scan on.  If no channel is provided, the active scan will cover all valid channels.

```bash
> scan
| J | Network Name     | Extended PAN     | PAN  | MAC Address      | Ch | dBm |
+---+------------------+------------------+------+------------------+----+-----+
| 0 | OpenThread       | dead00beef00cafe | ffff | f1d92a82c8d8fe43 | 11 | -20 |
Done
```

### start

Enable OpenThread.

```bash
> start
Done
```

### stop

Disable OpenThread.

```bash
> stop
Done
```

### whitelist add \<extaddr\>

Add an IEEE 802.15.4 extended address to the whitelist.

```bash
> whitelist add dead00beef00cafe
Done
```

### whitelist clear

Clear all entries from the whitelist.

```bash
> whitelist clear
Done
```

### whitelist disable

Disable MAC whitelist filtering.

```bash
> whitelist disable
Done
```

### whitelist enable

Enable MAC whitelist filtering.

```bash
> whitelist enable
Done
```

### whitelist remove \<extaddr\>

Remove an IEEE 802.15.4 extended address from the whitelist.

```bash
> whitelist remove dead00beef00cafe
Done
```
