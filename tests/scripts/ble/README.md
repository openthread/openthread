## BLE platform tests

To build the cli for the ble platform tests:

```
V=1 FULL_LOGS=1 BLE=1 BLE_HOST=ot BLE_LL=hci make -f examples/Makefile-posix
```

To run BLE simulation tests, sudo is required:

```
sudo VERBOSE=1 NODE_TYPE=sim OT_CLI_PATH=../../../examples/apps/cli/ot-cli-ftd ./test_beacon.py

sudo VERBOSE=1 NODE_TYPE=sim OT_CLI_PATH=../../../examples/apps/cli/ot-cli-ftd ./test_conn.py
```

Watch out for stray HCI simulators:

```
$ ps aux | grep btvirt

root      64552  0.0  0.0   4464   676 pts/12   S    22:25   0:00 ../../../third_party/bluez/repo/emulator/btvirt -l3 -L

$ sudo killall btvirt
```

To run a test against existing HCI controllers:

```
sudo NO_BTVIRT_SETUP=1 VERBOSE=1 NODE_TYPE=sim OT_CLI_PATH=../../../output/x86_64-unknown-linux-gnu/bin/ot-cli-ftd ./test_gatt_discover.py
```