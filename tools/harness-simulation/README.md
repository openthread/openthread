# Test Harness on Simulation Environment Setup

THCI (Thread Host Controller Interface) is an implementation of the Python abstract class template `IThci`, which is used by the Thread Test Harness Software to control OpenThread-based reference devices according to each test scenario.

SI (Sniffer Interface) is an implementation of the sniffer abstract class template `ISniffer`, which is used by the Thread Test Harness Software to sniff all packets sent by devices.

Both OpenThread simulation and sniffer simulation are required to run on a POSIX environment. However, Harness has to be run on Windows, which is a non-POSIX environment. So both two systems are needed, and their setup procedures in detail are listed in the following sections. Either two machines or one machine running two (sub)systems (for example, VM, WSL) is feasible.

Platform developers should modify the THCI implementation and/or the SI implementation directly to match their platform (for example, the path of the OpenThread repository).

## POSIX Environment Setup

1. Build OpenThread to generate standalone OpenThread simulation `ot-cli-ftd`. Taking running OpenThread 1.2 test cases as an example, run the following command in the top directory of OpenThread.

   ```bash
   $ CFLAGS='-DOPENTHREAD_CONFIG_IP6_MAX_EXT_MCAST_ADDRS=8' \
     CXXFLAGS='-DOPENTHREAD_CONFIG_IP6_MAX_EXT_MCAST_ADDRS=8' \
     script/cmake-build simulation \
         -DOT_THREAD_VERSION=1.2 \
         -DOT_DUA=ON \
         -DOT_MLR=ON \
         -DOT_COMMISSIONER=ON \
         -DOT_CSL_RECEIVER=ON
   ```

   Then `ot-cli-ftd` is built in the directory `build/simulation/examples/apps/cli/`.

2. Check the configuration file `config.py`

   - Edit the value of `OT_PATH` to the absolute path where the top directory of the OpenThread repository is located.

3. Run the `build_docker_image.sh` with the environment variable `OT_PATH` set properly. For example run the following command.

   ```bash
   $ OT_PATH=~/repo/openthread ./build_docker_image.sh
   ```

4. Run the installation script.

   ```bash
   $ tools/harness-simulation/posix/install.sh
   ```

## Test Harness Environment Setup

1. Double click the file `harness\install.bat` on the machine which installed Harness.

2. Check the configuration file `C:\GRL\Thread1.2\Thread_Harness\simulation\config.py`

   - Edit the value of `REMOTE_USERNAME` to the username expected to connect to on the remote POSIX environment.
   - Edit the value of `REMOTE_PASSWORD` to the password corresponding to the username above.
   - Edit the value of `REMOTE_OT_PATH` to the absolute path where the top directory of the OpenThread repository is located.

3. Add the additional simulation device information in `harness\Web\data\deviceInputFields.xml` to `C:\GRL\Thread1.2\Web\data\deviceInputFields.xml`.

## Run Test Harness on Simulation

1. On POSIX machine, change directory to the top of OpenThread repository, and run the following commands.

   ```bash
   $ cd tools/harness-simulation/posix
   $ python3 launch_testbed.py \
         --interface=eth0      \
         --ot=6                \
         --otbr=4              \
         --sniffer=2
   ```

   It starts 6 OT FTD simulations, 4 OTBR simulations and 2 sniffer simulations and can be discovered on eth0.

   The arguments can be adjusted according to the requirement of test cases.

2. Run Test Harness. The information field of the device is encoded as `<node_id>@<ip_addr>` for FTD and `otbr_<node_id>@<ip_addr>` for BR. Choose the proper device as the DUT accordingly.

3. Select one or more test cases to start the test.
