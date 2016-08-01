# OpenThread CLI Reference

The OpenThread CLI exposes configuration and management APIs via a
command line interface. Use the CLI to play with OpenThread, which 
can also be used with additional application code. The
OpenThread test scripts use the CLI to execute test cases.

## OpenThread Command List

* [channel](#channel)
* [child](#child)
* [childtimeout](#childtimeout)
* [contextreusedelay](#contextreusedelay)
* [counter](#counter)
* [eidcache](#eidcache)
* [extaddr](#extaddr)
* [extpanid](#extpanid)
* [ifconfig](#ifconfig)
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
* [router](#router)
* [routerupgradethreshold](#routerupgradethreshold)
* [scan](#scan)
* [state](#state)
* [thread](#thread)
* [version](#version)
* [whitelist](#whitelist)

## OpenThread Command Details

### channel

Get the IEEE 802.15.4 Channel value.

```bash
> channel
11
Done
```

### channel \<channel\>

Set the IEEE 802.15.4 Channel value.

```bash
> channel 11
Done
```

### child list

List attached Child IDs

```bash
> child list
1 2 3 6 7 8
Done
```

### child \<id\>

Print diagnostic information for an attached Thread Child.  The `id` may be a Child ID or an RLOC16.

```bash
> child 1
Child ID: 1
Rloc: 9c01
Ext Addr: e2b3540590b0fd87
Mode: rsn
Net Data: 184
Timeout: 100
Age: 0
LQI: 3
RSSI: -20
Done
```

### childtimeout

Get the Thread Child Timeout value.

```bash
> childtimeout
300
Done
```

### childtimeout \<timeout\>

Set the Thread Child Timeout value.

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

### counter

Get the supported counter names.

```bash
>counter
mac
Done
```

### counter \<countername\>

Get the counter value.

```bash
>counter mac
TxTotal: 10
    TxAckRequested: 4
    TxAcked: 4
    TxNoAckRequested: 6
    TxData: 10
    TxDataPoll: 0
    TxBeacon: 0
    TxBeaconRequest: 0
    TxOther: 0
    TxRetry: 0
    TxErrCca: 0
RxTotal: 11
    RxData: 11
    RxDataPoll: 0
    RxBeacon: 0
    RxBeaconRequest: 0
    RxOther: 0
    RxWhitelistFiltered: 0
    RxDestAddrFiltered: 0
    RxErrNoFrame: 0
    RxErrNoUnknownNeighbor: 0
    RxErrInvalidSrcAddr: 0
    RxErrSec: 0
    RxErrFcs: 0
    RxErrOther: 0
```

### eidcache

Print the EID-to-RLOC cache entries.

```bash
> eidcache
fdde:ad00:beef:0:bb1:ebd6:ad10:f33 ac00
fdde:ad00:beef:0:110a:e041:8399:17cd 6000
Done
```

### extaddr

Get the IEEE 802.15.4 Extended Address.

```bash
> extaddr
dead00beef00cafe
Done
```

### extaddr \<extaddr\>

Set the IEEE 802.15.4 Extended Address.

```bash
> extaddr dead00beef00cafe
dead00beef00cafe
Done
```

### extpanid

Get the Thread Extended PAN ID value.

```bash
> extpanid
dead00beef00cafe
Done
```

### extpanid \<extpanid\>

Set the Thread Extended PAN ID value.

```bash
> extpanid dead00beef00cafe
Done
```

### ifconfig up

Bring up the IPv6 interface.

```bash
> ifconfig up
Done
```

### ifconfig down

Bring down the IPv6 interface.

```bash
> ifconfig down
Done
```

### ipaddr

List all IPv6 addresses assigned to the Thread interface.

```bash
> ipaddr
fdde:ad00:beef:0:0:ff:fe00:0
fe80:0:0:0:0:ff:fe00:0
fdde:ad00:beef:0:558:f56b:d688:799
fe80:0:0:0:f3d9:2a82:c8d8:fe43
Done
```

### ipaddr add \<ipaddr\>

Add an IPv6 address to the Thread interface.

```bash
> ipaddr add 2001::dead:beef:cafe
Done
```

### ipaddr del \<ipaddr\>

Delete an IPv6 address from the Thread interface.

```bash
> ipaddr del 2001::dead:beef:cafe
Done
```

### keysequence

Get the Thread Key Sequence.

```bash
> keysequence
10
Done
```

### keysequence \<keysequence\>

Set the Thread Key Sequence.

```bash
> keysequence 10
Done
```

### leaderweight

Get the Thread Leader Weight.

```bash
> leaderweight
128
Done
```

### leaderweight \<weight\>

Set the Thread Leader Weight.

```bash
> leaderweight 128
Done
```

### masterkey

Get the Thread Master Key value.

```bash
> masterkey
00112233445566778899aabbccddeeff
Done
```

### masterkey \<key\>

Set the Thread Master Key value.

```bash
> masterkey 00112233445566778899aabbccddeeff
Done
```

### mode

Get the Thread Device Mode value.

* r: rx-on-when-idle
* s: Secure IEEE 802.15.4 data requests
* d: Full Function Device
* n: Full Network Data

```bash
> mode
rsdn
Done
```

### mode [rsdn]

Set the Thread Device Mode value.

* r: rx-on-when-idle
* s: Secure IEEE 802.15.4 data requests
* d: Full Function Device
* n: Full Network Data

```bash
> mode rsdn
Done
```

### netdataregister

Register local network data with Thread Leader.

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

Get the Thread Network Name.  

```bash
> networkname
OpenThread
Done
```

### networkname \<name\>

Set the Thread Network Name.  

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

### ping \<ipaddr\> [size] [count] [interval]

Send an ICMPv6 Echo Request.

```bash
> ping fdde:ad00:beef:0:558:f56b:d688:799
16 bytes from fdde:ad00:beef:0:558:f56b:d688:799: icmp_seq=1 hlim=64 time=28ms
```

### prefix add \<prefix\> [pvdcsr] [prf]

Add a valid prefix to the Network Data.

* p: Preferred flag
* a: Stateless IPv6 Address Autoconfiguration flag
* d: DHCPv6 IPv6 Address Configuration flag
* c: DHCPv6 Other Configuration flag
* r: Default Route flag
* o: On Mesh flag
* s: Stable flag
* prf: Default router preference, which may be 'high', 'med', or 'low'.

```bash
> prefix add 2001:dead:beef:cafe::/64 paros med
Done
```

### prefix remove \<prefix\>

Invalidate a prefix in the Network Data.

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

Get the Thread RLOC16 value.

```bash
> rloc16
0xdead
Done
```

### route add \<prefix\> [s] [prf]

Add a valid prefix to the Network Data.

* s: Stable flag
* prf: Default Router Preference, which may be: 'high', 'med', or 'low'.

```bash
> route add 2001:dead:beef:cafe::/64 s med
Done
```

### route remove \<prefix\>

Invalidate a prefix in the Network Data.

```bash
> route remove 2001:dead:beef:cafe::/64
Done
```

### router list

List allocated Router IDs

```bash
> router list
8 24 50
Done
```

### router \<id\>

Print diagnostic information for a Thread Router.  The `id` may be a Router ID or an RLOC16.

```bash
> router 50
Alloc: 1
Router ID: 50
Rloc: c800
Next Hop: c800
Link: 1
Ext Addr: e2b3540590b0fd87
Cost: 0
LQI In: 3
LQI Out: 3
Age: 3
Done
```

```bash
> router 0xc800
Alloc: 1
Router ID: 50
Rloc: c800
Next Hop: c800
Link: 1
Ext Addr: e2b3540590b0fd87
Cost: 0
LQI In: 3
LQI Out: 3
Age: 7
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

Perform an IEEE 802.15.4 Active Scan.

* channel: The channel to scan on.  If no channel is provided, the active scan will cover all valid channels.

```bash
> scan
| J | Network Name     | Extended PAN     | PAN  | MAC Address      | Ch | dBm |
+---+------------------+------------------+------+------------------+----+-----+
| 0 | OpenThread       | dead00beef00cafe | ffff | f1d92a82c8d8fe43 | 11 | -20 |
Done
```

### thread start

Enable Thread protocol operation and attach to a Thread network.

```bash
> thread start
Done
```

### thread stop

Disable Thread protocol operation and detach from a Thread network.

```bash
> thread stop
Done
```

### version

Print the build version information.

```bash
> version
OPENTHREAD/gf4f2f04; Jul  1 2016 17:00:09
Done
```

### whitelist

List the whitelist entries.

```bash
> whitelist
Enabled
e2b3540590b0fd87
d38d7f875888fccb
c467a90a2060fa0e
Done
```

### whitelist add \<extaddr\>

Add an IEEE 802.15.4 Extended Address to the whitelist.

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

Remove an IEEE 802.15.4 Extended Address from the whitelist.

```bash
> whitelist remove dead00beef00cafe
Done
```
