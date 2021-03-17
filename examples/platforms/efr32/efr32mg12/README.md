# OpenThread on EFR32MG12 Example

This directory contains example platform drivers for the [Silicon Labs EFR32MG12][efr32mg12] based on [EFR32™ Mighty Gecko Wireless Starter Kit][slwstk6000b] or [Thunderboard™ Sense 2 Sensor-to-Cloud Advanced IoT Development Kit][sltb004a].

[efr32mg]: http://www.silabs.com/products/wireless/mesh-networking/efr32mg-mighty-gecko-zigbee-thread-soc
[slwstk6000b]: http://www.silabs.com/products/development-tools/wireless/mesh-networking/mighty-gecko-starter-kit
[sltb004a]: https://www.silabs.com/products/development-tools/thunderboard/thunderboard-sense-two-kit

The example platform drivers are intended to present the minimal code necessary to support OpenThread. [EFR32MG12P SoC][efr32mg12p] has rich memory and peripheral resources which can support all OpenThread capabilities. See the "Run the example with EFR32MG12 boards" section below for an example using basic OpenThread capabilities.

See [sleepy-demo/README.md](sleepy-demo/README.md) for instructions for an example that uses the low-energy modes of the EFR32MG12 when running as a Sleepy End Device.

[efr32mg12p]: http://www.silabs.com/products/wireless/mesh-networking/efr32mg-mighty-gecko-zigbee-thread-soc/device.EFR32MG12P432F1024GL125

## Toolchain

Download and install the [GNU toolchain for ARM Cortex-M][gnu-toolchain].

[gnu-toolchain]: https://developer.arm.com/open-source/gnu-toolchain/gnu-rm

In a Bash terminal, follow these instructions to install the GNU toolchain and other dependencies.

```bash
$ cd <path-to-openthread>
$ ./script/bootstrap
```

## Build Examples

1. Download and install the [Simplicity Studio][simplicity_studio].

[simplicity_studio]: http://www.silabs.com/products/development-tools/software/simplicity-studio

2. Install Flex (Gecko) SDK including RAIL Library from Simplicity Studio.
   - Connect EFR32MG12P Wireless Starter Kit to Simplicity Studio.
   - Find Flex SDK v3.1 in the Software Update page and click Install.
   - Flex SDK v3.1 will be installed in the path:
     - Mac
       - `/Applications/Simplicity\ Studio.app/Contents/Eclipse/developer/sdks/gecko_sdk_suite`
     - Windows
       - `C:\SiliconLabs\SimplicityStudio\v5\developer\sdks\gecko_sdk_suite`
     - Linux
       - `./SimplicityStudio_v5/developer/sdks/gecko_sdk_suite`

For more information on configuring, building, and installing applications for the Wireless Gecko (EFR32) portfolio using FLEX, see [Getting Started with the Silicon Labs Flex Software Development Kit for the Wireless Gecko (EFR32™) Portfolio][qsg138]. For more information on RAIL, see [Radio Abstraction Interface Layer][rail].

[qsg138]: https://www.silabs.com/documents/public/quick-start-guides/qsg138-flex-efr32.pdf
[rail]: http://www.silabs.com/products/development-tools/software/radio-abstraction-interface-layer-sdk

3. Configure the path to Flex SDK source code.

```bash
$ cd <path-to-openthread>/third_party
$ mkdir silabs
$ cd <path-to-Simplicity-Studio>/developer/sdks
$ cp -rf gecko_sdk_suite <path-to-openthread>/third_party/silabs/
```

Alternatively create a symbolic link to the Flex SDK source code.

```bash
$ cd <path-to-openthread>/third_party
$ mkdir silabs
$ ln -s <path-to-Simplicity-Studio>/developer/sdks/gecko_sdk_suite silabs/gecko_sdk_suite
```

4. Build OpenThread Firmware (CLI example) on EFR32 platform.

```bash
$ cd <path-to-openthread>
$ ./bootstrap
```

For EFR32MG12™ Mighty Gecko Wireless Starter Kit, this can be done using both the CMake and autotools build systems

**CMake (preferred)**

```bash
$ ./script/cmake-build efr32mg12 -DBOARD=brd4161a
...
-- Configuring done
-- Generating done
-- Build files have been written to: <path-to-openthread>/build/efr32mg12
+ [[ -n ot-rcp ot-cli-ftd ot-cli-mtd ot-ncp-ftd ot-ncp-mtd sleepy-demo-ftd sleepy-demo-mtd ]]
+ ninja ot-rcp ot-cli-ftd ot-cli-mtd ot-ncp-ftd ot-ncp-mtd sleepy-demo-ftd sleepy-demo-mtd
[572/572] Linking CXX executable examples/platforms/efr32/sleepy-demo/sleepy-demo-ftd/sleepy-demo-ftd
+ cd <path-to-openthread>
```

After a successful build, the `elf` files are found in `<path-to-openthread>/build/efr32mg12/examples`.

```bash
# For linux
$ find build/efr32mg12/examples -type f -executable
build/efr32mg12/examples/apps/cli/ot-cli-mtd
build/efr32mg12/examples/apps/cli/ot-cli-ftd
build/efr32mg12/examples/apps/ncp/ot-ncp-ftd
build/efr32mg12/examples/apps/ncp/ot-ncp-mtd
build/efr32mg12/examples/apps/ncp/ot-rcp
build/efr32mg12/examples/platforms/efr32/sleepy-demo/sleepy-demo-ftd/sleepy-demo-ftd
build/efr32mg12/examples/platforms/efr32/sleepy-demo/sleepy-demo-mtd/sleepy-demo-mtd

# For BSD/Darwin/mac systems
$ find build/efr32mg12/examples -type f -perm +111
build/efr32mg12/examples/apps/cli/ot-cli-mtd
build/efr32mg12/examples/apps/cli/ot-cli-ftd
build/efr32mg12/examples/apps/ncp/ot-ncp-ftd
build/efr32mg12/examples/apps/ncp/ot-ncp-mtd
build/efr32mg12/examples/apps/ncp/ot-rcp
build/efr32mg12/examples/platforms/efr32/sleepy-demo/sleepy-demo-ftd/sleepy-demo-ftd
build/efr32mg12/examples/platforms/efr32/sleepy-demo/sleepy-demo-mtd/sleepy-demo-mtd
```

**autotools (soon to be depracated)**

```bash
$ make -f examples/Makefile-efr32mg12 BOARD=BRD4161A
```

or alternatively for the Thunderboard™ Sense 2:

```bash
$ make -f examples/Makefile-efr32mg12 BOARD=BRD4166A
```

After a successful build, the `elf` files are found in `<path-to-openthread>/output/efr32mg12/bin`.

## Flash Binaries

Compiled binaries may be flashed onto the EFR32 using [JLinkGDBServer][jlinkgdbserver]. EFR32 Starter kit mainboard integrates an on-board SEGGER J-Link debugger.

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

Or Compiled binaries also may be flashed onto the specified EFR32 dev board using [J-Link Commander][j-link-commander].

[j-link-commander]: https://www.segger.com/products/debug-probes/j-link/tools/j-link-commander/

```bash
$ cd <path-to-openthread>/output/efr32mg12/bin
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

Alternatively Simplicity Commander provides a graphical interface for J-Link Commander.

```bash
$ cd <path-to-openthread>/output/efr32mg12/bin
$ arm-none-eabi-objcopy -O ihex ot-cli-ftd ot-cli-ftd.ihex
$ <path-to-simplicity-studio>/developer/adapter_packs/commander/commander
```

In the J-Link Device drop-down list select the serial number of the device to flash. Click the Adapter Connect button. Ensure the Debug Interface drop-down list is set to SWD and click the Target Connect button. Click on the Flash icon on the left side of the window to switch to the flash page. In the Flash MCU pane enter the path of the ot-cli-ftd.s37 file or choose the file with the Browse... button. Click the Flash button located under the Browse... button.

## Run the example with EFR32MG12 boards

1. Flash two EFR32 boards with the `CLI example` firmware (as shown above).
2. Open terminal to first device `/dev/ttyACM0` (serial port settings: 115200 8-N-1). Type `help` for a list of commands.

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

3. Start a Thread network as Leader.

   ```bash
   > dataset init new
   Done
   > dataset
   Active Timestamp: 1
   Channel: 13
   Channel Mask: 0x07fff800
   Ext PAN ID: d63e8e3e495ebbc3
   Mesh Local Prefix: fd3d:b50b:f96d:722d::/64
   Master Key: dfd34f0f05cad978ec4e32b0413038ff
   Network Name: OpenThread-8f28
   PAN ID: 0x8f28
   PSKc: c23a76e98f1a6483639b1ac1271e2e27
   Security Policy: 0, onrcb
   Done
   > dataset commit active
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

4. Open terminal to second device `/dev/ttyACM1` (serial port settings: 115200 8-N-1) and attach it to the Thread network as a Router.

   ```bash
   > dataset masterkey dfd34f0f05cad978ec4e32b0413038ff
   Done
   > dataset commit active
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
   fd3d:b50b:f96d:722d:0:ff:fe00:fc00
   fd3d:b50b:f96d:722d:0:ff:fe00:c00
   fd3d:b50b:f96d:722d:7a73:bff6:9093:9117
   fe80:0:0:0:6c41:9001:f3d6:4148
   Done
   ```

6. Send an ICMPv6 ping to Leader's Mesh-EID IPv6 address.

   ```bash
   > ping fd3d:b50b:f96d:722d:7a73:bff6:9093:9117
   16 bytes from fd3d:b50b:f96d:722d:558:f56b:d688:799: icmp_seq=1 hlim=64 time=24ms
   ```

The above example demonstrates basic OpenThread capabilities. Enable more features/roles (e.g. commissioner, joiner, DHCPv6 Server/Client, etc.) by assigning compile-options before compiling.

```bash
$ cd <path-to-openthread>
$ ./bootstrap
$ make -f examples/Makefile-efr32mg12 COMMISSIONER=1 JOINER=1 DHCP6_CLIENT=1 DHCP6_SERVER=1
```

For a list of all available commands, visit [OpenThread CLI Reference README.md][cli].

[cli]: https://github.com/openthread/openthread/blob/main/src/cli/README.md

## Verification

The following toolchain has been used for testing and verification:

- gcc version 7.3.1

The EFR32 example has been verified with following Flex SDK/RAIL Library version:

- Flex SDK v3.1.x
