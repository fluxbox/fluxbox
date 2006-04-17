// WorkspaceNameTool.hh
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

#ifndef WORKSPACENAMETOOL_HH
#define WORKSPACENAMETOOL_HH

#include "ToolbarItem.hh"

#include "FbTk/TextButton.hh"
#include "FbTk/Observer.hh"

class BScreen;
class ToolTheme;

class WorkspaceNameTool: public ToolbarItem, public FbTk::Observer {
public:
    WorkspaceNameTool(const FbTk::FbWindow &parent, ToolTheme &theme, BScreen &screen);
    virtual ~WorkspaceNameTool();

    void move(int x, int y);
    void resize(unsigned int width, unsigned int height);
    void moveResize(int x, int y,
                    unsigned int width, unsigned int height);

    void show();
    void hide();
    unsigned int width() const;
    unsigned int height() const;
    unsigned int borderWidth() const;

    void update(FbTk::Subject *subj);
    FbTk::Button &button() { return m_button; }
    const FbTk::Button &button() const { return m_button; }
    void setOrientation(FbTk::Orientation orient);

    void parentMoved() { m_button.parentMoved(); }

private:
    void renderTheme(unsigned char alpha);
    void reRender();
    void updateSizing();
    FbTk::TextButton m_button;
    const ToolTheme &m_theme;
    BScreen &m_screen;
    Pixmap m_pixmap;
};

#endif // WORKSPACENAMETOOL_HH

