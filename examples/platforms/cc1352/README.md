# OpenThread on CC1352 Example

This directory contains example platform drivers for the [Texas Instruments
CC1352R1][cc1352r1].

The example platform drivers are intended to present the minimal code necessary
to support OpenThread. As a result, the example platform drivers do not
necessarily highlight the platform's full capabilities. Consult the [SimpleLink
CC26X2R1 SDK][cc26x2r1-sdk] for more development option. The platform drivers
were built for the [CC1352R1 LAUNCHXL][cc1352r1-launchxl], usage on other
boards with a cc1352r1 may require changes to the peripheral drivers.

[cc1352r1-launchxl]: http://www.ti.com/tool/launchxl-cc26x2r1
[cc26x2r1-sdk]: http://www.ti.com/tool/simplelink-cc26x2-sdk
<!---
TODO: Update link when cc1352 product page is live
[cc1352r1]: http://www.ti.com/product/cc1352r1
-->
[cc1352r1]: http://www.ti.com/tool/launchxl-cc26x2r1

## Toolchain

Building the examples for the cc1352 requires [GNU AutoConf][gnu-autoconf],
[GNU AutoMake][gnu-automake], [Python][python], and the [ARM gcc
toolchain][arm-toolchain].

With the exception of the arm toolchain, most of these tools are installed by
default on modern Posix systems. It is recommended to setup a Linux virtual
machine for building on a Windows host system. For help setting up VirtualBox
with Ubuntu, consult this [community help wiki
article][ubuntu-wiki-virtualbox].

[gnu-autoconf]: https://www.gnu.org/software/autoconf
[gnu-automake]: https://www.gnu.org/software/automake
[python]: https://www.python.org
[arm-toolchain]: https://launchpad.net/gcc-arm-embedded
[cygwin]: https://www.cygwin.com
[mingw]: http://www.mingw.org
[ubuntu-wiki-virtualbox]: https://help.ubuntu.com/community/VirtualBox

In a Bash terminal, follow these instructions to install the GNU toolchain and
other dependencies.

```bash
$ cd <path-to-openthread>
$ ./script/bootstrap
```

## Building

In a Bash terminal, follow these instructions to build the cc1352 examples.

```bash
$ cd <path-to-openthread>
$ ./bootstrap
$ make -f examples/Makefile-cc1352
```

## Flash Binaries

If the build completed successfully, the `elf` files may be found in
`<path-to-openthread>/output/cc1352/bin`.

Flash the images with [Uniflash][uniflash]. Make sure to deselect the binary
check-box, Uniflash assumes a file without an extension is a binary file.

[uniflash]: http://www.ti.com/tool/uniflash

## Interact

### CLI example

1. With a terminal client (PuTTY, minicom, etc.) open the com port associated
   with the cc1352 UART. The serial port settings are:
    * 115200 baud
    * 8 data bits
    * no parity bit
    * 1 stop bit
2. Type `help` for a list of commands.
3. Follow the instructions in the [CLI README][cli-readme] for instructions on
   setting up a network.

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

### NCP example

Refer to the documentation in the [wpantund][wpantund] project for build
instructions and usage information.

[wpantund]: https://github.com/openthread/wpantund
