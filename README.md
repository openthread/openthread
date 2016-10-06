[![OpenThread][ot-logo]][ot-repo]  
[![Build Status][ot-travis-svg]][ot-travis]
[![Build Status][ot-appveyor-svg]][ot-appveyor]
[![Coverage Status][ot-codecov-svg]][ot-codecov]

---

OpenThread is an open-source implementation of the [Thread][thread]
networking protocol. Nest has released OpenThread to make the technology
used in Nest products more broadly available to developers to accelerate 
the development of products for the connected home.

The Thread specification defines an IPv6-based reliable, secure and
low-power wireless device-to-device communication protocol for home
applications. More information about Thread can be found on
[threadgroup.org](http://threadgroup.org/).

[thread]: http://threadgroup.org/technology/ourtechnology
[ot-repo]: https://github.com/openthread/openthread
[ot-logo]: doc/images/openthread_logo.png
[ot-travis]: https://travis-ci.org/openthread/openthread
[ot-travis-svg]: https://travis-ci.org/openthread/openthread.svg?branch=master
[ot-appveyor]: https://ci.appveyor.com/project/jwhui/openthread
[ot-appveyor-svg]: https://ci.appveyor.com/api/projects/status/r5qwyhn9p26nmfk3?svg=true
[ot-codecov]: https://codecov.io/gh/openthread/openthread
[ot-codecov-svg]: https://codecov.io/gh/openthread/openthread/branch/master/graph/badge.svg

## Features ##

 *  Highly portable: OS and platform agnostic with a radio
    abstraction layer
 *  Implements the End Device, Router, Leader and Border Router roles
 *  Small memory footprint

OpenThread implements all Thread networking layers including IPv6,
6LoWPAN, IEEE 802.15.4 with MAC security, Mesh Link Establishment, and
Mesh Routing.


# Who is supporting OpenThread #

![OpenThread Contributor Logos](doc/images/openthread_contrib.png)

Nest, along with ARM, Atmel, a subsidiary of Microchip Technology,
Dialog Semiconductor, Qualcomm Technologies, Inc. (a subsidiary of
Qualcomm Incorporated and Texas Instruments Incorporated), and Microsoft
Corporation are contributing to the ongoing development of OpenThread.


# Getting started #

The easiest way to get started is to run the CLI example in
`/examples/apps/cli`. See the
[CLI example README](examples/apps/cli/README.md)
for more details.


## What's included ##

In the repo you'll find the following directories and files:

File/Folder   | Provides
--------------|----------------------------------------------------------------
`doc`         | Doxygen docs
`examples`    | Sample applications demonstrating OpenThread
`include`     | Public API header files
`src`         | Core implementation of the Thread standard and related add-ons
`tests`       | Unit and Thread conformance tests
`third_party` | Third-party code used by OpenThread
`tools`       | Helpful utilities related to the OpenThread project


## Documentation ##

The Doxygen reference docs are [hosted online][ot-docs] and generated
as part of the build.

[ot-docs]: http://openthread.github.io/openthread/


# Getting help #

Submit bugs and feature requests to [issue tracker][ot-issues]. Usage
questions? Post your questions to [Stack Overflow][stackoverflow] using the
[`openthread` tag][ot-tag]. We also use Google Groups for discussion
and announcements:

 *  [openthread-announce](https://groups.google.com/forum/#!forum/openthread-announce)
    \- subscribe for release notes and new updates on OpenThread
 *  [openthread-users](https://groups.google.com/forum/#!forum/openthread-users)
    \- the best place for users to discuss OpenThread and interact with
    the OpenThread team
 *  [openthread-devel](https://groups.google.com/forum/#!forum/openthread-devel)
    \- team members discuss the on-going development of OpenThread

[ot-issues]: https://github.com/openthread/openthread/issues
[stackoverflow]: http://stackoverflow.com/
[ot-tag]: http://stackoverflow.com/questions/tagged/openthread


# Versioning #

OpenThread follows [the Semantic Versioning guidelines][semver] for
release cycle transparency and to maintain backwards compatibility.
OpenThread's versioning is independent of the Thread protocol
specification version but will clearly indicate which version of the
specification it currently supports.

[semver]: http://semver.org/


# Contributing #

See the [`CONTRIBUTING.md`](CONTRIBUTING.md) file for more information.


# License #

OpenThread is released under the [BSD 3-Clause license](LICENSE). See
the [`LICENSE`](LICENSE) file for more information.

Please only use the OpenThread name and marks when accurately
referencing this software distribution. Do not use the marks in
a way that suggests you are endorsed by or otherwise affiliated with
Nest, Google, or The Thread Group.
