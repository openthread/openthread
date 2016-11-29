OpenThread THCI
===============

OpenThread THCI(Thread Host Controller Interface) is implemented according
to the python abstract class template-"IThci" that is provided by GRL. It's
intented to be called by Thread Test Harness Software to control the reference
device behavior.

## Environment Setup ##

1. Copy the "OpenThread.py" to /GRL/Thread1.1/Thread_Harness/THCI folder.

2. Connect DUT(Device Under Test), sniffer and CC2538DK(or hardware running OpenThread) as reference device to Host PC.

3. Launch Thread Test Harness Software and modify the default configuration if needed then click "Start".

4. Drag OpenThread: TI CC2538DK reference device to Test Bed list with desired number.

5. Select one or multiple test cases to execute.

