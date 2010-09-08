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

#include <errno.h>

using std::string;
using std::map;
using std::list;


#ifdef HAVE_SETLOCALE
#include <locale.h>
#endif //HAVE_SETLOCALE

namespace {

// use to map <font1>|<font2>|<font3> => <fontthatworks>
typedef map<string, string> StringMap;
typedef StringMap::iterator StringMapIt;
StringMap lookup_map;

// stores <fontthatworks and the fontimp
typedef map<string, FbTk::FontImp* > FontCache;
typedef FontCache::iterator FontCacheIt;
FontCache font_cache;


void resetEffects(FbTk::Font& font) {
    font.setHalo(false);
    font.setHaloColor(FbTk::Color("white", DefaultScreen(FbTk::App::instance()->display())));
    font.setShadow(false);
    font.setShadowColor(FbTk::Color("black", DefaultScreen(FbTk::App::instance()->display())));
    font.setShadowOffY(2);
    font.setShadowOffX(2);
}

} // end nameless namespace



namespace FbTk {

bool Font::s_multibyte = false;
bool Font::s_utf8mode = false;


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
    m_halo(false), m_halo_color("white", DefaultScreen(App::instance()->display()))
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
        s_utf8mode = FbStringUtil::haveUTF8();
    }

    if (name != 0) {
        load(name);
    }

}

Font::~Font() {
}

bool Font::load(const string &name) {

    if (name.size() == 0)
        return false;

    StringMapIt lookup_entry;
    FontCacheIt cache_entry;

    // check if one of <font1>|<font2>|<font3> is already there
    if ((lookup_entry = lookup_map.find(name)) != lookup_map.end() &&
            (cache_entry = font_cache.find(lookup_entry->second)) != font_cache.end()) {
        m_fontstr = cache_entry->first;
        m_fontimp = cache_entry->second;
        resetEffects(*this);
        return true;
     }

    // split up the namelist
    typedef list<string> StringList;
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
            resetEffects(*this);
            return true;
        }

        FontImp* tmp_font(0);

        // Xft and X/Xmb fonts have different defaults
        // (fixed doesn't really work right with Xft, especially rotated)

        // HOWEVER, note that if a Xft-style font is requested (not start with "-"), and
        // it turns out to be a bitmapped XFont, then Xft will load it, BUT it does not 
        // currently (5jan2007) rotate bitmapped fonts (ok-ish), nor adjust the baseline for its
        // lack of rotation (not ok: messes up placement). I can't see a neat way around this, 
        // other than the user re-specifying their font explicitly in XFont form so we don't use the
        // Xft backend.

        std::string realname = *name_it;

#ifdef USE_XFT
        if ((*name_it)[0] != '-') {

            if (*name_it == "__DEFAULT__")
                realname = "monospace";

            tmp_font = new XftFontImp(0, s_utf8mode);
        }
#endif // USE_XFT

        if (!tmp_font) {
            if (*name_it == "__DEFAULT__")
                realname = "fixed";

#ifdef USE_XMB

            if (s_multibyte || s_utf8mode) {
                tmp_font = new XmbFontImp(0, s_utf8mode);
            } else // basic font implementation
#endif // USE_XMB
    {
               tmp_font = new XFontImp();
    }
        }

        if (tmp_font && tmp_font->load(realname.c_str())) {
            lookup_map[name] = (*name_it);
            m_fontimp = tmp_font;
            font_cache[(*name_it)] = tmp_font;
            m_fontstr = name;
            resetEffects(*this);
            return true;
        }

        delete tmp_font;
    }

    return false;
}

unsigned int Font::textWidth(const char* text, unsigned int size) const {
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
                    const char* text, size_t len, int x, int y,
                    Orientation orient) const {

    if (!text || !*text || len == 0)
        return;

    // draw "effects" first
    if (m_shadow) {
        FbTk::GContext shadow_gc(w);
        shadow_gc.setForeground(m_shadow_color);
        m_fontimp->drawText(w, screen, shadow_gc.gc(), text, len,
                 x + m_shadow_offx, y + m_shadow_offy, orient);
    } else if (m_halo) {
        FbTk::GContext halo_gc(w);
        halo_gc.setForeground(m_halo_color);
        m_fontimp->drawText(w, screen, halo_gc.gc(), text, len, x + 1, y + 1, orient);
        m_fontimp->drawText(w, screen, halo_gc.gc(), text, len, x - 1, y + 1, orient);
        m_fontimp->drawText(w, screen, halo_gc.gc(), text, len, x - 1, y - 1, orient);
        m_fontimp->drawText(w, screen, halo_gc.gc(), text, len, x + 1, y - 1, orient);
    }

    m_fontimp->drawText(w, screen, gc, text, len, x, y, orient);


}

}

