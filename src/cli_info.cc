// cli_info.cc for Fluxbox Window Manager
// Copyright (c) 2014 - Mathias Gumz <akira at fluxbox.org>
//
// Permission is hereby granted, free of charge, to any person obtaining a
// copy of this software and associated documentation files (the "Software"),
// to deal in the Software without restriction, including without limitation
// the rights to use, copy, modify, merge, publish, distribute, sublicense,
// and/or sell copies of the Software, and to permit persons to whom the
// Software is furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
// THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
// DEALINGS IN THE SOFTWARE.

#include "cli.hh"
#include "defaults.hh"
#include "version.h"

#include "FbTk/I18n.hh"
#include "FbTk/StringUtil.hh"

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif // HAVE_CONFIG_H

#ifdef HAVE_CSTRING
  #include <cstring>
#else
  #include <string.h>
#endif

using std::endl;
using std::ostream;

void FluxboxCli::showInfo(ostream &ostr) {
    _FB_USES_NLS;
    ostr <<
        _FB_CONSOLETEXT(Common, FluxboxVersion, "Fluxbox version", "Fluxbox version heading")
        << ": " 
        << __fluxbox_version <<endl;

    if (strlen(gitrevision()) > 0)
        ostr << _FB_CONSOLETEXT(Common, SvnRevision, "GIT Revision", "Revision number in GIT repositary") 
            << ": " 
            << gitrevision() << endl;
#if defined(__DATE__) && defined(__TIME__)
    ostr << _FB_CONSOLETEXT(Common, Compiled, "Compiled", "Time fluxbox was compiled")
        << ": " 
        << __DATE__ 
        << " "
        << __TIME__ << endl;
#endif
#ifdef __fluxbox_compiler
    ostr << _FB_CONSOLETEXT(Common, Compiler, "Compiler", "Compiler used to build fluxbox")
        << ": "
        << __fluxbox_compiler << endl;
#endif // __fluxbox_compiler
#ifdef __fluxbox_compiler_version
    ostr << _FB_CONSOLETEXT(Common, CompilerVersion, "Compiler version", "Compiler version used to build fluxbox")
        << ": " 
        << __fluxbox_compiler_version << endl;
#endif // __fluxbox_compiler_version

    ostr << endl 
        <<_FB_CONSOLETEXT(Common, Defaults, "Defaults", "Default values compiled in") 
        << ": " << endl;

    ostr <<_FB_CONSOLETEXT(Common, DefaultMenuFile, "       menu", "default menu file (right aligned - make sure same width as other default values)")
        << ": "
        << FbTk::StringUtil::expandFilename(DEFAULTMENU) << endl;
    ostr <<_FB_CONSOLETEXT(Common, DefaultWindowMenuFile, " windowmenu", "default windowmenu file (right aligned - make sure same width as other default values)")
        << ": "
        << FbTk::StringUtil::expandFilename(DEFAULT_WINDOWMENU) << endl;
    ostr << _FB_CONSOLETEXT(Common, DefaultStyle, "      style", "default style (right aligned - make sure same width as other default values)")
        << ": "
        << FbTk::StringUtil::expandFilename(DEFAULTSTYLE) << endl;

    ostr << _FB_CONSOLETEXT(Common, DefaultKeyFile, "       keys", "default key file (right aligned - make sure same width as other default values)")
        << ": "
        << FbTk::StringUtil::expandFilename(DEFAULTKEYSFILE) << endl;
    ostr << _FB_CONSOLETEXT(Common, DefaultInitFile, "       init", "default init file (right aligned - make sure same width as other default values)")
        << ": "
        << FbTk::StringUtil::expandFilename(DEFAULT_INITFILE) << endl;

#ifdef NLS
    ostr << _FB_CONSOLETEXT(Common, DefaultLocalePath, "         nls", "location for localization files (right aligned - make sure same width as other default values)")
        << ": "
        << FbTk::StringUtil::expandFilename(LOCALEPATH) << endl;
#endif

    const char NOT[] = "-";
    ostr << endl
        << _FB_CONSOLETEXT(Common, CompiledOptions, "Compiled options", "Options used when compiled")
        << " (" << NOT << " => " 
        << _FB_CONSOLETEXT(Common, Disabled, "disabled", "option is turned off") << "): " << endl
        <<

/**** NOTE: This list is in alphabetical order! ****/

#ifndef HAVE_FRIBIDI
        NOT <<
#endif
        "BIDI" << endl <<

#ifndef DEBUG
        NOT <<
#endif // DEBUG
        "DEBUG" << endl <<

#ifndef USE_EWMH
        NOT <<
#endif // USE_EWMH
        "EWMH" << endl <<

#ifndef HAVE_IMLIB2
        NOT<<
#endif // HAVE_IMLIB2
        "IMLIB2" << endl <<

#ifndef NLS
        NOT<<
#endif // NLS
        "NLS" << endl <<

#ifndef REMEMBER
        NOT <<
#endif // REMEMBER
        "REMEMBER" << endl <<

#ifndef HAVE_XRENDER
        NOT <<
#endif // HAVE_XRENDER
        "RENDER" << endl <<

#ifndef SHAPE
        NOT <<
#endif // SHAPE
        "SHAPE" << endl <<

#ifndef USE_SLIT
        NOT <<
#endif // SLIT
        "SLIT" << endl <<

#ifndef USE_SYSTRAY
        NOT <<
#endif
        "SYSTEMTRAY" << endl <<


#ifndef USE_TOOLBAR
        NOT <<
#endif // USE_TOOLBAR
        "TOOLBAR" << endl <<

#ifndef HAVE_RANDR
        NOT <<
#endif
        "RANDR" <<
#ifdef HAVE_RANDR1_2
        "1.2" <<
#endif
        endl <<

#ifndef USE_XFT
        NOT <<
#endif // USE_XFT
        "XFT" << endl <<

#ifndef XINERAMA
        NOT <<
#endif // XINERAMA
        "XINERAMA" << endl <<

#ifndef USE_XMB
        NOT <<
#endif // USE_XMB
        "XMB" << endl <<

#ifndef HAVE_XPM
        NOT <<
#endif // HAVE_XPM
        "XPM" << endl

        << endl;
}

