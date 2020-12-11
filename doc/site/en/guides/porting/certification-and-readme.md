# Certification and README

## Thread Certification

To achieve Thread Certification, the port must be tested against the official
[Thread Harness](http://graniteriverlabs.com/thread/) and pass all scenarios
listed in the Thread Certification Test Plan. 

For more information, see
[Certification](https://openthread.io/certification). 

## README

A detailed README is necessary to demonstrate how to build and run OpenThread on
a new hardware platform.

At a minimum, the README should include:

-   Information about the hardware platform
-   Links to the required toolchain
-   How to configure platform-specific vendor software
-   How to build and flash binaries onto the platform
-   Versions of libraries and toolchains used for validation of the port

See the
[EFR32MG12 README](https://github.com/openthread/openthread/blob/master/examples/platforms/efr32/efr32mg12/README.md)
for an example.
