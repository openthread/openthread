# OpenThread CLI Reference

The OpenThread CLI exposes configuration and management APIs via a
command line interface. Use the CLI to play with OpenThread, which 
can also be used with additional application code. The
OpenThread test scripts use the CLI to execute test cases.

## OpenThread Command List

* [channel](#channel)
* [blacklist](#blacklist)
* [child](#child)
* [childmax](#childmax)
* [childtimeout](#childtimeout)
* [contextreusedelay](#contextreusedelay)
* [counter](#counter)
* [dataset](#dataset)
* [discover](#discover)
* [eidcache](#eidcache)
* [extaddr](#extaddr)
* [extpanid](#extpanid)
* [ifconfig](#ifconfig)
* [ipaddr](#ipaddr)
* [keysequence](#keysequence)
* [leaderpartitionid](#leaderpartitionid)
* [leaderweight](#leaderweight)
* [linkquality](#linkquality)
* [masterkey](#masterkey)
* [mode](#mode)
* [netdataregister](#netdataregister)
* [networkidtimeout](#networkidtimeout)
* [networkname](#networkname)
* [panid](#panid)
* [parent](#parent)
* [ping](#ping)
* [pollperiod](#pollperiod)
* [prefix](#prefix)
* [promiscuous](#promiscuous)
* [releaserouterid](#releaserouterid)
* [reset](#reset)
* [rloc16](#rloc16)
* [route](#route)
* [router](#router)
* [routerdowngradethreshold](#routerdowngradethreshold)
* [routerrole](#routerrole)
* [routerupgradethreshold](#routerupgradethreshold)
* [scan](#scan)
* [singleton](#singleton)
* [state](#state)
* [thread](#thread)
* [version](#version)
* [whitelist](#whitelist)
* [diag](#diag)

## OpenThread Command Details

### blacklist

List the blacklist entries.

```bash
> blacklist
Enabled
166e0a0000000002
166e0a0000000003
Done
```

### blacklist add \<extaddr\>

Add an IEEE 802.15.4 Extended Address to the blacklist.

```bash
> blacklist add 166e0a0000000002
Done
```

### blacklist clear

Clear all entries from the blacklist.

```bash
> blacklist clear
Done
```

### blacklist disable

Disable MAC blacklist filtering.

```bash
> blacklist disable
Done
```

### blacklist enable

Enable MAC blacklist filtering.

```bash
> blacklist enable
Done
```

### blacklist remove \<extaddr\>

Remove an IEEE 802.15.4 Extended Address from the blacklist.

```bash
> blacklist remove 166e0a0000000002
Done
```
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

### childmax

Get the Thread maximum number of allowed children.

```bash
> childmax
5
Done
```

### childmax \<count\>

Set the Thread maximum number of allowed children.

```bash
> childmax 2
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

### dataset help

Print meshcop dataset help menu.

```bash
> dataset help
help
active
activetimestamp
channel
clear
commit
delay
extpanid
masterkey
meshlocalprefix
mgmtgetcommand
mgmtsetcommand
networkname
panid
pending
pendingtimestamp
userdata
Done
```

### dataset active

Print meshcop active operational dataset.

```bash
> dataset active
Active Timestamp: 0
Done
```

### dataset activetimestamp

Set getting active timestamp flag.

```bash
> dataset activestamp
Done
```

### dataset activetimestamp \[activetimestamp\]

Set active timestamp.

```bash
> dataset activestamp 123456789
Done
```

### dataset channel

Set getting channel flag.

```bash
> dataset channel
Done
```

### dataset channel \[channel\]

Set channel.

```bash
> dataset channel 12
Done
```

### dataset clear

Reset operational dataset buffer.

```bash
> dataset clear
Done
```

### dataset commit \[commit\]

Commit operational dataset buffer to active/pending operational dataset.

```bash
> dataset commit active
Done
```

### dataset delay

Set getting delay timer value flag.

```bash
> dataset delay
Done
```

### dataset delay \[delay\]

Set delay timer value.

```bash
> dataset delay 1000
Done
```

### dataset extpanid

Set getting extended panid flag.

```bash
> dataset extpanid
Done
```

### dataset extpanid \[extpanid\]

Set extended panid.

```bash
> dataset extpanid 000db80123456789
Done
```

### dataset masterkey

Set getting master key flag.

```bash
> dataset masterkey
Done
```

### dataset masterkey \[masterkey\]

Set master key.

```bash
> dataset master 1234567890123456
Done
```

### dataset meshlocalprefix

Set getting mesh local prefix flag.

```bash
> dataset meshlocalprefix
Done
```

### dataset meshlocalprefix fd00:db8::

Set mesh local prefix.

```bash
> dataset meshlocalprefix fd00:db8::
Done
```

### dataset mgmtgetcommand active \[TLVs list\] \[binary\]

Send MGMT_ACTIVE_GET.

```bash
> dataset mgmtgetcommand active activetimestamp 123 binary 0001
Done
```

### dataset mgmtsetcommand active \[TLV Types list\] \[binary\]

Send MGMT_ACTIVE_SET.

```bash
> dataset mgmtsetcommand active activetimestamp binary 820155
Done
```

### dataset mgmtgetcommand pending \[TLVs list\] \[binary\]

Send MGMT_PENDING_GET.

```bash
> dataset mgmtgetcommand pending activetimestamp binary 0001
Done
```

### dataset mgmtsetcommand pending \[TLV Types list\] \[binary\]

Send MGMT_PENDING_SET.

```bash
> dataset mgmtsetcommand pending activetimestamp binary 820155
Done
```

### dataset networkname

Set getting network name flag.

```bash
> dataset networkname
Done
```

### dataset networkname \[networkname\]

Set network name.

```bash
> dataset networkname openthread
Done
```

### dataset panid

Set getting panid flag.

```bash
> dataset panid
Done
```

### dataset panid \[panid\]

Set panid.

```bash
> dataset panid 0x1234
Done
```

### dataset pending

Print meshcop pending operational dataset.

```bash
> dataset pending
Done
```

### dataset pendingtimestamp

Set getting pending timestamp flag.

```bash
> dataset pendingtimestamp
Done
```

### dataset pendingtimestamp \[pendingtimestamp\]

Set pending timestamp.

```bash
> dataset pendingtimestamp 123456789
Done
```

### dataset userdata \[size\] \[data\]

Set user specific data for the command.

```bash
> dataset userdata 3 820155
Done
```

### discover \[channel\]

Perform an MLE Discovery operation.

* channel: The channel to discover on.  If no channel is provided, the discovery will cover all valid channels.

```bash
> discover
| J | Network Name     | Extended PAN     | PAN  | MAC Address      | Ch | dBm | LQI |
+---+------------------+------------------+------+------------------+----+-----+-----+
| 0 | OpenThread       | dead00beef00cafe | ffff | f1d92a82c8d8fe43 | 11 | -20 |   0 |
Done
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

### ifconfig

Show the status of the IPv6 interface.

```bash
> ifconfig
down
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

### leaderpartitionid

Get the Thread Leader Partition ID.

```bash
> leaderpartitionid
4294967295
Done
```

### leaderpartitionid \<partitionid>\

Set the Thread Leader Partition ID.

```bash
> leaderpartitionid 0xffffffff
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

### linkquality \<extaddr>\

Get the link quality on the link to a given extended address.

```bash
> linkquality 36c1dd7a4f5201ff
3
Done
```

### linkquality \<extaddr>\ \<linkquality>\

Set the link quality on the link to a given extended address.

```bash
> linkquality 36c1dd7a4f5201ff 3
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

### parent

Get the diagnostic information for a Thread Router as parent.

```bash
> parent
Ext Addr: be1857c6c21dce55
Rloc: 5c00
Done
```

### ping \<ipaddr\> [size] [count] [interval]

Send an ICMPv6 Echo Request.

```bash
> ping fdde:ad00:beef:0:558:f56b:d688:799
16 bytes from fdde:ad00:beef:0:558:f56b:d688:799: icmp_seq=1 hlim=64 time=28ms
```

### pollperiod

Get the customized data poll period of sleepy end device (seconds). Only for certification test

```bash
> pollperiod
0
Done
```

### pollperiod \<pollperiod>\

Set the customized data poll period for sleepy end device (seconds). Only for certification test

```bash
> pollperiod 10
Done
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

### promiscuous

Get radio promiscuous property.

```bash
> promiscuous
Disabled
Done
```

### promiscuous enable

Enable radio promiscuous operation and print raw packet content.

```bash
> promiscuous enable
Done
```

### promiscuous disable

Disable radio promiscuous operation.

```bash
> promiscuous disable
Done
```

### releaserouterid \<routerid\>
Release a Router ID that has been allocated by the device in the Leader role.

```bash
> releaserouterid 16
Done
```

### reset
Signal a platform reset.

```bash
> reset
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

### routerdowngradethreshold

Get the ROUTER_DOWNGRADE_THRESHOLD value.

```bash
> routerdowngradethreshold
23
Done
```

### routerdowngradethreshold \<threshold\>

Set the ROUTER_DOWNGRADE_THRESHOLD value.

```bash
> routerdowngradethreshold 23
Done
```

### routerrole

Indicates whether the router role is enabled or disabled.

```bash
> routerrole
Enabled
Done
```

### routerrole enable

Enable the router role.

```bash
> routerrole enable
Done
```

### routerrole disable

Disable the router role.

```bash
> routerrole disable
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
| J | Network Name     | Extended PAN     | PAN  | MAC Address      | Ch | dBm | LQI |
+---+------------------+------------------+------+------------------+----+-----+-----+
| 0 | OpenThread       | dead00beef00cafe | ffff | f1d92a82c8d8fe43 | 11 | -20 |   0 |
Done
```

### singleton
Return true when there are no other nodes in the network, otherwise return false.

```bash
> singleton
true or false
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

### diag

Diagnostics module is enabled only when building OpenThread with --enable-diag option.
Go [diagnostics module][1] for more information.

[1]:../diag/README.md

