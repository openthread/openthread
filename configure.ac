#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

#
#  Copyright (c) 2016, Nest Labs, Inc.
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

#
# Declare autoconf version requirements
#
AC_PREREQ([2.68])

#
# Initialize autoconf for the package
#
AC_INIT([OPENTHREAD],
        m4_esyscmd([third_party/nlbuild-autotools/repo/scripts/mkversion -b `cat .default-version` .]),
        [openthread-devel@googlegroups.com],
        [openthread],
        [http://github.com/openthread/openthread])

# Tell the rest of the build system the absolute path where the
# nlbuild-autotools repository is rooted at.

AC_SUBST(nlbuild_autotools_stem,[third_party/nlbuild-autotools/repo])
AC_SUBST(abs_top_nlbuild_autotools_dir,[\${abs_top_srcdir}/\${nlbuild_autotools_stem}])

#
# OPENTHREAD interface current, revision, and age versions.
#
# Maintainters: Please manage these fields as follows:
#
#   Interfaces removed:    CURRENT++, AGE = 0, REVISION = 0
#   Interfaces added:      CURRENT++, AGE++,   REVISION = 0
#   No interfaces changed:                     REVISION++
#
#
AC_SUBST(LIBOPENTHREAD_VERSION_CURRENT,  [0])
AC_SUBST(LIBOPENTHREAD_VERSION_AGE,      [5])
AC_SUBST(LIBOPENTHREAD_VERSION_REVISION, [0])
AC_SUBST(LIBOPENTHREAD_VERSION_INFO,     [${LIBOPENTHREAD_VERSION_CURRENT}:${LIBOPENTHREAD_VERSION_REVISION}:${LIBOPENTHREAD_VERSION_AGE}])

#
# Check the sanity of the source directory by checking for the
# presence of a key watch file
#
AC_CONFIG_SRCDIR([include/openthread.h])

#
# Tell autoconf where to find auxilliary build tools (e.g. config.guess,
# install-sh, missing, etc.)
#
AC_CONFIG_AUX_DIR([third_party/nlbuild-autotools/repo/autoconf])

#
# Tell autoconf where to find auxilliary M4 macros
#
AC_CONFIG_MACRO_DIR([third_party/nlbuild-autotools/repo/autoconf/m4])

#
# Tell autoconf what file the package is using to aggregate C preprocessor
# defines.
#
AC_CONFIG_HEADERS([include/openthread-config.h])

#
# Figure out what the canonical build, host and target tuples are.
#
AC_CANONICAL_BUILD
AC_CANONICAL_HOST
AC_CANONICAL_TARGET

#
# Mac OS X / Darwin ends up putting some versioning cruft on the end of its
# tuple that we don't care about in this script. Create "clean" variables
# devoid of it.
#

NL_FILTERED_CANONICAL_BUILD
NL_FILTERED_CANONICAL_HOST
NL_FILTERED_CANONICAL_TARGET

#
# Configure automake with the desired options, indicating that this is not
# a native GNU package, that we want "silent" build rules, and that we want
# objects built in the same subdirectory as their source rather than collapsed
# together at the top-level directory.
#
# Disable silent build rules by either passing --disable-silent-rules to
# configure or passing V=1 to make
#
AM_INIT_AUTOMAKE([1.14 foreign silent-rules subdir-objects tar-pax])

#
# Silent build rules requires at least automake-1.11. Employ
# techniques for not breaking earlier versions of automake.
#
m4_ifdef([AM_SILENT_RULES], [AM_SILENT_RULES([yes])])
AM_SILENT_RULES([yes])

#
# Enable maintainer mode to prevent the package from constantly trying
# to rebuild configure, Makefile.in, etc. Rebuilding such files rarely,
# if ever, needs to be done "in the field".
#
# Use the included 'bootstrap' script instead when necessary.
#
AM_MAINTAINER_MODE

#
# Check for target
#

case ${target_os} in

    *darwin*)
        OPENTHREAD_TARGET=darwin
        OPENTHREAD_TARGET_DEFINES="-DOPENTHREAD_TARGET_DARWIN"
        ;;

    *linux*)
        OPENTHREAD_TARGET=linux
        OPENTHREAD_TARGET_DEFINES="-DOPENTHREAD_TARGET_LINUX"
        ;;

esac

AC_SUBST(OPENTHREAD_TARGET_DARWIN)
AM_CONDITIONAL([OPENTHREAD_TARGET_DARWIN], [test "${OPENTHREAD_TARGET}" = "darwin"])

AC_SUBST(OPENTHREAD_TARGET_LINUX)
AM_CONDITIONAL([OPENTHREAD_TARGET_LINUX], [test "${OPENTHREAD_TARGET}" = "linux"])

AC_SUBST(OPENTHREAD_TARGET_DEFINES)

#
# Checks for build host programs
#

# If we are cross-compiling and we are on an embedded target that
# doesn't support independent, standalone executables, then all
# compiler tests that attempt to create an executable will fail. In
# such circumstances, set AC_NO_EXECUTABLES (see http://sourceware.org/
# ml/newlib/2006/msg00353.html).

AC_MSG_CHECKING([whether to disable executable checking])
if test "$cross_compiling" = yes; then
    AC_NO_EXECUTABLES
    AC_MSG_RESULT([yes])
else
    AC_MSG_RESULT([no])
fi

# Passing -Werror to GCC-based or -compatible compilers breaks some
# autoconf tests (see
# http://lists.gnu.org/archive/html/autoconf-patches/2008-09/msg00014.html).
#
# If -Werror has been passed transform it into -Wno-error. We'll
# transform it back later with NL_RESTORE_WERROR.

NL_SAVE_WERROR

# Check for compilers.
#
# These should be checked BEFORE we check for and, implicitly,
# initialize libtool such that libtool knows what languages it has to
# work with.

AC_PROG_CPP
AC_PROG_CPP_WERROR

AC_PROG_CC
AC_PROG_CC_C_O

AC_PROG_CXXCPP

AC_PROG_CXX
AC_PROG_CXX_C_O

# Check for other compiler toolchain tools.

AC_CHECK_TOOL(AR, ar)
AC_CHECK_TOOL(RANLIB, ranlib)
AC_CHECK_TOOL(OBJCOPY, objcopy)
AC_CHECK_TOOL(STRIP, strip)

# Check for other host tools.

AC_PROG_INSTALL
AC_PROG_LN_S

#
# Checks for specific compiler characteristics
#

#
# Common compiler flags we would like to have.
#
#   -Wall                        CC, CXX
#

PROSPECTIVE_CFLAGS="-Wall -std=c99 -pedantic-errors"
PROSPECTIVE_CXXFLAGS="-Wall"

AX_CHECK_COMPILER_OPTIONS([C],   ${PROSPECTIVE_CFLAGS})
AX_CHECK_COMPILER_OPTIONS([C++], ${PROSPECTIVE_CXXFLAGS})

# Check for and initialize libtool

LT_INIT

#
# Debug instances
#
AC_MSG_NOTICE([checking whether to build debug instances])

# Debug

NL_ENABLE_DEBUG([no])

AM_CONDITIONAL([OPENTHREAD_BUILD_DEBUG], [test "${nl_cv_build_debug}" = "yes"])

#
# Code coverage and compiler optimization
#

# Coverage

NL_ENABLE_COVERAGE([no])

AM_CONDITIONAL([OPENTHREAD_BUILD_COVERAGE], [test "${nl_cv_build_coverage}" = "yes"])

NL_ENABLE_COVERAGE_REPORTS([auto])

AM_CONDITIONAL([OPENTHREAD_BUILD_COVERAGE_REPORTS], [test "${nl_cv_build_coverage_reports}" = "yes"])

# Optimization

NL_ENABLE_OPTIMIZATION([yes])

AM_CONDITIONAL([OPENTHREAD_BUILD_OPTIMIZED], [test "${nl_cv_build_optimized}" = "yes"])

#
# Code style
#

AC_SUBST(PRETTY, ["\${abs_top_srcdir}/.astyle/astyle-wrap.sh"])
AC_SUBST(PRETTY_ARGS, ["astyle --options=\${abs_top_builddir}/.astyle/astyle-opts"])
AC_SUBST(PRETTY_CHECK_ARGS, ["astyle --options=\${abs_top_builddir}/.astyle/astyle-opts --dry-run"])

#
# Tests
#
AC_MSG_NOTICE([checking whether to build tests])

# Tests

NL_ENABLE_TESTS([yes])

AM_CONDITIONAL([OPENTHREAD_BUILD_TESTS], [test "${nl_cv_build_tests}" = "yes"])

# Check whether to run only the minimum set of tests.

AC_MSG_CHECKING([whether to run only the minimum set of tests.])
AC_ARG_ENABLE(minimum-set-of-tests,
    [AS_HELP_STRING([--enable-minimum-set-of-tests],[Run only the most essential tests. @<:@default=no@:>@.])],
    [
        case "${enableval}" in

        no|yes)
            enable_minimum_set_of_tests=${enableval}
            ;;

        *)
            AC_MSG_ERROR([Invalid value ${enableval} for --enable-minimum-set-of-tests])
            ;;

        esac
    ],
    [enable_minimum_set_of_tests=no])
AC_MSG_RESULT(${enable_minimum_set_of_tests})

AM_CONDITIONAL([OPENTHREAD_WITH_MINIMUM_SET_OF_TESTS], [test "${enable_minimum_set_of_tests}" = "yes"])

if test "${enable_minimum_set_of_tests}" = "yes"; then
    if test "${nl_cv_build_coverage}" = "yes"; then
        AC_MSG_ERROR(["Do not run code coverage only with the minimum set of tests."])
    fi
fi

# Check for Python and Python libraries used in the tests
if test "${nl_cv_build_tests}" = "yes"; then
    AC_MSG_CHECKING([checking for Python and Python libraries used in tests.])

    # Check for Python
    python --version 2> /dev/null
    if test $? != 0; then
        AC_MSG_ERROR(["Cannot run tests: missing Python."])
    fi

    # Check for Python pexpect library
    python -c 'import pexpect' 2> /dev/null
    if test $? != 0; then
        AC_MSG_ERROR(["Cannot run tests: missing Python pexpect library."])
    fi

    AC_MSG_RESULT([ok])
fi

#
# CLI Library
#

AC_ARG_ENABLE(cli,
    [AS_HELP_STRING([--enable-cli],[Enable CLI suport @<:@default=no@:>@.])],
    [
        case "${enableval}" in

        no|yes)
            enable_cli=${enableval}
            ;;

        *)
            AC_MSG_ERROR([Invalid value ${with_cli} for --enable-cli])
            ;;
        esac
    ],
    [enable_cli=no])
AC_MSG_RESULT(${enable_cli})
AM_CONDITIONAL([OPENTHREAD_ENABLE_CLI], [test "${enable_cli}" = "yes"])

#
# NCP Library
#

AC_ARG_ENABLE(ncp,
    [AS_HELP_STRING([--enable-ncp],[Enable NCP suport @<:@default=no@:>@.])],
    [
        case "${enableval}" in

        no|yes)
            enable_ncp=${enableval}
            ;;

        *)
            AC_MSG_ERROR([Invalid value ${with_ncp} for --enable-ncp])
            ;;
        esac
    ],
    [enable_ncp=no])
AC_MSG_RESULT(${enable_ncp})
AM_CONDITIONAL([OPENTHREAD_ENABLE_NCP], [test "${enable_ncp}" = "yes"])

#
# Examples
#

AC_ARG_WITH(examples,
    [AS_HELP_STRING([--with-examples=TARGET],
        [Specify the examples from one of: none, posix, cc2538 @<:@default=none@:>@.])],
    [
        case "${with_examples}" in

        none|posix|cc2538)
            ;;
        *)
            AC_MSG_ERROR([Invalid value ${with_examples} for --with-examples])
            ;;
        esac
    ],
    [with_examples=none])

OPENTHREAD_EXAMPLES=${with_examples}

case ${with_examples} in

    posix)
        OPENTHREAD_EXAMPLES_POSIX=1
        ;;

    cc2538)
        OPENTHREAD_EXAMPLES_CC2538=1
        ;;

esac

AC_MSG_RESULT(${OPENTHREAD_EXAMPLES})

AC_SUBST(OPENTHREAD_EXAMPLES)
AC_DEFINE_UNQUOTED([OPENTHREAD_EXAMPLES],[${OPENTHREAD_EXAMPLES}],[OpenThread examples])

AC_SUBST(OPENTHREAD_EXAMPLES_POSIX)
AM_CONDITIONAL([OPENTHREAD_EXAMPLES_POSIX], [test "${OPENTHREAD_EXAMPLES}" = "posix"])
AC_DEFINE_UNQUOTED([OPENTHREAD_EXAMPLES_POSIX],[${OPENTHREAD_EXAMPLES_POSIX}],[Define to 1 if you want to use posix examples])

AC_SUBST(OPENTHREAD_EXAMPLES_CC2538)
AM_CONDITIONAL([OPENTHREAD_EXAMPLES_CC2538], [test "${OPENTHREAD_EXAMPLES}" = "cc2538"])
AC_DEFINE_UNQUOTED([OPENTHREAD_EXAMPLES_CC2538],[${OPENTHREAD_EXAMPLES_CC2538}],[Define to 1 if you want to use cc2538 examples])

#
# Tools
#

AC_MSG_CHECKING([whether to build tools])
AC_ARG_ENABLE(tools,
    [AS_HELP_STRING([--disable-tools],[Disable building of tools @<:@default=no@:>@.])],
    [
        case "${enableval}" in

        no|yes)
            build_tools=${enableval}
            ;;

        *)
            AC_MSG_ERROR([Invalid value ${enableval} for --enable-tools])
            ;;

        esac
    ],
    [build_tools=yes])
AC_MSG_RESULT(${build_tools})
AM_CONDITIONAL([OPENTHREAD_BUILD_TOOLS], [test "${build_tools}" = "yes"])

#
# Documentation
#

# Determine whether or not documentation (via Doxygen) should be built
# or not, with 'auto' as the default and establish a default support
# value for GraphViz 'dot' support.

NL_ENABLE_DOCS([auto],[NO])

AM_CONDITIONAL(OPENTHREAD_BUILD_DOCS, [test "${nl_cv_build_docs}" = "yes"])

#
# Checks for libraries and packages.
#
# At minimum, the following packages are optional, depending on
# configuration:
#
#   * TBD
#
AC_MSG_NOTICE([checking required package dependencies])

# NL_WITH_PACKAGE(...)

#
# Check for headers
#
AC_HEADER_STDBOOL
AC_HEADER_STDC

AC_CHECK_HEADERS([stdint.h])
AC_CHECK_HEADERS([string.h])

#
# Check for types and structures
#
AC_TYPE_INT8_T
AC_TYPE_INT16_T
AC_TYPE_INT32_T
AC_TYPE_INT64_T
AC_TYPE_UINT8_T
AC_TYPE_UINT16_T
AC_TYPE_UINT32_T
AC_TYPE_UINT64_T

#
# Checks for library functions
#

if test "${ac_no_link}" != "yes"; then
    AC_CHECK_FUNCS([memcpy])
fi

# Add any code coverage CPPFLAGS and LDFLAGS

CPPFLAGS="${CPPFLAGS} ${NL_COVERAGE_CPPFLAGS}"
LDFLAGS="${LDFLAGS} ${NL_COVERAGE_LDFLAGS}"

# At this point, we can restore the compiler flags to whatever the
# user passed in, now that we're clear of an -Werror issues by
# transforming -Wno-error back to -Werror.

NL_RESTORE_WERROR

#
# Identify the various makefiles and auto-generated files for the package
#
AC_CONFIG_FILES([
Makefile
include/Makefile
include/cli/Makefile
include/crypto/Makefile
include/ncp/Makefile
include/platform/Makefile
src/Makefile
src/cli/Makefile
src/ncp/Makefile
src/core/Makefile
third_party/Makefile
third_party/mbedtls/Makefile
examples/Makefile
examples/posix/Makefile
examples/posix/app/Makefile
examples/posix/app/cli/Makefile
examples/posix/app/ncp/Makefile
examples/posix/platform/Makefile
examples/cc2538/Makefile
examples/cc2538/app/Makefile
examples/cc2538/app/cli/Makefile
examples/cc2538/app/ncp/Makefile
examples/cc2538/platform/Makefile
tests/Makefile
tests/scripts/Makefile
tests/unit/Makefile
doc/Makefile
])

#
# Generate the auto-generated files for the package
#
AC_OUTPUT

#
# Summarize the package configuration
#

AC_MSG_NOTICE([

  Configuration Summary
  ---------------------
  Package                                   : ${PACKAGE_NAME}
  Version                                   : ${PACKAGE_VERSION}
  Interface                                 : ${LIBOPENTHREAD_VERSION_INFO//:/.}
  Build system                              : ${build}
  Host system                               : ${host}
  Target system                             : ${target}
  Target architecture                       : ${target_cpu}
  Target OS                                 : ${target_os}
  Cross compiling                           : ${cross_compiling}
  Build shared libraries                    : ${enable_shared}
  Build static libraries                    : ${enable_static}
  Build debug libraries                     : ${nl_cv_build_debug}
  Build optimized libraries                 : ${nl_cv_build_optimized}
  Build coverage libraries                  : ${nl_cv_build_coverage}
  Build coverage reports                    : ${nl_cv_build_coverage_reports}
  Lcov                                      : ${LCOV:--}
  Genhtml                                   : ${GENHTML:--}
  Build tests                               : ${nl_cv_build_tests}
  Build tools                               : ${build_tools}
  Minimum set of tests                      : ${enable_minimum_set_of_tests}
  Prefix                                    : ${prefix}
  Documentation support                     : ${nl_cv_build_docs}
  Doxygen                                   : ${DOXYGEN:--}
  GraphViz dot                              : ${DOT:--}
  C Preprocessor                            : ${CPP}
  C Compiler                                : ${CC}
  C++ Preprocessor                          : ${CXXCPP}
  C++ Compiler                              : ${CXX}
  Archiver                                  : ${AR}
  Archive Indexer                           : ${RANLIB}
  Symbol Stripper                           : ${STRIP}
  Object Copier                             : ${OBJCOPY}
  C Preprocessor flags                      : ${CPPFLAGS:--}
  C Compile flags                           : ${CFLAGS:--}
  C++ Compile flags                         : ${CXXFLAGS:--}
  Link flags                                : ${LDFLAGS:--}
  Link libraries                            : ${LIBS}
  Astyle                                    : ${PRETTY}
  Astyle pretty args                        : ${PRETTY_ARGS}
  Astyle pretty check args                  : ${PRETTY_CHECK_ARGS}
  OpenThread CLI support                    : ${enable_cli}
  OpenThread NCP support                    : ${enable_ncp}
  OpenThread examples                       : ${OPENTHREAD_EXAMPLES}

])
