#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

#
#  Copyright (c) 2016, The OpenThread Authors.
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
AC_CONFIG_SRCDIR([include/openthread/error.h])

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
AC_CONFIG_HEADERS([include/openthread-config-generic.h])

#
# Figure out what the canonical build and host tuples are.
#
AC_CANONICAL_BUILD
AC_CANONICAL_HOST

#
# Mac OS X / Darwin ends up putting some versioning cruft on the end of its
# tuple that we don't care about in this script. Create "clean" variables
# devoid of it.
#

NL_FILTERED_CANONICAL_BUILD
NL_FILTERED_CANONICAL_HOST

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
# Host-os-specific checks
#

case ${host_os} in

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

# This is a hack to restore some old broken behavior that was
# removed in pull request #1527. It's use is highly discouraged,
# you should try to fix your build environment instead.
AC_ARG_ENABLE(no-executables-hack,
    [AS_HELP_STRING([--enable-no-executables-hack],
        [Enable hack that prevents link checks at configure time. Highly discouraged.])])
AC_MSG_CHECKING([whether to disable executable checking])
if test "${enable_no_executables_hack}" = "yes"
then
    AC_MSG_RESULT([yes])
    AC_NO_EXECUTABLES
    # Here we guess conservative values for tests that require link checks
    # to test for these features. This will prevent these checks from
    # being performed later in the configuration process.
    ac_cv_func_strlcat=${ac_cv_func_strlcat-no}
    ac_cv_func_strlcpy=${ac_cv_func_strlcpy-no}
    ac_cv_func_strnlen=${ac_cv_func_strnlen-no}
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

AM_PROG_AS

# Check for other compiler toolchain tools.

AC_CHECK_TOOL(AR, ar)
AC_CHECK_TOOL(RANLIB, ranlib)
AC_CHECK_TOOL(OBJCOPY, objcopy)
AC_CHECK_TOOL(STRIP, strip)

# Check for other host tools.

AC_PROG_INSTALL
AC_PROG_LN_S

AC_PATH_PROG(CMP, cmp)

#
# Checks for specific compiler characteristics
#

#
# Common compiler flags we would like to have.
#
#   -Wall                        CC, CXX
#

PROSPECTIVE_CFLAGS="-Wall -Wextra -Wshadow -Werror -std=c99 -pedantic-errors"
PROSPECTIVE_CXXFLAGS="-Wall -Wextra -Wshadow -Werror -std=gnu++98 -Wno-c++14-compat -fno-exceptions"

AC_CACHE_CHECK([whether $CC is Clang],
    [nl_cv_clang],
    [nl_cv_clang=no
    if test "x${GCC}" = "xyes"; then
        AC_EGREP_CPP([NL_CC_IS_CLANG],
            [/* Note: Clang 2.7 lacks __clang_[a-z]+__ */
#            if defined(__clang__) && defined(__llvm__)
             NL_CC_IS_CLANG
#            endif
            ],
            [nl_cv_clang=yes])
    fi
    ])

if test "${nl_cv_clang}" = "yes"; then
    PROSPECTIVE_CFLAGS="${PROSPECTIVE_CFLAGS} -Wconversion"
    PROSPECTIVE_CXXFLAGS="${PROSPECTIVE_CXXFLAGS} -Wconversion"
fi

AX_CHECK_COMPILER_OPTIONS([C],   ${PROSPECTIVE_CFLAGS})
AX_CHECK_COMPILER_OPTIONS([C++], ${PROSPECTIVE_CXXFLAGS})

# Check for and initialize libtool

LT_INIT
AC_PROG_LIBTOOL

# Disable building shared libraries by default (can be enabled with --enable-shared)

AC_DISABLE_SHARED

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

# Fuzz Targets

AC_MSG_CHECKING([whether to build fuzz targets])
AC_ARG_ENABLE(fuzz-targets,
    [AS_HELP_STRING([--enable-fuzz-targets],[Enable building of fuzz targets @<:@default=no@:>@.])],
    [
        case "${enableval}" in

        no|yes)
            enable_fuzz_targets=${enableval}
            ;;

        *)
            AC_MSG_ERROR([Invalid value ${enableval} for --enable-fuzz-targets])
            ;;

        esac
    ],
    [enable_fuzz_targets=no])
AC_MSG_RESULT(${enable_fuzz_targets})

AM_CONDITIONAL([OPENTHREAD_ENABLE_FUZZ_TARGETS], [test "${enable_fuzz_targets}" = "yes"])

if test "${enable_fuzz_targets}" = "no" ; then
    PROSPECTIVE_CXXFLAGS="-fno-rtti"
    AX_CHECK_COMPILER_OPTIONS([C++], ${PROSPECTIVE_CXXFLAGS})
fi

# Address Sanitizer

AC_MSG_CHECKING([whether to build with Address Sanitizer support])
AC_ARG_ENABLE(address-sanitizer,
    [AS_HELP_STRING([--enable-address-sanitizer],[Enable Address Sanitizer support @<:@default=no@:>@.])],
    [
        case "${enableval}" in

        no|yes)
            enable_address_sanitizer=${enableval}
            ;;

        *)
            AC_MSG_ERROR([Invalid value ${enableval} for --enable-address-sanitizer])
            ;;

        esac
    ],
    [enable_address_sanitizer=no])
AC_MSG_RESULT(${enable_address_sanitizer})

AM_CONDITIONAL([OPENTHREAD_WITH_ADDRESS_SANITIZER], [test "${enable_address_sanitizer}" = "yes"])

if test "${enable_address_sanitizer}" = "yes" ; then

    PROSPECTIVE_CFLAGS="-fsanitize=address"

    # Check if the compilers support address sanitizer
    AX_CHECK_COMPILER_OPTIONS([C],   ${PROSPECTIVE_CFLAGS})
    AX_CHECK_COMPILER_OPTIONS([C++], ${PROSPECTIVE_CFLAGS})

fi

#
# Code style
#

AC_SUBST(PRETTY, ["\${abs_top_srcdir}/script/clang-format.sh"])
AC_SUBST(PRETTY_ARGS, ["-style=file -i"])
AC_SUBST(PRETTY_CHECK, ["\${abs_top_srcdir}/script/clang-format-check.sh"])
AC_SUBST(PRETTY_CHECK_ARGS, [""])

#
# Tests
#
AC_MSG_NOTICE([checking whether to build tests])

# Tests

NL_ENABLE_TESTS([yes])

AM_CONDITIONAL([OPENTHREAD_BUILD_TESTS], [test "${nl_cv_build_tests}" = "yes"])

#
# Multiple OpenThread Instances
#

AC_ARG_ENABLE(multiple-instances,
    [AS_HELP_STRING([--enable-multiple-instances],[Enable support for multiple OpenThread instances @<:@default=no@:>@.])],
    [
        case "${enableval}" in

        no|yes)
            enable_multiple_instances=${enableval}
            ;;

        *)
            AC_MSG_ERROR([Invalid value ${enable_multiple_instances} for --enable-multiple-instances])
            ;;
        esac
    ],
    [enable_multiple_instances=no])

if test "$enable_multiple_instances" = "yes"; then
    OPENTHREAD_ENABLE_MULTIPLE_INSTANCES=1
else
    OPENTHREAD_ENABLE_MULTIPLE_INSTANCES=0
fi

AC_MSG_RESULT(${enable_multiple_instances})
AC_SUBST(OPENTHREAD_ENABLE_MULTIPLE_INSTANCES)
AM_CONDITIONAL([OPENTHREAD_ENABLE_MULTIPLE_INSTANCES], [test "${enable_multiple_instances}" = "yes"])
AC_DEFINE_UNQUOTED([OPENTHREAD_ENABLE_MULTIPLE_INSTANCES],[${OPENTHREAD_ENABLE_MULTIPLE_INSTANCES}],[Define to 1 if you want to enable support for multiple OpenThread instances.])

#
# Builtin mbedtls
#

AC_ARG_ENABLE(builtin-mbedtls,
    [AS_HELP_STRING([--enable-builtin-mbedtls],[Enable builtin mbedtls @<:@default=yes@:>@.])],
    [
        case "${enableval}" in

        no|yes)
            enable_builtin_mbedtls=${enableval}
            ;;

        *)
            AC_MSG_ERROR([Invalid value ${enable_builtin_mbedtls} for --enable-builtin-mbedtls])
            ;;
        esac
    ],
    [enable_builtin_mbedtls=yes])

if test "$enable_builtin_mbedtls" = "yes" -a ! "${MBEDTLS_CPPFLAGS}"; then
    MBEDTLS_CPPFLAGS="-I\${abs_top_srcdir}/third_party/mbedtls"
    MBEDTLS_CPPFLAGS="${MBEDTLS_CPPFLAGS} -I\${abs_top_srcdir}/third_party/mbedtls/repo/include"
    MBEDTLS_CPPFLAGS="${MBEDTLS_CPPFLAGS} -DMBEDTLS_CONFIG_FILE=\\\"mbedtls-config.h\\\""
fi
AC_MSG_CHECKING([whether mbed TLS should be enabled])
AC_MSG_RESULT(${enable_builtin_mbedtls})
AM_CONDITIONAL([OPENTHREAD_ENABLE_BUILTIN_MBEDTLS], [test "${enable_builtin_mbedtls}" = "yes"])

#
# POSIX Application
#

AC_MSG_CHECKING([whether to build POSIX applicaton])
AC_ARG_ENABLE(posix-app,
    [AS_HELP_STRING([--enable-posix-app], [Build POSIX application @<:@default=no@:>@.])],
    [
        case "${enableval}" in

        no|yes)
            enable_posix_app=${enableval}
            ;;

        *)
            AC_MSG_ERROR([Invalid value ${enable_posix_app} for --enable-posix-app])
            ;;
        esac
    ],
    [enable_posix_app=no])

if test "$enable_posix_app" = "yes"; then
    OPENTHREAD_ENABLE_POSIX_APP=1
else
    OPENTHREAD_ENABLE_POSIX_APP=0
fi

AC_MSG_RESULT(${enable_posix_app})
AC_SUBST(OPENTHREAD_ENABLE_POSIX_APP)
AM_CONDITIONAL([OPENTHREAD_ENABLE_POSIX_APP], [test "${enable_posix_app}" = "yes"])
AC_DEFINE_UNQUOTED([OPENTHREAD_ENABLE_POSIX_APP], [${OPENTHREAD_ENABLE_POSIX_APP}], [Define to 1 to build posix application.])

#
# FTD Library
#

AC_MSG_CHECKING([whether to build FTD library])
AC_ARG_ENABLE(ftd,
    [AS_HELP_STRING([--enable-ftd], [Build FTD library @<:@default=no@:>@.])],
    [
        case "${enableval}" in

        no|yes)
            enable_ftd=${enableval}
            ;;

        *)
            AC_MSG_ERROR([Invalid value ${enable_ftd} for --enable-ftd])
            ;;
        esac
    ],
    [enable_ftd=no])

if test "$enable_ftd" = "yes"; then
    OPENTHREAD_ENABLE_FTD=1
else
    OPENTHREAD_ENABLE_FTD=0
fi

AC_MSG_RESULT(${enable_ftd})
AC_SUBST(OPENTHREAD_ENABLE_FTD)
AM_CONDITIONAL([OPENTHREAD_ENABLE_FTD], [test "${enable_ftd}" = "yes"])
AC_DEFINE_UNQUOTED([OPENTHREAD_ENABLE_FTD], [${OPENTHREAD_ENABLE_FTD}], [Define to 1 to build FTD library.])

#
# MTD Library
#

AC_MSG_CHECKING([whether to build MTD library])
AC_ARG_ENABLE(mtd,
    [AS_HELP_STRING([--enable-mtd], [Build MTD library @<:@default=no@:>@.])],
    [
        case "${enableval}" in

        no|yes)
            enable_mtd=${enableval}
            ;;

        *)
            AC_MSG_ERROR([Invalid value ${enable_mtd} for --enable-mtd])
            ;;
        esac
    ],
    [enable_mtd=no])

if test "$enable_mtd" = "yes"; then
    OPENTHREAD_ENABLE_MTD=1
else
    OPENTHREAD_ENABLE_MTD=0
fi

AC_MSG_RESULT(${enable_mtd})
AC_SUBST(OPENTHREAD_ENABLE_MTD)
AM_CONDITIONAL([OPENTHREAD_ENABLE_MTD], [test "${enable_mtd}" = "yes"])
AC_DEFINE_UNQUOTED([OPENTHREAD_ENABLE_MTD], [${OPENTHREAD_ENABLE_MTD}], [Define to 1 to build MTD library.])

#
# Radio Library
#

AC_MSG_CHECKING([whether to build radio-only library])
AC_ARG_ENABLE(radio-only,
    [AS_HELP_STRING([--enable-radio-only], [Build radio-only library @<:@default=no@:>@.])],
    [
        case "${enableval}" in

        no|yes)
            enable_radio_only=${enableval}
            ;;

        *)
            AC_MSG_ERROR([Invalid value ${enable_radio_only} for --enable-radio-only])
            ;;
        esac
    ],
    [enable_radio_only=no])

if test "$enable_radio_only" = "yes"; then
    OPENTHREAD_ENABLE_RADIO_ONLY=1
else
    OPENTHREAD_ENABLE_RADIO_ONLY=0
fi

AC_MSG_RESULT(${enable_radio_only})
AC_SUBST(OPENTHREAD_ENABLE_RADIO_ONLY)
AM_CONDITIONAL([OPENTHREAD_ENABLE_RADIO_ONLY], [test "${enable_radio_only}" = "yes"])
AC_DEFINE_UNQUOTED([OPENTHREAD_ENABLE_RADIO_ONLY], [${OPENTHREAD_ENABLE_RADIO_ONLY}], [Define to 1 to build radio-only library.])

#
# whether to build executables
#

AC_MSG_CHECKING([whether to build executables])
AC_ARG_ENABLE(executable,
    [AS_HELP_STRING([--enable-executable], [Build executables @<:@default=yes@:>@.])],
    [
        case "${enableval}" in

        no|yes)
            enable_executable=${enableval}
            ;;

        *)
            AC_MSG_ERROR([Invalid value ${enable_executable} for --enable-executable])
            ;;
        esac
    ],
    [enable_executable=yes])

if test "$enable_executable" = "yes"; then
    OPENTHREAD_ENABLE_EXECUTABLE=1
else
    OPENTHREAD_ENABLE_EXECUTABLE=0
fi

AC_MSG_RESULT(${enable_executable})
AC_SUBST(OPENTHREAD_ENABLE_EXECUTABLE)
AM_CONDITIONAL([OPENTHREAD_ENABLE_EXECUTABLE], [test "${enable_executable}" = "yes"])
AC_DEFINE_UNQUOTED([OPENTHREAD_ENABLE_EXECUTABLE], [${OPENTHREAD_ENABLE_EXECUTABLE}], [Define to 1 to build executables.])

#
# CLI Library
#

AC_MSG_CHECKING([whether to build CLI library])
AC_ARG_ENABLE(cli,
    [AS_HELP_STRING([--enable-cli], [Build CLI library @<:@default=no@:>@.])],
    [
        case "${enableval}" in

        no|yes)
            enable_cli=${enableval}
            ;;

        *)
            AC_MSG_ERROR([Invalid value ${enable_cli} for --enable-cli])
            ;;
        esac
    ],
    [enable_cli=no])

if test "$enable_cli" = "yes"; then
    OPENTHREAD_ENABLE_CLI=1
else
    OPENTHREAD_ENABLE_CLI=0
fi

AC_MSG_RESULT(${enable_cli})
AC_SUBST(OPENTHREAD_ENABLE_CLI)
AM_CONDITIONAL([OPENTHREAD_ENABLE_CLI], [test "${enable_cli}" = "yes"])
AC_DEFINE_UNQUOTED([OPENTHREAD_ENABLE_CLI], [${OPENTHREAD_ENABLE_CLI}], [Define to 1 to build CLI library.])

#
# NCP Library
#

AC_MSG_CHECKING([whether to build NCP library])
AC_ARG_ENABLE(ncp,
    [AS_HELP_STRING([--enable-ncp], [Build NCP library @<:@default=no@:>@.])],
    [
        case "${enableval}" in

        no|yes)
            enable_ncp=${enableval}
            ;;

        *)
            AC_MSG_ERROR([Invalid value ${enable_ncp} for --enable-ncp])
            ;;
        esac
    ],
    [enable_ncp=no])

if test "$enable_ncp" = "yes"; then
    OPENTHREAD_ENABLE_NCP=1
else
    OPENTHREAD_ENABLE_NCP=0
fi

AC_MSG_RESULT(${enable_ncp})
AC_SUBST(OPENTHREAD_ENABLE_NCP)
AM_CONDITIONAL([OPENTHREAD_ENABLE_NCP], [test "${enable_ncp}" = "yes"])
AC_DEFINE_UNQUOTED([OPENTHREAD_ENABLE_NCP], [${OPENTHREAD_ENABLE_NCP}], [Define to 1 to build NCP library.])

#
# NCP BUS - how does the NCP talk to the host?
#

AC_ARG_WITH(
    [ncp-bus],
    [AS_HELP_STRING([--with-ncp-bus],[Specify the NCP bus (none|spi|uart) @<:@default=none@:>@.])],
    [
        case "${with_ncp_bus}" in
        "none")
            OPENTHREAD_ENABLE_NCP_SPI=0
            OPENTHREAD_ENABLE_NCP_UART=0
            ;;
        "spi")
            OPENTHREAD_ENABLE_NCP_SPI=1
            OPENTHREAD_ENABLE_NCP_UART=0
            ;;
        "uart")
            OPENTHREAD_ENABLE_NCP_SPI=0
            OPENTHREAD_ENABLE_NCP_UART=1
            ;;
        *)
            AC_MSG_ERROR([unexpected --with-ncp-bus=${with_ncp_bus}])
            ;;
        esac
    ],
    [
       OPENTHREAD_ENABLE_NCP_SPI=0
       OPENTHREAD_ENABLE_NCP_UART=0
    ])

AC_SUBST(OPENTHREAD_ENABLE_NCP_SPI)
AM_CONDITIONAL([OPENTHREAD_ENABLE_NCP_SPI],  [test "${with_ncp_bus}"  = "spi"])
AC_DEFINE_UNQUOTED([OPENTHREAD_ENABLE_NCP_SPI],[${OPENTHREAD_ENABLE_NCP_SPI}],[Define to 1 to enable the NCP SPI interface.])
AC_MSG_CHECKING([should NCP support SPI])
AC_MSG_RESULT([${OPENTHREAD_ENABLE_SPI}])

AC_SUBST(OPENTHREAD_ENABLE_NCP_UART)
AM_CONDITIONAL([OPENTHREAD_ENABLE_NCP_UART], [test "${with_ncp_bus}"  = "uart"])
AC_DEFINE_UNQUOTED([OPENTHREAD_ENABLE_NCP_UART],[${OPENTHREAD_ENABLE_NCP_UART}],[Define to 1 to enable the NCP UART interface.])
AC_MSG_CHECKING([should NCP support UART])
AC_MSG_RESULT([${OPENTHREAD_ENABLE_UART}])

#
# Thread UDP forward
#

AC_MSG_CHECKING([whether to enable UDP forward])
AC_ARG_ENABLE(udp_forward,
    [AS_HELP_STRING([--enable-udp-forward], [Enable UDP forward support @<:@default=no@:>@.])],
    [
        case "${enableval}" in

        no|yes)
            enable_udp_forward=${enableval}
            ;;

        *)
            AC_MSG_ERROR([Invalid value ${enable_udp_forward} for --enable-udp-forward])
            ;;
        esac
    ],
    [enable_udp_forward=no])

if test "$enable_udp_forward" = "yes"; then
    OPENTHREAD_ENABLE_UDP_FORWARD=1
else
    OPENTHREAD_ENABLE_UDP_FORWARD=0
fi

AC_MSG_RESULT(${enable_udp_forward})
AC_SUBST(OPENTHREAD_ENABLE_UDP_FORWARD)
AM_CONDITIONAL([OPENTHREAD_ENABLE_UDP_FORWARD], [test "${enable_udp_forward}" = "yes"])
AC_DEFINE_UNQUOTED([OPENTHREAD_ENABLE_UDP_FORWARD], [${OPENTHREAD_ENABLE_UDP_FORWARD}], [Define to 1 to enable the UDP forward feature.])

#
# Thread Border Agent
#

AC_MSG_CHECKING([whether to enable border agent])
AC_ARG_ENABLE(border_agent,
    [AS_HELP_STRING([--enable-border-agent], [Enable border agent support @<:@default=no@:>@.])],
    [
        case "${enableval}" in

        no|yes)
            enable_border_agent=${enableval}
            ;;

        *)
            AC_MSG_ERROR([Invalid value ${enable_border_agent} for --enable-border-agent])
            ;;
        esac
    ],
    [enable_border_agent=no])

if test "${enable_border_agent}" = "yes"; then
    OPENTHREAD_ENABLE_BORDER_AGENT=1
else
    OPENTHREAD_ENABLE_BORDER_AGENT=0
fi

AC_MSG_RESULT(${enable_border_agent})
AC_SUBST(OPENTHREAD_ENABLE_BORDER_AGENT)
AM_CONDITIONAL([OPENTHREAD_ENABLE_BORDER_AGENT], [test "${enable_border_agent}" = "yes"])
AC_DEFINE_UNQUOTED([OPENTHREAD_ENABLE_BORDER_AGENT], [${OPENTHREAD_ENABLE_BORDER_AGENT}], [Define to 1 to enable the border agent feature.])

#
# Thread Network Diagnostic for MTD
#

AC_ARG_ENABLE(mtd_network_diagnostic,
    [AS_HELP_STRING([--enable-mtd-network-diagnostic],[Enable network diagnostic support for MTD @<:@default=no@:>@.])],
    [
        case "${enableval}" in

        no|yes)
            enable_mtd_network_diagnostic=${enableval}
            ;;

        *)
            AC_MSG_ERROR([Invalid value ${enable_mtd_network_diagnostic} for --enable-mtd-network-diagnostic])
            ;;
        esac
    ],
    [enable_mtd_network_diagnostic=no])

if test "$enable_mtd_network_diagnostic" = "yes"; then
    OPENTHREAD_ENABLE_MTD_NETWORK_DIAGNOSTIC=1
else
    OPENTHREAD_ENABLE_MTD_NETWORK_DIAGNOSTIC=0
fi

AC_MSG_CHECKING([whether to enable the network diagnostic for MTD])
AC_MSG_RESULT(${enable_mtd_network_diagnostic})
AC_SUBST(OPENTHREAD_ENABLE_MTD_NETWORK_DIAGNOSTIC)
AM_CONDITIONAL([OPENTHREAD_ENABLE_MTD_NETWORK_DIAGNOSTIC], [test "${enable_mtd_network_diagnostic}" = "yes"])
AC_DEFINE_UNQUOTED([OPENTHREAD_ENABLE_MTD_NETWORK_DIAGNOSTIC],[${OPENTHREAD_ENABLE_MTD_NETWORK_DIAGNOSTIC}],[Define to 1 to enable network diagnostic for MTD.])

#
# Application CoAP Secure
#

AC_ARG_ENABLE(application_coap_secure,
    [AS_HELP_STRING([--enable-application-coap-secure],[Enable CoAP Secure to an application.@<:@default=no@:>@.])],
    [
        case "${enableval}" in

        no|yes)
            enable_application_coap_secure=${enableval}
            ;;

        *)
            AC_MSG_ERROR([Invalid value ${enable_application_coap_secure} for --enable-application-coap_secure])
            ;;
        esac
    ],
    [enable_application_coap_secure=no])

if test "$enable_application_coap_secure" = "yes"; then
    OPENTHREAD_ENABLE_APPLICATION_COAP_SECURE=1
    enable_application_coap="yes"
else
    OPENTHREAD_ENABLE_APPLICATION_COAP_SECURE=0
fi

AC_SUBST(OPENTHREAD_ENABLE_APPLICATION_COAP_SECURE)
AM_CONDITIONAL([OPENTHREAD_ENABLE_APPLICATION_COAP_SECURE], [test "${enable_application_coap_secure}" = "yes"])
AC_DEFINE_UNQUOTED([OPENTHREAD_ENABLE_APPLICATION_COAP_SECURE],[${OPENTHREAD_ENABLE_APPLICATION_COAP_SECURE}],[Define to 1 if you want to enable CoAP Secure to an application.])

#
# Platform UDP
#

AC_ARG_ENABLE(platform_udp,
    [AS_HELP_STRING([--enable-platform-udp],[Enable platform UDP support @<:@default=no@:>@.])],
    [
        case "${enableval}" in
        no|yes)
            enable_platform_udp=${enableval}
            ;;
        *)
            AC_MSG_ERROR([Invalid value ${enable_platform_udp} for --enable-platform-udp])
            ;;
        esac
    ],
    [enable_platform_udp=no])

if test "$enable_platform_udp" = "yes"; then
    OPENTHREAD_ENABLE_PLATFORM_UDP=1
else
    OPENTHREAD_ENABLE_PLATFORM_UDP=0
fi

AC_MSG_RESULT(${enable_platform_udp})
AC_SUBST(OPENTHREAD_ENABLE_PLATFORM_UDP)
AM_CONDITIONAL([OPENTHREAD_ENABLE_PLATFORM_UDP], [test "${enable_platform_udp}" = "yes"])
AC_DEFINE_UNQUOTED([OPENTHREAD_ENABLE_PLATFORM_UDP], [${OPENTHREAD_ENABLE_PLATFORM_UDP}], [Define to 1 to enable platform UDP.])

#
# Platform Netif
#

AC_ARG_ENABLE(platform_netif,
    [AS_HELP_STRING([--enable-platform-netif],[Enable platform Netif support @<:@default=no@:>@.])],
    [
        case "${enableval}" in
        no|yes)
            enable_platform_netif=${enableval}
            ;;
        *)
            AC_MSG_ERROR([Invalid value ${enable_platform_netif} for --enable-platform-netif])
            ;;
        esac
    ],
    [enable_platform_netif=no])

if test "$enable_platform_netif" = "yes"; then
    OPENTHREAD_ENABLE_PLATFORM_NETIF=1
else
    OPENTHREAD_ENABLE_PLATFORM_NETIF=0
fi

AC_MSG_RESULT(${enable_platform_netif})
AC_SUBST(OPENTHREAD_ENABLE_PLATFORM_NETIF)
AM_CONDITIONAL([OPENTHREAD_ENABLE_PLATFORM_NETIF], [test "${enable_platform_netif}" = 'yes'])
AC_DEFINE_UNQUOTED([OPENTHREAD_ENABLE_PLATFORM_NETIF], [${OPENTHREAD_ENABLE_PLATFORM_NETIF}], [Define to 1 to enable platform Netif.])

#
# Thread Commissioner
#

AC_ARG_ENABLE(commissioner,
    [AS_HELP_STRING([--enable-commissioner],[Enable commissioner support @<:@default=no@:>@.])],
    [
        case "${enableval}" in

        no|yes)
            enable_commissioner=${enableval}
            ;;

        *)
            AC_MSG_ERROR([Invalid value ${enable_commissioner} for --enable-commissioner])
            ;;
        esac
    ],
    [enable_commissioner=no])

if test "$enable_commissioner" = "yes"; then
    OPENTHREAD_ENABLE_COMMISSIONER=1
else
    OPENTHREAD_ENABLE_COMMISSIONER=0
fi

AC_MSG_CHECKING([whether to enable the commissioner])
AC_MSG_RESULT(${enable_commissioner})
AC_SUBST(OPENTHREAD_ENABLE_COMMISSIONER)
AM_CONDITIONAL([OPENTHREAD_ENABLE_COMMISSIONER], [test "${enable_commissioner}" = "yes"])
AC_DEFINE_UNQUOTED([OPENTHREAD_ENABLE_COMMISSIONER],[${OPENTHREAD_ENABLE_COMMISSIONER}],[Define to 1 to enable the commissioner role.])

#
# Thread Joiner
#

AC_ARG_ENABLE(joiner,
    [AS_HELP_STRING([--enable-joiner],[Enable joiner support @<:@default=no@:>@.])],
    [
        case "${enableval}" in

        no|yes)
            enable_joiner=${enableval}
            ;;

        *)
            AC_MSG_ERROR([Invalid value ${enable_joiner} for --enable-joiner])
            ;;
        esac
    ],
    [enable_joiner=no])

if test "$enable_joiner" = "yes"; then
    OPENTHREAD_ENABLE_JOINER=1
else
    OPENTHREAD_ENABLE_JOINER=0
fi

AC_MSG_CHECKING([whether to enable the joiner feature])
AC_MSG_RESULT(${enable_joiner})
AC_SUBST(OPENTHREAD_ENABLE_JOINER)
AM_CONDITIONAL([OPENTHREAD_ENABLE_JOINER], [test "${enable_joiner}" = "yes"])
AC_DEFINE_UNQUOTED([OPENTHREAD_ENABLE_JOINER],[${OPENTHREAD_ENABLE_JOINER}],[Define to 1 to enable the joiner role.])

if test "${enable_commissioner}" = "yes" -o "${enable_joiner}" = "yes" -o "${enable_border_agent}" = "yes" -o "${enable_application_coap_secure}" = "yes"; then
    enable_dtls="yes"
    OPENTHREAD_ENABLE_DTLS=1
else
    enable_dtls="no"
    OPENTHREAD_ENABLE_DTLS=0
fi

AC_MSG_CHECKING([whether to enable DTLS due to joiner/commissioner])
AC_MSG_RESULT(${enable_dtls})
AC_SUBST(OPENTHREAD_ENABLE_DTLS)
AM_CONDITIONAL([OPENTHREAD_ENABLE_DTLS], [test "${enable_dtls}" = "yes"])
AC_DEFINE_UNQUOTED([OPENTHREAD_ENABLE_DTLS],[${OPENTHREAD_ENABLE_DTLS}],[Define to 1 to enable dtls support.])

#
# Jam Detection
#

AC_ARG_ENABLE(jam_detection,
    [AS_HELP_STRING([--enable-jam-detection],[Enable Jam Detection support @<:@default=no@:>@.])],
    [
        case "${enableval}" in

        no|yes)
            enable_jam_detection=${enableval}
            ;;

        *)
            AC_MSG_ERROR([Invalid value ${enable_jam_detection} for --enable-jam-detection])
            ;;
        esac
    ],
    [enable_jam_detection=no])

if test "$enable_jam_detection" = "yes"; then
    OPENTHREAD_ENABLE_JAM_DETECTION=1
else
    OPENTHREAD_ENABLE_JAM_DETECTION=0
fi

AC_MSG_CHECKING([whether to enable jam detection])
AC_MSG_RESULT(${enable_jam_detection})
AC_SUBST(OPENTHREAD_ENABLE_JAM_DETECTION)
AM_CONDITIONAL([OPENTHREAD_ENABLE_JAM_DETECTION], [test "${enable_jam_detection}" = "yes"])
AC_DEFINE_UNQUOTED([OPENTHREAD_ENABLE_JAM_DETECTION],[${OPENTHREAD_ENABLE_JAM_DETECTION}],[Define to 1 if you want to use jam detection feature])

#
# Channel Monitor
#

AC_ARG_ENABLE(channel_monitor,
    [AS_HELP_STRING([--enable-channel-monitor],[Enable Channel Monitor support @<:@default=no@:>@.])],
    [
        case "${enableval}" in

        no|yes)
            enable_channel_monitor=${enableval}
            ;;

        *)
            AC_MSG_ERROR([Invalid value ${enable_channel_monitor} for --enable-channel-monitor])
            ;;
        esac
    ],
    [enable_channel_monitor=no])

if test "$enable_channel_monitor" = "yes"; then
    OPENTHREAD_ENABLE_CHANNEL_MONITOR=1
else
    OPENTHREAD_ENABLE_CHANNEL_MONITOR=0
fi

AC_MSG_CHECKING([whether to enable channel monitor])
AC_MSG_RESULT(${enable_channel_monitor})
AC_SUBST(OPENTHREAD_ENABLE_CHANNEL_MONITOR)
AM_CONDITIONAL([OPENTHREAD_ENABLE_CHANNEL_MONITOR], [test "${enable_channel_monitor}" = "yes"])
AC_DEFINE_UNQUOTED([OPENTHREAD_ENABLE_CHANNEL_MONITOR],[${OPENTHREAD_ENABLE_CHANNEL_MONITOR}],[Define to 1 if you want to use channel monitor feature])

#
# Channel Manager
#

AC_ARG_ENABLE(channel_manager,
    [AS_HELP_STRING([--enable-channel-manager],[Enable Channel Manager support @<:@default=no@:>@.])],
    [
        case "${enableval}" in

        no|yes)
            enable_channel_manager=${enableval}
            ;;

        *)
            AC_MSG_ERROR([Invalid value ${enable_channel_manager} for --enable-channel-manager])
            ;;
        esac
    ],
    [enable_channel_manager=no])

if test "$enable_channel_manager" = "yes"; then
    OPENTHREAD_ENABLE_CHANNEL_MANAGER=1
else
    OPENTHREAD_ENABLE_CHANNEL_MANAGER=0
fi

AC_MSG_CHECKING([whether to enable channel manager])
AC_MSG_RESULT(${enable_channel_manager})
AC_SUBST(OPENTHREAD_ENABLE_CHANNEL_MANAGER)
AM_CONDITIONAL([OPENTHREAD_ENABLE_CHANNEL_MANAGER], [test "${enable_channel_manager}" = "yes"])
AC_DEFINE_UNQUOTED([OPENTHREAD_ENABLE_CHANNEL_MANAGER],[${OPENTHREAD_ENABLE_CHANNEL_MANAGER}],[Define to 1 if you want to enable channel manager feature])

#
# MAC Filter (include AddressFilter and RssInFilter)
#

AC_ARG_ENABLE(mac_filter,
              [AS_HELP_STRING([--enable-mac-filter],[Enable MAC filter support @<:@default=no@:>@.])],
    [
        case "${enableval}" in

        no|yes)
            enable_mac_filter=${enableval}
            ;;

        *)
            AC_MSG_ERROR([Invalid value ${enable_mac_filter} for --enable-mac-filter])
            ;;
        esac
    ],
    [enable_mac_filter=no])

if test "$enable_mac_filter" = "yes"; then
    OPENTHREAD_ENABLE_MAC_FILTER=1
else
    OPENTHREAD_ENABLE_MAC_FILTER=0
fi

AC_MSG_CHECKING([whether to enable mac filter])
AC_MSG_RESULT(${enable_mac_filter})
AC_SUBST(OPENTHREAD_ENABLE_MAC_FILTER)
AM_CONDITIONAL([OPENTHREAD_ENABLE_MAC_FILTER], [test "${enable_mac_filter}" = "yes"])
AC_DEFINE_UNQUOTED([OPENTHREAD_ENABLE_MAC_FILTER],[${OPENTHREAD_ENABLE_MAC_FILTER}],[Define to 1 if you want to use MAC filter feature])

#
# Diagnostics Library
#

AC_ARG_ENABLE(diag,
    [AS_HELP_STRING([--enable-diag],[Enable diagnostics support @<:@default=no@:>@.])],
    [
        case "${enableval}" in

        no|yes)
            enable_diag=${enableval}
            ;;

        *)
            AC_MSG_ERROR([Invalid value ${enable_diag} for --enable-diag])
            ;;
        esac
    ],
    [enable_diag=no])

if test "$enable_diag" = "yes"; then
    OPENTHREAD_ENABLE_DIAG=1
else
    OPENTHREAD_ENABLE_DIAG=0
fi

AC_MSG_CHECKING([whether to enable diag])
AC_MSG_RESULT(${enable_diag})
AC_SUBST(OPENTHREAD_ENABLE_DIAG)
AM_CONDITIONAL([OPENTHREAD_ENABLE_DIAG], [test "${enable_diag}" = "yes"])
AC_DEFINE_UNQUOTED([OPENTHREAD_ENABLE_DIAG],[${OPENTHREAD_ENABLE_DIAG}],[Define to 1 if you want to use diagnostics module])

#
# Vendor Extension - Specify a C++ source file which will be built as part of OpenThread core library.
#

AC_ARG_WITH(
    [vendor-extension],
    [AS_HELP_STRING([--with-vendor-extension=<VENDOR_EXT.CPP>],[Specify a C++ source file built as part of OpenThread core library. @<:@default=none@:>@.])],
    [
        if test "${withval}" = "no"
        then OPENTHREAD_ENABLE_VENDOR_EXTENSION=0
        elif test '!' -f "${withval}"
        then AC_MSG_ERROR([Can't open ${withval} for --with-vendor-extension])
        else
            OPENTHREAD_ENABLE_VENDOR_EXTENSION=1

            # Get the absolute path.
            OPENTHREAD_VENDOR_EXTENSION_SOURCE=$(cd `dirname ${withval}` && pwd)/$(basename ${withval})
        fi
    ],
    [
        OPENTHREAD_ENABLE_VENDOR_EXTENSION=0
        with_vendor_extension=none
    ])

AC_MSG_CHECKING([for vendor extension])
AC_MSG_RESULT(${OPENTHREAD_VENDOR_EXTENSION_SOURCE-none})
AC_SUBST(OPENTHREAD_VENDOR_EXTENSION_SOURCE)
AC_DEFINE_UNQUOTED([OPENTHREAD_ENABLE_VENDOR_EXTENSION],[${OPENTHREAD_ENABLE_VENDOR_EXTENSION}],[Define to 1 if using vendor extension])
AM_CONDITIONAL([OPENTHREAD_ENABLE_VENDOR_EXTENSION], [test "${OPENTHREAD_ENABLE_VENDOR_EXTENSION}" = "1"])

#
# Legacy Network
#

AC_ARG_ENABLE(legacy,
    [AS_HELP_STRING([--enable-legacy],[Enable legacy network support @<:@default=no@:>@.])],
    [
        case "${enableval}" in

        no|yes)
            enable_legacy=${enableval}
            ;;

        *)
            AC_MSG_ERROR([Invalid value ${enable_legacy} for --enable-legacy])
            ;;
        esac
    ],
    [enable_legacy=no])

if test "$enable_legacy" = "yes"; then
    OPENTHREAD_ENABLE_LEGACY=1
else
    OPENTHREAD_ENABLE_LEGACY=0
fi

AC_MSG_CHECKING([whether to enable legacy])
AC_MSG_RESULT(${enable_legacy})
AC_SUBST(OPENTHREAD_ENABLE_LEGACY)
AM_CONDITIONAL([OPENTHREAD_ENABLE_LEGACY], [test "${enable_legacy}" = "yes"])
AC_DEFINE_UNQUOTED([OPENTHREAD_ENABLE_LEGACY],[${OPENTHREAD_ENABLE_LEGACY}],[Define to 1 if you want to use legacy network support])

#
# NCP Vendor Hook Source - Specify a C++ file that implements the NCP vendor hook.
#

AC_ARG_WITH(
    [ncp-vendor-hook-source],
    [AS_HELP_STRING([--with-ncp-vendor-hook-source=<VENDOR_HOOK.CPP>],[Specify a C++ file that implements the NCP vendor hook. @<:@default=none@:>@.])],
    [
        if test "${withval}" = "no"
        then OPENTHREAD_ENABLE_NCP_VENDOR_HOOK=0
        elif test '!' -f "${withval}"
        then AC_MSG_ERROR([Can't open ${withval} for --with-ncp-vendor-hook-source])
        else
            OPENTHREAD_ENABLE_NCP_VENDOR_HOOK=1

            # Get the absolute path.
            OPENTHREAD_NCP_VENDOR_HOOK_SOURCE=$(cd `dirname ${withval}` && pwd)/$(basename ${withval})
        fi
    ],
    [
        OPENTHREAD_ENABLE_NCP_VENDOR_HOOK=0
        with_ncp_vendor_hook_source=none
    ])

AC_MSG_CHECKING([for NCP vendor hook source])
AC_MSG_RESULT(${OPENTHREAD_NCP_VENDOR_HOOK_SOURCE-none})
AC_SUBST(OPENTHREAD_NCP_VENDOR_HOOK_SOURCE)
AC_DEFINE_UNQUOTED([OPENTHREAD_ENABLE_NCP_VENDOR_HOOK],[${OPENTHREAD_ENABLE_NCP_VENDOR_HOOK}],[Define to 1 if using NCP vendor hook])
AM_CONDITIONAL([OPENTHREAD_ENABLE_NCP_VENDOR_HOOK], [test "${OPENTHREAD_ENABLE_NCP_VENDOR_HOOK}" = "1"])

#
# NCP Spinel Encrypter
#

AC_ARG_WITH(
    [ncp-spinel-encrypter-libs],
    [AS_HELP_STRING([--with-ncp-spinel-encrypter-libs=<LIBSPINEL_ENCRYPTER.A>],[Specify library files (absolute paths) implementing the NCP Spinel Encrypter. @<:@default=none@:>@.])],
    [
        if test "${withval}" = "no"
        then OPENTHREAD_ENABLE_NCP_SPINEL_ENCRYPTER=0
        else
            OPENTHREAD_ENABLE_NCP_SPINEL_ENCRYPTER=1
            OPENTHREAD_NCP_SPINEL_ENCRYPTER_LIBS=${withval}
        fi
    ],
    [
        OPENTHREAD_ENABLE_NCP_SPINEL_ENCRYPTER=0
        with_ncp_spinel_encrypter_libs=none
    ])

AC_MSG_CHECKING([for NCP Spinel Encrypter])
AC_MSG_RESULT(${OPENTHREAD_NCP_SPINEL_ENCRYPTER_LIBS-none})
AC_SUBST(OPENTHREAD_NCP_SPINEL_ENCRYPTER_LIBS)
AC_DEFINE_UNQUOTED([OPENTHREAD_ENABLE_NCP_SPINEL_ENCRYPTER],[${OPENTHREAD_ENABLE_NCP_SPINEL_ENCRYPTER}],[Define to 1 if using NCP Spinel Encrypter])
AM_CONDITIONAL([OPENTHREAD_ENABLE_NCP_SPINEL_ENCRYPTER], [test "${OPENTHREAD_ENABLE_NCP_SPINEL_ENCRYPTER}" = "1"])

#
# Child Supervision
#

AC_ARG_ENABLE(child_supervision,
    [AS_HELP_STRING([--enable-child-supervision],[Enable child supervision feature @<:@default=no@:>@.])],
    [
        case "${enableval}" in

        no|yes)
            enable_child_supervision=${enableval}
            ;;

        *)
            AC_MSG_ERROR([Invalid value ${enable_child_supervision} for --enable-child-supervision])
            ;;
        esac
    ],
    [enable_child_supervision=no])

if test "$enable_child_supervision" = "yes"; then
    OPENTHREAD_ENABLE_CHILD_SUPERVISION=1
else
    OPENTHREAD_ENABLE_CHILD_SUPERVISION=0
fi

AC_MSG_RESULT(${enable_child_supervision})
AC_SUBST(OPENTHREAD_ENABLE_CHILD_SUPERVISION)
AM_CONDITIONAL([OPENTHREAD_ENABLE_CHILD_SUPERVISION], [test "${enable_child_supervision}" = "yes"])
AC_DEFINE_UNQUOTED([OPENTHREAD_ENABLE_CHILD_SUPERVISION],[${OPENTHREAD_ENABLE_CHILD_SUPERVISION}],[Define to 1 if you want to use child supervision feature])

#
# Log for certification test
#

AC_ARG_ENABLE(cert_log,
    [AS_HELP_STRING([--enable-cert-log],[Enable certification log support @<:@default=no@:>@.])],
    [
        case "${enableval}" in

        no|yes)
            enable_cert_log=${enableval}
            ;;

        *)
            AC_MSG_ERROR([Invalid value ${enable_cert_log} for --enable-cert-log])
            ;;
        esac
    ],
    [enable_cert_log=no])

if test "$enable_cert_log" = "yes"; then
    OPENTHREAD_ENABLE_CERT_LOG=1
else
    OPENTHREAD_ENABLE_CERT_LOG=0
fi

AC_SUBST(OPENTHREAD_ENABLE_CERT_LOG)
AM_CONDITIONAL([OPENTHREAD_ENABLE_CERT_LOG], [test "${enable_cert_log}" = "yes"])
AC_DEFINE_UNQUOTED([OPENTHREAD_ENABLE_CERT_LOG],[${OPENTHREAD_ENABLE_CERT_LOG}],[Define to 1 if you want to enable log for certification test])

#
# DHCPv6 Client
#

AC_ARG_ENABLE(dhcp6_client,
    [AS_HELP_STRING([--enable-dhcp6-client],[Enable DHCPv6 client support @<:@default=no@:>@.])],
    [
        case "${enableval}" in

        no|yes)
            enable_dhcp6_client=${enableval}
            ;;

        *)
            AC_MSG_ERROR([Invalid value ${enable_dhcp6_client} for --enable-dhcp6-client])
            ;;
        esac
    ],
    [enable_dhcp6_client=no])

if test "$enable_dhcp6_client" = "yes"; then
    OPENTHREAD_ENABLE_DHCP6_CLIENT=1
else
    OPENTHREAD_ENABLE_DHCP6_CLIENT=0
fi

AC_SUBST(OPENTHREAD_ENABLE_DHCP6_CLIENT)
AM_CONDITIONAL([OPENTHREAD_ENABLE_DHCP6_CLIENT], [test "${enable_dhcp6_client}" = "yes"])
AC_DEFINE_UNQUOTED([OPENTHREAD_ENABLE_DHCP6_CLIENT],[${OPENTHREAD_ENABLE_DHCP6_CLIENT}],[Define to 1 if you want to enable DHCPv6 Client])

#
# DHCPv6 Server
#

AC_ARG_ENABLE(dhcp6_server,
    [AS_HELP_STRING([--enable-dhcp6-server],[Enable DHCPv6 server support @<:@default=no@:>@.])],
    [
        case "${enableval}" in

        no|yes)
            enable_dhcp6_server=${enableval}
            ;;

        *)
            AC_MSG_ERROR([Invalid value ${enable_dhcp6_server} for --enable-dhcp6-server])
            ;;
        esac
    ],
    [enable_dhcp6_server=no])

if test "$enable_dhcp6_server" = "yes"; then
    OPENTHREAD_ENABLE_DHCP6_SERVER=1
else
    OPENTHREAD_ENABLE_DHCP6_SERVER=0
fi

AC_SUBST(OPENTHREAD_ENABLE_DHCP6_SERVER)
AM_CONDITIONAL([OPENTHREAD_ENABLE_DHCP6_SERVER], [test "${enable_dhcp6_server}" = "yes"])
AC_DEFINE_UNQUOTED([OPENTHREAD_ENABLE_DHCP6_SERVER],[${OPENTHREAD_ENABLE_DHCP6_SERVER}],[Define to 1 if you want to enable DHCPv6 Server])

#
# DNS Client
#

AC_ARG_ENABLE(dns_client,
    [AS_HELP_STRING([--enable-dns-client],[Enable DNS client support @<:@default=no@:>@.])],
    [
        case "${enableval}" in

        no|yes)
            enable_dns_client=${enableval}
            ;;

        *)
            AC_MSG_ERROR([Invalid value ${enable_dns_client} for --enable-dns-client])
            ;;
        esac
    ],
    [enable_dns_client=no])

if test "$enable_dns_client" = "yes"; then
    OPENTHREAD_ENABLE_DNS_CLIENT=1
else
    OPENTHREAD_ENABLE_DNS_CLIENT=0
fi

AC_SUBST(OPENTHREAD_ENABLE_DNS_CLIENT)
AM_CONDITIONAL([OPENTHREAD_ENABLE_DNS_CLIENT], [test "${enable_dns_client}" = "yes"])
AC_DEFINE_UNQUOTED([OPENTHREAD_ENABLE_DNS_CLIENT],[${OPENTHREAD_ENABLE_DNS_CLIENT}],[Define to 1 if you want to enable DNS Client])

#
# SNTP Client
#

AC_ARG_ENABLE(sntp_client,
    [AS_HELP_STRING([--enable-sntp-client],[Enable SNTP client support @<:@default=no@:>@.])],
    [
        case "${enableval}" in

        no|yes)
            enable_sntp_client=${enableval}
            ;;

        *)
            AC_MSG_ERROR([Invalid value ${enable_sntp_client} for --enable-sntp-client])
            ;;
        esac
    ],
    [enable_sntp_client=no])

if test "$enable_sntp_client" = "yes"; then
    OPENTHREAD_ENABLE_SNTP_CLIENT=1
else
    OPENTHREAD_ENABLE_SNTP_CLIENT=0
fi

AC_SUBST(OPENTHREAD_ENABLE_SNTP_CLIENT)
AM_CONDITIONAL([OPENTHREAD_ENABLE_SNTP_CLIENT], [test "${enable_sntp_client}" = "yes"])
AC_DEFINE_UNQUOTED([OPENTHREAD_ENABLE_SNTP_CLIENT],[${OPENTHREAD_ENABLE_SNTP_CLIENT}],[Define to 1 if you want to enable SNTP Client])

#
# Application CoAP
#

AC_ARG_ENABLE(application_coap,
    [AS_HELP_STRING([--enable-application-coap],[Enable CoAP to an application.@<:@default=no@:>@.])],
    [
        case "${enableval}" in

        no|yes)
            enable_application_coap=${enableval}
            ;;

        *)
            AC_MSG_ERROR([Invalid value ${enable_application_coap} for --enable-application-coap])
            ;;
        esac
    ],
    [enable_application_coap=no])

if test "$enable_application_coap" = "yes"; then
    OPENTHREAD_ENABLE_APPLICATION_COAP=1
else
    OPENTHREAD_ENABLE_APPLICATION_COAP=0
fi

AC_SUBST(OPENTHREAD_ENABLE_APPLICATION_COAP)
AM_CONDITIONAL([OPENTHREAD_ENABLE_APPLICATION_COAP], [test "${enable_application_coap}" = "yes"])
AC_DEFINE_UNQUOTED([OPENTHREAD_ENABLE_APPLICATION_COAP],[${OPENTHREAD_ENABLE_APPLICATION_COAP}],[Define to 1 if you want to enable CoAP to an application.])

#
# otLinkRaw API
#

AC_ARG_ENABLE(raw_link_api,
    [AS_HELP_STRING([--enable-raw-link-api],[Enable raw link-layer API support @<:@default=no@:>@.])],
    [
        case "${enableval}" in

        no|yes)
            enable_raw_link_api=${enableval}
            ;;

        *)
            AC_MSG_ERROR([Invalid value ${enable_raw_link_api} for --enable-raw-link-api])
            ;;
        esac
    ],
    [enable_raw_link_api=no])

if test "$enable_raw_link_api" = "yes"; then
    OPENTHREAD_ENABLE_RAW_LINK_API=1
else
    OPENTHREAD_ENABLE_RAW_LINK_API=0
fi

AC_SUBST(OPENTHREAD_ENABLE_RAW_LINK_API)
AM_CONDITIONAL([OPENTHREAD_ENABLE_RAW_LINK_API], [test "${enable_raw_link_api}" = "yes"])
AC_DEFINE_UNQUOTED([OPENTHREAD_ENABLE_RAW_LINK_API],[${OPENTHREAD_ENABLE_RAW_LINK_API}],[Define to 1 if you want to enable raw link-layer API])

#
# Border Router
#

AC_ARG_ENABLE(border_router,
    [AS_HELP_STRING([--enable-border-router],[Enable Border Router support @<:@default=no@:>@.])],
    [
        case "${enableval}" in

        no|yes)
            enable_border_router=${enableval}
            ;;

        *)
            AC_MSG_ERROR([Invalid value ${enable_border_router} for --enable-border-router])
            ;;
        esac
    ],
    [enable_border_router=no])

if test "$enable_border_router" = "yes"; then
    OPENTHREAD_ENABLE_BORDER_ROUTER=1
else
    OPENTHREAD_ENABLE_BORDER_ROUTER=0
fi

AC_SUBST(OPENTHREAD_ENABLE_BORDER_ROUTER)
AM_CONDITIONAL([OPENTHREAD_ENABLE_BORDER_ROUTER], [test "${enable_border_router}" = "yes"])
AC_DEFINE_UNQUOTED([OPENTHREAD_ENABLE_BORDER_ROUTER],[${OPENTHREAD_ENABLE_BORDER_ROUTER}],[Define to 1 if you want to enable Border Router])

#
# Service
#

AC_ARG_ENABLE(service,
    [AS_HELP_STRING([--enable-service],[Enable Service @<:@default=no@:>@.])],
    [
        case "${enableval}" in

        no|yes)
            enable_service=${enableval}
            ;;

        *)
            AC_MSG_ERROR([Invalid value ${enable_service} for --enable-service])
            ;;
        esac
    ],
    [enable_service=no])

if test "$enable_service" = "yes"; then
    OPENTHREAD_ENABLE_SERVICE=1
else
    OPENTHREAD_ENABLE_SERVICE=0
fi

AC_SUBST(OPENTHREAD_ENABLE_SERVICE)
AM_CONDITIONAL([OPENTHREAD_ENABLE_SERVICE], [test "${enable_service}" = "yes"])
AC_DEFINE_UNQUOTED([OPENTHREAD_ENABLE_SERVICE],[${OPENTHREAD_ENABLE_SERVICE}],[Define to 1 if you want to enable Service])

#
# Linker Map Output
#

AC_ARG_ENABLE(linker-map,
    [AS_HELP_STRING([--enable-linker-map],[Enable linker map output @<:@default=no@:>@.])],
    [
        case "${enableval}" in

        no|yes)
            enable_linker_map=${enableval}
            ;;

        *)
            AC_MSG_ERROR([Invalid value ${enable_linker_map} for --enable-linker-map])
            ;;
        esac
    ],
    [enable_linker_map=no])

AC_MSG_CHECKING([whether to enable linker map output])
AC_MSG_RESULT(${enable_linker_map})
AM_CONDITIONAL([OPENTHREAD_ENABLE_LINKER_MAP], [test "${enable_linker_map}" = "yes"])

#
# Custom linker file
#

AC_ARG_WITH(
    [custom-linker-file],
    [AS_HELP_STRING([--with-custom-linker-file=<linkerfile.ld>],[Specify custom linker file (absolute path). @<:@default=none@:>@.])],
    [
        with_custom_linker_file=yes
        OPENTHREAD_CUSTOM_LINKER_FILE=${withval}
    ],
    [
        with_custom_linker_file=no
    ])

AC_SUBST(OPENTHREAD_CUSTOM_LINKER_FILE)
AM_CONDITIONAL([OPENTHREAD_ENABLE_CUSTOM_LINKER_FILE], [test "${with_custom_linker_file}" = "yes"])

#
# Support for ECDSA
#

AC_MSG_CHECKING([whether to build with ECDSA support])
AC_ARG_ENABLE(ecdsa,
    [AS_HELP_STRING([--enable-ecdsa], [Enable ECDSA support @<:@default=no@:>@.])],
    [
        case "${enableval}" in

        no|yes)
            enable_ecdsa=${enableval}
            ;;

        *)
            AC_MSG_ERROR([Invalid value ${enable_ecdsa} for --enable-ecdsa])
            ;;
        esac
    ],
    [enable_ecdsa=no])

if test "$enable_ecdsa" = "yes"; then
    OPENTHREAD_ENABLE_ECDSA=1
else
    OPENTHREAD_ENABLE_ECDSA=0
fi

AC_MSG_RESULT(${enable_ecdsa})
AC_SUBST(OPENTHREAD_ENABLE_ECDSA)
AM_CONDITIONAL([OPENTHREAD_ENABLE_ECDSA], [test "${enable_ecdsa}" = "yes"])
AC_DEFINE_UNQUOTED([OPENTHREAD_ENABLE_ECDSA], [${OPENTHREAD_ENABLE_ECDSA}], [Define to 1 if you want to enable ECDSA support])


#
# Examples
#

AC_ARG_WITH(examples,
    [AS_HELP_STRING([--with-examples=TARGET],
        [Specify the examples from one of: none, posix, cc1352, cc2538, cc2650, cc2652, da15000, efr32, emsk, gp712, kw41z, nrf52840, samr21 @<:@default=none@:>@.])],
    [
        case "${with_examples}" in
        none)
            ;;
        posix|cc1352|cc2538|cc2650|cc2652|da15000|efr32|emsk|gp712|kw41z|nrf52840|samr21)
            if test ${enable_posix_app} = "yes"; then
                AC_MSG_ERROR([--with-examples must be none when POSIX apps are enabled by --enable-posix-app])
            fi
            ;;
        *)
            AC_MSG_ERROR([Invalid value ${with_examples} for --with-examples])
            ;;
        esac
    ],
    [with_examples=none])

OPENTHREAD_ENABLE_EXAMPLES=${with_examples}

case ${with_examples} in

    posix)
        OPENTHREAD_EXAMPLES_POSIX=1
        AC_DEFINE_UNQUOTED([OPENTHREAD_EXAMPLES_POSIX],[${OPENTHREAD_EXAMPLES_POSIX}],[Define to 1 if you want to use posix examples])
        ;;

    cc1352)
        OPENTHREAD_EXAMPLES_CC1352=1
        AC_DEFINE_UNQUOTED([OPENTHREAD_EXAMPLES_CC1352],[${OPENTHREAD_EXAMPLES_CC1352}],[Define to 1 if you want to use cc1352 examples])
        ;;

    cc2538)
        OPENTHREAD_EXAMPLES_CC2538=1
        AC_DEFINE_UNQUOTED([OPENTHREAD_EXAMPLES_CC2538],[${OPENTHREAD_EXAMPLES_CC2538}],[Define to 1 if you want to use cc2538 examples])
        ;;

    cc2650)
        OPENTHREAD_EXAMPLES_CC2650=1
        AC_DEFINE_UNQUOTED([OPENTHREAD_EXAMPLES_CC2650],[${OPENTHREAD_EXAMPLES_CC2650}],[Define to 1 if you want to use cc2650 examples])
        ;;

    cc2652)
        OPENTHREAD_EXAMPLES_CC2652=1
        AC_DEFINE_UNQUOTED([OPENTHREAD_EXAMPLES_CC2652],[${OPENTHREAD_EXAMPLES_CC2652}],[Define to 1 if you want to use cc2652 examples])
        ;;

    da15000)
        OPENTHREAD_EXAMPLES_DA15000=1
        AC_DEFINE_UNQUOTED([OPENTHREAD_EXAMPLES_DA15000],[${OPENTHREAD_EXAMPLES_DA15000}],[Define to 1 if you want to use da15000 examples])
        ;;

    efr32)
        OPENTHREAD_EXAMPLES_EFR32=1
        AC_DEFINE_UNQUOTED([OPENTHREAD_EXAMPLES_EFR32],[${OPENTHREAD_EXAMPLES_EFR32}],[Define to 1 if you want to use efr32 examples])
        ;;

     emsk)
        OPENTHREAD_EXAMPLES_EMSK=1
        AC_DEFINE_UNQUOTED([OPENTHREAD_EXAMPLES_EMSK],[${OPENTHREAD_EXAMPLES_EMSK}],[Define to 1 if you want to use emsk examples])
        ;;

    gp712)
        OPENTHREAD_EXAMPLES_GP712=1
        AC_DEFINE_UNQUOTED([OPENTHREAD_EXAMPLES_GP712],[${OPENTHREAD_EXAMPLES_GP712}],[Define to 1 if you want to use gp712 examples])
        ;;

    kw41z)
        OPENTHREAD_EXAMPLES_KW41Z=1
        AC_DEFINE_UNQUOTED([OPENTHREAD_EXAMPLES_KW41Z],[${OPENTHREAD_EXAMPLES_KW41Z}],[Define to 1 if you want to use kw41z examples])
        ;;

    nrf52840)
        OPENTHREAD_EXAMPLES_NRF52840=1
        AC_DEFINE_UNQUOTED([OPENTHREAD_EXAMPLES_NRF52840],[${OPENTHREAD_EXAMPLES_NRF52840}],[Define to 1 if you want to use nrf52840 examples])
        ;;

    samr21)
        OPENTHREAD_EXAMPLES_SAMR21=1
        AC_DEFINE_UNQUOTED([OPENTHREAD_EXAMPLES_SAMR21],[${OPENTHREAD_EXAMPLES_SAMR21}],[Define to 1 if you want to use samr21 examples])
        ;;

esac

AC_MSG_CHECKING([whether to enable examples])
AC_MSG_RESULT(${OPENTHREAD_EXAMPLES})

AC_SUBST(OPENTHREAD_ENABLE_EXAMPLES)
AM_CONDITIONAL([OPENTHREAD_ENABLE_EXAMPLES], [test ${with_examples} != "none"])
AC_DEFINE_UNQUOTED([OPENTHREAD_ENABLE_EXAMPLES],[${OPENTHREAD_ENABLE_EXAMPLES}],[OpenThread examples])

AC_SUBST(OPENTHREAD_EXAMPLES_POSIX)
AM_CONDITIONAL([OPENTHREAD_EXAMPLES_POSIX], [test "${with_examples}" = "posix"])

AC_SUBST(OPENTHREAD_EXAMPLES_CC1352)
AM_CONDITIONAL([OPENTHREAD_EXAMPLES_CC1352], [test "${with_examples}" = "cc1352"])

AC_SUBST(OPENTHREAD_EXAMPLES_CC2538)
AM_CONDITIONAL([OPENTHREAD_EXAMPLES_CC2538], [test "${with_examples}" = "cc2538"])

AC_SUBST(OPENTHREAD_EXAMPLES_CC2650)
AM_CONDITIONAL([OPENTHREAD_EXAMPLES_CC2650], [test "${with_examples}" = "cc2650"])

AC_SUBST(OPENTHREAD_EXAMPLES_CC2652)
AM_CONDITIONAL([OPENTHREAD_EXAMPLES_CC2652], [test "${with_examples}" = "cc2652"])

AC_SUBST(OPENTHREAD_EXAMPLES_DA15000)
AM_CONDITIONAL([OPENTHREAD_EXAMPLES_DA15000], [test "${with_examples}" = "da15000"])

AC_SUBST(OPENTHREAD_EXAMPLES_EFR32)
AM_CONDITIONAL([OPENTHREAD_EXAMPLES_EFR32], [test "${with_examples}" = "efr32"])

AC_SUBST(OPENTHREAD_EXAMPLES_EMSK)
AM_CONDITIONAL([OPENTHREAD_EXAMPLES_EMSK], [test "${with_examples}" = "emsk"])

AC_SUBST(OPENTHREAD_EXAMPLES_GP712)
AM_CONDITIONAL([OPENTHREAD_EXAMPLES_GP712], [test "${with_examples}" = "gp712"])

AC_SUBST(OPENTHREAD_EXAMPLES_KW41Z)
AM_CONDITIONAL([OPENTHREAD_EXAMPLES_KW41Z], [test "${with_examples}" = "kw41z"])

AC_SUBST(OPENTHREAD_EXAMPLES_NRF52840)
AM_CONDITIONAL([OPENTHREAD_EXAMPLES_NRF52840], [test "${with_examples}" = "nrf52840"])

AC_SUBST(OPENTHREAD_EXAMPLES_SAMR21)
AM_CONDITIONAL([OPENTHREAD_EXAMPLES_SAMR21], [test "${with_examples}" = "samr21"])

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

AM_CONDITIONAL(OPENTHREAD_POSIX, [test "${OPENTHREAD_ENABLE_POSIX_APP}" = "1" -o "${OPENTHREAD_EXAMPLES_POSIX}" = "1"])

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

#---------------------------------------------------
# Enable BSD Security Features
# This enables strlcpy() and other friends in GNU land.
# While the references below generally speak of: "glibc"
# The ARM Embedded platform uses the nano instance of NEWLIB
# Which greatly follows and mirrors glibc.
# --------------------------------------------------
#
# References:
# 1) http://stackoverflow.com/questions/29201515/what-does-d-default-source-do
# 2) http://man7.org/linux/man-pages/man7/feature_test_macros.7.html
#
CFLAGS="${CFLAGS} -D_BSD_SOURCE=1 -D_DEFAULT_SOURCE=1"
CXXFLAGS="${CXXFLAGS} -D_BSD_SOURCE=1 -D_DEFAULT_SOURCE=1"

OLD_CFLAGS="${CFLAGS}"
CFLAGS="${CFLAGS} -Wno-error=address"
AC_HEADER_STDBOOL
CFLAGS="${OLD_CFLAGS}"
AC_HEADER_STDC

AC_CHECK_HEADERS([stdint.h])
AC_CHECK_HEADERS([string.h])

#
# Missing Functions
#
AC_CHECK_FUNC([strlcpy], [AC_DEFINE([HAVE_STRLCPY], [1], [Define if strlcpy exists.])])
AC_CHECK_FUNC([strlcat], [AC_DEFINE([HAVE_STRLCAT], [1], [Define if strlcat exists.])])
AC_CHECK_FUNC([strnlen], [AC_DEFINE([HAVE_STRNLEN], [1], [Define if strnlen exists.])])

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

# Add any mbedtls CPPFLAGS

CPPFLAGS="${CPPFLAGS} ${MBEDTLS_CPPFLAGS}"

# Add any code coverage CPPFLAGS and LDFLAGS

CPPFLAGS="${CPPFLAGS} ${NL_COVERAGE_CPPFLAGS}"
LIBS="${LIBS} ${NL_COVERAGE_LIBS}"

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
include/openthread/Makefile
include/openthread/platform/Makefile
src/Makefile
src/cli/Makefile
src/ncp/Makefile
src/core/Makefile
src/diag/Makefile
src/posix/Makefile
src/posix/platform/Makefile
third_party/Makefile
third_party/jlink/Makefile
third_party/mbedtls/Makefile
examples/Makefile
examples/apps/Makefile
examples/apps/cli/Makefile
examples/apps/ncp/Makefile
examples/platforms/Makefile
examples/platforms/cc1352/Makefile
examples/platforms/cc2538/Makefile
examples/platforms/cc2650/Makefile
examples/platforms/cc2652/Makefile
examples/platforms/da15000/Makefile
examples/platforms/efr32/Makefile
examples/platforms/emsk/Makefile
examples/platforms/gp712/Makefile
examples/platforms/kw41z/Makefile
examples/platforms/nrf52840/Makefile
examples/platforms/samr21/Makefile
examples/platforms/posix/Makefile
examples/platforms/utils/Makefile
tools/Makefile
tools/harness-automation/Makefile
tools/harness-thci/Makefile
tools/spi-hdlc-adapter/Makefile
tests/Makefile
tests/fuzz/Makefile
tests/scripts/Makefile
tests/scripts/thread-cert/Makefile
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
  Host architecture                         : ${host_cpu}
  Host OS                                   : ${host_os}
  Cross compiling                           : ${cross_compiling}
  Build shared libraries                    : ${enable_shared}
  Build static libraries                    : ${enable_static}
  Build debug libraries                     : ${nl_cv_build_debug}
  Build optimized libraries                 : ${nl_cv_build_optimized}
  Build coverage libraries                  : ${nl_cv_build_coverage}
  Build coverage reports                    : ${nl_cv_build_coverage_reports}
  Address sanitizer support                 : ${enable_address_sanitizer}
  Lcov                                      : ${LCOV:--}
  Genhtml                                   : ${GENHTML:--}
  Build tests                               : ${nl_cv_build_tests}
  Build fuzz targets                        : ${enable_fuzz_targets}
  Build tools                               : ${build_tools}
  OpenThread tests                          : ${with_tests}
  Prefix                                    : ${prefix}
  Documentation support                     : ${nl_cv_build_docs}
  Doxygen                                   : ${DOXYGEN:--}
  GraphViz dot                              : ${DOT:--}
  C Preprocessor                            : ${CPP}
  C Compiler                                : ${CC}
  C++ Preprocessor                          : ${CXXCPP}
  C++ Compiler                              : ${CXX}
  Assembler Compiler                        : ${CCAS}
  Archiver                                  : ${AR}
  Archive Indexer                           : ${RANLIB}
  Symbol Stripper                           : ${STRIP}
  Object Copier                             : ${OBJCOPY}
  C Preprocessor flags                      : ${CPPFLAGS:--}
  C Compile flags                           : ${CFLAGS:--}
  C++ Compile flags                         : ${CXXFLAGS:--}
  Assembler flags                           : ${CCASFLAGS:--}
  Link flags                                : ${LDFLAGS:--}
  Link libraries                            : ${LIBS}
  Link maps                                 : ${enable_linker_map}
  Pretty                                    : ${PRETTY:--}
  Pretty args                               : ${PRETTY_ARGS:--}
  Pretty check                              : ${PRETTY_CHECK:--}
  Pretty check args                         : ${PRETTY_CHECK_ARGS:--}
  OpenThread FTD support                    : ${enable_ftd}
  OpenThread MTD support                    : ${enable_mtd}
  OpenThread Radio Only support             : ${enable_radio_only}
  OpenThread CLI support                    : ${enable_cli}
  OpenThread NCP support                    : ${enable_ncp}
  OpenThread NCP-BUS Configuration          : ${with_ncp_bus}
  OpenThread NCP Vendor Hook Source         : ${with_ncp_vendor_hook_source}
  OpenThread NCP Spinel Encrypter           : ${with_ncp_spinel_encrypter_libs}
  OpenThread Vendor Extension Source        : ${with_vendor_extension}
  OpenThread Multiple Instances support     : ${enable_multiple_instances}
  OpenThread MTD Network Diagnostic support : ${enable_mtd_network_diagnostic}
  OpenThread builtin mbedtls support        : ${enable_builtin_mbedtls}
  OpenThread UDP forward support            : ${enable_udp_forward}
  OpenThread Commissioner support           : ${enable_commissioner}
  OpenThread Joiner support                 : ${enable_joiner}
  OpenThread DTLS support                   : ${enable_dtls}
  OpenThread Jam Detection support          : ${enable_jam_detection}
  OpenThread Channel Monitor support        : ${enable_channel_monitor}
  OpenThread Channel Manager support        : ${enable_channel_manager}
  OpenThread MAC Filter support             : ${enable_mac_filter}
  OpenThread Diagnostics support            : ${enable_diag}
  OpenThread Child Supervision support      : ${enable_child_supervision}
  OpenThread Legacy network support         : ${enable_legacy}
  OpenThread Certification log support      : ${enable_cert_log}
  OpenThread DHCPv6 Server support          : ${enable_dhcp6_server}
  OpenThread DHCPv6 Client support          : ${enable_dhcp6_client}
  OpenThread DNS Client support             : ${enable_dns_client}
  OpenThread SNTP Client support            : ${enable_sntp_client}
  OpenThread Application CoAP support       : ${enable_application_coap}
  OpenThread Application CoAP Secure support: ${enable_application_coap_secure}
  OpenThread Raw Link-Layer support         : ${enable_raw_link_api}
  OpenThread Border Agent support           : ${enable_border_agent}
  OpenThread Border Router support          : ${enable_border_router}
  OpenThread Service support                : ${enable_service}
  OpenThread ECDSA support                  : ${enable_ecdsa}
  OpenThread Examples                       : ${with_examples}
  OpenThread POSIX Application              : ${enable_posix_app}

])
