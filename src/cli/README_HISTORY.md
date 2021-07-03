# OpenThread CLI - History Tracker

History Tracker module records history of different events (e.g., RX and TX IPv6 messages or network info changes, etc.) as the Thread network operates. All tracked entries are timestamped.

All commands under `history` require `OPENTHREAD_CONFIG_HISTORY_TRACKER_ENABLE` feature to be enabled.

The number of entries recorded for each history list is configurable through a set of OpenThread config options, e.g. `OPENTHREAD_CONFIG_HISTORY_TRACKER_NET_INFO_LIST_SIZE` specifies the number of entries in Network Info history list. The History Tracker will keep the most recent entries overwriting oldest one when the list gets full.

## Command List

Usage : `history [command] ...`

- [help](#help)
- [netinfo](#netinfo)
- [rx](#rx)
- [tx](#tx)

## Timestamp Format

Recorded entries are timestamped. When the history list is printed, the timestamps are shown relative the time the command was issues (i.e., when the list was printed) indicating how long ago the entry was recorded.

```bash
> history netinfo
| Time                     | Role     | Mode | RLOC16 | Partition ID |
+--------------------------+----------+------+--------+--------------+
|         02:31:50.628 ago | leader   | rdn  | 0x2000 |    151029327 |
|         02:31:53.262 ago | detached | rdn  | 0xfffe |            0 |
|         02:31:54.663 ago | detached | rdn  | 0x2000 |            0 |
Done
```

For example `02:31:50.628 ago` indicates the event was recorded "2 hours, 31 minutes, 50 seconds, and 628 milliseconds ago". Number of days is added for events that are older than 24 hours, e.g., `1 day 11:25:31.179 ago`, or `31 days 03:00:23.931 ago`.

Timestamps use millisecond accuracy and are tacked up to 49 days. If the event is older than 49 days, the entry is still tracked in the list but the timestamp is shown as `more than 49 days ago`.

## Command Details

### help

Usage: `history help`

Print SRP client help menu.

```bash
> history help
help
netinfo
rx
tx
Done
>
```

### netinfo

Usage `history netinfo [list] [<num-entries>]`

Print the Network Info history. Each Network Info provides

- Device Role
- MLE Link Mode
- RLOC16
- Partition ID

Print the Network Info history as a table.

```bash
> history netinfo
| Time                     | Role     | Mode | RLOC16 | Partition ID |
+--------------------------+----------+------+--------+--------------+
|         00:00:10.069 ago | router   | rdn  | 0x6000 |    151029327 |
|         00:02:09.337 ago | child    | rdn  | 0x2001 |    151029327 |
|         00:02:09.338 ago | child    | rdn  | 0x2001 |    151029327 |
|         00:07:40.806 ago | child    | -    | 0x2001 |    151029327 |
|         00:07:42.297 ago | detached | -    | 0x6000 |            0 |
|         00:07:42.968 ago | disabled | -    | 0x6000 |            0 |
Done
```

Print the Network Info history as a list.

```bash
> history netinfo list
00:00:59.467 ago -> role:router mode:rdn rloc16:0x6000 partition-id:151029327
00:02:58.735 ago -> role:child mode:rdn rloc16:0x2001 partition-id:151029327
00:02:58.736 ago -> role:child mode:rdn rloc16:0x2001 partition-id:151029327
00:08:30.204 ago -> role:child mode:- rloc16:0x2001 partition-id:151029327
00:08:31.695 ago -> role:detached mode:- rloc16:0x6000 partition-id:0
00:08:32.366 ago -> role:disabled mode:- rloc16:0x6000 partition-id:0
Done
```

Print only the latest 2 entries.

```bash
> history netinfo 2
| Time                     | Role     | Mode | RLOC16 | Partition ID |
+--------------------------+----------+------+--------+--------------+
|         00:02:05.451 ago | router   | rdn  | 0x6000 |    151029327 |
|         00:04:04.719 ago | child    | rdn  | 0x2001 |    151029327 |
Done
```

### rx

Usage `history rx [list] [<num-entries>]`

Print the IPv6 message RX history. Each entry provides:

- IPv6 message type: UDP, TCP, ICMP6 (and its subtype), etc.
- IPv6 payload length (excludes the IPv6 header).
- Source IPv6 address and port number.
- Destination IPv6 address and port number (port number is valid for UDP/TCP, it is zero otherwise).
- Whether or not link-layer security was used.
- Message priority: low, norm, high, net (for Thread Control traffic).
- RSS: Received Signal Strength (in dBm) - averaged over all received fragment frames that formed the message.
- Short address (RLOC16) of neighbor from which message was received (`0xfffe` if not known).
- Radio link on which the message was received (useful when `OPENTHREAD_CONFIG_MULTI_RADIO` is enabled).

Print the IPv6 message RX history as a table:

```bash
> history rx
| Time                     | Type             | Len   | Sec | Prio | RSS  | From   | Radio |
+--------------------------+------------------+-------+-----+------+------+--------+-------+
|                          | ICMP6(EchoReply) |    16 | yes | norm |  -20 | 0x2000 |  15.4 |
|         00:00:07.155 ago | src: [fd00:db8:0:0:c312:43a7:cd14:4634]:0                     |
|                          | dst: [fd00:db8:0:0:a127:7275:f3d:e723]:0                      |
+--------------------------+------------------+-------+-----+------+------+--------+-------+
|                          | UDP              |    51 |  no |  net |  -20 | 0x2000 |  15.4 |
|         00:00:08.018 ago | src: [fe80:0:0:0:146e:a00:0:1]:19788                          |
|                          | dst: [ff02:0:0:0:0:0:0:1]:19788                               |
+--------------------------+------------------+-------+-----+------+------+--------+-------+
|                          | ICMP6(EchoReqst) |    16 | yes | norm |  -20 | 0x2000 |  15.4 |
|         00:00:18.348 ago | src: [fd00:db8:0:0:c312:43a7:cd14:4634]:0                     |
|                          | dst: [fd00:db8:0:0:a127:7275:f3d:e723]:0                      |
+--------------------------+------------------+-------+-----+------+------+--------+-------+
|                          | ICMP6(EchoReqst) |    16 | yes | norm |  -20 | 0x2000 |  15.4 |
|         00:00:20.215 ago | src: [fd00:db8:0:0:c312:43a7:cd14:4634]:0                     |
|                          | dst: [fd00:db8:0:0:a127:7275:f3d:e723]:0                      |
+--------------------------+------------------+-------+-----+------+------+--------+-------+
|                          | HopOpts          |    44 | yes | norm |  -20 | 0x2000 |  15.4 |
|         00:00:22.250 ago | src: [fd00:db8:0:0:0:ff:fe00:2000]:0                          |
|                          | dst: [ff03:0:0:0:0:0:0:2]:0                                   |
+--------------------------+------------------+-------+-----+------+------+--------+-------+
|                          | UDP              |    12 | yes |  net |  -20 | 0x2000 |  15.4 |
|         00:00:22.268 ago | src: [fd00:db8:0:0:0:ff:fe00:2000]:61631                      |
|                          | dst: [fd00:db8:0:0:0:ff:fe00:6000]:61631                      |
+--------------------------+------------------+-------+-----+------+------+--------+-------+
|                          | ICMP6(EchoReqst) |    16 | yes | norm |  -20 | 0x2000 |  15.4 |
|         00:00:22.268 ago | src: [fd00:db8:0:0:c312:43a7:cd14:4634]:0                     |
|                          | dst: [fd00:db8:0:0:a127:7275:f3d:e723]:0                      |
+--------------------------+------------------+-------+-----+------+------+--------+-------+
|                          | HopOpts          |    44 | yes | norm |  -20 | 0x2000 |  15.4 |
|         00:00:22.271 ago | src: [fd00:db8:0:0:0:ff:fe00:2000]:0                          |
|                          | dst: [ff03:0:0:0:0:0:0:2]:0                                   |
+--------------------------+------------------+-------+-----+------+------+--------+-------+
|                          | UDP              |    51 |  no |  net |  -20 | 0x2000 |  15.4 |
|         00:00:36.524 ago | src: [fe80:0:0:0:146e:a00:0:1]:19788                          |
|                          | dst: [ff02:0:0:0:0:0:0:1]:19788                               |
Done
```

Print the latest 5 entries of the IPv6 message RX history as a list:

```bash
> history rx list 5
00:00:14.011 ago
    type:ICMP6(EchoReply) len:16 sec:yes prio:norm rss:-20 from:0x2000 radio:15.4
    src:[fd00:db8:0:0:c312:43a7:cd14:4634]:0
    dst:[fd00:db8:0:0:a127:7275:f3d:e723]:0
00:00:14.874 ago
    type:UDP len:51 sec:no prio:net rss:-20 from:0x2000 radio:15.4
    src:[fe80:0:0:0:146e:a00:0:1]:19788
    dst:[ff02:0:0:0:0:0:0:1]:19788
00:00:25.204 ago
    type:ICMP6(EchoReqst) len:16 sec:yes prio:norm rss:-20 from:0x2000 radio:15.4
    src:[fd00:db8:0:0:c312:43a7:cd14:4634]:0
    dst:[fd00:db8:0:0:a127:7275:f3d:e723]:0
00:00:27.071 ago
    type:ICMP6(EchoReqst) len:16 sec:yes prio:norm rss:-20 from:0x2000 radio:15.4
    src:[fd00:db8:0:0:c312:43a7:cd14:4634]:0
    dst:[fd00:db8:0:0:a127:7275:f3d:e723]:0
00:00:29.106 ago
    type:HopOpts len:44 sec:yes prio:norm rss:-20 from:0x2000 radio:15.4
    src:[fd00:db8:0:0:0:ff:fe00:2000]:0
    dst:[ff03:0:0:0:0:0:0:2]:0
Done

```

### tx

Usage `history tx [list] [<num-entries>]`

Print the IPv6 message TX history. Each entry provides:

- IPv6 message type: UDP, TCP, ICMP6 (and its subtype), etc.
- IPv6 payload length (excludes the IPv6 header).
- Source IPv6 address and port number.
- Destination IPv6 address and port number (port number is valid for UDP/TCP, it is zero otherwise).
- Whether or not link-layer security was used.
- Message priority: low, norm, high, net (for Thread control messages).
- Whether or not the message tx was successful (e.g., all fragments acked by received if ack-request was enabled).
- RSS: Received Signal Strength (in dBm) - averaged over all received fragment frames that formed the message.
- Short address (RLOC16) of neighbor to which the message was sent (`0xffff` if it was broadcast and `0xfffe` if short address is not known).
- Radio link on which the message was sent (useful when `OPENTHREAD_CONFIG_MULTI_RADIO` is enabled).

Print the IPv6 message TX history as a table (10 latest entries):

```bash
> history tx 10
| Time                     | Type             | Len   | Sec | Prio | Succ | To     | Radio |
+--------------------------+------------------+-------+-----+------+------+--------+-------+
|                          | UDP              |    51 |  no |  net |  yes | 0xffff |  15.4 |
|         00:00:20.114 ago | src: [fe80:0:0:0:146e:a00:0:1]:19788                          |
|                          | dst: [ff02:0:0:0:0:0:0:1]:19788                               |
+--------------------------+------------------+-------+-----+------+------+--------+-------+
|                          | ICMP6(EchoReply) |    16 | yes | norm |  yes | 0x6000 |  15.4 |
|         00:00:31.014 ago | src: [fd00:db8:0:0:c312:43a7:cd14:4634]:0                     |
|                          | dst: [fd00:db8:0:0:a127:7275:f3d:e723]:0                      |
+--------------------------+------------------+-------+-----+------+------+--------+-------+
|                          | ICMP6(EchoReply) |    16 | yes | norm |  yes | 0x6000 |  15.4 |
|         00:00:36.011 ago | src: [fd00:db8:0:0:c312:43a7:cd14:4634]:0                     |
|                          | dst: [fd00:db8:0:0:a127:7275:f3d:e723]:0                      |
+--------------------------+------------------+-------+-----+------+------+--------+-------+
|                          | UDP              |    51 |  no |  net |  yes | 0xffff |  15.4 |
|         00:00:50.050 ago | src: [fe80:0:0:0:146e:a00:0:1]:19788                          |
|                          | dst: [ff02:0:0:0:0:0:0:1]:19788                               |
+--------------------------+------------------+-------+-----+------+------+--------+-------+
|                          | UDP              |    51 |  no |  net |  yes | 0xffff |  15.4 |
|         00:01:22.928 ago | src: [fe80:0:0:0:146e:a00:0:1]:19788                          |
|                          | dst: [ff02:0:0:0:0:0:0:1]:19788                               |
+--------------------------+------------------+-------+-----+------+------+--------+-------+
|                          | UDP              |    51 |  no |  net |  yes | 0xffff |  15.4 |
|         00:02:00.035 ago | src: [fe80:0:0:0:146e:a00:0:1]:19788                          |
|                          | dst: [ff02:0:0:0:0:0:0:1]:19788                               |
+--------------------------+------------------+-------+-----+------+------+--------+-------+
|                          | UDP              |    51 |  no |  net |  yes | 0xffff |  15.4 |
|         00:02:30.467 ago | src: [fe80:0:0:0:146e:a00:0:1]:19788                          |
|                          | dst: [ff02:0:0:0:0:0:0:1]:19788                               |
+--------------------------+------------------+-------+-----+------+------+--------+-------+
|                          | UDP              |    51 |  no |  net |  yes | 0xffff |  15.4 |
|         00:02:54.857 ago | src: [fe80:0:0:0:146e:a00:0:1]:19788                          |
|                          | dst: [ff02:0:0:0:0:0:0:1]:19788                               |
+--------------------------+------------------+-------+-----+------+------+--------+-------+
|                          | UDP              |    51 |  no |  net |  yes | 0xffff |  15.4 |
|         00:03:34.186 ago | src: [fe80:0:0:0:146e:a00:0:1]:19788                          |
|                          | dst: [ff02:0:0:0:0:0:0:1]:19788                               |
+--------------------------+------------------+-------+-----+------+------+--------+-------+
|                          | UDP              |    51 |  no |  net |  yes | 0xffff |  15.4 |
|         00:04:06.502 ago | src: [fe80:0:0:0:146e:a00:0:1]:19788                          |
|                          | dst: [ff02:0:0:0:0:0:0:1]:19788                               |
Done
```

Print the IPv6 message RX history as a list (7 latest entries):

```bash
> history tx list 7
00:00:11.115 ago
    type:UDP len:51 sec:no prio:net tx-success:yes to:0xffff radio:15.4
    src:[fe80:0:0:0:146e:a00:0:1]:19788
    dst:[ff02:0:0:0:0:0:0:1]:19788
00:00:22.015 ago
    type:ICMP6(EchoReply) len:16 sec:yes prio:norm tx-success:yes to:0x6000 radio:15.4
    src:[fd00:db8:0:0:c312:43a7:cd14:4634]:0
    dst:[fd00:db8:0:0:a127:7275:f3d:e723]:0
00:00:27.012 ago
    type:ICMP6(EchoReply) len:16 sec:yes prio:norm tx-success:yes to:0x6000 radio:15.4
    src:[fd00:db8:0:0:c312:43a7:cd14:4634]:0
    dst:[fd00:db8:0:0:a127:7275:f3d:e723]:0
00:00:41.051 ago
    type:UDP len:51 sec:no prio:net tx-success:yes to:0xffff radio:15.4
    src:[fe80:0:0:0:146e:a00:0:1]:19788
    dst:[ff02:0:0:0:0:0:0:1]:19788
00:01:13.929 ago
    type:UDP len:51 sec:no prio:net tx-success:yes to:0xffff radio:15.4
    src:[fe80:0:0:0:146e:a00:0:1]:19788
    dst:[ff02:0:0:0:0:0:0:1]:19788
00:01:51.036 ago
    type:UDP len:51 sec:no prio:net tx-success:yes to:0xffff radio:15.4
    src:[fe80:0:0:0:146e:a00:0:1]:19788
    dst:[ff02:0:0:0:0:0:0:1]:19788
00:02:21.468 ago
    type:UDP len:51 sec:no prio:net tx-success:yes to:0xffff radio:15.4
    src:[fe80:0:0:0:146e:a00:0:1]:19788
    dst:[ff02:0:0:0:0:0:0:1]:19788
Done
```
