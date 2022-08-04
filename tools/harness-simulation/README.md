# Test Harness on Simulation Environment Setup

THCI (Thread Host Controller Interface) is an implementation of the Python abstract class template `IThci`, which is used by the Thread Test Harness Software to control OpenThread-based reference devices according to each test scenario.

SI (Sniffer Interface) is an implementation of the sniffer abstract class template `ISniffer`, which is used by the Thread Test Harness Software to sniff all packets sent by devices.

Both OpenThread simulation and sniffer simulation are required to run on a POSIX environment. However, Harness has to be run on Windows, which is a non-POSIX environment. So both two systems are needed, and their setup procedures in detail are listed in the following sections. Either two machines or one machine running two (sub)systems (for example, VM, WSL) is feasible.

Platform developers should modify the THCI implementation and/or the SI implementation directly to match their platform (for example, the path of the OpenThread repository).

## POSIX Environment Setup

1. Build OpenThread to generate standalone OpenThread simulation `ot-cli-ftd`. For example, run the following command in the top directory of OpenThread.

   ```bash
   $ script/cmake-build simulation
   ```

   Then `ot-cli-ftd` is built in the directory `build/simulation/examples/apps/cli/`.

2. Run the installation script.

   ```bash
   $ tools/harness-simulation/posix/install.sh
   ```

## Test Harness Environment Setup

1. Double click the file `harness\install.bat` on the machine which installed Harness.

2. Check the configuration file `C:\GRL\Thread1.2\Thread_Harness\simulation\config.py`

   - Edit the value of `REMOTE_OT_PATH` to the absolute path where the top directory of the OpenThread repository is located.

3. Add the additional simulation device information in `harness\Web\data\deviceInputFields.xml` to `C:\GRL\Thread1.2\Web\data\deviceInputFields.xml`.

## Run Test Harness on Simulation

1. On POSIX machine, change directory to the top of OpenThread repository, and run the following commands.

   ```bash
   $ cd tools/harness-simulation/posix
   $ python harness_dev_discovery.py \
         --interface=eth0            \
         --ot1.1=24                  \
         --sniffer=2
   ```

   It starts 24 OT FTD simulations and 2 sniffer simulations and can be discovered on eth0.

   The arguments can be adjusted according to the requirement of test cases.

2. Run Test Harness. The information field of the device is encoded as `<node_id>@<ip_addr>`. Choose the proper device as the DUT accordingly.

3. Select one or more test cases to start the test.
