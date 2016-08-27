// FbMenu.hh for Fluxbox Window Manager
// Copyright (c) 2003 - 2006 Henrik Kinnunen (fluxgen at fluxbox dot org)
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

#ifndef FBMENU_HH
#define FBMENU_HH

#include <memory>

#include "FbTk/Menu.hh"
#include "FbTk/LayerItem.hh"
#include "FbTk/AutoReloadHelper.hh"

class FluxboxWindow;

namespace FbTk {
class MenuTheme;
}

/// a layered and shaped menu
class FbMenu:public FbTk::Menu {
public:

    static void setWindow(FluxboxWindow *win);
    static FluxboxWindow *window();


    FbMenu(FbTk::ThemeProxy<FbTk::MenuTheme> &tm, FbTk::ImageControl &imgctrl,
           FbTk::Layer &layer);
    virtual ~FbMenu() { }
    void raise() { m_layeritem.raise(); }
    void lower() { m_layeritem.lower(); }
    void buttonPressEvent(XButtonEvent &be);
    void buttonReleaseEvent(XButtonEvent &be);
    void keyPressEvent(XKeyEvent &ke);

    void setReloadHelper(FbTk::AutoReloadHelper *helper) { m_reloader.reset(helper); }
    FbTk::AutoReloadHelper *reloadHelper() { return m_reloader.get(); }

private:
    FbTk::LayerItem m_layeritem;
    std::unique_ptr<FbTk::AutoReloadHelper> m_reloader;
};

#endif // FBMENU_HH

