Nest Labs Unit Test
===================

Nest Labs Unit Test (nlunittest) is designed to provide a
simple, portable unit test suite framework.

It should be capable of running on just about any system, no
matter how constrained and depends only on the C Standard Library.

It is recognized and acknowledged that not every piece of code or
subsystem, library, thread or process can be easily unit tested.
However, unit tests are a Good Thing&tm; and will give you the
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

