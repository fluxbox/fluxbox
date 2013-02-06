// WinButton.hh for Fluxbox Window Manager
// Copyright (c) 2003 - 2006 Henrik Kinnunen (fluxgen at fluxbox dot org)
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

#ifndef WINBUTTON_HH
#define WINBUTTON_HH

#include "FbTk/Button.hh"
#include "FbTk/FbPixmap.hh"
#include "FbTk/Signal.hh"

class FluxboxWindow;
class WinButtonTheme;

namespace FbTk{
class Color;
template <class T> class ThemeProxy;
}

/// draws and handles basic window button graphic
class WinButton:public FbTk::Button, public FbTk::SignalTracker {
public:
    /// draw type for the button
    enum Type {
        MAXIMIZE,
        MINIMIZE,
        SHADE,
        STICK,
        CLOSE,
        MENUICON,
        LEFT_HALF,
        RIGHT_HALF
    };

    WinButton(FluxboxWindow &listen_to, 
              FbTk::ThemeProxy<WinButtonTheme> &theme,
              FbTk::ThemeProxy<WinButtonTheme> &pressed,
              Type buttontype, const FbTk::FbWindow &parent, int x, int y, 
              unsigned int width, unsigned int height);
    /// override for drawing
    void exposeEvent(XExposeEvent &event);
    void buttonReleaseEvent(XButtonEvent &event);
    void setBackgroundPixmap(Pixmap pm);
    void setPressedPixmap(Pixmap pm);
    void setBackgroundColor(const FbTk::Color &color);
    void setPressedColor(const FbTk::Color &color);

    Pixmap getBackgroundPixmap() const { return getPixmap(m_theme); }
    Pixmap getPressedPixmap() const { return getPixmap(m_pressed_theme); }
    /// override for redrawing
    void clear();
    void updateAll();
private:
    void drawType();
    Pixmap getPixmap(const FbTk::ThemeProxy<WinButtonTheme> &) const;
    Type m_type; ///< the button type
    FluxboxWindow &m_listen_to;
    FbTk::ThemeProxy<WinButtonTheme> &m_theme, &m_pressed_theme;

    FbTk::FbPixmap m_icon_pixmap;
    FbTk::FbPixmap m_icon_mask;

    bool overrode_bg, overrode_pressed;
};

#endif // WINBUTTON_HH
