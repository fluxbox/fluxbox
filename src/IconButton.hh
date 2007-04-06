// IconButton.hh
// Copyright (c) 2003 - 2006 Henrik Kinnunen (fluxgen at fluxbox dot org)
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

// $Id$

#ifndef ICONBUTTON_HH
#define ICONBUTTON_HH

#include "FbTk/CachedPixmap.hh"
#include "FbTk/FbPixmap.hh"
#include "FbTk/Observer.hh"
#include "FbTk/TextButton.hh"

class Focusable;
class IconbarTheme;

class IconButton: public FbTk::TextButton, public FbTk::Observer {
public:
    IconButton(const FbTk::FbWindow &parent, IconbarTheme &theme,
               Focusable &window);
    virtual ~IconButton();

    void exposeEvent(XExposeEvent &event);
    void clear();
    void clearArea(int x, int y,
                   unsigned int width, unsigned int height,
                   bool exposure = false);
    void moveResize(int x, int y,
                    unsigned int width, unsigned int height);
    void resize(unsigned int width, unsigned int height);

    void reconfigTheme();

    void update(FbTk::Subject *subj);
    void setPixmap(bool use);

    Focusable &win() { return m_win; }
    const Focusable &win() const { return m_win; }

    bool setOrientation(FbTk::Orientation orient);

protected:
    void drawText(int x, int y, FbTk::FbDrawable *drawable_override);
private:
    void setupWindow();

    Focusable &m_win;
    FbTk::FbWindow m_icon_window;
    FbTk::FbPixmap m_icon_pixmap;
    FbTk::FbPixmap m_icon_mask;
    bool m_use_pixmap;

    IconbarTheme &m_theme;
    // cached pixmaps
    FbTk::CachedPixmap m_focused_pm, m_unfocused_pm;
};

#endif // ICONBUTTON_HH
