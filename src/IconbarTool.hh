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

// $Id: IconbarTool.hh,v 1.8 2003/09/10 11:08:14 fluxgen Exp $

#ifndef ICONBARTOOL_HH
#define ICONBARTOOL_HH

#include "ToolbarItem.hh"
#include "Container.hh"

#include "FbTk/Observer.hh"
#include "FbTk/Resource.hh"
#include "FbTk/Menu.hh"

#include <X11/Xlib.h>

#include <list>

class IconbarTheme;
class BScreen;
class IconButton;
class FluxboxWindow;

class IconbarTool: public ToolbarItem, public FbTk::Observer {
public:
    typedef std::list<IconButton *> IconList;
    /// iconbar mode
    enum Mode {
        NONE, ///< no icons
        ICONS,  ///< all icons from all workspaces
        WORKSPACEICONS,  ///< icons on current workspace
        WORKSPACE, ///< all windows and all icons on current workspace
        ALLWINDOWS ///< all windows and all icons from all workspaces
    };

    IconbarTool(const FbTk::FbWindow &parent, IconbarTheme &theme, 
                BScreen &screen, FbTk::Menu &menu);
    ~IconbarTool();

    void move(int x, int y);
    void resize(unsigned int width, unsigned int height);
    void moveResize(int x, int y,
                    unsigned int width, unsigned int height);

    void update(FbTk::Subject *subj);
    void show();
    void hide();

    void setMode(Mode mode);

    unsigned int width() const;
    unsigned int height() const;
    unsigned int borderWidth() const;

    Mode mode() const { return *m_rc_mode; }

private:

    /// render single button that holds win
    void renderWindow(FluxboxWindow &win);
    /// render single button
    void renderButton(IconButton &button);
    /// render all buttons
    void renderTheme();
    /// destroy all icons
    void deleteIcons();
    /// remove a single window
    void removeWindow(FluxboxWindow &win);
    /// add a single window 
    void addWindow(FluxboxWindow &win);
    /// add icons to the list
    void updateIcons();
    /// add normal windows to the list
    void updateWorkspace();
    /// add all windows to the list
    void updateAllWindows();
    /// add a list of windows 
    void addList(std::list<FluxboxWindow *> &winlist);

    BScreen &m_screen;
    Container m_icon_container;
    const IconbarTheme &m_theme;
    // cached pixmaps
    Pixmap m_focused_pm, m_unfocused_pm;
    Pixmap m_empty_pm; ///< pixmap for empty container


    IconList m_icon_list;
    FbTk::Resource<Mode> m_rc_mode;

    FbTk::Menu m_menu;
};

#endif // ICONBARTOOL_HH
