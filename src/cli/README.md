# OpenThread CLI Reference

The OpenThread CLI exposes configuration and management APIs via a command line interface. Use the CLI to play with OpenThread, which can also be used with additional application code. The OpenThread test scripts use the CLI to execute test cases.

## Separator and escaping characters

The whitespace character (`' '`) is used to delimit the command name and the different arguments, together with tab (`'\t'`) and new line characters (`'\r'`, `'\n'`).

Some arguments might require to accept whitespaces on them. For those cases the backslash character (`'\'`) can be used to escape separators or the backslash itself.

Example:

```bash
> networkname Test\ Network
Done
> networkname
Test Network
Done
>
```

## OpenThread Command List

- [ba](#ba)
- [bbr](#bbr)
- [br](#br)
- [bufferinfo](#bufferinfo)
- [ccathreshold](#ccathreshold)
- [channel](#channel)
- [child](#child-list)
- [childip](#childip)
- [childmax](#childmax)
- [childsupervision](#childsupervision-interval)
- [childtimeout](#childtimeout)
- [coap](README_COAP.md)
- [coaps](README_COAPS.md)
- [commissioner](README_COMMISSIONER.md)
- [contextreusedelay](#contextreusedelay)
- [counters](#counters)
- [csl](#csl)
- [dataset](README_DATASET.md)
- [delaytimermin](#delaytimermin)
- [diag](#diag)
- [discover](#discover-channel)
- [dns](#dns-config)
- [domainname](#domainname)
- [dua](#dua-iid)
- [eidcache](#eidcache)
- [eui64](#eui64)
- [extaddr](#extaddr)
- [extpanid](#extpanid)
- [factoryreset](#factoryreset)
- [fake](#fake)
- [fem](#fem)
- [history](README_HISTORY.md)
- [ifconfig](#ifconfig)
- [ipaddr](#ipaddr)
- [ipmaddr](#ipmaddr)
- [joiner](README_JOINER.md)
- [joinerport](#joinerport-port)
- [keysequence](#keysequence-counter)
- [leaderdata](#leaderdata)
- [leaderweight](#leaderweight)
- [linkmetrics](#linkmetrics-mgmt-ipaddr-enhanced-ack-clear)
- [linkquality](#linkquality-extaddr)
- [locate](#locate)
- [log](#log-filename-filename)
- [mac](#mac-retries-direct)
- [macfilter](#macfilter)
- [mliid](#mliid-iid)
- [mlr](#mlr-reg-ipaddr--timeout)
- [mode](#mode)
- [multiradio](#multiradio)
- [neighbor](#neighbor-list)
- [netdata](README_NETDATA.md)
- [netstat](#netstat)
- [networkdiagnostic](#networkdiagnostic-get-addr-type-)
- [networkidtimeout](#networkidtimeout)
- [networkkey](#networkkey)
- [networkname](#networkname)
- [networktime](#networktime)
- [panid](#panid)
- [parent](#parent)
- [parentpriority](#parentpriority)
- [partitionid](#partitionid)
- [ping](#ping--i-source-ipaddr-size-count-interval-hoplimit-timeout)
- [pollperiod](#pollperiod-pollperiod)
- [preferrouterid](#preferrouterid-routerid)
- [prefix](#prefix)
- [promiscuous](#promiscuous)
- [pskc](#pskc--p-keypassphrase)
- [radiofilter](#radiofilter)
- [rcp](#rcp)
- [region](#region)
- [releaserouterid](#releaserouterid-routerid)
- [reset](#reset)
- [rloc16](#rloc16)
- [route](#route)
- [router](#router-list)
- [routerdowngradethreshold](#routerdowngradethreshold)
- [routereligible](#routereligible)
- [routerselectionjitter](#routerselectionjitter)
- [routerupgradethreshold](#routerupgradethreshold)
- [scan](#scan-channel)
- [service](#service)
- [singleton](#singleton)
- [sntp](#sntp-query-sntp-server-ip-sntp-server-port)
- [state](#state)
- [srp](README_SRP.md)
- [tcp](README_TCP.md)
- [thread](#thread-start)
- [trel](#trel)
- [tvcheck](#tvcheck-enable)
- [txpower](#txpower)
- [udp](README_UDP.md)
- [unsecureport](#unsecureport-add-port)
- [uptime](#uptime)
- [version](#version)

## OpenThread Command Details

### bbr

Show current Primary Backbone Router information for Thread 1.2 device.

```bash
> bbr
BBR Primary:
server16: 0xE400
seqno:    10
delay:    120 secs
timeout:  300 secs
Done
```

```bash
> bbr
BBR Primary: None
Done
```

### bbr mgmt dua \<status\|coap-code\> [meshLocalIid]

Configure the response status for DUA.req with meshLocalIid in payload. Without meshLocalIid, simply respond any coming DUA.req next with the specified status or COAP code.

Only for testing/reference device.

known status value:

- 0: ST_DUA_SUCCESS
- 1: ST_DUA_REREGISTER
- 2: ST_DUA_INVALID
- 3: ST_DUA_DUPLICATE
- 4: ST_DUA_NO_RESOURCES
- 5: ST_DUA_BBR_NOT_PRIMARY
- 6: ST_DUA_GENERAL_FAILURE
- 160: COAP code 5.00

```bash
> bbr mgmt dua 1 2f7c235e5025a2fd
Done
> bbr mgmt dua 160
Done
```

### bbr mgmt mlr listener

Show the Multicast Listeners.

Only for testing/reference Backbone Router device.

```bash
> bbr mgmt mlr listener
ff04:0:0:0:0:0:0:abcd 3534000
ff04:0:0:0:0:0:0:eeee 3537610
Done
```

### bbr mgmt mlr listener add \<ipaddr\> \[\<timeout\>\]

Add a Multicast Listener with a given IPv6 multicast address and timeout (in seconds).

Only for testing/reference Backbone Router device.

```bash
> bbr mgmt mlr listener add ff04::1
Done
> bbr mgmt mlr listener add ff04::2 300
Done
> bbr mgmt mlr listener
ff04:0:0:0:0:0:0:2 261
ff04:0:0:0:0:0:0:1 3522
Done
```

### bbr mgmt mlr listener clear

Removes all the Multicast Listeners.

Only for testing/reference Backbone Router device.

```bash
> bbr mgmt mlr listener clear
Done
> bbr mgmt mlr listener
Done
```

### bbr mgmt mlr response \<status\>

Configure the response status for the next MLR.req.

Only for testing/reference device.

Known status values:

- 0: ST_MLR_SUCCESS
- 2: ST_MLR_INVALID
- 3: ST_MLR_NO_PERSISTENT
- 4: ST_MLR_NO_RESOURCES
- 5: ST_MLR_BBR_NOT_PRIMARY
- 6: ST_MLR_GENERAL_FAILURE

```bash
> bbr mgmt mlr response 2
Done
```

### bbr state

Show local Backbone state ([`Disabled`,`Primary`, `Secondary`]) for Thread 1.2 FTD.

`OPENTHREAD_CONFIG_BACKBONE_ROUTER_ENABLE` is required.

```bash
> bbr state
Disabled
Done

> bbr state
Primary
Done

> bbr state
Secondary
Done
```

### bbr enable

Enable Backbone Router Service for Thread 1.2 FTD. `SRV_DATA.ntf` would be triggerred for attached device if there is no Backbone Router Service in Thread Network Data.

`OPENTHREAD_CONFIG_BACKBONE_ROUTER_ENABLE` is required.

```bash
> bbr enable
Done
```

### bbr disable

Disable Backbone Router Service for Thread 1.2 FTD. `SRV_DATA.ntf` would be triggerred if Backbone Router is Primary state. o `OPENTHREAD_CONFIG_BACKBONE_ROUTER_ENABLE` is required.

```bash
> bbr disable
Done
```

### bbr register

Register Backbone Router Service for Thread 1.2 FTD. `SRV_DATA.ntf` would be triggerred for attached device.

`OPENTHREAD_CONFIG_BACKBONE_ROUTER_ENABLE` is required.

```bash
> bbr register
Done
```

### bbr config

Show local Backbone Router configuration for Thread 1.2 FTD.

`OPENTHREAD_CONFIG_BACKBONE_ROUTER_ENABLE` is required.

```bash
> bbr config
seqno:    10
delay:    120 secs
timeout:  300 secs
Done
```

### bbr config \[seqno \<seqno\>\] \[delay \<delay\>\] \[timeout \<timeout\>\]

Configure local Backbone Router configuration for Thread 1.2 FTD. `bbr register` should be issued explicitly to register Backbone Router service to Leader for Secondary Backbone Router. `SRV_DATA.ntf` would be initiated automatically if BBR Dataset changes for Primary Backbone Router.

`OPENTHREAD_CONFIG_BACKBONE_ROUTER_ENABLE` is required.

```bash
> bbr config seqno 20 delay 30
Done
```

### bbr jitter

Show jitter (in seconds) for Backbone Router registration for Thread 1.2 FTD.

`OPENTHREAD_CONFIG_BACKBONE_ROUTER_ENABLE` is required.

```bash
> bbr jitter
20
Done
```

### bbr jitter \<jitter\>

Set jitter (in seconds) for Backbone Router registration for Thread 1.2 FTD.

`OPENTHREAD_CONFIG_BACKBONE_ROUTER_ENABLE` is required.

```bash
> bbr jitter 10
Done
```

### ba

Show current Border Agent information.

### ba port

Print border agent service port.

```bash
> ba port
49152
Done
```

### ba state

Print border agent state.

```bash
> ba state
Started
Done
```

### br

Enbale/disable the Border Routing functionality.

```bash
> br enable
Done
```

```bash
> br disable
Done
```

### br omrprefix

Get the randomly generated off-mesh-routable prefix of the Border Router.

```bash
> br omrprefix
fdfc:1ff5:1512:5622::/64
Done
```

### br onlinkprefix

Get the randomly generated on-link prefix of the Border Router.

```bash
> br onlinkprefix
fd41:2650:a6f5:0::/64
Done
```

### br nat64prefix

Get the local NAT64 prefix of the Border Router.

`OPENTHREAD_CONFIG_BORDER_ROUTING_NAT64_ENABLE` is required.

```bash
> br nat64prefix
fd14:1078:b3d5:b0b0:0:0::/96
Done
```

### bufferinfo

Show the current message buffer information.

- The `total` shows total number of message buffers in pool.
- The `free` shows the number of free message buffers.
- This is then followed by info about different queues used by OpenThread stack, each line representing info about a queue.
  - The first number shows number messages in the queue.
  - The second number shows number of buffers used by all messages in the queue.
  - The third number shows total number of bytes of all messages in the queue.

```bash
> bufferinfo
total: 40
free: 40
6lo send: 0 0 0
6lo reas: 0 0 0
ip6: 0 0 0
mpl: 0 0 0
mle: 0 0 0
coap: 0 0 0
coap secure: 0 0 0
application coap: 0 0 0
Done
```

### ccathreshold

Get the CCA threshold in dBm measured at antenna connector per IEEE 802.15.4 - 2015 section 10.1.4.

```bash
> ccathreshold
-75 dBm
Done
```

### ccathreshold \<ccathreshold\>

Set the CCA threshold measured at antenna connector per IEEE 802.15.4 - 2015 section 10.1.4.

```bash
> ccathreshold -62
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

### channel manager

Get channel manager state.

`OPENTHREAD_CONFIG_CHANNEL_MANAGER_ENABLE` is required.

```bash
channel: 11
auto: 1
delay: 120
interval: 10800
supported: { 11-26}
favored: { 11-26}
Done
```

### channel manager change \<channel\>

Initiate a channel change with the channel manager.

`OPENTHREAD_CONFIG_CHANNEL_MANAGER_ENABLE` is required.

```bash
> channel manager change 11
channel manager change 11
Done
```

### channel manager select \<skip quality check (boolean)\>

Request a channel selection with the channel manager.

`OPENTHREAD_CONFIG_CHANNEL_MANAGER_ENABLE` and `OPENTHREAD_CONFIG_CHANNEL_MONITOR_ENABLE` are required.

```bash
> channel manager select 1
channel manager select 1
Done
```

### channel manager auto \<enable (boolean)\>

Enable/disable the auto-channel-selection functionality.

`OPENTHREAD_CONFIG_CHANNEL_MANAGER_ENABLE` and `OPENTHREAD_CONFIG_CHANNEL_MONITOR_ENABLE` are required.

```bash
> channel manager auto 1
channel manager auto 1
Done
```

### channel manager delay \<delay\>

Set the channel change delay (in seconds).

`OPENTHREAD_CONFIG_CHANNEL_MANAGER_ENABLE` and `OPENTHREAD_CONFIG_CHANNEL_MONITOR_ENABLE` are required.

```bash
> channel manager delay 120
channel manager delay 120
Done
```

### channel manager interval \<interval\>

Set the auto-channel-selection interval (in seconds).

`OPENTHREAD_CONFIG_CHANNEL_MANAGER_ENABLE` and `OPENTHREAD_CONFIG_CHANNEL_MONITOR_ENABLE` are required.

```bash
> channel manager interval 10800
channel manager interval 10800
Done
```

### channel manager supported \<mask\>

Set the supported channel mask for the auto-channel-selection.

`OPENTHREAD_CONFIG_CHANNEL_MANAGER_ENABLE` and `OPENTHREAD_CONFIG_CHANNEL_MONITOR_ENABLE` are required.

```bash
> channel manager supported 0x7fffc00
channel manager supported 0x7fffc00
Done
```

### channel manager favored \<mask\>

Set the favored channel mask for the auto-channel-selection.

`OPENTHREAD_CONFIG_CHANNEL_MANAGER_ENABLE` and `OPENTHREAD_CONFIG_CHANNEL_MONITOR_ENABLE` are required.

```bash
> channel manager favored 0x7fffc00
channel manager favored 0x7fffc00
Done
```

### channel monitor

Get current channel monitor state and channel occupancy.

`OPENTHREAD_CONFIG_CHANNEL_MONITOR_ENABLE` is required.

```bash
> channel monitor
channel monitor
enabled: 1
interval: 41000
threshold: -75
window: 960
count: 10552
occupancies:
ch 11 (0x0cb7)  4.96% busy
ch 12 (0x2e2b) 18.03% busy
ch 13 (0x2f54) 18.48% busy
ch 14 (0x0fef)  6.22% busy
ch 15 (0x1536)  8.28% busy
ch 16 (0x1746)  9.09% busy
ch 17 (0x0b8b)  4.50% busy
ch 18 (0x60a7) 37.75% busy
ch 19 (0x0810)  3.14% busy
ch 20 (0x0c2a)  4.75% busy
ch 21 (0x08dc)  3.46% busy
ch 22 (0x101d)  6.29% busy
ch 23 (0x0092)  0.22% busy
ch 24 (0x0028)  0.06% busy
ch 25 (0x0063)  0.15% busy
ch 26 (0x058c)  2.16% busy

Done
```

### channel monitor start

Start the channel monitor.

`OPENTHREAD_CONFIG_CHANNEL_MONITOR_ENABLE` is required.

```bash
> channel monitor start
channel monitor start
Done
```

### channel monitor stop

Stop the channel monitor.

`OPENTHREAD_CONFIG_CHANNEL_MONITOR_ENABLE` is required.

```bash
> channel monitor stop
channel monitor stop
Done
```

### channel preferred

Get preferred channel mask.

```bash
> channel preferred
0x7fff800
Done
```

### channel supported

Get supported channel mask.

```bash
> channel supported
0x7fff800
Done
```

### child list

List attached Child IDs.

```bash
> child list
1 2 3 6 7 8
Done
```

### child table

Print table of attached children.

```bash
> child table
| ID  | RLOC16 | Timeout    | Age        | LQ In | C_VN |R|D|N|Ver|CSL|QMsgCnt| Extended MAC     |
+-----+--------+------------+------------+-------+------+-+-+-+---+---+-------+------------------+
|   1 | 0xc801 |        240 |         24 |     3 |  131 |1|0|0|  3| 0 |     0 | 4ecede68435358ac |
|   2 | 0xc802 |        240 |          2 |     3 |  131 |0|0|0|  3| 1 |     0 | a672a601d2ce37d8 |
Done
```

### child \<id\>

Print diagnostic information for an attached Thread Child. The `id` may be a Child ID or an RLOC16.

```bash
> child 1
Child ID: 1
Rloc: 9c01
Ext Addr: e2b3540590b0fd87
Mode: rn
Net Data: 184
Timeout: 100
Age: 0
Link Quality In: 3
RSSI: -20
Done
```

### childip

Get the list of IP addresses stored for MTD children.

```bash
> childip
3401: fdde:ad00:beef:0:3037:3e03:8c5f:bc0c
Done
```

### childip max

Get the maximum number of IP addresses that each MTD child may register with this device as parent.

```bash
> childip max
4
Done
```

### childip max \<count\>

Set the maximum number of IP addresses that each MTD child may register with this device as parent. 0 to clear the setting and restore the default.

`OPENTHREAD_CONFIG_REFERENCE_DEVICE_ENABLE` is required.

```bash
> childip max 2
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

### childsupervision interval

Get the Child Supervision Interval value.

Child supervision feature provides a mechanism for parent to ensure that a message is sent to each sleepy child within the supervision interval. If there is no transmission to the child within the supervision interval, OpenThread enqueues and sends a supervision message (a data message with empty payload) to the child. This command can only be used with FTD devices.

```bash
> childsupervision interval
30
Done
```

### childsupervision interval \<interval\>

Set the Child Supervision Interval value. This command can only be used with FTD devices.

```bash
> childsupervision interval 30
Done
```

### childsupervision checktimeout

Get the Child Supervision Check Timeout value.

If the device is a sleepy child and it does not hear from its parent within the specified check timeout, it initiates the re-attach process (MLE Child Update Request/Response exchange with its parent).

```bash
> childsupervision checktimeout
30
Done
```

### childsupervision checktimeout \<timeout\>

Set the Child Supervision Check Timeout value.

```bash
> childsupervision checktimeout 30
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

### counters

Get the supported counter names.

```bash
> counters
ip
mac
mle
Done
```

### counters \<countername\>

Get the counter value.

```bash
> counters mac
TxTotal: 10
    TxUnicast: 3
    TxBroadcast: 7
    TxAckRequested: 3
    TxAcked: 3
    TxNoAckRequested: 7
    TxData: 10
    TxDataPoll: 0
    TxBeacon: 0
    TxBeaconRequest: 0
    TxOther: 0
    TxRetry: 0
    TxErrCca: 0
    TxErrBusyChannel: 0
RxTotal: 2
    RxUnicast: 1
    RxBroadcast: 1
    RxData: 2
    RxDataPoll: 0
    RxBeacon: 0
    RxBeaconRequest: 0
    RxOther: 0
    RxAddressFiltered: 0
    RxDestAddrFiltered: 0
    RxDuplicated: 0
    RxErrNoFrame: 0
    RxErrNoUnknownNeighbor: 0
    RxErrInvalidSrcAddr: 0
    RxErrSec: 0
    RxErrFcs: 0
    RxErrOther: 0
Done
> counters mle
Role Disabled: 0
Role Detached: 1
Role Child: 0
Role Router: 0
Role Leader: 1
Attach Attempts: 1
Partition Id Changes: 1
Better Partition Attach Attempts: 0
Parent Changes: 0
Done
> counters ip
TxSuccess: 10
TxFailed: 0
RxSuccess: 5
RxFailed: 0
Done
```

### counters \<countername\> reset

Reset the counter value.

```bash
> counters mac reset
Done
> counters mle reset
Done
> counters ip reset
Done
```

### csl

Get the CSL configuration.

```bash
> csl
Channel: 11
Period: 1000 (in units of 10 symbols), 160ms
Timeout: 1000s
Done
```

### csl channel \<channel\>

Set CSL channel.

```bash
> csl channel 20
Done
```

### csl period \<period\>

Set CSL period in units of 10 symbols. Disable CSL by setting this parameter to `0`.

```bash
> csl period 3000
Done
```

### csl timeout \<timeout\>

Set the CSL timeout in seconds.

```bash
> csl timeout 10
Done
```

### networktime

Get the Thread network time and the time sync parameters.

```bash
> networktime
Network Time:     21084154us (synchronized)
Time Sync Period: 100s
XTAL Threshold:   300ppm
Done
```

### networktime \<timesyncperiod\> \<xtalthreshold\>

Set time sync parameters

- timesyncperiod: The time synchronization period, in seconds.
- xtalthreshold: The XTAL accuracy threshold for a device to become Router-Capable device, in PPM.

```bash
> networktime 100 300
Done
```

### delaytimermin

Get the minimal delay timer (in seconds).

```bash
> delaytimermin
30
Done
```

### delaytimermin \<delaytimermin\>

Set the minimal delay timer (in seconds).

```bash
> delaytimermin 60
Done
```

### discover \[channel\]

Perform an MLE Discovery operation.

- channel: The channel to discover on. If no channel is provided, the discovery will cover all valid channels.

```bash
> discover
| J | Network Name     | Extended PAN     | PAN  | MAC Address      | Ch | dBm | LQI |
+---+------------------+------------------+------+------------------+----+-----+-----+
| 0 | OpenThread       | dead00beef00cafe | ffff | f1d92a82c8d8fe43 | 11 | -20 |   0 |
Done
```

### dns config

Get the default query config used by DNS client.

The config includes the server IPv6 address and port, response timeout in msec (wait time to rx response), maximum tx attempts before reporting failure, boolean flag to indicate whether the server can resolve the query recursively or not.

```bash
> dns config
Server: [fd00:0:0:0:0:0:0:1]:1234
ResponseTimeout: 5000 ms
MaxTxAttempts: 2
RecursionDesired: no
Done
>
```

### dns config \[DNS server IP\] \[DNS server port\] \[response timeout (ms)\] \[max tx attempts\] \[recursion desired (boolean)\]

Set the default query config.

```bash
> dns config fd00::1 1234 5000 2 0
Done

> dns config
Server: [fd00:0:0:0:0:0:0:1]:1234
ResponseTimeout: 5000 ms
MaxTxAttempts: 2
RecursionDesired: no
Done
```

We can leave some of the fields as unspecified (or use value zero). The unspecified fields are replaced by the corresponding OT config option definitions `OPENTHREAD_CONFIG_DNS_CLIENT_DEFAULT_{}` to form the default query config.

```bash
> dns config fd00::2
Done

> dns config
Server: [fd00:0:0:0:0:0:0:2]:53
ResponseTimeout: 3000 ms
MaxTxAttempts: 3
RecursionDesired: yes
Done
```

### dns resolve \<hostname\> \[DNS server IP\] \[DNS server port\] \[response timeout (ms)\] \[max tx attempts\] \[recursion desired (boolean)\]

Send DNS Query to obtain IPv6 address for given hostname.

The parameters after `hostname` are optional. Any unspecified (or zero) value for these optional parameters is replaced by the value from the current default config (`dns config`).

```bash
> dns resolve ipv6.google.com
> DNS response for ipv6.google.com - 2a00:1450:401b:801:0:0:0:200e TTL: 300
```

### dns browse \<service-name\> \[DNS server IP\] \[DNS server port\] \[response timeout (ms)\] \[max tx attempts\] \[recursion desired (boolean)\]

Send a browse (service instance enumeration) DNS query to get the list of services for given service-name.

The parameters after `service-name` are optional. Any unspecified (or zero) value for these optional parameters is replaced by the value from the current default config (`dns config`).

```bash
> dns browse _service._udp.example.com
DNS browse response for _service._udp.example.com.
inst1
    Port:1234, Priority:1, Weight:2, TTL:7200
    Host:host.example.com.
    HostAddress:fd00:0:0:0:0:0:0:abcd TTL:7200
    TXT:[a=6531, b=6c12] TTL:7300
instance2
    Port:1234, Priority:1, Weight:2, TTL:7200
    Host:host.example.com.
    HostAddress:fd00:0:0:0:0:0:0:abcd TTL:7200
    TXT:[a=1234] TTL:7300
Done
```

### dns service \<service-instance-label\> \<service-name\> \[DNS server IP\] \[DNS server port\] \[response timeout (ms)\] \[max tx attempts\] \[recursion desired (boolean)\]

Send a service instance resolution DNS query for a given service instance. Service instance label is provided first, followed by the service name (note that service instance label can contain dot '.' character).

The parameters after `service-name` are optional. Any unspecified (or zero) value for these optional parameters is replaced by the value from the current default config (`dns config`).

### dns compression \[enable|disable\]

Enable/Disable the "DNS name compression" mode.

By default DNS name compression is enabled. When disabled, DNS names are appended as full and never compressed. This is applicable to OpenThread's DNS and SRP client/server modules.

This is intended for testing only and available under `REFERENCE_DEVICE` config.

Get the current "DNS name compression" mode.

```
> dns compression
Enabled
```

Set the "DNS name compression" mode.

```
> dns compression disable
Done
>
>
> dns compression
Disabled
Done
```

### domainname

Get the Thread Domain Name for Thread 1.2 device.

```bash
> domainname
Thread
Done
```

### domainname \<name\>

Set the Thread Domain Name for Thread 1.2 device.

```bash
> domainname Test\ Thread
Done
```

### dua iid

Get the Interface Identifier mannually specified for Thread Domain Unicast Address on Thread 1.2 device.

```bash
> dua iid
0004000300020001
Done
```

### dua iid \<iid\>

Set the Interface Identifier mannually specified for Thread Domain Unicast Address on Thread 1.2 device.

```bash
> dua iid 0004000300020001
Done
```

### dua iid clear

Clear the Interface Identifier mannually specified for Thread Domain Unicast Address on Thread 1.2 device.

```bash
> dua iid clear
Done
```

### eidcache

Print the EID-to-RLOC cache entries.

```bash
> eidcache
fd49:caf4:a29f:dc0e:97fc:69dd:3c16:df7d 2000 cache canEvict=1 transTime=0 eid=fd49:caf4:a29f:dc0e:97fc:69dd:3c16:df7d
fd49:caf4:a29f:dc0e:97fc:69dd:3c16:df7f fffe retry canEvict=1 timeout=10 retryDelay=30
Done
```

### eui64

Get the factory-assigned IEEE EUI-64.

```bash
> eui64
0615aae900124b00
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

**NOTE** The current commissioning credential becomes stale after changing this value. Use [pskc](#pskc--p-keypassphrase) to reset.

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

### factoryreset

Delete all stored settings, and signal a platform reset.

```bash
> factoryreset
```

### fake

Send fake Thread messages.

Note: Only for certification test.

#### fake /a/an \<dst-ipaddr\> \<target\> \<meshLocalIid\>

```bash
> fake /a/an fdde:ad00:beef:0:0:ff:fe00:a800 fd00:7d03:7d03:7d03:55f2:bb6a:7a43:a03b 1111222233334444
Done
```

### fem

Get external FEM parameters.

```bash
> fem
LNA gain 11 dBm
Done
```

### fem lnagain

Get the Rx LNA gain in dBm of the external FEM.

```bash
> fem lnagain
11
Done
```

### fem lnagain \<LNA gain\>

Set the Rx LNA gain in dBm of the external FEM.

```bash
> fem lnagain 8
Done
```

### ifconfig

Show the status of the IPv6 interface.

```bash
> ifconfig
down
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
fdde:ad00:beef:0:558:f56b:d688:799
fe80:0:0:0:f3d9:2a82:c8d8:fe43
Done
```

Use `-v` to get more verbose informations about the address.

```bash
> ipaddr -v
fdde:ad00:beef:0:0:ff:fe00:0 origin:thread
fdde:ad00:beef:0:558:f56b:d688:799 origin:thread
fe80:0:0:0:f3d9:2a82:c8d8:fe43 origin:thread
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

### ipaddr linklocal

Print Thread link-local IPv6 address.

```bash
> ipaddr linklocal
fe80:0:0:0:f3d9:2a82:c8d8:fe43
Done
```

### ipaddr mleid

Print Thread Mesh Local EID address.

```bash
> ipaddr mleid
fdde:ad00:beef:0:558:f56b:d688:799
Done
```

### ipaddr rloc

Print Thread Routing Locator (RLOC) address.

```bash
> ipaddr rloc
fdde:ad00:beef:0:0:ff:fe00:0
Done
```

### ipmaddr

List all IPv6 multicast addresses subscribed to the Thread interface.

```bash
> ipmaddr
ff05:0:0:0:0:0:0:1
ff33:40:fdde:ad00:beef:0:0:1
ff32:40:fdde:ad00:beef:0:0:1
Done
```

### ipmaddr add \<ipaddr\>

Subscribe the Thread interface to the IPv6 multicast address.

```bash
> ipmaddr add ff05::1
Done
```

### ipmaddr del \<ipaddr\>

Unsubscribe the Thread interface to the IPv6 multicast address.

```bash
> ipmaddr del ff05::1
Done
```

### ipmaddr llatn

Get the Link-Local All Thread Nodes multicast address.

```
> ipmaddr llatn
ff32:40:fdde:ad00:beef:0:0:1
Done
```

### ipmaddr promiscuous

Get multicast promiscuous mode.

```bash
> ipmaddr promiscuous
Disabled
Done
```

### ipmaddr promiscuous enable

Enable multicast promiscuous mode.

```bash
> ipmaddr promiscuous enable
Done
```

### ipmaddr promiscuous disable

Disable multicast promiscuous mode.

```bash
> ipmaddr promiscuous disable
Done
```

### ipmaddr rlatn

Get the Realm-Local All Thread Nodes multicast address.

```
> ipmaddr rlatn
ff33:40:fdde:ad00:beef:0:0:1
Done
```

### joinerport \<port\>

Set the Joiner port.

```bash
> joinerport 1000
Done
```

### keysequence counter

Get the Thread Key Sequence Counter.

```bash
> keysequence counter
10
Done
```

### keysequence counter \<counter\>

Set the Thread Key Sequence Counter.

```bash
> keysequence counter 10
Done
```

### keysequence guardtime

Get Thread Key Switch Guard Time (in hours)

```bash
> keysequence guardtime
0
Done
```

### keysequence guardtime \<guardtime\>

Set Thread Key Switch Guard Time (in hours) 0 means Thread Key Switch imediately if key index match

```bash
> keysequence guardtime 0
Done
```

### leaderdata

Show the Thread Leader Data.

```bash
> leaderdata
Partition ID: 1077744240
Weighting: 64
Data Version: 109
Stable Data Version: 211
Leader Router ID: 60
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

### linkmetrics mgmt \<ipaddr\> enhanced-ack clear

Send a Link Metrics Management Request to clear an Enhanced-ACK Based Probing.

- ipaddr: Peer address (SHOULD be link local address of the neighboring device).

```bash
> linkmetrics mgmt fe80:0:0:0:3092:f334:1455:1ad2 enhanced-ack clear
Done
> Received Link Metrics Management Response from: fe80:0:0:0:3092:f334:1455:1ad2
Status: Success
```

### linkmetrics mgmt \<ipaddr\> enhanced-ack register [qmr][r]

Send a Link Metrics Management Request to register an Enhanced-ACK Based Probing.

- ipaddr: Peer address.
- qmr: This specifies what metrics to query. At most two options are allowed to select (per spec 4.11.3.4.4.6).
  - q: Layer 2 LQI.
  - m: Link Margin.
  - r: RSSI.
- r: This is optional and only used for reference devices. When this option is specified, Type/Average Enum of each Type Id Flags would be set to `reserved`. This is used to verify the Probing Subject correctly handles invalid Type Id Flags. This is only available when `OPENTHREAD_CONFIG_REFERENCE_DEVICE_ENABLE` is enabled.

```bash
> linkmetrics mgmt fe80:0:0:0:3092:f334:1455:1ad2 enhanced-ack register qm
Done
> Received Link Metrics Management Response from: fe80:0:0:0:3092:f334:1455:1ad2
Status: Success

> linkmetrics mgmt fe80:0:0:0:3092:f334:1455:1ad2 enhanced-ack register qm r
Done
> Received Link Metrics Management Response from: fe80:0:0:0:3092:f334:1455:1ad2
Status: Cannot support new series
```

### linkmetrics mgmt \<ipaddr\> forward \<seriesid\> [ldraX][pqmr]

Send a Link Metrics Management Request to configure a Forward Tracking Series.

- ipaddr: Peer address.
- seriesid: The Series ID.
- ldraX: This specifies which frames are to be accounted.
  - l: MLE Link Probe.
  - d: MAC Data.
  - r: MAC Data Request.
  - a: MAC Ack.
  - X: This represents none of the above flags, i.e., to stop accounting and remove the series. This can only be used without any other flags.
- pqmr: This specifies what metrics to query.
  - p: Layer 2 Number of PDUs received.
  - q: Layer 2 LQI.
  - m: Link Margin.
  - r: RSSI.

```bash
> linkmetrics mgmt fe80:0:0:0:3092:f334:1455:1ad2 forward 1 dra pqmr
Done
> Received Link Metrics Management Response from: fe80:0:0:0:3092:f334:1455:1ad2
Status: SUCCESS
```

### linkmetrics probe \<ipaddr\> \<seriesid\> \<length\>

Send a MLE Link Probe message to the peer.

- ipaddr: Peer address.
- seriesid: The Series ID for which this Probe message targets at.
- length: The length of the Probe message, valid range: [0, 64].

```bash
> linkmetrics probe fe80:0:0:0:3092:f334:1455:1ad2 1 10
Done
```

### linkmetrics query \<ipaddr\> single [pqmr]

Perform a Link Metrics query (Single Probe).

- ipaddr: Peer address.
- pqmr: This specifies what metrics to query.
- p: Layer 2 Number of PDUs received.
- q: Layer 2 LQI.
- m: Link Margin.
- r: RSSI.

```bash
> linkmetrics query fe80:0:0:0:3092:f334:1455:1ad2 single qmr
Done
> Received Link Metrics Report from: fe80:0:0:0:3092:f334:1455:1ad2

 - LQI: 76 (Exponential Moving Average)
 - Margin: 82 (dB) (Exponential Moving Average)
 - RSSI: -18 (dBm) (Exponential Moving Average)
```

### linkmetrics query \<ipaddr\> forward \<seriesid\>

Perform a Link Metrics query (Forward Tracking Series).

- ipaddr: Peer address.
- seriesid: The Series ID.

```bash
> linkmetrics query fe80:0:0:0:3092:f334:1455:1ad2 forward 1
Done
> Received Link Metrics Report from: fe80:0:0:0:3092:f334:1455:1ad2

 - PDU Counter: 2 (Count/Summation)
 - LQI: 76 (Exponential Moving Average)
 - Margin: 82 (dB) (Exponential Moving Average)
 - RSSI: -18 (dBm) (Exponential Moving Average)
```

### linkquality \<extaddr\>

Get the link quality on the link to a given extended address.

```bash
> linkquality 36c1dd7a4f5201ff
3
Done
```

### linkquality \<extaddr\> \<linkquality\>

Set the link quality on the link to a given extended address.

```bash
> linkquality 36c1dd7a4f5201ff 3
Done
```

### locate

Gets the current state (`In Progress` or `Idle`) of anycast locator.

`OPENTHREAD_CONFIG_TMF_ANYCAST_LOCATOR_ENABLE` is required.

```bash
> locate
Idle
Done

> locate fdde:ad00:beef:0:0:ff:fe00:fc10

> locate
In Progress
Done
```

### locate \<anycastaddr\>

Locate the closest destination of an anycast address (i.e., find the destination's mesh local EID and RLOC16).

`OPENTHREAD_CONFIG_TMF_ANYCAST_LOCATOR_ENABLE` is required.

The closest destination is determined based on the the current routing table and path costs within the Thread mesh.

Locate the leader using its anycast address:

```bash
> locate fdde:ad00:beef:0:0:ff:fe00:fc00
fdde:ad00:beef:0:d9d3:9000:16b:d03b 0xc800
Done
```

Locate the closest destination of a service anycast address:

```bash

> srp server enable
Done

> netdata show
Prefixes:
Routes:
Services:
44970 5d c002 s c800
44970 5d c002 s cc00
Done

> locate fdde:ad00:beef:0:0:ff:fe00:fc10
fdde:ad00:beef:0:a477:dc98:a4e4:71ea 0xcc00
done
```

### log filename \<filename\>

- Note: Simulation Only, ie: `OPENTHREAD_EXAMPLES_SIMULATION`
- Requires `OPENTHREAD_CONFIG_LOG_OUTPUT == OPENTHREAD_CONFIG_LOG_OUTPUT_DEBUG_UART`

Specifies filename to capture otPlatLog() messages, useful when debugging automated test scripts on Linux when logging disrupts the automated test scripts.

### log level

Get the log level.

```bash
> log level
1
Done
```

### log level \<level\>

Set the log level.

```bash
> log level 4
Done
```

### mliid \<iid\>

Set the Mesh Local IID.

It must be used before Thread stack is enabled.

Only for testing/reference device.

```bash
> mliid 1122334455667788
Done
```

### mlr reg \<ipaddr\> ... [timeout]

Register Multicast Listeners to Primary Backbone Router, with an optional `timeout` (in seconds).

Omit `timeout` to use the default MLR timeout on the Primary Backbone Router.

Use `timeout = 0` to deregister Multicast Listeners.

NOTE: Only for Thread 1.2 Commissioner FTD device.

```bash
> mlr reg ff04::1
status 0, 0 failed
Done
> mlr reg ff04::1 ff04::2 ff02::1
status 2, 1 failed
ff02:0:0:0:0:0:0:1
Done
> mlr reg ff04::1 ff04::2 1000
status 0, 0 failed
Done
> mlr reg ff04::1 ff04::2 0
status 0, 0 failed
Done
```

### mode

Get the Thread Device Mode value.

- -: no flags set (rx-off-when-idle, minimal Thread device, stable network data)
- r: rx-on-when-idle
- d: Full Thread Device
- n: Full Network Data

```bash
> mode
rdn
Done
```

### mode [rdn]

Set the Thread Device Mode value.

- -: no flags set (rx-off-when-idle, minimal Thread device, stable network data)
- r: rx-on-when-idle
- d: Full Thread Device
- n: Full Network Data

```bash
> mode rdn
Done
```

```bash
> mode -
Done
```

### multiradio

Get the list of supported radio links by the device.

This command is always available, even when only a single radio is supported by the device.

```bash
> multiradio
[15.4, TREL]
Done
```

### multiradio neighbor list

Get the list of neighbors and their supported radios and their preference.

This command is only available when device supports more than one radio link.

```bash
> multiradio neighbor list
ExtAddr:3a65bc38dbe4a5be, RLOC16:0xcc00, Radios:[15.4(255), TREL(255)]
ExtAddr:17df23452ee4a4be, RLOC16:0x1300, Radios:[15.4(255)]
Done
```

### multiradio neighbor \<ext address\>

Get the radio info for specific neighbor with a given extended address.

This command is only available when device supports more than one radio link.

```bash
> multiradio neighbor 3a65bc38dbe4a5be
[15.4(255), TREL(255)]
Done
```

### neighbor list

List RLOC16 of neighbors.

```bash
> neighbor list
0xcc01 0xc800 0xf000
Done
```

### neighbor table

Print table of neighbors.

```bash
> neighbor table
| Role | RLOC16 | Age | Avg RSSI | Last RSSI |R|D|N| Extended MAC     |
+------+--------+-----+----------+-----------+-+-+-+------------------+
|   C  | 0xcc01 |  96 |      -46 |       -46 |1|1|1| 1eb9ba8a6522636b |
|   R  | 0xc800 |   2 |      -29 |       -29 |1|1|1| 9a91556102c39ddb |
|   R  | 0xf000 |   3 |      -28 |       -28 |1|1|1| 0ad7ed6beaa6016d |
Done
```

### netstat

List all UDP sockets.

```bash
> netstat
| Local Address                                   | Peer Address                                    |
+-------------------------------------------------+-------------------------------------------------+
| [0:0:0:0:0:0:0:0]:49153                         | [0:0:0:0:0:0:0:0]:0                             |
| [0:0:0:0:0:0:0:0]:49152                         | [0:0:0:0:0:0:0:0]:0                             |
| [0:0:0:0:0:0:0:0]:61631                         | [0:0:0:0:0:0:0:0]:0                             |
| [0:0:0:0:0:0:0:0]:19788                         | [0:0:0:0:0:0:0:0]:0                             |
Done
```

### networkdiagnostic get \<addr\> \<type\> ..

Send network diagnostic request to retrieve tlv of \<type\>s.

If \<addr\> is unicast address, `Diagnostic Get` will be sent. if \<addr\> is multicast address, `Diagnostic Query` will be sent.

```bash
> networkdiagnostic get fdde:ad00:beef:0:0:ff:fe00:fc00 0 1 6
> DIAG_GET.rsp/ans: 00080e336e1c41494e1c01020c000608640b0f674074c503
Ext Address: '0e336e1c41494e1c'
Rloc16: 0x0c00
Leader Data:
    PartitionId: 0x640b0f67
    Weighting: 64
    DataVersion: 116
    StableDataVersion: 197
    LeaderRouterId: 0x03
Done

> networkdiagnostic get ff02::1 0 1
> DIAG_GET.rsp/ans: 00080e336e1c41494e1c01020c00
Ext Address: '0e336e1c41494e1c'
Rloc16: 0x0c00
Done
DIAG_GET.rsp/ans: 00083efcdb7e3f9eb0f201021800
Ext Address: '3efcdb7e3f9eb0f2'
Rloc16: 0x1800
Done
```

### networkdiagnostic reset \<addr\> \<type\> ..

Send network diagnostic request to reset \<addr\>'s tlv of \<type\>s. Currently only `MAC Counters`(9) is supported.

```bash
> diagnostic reset fd00:db8::ff:fe00:0 9
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

### networkkey

Get the Thread Network Key value.

```bash
> networkkey
00112233445566778899aabbccddeeff
Done
```

### networkkey \<key\>

Set the Thread Network Key value.

```bash
> networkkey 00112233445566778899aabbccddeeff
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

**NOTE** The current commissioning credential becomes stale after changing this value. Use [pskc](#pskc--p-keypassphrase) to reset.

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

Note: When operating as a Thread Router, this command will return the cached information from when the device was previously attached as a Thread Child. Returning cached information is necessary to support the Thread Test Harness - Test Scenario 8.2.x requests the former parent (i.e. Joiner Router's) MAC address even if the device has already promoted to a router.

```bash
> parent
Ext Addr: be1857c6c21dce55
Rloc: 5c00
Link Quality In: 3
Link Quality Out: 3
Age: 20
Done
```

### parentpriority

Get the assigned parent priority value, -2 means not assigned.

```bash
> parentpriority
1
Done
```

### parentpriority \<parentpriority\>

Set the assigned parent priority value: 1, 0, -1 or -2.

```bash
> parentpriority 1
Done
```

### partitionid

Get the Thread Network Partition ID.

```bash
> partitionid
4294967295
Done
```

### partitionid preferred

Get the preferred Thread Leader Partition ID.

`OPENTHREAD_CONFIG_REFERENCE_DEVICE_ENABLE` is required.

```bash
> partitionid preferred
4294967295
Done
```

### partitionid preferred \<partitionid\>

Set the preferred Thread Leader Partition ID.

`OPENTHREAD_CONFIG_REFERENCE_DEVICE_ENABLE` is required.

```bash
> partitionid preferred 0xffffffff
Done
```

### ping \[-I source\] \<ipaddr\> \[size\] \[count\] \[interval\] \[hoplimit\] \[timeout\]

Send an ICMPv6 Echo Request.

- source: The source IPv6 address of the echo request.
- size: The number of data bytes to be sent.
- count: The number of ICMPv6 Echo Requests to be sent.
- interval: The interval between two consecutive ICMPv6 Echo Requests in seconds. The value may have fractional form, for example `0.5`.
- hoplimit: The hoplimit of ICMPv6 Echo Request to be sent.
- timeout: Time in seconds to wait for the final ICMPv6 Echo Reply after sending out the request. The value may have fractional form, for example `3.5`.

```bash
> ping fd00:db8:0:0:76b:6a05:3ae9:a61a
> 16 bytes from fd00:db8:0:0:76b:6a05:3ae9:a61a: icmp_seq=5 hlim=64 time=0ms
1 packets transmitted, 1 packets received. Packet loss = 0.0%. Round-trip min/avg/max = 0/0.0/0 ms.
Done

> ping -I fd00:db8:0:0:76b:6a05:3ae9:a61a ff02::1 100 1 1 1
> 108 bytes from fd00:db8:0:0:f605:fb4b:d429:d59a: icmp_seq=4 hlim=64 time=7ms
1 packets transmitted, 1 packets received. Round-trip min/avg/max = 7/7.0/7 ms.
Done
```

### ping stop

Stop sending ICMPv6 Echo Requests.

```bash
> ping stop
Done
```

### pollperiod

Get the customized data poll period of sleepy end device (milliseconds). Only for certification test

```bash
> pollperiod
0
Done
```

### pollperiod \<pollperiod\>

Set the customized data poll period for sleepy end device (milliseconds >= 10ms). Only for certification test

```bash
> pollperiod 10
Done
```

### pskc [-p] \<key\>|\<passphrase\>

With `-p` generate pskc from \<passphrase\> (UTF-8 encoded) together with **current** network name and extended PAN ID, otherwise set pskc as \<key\> (hex format).

```bash
> pskc 67c0c203aa0b042bfb5381c47aef4d9e
Done
> pskc -p 123456
Done
```

### preferrouterid \<routerid\>

Prefer a Router ID when solicit router id from Leader.

```bash
> preferrouterid 16
Done
```

### prefix

Get the prefix list in the local Network Data. Note: For the Thread 1.2 border router with backbone capability, the local Domain Prefix would be listed as well (with flag `D`), with preceeding `-` if backbone functionality is disabled.

```bash
> prefix
2001:dead:beef:cafe::/64 paros med
- fd00:7d03:7d03:7d03::/64 prosD med
Done
```

### prefix add \<prefix\> [padcrosnD][prf]

Add a valid prefix to the Network Data.

Note: The Domain Prefix flag (`D`) is only available for Thread 1.2.

- p: Preferred flag
- a: Stateless IPv6 Address Autoconfiguration flag
- d: DHCPv6 IPv6 Address Configuration flag
- c: DHCPv6 Other Configuration flag
- r: Default Route flag
- o: On Mesh flag
- s: Stable flag
- n: Nd Dns flag
- D: Domain Prefix flag
- prf: Default router preference, which may be 'high', 'med', or 'low'.

```bash
> prefix add 2001:dead:beef:cafe::/64 paros med
Done

> prefix add fd00:7d03:7d03:7d03::/64 prosD med
Done
```

### prefix meshlocal

Get the mesh local prefix.

```bash
> prefix meshlocal
fdde:ad00:beef:0::/64
Done
```

### prefix meshlocal <prefix>

Set the mesh local prefix.

```bash
> prefix meshlocal fdde:ad00:beef:0::/64
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

### radiofilter

`OPENTHREAD_CONFIG_MAC_FILTER_ENABLE` is required.

The radio filter is mainly intended for testing. It can be used to temporarily block all tx/rx on the IEEE 802.15.4 radio.

When radio filter is enabled, radio is put to sleep instead of receive (to ensure device does not receive any frame and/or potentially send ack). Also the frame transmission requests return immediately without sending the frame over the air (return "no ack" error if ack is requested, otherwise return success).

Get radio filter status (enabled or disabled).

```bash
> radiofilter
Disabled
Done
```

### radiofilter enable

`OPENTHREAD_CONFIG_MAC_FILTER_ENABLE` is required.

Enable radio radio filter.

```bash
> radiofilter enable
Done
```

### radiofilter disable

`OPENTHREAD_CONFIG_MAC_FILTER_ENABLE` is required.

Disable radio radio filter.

```bash
> radiofilter disable
Done
```

### rcp

RCP-related commands.

### region

Set the radio region, this can affect the transmit power limit.

```bash
> region US
Done
> region
US
Done
```

### rcp version

Print RCP version string.

```bash
> rcp version
OPENTHREAD/20191113-00825-g82053cc9d-dirty; SIMULATION; Jun  4 2020 17:53:16
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

### route

Get the external route list in the local Network Data.

```bash
> route
2001:dead:beef:cafe::/64 s med
Done
```

### route add \<prefix\> [sn][prf]

Add a valid external route to the Network Data.

- s: Stable flag
- n: NAT64 flag
- prf: Default Router Preference, which may be: 'high', 'med', or 'low'.

```bash
> route add 2001:dead:beef:cafe::/64 s med
Done
```

### route remove \<prefix\>

Invalidate a external route in the Network Data.

```bash
> route remove 2001:dead:beef:cafe::/64
Done
```

### router list

List allocated Router IDs.

```bash
> router list
8 24 50
Done
```

### router table

Print table of routers.

```bash
> router table
| ID | RLOC16 | Next Hop | Path Cost | LQ In | LQ Out | Age | Extended MAC     | Link |
+----+--------+----------+-----------+-------+--------+-----+------------------+------+
| 22 | 0x5800 |       63 |         0 |     0 |      0 |   0 | 0aeb8196c9f61658 |    0 |
| 49 | 0xc400 |       63 |         0 |     3 |      3 |   0 | faa1c03908e2dbf2 |    1 |
Done
```

### router \<id\>

Print diagnostic information for a Thread Router. The `id` may be a Router ID or an RLOC16.

```bash
> router 50
Alloc: 1
Router ID: 50
Rloc: c800
Next Hop: c800
Link: 1
Ext Addr: e2b3540590b0fd87
Cost: 0
Link Quality In: 3
Link Quality Out: 3
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
Link Quality In: 3
Link Quality Out: 3
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

### routereligible

Indicates whether the router role is enabled or disabled.

```bash
> routereligible
Enabled
Done
```

### routereligible enable

Enable the router role.

```bash
> routereligible enable
Done
```

### routereligible disable

Disable the router role.

```bash
> routereligible disable
Done
```

### routerselectionjitter

Get the ROUTER_SELECTION_JITTER value.

```bash
> routerselectionjitter
120
Done
```

### routerselectionjitter \<jitter\>

Set the ROUTER_SELECTION_JITTER value.

```bash
> routerselectionjitter 120
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

- channel: The channel to scan on. If no channel is provided, the active scan will cover all valid channels.

```bash
> scan
| PAN  | MAC Address      | Ch | dBm | LQI |
+------+------------------+----+-----+-----+
| ffff | f1d92a82c8d8fe43 | 11 | -20 |   0 |
Done
```

### scan energy \[duration\] \[channel\]

Perform an IEEE 802.15.4 Energy Scan.

- duration: The time in milliseconds to spend scanning each channel.

```bash
> scan energy 10
| Ch | RSSI |
+----+------+
| 11 |  -59 |
| 12 |  -62 |
| 13 |  -67 |
| 14 |  -61 |
| 15 |  -87 |
| 16 |  -86 |
| 17 |  -86 |
| 18 |  -52 |
| 19 |  -58 |
| 20 |  -82 |
| 21 |  -76 |
| 22 |  -82 |
| 23 |  -74 |
| 24 |  -81 |
| 25 |  -88 |
| 26 |  -71 |
Done
```

```bash
> scan energy 10 20
| Ch | RSSI |
+----+------+
| 20 |  -82 |
Done
```

### service

Module for controlling service registration in Network Data. Each change in service registration must be sent to leader by `netdata register` command before taking effect.

### service add \<enterpriseNumber\> \<serviceData\> \<serverData\>

Add service to the Network Data.

- enterpriseNumber: IANA enterprise number
- serviceData: hex-encoded binary service data
- serverData: hex-encoded binary server data

```bash
> service add 44970 112233 aabbcc
Done
> netdata register
Done
```

### service remove \<enterpriseNumber\> \<serviceData\>

Remove service from Network Data.

- enterpriseNumber: IANA enterprise number
- serviceData: hext-encoded binary service data

```bash
> service remove 44970 112233
Done
> netdata register
Done
```

### singleton

Return true when there are no other nodes in the network, otherwise return false.

```bash
> singleton
true or false
Done
```

### sntp query \[SNTP server IP\] \[SNTP server port\]

Send SNTP Query to obtain current unix epoch time (from 1st January 1970). The latter two parameters have following default values:

- NTP server IP: 2001:4860:4806:8:: (Google IPv6 NTP Server)
- NTP server port: 123

```bash
> sntp query
> SNTP response - Unix time: 1540894725 (era: 0)
```

You can use NAT64 of OpenThread Border Router to reach e.g. Google IPv4 NTP Server:

```bash
> sntp query 64:ff9b::d8ef:2308
> SNTP response - Unix time: 1540898611 (era: 0)
```

### state

Return state of current state.

```bash
> state
offline, disabled, detached, child, router or leader
Done
```

### state <state>

Try to switch to state `detached`, `child`, `router` or `leader`.

```bash
> state leader
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

### thread version

Get the Thread Version number.

```bash
> thread version
2
Done
```

### trel

Indicate whether TREL radio operation is enabled or not.

`OPENTHREAD_CONFIG_RADIO_LINK_TREL_ENABLE` is required for all `trel` sub-commands.

```bash
> trel
Enabled
Done
```

### trel enable

Enable TREL operation.

```bash
> trel enable
Done
```

### trel disable

Disable TREL operation.

```bash
> trel disable
Done
```

### trel filter

Indicate whether TREL filter mode is enabled or not

When filter mode is enabled, any rx and tx traffic through TREL interface is silently dropped. This is mainly intended for use during testing.

```bash
> trel filter
Disabled
Done
```

### trel filter enable

Enable TREL filter mode.

```bash
> trel filter enable
Done
```

### trel filter disable

Disable TREL filter mode.

```bash
> trel filter disable
Done
```

### trel peers [list]

Get the TREL peer table in table format or as a list.

```bash
> trel peers
| No  | Ext MAC Address  | Ext PAN Id       | IPv6 Socket Address                              |
+-----+------------------+------------------+--------------------------------------------------+
|   1 | 5e5785ba3a63adb9 | f0d9c001f00d2e43 | [fe80:0:0:0:cc79:2a29:d311:1aea]:9202            |
|   2 | ce792a29d3111aea | dead00beef00cafe | [fe80:0:0:0:5c57:85ba:3a63:adb9]:9203            |
Done

> trel peers list
001 ExtAddr:5e5785ba3a63adb9 ExtPanId:f0d9c001f00d2e43 SockAddr:[fe80:0:0:0:cc79:2a29:d311:1aea]:9202
002 ExtAddr:ce792a29d3111aea ExtPanId:dead00beef00cafe SockAddr:[fe80:0:0:0:5c57:85ba:3a63:adb9]:9203
>>>>>>> [trel] implement new TREL model using DNS-SD
Done
```

### tvcheck enable

Enable thread version check when upgrading to router or leader.

Note: Thread version check is enabled by default.

`OPENTHREAD_CONFIG_REFERENCE_DEVICE_ENABLE` is required.

```bash
> tvcheck enable
Done
```

### tvcheck disable

Enable thread version check when upgrading to router or leader.

Note: Thread version check is enabled by default.

`OPENTHREAD_CONFIG_REFERENCE_DEVICE_ENABLE` is required.

```bash
> tvcheck disable
Done
```

### txpower

Get the transmit power in dBm.

```bash
> txpower
-10 dBm
Done
```

### txpower \<txpower\>

Set the transmit power in dBm.

```bash
> txpower -10
Done
```

### unsecureport add \<port\>

Add a port to the allowed unsecured port list.

```bash
> unsecureport add 1234
Done
```

### unsecureport remove \<port\>

Remove a port from the allowed unsecured port list.

```bash
> unsecureport remove 1234
Done
```

### unsecureport remove all

Remove all ports from the allowed unsecured port list.

```bash
> unsecureport remove all
Done
```

### unsecureport get

Print all ports from the allowed unsecured port list.

```bash
> unsecureport get
1234
Done
```

### uptime

This command requires `OPENTHREAD_CONFIG_UPTIME_ENABLE` to be enabled.

Print the OpenThread stack uptime (duration since OpenThread stack initialization).

```bash
> uptime
12:46:35.469
Done
>
```

### uptime ms

This command requires `OPENTHREAD_CONFIG_UPTIME_ENABLE` to be enabled.

Print the OpenThread stack uptime in msec.

```bash
> uptime ms
426238
Done
>
```

### version

Print the build version information.

```bash
> version
OPENTHREAD/gf4f2f04; Jul  1 2016 17:00:09
Done
```

### version api

Print API version number.

```bash
> version api
28
Done
```

### mac retries direct

Get the number of direct TX retries on the MAC layer.

```bash
> mac retries direct
3
Done
```

### mac retries direct \<number\>

Set the number of direct TX retries on the MAC layer.

```bash
> mac retries direct 5
Done
```

### mac retries indirect

Get the number of indirect TX retries on the MAC layer.

```bash
> mac retries indirect
3
Done
```

### mac retries indirect \<number\>

Set the number of indirect TX retries on the MAC layer.

```bash
> mac retries indirect 5
Done
```

### mac send \<op\>

Instruct an Rx-Off-When-Idle device to send a mac frame to its parent. The mac frame could be either a mac data request or an empty mac data frame. Use `datarequest` to send a mac data request and `data` to send an empty mac data. This feature is for certification, it can only be used when `OPENTHREAD_CONFIG_REFERENCE_DEVICE_ENABLE` is enabled.

```bash
> mac send datarequest
Done
```

```bash
> mac send emptydata
Done
```

### macfilter

List the macfilter status, including address and received signal strength filter settings.

```bash
> macfilter
Address Mode: Allowlist
0f6127e33af6b403 : rss -95 (lqi 1)
0f6127e33af6b402
RssIn List:
0f6127e33af6b403 : rss -95 (lqi 1)
Default rss : -50 (lqi 3)
Done
```

### macfilter addr

List the address filter status.

```bash
> macfilter addr
Allowlist
0f6127e33af6b403 : rss -95 (lqi 1)
0f6127e33af6b402
Done
```

### macfilter addr disable

Disable address filter mode.

```bash
> macfilter addr disable
Done
```

### macfilter addr allowlist

Enable allowlist address filter mode.

```bash
> macfilter addr allowlist
Done
```

### macfilter addr denylist

Enable denylist address filter mode.

```bash
> macfilter addr denylist
Done
```

### macfilter addr add \<extaddr\> \[rss\]

Add an IEEE 802.15.4 Extended Address to the address filter, and fixed the received singal strength for the messages from the address if rss is specified.

```bash
> macfilter addr add 0f6127e33af6b403 -95
Done
```

```bash
> macfilter addr add 0f6127e33af6b402
Done
```

### macfilter addr remove \<extaddr\>

Remove the IEEE802.15.4 Extended Address from the address filter.

```bash
> macfilter addr remove 0f6127e33af6b402
Done
```

### macfilter addr clear

Clear all the IEEE802.15.4 Extended Addresses from the address filter.

```bash
> macfilter addr clear
Done
```

### macfilter rss

List the rss filter status

```bash
> macfilter rss
0f6127e33af6b403 : rss -95 (lqi 1)
Default rss: -50 (lqi 3)
Done
```

### macfilter rss add \<extaddr\> \<rss\>

Set the received signal strength for the messages from the IEEE802.15.4 Extended Address. If extaddr is \*, default received signal strength for all received messages would be set.

```bash
> macfilter rss add * -50
Done
```

```bash
> macfilter rss add 0f6127e33af6b404 -85
Done
```

### macfilter rss add-lqi \<extaddr\> \<lqi\>

Set the received link quality for the messages from the IEEE802.15.4 Extended Address. Valid lqi range [0,3] If extaddr is \*, default received link quality for all received messages would be set. Equivalent with 'filter rss add' with similar usage

```bash
> macfilter rss add-lqi * 3
Done
```

```bash
> macfilter rss add 0f6127e33af6b404 2
Done
```

### macfilter rss remove \<extaddr\>

Removes the received signal strength or received link quality setting on the Extended Address. If extaddr is \*, default received signal strength or link quality for all received messages would be unset.

```bash
> macfilter rss remove *
Done
```

```bash
> macfilter rss remove 0f6127e33af6b404
Done
```

### macfilter rss clear

Clear all the received signal strength or received link quality settings.

```bash
> macfilter rss clear
Done
```

### diag

Factory Diagnostics module is enabled only when building OpenThread with `OPENTHREAD_CONFIG_DIAG_ENABLE=1` option. Go [diagnostics module][diag] for more information.

[diag]: ../../src/core/diags/README.md
