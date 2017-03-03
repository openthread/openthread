OpenThread THCI
===============

OpenThread THCI (Thread Host Controller Interface) is an implementation of the Python abstract class template "IThci",
which is used by Thread Test Harness Software to control OpenThread-based reference devices according to each test
scenario.

The current implementation is based on the CC2538 example platform and is included in the current Thread Test Harness
Software release.

Platform developers should modify the THCI implementation directly to match their platform (e.g. serial baud rate).
Alternatively, platform developers may follow the instructions below to add a new THCI implementation to the Test
Harness. Adding a new THCI implementation allows the existing CC2538-based THCI implementation to coexist with the
new THCI implementation.

## Environment Setup ##

1. Copy the "OpenThread.png" to /GRL/Thread1.1/MiniWeb/htdocs/images folder.

2. Copy the "deviceInputFields.xml" to /GRL/Thread1.1/MiniWeb/htdocs/data folder.

3. Copy the "OpenThread.py" to /GRL/Thread1.1/Thread_Harness/THCI folder.

4. Connect DUT(Device Under Test), sniffer and CC2538DK(or hardware running OpenThread) as reference device to Host PC.

5. Launch Thread Test Harness Software and modify the default configuration if needed then click "Start".

6. Drag OpenThread: TI CC2538DK reference device to Test Bed list with desired number.

7. Select one or multiple test cases to execute.
