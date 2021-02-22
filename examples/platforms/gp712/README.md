# OpenThread on Qorvo gp712 Example

This directory contains example platform drivers for Qorvo gp712 on RPi.

## Toolchain

This example uses the GNU GCC toolchain on the Raspberry Pi. To build on the Pi:

1. Download the repo to the Pi
2. go to the subfolder in the openthread repo: third_party/nlbuild-autotools/repo/ and enter this command:

```bash
$ make tools
```

Note that you may need to install additional packages to make this build work, depending on your actual RPi OS version. The build process will complain if additional packages are required.

## Build Examples

```bash
$ cd <path-to-openthread>
$ ./script/bootstrap
$ ./bootstrap
$ REFERENCE_DEVICE=1 CLI_LOGGING=1 COMMISSIONER=1 JOINER=1 DHCP6_CLIENT=1 DHCP6_SERVER=1 BORDER_ROUTER=1 make -f examples/Makefile-gp712
```

After a successful build, the `elf` files are found in `<path-to-openthread>/output/gp712/bin`.

##

## Interact

1. Spawn the process:

```bash
$ cd <path-to-openthread>/output/gp712/bin
$ ./gp712-ot-cli-ftd
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
