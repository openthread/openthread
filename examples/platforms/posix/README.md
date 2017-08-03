# OpenThread on POSIX Emulation Example

This directory contains example platform drivers for POSIX emulation.

## Build Examples

```bash
$ cd <path-to-openthread>
$ ./bootstrap
$ make -f examples/Makefile-posix
```

After a successful build, the `elf` files are found in
`<path-to-openthread>/output/<platform>/bin`.

## 

## Interact

1. Spawn the process:

```bash
$ cd <path-to-openthread>/output/<platform>/bin
$ ./ot-cli-ftd 1
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
