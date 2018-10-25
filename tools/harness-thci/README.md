THCI & Test Environment Setup
=============================

THCI (Thread Host Controller Interface) is an implementation of the Python abstract class template "IThci",
which is used by the Thread Test Harness Software to control OpenThread-based reference devices according to each test
scenario.

Currently, there are two THCI implementations for OpenThread:

* OpenThread CLI — Based on the CC2538 example platform, which is included in the current Thread Test Harness Software
  release.
* OpenThread `wpanctl` — Based on `wpantund` running on a Linux host (for example, a Raspberry Pi 3B) working with a Network
  Co-Processor (NCP) (for example, a Nordic Semiconductor nRF52840) running an OpenThread NCP image.

Platform developers should modify the THCI implementation directly to match their platform (for example, serial baud rate).
Alternatively, platform developers may follow the instructions below to add a new THCI implementation to the Test Harness.

## OpenThread Environment Setup ##

1. Copy "OpenThread.png" to `C:\GRL\Thread1.1\Web\images`.

2. Copy "deviceInputFields.xml" to `C:\GRL\Thread1.1\Web\data`.

3. Copy "OpenThread.py" to `C:GRL\Thread1.1\Thread_Harness\THCI`.

4. Connect the DUT (Device Under Test), a sniffer, and CC2538DK (or other hardware running OpenThread, as the reference device)
   to the Host PC.

5. Launch the Thread Test Harness Software, modify the default configuration if needed, and select **Start**.

6. Drag the "OpenThread: TI CC2538DK" reference device to the **Test Bed** section with the desired number.

7. Select one or more test cases to execute.


## OpenThread WpanCtl Environment Setup ##

1. Copy "OpenThread_WpanCtl.png" to `C:\GRL\Thread1.1\Web\images`.

2. Copy "deviceInputFields.xml" to `C:\GRL\Thread1.1\Web\data`.

3. Copy "OpenThread_WpanCtl.py" to `C:\GRL\Thread1.1\Thread_Harness\THCI`.

4. Connect the NCP board (nRF52840) to the Raspberry Pi's USB port and verify that the `wpanctl` command works.

5. Connect the Raspberry Pi's GPIOs (for Raspberry Pi 3B, link Pin8 as TXD, Pin10 as RXD, and Pin14 as GND) with
   a UART2USB adapter.

6. Connect the DUT (with Step 5's adapter), sniffer, and other golden devices (as reference devices) to the Host PC.

7. Get the DUT serial port hardware identifier and add a new platform group named OpenThread_WpanCtl in
   `C:\GRL\Thread1.1\Config\Configuration.ini`. See https://openthread.io/certification/automation-setup#acting_as_a_new_reference_platform
   for more information.

8. Launch the Thread Test Harness Software, modify the default configuration if needed, and select **Start**.

9. Drag the "OpenThread_WpanCtl: Wpantund + NCP" reference device to the **Test Bed** section with the desired number.

10. Select one or more test cases to execute.


The above is for the serial connection mode between the DUT and the Host PC. The ssh connection mode is
also supported for OpenThread WpanCtl, but it can not be auto-discovered and only can be used via ‘Drag and Add’ way
in the ‘Configure Test Bed’ Page of Test Harness. The configuration is as following:

1. Make Raspberry Pi and Host PC in the same LAN.

2. Login Raspberry Pi, sudo raspi-config, then -> interfacing options -> enable ssh.

3. Install python paramiko package to `C:\GRL\Pyton2.7`

4. Copy "OpenThread_WpanCtl.png" to `C:\GRL\Thread1.1\Web\images`.

5. Copy "deviceInputFields.xml" to `C:\GRL\Thread1.1\Web\data`.

6. Copy "OpenThread_WpanCtl.py" to `C:\GRL\Thread1.1\Thread_Harness\THCI`.

7. Launch the Thread Test Harness Software, modify the default configuration if needed, and select **Start**.

8. Drag the "OpenThread_WpanCtl: Wpantund + NCP (SSH)" reference device to the **Test Bed** section with the desired number.
   Then fill in the Raspberry Pi IPv4 address and port and click `connect` icon button to get the NCP version.

9. Select one or more test cases to execute.
