# OpenThread Simulation on POSIX

This directory contains example platform drivers for simulation on POSIX.

## Build Examples

```bash
$ cd <path-to-openthread>
$ mkdir build && cd build
$ cmake -GNinja -DOT_PLATFORM=simulation ..
$ ninja
```

After a successful build, the `elf` files are found in:

- `<path-to-openthread>/build/examples/apps/cli`
- `<path-to-openthread>/build/examples/apps/ncp`

## Interact

1. Spawn the process:

```bash
$ cd <path-to-openthread>/build/simulation/examples/apps/cli
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
mode
netdata register
networkidtimeout
networkkey
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
