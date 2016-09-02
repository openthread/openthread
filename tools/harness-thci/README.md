OpenThread THCI
===============

OpenThread THCI(Thread Host Controller Interface) is implemented according
to the python abstract class template-"IThci" that is provided by GRL. It's
intented to be called by Thread Test Harness Software to control the reference
unit behavior.

## Restriction ##

Currently Test Harness Software only supports three kinds of reference unit
officially: ARM, NXP(Freescale) and SiLabs. We make OpenThread THCI be able
to interact with Test Harness Software with the aid of the official THCI class
name of our partner ARM. And we are using TI CC2538DK as a target hardware platform
for reference unit.

OpenThread THCI works along with Thread Test Harness Software on Windows OS.

## Environment Setup ##

1. install or update essential modules `pexpect` and `pexpect_serial` with pip command.
   current `pexpect_serial` module version is 0.4.4; `pexpect` module version is 4.1.0

   * pip install pexpect

   * pip install pexpect_serial

   If there is already `pexpect` and `pexpect_serial` installed, just need to upgrade them.

   * pip install pexpect --upgrade

   * pip install pexpect_serial --upgrade

   If there is no 'pip' command installed before, please follow the below instructions to install 'pip' on
   Windows OS first.

   1). Download the pip python source code from "https://pypi.python.org/pypi/pip#downloads"

   2). Unzip the source package and go to the pip directory then install.

       * python setup.py install

   3). Add "pip" executable file path in the Environment Variables.

       * add "C\:Python27\Scripts" to environment variable "Path".

2. Copy the new installed or upgraded `pexpect` and `pexpect_serial` packages to Harness' Python27 directory.

   * copy C:\Python27\Lib\site-packages\pexpect C:\GRL\Thread1.1\Python27\Lib\site-packages\

   * copy C:\Python27\Lib\site-packages\pexpect-4.1.0.dist-info C:\GRL\Thread1.1\Python27\Lib\site-packages\

   * copy C:\Python27\Lib\site-packages\pexpect_serial C:\GRL\Thread1.1\Python27\Lib\site-packages\

   * copy C:\Python27\Lib\site-packages\pexpect_serial-0.4.4-py2.7.egg-info C:\GRL\Thread1.1\Python27\Lib\site-packages\

3. Replace the origin "ARM.py" in Harness THCI directory with OpenThread THCI python script in /tools/harness-thci
   (the same name "ARM.py").

4. Connect sniffer and CC2538DK as reference unit to Host PC.

5. Launch Thread Test Harness Software and modify the default configuration if needed then click "Start".

6. Drag ARM:NXP FRDM-K64F with FireFly 6LoWPAN SHIELD reference unit to Test Bed list with desired number.

7. Select one or multiple test cases to execute.

