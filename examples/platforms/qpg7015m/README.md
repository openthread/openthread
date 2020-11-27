# OpenThread on Qorvo qpg7015m Example

This directory contains example platform drivers for Qorvo qpg7015m on RPi.

## Toolchain

This example uses the GNU GCC toolchain on the Raspberry Pi. To build on the Pi:

1. Download the repo to the Pi
2. go to the subfolder in the openthread repo: third_party/nlbuild-autotools/repo/tools/packages and enter this command:

```bash
$ sudo /bin/bash build
```

Note that you may need to install additional packages to make this build work, depending on your actual RPi OS version. The build process will complain if additional packages are required.

## Build Examples

```bash
$ cd <path-to-openthread>
$ ./script/bootstrap
$ ./bootstrap
$ REFERENCE_DEVICE=1 CLI_LOGGING=1 COMMISSIONER=1 JOINER=1 DHCP6_CLIENT=1 DHCP6_SERVER=1 BORDER_ROUTER=1 make -f examples/Makefile-qpg7015m
```

After a successful build, the `elf` files are found in `<path-to-openthread>/output/qpg7015m/bin`.

Building a variant which interfaces via a tcp socket is also possible. Replace the uart-posix.c with uart-socket.c in the Makefile.am from examples/platforms/qpg7015m/Makefile.am and rebuild. Now it should be possible to open a telnet to socket 9190 of the raspberry pi from a remote PC. This also easier testing with the official Thread Test Harness.

##

## Interact

1. Spawn the process:

```bash
$ cd <path-to-openthread>/output/qpg7015m/bin
$ ./qpg7015m-ot-cli-ftd
```

2. Type `help` for list of commands.

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
