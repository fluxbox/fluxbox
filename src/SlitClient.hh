// SlitClient.hh for fluxbox
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

// $Id$

#ifndef SLITCLIENT_HH
#define SLITCLIENT_HH

#include "NotCopyable.hh"

#include <X11/Xlib.h>

#include <string>

class BScreen;

/// holds slit client info
class SlitClient: private FbTk::NotCopyable {
public:
    /// For adding an actual window
    SlitClient(BScreen *screen, Window win);
    /// For adding a placeholder
    explicit SlitClient(const char *name);

    inline const std::string &matchName() const { return m_match_name; }
    inline Window window() const { return m_window; }
    inline Window clientWindow() const { return m_client_window; }
    inline Window iconWindow() const { return m_icon_window; }
    inline int x() const { return m_x; }
    inline int y() const { return m_y; }
    inline unsigned int width() const { return m_width; }
    inline unsigned int height() const { return m_height; }
    bool visible() const { return m_visible; }


    void setIconWindow(Window win) { m_icon_window = win; }
    void setWindow(Window win) { m_window = win; }
    void move(int x, int y) { m_x = x; m_y = y; }
    void resize(unsigned int width, unsigned int height) { m_width = width; m_height = height; }
    void moveResize(int x, int y, unsigned int width, unsigned int height) { m_x = x; m_y = y; m_width = width; m_height = height; }
    void hide();
    void show();
    void setVisible(bool value) { m_visible = value; }

    void initialize(BScreen *screen = 0, Window win= None);
    void disableEvents();
    void enableEvents();

private:
    std::string m_match_name;
    Window m_window, m_client_window, m_icon_window;
    int m_x, m_y;
    unsigned int m_width, m_height;
    bool m_visible; ///< whether the client should be visible or not
};

#endif // SLITCLIENT_HH
