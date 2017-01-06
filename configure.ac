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

PROSPECTIVE_CFLAGS="-Wall -Wextra -Wshadow -Werror -std=c99 -pedantic-errors"
PROSPECTIVE_CXXFLAGS="-Wall -Wextra -Wshadow -Werror -std=gnu++98 -Wno-c++14-compat"

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
AC_SUBST(PRETTY_CHECK, ["\${abs_top_srcdir}/.astyle/astyle-wrap.sh"])
AC_SUBST(PRETTY_CHECK_ARGS, ["astyle --options=\${abs_top_builddir}/.astyle/astyle-opts --dry-run"])

#
# Tests
#
AC_MSG_NOTICE([checking whether to build tests])

# Tests

NL_ENABLE_TESTS([yes])

AM_CONDITIONAL([OPENTHREAD_BUILD_TESTS], [test "${nl_cv_build_tests}" = "yes"])

# Check whether to run only the minimum set of tests.

AC_ARG_WITH(tests,
    [AS_HELP_STRING([--with-tests=TESTS],
        [Specify the tests from one of: all, subset1, subset2, subset3, subset4, subset5 @<:@default=all@:>@.])],
    [
        case "${with_tests}" in

        all|subset1|subset2|subset3|subset4|subset5)
            ;;
        *)
            AC_MSG_ERROR([Invalid value ${with_tests} for --with-tests])
            ;;
        esac
    ],
    [with_tests=all])

case ${with_tests} in

    all)
        OPENTHREAD_TESTS_SUBSET1=yes
        OPENTHREAD_TESTS_SUBSET2=yes
        OPENTHREAD_TESTS_SUBSET3=yes
        OPENTHREAD_TESTS_SUBSET4=yes
        OPENTHREAD_TESTS_SUBSET5=yes
        ;;

    subset1)
        OPENTHREAD_TESTS_SUBSET1=yes
        ;;

    subset2)
        OPENTHREAD_TESTS_SUBSET2=yes
        ;;

    subset3)
        OPENTHREAD_TESTS_SUBSET3=yes
        ;;

    subset4)
        OPENTHREAD_TESTS_SUBSET4=yes
        ;;

    subset5)
        OPENTHREAD_TESTS_SUBSET5=yes
        ;;

esac

AC_SUBST(OPENTHREAD_TESTS_SUBSET1)
AM_CONDITIONAL([OPENTHREAD_TESTS_SUBSET1], [test "${OPENTHREAD_TESTS_SUBSET1}" = "yes"])
AM_CONDITIONAL([OPENTHREAD_TESTS_SUBSET2], [test "${OPENTHREAD_TESTS_SUBSET2}" = "yes"])
AM_CONDITIONAL([OPENTHREAD_TESTS_SUBSET3], [test "${OPENTHREAD_TESTS_SUBSET3}" = "yes"])
AM_CONDITIONAL([OPENTHREAD_TESTS_SUBSET4], [test "${OPENTHREAD_TESTS_SUBSET4}" = "yes"])
AM_CONDITIONAL([OPENTHREAD_TESTS_SUBSET5], [test "${OPENTHREAD_TESTS_SUBSET5}" = "yes"])

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
    [AS_HELP_STRING([--enable-cli],[Enable CLI support @<:@default=no@:>@.])],
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
    [AS_HELP_STRING([--enable-ncp[[=spi|uart]]],[Enable NCP support @<:@default=no@:>@.])],
    [
        case "${enableval}" in

        no|spi|uart)
            enable_ncp=${enableval}
            ;;

        yes)
            enable_ncp=uart
            ;;

        *)
            AC_MSG_ERROR([Invalid value ${with_ncp} for --enable-ncp])
            ;;
        esac
    ],
    [enable_ncp=no])
AC_MSG_RESULT(${enable_ncp})
AM_CONDITIONAL([OPENTHREAD_ENABLE_NCP],      [test "${enable_ncp}" != "no"])
AM_CONDITIONAL([OPENTHREAD_ENABLE_NCP_SPI],  [test "${enable_ncp}"  = "spi"])
AM_CONDITIONAL([OPENTHREAD_ENABLE_NCP_UART], [test "${enable_ncp}"  = "uart"])

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

if test "$enable_builtin_mbedtls" = "yes"; then
    MBEDTLS_CPPFLAGS="-I\${abs_top_srcdir}/third_party/mbedtls"
    MBEDTLS_CPPFLAGS="${MBEDTLS_CPPFLAGS} -I\${abs_top_srcdir}/third_party/mbedtls/repo/include"
    MBEDTLS_CPPFLAGS="${MBEDTLS_CPPFLAGS} -I\${abs_top_srcdir}/third_party/mbedtls/repo/include/mbedtls"
    MBEDTLS_CPPFLAGS="${MBEDTLS_CPPFLAGS} -DMBEDTLS_CONFIG_FILE=\\\"mbedtls-config.h\\\""
fi
AC_MSG_RESULT(${enable_builtin_mbedtls})
AM_CONDITIONAL([OPENTHREAD_ENABLE_BUILTIN_MBEDTLS], [test "${enable_builtin_mbedtls}" = "yes"])

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

AC_MSG_RESULT(${enable_joiner})
AC_SUBST(OPENTHREAD_ENABLE_JOINER)
AM_CONDITIONAL([OPENTHREAD_ENABLE_JOINER], [test "${enable_joiner}" = "yes"])
AC_DEFINE_UNQUOTED([OPENTHREAD_ENABLE_JOINER],[${OPENTHREAD_ENABLE_JOINER}],[Define to 1 to enable the joiner role.])

if test "${enable_commissioner}" = "yes" -o "${enable_joiner}" = "yes"; then
    enable_dtls="yes"
    OPENTHREAD_ENABLE_DTLS=1
else
    enable_dtls="no"
    OPENTHREAD_ENABLE_DTLS=0
fi

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

AC_MSG_RESULT(${enable_jam_detection})
AC_SUBST(OPENTHREAD_ENABLE_JAM_DETECTION)
AM_CONDITIONAL([OPENTHREAD_ENABLE_JAM_DETECTION], [test "${enable_jam_detection}" = "yes"])
AC_DEFINE_UNQUOTED([OPENTHREAD_ENABLE_JAM_DETECTION],[${OPENTHREAD_ENABLE_JAM_DETECTION}],[Define to 1 if you want to use jam detection feature])

#
# MAC Whitelist and Blacklist
#

AC_ARG_ENABLE(mac_whitelist,
    [AS_HELP_STRING([--enable-mac-whitelist],[Enable MAC whitelist/blacklist support @<:@default=yes@:>@.])],
    [
        case "${enableval}" in

        no|yes)
            enable_mac_whitelist=${enableval}
            ;;

        *)
            AC_MSG_ERROR([Invalid value ${enable_mac_whitelist} for --enable-mac-whitelist])
            ;;
        esac
    ],
    [enable_mac_whitelist=yes])

if test "$enable_mac_whitelist" = "yes"; then
    OPENTHREAD_ENABLE_MAC_WHITELIST=1
else
    OPENTHREAD_ENABLE_MAC_WHITELIST=0
fi

AC_MSG_RESULT(${enable_mac_whitelist})
AC_SUBST(OPENTHREAD_ENABLE_MAC_WHITELIST)
AM_CONDITIONAL([OPENTHREAD_ENABLE_MAC_WHITELIST], [test "${enable_mac_whitelist}" = "yes"])
AC_DEFINE_UNQUOTED([OPENTHREAD_ENABLE_MAC_WHITELIST],[${OPENTHREAD_ENABLE_MAC_WHITELIST}],[Define to 1 if you want to use MAC whitelist/blacklist feature])

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

AC_MSG_RESULT(${enable_diag})
AC_SUBST(OPENTHREAD_ENABLE_DIAG)
AM_CONDITIONAL([OPENTHREAD_ENABLE_DIAG], [test "${enable_diag}" = "yes"])
AC_DEFINE_UNQUOTED([OPENTHREAD_ENABLE_DIAG],[${OPENTHREAD_ENABLE_DIAG}],[Define to 1 if you want to use diagnostics module])

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

AC_MSG_RESULT(${enable_legacy})
AC_SUBST(OPENTHREAD_ENABLE_LEGACY)
AM_CONDITIONAL([OPENTHREAD_ENABLE_LEGACY], [test "${enable_legacy}" = "yes"])
AC_DEFINE_UNQUOTED([OPENTHREAD_ENABLE_LEGACY],[${OPENTHREAD_ENABLE_LEGACY}],[Define to 1 if you want to use legacy network support])

#
# Default Logging
#

AC_ARG_ENABLE(default_logging,
    [AS_HELP_STRING([--enable-default-logging],[Enable default logging support @<:@default=no@:>@.])],
    [
        case "${enableval}" in

        no|yes)
            enable_default_logging=${enableval}
            ;;

        *)
            AC_MSG_ERROR([Invalid value ${enable_default_logging} for --enable-default-logging])
            ;;
        esac
    ],
    [enable_default_logging=no])

if test "$enable_default_logging" = "yes"; then
    OPENTHREAD_ENABLE_DEFAULT_LOGGING=1
else
    OPENTHREAD_ENABLE_DEFAULT_LOGGING=0
fi

AC_SUBST(OPENTHREAD_ENABLE_DEFAULT_LOGGING)
AM_CONDITIONAL([OPENTHREAD_ENABLE_DEFAULT_LOGGING], [test "${enable_default_logging}" = "yes"])
AC_DEFINE_UNQUOTED([OPENTHREAD_ENABLE_DEFAULT_LOGGING],[${OPENTHREAD_ENABLE_DEFAULT_LOGGING}],[Define to 1 if you want to enable default logging])

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
# Examples
#

AC_ARG_WITH(examples,
    [AS_HELP_STRING([--with-examples=TARGET],
        [Specify the examples from one of: none, posix, cc2538, da15000 @<:@default=none@:>@.])],
    [
        case "${with_examples}" in

        none|posix|cc2538|da15000)
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
        AC_DEFINE_UNQUOTED([OPENTHREAD_EXAMPLES_POSIX],[${OPENTHREAD_EXAMPLES_POSIX}],[Define to 1 if you want to use posix examples])
        ;;

    cc2538)
        OPENTHREAD_EXAMPLES_CC2538=1
        AC_DEFINE_UNQUOTED([OPENTHREAD_EXAMPLES_CC2538],[${OPENTHREAD_EXAMPLES_CC2538}],[Define to 1 if you want to use cc2538 examples])
        ;;

    da15000)
        OPENTHREAD_EXAMPLES_DA15000=1
        AC_DEFINE_UNQUOTED([OPENTHREAD_EXAMPLES_DA15000],[${OPENTHREAD_EXAMPLES_DA15000}],[Define to 1 if you want to use da15000 examples])
        ;;

esac

AC_MSG_RESULT(${OPENTHREAD_EXAMPLES})

AC_SUBST(OPENTHREAD_EXAMPLES)
AM_CONDITIONAL([OPENTHREAD_EXAMPLES], [test ${with_examples} != "none"])
AC_DEFINE_UNQUOTED([OPENTHREAD_EXAMPLES],[${OPENTHREAD_EXAMPLES}],[OpenThread examples])

AC_SUBST(OPENTHREAD_EXAMPLES_POSIX)
AM_CONDITIONAL([OPENTHREAD_EXAMPLES_POSIX], [test "${OPENTHREAD_EXAMPLES}" = "posix"])

AC_SUBST(OPENTHREAD_EXAMPLES_CC2538)
AM_CONDITIONAL([OPENTHREAD_EXAMPLES_CC2538], [test "${OPENTHREAD_EXAMPLES}" = "cc2538"])

AC_SUBST(OPENTHREAD_EXAMPLES_DA15000)
AM_CONDITIONAL([OPENTHREAD_EXAMPLES_DA15000], [test "${OPENTHREAD_EXAMPLES}" = "da15000"])

#
# Platform Information
#

AC_ARG_WITH(platform-info,
    [AS_HELP_STRING([--with-platform-info=PLATFORM],
        [Specify the platform information @<:@default=none@:>@.])],
    [],
    [with_platform_info=none])

PLATFORM_INFO=${with_platform_info}

AC_MSG_RESULT(${PLATFORM_INFO})

if test "$PLATFORM_INFO" != "none"; then
    AC_SUBST(PLATFORM_INFO)
    AC_DEFINE_UNQUOTED([PLATFORM_INFO],["${PLATFORM_INFO}"],[OpenThread platform information])
fi

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

# Add any mbedtls CPPFLAGS

CPPFLAGS="${CPPFLAGS} ${MBEDTLS_CPPFLAGS}"

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
include/commissioning/Makefile
include/dhcp6/Makefile
include/ncp/Makefile
include/platform/Makefile
src/Makefile
src/cli/Makefile
src/ncp/Makefile
src/core/Makefile
src/diag/Makefile
third_party/Makefile
third_party/mbedtls/Makefile
examples/Makefile
examples/apps/Makefile
examples/apps/cli/Makefile
examples/apps/ncp/Makefile
examples/platforms/Makefile
examples/platforms/cc2538/Makefile
examples/platforms/da15000/Makefile
examples/platforms/posix/Makefile
examples/platforms/utils/Makefile
tools/Makefile
tools/harness-automation/Makefile
tools/harness-thci/Makefile
tools/spi-hdlc-adapter/Makefile
tools/spinel-cli/Makefile
tools/spinel-cli/spinel/Makefile
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
  OpenThread tests                          : ${with_tests}
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
  Pretty                                    : ${PRETTY:--}
  Pretty args                               : ${PRETTY_ARGS:--}
  Pretty check                              : ${PRETTY_CHECK:--}
  Pretty check args                         : ${PRETTY_CHECK_ARGS:--}
  OpenThread CLI support                    : ${enable_cli}
  OpenThread NCP support                    : ${enable_ncp}
  OpenThread builtin mbedtls support        : ${enable_builtin_mbedtls}
  OpenThread Commissioner support           : ${enable_commissioner}
  OpenThread Joiner support                 : ${enable_joiner}
  OpenThread DTLS support                   : ${enable_dtls}
  OpenThread Jam Detection support          : ${enable_jam_detection}
  OpenThread MAC Whitelist support          : ${enable_mac_whitelist}
  OpenThread Diagnostics support            : ${enable_diag}
  OpenThread Legacy network support         : ${enable_legacy}
  OpenThread Default logging support        : ${enable_default_logging}
  OpenThread Certification log support      : ${enable_cert_log}
  OpenThread DHCPv6 Server support          : ${enable_dhcp6_server}
  OpenThread DHCPv6 Client support          : ${enable_dhcp6_client}
  Openthread Application CoAP support       : ${enable_application_coap}
  OpenThread examples                       : ${OPENTHREAD_EXAMPLES}
  OpenThread platform information           : ${PLATFORM_INFO}

])
