# RCP Capabilities Test

This test is used for testing RCP capabilities.

## Test Topology

```
                    +-------+
    +---------------|  PC   |----------------+
    |               +-------+                |
    | ADB/SSH                                | ADB/SSH/SERIAL
    |                                        |
+-------+                           +------------------+
|  DUT  |<-----------Thread-------->| Reference Device |
+-------+                           +------------------+

```

- PC : The computer to run the test script.
- DUT : The device under test.
- Reference Device : The device that supports all tested features.

### Python Dependences

Before running the test script on PC, testers should install dependences first.

```bash
$ pip3 install -r ./tools/cp-caps/requirements.txt ./tools/otci
```

### Reference Device

The [nRF52840DK][ot-nrf528xx-nrf52840] is set as the reference device by default. Testers can also select the other Thread device as the reference device.

[ot-nrf528xx-nrf52840]: https://github.com/openthread/ot-nrf528xx/blob/main/src/nrf52840/README.md

Quick guide to setting up the nRF52840DK:

```bash
$ git clone git@github.com:openthread/ot-nrf528xx.git
$ cd ot-nrf528xx/
$ git submodule update --init
$ ./script/bootstrap
$ ./script/build nrf52840 UART_trans -DOT_DIAGNOSTIC=ON -DOT_CSL_RECEIVER=ON -DOT_LINK_METRICS_INITIATOR=ON -DOT_LINK_METRICS_SUBJECT=ON
$ arm-none-eabi-objcopy -O ihex build/bin/ot-cli-ftd ot-cli-ftd.hex
$ nrfjprog -f nrf52 --chiperase --program ot-cli-ftd.hex --reset
```

## Test Commands

### Help

Show help info.

```bash
$ python3 ./tools/cp-caps/rcp_caps_test.py -h
usage: rcp_caps_test.py [-h] [-c] [-d] [-p] [-t] [-v] [-D]

This script is used for testing RCP capabilities.

options:
  -h, --help           show this help message and exit
  -c, --csl            test whether the RCP supports CSL transmitter
  -d, --diag-commands  test whether the RCP supports all diag commands
  -l, --link-metrics   test whether the RCP supports link metrics
  -p, --data-poll      test whether the RCP supports data poll
  -t, --throughput     test the Thread network 1-hop throughput
  -v, --version        output version
  -D, --debug          output debug information

Device Interfaces:
  DUT_SSH=<device_ip>            Connect to the DUT via ssh
  DUT_ADB_TCP=<device_ip>        Connect to the DUT via adb tcp
  DUT_ADB_USB=<serial_number>    Connect to the DUT via adb usb
  REF_CLI_SERIAL=<serial_device> Connect to the reference device via cli serial port
  REF_ADB_USB=<serial_number>    Connect to the reference device via adb usb
  REF_SSH=<device_ip>            Connect to the reference device via ssh

Example:
  DUT_ADB_USB=1169UC2F2T0M95OR REF_CLI_SERIAL=/dev/ttyACM0 python3 ./tools/cp-caps/rcp_caps_test.py -d
```

### Test Diag Commands

The parameter `-d` or `--diag-commands` starts to test all diag commands.

Following environment variables are used to configure diag command parameters:

- DUT_DIAG_GPIO: Diag gpio value. The default value is `0` if it is not set.
- DUT_DIAG_RAW_POWER_SETTING: Diag raw power setting value. The default value is `112233` if it is not set.
- DUT_DIAG_POWER: Diag power value. The default value is `10` if it is not set.

> Note: If you meet the error `LIBUSB_ERROR_BUSY` when you are using the ADB usb interface, please run the command `adb kill-server` to kill the adb server.

```bash
$ DUT_ADB_USB=1269UCKFZTAM95OR REF_CLI_SERIAL=/dev/ttyACM0 DUT_DIAG_GPIO=2 DUT_DIAG_RAW_POWER_SETTING=44556688 DUT_DIAG_POWER=11 python3 ./tools/cp-caps/rcp_caps_test.py -d
diag channel --------------------------------------------- OK
diag channel 20 ------------------------------------------ OK
diag power ----------------------------------------------- OK
diag power 11 -------------------------------------------- OK
diag radio sleep ----------------------------------------- OK
diag radio receive --------------------------------------- OK
diag radio state ----------------------------------------- OK
diag repeat 10 64 ---------------------------------------- OK
diag repeat stop ----------------------------------------- OK
diag send 100 64 ----------------------------------------- OK
diag stats ----------------------------------------------- OK
diag stats clear ----------------------------------------- OK
diag frame 00010203040506070809 -------------------------- OK
diag echo 0123456789 ------------------------------------- OK
diag echo -n 10 ------------------------------------------ OK
diag cw start -------------------------------------------- OK
diag cw stop --------------------------------------------- OK
diag stream start ---------------------------------------- OK
diag stream stop ----------------------------------------- OK
diag stats ----------------------------------------------- OK
diag stats clear ----------------------------------------- OK
diag rawpowersetting enable ------------------------------ NotSupported
diag rawpowersetting 44556688 ---------------------------- NotSupported
diag rawpowersetting ------------------------------------- NotSupported
diag rawpowersetting disable ----------------------------- NotSupported
diag powersettings --------------------------------------- NotSupported
diag powersettings 20 ------------------------------------ NotSupported
diag gpio mode 2 ----------------------------------------- NotSupported
diag gpio mode 2 in -------------------------------------- NotSupported
diag gpio mode 2 out ------------------------------------- NotSupported
diag gpio get 2 ------------------------------------------ NotSupported
diag gpio set 2 0 ---------------------------------------- NotSupported
diag gpio set 2 1 ---------------------------------------- NotSupported
```

### Test CSL Transmitter

The parameter `-c` or `--csl` starts to test whether the RCP supports the CSL transmitter.

```bash
$ DUT_ADB_USB=TW69UCKFZTGM95OR REF_CLI_SERIAL=/dev/ttyACM0 python3 ./tools/cp-caps/rcp_caps_test.py -c
CSL Transmitter ------------------------------------------ OK
```

### Test Data Poll

The parameter `-p` or `--data-poll` starts to test whether the RCP supports data poll.

```bash
$ DUT_ADB_USB=1269UCKFZTAM95OR REF_CLI_SERIAL=/dev/ttyACM0 python3 ./tools/cp-caps/rcp_caps_test.py -p
Data Poll Parent ----------------------------------------- OK
Data Poll Child ------------------------------------------ OK
```

### Test Link Metrics

The parameter `-l` or `--link-metrics` starts to test whether the RCP supports link metrics.

```bash
$ DUT_ADB_USB=1269UCKFZTAM95OR REF_CLI_SERIAL=/dev/ttyACM0 python3 ./tools/cp-caps/rcp_caps_test.py -l
Link Metrics Initiator ----------------------------------- OK
Link Metrics Subject ------------------------------------- OK
```

### Test Throughput

The parameter `-t` or `--throughput` starts to test the Thread network 1-hop throughput of the DUT.

```bash
$ DUT_ADB_USB=1269UCKFZTAM95OR REF_ADB_USB=44061HFAG01AQK python3 ./tools/cp-caps/rcp_caps_test.py -t
Throughput ----------------------------------------------- 75.6 Kbits/sec
```
