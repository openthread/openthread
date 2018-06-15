# OpenThread on POSIX

This directory contains platform drivers for POSIX with an external radio tranceiver.

## Build

```bash
$ cd <path-to-openthread>
$ ./bootstrap
$ make -f src/posix/Makefile-posix
```

After a successful build, the `elf` files are found in
`<path-to-openthread>/output/posix/<platform>/bin`.

## 

## Interact

1. Spawn the process:

`stty` is required.

```bash
$ cd <path-to-openthread>/output/<platform>/bin
$ ./ot-cli /dev/ttyUSB0 115200
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
netdataregister
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
whitelist
```
