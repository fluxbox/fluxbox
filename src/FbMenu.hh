// FbMenu.hh for Fluxbox Window Manager
// Copyright (c) 2003 Henrik Kinnunen (fluxgen at users.sourceforge.net)
//                and Simon Bowden    (rathnor at users.sourceforge.net)
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

// $Id: FbMenu.hh,v 1.3 2003/12/10 23:08:03 fluxgen Exp $

#ifndef FBMENU_HH
#define FBMENU_HH

#include "Menu.hh"
#include "XLayerItem.hh"
#include <memory>

class MenuTheme;
class Shape;
/// a layered and shaped menu
class FbMenu:public FbTk::Menu {
public:
    FbMenu(MenuTheme &tm, FbTk::ImageControl &imgctrl,
           FbTk::XLayer &layer);
    ~FbMenu();
    void clearWindow();
    void raise() { m_layeritem.raise(); }
    void lower() { m_layeritem.lower(); }
    void reconfigure();
private:
    FbTk::XLayerItem m_layeritem;
    std::auto_ptr<Shape> m_shape;
};

#endif // FBMENU_HH

