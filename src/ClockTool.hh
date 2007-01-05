// ClockTool.hh
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

#ifndef CLOCKTOOL_HH
#define CLOCKTOOL_HH


#include "ToolbarItem.hh"

#include "FbTk/TextButton.hh"
#include "FbTk/Observer.hh"
#include "FbTk/Resource.hh"
#include "FbTk/Timer.hh"
#include "FbTk/FbString.hh"

#include <string>

class ToolTheme;
class BScreen;

namespace FbTk {
class ImageControl;
class Subject;
class Menu;
}

class ClockTool:public ToolbarItem, public FbTk::Observer {
public:
    ClockTool(const FbTk::FbWindow &parent, ToolTheme &theme, BScreen &screen, FbTk::Menu &menu);
    virtual ~ClockTool();

    void move(int x, int y);
    void resize(unsigned int width, unsigned int height);
    void moveResize(int x, int y,
                    unsigned int width, unsigned int height);

    void show();
    void hide();
    void setTimeFormat(const std::string &format);
    // accessors
    unsigned int width() const;
    unsigned int height() const;
    unsigned int borderWidth() const;
    inline const std::string &timeFormat() const { return *m_timeformat; }

    void setOrientation(FbTk::Orientation orient);

    void parentMoved() { m_button.parentMoved(); }

private:
    void updateTime();
    void update(FbTk::Subject *subj);
    void renderTheme(unsigned char alpha);
    void reRender();
    void updateSizing();

    FbTk::TextButton m_button;
    
    const ToolTheme &m_theme;
    BScreen &m_screen;
    Pixmap m_pixmap;
    FbTk::Timer m_timer;

    FbTk::Resource<std::string> m_timeformat;

    FbTk::StringConvertor m_stringconvertor;
};

#endif // CLOCKTOOL_HH
