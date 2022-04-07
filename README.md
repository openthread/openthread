[![OpenThread][ot-logo]][ot-repo] [![Build][ot-gh-action-build-svg]][ot-gh-action-build] [![Simulation][ot-gh-action-simulation-svg]][ot-gh-action-simulation] [![Docker][ot-gh-action-docker-svg]][ot-gh-action-docker] [![Language grade: C/C++][ot-lgtm-svg]][ot-lgtm] [![Coverage Status][ot-codecov-svg]][ot-codecov]

---

# What is OpenThread?

OpenThread released by Google is... <a href="https://www.threadgroup.org/What-is-Thread/Thread-Benefits#certifiedproducts"> <img src="https://cdn.rawgit.com/openthread/openthread/ab4c4e1e/doc/images/certified.svg" alt="Thread Certified Component" width="150px" align="right"> </a>

**...an open-source implementation of the [Thread](https://www.threadgroup.org/What-is-Thread/Overview) networking protocol.** Google Nest has released OpenThread to make the technology used in Nest products more broadly available to developers to accelerate the development of products for the connected home.

**...OS and platform agnostic**, with a narrow platform abstraction layer and a small memory footprint, making it highly portable. It supports both system-on-chip (SoC) and network co-processor (NCP) designs.

**...a Thread Certified Component**, implementing all features defined in the [Thread 1.2 specification](https://www.threadgroup.org/support#specifications), including all Thread networking layers (IPv6, 6LoWPAN, IEEE 802.15.4 with MAC security, Mesh Link Establishment, Mesh Routing) and device roles, as well as [Border Router](https://github.com/openthread/ot-br-posix) support.

More information about Thread can be found at [threadgroup.org](http://threadgroup.org/). Thread is a registered trademark of the Thread Group, Inc.

[ot-repo]: https://github.com/openthread/openthread
[ot-logo]: https://github.com/openthread/openthread/raw/main/doc/images/openthread_logo.png
[ot-gh-action-build]: https://github.com/openthread/openthread/actions?query=workflow%3ABuild+branch%3Amain+event%3Apush
[ot-gh-action-build-svg]: https://github.com/openthread/openthread/workflows/Build/badge.svg?branch=main&event=push
[ot-gh-action-simulation]: https://github.com/openthread/openthread/actions?query=workflow%3ASimulation+branch%3Amain+event%3Apush
[ot-gh-action-simulation-svg]: https://github.com/openthread/openthread/workflows/Simulation/badge.svg?branch=main&event=push
[ot-gh-action-docker]: https://github.com/openthread/openthread/actions?query=workflow%3ADocker+branch%3Amain+event%3Apush
[ot-gh-action-docker-svg]: https://github.com/openthread/openthread/workflows/Docker/badge.svg?branch=main&event=push
[ot-lgtm]: https://lgtm.com/projects/g/openthread/openthread/context:cpp
[ot-lgtm-svg]: https://img.shields.io/lgtm/grade/cpp/g/openthread/openthread.svg?logo=lgtm&logoWidth=18
[ot-codecov]: https://codecov.io/gh/openthread/openthread
[ot-codecov-svg]: https://codecov.io/gh/openthread/openthread/branch/main/graph/badge.svg

# Who supports OpenThread?

<a href="https://www.arm.com/"><img src="https://github.com/openthread/openthread/raw/main/doc/images/ot-contrib-arm.png" alt="ARM" width="200px"></a><a href="https://www.cascoda.com/"><img src="https://github.com/openthread/openthread/raw/main/doc/images/ot-contrib-cascoda.png" alt="Cascoda" width="200px"></a><a href="https://www.espressif.com/"><img src="https://github.com/openthread/openthread/raw/main/doc/images/ot-contrib-espressif-github.png" alt="Espressif" width="200px"></a><a href="https://www.google.com/"><img src="https://github.com/openthread/openthread/raw/main/doc/images/ot-contrib-google.png" alt="Google" width="200px"></a><a href="https://www.infineon.com/"><img src="https://github.com/openthread/openthread/raw/main/doc/images/ot-contrib-infineon.png" alt="Infineon" width="200px"></a><a href="http://www.nordicsemi.com/"><img src="https://github.com/openthread/openthread/raw/main/doc/images/ot-contrib-nordic.png" alt="Nordic" width="200px"></a><a href="http://www.nxp.com/"><img src="https://github.com/openthread/openthread/raw/main/doc/images/ot-contrib-nxp.png" alt="NXP" width="200px"></a><a href="http://www.qorvo.com/"><img src="https://github.com/openthread/openthread/raw/main/doc/images/ot-contrib-qorvo.png" alt="Qorvo" width="200px"></a><a href="https://www.qualcomm.com/"><img src="https://github.com/openthread/openthread/raw/main/doc/images/ot-contrib-qc.png" alt="Qualcomm" width="200px"></a><a href="https://www.samsung.com/"><img src="https://github.com/openthread/openthread/raw/main/doc/images/ot-contrib-samsung.png" alt="Samsung" width="200px"></a><a href="https://www.silabs.com/"><img src="https://github.com/openthread/openthread/raw/main/doc/images/ot-contrib-silabs.png" alt="Silicon Labs" width="200px"></a><a href="https://www.st.com/"><img src="https://github.com/openthread/openthread/raw/main/doc/images/ot-contrib-stm.png" alt="STMicroelectronics" width="200px"></a><a href="https://www.synopsys.com/"><img src="https://github.com/openthread/openthread/raw/main/doc/images/ot-contrib-synopsys.png" alt="Synopsys" width="200px"></a><a href="https://www.telink-semi.com/"><img src="https://github.com/openthread/openthread/raw/main/doc/images/ot-contrib-telink-github.png" alt="Telink Semiconductor" width="200px"></a><a href="https://www.ti.com/"><img src="https://github.com/openthread/openthread/raw/main/doc/images/ot-contrib-ti.png" alt="Texas Instruments" width="200px"></a><a href="https://www.zephyrproject.org/"><img src="https://github.com/openthread/openthread/raw/main/doc/images/ot-contrib-zephyr.png" alt="Zephyr Project" width="200px"></a>

# Getting started

All end-user documentation and guides are located at [openthread.io](https://openthread.io). If you're looking to do things like...

- Learn more about OpenThread features and enhancements
- Use OpenThread in your products
- Learn how to build and configure a Thread network
- Port OpenThread to a new platform
- Build an application on top of OpenThread
- Certify a product using OpenThread

...then [openthread.io](https://openthread.io) is the place for you.

> Note: For users in China, end-user documentation is available at [openthread.google.cn](https://openthread.google.cn).

If you're interested in contributing to OpenThread, read on.

# Contributing

We would love for you to contribute to OpenThread and help make it even better than it is today! See our [Contributing Guidelines](https://github.com/openthread/openthread/blob/main/CONTRIBUTING.md) for more information.

Contributors are required to abide by our [Code of Conduct](https://github.com/openthread/openthread/blob/main/CODE_OF_CONDUCT.md) and [Coding Conventions and Style Guide](https://github.com/openthread/openthread/blob/main/STYLE_GUIDE.md).

# Versioning

OpenThread follows the [Semantic Versioning guidelines](http://semver.org/) for release cycle transparency and to maintain backwards compatibility. OpenThread's versioning is independent of the Thread protocol specification version but will clearly indicate which version of the specification it currently supports.

# License

OpenThread is released under the [BSD 3-Clause license](https://github.com/openthread/openthread/blob/main/LICENSE). See the [`LICENSE`](https://github.com/openthread/openthread/blob/main/LICENSE) file for more information.

Please only use the OpenThread name and marks when accurately referencing this software distribution. Do not use the marks in a way that suggests you are endorsed by or otherwise affiliated with Nest, Google, or The Thread Group.

# Need help?

OpenThread support is available on GitHub:

- Bugs and feature requests — [submit to the Issue Tracker](https://github.com/openthread/openthread/issues)
- Community Discussion - [ask questions, share ideas, and engage with other community members](https://github.com/openthread/openthread/discussions)
