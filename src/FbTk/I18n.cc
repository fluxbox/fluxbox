// I18n.hh for Fluxbox Window Manager
// Copyright (c) 2001 - 2006 Henrik Kinnunen (fluxgen at fluxbox dot org)
//
// I18n.cc for Blackbox - an X11 Window manager
// Copyright (c) 1997 - 2000 Brad Hughes (bhughes@tcac.net)
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
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
// THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
// DEALINGS IN THE SOFTWARE.

/* Note:
 * A good reference for the older non-gettext style I18n
 * functions is the "Locale tutorial"
 *     Written by Patrick D'Cruze (pdcruze@orac.iinet.com.au)
 * A copy of which is available (at the time of writing) here:
 * http://www.kulichki.com/moshkow/CYRILLIC/locale-tutorial-0_8.txt
 */

#include "I18n.hh"
#include "FileUtil.hh"

#include <X11/Xlocale.h>

#include <cstdlib>
#include <cstring>
#include <cstdarg>
#include <iostream>

#ifdef HAVE_LOCALE_H
#include <locale.h>
#endif // HAVE_LOCALE_H

#ifdef HAVE_NL_TYPES_H
// this is needed for linux libc5 systems
extern "C" {
#include <nl_types.h>
}
#elif defined(__CYGWIN__) || defined(__EMX__) || defined(__APPLE__)
extern "C" {
typedef int nl_catd;
char *catgets(nl_catd cat, int set_number, int message_number, char *message);
nl_catd catopen(char *name, int flag);
void catclose(nl_catd cat);
}

#endif // HAVE_NL_TYPES_H


using std::cerr;
using std::endl;
using std::string;

namespace {

#if defined(NLS)

const char     UTF8_SUFFIX[]     = "-UTF-8.cat";
const size_t   UTF8_SUFFIX_LEN   = sizeof(UTF8_SUFFIX)-1; // without \0
const char     DEFAULT_CATFILE[] = "fluxbox.cat";
const char     ENV_CATFILE[]     = "FLUXBOX_CATFILE";
const char     ENV_CATDIR[]      = "FLUXBOX_CATDIR";


const char* getCatalogDir() {
    const char* cat_dir = getenv(ENV_CATDIR);
    if (cat_dir) {
        return cat_dir;
    }
    return LOCALEPATH;
}

#endif


const nl_catd  INVALID_CATALOG   = (nl_catd)(-1);
nl_catd        s_catalog_fd      = INVALID_CATALOG;


std::string join_str(size_t n, ...) {
    std::string s;
    va_list args;
    va_start(args, n);
    for (; n > 0; n--) {
        s.append(va_arg(args, const char*));
    }
    return s;
}

}


namespace FbTk {


// initialize the i18n-system be opening the catalog-file
// named by 'catalog'. per default we expect 'catalog' to 
// be 0/NULL, the code picks a sane default then:
//
// - environment variable FLUXBOX_CATFILE is set? use it
// - DEFAULT_CATFILE ("fluxbox.cat")
// - the utf8 encoded translation for the current locale
//
// handling things this was allows us to test catalog files
// without putting them into the install path 
// $PREFIX/share/fluxbox/nls/XYZ/
void I18n::init(const char* catalog) {

    static bool init = false;
    if (init) {
        return;
    }

#if defined(NLS) && defined(HAVE_CATOPEN)

    if (!catalog) {
        const char* c = getenv(ENV_CATFILE);
        if (!c) {
            c = DEFAULT_CATFILE;
        }
        catalog = c;
    }

    FbStringUtil::init();

    int flag;

    I18n& i18n = I18n::instance();
    const string dir = getCatalogDir();
    const string locale = i18n.m_locale;
    string clean_locale = locale;
    size_t i;

    // clean the locale, we have to append something later on
    i = clean_locale.find('.');
    if (i != string::npos)
        clean_locale.erase(i);

#ifdef MCLoadBySet
    flag = MCLoadBySet;
#else
    flag = NL_CAT_LOCALE;
#endif

    struct { std::string catalog; std::string locale; bool utf8; } _catalog[] = {

        // first try pure 'catalog'. catopen() will use NLSPATH if it's
        // set and replaces '%N' by 'catalog'. eg: with catalog="fluxbox.cat"
        // "/usr/share/fluxbox/nls/C/%N" becomes "/usr/share/fluxbox/nls/C/fluxbox.cat"
        { string(catalog), locale, false },

        // try full-path to 'catalog'
        { join_str(5, dir.c_str(), "/", locale.c_str(), "/", catalog), locale, false },

        // try the UTF-8 catalog, this also picks up situations where
        // the codeset somehow isn't specified
        { join_str(5, dir.c_str(), "/", clean_locale.c_str(), ".UTF-8/", catalog),
            join_str(2, clean_locale.c_str(), ".UTF8"), true},

    };

    for (i = 0; i < sizeof(_catalog)/sizeof(_catalog[0]); i++) {

        if (_catalog[i].utf8 && locale == "C") {
            continue;
        }

        const char* fname = _catalog[i].catalog.c_str();

        s_catalog_fd = catopen(fname, flag);
        if (s_catalog_fd == INVALID_CATALOG) {
            continue;
        }

        i18n.m_locale = _catalog[i].locale;
        if (FbStringUtil::haveUTF8()) {
            if (_catalog[i].utf8) {
                i18n.m_utf8_translate = true;
            } else {
                size_t n = _catalog[i].catalog.rfind(UTF8_SUFFIX);
                if (n != std::string::npos && (n + UTF8_SUFFIX_LEN) == _catalog[i].catalog.size()) {
                    i18n.m_utf8_translate = true;
                }
            }
        }
        break;
    }

    if (s_catalog_fd == INVALID_CATALOG) {
        cerr<<"Warning: Failed to open file("<< catalog <<")"<<endl
            <<"for translation, using default messages."<<endl;
    }
#endif // HAVE_CATOPEN
}

I18n::I18n() : m_multibyte(false), m_utf8_translate(false) {
#if defined(HAVE_SETLOCALE) && defined(NLS)
    //make sure we don't get 0 to m_locale string
    char *temp = setlocale(LC_MESSAGES, "");
    m_locale = ( temp ?  temp : "");
    if (m_locale.empty()) {
        cerr<<"Warning: Failed to set locale, reverting to \"C\""<<endl;
#endif // defined(HAVE_SETLOCALE) && defined(NLS)

        m_locale = "C";

#if defined(HAVE_SETLOCALE) && defined(NLS)

    } else {

        setlocale(LC_TIME, "");
        // MB_CUR_MAX returns the size of a char in the current locale
        if (MB_CUR_MAX > 1)
            m_multibyte = true;

        // truncate any encoding off the end of the locale

        string::size_type index = m_locale.find('@');
        if (index != string::npos)
            m_locale.erase(index);

        index = m_locale.find('=');
        if (index != string::npos)
            m_locale.erase(0, index+1);
    }
#endif // defined(HAVE_SETLOCALE) && defined(NLS)
}


I18n::~I18n() {

#if defined(NLS) && defined(HAVE_CATCLOSE)
    if (s_catalog_fd != INVALID_CATALOG)
        catclose(s_catalog_fd);
#endif // HAVE_CATCLOSE
}

I18n& I18n::instance() {
    static I18n singleton; //singleton object
    return singleton;
}

// Translate_FB means it'll become an FbString that goes to X for Fonts,
// No translate means it stays in the local encoding, for printing to the
// console.
FbString I18n::getMessage(int set_number, int message_number,
                             const char *default_message, bool translate_fb) const {

    FbString msg(default_message);

#if defined(NLS) && defined(HAVE_CATGETS)
    if (s_catalog_fd != INVALID_CATALOG) {
        const char *ret = catgets(s_catalog_fd, set_number, message_number, default_message);

        if (ret == default_message || ret == NULL) {
             // can't translate, leave it in raw ascii (utf-8 compatible)
        } else if (!m_utf8_translate && translate_fb)
            // Local input, UTF-8 output
            msg = FbStringUtil::LocaleStrToFb(ret);
        else if (m_utf8_translate && !translate_fb)
            // UTF-8 input, local output
            msg = FbStringUtil::FbStrToLocale(ret);
        else
            // UTF-8 input, UTF-8 output OR
            // local input, local output
            msg = ret;
    }

#endif // NLS && HAVE_CATGETS
    return msg;
}

} // end namespace FbTk
