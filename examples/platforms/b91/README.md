# OpenThread on B91 Example

This directory contains example platform drivers for the Telink B91.

The example platform drivers are intended to present the minimal code necessary to support OpenThread. As a result, the example platform drivers do not necessarily highlight the platform's full capabilities.

This platform is intended for exprimentation and exploration of OpenThread, not a production ready environment.

## Build Environment

Building the examples for the b91 requires [GNU AutoConf][gnu-autoconf], [GNU AutoMake][gnu-automake],[GNU Libtool][gnu-libtool], [Python][python], and the [Telink risc-v toolchain][telink-toolchain].

After the Telink risc-v toolchain is installed, the Cygwin environment is installed too, considering the lack of building tools used by openthread, you should install these tools(AutoConf, AutoMake, Libtool) manually.

[gnu-autoconf]: https://www.gnu.org/software/autoconf
[gnu-automake]: https://www.gnu.org/software/automake
[gnu-libtool]: https://www.gnu.org/software/libtool
[python]: https://www.python.org
[telink-toolchain]: http://wiki.telink-semi.cn/tools_and_sdk/Tools/IDE/telink_v323_rds_official_windows.zip

## Building

In a Bash terminal, follow these instructions to build the b91 examples.

```bash
$ cd <path-to-openthread>
$ ./bootstrap
$ make -f examples/Makefile-b91
```

## Flash Binaries

If the build completed successfully, the `elf` files may be found in `<path-to-openthread>/output/b91/bin`.

You can download the [BDT tool][bdt-tool] and refer to the [user guide][user-guide] provided by Telink to burn the images to the board, in order to load the images with the [BDT tool][bdt-tool], the images must be converted to `bin`. This is done using `riscv32-elf-objcopy`

```bash
$ cd <path-to-openthread>/output/b91/bin
$ riscv32-elf-objcopy -S -O binary ot-cli-ftd ot-cli-ftd.bin
```

[bdt-tool]: http://wiki.telink-semi.cn/tools_and_sdk/Tools/BDT/BDT.zip
[user-guide]: http://wiki.telink-semi.cn/tools_and_sdk/Tools/BDT/BDT%20%20User%20Guide.zip

## Interact

### CLI example

1. With a terminal client (putty, minicom, etc.) open the com port associated with the b91 UART. The serial port settings are:
   - 115200 baud
   - 8 data bits
   - no parity bit
   - 1 stop bit
2. Type `help` for a list of commands
3. follow the instructions in the [CLI README][cli-readme] for instructions on setting up a network

[cli-readme]: ../../../src/cli/README.md

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
