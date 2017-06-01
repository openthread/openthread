# OpenThread on nRF52840 Example

This directory contains example platform drivers for [Nordic Semiconductor nRF52840 SoC][nRF52840].

[nRF52840]: https://www.nordicsemi.com/eng/Products/nRF52840

To facilitate Thread products development with the nRF52840 platform, Nordic Semiconductor provides <i>nRF5 SDK for Thread</i>. See [Nordic Semiconductor's nRF5 SDK for Thread][nRF5-SDK-section] section for more details.

[nRF5-SDK-section]: #nordic-semiconductors-nrf5-sdk-for-thread

## Toolchain

Download and install [GNU toolchain for ARM Cortex-M][gnu-toolchain].

[gnu-toolchain]: https://launchpad.net/gcc-arm-embedded

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

## Disabling the Mass Storage Device

Due to a known issue in Segger’s J-Link firmware, depending on your version, you might experience data corruption or drops if you use the serial port. You can avoid this issue by disabling the Mass Storage Device:

 - On Linux or macOS (OS X), open JLinkExe from the terminal.
 - On Microsoft Windows, open the J-Link Commander application.

Run the following command: `MSDDisable`

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

The nRF52840 example has been verified by Nordic Semiconductor with following commits:
  - `030efba` - 22.04.2017 (the newest checked)
  - `de48acf` - 02.03.2017
  - `50db58d` - 23.01.2017

# Nordic Semiconductor's nRF5 SDK for Thread

The [nRF5 Software Development Kit (SDK) for Thread][nRF5-SDK-Thread] helps you when developing Thread products with Nordic Semiconductor's advanced nRF52840 System on Chip (SoC).

The <i>nRF5 SDK for Thread</i> includes:
 - a pre-built OpenThread stack for the Nordic nRF52840 SoC with ARM® CryptoCell-310 support,
 - unique support for DFU-over-Thread (Device Firmware Upgrade),
 - examples to demonstrate interactions between nodes performing different Thread roles with the use of OpenThread and built-in CoAP protocol,
 - examples to demonstrate multiprotocol support and switching between BLE peripheral
  and Thread FTD and MTD roles,
 - support for an OpenThread Network Co-Processor (NCP),
 - Border Router and cloud connectivity example,
 - Thread native commissioning with NFC example,
 - range of PC tools including a Thread Topology Monitor,
 - software modules inherited from the nRF5 SDK e.g. peripheral drivers, NFC libraries etc.

#### Important notice

Due to legal restrictions support for hardware-accelerated cryptography utilizing ARM CryptoCell-310 is only available in mbedTLS library (libmbedcrypto.a) provided with the <i>nRF5 SDK for Thread</i>. The library available in OpenThread repository does not support hardware acceleration. You can still use it, but the commissioning procedure takes much more time in such case. For the best performance and user experience, use the library provided with the SDK.

[nRF5-SDK-Thread]: http://www.nordicsemi.com/eng/Products/nRF5-SDK-for-Thread
