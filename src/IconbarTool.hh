// IconbarTool.hh
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

// $Id: IconbarTool.hh,v 1.4 2003/08/13 10:11:14 fluxgen Exp $

#ifndef ICONBARTOOL_HH
#define ICONBARTOOL_HH

#include "ToolbarItem.hh"
#include "Container.hh"

#include "FbTk/Observer.hh"

#include <list>

#include <X11/Xlib.h>

class IconbarTheme;
class BScreen;
class IconButton;
class FluxboxWindow;

class IconbarTool: public ToolbarItem, public FbTk::Observer {
public:
    IconbarTool(const FbTk::FbWindow &parent, IconbarTheme &theme, BScreen &screen);
    ~IconbarTool();

    void move(int x, int y);
    void resize(unsigned int width, unsigned int height);
    void moveResize(int x, int y,
                    unsigned int width, unsigned int height);

    void update(FbTk::Subject *subj);
    void show();
    void hide();
    unsigned int width() const;
    unsigned int height() const;
    unsigned int borderWidth() const;

private:
    /// render single button that holds win
    void renderWindow(FluxboxWindow &win);
    /// render single button
    void renderButton(IconButton &button);
    /// render all buttons
    void renderTheme();
    /// destroy all icons
    void deleteIcons();

    BScreen &m_screen;
    Container m_icon_container;
    const IconbarTheme &m_theme;
    // cached pixmaps
    Pixmap m_focused_pm, m_unfocused_pm;
    Pixmap m_empty_pm; ///< pixmap for empty container

    typedef std::list<IconButton *> IconList;
    IconList m_icon_list;
};

#endif // ICONBARTOOL_HH
