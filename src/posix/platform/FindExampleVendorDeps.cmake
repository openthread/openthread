#
#  Copyright (c) 2023, The OpenThread Authors.
#  All rights reserved.
#
#  Redistribution and use in source and binary forms, with or without
#  modification, are permitted provided that the following conditions are met:
#  1. Redistributions of source code must retain the above copyright
#     notice, this list of conditions and the following disclaimer.
#  2. Redistributions in binary form must reproduce the above copyright
#     notice, this list of conditions and the following disclaimer in the
#     documentation and/or other materials provided with the distribution.
#  3. Neither the name of the copyright holder nor the
#     names of its contributors may be used to endorse or promote products
#     derived from this software without specific prior written permission.
#
#  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
#  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
#  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
#  ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
#  LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
#  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
#  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
#  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
#  CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
#  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
#  POSSIBILITY OF SUCH DAMAGE.
#

#[=======================================================================[.rst:
FindExampleRcpVendorDeps
------------------------

This file provides a reference for how to implement an RCP vendor
dependency CMake module to resolve external libraries and header
files used by a vendor implementation in the posix library.

The name of this file and the name of the targets it defines are
conventionally related. For the purpose of this reference, targets
will be based off of the identifier "ExampleRcpVendorDeps".

For more information about package resolution using CMake find modules,
reference the cmake-developer documentation.

Imported Targets
^^^^^^^^^^^^^^^^

This module provides the following imported targets, if found:

``ExampleRcpVendorDeps::ExampleRcpVendorDeps``
  RCP vendor interface library dependencies

Result Variables
^^^^^^^^^^^^^^^^

This will define the following variables:

``ExampleRcpVendorDeps_FOUND``
  True if the system has all of the required external dependencies
``ExampleRcpVendorDeps_INCLUDE_DIRS``
  Include directories needed by vendor interface
``ExampleRcpVendorDeps_LIBRARIES``
  Libraries needed by vendor interface

Cache Variables
^^^^^^^^^^^^^^^

Vendors modules may configure various cache variables
while resolving dependencies:

``Dependency0_INCLUDE_DIR``
  The directory containing include files for dependency 0
``Dependency0_LIBRARY``
  The path to the library containing symbols for dependency 0
``Dependency1_INCLUDE_DIR``
  The directory containing include files for dependency 1
``Dependency1_LIBRARY``
  The path to the library containing symbols for dependency 1

#]=======================================================================]

include(FindPackageHandleStandardArgs)

find_path(Dependency0_INCLUDE_DIR
    NAMES example0/example.h
    PATH  ${EXAMPLES_ROOT}/include
)

find_library(Dependency0_LIBRARY
    NAMES example0
    PATH  ${EXAMPLES_ROOT}/lib
)

find_path(Dependency1_INCLUDE_DIR
    NAMES example1/example.h
    PATH ${EXAMPLES_ROOT}/include
)

find_library(Dependency1_LIBRARY
    NAMES example1
    PATH ${EXAMPLES_ROOT}/lib
)

find_package_handle_standard_args(ExampleRcpVendorDeps
    FOUND_VAR ExampleRcpVendorDeps_FOUND
    REQUIRED_VARS Dependency0_INCLUDE_DIR Dependency0_LIBRARY Dependency1_INCLUDE_DIR Dependency1_LIBRARY
)

if(ExampleRcpVendorDeps_FOUND AND NOT ExampleRcpVendorDeps::ExampleRcpVendorDeps)
    set(ExampleRcpVendorDeps_INCLUDE_DIRS ${Dependency0_INCLUDE_DIR} ${Dependency1_INCLUDE_DIR})
    set(ExampleRcpVendorDeps_LIBRARIES ${Dependency0_LIBRARY} ${Dependency1_LIBRARY})

    add_library(ExampleRcpVendorDeps::ExampleRcpVendorDeps UNKNOWN IMPORTED)
    set_target_properties(ExampleRcpVendorDeps::ExampleRcpVendorDeps PROPERTIES
        IMPORTED_LOCATION "${ExampleRcpVendorDeps_LIBRARIES}"
        INTERFACE_INCLUDE_DIRECTORIES "${ExampleRcpVendorDeps_INCLUDE_DIRS}"
    )

    mark_as_advanced(
        Dependency0_INCLUDE_DIR
        Dependency0_LIBRARY
        Dependency1_INCLUDE_DIR
        Dependency1_LIBRARY
    )
endif()

