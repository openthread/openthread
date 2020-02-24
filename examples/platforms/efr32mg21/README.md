# OpenThread on EFR32MG21 Example

This directory contains example platform drivers for the [Silicon Labs EFR32MG21][efr32mg]
based on [EFR32™ Mighty Gecko Wireless Starter Kit][SLWSTK6000B] 

[efr32mg]: http://www.silabs.com/products/wireless/mesh-networking/efr32mg-mighty-gecko-zigbee-thread-soc
[SLWSTK6000B]: http://www.silabs.com/products/development-tools/wireless/mesh-networking/mighty-gecko-starter-kit

The example platform drivers are intended to present the minimal code
necessary to support OpenThread. [EFR32MG21 SoC][efr32mg21]
has rich memory and peripheral resources which can support all OpenThread
capabilities. See the "Run the example with EFR32 boards" section below
for an example using basic OpenThread capabilities.

[efr32mg21]: https://www.silabs.com/products/wireless/mesh-networking/series-2-efr32-mighty-gecko-zigbee-thread-soc/device.efr32mg21a020f768im32

## Toolchain

Download and install the [GNU toolchain for ARM Cortex-M][gnu-toolchain].

[gnu-toolchain]: https://developer.arm.com/open-source/gnu-toolchain/gnu-rm

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
   - Connect EFR32MG21 Wireless Starter Kit to Simplicity Studio.
   - Find Flex SDK v2.7 in the Software Update page and click Install.
   - Flex SDK v2.7 will be installed in the path: `/SimplicityStudio_v4/developer/sdks/gecko_sdk_suite`.

For more information on configuring, building, and installing applications for the Wireless Gecko (EFR32)
portfolio using FLEX, see [Getting Started with the Silicon Labs Flex Software Development Kit for the 
Wireless Gecko (EFR32™) Portfolio][QSG138]. For more information
on RAIL, see [Radio Abstraction Interface Layer][rail].

[QSG138]: https://www.silabs.com/documents/public/quick-start-guides/qsg138-flex-efr32.pdf
[rail]: http://www.silabs.com/products/development-tools/software/radio-abstraction-interface-layer-sdk

3. Configure the path to Flex SDK source code.
```bash
$ cd <path-to-Simplicity-Studio>/developer/sdks
$ cp -rf gecko_sdk_suite <path-to-openthread>/third_party/silabs/
```

Alternatively create a symbolic link to the Flex SDK source code.
```bash
$ cd <path-to-openthread>/third_party
$ ln -s <path-to-Simplicity-Studio>/developer/sdks/gecko_sdk_suite silabs/gecko_sdk_suite
```

Note: Due to an error in the core_cm33.h file provided by ARM, the compiler will throw an error when pedantic
option is used on the builds. To avoid this, please add the following lines of code at the top of the
file core_cm33.h:

```
#if defined(__GNUC__)
#pragma GCC diagnostic ignored "-Wpedantic"
#endif
```

core_cm33.h can be found at:

```
<path-to-Simplicity-Studio>/developer/sdks/gecko_sdk_suite/v2.7/platform/CMSIS/Include
```

4. Build OpenThread Firmware (CLI example) on EFR32 platform.
```bash
$ cd <path-to-openthread>
$ ./bootstrap
```
For EFR32MG21™ Mighty Gecko Wireless Starter Kit:
```bash
$ make -f examples/Makefile-efr32mg21 BOARD=BRD4180A
```

After a successful build, the `elf` files are found in
`<path-to-openthread>/output/efr32mg21/bin`.

## Flash Binaries

Simplicity Commander provides a graphical interface for J-Link Commander.

```bash
$ cd <path-to-openthread>/output/efr32mg21/bin
$ arm-none-eabi-objcopy -O ihex ot-cli-ftd ot-cli-ftd.hex
$ <path-to-simplicity-studio>/developer/adapter_packs/commander/commander
```

In the J-Link Device drop-down list select the serial number of the device to flash.  Click the Adapter Connect button.
Esnure the Debug Interface drop-down list is set to SWD and click the Target Connect button.
Click on the Flash icon on the left side of the window to switch to the flash page.
In the Flash MCU pane enter the path of the ot-cli-ftd.s37 file or choose the file with the Browse... button.
Click the Flash button located under the Browse... button.

## Run the example with EFR32MG21 boards
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
$ make -f examples/Makefile-efr32mg21 COMMISSIONER=1 JOINER=1 DHCP6_CLIENT=1 DHCP6_SERVER=1
```

For a list of all available commands, visit [OpenThread CLI Reference README.md][CLI].

[CLI]: https://github.com/openthread/openthread/blob/master/src/cli/README.md

## Verification

The following toolchain has been used for testing and verification:
   - gcc version 7.3.1

The EFR32 example has been verified with following Flex SDK/RAIL Library version:
   - Flex SDK version 2.7.0.0
