// i18n.hh for Fluxbox Window Manager
// Copyright (c) 2001 - 2002 Henrik Kinnunen (fluxgen@linuxmail.org)
//
// i18n.hh for Blackbox - an X11 Window manager
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

// $Id: I18n.hh,v 1.2 2003/12/08 17:29:24 fluxgen Exp $

#ifndef	 I18N_HH
#define	 I18N_HH

#include "../nls/blackbox-nls.hh"

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif // HAVE_CONFIG_H

#ifdef HAVE_LOCALE_H
#include <locale.h>
#endif // HAVE_LOCALE_H

#ifdef HAVE_NL_TYPES_H
// this is needed for linux libc5 systems
extern "C" {
#include <nl_types.h>
}
#endif // HAVE_NL_TYPES_H

#include <string>

class I18n {
public:
    static I18n *instance();
    inline const char *getLocale() const { return m_locale.c_str(); }
    inline bool multibyte() const { return m_multibyte; }
    inline const nl_catd &getCatalogFd() const { return m_catalog_fd; }

    const char *getMessage(int set_number, int message_number, 
                           const char *default_messsage = 0) const;
    void openCatalog(const char *catalog);
private:
    I18n();
    ~I18n();
    std::string m_locale;
    bool m_multibyte;
    nl_catd m_catalog_fd;

};

void NLSInit(const char *);

#endif // I18N_HH
