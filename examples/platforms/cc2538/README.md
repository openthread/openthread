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

In a Bash terminal, follow these instructions to install the GNU toolchain and
other dependencies.

```bash
$ cd <path-to-openthread>
$ ./script/bootstrap
```

## Building

In a Bash terminal, follow these instructions to build the cc2538 examples.

```bash
$ cd <path-to-openthread>
$ ./bootstrap
$ make -f examples/Makefile-cc2538
```

## Flash Binaries

If the build completed successfully, the `elf` files may be found in
`<path-to-openthread>/output/cc2538/bin`.

To flash the images with [Flash Programmer 2][ti-flash-programmer-2],
the files must have the `*.elf` extension.

```bash
$ cd <path-to-openthread>/output/cc2538/bin
$ cp ot-cli ot-cli.elf
```

To load the images with the [serial bootloader][ti-cc2538-bootloader],
the images must be converted to `bin`. This is done using
`arm-none-eabi-objcopy`

```bash
$ cd <path-to-openthread>/output/cc2538/bin
$ arm-none-eabi-objcopy -O binary ot-cli ot-cli.bin
```

The [cc2538-bsl.py script][cc2538-bsl-tool] provides a convenient
method for flashing a CC2538 via the UART. To enter the bootloader
backdoor for flashing, hold down SELECT for CC2538DK (corresponds to
logic '0') while you press the Reset button.

[ti-flash-programmer-2]: http://www.ti.com/tool/flash-programmer
[ti-cc2538-bootloader]: http://www.ti.com/lit/an/swra466a/swra466a.pdf
[cc2538-bsl-tool]: https://github.com/JelmerT/cc2538-bsl

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
