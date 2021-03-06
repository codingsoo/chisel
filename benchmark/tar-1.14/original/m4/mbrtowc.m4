# mbrtowc.m4 serial 7
dnl Copyright (C) 2001-2002, 2004 Free Software Foundation, Inc.
dnl This file is free software, distributed under the terms of the GNU
dnl General Public License.  As a special exception to the GNU General
dnl Public License, this file may be distributed as part of a program
dnl that contains a configuration script generated by Autoconf, under
dnl the same distribution terms as the rest of that program.

dnl From Paul Eggert

dnl This file can be removed, and gl_FUNC_MBRTOWC replaced with
dnl AC_FUNC_MBRTOWC, when autoconf 2.57 can be assumed everywhere.

AC_DEFUN([gl_FUNC_MBRTOWC],
[
  AC_CACHE_CHECK([whether mbrtowc and mbstate_t are properly declared],
    gl_cv_func_mbrtowc,
    [AC_TRY_LINK(
       [#include <wchar.h>],
       [mbstate_t state; return ! (sizeof state && mbrtowc);],
       gl_cv_func_mbrtowc=yes,
       gl_cv_func_mbrtowc=no)])
  if test $gl_cv_func_mbrtowc = yes; then
    AC_DEFINE(HAVE_MBRTOWC, 1,
      [Define to 1 if mbrtowc and mbstate_t are properly declared.])
  fi
])
