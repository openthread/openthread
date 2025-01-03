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

To build Nexus test cases, the `build.sh` script can be used:

```bash
./tests/nexus/build.sh
```

Afterwards, each test can be run directly:

```bash
./tests/nexus/nexus_form_join
```
