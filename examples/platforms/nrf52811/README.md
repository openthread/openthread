# OpenThread on nRF52811 Example

This SoC is meant to be used in the configuration that involves the Host Processor and the IEEE 802.15.4 radio.
In this configuration, the full OpenThread stack is running on the Host Processor and the nRF52811 SoC acts as an IEEE 802.15.4 radio.
The radio is running a minimal OpenThread implementation that allows for communication between the Host Processor and the nRF52811. In this architecture the nRF52811 SoC device is called NCP radio or RCP (Radio Co-Processor).

The nRF52811 platform is currently under development.

For the SoC capable to a run full OpenThread stack please check the [nRF52840 platform][nRF52840-page].

[nRF52840-page]: ./../nrf52840/README.md

## Toolchain

Download and install the [GNU toolchain for ARM Cortex-M][gnu-toolchain].

[gnu-toolchain]: https://launchpad.net/gcc-arm-embedded

To install the GNU toolchain and its dependencies,
run the following commands in Bash:

```bash
$ cd <path-to-openthread>
$ ./script/bootstrap
```

## Building the examples

With this platform you can build:
 - Limited version of CLI example (e.g. without Thread commissioning functionality)
 - NCP radio-only example that consists of two parts:
    - firmware that is flashed to the nRF52811 SoC
    - host executables to be executed on a POSIX platform in case of the RCP usage

### Building the firmware

To build the firmware using default UART RCP transport, run the following make commands:

```bash
$ cd <path-to-openthread>
$ ./bootstrap
$ make -f examples/Makefile-nrf52811
```

After a successful build, the `elf` files can be found in
`<path-to-openthread>/output/nrf52811/bin`.  
You can convert them to hex using arm-none-eabi-objcopy`:
```bash
$ arm-none-eabi-objcopy -O ihex ot-cli-mtd ot-cli-mtd.hex
$ arm-none-eabi-objcopy -O ihex ot-ncp-radio ot-ncp-radio.hex
```

### Building host executables
```bash
$ cd <path-to-openthread>
$ ./bootstrap
$ make -f src/posix/Makefile-posix
```

After a successful build, executables can be found in
`<path-to-openthread>/openthread/output/posix/<system-architecture>/bin`.
It is recommended to copy them to `/usr/bin` for easier access.

## Building the examples with native SPI Slave support

You can build the libraries with support for the native SPI Slave.
To do so, build the libraries with the following parameter:
```
$ make -f examples/Makefile-nrf52811 NCP_SPI=1
```

With the `NCP_SPI` option enabled, SPI communication between the RCP example and wpantund is possible
(provided that the wpantund host supports SPI Master).
To achieve the communication between the RCP example and wpantund,
choose an appropriate SPI device in the wpantund configuration file,
`/etc/wpantund.conf`. See the following example.

```
Config:NCP:SocketPath "system:/usr/bin/ot-ncp /usr/bin/spi-hdlc-adapter -- '--stdio -i /sys/class/gpio/gpio25 /dev/spidev0.1'"
```

[spi-hdlc-adapter][spi-hdlc-adapter]
is a tool that can be used to perform communication between RCP and wpantund over SPI.
In this example, `spi-hdlc-adapter` is installed in `/usr/bin`.

The default SPI Slave pin configuration for nRF52811 is defined in `examples/platforms/nrf52811/platform-config.h`.

[spi-hdlc-adapter]: https://github.com/openthread/openthread/tree/master/tools/spi-hdlc-adapter

### Building the host executables

To build the host executables, run the following make commands:

```bash
$ cd <path-to-openthread>
$ ./bootstrap
$ make -f src/posix/Makefile-posix
```

After a successful build, executables can be found in
`<path-to-openthread>/openthread/output/posix/<system-architecture>/bin`.
Copy the files to /usr/bin for easier access.

## Flashing binaries

Once the examples and libraries are built, flash the compiled binaries onto nRF52811
using `nrfjprog` that is part of the [nRF5x Command Line Tools][nRF5x-Command-Line-Tools].

[nRF5x-Command-Line-Tools]: https://www.nordicsemi.com/eng/Products/nRF52840#Downloads

Run the following command:

```bash
$ nrfjprog -f nrf52 --chiperase --program output/nrf52811/bin/ot-cli-mtd.hex --reset
```

## Running the example

To test the example:

1. Prepare two boards.

2. Flash one with the `CLI MTD Example` (ot-cli-mtd.hex, as shown above).

3. Flash `RCP Example` (ot-ncp-radio.hex) to the other board.

4. Connect the RCP to the PC.

5. Start the ot-cli host application and connect with the RCP.
   It is assumed that the default UART version of RCP is being used (make executed without the NCP=SPI=1 flag).
   On Linux system, call a port name, for example `/dev/ttyACM0` for the first connected development kit,
   and `/dev/ttyACM1` for the second one.

```bash
$ /usr/bin/ot-cli /dev/ttyACM0 115200
```

Now you are connected with the CLI.

6. Use the following commands to form a network:

```bash
 > panid 0xabcd
 Done
 > ifconfig up
 Done
 > thread start
 Done
```

7. Open a terminal connection on the first board and start a new Thread network:

 ```bash
 > panid 0xabcd
 Done
 > ifconfig up
 Done
 > thread start
 Done
 ```

After a couple of seconds the node will become a Leader of the network.

 ```bash
 > state
 Leader
 ```

8. Open a terminal connection on the second board and attach a node to the network.
   The CLI MTD example uses the direct UART connection. To view raw UART output, start a terminal
   emulator like screen and connect to the used COM port with the following UART settings:
    - Baud rate: 115200
    - 8 data bits
    - 1 stop bit
    - No parity
    - HW flow control: RTS/CTS

Run the following to connect to the second board.

 ```bash
 screen /dev/ttyACM1 115200
 ```

Now you are connected with the CLI.

9. Use the following commands to form a network:

 ```bash
 > panid 0xabcd
 Done
 > ifconfig up
 Done
 > thread start
 Done
 ```

After a couple of seconds the second node will attach and become a Child.

 ```bash
 > state
 Child
 ```

10. List all IPv6 addresses of the first board.

 ```bash
 > ipaddr
 fdde:ad00:beef:0:0:ff:fe00:fc00
 fdde:ad00:beef:0:0:ff:fe00:9c00
 fdde:ad00:beef:0:4bcb:73a5:7c28:318e
 fe80:0:0:0:5c91:c61:b67c:271c
 ```

11. Choose one of them and send an ICMPv6 ping from the second board.

 ```bash
 > ping fdde:ad00:beef:0:0:ff:fe00:fc00
 16 bytes from fdde:ad00:beef:0:0:ff:fe00:fc00: icmp_seq=1 hlim=64 time=8ms
 ```

For a list of all available commands, visit [OpenThread CLI Reference README.md][CLI].

[CLI]: ./../../../src/cli/README.md

## Working with the logging module

By default, the OpenThread's logging module provides functions to output logging
information over SEGGER's Real Time Transfer (RTT).

The RTT output can be viewed in the J-Link RTT Viewer, which is available from SEGGER.

The viewer is also included in the nRF Tools. 
To read or write messages over RTT:

1. Connect an nRF5 development board via with a USB cable. 
2. Run the J-Link RTT Viewer.
3. In the Specify Target Device dropdown menu, select the nRF52 device.
4. In the Target Interface & Speed dropdown menu, select the SWD interface.

The intended log level can be set using `OPENTHREAD_CONFIG_LOG_LEVEL` define.

## Disabling the Mass Storage Device

Depending on your software version, a known issue in SEGGER's J-Link firmware can cause data corruption or data drops if you use the serial port. You can avoid this issue by disabling the Mass Storage Device:

 - On Linux or macOS (OS X), open JLinkExe from the terminal and run the command `MSDDisable`.
 - On Microsoft Windows, open the J-Link Commander application and run the command `MSDDisable`.

## Diagnostic module

nRF52811 supports [OpenThread Diagnostics Module][DIAG], with some additional features.

For more information, see [nRF Diag command reference][nRFDIAG].

[DIAG]: ./../../../src/diag/README.md
[nRFDIAG]: DIAG.md

## Radio driver documentation

The radio driver documentation includes *.uml state machines sequence diagrams that can be opened with [PlantUML][PlantUML-url].

[PlantUML-url]: http://plantuml.com/
