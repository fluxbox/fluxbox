// Font.cc
// Copyright (c) 2002 - 2006 Henrik Kinnunen (fluxgen at fluxbox dot org)
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
#include "stringstream.hh"
#include "Font.hh"
#include "FontImp.hh"
#include "App.hh"

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
#include <map>
#include <typeinfo>
#include <langinfo.h>


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
   @param size number of chars to convert
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

// use to map <font1>|<font2>|<font3> => <fontthatworks>
typedef std::map<std::string, std::string> StringMap;
typedef StringMap::iterator StringMapIt;
StringMap lookup_map;

// stores <fontthatworks and the fontimp
typedef std::map<std::string, FbTk::FontImp* > FontCache;
typedef FontCache::iterator FontCacheIt;
FontCache font_cache;


void resetEffects(FbTk::Font* font) {
    font->setHalo(false);
    font->setHaloColor(FbTk::Color("white", DefaultScreen(FbTk::App::instance()->display())));
    font->setShadow(false);
    font->setShadowColor(FbTk::Color("black", DefaultScreen(FbTk::App::instance()->display())));
    font->setShadowOffY(2);
    font->setShadowOffX(2);
}

}; // end nameless namespace



namespace FbTk {

bool Font::s_multibyte = false; 
bool Font::s_utf8mode = false;


void Font::init() {
    // must be set before the first XFontSet is created
    setlocale(LC_CTYPE, "");
}

void Font::shutdown() {

    FontCacheIt fit;
    for (fit = font_cache.begin(); fit != font_cache.end(); fit++) {
        FontImp* font = fit->second;
        if (font) {
            FontCacheIt it;
            for (it = fit; it != font_cache.end(); it++)
                if (it->second == font)
                    it->second = 0;
            delete font;
        }
    }
}

Font::Font(const char *name):
    m_fontimp(0),
    m_shadow(false), m_shadow_color("black", DefaultScreen(App::instance()->display())), 
    m_shadow_offx(2), m_shadow_offy(2),
    m_halo(false), m_halo_color("white", DefaultScreen(App::instance()->display())),
#ifdef HAVE_ICONV
    m_iconv((iconv_t)(-1))
#else
    m_iconv(-1)
#endif // HAVE_ICONV
{
    // MB_CUR_MAX returns the size of a char in the current locale
    if (MB_CUR_MAX > 1) // more than one byte, then we're multibyte
        s_multibyte = true;

    // check for utf-8 mode
#ifdef CODESET
    char *locale_codeset = nl_langinfo(CODESET);
#else // openbsd doesnt have this (yet?)
    char *locale_codeset = 0;
#endif // CODESET

    if (locale_codeset && strcmp("UTF-8", locale_codeset) == 0) {
        s_utf8mode = true;
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
            s_utf8mode = false;
        } else {
            // success, we can now enable utf8mode 
            // and if antialias is on later we can recode
            // the non utf-8 string to utf-8 and use utf-8 
            // drawing functions
            s_utf8mode = true;
        }
#endif // HAVE_ICONV
    }

#ifdef DEBUG
    cerr<<"FbTk::Font m_iconv = "<<(int)m_iconv<<endl;
#endif // DEBUG

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

bool Font::load(const std::string &name) {

    if (name.size() == 0)
        return false;
 
    StringMapIt lookup_entry;
    FontCacheIt cache_entry;

    // check if one of <font1>|<font2>|<font3> is already there
    if ((lookup_entry = lookup_map.find(name)) != lookup_map.end() &&
            (cache_entry = font_cache.find(lookup_entry->second)) != font_cache.end()) {
        m_fontstr = cache_entry->first;
        m_fontimp = cache_entry->second;
        resetEffects(this);
        return true;
     }
    
    // split up the namelist
    typedef std::list<std::string> StringList;
    typedef StringList::iterator StringListIt;
    StringList names;
    FbTk::StringUtil::stringtok<StringList>(names, name, "|");
    
    StringListIt name_it;
    for (name_it = names.begin(); name_it != names.end(); name_it++) {
        FbTk::StringUtil::removeTrailingWhitespace(*name_it);
        FbTk::StringUtil::removeFirstWhitespace(*name_it);

        if ((cache_entry = font_cache.find(*name_it)) != font_cache.end()) {
            m_fontstr = cache_entry->first;
            m_fontimp = cache_entry->second;
            lookup_map[name] = m_fontstr;
            resetEffects(this);
            return true;
        }

        FontImp* tmp_font(0);
        
#ifdef USE_XFT
        if ((*name_it)[0] != '-')
            tmp_font = new XftFontImp(0, s_utf8mode);
#endif // USE_XFT
    
        if (!tmp_font) {
#ifdef USE_XMB
            if (s_multibyte || s_utf8mode)
                tmp_font = new XmbFontImp(0, s_utf8mode);
            else // basic font implementation
#endif // USE_XMB
                tmp_font = new XFontImp();
        }

        if (tmp_font && tmp_font->load((*name_it).c_str())) {
            lookup_map[name] = (*name_it);
            m_fontimp = tmp_font;
            font_cache[(*name_it)] = tmp_font;
            m_fontstr = name;
            resetEffects(this);
            return true;
        }
        
        delete tmp_font;
    }

    return false;
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

bool Font::validOrientation(FbTk::Orientation orient) {
    return m_fontimp->validOrientation(orient);
}

void Font::drawText(const FbDrawable &w, int screen, GC gc,
                    const char *text, size_t len, int x, int y, 
                    Orientation orient) const {
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
            shadow_gc.setForeground(m_shadow_color);
            first_run = false;
            drawText(w, screen, shadow_gc.gc(), real_text, len,
                     x + m_shadow_offx, y + m_shadow_offy, orient);
            first_run = true;
        } else if (m_halo) {
            FbTk::GContext halo_gc(w);
            halo_gc.setForeground(m_halo_color);
            first_run = false;
            drawText(w, screen, halo_gc.gc(), real_text, len, x + 1, y + 1, orient);
            drawText(w, screen, halo_gc.gc(), real_text, len, x - 1, y + 1, orient);
            drawText(w, screen, halo_gc.gc(), real_text, len, x - 1, y - 1, orient);
            drawText(w, screen, halo_gc.gc(), real_text, len, x + 1, y - 1, orient);
            first_run = true;
        }
    }

    m_fontimp->drawText(w, screen, gc, real_text, len, x, y, orient);

    if (rtext != 0)
        delete[] rtext;

}	

};

