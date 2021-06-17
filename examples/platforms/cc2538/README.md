# OpenThread on CC2538 Example

This directory contains example platform drivers for the [Texas Instruments CC2538][cc2538].

[cc2538]: http://www.ti.com/product/CC2538

The example platform drivers are intended to present the minimal code necessary to support OpenThread. As a result, the example platform drivers do not necessarily highlight the platform's full capabilities.

## Toolchain

Download and install the [GNU toolchain for ARM Cortex-M][gnu-toolchain].

[gnu-toolchain]: https://developer.arm.com/tools-and-software/open-source-software/developer-tools/gnu-toolchain/gnu-rm

In a Bash terminal, follow these instructions to install the GNU toolchain and other dependencies.

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

### CC2592 support

If your board has a CC2592 range extender front-end IC connected to the CC2538 (e.g. the CC2538-CC2592 EM reference design), you need to initialise this part before reception of radio traffic will work.

Support is enabled in OpenThread by building with `CC2592=1`:

```bash
$ make -f examples/Makefile-cc2538 CC2592=1
```

The default settings should work for any design following the integration advice given in TI's application report ["AN130 - Using CC2592 Front End With CC2538"](http://www.ti.com/lit/pdf/swra447).

Additional settings can be customised:

- `CC2592_PA_EN`: This specifies which pin (on port C of the CC2538) connects to the CC2592's `PA_EN` pin. The default is `3` (PC3).
- `CC2592_LNA_EN`: This specifies which pin (on port C of the CC2538) connects to the CC2592's `LNA_EN` pin. The default is `2` (PC2).
- `CC2592_USE_HGM`: This defines whether the HGM pin of the CC2592 is under GPIO control or not. If not, it is assumed that the HGM pin is tied to a power rail.
- `CC2592_HGM_PORT`: The HGM pin can be connected to any free GPIO. TI recommend using PD2, however if you've used a pin on another GPIO port, you may specify that port (`A`, `B` or `C`) here.
- `CC2592_HGM_PORT`: The HGM pin can be connected to any free GPIO. TI recommend using PD2, however if you've used a pin on another GPIO port, you may specify that port (`A`, `B` or `C`) here. Default is `D`.
- `CC2592_HGM_PIN`: The HGM pin can be connected to any free GPIO. TI recommend using PD2, however if you've used a pin on another GPIO pin, you can specify the pin here. Default is `2`.
- `CC2592_HGM_DEFAULT_STATE`: By default, HGM is enabled at power-on, but you may want to have it default to off, specify `CC2592_HGM_DEFAULT_STATE=0` to do so.
- `CC2538_RECEIVE_SENSITIVITY`: If you have tied the HGM pin to a power rail, this allows you to calibrate the RSSI values according to the new receive sensitivity. This has no effect if `CC2592_USE_HGM=1` (the default).
- `CC2538_RSSI_OFFSET`: If you have tied the HGM pin to a power rail, this allows you to calibrate the RSSI values according to the new RSSI offset. This has no effect if `CC2592_USE_HGM=1` (the default).

## Flash Binaries

If the build completed successfully, the `elf` files may be found in `<path-to-openthread>/output/cc2538/bin`.

To flash the images with [Flash Programmer 2][ti-flash-programmer-2], the files must have the `*.elf` extension.

```bash
$ cd <path-to-openthread>/output/cc2538/bin
$ cp ot-cli ot-cli.elf
```

To load the images with the [serial bootloader][ti-cc2538-bootloader], the images must be converted to `bin`. This is done using `arm-none-eabi-objcopy`

```bash
$ cd <path-to-openthread>/output/cc2538/bin
$ arm-none-eabi-objcopy -O binary ot-cli ot-cli.bin
```

The [cc2538-bsl.py script][cc2538-bsl-tool] provides a convenient method for flashing a CC2538 via the UART. To enter the bootloader backdoor for flashing, hold down SELECT for CC2538DK (corresponds to logic '0') while you press the Reset button.

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
mode
netdata register
networkidtimeout
networkkey
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
```
