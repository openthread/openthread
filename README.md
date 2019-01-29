[![OpenThread][ot-logo]][ot-repo]
[![Build Status][ot-travis-svg]][ot-travis]
[![Build Status][ot-appveyor-svg]][ot-appveyor]
[![Coverage Status][ot-codecov-svg]][ot-codecov]

---

# What is OpenThread?

OpenThread is...
<a href="http://threadgroup.org/technology/ourtechnology#certifiedproducts">
<img src="https://cdn.rawgit.com/openthread/openthread/ab4c4e1e/doc/images/certified.svg" alt="Thread Certified Component" width="150px" align="right">
</a>

**...an open-source implementation of the [Thread](http://threadgroup.org/technology/ourtechnology) networking protocol.** Nest has released OpenThread to make the technology used in Nest products more broadly available to developers to accelerate the development of products for the connected home.

**...OS and platform agnostic**, with a narrow platform abstraction layer and a small memory footprint, making it highly portable. It supports both system-on-chip (SoC) and network co-processor (NCP) designs.

**...a Thread Certified Component**, implementing all features defined in the [Thread 1.1.1 specification](http://threadgroup.org/technology/ourtechnology#specifications), including all Thread networking layers (IPv6, 6LoWPAN, IEEE 802.15.4 with MAC security, Mesh Link Establishment, Mesh Routing) and device roles, as well as [Border Router](https://github.com/openthread/borderrouter) support.

More information about Thread can be found at [threadgroup.org](http://threadgroup.org/). Thread is a registered trademark of the Thread Group, Inc.

[thread]: http://threadgroup.org/technology/ourtechnology
[ot-repo]: https://github.com/openthread/openthread
[ot-logo]: doc/images/openthread_logo.png
[ot-travis]: https://travis-ci.org/openthread/openthread
[ot-travis-svg]: https://travis-ci.org/openthread/openthread.svg?branch=master
[ot-appveyor]: https://ci.appveyor.com/project/jwhui/openthread
[ot-appveyor-svg]: https://ci.appveyor.com/api/projects/status/r5qwyhn9p26nmfk3?svg=true
[ot-codecov]: https://codecov.io/gh/openthread/openthread
[ot-codecov-svg]: https://codecov.io/gh/openthread/openthread/branch/master/graph/badge.svg

# Who supports OpenThread?

<a href="https://www.arm.com/"><img src="doc/images/ot-contrib-arm.png" alt="ARM" width="200px"></a><a href="http://www.dialog-semiconductor.com/"><img src="doc/images/ot-contrib-dialog.png" alt="Dialog" width="200px"></a><a href="https://www.microsoft.com/en-us/"><img src="doc/images/ot-contrib-ms.png" alt="Microsoft" width="200px"></a><a href="https://nest.com/"><img src="doc/images/ot-contrib-nest.png" alt="Nest" width="200px"></a><br /><a href="http://www.nordicsemi.com/"><img src="doc/images/ot-contrib-nordic.png" alt="Nordic" width="200px"></a><a href="http://www.nxp.com/"><img src="doc/images/ot-contrib-nxp.png" alt="NXP" width="200px"></a><a href="https://www.particle.io/"><img src="doc/images/ot-contrib-particle.png" alt="Particle" width="200px"></a><a href="http://www.qorvo.com/"><img src="doc/images/ot-contrib-qorvo.png" alt="Qorvo" width="200px"></a><br /><a href="https://www.qualcomm.com/"><img src="doc/images/ot-contrib-qc.png" alt="Qualcomm" width="200px"></a><a href="https://www.synopsys.com/"><img src="doc/images/ot-contrib-synopsys.png" alt="Synopsys" width="200px"></a><a href="https://www.ti.com/"><img src="doc/images/ot-contrib-ti.png" alt="Texas Instruments" width="200px"></a><a href="https://www.zephyrproject.org/"><img src="doc/images/ot-contrib-zephyr.png" alt="Zephyr Project" width="200px"></a>

# Getting started

All end-user documentation and guides are located at [openthread.io](https://openthread.io). If you're looking to do things like...

* Learn more about OpenThread features and enhancements
* Use OpenThread in your products
* Learn how to build and configure a Thread network
* Port OpenThread to a new platform
* Build an application on top of OpenThread
* Certify a product using OpenThread

...then [openthread.io](https://openthread.io) is the place for you.

> Note: For users in China, end-user documentation is available at [openthread.google.cn](https://openthread.google.cn).

If you're interested in contributing to OpenThread, read on.

# Contributing

We would love for you to contribute to OpenThread and help make it even better than it is today! See our [Contributing Guidelines](https://github.com/openthread/openthread/blob/master/CONTRIBUTING.md) for more information.

Contributors are required to abide by our [Code of Conduct](https://github.com/openthread/openthread/blob/master/CODE_OF_CONDUCT.md) and [Coding Conventions and Style Guide](https://github.com/openthread/openthread/blob/master/STYLE_GUIDE.md).

# Versioning

OpenThread follows the [Semantic Versioning guidelines](http://semver.org/) for release cycle transparency and to maintain backwards compatibility. OpenThread's versioning is independent of the Thread protocol specification version but will clearly indicate which version of the specification it currently supports.

# License

OpenThread is released under the [BSD 3-Clause license](https://github.com/openthread/openthread/blob/master/LICENSE). See the [`LICENSE`](https://github.com/openthread/openthread/blob/master/LICENSE) file for more information.

Please only use the OpenThread name and marks when accurately referencing this software distribution. Do not use the marks in a way that suggests you are endorsed by or otherwise affiliated with Nest, Google, or The Thread Group.

# Need help?

There are numerous avenues for OpenThread support:

* Bugs and feature requests — [submit to the Issue Tracker](https://github.com/openthread/openthread/issues)
* Stack Overflow — [post questions using the `openthread` tag](http://stackoverflow.com/questions/tagged/openthread)
* Google Groups — [discussion and announcements at openthread-users](https://groups.google.com/forum/#!forum/openthread-users)

The openthread-users Google Group is the recommended place for users to discuss OpenThread and interact directly with the OpenThread team.
