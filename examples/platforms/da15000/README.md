# Dialog OpenThread DA15000 Example

This directory contains example platform drivers for the [Dialog Semiconductor DA15000][da15000]

[da15000]: https://support.dialog-semiconductor.com/connectivity/product/openthread-sandbox

**NOTE:** Each Thread node requires a unique EUI-64.
Please make sure all Thread nodes in your network have a unique EUI-64 by setting HARDCODED_NODE_ID in radio.c to a unique value.

**NOTE:** Current version works only with DA15000

## Toolchain

Download and install the [GNU toolchain for ARM
Cortex-M][gnu-toolchain].

[gnu-toolchain]: https://launchpad.net/gcc-arm-embedded

In a Bash terminal, follow these instructions to install the GNU toolchain and
other dependencies.

```bash
$ cd <path-to-openthread>
$ ./script/bootstrap
```

## Build Examples (How to build and flash):
```bash
$ cd <path-to-openthread>
$ ./bootstrap
$ make -f examples/Makefile-da15000 clean
$ make -f examples/Makefile-da15000
```

Execute script to prepare image for flashing:

**NOTE:** Script is based on Dialog Tools binaries compiled for x86 Linux.

```bash
$ cd ./third_party/dialog/DialogTools
$ ./imgprep.sh
```

Flash Binaries:

**NOTE:** Flashing process requires JLinkGDBServer. Please visit https://www.segger.com/jlink-gdb-server.html for details.

1. Open another terminal window in order to start gdb server. Write the command (Linux):

   ```bash
   $ JLinkGDBServer -ir -if swd -speed auto -device Cortex-M0 -logtofile on -localhostonly
   ```
2. Return to previous terminal window and execute a command in order to flash the board:

   ```bash
   $ ./cli_programmer gdbserver write_qspi_exec ../../../output/da15000/bin/ot-cli-ftd.bin
   ```

## Interact:

Board will indicate state of device according to LED blink speed.

| Frequency | Role |
| --- | --- |
| 5Hz | Leader |
| 2Hz | Router |
| 0.5Hz | End Device |


1. Open terminal to /dev/ttyACM0 (serial port settings: 115200 8-N-1).
2. Type help for list of commands

## Examples:
1. Create Topology and become Leader on channel 13.
   ```
   > channel 13
   Done
   > panid 0xface
   Done
   > ifconfig up
   Done
   > thread start
   Done
   ```
   Wait and see that LED started to blink with 5Hz.

   Check node state:
   ```
   > state
   Leader
   ```
2. Attach Router and ping Leader Mesh Local address.

   Open terminal for second device /dev/ttyACM1 and issue commands:
   ```
   > channel 13
   Done
   > panid 0xface
   Done
   > ifconfig up
   Done
   > thread start
   ```

   Wait until LED start to blink. Should blink slower.

   Check node state:

   ```
   > state
   Router
   ```

   List addresses on terminal with Leader:

   ```
   > ipaddr
   fdde:ad00:beef:0:0:ff:fe00:4400
   fdde:ad00:beef:0:92f5:9844:67ad:a0c9
   fe80:0:0:0:2c11:aba1:ecc1:96da
   ```

   Ping Leader from Router terminal:

   ```
   > ping fdde:ad00:beef:0:92f5:9844:67ad:a0c9
   16 bytes from fdde:ad00:beef:0:92f5:9844:67ad:a0c9: icmp_seq=2 hlim=64 time=359ms
   ```

## Remarks
* Validation

   The DA15000 example has been validated by Dialog Semiconductor with commit 1f6528e included.

* Build environment

   The DA15000 example code and all required dependencies have been complied with gcc version 5.4.0 (20160609).

## Troubleshooting
* Why can’t I see the VCOM port?

   The Segger Virtual COM port can be disabled by software configuration. If you can see the Segger
   driver properly installed but the VCOM not showing up in the device manager, it’s probably disabled
   by software. To re-enable it you have to run JLinkExe and issue the command ` vcom enable`.It is
   required to power cycle the device to apply the change.

