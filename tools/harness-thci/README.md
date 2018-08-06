THCI & Test Environment Setup
=============================

THCI (Thread Host Controller Interface) is an implementation of the Python abstract class template "IThci",
which is used by Thread Test Harness Software to control OpenThread-based reference devices according to each test
scenario.

Currently, there are two THCI implementations for Open Thread. One is based on the CC2538 example platform and is 
included in the current Thread Test Harness Software release. The other is for OTBR (Open Thread Border Router) which 
is based on Raspberry Pi connected with NCP.   

Platform developers should modify the THCI implementation directly to match their platform (e.g. serial baud rate).
Alternatively, platform developers may follow the instructions below to add a new THCI implementation to the Test
Harness.

## Open Thread Environment Setup ##

1. Copy the "OpenThread.png" to /GRL/Thread1.1/MiniWeb/htdocs/images folder.

2. Copy the "deviceInputFields.xml" to /GRL/Thread1.1/MiniWeb/htdocs/data folder.

3. Copy the "OpenThread.py" to /GRL/Thread1.1/Thread_Harness/THCI folder.

4. Connect DUT(Device Under Test), sniffer and CC2538DK(or hardware running OpenThread) as reference device to Host PC.

5. Launch Thread Test Harness Software and modify the default configuration if needed then click "Start".

6. Drag OpenThread: TI CC2538DK reference device to Test Bed list with desired number.

7. Select one or multiple test cases to execute.


## OTBR Environment Setup ##

1. Copy the "OTBR.png" to /GRL/Thread1.1/MiniWeb/htdocs/images folder.

2. Copy the "deviceInputFields.xml" to /GRL/Thread1.1/MiniWeb/htdocs/data folder.

3. Copy the "OTBR.py" to /GRL/Thread1.1/Thread_Harness/THCI folder.

4. Connect NCP to Raspberry Pi's usb port and make sure wpanctl work

5. Connect the Raspberry Pi's UART (BCM14 as TXD, BCM15 as RXD) with a UART2USB adapter 

6. Connect DUT(with Step 5's adapter), sniffer and other golden devices as reference device to Host PC.

7. Launch Thread Test Harness Software and modify the default configuration if needed then click "Start".

8. Drag OTBR: Raspberry Pi + NCP reference device to Test Bed list with desired number.

9. Select one or multiple test cases to execute.
