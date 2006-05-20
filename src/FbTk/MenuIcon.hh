// MenuIcon.hh for FbTk - Fluxbox ToolKit
// Copyright (c) 2004 Henrik Kinnunen (fluxgen at fluxbox dot org)
//                and Simon Bowden (rathnor at users.sourceforge.net)
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

// $Id$

#ifndef MENUICON_HH
#define MENUICON_HH

#include "MenuItem.hh"
#include "FbPixmap.hh"

#include <string>

namespace FbTk {

class MenuIcon: public MenuItem {
public:
    MenuIcon(const std::string &filename, FbString &label, int screen_num);
    void draw(FbDrawable &drawable, 
              const MenuTheme &theme,
              bool highlight,
              bool draw_foreground, bool draw_background,
              int x, int y,
              unsigned int width, unsigned int height) const;
    unsigned int width(const MenuTheme &item) const;
    void updateTheme(const MenuTheme &theme);
private:
    mutable FbPixmap m_pixmap, m_mask;
    const std::string m_filename;
};

} // end namespace FbTk

#endif // MENUICON_HH

