# OpenThread on SAMR21 Example

This directory contains example platform drivers for the [Microchip ATSAMR21G18A][samr21] based on [SAM R21 Xplained Pro Evaluation Kit][samr21_xplained_pro].

[samr21]: http://www.microchip.com/wwwproducts/en/ATSAMR21G18A
[samr21_xplained_pro]: https://www.microchip.com/DevelopmentTools/ProductDetails/ATSAMR21-XPRO

The example platform drivers are intended to present the minimal code necessary to support OpenThread. See the "Run the example with SAMR21 boards" section below for an example using basic OpenThread capabilities.

## Toolchain

Download and install the [GNU toolchain for ARM Cortex-M][gnu-toolchain].

[gnu-toolchain]: https://developer.arm.com/tools-and-software/open-source-software/developer-tools/gnu-toolchain/gnu-rm

In a Bash terminal, follow these instructions to install the GNU toolchain and other dependencies.

```bash
$ cd <path-to-openthread>
$ ./script/bootstrap
```

## Build Examples

1. Download [Advanced Software Framework (ASF)][asf].

[asf]: https://www.microchip.com/mplab/avr-support/advanced-software-framework

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

- [ATSAMR21G18-MR210UA][module-mr210ua]
- [ATSAMR21B18-MZ210PA][module-mz210pa]

To build for these modules set BOARD variable in command line as following:

```bash
$ make -f examples/Makefile-samr21 BOARD=<MODULE>
```

where:

| `<module>`            | Board                               |
| --------------------- | ----------------------------------- |
| `SAMR21_XPLAINED_PRO` | SAM R21 Xplained Pro Evaluation Kit |
| `SAMR21G18_MODULE`    | ATSAMR21G18-MR210UA                 |
| `SAMR21B18_MODULE`    | ATSAMR21B18-MZ210PA                 |

[module-mr210ua]: http://ww1.microchip.com/downloads/en/devicedoc/atmel-42475-atsamr21g18-mr210ua_datasheet.pdf
[module-mz210pa]: http://ww1.microchip.com/downloads/en/devicedoc/atmel-42486-atsamr21b18-mz210pa_datasheet.pdf

After a successful build, the `elf` files are found in `<path-to-openthread>/output/samr21/bin`.

## Flash Binaries

Compiled binaries may be flashed onto the SAM R21 Xplained Pro using embedded debugger EDBG.

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
2. Open terminal to first device `/dev/ttyACM0` (serial port settings: 115200 8-N-1). Type `help` for a list of commands.

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
   netdata register
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
   ```

3. Start a Thread network as Leader.

   ```bash
   > dataset init new
   Done
   > dataset
   Active Timestamp: 1
   Channel: 13
   Channel Mask: 0x07fff800
   Ext PAN ID: d63e8e3e495ebbc3
   Mesh Local Prefix: fd3d:b50b:f96d:722d::/64
   Master Key: dfd34f0f05cad978ec4e32b0413038ff
   Network Name: OpenThread-8f28
   PAN ID: 0x8f28
   PSKc: c23a76e98f1a6483639b1ac1271e2e27
   Security Policy: 0, onrcb
   Done
   > dataset commit active
   Done
   > ifconfig up
   Done
   > thread start
   Done
   ```

   wait a couple of seconds...

   > state leader Done

   ```

   ```

4. Open terminal to second device `/dev/ttyACM1` (serial port settings: 115200 8-N-1) and attach it to the Thread network as a Router.

   ```bash
   > dataset masterkey dfd34f0f05cad978ec4e32b0413038ff
   Done
   > dataset commit active
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
   fd3d:b50b:f96d:722d:0:ff:fe00:fc00
   fd3d:b50b:f96d:722d:0:ff:fe00:c00
   fd3d:b50b:f96d:722d:7a73:bff6:9093:9117
   fe80:0:0:0:6c41:9001:f3d6:4148
   Done
   ```

6. Send an ICMPv6 ping to Leader's Mesh-EID IPv6 address.

   ```bash
   > ping fd3d:b50b:f96d:722d:7a73:bff6:9093:9117
   16 bytes from fd3d:b50b:f96d:722d:558:f56b:d688:799: icmp_seq=1 hlim=64 time=24ms
   ```

The above example demonstrates basic OpenThread capabilities. Enable more features/roles (e.g. commissioner, joiner, DHCPv6 Server/Client, etc.) by assigning compile-options before compiling.

```bash
$ cd <path-to-openthread>
$ ./bootstrap
$ make -f examples/Makefile-samr21 COMMISSIONER=1 JOINER=1 DHCP6_CLIENT=1 DHCP6_SERVER=1
```

For a list of all available commands, visit [OpenThread CLI Reference README.md][cli].

[cli]: https://github.com/openthread/openthread/blob/master/src/cli/README.md

## Other boards

## Verification

The following toolchain has been used for testing and verification:

- gcc version 7.3.1
- Advanced Software Framework (ASF) version 3.45.0
