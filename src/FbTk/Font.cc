// Font.cc
// Copyright (c) 2002-2004 Henrik Kinnunen (fluxgen at users.sourceforge.net)
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

//$Id$


#include "StringUtil.hh"
#include "Font.hh"
#include "FontImp.hh"
#include "I18n.hh"

#ifdef    HAVE_CONFIG_H
#include "config.h"
#endif // HAVE_CONFIG_H

// for antialias 
#ifdef USE_XFT
#include "XftFontImp.hh"
#endif // USE_XFT

// for multibyte support
#ifdef USE_XMB
#include "XmbFontImp.hh"
#endif //USE_XMB

// standard font system
#include "XFontImp.hh"

#include "GContext.hh"
//use gnu extensions
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif //_GNU_SOURCE

#ifndef __USE_GNU
#define __USE_GNU
#endif //__USE_GNU

#include <iostream> 
#ifdef HAVE_CSTRING
  #include <cstring>
#else
  #include <string.h>
#endif
#ifdef HAVE_CSTDLIB
  #include <cstdlib>
#else
  #include <stdlib.h>
#endif
#include <list>
#include <typeinfo>
#include <langinfo.h>

#ifdef HAVE_SSTREAM
#include <sstream>
#define FB_istringstream istringstream
#elif HAVE_STRSTREAM
#include <strstream>
#define FB_istringstream istrstream
#else
#error "You dont have sstream or strstream headers!"
#endif // HAVE_STRSTREAM

#ifdef HAVE_CSTDLIB
  #include <cstdlib>
#else
  #include <stdlib.h>
#endif

using namespace std;


namespace {

#ifdef HAVE_SETLOCALE
#include <locale.h>
#endif //HAVE_SETLOCALE

#ifdef HAVE_ICONV
/**
   Recodes the text from one encoding to another
   assuming cd is correct
   @param cd the iconv type
   @param msg text to be converted
   @param len number of chars to convert
   @return the recoded string, or 0 on failure
*/
char* recode(iconv_t cd,
             const char *msg, size_t size) {

    // If empty message, yes this can happen, return
    if(strlen(msg) == 0 || size == 0) 
        return 0;

    if(strlen(msg) < size)
        size = strlen(msg);
    
    size_t inbytesleft = size;
    size_t outbytesleft = 4*inbytesleft;
    char *new_msg = new char[outbytesleft];
    char *new_msg_ptr = new_msg;
    char *msg_ptr = strdup(msg);
    char *orig_msg_ptr = msg_ptr; // msg_ptr modified in iconv call
    size_t result = (size_t)(-1);

#ifdef HAVE_CONST_ICONV    
    result = iconv(cd, (const char**)(&msg_ptr), &inbytesleft, &new_msg, &outbytesleft);
#else
    result = iconv(cd, &msg_ptr, &inbytesleft, &new_msg, &outbytesleft);
#endif  // HAVE_CONST_ICONV

    if (result == (size_t)(-1)) {
        // iconv can fail for three reasons
        // 1) Invalid multibyte sequence is encountered in the input
        // 2) An incomplete multibyte sequence 
        // 3) The output buffer has no more room for the next converted character.
        // So we the delete new message and return original message
        delete[] new_msg_ptr;
        free(orig_msg_ptr);
        return 0;
    }
    free(orig_msg_ptr);

    *new_msg = '\0';
 
    if(inbytesleft != 0) {
        delete[] new_msg_ptr;
        return 0;
    }

    return new_msg_ptr;
}
#else

char *recode(int cd,
             const char *msg, size_t size) {
    return 0;
}
#endif // HAVE_ICONV

int extract_halo_options(const std::string& opts, std::string& color) {
   std::list< std::string > tokens;
   size_t sep= opts.find_first_of(':');

   if ( sep == std::string::npos )
       return 1;

   FbTk::StringUtil::stringtok(tokens, opts.substr(sep + 1, opts.length()), ";");
   tokens.unique();
   std::list< std::string >::const_iterator token;

   for ( token= tokens.begin(); token != tokens.end(); token++ ) {
       if ( (*token).find("color=", 0) != std::string::npos ) {
           size_t s= (*token).find_first_of('=');
           std::string c= (*token).substr(s + 1, (*token).length());
           if ( !c.empty() )
               std::swap(color, c);
       }
   }

   return 1;
}

int extract_shadow_options(const std::string& opts, 
                           std::string& color, 
                           int& offx, int& offy) {

   std::list< std::string > tokens;
   size_t sep= opts.find_first_of(':');

   if ( sep == std::string::npos )
       return 1;

   FbTk::StringUtil::stringtok(tokens, opts.substr(sep + 1, opts.length()), ";");
   tokens.unique();
   std::list< std::string >::const_iterator token;

   for ( token= tokens.begin(); token != tokens.end(); token++ ) {
       if ( (*token).find("color=", 0) != std::string::npos ) {
           size_t s= (*token).find_first_of('=');
           std::string c= (*token).substr(s + 1, (*token).length());
           if ( !c.empty() )
               std::swap(color, c);
       }
       else if ( (*token).find("offsetx=", 0) != std::string::npos ) {
           size_t s= (*token).find_first_of('=');
           FB_istringstream o((*token).substr(s + 1, (*token).length()));
           if ( !o.eof() ) {
               o >> offx;
           }
       }
       else if ( (*token).find("offsety=", 0) != std::string::npos ) {
           size_t s= (*token).find_first_of('=');
           FB_istringstream o((*token).substr(s + 1, (*token).length()));
           if ( !o.eof() ) {
               o >> offy;
           }
       }
   }

   return 1;

};

}; // end nameless namespace



namespace FbTk {

bool Font::m_multibyte = false; 
bool Font::m_utf8mode = false;

// some initialisation for using fonts
void fontInit() {
   setlocale(LC_CTYPE, "");
}

Font::Font(const char *name, bool antialias):
    m_fontimp(0),
    m_antialias(false), m_rotated(false), 
    m_shadow(false), m_shadow_color("#000000"), 
    m_shadow_offx(1), m_shadow_offy(1),
    m_halo(false), m_halo_color("#ffffff"),
#ifdef HAVE_ICONV
    m_iconv((iconv_t)(-1))
#else
    m_iconv(-1)
#endif // HAVE_ICONV
{

    // MB_CUR_MAX returns the size of a char in the current locale
    if (MB_CUR_MAX > 1) // more than one byte, then we're multibyte
        m_multibyte = true;

    // check for utf-8 mode
#ifdef CODESET
    char *locale_codeset = nl_langinfo(CODESET);
#else // openbsd doesnt have this (yet?)
    char *locale_codeset = 0;
#endif // CODESET

    if (locale_codeset && strcmp("UTF-8", locale_codeset) == 0) {
        m_utf8mode = true;
    } else if (locale_codeset != 0) {
        // if locale isn't UTF-8 we try to
        // create a iconv pointer so we can
        // convert non utf-8 strings to utf-8

#ifdef DEBUG
        cerr<<"FbTk::Font: check UTF-8 convert for codeset = "<<locale_codeset<<endl;
#endif // DEBUG

#ifdef HAVE_ICONV
        m_iconv = iconv_open("UTF-8", locale_codeset);
        if(m_iconv == (iconv_t)(-1)) {
            cerr<<"FbTk::Font: code error: from "<<locale_codeset<<" to: UTF-8"<<endl;
            // if we failed with iconv then we can't convert
            // the strings to utf-8, so we disable utf8 mode
            m_utf8mode = false;
        } else {
            // success, we can now enable utf8mode 
            // and if antialias is on later we can recode
            // the non utf-8 string to utf-8 and use utf-8 
            // drawing functions
            m_utf8mode = true;
        }
#endif // HAVE_ICONV
    }

#ifdef DEBUG
    cerr<<"FbTk::Font m_iconv = "<<(int)m_iconv<<endl;
#endif // DEBUG

    // create the right font implementation
    // antialias is prio 1
#ifdef USE_XFT
    if (antialias) {
        m_fontimp.reset(new XftFontImp(0, m_utf8mode));
    }
#endif //USE_XFT
    // if we didn't create a Xft font then create basic font
    if (m_fontimp.get() == 0) {
#ifdef USE_XMB
        if (m_multibyte || m_utf8mode)
            m_fontimp.reset(new XmbFontImp(0, m_utf8mode));
        else // basic font implementation
#endif // USE_XMB
            m_fontimp.reset(new XFontImp());
    }

    if (name != 0) {
        load(name);
    }

}

Font::~Font() {
#ifdef HAVE_ICONV
    if (m_iconv != (iconv_t)(-1))
        iconv_close(m_iconv);
#endif // HAVE_ICONV
}

void Font::setAntialias(bool flag) {
    bool loaded = m_fontimp->loaded();
#ifdef USE_XFT
    if (flag && !isAntialias() && !m_rotated) {
        m_fontimp.reset(new XftFontImp(m_fontstr.c_str(), m_utf8mode));
    } else if (!flag && isAntialias()) 
#endif // USE_XFT
	{
#ifdef USE_XMB
            if (m_multibyte || m_utf8mode)
                m_fontimp.reset(new XmbFontImp(m_fontstr.c_str(), m_utf8mode));
            else
#endif // USE_XMB
                m_fontimp.reset(new XFontImp(m_fontstr.c_str()));
	}

    if (m_fontimp->loaded() != loaded) { // if the new font failed to load, fall back to 'fixed'
        if (!m_fontimp->load("fixed")) {// if that failes too, output warning
            _FB_USES_NLS;
            cerr<<_FBTKTEXT(Error, CantFallbackFont, "Warning: can't load fallback font", "Attempt to load the last-resort default font failed")<<" 'fixed'."<<endl;
        } 
    }
    
    m_antialias = flag;
}

bool Font::load(const std::string &name) {
    if (name.size() == 0)
        return false;
    // default values for font options
    m_shadow = false;
    m_halo = false;

    // everything after ':' is a fontoption
    // -> extract 'halo' and 'shadow' and
    // load remaining fname
    size_t                   sep= name.find_first_of(':');

    if ( sep != std::string::npos ) {

        std::list< std::string > tokens;
        std::string              fname;

        fname= std::string(name.c_str(), sep);

        FbTk::StringUtil::stringtok(tokens, name.substr(sep + 1), ",");

        tokens.unique();
        bool firstone= true;
        std::list< std::string >::const_iterator token;

        // check tokens and extract extraoptions for halo and shadow
        for( token= tokens.begin(); token != tokens.end(); token++ ) {
            if ( (*token).find("halo",0) != std::string::npos ) {
                m_halo= true;
                extract_halo_options(*token, m_halo_color);
            }
            else if ( (*token).find("shadow", 0) != std::string::npos ) {
                m_shadow= true;
                extract_shadow_options(*token, m_shadow_color, m_shadow_offx, m_shadow_offy);
            }
            else {
                if ( !firstone )
                    fname+= ", ";
                else
                    firstone= false;
                fname= fname + ":" + *token;
            }
        }

        m_fontstr = fname;
    } else
        m_fontstr = name;

    return m_fontimp->load(m_fontstr.c_str());
}

unsigned int Font::textWidth(const char * const text, unsigned int size) const {
#ifdef HAVE_ICONV
    if (m_fontimp->utf8() && m_iconv != (iconv_t)(-1)) {
        char* rtext  = recode(m_iconv, text, size);
        if (rtext != 0)
            size = strlen(rtext);
        unsigned int r = m_fontimp->textWidth(rtext ? rtext : text, size);
        if (rtext != 0)
            delete[] rtext;
        return r;
    }
#endif // HAVE_ICONV
    return m_fontimp->textWidth(text, size);
}

unsigned int Font::height() const {
    return m_fontimp->height();
}

int Font::ascent() const {
    return m_fontimp->ascent();
}

int Font::descent() const { 
    return m_fontimp->descent();
}

void Font::drawText(const FbDrawable &w, int screen, GC gc,
                    const char *text, size_t len, int x, int y, 
                    bool rotate) const {
    if (text == 0 || len == 0)
        return;

    char* rtext = 0;

    // so we don't end up in a loop with m_shadow
    static bool first_run = true; 
    
#ifdef HAVE_ICONV
    if (m_fontimp->utf8() && m_iconv != (iconv_t)(-1) && first_run) {
        rtext = recode(m_iconv, text, len);
        if (rtext != 0) {
            len = strlen(rtext);
            // ok, we can't use utf8 mode since the string is invalid
        }
    } 
#endif // HAVE_ICONV

    const char *real_text = rtext ? rtext : text;

    // draw "effects" first
    if (first_run) {
        if (m_shadow) {
            FbTk::GContext shadow_gc(w);
            shadow_gc.setForeground(FbTk::Color(m_shadow_color.c_str(), screen));
            first_run = false;
            drawText(w, screen, shadow_gc.gc(), real_text, len,
                     x + m_shadow_offx, y + m_shadow_offy, rotate);
            first_run = true;
        } else if (m_halo) {
            FbTk::GContext halo_gc(w);
            halo_gc.setForeground(FbTk::Color(m_halo_color.c_str(), screen));
            first_run = false;
            drawText(w, screen, halo_gc.gc(), real_text, len, x + 1, y + 1, rotate);
            drawText(w, screen, halo_gc.gc(), real_text, len, x - 1, y + 1, rotate);
            drawText(w, screen, halo_gc.gc(), real_text, len, x - 1, y - 1, rotate);
            drawText(w, screen, halo_gc.gc(), real_text, len, x + 1, y - 1, rotate);
            first_run = true;
        }
    }

    if (!rotate && isRotated()) {
        // if this was called with request to not rotated the text
        // we just forward it to the implementation that handles rotation
        // currently just XFontImp
        // Using dynamic_cast just temporarly until there's a better solution 
        // to put in FontImp
        try {
            XFontImp *font = dynamic_cast<XFontImp *>(m_fontimp.get());
            font->setRotate(false); // disable rotation temporarly

            font->drawText(w, screen, gc, real_text, len, x, y);
            font->setRotate(true); // enable rotation
        } catch (std::bad_cast &bc) {
            // draw normal...
            m_fontimp->drawText(w, screen, gc, real_text, len, x, y);
        }

    } else
        m_fontimp->drawText(w, screen, gc, real_text, len, x, y);		

    if (rtext != 0)
        delete[] rtext;

}	

void Font::rotate(float angle) {
#ifdef USE_XFT
    // if we are rotated and we are changing to horiz text 
    // and we were antialiased before we rotated then change to XftFontImp
    if (isRotated() && angle == 0 && isAntialias())
        m_fontimp.reset(new XftFontImp(m_fontstr.c_str(), m_utf8mode));
#endif // USE_XFT
    // change to a font imp that handles rotated fonts (i.e just XFontImp at the moment)
    // if we're going to rotate this font
    if (angle != 0 && isAntialias() && !isRotated()) {
        m_fontimp.reset(new XFontImp(m_fontstr.c_str()));
        if (!m_fontimp->loaded()) // if it failed to load font, try default font fixed
            m_fontimp->load("fixed");
    }

    //Note: only XFontImp implements FontImp::rotate
    m_fontimp->rotate(angle);

    m_rotated = (angle == 0 ? false : true);
    m_angle = angle;
}


};

