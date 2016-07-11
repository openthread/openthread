# AM_COND_IF                                            -*- Autoconf -*-

# Copyright (C) 2008-2013 Free Software Foundation, Inc.
#
# This file is free software; the Free Software Foundation
# gives unlimited permission to copy and/or distribute it,
# with or without modifications, as long as this notice is preserved.

# _AM_COND_IF
# _AM_COND_ELSE
# _AM_COND_ENDIF
# --------------
# These macros are only used for tracing.
m4_define([_AM_COND_IF])
m4_define([_AM_COND_ELSE])
m4_define([_AM_COND_ENDIF])

# AM_COND_IF(COND, [IF-TRUE], [IF-FALSE])
# ---------------------------------------
# If the shell condition COND is true, execute IF-TRUE, otherwise execute
# IF-FALSE.  Allow automake to learn about conditional instantiating macros
# (the AC_CONFIG_FOOS).
AC_DEFUN([AM_COND_IF],
[m4_ifndef([_AM_COND_VALUE_$1],
	   [m4_fatal([$0: no such condition "$1"])])dnl
_AM_COND_IF([$1])dnl
if test -z "$$1_TRUE"; then :
  m4_n([$2])[]dnl
m4_ifval([$3],
[_AM_COND_ELSE([$1])dnl
else
  $3
])dnl
_AM_COND_ENDIF([$1])dnl
fi[]dnl
])
