# OpenThread on EFR32 Example

This directory contains example platform drivers for the [Silicon Labs EFR32][efr32mg]
based on [EFR32™ Mighty Gecko Wireless Starter Kit][SLWSTK6000B] or [Thunderboard™ Sense 2 Sensor-to-Cloud Advanced IoT Development Kit][SLTB004A].

[efr32mg]: http://www.silabs.com/products/wireless/mesh-networking/efr32mg-mighty-gecko-zigbee-thread-soc
[SLWSTK6000B]: http://www.silabs.com/products/development-tools/wireless/mesh-networking/mighty-gecko-starter-kit
[SLTB004A]: https://www.silabs.com/products/development-tools/thunderboard/thunderboard-sense-two-kit

The example platform drivers are intended to present the minimal code
necessary to support OpenThread. [EFR32MG12P SoC][efr32mg12p]
has rich memory and peripheral resources which can support all OpenThread
capabilities. See the "Run the example with EFR32 boards" section below
for an example using basic OpenThread capabilities.

[efr32mg12p]: http://www.silabs.com/products/wireless/mesh-networking/efr32mg-mighty-gecko-zigbee-thread-soc/device.EFR32MG12P432F1024GL125

## Toolchain

Download and install the [GNU toolchain for ARM Cortex-M][gnu-toolchain].

[gnu-toolchain]: https://launchpad.net/gcc-arm-embedded

In a Bash terminal, follow these instructions to install the GNU toolchain and
other dependencies.

```bash
$ cd <path-to-openthread>
$ ./script/bootstrap
```

## Build Examples

1. Download and install the [Simplicity Studio][simplicity_studio].

[simplicity_studio]: http://www.silabs.com/products/development-tools/software/simplicity-studio

2. Install Flex (Gecko) SDK including RAIL Library from Simplicity Studio.
   - Connect EFR32MG12P Wireless Starter Kit to Simplicity Studio.
   - Find Flex SDK v2.3 in the Software Update page and click Install.
   - Flex SDK v2.3 will be installed in the path: `/SimplicityStudio_v4/developer/sdks/gecko_sdk_suite`.

For more information on configuring, building, and installing applications for the Wireless Gecko (EFR32)
portfolio using RAIL, see [Getting Started with RAIL Library quick start guide][QSG121]. For more information
on RAIL, see [Radio Abstraction Interface Layer][rail].

[QSG121]: http://www.silabs.com/documents/public/quick-start-guides/QSG121-EFR32-RAIL.pdf
[rail]: http://www.silabs.com/products/development-tools/software/radio-abstraction-interface-layer-sdk

3. Configure the path to Flex SDK source code.
```bash
$ cd <path-to-openthread>/third_party
$ mkdir silabs
$ cd <path-to-Simplicity-Studio>/developer/sdks
$ cp -rf gecko_sdk_suite <path-to-openthread>/third_party/silabs/
```

4. Build OpenThread Firmware (CLI example) on EFR32 platform.
```bash
$ cd <path-to-openthread>
$ ./bootstrap
```
For EFR32™ Mighty Gecko Wireless Starter Kit:
```bash
$ make -f examples/Makefile-efr32 BOARD=BRD4161A
```
or alternatively for the Thunderboard™ Sense 2:

```bash
$ make -f examples/Makefile-efr32 BOARD=BRD4166A
```

After a successful build, the `elf` files are found in
`<path-to-openthread>/output/efr32/bin`.

## Flash Binaries

Compiled binaries may be flashed onto the EFR32 using [JLinkGDBServer][jlinkgdbserver].
EFR32 Starter kit mainboard integrates an on-board SEGGER J-Link debugger.

[jlinkgdbserver]: https://www.segger.com/jlink-gdb-server.html

```bash
$ cd <path-to-JLinkGDBServer>
$ sudo ./JLinkGDBServer -if swd -device EFR32MG12PxxxF1024
$ cd <path-to-openthread>/output/efr32/bin
$ arm-none-eabi-gdb ot-cli-ftd
$ (gdb) target remote 127.0.0.1:2331
$ (gdb) load
$ (gdb) monitor reset
$ (gdb) c
```

Note: Support for the "EFR32MG12PxxxF1024" device was added to JLinkGDBServer V6.14d.

Or

Compiled binaries also may be flashed onto the specified EFR32 dev borad using [J-Link Commander][j-link-commander].

[j-link-commander]: https://www.segger.com/products/debug-probes/j-link/tools/j-link-commander/

```bash
$ cd <path-to-openthread>/output/efr32/bin
$ arm-none-eabi-objcopy -O ihex ot-cli-ftd ot-cli-ftd.hex
$ JLinkExe -device EFR32MG12PxxxF1024 -speed 4000 -if SWD -autoconnect 1 -SelectEmuBySN <SerialNo>
$ J-Link>loadfile ot-cli-ftd.hex
$ J-Link>r
$ J-Link>q
```

Note: SerialNo is J-Link serial number. Use the following command to get the serial number of the connected J-Link.

```bash
$ JLinkExe
```

## Run the example with EFR32 boards
1. Flash two EFR32 boards with the `CLI example` firmware (as shown above).
2. Open terminal to first device `/dev/ttyACM0` (serial port settings: 115200 8-N-1).
   Type `help` for a list of commands.

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

3. Start a Thread network as Leader.

```bash
> panid 0xface
Done
> ifconfig up
Done
> thread start
Done

wait a couple of seconds...

> state
leader
Done
```

4. Open terminal to second device `/dev/ttyACM1` (serial port settings: 115200 8-N-1)
   and attach it to the Thread network as a Router.

```bash
> panid 0xface
Done
> routerselectionjitter 1
Done
> ifconfig up
Done
> thread start
Done

wait a couple of seconds...

> state
router
Done
```

5. List all IPv6 addresses of Leader.

```bash
> ipaddr
fdde:ad00:beef:0:0:ff:fe00:fc00
fdde:ad00:beef:0:0:ff:fe00:800
fdde:ad00:beef:0:5b:3bcd:deff:7786
fe80:0:0:0:6447:6e10:cf7:ee29
Done
```

6. Send an ICMPv6 ping to Leader's Mesh-EID IPv6 address.

```bash
> ping fdde:ad00:beef:0:5b:3bcd:deff:7786
8 bytes from fdde:ad00:beef:0:5b:3bcd:deff:7786: icmp_seq=1 hlim=64 time=24ms
```

The above example demonstrates basic OpenThread capabilities. Enable more features/roles (e.g. commissioner,
joiner, DHCPv6 Server/Client, etc.) by assigning compile-options before compiling.

```bash
$ cd <path-to-openthread>
$ ./bootstrap
$ make -f examples/Makefile-efr32 COMMISSIONER=1 JOINER=1 DHCP6_CLIENT=1 DHCP6_SERVER=1
```

For a list of all available commands, visit [OpenThread CLI Reference README.md][CLI].

[CLI]: https://github.com/openthread/openthread/blob/master/src/cli/README.md

## Verification

The following toolchain has been used for testing and verification:
   - gcc version 4.9.3
   - gcc version 5.4.1

The EFR32 example has been verified with following Flex SDK/RAIL Library version:
   - Flex SDK version 2.0.0.0
