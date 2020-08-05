# OpenThread on nRF52811 Example

This directory contains example platform drivers for [Nordic Semiconductor nRF52811 SoC][nrf52811].

[nrf52811]: https://www.nordicsemi.com/Products/Low-power-short-range-wireless/nRF52811

This SoC is meant to be used in the configuration that involves the Host Processor and the IEEE 802.15.4 radio. In this configuration, the full OpenThread stack is running on the Host Processor and the nRF52811 SoC acts as an IEEE 802.15.4 radio. The radio is running a minimal OpenThread implementation that allows for communication between the Host Processor and the nRF52811. In this architecture the nRF52811 SoC device is called RCP (Radio Co-Processor).

The nRF52811 platform is currently under development.

For the SoC capable to a run full OpenThread stack, see the [nRF52840 platform][nrf52840-page].

[nrf52840-page]: ./../nrf52840/README.md

## Emulation on nRF52840

You can use examples for nRF52811 on nRF52840 without any changes in the generated binary files. This allows for easier testing in early stages of development.

## Prerequisites

Before you start building the examples, you must download and install the toolchain and the tools required for flashing and debugging.

### Toolchain

Download and install the [GNU toolchain for ARM Cortex-M][gnu-toolchain].

[gnu-toolchain]: https://developer.arm.com/tools-and-software/open-source-software/developer-tools/gnu-toolchain/gnu-rm

To install the GNU toolchain and its dependencies, run the following commands in Bash:

```bash
$ cd <path-to-openthread>
$ ./script/bootstrap
```

### Flashing and debugging tools

[nrf-command-line-tools]: https://www.nordicsemi.com/Software-and-Tools/Development-Tools/nRF-Command-Line-Tools

Install the [nRF Command Line Tools][nrf-command-line-tools] to flash, debug, and make use of logging features on the nRF52811 DK with SEGGER J-Link.

## Building the examples

With this platform, you can build:

- Limited version of CLI example (e.g. without Thread commissioning functionality)
- RCP example that consists of two parts:
  - firmware that is flashed to the nRF52811 SoC
  - host executables to be executed on a POSIX platform in case of the RCP usage

### Building host executables

```bash
$ cd <path-to-openthread>
$ ./bootstrap
$ make -f src/posix/Makefile-posix
```

After a successful build, executables can be found in `<path-to-openthread>/openthread/output/posix/<system-architecture>/bin`. It is recommended to copy them to `/usr/bin` for easier access.

### Building the firmware with UART support

To build the firmware using default UART RCP transport, run the following make commands:

```bash
$ cd <path-to-openthread>
$ ./bootstrap
$ make -f examples/Makefile-nrf52811
```

After a successful build, the `elf` files can be found in `<path-to-openthread>/output/nrf52811/bin`. You can convert them to hex using `arm-none-eabi-objcopy`:

```bash
$ arm-none-eabi-objcopy -O ihex ot-cli-mtd ot-cli-mtd.hex
$ arm-none-eabi-objcopy -O ihex ot-rcp ot-rcp.hex
```

### Building the firmware with native SPI support

You can build the libraries with support for the native SPI Slave. To do so, build the libraries with the following parameter:

```
$ make -f examples/Makefile-nrf52811 NCP_SPI=1
```

With this option enabled, the RCP example can communicate through SPI with wpantund (provided that the wpantund host supports SPI Master). To achieve this communication, choose the right SPI device in the wpantund configuration file, `/etc/wpantund.conf`. See the following example.

```
Config:NCP:SocketPath "system:/usr/bin/ot-ncp /usr/bin/spi-hdlc-adapter -- '--stdio -i /sys/class/gpio/gpio25 /dev/spidev0.1'"
```

In this example, [spi-hdlc-adapter][spi-hdlc-adapter] is a tool that you can use to communicate between RCP and wpantund over SPI. For this example, `spi-hdlc-adapter` is installed in `/usr/bin`.

The default SPI Slave pin configuration for nRF52811 is defined in `examples/platforms/nrf52811/transport-config.h`.

[spi-hdlc-adapter]: https://github.com/openthread/openthread/tree/master/tools/spi-hdlc-adapter

### IEEE EUI-64 address

When the Thread device is configured to obtain the Thread Network security credentials with either Thread Commissioning or an out-of-band method, the extended MAC address should be constructed out of the globally unique IEEE EUI-64.

The IEEE EUI-64 address consists of two parts:

- 24 bits of MA-L (MAC Address Block Large), formerly called OUI (Organizationally Unique Identifier)
- 40-bit device unique identifier

By default, the device uses Nordic Semiconductor's MA-L (f4-ce-36). You can modify it by overwriting the `OPENTHREAD_CONFIG_STACK_VENDOR_OUI` define, located in the `openthread-core-nrf52811-config.h` file. This value must be publicly registered by the IEEE Registration Authority.

You can also provide the full IEEE EUI-64 address by providing a custom `otPlatRadioGetIeeeEui64` function. To do this, define the flag `OPENTHREAD_CONFIG_ENABLE_PLATFORM_EUI64_CUSTOM_SOURCE`.

After the Thread Network security credentials have been successfully obtained, the device uses randomly generated extended MAC address.

## Flashing binaries

Once the examples and libraries are built, flash the compiled binaries onto nRF52811 using `nrfjprog` that is part of the [nRF Command Line Tools][nrf-command-line-tools].

Run the following command:

```bash
$ nrfjprog -f nrf52 --chiperase --program output/nrf52811/bin/ot-cli-mtd.hex --reset
```

## Running the example

To test the example:

1. Prepare two boards.

2. Flash one with the `CLI MTD Example` (ot-cli-mtd.hex, as shown above).

3. Flash `RCP Example` (ot-rcp.hex) to the other board.

4. Connect the RCP to the PC.

5. Start the ot-cli host application and connect with the RCP. It is assumed that the default UART version of RCP is being used (make executed without the NCP-SPI=1 flag). On Linux system, call a port name, for example `/dev/ttyACM0` for the first connected development kit, and `/dev/ttyACM1` for the second one.

   ```bash
   $ /usr/bin/ot-cli /dev/ttyACM0 115200
   ```

   You are now connected with the CLI on the first board

6. Use the following commands to form a network:

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

   After a couple of seconds the node will become a Leader of the network.

   ```bash
   > state
   leader
   ```

7. Open a terminal connection on the second board and attach a node to the network:

   a. Start a terminal emulator like screen.

   b. Connect to the used COM port with the following direct UART settings:

   - Baud rate: 115200
   - 8 data bits
   - 1 stop bit
   - No parity
   - HW flow control: RTS/CTS This allows you to view the raw UART output.

   c. Run the following command to connect to the second board.

   ```shell
   screen /dev/ttyACM1 115200
   ```

   You are now connected with the CLI on the second board

8. Use the following commands to attach to the network on the second board:

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

   After a couple of seconds the second node will attach and become a Child.

   ```bash
   > state
   child
   ```

9. List all IPv6 addresses of the first board.

   ```bash
   > ipaddr
   fd3d:b50b:f96d:722d:0:ff:fe00:fc00
   fd3d:b50b:f96d:722d:0:ff:fe00:c00
   fd3d:b50b:f96d:722d:7a73:bff6:9093:9117
   fe80:0:0:0:6c41:9001:f3d6:4148
   Done
   ```

10. Choose one of them and send an ICMPv6 ping from the second board.

    ```bash
    > ping fd3d:b50b:f96d:722d:7a73:bff6:9093:9117
    16 bytes from fd3d:b50b:f96d:722d:558:f56b:d688:799: icmp_seq=1 hlim=64 time=24ms
    ```

For a list of all available commands, visit [OpenThread CLI Reference README.md][cli].

[cli]: ./../../../src/cli/README.md

## SEGGER J-Link tools

SEGGER J-Link tools allow to debug and flash generated firmware using on-board debugger or external one.

### Working with RTT logging

By default, the OpenThread's logging module provides functions to output logging information over SEGGER's Real Time Transfer (RTT).

You can set the desired log level by using the `OPENTHREAD_CONFIG_LOG_LEVEL` define.

To enable the highest verbosity level, append `FULL_LOGS` flag to the `make` command:

```
$ make -f examples/Makefile-nrf52811 FULL_LOGS=1
```

#### Enable logging on Windows

1. Connect the DK to your machine with a USB cable.
2. Run `J-Link RTT Viewer`. The configuration window appears.
3. From the Specify Target Device dropdown menu, select `NRF52810_XXAA`.
4. From the Target Interface & Speed dropdown menu, select `SWD`.

#### Enable logging on Linux

1. Connect the DK to your machine with a USB cable.
2. Run `JLinkExe` to connect to the target. For example:

```
JLinkExe -device NRF52810_XXAA -if SWD -speed 4000 -autoconnect 1 -SelectEmuBySN <SEGGER_ID> -RTTTelnetPort 19021
```

3. Run `JLinkRTTTelnet` to obtain the RTT logs from the connected device in a separate console. For example:

```
JLinkRTTClient -RTTTelnetPort 19021
```

### Mass Storage Device known issue

Depending on your version, due to a known issue in SEGGER's J-Link firmware, you might experience data corruption or data drops if you use the serial port. You can avoid this issue by disabling the Mass Storage Device.

#### Disabling the Mass Storage Device on Windows

1. Connect the DK to your machine with a USB cable.
2. Run `J-Link Commander`. The configuration window appears.
3. From the Specify Target Device dropdown menu, select `NRF52810_XXAA`.
4. From the Target Interface & Speed dropdown menu, select `SWD`.
5. Run the following command:

```
MSDDisable
```

6. Power cycle the DK.

#### Disabling the Mass Storage Device on Linux

1. Connect the DK to your machine with a USB cable.
2. Run `JLinkExe` to connect to the target. For example:

```
JLinkExe -device NRF52810_XXAA -if SWD -speed 4000 -autoconnect 1 -SelectEmuBySN <SEGGER_ID>
```

3. Run the following command:

```
MSDDisable
```

4. Power cycle the DK.

### Hardware Flow Control detection

By default, SEGGER J-Link automatically detects at runtime whether the target is using Hardware Flow Control (HWFC).

The automatic HWFC detection is done by driving P0.07 (Clear to Send - CTS) from the interface MCU and evaluating the state of P0.05 (Request to Send - RTS) when the first data is sent or received. If the state of P0.05 (RTS) is high, it is assumed that HWFC is not used.

To avoid potential race conditions, you can force HWFC and bypass the runtime auto-detection.

#### Disabling the HWFC detection on Windows

1. Connect the DK to your machine with a USB cable.
2. Run `J-Link Commander`. The configuration window appears.
3. From the Specify Target Device dropdown menu, select `NRF52810_XXAA`.
4. From the Target Interface & Speed dropdown menu, select `SWD`.
5. Run the following command:

```
SetHWFC Force
```

6. Power cycle the DK.

#### Disabling the HWFC detection on Linux

1. Connect the DK to your machine with a USB cable.
2. Run `JLinkExe` to connect to the target. For example:

```
JLinkExe -device NRF52810_XXAA -if SWD -speed 4000 -autoconnect 1 -SelectEmuBySN <SEGGER_ID>
```

3. Run the following command:

```
SetHWFC Force
```

4. Power cycle the DK.

You can find more details [here][j-link-ob].

[j-link-ob]: https://wiki.segger.com/J-Link_OB_SAM3U_NordicSemi#Hardware_flow_control_support

## Diagnostic module

nRF52811 supports [OpenThread Diagnostics Module][diag], with some additional features.

For more information, see [nRF Diag command reference][nrfdiag].

[diag]: ./../../../src/core/diags/README.md
[nrfdiag]: ./../DIAG.md

## Radio driver documentation

The radio driver documentation includes \*.uml state machines sequence diagrams that can be opened with [PlantUML][plantuml-url].

[plantuml-url]: http://plantuml.com/
