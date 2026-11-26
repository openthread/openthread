# Co-Processor Capability Test Suite (cp-caps)

The Co-Processor Capability Test Suite (`cp-caps`) checks which capabilities an OpenThread co-processor (currently an RCP) supports, such as diag commands, IEEE 802.15.4 frame formats, CSL, data poll, and link metrics. It also measures performance metrics, such as Thread network 1-hop throughput.

Unlike a conformance or certification test suite, it profiles the co-processor: each test case reports whether a capability is supported (`OK` or `NotSupported`) or a measured value, rather than an overall pass or fail.

The test suite consists of:

- Test cases (`test_*.py`): each test case tests a capability area and is run using Python's `unittest` framework.
- A test harness (`device_manager.py`): connects to and controls the testbed devices using [OTCI](../otci) and outputs the results.
- A testbed: the devices the test cases run on (see [Testbed](#testbed)).

## Testbed

```
                    +-------+
    +---------------|  PC   |----------------+
    |               +-------+                |
    | ADB/SSH           |                    | ADB/SSH/SERIAL
    |                   | SERIAL             |
+-------+               |           +------------------+
|  DUT  |<-----------Thread-------->| Reference Device |
+-------+               |           +------------------+
                  +-----------+
                  |  Sniffer  | (optional)
                  +-----------+
```

- PC : The computer to run the test script.
- DUT : The device under test.
- Reference Device : The device that supports all tested features.
- Sniffer : An RCP that captures frames with timestamps (e.g. to check the timing of frames). It is optional and only used by some test cases.

### Reference Device

The [nRF52840DK][ot-nrf528xx-nrf52840] is set as the reference device by default. Testers can also select the other Thread device as the reference device.

[ot-nrf528xx-nrf52840]: https://github.com/openthread/ot-nrf528xx/blob/main/src/nrf52840/README.md

### Sniffer

The sniffer is an RCP (e.g. an nRF52840 dongle running `ot-rcp`) driven by [ot-extcap](../extcap/README.md). The test cases capture frames by running `ot-extcap` on the channel of interest, and read each frame with its timestamp in microseconds taken by the sniffer radio. The timing analysis requires the sniffer radio to timestamp frames at the end of SFD, as `otRadioFrame.mInfo.mRxInfo.mTimestamp` specifies.

Quick guide to building `ot-extcap` (from the root of the OpenThread repository):

```bash
$ ./script/cmake-build extcap
```

The executable is at `build/extcap/ot-extcap`. Set `OT_EXTCAP` to its full path (unless it is in the `PATH`), and `SNIFFER_SERIAL` (or `SNIFFER_RADIO_URL`) to the sniffer RCP.

Quick guide to setting up the nRF52840DK:

```bash
$ git clone git@github.com:openthread/ot-nrf528xx.git
$ cd ot-nrf528xx/
$ git submodule update --init
$ ./script/bootstrap
$ ./script/build nrf52840 UART_trans -DOT_DIAGNOSTIC=ON -DOT_CSL_RECEIVER=ON -DOT_LINK_METRICS_INITIATOR=ON -DOT_LINK_METRICS_SUBJECT=ON -DOT_WAKE_INITIATOR=ON
$ arm-none-eabi-objcopy -O ihex build/bin/ot-cli-ftd ot-cli-ftd.hex
$ nrfutil device program --firmware ot-cli-ftd.hex --options chip_erase_mode=ERASE_ALL
$ nrfutil device reset --reset-kind=RESET_PIN
```

### Simulation Testbed

[check-cp-caps](../../tests/scripts/check-cp-caps) runs all test cases on a testbed of simulated devices, and fails if any capability that the simulation RCP supports is reported as `NotSupported`. The CI runs it on every pull request:

```bash
$ ./tests/scripts/check-cp-caps
```

## Python Virtual Environment Setup

It is recommended to use a Python virtual environment to manage project dependencies and avoid conflicts with system-wide packages.

### Creation

Create a virtual environment named `.venv` in the project directory:

```bash
$ cd openthread
$ python3 -m venv .venv
```

### Activation

Activate the virtual environment using the following command:

```bash
$ source .venv/bin/activate
```

After activation, your shell prompt should indicate that you are working within the virtual environment.

### Dependency Installation

Once the virtual environment is activated, install the required dependencies using `pip`:

```bash
$ pip3 install -r ./tools/cp-caps/requirements.txt ./tools/otci
```

### Deactivation

When you are finished working in the virtual environment, you can deactivate it by running the following command. This will return you to your system's global Python environment.

```bash
$ deactivate
```

## Test Commands

### Help Info

```bash
usage: [device configurations] python3 -m unittest -q [test cases]

This test suite is used for testing the capabilities of the RCP.

Device configurations:
  ADB_KEY=<adb_key>                  Full path to the adb key
  DEBUG=on                           Enable the debug information
  DUT_ADB_TCP=<device_ip>            Connect to the DUT via adb tcp
  DUT_ADB_USB=<serial_number>        Connect to the DUT via adb usb
  DUT_ADB_SPINEL_PROXY=<serial>      Connect to the DUT Android device via adb Spinel proxy (host ot-cli)
  DUT_SPINEL_PROXY=1                 Enable adb Spinel proxy mode for DUT_ADB_USB/DUT_ADB_TCP (host ot-cli)
  DUT_SPINEL_PROXY_SOCKET=<path>     Socket path for adb Spinel proxy forwarding
  DUT_SPINEL_PROXY_RADIO_URL_PARAMS=<params>
                                     Radio URL parameters for adb Spinel proxy (e.g. bus-latency=20000)
  DUT_RADIO_URL=<radio_url>          Connect to the DUT via host POSIX ot-cli over radio URL
  DUT_CLI_SERIAL=<serial_device>     Connect to the DUT via cli serial port
  DUT_SSH=<device_ip>                Connect to the DUT via ssh
  REF_ADB_TCP=<device_ip>            Connect to the reference device via adb tcp
  REF_ADB_USB=<serial_number>        Connect to the reference device via adb usb
  REF_ADB_SPINEL_PROXY=<serial>      Connect to the reference Android device via adb Spinel proxy (host ot-cli)
  REF_SPINEL_PROXY=1                 Enable adb Spinel proxy mode for REF_ADB_USB/REF_ADB_TCP (host ot-cli)
  REF_SPINEL_PROXY_SOCKET=<path>     Socket path for reference device adb Spinel proxy forwarding
  REF_SPINEL_PROXY_RADIO_URL_PARAMS=<params>
                                     Radio URL parameters for adb Spinel proxy (e.g. bus-latency=20000)
  REF_RADIO_URL=<radio_url>          Connect to the reference device via host POSIX ot-cli over radio URL
  REF_CLI_SERIAL=<serial_device>     Connect to the reference device via cli serial port
  REF_SSH=<device_ip>                Connect to the reference device via ssh
  OT_CLI=<ot_cli_path>               Full path to the host POSIX ot-cli executable (default: ot-cli)
  SNIFFER_SERIAL=<serial_device>     Use the RCP at the serial port as the sniffer (optional)
  SNIFFER_RADIO_URL=<radio_url>      Use the RCP at the radio URL as the sniffer (optional)
  OT_EXTCAP=<ot_extcap_path>         Full path to the ot-extcap executable used by the sniffer (default: ot-extcap)

Test cases:
  test_ack                 test whether the RCP reports the ACK frames of the transmitted frames correctly
  test_csl                 test whether the RCP supports CSL transmitter (on the PAN channel and another channel)
  test_data_poll           test whether the RCP supports data poll
  test_diag_commands       test whether the RCP supports all diag commands
  test_frame_formats       test whether the RCP supports 802.15.4 frames of all formats
  test_link_metrics        test whether the RCP supports link metrics
  test_radio_frame_tx_info test mTxInfo field of the radio frame
  test_throughput          test Thread network 1-hop throughput

Examples:

Run test cases of the specified categories:
  DUT_ADB_USB=1169UC2F2T0M95OR REF_CLI_SERIAL=/dev/ttyACM0 python3 -m unittest -q test_csl test_data_poll

Run test cases with host POSIX ot-cli over Radio URL (e.g. Unix socket):
  DUT_RADIO_URL="spinel+hdlc+unix:///tmp/ot_spinel_proxy" REF_CLI_SERIAL=/dev/ttyACM0 python3 -m unittest -q test_diag_commands

Run test cases with automated Android adb Spinel proxy setup:
  DUT_ADB_SPINEL_PROXY=1169UC2F2T0M95OR REF_CLI_SERIAL=/dev/ttyACM0 python3 -m unittest -q test_diag_commands

Run test cases with adb Spinel proxy and a bus latency of 20 ms for the adb forwarding:
  DUT_ADB_SPINEL_PROXY=1169UC2F2T0M95OR DUT_SPINEL_PROXY_RADIO_URL_PARAMS=bus-latency=20000 REF_CLI_SERIAL=/dev/ttyACM0 python3 -m unittest -q test_csl

Run all test cases:
  DUT_ADB_USB=1169UC2F2T0M95OR REF_CLI_SERIAL=/dev/ttyACM0 python3 -m unittest discover -q -s ./ -p 'test_*.py'

Run specified test case:
  DUT_ADB_USB=1169UC2F2T0M95OR REF_CLI_SERIAL=/dev/ttyACM0 python3 -m unittest -q test_diag_commands.TestDiagCommands.test_diag_channel

These commands should be run in the path `tools/cp-caps`.
```

> Note: If you get an error of `LIBUSB_ERROR_BUSY` when you are using the ADB USB interface, please run the command `adb kill-server` to kill the ADB server.

### Test Diag Commands

The test case `test_diag_commands` tests all diag commands.

Following environment variables are used to configure diag command parameters:

- DUT_DIAG_GPIO: Diag gpio value. The default value is `0` if it is not set.
- DUT_DIAG_RAW_POWER_SETTING: Diag raw power setting value. The default value is `112233` if it is not set.
- DUT_DIAG_POWER: Diag power value. The default value is `10` if it is not set.

```bash
tools/cp-caps$ DUT_ADB_USB=1269UCKFZTAM95OR REF_CLI_SERIAL=/dev/ttyACM0 DUT_DIAG_GPIO=2 DUT_DIAG_RAW_POWER_SETTING=44556688 DUT_DIAG_POWER=11 python3 -m unittest -q test_diag_commands
diag channel --------------------------------------------- OK
diag channel 20 ------------------------------------------ OK
diag cw start -------------------------------------------- OK
diag cw stop --------------------------------------------- OK
diag echo 0123456789 ------------------------------------- OK
diag echo -n 10 ------------------------------------------ OK
diag frame 00010203040506070809 -------------------------- OK
diag gpio mode 2 ----------------------------------------- OK
diag gpio mode 2 in -------------------------------------- OK
diag gpio mode 2 out ------------------------------------- OK
diag gpio get 2 ------------------------------------------ OK
diag gpio set 2 0 ---------------------------------------- OK
diag gpio set 2 1 ---------------------------------------- OK
diag power ----------------------------------------------- OK
diag power 11 -------------------------------------------- OK
diag powersettings --------------------------------------- NotSupported
diag powersettings 20 ------------------------------------ NotSupported
diag radio receive --------------------------------------- OK
diag radio sleep ----------------------------------------- OK
diag radio state ----------------------------------------- OK
diag rawpowersetting enable ------------------------------ NotSupported
diag rawpowersetting 44556688 ---------------------------- NotSupported
diag rawpowersetting ------------------------------------- NotSupported
diag rawpowersetting disable ----------------------------- NotSupported
diag repeat 10 64 ---------------------------------------- OK
diag repeat stop ----------------------------------------- OK
diag send 100 64 ----------------------------------------- OK
diag stats ----------------------------------------------- OK
diag stats clear ----------------------------------------- OK
diag stream start ---------------------------------------- NotSupported
diag stream stop ----------------------------------------- NotSupported
```

### Test CSL Transmitter

The test case `test_csl` tests whether the RCP supports the CSL transmitter. The reference device attaches to the DUT as an SSED, and the DUT pings it twice:

- `Same Channel`: the CSL channel is the PAN channel, which tests the timing of the CSL transmissions (`mTxDelay`) without channel switching.
- `Cross Channel`: the CSL channel is another channel, which also tests switching to the CSL channel for the CSL transmissions.

```bash
tools/cp-caps$ DUT_ADB_USB=TW69UCKFZTGM95OR REF_CLI_SERIAL=/dev/ttyACM0 python3 -m unittest -q test_csl
CSL Transmitter (Same Channel) --------------------------- OK
CSL Transmitter (Cross Channel) -------------------------- OK
```

If a [sniffer](#sniffer) is configured, it captures the frames on the CSL channel, and a failed sub-test is followed by an analysis of the CSL frame exchanges, which tells a CSL transmitter issue of the DUT from a CSL receiver issue of the reference device. The CSL sample times of the reference device are derived from the CSL IEs it sends (e.g. in the Enh-ACKs), and each data frame from the DUT to the reference device is checked for its offset from the CSL sample time and for its Enh-ACK:

- `CSL transmitter issue (no TX)`: the DUT sent no data frame to the reference device on the CSL channel.
- `CSL transmitter issue (TX timing miss)`: the DUT sent data frames out of the CSL sample window, e.g. the RCP sends them without waiting for the target TX time (`mTxDelayBaseTime` + `mTxDelay`).
- `CSL receiver issue (missing Enh-ACK)`: the DUT sent data frames in the CSL sample window, but the reference device did not acknowledge them.
- `CSL timing inconclusive (clock drift)`: the DUT sent data frames near the edge of the CSL sample window, and the clock drift since the CSL IE they are timed from makes it unknown whether they were in it.
- `CSL timing inconclusive (CSL schedule changed)`: the reference device changed its CSL sample time (e.g. it attached again) between the CSL IEs before and after the data frames, and only one of them puts the data frames in the CSL sample window.
- `CSL transmitter issue (retransmission with new frame counter)`: the DUT retransmitted a secured data frame with another frame counter or key index than its first transmission, e.g. the RCP assigns a new frame counter or key index although `mIsHeaderUpdated` is set. The frame counters are shown as `expected <first>, observed <first>, <retx 1>, ...`. This issue does not change the timing analysis of the data frames.

A data frame is a retransmission if it has the same sequence number and length as the previous data frame to the same address of the reference device, since the reference device last attached.

Each data frame is timed from the last CSL IE before it, or the first CSL IE after it if there is none, from the same attachment of the reference device. The CSL sample window is ±352 us around the CSL sample time, and a frame is only in or out of it if it is so even with the worst clock drift (±40 ppm of each of the sniffer and the reference device) since that CSL IE.

When the CSL channel is not the PAN channel and the reference device sent no Enh-ACK on the CSL channel, no CSL IE is captured to time the data frames, so the analysis refers to the result of the same channel sub-test.

Whenever data frames are timed, a `CSL TX timing` line follows, also for passed sub-tests. It shows the offsets of the timed data frames, acknowledged or not, and models them as a fixed bias, a clock drift and a jitter:

- `bias`: the median offset after removing the clock drift.
- `jitter`: the half range of the offsets after removing the clock drift.
- `drift vs. sniffer`: the clock drift of the DUT relative to the sniffer (± its standard error), which is not the clock accuracy of the DUT because the clock of the sniffer also drifts. It is estimated from the data frames timed from the same CSL IE (e.g. the retransmissions of an unacknowledged frame), so it is `unknown (insufficient span)` unless they span at least 600 ms.

On the PAN channel, `CSL TX timing warning` lines show if the offsets after removing the clock drift exceed the CSL uncertainty of the DUT (`csl uncertainty`), or if the clock drift exceeds the CSL accuracy of the DUT (`csl accuracy`) plus 40 ppm for the sniffer by more than its standard error. If the clock drift is unknown, the offsets may also exceed the CSL uncertainty by that clock drift since the CSL IEs they are timed from. The DUT advertises these values to the reference device to widen its CSL sample window. The warnings do not change the result of the sub-test. They are not shown for other channels: the sniffer does not capture the CSL IEs that the reference device sends on the PAN channel, from which the DUT may time data frames, and since the CSL Phase is truncated to units of 160 us, a data frame timed from another CSL IE than the DUT used is off by up to 160 us.

```bash
tools/cp-caps$ DUT_ADB_USB=TW69UCKFZTGM95OR REF_CLI_SERIAL=/dev/ttyACM0 SNIFFER_SERIAL=/dev/ttyACM1 python3 -m unittest -q test_csl
CSL Transmitter (Same Channel) --------------------------- NotSupported
  CSL transmitter issue (TX timing miss): 4/4 data frames out of the CSL sample window (offsets +552, +565, +561, +591 us, tolerance 352 us, clock drift up to 111 us for CSL IEs up to 1381 ms old)
  CSL TX timing: 4 data frames, offsets min +552 / median +563 / max +591 us, bias +538 us, jitter ±9 us, drift vs. sniffer +35 ± 14 ppm
CSL Transmitter (Cross Channel) -------------------------- NotSupported
  4/4 data frames on channel 15 were not acknowledged, and no CSL IE from the CSL receiver was captured on channel 15 to time them
  Same channel (20): CSL transmitter issue (TX timing miss): 4/4 data frames out of the CSL sample window (offsets +552, +565, +561, +591 us, tolerance 352 us, clock drift up to 111 us for CSL IEs up to 1381 ms old)
```

The data frames sent while the reference device polls data or attaches again are not CSL transmissions, so they are not analyzed. CSL IEs whose CSL Period is not the configured one, or whose CSL Phase is not less than the CSL Period, are ignored. When the CSL channel is not the PAN channel, the sniffer cannot see the reference device attaching again, so the data frames after that are timed from its old CSL sample time until it sends a CSL IE on the CSL channel.

With `DEBUG=on`, the analysis is also shown for passed sub-tests, and the captures are kept in PCAPNG files, whose paths are shown.

The unit tests of the analysis do not need any device:

```bash
tools/cp-caps$ python3 tests/test_csl_analyzer.py
```

### Test Data Poll

The test case `test_data_poll` tests whether the RCP supports data poll.

```bash
tools/cp-caps$ DUT_ADB_USB=TW69UCKFZTGM95OR REF_CLI_SERIAL=/dev/ttyACM0 python3 -m unittest -q test_data_poll
Data Poll Child ------------------------------------------ OK
Data Poll Parent ----------------------------------------- OK
```

### Test ACK

The test case `test_ack` tests whether the RCP reports the ACK frames of the transmitted frames correctly. The DUT attaches to the reference device as a child and pings it:

- `Imm-Ack`: the RCP reports each Imm-Ack in the transmit done response with the 2-byte FCS (`otRadioFrame.mLength` is 5), even if the radio has verified and removed the FCS. The host fails to parse an Imm-Ack reported without the FCS, so this is checked by comparing the number of ACKs the host accepted with the number of ACKs the RCP reported, which are shown with the result: the host accepts every reported ACK unless the RCP reports Imm-Acks without the FCS.

A host with `OPENTHREAD_SPINEL_CONFIG_RCP_IMM_ACK_FCS_COMPATIBILITY_WORKAROUND_ENABLE` (enabled by default) restores the FCS of an Imm-Ack reported without it, which hides the issue from this test case. Run it with a host POSIX `ot-cli` built with the workaround disabled:

```bash
$ ./script/cmake-build posix -DOT_DIAGNOSTIC=ON -DCMAKE_CXX_FLAGS=-DOPENTHREAD_SPINEL_CONFIG_RCP_IMM_ACK_FCS_COMPATIBILITY_WORKAROUND_ENABLE=0
```

```bash
tools/cp-caps$ DUT_ADB_SPINEL_PROXY=TW69UCKFZTGM95OR OT_CLI=../../build/posix/src/posix/ot-cli REF_CLI_SERIAL=/dev/ttyACM0 python3 -m unittest -q test_ack
Imm-Ack -------------------------------------------------- OK (10/10 ACKs)
```

### Test Link Metrics

The test case `test_link_metrics` tests whether the RCP supports link metrics.

```bash
tools/cp-caps$ DUT_ADB_USB=TW69UCKFZTGM95OR REF_CLI_SERIAL=/dev/ttyACM0 python3 -m unittest -q test_link_metrics
Link Metrics Initiator ----------------------------------- OK
Link Metrics Subject ------------------------------------- OK
```

### Test Throughput

The test case `test_throughput` tests the Thread network 1-hop throughput of the DUT.

```bash
tools/cp-caps$ DUT_ADB_USB=TW69UCKFZTGM95OR REF_ADB_USB=44061HFAG01AQK python3 -m unittest -q test_throughput
Throughput ----------------------------------------------- 75.6 Kbits/sec
```

### Test Frame Format

The test case `test_frame_formats` tests whether the RCP supports sending and receiving 802.15.4 frames of all formats.

In the RX direction, the DUT disables promiscuous mode and configures its radio address filter (`diag radio panid`, `diag radio shortaddr` and `diag radio extaddr`) with the destination of each frame, so the RX results also cover the radio address filtering behaviors documented in `otPlatRadioEnable()`. For example, frames without a destination address must be accepted. The reference device stays in promiscuous mode, so the TX results do not depend on its address filter. If the DUT does not support the `diag radio promiscuous` command, the test prints a warning and receives in promiscuous mode, so the RX results do not cover address filtering.

```bash
tools/cp-caps$ DUT_ADB_USB=TW69UCKFZTGM95OR REF_CLI_SERIAL=/dev/ttyACM0 python3 -m unittest -q test_frame_formats
1 TX ver:2003,Cmd,seq,dst[addr:short,pan:id],src[addr:no,pan:no],sec:no,ie:no,plen:0 --------------- OK
1 RX ver:2003,Cmd,seq,dst[addr:short,pan:id],src[addr:no,pan:no],sec:no,ie:no,plen:0 --------------- OK
2 TX ver:2003,Bcon,seq,dst[addr:no,pan:no],src[addr:extd,pan:id],sec:no,ie:no,plen:30 -------------- OK
2 RX ver:2003,Bcon,seq,dst[addr:no,pan:no],src[addr:extd,pan:id],sec:no,ie:no,plen:30 -------------- OK
3 TX ver:2015,MP,noseq,dst[addr:extd,pan:id],src[addr:extd,pan:no],sec:l5,ie[ren con],plen:0 ------- OK
3 RX ver:2015,MP,noseq,dst[addr:extd,pan:id],src[addr:extd,pan:no],sec:l5,ie[ren con],plen:0 ------- OK
4 TX ver:2006,Cmd,seq,dst[addr:short,pan:id],src[addr:short,pan:no],sec:l5,ie:no,plen:0 ------------ OK
4 RX ver:2006,Cmd,seq,dst[addr:short,pan:id],src[addr:short,pan:no],sec:l5,ie:no,plen:0 ------------ OK
5 TX ver:2006,Cmd,seq,dst[addr:extd,pan:id],src[addr:extd,pan:no],sec:l5,ie:no,plen:0 -------------- OK
5 RX ver:2006,Cmd,seq,dst[addr:extd,pan:id],src[addr:extd,pan:no],sec:l5,ie:no,plen:0 -------------- OK
6 TX ver:2006,Data,seq,dst[addr:extd,pan:id],src[addr:extd,pan:no],sec:no,ie:no,plen:0 ------------- OK
6 RX ver:2006,Data,seq,dst[addr:extd,pan:id],src[addr:extd,pan:no],sec:no,ie:no,plen:0 ------------- OK
7 TX ver:2006,Data,seq,dst[addr:short,pan:id],src[addr:short,pan:id],sec:no,ie:no,plen:0 ----------- OK
7 RX ver:2006,Data,seq,dst[addr:short,pan:id],src[addr:short,pan:id],sec:no,ie:no,plen:0 ----------- OK
8 TX ver:2006,Data,seq,dst[addr:extd,pan:id],src[addr:no,pan:no],sec:no,ie:no,plen:0 --------------- OK
8 RX ver:2006,Data,seq,dst[addr:extd,pan:id],src[addr:no,pan:no],sec:no,ie:no,plen:0 --------------- OK
9 TX ver:2006,Data,seq,dst[addr:short,pan:id],src[addr:no,pan:no],sec:no,ie:no,plen:0 -------------- OK
9 RX ver:2006,Data,seq,dst[addr:short,pan:id],src[addr:no,pan:no],sec:no,ie:no,plen:0 -------------- OK
10 TX ver:2015,Data,seq,dst[addr:no,pan:no],src[addr:no,pan:no],sec:no,ie:no,plen:0 ---------------- OK
10 RX ver:2015,Data,seq,dst[addr:no,pan:no],src[addr:no,pan:no],sec:no,ie:no,plen:0 ---------------- OK
11 TX ver:2015,Data,seq,dst[addr:no,pan:id],src[addr:no,pan:no],sec:no,ie:no,plen:0 ---------------- OK
11 RX ver:2015,Data,seq,dst[addr:no,pan:id],src[addr:no,pan:no],sec:no,ie:no,plen:0 ---------------- OK
12 TX ver:2015,Data,seq,dst[addr:extd,pan:id],src[addr:no,pan:no],sec:no,ie:no,plen:0 -------------- OK
12 RX ver:2015,Data,seq,dst[addr:extd,pan:id],src[addr:no,pan:no],sec:no,ie:no,plen:0 -------------- OK
13 TX ver:2015,Data,seq,dst[addr:extd,pan:no],src[addr:no,pan:no],sec:no,ie:no,plen:0 -------------- OK
13 RX ver:2015,Data,seq,dst[addr:extd,pan:no],src[addr:no,pan:no],sec:no,ie:no,plen:0 -------------- OK
14 TX ver:2015,Data,seq,dst[addr:no,pan:no],src[addr:extd,pan:id],sec:no,ie:no,plen:0 -------------- OK
14 RX ver:2015,Data,seq,dst[addr:no,pan:no],src[addr:extd,pan:id],sec:no,ie:no,plen:0 -------------- OK
15 TX ver:2015,Data,seq,dst[addr:no,pan:no],src[addr:extd,pan:no],sec:no,ie:no,plen:0 -------------- OK
15 RX ver:2015,Data,seq,dst[addr:no,pan:no],src[addr:extd,pan:no],sec:no,ie:no,plen:0 -------------- OK
16 TX ver:2015,Data,seq,dst[addr:extd,pan:id],src[addr:extd,pan:no],sec:no,ie:no,plen:0 ------------ OK
16 RX ver:2015,Data,seq,dst[addr:extd,pan:id],src[addr:extd,pan:no],sec:no,ie:no,plen:0 ------------ OK
17 TX ver:2015,Data,seq,dst[addr:extd,pan:no],src[addr:extd,pan:no],sec:no,ie:no,plen:0 ------------ OK
17 RX ver:2015,Data,seq,dst[addr:extd,pan:no],src[addr:extd,pan:no],sec:no,ie:no,plen:0 ------------ OK
18 TX ver:2015,Data,seq,dst[addr:short,pan:id],src[addr:short,pan:id],sec:no,ie:no,plen:0 ---------- OK
18 RX ver:2015,Data,seq,dst[addr:short,pan:id],src[addr:short,pan:id],sec:no,ie:no,plen:0 ---------- OK
19 TX ver:2015,Data,seq,dst[addr:short,pan:id],src[addr:extd,pan:id],sec:no,ie:no,plen:0 ----------- OK
19 RX ver:2015,Data,seq,dst[addr:short,pan:id],src[addr:extd,pan:id],sec:no,ie:no,plen:0 ----------- OK
20 TX ver:2015,Data,seq,dst[addr:extd,pan:id],src[addr:short,pan:id],sec:no,ie:no,plen:0 ----------- OK
20 RX ver:2015,Data,seq,dst[addr:extd,pan:id],src[addr:short,pan:id],sec:no,ie:no,plen:0 ----------- OK
21 TX ver:2015,Data,seq,dst[addr:short,pan:id],src[addr:short,pan:id],sec:no,ie[csl],plen:0 -------- OK
21 RX ver:2015,Data,seq,dst[addr:short,pan:id],src[addr:short,pan:id],sec:no,ie[csl],plen:0 -------- OK
22 TX ver:2015,Data,noseq,dst[addr:short,pan:id],src[addr:short,pan:id],sec:no,ie:no,plen:0 -------- OK
22 RX ver:2015,Data,noseq,dst[addr:short,pan:id],src[addr:short,pan:id],sec:no,ie:no,plen:0 -------- OK
23 TX ver:2015,Data,seq,dst[addr:extd,pan:id],src[addr:extd,pan:no],sec:l5,keyid:2,ie:no,plen:0 ---- OK
23 RX ver:2015,Data,seq,dst[addr:extd,pan:id],src[addr:extd,pan:no],sec:l5,keyid:2,ie:no,plen:0 ---- OK
```

### Test mTxInfo field of the radio frame.

The test case `test_radio_frame_tx_info` tests whether the RCP supports the mTxInfo field of the radio frame.

If a [sniffer](#sniffer) is configured, the sub-test `mTxDelay,mCsmaCaEnabled=1 vs 0` checks that the DUT sends delayed frames on time whether CCA is enabled or not, as `mTxDelayBaseTime` + `mTxDelay` is the time of the end of the SFD. The DUT sends delayed frames 200 ms apart from the same `mTxDelayBaseTime`, with `mCsmaCaEnabled` alternately 0 and 1. The offset of each frame with CCA is its timestamp minus the average timestamp of the frames without CCA before and after it, which cancels the clock drift between the DUT and the sniffer. The sub-test passes if the median offset is within ±64 us (half of the CCA duration); a radio that does not start CCA early enough sends the frames with CCA late, and a radio that starts every frame early by the CCA time sends the frames without CCA early.

The sub-tests `mTxDelay past due,mCsmaCaEnabled=0` and `mTxDelay past due,mCsmaCaEnabled=1` check delayed frames without ACK request that reach the radio near or after their scheduled time, e.g. due to the latency between the host and the RCP. The DUT measures the time `L` to read the radio time, set and send a frame, and sends 20 delayed frames scheduled from `L` before to `3 * L` after the radio time read right before setting each frame. The radio may send a late frame right away or abort it, so the sub-test only fails if the radio reports an error other than `OT_ERROR_ABORT` or `OT_ERROR_CHANNEL_ACCESS_FAILURE`, such as `OT_ERROR_NO_ACK`. The result shows how many frames were sent, aborted or failed and, if a sniffer is configured, how late the latest frame was sent after its scheduled time (relative to the frame sent the earliest relative to its scheduled time).

```bash
tools/cp-caps$ DUT_ADB_USB=TW69UCKFZTGM95OR REF_CLI_SERIAL=/dev/ttyACM0 SNIFFER_SERIAL=/dev/ttyACM1 python3 -m unittest -q test_radio_frame_tx_info
mCsmaCaEnabled=0 ----------------------------------------- OK
mCsmaCaEnabled=1 ----------------------------------------- OK
mIsHeaderUpdated=True ------------------------------------ OK
mIsHeaderUpdated=False ----------------------------------- OK
mIsSecurityProcessed=True,KeyIdMode0 --------------------- OK
mIsSecurityProcessed=True,KeyIdMode1 --------------------- OK
mIsSecurityProcessed=True,KeyIdMode2 --------------------- OK
mIsSecurityProcessed=False ------------------------------- OK
mMaxCsmaBackoffs=0 --------------------------------------- OK (12 ms)
mMaxCsmaBackoffs=100 ------------------------------------- OK (541 ms)
mRxChannelAfterTxDone ------------------------------------ OK
mTxDelayBaseTime=now,mTxDelay=500000 --------------------- OK
mTxDelay,mCsmaCaEnabled=1 vs 0 --------------------------- OK (median -3 us, mean -5 us, 10 frames)
mTxDelay past due,mCsmaCaEnabled=0 ----------------------- OK (20 sent, 0 aborted, 0 CCA failures, 0 other errors, up to 1994 us late)
mTxDelay past due,mCsmaCaEnabled=1 ----------------------- OK (20 sent, 0 aborted, 0 CCA failures, 0 other errors, up to 1917 us late)
```
