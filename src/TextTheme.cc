// TextTheme.cc
// Copyright (c) 2003 Henrik Kinnunen (fluxgen at users.sourceforge.net)
//                and Simon Bowden    (rathnor at users.sourceforge.net)
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
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
// THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
// DEALINGS IN THE SOFTWARE.

// $Id: TextTheme.cc,v 1.2 2003/08/12 00:20:47 fluxgen Exp $

#include "TextTheme.hh"

#include "FbTk/App.hh"

#include <X11/Xlib.h>

TextTheme::TextTheme(FbTk::Theme &theme, 
                     const std::string &name, const std::string &altname):
    m_font(theme, name + ".font", altname + ".Font"),
    m_text_color(theme, name + ".textColor", altname + ".TextColor"),
    m_justify(theme, name + ".justify", altname + ".Justify"),
    m_text_gc(XCreateGC(FbTk::App::instance()->display(),
                        RootWindow(FbTk::App::instance()->display(), 
                                   theme.screenNum()), 0, 0)) {
    // load default font
    m_font->load("fixed");
    update();
}

TextTheme::~TextTheme() {
    if (m_text_gc)
        XFreeGC(FbTk::App::instance()->display(), m_text_gc);
}

void TextTheme::update() {
    XGCValues gcv;
    gcv.foreground = m_text_color->pixel();
    XChangeGC(FbTk::App::instance()->display(), m_text_gc,
              GCForeground, &gcv);
}
