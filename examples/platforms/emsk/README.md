# OpenThread on EMSK Example

This directory contains example platform drivers for Synopsys [DesignWare® ARC® EM][arcem] Starter Kit 2.3[(EMSK2.3)][emsk23] and Digilent [Pmod RF2][pmodrf2] equipped with the Microchip [MRF24J40][mrf24j40] IEEE 802.15.4™ 2.4GHz RF transceiver.

[arcem]: https://www.synopsys.com/designware-ip/processor-solutions/arc-processors/arc-em-family.html

[emsk23]: https://www.synopsys.com/dw/ipdir.php?ds=arc_em_starter_kit

[pmodrf2]: https://reference.digilentinc.com/reference/pmod/pmodrf2/start

[mrf24j40]: http://www.microchip.com/wwwproducts/en/en027752

## Note

The EMSK platform is a configurable, FPGA-based software development platform for the ARC EM Processor Family. Please note that:

* The platform does not currently feature a True Random Number Generator (TRNG), which is required for certification to the Thread Specification. Users looking to certify the OpenThread implementation for their own ARC-based platforms will need to add a TRNG to their platform and modify our reference implementation based on EMSK accordingly in order to achieve certification for their platform.
* The platform does not currently contain a on-chip EUI64, which is required for a network device. Users will need to add a unique number as EUI64 for their own ARC-based platform in `<path-to-openthread>/examples/platforms/emsk/radio.c`. It can be set manually or loaded from non-volatile memory.

## Toolchain

Download and install [GNU toolchain for ARC EM][gnu-toolchain].

[gnu-toolchain]: https://github.com/foss-for-synopsys-dwc-arc-processors/toolchain/tags

Download and install [Digilent Adept Software][digilent-adept] for the Digilent JTAG-USB cable.

[digilent-adept]: https://store.digilentinc.com/digilent-adept-2-download-only

## Building the examples

```bash
$ cd <path-to-openthread>
$ ./script/bootstrap
$ ./bootstrap
$ make -f examples/Makefile-emsk clean
$ make -f examples/Makefile-emsk
```

After a successful build, the `elf` files can be found in
`<path-to-openthread>/output/arc-elf32/bin`.  You can convert them to `bin`
files using `arc-elf32-objcopy`:
```bash
$ arc-elf32-objcopy -O binary ot-cli-ftd ot-cli-ftd.bin
```

## Booting the binaries from SD card

embARC [embarc] is an open software platform to facilitate the development of embedded systems based on ARCv2 processors. Download embARC [embarc] to generate and flash the EMSK bootloader from source code. See the application note [Using a secondary bootloader on the EMSK][bootloader-appnote] for details.

[embarc]: https://embarc.org/index.html

[bootloader-appnote]: https://embarc.org/pdf/20150710_embARC_application_note_secondary_bootloader.pdf

Modify the file name `arc-elf32-ot-cli-ftd.bin` to `boot.bin`. Copy `boot.bin` to the SD card. The EMSK2.3 includes an SPI flash storage device pre-programmed with FPGA images containing different configurations of DesignWare® ARC EM cores. Set pins 1 and 4 of `SW1` DIP switch and press `FPGA configure` button to configure the ARC EM11D and secondary bootloader.

## Running the example

1. Prepare two EMSK2.3 boards and copy the `boot.bin` (CLI example) to the EMSK2.3 SD card (as shown above).

2. The CLI example uses UART connection. To view raw UART output, start a terminal emulator like Tera Term and PuTTY. Connect EMSK J7 to the COM port with the following serial port settings:
    - Baud rate: 115200
    - 8 data bits
    - 1 stop bit
    - No parity
    - No flow control

3. Open a terminal connection on the first board. The `boot.bin` in the SD card will be loaded to the EMSK after a few seconds.

 ```bash
 MRF24J40 Init started.
 MRF24J40 Init finished.
 Node No. :
 ```

4. Set the node number manually. Input `1` and press the `Enter` key in the terminal emulator.

 ```bash
 Node No. :1
 OpenThread Init Finished
 ```

4. Start a new Thread network.

 ```bash
 > panid 0x1234
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

5. Open a terminal connection and reset the second board. After a few seconds, input `2` and press the `Enter` key in the terminal emulator.

 ```bash
 MRF24J40 Init started.
 MRF24J40 Init finished.
 Node No. :2
 OpenThread Init Finished
 ```

6. Attach the second node to the network.

 ```bash
 > panid 0x1234
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
 fdde:ad00:beef:0:0:ff:fe00:ec00
 fdde:ad00:beef:0:4f6e:7e53:67c8:f5b0
 fe80:0:0:0:c462:f165:44eb:ef9f
 ```

8. Choose one of them and send an ICMPv6 ping from the second board.

 ```bash
 > ping fdde:ad00:beef:0:0:ff:fe00:ec00
 8 bytes from fdde:ad00:beef:0:0:ff:fe00:ec00: icmp_seq=1 hlim=64 time=30ms
 ```

For a list of all available commands, visit [OpenThread CLI Reference README.md][CLI].

[CLI]: https://github.com/openthread/openthread/blob/master/src/cli/README.md

## Verification

The following hardware have been used for testin and verification:
  - ARC EM11D on EMSK2.3
  - Pmod RF2

The following toolchains and software have been used for testing and verification:
  - arc-2016.09-release
  - embARC 2016.05

The EMSK example has been verified by Synopsys with commit `064aba2`.
