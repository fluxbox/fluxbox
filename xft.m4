# xft.m4
# Copyright (c) 2002 Henrik Kinnunen (fluxgen at linuxmail.org)

# Permission is hereby granted, free of charge, to any person obtaining a
# copy of this software and associated documentation files (the "Software"),
# to deal in the Software without restriction, including without limitation
# the rights to use, copy, modify, merge, publish, distribute, sublicense,
# and/or sell copies of the Software, and to permit persons to whom the 
# Software is furnished to do so, subject to the following conditions:

# The above copyright notice and this permission notice shall be included in 
# all copies or substantial portions of the Software. 

# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR 
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, 
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL 
# THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER 
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING 
# FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER 
# DEALINGS IN THE SOFTWARE.

# AM_PATH_XFT1([ACTION-IF-FOUND [, ACTION-IF-NOT-FOUND]]])
AC_DEFUN(AM_PATH_XFT1,
[
	AC_CHECK_LIB(Xft, XftFontOpen,
		XFT_LIBS="-lXft"
		[$1],
		[$2]
	)
])

# AM_PATH_XFT2([ACTION-IF-FOUND [, ACTION-IF-NOT-FOUND]]])
AC_DEFUN(AM_PATH_XFT2,
[
	if test x$pkg_exec_prefix != x ; then
	   xft_args="$xft_args --exec-prefix=$pkg_exec_prefix"
	   if test x${PKG_CONFIG+set} != xset ; then
	       PKG_CONFIG=$pkg_exec_prefix/bin/pkg-config
    fi
fi

if test x$xft_prefix != x ; then
   xft_args="$xft_args --prefix=$xft_prefix"
   if test x${PKG_CONFIG+set} != xset ; then
      PKG_CONFIG=$xft_prefix/bin/pkg-config	  
   fi
fi

AC_PATH_PROG(PKG_CONFIG, pkg-config, no)
if test "x$PKG_CONFIG" = "xno" ; then
	ifelse([$2], , :, [$2])
else 
	XFT_CFLAGS=`$PKG_CONFIG $xftconf_args --cflags xft`
	XFT_LIBS=`$PKG_CONFIG $xftconf_args --libs xft`
	ifelse([$1], , :, [$1])
fi

])

# AM_PATH_XFT(default-value, [ACTION-IF-FOUND [, ACTION-IF-NOT-FOUND]]])
# Test for Xft, and define XFT_CFLAGS and XFT_LIBS
AC_DEFUN(AM_PATH_XFT,
[
 AC_ARG_WITH(xft-prefix,[  --with-xft-prefix=path  Prefix where Xft is installed (optional)],
            xft_prefix="$withval", xft_prefix="")
 AC_ARG_WITH(pkg-exec-prefix,[  --with-pkg-exec-prefix=path Exec prefix where pkg-config is installed (optional)],
            pkg_exec_prefix="$withval", pkg_exec_prefix="")
 AC_ARG_ENABLE(xft, [  --enable-xft            Xft (antialias) support (default=$1)],
	if test "x$enableval" = "xyes"; then
		TRY_XFT=yes
	else
		TRY_XFT=no
	fi
	,
	TRY_XFT=$1
 )

if test "x$TRY_XFT" = "xyes"; then
	AC_MSG_RESULT(yes)
	AM_PATH_XFT2(
		[$2],
		# xft2 failed: try xft1
		AM_PATH_XFT1(
		[$2],
		[$3]
		AC_MSG_RESULT([Cant find Xft libraries! Disabling Xft]))
	)
else
	AC_MSG_RESULT(no)
	[$3]
fi

CFLAGS="$CFLAGS $XFT_CFLAGS"
CXXFLAGS="$CXXFLAGS $XFT_CFLAGS"
LIBS="$LIBS $XFT_LIBS"

])
