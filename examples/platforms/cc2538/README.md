# OpenThread on CC2538 Example

This directory contains example platform drivers for the [Texas
Instruments CC2538][cc2538].

[cc2538]: http://www.ti.com/product/CC2538

The example platform drivers are intended to present the minimal code
necessary to support OpenThread.  As a result, the example platform
drivers do not necessarily highlight the platform's full capabilities.

## Toolchain

Download and install the [GNU toolchain for ARM
Cortex-M][gnu-toolchain].

[gnu-toolchain]: https://launchpad.net/gcc-arm-embedded

## Build Examples

```bash
$ cd <path-to-openthread>
$ ./bootstrap
$ make -f examples/Makefile-cc2538
```

After a successful build, the `elf` files are found in
`<path-to-openthread>/output/cc2538/bin`.  You can convert them to `bin`
files using `arm-none-eabi-objcopy`:
```bash
$ arm-none-eabi-objcopy -O binary ot-cli-ftd ot-cli-ftd.bin
```

## Flash Binaries

Compiled binaries may be flashed onto the CC2538 using the [Serial
Bootloader Interface][cc2538-bsl] or [JTAG interface][jtag].  The
[cc2538-bsl.py script][cc2538-bsl-tool] provides a convenient method
for flashing a CC2538 via the UART.

[cc2538-bsl]: http://www.ti.com/lit/an/swra466a/swra466a.pdf
[cc2538-bsl-tool]: https://github.com/JelmerT/cc2538-bsl
[jtag]: https://en.wikipedia.org/wiki/JTAG

## Interact

1. Open terminal to `/dev/ttyUSB1` (serial port settings: 115200 8-N-1).
2. Type `help` for list of commands.

```bash
> help
help
channel
childtimeout
contextreusedelay
extaddr
extpanid
ipaddr
keysequence
leaderweight
masterkey
mode
netdataregister
networkidtimeout
networkname
panid
ping
prefix
releaserouterid
rloc16
route
routerupgradethreshold
scan
start
state
stop
whitelist
```
