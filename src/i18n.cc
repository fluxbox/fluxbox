// i18n.hh for Fluxbox Window Manager
// Copyright (c) 2001 - 2002 Henrik Kinnunen (fluxgen@linuxmail.org)
//
// i18n.cc for Blackbox - an X11 Window manager
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

// $Id: i18n.cc,v 1.7 2002/12/01 13:42:07 rathnor Exp $

//usr GNU extensions
#ifndef	 _GNU_SOURCE
#define	 _GNU_SOURCE
#endif // _GNU_SOURCE

#ifdef	HAVE_CONFIG_H
#include "../config.h"
#endif // HAVE_CONFIG_H

#include "i18n.hh"

#include <X11/Xlocale.h>

#ifdef STDC_HEADERS
#	include <stdlib.h>
#	include <string.h>
#	include <stdio.h>
#endif // STDC_HEADERS

#ifdef		HAVE_LOCALE_H
#	include <locale.h>
#endif // HAVE_LOCALE_H

#include <iostream>
using std::cerr;
using std::endl;
using std::string;

void NLSInit(const char *catalog) {
    I18n *i18n = I18n::instance();
    i18n->openCatalog(catalog);
}


I18n::I18n():m_multibyte(false), m_catalog_fd((nl_catd)(-1)) {
#ifdef		HAVE_SETLOCALE
    //make sure we don't get 0 to m_locale string
    char *temp = setlocale(LC_ALL, "");
    m_locale = ( temp ?  temp : ""); 
    if (m_locale.size() == 0) {
        cerr<<"Warning: Failed to set locale, reverting to \"C\""<<endl;
#endif // HAVE_SETLOCALE
        m_locale = "C";
#ifdef		HAVE_SETLOCALE
    } else {		
        // MB_CUR_MAX returns the size of a char in the current locale
        if (MB_CUR_MAX > 1)
            m_multibyte = true;
		
        // truncate any encoding off the end of the locale
				
        string::size_type index = m_locale.find('@');
        if (index != string::npos)
            m_locale.erase(index); //erase all characters starting at index 				
		
        index = m_locale.find('.');
        if (index != string::npos) 
            m_locale.erase(index); //erase all characters starting at index 
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

#ifdef		MCLoadBySet
    m_catalog_fd = catopen(catalog_filename.c_str(), MCLoadBySet);
#else // !MCLoadBySet
    m_catalog_fd = catopen(catalog_filename.c_str(), NL_CAT_LOCALE);
#endif // MCLoadBySet

    if (m_catalog_fd == (nl_catd)-1)
        cerr<<"Warning: Failed to open catalog, using default messages."<<endl;
	
#else // !HAVE_CATOPEN
	
    m_catalog_fd = (nl_catd)-1;
#endif // HAVE_CATOPEN
}


const char *I18n::getMessage(int set_number, int message_number, const char *default_message) {

#if defined(NLS) && defined(HAVE_CATGETS)
    if (m_catalog_fd != (nl_catd)-1)
        return (const char *) catgets(m_catalog_fd, set_number, message_number, default_message);
    else
#endif // NLS && HAVE_CATGETS
        return default_message;
}
