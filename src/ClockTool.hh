// ClockTool.hh
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

// $Id: ClockTool.hh,v 1.1 2003/08/11 14:32:39 fluxgen Exp $

#ifndef CLOCKTOOL_HH
#define CLOCKTOOL_HH


#include "ToolbarItem.hh"
#include "TextButton.hh"

#include "FbTk/Observer.hh"
#include "FbTk/Resource.hh"
#include "FbTk/Timer.hh"

#include <string>

class ToolTheme;
class BScreen;

namespace FbTk {
class ImageControl;
class Subject;
}

class ClockTool:public ToolbarItem, public FbTk::Observer {
public:
    ClockTool(const FbTk::FbWindow &parent, ToolTheme &theme, BScreen &screen);
    virtual ~ClockTool();

    void move(int x, int y);
    void resize(unsigned int width, unsigned int height);
    void moveResize(int x, int y,
                    unsigned int width, unsigned int height);

    void show();
    void hide();
    unsigned int width() const;
    unsigned int height() const;

private:
    void updateTime();
    void update(FbTk::Subject *subj);
    void renderTheme();

    TextButton m_button;
    
    const ToolTheme &m_theme;
    BScreen &m_screen;
    Pixmap m_pixmap;
    FbTk::Timer m_timer;

    FbTk::Resource<std::string> m_timeformat;

};

#endif // CLOCKTOOL_HH
