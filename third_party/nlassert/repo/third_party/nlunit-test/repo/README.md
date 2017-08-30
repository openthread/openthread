Nest Labs Unit Test
===================

# Introduction

Nest Labs Unit Test (nlunit-test) is designed to provide a
simple, portable unit test suite framework.

It should be capable of running on just about any system, no
matter how constrained and depends only on the C Standard Library.

It is recognized and acknowledged that not every piece of code or
subsystem, library, thread or process can be easily unit tested.
However, unit tests are a "Good Thing&trade;" and will give you the
confidence to make changes and prove to both yourself and other
colleagues that you have things **right**, in so far as your
unit tests cover **right**.

Unit tests will be combined with other methods of verification to
fully cover the various test requirements in the system. The
purpose of the unit test is to isolate your module's methods and
ensure that they have the proper output, affect state properly,
and handle errors appropriately. This ensures each of the building
blocks of the system functions correctly. These tests can also be
used for regression testing so that any code changes that affect a
module's output will be automatically flagged.

# Interact

There are numerous avenues for nlunit-test support:

  * Bugs and feature requests — [submit to the Issue Tracker](https://github.com/nestlabs/nlunit-test/issues)
  * Google Groups — discussion and announcements
    * [nlunit-test-announce](https://groups.google.com/forum/#!forum/nlunit-test-announce) — release notes and new updates on nlunit-test
    * [nlunit-test-users](https://groups.google.com/forum/#!forum/nlunit-test-users) — discuss use of and enhancements to nlunit-test

# Versioning

nlunit-test follows the [Semantic Versioning guidelines](http://semver.org/) 
for release cycle transparency and to maintain backwards compatibility.

# License

nlunit-test is released under the [Apache License, Version 2.0 license](https://opensource.org/licenses/Apache-2.0). 
See the `LICENSE` file for more information.
