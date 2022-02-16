// ToolFactory.hh for Fluxbox
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

#ifndef TOOLFACTORY_HH
#define TOOLFACTORY_HH

#include "ToolTheme.hh"
#include "IconbarTheme.hh"

#include "FbTk/NotCopyable.hh"

#include <memory>

class ToolbarItem;
class BScreen;
class Toolbar;

namespace FbTk {
class FbWindow;
}

/// creates toolbaritems
class ToolFactory:private FbTk::NotCopyable {
public:
    explicit ToolFactory(BScreen &screen);
    virtual ~ToolFactory() { }

    ToolbarItem *create(const std::string &name, const FbTk::FbWindow &parent, Toolbar &tbar);
    void updateThemes();
    int maxFontHeight();
    const BScreen &screen() const { return m_screen; }
    BScreen &screen() { return m_screen; }

private:
    BScreen &m_screen;
    ToolTheme m_clock_theme;
    std::unique_ptr<ToolTheme> m_button_theme;
    std::unique_ptr<ToolTheme> m_workspace_theme;
    std::unique_ptr<ToolTheme> m_systray_theme;
    IconbarTheme m_iconbar_theme, m_focused_iconbar_theme,
                 m_unfocused_iconbar_theme;
};

#endif // TOOLFACTORY_HH
