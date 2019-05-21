# OpenThread CLI - BLE Example

The OpenThread BLE Platform APIs will be invoked via the OpenThread CLI.

## Quick Start

### Build with BLE support

Use the `CLI_BLE=1` build switch to enable BLE Host and BLE cli support.
Use the `BLE_CONTROLLER=1` build switch to enable BLE Controller support.
Use the `BLE_L2CAP=1` build switch to enable BLE Platform L2CAP API support.

#### On platform example/platform/posix

```bash
> ./bootstrap
> make -f examples/Makefile-posix CLI_BLE=1 BLE_CONTROLLER=1 BLE_L2CAP=1
> ./output/x86_64-unknown-linux-gnu/bin/ot-cli-ftd 1
```

#### On platform src/posix
The platform src/posix only support BLE dual-chip modes. The BEL Host comunicates with the BLE controller through HCI UART interface.
The hardware of BLE controller can use dev board [nRF52840DK](https://www.nordicsemi.com/Software-and-Tools/Development-Kits/nRF52840-DK).
The source code of BLE controller can get from [zephyr](https://github.com/zephyrproject-rtos/zephyr) and you can follow the guide [zephyr-ble-controller](https://devzone.nordicsemi.com/b/blog/posts/nrf5x-support-within-the-zephyr-project-rtos) to learn how to compile BLE Controller code and donwload the binary to dev borad.The default bluetooth public address of the BLE Controller is zero. You need to add the following code to [samples/bluetooth/hci_uart/src/main.c](https://github.com/zephyrproject-rtos/zephyr/blob/master/samples/bluetooth/hci_uart/src/main.c#L355) to set the bluetooth public address:
```bash
#include <bluetooth/controller.h>

void main(void)
{
    ...
    uint8_t ble_public_addr[6] = {0x01, 0x00, 0x00, 0x00, 0x79, 0x2b};
    bt_ctlr_set_public_addr(ble_public_addr);
    ...
}
```

Compile and run ot-cli:
```bash
> ./bootstrap
# build ot-ncp-radio for NCP radio simulation
> make -f examples/Makefile-posix
# build ot-cli with BLE Host support
> make -f src/posix/Makefile-posix  CLI_BLE=1 BLE_L2CAP=1
> ./output/posix/x86_64-unknown-linux-gnu/bin/ot-cli ./output/x86_64-unknown-linux-gnu/bin/ot-ncp-radio 1 -d /dev/ttyACM0 -b 1000000
```

### Initialize BLE Connection

Initialize a BLE Connection between two devices.

### Node 1

On node 1, set it as an advertizer.

```bash
> ble enable
Done
> BLE is enabled
> 
> ble adv start
Done
>
> ble bdaddr
0 2b7900000001
Done
>
```

### Node 2

On node 2, set it as an scanner and initialize a BLE connection.

```bash
> ble enable
Done
> BLE is enabled
>
> ble scan start 2
Done
| advType | addrType |   address    | rssi | AD or Scan Rsp Data |
+---------+----------+--------------+------+---------------------|
| ADV     |    1     | 02d809fc76b5 | -43  | 1eff0600010f200229ad2cce0007f35452528fce771e14bb0255a0fb0b7f38
| ADV     |    1     | 09c0a8424fa8 | -38  | 1eff0600010920023b3639f6d1f193f8b0dffaa5ed0b7f0ae79a7f27ef0a3d
| ADV     |    1     | 4607f1156334 | -45  | 02011a0aff4c0010050b1c53f803
| SCAN_RSP|    1     | 4607f1156334 | -45  | 
| ADV     |    1     | 02d809fc76b5 | -44  | 1eff0600010f200229ad2cce0007f35452528fce771e14bb0255a0fb0b7f38
| ADV     |    0     | 2b7900000001 | -15  | 
| SCAN_RSP|    0     | 2b7900000001 | -14  | 
| ADV     |    1     | 09c0a8424fa8 | -49  | 1eff060001092002556e54baed057ce630e71acebafcac25c5c4f02de67cb5
| SCAN_RSP|    1     | 5999a0756757 | -49  | 
| ADV     |    0     | 2b7900000001 | -15  | 
| SCAN_RSP|    0     | 2b7900000001 | -14  | 
| ADV     |    1     | 7a1f4ae92a63 | -49  | 02011a0aff4c001005131c44fdf4
| SCAN_RSP|    1     | 7a1f4ae92a63 | -49  | 
| ADV     |    1     | 52943ab2bbcd | -48  | 02011a0aff4c0010050318fedc9d
| SCAN_RSP|    1     | 52943ab2bbcd | -48  |
>
> ble connect 0 2b7900000001
Done
> GapOnConnected: connectionId = 1
>
```

### Node 1

On node 1, generate a BLE Connection event.
```bash
>
> GapOnConnected: connectionId = 1
>
```

### Initialize BLE L2CAP Coc Connection

### Node 1

On node 1, register it as a L2CAP Coc connection acceptor.

```bash
>
> ble l2cap register 1 1 1000 55
L2cap Handle: 1
Done
>
```

### Node 2

On node 2, register it as a L2CAP Coc connection initiator and initialize a L2CAP Coc connection.

```bash
>
> ble l2cap register 1 0 1000 55
L2cap Handle: 1
Done
>
> ble l2cap connect 1
Done
> L2capOnConnectionResponseReceived: aL2capHandle = 1, aMtu = 1000
>
> ble l2cap send 1 400
Done
> L2capOnSduSent: aL2capHandle = 1, error = 0
>
```

### Node 1

On node 1, generate a L2CAP Coc connection event and receive a L2CAP packet.

```bash
> L2capOnConnectionRequestReceived: aL2capHandle = 1, aMtu = 1000 
L2capOnSduReceived: aL2capHandle = 1, length = 400
```

### ATT/GATT Operations

### Node 1

On node 1, set it as a GATT server.

```bash
> ble gatt server register
service       : handle =  6, uuid = fffb 
characteristic: handle =  8, properties = 0x08, handleCccd =  0, uuid = 18ee2ef5263d4559959f4f9c429f9d11
characteristic: handle = 10, properties = 0x22, handleCccd = 11, uuid = 18ee2ef5263d4559959f4f9c429f9d12
Done
>
```

### Node 2

On node 2, start GATT operations.

```bash
> ble gatt client get service all
| startHandle |   endHandle  | uuid |
+-------------+--------------+------+
Done
|       1     |      5       | 1800 |
|       6     |  65535       | fffb |
>
> ble gatt client get service fffb
| startHandle |   endHandle  | uuid |
+-------------+--------------+------+
Done
|       6     |  65535       | fffb |
>
> ble gatt client get chars 6 65535
| handle |  properties |               uuid               |
+--------+-------------+----------------------------------+
Done
|      8 |    0x08     | 18ee2ef5263d4559959f4f9c429f9d11 |
|     10 |    0x22     | 18ee2ef5263d4559959f4f9c429f9d12 |
>
> ble gatt client get desc 8 65535
| handle |               uuid               |
+--------+----------------------------------+
Done
|      8 | 18ee2ef5263d4559959f4f9c429f9d11 |
|      9 | 2803 |
|     10 | 18ee2ef5263d4559959f4f9c429f9d12 |
|     11 | 2902 |
>
> ble gatt client write 8 aabbccddeeff
Done
> GattClientOnWriteResponse: handle = 8
>
> ble gatt client read 10
Done
> GattClientOnReadResponse: aabbccddeeff
>
```

### Node 1

On node 1, generate write and read events.

```bash
> GattServerOnWriteRequest: handle = 8, value = aabbccddeeff
> GattServerOnReadRequest: handle = 10
```

### Node 2

On node 2, subscribe a Client Characteristc Configuration handle.

```bash
> ble gatt client subscribe 11 1
Done
> GattClientOnSubscribeResponse: handle = 11
>
```

### Node 1

On node 1, generate a subscribe event and send an Inication packet to the GATT client.

```bash
> GattServerOnSubscribeRequest: handle = 11, subscribing = 1

> ble gatt server indicate  11 55667788
Done
> GattServerOnIndicationConfirmation: handle = 11
```

## Command List

* [help](#help)
* [enable](#enable)
* [bdaddr](#bdaddr)
* [adv](#adv-start)
* [scan](#scan-start-duration)
* [connect](#connect-bdaddr-type-bdaddr)
* [disconnect](#disconnect)
* [l2cap](#l2cap-register-connection-id-role-mtu-psm)
* [gatt](#gatt-server-register)

## Command Details

### help

```bash
>ble help
help
bdaddr
enable
disable
adv
scan
connect
disconnect
l2cap
gatt
Done
```

List the BLE CLI commands.

### enable

Enable the BLE stack.

```bash
> ble enable
Done
> BLE is enabled
```

### disable

Disable the BLE stack.

```bash
> ble enable
Done
```

### bdaddr

Show the Bluetooth device address.

```bash
> ble bdaddr
0 2b7900000001
Done
```

### adv start

Start BLE advertising.

```bash
> ble adv start
Done
```

### adv advdata \<data\>

Set advertising data.

* data: The advertising data.

```bash
> ble adv advdata 11AA22BB33CC
Done
```

### adv stop

Stop BLE advertising.

```bash
> ble adv stop
Done
```

### scan start \<duration\>

Start BLE scanning.

* duration: The duration of BLE scanning.

```bash
> ble scan start 2
| advType | addrType |   address    | rssi | AD or Scan Rsp Data |
+---------+----------+--------------+------+---------------------|
Done
> | ADV     |    0     | 2b7900000001 | -15  | 11aa22bb33cc
| SCAN_RSP|    0     | 2b7900000001 | -14  | 55dd66ee77ff
| ADV     |    1     | 2a16a5c62c44 | -47  | 1eff0600010f20020138b638887af0ba3cc6186eb2886928e3e2ae8e8f2a86
| ADV     |    1     | 401d026184db | -44  | 02010607ff4c0010020b00
| SCAN_RSP|    1     | 401d026184db | -44  | 
| ADV     |    0     | 08df1fe65e8b | -35  | 02011a0303befe13ff1003400c01538c85902510ffb8c111eb2a9d
| SCAN_RSP|    0     | 08df1fe65e8b | -35  | 020a0416094c452de4b88de683b3e590ace4bda0e8afb4e8af9d

```

### adv stop

Stop BLE scanning.

```bash
> ble scan stop
Done
```

### scan rspdata \<data\>

Set scan response data.

* data: The scan response data.

```bash
> ble scan rspdata 55dd66ee77ff
Done
```

### connect \<bdaddr-type\> \<bdaddr\>

Initiate a BLE connection.

* bdaddr-type : The type of the Bluetooth device address.
* bdaddr : The Bluetooth device address.

```bash
> ble connect 0 2b7900000001
Done
> GapOnConnected: connectionId = 1
>
```

### disconnect

Disconnect the BLE connection.

```bash
> ble disconnect
Done
> GapOnDisconnected: connectionId = 1
>
```

### l2cap register \<connection-id\> \<role\> \<mtu\> \<psm\>

Register parameters to initialize a L2CAP connection oriented channel.

* connection-id: BLE connection ID.
* role: L2CAP connection oriented channel role. 0 for Coc initiator or 1 for Coc acceptor.
* mtu: Maximum Transmission Unit (default: 1280).
* psm: LE Protocol/Service Multiplexer (default: 0x0080).

```bash
> ble l2cap register 1 0 1000 55
L2cap Handle: 1
Done
>
```

### l2cap deregister \<l2cap-handle\>

Deregister and deallocate a connection oriented channel registration instance.

* l2cap-handle: The handle of connection oriented channel.

```bash
> ble l2cap deregister 1
Done
>
```

### l2cap connect \<l2cap-handle\>

Initiate a connection for thr given L2CAP handle.

* l2cap-handle: The handle of connection oriented channel initiator.

```bash
> ble l2cap connect 1
Done
> L2capOnConnectionResponseReceived: aL2capHandle = 1, aMtu = 1000
>
```

### l2cap disconnect \<l2cap-handle\>

Disconnect the channel for the given L2CAP handle.

* l2cap-handle: The handle of connection oriented channel.

```bash
> ble l2cap disconnect 1
Done
> L2capOnDisconnected: aL2capHandle = 1
>
```

### l2cap send \<l2cap-handle\> \<payload-length\>

Send data on the L2CAP connection.

* l2cap-handle: The handle of connection oriented channel.
* payload-length: The length of the L2CAP payload.

```bash
> ble l2cap send 1 400
Done
> L2capOnSduSent: aL2capHandle = 1, error = 0
```

### gatt server register

Register default server GATT profile.

```bash
> ble gatt server register
service       : handle =  6, uuid = fffb 
characteristic: handle =  8, properties = 0x08, handleCccd =  0, uuid = 18ee2ef5263d4559959f4f9c429f9d11
characteristic: handle = 10, properties = 0x22, handleCccd = 11, uuid = 18ee2ef5263d4559959f4f9c429f9d12
Done
>
```

### gatt server indicate \<handle\>  \<data\>

Send an Indication to GATT client.

* handle: The CCC handle.
* data: The data.

```bash
> ble gatt server indicate 11 55667788 
Done
> GattServerOnIndicationConfirmation: handle = 11
>
```

### gatt client mtu exchange \<mtu\>

Exchange GATT MTU.

* mtu: GATT MTU.

```bash
> ble gatt client mtu exchange 131
Done
```

### gatt client mtu

Show GATT MTU.

```bash
> ble gatt client mtu
mtu: 131
Done
```

### gatt client get service all

Get all GATT server supported services.

```bash
> ble gatt client get service all
| startHandle |   endHandle  | uuid |
+-------------+--------------+------+
Done
|       1     |      5       | 1800 |
|       6     |  65535       | fffb |
>
```

### gatt client get service \<service-uuid\>

Get specified service from GATT server.

* service-uuid: The service UUID.

```bash
> ble gatt client get service fffb

| startHandle |   endHandle  | uuid |
+-------------+--------------+------+
Done
|       6     |  65535       | fffb |
>
```

### gatt client get chars \<start-handle\>  \<end-handle\>

Get all GATT Characteristics between start-handle and end-handle from GATT server.

* start-handle: The start handle of a service.
* end-handle: The end handle of a service.

```bash
> ble gatt client get chars 6 65535
| handle |  properties |               uuid               |
+--------+-------------+----------------------------------+
Done
|      8 |    0x08     | 18ee2ef5263d4559959f4f9c429f9d11 |
|     10 |    0x22     | 18ee2ef5263d4559959f4f9c429f9d12 |
>
```
### gatt client get desc \<start-handle\>  \<end-handle\>

Get all GATT Descriptors between start-handle and end-handle from GATT server.

* start-handle: The start handle.
* end-handle: The end handle.

```bash
> ble gatt client get desc 8 65535
| handle |               uuid               |
+--------+----------------------------------+
Done
|      8 | 18ee2ef5263d4559959f4f9c429f9d11 |
|      9 | 2803 |
|     10 | 18ee2ef5263d4559959f4f9c429f9d12 |
|     11 | 2902 |
>
```

### gatt client write \<handle\>  \<data\>

Write data to specified GATT handle.

* handle: The GATT handle.
* data: The data.

```bash
> ble gatt client write 8 aabbccddeeff
Done
> GattClientOnWriteResponse: handle = 8
>
```

### gatt client read \<handle\>

Read data from specified GATT handle.

* handle: The GATT handle.

```bash
> ble gatt client read 10
Done
> GattClientOnReadResponse: aabbccddeeff
>
```

### gatt client subscribe \<handle\> \<subscribing\>

Subscribe or unsubscribe a Client Characteristc Configuration handle.

* handle: The CCC handle.
* subscribing: 0 for unsubscribing. 1 for subscribing.

```bash
> ble gatt client subscribe 11 1
Done
> GattClientOnSubscribeResponse: handle = 11
>
```
