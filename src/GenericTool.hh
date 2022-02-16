// GenericTool.hh for Fluxbox
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
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
// THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
// DEALINGS IN THE SOFTWARE.

#ifndef GENERICTOOL_HH
#define GENERICTOOL_HH

#include "ToolbarItem.hh"

#include "FbTk/NotCopyable.hh"
#include "FbTk/Signal.hh"

#include <memory>

class ToolTheme;

namespace FbTk {
class FbWindow;
template <class T> class ThemeProxy;
}

/// helper class for simple tools, i.e buttons etc
class GenericTool: public ToolbarItem, private FbTk::NotCopyable {
public:
    GenericTool(FbTk::FbWindow *new_window, ToolbarItem::Type type,
                FbTk::ThemeProxy<ToolTheme> &theme);
    virtual ~GenericTool();
    void move(int x, int y);
    void resize(unsigned int x, unsigned int y);
    void moveResize(int x, int y,
                    unsigned int width, unsigned int height);
    void show();
    void hide();

    unsigned int width() const;
    unsigned int height() const;
    unsigned int borderWidth() const;

    void parentMoved();

    const FbTk::ThemeProxy<ToolTheme> &theme() const { return m_theme; }
    FbTk::FbWindow &window() { return *m_window; }
    const FbTk::FbWindow &window() const { return *m_window; }

protected:
    virtual void renderTheme(int alpha);

private:
    void themeReconfigured();
    
    std::unique_ptr<FbTk::FbWindow> m_window;
    FbTk::ThemeProxy<ToolTheme> &m_theme;
    FbTk::SignalTracker m_tracker;
};

#endif // GENERICTOOL_HH
