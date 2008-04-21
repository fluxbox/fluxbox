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

//use GNU extensions
#ifndef	 _GNU_SOURCE
#define	 _GNU_SOURCE
#endif // _GNU_SOURCE

#include "I18n.hh"
#include "FileUtil.hh"

#include <X11/Xlocale.h>

#ifdef HAVE_CSTDLIB
  #include <cstdlib>
#else
  #include <stdlib.h>
#endif
#ifdef HAVE_CSTRING
  #include <cstring>
#else
  #include <string.h>
#endif
#ifdef HAVE_CSTDIO
  #include <cstdio>
#else
  #include <stdio.h>
#endif

#include <iostream>

using std::cerr;
using std::endl;
using std::string;

namespace FbTk {

void NLSInit(const char *catalog) {
    FbStringUtil::init();
    I18n *i18n = I18n::instance();
    i18n->openCatalog(catalog);
}


I18n::I18n():m_multibyte(false), m_utf8_translate(false), m_catalog_fd((nl_catd)(-1)) {
#ifdef 	HAVE_SETLOCALE
    //make sure we don't get 0 to m_locale string
    char *temp = setlocale(LC_MESSAGES, "");
    m_locale = ( temp ?  temp : "");
    if (m_locale.empty()) {
        cerr<<"Warning: Failed to set locale, reverting to \"C\""<<endl;
#endif // HAVE_SETLOCALE

        m_locale = "C";

#ifdef	HAVE_SETLOCALE

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
#endif // HAVE_SETLOCALE
}


I18n::~I18n() {

#if defined(NLS) && defined(HAVE_CATCLOSE)
    if (m_catalog_fd != (nl_catd)-1)
        catclose(m_catalog_fd);
#endif // HAVE_CATCLOSE
}

I18n *I18n::instance() {
    static I18n singleton; //singleton object
    return &singleton;
}

void I18n::openCatalog(const char *catalog) {
#if defined(NLS) && defined(HAVE_CATOPEN)

    string catalog_filename = LOCALEPATH;
    catalog_filename += '/';
    catalog_filename += m_locale;
    catalog_filename += '/';
    catalog_filename += catalog;

    if (!FileUtil::isRegularFile(catalog_filename.c_str()) && m_locale != "C" && FbStringUtil::haveUTF8()) {
        // try the UTF-8 catalog, this also picks up situations where
        // the codeset somehow isn't specified

        // remove everything after @
        string::size_type index = m_locale.find('.');
        // erase all characters starting at index
        if (index != string::npos)
            m_locale.erase(index); 

        m_locale.append(".UTF-8");
        m_utf8_translate = true;

        catalog_filename = LOCALEPATH;
        catalog_filename += '/';
        catalog_filename += m_locale;
        catalog_filename += '/';
        catalog_filename += catalog;
    }

#ifdef MCLoadBySet
    m_catalog_fd = catopen(catalog_filename.c_str(), MCLoadBySet);
#else // !MCLoadBySet
    m_catalog_fd = catopen(catalog_filename.c_str(), NL_CAT_LOCALE);
#endif // MCLoadBySet

    if (m_catalog_fd == (nl_catd)-1) {
        cerr<<"Warning: Failed to open file("<<catalog_filename<<")"<<endl;
        cerr<<"for translation, using default messages."<<endl;
    }
	
#else // !HAVE_CATOPEN
	
    m_catalog_fd = (nl_catd)-1;
#endif // HAVE_CATOPEN
}


// Translate_FB means it'll become an FbString that goes to X for Fonts, 
// No translate means it stays in the local encoding, for printing to the
// console.
FbString I18n::getMessage(int set_number, int message_number, 
                             const char *default_message, bool translate_fb) const {

#if defined(NLS) && defined(HAVE_CATGETS)
    if (m_catalog_fd != (nl_catd)-1) {
        const char *ret = catgets(m_catalog_fd, set_number, message_number, default_message);
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
