## ----------------------------------- ##                   -*- Autoconf -*-
## Check if --with-dmalloc was given.  ##
## From Franc,ois Pinard               ##
## ----------------------------------- ##

# Copyright (C) 1996-2013 Free Software Foundation, Inc.
#
# This file is free software; the Free Software Foundation
# gives unlimited permission to copy and/or distribute it,
# with or without modifications, as long as this notice is preserved.

AC_DEFUN([AM_WITH_DMALLOC],
[AC_MSG_CHECKING([if malloc debugging is wanted])
AC_ARG_WITH([dmalloc],
[AS_HELP_STRING([--with-dmalloc],
                [use dmalloc, as in http://www.dmalloc.com])],
[if test "$withval" = yes; then
  AC_MSG_RESULT([yes])
  AC_DEFINE([WITH_DMALLOC], [1],
	    [Define if using the dmalloc debugging malloc package])
  LIBS="$LIBS -ldmalloc"
  LDFLAGS="$LDFLAGS -g"
else
  AC_MSG_RESULT([no])
fi], [AC_MSG_RESULT([no])])
])
