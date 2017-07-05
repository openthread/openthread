#  -*- Autoconf -*-
# Obsolete and "removed" macros, that must however still report explicit
# error messages when used, to smooth transition.
#
# Copyright (C) 1996-2013 Free Software Foundation, Inc.
#
# This file is free software; the Free Software Foundation
# gives unlimited permission to copy and/or distribute it,
# with or without modifications, as long as this notice is preserved.

AC_DEFUN([AM_CONFIG_HEADER],
[AC_DIAGNOSE([obsolete],
['$0': this macro is obsolete.
You should use the 'AC][_CONFIG_HEADERS' macro instead.])dnl
AC_CONFIG_HEADERS($@)])

AC_DEFUN([AM_PROG_CC_STDC],
[AC_PROG_CC
am_cv_prog_cc_stdc=$ac_cv_prog_cc_stdc
AC_DIAGNOSE([obsolete],
['$0': this macro is obsolete.
You should simply use the 'AC][_PROG_CC' macro instead.
Also, your code should no longer depend upon 'am_cv_prog_cc_stdc',
but upon 'ac_cv_prog_cc_stdc'.])])

AC_DEFUN([AM_C_PROTOTYPES],
         [AC_FATAL([automatic de-ANSI-fication support has been removed])])
AU_DEFUN([fp_C_PROTOTYPES], [AM_C_PROTOTYPES])
