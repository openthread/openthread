## Diagnostic module

nRF52840 port extends [OpenThread Diagnostics Module][DIAG].

New commands allow for more accurate low level radio testing.

### New commands
 * [diag ccathreshold](#diag-ccathreshold)
 * [diag gpio](#diag-gpio)
 * [diag id](#diag-id)
 * [diag listen](#diag-listen)
 * [diag temp](#diag-temp)
 * [diag transmit](#diag-transmit)

### Diagnostic radio packet
[diag listen](#diag-listen) and [diag transmit](#diag-transmit) use radio frame payload specified below.

 ```c
 struct PlatformDiagMessage
 {
     const char mMessageDescriptor[11];
     uint8_t mChannel;
     int16_t mID;
     uint32_t mCnt;
 };
 ```

`mMessageDescriptor` is constant string `"DiagMessage"`.<br />
`mChannel` contains channel number on which packet was transmitted.<br />
`mID` contains board ID set with [diag id](#diag-id) command.<br />
`mCnt` is a counter incremented every time board transmits diagnostic radio packet.

If [listen](#diag-listen) mode is enabled and OpenThread was built with `DEFAULT_LOGGING` flag, JSON string is printed every time diagnostic radio packet is received.

```JSON
 {"Frame":{
   "LocalChannel":"<listening board channel>",
   "RemoteChannel":"<mChannel>",
   "CNT":"<mCnt>",
   "LocalID":"<listening board ID>",
   "RemoteID":"<mID>",
   "RSSI":"<packet RSSI>"
 }}
```

### diag ccathreshold
Get current CCA threshold.

### diag ccathreshold \<threshold\>
Set CCA threshold.

Value range 0 to 255.

Default: `45`.

### diag gpio
Set of commands for managing gpio pins.

### diag gpio \<pinnum\>
Return the current value of the gpio.

Note: \<pinnum\> is an integer that combines port and pin into a single,
contiguous number space as follows:
```
   pinnum = (port * 32) + pin
```
See also the [`NRF_GPIO_PIN_MAP`](../../../third_party/NordicSemiconductor/hal/nrf_gpio.h) macro.

```bash
> diag gpio 47
gpio 47 = 0
```

### diag gpio out \<pinnum\>
Sets the given gpio to output mode.
```bash
> diag gpio out 47
gpio 47: out
```

### diag gpio in \<pinnum\>
Sets the given gpio to input, no pull mode.
```bash
> diag gpio in 47
gpio 47: in no pull
```

### diag gpio set \<pinnum\>
Sets the given output gpio to high.
```bash
> diag gpio set 47
gpio 47 = 1
```

### diag gpio clr \<pinnum\>
Sets the given output gpio to low.
```bash
> diag gpio clr 47
gpio 47 = 0
```

### diag id
Get board ID.

### diag id \<id\>
Set board ID.

Value range 0 to 32767.

Default: `-1`.

### diag listen
Get listen state.

### diag listen \<listen\>
Set listen state.

`0` disable listen state.<br />
`1` enable listen state.

Default: listen disabled.

### diag temp
Get temperature from internal temperature sensor, in degrees Celsius.

### diag transmit
Get messages count and interval between them that will be transmitted after `diag transmit start`.

### diag transmit interval \<interval\>
Set interval in ms between transmitted messages.

Value range 1 to 4294967295.

Default: `1`.

### diag transmit count \<count\>
Set number of messages to be transmitted.

Value range 1 to 2147483647<br />
or<br />
`-1` continuous transmission.

Default: `1`

### diag transmit stop
Stop ongoing transmission regardless of remaining number of messages to be sent.

### diag transmit start
Start transmiting messages with specified interval.

### diag transmit carrier
Start transmitting continuous carrier wave.

[DIAG]: ./../../../src/diag/README.md