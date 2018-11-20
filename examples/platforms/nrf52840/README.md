# OpenThread on nRF52840 Example

This directory contains example platform drivers for [Nordic Semiconductor nRF52840 SoC][nRF52840].

[nRF52840]: https://www.nordicsemi.com/eng/Products/nRF52840

To facilitate Thread products development with the nRF52840 platform, Nordic Semiconductor provides <i>nRF5 SDK for Thread and Zigbee</i>. See [Nordic Semiconductor's nRF5 SDK for Thread and Zigbee][nRF5-SDK-section] section for more details.

[nRF5-SDK-section]: #nordic-semiconductors-nrf5-sdk-for-thread-and-zigbee

## Toolchain

Download and install [GNU toolchain for ARM Cortex-M][gnu-toolchain].

[gnu-toolchain]: https://launchpad.net/gcc-arm-embedded

In a Bash terminal, follow these instructions to install the GNU toolchain and
other dependencies.

```bash
$ cd <path-to-openthread>
$ ./script/bootstrap
```

## Building the examples

```bash
$ cd <path-to-openthread>
$ ./bootstrap
$ make -f examples/Makefile-nrf52840
```

After a successful build, the `elf` files can be found in
`<path-to-openthread>/output/nrf52840/bin`.  You can convert them to `hex`
files using `arm-none-eabi-objcopy`:
```bash
$ arm-none-eabi-objcopy -O ihex ot-cli-ftd ot-cli-ftd.hex
```

## Native USB support

You can build the libraries with support for native USB CDC ACM as a serial transport.
To do so, build the libraries with the following parameter:
```
$ make -f examples/Makefile-nrf52840 USB=1
```

Note, that if Windows 7 or earlier is used, an additional USB CDC driver has to be loaded.
It can be found in third_party/NordicSemiconductor/libraries/usb/nordic_cdc_acm_example.inf

## nRF52840 dongle support (PCA10059)
You can build the libraries with support for USB bootloader with automatic USB DFU trigger support in PCA10059. As this dongle uses native USB support we have to enable it as well. To do so, build the libraries with the following parameter:
```
$ make -f examples/Makefile-nrf52840 USB=1 BOOTLOADER=1
```
Please see [nRF52840 Dongle Programming][nrf52840-dongle-programming] for more details about how to use USB bootloader.

[nrf52840-dongle-programming]: https://infocenter.nordicsemi.com/index.jsp?topic=%2Fcom.nordic.infocenter.nrf52%2Fdita%2Fnrf52%2Fdevelopment%2Fnrf52840_dongle%2Fprogramming.html&cp=2_0_4_4

## Prefixing compiler command

You can prefix compiler command using CCPREFIX parameter. This is useful when you utilize tools like [ccache][ccache-website] to speed up compilation. Example usage:

[ccache-website]: https://ccache.samba.org/

```
$ make -f examples/Makefile-nrf52840 USB=1 CCPREFIX=ccache
```

## Native SPI Slave support

You can build the libraries with support for native SPI Slave.
To do so, build the libraries with the following parameter:
```
$ make -f examples/Makefile-nrf52840 NCP_SPI=1
```

With this option enabled, SPI communication between the NCP example and wpantund is possible
(provided that the wpantund host supports SPI Master). To achieve that, an appropriate SPI device
should be chosen in wpantund configuration file, `/etc/wpantund.conf`. You can find an example below.
```
Config:NCP:SocketPath "system:/usr/bin/spi-hdlc-adapter --gpio-int /sys/class/gpio/gpio25 /dev/spidev0.0"
```

[spi-hdlc-adapter][spi-hdlc-adapter]
is a tool that can be used to perform communication between NCP and wpantund over SPI.
In the above example it is assumed that `spi-hdlc-adapter` is installed in `/usr/bin`.

The default SPI Slave pin configuration for nRF52840 is defined in `examples/platforms/nrf52840/platform-config.h`.

Note that the native SPI Slave support is not intended to be used with Engineering sample A of the nRF52840 chip due to
single transfer size limitation.

[spi-hdlc-adapter]: https://github.com/openthread/openthread/tree/master/tools/spi-hdlc-adapter

## CryptoCell 310 support

By default, mbedTLS library is built with support for CryptoCell 310 hardware acceleration of cryptographic operations used in OpenThread. You can disable CryptoCell 310 and use software cryptography instead by building OpenThread with the following parameter:
```
$ make -f examples/Makefile-nrf52840 DISABLE_CC310=1
```

## Flashing the binaries

Flash the compiled binaries onto nRF52840 using `nrfjprog` which is
part of the [nRF5x Command Line Tools][nRF5x-Command-Line-Tools].

[nRF5x-Command-Line-Tools]: https://www.nordicsemi.com/eng/Products/nRF52840#Downloads

```bash
$ nrfjprog -f nrf52 --chiperase --program output/nrf52840/bin/ot-cli-ftd.hex --reset
```

## Running the example

1. Prepare two boards with the flashed `CLI Example` (as shown above).
2. The CLI example uses UART connection. To view raw UART output, start a terminal
   emulator like PuTTY and connect to the used COM port with the following UART settings:
    - Baud rate: 115200
    - 8 data bits
    - 1 stop bit
    - No parity
    - HW flow control: RTS/CTS

   On Linux system a port name should be called e.g. `/dev/ttyACM0` or `/dev/ttyACM1`.
3. Open a terminal connection on the first board and start a new Thread network.

 ```bash
 > panid 0xabcd
 Done
 > ifconfig up
 Done
 > thread start
 Done
 ```

4. After a couple of seconds the node will become a Leader of the network.

 ```bash
 > state
 Leader
 ```

5. Open a terminal connection on the second board and attach a node to the network.

 ```bash
 > panid 0xabcd
 Done
 > ifconfig up
 Done
 > thread start
 Done
 ```

6. After a couple of seconds the second node will attach and become a Child.

 ```bash
 > state
 Child
 ```

7. List all IPv6 addresses of the first board.

 ```bash
 > ipaddr
 fdde:ad00:beef:0:0:ff:fe00:fc00
 fdde:ad00:beef:0:0:ff:fe00:9c00
 fdde:ad00:beef:0:4bcb:73a5:7c28:318e
 fe80:0:0:0:5c91:c61:b67c:271c
 ```

8. Choose one of them and send an ICMPv6 ping from the second board.

 ```bash
 > ping fdde:ad00:beef:0:0:ff:fe00:fc00
 16 bytes from fdde:ad00:beef:0:0:ff:fe00:fc00: icmp_seq=1 hlim=64 time=8ms
 ```

For a list of all available commands, visit [OpenThread CLI Reference README.md][CLI].

[CLI]: ./../../../src/cli/README.md

## Logging module

By default, the OpenThread's logging module provides functions to output logging
information over SEGGER's Real Time Transfer (RTT).

RTT output can be viewed in the J-Link RTT Viewer, which is available from SEGGER.
The viewer is also included in the nRF Tools. To read or write messages over RTT,
connect an nRF5 development board via USB and run the J-Link RTT Viewer.

Select the correct target device (nRF52) and the target interface "SWD".

The intended log level can be set using `OPENTHREAD_CONFIG_LOG_LEVEL` define.

## Segger J-Link configuration

### Disabling the Mass Storage Device

Due to a known issue in Segger’s J-Link firmware, depending on your version, you might experience data corruption or drops if you use the serial port. You can avoid this issue by disabling the Mass Storage Device:

 - On Linux or macOS (OS X), open JLinkExe from the terminal.
 - On Microsoft Windows, open the J-Link Commander application.

Run the following command:

```bash
MSDDisable
```

This command takes effect after a power cycle of the development kit and stay this way until changed again.

### Hardware Flow Control detection

By default, J-Link detects at runtime if the target uses flow control or not.

Automatic HWFC detection is done by driving P0.07 (Clear to Send (CTS)) from the interface MCU and evaluating the state of P0.05 (Request to Send (RTS)) when the first data is sent or received. If the state of P0.05 (RTS) is high, HWFC is assumed not to be used.

To avoid potential race conditions, it is possible to force hardware flow control and bypass runtime auto-detection.

 - On Linux or macOS (OS X), open JLinkExe from the terminal.
 - On Microsoft Windows, open the J-Link Commander application.

Run the following command:

```bash
SetHWFC Force
```

The auto-detection feature may be enabled again by the following command:

```bash
SetHWFC Enable
```

You can find more details [here][J-Link_OB].

[J-Link_OB]: https://wiki.segger.com/J-Link_OB_SAM3U_NordicSemi#Hardware_flow_control_support

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

The following toolchains have been used for testing and verification:
  - gcc version 4.9.3
  - gcc version 6.2.0

 The following OpenThread commits have been verified with nRF52840 examples by Nordic Semiconductor:
  - `704511c` - 18.09.2018 (the newest checked)
  - `ec59d7e` - 06.04.2018
  - `a89eb88` - 16.11.2017
  - `6a15261` - 29.06.2017
  - `030efba` - 22.04.2017
  - `de48acf` - 02.03.2017
  - `50db58d` - 23.01.2017

# Nordic Semiconductor's nRF5 SDK for Thread and Zigbee

The [nRF5 Software Development Kit (SDK) for Thread and Zigbee][nRF5-SDK-Thread-Zigbee] helps you when developing Thread products with Nordic Semiconductor's advanced nRF52840 System on Chip (SoC).

The <i>nRF5 SDK for Thread and Zigbee</i> includes:
 - a pre-built OpenThread stack for the Nordic nRF52840 SoC with ARM® CryptoCell-310 support,
 - unique Thread/Bluetooth Low Energy dynamic multiprotocol solution which allows for concurrent operation of Thread and Bluetooth Low Energy utilizing OpenThread and SoftDevice (Nordic’s Bluetooth Low Energy stack) with accompanying example applications,
 - Thread/Bluetooth Low Energy switched multiprotocol solution with accompanying example applications,
 - unique support for DFU-over-Thread (Device Firmware Upgrade),
 - examples to demonstrate interactions between nodes performing different Thread roles with the use of OpenThread and CoAP or MQTT-SN protocols,
 - support for an OpenThread Network Co-Processor (NCP) using UART, USB or SPI transport protocol,
 - Border Router and cloud connectivity example,
 - Thread native commissioning with NFC example,
 - example applications demonstrating the use of FreeRTOS with OpenThread,
 - support for IAR, Keil MDK-ARM and Segger Embedded Studio (SES) IDEs for OpenThread stack and all example applications,
 - range of PC tools including a Thread Topology Monitor,
 - software modules inherited from the nRF5 SDK e.g. peripheral drivers, NFC libraries, Bluetooth Low Energy libraries etc.

[nRF5-SDK-Thread-Zigbee]: http://www.nordicsemi.com/eng/Products/nRF5-SDK-for-Thread
