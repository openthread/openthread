<a href="https://github.com/openthread/openthread">![Logo](doc/images/openthread_logo.png)</a>

[![Build Status](https://travis-ci.org/openthread/openthread.svg?branch=master)](https://travis-ci.org/openthread/openthread)

OpenThread is an open-source implementation of the [Thread](http://threadgroup.org/technology/ourtechnology) networking protocol. With OpenThread, Nest is making the technology used in Nest products more broadly available to accelerate the development of products for the connected home.

The Thread specification defines an IPv6-based reliable, secure and low-power wireless device-to-device communication protocol for home applications. More information about Thread can be found on [threadgroup.org](http://www.threadgroup.org/).

## Features
- Highly portable: OS and platform agnostic, with a radio abstraction layer
- Implements the End Device, Router, Leader & Border Router roles
- Small memory footprint

OpenThread implements all Thread networking layers, including IPv6, 6LoWPAN, IEEE 802.15.4 with MAC security, Mesh Link Establishment, and Mesh Routing.


# Who is behind OpenThread

![Logo](doc/images/openthread_contrib.png)

Nest, along with ARM, Atmel, a subsidiary of Microchip Technology, Dialog Semiconductor,  Qualcomm Technologies, Inc., a subsidiary of Qualcomm Incorporated and Texas Instruments Incorporated are contributing to the ongoing development of OpenThread.


# Getting Started

The easiest way to get started is to run the CLI example in `/examples/cli`. See the [CLI example README](examples/cli/README.md) for more details.


## What's included

In the repo you'll find the following directories and files

File/Folder	 | Provides
-------|--------
doc | Doxygen docs
examples | Sample applications demonstrating various parts of OpenThread
include | Includes header files for OpenThread API
src | The core implementation of the Thread standard
tests | Unit and Thread conformance tests
third_party | Third-party code used by OpenThread


## Docs
The Doxygen reference docs are [hosted online](http://openthread.github.io/openthread/) and generated as part of the build.


# Getting Help

Submit bugs and feature requests to [issue tracker](https://github.com/openthread/openthread/issues). Usage questions? Post questions to [Stack Overflow](http://stackoverflow.com/) using the [[openthread] tag](http://stackoverflow.com/questions/tagged/openthread). We also use Google Groups for discussion and announcements:

* [openthread-announce](https://groups.google.com/forum/#!forum/openthread-announce) - subscribe for release notes and new updates on OpenThread
* [openthread-users](https://groups.google.com/forum/#!forum/openthread-users) - the best place for users to discuss OpenThread and interact with the OpenThread team
* [openthread-devel](https://groups.google.com/forum/#!forum/openthread-devel) - team members discuss the on-going development of OpenThread


# Versioning

OpenThread follows [the Semantic Versioning guidelines](http://semver.org/) for release cycle transparency and to maintain backwards compatibility. OpenThread's versioning is independent of the Thread protocol specification version but will clearly indicate which version of the specification it currently supports.


# Contributing

See the [CONTRIBUTING.md](CONTRIBUTING.md) file for more information.


# About the Thread Group

OpenThread is an open source implementation of the Thread 1.0.1 Final Specification.
The Thread 1.0.1 Final Specification is promulgated by the Thread Group. The Thread
Group is a non-profit organization formed for the purposes of defining one or
more specifications, best practices, reference architectures, implementation
guidelines and certification programs to promote the availability of compliant
implementations of the Thread protocol. Information on becoming a Member, including
information about the benefits thereof, can be found at <http://threadgroup.org>.

OpenThread is not affiliated with or endorsed by the Thread Group. Implementation
of this OpenThread code does not assure compliance with the Thread 1.0.1 Final
Specification and does not convey the right to identify any final product as Thread
certified. Members of the Thread Group may hold patents and other intellectual
property rights relating to the Thread 1.0.1 Final Specification, ownership and
licenses of which are subject to the Thread Group’s IP Policies, and not this license.


# License

OpenThread is released under the [BSD 3-Clause license](LICENSE).
See the [LICENSE](LICENSE) file for more information.

The included copyright to the OpenThread code is subject to the license in the
LICENSE file, and all other rights and licenses are expressly reserved.
No warranty or assurance is made with respect to additional rights that may be
required to implement this code.

Please only use the OpenThread name and marks when accurately
referencing this software distribution, and do not use these marks in
a way that suggests you are endorsed by or otherwise affiliated with
Nest, Google, or The Thread Group.
