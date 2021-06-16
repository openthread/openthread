# OpenThread POSIX app

OpenThread supports running its core on POSIX and transmits radio frames through a radio transceiver.

Currently most platforms in [examples/platforms](../../examples/platforms) support transceiver mode.

The figure below shows the architecture of OpenThread running in transceiver mode.

```
+-------------------------+
|     MLE TMF UDP IP      |
|  MeshForwarder 6LoWPAN  |
| _ _ _ _ _ _ _ _ _ _ _ _ |      spinel         +------------------+
|    OpenThread Core      | <---------------->  | OpenThread Radio |
+-------------------------+     UART|SPI        +------------------+
         POSIX                                          Chip
```

## Build POSIX CLI

```sh
./script/cmake-build posix
```

If built successfully, the binary should be found at: `build/posix/src/posix/ot-cli`.

## Transceivers on different platforms

### Simulation

OpenThread provides an implemenation on the simulation platform which enables running a simulated transceiver on the host.

#### Build

```sh
# Only build RCP so that it goes faster
./script/cmake-build simulation -DOT_APP_CLI=OFF -DOT_APP_NCP=OFF -DOT_FTD=OFF -DOT_MTD=OFF
```

#### Run

**NOTE** Assuming the build system is 64bit Linux, you can use the normal OpenThread CLI as described in the [command line document](../../src/cli/README.md). You can also perform radio diagnostics using the command [diag](../../src/core/diags/README.md).

```sh
./build/posix/src/posix/ot-cli 'spinel+hdlc+forkpty://build/simulation/examples/apps/ncp/ot-rcp?forkpty-arg=1'
```

### Nordic Semiconductor nRF52840

The nRF example platform driver can be found in the [ot-nrf528xx](https://github.com/openthread/ot-nrf528xx) repo.

#### Build

To build and program the device with RCP application, complete the following steps:

1. Clone the OpenThread nRF528xx platform repository into the current directory:

   ```sh
   git clone --recursive https://github.com/openthread/ot-nrf528xx.git
   ```

2. Enter the `ot-nrf528xx` directory:

   ```sh
   cd ot-nrf528xx
   ```

3. Install the OpenThread dependencies:

   ```sh
   ./script/bootstrap
   ```

4. Build the RCP example for the hardware platform and the transport of your choice:

   a. nRF52840 Dongle (USB transport)

   ```sh
   rm -rf build
   script/build nrf52840 USB_trans -DOT_BOOTLOADER=USB -DOT_THREAD_VERSION=1.2
   ```

   b. For nRF52840 Development Kit

   ```sh
   rm -rf build
   script/build nrf52840 UART_trans -DOT_THREAD_VERSION=1.2
   ```

   This creates an RCP image at `build/bin/ot-rcp`.

5. Depending on the hardware platform, complete the following steps to program the device:

   a. nRF52840 Dongle (USB transport)

   ```sh
   # Install nRF Util:
   python3 -m pip install -U nrfutil

   # Generate the RCP firmware package:
   nrfutil pkg generate --hw-version 52 --sd-req=0x00 \
       --application build/bin/ot-rcp.hex --application-version 1 build/bin/ot-rcp.zip
   ```

   Connect the nRF52840 Dongle to the USB port and press the **RESET** button on the dongle to put it into the DFU mode. The LED on the dongle starts blinking red.

   ```sh
   # Install the RCP firmware package onto the dongle
   nrfutil dfu usb-serial -pkg build/bin/ot-rcp.zip -p /dev/ttyACM0
   ```

   b. nRF52840 Development Kit

   ```sh
   # Program the image using the nrfjprog utility.
   nrfjprog -f nrf52 --chiperase --program build/bin/ot-rcp.hex --reset
   ```

   Disable the Mass Storage feature on the device, so that it does not interfere with the core RCP functionalities:

   ```sh
   JLinkExe -device NRF52840_XXAA -if SWD -speed 4000 -autoconnect 1
   J-Link>MSDDisable
   Probe configured successfully.
   J-Link>exit
   ```

   Power-cycle the device to apply the changes.

#### Run

```sh
./build/posix/src/posix/ot-cli 'spinel+hdlc+uart:///dev/ttyACM0?uart-baudrate=115200'
```

### CC2538

#### Build

```
./script/cmake-build cc2538 -DOT_APP_CLI=OFF -DOT_APP_NCP=OFF -DOT_FTD=OFF -DOT_MTD=OFF
```

#### Flash

```sh
arm-none-eabi-objcopy -O ihex build/cc2538/examples/apps/ncp/ot-rcp ot-rcp.bin
# see https://github.com/JelmerT/cc2538-bsl
python cc2538-bsl/cc2538-bsl.py -b 460800 -e -w -v -p /dev/ttyUSB0 ot-rcp.bin
```

#### Run

```sh
./build/posix/src/posix/ot-cli 'spinel+hdlc+uart:///dev/ttyUSB0?uart-baudrate=115200'
```

## Daemon Mode

OpenThread Posix Daemon mode uses a unix socket as input and output, so that OpenThread core can run as a service. And a client can communicate with it by connecting to the socket. The protocol is OpenThread CLI.

```
# build daemon mode core stack for POSIX
./script/cmake-build posix -DOT_DAEMON=ON
# Daemon with simulation
./build/posix/src/posix/ot-daemon 'spinel+hdlc+forkpty://build/simulation/examples/apps/ncp/ot-rcp?forkpty-arg=1'
# Daemon with real device
./build/posix/src/posix/ot-daemon 'spinel+hdlc+uart:///dev/ttyACM0?uart-baudrate=115200'
# Built-in controller
./build/posix/src/posix/ot-ctl
```
