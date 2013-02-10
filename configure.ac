dnl Initialize autoconf and automake
AC_PREREQ(2.52)
AC_INIT([fluxbox],
	[1.3.4],
	[fluxbox-devel@lists.sourceforge.net],,
	[http://fluxbox.org/])
AC_CONFIG_SRCDIR([src/fluxbox.cc])
AM_INIT_AUTOMAKE([foreign 1.10 tar-pax dist-bzip2])

dnl Determine default prefix
AS_IF(test "x$prefix" = "xNONE",[prefix="$ac_default_prefix"])

dnl Check for various flavors of UNIX(r)
dnl AC_AIX
dnl AC_ISC_POSIX

AC_LANG_CPLUSPLUS

dnl Locate required external software
AC_PROG_CC
AC_PROG_CXX
AC_PROG_INSTALL
AC_PROG_RANLIB

AC_C_CONST
AC_C_INLINE

AC_TYPE_PID_T
AC_TYPE_SIZE_T


AC_CHECK_PROGS(regex_cmd, sed)
AS_IF(test "x$regex_cmd" = "x",[AC_MSG_ERROR([error. sed is required to build the data files.])])

dnl Check for system header files
AC_HEADER_STDC
AC_HEADER_STDBOOL
AC_CHECK_HEADERS(errno.h ctype.h dirent.h fcntl.h libgen.h \
		 locale.h math.h nl_types.h process.h signal.h stdarg.h \
		 stdint.h stdio.h time.h unistd.h \
		 sys/param.h sys/select.h sys/signal.h sys/stat.h \
		 sys/time.h sys/types.h sys/wait.h \
		 langinfo.h iconv.h)



AC_CHECK_HEADERS(sstream,,[
    AC_CHECK_HEADERS(strstream,,[
	AC_MSG_ERROR([Your libstdc++ doesn't have the sstream or strstream classes])]
	)]
)

AC_CHECK_HEADERS(cassert cctype cerrno cmath cstdarg cstdint cstdio cstdlib cstring ctime)


dnl Check for existance of basename(), setlocale() and strftime()
AC_FUNC_CLOSEDIR_VOID
AC_FUNC_ERROR_AT_LINE
AC_FUNC_FORK
AC_FUNC_MALLOC
AC_FUNC_REALLOC
AC_FUNC_SELECT_ARGTYPES
AC_FUNC_STAT

AC_CHECK_FUNCS(basename, , AC_CHECK_LIB(gen, basename, LIBS="-lgen $LIBS"))
AC_CHECK_FUNCS(catclose catgets catopen getpid memset mkdir \
    nl_langinfo putenv regcomp select setenv setlocale sigaction snprintf \
    sqrt strcasecmp strcasestr strchr strstr strtol strtoul sync vsnprintf)


dnl Windows requires the mingw-catgets library for the catgets function.
AC_SEARCH_LIBS([catgets], [catgets], [], [])

dnl The autoconf test for strftime is broken now (due to gcc 3.3 bug?):
dnl Gcc 3.3 testprog = ``extern "C" char strftime;'', build with g++ test.cc
dnl breaks with:
dnl   test.cc:1: error: nonnull argument with out-of-range operand number
dnl   (arg 1, operand 3)


AC_MSG_CHECKING(for strftime)

AC_COMPILE_IFELSE(
[AC_LANG_PROGRAM([#include <time.h>],
[[
char * s;
time_t t = time(NULL);
size_t x = strftime(s, 5, "%a", localtime(&t));
]]
)],
[
	AC_DEFINE(HAVE_STRFTIME, 1, [Define to 1 if you have the 'strftime' function.])
	AC_MSG_RESULT(yes)
],
[AC_MSG_RESULT(no)])


AC_MSG_CHECKING(for clock_gettime)
AC_COMPILE_IFELSE(
    [AC_LANG_PROGRAM(
	[#include <time.h>],
	[[
	clock_gettime(CLOCK_MONOTONIC, 0);
	return 0;
	]]
    )],
    [
	AC_DEFINE(HAVE_CLOCK_GETTIME, 1, [Define to 1 if you have the 'clock_gettime' function.])
	AC_MSG_RESULT(yes)
	# *bsd has clock_gettime() in libc
	AC_CHECK_LIB(rt, clock_gettime, LIBS="-lrt $LIBS")
    ],
    [
	AC_MSG_RESULT(no)
    ]
)

AC_MSG_CHECKING(for mach_absolute_time)
AC_COMPILE_IFELSE(
    [AC_LANG_PROGRAM(
	[#include <mach/mach_time.h>],
	[[
	mach_absolute_time();
	return 0;
	]]
    )],
    [
	AC_DEFINE(HAVE_MACH_ABSOLUTE_TIME, 1, [Define to 1 if you have the 'mach_absolute_time' function.])
	AC_MSG_RESULT(yes)
	#AC_CHECK_LIB(, clock_gettime, LIBS="-lrt $LIBS")
    ],
    [
	AC_MSG_RESULT(no)
    ]
)




AC_STRUCT_TM

dnl ---------------
dnl CHECK FOR ICONV
dnl ---------------
dnl Find iconv. It may be in libiconv and may be iconv() or libiconv()
if test "x$ac_cv_header_iconv_h" = "xyes"; then
    ac_found_iconv=no
    AC_COMPILE_IFELSE(
	[AC_LANG_PROGRAM(
	[[#include <stdlib.h>
	  #include <iconv.h>]],
	[[
	  iconv_open(NULL, NULL);
	  return 0;
	]]
	)],
	[
	    ac_found_iconv=yes
	],
	[ ])

    AC_COMPILE_IFELSE(
	[AC_LANG_PROGRAM(
	[[#include <stdlib.h>
	  #include <iconv.h>]],
	[[
	  libiconv_open(NULL, NULL);
	  return 0;
	]]
	)],
	[
	    ac_found_iconv=yes
	],
	[ ])

    if test "x$ac_found_iconv" = xyes; then
	AC_DEFINE(HAVE_ICONV, 1, [Define if you have the iconv() function.])
	AC_CHECK_LIB(iconv, iconv_open, LIBS="-liconv $LIBS")
	AC_CHECK_LIB(iconv, libiconv_open, LIBS="-liconv $LIBS")

dnl Check if iconv uses const in prototype declaration
	AC_CACHE_CHECK(for iconv declaration,
			ac_cv_iconv_const,
			[AC_COMPILE_IFELSE(
			    [AC_LANG_PROGRAM(
			     [[#include <stdlib.h>
			       #include <iconv.h>]],
			     [[
			       char **msg, **new_msg;
			       size_t result, inleft, outleft;
			       result = iconv((iconv_t)(-1), msg, &inleft, new_msg, &outleft);
			       return 0;
			     ]]
			     )],
			     [ac_cv_iconv_const=no],
			     [ac_cv_iconv_const=yes])
			])
	if test "x$ac_cv_iconv_const" = xyes; then
	    AC_DEFINE(HAVE_CONST_ICONV, 1, [Define if you have the iconv() function.])
	fi
    fi
fi

AC_CHECK_LIB(nsl, t_open, LIBS="-lnsl $LIBS")
AC_CHECK_LIB(socket, socket, LIBS="-lsocket $LIBS")

dnl Check for X headers and libraries
AC_PATH_X
AC_PATH_XTRA

test x$no_x = "xyes" && AC_MSG_ERROR([Fluxbox requires the X Window System libraries and headers.])

AS_IF(test "x$x_includes" = "x",[x_includes="/usr/include"])
AS_IF(test "x$x_libraries" = "x",[x_libraries="/usr/lib"])

CFLAGS="$CFLAGS $X_CFLAGS"
CXXFLAGS="$CXXFLAGS $X_CFLAGS"
LIBS="$X_LIBS $LIBS"
LDFLAGS="$LDFLAGS $LIBS $X_PRE_LIBS"


dnl Check for required functions in -lX11
AC_CHECK_LIB(X11, XOpenDisplay, LIBS="-lX11 $LIBS", AC_MSG_ERROR([Could not find XOpenDisplay in -lX11.]))
LIBS="$X_EXTRA_LIBS $LIBS"

AC_CHECK_LIB(xpg4, setlocale, LIBS="-lxpg4 $LIBS")
AC_CHECK_PROGS(gencat_cmd, gencat)
AS_IF(test "x$gencat_cmd" = "x",[NLS=""],[])

Xext_lib=""



dnl Check for Remember options
AC_MSG_CHECKING([whether to include remember functionality])
AC_ARG_ENABLE(remember,
    AS_HELP_STRING([--enable-remember],[include Remembering attributes (default=yes)]),,[enable_remember=yes])
AC_MSG_RESULT([$enable_remember])
AS_IF(test "x$enable_remember" = "xyes",[AC_DEFINE(REMEMBER, 1, " compile with remember")],[])
AM_CONDITIONAL(REMEMBER_SRC, test "x$enable_remember" = "xyes")


AC_MSG_CHECKING([whether to have (POSIX) regular expression support])
AC_ARG_ENABLE(regexp,
    AS_HELP_STRING([--enable-regexp],[regular expression support (default=yes)]),,[enable_regexp=yes])
AS_IF(test "x$enable_regexp" = "xyes",[
    AC_EGREP_HEADER([regex_t],regex.h,
	AC_DEFINE(USE_REGEXP, 1, "Regular Expression support")
	AC_MSG_RESULT([yes])
	REGEXP_SRC=true,
	AC_MSG_RESULT([no])
	REGEXP_SRC=false
    )],[])
AM_CONDITIONAL(REGEXP_SRC, test x$REGEXP_SRC = xtrue)




dnl Check for the Slit
AC_MSG_CHECKING([whether to include the Slit])
AC_ARG_ENABLE(slit,
    AS_HELP_STRING([--enable-slit],[include code for the Slit (default=yes)]),,[enable_slit=yes])
AC_MSG_RESULT([$enable_slit])
AS_IF(test "x$enable_slit" = "xyes",[AC_DEFINE(USE_SLIT, 1, " compile with slit")],[])
AM_CONDITIONAL(SLIT_SRC, test "x$enable_slit" = "xyes")



dnl Check for Systemtray options
AC_MSG_CHECKING([whether to include SystemTray])
AC_ARG_ENABLE(systray,
    AS_HELP_STRING([--enable-systray],[include SystemTray (default=yes)]),,[enable_systray=yes])
AC_MSG_RESULT([$enable_systray])
AS_IF(test "x$enable_systray" = "xyes",[AC_DEFINE(USE_SYSTRAY, 1, " compile with systemtray")],[])
AM_CONDITIONAL(SYSTRAY_SRC, test "x$enable_systray" = "xyes")

dnl Check for Toolbar options
AC_MSG_CHECKING([whether to include Toolbaddr])
AC_ARG_ENABLE(toolbar,
    AS_HELP_STRING([--enable-toolbar],[include Toolbar (default=yes)]),,[enable_toolbar=yes])
AC_MSG_RESULT([$enable_toolbar])
AS_IF(test "x$enable_systray" = "xyes" -a "x$enable_toolbar" != "xyes",[
    AC_MSG_RESULT([disabled toolbar, but enabled systray => reenable toolbar])
    AS_VAR_SET(enable_toolbar, "yes")],[])
AS_IF(test "x$enable_toolbar" = "xyes",[AC_DEFINE(USE_TOOLBAR, 1, " compile with toolbar")],[])
AM_CONDITIONAL(TOOLBAR_SRC, test "x$enable_toolbar" = "xyes")



AC_MSG_CHECKING([whether to support Extended Window Manager Hints])
AC_ARG_ENABLE(ewmh,
    AS_HELP_STRING([--enable-ewmh],[enable support for Extended Window Manager Hints (default=yes)]),,[enable_ewmh=yes])
AC_MSG_RESULT([$enable_ewmh])
AS_IF(test "x$enable_ewmh" = "xyes",[AC_DEFINE(USE_EWMH, 1, "use extened window manager hints")],[])
AM_CONDITIONAL(EWMH, test "x$enable_ewmh" = "xyes")



dnl Check whether to include debugging code
DEBUG=""
AC_MSG_CHECKING([whether to include verbose debugging code])
AC_ARG_ENABLE(debug,
  AS_HELP_STRING([--enable-debug],[include verbose debugging code (default=no)]),,[enable_debug=no])
AC_MSG_RESULT([$enable_debug])
AS_IF(test "x$enable_debug" = "xyes",[ DEBUG="-DDEBUG -Wall" ],[])
AC_SUBST(DEBUG)
CXXFLAGS="$CXXFLAGS $DEBUG"



dnl Check whether to build test programs
AC_MSG_CHECKING([whether to build test programs])
AC_ARG_ENABLE(test,
    AS_HELP_STRING([--enable-test],[build programs used in testing fluxbox (default=no)]),,[enable_test=no])
AC_MSG_RESULT([$enable_test])
AM_CONDITIONAL(TEST, test "x$enable_test" = "xyes")



dnl Check whether to include native language support (i18n)
AC_MSG_CHECKING([whether to include NLS support])
AC_ARG_ENABLE(nls,
    AS_HELP_STRING([--enable-nls],[include native language support (default=no)]),,[enable_nls=yes])
AC_MSG_RESULT([$enable_nls])
AS_IF(test "x$enable_nls" = "xyes",[
    AC_DEFINE(NLS, 1, "Native language support")
    NLS="-DNLS"],[])
AC_SUBST(NLS)




dnl Check for new timed pixmap cache
AC_MSG_CHECKING([whether to use a timed pixmap cache])
AC_ARG_ENABLE(timedcache,
    AS_HELP_STRING([--enable-timedcache],[use new timed pixmap cache (default=yes)]),,[enable_timedcache=yes])
AC_MSG_RESULT([$enable_timedcache])
AS_IF(test "x$enable_timedcached" = "xyes",[
    AC_DEFINE(TIMEDCACHE, 1, "timed cache")],[])



AC_MSG_CHECKING([whether to have Xft support])
AM_PATH_XFT(yes, XFT=true, XFT=false)

AS_IF(test "x$XFT" = "xtrue",[
	AC_TRY_LINK([
			#include <X11/Xft/Xft.h>
		], [ XftFontClose(0, 0); return 1; ],
	[

		AC_DEFINE(USE_XFT, 1, "antialias support")
	AC_MSG_CHECKING([Xft UTF-8 support])
	AC_TRY_LINK([
			#include <X11/Xft/Xft.h>
		], [ XftDrawStringUtf8(0, 0, 0, 0, 0, 0, 0); return 0; ],
		AC_DEFINE(HAVE_XFT_UTF8_STRING, 1, "Xft UTF8 support")
	FEATURES="$FEATURES XFT"
		AC_MSG_RESULT(yes),
		AC_MSG_RESULT(no)
		)
	],
	[	AC_MSG_RESULT([Could not link with Xft. Install Xft if you want support for it.])
		XFT=false
	])])

AM_CONDITIONAL(XFT, test "x$XFT" = "xtrue")


AC_MSG_CHECKING([whether to have XRENDER (transparent) support])
AC_ARG_ENABLE(xrender,
	      AS_HELP_STRING([--enable-xrender],[XRENDER (transparent) support (default=yes)]),,[enable_xrender=yes])
AC_MSG_RESULT([$enable_xrender])
AS_IF(test "x$enable_xrender" = "xyes",[
    AC_CHECK_LIB(Xrender, XRenderCreatePicture,
	AC_DEFINE(HAVE_XRENDER, 1, "Xrender support")
	LIBS="-lXrender $LIBS")
    ],[])


XPM=false
AC_MSG_CHECKING([whether to have XPM (pixmap themes) support])
AC_ARG_ENABLE(xpm,
    AS_HELP_STRING([--enable-xpm],[XPM (pixmap themes) support (default=yes)]),[],[enable_xpm=yes])
AC_MSG_RESULT([$enable_xpm])
AS_IF(test "x$enable_xpm" = "xyes",[
   AC_CHECK_LIB(Xpm, XpmReadFileToPixmap,
		AC_DEFINE(HAVE_XPM, 1, "Xpm support")
		XPM=true
		LIBS="-lXpm $LIBS")
   ],[])

AM_CONDITIONAL(XPM, test "x$XPM" = "xtrue")

dnl Check whether to use imlib2
IMLIB2=false
AC_MSG_CHECKING([whether to have IMLIB2 (pixmap themes) support])
AC_ARG_ENABLE(imlib2,
    AS_HELP_STRING([--enable-imlib2], [IMLIB2 (pixmap themes) support (default=yes)]), , [enable_imlib2=yes])
AC_MSG_RESULT([$enable_imlib2])
AS_IF(test x$enable_imlib2 = "xyes",[
    AC_PATH_GENERIC(imlib2, 1.0.0,[
	IMLIB2=true
	AC_DEFINE(HAVE_IMLIB2, [], [Imlib2 support])
	LIBS="$IMLIB2_LIBS $LIBS"
	CXXFLAGS="$CXXFLAGS $IMLIB2_CFLAGS"
	FEATURES="$FEATURES IMLIB2"
	], []
	)],[])

AM_CONDITIONAL(IMLIB2, test "x$IMLIB2" = "xtrue")



AC_MSG_CHECKING([whether to have XMB (multibyte font, utf-8) support])
AC_ARG_ENABLE(xmb, AS_HELP_STRING([--enable-xmb],[XMB (multibyte font, utf-8) support (default=yes)]),,[enable_xmb=yes])
AC_MSG_RESULT([$enable_xmb])
AS_IF([test "x$enable_xmb" = "xyes"],[ AC_DEFINE(USE_XMB, 1, "multibyte support")], [])
AM_CONDITIONAL(MULTIBYTE, test "x$enable_xmb" = "xyes")


dnl Check for Xinerama support and proper library files.
AC_MSG_CHECKING([whether to build support for the XINERAMA extension])
AC_ARG_ENABLE(xinerama,
	      AS_HELP_STRING([--enable-xinerama], [XINERAMA extension support (default=yes)]), ,[enable_xinerama=yes])

AS_IF(test "x$enable_xinerama" = "xyes",[
    AC_CHECK_LIB(Xinerama, XineramaQueryScreens,
		 AC_MSG_CHECKING([for X11/extensions/Xinerama.h])
		 AC_TRY_COMPILE(
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/extensions/Xinerama.h>
      , XineramaQueryScreens(0, 0),
		 AC_MSG_RESULT([yes])
		 AC_DEFINE(XINERAMA, [1], [Define to 1 if you have XINERAMA])
		 LIBS="-lXinerama $LIBS"
		 FEATURES="$FEATURES XINERAMA",
		 AC_MSG_RESULT([no])))],[
  AC_MSG_RESULT([no])
  CONFIGOPTS="$CONFIGOPTS --disable-xinerama"])


dnl Check for XShape extension support and proper library files.
AC_MSG_CHECKING([whether to build support for the XSHAPE extension])
AC_ARG_ENABLE(shape,
	AS_HELP_STRING([--enable-shape], [XSHAPE extension support (default=yes)]), , [enable_shape=yes])
AS_IF(test "x$enable_shape" = "xyes",[
    AC_CHECK_LIB(Xext, XShapeCombineShape,
	AC_MSG_CHECKING([for X11/extensions/shape.h])
	AC_TRY_COMPILE(
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/extensions/shape.h>
      , long foo = ShapeSet,
	AC_MSG_RESULT([yes])
	AC_DEFINE(SHAPE, [1], [Define to 1 if you have SHAPE])
	LIBS="-lXext $LIBS"
	FEATURES="$FEATURES SHAPE",
	AC_MSG_RESULT([no])))],[
  AC_MSG_RESULT([no])
  CONFIGOPTS="$CONFIGOPTS --disable-shape"])



dnl Check for RANDR support and proper library files.

AC_MSG_CHECKING([whether to build support for the XRANDR (X Resize And Rotate) extension])
AC_ARG_ENABLE(randr,
	AS_HELP_STRING([--enable-randr], [XRANDR extension support (default=no)]),,[enable_randr=yes])

AS_IF(test "x$enable_randr" = "xyes", [
    AC_MSG_RESULT([yes])
    AC_CHECK_LIB(Xrandr, XRRQueryExtension, [
	AC_MSG_CHECKING([for X11/extensions/Xrandr.h])
	AC_COMPILE_IFELSE([
	    AC_LANG_PROGRAM([[#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/extensions/Xrandr.h>
		]], [[XRRQueryExtension(0, 0, 0);]]
	    )], [
	    AC_MSG_RESULT([yes])
	    AC_DEFINE(HAVE_RANDR, [1], [Define to 1 if you have RANDR])
	    LIBS="-lXrandr $LIBS"
	    FEATURES="$FEATURES RANDR"

	    AC_MSG_CHECKING([for XRRUpdateConfiguration])
	    AC_COMPILE_IFELSE([
		AC_LANG_PROGRAM([[#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/extensions/Xrandr.h>
		    ]], [[XRRUpdateConfiguration(0);]]
		)], [
		    AC_MSG_RESULT([yes])
		    AC_DEFINE(HAVE_RANDR1_2, [1], [Define to 1 if you have RANDR 1.2])
		], [
		    AC_MSG_RESULT([no])
		])
	    ], [
		AC_MSG_RESULT([no])
	    ])
	],[
	AC_MSG_RESULT([no])
    ])],[
    AC_MSG_RESULT([no])
    CONFIGOPTS="$CONFIGOPTS --disable-randr"])




AC_MSG_CHECKING([whether to have FRIBIDI support])
AC_ARG_ENABLE(fribidi,
	AS_HELP_STRING([--enable-fribidi], [FRIBIDI support (default=yes)]), , [enable_fribidi=yes])
AS_IF(test "x$enable_fribidi" = "xyes",[
	AC_MSG_RESULT([yes])
	AC_CHECK_LIB(fribidi, fribidi_version_info,
		AC_MSG_CHECKING([for fribidi/fribidi.h])
		AC_TRY_COMPILE(
#include <fribidi/fribidi.h>
      , fribidi_version_info,
			AC_MSG_RESULT([yes])
			AC_DEFINE(HAVE_FRIBIDI, [1], [Define to 1 if you have FRIBIDI])
			LIBS="-lfribidi $LIBS",
		AC_MSG_RESULT([no])))],[
    AC_MSG_RESULT([no])
    CONFIGOPTS="$CONFIGOPTS --disable-fribidi"])




AC_ARG_WITH(menu,
    AS_HELP_STRING([--with-menu=path],[location menu file (PREFIX/share/fluxbox/menu)]),
    DEFAULT_MENU=$with_menu,
    DEFAULT_MENU=\$\(prefix\)/share/fluxbox/menu
)
AC_SUBST(DEFAULT_MENU)

AC_ARG_WITH(windowmenu,
    AS_HELP_STRING([--with-windowmenu=path],[location windowmenu file (PREFIX/share/fluxbox/windowmenu)]),
    DEFAULT_WINDOWMENU=$with_windowmenu,
    DEFAULT_WINDOWMENU=\$\(prefix\)/share/fluxbox/windowmenu
)
AC_SUBST(DEFAULT_WINDOWMENU)

AC_ARG_WITH(style,
  AS_HELP_STRING([--with-style=path],[style by default (PREFIX/share/fluxbox/styles/bloe)]),
  DEFAULT_STYLE=$with_style,
  DEFAULT_STYLE=\$\(prefix\)/share/fluxbox/styles/bloe
)
AC_SUBST(DEFAULT_STYLE)

AC_ARG_WITH(keys,
  AS_HELP_STRING([--with-keys=path],[location keys file (PREFIX/share/fluxbox/keys)]),
  DEFAULT_KEYS=$with_keys,
  DEFAULT_KEYS=\$\(prefix\)/share/fluxbox/keys
)
AC_SUBST(DEFAULT_KEYS)

AC_ARG_WITH(apps,
  AS_HELP_STRING([--with-apps=path],[location apps file (PREFIX/share/fluxbox/apps)]),
  DEFAULT_APPS=$with_apps,
  DEFAULT_APPS=\$\(prefix\)/share/fluxbox/apps
)
AC_SUBST(DEFAULT_APPS)

AC_ARG_WITH(overlay,
  AS_HELP_STRING([--with-overlay=path],[location overlay file (PREFIX/share/fluxbox/overlay)]),
  DEFAULT_OVERLAY=$with_overlay,
  DEFAULT_OVERLAY=\$\(prefix\)/share/fluxbox/overlay
)
AC_SUBST(DEFAULT_OVERLAY)

AC_ARG_WITH(init,
  AS_HELP_STRING([--with-init=path],[location init file (PREFIX/share/fluxbox/init)]),
  DEFAULT_INIT=$with_init,
  DEFAULT_INIT=\$\(prefix\)/share/fluxbox/init
)
AC_SUBST(DEFAULT_INIT)

# we have to expand locale_path in the config.h file, but NOT in the makefiles!
AC_ARG_WITH(locale,
  AS_HELP_STRING([--with-locale=path],[location for nls files (PREFIX/share/fluxbox/nls)]),
  LOCALE_PATH=$with_locale
  AC_DEFINE_UNQUOTED(LOCALEPATH, "$LOCALE_PATH", "location for nls files")
  ,
  LOCALE_PATH=\$\(prefix\)/share/fluxbox/nls
  AC_DEFINE_UNQUOTED(LOCALEPATH, "$prefix/share/fluxbox/nls", "location for nls files")
)
AC_SUBST(LOCALE_PATH)

AC_SUBST(program_prefix)
AC_SUBST(program_suffix)

dnl Determine the return type of signal handlers
AC_TYPE_SIGNAL

dnl Determine if maintainer portions of the Makefiles should be included.
dnl AM_MAINTAINER_MODE

dnl Output files
AC_CONFIG_HEADER(config.h)

AC_OUTPUT(Makefile
version.h
src/Makefile
src/FbTk/Makefile
src/tests/Makefile
util/Makefile
util/fbrun/Makefile
data/Makefile
data/styles/Makefile
data/styles/BlueFlux/Makefile
data/styles/BlueFlux/pixmaps/Makefile
data/styles/Emerge/Makefile
data/styles/Emerge/pixmaps/Makefile
data/styles/arch/Makefile
data/styles/arch/pixmaps/Makefile
data/styles/zimek_bisque/Makefile
data/styles/zimek_darkblue/Makefile
data/styles/zimek_green/Makefile
data/styles/ostrich/Makefile
data/styles/green_tea/Makefile
data/styles/bora_black/Makefile
data/styles/bora_blue/Makefile
data/styles/bora_green/Makefile
data/styles/carp/Makefile
data/styles/bloe/Makefile
data/styles/bloe/pixmaps/Makefile
doc/Makefile
nls/Makefile
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
)

dnl Print results
AC_MSG_RESULT([])
AC_MSG_RESULT([	$PACKAGE version $VERSION configured successfully.])
AC_MSG_RESULT([])
AC_MSG_RESULT([Features:])
AC_MSG_RESULT([	$FEATURES])
AC_MSG_RESULT([])
AC_MSG_RESULT([Using:])
AC_MSG_RESULT([	'$prefix' for installation.])
AC_MSG_RESULT([	'$DEFAULT_MENU' for location menu file.])
AC_MSG_RESULT([	'$DEFAULT_STYLE' by default style.])
AC_MSG_RESULT([	'$DEFAULT_KEYS' for location keys file.])
AC_MSG_RESULT([	'$DEFAULT_INIT' for location init file.])
AC_MSG_RESULT([	'$LOCALE_PATH' for nls files.])
AC_MSG_RESULT([	'$CXX' for C++ compiler.])
AC_MSG_RESULT([])
AC_MSG_RESULT([Building with:])
AC_MSG_RESULT([	'$CXXFLAGS' for C++ compiler flags.])
AC_MSG_RESULT([	'$LIBS' for linker flags.])
AC_MSG_RESULT([])
AC_MSG_RESULT([Now build $PACKAGE with 'make'])
AC_MSG_RESULT([])
