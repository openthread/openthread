THCI & Test Environment Setup
=============================

THCI (Thread Host Controller Interface) is an implementation of the Python abstract class template "IThci",
which is used by Thread Test Harness Software to control OpenThread-based reference devices according to each test
scenario.

Currently, there are two THCI implementations for OpenThread. One is for OpenThread CLI which is based on the CC2538 
example platform and is included in the current Thread Test Harness Software release. The other is for OpenThread WpanCtl
which is based on wpantund running on a linux host (For example: Raspberry Pi 3B) working with a NCP (Network Co-Processor) 
board (For example: Nordic Semiconductor nRF52840) running a OpenThread NCP image.

Platform developers should modify the THCI implementation directly to match their platform (e.g. serial baud rate).
Alternatively, platform developers may follow the instructions below to add a new THCI implementation to the Test
Harness.

## OpenThread Environment Setup ##

1. Copy the "OpenThread.png" to \GRL\Thread1.1\Web\images folder.

2. Copy the "deviceInputFields.xml" to \GRL\Thread1.1\Web\data folder.

3. Copy the "OpenThread.py" to \GRL\Thread1.1\Thread_Harness\THCI folder.

4. Connect DUT(Device Under Test), sniffer and CC2538DK(or hardware running OpenThread) as reference device to Host PC.

5. Launch Thread Test Harness Software and modify the default configuration if needed then click "Start".

6. Drag OpenThread: TI CC2538DK reference device to Test Bed list with desired number.

7. Select one or multiple test cases to execute.


## OpenThread WpanCtl Environment Setup ##

1. Copy the "OpenThread_WpanCtl.png" to \GRL\Thread1.1\Web\images folder.

2. Copy the "deviceInputFields.xml" to \GRL\Thread1.1\Web\data folder.

3. Copy the "OpenThread_WpanCtl.py" to \GRL\Thread1.1\Thread_Harness\THCI folder.

4. Connect the NCP board (nRF52840) to Raspberry Pi's usb port and make sure wpanctl command work.

5. Connect the Raspberry Pi's GPIOs (For Raspberry Pi 3B, link GPIO14 as TXD, GPIO15 as RXD and Pin14 as GND) with
   a UART2USB adapter.

6. Connect DUT(with Step 5's adapter), sniffer and other golden devices as reference device to Host PC.

7. Get the DUT serial port hardware identifier and add a new platform group named OpenThread_WpanCtl in
   \GRL\Thread1.1\Config\Configuration.ini referring to https://openthread.io/certification/automation-setup#acting_as_a_new_reference_platform.

8. Launch Thread Test Harness Software and modify the default configuration if needed then click "Start".

9. Drag OpenThread_WpanCtl: Wpantund + NCP reference device to Test Bed list with desired number.

10. Select one or multiple test cases to execute.
