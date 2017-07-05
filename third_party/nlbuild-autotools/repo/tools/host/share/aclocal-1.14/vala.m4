# Autoconf support for the Vala compiler

# Copyright (C) 2008-2013 Free Software Foundation, Inc.
#
# This file is free software; the Free Software Foundation
# gives unlimited permission to copy and/or distribute it,
# with or without modifications, as long as this notice is preserved.

# Check whether the Vala compiler exists in $PATH.  If it is found, the
# variable VALAC is set pointing to its absolute path.  Otherwise, it is
# simply set to 'valac'.
# Optionally a minimum release number of the compiler can be requested.
# If the ACTION-IF-FOUND parameter is given, it will be run if a proper
# Vala compiler is found.
# Similarly, if the ACTION-IF-FOUND is given, it will be run if no proper
# Vala compiler is found.  It defaults to simply print a warning about the
# situation, but otherwise proceeding with the configuration.
#
# AM_PROG_VALAC([MINIMUM-VERSION], [ACTION-IF-FOUND], [ACTION-IF-NOT-FOUND])
# --------------------------------------------------------------------------
AC_DEFUN([AM_PROG_VALAC],
  [AC_PATH_PROG([VALAC], [valac], [valac])
   AS_IF([test "$VALAC" != valac && test -n "$1"],
      [AC_MSG_CHECKING([whether $VALAC is at least version $1])
       am__vala_version=`$VALAC --version | sed 's/Vala  *//'`
       AS_VERSION_COMPARE([$1], ["$am__vala_version"],
         [AC_MSG_RESULT([yes])],
         [AC_MSG_RESULT([yes])],
         [AC_MSG_RESULT([no])
          VALAC=valac])])
    if test "$VALAC" = valac; then
      m4_default([$3],
        [AC_MSG_WARN([no proper vala compiler found])
         AC_MSG_WARN([you will not be able to compile vala source files])])
    else
      m4_default([$2], [:])
    fi])
