// Font.cc
// Copyright (c) 2002 Henrik Kinnunen (fluxgen@linuxmail.org)
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

//$Id: Font.cc,v 1.6 2003/12/01 19:57:01 fluxgen Exp $


#include "Font.hh"
#include "FontImp.hh"

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
#include <cstring>
#include <cstdlib>
#include <typeinfo>
using namespace std;

#ifdef HAVE_SETLOCALE
#include <locale.h>
#endif //HAVE_SETLOCALE

namespace FbTk {

bool Font::m_multibyte = false; 
bool Font::m_utf8mode = false;

Font::Font(const char *name, bool antialias):
    m_fontimp(0),
    m_antialias(false), m_rotated(false), m_shadow(false) {
	
    // MB_CUR_MAX returns the size of a char in the current locale
    if (MB_CUR_MAX > 1) // more than one byte, then we're multibyte
        m_multibyte = true;

    char *s; // temporary string for enviroment variable
    // check for utf-8 mode
    if (((s = getenv("LC_ALL")) && *s) ||
        ((s = getenv("LC_CTYPE")) && *s) ||
        ((s = getenv("LANG")) && *s)) {
        if (strstr(s, "UTF-8"))
            m_utf8mode = true;
    }

    // create the right font implementation
    // antialias is prio 1
#ifdef USE_XFT
    if (antialias) {
        m_fontimp.reset(new XftFontImp(0, m_utf8mode));
        m_antialias = true;
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
        if (!m_fontimp->load("fixed")) // if that failes too, output warning
            cerr<<"Warning: can't load fallback font 'fixed'."<<endl;
    }

    m_antialias = flag;
}

bool Font::load(const std::string &name) {
    if (name.size() == 0)
        return false;
    // copy name so we can manipulate it
    std::string new_name = name;

    m_shadow = false;

    // find font option "shadow"	
    size_t start_pos = new_name.find_first_of(':');
    if (start_pos != std::string::npos) {        
        size_t shadow_pos = new_name.find("shadow", start_pos);
        if (shadow_pos != std::string::npos) {
            m_shadow = true;
            // erase "shadow" since it's not a valid option for the font
            new_name.erase(shadow_pos, 6);
            
            // is the option row empty?
            if (new_name.find_first_not_of("\t ,", start_pos + 1) == std::string::npos)
                new_name.erase(start_pos); // erase the ':' and the rest of the line
            else {
                // there might be some options left so we need to remove the ","
                // before/after "shadow" option
                size_t pos = new_name.find_last_not_of("\t ", shadow_pos);
                if (pos != std::string::npos) {
                    if (new_name[pos] == ',')
                        new_name.erase(pos, 1);
                
                }

                // ok, we removed the "," and "shadow" now we need to determine
                // if we need to remove the ":" , so we search for anything except
                // \t and space and if we dont find anything the ":" is removed
                if (new_name.find_first_not_of("\t ", start_pos + 1) == std::string::npos)
                    new_name.erase(start_pos, 1);               

            }

        }
    }

    m_fontstr = name;
    return m_fontimp->load(new_name.c_str());
}

unsigned int Font::textWidth(const char * const text, unsigned int size) const {
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
void Font::drawText(Drawable w, int screen, GC gc,
                    const char *text, size_t len, int x, int y, 
                    bool rotate) const {
    if (text == 0 || len == 0)
        return;

    // so we don't end up in a loop with m_shadow
    static bool first_run = true; 

    // draw shadow first
    if (first_run && m_shadow) {
        FbTk::GContext shadow_gc(w);
        shadow_gc.setForeground(FbTk::Color("black", screen));
        first_run = false; // so we don't end up in a loop
        drawText(w, screen, shadow_gc.gc(), text, len, x + 1, y + 1);
        first_run = true;
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

            font->drawText(w, screen, gc, text, len, x, y);
            font->setRotate(true); // enable rotation
        } catch (std::bad_cast &bc) {
            // draw normal...
            m_fontimp->drawText(w, screen, gc, text, len, x, y);
        }

    } else
        m_fontimp->drawText(w, screen, gc, text, len, x, y);		


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
