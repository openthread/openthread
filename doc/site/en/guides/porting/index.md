# Porting OpenThread to New Hardware Platforms

Porting the OpenThread stack to a new hardware platform consists of five steps:

1.  [Set up the build environment](https://github.com/openthread/openthread/blob/master/doc/site/en/guides/porting/set-up-the-build-environment.md)
1.  [Implement Platform Abstraction Layer APIs](https://github.com/openthread/openthread/blob/master/doc/site/en/guides/porting/implement-platform-abstraction-layer-apis.md)
1.  [Implement advanced features (Hardware Abstraction Layer)](https://github.com/openthread/openthread/blob/master/doc/site/en/guides/porting/implement-advanced-features.md)
1.  [Validate the port](https://github.com/openthread/openthread/blob/master/doc/site/en/guides/porting/validate-the-port.md)
1.  [Certification and README](https://github.com/openthread/openthread/blob/master/doc/site/en/guides/porting/certification-and-readme.md)

## Hardware platform requirements

OpenThread requires the following platform services:

-   [IEEE 802.15.4-2006](https://standards.ieee.org/findstds/standard/802.15.4-2006.html)
    2.4 GHz radio
    -   Send and receive IEEE 802.15.4 frames
    -   Generate IEEE 802.15.4 Acknowledgment frames
    -   Provide Received Signal Strength Indicator (RSSI) measurements on
        received frames
-   A millisecond-resolution free-running timer with alarm
-   Non-volatile storage for storing network configuration settings
-   A true random number generator (TRNG)

## Example builds

Several example builds are provided in the OpenThread repository. For more
information, see [Platforms](https://openthread.io/platforms).

For a complete end-to-end example of how to port OpenThread from scratch, see
the [Add support for EFR32](https://github.com/openthread/openthread/pull/1592)
pull request.

