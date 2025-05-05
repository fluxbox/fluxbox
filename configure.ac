dnl Initialize autoconf and automake
AC_PREREQ([2.52])
AC_INIT([fluxbox],
	[1.3.7],
	[fluxbox-devel@lists.sourceforge.net], [],
	[http://fluxbox.org/])
AC_CONFIG_SRCDIR([src/fluxbox.cc])
AC_CONFIG_AUX_DIR([build-aux])
AC_CONFIG_MACRO_DIR([m4])
AC_USE_SYSTEM_EXTENSIONS
AM_INIT_AUTOMAKE([foreign 1.10 tar-pax dist-bzip2 subdir-objects])

dnl Determine default prefix
AS_IF([test "x$prefix" = "xNONE"], [prefix="$ac_default_prefix"])

AC_LANG_CPLUSPLUS
AX_CXX_COMPILE_STDCXX_11([ext], [mandatory])

dnl Locate required external software
AC_PROG_CC
AC_PROG_CXX
AC_PROG_INSTALL
AC_PROG_RANLIB

AC_C_CONST
AC_C_INLINE

AC_TYPE_PID_T
AC_TYPE_SIZE_T
AC_STRUCT_TM

dnl Require pkg-config
m4_ifndef([PKG_PROG_PKG_CONFIG],
	[m4_fatal([Could not locate the pkg-config autoconf macros.  These are usually
located in /usr/share/aclocal/pkg.m4.  If your macros are in a different
location, try setting the environment variable
export ACLOCAL_FLAGS="-I/other/macro/dir"
before running ./autogen.sh or autoreconf again.])
])
PKG_PROG_PKG_CONFIG

dnl Check for sed
AC_CHECK_PROGS([regex_cmd], [sed])
AS_IF([test "x$regex_cmd" = "x"], [
	AC_MSG_ERROR([error. sed is required to build the data files.])
])

dnl Check for system header files
AC_HEADER_STDC
AC_HEADER_STDBOOL
AC_CHECK_HEADERS([ \
	ctype.h \
	dirent.h \
	errno.h \
	fcntl.h \
	langinfo.h \
	libgen.h \
	locale.h \
	math.h \
	nl_types.h \
	process.h \
	signal.h \
	stdarg.h \
	stdint.h \
	stdio.h \
	sys/param.h \
	sys/select.h \
	sys/signal.h \
	sys/stat.h \
	sys/time.h \
	sys/types.h \
	sys/wait.h \
	time.h \
	unistd.h \
])
AC_CHECK_HEADERS([ \
	cassert \
	cctype \
	cerrno \
	cmath \
	cstdarg \
	cstdint \
	cstdio \
	cstdlib \
	cstring \
	ctime \
])
AC_CHECK_HEADERS([sstream], [], [
	AC_CHECK_HEADERS([strstream], [], [
		AC_MSG_ERROR([Your libstdc++ doesn't have the sstream or strstream classes])
	])
])

dnl Check for existance of basename(), setlocale() and strftime()
AC_FUNC_CLOSEDIR_VOID
AC_FUNC_ERROR_AT_LINE
AC_FUNC_FORK
AC_FUNC_MALLOC
AC_FUNC_REALLOC
AC_FUNC_SELECT_ARGTYPES
AC_FUNC_STAT

AC_CHECK_FUNCS([basename], [], [
	AC_CHECK_LIB([gen], [basename], [LIBS="-lgen $LIBS"])
])
AC_CHECK_FUNCS([ \
	alarm \
	catclose \
	catgets \
	catopen \
	getpid \
	memset \
	mkdir \
	nl_langinfo \
	putenv \
	regcomp \
	select \
	setenv \
	setlocale \
	sigaction \
	snprintf \
	sqrt \
	strcasecmp \
	strcasestr \
	strchr \
	strstr \
	strtol \
	strtoul \
	sync \
	vsnprintf \
])

AC_CHECK_LIBM
AC_SUBST(LIBM)
LIBS="$LIBS $LIBM"

dnl Windows requires the mingw-catgets library for the catgets function.
AC_SEARCH_LIBS([catgets], [catgets], [], [])

dnl The autoconf test for strftime is broken now (due to gcc 3.3 bug?):
dnl Gcc 3.3 testprog = ``extern "C" char strftime;'', build with g++ test.cc
dnl breaks with:
dnl   test.cc:1: error: nonnull argument with out-of-range operand number
dnl   (arg 1, operand 3)
AC_MSG_CHECKING([for strftime])

AC_COMPILE_IFELSE([
	AC_LANG_PROGRAM([#include <time.h>], [[
		char * s;
		time_t t = time(NULL);
		size_t x = strftime(s, 5, "%a", localtime(&t));
	]])
], [
	AC_DEFINE([HAVE_STRFTIME], [1], [Define to 1 if you have the 'strftime' function.])
	AC_MSG_RESULT([yes])
], [
	AC_MSG_RESULT([no])
])

AC_MSG_CHECKING([for clock_gettime])
AC_COMPILE_IFELSE([
	AC_LANG_PROGRAM([#include <time.h>], [[
		clock_gettime(CLOCK_MONOTONIC, 0);
		return 0;
	]])
], [
	AC_DEFINE([HAVE_CLOCK_GETTIME], [1], [Define to 1 if you have the 'clock_gettime' function.])
	AC_MSG_RESULT([yes])
	# *bsd has clock_gettime() in libc
	AC_CHECK_LIB([rt], [clock_gettime], [LIBS="-lrt $LIBS"])
], [
	AC_MSG_RESULT([no])
])

AC_MSG_CHECKING([for mach_absolute_time])
AC_COMPILE_IFELSE([
	AC_LANG_PROGRAM([#include <mach/mach_time.h>], [[
		mach_absolute_time();
		return 0;
	]])
], [
	AC_DEFINE([HAVE_MACH_ABSOLUTE_TIME], [1], [Define to 1 if you have the 'mach_absolute_time' function.])
	AC_MSG_RESULT([yes])
	#AC_CHECK_LIB([], [clock_gettime], [LIBS="-lrt $LIBS"])
], [
	AC_MSG_RESULT([no])
])

AM_GNU_GETTEXT_VERSION([0.11.5])
AM_ICONV
AS_IF([test x$am_cv_proto_iconv_arg1 = "xconst"], [
	AC_DEFINE([HAVE_CONST_ICONV], [1], [Is second argument of iconv() is of type 'const char **' or 'char **'])
])
LIBS="$LIBICONV $LIBS"

dnl Check for networking libraries
AC_CHECK_LIB([nsl], [t_open], [LIBS="-lnsl $LIBS"])
AC_CHECK_LIB([socket], [socket], [LIBS="-lsocket $LIBS"])

#dnl Check for X headers and libraries
#AC_PATH_X
#AC_PATH_XTRA
#AS_IF([test x$no_x = "xyes"], [
#	AC_MSG_ERROR([Fluxbox requires the X Window System libraries and headers.])
#])

dnl Check for X11
PKG_CHECK_MODULES([X11], [ x11 ],
	[AC_DEFINE([HAVE_X11], [1], [Define if x11 is available]) have_x11=yes],
	[have_x11=no])
AM_CONDITIONAL([X11], [test "$have_x11" = "yes"], AC_MSG_ERROR([Could not find XOpenDisplay in -lX11.]))
AS_IF([test x$have_x11 = "xno"], [
	AC_MSG_ERROR([Fluxbox requires the X Window System libraries and headers.])
])

CXXFLAGS="$X11_CFLAGS $CXXFLAGS"
LIBS="$X11_LIBS $LIBS"

dnl Check for xpg4
AC_CHECK_LIB([xpg4], [setlocale], [LIBS="-lxpg4 $LIBS"])
AC_CHECK_PROGS([gencat_cmd], [gencat])
AS_IF([test "x$gencat_cmd" = "x"], [NLS=""], [])

dnl Check for Remember options
AC_MSG_CHECKING([whether to include remember functionality])
AC_ARG_ENABLE([remember],
	AS_HELP_STRING([--enable-remember],
		[include Remembering attributes (default=yes)]),
	[], [enable_remember=yes]
)
AC_MSG_RESULT([$enable_remember])
AS_IF([test "x$enable_remember" = "xyes"], [
	AC_DEFINE([REMEMBER], [1], [compile with remember])
])
AM_CONDITIONAL([REMEMBER_SRC], [test "x$enable_remember" = "xyes"])

dnl Check for regular expressions
AC_MSG_CHECKING([whether to have (POSIX) regular expression support])
AC_ARG_ENABLE([regexp],
	AS_HELP_STRING([--enable-regexp],
		[regular expression support (default=yes)]),
	[], [enable_regexp=yes]
)
AS_IF([test "x$enable_regexp" = "xyes"], [
	AC_EGREP_HEADER([regex_t], [regex.h], [
		AC_DEFINE([USE_REGEXP], [1], [Regular Expression support])
		AC_MSG_RESULT([yes])
		REGEXP_SRC=true
	], [
		AC_MSG_RESULT([no])
		REGEXP_SRC=false
	])
])
AM_CONDITIONAL([REGEXP_SRC], [test x$REGEXP_SRC = xtrue])

dnl Check for the Slit
AC_MSG_CHECKING([whether to include the Slit])
AC_ARG_ENABLE([slit],
	AS_HELP_STRING([--enable-slit],
		[include code for the Slit (default=yes)]),
	[], [enable_slit=yes]
)
AC_MSG_RESULT([$enable_slit])
AS_IF([test "x$enable_slit" = "xyes"], [
	AC_DEFINE([USE_SLIT], [1], [compile with slit])
])
AM_CONDITIONAL([SLIT_SRC], [test "x$enable_slit" = "xyes"])

dnl Check for Systemtray options
AC_MSG_CHECKING([whether to include SystemTray])
AC_ARG_ENABLE([systray],
	AS_HELP_STRING([--enable-systray],
		[include SystemTray (default=yes)]),
	[], [enable_systray=yes]
)
AC_MSG_RESULT([$enable_systray])
AS_IF([test "x$enable_systray" = "xyes"], [
	AC_DEFINE([USE_SYSTRAY], [1], [compile with systemtray])
])
AM_CONDITIONAL([SYSTRAY_SRC], [test "x$enable_systray" = "xyes"])

dnl Check for Toolbar options
AC_MSG_CHECKING([whether to include Toolbar])
AC_ARG_ENABLE([toolbar],
	AS_HELP_STRING([--enable-toolbar],
		[include Toolbar (default=yes)]),
	[], [enable_toolbar=yes]
)
AC_MSG_RESULT([$enable_toolbar])

AS_IF([test "x$enable_systray" = "xyes" -a "x$enable_toolbar" != "xyes"], [
	AC_MSG_RESULT([disabled toolbar, but enabled systray => reenable toolbar])
	AS_VAR_SET([enable_toolbar], [yes])
])
AS_IF([test "x$enable_toolbar" = "xyes"], [
	AC_DEFINE([USE_TOOLBAR], [1], [compile with toolbar])
])
AM_CONDITIONAL([TOOLBAR_SRC], [test "x$enable_toolbar" = "xyes"])

dnl Extended Window Manager Hints
AC_MSG_CHECKING([whether to support Extended Window Manager Hints])
AC_ARG_ENABLE([ewmh],
	AS_HELP_STRING([--enable-ewmh],
		[enable support for Extended Window Manager Hints (default=yes)]),
	[], [enable_ewmh=yes]
)
AC_MSG_RESULT([$enable_ewmh])
AS_IF([test "x$enable_ewmh" = "xyes"], [
	AC_DEFINE([USE_EWMH], [1], [use extened window manager hints])
])
AM_CONDITIONAL([EWMH], [test "x$enable_ewmh" = "xyes"])

dnl Check whether to include debugging code
DEBUG=""
AC_MSG_CHECKING([whether to include verbose debugging code])
AC_ARG_ENABLE([debug],
	AS_HELP_STRING([--enable-debug],
		[include verbose debugging code (default=no)]),
	[], [enable_debug=no]
)
AC_MSG_RESULT([$enable_debug])
AS_IF([test "x$enable_debug" = "xyes"], [DEBUG="-DDEBUG -Wall"])
AC_SUBST([DEBUG])
CXXFLAGS="$CXXFLAGS $DEBUG"

dnl Check whether to build test programs
AC_MSG_CHECKING([whether to build test programs])
AC_ARG_ENABLE([test],
	AS_HELP_STRING([--enable-test],
		[build programs used in testing fluxbox (default=no)]),
	[], [enable_test=no]
)
AC_MSG_RESULT([$enable_test])
AM_CONDITIONAL([TEST], [test "x$enable_test" = "xyes"])

dnl Check whether to include native language support (i18n)
AC_MSG_CHECKING([whether to include NLS support])
AC_ARG_ENABLE([nls],
	AS_HELP_STRING([--enable-nls],
		[include native language support (default=yes)]),
	[], [enable_nls=yes]
)
AC_MSG_RESULT([$enable_nls])
AS_IF([test "x$enable_nls" = "xyes"], [
	AC_DEFINE([NLS], [1], [Native language support])
	NLS="-DNLS"
], [])
AC_SUBST([NLS])

dnl Check for new timed pixmap cache
AC_MSG_CHECKING([whether to use a timed pixmap cache])
AC_ARG_ENABLE([timedcache],
	AS_HELP_STRING([--enable-timedcache],
		[use new timed pixmap cache (default=yes)]),
	[], [enable_timedcache=yes]
)
AC_MSG_RESULT([$enable_timedcache])
AS_IF([test "x$enable_timedcached" = "xyes"], [
	AC_DEFINE([TIMEDCACHE], [1], [timed cache])
])

dnl Check for UTF-8 support
AC_MSG_CHECKING([whether to have XMB (multibyte font, utf-8) support])
AC_ARG_ENABLE([xmb],
	AS_HELP_STRING([--enable-xmb],
		[XMB (multibyte font, utf-8) support (default=yes)]),
	[], [enable_xmb=yes]
)
AC_MSG_RESULT([$enable_xmb])
AS_IF([test "x$enable_xmb" = "xyes"], [
	AC_DEFINE([USE_XMB], [1], [multibyte support])
])
AM_CONDITIONAL([MULTIBYTE], [test "x$enable_xmb" = "xyes"])

dnl Check for imlib2
have_imlib2=no
AC_ARG_ENABLE([imlib2], AS_HELP_STRING([--disable-imlib2], [disable imlib2 support]))
AS_IF([test "x$enable_imlib2" != "xno"], [
	PKG_CHECK_MODULES([IMLIB2], [ imlib2 >= 1.0.0 ],
		[AC_DEFINE([HAVE_IMLIB2], [1], [Define if imlib2 is available]) have_imlib2=yes], [have_imlib2=no])
	AS_IF([test "x$have_imlib2" = xno -a "x$enable_imlib2" = xyes], [
		AC_MSG_ERROR([*** imlib2 support requested but libraries not found])
	])
])
AM_CONDITIONAL([IMLIB2], [test "$have_imlib2" = "yes"])

dnl Check for X fonts
have_xft=no
AC_ARG_ENABLE([xft], AS_HELP_STRING([--disable-xft], [disable xft support]))
AS_IF([test "x$enable_xft" != "xno"], [
	PKG_CHECK_MODULES([XFT], [ xft ],
		[AC_DEFINE([USE_XFT], [1], [Define if xft is available]) have_xft=yes], [have_xft=no])
	AS_IF([test "x$have_xft" = xno -a "x$enable_xft" = xyes], [
		AC_MSG_ERROR([*** xft support requested but libraries not found])
	])
])
AM_CONDITIONAL([XFT], [test "$have_xft" = "yes"])
AS_IF([test "$have_xft" = "yes"], [
	AC_CHECK_LIB([Xft], [XftDrawStringUtf8], [have_utf8=yes], [have_utf8=no])
	AS_IF([test "$have_utf8" = yes], [
		AC_DEFINE([HAVE_XFT_UTF8_STRING], [1], [Define if you have XftDrawStringUtf8])
	])
	PKG_CHECK_MODULES([FONTCONFIG], [ fontconfig ], [], AC_MSG_ERROR([Could not find -lfontconfig.]))
])

dnl Check for freetype
have_freetype2=no
AC_ARG_ENABLE([freetype2], AS_HELP_STRING([--disable-freetype2], [disable freetype2 support]))
AS_IF([test "x$enable_freetype2" != "xno"], [
	PKG_CHECK_MODULES([FREETYPE2], [ freetype2 ],
		[AC_DEFINE([HAVE_FREETYPE2], [1], [Define if freetype2 is available]) have_freetype2=yes], [have_freetype2=no])
	AS_IF([test "x$have_freetype2" = xno -a "x$enable_freetype2" = xyes], [
		AC_MSG_ERROR([*** freetype2 support requested but libraries not found])
	])
])
AM_CONDITIONAL([FREETYPE2], [test "$have_freetype2" = "yes"])

dnl Check for xrender
have_xrender=no
AC_ARG_ENABLE([xrender], AS_HELP_STRING([--disable-xrender], [disable xrender support]))
AS_IF([test "x$enable_xrender" != "xno"], [
	PKG_CHECK_MODULES([XRENDER], [ xrender ],
		[AC_DEFINE([HAVE_XRENDER], [1], [Define if xrender is available]) have_xrender=yes], [have_xrender=no])
	AS_IF([test "x$have_xrender" = xno -a "x$enable_xrender" = xyes], [
		AC_MSG_ERROR([*** xrender support requested but libraries not found])
	])
])
AM_CONDITIONAL([XRENDER], [test "$have_xrender" = "yes"])

dnl Check for xpm
have_xpm=no
AC_ARG_ENABLE([xpm], AS_HELP_STRING([--disable-xpm], [disable xpm support]))
AS_IF([test "x$enable_xpm" != "xno"], [
	PKG_CHECK_MODULES([XPM], [ xpm ],
		[AC_DEFINE([HAVE_XPM], [1], [Define if xpm is available]) have_xpm=yes], [have_xpm=no])
	AS_IF([test "x$have_xpm" = xno -a "x$enable_xpm" = xyes], [
		AC_MSG_ERROR([*** xpm support requested but libraries not found])
	])
])
AM_CONDITIONAL([XPM], [test "$have_xpm" = "yes"])

dnl Check for Xinerama support and proper library files.
have_xinerama=no
AC_ARG_ENABLE([xinerama], AS_HELP_STRING([--disable-xinerama], [disable xinerama support]))
AS_IF([test "x$enable_xinerama" != "xno"], [
	PKG_CHECK_MODULES([XINERAMA], [ xinerama ],
		[AC_DEFINE([XINERAMA], [1], [Define if xinerama is available]) have_xinerama=yes], [have_xinerama=no])
	AS_IF([test "x$have_xinerama" = xno -a "x$enable_xinerama" = xyes], [
		AC_MSG_ERROR([*** xinerama support requested but libraries not found])
	])
])
AM_CONDITIONAL([XINERAMA], [test "$have_xinerama" = "yes"])

dnl Check for XShape extension support and proper library files.
have_xext=no
AC_ARG_ENABLE([xext], AS_HELP_STRING([--disable-xext], [disable Misc X Extension Library support]))
AS_IF([test "x$enable_xext" != "xno"], [
	PKG_CHECK_MODULES([XEXT], [ xext ],
		[AC_DEFINE([SHAPE], [1], [Define if xext is available]) have_xext=yes], [have_xext=no])
	AS_IF([test "x$have_xext" = xno -a "x$enable_xext" = xyes], [
		AC_MSG_ERROR([*** xext support requested but libraries not found])
	])
])
AM_CONDITIONAL([XEXT], [test "$have_xext" = "yes"])

dnl Check for RANDR support and proper library files.
have_xrandr=no
AC_ARG_ENABLE([xrandr], AS_HELP_STRING([--disable-xrandr], [disable xrandr support]))
AS_IF([test "x$enable_xrandr" != "xno"], [
	PKG_CHECK_MODULES([RANDR], [ xrandr >= 1.4.0 ],
		[AC_DEFINE([HAVE_RANDR], [1], [Define if xrandr is available]) have_xrandr=yes], [have_xrandr=no])
	AS_IF([test "x$have_xrandr" = xno -a "x$enable_xrandr" = xyes], [
		AC_MSG_ERROR([*** xrandr support requested but libraries not found])
	])
])
AM_CONDITIONAL([RANDR], [test "$have_xrandr" = "yes"])

dnl Check for fribidi
have_fribidi=no
AC_ARG_ENABLE([fribidi], AS_HELP_STRING([--disable-fribidi], [disable fribidi support]))
AS_IF([test "x$enable_fribidi" != "xno"], [
	PKG_CHECK_MODULES([FRIBIDI], [ fribidi ],
		[AC_DEFINE([HAVE_FRIBIDI], [1], [Define if fribidi is available]) have_fribidi=yes], [have_fribidi=no])
	AS_IF([test "x$have_fribidi" = xno -a "x$enable_fribidi" = xyes], [
		AC_MSG_ERROR([*** fribidi support requested but libraries not found])
	])
])
AM_CONDITIONAL([FRIBIDI], [test "$have_fribidi" = "yes"])

dnl Various resource paths
AC_ARG_WITH([menu],
	AS_HELP_STRING([--with-menu=path], [location menu file (PREFIX/share/fluxbox/menu)]),
	[default_menu="$withval"],
	[default_menu="$prefix/share/fluxbox/menu"]
)
AC_SUBST([DEFAULT_MENU], [$default_menu])

AC_ARG_WITH([windowmenu],
	AS_HELP_STRING([--with-windowmenu=path], [location windowmenu file (PREFIX/share/fluxbox/windowmenu)]),
	[default_windowmenu="$withval"],
	[default_windowmenu="$prefix/share/fluxbox/windowmenu"]
)
AC_SUBST([DEFAULT_WINDOWMENU], [$default_windowmenu])

AC_ARG_WITH([style],
	AS_HELP_STRING([--with-style=path], [style by default (PREFIX/share/fluxbox/styles/bloe)]),
	[default_style="$withval"],
	[default_style="$prefix/share/fluxbox/styles/bloe"]
)
AC_SUBST([DEFAULT_STYLE], [$default_style])

AC_ARG_WITH([keys],
	AS_HELP_STRING([--with-keys=path], [location keys file (PREFIX/share/fluxbox/keys)]),
	[default_keys="$withval"],
	[default_keys="$prefix/share/fluxbox/keys"]
)
AC_SUBST([DEFAULT_KEYS], [$default_keys])

AC_ARG_WITH([apps],
	AS_HELP_STRING([--with-apps=path], [location apps file (PREFIX/share/fluxbox/apps)]),
	[default_apps="$withval"],
	[default_apps="$prefix/share/fluxbox/apps"]
)
AC_SUBST([DEFAULT_APPS], [$default_apps])

AC_ARG_WITH([overlay],
	AS_HELP_STRING([--with-overlay=path], [location overlay file (PREFIX/share/fluxbox/overlay)]),
	[default_overlay="$withval"],
	[default_overlay="$prefix/share/fluxbox/overlay"]
)
AC_SUBST([DEFAULT_OVERLAY], [$default_overlay])

AC_ARG_WITH([init],
	AS_HELP_STRING([--with-init=path], [location init file (PREFIX/share/fluxbox/init)]),
	[default_init="$withval"],
	[default_init="$prefix/share/fluxbox/init"]
)
AC_SUBST([DEFAULT_INIT], [$default_init])

dnl we have to expand locale_path in the config.h file, but NOT in the
dnl makefiles!
AC_ARG_WITH([locale],
	AS_HELP_STRING([--with-locale=path], [location for nls files (PREFIX/share/fluxbox/nls)]), [
	locale_path="$withval"
	AC_DEFINE_UNQUOTED([LOCALEPATH], ["$locale_path"], [location for nls files])
], [
	locale_path="$prefix/share/fluxbox/nls"
	AC_DEFINE_UNQUOTED([LOCALEPATH], ["$prefix/share/fluxbox/nls"], [location for nls files])
])
AC_SUBST([LOCALE_PATH], [$locale_path])

AC_SUBST([program_prefix])
AC_SUBST([program_suffix])

dnl Determine the return type of signal handlers
AC_TYPE_SIGNAL

dnl Output files
AC_CONFIG_HEADER([config.h])

AC_OUTPUT([
Makefile
version.h
nls/C/Makefile
nls/be_BY/Makefile
nls/bg_BG/Makefile
nls/cs_CZ/Makefile
nls/da_DK/Makefile
nls/de_AT/Makefile
nls/de_CH/Makefile
nls/de_DE/Makefile
nls/el_GR/Makefile
nls/en_GB/Makefile
nls/en_US/Makefile
nls/es_AR/Makefile
nls/es_ES/Makefile
nls/et_EE/Makefile
nls/fi_FI/Makefile
nls/fr_CH/Makefile
nls/fr_FR/Makefile
nls/he_IL/Makefile
nls/it_IT/Makefile
nls/ja_JP/Makefile
nls/ko_KR/Makefile
nls/lv_LV/Makefile
nls/mk_MK/Makefile
nls/nb_NO/Makefile
nls/nl_NL/Makefile
nls/no_NO/Makefile
nls/pl_PL/Makefile
nls/pt_BR/Makefile
nls/pt_PT/Makefile
nls/ru_RU/Makefile
nls/sk_SK/Makefile
nls/sl_SI/Makefile
nls/sv_SE/Makefile
nls/tr_TR/Makefile
nls/uk_UA/Makefile
nls/vi_VN/Makefile
nls/zh_CN/Makefile
nls/zh_TW/Makefile
])

MSG_RESULT_CXXFLAGS="$FRIBIDI_CFLAGS $XRANDR_CFLAGS $AM_CPPFLAGS $CXXFLAGS"
MSG_RESULT_LIBS="$LDADD $FONTCONFIG_LIBS $FREETYPE2_LIBS $FRIBIDI_LIBS $IMLIB2_LIBS $RANDR_LIBS $XEXT_LIBS $XFT_LIBS $XINERAMA_LIBS $XPM_LIBS $XRENDER_LIBS"

dnl Print results
AC_MSG_RESULT([])
AC_MSG_RESULT([	$PACKAGE version $VERSION configured successfully.])
AC_MSG_RESULT([])
AC_MSG_RESULT([Using:])
AC_MSG_RESULT([	'$prefix' for installation.])
AC_MSG_RESULT([	'$default_menu' for location menu file.])
AC_MSG_RESULT([	'$default_windowmenu' for location window menu file.])
AC_MSG_RESULT([	'$default_style' by default style.])
AC_MSG_RESULT([	'$default_keys' for location keys file.])
AC_MSG_RESULT([	'$default_init' for location init file.])
AC_MSG_RESULT([	'$locale_path' for nls files.])
AC_MSG_RESULT([	'$CXX' for C++ compiler.])
AC_MSG_RESULT([])
AC_MSG_RESULT([Building with:])
AC_MSG_RESULT([	'$MSG_RESULT_CXXFLAGS' for C++ compiler flags.])
AC_MSG_RESULT([	'$MSG_RESULT_LIBS' for linker flags.])
AC_MSG_RESULT([])
AC_MSG_RESULT([Now build $PACKAGE with 'make'])
AC_MSG_RESULT([])
