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

## Build

```sh
# build core for POSIX
make -f src/posix/Makefile-posix
# build transceiver, where xxxx is the platform name, such as cc2538, nrf52840 and so on
make -f examples/Makefile-xxxx
```

## Test

**NOTE** Assuming the build system is 64bit Linux, you can use the normal OpenThread CLI as described in the [command line document](../../src/cli/README.md). You can also perform radio diagnostics using the command [diag](../../src/core/diags/README.md).

### With Simulation

```sh
make -f examples/Makefile-simulation
./output/posix/x86_64-unknown-linux-gnu/bin/ot-cli 'spinel+hdlc+forkpty://output/x86_64-unknown-linux-gnu/bin/ot-rcp?forkpty-arg=1'
```

### With Real Device

#### nRF52840

- USB=0

```sh
make -f examples/Makefile-nrf52840
arm-none-eabi-objcopy -O ihex output/nrf52840/bin/ot-rcp ot-rcp.hex
nrfjprog -f nrf52 --chiperase --reset --program ot-rcp.hex
nrfjprog -f nrf52 --pinresetenable
nrfjprog -f nrf52 --reset

# Disable MSD
expect <<EOF
spawn JLinkExe
expect "J-Link>"
send "msddisable\n"
expect "Probe configured successfully."
exit
EOF

./output/posix/x86_64-unknown-linux-gnu/bin/ot-cli 'spinel+hdlc+uart:///dev/ttyACM0?uart-baudrate=115200'
```

- USB=1

```sh
# without USB=1 may result in failure for serial port issue
make -f examples/Makefile-nrf52840 USB=1
arm-none-eabi-objcopy -O ihex output/nrf52840/bin/ot-rcp ot-rcp.hex
nrfjprog -f nrf52 --chiperase --reset --program ot-rcp.hex
# plug the CDC serial USB port
./output/posix/x86_64-unknown-linux-gnu/bin/ot-cli 'spinel+hdlc+uart:///dev/ttyACM0?uart-baudrate=115200'
```

#### CC2538

```sh
make -f examples/Makefile-cc2538
arm-none-eabi-objcopy -O binary output/cc2538/bin/ot-rcp ot-rcp.bin
# see https://github.com/JelmerT/cc2538-bsl
python cc2538-bsl/cc2538-bsl.py -b 460800 -e -w -v -p /dev/ttyUSB0 ot-rcp.bin
./output/posix/x86_64-unknown-linux-gnu/bin/ot-cli 'spinel+hdlc+uart:///dev/ttyUSB0?uart-baudrate=115200'
```

## Wpantund Support

**NOTE** Assuming the build system is 64bit Linux and _wpantund_ is already installed and **stopped**.

### With Simulation

```sh
sudo wpantund -s 'system:./output/posix/x86_64-unknown-linux-gnu/bin/ot-ncp spinel+hdlc+forkpty://output/x86_64-unknown-linux-gnu/bin/ot-rcp?forkpty-arg=1'
```

### With Real Device

```sh
# nRF52840
sudo wpantund -s 'system:./output/posix/x86_64-unknown-linux-gnu/bin/ot-ncp spinel+hdlc+uart:///dev/ttyACM0?uart-baudrate=115200'
# CC2538
sudo wpantund -s 'system:./output/posix/x86_64-unknown-linux-gnu/bin/ot-ncp spinel+hdlc+uart:///dev/ttyUSB0?uart-baudrate=115200'
```

## Daemon Mode Support

OpenThread Posix Daemon mode uses a unix socket as input and output, so that OpenThread core can run as a service. And a client can communicate with it by connecting to the socket. The protocol is OpenThread CLI.

```
# build daemon mode core stack for POSIX
make -f src/posix/Makefile-posix DAEMON=1
# Daemon with simulation
./output/posix/x86_64-unknown-linux-gnu/bin/ot-daemon 'spinel+hdlc+forkpty://output/x86_64-unknown-linux-gnu/bin/ot-rcp?forkpty-arg=1'
# Daemon with real device
./output/posix/x86_64-unknown-linux-gnu/bin/ot-daemon 'spinel+hdlc+uart:///dev/ttyACM0?uart-baudrate=115200'
# Built-in controller
./output/posix/x86_64-unknown-linux-gnu/bin/ot-ctl
```
