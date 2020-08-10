# OpenThread on NXP(Freescale) Kinetis MKW41Z512 Example

This directory contains example platform drivers for the [NXP(Freescale) Kinetis MKW41Z512][mkw41z512] based on [FRDM-KW41Z][frdm-kw41z] hardware platform.

[mkw41z512]: http://www.nxp.com/products/microcontrollers-and-processors/arm-processors/kinetis-cortex-m-mcus/w-series-wireless-m0-plus-m4/kinetis-kw41z-2.4-ghz-dual-mode-ble-and-802.15.4-wireless-radio-microcontroller-mcu-based-on-arm-cortex-m0-plus-core:KW41Z
[frdm-kw41z]: http://www.nxp.com/products/software-and-tools/hardware-development-tools/freedom-development-boards/nxp-freedom-development-kit-for-kinetis-kw41z-31z-21z-mcus:FRDM-KW41Z

The example platform drivers are intended to present the minimal code necessary to support OpenThread. As a result, the example platform drivers do not necessarily highlight the platform's full capabilities.

## Toolchain

Download and install the [GNU toolchain for ARM Cortex-M][gnu-toolchain].

[gnu-toolchain]: https://developer.arm.com/tools-and-software/open-source-software/developer-tools/gnu-toolchain/gnu-rm

In a Bash terminal, follow these instructions to install the GNU toolchain and other dependencies.

```bash
$ cd <path-to-openthread>
$ ./script/bootstrap
```

## Build Examples

```bash
$ cd <path-to-openthread>
$ ./bootstrap
$ make -f examples/Makefile-kw41z
```

After a successful build, the `elf` files are found in `<path-to-openthread>/output/kw41z/bin`. You can convert them to `bin` files using `arm-none-eabi-objcopy`:

```bash
$ arm-none-eabi-objcopy -O binary ot-cli-ftd ot-cli-ftd.bin
```

## Flash Binaries

Compiled binaries may be flashed onto the MKW41Z512 using drag-and-drop into the board's MSD Bootloader or the [NXP(Freescale) Test Tool][test-tool] or [JTAG interface][jtag]. The [NXP(Freescale) Test Tool][test-tool] provides a convenient method for flashing a MKW41Z512 via the [J-Link][jlink].

[test-tool]: http://www.nxp.com/webapp/sps/download/license.jsp?colCode=TESTTOOL_SETUP
[jtag]: https://en.wikipedia.org/wiki/JTAG
[jlink]: https://www.segger.com/jlink-software.html

## Running the example

1. Prepare two boards with the flashed `CLI Example` (as shown above).
2. The CLI example uses UART connection. To view raw UART output, start a terminal emulator like PuTTY and connect to the used COM port with the following UART settings:

   - Baud rate: 115200
   - 8 data bits
   - 1 stop bit
   - No parity
   - No flow control

3. Open a terminal connection on the first board and start a new Thread network.

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

4. After a couple of seconds the node will become a Leader of the network.

   ```bash
   > state
   leader
   ```

5. Open a terminal connection on the second board and attach a node to the network.

   ```bash
   > dataset masterkey dfd34f0f05cad978ec4e32b0413038ff
   Done
   > dataset commit active
   Done
   > ifconfig up
   Done
   > thread start
   Done
   ```

6. After a couple of seconds the second node will attach and become a Child.

   ```bash
   > state
   child
   ```

7. List all IPv6 addresses of the first board.

   ```bash
   > ipaddr
   fd3d:b50b:f96d:722d:0:ff:fe00:fc00
   fd3d:b50b:f96d:722d:0:ff:fe00:c00
   fd3d:b50b:f96d:722d:7a73:bff6:9093:9117
   fe80:0:0:0:6c41:9001:f3d6:4148
   Done
   ```

8. Choose one of them and send an ICMPv6 ping from the second board.

   ```bash
   > ping fd3d:b50b:f96d:722d:7a73:bff6:9093:9117
   16 bytes from fd3d:b50b:f96d:722d:558:f56b:d688:799: icmp_seq=1 hlim=64 time=24ms
   ```

For a list of all available commands, visit [OpenThread CLI Reference README.md][cli].

[cli]: https://github.com/openthread/openthread/blob/master/src/cli/README.md
