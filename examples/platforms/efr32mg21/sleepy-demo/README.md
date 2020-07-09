# EFR32MG21 Sleepy Demo Example

The EFR32 Sleepy applications demonstrates Sleepy End Device behavior using the EFR32's low power EM2 mode. The steps below will take you through the process of building and running the demo

For setting up the build environment refer to [examples/platforms/efr32mg21/README.md](../README.md).

## 1. Build

```bash
$ cd <path-to-openthread>
$ ./bootstrap
$ make -f examples/Makefile-efr32mg21 COMMISSIONER=1 JOINER=1 DHCP6_CLIENT=1 DHCP6_SERVER=1 BOARD=BRD4180A
```

Convert the resulting executables into S-Record format and append a s37 suffix.

```bash
$ cd output/efr32mg21/bin
$ arm-none-eabi-objcopy -O srec sleepy-demo-mtd sleepy-demo-mtd.s37
$ arm-none-eabi-objcopy -O srec sleepy-demo-ftd sleepy-demo-ftd.s37
```

In Silicon Labs Simplicity Studio flash one device with the sleepy-demo-mtd.s37 image and the other device with the sleepy-demo-ftd.s37 image.

For instructions on flashing firmware see [examples/platforms/efr32mg21/README.md](../README.md#flash-binaries)

## 2. Starting nodes

For demonstration purposes the network settings are hardcoded within the source files. The devices start Thread and form a network within a few seconds of powering on. In a real-life application the devices should implement and go through a commissioning process to create a network and add devices.

When the sleepy-demo-ftd device is started in the CLI the user shall see:

```
sleepy-demo-ftd started
sleepy-demo-ftd changed to leader
```

When the sleepy-demo-mtd device starts it joins the pre-configured Thread network before disabling Rx-On-Idle to become a Sleepy-End-Device.

Use the command "child table" in the FTD console and observe the R flag of the child is 0.

```
> child table
| ID  | RLOC16 | Timeout    | Age        | LQ In | C_VN |R|S|D|N| Extended MAC     |
+-----+--------+------------+------------+-------+------+-+-+-+-+------------------+
|   1 | 0x8401 |        240 |          3 |     3 |    3 |0|1|0|0| 8e8582dbd78c243c |

Done
```

## 3. MTD behavior on wake-up

MTD wakes up every 5 seconds and sends a multicast UDP message containing the string "mtd is awake". The FTD listens on the multicast address and will toggle LED 0 and display a message in the CLI showing "Message Received: mtd is awake".

## 4. Monitoring power consumption of the MTD

Open the Energy Profiler within Silicon Labs Simplicity Studio. Within the Quick Access menu select Start Energy Capture... and select the MTD device. When operating as a Sleepy End Device with no LEDs on the current should be under 20-80 microamps with occasional spikes during waking and polling the parent. With the LED on the MTD has a current consumption of approximately 1mA.

When operating as a Minimal End Device with the Rx on Idle observe that the current is in the order of 10ma.

With further configuration of GPIOs and peripherals it is possible to reduce the sleepy current consumption further.

## 5. Notes on sleeping, sleepy callback and interrupts

To allow the EFR32 to enter sleepy mode the application must register a callback with efr32SetSleepCallback. The return value of callback is used to indicate that the application has no further work to do and that it is safe to go into a low power mode. The callback is called with interrupts disabled so should do the minimum required to check if it can sleep.
