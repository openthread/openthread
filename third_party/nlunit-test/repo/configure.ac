#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

#
#    Copyright 2015-2016 Nest Labs Inc. All Rights Reserved.
#
#    Licensed under the Apache License, Version 2.0 (the "License");
#    you may not use this file except in compliance with the License.
#    You may obtain a copy of the License at
#
#    http://www.apache.org/licenses/LICENSE-2.0
#
#    Unless required by applicable law or agreed to in writing, software
#    distributed under the License is distributed on an "AS IS" BASIS,
#    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#    See the License for the specific language governing permissions and
#    limitations under the License.
#

#
#    Description:
#      This file is the GNU autoconf input source file for Nest
#      Labs Unit Test.
#

#
# Declare autoconf version requirements
#
AC_PREREQ([2.68])

#
# Initialize autoconf for the package
#
AC_INIT([nlunit-test],
        m4_esyscmd([third_party/nlbuild-autotools/repo/scripts/mkversion -b `cat .default-version` .]),
        [nlunit-test-users@google.com],
        [nlunit-test],
        [https://github.com/nestlabs/nlunit-test/])

# Tell the rest of the build system the absolute path where the
# nlbuild-autotools repository is rooted at.

AC_SUBST(nlbuild_autotools_stem,[third_party/nlbuild-autotools/repo])
AC_SUBST(abs_top_nlbuild_autotools_dir,[\${abs_top_srcdir}/\${nlbuild_autotools_stem}])

#
# NLUNIT_TEST interface current, revision, and age versions.
#
# NOTE: At present, NLUNIT_TEST makes NO ABI compatibility
#       commitments. Consequently, these simply serve as documentation
#       for how the interfaces have evolved.
#
# Maintainters: Please manage these fields as follows:
#
#   Interfaces removed:    CURRENT++, AGE = 0, REVISION = 0
#   Interfaces added:      CURRENT++, AGE++,   REVISION = 0
#   No interfaces changed:                     REVISION++
#
#
AC_SUBST(LIBNLUNIT_TEST_VERSION_CURRENT,  [1])
AC_SUBST(LIBNLUNIT_TEST_VERSION_AGE,      [0])
AC_SUBST(LIBNLUNIT_TEST_VERSION_REVISION, [0])
AC_SUBST(LIBNLUNIT_TEST_VERSION_INFO,     [${LIBNLUNIT_TEST_VERSION_CURRENT}:${LIBNLUNIT_TEST_VERSION_REVISION}:${LIBNLUNIT_TEST_VERSION_AGE}])

#
# Check the sanity of the source directory by checking for the
# presence of a key watch file
#
AC_CONFIG_SRCDIR([src/nlunit-test.h])

#
# Tell autoconf where to find auxilliary build tools (e.g. config.guess,
# install-sh, missing, etc.)
#
AC_CONFIG_AUX_DIR([third_party/nlbuild-autotools/repo/third_party/autoconf])

#
# Tell autoconf where to find auxilliary M4 macros
#
AC_CONFIG_MACRO_DIRS([third_party/nlbuild-autotools/repo/third_party/autoconf/m4 third_party/nlbuild-autotools/repo/autoconf/m4])

#
# Tell autoconf what file the package is using to aggregate C preprocessor
# defines.
#
AC_CONFIG_HEADERS([src/nlunit-test-config.h])

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

# Check for other compiler toolchain tools.

AC_CHECK_TOOL(AR, ar)
AC_CHECK_TOOL(RANLIB, ranlib)
AC_CHECK_TOOL(OBJCOPY, objcopy)
AC_CHECK_TOOL(STRIP, strip)

# Check for other host tools.

AC_PROG_INSTALL
AC_PROG_LN_S

AC_PATH_PROG(CMP, cmp)
AC_PATH_PROG(PERL, perl)

#
# Checks for specific compiler characteristics
#

#
# Common compiler flags we would like to have.
#
#   -Wall                        CC, CXX
#

PROSPECTIVE_CFLAGS="-Wall"

AX_CHECK_COMPILER_OPTIONS([C],   ${PROSPECTIVE_CFLAGS})

# Check for and initialize libtool

LT_INIT

#
# Debug instances
#
AC_MSG_NOTICE([checking whether to build debug instances])

# Debug

NL_ENABLE_DEBUG([no])

AM_CONDITIONAL([NLUNIT_TEST_BUILD_DEBUG], [test "${nl_cv_build_debug}" = "yes"])

# Optimization

NL_ENABLE_OPTIMIZATION([yes])

AM_CONDITIONAL([NLUNIT_TEST_BUILD_OPTIMIZED], [test "${nl_cv_build_optimized}" = "yes"])

#
# Documentation
#

# Determine whether or not documentation (via Doxygen) should be built
# or not, with 'auto' as the default and establish a default support
# value for GraphViz 'dot' support.

NL_ENABLE_DOCS([auto],[NO])

AM_CONDITIONAL(NLUNIT_TEST_BUILD_DOCS, [test "${nl_cv_build_docs}" = "yes"])

#
# Check for headers
#
AC_HEADER_STDBOOL
AC_HEADER_STDC

# At this point, we can restore the compiler flags to whatever the
# user passed in, now that we're clear of an -Werror issues by
# transforming -Wno-error back to -Werror.

NL_RESTORE_WERROR

#
# Identify the various makefiles and auto-generated files for the package
#
AC_CONFIG_FILES([
Makefile
src/Makefile
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
  Interface                                 : ${LIBNLUNIT_TEST_VERSION_INFO//:/.}
  Build system                              : ${build}
  Host system                               : ${host}
  Target system                             : ${target}
  Target architecture                       : ${target_cpu}
  Target OS                                 : ${target_os}
  Cross compiling                           : ${cross_compiling}
  Build shared libraries                    : ${enable_shared}
  Build static libraries                    : ${enable_static}
  Build optimized libraries                 : ${nl_cv_build_optimized}
  Prefix                                    : ${prefix}
  Documentation support                     : ${nl_cv_build_docs}
  Doxygen                                   : ${DOXYGEN:--}
  GraphViz dot                              : ${DOT:--}
  PERL                                      : ${PERL:--}
  C Preprocessor                            : ${CPP}
  C Compiler                                : ${CC}
  Archiver                                  : ${AR}
  Archive Indexer                           : ${RANLIB}
  Symbol Stripper                           : ${STRIP}
  Object Copier                             : ${OBJCOPY}
  C Preprocessor flags                      : ${CPPFLAGS:--}
  C Compile flags                           : ${CFLAGS:--}
  Link flags                                : ${LDFLAGS:--}
  Link libraries                            : ${LIBS}

])



