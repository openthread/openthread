# OpenThread on CC2650 Example

This directory contains example platform drivers for the [Texas
Instruments CC2650][cc2650].

The example platform drivers are intended to present the minimal code necessary
to support OpenThread. As a result, the example platform drivers do not
necessarily highlight the platform's full capabilities. The platform
abstraction layer was build for the [CC2650 LAUNCHXL][cc2650-launchxl], usage
on other boards with a CC2650 will require changes to the peripheral drivers.

Due to flash size limitations, some features of OpenThread are not supported on
the [Texas Instruments CC2650][cc2650]. This platform is intended for
exprimentation and exploration of OpenThread, not a production ready
environment. Texas Instruments recommends future TI SoCs for production.

Building with gcc 5.4 is recommended due to generated code size concerns.

All three configurations were tested with `arm-none-eabi-gcc 5.4.1 20160609
(release)` on [this commit][tested-commit]. The automatic integration builds have since
been limited to only the `cli-mtd` configuration to limit the impact on pull
requests.

[cc2650]: http://www.ti.com/product/CC2650
[cc2650-launchxl]: http://www.ti.com/tool/Launchxl-cc2650
[tested-commit]: https://github.com/openthread/openthread/commit/e8611291d65e8ad28d77a7645695c5352504c3dd

## Build Environment

Building the examples for the cc2650 requires [GNU AutoConf][gnu-autoconf],
[GNU AutoMake][gnu-automake], [Python][python], and the
[ARM gcc toolchain][arm-toolchain].

With the exception of the arm toolchain, most of these tools are installed by
default on modern Posix systems. Windows does not have these tools installed by
default, and the bootstrap script requires a Posix or MSYS environment to run.
It is possible to setup an MSYS environment inside of Windows using tools such
as [Cygwin][cygwin] or [MinGW][mingw] but it is recommended to setup a Linux VM
for building on a Windows system. For help setting up VirtualBox with Ubuntu,
consult this [community help wiki article][ubuntu-wiki-virtualbox].

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

In a Bash terminal, follow these instructions to build the cc2650 examples.

```bash
$ cd <path-to-openthread>
$ ./bootstrap
$ make -f examples/Makefile-cc2650
```

## Flash Binaries

If the build completed successfully, the `elf` files may be found in
`<path-to-openthread>/output/cc2650/bin`.

To flash the images with [Flash Programmer 2][ti-flash-programmer-2], the files
must have the `*.elf` extension.
```bash
$ cd <path-to-openthread>/output/cc2650/bin
$ cp ot-cli ot-cli.elf
```

To load the images with the [serial bootloader][ti-cc2650-bootloader], the
images must be converted to `bin`. This is done using `arm-none-eabi-objcopy`
```bash
$ cd <path-to-openthread>/output/cc2650/bin
$ arm-none-eabi-objcopy -O binary ot-cli ot-cli.bin
```
The [cc2538-bsl.py script][cc2538-bsl-tool] provides a convenient method
for flashing a CC2650 via the UART. To enter the bootloader backdoor for flashing,
hold down BTN-1 on CC2650 LauchPad or SELECT for CC2650DK (corresponds to logic '0')
while you press the Reset button.

[ti-flash-programmer-2]: http://www.ti.com/tool/flash-programmer
[ti-cc2650-bootloader]: http://www.ti.com/lit/an/swra466a/swra466a.pdf
[cc2538-bsl-tool]: https://github.com/JelmerT/cc2538-bsl

## Interact

### CLI example

1. With a terminal client (putty, minicom, etc.) open the com port associated
   with the cc2650 UART. The serial port settings are:
    * 115200 baud
    * 8 data bits
    * no parity bit
    * 1 stop bit
2. Type `help` for a list of commands
3. follow the instructions in the [CLI README][cli-readme] for instructions on
   setting up a network

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
