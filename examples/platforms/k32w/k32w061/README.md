# OpenThread on NXP K32W061 Example

This directory contains example platform drivers for the [NXP K32W061][k32w061] based on [K32W061-DK006][k32w061-dk006] hardware platform.

The example platform drivers are intended to present the minimal code necessary to support OpenThread. As a result, the example platform drivers do not necessarily highlight the platform's full capabilities.

## Toolchain

OpenThread environment is suited to be run on a Linux-based OS. Recommended OS is Ubuntu 18.04.2 LTS. Download and install the [MCUXpresso IDE][mcuxpresso ide].

[mcuxpresso ide]: https://www.nxp.com/support/developer-resources/software-development-tools/mcuxpresso-software-and-tools/mcuxpresso-integrated-development-environment-ide:MCUXpresso-IDE

In a Bash terminal (found, for example, in Ubuntu OS), follow these instructions to install the GNU toolchain and other dependencies.

```bash
$ cd <path-to-openthread>
$ ./script/bootstrap
```

If a network connection timeout is encountered, re-run the script.

Python-pip is also required for the build. User can install it by running "sudo apt-get install python-pip" in bash. After installing Python-pip, execute "pip install pycryptodome" in bash. This is needed for signing the built binary in order to load it on the board. Also, pycrypto "pip install pycrypto" is required for PKCS1.

Windows 10 offers the possibility of running bash by installing "Ubuntu on Windows" from Microsoft Store. This application allows the user to use Ubuntu Terminal and run Ubuntu command line utilities including bash, ssh, git, apt and many more. If this option is used, it is recommended to add instructions for the path mapping in MCUXpresso IDE. This can be done after adding the project to the workspace by going to Run->"Debug Configuration"->"C/C++(NXP Semiconductors) MCU Application"->Source->Add. Then the user should create a path mapping such that MCUXpresso IDE will find the mount point for the "Ubuntu in Windows" subsystem. For example, user can enter compilation path recognized by Ubuntu as /mnt/c/<path-to-openthread>, while equivalent "Local file system path" is C:/<path-to-openthread>. This example assumes that the openthread package is installed on the C drive.

## Build Examples

```bash
$ cd <path-to-openthread>
$ ./bootstrap
$ make -f examples/Makefile-k32w061
```

After a successful build, the `elf` files are found in `<path-to-openthread>/output/k32w061/bin`.

## Flash Binaries

Connect to the board by plugging a mini-USB cable to the connector marked with TARGET on the DK6 board. This connector is situated on the same side with the power connector.

OpenThread example application compiled binaries can be found in `<path-to-openthread>/output/k32w061/bin` and include FTD (Full Thread Device) and MTD (Minimal Thread Device) variants of CLI and NCP applications. The compiled binaries can be flashed onto the K32W061 using MCUXpresso IDE. This requires the following steps:

1. Import the K32W061 SDK into MCUXpresso IDE. This can be done by dragging and dropping the SDK archive into MCUXpresso IDE's Installed SDKs tab. The archive for SDK_2.6.0_K32W061DK6 is available for download at https://mcuxpresso.nxp.com/en/welcome
2. In MCUXpresso IDE, go to File->Import->C/C++->"Existing Code as Makefile Project" and click Next.
3. Select the OpenThread folder as the "Existing Code Location". In the "Toolchain for Indexer Settings" list, be sure to keep the setting to <none>. Click Finish.
4. Right click on the newly created openthread project in the Workspace and go to Properties->"C/C++ Build"->"MCU Settings". Select the JN518x from the SDK MCUs list.
5. Go to C/C++ Build->"Tool Chain Editor" and untick the "Display compatible toolchains only" checkbox. In the drop-down menu named "Current toolchain", select "NXP MCU Tools". Click "Apply and Close".
6. Right click on the openthread project and select "Debug As"->"MCUXpresso IDE LinkServer (inc. CMSIS-DAP) probes"
7. A window to select the binary will appear. Select "output/k32w061/bin/ot-<application>" and click Ok.
8. Under the menu bar, towards the center of the screen, there is a green bug icon with a drop-down arrow next to it. Click on the arrow and select "Debug Configurations".
9. In the right side of the Debug Configurations window, go to "C/C++ (NXP Semiconductors) MCU Application"->"openthread LinkServer Default".
10. Make sure that in the "C/C++ Application:" text box contains "output\k32w061\bin\ot-<application>" path.
11. Go to "GUI Flash Tool" tab. In "Target Operations"->Program->Options, select "bin" as the "Format to use for programming". Make sure the "Base address" is 0x0.
12. Click Debug.
13. A pop-up window entitled "Errors in Workspace" will appear. Click Proceed.
14. The board is now flashed.

[cmsis-dap]: https://os.mbed.com/handbook/CMSIS-DAP

## Running the example

1. Prepare two boards with the flashed `CLI Example` (as shown above). Make sure that the JN4 jumper is set to RX and the JN7 jumper is set to TX, connecting the LPC and JN UART0 pins.
2. The CLI example uses UART connection. To view raw UART output, start a terminal emulator like PuTTY and connect to the used COM port with the following UART settings:

   - Baud rate: 115200
   - 8 data bits
   - 1 stop bit
   - No parity
   - No flow control

3. Open a terminal connection on the first board and start a new Thread network.

```bash
> panid 0xabcd
Done
> ifconfig up
Done
> thread start
Done
```

4. After a couple of seconds the node will become a Leader of the network.

```bash
> state
Leader
```

5. Open a terminal connection on the second board and attach a node to the network.

```bash
> panid 0xabcd
Done
> ifconfig up
Done
> thread start
Done
```

6. After a couple of seconds the second node will attach and become a Child.

```bash
> state
Child
```

7. List all IPv6 addresses of the first board.

```bash
> ipaddr
fdde:ad00:beef:0:0:ff:fe00:fc00
fdde:ad00:beef:0:0:ff:fe00:9c00
fdde:ad00:beef:0:4bcb:73a5:7c28:318e
fe80:0:0:0:5c91:c61:b67c:271c
```

8. Choose one of them and send an ICMPv6 ping from the second board.

```bash
> ping fdde:ad00:beef:0:0:ff:fe00:fc00
16 bytes from fdde:ad00:beef:0:0:ff:fe00:fc00: icmp_seq=1 hlim=64 time=8ms
```

For a list of all available commands, visit [OpenThread CLI Reference README.md][cli].

[cli]: https://github.com/openthread/openthread/blob/master/src/cli/README.md
