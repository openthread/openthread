# OpenThread Simulation on POSIX

This directory contains example platform drivers for simulation on POSIX.

## Build Examples

### Build using autotools

```bash
$ cd <path-to-openthread>
$ ./bootstrap
$ make -f examples/Makefile-simulation
```

After a successful build, the `elf` files are found in:

- `<path-to-openthread>/output/<platform>/bin`

### Build using cmake/ninja

```bash
$ cd <path-to-openthread>
$ mkdir build && cd build
$ cmake -GNinja -DOT_PLATFORM=simulation ..
$ ninja
```

After a successful build, the `elf` files are found in:

- `<path-to-openthread>/build/examples/apps/cli`
- `<path-to-openthread>/build/examples/apps/ncp`

### Build using cmake build script for use in OT-NS

Below shows an example build for OT-NS that has support for OT-external RF/timing models.
The build option OT_FULL_LOGS can also be set to 'ON' in this case, for extra debug info.

```bash
$ cd <path-to-openthread>
$ ./script/cmake-build simulation -DOT_PLATFORM=simulation -DOT_OTNS=ON -DOT_SIMULATION_EXT_RF_MODELS=ON\
 -DOT_SIMULATION_VIRTUAL_TIME=ON -DOT_SIMULATION_VIRTUAL_TIME_UART=ON -DOT_SIMULATION_MAX_NETWORK_SIZE=999 \
 -DOT_COMMISSIONER=ON -DOT_JOINER=ON -DOT_BORDER_ROUTER=ON -DOT_SERVICE=ON -DOT_COAP=ON -DOT_FULL_LOGS=OFF
```

Below shows an example build for OT-NS with default (ideal) OT RF/timing model.
The build option OT_FULL_LOGS must be set to 'OFF' in this case.

```bash
$ cd <path-to-openthread>
$ ./script/cmake-build simulation -DOT_PLATFORM=simulation -DOT_OTNS=ON \
 -DOT_SIMULATION_VIRTUAL_TIME=ON -DOT_SIMULATION_VIRTUAL_TIME_UART=ON -DOT_SIMULATION_MAX_NETWORK_SIZE=999 \
 -DOT_COMMISSIONER=ON -DOT_JOINER=ON -DOT_BORDER_ROUTER=ON -DOT_SERVICE=ON -DOT_COAP=ON -DOT_FULL_LOGS=OFF
```

After a successful build, the `elf` files are found in:

- `<path-to-openthread>/build/<platform>/examples/apps/cli`
- `<path-to-openthread>/build/<platform>/examples/apps/ncp`

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
