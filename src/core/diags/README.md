# OpenThread Factory Diagnostics Module Reference

The OpenThread diagnostics module is a tool for debugging platform hardware manually, and it will also be used during manufacturing process, to verify platform hardware performance.

The diagnostics module supports common diagnostics features that are listed below, and it also provides a mechanism for expanding platform specific diagnostics features.

## Common Diagnostics Command List

- [diag](#diag)
- [diag start](#diag-start)
- [diag channel](#diag-channel)
- [diag cw](#diag-cw-start)
- [diag frame](#diag-frame)
- [diag stream](#diag-stream-start)
- [diag power](#diag-power)
- [diag powersettings](#diag-powersettings)
- [diag send](#diag-send-packets-length)
- [diag repeat](#diag-repeat-delay-length)
- [diag radio](#diag-radio-sleep)
- [diag rawpowersetting](#diag-rawpowersetting)
- [diag stats](#diag-stats)
- [diag gpio](#diag-gpio-get-gpio)
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

### diag cw start

Start transmitting continuous carrier wave.

```bash
> diag cw start
Done
```

### diag cw stop

Stop transmitting continuous carrier wave.

```bash
> diag cw stop
Done
```

### diag frame

Usage: `diag frame [-b MaxCsmaBackoffs] [-c] [-C RxChannelAfterTxDone] [-d TxDelay] [-p TxPower] [-r MaxFrameRetries] [-s] [-u] <frame>`

Set the frame (hex encoded) to be used by `diag send` and `diag repeat`. The frame may be overwritten by `diag send` and `diag repeat`.

- Specify `-b` to specify the `mInfo.mTxInfo.mMaxCsmaBackoffs` field for this frame.
- Specify `-c` to enable CSMA/CA for this frame in the radio layer.
- Specify `-C` to specify the `mInfo.mTxInfo.mRxChannelAfterTxDone` field for this frame.
- Specify `-d` to specify the `mInfo.mTxInfo.mTxDelay` field for this frame and the `mInfo.mTxInfo.mTxDelayBaseTime` field is set to the current radio time.
- Specify `-p` to specify the tx power in dBm for this frame.
- Specify `-r` to specify the `mInfo.mTxInfo.mMaxFrameRetries` field for this frame.
- Specify `-s` to indicate that tx security is already processed thus it should be skipped in the radio layer.
- Specify `-u` to specify the `mInfo.mTxInfo.mIsHeaderUpdated` field for this frame.

```bash
> diag frame 11223344
Done
```

### diag stream start

Start transmitting a stream of characters.

```bash
> diag stream start
Done
```

### diag stream stop

Stop transmitting a stream of characters.

```bash
> diag stream stop
Done
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

### diag powersettings

Show the currently used power settings table.

- Note: The unit of `TargetPower` and `ActualPower` is 0.01dBm.

```bash
> diag powersettings
| StartCh | EndCh | TargetPower | ActualPower | RawPowerSetting |
+---------+-------+-------------+-------------+-----------------+
|      11 |    14 |        1700 |        1000 |          223344 |
|      15 |    24 |        2000 |        1900 |          112233 |
|      25 |    25 |        1600 |        1000 |          223344 |
|      26 |    26 |        1600 |        1500 |          334455 |
Done
```

### diag powersettings \<channel\>

Show the currently used power settings for the given channel.

```bash
> diag powersettings 11
TargetPower(0.01dBm): 1700
ActualPower(0.01dBm): 1000
RawPowerSetting: 223344
Done
```

### diag send \<packets\> [length]

Transmit a fixed number of packets.

Send the frame set by `diag frame` if length is omitted. Otherwise overwrite the frame set by `diag frame` and send a frame of the given length(MUST be in range [3, 127]).

```bash
> diag send 20 100
sending 0x14 packet(s), length 0x64
status 0x00
```

### diag repeat \<delay\> [length]

Transmit packets repeatedly with a fixed interval.

Send the frame set by `diag frame` if length is omitted. Otherwise overwrite the frame set by `diag frame` and send a frame of the given length (MUST be in range [3, 127]).

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

### diag radio receive \[async\] \<number\> \[lpr\]

Set the radio to receive mode and receive a specified number of frames.

- async: Use the non-blocking mode.
- number: The number of frames expected to be received.
- l: Show Lqi.
- p: Show Psdu.
- r: Show Rssi.

```bash
> diag radio receive 5 lpr
0, rssi:-49, lqi:119, len:10, psdu:000102030405060771e
1, rssi:-51, lqi:112, len:10, psdu:000102030405060771e
2, rssi:-42, lqi:120, len:10, psdu:000102030405060771e
3, rssi:-54, lqi:111, len:10, psdu:000102030405060771e
4, rssi:-56, lqi:108, len:10, psdu:000102030405060771e
Done
```

### diag radio state

Return the state of the radio.

```bash
> diag radio state
sleep
```

### diag radio enable

Enable radio interface and put it in receive mode.

```bash
> diag radio enable
Done
```

### diag radio disable

Disable radio interface.

```bash
> diag radio disable
Done
```

### diag rawpowersetting

Show the raw power setting for diagnostics module.

```bash
> diag rawpowersetting
112233
Done
```

### diag rawpowersetting \<settings\>

Set the raw power setting for diagnostics module.

```bash
> diag rawpowersetting 112233
Done
```

### diag rawpowersetting enable

Enable the platform layer to use the value set by the command `diag rawpowersetting \<settings\>`.

```bash
> diag rawpowersetting enable
Done
```

### diag rawpowersetting disable

Disable the platform layer to use the value set by the command `diag rawpowersetting \<settings\>`.

```bash
> diag rawpowersetting disable
Done
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

### diag gpio get \<gpio\>

Get the gpio value.

```bash
> diag gpio get 0
1
Done
```

### diag gpio set \<gpio\> \<value\>

Set the gpio value.

The parameter `value` has to be `0` or `1`.

```bash
> diag gpio set 0 1
Done
```

### diag gpio mode \<gpio\>

Get the gpio mode.

```bash
> diag gpio mode 1
in
Done
```

### diag gpio mode \<gpio\> in

Sets the given gpio to the input mode without pull resistor.

```bash
> diag gpio mode 1 in
Done
```

### diag gpio mode \<gpio\> out

Sets the given gpio to the output mode.

```bash
> diag gpio mode 1 out
Done
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
