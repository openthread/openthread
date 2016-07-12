Nest Labs Assert
================

## Introduction

This package defines macros and interfaces for performing both
compile- and run-time assertion checking and run-time exception
handling.

Where exception-handing is concerned, the format of the macros are
inspired by those found in Mac OS Classic and, later, Mac OS
X. These, in turn, were inspired by "Living In an Exceptional
World" by Sean Parent (develop, The Apple Technical Journal, Issue
11, August/September 1992). See:

  http://www.mactech.com/articles/develop/issue_11/Parent_final.html

for the methodology behind these error handling and assertion macros.

## Overview

The interfaces in this package come in two interface modalities:

* **Run-time** Interfaces that dynamically check a logical assertion and alter run-time execution on assertion firing.
* **Compile-time** Interfaces that statically check a logical assertion and terminate compile-time execution on assertion firing.

### Run-time

The run-time modality interfaces in this package come in three
families:

* **Assertion** Similar to the traditional C Standard Library
    [assert()](http://pubs.opengroup.org/onlinepubs/009695399/functions/assert.html).
* **Precondition** Designed to be placed at the head of an interface or
    method to check incoming parameters and return on assertion failure.
* **Exception** Designed to jump to a local label on assertion failure
    to support the method of error and exception handling that advocates
    a single function or method return site and, by extension, consolidated
    points of exception and error handling as well as resource clean-up.

There are several styles of interfaces within each family and several
potential variants within each style, all of which are summarized
below and are described in detail in the following sections.

### Compile-time

The compile-time modality interfaces in this package are simpler and
come in a single family with a couple of styles and variants, all
intended to provide parallelism with the run-time interfaces.

