# Nest Labs Fault Injection

## Introduction

Nest Labs Fault Injection (nlfaultinjection) is designed to provide a
simple, portable fault injection framework.

It should be capable of running on just about any system, no
matter how constrained and depends only on the C Standard Library.

This is a simple framework that helps implement fault injection APIs.
Fault injection APIs simplify the testing (manual or automated) of the error
handling logic of individual components and whole systems; as a testing tool, a
fault injection API is not as powerful as an extensive set of mock objects, but
has the advantage of being able to run on a live system.

This is not an official Google product.

## Getting Started

A SW module implements a fault-injection API by doing the following:

1. Instantiate a FaultInjection::Manager object and define a list fault IDs; the
   Manager needs a name, storage for the configuration of the faults and the name
   of each fault; the Manager instance is the public API, through which another
   module of the system can configure faults (for example a test harness can turn
   on a fault by calling the FailAtFault() method). The module should also provide
   a public function that returns a C++ reference to the Manager and initializes
   the Manager the first time it is called.
2. At the location in which the fault is to be injected, the nlFAULT_INJECT
   macro is used to inline code that if executed triggers a fault in the
   system; the macro queries the Manager object calling the CheckFault() method
   and executes the extra code if it returns true.  The invocations of CheckFault
   are counted per faultID, which allows a test harness to automatically loop over
   different valid configurations.

## Interact

There are numerous avenues for nlfaultinjection support:

  * Bugs and feature requests — [submit to the Issue Tracker](https://github.com/nestlabs/nlfaultinjection/issues)
  * Google Groups — discussion and announcements
    * [nlfaultinjection-announce](https://groups.google.com/forum/#!forum/nlfaultinjection-announce) — release notes and new updates on nlfaultinjection
    * [nlfaultinjection-users](https://groups.google.com/forum/#!forum/nlfaultinjection-users) — discuss use of and enhancements to nlfaultinjection
