# OpenThread on SAMR21 Example

This directory contains example platform drivers for the [Microchip ATSAMR21G18A][samr21]
based on [SAM R21 Xplained Pro Evaluation Kit][SAMR21_XPLAINED_PRO].

[samr21]: http://www.microchip.com/wwwproducts/en/ATSAMR21G18A
[SAMR21_XPLAINED_PRO]: https://www.microchip.com/DevelopmentTools/ProductDetails/ATSAMR21-XPRO

The example platform drivers are intended to present the minimal code
necessary to support OpenThread. See the "Run the example with SAMR21 boards" section below
for an example using basic OpenThread capabilities.

## Toolchain

Download and install the [GNU toolchain for ARM Cortex-M][gnu-toolchain].

[gnu-toolchain]: https://launchpad.net/gcc-arm-embedded

In a Bash terminal, follow these instructions to install the GNU toolchain and
other dependencies.

```bash
$ cd <path-to-openthread>
$ ./script/bootstrap
```

## Build Examples

1. Download [Advanced Software Framework (ASF)][ASF].

[ASF]: https://www.microchip.com/mplab/avr-support/advanced-software-framework

2. Unzip it to <path-to-openthread>/third_party/microchip folder

```bash
$ unzip asf-standalone-archive-3.45.0.85.zip 
$ cp xdk-asf-3.45.0 -rf  <path-to-openthread>/third_party/microchip/asf
```

3. Build OpenThread Firmware (CLI example) on SAMR21 platform.

```bash
$ cd <path-to-openthread>
$ ./bootstrap
$ make -f examples/Makefile-samr21
```

4. This example can be built for other SAMR21 based modules e.g.:
* [ATSAMR21G18-MR210UA][MODULE-MR210UA]
* [ATSAMR21B18-MZ210PA][MODULE-MZ210PA]

To build for these modules set BOARD variable in command line as following:
```bash
$ make -f examples/Makefile-samr21 BOARD=<MODULE>
```

where:


| `<module>`            | Board                               |
| ----------------------|-------------------------------------|
| `SAMR21_XPLAINED_PRO` | SAM R21 Xplained Pro Evaluation Kit |
| `SAMR21G18_MODULE`    | ATSAMR21G18-MR210UA                 |
| `SAMR21B18_MODULE`    | ATSAMR21B18-MZ210PA                 |

[MODULE-MR210UA]: http://ww1.microchip.com/downloads/en/devicedoc/atmel-42475-atsamr21g18-mr210ua_datasheet.pdf
[MODULE-MZ210PA]: http://ww1.microchip.com/downloads/en/devicedoc/atmel-42486-atsamr21b18-mz210pa_datasheet.pdf


After a successful build, the `elf` files are found in
`<path-to-openthread>/output/samr21/bin`.

## Flash Binaries

Compiled binaries may be flashed onto the SAM R21 Xplained Pro using embedded
debugger EDBG.

```bash
$ openocd -f board/atmel_samr21_xplained_pro.cfg
$ cd <path-to-openthread>/output/samr21/bin
$ arm-none-eabi-gdb ot-cli-ftd
$ (gdb) target remote 127.0.0.1:3333
$ (gdb) load
$ (gdb) monitor reset
$ (gdb) c
```

## Run the example with SAM R21 Xplained Pro boards
1. Flash two SAM R21 Xplained Pro boards with the `CLI example` firmware (as shown above).
2. Open terminal to first device `/dev/ttyACM0` (serial port settings: 115200 8-N-1).
   Type `help` for a list of commands.

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

3. Start a Thread network as Leader.

```bash
> panid 0xface
Done
> ifconfig up
Done
> thread start
Done

wait a couple of seconds...

> state
leader
Done
```

4. Open terminal to second device `/dev/ttyACM1` (serial port settings: 115200 8-N-1)
   and attach it to the Thread network as a Router.

```bash
> panid 0xface
Done
> routerselectionjitter 1
Done
> ifconfig up
Done
> thread start
Done

wait a couple of seconds...

> state
router
Done
```

5. List all IPv6 addresses of Leader.

```bash
> ipaddr
fdde:ad00:beef:0:0:ff:fe00:fc00
fdde:ad00:beef:0:0:ff:fe00:800
fdde:ad00:beef:0:5b:3bcd:deff:7786
fe80:0:0:0:6447:6e10:cf7:ee29
Done
```

6. Send an ICMPv6 ping to Leader's Mesh-EID IPv6 address.

```bash
> ping fdde:ad00:beef:0:5b:3bcd:deff:7786
8 bytes from fdde:ad00:beef:0:5b:3bcd:deff:7786: icmp_seq=1 hlim=64 time=24ms
```

The above example demonstrates basic OpenThread capabilities. Enable more features/roles (e.g. commissioner,
joiner, DHCPv6 Server/Client, etc.) by assigning compile-options before compiling.

```bash
$ cd <path-to-openthread>
$ ./bootstrap
$ make -f examples/Makefile-samr21 COMMISSIONER=1 JOINER=1 DHCP6_CLIENT=1 DHCP6_SERVER=1
```

For a list of all available commands, visit [OpenThread CLI Reference README.md][CLI].

[CLI]: https://github.com/openthread/openthread/blob/master/src/cli/README.md

## Other boards

## Verification

The following toolchain has been used for testing and verification:
   - gcc version 7.3.1
   - Advanced Software Framework (ASF) version 3.45.0
