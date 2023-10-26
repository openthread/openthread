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
- [br](README_BR.md)
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
- [coex](#coex)
- [commissioner](README_COMMISSIONER.md)
- [contextreusedelay](#contextreusedelay)
- [counters](#counters)
- [csl](#csl)
- [dataset](README_DATASET.md)
- [debug](#debug)
- [delaytimermin](#delaytimermin)
- [detach](#detach)
- [deviceprops](#deviceprops)
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
- [instanceid](#instanceid)
- [ipaddr](#ipaddr)
- [ipmaddr](#ipmaddr)
- [joiner](README_JOINER.md)
- [joinerport](#joinerport-port)
- [keysequence](#keysequence-counter)
- [leaderdata](#leaderdata)
- [leaderweight](#leaderweight)
- [linkmetrics](#linkmetrics-mgmt-ipaddr-enhanced-ack-clear)
- [linkmetricsmgr](#linkmetricsmgr-disable)
- [locate](#locate)
- [log](#log-filename-filename)
- [mac](#mac-retries-direct)
- [macfilter](#macfilter)
- [meshdiag](#meshdiag-topology)
- [mliid](#mliid-iid)
- [mlr](#mlr-reg-ipaddr--timeout)
- [mode](#mode)
- [multiradio](#multiradio)
- [nat64](#nat64-cidr)
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
- [ping](#ping-async--i-source--m-ipaddr-size-count-interval-hoplimit-timeout)
- [platform](#platform)
- [pollperiod](#pollperiod-pollperiod)
- [preferrouterid](#preferrouterid-routerid)
- [prefix](#prefix)
- [promiscuous](#promiscuous)
- [pskc](#pskc)
- [pskcref](#pskcref)
- [radio](#radio-enable)
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
- [childrouterlinks](#childrouterlinks)
- [scan](#scan-channel)
- [service](#service)
- [singleton](#singleton)
- [sntp](#sntp-query-sntp-server-ip-sntp-server-port)
- [state](#state)
- [srp](README_SRP.md)
- [tcat](README_TCAT.md)
- [tcp](README_TCP.md)
- [thread](#thread-start)
- [timeinqueue](#timeinqueue)
- [trel](#trel)
- [tvcheck](#tvcheck-enable)
- [txpower](#txpower)
- [udp](README_UDP.md)
- [unsecureport](#unsecureport-add-port)
- [uptime](#uptime)
- [vendor](#vendor-name)
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

Enable Backbone Router Service for Thread 1.2 FTD. `SRV_DATA.ntf` would be triggered for attached device if there is no Backbone Router Service in Thread Network Data.

`OPENTHREAD_CONFIG_BACKBONE_ROUTER_ENABLE` is required.

```bash
> bbr enable
Done
```

### bbr disable

Disable Backbone Router Service for Thread 1.2 FTD. `SRV_DATA.ntf` would be triggered if Backbone Router is Primary state. o `OPENTHREAD_CONFIG_BACKBONE_ROUTER_ENABLE` is required.

```bash
> bbr disable
Done
```

### bbr register

Register Backbone Router Service for Thread 1.2 FTD. `SRV_DATA.ntf` would be triggered for attached device.

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

### bufferinfo

Show the current message buffer information.

- The `total` shows total number of message buffers in pool.
- The `free` shows the number of free message buffers.
- The `max-used` shows the maximum number of used buffers at the same time since OT stack initialization or last `bufferinfo reset`.
- This is then followed by info about different queues used by OpenThread stack, each line representing info about a queue.
  - The first number shows number messages in the queue.
  - The second number shows number of buffers used by all messages in the queue.
  - The third number shows total number of bytes of all messages in the queue.

```bash
> bufferinfo
total: 40
free: 40
max-used: 5
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

### bufferinfo reset

Reset the message buffer counter tracking maximum number buffers in use at the same time.

```bash
> bufferinfo reset
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
| ID  | RLOC16 | Timeout    | Age        | LQ In | C_VN |R|D|N|Ver|CSL|QMsgCnt|Suprvsn| Extended MAC     |
+-----+--------+------------+------------+-------+------+-+-+-+---+---+-------+-------+------------------+
|   1 | 0xc801 |        240 |         24 |     3 |  131 |1|0|0|  3| 0 |     0 |   129 | 4ecede68435358ac |
|   2 | 0xc802 |        240 |          2 |     3 |  131 |0|0|0|  3| 1 |     0 |     0 | a672a601d2ce37d8 |
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
CSL Synchronized: 1
Net Data: 184
Timeout: 100
Age: 0
Link Quality In: 3
RSSI: -20
Supervision Interval: 129
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

Get the Child Supervision interval value on the child.

Child Supervision feature provides a mechanism for parent to ensure that a message is sent to each sleepy child within the supervision interval. If there is no transmission to the child within the supervision interval, OpenThread enqueues and sends a Child Supervision Message to the child.

```bash
> childsupervision interval
30
Done
```

### childsupervision interval \<interval\>

Set the Child Supervision interval value on the child.

```bash
> childsupervision interval 30
Done
```

### childsupervision checktimeout

Get the Child Supervision Check Timeout value on the child.

If the device is a sleepy child and it does not hear from its parent within the specified check timeout, it initiates the re-attach process (MLE Child Update Request/Response exchange with its parent).

```bash
> childsupervision checktimeout
30
Done
```

### childsupervision checktimeout \<timeout\>

Set the Child Supervision Check Timeout value on the child.

```bash
> childsupervision checktimeout 30
Done
```

### childsupervision failcounter

Get the current value of supervision check timeout failure counter.

The counter tracks the number of supervision check failures on the child. It is incremented when the child does not hear from its parent within the specified check timeout interval.

```bash
> childsupervision failcounter
0
Done
```

### childsupervision failcounter reset

Reset the supervision check timeout failure counter to zero.

```bash
> childsupervision failcounter reset
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

### coex

Get the coex status.

`OPENTHREAD_CONFIG_PLATFORM_RADIO_COEX_ENABLE` is required.

```bash
> coex
Enabled
Done
```

### coex disable

Disable coex.

`OPENTHREAD_CONFIG_PLATFORM_RADIO_COEX_ENABLE` is required.

```bash
> coex disable
Done
```

### coex enable

Enable coex.

`OPENTHREAD_CONFIG_PLATFORM_RADIO_COEX_ENABLE` is required.

```bash
> coex enable
Done
```

### coex metrics

Show coex metrics.

`OPENTHREAD_CONFIG_PLATFORM_RADIO_COEX_ENABLE` is required.

```bash
> coex metrics
Stopped: false
Grant Glitch: 0
Transmit metrics
    Request: 0
    Grant Immediate: 0
    Grant Wait: 0
    Grant Wait Activated: 0
    Grant Wait Timeout: 0
    Grant Deactivated During Request: 0
    Delayed Grant: 0
    Average Request To Grant Time: 0
Receive metrics
    Request: 0
    Grant Immediate: 0
    Grant Wait: 0
    Grant Wait Activated: 0
    Grant Wait Timeout: 0
    Grant Deactivated During Request: 0
    Delayed Grant: 0
    Average Request To Grant Time: 0
    Grant None: 0
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
br
ip
mac
mle
Done
```

### counters \<countername\>

Get the counter value.

Note:

- `OPENTHREAD_CONFIG_UPTIME_ENABLE` is required for MLE role time tracking in `counters mle`
- `OPENTHREAD_CONFIG_IP6_BR_COUNTERS_ENABLE` is required for `counters br`

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
Time Disabled Milli: 10026
Time Detached Milli: 6852
Time Child Milli: 0
Time Router Milli: 0
Time Leader Milli: 16195
Time Tracked Milli: 33073
Done
> counters ip
TxSuccess: 10
TxFailed: 0
RxSuccess: 5
RxFailed: 0
Done
> counters br
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

CSL period is shown in microseconds.

```bash
> csl
Channel: 11
Period: 160000us
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

Set CSL period in microseconds. Disable CSL by setting this parameter to `0`.

The CSL period MUST be a multiple 160 microseconds which is 802.15.4 "ten symbols time".

```bash
> csl period 30000000
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

### debug

Executes a series of CLI commands to gather information about the device and thread network. This is intended for debugging.

The output will display each executed CLI command preceded by "\$", followed by the corresponding command's generated output.

The generated output encompasses the following information:

- Version
- Current state
- RLOC16, extended MAC address
- Unicast and multicast IPv6 address list
- Channel
- PAN ID and extended PAN ID
- Network Data
- Partition ID
- Leader Data

If the device is operating as FTD:

- Child and neighbor table
- Router table and next hop Info
- Address cache table
- Registered MTD child IPv6 address
- Device properties

If the device supports and acts as an SRP client:

- SRP client state
- SRP client services and host info

If the device supports and acts as an SRP sever:

- SRP server state and address mode
- SRP server registered hosts and services

If the device supports TREL:

- TREL status and peer table

If the device supports and acts as a border router:

- BR state
- BR prefixes (OMR, on-link, NAT64)
- Discovered prefix table

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

### detach

Start the graceful detach process by first notifying other nodes (sending Address Release if acting as a router, or setting Child Timeout value to zero on parent if acting as a child) and then stopping Thread protocol operation.

```bash
> detach
Finished detaching
Done
```

### detach async

Start the graceful detach process similar to the `detach` command without blocking and waiting for the callback indicating that detach is finished.

```bash
> detach async
Done
```

### deviceprops

Get the current device properties.

```bash
> deviceprops
PowerSupply      : external
IsBorderRouter   : yes
SupportsCcm      : no
IsUnstable       : no
WeightAdjustment : 0
Done
```

### deviceprops \<power-supply\> \<is-br\> \<supports-ccm\> \<is-unstable\> \<weight-adjustment\>

Set the device properties which are then used to determine and set the Leader Weight.

- power-supply: `battery`, `external`, `external-stable`, or `external-unstable`.
- weight-adjustment: Valid range is from -16 to +16. Clamped if not within the range.

```bash
> deviceprops battery 0 0 0 -5
Done

> deviceprops
PowerSupply      : battery
IsBorderRouter   : no
SupportsCcm      : no
IsUnstable       : no
WeightAdjustment : -5
Done

> leaderweight
51
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

The config includes

- Server IPv6 address and port
- Response timeout in msec (wait time to rx response)
- Maximum tx attempts before reporting failure
- Boolean flag to indicate whether the server can resolve the query recursively or not.
- Service resolution mode which specifies which records to query. Possible options are:
  - `srv` : Query for SRV record only.
  - `txt` : Query for TXT record only.
  - `srv_txt` : Query for both SRV and TXT records in the same message.
  - `srv_txt_sep`: Query in parallel for SRV and TXT using separate messages.
  - `srv_txt_opt`: Query for TXT/SRV together first, if it fails then query separately.
- Whether to allow/disallow NAT64 address translation during address resolution (requires `OPENTHREAD_CONFIG_DNS_CLIENT_NAT64_ENABLE`)
- Transport protocol UDP or TCP (requires `OPENTHREAD_CONFIG_DNS_CLIENT_OVER_TCP_ENABLE`)

```bash
> dns config
Server: [fd00:0:0:0:0:0:0:1]:1234
ResponseTimeout: 5000 ms
MaxTxAttempts: 2
RecursionDesired: no
ServiceMode: srv_txt_opt
Nat64Mode: allow
TransportProtocol: udp
Done
>
```

### dns config \[DNS server IP\] \[DNS server port\] \[response timeout (ms)\] \[max tx attempts\] \[recursion desired (boolean)\] \[service mode]

Set the default query config.

Service mode specifies which records to query. Possible options are:

- `def` : Use default option.
- `srv` : Query for SRV record only.
- `txt` : Query for TXT record only.
- `srv_txt` : Query for both SRV and TXT records in the same message.
- `srv_txt_sep`: Query in parallel for SRV and TXT using separate messages.
- `srv_txt_opt`: Query for TXT/SRV together first, if it fails then query separately.

To set protocol effectively to tcp `OPENTHREAD_CONFIG_DNS_CLIENT_OVER_TCP_ENABLE` is required.

```bash
> dns config fd00::1 1234 5000 2 0 srv_txt_sep tcp
Done

> dns config
Server: [fd00:0:0:0:0:0:0:1]:1234
ResponseTimeout: 5000 ms
MaxTxAttempts: 2
RecursionDesired: no
ServiceMode: srv_txt_sep
Nat64Mode: allow
TransportProtocol: tcp
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

### dns resolve \<hostname\> \[DNS server IP\] \[DNS server port\] \[response timeout (ms)\] \[max tx attempts\] \[recursion desired (boolean)\] \[transport protocol\]

Send DNS Query to obtain IPv6 address for given hostname.

The parameters after `hostname` are optional. Any unspecified (or zero) value for these optional parameters is replaced by the value from the current default config (`dns config`).

To use tcp, `OPENTHREAD_CONFIG_DNS_CLIENT_OVER_TCP_ENABLE` is required.

```bash
> dns resolve ipv6.google.com
> DNS response for ipv6.google.com - 2a00:1450:401b:801:0:0:0:200e TTL: 300
```

The DNS server IP can be an IPv4 address, which will be synthesized to an IPv6 address using the preferred NAT64 prefix from the network data.

> Note: The command will return `InvalidState` when the DNS server IP is an IPv4 address but the preferred NAT64 prefix is unavailable.

```bash
> dns resolve example.com 8.8.8.8
Synthesized IPv6 DNS server address: fdde:ad00:beef:2:0:0:808:808
DNS response for example.com. - fd4c:9574:3720:2:0:0:5db8:d822 TTL:20456
Done
```

### dns resolve4 \<hostname\> \[DNS server IP\] \[DNS server port\] \[response timeout (ms)\] \[max tx attempts\] \[recursion desired (boolean)\]

Send DNS query to obtain IPv4 address for a given hostname and provide the NAT64 synthesized IPv6 addresses for the IPv4 addresses from the query response.

Requires `OPENTHREAD_CONFIG_DNS_CLIENT_NAT64_ENABLE`.

The parameters after `hostname` are optional. Any unspecified (or zero) value for these optional parameters is replaced by the value from the current default config (`dns config`).

This command requires a NAT64 prefix to be configured and present in Thread Network Data.

For example, if a NAT64 prefix of `2001:db8:122:344::/96` is used within the Thread mesh, the outputted IPv6 address corresponds to an IPv4 address of `142.250.191.78` for the `ipv4.google.com` host:

```bash
> dns resolve4 ipv4.google.com
> DNS response for ipv4.google.com - 2001:db8:122:344:0:0:8efa:bf4e TTL: 20456
```

### dns browse \<service-name\> \[DNS server IP\] \[DNS server port\] \[response timeout (ms)\] \[max tx attempts\] \[recursion desired (boolean)\]

Send a browse (service instance enumeration) DNS query to get the list of services for given service-name.

The parameters after `service-name` are optional. Any unspecified (or zero) value for these optional parameters is replaced by the value from the current default config (`dns config`).

```bash
> dns browse _service._udp.example.com
DNS browse response for _service._udp.example.com.
inst1
inst2
inst3
Done
```

The detailed service info (port number, weight, host name, TXT data, host addresses) is outputted only when provided by server/resolver in the browse response (in additional Data Section). This is a SHOULD and not a MUST requirement, and servers/resolvers are not required to provide this.

The recommended behavior, which is supported by the OpenThread DNS-SD resolver, is to only provide the additional data when there is a single instance in the response. However, users should assume that the browse response may only contain the list of matching service instances and not any detail service info. To resolve a service instance, users can use the `dns service` or `dns servicehost` commands.

```bash
> dns browse _service._udp.example.com
DNS browse response for _service._udp.example.com.
inst1
    Port:1234, Priority:1, Weight:2, TTL:7200
    Host:host.example.com.
    HostAddress:fd00:0:0:0:0:0:0:abcd TTL:7200
    TXT:[a=6531, b=6c12] TTL:7300
Done
```

```bash
> dns browse _airplay._tcp.default.service.arpa
DNS browse response for _airplay._tcp.default.service.arpa.
Gabe's Mac mini
    Port:7000, Priority:0, Weight:0, TTL:10
    Host:Gabes-Mac-mini.default.service.arpa.
    HostAddress:fd97:739d:386a:1:1c2e:d83c:fcbe:9cf4 TTL:10
Done
```

> Note: The DNS server IP can be an IPv4 address, which will be synthesized to an IPv6 address using the preferred NAT64 prefix from the network data. The command will return `InvalidState` when the DNS server IP is an IPv4 address but the preferred NAT64 prefix is unavailable. When testing DNS-SD discovery proxy, the zone is not `local` and instead should be `default.service.arpa`.

### dns service \<service-instance-label\> \<service-name\> \[DNS server IP\] \[DNS server port\] \[response timeout (ms)\] \[max tx attempts\] \[recursion desired (boolean)\]

Send a service instance resolution DNS query for a given service instance. Service instance label is provided first, followed by the service name (note that service instance label can contain dot '.' character).

The parameters after `service-name` are optional. Any unspecified (or zero) value for these optional parameters is replaced by the value from the current default config (`dns config`).

> Note: The DNS server IP can be an IPv4 address, which will be synthesized to an IPv6 address using the preferred NAT64 prefix from the network data. The command will return `InvalidState` when the DNS server IP is an IPv4 address but the preferred NAT64 prefix is unavailable.

### dns servicehost \<service-instance-label\> \<service-name\> \[DNS server IP\] \[DNS server port\] \[response timeout (ms)\] \[max tx attempts\] \[recursion desired (boolean)\]

Send a service instance resolution DNS query for a given service instance with a potential follow-up address resolution for the host name discovered for the service instance (if the server/resolver does not provide AAAA/A records for the host name in the response to SRV query).

Service instance label is provided first, followed by the service name (note that service instance label can contain dot '.' character).

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

Get the Interface Identifier manually specified for Thread Domain Unicast Address on Thread 1.2 device.

```bash
> dua iid
0004000300020001
Done
```

### dua iid \<iid\>

Set the Interface Identifier manually specified for Thread Domain Unicast Address on Thread 1.2 device.

```bash
> dua iid 0004000300020001
Done
```

### dua iid clear

Clear the Interface Identifier manually specified for Thread Domain Unicast Address on Thread 1.2 device.

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

### instanceid

Show OpenThread instance identifier.

```bash
> instanceid
468697314
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

Use `-v` to get more verbose information about the address:

- `origin`: can be `thread`, `slaac`, `dhcp6`, or `manual`, and indicates the origin of the address
- `plen`: prefix length (in bits)
- `preferred`: preferred flag (boolean)
- `valid`: valid flag (boolean)

```bash
> ipaddr -v
fd5e:18fa:f4a5:b8:0:ff:fe00:fc00 origin:thread plen:64 preferred:0 valid:1
fd5e:18fa:f4a5:b8:0:ff:fe00:dc00 origin:thread plen:64 preferred:0 valid:1
fd5e:18fa:f4a5:b8:f8e:5d95:87a0:e82c origin:thread plen:64 preferred:0 valid:1
fe80:0:0:0:4891:b191:e277:8826 origin:thread plen:64 preferred:1 valid:1
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

Set Thread Key Switch Guard Time (in hours) 0 means Thread Key Switch immediately if key index match

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

### linkmetricsmgr disable

Disable the Link Metrics Manager.

`OPENTHREAD_CONFIG_LINK_METRICS_MANAGER_ENABLE` is required.

```bash
> linkmetricsmgr disable
Done
```

### linkmetricsmgr enable

Enable the Link Metrics Manager.

`OPENTHREAD_CONFIG_LINK_METRICS_MANAGER_ENABLE` is required.

```bash
> linkmetricsmgr enable
Done
```

### linkmetricsmgr show

Display the Link Metrics data of all subjects. The subjects are identified by its extended address.

`OPENTHREAD_CONFIG_LINK_METRICS_MANAGER_ENABLE` is required.

```bash

> linkmetricsmgr show
ExtAddr:827aa7f7f63e1234, LinkMargin:80, Rssi:-20
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

### meshdiag topology [ip6-addrs][children]

Discover network topology (list of routers and their connections).

This command requires `OPENTHREAD_CONFIG_MESH_DIAG_ENABLE` and `OPENTHREAD_FTD`.

Parameters are optional and indicate additional items to discover. Can be added in any order.

- `ip6-addrs` to discover the list of IPv6 addresses of every router.
- `children` to discover the child table of every router.

Output lists all discovered routers. Information per router:

- Router ID
- RLOC16
- Extended MAC address
- Thread Version (if known).
- Whether the router is this device is itself (`me`)
- Whether the router is the parent of this device when device is a child (`parent`)
- Whether the router is `leader`
- Whether the router acts as a border router providing external connectivity (`br`)
- List of routers to which this router has a link:
  - `3-links`: Router IDs to which this router has a incoming link with link quality 3
  - `2-links`: Router IDs to which this router has a incoming link with link quality 2
  - `1-links`: Router IDs to which this router has a incoming link with link quality 1
  - If a list if empty, it is omitted in the out.
- If `ip6-addrs`, list of IPv6 addresses of the router
- If `children`, list of all children of the router. Information per child:
  - RLOC16
  - Incoming Link Quality from perspective of parent to child (zero indicates unknown)
  - Child Device mode (`r` rx-on-when-idle, `d` Full Thread Device, `n` Full Network Data, `-` no flags set)
  - Whether the child is this device itself (`me`)
  - Whether the child acts as a border router providing external connectivity (`br`)

Discover network topology:

```bash
> meshdiag topology
id:02 rloc16:0x0800 ext-addr:8aa57d2c603fe16c ver:4 - me - leader
   3-links:{ 46 }
id:46 rloc16:0xb800 ext-addr:fe109d277e0175cc ver:4
   3-links:{ 02 51 57 }
id:33 rloc16:0x8400 ext-addr:d2e511a146b9e54d ver:4
   3-links:{ 51 57 }
id:51 rloc16:0xcc00 ext-addr:9aab43ababf05352 ver:4
   3-links:{ 33 57 }
   2-links:{ 46 }
id:57 rloc16:0xe400 ext-addr:dae9c4c0e9da55ff ver:4
   3-links:{ 46 51 }
   1-links:{ 33 }
Done
```

Discover network topology with router's IPv6 addresses and children:

```bash
> meshdiag topology children ip6-addrs
id:62 rloc16:0xf800 ext-addr:ce349873897233a5 ver:4 - me - br
   3-links:{ 46 }
   ip6-addrs:
       fdde:ad00:beef:0:0:ff:fe00:f800
       fdde:ad00:beef:0:211d:39e9:6b2e:4ad1
       fe80:0:0:0:cc34:9873:8972:33a5
   children: none
id:02 rloc16:0x0800 ext-addr:8aa57d2c603fe16c ver:4 - leader - br
   3-links:{ 46 51 }
   ip6-addrs:
       fdde:ad00:beef:0:0:ff:fe00:fc00
       fdde:ad00:beef:0:0:ff:fe00:800
       fdde:ad00:beef:0:8a36:a3eb:47ae:a9b0
       fe80:0:0:0:88a5:7d2c:603f:e16c
   children:
       rloc16:0x0803 lq:3, mode:rn
       rloc16:0x0804 lq:3, mode:rdn
id:33 rloc16:0x8400 ext-addr:d2e511a146b9e54d ver:4
   3-links:{ 57 }
   ip6-addrs:
       fdde:ad00:beef:0:0:ff:fe00:8400
       fdde:ad00:beef:0:824:a126:cf19:a9f4
       fe80:0:0:0:d0e5:11a1:46b9:e54d
   children: none
id:51 rloc16:0xcc00 ext-addr:9aab43ababf05352 ver:4
   3-links:{ 02 46 57 }
   ip6-addrs:
       fdde:ad00:beef:0:0:ff:fe00:cc00
       fdde:ad00:beef:0:2986:bba3:12d0:1dd2
       fe80:0:0:0:98ab:43ab:abf0:5352
   children: none
id:57 rloc16:0xe400 ext-addr:dae9c4c0e9da55ff ver:4
   3-links:{ 33 51 }
   ip6-addrs:
       fdde:ad00:beef:0:0:ff:fe00:e400
       fdde:ad00:beef:0:87d0:550:bc18:9920
       fe80:0:0:0:d8e9:c4c0:e9da:55ff
   children:
       rloc16:0xe402 lq:3, mode:rn - br
       rloc16:0xe403 lq:3, mode:rn
id:46 rloc16:0xb800 ext-addr:fe109d277e0175cc ver:4
   3-links:{ 02 51 62 }
   ip6-addrs:
       fdde:ad00:beef:0:0:ff:fe00:b800
       fdde:ad00:beef:0:df4d:2994:d85c:c337
       fe80:0:0:0:fc10:9d27:7e01:75cc
   children: none
Done
```

Discover network topology with children:

```bash
> meshdiag topology children
id:02 rloc16:0x0800 ext-addr:8aa57d2c603fe16c ver:4 - parent - leader - br
   3-links:{ 46 51 }
   children:
       rloc16:0x0803 lq:0, mode:rn
       rloc16:0x0804 lq:0, mode:rdn - me
id:46 rloc16:0xb800 ext-addr:fe109d277e0175cc ver:4
   3-links:{ 02 51 62 }
   children: none
id:33 rloc16:0x8400 ext-addr:d2e511a146b9e54d ver:4
   3-links:{ 57 }
   children: none
id:51 rloc16:0xcc00 ext-addr:9aab43ababf05352 ver:4
   3-links:{ 02 46 57 }
   children: none
id:57 rloc16:0xe400 ext-addr:dae9c4c0e9da55ff ver:4
   3-links:{ 33 51 }
   children:
       rloc16:0xe402 lq:3, mode:rn - br
       rloc16:0xe403 lq:3, mode:rn
id:62 rloc16:0xf800 ext-addr:ce349873897233a5 ver:4 - br
   3-links:{ 46 }
   children: none
```

### meshdiag childtable \<router-rloc16\>

Start a query for child table of a router with a given RLOC16.

Output lists all child entries. Information per child:

- RLOC16
- Extended MAC address
- Thread Version
- Timeout (in seconds)
- Age (seconds since last heard)
- Supervision interval (in seconds)
- Number of queued messages (in case the child is sleepy)
- Device Mode
- RSS (average and last) and link margin
- Error rates, frame tx (at MAC layer), IPv6 message tx (above MAC)
- Connection time (seconds since link establishment {dd}d.{hh}:{mm}:{ss} format)
- CSL info
  - If synchronized
  - Period (in unit of 10-symbols-time)
  - Timeout (in seconds)
  - Channel

```bash
> meshdiag childtable 0x6400
rloc16:0x6402 ext-addr:8e6f4d323bbed1fe ver:4
    timeout:120 age:36 supvn:129 q-msg:0
    rx-on:yes type:ftd full-net:yes
    rss - ave:-20 last:-20 margin:80
    err-rate - frame:11.51% msg:0.76%
    conn-time:00:11:07
    csl - sync:no period:0 timeout:0 channel:0
rloc16:0x6403 ext-addr:ee24e64ecf8c079a ver:4
    timeout:120 age:19 supvn:129 q-msg:0
    rx-on:no type:mtd full-net:no
    rss - ave:-20 last:-20 margin:80
    err-rate - frame:0.73% msg:0.00%
    conn-time:01:08:53
    csl - sync:no period:0 timeout:0 channel:0
Done
```

### meshdiag childip6 \<parent-rloc16\>

Send a query to a parent to retrieve the IPv6 addresses of all its MTD children.

```bash
> meshdiag childip6 0xdc00
child-rloc16: 0xdc02
    fdde:ad00:beef:0:ded8:cd58:b73:2c21
    fd00:2:0:0:c24a:456:3b6b:c597
    fd00:1:0:0:120b:95fe:3ecc:d238
child-rloc16: 0xdc03
    fdde:ad00:beef:0:3aa6:b8bf:e7d6:eefe
    fd00:2:0:0:8ff8:a188:7436:6720
    fd00:1:0:0:1fcf:5495:790a:370f
Done
```

### meshdiag routerneighbortable \<router-rloc16\>

Start a query for router neighbor table of a router with a given RLOC16.

Output lists all router neighbor entries. Information per entry:

- RLOC16
- Extended MAC address
- Thread Version
- RSS (average and last) and link margin
- Error rates, frame tx (at MAC layer), IPv6 message tx (above MAC)
- Connection time (seconds since link establishment {dd}d.{hh}:{mm}:{ss} format)

```bash
> meshdiag routerneighbortable 0x7400
rloc16:0x9c00 ext-addr:764788cf6e57a4d2 ver:4
   rss - ave:-20 last:-20 margin:80
   err-rate - frame:1.38% msg:0.00%
   conn-time:01:54:02
rloc16:0x7c00 ext-addr:4ed24fceec9bf6d3 ver:4
   rss - ave:-20 last:-20 margin:80
   err-rate - frame:0.72% msg:0.00%
   conn-time:00:11:27
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

### nat64 cidr

Gets the IPv4 configured CIDR in the NAT64 translator.

`OPENTHREAD_CONFIG_NAT64_TRANSLATOR_ENABLE` is required.

```bash
> nat64 cidr
192.168.255.0/24
Done
```

### nat64 cidr \<IPv4 address\>

Sets the IPv4 CIDR in the NAT64 translator.

Note:

- `OPENTHREAD_CONFIG_NAT64_TRANSLATOR_ENABLE` is required.
- A valid CIDR must have a non-zero prefix length.
- When updating the CIDR, NAT64 translator will be reset and all existing sessions will be expired.

```bash
> nat64 cidr 192.168.100.0/24
Done
```

### nat64 disable

Disable NAT64 functions, including the translator and the prefix publishing.

This command will reset the mapping table in the translator (if NAT64 translator is enabled in the build).

`OPENTHREAD_CONFIG_NAT64_TRANSLATOR_ENABLE` or `OPENTHREAD_CONFIG_NAT64_BORDER_ROUTING_ENABLE` are required.

```bash
> nat64 disable
Done
```

### nat64 enable

Enable NAT64 functions, including the translator and the prefix publishing.

This command can be called anytime.

`OPENTHREAD_CONFIG_NAT64_TRANSLATOR_ENABLE` or `OPENTHREAD_CONFIG_NAT64_BORDER_ROUTING_ENABLE` are required.

```bash
> nat64 enable
Done
```

### nat64 state

Gets the state of NAT64 functions.

Possible results for prefix manager are (`OPENTHREAD_CONFIG_NAT64_BORDER_ROUTING_ENABLE` is required):

- `Disabled`: NAT64 prefix manager is disabled.
- `NotRunning`: NAT64 prefix manager is enabled, but is not running, probably because the routing manager is disabled.
- `Idle`: NAT64 prefix manager is enabled and is running, but is not publishing a NAT64 prefix. Usually when there is another border router publishing a NAT64 prefix with higher priority.
- `Active`: NAT64 prefix manager is enabled, running and publishing a NAT64 prefix.

Possible results for NAT64 translator are (`OPENTHREAD_CONFIG_NAT64_TRANSLATOR_ENABLE` is required):

- `Disabled`: NAT64 translator is disabled.
- `NotRunning`: NAT64 translator is enabled, but is not translating packets, probably because it is not configured with a NAT64 prefix or a CIDR for NAT64.
- `Active`: NAT64 translator is enabled and is translating packets.

`OPENTHREAD_CONFIG_NAT64_TRANSLATOR_ENABLE` or `OPENTHREAD_CONFIG_NAT64_BORDER_ROUTING_ENABLE` are required.

```bash
> nat64 state
PrefixManager: NotRunning
Translator:    NotRunning
Done

> nat64 state
PrefixManager: Idle
Translator:    NotRunning
Done

> nat64 state
PrefixManager: Active
Translator:    Active
Done
```

### nat64 mappings

Get the NAT64 translator mappings.

`OPENTHREAD_CONFIG_NAT64_TRANSLATOR_ENABLE` is required.

```bash
> nat64 mappings
|          | Address                   |        | 4 to 6       | 6 to 4       |
+----------+---------------------------+--------+--------------+--------------+
| ID       | IPv6       | IPv4         | Expiry | Pkts | Bytes | Pkts | Bytes |
+----------+------------+--------------+--------+------+-------+------+-------+
| 00021cb9 | fdc7::df79 | 192.168.64.2 |  7196s |    6 |   456 |   11 |  1928 |
|          |                                TCP |    0 |     0 |    0 |     0 |
|          |                                UDP |    1 |   136 |   16 |  1608 |
|          |                               ICMP |    5 |   320 |    5 |   320 |
```

### nat64 counters

Get the NAT64 translator packet and error counters.

`OPENTHREAD_CONFIG_NAT64_TRANSLATOR_ENABLE` is required.

```bash
> nat64 counters
|               | 4 to 6                  | 6 to 4                  |
+---------------+-------------------------+-------------------------+
| Protocol      | Pkts     | Bytes        | Pkts     | Bytes        |
+---------------+----------+--------------+----------+--------------+
|         Total |       11 |          704 |       11 |          704 |
|           TCP |        0 |            0 |        0 |            0 |
|           UDP |        0 |            0 |        0 |            0 |
|          ICMP |       11 |          704 |       11 |          704 |
| Errors        | Pkts                    | Pkts                    |
+---------------+-------------------------+-------------------------+
|         Total |                       8 |                       4 |
|   Illegal Pkt |                       0 |                       0 |
|   Unsup Proto |                       0 |                       0 |
|    No Mapping |                       2 |                       0 |
Done
```

### neighbor linkquality

Print link quality info for all neighbors.

```bash
> neighbor linkquality
| RLOC16 | Extended MAC     | Frame Error | Msg Error | Avg RSS | Last RSS | Age   |
+--------+------------------+-------------+-----------+---------+----------+-------+
| 0xe800 | 9e2fa4e1b84f92db |      0.00 % |    0.00 % |     -46 |      -48 |     1 |
| 0xc001 | 0ad7ed6beaa6016d |      4.67 % |    0.08 % |     -68 |      -72 |    10 |
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

### neighbor conntime

Print connection time and age of neighbors.

The table provides the following info per neighbor:

- RLOC16
- Extended MAC address
- Age (seconds since last heard from neighbor)
- Connection time (seconds since link establishment with neighbor)

Duration intervals are formatted as `<hh>:<mm>:<ss>` for hours, minutes, and seconds if the duration is less than one day. If the duration is longer than one day, the format is `<dd>d.<hh>:<mm>:<ss>`.

```bash
> neighbor conntime
| RLOC16 | Extended MAC     | Last Heard (Age) | Connection Time  |
+--------+------------------+------------------+------------------+
| 0x8401 | 1a28be396a14a318 |         00:00:13 |         00:07:59 |
| 0x5c00 | 723ebf0d9eba3264 |         00:00:03 |         00:11:27 |
| 0xe800 | ce53628a1e3f5b3c |         00:00:02 |         00:00:15 |
Done
```

### neighbor conntime list

Print connection time and age of neighbors.

This command is similar to `neighbor conntime`, but it displays the information in a list format. The age and connection time are both displayed in seconds.

```bash
> neighbor conntime list
0x8401 1a28be396a14a318 age:63 conn-time:644
0x5c00 723ebf0d9eba3264 age:23 conn-time:852
0xe800 ce53628a1e3f5b3c age:23 conn-time:180
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

Note: When operating as a Thread Router when `OPENTHREAD_CONFIG_REFERENCE_DEVICE_ENABLE` is enabled, this command will return the cached information from when the device was previously attached as a Thread Child. Returning cached information is necessary to support the Thread Test Harness - Test Scenario 8.2.x requests the former parent (i.e. Joiner Router's) MAC address even if the device has already promoted to a router.

```bash
> parent
Ext Addr: be1857c6c21dce55
Rloc: 5c00
Link Quality In: 3
Link Quality Out: 3
Age: 20
Version: 4
Done
```

Note: When `OPENTHREAD_CONFIG_MAC_CSL_RECEIVER_ENABLE` is enabled, this command will return two extra lines with information relevant for CSL Receiver operation.

```bash
CSL clock accuracy: 20
CSL uncertainty: 5
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

### ping \[async\] \[-I source\] \[-m] \<ipaddr\> \[size\] \[count\] \[interval\] \[hoplimit\] \[timeout\]

Send an ICMPv6 Echo Request.

- async: Use the non-blocking mode. New commands are allowed before the ping process terminates.
- source: The source IPv6 address of the echo request.
- -m: multicast loop, which allows looping back pings to multicast addresses that the device itself is subscribed to.
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

The address can be an IPv4 address, which will be synthesized to an IPv6 address using the preferred NAT64 prefix from the network data.

> Note: The command will return `InvalidState` when the preferred NAT64 prefix is unavailable.

```bash
> ping 172.17.0.1
Pinging synthesized IPv6 address: fdde:ad00:beef:2:0:0:ac11:1
> 16 bytes from fdde:ad00:beef:2:0:0:ac11:1: icmp_seq=5 hlim=64 time=0ms
1 packets transmitted, 1 packets received. Packet loss = 0.0%. Round-trip min/avg/max = 0/0.0/0 ms.
Done
```

### ping stop

Stop sending ICMPv6 Echo Requests.

```bash
> ping stop
Done
```

### platform

Print the current platform

```bash
> platform
NRF52840
Done
```

### pollperiod

Get the customized data poll period of sleepy end device (milliseconds). Only for certification test.

```bash
> pollperiod
0
Done
```

### pollperiod \<pollperiod\>

Set the customized data poll period for sleepy end device (milliseconds >= 10ms). Only for certification test.

```bash
> pollperiod 10
Done
```

### pskc

Get pskc in hex format.

```bash
> pskc
00000000000000000000000000000000
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

### pskcref

Get pskc key reference.

`OPENTHREAD_CONFIG_PLATFORM_KEY_REFERENCES_ENABLE` is required.

```bash
> pskcref
0x80000000
Done
```

### pskcref \<keyref\>

Set pskc key reference as \<keyref\>.

`OPENTHREAD_CONFIG_PLATFORM_KEY_REFERENCES_ENABLE` is required.

```bash
> pskcref 0x20017
Done
```

### preferrouterid \<routerid\>

Prefer a Router ID when solicit router id from Leader.

```bash
> preferrouterid 16
Done
```

### prefix

Get the prefix list in the local Network Data. Note: For the Thread 1.2 border router with backbone capability, the local Domain Prefix would be listed as well (with flag `D`), with preceding `-` if backbone functionality is disabled.

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

### radio enable

Enable radio.

```bash
> radio enable
Done
```

### radio disable

Disable radio.

```bash
> radio disable
Done
```

### radio stats

`OPENTHREAD_CONFIG_RADIO_STATS_ENABLE` is required. This feature is only available on FTD and MTD.

The radio statistics shows the time when the radio is in sleep/tx/rx state. The command will show the time of these states since last reset in unit of microseconds. It will also show the percentage of the time.

```bash
> radio stats
Radio Statistics:
Total Time: 67.756s
Tx Time: 0.022944s (0.03%)
Rx Time: 1.482353s (2.18%)
Sleep Time: 66.251128s (97.77%)
Disabled Time: 0.000080s (0.00%)
Done
```

### radio stats clear

`OPENTHREAD_CONFIG_RADIO_STATS_ENABLE` is required. This feature is only available on FTD and MTD.

This command resets the radio statistics. It sets all the time to 0.

```bash
> radio stats clear
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

### reset bootloader

Signal a platform reset to bootloader mode, if supported.

Requires `OPENTHREAD_CONFIG_PLATFORM_BOOTLOADER_MODE_ENABLE`.

```bash
> reset bootloader
Done
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

### route add \<prefix\> [sna][prf]

Add a valid external route to the Network Data.

- s: Stable flag
- n: NAT64 flag
- a: Advertising PIO (AP) flag
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

### childrouterlinks

Get the MLE_CHILD_ROUTER_LINKS value.

```bash
> childrouterlinks
16
Done
```

### childrouterlinks \<number_of_links\>

Set the MLE_CHILD_ROUTER_LINKS value.

```bash
> childrouterlinks 16
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

### timeinqueue

Print the tx queue time-in-queue histogram.

Requires `OPENTHREAD_CONFIG_TX_QUEUE_STATISTICS_ENABLE`.

The time-in-queue is tracked for direct transmissions only and is measured as the duration from when a message is added to the transmit queue until it is passed to the MAC layer for transmission or dropped.

Each table row shows min and max time-in-queue (in milliseconds) followed by number of messages with time-in-queue within the specified min-max range. The histogram information is collected since the OpenThread instance was initialized or since the last time statistics collection was reset by the `timeinqueue reset` command.

The collected statistics can be reset by `timeinqueue reset`.

```bash
> timeinqueue
| Min  | Max  |Msg Count|
+------+------+---------+
|    0 |    9 |    1537 |
|   10 |   19 |     156 |
|   20 |   29 |      57 |
|   30 |   39 |     108 |
|   40 |   49 |      60 |
|   50 |   59 |      76 |
|   60 |   69 |      88 |
|   70 |   79 |      51 |
|   80 |   89 |      86 |
|   90 |   99 |      45 |
|  100 |  109 |      43 |
|  110 |  119 |      44 |
|  120 |  129 |      38 |
|  130 |  139 |      44 |
|  140 |  149 |      35 |
|  150 |  159 |      41 |
|  160 |  169 |      34 |
|  170 |  179 |      13 |
|  180 |  189 |      24 |
|  190 |  199 |       3 |
|  200 |  209 |       0 |
|  210 |  219 |       0 |
|  220 |  229 |       2 |
|  230 |  239 |       0 |
|  240 |  249 |       0 |
|  250 |  259 |       0 |
|  260 |  269 |       0 |
|  270 |  279 |       0 |
|  280 |  289 |       0 |
|  290 |  299 |       1 |
|  300 |  309 |       0 |
|  310 |  319 |       0 |
|  320 |  329 |       0 |
|  330 |  339 |       0 |
|  340 |  349 |       0 |
|  350 |  359 |       0 |
|  360 |  369 |       0 |
|  370 |  379 |       0 |
|  380 |  389 |       0 |
|  390 |  399 |       0 |
|  400 |  409 |       0 |
|  410 |  419 |       0 |
|  420 |  429 |       0 |
|  430 |  439 |       0 |
|  440 |  449 |       0 |
|  450 |  459 |       0 |
|  460 |  469 |       0 |
|  470 |  479 |       0 |
|  480 |  489 |       0 |
|  490 |  inf |       0 |
Done
```

### timeinqueue max

Print the maximum observed time-in-queue in milliseconds.

Requires `OPENTHREAD_CONFIG_TX_QUEUE_STATISTICS_ENABLE`.

The time-in-queue is tracked for direct transmissions only and is measured as the duration from when a message is added to the transmit queue until it is passed to the MAC layer for transmission or dropped.

```bash
> timeinqueue max
291
```

### timeinqueue reset

Reset the TX queue time-in-queue statistics.

```bash
> timeinqueue reset
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

### vendor name

Get the vendor name.

```bash
> vendor name
nest
Done
```

Set the vendor name (requires `OPENTHREAD_CONFIG_NET_DIAG_VENDOR_INFO_SET_API_ENABLE`).

```bash
> vendor name nest
Done
```

### vendor model

Get the vendor model.

```bash
> vendor model
Hub Max
Done
```

Set the vendor model (requires `OPENTHREAD_CONFIG_NET_DIAG_VENDOR_INFO_SET_API_ENABLE`).

```bash
> vendor model Hub\ Max
Done
```

### vendor swversion

Get the vendor SW version.

```bash
> vendor swversion
Marble3.5.1
Done
```

Set the vendor SW version (requires `OPENTHREAD_CONFIG_NET_DIAG_VENDOR_INFO_SET_API_ENABLE`).

```bash
> vendor swversion Marble3.5.1
Done
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
Default rss: -50 (lqi 3)
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

Add an IEEE 802.15.4 Extended Address to the address filter, and fixed the received signal strength for the messages from the address if rss is specified.

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
> macfilter rss add-lqi 0f6127e33af6b404 2
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
