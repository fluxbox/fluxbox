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

// $Id: i18n.hh,v 1.3 2002/01/09 14:11:20 fluxgen Exp $

#ifndef   _I18N_HH_
#define   _I18N_HH_

#ifdef    NLS
#  include "../nls/blackbox-nls.hh"
#endif // NLS

#ifdef    HAVE_LOCALE_H
#  include <locale.h>
#endif // HAVE_LOCALE_H

#ifdef    HAVE_NL_TYPES_H
// this is needed for linux libc5 systems
extern "C" {
#  include <nl_types.h>
}
#endif // HAVE_NL_TYPES_H


class I18n {
private:
  char *locale, *catalog_filename;
  int mb;
  nl_catd catalog_fd;


protected:
  I18n(void);
 
public:
	//so old compilators dont complain
	~I18n(void);
	
	static I18n *instance();
  inline const char *getLocale(void) const { return locale; }
  inline const char *getCatalogFilename(void) const { return catalog_filename; }
  
  inline const int &multibyte(void) const { return mb; }

  inline const nl_catd &getCatalogFd(void) const { return catalog_fd; }

  const char *getMessage(int, int, const char * = 0);
  void openCatalog(const char *);
};


//extern I18n *i18n;
extern void NLSInit(const char *);

#endif // __i18n_h
