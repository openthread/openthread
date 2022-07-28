# OpenThread Factory Diagnostics Module Reference

The OpenThread diagnostics module is a tool for debugging platform hardware manually, and it will also be used during manufacturing process, to verify platform hardware performance.

The diagnostics module supports common diagnostics features that are listed below, and it also provides a mechanism for expanding platform specific diagnostics features.

## Common Diagnostics Command List

- [diag](#diag)
- [diag start](#diag-start)
- [diag channel](#diag-channel)
- [diag power](#diag-power)
- [diag send](#diag-send-packets-length)
- [diag repeat](#diag-repeat-delay-length)
- [diag radio](#diag-radio-sleep)
- [diag stats](#diag-stats)
- [diag stop](#diag-stop)

### diag

Show diagnostics mode status.

```bash
> diag
diagnostics mode is disabled
```

### diag start

Start diagnostics mode.

```bash
> diag start
start diagnostics mode
status 0x00
```

### diag channel

Get the IEEE 802.15.4 Channel value for diagnostics module.

```bash
> diag channel
channel: 11
```

### diag channel \<channel\>

Set the IEEE 802.15.4 Channel value for diagnostics module.

```bash
> diag channel 11
set channel to 11
status 0x00
```

### diag power

Get the tx power value(dBm) for diagnostics module.

```bash
> diag power
tx power: -10 dBm
```

### diag power \<power\>

Set the tx power value(dBm) for diagnostics module.

```bash
> diag power -10
set tx power to -10 dBm
status 0x00
```

### diag send \<packets\> \<length\>

Transmit a fixed number of packets with fixed length.

Length parameter has to be in range [3, 127].

```bash
> diag send 20 100
sending 0x14 packet(s), length 0x64
status 0x00
```

### diag repeat \<delay\> \<length\>

Transmit packets repeatedly with a fixed interval.

Length parameter has to be in range [3, 127].

```bash
> diag repeat 100 100
sending packets of length 0x64 at the delay of 0x64 ms
status 0x00
```

### diag repeat stop

Stop repeated packet transmission.

```bash
> diag repeat stop
repeated packet transmission is stopped
status 0x00
```

### diag radio sleep

Enter radio sleep mode.

```bash
> diag radio sleep
set radio from receive to sleep
status 0x00
```

### diag radio receive

Set radio from sleep mode to receive mode.

```bash
> diag radio receive
set radio from sleep to receive on channel 11
status 0x00
```

### diag radio state

Return the state of the radio.

```bash
> diag radio state
sleep
```

### diag stats

Print statistics during diagnostics mode.

```bash
> diag stats
received packets: 10
sent packets: 10
first received packet: rssi=-65, lqi=101
last received packet: rssi=-64, lqi=98
```

### diag stats clear

Clear statistics during diagnostics mode.

```bash
> diag stats clear
stats cleared
```

### diag stop

Stop diagnostics mode and print statistics.

```bash
> diag stop
received packets: 10
sent packets: 10
first received packet: rssi=-65, lqi=101
last received packet: rssi=-61, lqi=98

stop diagnostics mode
status 0x00
```

### diag rcp

RCP-related diagnostics commands. These commands are used for debugging and testing only.

#### diag rcp start

Start RCP diagnostics mode.

```bash
> diag rcp start
Done
```

#### diag rcp stop

Stop RCP diagnostics mode.

```bash
> diag rcp stop
Done
```

#### diag rcp channel \<channel\>

Set the RCP IEEE 802.15.4 Channel value for diagnostics module.

```bash
> diag rcp channel 11
Done
```

#### diag rcp power \<power\>

Set the RCP tx power value(dBm) for diagnostics module.

```bash
> diag rcp power 0
Done
```

#### diag rcp echo \<message\>

RCP echoes the given message.

```bash
> diag rcp echo 0123456789
0123456789
Done
```

#### diag rcp echo -n \<number\>

RCP echoes the message with the given number of bytes.

```bash
> diag rcp echo -n 20
01234567890123456789
Done
```
