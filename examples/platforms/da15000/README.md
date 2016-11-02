# Dialog OpenThread DA1500 Example

This directory contains example platform drivers for the [Dialog Semiconductor DA1500][da15000]

[da15000]: https://support.dialog-semiconductor.com/connectivity/product/openthread-sandbox

## Build Examples (How to build and flash):

# cd <path-to-openthread>
$ ./bootstrap
$ make -f examples/Makefile-da15000 clean
$ make -f examples/Makefile-da15000

Execute script to prepare image for flashing:

$ cd ./third_party/dialog/DialogTools
$ ./imgprep.sh

Flash Binaries:

1. Open another terminal window in order to start gdb server. Write the command (Linux):
$ JLinkGDBServer -ir -if swd -speed auto -device Cortex-M0 -logtofile on -localhostonly

2. Return to previous terminal window and execute a command in order to flash the board:
$ ./cli_programmer -b uartboot.bin gdbserver write_qspi 0x0 ../../../output/bin/arm-none-eabi-ot-cli.img


## Interact:

******************************************************************************
*  NOTE:    Open Thread used in this example is updated to:                  *
*  a853ffc spinel-cli: Add ipaddr removal to prefix change handler. (#669)   *
******************************************************************************


Board will indicate state of device according to LED blink speed.
5Hz   - Leader
2Hz   - Router
0.5Hz - End Device


1. Open terminal to /dev/ttyACM0 (serial port settings: 9600 8-N-1).

2. Type help for list of commands

I. Example - Create Topology and become Leader on channel 13.

> channel 13
Done
> panid 0xface
Done
> ifconfig up
Done
> thread start
Done

Wait and see that LED started to blink with 5Hz.

Check node state:
> state
Leader

II. Attach Router and ping Leader Mesh Local address.

Open terminal for second device /dev/ttyACM1 and issue commands:

> channel 13
Done
> panid 0xface
Done
> ifconfig up
Done
> thread start

Wait until LED start to blink. Should blink slower.

Check node state:
> state
Router

List addresses on terminal with Leader:
> ipaddr
fdde:ad00:beef:0:0:ff:fe00:4400
fdde:ad00:beef:0:92f5:9844:67ad:a0c9
fe80:0:0:0:2c11:aba1:ecc1:96da

Ping Leader from Router terminal:
> ping fdde:ad00:beef:0:92f5:9844:67ad:a0c9
16 bytes from fdde:ad00:beef:0:92f5:9844:67ad:a0c9: icmp_seq=2 hlim=64 time=359ms <- Example response
