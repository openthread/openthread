# OpenThread BLE - BLE Platform Testing Example

The OpenThread BLE Platform APIs may be invoked via the OpenThread CLI.

## Quick Start

### Build with BLE Platform API support

Use the `BLE=1` build switch to enable BLE Platform API support.

```bash
> ./bootstrap
> make -f examples/Makefile-posix BLE=1
```

### Form a connection

Form a connection between two devices.

### Node 1

On node 1, setup a GATT server by advertising as a peripheral device.

```bash
> ble start
Done
> ble adv start
Done
> ble bdaddr
5ee57b60f032 [1]
Done
```

### Node 2

On node 2, setup a GATT client by scanning, connecting, and performing
GATT discovery as a central device.

```bash
> ble start
Done

> ble scan start
Done
Got BLE_ADV from 5ee57b60f032 [1] - 020104020a00

> ble scan stop
Done

> ble conn start 5ee57b60f032 1
Done
BLE connected: @id=42

> ble gatt disc services
Done
service: uuid=0x1800 @start=1 @end=5
service: uuid=0x1801 @start=6 @end=9
service: uuid=0x1811 @start=10 @end=65535

> ble gatt disc char 1 5
Done
characteristic: @value=3 @cccd=0 properties=0x02 uuid=0x2a00

> ble gatt read 3
Done
6e696d626c65

> ble gatt write 3 123456
Done

> ble gatt read 3
Done
123456
```

## Command List

* [help](#help)
* [bdaddr](#ble-bdaddr)
* [start](#ble-start)
* [stop](#ble-stop)
* [adv start](#ble-adv-start)
* [adv stop](#ble-adv-stop)
* [adv data](#ble-adv-data-payload)
* [scan rsp](#ble-scan-rsp-payload)
* [scan start](#ble-scan-start)
* [scan stop](#ble-scan-stop)
* [conn start](#ble-conn-start-bdaddr-bdaddr_type)
* [conn stop](#ble-conn-stop)
* [gatt read](#ble-gatt-read-handle)
* [gatt write](#ble-gatt-write-handle-data)
* [gatt discover services](#ble-gatt-discover-services)
* [gatt subscribe](#ble-gatt-subscribe-handle)
* [gatt unsubscribe](#ble-gatt-unsubscribe-handle)

## Command Details

### help

```bash
> ble help
help
bdaddr
start
stop
adv
scan
conn
mtu
gatt
ch
reset
Done
```

List the BLE CLI commands.

### ble start

Enable the BLE stack.

```bash
> ble start
Done
```

### ble stop

Disable the BLE stack.

```bash
> ble stop
Done
```

### ble adv start

Enable BLE advertising.

```bash
> ble adv start
Done
```

### ble adv stop

Disable BLE advertising.

```bash
> ble adv stop
Done
```

### ble adv data \<payload\>

Set the advertising data payload.

* payload: Advertising data payload in hexidecimal.

```bash
> ble adv data 020401020304
Done
```

### ble scan rsp \<payload\>

Set the advertising scan response payload.

* payload: Scan response payload in hexidecimal.

```bash
> ble scan rsp 0b096f70656e746872656164
Done
```

### ble scan start

Start BLE scanning.

```bash
> ble scan start
Done
```

### ble scan stop

Stop BLE scanning.

```bash
> ble scan stop
Done
```

### ble conn start \<bdaddr\> \<bdaddr_type\>

Start a BLE connection to the given Bluetooth Device Address.

* bdaddr: Bluetooth Device Address to connect to in big endian hexidecimal
* bdaddr_type: Bluetooth Device Address Type, where:
  - 0 = public
  - 1 = random static
  - 2 = random private resolvable
  - 3 = random private non-resolvable

```bash
> ble conn start 24000001aa00
Done
```

### ble conn stop

Stop a BLE connection.

```bash
> ble conn stop
Done
```

### ble bdaddr

Get Bluetooth Device Address.

```bash
> ble bdaddr
23000001aa00
Done
```

### ble gatt read \<handle\>

Read BLE Attribute at given handle.

* handle: Offset into attribute database of field to read

```bash
> ble gatt read 3
546f424c4500
Done
```

### ble gatt write \<handle\> \<data\>

Write given data to BLE Attribute at given handle.

* handle: Offset into attribute database of field to write
* data:   Data to write to field in hexidecimal format.

```bash
> ble gatt write 10 01020304
Done
```

### ble gatt discover services

Discover GATT services on remote node.

```bash
> ble gatt disc services
service uuid=0x1800 start=1 end=6
service uuid=0x1801 start=7 end=7
service uuid=0xfeaf start=8 end=13
Done
```

### ble gatt subscribe \<handle\>

Subscribe to indications of GATT Attribute updates.

* handle: Offset into attribute database of field to subscribe to

```bash
> ble gatt subscribe 3
Done
```

### ble gatt unsubscribe \<handle\>

Unsubscribe to indications of GATT Attribute updates.

* handle: Offset into attribute database of field to unsubscribe to

```bash
> ble gatt unsubscribe 3
Done
```
