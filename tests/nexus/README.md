# Nexus test framework

Nexus is a test framework for OpenThread testing.

### Design Goals

- **Faster and more scalable network simulation**: Enable faster and more efficient simulations of OpenThread networks involving a large number of nodes over extended durations.
- **Enhanced control**: Achieve greater control and scalability over simulated tests.

### Features

- Includes the Nexus platform implementation that emulates platform behavior, allowing multiple nodes running the OpenThread core stack to be simulated and interact with each other within the same process.
- Unlike the simulation platform (under `examples/platforms/simulation`), where nodes run in separate processes and interact via POSIX sockets, Nexus nodes are simulated within a single process.
- Nexus tests can interact directly with the C++ or C OT core APIs, providing more control than the simulation platform's CLI-based interactions.
- The flow of time in Nexus tests is directly controlled by the test itself, allowing for quick time interval advancement.

### How to build and run tests

To build Nexus test cases, the `tests/nexus/build.sh` script can be used:

```bash
mkdir nexus_test
top_builddir=nexus_test ./tests/nexus/build.sh
```

By default, the script builds for IEEE 802.15.4. To build for TREL tests, use the `trel` argument:

```bash
top_builddir=nexus_test ./tests/nexus/build.sh trel
```

#### Automated testing and packet verification

The `tests/nexus/run_nexus_tests.sh` script automates the process of running tests and performing packet verification using corresponding Python scripts.

To run all default tests:

```bash
top_builddir=nexus_test ./tests/nexus/run_nexus_tests.sh
```

To run a specific test:

```bash
top_builddir=nexus_test ./tests/nexus/run_nexus_tests.sh 5_1_1
```

The script runs the Nexus C++ test (which generates a JSON file and optionally a PCAP file) and then executes the Python verification script (e.g., `verify_5_1_1.py`) if it exists. Artifacts for each test are preserved in a temporary directory if the test fails.

#### Manual execution

Each test can be run directly from the build directory. C++ tests typically take a topology name (if applicable) and a JSON output filename as arguments.

```bash
./nexus_test/tests/nexus/nexus_6_1_1 A test_6_1_1.json
```

The verification script can then be run manually:

```bash
python3 ./tests/nexus/verify_6_1_1.py test_6_1_1.json
```
