# OpenThread on Windows #

OpenThread includes Windows 10 drivers necessary for interfacing with UART NCP devices. The design allows for support of both simple radio devices and devices running the complete OpenThread stack.

> **NOTE:** OpenThread's Windows integration has not been actively maintained for some time. While Windows builds are still a part of OpenThread's continuous integration (CI), the Windows CI only tests basic compilation.

## Architecture ##

Details on the architecture can be found [here](https://openthread.io/platforms/windows10).

## Building OpenThread using Windows

To build locally using the Windows platform, use Visual Studio 2015. To build for the POSIX and TI CC2538 platforms, use Bash on Ubuntu on Windows.

All the following steps assume you have already cloned and checked out a branch.

### Visual Studio 2015

To build OpenThread on Windows you need to install the following:

*  Any Visual Studio 2015 edition. The [Community](https://www.microsoft.com/en-us/download/details.aspx?id=48146) edition is free. Generally most of the optional features will be required.
*  The [Windows 10 Driver Kit](https://go.microsoft.com/fwlink/p/?LinkId=526733).

Once all these are installed, open the Solution file `etc/visual-studio/openthread.sln`. Select the Configuration and Platform (i.e. Release/x64) and then Build All (F6).

### Bash on Ubuntu on Windows

[Bash on Ubuntu on Windows](https://msdn.microsoft.com/en-us/commandline/wsl/about) is a new feature to Windows 10. To set it up, follow the steps [here](https://msdn.microsoft.com/commandline/wsl/install_guide). Other FAQs can be found [here](https://msdn.microsoft.com/en-us/commandline/wsl/faq).

Once installed, open the "Bash on Ubuntu on Windows" app. Do the following to install everything necessary to build OpenThread.

Make sure line ending are UNIX style:

```
git config core.autocrlf false
git rm --cached -r . && git reset --hard
```

Install the compilers:

```
apt install gcc
apt install g++
```

Install Python (with pexpect):

```
apt install python
apt install python-pip
pip install pexpect
```

To build the TI CC2538 platform, you need to install the correct toolchain:

```
add-apt-repository ppa:team-gcc-arm-embedded/ppa
apt-get update
apt-get install gcc-arm-embedded
```

Set up the build environment:

```
./bootstrap
```

### Platforms

Before building, manually configure features as desired. For example:

```
./configure --enable-cli --enable-diag --enable-commissioner --enable-joiner --with-examples=posix
make
```

Or clean and build with desired features enabled:

```
make -f examples/Makefile-<platform-name> clean
COMMISSIONER=1 JOINER=1 make -f examples/Makefile-<platform-name>
```

For detailed instructions on building and creating binaries for Windows-supported platforms, see each platform's respective README:

* [POSIX](https://github.com/openthread/openthread/tree/master/examples/platforms/posix)
* [TI CC2538](https://github.com/openthread/openthread/tree/master/examples/platforms/cc2538)
* [Nordic nRF52840](https://github.com/openthread/openthread/tree/master/examples/platforms/nrf52840)

## Installing OpenThread on Windows

Microsoft has made available a test VHD specifically modified for virtually testing OpenThread. To get access, please email `nibanks` via his `@microsoft.com` email address.

Once the Windows VHD is loaded and you have gone through OOBE, see `C:\OpenThread\readme.txt` for additional instructions on how to set everything up.

> **Note:** Since none of the binaries mentioned below are production signed; you need to enable test signing on the machine: `bcdedit /set testsigning on`.  All these binaries require Windows 10 at a minimum.

Since most of these drivers are still under development, it is recommended to also have a kernel debugger configured for the test machine.

### otlwf.sys & otapi.dll

The filter driver `otlwf.sys` exposes the IOCTL interface that `otapi.dll` uses and houses the bulk of the actual Thread logic. `otapi.dll` exposes the C interface for user mode applications to use to control the Thread interfaces.

1. Download the latest platform binary on AppVeyor:
	1. Go to the [latest build](https://ci.appveyor.com/project/jwhui/openthread) page
	1. Select the desired platform (x64, x86, arm)
	1. On the platform page, select **ARTIFACTS**
	1. Download the release.zip file for that platform
1. Extract the files to a temporary location
1. Open an admin command prompt in the temporary location
1. Run the `install_otlwf.cmd` file
1. On **non-IoT platforms**, install the [Visual C++ Redistributable for Visual Studio 2015](https://www.microsoft.com/en-us/download/details.aspx?id=48145)
1. **Note:** A reboot is required, but can be done after all installation is complete.

### ottmp.sys (Miniport for Serial Devices)

The miniport driver `ottmp.sys` binds to serial devices on the machine and exposes them to `otlwf.sys`. It is only needed if you are connecting to an actual device (such as the TI CC2538 or Nordic Semiconductor nRF52840 device).

1. Open an admin command prompt (same location as above).
1. Run the install_ottmp.cmd file.
1. **Note:** A reboot is required, but can be done after all installation is complete.
1. Once the test machine boots and the serial device is connected, manually restart the miniport so that it can find the serial device. **This will have to be done for every reboot**:
```
devcon.exe restart *ottmp*
```

### Command Line Tool Usage

The command line tool `otCli.exe` exposes many of the APIs in `otApi.dll` via the command line. It is included with the same release binaries downloaded above. See the [CLI example README](https://github.com/openthread/openthread/blob/master/examples/apps/cli/README.md) for more details.

The main differences between `otCli.exe` on Windows and the `ot-cli` on Linux, are the following:

*  On Linux, the CLI runs the actual OpenThread stack in-proc. On Windows, it just provides an interface to control the OpenThread stack running either in the installed driver or on the physical device.
*  On Windows, the CLI supports multiple interfaces simultaneously via the `instance` and `instancelist` commands.

The `instance` command allows for getting/setting the current instance that the rest of the commands act on.

```
> instance 0
Done
> instance
[0] {01234567-89AB-CDEF-0123-4567890ABCDE} (Compartment 1)
```

The `instancelist` command queries for all available instances/interfaces on the machine.

```
> instancelist
1 instances found:
[0] {01234567-89AB-CDEF-0123-4567890ABCDE} (Compartment 1)
```

### Sample Universal App

The latest builds of the sample universal app can be downloaded from the [latest AppVeyor build](https://ci.appveyor.com/project/jwhui/openthread).

### Running Python Certification Tests

The Python certification tests approximate the testing done for real certification. On Windows they are run using virtual OpenThread devices. To set up the environment:

1. Install Python 3.6 to `C:\Python36` for all users. Make sure to add the install path to `PATH`.
1. Install the Python crypto libraries: `python.exe C:\Python36\Scripts\pip.exe install pycryptodome==3.4.3`
1. Copy Python cert scripts to the machine (to a 'scripts' folder)

To run all the tests:

```
otTestRunner.exe scripts Cert_* parallel:4 retry:2
```