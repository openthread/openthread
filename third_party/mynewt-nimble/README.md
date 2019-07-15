# OpenThread BLE Host Stack Reference

The OpenThread BLE host reference provides a minimal reference implementation
of L2CAP, ATT, and GATT to support simulation testing of the OpenThread BLE features.

## Building OpenThread BLE Simulation

The OpenThread BLE simulator uses btvirt interfaces provided by Bluez.
In order to prepar the Bluez third_party repo, execute the following commands:

```
# Clone bluez, install dependencies, manually configure
sudo apt-get install automake libtool libglib2.0-dev libdbus-1-dev elfutils libudev-dev libical-dev libreadline-dev libusb-dev

./bootstrap
./configure
# Note, on Ubuntu 14.04, you may need to configure as follows:
# ./configure --prefix=/usr --mandir=/usr/share/man --sysconfdir=/etc --localstatedir=/var --enable-experimental --with-systemdsystemunitdir=/lib/systemd/system --with-systemduserunitdir=/usr/lib/systemd
```

## Build ##

To clone and prepare a Bluetooth capable tree for the first time:

```git clone git@github.com:openthread/openthread.git openthread-ble
cd openthread-ble
git submodule update --init --remote
```

To build OpenThread with NimBLE as the host stack, use the following configure command:

```./bootstrap
./configure --with-examples=posix  --enable-ftd --enable-cli --enable-ble --with-ble-host=nimble
make
```

or

```./bootstrap
BLE=1 make -f examples/Makefile-posix
```
