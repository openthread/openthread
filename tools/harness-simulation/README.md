# Test Harness on Simulation Environment Setup

THCI (Thread Host Controller Interface) is an implementation of the Python abstract class template `IThci`, which is used by the Thread Test Harness Software to control OpenThread-based reference devices according to each test scenario.

SI (Sniffer Interface) is an implementation of the sniffer abstract class template `ISniffer`, which is used by the Thread Test Harness Software to sniff all packets sent by devices.

Both OpenThread simulation and sniffer simulation are required to run on a POSIX environment. However, Harness has to be run on Windows, which is a non-POSIX environment. So two systems are needed, and their setup procedures in detail are listed in the following sections. Either two machines or one machine running two (sub)systems (for example, VM, WSL) is feasible.

Platform developers should modify the THCI implementation and/or the SI implementation directly to match their platform (for example, the path of the OpenThread repository).

## POSIX Environment Setup

1. Open the JSON format configuration file `tools/harness-simulation/posix/simulation.conf`:

   - Edit the value of `ot_path` to the absolute path where the top directory of the OpenThread repository is located. For example, change the value of `ot_path` to `/home/<username>/repo/openthread`.
   - As to each entry in `ot_build.ot`,
     - Edit the value of `number` to the number of the OT FTD simulations with the corresponding version.
   - As to each entry in `ot_build.otbr`,
     - Edit the value of `number` to the number of the OTBR simulations with the corresponding version.
   - The numbers above can be adjusted according to the requirement of test cases.
   - Edit the value of `ssh.username` to the username expected to connect to on the remote POSIX environment.
   - Edit the value of `ssh.password` to the password corresponding to the username above.

   Note that it may be time-consuming to build all versions of `ot-cli-ftd`s and OTBR Docker images especially on devices such as Raspberry Pis.

2. Run the installation script.

   ```bash
   $ tools/harness-simulation/posix/install.sh
   ```

## Test Harness Environment Setup

1. Copy the directory `tools/harness-simulation` from the POSIX machine to the Windows machine, and then switch to the directory.

2. Double click the file `harness\install.bat` on Windows.

3. Add the additional simulation device information in `harness\Web\data\deviceInputFields.xml` to `C:\GRL\Thread1.2\Web\data\deviceInputFields.xml`.

## Run Test Harness on Simulation

1. On the POSIX machine, change directory to the top of OpenThread repository, and run the following commands.

   ```bash
   $ cd tools/harness-simulation/posix
   $ ./launch_testbed.py -c simulation.conf
   ```

   This example starts several OT FTD simulations, OTBR simulations, and sniffer simulations and can be discovered on `eth0`. The number of each type of simulations is specified in the configuration file `simulation.conf`.

2. Run Test Harness. The information field of the device is encoded as `<tag>_<node_id>@<ip_addr>`. Choose the proper device as the DUT accordingly.

3. Select one or more test cases to start the test.
