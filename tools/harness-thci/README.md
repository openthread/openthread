THCI & Test Environment Setup
=============================

THCI (Thread Host Controller Interface) is an implementation of the Python abstract class template "IThci",
which is used by Thread Test Harness Software to control OpenThread-based reference devices according to each test
scenario.

Currently, there are two THCI implementations for OpenThread. One is for OpenThread CLI which is based on the CC2538 
example platform and is included in the current Thread Test Harness Software release. The other is for OpenThread wpanctl
which is based on wpantund running on a linux host (for example: Raspberry Pi 3B) working with a NCP (Network Co-Processor) 
board (for example: Nordic Semiconductor nRF52840) running a OpenThread NCP image.

Platform developers should modify the THCI implementation directly to match their platform (for example: serial baud rate).
Alternatively, platform developers may follow the instructions below to add a new THCI implementation to the Test
Harness.

## OpenThread Environment Setup ##

1. Copy "OpenThread.png" to \GRL\Thread1.1\Web\images folder.

2. Copy "deviceInputFields.xml" to \GRL\Thread1.1\Web\data folder.

3. Copy "OpenThread.py" to \GRL\Thread1.1\Thread_Harness\THCI folder.

4. Connect DUT (Device Under Test), sniffer and CC2538DK (or hardware running OpenThread) as reference device to Host PC.

5. Launch Thread Test Harness Software and modify the default configuration if needed then click "Start".

6. Drag OpenThread: TI CC2538DK reference device to Test Bed list with desired number.

7. Select one or more test cases to execute.


## OpenThread WpanCtl Environment Setup ##

1. Copy "OpenThread.png" to \GRL\Thread1.1\Web\images folder.

2. Copy "deviceInputFields.xml" to \GRL\Thread1.1\Web\data folder.

3. Copy "OpenThread_WpanCtl.py" to \GRL\Thread1.1\Thread_Harness\THCI folder.

4. Connect the NCP board (nRF52840) to Raspberry Pi's USB port and verify that wpanctl command works.

5. Connect the Raspberry Pi's GPIOs (for Raspberry Pi 3B, link Pin8 as TXD, Pin10 as RXD and Pin14 as GND) with
   a UART2USB adapter.

6. Connect DUT (with Step 5's adapter), sniffer and other golden devices as reference device to Host PC.

7. Get the DUT serial port hardware identifier and add a new platform group named OpenThread_WpanCtl in
   \GRL\Thread1.1\Config\Configuration.ini. See https://openthread.io/certification/automation-setup#acting_as_a_new_reference_platform.

8. Launch Thread Test Harness Software and modify the default configuration if needed then click "Start".

9. Drag OpenThread_WpanCtl: Wpantund + NCP reference device to Test Bed list with desired number.

10. Select one or more test cases to execute.
