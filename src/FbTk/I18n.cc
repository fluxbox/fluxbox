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

const nl_catd INVALID_CATALOG = ((nl_catd)(-1));
nl_catd s_catalog_fd = INVALID_CATALOG;

}


namespace FbTk {

void I18n::init(const char* catalog) {
    static bool init = false;
    if (init) {
        return;
    }

#if defined(NLS) && defined(HAVE_CATOPEN)

    FbStringUtil::init();

    I18n& i18n = I18n::instance();

    string filename = LOCALEPATH;
    filename += '/';
    filename += i18n.m_locale;
    filename += '/';
    filename += catalog;

    if (!FileUtil::isRegularFile(filename.c_str()) && i18n.m_locale != "C" && FbStringUtil::haveUTF8()) {
        // try the UTF-8 catalog, this also picks up situations where
        // the codeset somehow isn't specified

        // remove everything after @
        string::size_type index = i18n.m_locale.find('.');
        // erase all characters starting at index
        if (index != string::npos)
            i18n.m_locale.erase(index);

        i18n.m_locale.append(".UTF-8");
        i18n.m_utf8_translate = true;

        filename = LOCALEPATH;
        filename += '/';
        filename += i18n.m_locale;
        filename += '/';
        filename += catalog;
    }

#ifdef MCLoadBySet
    s_catalog_fd = catopen(filename.c_str(), MCLoadBySet);
#else // !MCLoadBySet
    s_catalog_fd = catopen(filename.c_str(), NL_CAT_LOCALE);
#endif // MCLoadBySet

    if (s_catalog_fd == INVALID_CATALOG) {
        cerr<<"Warning: Failed to open file("<<filename<<")"<<endl
            <<"for translation, using default messages."<<endl;
    }
#endif // HAVE_CATOPEN
}

I18n::I18n():m_multibyte(false), m_utf8_translate(false) {
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

        // remove everything after @
        string::size_type index = m_locale.find('@');
        if (index != string::npos)
            m_locale.erase(index); //erase all characters starting at index
        // remove everything before =
        index = m_locale.find('=');
        if (index != string::npos)
            m_locale.erase(0,index+1); //erase all characters starting up to index
    }
#endif // defined(HAVE_SETLOCALE) && defined(NLS)
}


I18n::~I18n() {

#if defined(NLS) && defined(HAVE_CATCLOSE)
    if (s_catalog_fd != (nl_catd)-1)
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

#if defined(NLS) && defined(HAVE_CATGETS)
    if (s_catalog_fd != INVALID_CATALOG) {
        const char *ret = catgets(s_catalog_fd, set_number, message_number, default_message);
        // can't translate, leave it in raw ascii (utf-8 compatible)
        if (ret == default_message || ret == NULL)
            return default_message;

        if (!m_utf8_translate && translate_fb)
            // Local input, UTF-8 output
            return FbStringUtil::LocaleStrToFb(ret);
        else if (m_utf8_translate && !translate_fb)
            // UTF-8 input, local output
            return FbStringUtil::FbStrToLocale(ret);
        else
            // UTF-8 input, UTF-8 output OR
            // local input, local output
            return ret;
    }
    else
#endif // NLS && HAVE_CATGETS
        return default_message;
}

} // end namespace FbTk
