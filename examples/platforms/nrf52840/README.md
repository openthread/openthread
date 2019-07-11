# OpenThread on nRF52840 Example

<a href="http://threadgroup.org/technology/ourtechnology#certifiedproducts">
<img src="https://cdn.rawgit.com/openthread/openthread/ab4c4e1e/doc/images/certified.svg" alt="Thread Certified Component" width="150px" align="right">
</a>

This directory contains example platform drivers for [Nordic Semiconductor nRF52840 SoC][nRF52840]. The OpenThread stack has been officially certified as a *Thread Certified Component* on this platform.

[nRF52840]: https://www.nordicsemi.com/Products/Low-power-short-range-wireless/nRF52840

To facilitate Thread products development with the nRF52840 platform, Nordic Semiconductor provides <i>nRF5 SDK for Thread and Zigbee</i>. See [Nordic Semiconductor's nRF5 SDK for Thread and Zigbee][nRF5-SDK-section] section for more details.

[nRF5-SDK-section]: #nordic-semiconductors-nrf5-sdk-for-thread-and-zigbee

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

[nRF5-Command-Line-Tools]: https://www.nordicsemi.com/Software-and-Tools/Development-Tools/nRF5-Command-Line-Tools

Install the [nRF5 Command Line Tools][nRF5-Command-Line-Tools] to flash, debug, and make use of logging features on the nRF52840 DK with SEGGER J-Link.

## Building the examples

 To build the examples, run the following command in Bash:

```bash
$ cd <path-to-openthread>
$ ./bootstrap
$ make -f examples/Makefile-nrf52840
```

After a successful build, the `elf` files can be found in
`<path-to-openthread>/output/nrf52840/bin`.
You can convert them to hex using `arm-none-eabi-objcopy`:
```bash
$ arm-none-eabi-objcopy -O ihex ot-cli-ftd ot-cli-ftd.hex
```

### USB CDC ACM support

You can build the libraries with support for the native USB CDC ACM as a serial transport.
To do so, build the firmware with the following parameter:
```
$ make -f examples/Makefile-nrf52840 USB=1
```

Note that the USB CDC ACM serial transport is not supported with Engineering sample A of the nRF52840 chip.

If you are using Windows 7 or earlier, you must load an additional USB CDC driver.
The driver can be found in `third_party/NordicSemiconductor/libraries/usb/nordic_cdc_acm_example.inf`.

### nRF52840 dongle support (PCA10059)

You can build the libraries with support for the USB bootloader with USB DFU trigger support in PCA10059. As this dongle uses the native USB support, you must enable it as well.

To build the libraries, run make with the following parameters:

```
$ make -f examples/Makefile-nrf52840 USB=1 BOOTLOADER=1
```

See [nRF52840 Dongle Programming][nrf52840-dongle-programming] for more details about how to use the USB bootloader.

[nrf52840-dongle-programming]: https://infocenter.nordicsemi.com/topic/ug_nc_programmer/UG/nrf_connect_programmer/ncp_programming_dongle.html

### Native SPI support

You can build the libraries with support for native SPI Slave.
To build the libraries, run make with the following parameter:

```
$ make -f examples/Makefile-nrf52840 NCP_SPI=1
```

With this option enabled, SPI communication between the NCP example and wpantund is possible
(provided that the wpantund host supports SPI Master). To achieve that, an appropriate SPI device
should be chosen in wpantund configuration file, `/etc/wpantund.conf`. You can find an example below.
```
Config:NCP:SocketPath "system:/usr/bin/spi-hdlc-adapter --gpio-int /sys/class/gpio/gpio25 /dev/spidev0.0"
```

In this example, [spi-hdlc-adapter][spi-hdlc-adapter] is a tool that you can use to communicate between NCP and wpantund over SPI. For this example, `spi-hdlc-adapter` is installed in `/usr/bin`.

The default SPI Slave pin configuration for nRF52840 is defined in `examples/platforms/nrf52840/platform-config.h`.

Note that the native SPI Slave support is not intended to be used with Engineering sample A of the nRF52840 chip due to
single transfer size limitation.

[spi-hdlc-adapter]: https://github.com/openthread/openthread/tree/master/tools/spi-hdlc-adapter

### Optional prefix for compiler command

You can prefix the compiler command using the CCPREFIX parameter. This speeds up the compilation when you use tools like [ccache][ccache-website]. Example usage:

[ccache-website]: https://ccache.samba.org/

```
$ make -f examples/Makefile-nrf52840 USB=1 CCPREFIX=ccache
```

### Optional disabling of CryptoCell 310 support

By default, mbedTLS library is built with support for CryptoCell 310 hardware acceleration of cryptographic operations used in OpenThread. You can disable CryptoCell 310 and use software cryptography instead by building OpenThread with the following parameter:
```
$ make -f examples/Makefile-nrf52840 DISABLE_CC310=1
```

### Optional mbedTLS threading support
By default, mbedTLS library is built without support for multiple threads. You can enable this built-in support by building OpenThread with the following parameter:

```
$ make -f examples/Makefile-nrf52840 MBEDTLS_THREADING=1
```

The simple mutex definition is used as shown below:

```
typedef void * mbedtls_threading_mutex_t;
```

However, you can modify it, by providing a path to a header file with proper definition. To do that, build OpenThread with the following parameter:

```
$ make -f examples/Makefile-nrf52840 MBEDTLS_THREADING=1 MBEDTLS_THREADING_MUTEX_DEF="path_to_a_header_file_with_mutex_definition.h"
```

See [mbedTls Thread Safety and Multi Threading][mbedtls-thread-safety-and-multi-threading] for more details.

Note that as a temporary limitation CryptoCell 310 hardware acceleration is disabled when using mbedTLS threading.

[mbedtls-thread-safety-and-multi-threading]: https://tls.mbed.org/kb/development/thread-safety-and-multi-threading

### IEEE EUI-64 address

When the Thread device is configured to obtain the Thread Network security credentials with either Thread Commissioning or an out-of-band method, the extended MAC address should be constructed out of the globally unique IEEE EUI-64.

The IEEE EUI-64 address consists of two parts:
 - 24 bits of MA-L (MAC Address Block Large), formerly called OUI (Organizationally Unique Identifier)
 - 40-bit device unique identifier

By default, the device uses Nordic Semiconductor's MA-L (f4-ce-36). You can modify it by overwriting the `OPENTHREAD_CONFIG_STACK_VENDOR_OUI` define, located in the `openthread-core-nrf52840-config.h` file. This value must be publicly registered by the IEEE Registration Authority.

You can also provide the full IEEE EUI-64 address by providing a custom `otPlatRadioGetIeeeEui64` function. To do this, define the flag `OPENTHREAD_CONFIG_ENABLE_PLATFORM_EUI64_CUSTOM_SOURCE`.

After the Thread Network security credentials have been successfully obtained, the device uses randomly generated extended MAC address.

## Flashing the binaries

Flash the compiled binaries onto nRF52840 using `nrfjprog` which is
part of the [nRF5 Command Line Tools][nRF5-Command-Line-Tools].

```bash
$ nrfjprog -f nrf52 --chiperase --program output/nrf52840/bin/ot-cli-ftd.hex --reset
```

## Running the example

To test the example:

1. Prepare two boards with the flashed `CLI Example` (as shown above). The CLI FTD example uses the direct UART connection.

2. Open a terminal connection on two boards:

   a. Start a terminal emulator like screen.

   b. Connect to the used COM port with the following direct UART settings:

   * Baud rate: 115200
   * 8 data bits
   * 1 stop bit
   * No parity
   * HW flow control: RTS/CTS
     This allows you to view the raw UART output.

     On Linux system a port name should be called e.g. `/dev/ttyACM0` or `/dev/ttyACM1`.

   c. Run the following command to connect to a board.

   ```shell
   screen /dev/ttyACM0 115200
   ```

   Now you are connected with the CLI.

3. Use the following commands to form a network on the first board.

   ```bash
   > dataset init new
   Done
   > dataset
   Active Timestamp: 1
   Channel: 13
   Channel Mask: 07fff800
   Ext PAN ID: d63e8e3e495ebbc3
   Mesh Local Prefix: fd3d:b50b:f96d:722d/64
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

4. Use the following commands to attach to the network on the second board.

   ```bash
   > dataset masterkey dfd34f0f05cad978ec4e32b0413038ff
   Done
   > dataset panid 0x8f28
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

5. List all IPv6 addresses of the first board.

   ```bash
   > ipaddr
   fd3d:b50b:f96d:722d:0:ff:fe00:fc00
   fd3d:b50b:f96d:722d:0:ff:fe00:c00
   fd3d:b50b:f96d:722d:7a73:bff6:9093:9117
   fe80:0:0:0:6c41:9001:f3d6:4148
   Done
   ```

6. Choose one of them and send an ICMPv6 ping from the second board.

   ```bash
   > ping fd3d:b50b:f96d:722d:7a73:bff6:9093:9117
   16 bytes from fd3d:b50b:f96d:722d:558:f56b:d688:799: icmp_seq=1 hlim=64 time=24ms
   ```

For a list of all available commands, visit [OpenThread CLI Reference README.md][CLI].

[CLI]: ./../../../src/cli/README.md

## SEGGER J-Link tools

SEGGER J-Link tools allow to debug and flash generated firmware using on-board debugger or external one.

### Working with RTT logging

By default, the OpenThread's logging module provides functions to output logging
information over SEGGER's Real Time Transfer (RTT).

You can set the desired log level by using the `OPENTHREAD_CONFIG_LOG_LEVEL` define.

To enable the highest verbosity level, append `FULL_LOGS` flag to the `make` command:
```
$ make -f examples/Makefile-nrf52840 FULL_LOGS=1
```

#### Enable logging on Windows

1. Connect the DK to your machine with a USB cable.
2. Run `J-Link RTT Viewer`. The configuration window appears.
3. From the Specify Target Device dropdown menu, select `NRF52840_XXAA`.
4. From the Target Interface & Speed dropdown menu, select `SWD`.

#### Enable logging on Linux

1. Connect the DK to your machine with a USB cable.
2. Run `JLinkExe` to connect to the target. For example:
```
JLinkExe -device NRF52840_XXAA -if SWD -speed 4000 -autoconnect 1 -SelectEmuBySN <SEGGER_ID> -RTTTelnetPort 19021
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
3. From the Specify Target Device dropdown menu, select `NRF52840_XXAA`.
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
JLinkExe -device NRF52840_XXAA -if SWD -speed 4000 -autoconnect 1 -SelectEmuBySN <SEGGER_ID>
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
3. From the Specify Target Device dropdown menu, select `NRF52840_XXAA`.
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
JLinkExe -device NRF52840_XXAA -if SWD -speed 4000 -autoconnect 1 -SelectEmuBySN <SEGGER_ID>
```
3. Run the following command:
```
SetHWFC Force
```
4. Power cycle the DK.

You can find more details [here][J-Link-OB].

[J-Link-OB]: https://wiki.segger.com/J-Link_OB_SAM3U_NordicSemi#Hardware_flow_control_support

## Diagnostic module

nRF52840 port extends [OpenThread Diagnostics Module][DIAG].

You can read about all the features [here][nRFDIAG].

[DIAG]: ./../../../src/diag/README.md
[nRFDIAG]: DIAG.md

## Radio driver documentation

The radio driver comes with documentation that describes the operation of state
machines in this module. To open the `*.uml` sequence diagrams, use [PlantUML][PlantUML-url].

[PlantUML-url]: http://plantuml.com/

## Verification

The following development kits have been used for testing and verification:
  - PCA10056 1.0.0
  - PCA10059 1.0.0
  - PCA10068 0.5.0

The following toolchains have been used for testing and verification:
  - GCC: GCC ARM Embedded 7.2018 q2 update
  - IAR: IAR Workbench 7.80.4
  - SES: SES 4.12
  - ARM: MDK-ARM version 5.25

 The following OpenThread commits have been verified with nRF52840 and nRF52811 examples by Nordic Semiconductor:
  - `2279ef6` - 23.05.2019 (the latest checked)
  - `23ff101` - 22.03.2019
  - `704511c` - 18.09.2018
  - `ec59d7e` - 06.04.2018
  - `a89eb88` - 16.11.2017
  - `6a15261` - 29.06.2017
  - `030efba` - 22.04.2017
  - `de48acf` - 02.03.2017
  - `50db58d` - 23.01.2017

# Nordic Semiconductor's nRF5 SDK for Thread and Zigbee

Use [nRF5 Software Development Kit (SDK) for Thread and Zigbee][nRF5-SDK-Thread-Zigbee] when developing Thread products with Nordic Semiconductor's advanced nRF52840 or nRF52811 SoCs.

The <i>nRF5 SDK for Thread and Zigbee</i> includes:
 - a pre-built OpenThread stack for the Nordic nRF52840 and nRF52811 SoCs,
 - support for hardware-accelerated cryptographic operations using ARM® CryptoCell-310,
 - unique Thread/Bluetooth Low Energy dynamic multiprotocol solution which allows for concurrent operation of Thread and Bluetooth Low Energy utilizing OpenThread and SoftDevice (Nordic’s Bluetooth Low Energy stack) with accompanying example applications,
 - Thread/Bluetooth Low Energy switched multiprotocol solution with accompanying example applications,
 - unique support for DFU-over-Thread (Device Firmware Upgrade),
 - examples to demonstrate interactions between nodes performing different Thread roles with the use of OpenThread and CoAP, CoAP Secure or MQTT-SN protocols,
 - support for OpenThread Network Co-Processor (NCP) and Radio Co-Processor (RCP) using UART, USB or SPI transport protocol,
 - Border Router and cloud connectivity example (e.g. with Google Cloud Platform),
 - Thread native commissioning with NFC example,
 - example applications demonstrating the use of FreeRTOS with OpenThread,
 - support for IAR, Keil MDK-ARM and SEGGER Embedded Studio (SES) IDEs for OpenThread stack and all example applications,
 - range of PC tools including Thread Topology Monitor and nRF Sniffer for 802.15.4,
 - software modules inherited from the nRF5 SDK e.g. peripheral drivers, NFC libraries, Bluetooth Low Energy libraries etc.

[nRF5-SDK-Thread-Zigbee]: https://www.nordicsemi.com/Software-and-Tools/Software/nRF5-SDK-for-Thread-and-Zigbee
