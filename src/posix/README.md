OpenThread POSIX app
====================

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

Build
-----

```sh
# build core for POSIX
make -f src/posix/Makefile-posix
# build transceiver, where xxxx is the platform name, such as cc2538, nrf52840 and so on
make -f examples/Makefile-xxxx
```

Test
----

**NOTE** Assuming the build system is 64bit Linux, you can use the normal OpenThread CLI as described in the [command line document](../../src/cli/README.md).
You can also perform radio diagnostics using the command [diag](../../src/diag/README.md).

### With Simulation

```sh
make -f examples/Makefile-posix
./output/posix/x86_64-unknown-linux-gnu/bin/ot-cli ./output/x86_64-unknown-linux-gnu/bin/ot-ncp-radio 1
```

### With Real Device

#### nRF52840

* USB=0

```sh
make -f examples/Makefile-nrf52840
arm-none-eabi-objcopy -O ihex output/nrf52840/bin/ot-ncp-radio ot-ncp-radio.hex
nrfjprog -f nrf52 --chiperase --reset --program ot-ncp-radio.hex
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

./output/posix/x86_64-unknown-linux-gnu/bin/ot-cli /dev/ttyACM0 115200
```

* USB=1

```sh
# without USB=1 may result in failure for serial port issue
make -f examples/Makefile-nrf52840 USB=1
arm-none-eabi-objcopy -O ihex output/nrf52840/bin/ot-ncp-radio ot-ncp-radio.hex
nrfjprog -f nrf52 --chiperase --reset --program ot-ncp-radio.hex
# plug the CDC serial USB port
./output/posix/x86_64-unknown-linux-gnu/bin/ot-cli /dev/ttyACM0 115200
```

#### CC2538

```sh
make -f examples/Makefile-cc2538
arm-none-eabi-objcopy -O binary output/cc2538/bin/ot-ncp-radio ot-ncp-radio.bin
# see https://github.com/JelmerT/cc2538-bsl
python cc2538-bsl/cc2538-bsl.py -b 460800 -e -w -v -p /dev/ttyUSB0 ot-ncp-radio.bin
./output/posix/x86_64-unknown-linux-gnu/bin/ot-cli /dev/ttyUSB0 115200
```

Wpantund Support
----------------

**NOTE** Assuming the build system is 64bit Linux and *wpantund* is already installed and **stopped**.

### With Simulation

```sh
sudo wpantund -s 'system:./output/posix/x86_64-unknown-linux-gnu/bin/ot-ncp ./output/x86_64-unknown-linux-gnu/bin/ot-ncp-radio 1'
```

### With Real Device

```sh
# nRF52840
sudo wpantund -s 'system:./output/posix/x86_64-unknown-linux-gnu/bin/ot-ncp /dev/ttyACM0 115200'
# CC2538
sudo wpantund -s 'system:./output/posix/x86_64-unknown-linux-gnu/bin/ot-ncp /dev/ttyUSB0 115200'
```
